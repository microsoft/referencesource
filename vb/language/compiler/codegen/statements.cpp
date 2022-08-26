//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Generate COM+ byte codes for statements.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

//-------------------------------------------------------------------------------------------------
//
// Generate a statement list
//
void CodeGenerator::GenerateBlockStatementList(ILTree::ExecutableBlock &exblock)
{
    BlockScope *CurrentScope = exblock.LocalScope;
    if (m_IsGeneratingAwait)
    {
        CurrentScope = m_MethodScope;
    }
    else
    {
        CurrentScope = exblock.LocalScope;
    }
    

    VSASSERT(exblock.IsExecutableBlockNode(), "how can this be?  it's not an executable block?");

    // A non-null scope implies that we are generating PDB information.
    // Therefore, we need to keep track of block scoping information.
    // If so, record the open and close address of each scope.
    //
    VSASSERT(!(!!CurrentScope ^ m_Project->GeneratePDB()),
              "AddVariableToMethod: The validity of CurrentScope must follow decision to generate pdb");

    if (CurrentScope)
    {
        // store away the Opening code address
        CurrentScope->OpenAddress.pcblk = m_pcblkCurrent;
        CurrentScope->OpenAddress.uOffset = CodeBuf_GetSize();
    }


    GenerateStatementList(exblock.ptreeChild);


    if (CurrentScope)
    {
        // store away the Closing code address
        CurrentScope->CloseAddress.pcblk = m_pcblkCurrent;
        CurrentScope->CloseAddress.uOffset = CodeBuf_GetSize();
    }
}

void CodeGenerator::GenerateStatementList
(
ILTree::PILNode ptreeStmt
)
{
    ILTree::PILNode ptreeStmtCur;


    for ( ptreeStmtCur = ptreeStmt;
          ptreeStmtCur != NULL;
          ptreeStmtCur = ptreeStmtCur->AsStatement().Next, m_pCodeGenTemporaryManager->FreeShortLivedTemporaries() )
    {
        VSASSERT(!(ptreeStmtCur->uFlags & SF_ERROR), "GenerateStatementList: didn't expect bad flag on bound tree");

        VSASSERT(m_usCurStackLvl == m_stackTypes.Count(), "GenerateRvalue: m_usCurStackLvl != m_stackTypes.Count()!");

        switch (ptreeStmtCur->bilop)
        {
        case SL_STMT:
            {
                VSASSERT(ptreeStmtCur->AsStatementWithExpression().ptreeOp1, "empty statement unexpected. : #11/12/2003#");
                UpdateLineTable(ptreeStmtCur);
                UpdateResumeTable(ptreeStmtCur->AsStatement().ResumeIndex);

                unsigned short usStackLevelBefore = m_usCurStackLvl;
                GenerateRvalue(ptreeStmtCur->AsStatementWithExpression().ptreeOp1);
                if (HasFlag32(ptreeStmtCur, SXF_STMT_ENSURE_NO_ITEMS_ADDED_TO_STACK_BY_POPPING))
                {
                    int stackDifference = m_usCurStackLvl - usStackLevelBefore;
                    if (stackDifference < 0)
                    {
                        VSASSERT(false, "HOW can an RValue interpretation shrink the STACK? Handle this case gracefully.");
                        stackDifference = 0;
                    }
                    // For each item added to the stack. if SXF_STMT_ENSURE_NO_ITEMS_ADDED_TO_STACK_BY_POPPING flag
                    // is set, pop all the values left behind.
                    for (int i = 0; i < stackDifference; i++)
                    {
                        EmitOpcode(CEE_POP, Symbols::GetVoidType());
                    }
                }
            }
            break;

            /*
             * If/Then/Else statements
             */
        case SB_IF_BLOCK:

            UpdateLineTable(ptreeStmtCur);
            UpdateResumeTable(ptreeStmtCur->AsIfGroup().ResumeIndex);

            // Child stmt list is the If-ElseIf-Else chain, no need to add
            // any resume behavior here
            GenerateBlockStatementList(ptreeStmtCur->AsExecutableBlock());

            // Done with block
            // The branch out code block becomes the current
            VSASSERT(ptreeStmtCur->AsIfGroup().EndBlock != NULL,
                       "GenerateBlkEnd: SB_IF should have set block end code block.");

            // If this is the last single line IF, hide the jump instruction.
            if (ptreeStmtCur->AsStatement().Next == NULL && ptreeStmtCur->uFlags & SBF_LINE_IF)
            {
                StartHiddenIL();
            }

            SetAsCurrentCodeBlock(ptreeStmtCur->AsIfGroup().EndBlock);

            // Add a NOP for the End If, if there is an End If
            if (!(ptreeStmtCur->uFlags & SBF_LINE_IF))
            {
                UpdateLineTable(ptreeStmtCur->AsExecutableBlock().EndConstructLocation);
                UpdateResumeTable(ptreeStmtCur->AsIfGroup().TerminatingResumeIndex, false);  // so we can "resume next" onto the End If
                InsertNOP();
            }

            break;

        case SB_IF:
            {
                CODE_BLOCK * pcblkFalse;
                CODE_BLOCK * pcblkTrue;
                ILTree::PILNode     ptreeParent;

                pcblkFalse = NewCodeBlock();
                pcblkTrue = NewCodeBlock();

                ptreeParent = ptreeStmtCur->AsExecutableBlock().Parent;
                VSASSERT(ptreeParent && ptreeParent->bilop == SB_IF_BLOCK, "parent is not an If context block.  how can this be?");

                // Updating the line table and resume table for SB_IF is handled by the SB_IF_BLOCK node.

                GenerateConditionWithENCRemapPoint(ptreeStmtCur->AsIfBlock().Conditional, true, &pcblkTrue, &pcblkFalse);

                // The pvOut stores the code block for the next else/elseif.
                // The pvBlk stores the code block for the block end.
                // If there are no else/elseif's, the branch out for the if
                // is also the block end.
                //
                ptreeStmtCur->AsIfBlock().FalseBlock = pcblkFalse;

                // if there's an Else of ElseIf
                if (ptreeStmtCur->AsIfBlock().Next != NULL)
                {
                    ptreeParent->AsIfGroup().EndBlock = NewCodeBlock();
                }
                else
                {
                    ptreeParent->AsIfGroup().EndBlock = ptreeStmtCur->AsIfBlock().FalseBlock;
                }

                // Generate statements in block
                GenerateBlockStatementList(ptreeStmtCur->AsExecutableBlock());
            }
            break;

        case SB_ELSE:
        case SB_ELSE_IF:
            GenerateElse(ptreeStmtCur);

            // Generate statements in block
            GenerateBlockStatementList(ptreeStmtCur->AsExecutableBlock());
            break;

        case SB_HIDDEN_CODE_BLOCK:
        case SB_STATEMENT_LAMBDA:
            // Generate the statements in block
            GenerateBlockStatementList(ptreeStmtCur->AsExecutableBlock());
            break;

        case SB_SELECT:
            GenerateSelect(ptreeStmtCur);

            // Generate statements in block
            GenerateBlockStatementList(ptreeStmtCur->AsExecutableBlock());

            GenerateEndSelect(ptreeStmtCur);
            break;

        case SB_CASE:
            GenerateCase(ptreeStmtCur);

            // Generate statements in block
            GenerateBlockStatementList(ptreeStmtCur->AsExecutableBlock());
            break;

            /*
             * Loop statements
             */
        case SB_LOOP:
            GenerateLoop(&ptreeStmtCur->AsLoopBlock());
            break;

        case SB_FOR:
            GenerateForLoop(ptreeStmtCur);
            break;

        case SB_FOR_EACH:
            GenerateForEachLoop(ptreeStmtCur);
            break;

        case SL_CONTINUE:
            UpdateLineTable(ptreeStmtCur);
            UpdateResumeTable(ptreeStmtCur->AsStatement().ResumeIndex, false);
            GenerateContinue(ptreeStmtCur);
            break;

        case SL_RETURN:
            GenerateReturn(ptreeStmtCur);
            break;

        case SL_EXIT:
            UpdateLineTable(ptreeStmtCur);
            // Generate a NOP so that we can set a breakpoint on this line
            InsertNOP();

            switch (ptreeStmtCur->AsExitStatement().exitkind)
            {
                case EXIT_DO:
                case EXIT_FOR:
                {
                    GenerateExitLoop(ptreeStmtCur);
                    break;
                }

                case EXIT_TRY:
                {
                    VSASSERT(m_tryListPending.NumberOfEntries(), "have to be in try!");

                    // Skip the synthesized Trys (used for For Each and SyncLock). They are not valid Exit Try targets.
                    //
                    // 
                    TRY_BLOCK *Target = m_tryListPending.GetFirst();
                    while (Target->IsSynthetic)
                    {
                        Target = Target->Next();
                        VSASSERT(Target, "Synthetic Try cannot be last in the list;  Exit Try usage should have caused an error.");
                    }

                    // To branch from a Try, we need to "Leave" it.
                    LeaveCodeBuffer(
                        Target->pcblkFallThrough,
                        ptreeStmtCur->uFlags & SLF_EXITS_TRY,
                        ptreeStmtCur->uFlags & SLF_EXITS_CATCH,
                        ptreeStmtCur->uFlags & SLF_EXITS_CATCH_TO_CORRESPONDING_TRY);
                    SetAsCurrentCodeBlock(0);
                    break;
                }

                case EXIT_SELECT:
                {
                    GenerateExitSelect(ptreeStmtCur);
                    break;
                }

                default:
                {
                    VSFAIL("SL_EXIT: Unexpected exitkind.");
                    VbThrow(HrMake(ERRID_CodegenError));
                }
            }
            break;

            /*
             * Misc statements
             */
        case SB_WITH:
            UpdateLineTable(ptreeStmtCur);
            UpdateResumeTable(ptreeStmtCur->AsWithBlock().ResumeIndex);

            // reference types will assign to temporary on entry of with block
            if (ptreeStmtCur->AsWithBlock().ptreeWith)
            {
                GenerateRvalue(ptreeStmtCur->AsWithBlock().ptreeWith);
            }
            else
            {
                InsertNOP();
            }

            GenerateBlockStatementList(ptreeStmtCur->AsWithBlock());

            UpdateLineTable(ptreeStmtCur->AsExecutableBlock().EndConstructLocation);
            UpdateResumeTable(ptreeStmtCur->AsWithBlock().TerminatingResumeIndex, false);

            // reference types will assign from temporary on exit of with block
            if (ptreeStmtCur->AsWithBlock().TemporaryClearAssignment)
            {
                GenerateRvalue(ptreeStmtCur->AsWithBlock().TemporaryClearAssignment);
            }
            else
            {
                InsertNOP();
            }
            break;

            /*
             * Array statements
             */
        case SL_REDIM:
            UpdateLineTable(ptreeStmtCur);
            UpdateResumeTable(ptreeStmtCur->AsStatement().ResumeIndex);

            GenerateRedim(ptreeStmtCur);
            break;

            /*
             * Error/debug statements
             */
        case SL_ERROR:
            UpdateLineTable(ptreeStmtCur);
            UpdateResumeTable(ptreeStmtCur->AsStatement().ResumeIndex);

            if (ptreeStmtCur->AsStatementWithExpression().ptreeOp1)
            {
                GenerateRvalue(ptreeStmtCur->AsStatementWithExpression().ptreeOp1);

                if (ptreeStmtCur->AsStatementWithExpression().ptreeOp1->AsExpression().vtype != t_ref &&
                    ptreeStmtCur->AsStatementWithExpression().ptreeOp1->AsExpression().vtype != t_generic)
                {
                    GenerateCallToRuntimeHelper(CreateErrorMember, GetSymbolForFXType(FX::ExceptionType), &ptreeStmtCur->Loc); // Called for standalone Error n, statement
                }
                EndCodeBuffer(CEE_THROW);
            }
            else
            {
                EndCodeBuffer(CEE_RETHROW);
            }
            SetAsCurrentCodeBlock(0);
            break;

        case SL_ON_ERR:
            UpdateLineTable(ptreeStmtCur);
            UpdateResumeTable(ptreeStmtCur->AsStatement().ResumeIndex, false);
            GenerateOnError(ptreeStmtCur);
            break;

        case SL_RESUME:
            UpdateLineTable(ptreeStmtCur);
            UpdateResumeTable(ptreeStmtCur->AsStatement().ResumeIndex);
            GenerateResume(ptreeStmtCur);
            break;

        case SL_END:
            UpdateLineTable(ptreeStmtCur);
            GenerateCallToRuntimeHelper(EndStatementMember, Symbols::GetVoidType(), &ptreeStmtCur->Loc);
            break;

        case SL_STOP:
            UpdateLineTable(ptreeStmtCur);
            UpdateResumeTable(ptreeStmtCur->AsStatement().ResumeIndex);
            GenerateCallToRuntimeHelper(SystemDebugBreakMember, Symbols::GetVoidType(), &ptreeStmtCur->Loc);
            break;

        case SL_DEBUGLOCNOP:
            UpdateLineTable(ptreeStmtCur);
            InsertNOP();
            break;

        case SB_TRY:
            GenerateTry(ptreeStmtCur);
            break;

        case SB_CATCH:
            GenerateCatch(&ptreeStmtCur->AsCatchBlock());
            break;

        case SB_FINALLY:
            GenerateFinally(ptreeStmtCur);
            break;

        case SB_SYNCLOCK:
        case SB_USING:
            UpdateLineTable(ptreeStmtCur);
            UpdateResumeTable(ptreeStmtCur->AsExecutableBlock().ResumeIndex);
            InsertNOP();  // Insert a NOP so we can break on the entire Using or SyncLock line.

            GenerateBlockStatementList(ptreeStmtCur->AsExecutableBlock());

            InsertNOP();  // Insert a NOP so that Using block variable does not have its scope mapped to the next statement

            // No Resume on the End Using/End Synclock because that corresponds to a Finally block.
            break;

            /*
             * Branch statements
             */
        case SL_GOTO:
            UpdateLineTable(ptreeStmtCur);
            UpdateResumeTable(ptreeStmtCur->AsStatement().ResumeIndex, false);
            GenerateGotoLabel(
                ptreeStmtCur->AsGotoStatement().Label,
                ptreeStmtCur->uFlags & SLF_EXITS_TRY,
                ptreeStmtCur->uFlags & SLF_EXITS_CATCH,
                ptreeStmtCur->uFlags & SLF_EXITS_CATCH_TO_CORRESPONDING_TRY);
            break;

        case SL_ASYNCSWITCH:
            GenerateAsyncSwitch(ptreeStmtCur);
            break;

        case SL_LABEL:
            GenerateLabel(ptreeStmtCur);
            break;

        default:
            VSFAIL("GenerateStatementList: Unexpected stmtkind.");
            VbThrow(HrMake(ERRID_CodegenError));

        } // switch

        // Setup the on error try block if required for the instance constructor. By default setting
        // the try block is delayed for the instance constructors because the MyBase.New, MyClass.New
        // and initobj statements should be emitted outside the try block. MyBase.New or MyClass.New
        // if emitted inside the try block will result in unverifiable code. Bug VSWhidbey 204996
        //
        if (HasFlag32(ptreeStmtCur->AsStatement(), SLF_INIT_ME))
        {
            VSASSERT(m_pproc->IsInstanceConstructor(), "SLF_INIT_ME unexpected outside of an instance constructor!!!");
            VSASSERT(m_ptryOnError == NULL, "Multiple SLF_INIT_ME statements ?");
            StartOnErrorTryBlock();
        }

    } // while
}

