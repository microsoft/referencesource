//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ILTreeETGenerator:
//      Expression tree factory class that specializes the base ExpressionTreeGenerator
// class. Using this specialization, the factory methods produce bound trees that
// represent the expression tree creation.
//
//-------------------------------------------------------------------------------------------------

#pragma once

#include "ExpressionTreeGenerator.h"

class ILTreeETGenerator :
    public ExpressionTreeGenerator<ILTree::Expression>,
    public ExprTreeGenRuntimeValuesSupport<ILTree::Expression>
{
public:

    ILTreeETGenerator(Semantics *SemanticsContext)
      : 
        ExpressionTreeGenerator(SemanticsContext->GetCompiler()),
        m_Semantics(SemanticsContext),
        m_SymbolCreator(SemanticsContext->m_SymbolCreator),
        m_PH(SemanticsContext->GetTreeStorage()),
        m_Allocator(SemanticsContext->GetTreeStorage())
    {
    }

    // Expression tree generation factory methods

    ILTree::Expression* CreateBadExpr
    (
        Type *ExprType,     // This is the result type of the expression for which the the bad expr node is being created
        Type *TargetType,   // This is the type of the resulting expr tree node
        Location& Loc
    );

    ILTree::Expression* CreateConvertExpr
    (
        ILTree::Expression *Operand,
        Type *TypeToConvertTo,
        bool UseCheckedSemantics,
        Type *TargetType,
        const Location &Loc
    );

    ILTree::Expression* CreateConvertExpr
    (
        ILTree::Expression *Operand,
        Type *TypeToConvertTo,
        bool UseCheckedSemantics,
        Procedure *ImplementingMethod,
        GenericBinding *MethodBindingContext,
        Type *TargetType,
        const Location &Loc
    );

    ILTree::Expression* CreateTypeAsExpr
    (
        ILTree::Expression *Operand,
        Type *TypeToConvertTo,
        Type *TargetType,
        const Location &Loc
    );

    ILTree::Expression* CreateCastExpr
    (
        ILTree::Expression *Operand,
        Type *TypeToConvertTo,
        Type *TargetType,
        const Location &Loc
    );

    ILTree::Expression* CreateArrayIndexExpr
    (
        ILTree::Expression *Array,
        ILTree::Expression *Index,
        Type *TargetType,
        const Location &Loc
    );

    ILTree::Expression* CreateArrayIndexExpr
    (
        ILTree::Expression *Array,
        IReadOnlyList<ILTree::Expression *> *Indices,
        Type *TargetType,
        const Location &Loc
    );

    ILTree::Expression* CreateArrayLengthExpr
    (
        ILTree::Expression *Array,
        Type *TargetType,
        const Location &Loc
    );

    ILTree::Expression* CreateNewArrayBoundsExpr
    (
        Type *ElementType,
        IReadOnlyList<ILTree::Expression *> *Bounds,
        Type *TargetType,
        const Location &Loc
    );

    // Create a 1-D array of the specified type and initialized with the specified elements

    ILTree::Expression* CreateNewArrayInitExpr
    (
        Type *ElementType,
        IReadOnlyList<ILTree::Expression *> *Elements,
        Type *TargetType,
        const Location &Loc
    );

    ILTree::Expression* CreateListInitExpr
    (
        ILTree::Expression *ListExpr,
        IReadOnlyList<ListElementInitInfo<ILTree::Expression>> *Initializers,
        Type *TargetType,
        const Location &Loc
    );

    ILTree::Expression* CreateMemberInitExpr
    (
        ILTree::Expression *NewExpr,
        IReadOnlyList<MemberAssignmentInfo<ILTree::Expression>> *Initializers,
        Type *TargetType,
        const Location &Loc
    );

    ILTree::Expression *CreateFieldExpr
    (
        BCSYM_Variable *Field,
        GenericTypeBinding *GenericBindingContext,
        ILTree::Expression *Instance,
        Type *TargetType,
        const Location &Loc
    );

    ILTree::Expression* CreateCallExpr
    (
        Procedure *Proc,
        GenericBinding *GenericBindingContext,
        ILTree::Expression *Instance,
        IReadOnlyList<ILTree::Expression *> *Arguments,
        Type *TargetType,
        const Location &Loc
    );

    ILTree::Expression* CreateInvokeExpr
    (
        ILTree::Expression *DelegateInstance,
        IReadOnlyList<ILTree::Expression *> *Arguments,
        Type *TargetType,
        const Location &Loc
    );

    ILTree::Expression* CreatePropertyAccessExpr
    (
        Procedure *PropertyGet,
        GenericBinding *GenericBindingContext,
        ILTree::Expression *Instance,
        Type *TargetType,
        const Location &Loc
    );

    ILTree::Expression* CreateTypeIsExpr
    (
        ILTree::Expression *ExprOperand,
        Type *TypeOperand,
        Type *TargetType,
        const Location &Loc
    );

    ILTree::Expression* CreateNegateExpr
    (
        ILTree::Expression *Operand,
        Type *TargetType,
        const Location &Loc
    );

    ILTree::Expression* CreateUnaryOperatorExpr
    (
        BILOP Operator,
        ILTree::Expression *Operand,
        bool UseCheckedSemantics,
        Type *TargetType,
        const Location &Loc
    );

    ILTree::Expression* CreateUnaryOperatorExpr
    (
        BILOP Operator,
        ILTree::Expression *Operand,
        bool UseCheckedSemantics,
        Procedure *ImplementingMethod,
        GenericBinding *MethodBindingContext,
        Type *TargetType,
        const Location &Loc
    );

    ILTree::Expression* CreateBinaryOperatorExpr
    (
        BILOP Operator,
        ILTree::Expression *LeftOperand,
        ILTree::Expression *RightOperand,
        bool UseCheckedSemantics,
        Type *TargetType,
        const Location &Loc
    );

    ILTree::Expression* CreateBinaryOperatorExpr
    (
        BILOP Operator,
        ILTree::Expression *LeftOperand,
        ILTree::Expression *RightOperand,
        bool UseCheckedSemantics,
        bool LiftToNull,
        Procedure *ImplementingMethod,
        GenericBinding *MethodBindingContext,
        Type *TargetType,
        const Location &Loc
    );

    ILTree::Expression* CreateTernaryIfExpr
    (
        ILTree::Expression *Condition,
        ILTree::Expression *TrueValue,
        ILTree::Expression *FalseValue,
        Type *TargetType,
        const Location &Loc
    );

    // ----------------------------------------------------------------------------
    // This method returns an expression to create a coalesce expression parameter.
    // ----------------------------------------------------------------------------

    ILTree::Expression *CreateCoalesceExpr
    (
        ILTree::Expression *LeftOperand,
        ILTree::Expression *RightOperand,
        Type *TargetType,
        const Location &Loc
    );

    // ----------------------------------------------------------------------------
    // This method returns an expression to create a coalesce expression parameter.
    // But along with the normal operands, it also accepts a lambda to perform
    // a conversion from the LeftOperand type.
    // ----------------------------------------------------------------------------

    ILTree::Expression *CreateCoalesceExpr
    (
        ILTree::Expression *LeftOperand,
        ILTree::Expression *RightOperand,
        ILTree::Expression *ConversionLambda,
        Type *TargetType,
        const Location &Loc
    );

    // ----------------------------------------------------------------------------
    // This method returns a LambdaParamHandle which encapsulates both the
    // temporary generated for the lambda parameter as well as the lambda parameter
    // creation expression.
    // ----------------------------------------------------------------------------

    LambdaParamHandle* CreateLambdaParameter
    (
        _In_z_ STRING *ParameterName,
        Type *ParameterType,
        const Location &Loc
    );

    // ----------------------------------------------------------------------------
    // This method returns an expression that loads a previously created lambda
    // parameter expression from a temporary corresponding to that lambda parameter.
    // ----------------------------------------------------------------------------

    ILTree::Expression* CreateLambdaParamReferenceExpr
    (
        LambdaParamHandle *LambdaParameter,
        const Location &Loc
    );

    // ----------------------------------------------------------------------------
    // This method returns an expression that creates a lambda expression.
    // ----------------------------------------------------------------------------

    ILTree::Expression* CreateLambdaExpr
    (
        IReadOnlyList<LambdaParamHandle *> *Parameters,
        ILTree::Expression *LambdaBody,
        Type *DelegateType,
        Type *TargetType,
        const Location &Loc
    );

    ILTree::Expression* CreateQuoteExpr
    (
        ILTree::Expression *Operand,
        Type *TargetType,
        const Location &Loc
    );

    ILTree::Expression* CreateNewExpr
    (
        Type *InstanceType,
        Type *TargetType,
        const Location &Loc
    );

    ILTree::Expression* CreateNewExpr
    (
        Procedure *Constructor,
        GenericTypeBinding *GenericTypeBindingContext,
        IReadOnlyList<ILTree::Expression *> *Arguments,
        Type *TargetType,
        const Location &Loc
    );

    ILTree::Expression* CreateNewExpr
    (
        Procedure *Constructor,
        GenericTypeBinding *GenericTypeBindingContext,
        IReadOnlyList<PropertyAssignmentInfo<ILTree::Expression>> *Initializers,
        Type *TargetType,
        const Location &Loc
    );

    ILTree::Expression* CreateNothingConstantExpr
    (
        Type *NothingType,
        Type *TargetType,
        const Location &Loc
    );

    ILTree::Expression* CaptureRuntimeValueAsConstant
    (
        ILTree::Expression *Operand,
        Type *TargetType
    );

private:

    // internal helpers

    // ----------------------------------------------------------------------------
    // This method takes 2 expression and appends them so that first expression is
    // executed first followed by the second expression. The value of the resulting
    // expression is the value of the second expression.
    // ----------------------------------------------------------------------------

    ILTree::Expression* BuildExpressionSequence
    (
        ILTree::Expression *Expr1,
        ILTree::Expression *Expr2,
        const Location &Loc
    );

    ILTree::Expression* CreateElementInitExpr
    (
        Procedure *Proc,
        GenericBinding *GenericBindingContext,
        IReadOnlyList<ILTree::Expression *> *Arguments,
        Type *TargetType,
        const Location &Loc
    );

    ILTree::Expression* CreateTypeRef
    (
        Type *Type,
        const Location &Loc
    );

    ILTree::Expression* CreateMethodRef
    (
        Procedure *Method,
        GenericBinding *GenericBindingContext,
        const Location &Loc
    );

    ILTree::Expression* CreateFieldRef
    (
        Variable *Field,
        GenericBinding *GenericBindingContext,
        const Location &Loc
    );


    // ----------------------------------------------------------------------------
    // Given a list of bound expressions, builds an unbound array that
    // holds these expressions.
    //
    // If there are any elements, then the type of resulting expression
    // is 1-D array of ArrayElementType type, else the resulting expression
    // is of EmptyArrayExprType type.
    // ----------------------------------------------------------------------------

    ParseTree::Expression *BuildExpressionArray
    (
        Type *ArrayElementType,
        IReadOnlyList<ILTree::Expression *> *Elements,
        const Location &Loc,
        Type *EmptyArrayExprType = NULL
    );

    // ----------------------------------------------------------------------------
    // Generate the call to the expression tree given the information on the
    // expression tree factory method.
    // Initialization is used to initialize the lambda temporary parameters.
    // ----------------------------------------------------------------------------

    ILTree::Expression* GenerateExpressionTreeCall
    (
        _In_z_ STRING* MethodName,
        ParseTree::ArgumentList* Arguments,
        Type* TargetType,
        ParseTree::TypeList* TypeArgs = NULL
    );

    ILTree::Expression* AllocateSymbolReference
    (
        Declaration *Symbol,
        Type *ResultType,
        ILTree::Expression *BaseReference,
        const Location &TreeLocation,
        GenericBinding *GenericBindingContext
    );

    ILTree::Expression* AllocateExpression
    (
        BILOP Opcode,
        Type *ResultType,
        const Location &TreeLocation
    );

    ILTree::Expression* AllocateExpression
    (
        BILOP Opcode,
        Type *ResultType,
        ILTree::Expression *Operand,
        const Location &TreeLocation
    );

    ILTree::Expression* AllocateExpression
    (
        BILOP Opcode,
        Type *ResultType,
        ILTree::Expression *Left,
        ILTree::Expression *Right,
        const Location &TreeLocation
    );

    ILTree::Expression* AllocateBadExpression
    (
        Type *ResultType,
        const Location &TreeLocation
    );

    FX::FXSymbolProvider* GetFXSymbolProvider();

private:

    // Data members

    Semantics *m_Semantics;

    Symbols &m_SymbolCreator;
    ParserHelper m_PH;
    NorlsAllocator *m_Allocator;

    // Types

    class LambdaParameterInfo : public LambdaParamHandle
    {
    public:
        LambdaParameterInfo
        (
            ILTree::Expression *ParameterCreationExpr,
            Variable *ParameterTemp
        ) :
            m_ParameterCreationExpr(ParameterCreationExpr),
            m_ParameterTemp(ParameterTemp)
        {
        }

        ILTree::Expression* GetParameterCreationExpr()
        {
            return m_ParameterCreationExpr;
        }

        Variable* GetParameterTemp()
        {
            return m_ParameterTemp;
        }

    private:

        ILTree::Expression *m_ParameterCreationExpr;
        Variable *m_ParameterTemp;
    };
};
