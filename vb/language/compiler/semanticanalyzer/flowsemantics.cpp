//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implementation of VB data and control flow semantics.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

// DefAsgBlockVisitor: used to initialize a tree prior to flow-analyzing it (setting up slots, and in/out bitsets)
//
// !!! DANGER !!! If a method has lambdas in it, then flow analysis will be performed on it
// twice -- once in the context of the method, and once just for the lambda on its own.
// We have to make sure to reset all the "outDegAsgBitsets" that we happen to store on
// blocks and statements. The current list is Block, TryBlock, GeneralLoobBlock,
// ExitStatement, ReturnStatement, LabelStatement, GotoStatement. If anyone adds
// BITSET to other nodes, then they have to update the DefAsgBlockVisitor as well.
// See also Dev10#562653.
//
class DefAsgBlockVisitor : public BoundTreeVisitor 
{
public:
    Semantics *m_Semantics;
    bool m_IncludeLocals;
    bool m_IncludeReturns;

    DefAsgBlockVisitor(Semantics* semantics) :
       m_Semantics(semantics)
    {
    }

    void Visit(ILTree::ProcedureBlock *body, bool IncludeLocals, bool IncludeReturns)
    {
        m_IncludeLocals = IncludeLocals;
        m_IncludeReturns = IncludeReturns;
        BoundTreeVisitor::Visit(body);
    }

    virtual bool StartBlock(ILTree::ExecutableBlock *block)
    {
        m_Semantics->DefAsgProcessSlots(block, m_IncludeLocals, m_IncludeReturns);
        block->inDefAsgBitset = NULL;
        block->outDefAsgBitset = NULL;
        switch (block->bilop)
        {
            case SB_TRY:
                block->AsTryBlock().outDefAsgBitsetTryGroup = NULL;
                break;
            case SB_FOR: case SB_FOR_EACH: case SB_DO: case SB_LOOP:
                block->AsGeneralLoopBlock().outDefAsgBitsetLocal = NULL;
        }
        return true;
    }

    virtual bool StartStatement(ILTree::Statement *statement)
    {
        if (IsBad(statement))
        {
            return false;
        }

        switch (statement->bilop)
        {
            case SL_EXIT:
                statement->AsExitStatement().outDefAsgBitset = NULL;
                break;
            case SL_RETURN:
                statement->AsReturnStatement().outDefAsgBitset = NULL;
                break;
            case SL_LABEL:
                statement->AsLabelStatement().inDefAsgBitset = NULL;
                break;
            case SL_GOTO:
                statement->AsGotoStatement().outDefAsgBitset = NULL;
                break;
        }
        return true;
    }

};

// DefAsgUnusedVisitor: used to iterate executable blocks and multiline lambdas, and check whether they were unused:
class DefAsgUnusedVisitor : public BoundTreeVisitor 
{
public:
    Semantics *m_Semantics;
    DefAsgUnusedVisitor(Semantics* semantics) : m_Semantics(semantics) {}
    virtual bool StartBlock(ILTree::ExecutableBlock *block)
    {
        m_Semantics->CheckUnusedLocals(block);
        return true;
    }
};

bool IsWinRTEventAddHandler(BCSYM_Proc *pProcedure, FX::FXSymbolProvider *pSymbols)
{
    return pProcedure->IsEventAccessor() && 
           pSymbols->IsTypeAvailable(FX::EventRegistrationTokenType) &&
           BCSYM::AreTypesEqual(pProcedure->GetType(), pSymbols->GetType(FX::EventRegistrationTokenType));

    // 




}



void Semantics::CheckFlow(ILTree::ProcedureBlock *BoundBody, bool OnlyCheckReturns)
{
    // flow analysis cannot run in the following conditions
    if ( IsBad(BoundBody) ||                    // errors in the bound tree
         m_IsGeneratingXML ||                   // uncomplete bound tree for XML generaton
         !m_Procedure ||                        // guard for the next checks, m_Procedure can be null.
         m_Procedure->IsMyGenerated() ||        // synthetic properties generated for My* collections
         m_Procedure->GetPWellKnownAttrVals()->GetDllImportData() ||    //DLL imports
         m_Procedure->IsMustOverrideKeywordUsed()) // must override (abstract) methods
    {
        return;
    }

    DBG_SWITCH_PRINTF(fDumpFlow, L"\nCheckFlow:Slots: %s.%s\n", m_Procedure->GetPhysicalContainer()->GetName(), m_Procedure->GetName());


    //clean static locals isUsed flag. Needed to make up for decompilation
    VSASSERT(ContainingClass(), "why no containing class for this procedure");

    VSASSERT(!OnlyCheckReturns || m_ProcedureTree->Child->bilop == SB_STATEMENT_LAMBDA, "unexpected: the only flow analysis we should be doing on lifted closures is for their return statements");

    Container *PhysicalContainer = m_Procedure->GetPhysicalContainer();
    AssertIfFalse(PhysicalContainer && PhysicalContainer->IsClass());
    //
    if (PhysicalContainer && PhysicalContainer->IsClass() && PhysicalContainer->PClass()->GetBackingFieldsForStatics())
    {
        BCITER_HASH_CHILD BackingFields(PhysicalContainer->PClass()->GetBackingFieldsForStatics());

        for (Declaration *BackingField = BackingFields.GetNext();
            BackingField;
            BackingField = BackingFields.GetNext())
        {
            if (BackingField->IsStaticLocalBackingField() &&
                !BackingField->PStaticLocalBackingField()->IsInitFlag() &&
                BackingField->PStaticLocalBackingField()->GetProcDefiningStatic() == m_Procedure &&
                BackingField->IsVariable())
            {
                BackingField->PVariable()->CleanIsUsed();
            }
        }
    }
    m_DefAsgOnErrorSeen = false;

    // By default, we'll always check that functions have return statements.
    // But in state-machine methods (Async/Iterator), functions don't always need return statements.
    bool CheckForReturns = true;
    switch (BoundBody->m_ResumableKind)
    {
        case ILTree::SubResumable:
        case ILTree::IterableResumable:
        case ILTree::IteratorResumable:
            CheckForReturns = false; // none of these things need return statements
            break;
        case ILTree::UnknownResumable:
        case ILTree::NotResumable:
            CheckForReturns = true; // These potentially do, based on the return type
            break;
        case ILTree::TaskResumable:
        {
            Type *retType = BoundBody->m_ResumableGenericType;
            CheckForReturns = (retType != NULL && !retType->IsVoidType());
            break;
        }
    }

    // START DFA
    // Assign slots to local variables, including local variables in lambdas
#if DEBUG
    m_DefAsgSlotAssignments.Reset();
#endif
	VSASSERT(m_DefAsgCount==0, "unexpected: m_DefAstCount should have been zero. If it's not, there might be slotted variables lying around somewhere...");
	m_DefAsgCount=0;
    DefAsgBlockVisitor blockVisitor(this);
    blockVisitor.Visit(BoundBody, !OnlyCheckReturns/*slots for locals?*/, CheckForReturns/*slots for returns?*/);
    // this calls DefAsgProcessSlots(block) for each executable block in BoundBody, which
    // sets up slots in the m_DefAsgCount array and sets pvar->DefAsgSlot;
    // It also sets inDefAsgBitset and outDefAsgBitset to NULL for all blocks, in case the body had been
    // examined before.

    m_DefAsgErrorDisplayedBitset = BITSET::create(m_DefAsgCount, &m_DefAsgAllocator, 0);
    m_DefAsgCurrentBitset = BITSET::create(m_DefAsgCount, &m_DefAsgAllocator, 0);
    BoundBody->inDefAsgBitset = BITSET::create(m_DefAsgCount, &m_DefAsgAllocator, 0);
    m_DefAsgTempBitset = BITSET::create(m_DefAsgCount, &m_DefAsgAllocator, 0);
    m_DefAsgIsReachableCode = true;


    // Flow analysis: initial run
    DBG_SWITCH_PRINTF(fDumpFlow, L"CheckFlow:Eval: %s.%s  {CURBITSET=%08x}\n", PhysicalContainer->GetName(), m_Procedure->GetName(), m_DefAsgCurrentBitset);
    DefAsgEvalBlock(BoundBody,0);

    // Flow analysis: if there had been cycles in the flow graph, then we keep evaluating them until
    // we reach a fixed point
    ILTree::PILNode currentReEvalStmt;
    while (m_DefAsgReEvalStmtList.NumberOfEntries()>0)
    {
        DefAsgReEvalStmt* top = m_DefAsgReEvalStmtList.GetFirst();
        currentReEvalStmt = top->m_ReEvalStmt;
        unsigned int ClosureDepth = top->ClosureDepth;
        m_DefAsgReEvalStmtList.Remove(top);
        if (currentReEvalStmt->bilop == SL_LABEL)
        {
            // the re eval point is a label. There is a current bitset and it is reachable
            ILTree::LabelStatement *label = &currentReEvalStmt->AsLabelStatement();
            DBG_SWITCH_PRINTF(fDumpFlow, L"--re-eval %S  {inBITSET=%08x}\n", ILTree::BilopName(label->bilop), label->inDefAsgBitset);
            m_DefAsgCurrentBitset = m_DefAsgCurrentBitset->setInto(label->inDefAsgBitset, &m_DefAsgAllocator);
            m_DefAsgIsReachableCode = true;
        }
        else if  (currentReEvalStmt->IsExecutableBlockNode())
        {
            // an executable block in the re-eval list can be introduced by 'exit/break'. The sequence following the
            // block is re evaluated based on the out-bitset of the block
            ILTree::ExecutableBlock* block = &currentReEvalStmt->AsExecutableBlock();
            DBG_SWITCH_PRINTF(fDumpFlow, L"--re-eval %S  {outBITSET=%08x}\n", ILTree::BilopName(block->bilop), block->outDefAsgBitset);

            if (block->bilop == SB_TRY)
            {
                // 'try,catch,finally' nodes are considered as a group statement
                if (block->AsTryBlock().outDefAsgBitsetTryGroup)
                {
                    m_DefAsgCurrentBitset = m_DefAsgCurrentBitset->setInto(block->AsTryBlock().outDefAsgBitsetTryGroup);
                    m_DefAsgIsReachableCode = true;

                    // move to the end of the group
                    while(block->Next != NULL &&
                          (block->Next->bilop == SB_CATCH || block->Next->bilop == SB_FINALLY))
                    {
                        block = &block->Next->AsExecutableBlock();
                    }
                    if (block->Next != NULL)
                    {
                        currentReEvalStmt = block->Next;
                    }
                    else
                    {
                        // the group is the last stmt in the enclosing block
                        currentReEvalStmt = DefAsgUpdateParentGetContinuation(currentReEvalStmt, ClosureDepth);
                    }
                }
                else
                {
                    VSFAIL("Definite assignment: how a block re eval without out-bitset?");
                    currentReEvalStmt = NULL;
                }

            }
            else
            {
                //other block than try or catch
                if (block->outDefAsgBitset)
                {
                    m_DefAsgCurrentBitset = m_DefAsgCurrentBitset->setInto(block->outDefAsgBitset);
                    m_DefAsgIsReachableCode = true;
                    if (block->Next  != NULL)
                    {
                        currentReEvalStmt = block->Next;
                    }
                    else
                    {
                        currentReEvalStmt = DefAsgUpdateParentGetContinuation(currentReEvalStmt, ClosureDepth);
                    }
                }
                else
                {

                    VSFAIL("Definite assignment: how a block re eval without out-bitset?");
                    currentReEvalStmt = NULL;
                }
            }
        }
        else
        {
            // land nicely for now, to change to asserts lately.
            VSFAIL("Definite assignment: why were we asked to re-evaluate a non-label non-block?");
            currentReEvalStmt = NULL;
        }


        // re eval the statement list started by currentReEvalStmt,
        // then walk up the block structure for its continuation and re eval it
        // THe process stops when the code become unreachable, or the chages introduced by re eval become local
        //
        while (currentReEvalStmt != NULL)
        {
            DefAsgEvalStatementList(currentReEvalStmt, ClosureDepth);
            currentReEvalStmt = DefAsgUpdateParentGetContinuation(currentReEvalStmt, ClosureDepth);
        }

        DBG_SWITCH_PRINTF(fDumpFlow, L"  finished re-eval.  {CURBITSET=%08x}\n",m_DefAsgCurrentBitset);

    } // next re eval stmt list

    //check for functions without return value
    if ( !m_DefAsgOnErrorSeen )
    {
        DBG_SWITCH_PRINTF(fDumpFlow, L"CheckFlow:MissingReturns?: %s.%s\n", PhysicalContainer->GetName(), m_Procedure->GetName());
        BCSYM_Variable * pRetVar = m_ProcedureTree->AsProcedureBlock().ReturnVariable;
        if (pRetVar &&
            pRetVar->GetDefAsgSlot() &&
            !m_ProcedureTree->AsProcedureBlock().fDefAsgNoRetValReported)
        {

            if (m_ProcedureTree->outDefAsgBitset &&
                !m_ProcedureTree->outDefAsgBitset->testBits(pRetVar->GetDefAsgSlot(), pRetVar->GetDefAsgSize()))
            {
                DBG_SWITCH_PRINTF(fDumpFlow,L"  *MISSING RETURN STATEMENT\n");
                m_ProcedureTree->AsProcedureBlock().fDefAsgNoRetValReported = true;

                // synthesize the ending location for the method, so the error is shown to "End Function"
                // The check IsMethodImpl() is an extra precaution. If it is not an method implementation
                // the use the location of the function name as a fall back.
                Location loc;
                if (m_Procedure->IsMethodImpl())
                {
                    loc.m_lBegLine = m_Procedure->PMethodImpl()->GetCodeBlock()->m_lEndLine;
                    loc.m_lBegColumn = m_Procedure->PMethodImpl()->GetCodeBlock()->m_lEndColumn;
                    if (m_Procedure->PMethodImpl()->GetProcBlock()->m_lEndColumn == 0)
                    {
                        // Devdiv Bugs[28502] There's no valid code block location.  This is the best we can do.
                        loc.m_lEndLine = loc.m_lBegLine;
                        loc.m_lEndColumn = loc.m_lBegColumn;
                    }
                    else
                    {
                        loc.m_lEndLine = m_Procedure->PMethodImpl()->GetProcBlock()->m_lEndLine;
                        loc.m_lEndColumn = max(0, m_Procedure->PMethodImpl()->GetProcBlock()->m_lEndColumn-1);
                    }
                }

                Location *pLoc = NULL;

                if (m_ProcedureTree->Child!=NULL && m_ProcedureTree->Child->bilop == SB_STATEMENT_LAMBDA)
                {
                    pLoc = &(m_ProcedureTree->Child->AsStatementLambdaBlock().EndConstructLocation);
                }
                else if (m_Procedure->IsMethodImpl())
                {
                    pLoc = &loc;
                }
                else
                {
                    pLoc = m_Procedure->GetLocation();
                }
                    
                STRING *pName = NULL;

                if (m_ProcedureTree->Child!=NULL && m_ProcedureTree->Child->bilop == SB_STATEMENT_LAMBDA)
                {
                    StringBuffer sbCurrentName;
                    ResLoadStringRepl(STRID_AnonymousDelegate, &sbCurrentName);
                    pName = GetCompiler()->AddString(&sbCurrentName);
                }
                else if (m_Procedure->IsUserDefinedOperatorMethod())
                {
                    pName = m_Procedure->GetAssociatedOperatorDef()->GetName();
                }
                else if (m_Procedure->IsPropertyGet() )
                {
                    pName = m_Procedure->GetAssociatedPropertyDef()->GetName();
                }
                else if (IsWinRTEventAddHandler(m_Procedure, GetFXSymbolProvider()))
                {
                    pName = GetCompiler()->AddString(L"AddHandler As EventRegistrationToken");
                    // Note: it would have been better if users wrote in their VB source code 
                    // something like "AddHandler(...) As EventRegistrationTokenType"
                    // But we're too late into the cycle to make that language change.
                    // Writing it out here in the error text is the best we can manage for alerting
                    // users about what is the thing they need to return.
                }
                else
                {
                    pName = m_Procedure->GetErrorName(m_Compiler);
                }

                ReportDefAsgNoRetVal(m_Procedure, pRetVar->GetType(), *pLoc, pName);
            }
        }
    }


    // Unused locals
    if ( !m_DefAsgOnErrorSeen && !OnlyCheckReturns )
    {
        DBG_SWITCH_PRINTF(fDumpFlow, L"CheckFlow:UnusedLocals?: %s.%s\n", PhysicalContainer->GetName(), m_Procedure->GetName());
        DefAsgUnusedVisitor unusedVisitor(this);
        unusedVisitor.Visit(BoundBody);  // this calls CheckUnusedLocals(block) for each executable block in BoundBody

        // check the static locals
        VSASSERT(ContainingClass(), "why no containing class for this procedure");
        AssertIfNull(PhysicalContainer);

        if (PhysicalContainer && PhysicalContainer->IsClass() && PhysicalContainer->PClass()->GetBackingFieldsForStatics())
        {
            BCITER_HASH_CHILD BackingFields(PhysicalContainer->PClass()->GetBackingFieldsForStatics());

            for (Declaration *BackingField = BackingFields.GetNext();
                BackingField;
                BackingField = BackingFields.GetNext())
            {
                if (BackingField->IsStaticLocalBackingField() &&
                    !BackingField->PStaticLocalBackingField()->IsInitFlag() &&
                    BackingField->PStaticLocalBackingField()->GetProcDefiningStatic() == m_Procedure &&
                    BackingField->IsVariable() && !BackingField->PVariable()->IsUsed())
                {
                    ReportSemanticError( WRNID_UnusedLocal, BackingField->PVariable()->GetLocation(), BackingField->PVariable());
                }
            }
        }
    }

    // And now unslot the locals we might have been using,
    // so that any future runs on parts of this routine won't be confused by our old slot assignments.
    // 



    blockVisitor.Visit(BoundBody, false, false);
	m_DefAsgCount=0;

}


