//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Generate COM+ byte codes from BIL.
//
//  File organization:
//
//      CodeGenerator.h    : definition of CodeGenerator class
//
//      CodeGenerator.cpp  : implementations of public CodeGenerator entry functions
//                           implementations of construction routines
//
//      Expressions.cpp    : methods for generating SX_<type> nodes (ie expr level)
//
//      Statements.cpp     : methods for generating SL_<type> nodes (ie stmt level)
//
//      CodeGenerator.inl  : definitions of helper routines
//                           code for inlined functions
//
//      Utilities.cpp      : common utility/helper routines
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

#define VBCodeGenTemporaryPrefix L"CG"

//-------------------------------------------------------------------------------------------------
//
// Initialize the code generator
//
void CodeGenerator::InitializeGenerator(ILTree::ILNode *ptreeFunc)
{
    m_ptreeFunc = ptreeFunc;
    m_pproc     = m_ptreeFunc->AsProcedureBlock().pproc;
    m_Project   = m_pproc->GetContainingProject();
    m_NoIntChecks = m_Project && m_Project->RemoveIntChecks() || m_pproc && m_pproc->IsSyntheticMethod() && m_pproc->PSyntheticMethod()->GetSyntheticKind() == SYNTH_TransientNoIntCheckSymbol;

    m_usMaxStackLvl          = 0;
    m_usCurStackLvl          = 0;
    m_usCurStmtLocal         = INVALID_LCL_INDEX;
    m_usOnErrorLocal         = INVALID_LCL_INDEX;
    m_usResumeLocal          = INVALID_LCL_INDEX;
    m_usCurLineLocal         = INVALID_LCL_INDEX;
    m_pcblkOnErrorHandler    = NULL;
    m_pcblkOnErrorFilter     = NULL;
    m_pcblkOnErrorFailure    = NULL;
    m_pcblkLoadForResume     = NULL;
    m_pcblkLoadForResumeNext = NULL;

    m_stackTypes.Clear();
    m_cblkCodeList.Reset();

    // Allocate the temp manager from which any code gen temporaries will be allocated
    //
    m_pCodeGenTemporaryManager = new (*m_pnra) TemporaryManager(
                m_pCompiler,
                m_Project->GetCompilerHost(),
                m_pproc,
                m_pnra,
                VBCodeGenTemporaryPrefix);

    // initialize code block logic
    CodeBuf_Create();
    m_pcblkCurrent = NewCodeBlock();

    if (WithinFunction())
    {
        m_pcblkEndProcLoadLocal = NewCodeBlock();
    }
    m_pcblkEndProc = NewCodeBlock();

    CheckRTLangHelper();
}

//========================================================================
// Initialize debug information for a method
//========================================================================

void CodeGenerator::InitializeDebugInformation
(
)
{
    if (!m_Project->GeneratePDB())
    {
        return;
    }

    // Allocate memory based on an estimate, we will resize it if more memory is needed
    m_pltblLineTbl = (LINETBL *)m_nraLineTable.Alloc(16 * sizeof(LINETBL));
    m_pltblCurrent = m_pltblLineTbl;
    m_pltblEnd = m_pltblLineTbl + 16;

    // Generate a NOP so that we can put a breakpoint on the line containing the Sub/Function foo()
    VSASSERT(m_ptreeFunc->bilop == SB_PROC, "GenerateMethod: expected Proc node");

    if (m_pproc->IsAsyncKeywordUsed() || m_pproc->IsIteratorKeywordUsed())
    {
        // Kick off methods should not map any source code.  Instead the MoveNext methods map the source code.
        return;
    }

    Location LineTableEntry = GetDebugBeginLocation(&m_ptreeFunc->AsProcedureBlock());

    if (LineTableEntry.IsValid())
    {
        UpdateLineTable(LineTableEntry);
        InsertNOP();
    }
}

//
// Get the method slots object for this method.  We create one if this method is not on
// the stack in an EnC session.
//
MethodSlots *
CodeGenerator::GetMethodSlots
(
    BCSYM_Proc *pproc
)
{
    MethodSlots *CurrentMethodSlots = NULL;

#if IDE
    mdToken tkMethod = m_pmdemit->GetToken(pproc);
    Builder *pBuilder = m_pmdemit->GetBuilder();

    if (pBuilder && pBuilder->IsENCBuilder())
    {
        CurrentMethodSlots = static_cast<ENCBuilder *>(pBuilder)->GetMethodSlotsFromStack(tkMethod);
    }
#endif

    if (CurrentMethodSlots == NULL)
    {
        // Since the MethodSlots will live longer than CodeGen,we use the Debug Space allocator to create them
        CurrentMethodSlots = (MethodSlots *)m_pnraDebugSpace->Alloc(sizeof(MethodSlots));
        CurrentMethodSlots->Init(m_pCompiler, m_pnraDebugSpace, m_pmdemit->GetToken(pproc));
    }

    return CurrentMethodSlots;
}


