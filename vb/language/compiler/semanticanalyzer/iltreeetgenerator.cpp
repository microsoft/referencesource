//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"
#include "ILTreeETGenerator.h"

#if DEBUG

#include "DebugExpressionTreeSemantics.h"

// This ensures that the constructor for DebugExpressionTreeSemantics is run
// at compiler startup (in debug only) which then checks that all the expression
// kinds are anticipated in ET semantics.
//
#pragma prefast(suppress: 22109, "Debug only.")
DebugExpressionTreeSemantics g_DebugETObject;

#endif DEBUG


ILTree::Expression* ILTreeETGenerator::CreateBadExpr
(
    Type *ExprType,     // This is the result type of the expression for which the the bad expr node is being created
    Type *TargetType,   // This is the type of the resulting expr tree node
    Location& Loc
)
{
    ThrowIfNull(TargetType);

    return AllocateBadExpression(TargetType, Loc);
}

ILTree::Expression* ILTreeETGenerator::CreateConvertExpr
(
    ILTree::Expression *Operand,
    Type *TypeToConvertTo,
    bool UseCheckedSemantics,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Operand);
    ThrowIfNull(TypeToConvertTo);

    m_PH.SetTextSpan(Loc);

    STRING *MethodName = MapBilopToOperatorNode(SX_CTYPE, UseCheckedSemantics);
    ThrowIfNull(MethodName);

    ParseTree::ArgumentList* Arguments = m_PH.CreateArgList(
        2,
        m_PH.CreateBoundExpression(Operand),
        m_PH.CreateBoundExpression(CreateTypeRef(TypeToConvertTo, Loc))
        );

    return GenerateExpressionTreeCall(
        MethodName,
        Arguments,
        TargetType
        );
}

ILTree::Expression* ILTreeETGenerator::CreateConvertExpr
(
    ILTree::Expression *Operand,
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

    m_PH.SetTextSpan(Loc);

    STRING *MethodName = MapBilopToOperatorNode(SX_CTYPE, UseCheckedSemantics);
    ThrowIfNull(MethodName);

    ILTree::Expression *ImplementingMethodRef = CreateMethodRef(
        ImplementingMethod,
        MethodBindingContext,
        Loc
        );

    ParseTree::ArgumentList* Arguments = m_PH.CreateArgList(
        3,
        m_PH.CreateBoundExpression(Operand),
        m_PH.CreateBoundExpression(CreateTypeRef(TypeToConvertTo, Loc)),
        m_PH.CreateBoundExpression(ImplementingMethodRef)
        );

    return GenerateExpressionTreeCall(
        MethodName,
        Arguments,
        TargetType
        );
}

ILTree::Expression* ILTreeETGenerator::CreateTypeAsExpr
(
    ILTree::Expression *Operand,
    Type *TypeToConvertTo,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Operand);
    ThrowIfNull(TypeToConvertTo);

    m_PH.SetTextSpan(Loc);

    STRING *MethodName = STRING_CONST(m_Compiler, ComExpressionTypeAs);
    ThrowIfNull(MethodName);

    ParseTree::ArgumentList* Arguments = m_PH.CreateArgList(
        2,
        m_PH.CreateBoundExpression(Operand),
        m_PH.CreateBoundExpression(CreateTypeRef(TypeToConvertTo, Loc))
        );

    return GenerateExpressionTreeCall(
        MethodName,
        Arguments,
        TargetType
        );
}

ILTree::Expression* ILTreeETGenerator::CreateCastExpr
(
    ILTree::Expression *Operand,
    Type *TypeToConvertTo,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Operand);
    ThrowIfNull(TypeToConvertTo);

    return CreateConvertExpr(
        Operand,
        TypeToConvertTo,
        false,   // No int overflow checks
        TargetType,
        Loc
        );
}

ILTree::Expression* ILTreeETGenerator::CreateArrayIndexExpr
(
    ILTree::Expression *Array,
    ILTree::Expression *Index,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Array);
    ThrowIfNull(Index);

    m_PH.SetTextSpan(Loc);

    STRING* MethodName = STRING_CONST(m_Compiler, ComExpressionArrayIndex);
    ThrowIfNull(MethodName);

    ParseTree::ArgumentList *Arguments = m_PH.CreateArgList(
        2,
        m_PH.CreateBoundExpression(Array),
        m_PH.CreateBoundExpression(Index)
        );

    return GenerateExpressionTreeCall(
        MethodName,
        Arguments,
        TargetType
        );
}

ILTree::Expression* ILTreeETGenerator::CreateArrayIndexExpr
(
    ILTree::Expression *Array,
    IReadOnlyList<ILTree::Expression *> *Indices,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Array);
    ThrowIfFalse(Indices && Indices->Count() > 0);

    m_PH.SetTextSpan(Loc);

    STRING *MethodName = STRING_CONST( m_Compiler, ComExpressionArrayIndex );
    ThrowIfNull(MethodName);

    ParseTree::Expression *IndicesArray = BuildExpressionArray(
        GetFXSymbolProvider()->GetExpressionType(),
        Indices,
        Loc
        );

    ParseTree::ArgumentList *Arguments = m_PH.CreateArgList(
        2,
        m_PH.CreateBoundExpression(Array),
        IndicesArray
        );

    return GenerateExpressionTreeCall(
        MethodName,
        Arguments,
        TargetType
        );
}

ILTree::Expression* ILTreeETGenerator::CreateArrayLengthExpr
(
    ILTree::Expression *Array,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Array);

    m_PH.SetTextSpan(Loc);

    STRING *MethodName = STRING_CONST( m_Compiler, ComExpressionArrayLength );
    ThrowIfNull(MethodName);

    ParseTree::ArgumentList *Arguments = m_PH.CreateArgList(
        1,
        m_PH.CreateBoundExpression(Array)
        );

    return GenerateExpressionTreeCall(
        MethodName,
        Arguments,
        TargetType
        );
}