// Start with the current bitset and the current reachabble flag, run the list
// and let the output bitset and reachable flag in
// m_DefAsgCurrentBitset/m_DefAsgIsReachableCode
void
Semantics::DefAsgEvalStatementList
(
 ILTree::PILNode ptreeStmt,
 unsigned int ClosureDepth
)
{
    // Start with the current bitset and the current reachable flag, run the list
    // and let the output bitset and reachable flag in
    // m_DefAsgCurrentBitset/m_DefAsgIsReachableCode
    // The caller is supposed to set before / use after these values.

    for (ILTree::PILNode ptreeStmtCur = ptreeStmt;
        ptreeStmtCur != NULL;
        ptreeStmtCur = ptreeStmtCur->AsStatement().Next)
    {
#pragma warning (suppress:4302)
        DBG_SWITCH_PRINTF(fDumpFlow, L"%*c  EvalStatement %S  {CURBITSET=%08x}\n", ClosureDepth*2, ' ', ILTree::BilopName(ptreeStmt->bilop), PtrToUlong(m_DefAsgCurrentBitset));

        // stop the eval of this sequence if the code is no more reachable
        // the block will end in non reachable state
        if (!m_DefAsgIsReachableCode)
        {
            DBG_SWITCH_PRINTF(fDumpFlow, L"%*c  .unreachable.\n", ClosureDepth*2, ' ');
            break;
        }


        // This statement may be in the re-eval list ( if the previous is a block and it is in the list).
        // if so, remove it.
        if (ptreeStmtCur->AsStatement().Prev != NULL &&
            ptreeStmtCur->AsStatement().Prev->IsExecutableBlockNode())
        {
            // again try/catch/finally sequence is considered as a group
            // 'catch' and 'finally' textually follow a 'try' but the logically following statement is the one after the group
            if (ptreeStmtCur->bilop != SB_CATCH && ptreeStmtCur->bilop != SB_FINALLY) // ignore the components of the try group
            {
                ILTree::PILNode prevStmt = ptreeStmtCur->AsStatement().Prev;
                if (prevStmt->bilop == SB_CATCH || prevStmt->bilop == SB_FINALLY)
                {
                    prevStmt = GetTryBlockInTryGroup(prevStmt);
                }

                this->m_DefAsgReEvalStmtList.RemoveReEvalStmt(prevStmt);
            }
        }


        ILTree::ExecutableBlock* targetBlock = NULL;
        switch (ptreeStmtCur->bilop)
        {
        case SL_STMT:
            {
                VSASSERT(ptreeStmtCur->AsStatementWithExpression().ptreeOp1, "empty statement unexpected. : #11/12/2003#");

                // For partial classes, members could come from different files and hence their initializers'
                // code could come from different files. Errors need to be reported into the appropriate
                // error table.
                //
                ErrorTable *AlternateErrorTable = NULL;
                ErrorTable *SavedErrors = m_Errors;

                if (m_AltErrTablesForConstructor &&
                    m_Procedure->IsAnyConstructor() &&
                    ptreeStmtCur->AsStatementWithExpression().GetContainingFile() &&
                    ptreeStmtCur->AsStatementWithExpression().GetContainingFile() != m_SourceFile &&
                    m_ReportErrors &&
                    m_Errors)
                {
                    AlternateErrorTable = m_AltErrTablesForConstructor(ptreeStmtCur->AsStatementWithExpression().GetContainingFile());
                }

                if (AlternateErrorTable)
                {
                    m_Errors = AlternateErrorTable;
                }

                if (ptreeStmtCur->AsStatementWithExpression().ptreeOp1->bilop == SX_SEQ &&
                    ptreeStmtCur->AsStatementWithExpression().ptreeOp1->AsExpressionWithChildren().Left &&
                    ptreeStmtCur->AsStatementWithExpression().ptreeOp1->AsExpressionWithChildren().Left->bilop == SX_ASG &&
                    ptreeStmtCur->AsStatementWithExpression().ptreeOp1->AsExpressionWithChildren().Left->AsExpressionWithChildren().Left &&
                    ptreeStmtCur->AsStatementWithExpression().ptreeOp1->AsExpressionWithChildren().Left->AsExpressionWithChildren().Left->bilop == SX_SYM &&
                    ptreeStmtCur->AsStatementWithExpression().ptreeOp1->AsExpressionWithChildren().Left->AsExpressionWithChildren().Left->uFlags & SXF_SYM_ASGVIACALL)
                {
                    ILTree::PILNode    ptreeLHS = ptreeStmtCur->AsStatementWithExpression().ptreeOp1->AsExpressionWithChildren().Left->AsExpressionWithChildren().Left;
                    ILTree::PILNode    ptreeRHS = ptreeStmtCur->AsStatementWithExpression().ptreeOp1->AsExpressionWithChildren().Left->AsExpressionWithChildren().Right;
                    DefAsgCheckUse(ptreeRHS, ClosureDepth);

                    DefAsgSetOrCheckForUse(ptreeLHS, ClosureDepth);
                }
                else if (ptreeStmtCur->AsStatementWithExpression().ptreeOp1->bilop == SX_ASG)
                {
                    ILTree::PILNode    ptreeLHS = ptreeStmtCur->AsStatementWithExpression().ptreeOp1->AsExpressionWithChildren().Left;
                    ILTree::PILNode    ptreeRHS = ptreeStmtCur->AsStatementWithExpression().ptreeOp1->AsExpressionWithChildren().Right;

                    if (ptreeRHS->bilop != SX_LAMBDA)
                    {
                        // Normally, in "Dim var = expr", we evaluate expr before assigning var
                        DefAsgCheckUse(ptreeRHS, ClosureDepth);
                        DefAsgSetOrCheckForUse(ptreeLHS, ClosureDepth);
                    }
                    else
                    {  
                        // But for lambdas, "Dim f = function() ...f()", then the lambda is assigned
                        // to f before the body will be executed
                        DefAsgSetOrCheckForUse(ptreeLHS, ClosureDepth);
                        DefAsgCheckUse(ptreeRHS, ClosureDepth);
                    }
                }
                else
                {
                    DefAsgCheckUse(ptreeStmtCur->AsStatementWithExpression().ptreeOp1, ClosureDepth);
                }

                // Restore the original error table
                m_Errors = SavedErrors;
                break;
            }

        case SL_CONTINUE:
            {
                ILTree::GeneralLoopBlock* block = &(ptreeStmtCur->AsContinueStatement().EnclosingLoopStatement->AsGeneralLoopBlock());
                block->outDefAsgBitsetLocal =
                    block->outDefAsgBitsetLocal->andIntoOrCopy(
                    m_DefAsgCurrentBitset,
                    &m_DefAsgAllocator);
                m_DefAsgIsReachableCode = false;
                break;
            }

        case SL_RETURN:
            {
                // Return should behave as exit vs. finally blocks.
                // Since the only bit we use in the out bitset of a method is the one associated to
                // the return variable in functions, there is no point to do all the work.
                // Yet, the semantics turns 'Exit Sub/Function/Property' into return. Such 'return' statements should be treated as 'exit'
                if ( HasFlag32(ptreeStmtCur->AsReturnStatement(),SLF_RET_FROM_EXIT))
                {
                    // Dev10 #682864: Skip 'Exit Sub'/'Exit Function' statements in lambdas.
                    if ( ClosureDepth == 0 )
                    {
                        ptreeStmtCur->AsReturnStatement().outDefAsgBitset =
                            ptreeStmtCur->AsReturnStatement().outDefAsgBitset->andIntoOrCopy(
                            m_DefAsgCurrentBitset,
                            &m_DefAsgAllocator);

                        targetBlock = m_ProcedureTree;

                        // goto into the exit block, this is an Exit statement
                        goto RET_EXIT;
                    }
                }
                else 
                {
                    // bug500329,500333: the next two statements must be done even in lambdas
                    DefAsgCheckUse(ptreeStmtCur->AsReturnStatement().ReturnExpression, ClosureDepth);
                    m_DefAsgIsReachableCode = false;

                    // Following code for setting the "return" bit is only done at the top-level procedure;
                    // we don't check return bits for lambdas yet (indeed they don't even have slots).
                    // That's done in a separate later CheckFlow, once per lambda.
                    if (ClosureDepth == 0)
                    {
                        BCSYM_Variable * pRetVar = m_ProcedureTree->AsProcedureBlock().ReturnVariable;
                        if (pRetVar && pRetVar->GetDefAsgSlot())
                        {
                            m_DefAsgCurrentBitset = m_DefAsgCurrentBitset->setBits(pRetVar->GetDefAsgSlot(), pRetVar->GetDefAsgSize());
                            DBG_SWITCH_PRINTF(fDumpFlow, L"  *Return %s  [size=%u slot=%u]  {CURBITSET=%08x}\n", pRetVar->GetName(), pRetVar->GetDefAsgSlot(), pRetVar->GetDefAsgSize(), m_DefAsgCurrentBitset);
                            m_ProcedureTree->outDefAsgBitset =
                                m_ProcedureTree->outDefAsgBitset->andIntoOrCopy(
                                m_DefAsgCurrentBitset,
                                &m_DefAsgAllocator);
                        }
                    }
                }
                break;
            }

        case SL_YIELD:
            {
                DefAsgCheckUse(ptreeStmtCur->AsYieldStatement().YieldExpression, ClosureDepth);
                break;
            }

        case SL_EXIT:
            {
                ptreeStmtCur->AsExitStatement().outDefAsgBitset =
                    ptreeStmtCur->AsExitStatement().outDefAsgBitset->andIntoOrCopy(
                    m_DefAsgCurrentBitset,
                    &m_DefAsgAllocator);
                targetBlock = &(ptreeStmtCur->AsExitStatement().ExitedStatement->AsExecutableBlock());

RET_EXIT:
                DWORD_PTR isBitSetChanged = 0;
                // 'cross try' exits are treated as goto's out of try. The finally clauses are executed on the way too the target
                // a 'cross try' 'exit try' statements do not make much of a sense. But they are possible
                // when 'exit try' happens in a synthetic try generated by i.e synclock. Such cases
                // are treated as normal exit try/catch.
                if ((ptreeStmtCur->uFlags & SLF_EXITS_TRY || ptreeStmtCur->uFlags & SLF_EXITS_CATCH) &&
                    targetBlock->bilop != SB_TRY && targetBlock->bilop != SB_CATCH)
                {
                    this->UpdateLabelOrBlockAcrossTryCatch(ptreeStmtCur, true/*updateFinallyFixups*/, ClosureDepth);
                }
                else
                {
                    targetBlock->outDefAsgBitset =
                        targetBlock->outDefAsgBitset->andIntoOrCopy(
                        m_DefAsgCurrentBitset,
                        &isBitSetChanged,
                        &m_DefAsgAllocator);

                    //check exit function without a return value
                    if ( targetBlock->bilop == SB_PROC &&
                        m_ProcedureTree->AsProcedureBlock().ReturnVariable  &&
                        m_ProcedureTree->AsProcedureBlock().ReturnVariable->GetDefAsgSlot() &&
                        !m_ProcedureTree->AsProcedureBlock().fDefAsgNoRetValReported &&
                        ClosureDepth == 0)
                    {
                        BCSYM_Variable * pRetVar = m_ProcedureTree->AsProcedureBlock().ReturnVariable;
                        if (!m_DefAsgCurrentBitset->testBits(pRetVar->GetDefAsgSlot(), pRetVar->GetDefAsgSize()))
                        {
                            m_ProcedureTree->AsProcedureBlock().fDefAsgNoRetValReported = true;

                            if (m_ProcedureTree->Child!=NULL && m_ProcedureTree->Child->bilop == SB_STATEMENT_LAMBDA)
                            {
                                StringBuffer sbCurrentName;
                                ResLoadStringRepl(STRID_AnonymousDelegate, &sbCurrentName);
                                STRING * pName = GetCompiler()->AddString(&sbCurrentName);

                                ReportDefAsgNoRetVal(m_Procedure, pRetVar->GetType(), ptreeStmtCur->Loc, pName);
                            }
                            else
                            {
                                ReportDefAsgNoRetVal(m_Procedure,
                                                     pRetVar->GetType(),
                                                     ptreeStmtCur->Loc,
                                                     m_Procedure->IsUserDefinedOperatorMethod() ?
                                                       m_Procedure->GetAssociatedOperatorDef()->GetName() :
                                                         (m_Procedure->IsPropertyGet() ?
                                                            m_Procedure->GetAssociatedPropertyDef()->GetName() :
                                                            m_Procedure->GetErrorName(m_Compiler)));
                            }
                        }

                    }

                    // add the block in the re-eval list if a change is introduced by 'exit'.
                    // Actually the statement following the block wil be the re-eval start statement
                    if (isBitSetChanged)
                    {
                        if(targetBlock->bilop == SB_TRY || targetBlock->bilop == SB_CATCH)
                        {
                            // a 'try,catch,finally' sequence is semantically a group statement
                            // the change may not be just local. Recompute the general out-bitset of the whole group with the new component
                            // see if the change gets out of the group. No change could mean stable out bit set of the try group.
                            // Or, it could mean no out bit set at all and the next statement is unreachable. Either way, no continnuation
                            // return 0
                            isBitSetChanged = 0;
                            ILTree::TryBlock* pTry = DefAsgUpdateTryGroupOutBitSet(targetBlock, &isBitSetChanged);
                            if (isBitSetChanged)
                            {
                                m_DefAsgReEvalStmtList.AddReEvalStmt(pTry, ClosureDepth, &m_DefAsgAllocator);
                            }
                        }
                        else
                        {
                            m_DefAsgReEvalStmtList.AddReEvalStmt(targetBlock, ClosureDepth, &m_DefAsgAllocator);
                        }
                    }
                }
                m_DefAsgIsReachableCode = false;
                break;
            }

            /*
             * Array statements
             */
        case SL_REDIM:
            {
                ILTree::PILNode pSym = ptreeStmtCur->AsRedimStatement().ptreeOp1;
                VSASSERT(pSym->bilop == SX_SYM,  "Definite assignment: malformed redim tree.");
                //Note: redim preserve semantics is still under debate. For now consider as an assignment
                // Uncomment the code bellow if 'redim preserve' is meant as an use
                //if (ptreeStmtCur->uFlags & SLF_REDIM_PRESERVE)
                //{
                //    // preserve is a use
                //    DefAsgCheckUse(pSym);
                //}
                //else
                DefAsgSetOrCheckForUse(pSym, ClosureDepth);

                // check the list of operands assigned to redim
                DefAsgCheckUse(ptreeStmtCur->AsRedimStatement().Operand2, ClosureDepth);
        }

            //GenerateRedim(ptreeStmtCur);
            break;

            /*
             * Error/debug statements
             */
        case SL_ERROR:
            DefAsgCheckUse(ptreeStmtCur->AsStatementWithExpression().Operand1, ClosureDepth);
            m_DefAsgIsReachableCode = false;
            break;

        case SL_ON_ERR:
        case SL_RESUME:
            m_DefAsgOnErrorSeen = true;
            break;

        case SL_END:
            m_DefAsgIsReachableCode = false;
            break;

        case SL_STOP:
            break;

            /*
             * Branch statements
             */
        case SL_GOTO:
            {
                ILTree::GotoStatement &gotoNode = ptreeStmtCur->AsGotoStatement();
                gotoNode.outDefAsgBitset =
                    gotoNode.outDefAsgBitset->andIntoOrCopy(
                        m_DefAsgCurrentBitset,
                        &m_DefAsgAllocator);

                DWORD_PTR isLabelBitSetChanged = 0;
                ILTree::LabelStatement &labelNode = *(gotoNode.Label->pLabelParent);
                if (ptreeStmtCur->uFlags & SLF_EXITS_TRY || ptreeStmtCur->uFlags & SLF_EXITS_CATCH)
                {
                    this->UpdateLabelOrBlockAcrossTryCatch(ptreeStmtCur, true/*updateFinallyFixups*/, ClosureDepth);
                }
                else
                {
                    // goto not crossing try/catch
                    labelNode.inDefAsgBitset =
                        labelNode.inDefAsgBitset->andIntoOrCopy(
                        gotoNode.outDefAsgBitset,
                        &isLabelBitSetChanged,
                        &m_DefAsgAllocator);
                    if (isLabelBitSetChanged)
                    {
                        m_DefAsgReEvalStmtList.AddReEvalStmt(&labelNode, ClosureDepth, &m_DefAsgAllocator);
                    }
                }

                //Location targetLocation = ((ParseTree::Statement *)(labelNode.Label->DefiningStatement))->TextSpan;
                //bool gotoBack = ptreeStmtCur->Loc.m_lBegLine > targetLocation.m_lBegLine ||
                //                    (ptreeStmtCur->Loc.m_lBegLine == targetLocation.m_lBegLine &&
                //                      ptreeStmtCur->Loc.m_lBegColumn > targetLocation.m_lBegColumn);

                //if(isLabelBitSetChanged && gotoBack)

                //the code that follows goto is not reachable
                m_DefAsgIsReachableCode = false;
                m_DefAsgCurrentBitset = m_DefAsgCurrentBitset->setInto(BITSET_ALL);

            }
            break;

        case SL_LABEL:
            {
                // this label node is curently evaluated but also it might be in the list for re eval
                // it can safely removed and avoid an unecessary re eval
                this->m_DefAsgReEvalStmtList.RemoveReEvalStmt(ptreeStmtCur);

                // label reached by fallthrough, ( possiblly by previous go to as well)
                // if there is a previous in-bitset for the label, join(and) it to the current bitset
                ILTree::LabelStatement &label = ptreeStmtCur->AsLabelStatement();
                label.inDefAsgBitset =
                    label.inDefAsgBitset->andIntoOrCopy(
                        m_DefAsgCurrentBitset,
                        &m_DefAsgAllocator);

                m_DefAsgCurrentBitset = m_DefAsgCurrentBitset->setInto(label.inDefAsgBitset);

            }

            break;

        case SB_IF_BLOCK:
        case SB_SELECT:
        case SB_LOOP:
        case SB_FOR:
        case SB_FOR_EACH:
        case SB_HIDDEN_CODE_BLOCK:
        case SB_STATEMENT_LAMBDA:
            {
                ILTree::ExecutableBlock* block = &ptreeStmtCur->AsExecutableBlock();
                DBG_SWITCH_PRINTF(fDumpFlow, L"%*c  .Block %S: {INBITSET=%08x}\n", ClosureDepth*2, ' ', ILTree::BilopName(block->bilop), block->inDefAsgBitset);
                block->inDefAsgBitset = block->inDefAsgBitset->andIntoOrCopy(m_DefAsgCurrentBitset,&m_DefAsgAllocator);
                DefAsgEvalBlock(ptreeStmtCur, ClosureDepth);

                // the end of the block is reachable. Therefore, the statement that follows is reachable
                // set the current bitset and the reachable flag.
                if (block->outDefAsgBitset)
                {
                    m_DefAsgCurrentBitset = m_DefAsgCurrentBitset->setInto(block->outDefAsgBitset,&m_DefAsgAllocator);
                    m_DefAsgIsReachableCode = true;
                }
                else
                {
                    VSASSERT(m_DefAsgCurrentBitset, "Why the default bitset is NULL");
                    m_DefAsgCurrentBitset =  m_DefAsgCurrentBitset->setInto(BITSET_ALL);
                    m_DefAsgIsReachableCode = false;
                }

                break;
            }

            // a group of try-cath-finally nodes are in sequence. Too bad they are not members of a master block (like
            // IF block or statement). Use the m_DefAsgCurrentBitset to glue them together.
            // Since each statement in the try block can throw an exception, the in-bitset of the try block is passed
            // as in-bitset for the following catch and try clauses.

            // What is the out-bitset for all the group?
            // If the group has a finally clause:
            //      out-bitset = OR( out-bitset-finally, AND(out-bitset-try,out-bitset-catch1,out-bitset-catch2..))
            // If there is no finally clause
            //      out-bitset = AND(out-bitset-try,out-bitset-catch1,out-bitset-catch2..)
        case SB_TRY:
            {
                ILTree::ExecutableBlock* block = &ptreeStmtCur->AsExecutableBlock();
                DBG_SWITCH_PRINTF(fDumpFlow, L"%*c  .Block %S: {INBITSET=%08x}\n", ClosureDepth*2, ' ', ILTree::BilopName(block->bilop), block->inDefAsgBitset);
                block->inDefAsgBitset = block->inDefAsgBitset->andIntoOrCopy(m_DefAsgCurrentBitset,&m_DefAsgAllocator);

                // evaluate the finally block in advance in order to create support for the out of try goto's
                if (!IsInSyntheticTryGroup(ptreeStmtCur))
                {
                    ILTree::PILNode finBlk = GetFinallyBlockInTryGroup(ptreeStmtCur);
                    if (finBlk)
                    {
                        ILTree::FinallyBlock *finallyBlock = &finBlk->AsFinallyBlock();
                        // use the in bitset of the try block(current bitset)
                        finallyBlock->inDefAsgBitset = finallyBlock->inDefAsgBitset->andIntoOrCopy(block->inDefAsgBitset,&m_DefAsgAllocator);
                        DefAsgEvalBlock(finallyBlock, ClosureDepth);
                        // postpone the 'finally' specific eval by the time it is reached
                    }
                    //let the eval continue within the try/cath group even if this finally hit a thow/goto
                    m_DefAsgIsReachableCode = true;
                }
                // eval the try block
                DefAsgEvalBlock(ptreeStmtCur, ClosureDepth);

                // Synthetic try constructs are everywhere ( using, foreach, synclock, etc). They are related to
                // some implementation semantics.
                // Definite assignement treats a synthetic try as a simple block and ignores the associated
                // catch, finally block
                if (IsInSyntheticTryGroup(ptreeStmtCur))
                {
                    // synthetic try - see it as a normal bloc
                    if (block->outDefAsgBitset)
                    {
                        // the end of the block is reachable. Therefore, the statement that follows is reachable
                        // set the current bitset and the reachable flag.
                        VSASSERT(m_DefAsgCurrentBitset, "Why the default bitset is NULL");
                        m_DefAsgCurrentBitset = m_DefAsgCurrentBitset->setInto(block->outDefAsgBitset);
                        m_DefAsgIsReachableCode = true;
                    }
                    else
                    {
                        VSASSERT(m_DefAsgCurrentBitset, "Why the default bitset is NULL");
                        m_DefAsgCurrentBitset =  m_DefAsgCurrentBitset->setInto(BITSET_ALL);
                        m_DefAsgIsReachableCode = false;
                    }

                }
                else
                {
                    // non synthetic try. The execution flow can leave the block in very first statement.
                    m_DefAsgCurrentBitset = m_DefAsgCurrentBitset->setInto(block->inDefAsgBitset,&m_DefAsgAllocator);
                    m_DefAsgIsReachableCode = true;

                }
                break;
            }

        case SB_CATCH:
            {
                if (IsInSyntheticTryGroup(ptreeStmtCur) || (ptreeStmtCur->uFlags & SBF_OVERLAPED_CATCH))
                {
                    //ignore synthetic and overlaped catch blocks
                    break;
                }
                ILTree::ExecutableBlock* block = &ptreeStmtCur->AsExecutableBlock();
                block->inDefAsgBitset = block->inDefAsgBitset->andIntoOrCopy(m_DefAsgCurrentBitset,&m_DefAsgAllocator);
                DefAsgEvalBlock(ptreeStmtCur, ClosureDepth);

                // is this catch the end of the group? If not just pass the general in-bitset via current bitset
                // otherwise: do the end of the group calculations
                if ( !IsLastNonOverlapedCatchInTheGroup(ptreeStmtCur))
                {
                    // not the last in the group: pass the in-bitset
                    m_DefAsgCurrentBitset = m_DefAsgCurrentBitset->setInto(block->inDefAsgBitset,&m_DefAsgAllocator);
                    m_DefAsgIsReachableCode = true;
                }
                else
                {
                    // end of the group: Need to compute the general out-bitset of the group
                    DWORD_PTR dummyChanged;
                    ILTree::TryBlock* pTry = DefAsgUpdateTryGroupOutBitSet(ptreeStmtCur, &dummyChanged);
                    if (pTry->outDefAsgBitsetTryGroup)
                    {
                        VSASSERT(m_DefAsgCurrentBitset, "Why the default bitset is NULL");
                        m_DefAsgCurrentBitset = m_DefAsgCurrentBitset->setInto(pTry->outDefAsgBitsetTryGroup,&m_DefAsgAllocator);
                        m_DefAsgIsReachableCode = true;
                    }
                    else
                    {
                        VSASSERT(m_DefAsgCurrentBitset, "Why the default bitset is NULL");
                        m_DefAsgCurrentBitset =  m_DefAsgCurrentBitset->setInto(BITSET_ALL);
                        m_DefAsgIsReachableCode = false;
                    }
                }

                break;
            }

        case SB_FINALLY:
            {
                if (IsInSyntheticTryGroup(ptreeStmtCur))
                {
                    //ignore synthetic catch block
                    break;
                }

                // the block should be already evaluated by the 'try' node
                VSASSERT(ptreeStmtCur->AsExecutableBlock().inDefAsgBitset, "The finally block should be already evaluated");
                //BILTREE_sbExecutableBlock* block = &ptreeStmtCur->AsExecutableBlock();
                //block->inDefAsgBitset = block->inDefAsgBitset->andIntoOrCopy(m_DefAsgCurrentBitset,&m_DefAsgAllocator);
                //DefAsgEvalBlock(ptreeStmtCur);

                //
                // End of the 'try' group. need to build the out-bit set of the group which is:
                //    OR( out-bitset-finally, AND(out-bitset-try,out-bitset-catch1,out-bitset-catch2..))

                // end of the finally block is reachable if at least one of the try block or catch block is reachable and finally block
                // is reachable too.
                DWORD_PTR dummyChanged;
                ILTree::TryBlock* pTry = DefAsgUpdateTryGroupOutBitSet(ptreeStmtCur, &dummyChanged);
                if (pTry->outDefAsgBitsetTryGroup)
                {
                    m_DefAsgCurrentBitset = m_DefAsgCurrentBitset->setInto(pTry->outDefAsgBitsetTryGroup,&m_DefAsgAllocator);
                    m_DefAsgIsReachableCode = true;
                }
                else
                {
                    //end of finally block is reachable - but all the try group is not
                    VSASSERT(m_DefAsgCurrentBitset, "Why the default bitset is NULL");
                    m_DefAsgCurrentBitset =  m_DefAsgCurrentBitset->setInto(BITSET_ALL);
                    m_DefAsgIsReachableCode = false;
                }

                break;
            }

        case SB_SYNCLOCK:
        case SB_USING:
        case SB_WITH:
            {
                // Using is a normal block, just execute it. No goto's into are allowed

                ILTree::ExecutableBlock* block = &ptreeStmtCur->AsExecutableBlock();
                block->inDefAsgBitset = block->inDefAsgBitset->andIntoOrCopy(m_DefAsgCurrentBitset,&m_DefAsgAllocator);
                DefAsgEvalBlock(ptreeStmtCur, ClosureDepth);

                if (block->outDefAsgBitset)
                {
                    // the end of the block is reachable. Therefore, the statement that follows is reachable
                    // set the current bitset and the reachable flag.
                    m_DefAsgCurrentBitset = m_DefAsgCurrentBitset->setInto(block->outDefAsgBitset,&m_DefAsgAllocator);
                    m_DefAsgIsReachableCode = true;
                }
                else
                {
                    // could hapen if using has a throw or some fancy goto
                    VSASSERT(m_DefAsgCurrentBitset, "Why the default bitset is NULL");
                    m_DefAsgCurrentBitset =  m_DefAsgCurrentBitset->setInto(BITSET_ALL);
                    m_DefAsgIsReachableCode = false;
                }
                break;
            }

        case SB_CASE:
        case SB_IF:
        case SB_ELSE:
        case SB_ELSE_IF:
            VSFAIL("ill formated bound tree, SB_IF/SB_ELSE/SB_ELSE_IF/SB_CASE out of a SB_IF_BLOCK");
            break;

        case SL_VAR_DECL:
        case SL_DEBUGLOCNOP:
            break;

        default:
            VSFAIL("Definite assignment eval: Unexpected stmtkind.");

        } // switch

    } // for
}