//
// Assign slot numbers to all the local variables in the method.
// We recurse down the executable-block hierarchy to assign slot numbers.  We build the scope hierarchy
// as we recurse.
//
void CodeGenerator::AssignBlockLocalSlots
(
    ILTree::ExecutableBlock &exblock,
    BlockScope *CurrentScope
)
{
    // Store this scope in the tree so we can conjur it up again when hooking up Open and Close codeblocks.
    exblock.LocalScope = CurrentScope;

    if (exblock.Locals)
    {
        BCSYM_NamedRoot *pLocal;

        BCITER_CHILD_SORTED iterLocals(exblock.Locals);
        while (pLocal = iterLocals.GetNext())
        {
            if (pLocal->IsAlias())
            {
                continue;
            }

            // He added a variable
            BCSYM_Variable *pvar = pLocal->PVariable();

            if (pvar->GetVarkind() == VAR_Param)
            {
                continue;
            }

            if (pvar->GetVarkind() == VAR_Const)
            {
                AddConstantToMethod(CurrentScope, pLocal->PVariable());
                continue;
            }

            AddVariableToMethod(
                CurrentScope,
                pLocal->PVariable(),
                pLocal->PVariable()->IsTemporary() ? VAR_IS_COMP_GEN : 0);

        } // while locals
    }

    // Iterate over the blocks that the current block contains
    for (ILTree::ILNode *ptreeStatementInBlock = exblock.ptreeChild;
         ptreeStatementInBlock;
         ptreeStatementInBlock = ptreeStatementInBlock->AsStatement().Next)
    {
        if (ptreeStatementInBlock->IsExecutableBlockNode())
        {
            BlockScope *NewChildScope = NULL;

            // A non-null scope implies that we are generating PDB information.
            // Therefore, we need to keep track of block scoping information.
            // If so, build a scope for this executable block
            //
            VSASSERT(!(!!CurrentScope ^ m_Project->GeneratePDB()),
                      "AssignBlockLocalSlots: The validity of CurrentScope must follow decision to generate pdb");

            if (CurrentScope)
            {
                // Allocate the blockscope in debug space because it needs to live longer than codegen
                NewChildScope = (BlockScope *)m_pnraDebugSpace->Alloc(sizeof(BlockScope));
                CurrentScope->ChildScopeList.InsertLast(NewChildScope);
            }

            AssignBlockLocalSlots(ptreeStatementInBlock->AsExecutableBlock(), NewChildScope);
        }
    }
}


/*****************************************************************************
// If the function is on the call stack we have to use the EnCDebugInfo
// to make sure we generate a function that EnC supports
//    Have the original set of locals defined with the same offset
//
*****************************************************************************/
void CodeGenerator::AssignLocalSlots()
{
    TemporaryIterator   iterTemp;
    TemporaryManager  * ptempmgr = m_ptreeFunc->AsProcedureBlock().ptempmgr;

    BCSYM_Variable *ActiveHandlerTemporary = NULL;
    BCSYM_Variable *ResumeTargetTemporary = NULL;
    BCSYM_Variable *CurrentLineTemp = NULL;
    BCSYM_Variable *CurrentStatementTemp = NULL;


    // First allocate the On Error temporary locals if any are needed
    if (m_ptreeFunc->AsProcedureBlock().fSeenOnErr)
    {
        ptempmgr->AllocateOnErrorTemporaries(ActiveHandlerTemporary, ResumeTargetTemporary);

        // When doing ENC, the presence of On Error will always cause the Resume temporary
        // to get allocated (to avoid a rude-edit when removing all Resumes).
        if (m_ptreeFunc->AsProcedureBlock().fSeenResume || m_Project->GenerateENCableCode())
        {
            CurrentStatementTemp = ptempmgr->AllocateCurrentStatementTemporary();
        }
    }

    // Allocate the line number temporary if needed.
    // When doing ENC, the presence of a line number always causes the temporary
    // to get allocated (to avoid a rude-edit annoyance when removing all Catch
    // blocks).
    if (NeedLineNumber() ||
        (m_ptreeFunc->AsProcedureBlock().fSeenLineNumber && m_Project->GenerateENCableCode()))
    {
        CurrentLineTemp = ptempmgr->AllocateCurrentLineTemporary();
    }

    // Assign slots by recursing down the method's block structure
    AssignBlockLocalSlots(m_ptreeFunc->AsProcedureBlock(), m_MethodScope);


    // Now iterate over the method's temps and assign their slot numbers.
    // The On Error temporary locals allocated above will appear in this list.
    //
    VSASSERT(ptempmgr, "Where's the tempmgr?  Every proc should have one!");
    iterTemp.Init(ptempmgr);

    while (Temporary *Current = iterTemp.GetNext())
    {
        AddVariableToMethod(m_MethodScope, Current->Symbol, VAR_IS_COMP_GEN);
    }

    // As a shortcut, cache the On Error slot numbers.
    if (ActiveHandlerTemporary)
    {
        // 
        m_usOnErrorLocal   = (unsigned short)ActiveHandlerTemporary->GetLocalSlot();
        m_usResumeLocal    = (unsigned short)ResumeTargetTemporary->GetLocalSlot();

        if (CurrentStatementTemp && m_ptreeFunc->AsProcedureBlock().fSeenResume)
        {
            m_usCurStmtLocal = (unsigned short)CurrentStatementTemp->GetLocalSlot();
        }
    }

    if (CurrentLineTemp)
    {
        m_usCurLineLocal = (unsigned short)CurrentLineTemp->GetLocalSlot();
    }
}


