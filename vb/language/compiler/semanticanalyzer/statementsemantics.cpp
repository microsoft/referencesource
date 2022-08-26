//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implementation of VB method body semantic analysis.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

// returns FALSE if aborting
bool Semantics::TryInterpretStatementSequence
(
    _Inout_opt_ ParseTree::StatementList *First)
{
    ILTree::TryBlock *TryPendingCompletionInCurrentBlock = NULL;

    for (ParseTree::StatementList *Current = First;
         Current;
         Current = Current->NextInBlock, m_TemporaryManager->FreeShortLivedTemporaries())
    {

        // Increase the statement id before the next statement.  Several methods including
        // InitializeFields run before this and we need to keep their group ID separate from
        // the ones allocated here
        ++m_statementGroupId;
        InterpretStatement(Current->Element, TryPendingCompletionInCurrentBlock);
#if IDE    
        if (CheckStop(this)) // if the foreground thread wants to stop the bkd, then let's not recur so IDE is more responsive
        {
            return false; //indicate aborting
        }
#endif IDE

        switch (Current->Element->Opcode)
        {
            // If statements use a sort of dual-block nesting in the bound trees,
            // and so don't quite obey the natural nesting structure of the
            // parse trees. If the current statement is part of an if statement,
            // and the next statement is not, end the if block.

            case ParseTree::Statement::BlockIf:
            case ParseTree::Statement::LineIf:
            case ParseTree::Statement::ElseIf:
            case ParseTree::Statement::BlockElse:
            case ParseTree::Statement::LineElse:
            {
                if (m_BlockContext->bilop == SB_IF_BLOCK &&
                   (Current->NextInBlock == NULL ||
                    (Current->NextInBlock->Element->Opcode != ParseTree::Statement::ElseIf &&
                     Current->NextInBlock->Element->Opcode != ParseTree::Statement::BlockElse &&
                     Current->NextInBlock->Element->Opcode != ParseTree::Statement::LineElse)))
                {
                    PopBlockContext();
                }
            }
            break;

            // Try statements must have an associated Finally or Catch block,
            // otherwise it's a compile error

            case ParseTree::Statement::Try:
            {
                if (Current->NextInBlock == NULL ||
                    (Current->NextInBlock->Element->Opcode != ParseTree::Statement::Catch &&
                     Current->NextInBlock->Element->Opcode != ParseTree::Statement::Finally))
                {
                    ReportSemanticError(
                        ERRID_TryWithoutCatchOrFinally,
                        Current->Element->TextSpan);
                }
            }
            break;
        }
    }
#if IDE    
    // handle the case of empty method
    if (CheckStop(this)) // if the foreground thread wants to stop the bkd, then let's not recur so IDE is more responsive
    {
        return false; //indicate aborting
    }
#endif IDE

    // Make sure that no statements allocated after this could be associated with the statements
    // allocated for this set of statements
    ++m_statementGroupId;
    return true;    //indicate not aborting
}

void
Semantics::InterpretAndPopBlock
(
    _In_ ParseTree::ExecutableBlockStatement *Block
)
{
    if (TryInterpretBlock(Block)) // if we didn't abort
    {
        PopBlockContext();
    }
}

// returns FALSE if aborting
bool
Semantics::TryInterpretBlock
(
    _In_ ParseTree::ExecutableBlockStatement *Block
)
{
    if (Block->LocalsCount)
    {
        // Create symbols for all locals at block entry.
        // A local is not accessible until its point of declaration,
        // but its scope is the entire containing block.

        if (m_BlockContext->Locals == NULL)
        {
            m_BlockContext->Locals =
                m_SymbolCreator.GetHashTable(
                    NULL,
                    m_Lookup,
                    true,
                    m_BlockContext->LocalsCount + 1,
                    NULL);
        }

        m_Lookup = m_BlockContext->Locals;

        for (ParseTree::StatementList *Current = Block->Children;
             Current;
             Current = Current->NextInBlock)
        {
            if (Current->Element->Opcode == ParseTree::Statement::VariableDeclaration)
            {
                ParseTree::VariableDeclarationStatement *VariableDeclaration =
                    Current->Element->AsVariableDeclaration();

                // For field level lambdas, we do not have an owning procedure symbol, so we must use this
                // overload of MakeLocalsFromTrees.
                Declared::MakeLocalsFromTrees(
                    m_CompilerHost,
                    &m_TreeStorage,
                    m_CompilationCaches,
                    m_ReportErrors ? m_Errors : NULL, // If we do not want to report errors, do not pass the error table in beceause we may report errors if we spin up a new instance of semantics.
                    ContainingClass()->PClass(),
                    m_SourceFile,
                    m_Procedure,
                    m_Lookup,
                    VariableDeclaration,
                    m_CreatedInParseTreeService,  // not being called from ParseTreeService
                    PerformObsoleteChecks() ? TypeResolvePerformObsoleteChecks : TypeResolveNoFlags,
                    NULL,
                    true);
            }
        }
    }

    return TryInterpretStatementSequence(Block->Children);
}

void
Semantics::InterpretStatement
(
    _In_ ParseTree::Statement *Input,
    _Inout_ ILTree::TryBlock *&TryPendingCompletionInCurrentBlock
)
{
    if (Input->HasSyntaxError)
    {
        MarkContainingLambdaOrMethodBodyBad();
    }

    switch (Input->Opcode)
    {
        case ParseTree::Statement::BlockElse:
        case ParseTree::Statement::LineElse:

            VSASSERT(
                Input->Opcode == ParseTree::Statement::LineElse ?
                    HasFlag32(m_BlockContext, SBF_LINE_IF) :
                    !HasFlag32(m_BlockContext, SBF_LINE_IF),
                "inconsistent end construct flags");

            AllocateBlock(SB_ELSE, Input->AsExecutableBlock());

            InterpretAndPopBlock(Input->AsExecutableBlock());
            break;

        case ParseTree::Statement::Finally:
        {
            ILTree::FinallyBlock *FinallyBlock =
                &AllocateFinallyBlock(TryPendingCompletionInCurrentBlock, Input->AsExecutableBlock())->AsFinallyBlock();

            TryPendingCompletionInCurrentBlock = NULL;

            InterpretAndPopBlock(Input->AsExecutableBlock());
            break;
        }

        case ParseTree::Statement::Try:

            TryPendingCompletionInCurrentBlock = InterpretTryStatement(Input->AsExecutableBlock());

            InterpretAndPopBlock(Input->AsExecutableBlock());
            break;

        case ParseTree::Statement::ElseIf:

            InterpretElseIfStatement(Input->AsExpressionBlock());
            InterpretAndPopBlock(Input->AsExecutableBlock());
            break;

        case ParseTree::Statement::BlockIf:
        case ParseTree::Statement::LineIf:
            InterpretIfStatement(Input->AsExpressionBlock());
            InterpretAndPopBlock(Input->AsExecutableBlock());
            break;

        case ParseTree::Statement::While:
        case ParseTree::Statement::DoWhileTopTest:
        case ParseTree::Statement::DoUntilTopTest:
        case ParseTree::Statement::DoWhileBottomTest:
        case ParseTree::Statement::DoUntilBottomTest:
        case ParseTree::Statement::DoForever:

            InterpretLoopStatement(Input->AsExpressionBlock());
            InterpretAndPopBlock(Input->AsExecutableBlock());
            break;

        case ParseTree::Statement::Select:

            InterpretSelectStatement(Input->AsExpressionBlock());
            if (TryInterpretBlock(Input->AsExecutableBlock()))
            {
                OptimizeSelectStatement(&m_BlockContext->AsSelectBlock());
                PopBlockContext();
            }
            break;

        case ParseTree::Statement::Case:

            InterpretCaseStatement(Input->AsCase());
            InterpretAndPopBlock(Input->AsExecutableBlock());
            break;

        case ParseTree::Statement::CaseElse:

            InterpretCaseElseStatement(Input->AsExecutableBlock());
            InterpretAndPopBlock(Input->AsExecutableBlock());
            break;

        case ParseTree::Statement::ForFromTo:
        case ParseTree::Statement::ForEachIn:

            InterpretForStatement(Input->AsFor());
            break;

        case ParseTree::Statement::With:
        {
            ILTree::WithBlock *EnclosingWith = m_EnclosingWith;
            m_EnclosingWith = InterpretWithStatement(Input->AsExpressionBlock());
            InterpretAndPopBlock(Input->AsExecutableBlock());
            m_EnclosingWith = EnclosingWith;
            break;
        }

        case ParseTree::Statement::Using:
        {
            InterpretUsingStatement(Input->AsUsing());
            break;
        }

        case ParseTree::Statement::SyncLock:
        {
            // Encose all the Synclock declarations in an extra block for correct scoping.
            AllocateBlock(SB_SYNCLOCK, Input->AsExpressionBlock());

            ILTree::TryBlock *TryStatementForSynclock = NULL;
            Variable *pLockTakenVar = NULL;
            Variable *CapturedObject = InterpretSyncLockStatement(Input->AsExpressionBlock(), TryStatementForSynclock, pLockTakenVar);

            InterpretAndPopBlock(Input->AsExecutableBlock());

            TerminateSyncLockStatement(
                CapturedObject,
                Input->AsExecutableBlock()->TerminatingConstruct ?
                    Input->AsExecutableBlock()->TerminatingConstruct->TextSpan:
                    Location::GetHiddenLocation(),
                TryStatementForSynclock,
                pLockTakenVar);

            PopBlockContext();
            break;
        }

        case ParseTree::Statement::Catch:

            InterpretCatchStatement(Input->AsCatch());
            InterpretAndPopBlock(Input->AsExecutableBlock());
            break;

        case ParseTree::Statement::ContinueDo:
        case ParseTree::Statement::ContinueFor:
        case ParseTree::Statement::ContinueWhile:

            InterpretContinueStatement(Input);
            break;

        case ParseTree::Statement::ExitDo:
        case ParseTree::Statement::ExitFor:
        case ParseTree::Statement::ExitWhile:
        case ParseTree::Statement::ExitSelect:
        case ParseTree::Statement::ExitSub:
        case ParseTree::Statement::ExitFunction:
        case ParseTree::Statement::ExitProperty:
        case ParseTree::Statement::ExitTry:

            InterpretExitStatement(Input);
            break;

        case ParseTree::Statement::Goto:

            InterpretGotoStatement(Input->AsLabelReference());
            break;

        case ParseTree::Statement::Label:

            InterpretLabelStatement(Input->AsLabelReference());
            break;

        case ParseTree::Statement::End:

            if (m_Lookup->GetContainingProject()->OutputIsLibrary())
            {
                ReportSemanticError(ERRID_EndDisallowedInDllProjects, Input->TextSpan);
                break;
            }

            // Starlite Library doesn't support the End statement.
            if (m_CompilerHost->IsStarliteHost())
            {
                ReportSemanticError(ERRID_StarliteDisallowsEndStatement, Input->TextSpan);
                break;
            }

            AllocateStatement(SL_END, Input);
            break;

        case ParseTree::Statement::EndTry:

            TryPendingCompletionInCurrentBlock = NULL;
            break;

        case ParseTree::Statement::EndIf:
        case ParseTree::Statement::EndUsing:
        case ParseTree::Statement::EndWith:
        case ParseTree::Statement::EndNext:
        case ParseTree::Statement::EndWhile:
        case ParseTree::Statement::EndLoop:
        case ParseTree::Statement::EndLoopWhile:
        case ParseTree::Statement::EndLoopUntil:
        case ParseTree::Statement::EndSelect:
        case ParseTree::Statement::EndSyncLock:

        case ParseTree::Statement::EndRegion:
        case ParseTree::Statement::EndStructure:
        case ParseTree::Statement::EndEnum:
        case ParseTree::Statement::EndInterface:
        case ParseTree::Statement::EndClass:
        case ParseTree::Statement::EndModule:
        case ParseTree::Statement::EndNamespace:
        case ParseTree::Statement::EndSub:
        case ParseTree::Statement::EndFunction:
        case ParseTree::Statement::EndOperator:
        case ParseTree::Statement::EndProperty:
        case ParseTree::Statement::EndEvent:
        case ParseTree::Statement::EndAddHandler:
        case ParseTree::Statement::EndRemoveHandler:
        case ParseTree::Statement::EndRaiseEvent:
        case ParseTree::Statement::EndUnknown:
        case ParseTree::Statement::EndGet:
        case ParseTree::Statement::EndSet:
        case ParseTree::Statement::EndInvalid:
        case ParseTree::Statement::ContinueUnknown:
        case ParseTree::Statement::ContinueInvalid:
        case ParseTree::Statement::ExitUnknown:
        case ParseTree::Statement::ExitInvalid:
        case ParseTree::Statement::ExitOperator:
        case ParseTree::Statement::CCIf:
        case ParseTree::Statement::CCElseIf:
        case ParseTree::Statement::CCElse:
        case ParseTree::Statement::CCEndIf:
        case ParseTree::Statement::CCConst:
        case ParseTree::Statement::SyntaxError:
        case ParseTree::Statement::Region:
        case ParseTree::Statement::Empty:
            break;

        case ParseTree::Statement::Stop:

            AllocateStatement(SL_STOP, Input);
            break;

        case ParseTree::Statement::VariableDeclaration:
        {

            // Declaration semantics does not process variable declaration statements
            // that contain any kind of syntax error. Therefore, processing
            // initializers for such statements is unsound.

            if (!Input->HasSyntaxError)
            {
                InterpretVariableDeclarationStatement(Input->AsVariableDeclaration());
            }
            break;
        }

        case ParseTree::Statement::Error:

            InterpretErrorStatement(Input->AsExpression());
            break;

        case ParseTree::Statement::Throw:

            InterpretThrowStatement(Input->AsExpression());
            break;

        case ParseTree::Statement::Redim:

            InterpretRedimStatement(Input->AsRedim());
            break;

        case ParseTree::Statement::Erase:

            InterpretEraseStatement(Input->AsErase());
            break;

        case ParseTree::Statement::Assign:

            InterpretAssignment(Input->AsAssignment());
            break;

        case ParseTree::Statement::AssignPlus:
        case ParseTree::Statement::AssignMinus:
        case ParseTree::Statement::AssignMultiply:
        case ParseTree::Statement::AssignDivide:
        case ParseTree::Statement::AssignPower:
        case ParseTree::Statement::AssignIntegralDivide:
        case ParseTree::Statement::AssignConcatenate:
        case ParseTree::Statement::AssignShiftLeft:
        case ParseTree::Statement::AssignShiftRight:

            InterpretOperatorAssignment(Input->AsAssignment());
            break;

        case ParseTree::Statement::Call:

            InterpretCall(Input->AsCall());
            break;

        case ParseTree::Statement::Return:

            InterpretReturn(Input->AsExpression());
            break;

        case ParseTree::Statement::OnError:

            InterpretOnErrorStatement(Input->AsOnError());
            break;

        case ParseTree::Statement::Resume:

            InterpretResumeStatement(Input->AsResume());
            break;

        case ParseTree::Statement::AssignMid:

            InterpretAssignMidStatement(Input->AsAssignMid());
            break;

        case ParseTree::Statement::RaiseEvent:

            InterpretRaiseEventStatement(Input->AsRaiseEvent());
            break;

        case ParseTree::Statement::AddHandler:
        case ParseTree::Statement::RemoveHandler:

            InterpretHandlerStatement(Input->AsHandler());
            break;

        case ParseTree::Statement::Await:

            InterpretAwaitStatement(Input->AsExpression());
            break;

        case ParseTree::Statement::Yield:

            InterpretYieldStatement(Input->AsExpression());
            break;

        default:

            VSFAIL("Surprising statement opcode.");
            break;
    }
}

static ParseTree::Expression::Opcodes
OperatorOpcode
(
    ParseTree::Statement::Opcodes AssignmentOpcode
)
{
    VSASSERT(
        AssignmentOpcode >= ParseTree::Statement::FirstOperatorAssign &&
            AssignmentOpcode <= ParseTree::Statement::LastOperatorAssign,
        "Surprising operator assignment opcode");

    return
        (ParseTree::Expression::Opcodes)
            (ParseTree::Expression::FirstBinaryOperator +
                (AssignmentOpcode - ParseTree::Statement::FirstOperatorAssign));
}

ILTree::Statement *
Semantics::InterpretAssignment
(
    _In_ ParseTree::AssignmentStatement *Assignment,
    const bool link
)
{
    return
        InterpretAssignment(Assignment->Target, Assignment->Source, Assignment->TextSpan, Assignment->ResumeIndex,link);
}

ILTree::Statement *
Semantics::InterpretAssignment
(
    ParseTree::Expression *UnboundTarget,
    ParseTree::Expression *UnboundSource,
    const Location &TextSpan,
    const unsigned short ResumeIndex,
    const bool link
)
{
    return
        InterpretAssignment
        (
            InterpretStatementOperand
            (
                UnboundTarget,
                ExprIsAssignmentTarget | ExprPropagatePropertyReference
            ),
            UnboundSource,
            TextSpan,
            ResumeIndex,
            link
        );
}

ILTree::Statement *
Semantics::InterpretAssignment
(
    ILTree::Expression *Target,
    ParseTree::Expression *UnboundSource,
    const Location &TextSpan,
    const unsigned short ResumeIndex,
    const bool link
)
{
    Type *TargetType = NULL;

    // TO DO:
    // This function has inexplicably different paths for properties vs non-properties.
    //   Properties: use InterpretStatementOperand(UnboundSource, ExprPropagatePropertyReference)
    //   Nonproperties: use InterpretStatementOperand(UnboundSource, ExprForceRValue, TargetType, NULL);
    // There seems to be no need for this difference. It should be rationalized.

    if (!IsBad(Target))
    {
        if (IsPropertyReference(Target))
        {
            return AssignToProperty(TextSpan, ResumeIndex, Target, UnboundSource,link);
        }

        TargetType = Target->ResultType;
    }

#if HOSTED

    // NOTE NOTE:
    // IF THE BELOW CODE CHANGES AFTER CHANGESET 1048815, NOTE THAT THE COPY OF THE CODE IN
    // InterpretExpressionForHostedCompiler WILL ALSO NEED TO BE CHANGED APPROPRIATELY.
    // THIS SYNC NEEDS TO HAPPEN TILL THE POINT OF TIME THIS CODE IS RE-FACTORED
    // (POST BETA2 HOPEFULLY) AND SHARED.

#endif HOSTED

    ILTree::Expression *Source;
    if( UnboundSource->Opcode == ParseTree::Expression::ArrayInitializer )
    {
        // Assignment of an array literal should consider the target type. See Dev10#722656.
        // This should call InterpretStatementOperand, just like below, but that doesn't work,
        // because it calls InterpretExpressionWithTargetType which throws away the TargetType
        // (passes NULL to InterpretExpression's TargetType parameter). This prevents us from
        // doing the right thing inside InterpretExpression (ultimately MakeRValue).
        Source = InterpretExpression(UnboundSource, ExprForceRValue | ExprDontInferResultType, 0, NULL, TargetType);
        if (TargetType != NULL && !IsBad(Source))
        {
            Source = ConvertWithErrorChecking(Source, TargetType, ExprForceRValue | ExprDontInferResultType);
        }
        if (IsBad(Source))
        {    
            MarkContainingLambdaOrMethodBodyBad();
        }
    }
    else
    {
        Source = InterpretStatementOperand(UnboundSource, ExprForceRValue, TargetType, NULL);
    }

    return
        FinishNonPropertyAssignment
        (
            TextSpan,
            ResumeIndex,
            Target,
            Source,
            link
        );
}

void
Semantics::InterpretOperatorAssignment
(
    _In_ ParseTree::AssignmentStatement *UnboundAssignment
)
{
    Type *TargetType = NULL;

    ILTree::Expression *Target =
        InterpretStatementOperand(
            UnboundAssignment->Target,
            ExprIsAssignmentTarget | ExprPropagatePropertyReference);

    ILTree::Expression *TargetAsOperand = Target;

    if (!IsBad(Target))
    {
        // The target appears twice in the generated code, because
        // "Target += Source" expands to "Target = Target + Source".
        // Any side effects associated with the target reference are
        // to be evaluated once, not twice.

        UseTwiceShortLived(Target, Target, TargetAsOperand);

        if (IsPropertyReference(TargetAsOperand))
        {
            TargetAsOperand = FetchFromProperty(TargetAsOperand);
        }
        else
        {
            TargetType = Target->ResultType;
        }
    }

    ExpressionFlags SourceFlags = ExprForceRValue;

    if (UnboundAssignment->Opcode == ParseTree::Statement::AssignConcatenate ||
        UnboundAssignment->Opcode == ParseTree::Statement::AssignPlus)
    {
        SourceFlags |= ExprIsOperandOfConcatenate;
    }

    ILTree::Expression *Source =
        InterpretStatementOperand(
            UnboundAssignment->Source,
            SourceFlags);

    // At this point TargetAsOperand and Source have been fully interpreted.

    if (!IsBad(Source) && !IsBad(TargetAsOperand))
    {
        TargetAsOperand = MakeRValue(TargetAsOperand);

        if (!IsBad(TargetAsOperand))
        {
            Source =
                InterpretBinaryOperation(
                    OperatorOpcode(UnboundAssignment->Opcode),
                    UnboundAssignment->TextSpan,
                    TargetAsOperand,
                    Source,
                    ExprNoFlags);

            // If we're at the top of a bound concatenate tree, then select the optimal overload of System.String.Concat()
            if (!IsBad(Source) &&
                Source->bilop == SX_CONC &&
                Source->vtype == t_string &&  // Note (8/24/2001):  Only optimize for string concatenations.  Object concatenation is complicated by DBNull.
                !m_IsGeneratingXML)
            {
                Source = OptimizeConcatenate(Source, UnboundAssignment->TextSpan);
            }

            if (TargetType && !IsBad(Source))
            {
                Source = ConvertWithErrorChecking(Source, TargetType, ExprNoFlags);
            }
        }
    }

    ILTree::Statement *Result = NULL;

    if (!IsBad(Target) &&
        !IsBad(Source) &&
        IsPropertyReference(Target))
    {
        Result = AssignToProperty(UnboundAssignment->TextSpan, UnboundAssignment->ResumeIndex, Target, Source);
    }
    else
    {
        Result =
            FinishNonPropertyAssignment(
            UnboundAssignment->TextSpan,
            UnboundAssignment->ResumeIndex,
            Target,
            Source);
    }
}

ILTree::Expression *
Semantics::InterpretPropertyAssignmentSource
(
    ParseTree::Expression *pUnboundSource,
    Type *pPropertyType
)
{

    ILTree::Expression * pBoundSource = NULL;

    if (pUnboundSource->Opcode == ParseTree::Expression::ArrayInitializer)
    {
        // Assignment of an array literal should consider the target type. See Dev10#749866.
        // This is the same as for #722656 below, but for properties.
        
        pBoundSource = InterpretExpression(pUnboundSource, ExprForceRValue | ExprDontInferResultType, 0, NULL, pPropertyType);
        if (pPropertyType != NULL && !IsBad(pBoundSource))
        {
            pBoundSource = ConvertWithErrorChecking(pBoundSource, pPropertyType, ExprForceRValue | ExprDontInferResultType);
        }
        if (IsBad(pBoundSource))
        {
            MarkContainingLambdaOrMethodBodyBad();
        }
    }
    else
    {
        if (pPropertyType == NULL)
        {
            pBoundSource = InterpretStatementOperand(pUnboundSource, ExprPropagatePropertyReference);
        }
        else
        {
            pBoundSource = InterpretStatementOperand(pUnboundSource, ExprPropagatePropertyReference, pPropertyType, NULL);
        }
    }
    return pBoundSource;
}

ILTree::Expression *
Semantics::InterpretPropertyAssignment
(
    const Location &AssignmentLocation,
    ILTree::Expression *pTarget,
    ParseTree::Expression *pUnboundSource,
    bool IsAggrInitAssignment
)
{

    // This function is created to fix bug 772951
    // The bug is caused by not interpreting source respect to type of property.
    // The type of property is only available after resulotion, 
    // then we attach Nothing as the last argument to setter in order to get the 
    // type of last argument, then we can use it to interpret source.

    ILTree::Expression *pBoundSource = NULL;
    BILOP TargetBilop = pTarget->bilop;

    if(TargetBilop == SX_LATE_REFERENCE)
    {
        pBoundSource = InterpretPropertyAssignmentSource(pUnboundSource,NULL);
        if (IsBad(pBoundSource))
        {
            return AllocateBadExpression(pUnboundSource->TextSpan);
        }
    }
    else
    {
        // SX_Nothing is used in order to get resolved setter argument type, 
        // the type is needed to interpret pUnboundSource.
    
        pBoundSource = AllocateExpression(
            SX_NOTHING,
            GetFXSymbolProvider()->GetObjectType(),
            pUnboundSource->TextSpan);
    }

    ILTree::Expression *pBoundSetter = InterpretPropertyAssignment(
        AssignmentLocation,
        pTarget,
        pBoundSource,
        IsAggrInitAssignment);

    if(TargetBilop != SX_LATE_REFERENCE)
    {
        // Here pBoundSource is interpreted with the type of Nothing. And pBoundSouce is attached to 
        // the setter if nothing is bad.
        if (IsBad(pBoundSetter))
        {
            // if setter is bad, then source only can be interpreted without target type.
            pBoundSource = InterpretPropertyAssignmentSource(pUnboundSource,NULL);
        }
        else
        {

            ThrowIfFalse(pBoundSetter->bilop == SX_CALL || pBoundSetter->bilop == SX_LATE);


            ExpressionList *pArgument = NULL;

            ILTree::Expression **ppLastArgument = NULL;

            if (pBoundSetter->bilop == SX_CALL)
            {

                pArgument = pBoundSetter->AsExpressionWithChildren().Right;

                ThrowIfFalse(pArgument != NULL && pArgument->bilop == SX_LIST);

                // Find the last argument which is Nothing, now we need the type of nothing in order to interpret 
                // pBoundSource. 
                while(pArgument->AsExpressionWithChildren().Right != NULL)
                {
                    pArgument = pArgument->AsExpressionWithChildren().Right;
                    ThrowIfFalse(pArgument->bilop == SX_LIST);
                }

                ppLastArgument = &(pArgument->AsExpressionWithChildren().Left);

                // Dev10 #811634 If setter takes value ByRef, ppLastArgument is going to
                // point to a SX_ASG_RESADR node. The right child of SX_ASG_RESADR
                // is the actual value assigned to the property.
                if ((*ppLastArgument)->bilop == SX_ASG_RESADR)
                {
                    ppLastArgument = &((*ppLastArgument)->AsExpressionWithChildren().Right);
                }
            }
            else // (pBoundSetter->bilop == SX_LATE)
            {

                // The following ThrowIfFalse caputres the shape of the argument list of late binding call. 

                ILTree::Expression *pLateCallArgument = pBoundSetter->AsLateBoundExpression().Right;
                ILTree::Expression *pSetterArgument = NULL;
               
                ThrowIfFalse(
                    pLateCallArgument != NULL &&
                    pLateCallArgument->bilop == SX_LIST &&

                    pLateCallArgument->AsExpressionWithChildren().Left != NULL);


                if (pLateCallArgument->AsExpressionWithChildren().Left->bilop == SX_SEQ_OP2)
                {
                    pSetterArgument = pLateCallArgument->AsExpressionWithChildren().Left->AsExpressionWithChildren().Left;

                    ThrowIfFalse( pSetterArgument != NULL && pSetterArgument->bilop == SX_SEQ);

                    pArgument = pSetterArgument->AsExpressionWithChildren().Right;

                    // The argument list should contain at least two elements, a property index and the Nothing we attached.
                    // There must be a property index, otherewise we would not have been in this branch.
                    // pArgument->bilop == SX_SEQ is true if there are at least two arguments.
     
                    ThrowIfFalse(pArgument != NULL && pArgument->bilop == SX_SEQ);
                    
                    do 
                    {
                        pArgument = pArgument->AsExpressionWithChildren().Right;
                        ThrowIfFalse(pArgument != NULL);
                    }while(pArgument->bilop == SX_SEQ);

                    ThrowIfFalse(pArgument->bilop == SX_ASG);
                    ppLastArgument = &(pArgument->AsExpressionWithChildren().Right);

                }
                // This case happens when m_GeneralXML is true, then InitiaArray will produce this shape of the tree
                else if(pLateCallArgument->AsExpressionWithChildren().Left->bilop == SX_CREATE_ARRAY)
                {
                    pSetterArgument = pLateCallArgument->AsExpressionWithChildren().Left->AsExpressionWithChildren().Right;
                    // This assumes the there must be one element.
                    ThrowIfFalse( pSetterArgument != NULL && pSetterArgument->bilop == SX_LIST);

                    pArgument = pSetterArgument->AsExpressionWithChildren().Right;

                    //This assumes the second element exists.
                    ThrowIfFalse(pArgument && pArgument->bilop == SX_LIST);

                    while(pArgument->AsExpressionWithChildren().Right != NULL)
                    {
                        pArgument = pArgument->AsExpressionWithChildren().Right;
                        ThrowIfFalse(pArgument->bilop == SX_LIST);
                    }

                    ThrowIfFalse(pArgument != NULL);
                    ppLastArgument = &(pArgument->AsExpressionWithChildren().Left);
                
                }
                else
                {
                    ThrowIfFalse(false);
                }

                ThrowIfFalse((*ppLastArgument)->bilop == SX_NOTHING);

            }
 
            if (IsBad(*ppLastArgument))
            {
                // if the last argument is bad, then source only can be interpreted without target type.
                pBoundSource = InterpretPropertyAssignmentSource(pUnboundSource,NULL);
            }
            else
            {
                pBoundSource = InterpretPropertyAssignmentSource(pUnboundSource, (*ppLastArgument)->ResultType);
 
                if (IsBad(pBoundSource))
                {
                    return MakeBad(pBoundSetter);  
                }
                
                pBoundSource = MakeRValue(pBoundSource);
                *ppLastArgument = pBoundSource;
            }
        }
    
     }
     return pBoundSetter;
}
    

    

ILTree::Expression *
Semantics::InterpretPropertyAssignment
(
    const Location &AssignmentLocation,
    ILTree::Expression *Target,
    ILTree::Expression *Source,
    bool IsAggrInitAssignment
)
{
    // Interpret an assignment to a property by adding the source of the
    // assignment to the end of the argument list of the property
    // reference and then interpreting the property reference as a call.

    ExpressionList *CallArguments = Target->AsExpressionWithChildren().Right;

    // Find where to put the last argument.
    ExpressionList **ListTarget = &CallArguments;
    for (ExpressionList *EndOfList = CallArguments;
         EndOfList;
         EndOfList = EndOfList->AsExpressionWithChildren().Right)
    {
        ListTarget = &EndOfList->AsExpressionWithChildren().Right;
    }

    // Because of this MakeRValue call, the Source value is never "trully"
    // passed ByRef. The Source becomes an RValue and, even if setter takes value ByRef, 
    // Semantics::PassArgumentByref() applies Semantics::CaptureInAddressedTemporary() 
    // to the value, which has an effect of capturing it (the value) in a temporary and passing
    // address of the temporary instead. We are relying on this fact in another overload of
    // Semantics::InterpretPropertyAssignment() to address Dev10 #811634.
    Source = MakeRValue(Source);

    ILTree::Expression *Argument =
        AllocateExpression(
            SX_ARG,
            TypeHelpers::GetVoidType(),
            Source,
            Source->Loc);

    *ListTarget =
        AllocateExpression(
            SX_LIST,
            TypeHelpers::GetVoidType(),
            Argument,
            NULL,
            Argument->Loc);

    ExpressionFlags PropertyFlags =
            ExprIsPropertyAssignment | ExprResultNotNeeded;

    if (IsAggrInitAssignment)
    {
        PropertyFlags |= ExprIsLHSOfObjectInitializer;
    }

    if (Target->bilop == SX_LATE_REFERENCE)
    {
        return
            InterpretLateBoundExpression(
                AssignmentLocation,
                Target->AsPropertyReferenceExpression(),
                CallArguments,
                PropertyFlags);
    }
    else
    {
        return
            InterpretCallExpressionWithNoCopyout(
                AssignmentLocation,
                Target->AsPropertyReferenceExpression().Left,
                Target->AsPropertyReferenceExpression().TypeCharacter,
                CallArguments,
                false,
                PropertyFlags,
                NULL);
    }
}




ILTree::Statement *
Semantics::AssignToPropertyInternal
(
    const Location &AssignmentLocation,
    const unsigned short ResumeIndex,
    ILTree::Expression *pTarget,
    ILTree::Expression *pSetCall,
    const bool link
)
{
    ILTree::Statement *Result = AllocateStatement(SL_STMT, AssignmentLocation, ResumeIndex, link);

    if (IsBad(pSetCall))
    {
        MarkContainingLambdaOrMethodBodyBad();
    }

    // Propagate extension call flag for properties (only Xml Value property supported for now)
    if (HasFlag32(pTarget, SXF_CALL_WAS_EXTENSION_CALL))
        SetFlag32(pSetCall, SXF_CALL_WAS_EXTENSION_CALL);

    Result->AsStatementWithExpression().Operand1 = pSetCall;
    return Result;
}

ILTree::Statement *
Semantics::AssignToProperty
(
    const Location &AssignmentLocation,
    const unsigned short ResumeIndex,
    ILTree::Expression *pTarget,
    ILTree::Expression *pBoundSource,
    const bool link
)
{
    ILTree::Expression *SetCall =
        InterpretPropertyAssignment(
            AssignmentLocation,
            pTarget,
            pBoundSource);
    
    return AssignToPropertyInternal(
        AssignmentLocation,
        ResumeIndex,
        pTarget,
        SetCall,
        link);
}

ILTree::Statement *
Semantics::AssignToProperty
(
    const Location &AssignmentLocation,
    const unsigned short ResumeIndex,
    ILTree::Expression *pTarget,
    ParseTree::Expression *pUnboundSource,
    const bool link
)
{
    ILTree::Expression *SetCall =
        InterpretPropertyAssignment(
            AssignmentLocation,
            pTarget,
            pUnboundSource);
    
    return AssignToPropertyInternal(
        AssignmentLocation,
        ResumeIndex,
        pTarget,
        SetCall,
        link);
}


ILTree::Expression *
Semantics::FetchFromProperty
(
    ILTree::Expression *Source
)
{
    ILTree::Expression *Result;

    if (Source->bilop == SX_LATE_REFERENCE)
    {
        LateReferenceExpression &LateReference = Source->AsPropertyReferenceExpression();

        Result =
            InterpretLateBoundExpression(
                Source->Loc,
                LateReference,
                LateReference.Right,
                ExprNoFlags);
    }
    else
    {
        ILTree::PropertyReferenceExpression &PropertyReference = Source->AsPropertyReferenceExpression();

        Result =
            InterpretCallExpressionWithNoCopyout(
                Source->Loc,
                PropertyReference.Left,
                PropertyReference.TypeCharacter,
                PropertyReference.Right,
                false,
                ExprNoFlags,
                NULL);

        // Propagate extension call flag for properties (only Xml Value property supported for now)
        if (HasFlag32(Source, SXF_CALL_WAS_EXTENSION_CALL))
            SetFlag32(Result, SXF_CALL_WAS_EXTENSION_CALL);
    }

    return ApplyContextSpecificSemantics(Result, ExprNoFlags, NULL);
}

ILTree::Statement *
Semantics::FinishNonPropertyAssignment
(
    const Location &AssignmentLocation,
    const unsigned short ResumeIndex,
    ILTree::Expression *Target,
    ILTree::Expression *Source,
    const bool link
)
{
    if (!m_fIncludeBadExpressions &&
        (IsBad(Target) || IsBad(Source)))
    {
        return NULL;
    }

    ILTree::Statement *AssignmentStatement = AllocateStatement(SL_STMT, AssignmentLocation, ResumeIndex,link);
    AssignmentStatement->AsStatementWithExpression().Operand1 =
        GenerateNonPropertyAssignment(
            AssignmentLocation,
            Target,
            Source);

    return AssignmentStatement;
}


static bool
ExpressionRefersToReadonlyVariable
(
    ILTree::Expression *RValue,
    bool PropertyReferenceAllowed = true
)
{
    ILTree::Expression *BaseReference = NULL;

    if (PropertyReferenceAllowed &&
        RValue->bilop == SX_PROPERTY_REFERENCE &&
        RValue->AsPropertyReferenceExpression().Left &&
        RValue->AsPropertyReferenceExpression().Left->bilop == SX_SYM)
    {
        BaseReference = RValue->AsPropertyReferenceExpression().Left->AsSymbolReferenceExpression().BaseReference;

        return
            ExpressionRefersToReadonlyVariable(
                BaseReference,
                false); // Property Reference Not Allowed

    }
    else if (RValue->bilop == SX_SYM &&
             RValue->AsSymbolReferenceExpression().Symbol->IsVariable())
    {
        if (RValue->AsSymbolReferenceExpression().Symbol->PVariable()->IsReadOnly())
        {
            return true;
        }

        BaseReference = RValue->AsSymbolReferenceExpression().BaseReference;
    }

    if (BaseReference)
    {
        if (BaseReference->bilop == SX_ASG_RESADR &&
            HasFlag32(BaseReference, SXF_ASG_RESADR_READONLYVALUE))
        {
            return true;
        }

        if (BaseReference->bilop == SX_ADR)
        {
            BaseReference = BaseReference->AsExpressionWithChildren().Left;

            if (BaseReference->ResultType && TypeHelpers::IsValueType(BaseReference->ResultType))
            {
                return
                    ExpressionRefersToReadonlyVariable(
                        BaseReference,
                        false); // Property Reference Not Allowed
            }
        }
    }

    return false;
}

void
Semantics::ReportAssignmentToRValue
(
    ILTree::Expression *RValue
)
{
    unsigned ErrorId;

    if (IsConstant(RValue))
    {
        ErrorId = ERRID_CantAssignToConst;
    }
    else if (ExpressionRefersToReadonlyVariable(RValue))
    {
        ErrorId = ERRID_ReadOnlyAssignment;
    }

    else
    {
        ErrorId = ERRID_LValueRequired;
    }

    ReportSemanticError(ErrorId, RValue->Loc);
}

void
Semantics::InterpretCall
(
    _In_ ParseTree::CallStatement *UnboundCall
)
{
    ExpressionFlags ConstructorFlags = ExprNoFlags;
    bool ExpectingConstructorCall = false;

    if (m_ExpectingConstructorCall)
    {
        SetFlag(ConstructorFlags, ExprIsConstructorCall);
        m_ExpectingConstructorCall = false;
        ExpectingConstructorCall = true;
    }

    typeChars TypeCharacter = ExtractTypeCharacter(UnboundCall->Target);

    ILTree::Expression *Target =
        InterpretStatementOperand(
            UnboundCall->Target,
            (ExprIsExplicitCallTarget |
                ExprPropagatePropertyReference |
                ExprSuppressImplicitVariableDeclaration |
                ConstructorFlags));

    ILTree::Expression *Result = NULL;

    if (!IsBad(Target) && Target->bilop == SX_LATE_REFERENCE)
    {
        Result =
            InterpretLateBoundExpression(
                UnboundCall->TextSpan,
                Target->AsPropertyReferenceExpression(),
                UnboundCall->Arguments.Values,
                ExprResultNotNeeded);
    }

    // For IntelliSense, array references can sneak in here, for
    // statements that begin "a(".

    else if (m_PreserveExtraSemanticInformation &&
            !IsBad(Target) &&
            (Target->bilop != SX_SYM || !IsProcedure(Target->AsSymbolReferenceExpression().Symbol)) &&
            (Target->ResultType && TypeHelpers::IsArrayType(Target->ResultType)))
    {
        Result =
            InterpretArrayIndexReference(
                UnboundCall->TextSpan,
                Target,
                UnboundCall->Arguments);
    }

    else
    {
        // References to Me are not allowed in the arguments of a constructor call
        // if that call is the first statement in another constructor.

        if (ExpectingConstructorCall)
        {
            SetFlag(ConstructorFlags, ExprIsInitializationCall);
        }
        

        Result =
            BindArgsAndInterpretCallExpressionWithNoCopyOut(
            UnboundCall->TextSpan,
            Target,
            TypeCharacter,
            UnboundCall->Arguments.Values,
            ExprResultNotNeeded | ConstructorFlags,
            OvrldNoFlags,
            NULL);

    }

    ApplyContextSpecificSemantics(Result, ExprResultNotNeeded, NULL);

    if (IsBad(Result))
    {
        MarkContainingLambdaOrMethodBodyBad();
    }
    else
    {
        // Check for the conditional custom attribute, but only check if
        // the call isn't going to return anything.

        // Also check if we should warn for an unobserved call that returns an awaitable value.
        
        if (!m_IsGeneratingXML && Result->bilop == SX_CALL)
        {
            ILTree::SymbolReferenceExpression& symbolReference = Result->AsCallExpression().Left->AsSymbolReferenceExpression();
            Procedure *TargetProcedure = ViewAsProcedure(symbolReference.Symbol);

            if (m_ReportErrors && !UnboundCall->CallIsExplicit && TargetProcedure->GetType() != NULL && !TargetProcedure->GetType()->IsObject())
            {
                // We will show a warning:
                //   1. In any method, when invoking a method that is marked
                //      "Async" and returns a type other than void. Note that
                //      this requires the caller and callee to be in the same
                //      compilation unit, as "Async" is not propagated into
                //      metadata.
                //   2. In any method, when invoking a method that returns one
                //      of the Windows Runtime async types:
                //        IAsyncAction
                //        IAsyncActionWithProgress(Of T)
                //        IAsyncOperation(Of T)
                //        IAsyncOperationWithProgress(Of T, U)
                //   3. In an async method, when invoking a method that returns
                //      any awaitable type.

                bool WarnAboutAwaitability = false;

                if (TargetProcedure->IsAsyncKeywordUsed())
                {
                    bool targetInSameCompilationUnit = TargetProcedure->GetContainingProject() == m_ContainingClass->GetContainingProject();
                    WarnAboutAwaitability = targetInSameCompilationUnit;
                }

                if (!WarnAboutAwaitability)
                {
                    BCSYM* pTargetType = TargetProcedure->GetType();

                    if (pTargetType->IsGenericBinding())
                    {
                        pTargetType = pTargetType->PGenericBinding()->GetGeneric();
                    }

                    if ((GetFXSymbolProvider()->IsTypeAvailable(FX::IAsyncActionType) &&
                         IsOrInheritsFromOrImplements(pTargetType, GetFXSymbolProvider()->GetType(FX::IAsyncActionType))) ||
                        (GetFXSymbolProvider()->IsTypeAvailable(FX::IAsyncActionWithProgressType) &&
                         IsOrInheritsFromOrImplements(pTargetType, GetFXSymbolProvider()->GetType(FX::IAsyncActionWithProgressType))) ||
                        (GetFXSymbolProvider()->IsTypeAvailable(FX::IAsyncOperationType) &&
                         IsOrInheritsFromOrImplements(pTargetType, GetFXSymbolProvider()->GetType(FX::IAsyncOperationType))) ||
                        (GetFXSymbolProvider()->IsTypeAvailable(FX::IAsyncOperationWithProgressType) &&
                         IsOrInheritsFromOrImplements(pTargetType, GetFXSymbolProvider()->GetType(FX::IAsyncOperationWithProgressType))))
                    {
                        WarnAboutAwaitability = true;
                    }
                }

                ILTree::ResumableKind resumableKind = GetLocalResumableKind();
                if (!WarnAboutAwaitability && (resumableKind == ILTree::TaskResumable || resumableKind == ILTree::SubResumable))
                {
                    ILTree::Expression * pAwaitExpression = NULL;
                    {
                        BackupValue<ErrorTable*> backup_report_errors(&m_Errors);
                        ErrorTable Errors(m_Compiler, m_Project, NULL);
                        m_Errors = &Errors;

                        BCSYM* pType = ReplaceGenericParametersWithArguments(TargetProcedure->GetType(), symbolReference.GenericBindingContext, m_SymbolCreator);

                        Location loc = UnboundCall->Target->TextSpan;
                        ParserHelper ph(&m_TreeStorage, loc);
                        ParseTree::Expression *pAwaitableExpression = ph.CreateConversion(ph.CreateNothingConst(), ph.CreateBoundType(pType, loc));

                        pAwaitExpression = InterpretAwaitExpression(loc, pAwaitableExpression, ExprResultNotNeeded | ExprSpeculativeBind);
                        WarnAboutAwaitability = !IsBad(pAwaitExpression) && 
                            m_Errors->GetErrorCount() == 0 &&
                            pAwaitExpression->bilop == SX_AWAIT &&
                            pAwaitExpression->AsAwaitExpression().GetAwaiterDummy &&
                            pAwaitExpression->AsAwaitExpression().GetAwaiterDummy->bilop == SX_CALL;
                    }
                }

                if (WarnAboutAwaitability)
                {
                    ReportSemanticError(WRNID_UnobservedAwaitableExpression, UnboundCall->Target->TextSpan);
                }
            }

            if ( !IgnoreConditionalAttribute() &&
                TargetProcedure->GetType() == NULL &&
                !TargetProcedure->GetContainer()->IsInterface())
            {
                ConditionalString *CurrentString;
                bool FoundConstant = false;

                TargetProcedure->GetPWellKnownAttrVals()->GetConditionalData(&CurrentString);

                if (CurrentString)
                {
                    while (CurrentString)
                    {
                        bool NameIsBad = false;

                        Declaration *ConditionalResult =
                            EnsureNamedRoot
                            (
                                LookupName
                                (
                                    ViewAsScope(m_SourceFile->GetConditionalCompilationConstants()),
                                    NULL,   // No Type parameter lookup
                                    CurrentString->m_pstrConditional,
                                    NameSearchConditionalCompilation |
                                        NameSearchIgnoreImports |
                                        NameSearchIgnoreExtensionMethods,
                                    ContainingClass(),
                                    UnboundCall->TextSpan,
                                    NameIsBad,
                                    NULL,   // No binding context for conditional constants
                                    -1
                                )
                            );

                        // 






                        if (!NameIsBad && ConditionalResult == NULL)
                        {
                            // Try the project level CC constants.
                            ConditionalResult =
                                EnsureNamedRoot
                                (
                                    LookupName
                                    (
                                        ViewAsScope(m_SourceFile->GetProject()->GetProjectLevelCondCompScope()),
                                        NULL,   // No Type parameter lookup
                                        CurrentString->m_pstrConditional,
                                        NameSearchConditionalCompilation | NameSearchIgnoreImports | NameSearchIgnoreExtensionMethods,
                                        ContainingClass(),
                                        UnboundCall->TextSpan,
                                        NameIsBad,
                                        NULL,   // No binding context for conditional constants
                                        -1
                                    )
                                );
                        }

                        if (!NameIsBad &&
                            ConditionalResult &&
                            !IsBad(ConditionalResult))
                        {
                            ConstantValue Value =
                                ConditionalResult->PVariableWithValue()->GetExpression()->GetValue();

                            switch (Value.TypeCode)
                            {
                                case t_bool:
                                case t_i1:
                                case t_ui1:
                                case t_i2:
                                case t_ui2:
                                case t_i4:
                                case t_ui4:
                                case t_i8:
                                case t_ui8:

                                    FoundConstant = Value.Integral != 0;
                                    break;
                            }

                            if (FoundConstant)
                            {
                                break;
                            }
                        }

                        CurrentString = CurrentString->m_pConditionalStringNext;
                    }

                    if (!FoundConstant)
                    {
                        // The call should be ignored.
                        return;
                    }
                }
            }
        }

        ILTree::Statement *CallStatement = AllocateStatement(SL_STMT, UnboundCall);
        CallStatement->AsStatementWithExpression().Operand1 = Result;

        // If this is a valid call from a constructor to a constructor
        // of the same class, record the link from this constructor
        // to the called one to enable detection of cycles in calls
        // between constructors.
        //
        // If this is valid call from a constructor to a constructor of
        // the base class, record that this constructor does not call
        // another constructor of the same class.

        if (m_CompilingConstructorDefinition &&
            (Result->bilop == SX_CALL ||
            (Result->bilop == SX_SEQ && Result->AsExpressionWithChildren().Left->bilop == SX_CALL)))
        {
            // a byref param will add a SEQ note for copy back on top of the original call in result.
            // dig and check the original target (bug 575567)
            Procedure *TargetProcedure = (Result->bilop == SX_CALL) ?
                ViewAsProcedure(Result->AsCallExpression().Left->AsSymbolReferenceExpression().Symbol):
                ViewAsProcedure(Result->AsExpressionWithChildren().Left->AsCallExpression().Left->AsSymbolReferenceExpression().Symbol);

            if (TargetProcedure->IsInstanceConstructor())
            {
                m_ConstructorCycles->AddSymbolToVerify(
                    m_Procedure,
                    TargetProcedure->GetParent() == ContainingClass() ?
                        TargetProcedure :
                        NULL);

                // Non-shared fields get initialized immediately after the
                // call to this constructor. Skip the initializations if the call
                // is to another constructor of the same class, because that
                // constructor will have initialized the fields.

                if (TargetProcedure->GetParent() != ContainingClass())
                {
                    InitializeFields();
                }

                // Indicate that this statement is an initialization statement
                // in the constructor so that it is generated outside the try
                // block generated for the on error statement.
                // Bug VSWhidbey 204996
                //
                SetFlag32(CallStatement, SLF_INIT_ME);
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Type inference methods.
// ----------------------------------------------------------------------------

// Determine whether this expression can have it's type inferred.

bool
Semantics::OptionInferOn()
{
    if( m_SourceFile != NULL )
    {
        return( m_SourceFile->GetOptionFlags() & OPTION_OptionInfer );
    }
    else
    {
        return( false );
    }
}

bool
Semantics::OptionStrictOn()
{
    if( m_SourceFile != NULL )
    {
        return( m_SourceFile->GetOptionFlags() & OPTION_OptionStrict );
    }
    else
    {
        return( false );
    }
}

bool
Semantics::ShouldInfer(ILTree::Expression *Target)
{
    AssertIfNull( Target );

    return
        OptionInferOn() &&
        Target->bilop == SX_SYM &&
        Target->AsSymbolReferenceExpression().Symbol->IsVariable() &&
        Target->AsSymbolReferenceExpression().Symbol->PVariable()->IsLocal() &&
        !Target->AsSymbolReferenceExpression().Symbol->PVariable()->IsStatic() &&
        !Target->AsSymbolReferenceExpression().Symbol->PVariable()->IsExplicitlyTyped();
}

// Interpret the initializer - contains type inference and aggr initializer.

ILTree::Expression *
Semantics::InterpretInitializer
(
    _In_ ParseTree::Initializer *Init,
    _Inout_ ILTree::Expression *Target
)
{
    AssertIfFalse(ParseTree::Initializer::Deferred == Init->Opcode ||
                  ParseTree::Initializer::Expression == Init->Opcode);

    Location AssignmentTextSpan;
    Type *TargetType = Target->ResultType;
    Type *OldTargetType = TargetType;

    AssignmentTextSpan.m_lBegLine = Target->Loc.m_lBegLine;
    AssignmentTextSpan.m_lBegColumn = Target->Loc.m_lBegColumn;

    bool doInfer = false;

    if (Init->Opcode == ParseTree::Initializer::Deferred)
    {
        Init = Init->AsDeferred()->Value;
    }

    AssertIfFalse(ParseTree::Initializer::Expression == Init->Opcode);

    ParseTree::Expression *InitValue = Init->AsExpression()->Value;

    if (InitValue->Opcode == ParseTree::Expression::Deferred)
    {
        InitValue = InitValue->AsDeferred()->Value;
    }

    AssignmentTextSpan.m_lEndLine = Init->AsExpression()->Value->TextSpan.m_lEndLine;
    AssignmentTextSpan.m_lEndColumn = Init->AsExpression()->Value->TextSpan.m_lEndColumn;

    // Don't assume the type of the initializer if we're inferring from it

    if (ShouldInfer(Target) &&
        Target->AsSymbolReferenceExpression().Symbol->PVariable()->IsLocal() &&
        !Target->AsSymbolReferenceExpression().Symbol->PVariable()->IsStatic())
    {
        doInfer = true;
        TargetType = NULL;
    }

    if( doInfer && InitValue->Opcode == ParseTree::Expression::ArrayInitializer )
    {
        if
        (
            Target->bilop == SX_SYM &&
            !Target->AsSymbolReferenceExpression().Symbol->PVariable()->IsExplicitlyTyped() &&
            Target->AsSymbolReferenceExpression().Symbol->PVariable()->HasNullableDecoration()
        )
        {
            ReportSemanticError(ERRID_NullableTypeInferenceNotSupported, Target->Loc);
        }
    }

    // Turn an initialization into an assignment to the declared variable.

    InitializerInferInfo inferInfo( &m_InitializerInferStack );

    if( doInfer )
    {
        inferInfo.Variable = Target->AsSymbolReferenceExpression().Symbol->PVariable();
    }

    // Dev10 introduces the /langVersion switch so we need to flag situations where a previous
    // version of the compiler can't process a language construct.  In this case, we need to catch the case
    // of collection initializers.  This was not legal in < vb10: dim a = {1,2}  But this was: dim a()={1,2}
    // So here we are checking to see if the left hand side is an array or not because if it isn't, then we
    // are looking at collection initializer usage that wasn't legal before vb10
    // The problem is that we only have enough information at this stage of compilation to know what the
    // target context is.  It won't be until ConvertArrayLiteral() (eventually called by InterpretInitializer(),
    // below) that we can make use of this information so we set this flag now to track it.
    bool BackupInitializerTargetIsArray = m_InitializerTargetIsArray;
    m_InitializerTargetIsArray = Target->ResultType->IsArrayType();
    ILTree::Expression *InitialValue = InterpretInitializer(Init, TargetType);

    if (TargetType==NULL && InitialValue->bilop==SX_ARRAYLITERAL &&
        InitialValue->AsArrayLiteralExpression().ElementList==NULL &&
        Target->ResultType->IsArrayType() && Target->ResultType->PArrayType()->GetRank()>1)
    {
        // Dev10#491530: Dim x(,) = {}. For the special case of an empt array literal, we
        // should NOT interpret the array literal (which would give "assumed Object()").
        // Instead we should use the rank indicated by x, giving Object(,).
        // Note that "object assumed" errors must still be given.
        InitialValue = InterpretInitializer(Init, Target->ResultType);
    }
    m_InitializerTargetIsArray = BackupInitializerTargetIsArray;

    if (doInfer)
    {
        inferInfo.Restore();
    }

    if( InitialValue == NULL || IsBad( InitialValue ) )
    {
#if IDE
        if (m_PreserveExtraSemanticInformation)
        {
            if (doInfer && InitialValue)
            {
                InferVariableType(Target, InitialValue->ResultType);
            }
        }
#endif
        MarkContainingLambdaOrMethodBodyBad();
        return InitialValue;
    }

    if( inferInfo.CircularReferenceDetected )
    {
        AssertIfFalse( doInfer );
        // Error was reported in name lookup; don't write error here.
        return AllocateBadExpression(Target->Loc);
    }

    if( doInfer )
    {
        // 


        if( InitialValue->bilop == SX_ADDRESSOF )
        {
            InitialValue = ConvertWithErrorChecking( InitialValue, OldTargetType, ExprForceRValue );
        }

        if (InitialValue == NULL || IsBad(InitialValue))
        {
            return InitialValue;
        }

        Type * VariableType = InferVariableType(Target, InitialValue);

        if
        (
            VariableType &&
            (
                TypeHelpers::IsNullableType(VariableType, m_CompilerHost) ||
                InitialValue->bilop == SX_ARRAYLITERAL
            ) &&
            !BCSYM::AreTypesEqual(VariableType, InitialValue->ResultType)
        )
        {
            InitialValue = ConvertWithErrorChecking(InitialValue, VariableType, ExprForceRValue);
        }
        else if (VariableType && TypeHelpers::IsArrayType(OldTargetType) && TypeHelpers::IsArrayType(InitialValue->ResultType))
        {
            // For arrays, we need to check a potential conversion.
            ConvertWithErrorChecking( InitialValue, Target->ResultType, ExprForceRValue );
        }
    }

    // Check if we need to generate a warning/error.

    if
    (
        !OptionStrictOn() &&
        ErroneousTypeInference(Target) &&
        Target->AsSymbolReferenceExpression().Symbol->PVariable()->HasNullableDecoration() &&
        (
            !OptionInferOn() ||
            !Target->AsSymbolReferenceExpression().Symbol->GetParent()->IsProc()
        )
    )
    {
        ReportSemanticError
        (
            ERRID_NullableTypeInferenceNotSupported,
            Target->Loc
        );
    }
    else if( OptionInferOn() && ErroneousTypeInference( Target ) && ! Target->AsSymbolReferenceExpression().Symbol->PVariable()->SuppressAsClauseMessages())
    {
        AssertIfFalse( Target->bilop == SX_SYM );
        if (m_SourceFile->GetOptionFlags() & OPTION_OptionStrict)
        {
            ReportSemanticError(ERRID_StrictDisallowImplicitObject,
                                *Target->AsSymbolReferenceExpression().Symbol->PVariable()->GetLocation() );
        }
        else
        {
            StringBuffer buf;
            ReportSemanticError(WRNID_ObjectAssumedVar1,
                                *Target->AsSymbolReferenceExpression().Symbol->PVariable()->GetLocation(),
                                ResLoadString(WRNID_MissingAsClauseinVarDecl, &buf) );

        }
    }

    return
        GenerateAssignment(
            AssignmentTextSpan,
            Target,
            InitialValue,
            false);
}

// Determine whether the expression is a bad type inference.

bool Semantics::ErroneousTypeInference(ILTree::Expression * expression)
{
    // Static's already have warnings generated.

    bool bRet = false;

    if( expression->bilop == SX_SYM &&
        expression->AsSymbolReferenceExpression().Symbol->IsVariable() &&
        ! expression->AsSymbolReferenceExpression().Symbol->PVariable()->IsExplicitlyTyped() )
    {
        // We don't have an explicit type, but statics already have warnings generated.
        if( ! expression->AsSymbolReferenceExpression().Symbol->IsStaticLocalBackingField() )
        {
            bRet = true;
        }
    }

    return( bRet );
}

// Coerce the target expression's type to the result type.

void
Semantics::CoerceType(_Inout_ ILTree::Expression* Target, _In_ BCSYM* ResultType)
{
    AssertIfNull( Target );
    AssertIfNull( ResultType );
    VSASSERT( Target->bilop == SX_SYM, "Cannot coerce into a non-symbol type" );
    VSASSERT( Target->AsSymbolReferenceExpression().Symbol->IsVariable(), "Cannot coerce into a non-variable" );

    // Infer the type of the result expression
    Target->ResultType = ResultType;

    // Transfer the vtype
    Target->AsExpression().vtype = ResultType->GetVtype();

    // Infer the type of the variable
    Target->AsSymbolReferenceExpression().Symbol->PVariable()->SetType(ResultType);
    Target->AsSymbolReferenceExpression().Symbol->PVariable()->SetExplicitlyTyped(true);
}


// Infer a variable's type by looking at the bound tree representation
// of the initializer. It will set the type in the Target parameter
// and return the BCSYM of the type.


Type *
Semantics::InjectNullableIntoArray
(
    Type * pType
)
{

    if (pType->IsArrayType())
    {
        ArrayType * pArrayType = pType->PArrayType();
        return
            m_SymbolCreator.GetArrayType
            (
                pArrayType->GetRank(),
                InjectNullableIntoArray(pArrayType->GetRoot())
            );
    }
    else if (TypeHelpers::IsNullableType(pType, m_CompilerHost))
    {
        return pType;
    }
    else
    {
        return
            m_SymbolCreator.LiftType
            (
                pType,
                m_CompilerHost
            );
    }

}

BCSYM *
Semantics::InferVariableType
(
    _Inout_ ILTree::Expression * Target, 
    ILTree::Expression *InitialValue
)
{
    AssertIfNull( Target );

    if (!InitialValue || IsBad(InitialValue))
    {
        return NULL;
    }
    else if (InitialValue->bilop == SX_NOTHING && Target->ResultType!=NULL && Target->ResultType->IsArrayType())
    {
        // e.g. "Dim x() = Nothing". Conviently here, "x" has already provisionally been given Object() type.
        // e.g. "Dim x()() = Nothing". Conveniently here, "x" has already been provisionally given Object()() type.
        return Target->ResultType;
    }
    else
    {
        return InferVariableType(Target, InitialValue->ResultType);
    }
}


// Coerce a variable to type specific type passed in ResultType. We will do basic
// validation of the ResultType and if it is good, then we will assign the
// type into Target and return the same type.

BCSYM*
Semantics::InferVariableType
(
    _Inout_ ILTree::Expression * Target, 
    BCSYM * ResultType
)
{
    AssertIfNull( Target );
    AssertIfNull(ResultType);

    if (TypeHelpers::IsVoidArrayLiteralType(ResultType))
    {
        ReportSemanticError(ERRID_ArrayInitNoType, ResultType->PArrayLiteralType()->GetLiteralLocation());
        return NULL;
    }
    else
    {
        ResultType = ResultType->DigThroughArrayLiteralType(&m_SymbolCreator);
    }    

    if (IsBad(Target) || Target->bilop != SX_SYM)
    {
        return NULL;
    }

    if (ResultType &&
        Target->AsSymbolReferenceExpression().Symbol->PVariable()->IsLocal() &&
        !Target->AsSymbolReferenceExpression().Symbol->PVariable()->IsStatic())
    {

        if
        (
            Target->AsSymbolReferenceExpression().Symbol->PVariable()->HasNullableDecoration() &&
            ResultType->IsArrayType() &&
            !Target->ResultType->IsArrayType ()
        )
        {
            ReportSemanticError
            (
                ERRID_CannotInferNullableForVariable1,
                Target->AsSymbolReferenceExpression().Symbol->PVariable()->GetLocation(),
                Target->AsSymbolReferenceExpression().Symbol
            );

            return NULL;
        }
        else if (Target->ResultType->IsArrayType() && !ResultType->IsArrayType())
        {
            ReportSemanticError(
                ERRID_InferringNonArrayType1,
                *Target->AsSymbolReferenceExpression().Symbol->PVariable()->GetLocation(),
                ResultType);

            return NULL;
        }
        else if
        (
            Target->ResultType->IsObject() &&
            Target->AsSymbolReferenceExpression().Symbol->PVariable()->HasNullableDecoration()
        )
        {
          
            // MQ Bug 899116
            // ResultType->IsGenericParam() is added to by pass generic parameter
            // the error will be caught in ValidateGenericArguments, then in ValidateValueConstraintForType.
            if (!IsValidInLiftedSignature(ResultType) && !ResultType->IsGenericParam())
            {
                ReportSemanticError
                (
                    ERRID_CannotInferNullableForVariable1,
                    Target->AsSymbolReferenceExpression().Symbol->PVariable()->GetLocation(),
                    Target->AsSymbolReferenceExpression().Symbol
                );

                return NULL;
            }
            else if (TypeHelpers::IsNullableType(ResultType, m_CompilerHost))
            {
                CoerceType(Target, ResultType);
                return ResultType;
            }
            else
            {
                if (GetFXSymbolProvider()->IsTypeAvailable(FX::GenericNullableType))
                {

                    bool ignored = false;
                    if
                    (
                        ValidateGenericArguments
                        (
                            Target->Loc,
                            GetFXSymbolProvider()->GetType(FX::GenericNullableType),
                            &ResultType,
                            &Target->Loc,
                            1,
                            NULL,
                            ignored
                        )
                    )
                    {
                        ResultType = m_SymbolCreator.LiftType(ResultType, m_CompilerHost);

                        CoerceType(Target, ResultType);
                        return ResultType;
                    }
                }

                return NULL;
            }

        }
        else if
        (
            Target->ResultType->IsObject()
        )
        {                
            CoerceType( Target, ResultType );
            return ResultType;

        }
        else if
        (
            ResultType->IsArrayType() &&
            Target->ResultType->IsArrayType()
        )
        {
            if
            (
                Target->AsSymbolReferenceExpression().Symbol->PVariable()->HasNullableDecoration() &&
                !(TypeHelpers::IsNullableType(ResultType->ChaseToType(), m_CompilerHost))
            )
            {
                ReportSemanticError
                (
                    ERRID_CannotInferNullableForVariable1,
                    Target->AsSymbolReferenceExpression().Symbol->PVariable()->GetLocation(),
                    Target->AsSymbolReferenceExpression().Symbol
                );

                Target->AsSymbolReferenceExpression().Symbol->PVariable()->SetSuppressAsClauseMessages();

                return NULL;
            }
            else if (Target->ResultType->PArrayType()->GetRank() != ResultType->PArrayType()->GetRank())
            {
                ReportSemanticError
                (
                    ERRID_TypeInferenceArrayRankMismatch1,
                    Target->AsSymbolReferenceExpression().Symbol->PVariable()->GetLocation(),
                    Target->AsSymbolReferenceExpression().Symbol
                );

                Target->AsSymbolReferenceExpression().Symbol->PVariable()->SetSuppressAsClauseMessages();

                return NULL;
            }


            // For arrays, if both the LHS and RHS specify an array shape, we
            // have to allow inference to infer "arrays" when possible. That is, if
            // we consider matching the shape from left to right, if we "consume" the arrays
            // of the RHS and have "extra" arrays left over, we infer that.
            //
            // For example:
            // dim x() = new Integer()() - we infer integer()
            // dim x(,)() = new Integer(,)()(,) - we infer integer()(,)
            //
            // The algorithm used is to match the array rank until we exhaust the LHS,
            // and if the RHS has more arrays, simply infer the RHS into the LHS. If the RHS
            // "runs out of arrays" before the LHS, then we just infer the base type and give
            // a conversion error.

            BCSYM* symLHS = Target->ResultType;
            BCSYM* symRHS = ResultType;

            while( true )
            {
                VSASSERT( symLHS != NULL, "How did we chase to NULL?" );
                VSASSERT( symRHS != NULL, "How did we chase to NULL?" );

                if( TypeHelpers::IsArrayType( symLHS ) && TypeHelpers::IsArrayType( symRHS ) )
                {
                    if( symLHS->PArrayType()->GetRank() == symRHS->PArrayType()->GetRank() )
                    {
                        symLHS = symLHS->PArrayType()->GetRoot();
                        symRHS = symRHS->PArrayType()->GetRoot();
                    }
                    else
                    {
                        break;
                    }
                }
                else if( TypeHelpers::IsArrayType( symRHS ) || ! TypeHelpers::IsArrayType(symLHS))
                {
                    AssertIfTrue( TypeHelpers::IsArrayType( symLHS ) );

                    CoerceType( Target, ResultType );
                    return ResultType;
                }
                else
                {
                    AssertIfTrue( TypeHelpers::IsArrayType( symRHS ) );

                    break;
                }
            }

            AssertIfFalse( symLHS->IsArrayType() );


            // Chase to the last array type on LHS and coerce the base type.

            while( TypeHelpers::IsArrayType( symLHS->PArrayType()->GetRoot() ) )
            {
                symLHS = symLHS->PArrayType()->GetRoot();
            }

            AssertIfFalse( TypeHelpers::IsArrayType( symLHS ) );
            AssertIfTrue( TypeHelpers::IsArrayType( symLHS->PArrayType()->GetRoot() ) );

            symLHS->PArrayType()->SetElementType(symRHS->ChaseToType());

            Target->AsExpression().vtype = Target->ResultType->GetVtype();
            Target->AsSymbolReferenceExpression().Symbol->PVariable()->SetType(Target->ResultType);
            Target->AsSymbolReferenceExpression().Symbol->PVariable()->SetExplicitlyTyped(true);
            return Target->ResultType;
        }
    }

    return NULL;
}



Type*
Semantics::InferLambdaReturnTypeFromArguments
(
    _In_opt_ Parameter* parameters,
    _In_ ILTree::UnboundLambdaExpression *unboundLambda
)
{
    VSASSERT(unboundLambda->IsFunctionLambda, "Must be function lambda here!");

    Type *lambdaReturnType = NULL;
    // Infer the returntype of the expression on a clone of the lambda with inferred arguments.
    ILTree::UnboundLambdaExpression * cloneUnboundLambda = ShallowCloneUnboundLambdaExpression(unboundLambda);
    cloneUnboundLambda->FirstParameter = parameters;

    BackupValue<bool> backup_report_errors(&m_ReportErrors);
    m_ReportErrors = false;

    BackupValue<TriState<bool>> backup_lambdareporting(&m_ReportMultilineLambdaReturnTypeInferenceErrors);
    if (!m_ReportMultilineLambdaReturnTypeInferenceErrors.HasValue())
    {
        // Exactly the same as we do for multiline lambdas (see comments in function immediately below)
        m_ReportMultilineLambdaReturnTypeInferenceErrors.SetValue(true);
    }

    // Interpret the unbound lambda without error reporting to determine the type inside
    // if it fails we will assume object
    ILTree::LambdaExpression * boundLambda = InterpretUnboundLambda(cloneUnboundLambda, NULL);

    if (boundLambda && !IsBad(boundLambda) && !IsBad(boundLambda->GetExpressionLambdaBody()))
    {
        lambdaReturnType = boundLambda->GetExpressionLambdaBody()->ResultType;
    }
    else
    {
        lambdaReturnType = GetFXSymbolProvider()->GetObjectType();
    }

    return lambdaReturnType;
}

Type*
Semantics::InferMultilineLambdaReturnTypeFromReturnStatements
(
    _In_ ILTree::UnboundLambdaExpression *unboundLambda
)
{
    VSASSERT(unboundLambda->IsFunctionLambda, "Must be function lambda here!");
    VSASSERT(!unboundLambda->IsSingleLine || unboundLambda->IsAsyncKeywordUsed, "Must be singla line async function lambda here!");


    Type *lambdaReturnType = NULL;
    // Infer the returntype of the expression on a clone of the lambda with inferred arguments.
    ILTree::UnboundLambdaExpression * cloneUnboundLambda = ShallowCloneUnboundLambdaExpression(unboundLambda);

    BackupValue<bool> backup_report_errors(&m_ReportErrors);

    // A state variable that can get changed while we interpret the lambda block.
    InitializerInferInfo inferInfo( &m_InitializerInferStack );    

    m_ReportErrors = false;

    BackupValue<TriState<bool>> backup_lambdareporting(&m_ReportMultilineLambdaReturnTypeInferenceErrors);
    if (!m_ReportMultilineLambdaReturnTypeInferenceErrors.HasValue())
    {
        // Only set m_ReportMultilineLambdaReturnTypeInferenceErrors to true if we are inferring the return type of the
        // outermost multiline function lambda that requires return type inference. Otherwise we will report duplicate type inference errors. 
        //
        // Example:
        //
        // Dim a = Function()
        //             Dim b = Function()
        //                          Dim c = Function()
        //                                 End Function
        //                     End Function
        //         End Function
        //
        // We interpret each lambda in its entirety once to infer its return type and once to compile it. Thus, Dim a's lambda
        // is interpretted twice; however, Dim b's lambda is iterpretted twice (infer return type/compile) when a is interpretted to infer
        // its return type and b is interpretted twice when a is interpretted to be compiled. Following this exponential pattern, Dim c's lambda
        // is interpetted 8 times. Half of these interpretations result in us inferring the lambdas return type; thus, Dim c's lambda  passes through
        // InferMultilineLambdaReturnTypeFromReturnStatements 4 times. We obviously only want to report any type inference errors once for each lambda.
        //
        // To do this , we set m_ReportMultilineLambdaReturnTypeInferenceErrors true if and only if it has no value (meaning it has not been set before) and
        // m_ReportMultilineLambdaReturnTypeInferenceErrors is set to false if we are interpretting the lambda for compilation AFTER having interpretted it for
        // type inference. However, if we do not need to perform type inference because we have an explicity type, or the lambda is a Sub(), then 
        // m_m_ReportMultilineLambdaReturnTypeInferenceErrors  keeps its original value through compilations. This algorithm ensures we do not report
        // duplicate errors.
        //
        // Note: This only applies to function lambdas that do not have an explicit target type (e.g. Dim x As Func(Of Integer) = Function()
        //                                                                                                                        End Function
        //       has an explicity target type of Func(Of Integer).
        m_ReportMultilineLambdaReturnTypeInferenceErrors.SetValue(true);
    }

    // Interpret the unbound lambda without error reporting to determine the type inside
    // if it fails we will assume object. When interpretting the lambda body we use the Object target type.
    // This is necessary to deal with cases like Dev10 bug 474772.
    ILTree::LambdaExpression * boundLambda = InterpretUnboundLambda(cloneUnboundLambda, 
                                                                    NULL,
                                                                    NULL, 
                                                                    true);


    if (!boundLambda || IsBad(boundLambda) || IsBad(boundLambda->GetStatementLambdaBody()))
    {

        Type *defaultType = GetFXSymbolProvider()->GetObjectType();

        if (boundLambda && 
            boundLambda->IsAsyncKeywordUsed && 
            GetFXSymbolProvider()->IsTypeAvailable(FX::GenericTaskType))
        {  
            defaultType = m_SymbolCreator.GetGenericTypeInsantiation(
                GetFXSymbolProvider()->GetType(FX::GenericTaskType),
                defaultType);
        }
        else if (boundLambda && 
            boundLambda->IsIteratorKeywordUsed && 
            GetFXSymbolProvider()->IsTypeAvailable(FX::GenericIEnumerableType))
        {
            defaultType = m_SymbolCreator.GetGenericTypeInsantiation(
                GetFXSymbolProvider()->GetType(FX::GenericIEnumerableType),
                defaultType);
        }   

        return defaultType;
    }

    NorlsAllocWrapper norlsWrapper(&m_TreeStorage);

    // Visit all the return or yield expressions and gather then up in "expressionList":
    ArrayList<ILTree::Expression*> expressionList;
    MultilineLambdaReturnExpressionVisitor visitor(&expressionList, this, &m_TreeStorage, &norlsWrapper);
    backup_report_errors.Restore();
    if (boundLambda->IsIteratorKeywordUsed)
    {
        visitor.VisitYieldExpressions(boundLambda->GetStatementLambdaBody());
    }
    else        
    {
        visitor.VisitReturnExpressions(boundLambda->GetStatementLambdaBody());
    }
        
    // Use the standard InferDominantTypeOfExpressions algorithm to find the dominant type:
    ConstIterator<ILTree::Expression*> iterator(expressionList.GetIterator());
    unsigned NumCandidates = 0;
    ILTree::Expression *winner = NULL;
    lambdaReturnType = InferDominantTypeOfExpressions(NumCandidates, winner, iterator);

    // Error-checking on the dominant type:

    bool strict = m_UsingOptionTypeStrict;
    bool custom = !strict && WarnOptionStrict();
    bool report = m_ReportMultilineLambdaReturnTypeInferenceErrors.HasValue() &&
                  m_ReportMultilineLambdaReturnTypeInferenceErrors.GetValue() &&
                  m_Errors != NULL;
    Location loc = boundLambda->GetStatementLambdaBody()->Loc;
    
    BackupValue<bool> backupReportErrors(&m_ReportErrors);
    m_ReportErrors = report;
    // Note: all the following lines which do "ReportSemanticError": if !m_ReportErrors, then
    // those calls will do nothing.

    bool canInferIEnumerable = (boundLambda->IsIteratorKeywordUsed && GetFXSymbolProvider()->IsTypeAvailable(FX::GenericIEnumerableType));
    bool canInferTask = (boundLambda->IsAsyncKeywordUsed &&
                         GetFXSymbolProvider()->IsTypeAvailable(FX::TaskType) && GetFXSymbolProvider()->IsTypeAvailable(FX::AsyncTaskMethodBuilderType) &&
                         GetFXSymbolProvider()->IsTypeAvailable(FX::GenericTaskType) && GetFXSymbolProvider()->IsTypeAvailable(FX::GenericAsyncTaskMethodBuilderType) &&
                         GetFXSymbolProvider()->IsTypeAvailable(FX::AsyncVoidMethodBuilderType) &&
                         GetFXSymbolProvider()->IsTypeAvailable(FX::IAsyncStateMachineType) &&
                         GetFXSymbolProvider()->IsTypeAvailable(FX::INotifyCompletionType) &&
                         GetFXSymbolProvider()->IsTypeAvailable(FX::ICriticalNotifyCompletionType));

    Type *defaultType = GetFXSymbolProvider()->GetObjectType();
    if (canInferIEnumerable || canInferTask)
    {
        defaultType = m_SymbolCreator.GetGenericTypeInsantiation(
            GetFXSymbolProvider()->GetType(canInferTask ? FX::GenericTaskType : FX::GenericIEnumerableType),
            defaultType);
    }

    if (boundLambda->IsIteratorKeywordUsed && !canInferIEnumerable)
    {
        // No need to report an error here. That will come later from IndicateLocalResumable.
        return defaultType;
    }

    else if (boundLambda->IsAsyncKeywordUsed && !canInferTask)
    {
        // No need to report an error here. That will come later from IndicateLocalResumable.
        return defaultType;
    }

    else if (IsRestrictedType(lambdaReturnType, m_CompilerHost))
    {
        // "'|1' cannot be made nullable, and cannot be used as the data type of an array element, field, anonymous type member, type argument, 'ByRef' parameter, or return statement."
        ReportSemanticError(ERRID_RestrictedType1, loc, lambdaReturnType);
        return defaultType;
    }

    else if (lambdaReturnType->IsVoidType())
    {
        // "Cannot infer a return type. Specifying the return type might correct this error."
        ReportSemanticError(ERRID_LambdaNoType, loc);
        return defaultType;
    }

    else if (expressionList.Count()==0 && canInferTask)
    {
        // It's fine if there were no return statements in an Async Function.
        // It simply returns "Task".
        // NB. This is keyed off expressionList.Count, rather than numCandidates.
        // Hence "Dim f = Async Function() : return Nothing : End Function" won't just assume Task as its return type.
        lambdaReturnType = GetFXSymbolProvider()->GetType(FX::TaskType);
    }

    else if (NumCandidates==0 && strict)
    {
        // "Cannot infer a return type, and Option Strict On does not allow 'Object' to be assumed. Specifying the return type might correct this error."
        ReportSemanticError(ERRID_LambdaNoTypeObjectDisallowed, loc);
        return defaultType;
    }

    else if (NumCandidates==0 && custom)
    {
        // "Cannot infer a return type; 'Object' assumed."
        StringBuffer buf;
        ReportSemanticError(WRNID_ObjectAssumed1, loc, ResLoadString(WRNID_LambdaNoTypeObjectAssumed, &buf));
        // This is just a warning; we still continue.
        lambdaReturnType = defaultType;
    }

    else if (NumCandidates==0)
    {
        lambdaReturnType = defaultType;
    }

    else if (NumCandidates>1 && strict)
    {
        // "Cannot infer a return type because more than one type is possible. Specifying the return type might correct this error."
        ReportSemanticError(ERRID_LambdaTooManyTypesObjectDisallowed, loc);
        return defaultType;
    }

    else if (NumCandidates>1 && custom)
    {
        // "Cannot infer a return type because more than one type is possible; 'Object' assumed."
        StringBuffer buf;
        ReportSemanticError(WRNID_ObjectAssumed1, loc, ResLoadString(WRNID_LambdaTooManyTypesObjectAssumed, &buf));
        lambdaReturnType = defaultType;
    }

    else if (NumCandidates>1)
    {
        lambdaReturnType = defaultType;
    }

    else if (NumCandidates==1 && (canInferIEnumerable || canInferTask))
    {
        lambdaReturnType = m_SymbolCreator.GetGenericTypeInsantiation(
            GetFXSymbolProvider()->GetType(canInferTask ? FX::GenericTaskType : FX::GenericIEnumerableType),
            lambdaReturnType);
    }

    else if (NumCandidates==1)
    {
        // lambdaReturnType is fine as it is in this case
    }


    // Microsoft: 












    backupReportErrors.Restore();


    return lambdaReturnType;
}



// InferLambdaType: this never returns NULL, except in one case (where one of the parameters is
// a query iteration variable).
BCSYM*
Semantics::InferLambdaType
(
    _Inout_ ILTree::UnboundLambdaExpression *UnboundLambda,
    _In_  Location &Location,
    _Inout_ bool *pRanDominantTypeAlgorithm
)
{
    VSASSERT(TypeHelpers::IsVoidType(UnboundLambda->ResultType) || UnboundLambda->ResultType->IsAnonymousDelegate(), "Expecting to Infer only once and all lambda's to be generated with VoidType. There are others who assume VoidType too so assert for now.");

    BCSYM_Param * FirstParam = NULL;
    BCSYM_Param * LastParam = NULL;

    // Do not Infer Anonymous Delegate Type for Lamdas built by Queries
    FirstParam = UnboundLambda->FirstParameter;

    while (FirstParam != NULL)
    {
        if(FirstParam->IsQueryIterationVariable())
        {
            return NULL;
        }

        FirstParam = FirstParam->GetNext();
    }

    // Clone the parameters and infer object for the typeless parameters.
    FirstParam = Declared::CloneParameters(UnboundLambda->FirstParameter, false, &LastParam, &m_TransientSymbolCreator);
    BCSYM_Param* CurrentParam = FirstParam;
    while (CurrentParam != NULL)
    {
        if (CurrentParam->GetType() == NULL)
        {
            Type* ParameterType = GetFXSymbolProvider()->GetType(FX::ObjectType);
            CurrentParam->SetType(ParameterType);
        }
        CurrentParam = CurrentParam->GetNext();
    }

    BackupValue<ExpressionFlags> oldExpressionFlags(&UnboundLambda->InterpretBodyFlags);
    UnboundLambda->InterpretBodyFlags |= ExprInferResultTypeExplicit;

    Type * LambdaReturnType = NULL;

    if (UnboundLambda->IsFunctionLambda)
    {
        if (UnboundLambda->IsStatementLambda && UnboundLambda->GetLambdaStatement()->pReturnType)
        {
            // This lambda is of the form "Function(Parameters) As Type"
            bool typeIsBad;
            LambdaReturnType = InterpretTypeName(UnboundLambda->GetLambdaStatement()->pReturnType, typeIsBad);
        }
        else if ((UnboundLambda->IsStatementLambda || UnboundLambda->IsAsyncKeywordUsed) && UnboundLambda->IsFunctionLambda)
        {
            // This lambda is of the form "Function(Parameters)"
            
            // When interpretting the return type of the lambda we want all typeless parameters to be
            // inferred to object.
            BackupValue <BCSYM_Param*> backupFirstParam(&UnboundLambda->FirstParameter);
            UnboundLambda->FirstParameter = FirstParam;

            LambdaReturnType = InferMultilineLambdaReturnTypeFromReturnStatements(UnboundLambda);

            if (pRanDominantTypeAlgorithm)
            {
                // InferMultilineLambdaReturnTypeFromReturnStatements runs the dominant type algorithm.
                // So we set this bool to true.
                *pRanDominantTypeAlgorithm = true;
            }
        }
        else
        {
            LambdaReturnType = InferLambdaReturnTypeFromArguments(FirstParam, UnboundLambda);
        }
    }

    Type* ResultType = GetInstantiatedAnonymousDelegate(
        FirstParam,
        LambdaReturnType,
        UnboundLambda->IsFunctionLambda,
        Location);

    return ResultType;
}

void
Semantics::InterpretGotoStatement
(
    _In_ ParseTree::LabelReferenceStatement *Goto
)
{
    if (!Goto->Label.IsBad)
    {
        ILTree::GotoStatement *Result = &AllocateStatement(SL_GOTO, Goto)->AsGotoStatement();
        bool GotoExitsTry = false;
        bool GotoExitsCatch = false;
        bool GotoExitsCatchToCorrespondingTry = false;
        Result->Label = ReferToLabel(Goto, &GotoExitsTry, &GotoExitsCatch, &GotoExitsCatchToCorrespondingTry);

        if (GotoExitsTry)
        {
            SetFlag32(Result, SLF_EXITS_TRY);
        }

        if (GotoExitsCatch)
        {
            SetFlag32(Result, SLF_EXITS_CATCH);

            if (GotoExitsCatchToCorrespondingTry)
            {
                SetFlag32(Result, SLF_EXITS_CATCH_TO_CORRESPONDING_TRY);
            }
        }

        if (WithinProcedure() && (GotoExitsTry || GotoExitsCatch))
        {
            Result->Label->IsBranchTargetFromEHContext = true;
        }
    }
}

void
Semantics::InterpretLabelStatement
(
    _Inout_ ParseTree::LabelReferenceStatement *Label
)
{
    if (!Label->Label.IsBad)
    {
        ILTree::LabelStatement *Result = &AllocateStatement(SL_LABEL, Label)->AsLabelStatement();
        Result->Label = GetLabelForDefinition(Label);

        if (Result->Label->IsDefined)
        {
            ReportSemanticError(
                ERRID_MultiplyDefined1,
                Label->TextSpan,
                Label->Label.Name);
        }

        Result->Label->IsDefined = true;

        if (Label->LabelIsLineNumber)
        {
            if (WithinProcedure())
            {
                m_ProcedureTree->fSeenLineNumber = true;
            }
            Result->Label->IsLineNumber = true;
        }

        Result->Label->pLabelParent = Result;
    }
}

// Returns True if the ExpectedParent contains the Child
static bool
Contains
(
    ParseTree::BlockStatement *ExpectedParent,
    _In_ ParseTree::Statement *Child
)
{
    for (ParseTree::BlockStatement *Parent = Child->GetParent(); Parent; Parent = Parent->GetParent())
    {
        if (Parent == ExpectedParent)
        {
            return true;
        }
    }

    return false;
}

static bool
IsValidBranchTarget
(
    ParseTree::BlockStatement *ExpectedParent,
    _In_ ParseTree::Statement *Child
)
{
    for (ParseTree::BlockStatement *Parent = Child->GetParent(); Parent; Parent = Parent->GetParent())
    {
        if (Parent == ExpectedParent)
        {
            return true;
        }

        // A Catch statement is contained by its associated Try statement.  This is a special
        // case because the Try statement is not accessible through the Parent field.

        if (Parent->Opcode == ParseTree::Statement::Catch &&
            Parent->AsCatch()->ContainingTry == ExpectedParent)
        {
            return true;
        }
    }

    return false;
}

LabelDeclaration *
Semantics::GetLabel
(
    _In_ ParseTree::LabelReferenceStatement *LabelReference,
    bool IsDefinition,
    _Out_ bool *ExitsTry,
    _Out_ bool *ExitsCatch,
    _Out_ bool *ExitsCatchToCorrespondingTry
)
{
    const ParseTree::IdentifierDescriptor &LabelName = LabelReference->Label;

    VSASSERT(!LabelName.IsBad, "Attempted reference to bad label.");
    VSASSERT(IsDefinition ^ !!ExitsTry, "Must have one or other, not both");
    VSASSERT((!!ExitsTry == !!ExitsCatch) && (!!ExitsTry == !!ExitsCatchToCorrespondingTry), "Must have both or neither");

    unsigned LabelBucketIndex =
        StringPool::HashValOfString(LabelName.Name) % m_LabelHashBucketCount;
    LabelDeclarationList *LabelBucket =
        m_LabelHashBuckets[LabelBucketIndex];

    for (LabelDeclarationList *Candidate = LabelBucket;
         Candidate;
         Candidate = Candidate->Next)
    {
        if (StringPool::IsEqual(Candidate->Name, LabelName.Name))
        {
            if (!IsDefinition)
            {
                ParseTree::Statement *LabelDefinition = (ParseTree::Statement *)Candidate->DefiningStatement;

                // Determine if the reference is a branch that crosses
                // into a Try/Catch/Finally or With statement.

                for (ParseTree::BlockStatement *LabelParent = LabelDefinition->GetParent();
                     LabelParent;
                     LabelParent = LabelParent->GetParent())
                {
                    ERRID BranchError = ERRID_None;

                    switch (LabelParent->Opcode)
                    {
                        case ParseTree::Statement::Try:
                        case ParseTree::Statement::Catch:
                        case ParseTree::Statement::Finally:
                            BranchError = ERRID_GotoIntoTryHandler;
                            break;

                        case ParseTree::Statement::Using:
                            BranchError = ERRID_GotoIntoUsing;
                            break;

                        case ParseTree::Statement::SyncLock:
                            BranchError = ERRID_GotoIntoSyncLock;
                            break;

                        case ParseTree::Statement::With:
                            BranchError = ERRID_GotoIntoWith;
                            break;

                        case ParseTree::Statement::ForFromTo:
                        case ParseTree::Statement::ForEachIn:
                            BranchError = ERRID_GotoIntoFor;
                            break;

                        default:
                            break;
                    }

                    if (BranchError)
                    {
                        if (!IsValidBranchTarget(LabelParent, LabelReference))
                        {
                            ReportSemanticError(
                                BranchError,
                                LabelName.TextSpan,
                                LabelName.Name);
                        }

                        // If the innermost construct contains the reference, outer ones
                        // do also, so further checking is unnecessary. If there was an
                        // error, one diagnostic is enough.

                        break;
                    }
                }

                // Determine if this is a branch that exits a Try/Catch/Finally.

                for (ParseTree::BlockStatement *ReferenceParent = LabelReference->GetParent();
                     ReferenceParent;
                     ReferenceParent = ReferenceParent->GetParent())
                {
                    if (ReferenceParent->Opcode == ParseTree::Statement::Try ||
                        ReferenceParent->Opcode == ParseTree::Statement::Catch ||
                        ReferenceParent->Opcode == ParseTree::Statement::Finally ||
                        ReferenceParent->Opcode == ParseTree::Statement::Using ||
                        ReferenceParent->Opcode == ParseTree::Statement::SyncLock ||
                        ReferenceParent->Opcode == ParseTree::Statement::ForEachIn ||
                        ReferenceParent->Opcode == ParseTree::Statement::LambdaBody)
                    {
                        if (Contains(ReferenceParent, LabelDefinition))
                        {
                            // If the reference parent contains the label definition, so
                            // do containing parents.

                            break;
                        }
                        else
                        {
                            if (ReferenceParent->Opcode == ParseTree::Statement::Finally)
                            {
                                ReportSemanticError(
                                    ERRID_BranchOutOfFinally,
                                    LabelName.TextSpan);

                                break;
                            }
                            else if (ReferenceParent->Opcode == ParseTree::Statement::Catch)
                            {
                                *ExitsCatch = true;

                                if (ReferenceParent->AsCatch()->ContainingTry &&
                                    Contains(ReferenceParent->AsCatch()->ContainingTry, LabelDefinition))
                                {
                                    *ExitsCatchToCorrespondingTry = true;
                                }
                            }
                            else if (ReferenceParent->Opcode == ParseTree::Statement::LambdaBody)
                            {
                                ReportSemanticError(
                                    ERRID_BranchOutOfMultilineLambda,
                                    LabelName.TextSpan);
                            }
                            // 
                            else if (ReferenceParent->Opcode == ParseTree::Statement::ForEachIn)
                            {
                                *ExitsTry = true;
                            }
                            else
                            {
                                *ExitsTry = true;
                            }
                        }
                    }
                }
            }

            return Candidate;
        }
    }

    if (!IsDefinition)
    {
        ReportSemanticError(
            ERRID_LabelNotDefined1,
            LabelName.TextSpan,
            LabelName.Name);

        return NULL;
    }

    LabelDeclarationList *Result = new(m_TreeAllocator) LabelDeclarationList;
    Result->Name = LabelName.Name;
    Result->Next = LabelBucket;
    m_LabelHashBuckets[LabelBucketIndex] = Result;

    return Result;
}

void
Semantics::InterpretOnErrorStatement
(
    _In_ ParseTree::OnErrorStatement *OnError
)
{

    ILTree::ResumableKind resumableKind = GetLocalResumableKind();
    if (resumableKind != ILTree::UnknownResumable && resumableKind != ILTree::NotResumable)
    {
        ReportSemanticError(ERRID_ResumablesCannotContainOnError, OnError->TextSpan);
        MarkContainingLambdaOrMethodBodyBad();
        // I don't know why the other paths don't also mark it as bad.
        // But I'm certain that in this case we want to prevent resumable codegen from happening, since it'll crash.
        // Belt and braces.
    }
    else if (m_StatementLambdaInterpreter)
    {
        ReportSemanticError(
            ERRID_MultilineLambdasCannotContainOnError,
            OnError->TextSpan);
    }
    else if (m_ProcedureContainsTry)
    {
        ReportSemanticError(
            ERRID_TryAndOnErrorDoNotMix,
            OnError->TextSpan);
    }

    // You can't have an On Error statement in a SyncLock or Using
    for (ParseTree::BlockStatement *OnErrorParent = OnError->GetParent();
         OnErrorParent;
         OnErrorParent = OnErrorParent->GetParent())
    {
        if (OnErrorParent->Opcode == ParseTree::Statement::SyncLock ||
            OnErrorParent->Opcode == ParseTree::Statement::Using)
        {

            ReportSemanticError(
                OnErrorParent->Opcode == ParseTree::Statement::SyncLock ?
                    ERRID_OnErrorInSyncLock :
                    ERRID_OnErrorInUsing,
                OnError->TextSpan);

            // If the innermost construct contains the reference, outer ones
            // do also, so further checking is unnecessary. If there was an
            // error, one diagnostic is enough.
            break;
        }
    }

    ILTree::OnErrorStatement *Result = &AllocateStatement(SL_ON_ERR, OnError)->AsOnErrorStatement();
    Result->OnErrorIndex = OnError->OnErrorIndex;

    if (OnError->GotoType == ParseTree::OnErrorStatement::GotoLabel &&
        !OnError->Label.IsBad)
    {
        bool OnErrorExitsTry = false;
        bool OnErrorExitsCatch = false;
        bool OnErrorExitsCatchToCorrespondingTry = false;
        Result->Label = ReferToLabel(OnError, &OnErrorExitsTry, &OnErrorExitsCatch, &OnErrorExitsCatchToCorrespondingTry);
    }
    else if (OnError->GotoType == ParseTree::OnErrorStatement::Next)
    {
        SetFlag32(Result, SLF_ONERR_NEXT);
    }
    else if (OnError->GotoType == ParseTree::OnErrorStatement::Zero)
    {
        SetFlag32(Result, SLF_ONERR_ZERO);
    }
    else if (OnError->GotoType == ParseTree::OnErrorStatement::MinusOne)
    {
        SetFlag32(Result, SLF_ONERR_MINUS1);
    }
}

void
Semantics::InterpretResumeStatement
(
    _Inout_ ParseTree::ResumeStatement *Resume
)
{
    ILTree::ResumableKind resumableKind = GetLocalResumableKind();
    if (resumableKind != ILTree::UnknownResumable && resumableKind != ILTree::NotResumable)
    {
        ReportSemanticError(ERRID_ResumablesCannotContainOnError, Resume->TextSpan);
        MarkContainingLambdaOrMethodBodyBad();
        // I don't know why the other paths don't also mark it as bad.
        // But I'm certain that in this case we want to prevent resumable codegen from happening, since it'll crash.
        // Belt and braces.
    }
    else if (m_StatementLambdaInterpreter)
    {
        ReportSemanticError(
            ERRID_MultilineLambdasCannotContainOnError,
            Resume->TextSpan);
    }
    else if (m_ProcedureContainsTry)
    {
        ReportSemanticError(
            ERRID_TryAndOnErrorDoNotMix,
            Resume->TextSpan);
    }

    ILTree::OnErrorStatement *Result = &AllocateStatement(SL_RESUME, Resume)->AsOnErrorStatement();

    if (Resume->ResumeType == ParseTree::ResumeStatement::ResumeLabel &&
        !Resume->Label.IsBad)
    {
        bool ResumeExitsTry = false;
        bool ResumeExitsCatch = false;
        bool ResumeExitsCatchToCorrespondingTry = false;
        Result->Label = ReferToLabel(Resume, &ResumeExitsTry, &ResumeExitsCatch, &ResumeExitsCatchToCorrespondingTry);

        if (ResumeExitsTry)
        {
            SetFlag32(Result, SLF_EXITS_TRY);
        }

        if (ResumeExitsCatch)
        {
            SetFlag32(Result, SLF_EXITS_CATCH);

            if (ResumeExitsCatchToCorrespondingTry)
            {
                SetFlag32(Result, SLF_EXITS_CATCH_TO_CORRESPONDING_TRY);
            }
        }
    }
    else
    {
        if (Resume->ResumeType == ParseTree::ResumeStatement::Next)
        {
            SetFlag32(Result, SLF_ONERR_NEXT);
        }
    }
}

void
Semantics::InterpretElseIfStatement
(
    _In_ ParseTree::ExpressionBlockStatement *If
)
{
    ILTree::IfBlock *Result = &AllocateBlock(SB_ELSE_IF, If)->AsIfBlock();
    Result->Conditional = InterpretConditionalOperand(If->Operand);
}

void
Semantics::InterpretIfStatement
(
    _In_ ParseTree::ExpressionBlockStatement *If
)
{
    // In the bound trees, If statements have an IF block that encloses
    // If, ElseIf, and Else blocks.
    AllocateBlock(SB_IF_BLOCK, If);

    // A LineIf cannot have an End If construct.  A BlockIf must have an End If construct.
    // Set flags on the block context to indicate absense of End If construct.
    if (If->Opcode == ParseTree::Statement::LineIf)
    {
        SetFlag32(m_BlockContext, SBF_LINE_IF);  // End If construct is absent.
    }

    ILTree::IfBlock *Result = &AllocateBlock(SB_IF, If)->AsIfBlock();
    Result->Conditional = InterpretConditionalOperand(If->Operand);
}

void
Semantics::InterpretLoopStatement
(
    _In_ ParseTree::ExpressionBlockStatement *Loop
)
{
    ILTree::LoopBlock *Result = &AllocateBlock(SB_LOOP, Loop)->AsLoopBlock();

    if (Loop->Operand)
    {
        ILTree::Expression *Conditional = InterpretConditionalOperand(Loop->Operand);

        if (!IsBad(Conditional) &&
            (Loop->Opcode == ParseTree::Statement::DoUntilTopTest ||
             Loop->Opcode == ParseTree::Statement::DoUntilBottomTest))
        {
            Conditional = NegateBooleanExpression(Conditional);
        }

        Result->Conditional = Conditional;

        if (Loop->Opcode == ParseTree::Statement::DoWhileTopTest ||
            Loop->Opcode == ParseTree::Statement::DoUntilTopTest ||
            Loop->Opcode == ParseTree::Statement::While)
        {
            SetFlag32(Result, SBF_DO_WHILE);
        }
        else
        {
            SetFlag32(Result, SBF_LOOP_WHILE);
        }

        // Mark While loops as distinct from Do While.
        if (Loop->Opcode == ParseTree::Statement::While)
        {
            SetFlag32(Result, SBF_WHILE);
        }
    }
    else
    {
        SetFlag32(Result, SBF_DO_LOOP);
    }
}

void
Semantics::InterpretSelectStatement
(
    _In_ ParseTree::ExpressionBlockStatement *Select
)
{
    ILTree::SelectBlock *Result = &AllocateBlock(SB_SELECT, Select)->AsSelectBlock();

    ILTree::Expression *Selector =
        InterpretStatementOperand(
            Select->Operand,
            ExprForceRValue);

    VSASSERT(Selector->ResultType, "expected type");

    if (IsBad(Selector) || Selector->ResultType == NULL)
    {
        MakeBad(Result);
        return;
    }

    ILTree::ResumableKind resumableKind = GetLocalResumableKind();
    if (resumableKind != ILTree::UnknownResumable && resumableKind != ILTree::NotResumable)
    {
        if (IsRestrictedType(Selector->ResultType, GetCompilerHost()) ||
            IsRestrictedArrayType(Selector->ResultType, GetCompilerHost()))
        {
            // Select Case CType(Nothing, ArgIterator)
            // Normally this is just weird, but in the case of resumable methods its weird and fails PEVerify.
            // (that's because the selector temporary gets lifted into a field, and it's illegal to have fields
            // of restricted type).
            ReportSemanticError(ERRID_CannotLiftRestrictedTypeResumable1, Select->Operand->TextSpan, Selector->ResultType);
        }
    }


    if (Selector->bilop == SX_ADDRESSOF || Selector->bilop == SX_LAMBDA || Selector->bilop == SX_UNBOUND_LAMBDA)
    {
        ERRID errorId = Selector->bilop == SX_ADDRESSOF ? ERRID_AddressOfInSelectCaseExpr : ERRID_LambdaInSelectCaseExpr;

        ReportSemanticError(
            errorId,
            Selector->Loc);

        MakeBad(Result);
        return;
    }

    if (TypeHelpers::IsIntegralType(Selector->ResultType) ||
        TypeHelpers::IsBooleanType(Selector->ResultType) ||  // Boolean works here because Boolean const values use the same tree as integrals.
        TypeHelpers::IsCharType(Selector->ResultType))       // Char works here because Char const values use the same tree as integrals.
    {
        // Optimistically assume all associated Case statements are simple constant integers
        // (if they aren't they would be conditionals).  We will set this flag to false when
        // a Case statement requiring a conditional is encountered while interpreting Case statements.
        //
        SetFlag32(Result, SBF_SELECT_TABLE);
    }

    Result->SelectorCapture = CaptureInLongLivedTemporary(Selector, Result->SelectorTemporary, Result);
}

void
Semantics::InterpretCaseStatement
(
    _In_ ParseTree::CaseStatement *Case
)
{
    ILTree::SelectBlock &EnclosingSelect = m_BlockContext->AsSelectBlock();

    // Get the type of the selector expression (from the Select statement.)
    Type *TargetType =
        IsBad(EnclosingSelect) ?
            NULL :
            GetDataType(EnclosingSelect.SelectorTemporary);

    bool ConvertCaseElements = TargetType && (TargetType->IsIntrinsicType() || TargetType->IsEnum());

    // Iterate over the unbound cases, constructing a list of bound cases.

    ILTree::CaseBlock *Result = &AllocateBlock(SB_CASE, Case)->AsCaseBlock();
    ILTree::CASELIST **BoundCaseListTarget = &Result->BoundCaseList;

    for (ParseTree::CaseList *UnboundCaseList = Case->Cases;
         UnboundCaseList;
         UnboundCaseList = UnboundCaseList->Next)
    {
        ParseTree::Case *CaseElement = UnboundCaseList->Element;

#pragma warning(disable:6014)//,"We are not leaking memory; PREFast is just confused.")
        ILTree::CASELIST *BoundCaseElement = new(m_TreeAllocator) ILTree::CASELIST;
#pragma warning(default:6014)//,"We were not leaking memory; PREFast was just confused.")

        // There are three forms:
        //
        //   IS RelationalOperator Expression
        //   Expression TO Expression
        //   Expression

        // There is no requirement that case expressions be constant.
        // Incredibly, it is apparently valid for duplicate case values to
        // occur, as well as for empty ranges, e.g. "3 to 3" or "4 to 3".

        if (CaseElement->Opcode == ParseTree::Case::Relational)
        {
            if (CaseElement->AsRelational()->RelationalOpcode == ParseTree::Expression::SyntaxError)
            {
                continue;
            }

            // Interpret the operand to the relational operator,
            // and collect the relational operator.

            BoundCaseElement->LowBound =
                InterpretStatementOperand(
                    CaseElement->AsRelational()->Value,
                    ExprForceRValue);

            BoundCaseElement->RelationalOpcode =
                MapOperator(CaseElement->AsRelational()->RelationalOpcode);
        }
        else if (CaseElement->Opcode == ParseTree::Case::Range)
        {
            BoundCaseElement->LowBound =
                InterpretStatementOperand(
                    CaseElement->AsRange()->Low,
                    ExprForceRValue);

            BoundCaseElement->HighBound =
                InterpretStatementOperand(
                    CaseElement->AsRange()->High,
                    ExprForceRValue);

            BoundCaseElement->IsRange = true;
        }
        else if (CaseElement->Opcode == ParseTree::Case::Value)
        {
            // The case value is a single expression.

            BoundCaseElement->LowBound =
                InterpretStatementOperand(
                    CaseElement->AsValue()->Value,
                    ExprForceRValue);

            BoundCaseElement->RelationalOpcode = SX_EQ;
        }
        else
        {
            VSASSERT(
                CaseElement->Opcode == ParseTree::Case::SyntaxError,
                "Unexpected sort of Case");

            continue;
        }

        if (ConvertCaseElements)
        {
            if (!IsBad(BoundCaseElement->LowBound) &&
                (BoundCaseElement->LowBound->ResultType->IsIntrinsicType() ||
                    BoundCaseElement->LowBound->ResultType->IsEnum()))
            {
                BoundCaseElement->LowBound =
                    ConvertWithErrorChecking(
                        BoundCaseElement->LowBound,
                        TargetType,
                        ExprForceRValue);
            }

            if (BoundCaseElement->IsRange &&
                !IsBad(BoundCaseElement->HighBound) &&
                (BoundCaseElement->HighBound->ResultType->IsIntrinsicType() ||
                    BoundCaseElement->HighBound->ResultType->IsEnum()))
            {
                BoundCaseElement->HighBound =
                    ConvertWithErrorChecking(
                        BoundCaseElement->HighBound,
                        TargetType,
                        ExprForceRValue);
            }
        }

        if (BoundCaseElement->LowBound->bilop != SX_CNS_INT ||
            (BoundCaseElement->IsRange && BoundCaseElement->HighBound->bilop != SX_CNS_INT) ||
            (!BoundCaseElement->IsRange && BoundCaseElement->RelationalOpcode != SX_EQ))
        {
            ClearFlag32(EnclosingSelect, SBF_SELECT_TABLE);
        }

        *BoundCaseListTarget = BoundCaseElement;
        BoundCaseListTarget = &BoundCaseElement->Next;
    }
}

bool
Semantics::RecommendSwitchTable
(
    _Out_ ILTree::SelectBlock *Select
)
{
    if (!HasFlag32(Select, SBF_SELECT_TABLE))
    {
        return false;
    }

    bool IsUnsigned = TypeHelpers::IsUnsignedType(GetDataType(Select->SelectorTemporary));

    Quadword Minimum = IsUnsigned ? _UI64_MAX : _I64_MAX;
    Quadword Maximum = IsUnsigned ? 0 : _I64_MIN;

    unsigned IfBlockCount = 0;      // number of IF blocks - used to size IF list
    unsigned IfRangeBlockCount = 0; // number of IF range blocks - see above

    bool FoundCaseClause = false;
    ILTree::Statement *CurrentStatement = Select->Child;

    while (CurrentStatement &&
           CurrentStatement->bilop == SB_CASE &&
           !HasFlag32(CurrentStatement, SBF_CASE_ISELSE))
    {
        ILTree::CaseBlock *CurrentCase = &CurrentStatement->AsCaseBlock();
        ILTree::Expression *Conditional = NULL;
        ILTree::CASELIST *CurrentCaseClause = CurrentCase->BoundCaseList;

        while (CurrentCaseClause)
        {
            FoundCaseClause = true;

            // find new minimums and maximums
            Quadword LowBound = CurrentCaseClause->LowBound->AsIntegralConstantExpression().Value;

            // expand min and max values if necessary.  must check for max
            // since this may not be a range
            if ((IsUnsigned && (unsigned __int64)LowBound < (unsigned __int64)Minimum) ||
                (!IsUnsigned && LowBound < Minimum))
            {
                Minimum = LowBound;
            }

            if ((IsUnsigned && (unsigned __int64)LowBound > (unsigned __int64)Maximum) ||
                (!IsUnsigned && LowBound > Maximum))
            {
                Maximum = LowBound;
            }

            if (CurrentCaseClause->IsRange)
            {
                Quadword HighBound = CurrentCaseClause->HighBound->AsIntegralConstantExpression().Value;

                // if first number of range is larger than second, then cannot use
                // SWITCH table because this range must instead fall to an IF list
                // to maintain correctness with previous versions of VB
                //
                if ((IsUnsigned && (unsigned __int64)LowBound > (unsigned __int64)HighBound) ||
                    (!IsUnsigned && LowBound > HighBound))
                {
                    ReportSemanticError(
                        WRNID_SelectCaseInvalidRange,
                        CurrentStatement->Loc);

                    ClearFlag32(Select, SBF_SELECT_TABLE);
                    return false;
                }

                // Figure out max constant values.
                else if ((IsUnsigned && (unsigned __int64)HighBound > (unsigned __int64)Maximum) ||
                        (!IsUnsigned && HighBound > Maximum))
                {
                    Maximum = HighBound;
                }

                IfRangeBlockCount++;
            }
            else
            {
                IfBlockCount++;
            }

            CurrentCaseClause = CurrentCaseClause->Next;
        }

        CurrentStatement = CurrentStatement->Next;
    }

    if (!FoundCaseClause)
    {
        ClearFlag32(Select, SBF_SELECT_TABLE);
        return false;
    }

    // if we have reached this point, we can definately use a SWITCH table.
    // Now determine if it is worthwhile to use one over an IF list.
    // SWITCH tables are preferrable (to a point) because of their speed, so we
    // are willing to sacrifice some space for speed.  However, SWITCH tables
    // can get very large, depending on the number of entries, and can use more
    // space than we are willing to sacrifice.
    // Use a simple heuristic to choose which method to use.
    //
    // an IF list is made up of blocks, each block containing a
    // comparison and a branch off that comparison.
    // Some blocks are comparisons that check whole ranges (eg:  if 0 < n < 5 ).
    // These are called range blocks.
    // A SWITCH table has some normalization arithmetic (called the header) to
    // normalize the bottom value to 0, followed by the branch addresses that
    // make up the table
    //
    // A simple heuristic is to calculate the size of either method given the
    // number of cases and the maximum (normalized) CASE value.  If the size
    // of the SWITCH table is more than twice the size of the IF list,
    // then use the IF list.  i.e.,
    //
    // a = IF block size
    // b = IF range block size
    // c = SWITCH header size
    // d = SWITCH table element size
    // e = normalized maximum Case value
    // n = number of IF blocks
    // m = number of IF range blocks
    //
    // if (c+d*e) > (2*(a*n+b*m)) then use IF list
    //

    // 


    unsigned const IF_BLOCK_SIZE = 15;   // ld+param, ld+param, br+addr
    unsigned const IF_RANGE_BLOCK_SIZE = 30;   // 2 * if block size
    unsigned const SWITCH_HEADER_SIZE = 29;   // ld+param, ld+param, sub, switch+ui4
    unsigned const SWITCH_ELEMENT_SIZE = 4;    // addr

    VSASSERT((IsUnsigned && (unsigned __int64)Maximum >= (unsigned __int64)Minimum) ||
             (!IsUnsigned && Maximum >= Minimum),
             "PreprocessSelect: Max and Min invalid");

    // num elements in SWITCH table
    unsigned __int64 CountOfEntries = Maximum - Minimum + 1;

    // overflow - need to guard against Max == Int64.MaxValue and Min == Int64.MinValue.
    if (CountOfEntries == 0)
    {
        ClearFlag32(Select, SBF_SELECT_TABLE);
        return false;
    }

    // if size of switch table is over twice as large as if list, or the switch table
    // would be too large to index into, use an if list
    if ( (SWITCH_HEADER_SIZE + SWITCH_ELEMENT_SIZE * CountOfEntries >
         2 * (IF_BLOCK_SIZE * IfBlockCount + IF_RANGE_BLOCK_SIZE * IfRangeBlockCount)) ||
         (CountOfEntries > INT_MAX))
    {
        ClearFlag32(Select, SBF_SELECT_TABLE);
        return false;
    }

    Select->Minimum = Minimum;
    Select->Maximum = Maximum;

    // if this point reached, we will use a SWITCH table for current Select stmt
    return true;
}

// Determine what kind of byte codes to generate for SELECT.
// There are two choices, use an IF list or a SWITCH table
// This function determines which method to use
// The conditions for choosing the SWITCH table are:
//   switch value must be integer
//   case values must be (or expand to, as in ranges) integer constants
//   no "Is <relop>" cases
//   must have at least one CASE
//   beneficial over IF lists (as per a threshold on size ratio)
//   ranges must have lower bound first

void
Semantics::OptimizeSelectStatement
(
    _Out_ ILTree::SelectBlock *Select
)
{
    if (IsBad(Select))
    {
        return;
    }

    if (RecommendSwitchTable(Select))
    {
        return;
    }

    ILTree::Expression *SelectorReference =
        ReferToSymbol(
            Select->Loc,
            Select->SelectorTemporary,
            chType_NONE,
            NULL,
            NULL, // 
            ExprNoFlags);

    ILTree::Statement *CurrentStatement = Select->Child;
    Location Loc;

    while (CurrentStatement &&
           CurrentStatement->bilop == SB_CASE &&
           !HasFlag32(CurrentStatement, SBF_CASE_ISELSE))
    {
        ILTree::CaseBlock *CurrentCase = &CurrentStatement->AsCaseBlock();
        ILTree::Expression *Conditional = NULL;
        ILTree::CASELIST *CurrentCaseClause = CurrentCase->BoundCaseList;

        while (CurrentCaseClause)
        {
            ILTree::Expression *BoundClause;

            BoundClause =
                InterpretBinaryOperation(
                    CurrentCaseClause->IsRange ?
                        SX_GE :
                        CurrentCaseClause->RelationalOpcode,
                    CurrentCaseClause->LowBound->Loc,
                    SelectorReference,
                    CurrentCaseClause->LowBound,
                    ExprForceRValue,
                    true /*fSelectGenerated*/);

            if (CurrentCaseClause->IsRange)
            {
                ILTree::Expression *HighBoundTest;

                HighBoundTest =
                    InterpretBinaryOperation(
                        SX_LE,
                        CurrentCaseClause->HighBound->Loc,
                        SelectorReference,
                        CurrentCaseClause->HighBound,
                        ExprForceRValue,
                        true /*fSelectGenerated*/);

                BoundClause =
                    InterpretBinaryOperation(
                        SX_ANDALSO,
                        GetSpan(Loc, CurrentCaseClause->LowBound->Loc, CurrentCaseClause->HighBound->Loc),
                        BoundClause,
                        HighBoundTest,
                        ExprForceRValue,
                        true /*fSelectGenerated*/);
            }

            if (Conditional == NULL)
            {
                Conditional = BoundClause;
            }
            else
            {
                Conditional =
                    InterpretBinaryOperation(
                        SX_ORELSE,
                        GetSpan(
                            Loc,
                            Conditional->Loc,
                            BoundClause->Loc),
                        Conditional,
                        BoundClause,
                        ExprForceRValue,
                        true /*fSelectGenerated*/);  // what goes here?
            }

            CurrentCaseClause = CurrentCaseClause->Next;
        }

        if (Conditional && !IsBad(Conditional))
        {
            Conditional =
                ConvertWithErrorChecking(
                    Conditional,
                    GetFXSymbolProvider()->GetBooleanType(),
                    ExprIsOperandOfConditionalBranch);

            if (!m_IsGeneratingXML)
            {
                SetFlag32(CurrentCase, SBF_CASE_CONDITION);
                CurrentCase->Conditional = Conditional;
            }
        }

        CurrentStatement = CurrentStatement->Next;
    }
}

void
Semantics::InterpretCaseElseStatement
(
    _In_ ParseTree::ExecutableBlockStatement *CaseElse
)
{
    SetFlag32(m_BlockContext, SBF_SELECT_HAS_CASE_ELSE);

    ILTree::Statement *Result = AllocateBlock(SB_CASE, CaseElse);
    SetFlag32(Result, SBF_CASE_ISELSE);
}

void Semantics::InterpretForStatement
(
    _In_ ParseTree::ForStatement *For
)
{
    ParseTree::VariableDeclarationStatement *ControlVariableDeclaration = For->ControlVariableDeclaration;

    //Dig into the variable declaration and check to see if
    // 1) it is missing an as clause, and
    // 2) it is marked as "nullable"
    // If so, generate an error

    Scope *ForLocals = NULL;
    if (ControlVariableDeclaration)
    {
        ParseTree::Declarator* Variable = ControlVariableDeclaration->Declarations->Element->Variables->Element;
        ParseTree::Type * Type = ControlVariableDeclaration->Declarations->Element->Type;

        if (!Type  && Variable->Name.IsNullable)
        {
            ReportSemanticError(ERRID_NullableTypeInferenceNotSupported, Variable->TextSpan);
        }


        if (Variable->ArrayInfo && Variable->ArrayInfo->Opcode == ParseTree::Type::ArrayWithSizes)
        {
            ReportSemanticError(
                ERRID_ForCtlVarArraySizesSpecified,
                Variable->TextSpan);
        }

        ForLocals =
            m_SymbolCreator.GetHashTable(
                NULL,
                m_Lookup,
                true,
                For->LocalsCount + ForLoopVariableCount,
                NULL);

        m_Lookup = ForLocals;

        Declared::MakeLocalsFromTrees(
                m_CompilerHost,
                &m_TreeStorage,
                m_CompilationCaches,
                m_Errors,
                ContainingClass()->PClass(),
                m_SourceFile,
                m_Procedure,
                m_Lookup,
                ControlVariableDeclaration,
                false,  // not being called from ParseTreeService
                PerformObsoleteChecks() ? TypeResolvePerformObsoleteChecks : TypeResolveNoFlags,
                NULL,
                m_StatementLambdaInterpreter!=NULL ? m_StatementLambdaInterpreter->m_Tree : NULL);

    }

    // Backup this value for both the loop statements, and interpret the control variable
    // here, rather then inside the for analysis.

    BackupValue< bool > backupExplicitLoopVariableCreated( &m_ExplicitLoopVariableCreated );
    BackupValue< bool > backupCreateImplicitDeclarations( &m_CreateImplicitDeclarations );
    BackupValue< unsigned > backupCreateExplicitScopeForLoop( &m_CreateExplicitScopeForLoop );

    ExpressionFlags Flags = ExprIsAssignmentTarget | ExprPropagatePropertyReference;
    m_ExplicitLoopVariableCreated = false;

    if( OptionInferOn() )
    {
        m_CreateImplicitDeclarations = m_CreateImplicitDeclarations || For->ControlVariable->Opcode == ParseTree::Expression::Name;
        m_CreateExplicitScopeForLoop = For->LocalsCount + ForLoopVariableCount;
        Flags |= ExprInferLoopControlVariableExplicit;
    }

    ILTree::Expression *ControlVariableReference =
        InterpretStatementOperand(
            For->ControlVariable,
            Flags
            );

    backupCreateImplicitDeclarations.Restore();
    backupCreateExplicitScopeForLoop.Restore();

    // Don't restore ExplicitLoopVariableCreated - we need this in the analysis methods.

    if( m_ExplicitLoopVariableCreated )
    {
        AssertIfFalse( ( ForLocals == NULL ) || ( ForLocals == m_Lookup ) );
        ForLocals = m_Lookup;
    }

    switch (For->Opcode)
    {
        case ParseTree::Statement::ForFromTo:
        {
            InterpretForFromToStatement(For->AsForFromTo(), ControlVariableReference, ForLocals);
            InterpretAndPopBlock(For->AsExecutableBlock());
            break;
        }

        case ParseTree::Statement::ForEachIn:
        {
            InterpretForEachStatement(For->AsForEachIn(), ControlVariableReference, ForLocals);
            break;
        }

        default:
            VSASSERT(false, "Unexpected OpCode!");
    }

    // in some error cases...the for Block is not established nor popped. So the hash,
    // the current scope i.e. m_Lookup is still the one for the for loop. So we need to
    // explicitly pop the scope.
    if ((ControlVariableDeclaration || m_ExplicitLoopVariableCreated) && (m_Lookup == ForLocals))
    {
        m_Lookup = m_Lookup->GetImmediateParent()->PHash();
    }
}

ILTree::Expression *
Semantics::InterpretForLoopOperator
(
    BILOP Opcode,
    const Location &ExpressionLocation,
    Type *ControlVariableType,
    _In_ ILTree::Expression *Left,
    _In_ ILTree::Expression *Right
)
{
    VSASSERT(Opcode == SX_ADD || Opcode == SX_SUB || Opcode == SX_GE || Opcode == SX_LE,
             "unexpected For loop operator");

    ILTree::Expression *Result = NULL;
    bool ResolutionFailed = false;
    bool ResolutionIsLateBound = false;
    Procedure *OperatorMethod = NULL;
    GenericBinding *OperatorMethodGenericContext = NULL;

    bool PreviouslyReportingErrors = m_ReportErrors;
    m_ReportErrors = false;

    Type *ResultType =
        ResolveUserDefinedOperator(
            Opcode,
            ExpressionLocation,
            Left,
            Right,
            ResolutionFailed,
            ResolutionIsLateBound,
            OperatorMethod,
            OperatorMethodGenericContext);

    m_ReportErrors = PreviouslyReportingErrors;

    if (ResolutionFailed || ResolutionIsLateBound)
    {
        ReportSemanticError(
            ERRID_ForLoopOperatorRequired2,
            ExpressionLocation,
            ControlVariableType,
            m_Compiler->OperatorToString(MapToUserDefinedOperator(Opcode)));

        return AllocateBadExpression(ExpressionLocation);
    }

    if (Opcode == SX_GE || Opcode == SX_LE)
    {
        if (!TypeHelpers::EquivalentTypes(
                TypeInGenericContext(OperatorMethod->GetFirstParam()->GetType(), OperatorMethodGenericContext),
                TypeInGenericContext(OperatorMethod->GetFirstParam()->GetNext()->GetType(), OperatorMethodGenericContext)) ||
            !TypeHelpers::EquivalentTypes(
                ControlVariableType,
                TypeInGenericContext(OperatorMethod->GetFirstParam()->GetType(), OperatorMethodGenericContext)))
        {
            ReportSemanticError(
                ERRID_UnacceptableForLoopRelOperator2,
                ExpressionLocation,
                OperatorMethod->GetAssociatedOperatorDef(),
                ControlVariableType);

            return AllocateBadExpression(ExpressionLocation);
        }

        Result =
            InterpretUserDefinedOperator(
                Opcode,
                OperatorMethod,
                OperatorMethodGenericContext,
                ExpressionLocation,
                Left,
                Right,
                ExprForceRValue);
    }
    else
    {
        if (!TypeHelpers::EquivalentTypes(
                ResultType,
                TypeInGenericContext(OperatorMethod->GetFirstParam()->GetType(), OperatorMethodGenericContext)) ||
            !TypeHelpers::EquivalentTypes(
                ResultType,
                TypeInGenericContext(OperatorMethod->GetFirstParam()->GetNext()->GetType(), OperatorMethodGenericContext)) ||
            !TypeHelpers::EquivalentTypes(ResultType, ControlVariableType))
        {
            ReportSemanticError(
                ERRID_UnacceptableForLoopOperator2,
                ExpressionLocation,
                OperatorMethod->GetAssociatedOperatorDef(),
                ControlVariableType->IsGenericBinding() ?
                    ControlVariableType->PNamedRoot() :
                    ControlVariableType);

            return AllocateBadExpression(ExpressionLocation);
        }

        Result =
            InterpretUserDefinedOperator(
                Opcode,
                OperatorMethod,
                OperatorMethodGenericContext,
                ExpressionLocation,
                Left,
                Right,
                ExprForceRValue);
    }

    return Result;
}

void
Semantics::GenerateWarningIfNoExplicitFieldReference
(
    _In_ ParseTree::ForStatement *For,
    ILTree::Expression* ControlVariableReference
)
{
    AssertIfNull( For );
    AssertIfNull( ControlVariableReference );
    AssertIfFalse( OptionInferOn() );


    if( ControlVariableReference->bilop == SX_SYM )
    {
        BCSYM_NamedRoot* sym = NULL;
        sym = ControlVariableReference->AsSymbolReferenceExpression().Symbol;

        if( !sym->IsStaticLocalBackingField() &&
            !sym->IsLocal() &&
            !sym->PVariable()->IsLambdaMember() &&
            For->ControlVariable->Opcode == ParseTree::Expression::Name )
        {
            ReportSemanticError(
                WRNID_FieldInForNotExplicit,
                For->ControlVariable->TextSpan,
                For->ControlVariable);
        }
    }

}

bool
Semantics::TryInferForFromToType
(
    _In_ ParseTree::ForFromToStatement *For,
    _In_ ILTree::Expression*& ControlVariableReference,
    _Out_ ILTree::Expression*& InitialValue,
    _Out_ ILTree::Expression*& Limit,
    _Inout_ ILTree::Expression*& Step
)
{
    AssertIfNull( For );
    AssertIfNull( ControlVariableReference );
    AssertIfTrue( IsBad( ControlVariableReference ) );
    AssertIfFalse( OptionInferOn() );

    ParseTree::ExpressionInitializer ControlVariableInitializer;

    Type* ResultToUse;
    DynamicArray< BCSYM* > TypeResults;
    NorlsAllocWrapper AllocWrapper(&m_TreeStorage);
    DominantTypeDataList TypeDataList(AllocWrapper);
    TypeInferenceCollection bestType( this, &m_TreeStorage, &AllocWrapper );

    // Determine circular references.

    InitializerInferInfo inferInfo( &m_InitializerInferStack );

    if( ShouldInfer( ControlVariableReference ) && !IsBad( ControlVariableReference ) )
    {
        inferInfo.Variable = ControlVariableReference->AsSymbolReferenceExpression().Symbol->PVariable();
    }
    
    ControlVariableInitializer.Opcode = ParseTree::Initializer::Expression;
    ControlVariableInitializer.Value = For->InitialValue;
    InitialValue = InterpretStatementOperand(
                    For->InitialValue,
                    ExprForceRValue);
    
    // Dev10 #623867 - Bail if the InitialValue is bad rather than let junk run through the dominant type below
    if ( InitialValue == NULL || IsBad( InitialValue ))
    {
        return false;
    }

    // Start with the type for the initial value, and derive the best type from
    // the limit and possibly the step.

    Limit = InterpretStatementOperand(
            For->FinalValue,
            ExprForceRValue);

    if( Limit == NULL || IsBad( Limit ) )
    {
        return( false );
    }

    if( For->IncrementValue != NULL )
    {
        Step = InterpretStatementOperand(
                For->IncrementValue,
                ExprForceRValue);

        if( IsBad( Step ) )
        {
            return( false );
        }
    }

    if( inferInfo.CircularReferenceDetected )
    {
        // Error was reported in name lookup; don't write error here.
        MakeBad( ControlVariableReference );
        return( false );
    }

    inferInfo.Restore();

    bestType.AddType( InitialValue->ResultType, ConversionRequired::Any, InitialValue );
    bestType.AddType( Limit->ResultType, ConversionRequired::Any, Limit );
    if( Step != NULL ) bestType.AddType( Step->ResultType, ConversionRequired::Any, Step );

    InferenceErrorReasons errorReasons = InferenceErrorReasonsOther;

    bestType.FindDominantType( TypeDataList, &errorReasons);

    if( TypeDataList.Count() == 0 || (TypeDataList.Count() > 1 && !(errorReasons & InferenceErrorReasonsAmbiguous) ))
    {
        ReportSemanticError(
                ERRID_NoSuitableWidestType1,
                ControlVariableReference->Loc,
                ControlVariableReference->AsSymbolReferenceExpression().Symbol);

        MakeBad( ControlVariableReference );
        return( false );
    }
    else if( TypeDataList.Count() > 1 )
    {
        ReportSemanticError(
                ERRID_AmbiguousWidestType3,
                ControlVariableReference->Loc,
                ControlVariableReference->AsSymbolReferenceExpression().Symbol,
                TypeDataList[0]->ResultType,
                TypeDataList[1]->ResultType);

        MakeBad( ControlVariableReference );
        return( false );
    }

    ResultToUse = TypeDataList[0]->ResultType;

    // Infer the variable type, and convert the other expressions.

    InferVariableType(ControlVariableReference, ResultToUse);

    if( !BCSYM::AreTypesEqual( InitialValue->ResultType, ResultToUse ) )
    {
        InitialValue = ConvertWithErrorChecking(
                InitialValue,
                ResultToUse,
                ExprForceRValue
                );
    }

    if( !BCSYM::AreTypesEqual( Limit->ResultType, ResultToUse ) )
    {
        Limit = ConvertWithErrorChecking(
                Limit,
                ResultToUse,
                ExprForceRValue
                );
    }

    if( Step != NULL && !BCSYM::AreTypesEqual( Step->ResultType, ResultToUse ) )
    {
        Step = ConvertWithErrorChecking(
                Step,
                ResultToUse,
                ExprForceRValue
                );
    }

    return( true );
}

bool
Semantics::TypeMaySupportFor
(
    Type * pType
)
{
    return
        (
            TypeHelpers::IsNullableType(pType, m_CompilerHost) &&
            TypeMaySupportFor(TypeHelpers::GetElementTypeOfNullable(pType, m_CompilerHost))
        ) ||
        (
            !TypeHelpers::IsNullableType(pType, m_CompilerHost) &&
            (
                TypeHelpers::IsIntegralType(pType) ||
                TypeHelpers::IsFloatingType(pType) ||
                TypeHelpers::IsDecimalType(pType) ||
                pType->IsObject() ||
                (
                    HasUserDefinedOperators(pType) &&
                    !TypeHelpers::IsIntrinsicOrEnumType(pType)
                )
            )
        );
}

/*=======================================================================================
InterpretForFromToStatement

    Dim i As T
    Dim a As U
    Dim b As V
    Dim c As W

    For i = a To b [Step c]
        ...
    Next i

The For loop control variable (i) determines the overall type of the For loop, T.  If T
is an intrinsic type, then the normal rules apply.  If T is a user-defined type with
overloaded operators, then the For statement is interpreted as the equivalent of:

    'LOOP INITIALIZE
    i = a
    Dim _bound As T = b
    Dim _step As T = c
    Dim _ispositive As Boolean

    'POSITIVE 

































*/


void
Semantics::InterpretForFromToStatement
(
    _In_ ParseTree::ForFromToStatement *For,
    ILTree::Expression* ControlVariableReference,
    Scope *ForLocals
)
{
    AssertIfNull( ControlVariableReference );

    Type *TargetType = NULL;
    ILTree::Expression *InitialValue = NULL;
    ILTree::Expression *Limit = NULL;
    ILTree::Expression *Step = NULL;

    if (!IsBad(ControlVariableReference))
    {
        bool bTypeOk = true;

        if( m_ExplicitLoopVariableCreated && ShouldInfer( ControlVariableReference ) )
        {
            bTypeOk = TryInferForFromToType(
                    For,
                    ControlVariableReference,
                    InitialValue,
                    Limit,
                    Step
                    );
        }

        if( !m_ExplicitLoopVariableCreated && OptionInferOn() )
        {
            GenerateWarningIfNoExplicitFieldReference( For, ControlVariableReference );
        }

        // Verify that the control variable reference is an lvalue and does
        // not refer to the control variable of an enclosing loop. Also
        // verify that the reference in the Next is to the same symbol.

        if (bTypeOk && VerifyForLoopControlReference(ControlVariableReference, For))
        {
            // Verify that the type of the control variable is OK.

            Type *ControlVariableType = ControlVariableReference->ResultType;

            if (TypeMaySupportFor(ControlVariableType))
            {
                TargetType = ControlVariableType;
            }
            else
            {
                ReportSemanticError(
                    ERRID_ForLoopType1,
                    For->ControlVariable->TextSpan,
                    ControlVariableType);
            }
        }
    }

    ILTree::ForBlock *Result = &AllocateBlock(SB_FOR, For)->AsForBlock();
    Result->Locals = ForLocals;
    Result->TerminatingResumeIndex = For->ResumeIndexForEndNext;

    // LOOP INITIALIZE

    if (!InitialValue)
    {
        InitialValue =
            InterpretStatementOperand(
                For->InitialValue,
                ExprForceRValue,
                TargetType,
                NULL);
    }

    if( Limit == NULL )
    {
        Limit =
            InterpretStatementOperand(
                For->FinalValue,
                ExprForceRValue,
                TargetType,
                NULL);
    }

    if (For->IncrementValue)
    {
        if( Step == NULL )
        {
            Step =
                InterpretStatementOperand(
                    For->IncrementValue,
                    ExprForceRValue,
                    TargetType,
                    NULL);
        }
    }
    else if (TargetType)
    {
        // There is no explicit step value. Synthesize a constant with a
        // value of 1 and a type matching the control variable's.

        // 


        Step =
            ConvertWithErrorChecking(
                ProduceConstantExpression(
                    1,
                    For->ControlVariable->TextSpan,
                    GetFXSymbolProvider()->GetIntegerType()
                    IDE_ARG(0)),
                TargetType,
                ExprHasExplicitCastSemantics);
    }

    if (TargetType == NULL ||
        IsBad(InitialValue) ||
        IsBad(Limit) ||
        IsBad(Step))
    {
        MakeBad(Result);
        return;
    }

    Type * NullableElementType = TypeHelpers::IsNullableType(TargetType, m_CompilerHost) ? TypeHelpers::GetElementTypeOfNullable(TargetType, m_CompilerHost) : NULL;

    bool shouldDoUserDefinedOverloadResolution =
        (
            NullableElementType &&
            !TypeHelpers::IsIntrinsicOrEnumType(NullableElementType) &&
            HasUserDefinedOperators(NullableElementType)
        ) ||
        (
            !NullableElementType &&
            !TypeHelpers::IsIntrinsicOrEnumType(TargetType) &&
            HasUserDefinedOperators(TargetType)
        );


    // The values used in multiple iterations of the loop need to be captured.
    // The code generator does this, using temporaries created here.

    if
    (
        !IsConstant(Limit) ||
        (
            TypeHelpers::IsDecimalType(TargetType) &&
            !IsDecimalZeroOrOneValue(Limit->AsDecimalConstantExpression())
        ) ||
        shouldDoUserDefinedOverloadResolution
    )  // Force temps for better perf (VSW#32597).
    {
        Result->LimitTemporary = AllocateLongLivedTemporary(TargetType, &Limit->Loc, Result);
    }

    if
    (
        !IsConstant(Step) ||
        (
            TypeHelpers::IsDecimalType(TargetType) &&
            !IsDecimalZeroOrOneValue(Step->AsDecimalConstantExpression())
        ) ||
        shouldDoUserDefinedOverloadResolution
    )  // Force temps for better perf (VSW#32597).
    {
        Result->StepTemporary = AllocateLongLivedTemporary(TargetType, &Step->Loc, Result);
    }

    if (TargetType->IsObject())
    {
        Result->EnumeratorTemporary = AllocateLongLivedTemporary(
            GetFXSymbolProvider()->GetObjectType(),
            &For->TextSpan,
            Result);
    }

    // 


    if
    (
        shouldDoUserDefinedOverloadResolution
    )
    {
        VSASSERT(Result->LimitTemporary && Result->StepTemporary, "must have limit and step temps at this point");

        const Location &HiddenLocation = Location::GetHiddenLocation();
        const Location &TerminatingConstructLocation =
            For->TerminatingConstruct ?
                For->TerminatingConstruct->TextSpan :
                For->TextSpan;

        ILTree::Expression *StepReference =
            ReferToSymbol(
                Step->Loc,
                Result->StepTemporary,
                chType_NONE,
                NULL,
                NULL,
                ExprNoFlags);

        // INCREMENT
        // Don't use AllocateStatement here to link the statemetn because we do not want this node linked into the context's list.
        ILTree::Statement *IncrementStatement = &AllocateStatement(SL_STMT, HiddenLocation, NoResume, false)->AsStatement();
        IncrementStatement->Parent = m_BlockContext;

        Result->IncrementStatement = IncrementStatement;

        // Uses T::+(T,T) As T
        ILTree::Expression *IndexIncrement =
            InterpretForLoopOperator(
                SX_ADD,
                TerminatingConstructLocation,
                TargetType,
                ControlVariableReference,
                StepReference);

        IncrementStatement->AsStatementWithExpression().Operand1 =
            AllocateExpression(
                SX_ASG,
                TypeHelpers::GetVoidType(),
                ControlVariableReference,
                IndexIncrement,
                TerminatingConstructLocation);

        // POSITIVE 
        Variable *SignTemporary = AllocateLongLivedTemporary(
            GetFXSymbolProvider()->GetBooleanType(),
            &Step->Loc,
            Result);

        ILTree::Expression *SignReference =
            ReferToSymbol(
                Step->Loc,
                SignTemporary,
                chType_NONE,
                NULL,
                NULL,
                ExprNoFlags);

        // Uses T::-(T,T) As T
        ILTree::Expression *ZeroStep =
            InterpretForLoopOperator(
                SX_SUB,
                Step->Loc,
                TargetType,
                StepReference,
                StepReference);

        if (IsBad(ZeroStep))
        {
            MakeBad(Result);
            return;
        }

        // Uses T::>=(T,T) As B
        ILTree::Expression *PositiveCheck =
            InterpretForLoopOperator(
                SX_GE,
                Step->Loc,
                TargetType,
                StepReference,
                ZeroStep);

        if (IsBad(PositiveCheck))
        {
            MakeBad(Result);
            return;
        }

        // Converts type B to Boolean in the context of a conditional branch.
        PositiveCheck =
            ConvertWithErrorChecking(
                PositiveCheck,
                GetFXSymbolProvider()->GetBooleanType(),
                ExprIsOperandOfConditionalBranch);

        ILTree::Statement *SignCheckStatement = &AllocateStatement(SL_STMT, Step->Loc, NoResume, false)->AsStatement();
        SignCheckStatement->Parent = m_BlockContext;

        Result->SignCheckStatement = SignCheckStatement;

        SignCheckStatement->AsStatementWithExpression().Operand1 =
            AllocateExpression(
                SX_ASG,
                TypeHelpers::GetVoidType(),
                SignReference,
                PositiveCheck,
                Step->Loc);

        // CONDITION
        ILTree::Expression *LimitReference =
            ReferToSymbol(
                Limit->Loc,
                Result->LimitTemporary,
                chType_NONE,
                NULL,
                NULL,
                ExprNoFlags);

        // Uses T::<=(T,T) As B
        Result->Condition =
            InterpretForLoopOperator(
                SX_LE,
                TerminatingConstructLocation,
                TargetType,
                ControlVariableReference,
                LimitReference);

        if (IsBad(Result->Condition))
        {
            MakeBad(Result);
            return;
        }

        // Converts type B to Boolean in the context of a conditional branch.
        Result->Condition =
            ConvertWithErrorChecking(
                Result->Condition,
                GetFXSymbolProvider()->GetBooleanType(),
                ExprIsOperandOfConditionalBranch);

        if (IsBad(Result->Condition))
        {
            MakeBad(Result);
            return;
        }

        // Although the condition is semantically equivalent to:
        //
        //    If _ispositive Then
        //        If i <= _bound Then GoTo loopbody
        //    Else
        //        If i >= _bound Then GoTo loopbody
        //    End If
        //
        // we cheat a little bit and use AndAlso, OrElse bound trees because
        // the code generator will do all the branching automatically without
        // needing to encode it in the trees as a set of If blocks.  Note that
        // we cannot actually interpret the expression as using AndAlso, OrElse
        // operators because that would attempt to use the overloaded short
        // circuit operators which we do not want.

        // Uses T::>=(T,T) As B
        ILTree::Expression *NegativeStepConditionCheck =
            InterpretForLoopOperator(
                SX_GE,
                TerminatingConstructLocation,
                TargetType,
                ControlVariableReference,
                LimitReference);

        if (IsBad(NegativeStepConditionCheck))
        {
            MakeBad(Result);
            return;
        }

        // Converts type B to Boolean in the context of a conditional branch.
        NegativeStepConditionCheck =
            ConvertWithErrorChecking(
                NegativeStepConditionCheck,
                GetFXSymbolProvider()->GetBooleanType(),
                ExprIsOperandOfConditionalBranch);

        // Combine into an expression which looks like:
        //    (_ispositive AndAlso i <= _bound) OrElse (Not _ispositive AndAlso i >= _bound)
        Result->Condition =
            AllocateExpression(
                SX_ORELSE,
                GetFXSymbolProvider()->GetBooleanType(),
                AllocateExpression(
                    SX_ANDALSO,
                    GetFXSymbolProvider()->GetBooleanType(),
                    SignReference,
                    Result->Condition,
                    TerminatingConstructLocation),
                AllocateExpression(
                    SX_ANDALSO,
                    GetFXSymbolProvider()->GetBooleanType(),
                    AllocateExpression(
                        SX_NOT,
                        GetFXSymbolProvider()->GetBooleanType(),
                        SignReference,
                        TerminatingConstructLocation),
                    NegativeStepConditionCheck,
                    TerminatingConstructLocation),
                TerminatingConstructLocation);
    }
    else if (NullableElementType && TypeHelpers::IsIntrinsicOrEnumType(NullableElementType))
    {


        //Handle a for loop with a control variable with a nullable intrinsic type.
        //This is similar to the case of interpreting user defined operators because
        //we need to generate expressions for IncrementStatment, SignCheckStatement, and Condition statements.
        //These expressions will then go through the lowering phase which will generate the appropriate lifting code.
        //
        //Unlike the user defined operator case, however, we avoid generating bound trees directly.
        //In particular we create parse trees via the parser helper and then interpret them. This then calls
        //into InterpretBinaryOperand as necessary which handles creating the approriate structures needed
        //by the lifted user defined operator lowering phase. It also inserts any necessary conversions. Therefore we
        //are able to avoid having to duplicate that logic here.

        ParserHelper ph(&m_TreeStorage,For->TextSpan );

        //ControlVariable = ControlVariable + Step
        Result->IncrementStatement =
            InterpretAssignment
            (
                ph.CreateAssignment
                (
                    ph.CreateBoundExpression(ControlVariableReference),
                    ph.CreateBinaryExpression
                    (
                        ParseTree::Expression::Plus,
                        ph.CreateBoundExpression(ControlVariableReference),
                        ph.CreateBoundSymbol(Result->StepTemporary)
                    )
                ),
                false
            );

        if (Result->IncrementStatement)
        {
            Result->IncrementStatement->Parent = m_BlockContext;
            Result->IncrementStatement->ResumeIndex = NoResume;
        }

        Variable *SignTemporary = AllocateLongLivedTemporary(
            GetFXSymbolProvider()->GetBooleanType(),
            &Step->Loc,
            Result);

        //
        // SignTemporary = Step >= new NullalbleElementType()

        Result->SignCheckStatement =
            InterpretAssignment
            (
                ph.CreateAssignment
                (
                    ph.CreateBoundSymbol(SignTemporary),
                    ph.CreateBoundExpression
                    (
                        InterpretExpression
                        (
                            ph.CreateBinaryExpression
                            (
                                ParseTree::Expression::GreaterEqual,
                                ph.CreateBoundSymbol(Result->StepTemporary),
                                ph.CreateNewObject
                                (
                                    ph.CreateBoundType(NullableElementType),
                                    ph.CreateArgList(0, NULL)
                                )
                            ),
                            ExprIsOperandOfConditionalBranch
                        )
                    )
                ),
                false
            );

        if (Result->SignCheckStatement)
        {
            Result->SignCheckStatement->Parent = m_BlockContext;
            Result->SignCheckStatement->ResumeIndex = NoResume;
        }


        Result->Condition =
            InterpretConditionalOperand
            (
                ph.CreateBinaryExpression
                (
                    ParseTree::Expression::OrElse,
                    ph.CreateBinaryExpression
                    (
                        ParseTree::Expression::AndAlso,
                        ph.CreateBoundSymbol(SignTemporary),
                        ph.CreateBinaryExpression
                        (
                            ParseTree::Expression::LessEqual,
                            ph.CreateBoundExpression(ControlVariableReference),
                            ph.CreateBoundSymbol(Result->LimitTemporary)
                        )
                    ),
                    ph.CreateBinaryExpression
                    (
                        ParseTree::Expression::AndAlso,
                        ph.CreateUnaryExpression
                        (
                            ParseTree::Expression::Not,
                            ph.CreateBoundSymbol(SignTemporary)
                        ),
                        ph.CreateBinaryExpression
                        (
                            ParseTree::Expression::GreaterEqual,
                            ph.CreateBoundExpression(ControlVariableReference),
                            ph.CreateBoundSymbol(Result->LimitTemporary)
                        )
                    )
                )
            );
    }

    Result->ControlVariableReference = ControlVariableReference;
    Result->Initial = InitialValue;
    Result->Limit = Limit;
    Result->Step = Step;
}

Declaration *
Semantics::ExtensionMethodMatchesForEachRequirements
(
    ExtensionCallLookupResult * LookupResult,
    Type *AccessingInstanceType,
    const Location &SourceLocation,
    DeclarationTester *CandidateTester,
    _Out_opt_ GenericBinding ** GenericBindingContext
)
{
    ExtensionCallLookupResult::iterator_type candidateIterator = LookupResult->GetExtensionMethods();

    ExtensionCallInfo match(NULL, 0, NULL);

    while (candidateIterator.MoveNext())
    {
        ExtensionCallInfo current = candidateIterator.Current();

        if
        (
            CandidateTester(current.m_pProc) &&
            ! current.GetFreeArgumentCount() &&
            current.m_pProc->GetRequiredParameterCount() == 1
        )
        {
            Type * ReturnType = GetReturnType(current.m_pProc);

            if (current.m_pPartialGenericBinding)
            {
                ReturnType =
                    ReplaceGenericParametersWithArguments
                    (
                        ReturnType,
                        current.m_pPartialGenericBinding->m_pGenericBinding,
                        m_SymbolCreator
                    );
            }

            if (!match.m_pProc)
            {
                match = current;
            }
            else
            {
                //Candidate should be in order from highest precedence(0) to lowest precedence(N)
                Assume(match.m_precedenceLevel <= current.m_precedenceLevel, L"Why are candidates out of order?");

                if (match.m_precedenceLevel == current.m_precedenceLevel)
                {
                    return NULL;
                }
                else
                {
                    break;
                }
            }
        }
    }

    if (match.m_pProc)
    {
        if (GenericBindingContext)
        {
            GenericBindingInfo binding = match.m_pPartialGenericBinding;
            binding.ConvertToFullBindingIfNecessary
            (
                this,
                match.m_pProc
            );

            *GenericBindingContext = binding.PGenericBinding();
        }

        return match.m_pProc;
    }
    else
    {
        return NULL;
    }
}


Declaration *
Semantics::MatchesForEachRequirements
(
    Symbol * LookupResult,
    Type *AccessingInstanceType,
    const Location &SourceLocation,
    DeclarationTester *CandidateTester,
    _Inout_ bool &IsBad,
    _Out_opt_ GenericBinding **GenericBindingContext
)
{
    if (IsBad || LookupResult == NULL)
    {
        return NULL;
    }
    else if (LookupResult->IsExtensionCallLookupResult())
    {
        ExtensionCallLookupResult* ExtensionCall = LookupResult->PExtensionCallLookupResult();

        // Dev10#660568: match instance method before trying extension methods.
        Declaration* Candidate =
            MatchesForEachRequirements(
                ExtensionCall->GetInstanceMethodLookupResult(),
                AccessingInstanceType,
                SourceLocation,
                CandidateTester,
                IsBad,
                GenericBindingContext
            );

        if (Candidate)
        {
            return Candidate;
        }

        return
            ExtensionMethodMatchesForEachRequirements
            (
                ExtensionCall,
                AccessingInstanceType,
                SourceLocation,
                CandidateTester,
                GenericBindingContext
            );
    }

    Declaration * MemberCandidate = EnsureNamedRoot(LookupResult);

    do
    {
        for (Declaration *Candidate = MemberCandidate;
             Candidate;
             Candidate = Candidate->GetNextOverload())
        {
            if (CandidateTester(Candidate) &&
                !ViewAsProcedure(Candidate)->IsGeneric() &&
                ViewAsProcedure(Candidate)->GetRequiredParameterCount() == 0)
            {
                GenericBinding *CandidateGenericBindingContext =
                    DeriveGenericBindingForMemberReference(
                        AccessingInstanceType,
                        Candidate,
                        m_SymbolCreator,
                        m_CompilerHost);

                if (IsAccessible(Candidate, CandidateGenericBindingContext, AccessingInstanceType))
                {
                    if (GenericBindingContext)
                    {
                        *GenericBindingContext = CandidateGenericBindingContext;
                    }

                    return Candidate;
                }
            }
        }

    } while (MemberCandidate = FindMoreOverloadedProcedures(MemberCandidate, AccessingInstanceType, SourceLocation, IsBad));

    return NULL;
}

static bool
AlwaysTrue
(
    Type *T
)
{
    return true;
}

bool
Semantics::MatchesForEachCollectionDesignPattern
(
    Type *CollectionType,
    ILTree::Expression *Collection,
    _Out_opt_ Type **ElementType
)
{
    // The rules are that the collection type must have an
    // accessible GetEnumerator method that takes no parameters and
    // returns a type that has both:
    //
    //     an accessible MoveNext method that takes no parameters and
    //     returns bool
    //
    //     an accessible Current property that takes no parameters and
    //     is not writeonly
    //
    // NOTE: this function ONLY checks for a function named "GetEnumerator" with the appropriate properties.
    // In the spec $10.9 it has these conditions: a type C is a "collection type" if one of
    //    (1) it satisfies MatchesForEachCollectionDesignPattern (i.e. has a method named GetEnumerator() which
    //        returns a type with MoveNext/Current); or
    //    (2) it implements System.Collections.Generic.IEnumerable(Of T); or
    //    (3) it implements System.Collections.IEnumerable.
    //
    // This function ONLY checks for part (1). Callers are expected to check for (2)/(3) themselves. The
    // scenario where something satisfies (2/3) but not (1) is
    //   Class C : implements IEnumerable: function g1() as IEnumerator implements IEnumerable.GetEnumerator.
    // Clearly this class does not have a method named GetEnumerator. But it does implement IEnumerable.

    if (TypeHelpers::IsArrayType(CollectionType))
    {
        if (ElementType)
        {
            *ElementType = CollectionType->PArrayType()->GetRoot();
        }
        return true;
    }

    if (TypeHelpers::IsGenericParameter(CollectionType))
    {
        // Check to see if any constraints on the type implement the pattern.

        for (GenericTypeConstraint *Constraint = CollectionType->PGenericParam()->GetTypeConstraints();
             Constraint;
             Constraint = Constraint->Next())
        {
            if (Constraint->IsBadConstraint())
            {
                continue;
            }

            if (MatchesForEachCollectionDesignPattern(Constraint->GetType(), Collection, ElementType))
            {
                return true;
            }
        }

        return false;
    }

    if (!TypeHelpers::IsClassOrInterfaceType(CollectionType) && !TypeHelpers::IsValueType(CollectionType))
    {
        return false;
    }

    bool NameIsBad = false;
    Scope *Lookup = NULL;

    if (CollectionType->IsContainer())
    {
        Lookup = ViewAsScope(CollectionType->PContainer());
    }
    else
    {
        return false;
    }

    GenericBinding *GenericBindingContextForGetEnumerator = NULL;


    Symbol * tmp = NULL;

    BackupValue<Type *> backup_receiver_type(&m_pReceiverType);
    BackupValue<Location *> backup_receiver_location(&m_pReceiverLocation);
    m_pReceiverLocation = NULL;

    //Introduce a scope to control the life time of the backup variable
    //declared below.
    {
        //BackupValue<bool> backup_report_errors(&m_ReportErrors);

        //m_ReportErrors = false;
        m_pReceiverType = CollectionType;

        tmp =
            LookupInScope
            (
                Lookup,
                NULL,
                STRING_CONST(m_Compiler, ForEachGetEnumerator),
                NameSearchMethodsOnly,
                CollectionType,
                BINDSPACE_Normal,
                true,
                true,
                Collection->Loc,
                NameIsBad,
                &GenericBindingContextForGetEnumerator,
                -1
            );
    }


    Declaration *GetEnumerator =
        MatchesForEachRequirements
        (
            tmp,
            CollectionType,
            Collection->Loc,
            &IsFunction,
            NameIsBad,
            NULL
        );

    if (GetEnumerator == NULL)
    {
        return false;
    }

    UpdateNameLookupGenericBindingContext(tmp, CollectionType, &GenericBindingContextForGetEnumerator);

    Type *EnumeratorType =
        ReplaceGenericParametersWithArguments(
            GetReturnType(ViewAsProcedure(GetEnumerator)),
            GenericBindingContextForGetEnumerator,
            m_SymbolCreator);

    if (!TypeHelpers::IsClassInterfaceRecordOrGenericParamType(EnumeratorType))
    {
        return false;
    }

    GenericParameter *TypeParamToLookupIn;

    if (EnumeratorType->IsContainer())
    {
        Lookup = ViewAsScope(EnumeratorType->PContainer());
        TypeParamToLookupIn = NULL;
    }
    else if (TypeHelpers::IsGenericParameter(EnumeratorType))
    {
        Lookup = NULL;
        TypeParamToLookupIn = EnumeratorType->PGenericParam();
    }
    else
    {
        return false;
    }

    m_pReceiverType = EnumeratorType;

    //Create a scope to control the lifetime of the backup variable declared below.
    {
        //BackupValue<bool> backup_report_errors(&m_ReportErrors);
        //m_ReportErrors = false;

        tmp =
            LookupInScope
            (
                Lookup,
                TypeParamToLookupIn,
                STRING_CONST(m_Compiler, ForEachMoveNext),
                NameNoFlags,
                EnumeratorType,
                BINDSPACE_Normal,
                true,
                true,
                Collection->Loc,
                NameIsBad,
                NULL,
                -1
            );
    }

    Declaration *MoveNext =
        MatchesForEachRequirements(
            LookupInScope(
                Lookup,
                TypeParamToLookupIn,
                STRING_CONST(m_Compiler, ForEachMoveNext),
                NameNoFlags,
                EnumeratorType,
                BINDSPACE_Normal,
                true,
                true,
                Collection->Loc,
                NameIsBad,
                NULL,
                -1),
            EnumeratorType,
            Collection->Loc,
            &IsFunction,
            NameIsBad);

    if (!MoveNext || !TypeHelpers::IsBooleanType(GetReturnType(ViewAsProcedure(MoveNext))))
    {
        return false;
    }

    GenericBinding *GenericBindingContextForCurrent = NULL;

    tmp =
        LookupInScope
        (
            Lookup,
            TypeParamToLookupIn,
            STRING_CONST(m_Compiler, ForEachCurrent),
            NameNoFlags,
            EnumeratorType,
            BINDSPACE_Normal,
            true,
            true,
            Collection->Loc,
            NameIsBad,
            &GenericBindingContextForCurrent,
            -1
         );


    Declaration *Current =
        MatchesForEachRequirements(
            LookupInScope(
                Lookup,
                TypeParamToLookupIn,
                STRING_CONST(m_Compiler, ForEachCurrent),
                NameNoFlags,
                EnumeratorType,
                BINDSPACE_Normal,
                true,
                true,
                Collection->Loc,
                NameIsBad,
                &GenericBindingContextForCurrent,
                -1),
            EnumeratorType,
            Collection->Loc,
            &IsReadableProperty,
            NameIsBad);

    if (Current != NULL)
    {
        UpdateNameLookupGenericBindingContext(tmp, EnumeratorType, &GenericBindingContextForCurrent);

        // Dev 10 446622

        if (ElementType != NULL)
        {
            *ElementType =
                ReplaceGenericParametersWithArguments(
                    GetReturnType(ViewAsProcedure(Current)),
                    GenericBindingContextForCurrent,
                    m_SymbolCreator);
        }

        return true;
    }

    return false;
}

bool
Semantics::MatchesForEachCollectionDesignPattern
(
    _In_ ILTree::Expression *Collection,
    _Out_opt_ Type **ElementType
)
{
    return MatchesForEachCollectionDesignPattern(Collection->ResultType, Collection, ElementType);
}

void
Semantics::InterpretForEachArrayStatement
(
    _In_ ParseTree::ForEachInStatement *ForEach,
    ILTree::Expression *Array,
    ILTree::Expression *ControlVariableReference,
    Scope *ForLocals,
    ILTree::Expression *ControlVariableTempInitializer
)
{
    // The construct
    //
    //     For Each i In c
    //         <loop body>
    //     Next
    //
    // where c is an array type of rank 1, becomes:
    //
    //     Dim a As C = c
    //     Dim x As Integer = 0
    //     Do While x < len(a)    'len(a) represents the LDLEN opcode
    //         i = a(x)
    //         <loop body>
    //         x += 1
    //     Loop

    VSASSERT(
        TypeHelpers::IsArrayType(Array->ResultType) &&
        Array->ResultType->PArrayType()->GetRank() == 1,
        "expected one-dimensional array");

    ILTree::ForEachBlock *Result = &AllocateBlock(SB_FOR_EACH, ForEach)->AsForEachBlock();
    Result->ControlVaribleTempInitializer = ControlVariableTempInitializer;
    Result->Locals = ForLocals;
    Result->TerminatingResumeIndex = ForEach->ResumeIndexForEndNext;
    SetFlag32(Result, SBF_DO_WHILE);

    Result->Collection = &m_TreeAllocator.xCopyBilTreeForScratch(Array)->AsExpression();

    // Copy the control variable reference to maintain a true tree rather than a graph
    //
    // DevDiv Bug 40236: Do an unsafe copy here.  It's legal for a control variable
    // to be any LValue and copying the side effect here is completely intentional
    Result->ControlVariableReference = &m_TreeAllocator.xCopyBilTreeUnsafe(ControlVariableReference)->AsExpression();

    // Assign the array into a temporary and initialize the indexer to 0.
    //      Dim a As C = c
    //      Dim x As Integer = 0

    Variable *ArrayTemporary = NULL;
    Variable *IndexTemporary = NULL;

    Result->Initializer =
        AllocateExpression(
            SX_SEQ,
            TypeHelpers::GetVoidType(),
            CaptureInLongLivedTemporary(
                Array,
                ArrayTemporary,
                Result),
            CaptureInLongLivedTemporary(
                ProduceConstantExpression(
                    0,
                    ForEach->TextSpan,
                    GetFXSymbolProvider()->GetIntegerType()
                    IDE_ARG(0)),
                IndexTemporary,
                Result),
           ForEach->TextSpan);

    // Loop termination condition.
    //      x < len(a)

    Result->Conditional =
        AllocateExpression(
            SX_LT,
            GetFXSymbolProvider()->GetBooleanType(),
            AllocateSymbolReference(
                IndexTemporary,
                GetFXSymbolProvider()->GetIntegerType(),
                NULL,
                ForEach->TextSpan),
            AllocateExpression(
                SX_ARRAYLEN,
                GetFXSymbolProvider()->GetIntegerType(),
                AllocateSymbolReference(
                    ArrayTemporary,
                    ArrayTemporary->GetType(),
                    NULL,
                    ForEach->TextSpan),
                ForEach->TextSpan),
            ForEach->TextSpan);

    // Create assignment for each loop iteration.
    //      i = a(x)

    ILTree::Expression *ArrayIndex =
        AllocateExpression(
            SX_INDEX,
            TypeHelpers::GetElementType(Array->ResultType->PArrayType()),
            AllocateSymbolReference(
                ArrayTemporary,
                ArrayTemporary->GetType(),
                NULL,
                ForEach->Collection->TextSpan),
            AllocateExpression(
                SX_LIST,
                TypeHelpers::GetVoidType(),
                AllocateSymbolReference(
                    IndexTemporary,
                    GetFXSymbolProvider()->GetIntegerType(),
                    NULL,
                    ForEach->Collection->TextSpan),
                ForEach->Collection->TextSpan),
            ForEach->Collection->TextSpan);
    ArrayIndex->AsIndexExpression().DimensionCount = 1;

    // "Current" is converted to the type of the control variable as if
    // it were an explicit cast. This language rule exists because there is
    // no way to write a cast, and the type of the IEnumerator.Current is Object,
    // which would make
    //
    //  Dim I, A() As Integer : For Each I in A : Next
    //
    // invalid in strict mode.

    ArrayIndex =
        ConvertWithErrorChecking(
            ArrayIndex,
            ControlVariableReference->ResultType,
            ExprHasExplicitCastSemantics);

    Result->CurrentIndexAssignment =
        AllocateExpression(
            SX_ASG,
            TypeHelpers::GetVoidType(),
            ControlVariableReference,
            ArrayIndex,
            ControlVariableReference->Loc);

    // Interpret the loop body.

    if (TryInterpretBlock(ForEach->AsExecutableBlock()))
    {

        // Increment the loop indexer.  We don't want this to show up if we're generating XML.
        //      x += 1

        const Location &Hidden = Location::GetHiddenLocation();

        if (!m_IsGeneratingXML)
        {
            // Don't use AllocateStatement here to link because it links it to the current body of
            // statements. We don't want that because we only want to see the IncrementIndex
            // statement once during codegen and that should be through ForEach->IncrementStatement.
            //
            ILTree::Statement *IndexIncrement = &AllocateStatement(SL_STMT, Hidden, NoResume, false)->AsStatement();
            IndexIncrement->Parent = m_BlockContext;

            // Needs to be set so that any continue statements in this loop can branch to it
            Result->IncrementStatement = IndexIncrement;

            IndexIncrement->AsStatementWithExpression().Operand1 =
                AllocateExpression(
                    SX_ASG,
                    TypeHelpers::GetVoidType(),
                    AllocateSymbolReference(
                        IndexTemporary,
                        GetFXSymbolProvider()->GetIntegerType(),
                        NULL,
                        Hidden),
                    AllocateExpression(
                        SX_ADD,
                        GetFXSymbolProvider()->GetIntegerType(),
                        AllocateSymbolReference(
                            IndexTemporary,
                            GetFXSymbolProvider()->GetIntegerType(),
                            NULL,
                            Hidden),
                        ProduceConstantExpression(
                            1,
                            Hidden,
                            GetFXSymbolProvider()->GetIntegerType()
                            IDE_ARG(0)),
                        Hidden),
                    Hidden);
        }
        PopBlockContext();
    }

}

void
Semantics::InterpretForEachStringStatement
(
    _In_ ParseTree::ForEachInStatement *ForEach,
    ILTree::Expression *String,
    ILTree::Expression *ControlVariableReference,
    Scope *ForLocals,
    ILTree::Expression *ControlVariableTempInitializer
)
{
    // The construct
    //
    //     For Each c In s
    //         <loop body>
    //     Next
    //
    // where s is as string, becomes:
    //
    //     Dim a As String = s
    //     Dim x As Integer = 0
    //     Dim limit as Integer = s.Length
    //
    //     Do While x < limit
    //         c = a.Chars(x)
    //         <loop body>
    //         x += 1
    //     Loop

    VSASSERT(String && TypeHelpers::IsStringType(String->ResultType),"expected String");

    ILTree::ForEachBlock *Result = &AllocateBlock(SB_FOR_EACH, ForEach)->AsForEachBlock();	
    Result->ControlVaribleTempInitializer = ControlVariableTempInitializer;
    Result->Locals = ForLocals;
    Result->TerminatingResumeIndex = ForEach->ResumeIndexForEndNext;
    SetFlag32(Result, SBF_DO_WHILE);

    Result->Collection = &m_TreeAllocator.xCopyBilTreeForScratch(String)->AsExpression();

    // Copy the control variable reference to maintain a true tree rather than a graph
    //
    // DevDiv Bug 40236: Do an unsafe copy here.  It's legal for a control variable
    // to be any LValue and copying the side effect here is completely intentional
    Result->ControlVariableReference = &m_TreeAllocator.xCopyBilTreeUnsafe(ControlVariableReference)->AsExpression();

    // Assign the array into a temporary and initialize the indexer to 0.
    //      Dim a As String = s
    //      Dim x As Integer = 0

    Variable *StringTemporary = NULL;
    Variable *IndexTemporary = NULL;

    Result->Initializer =
        AllocateExpression(
            SX_SEQ,
            TypeHelpers::GetVoidType(),
            CaptureInLongLivedTemporary(
                String,
                StringTemporary,
                Result),
            CaptureInLongLivedTemporary(
                ProduceConstantExpression(
                    0,
                    ForEach->TextSpan,
                    GetFXSymbolProvider()->GetIntegerType()
                    IDE_ARG(0)),
                IndexTemporary,
                Result),
           ForEach->TextSpan);

    // Get the length of the String and assigning it to a temporary to be used loop upper bound
    //      Dim StringLength as Integer
    //      StringPool::StringLength = c.length
    //
    ILTree::Expression *StringLengthReference =
        InterpretQualifiedExpression(
            AllocateSymbolReference(
                StringTemporary,
                GetFXSymbolProvider()->GetStringType(),
                NULL,
                ForEach->Collection->TextSpan),
            STRING_CONST(m_Compiler, Length),
            ParseTree::Expression::DotQualified,
            ForEach->Collection->TextSpan,
            ExprForceRValue);

    if (IsBad(StringLengthReference))
    {
        MarkContainingLambdaOrMethodBodyBad();
        return;
    }

    Variable *LimitTemporary = NULL;
    ILTree::Expression *StringLength----g =
            CaptureInLongLivedTemporary(
                StringLengthReference,
                LimitTemporary,
                Result);

    // Combine the Initialization Statements with SX_SEQ nodes
   Result->Initializer =
        AllocateExpression(
            SX_SEQ,
            TypeHelpers::GetVoidType(),
            Result->Initializer,
            StringLength----g,
            ForEach->TextSpan);


    // Loop termination condition.
    //      x < len(a) i.e. x < limit
    Result->Conditional =
        AllocateExpression(
            SX_LT,
            GetFXSymbolProvider()->GetBooleanType(),
            AllocateSymbolReference(
                IndexTemporary,
                GetFXSymbolProvider()->GetIntegerType(),
                NULL,
                ForEach->TextSpan),
            AllocateSymbolReference(
                LimitTemporary,
                GetFXSymbolProvider()->GetIntegerType(),
                NULL,
                ForEach->TextSpan),
            ForEach->TextSpan);

    // Create assignment for each loop iteration.
    //      c = a.Chars(x)

    ILTree::Expression *StringCharsPropertyReference =
        InterpretQualifiedExpression(
            AllocateSymbolReference(
                StringTemporary,
                GetFXSymbolProvider()->GetStringType(),
                NULL,
                ForEach->Collection->TextSpan),
            STRING_CONST(m_Compiler, Chars),
            ParseTree::Expression::DotQualified,
            ForEach->Collection->TextSpan,
            ExprIsExplicitCallTarget);

    ExpressionList *StringCharsPropertyArgument =
         AllocateExpression(
                SX_LIST,
                TypeHelpers::GetVoidType(),
                AllocateExpression(
                    SX_ARG,
                    TypeHelpers::GetVoidType(),
                    AllocateSymbolReference(
                        IndexTemporary,
                        GetFXSymbolProvider()->GetIntegerType(),
                        NULL,
                        ForEach->Collection->TextSpan),
                    ForEach->Collection->TextSpan),
                ForEach->Collection->TextSpan);

    ILTree::Expression *StringCharsCall =
        InterpretCallExpressionWithNoCopyout(
            ForEach->Collection->TextSpan,
            StringCharsPropertyReference,
            chType_NONE,
            StringCharsPropertyArgument,
            false,
            ExprForceRValue,
            NULL);


    // Result of "StringCharsCall" is converted to the type of the control variable as if
    // it were an explicit cast. This will trap any conversion errors from char to the control variable.
    //
    //  Dim I as Integer : For Each I in "Hi" : Next
    //
    //  Invalid to convert from Char to Integer I

    ILTree::Expression *CharIndexedInString =
        ConvertWithErrorChecking(
            StringCharsCall,
            ControlVariableReference->ResultType,
            ExprHasExplicitCastSemantics);

    Result->CurrentIndexAssignment =
        AllocateExpression(
            SX_ASG,
            TypeHelpers::GetVoidType(),
            ControlVariableReference,
            CharIndexedInString,
            ControlVariableReference->Loc);

    // Interpret the loop body.

    if (TryInterpretBlock(ForEach->AsExecutableBlock()))
    {

        // Increment the loop indexer.  We don't want this to show up if we're generating XML.
        //      x += 1

        const Location &Hidden = Location::GetHiddenLocation();

        if (!m_IsGeneratingXML)
        {
            // Don't use AllocateStatement here to link because it links it to the current body of
            // statements. We don't want that because we only want to see the IncrementIndex
            // statement once during codegen and that should be through ForEach->IncrementStatement.
            //
            ILTree::Statement *IndexIncrement = &AllocateStatement(SL_STMT, Hidden, NoResume, false)->AsStatement();
            IndexIncrement->Parent = m_BlockContext;

            // Needs to be set so that any continue statements in this loop can branch to it
            Result->IncrementStatement = IndexIncrement;

            IndexIncrement->AsStatementWithExpression().Operand1 =
                AllocateExpression(
                    SX_ASG,
                    TypeHelpers::GetVoidType(),
                    AllocateSymbolReference(
                        IndexTemporary,
                        GetFXSymbolProvider()->GetIntegerType(),
                        NULL,
                        Hidden),
                    AllocateExpression(
                        SX_ADD,
                        GetFXSymbolProvider()->GetIntegerType(),
                        AllocateSymbolReference(
                            IndexTemporary,
                            GetFXSymbolProvider()->GetIntegerType(),
                            NULL,
                            Hidden),
                        ProduceConstantExpression(
                            1,
                            Hidden,
                            GetFXSymbolProvider()->GetIntegerType()
                            IDE_ARG(0)),
                        Hidden),
                    Hidden);
        }

        PopBlockContext();
    }
}

ILTree::Expression *
Semantics::InterpretForEachStatementCollection
(
    _In_ ILTree::Expression *Collection,
    _Out_ bool *IsEnumerable,
    _Out_ Type **ElementType
)
{
    if (IsBad(Collection))
    {
        return NULL;
    }

    // If the collection matches the design pattern, use that.
    // Otherwise, if the collection implements IEnumerable, convert it
    // to IEnumerable (which matches the design pattern).
    //
    // The design pattern is preferred to the interface implementation
    // because it is more efficient.

    *IsEnumerable = false;

    Type *GenericIEnumerable =
        GetFXSymbolProvider()->IsTypeAvailable(FX::GenericIEnumerableType) ?
            GetFXSymbolProvider()->GetType(FX::GenericIEnumerableType) :
            NULL;
    Type *IEnumerable = GetFXSymbolProvider()->GetType(FX::IEnumerableType);

    DynamicArray<GenericTypeBinding *> ImplGenericIEnumerableBindings;

    DynamicArray<GenericTypeBinding *> *ImplBindings = NULL;
    unsigned MultipleBindingError = 0;
    Type *TargetCollectionType = NULL;

    if (MatchesForEachCollectionDesignPattern(Collection, ElementType))
    {
        // 










        // Do nothing, we're cool.
        *IsEnumerable = true;
    }
    else if (GenericIEnumerable &&
        TypeHelpers::IsOrInheritsFromOrImplements(
            Collection->ResultType,
            GenericIEnumerable,
            m_SymbolCreator,
            false,
            &ImplGenericIEnumerableBindings,
            m_CompilerHost))
    {
        VSASSERT(ImplGenericIEnumerableBindings.Count() > 0,
                    "How can an IEnumerable be inherited or implemented without a corresponding binding ?");

        ImplBindings = &ImplGenericIEnumerableBindings;
        MultipleBindingError = ERRID_ForEachAmbiguousIEnumerable1;
        TargetCollectionType = ImplGenericIEnumerableBindings.Element(0);
        *IsEnumerable = true;
        if (ElementType)
        {
            *ElementType = TargetCollectionType->PGenericTypeBinding()->GetArgument(0);
        }
    }
    else if ((IEnumerable && IsOrInheritsFromOrImplements(Collection->ResultType, IEnumerable)) ||
             (Collection->ResultType->IsObject() && !m_UsingOptionTypeStrict))
    {
        TargetCollectionType = IEnumerable;
        *IsEnumerable = true;
        if (ElementType)
        {
            *ElementType = GetFXSymbolProvider()->GetObjectType();
        }
    }
    else
    {
        // The collection neither matches the design pattern nor
        // implements IEnumerable.

        ReportSemanticError(
            ERRID_ForEachCollectionDesignPattern1,
            Collection->Loc,
            Collection->ResultType);

        return NULL;
    }

    // Check for implementations of multiple instantiations of this interface and if so
    // report an ambiguity error.

    if (ImplBindings && !TypeHelpers::EquivalentTypeBindings(*ImplBindings))
    {
        // Multiple instantiations of generic interface found found

        // "Expression is of type '|1', which is an ambiguous collection type since it implements
        // multiple instantiations of '<type>(Of T)'.

        ReportSemanticError(
            MultipleBindingError,
            Collection->Loc,
            Collection->ResultType);

        return NULL;
    }

    if (TargetCollectionType)
    {
        // Cast to target collection type.
        Collection =
            ConvertWithErrorChecking(
                Collection,
                TargetCollectionType,
                ExprNoFlags);

        if (IsBad(Collection))
        {
            return NULL;
        }
    }

    return Collection;
}

void
Semantics::InterpretForEachStatement
(
    _In_ ParseTree::ForEachInStatement *ForEach,
    _Out_ ILTree::Expression* ControlVariableReference,
    Scope *ForLocals
)
{
    AssertIfNull( ControlVariableReference );

    Variable *EnumeratorTemporary = NULL;
    Type *EnumeratorType = NULL;
    Type *ElementType = NULL;
    ILTree::Expression *EnumeratorCapture = NULL;
    ILTree::Expression *ResetReference = NULL;
    bool IsEnumerable = false;
    Variable *ControlVariableTemp = NULL;
    ILTree::Expression *ControlVariableTempInitializer = NULL;
 
    InitializerInferInfo inferInfo( &m_InitializerInferStack );

    // Check if we have circular reference when we have inference.

    if( ShouldInfer( ControlVariableReference ) && !IsBad( ControlVariableReference ) )
    {
        inferInfo.Variable = ControlVariableReference->AsSymbolReferenceExpression().Symbol->PVariable();
    }
        
     ILTree::Expression *Collection =
        InterpretStatementOperand(
            ForEach->Collection,
            ExprForceRValue);        
                
    if (IsBad(ControlVariableReference) || IsBad(Collection))
    {
        return;
    }

    if( inferInfo.CircularReferenceDetected )
    {
        AssertIfFalse( OptionInferOn() );
        // Error was reported in name lookup; don't write error here.
        return;
    }

    inferInfo.Restore();

    if (!(Collection = InterpretForEachStatementCollection(Collection, &IsEnumerable, &ElementType)))
    {
        return;
    }

    // LIFTING OF FOR-EACH CONTROL VARIABLES
    //
    // For Each <ControlVariable> In <EnumeratorExpression>
    //	<Body>
    // Next
    //
    // VB has two forms of "For Each". The first is where the <ControlVariable>
    // binds to an already-existing variable. The second is where it introduces a new
    // control variable.
    //
    // Execution of For Each (the form that introduces a new control variable) is as follows:
    // [1] Allocate a new storage location for the control variable and initialize it to default(T)
    // [2] Evaluate <EnumeratorExpression>, where any reference to "x" binds to the location in [1]
    // [3] Allocate a new temporary and initialize it to the result of calling GetEnumerator() on [2]
    // [4] We will call MoveNext/Current to execute the <Body> a number of times...
    // For each iteration of the body,
    // [5] Allocate a new storage location for the control variable
    // [6] Initialize it to [3].Current
    // [7] Execute <Body>, where any reference to "x" binds to the location in [5] for this iteration
    //
    // Of course, none of this allocation is observable unless there are lambdas.
    // If there are no lambdas then we can make do with only a single allocation for the whole thing.
    // But there may be lambdas in either <EnumeratorExpression> or <Body>...
    //
    //
    // IMPLEMENTATION:
    //
    // The Closures.cpp code will, if there were a lambda inside <Body> that captures x, it will
    // lift x into a closure which is allocated once for each iteration of the body.
    // (This allocation is initialized by copying the previously allocated closure if there was one).
    //
    // However, if <EnumeratorExpression> referred to x, then we'll need to allocate another one for it.
    // This rewrite is how we do it:
    //	 Dim tmp = default(CollectionVariableType)
    //	 For Each x In <EnumeratorExpression> {tmp/x}
    //	   <Body>
    //	 Next
    //

    bool PossibleForEachLoopHeaderSubstitute = 
        ControlVariableReference->bilop == SX_SYM &&        
        ControlVariableReference->AsSymbolReferenceExpression().Symbol->IsVariable() &&
        ControlVariableReference->AsSymbolReferenceExpression().Symbol->PVariable()->GetImmediateParent() == ForLocals;

    if (PossibleForEachLoopHeaderSubstitute)
    {
        Variable *ControlVariable = ControlVariableReference->AsSymbolReferenceExpression().Symbol->PVariable();

        VariableSubstitutionVisitor variableSubstitutionVisitor;
            
        if (variableSubstitutionVisitor.ContainsReference(&Collection, ControlVariable))
        {   
            ControlVariableTempInitializer = CaptureInLongLivedTemporary(
                ConvertWithErrorChecking( 
                   AllocateExpression(
                        SX_NOTHING, 
                        GetFXSymbolProvider()->GetObjectType(),
                        ForEach->TextSpan),    
                    ControlVariable->GetType(),
                    ExprNoFlags),                                        
                ControlVariableTemp, 
                m_BlockContext);
                        
            variableSubstitutionVisitor.ReplaceReference(&Collection, ControlVariable, ControlVariableTemp);
        }
    }
    
    // Verify that the control variable reference is an lvalue and does
    // not refer to the control variable of an enclosing loop. Also
    // verify that the reference in the Next is to the same symbol.

    if (!VerifyForLoopControlReference(ControlVariableReference, ForEach))
    {
        return;
    }

    if( !m_ExplicitLoopVariableCreated && OptionInferOn() )
    {
        GenerateWarningIfNoExplicitFieldReference( ForEach, ControlVariableReference );
    }

    // The construct
    //
    //     For Each i In c
    //         <loop body>
    //     Next
    //
    // has several forms depending on the context, but its basic form is:
    //
    //     Dim e As E = c.GetEnumerator()
    //     Do While e.MoveNext()
    //        i = e.Current
    //        <loop body>
    //     Loop
    //
    // As an optimization, if c is an array type of rank 1, the form becomes:
    //
    //     Dim a As C = c
    //     Dim x As Integer = 0
    //     Do While x < len(a)    'len(a) represents the LDLEN opcode
    //         i = a(x)
    //         <loop body>
    //         x += 1
    //     Loop
    //
    // To support disposable enumerators, the compiler will generate code to dispose the
    // enumerator after loop termination.  Only when E implements IDisposable can this be done.
    // The one exception to this rule is when E is specifically IEnumerator, in which case
    // the compiler will generate code to dynamically query the enumerator to determine
    // if it implements IDisposable.
    //
    // If E is IEnumerator the loop becomes:
    //
    //     Try
    //         Dim e As IEnumerator = c.GetEnumerator()
    //         Do While e.MoveNext()
    //            i = e.Current
    //         Loop
    //     Finally
    //         If TypeOf e Is IDisposable Then
    //             CType(e, IDisposable).Dispose()
    //         End If
    //     End Try
    //
    // If E is known at compile time to implement IDisposable the loop becomes:
    //
    //     Try
    //         Dim e As E = c.GetEnumerator()
    //         Do While e.MoveNext()
    //            i = e.Current
    //         Loop
    //     Finally
    //         If Not e Is Nothing Then
    //             CType(e, IDisposable).Dispose()
    //         End If
    //     End Try
    //
    // The exception to these forms is the existence of On Error in which case the Try/Finally
    // block will be eliminated (instead the body of the Finally will immediately follow
    // the end of the loop).

    // If we have a one-dimensional array, we can enumerate the array manually rather than
    // use an enumerator.  This results in much faster IL.


    if (TypeHelpers::IsArrayType(Collection->ResultType) && Collection->ResultType->PArrayType()->GetRank() == 1)
    {
        if( m_ExplicitLoopVariableCreated && ShouldInfer( ControlVariableReference ) )
        {
            // Infer control variable type
            if (!ControlVariableReference->AsSymbolReferenceExpression().Symbol->PVariable()->IsExplicitlyTyped())
            {
                InferVariableType(ControlVariableReference, Collection->ResultType->PArrayType()->GetRoot());
            }
        }

        // 


        InterpretForEachArrayStatement(ForEach, Collection, ControlVariableReference, ForLocals, ControlVariableTempInitializer);
        return;
    }
    else if (TypeHelpers::IsStringType(Collection->ResultType))
    {
        if( m_ExplicitLoopVariableCreated && ShouldInfer( ControlVariableReference ) )
        {
            // Infer control variable type
            if (!ControlVariableReference->AsSymbolReferenceExpression().Symbol->PVariable()->IsExplicitlyTyped())
            {
                InferVariableType(ControlVariableReference, GetFXSymbolProvider()->GetCharType());
            }
        }

        // Can Optimize by using the "Chars" property of the String to iterate over the string
        InterpretForEachStringStatement(ForEach, Collection, ControlVariableReference, ForLocals, ControlVariableTempInitializer);
        return;
    }
    else if (Collection->ResultType->IsObject())
    {
        // Will reset the Block to the 'For Each' once it's created

        if (!GetFXSymbolProvider()->IsTypeAvailable(FX::IEnumeratorType))
        {
            ReportMissingType(FX::IEnumeratorType, ForEach->Collection->TextSpan);
            MarkContainingLambdaOrMethodBodyBad();
            return;
        }
        EnumeratorType = GetFXSymbolProvider()->GetType(FX::IEnumeratorType);
        EnumeratorTemporary = AllocateLongLivedTemporary(EnumeratorType, &ForEach->TextSpan, m_BlockContext);
    }
    else if (IsEnumerable)
    {
        BackupValue<Type *> backup_receiver_type(&m_pReceiverType);
        BackupValue<Location *> backup_receiver_location(&m_pReceiverLocation);

        m_pReceiverType = Collection->ResultType;
        m_pReceiverLocation = &Collection->Loc;

        // Process a call to GetEnumerator.
        ILTree::Expression *GetEnumeratorReference =
            InterpretQualifiedExpression(
                Collection,
                STRING_CONST(m_Compiler, ForEachGetEnumerator),
                ParseTree::Expression::DotQualified,
                ForEach->Collection->TextSpan,
                ExprForceRValue);


        if (IsBad(GetEnumeratorReference))
        {
            MarkContainingLambdaOrMethodBodyBad();
            return;
        }

        // Capture the result of the call to GetEnumerator in a temporary.
        // Will reset the Block to the 'For Each' once it's created
        EnumeratorType = GetEnumeratorReference->ResultType;
        EnumeratorCapture = CaptureInLongLivedTemporary(GetEnumeratorReference, EnumeratorTemporary, m_BlockContext);
    }

    ILTree::Expression * MoveNextReference = NULL;

    //create a scope to control the life time of the backup variables below.
    {
        BackupValue<Type *> backup_receiver_type(&m_pReceiverType);
        BackupValue<Location *> backup_receiver_location(&m_pReceiverLocation);

        m_pReceiverType = EnumeratorType;
        m_pReceiverLocation = &Collection->Loc;

        // Interpret the MoveNext call.
        MoveNextReference =
            InterpretQualifiedExpression(
                AllocateSymbolReference(
                    EnumeratorTemporary,
                    EnumeratorType,
                    NULL,
                    ForEach->Collection->TextSpan),
                STRING_CONST(m_Compiler, ForEachMoveNext),
                ParseTree::Expression::DotQualified,
                ForEach->Collection->TextSpan,
                ExprForceRValue);
    }

    if (IsBad(MoveNextReference))
    {
        MarkContainingLambdaOrMethodBodyBad();
        return;
    }

    MoveNextReference =
        ConvertWithErrorChecking(MoveNextReference, GetFXSymbolProvider()->GetBooleanType(), ExprNoFlags);

    if (IsBad(MoveNextReference))
    {
        return;
    }

    ILTree::Expression *CurrentReference = NULL;

    {
        BackupValue<Type *> backup_receiver_type(&m_pReceiverType);
        BackupValue<Location *> backup_receiver_location(&m_pReceiverLocation);

        m_pReceiverType = EnumeratorType;
        m_pReceiverLocation = &Collection->Loc;

        // Interpret the Current reference and assign it to the loop control variable.
        CurrentReference =
            InterpretQualifiedExpression(
                AllocateSymbolReference(
                    EnumeratorTemporary,
                    EnumeratorType,
                    NULL,
                    ForEach->Collection->TextSpan),
                STRING_CONST(m_Compiler, ForEachCurrent),
                ParseTree::Expression::DotQualified,
                ForEach->Collection->TextSpan,
                ExprForceRValue);
    }

    if (IsBad(CurrentReference))
    {
        MarkContainingLambdaOrMethodBodyBad();
        return;
    }

    if( m_ExplicitLoopVariableCreated && ShouldInfer( ControlVariableReference ) )
    {
        // Infer control variable type if not already inferred.
        if (!ControlVariableReference->AsSymbolReferenceExpression().Symbol->PVariable()->IsExplicitlyTyped())
        {
            // If we have a multi-dimension array, we should infer the parent type.
            if (TypeHelpers::IsArrayType(Collection->ResultType) && Collection->ResultType->PArrayType()->GetRank() > 1)
            {
                InferVariableType(ControlVariableReference, Collection->ResultType->PArrayType()->GetRoot());
            }
            else
            {
                InferVariableType(ControlVariableReference, CurrentReference);
            }
        }
    }

    // "Current" is converted to the type of the control variable as if
    // it were an explicit cast. This language rule exists because there is
    // no way to write a cast, and the type of the IEnumerator.Current is Object,
    // which would make
    //
    //  Dim I, A() As Integer : For Each I in A : Next
    //
    // invalid in strict mode.

    CurrentReference =
        ConvertWithErrorChecking(
            CurrentReference,
            ControlVariableReference->ResultType,
            ExprHasExplicitCastSemantics);

    // We know a priori that arrays enumerate over the element type, so perform
    // a special check to verify the element type matches the loop control variable type.

    if (!IsBad(CurrentReference) && TypeHelpers::IsArrayType(Collection->ResultType))
    {
        Type *CollectionElementType = TypeHelpers::GetElementType(Collection->ResultType->PArrayType());
        Procedure *OperatorMethod = NULL;
        GenericBinding *OperatorMethodGenericContext = NULL;
        bool OperatorMethodIsLifted;

        ConversionClass ConversionClassification =
            ClassifyConversion(ControlVariableReference->ResultType, CollectionElementType, OperatorMethod, OperatorMethodGenericContext, OperatorMethodIsLifted);

        if (ConversionClassification == ConversionError)
        {
            ReportSemanticError(
                ERRID_TypeMismatch2,
                ForEach->Collection->TextSpan,
                CollectionElementType,
                ControlVariableReference->ResultType);
        }
    }

    ILTree::Expression *CurrentIndexAssignment =
        AllocateExpression(
            SX_ASG,
            TypeHelpers::GetVoidType(),
            ControlVariableReference,
            CurrentReference,
            ControlVariableReference->Loc);

    // Determine if we need to generate code to dispose the enumerator.

    Type *IEnumerator = GetFXSymbolProvider()->GetType(FX::IEnumeratorType);
    Type *IDisposable = GetFXSymbolProvider()->GetType(FX::IDisposableType);

    bool NeedToDispose = false;

    // No need to dispose if the collection is an array.
    if (!TypeHelpers::IsArrayType(Collection->ResultType)
       )
    {
        NeedToDispose =
            IsOrInheritsFromOrImplements(EnumeratorType, IDisposable) ||
            (EnumeratorType == IEnumerator);
    }

    // Determine if we need to wrap the loop with a Try block to guarantee enumerator disposal.

    bool UseTryFinally = false;
    const Location &Hidden = Location::GetHiddenLocation();
    ILTree::ExecutableBlock *ForEachTry = NULL;

    // We don't wrap the loop with a Try block if On Error is present.
    if (NeedToDispose && !m_ProcedureContainsOnError)
    {
        UseTryFinally = true;

        ForEachTry = &AllocateStatement(SB_TRY, Hidden, NoResume)->AsExecutableBlock();
        EstablishBlockContext(ForEachTry);
        ForEachTry->EndConstructLocation = Hidden;
    }

    // Build the ForEach context.

    ILTree::ForEachBlock *Result = &AllocateBlock(SB_FOR_EACH, ForEach)->AsForEachBlock();
    Result->Locals = ForLocals;
    Result->TerminatingResumeIndex = ForEach->ResumeIndexForEndNext;
    Result->ControlVaribleTempInitializer = ControlVariableTempInitializer;
	
    // Copy the control variable reference to maintain a true tree rather than a graph
    //
    // DevDiv Bug 40236: Do an unsafe copy here.  It's legal for a control variable
    // to be any LValue and copying the side effect here is completely intentional
    Result->ControlVariableReference = &m_TreeAllocator.xCopyBilTreeUnsafe(ControlVariableReference)->AsExpression();
    Result->Collection = &m_TreeAllocator.xCopyBilTreeForScratch(Collection)->AsExpression();

    Result->Initializer = EnumeratorCapture;
    SetFlag32(Result, SBF_DO_WHILE);
    Result->Conditional = MoveNextReference;
    Result->CurrentIndexAssignment = CurrentIndexAssignment;

    // Interpret the body of the loop.
    InterpretAndPopBlock(ForEach->AsExecutableBlock());

    // If necessary, generate the enumerator disposal mechanism.

    if (NeedToDispose)
    {
        if (UseTryFinally)
        {
            // Pop the Try and begin the Finally.

            PopBlockContext();

            ILTree::ExecutableBlock *ForEachFinally = &AllocateFinallyStatement(&ForEachTry->AsTryBlock(), Hidden, NoResume)->AsExecutableBlock();
            EstablishBlockContext(ForEachFinally);
            ForEachFinally->EndConstructLocation = Hidden;
        }

        GenerateDisposeCallBasedOnWellKnownPattern(
            EnumeratorTemporary,
            ForEach->TextSpan,
            true);

        if (UseTryFinally)
        {
            PopBlockContext();  // Pop the Finally
        }
    }
}

void
Semantics::GenerateDisposeCallBasedOnWellKnownPattern
(
    Variable *ObjectToDispose,
    const Location &ErrorLoc,
    bool ProtectAgainstInvalidCast
)
{
    Type *IDisposable = GetFXSymbolProvider()->GetType(FX::IDisposableType);

    if (IDisposable == NULL)
    {
        ReportMissingType(FX::IDisposableType, ErrorLoc);
        return;
    }

    Declaration *DisposeMethod =
        ImmediateLookup(
            ViewAsScope(IDisposable->PContainer()),
            STRING_CONST(m_Compiler, ComDispose),
            BINDSPACE_Normal);

    if (DisposeMethod == NULL)
    {
        ReportuntimeHelperNotFoundError(
            ErrorLoc,
            STRING_CONST(m_Compiler, SystemIDisposableDispose));

        return;
    }

    Type *TypeOfObjectToDispose = ObjectToDispose->GetType();
    ILTree::Expression *ValueToNullCheck = NULL;
    ILTree::Expression *BaseReference = NULL;
    Location Hidden = Location::GetHiddenLocation();

    ILTree::Expression *ObjectToDisposeReference =
        AllocateSymbolReference(
            ObjectToDispose,
            TypeOfObjectToDispose,
            NULL,
            Hidden);

    if (IsOrInheritsFromOrImplements(TypeOfObjectToDispose, IDisposable))
    {
        // "IDisposable" pattern chosen and the object to dispose is known to implement it.

        if (TypeHelpers::IsValueTypeOrGenericParameter(TypeOfObjectToDispose))    // A generic parameter can be a value type too
        {
            // For value types and generic parameters, when the dispose needs to be invoked from
            // IDisposable, this should result in a constrained call to avoid boxing.

            BaseReference =
                MakeValueTypeOrTypeParamBaseReferenceToProcedure(
                    DisposeMethod,
                    ObjectToDisposeReference,
                    false,
                    true);      // Generate constrained reference for value types too
        }
        else
        {
            // For reference types, no need to even cast since the clr know that the the member
            // is available from this type either directly or indirectly.
            //
            BaseReference = ObjectToDisposeReference;
        }
    }
    else
    {
        // "IDisposable" pattern chosen, but the object to dispose may not implement it.

        if (ProtectAgainstInvalidCast)
        {
            // 

            BaseReference =
                Convert(
                    ObjectToDisposeReference,
                    IDisposable,
                    ExprHasTryCastSemantics,
                    ConversionNarrowing);

            ValueToNullCheck =
                Convert(
                AllocateSymbolReference(
                    ObjectToDispose,
                    TypeOfObjectToDispose,
                    NULL,
                    Hidden),
                IDisposable,
                ExprHasTryCastSemantics,
                ConversionNarrowing);
        }
        else
        {
            BaseReference =
                Convert(
                    ObjectToDisposeReference,
                    IDisposable,
                    ExprHasDirectCastSemantics,     // Direct cast intentional to throw when !ProtectAgainstInvalidCast
                    ConversionNarrowing);
        }
    }

    if (ValueToNullCheck == NULL &&
        !TypeHelpers::IsValueType(TypeOfObjectToDispose))
    {
        // Null check required - so generate the expression that needs to be checked

        ValueToNullCheck =
            AllocateSymbolReference(
                ObjectToDispose,
                TypeOfObjectToDispose,
                NULL,
                Hidden);

        if (TypeHelpers::IsGenericParameter(TypeOfObjectToDispose))
        {
            // For generic parameters, the NULL check needs to be done against the boxed value

            ValueToNullCheck =
                AllocateExpression(
                    SX_DIRECTCAST,
                    GetFXSymbolProvider()->GetObjectType(),
                    ValueToNullCheck,
                    NULL,
                    Hidden);
        }
    }

    if (ValueToNullCheck != NULL)
    {
        // Guard against the cast failing or the object to dispose being Nothing.

        ILTree::ExecutableBlock *If = &AllocateStatement(SB_IF_BLOCK, Hidden, NoResume)->AsExecutableBlock();
        EstablishBlockContext(If);
        If->EndConstructLocation = Hidden;
        If->TerminatingResumeIndex = NoResume;

        ILTree::IfBlock *Guard = &AllocateStatement(SB_IF, Hidden, NoResume)->AsIfBlock();
        EstablishBlockContext(Guard);

        Guard->Conditional =
            AllocateExpression(
                SX_ISNOT,
                GetFXSymbolProvider()->GetBooleanType(),
                ValueToNullCheck,
                AllocateExpression(
                    SX_NOTHING,
                    ValueToNullCheck->ResultType,
                    Hidden),
                Hidden);
    }

    // Generate the call to "Dispose"

    ILTree::Statement *DisposeCall = AllocateStatement(SL_STMT, Hidden, NoResume);
    DisposeCall->AsStatementWithExpression().Operand1 =
        AllocateExpression(
            SX_CALL,
            TypeHelpers::GetVoidType(),
            AllocateSymbolReference(
                DisposeMethod,
                TypeHelpers::GetVoidType(),
                NULL,
                Hidden,
                NULL),      // NULL generic binding because IDisposable is non-generic.
            NULL,
            Hidden);

    DisposeCall->AsStatementWithExpression().Operand1->AsCallExpression().MeArgument = BaseReference;

    if (ValueToNullCheck != NULL)
    {
        PopBlockContext();  // Pop the If statement
        PopBlockContext();  // Pop the If block
    }
}

bool
Semantics::VerifyForLoopControlReference
(
    ILTree::Expression *ControlVariableReference,
    _In_ ParseTree::ForStatement *For
)
{

    if (!HasFlag32(ControlVariableReference, SXF_LVALUE))
    {
        // A property reference is not allowed as the control variable of any
        // kind of For statement.

        if (IsPropertyReference(ControlVariableReference))
        {
            ReportSemanticError(
                ERRID_LoopControlMustNotBeProperty,
                ControlVariableReference->Loc);
        }
        else
        {
            ReportAssignmentToRValue(ControlVariableReference);
        }
        return false;
    }


    // An Await isn't allowed in the control variable expression, e.g. For (Await tc).i = 1 to 10
    if (GetLocalResumableKind() == ILTree::TaskResumable || GetLocalResumableKind() == ILTree::SubResumable)
    {
        bool ContainsAwait = false;
        auto predicate = [&ContainsAwait](ILTree::ILNode*) -> bool { ContainsAwait = true; return false; };
        BoundTreeNodeVisitor visitor(SX_AWAIT, predicate);
        visitor.VisitExpression(&ControlVariableReference);
        if (ContainsAwait)
        {
            ReportSemanticError(ERRID_LoopControlMustNotAwait, ControlVariableReference->Loc);
            return false;
        }
    }


    // Verify that the referenced symbol is not the same as one used as
    // a control variable in an enclosing for loop.
    //
    // This check is somewhat bogus in that it can't tell the difference between
    // references to the same field in different objects, but it is consistent
    // with VB6.
    //

    // Start searching at the current block context (rather than the current
    // block context's parent) because the For Each block we are verifying
    // hasn't been build yet.
    Declaration *   RefSym = ReferencedSymbol(ControlVariableReference);
    for (ILTree::Statement *Parent = m_BlockContext; Parent; Parent = Parent->Parent)
    {
        if (Parent->bilop == SB_FOR || Parent->bilop == SB_FOR_EACH)
        {
            if (!IsBad(Parent) &&
                RefSym ==
                    ReferencedSymbol(
                        Parent->bilop == SB_FOR ?
                            Parent->AsForBlock().ControlVariableReference :
                           Parent->AsForEachBlock().ControlVariableReference))
            {
                if (RefSym)
                {
                    ReportSemanticError(ERRID_ForIndexInUse1, For->ControlVariable->TextSpan, RefSym );
                }
                else
                {
                    ReportSemanticError(ERRID_ForIndexInUse,For->ControlVariable->TextSpan);
                }
            }
        }
    }

    // Verify that the reference in the Next is to the same symbol.

    if (For->NextVariable)
    {
        ILTree::Expression *NextVariableReference =
            InterpretStatementOperand(
                For->NextVariable,
                ExprPropagatePropertyReference);

        if (!IsBad(NextVariableReference) &&
            ReferencedSymbol(NextVariableReference, false) != RefSym)
        {
            if (RefSym)
            {
                ReportSemanticError(ERRID_NextForMismatch1, For->NextVariable->TextSpan, RefSym);
            }
            else
            {
                ReportSemanticError( ERRID_NextForMismatch, For->NextVariable->TextSpan);
            }
        }
    }

    // force 'for control locals' as used to avoid being reported
    if (RefSym && RefSym->IsVariable())
    {
        RefSym->PVariable()->SetIsUsed();
    }
    return true;
}

void
Semantics::InterpretWithStatement
(
    ILTree::Expression *WithValue,
    _Inout_ ILTree::WithBlock *Result
)
{
    // Dev10 #783478 Make sure an Array Literal is snapped to its inferred type.
    if (!IsBad(WithValue) && WithValue->ResultType->IsArrayLiteralType() && WithValue->bilop==SX_ARRAYLITERAL)
    {
        // Convert the array literal to its inferred array type, reporting warnings/errors if necessary
        WithValue = ConvertArrayLiteral(&WithValue->AsArrayLiteralExpression(), NULL);

        // ConvertArrayLiteral claimes to never return NULL if its second argument is NULL.
        AssertIfNull(WithValue);
        if (WithValue == NULL)
        {
            MakeBad(Result);
            return;
        }
    }

    if (IsBad(WithValue))
    {
        MakeBad(Result);
        return;
    }

    // Cannot use a valueless expression as a With value.

    if (TypeHelpers::IsVoidType(WithValue->ResultType))
    {
        ReportSemanticError(
            ERRID_VoidValue,
            WithValue->Loc);

        MakeBad(Result);
        return;
    }
    else if (TypeHelpers::IsValueTypeOrGenericParameter(WithValue->ResultType))
    {
        // Force capturing of all side-effect-producing expressions
        // in the With value. Retain the With expression sans side effects
        // for future cloning in processing leading dot references.

        ILTree::Expression *WithCaptures = NULL;
        ILTree::Expression *WithValueCopy = NULL;
        UseTwiceLongLived(WithValue, WithCaptures, WithValueCopy, Result);

        Result->TemporaryBindAssignment = WithCaptures;
        SetFlag32(Result, SBF_WITH_RECORD);

        Result->RecordReference = WithValueCopy;
    }
    else
    {
        VSASSERT(TypeHelpers::IsReferenceType(WithValue->ResultType), "expected reference type here");

        // Bind the with temporary to the With value.

        Variable *Temporary;
        Result->TemporaryBindAssignment = CaptureInLongLivedTemporary(WithValue, Temporary, Result);

        // Create an assignment to clear the With temporary at block end.

        ILTree::Expression *Zero =
            AllocateExpression(
                SX_NOTHING,
                WithValue->ResultType,
                WithValue->Loc);

        Result->TemporaryClearAssignment =
            (ILTree::ExpressionWithChildren *)AllocateExpression(
                SX_ASG,
                TypeHelpers::GetVoidType(),
                ReferToSymbol(WithValue->Loc, Temporary, chType_NONE, NULL, NULL, ExprNoFlags),
                Convert(Zero, WithValue->ResultType, ExprNoFlags, ConversionWidening),
                Zero->Loc);
    }

    if (HasFlag32(WithValue, SXF_LVALUE))
    {
        SetFlag32(Result, SBF_WITH_LVALUE);
    }
}

ILTree::WithBlock *
Semantics::InterpretWithStatement
(
    _In_ ParseTree::ExpressionBlockStatement *With
)
{
    ILTree::WithBlock *Result = &AllocateBlock(SB_WITH, With)->AsWithBlock();

    ILTree::Expression *WithValue =
        InterpretStatementOperand(
            With->Operand,
            ExprNoFlags);  // We cannot use  ExprForceRValue flag here because WithValue should stay LValue, if it is.  

    InterpretWithStatement(WithValue, Result);

    return Result;
}

ILTree::TryBlock*
Semantics::InterpretTryStatement
(
    _In_ ParseTree::ExecutableBlockStatement *Try
)
{
    ILTree::ExecutableBlock *TryBlock = AllocateBlock(SB_TRY, Try);
    // This is the only place where Try blocks are allocated from user code.
    // All other allocations are assumed to be synthetic.
    SetFlag32(TryBlock, SBF_TRY_NOT_SYNTHETIC);

    if (m_ProcedureContainsOnError)
    {
        ReportSemanticError(
            ERRID_TryAndOnErrorDoNotMix,
            Try->TextSpan);
    }

    return &TryBlock->AsTryBlock();
}

void
Semantics::InterpretCatchStatement
(
    _In_ ParseTree::CatchStatement *Catch
)
{
    ILTree::CatchBlock *Result = &AllocateBlock(SB_CATCH, Catch)->AsCatchBlock();
    bool CatchTypeOK = false;

    Type *CatchType = NULL;

    if (m_StatementLambdaInterpreter)
    {
        ThrowIfNull(m_StatementLambdaInterpreter->m_Tree);
        m_StatementLambdaInterpreter->m_Tree->fSeenCatch = true;
    }
    if (WithinProcedure())
    {
        m_ProcedureTree->fSeenCatch = true;
    }

    // Either, both, or neither of the declaration and expression might be present.

    if (Catch->Name.Name)
    {
        Declaration *CatchSymbol = NULL;

        // The parse tree for the caught object is either a name or a declaration.

        if (Catch->Type == NULL)
        {
            bool NameIsBad = false;

            CatchSymbol =
                EnsureNamedRoot
                (
                    InterpretName
                    (
                        Catch->Name.Name,
                        m_Lookup,
                        NULL,   // No Type parameter lookup
                        NameSearchIgnoreExtensionMethods,
                        ContainingClass(),
                        Catch->Name.TextSpan,
                        NameIsBad,
                        NULL,   // binding context is determined later for all the different scenarios
                        -1
                    )
                );

            if (NameIsBad)
            {
                MarkContainingLambdaOrMethodBodyBad();
            }
            else
            {
                if (CatchSymbol == NULL)
                {
                    // Don't implicitly declare a local.

                    ReportSemanticError(
                        ERRID_NameNotDeclared1,
                        Catch->Name.TextSpan,
                        Catch->Name.Name);
                }
            }
        }
        else
        {
            Result->Locals =
                m_SymbolCreator.GetHashTable(
                    NULL,
                    m_Lookup,
                    true,
                    Catch->LocalsCount + 1,
                    NULL);

            m_Lookup = Result->Locals;

            // Manufacture local variable for Catch
            Declared::MakeLocalFromCatch(
                m_CompilerHost,
                &m_TreeStorage,
                Catch,
                m_CompilationCaches,
                m_Errors,
                ContainingClass()->PClass(),
                m_SourceFile,
                m_Procedure,
                m_Lookup,
                m_Compiler,
                PerformObsoleteChecks() ? TypeResolvePerformObsoleteChecks : TypeResolveNoFlags,
                &CatchSymbol);
        }

        if (CatchSymbol && !IsBad(CatchSymbol))
        {
            if (m_StatementLambdaInterpreter && CatchSymbol->IsVariable())
            {
                // This catch variable is a local inside a lambda.
                CatchSymbol->PVariable()->SetIsLambdaMember(true);
            }
            
            if (!CatchSymbol->IsLocal() || CatchSymbol->PVariable()->IsConstant())
            {
                ReportSemanticError(
                    ERRID_CatchVariableNotLocal1,
                    Catch->Name.TextSpan,
                    CatchSymbol);
            }
            else
            {
                // Verify that the caught object has a type that is
                // (or is derived from) Exception.

                //CatchTypeOK = false;
                CatchType = GetDataType(CatchSymbol->PVariable());

                if (TypeHelpers::IsPointerType(CatchType))
                {
                    CatchType = TypeHelpers::GetReferencedType(CatchType->PPointerType());

                    // If the catch symbol is a byref, then allocate a temporary so we can store the exception
                    // object that will appear on the stack.  We do this because store-indirect requires the
                    // param to be loaded on the stack first.
                    //
                    Result->ExceptionTemporary =
                        AllocateShortLivedTemporary(CatchType);
                }

                if (TypeHelpers::IsClassType(CatchType) || TypeHelpers::IsGenericParameter(CatchType))
                {
                    ClassOrRecordType *Exception = GetFXSymbolProvider()->GetType(FX::ExceptionType)->PClass();
                    CatchTypeOK = Exception && IsOrInheritsFrom(CatchType, Exception);
                }

                if (!CatchTypeOK && !TypeHelpers::IsBadType(CatchType))
                {
                    ReportSemanticError(
                        ERRID_CatchNotException1,
                        Catch->Type ?
                            Catch->Type->TextSpan :
                            Catch->Name.TextSpan,
                        CatchType);
                }

                // Only locals and arguments are allowed as valid catch variables, so need to synthesize
                // a generic binding context
                Result->CatchVariableTree =
                    ReferToSymbol(
                        Catch->Name.TextSpan,
                        CatchSymbol,
                        chType_NONE,
                        NULL,
                        IsGenericOrHasGenericParent(CatchSymbol->GetParent()) ?
                            SynthesizeOpenGenericBinding(CatchSymbol->GetParent(), m_SymbolCreator) :
                            NULL,
                        ExprNoFlags);

                if (IsBad(Result->CatchVariableTree))
                {
                    MarkContainingLambdaOrMethodBodyBad();
                }
            }
        }
    }

    if (Catch->WhenClause)
    {
        // Verify that the When expression produces a boolean result.

        Result->WhenExpression = InterpretConditionalOperand(Catch->WhenClause);
    }

    // check for overlapping catch
    if (!CatchType && !Catch->Name.Name && !Catch->Type)
    {
        // empty catch are assumed to cath Exception for overlapping purposes
        CatchType = GetFXSymbolProvider()->GetType(FX::ExceptionType);
        CatchTypeOK = true;
    }
    if (CatchTypeOK && CatchType && !TypeHelpers::IsBadType(CatchType))
    {
        for (
            ILTree::Statement* current = Result->Prev;
            (current && current->bilop == SB_CATCH);
            current = current->Prev)
        {
            ILTree::CatchBlock *currentCatch = &current->AsCatchBlock();

            // a filter can result in false and even the most general catch would not overlap the subsequent ones
            if (!current->AsCatchBlock().WhenExpression)
            {
                if (!current->AsCatchBlock().CatchVariableTree)
                {
                    if (!CatchType->IsObject())
                    {
                        // a simple catch would cover anything, unless we have a catch as object.
                        ReportSemanticError(WRNID_DuplicateCatch,Result->Loc, CatchType);
                        SetFlag32(Result, SBF_OVERLAPED_CATCH);
                    }
                }
                else
                {
                    if ( TypeHelpers::EquivalentTypes(CatchType, current->AsCatchBlock().CatchVariableTree->ResultType))
                    {
                        ReportSemanticError(WRNID_DuplicateCatch,Result->Loc, CatchType);
                        SetFlag32(Result, SBF_OVERLAPED_CATCH);
                    }
                    else if (IsOrInheritsFrom(CatchType, current->AsCatchBlock().CatchVariableTree->ResultType))
                    {
                        ReportSemanticError(WRNID_OverlapingCatch,Result->Loc, CatchType, current->AsCatchBlock().CatchVariableTree->ResultType);
                        SetFlag32(Result, SBF_OVERLAPED_CATCH);
                    }
                }
            }
        }
    }
}

void
Semantics::InterpretUsingStatement
(
 _In_ ParseTree::UsingStatement *Using
)
// using statement names a resource that is suposed to be disposed on completion.
// The resource can be an expression or a list of local variables with initializers.
// the type of the resource must implement System.IDispose
// An using statement of the form:
//      using Expression
//          list_of_statements
//      end using

// is translated into:
//      System.Idispose temp = Expression
//      try
//          list_of_statements
//      finally
//          if not( nothing is temp )       // consider:(Microsoft) use the new introduced 'isnot'operator
//              CType(temp, IDisposable).Dispose()
//          end if
//      end try
//
// when the resource is a using locally declared variable no temporary is generated but the variable is readonly
// An using statement of the form:
//      using v as new myDispose
//          list_of_statements
//      end using

// is translated into:
//      Dim v as  new myDispose
//      try
//         list_of_statements
//      finally
//          if not( nothing is v )       // consider:(Microsoft) use the new introduced 'isnot'operator
//              CType(v, IDisposable).Dispose()
//          end if
//      end try
//
// Multiple variable resources are released in a nested structure try. An using statement of the form:
//      using v1 as new myDispose, v2 as myDispose = new myDispose()
//          list_of_statements
//      end using
//
// is translated into:
//      Dim v1 as  new myDispose
//      try
//          Dim v2 as myDispose = new myDispose()
//          try
//              list_of_statements
//          finally
//              if not( nothing is v2 )       // consider:(Microsoft) use the new introduced 'isnot'operator
//                  CType(v2, IDisposable).Dispose()
//              end if
//          end try
//      finally
//          if not( nothing is v1 )       // consider:(Microsoft) use the new introduced 'isnot'operator
//              CType(v1, IDisposable).Dispose()
//          end if
//      end try
//
//
{

    // Encose all the Using declarations in an extra block for correct scoping.
    AllocateBlock(SB_USING, Using);

    if (Using->ControlVariableDeclaration && !Using->ControlVariableDeclaration->HasSyntaxError)
    {

        ParseTree::VariableDeclarationStatement *ControlVariableDeclaration = Using->ControlVariableDeclaration->AsVariableDeclaration();
        ParseTree::VariableDeclarationList *Declarations = ControlVariableDeclaration->Declarations;

        // count the variables for hash table of the block
        unsigned DeclarationCount = 0;
        for (ParseTree::VariableDeclarationList *DeclarationList = Declarations;
             DeclarationList;
             DeclarationList = DeclarationList->Next)
        {
            ParseTree::VariableDeclaration *Declaration = DeclarationList->Element;
            // normally the inner loop has exactly only one declarator for using.
            // Just count once. Playing safe for now. Consider to remove
            for (ParseTree::DeclaratorList *DeclaratorList = Declaration->Variables;
                 DeclaratorList;
                 DeclaratorList = DeclaratorList->Next)
            {
                DeclarationCount++;
            }
        }


        // make the hash for the using enclosing extra block
        m_BlockContext->LocalsCount = DeclarationCount;
        m_BlockContext->Locals =
            m_SymbolCreator.GetHashTable(
                NULL,
                m_Lookup,
                true,
                DeclarationCount + 1,
                NULL);
        m_Lookup = m_BlockContext->Locals;
        MakeInnerTryConstructForVariableDeclaration(Using, Declarations);

    }
    else
    {
        // make an empty hTable for the extra block
        m_BlockContext->Locals =
            m_SymbolCreator.GetHashTable(
                NULL,
                m_Lookup,
                true,
                1,
                NULL);
        m_Lookup = m_BlockContext->Locals;


        Variable *UsingObjectTemp = NULL;
        ILTree::Expression *UsingExpression;
        Type *IDisposable = GetFXSymbolProvider()->GetType(FX::IDisposableType);

        UsingExpression =
            InterpretStatementOperand(
                Using->ControlExpression,
                ExprForceRValue);

        if (!IsBad(UsingExpression))
        {
            // We need to make the Operand evaluationLocation appear
            // to be at the same code location as the Using so that
            // stepping works correctly and
            // especially for the "Set Next Statement" scenario
            bool HasErrors = false;
            const Location &UsingLocation = Using->TextSpan;

            bool lateBinding = (!m_UsingOptionTypeStrict && UsingExpression->ResultType->IsObject());
            bool validDispose = IDisposable && IsOrInheritsFromOrImplements(UsingExpression->ResultType, IDisposable);

            if ( lateBinding || validDispose)
            {
                // Capture the using expression to a temporary anyway if it is a late binding.
                // The temporary is disposed in the finally block.
                // A castclass to IDispose is added in the late binding case

                ILTree::Statement *AssignmentStatement = AllocateStatement(SL_STMT, UsingLocation, NoResume);
                AssignmentStatement->AsStatementWithExpression().Operand1 =
                    CaptureInLongLivedTemporary(UsingExpression, UsingObjectTemp, m_BlockContext);

                // if no valid dispose and late binding, warn for that extra cast class.
                if ( !validDispose && WarnOptionStrict() && m_ReportErrors)
                {
                    StringBuffer buf,buf1,buf2;
                    // "Implicit conversion from '|1' to '|2'."
                    ResLoadStringRepl(WRNID_ImplicitConversion2,
                                      &buf,
                                      ExtractErrorName(UsingExpression->ResultType,buf1),
                                      ExtractErrorName(IDisposable,buf2));
                    // substitutes into BC42016 "|1"...
                    ReportSemanticError(WRNID_ImplicitConversionSubst1, Using->ControlExpression->TextSpan, buf.GetString());
                }
            }
            else
            {
                // no late binding and no valid Dispose()
                ReportSemanticError(
                    ERRID_UsingRequiresDisposePattern,
                    Using->ControlExpression->TextSpan,
                    UsingExpression->ResultType);
                HasErrors = true;
            }

            ILTree::TryBlock *TryStatementForUsing = &AllocateBlock(SB_TRY, Using)->AsTryBlock();
            InterpretAndPopBlock(Using->AsExecutableBlock());

            if (!HasErrors)
            {
                MakeFinallyPartOfUsing(
                    Using,
                    UsingObjectTemp,
                    TryStatementForUsing);
            }
        }
    }

    PopBlockContext(); // pop the context created by AllocateBlock(SB_USING, Using)
}


// Should we report a WRNID_MutableStructureInUsing warning given that the 'typeToCheck' is the type of the Using variable?
// The purpose of this function is to avoid code duplication in 'Semantics::MakeInnerTryConstructForVariableDeclaration'.
// This is not a general purpose helper.
static bool
ShouldReportMutableStructureInUsing( Type* typeToCheck)
{
    if (typeToCheck->IsContainer())
    {
        BCSYM_Container * pContainer = typeToCheck->PContainer();

        if (IsStructOnly(pContainer))
        {
            BCSYM_NamedRoot *psymChild;
            BCITER_CHILD_ALL iter(pContainer);

            while (psymChild = iter.GetNext())
            {
                if (psymChild->IsVariable() && !psymChild->PVariable()->IsShared() && !psymChild->PVariable()->IsReadOnly())
                {
                    return true;
                }
            }
        }
    }
    else
    {
        AssertIfFalse(typeToCheck->IsVoidType()); 
    }

    return false;
}

void
Semantics::MakeInnerTryConstructForVariableDeclaration
(
    _In_ ParseTree::UsingStatement *Using,
    _In_ ParseTree::VariableDeclarationList *Declarations
)
{
    Type *IDisposable = GetFXSymbolProvider()->GetType(FX::IDisposableType);
    ILTree::Expression *Initialization = NULL;
    ILTree::Expression *Target = NULL;

    ParseTree::VariableDeclarationStatement *ControlVariableDeclaration = Using->ControlVariableDeclaration->AsVariableDeclaration();

    ParseTree::VariableDeclaration *Decl = Declarations->Element;

    // step 1: make a local in the current block context from the declaration

    Declared::MakeLocalsFromDeclarationTrees(
        m_CompilerHost,
        &m_TreeStorage,
        m_CompilationCaches,
        m_Errors,
        m_SourceFile,
        m_Procedure,
        m_Lookup,
        ControlVariableDeclaration,
        Decl,
        false,  // not being called from ParseTreeService
        PerformObsoleteChecks() ? TypeResolvePerformObsoleteChecks : TypeResolveNoFlags,
        DECLF_LocalInUsingReadOnly,
        NULL,
        m_InLambda);          // make all 'Using' variables as readonly

    // Step 2: Do several semantic checkings.

    // because each 'using' declaration must have an initializer and no multiple declarators can
    // be combined with the initialer, each declaration has only one declarator.

    // make sure the declaration has an initializer
    bool HasErrors = false;
    if (Decl->Opcode != ParseTree::VariableDeclaration::WithInitializer &&
        Decl->Opcode != ParseTree::VariableDeclaration::WithNew)
    {
        HasErrors = true;
        ReportSemanticError(
            ERRID_UsingResourceVarNeedsInitializer,
            Decl->TextSpan);

    }
    else
    {
        // and if the initalizer is present make sure there are no multiple declarators
        if (Decl->Variables->Next)
        {
            HasErrors = true;
            ReportSemanticError(
                ERRID_InitWithMultipleDeclarators,
                Decl->TextSpan);
        }
    }

    if (!HasErrors)
    {

        // 



        // Step 3: Look at the initializer, interpret it it, create the assignment statement for the initializer

        // As for any local, there are 3 different kinds of initialization:
        //
        //  -- Explicit array sizes in declarators imply creating an array. Not supported.
        //      The resource variable type implements System.IDisposable and System.Array cannot be derived from
        //  -- Declaration "As New" implies creating an instance.
        //  -- Explicit initialization.


        ParseTree::DeclaratorList *Variables = Decl->Variables;
        ParseTree::Declarator *Variable = Variables->Element;

        if (Variable->ArrayInfo)
        {
            ReportSemanticError(
                ERRID_UsingResourceVarCantBeArray,
                Decl->TextSpan);
            HasErrors = true;
        }
        else
        {
            ParseTree::NameExpression DeclReference;
            DeclReference.Opcode = ParseTree::Expression::Name;
            DeclReference.TextSpan = Variable->TextSpan;
            DeclReference.FirstPunctuator.Line = 0;
            DeclReference.FirstPunctuator.Column = 0;
            DeclReference.Name = Variable->Name;

            Target =
                InterpretStatementOperand(
                    &DeclReference,
                    ExprNoFlags);   // in order to allow initializer do not use ExprIsAssignmentTarget here. Using variables are read only locals that can be set only by initializer.

            if (IsBad(Target))
            {
                return;
            }

            Location InitBoundaries;

            InitBoundaries.m_lBegLine = Variable->TextSpan.m_lBegLine;
            InitBoundaries.m_lBegColumn = Variable->TextSpan.m_lBegColumn;

            // Set the End information for the Initialization.
            InitBoundaries.m_lEndLine = Decl->TextSpan.m_lEndLine;
            InitBoundaries.m_lEndColumn = Decl->TextSpan.m_lEndColumn;

            // Deal with straightforward initializations.

            if (Decl->Opcode == ParseTree::VariableDeclaration::WithInitializer)
            {
                Initialization =
                    InterpretInitializer(Decl->AsInitializer()->InitialValue, Target);
            }

            // Deal with "As New" declarations.
            //
            //   v as new T(1,2,3)
            //   v as new T(1,2,3) With {.Id = 1, .Name = "A"}
            //
            // is equivalent to
            //
            //   v as T = new T(1,2,3)
            //   v as T = new T(1,2,3) With {.Id = 1, .Name = "A"}

            else if (Decl->Opcode == ParseTree::VariableDeclaration::WithNew)
            {
                Initialization =
                    InitializeConstructedVariable
                    (
                        Target,
                        Decl->AsNew()->Arguments.Values,
                        Decl->Type ? &Decl->Type->TextSpan : NULL,
                        Decl->AsNew()->ObjectInitializer,
                        Decl->AsNew()->CollectionInitializer,
                        Decl->AsNew()->CollectionInitializer ? & Decl->AsNew()->From : NULL,
                        &Decl->TextSpan
                    );
            }

            else
            {
                VSASSERT(FALSE, "Unsuported initializer for using resource variable.");
            }

            if (m_IsGeneratingXML)
            {
                ILTree::Statement *Result = AllocateStatement(SL_VAR_DECL, InitBoundaries, NoResume);

                ExpressionList *BoundDeclaration =
                        AllocateExpression(
                            SX_LIST,
                            TypeHelpers::GetVoidType(),
                            NULL,
                            NULL,
                            Declarations->TextSpan);

                Result->AsStatementWithExpression().Operand1 = BoundDeclaration;

                BoundDeclaration->AsExpressionWithChildren().Left =
                    AllocateExpression(
                        SX_LIST,
                        TypeHelpers::GetVoidType(),
                        Initialization ? Initialization : Target,
                        NULL,
                        Variables->TextSpan);
            }
            else if (Initialization)
            {
                ILTree::Statement *InitStatement = AllocateStatement(SL_STMT, InitBoundaries, NoResume);
                InitStatement->AsStatementWithExpression().Operand1 = Initialization;
            }

            // Step 4: make sure the variable type implements IDisposable

            // keep similar structure with 'using expression' case for clarity sake
            bool lateBinding = (!m_UsingOptionTypeStrict && Target->ResultType->IsObject());
            bool validDispose = IDisposable && IsOrInheritsFromOrImplements(Target->ResultType, IDisposable);

            if (lateBinding || validDispose)
            {
                // the using structure generates anyway.
                // The variable is disposed in the finally block.
                // A castclass to IDispose is added in the late binding case

                // if no valid dispose and late binding, warn for that extra cast class.
                if ( !validDispose && WarnOptionStrict() && m_ReportErrors)
                {
                    StringBuffer buf,buf1,buf2;
                    // "Implicit conversion from '|1' to '|2'."
                    ResLoadStringRepl(WRNID_ImplicitConversion2,
                                      &buf,
                                      ExtractErrorName(Target->ResultType,buf1),
                                      ExtractErrorName(IDisposable,buf2));
                    // substitutes into BC42016 "|1"...
                    ReportSemanticError(WRNID_ImplicitConversionSubst1, Decl->TextSpan, buf.GetString());
                }
            }
            else
            {
                HasErrors = true;
                ReportSemanticError(
                    ERRID_UsingRequiresDisposePattern,
                    Decl->TextSpan,
                    Target->ResultType);
            }

            // set the variable as 'used' ahead for dfa ----isys.
            VSASSERT(Target->bilop == SX_SYM && Target->AsSymbolReferenceExpression().pnamed && Target->AsSymbolReferenceExpression().pnamed->IsVariable(), "bad USING control variable");
            Target->AsSymbolReferenceExpression().pnamed->PVariable()->SetIsUsed();

            // Dev10 #666593: Warn if the type of the variable is not a refernce type or an immutable structure.
            if (!TypeHelpers::IsReferenceType(Target->ResultType))
            {
                if (Target->ResultType->IsGenericParam())
                {
                    if (TypeHelpers::IsValueType(Target->ResultType))
                    {
                        // constrained as structure

                        // There is a weird case when a concrete structure type can be a constraint "inherited" from overriden generic method.

                        Type *ClassConstraint =
                            GetClassConstraint(
                                Target->ResultType->PGenericParam(),
                                m_CompilerHost,
                                &m_TreeStorage,
                                true,   // ReturnArraysAs"System.Array"
                                false   // false == don't ReturnValuesAs"System.ValueType"or"System.Enum"
                                );

                        AssertIfFalse(ClassConstraint == NULL || TypeHelpers::IsValueType(ClassConstraint));
                        
                        if ( ClassConstraint == NULL || ( TypeHelpers::IsValueType(ClassConstraint) && ShouldReportMutableStructureInUsing(ClassConstraint)) )
                        {
                            ReportSemanticError(WRNID_MutableStructureInUsing, Variable->TextSpan, Variable->Name.Name);
                        }
                    }
                    else
                    {
                        ReportSemanticError(WRNID_MutableGenericStructureInUsing, Variable->TextSpan, Variable->Name.Name);
                    }
                }
                else if ( ShouldReportMutableStructureInUsing(Target->ResultType))
                {
                    ReportSemanticError(WRNID_MutableStructureInUsing, Variable->TextSpan, Variable->Name.Name);
                }
            }
        }
    }

    // Step 5: Open a try block. If the DeclarationList member is the last interpret the 'Using' block. Otherwise go recursivelly for the next declaration
    // On returning from recursion generate finaly clause

    ILTree::ExecutableBlock* tryBlock = AllocateBlock(SB_TRY, Using);
    tryBlock->Loc = Location::GetHiddenLocation();

    if (Declarations->Next)
    {
        MakeInnerTryConstructForVariableDeclaration(Using, Declarations->Next);
        PopBlockContext(); // pop the context created by AllocateBlock(SB_TRY, Using)
    }
    else
    {
        InterpretAndPopBlock(Using->AsExecutableBlock()); // this do not create a new block context, but pops it on return
    }

    if (!HasErrors && Initialization && !IsBad(Initialization))
    {
        MakeFinallyPartOfUsing(
            Using,
            Target->AsSymbolReferenceExpression().pnamed->PVariable(),
            &tryBlock->AsTryBlock());
    }
}

void
Semantics::MakeFinallyPartOfUsing
(
    ParseTree::UsingStatement *Using,
    Variable *UsingVariable,
    _Out_opt_ ILTree::TryBlock *TryStatementForUsing
)
{
    Type *IDisposable = GetFXSymbolProvider()->GetType(FX::IDisposableType);

    VSASSERT((!m_UsingOptionTypeStrict && UsingVariable->GetType()->IsObject()) ||
            (IDisposable && IsOrInheritsFromOrImplements(UsingVariable->GetType(), IDisposable)),
            "Surprising non-disposable type.");

    const Location &EndSyncLocation =
        Using->AsExecutableBlock()->TerminatingConstruct ?
            Using->AsExecutableBlock()->TerminatingConstruct->TextSpan : Location::GetHiddenLocation();

    ILTree::FinallyBlock *Finally = AllocateFinallyStatement(TryStatementForUsing, EndSyncLocation, NoResume);
    EstablishBlockContext(Finally);

    GenerateDisposeCallBasedOnWellKnownPattern(
        UsingVariable,
        Using->TextSpan,
        false);

    PopBlockContext();  // Pop the Finally
}

Variable *
Semantics::InterpretSyncLockStatement
(
    _In_ ParseTree::ExpressionBlockStatement *SyncLock,
    _Out_ ILTree::TryBlock *&TryStatementForSynclock,
    _Out_ Variable * & pLockTakenVar
)
{
    Variable *CapturedObject = NULL;
    TryStatementForSynclock = NULL;

    ILTree::Expression *LockedObject =
        InterpretStatementOperand(
            SyncLock->Operand,
            ExprForceRValue);

    CapturedObject =
        InterpretSyncLockStatement(
            LockedObject,
            SyncLock->TextSpan,
            SyncLock->Operand->TextSpan,
            pLockTakenVar);

    TryStatementForSynclock = &AllocateBlock(SB_TRY, SyncLock)->AsTryBlock();

    // Dev10 #578762 If pLockTakenVar!=NULL, we need to
    // call Monitor.Enter(retVar, pLockTakenVar) as the first statement in the try block
    if (pLockTakenVar)
    {
        AssertIfFalse(CapturedObject && CapturedObject->IsLocal());
        
        CallMonitorEnter_ObjectByRefBoolean
        (
            SyncLock->TextSpan,
            CapturedObject,
            NULL, // No generic binding context for CapturedObject because it is a local variable
            pLockTakenVar
        );
    }
    
    return CapturedObject;
}

// Caller should create and manage the try statement
// Dev10 #578762 If pLockTakenVar!=NULL on exit, the caller is also responsible to 
// call Monitor.Enter(retVar, pLockTakenVar) as the first statement in the try block
Variable *
Semantics::InterpretSyncLockStatement
(
    ILTree::Expression *LockedObject,
    const Location &TreeLocation,
    const Location &OperandLocation,
    _Out_ Variable * & pLockTakenVar
)
{
    Variable *CapturedObject = NULL;
    pLockTakenVar = NULL;

    if (!IsBad(LockedObject))
    {
        // We need to make the Operand evaluationLocation appear
        // to be at the same code location as the SyncLock so that
        // stepping works correctly and
        // especially for the "Set Next Statement" scenario
        const Location &SyncLockLocation = TreeLocation;

        if (LockedObject->ResultType &&
            TypeHelpers::IsReferenceType(LockedObject->ResultType))
        {
            // Capture the locked object to a temporary.
            // (It will be needed both to get the lock and to release it.)

            ILTree::Statement *AssignmentStatement = AllocateStatement(SL_STMT, SyncLockLocation, NoResume);
            AssignmentStatement->AsStatementWithExpression().Operand1 =
                CaptureInLongLivedTemporary(
                    TypeHelpers::IsGenericParameter(LockedObject->ResultType) ?
                        AllocateExpression(
                            SX_CTYPE,
                            GetFXSymbolProvider()->GetObjectType(),
                            LockedObject,
                            LockedObject->Loc) :
                        LockedObject,
                    CapturedObject,
                    m_BlockContext);

            if (!m_IsGeneratingXML && 
                LockedObject->ResultType == GetFXSymbolProvider()->GetObjectType() &&
                m_Compiler->GetProjectBeingCompiled() && // This may be null while prettylisting on the UI thread
                m_Compiler->GetProjectBeingCompiled()->GetVBRuntimeKind() != EmbeddedRuntime)
            {
                // SyncLock on value types is not allowed, but at runtime a boxed value type may
                // be contained within an object.  Therefore, generate a runtime check that will
                // throw an exception if the object contains a value type.

                ILTree::Statement *CheckForValueTypeStatement = AllocateStatement(SL_STMT, SyncLockLocation, NoResume);
                CheckForValueTypeStatement->AsStatementWithExpression().Operand1 =
                    AllocateExpression(
                        SX_SYNCLOCK_CHECK,
                        TypeHelpers::GetVoidType(),
                        ReferToSymbol(
                            SyncLockLocation,
                            CapturedObject,
                            chType_NONE,
                            NULL,
                            NULL,
                            ExprForceRValue),
                        SyncLockLocation);
            }

            BCSYM_NamedRoot* pEnter_ObjectByRefBoolean = m_CompilerHost->GetSymbolForRuntimeMember(MonitorEnter_ObjectByRefBoolean, m_CompilationCaches);

            // Dev10 #578762 If we have new Enter method, declare and init pLockTakenVar temporary, but do not take the lock.
            // The lock should be taken inside Try block, the caller is resposible to do this 
            if (pEnter_ObjectByRefBoolean != NULL)
            {
                AssertIfFalse(pEnter_ObjectByRefBoolean->IsProc());
                pLockTakenVar = InitSyncLockTakenVar(SyncLockLocation, NoResume);
                AssertIfNull(pLockTakenVar);
            }
            else
            {
                // Call "System.Threading.Monitor.Enter" on the object.
                // 


                CallMonitorMethod(
                    SyncLockLocation,
                    NoResume,
                    CapturedObject,
                    NULL,   // No generic binding context for CapturedObject because it is a local variable
                    STRING_CONST(m_Compiler, SyncLockEnter));
            }
        }
        else
        {
            ReportSemanticError(
                ERRID_SyncLockRequiresReferenceType1,
                OperandLocation,  // Use true Operand location for reporting an error (correct?)
                LockedObject->ResultType);
        }
    }

    return CapturedObject;
}


// Dev10 #578762 Create and initialize LockTaken variable
Variable *
Semantics::InitSyncLockTakenVar
(
    _In_ const Location & syncLockLocation,
    const unsigned short resumeIndex
)
{
    Variable *pCapturedObject = NULL;

    ILTree::Statement *pAssignmentStatement = AllocateStatement(SL_STMT, syncLockLocation, resumeIndex);
    pAssignmentStatement->AsStatementWithExpression().Operand1 =
        CaptureInLongLivedTemporary(
                ProduceConstantExpression(
                    COMPLUS_FALSE,
                    syncLockLocation,
                    GetFXSymbolProvider()->GetBooleanType()
                    IDE_ARG(0)),
            pCapturedObject,
            m_BlockContext);

    return pCapturedObject;
}


// Dev10 #578762 Call "System.Threading.Monitor.Enter" with two arguments, the object and ByRef LockTaken flag.
void
Semantics::CallMonitorEnter_ObjectByRefBoolean
(
    const Location &OperandLocation,
    Variable *pLockedObject,
    GenericBinding *pGenericBindingContextForVar,
    Variable * pLockTakenVar
)
{
    AssertIfFalse(pLockTakenVar && pLockTakenVar->IsLocal());

    BCSYM_NamedRoot * pMember = m_CompilerHost->GetSymbolForRuntimeMember(MonitorEnter_ObjectByRefBoolean, m_CompilationCaches );
    AssertIfNull(pMember);
    Procedure* pEnter_ObjectByRefBoolean = ViewAsProcedure(pMember);

    ILTree::Expression *pObjectArgExpression =
        AllocateExpression(
            SX_ARG,
            TypeHelpers::GetVoidType(),
            ReferToSymbol(
                OperandLocation,
                pLockedObject,
                chType_NONE,
                NULL, // BaseReference
                pGenericBindingContextForVar,
                ExprForceRValue),
            OperandLocation);

    ILTree::Expression *pLockTakenArgExpression =
        AllocateExpression(
            SX_ARG,
            TypeHelpers::GetVoidType(),
            ReferToSymbol(
                OperandLocation,
                pLockTakenVar,
                chType_NONE,
                NULL, // BaseReference
                NULL, //GenericBindingContext
                ExprNoFlags),
            OperandLocation);

    ILTree::Expression *pCallExpression =
        InterpretCallExpressionWithNoCopyout(
            OperandLocation,
            AllocateSymbolReference(
                pEnter_ObjectByRefBoolean,
                TypeHelpers::GetVoidType(),
                NULL,
                OperandLocation),
            chType_NONE,
            AllocateExpression(
                SX_LIST,
                TypeHelpers::GetVoidType(),
                pObjectArgExpression,
                AllocateExpression(
                    SX_LIST,
                    TypeHelpers::GetVoidType(),
                    pLockTakenArgExpression,
                    NULL,
                    OperandLocation),
                OperandLocation),
            false,
            ExprNoFlags,
            NULL);

    SetFlag32(pCallExpression->AsCallExpression().Right, SXF_LIST_SUPPRESS_CLONE);
    ILTree::Statement *pCallStatement = AllocateStatement(SL_STMT, OperandLocation, NoResume);
    pCallStatement->AsStatementWithExpression().Operand1 = pCallExpression;
}


void
Semantics::TerminateSyncLockStatement
(
    Variable *CapturedObject,
    const Location &EndSyncLocation,
    _Out_opt_ ILTree::TryBlock *TryStatementForSynclock,
    _In_ Variable * pLockTakenVar
)
{
    ILTree::FinallyBlock *Finally = AllocateFinallyStatement(TryStatementForSynclock, EndSyncLocation, NoResume);
    EstablishBlockContext(Finally);

    if (CapturedObject)
    {
        // Only a local variable is currently expected here.
        //
        // This assumption is important because if ever a member variable ever needs to be
        // passed to this function, then a corresponding binding would need to be passed in too.
        //
        VSASSERT(CapturedObject->IsLocal(), "Non-local unexpected!!!");

        // Dev10 #578762 
        // If we have LockTaken var, we should call Monitor.Exit only if LockTaken contains True
        if (pLockTakenVar)
        {
            VSASSERT(pLockTakenVar->IsLocal(), "Non-local unexpected!!!");

            //             If (LockTaken) Then

            ILTree::ExecutableBlock *pIf = &AllocateStatement(SB_IF_BLOCK, EndSyncLocation, NoResume)->AsExecutableBlock();
            EstablishBlockContext(pIf);
            pIf->EndConstructLocation = EndSyncLocation;

            ILTree::IfBlock *pGuard = &AllocateStatement(SB_IF, EndSyncLocation, NoResume)->AsIfBlock();
            EstablishBlockContext(pGuard);
            pGuard->Conditional =
                        ReferToSymbol(
                            EndSyncLocation,
                            pLockTakenVar,
                            chType_NONE,
                            NULL, // BaseReference
                            NULL, // GenericBindingContext
                            ExprForceRValue);
        }
        
        // Call "System.Threading.Monitor.Exit" on the object.
        // 


        CallMonitorMethod(
            EndSyncLocation,
            NoResume,
            CapturedObject,
            NULL,   // No generic binding context for CapturedObject because it is a local variable.
            STRING_CONST(m_Compiler, SyncLockExit));

        // Dev10 #578762 
        // If we have LockTaken var, we should call Monitor.Exit only if LockTaken contains True.
        // Let's close the 'IF'
        if (pLockTakenVar)
        {

            //             End If

            PopBlockContext();
            PopBlockContext();
        }
    }

    PopBlockContext();
}

void
Semantics::CallMonitorMethod
(
    const Location &OperandLocation,
    const unsigned short ResumeIndex,
    Variable *LockedObject,
    GenericBinding *GenericBindingContextForVar,
    _In_z_ Identifier *MethodName
)
{
    Namespace *SystemThreading = m_Compiler->FindNamespace(STRING_CONST(m_Compiler, SystemThreading));

    if (SystemThreading == NULL)
    {
        ReportuntimeHelperNotFoundError(
            OperandLocation,
            STRING_CONST(m_Compiler, SystemThreading));
        return;
    }

    bool NameIsBad = false;

    Declaration *Monitor =
        LookupInNamespace(
            SystemThreading,
            STRING_CONST(m_Compiler, SyncLockMonitor),
            NameNoFlags,
            BINDSPACE_Type,
            true,
            true,
            OperandLocation,
            NameIsBad,
            NULL,
            -1);

    if (NameIsBad)
    {
        return;
    }

    if (Monitor == NULL ||
        !TypeHelpers::IsClassType(ChaseThroughAlias(Monitor)))
    {
        ReportuntimeHelperNotFoundError(
            OperandLocation,
            STRING_CONST(m_Compiler, SystemThreadingMonitor));
        return;
    }

    Declaration *Method =
        ImmediateLookup(
            ViewAsScope(ChaseThroughAlias(Monitor)->PContainer()),
            MethodName,
            BINDSPACE_Normal);

    if (Method == NULL ||
        !ChaseThroughAlias(Method)->IsProc())
    {
        ReportuntimeHelperNotFoundError(
            OperandLocation,
            ConcatNameSpaceAndName(
                m_Compiler,
                STRING_CONST(m_Compiler, SystemThreadingMonitor),
                MethodName));
        return;
    }

    ILTree::Expression *ArgExpression =
        AllocateExpression(
            SX_ARG,
            TypeHelpers::GetVoidType(),
            ReferToSymbol(
                OperandLocation,
                LockedObject,
                chType_NONE,
                NULL,
                GenericBindingContextForVar,
                ExprForceRValue),
            OperandLocation);

    ILTree::Expression *CallExpression =
        InterpretCallExpressionWithNoCopyout(
            OperandLocation,
            AllocateSymbolReference(
                Method,
                TypeHelpers::GetVoidType(),
                NULL,
                OperandLocation),
            chType_NONE,
            AllocateExpression(
                SX_LIST,
                TypeHelpers::GetVoidType(),
                ArgExpression,
                NULL,
                OperandLocation),
            false,
            ExprNoFlags,
            NULL);

    SetFlag32(CallExpression->AsCallExpression().Right, SXF_LIST_SUPPRESS_CLONE);
    ILTree::Statement *CallStatement = AllocateStatement(SL_STMT, OperandLocation, ResumeIndex);
    CallStatement->AsStatementWithExpression().Operand1 = CallExpression;
}

static const BILOP
ExpectedEnclosingOpcode [] =
{
    SB_PROC,  //  EXIT_UNKNOWN
    SB_LOOP,  //  EXIT_DO
    SB_FOR,   //  EXIT_FOR
    SB_PROC,  //  EXIT_SUB
    SB_PROC,  //  EXIT_FUNCTION
    SB_PROC,  //  EXIT_PROPERTY
    SB_TRY,   //  EXIT_TRY
    SB_SELECT, //  EXIT_SELECT
    SB_STATEMENT_LAMBDA, // EXIT_SUB_LAMBDA
    SB_STATEMENT_LAMBDA // EXIT_FUNCTION_LAMBDA
};

ILTree::Statement *
NearestEnclosing
(
    _In_ ILTree::Statement *Enclosed,
    BILOP OpcodeToMatch,
    bool ExitWhile,
    _Out_ bool &VisitsTry,
    _Out_ bool &VisitsCatch,
    _Out_ bool &VisitsFinally,
    _Out_opt_ bool *VisitsSynclock
)
{
    VisitsTry = false;
    VisitsCatch = false;
    VisitsFinally = false;

    for (ILTree::Statement *Enclosing = Enclosed;
         Enclosing;
         Enclosing = Enclosing->Parent)
    {
        if (Enclosing->bilop == SB_TRY)
        {
            VisitsTry = true;
        }

        if (Enclosing->bilop == SB_CATCH)
        {
            VisitsCatch = true;
        }

        if (Enclosing->bilop == SB_FINALLY)
        {
            VisitsFinally = true;
        }

        if (Enclosing->bilop == SB_SYNCLOCK && VisitsSynclock != NULL)
        {
            *VisitsSynclock = true;
        }

        if ((Enclosing->bilop == OpcodeToMatch &&
             // Both Exit Do and Exit While match SB_LOOP.
             (ExitWhile == HasFlag32(Enclosing, SBF_WHILE))) ||
            (OpcodeToMatch == SB_FOR && Enclosing->bilop == SB_FOR_EACH) ||
            (OpcodeToMatch == SB_TRY && Enclosing->bilop == SB_CATCH))
        {
            // Skip synthetic Try blocks; they are not valid for Exit Try statements because the user didn't write them.
            // 
            if (OpcodeToMatch == SB_TRY && Enclosing->bilop == SB_TRY && !HasFlag32(Enclosing, SBF_TRY_NOT_SYNTHETIC))
            {
                continue;
            }
            return Enclosing;
        }
    }

    return NULL;
}

void
Semantics::InterpretExitStatement
(
    _In_ ParseTree::Statement *Exit
)
{
    // Exit statements divide into two kinds -- those that exit methods and
    // those that exit statements.  The kinds that exit statements use SL_EXIT nodes.
    // The kinds that exit methods use SL_RETURN nodes.

    bool ExitIsBad = false;
    EXITKIND ExitKind = EXIT_UNKNOWN;

    switch (Exit->Opcode)
    {
        case ParseTree::Statement::ExitDo:
        case ParseTree::Statement::ExitWhile:
            ExitKind = EXIT_DO;
            break;
        case ParseTree::Statement::ExitFor:
            ExitKind = EXIT_FOR;
            break;
        case ParseTree::Statement::ExitSub:
            if (m_StatementLambdaInterpreter)
            {
                if (m_InLambda != MultiLineSubLambda)
                {
                    ExitIsBad = true;
                }
                ExitKind = EXIT_SUB_LAMBDA;
            }
            else
            {
                if (m_Procedure == NULL || !IsSub(m_Procedure))
                {
                    ExitIsBad = true;
                }
                ExitKind = EXIT_SUB;
            }
            break;
        case ParseTree::Statement::ExitFunction:
            if (m_StatementLambdaInterpreter)
            {
                if (m_InLambda != MultiLineFunctionLambda)
                {
                    ExitIsBad = true;
                }
                ExitKind = EXIT_FUNCTION_LAMBDA;
            }
            else
            {
                if (m_Procedure == NULL || !IsFunction(m_Procedure))
                {
                    ExitIsBad = true;
                }
                ExitKind = EXIT_FUNCTION;
            }
            break;
        case ParseTree::Statement::ExitProperty:
            if (m_Procedure == NULL || !IsPropertyMethod(m_Procedure))
            {
                ExitIsBad = true;
            }
            ExitKind = EXIT_PROPERTY;
            break;
        case ParseTree::Statement::ExitTry:
            ExitKind = EXIT_TRY;
            break;
        case ParseTree::Statement::ExitSelect:
            ExitKind = EXIT_SELECT;
            break;
        default:
            VSFAIL("Surprising Exit opcode.");
    }


    bool ExitsTry = false;
    bool ExitsCatch = false;
    bool ExitsFinally = false;
    ILTree::Statement *ExitedStatement =
        NearestEnclosing(
            m_BlockContext,
            ExpectedEnclosingOpcode[ExitKind],
            Exit->Opcode == ParseTree::Statement::ExitWhile,
            ExitsTry,
            ExitsCatch,
            ExitsFinally);

    ILTree::Statement *Result;

    if (Exit->Opcode == ParseTree::Statement::ExitSub ||
        Exit->Opcode == ParseTree::Statement::ExitFunction ||
        Exit->Opcode == ParseTree::Statement::ExitProperty)
    {
        Result = AllocateStatement(SL_RETURN, Exit);
        SetFlag32(Result,SLF_RET_FROM_EXIT);
    }
    else
    {
        Result = AllocateStatement(SL_EXIT, Exit);
        Result->AsExitStatement().exitkind = ExitKind;
        Result->AsExitStatement().ExitedStatement = ExitedStatement;
    }

    if (ExitIsBad || ExitedStatement == NULL)
    {
        unsigned ErrorId = 0;
        switch (Exit->Opcode)
        {
            case ParseTree::Statement::ExitDo:
                ErrorId = ERRID_ExitDoNotWithinDo;
                break;
            case ParseTree::Statement::ExitFor:
                ErrorId = ERRID_ExitForNotWithinFor;
                break;
            case ParseTree::Statement::ExitTry:
                ErrorId = ERRID_ExitTryNotWithinTry;
                break;
            case ParseTree::Statement::ExitWhile:
                ErrorId = ERRID_ExitWhileNotWithinWhile;
                break;
            case ParseTree::Statement::ExitSelect:
                ErrorId = ERRID_ExitSelectNotWithinSelect;
                break;
            case ParseTree::Statement::ExitSub:
                ErrorId = ERRID_ExitSubOfFunc;
                break;
            case ParseTree::Statement::ExitFunction:
                ErrorId = ERRID_ExitFuncOfSub;
                break;
            case ParseTree::Statement::ExitProperty:
                ErrorId = ERRID_ExitPropNot;
                break;
            default:
                VSFAIL("Surprising Exit opcode.");
        }

        ReportSemanticError(ErrorId, Exit->TextSpan);
    }

    else if (ExitsFinally)
    {
        ReportSemanticError(ERRID_BranchOutOfFinally, Exit->TextSpan);
    }

    if (ExitsTry)
    {
        SetFlag32(Result, SLF_EXITS_TRY);
    }

    if (ExitsCatch)
    {
        SetFlag32(Result, SLF_EXITS_CATCH);
    }
}

void
Semantics::InterpretReturn
(
    _Inout_ ParseTree::ExpressionStatement *Return
)
{

    // functionContext here means: "should I expect the Return statement to have an operand?"
    bool functionContext = m_StatementLambdaInterpreter ? 
                        m_InLambda == MultiLineFunctionLambda :
                        m_Procedure && (IsFunction(m_Procedure) || IsPropertyGet(m_Procedure));

    if (GetLocalResumableKind() == ILTree::IteratorResumable ||
        GetLocalResumableKind() == ILTree::IterableResumable)
    {
        functionContext = false;
    }
    else if (GetLocalResumableKind() == ILTree::TaskResumable)
    {
        Type* ReturnType = GetTypeForLocalReturnStatements();
        functionContext = (ReturnType==NULL || !ReturnType->IsVoidType());
        // If ReturnType is NULL, it means that for some reason the ResumableKind analysis
        // couldn't find anything out. e.g. we're in a lambda who is being interpreted just 
        // for its return types. So it's appropriate to interpret return statement operands
        // in that context.
    }

    if (!functionContext)
    {
        if (Return->Operand != NULL)
        {
            if (m_Procedure != NULL &&
                m_Procedure->CreatedByEventDecl())
            {
                VSASSERT(!m_Procedure->IsSyntheticMethod(), "Wrong code generation for Synthetic event method ?");
                ReportSemanticError(
                    ERRID_ReturnFromEventMethod,
                    Return->TextSpan);
            }
            else if (GetLocalResumableKind() == ILTree::IteratorResumable || GetLocalResumableKind() == ILTree::IterableResumable)
            {
                ReportSemanticError(ERRID_BadReturnValueInIterator, Return->TextSpan);
            }
            else if (GetLocalResumableKind() == ILTree::TaskResumable)
            {
                // We'll only fall in here if it's a non-generic Task resumable
                ReportSemanticError(ERRID_ReturnFromNonGenericTaskAsync, Return->TextSpan);
            }
            else
            {
                ReportSemanticError(ERRID_ReturnFromNonFunction, Return->TextSpan);
            }
            return;
        }
    }
    else
    {
        if (Return->Operand == NULL)
        {
            ReportSemanticError(
                ERRID_ReturnWithoutValue,
                Return->TextSpan);

            return;
        }
    }


    // Create the Return statement.
    ILTree::ReturnStatement *Ret = &AllocateStatement(SL_RETURN, Return)->AsReturnStatement();

    if (Return->Operand != NULL)
    {
        Type *ReturnType = NULL;
        Type *OriginalReturnType = NULL;

        ReturnType = GetTypeForLocalReturnStatements();

        // Dev11#173636: in some cases, if there was an error doing InterpretStatementOperand,
        // then we want to suppress that error and instead provide our own...
        TemporaryErrorTable temporary_error_table(m_Compiler, &m_Errors);
        if (m_Errors)
        {
            temporary_error_table.AddTemporaryErrorTable(new ErrorTable(*m_Errors));
            temporary_error_table.EnableMergeOnRestore(0);
            m_Errors = temporary_error_table.NewErrorTable(0);
        }
        ///////////////////

        ILTree::Expression *Source =
            InterpretStatementOperand(Return->Operand, ExprForceRValue, ReturnType, &OriginalReturnType);

        ///////////////////
        if (IsBad(Source) && ReturnType != NULL && OriginalReturnType!=NULL &&  !OriginalReturnType->IsBad() &&
            m_Errors!=NULL && m_Errors->GetErrorCount()>=1 &&
            GetLocalResumableKind()==ILTree::TaskResumable)
        {
            // If we were in an async method that returned Task<T> for some T,
            // and the return expression can't be converted to T but is identical to Task<T>,
            // then don't give the normal error message about "there is no conversion from T to Task<T>"
            // and instead say "Since this is async, the return expression must be 'T' rather than 'Task<T>'."

            Type *DeclaredReturnType = GetLocalReturnType(); // i.e. the return type of the method
            Type *GenericTask = GetFXSymbolProvider()->IsTypeAvailable(FX::GenericTaskType) ? GetFXSymbolProvider()->GetType(FX::GenericTaskType) : NULL;

            Procedure * OperatorMethod = NULL;
            GenericBinding *OperatorMethodGenericContext = NULL;
            bool OperatorMethodIsLifted;
            ConversionClass Conversion = ClassifyConversion(ReturnType, OriginalReturnType, OperatorMethod, OperatorMethodGenericContext, OperatorMethodIsLifted);

            if (DeclaredReturnType != NULL && !DeclaredReturnType->IsBad() &&
                DeclaredReturnType->IsGenericBinding() && DeclaredReturnType->PGenericBinding()->GetGeneric() == GenericTask &&
                Conversion == ConversionError && 
                BCSYM::AreTypesEqual(OriginalReturnType, DeclaredReturnType))
            {
                temporary_error_table.SuppressMergeOnRestore();
                temporary_error_table.Restore();
                ReportSemanticError(ERRID_BadAsyncReturnOperand1, Return->Operand->TextSpan, ReturnType);
            }
        }


        if (IsBad(Source))
        {
            return;
        }

        Ret->ReturnExpression = Source;

        if (m_StatementLambdaInterpreter)
        {
            // Dev10#524269: inside a lambda, for each return statement, we accumulate a log of how much
            // relaxation was involved in the return statement
            MethodConversionClass ReturnTypeConversion = MethodConversionIdentity;
            ClassifyReturnTypeForMethodConversion(OriginalReturnType,  ReturnType, ReturnTypeConversion);
            m_StatementLambdaInterpreter->m_ReturnRelaxationLevel =
                     max(DetermineDelegateRelaxationLevel(ReturnTypeConversion),
                         m_StatementLambdaInterpreter->m_ReturnRelaxationLevel);

            // Dev11#94373: also need to track whether we encountered any return statements with operands
            m_StatementLambdaInterpreter->m_SeenReturnWithOperand = true;
        }

    }

    bool ExitsTry = false;
    bool ExitsCatch = false;
    bool ExitsFinally = false;

    BILOP opCode = m_StatementLambdaInterpreter ? SB_STATEMENT_LAMBDA : SB_PROC;
    
    ILTree::Statement *ExitedStatement =
        NearestEnclosing(
            m_BlockContext,
            opCode,
            false,
            ExitsTry,
            ExitsCatch,
            ExitsFinally);

    if (ExitsTry)
    {
        SetFlag32(Ret, SLF_EXITS_TRY);
    }

    if (ExitsCatch)
    {
        SetFlag32(Ret, SLF_EXITS_CATCH);
    }

    if (ExitsFinally)
    {
        ReportSemanticError(ERRID_BranchOutOfFinally, Return->TextSpan);
    }
}



void
Semantics::InterpretAwaitStatement
(
    _Inout_ ParseTree::ExpressionStatement *AwaitStatementTree
)
{
    ThrowIfFalse(AwaitStatementTree!=NULL && AwaitStatementTree->Operand!=NULL && AwaitStatementTree->Operand->Opcode==ParseTree::Expression::Await);
    
    // The operand of an await statement is itself just the await expression.
    ILTree::Expression *AwaitExpression = InterpretExpression(AwaitStatementTree->Operand, ExprResultNotNeeded);
    if (IsBad(AwaitExpression))
    {
        MarkContainingLambdaOrMethodBodyBad();
    }
    else
    {
        ILTree::Statement *AwaitStatement = AllocateStatement(SL_STMT, AwaitStatementTree); // this links it (appends it) onto the current block
        AwaitStatement->AsStatementWithExpression().Operand1 = AwaitExpression;
    }
}

void
Semantics::InterpretYieldStatement
(
    _Inout_ ParseTree::ExpressionStatement *Yield
)
{
    if (Yield->Operand == NULL) // may happen in IDE scenarios
    {
        MarkContainingLambdaOrMethodBodyBad();
        return;
    }

    ILTree::ResumableKind resumableKind = GetLocalResumableKind();
    if (resumableKind != ILTree::UnknownResumable &&
        resumableKind != ILTree::IteratorResumable &&
        resumableKind != ILTree::IterableResumable)
    {
        VSASSERT(resumableKind != ILTree::UnknownResumable, "Oops! someone should have called IndicateResumable before interpreting the body of this lambda");
        if (resumableKind != ILTree::ErrorResumable) ReportSemanticError(ERRID_BadYieldInNonIteratorMethod, Yield->TextSpan);
        MarkContainingLambdaOrMethodBodyBad();
        return;
    }

    Type *OriginalReturnType = NULL;
    Type *ExpectedReturnType = GetTypeForLocalYieldStatements();
    ILTree::Expression *Source = InterpretStatementOperand(Yield->Operand, ExprForceRValue, ExpectedReturnType, &OriginalReturnType);
    if (IsBad(Source)) return;

    ILTree::YieldStatement *Result = &AllocateStatement(SL_YIELD, Yield)->AsYieldStatement();
    Result->YieldExpression = Source;

    if (m_StatementLambdaInterpreter)
    {
        // Inside a lambda, for each yield statement, we accumulate a log of how much
        // relaxation was involved in the yield statement.
        // This is similar to what's done with return statements.
        MethodConversionClass YieldTypeConversion = MethodConversionIdentity;
        ClassifyReturnTypeForMethodConversion(OriginalReturnType,  ExpectedReturnType, YieldTypeConversion);
        m_StatementLambdaInterpreter->m_ReturnRelaxationLevel =
                 max(DetermineDelegateRelaxationLevel(YieldTypeConversion),
                     m_StatementLambdaInterpreter->m_ReturnRelaxationLevel);
    }


    BILOP opCode = m_StatementLambdaInterpreter ? SB_STATEMENT_LAMBDA : SB_PROC;
    bool InTry=false, InCatch=false, InFinally=false;
    ILTree::Statement *EnclosingStatement = NearestEnclosing(m_BlockContext, opCode,  false, InTry, InCatch, InFinally);
    if (InTry) SetFlag32(Result, SLF_EXITS_TRY);
    if (InCatch || InFinally) ReportSemanticError(ERRID_BadYieldInTryHandler, Yield->TextSpan);
}

void
Semantics::InterpretContinueStatement
(
    _In_ ParseTree::Statement *Continue
)
{
    BILOP ExpectedContextOfContinue;

    switch (Continue->Opcode)
    {
        case ParseTree::Statement::ContinueDo:
        case ParseTree::Statement::ContinueWhile:
            ExpectedContextOfContinue = SB_LOOP;
            break;
        case ParseTree::Statement::ContinueFor:
            ExpectedContextOfContinue = SB_FOR;
            break;
        default:
            VSFAIL("Surprising Continue opcode.");
    }

    bool ExitsTry = false;
    bool ExitsCatch = false;
    bool ExitsFinally = false;

#pragma warning (push)
#pragma warning (disable:6001) // ExpectedContextOfContinue is initialized above
    ILTree::Statement *ContextOfContinue =
        NearestEnclosing(
            m_BlockContext,
            ExpectedContextOfContinue,
            Continue->Opcode == ParseTree::Statement::ContinueWhile,
            ExitsTry,
            ExitsCatch,
            ExitsFinally);
#pragma warning (pop)

    ILTree::Statement *Result = AllocateStatement(SL_CONTINUE, Continue);
    Result->AsContinueStatement().EnclosingLoopStatement = ContextOfContinue;

    if (ContextOfContinue == NULL)
    {
        unsigned ErrorId = 0;
        switch (Continue->Opcode)
        {
            case ParseTree::Statement::ContinueDo:
                ErrorId = ERRID_ContinueDoNotWithinDo;
                break;
            case ParseTree::Statement::ContinueFor:
                ErrorId = ERRID_ContinueForNotWithinFor;
                break;
            case ParseTree::Statement::ContinueWhile:
                ErrorId = ERRID_ContinueWhileNotWithinWhile;
                break;
            default:
                VSFAIL("Surprising Continue opcode.");
        }

        ReportSemanticError(ErrorId, Continue->TextSpan);
    }
    else if (ExitsFinally)
    {
        ReportSemanticError(ERRID_BranchOutOfFinally, Continue->TextSpan);
    }

    if (ExitsTry)
    {
        SetFlag32(Result, SLF_EXITS_TRY);
    }

    if (ExitsCatch)
    {
        SetFlag32(Result, SLF_EXITS_CATCH);
    }
}



void
Semantics::InterpretVariableDeclarationStatement
(
    _In_ ParseTree::VariableDeclarationStatement *VariableDeclaration
)
{
    // The symbols for the declared variables will have been created at block entry.

    // Interpret initializers.
    // For XML generation, create a tree listing the declared symbols
    // and their initializations.
    // For normal compilation, generate the initializations.

    if (VariableDeclaration->Specifiers->HasSpecifier(ParseTree::Specifier::Const))
    {
        // Const initializations are processed during declaration semantics.
        return;
    }


    ILTree::ResumableKind resumableKind = GetLocalResumableKind();
    bool isResumable = (resumableKind == ILTree::TaskResumable ||
                        resumableKind == ILTree::SubResumable ||
                        resumableKind == ILTree::IteratorResumable ||
                        resumableKind == ILTree::IterableResumable);

    if (VariableDeclaration->Specifiers->HasSpecifier(ParseTree::Specifier::Static) && isResumable)
    {
        ReportSemanticError(ERRID_BadStaticInitializerInResumable, VariableDeclaration->TextSpan);
    }


    ExpressionList *BoundDeclarations = NULL;
    ExpressionList **BoundDeclarationsTarget = m_IsGeneratingXML ? &BoundDeclarations : NULL;

    for (ParseTree::VariableDeclarationList *Declarations = VariableDeclaration->Declarations;
         Declarations;
         Declarations = Declarations->Next)
    {
        ParseTree::VariableDeclaration *Decl = Declarations->Element;

        // Can't combine an initializer with multiple declarators.

        if (Decl->Opcode == ParseTree::VariableDeclaration::WithInitializer &&
            Decl->Variables->Next)
        {
            ReportSemanticError(
                ERRID_InitWithMultipleDeclarators,
                Decl->TextSpan);
        }

        ExpressionList **BoundDeclaratorsTarget = NULL;

        if (m_IsGeneratingXML)
        {
            ExpressionList *BoundDeclaration =
                AllocateExpression(
                    SX_LIST,
                    TypeHelpers::GetVoidType(),
                    NULL,
                    NULL,
                    Declarations->TextSpan);

            *BoundDeclarationsTarget = BoundDeclaration;
            BoundDeclarationsTarget = &BoundDeclaration->AsExpressionWithChildren().Right;

            BoundDeclaratorsTarget = &BoundDeclaration->AsExpressionWithChildren().Left;
        }

        // There are 3 different kinds of initialization:
        //
        //  -- Explicit array sizes in declarators imply creating an array.
        //  -- Declaration "As New" implies creating an instance.
        //  -- Explicit initialization.

        for (ParseTree::DeclaratorList *Variables = Decl->Variables;
             Variables;
             Variables = Variables->Next)
        {
            ParseTree::Declarator *Variable = Variables->Element;

            bool noInitializer = (Decl->Opcode == ParseTree::VariableDeclaration::NoInitializer &&
                (Variable->ArrayInfo == NULL || Variable->ArrayInfo->Opcode != ParseTree::Type::ArrayWithSizes) &&
                !m_IsGeneratingXML);

            if (noInitializer && !isResumable)
            {
                continue;
                // inside resumables, we have to do a little more checking before we continue...
            }

            // VSW bug 179286. If the declared variable has the same name as the owning function 'foo' then
            // foo is still a non bad symbol. The declaration is wrongly interpreted: 'dim foo as new bar' will
            // be interpreted as 'dim foo as new foo'. The resulting assignent can go very bad.
            if ( m_Procedure && // if m_procedure is null we are in an unamed multiline lambda and we do not have to worry about this.
                 m_Procedure->IsProcedure() && // Don't consider properties
                 m_Procedure->GetRawType() && // functions provide a return type
                 !m_Procedure->IsUserDefinedOperatorMethod() && // Don't consider operators either
                 StringPool::IsEqual( m_Procedure->GetName(), Variable->Name.Name ))
            {
                // No need to report the error again. It is reported by declared.
                continue; // this var was horked - move on to the next
            }

            ParseTree::NameExpression DeclReference;
            DeclReference.Opcode = ParseTree::Expression::Name;
            DeclReference.TextSpan = Variable->TextSpan;
            DeclReference.FirstPunctuator.Line = 0;
            DeclReference.FirstPunctuator.Column = 0;
            DeclReference.Name = Variable->Name;

            ILTree::Expression *Target =
                InterpretStatementOperand(
                    &DeclReference,
                    ExprIsAssignmentTarget);

            if (IsBad(Target))
            {
                continue;
            }

            if (noInitializer)
            {
                // This is the extra work we had to do even in the noInitializer case...
                if (isResumable)
                {
                    Location *loc = (Decl->Type != NULL) ? &Decl->Type->TextSpan : &Decl->TextSpan;
                    if (IsRestrictedType(Target->ResultType, GetCompilerHost()) || IsRestrictedArrayType(Target->ResultType, GetCompilerHost()))
                    {
                        ReportSemanticError(ERRID_CannotLiftRestrictedTypeResumable1, loc, Target->ResultType);
                        MakeBad(Target);
                    }
                }
                continue;
            }

            ILTree::Expression *Initialization = NULL;

            Location InitBoundaries;

            InitBoundaries.m_lBegLine = Variable->TextSpan.m_lBegLine;
            InitBoundaries.m_lBegColumn = Variable->TextSpan.m_lBegColumn;

            // Set the End information for the Initialization.
            InitBoundaries.m_lEndLine = Decl->TextSpan.m_lEndLine;
            InitBoundaries.m_lEndColumn = Decl->TextSpan.m_lEndColumn;

            // Deal with array type specifications that include sizes.
            //
            //   a(v1, v2) as T
            //
            // is equivalent to
            //
            //   a(,) as T = new T (v1, v2) {}

            if (Variable->ArrayInfo && Variable->ArrayInfo->Opcode == ParseTree::Type::ArrayWithSizes)
            {
                // Can't combine explicit array sizes with explicit initialization.

                if (Decl->Opcode == ParseTree::VariableDeclaration::WithInitializer ||
                    Decl->Opcode == ParseTree::VariableDeclaration::WithNew)
                {
                    ReportSemanticError(
                        ERRID_InitWithExplicitArraySizes,
                        Variable->TextSpan);
                }

                ExpressionList *SizesList = NULL;

                ExpressionList **SizesListTarget = &SizesList;

                for (ParseTree::ArrayDimList *UnboundDims = Variable->ArrayInfo->AsArrayWithSizes()->Dims;
                     UnboundDims;
                     UnboundDims = UnboundDims->Next)
                {
                    ILTree::Expression *Size =
                        InterpretStatementOperand(UnboundDims->Element->upperBound,
                        ExprScalarValueFlags,
                        GetFXSymbolProvider()->GetIntegerType(),
                        NULL);

                    if (!IsBad(Size))
                    {
                        if (Size->bilop == SX_CNS_INT)
                        {
                            Size->AsIntegralConstantExpression().Value++;
                            if (Size->AsIntegralConstantExpression().Value < 0)
                            {
                                ReportSemanticError(
                                    ERRID_NegativeArraySize,
                                    Size->Loc);
                            }
                        }
                        else
                        {
                            ILTree::Expression *One =
                                ProduceConstantExpression(
                                    1,
                                    Size->Loc,
                                    GetFXSymbolProvider()->GetIntegerType()
                                    IDE_ARG(0));

                            Size =
                                AllocateExpression(
                                    SX_ADD,
                                    GetFXSymbolProvider()->GetIntegerType(),
                                    Size,
                                    One,
                                    Size->Loc);
                        }
                    }

                    ExpressionList *ListElement =
                        AllocateExpression(
                            SX_LIST,
                            TypeHelpers::GetVoidType(),
                            Size,
                            NULL,
                            Size->Loc);

                    *SizesListTarget = ListElement;
                    SizesListTarget = &ListElement->AsExpressionWithChildren().Right;
                }

                Initialization =
                    AllocateExpression(
                        SX_ASG,
                        TypeHelpers::GetVoidType(),
                        Target,
                        AllocateExpression(
                            SX_NEW_ARRAY,
                            Target->ResultType,
                            SizesList,
                            Variable->TextSpan),
                        Variable->TextSpan);
            }

            // Deal with straightforward initializations.

            else if (Decl->Opcode == ParseTree::VariableDeclaration::WithInitializer)
            {
                Initialization =
                    InterpretInitializer(Decl->AsInitializer()->InitialValue, Target);
            }

            // Deal with "As New" declarations.
            //
            //   v as new T(1,2,3)
            //   v as new T(1,2,3) With {.Id = 1, .Name = "A"}
            //
            // is equivalent to
            //
            //   v as T = new T(1,2,3)
            //   v as T = new T(1,2,3) With {.Id = 1, .Name = "A"}

            else if (Decl->Opcode == ParseTree::VariableDeclaration::WithNew)
            {
                Initialization =
                    InitializeConstructedVariable
                    (
                        Target,
                        Decl->AsNew()->Arguments.Values,
                        Decl->Type ? &Decl->Type->TextSpan : NULL,
                        Decl->AsNew()->ObjectInitializer,
                        Decl->AsNew()->CollectionInitializer,
                        Decl->AsNew()->CollectionInitializer ? & Decl->AsNew()->From : NULL,
                        &Decl->TextSpan
                    );
            }

            else if (Target)
            {
                VSASSERT(m_IsGeneratingXML, "Accidently generated a spurious symbol reference.");
            }

            if (isResumable)
            {
                Location *loc = (Decl->Type != NULL) ? &Decl->Type->TextSpan : &Decl->TextSpan;
                if (IsRestrictedType(Target->ResultType, GetCompilerHost()) || IsRestrictedArrayType(Target->ResultType, GetCompilerHost()))
                {
                    ReportSemanticError(ERRID_CannotLiftRestrictedTypeResumable1, loc, Target->ResultType);
                    MakeBad(Target);
                }
            }

            if (m_IsGeneratingXML)
            {
                // Set the location of the initializer to span the constructor call
                if (Decl->Opcode == ParseTree::VariableDeclaration::WithNew &&
                    Initialization &&
                    !IsBad(Initialization) &&
                    Initialization->AsBinaryExpression().Right)
                {
                    ParseTree::NewVariableDeclaration *NewDecl = Decl->AsNew();

                    Location loc;
                    loc.m_lBegLine = NewDecl->TextSpan.m_lBegLine + NewDecl->New.Line;
                    loc.m_lBegColumn = NewDecl->New.Column;
                    loc.m_lEndLine = NewDecl->TextSpan.m_lEndLine;
                    loc.m_lEndColumn = NewDecl->TextSpan.m_lEndColumn;
                    Initialization->AsBinaryExpression().Right->Loc = loc;
                }

                ExpressionList *BoundDeclarator =
                    AllocateExpression(
                        SX_LIST,
                        TypeHelpers::GetVoidType(),
                        Initialization ? Initialization : Target,
                        NULL,
                        Variables->TextSpan);

                *BoundDeclaratorsTarget = BoundDeclarator;
                BoundDeclaratorsTarget = &BoundDeclarator->AsExpressionWithChildren().Right;
            }

            else if (Initialization)
            {
                if (!VariableDeclaration->Specifiers->HasSpecifier(
                        ParseTree::Specifier::Static) || m_CreatedInParseTreeService)
                {
                    ILTree::Statement *InitStatement = AllocateStatement(SL_STMT, InitBoundaries, Variable->ResumeIndex);
                    InitStatement->AsStatementWithExpression().Operand1 = Initialization;
                }
                else
                {
                    // 




                    // Initialization of a static local occurs the first time
                    // control reaches the declaration. To guarantee this,
                    // some code to guard the execution of the initializer
                    // is necessary. For a static local named "Var", this
                    // guard code utilizes a flag named Var$Init of type
                    //
                    // Class Microsoft.VisualBasic.CompilerServices.StaticLocalInitFlag
                    //     Public State As Short
                    // End Class
                    //
                    // and is of the form:
                    //
                    //     Monitor.Enter(Var$Init)
                    //     Try
                    //         If (Var$Init.State = 0) Then
                    //             Var$Init.State = 2
                    //             Var = Initialization
                    //         Else If (Var$Init.State = 2) Then
                    //             Throw New Microsoft.VisualBasic.CompilerServices.IncompleteInitialization()
                    //         End If
                    //     Finally
                    //         Var$Init.State = 1
                    //         Monitor.Leave(Var$Init)
                    //     End Try
                    // End If

                    bool VariableNameIsBad = false;
                    GenericBinding *GenericBindingContext = NULL;

                    // We've already interpreted the name of this variable at the
                    // start of this loop.  If this variable is obsolete, re-running
                    // InterpretName will result in a two obsolete error messages
                    // instead of just one.
                    BackupValue<bool> obsoleteCheck(&m_PerformObsoleteChecks);
                    m_PerformObsoleteChecks = false;
                    Declaration *BackingField =
                        EnsureNamedRoot
                        (
                            InterpretName
                            (
                                Variable->Name.Name,
                                m_Lookup,
                                NULL,   // No Type parameter lookup
                                NameSearchImmediateOnly | NameSearchIgnoreExtensionMethods,
                                ContainingClass(),
                                Variable->TextSpan,
                                VariableNameIsBad,
                                &GenericBindingContext,
                                -1
                            )
                        );
                    obsoleteCheck.Restore();

                    if (VariableNameIsBad ||
                        BackingField == NULL ||
                        TypeHelpers::IsBadType(GetDataType(BackingField->PVariable())))
                    {
                        MarkContainingLambdaOrMethodBodyBad();
                        continue;
                    }

                    ::Variable *BackingFieldInitFlag =
                        BackingField->PStaticLocalBackingField()->GetInitFlag();

                    if (BackingFieldInitFlag == NULL ||
                        IsBad(BackingFieldInitFlag) ||
                        TypeHelpers::IsBadType(GetDataType(BackingFieldInitFlag)))
                    {
                        MarkContainingLambdaOrMethodBodyBad();
                        continue;
                    }

                    // The synthesized code has the same location as the real statement,
                    // because the debugger needs to see the start of the synthesized
                    // code as the start of the statement. Otherwise, stepping around
                    // the statement leaves the init state locked.

                    Type *InitFlagType = GetFXSymbolProvider()->GetShortType();

                    // 
#if 0
                    // If (Var$Init.State <> 1) Then
                    // The If block acts as the resume point, so this node gets a valid resume index.
                    ILTree::ExecutableBlock *If = &AllocateStatement(SB_IF_BLOCK, InitBoundaries, Variable->ResumeIndex)->AsExecutableBlock();
                    EstablishBlockContext(If);
                    If->EndConstructLocation = InitBoundaries;
                    // There is no resume necessary on the End If since it doesn't actually exist in the code.
                    If->TerminatingResumeIndex = NoResume;

                    ILTree::IfBlock *Guard = &AllocateStatement(SB_IF, InitBoundaries, NoResume)->AsIfBlock();
                    EstablishBlockContext(Guard);
                    Guard->Conditional =
                        AllocateExpression(
                            SX_NE,
                            GetFXSymbolProvider()->GetBooleanType(),
                            InterpretQualifiedExpression(
                                ReferToSymbol(
                                    Variable->TextSpan,
                                    BackingFieldInitFlag,
                                    chType_NONE,
                                    NULL,
                                    GenericBindingContext,
                                    ExprForceRValue),
                                State,
                                NULL,
                                false,
                                Variable->TextSpan,
                                ExprForceRValue),
                            ProduceConstantExpression(
                                1,
                                InitBoundaries,
                                InitFlagType
                                IDE_ARG(0)),
                            InitBoundaries);
#endif

                    //     Monitor.Enter(Var$Init)
                    //     Try
                    //
                    // Or due to Dev10 #578762 
                    //
                    //     lockTaken = False
                    //     Try
                    //          Monitor.Enter(Var$Init, lockTaken)

                    ::Variable *SyncObject = BackingFieldInitFlag->PVariable();
                    BCSYM_NamedRoot* pEnter_ObjectByRefBoolean = m_CompilerHost->GetSymbolForRuntimeMember(MonitorEnter_ObjectByRefBoolean, m_CompilationCaches );
                    ::Variable * pLockTakenVar = NULL;

                    // Dev10 #578762 If we have new Enter method, declare and init pLockTakenVar temporary, but do not take the lock yet.
                    // The lock should be taken inside Try block.
                    if (pEnter_ObjectByRefBoolean != NULL)
                    {
                        AssertIfFalse(pEnter_ObjectByRefBoolean->IsProc());
                        
                        pLockTakenVar = InitSyncLockTakenVar(InitBoundaries, Variable->ResumeIndex);
                        AssertIfNull(pLockTakenVar);
                    }
                    else
                    {
                        CallMonitorMethod(
                            InitBoundaries,
                            Variable->ResumeIndex,
                            SyncObject,
                            GenericBindingContext,
                            STRING_CONST(m_Compiler, SyncLockEnter));
                    }

                    ILTree::ExecutableBlock *SyncLockTry = &AllocateStatement(SB_TRY, InitBoundaries, NoResume)->AsExecutableBlock();
                    EstablishBlockContext(SyncLockTry);
                    SyncLockTry->EndConstructLocation = InitBoundaries;

                    // Dev10 #578762 If we have new Enter method, take the lock now.
                    if (pEnter_ObjectByRefBoolean != NULL)
                    {
                        CallMonitorEnter_ObjectByRefBoolean
                        (
                            InitBoundaries,
                            SyncObject,
                            GenericBindingContext,
                            pLockTakenVar
                        );
                    }

                    //             If (Var$Init.State = 0) Then

                    ILTree::ExecutableBlock *If = &AllocateStatement(SB_IF_BLOCK, InitBoundaries, NoResume)->AsExecutableBlock();
                    EstablishBlockContext(If);
                    If->EndConstructLocation = InitBoundaries;

                    ILTree::IfBlock *Guard = &AllocateStatement(SB_IF, InitBoundaries, NoResume)->AsIfBlock();
                    EstablishBlockContext(Guard);
                    Guard->Conditional =
                        AllocateExpression(
                            SX_EQ,
                            GetFXSymbolProvider()->GetBooleanType(),
                            InterpretQualifiedExpression(
                                ReferToSymbol(
                                    Variable->TextSpan,
                                    BackingFieldInitFlag,
                                    chType_NONE,
                                    NULL,
                                    GenericBindingContext,
                                    ExprForceRValue),
                                STRING_CONST(m_Compiler, State),
                                ParseTree::Expression::DotQualified,
                                Variable->TextSpan,
                                ExprForceRValue),
                            ProduceConstantExpression(
                                0,
                                InitBoundaries,
                                InitFlagType
                                IDE_ARG(0)),
                            InitBoundaries);

                    //                 Var$Init.State = 2

                    ILTree::Statement *InitFlagAssign = AllocateStatement(SL_STMT, InitBoundaries, NoResume);
                    InitFlagAssign->AsStatementWithExpression().Operand1 =
                        AllocateExpression(
                            SX_ASG,
                            TypeHelpers::GetVoidType(),
                            InterpretQualifiedExpression(
                                ReferToSymbol(
                                    Variable->TextSpan,
                                    BackingFieldInitFlag,
                                    chType_NONE,
                                    NULL,
                                    GenericBindingContext,
                                    ExprForceRValue),
                                STRING_CONST(m_Compiler, State),
                                ParseTree::Expression::DotQualified,
                                Variable->TextSpan,
                                ExprForceRValue),
                            ProduceConstantExpression(
                                2,
                                InitBoundaries,
                                InitFlagType
                                IDE_ARG(0)),
                            InitBoundaries);

                    //                 Var = Initialization

                    ILTree::Statement *InitStatement = AllocateStatement(SL_STMT, InitBoundaries, NoResume);
                    InitStatement->AsStatementWithExpression().Operand1 = Initialization;

                    //             Else If (Var$Init.State = 2) Then

                    PopBlockContext();

                    Guard = &AllocateStatement(SB_ELSE_IF, InitBoundaries, NoResume)->AsIfBlock();
                    EstablishBlockContext(Guard);
                    Guard->Conditional =
                        AllocateExpression(
                            SX_EQ,
                            GetFXSymbolProvider()->GetBooleanType(),
                            InterpretQualifiedExpression(
                                ReferToSymbol(
                                    Variable->TextSpan,
                                    BackingFieldInitFlag,
                                    chType_NONE,
                                    NULL,
                                    GenericBindingContext,
                                    ExprForceRValue),
                                STRING_CONST(m_Compiler, State),
                                ParseTree::Expression::DotQualified,
                                Variable->TextSpan,
                                ExprForceRValue),
                            ProduceConstantExpression(
                                2,
                                InitBoundaries,
                                InitFlagType
                                IDE_ARG(0)),
                            InitBoundaries);

                    //                 Throw New Microsoft.VisualBasic.CompilerServices.IncompleteInitialization()

                    ClassOrRecordType *IncompleteInitialization =
                        FindHelperClass(
                            STRING_CONST(m_Compiler, IncompleteInitialization),
                            MicrosoftVisualBasicCompilerServicesNamespace,
                            Variable->TextSpan);

                    if (IncompleteInitialization)
                    {
                        ILTree::Expression *ThrownObject =
                            CreateConstructedInstance(
                                IncompleteInitialization,
                                Variable->TextSpan,
                                Variable->TextSpan,
                                NULL,
                                ExprNoFlags);

                        if (!IsBad(ThrownObject))
                        {
                            if (!GetFXSymbolProvider()->IsTypeAvailable(FX::ExceptionType) ||
                                !IsOrInheritsFrom(
                                    ThrownObject->ResultType,
                                    GetFXSymbolProvider()->GetType(FX::ExceptionType)))
                            {
                                ReportSemanticError(
                                    ERRID_CantThrowNonException,
                                    Variable->TextSpan);
                            }
                            else
                            {
                                ILTree::Statement *Throw = AllocateStatement(SL_ERROR, InitBoundaries, NoResume);
                                Throw->AsStatementWithExpression().Operand1 = ThrownObject;
                            }
                        }
                    }

                    //             End If

                    PopBlockContext();
                    PopBlockContext();

                    //     Finally
                    //         Var$Init.State = 1
                    //         Monitor.Exit(Var$Init)
                    //     End Try
                    //
                    // Or due to Dev10 #578762 
                    //
                    //     Finally
                    //         Var$Init.State = 1
                    //         If lockTaken
                    //              Monitor.Exit(Var$Init)
                    //         End If         
                    //     End Try
                    

                    PopBlockContext();

                    ILTree::FinallyBlock *Finally = AllocateFinallyStatement(&SyncLockTry->AsTryBlock(), InitBoundaries, NoResume);
                    EstablishBlockContext(Finally);

                    InitFlagAssign = AllocateStatement(SL_STMT, InitBoundaries, NoResume);
                    InitFlagAssign->AsStatementWithExpression().Operand1 =
                        AllocateExpression(
                            SX_ASG,
                            TypeHelpers::GetVoidType(),
                            InterpretQualifiedExpression(
                                ReferToSymbol(
                                    Variable->TextSpan,
                                    BackingFieldInitFlag,
                                    chType_NONE,
                                    NULL,
                                    GenericBindingContext,
                                    ExprForceRValue),
                                STRING_CONST(m_Compiler, State),
                                ParseTree::Expression::DotQualified,
                                Variable->TextSpan,
                                ExprForceRValue),
                            ProduceConstantExpression(
                                1,
                                InitBoundaries,
                                InitFlagType
                                IDE_ARG(0)),
                            InitBoundaries);

                    // Dev10 #578762 If we have new Enter method, we should call Monitor.Exit only if LockTaken contains True
                    if (pEnter_ObjectByRefBoolean != NULL)
                    {
                        VSASSERT(pLockTakenVar && pLockTakenVar->IsLocal(), "Non-local unexpected!!!");

                        //             If (LockTaken) Then

                        ILTree::ExecutableBlock *pIf = &AllocateStatement(SB_IF_BLOCK, InitBoundaries, NoResume)->AsExecutableBlock();
                        EstablishBlockContext(pIf);
                        pIf->EndConstructLocation = InitBoundaries;

                        ILTree::IfBlock *pGuard = &AllocateStatement(SB_IF, InitBoundaries, NoResume)->AsIfBlock();
                        EstablishBlockContext(pGuard);
                        pGuard->Conditional =
                                    ReferToSymbol(
                                        InitBoundaries,
                                        pLockTakenVar,
                                        chType_NONE,
                                        NULL, // BaseReference
                                        NULL, // GenericBindingContext
                                        ExprForceRValue);
                    }
        
                    CallMonitorMethod(
                        InitBoundaries,
                        NoResume,
                        SyncObject,
                        GenericBindingContext,
                        STRING_CONST(m_Compiler, SyncLockExit));

                    // Dev10 #578762 If we have new Enter method, we should call Monitor.Exit only if LockTaken contains True.
                    // Let's close the 'IF'
                    if (pEnter_ObjectByRefBoolean != NULL)
                    {

                        //             End If

                        PopBlockContext();
                        PopBlockContext();
                    }

                    PopBlockContext();

                    //
#if 0
                    // End If

                    PopBlockContext();
                    PopBlockContext();
#endif
                }
            }
        }
    }

    if (m_IsGeneratingXML)
    {
        ILTree::Statement *Result = AllocateStatement(SL_VAR_DECL, VariableDeclaration);
        Result->AsStatementWithExpression().Operand1 = BoundDeclarations;
    }
}

void
Semantics::InterpretErrorStatement
(
    _In_ ParseTree::ExpressionStatement *Error
)
{
    ILTree::Statement *Result = AllocateStatement(SL_ERROR, Error);

    ILTree::Expression *Operand = InterpretStatementOperand(Error->Operand, ExprForceRValue);

    if (!IsBad(Operand))
    {
        Operand = ConvertWithErrorChecking(Operand, GetFXSymbolProvider()->GetIntegerType(), ExprNoFlags);

        Result->AsStatementWithExpression().Operand1 = Operand;
    }
}

void
Semantics::InterpretThrowStatement
(
    _In_ ParseTree::ExpressionStatement *Error
)
{
    ILTree::Statement *Result = AllocateStatement(SL_ERROR, Error);

    if (Error->Operand)
    {
        ILTree::Expression *Operand = InterpretStatementOperand(Error->Operand, ExprForceRValue);

        if (!IsBad(Operand) &&
            //keep the check for NULL for prefix (see VSW344179)
            (Operand->ResultType == NULL || !GetFXSymbolProvider()->IsTypeAvailable(FX::ExceptionType) || 
             !IsOrInheritsFrom(Operand->ResultType, GetFXSymbolProvider()->GetType(FX::ExceptionType))))
        {
            ReportSemanticError(
                ERRID_CantThrowNonException,
                Error->TextSpan);
        }
        else
        {
            Operand =
                (Operand->ResultType && TypeHelpers::IsGenericParameter(Operand->ResultType)) ?
                    AllocateExpression(
                        SX_CTYPE,
                        GetFXSymbolProvider()->GetObjectType(),
                        Operand,
                        Operand->Loc) :
                    Operand;
        }

        Result->AsStatementWithExpression().Operand1 = Operand;
    }
    else
    {
        bool ExitsTry = false;
        bool ExitsCatch = false;
        bool ExitsFinally = false;

        NearestEnclosing(
            m_BlockContext,
            SB_CATCH,
            false,
            ExitsTry,
            ExitsCatch,
            ExitsFinally);

        if (!ExitsCatch || ExitsFinally)
        {
            ReportSemanticError(
                ERRID_MustBeInCatchToRethrow,
                Error->TextSpan);
        }
    }
}

void
Semantics::InterpretRedimStatement
(
    _In_ ParseTree::RedimStatement *Redim
)
{
    unsigned short ResumeIndex = Redim->ResumeIndex;

    for (ParseTree::ExpressionList *Redims = Redim->Redims; Redims; Redims = Redims->Next)
    {
        ParseTree::Expression *FullExpression = Redims->Element;

        if (FullExpression->Opcode == ParseTree::Expression::CallOrIndex)
        {
            ParseTree::Expression *ArrayVariable = FullExpression->AsCallOrIndex()->Target;

            ILTree::Expression *RedimVariable = InterpretStatementOperand(ArrayVariable, ExprIsAssignmentTarget | ExprPropagatePropertyReference);

            bool SomeSizesBad;
            unsigned SizesCount;
            ExpressionList *NewSizes =
                InterpretArrayIndices(
                    FullExpression->AsCallOrIndex()->Arguments.Values,
                    true,
                    SizesCount,
                    SomeSizesBad);

            if (IsBad(RedimVariable) || SomeSizesBad)
            {
                MarkContainingLambdaOrMethodBodyBad();
                continue;
            }

            if (!RedimVariable->ResultType ||
                (!TypeHelpers::IsArrayType(RedimVariable->ResultType) &&
                 !RedimVariable->ResultType->IsObject()))
            {
                ReportSemanticError(
                    ERRID_ExpectedArray1,
                    ArrayVariable->TextSpan,
                    ParseTree::Statement::Redim);

                continue;
            }

            ILTree::Expression *OperandAsTarget = NULL;
            Variable *OperandTemporary = NULL;

            if (RedimVariable->bilop != SX_SYM)
            {
                // The code generator requires that the operand to ReDim be
                // a simple symbol reference. If the operand here is anything else,
                // capture it to a temporary (if Preserve is in effect),
                // redim the temporary, and assign the temporary back to the original
                // operand.

                if (Redim->HasPreserve)
                {
                    ILTree::Expression *OperandAsSource = NULL;
                    UseTwiceShortLived(RedimVariable, OperandAsSource, OperandAsTarget);

                    OperandAsSource = MakeRValue(OperandAsSource);

                    if (IsBad(OperandAsSource))
                    {
                        continue;
                    }

                    ILTree::Statement *OperandCapture = AllocateStatement(SL_STMT, OperandAsSource->Loc, ResumeIndex++);
                    OperandCapture->AsStatementWithExpression().Operand1 =
                        CaptureInShortLivedTemporary(
                            OperandAsSource,
                            OperandTemporary);
                }
                else
                {
                    OperandAsTarget = RedimVariable;
                    OperandTemporary = AllocateShortLivedTemporary(RedimVariable->ResultType);
                }

                RedimVariable =
                    ReferToSymbol(
                        OperandAsTarget->Loc,
                        OperandTemporary,
                        chType_NONE,
                        NULL,
                        NULL,   // No generic binding context for locals
                        ExprNoFlags);
            }

            ILTree::RedimStatement *Result = &AllocateStatement(SL_REDIM, FullExpression->TextSpan, ResumeIndex++)->AsRedimStatement();

            if (Redim->HasPreserve)
            {
                SetFlag32(Result, SLF_REDIM_PRESERVE);
            }

            if (SizesCount == 0)
            {
                ReportSemanticError(
                    ERRID_RedimNoSizes,
                    FullExpression->TextSpan);
            }
            else if (TypeHelpers::IsArrayType(RedimVariable->ResultType) &&
                     RedimVariable->ResultType->PArrayType()->GetRank() != SizesCount)
            {
                ReportSemanticError(
                    ERRID_RedimRankMismatch,
                    FullExpression->TextSpan);
            }

            if (RedimVariable->ResultType->IsObject())
            {
                // Synthesize the array type for code generation.
                SetResultType(
                    RedimVariable,
                    m_SymbolCreator.GetArrayType(SizesCount, RedimVariable->ResultType));
            }

            if (SizesCount > ArrayRankLimit)
            {
                ReportSemanticError(ERRID_ArrayRankLimit, Redim->TextSpan);
            }

            Result->uDims = SizesCount;
            Result->ptyp = TypeHelpers::GetVoidType();
            Result->vtype = t_void;
            Result->Operand1 = RedimVariable;
            Result->Operand2 = NewSizes;

            if (OperandAsTarget)
            {
                // Assign the operand temporary to the original operand reference.

                ILTree::Statement *CopyBack = AllocateStatement(SL_STMT, OperandAsTarget->Loc, ResumeIndex++);
                CopyBack->AsStatementWithExpression().Operand1 =
                    GenerateAssignment(
                        OperandAsTarget->Loc,
                        OperandAsTarget,
                        ReferToSymbol(
                            OperandAsTarget->Loc,
                            OperandTemporary,
                            chType_NONE,
                            NULL,
                            NULL,
                            ExprNoFlags),
                        false);
            }
        }
        else if (FullExpression->Opcode != ParseTree::Expression::SyntaxError)
        {
            ReportSemanticError(
                ERRID_RedimNoSizes,
                FullExpression->TextSpan);
        }
    }
}

void
Semantics::InterpretEraseStatement
(
    _In_ ParseTree::EraseStatement *Erase
)
{
    // "Erase a1, a2" is equivalent to "a1 = Nothing : a2 = Nothing".

    unsigned short ResumeIndex = Erase->ResumeIndex;

    for (ParseTree::ExpressionList *Arrays = Erase->Arrays;
         Arrays;
         Arrays = Arrays->Next)
    {
        ParseTree::Expression *UnboundArray = Arrays->Element;

        ILTree::Expression *Array =
            InterpretStatementOperand(
                UnboundArray,
                ExprIsAssignmentTarget | ExprPropagatePropertyReference);

        if (!Array->ResultType || (!IsBad(Array) &&
            !TypeHelpers::IsArrayType(Array->ResultType) &&
            !TypeHelpers::IsRootArrayType(Array->ResultType, m_CompilerHost) &&
            !Array->ResultType->IsObject()))
        {
            ReportSemanticError(
                ERRID_ExpectedArray1,
                UnboundArray->TextSpan,
                ParseTree::Statement::Erase);

            MakeBad(Array);
            continue;
        }

        ParseTree::Expression *Nothing = new(m_TreeStorage) ParseTree::Expression;
        Nothing->Opcode = ParseTree::Expression::Nothing;
        Nothing->TextSpan = UnboundArray->TextSpan;

        InterpretAssignment(Array, Nothing, UnboundArray->TextSpan, ResumeIndex++);
    }
}

void
Semantics::InterpretAssignMidStatement
(
    _In_ ParseTree::AssignMidStatement *AssignMid
)
{
    // Transform
    //
    //      Mid(Target, Start, Length) = Source"
    //
    // into
    //
    //      Microsoft.VisualBasic.CompilerServices.StringType.MidStmtStr(Target, Start, Length, Source)

    ClassOrRecordType *StringType =
        FindHelperClass(
            STRING_CONST(m_Compiler, StringType),
            MicrosoftVisualBasicCompilerServicesNamespace,
            AssignMid->TextSpan);

    if (StringType == NULL)
    {
        return;
    }

    Procedure *Method =
        FindHelperMethod(STRING_CONST(m_Compiler, AssignMid), StringType, AssignMid->TextSpan);

    if (Method == NULL)
    {
        return;
    }

    VerifyTypeCharacterConsistency(
        AssignMid->TextSpan,
        GetFXSymbolProvider()->GetStringType(),
        AssignMid->TypeCharacter);

    ExpressionFlags ArgumentFlags = ExprPropagatePropertyReference;

    ILTree::Expression *Target = InterpretStatementOperand(AssignMid->Target, ArgumentFlags | ExprIsAssignmentTarget);
    ILTree::Expression *Start = InterpretStatementOperand(AssignMid->Start, ArgumentFlags);

    ILTree::Expression *Length = NULL;
    if (AssignMid->Length)
    {
        Length = InterpretStatementOperand(AssignMid->Length, ArgumentFlags);
    }
    else
    {
        // If the length is omitted, it is implicitly the full length of the string.
        // This is encoded as the largest positive long value, which is greater than the
        // maximum length of any string on a 32-bit platform.
        Length = ProduceConstantExpression(
            0x7fffffff,
            AssignMid->TextSpan,
            GetFXSymbolProvider()->GetIntegerType()
            IDE_ARG(0));
    }

    ILTree::Expression *Source = InterpretStatementOperand(AssignMid->Source, ArgumentFlags);

    ILTree::Statement *CallStatement = AllocateStatement(SL_STMT, AssignMid);
    CallStatement->AsStatementWithExpression().Operand1 =
        InterpretCallExpressionWithNoCopyout(
            AssignMid->TextSpan,
            AllocateSymbolReference(
                Method,
                TypeHelpers::GetVoidType(),
                NULL,
                AssignMid->TextSpan),
            chType_NONE,
            AllocateExpression(
                SX_LIST,
                TypeHelpers::GetVoidType(),
                AllocateExpression(
                    SX_ARG,
                    TypeHelpers::GetVoidType(),
                    Target,
                    Target->Loc),
                AllocateExpression(
                    SX_LIST,
                    TypeHelpers::GetVoidType(),
                    AllocateExpression(
                        SX_ARG,
                        TypeHelpers::GetVoidType(),
                        Start,
                        Start->Loc),
                    AllocateExpression(
                        SX_LIST,
                        TypeHelpers::GetVoidType(),
                        AllocateExpression(
                            SX_ARG,
                            TypeHelpers::GetVoidType(),
                            Length,
                            Length->Loc),
                        AllocateExpression(
                            SX_LIST,
                            TypeHelpers::GetVoidType(),
                            AllocateExpression(
                                SX_ARG,
                                TypeHelpers::GetVoidType(),
                                Source,
                                Source->Loc),
                            NULL,
                            Source->Loc),
                        Length->Loc),
                    Start->Loc),
                Target->Loc),
            false,
            ExprNoFlags,
            NULL);
}

void
Semantics::InterpretRaiseEventStatement
(
    _In_ ParseTree::RaiseEventStatement *RaiseEvent
)
{
    if (RaiseEvent->Event.IsBad || IsBad(ContainingClass()))
    {
        return;
    }

    bool EventNameIsBad = false;
    GenericBinding *EventGenericBindingContext = NULL;

    Declaration *Event =
        EnsureNamedRoot
        (
            InterpretName
            (
                RaiseEvent->Event.Name,
                ViewAsScope(ContainingClass()),
                NULL,   // No Type parameter lookup
                NameSearchIgnoreParent | NameSearchEventReference | NameSearchIgnoreExtensionMethods,
                ContainingClass(),
                RaiseEvent->Event.TextSpan,
                EventNameIsBad,
                &EventGenericBindingContext,
                -1
            )
        );

    if (EventNameIsBad)
    {
        MarkContainingLambdaOrMethodBodyBad();
        return;
    }

    if (Event == NULL || !IsEvent(Event))
    {
        ReportSemanticError(
            ERRID_NameNotEvent2,
            RaiseEvent->Event.TextSpan,
            RaiseEvent->Event.Name,
            ContainingClass());

        return;
    }

    Declaration *FireMethod = NULL;
    GenericBinding *FireMethodGenericBindingContext = NULL;

    Variable *DelegateVariable = Event->PEventDecl()->GetDelegateVariable();

    if (Event->PEventDecl()->IsBlockEvent())
    {
        VSASSERT(DelegateVariable == NULL, "Delegate variable unexpected for block event!!!");
        VSASSERT(Event->PEventDecl()->GetProcFire(), "Missing fire method unexpected for block events!!!");

        FireMethod = Event->PEventDecl()->GetProcFire();

        // 










        if (FireMethod == NULL ||
            FireMethod->GetContainer() != ContainingClass())
        {
            ReportSemanticError(
                ERRID_CantRaiseBaseEvent,
                RaiseEvent->Event.TextSpan);

            return;
        }

        FireMethodGenericBindingContext = EventGenericBindingContext;
    }
    else
    {
        if (DelegateVariable == NULL ||
            !IsAccessible(DelegateVariable, EventGenericBindingContext, ContainingClass()))
        {
            ReportSemanticError(
                ERRID_CantRaiseBaseEvent,
                RaiseEvent->Event.TextSpan);

            return;
        }

        // Raising an event actually calls the event delegate's
        // Invoke method.
        Type *EventDelegate = Event->PEventDecl()->GetDelegate();

        if (EventDelegate == NULL)
        {
            // Something is bad somewhere.
            return;
        }

        // Reinterpret the evetn's delegate type binding within the binding context of the event
        if (EventDelegate->IsGenericTypeBinding() && EventGenericBindingContext)
        {
            EventDelegate = ReplaceGenericParametersWithArguments(EventDelegate, EventGenericBindingContext, m_SymbolCreator);
        }

        bool InvokeIsBad = false;

        Declaration *Invoke =
            EnsureNamedRoot
            (
                InterpretName
                (
                    STRING_CONST(m_Compiler, DelegateInvoke),
                    ViewAsScope(EventDelegate->PClass()),
                    NULL,   // No Type parameter lookup
                    NameSearchIgnoreParent | NameSearchIgnoreExtensionMethods ,
                    ContainingClass(),
                    RaiseEvent->Event.TextSpan,
                    InvokeIsBad,
                    NULL,   // binding context is determined later below
                    -1
                )
            );

        if ((Invoke == NULL || !IsProcedure(Invoke)) &&
            !InvokeIsBad)
        {
            ReportSemanticError(
                ERRID_DelegateNoInvoke1,
                RaiseEvent->Event.TextSpan,
                EventDelegate);

            InvokeIsBad = true;
        }

        if (InvokeIsBad)
        {
            MarkContainingLambdaOrMethodBodyBad();
            return;
        }

        FireMethod = Invoke;

        FireMethodGenericBindingContext =
            DeriveGenericBindingForMemberReference(EventDelegate, Invoke, m_SymbolCreator, m_CompilerHost);
    }

    VSASSERT(FireMethod && !IsBad(FireMethod), "Bad event fire method unexpected!!!");

    ILTree::Expression *MeReference = NULL;

    if (!Event->PEventDecl()->IsShared())
    {
        MeReference =
            SynthesizeMeReference(
                RaiseEvent->Event.TextSpan,
                ContainingClass(),
                false);

        if (IsBad(MeReference))
        {
            MarkContainingLambdaOrMethodBodyBad();
            return;
        }
    }

    if (FireMethod == m_Procedure &&
          ( ViewAsProcedure(FireMethod)->IsShared() ||
            MeReference == 0 ||
            ( MeReference->bilop == SX_SYM &&
              MeReference->AsSymbolReferenceExpression().Symbol->IsVariable() &&
              MeReference->AsSymbolReferenceExpression().Symbol->PVariable()->IsMe())))
    {
        ReportSemanticError(
            WRNID_RecursiveAddHandlerCall,
            RaiseEvent->TextSpan,
            m_Compiler->TokenToString(tkRAISEEVENT),
            RaiseEvent->Event.Name);
    }

    ILTree::Expression *FireMethodBaseReference = MeReference;

    if (DelegateVariable)
    {
        FireMethodBaseReference =
            ReferToSymbol(
                RaiseEvent->Event.TextSpan,
                DelegateVariable,
                chType_NONE,
                MeReference,
                EventGenericBindingContext,
                ExprNoFlags);

        // If this is a WinRT event then we want to get the delegate from the TokenTable and generate
        // this.<delegateVar>.InvocationList.Invoke(..)
        if (Event->PEventDecl()->IsWindowsRuntimeEvent())
        {
            ParserHelper ph(&m_TreeStorage, RaiseEvent->TextSpan);

            bool GetOrCreateEventRegistrationTokenTableIsBad = false;
            Declaration* GetOrCreateEventRegistrationTokenTable = 
                EnsureNamedRoot
                (
                    InterpretName
                    (
                        m_Compiler->AddString(L"GetOrCreateEventRegistrationTokenTable"),
                        ViewAsScope(DelegateVariable->GetType()->PClass()),
                        NULL,   // No Type parameter lookup
                        NameSearchIgnoreParent | NameSearchIgnoreExtensionMethods ,
                        ContainingClass(),
                        RaiseEvent->Event.TextSpan,
                        GetOrCreateEventRegistrationTokenTableIsBad,
                        NULL,   // binding context is determined later below
                        -1
                    )
                );
            if ((GetOrCreateEventRegistrationTokenTable == NULL) || GetOrCreateEventRegistrationTokenTableIsBad)
            {
                // Missing EventRegistrationTokenTable type error should already have been reported
                MarkContainingLambdaOrMethodBodyBad();
                return;
            }

            ILTree::Expression* GetOrCreateEventRegistrationTokenTableCallTarget =
                ReferToSymbol(
                    RaiseEvent->Event.TextSpan,
                    GetOrCreateEventRegistrationTokenTable,
                    chType_NONE,
                    NULL,
                    DeriveGenericBindingForMemberReference(DelegateVariable->GetType(), GetOrCreateEventRegistrationTokenTable, m_SymbolCreator, m_CompilerHost),
                    ExprIsExplicitCallTarget);
            if (IsBad(GetOrCreateEventRegistrationTokenTableCallTarget))
            {
                MarkContainingLambdaOrMethodBodyBad();
                return;
            }

            ParseTree::ArgumentList * ArgListElement = NULL;
            ParseTree::Expression * ArgExpression = ph.CreateNameExpression(1, DelegateVariable->GetName());
            ArgListElement = new(m_TreeStorage) ParseTree::ArgumentList;
            ArgListElement->TextSpan = RaiseEvent->Event.TextSpan;
            ArgListElement->Element = new(m_TreeStorage) ParseTree::Argument;
            ArgListElement->Element->TextSpan = RaiseEvent->Event.TextSpan;
            ArgListElement->Element->Value = ArgExpression;

            ILTree::Expression* GetOrCreateEventRegistrationTokenTableCall = 
                BindArgsAndInterpretCallExpressionWithNoCopyOut(
                    RaiseEvent->TextSpan,
                    GetOrCreateEventRegistrationTokenTableCallTarget,
                    chType_NONE,
                    ArgListElement,
                    ExprNoFlags,
                    OvrldNoFlags,
                    Event);

            bool InvocationListIsBad = false;
            Declaration *InvocationList =
                EnsureNamedRoot
                (
                    InterpretName
                    (
                        STRING_CONST(m_Compiler, InvocationList),
                        ViewAsScope(DelegateVariable->GetType()->PClass()),
                        NULL,   // No Type parameter lookup
                        NameSearchIgnoreParent | NameSearchIgnoreExtensionMethods ,
                        ContainingClass(),
                        RaiseEvent->Event.TextSpan,
                        InvocationListIsBad,
                        NULL,   // binding context is determined later below
                        -1
                    )
                );

            if (InvocationListIsBad)
            {
                MarkContainingLambdaOrMethodBodyBad();
                return;
            }

            FireMethodBaseReference =
                ReferToSymbol(
                    RaiseEvent->Event.TextSpan,
                    InvocationList,
                    chType_NONE,
                    GetOrCreateEventRegistrationTokenTableCall,
                    DeriveGenericBindingForMemberReference(DelegateVariable->GetType(), InvocationList, m_SymbolCreator, m_CompilerHost),
                    ExprNoFlags);
        }

        if (IsBad(FireMethodBaseReference))
        {
            MarkContainingLambdaOrMethodBodyBad();
            return;
        }

        // Generate code to test that the delegate is non-null before
        // calling the Invoke method.

        // (Note: Microsoft - bug 34453. The use of EventVariableReference in the if first and later in the call is not thread
        // safe. Another thread could remove the last handler between the two instructions. THe fix is to take the EventVariableReference
        // into a temporary and use that temporary.
        //
        // XML doesn't like temporaries, so in XML generate code without temporary local

        ILTree::Expression *FirstEventVariableReference;
        ILTree::Expression *SecondEventVariableReference;

        if( m_IsGeneratingXML )
        {
            // set first and second use of variable to the event variable reference
            // Note (Microsoft) Not sure "UseTwiceShortLived" makes sense here; the event ref is just a name
            FirstEventVariableReference = FireMethodBaseReference;
            UseTwiceShortLived(FireMethodBaseReference, FireMethodBaseReference, SecondEventVariableReference);
        }
        else
        {
            // create temporary local, and assignment
            // set first and second reference to the local
            Variable * EventVariableTemp;
            ILTree::Statement *AssignmentStatement = AllocateStatement(SL_STMT, Location::GetHiddenLocation(), RaiseEvent->ResumeIndex);
            AssignmentStatement->AsStatementWithExpression().Operand1 =
                CaptureInShortLivedTemporary( FireMethodBaseReference, EventVariableTemp);

            FirstEventVariableReference = SecondEventVariableReference =
                AllocateSymbolReference(
                    EventVariableTemp,
                    EventVariableTemp->GetType(),
                    NULL,
                    RaiseEvent->Event.TextSpan);
        }

        ParseTree::ExecutableBlockStatement FakeBlock;
        (ParseTree::Statement &)FakeBlock = *(ParseTree::Statement *)RaiseEvent;
        FakeBlock.TerminatingConstruct = NULL;
        FakeBlock.HasProperTermination = true;
        FakeBlock.BodyTextSpan = RaiseEvent->TextSpan;
        FakeBlock.LocalsCount = 0;
        FakeBlock.ResumeIndex = NoResume;

        SetFlag32(AllocateBlock(SB_IF_BLOCK, &FakeBlock), SBF_IF_BLOCK_NO_XML | SBF_LINE_IF);
        ILTree::IfBlock *Guard = &AllocateBlock(SB_IF, &FakeBlock)->AsIfBlock();

        Guard->Conditional =
            NegateBooleanExpression(
                AllocateExpression(
                    SX_IS,
                    GetFXSymbolProvider()->GetBooleanType(),
                    FirstEventVariableReference,
                    AllocateExpression(
                        SX_NOTHING,
                        FirstEventVariableReference->ResultType,
                        FirstEventVariableReference->Loc),
                    FirstEventVariableReference->Loc));

        FireMethodBaseReference = SecondEventVariableReference;
    }

    // Call the fire/Invoke method.

    ILTree::Expression *RaiseEventCall =
        BindArgsAndInterpretCallExpressionWithNoCopyOut(
            RaiseEvent->TextSpan,
            ReferToSymbol(
                RaiseEvent->Event.TextSpan,
                FireMethod,
                chType_NONE,
                FireMethodBaseReference,
                FireMethodGenericBindingContext,
                ExprIsExplicitCallTarget),
            chType_NONE,
            RaiseEvent->Arguments.Values,
            ExprNoFlags,
            OvrldNoFlags,
            Event);
    SetFlag32(RaiseEventCall, SXF_CALL_RAISEEVENT);

    // 


    ILTree::Statement *CallStatement = AllocateStatement(SL_STMT, RaiseEvent->TextSpan, NoResume);
    CallStatement->AsStatementWithExpression().Operand1 = RaiseEventCall;

    // If context only exists if a delegate variable is present, so only pop if a
    // delegate variable is present.
    //
    if (DelegateVariable)
    {
        PopBlockContext();
        PopBlockContext();
    }
}

void
Semantics::InterpretHandlerStatement
(
    _In_ ParseTree::HandlerStatement *AddOrRemove
)
{
    // Don't infer the result type yet. Tis expression will be passed to InterpretExpression
    // where the target type will be present, so at that time it will infer if needed.
    ILTree::Expression *DelegateBinding = InterpretStatementOperand(AddOrRemove->Delegate, ExprForceRValue | ExprDontInferResultType);

    ILTree::Expression *Base = NULL;
    ILTree::Expression *OriginalBase = NULL;
    Type *BaseResultType = NULL;
    bool IsBaseExpressionNamespace = false;
    ParseTree::IdentifierDescriptor EventName;
    NameFlags Flags = 0;

    ParseTree::Expression *EventReference =
        AddOrRemove->Event->Opcode == ParseTree::Expression::Parenthesized ?
            AddOrRemove->Event->AsUnary()->Operand :
            AddOrRemove->Event;

    switch (EventReference->Opcode)
    {
        case ParseTree::Expression::DotQualified:
        {
            VSASSERT(EventReference->AsQualified()->Name != NULL, "QualifiedExpression::Name must be set!");
            if (!EventReference->AsQualified()->Name ||
                EventReference->AsQualified()->Name->Opcode != ParseTree::Expression::Name)
            {
                // Syntax error
                return;
            }

            EventName = EventReference->AsQualified()->Name->AsName()->Name;

            if (EventReference->AsQualified()->Base)
            {
                Base =
                    InterpretStatementOperand(
                        EventReference->AsQualified()->Base,
                        ExprAllowTypeReference | ExprAllowNamespaceReference);

                VSASSERT(IsBad(Base) || Base->bilop != SX_SYM || Base->AsSymbolReferenceExpression().Symbol,
                            "ILTree::Expression should be marked bad!");

                // Don't try to make namespaces RValues
                //
                OriginalBase = Base;
                if (!IsBad(Base) &&
                    (Base->bilop != SX_SYM ||
                     !IsNamespace(Base->AsSymbolReferenceExpression().Symbol)))
                {
                    Base = MakeRValue(Base);
                }

            }
            else
            {
                Base = EnclosingWithValue(EventReference->TextSpan);
            }

            if (!IsBad(Base))
            {
                BaseResultType = Base->ResultType;

                // If the base reference names a type or a namespace , then it specifies a lookup scope
                // but does not provide an object reference.

                if (Base->bilop == SX_SYM)
                {
                    if (Base->AsSymbolReferenceExpression().Symbol->IsType())
                    {
                        // Type parameters cannot be used as qualifiers
                        //
                        if (TypeHelpers::IsGenericParameter(Base->AsSymbolReferenceExpression().Symbol))
                        {
                            ReportSemanticError(
                                ERRID_TypeParamQualifierDisallowed,
                                Base->Loc);

                            return;
                        }

                        Base = NULL;
                    }
                    // For Namespaces, the lookup is the Namespace itself
                    //
                    else if (IsNamespace(Base->AsSymbolReferenceExpression().Symbol))
                    {
                        BaseResultType = Base->AsSymbolReferenceExpression().Symbol;
                        Base = NULL;
                    }
                }

                Flags = NameSearchIgnoreParent;
            }

            break;
        }

        case ParseTree::Expression::Name:
        {
            EventName = EventReference->AsName()->Name;

            // We'll synthesize base later, if needed
            BaseResultType = ContainingClass();

            break;
        }

        case ParseTree::Expression::SyntaxError:
            return;

        default:
            ReportSemanticError(
                ERRID_AddOrRemoveHandlerEvent,
                EventReference->TextSpan);

            return;
    }

    if (EventName.IsBad || (Base && IsBad(Base)) || IsBad(DelegateBinding))
    {
        return;
    }

    if (AddOrRemove->Opcode == ParseTree::Statement::RemoveHandler)
    {
        BILOP delegateOpCode = DelegateBinding->bilop;
        if (delegateOpCode == SX_LAMBDA || delegateOpCode == SX_UNBOUND_LAMBDA)
        {
            ReportSemanticError(
                WRNID_LambdaPassedToRemoveHandler,
                DelegateBinding->Loc);
        }
        else if (delegateOpCode == SX_ADDRESSOF)
        {
            SetFlag32(DelegateBinding, SXF_USED_IN_REMOVEHANDLER);
        }
    }

    Declaration *Event = NULL;
    GenericBinding *EventGenericBindingContext = NULL;

    Scope *EventLookup = NULL;
    GenericParameter *TypeParamToLookupInForEvent = NULL;

    if (TypeHelpers::IsClassOrRecordType(BaseResultType) || BaseResultType->IsInterface() || IsNamespace(BaseResultType))
    {
        EventLookup = ViewAsScope(BaseResultType->PContainer());
    }
    else if (TypeHelpers::IsGenericParameter(BaseResultType))
    {
        TypeParamToLookupInForEvent = BaseResultType->PGenericParam();
    }

    if (EventLookup || TypeParamToLookupInForEvent)
    {
        bool EventNameIsBad = false;

        Event =
            EnsureNamedRoot
            (
                InterpretName
                (
                    EventName.Name,
                    EventLookup,
                    TypeParamToLookupInForEvent,
                    Flags | NameSearchEventReference | NameSearchIgnoreExtensionMethods,
                    InstanceTypeOfReference(Base),
                    EventName.TextSpan,
                    EventNameIsBad,
                    &EventGenericBindingContext,
                    -1
                )
            );

        UpdateNameLookupGenericBindingContext(BaseResultType, &EventGenericBindingContext);


        if (EventNameIsBad)
        {
            MarkContainingLambdaOrMethodBodyBad();
            return;
        }
    }

    if (Event == NULL || !IsEvent(Event))
    {
        STRING *NameOfUnnamedNamespace = STRING_CONST(m_Compiler, EmptyString);
        if (BaseResultType->IsNamespace() && StringPool::IsEqual(BaseResultType->PNamespace()->GetName(), NameOfUnnamedNamespace))
        {
            ReportSemanticError(
                ERRID_NameNotEvent2,
                EventName.TextSpan,
                EventName.Name,
                STRING_CONST( m_Compiler, UnnamedNamespaceErrName) );
        }
        else
        {
            ReportSemanticError(
                ERRID_NameNotEvent2,
                EventName.TextSpan,
                EventName.Name,
                BaseResultType);
        }
        return;
    }

    Declaration *EventMethod =
        AddOrRemove->Opcode == ParseTree::Statement::AddHandler ?
            Event->PEventDecl()->GetProcAdd() :
            Event->PEventDecl()->GetProcRemove();

    VSASSERT(EventMethod, "NULL event method unexpected from a non-bad event!!!");

    if (!ViewAsProcedure(EventMethod)->IsShared())
    {
        bool EventBaseIsBad = false;
        if (Base == NULL)
        {
            if (OriginalBase != NULL)
            {
                // check for a default instance on base
                STRING* MyDefaultInstanceBaseName = NULL;
                bool MangleName = false;
                if ( OriginalBase->bilop == SX_SYM && OriginalBase->AsSymbolReferenceExpression().Symbol->IsClass() &&
                    (MyDefaultInstanceBaseName =
                     GetDefaultInstanceBaseNameForMyGroupMember(OriginalBase->AsSymbolReferenceExpression().Symbol->PClass(), &MangleName)))
                {
                    Base =
                        CheckForDefaultInstanceProperty(
                            EventReference->AsQualified()->Base->TextSpan,
                            OriginalBase,
                            MyDefaultInstanceBaseName,
                            ExprForceRValue | ExprAllowTypeReference,
                            MangleName);
                }
                if (Base)
                {
                    CheckAccessibility(
                        Event,
                        EventGenericBindingContext,
                        EventName.TextSpan,
                        NameSearchIgnoreParent | NameSearchEventReference,
                        InstanceTypeOfReference(Base),
                        EventBaseIsBad);
                }
            }
            if (Base == NULL)
            {
                // the base is missing, i.e. "AddHandler ev1, AddressOf foo" :
                // the event was picked up from the current class or, a base or, an outer class or, an import.
                // If it is from an import or an outer base we are getting the wrong 'me' (bugs 167315, 416909)
                // Also, make sure the base is really missing (VSW416909), i.e."AddHandler SomeType.ev1, AddressOf foo"
                // Update the base result type in this case.
                if (OriginalBase == NULL)
                {
                    Type *EventType = Event->GetContainingClassOrInterface();
                    if (!IsOrInheritsFromOrImplements(BaseResultType, EventType))
                    {
                        BaseResultType = EventType;
                    }
                }

                Base =
                    SynthesizeMeReference(
                        EventReference->TextSpan,
                        BaseResultType,
                        false);
            }
        }

        if (Base == NULL || IsBad(Base) || EventBaseIsBad)
        {
            MarkContainingLambdaOrMethodBodyBad();
            return;
        }
    }

    if (EventMethod == m_Procedure &&
          ( ViewAsProcedure(EventMethod)->IsShared() ||
            Base == 0 ||
            ( Base->bilop == SX_SYM &&
              Base->AsSymbolReferenceExpression().Symbol->IsVariable() &&
              Base->AsSymbolReferenceExpression().Symbol->PVariable()->IsMe())))
    {
        ReportSemanticError(
            WRNID_RecursiveAddHandlerCall,
            AddOrRemove->TextSpan,
            (AddOrRemove->Opcode == ParseTree::Statement::AddHandler ?
                m_Compiler->TokenToString(tkADDHANDLER):
                m_Compiler->TokenToString(tkREMOVEHANDLER)),
            EventName.Name);
    }

    ILTree::Expression *AddOrRemoveCall = NULL;
    if (!TypeHelpers::IsEmbeddableInteropType(BaseResultType))
    {
        ILTree::Expression *EventMethodReference =
                ReferToSymbol(
                    EventName.TextSpan,
                    EventMethod,
                    EventName.TypeCharacter,
                    Base,
                    EventGenericBindingContext,
                    ExprIsExplicitCallTarget);

        if (Event->PEventDecl()->IsWindowsRuntimeEvent()) 
        {
            CheckObsolete(EventMethod, EventName.TextSpan);

            if (!GetFXSymbolProvider()->IsTypeAvailable(FX::WindowsRuntimeMarshalType))
            {
                ReportMissingType(FX::WindowsRuntimeMarshalType, AddOrRemove->TextSpan);
                MarkContainingLambdaOrMethodBodyBad();
                return;
            }

            ParserHelper ph(&m_TreeStorage, AddOrRemove->TextSpan);

            // Interpret it with the target type.
            Type* TargetType = Event->PEventDecl()->GetDelegate();
            TargetType = ReplaceGenericParametersWithArguments(TargetType, EventGenericBindingContext, m_SymbolCreator);
            DelegateBinding = InterpretStatementOperand(AddOrRemove->Delegate, ExprForceRValue, TargetType, NULL);
            
            ParseTree::ArgumentList* arguments = NULL;
            ParseTree::Expression* methodNameExpression = NULL;

            // WindowsRuntimeMarshal.AddEventHandler(addMethod As Func(Of T, EventRegistrationToken) , removeMethod As Action(Of EventRegistrationToken) , handler As T)
            // or
            // WindowsRuntimeMarshal.RemoveEventHandler(removeMethod As Action(Of EventRegistrationToken) , handler As T)
            if (AddOrRemove->Opcode == ParseTree::Statement::AddHandler)
            {
                ILTree::Expression *TemporaryBase = NULL;
                UseTwiceShortLived(Base, Base, TemporaryBase);

                ILTree::Expression *EventAddMethodReference =
                                            ReferToSymbol(
                                                EventName.TextSpan,
                                                EventMethod,
                                                EventName.TypeCharacter,
                                                Base,
                                                EventGenericBindingContext,
                                                ExprIsExplicitCallTarget);

                // The add handler requires the remove method as well for indexing purposes. Create a SymbolRef for it.
                ILTree::Expression *EventRemoveMethodReference =
                                            ReferToSymbol(
                                                EventName.TextSpan,
                                                Event->PEventDecl()->GetProcRemove(),
                                                EventName.TypeCharacter,
                                                TemporaryBase,
                                                EventGenericBindingContext,
                                                ExprIsExplicitCallTarget);

                methodNameExpression = ph.CreateQualifiedExpression(
                                            ph.CreateBoundSymbol(GetFXSymbolProvider()->GetType(FX::WindowsRuntimeMarshalType)),
                                            ph.CreateNameExpression(STRING_CONST(m_Compiler, AddEventHandler),AddOrRemove->TextSpan));

                arguments = ph.CreateArgList(3,
                                             ph.CreateAddressOf(ph.CreateBoundExpression(EventAddMethodReference), false),
                                             ph.CreateAddressOf(ph.CreateBoundExpression(EventRemoveMethodReference), false),
                                             ph.CreateBoundExpression(DelegateBinding));
            }
            else
            {
                methodNameExpression = ph.CreateQualifiedExpression(
                                            ph.CreateBoundSymbol(GetFXSymbolProvider()->GetType(FX::WindowsRuntimeMarshalType)),
                                            ph.CreateNameExpression(STRING_CONST(m_Compiler, RemoveEventHandler),AddOrRemove->TextSpan));

                arguments = ph.CreateArgList(2,
                                             ph.CreateAddressOf(ph.CreateBoundExpression(EventMethodReference), false),
                                             ph.CreateBoundExpression(DelegateBinding));
            }

            AddOrRemoveCall = 
                    InterpretExpression(
                        ph.CreateMethodCall(methodNameExpression, arguments),
                        ExprIsExplicitCallTarget);
        }
        else
        {
            AddOrRemoveCall =
                InterpretCallExpressionWithNoCopyout(
                    AddOrRemove->TextSpan,
                    EventMethodReference,
                    chType_NONE,
                        AllocateExpression(
                            SX_LIST,
                            TypeHelpers::GetVoidType(),
                            AllocateExpression(
                                SX_ARG,
                                TypeHelpers::GetVoidType(),
                                DelegateBinding,
                                DelegateBinding->Loc),
                            NULL,
                            DelegateBinding->Loc),
                        false,
                        ExprNoFlags,
                        NULL);
        }
    }
    else
    {
        AddOrRemoveCall = ApplyComEventInterfaceBinder(
            Event, 
            EventName, 
            AddOrRemove,
            EventGenericBindingContext,
            Base,
            DelegateBinding);
    }
    AssertIfNull(AddOrRemoveCall);
    ILTree::Statement *CallStatement = AllocateStatement(SL_STMT, AddOrRemove);
    CallStatement->AsStatementWithExpression().Operand1 = AddOrRemoveCall;
}


ILTree::Expression*
Semantics::ApplyComEventInterfaceBinder
(
    Declaration *Event,
    ParseTree::IdentifierDescriptor &EventName,
    ParseTree::HandlerStatement *AddOrRemove,
    GenericBinding *EventGenericBindingContext,
    ILTree::Expression *Base,
    ILTree::Expression *DelegateBinding
 )
{
    // Events on NoPIA interfaces are handled through a special binder class called
    // ComAwareEventInfo. We expect that the event's container will have a
    // ComEventInterface attribute which points us to an event source interface.
    // We will create a new instance of ComAwareEventInfo, passing in the type of this
    // interface and the name of the event, then call its AddEventHandler method with
    // the base expression and the delegate binding expression. 
    STRING *eventNameStr = Event->PEventDecl()->GetName();
    BCSYM_Container *pContainer = Event->PEventDecl()->GetContainer();
    AssertIfNull(pContainer);
    
    // The code we are about to generate will not contain any direct references to the event itself,
    // but the com event binder needs the event to exist on the local type. We'll poke the pia reference
    // cache directly so that PEBuilder will make sure to include the event.
    m_Project->CachePiaTypeMemberRef(Event, &EventName.TextSpan, m_Errors);
    
    // Using the container and eventNameStr values, we can create an instance of ComAwareEventInfo.
    // The call expression will look like this:
    // New System.Runtime.InteropServices.ComAwareEventInfo(GetType(sourceInterface), "eventName")
    STRING *helperName[4];
    m_Compiler->SplitQualifiedName(L"System.Runtime.InteropServices.ComAwareEventInfo", 4, helperName);
    bool NameIsBad = false;
    Type *helperClass =
            Semantics::InterpretQualifiedName(
                helperName,
                4,
                NULL,
                NULL,
                m_Compiler->GetUnnamedNamespace(m_Project)->GetHash(),
                NameSearchIgnoreImports,
                EventName.TextSpan,
                NULL,
                m_Compiler,
                m_CompilerHost,
                m_CompilationCaches,
                NULL,
                true,
                NameIsBad);
    if (NameIsBad || !helperClass)
    {
        ReportSemanticError(ERRID_NoPiaEventsMissingSystemCore, EventName.TextSpan);
        return AllocateBadExpression(EventName.TextSpan);
    }

    if (!GetFXSymbolProvider()->IsTypeAvailable(FX::TypeType))
    {
        ReportMissingType(FX::TypeType, EventName.TextSpan);
        return AllocateBadExpression(EventName.TextSpan);
    }

    AssertIfNull(helperClass);
    AssertIfTrue(NameIsBad);
    AssertIfFalse(helperClass->IsContainer());
    AssertIfFalse(helperClass->IsClass());
    ILTree::Expression *ifxTypeArg = 
        AllocateExpression(
            SX_ARG,
            TypeHelpers::GetVoidType(),
            AllocateExpression(
                SX_METATYPE,
                GetFXSymbolProvider()->GetTypeType(),
                AllocateExpression(
                    SX_NOTHING,
                    pContainer,
                    EventName.TextSpan),
                EventName.TextSpan),
            EventName.TextSpan);
    ILTree::Expression *eventNameArg = 
        AllocateExpression(
            SX_ARG, 
            TypeHelpers::GetVoidType(), 
            ProduceStringConstantExpression(
                eventNameStr,
                wcslen(eventNameStr),
                EventName.TextSpan
                IDE_ARG(0)), 
            NULL, 
            EventName.TextSpan);
    ILTree::Expression *constructorArgList =
        AllocateExpression(
            SX_LIST, 
            TypeHelpers::GetVoidType(), 
            ifxTypeArg,
            AllocateExpression(
                SX_LIST, 
                TypeHelpers::GetVoidType(), 
                eventNameArg, 
                eventNameArg->Loc),
            ifxTypeArg->Loc);
    ILTree::Expression *eventInfoValue = CreateConstructedInstance(
        helperClass,            // type of instance
        EventName.TextSpan,     // span representing type name (implicit)
        EventName.TextSpan,     // span representing whole expression (also implied)
        constructorArgList,
        NameIsBad,
        ExprNoFlags);
        
    // Now that we have an instance of ComAwareEventInfo, we can bind the event itself.
    STRING *methodName = m_Compiler->AddString( 
            AddOrRemove->Opcode == ParseTree::Statement::AddHandler ?
            L"AddEventHandler" :
            L"RemoveEventHandler");
    bool InvokeIsBad = false;
    Declaration *handlerMethod =
        EnsureNamedRoot
        (
            InterpretName
            (
                methodName,
                helperClass->PClass()->GetHash(),
                NULL,
                NameSearchIgnoreParent | NameSearchIgnoreExtensionMethods,
                ContainingClass(),
                EventName.TextSpan,
                InvokeIsBad,
                NULL,
                -1
            )
        );
    if (InvokeIsBad || !handlerMethod)
    {
        ReportSemanticError(ERRID_NoPiaEventsMissingSystemCore, EventName.TextSpan);
        return AllocateBadExpression(EventName.TextSpan);
    }
    ILTree::Expression *BaseArg = 
        AllocateExpression(SX_ARG, TypeHelpers::GetVoidType(), Base, NULL, Base->Loc);        
    ILTree::Expression *DelegateArg =
        AllocateExpression(
            SX_ARG,
            TypeHelpers::GetVoidType(),
            ConvertWithErrorChecking(
                DelegateBinding, 
                Event->PEventDecl()->GetDelegate(), 
                ExprForceRValue),
            DelegateBinding->Loc);
    ILTree::Expression *helperArgList =
        AllocateExpression(
            SX_LIST,
            TypeHelpers::GetVoidType(),
            BaseArg,
            AllocateExpression(
                SX_LIST,
                TypeHelpers::GetVoidType(),
                DelegateArg,
                NULL,
                DelegateArg->Loc),
            BaseArg->Loc);
    return InterpretCallExpressionWithNoCopyout(
        AddOrRemove->TextSpan,
        ReferToSymbol(
            AddOrRemove->TextSpan,
            handlerMethod,
            chType_NONE,
            eventInfoValue,
            NULL,
            ExprIsExplicitCallTarget),
        chType_NONE,
        helperArgList,
        false,
        ExprResultNotNeeded,
        NULL);
}

/*=======================================================================================
InterpretConditionalOperand

Builds a bound tree representing the boolean expression of a conditional.
=======================================================================================*/
ILTree::Expression *
Semantics::InterpretConditionalOperand
(
    ParseTree::Expression *Conditional,
    ExpressionFlags Flags
)
{
    return
        InterpretStatementOperand(
            Conditional,
            Flags | ExprIsOperandOfConditionalBranch | ExprForceRValue,
            GetFXSymbolProvider()->GetBooleanType(),
            NULL);
}

void
Semantics::CheckChildrenForBlockLevelShadowing
(
    _In_ ILTree::ExecutableBlock *Parent
)
{
    for (ILTree::Statement *Child = Parent->Child; Child; Child = Child->Next)
    {
        if (Child->IsExecutableBlockNode())
        {
            CheckForBlockLevelShadowing(&Child->AsExecutableBlock());
        }
    }

    if (Parent->IsExecutableBlockNode())
    {
        ILTree::ExecutableBlock * ExecutableBlock = &Parent->AsExecutableBlock();

        if (!ExecutableBlock->TrackingVarsForImplicit.IsEmpty())
        {
            CSingleListIter<TrackingVariablesForImplicitCollision> listIter(&ExecutableBlock->TrackingVarsForImplicit);
            TrackingVariablesForImplicitCollision *current;
            while (current = listIter.Next())
            {
                    CheckNameForShadowingOfLocals(
                    current->Name,
                    current->Location,
                    current->ErrorId,
                    false /*DeferForImplicitDeclarations*/);
            }
        }

        while ( ExecutableBlock->TrackingLambdasForShadowing && !(ExecutableBlock->TrackingLambdasForShadowing->Count() == 0) )
        {
            ILTree::LambdaExpression *pLambdaExpression = ExecutableBlock->TrackingLambdasForShadowing->PopOrDequeue();
            BCSYM_Param *pLambdaParam = pLambdaExpression->FirstParameter;

            while (pLambdaParam)
            {
                if (!pLambdaParam->IsRelaxedDelegateVariable() && !pLambdaParam->IsQueryIterationVariable())
                {
                    // We should clean this up so that we walk up the hashes similarly to how we walk up the hashes while
                    // checking for block level shadowing of the locals.
                    BackupValue<Scope*> backupLookup(&m_Lookup);
                    m_Lookup = (pLambdaExpression->GetLocalsHash()->GetImmediateParent() && 
                                pLambdaExpression->GetLocalsHash()->GetImmediateParent()->IsHash()) ?
                                pLambdaExpression->GetLocalsHash()->GetImmediateParent()->PHash() :
                                m_Lookup;
                    
                    // We do not need to check relaxed delegate variables for name shadowing
                    CheckNameForShadowingOfLocals(
                        pLambdaParam->GetName(),
                        *(pLambdaParam->GetLocation()),
                        ERRID_LambdaParamShadowLocal1,
                        false);
                }

                pLambdaParam = pLambdaParam->GetNext();
            }

            if (pLambdaExpression->IsStatementLambda)
            {
                // If we have a field level lambda, then we want m_Lookup to be the hash of the
                // outermost lambda, which will be the first lambda we come across. The outermost 
                // lambda's hash contains the implicit variables declared within the lambda and any
                // nested lambdas; thus, it should be m_Lookup.
                BackupValue<Scope*> backup_Lookup(&m_Lookup);
                
                if (!m_Lookup->IsLocalsHash())
                {
                    m_Lookup = pLambdaExpression->GetStatementLambdaBody()->Locals;
                }
                CheckForBlockLevelShadowing(pLambdaExpression->GetStatementLambdaBody());
            }
                
        }
    }
}

void
Semantics::CheckForBlockLevelShadowing
(
    _In_ ILTree::ExecutableBlock *Child
)
{
    if (Child->Locals)
    {
        BCITER_HASH_CHILD Locals(Child->Locals);

        for (Declaration *Local = Locals.GetNext();
             Local;
             Local = Locals.GetNext())
        {
            Identifier *LocalName = Local->GetName();
            Scope *Parent = Child->Locals;

            bool shouldStop = false;

            do
            {
                Parent = GetEnclosingScope(Parent);

                BCSYM_NamedRoot *pShadowedSymbol = NULL;

                if (pShadowedSymbol = Parent->SimpleBind(LocalName))    
                {
                    if (pShadowedSymbol->IsAlias() &&
                        pShadowedSymbol->PAlias()->GetSymbol()->IsNamedRoot())
                    {
                        pShadowedSymbol = pShadowedSymbol->PAlias()->GetSymbol()->PNamedRoot();
                    }

                    if (pShadowedSymbol->IsVariable() &&
                        (pShadowedSymbol->PVariable()->GetVarkind() == VAR_Param) &&
                        pShadowedSymbol->PVariable()->IsLambdaMember())
                    {
                        ReportSemanticError(
                            ERRID_LocalNamedSameAsParamInLambda1,
                            ChaseThroughAlias(Local)->GetLocation(),
                            Local);
                    }
                    else if (pShadowedSymbol->IsVariable() &&
                        (pShadowedSymbol->PVariable()->GetVarkind() == VAR_FunctionResult))
                    {
                        ReportSemanticError(
                            ERRID_LocalSameAsFunc,
                            ChaseThroughAlias(Local)->GetLocation(),
                            Local);
                    }
                    // Microsoft:
                    // We should not enter this block if we bound to a static local backing field.
                    else if (!(pShadowedSymbol->IsVariable()) ||
                             ((pShadowedSymbol->PVariable()->GetVarkind() == VAR_Member) && !pShadowedSymbol->IsStaticLocalBackingField()))
                    {
                        // If we have come across a field, or other class level symbol, we have gone too far and should stop 
                        // checking for shadowing.
                        // This can happen when we are checking for shadowing of locals that are inside 
                        // multiline lambdas at the field level.
                        //
                        shouldStop = true;
                        break;
                    }
                    else
                    {
                        ReportSemanticError(
                            ERRID_BlockLocalShadowing1,
                            ChaseThroughAlias(Local)->GetLocation(),
                            Local);
                    }
                    break;
                }

                if (shouldStop)
                {
                    break;
                }

            } while (Parent && (Parent != m_ProcedureTree->Locals));

            // check the local for a restricted type
            // Note: a CheckRestrictedArraysInLocals(Child->Locals) would do the work as well, but it
            // is not worth an extra iteration.

            VSASSERT(Local->IsAlias() ||
                Local->PVariable()->GetVarkind() != VAR_Param, "A nested scope cannot have a param");

            if (!Local->IsAlias() && m_ReportErrors)
            {
                VSASSERT(Local->IsVariable(), "local symbols are expected to be variables");
                CheckRestrictedArrayType(
                    Local->PVariable()->GetType(),
                    Local->PVariable()->GetRawType()->GetTypeReferenceLocation(),
                    m_CompilerHost,
                    m_Errors);
            }
        }
    }

    CheckChildrenForBlockLevelShadowing(Child);
}

ILTree::Statement *
Semantics::AllocateStatement
(
    BILOP Opcode,
    const Location &TreeLocation,
    const unsigned short ResumeIndex,
    bool Link
)
{
    ILTree::Statement *Result = &m_TreeAllocator.xAllocBilNode(Opcode)->AsStatement();

    Result->Loc = TreeLocation;
    Result->ResumeIndex = ResumeIndex;
    Result->GroupId = m_statementGroupId;

    if (Link)
    {
        LinkStatement(Result);
    }

    return Result;
}

ILTree::Statement *
Semantics::AllocateStatement
(
    BILOP Opcode,
    _In_ ParseTree::Statement *UnboundStatement,
    bool link
)
{
    return AllocateStatement(Opcode, UnboundStatement->TextSpan, UnboundStatement->ResumeIndex, link);
}

ILTree::ExecutableBlock *
Semantics::AllocateBlock
(
    BILOP Opcode,
    _In_ ParseTree::ExecutableBlockStatement *UnboundBlock,
    bool link
)
{
    ILTree::ExecutableBlock *Result = &AllocateStatement(Opcode, UnboundBlock, link)->AsExecutableBlock();

    if (UnboundBlock->TerminatingConstruct)
    {
        Result->EndConstructLocation = UnboundBlock->TerminatingConstruct->TextSpan;
        Result->TerminatingResumeIndex = UnboundBlock->TerminatingConstruct->ResumeIndex;
    }
    else
    {
        Result->EndConstructLocation.Invalidate();
        Result->TerminatingResumeIndex = NoResume;  // No terminating construct, so no Resume necessary.
    }
    Result->LocalsCount = UnboundBlock->LocalsCount;

    //Result->parent = this->m_BlockContext;

    EstablishBlockContext(Result);

    return Result;
}


ILTree::ExecutableBlock *
Semantics::AllocateHiddenBlock
(
    BILOP Opcode,
    unsigned LocalsCount
)
{
    const Location &Hidden = Location::GetHiddenLocation();

    ILTree::ExecutableBlock *Result = &AllocateStatement(Opcode, Hidden, NoResume)->AsExecutableBlock();

    Result->EndConstructLocation = Hidden;
    Result->TerminatingResumeIndex = NoResume;
    Result->LocalsCount = LocalsCount;

    EstablishBlockContext(Result);

    return Result;
}

ILTree::FinallyBlock *
Semantics::AllocateFinallyStatement
(
    _Out_opt_ ILTree::TryBlock *OwningTry,
    _In_ const Location &TreeLocation,
    const unsigned short ResumeIndex
)
{
    // Some syntax error scenarios do result in the OwningTry being NULL.
    // Eg: Finally without a Try
    //
    // AssertIfNull(OwningTry);

    ILTree::FinallyBlock *Finally = &AllocateStatement(SB_FINALLY, TreeLocation, ResumeIndex)->AsFinallyBlock();

    if (OwningTry)
    {
        OwningTry->FinallyTree = Finally;
    }

    return Finally;
}

ILTree::FinallyBlock *
Semantics::AllocateFinallyBlock
(
    _Out_opt_ ILTree::TryBlock *OwningTry,
    _In_ ParseTree::ExecutableBlockStatement *UnboundBlock
)
{
    // Some syntax error scenarios do result in the OwningTry being NULL.
    // Eg: Finally without a Try
    //
    //AssertIfNull(OwningTry);

    ILTree::FinallyBlock *Finally = &AllocateBlock(SB_FINALLY, UnboundBlock)->AsFinallyBlock();

    if (OwningTry)
    {
        OwningTry->FinallyTree = Finally;
    }

    return Finally;
}

void
Semantics::LinkStatement
(
    ILTree::Statement *Statement
)
{
    ILTree::Statement &StatementRep = Statement->AsStatement();

    StatementRep.Prev = m_LastStatementInContext;
    StatementRep.Next = NULL;
    StatementRep.Parent = m_BlockContext;

    (*m_StatementTarget) = Statement;
    m_StatementTarget = &StatementRep.Next;

    m_LastStatementInContext = Statement;
}

// Establish a block statement as the current context.

void
Semantics::EstablishBlockContext
(
    _In_ ILTree::ExecutableBlock *NewContext
)
{
    m_BlockContext = NewContext;
    m_LastStatementInContext = NULL;
    m_StatementTarget = &NewContext->Child;
}

// End the current context and restore its parent as the current context.

void
Semantics::PopBlockContext
(
)
{
#if IDE    
    if (CheckStop(this)) // if the foreground thread wants to stop the bkd, then let's not recur so IDE is more responsive
    {
        return ;
    }
#endif IDE
    
    m_LastStatementInContext = m_BlockContext;
    m_StatementTarget = &m_BlockContext->Next;

    // If the block contained local declarations, a lookup table will
    // have been created for it, and the current scope will be that lookup
    // table. Make the current scope the parent of the exited block.

    if (m_BlockContext->Locals)
    {
        VSASSERT(m_Lookup == m_BlockContext->Locals, "ILTree::ExecutableBlock scope nesting confused.");

        m_Lookup = m_Lookup->GetImmediateParent()->PHash();
    }

    // We cannot assume that the current block context has a parent because the IDE does not build contexts in
    // the same order the core compiler expects.
    m_BlockContext = m_BlockContext->Parent ? &m_BlockContext->Parent->AsExecutableBlock() : NULL;
}

void
Semantics::CheckRestrictedArraysInLocals
(
    BCSYM_Hash *Locals
)
{
    if (Locals && m_ReportErrors)
    {
        BCSYM_NamedRoot *pLocal;
        BCITER_HASH_CHILD iterLocals(Locals);
        while (pLocal = iterLocals.GetNext())
        {
            if (pLocal->IsAlias() || pLocal->PVariable()->GetVarkind() == VAR_Param)
            {
                continue;   // skip params as they have different rules and are treated at declared time
            }
            VSASSERT(pLocal->IsVariable(), "local symbols are expected to be variables");
            CheckRestrictedArrayType(
                pLocal->PVariable()->GetType(),
                pLocal->PVariable()->GetRawType()->GetTypeReferenceLocation(),
                m_CompilerHost,
                m_Errors);
        } // while locals
    }
}