void
Semantics::DefAsgEvalBlock
(
    ILTree::PILNode ptreeBlock,
    unsigned int ClosureDepth
)
{
    ILTree::ExecutableBlock* block = &ptreeBlock->AsExecutableBlock();
    // the caller must set the in-bitset
    //
    VSASSERT(block->inDefAsgBitset && m_DefAsgIsReachableCode, "cannot eval a block with an empty bitset or when it is not reachable");
    m_DefAsgCurrentBitset = m_DefAsgCurrentBitset->setInto(block->inDefAsgBitset);

#pragma warning (suppress:4302)
    DBG_SWITCH_PRINTF(fDumpFlow, L"%*c  EvalBlock: %S@%08x  {INBITSET=%08x}  {OUTBITSET=%08x}\n", ClosureDepth*2, ' ', ILTree::BilopName(block->bilop), PtrToUlong(block), PtrToUlong(block->inDefAsgBitset), PtrToUlong(block->outDefAsgBitset));


    switch (block->bilop)
    {
    case SB_PROC:
    case SB_SYNCLOCK:
    case SB_USING:
    case SB_WITH:
    case SB_HIDDEN_CODE_BLOCK:
        {
            if (block->bilop == SB_WITH)
            {
                DefAsgCheckUse(ptreeBlock->AsWithBlock().ptreeWith, ClosureDepth);
            }

            DefAsgEvalStatementList(block->ptreeChild, ClosureDepth);

            if (m_DefAsgIsReachableCode) 
            {
                block->outDefAsgBitset = block->outDefAsgBitset->setInto(m_DefAsgCurrentBitset, &m_DefAsgAllocator);
            }
            break;
        }

        case SB_STATEMENT_LAMBDA:
        {
            // This case is hit when a lambda has been lifted into a closure, and
            // closures.cpp calls CheckFlow(invoke_procedure, OnlyCheckReturns)
            // It is NOT called when looking at a lambda in the context of the procedure where it was defined
            ILTree::ILNode *child = block->AsStatementLambdaBlock().ptreeChild;
            DefAsgEvalStatementList(child, ClosureDepth);
            if (m_DefAsgIsReachableCode)
            {
                block->outDefAsgBitset = block->outDefAsgBitset->setInto(m_DefAsgCurrentBitset, &m_DefAsgAllocator);
            }
            break;
        }


        /*
        * If/Then/Else statements
        */

        // The out-bitset (definite assignment set) of an if block is the 'and' operation of the if braches
        // as the flow can follow any branch. If there is no else, the flow can skip all the branches so
        // the in-bitset is part of the 'and' operation
        // 'If block' has a special semantics. The contained sequence is not a list of statements to be executed
        // in a sequence. It is a list of condition guarded blocks and needs a special treatment.

        // The select statement is very similar with if-else-if in VB as there is no fall down.
        // Therefore, the out-bitset of an select block is the 'and' operation of the select braches
        // as the flow can follow any branch. If there is no elseCase, the flow can skip all the branches so
        // the in-bitset is part of the 'and' operation
        // The only difference: Select statement has an expression to check and the case branches can have a list of (conditional) expressions to verify, but this is
        // done at the case block level.

    case SB_SELECT:
        DefAsgCheckUse(block->AsSelectBlock().SelectorCapture, ClosureDepth);
        __fallthrough;
    case SB_IF_BLOCK:
        {
            bool hasElseStmt = false;
            for (ILTree::PILNode pMemberStmtCur = block->ptreeChild;
                pMemberStmtCur != NULL;
                pMemberStmtCur = pMemberStmtCur->AsStatement().Next)
            {
                VSASSERT(pMemberStmtCur->bilop == SB_IF || pMemberStmtCur->bilop == SB_ELSE || pMemberStmtCur->bilop == SB_ELSE_IF ||
                    pMemberStmtCur->bilop == SB_CASE, "Unexpected member in a if block or a select block");
                if (pMemberStmtCur->bilop == SB_ELSE ||
                    (pMemberStmtCur->bilop == SB_CASE && (pMemberStmtCur->uFlags & SBF_CASE_ISELSE)))
                {
                    hasElseStmt = true;
                }
                ILTree::ExecutableBlock* memberBlock = &pMemberStmtCur->AsExecutableBlock();
                memberBlock->inDefAsgBitset =
                    memberBlock->inDefAsgBitset->setInto(
                    block->inDefAsgBitset,
                    &m_DefAsgAllocator);

                m_DefAsgIsReachableCode = true;
                DefAsgEvalBlock(pMemberStmtCur, ClosureDepth);

                // use out set as a temporary to keep the join AND of component sets
                if (memberBlock->outDefAsgBitset)
                {

                    block->outDefAsgBitset =
                        block->outDefAsgBitset->andIntoOrCopy(
                            memberBlock->outDefAsgBitset,
                            &m_DefAsgAllocator);
                }
            }

            if (!hasElseStmt && (block->inDefAsgBitset))
            {
                // if no else branch is present, the join set must consider the "in" set, if any.
                block->outDefAsgBitset =
                    block->outDefAsgBitset->andIntoOrCopy(
                        block->inDefAsgBitset,
                        &m_DefAsgAllocator);
            }
            break;
        }
    // block members in IF block
    case SB_IF:
    case SB_ELSE:
    case SB_ELSE_IF:
        {
            DefAsgCheckUse(ptreeBlock->AsIfBlock().Conditional, ClosureDepth);
            DefAsgEvalStatementList(block->ptreeChild, ClosureDepth);
            if (m_DefAsgIsReachableCode)
            {
                block->outDefAsgBitset =
                    block->outDefAsgBitset->andIntoOrCopy(
                        m_DefAsgCurrentBitset,
                        &m_DefAsgAllocator);
            }
            break;
        }

    // block members in SELECT block
    case SB_CASE:
        {
            ILTree::CASELIST     * pCaseList = ptreeBlock->AsCaseBlock().BoundCaseList;
            VSASSERT((pCaseList || (ptreeBlock->uFlags & SBF_CASE_ISELSE)), "DefAsg Case: CASE doesn't have caselist");

            if (ptreeBlock->uFlags & SBF_CASE_CONDITION)
            {
                DefAsgCheckUse(ptreeBlock->AsCaseBlock().Conditional, ClosureDepth);
            }
            else
            {
                while (pCaseList)
                {
                    DefAsgCheckUse(pCaseList->LowBound, ClosureDepth);
                    if (pCaseList->IsRange)
                    {
                        DefAsgCheckUse(pCaseList->HighBound, ClosureDepth);
                    }
                    pCaseList = pCaseList->Next;
                }  // while
            }

            DefAsgEvalStatementList(block->ptreeChild, ClosureDepth);
            if (m_DefAsgIsReachableCode)
            {
                block->outDefAsgBitset =
                    block->outDefAsgBitset->andIntoOrCopy(
                        m_DefAsgCurrentBitset,
                        &m_DefAsgAllocator);
            }
            break;
        }

        /*
        * Loop statements
        */
    case SB_LOOP:
        {
            // in-set of a loop depends also of the local out set if any.
            // The loop can be reachable via a goto inside the loop, as well
            ILTree::GeneralLoopBlock* loopBlock = &block->AsGeneralLoopBlock();
            if (loopBlock->outDefAsgBitsetLocal)
            {
                block->inDefAsgBitset = block->inDefAsgBitset->andIntoOrCopy(loopBlock->outDefAsgBitsetLocal, &m_DefAsgAllocator);
                m_DefAsgCurrentBitset =  m_DefAsgCurrentBitset->setInto(block->inDefAsgBitset,&m_DefAsgAllocator);
            }
            DWORD_PTR isLoopInBitSetChanged;
            do
            {
                m_DefAsgIsReachableCode = true;
                isLoopInBitSetChanged = 0;
                if (block->uFlags & SBF_DO_WHILE)
                {
                    VSASSERT(!(block->uFlags & SBF_DO_LOOP), " cann't be both: top and bottom loop");
                    DefAsgCheckUse(ptreeBlock->AsLoopBlock().Conditional, ClosureDepth);
                }

                DefAsgEvalStatementList(block->ptreeChild, ClosureDepth);

                // simulate a goto back to the loop start
                // the start of the loop can be yet non reachable with a null in-bitset, when a goto points an
                // non reachable loop
                if (m_DefAsgIsReachableCode)
                {
                    loopBlock->outDefAsgBitsetLocal =
                        loopBlock->outDefAsgBitsetLocal->andIntoOrCopy(
                            m_DefAsgCurrentBitset,
                            &m_DefAsgAllocator);
                }

                // the code might be not reachable but an continue statement can make the end of the block reachable
                if (loopBlock->outDefAsgBitsetLocal)
                {
                    m_DefAsgIsReachableCode = true;
                    //so the loop bottom condition is reachable too
                    if (block->uFlags & SBF_LOOP_WHILE)
                    {
                        VSASSERT(!(block->uFlags & SBF_DO_WHILE), " cann't bot top and bottom loop");
                        m_DefAsgCurrentBitset = m_DefAsgCurrentBitset->setInto(loopBlock->outDefAsgBitsetLocal,&m_DefAsgAllocator);
                        DefAsgCheckUse(ptreeBlock->AsLoopBlock().Conditional, ClosureDepth);
                    }

                    block->inDefAsgBitset =
                        block->inDefAsgBitset->andIntoOrCopy(
                            loopBlock->outDefAsgBitsetLocal,
                            &isLoopInBitSetChanged,
                            &m_DefAsgAllocator);

                    m_DefAsgCurrentBitset = m_DefAsgCurrentBitset->setInto(block->inDefAsgBitset,&m_DefAsgAllocator);

                }

                // start over because the loop start is reachable via the back goto with a more restrictive bitset.
            } while (isLoopInBitSetChanged);

            VSASSERT(block->uFlags & (SBF_DO_WHILE | SBF_LOOP_WHILE | SBF_DO_LOOP),
                "Definite assignment: invalid loop kind: one of these three should be set");
            if (block->uFlags & SBF_DO_WHILE)
            {
                // top test. The statement following the loop is reachable if the beginnig
                // of the loop is reachable ( by fall down or by a go to into the loop)
                // out-bitset is the in-bitset(iterated) if any
                if (block->inDefAsgBitset)
                {
                    block->outDefAsgBitset =
                        block->outDefAsgBitset->andIntoOrCopy(
                            block->inDefAsgBitset,
                            &m_DefAsgAllocator);
                }
                if (loopBlock->outDefAsgBitsetLocal)
                {
                    block->outDefAsgBitset =
                        block->outDefAsgBitset->andIntoOrCopy(
                            loopBlock->outDefAsgBitsetLocal,
                            &m_DefAsgAllocator);
                }

            }
            else if (block->uFlags & SBF_LOOP_WHILE)
            {
                // the flow must pass through the block of the loop.
                // The statement following the loop is reachable only if the end of the loop block is reachable
                // The out-bitset is the current bit set at the end of the block (if the end is reachable)
                if (loopBlock->outDefAsgBitsetLocal)
                {
                    loopBlock->outDefAsgBitset =
                        loopBlock->outDefAsgBitset->andIntoOrCopy(
                            loopBlock->outDefAsgBitsetLocal,
                            &m_DefAsgAllocator);
                }

            }
            // else
            //(block->uFlags & SBF_DO_LOOP) - infinite loop. no out-set except there is one via exit,
            // next statement could be not reachable
            break;
        }

    case SB_FOR:
    case SB_FOR_EACH:
        {
            // goto into for blocks are not allowed.
            // unlike other blocks, 'for' can be reachable only via fall through.
            ILTree::GeneralLoopBlock* loopBlock = &block->AsGeneralLoopBlock();

            // the limits and increments, (or the collection when 'foreach') are evaluated once before the loop
            ILTree::Expression  * ControlVariableReference = (block->bilop == SB_FOR)?
                    ptreeBlock->AsForBlock().ControlVariableReference :
                    ptreeBlock->AsForEachBlock().ControlVariableReference;
            if (block->bilop == SB_FOR)
            {
                // check the initial value of the 'for' control variable
                DefAsgCheckUse(ptreeBlock->AsForBlock().Initial, ClosureDepth);
                DefAsgCheckUse(ptreeBlock->AsForBlock().Limit, ClosureDepth);
                DefAsgCheckUse(ptreeBlock->AsForBlock().Step, ClosureDepth);
            }
            else
            {
                // it is 'for each' the collection must be available before the contol variable get assigned to the first member of the collection
                VSASSERT(block->bilop == SB_FOR_EACH, "Definite assignment: bad 'for' opcode");

                // Dev10 #801818 Iterate Initializer instead of Collection, Collection is there as a service for XmlGen and the IDE, 
                // it is not used for CodeGen.
                DefAsgCheckUse(ptreeBlock->AsForEachBlock().Initializer, ClosureDepth);
            }

            // the control variable gets initialized with the InitialValue(or first member of the collection in foreach).
            // Consider control variable initialized, set the slot.


            // Dev11 Bug 150829, if ControlVariableReference is NULL, it should 
            // skip DefAsgSet. Also assert if ControlVariableReference is NULL.
            if (NULL != ControlVariableReference)
            {
                DefAsgSet(ControlVariableReference, ClosureDepth);
            }
            else
            {
                // Reactivate  Dev11 Bug 150829, if this assert is hit.          
                VSFAIL("Why ControlVariableReference is NULL?");
            }

            // UNDOONE Microsoft. Do we really need to loop here. Goto's into 'for' blocks are not allowed.
            DWORD_PTR isLoopInBitSetChanged = 0;
            do
            {
                m_DefAsgIsReachableCode = true;

                DefAsgEvalStatementList(block->ptreeChild, ClosureDepth);

                // simulate a goto back to the loop start
                // the start of the loop can be yet unreachable with a null in-bitset,
                // when a goto points into an unreachable loop the start becomes reachable
                if (m_DefAsgIsReachableCode)
                {
                    loopBlock->outDefAsgBitsetLocal =
                        loopBlock->outDefAsgBitsetLocal->andIntoOrCopy(
                            m_DefAsgCurrentBitset,
                            &m_DefAsgAllocator);

                    m_DefAsgCurrentBitset = m_DefAsgCurrentBitset->setInto(block->inDefAsgBitset,&m_DefAsgAllocator);
                }
                // the code might be not reachable but an continue statement can make the end of the block reachable
                if (loopBlock->outDefAsgBitsetLocal)
                {
                    block->inDefAsgBitset =
                        block->inDefAsgBitset->andIntoOrCopy(
                            loopBlock->outDefAsgBitsetLocal,
                            &isLoopInBitSetChanged,
                            &m_DefAsgAllocator);
                    m_DefAsgCurrentBitset = m_DefAsgCurrentBitset->setInto(block->inDefAsgBitset,&m_DefAsgAllocator);
                }

                // start over because the loop start is reachable via the back goto with a more restrictive bitset.
            } while (isLoopInBitSetChanged);

            // the statement following the 'for' stmt is reachable if the 'for' is reachable via top entry. Even if the end
            // of the block is not reachable due to an astray goto, still the for can end normally before the first itteration.
            // Set the output block as an and of the input block (if any)
            if (block->inDefAsgBitset)
            {
                block->outDefAsgBitset =
                    block->outDefAsgBitset->andIntoOrCopy(
                        block->inDefAsgBitset,
                        &m_DefAsgAllocator);
            }
            if (loopBlock->outDefAsgBitsetLocal)
            {
                 block->outDefAsgBitset =
                    block->outDefAsgBitset->andIntoOrCopy(
                        loopBlock->outDefAsgBitsetLocal,
                        &m_DefAsgAllocator);
            }
            if (block->outDefAsgBitset && block->bilop == SB_FOR)
            {
                // the control variable is assigned in the local bit set but it gets removed because it is not
                // set in the inBitSet
                // restore it
                DefAsgSet(ControlVariableReference, block->outDefAsgBitset, ClosureDepth);
            }

            break;
        }

    case SB_CATCH:
            VSASSERT(!(ptreeBlock->uFlags & SBF_OVERLAPED_CATCH), "How an overlaped Cath block gets to call a DefAsgEvalBlock()?");
            __fallthrough;
    case SB_FINALLY:
    case SB_TRY:
        {
            if (ptreeBlock->bilop == SB_CATCH)
            {
                ILTree::Expression * CatchVariableTree = ptreeBlock->AsCatchBlock().CatchVariableTree;
                // the catch exception variable comes initialized when enters a catch
                // it rather should be ignored but it is difficult to know it is a catch variable when assigning the slots
                // Consider catch variable initialized, set the slot.
                if (CatchVariableTree)
                {
                    // Sanity check: it should be a va riable and a simple one. Othervise it is ignored
                    VSASSERT( CatchVariableTree->bilop != SX_SYM ||
                        !CatchVariableTree->AsSymbolReferenceExpression().pnamed->DigThroughAlias()->PNamedRoot()->IsVariable() ||
                        CatchVariableTree->AsSymbolReferenceExpression().BaseReference == NULL,
                        "a for control variable cannot have a base");
                    BCSYM_Variable * pvar = CatchVariableTree->AsSymbolReferenceExpression().pnamed->DigThroughAlias()->PNamedRoot()->PVariable();
                    pvar->SetIsUsed();  // hack: force catch variables as used
                    DefAsgSet(CatchVariableTree, ClosureDepth);
                }

                // make sure to scan for use the filter
                DefAsgCheckUse(ptreeBlock->AsCatchBlock().WhenExpression, ClosureDepth);
            }
            DefAsgEvalStatementList(block->ptreeChild, ClosureDepth);
            if (m_DefAsgIsReachableCode)
            {
                block->outDefAsgBitset =
                    block->outDefAsgBitset->andIntoOrCopy(
                    m_DefAsgCurrentBitset,
                    &m_DefAsgAllocator);
            }
            break;
        }


        // statements. Should not be here any
        //case SL_CONTINUE:
        //case SL_RETURN:
        //case SL_STMT:
        //case SL_EXIT:
        //case SL_REDIM:
        //case SL_ERROR:
        //case SL_ON_ERR:
        //case SL_RESUME:
        //case SL_END:
        //case SL_STOP:
        //case SL_GOTO:
        //case SL_LABEL:
    default:
        VSFAIL("Definite assignment eval: Unexpected stmtkind.");

    } // switch

#pragma warning (suppress:4302)
    DBG_SWITCH_PRINTF(fDumpFlow, L"%*c  EvalBlockDone: %S@%08x  {INBITSET=%08x}  {OUTBITSET=%08x}\n", ClosureDepth*2, ' ', ILTree::BilopName(block->bilop), PtrToUlong(block), PtrToUlong(block->inDefAsgBitset), PtrToUlong(block->outDefAsgBitset));

}