/*****************************************************************************
// Create the signature for the locals
*****************************************************************************/
void CodeGenerator::CreateLocalsSignature()
{
    VSASSERT(m_pmdemit->GetSignature() == NULL, "Can't have anything.");

    unsigned long cLocals = m_MethodSlots->LocalVariableList()->NumberOfEntries();

    m_pmdemit->EncodeInteger(IMAGE_CEE_CS_CALLCONV_LOCAL_SIG);
    m_pmdemit->EncodeInteger(cLocals);

    // Because the slot numbers may be scrambled in the linked list, we will walk the list
    // and populate an array with pointers to the nodes using the slot number as the array index.
    // Then we can iterate over the array to give us the slot numbers in order.
    LocalVariable ** SortedSlots = (LocalVariable**)m_pnra->Alloc(VBMath::Multiply(
        cLocals, 
        sizeof(LocalVariable *)));

    CSingleListIter<LocalVariable> IterLocals(m_MethodSlots->LocalVariableList());
    LocalVariable * CurrentLocalVariable;

    while (CurrentLocalVariable = IterLocals.Next())
    {
        VSASSERT(CurrentLocalVariable->SignatureSize >0 && CurrentLocalVariable->Signature,
                  "CreateLocalsSignature: this local has no signature!!");
        VSASSERT(CurrentLocalVariable->SlotNumber < cLocals,
                  "CreateLocalsSignature: invalid slot number!");
        VSASSERT(SortedSlots[CurrentLocalVariable->SlotNumber] == 0,
                  "CreateLocalsSignature: slot already occupied!");

        // assign into the array using slot number as index
        SortedSlots[CurrentLocalVariable->SlotNumber] = CurrentLocalVariable;
    }

    // iterate over the array to give us nodes in order of slot number
    for (unsigned long i = 0; i < cLocals; i++)
    {
        VSASSERT(SortedSlots[i] != NULL, "Bad slot configuration, ENC missed a bad local or temp variable deletion!");
        if (SortedSlots[i] != NULL) // VSWhidbey[387168] Guard against crashing due to bad slot configuration.
        {
            BYTE * pb;
            pb = m_pmdemit->EncodeExtendBuffer(SortedSlots[i]->SignatureSize);
            memcpy(pb, SortedSlots[i]->Signature, SortedSlots[i]->SignatureSize);
        }
    }
}

/*****************************************************************************
// Create temporaries required during codegen. Currently all such temporaries
// are short lived.
*****************************************************************************/
BCSYM_Variable* CodeGenerator::CreateCodeGenTemporary
(
    BCSYM *pType
)
{
    bool Reused = false;

    // Code gen currently uses only short lived temporaries
    BCSYM_Variable *pTemporary =
        m_pCodeGenTemporaryManager->AllocateShortLivedTemporary(
            pType,
            NULL,
            &Reused);

    if (!Reused)
    {
        Signature signature;
        BYTE *SignatureBuffer;
        unsigned long SigSize;

        m_pmdemit->StartNewSignature(&signature);

        // Error location not important here because the error would have already been reported
        // by this point of time, else in the worse case scenario, the error is reported on the
        // proc itself.
        m_pmdemit->EncodeType(pType, m_pproc->GetLocation());

        SigSize = m_pmdemit->GetSigSize();

        SignatureBuffer = (BYTE *)m_pnraDebugSpace->Alloc(sizeof(BYTE) * SigSize);
        memcpy(SignatureBuffer, m_pmdemit->GetSignature(), SigSize);

        LocalVariable *CurrentLocalVariable =
            m_MethodSlots->AddVariable(
                pTemporary,
                VAR_IS_COMP_GEN,
                SignatureBuffer,
                SigSize);

        m_pmdemit->ReleaseSignature(&signature);

        pTemporary->SetLocalSlot(CurrentLocalVariable->SlotNumber);

        if (m_MethodScope)
        {
            ScopeMember * NewScopeMember = (ScopeMember *)m_pnraDebugSpace->Alloc(sizeof(ScopeMember));
            NewScopeMember->MemberInfo = CurrentLocalVariable;
            m_MethodScope->ScopeMembers.InsertLast(NewScopeMember);
        }
    }

    return pTemporary;
}

/*****************************************************************************
// Assigns slots just for parameters.  Locals are handled elsewhere.
*****************************************************************************/
void CodeGenerator::AssignParameterSlots()
{
    BCITER_Parameters   iterParms;
    BCSYM_Param       * pparamNext;
    unsigned short      usNextParamIndex = 0;

    // Indices are assigned as follows. If Proc is a static, the parameters
    // are numbered from left to right starting from 0. If proc is
    // non-static, the parameters are numbered starting from 1. The 0 index
    // in this case is the 'this' parameter.
    // In the case that the procedure returns a variant, the retval is index
    // 0 (static) or 1 (virtual)
    //
    if (!m_pproc->IsShared())
    {
        usNextParamIndex = 1;
    }

    // Iterate over all parameters.  BCITER_CHILD does not guarantee
    // order of locals and parms so we need to iterate over paramters
    // separately.
    //
    m_pproc->InitParamIterator(&iterParms);
    while (pparamNext = iterParms.PparamNext())
    {
        // 
        BCSYM_NamedRoot *pnamedVariable = m_ptreeFunc->AsProcedureBlock().Locals->SimpleBind(pparamNext->GetName());
        VSASSERT(pnamedVariable, "have to have a local symbol!");

        pnamedVariable->PVariable()->SetLocalSlot(usNextParamIndex++);
    }
}


//========================================================================
// Initalize the variables (locals + parameters) for a method. Assigns
// each a slot and generates the local variable signature for the
// metadata.
//
// NOTE: If we encounter static variables that require construction, we
// will add them to the static hash
//========================================================================

void CodeGenerator::InitializeMethodSlots()
{
    VSASSERT(m_pmdemit->GetSignature() == NULL, "Can't have anything.");

    // Get slot info for this method, which may be pre-initalized if we're in an EnC session
    m_MethodSlots = GetMethodSlots(m_pproc);

    if (m_Project->GeneratePDB())
    {
        // Allocate the blockscope in debug space because it needs to live longer than codegen
        m_MethodScope = (BlockScope *)m_pnraDebugSpace->Alloc(sizeof(BlockScope));
    }
    else
    {
        m_MethodScope = NULL;
    }

    AssignParameterSlots();

    AssignLocalSlots();
}


