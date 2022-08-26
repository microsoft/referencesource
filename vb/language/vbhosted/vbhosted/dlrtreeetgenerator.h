//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
// DLRTreeETGenerator:
//      Expression tree factory class that specializes the base ExpressionTreeGenerator
// class. Using this specialization, the factory methods directly produce the DLR ASTs.
//
//-------------------------------------------------------------------------------------------------

#pragma once

#include <vcclr.h>
#include "sal.h"
#include "ExpressionTreeGenerator.h"
#include "SymbolMap.h"

using System::Linq::Expressions::Expression;
using System::Linq::Expressions::LambdaExpression;
using System::Linq::Expressions::MemberAssignment;

typedef struct{} DLRExpressionTree;
typedef System::Collections::Generic::Dictionary<unsigned, Expression^> IntToExprDictionary;

class DLRTreeETGenerator : public ExpressionTreeGenerator<DLRExpressionTree>
{
public:

    DLRTreeETGenerator(Compiler *Compiler, NorlsAllocator *Allocator)
      : 
        ExpressionTreeGenerator(Compiler),
        m_Allocator(Allocator)
    {
        ThrowIfNull(Compiler);
        ThrowIfNull(Allocator);

        m_ExprMap = gcnew IntToExprDictionary();

        m_UnmanagedToManagedSymbolMap =
            gcnew Microsoft::Compiler::VisualBasic::SymbolMap();

        // Start from 1, since 0 is used to represent null.
        m_NextAvailableKey = 1;
    }

    Expression^ GetManagedExpressionTree(DLRExpressionTree *ExprTree)
    {
        return (Expression^)FromDLRExpressionTree(ExprTree);
    }

    // Expression tree generation factory methods

    DLRExpressionTree* CreateBadExpr
    (
        Type *ExprType,     // This is the result type of the expression for which the the bad expr node is being created
        Type *TargetType,   // This is the type of the resulting expr tree node
        Location& Loc
    );

