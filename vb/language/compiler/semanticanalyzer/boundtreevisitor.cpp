//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

//==============================================================================
// Start BoundTreeVisitor
//==============================================================================

void BoundTreeVisitor::Visit(_In_opt_ ILTree::ProcedureBlock *body)
{
    ThrowIfNull(body);
    VisitBlock(body);
}

void BoundTreeVisitor::VisitBlock(_Inout_ ILTree::ExecutableBlock *block)
{
    if ( !StartBlock(block) )
    {
        return;
    }

    ProcessBlock(block);

    EndBlock(block);
}

void BoundTreeVisitor::ProcessBlock(_In_ ILTree::ExecutableBlock *block)
{
    switch ( block->bilop )
    {
        case SB_HIDDEN_CODE_BLOCK:
        case SB_IF_BLOCK:
        case SB_SYNCLOCK:
        case SB_USING:
        case SB_FINALLY:
        case SB_PROC:
        case SB_TRY:
        case SB_STATEMENT_LAMBDA:
            // Just need to visit the statement lists for these blocks
            break;

        case SB_IF:
        case SB_ELSE:
        case SB_ELSE_IF:
            VisitIf(&block->AsIfBlock());
            break;

        case SB_SELECT:
            AssertIfFalse(!block->AsSelectBlock().SelectorCapture ||
                          block->AsSelectBlock().SelectorCapture->IsExprNode());
            VisitExpression((ILTree::Expression **)&block->AsSelectBlock().SelectorCapture);
            break;

        case SB_CASE:
        {
#if DEBUG
            ILTree::Statement *Select = block->AsCaseBlock().Parent;
            VSASSERT(Select->bilop == SB_SELECT, "Case: owner must be a SB_SELECT.");
#endif
            VisitCase(&block->AsCaseBlock());
            break;
        }

        case SB_LOOP:
            VisitLoop(&block->AsLoopBlock());
            break;

        case SB_FOR:
            VisitFor(&block->AsForBlock());
            break;

        case SB_FOR_EACH:
            VisitForEach(&block->AsForEachBlock());
            break;

        case SB_WITH:
            VisitWith(&block->AsWithBlock());
            break;
        case SB_CATCH:
            VisitExpression(&block->AsCatchBlock().CatchVariableTree);
            VisitExpression(&block->AsCatchBlock().WhenExpression);
            // !!! WARNING !!!
            // We should also rewrite the statement-sequence block->AsCatchBlock().ExceptionStore
            // But BoundTreeVisitor class has no abstraction for rewriting a statement-sequence
            // (other than the single statement-sequence allowed per block, below).
            // So, descendents of BoundTreeVisitor should pay special attention to "ExceptionStore"
            break;
        default:
            VSFAIL("Visit bound trees: Unexpected stmtkind.");
            break;
    }

    // Visit the statements in the block
    if ( StartBlockStatements(block) )
    {
        for(ILTree::Statement *CurrentStmt = block->Child; CurrentStmt; CurrentStmt = CurrentStmt->Next)
        {
            if (IsBad(CurrentStmt))
            {
                continue;
            }

            VisitStatement(CurrentStmt);
        }

        EndBlockStatements(block);
    }
}

void BoundTreeVisitor::VisitFor ( ILTree::ForBlock *block)
{
    // If you make changes to this method, make sure to change ClosureRoot::VisitFor accordingly.
    VisitExpression(&block->ControlVariableReference);
    VisitExpression(&block->Initial);
    VisitExpression(&block->Limit);
    VisitExpression(&block->Step);
    VisitStatement(block->SignCheckStatement);
    VisitStatement(block->IncrementStatement);
    VisitExpression(&block->Condition);

    ILTree::Expression *expr;
    expr = block->LiftedLimitTemporary;
    VisitExpression(&expr);
    if (expr != NULL)
    {
        ThrowIfFalse(expr->bilop == SX_SYM);
        block->LiftedLimitTemporary = &expr->AsSymbolReferenceExpression();
    }

    expr = block->LiftedStepTemporary;
    VisitExpression(&expr);
    if (expr != NULL)
    {
        ThrowIfFalse(expr->bilop == SX_SYM);
        block->LiftedStepTemporary = &expr->AsSymbolReferenceExpression();
    }
   
}

void BoundTreeVisitor::VisitForEach ( ILTree::ForEachBlock *block)
{
    // If you make changes to this method, make sure to change ClosureRoot::VisitForEach accordingly.
    VisitExpression(&block->ControlVaribleTempInitializer);
    VisitExpression(&block->Initializer);
    VisitExpression(&block->CurrentIndexAssignment);
    VisitExpression(&block->ControlVariableReference);
    VisitStatement(block->IncrementStatement);

    VisitExpression(&block->Conditional);

    VisitStatement(block->InitializeClosure);
}

void BoundTreeVisitor::VisitLoop ( ILTree::LoopBlock *block)
{
    // If you make changes to this method, make sure to change ClosureRoot::VisitLoop accordingly.
    VisitExpression(&block->Conditional);
}

void BoundTreeVisitor::VisitIf ( ILTree::IfBlock *block)
{
    // If you make changes to this method, make sure to change ClosureRoot::VisitIf accordingly.
    VisitExpression(&block->Conditional);
}

void BoundTreeVisitor::VisitWith ( ILTree::WithBlock *block)
{
    // If you make changes to this method, make sure to change ClosureRoot::VisitWith accordingly.
    VisitExpression(&block->WithValue);
    VisitExpression(&block->TemporaryClearAssignment);
    VisitExpression(&block->RecordReference);
}

void BoundTreeVisitor::VisitCase ( ILTree::CaseBlock *block)
{
    // If you make changes to this method, make sure to change ClosureRoot::VisitCase accordingly.
    if (block->uFlags & SBF_CASE_CONDITION)
    {
        VisitExpression(&block->Conditional);
    }
}

void BoundTreeVisitor::VisitStatement ( _Inout_ ILTree::Statement *Stmt)
{
    ILTree::Statement *CurrentStmt = Stmt;

    if (!CurrentStmt)
    {
        return;
    }

    if ( !StartStatement(Stmt))
    {
        return;
    }

    ProcessStatement(CurrentStmt);

    EndStatement(Stmt);
}

void BoundTreeVisitor::ProcessStatement(_In_ ILTree::Statement *CurrentStmt)
{
    switch (CurrentStmt->bilop)
    {
        case SB_HIDDEN_CODE_BLOCK:
        case SB_IF_BLOCK:
        case SB_IF:
        case SB_ELSE:
        case SB_ELSE_IF:
        case SB_SELECT:
        case SB_CASE:
        case SB_LOOP:
        case SB_FOR:
        case SB_FOR_EACH:
        case SB_WITH:
        case SB_TRY:
        case SB_CATCH:
        case SB_FINALLY:
        case SB_SYNCLOCK:
        case SB_USING:
        case SB_STATEMENT_LAMBDA:
            VisitBlock(&CurrentStmt->AsExecutableBlock());
            break;

        case SL_STMT:
            VisitExpression(&CurrentStmt->AsStatementWithExpression().Operand1);
            break;
        case SL_REDIM:
            VisitExpression(&CurrentStmt->AsRedimStatement().Operand1);
            VisitExpression(&CurrentStmt->AsRedimStatement().Operand2);
            break;

        case SL_ERROR:
            VisitExpression(&CurrentStmt->AsStatementWithExpression().Operand1);
            break;

        case SL_ON_ERR:
        case SL_RESUME:
        case SL_END:
        case SL_STOP:
        case SL_GOTO:
        case SL_ASYNCSWITCH:
        case SL_LABEL:
        case SL_CONTINUE:
        case SL_EXIT:
        case SL_VAR_DECL:
        case SL_DEBUGLOCNOP:
            break;

        case SL_RETURN:
            AssertIfFalse(!CurrentStmt->AsReturnStatement().ReturnExpression ||
                          CurrentStmt->AsReturnStatement().ReturnExpression->IsExprNode());
            VisitExpression((ILTree::Expression **)&CurrentStmt->AsReturnStatement().ReturnExpression);
            break;

        case SL_YIELD:
            AssertIfFalse(CurrentStmt->AsYieldStatement().YieldExpression != NULL && CurrentStmt->AsYieldStatement().YieldExpression->IsExprNode());
            VisitExpression((ILTree::Expression **)&CurrentStmt->AsYieldStatement().YieldExpression);
            break;

        default:
            VSFAIL("Visit bound trees: Unexpected stmtkind.");
    }
}

void BoundTreeVisitor::ProcessRightSkewedTree(ILTree::Expression* ExprLoc)
{
    const BILOP SeqType = ExprLoc->bilop;
    AssertIfFalse(SeqType == SX_SEQ || SeqType == SX_LIST);

    //Microsoft Process a sequence iteratively, otherwise long sequences
    //can result in deep recursion and ---- the stack.
    while (true)
    {
        //process left as normal.
        VisitExpression(&ExprLoc->AsExpressionWithChildren().Left);

        //check right to see if it is a continuation of the sequence.
        ILTree::Expression* right = ExprLoc->AsExpressionWithChildren().Right;

        if (!right || IsBad(right))
        {
            break;
        }

        if (right->bilop == SeqType)
        {
            //attempt to preserve semantics of recursive solution by 
            //breaking if StartExpression() returns false on the node.
            //Pass in the address of right to preserve semantics. Perhaps
            //StartExpression wants to change what Right points to?
            if (!StartExpression(&ExprLoc->AsExpressionWithChildren().Right))
            {
                break;
            }
            ExprLoc = ExprLoc->AsExpressionWithChildren().Right;
        }
        else
        {
            VisitExpression(&ExprLoc->AsExpressionWithChildren().Right);
            break;
        }
    }
}

void BoundTreeVisitor::VisitExpression (_Inout_opt_ ILTree::Expression **ExprTree)
{
    if (!ExprTree || !(*ExprTree))
    {
        return;
    }

    if ( !StartExpression(ExprTree) )
    {
        return;
    }

    ILTree::Expression *&Expr = *ExprTree;
    ProcessExpression(Expr);

    EndExpression(ExprTree);
}