//========================================================================
// Initializes on error handling for the method
//========================================================================

void
CodeGenerator::InitializeOnError
(
)
{
    // Setup on error handling for a method. But if the method is an instance
    // constructor, delay this because the MyBase.New, MyClass.New or initobj
    // needs to be generated outside the try blocked for the on error.
    //
    if (!m_pproc->IsInstanceConstructor())
    {
        StartOnErrorTryBlock();
    }
}

//========================================================================
// Build and start the on error try block for the method
//========================================================================

void CodeGenerator::StartOnErrorTryBlock()
{
    if (m_ptreeFunc->AsProcedureBlock().fSeenOnErr)
    {
        m_pcblkOnErrorFilter = NewCodeBlock();
        m_pcblkOnErrorHandler = NewCodeBlock();
        m_pcblkOnErrorFailure = NewCodeBlock();

        // Create the error handler table. The zeroth entry will always be
        // the default handler (i.e. throw the exception).  Entry 1 will
        // be the resume block and the following entries will be labels.

        long cEntries = 2 + m_ptreeFunc->AsProcedureBlock().OnErrorHandlerCount;

        m_ptblswOnError = AllocSwitchTable(cEntries);
        m_ptblswOnError->LowVal  = 0;
        m_ptblswOnError->cEntries = cEntries;

        //
        // Now build the Try and Catch block which describes the On Error mechanism
        //

        m_ptryOnError = (TRY_BLOCK *)m_pnra->Alloc(sizeof(TRY_BLOCK));

        TRY_BLOCK_HOLDER * ptryholder = (TRY_BLOCK_HOLDER *)m_pnra->Alloc(sizeof(TRY_BLOCK_HOLDER));
        ptryholder->ptry = m_ptryOnError;

        CATCH_BLOCK * pcatchOnError = (CATCH_BLOCK *)m_pnra->Alloc(sizeof(CATCH_BLOCK));


        SetAsCurrentCodeBlock(0);
        m_ptryOnError->pcblkTry = m_pcblkCurrent;

        m_ptryOnError->pcblkFinally = NULL;     // no finally for On Error mechanism
        m_ptryOnError->pcblkFallThrough = NULL; // no fallthrough condition needed for On Error
        m_ptryOnError->IsSynthetic = true;

        pcatchOnError->pcblkFilter = m_pcblkOnErrorFilter;
        pcatchOnError->psymType = NULL;  // this defaults to System.Exception
        pcatchOnError->ExceptionVarLocation.Invalidate();
        pcatchOnError->pcblkCatch = m_pcblkOnErrorHandler;
        pcatchOnError->pcblkLast = m_pcblkOnErrorHandler;

        m_ptryOnError->catchList.InsertFirst(pcatchOnError);

        VSASSERT(m_tryListPending.NumberOfEntries() == 0, "pending Try list is supposed to be empty!");
        VSASSERT(m_tryholderEncounteredList.NumberOfEntries() == 0, "encountered Try list is supposed to be empty!");

        // And insert the Try block because the On Error mechanism is now pending
        m_tryListPending.InsertFirst(m_ptryOnError);
        m_tryholderEncounteredList.InsertLast(ptryholder);

        // Build the Resume state if necessary.
        if (m_ptreeFunc->AsProcedureBlock().fSeenResume)
        {
            m_pcblkLoadForResume = NewCodeBlock();
            m_pcblkLoadForResumeNext = NewCodeBlock();
            m_pcblkResumeTableFallThrough = NewCodeBlock();
            // The 0th Resume table entry falls through and
            // branches to failure because the ResumeTarget should never be 0.
            AllocateResumeEntry(m_pcblkResumeTableFallThrough, 0);
        }
    }
}
/*====================================================================

Code generators for method preamble/postamble

====================================================================*/

//========================================================================
// generate the table that jumps to specific lines during a resume
//========================================================================

void CodeGenerator::GenerateResumeTable
(
)
{
    SWITCH_TABLE   * ptblswResume;
    RESUME_ADDRESS * presaddr;
    CSingleListIter<RESUME_ADDRESS> Iter(&m_cadrResumeList);
    unsigned long    cEntries;


    CODE_BLOCK * pcblkResumeTable = NewCodeBlock();  // the codeblock which contains the resume switch table

    VSASSERT(m_pcblkLoadForResume && m_pcblkLoadForResumeNext && m_pcblkResumeTableFallThrough,
             "GenerateResumeTable: must have codeblocks to for the Resume and Resume Next statements. : #11/13/2003#");

    // Unless a Resume <label> statement is present, this codeblock is unnecessary.
    // Emitting it when we know it to be dead causes stepping problems (VS #254579). is this still an issue?
    //
    if (m_ptreeFunc->AsProcedureBlock().fSeenResume)
    {
        // Resume statements will branch here.  Just load the resume local and
        // branch to the switch table
        SetAsCurrentCodeBlock(m_pcblkLoadForResume);

        StartHiddenIL();

        GenerateLoadLocal(m_usResumeLocal, GetSymbolForVtype(t_i4));
        // This load is consumed by the switch table
        AddToCurStack(-1);
        m_stackTypes.PopOrDequeue();
        EndCodeBuffer(CEE_BR_S, pcblkResumeTable);
    }

    // Resume Next statements will branch here.  Increment the resume local and
    // fall through to the switch table
    SetAsCurrentCodeBlock(m_pcblkLoadForResumeNext);
    StartHiddenIL();
    GenerateLoadLocal(m_usResumeLocal, GetSymbolForVtype(t_i4));
    GenerateLiteralInt(1);
    EmitOpcode(CEE_ADD, GetSymbolForVtype(t_i4));

    // now start generating the resume switch table
    SetAsCurrentCodeBlock(pcblkResumeTable);

    // but first clear the resume local
    StartHiddenIL();
    GenerateLiteralInt(0);
    GenerateStoreLocal(m_usResumeLocal);

    // get the size of the switch table
    cEntries = m_cadrResumeList.NumberOfEntries();

    // From the line number table, create the TBL_SWITCH info for the resume
    // table.
    //
    ptblswResume = AllocSwitchTable(cEntries);
    ptblswResume->LowVal   = 1;
    ptblswResume->cEntries = cEntries;

    ptblswResume->pcblkFallThrough = m_pcblkResumeTableFallThrough;

    Iter.Reset();
    unsigned long i = 0;

    while (presaddr = Iter.Next())
    {
        ptblswResume->pcodeaddrs[i++] = presaddr->codeaddr;
    }

    // Generate the tableswitch
    EndCodeBuffer(CEE_SWITCH, ptblswResume);

    // The fall through case will branch to On Error Failure which throws an internal error exception.
    // Should never get here at runtime unless something has gone wrong.
    SetAsCurrentCodeBlock(m_pcblkResumeTableFallThrough);
    StartHiddenIL();
    LeaveCodeBuffer(m_pcblkOnErrorFailure, false, false, false);
}