/*========================================================================

If/then/else statements

========================================================================*/

//========================================================================
// Generate ELSE statement
//========================================================================

void CodeGenerator::GenerateElse
(
ILTree::PILNode ptreeStmtCur      // SB_ELSE or SB_ELSE_IF
)
{
    ILTree::ILNode    *  ptreePrev;

    VSASSERT(ptreeStmtCur &&
               (ptreeStmtCur->bilop == SB_ELSE  || ptreeStmtCur->bilop == SB_ELSE_IF) &&
               ptreeStmtCur->AsIfBlock().Prev != NULL &&
               ptreeStmtCur->AsIfBlock().Parent->bilop == SB_IF_BLOCK &&
               ptreeStmtCur->AsIfBlock().Parent->AsIfGroup().EndBlock != NULL,
               "GenerateElse: ptreeStmtCur not set correcly or must have previous if/else");


    // Get the branch out code block from the previous if/else if
    ptreePrev = ptreeStmtCur->AsIfBlock().Prev;
    VSASSERT(ptreePrev->AsIfBlock().FalseBlock != NULL,
               "GenerateElse: there must be branch out code block.");

    // Update the resume table so exceptions that occur in the previous block don't fall through to this
    // next block upon Resume Next
    //
    UpdateResumeTable(ptreeStmtCur->AsIfBlock().ResumeIndex, false);

    // End the current code block with a goto to the forward code block
    // in the If stmt.
    //
    EndCodeBuffer(CEE_BR_S, ptreeStmtCur->AsIfBlock().Parent->AsIfGroup().EndBlock);

    SetAsCurrentCodeBlock(ptreePrev->AsIfBlock().FalseBlock);

    UpdateLineTable(ptreeStmtCur);
    // Each Else gets two resume indices, so do +1 to get the second one.
    UpdateResumeTable(ptreeStmtCur->AsIfBlock().ResumeIndex + 1);

    if (ptreeStmtCur->AsIfBlock().Conditional)
    {
        CODE_BLOCK * pcblkFalse;
        CODE_BLOCK * pcblkTrue;

        pcblkTrue = NewCodeBlock();

        // If this is the last elseif, we want the False block (the jump out)
        // to be the blkend code block.
        //
        if (ptreeStmtCur->AsIfBlock().Next == NULL)
        {
            pcblkFalse = ptreeStmtCur->AsIfBlock().Parent->AsIfGroup().EndBlock;
        }
        else
        {
            pcblkFalse = NewCodeBlock();
        }

        GenerateConditionWithENCRemapPoint(ptreeStmtCur->AsIfBlock().Conditional, true, &pcblkTrue, &pcblkFalse);

        ptreeStmtCur->AsIfBlock().FalseBlock = pcblkFalse;
    }
    else
    {
        // Generate an opcode for the else block so that we can step onto the line containing "else"
        InsertNOP();
    }
}

void
RecordSwitchTableEntry
(
    SWITCH_TABLE *Table,
    unsigned __int64 Index,
    CODE_BLOCK *NewEntry
)
{
    VSASSERT(Index >= 0 && Index < Table->cEntries, "RecordSwitchTableEntry: invalid index into SWITCH table");

    if (Table->pcodeaddrs[Index].pcblk == NULL)
    {
        Table->pcodeaddrs[Index].pcblk = NewEntry;
    }
}

//========================================================================
// Generate CASE statement - This is highly dependent
// on work done earlier in GenerateSelect() and PreprocessSelect().
// Each Case statment ends the code block of the previous Case.  Thus,
// the first Case statement is a special case and a check is made.
// If generating an IF list, the caselist is turned into a cond tree
// using BuildCondFromCaseList() and GenerateCondition() is called on this tree.
// If generating a SWITCH table, GenerateCase populates the entries in
// the table that correspond to the current Case stmt.
//========================================================================

void CodeGenerator::GenerateCase
(
ILTree::PILNode ptreeStmtCur    // SB_CASE
)
{
    ILTree::PILNode ptreeSelect;

    VSASSERT(ptreeStmtCur->bilop == SB_CASE,
             "GenerateCase: current stmt must be SB_CASE.");

    ptreeSelect = ptreeStmtCur->AsCaseBlock().Parent;

    VSASSERT(ptreeSelect->bilop == SB_SELECT,
             "GenerateCase: owner must be a SB_SELECT.");

    if (ptreeSelect->uFlags & SBF_SELECT_TABLE)
    {
        VSASSERT(ptreeSelect->AsSelectBlock().SwitchTable, "GenerateCase: must have switch table.");

        // If not first Case, end the previous code block (which contains
        // the code body for previous Case) with a branch out to end of
        // Select.
        //
        if (ptreeSelect->AsSelectBlock().ptreeChild != ptreeStmtCur)
        {
            VSASSERT(ptreeSelect->AsSelectBlock().EndBlock,
                       "GenerateCase: SELECT must have a block end code block.");

            // Add a Resume entry to protect against fall-through from the previous Case block.
            UpdateResumeTable(ptreeStmtCur->AsStatement().ResumeIndex, false);
            EndCodeBuffer(CEE_BR_S, ptreeSelect->AsSelectBlock().EndBlock);
            SetAsCurrentCodeBlock(0);
        }

        if (ptreeStmtCur->uFlags & SBF_CASE_ISELSE)
        {
            VSASSERT(ptreeSelect->AsSelectBlock().SwitchTable->pcblkCaseElse,
                       "GenerateCase: code block for CASE ELSE doesn't exist");

            SetAsCurrentCodeBlock( ptreeSelect->AsSelectBlock().SwitchTable->pcblkCaseElse );
            UpdateLineTable(ptreeStmtCur);
            InsertNOP();
        }
        else
        {
            SWITCH_TABLE * pswTable = ptreeSelect->AsSelectBlock().SwitchTable;
            ILTree::CASELIST     * pcaselist = ptreeStmtCur->AsCaseBlock().BoundCaseList;

            VSASSERT(CodeBuf_GetSize() == 0, "GenerateCase:  CASE condition needs fresh code block");
            VSASSERT(pcaselist, "GenerateCase: CASE doesn't have caselist");

            bool IsUnsigned = IsUnsignedType(ptreeSelect->AsSelectBlock().SelectorTemporary->GetType()->GetVtype());

            UpdateLineTable(ptreeStmtCur);
            InsertNOP();

            while (pcaselist)
            {
                VSASSERT(pcaselist->LowBound->bilop == SX_CNS_INT ||
                         (pcaselist->IsRange && pcaselist->HighBound->bilop == SX_CNS_INT) ||
                         (!pcaselist->IsRange && pcaselist->RelationalOpcode == SX_EQ),
                         "GenerateCase: unexpected conditions for use of SWITCH table");

                __int64 LowIndex = pswTable->LowVal;
                __int64 Lower = pcaselist->LowBound->AsIntegralConstantExpression().Value;

                VSASSERT((IsUnsigned && (unsigned __int64)Lower >= (unsigned __int64)LowIndex) ||
                         (!IsUnsigned && Lower >= LowIndex),
                         "GenerateCase: invalid LowIndex.  Value is smaller.");

                if (pcaselist->IsRange)
                {
                    __int64 Higher = pcaselist->HighBound->AsIntegralConstantExpression().Value;

                    VSASSERT((IsUnsigned && (unsigned __int64)Lower <= (unsigned __int64)Higher) ||
                             (!IsUnsigned && Lower <= Higher),
                             "GenerateCase: invalid range values - lower value must come first");

                    for (__int64 i = Higher;
                         (IsUnsigned && (unsigned __int64)i > (unsigned __int64)Lower) || (!IsUnsigned && i > Lower);
                         i--)
                    {
                        RecordSwitchTableEntry(pswTable, i - LowIndex, m_pcblkCurrent);
                    }
                }

                RecordSwitchTableEntry(pswTable, Lower - LowIndex, m_pcblkCurrent);

                pcaselist = pcaselist->Next;
            }
        }
    }
    else
    {
        ILTree::PILNode ptreeChild;

        // If not first Case, end the previous code block (which contains
        // the code body for previous Case) with a branch out to end of
        // Select and set the current code block to the previous Case's
        // branch out
        //
        ptreeChild = ptreeSelect->AsSelectBlock().ptreeChild;

        if (ptreeChild != ptreeStmtCur)
        {
            VSASSERT(ptreeSelect->AsSelectBlock().EndBlock,
                       "GenerateCase: SELECT must have a block end code block.");

            VSASSERT(ptreeStmtCur->AsCaseBlock().Prev->bilop == SB_CASE &&
                       ptreeStmtCur->AsCaseBlock().Prev->AsCaseBlock().FalseBlock,
                       "GenerateCase: previous SB_CASE inconsistent.");

            // Add a Resume entry to protect against fall-through from the previous Case block.
            UpdateResumeTable(ptreeStmtCur->AsStatement().ResumeIndex, false);
            EndCodeBuffer(CEE_BR_S, ptreeSelect->AsSelectBlock().EndBlock);
            SetAsCurrentCodeBlock(ptreeStmtCur->AsCaseBlock().Prev->AsCaseBlock().FalseBlock);
        }

        if (!(ptreeStmtCur->uFlags & SBF_CASE_ISELSE))
        {
            CODE_BLOCK * pcblkTrue;
            CODE_BLOCK * pcblkFalse;

            pcblkTrue = NewCodeBlock();

            // If this is the last case, then the branch out for the
            // condition is the EndBlock code block.
            //
            if (ptreeStmtCur->AsCaseBlock().Next == NULL)
            {
                pcblkFalse = ptreeSelect->AsSelectBlock().EndBlock;
            }
            else
            {
                pcblkFalse = NewCodeBlock();
            }

            bool fHideCase = ptreeStmtCur->AsCaseBlock().Conditional &&
                             ptreeStmtCur->AsCaseBlock().Conditional->bilop == SX_EQ &&
                             ptreeStmtCur->AsCaseBlock().Conditional->AsBinaryExpression().Right->bilop == SX_CNS_STR;

            if (fHideCase)
            {
                StartHiddenIL();
            }
            else
            {
                UpdateLineTable(ptreeStmtCur);
            }

            // Each Case gets two resume indices, so do +1 to get the second one.
            UpdateResumeTable(ptreeStmtCur->AsStatement().ResumeIndex + 1);
            InsertNOP();

            VSASSERT(ptreeStmtCur->uFlags & SBF_CASE_CONDITION, "expected condition");

            // The condition tree has already been built by Semantics.
            VSASSERT(TypeHelpers::IsBooleanType(ptreeStmtCur->AsCaseBlock().Conditional->ResultType),
                     "expected boolean type for Case condition");

            GenerateConditionWithENCRemapPoint(ptreeStmtCur->AsCaseBlock().Conditional, true, &pcblkTrue, &pcblkFalse);

            ptreeStmtCur->AsCaseBlock().FalseBlock = pcblkFalse;

            if (fHideCase)
            {
                // End the hidden IL block
                UpdateLineTable(ptreeStmtCur);
                InsertNOP();
            }
        }
        else
        {
            UpdateLineTable(ptreeStmtCur);
            InsertNOP();
        }
    }
}

//========================================================================
// Generate a Select statement.
//
// First, determine what method to use:  SWITCH table or IF list.
// Next, generate code to initialize the Select temp variable
// If a SWITCH table is to be used, then generate the code needed
// before the switch happens (load the temp var onto the stack
// and normalize it).  Then handle the default fall through.
//
// The following is an example of a SWITCH table as used for Select
//
//   VB Code                                   COM+ pseudocode
//   ---------------------------------         ----------------------------------
//   1     Dim i As Integer
//   2
//   3     Select Case i                       temp = i
//   4                                         load temp onto stack
//   5                                         subtract 1         (normalization)
//   6                                         switch (2 cases)
//   7                                         0:  branch 12
//   8                                         1:  branch 16
//   9
//   10                                        branch 20          (FallThrough cblk)
//   11
//   12      Case 1                                               (Case 1 cblk)
//   13        <do something>                  ...
//   14                                        branch 23
//   15
//   16      Case 2                                               (Case 2 cblk)
//   17        <do something>                  ...
//   18                                        branch 23
//   19
//   20      Case Else                                            (CaseElse cblk)
//   21        <do something>                  ...
//   22
//   23    End Select                                             (BlockEnd cblk)
//   24
//   25    <do something>                      ...
//   26                                        ...
//========================================================================