// Create an array given the bounds

ILTree::Expression* ILTreeETGenerator::CreateNewArrayBoundsExpr
(
    Type *ElementType,
    IReadOnlyList<ILTree::Expression *> *Bounds,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(ElementType);

    m_PH.SetTextSpan(Loc);

    STRING* MethodName = STRING_CONST(m_Compiler, ComExpressionNewArrayBounds);
    ThrowIfNull(MethodName);

    ParseTree::Expression *BoundsArray = BuildExpressionArray(
        GetFXSymbolProvider()->GetExpressionType(),
        Bounds,
        Loc
        );

    ParseTree::Expression *ElementTypeRef = m_PH.CreateBoundExpression(
        CreateTypeRef(ElementType, Loc)
        );

    ParseTree::ArgumentList *Arguments = m_PH.CreateArgList(
        2,
        ElementTypeRef,
        BoundsArray
        );

    return GenerateExpressionTreeCall(
        MethodName,
        Arguments,
        TargetType
        );
}

// Create a 1-D array of the specified type and initialized with the specified elements

ILTree::Expression* ILTreeETGenerator::CreateNewArrayInitExpr
(
    Type *ElementType,
    IReadOnlyList<ILTree::Expression *> *Elements,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(ElementType);

    m_PH.SetTextSpan(Loc);

    STRING* MethodName = STRING_CONST(m_Compiler, ComExpressionNewArrayInit);
    ThrowIfNull(MethodName);

    ParseTree::Expression *ElementsArray = BuildExpressionArray(
        GetFXSymbolProvider()->GetExpressionType(),
        Elements,
        Loc
        );

    ParseTree::Expression *ElementTypeRef = m_PH.CreateBoundExpression(
        CreateTypeRef(ElementType, Loc)
        );

    ParseTree::ArgumentList *Arguments = m_PH.CreateArgList(
        2,
        ElementTypeRef,
        ElementsArray
        );

    return GenerateExpressionTreeCall(
        MethodName,
        Arguments,
        TargetType
        );
}

ILTree::Expression* ILTreeETGenerator::CreateListInitExpr
(
    ILTree::Expression *ListExpr,
    IReadOnlyList<ListElementInitInfo<ILTree::Expression>> *Initializers,
    Type *TargetType,
    const Location &Loc
)   
{
    ThrowIfNull(ListExpr);

    m_PH.SetTextSpan(Loc);

    STRING* MethodName = STRING_CONST(m_Compiler, ComExpressionListInit);
    ThrowIfNull(MethodName);

    ParseTree::Expression *InitializersArray = NULL;

    if (Initializers && Initializers->Count() > 0)
    {
        ParseTree::NewArrayInitializerExpression *Array = m_PH.CreateNewArray(
            m_PH.CreateBoundType(GetFXSymbolProvider()->GetElementInitType())
            );

        for(unsigned i = 0, Count = Initializers->Count(); i < Count; i++)
        {
            const ListElementInitInfo<ILTree::Expression> &Initializer = (*Initializers)[i];

            ThrowIfNull(Initializer.AddMethod);
            ThrowIfNull(Initializer.Arguments);

            ILTree::Expression *ElementInitExpr = CreateElementInitExpr(
                Initializer.AddMethod,
                Initializer.BindingContext,
                Initializer.Arguments,
                GetFXSymbolProvider()->GetElementInitType(),
                Loc
                );

            m_PH.AddElementInitializer(
                Array,
                m_PH.CreateBoundExpression(ElementInitExpr)
                );
        }

        InitializersArray = Array;
    }
    else
    {
        InitializersArray = m_PH.CreateBoundExpression(
            AllocateExpression(
                SX_NOTHING,
                m_SymbolCreator.GetArrayType(1, GetFXSymbolProvider()->GetElementInitType()),
                Loc
                )
            );
    }

    ParseTree::ArgumentList * ListInitMethodArgs = m_PH.CreateArgList(
        2,
        m_PH.CreateBoundExpression(ListExpr),
        InitializersArray
        );

    return GenerateExpressionTreeCall(
        MethodName,
        ListInitMethodArgs,
        TargetType
        );    
}

ILTree::Expression* ILTreeETGenerator::CreateMemberInitExpr
(
    ILTree::Expression *NewExpr,
    IReadOnlyList<MemberAssignmentInfo<ILTree::Expression>> *Initializers,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(NewExpr);

    m_PH.SetTextSpan(Loc);

    STRING* MethodName = STRING_CONST(m_Compiler, ComExpressionMemberInit);
    ThrowIfNull(MethodName);

    ParseTree::Expression *InitializersArray = NULL;

    if (Initializers && Initializers->Count() > 0)
    {
        ParseTree::NewArrayInitializerExpression *Array = m_PH.CreateNewArray(
            m_PH.CreateBoundType(GetFXSymbolProvider()->GetMemberBindingType())
            );

        for(unsigned i = 0, Count = Initializers->Count(); i < Count; i++)
        {
            const MemberAssignmentInfo<ILTree::Expression> &Current = (*Initializers)[i];

            ILTree::Expression *MemberRef = Current.Member->IsVariable() ?
                CreateFieldRef(
                    Current.Member->PVariable(), 
                    Current.BindingContext,
                    Loc
                    ) :
                CreateMethodRef(
                    Current.Member->PProc(),
                    Current.BindingContext,
                    Loc
                    );

            ParseTree::ArgumentList *BindMethodArgs = m_PH.CreateArgList(
                2,
                m_PH.CreateBoundExpression(MemberRef),
                m_PH.CreateBoundExpression(Current.Value)
                );

            STRING *BindMethodName = STRING_CONST(m_Compiler, ComExpressionBind);
            ThrowIfNull(BindMethodName);

            ParseTree::Expression *MemberAssignment = m_PH.CreateMethodCall(
                m_PH.CreateExpressionTreeNameExpression(
                    m_Compiler,
                    BindMethodName
                    ),
                BindMethodArgs
                );

            m_PH.AddElementInitializer(
                Array,
                MemberAssignment
                );
        }

        InitializersArray = Array;
    }
    else
    {
        InitializersArray = m_PH.CreateBoundExpression(
            AllocateExpression(
                SX_NOTHING,
                m_SymbolCreator.GetArrayType(1, GetFXSymbolProvider()->GetMemberBindingType()),
                Loc
                )
            );
    }


    ParseTree::ArgumentList * MemberInitMethodArgs = m_PH.CreateArgList(
        2,
        m_PH.CreateBoundExpression(NewExpr),
        InitializersArray
        );

    return GenerateExpressionTreeCall(
        MethodName,
        MemberInitMethodArgs,
        TargetType
        );
}