void BoundTreeVisitor::ProcessExpression(_In_ ILTree::Expression *Expr)
{
    switch (Expr->bilop)
    {
        case SX_SYM:
            VisitExpression(&Expr->AsSymbolReferenceExpression().BaseReference);
            break;

        case SX_LAMBDA:
            if (Expr->AsLambdaExpression().IsStatementLambda)
            {
                VisitBlock(Expr->AsLambdaExpression().GetStatementLambdaBody());
            }
            else
            {
                VisitExpression(Expr->AsLambdaExpression().GetAddressOfExpressionLambdaBody());
            }
            break;

        case SX_ADR:
            VisitExpression(&Expr->AsExpressionWithChildren().Left);
            break;

        case SX_NOTHING:
        case SX_CNS_STR:
        case SX_CNS_INT:
        case SX_CNS_FLT:
        case SX_CNS_DEC:
        case SX_DEFERRED_TEMP:
            break;

        case SX_METATYPE:
            VisitExpression(&Expr->AsExpressionWithChildren().Left);
            break;

        case SX_INDEX:
            VisitExpression(&Expr->AsIndexExpression().Left);
            VisitExpression(&Expr->AsIndexExpression().Right);
            break;

        case SX_VARINDEX:
            VisitExpression(&Expr->AsVariantIndexExpression().Left);
            VisitExpression(&Expr->AsVariantIndexExpression().Right);
            break;

        case SX_ASG:
        case SX_ASG_RESADR:
            VisitExpression(&Expr->AsExpressionWithChildren().Left);
            VisitExpression(&Expr->AsExpressionWithChildren().Right);
            break;

        case SX_CTYPE:
        case SX_DIRECTCAST:
        case SX_TRYCAST:
            VisitExpression(&Expr->AsBinaryExpression().Left);
            break;

        case SX_WIDE_COERCE:
            VisitExpression(&Expr->AsExpressionWithChildren().Left);
            break;

        case SX_NEW:
            VisitExpression(&Expr->AsNewExpression().ConstructorCall);
            break;

        case SX_NEW_ARRAY:
        case SX_CREATE_ARRAY:
            VisitExpression(&Expr->AsExpressionWithChildren().Left);
            break;

        case SX_INIT_STRUCTURE:
            VisitExpression(&Expr->AsInitStructureExpression().StructureReference);
            break;

        case SX_IIF:
            VisitExpression(&Expr->AsIfExpression().condition);
            VisitExpression(&Expr->AsIfExpression().Left);
            VisitExpression(&Expr->AsIfExpression().Right);
            break;

        case SX_IIFCoalesce:
            VisitExpression(&Expr->AsCoalesceExpression().Left);
            VisitExpression(&Expr->AsCoalesceExpression().Right);
            break;

        case SX_CTYPEOP:
            VisitExpression(&Expr->AsLiftedCTypeExpression().Left);
            break;

        case SX_CALL:
            //WARNING :
            //The extension method AddImports code is dependant on the order 
            //call expressions are visted in. In particular, the code for ExtensionMethodBindingChangeDetector::StartExpression
            //assumes that if it sees an SX_CallExpression that the next expression it sees must be the target of the procedure call.
            //If you change this order, you must make appropriate changes to that code.
            VisitExpression(&Expr->AsCallExpression().Left);
            VisitExpression(&Expr->AsCallExpression().Right);
            VisitExpression(&Expr->AsCallExpression().MeArgument);
            break;

        case SX_LATE:
            VisitExpression(&Expr->AsLateBoundExpression().Left);
            VisitExpression(&Expr->AsLateBoundExpression().Right);
            VisitExpression(&Expr->AsLateBoundExpression().LateIdentifier);
            AssertIfFalse(!Expr->AsLateBoundExpression().AssignmentInfoArrayParam ||
                          Expr->AsLateBoundExpression().AssignmentInfoArrayParam->IsExprNode());
            VisitExpression((ILTree::Expression **)&Expr->AsLateBoundExpression().AssignmentInfoArrayParam);
            VisitExpression(&Expr->AsLateBoundExpression().TypeArguments);
            break;

        case SX_ISTYPE:
        case SX_POW:
        case SX_NEG:
        case SX_PLUS:
        case SX_DIV:
        case SX_MOD:
        case SX_IDIV:
        case SX_CONC:
        case SX_NOT:
        case SX_AND:
        case SX_OR:
        case SX_XOR:
        case SX_MUL:
        case SX_ADD:
        case SX_SUB:
        case SX_SHIFT_LEFT:
        case SX_SHIFT_RIGHT:
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
        case SX_IF:
        case SX_ISTRUE:
        case SX_ISFALSE:
            VisitExpression(&Expr->AsExpressionWithChildren().Left);
            VisitExpression(&Expr->AsExpressionWithChildren().Right);
            break;

        case SX_ARRAYLEN:
        case SX_SYNCLOCK_CHECK:
            VisitExpression(&Expr->AsExpressionWithChildren().Left);
            break;

            //Microsoft handle lists and sequences iteratively to conserve stack.
		case SX_SEQ:
            ProcessRightSkewedTree(Expr);
            break;
        case SX_LIST:
            ProcessRightSkewedTree(Expr);
            break;
        case SX_SEQ_OP1:
        case SX_SEQ_OP2:
            VisitExpression(&Expr->AsExpressionWithChildren().Left);
            VisitExpression(&Expr->AsExpressionWithChildren().Right);
            break;

        case SX_DELEGATE_CTOR_CALL:
            AssertIfFalse(!Expr->AsDelegateConstructorCallExpression().Constructor ||
                          Expr->AsDelegateConstructorCallExpression().Constructor->IsExprNode());
            VisitExpression((ILTree::Expression **)&Expr->AsDelegateConstructorCallExpression().Constructor);

            AssertIfFalse(!Expr->AsDelegateConstructorCallExpression().ObjectArgument ||
                          Expr->AsDelegateConstructorCallExpression().ObjectArgument->IsExprNode());
            VisitExpression((ILTree::Expression **)&Expr->AsDelegateConstructorCallExpression().ObjectArgument);

            AssertIfFalse(!Expr->AsDelegateConstructorCallExpression().Method ||
                          Expr->AsDelegateConstructorCallExpression().Method->IsExprNode());
            VisitExpression((ILTree::Expression **)&Expr->AsDelegateConstructorCallExpression().Method);
            break;

        case SX_ANONYMOUSTYPE:
        {
            ILTree::AnonymousTypeExpression* AnonymousType = &Expr->AsAnonymousTypeExpression();

            for( ULONG i = 0; i < AnonymousType->BoundMembersCount; i += 1 )
            {
                VisitExpression( ( ILTree::Expression** )&( AnonymousType->BoundMembers[ i ].Temp ) );
                VisitExpression( ( ILTree::Expression** )&( AnonymousType->BoundMembers[ i ].BoundExpression ) );
            }

            break;
        }
        case SX_ARRAYLITERAL:
        {
            VisitExpression((ILTree::Expression **)&Expr->AsArrayLiteralExpression().ElementList);
            break;
        }
        case SX_NESTEDARRAYLITERAL:
        {
            VisitExpression((ILTree::Expression **)&Expr->AsNestedArrayLiteralExpression().ElementList);
            break;
        }
        case SX_COLINIT:
        {
            VisitExpression((ILTree::Expression **)&Expr->AsColInitExpression().NewExpression);
            VisitExpression((ILTree::Expression **)&Expr->AsColInitExpression().Elements);
            VisitExpression((ILTree::Expression **)&Expr->AsColInitExpression().ResultTemporary);

            break;
        }
        case SX_COLINITELEMENT:            
        {
            VisitExpression((ILTree::Expression **)&Expr->AsColInitElementExpression().CallExpression);
            VisitExpression((ILTree::Expression **)&Expr->AsColInitElementExpression().CopyOutArguments);
            break;
        }
        case SX_AWAIT:
        {
            ILTree::AwaitExpression* await = &Expr->AsAwaitExpression();
            VisitExpression(&await->Left);
            ILTree::Expression *expr;

            bool allok0 = (await->GetAwaiterDummy != NULL && await->IsCompletedDummy != NULL && await->GetResultDummy != NULL);

            // 1. visit GetAwaiter
            expr = await->GetAwaiterDummy;
            if (expr!=NULL)
            {
                VisitExpression(&expr);
                if (expr != NULL &&
                    (expr->bilop == SX_CALL || expr->bilop == SX_LATE))
                {
                    await->GetAwaiterDummy = expr;
                }
                else
                {
                    await->GetAwaiterDummy = NULL;
                }
            }

            // 2. visit IsCompletedDummy
            expr = await->IsCompletedDummy;
            if (expr!=NULL)
            {
                VisitExpression(&expr);
                if (expr != NULL &&
                    (expr->bilop == SX_CALL || expr->bilop == SX_LATE))
                {
                    await->IsCompletedDummy = expr;
                }
                else
                {
                    await->IsCompletedDummy = NULL;
                }
            }

            // 3. visit GetResultDummy
            expr = await->GetResultDummy;
            if (expr!=NULL)
            {
                VisitExpression(&expr);
                if (expr != NULL &&
                    (expr->bilop == SX_CALL || expr->bilop == SX_LATE))
                {
                    await->GetResultDummy = expr;
                }
                else
                {
                    await->GetResultDummy = NULL;
                }
            }

            bool allok1 = (await->GetAwaiterDummy != NULL && await->IsCompletedDummy != NULL && await->GetResultDummy != NULL);

            VSASSERT(!allok0 || allok1, "Internal error: in visiting the SX_AWAIT, we've somehow invalidated its awaitor info");
            break;
        }
        case SX_ASYNCSPILL:
        {
            // ASYNCTODO T011: Also visit the SymbolReferenceExpression for the
            // stack storage location.
            ILTree::AsyncSpillExpression* asyncSpill = &Expr->AsAsyncSpillExpression();
            VisitExpression(&asyncSpill->IsCompletedCondition);
            VisitExpression(&asyncSpill->CreateAwaiterArray);
            VisitExpression(&asyncSpill->AwaitXAssignment);
            VisitExpression(&asyncSpill->OnCompletedExtraCondition);
            VisitExpression(&asyncSpill->OnCompletedExtraIfTrue);
            VisitExpression(&asyncSpill->OnCompletedExtraIfFalse1);
            VisitExpression(&asyncSpill->OnCompletedExtraIfFalse2);
            VisitExpression(&asyncSpill->OnCompletedCall);
            VisitExpression(&asyncSpill->StateAssignment);
            VisitExpression(&asyncSpill->StateResetAssignment);
            VisitExpression(&asyncSpill->RestoreAwaiterStatements);
            VisitExpression(&asyncSpill->DoFinallyBodiesAssignment);
            break;
        }
        case SX_BOGUS:
            // (Bug 71364 - DevDiv Bugs) ignore SX_BOGUS by default
            // !!! Consider to handle this node specially when you implement your own BoundTreeVisitor. 
            // !!! You probably don't want to ignore it.            
            break;

#if DEBUG
        case SX_UNBOUND_LAMBDA:
            VSFAIL("SX_UNBOUND_LAMBDA survived until codegen, should have been re-interpreted as an SX_LAMBDA!");
            break;
#endif
        default:
            VSFAIL("Visit bound trees: unexpected SX node");
    }
}

//============================================================================== 
// End BoundTreeVisitor
//==============================================================================


//==============================================================================
// Start BoundTreeTypeVisitor
//==============================================================================    

//==============================================================================
// Look at the passed in type.  Pass it to our base class and then explore the
// type to process any sub types
//==============================================================================    
void BoundTreeTypeVisitor::VisitType(_Inout_ _Deref_prepost_opt_valid_ BCSYM **ppSym)
{
    if ( !ppSym || !(*ppSym) )
    {
        return;
    }

    if ( !StartType(ppSym) )
    {
        return;
    }

    ProcessType(ppSym);
    // Oops! This is inconsistent with VisitExpression above. It's possible for ProcessType
    // to change what ppSym points to -- in which case EndType will no longer be ending
    // the same thing that StartType started. In the case of VisitExpression, it
    // solves that by doing the equivalent of the following:
    //    if (!StartType(ppSym)) return;
    //    BCSYM *&type = *ppSym;
    //    ProcessExpression(type);
    //    Endtype(ppSym);


    EndType(ppSym);
}

void BoundTreeTypeVisitor::ProcessType(_In_ BCSYM **ppSym)
{
    BCSYM *type = *ppSym;

    // Look for types that contain other type references and iterate through those
    if ( type->IsGenericParam() )
    {
        // Nothing special to do for GenericParams at the moment.  If Lambda's eventually
        // get generic support we will need to iterate the constraints
    }
    else if ( type->IsGenericTypeBinding() )
    {
        VisitGenericTypeBinding((GenericTypeBinding**)ppSym);
    }
    else if ( type->IsPointerType() )
    {
        PointerType *pointer = type->PPointerType();
        BCSYM * root_type = pointer->GetRawRoot();
        VisitType(&root_type);
        pointer->SetRoot(root_type);
    }
    else if ( type->IsArrayType() )
    {
        BCSYM_ArrayType *arrayType = type->PArrayType();

        BCSYM *elementType = arrayType->GetRawRoot();
        VisitType(&elementType);
        arrayType->SetElementType(elementType);
    }
    else if ( type->IsNamedType() )
    {
        BCSYM_NamedType *namedType = type->PNamedType();
        BCSYM * temp = namedType->GetSymbol();
        VisitType(&temp);
        namedType->SetSymbol(temp);
    }
}
//==============================================================================
// Look at the passed in binding.  Pass it to our base class and then explore
// the parts of the binding. 
//==============================================================================    
void BoundTreeTypeVisitor::VisitGenericBinding(_Inout_ _Deref_prepost_opt_valid_ GenericBinding **ppBinding)
{
    if ( !ppBinding || !(*ppBinding) )
    {
        return;
    }

    if ( !StartGenericBinding(ppBinding))
    {
        return;
    }

    ProcessGenericBinding(*ppBinding);

    EndGenericBinding(ppBinding);
}

void BoundTreeTypeVisitor::VisitGenericTypeBinding(_Inout_ _Deref_prepost_opt_valid_ GenericTypeBinding **ppBinding)
{
    if ( !ppBinding || !(*ppBinding))
    {
        return;
    }

    if ( !StartGenericTypeBinding(ppBinding))
    {
        return;
    }

    ProcessGenericBinding(*ppBinding);

    EndGenericTypeBinding(ppBinding);
}

bool BoundTreeTypeVisitor::StartBlock(_Inout_ ILTree::ExecutableBlock *block)
{
    switch (block->bilop )
    {
        case SB_PROC: 
            {
                ILTree::ProcedureBlock *body = &(block->AsProcedureBlock());

                // Update the return type
                if ( body->ReturnVariable ) 
                {
                    BCSYM * temp_hack = body->ReturnVariable->GetRawType();
                    VisitType(&temp_hack);
                    body->ReturnVariable->SetType(temp_hack);
                }

                // Update the ResumableGenericType
                if ( body->m_ResumableGenericType != NULL)
                {
                    VisitType(&body->m_ResumableGenericType);
                }

                // Update the temporaries
                ProcessTemporaryManager(body->ptempmgr);
            }
            break;
    }

    // Next iterate over the type of the variables in the block
    ProcessLocalsHash(block->Locals);

    return true;
}

bool BoundTreeTypeVisitor::StartStatement(_Inout_ ILTree::Statement *statement)
{
    if (IsBad(statement))
    {
        return false;
    }
    
    ProcessStatementTypes(statement);
    return true;
}

void BoundTreeTypeVisitor::ProcessStatementTypes(_Inout_ ILTree::Statement *statement)
{
    switch ( statement->bilop )
    {
        case SL_REDIM:
            {
                ILTree::RedimStatement *reStmt = &statement->AsRedimStatement();
                VisitType(&reStmt->ptyp);
            }
            break;
    }
}

//==============================================================================
// Iterate the types on the expressions
//==============================================================================    
bool BoundTreeTypeVisitor::StartExpression(_Inout_ ILTree::Expression **ppExpr)
{
    if (IsBad(*ppExpr))
    {
        return false;
    }
        
    ProcessExpressionTypes(*ppExpr);
    return true;
}

void BoundTreeTypeVisitor::ProcessExpressionTypes(_In_ ILTree::Expression *expr)
{
    // Look for any extra types on the expression
    switch ( expr->bilop )
    {
        case SX_SYM:
            {
                // Fixup the generic binding context if it's present
                ILTree::SymbolReferenceExpression *symRef = &expr->AsSymbolReferenceExpression();
                VisitGenericBinding(&(symRef->GenericBindingContext));
            }
            break;
        case SX_NEW:
            {
                // Need to update the type of the new expression.  In cases where a Generic
                // parameter has the New constraint we need to rebind the class inside 
                // the closure
                ILTree::NewExpression *newExpr = &expr->AsNewExpression();
                VisitType(&newExpr->Class);
            }
            break;
        case SX_LATE:
            {
                ILTree::LateBoundExpression *AsLateBoundExpression = &expr->AsLateBoundExpression();
                VisitType(&AsLateBoundExpression->LateClass);
            }
            break;
        case SX_INIT_STRUCTURE:
            {
                ILTree::InitStructureExpression *initExpr = &expr->AsInitStructureExpression();
                VisitType(&initExpr->StructureType); 
            }
            break;
        case SX_ANDALSO:
        case SX_ORELSE:
            {
                if( expr->IsUserDefinedOperator())
                {
                    VisitGenericBinding(&(expr->AsShortCircuitBooleanOperatorExpression().ConditionOperatorContext));
                }
            }
            break;
        case SX_OVERLOADED_GENERIC:
            {
                ILTree::OverloadedGenericExpression *sxGen = &expr->AsOverloadedGenericExpression();
                for ( unsigned i = 0; i < sxGen->TypeArgumentCount; ++i )
                {
                    VisitType(&(sxGen->TypeArguments[i]));
                }
            }
            break;
        case SX_EXTENSION_CALL:
            {
                ILTree::ExtensionCallExpression *sxExtCall = &expr->AsExtensionCallExpression();
                for ( unsigned i = 0; i < sxExtCall->TypeArgumentCount; ++i )
                {
                    VisitType(&(sxExtCall->TypeArguments[i]));
                }

                //


                //DO NOT 
            }
            break;
        case SX_LAMBDA:
            {
                ILTree::LambdaExpression *AsLambdaExpression = &expr->AsLambdaExpression();
                ProcessTemporaryManager(AsLambdaExpression->TemporaryManager);
                ProcessParameters(AsLambdaExpression->FirstParameter);
                ProcessLocalsHash(AsLambdaExpression->GetLocalsHash());
                VisitType(&AsLambdaExpression->m_ResumableGenericType);
            }
            break;
        case SX_UNBOUND_LAMBDA:
            {
                ILTree::UnboundLambdaExpression *sxUnbound = &expr->AsUnboundLambdaExpression();
                ProcessParameters(sxUnbound->FirstParameter);
            }
            break;
    }

    // Check if we need to handle user defined operators.
    if (expr->IsUserDefinedOperator())
    {
        VisitGenericBinding(&(expr->AsUserDefinedBinaryOperatorExpression().OperatorMethodContext));
    }

    VisitType(&(expr->ResultType));
}

