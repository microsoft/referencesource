//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Base template and types for expression tree factory abstraction.
//
// ExpressionTreeGenerator:
//      Abstract class that has factory methods using which expression trees
// can be created.
//
// This class needs to specialized in order to be used. Currently there
// are 2 specializations:
//
//  1. One that produces bound trees that specify the expression tree creation
//  2. the other produces expression tree objects directly
//
//-------------------------------------------------------------------------------------------------

#pragma once

template<class TREE_TYPE>
struct MemberAssignmentInfo
{
    Member *Member;
    GenericBinding *BindingContext;
    TREE_TYPE *Value;
};

template<class TREE_TYPE>
struct PropertyAssignmentInfo
{
    Procedure *PropertyGetter;
    GenericBinding *BindingContext;
    TREE_TYPE *Value;
};

template<class TREE_TYPE>
struct ListElementInitInfo
{
    Procedure *AddMethod;
    GenericBinding *BindingContext;
    IReadOnlyList<TREE_TYPE *> *Arguments;	// Arguments to the add method
};

class LambdaParamHandle
{
};

template<class TREE_TYPE>
class ExpressionTreeGenerator
{
public:

    // Expression tree generation factory methods

    virtual TREE_TYPE *CreateBadExpr
    (
        Type *ExprType,     // This is the result type of the expression for which the the bad expr node is being created
        Type *TargetType,   // This is the type of the resulting expr tree node
        Location& Loc
    ) = 0;

