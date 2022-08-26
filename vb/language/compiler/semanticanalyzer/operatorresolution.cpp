//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implementation of VB operator resolution semantics.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

//-------------------------------------------------------------------------------------------------
//
// For each matching operator in the container, add it to the list. Conversions get added
// in a manner that groups widening and narrowing conversions together.
//
void Semantics::ScanMembers(
    UserDefinedOperators Operator,
    Type *Class,
    _Out_ bool &KeepClimbing,
    _Inout_ CDoubleList<OperatorInfo> & OperatorSet,
    _Inout_ NorlsAllocator &Scratch)
{

    VSASSERT(Class->IsClass(), "Non-class unexpected when scanning for user defined operators!!!");

    ClassOrRecordType *Container = Class->PClass();
    GenericTypeBinding *GenericBindingContext = (Class->IsGenericTypeBinding() ? Class->PGenericTypeBinding() : NULL);

    Identifier *Name = m_Compiler->OperatorToString(Operator);

    // Operator symbols live in the unbindable hash.
    for (Declaration *Member = Container->GetUnBindableChildrenHash()->SimpleBind(Name);
         Member;
         Member = Member->GetNextOfSameName())
    {
        if (IsBad(Member) || !IsUserDefinedOperator(Member))
        {
            continue;
        }

        BCSYM_UserDefinedOperator *Bound = Member->PUserDefinedOperator();

        // Make a check for all the things that would cause us to generate unverifiable code.
        VSASSERT(Bound->IsShared() &&
                 Bound->GetType() &&
                 Bound->GetFirstParam() &&
                 Bound->GetFirstParam()->GetType(),
                 "malformed operator - how did this get into the symbol table?");

        VSASSERT((IsBinaryOperator(Bound->GetOperator()) && Bound->GetParameterCount() == 2) ||
                 (IsUnaryOperator(Bound->GetOperator()) && Bound->GetParameterCount() == 1),
                 "malformed operator - how did this get into the symbol table?");

        // Don't differentiate between conversion operators. Gather all of them together.
        if (Bound->GetOperator() == Operator || IsConversionOperator(Operator))
        {
            // 



            // Make note if this operator shadows. This will stop further searching up the
            // inheritance chain.
            // 
            if (!Bound->IsOverloadsKeywordUsed())
            {
                KeepClimbing = false;
            }

            OperatorInfo *NewInfo = new(Scratch) OperatorInfo;
            NewInfo->Symbol = Bound;
            NewInfo->GenericBindingContext = GenericBindingContext;

            if (OperatorWiden == Bound->GetOperator())
            {
                OperatorSet.InsertFirst(NewInfo);
            }
           else
           {
                OperatorSet.InsertLast(NewInfo);
            }

        }
    }
}

/*=======================================================================================
CollectOperators

Given an operator kind and two types to scan, construct a list of operators by walking
the inheritance hierarchy and adding matching operators to the list.

The second type can be NULL, in which case operators are collected from only the first
type.
=======================================================================================*/
void
Semantics::CollectOperators
(
    UserDefinedOperators Operator,
    Type *Type1,
    Type *Type2,
    _Inout_ CDoubleList<OperatorInfo> & OperatorSet,
    _Inout_ NorlsAllocator &Scratch
)
{
    bool SearchBothTypes = Type2 != NULL;
    Type *CommonAncestor = NULL;

    // Dig through type params to their class constraints

    // User defined operators for nullable value types are defined in the element type
    if (TypeHelpers::IsNullableType(Type1, m_CompilerHost))
    {
        Type1 = TypeHelpers::GetElementTypeOfNullable(Type1, m_CompilerHost);
    }
    if (TypeHelpers::IsNullableType(Type2, m_CompilerHost))
    {
        Type2 = TypeHelpers::GetElementTypeOfNullable(Type2, m_CompilerHost);
    }

    if (TypeHelpers::IsGenericParameter(Type1))
    {
        Type *ClassConstraint =
            GetClassConstraint(
                Type1->PGenericParam(),
                m_CompilerHost,
                &m_TreeStorage,
                false, // don't ReturnArraysAs"System.Array"
                false  // don't ReturnValuesAs"System.ValueType"or"System.Enum"
                );

        if (ClassConstraint)
        {
            Type1 = ClassConstraint;
        }
    }

    if (SearchBothTypes && TypeHelpers::IsGenericParameter(Type2))
    {
        Type *ClassConstraint =
            GetClassConstraint(
                Type2->PGenericParam(),
                m_CompilerHost,
                &m_TreeStorage,
                false, // don't ReturnArraysAs"System.Array"
                false  // don't ReturnValuesAs"System.ValueType"or"System.Enum"
                );      

        if (ClassConstraint)
        {
            Type2 = ClassConstraint;
        }
    }

    if (TypeHelpers::IsClassOrRecordType(Type1))
    {
        // Notes when Shadows is encountered on the upward walk.
        bool KeepClimbing = true;

        Type *Cursor = Type1;
        while (Cursor && KeepClimbing)
        {
            if (SearchBothTypes && CommonAncestor == NULL && IsOrInheritsFrom(Type2, Cursor))
            {
                CommonAncestor = Cursor;
            }

            if (Cursor->PClass()->HasUserDefinedOperators())
            {
                ScanMembers(Operator, Cursor, KeepClimbing, OperatorSet, Scratch);
            }
            Cursor = GetBaseClass(Cursor);
        }
    }

    if (SearchBothTypes && TypeHelpers::IsClassOrRecordType(Type2))
    {
        // Notes when Shadows is encountered on the upward walk.
        bool KeepClimbing = true;

        Type *Cursor = Type2;
        while (Cursor &&
               KeepClimbing &&
               !(CommonAncestor && TypeHelpers::EquivalentTypes(Cursor, CommonAncestor)))
        {
            if (Cursor->PClass()->HasUserDefinedOperators())
            {
                ScanMembers(Operator, Cursor, KeepClimbing, OperatorSet, Scratch);
            }
            Cursor = GetBaseClass(Cursor);
        }
    }
}

/*=======================================================================================
IsConvertible

Returns true if the conversion from Source to Target is possible.
=======================================================================================*/
/* bool
Semantics::IsConvertible
(
    Type *TargetType,
    Type *SourceType
)
{
    Procedure *ConversionMethod = NULL;
    ConversionClass Classification = ClassifyConversion(TargetType, SourceType, ConversionMethod);

    return Classification != ConversionError;
}
*/
/*=======================================================================================
RejectInapplicableOperators

An operator is applicable to a set of operands if both operands can convert to the
parameters of the operator.  This function removes all operators from the set that are
not applicable to Left and Right.
=======================================================================================*/
/* void
Semantics::RejectInapplicableOperators
(
    UserDefinedOperators Operator,
    Type *LeftType,
    Type *RightType,
    CDoubleList<OperatorInfo> &OperatorSet
)
{
    CDoubleListForwardIter<OperatorInfo> Iter(&OperatorSet);
    while (OperatorInfo *Current = Iter.Next())
    {
        VSASSERT(Current->Symbol->GetType() &&
                 Current->Symbol->GetFirstParam() &&
                 (IsUnaryOperator(Operator) || Current->Symbol->GetFirstParam()->GetNext()),
                 "expected correct operator shape");

        Type *LeftParamType = Current->Symbol->GetFirstParam()->GetType();
        Type *RightParamType =
            IsBinaryOperator(Operator) ?
                Current->Symbol->GetFirstParam()->GetNext()->GetType() :
                NULL;

        if (!IsConvertible(LeftParamType, LeftType) ||
            (IsBinaryOperator(Operator) && !IsConvertible(RightParamType, RightType)))
        {
            // 





*/
/****************************************************************************************
BEGIN intrinsic operator tables
****************************************************************************************/