void CodeGenerator::GenerateSelect
(
ILTree::PILNode ptreeStmtCur    // SB_SELECT
)
{
    ILTree::SelectBlock *Select = &ptreeStmtCur->AsSelectBlock();

    // Create a foward label for the block end
    Select->EndBlock = NewCodeBlock();

    UpdateLineTable(Select);
    UpdateResumeTable(Select->ResumeIndex);
    InsertNOP();

    // If the Select throws, a Resume Next should branch to the End Select.
    // Each Select gets two resume indices for the Select line, so do +1 to get the second one.
    UpdateResumeTable(Select->ResumeIndex + 1, false, Select->EndBlock);

    if (Select->uFlags & SBF_SELECT_TABLE)
    {
        // create the switch table
        if (Select->Maximum < Select->Minimum || (Select->Maximum - Select->Minimum) + 1 < 1 )
        {
            return; // Overflow
        }
        unsigned __int64 CountOfEntries = Select->Maximum - Select->Minimum + 1;
        if ((unsigned long)CountOfEntries != CountOfEntries)
        {
            return; // Overflow
        }
#pragma warning(disable:22005) // I round-trip the casting above
        SWITCH_TABLE *SwitchTable = AllocSwitchTable((unsigned long)CountOfEntries);

        SwitchTable->cEntries = (unsigned long)CountOfEntries;
#pragma warning(default:22005) // I round-trip the casting above
        SwitchTable->LowVal = Select->Minimum;
        SwitchTable->HiVal = Select->Maximum;

        // the SWITCH table should know the next code block because branch values are offsets
        // from the instruction AFTER the switch instruction
        SwitchTable->pcblkFallThrough = NewCodeBlock();

        // if the Select has a Case Else statement in it, alloc a code block for it
        if (Select->uFlags & SBF_SELECT_HAS_CASE_ELSE)
        {
            SwitchTable->pcblkCaseElse = NewCodeBlock();
        }

        Select->SwitchTable = SwitchTable;

        // If the temp is typed I8 or UI8, we need to guard against truncation error:
        // e.g.:  0x0300000002 - 0x0000000001 = 0x0300000001 truncated to I4 --> 0x00000001 *error*
        // We do this by branching to the case else if value > HiVal or value < LowVal
        //
        Vtypes vtype = Select->SelectorCapture->AsExpressionWithChildren().Right->vtype;

        if (vtype == t_i8 || vtype == t_ui8)
        {
            // Generate the select assignment statement
            GenerateRvalue(Select->SelectorCapture);

            GenerateLoadOrElseLoadLocal(Select->LiftedSelectorTemporary, Select->SelectorTemporary);

            GenerateLiteralInt(SwitchTable->HiVal, true);

            // 
            EndCodeBuffer(
                IsUnsignedType(vtype) ?
                    CEE_BGT_UN_S :
                    CEE_BGT_S,
                SwitchTable->pcblkCaseElse ?
                    SwitchTable->pcblkCaseElse :
                    Select->EndBlock,
                vtype);

            GenerateLoadOrElseLoadLocal(Select->LiftedSelectorTemporary, Select->SelectorTemporary);
            GenerateLiteralInt(SwitchTable->LowVal, true);

            EndCodeBuffer(
                IsUnsignedType(vtype) ?
                    CEE_BLT_UN_S :
                    CEE_BLT_S,
                SwitchTable->pcblkCaseElse ?
                    SwitchTable->pcblkCaseElse :
                    Select->EndBlock,
                vtype);

            // load up the select temp for the switch table
            GenerateLoadOrElseLoadLocal(Select->LiftedSelectorTemporary, Select->SelectorTemporary);
        }
        else
        {
            GenerateRvalue(Select->SelectorCapture->AsExpressionWithChildren().Right);
        }

        // add normalization instructions if needed
        // This will subtract the low value of the table to normalize it to 0
        //
        if (SwitchTable->LowVal != 0)
        {
            // signed because we can (and will) have negative numbers  //@UNSINGED - this comment doesn't mean anything anymore.
            GenerateLiteralInt(SwitchTable->LowVal, vtype == t_i8 || vtype == t_ui8 ? true : false);
            EmitOpcode(CEE_SUB, GetSymbolForVtype(vtype));
        }

        // switch opcodes only take I4, so convert if needed.  We've already
        // guarded against this conversion yielding an erroneous value (see above)
        //
        Vtypes vtypeNormalization = vtype;
        if (vtype == t_i8)
        {
            vtypeNormalization = t_i4;
            EmitOpcode(CEE_CONV_I4, GetSymbolForVtype(vtypeNormalization));
        }
        else if (vtype == t_ui8)
        {
            vtypeNormalization = t_ui4;
            EmitOpcode(CEE_CONV_U4, GetSymbolForVtype(vtypeNormalization));
        }

        // ENC requires a remappable point before each switch (VSW#205252).
        InsertENCRemappablePoint(GetSymbolForVtype(vtypeNormalization));

        StartHiddenIL();
        EndCodeBuffer(CEE_SWITCH, SwitchTable); // CEE_SWITCH does not push any values.

        // now we need to handle the default, fall through case.
        SetAsCurrentCodeBlock(SwitchTable->pcblkFallThrough);

        // if there's a CASE ELSE statment, then branch to it,
        // else branch to END SELECT
        //
        if (SwitchTable->pcblkCaseElse)
        {
            EndCodeBuffer(CEE_BR_S, SwitchTable->pcblkCaseElse);
        }
        else
        {
            EndCodeBuffer(CEE_BR_S, Select->EndBlock);
        }

        SetAsCurrentCodeBlock(0);
    }
    else
    {
        // Generate the select assignment statement
        GenerateRvalue(Select->SelectorCapture);
    }
}

//========================================================================
// Does the block end processing (patching jumps) for all control constructs.
//========================================================================

void CodeGenerator::GenerateEndSelect
(
ILTree::PILNode ptreeStmtCur    //SB_SELECT
)
{
    VSASSERT(ptreeStmtCur &&
               ptreeStmtCur->bilop == SB_SELECT,
               "GenerateEndSelect: ptreeStmtCur not set correctly");

    VSASSERT(ptreeStmtCur->AsSelectBlock().EndBlock,
               "GenerateEndSelect: select must have block end code block.");

    SetAsCurrentCodeBlock(ptreeStmtCur->AsSelectBlock().EndBlock);

    // fill in empty entries in SWITCH table
    if (ptreeStmtCur->uFlags & SBF_SELECT_TABLE)
    {
        SWITCH_TABLE  * pswTable;
        CODE_BLOCK    * pcblkDefault;

        // A SWITCH table may have empty entries. Set them to the branch
        // to the default.  The default is the Case Else stmt (if it exists),
        // otherwise it's the End Select block
        //
        pswTable = ptreeStmtCur->AsSelectBlock().SwitchTable;
        pcblkDefault =
            (pswTable->pcblkCaseElse) ?
                pswTable->pcblkCaseElse  :
                ptreeStmtCur->AsSelectBlock().EndBlock;

        // scan all entries
        for (unsigned index=0; index < pswTable->cEntries; index++)
        {
            if (!pswTable->pcodeaddrs[index].pcblk)
            {
                pswTable->pcodeaddrs[index].pcblk = pcblkDefault;
            }
        }
    }

    // Generate a NOP so that we can set a breakpoint on the End Select.
    // Update the resume table so we can Resume Next onto the End Select.
    UpdateLineTable(ptreeStmtCur->AsExecutableBlock().EndConstructLocation);
    UpdateResumeTable(ptreeStmtCur->AsExecutableBlock().TerminatingResumeIndex, false);
    InsertNOP();
} // GenerateEndSelect




void CodeGenerator::GenerateAsyncSwitch
(
ILTree::PILNode ptreeStmtCur    // SL_ASYNCSWITCH
)
{
    ILTree::AsyncSwitchStatement *Switch = &ptreeStmtCur->AsAsyncSwitchStatement();

    CODE_BLOCK *fallthroughBlock = NewCodeBlock();

    SWITCH_TABLE *table = AllocSwitchTable(Switch->cEntries);
    table->cEntries = Switch->cEntries;
    table->pcblkFallThrough = fallthroughBlock;
    for (unsigned long i=0; i<Switch->cEntries; i++)
    {
        LabelDeclaration *labl = Switch->Labels[i];
        VSASSERT(labl != NULL, "Unexpected label absence");
        CODE_BLOCK *tgt = labl->TargetCodeBlock;
        if (tgt == NULL)
        {
            tgt = NewCodeBlock();
            labl->TargetCodeBlock = tgt;
        }
        table->pcodeaddrs[i].pcblk = tgt;
    }

    VSASSERT(TypeHelpers::EquivalentTypes(Switch->NormalizedSelector->ResultType, GetSymbolForVtype(t_ui4)) ||
             TypeHelpers::EquivalentTypes(Switch->NormalizedSelector->ResultType, GetSymbolForVtype(t_i4)),
             "Expected AsyncSwitch on an Int32 or UInt32");
    GenerateRvalue(Switch->NormalizedSelector);
    EndCodeBuffer(CEE_SWITCH, table);

    SetAsCurrentCodeBlock(fallthroughBlock);
}


/*========================================================================

Error statements

========================================================================*/

//========================================================================
// Generate try block
//========================================================================
void CodeGenerator::GenerateTry
(
ILTree::PILNode ptreeStmt
)
{
    TRY_BLOCK        * ptry;
    TRY_BLOCK_HOLDER * ptryholder;

    VSASSERT( ptreeStmt->bilop == SB_TRY,
               "GenerateTry: must have try block node" );

    VSASSERT( ptreeStmt->AsStatement().Next &&
                (ptreeStmt->AsStatement().Next->bilop == SB_CATCH ||
                 ptreeStmt->AsStatement().Next->bilop == SB_FINALLY),
               "GenerateTry:  Try block must have catch or finally" );

    UpdateLineTable(ptreeStmt);
    InsertNOP();

    // allocate the try block
    ptry = (TRY_BLOCK *)m_pnra->Alloc(sizeof(TRY_BLOCK));
    ptryholder = (TRY_BLOCK_HOLDER *)m_pnra->Alloc(sizeof(TRY_BLOCK_HOLDER));

    // we've encountered a try, so insert it into the queue
    ptryholder->ptry = ptry;
    m_tryholderEncounteredList.InsertLast(ptryholder);

    // now we are "in" a new try block, so push it onto the stack
    m_tryListPending.InsertFirst(ptry);

    // End construct location is kept by the try, so store it into the Try stack so we can access it later
    ptry->EndTryLocation = ptreeStmt->AsExecutableBlock().EndConstructLocation;

    // In Semantics, it's easier to track non-synthetic.  In codegen, it's easier to track synthetic.  Reverse it here.
    ptry->IsSynthetic = !(ptreeStmt->uFlags & SBF_TRY_NOT_SYNTHETIC);

    ptry->pcblkTry = NewCodeBlock();
    ptry->pcblkFallThrough = NewCodeBlock();

    SetAsCurrentCodeBlock(ptry->pcblkTry);

    GenerateBlockStatementList(ptreeStmt->AsTryBlock());

    if (ptreeStmt->AsTryBlock().FinallyTree && ptry->EndTryLocation.IsEqual(&ptreeStmt->AsTryBlock().FinallyTree->Loc))
    {
        // The "nop" at the end of "Try" wil be mapped to the "Finally" statement. The "Leave" needs to be hidden
        // for ENC remap scenarios and hence the requirement for the "nop", else the "Leave" itself could have been
        // mapped to the "Finally" statement. Bug VSWhidbey 380886.
        // VSWhidbey[532615] This should only be generated if Finally is hidden such as in ForEach, Using and SyncLock
        UpdateLineTable(ptreeStmt->AsTryBlock().FinallyTree);
        InsertNOP();
    }
    else
    {
        // VS#271710
        StartHiddenIL();
    }

    LeaveCodeBuffer(ptry->pcblkFallThrough, true, false, false);
    SetAsCurrentCodeBlock(0);
}

//========================================================================
// Generate filter for a catch clause
//========================================================================

void
CodeGenerator::GenerateCatchFilter
(
    ILTree::CatchBlock *ptreeCatch
)
{
    CODE_BLOCK *pcblkTypeIsCorrect = NewCodeBlock();
    CODE_BLOCK *pcblkFilterEnd = NewCodeBlock();

    ILTree::ILNode         *CatchVariableTree  = ptreeCatch->CatchVariableTree;
    BCSYM_Variable  *ExceptionTemporary = ptreeCatch->ExceptionTemporary;
    ILTree::SymbolReferenceExpression * LiftedExceptionTemporary = ptreeCatch->LiftedExceptionTemporary;
    VSASSERT(ExceptionTemporary || !LiftedExceptionTemporary, "How did we lift an exception temporary that doesn't exist?");

    // We begin the filter with the exception object on the stack. However, we
    // haven't determined the exception's type yet, so just use Object for the
    // moment.
    AddToCurStack(1);
    m_stackTypes.PushOrEnqueue(GetSymbolForFXType(FX::ObjectType));

    // First check the type of the exception.  Use the catch variable's type, otherwise use System.Exception.
    BCSYM *CatchType;
    if (CatchVariableTree)
    {
        VSASSERT(CatchVariableTree->bilop == SX_SYM, "SX_SYM node needed");
        CatchType = CatchVariableTree->AsSymbolReferenceExpression().pnamed->PVariable()->GetType()->ChaseToType();
    }
    else
    {
        CatchType = GetSymbolForFXType(FX::ExceptionType);
    }

    mdTypeRef typref = m_pmdemit->DefineTypeRefBySymbol(CatchType, &ptreeCatch->Loc);

    StartHiddenIL();
    EmitOpcode_Tok(CEE_ISINST, typref, CatchType);

    BCSYM* pTypeOnTopOfStack = m_stackTypes.Peek();
    EmitOpcode(CEE_DUP, pTypeOnTopOfStack);
    EndCodeBuffer(CEE_BRTRUE_S, pcblkTypeIsCorrect);

    // If the type doesn't match, we need to pop the exception off the stack and exit the
    // filter to continue the search for a handler.

    SetAsCurrentCodeBlock(0);
    EmitOpcode(CEE_POP, Symbols::GetVoidType());
    GenerateLiteralInt(COMPLUS_FALSE);  // exception_continue_search
    EndCodeBuffer(CEE_BR_S, pcblkFilterEnd);

    // Otherwise the type matches.  Prepare to execute the When expression.
    AddToCurStack(1);
    m_stackTypes.PushOrEnqueue(CatchType);
    SetAsCurrentCodeBlock(pcblkTypeIsCorrect);

    // If there's no variable and we aren't setting the Err object, just pop the exception
    if (!CatchVariableTree && g_CompilingTheVBRuntime)
    {
        EmitOpcode(CEE_POP, Symbols::GetVoidType());
    }
    else
    {

        // If we're doing both, dup the exception
        if (CatchVariableTree && !g_CompilingTheVBRuntime)
        {
            pTypeOnTopOfStack = m_stackTypes.Peek();
            EmitOpcode(CEE_DUP, pTypeOnTopOfStack);
        }

        // If we have a catch variable, store the exception into it.
        if (CatchVariableTree)
        {
            bool isLocal = CatchVariableTree->AsSymbolReferenceExpression().pnamed->IsLocal();
            AssertIfFalse(isLocal || ptreeCatch->ExceptionStore);

            // For generic param typed exceptions, CEE_ISINST above produces a boxed object,
            // which needs to be unboxed to assign to the generic param typed catch variable.
            if (CatchType->IsGenericParam())
            {
                EmitOpcode_Tok(CEE_UNBOX_ANY, typref, CatchType);
            }

            // A temporary is needed if the catch variable is byref because the store opcode requires the
            // address to be loaded on the stack first.  So use the temporary to store the exception.
            if (ExceptionTemporary)
            {
                GenerateStoreOrElseStoreLocal(LiftedExceptionTemporary, ExceptionTemporary);
            }

            if ( isLocal )
            {
                // Now load the address of the catch variable.  This will do nothing unless the catch variable is byref.
                ADDRMODE addrmode;
                GenerateAddr(CatchVariableTree, &addrmode);

                // Load the exception from the temporary storage if we need to.
                if (ExceptionTemporary)
                {
                    GenerateLoadOrElseLoadLocal(LiftedExceptionTemporary, ExceptionTemporary);
                }

                // If this is a local variable then go ahead and perform the store.  Otherwise
                // it's the job of the code that created the exception variable to create a
                // store in the block from the exception temporary
                GenerateStore(CatchVariableTree, &addrmode);
            }
        }

        if ( ptreeCatch->ExceptionStore )
        {
            GenerateStatementList(ptreeCatch->ExceptionStore);
        }

        if (!g_CompilingTheVBRuntime)
        {
            // generated when we have a When clause in the catch statement.
            GenerateRTSetErrorMember(&ptreeCatch->Loc, Error);  // Bug #178026 - DevDiv Bugs: log an error
        }
    }

    // Now generate the When expression.
    UpdateLineTable(ptreeCatch);
    VSASSERT(ptreeCatch->WhenExpression->AsExpression().vtype == t_bool, "GenerateCatchFilter: When expression is not t_bool!");
    GenerateRvalue(ptreeCatch->WhenExpression);

    // Normalize the return value because values other than 0 or 1 produce unspecified results.
    GenerateLiteralInt(COMPLUS_FALSE);
    EmitOpcode(CEE_CGT_UN, GetSymbolForVtype(t_bool));

    SetAsCurrentCodeBlock(pcblkFilterEnd);
    StartHiddenIL();
    EndCodeBuffer(CEE_ENDFILTER);
    // Either 0 (exception_continue_search) or 1 (exception_execute_handler) remains on the stack when the filter finishes.
    AddToCurStack(-1);
    m_stackTypes.PopOrDequeue();
}