ILTree::Expression* ILTreeETGenerator::CreateFieldExpr
(
    BCSYM_Variable *Field,
    GenericTypeBinding *GenericBindingContext,
    ILTree::Expression *Instance,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Field);

    m_PH.SetTextSpan(Loc);

    STRING* MethodName = STRING_CONST(m_Compiler, ComExpressionField);
    ThrowIfNull(MethodName);

    if (Instance == NULL)
    {
        VSASSERT(Field->IsShared(), "Non-static method requires an instance");
        
        Instance = AllocateExpression(
            SX_NOTHING,
            GetFXSymbolProvider()->GetExpressionType(),
            Loc
            );
    }

    ILTree::Expression *FieldRef = CreateFieldRef(Field, GenericBindingContext, Loc);

    ParseTree::ArgumentList *Arguments = m_PH.CreateArgList(
        2,
        m_PH.CreateBoundExpression(Instance),
        m_PH.CreateBoundExpression(FieldRef)
        );

   return GenerateExpressionTreeCall(
        MethodName,
        Arguments,
        TargetType
        );
}

ILTree::Expression* ILTreeETGenerator::CreateCallExpr
(
    Procedure *Proc,
    GenericBinding *GenericBindingContext,
    ILTree::Expression *Instance,
    IReadOnlyList<ILTree::Expression *> *Arguments,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Proc);

    m_PH.SetTextSpan(Loc);

    STRING* MethodName = STRING_CONST(m_Compiler, ComExpressionCall);
    ThrowIfNull(MethodName);

    if (Instance == NULL)
    {
        VSASSERT(Proc->IsShared(), "Non-static method requires an instance");
        
        Instance = AllocateExpression(
            SX_NOTHING,
            GetFXSymbolProvider()->GetExpressionType(),
            Loc
            );
    }

    ParseTree::Expression *ArgumentsArray = BuildExpressionArray(
        GetFXSymbolProvider()->GetExpressionType(),
        Arguments,
        Loc,
        GetFXSymbolProvider()->GetObjectType()	// Note that the type used is Object rather than Expression array.
                                                // This is done to maintain compat with orcas. Don't know if this
                                                // level of compat is required, but ensuring the compat.
        );

    ILTree::Expression *ProcReference = CreateMethodRef(Proc, GenericBindingContext, Loc);

    ParseTree::ArgumentList *CallMethodArguments = m_PH.CreateArgList(
        3,
        m_PH.CreateBoundExpression(Instance),
        m_PH.CreateBoundExpression(ProcReference),
        ArgumentsArray
        );

   return GenerateExpressionTreeCall(
        MethodName,
        CallMethodArguments,
        TargetType
        );
}

ILTree::Expression* ILTreeETGenerator::CreateInvokeExpr
(
    ILTree::Expression *DelegateInstance,
    IReadOnlyList<ILTree::Expression *> *Arguments,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(DelegateInstance);

    m_PH.SetTextSpan(Loc);

    STRING* MethodName = STRING_CONST( m_Compiler, ComExpressionInvoke );
    ThrowIfNull(MethodName);

    ParseTree::Expression *ArgumentsArray = BuildExpressionArray(
        GetFXSymbolProvider()->GetExpressionType(),
        Arguments,
        Loc,
        GetFXSymbolProvider()->GetObjectType()	// Note that the type used is Object rather than Expression array.
                                                // This is done to maintain compat with orcas. Don't know if this
                                                // level of compat is required, but ensuring the compat.
        );

    ParseTree::ArgumentList *CallMethodArguments = m_PH.CreateArgList(
        2,
        m_PH.CreateBoundExpression(DelegateInstance),
        ArgumentsArray
        );

   return GenerateExpressionTreeCall(
        MethodName,
        CallMethodArguments,
        TargetType
        );
}

ILTree::Expression* ILTreeETGenerator::CreatePropertyAccessExpr
(
    Procedure *PropertyGet,
    GenericBinding *GenericBindingContext,
    ILTree::Expression *Instance,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfFalse(PropertyGet && PropertyGet->IsPropertyGet());

    m_PH.SetTextSpan(Loc);

    STRING* MethodName = STRING_CONST(m_Compiler, ComExpressionProperty);
    ThrowIfNull(MethodName);

    if (Instance == NULL)
    {
        VSASSERT(PropertyGet->IsShared(), "Non-static method requires an instance");
        
        Instance = AllocateExpression(
            SX_NOTHING,
            GetFXSymbolProvider()->GetExpressionType(),
            Loc
            );
    }

    ILTree::Expression *ProcReference = CreateMethodRef(PropertyGet, GenericBindingContext, Loc);

    ParseTree::ArgumentList *Arguments = m_PH.CreateArgList(
        2,
        m_PH.CreateBoundExpression(Instance),
        m_PH.CreateBoundExpression(ProcReference)
        );

   return GenerateExpressionTreeCall(
        MethodName,
        Arguments,
        TargetType
        );
}