enum OperatorTable
{
    Addition,
    SubtractionMultiplicationModulo,
    Division,
    Power,
    IntegerDivision,
    Shift,
    Logical,
    Bitwise,
    Relational,
    ConcatenationLike
};

#define t_r4    t_single
#define t_r8    t_double
#define t_dec   t_decimal
#define t_str   t_string

typedef Vtypes BinaryOperatorTable[t_ref - t_bad + 1][t_ref - t_bad + 1];

const BinaryOperatorTable BinaryOperatorTables[] = {

// Addition
{
// RHS->    bad     bool    i1      ui1     i2      ui2     i4      ui4     i8      ui8     dec     r4      r8      date    char    str     ref
/* bad  */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad},
/* bool */ {t_bad,  t_i2,   t_i1,   t_i2,   t_i2,   t_i4,   t_i4,   t_i8,   t_i8,   t_dec,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* i1   */ {t_bad,  t_i1,   t_i1,   t_i2,   t_i2,   t_i4,   t_i4,   t_i8,   t_i8,   t_dec,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* ui1  */ {t_bad,  t_i2,   t_i2,   t_ui1,  t_i2,   t_ui2,  t_i4,   t_ui4,  t_i8,   t_ui8,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* i2   */ {t_bad,  t_i2,   t_i2,   t_i2,   t_i2,   t_i4,   t_i4,   t_i8,   t_i8,   t_dec,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* ui2  */ {t_bad,  t_i4,   t_i4,   t_ui2,  t_i4,   t_ui2,  t_i4,   t_ui4,  t_i8,   t_ui8,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* i4   */ {t_bad,  t_i4,   t_i4,   t_i4,   t_i4,   t_i4,   t_i4,   t_i8,   t_i8,   t_dec,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* ui4  */ {t_bad,  t_i8,   t_i8,   t_ui4,  t_i8,   t_ui4,  t_i8,   t_ui4,  t_i8,   t_ui8,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* i8   */ {t_bad,  t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_dec,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* ui8  */ {t_bad,  t_dec,  t_dec,  t_ui8,  t_dec,  t_ui8,  t_dec,  t_ui8,  t_dec,  t_ui8,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* dec  */ {t_bad,  t_dec,  t_dec,  t_dec,  t_dec,  t_dec,  t_dec,  t_dec,  t_dec,  t_dec,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* r4   */ {t_bad,  t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* r8   */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* date */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_str,  t_bad,  t_str,  t_ref},
/* char */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_str,  t_str,  t_ref},
/* str  */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_str,  t_str,  t_str,  t_ref},
/* ref  */ {t_bad,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref},
},

// Subtraction, Multiplication, and Modulo
{
// RHS->    bad     bool    i1      ui1     i2      ui2     i4      ui4     i8      ui8     dec     r4      r8      date    char    str     ref
/* bad  */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad},
/* bool */ {t_bad,  t_i2,   t_i1,   t_i2,   t_i2,   t_i4,   t_i4,   t_i8,   t_i8,   t_dec,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* i1   */ {t_bad,  t_i1,   t_i1,   t_i2,   t_i2,   t_i4,   t_i4,   t_i8,   t_i8,   t_dec,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* ui1  */ {t_bad,  t_i2,   t_i2,   t_ui1,  t_i2,   t_ui2,  t_i4,   t_ui4,  t_i8,   t_ui8,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* i2   */ {t_bad,  t_i2,   t_i2,   t_i2,   t_i2,   t_i4,   t_i4,   t_i8,   t_i8,   t_dec,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* ui2  */ {t_bad,  t_i4,   t_i4,   t_ui2,  t_i4,   t_ui2,  t_i4,   t_ui4,  t_i8,   t_ui8,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* i4   */ {t_bad,  t_i4,   t_i4,   t_i4,   t_i4,   t_i4,   t_i4,   t_i8,   t_i8,   t_dec,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* ui4  */ {t_bad,  t_i8,   t_i8,   t_ui4,  t_i8,   t_ui4,  t_i8,   t_ui4,  t_i8,   t_ui8,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* i8   */ {t_bad,  t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_dec,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* ui8  */ {t_bad,  t_dec,  t_dec,  t_ui8,  t_dec,  t_ui8,  t_dec,  t_ui8,  t_dec,  t_ui8,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* dec  */ {t_bad,  t_dec,  t_dec,  t_dec,  t_dec,  t_dec,  t_dec,  t_dec,  t_dec,  t_dec,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* r4   */ {t_bad,  t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* r8   */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* date */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad},
//                     Special Note:  Date - Date is actually TimeSpan, but that cannot be encoded in this table.   ^^^^^
/* char */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad},
/* str  */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* ref  */ {t_bad,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_bad,  t_bad,  t_ref,  t_ref},
},

// Division
{
// RHS->    bad     bool    i1      ui1     i2      ui2     i4      ui4     i8      ui8     dec     r4      r8      date    char    str     ref
/* bad  */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad},
/* bool */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* i1   */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* ui1  */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* i2   */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* ui2  */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* i4   */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* ui4  */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* i8   */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* ui8  */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* dec  */ {t_bad,  t_dec,  t_dec,  t_dec,  t_dec,  t_dec,  t_dec,  t_dec,  t_dec,  t_dec,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* r4   */ {t_bad,  t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* r8   */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* date */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad},
/* char */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad},
/* str  */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* ref  */ {t_bad,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_bad,  t_bad,  t_ref,  t_ref},
},

// Power
{
// RHS->    bad     bool    i1      ui1     i2      ui2     i4      ui4     i8      ui8     dec     r4      r8      date    char    str     ref
/* bad  */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad},
/* bool */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* i1   */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* ui1  */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* i2   */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* ui2  */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* i4   */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* ui4  */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* i8   */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* ui8  */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* dec  */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* r4   */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* r8   */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* date */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad},
/* char */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad},
/* str  */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* ref  */ {t_bad,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_bad,  t_bad,  t_ref,  t_ref},
},

// Integer Division
{
// RHS->    bad     bool    i1      ui1     i2      ui2     i4      ui4     i8      ui8     dec     r4      r8      date    char    str     ref
/* bad  */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad},
/* bool */ {t_bad,  t_i2,   t_i1,   t_i2,   t_i2,   t_i4,   t_i4,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_bad,  t_bad,  t_i8,   t_ref},
/* i1   */ {t_bad,  t_i1,   t_i1,   t_i2,   t_i2,   t_i4,   t_i4,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_bad,  t_bad,  t_i8,   t_ref},
/* ui1  */ {t_bad,  t_i2,   t_i2,   t_ui1,  t_i2,   t_ui2,  t_i4,   t_ui4,  t_i8,   t_ui8,  t_i8,   t_i8,   t_i8,   t_bad,  t_bad,  t_i8,   t_ref},
/* i2   */ {t_bad,  t_i2,   t_i2,   t_i2,   t_i2,   t_i4,   t_i4,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_bad,  t_bad,  t_i8,   t_ref},
/* ui2  */ {t_bad,  t_i4,   t_i4,   t_ui2,  t_i4,   t_ui2,  t_i4,   t_ui4,  t_i8,   t_ui8,  t_i8,   t_i8,   t_i8,   t_bad,  t_bad,  t_i8,   t_ref},
/* i4   */ {t_bad,  t_i4,   t_i4,   t_i4,   t_i4,   t_i4,   t_i4,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_bad,  t_bad,  t_i8,   t_ref},
/* ui4  */ {t_bad,  t_i8,   t_i8,   t_ui4,  t_i8,   t_ui4,  t_i8,   t_ui4,  t_i8,   t_ui8,  t_i8,   t_i8,   t_i8,   t_bad,  t_bad,  t_i8,   t_ref},
/* i8   */ {t_bad,  t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_bad,  t_bad,  t_i8,   t_ref},
/* ui8  */ {t_bad,  t_i8,   t_i8,   t_ui8,  t_i8,   t_ui8,  t_i8,   t_ui8,  t_i8,   t_ui8,  t_i8,   t_i8,   t_i8,   t_bad,  t_bad,  t_i8,   t_ref},
/* dec  */ {t_bad,  t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_bad,  t_bad,  t_i8,   t_ref},
/* r4   */ {t_bad,  t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_bad,  t_bad,  t_i8,   t_ref},
/* r8   */ {t_bad,  t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_bad,  t_bad,  t_i8,   t_ref},
/* date */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad},
/* char */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad},
/* str  */ {t_bad,  t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_bad,  t_bad,  t_i8,   t_ref},
/* ref  */ {t_bad,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_bad,  t_bad,  t_ref,  t_ref},
},

// Shift
// Note: The right operand serves little purpose in this table, however a table is utilized nonetheless
//       to make the most use of already existing code which analyzes binary operators.
{
// RHS->    bad     bool    i1      ui1     i2      ui2     i4      ui4     i8      ui8     dec     r4      r8      date    char    str     ref
/* bad  */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad},
/* bool */ {t_bad,  t_i2,   t_i2,   t_i2,   t_i2,   t_i2,   t_i2,   t_i2,   t_i2,   t_i2,   t_i2,   t_i2,   t_i2,   t_i2,   t_i2,   t_i2,   t_ref},
/* i1   */ {t_bad,  t_i1,   t_i1,   t_i1,   t_i1,   t_i1,   t_i1,   t_i1,   t_i1,   t_i1,   t_i1,   t_i1,   t_i1,   t_i1,   t_i1,   t_i1,   t_ref},
/* ui1  */ {t_bad,  t_ui1,  t_ui1,  t_ui1,  t_ui1,  t_ui1,  t_ui1,  t_ui1,  t_ui1,  t_ui1,  t_ui1,  t_ui1,  t_ui1,  t_ui1,  t_ui1,  t_ui1,  t_ref},
/* i2   */ {t_bad,  t_i2,   t_i2,   t_i2,   t_i2,   t_i2,   t_i2,   t_i2,   t_i2,   t_i2,   t_i2,   t_i2,   t_i2,   t_i2,   t_i2,   t_i2,   t_ref},
/* ui2  */ {t_bad,  t_ui2,  t_ui2,  t_ui2,  t_ui2,  t_ui2,  t_ui2,  t_ui2,  t_ui2,  t_ui2,  t_ui2,  t_ui2,  t_ui2,  t_ui2,  t_ui2,  t_ui2,  t_ref},
/* i4   */ {t_bad,  t_i4,   t_i4,   t_i4,   t_i4,   t_i4,   t_i4,   t_i4,   t_i4,   t_i4,   t_i4,   t_i4,   t_i4,   t_i4,   t_i4,   t_i4,   t_ref},
/* ui4  */ {t_bad,  t_ui4,  t_ui4,  t_ui4,  t_ui4,  t_ui4,  t_ui4,  t_ui4,  t_ui4,  t_ui4,  t_ui4,  t_ui4,  t_ui4,  t_ui4,  t_ui4,  t_ui4,  t_ref},
/* i8   */ {t_bad,  t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_ref},
/* ui8  */ {t_bad,  t_ui8,  t_ui8,  t_ui8,  t_ui8,  t_ui8,  t_ui8,  t_ui8,  t_ui8,  t_ui8,  t_ui8,  t_ui8,  t_ui8,  t_ui8,  t_ui8,  t_ui8,  t_ref},
/* dec  */ {t_bad,  t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_ref},
/* r4   */ {t_bad,  t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_ref},
/* r8   */ {t_bad,  t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_ref},
/* date */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad},
/* char */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad},
/* str  */ {t_bad,  t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_ref},
/* ref  */ {t_bad,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref},
},

// Logical Operators
{
// RHS->    bad     bool    i1      ui1     i2      ui2     i4      ui4     i8      ui8     dec     r4      r8      date    char    str     ref
/* bad  */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad},
/* bool */ {t_bad,  t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bad,  t_bad,  t_bool, t_ref},
/* i1   */ {t_bad,  t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bad,  t_bad,  t_bool, t_ref},
/* ui1  */ {t_bad,  t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bad,  t_bad,  t_bool, t_ref},
/* i2   */ {t_bad,  t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bad,  t_bad,  t_bool, t_ref},
/* ui2  */ {t_bad,  t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bad,  t_bad,  t_bool, t_ref},
/* i4   */ {t_bad,  t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bad,  t_bad,  t_bool, t_ref},
/* ui4  */ {t_bad,  t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bad,  t_bad,  t_bool, t_ref},
/* i8   */ {t_bad,  t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bad,  t_bad,  t_bool, t_ref},
/* ui8  */ {t_bad,  t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bad,  t_bad,  t_bool, t_ref},
/* dec  */ {t_bad,  t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bad,  t_bad,  t_bool, t_ref},
/* r4   */ {t_bad,  t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bad,  t_bad,  t_bool, t_ref},
/* r8   */ {t_bad,  t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bad,  t_bad,  t_bool, t_ref},
/* date */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad},
/* char */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad},
/* str  */ {t_bad,  t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bool, t_bad,  t_bad,  t_bool, t_ref},
/* ref  */ {t_bad,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_bad,  t_bad,  t_ref,  t_ref},
},

// Bitwise Operators
{
// RHS->    bad     bool    i1      ui1     i2      ui2     i4      ui4     i8      ui8     dec     r4      r8      date    char    str     ref
/* bad  */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad},
/* bool */ {t_bad,  t_bool, t_i1,   t_i2,   t_i2,   t_i4,   t_i4,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_bad,  t_bad,  t_bool, t_ref},
/* i1   */ {t_bad,  t_i1,   t_i1,   t_i2,   t_i2,   t_i4,   t_i4,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_bad,  t_bad,  t_i8,   t_ref},
/* ui1  */ {t_bad,  t_i2,   t_i2,   t_ui1,  t_i2,   t_ui2,  t_i4,   t_ui4,  t_i8,   t_ui8,  t_i8,   t_i8,   t_i8,   t_bad,  t_bad,  t_i8,   t_ref},
/* i2   */ {t_bad,  t_i2,   t_i2,   t_i2,   t_i2,   t_i4,   t_i4,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_bad,  t_bad,  t_i8,   t_ref},
/* ui2  */ {t_bad,  t_i4,   t_i4,   t_ui2,  t_i4,   t_ui2,  t_i4,   t_ui4,  t_i8,   t_ui8,  t_i8,   t_i8,   t_i8,   t_bad,  t_bad,  t_i8,   t_ref},
/* i4   */ {t_bad,  t_i4,   t_i4,   t_i4,   t_i4,   t_i4,   t_i4,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_bad,  t_bad,  t_i8,   t_ref},
/* ui4  */ {t_bad,  t_i8,   t_i8,   t_ui4,  t_i8,   t_ui4,  t_i8,   t_ui4,  t_i8,   t_ui8,  t_i8,   t_i8,   t_i8,   t_bad,  t_bad,  t_i8,   t_ref},
/* i8   */ {t_bad,  t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_bad,  t_bad,  t_i8,   t_ref},
/* ui8  */ {t_bad,  t_i8,   t_i8,   t_ui8,  t_i8,   t_ui8,  t_i8,   t_ui8,  t_i8,   t_ui8,  t_i8,   t_i8,   t_i8,   t_bad,  t_bad,  t_i8,   t_ref},
/* dec  */ {t_bad,  t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_bad,  t_bad,  t_i8,   t_ref},
/* r4   */ {t_bad,  t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_bad,  t_bad,  t_i8,   t_ref},
/* r8   */ {t_bad,  t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_bad,  t_bad,  t_i8,   t_ref},
/* date */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad},
/* char */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad},
/* str  */ {t_bad,  t_bool, t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_bad,  t_bad,  t_i8,   t_ref},
/* ref  */ {t_bad,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_bad,  t_bad,  t_ref,  t_ref},
},