//========================================================================
// Generate catch clause
//========================================================================

void
CodeGenerator::GenerateCatch
(
    ILTree::CatchBlock *ptreeCatch
)
{
    TRY_BLOCK   *ptry;
    CATCH_BLOCK *pcatch;
    bool         HasNoFilter = ptreeCatch->WhenExpression ? false : true;

    VSASSERT(m_tryListPending.NumberOfEntries(), "catch with no try block!");

    ptry = m_tryListPending.GetFirst();
    pcatch = (CATCH_BLOCK *)m_pnra->Alloc(sizeof(CATCH_BLOCK));
    ptry->catchList.InsertLast(pcatch);


    // First, take care of the filter expression
    if (!HasNoFilter)
    {
        pcatch->pcblkFilter = NewCodeBlock();
        SetAsCurrentCodeBlock(pcatch->pcblkFilter);
        GenerateCatchFilter(ptreeCatch);
    }

    pcatch->pcblkCatch = NewCodeBlock();
    SetAsCurrentCodeBlock(pcatch->pcblkCatch);

    if (ptreeCatch->uFlags & SBF_CATCH_ASYNCSUB)
    {
        VSASSERT(m_pcblkAsyncSubCatch == NULL, "Expect at most one generated catch handler per method, specifically exactly one for async subs");
        m_pcblkAsyncSubCatch = pcatch->pcblkCatch;
    }

    StartHiddenIL();

    // Assume the type of this Catch clause is the default type (NULL maps to System.Exception in EmitEHClauses).
    // The filter itself will verify the exception's type is correct.
    //
    pcatch->psymType = NULL;


    BCSYM *CatchType;
    if (ptreeCatch->CatchVariableTree)
    {
        VSASSERT(ptreeCatch->CatchVariableTree->bilop == SX_SYM, "SX_SYM node needed");
        CatchType = ptreeCatch->CatchVariableTree->AsSymbolReferenceExpression().pnamed->PVariable()->GetType()->ChaseToType();
    }
    else
    {
        CatchType = GetSymbolForFXType(FX::ExceptionType);
    }

    // Catch starts with exception on stack
    AddToCurStack(1);
    m_stackTypes.PushOrEnqueue(CatchType);

    // Now, take care of the actual catch expression
    // Store the exception away or pop it if no symbol used (or if the filter stored it away already)
    //
    ILTree::ILNode *CatchVariableTree = ptreeCatch->CatchVariableTree;

    if (CatchVariableTree)
    {
        pcatch->ExceptionVarLocation = CatchVariableTree->Loc;

        // We need to store to both the local and the Err object, so dup the exception.
        // Only do this if there's no filter since the filter would have already done this for us.
        if (HasNoFilter)
        {
            VSASSERT(CatchVariableTree->bilop == SX_SYM, "SX_SYM node needed");

            bool isLocal = CatchVariableTree->AsSymbolReferenceExpression().pnamed->IsLocal();
            BCSYM *CatchVariableType = CatchVariableTree->AsSymbolReferenceExpression().ResultType;
            BCSYM_Variable *ExceptionTemporary = ptreeCatch->ExceptionTemporary;
            ILTree::SymbolReferenceExpression * LiftedExceptionTemporary = ptreeCatch->LiftedExceptionTemporary;
            VSASSERT(ExceptionTemporary || !LiftedExceptionTemporary, "How did we lift an exception temporary that doesn't exist?");

            VSASSERT(isLocal || ExceptionTemporary, "Exception variable must either be a local or we need an exception temparory");
            VSASSERT(isLocal || ptreeCatch->ExceptionStore, "Must be a local or code generated to store to the target");

            if (!g_CompilingTheVBRuntime)
            {
                BCSYM* pTypeOnTopOfStack = m_stackTypes.Peek();
                EmitOpcode(CEE_DUP, pTypeOnTopOfStack);
                // catch ex as exception comes through here.
                GenerateRTSetErrorMember(&ptreeCatch->Loc, Error);  // Bug #178026 - DevDiv Bugs: log an error
            }

            // For generic param catch type, the boxed form is on the stack. So in order to store it into the generic param
            // type catch variable, it needs to be unboxed.
            if (CatchVariableType->IsGenericParam())
            {
                EmitOpcode_Tok(CEE_UNBOX_ANY, m_pmdemit->DefineTypeRefBySymbol(CatchVariableType, &CatchVariableTree->Loc), CatchVariableType);
            }

            // A temporary is needed if the catch variable is byref because the store opcode requires the
            // address to be loaded on the stack first.  So use the temporary to store the exception.
            if (ExceptionTemporary)
            {
                GenerateStoreOrElseStoreLocal(LiftedExceptionTemporary, ExceptionTemporary);
            }

            if ( isLocal )
            {
                // Now load the address of the catch variable.  This will do nothing unless the catch variable is byref.
                // If it's not a local then there is no way it can be byref
                ADDRMODE addrmode;
                GenerateAddr(CatchVariableTree, &addrmode);

                // Load the exception from the temporary storage if we need to
                if (ExceptionTemporary)
                {
                    GenerateLoadOrElseLoadLocal(LiftedExceptionTemporary, ExceptionTemporary);
                }

                // If this is a local variable then go ahead and perform the store.  Otherwise
                // it's the job of the code that created the exception variable to create a
                // store in the block from the exception temporary
                GenerateStore(CatchVariableTree, &addrmode);
            }

            if ( ptreeCatch->ExceptionStore)
            {
                GenerateStatementList(ptreeCatch->ExceptionStore);
            }

            // Set the type of this catch clause to the local's type
            pcatch->psymType = CatchVariableType;
        }
        else
        {
            // A filter will have already set the Exception Local, so pop this off the stack because we don't need it.
            EmitOpcode(CEE_POP, Symbols::GetVoidType());
        }
    }
    else
    {
        pcatch->ExceptionVarLocation.Invalidate();

        // If there is one, the filter will have already set the Err object
        if (HasNoFilter && !g_CompilingTheVBRuntime)
        {   // for plain catch (with no expression) statements
            GenerateRTSetErrorMember(&ptreeCatch->Loc, Error);   // Bug #178026 - DevDiv Bugs: log an error
        }
        // If the Err object has already been set and we have no local, then we have no need for
        // the exception on the stack
        else
        {
            EmitOpcode(CEE_POP, Symbols::GetVoidType());
        }
    }

    if (HasNoFilter)
    {
        // Generate a NOP to allow breakpoints after the exception local has been set.
        UpdateLineTable(ptreeCatch);
        InsertNOP();
    }

    GenerateBlockStatementList(*ptreeCatch);

    pcatch->pcblkLast = m_pcblkCurrent;

    LeaveCodeBuffer(ptry->pcblkFallThrough, false, true, false, false, true, &ptreeCatch->Loc);

    // HACK:  Leaving the code buffer may overflow the current codebuffer and create a new one.
    // If this situation is not detected, then the handler offsets will be calculated incorrectly.
    if (pcatch->pcblkLast->opcodeJump == CEE_NOP)
    {
        VSASSERT(pcatch->pcblkLast->Next()->opcodeJump == CEE_LEAVE_S, "expected leave");
        pcatch->pcblkLast = pcatch->pcblkLast->Next();
    }

    if (!ptreeCatch->AsStatement().Next ||
        (ptreeCatch->AsStatement().Next->bilop != SB_CATCH &&
         ptreeCatch->AsStatement().Next->bilop != SB_FINALLY))
    {
        // Last handler, link in final list
        m_tryListPending.Remove(ptry);
        m_tryList.InsertLast(ptry);
        SetAsCurrentCodeBlock(ptry->pcblkFallThrough);

        // Generate a NOP so that we can set a breakpoint on the End Try statement
        UpdateLineTable(ptry->EndTryLocation);
        InsertNOP();
    }
    else
    {
        SetAsCurrentCodeBlock(0);
    }
}

//========================================================================
// Generate finally clause
//========================================================================

void CodeGenerator::GenerateFinally
(
ILTree::PILNode ptreeStmtCur
)
{
    TRY_BLOCK  * ptry;
    CODE_BLOCK * pcblkFinally;

    VSASSERT(m_tryListPending.NumberOfEntries(),
              "finally with no try block!");

    // Generate finally block
    pcblkFinally = NewCodeBlock();
    SetAsCurrentCodeBlock(pcblkFinally);

    ptry = m_tryListPending.GetFirst();
    if (!ptry->EndTryLocation.IsEqual(&ptreeStmtCur->Loc))
    {
        // VSWhidbey[532615] This should not generated if Finally is hidden such as in ForEach, Using and SyncLock
        UpdateLineTable(ptreeStmtCur);
        InsertNOP();
    }

    GenerateBlockStatementList(ptreeStmtCur->AsFinallyBlock());

    // Bug VSWhidbey 365232. Need to generate a hidden sequence point for the "endfinally".
    StartHiddenIL();

    EndCodeBuffer(CEE_ENDFINALLY);
    SetAsCurrentCodeBlock(0);

    // Set it in the try block
    ptry->pcblkFinally = pcblkFinally;

    // Remove try block from stack, add to queue
    m_tryListPending.Remove(ptry);
    m_tryList.InsertLast(ptry);
    SetAsCurrentCodeBlock(ptry->pcblkFallThrough);

    if (!ptry->EndTryLocation.IsEqual(&ptreeStmtCur->Loc))
    {
        // Generate a NOP so that we can set a breakpoint on the next statement.
        // But we don't generate this in scenarios like the "Using" statement
        // where both the "finally and the "end try" point to the same "End Using"
        // statement.
        //
        UpdateLineTable(ptry->EndTryLocation);
        InsertNOP();
    }
}

//========================================================================
// Generate resume statement
//========================================================================

void CodeGenerator::GenerateResume
(
ILTree::PILNode ptreeStmtCur    // SL_RESUME
)
{
    CODE_BLOCK * pcblkTrue;
    CODE_BLOCK * pcblkFalse;

    pcblkTrue = NewCodeBlock();
    pcblkFalse = NewCodeBlock();

    VSASSERT(ptreeStmtCur && ptreeStmtCur->bilop == SL_RESUME,
              "GenerateResume: current stmt must be SL_RESUME.");

    VSASSERT(m_usResumeLocal != INVALID_LCL_INDEX,
              "GenerateResume: must have an on error local.");

    StartHiddenIL();

    // Reset the error object
    GenerateCallToRuntimeHelper(ClearErrorMember, Symbols::GetVoidType(), &ptreeStmtCur->Loc, Error ); // There isn't location info for the Resume statement so we use the method declaration loc.

    // Emit code to check to see if we're in a handler. Compare m_usResumeLocal against 0.
    GenerateLoadLocal(m_usResumeLocal, GetSymbolForVtype(t_i4));
    EndCodeBuffer(CEE_BRTRUE_S, pcblkTrue);
    SetAsCurrentCodeBlock(pcblkFalse);

    StartHiddenIL();

    // pcblkFalse will be current. Throw the error
    GenerateRTException(E_RESUMEWITHOUTERROR, &ptreeStmtCur->Loc);

    // Now generate code based on what kind of Resume we have
    SetAsCurrentCodeBlock(pcblkTrue);

    if (ptreeStmtCur->AsOnErrorStatement().Label)
    {
        // Resume label. Reset resume local
        GenerateLiteralInt(0);
        GenerateStoreLocal(m_usResumeLocal);

        // Each Resume gets two resume indices, so do +1 to get the second one.
        UpdateResumeTable(ptreeStmtCur->AsStatement().ResumeIndex + 1, false);
        GenerateGotoLabel(
            ptreeStmtCur->AsOnErrorStatement().Label,
            ptreeStmtCur->uFlags & SLF_EXITS_TRY,
            ptreeStmtCur->uFlags & SLF_EXITS_CATCH,
            ptreeStmtCur->uFlags & SLF_EXITS_CATCH_TO_CORRESPONDING_TRY);
    }
    else
    {
        // If this is a Resume Next statement, branch to the codeblock which
        // increments the resume local (since it holds the current line)
        // Else we have a Resume statement which doesn't increment the resume local,
        // so branch to the codeblock that just loads it.
        //
        CODE_BLOCK * pcblkJumpDest =
            (ptreeStmtCur->uFlags & SLF_ONERR_NEXT) ?
                m_pcblkLoadForResumeNext :
                m_pcblkLoadForResume;

        // A Resume statement will generate a branch to the Resume Table machinery.
        // If this Resume statement is nested in a Try/Catch block, the branch needs to be a Leave.
        // We know we are in a nested Try/Catch block if the number of pending Try's is 2 or greater
        // (since the first one will be the On Error mechanism).
        //
        bool UseLeave = m_tryListPending.NumberOfEntries() > 1;

        if (UseLeave)
        {
            LeaveCodeBuffer(pcblkJumpDest, true, false, false);
        }
        else
        {
            EndCodeBuffer(CEE_BR_S, pcblkJumpDest);
        }

        SetAsCurrentCodeBlock(0);
    }
}