ILTree::Expression* ILTreeETGenerator::CreateTypeIsExpr
(
    ILTree::Expression *ExprOperand,
    Type *TypeOperand,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(ExprOperand);
    ThrowIfNull(TypeOperand);

    m_PH.SetTextSpan(Loc);

    STRING *MethodName = STRING_CONST(m_Compiler, ComExpressionTypeIs);
    ThrowIfNull(MethodName);

    ParseTree::ArgumentList* Arguments = m_PH.CreateArgList(
        2,
        m_PH.CreateBoundExpression(ExprOperand),
        m_PH.CreateBoundExpression(CreateTypeRef(TypeOperand, Loc))
        );

    return GenerateExpressionTreeCall(
        MethodName,
        Arguments,
        TargetType
        );
}

ILTree::Expression* ILTreeETGenerator::CreateNegateExpr
(
    ILTree::Expression *Operand,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Operand);

    m_PH.SetTextSpan(Loc);

    STRING *MethodName = STRING_CONST(m_Compiler, ComExpressionNegate);
    ThrowIfNull(MethodName);

    ParseTree::ArgumentList *Arguments = m_PH.CreateArgList(
        1,
        m_PH.CreateBoundExpression(Operand)
        );

    return GenerateExpressionTreeCall(
        MethodName,
        Arguments,
        TargetType
        );
}

ILTree::Expression* ILTreeETGenerator::CreateUnaryOperatorExpr
(
    BILOP Operator,
    ILTree::Expression *Operand,
    bool UseCheckedSemantics,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfFalse(SX_PLUS == Operator ||
                 SX_NEG == Operator ||
                 SX_NOT == Operator);
    ThrowIfNull(Operand);

    m_PH.SetTextSpan(Loc);

    STRING* MethodName = MapBilopToOperatorNode(Operator, UseCheckedSemantics);
    ThrowIfNull(MethodName);
    
    ParseTree::ArgumentList *Arguments = m_PH.CreateArgList(
        1,
        m_PH.CreateBoundExpression(Operand)
        );

    return GenerateExpressionTreeCall(
        MethodName,
        Arguments,
        TargetType
        );
}

ILTree::Expression* ILTreeETGenerator::CreateUnaryOperatorExpr
(
    BILOP Operator,
    ILTree::Expression *Operand,
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

    m_PH.SetTextSpan(Loc);

    STRING* MethodName = MapBilopToOperatorNode(Operator, UseCheckedSemantics);
    ThrowIfNull(MethodName);

    ILTree::Expression *ImplementingMethodRef = CreateMethodRef(
        ImplementingMethod,
        MethodBindingContext,
        Loc
        );

    ParseTree::ArgumentList *Arguments = m_PH.CreateArgList(
        2,
        m_PH.CreateBoundExpression(Operand),
        m_PH.CreateBoundExpression(ImplementingMethodRef)
        );

    return GenerateExpressionTreeCall(
        MethodName,
        Arguments,
        TargetType
        );
}

ILTree::Expression* ILTreeETGenerator::CreateBinaryOperatorExpr
(
    BILOP Operator,
    ILTree::Expression *LeftOperand,
    ILTree::Expression *RightOperand,
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

    m_PH.SetTextSpan(Loc);

    STRING* MethodName = MapBilopToOperatorNode(Operator, UseCheckedSemantics);
    ThrowIfNull(MethodName);

    ParseTree::ArgumentList *Arguments = m_PH.CreateArgList(
        2,
        m_PH.CreateBoundExpression(LeftOperand),
        m_PH.CreateBoundExpression(RightOperand)
        );

    return GenerateExpressionTreeCall(
        MethodName,
        Arguments,
        TargetType
        );
}

ILTree::Expression* ILTreeETGenerator::CreateBinaryOperatorExpr
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

    m_PH.SetTextSpan(Loc);

    STRING* MethodName = MapBilopToOperatorNode(Operator, UseCheckedSemantics);
    ThrowIfNull(MethodName);

    ILTree::Expression *ImplementingMethodRef = CreateMethodRef(
        ImplementingMethod,
        MethodBindingContext,
        Loc
        );

    ParseTree::ArgumentList *Arguments = NULL;
    
    switch( Operator )
    {
    case SX_ADD:
    case SX_SUB:
    case SX_MUL:
    case SX_IDIV:
    case SX_DIV:
    case SX_MOD:
    case SX_POW:
    case SX_AND:
    case SX_OR:
    case SX_XOR:
    case SX_SHIFT_LEFT:
    case SX_SHIFT_RIGHT:
    case SX_ANDALSO:
    case SX_ORELSE:
        
        Arguments = m_PH.CreateArgList(
            3,
            m_PH.CreateBoundExpression(LeftOperand),
            m_PH.CreateBoundExpression(RightOperand),
            m_PH.CreateBoundExpression(ImplementingMethodRef)
            );
        break;

    case SX_IS:
    case SX_EQ:
    case SX_ISNOT:
    case SX_NE:
    case SX_LT:
    case SX_GT:
    case SX_GE:
    case SX_LE:
    
        Arguments = m_PH.CreateArgList(
            4,
            m_PH.CreateBoundExpression(LeftOperand),
            m_PH.CreateBoundExpression(RightOperand),
            m_PH.CreateBoundExpression(
                m_Semantics->ProduceConstantExpression(
                    LiftToNull ? 1 : 0,
                    Loc,
                    GetFXSymbolProvider()->GetBooleanType()
                    IDE_ARG(0)
                    )
                ),
            m_PH.CreateBoundExpression(ImplementingMethodRef)
            );
        break;

    default:
        IfFalseThrow(false);
    }

    return GenerateExpressionTreeCall(
        MethodName,
        Arguments,
        TargetType
        );
}