bool
Semantics::IsFieldAccessInLocalOfStructType
(
  ILTree::PILNode ptree
)
{
    // check for a field of a local of type structure. It is a sequence a.b.c.d, where 'a' is a local variable
    // of type struct, 'b','c' are fields of type struct and 'd' is a field
    // a recursive implementation would be more descriptive but the simple tree walk is more eficient
    // The tree has the shape:

    //                  SX_SYM
    //                  /   \
    //              SX_ADR   d
    //                 |
    //              SX_SYM
    //              /   \
    //          SX_ADR   c
    //             |
    //          SX_SYM
    //          /   \
    //      SX_ADR   b
    //         |
    //      SX_SYM
    //      /   \
    //     0     a

    if (ptree == NULL)
    {
        return false;
    }

    while (ptree != NULL &&
        ptree->bilop == SX_SYM &&
        ptree->AsSymbolReferenceExpression().BaseReference != NULL &&
        ptree->AsSymbolReferenceExpression().BaseReference->bilop == SX_ADR &&
        (ptree->AsSymbolReferenceExpression().BaseReference->AsExpressionWithChildren().Left->ResultType->GetVtype() == t_struct) &&
        DefAsgIsValidMember(ptree->AsSymbolReferenceExpression().pnamed->DigThroughAlias()->PNamedRoot()))
    {
        ptree = ptree->AsSymbolReferenceExpression().BaseReference->AsExpressionWithChildren().Left;  // look at the base
        if (ptree->bilop == SX_SYM &&
            ptree->AsSymbolReferenceExpression().BaseReference == NULL &&
            ptree->AsSymbolReferenceExpression().pnamed->DigThroughAlias()->PNamedRoot()->IsVariable())

        {
            BCSYM_Variable * pvar = ptree->AsSymbolReferenceExpression().pnamed->DigThroughAlias()->PNamedRoot()->PVariable();
            SetLocalIsUsed(pvar);
            if (pvar->GetDefAsgSlot())
            {
                return true;
            }
        }
    }
    return false;
}

void Semantics::SetLocalIsUsed(BCSYM_Variable * pvar)
{
    DBG_SWITCH_PRINTF(fDumpFlow, L"  *LocalIsUsed: %s\n", pvar->GetName());
    if ( (pvar->IsLocal() && !pvar->IsTemporary() && !pvar->IsAlias() &&
            pvar->GetVarkind() != VAR_FunctionResult && pvar->GetVarkind() != VAR_Param) ||
         ( pvar->IsStaticLocalBackingField() && !pvar->PStaticLocalBackingField()->IsInitFlag() &&
            pvar->PStaticLocalBackingField()->GetProcDefiningStatic() == m_Procedure ))
    {
        pvar->SetIsUsed();
    }
}

unsigned Semantics::DefAsgGetSlotOfFieldStructExpr(ILTree::PILNode ptree)
{
    // set the slot and size of a field of a local of type structure.
    // It is a sequence a.b.c.d, where 'a' is a local variable
    // of type struct, 'b','c' are fields of type struct and 'd' is a field
    // a recursive implementation would be more descriptive but the simple tree walk is more eficient
    // The tree has the shape:

    //                  SX_SYM
    //                  /   \
    //              SX_ADR   d
    //                 |
    //              SX_SYM
    //              /   \
    //          SX_ADR   c
    //             |
    //          SX_SYM
    //          /   \
    //      SX_ADR   b
    //         |
    //      SX_SYM
    //      /   \
    //     0     a

    if (ptree == NULL || ptree->bilop != SX_SYM )
    {
        ptree->AsSymbolReferenceExpression().defAsgSlotCache = 1;
        return 0;
    }
    if (ptree->AsSymbolReferenceExpression().defAsgSlotCache)
    {
        return ptree->AsSymbolReferenceExpression().defAsgSlotCache - 1;
    }
    unsigned bSlot = 0;
    if (ptree->AsSymbolReferenceExpression().BaseReference == NULL )
    {
        if ( ptree->AsSymbolReferenceExpression().pnamed->DigThroughAlias()->PNamedRoot()->IsVariable())
        {
            bSlot = ptree->AsSymbolReferenceExpression().pnamed->DigThroughAlias()->PNamedRoot()->PVariable()->GetDefAsgSlot();
        }
        ptree->AsSymbolReferenceExpression().defAsgSlotCache = bSlot + 1;
        return bSlot;
    }

    if ( ptree->AsSymbolReferenceExpression().BaseReference->bilop != SX_ADR ||
         (ptree->AsSymbolReferenceExpression().BaseReference->AsExpressionWithChildren().Left->ResultType->GetVtype() != t_struct) ||
         !DefAsgIsValidMember(ptree->AsSymbolReferenceExpression().pnamed->DigThroughAlias()->PNamedRoot()))
    {
        ptree->AsSymbolReferenceExpression().defAsgSlotCache = 1;
        return 0;
    }

    if ( !(bSlot = DefAsgGetSlotOfFieldStructExpr(ptree->AsSymbolReferenceExpression().BaseReference->AsExpressionWithChildren().Left)))
    {
        ptree->AsSymbolReferenceExpression().defAsgSlotCache = 1;
        return 0;
    }
    // the current tree is  "base.field". The slot of base is known and base is a structure.
    // Walk the members of structure counting the size of each of them up to the 'field' member.
    // Get the offset of field
    DWORD count = 0;
    BCSYM* type = ptree->AsSymbolReferenceExpression().BaseReference->AsExpressionWithChildren().Left->ResultType;
    BCSYM_GenericBinding* GenericBindingContext=NULL;

    if (type->IsGenericBinding())
    {
        GenericBindingContext = type->PGenericBinding();
    }
    BCITER_CHILD ContainerIter;
    ContainerIter.Init(type->PNamedRoot());
    BCSYM_NamedRoot *Member;
    while (Member = ContainerIter.GetNext())
    {
        if (ptree->AsSymbolReferenceExpression().pnamed == Member)
        {
            break;
        }
        if (!DefAsgIsValidMember(Member))
        {
            continue;
        }
        BCSYM* memberType = Member->PMember()->GetType();
        if ( memberType->IsGenericParam() ||
            (memberType->GetVtype() == t_struct &&
                ( memberType->PClass()->IsGeneric() || memberType->PClass()->HasGenericParent())))
        {
            BCSYM* ResultType = ReplaceGenericParametersWithArguments(memberType, GenericBindingContext, m_SymbolCreator);
            count += DefAsgTypeSize(ResultType);
        }
        else
        {
            count += DefAsgTypeSize(memberType);
        }
    }
    ptree->AsSymbolReferenceExpression().defAsgSlotCache = bSlot + count + 1;
    return bSlot+count;
}


void
Semantics::DefAsgProcessSlots
(
    ILTree::ExecutableBlock *block,
    bool IncludeLocals,
    bool IncludeReturns
)
{
    DBG_SWITCH_PRINTF(fDumpFlow, L"  ProcessSlots: block %S\n", ILTree::BilopName(block->bilop));

    BCSYM_Hash *Locals = block->Locals;
    if (Locals)
    {
        BCSYM_NamedRoot *pLocal;

        HRESULT hr = S_OK;
        BCITER_CHILD_SORTED iterLocals(Locals);
        VSASSERT(SUCCEEDED(hr),"Ran out of stack space!");

        if (SUCCEEDED(hr))
        {
            while (pLocal = iterLocals.GetNext())
            {
                if (pLocal->IsAlias())
                {
                    continue;
                }

                BCSYM_Variable *pvar = pLocal->PVariable();
                // reset var, in case it had been used by a previous analysis
                pvar->SetDefAsgSlot(0);
                pvar->SetDefAsgSize(0);

                // the condition used to assign a slot is different than the ones used in check usage and
                // unused locals. Function result locals are set as temporary in user defined operators
                // Operator result locals still get a slot but they are ignored when checked for usage or unused locals.
                // The reason for the slot is only to track whether the operator returns a value.

                if ((IncludeLocals && pvar->GetVarkind() == VAR_Local && !pvar->IsTemporary()) ||
                         (IncludeReturns && pvar->GetVarkind() == VAR_FunctionResult)) 
                {
                    // the condition
                    BCSYM* varType = pvar->GetType();
                    unsigned size = DefAsgTypeSize(varType);
                    if (size == 0 && pvar->GetVarkind() == VAR_FunctionResult && IncludeReturns)
                    {
                        if (TypeHelpers::IsPrimitiveValueType(varType))
                        {
                            // Dev10#704852: also check for returns of the above primitive types. The same "is primitive" list is
                            // used inside GetDegAsgNoRetValErrorId, where it needs to know what kind of error message to give.
                            // The two lists must be identical.
                            size = 1;
                        }
                        else if (block->bilop == SB_PROC && IsWinRTEventAddHandler(block->AsProcedureBlock().pproc, GetFXSymbolProvider()))
                        {
                            // Dev11#376690: also there's one case so egregious that we special-case it. In a winmd
                            // custom event, the AddHandler is expected to return a value, but being an AddHandler
                            // it's not printed with a "As ..." clause. So users won't even notice
                            size = 1;
                        }
                    }
#pragma warning (suppress:4302)
                    DBG_SWITCH_PRINTF(fDumpFlow, L"    %s @%08x: slot=%i, size=%i\n", pvar->GetName(), PtrToUlong(pvar), m_DefAsgCount+1, size);
                    if (size)
                    {
#if DEBUG
                        m_DefAsgSlotAssignments.Grow(m_DefAsgCount+2 - m_DefAsgSlotAssignments.Count());
                        m_DefAsgSlotAssignments.Element(m_DefAsgCount+1) = pvar;
#endif
                        pvar->SetDefAsgSlot(m_DefAsgCount+1);
                        pvar->SetDefAsgSize(size);
                        m_DefAsgCount +=size;
                    }
                }
                else
                {
#pragma warning (suppress:4302)
                    DBG_SWITCH_PRINTF(fDumpFlow, L"    %s @%08x skipped; Varkind=%i\n", pvar->GetName(), PtrToUlong(pvar), pvar->GetVarkind());
                }
            } // while locals
        }
    }

    // Note: this function only processes a single block.
    // It is expected that the caller used DefAsgSlotVisitor, to also iterate over
    // the blocks and multiline lambdas that the current block contains.
}



