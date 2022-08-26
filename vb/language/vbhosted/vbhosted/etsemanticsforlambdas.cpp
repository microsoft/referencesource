//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Implementation of the ETSemanticsForLambdas class
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

DLRExpressionTree*
ETSemanticsForLambdas::ConvertConstantValue
(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
)
{
    // -----------------------------------Contract start------------------------------------

    ThrowIfNull(Input);

    ThrowIfFalse(Input->bilop == SX_NOTHING ||
                 Input->bilop == SX_CNS_INT ||
                 Input->bilop == SX_CNS_FLT ||
                 Input->bilop == SX_CNS_DEC ||
                 Input->bilop == SX_CNS_STR ||
                 Input->bilop == SX_METATYPE);

    // 




    // -----------------------------------Contract end------------------------------------

    return ConvertConstantValueHelper(
        Input,
        Flags,
        TargetType,
        m_ExprTreeGenerator,
        GetFXSymbolProvider()
        );
}

DLRExpressionTree*
ETSemanticsForLambdas::ConvertConstantValueHelper
(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType,
    DLRTreeETGenerator *ExprTreeGenerator,
    FX::FXSymbolProvider *FXSymbolProvider
)
{
    ThrowIfNull(Input);
    ThrowIfNull(ExprTreeGenerator);
    ThrowIfNull(FXSymbolProvider);

    DLRExpressionTree *Result = NULL;

    switch (Input->bilop)
    {
    case SX_NOTHING:

        Result = ExprTreeGenerator->CreateNothingConstantExpr(
            Input->ResultType,
            TargetType,
            Input->Loc
            );

        break;

    case SX_CNS_INT:

        if (TypeHelpers::IsIntegralType(Input->ResultType))
        {
            Result = ExprTreeGenerator->CreateIntegralConstantExpr(
                Input->AsIntegralConstantExpression().Value,
                Input->ResultType,
                TargetType,
                Input->Loc
                );
        }
        else if (TypeHelpers::IsBooleanType(Input->ResultType))
        {
            Result = ExprTreeGenerator->CreateBooleanConstantExpr(
                (bool)Input->AsIntegralConstantExpression().Value,
                TargetType,
                Input->Loc
                );

        }
        else if (TypeHelpers::IsCharType(Input->ResultType))
        {
            Result = ExprTreeGenerator->CreateCharConstantExpr(
                (WCHAR)Input->AsIntegralConstantExpression().Value,
                TargetType,
                Input->Loc
                );                
        }
        else if (TypeHelpers::IsDateType(Input->ResultType))
        {
            Result = ExprTreeGenerator->CreateDateConstantExpr(
                Input->AsIntegralConstantExpression().Value,
                TargetType,
                Input->Loc
                );
        }

        break;

    case SX_CNS_FLT:

        Result = ExprTreeGenerator->CreateFloatingPointConstantExpr(
            Input->AsFloatConstantExpression().Value,
            Input->ResultType,
            TargetType,
            Input->Loc
            );

        break;

    case SX_CNS_DEC:

        Result = ExprTreeGenerator->CreateDecimalConstantExpr(
            Input->AsDecimalConstantExpression().Value,
            TargetType,
            Input->Loc
            );

        break;

    case SX_CNS_STR:

        Result = ExprTreeGenerator->CreateStringConstantExpr(
            Input->AsStringConstant().Spelling,
            (unsigned int)Input->AsStringConstant().Length, //Cast on this line for 64bit compilation. Cast is safe because string will be truncated in the worst case. Only happens for 4GB+ strings.
            Input->ResultType,
            Input->Loc
            );

        break;

    case SX_METATYPE:

        VSASSERT(
            Input->AsExpressionWithChildren().Left->bilop == SX_SYM ||
                Input->AsExpressionWithChildren().Left->bilop == SX_NOTHING,
            "How did this metatype node get here?"
            );

        ILTree::Expression *SymbolRef = Input->AsExpressionWithChildren().Left;

        if (SymbolRef->bilop == SX_NOTHING)
        {
            // Type reference

            ThrowIfFalse(FXSymbolProvider->GetTypeType() == Input->ResultType);

            Result = ExprTreeGenerator->CreateTypeRefExpr(
                SymbolRef->ResultType,
                TargetType,
                Input->Loc
                );
        }
        else if (SymbolRef->AsSymbolReferenceExpression().Symbol->IsVariable())
        {
            // Field reference

            ThrowIfFalse(FXSymbolProvider->GetFieldInfoType() == Input->ResultType);

            Result = ExprTreeGenerator->CreateFieldRefExpr(
                SymbolRef->AsSymbolReferenceExpression().Symbol->PVariable(),
                SymbolRef->AsSymbolReferenceExpression().GenericBindingContext->PGenericTypeBinding(),
                TargetType,
                Input->Loc
                );
        }
        else
        {
            // Method reference

            ThrowIfFalse(FXSymbolProvider->GetMethodInfoType() == Input->ResultType ||
                         FXSymbolProvider->GetConstructorInfoType() == Input->ResultType);

            Result = ExprTreeGenerator->CreateMethodRefExpr(
                SymbolRef->AsSymbolReferenceExpression().Symbol->PProc(),
                SymbolRef->AsSymbolReferenceExpression().GenericBindingContext,
                TargetType,
                Input->Loc
                );
        }

        break;
    }

    if (Result == NULL)
    {
        ASSERT(false,"[ETSemanticsForLambdas::ConvertConstant] non-constant expression unexpected");
        ThrowIfFalse(false);
    }

    return Result;
}