void BoundTreeTypeVisitor::ProcessGenericBinding(GenericBinding *pBinding)
{
    if ( !pBinding )
    {
        return;
    }

    // Look at the arguments on the binding
    unsigned argCount = pBinding->GetArgumentCount();
    BCSYM **argArray= pBinding->GetArguments();
    for ( unsigned i = 0; i < argCount; ++i )
    {
        VisitType(&(argArray[i]));
    }

    // Iterate the parent binding
    BCSYM_GenericTypeBinding * parent_binding = pBinding->GetParentBinding();
    VisitGenericTypeBinding(&parent_binding);
    pBinding->SetParentBinding(parent_binding);

    // Iterate the root of the generic
    BCSYM * cur_generic = pBinding->GetGenericRaw();
    VisitType(&cur_generic);
    pBinding->SetGeneric(cur_generic);
}

//==============================================================================
// Iterate over the type of the symbols in the temporary manager
//==============================================================================    
void BoundTreeTypeVisitor::ProcessTemporaryManager(_Inout_opt_ TemporaryManager *tempmgr)
{
    if ( !tempmgr )
    {
        return;
    }

    TemporaryIterator it;
    it.Init(tempmgr);

    Temporary *cur;
    while (cur=it.GetNext())
    {
        BCSYM * temp_hack = cur->Symbol->GetRawType();
        VisitType(&temp_hack);
        cur->Symbol->SetType(temp_hack);
    }
}

//==============================================================================
// Process the variables in the locals hash and iterate their types
//==============================================================================    
void BoundTreeTypeVisitor::ProcessLocalsHash(Scope *scope)
{
    if ( !scope )
    {
        return;
    }

    BCITER_HASH_CHILD hashIt(scope);
    Declaration *decl;
    while (decl = hashIt.GetNext())
    {
        if ( decl->IsVariable()) 
        {
            Variable *var = decl->PVariable();
            BCSYM * temp_hack = var->GetRawType();
            VisitType(&temp_hack);
            var->SetType(temp_hack);
        }
    }
}

//==============================================================================
// Process the parameters
//==============================================================================    
void BoundTreeTypeVisitor::ProcessParameters(_Inout_opt_ Parameter *p)
{
    while (p) 
    {
		BCSYM * ptyp = p->GetRawType(); 
        VisitType(&ptyp);
		p->SetType(ptyp);
        p = p->GetNext();
    }
}

//==============================================================================
// End BoundTreeTypeVisitor
//==============================================================================    







//============================================================================== 
// Start BoundTreeRewriter
//==============================================================================


ILTree::Statement* BoundTreeRewriter::RewriteStatementList(_In_opt_ ILTree::Statement *statements)
{
    ILTree::Statement *Return = NULL;
    StatementListBuilder bldr(&Return);

    ILTree::Statement *stmt = statements;
    while (stmt!=NULL)
    {
        ILTree::Statement *next = stmt->Next;
        stmt->Next = NULL; // We're putting stmt->Next=NULL here, just so the implementor of RewriteStatement sees that they're getting just a single statement
        stmt = RewriteStatement(stmt);
        bldr.AddList(stmt);
        stmt = next;
    }

    return Return;
}

ILTree::Expression* BoundTreeRewriter::RewriteExpression(_In_ ILTree::Expression *pExpr)
{
    // The logic around "RewriteExpression" and "StartExpression" is a bit convoluted.
    // GOAL: users who inherit from BoundTreeRewriter should be able to override "RewriteExpression",
    // and should be able to call the base method.
    // GOAL: do this with minimal code-changes to the existing BoundTreeVisitor / BoundTreeTypeVisitor.
    //
    // DESIGN: if you call RewriteExpression, then it goes to BoundTreeVisitor.ProcessExpression / BoundTreeTypeVisitor.ProcessExpressionTypes.
    // These are two huge switch statements whose job is to call BoundTreeVisitor.VisitExpression and BoundTreeTypeVisitor.VisitType on each sub-component.
    //
    // VisitExpression starts by calling "StartExpression". Normally it goes on to do ProcessExpression/EndExpression.
    // But we've subverted it so its StartExpression gets redirected back to RewriteExpression.
    //
    // The same design applies to types and statements. Except for statements we hook into StartBlockStatements
    // rather than StartExpression. We do that because StartBlockStatements is the only place
    // that has access to the sequence of statements (hence letting us remove/add statements rather than
    // just replacing them on a 1-1 basis).

    if (!pExpr || IsBad(pExpr)) return pExpr;
    m_adapter.ProcessExpression(pExpr);
    m_adapter.ProcessExpressionTypes(pExpr);
    return pExpr;
}

bool BoundTreeRewriter::VisitorAdapter::StartExpression(_Inout_ ILTree::Expression **ppExpr)
{
    // See comments in "RewriteExpression" for an explanation of the (confusing) control-flow here.
    if (IsBad(*ppExpr)) return false;
    *ppExpr = m_rewriter->RewriteExpression(*ppExpr);
    // Note: it's legal for this to change the type, e.g. if we were using a BoundTreeRewriter to turn all
    // references to "U" in "Iterator Function f<U>()" into references to "U2" in "Class f_StateMachine<U2>".
    return false;
}

BCSYM* BoundTreeRewriter::RewriteType(_In_ BCSYM *type)
{
    if (type==NULL || type->IsBad()) return type;
    if (type->IsGenericParam())
    {
        return type;
    }
    else if (type->IsGenericTypeBinding())
    {
        return RewriteGenericTypeBinding(type->PGenericTypeBinding());
    }
    else if (type->IsPointerType())
    {
        PointerType *pointerType = type->PPointerType();
        BCSYM *oldRoot = pointerType->GetRawRoot();
        BCSYM *newRoot = RewriteType(oldRoot);
        if (oldRoot == newRoot) return pointerType;
        else return m_TransientSymbolCreator->MakePtrType(newRoot);
    }
    else if (type->IsArrayType())
    {
        BCSYM_ArrayType *arrayType = type->PArrayType();
        BCSYM *oldRoot = arrayType->GetRawRoot();
        BCSYM *newRoot = RewriteType(oldRoot);
        if (oldRoot == newRoot) return arrayType;
        else return m_TransientSymbolCreator->GetArrayType(arrayType->GetRank(), newRoot);
    }
    else if (type->IsNamedType())
    {
        BCSYM_NamedType *namedType = type->PNamedType();
        BCSYM *oldRoot = namedType->GetSymbol();
        BCSYM *newRoot = RewriteType(oldRoot);
        if (oldRoot == newRoot) return namedType;
        // A named type is basically a collection of names. At some stage it gets resolved to a symbol,
        // and that symbol is cached in "namedType->GetSymbol()". What we're doing here is creating a
        // new type and pre-emptively setting its resolved-symbol to an updated one.
        BCSYM_NamedType *newNamedType = m_TransientSymbolCreator->GetNamedType(namedType->GetLocation(), namedType->GetContext(), namedType->GetNameCount(), NULL);
        newNamedType->SetSymbol(newRoot);
        return newNamedType;
    }
    else
    {
        return type;
    }
}

BCSYM_GenericTypeBinding* BoundTreeRewriter::RewriteGenericTypeBinding(_In_ BCSYM_GenericTypeBinding *pBinding)
{
    if (pBinding == NULL) return NULL;

    bool anyChanges = false;

    unsigned int argCount = pBinding->GetArgumentCount();
    BCSYM** argArray = NULL;
    for (unsigned int i=0; i<argCount; i++)
    {
        BCSYM *oldRoot = pBinding->GetArguments()[i];
        BCSYM *newRoot = RewriteType(oldRoot);
        if (oldRoot != newRoot || argArray!=NULL)
        {
            if (argArray==NULL)
            {
                argArray = reinterpret_cast<BCSYM**>(m_TransientSymbolCreator->GetNorlsAllocator()->Alloc(argCount * sizeof(BCSYM*)));
                for (unsigned int j=0; j<i; j++) argArray[j] = pBinding->GetArguments()[j];
            }
            argArray[i] = newRoot;
        }
    }

    BCSYM_GenericTypeBinding *oldParentBinding = pBinding->GetParentBinding();
    BCSYM_GenericTypeBinding *newParentBinding = RewriteGenericTypeBinding(oldParentBinding);

    BCSYM_NamedRoot *oldGenericType = pBinding->GetGeneric();
    BCSYM_NamedRoot *newGenericType = RewriteType(oldGenericType)->PNamedRoot();

    if (argArray==NULL && oldParentBinding==newParentBinding && oldGenericType==newGenericType) return pBinding;
    else return m_TransientSymbolCreator->GetGenericBinding(false, newGenericType, argArray, argCount, newParentBinding)->PGenericTypeBinding();
}


bool BoundTreeRewriter::VisitorAdapter::StartType(_Inout_ BCSYM **ppType)
{
    // See comments in "RewriteExpression" for an explanation of the (confusing) control-flow here.
    if (ppType==NULL || *ppType==NULL || (*ppType)->IsBad()) return false;
    *ppType = m_rewriter->RewriteType(*ppType);
    return false;
}

bool BoundTreeRewriter::VisitorAdapter::StartGenericBinding(_Inout_ GenericBinding **ppBinding)
{
    if (ppBinding != nullptr && *ppBinding != nullptr && !(*ppBinding)->IsBad())
    {
        *ppBinding = m_rewriter->RewriteGenericBinding(*ppBinding);
    }

    return true;
}


ILTree::Statement* BoundTreeRewriter::RewriteStatement(_In_ ILTree::Statement *pStatement)
{
    // See comments in "RewriteExpression" for an explanation of the (confusing) control-flow here.
    if (!pStatement || IsBad(pStatement)) return pStatement;
    m_adapter.ProcessStatement(pStatement);
    m_adapter.ProcessStatementTypes(pStatement);
    return pStatement;
}

bool BoundTreeRewriter::VisitorAdapter::StartBlockStatements(ILTree::ExecutableBlock *block)
{
    // See comments in "RewriteExpression" for an explanation of the (confusing) control-flow here.

    // This is called by BoundTreeVisitor::VisitBlockStatement which has no ability to
    // "substitute 0-or-more statements for one that it visits".
    // Instead we'll override it, and return "false" to tell it not to do any walking of the block
    // statements itself, and we ourselves will walk the the block.
    block->Child = m_rewriter->RewriteStatementList(block->Child);
    return false;
}

//============================================================================== 
// End BoundTreeRewriter
//==============================================================================





#if DEBUG

//============================================================================== 
// Start BoundTreeDebugVisitor
//==============================================================================

BoundTreeDebugVisitor::BoundTreeDebugVisitor(Compiler *compiler) 
    : m_norls(NORLSLOC),
    m_norlsWrapper(&m_norls),
    m_statementStack(m_norlsWrapper),
    m_blockStack(m_norlsWrapper)
{

}

BoundTreeDebugVisitor::~BoundTreeDebugVisitor()
{
    // Make sure the stacks completely unwound
    ThrowIfFalse(0 == m_statementStack.Count());
    ThrowIfFalse(0 == m_blockStack.Count());
}

bool BoundTreeDebugVisitor::StartStatement(ILTree::Statement *stmt)
{
    if (IsBad(stmt))
    {
        return false;
    }

    ThrowIfTrue(stmt->IsScratch);       // Scratch statements should not appear in the final BILTREE
    ThrowIfTrue(0 == stmt->GroupId);    // All statements should have a groupid
    m_statementStack.PushOrEnqueue(stmt);
    return true;
}

void BoundTreeDebugVisitor::EndStatement(ILTree::Statement *stmt)
{
    // Make sure this was the last statement we pushed onto the stack
    ThrowIfFalse(m_statementStack.Peek() == stmt);
    m_statementStack.PopOrDequeue();
}

bool BoundTreeDebugVisitor::StartBlock(ILTree::ExecutableBlock *block)
{
    ThrowIfTrue(block->IsScratch);      // Scratch statements should not appear in the final BILTREE
    m_blockStack.PushOrEnqueue(block);
    return true;
}

void BoundTreeDebugVisitor::EndBlock(ILTree::ExecutableBlock *block)
{ 
    // Make usure this was the last block we pushed onto the stack
    ThrowIfFalse(m_blockStack.Peek() == block);
    m_blockStack.PopOrDequeue();
}

bool BoundTreeDebugVisitor::StartExpression(_Inout_ ILTree::Expression **ppExpr)
{
    ThrowIfNull(ppExpr);
    ThrowIfNull(*ppExpr);

    if (IsBad(*ppExpr))
    {
        return false;
    }
        
    ILTree::Expression *expr = *ppExpr;
    ThrowIfTrue(expr->IsScratch);       // Scratch statements should not appear in the final BILTREE
    return true;
}

//============================================================================== 
// End BoundTreeDebugVisitor
//==============================================================================

#endif // DEBUG

//============================================================================== 
// Start BoundTreeBadnessVisitor
//==============================================================================

BoundTreeBadnessVisitor::BoundTreeBadnessVisitor() 
    : m_isBad(false)
{

}

BoundTreeBadnessVisitor::~BoundTreeBadnessVisitor()
{
}

bool BoundTreeBadnessVisitor::StartStatement(ILTree::Statement *stmt)
{
    if (m_isBad)
    {
        return false;
    }
    
    if (IsBad(stmt))
    {
        m_isBad = true;
        return false;
    }
    
    return true;
}

void BoundTreeBadnessVisitor::EndStatement(ILTree::Statement *stmt)
{
}

bool BoundTreeBadnessVisitor::StartBlock(ILTree::ExecutableBlock *block)
{
    if (m_isBad)
    {
        return false;
    }
    
    if (IsBad(block))
    {
        m_isBad = true;
        return false;
    }

    // Note: the ancestor BoundTreeVisitor::VisitBlock will call StartBlock, then it'll iterate
    // through all the statements, then will call EndBlock. HOWEVER it declines to even visit
    // any bad statements. We want to detect bad statements (as well as bad expressions inside
    // good statements) which is why we need to scan for bad statements right here.
    for(ILTree::Statement *CurrentStmt = block->Child; CurrentStmt; CurrentStmt = CurrentStmt->Next)
    {
        if (IsBad(CurrentStmt))
        {
            m_isBad = true;
            return false;
        }
    }

    return true;
}

void BoundTreeBadnessVisitor::EndBlock(ILTree::ExecutableBlock *block)
{
}