//========================================================================
// Generate On...Error statement
//========================================================================

void
CodeGenerator::GenerateOnError
(
    ILTree::PILNode ptreeStmtCur    // SL_ON_ERR
)
{
    VSASSERT(ptreeStmtCur && ptreeStmtCur->bilop == SL_ON_ERR,
               "GenerateOn ptreeStmtCur not set correctly.");

    // Reset the error object
    GenerateCallToRuntimeHelper(ClearErrorMember, Symbols::GetVoidType(), &ptreeStmtCur->Loc);

    // If the code only has On Error Goto 0, we don't actually
    // generate any handler code.
    //
    if (m_usOnErrorLocal == INVALID_LCL_INDEX)
    {
        return;
    }

    int OnErrorIndex = 0;

    // Undocumented feature. -1 means to reset the handler. So, if
    // we're currently in a handler, this instruction resets state
    // to normal.
    if (ptreeStmtCur->uFlags & SLF_ONERR_MINUS1)
    {
        // Reset the resume local.
        GenerateLiteralInt(DISABLE_HANDLER);
        GenerateStoreLocal(m_usResumeLocal);
    }
    else
    {
        if (ptreeStmtCur->AsOnErrorStatement().Label)
        {
            VSASSERT(ptreeStmtCur->AsOnErrorStatement().Label,
                       "GenerateOn must have a label.");

            OnErrorIndex = ptreeStmtCur->AsOnErrorStatement().OnErrorIndex;
            VSASSERT(1 < OnErrorIndex && OnErrorIndex < (long)m_ptblswOnError->cEntries,
                     "On Error index is out of range : #11/19/2003#");

            // Save the code page into the switch table
            if (!ptreeStmtCur->AsOnErrorStatement().Label->TargetCodeBlock)
            {
                ptreeStmtCur->AsOnErrorStatement().Label->TargetCodeBlock = NewCodeBlock();
            }

            m_ptblswOnError->pcodeaddrs[OnErrorIndex].pcblk =
                ptreeStmtCur->AsOnErrorStatement().Label->TargetCodeBlock;
        }
        else if (ptreeStmtCur->uFlags & SLF_ONERR_NEXT)
        {
            if (m_Project->GenerateDebugCode())
            {
                OnErrorIndex = ptreeStmtCur->AsOnErrorStatement().OnErrorIndex;
                VSASSERT(RESUME_NEXT_BASE >= OnErrorIndex, "Invalid On Error Resume Index");
            }
            else
            {
                OnErrorIndex = RESUME_NEXT;
            }
        }
        else
        {
            OnErrorIndex = DISABLE_HANDLER;
        }

        // Update the current handler variable.
        GenerateLiteralInt(OnErrorIndex);
        GenerateStoreLocal(m_usOnErrorLocal);
    }
}

/*========================================================================

Branch statements

========================================================================*/

//========================================================================
// Generate a goto statement
//========================================================================

void
CodeGenerator::GenerateGotoLabel
(
    LABEL_ENTRY_BIL *pLabel,
    bool ExitsTry,
    bool ExitsCatch,
    bool ExitsCatchToCorrespondingTry
)
{
    // If there is no code block defined for the label (ie label has
    // not been processed yet, create one and save it in the label.)
    // Jump to the code block.
    //
    if (!pLabel->TargetCodeBlock)
    {
        pLabel->TargetCodeBlock = NewCodeBlock();
    }

    if (ExitsTry || ExitsCatch)
    {
        LeaveCodeBuffer(pLabel->TargetCodeBlock, ExitsTry, ExitsCatch, ExitsCatchToCorrespondingTry, true /* Insert nop for better debugging */);
    }
    else
    {
        EndCodeBuffer(CEE_BR_S, pLabel->TargetCodeBlock);
    }
    SetAsCurrentCodeBlock(0);
}

//========================================================================
// Create a block for a label and store line info in Erl
//========================================================================

void CodeGenerator::GenerateLabel
(
ILTree::PILNode ptreeStmtCur
)
{

    if (m_Project->GenerateENCableCode())
    {
        // ENC requires a remappable point before each label (VSW#178951).
        SetAsCurrentCodeBlock(0);
        InsertENCRemappablePoint();
    }

    if (!ptreeStmtCur->AsLabelStatement().Label->TargetCodeBlock)
    {
        // Create a new code block for the label so we can jump to
        // it. Store code block in the label's cookie.
        //
        ptreeStmtCur->AsLabelStatement().Label->TargetCodeBlock = NewCodeBlock();
    }

    SetAsCurrentCodeBlock(ptreeStmtCur->AsLabelStatement().Label->TargetCodeBlock);

    UpdateLineTable(ptreeStmtCur);

    // Now store away the line number if there is one, but only
    // if we have an error handler present.
    //
    if (NeedLineNumber() &&
        ptreeStmtCur->AsLabelStatement().Label->IsLineNumber)
    {
        VSASSERT(m_usCurLineLocal != INVALID_LCL_INDEX, "Line number local not found");

        unsigned short usLineNum;

        UpdateResumeTable(ptreeStmtCur->AsLabelStatement().ResumeIndex);
        usLineNum = (unsigned short)_wtol(ptreeStmtCur->AsLabelStatement().Label->Name);
        GenerateLiteralInt(usLineNum);
        GenerateStoreLocal(m_usCurLineLocal);
    }
    else
    {
        // Generate a NOP so that we can set a breakpoint on the label.
        // Force the NOP if this is the target of a GoTo which exits a Try/Catch
        // because otherwise the NOP will disappear with -optimize+ and the label
        // could become associated with IL inside a Try block (see VSW#5876).
        //
        InsertNOP(ptreeStmtCur->AsLabelStatement().Label->IsBranchTargetFromEHContext);
    }
}

//========================================================================
// Generates code for Exit Sub/Function/Property and Return statements
//========================================================================

void CodeGenerator::GenerateReturn
(
ILTree::PILNode ptreeReturn
)
{
    VSASSERT(ptreeReturn->bilop == SL_RETURN, "return node expected");

    ILTree::PILNode ReturnExpression = ptreeReturn->AsReturnStatement().ReturnExpression;

    // You cannot just return out of a try block -- you have to do a leave so the finally will run.
    bool UseLeave = m_tryListPending.NumberOfEntries() > 0;

    // Store the return expression to the return variable if the expression is within the
    // context of a Try.  Additionally, to guarantee correct stepping behavior, store to the return
    // variable if optimizations are turned off.
    //
    bool StoreToReturnVariable = WithinFunction() && (UseLeave || !m_Project->GenerateOptimalIL());

    // Load the return expression if one is supplied.
    if (ReturnExpression)
    {
        VSASSERT(WithinFunction(), "how can we have a return expression and not be within a function?");

        UpdateLineTable(ptreeReturn);
        // The return expression can throw an exception, so update the resume local.
        UpdateResumeTable(ptreeReturn->AsReturnStatement().ResumeIndex);

        if (StoreToReturnVariable)
        {
            if (m_ptreeFunc->AsProcedureBlock().LiftedReturnVarRef)
            {
                // Now we will store the return value to the corresponding closure class field.
                ADDRMODE addrmode;
                GenerateAddr(m_ptreeFunc->AsProcedureBlock().LiftedReturnVarRef, &addrmode);
                GenerateRvalue(ReturnExpression);
                GenerateStore(m_ptreeFunc->AsProcedureBlock().LiftedReturnVarRef, &addrmode);
            }
            else
            {
                GenerateRvalue(ReturnExpression);
                GenerateStoreLocal(m_ptreeFunc->AsProcedureBlock().ReturnVariable->GetLocalSlot());
            }
        }
        else
        {
            GenerateRvalue(ReturnExpression);

            // The return expression result will be left on the stack and consumed
            // by the Ret opcode.
            //
            AddToCurStack(-1);
            m_stackTypes.PopOrDequeue();
        }
    }
    else
    {
        // This is an Exit Sub/Function/Property statement.

        UpdateLineTable(ptreeReturn);
        // No exception is possible.
        UpdateResumeTable(ptreeReturn->AsReturnStatement().ResumeIndex, false);
        // Generate a NOP so that we can set a breakpoint on this line.
        InsertNOP();
    }


    // The return statement branches to the code block in the Epilogue that loads the return variable
    // if we are within a function and we stored to the return variable.  Exit Function/Property is a
    // special-case because no return expression is supplied.
    //
    CODE_BLOCK * pcblkExitPoint =
        WithinFunction() && (StoreToReturnVariable || ReturnExpression == NULL) ?
            m_pcblkEndProcLoadLocal :
            m_pcblkEndProc;

    if (UseLeave)
    {
        VSASSERT(
            ptreeReturn->uFlags & SLF_EXITS_TRY ||
            ptreeReturn->uFlags & SLF_EXITS_CATCH ||
            m_tryListPending.GetFirst() == m_ptryOnError,
            "GenerateEndProc: inconsistent flags");

        LeaveCodeBuffer(
            pcblkExitPoint,
            ptreeReturn->uFlags & SLF_EXITS_TRY,
            ptreeReturn->uFlags & SLF_EXITS_CATCH,
            ptreeReturn->uFlags & SLF_EXITS_CATCH_TO_CORRESPONDING_TRY);
    }
    else
    {
        EndCodeBuffer(CEE_BR_S, pcblkExitPoint);
    }

    SetAsCurrentCodeBlock(0);
}

//========================================================================
// Generate code for an exit select statement
//========================================================================

void
CodeGenerator::GenerateExitSelect
(
    ILTree::PILNode ptreeStmtCur    // SL_EXIT
)
{
    // Look at the containing select for branch out
    VSASSERT(ptreeStmtCur->AsExitStatement().ExitedStatement &&
               ptreeStmtCur->AsExitStatement().ExitedStatement->AsSelectBlock().EndBlock,
               "GenerateRvalue: ExitSelect: containing select must have branch out.");

    UpdateResumeTable(ptreeStmtCur->AsExitStatement().ResumeIndex, false);

    // If the select is not in the same try block as the exit, then
    // we need to generate a leave so that all finallys can be run.
    if (ptreeStmtCur->uFlags & SLF_EXITS_TRY || ptreeStmtCur->uFlags & SLF_EXITS_CATCH)
    {
        LeaveCodeBuffer(
            ptreeStmtCur->AsExitStatement().ExitedStatement->AsSelectBlock().EndBlock,
            ptreeStmtCur->uFlags & SLF_EXITS_TRY,
            ptreeStmtCur->uFlags & SLF_EXITS_CATCH,
            ptreeStmtCur->uFlags & SLF_EXITS_CATCH_TO_CORRESPONDING_TRY);
    }
    else
    {
        EndCodeBuffer(CEE_BR_S, ptreeStmtCur->AsExitStatement().ExitedStatement->AsSelectBlock().EndBlock);
    }

    SetAsCurrentCodeBlock(0);
} // GenerateExitSelect


//========================================================================
// Generate code for an exit loop statement
//========================================================================

void
CodeGenerator::GenerateExitLoop
(
    ILTree::PILNode ptreeStmtCur    // SL_EXIT
)
{
    // Look at the containing loop for branch out
    VSASSERT(ptreeStmtCur->AsExitStatement().ExitedStatement &&
               ptreeStmtCur->AsExitStatement().ExitedStatement->AsGeneralLoopBlock().EndBlock,
               "GenerateRvalue: ExitLoop: containing loop must have branch out.");

    UpdateResumeTable(ptreeStmtCur->AsExitStatement().ResumeIndex, false);

    // If the loop is not in the same try block as the exit, then
    // we need to generate a leave so that all finallys can be run.
    if (ptreeStmtCur->uFlags & SLF_EXITS_TRY || ptreeStmtCur->uFlags & SLF_EXITS_CATCH)
    {
        LeaveCodeBuffer(
            ptreeStmtCur->AsExitStatement().ExitedStatement->AsGeneralLoopBlock().EndBlock,
            ptreeStmtCur->uFlags & SLF_EXITS_TRY,
            ptreeStmtCur->uFlags & SLF_EXITS_CATCH,
            ptreeStmtCur->uFlags & SLF_EXITS_CATCH_TO_CORRESPONDING_TRY);
    }
    else
    {
        EndCodeBuffer(CEE_BR_S, ptreeStmtCur->AsExitStatement().ExitedStatement->AsGeneralLoopBlock().EndBlock);
    }

    SetAsCurrentCodeBlock(0);
} // GenerateExitLoop

//========================================================================
// Generate code for continue statement
//========================================================================

void
CodeGenerator::GenerateContinue
(
    ILTree::PILNode ptreeStmtCur
)
{
    // Look at the containing loop for branch out
    VSASSERT(ptreeStmtCur->AsContinueStatement().EnclosingLoopStatement,
                "GenerateContinue: AsContinueStatement: no containing loop.");

    ILTree::PILNode EnclosingLoop = ptreeStmtCur->AsContinueStatement().EnclosingLoopStatement;
    CODE_BLOCK *ContinueBranch = EnclosingLoop->AsGeneralLoopBlock().ContinueBlock;

    VSASSERT(ContinueBranch, "GenerateContinue: AsContinueStatement: Continue Block not set.");

    // If the loop is not in the same try block as the exit, then
    // we need to generate a leave so that all finallys can be run.
    if (ptreeStmtCur->uFlags & SLF_EXITS_TRY || ptreeStmtCur->uFlags & SLF_EXITS_CATCH)
    {
        LeaveCodeBuffer(
            ContinueBranch,
            ptreeStmtCur->uFlags & SLF_EXITS_TRY,
            ptreeStmtCur->uFlags & SLF_EXITS_CATCH,
            ptreeStmtCur->uFlags & SLF_EXITS_CATCH_TO_CORRESPONDING_TRY,
            true);  // Insert nop so that breakpoint can be set
    }
    else
    {
        EndCodeBuffer(CEE_BR_S, ContinueBranch);
    }

    SetAsCurrentCodeBlock(0);
}


/*========================================================================

Loops

========================================================================*/

//========================================================================
// Generate code for For Each
//========================================================================