//========================================================================
// Generate all the various handlers needed for On Error
//========================================================================

void CodeGenerator::GenerateOnErrorHandler
(
)
{
    bool fResumePresent = m_ptreeFunc->AsProcedureBlock().fSeenResume;

    VSASSERT(!g_CompilingTheVBRuntime,
             "Do you really want to use On Error in the VB runtime? : #11/12/2003#");

    VSASSERT(m_ptreeFunc->AsProcedureBlock().fSeenOnErr,
             "GenerateOnErrorHandler: no On Error in the method!");

    VSASSERT(m_ptblswOnError,
             "GenerateOnErrorHandler: must have an on error table.");

    VSASSERT(m_usOnErrorLocal != INVALID_LCL_INDEX &&
             m_usResumeLocal != INVALID_LCL_INDEX,
             "GenerateOnErrorHandler: On Error local must be assigned.");

    VSASSERT(m_pcblkOnErrorFilter &&
             m_pcblkOnErrorHandler &&
             m_pcblkOnErrorFailure,
             "GenerateOnErrorHandler:  handler blocks don't exist.");

    CODE_BLOCK *pcblkOnErrorStub = NewCodeBlock();
    SetAsCurrentCodeBlock(pcblkOnErrorStub);

    StartHiddenIL();

    // If we are not keeping track of the current line, then just
    // store a non-zero into m_usResumeLocal so we know we're in a
    // handler. Otherwise, store the current line into the resume local
    // so we know where to resume and to indicate that we're currently
    // in a handler.
    //
    if (m_usCurStmtLocal == INVALID_LCL_INDEX)
    {
        GenerateLiteralInt(-1);
    }
    else
    {
        GenerateLoadLocal(m_usCurStmtLocal, GetSymbolForVtype(t_i4));
    }

    GenerateStoreLocal(m_usResumeLocal);

    // On Error switch table
    CODE_BLOCK *pcblkFallThrough = NewCodeBlock();

    VSASSERT(m_ptblswOnError->pcodeaddrs[0].pcblk == NULL &&
             m_ptblswOnError->pcodeaddrs[1].pcblk == NULL,
             "the first two entries for the On Error switch table should be empty : #11/12/2003#");

    m_ptblswOnError->pcblkFallThrough = pcblkFallThrough;
    m_ptblswOnError->pcodeaddrs[0].pcblk = pcblkFallThrough;
    // Branch to the Resume table if a Resume is present.
    m_ptblswOnError->pcodeaddrs[1].pcblk =
        fResumePresent ? m_pcblkLoadForResumeNext : pcblkFallThrough;

    CODE_BLOCK *pcblkExecuteSwitch = NewCodeBlock();

    if (fResumePresent && m_Project->GenerateDebugCode())
    {
        //Determine if the ResumeIndex is less than or equal to -2:
        CODE_BLOCK *pcblkNonAdjustedSwitch = NewCodeBlock();

        GenerateLoadLocal(m_usOnErrorLocal, GetSymbolForVtype(t_i4));
        GenerateLiteralInt(RESUME_NEXT_BASE);
        EndCodeBuffer(CEE_BGT_S, pcblkNonAdjustedSwitch, t_i4);

        // If so, replace it with RESUME_NEXT and jum to the switch.
        CODE_BLOCK *pcblkAdjustedSwitch = NewCodeBlock();
        SetAsCurrentCodeBlock(pcblkAdjustedSwitch);

        StartHiddenIL();
        GenerateLiteralInt(RESUME_NEXT);
        AddToCurStack(-1); // Load will be consumed later by the switch.
        m_stackTypes.PopOrDequeue();
        EndCodeBuffer(CEE_BR_S, pcblkExecuteSwitch);

        // Otherwise load the local containing active handler and switch on it.
        SetAsCurrentCodeBlock(pcblkNonAdjustedSwitch);
        StartHiddenIL();
    }

    // Load the local containing active handler and switch on it.
    GenerateLoadLocal(m_usOnErrorLocal, GetSymbolForVtype(t_i4));
    AddToCurStack(-1); // Load will be consumed later by the switch.
    m_stackTypes.PopOrDequeue();
    EndCodeBuffer(CEE_NOP);

    // Execute the switch statement.
    SetAsCurrentCodeBlock(pcblkExecuteSwitch);
    StartHiddenIL();
    AddToCurStack(1); // Switch value is already on the stack.
    m_stackTypes.PushOrEnqueue(GetSymbolForVtype(t_i4));
    EndCodeBuffer(CEE_SWITCH, m_ptblswOnError);

    // Something has gone wrong with the On Error mechanism if execution
    // makes it here.  Branch to the Failure block which will throw an internal
    // error exception.
    SetAsCurrentCodeBlock(pcblkFallThrough);
    StartHiddenIL();
    LeaveCodeBuffer(m_pcblkOnErrorFailure, false, false, false);

    //
    // Generate the OnError filter.
    //
    // The On Error filter catches an exception when a handler is active and the method
    // isn't currently in the process of handling an earlier error.  We know the method
    // is handling an earlier error when we have a valid Resume target.
    //
    // The filter expression is the equivalent of:
    //
    //     Catch e When (TypeOf e Is Exception) And (ActiveHandler <> 0) And (ResumeTarget = 0)
    //

    StartHiddenIL();

    SetAsCurrentCodeBlock(m_pcblkOnErrorFilter);
    AddToCurStack(1); // Exception starts on stack
    m_stackTypes.PushOrEnqueue(GetSymbolForFXType(FX::ObjectType));

    // Determine if the exception object is or inherits from System.Exception
    mdTypeRef typref = m_pmdemit->DefineTypeRefBySymbol(GetSymbolForFXType(FX::ExceptionType), NULL);
    EmitOpcode_Tok(CEE_ISINST, typref, GetSymbolForFXType(FX::ExceptionType));
    EmitOpcode(CEE_LDNULL, GetSymbolForFXType(FX::ExceptionType));
    EmitOpcode(CEE_CGT_UN, GetSymbolForVtype(t_bool));

    // Calculate ActiveHandler <> 0
    GenerateLoadLocal(m_usOnErrorLocal, GetSymbolForVtype(t_i4));
    GenerateZeroConst(t_i4);
    EmitOpcode(CEE_CGT_UN, GetSymbolForVtype(t_bool));

    // AND the values together.
    EmitOpcode(CEE_AND, GetSymbolForVtype(t_bool));

    // Calculate ResumeTarget = 0
    GenerateLoadLocal(m_usResumeLocal, GetSymbolForVtype(t_i4));
    GenerateZeroConst(t_i4);
    EmitOpcode(CEE_CEQ, GetSymbolForVtype(t_bool));

    // AND the values together.
    EmitOpcode(CEE_AND, GetSymbolForVtype(t_bool));

    // Filter is finished with 0 or 1 on the stack.
    EmitOpcode(CEE_ENDFILTER, Symbols::GetVoidType());

    //
    // Filter is generated.  Now do the actual handler.
    //

    SetAsCurrentCodeBlock(m_pcblkOnErrorHandler);
    AddToCurStack(1); // Exception starts on stack
    m_stackTypes.PushOrEnqueue(GetSymbolForFXType(FX::ObjectType));

    // Cast the exception to System.Exception from its default type of System.Object
    if (m_Project->GetCompilerHost()->GetFXSymbolProvider()->IsTypeAvailable(FX::ExceptionType))
    {
        GenerateCastClass(m_Project->GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::ExceptionType), NULL);
    }

    if (g_CompilingTheVBRuntime)
    {
        EmitOpcode(CEE_POP, Symbols::GetVoidType());
    }
    else
    {
        // Save the exception into the Err object
        // There is no location information for the On Error statement; only bit flags that tell us On Error was parsed.
        // So I don't have any decent location information to use.  We'll scribble the function signature :-|
        // But only internal people should see this grossness.
        GenerateRTSetErrorMember(&m_ptreeFunc->AsProcedureBlock().SigLocation, Error);
    }

    // Branch to the On Error switch table.
    LeaveCodeBuffer(pcblkOnErrorStub, false, false, false);

    m_tryListPending.Remove(m_ptryOnError);
    m_tryList.InsertLast(m_ptryOnError);

    VSASSERT(m_tryListPending.NumberOfEntries() == 0, "unexpected pending Try");

    SetAsCurrentCodeBlock(m_pcblkOnErrorFailure);
    StartHiddenIL();

    GenerateRTException(E_INTERNALERROR, &m_ptreeFunc->AsProcedureBlock().SigLocation);

    SetAsCurrentCodeBlock(0);
}