DLRExpressionTree*
ETSemanticsForLambdas::ConvertSymbol
(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
)
{
    ThrowIfNull(Input);
    ThrowIfFalse(Input->bilop == SX_SYM);

    Declaration* Symbol = Input->AsSymbolReferenceExpression().pnamed;

    if(Symbol->IsVariable())
    {
        Variable *Var = Symbol->PVariable();

        // IsLocal check also required for Chimayo scenarios.
        if (Var->IsTemporary() || Var->IsFromScriptScope())
        {
            DLRExpressionTree **TreeNode = NULL;
            if((TreeNode = m_VariablesMap->HashFind(Var)) && *TreeNode)
            {
                return *TreeNode;
            }

            if (Var->IsFromScriptScope())
            {
                DLRExpressionTree *VarExprNode = m_ExprTreeGenerator->CreateVariableExpr(
                    Var->GetEmittedName(),
                    Var->GetType(),
                    TargetType,
                    Input->Loc
                    );

                // All variables need to be added to the map to ensure uniqueness.
                m_VariablesMap->HashAdd(Var, VarExprNode);

                return VarExprNode;
            }

            // Non-script scope variable should already exist from the outer block
            // since user lambdas in VB cannot result in creation of new locals.
            ThrowIfFalse(false);
        }
    }

    // fall back to the base implementation
    return ExpressionTreeSemantics<DLRExpressionTree>::ConvertSymbol(
        Input,
        Flags,
        TargetType
        );
}

DLRExpressionTree*
ETSemanticsForLambdas::ConvertLambda
(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
)
{
    ThrowIfNull(Input);
    ThrowIfFalse(Input->bilop == SX_LAMBDA);

    // Don't allow Sub lambdas to be converted to expression trees.
    if (Input->ResultType->IsDelegate())
    {
        BCSYM_NamedRoot* proc = GetInvokeFromDelegate(Input->ResultType, m_Compiler);
        if(proc && proc->PProc() && (!proc->PProc()->GetType() || TypeHelpers::IsVoidType(proc->PProc()->GetType())))
        {
            ReportErrorForExpressionTree( ERRID_StatementLambdaInExpressionTree, Input->Loc );
            return NULL;
        }
    }

    return ExpressionTreeSemantics<DLRExpressionTree>::ConvertLambda(Input, Flags, TargetType);
}