ILTree::PILNode
Semantics::DefAsgUpdateParentGetContinuation
(
 ILTree::PILNode currentReEvalStmt,
 unsigned int ClosureDepth
)
{
    // Find the statement that is logically the next to the outer block of curentReEvalStmt.
    // If such a statement is reachable and needs to be re evaluated, set m_currentBitSet and return that statement
    // Note: besides the returned node, this function also sets the current bitset: m_DefAsgCurrentBitset

    // This method is invoked when the re eval process has reached the end of current block and the end of the block is reachable.
    // the out bitset of the parent may change. The efect of this reevaluation can be local or can affect
    // the ancestor blocks or statements following some ancestors.
    // Depending on the parrent block, the change may triger another re evaluation of the parent block or one of its ancestors

    // a parent block can be re evaluated in two ways:
    //   - starting from itself if it has a in-bitset
    //   - starting with the statement following the parent block if in-bitset is null but there is an out-bitset
    // The second case is a little murky: What if the parent is the last in the block that surrounds it? The next
    // statement is null. We need to walk up the parent structure and find the logical next statement.

    if (!m_DefAsgIsReachableCode)
    {
        return NULL;
    }
    ILTree::ExecutableBlock *parentBlock = currentReEvalStmt->AsStatement().Parent;
    if ( !parentBlock )
    {
        // this can hapen when the procedure block is added for reevaluation via an Exit Function
        return NULL;
    }

    DWORD_PTR isParentBitSetChangedNeedReEval = 0;
    switch (parentBlock->bilop)
    {
    case SB_PROC:
    case SB_STATEMENT_LAMBDA:
        // no reason to re-eval the procedure or lambda body itself
        {
            parentBlock->outDefAsgBitset =
                parentBlock->outDefAsgBitset->andIntoOrCopy(
                    m_DefAsgCurrentBitset,
                    &isParentBitSetChangedNeedReEval,
                    &m_DefAsgAllocator);

            DBG_SWITCH_PRINTF(fDumpFlow, L"  UpdateParentGetContinuation: %S {OUTBITSET=%08x}\n", ILTree::BilopName(parentBlock->bilop), parentBlock->outDefAsgBitset);

            // A multiline lambda can appear either within the context of a larger procedure (in which
            // case its parent will be expression node), or as a closure, in which case it is the
            // sole child of a SB_PROC. In the latter case, we need to propogate its output state
            // to that SB_PROC. That's where missing returns will be detected.
            //
            if (parentBlock->bilop==SB_STATEMENT_LAMBDA && parentBlock->Parent!=NULL && parentBlock->Parent->bilop==SB_PROC)
            {
                parentBlock->Parent->outDefAsgBitset = parentBlock->outDefAsgBitset;
            }

        }
        break;
        
    case SB_LOOP:
    case SB_FOR:
    case SB_FOR_EACH:
        {
            // the end of this loop is reachable. If the local out-bitset get changed the loop block needs to re-eval
            DWORD_PTR isLoopCtlLocalBitSetChanged = 0;
            parentBlock->AsGeneralLoopBlock().outDefAsgBitsetLocal =
                parentBlock->AsGeneralLoopBlock().outDefAsgBitsetLocal->andIntoOrCopy(
                m_DefAsgCurrentBitset,
                &isLoopCtlLocalBitSetChanged,
                &m_DefAsgAllocator);

            parentBlock->inDefAsgBitset =
                parentBlock->inDefAsgBitset->andIntoOrCopy(
                    parentBlock->AsGeneralLoopBlock().outDefAsgBitsetLocal,
                    &isParentBitSetChangedNeedReEval,
                    &m_DefAsgAllocator);

            // a change in in-bit set or out-bit set trigers the re eval
            if (isParentBitSetChangedNeedReEval ||
                (isLoopCtlLocalBitSetChanged && (parentBlock->uFlags & SBF_LOOP_WHILE))) // end condition loops can exit by the end
            {
                // as the in-bitset of the loop changed, the reeval should continue with all the loop
                // it is not worth to hash out the case when the out-bitset did not change. The loop has expressions
                // to scan, so go back for all the loop
                m_DefAsgCurrentBitset->setInto(parentBlock->inDefAsgBitset, &m_DefAsgAllocator);
                return parentBlock;
                //m_DefAsgReEvalStmtList.AddReEvalStmt(parentBlock, ClosureDepth, &m_DefAsgAllocator);
            }

            break;
        }

    case SB_IF_BLOCK:
    case SB_SELECT:
        VSFAIL("Definite assignment eval: an IF or SELECT block has only blocks as members and never is  re eval-ed as a partial sequence.");
        break;

    case SB_IF:
    case SB_ELSE:
    case SB_ELSE_IF:
    case SB_CASE:
        {
            // members of a larger structure an IF block or a SELECT
            // the parent of the parent needs to be revaluated if a change in the out-bitset
            // if "parent of the parent"
            // if/case structures are the worst. It is possible to have goto's into an if/case branch.
            // It is possible to hit by a go to a part of a if/case branch deeply nested in complex if/case structure
            // which is unreachable yet.
            parentBlock->outDefAsgBitset =
                parentBlock->outDefAsgBitset->andIntoOrCopy(
                    m_DefAsgCurrentBitset,
                    &isParentBitSetChangedNeedReEval,
                    &m_DefAsgAllocator);


            if (isParentBitSetChangedNeedReEval)
            {
                // If the parent block is reachable, the new output set can change the output of the "parent of the parent".
                // For example, the re eval point is in an else block. Then the BlockIF must be re evaluated.
                // If the "parent of the parent" block is not reachable, still the statement following it is reachable.
                // Therefore the "parent of The parent" should go in the re-eval list, at least with a non empty out-bitset
                // Make sure to adjust the output of the "parent of the parent"
                ILTree::ExecutableBlock *parentParentBlock = parentBlock->AsStatement().Parent;

                isParentBitSetChangedNeedReEval = 0;
                parentParentBlock->outDefAsgBitset =
                    parentParentBlock->outDefAsgBitset->andIntoOrCopy(
                        parentBlock->outDefAsgBitset,
                        &isParentBitSetChangedNeedReEval, // subtle behaviour here: may be the parentParent out-bitset do not change after all
                        &m_DefAsgAllocator);
                if (isParentBitSetChangedNeedReEval)
                {
                    if (parentParentBlock->Next != NULL)
                    {
                        // there is a  statement following the 'IF/SELECT' block. Set it as the continuation
                        m_DefAsgCurrentBitset->setInto(parentParentBlock->outDefAsgBitset, &m_DefAsgAllocator);
                        return parentParentBlock->Next;
                    }
                    else
                    {
                        // the IF/SELECT block  it is the last statement in the
                        // enclosing block. Need to update the  parent up.
                        m_DefAsgCurrentBitset = m_DefAsgCurrentBitset->setInto(parentParentBlock->outDefAsgBitset);
                        return DefAsgUpdateParentGetContinuation(parentParentBlock, ClosureDepth);
                    }
                }
            }
            break;
        }


    case SB_CATCH:
        VSASSERT(!(parentBlock->uFlags & SBF_OVERLAPED_CATCH), "How an overlaped Cath block gets continued in data flow?");
        __fallthrough;
    case SB_TRY:
    case SB_FINALLY:
        {
            // Except when the try/catch is synthetic. The parent block is interpreted as a normal block
            if (IsInSyntheticTryGroup(parentBlock))
            {
                VSASSERT(parentBlock->bilop != SB_CATCH && parentBlock->bilop != SB_FINALLY, "Why the code  of a synthetic cath got re evalued?");
                isParentBitSetChangedNeedReEval = 0;
                parentBlock->outDefAsgBitset =
                    parentBlock->outDefAsgBitset->andIntoOrCopy(
                    m_DefAsgCurrentBitset,
                    &isParentBitSetChangedNeedReEval,
                    &m_DefAsgAllocator);


                if (isParentBitSetChangedNeedReEval)
                {
                    m_DefAsgCurrentBitset = m_DefAsgCurrentBitset->setInto(parentBlock->outDefAsgBitset);
                    if (parentBlock->Next != NULL)
                    {
                        // the parent block TRY(synthetic) has a following statement
                        // the continuation is the following statement. It might be some synthetic 'catch/finally' but
                        // such synthetics are ignored
                        return (parentBlock->Next);
                    }
                    else
                    {
                        // as the goto's in such blocks should be intercepted, never a synthetic try block can have
                        // a null in-bitset (not reachable).
                        // Play safe for now and let the code recover even in this case.
                        return DefAsgUpdateParentGetContinuation(parentBlock, ClosureDepth);
                    }
                }
            }
            else
            {
                // non synthetic try/catch/finally
                isParentBitSetChangedNeedReEval = 0;
                parentBlock->outDefAsgBitset =
                    parentBlock->outDefAsgBitset->andIntoOrCopy(
                    m_DefAsgCurrentBitset,
                    &isParentBitSetChangedNeedReEval,
                    &m_DefAsgAllocator);

                if (isParentBitSetChangedNeedReEval)
                {
                    // the goto and exit statements in the try/catch blocks of this group that cross the try structure served by this
                    // finally need to be upadated. Update the fixup list if the finally block changes
                    if (parentBlock->bilop == SB_FINALLY &&
                        parentBlock->AsFinallyBlock().m_DefAsgGoToFixupList.NumberOfEntries())
                    {
                        ILTree::FinallyBlock& finallyBlock = parentBlock->AsFinallyBlock();
                        CSingleListIter<DefAsgReEvalStmt> IterFixupList(&finallyBlock.m_DefAsgGoToFixupList);
                        DefAsgReEvalStmt* pListElement;
                        IterFixupList.Reset();
                        while (pListElement = IterFixupList.Next())
                        {
                            UpdateLabelOrBlockAcrossTryCatch( pListElement->m_ReEvalStmt, false /*updateFinallyFixups*/, ClosureDepth);
                        }
                    }

                    // the change is not local. Recompute the general out-bitset of the whole group with the new component
                    // see if the change gets out of the group. No change could mean stable out bit set of the try group.
                    // Or, it could mean no out bit set at all and the next statement is unreachable. Either way, no continnuation
                    // return 0
                    isParentBitSetChangedNeedReEval = 0;
                    ILTree::TryBlock* pTry = DefAsgUpdateTryGroupOutBitSet(parentBlock, &isParentBitSetChangedNeedReEval);
                    if (isParentBitSetChangedNeedReEval)
                    {
                        m_DefAsgCurrentBitset = m_DefAsgCurrentBitset->setInto(pTry->outDefAsgBitsetTryGroup);
                        // move to the end of the group
                        while(parentBlock->Next != NULL &&
                            (parentBlock->Next->bilop == SB_CATCH || parentBlock->Next->bilop == SB_FINALLY))
                        {
                            parentBlock = &parentBlock->Next->AsExecutableBlock();
                        }
                        if (parentBlock->Next != NULL)
                        {
                            // the parent block is in a try/catch/finnaly group that  has a following statement
                            // the continuation is the statement following the group.

                            return (parentBlock->Next);
                        }
                        else
                        {
                            // the group is the last stmt in the enclosing block
                            return DefAsgUpdateParentGetContinuation(parentBlock, ClosureDepth);
                        }
                    }
                }
            }
            break;
        }

    case SB_SYNCLOCK:
    case SB_USING:
    case SB_WITH:
        {
            // the out-bit set of a Using/With block is the current bit set for the the statement that follows.
            // goto's into  'using/with' blocks are not allowed, If the block is not the last in its block then
            // there is a continuation
            parentBlock->outDefAsgBitset =
                parentBlock->outDefAsgBitset->andIntoOrCopy(
                m_DefAsgCurrentBitset,
                &isParentBitSetChangedNeedReEval,
                &m_DefAsgAllocator);


            if (isParentBitSetChangedNeedReEval)
            {
                m_DefAsgCurrentBitset->setInto(parentBlock->outDefAsgBitset, &m_DefAsgAllocator);
                if (parentBlock->Next != NULL)
                {
                    // the parent block USING/WITH is reachable or has a following statement
                    return parentBlock->Next;
                }
                else
                {
                    return DefAsgUpdateParentGetContinuation(parentBlock, ClosureDepth);
                }
            }
        break;
        }


    default:
        VSFAIL("Definite assignment eval: Unexpected stmtkind.");
    }
    return NULL;
}

ILTree::TryBlock* Semantics::DefAsgUpdateTryGroupOutBitSet(ILTree::PILNode ptree, DWORD_PTR *changed)
{
    // Builds the out-bit set of the group which is:
    //    OR( out-bitset-finally, AND(out-bitset-try,out-bitset-catch1,out-bitset-catch2..))
    //    out-bitset = AND(out-bitset-try,out-bitset-catch1,out-bitset-catch2..)   ( if finally is missing)
    // the assumption is the blocks are in the proper order: try, catch, catch, finally
    VSASSERT(ptree->bilop == SB_TRY || ptree->bilop == SB_CATCH || ptree->bilop == SB_FINALLY,
        "Definite assignment: improper call to update try group bitset");
    ILTree::PILNode pStmtCur;

    // go back to identify 'Try' statement
    pStmtCur = GetTryBlockInTryGroup(ptree);

    ILTree::TryBlock* pTry = &pStmtCur->AsTryBlock();
    *changed = 0;

    m_DefAsgTempBitset = m_DefAsgTempBitset->setInto(BITSET_ALL);
    bool TryOrCatchEndReachable = false;

    //AND part of the operation
    for ( /* pStmtCur = ptry */;
        (pStmtCur != NULL) && (pStmtCur == pTry || pStmtCur->bilop == SB_CATCH ) ;
        pStmtCur = pStmtCur->AsStatement().Next)
    {
        if (pStmtCur->AsExecutableBlock().outDefAsgBitset)
        {
            VSASSERT( pStmtCur->bilop != SB_CATCH || !(pStmtCur->uFlags & SBF_OVERLAPED_CATCH),
                    " How can an overlaped Catch block have an output bit set");

            m_DefAsgTempBitset = m_DefAsgTempBitset->andInto(pStmtCur->AsExecutableBlock().outDefAsgBitset);
            TryOrCatchEndReachable = true;
        }
    }

    if (!TryOrCatchEndReachable ||
        (pStmtCur != NULL && pStmtCur->bilop == SB_FINALLY && !pStmtCur->AsExecutableBlock().outDefAsgBitset))
    {
        //      1. or 2.
        // 1. none of the try/catch block has the end reachable. Throw, goto's or partial evaluation can be the reason.
        //    No mater a finally block exists or not, the statement following the try block is not reachable (yet)
        // 2. some of the try/cath block is reachable but here is a finally which has the end unreachable(a throw or partial evaluation).
        //    Again, the statement following the try block is not reachable (yet)

        // * changed = 0
        return pTry;
    }

    // OR operation (if there is a finally)
    if (pStmtCur != NULL && pStmtCur->bilop == SB_FINALLY)
    {
        VSASSERT(pStmtCur->AsExecutableBlock().outDefAsgBitset, "based on the condition above, the finally out bitset cannot be null");
        m_DefAsgTempBitset = m_DefAsgTempBitset->orInto(pStmtCur->AsExecutableBlock().outDefAsgBitset);
    }

    // copy the result in the out bitset of the try group
    pTry->outDefAsgBitsetTryGroup =
        pTry->outDefAsgBitsetTryGroup->andIntoOrCopy(
            m_DefAsgTempBitset,
            changed,
            &m_DefAsgAllocator);

    return pTry;
}

bool Semantics::ContainsStatement(ILTree::ExecutableBlock* block, ILTree::PILNode targetNode)
{
    for (ILTree::ExecutableBlock* parentNode = targetNode->AsStatement().Parent;
         parentNode;
         parentNode = parentNode->Parent)
    {
        if (block == parentNode)
        {
            return true;
        }
    }
    return false;
}

bool Semantics::IsInSyntheticTryGroup(ILTree::PILNode ptree)
{
    VSASSERT(ptree->IsStmtNode() &&
        (ptree->bilop == SB_TRY ||ptree->bilop == SB_CATCH || ptree->bilop == SB_FINALLY),
        "IsInSyntheticTryGroup can be called only on try, catch, finally nodes");
    ILTree::PILNode pStmtCur = GetTryBlockInTryGroup(ptree);
    return (pStmtCur ? (!(pStmtCur->uFlags & SBF_TRY_NOT_SYNTHETIC)) : false);
}

bool Semantics::IsLastNonOverlapedCatchInTheGroup(ILTree::PILNode ptree)
{
    VSASSERT(ptree && ptree->IsStmtNode() && ptree->bilop == SB_CATCH && !(ptree->uFlags & SBF_OVERLAPED_CATCH),
        "bad invocation of IsLastNonOverlapedCatchInTheGroup()");
    ILTree::PILNode pStmtCur;
    for ( pStmtCur = ptree;
          pStmtCur;
          pStmtCur = pStmtCur->AsStatement().Next)
    {
        if ( pStmtCur->AsStatement().Next &&
             pStmtCur->AsStatement().Next->bilop == SB_CATCH &&
             (pStmtCur->AsStatement().Next->uFlags & SBF_OVERLAPED_CATCH))
        {
            // skip overlaped catch blocks
            continue;
        }
        return (!pStmtCur->AsStatement().Next ||
                 (pStmtCur->AsStatement().Next->bilop != SB_CATCH &&
                    pStmtCur->AsStatement().Next->bilop != SB_FINALLY));
    }
    VSFAIL("unreachable code");
    return false;
}

ILTree::PILNode Semantics::GetTryBlockInTryGroup(ILTree::PILNode ptree)
{
    VSASSERT(ptree && ptree->IsStmtNode() &&
        (ptree->bilop == SB_TRY ||ptree->bilop == SB_CATCH || ptree->bilop == SB_FINALLY),
        "GetTryBlockInTryGroup can be called only on try, catch, finally nodes");
    ILTree::PILNode pStmtCur;
    for ( pStmtCur = ptree;
          pStmtCur->bilop != SB_TRY;
          pStmtCur = pStmtCur->AsStatement().Prev)
    {
        VSASSERT(pStmtCur->AsStatement().Prev &&
            (pStmtCur->AsStatement().Prev->bilop == SB_TRY || pStmtCur->AsStatement().Prev->bilop == SB_CATCH) ,
            "Definite assignment: bad try group sequence");
    }
    return pStmtCur;
}

ILTree::PILNode Semantics::GetFinallyBlockInTryGroup(ILTree::PILNode ptree)
{
    VSASSERT(ptree && ptree->IsStmtNode() &&
        (ptree->bilop == SB_TRY ||ptree->bilop == SB_CATCH || ptree->bilop == SB_FINALLY),
        "GetFinallyBlockInTryGroup can be called only on try, catch, finally nodes");

    // if start at a try block and the pointer to finally is cached use it
    if(ptree->bilop == SB_TRY)
    {
        return ptree->AsTryBlock().FinallyTree;
    }

    // search the finally block
    ILTree::PILNode pStmtCur;
    for (  pStmtCur = ptree;
            pStmtCur != NULL;
            pStmtCur = pStmtCur->AsStatement().Next)
    {
        if (pStmtCur->bilop == SB_FINALLY)
        {
            return pStmtCur;
        }

        if (pStmtCur->bilop == SB_CATCH)
        {
            continue;   // still inside the block
        }
        else
        {
            // 2 Cases:
            // Case 1:
            //      pStmtCur->bilop == SB_TRY
            //      new 'Try' block encountered without encountering a finally. So
            //      previous 'Try' has ended.
            // Case 2:
            //      pStmtCur->bilop != SB_CATCH && pStmtCur->bilop != SB_TRY
            //      reached some other node , no finally at all

            return NULL;
        }
    }
    return NULL;    // reached the end of the enclossing block, no finally at all
}

//Microsoft LIST and SEQ should not be processed recursively. Otherwise
//long sequences run the risk of causing stack overflow unnecessarily.
void Semantics::IterativelyCheckRightSkewedTree(
    ILTree::PILNode ptree,
    unsigned int ClosureDepth)
{
    const BILOP SeqType = ptree->bilop;
    AssertIfFalse(SeqType == SX_SEQ || SeqType == SX_LIST);

    while (true)
    {
        DefAsgCheckUse(ptree->AsExpressionWithChildren().Left, ClosureDepth);

        ILTree::PILNode right = ptree->AsExpressionWithChildren().Right;

        if (right)
        {
            if (right->bilop == SeqType)
            {
                if (!m_DefAsgIsReachableCode)
                {
                    return;
                }
                else
                {
                    ptree = right;
                    DBG_SWITCH_PRINTF(fDumpFlow, L"%*c  CheckUse %S\n", ClosureDepth*2, ' ', ILTree::BilopName(ptree->bilop));
                }
            }
            else
            {
                DefAsgCheckUse(ptree->AsExpressionWithChildren().Right, ClosureDepth);
                break;
            }
        }
        else
        {
            break;
        }
    }
}