void CodeGenerator::GenerateForEachLoop
(
ILTree::PILNode ptreeStmtCur
)
{
    VSASSERT(ptreeStmtCur->bilop == SB_FOR_EACH, "GenerateForEachLoop: invalid For Each tree");
    VSASSERT(ptreeStmtCur->uFlags & SBF_DO_WHILE, "GenerateForEachLoop: invalid loop kind");

    CODE_BLOCK * pcblkBody = NewCodeBlock();
    CODE_BLOCK * pcblkCondition = NewCodeBlock();
    CODE_BLOCK * pcblkOut = NewCodeBlock();
    CODE_BLOCK * pcblkNext = NewCodeBlock();

    // save the loop exit in the tree for the Exit statment
    ptreeStmtCur->AsLoopBlock().EndBlock = pcblkOut;

    // save the for each loop "Next" in the tree for the Continue statement
    ptreeStmtCur->AsLoopBlock().ContinueBlock = pcblkNext;

    // Initialize the For Each control
    UpdateLineTable(ptreeStmtCur);
    UpdateResumeTable(ptreeStmtCur->AsForEachBlock().ResumeIndex);

    if (ptreeStmtCur->AsForEachBlock().ControlVaribleTempInitializer != NULL)
    {
        GenerateRvalue(ptreeStmtCur->AsForEachBlock().ControlVaribleTempInitializer);	
    }

    GenerateRvalue(ptreeStmtCur->AsForEachBlock().Initializer);

    // Branch immediately to the initial condition check
    EndCodeBuffer(CEE_BR_S, pcblkCondition);

    // Generate the loop body
    SetAsCurrentCodeBlock(pcblkBody);

    // Generate Closure Initialization Code
    GenerateStatementList(ptreeStmtCur->AsForEachBlock().InitializeClosure);

    // In a For Each loop, the index assignment occurs at the beginning of each iteration.
    // Mark it hidden because we don't want it associated with the first statement in the loop body (because
    // doing a set-next-statement on the first statement in the loop body would reassign the index again)
    //
    StartHiddenIL();
    GenerateRvalue(ptreeStmtCur->AsForEachBlock().CurrentIndexAssignment);

    // Generate the actual body statements
    GenerateBlockStatementList(ptreeStmtCur->AsExecutableBlock());

    // Generate the "Next" statement index increment
    //
    // Note that ptreeStmtCur->AsForEachBlock().IncrementStatement will be NULL for complex "For Each" i.e.
    // for each on Multi-Dimensional arrays, etc. This is because the increment is the MoveNext
    // call which is part of the condition. But we still want to do the SetAsCurrentBlock because
    // any continue statements in this loop would have branched to this block.
    //
    SetAsCurrentCodeBlock(pcblkNext);
    GenerateStatementList(ptreeStmtCur->AsForEachBlock().IncrementStatement);

    // Insert a NOP so we can step on the "Next" statment
    UpdateLineTable(ptreeStmtCur->AsExecutableBlock().EndConstructLocation);
    UpdateResumeTable(ptreeStmtCur->AsForEachBlock().TerminatingResumeIndex);  // don't specify False here because this resume table entry covers the condition code generated below
    InsertNOP();

    // Generate the condition
    SetAsCurrentCodeBlock(pcblkCondition);

    // Mark it hidden because we don't want it associated with the "Next" statement because...?
    StartHiddenIL();
    GenerateConditionWithENCRemapPoint(ptreeStmtCur->AsLoopBlock().Conditional, false, &pcblkBody, &pcblkOut);
}


//========================================================================
// Generate code for all While, Do While, Do Until, Loop While, Loop Until
//========================================================================

void
CodeGenerator::GenerateLoop
(
    ILTree::LoopBlock *Loop     // SB_LOOP
)
{
    VSASSERT(Loop->uFlags & (SBF_DO_WHILE | SBF_LOOP_WHILE | SBF_DO_LOOP),
             "GenerateLoop: invalid loop kind: one of these three should be set");

    CODE_BLOCK *pcblkBody = NewCodeBlock();
    CODE_BLOCK *pcblkCondition = NewCodeBlock();
    CODE_BLOCK *pcblkOut = NewCodeBlock();

    // save the loop exit in the tree for the Exit statment
    Loop->EndBlock = pcblkOut;

    // save the loop Condition in the tree for the Continue statement
    Loop->ContinueBlock = pcblkCondition;

    unsigned short ResumeIndex = 0;

    // branch to condition check immediately if we have a Do While loop
    if (Loop->uFlags & SBF_DO_WHILE)
    {
        StartHiddenIL();
        UpdateResumeTable(Loop->ResumeIndex, false);
        EndCodeBuffer(CEE_BR_S, pcblkCondition);
        ResumeIndex = AllocateResumeEntry(pcblkCondition, Loop->ResumeIndex + 1);
    }
    else
    {
        UpdateLineTable(Loop);
        UpdateResumeTable(Loop->ResumeIndex, false);
        InsertNOP();
    }

    // Generate the loop body
    SetAsCurrentCodeBlock(pcblkBody);
    GenerateBlockStatementList(*Loop);

    // A Do While loop's condition is not part of the end construct, so we need to insert a NOP for
    // the end construct then update the line table to the location of the loop condition
    //
    if (Loop->uFlags & SBF_DO_WHILE)
    {
        UpdateLineTable(Loop->EndConstructLocation);
        UpdateResumeTable(Loop->TerminatingResumeIndex, false);
        InsertNOP();
    }

    // Generate the loop condition
    SetAsCurrentCodeBlock(pcblkCondition);

    if (Loop->uFlags & SBF_DO_LOOP)
    {
        // a Do..Loop has no condition, so do an unconditional branch to the top of the loop body
        UpdateLineTable(Loop->EndConstructLocation);
        UpdateResumeTable(Loop->TerminatingResumeIndex, false);
        EndCodeBuffer(CEE_BR_S, pcblkBody);
        SetAsCurrentCodeBlock(pcblkOut);
    }
    else
    {
        if (Loop->uFlags & SBF_DO_WHILE)
        {
            UpdateLineTable(Loop);
            VSASSERT(ResumeIndex != 0, "invalid resume entry");
            UpdateCurrentStatementLocal(ResumeIndex);
        }
        else
        {
            UpdateLineTable(Loop->EndConstructLocation);
            UpdateResumeTable(Loop->TerminatingResumeIndex);
        }

        GenerateConditionWithENCRemapPoint(Loop->Conditional, false, &pcblkBody, &pcblkOut);
    }

} // GenerateLoop


//========================================================================
//  Generate the initialization code for an intrinsic for loop.
//
//  We will generate the following:
//
//             ptreeVar = ptreeBeg;
//             LimitTemp = ptreeEnd;  (optimized away if possible)
//             StepTemp  = ptreeStep;  (optimized away if possible)
//
//             goto cond (optimized away if possible)
//
//      body:
//             loop body
//
//             ptreeVar += StepTemp
//
//      cond:
//             condition check
//             if (true) goto body
//
//      exit:
//             ...
//
//
//   The condition check can be one of:
//     known step:     i)  ptreeVar <= LimitTemp
//                     ii) ptreeVar >= LimitTemp
//
//     unknown step:
//                     iii) (StepTemp >> (sizeof(StepTemp)*8-1)) ^ sCount <=
//                             (sStep >> (sizeof(sStep)*8-1)) ^ sLimit
//
//                     iv)  RT func(ptreeVar, LimitTemp, StepTemp)
//========================================================================

void CodeGenerator::GenerateForLoopStart
(
ILTree::PILNode ptreeFor
)
{
    VSASSERT( ptreeFor->bilop == SB_FOR,
               "GenerateForLoopStart: current stmt must be SB_FOR." );

    VSASSERT( ptreeFor->AsForBlock().ControlVariableReference != NULL &&
              ptreeFor->AsForBlock().Step != NULL &&
              ptreeFor->AsForBlock().Initial != NULL &&
              ptreeFor->AsForBlock().Limit != NULL,
              "GenerateForLoopStart: For tree needs control expressions" );

    ILTree::PILNode  ptreeVar  = ptreeFor->AsForBlock().ControlVariableReference;
    Vtypes    vtypeLoop = ptreeFor->AsForBlock().ControlVariableReference->AsExpression().vtype;

    BCSYM_Variable * pLimitTemp = ptreeFor->AsForBlock().LimitTemporary;
    BCSYM_Variable * pStepTemp = ptreeFor->AsForBlock().StepTemporary;
    // If the for loop had been lifted, then we have to use lifted temporaries instead:
    ILTree::SymbolReferenceExpression * pLiftedLimitTemp = ptreeFor->AsForBlock().LiftedLimitTemporary;
    ILTree::SymbolReferenceExpression * pLiftedStepTemp  = ptreeFor->AsForBlock().LiftedStepTemporary;
    VSASSERT(pLimitTemp || !pLiftedLimitTemp, "How did we lift a limit temp that doesn't exist?");
    VSASSERT(pStepTemp  || !pLiftedStepTemp,  "How did we lift a step temp that doesn't exist?");


    // Create forward labels for the condition and for the blockend
    ptreeFor->AsForBlock().ConditionBlock = NewCodeBlock();
    ptreeFor->AsForBlock().EndBlock  = NewCodeBlock();

    Queue<HydrationStep*, NorlsAllocWrapper>* hydrationContextBackupForInitial = NULL;
    ADDRMODE  addrmodeForInitial;
  
    
    // Load the initial value onto the stack
    {
        // As specified on MSDN, the evaluation order for the expressions in header of For loop is initial, limit, step, the last step is to 
        // assign initial value to control variable. Then we have to generate initial value here, then store it after limit and step. 
        // HydrationContext is needed if one of initial, limit, step contains async spill.
        BackupValue<Queue<HydrationStep*, NorlsAllocWrapper>*> hydrationContextBackup(&m_currentHydrationContext);        
        hydrationContextBackupForInitial = 
            ptreeFor->AsForBlock().Initial->ContainsAsyncSpill || 
            ptreeFor->AsForBlock().Limit->ContainsAsyncSpill   || 
            ptreeFor->AsForBlock().Step->ContainsAsyncSpill 
            ? new (*m_pnra) Queue<HydrationStep*, NorlsAllocWrapper>(m_nraWrapper) : NULL;
        m_currentHydrationContext = hydrationContextBackupForInitial;

        GenerateAddr(ptreeVar, &addrmodeForInitial);
        GenerateRvalue(ptreeFor->AsForBlock().Initial);
    }

    // The following code is weird. It pushes the address of LiftedLimitTemp onto the
    // stack, then does the step, then stores the limit. It should just store the LiftedLimitTemp
    // straight away.

    // If semantics supplies us with a Temporary to use, then use it regardless of const-ness.
    ADDRMODE addrmode_LiftedLimitTemp;
    if ((ptreeFor->AsForBlock().Limit->Sxkind() & SXK_CONST) && !pLimitTemp)
    {
        // Set flag so we can check for this faster later
        ptreeFor->uFlags |= SBF_FOR_CNS_LIMIT;
    }
    else
    {
        BackupValue<Queue<HydrationStep*, NorlsAllocWrapper>*> hydrationContextBackup(&m_currentHydrationContext);
        m_currentHydrationContext = ptreeFor->AsForBlock().Limit->ContainsAsyncSpill ? new (*m_pnra) Queue<HydrationStep*, NorlsAllocWrapper>(m_nraWrapper) : NULL;

        VSASSERT( pLimitTemp, "GenerateForLoopStart: limit temporary missing" );
        if (pLiftedLimitTemp!=NULL) GenerateAddr(pLiftedLimitTemp, &addrmode_LiftedLimitTemp);
        // Load limit value onto the stack
        GenerateRvalue(ptreeFor->AsForBlock().Limit);

        RehydrateManagedPointers(NULL);
        if (pLiftedLimitTemp == NULL) GenerateStoreLocal(pLimitTemp->GetLocalSlot());
        else GenerateStore(pLiftedLimitTemp, &addrmode_LiftedLimitTemp);
    }

    // If semantics supplies us with a Temporary to use, then use it regardless of const-ness.
    if ((ptreeFor->AsForBlock().Step->Sxkind() & SXK_CONST) && !pStepTemp)
    {
        // Set flag so we can check for this faster later
        ptreeFor->uFlags |= SBF_FOR_CNS_STEP;
    }
    else
    {
        VSASSERT( pStepTemp, "GenerateForLoopStart: step temporary missing" );
        // Load step value onto the stack and store it
        if (pLiftedStepTemp == NULL)
        {
            GenerateRvalue(ptreeFor->AsForBlock().Step);
            GenerateStoreLocal(pStepTemp->GetLocalSlot());
        }
        else
        {
            BackupValue<Queue<HydrationStep*, NorlsAllocWrapper>*> hydrationContextBackup(&m_currentHydrationContext);
            m_currentHydrationContext = ptreeFor->AsForBlock().Step->ContainsAsyncSpill ? new (*m_pnra) Queue<HydrationStep*, NorlsAllocWrapper>(m_nraWrapper) : NULL;

            ADDRMODE addrmode;
            GenerateAddr(pLiftedStepTemp, &addrmode);
            GenerateRvalue(ptreeFor->AsForBlock().Step);

            RehydrateManagedPointers(NULL);
            GenerateStore(pLiftedStepTemp, &addrmode);
        }
    }

    {
        BackupValue<Queue<HydrationStep*, NorlsAllocWrapper>*> hydrationContextBackup(&m_currentHydrationContext);        
        m_currentHydrationContext = hydrationContextBackupForInitial;

        RehydrateManagedPointers(NULL);
        GenerateStore(ptreeVar, &addrmodeForInitial);
    }
   
    if (ptreeFor->AsForBlock().SignCheckStatement)
    {
        GenerateStatementList(ptreeFor->AsForBlock().SignCheckStatement);
    }

    // Create a code block for the loop body so we can branch back to it.
    ptreeFor->AsForBlock().BodyBlock = NewCodeBlock();

    // Determine if we need to do an initial condition check or not.
    //
    if (!SkipInitialForLoopCheck( ptreeFor ))
    {
        // End code block and jump to condition
        EndCodeBuffer(CEE_BR_S, ptreeFor->AsForBlock().ConditionBlock);   // jump dest
    }

    // Start generating loop body
    SetAsCurrentCodeBlock(ptreeFor->AsForBlock().BodyBlock);
}

//========================================================================
// Determines whether we can skip the initial for loop check
//========================================================================