ILTree::Expression* ILTreeETGenerator::CreateTernaryIfExpr
(
    ILTree::Expression *Condition,
    ILTree::Expression *TrueValue,
    ILTree::Expression *FalseValue,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Condition);
    ThrowIfNull(TrueValue);
    ThrowIfNull(FalseValue);

    m_PH.SetTextSpan(Loc);

    STRING *MethodName = STRING_CONST(m_Compiler, ComExpressionCondition);
    ThrowIfNull(MethodName);

    ParseTree::ArgumentList *Arguments = m_PH.CreateArgList(
        3,
        m_PH.CreateBoundExpression(Condition),
        m_PH.CreateBoundExpression(TrueValue),
        m_PH.CreateBoundExpression(FalseValue)
        );

    return GenerateExpressionTreeCall(
        MethodName,
        Arguments,
        TargetType
        );
}

// ----------------------------------------------------------------------------
// This method returns an expression to create a coalesce expression parameter.
// ----------------------------------------------------------------------------

ILTree::Expression* ILTreeETGenerator::CreateCoalesceExpr
(
    ILTree::Expression *LeftOperand,
    ILTree::Expression *RightOperand,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(LeftOperand);
    ThrowIfNull(RightOperand);

    m_PH.SetTextSpan(Loc);

    STRING *MethodName = STRING_CONST( m_Compiler, ComExpressionCoalesce );
    ThrowIfNull(MethodName);

    ParseTree::ArgumentList *Arguments = m_PH.CreateArgList(
        2,
        m_PH.CreateBoundExpression(LeftOperand),
        m_PH.CreateBoundExpression(RightOperand)
        );

    return GenerateExpressionTreeCall(
        MethodName,
        Arguments,
        TargetType
        );
}

// ----------------------------------------------------------------------------
// This method returns an expression to create a coalesce expression parameter.
// But along with the normal operands, it also accepts a lambda to perform
// a conversion from the LeftOperand type.
// ----------------------------------------------------------------------------

ILTree::Expression* ILTreeETGenerator::CreateCoalesceExpr
(
    ILTree::Expression *LeftOperand,
    ILTree::Expression *RightOperand,
    ILTree::Expression *ConversionLambda,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(LeftOperand);
    ThrowIfNull(RightOperand);
    ThrowIfNull(ConversionLambda);

    m_PH.SetTextSpan(Loc);

    STRING *MethodName = STRING_CONST( m_Compiler, ComExpressionCoalesce );
    ThrowIfNull(MethodName);

    ParseTree::ArgumentList *Arguments = m_PH.CreateArgList(
        3,
        m_PH.CreateBoundExpression(LeftOperand),
        m_PH.CreateBoundExpression(RightOperand),
        m_PH.CreateBoundExpression(ConversionLambda)
        );

    return GenerateExpressionTreeCall(
        MethodName,
        Arguments,
        TargetType
        );
}

// ----------------------------------------------------------------------------
// This method returns a LambdaParamHandle which encapsulates both the
// temporary generated for the lambda parameter as well as the lambda parameter
// creation expression.
// ----------------------------------------------------------------------------

LambdaParamHandle* ILTreeETGenerator::CreateLambdaParameter
(
    _In_z_ STRING *ParameterName,
    Type *ParameterType,
    const Location &Loc
)
{
    ThrowIfNull(ParameterName);
    ThrowIfNull(ParameterType);

    m_PH.SetTextSpan(Loc);
    
    STRING *MethodName = STRING_CONST(m_Compiler, ComExpressionParameter);
    ThrowIfNull(MethodName);

    Variable *LambdaParameterTemp = NULL;

    ParseTree::ArgumentList *Arguments = m_PH.CreateArgList(
        2,
        m_PH.CreateBoundExpression(CreateTypeRef(ParameterType, Loc)),
        m_PH.CreateStringConst(ParameterName)
        );

    ILTree::Expression *LambdaParamCreationExpr = GenerateExpressionTreeCall(
        MethodName,
        Arguments,
        NULL
        );

    // capture in temporary so that the temp can be referred to whenever
    // the lambda parameter needs to be referred to.

    LambdaParamCreationExpr = m_Semantics->CaptureInShortLivedTemporary(
        LambdaParamCreationExpr,
        LambdaParameterTemp
        );

    return new (*m_Allocator) LambdaParameterInfo(
        LambdaParamCreationExpr,
        LambdaParameterTemp
        );
}

// ----------------------------------------------------------------------------
// This method returns an expression that loads a previously created lambda
// parameter expression from a temporary corresponding to that lambda parameter.
// ----------------------------------------------------------------------------

ILTree::Expression* ILTreeETGenerator::CreateLambdaParamReferenceExpr
(
    LambdaParamHandle *LambdaParameter,
    const Location &Loc
)
{
    ThrowIfNull(LambdaParameter);

    m_PH.SetTextSpan(Loc);

    // Cast the handle to the actual derived implementation class LambdaParameterInfo.
    LambdaParameterInfo *ParamInfo = (LambdaParameterInfo *)LambdaParameter;

    Variable *LambdaParameterTemp = ParamInfo->GetParameterTemp();
    ThrowIfNull(LambdaParameterTemp);

    // This just needs to return the lambda parameter expression which is already
    // stored in a corresponding temporary for each lambda parameter.
    //
    // Note that for lambda parameters, the expression tree is not built
    // each time, instead it is built only once (lambda parameter creation) and
    // that expression tree used at all the referencing locations.

    return AllocateSymbolReference(
        LambdaParameterTemp,
        LambdaParameterTemp->GetType(),
        NULL,
        Loc,
        NULL
        );
}


// ----------------------------------------------------------------------------
// This method returns an expression that creates a lambda expression.
// ----------------------------------------------------------------------------