bool BoundTreeBadnessVisitor::StartExpression(_Inout_ ILTree::Expression **ppExpr)
{
    if (m_isBad)
    {
        return false;
    }

    if (IsBad(*ppExpr))
    {
        m_isBad = true;
        return false;
    }

    switch ((*ppExpr)->bilop)
    {
        case SX_LAMBDA:
            // Do not dig into a statement lambda, LambdaSemantics already did that.
            if ((*ppExpr)->AsLambdaExpression().IsStatementLambda)
            {
                return false;
            }

            break;

        case SX_ADDRESSOF:
            // A tree with errors may contain not-lowered SX_ADDRESSOF nodes.
            // BoundTreeVisitor, the base class, will assert in this case.
            // Let's ignore this node.
            return false;
    }
    
    return true;
}

void BoundTreeBadnessVisitor::Visit(_In_opt_ ILTree::ExecutableBlock *block)
{
    ThrowIfNull(block);
    VisitBlock(block);
}


//============================================================================== 
// End BoundTreeBadnessVisitor
//==============================================================================


//==============================================================================
// Start AnonymousDelegatesRegisterVisitor
//==============================================================================    

// This method overrides the StartGenericTypeBinding in the base iterator and registers
// any anonymous delegates seen during the tree walk.
//
bool AnonymousDelegatesRegisterVisitor::StartGenericTypeBinding(_In_opt_ GenericTypeBinding **ppTypeBinding)
{
    if (!ppTypeBinding || !*ppTypeBinding)
    {
        return true;
    }

    // If the generic type is an anonymous delegate, register it.

    if ((*ppTypeBinding)->GetGeneric()->IsClass())
    {
        BCSYM_Class *pClass = (*ppTypeBinding)->GetGeneric()->PClass();

        if (pClass->IsAnonymousDelegate())
        {
            m_pTransientSymbolStore->InsertAnonymousDelegateIfNotAlreadyInserted(pClass, m_Procedure);
        }
    }

    return true;
}

//==============================================================================
// End AnonymousDelegatesRegisterVisitor
//==============================================================================    


//============================================================================== 
// Start SerializingVisitor
//==============================================================================
SerializingVisitor::SerializingVisitor(IAppendable<ILTree::ILNode *> * pOutput) : 
    m_pOutput(pOutput)
{
    ThrowIfNull(pOutput);
}

bool SerializingVisitor::StartBlock(ILTree::ExecutableBlock* pBlock)
{
    m_pOutput->Add(pBlock);
    return true;
}

bool SerializingVisitor::StartStatement(ILTree::Statement* pStatement)
{
    if (IsBad(pStatement))
    {
        return false;
    }

    m_pOutput->Add(pStatement);
    return true;    
}

bool SerializingVisitor::StartExpression(_Inout_ ILTree::Expression ** ppExpression)
{
    ThrowIfNull(ppExpression);

    if (IsBad(*ppExpression))
    {
        return false;
    }
        
    m_pOutput->Add(*ppExpression);
    return true;
}
//============================================================================== 
// End SerializingVisitor
//==============================================================================


//==============================================================================
// Start ExtensionMethodBindingChangeDetector
//==============================================================================    
ExtensionMethodBindingChangeDetector::ExtensionMethodBindingChangeDetector() :
    m_ret(false),
    m_pWithoutImportsTreeIterator(NULL)
{
}

bool ExtensionMethodBindingChangeDetector::DidExtensionMethodBindingsChange
(
    _In_opt_ ILTree::ProcedureBlock * pBodyWithImports, 
    _In_opt_ ILTree::ProcedureBlock * pBodyWithoutImports, 
    _Inout_ NorlsAllocator * pNorls
)
{
    NorlsMark mark;
    memset(&mark, 0, sizeof(NorlsMark));

    pNorls->Mark(&mark);

    {
        List<ILTree::ILNode *, NorlsAllocWrapper> serializedBody(pNorls);

        SerializingVisitor visitor(&serializedBody);
        visitor.Visit(pBodyWithoutImports);

        m_ret = false;

        ListValueIterator<ILTree::ILNode *, NorlsAllocWrapper> iterator(&serializedBody);
        m_pWithoutImportsTreeIterator = &iterator;

        Visit(pBodyWithImports);
        CheckRemainingNodes();
        m_pWithoutImportsTreeIterator = NULL;
    }

    pNorls->Free(&mark);

    return m_ret;
}

// If the main iterator is finished walking the new bound tree we should
// have exhausted the original tree. If there are nodes left it means
// the tree changed. So therefore sd mark it as a failure if there are
// nodes in the tree that didn't have the imports.
void ExtensionMethodBindingChangeDetector::CheckRemainingNodes()
{
    if (! m_ret)
    {
        m_ret = m_pWithoutImportsTreeIterator->MoveNext();
    }
}

bool ExtensionMethodBindingChangeDetector::CompareGeneralNodes(ILTree::ILNode * pThisTree)
{
    if (! m_ret)
    {
        m_ret = ! m_pWithoutImportsTreeIterator->MoveNext();
    }

    if (! m_ret)
    {
        ILTree::ILNode * pOtherTree = m_pWithoutImportsTreeIterator->Current();

        if 
        (
            (BoolToInt(pThisTree) ^ BoolToInt(pOtherTree)) ||
            (
                pThisTree &&
                pOtherTree &&
                ! IsBad(pOtherTree) &&
                IsBad(pThisTree)
            ) ||
            (
                pThisTree &&
                pOtherTree &&
                ! IsBad(pOtherTree) &&
                pThisTree->bilop != pOtherTree->bilop
            )
        )
        {
            m_ret = true;
        } 
    }

    return ! m_ret;    
}

//WARNING:
//please do not see this method and think that it can be repurposed for anything, because
//chances are it can't. This method is only suitable for use in the AddImports extension method binding collision checks.
//In particular, it works because the 2 methods symbols being compared are guaranteed to have been obtained from the same
//method in the same class in the same project, and so there are no cross project identity issues. However, in general, equivalent types
//may have different pointers if they are accessed via different projects.
bool ExtensionMethodBindingChangeDetector::CompareSymbolNodes(_In_opt_ ILTree::SymbolReferenceExpression * pSymbolReference, _In_opt_ ILTree::SymbolReferenceExpression * pOtherSymbolReference )
{
    ThrowIfNull(pSymbolReference);
    ThrowIfNull(pOtherSymbolReference);


    if (! m_ret)
    {
        m_ret =  
            ! BCSYM_GenericBinding::AreBindingsEqual(pSymbolReference->GenericBindingContext, pOtherSymbolReference->GenericBindingContext) ||
            pSymbolReference->Symbol != pOtherSymbolReference->Symbol;
    }

    return ! m_ret;
}

bool ExtensionMethodBindingChangeDetector::StartBlock(ILTree::ExecutableBlock* pBlock)
{
    return CompareGeneralNodes(pBlock);
}

bool ExtensionMethodBindingChangeDetector::StartStatement(ILTree::Statement* pStatement)
{
    if (IsBad(pStatement))
    {
        return false;
    }

    return CompareGeneralNodes(pStatement);
}

bool ExtensionMethodBindingChangeDetector::StartExpression(_Inout_ ILTree::Expression ** ppExpression)
{
    ThrowIfNull(ppExpression);

    if (IsBad(*ppExpression))
    {
        return false;
    }
        
    bool isExtensionMethod = m_nextExpressionIsExtensionMethod;

    if (isExtensionMethod)
    {
        m_nextExpressionIsExtensionMethod = false;
    }

    ILTree::Expression * pExpression = *ppExpression;

    CompareGeneralNodes(pExpression);

    ILTree::ILNode * pOtherTree = m_pWithoutImportsTreeIterator->Current();

    if (! m_ret && pExpression && pOtherTree && !IsBad(pExpression))
    {
        if (pExpression->bilop == SX_CALL && HasFlag32(pExpression, SXF_CALL_WAS_EXTENSION_CALL))
        {
            m_nextExpressionIsExtensionMethod = true;
        }
        else if (isExtensionMethod && pExpression->bilop == SX_SYM)
        {
            CompareSymbolNodes(&(pExpression->AsSymbolReferenceExpression()), &(pOtherTree->AsSymbolReferenceExpression()));
        }
    }

    return ! m_ret;
}


void ExtensionMethodBindingChangeDetector::EndBlock(ILTree::ExecutableBlock* pBlock) 
{
    m_nextExpressionIsExtensionMethod = false;
}

void ExtensionMethodBindingChangeDetector::EndStatement(ILTree::Statement* pStatement) 
{
    m_nextExpressionIsExtensionMethod = false;    
}    

//==============================================================================
// End ExtensionMethodBindingChangeDetector
//==============================================================================    

//==============================================================================
// Start LowerBoundTreeVisitor
//==============================================================================

LowerBoundTreeVisitor::LowerBoundTreeVisitor(Semantics *semantics)
    :
    m_iterating(false),
    m_semantics(semantics),
    m_Compiler(semantics->m_Compiler),
    m_CompilerHost(semantics->m_CompilerHost),
    m_pEnclosingProc(NULL)
{
    ThrowIfNull(semantics);
}

LowerBoundTreeVisitor::~LowerBoundTreeVisitor()
{
}

// Iterator Methods

void LowerBoundTreeVisitor::Visit(_In_opt_ ILTree::ProcedureBlock *BoundBody)
{
    ThrowIfNull( BoundBody );

    BCSYM_Proc * pProcSave = m_pEnclosingProc;
    m_pEnclosingProc = BoundBody->pproc;
    m_pEnclosingProc->SetJITOptimizationsMustBeDisabled(false);


    m_iterating = true;
    BoundTreeVisitor::Visit(BoundBody);
    m_iterating = false;

    m_pEnclosingProc = pProcSave;
}

void LowerBoundTreeVisitor::Visit(_Inout_opt_ ILTree::Expression **ppExpr)
{
    ThrowIfNull( ppExpr );
    ThrowIfNull( *ppExpr );

    m_iterating = true;
    BoundTreeVisitor::VisitExpression(ppExpr);
    m_iterating = false;
}

/*=======================================================================================
InterpretUserDefinedShortCircuitOperator

This function builds a bound tree representing an overloaded short circuiting expression
after determining that the necessary semantic conditions are met.

An expression of the form:

    x AndAlso y  (where the type of x is X and the type of y is Y)

is an overloaded short circuit operation if X and Y are user-defined types and an
applicable operator And exists after applying normal operator resolution rules.

Given an applicable And operator declared in type T, the following must be true:

    - The return type and parameter types must be T.
    - T must contain a declaration of operator IsFalse.

If these conditions are met, the expression "x AndAlso y" is translated into:

    !T.IsFalse(temp = x) ? T.And(temp, y) : temp

The temporary is necessary for evaluating x only once. Similarly, "x OrElse y" is
translated into:

    !T.IsTrue(temp = x) ? T.Or(temp, y) : temp










*/


BILOP MapOperatorToTrueFalseBilOp(UserDefinedOperators op)
{
    switch (op)
    {
        case OperatorIsTrue:
            return SX_ISTRUE;
        case OperatorIsFalse:
            return SX_ISFALSE;
        default:
            return SX_COUNT;
    }
}

bool
LowerBoundTreeVisitor::VisitShortCircuitBooleanOperator
(
    _Inout_opt_ ILTree::Expression ** ppExpr
)
{
    ThrowIfNull( ppExpr );

    ILTree::Expression* Expr = *ppExpr;

    ThrowIfNull( Expr );
    ThrowIfFalse( Expr->bilop == SX_ANDALSO || Expr->bilop == SX_ORELSE );
    ThrowIfFalse( Expr->IsUserDefinedOperator() );
    ThrowIfNull( Expr->AsUserDefinedBinaryOperatorExpression().OperatorMethod );
    ThrowIfNull( Expr->AsShortCircuitBooleanOperatorExpression().ConditionOperator );

    BILOP Opcode = Expr->bilop;
    Procedure *OperatorMethod = Expr->AsUserDefinedBinaryOperatorExpression().OperatorMethod;
    GenericBinding *OperatorMethodGenericContext = Expr->AsUserDefinedBinaryOperatorExpression().OperatorMethodContext;
    Procedure *ConditionOperator = Expr->AsShortCircuitBooleanOperatorExpression().ConditionOperator;
    GenericBinding *ConditionOperatorGenericContext = Expr->AsShortCircuitBooleanOperatorExpression().ConditionOperatorContext;
    const Location &ExpressionLocation = Expr->Loc;
    ILTree::Expression *Left = Expr->AsBinaryExpression().Left;
    ILTree::Expression *Right = Expr->AsBinaryExpression().Right;
    ExpressionFlags Flags = Expr->AsUserDefinedBinaryOperatorExpression().InterpretationFlags;

    // Lift the Left operand into a temporary.
    Variable *LeftTemporary = NULL;

    ILTree::Expression *ConditionArgument = m_semantics->CaptureInShortLivedTemporary(Left, LeftTemporary);
    SetResultType(ConditionArgument, Expr->ResultType);

    // Build the call to IsTrue/IsFalse.
    ILTree::Expression *ConditionCall =
        m_semantics->InterpretUserDefinedOperator(
            SX_COUNT,
            ConditionOperator,
            ConditionOperatorGenericContext,
            ExpressionLocation,
            ConditionArgument,
            Flags);

    if (ConditionOperator->IsLiftedOperatorMethod())
    {
        //If we have a lifted IsTrue or IsFalse operator then
        //we need to generate an SX_ISTRUE or SX_ISFALSE node ontop of the Condition call that will
        //cause us to implement the correct lifting.

        ConditionCall = 
            GetSemantics()->AllocateExpression
            (
                MapOperatorToTrueFalseBilOp(ConditionOperator->PLiftedOperatorMethod()->GetActualProc()->GetAssociatedOperatorDef()->GetOperator()),
                ConditionOperator->PLiftedOperatorMethod()->GetActualProc()->GetType(), 
                ConditionCall, 
                ConditionCall->Loc
            );
    }

    // Build the call to And/Or.
    ILTree::Expression *OperatorCall =
        m_semantics->InterpretUserDefinedOperator(
            Opcode,
            OperatorMethod,
            OperatorMethodGenericContext,
            ExpressionLocation,
            m_semantics->ReferToSymbol(
                ExpressionLocation,
                LeftTemporary,
                chType_NONE,
                NULL,
                NULL,
                ExprNoFlags),
            Right,
            Flags);

    if (IsBad(ConditionCall) || IsBad(OperatorCall))
    {
        // Dev 10 393046
        *ppExpr = m_semantics->AllocateBadExpression(ExpressionLocation);
        return false;
    }

    // Now build the ternary operation using SX_IF.
    //
    // 



    ILTree::Expression* Result = m_semantics->AllocateExpression(
            SX_SEQ_OP2,
            Expr->ResultType,
            m_semantics->AllocateExpression(
                SX_IF,
                TypeHelpers::GetVoidType(),
                m_semantics->AllocateExpression(
                    SX_NOT,
                    m_semantics->GetFXSymbolProvider()->GetBooleanType(),
                    ConditionCall,
                    ExpressionLocation),
                m_semantics->AllocateExpression(
                    SX_ASG,
                    TypeHelpers::GetVoidType(),
                    m_semantics->ReferToSymbol(
                        ExpressionLocation,
                        LeftTemporary,
                        chType_NONE,
                        NULL,
                        NULL,
                        ExprNoFlags),
                    OperatorCall,
                    ExpressionLocation),
                ExpressionLocation),
            m_semantics->ReferToSymbol(
                ExpressionLocation,
                LeftTemporary,
                chType_NONE,
                NULL,
                NULL,
                ExprNoFlags),
            ExpressionLocation);

    *ppExpr = Result;
    return( true );
}

