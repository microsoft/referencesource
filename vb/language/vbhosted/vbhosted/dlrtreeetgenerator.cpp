//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

using namespace System::Reflection;
using System::Linq::Expressions::ParameterExpression;

DLRExpressionTree* DLRTreeETGenerator::CreateBadExpr
(
    Type *ExprType,     // This is the result type of the expression for which the the bad expr node is being created
    Type *TargetType,   // This is the expected type of the actual node created by this method. Can be ignored in this generator.
    Location& Loc
)
{
    ThrowIfNull(ExprType);

    // 










    System::Type^ ManagedExprType = GetTypeRef(ExprType);

    if (ManagedExprType == System::Void::typeid)
    {
        //Can not create a Bad Node of type void need to abort converting this tree.
        throw BadNodeException();
    }

    // Use a parameter expression to represent a bad node since it can be of
    // any type and also can be used in all contexts (RHS, LHS).
    //
    Expression ^Expr = Expression::Parameter(ManagedExprType, "BadNode");

    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateConvertExpr
(
    DLRExpressionTree *Operand,
    Type *TypeToConvertTo,
    bool UseCheckedSemantics,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Operand);
    ThrowIfNull(TypeToConvertTo);

    Expression^ ManagedOperand = FromDLRExpressionTree(Operand);
    System::Type^ ManagedType = GetTypeRef(TypeToConvertTo);

    Expression^ Expr = nullptr;

    if (UseCheckedSemantics)
    {
        Expr = Expression::ConvertChecked(ManagedOperand, ManagedType);
    }
    else
    {
        Expr = Expression::Convert(ManagedOperand, ManagedType);
    }

    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateConvertExpr
(
    DLRExpressionTree *Operand,
    Type *TypeToConvertTo,
    bool UseCheckedSemantics,
    Procedure *ImplementingMethod,
    GenericBinding *MethodBindingContext,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Operand);
    ThrowIfNull(TypeToConvertTo);

    Expression^ ManagedOperand = FromDLRExpressionTree(Operand);
    System::Type^ ManagedType = GetTypeRef(TypeToConvertTo);
    System::Reflection::MethodInfo^ ManagedMethod = GetMethodRef(ImplementingMethod, MethodBindingContext);

    Expression^ Expr = nullptr;

    if (UseCheckedSemantics)
    {
        Expr = Expression::ConvertChecked(ManagedOperand, ManagedType, ManagedMethod);
    }
    else
    {
        Expr = Expression::Convert(ManagedOperand, ManagedType, ManagedMethod);
    }

    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateTypeAsExpr
(
    DLRExpressionTree *Operand,
    Type *TypeToConvertTo,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Operand);
    ThrowIfNull(TypeToConvertTo);


    Expression^ Expr = Expression::TypeAs(
        FromDLRExpressionTree(Operand),
        GetTypeRef(TypeToConvertTo)
        );

    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateCastExpr
(
    DLRExpressionTree *Operand,
    Type *TypeToConvertTo,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Operand);
    ThrowIfNull(TypeToConvertTo);

    // There is no separate Cast factory method. Cast is same as Convert currently.
    //
    // Consider: why is there a separate notion of a conversion and a cast? The
    // original implementation of ExpressionTreeSemantics tried to keep them
    // separate. Need to find out the reason for this from Microsoft. If there is no
    // strong reason, then the CreateCastExpr should be removed and the uses
    // just replaced with CreateConvertExpr.

    Expression^ Expr = Expression::Convert(
        FromDLRExpressionTree(Operand),
        GetTypeRef(TypeToConvertTo)
        );

    return ToDLRExpressionTree(Expr);
}

//Macro ArrayIndex interferes with method call to ArrayIndex so temporarily undefine it
#pragma push_macro("ArrayIndex")
#undef ArrayIndex

DLRExpressionTree* DLRTreeETGenerator::CreateArrayIndexExpr
(
    DLRExpressionTree *Array,
    DLRExpressionTree *Index,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Array);
    ThrowIfNull(Index);

    Expression^ Expr = Expression::ArrayIndex(
        FromDLRExpressionTree(Array),
        FromDLRExpressionTree(Index)
        );
    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateArrayIndexExpr
(
    DLRExpressionTree *Array,
    IReadOnlyList<DLRExpressionTree *> *Indices,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Array);
    ThrowIfFalse(Indices && Indices->Count() > 0);

    Expression^ Expr = Expression::ArrayIndex(
        FromDLRExpressionTree(Array),
        CreateExpressionArray(Indices)
        );
    return ToDLRExpressionTree(Expr);
}

//Restore the ArrayIndex Macro
#pragma pop_macro("ArrayIndex")

DLRExpressionTree* DLRTreeETGenerator::CreateArrayLengthExpr
(
    DLRExpressionTree *Array,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Array);

    Expression^ Expr = Expression::ArrayLength(
        FromDLRExpressionTree(Array)
        );
    return ToDLRExpressionTree(Expr);
}

// Create an array given the bounds