bool CodeGenerator::SkipInitialForLoopCheck
(
ILTree::PILNode ptreeFor    // SB_FOR
)
{
    bool fConstant = true;

    VSASSERT(ptreeFor->bilop == SB_FOR,
               "SkipInitialForLoopCheck: current must be SB_FOR.");

    // We can only skip the initial for loop check if we can determine
    // at compile time that we will iterate the loop at least once.
    // To do this, the start, end, and step must all be constants and
    // start <= end if step is positive or start >= end if step is
    // negative.
    //

    if ( !(ptreeFor->AsForBlock().Initial->Sxkind() & SXK_CONST) ||
         !(ptreeFor->uFlags & SBF_FOR_CNS_LIMIT) )
    {
        fConstant = false;
    }

    if (!(ptreeFor->uFlags & SBF_FOR_CNS_STEP))
    {
        return false;
    }

    switch (ptreeFor->AsForBlock().ControlVariableReference->AsExpression().vtype)
    {
    case t_ui1:
    case t_ui2:
    case t_ui4:
    case t_ui8:

        VSASSERT(!fConstant ||
                   (ptreeFor->AsForBlock().Step->bilop == SX_CNS_INT &&
                    ptreeFor->AsForBlock().Initial->bilop == SX_CNS_INT &&
                    ptreeFor->AsForBlock().Limit->bilop == SX_CNS_INT),
                   "SkipInitialForLoopCheck: inconsistent loop type and step expr.");

        ptreeFor->uFlags |= SBF_FOR_POSITIVE_STEP;

        if ( !fConstant ||
             ((unsigned __int64)ptreeFor->AsForBlock().Initial->AsIntegralConstantExpression().Value >
                (unsigned __int64)ptreeFor->AsForBlock().Limit->AsIntegralConstantExpression().Value) )
        {
            return false;
        }
        break;

    case t_i1:
    case t_i2:
    case t_i4:
    case t_i8:

        VSASSERT(!fConstant ||
                   (ptreeFor->AsForBlock().Step->bilop == SX_CNS_INT &&
                    ptreeFor->AsForBlock().Initial->bilop == SX_CNS_INT &&
                    ptreeFor->AsForBlock().Limit->bilop == SX_CNS_INT),
                   "SkipInitialForLoopCheck: inconsistent loop type and step expr.");

        if (ptreeFor->AsForBlock().Step->AsIntegralConstantExpression().Value < 0)
        {
            ptreeFor->uFlags |= SBF_FOR_NEGATIVE_STEP;

            if ( !fConstant ||
                 (ptreeFor->AsForBlock().Initial->AsIntegralConstantExpression().Value <
                  ptreeFor->AsForBlock().Limit->AsIntegralConstantExpression().Value) )
            {
                return false;
            }
        }
        else
        {
            // A Zero step is considered Positive.
            ptreeFor->uFlags |= SBF_FOR_POSITIVE_STEP;

            if ( !fConstant ||
                 (ptreeFor->AsForBlock().Initial->AsIntegralConstantExpression().Value >
                  ptreeFor->AsForBlock().Limit->AsIntegralConstantExpression().Value) )
            {
                return false;
            }
        }
        break;

    case t_single:
    case t_double:
        VSASSERT(!fConstant ||
                   ptreeFor->AsForBlock().Step->bilop == SX_CNS_FLT &&
                   ptreeFor->AsForBlock().Initial->bilop == SX_CNS_FLT &&
                   ptreeFor->AsForBlock().Limit->bilop == SX_CNS_FLT,
                   "SkipInitialForLoopCheck: inconsistent loop type and step expr.");

        if (ptreeFor->AsForBlock().Step->AsFloatConstantExpression().Value < 0)
        {
            ptreeFor->uFlags |= SBF_FOR_NEGATIVE_STEP;

            if ( !fConstant ||
                 (ptreeFor->AsForBlock().Initial->AsFloatConstantExpression().Value <
                  ptreeFor->AsForBlock().Limit->AsFloatConstantExpression().Value) )
            {
                return false;
            }
        }
        else
        {
            // A Zero step is considered Positive.
            ptreeFor->uFlags |= SBF_FOR_POSITIVE_STEP;

            if ( !fConstant ||
                 (ptreeFor->AsForBlock().Initial->AsFloatConstantExpression().Value >
                  ptreeFor->AsForBlock().Limit->AsFloatConstantExpression().Value) )
            {
                return false;
            }
        }
        break;

    case t_decimal:
        VSASSERT(!fConstant ||
                   ptreeFor->AsForBlock().Step->bilop == SX_CNS_DEC &&
                   ptreeFor->AsForBlock().Initial->bilop == SX_CNS_DEC &&
                   ptreeFor->AsForBlock().Limit->bilop == SX_CNS_DEC,
                   "SkipInitialForLoopCheck: inconsistent loop type and step expr.");

        if (ptreeFor->AsForBlock().Step->AsDecimalConstantExpression().Value.sign & DECIMAL_NEG)
        {
            ptreeFor->uFlags |= SBF_FOR_NEGATIVE_STEP;

            if ( !fConstant ||
                 (VarDecCmp(&ptreeFor->AsForBlock().Initial->AsDecimalConstantExpression().Value,
                            &ptreeFor->AsForBlock().Limit->AsDecimalConstantExpression().Value) == (HRESULT)VARCMP_LT ))
            {
                return false;
            }
        }
        else
        {
            // A Zero step is considered Positive.
            ptreeFor->uFlags |= SBF_FOR_POSITIVE_STEP;

            if ( !fConstant ||
                 (VarDecCmp(&ptreeFor->AsForBlock().Initial->AsDecimalConstantExpression().Value,
                            &ptreeFor->AsForBlock().Limit->AsDecimalConstantExpression().Value) == (HRESULT)VARCMP_GT ))
            {
                return false;
            }
        }
        break;

    default:
        VSFAIL("SkipInitialForLoopCheck: unexpected type");
        VbThrow(HrMake(ERRID_CodegenError));
        break;
    }

    return true;
}

//========================================================================
// Generate the end of an intrinsic for loop
//========================================================================

void CodeGenerator::GenerateForLoopEnd
(
ILTree::PILNode  ptreeFor
)
{
    VSASSERT( ptreeFor->bilop == SB_FOR,
               "GenerateForLoopEnd: stmt must be SB_FOR." );

    VSASSERT( ptreeFor->AsForBlock().ControlVariableReference != NULL &&
              ptreeFor->AsForBlock().Step != NULL &&
              ptreeFor->AsForBlock().Limit != NULL,
              "GenerateForLoopEnd: For tree needs control expressions" );

    ILTree::PILNode  ptreeVar  = ptreeFor->AsForBlock().ControlVariableReference;
    ILTree::PILNode  ptreeStep = ptreeFor->AsForBlock().Step;
    ILTree::PILNode  ptreeEnd  = ptreeFor->AsForBlock().Limit;
    Vtypes    vtypeLoop = ptreeFor->AsForBlock().ControlVariableReference->AsExpression().vtype;


    // Generate the increment expression.  adds ptreeStep to ptreeVar.
    //
    ADDRMODE addrmode;

    if (ptreeFor->AsForBlock().IncrementStatement)
    {
        GenerateStatementList(ptreeFor->AsForBlock().IncrementStatement);
    }
    else
    {
        GenerateAddr(ptreeVar, &addrmode);
        GenerateDupLvalue(ptreeVar, &addrmode);
        GenerateLoad(ptreeVar, &addrmode);

        // If the step isn't constant, load up the local var
        if (ptreeFor->uFlags & SBF_FOR_CNS_STEP)
        {
            GenerateRvalue(ptreeStep);
        }
        else
        {
            GenerateLoadOrElseLoadLocal(ptreeFor->AsForBlock().LiftedStepTemporary, ptreeFor->AsForBlock().StepTemporary);
        }

        // can't use the ADD opcode if we have a decimal type
        if (vtypeLoop == t_decimal)
        {
            GenerateCallToRuntimeHelper(DecimalAddMember, GetSymbolForVtype(t_decimal), &ptreeVar->Loc); // for d as decimal = 1 to 10 -> Microsoft.VisualBasic.CompilerServices.ObjectFlowControl.ForLoopControl.ForNextCheckDec
        }
        else
        {
            GenerateMathOp(SX_ADD, ptreeVar->AsExpression().vtype, ptreeVar);
        }

        GenerateStore(ptreeVar, &addrmode);
    }

    // Generate the condition check ...
    {
        BILOP  bilop;

        VSASSERT(ptreeFor->AsForBlock().ConditionBlock,
                   "GenerateForLoopEnd: must have condition block.");

        // End code block and set condition block to be current.
        SetAsCurrentCodeBlock(ptreeFor->AsForBlock().ConditionBlock);
        StartHiddenIL();

        if (IsIntegralType(vtypeLoop) || vtypeLoop == t_char || vtypeLoop == t_bool)
        {
            if (ptreeFor->uFlags & SBF_FOR_CNS_STEP)
            {
                if (ptreeFor->uFlags & SBF_FOR_POSITIVE_STEP)
                {
                    bilop = SX_LE;
                }
                else
                {
                    VSASSERT(ptreeFor->uFlags & SBF_FOR_NEGATIVE_STEP,
                               "GenerateForLoopEnd: constant step must be -ve or +ve.");

                    bilop = SX_GE;
                }

                GenerateRvalue(ptreeVar);

                if (ptreeFor->uFlags & SBF_FOR_CNS_LIMIT)
                {
                    GenerateRvalue(ptreeEnd);
                }
                else
                {
                    GenerateLoadOrElseLoadLocal(ptreeFor->AsForBlock().LiftedLimitTemporary, ptreeFor->AsForBlock().LimitTemporary);
                }
            }
            // Unknown integral step
            else
            {
                int      nShiftSize;

                // 
                if (ptreeVar->AsExpression().vtype == t_ui1)
                {
                    GenerateRvalue(ptreeVar);
                }
                else
                {
                    // If this is an integer type For loop, we can generate code to
                    // check the condition instead of using a helper.
                    //
                    // Generate (_tstep >> (sizeof(_tstep)*8-1)) ^ ptreeVar <=
                    //                       (_tstep >> (sizeof(sStep)*8-1)) ^ _tlimit
                    //

                    // figure out how many bits to shift the step to reduce it to -1 or 0:
                    nShiftSize = CbOfVtype(vtypeLoop)*8 - 1;

                    // Generate the sign: _tstep >> nShiftSize
                    if (ptreeFor->uFlags & SBF_FOR_CNS_STEP)
                    {
                        GenerateRvalue(ptreeStep);
                    }
                    else
                    {
                        GenerateLoadOrElseLoadLocal(ptreeFor->AsForBlock().LiftedStepTemporary, ptreeFor->AsForBlock().StepTemporary);
                    }

                    GenerateLiteralInt(nShiftSize);

                    EmitOpcode(CEE_SHR, ptreeVar->AsExpression().ResultType);
                    GenerateRvalue(ptreeVar);
                    EmitOpcode(CEE_XOR, ptreeVar->AsExpression().ResultType);
                }

                // 
                if (ptreeEnd->AsExpression().vtype == t_ui1)
                {
                    if (ptreeFor->uFlags & SBF_FOR_CNS_LIMIT)
                    {
                        GenerateRvalue(ptreeEnd);
                    }
                    else
                    {
                        GenerateLoadOrElseLoadLocal(ptreeFor->AsForBlock().LiftedLimitTemporary, ptreeFor->AsForBlock().LimitTemporary);
                    }
                }
                else
                {
                    // Now generate the sign value again
                    if (ptreeFor->uFlags & SBF_FOR_CNS_STEP)
                    {
                        GenerateRvalue(ptreeStep);
                    }
                    else
                    {
                        GenerateLoadOrElseLoadLocal(ptreeFor->AsForBlock().LiftedStepTemporary, ptreeFor->AsForBlock().StepTemporary);
                    }

#pragma warning (push)
#pragma warning (disable:6001) // False warning as the code path above will initialize nShiftSize
                    GenerateLiteralInt(nShiftSize);
#pragma warning (pop)

                    EmitOpcode(CEE_SHR, ptreeEnd->AsExpression().ResultType);

                    // Do the second xor
                    if (ptreeFor->uFlags & SBF_FOR_CNS_LIMIT)
                    {
                        GenerateRvalue(ptreeEnd);
                    }
                    else
                    {
                        GenerateLoadOrElseLoadLocal(ptreeFor->AsForBlock().LiftedLimitTemporary, ptreeFor->AsForBlock().LimitTemporary);
                    }
                    EmitOpcode(CEE_XOR, ptreeEnd->AsExpression().ResultType);
                }

                bilop = SX_LE;
            }
        }
        else if (vtypeLoop == t_single || vtypeLoop == t_double)
        {
            GenerateRvalue(ptreeVar);

            if (ptreeFor->uFlags & SBF_FOR_CNS_LIMIT)
            {
                GenerateRvalue(ptreeEnd);
            }
            else 
            {
                GenerateLoadOrElseLoadLocal(ptreeFor->AsForBlock().LiftedLimitTemporary, ptreeFor->AsForBlock().LimitTemporary);
            }

            if (ptreeFor->uFlags & SBF_FOR_CNS_STEP)
            {
                if (ptreeFor->uFlags & SBF_FOR_POSITIVE_STEP)
                {
                    bilop = SX_LE;
                }
                else
                {
                    VSASSERT(ptreeFor->uFlags & SBF_FOR_NEGATIVE_STEP,
                             "GenerateForLoopEnd: constant step must be -ve or +ve. : #10/30/2003#");
                    bilop = SX_GE;
                }
            }
            else
            {
                GenerateLoadOrElseLoadLocal(ptreeFor->AsForBlock().LiftedStepTemporary, ptreeFor->AsForBlock().StepTemporary);
                GenerateCallToRuntimeHelper(vtypeLoop == t_single ? SingleForCheckMember : DoubleForCheckMember,
                                            GetSymbolForVtype(t_bool),
                                            &ptreeFor->Loc);

                // these helpers will leave t_bool on stack to indicate whether
                // to continue or not. EQ will not turn into anything.
                //
                bilop = SX_EQ;
            }
        }

        // non-integral step
        else if (vtypeLoop == t_decimal)
        {
            GenerateRvalue(ptreeVar);

            if (ptreeFor->uFlags & SBF_FOR_CNS_LIMIT)
            {
                GenerateRvalue(ptreeEnd);
            }
            else
            {
                GenerateLoadOrElseLoadLocal(ptreeFor->AsForBlock().LiftedLimitTemporary, ptreeFor->AsForBlock().LimitTemporary);
            }

            if (ptreeFor->uFlags & SBF_FOR_CNS_STEP)
            {
                GenerateRvalue(ptreeStep);
            }
            else
            {
                GenerateLoadOrElseLoadLocal(ptreeFor->AsForBlock().LiftedStepTemporary, ptreeFor->AsForBlock().StepTemporary);
            }

            GenerateCallToRuntimeHelper(DecimalForCheckMember, GetSymbolForVtype(t_bool), &ptreeFor->Loc);

            // these helpers will leave t_bool on stack to indicate whether
            // to continue or not. EQ will not turn into anything.
            //
            bilop = SX_EQ;
        }
        else
        {
            VSASSERT(ptreeFor->AsForBlock().Condition, "how did we get here with a null condition?");
            VSASSERT(ptreeFor->AsForBlock().Condition->ResultType->GetVtype() == t_bool, "how did we get here with a non-bool condition?");

            GenerateRvalue(ptreeFor->AsForBlock().Condition);
            bilop = SX_EQ;
        }

        // ENC requires a remappable point before each conditional branch (VSW#205259).
        InsertENCRemappablePoint(
            m_Project->GetCompilerHost()->GetFXSymbolProvider()->GetType(
                (bilop == SX_LE || bilop == SX_GE) ? vtypeLoop : t_bool));

        if (bilop == SX_LE)
        {
            // 
            EndCodeBuffer(
                IsUnsignedType(vtypeLoop) ?
                    CEE_BLE_UN_S :
                    CEE_BLE_S,
                ptreeFor->AsForBlock().BodyBlock,
                vtypeLoop);
        }
        else if (bilop == SX_GE)
        {
            EndCodeBuffer(
                IsUnsignedType(vtypeLoop) ?
                    CEE_BGE_UN_S :
                    CEE_BGE_S,
                ptreeFor->AsForBlock().BodyBlock,
                vtypeLoop);
        }
        else
        {
            EndCodeBuffer(CEE_BRTRUE_S, ptreeFor->AsForBlock().BodyBlock);
        }

        SetAsCurrentCodeBlock(ptreeFor->AsForBlock().EndBlock);

    } // condition expr
}