ILTree::Expression* ILTreeETGenerator::CreateLambdaExpr
(
    IReadOnlyList<LambdaParamHandle *> *Parameters,
    ILTree::Expression *LambdaBody,
    Type *DelegateType,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(LambdaBody);
    ThrowIfNull(DelegateType);

    m_PH.SetTextSpan(Loc);

    STRING* MethodName = STRING_CONST( m_Compiler, ComExpressionLambda );
    ThrowIfNull(MethodName);

    ILTree::Expression *ParamCreation = NULL;

    // Build the array for the lambda parameters
    //
    // Simultaneously also build the sequence of the expressions previously
    // already built to create the lambda parameters.

    ParseTree::Expression *ParametersArray = NULL;

    if (Parameters && Parameters->Count() > 0)
    {
        ParseTree::NewArrayInitializerExpression *Array = m_PH.CreateNewArray(
            m_PH.CreateBoundType(GetFXSymbolProvider()->GetParameterExpressionType())
            );

        for(unsigned i = 0, Count = Parameters->Count(); i < Count; i++)
        {
            LambdaParameterInfo *ParamInfo = (LambdaParameterInfo *)(*Parameters)[i];

            if (ParamCreation == NULL)
            {
                ParamCreation = ParamInfo->GetParameterCreationExpr();
            }
            else
            {
                ParamCreation = BuildExpressionSequence(
                    ParamCreation,
                    ParamInfo->GetParameterCreationExpr(),
                    Loc
                    );
            }

            m_PH.AddElementInitializer(
                Array,
                m_PH.CreateBoundExpression(CreateLambdaParamReferenceExpr((*Parameters)[i], Loc))
                );
        }

        ParametersArray = Array;
    }
    else
    {
        ParametersArray = m_PH.CreateBoundExpression(
            AllocateExpression(
                SX_NOTHING,
                m_SymbolCreator.GetArrayType(1, GetFXSymbolProvider()->GetParameterExpressionType()),
                Loc
                )
            );
    }

    // Build the type param list to pass in the delegate as the type param
    ParseTree::TypeList *TypeArgs = m_PH.CreateTypeList( 1, m_PH.CreateBoundType(DelegateType));

    // Build the arguments list for the call

    ParseTree::ArgumentList *Arguments = m_PH.CreateArgList(
        2,
        m_PH.CreateBoundExpression( LambdaBody ),
        ParametersArray
        );
    
    // Build the call to the lambda now

    ILTree::Expression *Lambda = GenerateExpressionTreeCall(
        MethodName,
        Arguments,
        TargetType,
        TypeArgs
        );

    if (ParamCreation != NULL)
    {
        Lambda = BuildExpressionSequence(
            ParamCreation,
            Lambda,
            Loc
            );
    }

    return Lambda;
}

ILTree::Expression* ILTreeETGenerator::CreateQuoteExpr
(
    ILTree::Expression *Operand,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Operand);

    m_PH.SetTextSpan(Loc);

    STRING* MethodName = STRING_CONST(m_Compiler, ComExpressionQuote);
    ThrowIfNull(MethodName);

    // Build the arguments list for the call

    ParseTree::ArgumentList *Arguments = m_PH.CreateArgList(
        1,
        m_PH.CreateBoundExpression(Operand)
        );
    
    // Build the call now

    return GenerateExpressionTreeCall(
        MethodName,
        Arguments,
        TargetType
        );
}

ILTree::Expression* ILTreeETGenerator::CreateNewExpr
(
    Type *InstanceType,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(InstanceType);

    m_PH.SetTextSpan(Loc);

    STRING *MethodName = STRING_CONST(m_Compiler, ComExpressionNew);
    ThrowIfNull(MethodName);

    // Create the argument list for the call to the factory method
    ParseTree::ArgumentList *Arguments = m_PH.CreateArgList(
        1,
        m_PH.CreateBoundExpression(CreateTypeRef(InstanceType, Loc))
        );

    // Generate the call to the factory method
    return GenerateExpressionTreeCall(
        MethodName,
        Arguments,
        TargetType
        );
}

ILTree::Expression* ILTreeETGenerator::CreateNewExpr
(
    Procedure *Constructor,
    GenericTypeBinding *GenericTypeBindingContext,
    IReadOnlyList<ILTree::Expression *> *Arguments,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Constructor);

    m_PH.SetTextSpan(Loc);

    STRING *MethodName = STRING_CONST(m_Compiler, ComExpressionNew);
    ThrowIfNull(MethodName);

    // Create the arguments array
    ParseTree::Expression *ArgumentsArray = BuildExpressionArray(
        GetFXSymbolProvider()->GetExpressionType(),
        Arguments,
        Loc,
        GetFXSymbolProvider()->GetObjectType()	// Note that the type used is Object rather than Expression array.
                                                // This is done to maintain compat with orcas. Don't know if this
                                                // level of compat is required, but ensuring the compat.
        );

    // Generate a reference to the constructor
    ParseTree::Expression *ConstructorRef = m_PH.CreateBoundExpression(
        CreateMethodRef(Constructor, GenericTypeBindingContext, Loc)
        );

    // Create the argument list for the call to the factory method
    ParseTree::ArgumentList *ArgumentsList = m_PH.CreateArgList(
        2,
        ConstructorRef,
        ArgumentsArray
        );

    // Generate the call to the factory method
    return GenerateExpressionTreeCall(
        MethodName,
        ArgumentsList,
        TargetType
        );
}