void LowerBoundTreeVisitor::EndStatement(ILTree::Statement  *stmt)
{
    m_semantics->m_TemporaryManager->FreeShortLivedTemporaries();

    if ( SL_END == stmt->bilop )
    {
        // Dev10 #850039 Risky function Microsoft.VisualBasic.CompilerServices.ProjectData.EndApp is used.
        m_pEnclosingProc->SetJITOptimizationsMustBeDisabled(true);
    }
}

bool LowerBoundTreeVisitor::StartExpression(_Inout_ ILTree::Expression **ppExpr)
{
    ThrowIfNull(ppExpr);
    ThrowIfNull(*ppExpr);

    if (IsBad(*ppExpr))
    {
        return false;
    }
        
    ILTree::Expression *expr = *ppExpr;
    switch (expr->bilop)
    {
        case SX_CTYPEOP:
        {
            // implement null check on lifted nullabble user defined conversions
            // 'methodCallOp' is the user defined conversion operator.
            // if S?.HasValue() then T?::op_Implicit(Convert_to_T_byUserDefinedConversion(S?.Value()) else New T?() ) -> T?

            Type *TargetType = expr->ResultType;
            AssertIfFalse(TypeHelpers::IsNullableType(TargetType, m_CompilerHost));
            ILTree::Expression *Source = expr->AsLiftedCTypeExpression().Left;
            Type *SourceType = Source->ResultType;
            AssertIfFalse(TypeHelpers::IsNullableType(SourceType, m_CompilerHost));
            Type    *ElementTargetType = TypeHelpers::GetElementTypeOfNullable(TargetType, m_CompilerHost);
            Type    *ElementSourceType = TypeHelpers::GetElementTypeOfNullable(SourceType, m_CompilerHost);
            ILTree::Expression *Condition = NULL, *Value = NULL;

            m_semantics->ProcessNullableOperand(Source, Condition, Value);

            ILTree::Expression* ElementConversion = m_semantics->ConvertUsingConversionOperator(
                            Value, 
                            ElementTargetType, 
                            expr->AsLiftedCTypeExpression().OperatorMethod, 
                            expr->AsLiftedCTypeExpression().OperatorMethodContext,
                            expr->AsLiftedCTypeExpression().InterpretationFlags);

            *ppExpr = m_semantics->AllocateIIfExpression
            (
                TargetType,
                Condition,
                m_semantics->WrapInNullable(TargetType, ElementConversion),
                m_semantics->CreateNullValue(TargetType, expr->Loc),
                expr->Loc
            );

            return true;
        }

        case SX_CTYPE:
        {
            *ppExpr = m_semantics->LowerCType( expr );
            return true;
        }
        case SX_IIFCoalesce:
        {
            ILTree::Expression  *Condition = NULL;
            ILTree::Expression  *TrueExpr = NULL;
            ILTree::Expression  *FalseExpr = NULL;
            m_semantics->LoadIIfElementsFromIIfCoalesce(expr, Condition, TrueExpr, FalseExpr, false/*fPreserveCondition*/);

            *ppExpr = m_semantics->AllocateIIfExpression(
                expr->ResultType,
                Condition,
                TrueExpr,
                FalseExpr,
                expr->Loc);

            return true;
        }
        case SX_NOT:
        case SX_NEG:
        case SX_PLUS:
        {
            if (HasFlag32(expr, SXF_OP_LIFTED_NULLABLE))
            {
                VisitLiftedIntrinsicUnaryOperator(ppExpr);
            }
            break;
        }
        case SX_ADD:
        case SX_SUB:
        case SX_MUL:
        case SX_DIV:
        case SX_POW:
        case SX_IDIV:
        case SX_SHIFT_LEFT:
        case SX_SHIFT_RIGHT:
        case SX_MOD:
        case SX_OR:
        case SX_XOR:
        case SX_AND:
        case SX_EQ:
        case SX_NE:
        case SX_LT:
        case SX_LE:
        case SX_GE:
        case SX_GT:
        {
            if (HasFlag32(expr, SXF_OP_LIFTED_NULLABLE))
            {
                VisitLiftedIntrinsicBinaryOperator(ppExpr);
            }
            break;
        }

        case SX_IS:
        case SX_ISNOT:
        {
            if (HasFlag32(expr, SXF_OP_LIFTED_NULLABLE))
            {
                VisitLiftedIsOrIsNotOperator(ppExpr);
            }
            break;
        }

        case SX_CALL:
            VisitCallExpression(ppExpr);
            break;

        case SX_ANDALSO:
        case SX_ORELSE:
        {
            if (HasFlag32(expr, SXF_OP_LIFTED_NULLABLE))
            {
                VisitLiftedIntrinsicBinaryOperator(ppExpr);
            }
            else if(expr->IsUserDefinedOperator())
            {
                VisitShortCircuitBooleanOperator(ppExpr);
            }
            break;
        }

        case SX_ISTRUE:
        case SX_ISFALSE:
            VisitIsTrueIsFalseNode(ppExpr);
            break;

        case SX_ANONYMOUSTYPE:
            VisitAnonymousType( ppExpr );
            break;
        case SX_ARRAYLITERAL:
            VisitArrayLiteral(ppExpr);
            return false; //don't automaticly visit children. We already did this inside VisitArrayLiteral
        case SX_NESTEDARRAYLITERAL:
            if (((*ppExpr)->AsNestedArrayLiteralExpression()).ElementList)
            {
                *ppExpr = ((*ppExpr)->AsNestedArrayLiteralExpression()).ElementList;
            }
            else
            {
                *ppExpr =
                    m_semantics->AllocateExpression
                    (
                        SX_LIST,
                        TypeHelpers::GetVoidType(),
                        NULL,
                        NULL,
                        (*ppExpr)->Loc
                    );
            }
            VisitExpression(ppExpr);
            return false;
        case SX_COLINIT:
            VisitCollectionInitializer(ppExpr);
            break;
    }

    return true;
}

void
LowerBoundTreeVisitor::VisitCollectionInitializer    
(
    ILTree::Expression ** ppExpr
)
{
    ThrowIfNull(ppExpr);
    ThrowIfNull(*ppExpr);
    ThrowIfFalse((*ppExpr)->bilop == SX_COLINIT);

    ILTree::ColInitExpression * pColInit = &(*ppExpr)->AsColInitExpression();

    ThrowIfFalse(!pColInit->Elements || pColInit->ResultTemporary);
    ThrowIfNull(pColInit->NewExpression);
    ThrowIfFalse(!pColInit->Elements || pColInit->Elements->bilop == SX_LIST);
    

    ILTree::SymbolReferenceExpression * pSymRef = pColInit->ResultTemporary;
    ILTree::ExpressionWithChildren * pList = pColInit->Elements;
    
    ILTree::Expression * pInitSeq = NULL;   

    while (pList)
    {
        ThrowIfFalse(pList->Left && pList->Left->bilop == SX_COLINITELEMENT);
        ILTree::ColInitElementExpression * pCurrent = &pList->Left->AsColInitElementExpression();
        ILTree::Expression * pCallTree = NULL;
        
        if (pCurrent->CopyOutArguments)
        {
            pCallTree = m_semantics->AppendCopyOutArguments( pCurrent->CallExpression, pCurrent->CopyOutArguments, pCurrent->CallInterpretationFlags);
        }
        else
        {
            pCallTree = pCurrent->CallExpression;
        }

        if (pInitSeq)
        {
            pInitSeq =
                m_semantics->AllocateExpression
                (
                    SX_SEQ,
                    TypeHelpers::GetVoidType(),
                    pInitSeq,
                    pCallTree,
                    pColInit->Loc
                );
        }
        else
        {
            pInitSeq = pCallTree;
        }

        pList = pList->Right ? &pList->Right->AsExpressionWithChildren() : NULL;
    }

    if (pInitSeq)
    {
        ILTree::Expression * pAsg = m_semantics->AllocateExpression(SX_ASG, TypeHelpers::GetVoidType(), pSymRef, pColInit->NewExpression, pColInit->NewExpression->Loc);

        ILTree::SymbolReferenceExpression * pSymRefCopy = 
        m_semantics->AllocateSymbolReference
        (
            pSymRef->Symbol, 
            pSymRef->ResultType,
            pSymRef->BaseReference,
            pSymRef->Loc,
            pSymRef->GenericBindingContext
        );

        *ppExpr = 
            m_semantics->AllocateExpression
            (
                SX_SEQ,
                pColInit->ResultType,
                m_semantics->AllocateExpression
                (
                    SX_SEQ,
                    TypeHelpers::GetVoidType(),
                    pAsg,
                    pInitSeq,
                    pColInit->Loc
                ),
                pSymRefCopy,
                pColInit->Loc
            );    

    }
    else
    {
        *ppExpr = pColInit->NewExpression;
    }
        
}

bool 
LowerBoundTreeVisitor::IsEmpty
(
    ILTree::ArrayLiteralExpression * pLiteral
)
{
    if (!pLiteral)
    {
        return true;
    }
    else
    {
        for (unsigned i = 0; i < pLiteral->Rank; ++i)
        {
            if (pLiteral->Dims[i])
            {
                return false;
            }
        }

        return true;
    }
}

void
LowerBoundTreeVisitor::VisitArrayLiteral
(
    ILTree::Expression ** ppExpr
)
{
    ThrowIfNull(ppExpr);
    ThrowIfNull(*ppExpr);
    
    ThrowIfFalse((*ppExpr)->bilop == SX_ARRAYLITERAL);
    
    ILTree::ArrayLiteralExpression * pLiteral = &(*ppExpr)->AsArrayLiteralExpression();

    if (!pLiteral->ResultType->IsArrayType())
    {
        VSFAIL("An array literal with a non array return type should not have made it to the bound tree lowering phase!");
        m_semantics->ReportSemanticError(ERRID_InternalCompilerError, pLiteral->Loc);
    }
    else
    {

        Semantics::ExpressionListHelper dimList(m_semantics);
        BCSYM_ArrayType * pArrayType = pLiteral->ResultType->PArrayType();

        VisitExpression((ILTree::Expression **)&pLiteral->ElementList);

        *ppExpr = 
            m_semantics->InitializeArray
            (
                pLiteral->ElementList,
                pArrayType,
                m_semantics->CreateDimList(pLiteral->Rank, pLiteral->Dims, pLiteral->Loc),
                pLiteral->Loc
            );

        VisitExpression(ppExpr);
    }
}

void 
LowerBoundTreeVisitor::VisitIsTrueIsFalseNode
(
    _Inout_opt_ ILTree::Expression ** ppExpr
)
{
    ThrowIfNull(ppExpr);
    ThrowIfNull(*ppExpr);

    ILTree::Expression * expr = *ppExpr;

    ThrowIfFalse(expr->bilop == SX_ISTRUE || expr->bilop == SX_ISFALSE);


    if 
    ( 
        !(
            GetSemantics() &&
            expr->ResultType && 
            GetSemantics()->GetFXSymbolProvider()->GetBooleanType() &&
            BCSYM::AreTypesEqual(expr->ResultType, GetSemantics()->GetFXSymbolProvider()->GetBooleanType()) &&
            expr->AsExpressionWithChildren().Left &&
            expr->AsExpressionWithChildren().Left->ResultType &&
            TypeHelpers::IsNullableType(expr->AsExpressionWithChildren().Left->ResultType, GetSemantics()->GetCompilerHost()) && 
            BCSYM::AreTypesEqual(GetSemantics()->GetFXSymbolProvider()->GetBooleanType(), TypeHelpers::GetElementTypeOfNullable(expr->AsExpressionWithChildren().Left->ResultType, GetSemantics()->GetCompilerHost()))
        )
    )
    {
        GetSemantics()->ReportSemanticError(ERRID_CodegenError, expr->Loc);
    }
    else
    {
        ILTree::Expression * pCondition = NULL;
        ILTree::Expression * value = NULL;


        if
        (
            (
                expr->AsExpressionWithChildren().Left->bilop == SX_CALL &&
                expr->AsExpressionWithChildren().Left->AsCallExpression().Left->bilop == SX_SYM &&
                expr->AsExpressionWithChildren().Left->AsCallExpression().Left->AsSymbolReferenceExpression().Symbol->IsLiftedOperatorMethod() &&
                MapOperatorToTrueFalseBilOp
                (
                    expr->AsExpressionWithChildren().Left->AsCallExpression().Left->AsSymbolReferenceExpression().Symbol->PLiftedOperatorMethod()->GetActualProc()->GetAssociatedOperatorDef()->GetOperator()
                ) == expr->bilop
            )
        )
        {
            ILTree::CallExpression * pCall = &(expr->AsExpressionWithChildren().Left->AsCallExpression());
            LiftedOperatorMethod * pLiftedOperator = pCall->Left->AsSymbolReferenceExpression().Symbol->PLiftedOperatorMethod();
            ILTree::ExpressionWithChildren * pCallArgument = &(pCall->Right->AsExpressionWithChildren());


            m_semantics->ProcessNullableOperand(pCallArgument->Left, pCondition, value);

            pCall->Left = 
                GetSemantics()->ReferToSymbol
                (
                    pCall->Loc, pLiftedOperator->GetActualProc(), 
                    chType_NONE, 
                    NULL,
                    pCall->Left->AsSymbolReferenceExpression().GenericBindingContext,
                    ExprIsExplicitCallTarget
                );

            pCall->ResultType = GetSemantics()->TypeInGenericContext(pLiftedOperator->GetActualProc()->GetType(), pCall->Left->AsSymbolReferenceExpression().GenericBindingContext);

            pCallArgument->Left = value;

            *ppExpr = 
                GetSemantics()->AllocateIIfExpression
                (
                    expr->ResultType,
                    pCondition,
                    pCall,
                    GetSemantics()->ProduceConstantExpression
                    (
                        COMPLUS_FALSE, 
                        expr->Loc, 
                        expr->ResultType
                        IDE_ARG(0)
                    ),
                    expr->Loc
                );

            SetFlag32(pCall->Left, SXF_SYM_NONVIRT);
        }
        else if (expr->bilop == SX_ISTRUE)
        {
            // IsTrue(Operand) is equivalent to Operand.GetValueOrDefault().
            *ppExpr = m_semantics->CreateNullableValueCall(expr->AsExpressionWithChildren().Left->ResultType, expr->AsExpressionWithChildren().Left);
        }
        else
        {
            m_semantics->ProcessNullableOperand(expr->AsExpressionWithChildren().Left, pCondition, value);

            *ppExpr = 
                GetSemantics()->AllocateExpression
                (
                    SX_ANDALSO,
                    m_semantics->GetFXSymbolProvider()->GetBooleanType(),
                    pCondition,
                    GetSemantics()->AllocateExpression
                    (
                        SX_NOT, 
                        expr->ResultType, 
                        value, 
                        expr->Loc
                    ),
                    expr->Loc
                );
        }       
    }
}