DLRExpressionTree* DLRTreeETGenerator::CreateNewArrayBoundsExpr
(
    Type *ElementType,
    IReadOnlyList<DLRExpressionTree *> *Bounds,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(ElementType);

    Expression^ Expr = Expression::NewArrayBounds(
        GetTypeRef(ElementType),
        CreateExpressionArray(Bounds)
        );
    return ToDLRExpressionTree(Expr);
}

// Create a 1-D array of the specified type and initialized with the specified elements

DLRExpressionTree* DLRTreeETGenerator::CreateNewArrayInitExpr
(
    Type *ElementType,
    IReadOnlyList<DLRExpressionTree *> *Elements,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(ElementType);

    Expression^ Expr = Expression::NewArrayInit(
        GetTypeRef(ElementType),
        CreateExpressionArray(Elements)
        );
    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateListInitExpr
(
    DLRExpressionTree *ListExpr,
    IReadOnlyList<ListElementInitInfo<DLRExpressionTree>> *Initializers,
    Type *TargetType,
    const Location &Loc
)   
{
    ThrowIfNull(ListExpr);
    
    unsigned Count = Initializers->Count();
    if(Count > 0)
    {
        array<System::Linq::Expressions::ElementInit^>^ ElementInitArray = ElementInitArray = gcnew array<System::Linq::Expressions::ElementInit^>(Count);

        for(unsigned ind = 0; ind < Count; ind++)
        {
            const ListElementInitInfo<DLRExpressionTree> &copyFrom = (*Initializers)[ind];

            ElementInitArray[ind] = Expression::ElementInit(
                GetMethodRef(copyFrom.AddMethod, copyFrom.BindingContext),
                CreateExpressionArray(copyFrom.Arguments)
                );
        }

        Expression^ Expr = Expression::ListInit(
            (System::Linq::Expressions::NewExpression^)(FromDLRExpressionTree(ListExpr)),
            ElementInitArray
            );
        return ToDLRExpressionTree(Expr);
    }
    else
    {
        //If there are no list inits this is a no op
        return ListExpr;
    }
}

DLRExpressionTree* DLRTreeETGenerator::CreateMemberInitExpr
(
    DLRExpressionTree *NewExpr,
    IReadOnlyList<MemberAssignmentInfo<DLRExpressionTree>> *Initializers,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(NewExpr);

    array<MemberAssignment^>^ MemberAssignmentsArray = nullptr;

    if (Initializers && Initializers->Count() > 0)
    {
        MemberAssignmentsArray = gcnew array<MemberAssignment^>(Initializers->Count());
    
        for(unsigned ind = 0, Count = Initializers->Count(); ind < Count; ind++)
        {
            const MemberAssignmentInfo<DLRExpressionTree> &copyFrom = (*Initializers)[ind];

            if(copyFrom.Member->IsVariable())
            {
                MemberAssignmentsArray[ind] = Expression::Bind(
                    GetFieldRef(
                        copyFrom.Member->PVariable(),
                        copyFrom.BindingContext->PGenericTypeBinding()
                        ),
                    FromDLRExpressionTree(copyFrom.Value)
                    );
            }
            else
            {
                MemberAssignmentsArray[ind] = Expression::Bind(
                    GetMethodRef(
                        copyFrom.Member->PProc(),
                        copyFrom.BindingContext->PGenericTypeBinding()
                        ),
                    FromDLRExpressionTree(copyFrom.Value)
                    );
            }
        }
    }

    Expression^ Expr = Expression::MemberInit(
        (System::Linq::Expressions::NewExpression^)(FromDLRExpressionTree(NewExpr)),
        MemberAssignmentsArray
        );
    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateFieldExpr
(
    BCSYM_Variable *Field,
    GenericTypeBinding *GenericBindingContext,
    DLRExpressionTree *Instance,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Field);

    FieldInfo^ Info = GetFieldRef(Field, GenericBindingContext);
    Expression^ Expr = nullptr;
    
    if(Info->IsLiteral)
    {   
        //Extract value for Constant Fields and Enums
        Expr = Expression::Constant(
            Info->GetValue(nullptr),
            Info->FieldType
            );
    }
    else
    {
        Expr = Expression::Field(
            FromDLRExpressionTree(Instance),
            Info
            );
    }
    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateCallExpr
(
    Procedure *Proc,
    GenericBinding *GenericBindingContext,
    DLRExpressionTree *Instance,
    IReadOnlyList<DLRExpressionTree *> *Arguments,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Proc);

    Expression^ Expr = Expression::Call(
        FromDLRExpressionTree(Instance), 
        GetMethodRef(Proc, GenericBindingContext), 
        CreateExpressionArray(Arguments)
        );
    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateCallExpr
(
    _In_opt_z_ WCHAR *Name,
    DLRExpressionTree *Instance,
    IReadOnlyList<DLRExpressionTree *> *Arguments,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Name);
    ThrowIfNull(Instance);

    Expression^ Expr = Expression::Call(
        FromDLRExpressionTree(Instance),
        gcnew System::String(Name),
        nullptr,
        CreateExpressionArray(Arguments)
        );
    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateInvokeExpr
(
    DLRExpressionTree *DelegateInstance,
    IReadOnlyList<DLRExpressionTree *> *Arguments,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(DelegateInstance);

    Expression^ Expr = Expression::Invoke(
        FromDLRExpressionTree(DelegateInstance),
        CreateExpressionArray(Arguments)
        );

    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreatePropertyAccessExpr
(
    Procedure *PropertyGet,
    GenericBinding *GenericBindingContext,
    DLRExpressionTree *Instance,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfFalse(PropertyGet && PropertyGet->IsPropertyGet());

    Expression^ Expr = Expression::Property(
        FromDLRExpressionTree(Instance), 
        GetMethodRef(PropertyGet, GenericBindingContext)
        );
    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateTypeIsExpr
(
    DLRExpressionTree *ExprOperand,
    Type *TypeOperand,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(ExprOperand);
    ThrowIfNull(TypeOperand);

    Expression^ Expr = Expression::TypeIs(
        FromDLRExpressionTree(ExprOperand), 
        GetTypeRef(TypeOperand)
        );
    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateNegateExpr
(
    DLRExpressionTree *Operand,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Operand);

    Expression^ Expr = Expression::Negate(
        FromDLRExpressionTree(Operand)
        );
    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateUnaryOperatorExpr
(
    BILOP Operator,
    DLRExpressionTree *Operand,
    bool UseCheckedSemantics,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfFalse(SX_PLUS == Operator ||
                 SX_NEG == Operator ||
                 SX_NOT == Operator);
    ThrowIfNull(Operand);

    Expression^ Expr = nullptr;

    switch( Operator )
    {
    case SX_PLUS:
        Expr = Expression::UnaryPlus(
            FromDLRExpressionTree(Operand)
            );
        break;
        
    case SX_NEG:
        if(UseCheckedSemantics)
        {
            Expr = Expression::NegateChecked(
                FromDLRExpressionTree(Operand)
                );
        }
        else
        {
            Expr = Expression::Negate(
                FromDLRExpressionTree(Operand)
                );
        }
        break;

    case SX_NOT:
        Expr = Expression::Not(
            FromDLRExpressionTree(Operand)
            );
        break;

    default:
        throw gcnew System::NotSupportedException;
    }

    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateUnaryOperatorExpr
(
    BILOP Operator,
    DLRExpressionTree *Operand,
    bool UseCheckedSemantics,
    Procedure *ImplementingMethod,
    GenericBinding *MethodBindingContext,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfFalse(SX_PLUS == Operator ||
                 SX_NEG == Operator ||
                 SX_NOT == Operator);
    ThrowIfNull(Operand);

    Expression^ Expr = nullptr;
    MethodInfo^ Method = ImplementingMethod ? GetMethodRef(ImplementingMethod, MethodBindingContext) : nullptr;

    switch( Operator )
    {
    case SX_PLUS:
        Expr = Expression::UnaryPlus(
            FromDLRExpressionTree(Operand),
            Method
            );
        break;
        
    case SX_NEG:
        if(UseCheckedSemantics)
        {
            Expr = Expression::NegateChecked(
                FromDLRExpressionTree(Operand),
                Method
                );
        }
        else
        {
            Expr = Expression::Negate(
                FromDLRExpressionTree(Operand),
                Method
                );
        }
        break;

    case SX_NOT:
        Expr = Expression::Not(
            FromDLRExpressionTree(Operand),
            Method
            );
        break;

    default:
        throw gcnew System::NotSupportedException;
    }

    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateBinaryOperatorExpr
(
    BILOP Operator,
    DLRExpressionTree *LeftOperand,
    DLRExpressionTree *RightOperand,
    bool UseCheckedSemantics,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfFalse(SX_ADD == Operator ||
                 SX_SUB == Operator ||
                 SX_MUL == Operator ||
                 SX_IDIV == Operator ||
                 SX_DIV == Operator ||
                 SX_MOD == Operator ||
                 SX_POW == Operator ||
                 SX_AND == Operator ||
                 SX_OR == Operator ||
                 SX_XOR == Operator ||
                 SX_SHIFT_LEFT == Operator ||
                 SX_SHIFT_RIGHT == Operator ||
                 SX_ANDALSO == Operator ||
                 SX_ORELSE == Operator ||
                 SX_IS == Operator ||
                 SX_EQ == Operator ||
                 SX_ISNOT == Operator ||
                 SX_NE == Operator ||
                 SX_LT == Operator ||
                 SX_GT == Operator ||
                 SX_GE == Operator ||
                 SX_LE == Operator);
    ThrowIfNull(LeftOperand);
    ThrowIfNull(RightOperand);

    Expression^ Expr = nullptr;

    switch( Operator )
    {
    case SX_ADD:
        if(UseCheckedSemantics)
        {
            Expr = Expression::AddChecked(
                FromDLRExpressionTree(LeftOperand),
                FromDLRExpressionTree(RightOperand)
                );
        }
        else
        {
            Expr = Expression::Add(
                FromDLRExpressionTree(LeftOperand),
                FromDLRExpressionTree(RightOperand)
                );
        }
        break;

    case SX_SUB:
        if(UseCheckedSemantics)
        {
            Expr = Expression::SubtractChecked(
                FromDLRExpressionTree(LeftOperand),
                FromDLRExpressionTree(RightOperand)
                );
        }
        else
        {
            Expr = Expression::Subtract(
                FromDLRExpressionTree(LeftOperand),
                FromDLRExpressionTree(RightOperand)
                );
        }
        break;

    case SX_MUL:
        if(UseCheckedSemantics)
        {
            Expr = Expression::MultiplyChecked(
                FromDLRExpressionTree(LeftOperand),
                FromDLRExpressionTree(RightOperand)
                );
        }
        else
        {
            Expr = Expression::Multiply(
                FromDLRExpressionTree(LeftOperand),
                FromDLRExpressionTree(RightOperand)
                );
        }
        break;

    case SX_IDIV:
    case SX_DIV:
        Expr = Expression::Divide(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand)
            );
        break;

    case SX_MOD:
        Expr = Expression::Modulo(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand)
            );
        break;

    case SX_POW:
        Expr = Expression::Power(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand)
            );
        break;

    case SX_AND:
        Expr = Expression::And(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand)
            );
        break;

    case SX_OR:
        Expr = Expression::Or(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand)
            );
        break;

    case SX_XOR:
        Expr = Expression::ExclusiveOr(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand)
            );
        break;

    case SX_SHIFT_LEFT:
        Expr = Expression::LeftShift(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand)
            );
        break;

    case SX_SHIFT_RIGHT:
        Expr = Expression::RightShift(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand)
            );
        break;

    case SX_ANDALSO:
        Expr = Expression::AndAlso(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand)
            );
        break;

    case SX_ORELSE:
        Expr = Expression::OrElse(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand)
            );
        break;

    case SX_IS:
    case SX_EQ:
        Expr = Expression::Equal(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand)
            );
        break;
        
    case SX_ISNOT:
    case SX_NE:
        Expr = Expression::NotEqual(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand)
            );
        break;
        
    case SX_LT:
        Expr = Expression::LessThan(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand)
            );
        break;
        
    case SX_GT:
        Expr = Expression::GreaterThan(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand)
            );
        break;
        
    case SX_GE:
        Expr = Expression::GreaterThanOrEqual(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand)
            );
        break;
        
    case SX_LE:
        Expr = Expression::LessThanOrEqual(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand)
            );
        break;

    default:
        throw gcnew System::NotSupportedException;
    }

    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateBinaryOperatorExpr
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
)
{
    ThrowIfFalse(SX_ADD == Operator ||
                 SX_SUB == Operator ||
                 SX_MUL == Operator ||
                 SX_IDIV == Operator ||
                 SX_DIV == Operator ||
                 SX_MOD == Operator ||
                 SX_POW == Operator ||
                 SX_AND == Operator ||
                 SX_OR == Operator ||
                 SX_XOR == Operator ||
                 SX_SHIFT_LEFT == Operator ||
                 SX_SHIFT_RIGHT == Operator ||
                 SX_ANDALSO == Operator ||
                 SX_ORELSE == Operator ||
                 SX_IS == Operator ||
                 SX_EQ == Operator ||
                 SX_ISNOT == Operator ||
                 SX_NE == Operator ||
                 SX_LT == Operator ||
                 SX_GT == Operator ||
                 SX_GE == Operator ||
                 SX_LE == Operator);
    ThrowIfNull(LeftOperand);
    ThrowIfNull(RightOperand);

    Expression^ Expr = nullptr;
    MethodInfo^ Method = ImplementingMethod ? GetMethodRef(ImplementingMethod, MethodBindingContext) : nullptr;

    switch( Operator )
    {
    case SX_ADD:
        if(UseCheckedSemantics)
        {
            Expr = Expression::AddChecked(
                FromDLRExpressionTree(LeftOperand),
                FromDLRExpressionTree(RightOperand),
                Method
                );
        }
        else
        {
            Expr = Expression::Add(
                FromDLRExpressionTree(LeftOperand),
                FromDLRExpressionTree(RightOperand),
                Method
                );
        }
        break;

    case SX_SUB:
        if(UseCheckedSemantics)
        {
            Expr = Expression::SubtractChecked(
                FromDLRExpressionTree(LeftOperand),
                FromDLRExpressionTree(RightOperand),
                Method
                );
        }
        else
        {
            Expr = Expression::Subtract(
                FromDLRExpressionTree(LeftOperand),
                FromDLRExpressionTree(RightOperand),
                Method
                );
        }
        break;

    case SX_MUL:
        if(UseCheckedSemantics)
        {
            Expr = Expression::MultiplyChecked(
                FromDLRExpressionTree(LeftOperand),
                FromDLRExpressionTree(RightOperand),
                Method
                );
        }
        else
        {
            Expr = Expression::Multiply(
                FromDLRExpressionTree(LeftOperand),
                FromDLRExpressionTree(RightOperand),
                Method
                );
        }
        break;

    case SX_IDIV:
    case SX_DIV:
        Expr = Expression::Divide(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand),
            Method
            );
        break;

    case SX_MOD:
        Expr = Expression::Modulo(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand),
            Method
            );
        break;

    case SX_POW:
        Expr = Expression::Power(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand),
            Method
            );
        break;

    case SX_AND:
        Expr = Expression::And(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand),
            Method
            );
        break;

    case SX_OR:
        Expr = Expression::Or(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand),
            Method
            );
        break;

    case SX_XOR:
        Expr = Expression::ExclusiveOr(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand),
            Method
            );
        break;

    case SX_SHIFT_LEFT:
        Expr = Expression::LeftShift(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand),
            Method
            );
        break;

    case SX_SHIFT_RIGHT:
        Expr = Expression::RightShift(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand),
            Method
            );
        break;

    case SX_ANDALSO:
        Expr = Expression::AndAlso(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand),
            Method
            );
        break;

    case SX_ORELSE:
        Expr = Expression::OrElse(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand),
            Method
            );
        break;

    case SX_IS:
    case SX_EQ:
        Expr = Expression::Equal(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand),
            LiftToNull,
            Method
            );
        break;
        
    case SX_ISNOT:
    case SX_NE:
        Expr = Expression::NotEqual(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand),
            LiftToNull,
            Method
            );
        break;
        
    case SX_LT:
        Expr = Expression::LessThan(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand),
            LiftToNull,
            Method
            );
        break;
        
    case SX_GT:
        Expr = Expression::GreaterThan(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand),
            LiftToNull,
            Method
            );
        break;
        
    case SX_GE:
        Expr = Expression::GreaterThanOrEqual(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand),
            LiftToNull,
            Method
            );
        break;
        
    case SX_LE:
        Expr = Expression::LessThanOrEqual(
            FromDLRExpressionTree(LeftOperand),
            FromDLRExpressionTree(RightOperand),
            LiftToNull,
            Method
            );
        break;

    default:
        throw gcnew System::NotSupportedException;
    }

    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateTernaryIfExpr
(
    DLRExpressionTree *Condition,
    DLRExpressionTree *TrueValue,
    DLRExpressionTree *FalseValue,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Condition);
    ThrowIfNull(TrueValue);
    ThrowIfNull(FalseValue);

    Expression^ Expr = Expression::Condition(
        FromDLRExpressionTree(Condition), 
        FromDLRExpressionTree(TrueValue),
        FromDLRExpressionTree(FalseValue)
        );
    return ToDLRExpressionTree(Expr);
}

// ----------------------------------------------------------------------------
// This method returns an expression to create a coalesce expression parameter.
// ----------------------------------------------------------------------------

DLRExpressionTree* DLRTreeETGenerator::CreateCoalesceExpr
(
    DLRExpressionTree *LeftOperand,
    DLRExpressionTree *RightOperand,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(LeftOperand);
    ThrowIfNull(RightOperand);

    Expression^ Expr = Expression::Coalesce(
        FromDLRExpressionTree(LeftOperand), 
        FromDLRExpressionTree(RightOperand)
        );
    return ToDLRExpressionTree(Expr);
}

// ----------------------------------------------------------------------------
// This method returns an expression to create a coalesce expression parameter.
// But along with the normal operands, it also accepts a lambda to perform
// a conversion from the LeftOperand type.
// ----------------------------------------------------------------------------