//========================================================================
//  Generate the initialization code for a variant for loop.
//
//  We will generate the following:
//
//             create radVariant[3] arrVar;
//             ForLoopInit(arrVar, step, limit, counter, start);
//             ptreeVar = arrVar[0]
//             if !continue goto exit
//
//      body:
//             loop body
//
//      cond:
//             ForLoopNext(arrVar);
//             ptreeVar = arrVar[0];
//             if (continue) goto body
//
//      exit:
//========================================================================

void CodeGenerator::GenerateObjectForLoopStart
(
ILTree::PILNode ptreeFor  // SB_FOR
)
{
    ADDRMODE addrmode;

    VSASSERT(ptreeFor->bilop == SB_FOR,
               "GenerateObjectForLoopStart: current stmt must be SB_FOR.");

    VSASSERT(TypeHelpers::IsRootObjectType(ptreeFor->AsForBlock().ControlVariableReference->AsExpression().ResultType),
             "GenerateObjectForLoopStart: loop type must be object.");

    // Create forward labels for the condition and for the blockend
    ptreeFor->AsForBlock().ConditionBlock = NewCodeBlock(); // condition
    ptreeFor->AsForBlock().EndBlock = NewCodeBlock();  // end

    // Do the loop initialization ...
    //

    // Generate call to ForLoopInit
    //
    GenerateRvalue(ptreeFor->AsForBlock().ControlVariableReference);
    GenerateRvalue(ptreeFor->AsForBlock().Initial);
    GenerateRvalue(ptreeFor->AsForBlock().Limit);
    GenerateRvalue(ptreeFor->AsForBlock().Step);
    if (ptreeFor->AsForBlock().LiftedEnumeratorTemporary != NULL)
    {
        GenerateAddr(ptreeFor->AsForBlock().LiftedEnumeratorTemporary, &addrmode);
        GenerateLoadAddr(ptreeFor->AsForBlock().LiftedEnumeratorTemporary, &addrmode);
    }
    else
    {
        GenerateLoadLocalAddr(ptreeFor->AsForBlock().EnumeratorTemporary->GetLocalSlot());
    }
    // load up the counter as byref
    GenerateAddr(ptreeFor->AsForBlock().ControlVariableReference, &addrmode);
    GenerateLoadAddr(ptreeFor->AsForBlock().ControlVariableReference, &addrmode);
    
    if (m_Project != NULL && m_Project->GetVBRuntimeKind() == EmbeddedRuntime)
    {
        GenerateCallToRuntimeHelper(LateBinderObjectForInitMember, GetSymbolForVtype(t_bool), &ptreeFor->Loc);
    }  
    else
    {
        GenerateCallToRuntimeHelper(ObjectForInitMember, GetSymbolForVtype(t_bool), &ptreeFor->Loc);
    }  
    // Create a code block for the loop body so we can branch back to it.
    ptreeFor->AsForBlock().BodyBlock = NewCodeBlock();

    // ENC requires a remappable point before each conditional branch (VSW#205259).
    InsertENCRemappablePoint(m_Project->GetCompilerHost()->GetFXSymbolProvider()->GetType(t_bool));

    // The runtime call ObjectForInitMember will leave a bool on the stack
    // indicating whether to execute loop or not. Emit code to do this check.
    // True means continue the loop, False means exit the loop
    //
    EndCodeBuffer(CEE_BRFALSE_S, ptreeFor->AsForBlock().EndBlock);
    SetAsCurrentCodeBlock(ptreeFor->AsForBlock().BodyBlock);
}

//========================================================================
// Generate the end of a variant for loop
//========================================================================

void CodeGenerator::GenerateObjectForLoopEnd
(
ILTree::PILNode ptreeFor
)
{
    VSASSERT( ptreeFor->bilop == SB_FOR,
               "GenerateObjectForLoopEnd: must have SB_FOR node" );
    VSASSERT( ptreeFor->AsForBlock().ControlVariableReference != NULL,
               "GenerateObjectForLoopEnd: for stmt needs variable expr" );

    ADDRMODE addrmode;

    // Set the condition block as current and generate the condition code.
    //
    SetAsCurrentCodeBlock(ptreeFor->AsForBlock().ConditionBlock);

    // Generate call to ForLoopNext();
    GenerateRvalue(ptreeFor->AsForBlock().ControlVariableReference);
    GenerateLoadOrElseLoadLocal(ptreeFor->AsForBlock().LiftedEnumeratorTemporary, ptreeFor->AsForBlock().EnumeratorTemporary);
    // load the counter as byref
    GenerateAddr(ptreeFor->AsForBlock().ControlVariableReference, &addrmode);
    GenerateLoadAddr(ptreeFor->AsForBlock().ControlVariableReference, &addrmode);
	
    if (m_Project != NULL && m_Project->GetVBRuntimeKind() == EmbeddedRuntime)
    {
        GenerateCallToRuntimeHelper(LateBinderObjectForNextMember, GetSymbolForVtype(t_bool), &ptreeFor->Loc);
    }
    else 
    {
        GenerateCallToRuntimeHelper(ObjectForNextMember, GetSymbolForVtype(t_bool), &ptreeFor->Loc);
    }
    
    // ENC requires a remappable point before each conditional branch (VSW#205259).
    InsertENCRemappablePoint(m_Project->GetCompilerHost()->GetFXSymbolProvider()->GetType(t_bool));

    // The runtime call ObjectForNextMember will leave a bool on the stack
    // indicating whether to execute loop or not. Emit code to do this check.
    // True means continue the loop, False means exit the loop
    //
    EndCodeBuffer(CEE_BRTRUE_S, ptreeFor->AsForBlock().BodyBlock);
    SetAsCurrentCodeBlock(ptreeFor->AsForBlock().EndBlock);
}

//========================================================================
// Generate a for loop.
//========================================================================

void CodeGenerator::GenerateForLoop
(
ILTree::PILNode ptreeFor
)
{
    VSASSERT( ptreeFor->bilop == SB_FOR,
               "GenerateForLoop: must have SB_FOR node" );

    VSASSERT( ptreeFor->AsForBlock().ControlVariableReference != NULL,
               "GenerateForLoop: for stmt needs variable expr" );
    VSASSERT( ptreeFor->AsForBlock().Step != NULL,
               "GenerateForLoop: for stmt needs step expr" );
    VSASSERT( ptreeFor->AsForBlock().Initial != NULL,
               "GenerateForLoop: for stmt needs initial expr" );
    VSASSERT( ptreeFor->AsForBlock().Limit != NULL,
               "GenerateForLoop: for stmt needs end expr" );

// verify our types are correct
#if DEBUG
    {
        Vtypes VarType = ptreeFor->AsForBlock().ControlVariableReference->AsExpression().vtype;
        Vtypes IncType = ptreeFor->AsForBlock().Step->AsExpression().vtype;
        Vtypes BegType = ptreeFor->AsForBlock().Initial->AsExpression().vtype;
        Vtypes EndType = ptreeFor->AsForBlock().Limit->AsExpression().vtype;

        VSASSERT(VarType == IncType && IncType == BegType && BegType == EndType,
                 "GenerateForLoop:  all tree types should match" );

        VSASSERT(IsNumericType(VarType) || VarType == t_ref || VarType == t_struct, "GenerateForLoop:  invalid For type");
    }
#endif

    // save the For Loop "Next" Block in the tree for the Continue Statement
    ptreeFor->AsForBlock().ContinueBlock = NewCodeBlock();

    UpdateLineTable(ptreeFor);
    UpdateResumeTable(ptreeFor->AsForBlock().ResumeIndex);

    if (TypeHelpers::IsRootObjectType(ptreeFor->AsForBlock().ControlVariableReference->ResultType))
    {
        GenerateObjectForLoopStart(ptreeFor);
    }
    else
    {
        GenerateForLoopStart(ptreeFor);
    }

    // Generate statements in block
    GenerateBlockStatementList( ptreeFor->AsExecutableBlock() );

    SetAsCurrentCodeBlock(ptreeFor->AsForBlock().ContinueBlock);

    // Generate a NOP so that we can set a breakpoint on the Next statement
    UpdateLineTable(ptreeFor->AsExecutableBlock().EndConstructLocation);
    UpdateResumeTable(ptreeFor->AsForBlock().TerminatingResumeIndex);
    InsertNOP();

    // Generate the increment expression and go back to condition check
    //
    if (TypeHelpers::IsRootObjectType(ptreeFor->AsForBlock().ControlVariableReference->ResultType))
    {
        GenerateObjectForLoopEnd(ptreeFor);
    }
    else
    {
        GenerateForLoopEnd(ptreeFor);
    }
}

//========================================================================
// Generate a redim expression
//========================================================================

void CodeGenerator::GenerateRedim
(
ILTree::PILNode ptree
)
{
    signed      cDims = ptree->AsRedimStatement().uDims;
    ILTree::PILNode    ptreeArray;
    ADDRMODE    addrmode;
    bool        fPreserve = ((ptree->uFlags & SLF_REDIM_PRESERVE) == SLF_REDIM_PRESERVE);
    BCSYM_ArrayType * ptypArr;


    VSASSERT(cDims, "GenerateRedim: must have dimensions.");

    ptreeArray = ptree->AsRedimStatement().ptreeOp1;

    VSASSERT(ptreeArray->bilop == SX_SYM,
               "GenerateRedim: malformed redim tree.");

    VSASSERT(ptree->AsRedimStatement().vtype == t_void, "how can this be?");

    // We want to know if ContainsAsyncSpill is true for any of the size operands. But while MarkAncestors() normally marks
    // this flag recursively up the expression tree, it only does it for "real" parent expression nodes -- not for sequences
    // such as our Operand2, and not for statements such as ptree itself. We don't want to bog down the MarkAncestors() code
    // with the work to mark it in a sequence, since it's needed for Redim and nothing else, which is why we've implemented it right here:
    bool operand2ContainsAsyncSpill = false;
    ILTree::Expression *pExpr = ptree->AsRedimStatement().Operand2;
    while (pExpr != NULL && pExpr->bilop == SX_LIST && !operand2ContainsAsyncSpill)
    {
        operand2ContainsAsyncSpill |= pExpr->AsExpressionWithChildren().Left->ContainsAsyncSpill;
        pExpr = pExpr->AsExpressionWithChildren().Right;
    }
    BackupValue<Queue<HydrationStep*, NorlsAllocWrapper>*> hydrationContextBackup(&m_currentHydrationContext);
    m_currentHydrationContext = operand2ContainsAsyncSpill ? new (*m_pnra) Queue<HydrationStep*, NorlsAllocWrapper>(m_nraWrapper) : NULL;

    GenerateAddr(ptreeArray, &addrmode);

    if (fPreserve)
    {
        // Go ahead and generate the source reference here so we can
        // call the copy array function down the line
        GenerateRvalue(ptreeArray);

        // 

        GenerateCastClass(
            m_Project->GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::ArrayType),
            &ptreeArray->Loc);
    }

    ptypArr = ptreeArray->AsSymbolReferenceExpression().ResultType->PArrayType();
    GenerateArrayConstruction(ptypArr, ptree->AsRedimStatement().Operand2, &ptreeArray->Loc);

    RehydrateManagedPointers(&ptree->Loc);

    if (fPreserve)
    {
        // Call runtime function to do copy
        GenerateCallToRuntimeHelper(CopyArrayMember, GetSymbolForFXType(FX::ArrayType), &ptree->Loc); // This affects the Preserve keyword.  We don't have location info for 'Preserve' so squiggle the whole thing.

        // Now cast to the actual type
        // If we have a byref array, then go through that pointer
        BCSYM * CompilerType = ptreeArray->AsSymbolReferenceExpression().pnamed->DigThroughAlias()->PMember()->GetCompilerType();

        if (CompilerType->IsPointerType())
        {
            VSASSERT( addrmode.addrkind == ADDR_Byref,
                       "GenerateRedim: have pointer type but no in Byref addressing mode!" );
            GenerateCastClass(CompilerType->PPointerType()->GetRoot(), &ptreeArray->Loc);
        }
        else
        {
            // Use the result type rather than the CompilerType because if pnamed is a member
            // of a generic type and refers to type parameters, then the CompilerType will be the
            // substituted type.
            //
            GenerateCastClass(ptreeArray->AsSymbolReferenceExpression().ResultType, &ptreeArray->Loc);
        }
    }
        
    GenerateStore(ptreeArray, &addrmode);
}

void CodeGenerator::GenerateRTSetErrorMember
(
    Location *pLoc, // location information for the language construct that us to generate the helper call
    MISSING_HELPER_ERROR_KIND errorKind // whether to log missing helpers as an error or warning.  In Catch statement, for instance, we want warnings since you may not be using the Err object in your app
)
{
    VSASSERT( m_ptreeFunc->AsProcedureBlock().fSeenOnErr || m_ptreeFunc->AsProcedureBlock().fSeenCatch, "Why are we setting the Err object if we don't need to?");

    if (NeedLineNumber())
    {
        VSASSERT(m_usCurLineLocal != INVALID_LCL_INDEX, "Line number local not found");

        GenerateLoadLocal(m_usCurLineLocal, GetSymbolForVtype(t_i4));
        GenerateCallToRuntimeHelper(SetErrorMemberWithLineNum, Symbols::GetVoidType(), pLoc, errorKind);
    }
    else
    {
        GenerateCallToRuntimeHelper(SetErrorMember, Symbols::GetVoidType(), pLoc, errorKind);
    }
}