void Semantics::DefAsgCheckUse
(
    ILTree::PILNode ptree,
    unsigned int ClosureDepth
)
{
    if (!ptree || !m_DefAsgIsReachableCode)
    {
        return;
    }

    DBG_SWITCH_PRINTF(fDumpFlow, L"%*c  CheckUse %S\n", ClosureDepth*2, ' ', ILTree::BilopName(ptree->bilop));

    switch (ptree->bilop)
    {

    case SX_SYM:
        {
            if (ptree->AsSymbolReferenceExpression().pnamed->IsVariable())
            {
                BCSYM_Variable * pvar = ptree->AsSymbolReferenceExpression().pnamed->DigThroughAlias()->PNamedRoot()->PVariable();
                SetLocalIsUsed(pvar);
                bool isStruct = (pvar->GetType()->GetVtype() == t_struct);

                // Microsoft 2009.06.23 -- consider extending this to also do the analysis for primitive value-types and for structs
                // whose members are all structs. They'll need different error messages of course (since they won't result in null
                // reference exceptions).
                ERRID errid;
                if (ptree->uFlags & SXF_SYM_PASSEDBYREF)
                {
                    // "Variable '|1' is passed by reference before it has been assigned a value. A null reference exception could result at runtime. Make sure the structure or all the reference members are initialized before use"
                    // "Variable '|1' is passed by reference before it has been assigned a value. A null reference exception could result at runtime."
                    errid = (isStruct ? WRNID_DefAsgUseNullRefByRefStr : WRNID_DefAsgUseNullRefByRef);
                }
                else
                {
                    // "Variable '|1' is used before it has been assigned a value. A null reference exception could result at runtime. Make sure the structure or all the reference members are initialized before use"
                    // "Variable '|1' is used before it has been assigned a value. A null reference exception could result at runtime."
                    errid = (isStruct ? WRNID_DefAsgUseNullRefStr : WRNID_DefAsgUseNullRef);
                }

                ILTree::Expression *brr = ptree->AsSymbolReferenceExpression().BaseReference;
                if (ptree->AsSymbolReferenceExpression().BaseReference == NULL)
                {
                    if (pvar->GetDefAsgSlot())
                    {
                        VSASSERT(pvar->GetVarkind() == VAR_Local || pvar->GetVarkind() == VAR_FunctionResult, "Why a non local has defasg slot");
                        if (ptree->uFlags & SXF_SYM_ASGVIACALL)
                        {
                            // this is actually an assignment not an use
                            // for now, it is happening for structures: 'dim s as new struct(a)' is morphed into
                            // 'call struct ctor(s,a)

                            DefAsgSet(ptree, ClosureDepth);
                        }
                        else if (!m_DefAsgCurrentBitset->testBits(pvar->GetDefAsgSlot(), pvar->GetDefAsgSize()) &&
                            !m_DefAsgErrorDisplayedBitset->testBits(pvar->GetDefAsgSlot(), pvar->GetDefAsgSize()))
                            
                        {
                              
                            if (pvar->GetVarkind() == VAR_FunctionResult && TypeHelpers::IsPrimitiveValueType(pvar->GetType()))
                            {

                                // Dev11 16156
                                // The implicit function return variable is a special case. Its DefAsgSet bit is allocated so we can give warnings if not all
                                // paths return a value (even for primitive types). But, like all primitive types, we don't want to give a warning
                                // if it's passed uninitialized to a ByRef parameter. (On the other hand, note that the CONSEQUENCE of being passed
                                // to a ByRef parameter is that, after the call, the variable is assumed "assigned"). Hence our fix here: if
                                // the implicit function return variable is passed ByRef, then we'll "just-in-time" mark it as assigned, just before checking
                                // to see whether we need to give the warning about passing an unassigned variable to ByRef.
                                //
                                // The whole design around when to warn is a bit messy. Here's a summary:
                                //
                                // PASS A LOCAL TO A BYREF BEFORE IT HAS BEEN INITIALIZED?
                                // Primitive, or struct containing only primitive: no warning. (since there'd be too much noise, e.g. Dim b As Boolean: TryParse(b))
                                // Reftype, or struct containing ref: warning
                                //
                                // RETURN THE IMPLICIT RETURN LOCAL BEFORE IT HAS BEEN INITIALIZED?
                                // Primitive: warning BC42353 - return of primitive before it has been initialized
                                // Reference: warning BC42105 - return of reference type before it has been initialized; may cause null-reference exception
                                // Struct: no warning. (This is inconsistent. 






                                
                                DefAsgSet(ptree, ClosureDepth);
                            }
                            else 
                            {
                                DBG_SWITCH_PRINTF(fDumpFlow, L"  *USE BEFORE ASSIGNMENT: %s [slot=%u size=%u]\n", pvar->GetName(), pvar->GetDefAsgSlot(), pvar->GetDefAsgSize());
                                ReportSemanticError(
                                    errid,
                                    ptree->Loc, pvar);

                                m_DefAsgErrorDisplayedBitset = m_DefAsgErrorDisplayedBitset->setBits(pvar->GetDefAsgSlot(), pvar->GetDefAsgSize());
                            }

                        }

                    }
                }
                else if (IsFieldAccessInLocalOfStructType(ptree))
                {
                    //  use of a field structure s1.s2.s3.s3.a
                    // identify the m_slot and the size of 'a'
                    // the local is s1, and it gets fully definite assigned when all of its components(recursivelly) are assigned
                    unsigned slot=0;
                    unsigned size = 0;
                    if ((slot= DefAsgGetSlotOfFieldStructExpr(ptree)))
                    {
                        if ((size = DefAsgTypeSize(ptree)))
                        {
                            if (!m_DefAsgCurrentBitset->testBits(slot, size) &&
                                !m_DefAsgErrorDisplayedBitset->testBits(slot,size))
                            {
                                ReportSemanticError(
                                    errid,
                                    ptree->Loc, pvar);

                                m_DefAsgErrorDisplayedBitset = m_DefAsgErrorDisplayedBitset->setBits(slot, size);

                            }
                        }
                    }
                }
                else
                {
                    DefAsgCheckUse(ptree->AsSymbolReferenceExpression().BaseReference, ClosureDepth);
                }
            }
        }
        break;

    case SX_ADR:
        {
            DefAsgCheckUse(ptree->AsExpressionWithChildren().Left, ClosureDepth);
            break;
        }

    case SX_NOTHING:
    case SX_CNS_INT:
    case SX_CNS_FLT:
    case SX_CNS_DEC:
    case SX_METATYPE:
    case SX_IF:
    case SX_SYNCLOCK_CHECK:
        break;

    case SX_INIT_STRUCTURE:
        if ( ptree->AsInitStructureExpression().StructureReference->bilop == SX_ADR &&
             ptree->AsInitStructureExpression().StructureReference->AsExpressionWithChildren().Left->bilop == SX_SYM)
        {
            DefAsgSet(ptree->AsInitStructureExpression().StructureReference->AsExpressionWithChildren().Left, ClosureDepth);
        }
        break;

    //        /*
    //         * Array expressions
    //         */
    case SX_INDEX:
    case SX_VARINDEX:
        {
            ILTree::PILNode   ptreeOrVar;
            ILTree::PILNode   ptreeListCursor;
            if (ptree->bilop == SX_INDEX)
            {
                ptreeOrVar = ptree->AsIndexExpression().Left;
                ptreeListCursor = ptree->AsIndexExpression().Right;
            }
            else
            {
                ptreeOrVar = ptree->AsVariantIndexExpression().Left;
                ptreeListCursor = ptree->AsVariantIndexExpression().Right;
            }

            DefAsgCheckUse(ptreeOrVar, ClosureDepth);

            while (ptreeListCursor)
            {
                DefAsgCheckUse(ptreeListCursor, ClosureDepth);
                ptreeListCursor = ptreeListCursor->AsExpressionWithChildren().Right;
            } // while
            break;
        }

    //        /*
    //         * Assignment/coercions expressions
    //         */

    // In general assignment in expressions are considered temp asignments. No local should be in the left.
    // But with the introduction of object initializers (i.e. Aggregate initializers), this is no longer true.
    // So both the left and right operands of the assignment operator need to be considered.
    // 
    case SX_ASG:
    case SX_ASG_RESADR:
        DefAsgSetOrCheckForUse(ptree->AsExpressionWithChildren().Left, ClosureDepth);
        DefAsgCheckUse(ptree->AsExpressionWithChildren().Right, ClosureDepth);
        break;

    case SX_CTYPE:
    case SX_DIRECTCAST:
    case SX_TRYCAST:
    case SX_WIDE_COERCE:
    case SX_ARRAYLEN:
    case SX_ISTYPE:
        DefAsgCheckUse(ptree->AsExpressionWithChildren().Left, ClosureDepth);
        break;



    //        /*
    //         * Instantiation expressions
    //         */
    case SX_NEW:
        {
            ILTree::PILNode ptreeCall = ptree->AsNewExpression().ConstructorCall;
            if (!ptreeCall)
            {
            }
            else if (ptreeCall->bilop == SX_DELEGATE_CTOR_CALL)
            {
                DefAsgCheckUse(ptreeCall->AsDelegateConstructorCallExpression().ObjectArgument, ClosureDepth);
            }
            else
            {
                VSASSERT(ptreeCall->bilop == SX_CALL, "Definite assignment: must have a call node here");
                DefAsgCheckUse(ptreeCall, ClosureDepth);
            }
            break;
        }

    case SX_NEW_ARRAY:
        DefAsgCheckUse(ptree->AsExpressionWithChildren().Left, ClosureDepth);        // the size expression can contain locals
        break;

    case SX_IIF:
        DefAsgCheckUse(ptree->AsIfExpression().condition, ClosureDepth);
        DefAsgCheckUse(ptree->AsIfExpression().Left, ClosureDepth);
        DefAsgCheckUse(ptree->AsIfExpression().Right, ClosureDepth);
        break;

    case SX_IIFCoalesce:
        DefAsgCheckUse(ptree->AsCoalesceExpression().Left, ClosureDepth);
        DefAsgCheckUse(ptree->AsCoalesceExpression().Right, ClosureDepth);
        break;

    case SX_CTYPEOP:
        DefAsgCheckUse(ptree->AsLiftedCTypeExpression().Left, ClosureDepth);
        break;


    //        /*
    //         * Call expressions
    //         */
    case SX_CALL:
        {
            ILTree::PILNode pinitializedStruct = NULL;
            if (ptree->AsCallExpression().ptreeThis)
            {
                // Make definite assignment handle the special case of initializing structures for the
                // scenario in bug VSWhidbey 270278.
                //
                if (ptree->AsCallExpression().ptreeThis->bilop == SX_ADR &&
                    ptree->AsCallExpression().ptreeThis->AsExpressionWithChildren().Left->bilop == SX_SYM &&
                    ptree->AsCallExpression().Left->bilop == SX_SYM &&
                    ptree->AsCallExpression().Left->AsSymbolReferenceExpression().pnamed->IsProc() &&
                    ptree->AsCallExpression().Left->AsSymbolReferenceExpression().pnamed->PProc()->IsInstanceConstructor() &&
                    ptree->AsCallExpression().Left->AsSymbolReferenceExpression().pnamed->PProc()->GetParent()->IsStruct())
                {
                    pinitializedStruct = ptree->AsCallExpression().ptreeThis->AsExpressionWithChildren().Left;
                }
                else
                {
                    DefAsgCheckUse(ptree->AsCallExpression().ptreeThis, ClosureDepth);
                }
            }
            ILTree::PILNode ptreeList = ptree->AsCallExpression().Right;
            while (ptreeList)
            {
                VSASSERT(ptreeList->bilop == SX_LIST,
                    "Definite assignment: each arg must be SX_LIST");
                DefAsgCheckUse(ptreeList->AsExpressionWithChildren().Left, ClosureDepth);
                ptreeList = ptreeList->AsExpressionWithChildren().Right;
            } // while ptreeList

            // Make definite assignment handle the special case of initializing structures for the
            // scenario in bug VSWhidbey 270278.
            //
            if (pinitializedStruct)
            {
                DefAsgSet(pinitializedStruct, ClosureDepth);
            }

            break;
        }

    case SX_LATE:
        DefAsgCheckUse(ptree->AsLateBoundExpression().Left, ClosureDepth);       // object - could be a local
        DefAsgCheckUse(ptree->AsLateBoundExpression().Right, ClosureDepth);       // arguments: SX_LIST
        break;

    //        /*
    //         * Operator expressions
    //         */


    // Binary operators
    case SX_POW:
    case SX_NEG:
    case SX_PLUS:
    case SX_DIV:
    case SX_MOD:
    case SX_IDIV:
    case SX_CONC:
    case SX_NOT:
    case SX_AWAIT:
    case SX_AND:
    case SX_OR:
    case SX_XOR:
    case SX_MUL:
    case SX_ADD:
    case SX_SUB:
    case SX_SHIFT_LEFT:
    case SX_SHIFT_RIGHT:
    // Relational operators
    case SX_IS:
    case SX_ISNOT:
    case SX_EQ:
    case SX_NE:
    case SX_LE:
    case SX_LT:
    case SX_GE:
    case SX_GT:
    case SX_LIKE:
    case SX_ANDALSO:
    case SX_ORELSE:
        DefAsgCheckUse(ptree->AsExpressionWithChildren().Left, ClosureDepth);
        DefAsgCheckUse(ptree->AsExpressionWithChildren().Right, ClosureDepth);
        break;

    case SX_LIST:
        IterativelyCheckRightSkewedTree(ptree, ClosureDepth);
        break;
    case SX_SEQ:
        IterativelyCheckRightSkewedTree(ptree, ClosureDepth);
        break;

    case SX_SEQ_OP1:
    case SX_SEQ_OP2:
    case SX_ISTRUE:
    case SX_ISFALSE:
        DefAsgCheckUse(ptree->AsExpressionWithChildren().Left, ClosureDepth);
        if (ptree->AsExpressionWithChildren().Right)
        {
            DefAsgCheckUse(ptree->AsExpressionWithChildren().Right, ClosureDepth);
        }
        break;
    case SX_LAMBDA:
        if (ptree->AsLambdaExpression().IsStatementLambda)
        {
            // CLOSURE STRATEGY (Microsoft)
            // Here we are evaluating a closure expression, specifically a multiline lambda closure, inside
            // the context of the procedure that created it, to find unused and unassigned variables.
            // (Later once the closure has been lifted to a _Lambda$__ method, we'll evaluate its
            // SB_STATEMENT_LAMBDA block in DefAsgEvalBlock, solely to check that a return has been performed
            // if needed).
            // Anyway, here we are evaluating the statement-list inside a lambda expression. Consider
            // these things:
            //   * m_DefAsgCurrentBitset will be updated within the statement-list as variables are
            //     assigned. Note that m_DefAsgCurrentBitset includes slots for all variables within
            //     this lambda, and for the parent procedure, and indeed for every other lambda
            //     in the parent procedure.
            //   * After the lambda-expression has finished, we restore m_DefAsgCurrentBitset to
            //     what it was before the lambda. That's because what comes after the lambda expression
            //     can't rely on the lambda having been executed.
            //   * Note that m_DefAsgCurrentBitset[return of procedure] exists. If the lambda had any
            //     return statements, the Eval routines will ignore them because they only respect
            //     return statements when ClosureDepth == 0
            //   * If there were any Exit statements within the lambda they
            //     will also be ignored, because the Eval routines only respect them when ClosureDepth == 0.
            //   * Exceptions are easier. A Throw (SL_ERROR) merely sets m_DefAsgReachable=false.
            //     The things that set m_DefAsgReachable=true are the ends of Try/Catch/Finally blocks,
            //     and also the end of a multiline-lambda.
            //   * Note that m_DefAsgCurrentBitset[return of lambda] does NOT exist. That's because
            //     "return_of_lambda" only exists as part of the SB_STATEMENT_LAMBDA block that's handled
            //     by DefAsgEvalBlock; it doesn't exist here.
            //
            // Consider what happens if inside the lambda there was a block that had to be re-evaluated
            // because of a flow cycle. That doesn't matter, since the correct bitset and ClosureDepth
            // will be stored with the re-eval task; and it won't affect the statements that followed the lambda.
            //
            // Consider what happens if the lambda is contained in a block that had to be re-evaluated.
            // Again it doesn't matter: we'll just repeat the following code.

            ILTree::StatementLambdaBlock *lambdaBlock = ptree->AsLambdaExpression().GetStatementLambdaBody();
            lambdaBlock->inDefAsgBitset = lambdaBlock->inDefAsgBitset->setInto(m_DefAsgCurrentBitset, &m_DefAsgAllocator);
            BITSET *bitsetCopy = m_DefAsgCurrentBitset->createCopy(m_DefAsgCurrentBitset, &m_DefAsgAllocator);
            DBG_SWITCH_PRINTF(fDumpFlow, L"%*c  .multiline lambda start  {CURBITSET=%08x}  {INBITSET=%08x}\n", ClosureDepth*2, ' ', m_DefAsgCurrentBitset, lambdaBlock->inDefAsgBitset);

            DefAsgEvalStatementList(lambdaBlock->ptreeChild, ClosureDepth+1);

            m_DefAsgCurrentBitset = bitsetCopy;  // copying it back
            DBG_SWITCH_PRINTF(fDumpFlow, L"%*c  .multiline lambda end    {CURBITSET=%08x}\n", ClosureDepth*2, ' ', m_DefAsgCurrentBitset);
            m_DefAsgIsReachableCode = true;
        }
        else
        {
            DefAsgCheckUse(ptree->AsLambdaExpression().GetExpressionLambdaBody(), ClosureDepth);
        }
        break;
    case SX_ARRAYLITERAL:
        DefAsgCheckUse(ptree->AsArrayLiteralExpression().ElementList, ClosureDepth);
        break;
    case SX_NESTEDARRAYLITERAL:
        DefAsgCheckUse(ptree->AsNestedArrayLiteralExpression().ElementList, ClosureDepth);
        break;
    case SX_COLINIT:
        DefAsgCheckUse(ptree->AsColInitExpression().NewExpression, ClosureDepth);
        DefAsgCheckUse(ptree->AsColInitExpression().Elements, ClosureDepth);
        break;
    case SX_COLINITELEMENT:
        DefAsgCheckUse(ptree->AsColInitElementExpression().CallExpression, ClosureDepth);
        break;
    case SX_ANONYMOUSTYPE:
        {
            ILTree::AnonymousTypeExpression& anonymousType = ptree->AsAnonymousTypeExpression();
            for( ULONG i = 0; i < anonymousType.BoundMembersCount; i += 1 )
            {
                DefAsgCheckUse(anonymousType.BoundMembers[i].BoundExpression, ClosureDepth);
            }
        }
        break;
    case SX_CNS_STR:
    case SX_DEFERRED_TEMP:
        break;
    default:
        // This assert is bad : errors may escape down here.
        //VSFAIL("CheckDefAsgFlow: unexpected SX node");
        ptree->bilop; // it's this bilop that's unknown.
        //VbThrow(HrMake(ERRID_CodegenError)); But we won't throw. Flow analysis is non-critical.
        break;
    }

}