//========================================================================
// Generate epilogue for the method
//========================================================================

void CodeGenerator::GenerateEpilogue
(
)
{
    VSASSERT(m_pcblkEndProc && (!WithinFunction() || m_pcblkEndProcLoadLocal),
             "GenerateEpilogue: exit codeblocks not set!");

    // If we are using On Error then we have to generate the handler before we finish the method.
    if (m_ptreeFunc->AsProcedureBlock().fSeenOnErr)
    {
        VSASSERT(m_tryListPending.NumberOfEntries() == 1,
                 "GenerateEpilogue:  should have just On Error Try block at end of method!");

        // Close-up the Try block by branching to the end
        UpdateResumeTable((unsigned short)m_cadrResumeList.NumberOfEntries(), false);
        LeaveCodeBuffer(
            WithinFunction() ? m_pcblkEndProcLoadLocal : m_pcblkEndProc,
            true,
            false,
            false);

        if (m_ptreeFunc->AsProcedureBlock().fSeenResume)
        {
            GenerateResumeTable();
        }

        GenerateOnErrorHandler();
    }
    else
    {
        VSASSERT(m_tryListPending.NumberOfEntries() == 0,
                 "GenerateEpilogue:  still in Try block at end of method!");
    }

    if (WithinFunction())
    {
        // Since we are inside a function, load up the return variable.

        // All return statements will branch here if optimizations are turned off
        // to guarantee that 1) breakpoints set on End Function get hit and 2) the user
        // can do set-next-statement on End Function and a value gets loaded on the stack.

        // With optimizations turned on, this IL will be dead and thus eliminated, provided
        // the user has terminated each code path with a Return statement.

        SetAsCurrentCodeBlock(m_pcblkEndProcLoadLocal);

        StartEpilogueIL();

        if (m_ptreeFunc->AsProcedureBlock().LiftedReturnVarRef)
        {
            // Now we will load the return value from the corresponding closure class field.
            ADDRMODE addrmode;
            GenerateAddr(m_ptreeFunc->AsProcedureBlock().LiftedReturnVarRef, &addrmode);
            GenerateLoad(m_ptreeFunc->AsProcedureBlock().LiftedReturnVarRef, &addrmode);
        }
        else
        {
            GenerateLoadLocal(m_ptreeFunc->AsProcedureBlock().ReturnVariable->GetLocalSlot(),
                              m_ptreeFunc->AsProcedureBlock().ReturnVariable->GetType());
        }

        AddToCurStack(-1);
        m_stackTypes.PopOrDequeue();

        SetAsCurrentCodeBlock(m_pcblkEndProc);
    }
    else
    {
        SetAsCurrentCodeBlock(m_pcblkEndProc);

        StartEpilogueIL();
        // Insert a NOP so we can set a breakpoint on the End Sub.
        // Actually, the routine StartEpilogueIL will have omitted line information in the
        // case that this function was for a single-line sub. The resulting behavior is that
        // single-step debugging won't land on the non-existent "End Sub" of a single-line sub lambda.
        InsertNOP();
    }

    if (m_ptreeFunc->AsProcedureBlock().fSeenOnErr)
    {
        // Upon exiting the method, the Err object is cleared, but only if an error has occured and it
        // has not been "handled" (i.e., a resume, resume next, or On Error Goto -1 has not been encountered).
        // Generate a check of the resume local;  if it's non-zero, then clear the Err object.

        CODE_BLOCK * pcblkDoneWithClear = NewCodeBlock();

        GenerateLoadLocal(m_usResumeLocal, GetSymbolForVtype(t_i4));
        EndCodeBuffer(CEE_BRFALSE_S, pcblkDoneWithClear);

        SetAsCurrentCodeBlock(0);

        GenerateCallToRuntimeHelper(ClearErrorMember, Symbols::GetVoidType(), NULL /* Loc */,  Ignore); // Don't log an error for this one.  We will have logged an error for the actual On Error or Resume

        SetAsCurrentCodeBlock(pcblkDoneWithClear);
    }

    EndCodeBuffer(CEE_RET);


    // now that we're done generating all code, set the open and closing addresses for the Method-wide scope
    if (m_Project->GeneratePDB())
    {
        // Force the Method-level scope to cover the entire codeblock list
        m_MethodScope->OpenAddress.pcblk = m_cblkCodeList.GetFirst();
        m_MethodScope->OpenAddress.uOffset = 0;
        m_MethodScope->CloseAddress.pcblk = m_cblkCodeList.GetLast();
        m_MethodScope->CloseAddress.uOffset = m_MethodScope->CloseAddress.pcblk->usCodeSize+CBytesOpcode(CEE_RET); // Bug 29305 - DevDiv Bugs: Make sure the last instruction is accounted for.
    }
}