    virtual TREE_TYPE *CreateConvertExpr
    (
        TREE_TYPE *Operand,
        Type *TypeToConvertTo,
        bool UseCheckedSemantics,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    virtual TREE_TYPE *CreateConvertExpr
    (
        TREE_TYPE *Operand,
        Type *TypeToConvertTo,
        bool UseCheckedSemantics,
        Procedure *ImplementingMethod,
        GenericBinding *MethodBindingContext,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    virtual TREE_TYPE *CreateTypeAsExpr
    (
        TREE_TYPE *Operand,
        Type *TypeToConvertTo,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    virtual TREE_TYPE *CreateCastExpr
    (
        TREE_TYPE *Operand,
        Type *TypeToConvertTo,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    virtual TREE_TYPE *CreateArrayIndexExpr
    (
        TREE_TYPE *Array,
        TREE_TYPE *Index,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    virtual TREE_TYPE *CreateArrayIndexExpr
    (
        TREE_TYPE *Array,
        IReadOnlyList<TREE_TYPE *> *Indices,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    virtual TREE_TYPE *CreateArrayLengthExpr
    (
        TREE_TYPE *Array,
        Type *TargetType,
        const Location &Loc
    ) = 0;


    // Create an array given the bounds

    virtual TREE_TYPE *CreateNewArrayBoundsExpr
    (
        Type *ElementType,
        IReadOnlyList<TREE_TYPE *> *Bounds,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    // Create a 1-D array of the specified type and initialized with the specified elements

    virtual TREE_TYPE *CreateNewArrayInitExpr
    (
        Type *ElementType,
        IReadOnlyList<TREE_TYPE *> *Elements,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    virtual TREE_TYPE *CreateListInitExpr
    (
        TREE_TYPE *ListExpr,
        IReadOnlyList<ListElementInitInfo<TREE_TYPE>> *Initializers,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    virtual TREE_TYPE *CreateMemberInitExpr
    (
        TREE_TYPE *NewExpr,
        IReadOnlyList<MemberAssignmentInfo<TREE_TYPE>> *Initializers,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    virtual TREE_TYPE *CreateFieldExpr
    (
        BCSYM_Variable *Field,
        GenericTypeBinding *GenericBindingContext,
        TREE_TYPE *Instance,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    virtual TREE_TYPE *CreateCallExpr
    (
        Procedure *Proc,
        GenericBinding *GenericBindingContext,
        TREE_TYPE *Instance,
        IReadOnlyList<TREE_TYPE *> *Arguments,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    virtual TREE_TYPE* CreateInvokeExpr
    (
        TREE_TYPE *DelegateInstance,
        IReadOnlyList<TREE_TYPE *> *Arguments,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    virtual TREE_TYPE *CreatePropertyAccessExpr
    (
        Procedure *PropertyGet,
        GenericBinding *GenericBindingContext,
        TREE_TYPE *Instance,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    virtual TREE_TYPE *CreateTypeIsExpr
    (
        TREE_TYPE *ExprOperand,
        Type *TypeOperand,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    virtual TREE_TYPE *CreateNegateExpr
    (
        TREE_TYPE *Operand,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    virtual TREE_TYPE *CreateUnaryOperatorExpr
    (
        BILOP Operator,
        TREE_TYPE *Operand,
        bool UseCheckedSemantics,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    virtual TREE_TYPE *CreateUnaryOperatorExpr
    (
        BILOP Operator,
        TREE_TYPE *Operand,
        bool UseCheckedSemantics,
        Procedure *ImplementingMethod,
        GenericBinding *MethodBindingContext,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    virtual TREE_TYPE *CreateBinaryOperatorExpr
    (
        BILOP Operator,
        TREE_TYPE *LeftOperand,
        TREE_TYPE *RightOperand,
        bool UseCheckedSemantics,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    virtual TREE_TYPE *CreateBinaryOperatorExpr
    (
        BILOP Operator,
        TREE_TYPE *LeftOperand,
        TREE_TYPE *RightOperand,
        bool UseCheckedSemantics,
        bool LifToNull,
        Procedure *ImplementingMethod,
        GenericBinding *MethodBindingContext,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    virtual TREE_TYPE *CreateTernaryIfExpr
    (
        TREE_TYPE *Condition,
        TREE_TYPE *TrueValue,
        TREE_TYPE *FalseValue,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    // ----------------------------------------------------------------------------
    // This method returns an expression to create a coalesce expression parameter.
    // ----------------------------------------------------------------------------

    virtual TREE_TYPE *CreateCoalesceExpr
    (
        TREE_TYPE *LeftOperand,
        TREE_TYPE *RightOperand,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    // ----------------------------------------------------------------------------
    // This method returns an expression to create a coalesce expression parameter.
    // But along with the normal operands, it also accepts a lambda to perform
    // a conversion from the LeftOperand type.
    // ----------------------------------------------------------------------------

    virtual TREE_TYPE *CreateCoalesceExpr
    (
        TREE_TYPE *LeftOperand,
        TREE_TYPE *RightOperand,
        TREE_TYPE *ConversionLambda,
        Type *TargetType,
        const Location &Loc
    ) = 0;



    // ----------------------------------------------------------------------------
    // This method returns an expression to create a lambda parameter. Additionally it
    // also returns a handle to the lambda parameter (via an out param) so that the
    // lambda parameter can be referred to.
    // ----------------------------------------------------------------------------

    virtual LambdaParamHandle* CreateLambdaParameter
    (
        _In_z_ STRING *ParameterName,
        Type *ParameterType,
        const Location &Loc
    ) = 0;

    // ----------------------------------------------------------------------------
    // This method returns an expression that loads a previously created lambda
    // parameter expression from a temporary corresponding to that lambda parameter.
    // ----------------------------------------------------------------------------

    virtual TREE_TYPE *CreateLambdaParamReferenceExpr
    (
        LambdaParamHandle* LambdaParameter,
        const Location &Loc
    ) = 0;

    // ----------------------------------------------------------------------------
    // This method returns an expression that creates a lambda expression.
    // ----------------------------------------------------------------------------

    virtual TREE_TYPE *CreateLambdaExpr
    (
        IReadOnlyList<LambdaParamHandle *> *Parameters,
        TREE_TYPE *LambdaBody,
        Type *DelegateType,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    virtual TREE_TYPE *CreateQuoteExpr
    (
        TREE_TYPE *Operand,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    virtual TREE_TYPE *CreateNewExpr
    (
        Type *InstanceType,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    virtual TREE_TYPE *CreateNewExpr
    (
        Procedure *Constructor,
        GenericTypeBinding *GenericTypeBindingContext,
        IReadOnlyList<TREE_TYPE *> *Arguments,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    virtual TREE_TYPE *CreateNewExpr
    (
        Procedure *Constructor,
        GenericTypeBinding *GenericTypeBindingContext,
        IReadOnlyList<PropertyAssignmentInfo<TREE_TYPE>> *Initializers,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    virtual TREE_TYPE* CreateNothingConstantExpr
    (
        Type *NothingType,
        Type *TargetType,
        const Location &Loc
    ) = 0;

    // ----------------------------------------------------------------------------
    // DoesBilopMapToOperatorNode is not virtual and is indicative of what operator
    // nodes need to be supported for every implementation of this generator.
    // ----------------------------------------------------------------------------

    bool DoesBilopMapToOperatorNode
    (
        BILOP Opcode,
        bool UseCheckedSemantics
    )
    {
        return MapBilopToOperatorNode(Opcode, UseCheckedSemantics) != NULL;
    }

    ExpressionTreeGenerator(Compiler *Compiler)
        : m_Compiler(Compiler)
    {
        IfNullThrow(Compiler);
    }

protected:

    // ----------------------------------------------------------------------------
    // Map the BILOP to an operator node.
    //
    // MapBilopToOperatorNode is not virtual and is indicative of what operator
    // nodes need to be supported for every implementation of this generator.
    // ----------------------------------------------------------------------------

    STRING* MapBilopToOperatorNode
    (
        BILOP bilop,
        bool bCheckedSemantics
    );

    Compiler *m_Compiler;
};

//-------------------------------------------------------------------------------------------------
// ExprTreeGenRuntimeValuesSupport:
//      Abstract class that has extra factory methods that ExpressionTreeSemantics
// would use if this interface is defined.
//
// This class defines a method to capture any object at runtime into a Constant
// expression ET node. This method can only be implemented if the ET construction
// occurs at runtime rather than at compile time. Currently the actual ET
// implementation occurs at runtime for the lamabda conversion scenarios in the
// core compiler and occurs at compile time for hosted compiler scenarios.So only
// the ILTreeETGenerator (which is the ET generator impl for the core compiler)
// provides an implementation for this class.
//
//-------------------------------------------------------------------------------------------------

template<class TREE_TYPE>
class ExprTreeGenRuntimeValuesSupport
{
public:

    // ----------------------------------------------------------------------------
    // This method should capture the value obtained on evaluating the tree
    // represented by the "Value" parameter (at runtime) in a "Constant"
    // DLR AST node.
    // ----------------------------------------------------------------------------

    virtual TREE_TYPE* CaptureRuntimeValueAsConstant
    (
        ILTree::Expression *Value,
        Type *TargetType
    ) = 0;
};


// ----------------------------------------------------------------------------
// Map the BILOP to an operator node.
//
// MapBilopToOperatorNode is not virtual and is indicative of what operator
// nodes need to be supported for every implementation of this generator.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> STRING*
ExpressionTreeGenerator<TREE_TYPE>::MapBilopToOperatorNode
(
    BILOP bilop,
    bool bCheckedSemantics
)
{
    STRING* MethodName = NULL;

    switch( bilop )
    {

    case SX_ADD:
        if( bCheckedSemantics )
        {
            MethodName = STRING_CONST( m_Compiler, ComExpressionAddChecked );
        }
        else
        {
            MethodName = STRING_CONST( m_Compiler, ComExpressionAdd );
        }
        break;

    case SX_SUB:
        if( bCheckedSemantics )
        {
            MethodName = STRING_CONST( m_Compiler, ComExpressionSubtractChecked );
        }
        else
        {
            MethodName = STRING_CONST( m_Compiler, ComExpressionSubtract );
        }
        break;

    case SX_MUL:
        if( bCheckedSemantics )
        {
            MethodName = STRING_CONST( m_Compiler, ComExpressionMultiplyChecked );
        }
        else
        {
            MethodName = STRING_CONST( m_Compiler, ComExpressionMultiply );
        }
        break;

    case SX_IDIV:
    case SX_DIV:
        MethodName = STRING_CONST( m_Compiler, ComExpressionDivide );
        break;

    case SX_MOD:
        MethodName = STRING_CONST( m_Compiler, ComExpressionModulo );
        break;

    case SX_POW:
        MethodName = STRING_CONST( m_Compiler, ComExpressionPower );
        break;

    case SX_AND:
        MethodName = STRING_CONST( m_Compiler, ComExpressionAnd );
        break;

    case SX_OR:
        MethodName = STRING_CONST( m_Compiler, ComExpressionOr );
        break;

    case SX_XOR:
        MethodName = STRING_CONST( m_Compiler, ComExpressionXor );
        break;

    case SX_SHIFT_LEFT:
        MethodName = STRING_CONST( m_Compiler, ComExpressionLeftShift );
        break;

    case SX_SHIFT_RIGHT:
        MethodName = STRING_CONST( m_Compiler, ComExpressionRightShift );
        break;

    case SX_IS:
    case SX_EQ:
        MethodName = STRING_CONST( m_Compiler, ComExpressionEQ );
        break;

    case SX_ISNOT:
    case SX_NE:
        MethodName = STRING_CONST( m_Compiler, ComExpressionNE );
        break;

    case SX_LT:
        MethodName = STRING_CONST( m_Compiler, ComExpressionLT );
        break;

    case SX_GT:
        MethodName = STRING_CONST( m_Compiler, ComExpressionGT );
        break;

    case SX_GE:
        MethodName = STRING_CONST( m_Compiler, ComExpressionGE );
        break;

    case SX_LE:
        MethodName = STRING_CONST( m_Compiler, ComExpressionLE );
        break;

    case SX_NEG:
        if( bCheckedSemantics )
        {
            MethodName = STRING_CONST( m_Compiler, ComExpressionNegateChecked );
        }
        else
        {
            MethodName = STRING_CONST( m_Compiler, ComExpressionNegate );
        }
        break;

    case SX_NOT:
        MethodName = STRING_CONST( m_Compiler, ComExpressionNot );
        break;

    case SX_PLUS:
        MethodName = STRING_CONST( m_Compiler, ComExpressionPlus );
        break;

    case SX_CTYPE:
    case SX_TRYCAST:
    case SX_DIRECTCAST:
        if( bCheckedSemantics )
        {
            MethodName = STRING_CONST( m_Compiler, ComExpressionConvertChecked );
        }
        else
        {
            MethodName = STRING_CONST( m_Compiler, ComExpressionConvert );
        }
        break;

    case SX_ANDALSO:
        MethodName = STRING_CONST( m_Compiler, ComExpressionAndAlso );
        break;

    case SX_ORELSE:
        MethodName = STRING_CONST( m_Compiler, ComExpressionOrElse );
        break;

    default:
        MethodName = NULL;
    }

    return MethodName;
}