ILTree::Expression* ILTreeETGenerator::CreateNewExpr
(
    Procedure *Constructor,
    GenericTypeBinding *GenericTypeBindingContext,
    IReadOnlyList<PropertyAssignmentInfo<ILTree::Expression>> *Initializers,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Constructor);

    m_PH.SetTextSpan(Loc);

    STRING *MethodName = STRING_CONST(m_Compiler, ComExpressionNew);
    ThrowIfNull(MethodName);

    // We need to match expressions with their properties in the expression tree
    // constructor.

    ParseTree::NewArrayInitializerExpression* MemberArray = m_PH.CreateNewArray(
        m_PH.CreateBoundType(GetFXSymbolProvider()->GetMethodInfoType())
        );

    ParseTree::NewArrayInitializerExpression* ExpressionArray = m_PH.CreateNewArray(
        m_PH.CreateBoundType(GetFXSymbolProvider()->GetExpressionType())
        );

    if (Initializers)
    {
        for(unsigned i = 0, Count = Initializers->Count(); i < Count; i++)
        {
            const PropertyAssignmentInfo<ILTree::Expression> &Initializer = (*Initializers)[i];

            Procedure *PropertyGetter = Initializer.PropertyGetter;
            GenericBinding *PropertyGenericContext = Initializer.BindingContext;
            ILTree::Expression *Value = Initializer.Value;

            ThrowIfFalse(PropertyGetter && PropertyGetter->IsPropertyGet());
            ThrowIfNull(Value);

            // Generate a reference to the property getter's MethodInfo

            ILTree::Expression *PropertyGetterRef = CreateMethodRef(
                PropertyGetter,
                PropertyGenericContext,
                Loc
                );

            m_PH.AddElementInitializer(
                MemberArray,
                m_PH.CreateBoundExpression(PropertyGetterRef)
                );
        
            m_PH.AddElementInitializer(
                ExpressionArray,
                m_PH.CreateBoundExpression(Value)
                );
        }
    }

    // Generate a reference to the constructor
    ParseTree::Expression *ConstructorRef = m_PH.CreateBoundExpression(
        CreateMethodRef(
            Constructor,
            GenericTypeBindingContext,
            Loc
            )
        );

    // Create the argument list for the call to the factory method
    ParseTree::ArgumentList *Arguments = m_PH.CreateArgList(
        3,
        ConstructorRef,
        ExpressionArray,
        MemberArray
        );

    // Generate the call to the factory method
    return GenerateExpressionTreeCall(
        MethodName,
        Arguments,
        TargetType
        );
}

ILTree::Expression* ILTreeETGenerator::CaptureRuntimeValueAsConstant
(
    ILTree::Expression *Value,
    Type *TargetType
)
{
    ThrowIfNull(Value);

    m_PH.SetTextSpan(Value->Loc);

    STRING* MethodName = STRING_CONST(m_Compiler, ComExpressionConstant);
    ThrowIfNull(MethodName);

    ParseTree::ArgumentList *Arguments = m_PH.CreateArgList(
        2,
        m_PH.CreateBoundExpression(Value),
        m_PH.CreateBoundExpression(CreateTypeRef(Value->ResultType, Value->Loc))
        );

    return GenerateExpressionTreeCall(
        MethodName,
        Arguments,
        TargetType
        );
}

ILTree::Expression* ILTreeETGenerator::CreateNothingConstantExpr
(
    Type *NothingType,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(NothingType);

    m_PH.SetTextSpan(Loc);

    STRING* MethodName = STRING_CONST(m_Compiler, ComExpressionConstant);
    ThrowIfNull(MethodName);

    ParseTree::ArgumentList *Arguments = m_PH.CreateArgList(
        2,
        m_PH.CreateNothingConst(),
        m_PH.CreateBoundExpression(CreateTypeRef(NothingType, Loc))
        );

    return GenerateExpressionTreeCall(
        MethodName,
        Arguments,
        TargetType
        );
}

// Internal helpers

// ----------------------------------------------------------------------------
// This method takes 2 expression and appends them so that first expression is
// executed first followed by the second expression. The value of the resulting
// expression is the value of the second expression.
// ----------------------------------------------------------------------------

ILTree::Expression* ILTreeETGenerator::BuildExpressionSequence
(
    ILTree::Expression *Expr1,
    ILTree::Expression *Expr2,
    const Location &Loc
)
{
    ThrowIfNull(Expr1);
    ThrowIfNull(Expr2);

    return AllocateExpression(
        SX_SEQ_OP2,
        Expr2->ResultType,
        Expr1,
        Expr2,
        Loc);
}

ILTree::Expression* ILTreeETGenerator::CreateElementInitExpr
(
    Procedure *Proc,
    GenericBinding *GenericBindingContext,
    IReadOnlyList<ILTree::Expression *> *Arguments,
    Type *TargetType,
    const Location &Loc
)
{
    ThrowIfNull(Proc);
    ThrowIfFalse(Arguments && Arguments->Count() > 0);

    m_PH.SetTextSpan(Loc);

    STRING* MethodName = STRING_CONST(m_Compiler, ComExpressionElementInit);
    ThrowIfNull(MethodName);

    ParseTree::Expression *ArgumentsArray = BuildExpressionArray(
        GetFXSymbolProvider()->GetExpressionType(),
        Arguments,
        Loc
        );

    ILTree::Expression *ProcReference = CreateMethodRef(Proc, GenericBindingContext, Loc);

    ParseTree::ArgumentList *CallMethodArguments = m_PH.CreateArgList(
        2,
        m_PH.CreateBoundExpression(ProcReference),
        ArgumentsArray
        );

   return GenerateExpressionTreeCall(
        MethodName,
        CallMethodArguments,
        TargetType
        );
}

ILTree::Expression* ILTreeETGenerator::CreateTypeRef
(
    Type *Type,
    const Location &Loc
)
{
    if (Type == NULL)
    {
        return AllocateExpression(
            SX_NOTHING,
            GetFXSymbolProvider()->GetTypeType(),
            Loc
            );
    }

    return AllocateExpression(
        SX_METATYPE,
        GetFXSymbolProvider()->GetTypeType(),
        AllocateExpression(
            SX_NOTHING,
            Type,
            Loc
            ),
        Loc
        );
}