void CodeGenerator::FixupScopeAddress
(
CODE_ADDRESS &Address
)
{
    if (Address.pcblk && !Address.pcblk->fLive)
    {
        Address.uOffset = 0;
        while (Address.pcblk != NULL && !Address.pcblk->fLive)
        {
            Address.pcblk = Address.pcblk->Next();
        }
    }
}

void CodeGenerator::AssignScopeOffsets
(
BlockScope * CurrentScope,
BlockScope * ParentScope
)
{
    if (CurrentScope->ChildScopeList.NumberOfEntries() > 0)
    {
        CSingleListIter<BlockScope> IterScopes(&CurrentScope->ChildScopeList);
        BlockScope * ChildScope;

        while (ChildScope = IterScopes.Next())
        {
            AssignScopeOffsets(ChildScope, CurrentScope);
        }
    }

    FixupScopeAddress(CurrentScope->OpenAddress);
    FixupScopeAddress(CurrentScope->CloseAddress);

    if (CurrentScope->OpenAddress.pcblk == NULL ||
        CurrentScope->CloseAddress.pcblk == NULL ||
        (CurrentScope->OpenAddress.pcblk->ulBlockOffset == CurrentScope->CloseAddress.pcblk->ulBlockOffset &&
         CurrentScope->OpenAddress.uOffset == CurrentScope->CloseAddress.uOffset))
    {
        // This scope is dead.
        if (CurrentScope != m_MethodScope)
        {
            ParentScope->ChildScopeList.Remove(CurrentScope);
        }
    }
    else
    {
        CurrentScope->OpenOffset = CodeAddress(&CurrentScope->OpenAddress);
        CurrentScope->CloseOffset = CodeAddress(&CurrentScope->CloseAddress);
    }
}

/*========================================================================

Public entrypoints

========================================================================*/