// Relational Operators -- This one is a little funky because it lists the type of the relational operation,
//     even though the result type is always boolean
{
// RHS->    bad     bool    i1      ui1     i2      ui2     i4      ui4     i8      ui8     dec     r4      r8      date    char    str     ref
/* bad  */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad},
/* bool */ {t_bad,  t_bool, t_i1,   t_i2,   t_i2,   t_i4,   t_i4,   t_i8,   t_i8,   t_dec,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_bool, t_ref},
/* i1   */ {t_bad,  t_i1,   t_i1,   t_i2,   t_i2,   t_i4,   t_i4,   t_i8,   t_i8,   t_dec,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* ui1  */ {t_bad,  t_i2,   t_i2,   t_ui1,  t_i2,   t_ui2,  t_i4,   t_ui4,  t_i8,   t_ui8,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* i2   */ {t_bad,  t_i2,   t_i2,   t_i2,   t_i2,   t_i4,   t_i4,   t_i8,   t_i8,   t_dec,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* ui2  */ {t_bad,  t_i4,   t_i4,   t_ui2,  t_i4,   t_ui2,  t_i4,   t_ui4,  t_i8,   t_ui8,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* i4   */ {t_bad,  t_i4,   t_i4,   t_i4,   t_i4,   t_i4,   t_i4,   t_i8,   t_i8,   t_dec,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* ui4  */ {t_bad,  t_i8,   t_i8,   t_ui4,  t_i8,   t_ui4,  t_i8,   t_ui4,  t_i8,   t_ui8,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* i8   */ {t_bad,  t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_i8,   t_dec,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* ui8  */ {t_bad,  t_dec,  t_dec,  t_ui8,  t_dec,  t_ui8,  t_dec,  t_ui8,  t_dec,  t_ui8,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* dec  */ {t_bad,  t_dec,  t_dec,  t_dec,  t_dec,  t_dec,  t_dec,  t_dec,  t_dec,  t_dec,  t_dec,  t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* r4   */ {t_bad,  t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r4,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* r8   */ {t_bad,  t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_bad,  t_bad,  t_r8,   t_ref},
/* date */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_date, t_bad,  t_date, t_ref},
/* char */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_char, t_str,  t_ref},
/* str  */ {t_bad,  t_bool, t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_r8,   t_date, t_str,  t_str,  t_ref},
/* ref  */ {t_bad,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref},
},

// Concatenation and Like
{
// RHS->    bad     bool    i1      ui1     i2      ui2     i4      ui4     i8      ui8     dec     r4      r8      date    char    str     ref
/* bad  */ {t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad,  t_bad},
/* bool */ {t_bad,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_ref},
/* i1   */ {t_bad,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_ref},
/* ui1  */ {t_bad,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_ref},
/* i2   */ {t_bad,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_ref},
/* ui2  */ {t_bad,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_ref},
/* i4   */ {t_bad,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_ref},
/* ui4  */ {t_bad,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_ref},
/* i8   */ {t_bad,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_ref},
/* ui8  */ {t_bad,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_ref},
/* dec  */ {t_bad,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_ref},
/* r4   */ {t_bad,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_ref},
/* r8   */ {t_bad,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_ref},
/* date */ {t_bad,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_ref},
/* char */ {t_bad,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_ref},
/* str  */ {t_bad,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_str,  t_ref},
/* ref  */ {t_bad,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref,  t_ref},
},

};

#undef t_r4
#undef t_r8
#undef t_dec
#undef t_str

Vtypes
LookupInOperatorTables
(
    BILOP Opcode,
    Vtypes Left,
    Vtypes Right
)
{
    OperatorTable Table;

    switch (Opcode)
    {
        case SX_ADD:         Table = Addition; break;
        case SX_SUB:
        case SX_MUL:
        case SX_MOD:         Table = SubtractionMultiplicationModulo; break;
        case SX_DIV:         Table = Division; break;
        case SX_IDIV:        Table = IntegerDivision; break;
        case SX_POW:         Table = Power; break;
        case SX_SHIFT_LEFT:
        case SX_SHIFT_RIGHT: Table = Shift; break;
        case SX_ORELSE:
        case SX_ANDALSO:     Table = Logical; break;
        case SX_CONC:
        case SX_LIKE:        Table = ConcatenationLike; break;
        case SX_EQ:
        case SX_NE:
        case SX_LE:
        case SX_GE:
        case SX_LT:
        case SX_GT:          Table = Relational; break;
        case SX_OR:
        case SX_XOR:
        case SX_AND:         Table = Bitwise; break;

        default:
            VSFAIL("unexpected binary operator");
            return t_bad;
            break;
    }

    if (Left >= t_bad && Left <= t_ref &&
        Right >= t_bad && Right <= t_ref)
    {
        return
            BinaryOperatorTables
                [Table]
                [Left - t_bad]
                [Right - t_bad];
    }

    VSFAIL("Invalid operaotr table");

    return t_bad;
}

Vtypes
LookupInOperatorTables
(
    BILOP Opcode,
    Vtypes Operand
)
{
    switch (Opcode)
    {
        case SX_NOT:
        {
            switch (Operand)
            {
                case t_bool:    return t_bool;
                case t_i1:      return t_i1;
                case t_ui1:     return t_ui1;
                case t_i2:      return t_i2;
                case t_ui2:     return t_ui2;
                case t_i4:      return t_i4;
                case t_ui4:     return t_ui4;
                case t_i8:      return t_i8;
                case t_ui8:     return t_ui8;
                case t_decimal: return t_i8;
                case t_single:  return t_i8;
                case t_double:  return t_i8;
                case t_date:    return t_bad;
                case t_char:    return t_bad;
                case t_string:  return t_i8;
                case t_ref:     return t_ref;
                default:
                    return t_bad;
            }
        }

        case SX_PLUS:
        {
            switch (Operand)
            {
                case t_bool:    return t_i2;  // 
                case t_i1:      return t_i1;
                case t_ui1:     return t_ui1;
                case t_i2:      return t_i2;
                case t_ui2:     return t_ui2;
                case t_i4:      return t_i4;
                case t_ui4:     return t_ui4;
                case t_i8:      return t_i8;
                case t_ui8:     return t_ui8;
                case t_decimal: return t_decimal;
                case t_single:  return t_single;
                case t_double:  return t_double;
                case t_date:    return t_bad;
                case t_char:    return t_bad;
                case t_string:  return t_double;
                case t_ref:     return t_ref;
                default:
                    return t_bad;
            }
        }

        case SX_NEG:
        {
            switch (Operand)
            {
                case t_bool:    return t_i2;  // 
                case t_i1:      return t_i1;
                case t_ui1:     return t_i2;
                case t_i2:      return t_i2;
                case t_ui2:     return t_i4;
                case t_i4:      return t_i4;
                case t_ui4:     return t_i8;
                case t_i8:      return t_i8;
                case t_ui8:     return t_decimal;
                case t_decimal: return t_decimal;
                case t_single:  return t_single;
                case t_double:  return t_double;
                case t_date:    return t_bad;
                case t_char:    return t_bad;
                case t_string:  return t_double;
                case t_ref:     return t_ref;
                default:
                    return t_bad;
            }
        }

        default:
            VSFAIL("unexpected unary operator");
            break;
    }

    return t_bad;
}

/****************************************************************************************
END intrinsic operator tables
****************************************************************************************/

void
Semantics::ReportUndefinedOperatorError
(
    BILOP Opcode,
    const Location &ExpressionLocation,
    _In_ ILTree::Expression *Left,
    _In_ ILTree::Expression *Right,
    _Out_ bool &ResolutionFailed
)
{
    ResolutionFailed = true;

    // Error -- the operation can't be performed on operands of these types.

    unsigned ErrorId = ERRID_BinaryOperands3;

    if ((Opcode == SX_EQ || Opcode == SX_NE) &&
        TypeHelpers::IsReferenceType(Left->ResultType) &&
        TypeHelpers::IsReferenceType(Right->ResultType))
    {
        ErrorId = ERRID_ReferenceComparison3;
    }
    else if (GetFXSymbolProvider()->IsTypeAvailable(FX::GenericIEnumerableType) &&
             m_XmlSymbols.GetXElement() )
    {
        if( TypeHelpers::IsCompatibleWithGenericEnumerableType(
                Left->ResultType,
                m_XmlSymbols.GetXElement(),
                m_SymbolCreator,
                m_CompilerHost
                )
          )
        {
            // XML literal error.
            ReportSemanticError(
                ERRID_BinaryOperandsForXml4,
                ExpressionLocation,
                Opcode,
                Left->ResultType,
                Right->ResultType,
                Left->ResultType
                );

            return;

        }
        else if( TypeHelpers::IsCompatibleWithGenericEnumerableType(
                Right->ResultType,
                m_XmlSymbols.GetXElement(),
                m_SymbolCreator,
                m_CompilerHost
                )
        )
        {
            // XML literal error.
            ReportSemanticError(
                ERRID_BinaryOperandsForXml4,
                ExpressionLocation,
                Opcode,
                Left->ResultType,
                Right->ResultType,
                Right->ResultType
                );

            return;
        }
    }

    ReportSemanticError(
        ErrorId,
        ExpressionLocation,
        Opcode,
        Left->ResultType,
        Right->ResultType
        );
}

void
Semantics::ReportUndefinedOperatorError
(
    BILOP Opcode,
    const Location &ExpressionLocation,
    _In_ ILTree::Expression *Operand,
    _Out_ bool &ResolutionFailed
)
{
    ResolutionFailed = true;

    ReportSemanticError(
        ERRID_UnaryOperand2,
        ExpressionLocation,
        Opcode,
        Operand->ResultType);
}

void
Semantics::ReportUndefinedOperatorError
(
    UserDefinedOperators Operator,
    const Location &ExpressionLocation,
    _In_ ILTree::Expression *Left,
    _In_opt_ ILTree::Expression *Right,
    _Out_ bool &ResolutionFailed
)
{
    ResolutionFailed = true;

    if (Right == NULL)
    {
        ReportSemanticError(
            ERRID_UnaryOperand2,
            ExpressionLocation,
            m_Compiler->OperatorToString(Operator),
            Left->ResultType);
    }
    else
    {
        if( GetFXSymbolProvider()->IsTypeAvailable(FX::GenericIEnumerableType) &&
            m_XmlSymbols.GetXElement() )
        {
            if( TypeHelpers::IsCompatibleWithGenericEnumerableType(
                    Left->ResultType,
                    m_XmlSymbols.GetXElement(),
                    m_SymbolCreator,
                    m_CompilerHost
                    )
              )
            {
                // XML literal error.
                ReportSemanticError(
                    ERRID_BinaryOperandsForXml4,
                    ExpressionLocation,
                    m_Compiler->OperatorToString(Operator),
                    Left->ResultType,
                    Right->ResultType,
                    Left->ResultType
                    );

                return;
            }
            else if( TypeHelpers::IsCompatibleWithGenericEnumerableType(
                    Right->ResultType,
                    m_XmlSymbols.GetXElement(),
                    m_SymbolCreator,
                    m_CompilerHost
                    )
            )
            {
                // XML literal error.
                ReportSemanticError(
                    ERRID_BinaryOperandsForXml4,
                    ExpressionLocation,
                    m_Compiler->OperatorToString(Operator),
                    Left->ResultType,
                    Right->ResultType,
                    Right->ResultType
                    );

                return;
            }
        }

        ReportSemanticError(
            ERRID_BinaryOperands3,
            ExpressionLocation,
            m_Compiler->OperatorToString(Operator),
            Left->ResultType,
            Right->ResultType
            );
    }
}

Type *
Semantics::ResolveBinaryOperatorResultType
(
    BILOP Opcode,
    const Location &ExpressionLocation,
    _In_ ILTree::Expression *Left,
    _In_ ILTree::Expression *Right,
    _Out_ bool &ResolutionFailed,
    _Out_ Procedure *&OperatorMethod,
    _Out_ GenericBinding *&OperatorMethodGenericContext,
    _Out_ bool &LiftedNullable
)
{
    ResolutionFailed = false;

    Type *LeftType = Left->ResultType;
    Type *RightType = Right->ResultType;

    bool HasNullableOperands = false;
    if (TypeHelpers::IsNullableType(LeftType, m_CompilerHost))
    {
        LeftType = TypeHelpers::GetElementTypeOfNullable(LeftType, m_CompilerHost);
        HasNullableOperands = true;
    }
    if (TypeHelpers::IsNullableType(RightType, m_CompilerHost))
    {
        RightType = TypeHelpers::GetElementTypeOfNullable(RightType, m_CompilerHost);
        HasNullableOperands = true;
    }

    // Try to lift the operator if any of the operands are nullable and operator supports lifting.
    LiftedNullable = HasNullableOperands && CanLiftIntrinsicOperator(Opcode);

    // If the operand type is an Enum,
    // a bitwise operation has the enum type as its result type.

    if (TypeHelpers::IsEnumType(LeftType) &&
        TypeHelpers::EquivalentTypes(LeftType, RightType) &&
        (Opcode == SX_AND || Opcode == SX_OR ||Opcode == SX_XOR))
    {
        return LiftedNullable ? GetFXSymbolProvider()->GetNullableType(LeftType, &m_SymbolCreator) : Left->ResultType;
    }

    Vtypes Result = t_bad;
    Vtypes VtypeLeft = LeftType->GetVtype();
    Vtypes VtypeRight = RightType->GetVtype();

    // Operands of type 1-dimensional array of Char are treated as if they
    // were of type String.

    if (TypeHelpers::IsCharArrayRankOne(LeftType))
    {
        VtypeLeft = t_string;
    }

    if (TypeHelpers::IsCharArrayRankOne(RightType))
    {
        VtypeRight = t_string;
    }

    // The conversion tables assume that a vtype of t_ref represents the type Object,
    // so the rules for other reference types need to be applied before accessing a
    // table.

    if ((VtypeLeft == t_ref && !TypeHelpers::IsRootObjectType(LeftType)) ||
        (VtypeRight == t_ref && !TypeHelpers::IsRootObjectType(RightType)) ||
        (VtypeLeft == t_void || VtypeRight == t_void) ||
        (VtypeLeft > t_ref || VtypeRight > t_ref) ||
        (VtypeLeft == t_date && VtypeRight == t_date && Opcode == SX_SUB))  // Let Date - Date use operator overloading.
    {
        // From this point on, intrinsic operators are not considered, so reset LiftedNullable.
        LiftedNullable = false;

        if (CanTypeContainUserDefinedOperators(LeftType) || CanTypeContainUserDefinedOperators(RightType))
        {
            // An operator with an Object operand and a Type Parameter operand
            // is latebound if the Type Parameter has no class constraint.

            if ((TypeHelpers::IsRootObjectType(LeftType) &&
                    TypeHelpers::IsGenericParameter(RightType) &&
                    !HasClassConstraint(RightType->PGenericParam())) ||
                (TypeHelpers::IsRootObjectType(RightType) &&
                    TypeHelpers::IsGenericParameter(LeftType) &&
                    !HasClassConstraint(LeftType->PGenericParam())))
            {
                return GetFXSymbolProvider()->GetObjectType();
            }

            // In the context of short-circuting operators, we bind to the And and Or operators to determine
            // if the special form of overloaded short-circuiting applies.

            bool ResolutionIsLateBound = false;

            Type *ResultType =
                ResolveUserDefinedOperator(
                    Opcode == SX_ANDALSO ? SX_AND : Opcode == SX_ORELSE ? SX_OR : Opcode,
                    ExpressionLocation,
                    Left,
                    Right,
                    ResolutionFailed,
                    ResolutionIsLateBound,
                    OperatorMethod,
                    OperatorMethodGenericContext);

            return (ResolutionIsLateBound ? GetFXSymbolProvider()->GetObjectType() : ResultType);
        }

        ReportUndefinedOperatorError(
            Opcode,
            ExpressionLocation,
            Left,
            Right,
            ResolutionFailed);

        return NULL;
    }

    Result = LookupInOperatorTables(Opcode, VtypeLeft, VtypeRight);

    // Disallow nullable lifting if result is not a value type,
    LiftedNullable = LiftedNullable && (Result < t_string || Result == t_date);

    if (Result == t_bad)
    {
        ReportUndefinedOperatorError(
            Opcode,
            ExpressionLocation,
            Left,
            Right,
            ResolutionFailed);

        return NULL;
    }

    if (Result == t_ref)
    {
        return GetFXSymbolProvider()->GetObjectType();
    }

    if (LiftedNullable && !GetFXSymbolProvider()->IsTypeAvailable(FX::GenericNullableType))
    {
        ReportMissingType(FX::GenericNullableType, ExpressionLocation);
        return NULL;
    }

    return LiftedNullable ?
        GetFXSymbolProvider()->GetNullableIntrinsicSymbol(Result) :
        GetFXSymbolProvider()->GetType(Result);
}

Type *
Semantics::ResolveUnaryOperatorResultType
(
    BILOP Opcode,
    const Location &ExpressionLocation,
    _In_ ILTree::Expression *Operand,
    _Out_ bool &ResolutionFailed,
    _Out_ Procedure *&OperatorMethod,
    _Out_ GenericBinding *&OperatorMethodGenericContext,
    _Out_ bool &LiftedNullable
)
{
    ResolutionFailed = false;
    LiftedNullable = false;

    Type *OperandType = Operand->ResultType;

    // Ignore nullable during operator resolution to acheive operator lifting.
    if (TypeHelpers::IsNullableType(OperandType, m_CompilerHost))
    {
        OperandType = TypeHelpers::GetElementTypeOfNullable(OperandType, m_CompilerHost);
        LiftedNullable = true;
    }

    Vtypes VtypeOperand = OperandType->GetVtype();

    if ((VtypeOperand == t_ref && !TypeHelpers::IsRootObjectType(OperandType)) ||
        VtypeOperand > t_ref || VtypeOperand == t_void)
    {
        if (CanTypeContainUserDefinedOperators(OperandType))
        {
            bool ResolutionIsLateBound = false;

            Type *ResultType =
                ResolveUserDefinedOperator(
                    Opcode,
                    ExpressionLocation,
                    Operand,
                    ResolutionFailed,
                    ResolutionIsLateBound,
                    OperatorMethod,
                    OperatorMethodGenericContext);

            AssertIfTrue(ResolutionIsLateBound);

            if (!ResolutionIsLateBound)
            {
                return ResultType;
            }
        }

        ReportUndefinedOperatorError(
            Opcode,
            ExpressionLocation,
            Operand,
            ResolutionFailed);

        return NULL;
    }

    // If the operand type is an Enum,
    // a bitwise operation has the enum type as its result type.
    if (TypeHelpers::IsEnumType(OperandType) && Opcode == SX_NOT)
    {
        // NOTE: Reget OperandType here since we might have erased Nullable from it.
        // We should still test nullable element type in the if though.
        return Operand->ResultType;
    }

    Vtypes Result = t_bad;
    if (!LiftedNullable || CanLiftIntrinsicOperator(Opcode))
    {
        Result = LookupInOperatorTables(Opcode, VtypeOperand);
    }

    if (Result == t_bad ||
        (Result == t_ref && LiftedNullable)) // Dev10 #844149 don't let invalid nullable type Nullable(Of Object) to slip through.
    {
        ReportUndefinedOperatorError(
            Opcode,
            ExpressionLocation,
            Operand,
            ResolutionFailed);

        return NULL;
    }

    if (Result == t_ref)
    {
        return GetFXSymbolProvider()->GetObjectType();
    }

    if (LiftedNullable && !GetFXSymbolProvider()->IsTypeAvailable(FX::GenericNullableType))
    {
        ReportMissingType(FX::GenericNullableType, ExpressionLocation);
        return NULL;
    }

    return LiftedNullable ?
        GetFXSymbolProvider()->GetNullableIntrinsicSymbol(Result) :
        GetFXSymbolProvider()->GetType(Result);
}

UserDefinedOperators
Semantics::MapToUserDefinedOperator
(
    BILOP Opcode
)
{
    switch (Opcode)
    {
        case SX_NOT:         return OperatorNot;
        case SX_NEG:         return OperatorNegate;
        case SX_PLUS:        return OperatorUnaryPlus;
        case SX_ANDALSO:     return OperatorIsFalse;
        case SX_ORELSE:      return OperatorIsTrue;
        case SX_ADD:         return OperatorPlus;
        case SX_SUB:         return OperatorMinus;
        case SX_MUL:         return OperatorMultiply;
        case SX_DIV:         return OperatorDivide;
        case SX_POW:         return OperatorPower;
        case SX_IDIV:        return OperatorIntegralDivide;
        case SX_CONC:        return OperatorConcatenate;
        case SX_SHIFT_LEFT:  return OperatorShiftLeft;
        case SX_SHIFT_RIGHT: return OperatorShiftRight;
        case SX_MOD:         return OperatorModulus;
        case SX_OR:          return OperatorOr;
        case SX_XOR:         return OperatorXor;
        case SX_AND:         return OperatorAnd;
        case SX_LIKE:        return OperatorLike;
        case SX_EQ:          return OperatorEqual;
        case SX_NE:          return OperatorNotEqual;
        case SX_LT:          return OperatorLess;
        case SX_LE:          return OperatorLessEqual;
        case SX_GE:          return OperatorGreaterEqual;
        case SX_GT:          return OperatorGreater;

        default:
            VSFAIL("unexpected operator opcode");
    }

    return OperatorUNDEF;
}

bool Semantics::IsValidInLiftedSignature
(
    Type * pType, 
    GenericTypeBinding * pGenericBindingContext
)
{
    pType = pType ? pType->DigThroughAlias() : NULL;

    if (pGenericBindingContext != NULL && pType && pType->IsGenericParam())
    {
        pType = this->TypeInGenericContext(pType, pGenericBindingContext);
    }

    return
        !IsRestrictedType(pType, m_CompilerHost) &&
        (TypeHelpers::IsValueType(pType) || TypeHelpers::IsNullableType(pType));
}

bool
Semantics::ShouldLiftType
(
    Type * pType,
    GenericTypeBinding * pGenericBindingContext,    
    CompilerHost * pCompilerHost
)
{
    if (pType == NULL)
    {
        return false;
    }

    if (pGenericBindingContext != NULL && pType->IsGenericParam())
    {
        pType = this->TypeInGenericContext(pType,pGenericBindingContext);
    }

    return
        ! TypeHelpers::IsNullableType(pType, pCompilerHost) &&
        TypeHelpers::IsValueType(pType) &&
        ! IsRestrictedType(pType, pCompilerHost);       

}


bool
Semantics::ShouldLiftParameter
(
    Parameter * pParam,
    GenericTypeBinding * pGenericBindingContext,    
    CompilerHost * pCompilerHost
)
{
    if (pParam == NULL)
    {
        return false;
    }

    return ShouldLiftType(pParam->GetType(), pGenericBindingContext, pCompilerHost);
}


bool
Semantics::ShouldLiftOperator
(
    Operator * pOperator,
    GenericTypeBinding * pGenericBindingContext
)
{
    return
    (
        ShouldLiftParameter(pOperator->GetFirstParam(), pGenericBindingContext, m_CompilerHost)
        ||
        (
            pOperator->GetFirstParam() &&
            ShouldLiftParameter(pOperator->GetFirstParam()->GetNext(), pGenericBindingContext, m_CompilerHost)
        )
    ) &&
    !
    (
        TypeHelpers::IsNullableType(pOperator->GetFirstParam()->GetType(), m_CompilerHost) ||
        (
            pOperator->GetFirstParam() &&
            pOperator->GetFirstParam()->GetNext() &&
            TypeHelpers::IsNullableType(pOperator->GetFirstParam()->GetNext()->GetType(), m_CompilerHost)
        )
    );

}


bool
Semantics::ShouldInsertLiftedOperator
(
    Operator * pLiftedOperator,
    Operator * pSourceOperator,
    GenericTypeBinding * pGenericBindingContext,
    _In_ CDoubleList<OperatorInfo> & OperatorSet
)
{
    ThrowIfNull(pLiftedOperator);

    OperatorInfo * pCurrent = OperatorSet.GetFirst();

    //I spoke with Microsoft and he mentioned that someone in the DDE
    //perf team is working on a global compiler wide replacement for
    //the NorlsAllocator that will be coming on line sometime in Orcas.
    //As a result I figured I was not adding too much baddness to our performance
    //by adding a new scratch allocator that held little data, as this would be optimized away
    //soon. The alternative I had was to refactor the symbols class
    //to take in an allocator interface rather than an allocator and then
    //supply it with an allocator that used a heap and then collected
    //together a set of all the pointers that were allocated so that they could be deleted later.
    //This, however, was almost identical to the NorlsAllocator replacement the DDE perf
    //team was working on, so I figured it would make more sense to not duplicate this effort.

    NorlsAllocator scratch(NORLSLOC);

    while (pCurrent)
    {
        unsigned int result =
            BCSYM::CompareProcs
            (
                pLiftedOperator,
                pGenericBindingContext,
                pCurrent->Symbol,
                pCurrent->GenericBindingContext,
                GetSymbols()
            );

        if ((result & (EQ_Match | EQ_Flags | EQ_Return) ) == result)
        {
            return false;
        }

        pCurrent = pCurrent->Next();
    }

    return true;
}

void
Semantics::LiftOperator
(
    _Inout_ CDoubleList<OperatorInfo> & OperatorSet,
    GenericTypeBinding * pGenericBindingContext,
    _In_ Operator * pSourceOperator,
    _Inout_ NorlsAllocator * pNorls
)
{
    Operator * pLifted = NULL;
    bool hasGenericParam = false;
    

    // Bug 841109, shiqic. 
    // When lifting a operator with generic parameter, LiftUserDefinedOperator does not always return the same lifted operator, so we can't cache them.
    // We only cache non-generic operator without generic parameter.
    ThrowIfNull(pSourceOperator->GetType());
    ThrowIfNull(pSourceOperator->GetFirstParam());
    ThrowIfNull(pSourceOperator->GetFirstParam()->GetType());    

    if (pSourceOperator->GetType()->IsGenericParam())
    {
        hasGenericParam = true;
    }
    else if (pSourceOperator->GetFirstParam()->GetType()->IsGenericParam())
    {
        hasGenericParam = true;
    }
    else if 
    (
        pSourceOperator->GetFirstParam()->GetNext() != NULL && 
        pSourceOperator->GetFirstParam()->GetNext()->GetType()->IsGenericParam()
    )
    {
        hasGenericParam = true;
    }

    if
    (
        !(  
            ! hasGenericParam &&
            m_PermitDeclarationCaching  &&
            m_LiftedOperatorCache &&
            m_LiftedOperatorCache->LookupInCache(pSourceOperator,  &pLifted)
        )
    )
    {
        BackupAllocator backup_symbol_allocator(&m_SymbolCreator);

        if (m_PermitDeclarationCaching && m_LiftedOperatorCache)
        {
            m_SymbolCreator.SetNorlsAllocator(m_LiftedOperatorCache->GetNorlsAllocator());
        }

        pLifted =
            m_SymbolCreator.LiftUserDefinedOperator
            (
                pSourceOperator,
                pGenericBindingContext,
                this,
                m_CompilerHost
            );

        //The purpose of this cache is limit memory usage rather than to improve speed.
        //In particular it is designed to enable lifted operator symbols to be reused
        //across operator usage sites. As a result, if there is no lifted operator
        //then we don't want to insert a new cache entry as this would waste space rather than
        //save it.
        if (!hasGenericParam && pLifted && m_PermitDeclarationCaching && m_LiftedOperatorCache)
        {
            m_LiftedOperatorCache->AddEntry(pSourceOperator, pLifted);
        }
    }

    if (ShouldInsertLiftedOperator(pLifted, pSourceOperator, pGenericBindingContext, OperatorSet))
    {
        OperatorInfo * pInfo = new (*pNorls) OperatorInfo();
        pInfo->Symbol = pLifted;
        pInfo->GenericBindingContext = pGenericBindingContext;
        OperatorSet.InsertFirst(pInfo);
    }
}

void
Semantics::LiftUserDefinedOperatorsForNullable
(
    _Inout_ CDoubleList<OperatorInfo>  & OperatorSet,
    Type *  pLeftType,
    Type * pRightType,
    _Inout_ NorlsAllocator * pNorls
)
{
    //pLeftType can't be NUL. pRightType is OK
    ThrowIfNull(pLeftType);


    OperatorInfo * pCurrent = OperatorSet.GetFirst();

    while (pCurrent)
    {
        if (ShouldLiftOperator(pCurrent->Symbol, pCurrent->GenericBindingContext))
        {
            LiftOperator
            (
                OperatorSet,
                pCurrent->GenericBindingContext,
                pCurrent->Symbol,
                pNorls
            );
        }
        pCurrent = pCurrent->Next();
    }
}

bool
Semantics::CanLiftIntrinsicOperator
(
    BILOP Opcode
)
{
    switch (Opcode)
    {
        // Liftable unary operators.
        case SX_NOT:
        case SX_PLUS:
        case SX_NEG:

        // Liftable binary operators.
        case SX_ADD:
        case SX_SUB:
        case SX_MUL:
        case SX_MOD:
        case SX_DIV:
        case SX_IDIV:
        case SX_POW:
        case SX_SHIFT_LEFT:
        case SX_SHIFT_RIGHT:
        case SX_ORELSE:
        case SX_ANDALSO:
        case SX_EQ:
        case SX_NE:
        case SX_LE:
        case SX_GE:
        case SX_LT:
        case SX_GT:
        case SX_OR:
        case SX_XOR:
        case SX_AND:
            return true;

        // Non-liftable binary operators.
        case SX_CONC:
        case SX_LIKE:
            return false;

        default:
            VSFAIL("Unexpected operator kind.");
            return false;
    }
}

/*=======================================================================================
ResolveUserDefinedOperator

Given an operation to perform with Left and Right operands, select the appropriate
user-defined operator.  If one exists, it will be supplied as an out parameter.  This
function will generate compile errors if the resolution is ambiguous.

Unary operators will pass NULL for the Right operand.

To select the appropriate operator, first collect all applicable operators.  If only one
exists, resolution is complete. If more than one exists, perform standard method overload
resolution to select the correct operator.  If none exist, report an error.

See the language specification for an in-depth discussion of the algorithm.
=======================================================================================*/
// It would be nice if this function had two modes - resolve and then give error messages.  Maybe a wrapper which turns off error reporting?
Type *
Semantics::ResolveUserDefinedOperator
(
    BILOP Opcode,
    const Location &ExpressionLocation,
    _In_ ILTree::Expression *Left,
    _In_ ILTree::Expression *Right,
    _Out_ bool &ResolutionFailed,
    _Out_ bool &ResolutionIsLateBound,
    _Out_ Procedure *&OperatorMethod,
    _Out_ GenericBinding *&OperatorMethodGenericContext
)
{
    UserDefinedOperators Operator = MapToUserDefinedOperator(Opcode);
    ResolutionFailed = false;
    ResolutionIsLateBound = false;
    bool ResolutionIsAmbiguous = false;

    VSASSERT(IsBinaryOperator(Operator) || (IsUnaryOperator(Operator) && Right == NULL),
             "Right operand supplied for a unary operator?");

    Type *LeftType = Left->ResultType;
    Type *RightType = IsBinaryOperator(Operator) ? Right->ResultType : NULL;
    OperatorMethod = NULL;
    OperatorMethodGenericContext = NULL;

    NorlsAllocator Scratch(NORLSLOC);

    CDoubleList<OperatorInfo> OperatorSet;

    // First construct the list of operators we will consider.
    CollectOperators(
        Operator,
        LeftType,
        RightType,
        OperatorSet,
        Scratch);

    LiftUserDefinedOperatorsForNullable(OperatorSet,LeftType, RightType, &Scratch);

    /*
    











*/

    if (OperatorSet.NumberOfEntries() > 0)
    {
        // There are operators available, so use standard method overload resolution
        // to choose the correct one.

        unsigned CandidateCount = 0;
        OverloadList *Candidates = NULL;

        // 


        // Build up the list of candidates for method overload resolution to use.

       OperatorInfo * Current = OperatorSet.GetFirst();

        while (Current)
        {

            Candidates =
                new(Scratch)
                OverloadList(
                    Current->Symbol,
                    Current->GenericBindingContext,
                    Candidates,
                    false);          // Operators cannot have paramarrays.
            CandidateCount++;

            Current = Current->Next();
        }

        // Build up the bound argument list for method overload resolution to use.
        ExpressionList *RightArgument =
            IsBinaryOperator(Operator) ?
                AllocateExpression(
                    SX_LIST,
                    TypeHelpers::GetVoidType(),
                    AllocateExpression(
                        SX_ARG,
                        TypeHelpers::GetVoidType(),
                        Right,
                        ExpressionLocation),
                    ExpressionLocation) :
                NULL;

        ExpressionList *Arguments =
            AllocateExpression(
                SX_LIST,
                TypeHelpers::GetVoidType(),
                AllocateExpression(
                    SX_ARG,
                    TypeHelpers::GetVoidType(),
                    Left,
                    ExpressionLocation),
                RightArgument,
                ExpressionLocation);

        GenericBinding *ResultGenericContext = NULL;


        Declaration *Result =
            ResolveOverloading
            (
                ExpressionLocation,
                Candidates,
                CandidateCount,
                Arguments,
                IsBinaryOperator(Operator) ? 2 : 1,
                NULL,
                NULL,
                ResolutionFailed,
                OvrldNoFlags, // AllowLateBound by not specifying OvrldIgnoreLateBound
                ResolutionIsLateBound,
                ResolutionIsAmbiguous,
                ResultGenericContext,
                OutputParameter<OverloadList *>(NULL),
                true
            );

        if (ResolutionFailed ||
            ResolutionIsLateBound)  // Bug VSWhidbey 472275. Latebound resolution will be required when all narrowing is from Object.
        {
            // Method overload resolution failed--errors have already been reported.
            return NULL;
        }
        else
        {
            OperatorMethod = Result->PUserDefinedOperator()->GetOperatorMethod();
            OperatorMethodGenericContext = ResultGenericContext;

            return TypeInGenericContext(OperatorMethod->GetType(), OperatorMethodGenericContext);
        }
    }
    // MQ Bug 842804
    // If OperatorSet.NumberOfEntries() == 0 and either of the oparand is object, it should be late bind.
    else if ((LeftType != NULL && LeftType->IsObject()) || (RightType != NULL && RightType->IsObject()))
    {
        ResolutionIsLateBound = true;
        return NULL;
    }   

    /*
    // No applicable operator found, so attempt to convert to an intrinsic and use an intrinsic operator.

    



















































































































































































*/

    // There are no results, so the operation is not defined for Left and Right.
    ReportUndefinedOperatorError(
        Operator,
        ExpressionLocation,
        Left,
        Right,
        ResolutionFailed);

    return NULL;
}

Type *
Semantics::ResolveUserDefinedOperator
(
    BILOP Opcode,
    const Location &ExpressionLocation,
    _In_ ILTree::Expression *Operand,
    _Out_ bool &ResolutionFailed,
    _Out_ bool &ResolutionIsLateBound,
    _Out_ Procedure *&OperatorMethod,
    _Out_ GenericBinding *&OperatorMethodGenericContext
)
{
    return
        ResolveUserDefinedOperator(
            Opcode,
            ExpressionLocation,
            Operand,
            NULL,
            ResolutionFailed,
            ResolutionIsLateBound,
            OperatorMethod,
            OperatorMethodGenericContext);
}