// out of try/catch goto's triger the execution of all the finally block on the way up thy try structures
// to the target label.
// Theoretically the current bitset should be considered in the in set of the first finally block. Then the
// out set of the first finally block should be considered in the in set of the next finally to execute.
// For VB language case a simplified approach can be used:
// All the blocks in a try group have as in set the in set of the try block. And no goto into try or catch
// blocks are allowed. Therefore, a goto bitset will not change the in bitset of a finally block.
// Consequently, the finally blocks are evaluated ahead of try and catch blocks. The in/out sets are not caried
// over among finally blocks.
// The try nested structures are crawled up. The output of each finally block that is visited is 'or'-ed in the set of the target
// label node. On the way, the fixup lists of each finally block get updated with the goto statement.
//
void Semantics::UpdateLabelOrBlockAcrossTryCatch( ILTree::PILNode gotoOrExitNode, bool updateFinallyFixups, unsigned int ClosureDepth)
{
    VSASSERT( gotoOrExitNode->bilop == SL_GOTO || gotoOrExitNode->bilop == SL_EXIT ||
        (gotoOrExitNode->bilop == SL_RETURN && HasFlag32(gotoOrExitNode, SLF_RET_FROM_EXIT)),
        "Definite assignment: expected GOTO, RETURN or EXIT node.");
    bool targetIsReachable = true;
    DWORD_PTR isTargetBitSetChanged = 0;
    ILTree::PILNode targetStmt = (gotoOrExitNode->bilop == SL_GOTO) ?
        gotoOrExitNode->AsGotoStatement().Label->pLabelParent :
        (gotoOrExitNode->bilop == SL_RETURN ? m_ProcedureTree : gotoOrExitNode->AsExitStatement().ExitedStatement);

    VSASSERT( targetStmt->bilop == SL_LABEL || targetStmt->bilop == SB_SELECT ||
        targetStmt->bilop == SB_LOOP || targetStmt->bilop == SB_FOR || targetStmt->bilop == SB_FOR_EACH ||
        targetStmt->bilop == SB_PROC,
        "Definite assignment: Expected LABEL node or target of an EXIT(except TRY)");

    VSASSERT( targetStmt->AsStatement().Parent != gotoOrExitNode->AsStatement().Parent,
        "A cross-try goto/select cannot target a statement in the same block as goto/select");

    m_DefAsgTempBitset = (gotoOrExitNode->bilop == SL_GOTO) ?
        m_DefAsgTempBitset->setInto(gotoOrExitNode->AsGotoStatement().outDefAsgBitset) :
        (gotoOrExitNode->bilop == SL_RETURN ?
            m_DefAsgTempBitset->setInto(gotoOrExitNode->AsReturnStatement().outDefAsgBitset):
            m_DefAsgTempBitset->setInto(gotoOrExitNode->AsExitStatement().outDefAsgBitset));

	ILTree::ExecutableBlock* parent;
    for (parent = gotoOrExitNode->AsStatement().Parent;
        parent;
        parent = parent->Parent)
    {
        if (ContainsStatement(parent, targetStmt) ||
           (parent->bilop == SB_CATCH &&        // also stop if it is jump from a catch into the associate try
            ContainsStatement(&GetTryBlockInTryGroup(parent)->AsTryBlock(),targetStmt)))
        {
            break;   // normal termination of the for
        }
        else
        {
            if ((parent->bilop == SB_TRY || parent->bilop == SB_CATCH) &&
                !IsInSyntheticTryGroup(parent) )         // synthetic try structures are ignored
            {
                VSASSERT( parent->bilop != SB_CATCH || !(parent->uFlags & SBF_OVERLAPED_CATCH),
                    " How a goto can cross from inside an overlaped Catch block");
                ILTree::PILNode finBlk = GetFinallyBlockInTryGroup(parent);
                if (finBlk)
                {
                    ILTree::FinallyBlock &finallyBlock = finBlk->AsFinallyBlock();
                    VSASSERT(finallyBlock.inDefAsgBitset, "The finally block of an outer try is supossed to be already evaluated");
                    if (updateFinallyFixups)
                    {
                        finallyBlock.m_DefAsgGoToFixupList.AddReEvalStmt(gotoOrExitNode,ClosureDepth,&m_DefAsgAllocator);
                    }
                    if (targetIsReachable)
                    {
                        if ( finallyBlock.outDefAsgBitset )
                        {
                            //OR the out bitset
                            m_DefAsgTempBitset = m_DefAsgTempBitset->orInto(finallyBlock.outDefAsgBitset);
                        }
                        else
                        {
                            // this finally block has the end of the block unreachable (possible due to a
                            // throw or just not reachable yet). The chain is broken. Or, a previous finally broke it.
                            // all the goto's going through this block will not reach their target(yet)
                            // make sure the target label node remains unchaged.
                            // Continue the crawl only to do the fixup if any. Fix-up lists will be used later to evaluate the
                            // goto's later when the finally block gets completed.
                            targetIsReachable = false;
                            if (!updateFinallyFixups)
                            {
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    VSASSERT(parent != 0 || (parent == 0 && targetStmt->bilop == SB_PROC),
        " Abnormal ending of an update label/block across try/catch");
    if (targetIsReachable)
    {
        if (targetStmt->bilop == SL_LABEL )
        {
            targetStmt->AsLabelStatement().inDefAsgBitset =
                targetStmt->AsLabelStatement().inDefAsgBitset->andIntoOrCopy(
                m_DefAsgTempBitset,
                &isTargetBitSetChanged,
                &m_DefAsgAllocator);
        }
        else
        {
            targetStmt->AsExecutableBlock().outDefAsgBitset =
                targetStmt->AsExecutableBlock().outDefAsgBitset->andIntoOrCopy(
                m_DefAsgTempBitset,
                &isTargetBitSetChanged,
                &m_DefAsgAllocator);

            //check return variable
            if ((gotoOrExitNode->bilop == SL_RETURN || gotoOrExitNode->bilop == SL_EXIT) && // this condition should be SL_RETURN only. Keep SL_EXIT as well for now.
                    targetStmt->bilop == SB_PROC &&
                    m_ProcedureTree->AsProcedureBlock().ReturnVariable  &&
                    m_ProcedureTree->AsProcedureBlock().ReturnVariable->GetDefAsgSlot() &&
                    !m_ProcedureTree->AsProcedureBlock().fDefAsgNoRetValReported &&
                    ClosureDepth == 0)
            {
                BCSYM_Variable * pRetVar = m_ProcedureTree->AsProcedureBlock().ReturnVariable;
                if (!m_DefAsgTempBitset->testBits(pRetVar->GetDefAsgSlot(), pRetVar->GetDefAsgSize()))
                {
                    m_ProcedureTree->AsProcedureBlock().fDefAsgNoRetValReported = true;
                    ReportDefAsgNoRetVal(m_Procedure,
                                         pRetVar->GetType(),
                                         gotoOrExitNode->Loc,
                                         m_Procedure->IsUserDefinedOperatorMethod() ?
                                            m_Procedure->GetAssociatedOperatorDef()->GetName() :
                                            m_Procedure->GetErrorName(m_Compiler));

                }

            }

        }

        if (isTargetBitSetChanged)
        {
            m_DefAsgReEvalStmtList.AddReEvalStmt(targetStmt, ClosureDepth, &m_DefAsgAllocator);
        }
    }
}

// first tries to set a variable (if any) of this tree as assigned
// if set fails, then consider the tree as an use
// used in constucts like:
// tree = expr
// redim tree
//


void Semantics::DefAsgSetOrCheckForUse(ILTree::PILNode ptree, BITSET   *&bitSet, bool onlySet, unsigned int ClosureDepth)
{
    VSASSERT( onlySet || bitSet == m_DefAsgCurrentBitset,
        "Bad use of DefAsgSetOrCheckForUse, check is done only on m_DefAsgCurrentBitset which is different than bitSet");
    if (ptree->bilop == SX_SYM &&
        ptree->AsSymbolReferenceExpression().pnamed->DigThroughAlias()->PNamedRoot()->IsVariable())
    {
        BCSYM_Variable * pvar = ptree->AsSymbolReferenceExpression().pnamed->DigThroughAlias()->PNamedRoot()->PVariable();
        SetLocalIsUsed(pvar);

        if (ptree->AsSymbolReferenceExpression().BaseReference == NULL)
        {
            // simple assignement: v = expr
            if (pvar->GetDefAsgSlot())
            {
                VSASSERT(pvar->GetVarkind() == VAR_Local || pvar->GetVarkind() == VAR_FunctionResult, "Why a non local has defasg slot");
                bitSet = bitSet->setBits(pvar->GetDefAsgSlot(), pvar->GetDefAsgSize());
#pragma warning (suppress:4302)
                DBG_SWITCH_PRINTF(fDumpFlow, L"  *Assigned: %s @%08x [slot:%u size:%u]  {CURBITSET=%08x}\n", pvar->GetName(), PtrToUlong(pvar), pvar->GetDefAsgSlot(), pvar->GetDefAsgSize(), m_DefAsgCurrentBitset);
                VSASSERT(pvar == m_DefAsgSlotAssignments.Element(pvar->GetDefAsgSlot()), "error: we just assigned a variable into a slot not intended for it. This will lead to errors.");
            }
        }
        else if (IsFieldAccessInLocalOfStructType(ptree))
        {
            // field structure assignemnt s1.s2.s3.s3.a = expr
            // the local is s1, and it gets fully definite assigned when all of its components(recursivelly) are assigned
            unsigned slot=0;
            unsigned size = 0;
            if ((slot= DefAsgGetSlotOfFieldStructExpr(ptree)))
            {
                if ((size = DefAsgTypeSize(ptree)))
                {
                    bitSet = bitSet->setBits(slot, size);
                    DBG_SWITCH_PRINTF(fDumpFlow, L"  *Assigned: <struct> [slot:%u size:%u]  {CURBITSET=%08x}\n", slot, size, m_DefAsgCurrentBitset);
                }
            }
        }
        else if (!onlySet)
        {
            // if is not simple assignment or a structure field assignement, then it is a use of the base
            DefAsgCheckUse(ptree->AsSymbolReferenceExpression().BaseReference, ClosureDepth);
        }
    }
    else if (!onlySet)
    {
        DefAsgCheckUse(ptree, ClosureDepth);
    }
}
bool Semantics::DefAsgIsValidMember(BCSYM_NamedRoot *pnamed)
{
    return (
        pnamed->IsVariable() &&
        // look into bad members of metadata structs; they can be of struct type: b196060
        (!pnamed->IsBad() || (pnamed->GetParent()->IsStruct() && Bindable::DefinedInMetaData(pnamed->GetParent()->PClass())))&&
        !pnamed->PVariable()->IsShared() &&
        !pnamed->PVariable()->CreatedByEventDecl() &&
        !pnamed->PVariable()->IsConstant());
}

unsigned Semantics::DefAsgTypeSize(ILTree::PILNode ptree)
{
    VSASSERT(ptree->bilop == SX_SYM , "why DefAsgTypeSize is called for a non symbol tree?");
    if (ptree->bilop != SX_SYM)
    {
        return 0;
    }

    if (ptree->AsSymbolReferenceExpression().defAsgSizeCache == 0)
    {
        if (!DefAsgIsValidMember(ptree->AsSymbolReferenceExpression().pnamed->DigThroughAlias()->PNamedRoot()))
        {
            ptree->AsSymbolReferenceExpression().defAsgSizeCache = 1;
            return 0;
        }
        ptree->AsSymbolReferenceExpression().defAsgSizeCache = DefAsgTypeSize(ptree->AsSymbolReferenceExpression().ResultType) + 1;
    }
    return ptree->AsSymbolReferenceExpression().defAsgSizeCache - 1;
}

unsigned Semantics::DefAsgTypeSize(BCSYM* type)
{
    // 

    if (TypeHelpers::IsReferenceType(type))
    {
        return 1;
    }

    Vtypes vtype = type->GetVtype();

    if (vtype == t_struct)
    {
        BCSYM_GenericBinding* GenericBindingContext=NULL;
        DWORD count = 0;
        if (type->IsGenericBinding())
        {
            GenericBindingContext = type->PGenericBinding();
        }
        BCITER_CHILD ContainerIter;
        ContainerIter.Init(type->PNamedRoot());
        BCSYM_NamedRoot *Member;
        while (Member = ContainerIter.GetNext())
        {
            if (!DefAsgIsValidMember(Member))
            {
                continue;
            }
            BCSYM* memberType = Member->PMember()->GetType();
            if ( memberType->IsGenericParam() ||
                (memberType->GetVtype() == t_struct &&
                    ( memberType->PClass()->IsGeneric() || memberType->PClass()->HasGenericParent())))

            {
                memberType = ReplaceGenericParametersWithArguments(memberType, GenericBindingContext, m_SymbolCreator);
            }
            count += DefAsgTypeSize(memberType);
        }
        return count;
    }
    else
    {
        return 0;
    }

}

void
Semantics::CheckUnusedLocals
(
    ILTree::ExecutableBlock *block
)
{
    DBG_SWITCH_PRINTF(fDumpFlow, L"  CheckUnusedLocals: block %S\n", ILTree::BilopName(block->bilop));

    BCSYM_Hash *Locals = block->Locals;
    if (Locals)
    {
        BCSYM_NamedRoot *pLocal;

        HRESULT hr = S_OK;
        BCITER_CHILD_SORTED iterLocals(Locals);
        VSASSERT(SUCCEEDED(hr),"Ran out of stack space!");

        if (SUCCEEDED(hr))
        {
            while (pLocal = iterLocals.GetNext())
            {
                if (pLocal->IsAlias())
                {
                    continue;
                }

                BCSYM_Variable *pvar = pLocal->PVariable();
                if (!pvar->IsTemporary() && !pvar->IsUsed() &&
                    pvar->GetVarkind() != VAR_FunctionResult && pvar->GetVarkind() != VAR_Param)
                {
                    DBG_SWITCH_PRINTF(fDumpFlow, L"  *UNUSED LOCAL: %s [slot=%u size=%u]\n", pvar->GetName(), pvar->GetDefAsgSlot(), pvar->GetDefAsgSize());

                    ReportSemanticError(
                        pvar->IsConstant() ? WRNID_UnusedLocalConst : WRNID_UnusedLocal,
                        pvar->GetLocation(),
                        pvar);
                }
                else
                {
                    DBG_SWITCH_PRINTF(fDumpFlow, L"  local: %s  %s  %s  Varkind=%i\n", pvar->GetName(), (pvar->IsTemporary()?L"temp":L""), (pvar->IsUsed()?L"used":L"unused"), pvar->GetVarkind());
                }
            } // while locals
        }
    }

    // Note: this function only processes a single block.
    // It is expected that the caller used DefAsgSlotVisitor, to also iterate over
    // the blocks and multiline lambdas that the current block contains.
}


// ReportDefAgNoRetVal - this report the appropriate error
// "this function/property/operator does not necessarily return a value [may have null reference]"
// depending on what kind of procedure it's in, and whether the thing is a value-type or not.
void Semantics::ReportDefAsgNoRetVal(Procedure *procedure, Type *pRetVarType, Location loc, _In_z_ STRING *pName)
{
    // Dev10#704852: with this bugfix we added support for checking return-values on all code paths.
    // We only added it, though, for the following primitive types. See also the same list of primitive
    // types in DefAsgProcessSlots. This list and that one must be identical.
    bool isPrimitive = TypeHelpers::IsPrimitiveValueType(pRetVarType) ||
                       IsWinRTEventAddHandler(procedure, GetFXSymbolProvider());

    unsigned errId;

    if (procedure->IsUserDefinedOperatorMethod())
    {
        errId = isPrimitive ? WRNID_DefAsgNoRetValOpVal1 : WRNID_DefAsgNoRetValOpRef1;
    }
    else if (procedure->IsPropertyGet())
    {
        errId = isPrimitive ? WRNID_DefAsgNoRetValPropVal1 : WRNID_DefAsgNoRetValPropRef1;
    }
    else
    {
        errId = isPrimitive ? WRNID_DefAsgNoRetValFuncVal1 : WRNID_DefAsgNoRetValFuncRef1;
    }

    // We need to report the error into the error table associated with the
    // physical container of the procedure. With partial classes it is not always
    // the current error table.
    ErrorTable *SavedErrors = m_Errors;
    SourceFile *SavedFile = m_SourceFile;

    SourceFile *File = procedure->GetSourceFile();

    if ( File &&
         File != m_SourceFile &&
         m_AltErrTablesForConstructor )
    {
        ErrorTable *Errors = m_AltErrTablesForConstructor(File);
        if ( Errors )
        {
            m_Errors = Errors;
            m_SourceFile = File;
        }
    }

    ReportSemanticError(errId, loc, pName);

    m_Errors = SavedErrors;
    m_SourceFile = SavedFile;
}





// bit set class
#define BIT_SHIFT BITSET_BIT_SHIFT      // Shift by this much
#define BIT_MASK BITSET_BIT_MASK        // This is how many bits can go in a 'slot'
#define MAX_SIZE BITSET_MAX_SIZE        // This is the size of a 'slot'

BITSET * __fastcall BITSET::setBit(unsigned bit)
{
    if (((DWORD_PTR)this) & 1) {
        VSASSERT(bit <= BIT_MASK && bit >= 1,"bad bit in setBit");
        DWORD_PTR bits = (DWORD_PTR) this;
        bits |= (((DWORD_PTR)1) << bit);
        return (BITSET*) bits;
    }
    return setBitLarge(bit);
}

BITSET * __fastcall BITSET::setBitLarge(unsigned bit)
{
    BITSETIMP * bitset = (BITSETIMP*)this;
    bitset->bitArray[bit >> BIT_SHIFT] |= ((DWORD_PTR)1) << (bit & BIT_MASK);
    return this;
}

// returns the bits between start(inclusive) and start+size (exclusive)
// set to 1, all other bits to 0
DWORD_PTR __forceinline GetMask(unsigned start, unsigned size)
{
    VSASSERT(start <= BIT_MASK && (start + size <= MAX_SIZE),"wrong mask");

    return (((DWORD_PTR) -1) << (MAX_SIZE - size)) >> (MAX_SIZE - size - start);
}

bool __fastcall BITSET::isEqual(BITSET * other)
{
    if (!this) return !other;
    if ((((DWORD_PTR)this)&1) != (((DWORD_PTR)other)&1))
    {
        VSFAIL("small/large this/other mismatch");
        VBFatal(L"small/large this/other mismatch");
        return true;
    }
    if (((DWORD_PTR)this) & 1) {
        return ((DWORD_PTR)this) == ((DWORD_PTR)other);
    }
    return isEqualLarge(other);
}

bool __fastcall BITSET::isEqualLarge(BITSET * other)
{
    BITSETIMP * bitset = (BITSETIMP*)this;
    BITSETIMP * compare = (BITSETIMP*)other;

    if (!compare) return false;

    size_t size = bitset->arrayEnd - bitset->bitArray;
    return !memcmp(bitset->bitArray, compare->bitArray, size * sizeof(DWORD_PTR));
}

BITSET * __fastcall BITSET::setBits(unsigned start, unsigned size)
{
    if (((DWORD_PTR)this) & 1) {
        DWORD_PTR bits = (DWORD_PTR) this;
        bits |= GetMask(start, size);
        return (BITSET *) bits;
    }
    return setBitsLarge(start, size);
}

BITSET * __fastcall BITSET::setBitsLarge(unsigned start, unsigned size)
{

    if (size == 1) return setBitLarge(start);

    BITSETIMP * bitset = (BITSETIMP*)this;

    unsigned index = start >> BIT_SHIFT;
    start = start & BIT_MASK;
    unsigned thisSize;

    while ((thisSize = __min(size, MAX_SIZE - start))) {
        bitset->bitArray[index] |= GetMask(start, thisSize);
        if (size > thisSize)
            size -= thisSize;
        else
            size = 0;
        start = 0;
        index ++;
    }

    return this;

}

DWORD_PTR __fastcall BITSET::testBit(unsigned bit)
{
    if (((DWORD_PTR)this) & 1) {
        VSASSERT(bit <= BIT_MASK && bit >= 1,"wrong bit to test");
        DWORD_PTR bits = (DWORD_PTR) this;
        return (bits & (((DWORD_PTR)1) << bit));
    }
    return testBitLarge(bit);
}

bool __fastcall BITSET::isZero()
{
    if (((DWORD_PTR)this) & 1) {
        DWORD_PTR bits = (DWORD_PTR) this;
        return bits == 1;
    }
    return isZeroLarge();
}

bool __fastcall BITSET::isZeroLarge()
{
    BITSETIMP * bitset = (BITSETIMP*)this;
    DWORD_PTR * thisDword = bitset->bitArray;
    do {
        if (*(thisDword++)) return false;
    } while (thisDword != bitset->arrayEnd);
    return true;
}


DWORD_PTR __fastcall BITSET::testBitLarge(unsigned bit)
{
    BITSETIMP * bitset = (BITSETIMP*)this;
    return (bitset->bitArray[bit >> BIT_SHIFT] & (((DWORD_PTR)1) << (bit & BIT_MASK)));
}


DWORD_PTR __fastcall BITSET::testBits(unsigned start, unsigned size)
{
    if (size == 1) return testBit(start);

    if (((DWORD_PTR)this) & 1) {
        DWORD_PTR bits = (DWORD_PTR) this;
        return (bits | ~(GetMask(start, size))) == (DWORD_PTR) -1;
    }
    return testBitsLarge(start, size);

}

DWORD_PTR __fastcall BITSET::testBitsLarge(unsigned start, unsigned size)
{
    if (size == 1) return testBitLarge(start);

    BITSETIMP * bitset = (BITSETIMP*)this;

    unsigned index = start >> BIT_SHIFT;
    start = start & BIT_MASK;
    unsigned thisSize;

    while ((thisSize = __min(size, MAX_SIZE - start))) {
        if ((bitset->bitArray[index] | ~(GetMask(start, thisSize))) != (DWORD_PTR) -1) {
            return 0;
        }

        if (size > thisSize)
            size -= thisSize;
        else
            size = 0;
        start = 0;
        index ++;
    }

    return (DWORD_PTR) -1;

}

static const unsigned bitsPerNibble[] =
{
    0,          // 0000
    1,          // 0001
    1,          // 0010
    2,          // 0011
    1,          // 0100
    2,          // 0101
    2,          // 0110
    3,          // 0111
    1,          // 1000
    2,          // 1001
    2,          // 1010
    3,          // 1011
    2,          // 1100
    3,          // 1101
    3,          // 1110
    4,          // 1111
};

unsigned __fastcall BITSET::numberSet(DWORD_PTR dw)
{
#define NUM_BITS(val) (bitsPerNibble[((val) & 0xF)])
#if defined(_M_IX86)
    // 8 Nibbles in a DWORD_PTR
    return NUM_BITS(dw      ) + NUM_BITS(dw >> 4 ) + NUM_BITS(dw >> 8 ) + NUM_BITS(dw >> 12)
         + NUM_BITS(dw >> 16) + NUM_BITS(dw >> 20) + NUM_BITS(dw >> 24) + NUM_BITS(dw >> 28);

#elif defined(_WIN64)
    // 16 Nibbles in a DWORD_PTR
    return NUM_BITS(dw      ) + NUM_BITS(dw >> 4 ) + NUM_BITS(dw >> 8 ) + NUM_BITS(dw >> 12)
         + NUM_BITS(dw >> 16) + NUM_BITS(dw >> 20) + NUM_BITS(dw >> 24) + NUM_BITS(dw >> 28)
         + NUM_BITS(dw >> 32) + NUM_BITS(dw >> 36) + NUM_BITS(dw >> 40) + NUM_BITS(dw >> 44)
         + NUM_BITS(dw >> 48) + NUM_BITS(dw >> 52) + NUM_BITS(dw >> 56) + NUM_BITS(dw >> 60);

#else
    unsigned count = 0;
    for ( ; dw; dw >>= 4) {
        count += NUM_BITS(dw);
    }
    return count;

#endif
#undef NUM_BITS
}

unsigned __fastcall BITSET::numberSet()
{
    if (((DWORD_PTR)this) & 1) {
        return numberSet((DWORD_PTR)this) - 1;
    }

    unsigned count = 0;
    for (DWORD_PTR *p = ((BITSETIMP*)this)->bitArray; p < ((BITSETIMP*)this)->arrayEnd; p += 1) {
        count += numberSet(*p);
    }
    return count;
}

BITSET * __fastcall BITSET::orInto(BITSET * source)
{
    if ((((DWORD_PTR)this)&1) != (((DWORD_PTR)source)&1))
    {
        VSFAIL("small/large this/source mismatch");
        VBFatal(L"small/large this/other mismatch");
        return this;
    }
    if (((DWORD_PTR)this) & 1) {
        DWORD_PTR bits = (DWORD_PTR) this;
        bits |= ((DWORD_PTR) source);
        return (BITSET*) bits;
    }
    return orIntoLarge(source);
}

BITSET * __fastcall BITSET::orIntoLarge(BITSET * source)
{
    BITSETIMP * bitset = (BITSETIMP*)this;
    DWORD_PTR * thisDword = bitset->bitArray;
    DWORD_PTR * sourceDword = ((BITSETIMP*)source)->bitArray;
    do {
        *(thisDword++) |= *(sourceDword++);
    } while (thisDword != bitset->arrayEnd);
    return this;
}

BITSET * __fastcall BITSET::orInto(BITSET * source, DWORD_PTR * changed)
{
    if ((((DWORD_PTR)this)&1) != (((DWORD_PTR)source)&1))
    {
        VSFAIL("small/large this/source mismatch");
        VBFatal(L"small/large this/other mismatch");
        return this;
    }
    if (((DWORD_PTR)this) & 1) {
        DWORD_PTR bits = (DWORD_PTR) this;
        bits |= ((DWORD_PTR) source); // compute the new bits
        *changed = ((DWORD_PTR)this & ~bits); // extinguished bits are those in old which are not in new
        return (BITSET*) bits;
    }
    return orIntoLarge(source, changed);
}

BITSET * __fastcall BITSET::orIntoLarge(BITSET * source, DWORD_PTR * changed)
{
    BITSETIMP * bitset = (BITSETIMP*)this;
    DWORD_PTR * thisDword = bitset->bitArray;
    DWORD_PTR * sourceDword = ((BITSETIMP*)source)->bitArray;
    *changed = 0;
    do {
        DWORD_PTR saved = *thisDword;   // save the old bits
        *thisDword |= *(sourceDword++); // compute the new bits
        *changed |= (saved & ~*(thisDword++));    // extinguished bits are those in old which are not in new
    } while (thisDword != bitset->arrayEnd);
    return this;
}

BITSET * __fastcall BITSET::andInto(BITSET * source)
{
    if ((((DWORD_PTR)this)&1) != (((DWORD_PTR)source)&1))
    {
        VSFAIL("small/large this/source mismatch");
        VBFatal(L"small/large this/other mismatch");
        return this;
    }
    if (((DWORD_PTR)this) & 1) {
        DWORD_PTR bits = (DWORD_PTR) this;
        bits &= ((DWORD_PTR) source);
        return (BITSET*) bits;
    }
    return andIntoLarge(source);
}

BITSET * __fastcall BITSET::andIntoLarge(BITSET * source)
{
    BITSETIMP * bitset = (BITSETIMP*)this;
    DWORD_PTR * thisDword = bitset->bitArray;
    DWORD_PTR * sourceDword = ((BITSETIMP*)source)->bitArray;
    do {
        *(thisDword++) &= *(sourceDword++);
    } while (thisDword != bitset->arrayEnd);
    return this;
}

BITSET * __fastcall BITSET::andInto(BITSET * source, DWORD_PTR * changed)
{
    if ((((DWORD_PTR)this)&1) != (((DWORD_PTR)source)&1))
    {
        VSFAIL("small/large this/source mismatch");
        VBFatal(L"small/large this/other mismatch");
        return this;
    }
    if (((DWORD_PTR)this) & 1) {
        DWORD_PTR bits = (DWORD_PTR) this;
        bits &= ((DWORD_PTR) source); // compute the new bits
        *changed = ((DWORD_PTR)this & ~bits); // extinguished bits are those in old which are not in new
        return (BITSET*) bits;
    }
    return andIntoLarge(source, changed);
}

BITSET * __fastcall BITSET::andIntoLarge(BITSET * source, DWORD_PTR * changed)
{
    BITSETIMP * bitset = (BITSETIMP*)this;
    DWORD_PTR * thisDword = bitset->bitArray;
    DWORD_PTR * sourceDword = ((BITSETIMP*)source)->bitArray;
    *changed = 0;
    do {
        DWORD_PTR saved = *thisDword;   // save the old bits
        *thisDword &= *(sourceDword++); // compute the new bits
        *changed |= (saved & ~*(thisDword++));    // extinguished bits are those in old which are not in new
    } while (thisDword != bitset->arrayEnd);
    return this;
}

BITSET * __fastcall BITSET::create(unsigned size, NorlsAllocator * alloc, int init)
{
    if (size <= BIT_MASK) {
        return (BITSET*) ((DWORD_PTR)init | (DWORD_PTR)1);
    } else {
        return createLarge(size, alloc, init);
    }
}

BITSET * __fastcall BITSET::createCopy(BITSET * source, NorlsAllocator * alloc)
{
    if (((DWORD_PTR)source) & 1) {
        return source;
    } else {
        return createCopyLarge(source, alloc);
    }
}

BITSET * __fastcall BITSET::createCopy(BITSET * source, void * space)
{
    VSASSERT(!(((DWORD_PTR)source) & 1),"bad source in createCopy");
    return createCopyLarge(source, space);
}


BITSET * __fastcall BITSET::createLarge(unsigned size, NorlsAllocator * alloc, int init)
{
    BITSETIMP * rval = (BITSETIMP*) alloc->Alloc(sizeof(BITSETIMP) + sizeof(DWORD_PTR) * ((size + MAX_SIZE) >> BIT_SHIFT));
    ASSERT(!(((DWORD_PTR)rval) & 1),"why is not initialized here");
    memset(rval->bitArray, init,  ((size + MAX_SIZE) >> BIT_SHIFT) * sizeof(DWORD_PTR));
    rval->arrayEnd = rval->bitArray + ((size + MAX_SIZE) >> BIT_SHIFT);
    return (BITSET*) rval;
}

unsigned __fastcall BITSET::getSize()
{
    VSASSERT(!(((DWORD_PTR)this) & 1),"why getsize called for small bitset");
    BITSETIMP * bitset = (BITSETIMP*)this;

    return (unsigned) ((bitset->arrayEnd - bitset->bitArray) * sizeof(DWORD_PTR) + sizeof(BITSETIMP));
}

unsigned __fastcall BITSET::sizeInBytes()
{
    if (((DWORD_PTR)this) & 1) {
        return sizeof(DWORD_PTR);
    } else {
        BITSETIMP * bitset = (BITSETIMP*)this;
        return (unsigned) ((bitset->arrayEnd - bitset->bitArray) * sizeof(DWORD_PTR));
    }
}

unsigned __fastcall BITSET::rawSizeInBytes(unsigned numberOfBits)
{
    return ((numberOfBits + MAX_SIZE) >> BIT_SHIFT) * sizeof(DWORD_PTR);
}

BYTE *   __fastcall BITSET::rawBytes(BITSET **bitset)
{
    if (((DWORD_PTR)*bitset) & 1) {
        return (BYTE*)bitset;
    } else {
        return (BYTE*)((BITSETIMP*)(*bitset))->bitArray;
    }
}

BITSET * __fastcall BITSET::createCopyLarge(BITSET *source, NorlsAllocator * alloc)
{
    BITSETIMP * bitset = (BITSETIMP*)source;
    size_t size = bitset->arrayEnd - bitset->bitArray; // this is size in DWORD_PTRS
    if (!VBMath::TryMultiply(sizeof(DWORD_PTR), size) || 
        sizeof(BITSETIMP) + sizeof(DWORD_PTR) * size < sizeof(BITSETIMP)) 
    {
        return NULL; // Overflow.
    }
    BITSETIMP * rval = (BITSETIMP*) alloc->Alloc(sizeof(BITSETIMP) + sizeof(DWORD_PTR) * size);
    memcpy(rval->bitArray, bitset->bitArray , size * sizeof(DWORD_PTR));
    rval->arrayEnd = rval->bitArray + size;
    return (BITSET*) rval;

}

BITSET * __fastcall BITSET::createCopyLarge(BITSET *source, void * space)
{
    BITSETIMP * bitset = (BITSETIMP*)source;
    size_t size = bitset->arrayEnd - bitset->bitArray; // this is size in DWORD_PTRS
    BITSETIMP * rval = (BITSETIMP*) space;
    memcpy(rval->bitArray, bitset->bitArray , size * sizeof(DWORD_PTR));
    rval->arrayEnd = rval->bitArray + size;
    return (BITSET*) rval;
}


BITSET * __fastcall BITSET::setInto(BITSET * source)
{
    if (((DWORD_PTR)source) & 1) {
        return source;
    }
    return setIntoLarge(source);
}

BITSET * __fastcall BITSET::setInto(BITSET * source, NorlsAllocator * alloc)
{
    if (((DWORD_PTR)source) & 1) {
        return source;
    }
    return setIntoLarge(source, alloc);
}

BITSET * __fastcall BITSET::setInto(BITSET * source, NorlsAllocator * alloc, DWORD_PTR * changed)
{
    if (((DWORD_PTR)source) & 1) {
        *changed = (((DWORD_PTR)this) != (DWORD_PTR)source);
        return source;
    }

    return setIntoLarge(source, alloc, changed);
}

BITSET * __fastcall BITSET::setInto(int value)
{
    if (((DWORD_PTR)this) & 1) {
        return (BITSET*) ((DWORD_PTR)value | 1);
    }

    return setIntoLarge(value);

}

BITSET * __fastcall BITSET::setIntoLarge(int value)
{
    BITSETIMP * bitset = (BITSETIMP*)this;
    VSASSERT(!(((DWORD_PTR)bitset) & 1),"bad val for setIntoLarge");
    size_t size = bitset->arrayEnd - bitset->bitArray; // this is size in DWORD_PTRS
    memset(bitset->bitArray, value,  size * sizeof(DWORD_PTR));
    return (BITSET*) bitset;
}


BITSET * __fastcall BITSET::setIntoLarge(BITSET * source)
{
    VSASSERT(!(((DWORD_PTR)this) & 1),"bad source in setIntoLarge");
    VSASSERT(this,"null ref");

    BITSETIMP * bitset = (BITSETIMP*)source;
    size_t size = bitset->arrayEnd - bitset->bitArray; // this is size in DWORD_PTRS
    BITSETIMP * rval = (BITSETIMP*) this;

    memcpy(rval->bitArray, bitset->bitArray , size * sizeof(DWORD_PTR));
    rval->arrayEnd = rval->bitArray + size;
    return (BITSET*) rval;
}

BITSET * __fastcall BITSET::setIntoLarge(BITSET * source, NorlsAllocator * alloc, DWORD_PTR * changed)
{
    VSASSERT(!(((DWORD_PTR)this) & 1),"bad this for setIntoLarge");
    if (!this) {
        *changed = 0;
        return createCopyLarge(source, alloc);
    }

    BITSETIMP * bitset = (BITSETIMP*)source;
    size_t size = bitset->arrayEnd - bitset->bitArray; // this is size in DWORD_PTRS
    BITSETIMP * rval = (BITSETIMP*) this;
    *changed = memcmp(bitset->bitArray, rval->bitArray, size * sizeof(DWORD_PTR));
    memcpy(rval->bitArray, bitset->bitArray , size * sizeof(DWORD_PTR));
    rval->arrayEnd = rval->bitArray + size;
    return (BITSET*) rval;
}

BITSET * __fastcall BITSET::setIntoLarge(BITSET * source, NorlsAllocator * alloc)
{
    VSASSERT(!(((DWORD_PTR)this) & 1),"bad this for setIntoLarge");
    if (!this) {
        return createCopyLarge(source, alloc);
    }

    BITSETIMP * bitset = (BITSETIMP*)source;
    size_t size = bitset->arrayEnd - bitset->bitArray; // this is size in DWORD_PTRS
    BITSETIMP * rval = (BITSETIMP*) this;
    memcpy(rval->bitArray, bitset->bitArray , size * sizeof(DWORD_PTR));
    rval->arrayEnd = rval->bitArray + size;
    return (BITSET*) rval;
}


__forceinline void BITSET::swap(BITSET *** bs1, BITSET *** bs2)
{
    BITSET ** temp = *bs1;
    *bs1 = *bs2;
    *bs2 = temp;
}

BITSET * __fastcall BITSET::andIntoOrCopy(BITSET * source, DWORD_PTR * changed, NorlsAllocator * alloc)
{
    if (this)
    {
        return andInto(source, changed);
    }
    else
    {
        *changed = 1;
        return BITSET::createCopy(source, alloc);
    }
}

BITSET * __fastcall BITSET::orIntoOrCopy(BITSET * source, DWORD_PTR * changed, NorlsAllocator * alloc)
{
    if (this)
    {
        return orInto(source, changed);
    }
    else
    {
        *changed = 1;
        return BITSET::createCopy(source, alloc);
    }
}


BITSET * __fastcall BITSET::andIntoOrCopy(BITSET * source, NorlsAllocator * alloc)
{
    if (this)
    {
        return andInto(source);
    }
    else
    {
        return BITSET::createCopy(source, alloc);
    }
}


void DefAsgReEvalStmtList::AddReEvalStmt(ILTree::PILNode pstmt, unsigned int ClosureDepth, NorlsAllocator* allocator)
{
    if (pstmt == NULL)
    {
        return;
    }
    CSingleListIter<DefAsgReEvalStmt> iter(this);
    DefAsgReEvalStmt* current;
    bool found = false;
    while((current = iter.Next()) != NULL && current->m_ReEvalStmt != pstmt )
        ;
    if (current == NULL)
    {
        DefAsgReEvalStmt* dfb = (DefAsgReEvalStmt*)allocator->Alloc(sizeof(DefAsgReEvalStmt));
        dfb->m_ReEvalStmt = pstmt;
        dfb->ClosureDepth = ClosureDepth;
        this->InsertLast(dfb);
    }
}
void DefAsgReEvalStmtList::RemoveReEvalStmt(ILTree::PILNode pstmt)
{
    if (pstmt == NULL || NumberOfEntries() == 0)
    {
        return;
    }
    if (m_first->m_ReEvalStmt == pstmt )
    {
        if (m_last == m_first)
        {
            m_last = NULL;
        }
        m_first = m_first->Next();
        m_ulCount -= 1;
        return;
    }
    for (DefAsgReEvalStmt* pPrev = m_first; pPrev->Next() != NULL; pPrev = pPrev->Next())
    {
        if (pPrev->Next()->m_ReEvalStmt == pstmt)
        {
            if (m_last == pPrev->Next())
            {
                m_last = pPrev;
            }
            *(pPrev->NextPointer()) = pPrev->Next()->Next();
            m_ulCount -=1;
            return;
        }
    }
    return;
}