//========================================================================
//
//     Public entry called from Compiled. This function is responsible for
//     generating all the byte codes for the given function and
//     related information such as the line number table and the
//     exception table.
//
//     If no OnError present, we do not generate any exception handlers for
//     the code block. Exceptions will unwind to the IDE who will then map it.
//
//     If there is OnError present, we need to create a try/catch block for the
//     code body only (no initialization) with the OnError handlers.
//     This is to keep the verifier happy. Since OnError and resume tables will
//     be jumping back into code, the verifier will complain of locals not being
//     initialized because it can't verify that all code paths will initialize
//     the locals. We will not trap the errors thrown during function
//     initialization. Function initialization is like a function's prolog
//     and VBA's semantics is that errors in the function's prolog are not
//     trapped in the function.
//
//       locals initialization
//
//       try
//       {
//         code body
//
//  lbl_Resume:
//         load curStatement
//         increment curStatement by 1
//         tableswitch(curStatement):
//                1 : lbl_line1
//               ...
//
//  lbl_OnError:
//         store exception object
//         load curHandler
//         tableswitch(curHandler):
//               -1 : lbl_Resume
//                0 : lbl_OnErrDefault
//                1 : lbl_1
//                ...
//
//       }
//       catch (Throwable)
//       {
//           goto lbl_OnError
//       }
//  lbl_OnErrDefault:
//
//         load exception object
//         call runtime to map exception
//
//
//
//========================================================================

void CodeGenerator::GenerateMethod
(
ILTree::ILNode *ptreeFunc
)
{
    VSASSERT(ptreeFunc->bilop == SB_PROC, "GenerateMethod: expected Proc node");
    VSASSERT(!ptreeFunc->AsProcedureBlock().pproc->IsBad(), "GenerateMethod: bad proc in codegen!  how can this be?");
    VSASSERT(ptreeFunc->AsProcedureBlock().pproc->PProc()->GetBindingSpace() != BINDSPACE_IgnoreSymbol, "Why generate a deleted method?");

    //
    // First, initialize everything we need for code generation. No
    // actual code will be generated yet.
    //

    // Initalize the code generator
    InitializeGenerator(ptreeFunc);

#if DEBUG
    DumpBeginMethod();
#endif

    // Allocate the line table and initialize debug info
    InitializeDebugInformation();

    // Initialize local and parameter slots & local variable signature
    InitializeMethodSlots();

    // Do processing required for On Error
    InitializeOnError();

    //
    // Done with initialization. Now start generating the actual
    // code
    //

    // Generate the byte codes
    GenerateBlockStatementList(ptreeFunc->AsProcedureBlock());

    // Generate Epilog
    GenerateEpilogue();

    // Create the locals signature, but only if the method contains locals.
    //
    // Note that this should be delayed till the method IL is generated so
    // that any new temporaries required during code gen can be created.
    //
    if (m_MethodSlots->LocalVariableList()->NumberOfEntries() > 0)
    {
        CreateLocalsSignature();
    }

    // All code generation for this method is complete by this point

#if DEBUG
    DumpEndMethod();
#endif

}

//========================================================================
// Returns the resulting size of the IL, headers and SEH information
//========================================================================

unsigned CodeGenerator::GetImageSize()
{
    unsigned long    cTotalBytes;
    unsigned long    cbImageSize;

    VSASSERT(m_usCurStackLvl == 0,
              "EmitByteCodes: stack level must be 0 at end");
    VSASSERT(m_stackTypes.Count() == 0,
              "There should be nothing left on m_stackTypes!");
    VSASSERT(m_tryListPending.NumberOfEntries() == 0,
              "EmitByteCodes: no open try blocks at end");

    // Remove any dead code blocks (i.e. unreachable) from the code block
    // list. Then lay out the blocks.
    //
    if (m_Project->GenerateOptimalIL())
    {
        OptimizeGeneratedIL();
    }
    else
    {
        // mark every block as live because the scope alogrithm relies on this info.
        CSingleListIter<CODE_BLOCK> Iter(&m_cblkCodeList);
        CODE_BLOCK * pcblk;

        while (pcblk = Iter.Next())
        {
            pcblk->fLive = true;
        }
    }

    cTotalBytes = CalculateBlockAddresses();

    if (m_Project->GeneratePDB())
    {
        FixupScopeAddress(m_MethodScope->OpenAddress);
        FixupScopeAddress(m_MethodScope->CloseAddress);

        // Fix-up the top-level scope if the end of the method was optimized away
        if (m_MethodScope->CloseAddress.pcblk == NULL)
        {
            CSingleListIter<CODE_BLOCK> Iter(&m_cblkCodeList);
            CODE_BLOCK * pcblk;
            CODE_BLOCK * LastLive = NULL;

            while (pcblk = Iter.Next())
            {
                if (pcblk->fLive)
                {
                    LastLive = pcblk;
                }
            }

            VSASSERT(LastLive, "GetImageSize: there are no live code blocks?!");

            m_MethodScope->CloseAddress.pcblk = LastLive;
            m_MethodScope->CloseAddress.uOffset = LastLive->usCodeSize;
            m_MethodScope->CloseOffset = CodeAddress(&m_MethodScope->CloseAddress);
        }

        AssignScopeOffsets(m_MethodScope, m_MethodScope);
    }

    // Push the exception handling stuff into MetaEmit.
    EmitEHClauses();

    // Calculate the size.
    cbImageSize = m_pmdemit->ImageSize(m_usMaxStackLvl,
                                       m_MethodSlots->LocalVariableList()->NumberOfEntries() != 0,
                                       cTotalBytes);

    return cbImageSize;
}

//========================================================================
// Emits the IL into a block of memory.  The memory is guarenteed
// to be large enough.
//========================================================================

void CodeGenerator::EmitImage
(
    BYTE *pbImage
)
{
    // write out the byte codes
    EmitByteCodes(pbImage);

    // write out debug info
    EmitDebugInformation();

}