DLRExpressionTree* DLRTreeETGenerator::CreateCoalesceExpr
(
    DLRExpressionTree *LeftOperand,
    DLRExpressionTree *RightOperand,
    DLRExpressionTree *ConversionLambda,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(LeftOperand);
    ThrowIfNull(RightOperand);
    ThrowIfNull(ConversionLambda);

    Expression^ Expr = Expression::Coalesce(
        FromDLRExpressionTree(LeftOperand),
        FromDLRExpressionTree(RightOperand),
        (LambdaExpression^)FromDLRExpressionTree(ConversionLambda)
        );

    return ToDLRExpressionTree(Expr);
}

// ----------------------------------------------------------------------------
// This method returns a LambdaParamHandle which encapsulates the
// lambda parameter instance created.
// ----------------------------------------------------------------------------

LambdaParamHandle* DLRTreeETGenerator::CreateLambdaParameter
(
    _In_opt_z_ STRING *ParameterName,
    Type *ParameterType,
    const Location &Loc
)
{
    ThrowIfNull(ParameterName);
    ThrowIfNull(ParameterType);

    Expression^ ParamExpr = Expression::Parameter(
        GetTypeRef(ParameterType),
        gcnew System::String(ParameterName)
        );

    return new (*m_Allocator) LambdaParameterInfo(
        ToDLRExpressionTree(ParamExpr)
        );
}