void
LowerBoundTreeVisitor::VisitAnonymousType( _Inout_opt_ ILTree::Expression** ppExpr )
{
    ThrowIfNull( ppExpr );
    ThrowIfNull( *ppExpr );

    ILTree::Expression* Expr = *ppExpr;
    ILTree::AnonymousTypeExpression* pAnonymousType = &Expr->AsAnonymousTypeExpression();

    // For anonymous types, we need to build a chain of temp assignments (if applicable)
    // and then we have to emit the constructor call.

    // While we do this, we will also build up the arguments for the constructor call.

    ParserHelper PH( &GetSemantics()->m_TreeStorage, Expr->Loc );
    ParseTree::ArgumentList* Arguments = NULL;
    ParseTree::ArgumentList* CurrentArguments = NULL;

    ILTree::Expression* Result = NULL;
    ILTree::Expression* Current = NULL;

    for( ULONG i = 0; i < pAnonymousType->BoundMembersCount; i += 1 )
    {
        ILTree::Expression* ArgumentExpression = NULL;

        if( pAnonymousType->BoundMembers[ i ].Temp != NULL )
        {
            ILTree::Expression* AssignmentStatement = GetSemantics()->AllocateExpression(
                SX_ASG,
                TypeHelpers::GetVoidType(),
                pAnonymousType->BoundMembers[ i ].Temp,
                pAnonymousType->BoundMembers[ i ].BoundExpression,
                Expr->Loc
                );

            // Link the statement into the sequence.

            ILTree::Expression* Seq = GetSemantics()->AllocateExpression(
                SX_SEQ_OP2,
                pAnonymousType->BoundMembers[ i ].Temp->AsSymbolReferenceExpression().Symbol->PVariable()->GetType(),
                AssignmentStatement,
                NULL,
                Expr->Loc
                );

            if( Result == NULL )
            {
                Result = Current = Seq;
            }
            else
            {
                VSASSERT( Current->AsExpressionWithChildren().Right == NULL, "Bad tree for SX_SEQ_OP2!" );
                Current->AsExpressionWithChildren().Right = Seq;
                Current = Seq;
            }

            ArgumentExpression = pAnonymousType->BoundMembers[ i ].Temp;
        }
        else
        {
            ArgumentExpression = pAnonymousType->BoundMembers[ i ].BoundExpression;
        }

        ThrowIfNull( ArgumentExpression );

        if( Arguments == NULL )
        {
            Arguments = CurrentArguments = PH.CreateArgList(
                PH.CreateBoundExpression( ArgumentExpression )
                );
        }
        else
        {
            CurrentArguments = PH.AddArgument(
                CurrentArguments,
                PH.CreateBoundExpression( ArgumentExpression )
                );
        }
    }

    // Ok, now let's build the constructor call.

    ILTree::Expression* ConstructorCall = GetSemantics()->CreateConstructedInstance(
        pAnonymousType->ResultType,
        Expr->Loc,
        Expr->Loc,
        Arguments,
        ExprNoFlags
        );

    if( Result == NULL )
    {
        Result = ConstructorCall;
    }
    else
    {
        VSASSERT( Current->AsExpressionWithChildren().Right == NULL, "Bad tree for SX_SEQ_OP2!" );
        Current->AsExpressionWithChildren().Right = ConstructorCall;
    }

    *ppExpr = Result;
}


void 
LowerBoundTreeVisitor::VisitCallExpression
(
    _Inout_ ILTree::Expression ** ppExpr
)
{
    ILTree::CallExpression * pCall = (ILTree::CallExpression *)(*ppExpr);

    if 
    (
        pCall->Left && 
        pCall->Left->bilop == SX_SYM && 
        pCall->Left->AsSymbolReferenceExpression().Symbol
    )
    {
        if (pCall->Left->AsSymbolReferenceExpression().Symbol->IsLiftedOperatorMethod())
        {

            LiftedOperatorMethod * pLiftedOperator = pCall->Left->AsSymbolReferenceExpression().Symbol->PLiftedOperatorMethod();

            ILTree::ExpressionWithChildren * pLeftArg = &(pCall->Right->AsExpressionWithChildren());
            ThrowIfNull(pLeftArg);

            ILTree::ExpressionWithChildren * pRightArg = pLeftArg->Right ? &(pLeftArg->Right->AsExpressionWithChildren()) : NULL;

            ILTree::Expression * pLeftCondition = NULL;
            ILTree::Expression * pRightCondition = NULL;
            ILTree::Expression * pTmp = NULL;                  

            //ProcessNullableOperand is rather clever, in that it embeds the assignment to a temporary inside the
            //the condition that it returns. If we need to do lifting on both parameters we can just create an condition for
            //the IIF node that combines the two conditions returned from the 2 calls to process nullable operand
            //using an "and" node. Because "and" is not short circuting this will produce the correct behavior.

            m_semantics->ProcessNullableOperand(pLeftArg->Left, pLeftCondition, pTmp);
            pLeftArg->Left= pTmp;                            

            if (pRightArg)
            {
                m_semantics->ProcessNullableOperand(pRightArg->Left, pRightCondition, pTmp);
                pRightArg->Left = pTmp;                        
            }

            if (pLeftCondition && pRightCondition)
            {
                pTmp = 
                    GetSemantics()->AllocateExpression
                    (
                        SX_AND, 
                        GetSemantics()->GetFXSymbolProvider()->GetBooleanType(), 
                        pLeftCondition, 
                        pRightCondition, 
                        pCall->Loc
                    );
            }
            else 
            {
                pTmp = pLeftCondition ? pLeftCondition : pRightCondition;
            }

            Type * resultType = pCall->ResultType;

            pCall->Left = 
                GetSemantics()->ReferToSymbol
                (
                    pCall->Loc, 
                    pLiftedOperator->GetActualProc(), 
                    chType_NONE, 
                    NULL, 
                    pCall->Left->AsSymbolReferenceExpression().GenericBindingContext,
                    ExprIsExplicitCallTarget
                );

            pCall->ResultType = m_semantics->TypeInGenericContext(
                    ViewAsProcedure(pCall->Left->AsSymbolReferenceExpression().Symbol)->GetType(), 
                    pCall->Left->AsSymbolReferenceExpression().GenericBindingContext);

            *ppExpr = 
                m_semantics->AllocateIIfExpression 
                (
                    resultType,
                    pTmp,
                    TypeHelpers::IsNullableType(pCall->ResultType, GetSemantics()->GetCompilerHost()) ? 
                    pCall : 
                        m_semantics->WrapInNullable(resultType, pCall),
                    m_semantics->CreateNullValue(resultType, pCall->Loc),
                    pCall->Loc
                );

            SetFlag32(pCall->Left, SXF_SYM_NONVIRT);
        }
        else if ( m_pEnclosingProc != NULL && !m_pEnclosingProc->JITOptimizationsMustBeDisabled())
        {
            // Dev10 #850039 Check if we must disable inlining and optimization for the enclosing proc.
            BCSYM_NamedRoot * pSymbol = pCall->Left->AsSymbolReferenceExpression().Symbol;
            
            if (pSymbol == m_CompilerHost->GetSymbolForRuntimeMember(EndStatementMember, m_semantics->m_CompilationCaches))
            {
                // Risky function Microsoft.VisualBasic.CompilerServices.ProjectData.EndApp is used.
                m_pEnclosingProc->SetJITOptimizationsMustBeDisabled(true);
            }
            else
            {
                BCSYM_NamedRoot * pParent = pSymbol->GetParent();
                
                if (pParent != NULL)
                {
                    bool fDisable = false;
                
                    if (pParent == m_CompilerHost->GetSymbolForRuntimeClass(VBFileSystem, m_semantics->m_CompilationCaches))
                    {   
                        if (m_Compiler->GetNamesOfRiskyMSVBFSFunctions().Contains(pSymbol->GetName()))
                        {
                            // Risky function from Microsoft.VisualBasic.FileSystem is used.
                            fDisable = true;
                        }
                    }
                    else if (pParent == m_CompilerHost->GetSymbolForRuntimeClass(VBApplicationBase, m_semantics->m_CompilationCaches))
                    {
                        if (pSymbol->GetName() == STRING_CONST(m_Compiler, MSVBASABInfo))
                        {
                            // Risky property Microsoft.VisualBasic.ApplicationServices.ApplicationBase.Info is used.
                            fDisable = true;
                        }
                    }
                    else if (pParent == m_CompilerHost->GetSymbolForRuntimeClass(VBWindowsFormsApplicationBase, m_semantics->m_CompilationCaches))
                    {
                        if (pSymbol->GetName() == STRING_CONST(m_Compiler, MSVBASWFABRun))
                        {
                            // Risky function Microsoft.VisualBasic.ApplicationServices.WindowsFormsApplicationBase.Run is used.
                            fDisable = true;
                        }
                    }
                    else if (pParent == m_CompilerHost->GetSymbolForRuntimeClass(VBErrObject, m_semantics->m_CompilationCaches))
                    {
                        if (pSymbol->GetName() == STRING_CONST(m_Compiler, MSVBERROBJRaise))
                        {
                            // Risky function Microsoft.VisualBasic.ErrObject.Raise is used.
                            fDisable = true;
                        }
                    }

                    if (fDisable)
                    {
                        m_pEnclosingProc->SetJITOptimizationsMustBeDisabled(true);
                    }
                }
            }
        }
    }          
}    

// Given a nullable operand, tries to remove nullable lifting if possible.
// Returns true if operand was lowered (lifting removed) and false otherwise.
bool LowerBoundTreeVisitor::OptimizeNullableOperand(_Inout_ ILTree::Expression *&Operand)
{
    AssertIfNull(Operand);
    AssertIfFalse(TypeHelpers::IsNullableType(Operand->ResultType, m_CompilerHost));

    // Skip all CType nodes which have nullable ResultType.
    ILTree::Expression *Argument = Operand;
    while (Argument->bilop == SX_CTYPE && TypeHelpers::IsNullableType(Argument->ResultType, m_CompilerHost))
    {
        Argument = Argument->AsExpressionWithChildren().Left;
    }

    // If what's left is not of nullable type, we can optimize.
    if (Argument != Operand && !TypeHelpers::IsNullableType(Argument->ResultType, m_CompilerHost))
    {
        if (Argument->bilop == SX_NOTHING)
        {
            // Nothing transforms to NULL without conversion as it will be thrown away.
            Operand = Argument;
        }
        else
        {
            Operand = m_semantics->ConvertWithErrorChecking
            (
                Argument,
                TypeHelpers::GetElementTypeOfNullable(Operand->ResultType, m_CompilerHost),
                ExprNoFlags
            );
        }
        return true;
    }

    return false;
}

// Transforms lifted intrinsic Is/IsNot operators into a set of lower-level instructions.
void LowerBoundTreeVisitor::VisitLiftedIsOrIsNotOperator(_Inout_ ILTree::Expression **ppExpr)
{
    AssertIfNull(ppExpr && *ppExpr);
    ILTree::Expression *expr = *ppExpr;

    AssertIfFalse(HasFlag32(expr, SXF_OP_LIFTED_NULLABLE));
    AssertIfFalse(expr->bilop == SX_IS || expr->bilop == SX_ISNOT);
    AssertIfFalse(TypeHelpers::IsBooleanType(expr->ResultType));

    ILTree::Expression *&Left = expr->AsBinaryExpression().Left;
    ILTree::Expression *&Right = expr->AsBinaryExpression().Right;

    // One and only one of the operands has to be Nothing.
    AssertIfTrue(IsNothingLiteral(Left) && IsNothingLiteral(Right));
    AssertIfFalse(IsNothingLiteral(Left) || IsNothingLiteral(Right));

    ILTree::Expression *Operand = IsNothingLiteral(Left) ? Right : Left;
    AssertIfFalse(TypeHelpers::IsNullableType(Operand->ResultType, m_CompilerHost));

    // Try to optimize nullable away from the operand.
    if (OptimizeNullableOperand(Operand))
    {
        // If operand is optimized, the result of SX_IS is True only if Operand is SX_NOTHING.
        // The result of SX_ISNOT is True only if Operand is NOT SX_NOTHING.
        bool ResultIsTrue = 
            (expr->bilop == SX_IS && Operand->bilop == SX_NOTHING) ||
            (expr->bilop == SX_ISNOT && Operand->bilop != SX_NOTHING);

        ILTree::Expression *Result = m_semantics->ProduceConstantExpression
        (
            ResultIsTrue ? COMPLUS_TRUE : COMPLUS_FALSE,
            expr->Loc,
            m_semantics->GetFXSymbolProvider()->GetBooleanType()
            IDE_ARG(0)
        );

        if (Operand->bilop == SX_NOTHING || IsConstant(Operand))
        {
            // Lower "NULL Is/IsNot NULL to "True/False".
            *ppExpr = Result;
        }
        else
        {
            // Lower "CType(Operand As S, T?) Is/IsNot Nothing" to "temp = Operand, False/True".
            Variable *Temp = m_semantics->AllocateLifetimeNoneTemporary(Operand->ResultType, &Operand->Loc);
            *ppExpr = m_semantics->AllocateExpression
            (
                SX_SEQ,
                m_semantics->GetFXSymbolProvider()->GetBooleanType(),
                m_semantics->AllocateExpression
                (
                    SX_ASG,
                    TypeHelpers::GetVoidType(),
                    m_semantics->AllocateSymbolReference
                    (
                        Temp, 
                        Operand->ResultType, 
                        NULL, 
                        Operand->Loc, 
                        NULL
                    ),
                    Operand,
                    Operand->Loc
                ),
                Result,
                expr->Loc
            );
        }
    }
    else
    {
        // Lower "Operand Is/IsNot Nothing" to "[Not] Operand.HasValue".
        ILTree::Expression *Condition = m_semantics->CreateNullableHasValueCall(Operand->ResultType, Operand);
        *ppExpr = (expr->bilop == SX_IS) ? m_semantics->NegateBooleanExpression(Condition) : Condition;
    }
}