ILTree::Expression* ILTreeETGenerator::CreateMethodRef
(
    Procedure *Method,
    GenericBinding *GenericBindingContext,
    const Location &Loc
)
{
    if (Method == NULL)
    {
        return AllocateExpression(
            SX_NOTHING,
            GetFXSymbolProvider()->GetMethodInfoType(),
            Loc
            );
    }

    return AllocateExpression(
        SX_METATYPE,
        Method->IsInstanceConstructor() || Method->IsSharedConstructor() ?
            GetFXSymbolProvider()->GetConstructorInfoType() :
            GetFXSymbolProvider()->GetMethodInfoType(),
        AllocateSymbolReference(
            Method,
            TypeHelpers::GetVoidType(),
            NULL,
            Loc,
            GenericBindingContext
            ),
        Loc
        );
}

ILTree::Expression* ILTreeETGenerator::CreateFieldRef
(
    Variable *Field,
    GenericBinding *GenericBindingContext,
    const Location &Loc
)
{
    if (Field == NULL)
    {
        return AllocateExpression(
            SX_NOTHING,
            GetFXSymbolProvider()->GetFieldInfoType(),
            Loc
            );
    }

    return AllocateExpression(
        SX_METATYPE,
        GetFXSymbolProvider()->GetFieldInfoType(),
        AllocateSymbolReference(
            Field,
            TypeHelpers::GetVoidType(),
            NULL,
            Loc,
            GenericBindingContext
            ),
        Loc
        );
}

// ----------------------------------------------------------------------------
// Given a list of bound expressions, builds an unbound array that
// holds these expressions.
//
// The type of resulting expression is 1-D array of ArrayElementType type.
// But if EmptyArrayExprType is a non-NULL value, then for empty arrays (i.e.
// when elements are provided), the type of the expression is of EmptyArrayExprType 
// type.
// ----------------------------------------------------------------------------

ParseTree::Expression *ILTreeETGenerator::BuildExpressionArray
(
    Type *ArrayElementType,
    IReadOnlyList<ILTree::Expression *> *Elements,
    const Location &Loc,
    Type *EmptyArrayExprType
)
{
    ParseTree::Expression *Result = NULL;

    if (Elements && Elements->Count() > 0)
    {
        ParseTree::NewArrayInitializerExpression *Array =
            m_PH.CreateNewArray(
                m_PH.CreateBoundType(ArrayElementType)
                );

        for(unsigned i = 0, Count = Elements->Count(); i < Count; i++)
        {
            m_PH.AddElementInitializer(
                Array,
                m_PH.CreateBoundExpression((*Elements)[i])
                );
        }

        Result = Array;
    }
    else
    {
        Result = m_PH.CreateBoundExpression(
            AllocateExpression(
                SX_NOTHING,
                EmptyArrayExprType ?
                    EmptyArrayExprType :
                    m_SymbolCreator.GetArrayType(1, ArrayElementType),
                Loc
                )
            );
    }

    return Result;
}

// ----------------------------------------------------------------------------
// Generate the call to the expression tree given the information on the
// expression tree factory method.
// ----------------------------------------------------------------------------

ILTree::Expression* ILTreeETGenerator::GenerateExpressionTreeCall
(
    _In_z_ STRING* MethodName,
    ParseTree::ArgumentList* Arguments,
    Type* TargetType,
    ParseTree::TypeList* TypeArgs
)
{
    ThrowIfNull( MethodName );
    ThrowIfNull( Arguments );

    ParseTree::Expression *ExpressionTree = m_PH.CreateMethodCall(
        TypeArgs ?
            m_PH.CreateGenericQualifiedExpression(
                m_PH.CreateExpressionTreeNameExpression(m_Compiler, MethodName),
                TypeArgs
                ) :
            m_PH.CreateExpressionTreeNameExpression(m_Compiler, MethodName),
        Arguments
        );

    ILTree::Expression *Result = m_Semantics->InterpretExpressionWithTargetType(
        ExpressionTree,
        ExprForceRValue,
        TargetType
        );

    // VSASSERT( !IsBad( Result ), "How can we have a bad expression tree?" );

    return Result;
}

ILTree::Expression* ILTreeETGenerator::AllocateSymbolReference
(
    Declaration *Symbol,
    Type *ResultType,
    ILTree::Expression *BaseReference,
    const Location &TreeLocation,
    GenericBinding *GenericBindingContext
)
{
    return m_Semantics->AllocateSymbolReference(
        Symbol,
        ResultType,
        BaseReference,
        TreeLocation,
        GenericBindingContext
        );
}

ILTree::Expression* ILTreeETGenerator::AllocateExpression
(
    BILOP Opcode,
    Type *ResultType,
    const Location &TreeLocation
)
{
    return m_Semantics->AllocateExpression(
        Opcode,
        ResultType,
        TreeLocation
        );
}

ILTree::Expression* ILTreeETGenerator::AllocateExpression
(
    BILOP Opcode,
    Type *ResultType,
    ILTree::Expression *Operand,
    const Location &TreeLocation
)
{
    return m_Semantics->AllocateExpression(
        Opcode,
        ResultType,
        Operand,
        TreeLocation
        );
}

ILTree::Expression* ILTreeETGenerator::AllocateExpression
(
    BILOP Opcode,
    Type *ResultType,
    ILTree::Expression *Left,
    ILTree::Expression *Right,
    const Location &TreeLocation
)
{
    return m_Semantics->AllocateExpression(
        Opcode,
        ResultType,
        Left,
        Right,
        TreeLocation
        );
}

ILTree::Expression* ILTreeETGenerator::AllocateBadExpression
(
    Type *ResultType,
    const Location &TreeLocation
)
{
    return m_Semantics->AllocateBadExpression(
        ResultType,
        TreeLocation
        );
}

FX::FXSymbolProvider* ILTreeETGenerator::GetFXSymbolProvider()
{
    return m_Semantics->GetFXSymbolProvider();
}