// ----------------------------------------------------------------------------
// This method returns an expression that loads a previously created lambda
// parameter expression from a temporary corresponding to that lambda parameter.
// ----------------------------------------------------------------------------

DLRExpressionTree* DLRTreeETGenerator::CreateLambdaParamReferenceExpr
(
    LambdaParamHandle *LambdaParameter,
    const Location &Loc
)
{
    ThrowIfNull(LambdaParameter);

    return GetLambdaParamExpr(LambdaParameter);
}

// ----------------------------------------------------------------------------
// This method returns an expression that creates a lambda expression.
// ----------------------------------------------------------------------------

DLRExpressionTree* DLRTreeETGenerator::CreateLambdaExpr
(
    IReadOnlyList<LambdaParamHandle *> *Parameters,
    DLRExpressionTree *LambdaBody,
    Type *DelegateType,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(LambdaBody);
    ThrowIfNull(DelegateType);

    // Construct the array of lambda parameters

    array<System::Linq::Expressions::ParameterExpression^>^ ParametersArray = nullptr;

    if (Parameters && Parameters->Count() > 0)
    {
        unsigned Count = Parameters->Count();
        ParametersArray = gcnew array<System::Linq::Expressions::ParameterExpression^>(Count);

        for(unsigned ind = 0; ind < Count; ind++)
        {
            ParametersArray[ind] = (ParameterExpression^)FromDLRExpressionTree(
                GetLambdaParamExpr((*Parameters)[ind])
                );
        }
    }

    Expression^ Expr = Expression::Lambda(
        GetTypeRef(DelegateType),
        FromDLRExpressionTree(LambdaBody),
        ParametersArray
        );

    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateQuoteExpr
(
    DLRExpressionTree *Operand,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Operand);

    Expression^ Expr = Expression::Quote(
        FromDLRExpressionTree(Operand)
        );

    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateNewExpr
(
    Type *InstanceType,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(InstanceType);

    Expression^ Expr = Expression::New(GetTypeRef(InstanceType));
    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateNewExpr
(
    Procedure *Constructor,
    GenericTypeBinding *GenericTypeBindingContext,
    IReadOnlyList<DLRExpressionTree *> *Arguments,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Constructor);

    Expression^ Expr = Expression::New(
        GetConstructorRef(Constructor, GenericTypeBindingContext), 
        CreateExpressionArray(Arguments)
        );
    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateNewExpr
(
    Procedure *Constructor,
    GenericTypeBinding *GenericTypeBindingContext,
    IReadOnlyList<PropertyAssignmentInfo<DLRExpressionTree>> *Initializers,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Constructor);

    array<Expression^>^ ArgumentsArray = nullptr;
    array<MemberInfo^>^ MembersArray = nullptr;

    if (Initializers && Initializers->Count() > 0)
    {
        ArgumentsArray = gcnew array<Expression^>(Initializers->Count());
        MembersArray = gcnew array<MemberInfo^>(Initializers->Count());

        for(unsigned ind = 0, Count = Initializers->Count(); ind < Count; ind++)
        {
            const PropertyAssignmentInfo<DLRExpressionTree> &copyFrom = (*Initializers)[ind];
            ArgumentsArray[ind] = FromDLRExpressionTree((*Initializers)[ind].Value);
            MembersArray[ind] = GetMethodRef((*Initializers)[ind].PropertyGetter, GenericTypeBindingContext);
        }
    }

    Expression^ Expr = Expression::New (
        GetConstructorRef(Constructor, GenericTypeBindingContext),
        ArgumentsArray,
        MembersArray
        );
    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateStringConstantExpr
(
    const WCHAR *Value,
    unsigned LengthInCharacters,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfFalse(Value != NULL || LengthInCharacters == 0);

    System::String^ ManagedString = nullptr;
    if(Value)
    {
        ManagedString = gcnew System::String(Value, 0, LengthInCharacters);
    }
    Expression^ Expr = Expression::Constant(ManagedString, System::String::typeid);
    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateIntegralConstantExpr
(
    const __int64 Value,
    Type *IntegralType,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfFalse(TypeHelpers::IsIntegralType(IntegralType));

    System::Type^ ManagedType = GetTypeRef(IntegralType);

    ThrowIfFalse(ManagedType->IsEnum || IsIntegralType(ManagedType));

    System::Object^ ManagedValue = nullptr;

    switch (IntegralType->GetVtype())
    {
    case t_i1:
        ManagedValue = gcnew System::SByte((__int8)Value);
        break;

    case t_ui1:
        ManagedValue = gcnew System::Byte((unsigned __int8)Value);
        break;

    case t_i2:
        ManagedValue = gcnew System::Int16((__int16)Value);
        break;
        
    case t_ui2:
        ManagedValue = gcnew System::UInt16((unsigned __int16)Value);
        break;

    case t_i4:
        ManagedValue = gcnew System::Int32((__int32)Value);
        break;

    case t_ui4:
        ManagedValue = gcnew System::UInt32((unsigned __int32)Value);
        break;

    case t_i8:
        ManagedValue = gcnew System::Int64((__int64)Value);
        break;

    case t_ui8:
        ManagedValue = gcnew System::UInt64((unsigned __int64)Value);
        break;

    default:
        ThrowIfFalse(false);
    }

    if(ManagedType->IsEnum)
    {
        //This is special handeling for the Like Operator which causes a reference to an enum to go through CreateIntegralConstant rather than CreateFieldRef

        //Get the strongly typed value of the enum
        ManagedValue = System::Enum::Parse(ManagedType, System::Convert::ToString(ManagedValue, System::Globalization::CultureInfo::InvariantCulture->NumberFormat));
    }

    Expression^ Expr = Expression::Constant(ManagedValue, ManagedType);

    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateFloatingPointConstantExpr
(
    const double Value,
    Type *FloatingPointType,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(FloatingPointType);
    ThrowIfFalse(TypeHelpers::IsFloatingType(FloatingPointType));

    System::Type^ ManagedType = GetTypeRef(FloatingPointType);
    ThrowIfFalse(IsFloatingPointType(ManagedType));

    System::Object^ ManagedValue = nullptr;

    switch (FloatingPointType->GetVtype())
    {
    case t_single:
        ManagedValue = gcnew System::Single((float)Value);
        break;

    case t_double:
        ManagedValue = gcnew System::Double((double)Value);
        break;

    default:
        ThrowIfFalse(false);
    }

    Expression^ Expr = Expression::Constant(ManagedValue, ManagedType);

    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateDecimalConstantExpr
(
    const DECIMAL &Value,
    Type *TargetType,
    const Location &Loc
)
{
    System::Decimal^ ManagedValue = gcnew System::Decimal(
        Value.Lo32,
        Value.Mid32,
        Value.Hi32,
        Value.sign,
        Value.scale);

    Expression ^Expr = Expression::Constant(ManagedValue, System::Decimal::typeid);

    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateBooleanConstantExpr
(
    const bool Value,
    Type *TargetType,
    const Location &Loc
)
{
    Expression^ Expr = Expression::Constant(gcnew System::Boolean(Value), System::Boolean::typeid);

    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateCharConstantExpr
(
    const WCHAR Value,
    Type *TargetType,
    const Location &Loc
)
{
    Expression^ Expr = Expression::Constant(gcnew System::Char(Value), System::Char::typeid);

    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateDateConstantExpr
(
    const __int64 Value,
    Type *TargetType,
    const Location &Loc
)
{
    Expression^ Expr = Expression::Constant(gcnew System::DateTime(Value), System::DateTime::typeid);

    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateNothingConstantExpr
(
    Type *NothingType,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(NothingType);

    System::Type^ ManagedType = GetTypeRef(NothingType);
    Expression^ Expr = Expression::Constant(nullptr, ManagedType);

    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateTypeRefExpr
(
    Type *TypeSymbol,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(TypeSymbol);

    System::Type^ ManagedType = GetTypeRef(TypeSymbol);
    Expression^ Expr = Expression::Constant(ManagedType, System::Type::typeid);

    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateMethodRefExpr
(
    Procedure *Method,
    GenericBinding *GenericBindingContext,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Method);

    Expression^ Expr = nullptr;

    if (Method->IsInstanceConstructor() || Method->IsSharedConstructor())
    {
        Expr = Expression::Constant(
            GetConstructorRef(Method, GenericBindingContext->PGenericTypeBinding()),
            ConstructorInfo::typeid
            );
    }
    else
    {
        Expr = Expression::Constant(
            GetMethodRef(Method, GenericBindingContext),
            MethodInfo::typeid
            );
    }

    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateFieldRefExpr
(
    Variable *Field,
    GenericTypeBinding *GenericTypeBindingContext,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Field);

    FieldInfo^ FieldRef = GetFieldRef(Field, GenericTypeBindingContext);
    Expression^ Expr = Expression::Constant(FieldRef, FieldInfo::typeid);

    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateBlockExpr
(
    IReadOnlyList<DLRExpressionTree *> *Variables,
    IReadOnlyList<DLRExpressionTree *> *Body,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Body);

    Expression^ Expr = Expression::Block(
        CreateExpressionArray<System::Linq::Expressions::ParameterExpression^>(Variables),
        CreateExpressionArray(Body)
        );
    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateBlockExpr
(
    IReadOnlyList<DLRExpressionTree *> *Body,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Body);

    Expression^ Expr = Expression::Block(
        CreateExpressionArray(Body)
        );
    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateVariableExpr
(
    _In_opt_z_ WCHAR *VarName,
    Type *VarType,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(VarName);
    ThrowIfNull(VarType);
    
    Expression^ Expr = Expression::Parameter(
        GetTypeRef(VarType),
        gcnew System::String(VarName)
        );

    return ToDLRExpressionTree(Expr);
}


DLRExpressionTree* DLRTreeETGenerator::CreateAssignExpr
(
    DLRExpressionTree *Left,
    DLRExpressionTree *Right,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Left);
    ThrowIfNull(Right);

    Expression^ Expr = Expression::Assign(
        FromDLRExpressionTree(Left),
        FromDLRExpressionTree(Right)
        );
    return ToDLRExpressionTree(Expr);
}

DLRExpressionTree* DLRTreeETGenerator::CreateReferenceEqualExpr
(
    DLRExpressionTree *Left,
    DLRExpressionTree *Right,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Left);
    ThrowIfNull(Right);

    Expression^ Expr = Expression::ReferenceEqual(
        FromDLRExpressionTree(Left),
        FromDLRExpressionTree(Right)
        );
    return ToDLRExpressionTree(Expr);
}

// Internal helpers

bool DLRTreeETGenerator::IsIntegralType
(
    System::Type^ Type
)
{
    // 



    return
        System::SByte::typeid->Equals(Type) ||
        System::Byte::typeid->Equals(Type) ||
        System::Int16::typeid->Equals(Type) ||
        System::UInt16::typeid->Equals(Type) ||
        System::Int32::typeid->Equals(Type) ||
        System::UInt32::typeid->Equals(Type) ||
        System::Int64::typeid->Equals(Type) ||
        System::UInt64::typeid->Equals(Type);

}

bool DLRTreeETGenerator::IsFloatingPointType
(
    System::Type^ Type
)
{
    // 

    return
        System::Single::typeid->Equals(Type) ||
        System::Double::typeid->Equals(Type);
}

MethodInfo^ DLRTreeETGenerator::GetMethodRef
(
    Procedure *Method,
    GenericBinding *GenericBindingContext
)
{
    return m_UnmanagedToManagedSymbolMap->GetMethod(Method, GenericBindingContext);
}

ConstructorInfo^ DLRTreeETGenerator::GetConstructorRef
(
    Procedure *Method,
    GenericTypeBinding *GenericTypeBindingContext
)
{
    return m_UnmanagedToManagedSymbolMap->GetConstructor(Method, GenericTypeBindingContext);
}

FieldInfo^ DLRTreeETGenerator::GetFieldRef
(
    Variable *Field,
    GenericTypeBinding *GenericTypeBindingContext
)
{
    return m_UnmanagedToManagedSymbolMap->GetField(Field, GenericTypeBindingContext);
}

System::Type^ DLRTreeETGenerator::GetTypeRef
(
    Type *Type
)
{
    return m_UnmanagedToManagedSymbolMap->GetType(Type);
}

array<Expression^>^ DLRTreeETGenerator::CreateExpressionArray
(
    IReadOnlyList<DLRExpressionTree *> *Expressions
)
{
    return CreateExpressionArray<Expression^>(Expressions);
}

template<class EXPRESSION_TREE>
array<EXPRESSION_TREE>^ DLRTreeETGenerator::CreateExpressionArray
(
    IReadOnlyList<DLRExpressionTree *> *Expressions
)
{
    array<EXPRESSION_TREE>^ ExpressionsArray = nullptr;

    if (Expressions)
    {
        ExpressionsArray = gcnew array<EXPRESSION_TREE>(Expressions->Count());
        if(Expressions->Count() > 0)
        {
            for(unsigned ind = 0, Count = Expressions->Count(); ind < Count; ind++)
            {
                ExpressionsArray[ind] = (EXPRESSION_TREE)FromDLRExpressionTree((*Expressions)[ind]);
            }
        }
    }

    return ExpressionsArray;
}

DLRExpressionTree *DLRTreeETGenerator::GetLambdaParamExpr
(
    LambdaParamHandle *LambdaParameter
)
{
    ThrowIfNull(LambdaParameter);

    // Cast the handle to the actual derived implementation class LambdaParameterInfo.
    LambdaParameterInfo *Info = (LambdaParameterInfo *)LambdaParameter;

    DLRExpressionTree *ParamExpr = Info->GetParameterExpr();
    ThrowIfNull(ParamExpr);

    return ParamExpr;
}

DLRExpressionTree* DLRTreeETGenerator::ToDLRExpressionTree
(
    Expression^ Expr
)
{
    if (Expr == nullptr)
    {
        return NULL;
    }

    unsigned Key = m_NextAvailableKey;
    ThrowIfFalse(Key > 0);
    m_NextAvailableKey++;

    m_ExprMap->Add(Key, Expr);

    return (DLRExpressionTree *)Key;

}

Expression^ DLRTreeETGenerator::FromDLRExpressionTree
(
    DLRExpressionTree *ExprTree
)
{
    if (ExprTree == NULL)
    {
        return nullptr;
    }

#pragma warning (push)
#pragma warning (disable:4311)
#pragma warning (disable:4302)
	int Key = (int)ExprTree;
#pragma warning (pop)

    return m_ExprMap->default[Key];
}