// Transforms lifted intrinsic unary operator into a set of lower-level instructions.
void LowerBoundTreeVisitor::VisitLiftedIntrinsicUnaryOperator(_Inout_ ILTree::Expression **ppExpr)
{
    AssertIfNull(ppExpr && *ppExpr);
    ILTree::Expression *expr = *ppExpr;

    AssertIfFalse(HasFlag32(expr, SXF_OP_LIFTED_NULLABLE));
    AssertIfFalse(TypeHelpers::IsNullableType(expr->ResultType, m_CompilerHost));

    Type *ResultType = expr->ResultType;
    ILTree::Expression *&Operand = expr->AsExpressionWithChildren().Left;
    AssertIfFalse(BCSYM::AreTypesEqual(ResultType, Operand->ResultType));

    // Lower original expression node to its non-nullable version.
    ClearFlag32(expr, SXF_OP_LIFTED_NULLABLE);
    SetResultType(expr, TypeHelpers::GetElementTypeOfNullable(ResultType, m_CompilerHost));

    // Try to optimize nullable away from the operand.
    if (OptimizeNullableOperand(Operand))
    {
        if (Operand->bilop == SX_NOTHING)
        {
            // Lower "<op> NULL" to "NULL".
            *ppExpr = m_semantics->CreateNullValue(ResultType, expr->Loc);
        }
        else
        {
            // Lower "<op> CType(Operand As S, T?)" to "CType(<op> Operand As S, T?)".
            *ppExpr = m_semantics->WrapInNullable(ResultType, expr);
        }
    }
    else
    {
        // Capture operand in a local variable.
        ILTree::Expression *Capture, *VariableRef;
        m_semantics->CaptureNullableOperand(Operand, Capture, VariableRef);

        // Lower the operand to "Operand.Value".
        Operand = m_semantics->CreateNullableValueCall(ResultType, VariableRef);

        // Lower "<op> Operand" to "IIf(Operand.HasValue, <op> Operand.Value, NULL)".
        *ppExpr = m_semantics->AllocateIIfExpression
        (
            ResultType,
            m_semantics->CreateNullableHasValueCall(ResultType, Capture),
            m_semantics->WrapInNullable(ResultType, expr),
            m_semantics->CreateNullValue(ResultType, expr->Loc),
            expr->Loc
        );
    }
}

void LowerBoundTreeVisitor::VisitLiftedIntrinsicBinaryOperator(_Inout_ ILTree::Expression **ppExpr)
{
    AssertIfNull(ppExpr && *ppExpr);
    ILTree::Expression *expr = *ppExpr;

    AssertIfFalse(HasFlag32(expr, SXF_OP_LIFTED_NULLABLE));
    AssertIfFalse(TypeHelpers::IsNullableType(expr->ResultType, m_CompilerHost));

    // Fetch information about the expression.
    Type *ResultType = expr->ResultType;
    Type *ElementType = TypeHelpers::GetElementTypeOfNullable(ResultType, m_CompilerHost);
    Location &Location = expr->Loc;
    ILTree::Expression *&Left = expr->AsBinaryExpression().Left;
    ILTree::Expression *&Right = expr->AsBinaryExpression().Right;

    // Try to optimize Nullable away from the operands.
    bool IsLeftOptimized = OptimizeNullableOperand(Left);
    bool IsRightOptimized = OptimizeNullableOperand(Right);

    // Check if Boolean operator semantics is applicable.
    bool IsShortCircuited = expr->bilop == SX_ORELSE || expr->bilop == SX_ANDALSO;
    bool IsOrOperator = expr->bilop == SX_OR || expr->bilop == SX_ORELSE;
    bool UseBooleanSemantics = TypeHelpers::IsBooleanType(ElementType) && 
        (IsShortCircuited || expr->bilop == SX_OR || expr->bilop == SX_AND);

    ILTree::Expression *Prefixes[2] = { NULL, NULL }; // set if SX_SEQ is needed at the end.

    if (Left->bilop == SX_NOTHING && Right->bilop == SX_NOTHING)
    {
        // Lower "NULL <op> NULL" to "NULL".
        *ppExpr = m_semantics->CreateNullValue(ResultType, Location);
    }
    else if (Left->bilop == SX_NOTHING || Right->bilop == SX_NOTHING)
    {
        // Get rid of the NULL operand.
        ILTree::Expression *Operand = Left->bilop == SX_NOTHING ? Right : Left;
        if (UseBooleanSemantics)
        {
            if (TypeHelpers::IsNullableType(Operand->ResultType, m_CompilerHost))
            {
                // For nullable operand:
                //   lower "Operand And NULL" to "IIf(Operand.HasValue AndAlso Not Operand.Value, False, NULL)".
                //   lower "Operand Or NULL" to "IIf(Operand.HasValue AndAlso Operand.Value, True, NULL)".
                ILTree::Expression *Capture, *VariableRef;
                m_semantics->CaptureNullableOperand(Operand, Capture, VariableRef);

                ILTree::Expression *ValueCall = m_semantics->CreateNullableValueCall(ResultType, VariableRef);
                *ppExpr = m_semantics->AllocateIIfExpression
                (
                    ResultType,
                    m_semantics->AllocateExpression
                    (
                        SX_ANDALSO,
                        m_semantics->GetFXSymbolProvider()->GetBooleanType(),
                        m_semantics->CreateNullableHasValueCall(ResultType, Capture),
                        IsOrOperator ? ValueCall : m_semantics->NegateBooleanExpression(ValueCall),
                        Operand->Loc
                    ),
                    m_semantics->WrapInNullable(IsOrOperator, Location),
                    m_semantics->CreateNullValue(ResultType, Location),
                    Location
                );
            }
            else
            {
                // For non-nullable operand:
                //   lower "Operand And NULL" to "IIf(Operand, NULL, False)".
                //   lower "Operand Or NULL" to "IIf(Operand, True, NULL)".
                *ppExpr = m_semantics->AllocateIIfExpression
                (
                    ResultType,
                    Operand,
                    IsOrOperator ?
                        m_semantics->WrapInNullable(true, Location) :
                        m_semantics->CreateNullValue(ResultType, Location),
                    IsOrOperator ?
                        m_semantics->CreateNullValue(ResultType, Location) :
                        m_semantics->WrapInNullable(false, Location),
                    Location
                );
            }
        }
        else
        {
            // Lower "Constant <op> NULL" to "NULL".
            *ppExpr = m_semantics->CreateNullValue(ResultType, Location);
            if (!IsConstant(Operand))
            {
                Variable *Temp = m_semantics->AllocateLifetimeNoneTemporary(Operand->ResultType, &Operand->Loc);
                Prefixes[0] = m_semantics->AllocateExpression
                (
                    SX_ASG,
                    TypeHelpers::GetVoidType(),
                    m_semantics->AllocateSymbolReference
                    (
                        Temp, 
                        Operand->ResultType, 
                        NULL, 
                        Operand->Loc, 
                        NULL
                    ),
                    Operand,
                    Operand->Loc
                );
            }
        }
    }
    else if (IsLeftOptimized && IsRightOptimized)
    {
        AssertIfTrue(Left->bilop == SX_NOTHING || Right->bilop == SX_NOTHING);

        // Lower original expression node to its non-nullable version.
        ClearFlag32(expr, SXF_OP_LIFTED_NULLABLE);
        SetResultType(expr, ElementType);

        // Lower "CType(Left, T?) <op> CType(Right, T?)" to "CType(Left <op> Right, T?)".
        *ppExpr = m_semantics->WrapInNullable(ResultType, expr);
    }
    else if (UseBooleanSemantics)
    {
        AssertIfTrue(Left->bilop == SX_NOTHING || Right->bilop == SX_NOTHING);
        AssertIfTrue(IsLeftOptimized && IsRightOptimized);

        // Capture left operand if it's not optimized (otherwise it's always evaluated).
        Variable *LeftVariable = NULL;
        ILTree::Expression *LeftCapture = NULL;
        if (!IsLeftOptimized)
        {
            m_semantics->CaptureNullableOperand(Left, LeftCapture, Left, &LeftVariable);
        }

        // Capture right operand if it's not optimized and we are not short-circuiting
        // on optimized left operand (in which case right operand is used only once).
        ILTree::Expression *RightCapture = NULL;
        if (!IsRightOptimized && !(IsLeftOptimized && IsShortCircuited))
        {
            m_semantics->CaptureNullableOperand(Right, RightCapture, Right);
        }

        ILTree::Expression *OperandValue = IsLeftOptimized ? Right : Left;
        ILTree::Expression *ConstValue = m_semantics->WrapInNullable(IsOrOperator, Location);
        *ppExpr = m_semantics->AllocateIIfExpression
        (
            ResultType,
            IsLeftOptimized ? 
                Left : 
                (IsRightOptimized ? 
                    Right : 
                    m_semantics->CreateNullableValueCall(ResultType, Right)),
            IsOrOperator ? 
                ConstValue : 
                OperandValue,
            IsOrOperator ? 
                OperandValue : 
                ConstValue,
            Location
        );

        if (!IsLeftOptimized)
        {
            if (!IsRightOptimized)
            {
                *ppExpr = m_semantics->AllocateIIfExpression 
                (
                    ResultType,
                    m_semantics->CreateNullableHasValueCall
                    (
                        ResultType, 
                        IsShortCircuited ? RightCapture : Right
                    ),
                    *ppExpr,
                    m_semantics->CreateNullValue(ResultType, Location),
                    Location
                );
                if (IsShortCircuited)
                {
                    RightCapture = NULL;
                }
            }
            if (!IsRightOptimized || IsShortCircuited)
            {
                AssertIfNull(LeftVariable);
                ILTree::Expression *ValueCall = m_semantics->CreateNullableValueCall
                (
                    ResultType,
                    m_semantics->AllocateSymbolReference
                    (
                        LeftVariable,
                        Left->ResultType,
                        NULL, 
                        Left->Loc, 
                        NULL
                    )
                );

                ILTree::Expression *LeftHasValueRef;
                if (RightCapture)
                {
                    LeftHasValueRef = m_semantics->AllocateSymbolReference
                    (
                        LeftVariable,
                        Left->ResultType,
                        NULL,
                        Left->Loc,
                        NULL
                    );
                }
                else
                {
                    LeftHasValueRef = LeftCapture;
                    LeftCapture = NULL;
                }

                *ppExpr = m_semantics->AllocateIIfExpression
                (
                    ResultType,
                    m_semantics->AllocateExpression 
                    (
                        SX_ANDALSO,
                        m_semantics->GetFXSymbolProvider()->GetBooleanType(),
                        m_semantics->CreateNullableHasValueCall(ResultType, LeftHasValueRef),
                        IsOrOperator ? ValueCall : m_semantics->NegateBooleanExpression(ValueCall),
                        Left->Loc
                    ),
                    m_semantics->WrapInNullable(IsOrOperator, Location),
                    *ppExpr,
                    Location
                );
            }
        }

        if (LeftCapture && RightCapture)
        {
            Prefixes[0] = LeftCapture;
            Prefixes[1] = RightCapture;
        }
        else
        {
            Prefixes[0] = LeftCapture ? LeftCapture : RightCapture;
        }
    }
    else
    {
        AssertIfTrue(UseBooleanSemantics);
        AssertIfTrue(Left->bilop == SX_NOTHING || Right->bilop == SX_NOTHING);
        AssertIfTrue(IsLeftOptimized && IsRightOptimized);

        // Capture both operands in temps.
        ILTree::Expression *LeftCapture, *RightCapture;
        m_semantics->CaptureNullableOperand(Left, LeftCapture, Left);
        m_semantics->CaptureNullableOperand(Right, RightCapture, Right);

        ILTree::Expression *Condition;
        if (IsLeftOptimized)
        {
            // Create Right.HasValue and store left capture to be prepended later.
            Condition = m_semantics->CreateNullableHasValueCall(Right->ResultType, RightCapture);
            Prefixes[0] = LeftCapture;
        }
        else if (IsRightOptimized)
        {
            // Create Left.HasValue and store right capture to be prepended later.
            Condition = m_semantics->CreateNullableHasValueCall(Left->ResultType, LeftCapture);
            Prefixes[0] = RightCapture;
        }
        else
        {
            // Create "(temp1 = Left).HasValue And (temp2 = Right).HasValue".
            Condition = m_semantics->AllocateExpression
            (
                SX_AND,
                m_semantics->GetFXSymbolProvider()->GetBooleanType(),
                m_semantics->CreateNullableHasValueCall(Left->ResultType, LeftCapture),
                m_semantics->CreateNullableHasValueCall(Right->ResultType, RightCapture),
                Location
            );
        }

        // Lower original expression node to its non-nullable version.
        ClearFlag32(expr, SXF_OP_LIFTED_NULLABLE);
        SetResultType(expr, ElementType);
        if (!IsLeftOptimized)
        {
            Left = m_semantics->CreateNullableValueCall(Left->ResultType, Left);
        }
        if (!IsRightOptimized)
        {
            Right = m_semantics->CreateNullableValueCall(Right->ResultType, Right);
        }

        // Lower "Left <op> Right" to  "IIf(Left.HasValue And Right.HasValue, Left.Value <op> Right.Value, NULL)".
        *ppExpr = m_semantics->AllocateIIfExpression
        (
            ResultType,
            Condition,
            m_semantics->WrapInNullable(ResultType, expr),
            m_semantics->CreateNullValue(ResultType, Location),
            Location
        );
    }

    for (int i = DIM(Prefixes) - 1; i >= 0; i--)
    {
        ILTree::Expression *Prefix = Prefixes[i];
        if (Prefix && (Prefix->bilop == SX_ASG || Prefix->bilop == SX_ASG_RESADR))
        {
            SetResultType(Prefix, TypeHelpers::GetVoidType());
            *ppExpr = m_semantics->AllocateExpression
            (
                SX_SEQ,
                ResultType,
                Prefix,
                *ppExpr,    
                Location
            );
        }
    }
}

//==============================================================================
// End LowerBoundTreeVisitor
//==============================================================================

//==============================================================================
// Start DeferredTempVisitor
//==============================================================================  