    DLRExpressionTree* CreateConvertExpr
    (
        DLRExpressionTree *Operand,
        Type *TypeToConvertTo,
        bool UseCheckedSemantics,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateConvertExpr
    (
        DLRExpressionTree *Operand,
        Type *TypeToConvertTo,
        bool UseCheckedSemantics,
        Procedure *ImplementingMethod,
        GenericBinding *MethodBindingContext,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateTypeAsExpr
    (
        DLRExpressionTree *Operand,
        Type *TypeToConvertTo,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateCastExpr
    (
        DLRExpressionTree *Operand,
        Type *TypeToConvertTo,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateArrayIndexExpr
    (
        DLRExpressionTree *Array,
        DLRExpressionTree *Index,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateArrayIndexExpr
    (
        DLRExpressionTree *Array,
        IReadOnlyList<DLRExpressionTree *> *Indices,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateArrayLengthExpr
    (
        DLRExpressionTree *Array,
        Type *TargetType,
        const Location &Loc
    );


    // Create an array given the bounds

    DLRExpressionTree* CreateNewArrayBoundsExpr
    (
        Type *ElementType,
        IReadOnlyList<DLRExpressionTree *> *Bounds,
        Type *TargetType,
        const Location &Loc
    );

    // Create a 1-D array of the specified type and initialized with the specified elements

    DLRExpressionTree* CreateNewArrayInitExpr
    (
        Type *ElementType,
        IReadOnlyList<DLRExpressionTree *> *Elements,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateListInitExpr
    (
        DLRExpressionTree *ListExpr,
        IReadOnlyList<ListElementInitInfo<DLRExpressionTree>> *Initializers,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateMemberInitExpr
    (
        DLRExpressionTree *NewExpr,
        IReadOnlyList<MemberAssignmentInfo<DLRExpressionTree>> *Initializers,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree *CreateFieldExpr
    (
        BCSYM_Variable *Field,
        GenericTypeBinding *GenericBindingContext,
        DLRExpressionTree *Instance,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateCallExpr
    (
        Procedure *Proc,
        GenericBinding *GenericBindingContext,
        DLRExpressionTree *Instance,
        IReadOnlyList<DLRExpressionTree *> *Arguments,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateCallExpr
    (
        _In_opt_z_ WCHAR *Name,
        DLRExpressionTree *Instance,
        IReadOnlyList<DLRExpressionTree *> *Arguments,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateInvokeExpr
    (
        DLRExpressionTree *DelegateInstance,
        IReadOnlyList<DLRExpressionTree *> *Arguments,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreatePropertyAccessExpr
    (
        Procedure *PropertyGet,
        GenericBinding *GenericBindingContext,
        DLRExpressionTree *Instance,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateTypeIsExpr
    (
        DLRExpressionTree *ExprOperand,
        Type *TypeOperand,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateNegateExpr
    (
        DLRExpressionTree *Operand,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateUnaryOperatorExpr
    (
        BILOP Operator,
        DLRExpressionTree *Operand,
        bool UseCheckedSemantics,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateUnaryOperatorExpr
    (
        BILOP Operator,
        DLRExpressionTree *Operand,
        bool UseCheckedSemantics,
        Procedure *ImplementingMethod,
        GenericBinding *MethodBindingContext,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateBinaryOperatorExpr
    (
        BILOP Operator,
        DLRExpressionTree *LeftOperand,
        DLRExpressionTree *RightOperand,
        bool UseCheckedSemantics,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateBinaryOperatorExpr
    (
        BILOP Operator,
        DLRExpressionTree *LeftOperand,
        DLRExpressionTree *RightOperand,
        bool UseCheckedSemantics,
        bool LiftToNull,
        Procedure *ImplementingMethod,
        GenericBinding *MethodBindingContext,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateTernaryIfExpr
    (
        DLRExpressionTree *Condition,
        DLRExpressionTree *TrueValue,
        DLRExpressionTree *FalseValue,
        Type *TargetType,
        const Location &Loc
    );

    // ----------------------------------------------------------------------------
    // This method returns an expression to create a coalesce expression parameter.
    // ----------------------------------------------------------------------------

    DLRExpressionTree *CreateCoalesceExpr
    (
        DLRExpressionTree *LeftOperand,
        DLRExpressionTree *RightOperand,
        Type *TargetType,
        const Location &Loc
    );

    // ----------------------------------------------------------------------------
    // This method returns an expression to create a coalesce expression parameter.
    // But along with the normal operands, it also accepts a lambda to perform
    // a conversion from the LeftOperand type.
    // ----------------------------------------------------------------------------

    DLRExpressionTree *CreateCoalesceExpr
    (
        DLRExpressionTree *LeftOperand,
        DLRExpressionTree *RightOperand,
        DLRExpressionTree *ConversionLambda,
        Type *TargetType,
        const Location &Loc
    );

    // ----------------------------------------------------------------------------
    // This method returns a LambdaParamHandle which encapsulates the
    // lambda parameter instance created.
    // ----------------------------------------------------------------------------

    LambdaParamHandle* CreateLambdaParameter
    (
        _In_opt_z_ STRING *ParameterName,
        Type *ParameterType,
        const Location &Loc
    );

    // ----------------------------------------------------------------------------
    // This method returns an expression that loads a previously created lambda
    // parameter expression from a temporary corresponding to that lambda parameter.
    // ----------------------------------------------------------------------------

    DLRExpressionTree* CreateLambdaParamReferenceExpr
    (
        LambdaParamHandle *LambdaParameter,
        const Location &Loc
    );

    // ----------------------------------------------------------------------------
    // This method returns an expression that creates a lambda expression.
    // ----------------------------------------------------------------------------

    DLRExpressionTree* CreateLambdaExpr
    (
        IReadOnlyList<LambdaParamHandle *> *Parameters,
        DLRExpressionTree *LambdaBody,
        Type *DelegateType,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateQuoteExpr
    (
        DLRExpressionTree *Operand,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateNewExpr
    (
        Type *InstanceType,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateNewExpr
    (
        Procedure *Constructor,
        GenericTypeBinding *GenericTypeBindingContext,
        IReadOnlyList<DLRExpressionTree *> *Arguments,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateNewExpr
    (
        Procedure *Constructor,
        GenericTypeBinding *GenericTypeBindingContext,
        IReadOnlyList<PropertyAssignmentInfo<DLRExpressionTree>> *Initializers,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateStringConstantExpr
    (
        _In_opt_z_ const WCHAR *Value,
        unsigned LengthInCharacters,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateIntegralConstantExpr
    (
        const __int64 Value,
        Type *IntegralType,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateFloatingPointConstantExpr
    (
        const double Value,
        Type *FloatingPointType,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateDecimalConstantExpr
    (
        const DECIMAL &Value,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateBooleanConstantExpr
    (
        const bool Value,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateCharConstantExpr
    (
        const WCHAR Value,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateDateConstantExpr
    (
        const __int64 Value,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateNothingConstantExpr
    (
        Type *NothingType,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateTypeRefExpr
    (
        Type *TypeSymbol,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateMethodRefExpr
    (
        Procedure *Method,
        GenericBinding *GenericBindingContext,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateFieldRefExpr
    (
        Variable *Field,
        GenericTypeBinding *GenericTypeBindingContext,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateBlockExpr
    (
        IReadOnlyList<DLRExpressionTree *> *Variables,
        IReadOnlyList<DLRExpressionTree *> *Body,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateBlockExpr
    (
        IReadOnlyList<DLRExpressionTree *> *Body,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateVariableExpr
    (
        _In_opt_z_ WCHAR *VarName,
        Type *VarType,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateAssignExpr
    (
        DLRExpressionTree *Left,
        DLRExpressionTree *Right,
        Type *TargetType,
        const Location &Loc
    );

    DLRExpressionTree* CreateReferenceEqualExpr
    (
        DLRExpressionTree *Left,
        DLRExpressionTree *Right,
        Type *TargetType,
        const Location &Loc
    );

private:

    // internal helpers

    static bool IsIntegralType
    (
        System::Type^ Type
    );

    static bool IsFloatingPointType
    (
        System::Type^ Type
    );

    System::Reflection::MethodInfo^ GetMethodRef
    (
        Procedure *Method,
        GenericBinding *GenericBindingContext
    );

    System::Reflection::ConstructorInfo^ GetConstructorRef
    (
        Procedure *Method,
        GenericTypeBinding *GenericTypeBindingContext
    );

    System::Reflection::FieldInfo^ GetFieldRef
    (
        Variable *Field,
        GenericTypeBinding *GenericTypeBindingContext
    );

    System::Type^ GetTypeRef
    (
        Type *Type
    );

    array<Expression^>^ CreateExpressionArray
    (
        IReadOnlyList<DLRExpressionTree *> *Expressions
    );

    template<class EXPRESSION_TREE>
    array<EXPRESSION_TREE>^ CreateExpressionArray
    (
        IReadOnlyList<DLRExpressionTree *> *Expressions
    );

    DLRExpressionTree *GetLambdaParamExpr
    (
        LambdaParamHandle *LambdaParameter
    );

    DLRExpressionTree* ToDLRExpressionTree
    (
        Expression^ Expr
    );

    Expression^ FromDLRExpressionTree
    (
        DLRExpressionTree *ExprTree
    );

private:

    // Data members

    NorlsAllocator *m_Allocator;

    unsigned m_NextAvailableKey;
    gcroot<IntToExprDictionary^> m_ExprMap;

    gcroot<Microsoft::Compiler::VisualBasic::SymbolMap^> m_UnmanagedToManagedSymbolMap;

    // Types

    class LambdaParameterInfo : public LambdaParamHandle
    {
    public:
        LambdaParameterInfo(DLRExpressionTree *ParameterExpr)
            : m_ParameterExpr(ParameterExpr)
        {
        }

        DLRExpressionTree* GetParameterExpr()
        {
            return m_ParameterExpr;
        }

    private:

        DLRExpressionTree *m_ParameterExpr;
    };
};