DeferredTempIterator::DeferredTempIterator(_In_ Semantics *semantics) :
    m_semantics(semantics),
    m_Vars(NULL),
    m_InitStmts(NULL),
    m_LastInitStmt(NULL)
{
    ThrowIfNull(semantics);
}

DeferredTempIterator::~DeferredTempIterator()
{
}

void DeferredTempIterator::Iterate()
{
    BoundTreeVisitor::Visit(m_semantics->m_ProcedureTree);

    LinkLongLivedTemps();
}

void DeferredTempIterator::Iterate( _Inout_ ILTree::Expression** ppExpr )
{
    BoundTreeVisitor::VisitExpression(ppExpr);
    LinkTempExpressions( ppExpr );
}

bool DeferredTempIterator::StartExpression(_Inout_ ILTree::Expression **ppExpr)
{
    ThrowIfNull(ppExpr);
    ThrowIfNull(*ppExpr);

    if (IsBad(*ppExpr))
    {
        return false;
    }
        
    ILTree::Expression *expr = *ppExpr;
    switch (expr->bilop)
    {
        case SX_DEFERRED_TEMP:
            *ppExpr = GetTempForDeferredTemp(&(*ppExpr)->AsDeferredTempExpression());
            break;
    }

    return true;
}

void  DeferredTempIterator::EndExpression(ILTree::Expression **)
{
    // During processing in the iterator, all SX_DEFERRED_TEMPs should be removed.
    // If the method still has deferred temps then they were created during iterate.
    // This is a bug in the iterate code. 

    ThrowIfTrue(m_semantics->m_methodDeferredTempCount != 0);
}


ILTree::Statement *
DeferredTempIterator::InitTemp(Variable *var, ILTree::Expression *value)
{
    AssertIfNull(var);
    AssertIfNull(value);

    Location hiddenLoc = Location::GetHiddenLocation();

    ILTree::SymbolReferenceExpression *target =
        m_semantics->AllocateSymbolReference(
            var,
            var->GetType(),
            NULL,
            hiddenLoc);

    ILTree::ExpressionWithChildren *initExpr =
        (ILTree::ExpressionWithChildren *)m_semantics->AllocateExpression(
            SX_ASG,
            TypeHelpers::GetVoidType(),
            target,
            value,
            hiddenLoc);

    ILTree::Statement *initStmt = m_semantics->AllocateStatement(SL_STMT, hiddenLoc, NoResume, false);
    initStmt->AsStatementWithExpression().Operand1 = initExpr;
    initStmt->Prev = m_InitStmts;
    if (m_InitStmts)
    {
        m_InitStmts->Next = initStmt;
    }
    m_InitStmts = initStmt;

    return initStmt;
}

void
DeferredTempIterator::LinkLongLivedTemps()
{
    if (m_InitStmts)
    {
        // Backup to the first xml local initialization statement
        while (m_InitStmts->Prev)
        {
            m_InitStmts = m_InitStmts->Prev;
        }

        // Save the current statement target and last statement in context used by LinkStatement
        ILTree::Statement **backupStatementTarget = m_semantics->m_StatementTarget;
        ILTree::Statement *backupLastStatementInContext = m_semantics->m_LastStatementInContext;
        ILTree::Statement *statementAfterLocals = NULL;

        if (m_LastInitStmt)
        {
            statementAfterLocals = m_LastInitStmt->Next;
            m_semantics->m_StatementTarget = &m_LastInitStmt->Next;
        }
        else
        {
            statementAfterLocals = m_semantics->m_ProcedureTree->Child;
            m_semantics->m_StatementTarget = &m_semantics->m_ProcedureTree->Child;
        }

        // Link the xml local statements into the procedure
        while (m_InitStmts)
        {
            ILTree::Statement *next = m_InitStmts->Next;
            m_semantics->LinkStatement(m_InitStmts);

            m_LastInitStmt = m_InitStmts;
            m_InitStmts = next;
        }

        // If the xml locals were inserted in front of some existing statements then 
        // restore the statement target and last statement in  context.  Otherwise, these
        // are already set correctly by LinkStatement.

        if (statementAfterLocals)
        {
            m_semantics->m_StatementTarget = backupStatementTarget;
            m_semantics->m_LastStatementInContext = backupLastStatementInContext;
            m_LastInitStmt->Next = statementAfterLocals;
            statementAfterLocals->Prev = m_LastInitStmt;
        }

        m_InitStmts = NULL;
    }
}

//-------------------------------------------------------------------------------------------------
//
// Link the temporary initialization in the context of an expression
//
//-------------------------------------------------------------------------------------------------
void 
DeferredTempIterator::LinkTempExpressions( _Inout_ ILTree::Expression** ppExpr )
{
    ThrowIfNull(ppExpr);

    ILTree::Expression *pOriginal = *ppExpr;
    ILTree::Expression *pReturn = pOriginal; 
    ILTree::Statement *pStatement = m_InitStmts;
    
    if (pStatement == NULL)
    {
        return;
        // Dev10#511202: if tried to iterate over deferred temp expressions, but there were none,
        // then m_InitStmts will be NULL. Another case to consider is where there were deferred temp
        // expressions, but the DeferredTempIterator didn't spot them because it encountered a bad
        // node too early on, and so it left m_InitStmts null.
    }

    while (pStatement->Next) 
    {
        pStatement = pStatement->Next;
    }

    while ( pStatement )
    {
        ILTree::Expression* pCur = m_semantics->AllocateExpression(
            SX_SEQ,
            pOriginal->ResultType,
            pStatement->AsStatementWithExpression().Operand1,
            pReturn,
            Location::GetHiddenLocation());
        pReturn = pCur;
        pStatement = pStatement->Prev;
    }

    *ppExpr = pReturn;
}

ILTree::Expression *
DeferredTempIterator::GetTempForDeferredTemp(_In_ ILTree::DeferredTempExpression *deferredTemp)
{
    Variable *var = NULL;

    if (!m_Vars)
    {
        m_Vars = new (m_semantics->m_TreeStorage) DynamicHashTable<size_t,BCSYM_Variable *,NorlsAllocWrapper>(&m_semantics->m_TreeStorage);
    }

    if (!m_Vars->GetValue(deferredTemp->Id, &var))
    {
        var = m_semantics->m_ProcedureTree
            ? m_semantics->AllocateLongLivedTemporary(deferredTemp->ResultType, &Location::GetHiddenLocation(), m_semantics->m_ProcedureTree)
            : m_semantics->AllocateShortLivedTemporary(deferredTemp->ResultType);
        m_Vars->SetValue(deferredTemp->Id, var);

        ILTree::Expression *value = m_semantics->InterpretExpression(deferredTemp->InitialValue,  deferredTemp->InterpretFlags);

        VisitExpression(&value);

        InitTemp(var, value);
    }

    return m_semantics->AllocateSymbolReference(var, var->GetType(), NULL, Location::GetHiddenLocation());
}

//==============================================================================
// End DeferredTempVisitor
//==============================================================================  

MultilineLambdaReturnExpressionVisitor::MultilineLambdaReturnExpressionVisitor
(
    ArrayList<ILTree::Expression*> *expressionList,
    Semantics *pSemantics,
    NorlsAllocator *pAllocator,
    NorlsAllocWrapper *pAllocWrapper
)
{
    ThrowIfNull(expressionList);
    ThrowIfNull(pSemantics);
    
    m_pLambdaBlock = NULL;
    m_expressionList = expressionList;
    m_pSemantics = pSemantics;
    m_VisitYields = false;
}

MultilineLambdaReturnExpressionVisitor::~MultilineLambdaReturnExpressionVisitor()
{
}

void MultilineLambdaReturnExpressionVisitor::VisitReturnExpressions(ILTree::StatementLambdaBlock *pLambdaBlock)
{
    ThrowIfNull(pLambdaBlock);
    m_pLambdaBlock = pLambdaBlock;
    m_VisitYields = false;
    VisitBlock(pLambdaBlock);
    m_pLambdaBlock = NULL;
}

void MultilineLambdaReturnExpressionVisitor::VisitYieldExpressions(ILTree::StatementLambdaBlock *pLambdaBlock)
{
    ThrowIfNull(pLambdaBlock);
    m_pLambdaBlock = pLambdaBlock;
    m_VisitYields = true;
    VisitBlock(pLambdaBlock);
    m_pLambdaBlock = NULL;
}

bool MultilineLambdaReturnExpressionVisitor::StartBlock( ILTree::ExecutableBlock *pBlock)
{
    return pBlock->bilop == SB_STATEMENT_LAMBDA ? (pBlock == m_pLambdaBlock) : true;
}

bool MultilineLambdaReturnExpressionVisitor::StartExpression(ILTree::Expression **pexpr)
{
    return false;
    // There's no need to visit expression-bound-trees here.
    // In any case, the BoundTreeVisitor asserts if it encounters e.g. AddressOf here,
    // so we couldn't walk that anyway.
}

void MultilineLambdaReturnExpressionVisitor::EndStatement(ILTree::Statement* pStmt)
{
    if (pStmt->bilop == SL_RETURN && !m_VisitYields)
    {
        if (pStmt->AsReturnStatement().ReturnExpression)
        {
            this->m_expressionList->Add(&pStmt->AsReturnStatement().ReturnExpression->AsExpression());
            // Note: some expressions (e.g. AddressOf) don't play a part in inferring the return type
            // of the lambda. That's fine. We add them all to expressionList anyway. The list will be
            // fed to InferDominantTypeOfExpressions, and that's where decisions are made as to which
            // expressions to ignore and which to use.
        }
    }
    else if (pStmt->bilop == SL_YIELD && m_VisitYields)
    {
        if (pStmt->AsYieldStatement().YieldExpression)
        {
            this->m_expressionList->Add(&pStmt->AsYieldStatement().YieldExpression->AsExpression());
        }
    }
}





// Dev10 #735384
// Check expressions for improper use of embedded types.
bool CheckUsageOfEmbeddedTypesVisitor::StartExpression(ILTree::Expression **ppExpr)
{
    
    if (IsBad(*ppExpr))
    {
        return false;
    }
        
    ILTree::Expression *pExpr = *ppExpr;

    switch ( pExpr->bilop )
    {
        case SX_SYM:
            {
                ILTree::SymbolReferenceExpression *pSymRef = &pExpr->AsSymbolReferenceExpression();
                BCSYM_NamedRoot * pSymbol = pSymRef->Symbol;

                if (pSymbol != NULL && !pSymbol->IsBad() && pSymbol->IsMember())
                {
                    CompilerProject * pImportedFromProject = pSymbol->GetContainingProject();

                    if ( pImportedFromProject!= NULL && pImportedFromProject != m_pProject)
                    {
                        BCSYM_Member * pMember = pSymbol->PMember();

                        BCSYM * pType = TypeHelpers::DescendToPossiblyEmbeddedType(pMember->GetRawType());

                        TypeHelpers::CheckTypeForUseOfEmbeddedTypes(
                            pType,            
                            m_pProject,
                            pImportedFromProject, 
                            m_pErrorTable,
                            &pSymRef->Loc);

                        // Dev10 #757245 - check container for shared members
                        if (pMember->IsShared())
                        {
                            BCSYM_Container * pContainer = pMember->GetContainer();

                            if (pContainer != NULL && !pContainer->IsBad() && pContainer->IsType())
                            {
                                pType = pContainer;
                                
                                if(pSymRef->GenericBindingContext != NULL)
                                {
                                    if (IsGenericOrHasGenericParent(pContainer))
                                    {
                                        if (!pSymRef->GenericBindingContext->IsGenericTypeBinding())
                                        {
                                            if (pSymRef->GenericBindingContext->GetParentBinding() != NULL)
                                            {
                                                pType = pSymRef->GenericBindingContext->GetParentBinding();
                                            }
                                        }
                                        else 
                                        {
                                            pType = pSymRef->GenericBindingContext;
                                        }

                                        AssertIfFalse(pType == pContainer ||
                                            (pType->IsGenericTypeBinding() && pType->PGenericTypeBinding()->GetGenericType() == pContainer));
                                    }
                                }

                                TypeHelpers::CheckTypeForUseOfEmbeddedTypes(
                                    pType,            
                                    m_pProject,
                                    pImportedFromProject, 
                                    m_pErrorTable,
                                    &pSymRef->Loc);
                            }
                        }
                    }
                }
            }
            break;

        case SX_CALL:
            {
                ILTree::CallExpression * pCall = &pExpr->AsCallExpression();

                if (
                    pCall->Left != NULL && 
                    pCall->Left->bilop == SX_SYM 
                    )
                {
                    ILTree::SymbolReferenceExpression *pSymRef = &pCall->Left->AsSymbolReferenceExpression();
                    BCSYM_NamedRoot * pSymbol = pSymRef->Symbol;
                    
                    if (pSymbol != NULL && !pSymbol->IsBad() && pSymbol->IsProc())
                    {
                        CompilerProject * pImportedFromProject = pSymbol->GetContainingProject();

                        if ( pImportedFromProject!= NULL && pImportedFromProject != m_pProject)
                        {
                            TypeHelpers::CheckProcForUseOfEmbeddedTypes(
                                pSymbol->PProc(),            
                                m_pProject,
                                pImportedFromProject, 
                                m_pErrorTable,
                                &pSymRef->Loc);
                        }
                    }
                }
            }
            break;
            
    }

    return true;
}

VariableSubstitutionVisitor::VariableSubstitutionVisitor()
    : m_foundOldVar(false),
      m_SearchVarOnly(false)
{
}

void VariableSubstitutionVisitor::ReplaceReference(ILTree::Expression ** ppExp, Variable *OldVar, Variable *NewVar)
{
    m_OldVar = OldVar;
    m_NewVar = NewVar;
    m_SearchVarOnly = false;
    m_foundOldVar = false;

    VisitExpression(ppExp);
}

bool VariableSubstitutionVisitor::ContainsReference(ILTree::Expression ** ppExp, Variable *pVar)
{
    m_OldVar = pVar;
    m_SearchVarOnly = true;
    m_foundOldVar = false;

    VisitExpression(ppExp);

    return m_foundOldVar;

}

bool VariableSubstitutionVisitor::StartExpression(ILTree::Expression **ppExpr)
{
    if (IsBad(*ppExpr))
    {
        return false;
    }
        
    ILTree::Expression *expr = *ppExpr;

    if (expr->bilop == SX_SYM && 
        expr->AsSymbolReferenceExpression().Symbol->IsVariable() &&
        expr->AsSymbolReferenceExpression().Symbol->PVariable() == m_OldVar)
    {
        if (!m_SearchVarOnly)
        {
            expr->AsSymbolReferenceExpression().Symbol = m_NewVar;
        }
        m_foundOldVar = true;
    }
    
    return true;
}



