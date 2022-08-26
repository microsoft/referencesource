//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Helper methods for working with Types
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

const bool TypeIsRepresentedAsObjectReference[] =
{
#define DEF_TYPE(type, clrscope, clrname, vbname, size, token, isnumeric, isintegral, isunsigned, isreference, allowoperation, allowconversion)  isreference,
#include "TypeTables.h"
#undef DEF_TYPE
};

//
//
// non-inlined methods
//
//

bool TypeHelpers::IsConstrainedToNullable
(
    Type * pType, 
    CompilerHost * pHost
)
{
    if (pType && pHost && pType->IsGenericParam())
    {
        BCSYM_GenericParam * pParam = pType->PGenericParam();

        BCSYM_GenericTypeConstraint * pConstraint = pParam->GetTypeConstraints();

        while (pConstraint)
        {
            BCSYM * pConstraintType = pConstraint->GetType();

            if (IsNullableType(pConstraintType, pHost) || IsConstrainedToNullable(pConstraintType, pHost))
            {
                return true;
            }

            pConstraint = pConstraint->Next();
        }
    }

    return NULL;
}



bool TypeHelpers::IsBadType(Type * T)
{
    if (IsPointerType(T))
    {
        T = GetReferencedType(T->PPointerType());
    }

    while (IsArrayType(T))
    {
        T = GetElementType(T->PArrayType());
    }

    // ISSUE: It is actually possible for the second clause of this
    // expression to be true when the first is not. This seems unsound.
    return T->IsBad() || T->GetVtype() == t_bad;
}



// IsKnownType
// At the top of TypeTables.h there's a list of all known types.
// Various functions in the code need to know every possible type that can be thrown at them.
// So if you add or change some type, you'll have to review those functions as well.
// Functions to review:
//  ClassifyPredefinedCLRConversion
//  IsReferenceType/IsValueType
#ifdef DEBUG
bool TypeHelpers::IsKnownType(Type * T)
{
    VSASSERT(T!=NULL, "please don't pass NULL to IsKnowntype");
    VSASSERT(T->IsType(), "T isn't even a type; see BCSYM::s_rgBilkindInfo in SymbolTable.cpp");
    VSASSERT(T->GetVtype()!=t_bad && T->GetVtype()!=t_UNDEF, "I thought that t_bad and t_UNDEF never appeared at runtime.");
    int known = 0; // a count of how many matches. Should be exactly 1. If >1, we didn't discriminate enough. If =0, then unknown type.
    if (T->IsBad())
    {
        return true; // we won't even investigate the bad case
    }
    VSASSERT(!T->IsBad() && !TypeHelpers::IsBadType(T) && !T->IsGenericBadNamedRoot(), "assumed that T wasn't bad");
    

    // ==============================================================================================================================================
    // VOID TYPE
    // ==============================================================================================================================================
    if (TypeHelpers::IsVoidType(T))
    {
        VSASSERT(!TypeHelpers::IsByteType(T) && !TypeHelpers::IsSignedByteType(T) && !TypeHelpers::IsShortType(T) && !TypeHelpers::IsUnsignedShortType(T) &&
                 !TypeHelpers::IsIntegerType(T) && !TypeHelpers::IsUnsignedIntegerType(T) && !TypeHelpers::IsLongType(T) && !TypeHelpers::IsUnsignedLongType(T) &&
                 !TypeHelpers::IsIntegralType(T) && !TypeHelpers::IsUnsignedType(T) && !TypeHelpers::IsDecimalType(T) && !TypeHelpers::IsSingleType(T) &&
                 !TypeHelpers::IsFloatingType(T) && !TypeHelpers::IsNumericType(T) && !TypeHelpers::IsDateType(T) && !TypeHelpers::IsBooleanType(T) &&
                 !TypeHelpers::IsCharType(T) && !TypeHelpers::IsStringType(T) && !TypeHelpers::IsArrayType(T) && !TypeHelpers::IsCharArrayRankOne(T) &&
                 !TypeHelpers::IsUndefinedType(T) && !TypeHelpers::IsPointerType(T) && TypeHelpers::IsVoidType(T) &&
                 !TypeHelpers::IsRootObjectType(T) && !TypeHelpers::IsIntrinsicType(T) &&
                 !TypeHelpers::IsInterfaceType(T) && !TypeHelpers::IsClassType(T) && !TypeHelpers::IsRecordType(T) &&
                 !TypeHelpers::IsClassOrRecordType(T) && !TypeHelpers::IsClassOrInterfaceType(T) && !TypeHelpers::IsClassInterfaceOrRecordType(T) &&
                 !TypeHelpers::IsClassInterfaceRecordOrGenericParamType(T) && !TypeHelpers::IsEnumType(T) && !TypeHelpers::IsDelegateType(T) &&
                 !TypeHelpers::IsGenericParameter(T) && !TypeHelpers::IsGenericTypeBinding(T) && !TypeHelpers::IsValueTypeOrGenericParameter(T) &&
                 !TypeHelpers::HasVariance(T) && !TypeHelpers::IsReferenceType(T) && !TypeHelpers::IsValueType(T),
                 "void inconsistent according to TypeHelpers");
        VSASSERT(T==GetVoidType(),
                 "void should agree with TypeHelpers::GetVoidType()");
        VSASSERT(T->GetKind()==SYM_VoidType && T->GetVtype()==t_void,
                 "void kind/type mismatch");
        VSASSERT(T->IsType() && T->IsSimpleType() && T->IsVoidType() && !T->IsObject() && !T->IsCOMValueType() &&
                 !T->IsStruct() && !T->IsDelegate() && !T->IsEnum() && !T->IsAnonymousType() && !T->IsAnonymousDelegate() &&
                 !T->IsIntrinsicType() && 
                 !T->IsGeneric() && T->GetFirstGenericParam()==0 && T->GetGenericParamCount()==0 && T->GetFirstImplements()==0 &&
                 !T->IsPointerType() && !T->IsNamedType() && !T->IsArrayType() && !T->IsArrayLiteralType() && !T->IsNamedRoot() &&
                 !T->IsGenericBadNamedRoot() && !T->IsContainer() && !T->IsClass() && !T->IsInterface() && !T->IsGenericParam() &&
                 !T->IsGenericBinding() && !T->IsGenericTypeBinding() && !T->IsTypeForwarder(),
                 "void inconsistent according to BCSYM");
        known++;
    }



    // ==============================================================================================================================================
    // BOOLEAN
    // ==============================================================================================================================================
    if (TypeHelpers::IsBooleanType(T))
    {
        VSASSERT(!TypeHelpers::IsByteType(T) && !TypeHelpers::IsSignedByteType(T) && !TypeHelpers::IsShortType(T) && !TypeHelpers::IsUnsignedShortType(T) &&
             !TypeHelpers::IsIntegerType(T) && !TypeHelpers::IsUnsignedIntegerType(T) && !TypeHelpers::IsLongType(T) && !TypeHelpers::IsUnsignedLongType(T) &&
             !TypeHelpers::IsIntegralType(T) && !TypeHelpers::IsUnsignedType(T) && !TypeHelpers::IsDecimalType(T) && !TypeHelpers::IsSingleType(T) &&
             !TypeHelpers::IsFloatingType(T) && !TypeHelpers::IsNumericType(T) && !TypeHelpers::IsDateType(T) && TypeHelpers::IsBooleanType(T) &&
             !TypeHelpers::IsCharType(T) && !TypeHelpers::IsStringType(T) && !TypeHelpers::IsArrayType(T) && !TypeHelpers::IsCharArrayRankOne(T) &&
             !TypeHelpers::IsUndefinedType(T) && !TypeHelpers::IsPointerType(T) && !TypeHelpers::IsVoidType(T) &&
             !TypeHelpers::IsRootObjectType(T) && TypeHelpers::IsIntrinsicType(T) &&
             !TypeHelpers::IsInterfaceType(T) && !TypeHelpers::IsClassType(T) && !TypeHelpers::IsRecordType(T) &&
             // confusingly, for an boolean, IsClassOrRecordType() but neither IsClassType() nor IsRecordType()
             TypeHelpers::IsClassOrRecordType(T) && !TypeHelpers::IsClassOrInterfaceType(T) && !TypeHelpers::IsClassInterfaceOrRecordType(T) &&
             !TypeHelpers::IsClassInterfaceRecordOrGenericParamType(T) && !TypeHelpers::IsEnumType(T) && !TypeHelpers::IsDelegateType(T) &&
             !TypeHelpers::IsGenericParameter(T) && !TypeHelpers::IsGenericTypeBinding(T) && TypeHelpers::IsValueTypeOrGenericParameter(T) &&
             !TypeHelpers::HasVariance(T) && !TypeHelpers::IsReferenceType(T) && TypeHelpers::IsValueType(T),
             "boolean inconsistent according to TypeHelpers");
        VSASSERT(T->GetKind()==SYM_Class && T->GetVtype()==t_bool,
             "boolean kind/type mismatch");
        VSASSERT(T->IsType() && !T->IsSimpleType() && !T->IsVoidType() && !T->IsObject() && !T->IsCOMValueType() &&
             T->IsStruct() && !T->IsDelegate() && !T->IsEnum() && !T->IsAnonymousType() && !T->IsAnonymousDelegate() &&
             T->IsIntrinsicType() && 
             !T->IsGeneric() && T->GetFirstGenericParam()==0 && T->GetGenericParamCount()==0 && /*T->GetFirstImplements()?=0 &&*/
             !T->IsPointerType() && !T->IsNamedType() && !T->IsArrayType() && !T->IsArrayLiteralType() && T->IsNamedRoot() &&
             !T->IsGenericBadNamedRoot() && T->IsContainer() && T->IsClass() && !T->IsInterface() && !T->IsGenericParam() &&
             !T->IsGenericBinding() && !T->IsGenericTypeBinding() && !T->IsTypeForwarder(),
             "boolean inconsistent according to BCSYM");
        known++;
    }



    // ==============================================================================================================================================
    // INTEGRAL TYPES: PRIMITIVE INTEGRALS (i1,ui1,i2,ui2,i4,ui4,i8,ui8)
    // ==============================================================================================================================================
    if (TypeHelpers::IsIntegralType(T) && !TypeHelpers::IsEnumType(T))
    {
        VSASSERT(
             (TypeHelpers::IsByteType(T) || TypeHelpers::IsSignedByteType(T) || !TypeHelpers::IsShortType(T) || !TypeHelpers::IsUnsignedShortType(T) ||
              TypeHelpers::IsIntegerType(T) || TypeHelpers::IsUnsignedIntegerType(T) || TypeHelpers::IsLongType(T) || TypeHelpers::IsUnsignedLongType(T)) &&
             TypeHelpers::IsIntegralType(T) && !TypeHelpers::IsDecimalType(T) && !TypeHelpers::IsSingleType(T) &&
             !TypeHelpers::IsFloatingType(T) && TypeHelpers::IsNumericType(T) && !TypeHelpers::IsDateType(T) && !TypeHelpers::IsBooleanType(T) &&
             !TypeHelpers::IsCharType(T) && !TypeHelpers::IsStringType(T) && !TypeHelpers::IsArrayType(T) && !TypeHelpers::IsCharArrayRankOne(T) &&
             !TypeHelpers::IsUndefinedType(T) && !TypeHelpers::IsPointerType(T) && !TypeHelpers::IsVoidType(T) &&
             !TypeHelpers::IsRootObjectType(T) && TypeHelpers::IsIntrinsicType(T) &&
             !TypeHelpers::IsInterfaceType(T) && !TypeHelpers::IsClassType(T) && !TypeHelpers::IsRecordType(T) &&
             // confusingly, for an integral, IsClassOrRecordType() but neither IsClassType() nor IsRecordType()
             TypeHelpers::IsClassOrRecordType(T) && !TypeHelpers::IsClassOrInterfaceType(T) && !TypeHelpers::IsClassInterfaceOrRecordType(T) &&
             !TypeHelpers::IsClassInterfaceRecordOrGenericParamType(T) && !TypeHelpers::IsEnumType(T) && !TypeHelpers::IsDelegateType(T) &&
             !TypeHelpers::IsGenericParameter(T) && !TypeHelpers::IsGenericTypeBinding(T) && TypeHelpers::IsValueTypeOrGenericParameter(T) &&
             !TypeHelpers::HasVariance(T) && !TypeHelpers::IsReferenceType(T) && TypeHelpers::IsValueType(T),
             "integral inconsistent according to TypeHelpers");
        VSASSERT(T->GetKind()==SYM_Class && (T->GetVtype()==t_i1 || T->GetVtype()==t_ui1 || T->GetVtype()==t_i2 ||
            T->GetVtype()==t_ui2 || T->GetVtype()==t_i4 || T->GetVtype()==t_ui4 || T->GetVtype()==t_i8 ||
            T->GetVtype()==t_ui8) && T->GetVtype()>=t_i1 && T->GetVtype()<=t_ui8,
             "integral kind/type mismatch");
        VSASSERT(T->IsType() && !T->IsSimpleType() && !T->IsVoidType() && !T->IsObject() && !T->IsCOMValueType() &&
             T->IsStruct() && !T->IsDelegate() && !T->IsEnum() && !T->IsAnonymousType() && !T->IsAnonymousDelegate() &&
             T->IsIntrinsicType() && 
             !T->IsGeneric() && T->GetFirstGenericParam()==0 && T->GetGenericParamCount()==0 && /*T->GetFirstImplements()?=0 &&*/
             !T->IsPointerType() && !T->IsNamedType() && !T->IsArrayType() && !T->IsArrayLiteralType() && T->IsNamedRoot() &&
             !T->IsGenericBadNamedRoot() && T->IsContainer() && T->IsClass() && !T->IsInterface() && !T->IsGenericParam() &&
             !T->IsGenericBinding() && !T->IsGenericTypeBinding() && !T->IsTypeForwarder(),
             "integral inconsistent according to BCSYM");
        known++;
    }


    // ==============================================================================================================================================
    // INTEGRAL TYPES: ENUMS
    // ==============================================================================================================================================
    if (TypeHelpers::IsEnumType(T))
    {
        VSASSERT(
             (TypeHelpers::IsByteType(T) || TypeHelpers::IsSignedByteType(T) || !TypeHelpers::IsShortType(T) || !TypeHelpers::IsUnsignedShortType(T) ||
              TypeHelpers::IsIntegerType(T) || TypeHelpers::IsUnsignedIntegerType(T) || TypeHelpers::IsLongType(T) || TypeHelpers::IsUnsignedLongType(T)) &&
             TypeHelpers::IsIntegralType(T) && !TypeHelpers::IsDecimalType(T) && !TypeHelpers::IsSingleType(T) &&
             !TypeHelpers::IsFloatingType(T) && TypeHelpers::IsNumericType(T) && !TypeHelpers::IsDateType(T) && !TypeHelpers::IsBooleanType(T) &&
             !TypeHelpers::IsCharType(T) && !TypeHelpers::IsStringType(T) && !TypeHelpers::IsArrayType(T) && !TypeHelpers::IsCharArrayRankOne(T) &&
             !TypeHelpers::IsUndefinedType(T) && !TypeHelpers::IsPointerType(T) && !TypeHelpers::IsVoidType(T) &&
             !TypeHelpers::IsRootObjectType(T) && !TypeHelpers::IsIntrinsicType(T) &&
             // confusingly, for an enum, IsClassOrRecordType() but neither IsClassType() nor IsRecordType()
             !TypeHelpers::IsInterfaceType(T) && !TypeHelpers::IsClassType(T) && !TypeHelpers::IsRecordType(T) &&
             TypeHelpers::IsClassOrRecordType(T) && !TypeHelpers::IsClassOrInterfaceType(T) && !TypeHelpers::IsClassInterfaceOrRecordType(T) &&
             !TypeHelpers::IsGenericParameter(T) && /*?TypeHelpers::IsGenericTypeBinding(T) &&*/ TypeHelpers::IsValueTypeOrGenericParameter(T) &&
             !TypeHelpers::HasVariance(T) && !TypeHelpers::IsReferenceType(T) && TypeHelpers::IsValueType(T),
             "enum inconsistent according to TypeHelpers");
        VSASSERT((T->GetKind()==SYM_Class || (TypeHelpers::IsGenericTypeBinding(T) && T->PGenericBinding()->GetGeneric()->GetKind()==SYM_Class)) &&
             (T->GetVtype()==t_i1 || T->GetVtype()==t_ui1 || T->GetVtype()==t_i2 ||
             T->GetVtype()==t_ui2 || T->GetVtype()==t_i4 || T->GetVtype()==t_ui4 || T->GetVtype()==t_i8 ||
             T->GetVtype()==t_ui8) && T->GetVtype()>=t_i1 && T->GetVtype()<=t_ui8,
             "enum kind/type mismatch");
        VSASSERT(T->IsType() && !T->IsSimpleType() && !T->IsVoidType() && !T->IsObject() && !T->IsCOMValueType() &&
             T->IsStruct() && !T->IsDelegate() && T->IsEnum() && !T->IsAnonymousType() && !T->IsAnonymousDelegate() &&
             !T->IsIntrinsicType() && 
             !T->IsGeneric() && T->GetFirstGenericParam()==0 && T->GetGenericParamCount()==0 && T->GetFirstImplements()==0 &&
             !T->IsPointerType() && !T->IsNamedType() && !T->IsArrayType() && !T->IsArrayLiteralType() && T->IsNamedRoot() &&
             !T->IsGenericBadNamedRoot() && T->IsContainer() && T->IsClass() && !T->IsInterface() && !T->IsGenericParam() &&
             /*?T->IsGenericBinding() && ?T->IsGenericTypeBinding() &&*/ !T->IsTypeForwarder(),
             "enum inconsistent according to BCSYM");
        known++;
    }


    // ==============================================================================================================================================
    // PRIMITIVE FLOATS (r4,r8)
    // ==============================================================================================================================================
    if (TypeHelpers::IsFloatingType(T)) 
    {
        VSASSERT(!TypeHelpers::IsByteType(T) && !TypeHelpers::IsSignedByteType(T) && !TypeHelpers::IsShortType(T) && !TypeHelpers::IsUnsignedShortType(T) &&
             !TypeHelpers::IsIntegerType(T) && !TypeHelpers::IsUnsignedIntegerType(T) && !TypeHelpers::IsLongType(T) && !TypeHelpers::IsUnsignedLongType(T) &&
             !TypeHelpers::IsIntegralType(T) && !TypeHelpers::IsUnsignedType(T) && !TypeHelpers::IsDecimalType(T) && /*?TypeHelpers::IsSingleType(T) &&*/
             TypeHelpers::IsFloatingType(T) && TypeHelpers::IsNumericType(T) && !TypeHelpers::IsDateType(T) && !TypeHelpers::IsBooleanType(T) &&
             !TypeHelpers::IsCharType(T) && !TypeHelpers::IsStringType(T) && !TypeHelpers::IsArrayType(T) && !TypeHelpers::IsCharArrayRankOne(T) &&
             !TypeHelpers::IsUndefinedType(T) && !TypeHelpers::IsPointerType(T) && !TypeHelpers::IsVoidType(T) &&
             !TypeHelpers::IsRootObjectType(T) && TypeHelpers::IsIntrinsicType(T) &&
             !TypeHelpers::IsInterfaceType(T) && !TypeHelpers::IsClassType(T) && !TypeHelpers::IsRecordType(T) &&
             // confusingly, for a float, IsClassOrRecordType() but neither IsClassType() nor IsRecordType()
             TypeHelpers::IsClassOrRecordType(T) && !TypeHelpers::IsClassOrInterfaceType(T) && !TypeHelpers::IsClassInterfaceOrRecordType(T) &&
             !TypeHelpers::IsClassInterfaceRecordOrGenericParamType(T) && !TypeHelpers::IsEnumType(T) && !TypeHelpers::IsDelegateType(T) &&
             !TypeHelpers::IsGenericParameter(T) && !TypeHelpers::IsGenericTypeBinding(T) && TypeHelpers::IsValueTypeOrGenericParameter(T) &&
             !TypeHelpers::HasVariance(T) && !TypeHelpers::IsReferenceType(T) && TypeHelpers::IsValueType(T),
             "float inconsistent according to TypeHelpers");
        VSASSERT(T->GetKind()==SYM_Class && (T->GetVtype()==t_single || T->GetVtype()==t_double),
             "float kind/type mismatch");
        VSASSERT(T->IsType() && !T->IsSimpleType() && !T->IsVoidType() && !T->IsObject() && !T->IsCOMValueType() &&
             T->IsStruct() && !T->IsDelegate() && !T->IsEnum() && !T->IsAnonymousType() && !T->IsAnonymousDelegate() &&
             T->IsIntrinsicType() && 
             !T->IsGeneric() && T->GetFirstGenericParam()==0 && T->GetGenericParamCount()==0 && /*T->GetFirstImplements()?=0 &&*/
             !T->IsPointerType() && !T->IsNamedType() && !T->IsArrayType() && !T->IsArrayLiteralType() && T->IsNamedRoot() &&
             !T->IsGenericBadNamedRoot() && T->IsContainer() && T->IsClass() && !T->IsInterface() && !T->IsGenericParam() &&
             !T->IsGenericBinding() && !T->IsGenericTypeBinding() && !T->IsTypeForwarder(),
             "float inconsistent according to BCSYM");
        known++;
    }


    // ==============================================================================================================================================
    // DECIMAL
    // ==============================================================================================================================================
    if (TypeHelpers::IsDecimalType(T))
    {
        VSASSERT(!TypeHelpers::IsByteType(T) && !TypeHelpers::IsSignedByteType(T) && !TypeHelpers::IsShortType(T) && !TypeHelpers::IsUnsignedShortType(T) &&
             !TypeHelpers::IsIntegerType(T) && !TypeHelpers::IsUnsignedIntegerType(T) && !TypeHelpers::IsLongType(T) && !TypeHelpers::IsUnsignedLongType(T) &&
             !TypeHelpers::IsIntegralType(T) && !TypeHelpers::IsUnsignedType(T) && TypeHelpers::IsDecimalType(T) && !TypeHelpers::IsSingleType(T) &&
             !TypeHelpers::IsFloatingType(T) && TypeHelpers::IsNumericType(T) && !TypeHelpers::IsDateType(T) && !TypeHelpers::IsBooleanType(T) &&
             !TypeHelpers::IsCharType(T) && !TypeHelpers::IsStringType(T) && !TypeHelpers::IsArrayType(T) && !TypeHelpers::IsCharArrayRankOne(T) &&
             !TypeHelpers::IsUndefinedType(T) && !TypeHelpers::IsPointerType(T) && !TypeHelpers::IsVoidType(T) &&
             !TypeHelpers::IsRootObjectType(T) && TypeHelpers::IsIntrinsicType(T) &&
             !TypeHelpers::IsInterfaceType(T) && !TypeHelpers::IsClassType(T) && !TypeHelpers::IsRecordType(T) &&
             // confusingly, for a decimal, IsClassOrRecordType() but neither IsClassType() nor IsRecordType()
             TypeHelpers::IsClassOrRecordType(T) && !TypeHelpers::IsClassOrInterfaceType(T) && !TypeHelpers::IsClassInterfaceOrRecordType(T) &&
             !TypeHelpers::IsClassInterfaceRecordOrGenericParamType(T) && !TypeHelpers::IsEnumType(T) && !TypeHelpers::IsDelegateType(T) &&
             !TypeHelpers::IsGenericParameter(T) && !TypeHelpers::IsGenericTypeBinding(T) && TypeHelpers::IsValueTypeOrGenericParameter(T) &&
             !TypeHelpers::HasVariance(T) && !TypeHelpers::IsReferenceType(T) && TypeHelpers::IsValueType(T),
             "decimal inconsistent according to TypeHelpers");
        VSASSERT(T->GetKind()==SYM_Class && T->GetVtype()==t_decimal,
             "decimal kind/type mismatch");
        VSASSERT(T->IsType() && !T->IsSimpleType() && !T->IsVoidType() && !T->IsObject() && !T->IsCOMValueType() &&
             T->IsStruct() && !T->IsDelegate() && !T->IsEnum() && !T->IsAnonymousType() && !T->IsAnonymousDelegate() &&
             T->IsIntrinsicType() && 
             !T->IsGeneric() && T->GetFirstGenericParam()==0 && T->GetGenericParamCount()==0 && /*T->GetFirstImplements()?=0 &&*/
             !T->IsPointerType() && !T->IsNamedType() && !T->IsArrayType() && !T->IsArrayLiteralType() && T->IsNamedRoot() &&
             !T->IsGenericBadNamedRoot() && T->IsContainer() && T->IsClass() && !T->IsInterface() && !T->IsGenericParam() &&
             !T->IsGenericBinding() && !T->IsGenericTypeBinding() && !T->IsTypeForwarder(),
             "decimal inconsistent according to BCSYM");
        known++;
    }


    // ==============================================================================================================================================
    // DATE
    // ==============================================================================================================================================
    if (TypeHelpers::IsDateType(T))
    {
        VSASSERT(!TypeHelpers::IsByteType(T) && !TypeHelpers::IsSignedByteType(T) && !TypeHelpers::IsShortType(T) && !TypeHelpers::IsUnsignedShortType(T) &&
             !TypeHelpers::IsIntegerType(T) && !TypeHelpers::IsUnsignedIntegerType(T) && !TypeHelpers::IsLongType(T) && !TypeHelpers::IsUnsignedLongType(T) &&
             !TypeHelpers::IsIntegralType(T) && !TypeHelpers::IsUnsignedType(T) && !TypeHelpers::IsDecimalType(T) && !TypeHelpers::IsSingleType(T) &&
             !TypeHelpers::IsFloatingType(T) && !TypeHelpers::IsNumericType(T) && TypeHelpers::IsDateType(T) && !TypeHelpers::IsBooleanType(T) &&
             !TypeHelpers::IsCharType(T) && !TypeHelpers::IsStringType(T) && !TypeHelpers::IsArrayType(T) && !TypeHelpers::IsCharArrayRankOne(T) &&
             !TypeHelpers::IsUndefinedType(T) && !TypeHelpers::IsPointerType(T) && !TypeHelpers::IsVoidType(T) &&
             !TypeHelpers::IsRootObjectType(T) && TypeHelpers::IsIntrinsicType(T) &&
             !TypeHelpers::IsInterfaceType(T) && !TypeHelpers::IsClassType(T) && !TypeHelpers::IsRecordType(T) &&
             // confusingly, for a date, IsClassOrRecordType() but neither IsClassType() nor IsRecordType()
             TypeHelpers::IsClassOrRecordType(T) && !TypeHelpers::IsClassOrInterfaceType(T) && !TypeHelpers::IsClassInterfaceOrRecordType(T) &&
             !TypeHelpers::IsClassInterfaceRecordOrGenericParamType(T) && !TypeHelpers::IsEnumType(T) && !TypeHelpers::IsDelegateType(T) &&
             !TypeHelpers::IsGenericParameter(T) && !TypeHelpers::IsGenericTypeBinding(T) && TypeHelpers::IsValueTypeOrGenericParameter(T) &&
             !TypeHelpers::HasVariance(T) && !TypeHelpers::IsReferenceType(T) && TypeHelpers::IsValueType(T),
             "date inconsistent according to TypeHelpers");
        VSASSERT(T->GetKind()==SYM_Class && T->GetVtype()==t_date,
             "date kind/type mismatch");
        VSASSERT(T->IsType() && !T->IsSimpleType() && !T->IsVoidType() && !T->IsObject() && !T->IsCOMValueType() &&
             T->IsStruct() && !T->IsDelegate() && !T->IsEnum() && !T->IsAnonymousType() && !T->IsAnonymousDelegate() &&
             T->IsIntrinsicType() && 
             !T->IsGeneric() && T->GetFirstGenericParam()==0 && T->GetGenericParamCount()==0 && /*T->GetFirstImplements()?=0 &&*/
             !T->IsPointerType() && !T->IsNamedType() && !T->IsArrayType() && !T->IsArrayLiteralType() && T->IsNamedRoot() &&
             !T->IsGenericBadNamedRoot() && T->IsContainer() && T->IsClass() && !T->IsInterface() && !T->IsGenericParam() &&
             !T->IsGenericBinding() && !T->IsGenericTypeBinding() && !T->IsTypeForwarder(),
             "date inconsistent according to BCSYM");
        known++;
    }


    // ==============================================================================================================================================
    // CHAR
    // ==============================================================================================================================================
    if (TypeHelpers::IsCharType(T))
    {
        VSASSERT(!TypeHelpers::IsByteType(T) && !TypeHelpers::IsSignedByteType(T) && !TypeHelpers::IsShortType(T) && !TypeHelpers::IsUnsignedShortType(T) &&
             !TypeHelpers::IsIntegerType(T) && !TypeHelpers::IsUnsignedIntegerType(T) && !TypeHelpers::IsLongType(T) && !TypeHelpers::IsUnsignedLongType(T) &&
             !TypeHelpers::IsIntegralType(T) && !TypeHelpers::IsUnsignedType(T) && !TypeHelpers::IsDecimalType(T) && !TypeHelpers::IsSingleType(T) &&
             !TypeHelpers::IsFloatingType(T) && !TypeHelpers::IsNumericType(T) && !TypeHelpers::IsDateType(T) && !TypeHelpers::IsBooleanType(T) &&
             TypeHelpers::IsCharType(T) && !TypeHelpers::IsStringType(T) && !TypeHelpers::IsArrayType(T) && !TypeHelpers::IsCharArrayRankOne(T) &&
             !TypeHelpers::IsUndefinedType(T) && !TypeHelpers::IsPointerType(T) && !TypeHelpers::IsVoidType(T) &&
             !TypeHelpers::IsRootObjectType(T) && TypeHelpers::IsIntrinsicType(T) &&
             !TypeHelpers::IsInterfaceType(T) && !TypeHelpers::IsClassType(T) && !TypeHelpers::IsRecordType(T) &&
             // confusingly, for a char, IsClassOrRecordType() but neither IsClassType() nor IsRecordType()
             TypeHelpers::IsClassOrRecordType(T) && !TypeHelpers::IsClassOrInterfaceType(T) && !TypeHelpers::IsClassInterfaceOrRecordType(T) &&
             !TypeHelpers::IsClassInterfaceRecordOrGenericParamType(T) && !TypeHelpers::IsEnumType(T) && !TypeHelpers::IsDelegateType(T) &&
             !TypeHelpers::IsGenericParameter(T) && !TypeHelpers::IsGenericTypeBinding(T) && TypeHelpers::IsValueTypeOrGenericParameter(T) &&
             !TypeHelpers::HasVariance(T) && !TypeHelpers::IsReferenceType(T) && TypeHelpers::IsValueType(T),
             "char inconsistent according to TypeHelpers");
        VSASSERT(T->GetKind()==SYM_Class && T->GetVtype()==t_char,
             "char kind/type mismatch");
        VSASSERT(T->IsType() && !T->IsSimpleType() && !T->IsVoidType() && !T->IsObject() && !T->IsCOMValueType() &&
             T->IsStruct() && !T->IsDelegate() && !T->IsEnum() && !T->IsAnonymousType() && !T->IsAnonymousDelegate() &&
             T->IsIntrinsicType() && 
             !T->IsGeneric() && T->GetFirstGenericParam()==0 && T->GetGenericParamCount()==0 && /*T->GetFirstImplements()?=0 &&*/
             !T->IsPointerType() && !T->IsNamedType() && !T->IsArrayType() && !T->IsArrayLiteralType() && T->IsNamedRoot() &&
             !T->IsGenericBadNamedRoot() && T->IsContainer() && T->IsClass() && !T->IsInterface() && !T->IsGenericParam() &&
             !T->IsGenericBinding() && !T->IsGenericTypeBinding() && !T->IsTypeForwarder(),
             "char inconsistent according to BCSYM");
        known++;
    }



    // ==============================================================================================================================================
    // STRING
    // ==============================================================================================================================================
    if (TypeHelpers::IsStringType(T))
    {
        VSASSERT(!TypeHelpers::IsByteType(T) && !TypeHelpers::IsSignedByteType(T) && !TypeHelpers::IsShortType(T) && !TypeHelpers::IsUnsignedShortType(T) &&
             !TypeHelpers::IsIntegerType(T) && !TypeHelpers::IsUnsignedIntegerType(T) && !TypeHelpers::IsLongType(T) && !TypeHelpers::IsUnsignedLongType(T) &&
             !TypeHelpers::IsIntegralType(T) && !TypeHelpers::IsUnsignedType(T) && !TypeHelpers::IsDecimalType(T) && !TypeHelpers::IsSingleType(T) &&
             !TypeHelpers::IsFloatingType(T) && !TypeHelpers::IsNumericType(T) && !TypeHelpers::IsDateType(T) && !TypeHelpers::IsBooleanType(T) &&
             !TypeHelpers::IsCharType(T) && TypeHelpers::IsStringType(T) && !TypeHelpers::IsArrayType(T) && !TypeHelpers::IsCharArrayRankOne(T) &&
             !TypeHelpers::IsUndefinedType(T) && !TypeHelpers::IsPointerType(T) && !TypeHelpers::IsVoidType(T) &&
             !TypeHelpers::IsRootObjectType(T) && TypeHelpers::IsIntrinsicType(T) &&
             !TypeHelpers::IsInterfaceType(T) && TypeHelpers::IsClassType(T) && !TypeHelpers::IsRecordType(T) &&
             TypeHelpers::IsClassOrRecordType(T) && TypeHelpers::IsClassOrInterfaceType(T) && TypeHelpers::IsClassInterfaceOrRecordType(T) &&
             TypeHelpers::IsClassInterfaceRecordOrGenericParamType(T) && !TypeHelpers::IsEnumType(T) && !TypeHelpers::IsDelegateType(T) &&
             !TypeHelpers::IsGenericParameter(T) && !TypeHelpers::IsGenericTypeBinding(T) && !TypeHelpers::IsValueTypeOrGenericParameter(T) &&
             !TypeHelpers::HasVariance(T) && TypeHelpers::IsReferenceType(T) && !TypeHelpers::IsValueType(T),
             "string inconsistent according to TypeHelpers");
        VSASSERT(T->GetKind()==SYM_Class && T->GetVtype()==t_string,
             "string kind/type mismatch");
        VSASSERT(T->IsType() && !T->IsSimpleType() && !T->IsVoidType() && !T->IsObject() && !T->IsCOMValueType() &&
             !T->IsStruct() && !T->IsDelegate() && !T->IsEnum() && !T->IsAnonymousType() && !T->IsAnonymousDelegate() &&
             T->IsIntrinsicType() && 
             !T->IsGeneric() && T->GetFirstGenericParam()==0 && T->GetGenericParamCount()==0 && /*T->GetFirstImplements()?=0 &&*/
             !T->IsPointerType() && !T->IsNamedType() && !T->IsArrayType() && !T->IsArrayLiteralType() && T->IsNamedRoot() &&
             !T->IsGenericBadNamedRoot() && T->IsContainer() && T->IsClass() && !T->IsInterface() && !T->IsGenericParam() &&
             !T->IsGenericBinding() && !T->IsGenericTypeBinding() && !T->IsTypeForwarder(),
             "string inconsistent according to BCSYM");
        known++;
    }


    // ==============================================================================================================================================
    // ARRAYS
    // ==============================================================================================================================================
    if (TypeHelpers::IsArrayType(T))
    {
        VSASSERT(!TypeHelpers::IsByteType(T) && !TypeHelpers::IsSignedByteType(T) && !TypeHelpers::IsShortType(T) && !TypeHelpers::IsUnsignedShortType(T) &&
             !TypeHelpers::IsIntegerType(T) && !TypeHelpers::IsUnsignedIntegerType(T) && !TypeHelpers::IsLongType(T) && !TypeHelpers::IsUnsignedLongType(T) &&
             !TypeHelpers::IsIntegralType(T) && !TypeHelpers::IsUnsignedType(T) && !TypeHelpers::IsDecimalType(T) && !TypeHelpers::IsSingleType(T) &&
             !TypeHelpers::IsFloatingType(T) && !TypeHelpers::IsNumericType(T) && !TypeHelpers::IsDateType(T) && !TypeHelpers::IsBooleanType(T) &&
             !TypeHelpers::IsCharType(T) && !TypeHelpers::IsStringType(T) && TypeHelpers::IsArrayType(T) && /*?TypeHelpers::IsCharArrayRankOne(T) &&*/
             !TypeHelpers::IsUndefinedType(T) && !TypeHelpers::IsPointerType(T) && !TypeHelpers::IsVoidType(T) &&
             !TypeHelpers::IsRootObjectType(T) && !TypeHelpers::IsIntrinsicType(T) &&
             !TypeHelpers::IsInterfaceType(T) && !TypeHelpers::IsClassType(T) && !TypeHelpers::IsRecordType(T) &&
             !TypeHelpers::IsClassOrRecordType(T) && !TypeHelpers::IsClassOrInterfaceType(T) && !TypeHelpers::IsClassInterfaceOrRecordType(T) &&
             !TypeHelpers::IsClassInterfaceRecordOrGenericParamType(T) && !TypeHelpers::IsEnumType(T) && !TypeHelpers::IsDelegateType(T) &&
             !TypeHelpers::IsGenericParameter(T) && !TypeHelpers::IsGenericTypeBinding(T) && !TypeHelpers::IsValueTypeOrGenericParameter(T) &&
             !TypeHelpers::HasVariance(T) && TypeHelpers::IsReferenceType(T) && !TypeHelpers::IsValueType(T),
             "array inconsistent according to TypeHelpers");
        VSASSERT((T->GetKind()==SYM_ArrayType || T->GetKind() == SYM_ArrayLiteralType) && T->GetVtype()==t_array,
             "array kind/type mismatch");
        VSASSERT(T->IsType() && T->IsSimpleType() && !T->IsVoidType() && !T->IsObject() && !T->IsCOMValueType() &&
             !T->IsStruct() && !T->IsDelegate() && !T->IsEnum() && !T->IsAnonymousType() && !T->IsAnonymousDelegate() &&
             !T->IsIntrinsicType() && 
             !T->IsGeneric() && T->GetFirstGenericParam()==0 && T->GetGenericParamCount()==0 && T->GetFirstImplements()==0 &&
             !T->IsPointerType() && !T->IsNamedType() && T->IsArrayType() && /*?T->IsArrayLiteralType() &&*/ !T->IsNamedRoot() &&
             !T->IsGenericBadNamedRoot() && !T->IsContainer() && !T->IsClass() && !T->IsInterface() && !T->IsGenericParam() &&
             !T->IsGenericBinding() && !T->IsGenericTypeBinding() && !T->IsTypeForwarder(),
             "array inconsistent according to BCSYM");
        known++;
    }



    // ==============================================================================================================================================
    // USER-DEFINED STRUCTURES
    // ==============================================================================================================================================
    if (TypeHelpers::IsRecordType(T))
    {
        VSASSERT(!TypeHelpers::IsByteType(T) && !TypeHelpers::IsSignedByteType(T) && !TypeHelpers::IsShortType(T) && !TypeHelpers::IsUnsignedShortType(T) &&
             !TypeHelpers::IsIntegerType(T) && !TypeHelpers::IsUnsignedIntegerType(T) && !TypeHelpers::IsLongType(T) && !TypeHelpers::IsUnsignedLongType(T) &&
             !TypeHelpers::IsIntegralType(T) && !TypeHelpers::IsUnsignedType(T) && !TypeHelpers::IsDecimalType(T) && !TypeHelpers::IsSingleType(T) &&
             !TypeHelpers::IsFloatingType(T) && !TypeHelpers::IsNumericType(T) && !TypeHelpers::IsDateType(T) && !TypeHelpers::IsBooleanType(T) &&
             !TypeHelpers::IsCharType(T) && !TypeHelpers::IsStringType(T) && !TypeHelpers::IsArrayType(T) && !TypeHelpers::IsCharArrayRankOne(T) &&
             !TypeHelpers::IsUndefinedType(T) && !TypeHelpers::IsPointerType(T) && !TypeHelpers::IsVoidType(T) &&
             !TypeHelpers::IsRootObjectType(T) && !TypeHelpers::IsIntrinsicType(T) &&
             !TypeHelpers::IsInterfaceType(T) && !TypeHelpers::IsClassType(T) && TypeHelpers::IsRecordType(T) &&
             TypeHelpers::IsClassOrRecordType(T) && !TypeHelpers::IsClassOrInterfaceType(T) && TypeHelpers::IsClassInterfaceOrRecordType(T) &&
             !TypeHelpers::IsGenericParameter(T) && /*?TypeHelpers::IsGenericTypeBinding(T) &&*/ TypeHelpers::IsValueTypeOrGenericParameter(T) &&
             !TypeHelpers::HasVariance(T) && !TypeHelpers::IsReferenceType(T) && TypeHelpers::IsValueType(T),
             "struct inconsistent according to TypeHelpers");
        VSASSERT((T->GetKind()==SYM_Class || (TypeHelpers::IsGenericTypeBinding(T) && T->PGenericTypeBinding()->GetGeneric()->GetKind()==SYM_Class)) &&
             T->GetVtype()==t_struct,
             "struct kind/type mismatch");
        VSASSERT(T->IsType() && !T->IsSimpleType() && !T->IsVoidType() && !T->IsObject() && !T->IsCOMValueType() &&
             T->IsStruct() && !T->IsDelegate() && !T->IsEnum() && !T->IsAnonymousType() && !T->IsAnonymousDelegate() &&
             !T->IsIntrinsicType() && 
             /*?T->IsGeneric() && T->GetFirstGenericParam()==0 && T->GetGenericParamCount()==0 && */ /*?T->GetFirstImplements()==0 &&*/
             !T->IsPointerType() && !T->IsNamedType() && !T->IsArrayType() && !T->IsArrayLiteralType() && T->IsNamedRoot() &&
             !T->IsGenericBadNamedRoot() && T->IsContainer() && T->IsClass() && !T->IsInterface() && !T->IsGenericParam() &&
             /*?T->IsGenericBinding() && ?T->IsGenericTypeBinding() &&*/ !T->IsTypeForwarder(),
             "struct inconsistent according to BCSYM");
        known++;
    }


    // ==============================================================================================================================================
    // USER-DEFINED REFERENCE TYPES: INTERFACES
    // ==============================================================================================================================================
    if (TypeHelpers::IsInterfaceType(T))
    {
        VSASSERT(!TypeHelpers::IsByteType(T) && !TypeHelpers::IsSignedByteType(T) && !TypeHelpers::IsShortType(T) && !TypeHelpers::IsUnsignedShortType(T) &&
             !TypeHelpers::IsIntegerType(T) && !TypeHelpers::IsUnsignedIntegerType(T) && !TypeHelpers::IsLongType(T) && !TypeHelpers::IsUnsignedLongType(T) &&
             !TypeHelpers::IsIntegralType(T) && !TypeHelpers::IsUnsignedType(T) && !TypeHelpers::IsDecimalType(T) && !TypeHelpers::IsSingleType(T) &&
             !TypeHelpers::IsFloatingType(T) && !TypeHelpers::IsNumericType(T) && !TypeHelpers::IsDateType(T) && !TypeHelpers::IsBooleanType(T) &&
             !TypeHelpers::IsCharType(T) && !TypeHelpers::IsStringType(T) && !TypeHelpers::IsArrayType(T) && !TypeHelpers::IsCharArrayRankOne(T) &&
             !TypeHelpers::IsUndefinedType(T) && !TypeHelpers::IsPointerType(T) && !TypeHelpers::IsVoidType(T) &&
             !TypeHelpers::IsRootObjectType(T) && !TypeHelpers::IsIntrinsicType(T) &&
             TypeHelpers::IsInterfaceType(T) && !TypeHelpers::IsClassType(T) && !TypeHelpers::IsRecordType(T) &&
             !TypeHelpers::IsClassOrRecordType(T) && TypeHelpers::IsClassOrInterfaceType(T) && TypeHelpers::IsClassInterfaceOrRecordType(T) &&
             TypeHelpers::IsClassInterfaceRecordOrGenericParamType(T) && !TypeHelpers::IsEnumType(T) && !TypeHelpers::IsDelegateType(T) &&
             !TypeHelpers::IsGenericParameter(T) && /*?TypeHelpers::IsGenericTypeBinding(T) &&*/ !TypeHelpers::IsValueTypeOrGenericParameter(T) &&
             /*?TypeHelpers::HasVariance(T) &&*/ TypeHelpers::IsReferenceType(T) && !TypeHelpers::IsValueType(T),
             "interface inconsistent according to TypeHelpers");
        VSASSERT((T->GetKind()==SYM_Interface || (TypeHelpers::IsGenericTypeBinding(T) && T->PGenericTypeBinding()->GetGeneric()->GetKind()==SYM_Interface)) &&
                 T->GetVtype()==t_ref,
             "interface kind/type mismatch");
        VSASSERT(T->IsType() && !T->IsSimpleType() && !T->IsVoidType() && !T->IsObject() && !T->IsCOMValueType() &&
             !T->IsStruct() && !T->IsDelegate() && !T->IsEnum() && !T->IsAnonymousType() && !T->IsAnonymousDelegate() &&
             !T->IsIntrinsicType() && 
             /*?T->IsGeneric() && ?T->GetFirstGenericParam()==0 && ?T->GetGenericParamCount()==0 &&*/ /*T->GetFirstImplements()!=0 &&*/
             !T->IsPointerType() && !T->IsNamedType() && !T->IsArrayType() && !T->IsArrayLiteralType() && T->IsNamedRoot() &&
             !T->IsGenericBadNamedRoot() && T->IsContainer() && !T->IsClass() && T->IsInterface() && !T->IsGenericParam() &&
             /*?T->IsGenericBinding() && ?T->IsGenericTypeBinding() &&*/ !T->IsTypeForwarder(),
             "interface inconsistent according to BCSYM");
        known++;
    }


    // ==============================================================================================================================================
    // USER-DEFINED REFERENCE TYPES: CLASSES
    // ==============================================================================================================================================
    if (TypeHelpers::IsClassType(T) && !TypeHelpers::IsStringType(T) && !TypeHelpers::IsDelegateType(T))
    {
        VSASSERT(!TypeHelpers::IsByteType(T) && !TypeHelpers::IsSignedByteType(T) && !TypeHelpers::IsShortType(T) && !TypeHelpers::IsUnsignedShortType(T) &&
             !TypeHelpers::IsIntegerType(T) && !TypeHelpers::IsUnsignedIntegerType(T) && !TypeHelpers::IsLongType(T) && !TypeHelpers::IsUnsignedLongType(T) &&
             !TypeHelpers::IsIntegralType(T) && !TypeHelpers::IsUnsignedType(T) && !TypeHelpers::IsDecimalType(T) && !TypeHelpers::IsSingleType(T) &&
             !TypeHelpers::IsFloatingType(T) && !TypeHelpers::IsNumericType(T) && !TypeHelpers::IsDateType(T) && !TypeHelpers::IsBooleanType(T) &&
             !TypeHelpers::IsCharType(T) && !TypeHelpers::IsStringType(T) && !TypeHelpers::IsArrayType(T) && !TypeHelpers::IsCharArrayRankOne(T) &&
             !TypeHelpers::IsUndefinedType(T) && !TypeHelpers::IsPointerType(T) && !TypeHelpers::IsVoidType(T) &&
             /*?TypeHelpers::IsRootObjectType(T) &&*/ !TypeHelpers::IsIntrinsicType(T) &&
             !TypeHelpers::IsInterfaceType(T) && TypeHelpers::IsClassType(T) && !TypeHelpers::IsRecordType(T) &&
             TypeHelpers::IsClassOrRecordType(T) && TypeHelpers::IsClassOrInterfaceType(T) && TypeHelpers::IsClassInterfaceOrRecordType(T) &&
             TypeHelpers::IsClassInterfaceRecordOrGenericParamType(T) && !TypeHelpers::IsEnumType(T) && !TypeHelpers::IsDelegateType(T) &&
             !TypeHelpers::IsGenericParameter(T) && /*?TypeHelpers::IsGenericTypeBinding(T) &&*/ !TypeHelpers::IsValueTypeOrGenericParameter(T) &&
             !TypeHelpers::HasVariance(T) && TypeHelpers::IsReferenceType(T) && !TypeHelpers::IsValueType(T),
             "class inconsistent according to TypeHelpers");
        VSASSERT((T->GetKind()==SYM_Class || (TypeHelpers::IsGenericTypeBinding(T) && T->PGenericTypeBinding()->GetGeneric()->GetKind()==SYM_Class)) &&
                 T->GetVtype()==t_ref,
             "class kind/type mismatch");
        VSASSERT(T->IsType() && !T->IsSimpleType() && !T->IsVoidType() && /*?T->IsObject() && ?T->IsCOMValueType() &&*/
             !T->IsStruct() && !T->IsDelegate() && !T->IsEnum() && /*?T->IsAnonymousType() &&*/ !T->IsAnonymousDelegate() &&
             !T->IsIntrinsicType() && 
             /*?T->IsGeneric() && ?T->GetFirstGenericParam()==0 && ?T->GetGenericParamCount()==0 &&*/ /*T->GetFirstImplements()!=0 &&*/
             !T->IsPointerType() && !T->IsNamedType() && !T->IsArrayType() && !T->IsArrayLiteralType() && T->IsNamedRoot() &&
             !T->IsGenericBadNamedRoot() && T->IsContainer() && T->IsClass() && !T->IsInterface() && !T->IsGenericParam() &&
             /*?T->IsGenericBinding() && ?T->IsGenericTypeBinding() &&*/ !T->IsTypeForwarder(),
             "class inconsistent according to BCSYM");
        known++;
    }


    // ==============================================================================================================================================
    // USER-DEFINED REFERENCE TYPES: DELEGATES
    // ==============================================================================================================================================
    if (TypeHelpers::IsDelegateType(T))
    {
        VSASSERT(!TypeHelpers::IsByteType(T) && !TypeHelpers::IsSignedByteType(T) && !TypeHelpers::IsShortType(T) && !TypeHelpers::IsUnsignedShortType(T) &&
             !TypeHelpers::IsIntegerType(T) && !TypeHelpers::IsUnsignedIntegerType(T) && !TypeHelpers::IsLongType(T) && !TypeHelpers::IsUnsignedLongType(T) &&
             !TypeHelpers::IsIntegralType(T) && !TypeHelpers::IsUnsignedType(T) && !TypeHelpers::IsDecimalType(T) && !TypeHelpers::IsSingleType(T) &&
             !TypeHelpers::IsFloatingType(T) && !TypeHelpers::IsNumericType(T) && !TypeHelpers::IsDateType(T) && !TypeHelpers::IsBooleanType(T) &&
             !TypeHelpers::IsCharType(T) && !TypeHelpers::IsStringType(T) && !TypeHelpers::IsArrayType(T) && !TypeHelpers::IsCharArrayRankOne(T) &&
             !TypeHelpers::IsUndefinedType(T) && !TypeHelpers::IsPointerType(T) && !TypeHelpers::IsVoidType(T) &&
             !TypeHelpers::IsRootObjectType(T) && !TypeHelpers::IsIntrinsicType(T) &&
             !TypeHelpers::IsInterfaceType(T) && TypeHelpers::IsClassType(T) && !TypeHelpers::IsRecordType(T) &&
             TypeHelpers::IsClassOrRecordType(T) && TypeHelpers::IsClassOrInterfaceType(T) && TypeHelpers::IsClassInterfaceOrRecordType(T) &&
             TypeHelpers::IsClassInterfaceRecordOrGenericParamType(T) && !TypeHelpers::IsEnumType(T) && TypeHelpers::IsDelegateType(T) &&
             !TypeHelpers::IsGenericParameter(T) && /*?TypeHelpers::IsGenericTypeBinding(T) &&*/ !TypeHelpers::IsValueTypeOrGenericParameter(T) &&
             /*?TypeHelpers::HasVariance(T) &&*/ TypeHelpers::IsReferenceType(T) && !TypeHelpers::IsValueType(T),
             "delegate inconsistent according to TypeHelpers");
        VSASSERT((T->GetKind()==SYM_Class || (TypeHelpers::IsGenericTypeBinding(T) && T->PGenericTypeBinding()->GetGeneric()->GetKind()==SYM_Class)) &&
                 T->GetVtype()==t_ref,
             "delegate kind/type mismatch");
        VSASSERT(T->IsType() && !T->IsSimpleType() && !T->IsVoidType() && !T->IsObject() && !T->IsCOMValueType() &&
             !T->IsStruct() && T->IsDelegate() && !T->IsEnum() && !T->IsAnonymousType() && /*?T->IsAnonymousDelegate() &&*/
             !T->IsIntrinsicType() && 
             /*?T->IsGeneric() && ?T->GetFirstGenericParam()==0 && ?T->GetGenericParamCount()==0 &&*/ /*T->GetFirstImplements()!=0 &&*/
             !T->IsPointerType() && !T->IsNamedType() && !T->IsArrayType() && !T->IsArrayLiteralType() && T->IsNamedRoot() &&
             !T->IsGenericBadNamedRoot() && T->IsContainer() && T->IsClass() && !T->IsInterface() && !T->IsGenericParam() &&
             /*?T->IsGenericBinding() && ?T->IsGenericTypeBinding() &&*/ !T->IsTypeForwarder(),
             "delegate inconsistent according to BCSYM");
        known++;
    }



    // ==============================================================================================================================================
    // USER-NAMED GENERIC PARAMETERS
    // ==============================================================================================================================================
    if (TypeHelpers::IsGenericParameter(T))
    {
        VSASSERT(!TypeHelpers::IsByteType(T) && !TypeHelpers::IsSignedByteType(T) && !TypeHelpers::IsShortType(T) && !TypeHelpers::IsUnsignedShortType(T) &&
             !TypeHelpers::IsIntegerType(T) && !TypeHelpers::IsUnsignedIntegerType(T) && !TypeHelpers::IsLongType(T) && !TypeHelpers::IsUnsignedLongType(T) &&
             !TypeHelpers::IsIntegralType(T) && !TypeHelpers::IsUnsignedType(T) && !TypeHelpers::IsDecimalType(T) && !TypeHelpers::IsSingleType(T) &&
             !TypeHelpers::IsFloatingType(T) && !TypeHelpers::IsNumericType(T) && !TypeHelpers::IsDateType(T) && !TypeHelpers::IsBooleanType(T) &&
             !TypeHelpers::IsCharType(T) && !TypeHelpers::IsStringType(T) && !TypeHelpers::IsArrayType(T) && !TypeHelpers::IsCharArrayRankOne(T) &&
             !TypeHelpers::IsUndefinedType(T) && !TypeHelpers::IsPointerType(T) && !TypeHelpers::IsVoidType(T) &&
             !TypeHelpers::IsRootObjectType(T) && !TypeHelpers::IsIntrinsicType(T) &&
             !TypeHelpers::IsInterfaceType(T) && !TypeHelpers::IsClassType(T) && !TypeHelpers::IsRecordType(T) &&
             !TypeHelpers::IsClassOrRecordType(T) && !TypeHelpers::IsClassOrInterfaceType(T) && !TypeHelpers::IsClassInterfaceOrRecordType(T) &&
             TypeHelpers::IsClassInterfaceRecordOrGenericParamType(T) && !TypeHelpers::IsEnumType(T) && !TypeHelpers::IsDelegateType(T) &&
             TypeHelpers::IsGenericParameter(T) && !TypeHelpers::IsGenericTypeBinding(T) && TypeHelpers::IsValueTypeOrGenericParameter(T) &&
             !TypeHelpers::HasVariance(T) /*&& ?TypeHelpers::IsReferenceType(T) && ?TypeHelpers::IsValueType(T)*/,
             "generic parameter inconsistent according to TypeHelpers");
        VSASSERT(T->GetKind()==SYM_GenericParam && T->GetVtype()==t_generic,
             "generic kind/type mismatch");
        VSASSERT(T->IsType() && !T->IsSimpleType() && !T->IsVoidType() && !T->IsObject() && !T->IsCOMValueType() &&
             !T->IsStruct() && !T->IsDelegate() && !T->IsEnum() && !T->IsAnonymousType() && !T->IsAnonymousDelegate() &&
             !T->IsIntrinsicType() && 
             !T->IsGeneric() && T->GetFirstGenericParam()==0 && T->GetGenericParamCount()==0 && T->GetFirstImplements()==0 &&
             !T->IsPointerType() && !T->IsNamedType() && !T->IsArrayType() && !T->IsArrayLiteralType() && T->IsNamedRoot() &&
             !T->IsGenericBadNamedRoot() && !T->IsContainer() && !T->IsClass() && !T->IsInterface() && T->IsGenericParam() &&
             !T->IsGenericBinding() && !T->IsGenericTypeBinding() && !T->IsTypeForwarder(),
             "generic inconsistent according to BCSYM");
        known++;
    }

    if (T->GetVtype()==t_ptr)
    {
        VSFAIL("Pointer types should not appear here. They should have been T->DigThroughPointerType() already.");
        known++;
    }

    VSASSERT(known<2, "The way that IsKnownType ----s a type appears to be ambiguous");
    VSASSERT(known!=0, "Unrecognized type");
    return (known>0);
}
#endif



// GetReturnTypeOfDelegateType
// Digs through a delegate type to find its return type.
// Returns "Void" if it was a Sub.
// Returns "NULL" if given something that's not a concrete delegate (e.g. Object, or System.Delegate, or String)
// Note: If "T" is a generic delegate, e.g. Func(Of T), then this function will merely return "T"...
// it's up to the caller to ReplaceGenerics on it if so needed.
Type * TypeHelpers::GetReturnTypeOfDelegateType(Type *T, Compiler *pCompiler)
{
    VSASSERT(T, "unexpected null type");
    VSASSERT(pCompiler, "unexpected null Compiler");

#if DEBUG
    // Note: System.Delegate itself doesn't satisfy "IsDelegate()". The "IsDelegate" function
    // only picks up "concrete" delegate types.
    bool b = pCompiler->GetDefaultCompilerHost()->GetFXSymbolProvider()->GetDelegateType()->IsDelegate();
    VSASSERT(!b, "oops, I didn't expect System.Delegate to be a delegate");
#endif

    if (T == NULL || !T->IsDelegate())
    {
        return NULL;
    }
    // Note: even if IsBad(T), we'll still proceed and do the best we can.
    // e.g. it might be bad because of a missing assembly reference; not because of anything
    // that prevents us from getting the return type of the delegate.

    Declaration *InvokeMethod = GetInvokeFromDelegate(T, pCompiler);
    if (InvokeMethod==NULL  || !InvokeMethod->IsProc())
    {
        return NULL;
    }
    // Likewise in the case of IsBad(InvokeMethod)

    BCSYM_Proc *DelegateProc = InvokeMethod->PProc();
    Type *DelegateReturnType = DelegateProc->GetType();
    if (DelegateReturnType == NULL || DelegateReturnType->IsVoidType())
    {
        return GetVoidType();
    }

    return DelegateReturnType;
}


// HasVariance
// this function says whether a given type is a generic binding (Of ...)
// where one or more of its generic parameters have variance, i.e.
// are "In" or "Out". Useful in several places, e.g. because an interface
// with variance isn't allowed to contain enums, but an interface without is.
// If the function is given a generic binding, it returns whether the
// underlying generic type has variance.
// 
bool TypeHelpers::HasVariance(Type * T)
{
    VSASSERT(T, "unexpected null type");

    if (T->IsGenericBinding())
    {
        T = T->PGenericBinding()->GetGeneric();
        VSASSERT(T, "unexpected null generic type");
    }

    for (BCSYM_GenericParam *gParam=T->GetFirstGenericParam(); gParam; gParam=gParam->GetNextParam())
    {
        if (gParam->GetVariance()!=Variance_None)
        {
            VSASSERT(T->IsInterface() || T->IsDelegate(), "unexpected variance on non-interface non-delegate");
            return true;
        }
    }

    return false;
}



// IsReferenceType
bool TypeHelpers::IsReferenceType(
    Type * T
)
{
    // (the function is badly named because the answer it gives for generic parameters is a little murky...
    // a more accurate name might be "IsReferenceOrIsExplicitlyConstrainedAsReference")
    //
    // This is a complete list of the types that satisfy IsReferenceType:
    //   * String (aka System.String)
    //   * Arrays
    //   * ByRef parameters (never used in the compiler)
    //   * Interfaces
    //   * Classes
    //   * Delegates
    //   * Generic parameters which have a "Ref" constraint-flag.
    //     (the actual VB keyword is "Class" but it's a badly-named keyword).
    //   * Generic parameters where they or any other generic parameter that they're constrained to be, directly
    //     or indirectly, has a non-generic-parameter type-constraint which itself satisfies IsReferenceType.
    //
    // Confusing case: given "Sub f(Of T as U, U as Ref)()", it's possible to instantiate it with
    // "f(Of MyStruct, InterfaceThatMyStructImplements)". In this case U is known-to-be-reference because any
    // value types passed to the function will have been implicitly boxed, but T isn't known-to-be-reference
    // because it might be given values. This is why "Ref" constraints don't carry through. Note that VB
    // uses the keyword "Class" instead of "Ref", which is confusing, because it covers interfaces too.
    //
    // Confusing case: given "Sub g(Of T as {U,Ref}, U)()" then we as humans can infer that U will inevitably
    // be a reference type, because T is a reference type and is constrained by U. The compiler sometimes
    // makes that inference in special cases (e.g. a conversion T()->U() is allowed in this case), but
    // it doesn't use that inference in "IsReferenceType()". In this case it will say that !U->IsReferenceType().
    // That's why the name "IsExplicitlyConstrainedAsReference" is more accurate.
    //
    // See also the comments for ConversionResolution.cpp::ClassifyPredefinedCLRConversion.
    // The correctness of that function depends crucially on which types are counted as reference types.
    // If you make any additions or changes here, then the conversion code will have to change as well.
    // If you have doubts, please talk with Microsoft (31/May/2008)

    Vtypes TypeCode = T->GetVtype();

#pragma prefast(suppress: 26010 26011, "Out of bounds access only if symbol is trashed")
    return TypeIsRepresentedAsObjectReference[TypeCode] ||
        (TypeCode == t_generic && T->PGenericParam()->IsReferenceType());
}



// IsOrInheritsFrom and Implements do not perform the cycles detection that
// the symbol table methods do. If there are cycles in inheritance or
// implementation graphs the classes should be marked bad and semantic analysis
// should never see them.
//
// * When "Derived" is a class, this function says whether it inherits (directly
//   or indirectly) from some other class "Base". It does not include "implements base".
// * When "Derived" is an interface, this function says whether it inherits (directly
//   or indirectly) from some other interface "Base".
// * When "Derived" is a generic parameter, this function says whether
//   * for "Base" a generic parameter, whether Derived --constrained-->* Base (for zero or more "constrained" steps)
//   * for "Base" an interface, whether Derived --constrained-->* interface --inherits-->* Base
//     (with zero or more "constrained" and "inherits" steps)
//   * for "Base" c class, whether Derived --constrained-->* class --inherits-->* Base
//     (with zero or more "constrained" and "inherits" steps)
//
// So strictly speaking we should rename it "IsOrInheritsAfterConstraints"
//
bool TypeHelpers::IsOrInheritsFrom(
    Type *Derived,
    Type *Base,
    _In_ Symbols &SymbolCreator,
    bool ChaseThroughGenericBindings,
    _Inout_opt_ DynamicArray<GenericTypeBinding *> *MatchingBaseGenericBindings,
    // Consider types as equivalent based on their project equivalence.
    bool IgnoreProjectEquivalence,
    // The project containing a component of type Derived that was considered
    // for a successful match ignoring project equivalence.
    _Out_opt_ CompilerProject **ProjectForDerived,
    // The project containing a component of type Base that was considered
    // for a successful match ignoring project equivalence.
    _Out_opt_ CompilerProject **ProjectForBase
)
{
    VSASSERT(!MatchingBaseGenericBindings || !ChaseThroughGenericBindings, "inconsistent options!!!");

    if (MatchingBaseGenericBindings &&
        (!Base->IsGeneric() || ChaseThroughGenericBindings))
    {
        // If not generic, then there will be no generic bindings to collect for the matching bases.
        // MatchingBaseGenericBindings only makes sense for generic bases. So setting this to NULL
        // for non-generic bases is advantageous to the rest of the process short circuiting even if
        // MatchingBaseGenericBindings are requested to be collected. This also avoids checks later
        // on to verify that the matching entity when trying to collect bindings is indeed a generic
        // binding.
        //
        MatchingBaseGenericBindings = NULL;
    }

    if (Derived->IsGenericParam())
    {
        if (EquivalentTypes(
                Derived,
                Base,
                IgnoreProjectEquivalence,
                ProjectForDerived,
                ProjectForBase))
        {
            VSASSERT(MatchingBaseGenericBindings == NULL, "Why are bindings required for non-generic types ?");
            return true;
        }

        // Return true if any constraint on the parameter has an appropriate relation to the base type.

        if (Derived->PGenericParam()->HasValueConstraint() &&
            (Base->IsObject() || Base->IsCOMValueType()))
        {
            VSASSERT(MatchingBaseGenericBindings == NULL, "Why are bindings required for non-generic types ?");
            return true;
        }

        bool Result = false;

        for (GenericTypeConstraint *Constraint = Derived->PGenericParam()->GetTypeConstraints();
             Constraint;
             Constraint = Constraint->Next())
        {
            if (Constraint->IsBadConstraint())
            {
                continue;
            }

            if (IsOrInheritsFrom(
                    Constraint->GetType(),
                    Base,
                    SymbolCreator,
                    ChaseThroughGenericBindings,
                    MatchingBaseGenericBindings,
                    IgnoreProjectEquivalence,
                    ProjectForDerived,
                    ProjectForBase))
            {
                Result = true;

                // If not requested to collect all the bindings of a generic base that show up, then
                // we can just return the boolean Result.
                //
                if (MatchingBaseGenericBindings == NULL)
                {
                    break;
                }
            }
        }

        return Result;
    }

    if (Base->IsGenericTypeBinding())
    {
        if (ChaseThroughGenericBindings)
        {
            Base = Base->PGenericTypeBinding()->GetGeneric();
        }
    }
    else if (MatchingBaseGenericBindings == NULL)
    {
        // Bindings between the derived type and the base are irrelevant if base is not a generic binding
        // or if bindings for the base are not required to be collected.
        //
        ChaseThroughGenericBindings = true;
    }

    if (ChaseThroughGenericBindings && Derived->IsGenericTypeBinding())
    {
        Derived = Derived->PGenericTypeBinding()->GetGeneric();
    }

    if (Type1EquivalentToType2OrItsRawGeneric(
            Base,
            Derived,
            IgnoreProjectEquivalence,
            ProjectForBase,
            ProjectForDerived))
    {
        if (MatchingBaseGenericBindings)
        {
            VSASSERT(Derived->IsGenericTypeBinding(), "Generic bindings expected when collecting bindings!!!");

            GenericTypeBinding *Binding = Derived->PGenericTypeBinding();
            MatchingBaseGenericBindings->AddElement(Binding);
        }

        return true;
    }

    if (!IsClassOrRecordType(Derived) || !IsClassOrRecordType(Base))
    {
        if (!Derived->IsInterface() || !Base->IsInterface())
        {
            return false;
        }
        else
        {
            // Both types are interfaces.

            bool Result = false;

            for (BCSYM_Implements *DerivedBase = Derived->GetFirstImplements();
                DerivedBase;
                DerivedBase = DerivedBase->GetNext())
            {
                if (!DerivedBase->IsBadImplements() &&
                    !DerivedBase->GetCompilerRoot()->IsBad())
                {
                    Type *DerivedBaseInterface = DerivedBase->GetCompilerRoot();

                    if (ChaseThroughGenericBindings && DerivedBaseInterface->IsGenericTypeBinding())
                    {
                        DerivedBaseInterface = DerivedBaseInterface->PGenericTypeBinding()->GetGeneric();
                    }

                    if (Derived->IsGenericTypeBinding() && RefersToGenericParameter(DerivedBaseInterface))
                    {
                        if (IsOrInheritsFrom(
                                ReplaceGenericParametersWithArguments(
                                    DerivedBaseInterface,
                                    Derived->PGenericTypeBinding(),
                                    SymbolCreator),
                                Base,
                                SymbolCreator,
                                ChaseThroughGenericBindings,
                                MatchingBaseGenericBindings,
                                IgnoreProjectEquivalence,
                                ProjectForDerived,
                                ProjectForBase))
                        {
                            Result = true;

                            // If not requested to collect all the bindings of a generic base that show up, then
                            // we can just return the boolean Result.
                            //
                            if (MatchingBaseGenericBindings == NULL)
                            {
                                break;
                            }
                        }
                    }

                    else if (IsOrInheritsFrom(
                                DerivedBaseInterface,
                                Base,
                                SymbolCreator,
                                ChaseThroughGenericBindings,
                                MatchingBaseGenericBindings,
                                IgnoreProjectEquivalence,
                                ProjectForDerived,
                                ProjectForBase))
                    {
                        Result = true;

                        // If not requested to collect all the bindings of a generic base that show up, then
                        // we can just return the boolean Result.
                        //
                        if (MatchingBaseGenericBindings == NULL)
                        {
                            break;
                        }
                    }
                }
            }

            return Result;
        }
    }

    else
    {
        // Both types are classes.

        Derived = GetBaseClass(Derived, SymbolCreator);

        while (Derived)
        {
            if (ChaseThroughGenericBindings && Derived->IsGenericTypeBinding())
            {
                Derived = Derived->PGenericTypeBinding()->GetGeneric();
            }

            if (Type1EquivalentToType2OrItsRawGeneric(
                    Base,
                    Derived,
                    IgnoreProjectEquivalence,
                    ProjectForBase,
                    ProjectForDerived))

            {
                if (MatchingBaseGenericBindings)
                {
                    VSASSERT(Derived->IsGenericTypeBinding(), "Generic bindings expected when collecting bindings!!!");

                    GenericTypeBinding *Binding = Derived->PGenericTypeBinding();
                    MatchingBaseGenericBindings->AddElement(Binding);

                    // Note that we don't to continue on looking because having multiple
                    // bindings of the same base is impossible for classes. This is
                    // because classes don't have multiple inheritance, and in general
                    // circular inheritance is not allowed.
                }

                return true;
            }

            Derived = GetBaseClass(Derived, SymbolCreator);
        }

        return false;
    }

}

bool TypeHelpers::Type1EquivalentToType2OrItsRawGeneric
(
    Type *Type1,
    Type *Type2,
    // Consider types as equivalent based on their project equivalence.
    bool IgnoreProjectEquivalence,
    // The project containing a component of type Type1 that was considered
    // for a successful match ignoring project equivalence.
    _Out_opt_ CompilerProject **ProjectForType1,
    // The project containing a component of type Type2 that was considered
    // for a successful match ignoring project equivalence.
    _Out_opt_ CompilerProject **ProjectForType2
)
{
    if (Type2->IsGenericTypeBinding() && !Type1->IsGenericTypeBinding())
    {
        // If Type1 is not a binding, it implies that raw comparison
        // needs to be done, so dig through to the raw generic of
        // Type2.
        //
        Type2 = Type2->PGenericTypeBinding()->GetGeneric();
    }

    return
        EquivalentTypes(
            Type1,
            Type2,
            IgnoreProjectEquivalence,
            ProjectForType1,
            ProjectForType2);
}

bool TypeHelpers::Implements
(
    Type *Implementor,
    Type *Interface,
    _In_ Symbols &SymbolCreator,
    bool ChaseThroughGenericBindings,
    _Inout_opt_ DynamicArray<GenericTypeBinding *> *MatchingBaseGenericBindings,
    CompilerHost *CompilerHost,
    // Consider types as equivalent based on their project equivalence.
    bool IgnoreProjectEquivalence,
    // The project containing a component of type "Implementor" that was considered
    // for a successful match ignoring project equivalence.
    _Out_opt_ CompilerProject **ProjectForImplementor,
    // The project containing a component of type "Interface" that was considered
    // for a successful match ignoring project equivalence.
    _Out_opt_ CompilerProject **ProjectForInterface
)
{
    VSASSERT(Interface->IsInterface(), "Cannot ask if a non-interface type is implemented.");

    VSASSERT(!MatchingBaseGenericBindings || !ChaseThroughGenericBindings, "inconsistent options!!!");

    if (MatchingBaseGenericBindings &&
        (!Interface->IsGeneric() || ChaseThroughGenericBindings))
    {
        // If not generic, then there will be no generic bindings to collect for the matching bases.
        //
        MatchingBaseGenericBindings = NULL;
    }

    if (Implementor->IsGenericParam())
    {
        // Return true if any constraint on the parameter has an appropriate relation to the Interface type.

        bool Result = false;

        // If a "Structure" constraint is present, then treat as through there is a System.ValueType
        // constraint
        //
        if (Implementor->PGenericParam()->HasValueConstraint() &&
            Implements(
                CompilerHost->GetFXSymbolProvider()->GetType(FX::ValueTypeType),
                Interface,
                SymbolCreator,
                ChaseThroughGenericBindings,
                MatchingBaseGenericBindings,
                CompilerHost,
                IgnoreProjectEquivalence,
                ProjectForImplementor,
                ProjectForInterface))
        {
            Result = true;

            // If not requested to collect all the bindings of a generic base that show up, then
            // we can just return the boolean Result.
            //
            if (MatchingBaseGenericBindings == NULL)
            {
                return Result;
            }
        }

        for (GenericTypeConstraint *Constraint = Implementor->PGenericParam()->GetTypeConstraints();
             Constraint;
             Constraint = Constraint->Next())
        {
            if (Constraint->IsBadConstraint())
            {
                continue;
            }

            if (Implements(
                    Constraint->GetType(),
                    Interface,
                    SymbolCreator,
                    ChaseThroughGenericBindings,
                    MatchingBaseGenericBindings,
                    CompilerHost,
                    IgnoreProjectEquivalence,
                    ProjectForImplementor,
                    ProjectForInterface))
            {
                Result = true;

                // If not requested to collect all the bindings of a generic base that show up, then
                // we can just return the boolean Result.
                //
                if (MatchingBaseGenericBindings == NULL)
                {
                    return Result;
                }
            }
        }

        return Result;
    }

    if (Interface->IsGenericTypeBinding())
    {
        if (ChaseThroughGenericBindings)
        {
            Interface = Interface->PGenericTypeBinding()->GetGeneric();
        }
    }
    else if (MatchingBaseGenericBindings == NULL)
    {
        // Bindings in the implementing type are irrelevant if the interface is not a generic binding or
        // if bindings of the implemented interface are not required to be collected.
        //
        ChaseThroughGenericBindings = true;
    }

    bool Result = false;

    while (Implementor)
    {
        if (ChaseThroughGenericBindings && Implementor->IsGenericTypeBinding())
        {
            Implementor = Implementor->PGenericTypeBinding()->GetGeneric();
        }

        BCITER_ImplInterfaces ImplInterfacesIter(
                                Implementor->IsClass() || Implementor->IsInterface() ?
                                    Implementor->PContainer() :
                                    NULL);

        while (BCSYM_Implements *Implemented = ImplInterfacesIter.GetNext())
        {
            if (!Implemented->IsBadImplements() &&
                !Implemented->IsRedundantImplements() &&
                !Implemented->GetCompilerRoot()->IsBad())
            {
                Type *ImplementedInterface = Implemented->GetCompilerRoot();

                if (ChaseThroughGenericBindings && ImplementedInterface->IsGenericTypeBinding())
                {
                    ImplementedInterface = ImplementedInterface->PGenericTypeBinding()->GetGeneric();
                }

                if (Implementor->IsGenericTypeBinding())
                {
                    ImplementedInterface = ReplaceGenericParametersWithArguments(
                        ImplementedInterface,
                        Implementor->PGenericTypeBinding(),
                        SymbolCreator);
                }

                if (IsOrInheritsFrom(
                        ImplementedInterface,
                        Interface,
                        SymbolCreator,
                        ChaseThroughGenericBindings,
                        MatchingBaseGenericBindings,
                        IgnoreProjectEquivalence,
                        ProjectForImplementor,
                        ProjectForInterface))
                {
                    Result = true;

                    // If not requested to collect all the bindings of a generic base that show up, then
                    // we can just return the boolean Result.
                    //
                    if (MatchingBaseGenericBindings == NULL)
                    {
                        return Result;
                    }
                }
            }
        }

        Implementor = GetBaseClass(Implementor, SymbolCreator);
    }

    return Result;
}

bool TypeHelpers::IsOrInheritsFromOrImplements
(
    Type *Derived,
    Type *Base,
    _In_ Symbols &SymbolCreator,
    bool ChaseThroughGenericBindings,
    _Inout_opt_ DynamicArray<GenericTypeBinding *> *MatchingBaseGenericBindings,
    CompilerHost *CompilerHost,
    // Consider types as equivalent based on their project equivalence.
    bool IgnoreProjectEquivalence,
    // The project containing a component of type Derived that was considered
    // for a successful match ignoring project equivalence.
    _Out_opt_ CompilerProject **ProjectForDerived,
    // The project containing a component of type Base that was considered
    // for a successful match ignoring project equivalence.
    _Out_opt_ CompilerProject **ProjectForBase
)
{
    VSASSERT(!MatchingBaseGenericBindings || !ChaseThroughGenericBindings, "inconsistent options!!!");

    if (MatchingBaseGenericBindings &&
        (!Base->IsGeneric() || ChaseThroughGenericBindings))
    {
        // If not generic, then there will be no generic bindings to collect for the matching bases.
        //
        MatchingBaseGenericBindings = NULL;
    }

    if (Derived->IsGenericParam())
    {
        if (EquivalentTypes(Base, Derived))
        {
            VSASSERT(MatchingBaseGenericBindings == NULL, "Why are bindings required for non-generic types ?");
            return true;
        }

        // Return true if any constraint on the parameter has an appropriate relation to the base type.

        bool Result = false;

        // If a "Structure" constraint is present, then treat as through there is a System.ValueType
        // constraint
        //
        if (Derived->PGenericParam()->HasValueConstraint() &&
            IsOrInheritsFromOrImplements(
                CompilerHost->GetFXSymbolProvider()->GetType(FX::ValueTypeType),
                Base,
                SymbolCreator,
                ChaseThroughGenericBindings,
                MatchingBaseGenericBindings,
                CompilerHost,
                IgnoreProjectEquivalence,
                ProjectForDerived,
                ProjectForBase))
        {
            Result = true;

            // If not requested to collect all the bindings of a generic base that show up, then
            // we can just return the boolean Result.
            //
            if (MatchingBaseGenericBindings == NULL)
            {
                return Result;
            }
        }

        for (GenericTypeConstraint *Constraint = Derived->PGenericParam()->GetTypeConstraints();
             Constraint;
             Constraint = Constraint->Next())
        {
            if (Constraint->IsBadConstraint())
            {
                continue;
            }

            if (IsOrInheritsFromOrImplements(
                    Constraint->GetType(),
                    Base,
                    SymbolCreator,
                    ChaseThroughGenericBindings,
                    MatchingBaseGenericBindings,
                    CompilerHost,
                    IgnoreProjectEquivalence,
                    ProjectForDerived,
                    ProjectForBase))
            {
                Result = true;

                // If not requested to collect all the bindings of a generic base that show up, then
                // we can just return the boolean Result.
                //
                if (MatchingBaseGenericBindings == NULL)
                {
                    return Result;
                }
            }
        }

        return Result;
    }

    if (IsClassOrRecordType(Derived))
    {

        return
            IsClassOrRecordType(Base) ?
                IsOrInheritsFrom(
                    Derived,
                    Base,
                    SymbolCreator,
                    ChaseThroughGenericBindings,
                    MatchingBaseGenericBindings,
                    IgnoreProjectEquivalence,
                    ProjectForDerived,
                    ProjectForBase) :
                Base->IsInterface() &&
                    Implements(
                        Derived,
                        Base,
                        SymbolCreator,
                        ChaseThroughGenericBindings,
                        MatchingBaseGenericBindings,
                        CompilerHost,
                        IgnoreProjectEquivalence,
                        ProjectForDerived,
                        ProjectForBase);
    }

    if (Derived->IsInterface() && Base->IsInterface())
    {
        return
            IsOrInheritsFrom(
                Derived,
                Base,
                SymbolCreator,
                ChaseThroughGenericBindings,
                MatchingBaseGenericBindings,
                IgnoreProjectEquivalence,
                ProjectForDerived,
                ProjectForBase);
    }

    return false;
}

Type * TypeHelpers::GetBaseClass(
    Type * Derived,
    _In_ Symbols &SymbolCreator)
{
    VSASSERT(IsClassOrRecordType(Derived), "Cannot get the base class of a non-class type.");

    Type *Base = Derived->PClass()->GetBaseClass();

    if (Base && Derived->IsGenericTypeBinding())
    {
        Base = ReplaceGenericParametersWithArguments(Base, Derived->PGenericTypeBinding(), SymbolCreator);
    }

    if (Base && IsBadType(Base))
    {
        return NULL;
    }

    return Base;
}

bool TypeHelpers::EquivalentTypes
(
    Type *T1,
    Type *T2,
    // Consider types as equivalent based on their project equivalence.
    bool IgnoreProjectEquivalence,
    // The project containing a component of type T1 that was considered
    // for a successful match ignoring project equivalence.
    _Out_opt_ CompilerProject **ProjectForT1,
    // The project containing a component of type T2 that was considered
    // for a successful match ignoring project equivalence.
    _Out_opt_ CompilerProject **ProjectForT2
    IDE_ARG(bool ConsiderOpenGenericParamsEqual)
)
{
    // Microsoft, 17 June 2008
    //
    // BCSYM::AreTypesEqual avoids the diamond-reference-problem by checking, in the case
    // where both T1 and T2 come from metadata, by checking that they have the same fully-qualified
    // names. Meanwhile, TypeHelpers::EquivalentTypes avoids it by checking their fully-qualified
    // names (regardless of whether they come from metadata) if you pass the IgnoreProjectEquivalence=true
    // flag. (The parameter's name was badly chosen. It's actually ignoring type-pointer-inequivalence
    // that has been caused by project-inequivalence.)
    //
    // Also, as demonstrated by the msvb7\vbintellisense test, BCSYM::AreTypesEqual lets two
    // anonymous delegates to be considered equal if they have the same bindings and if we're compiled
    // under "#if IDE" (and similarly for anonymous types); TypeHelpers::EquivalentTypes doesn't have
    // that feature.
    //
    // Suggestion: we should investigate merging these two functions. It's likely a bug that
    // AreTypesEqual has the special "anonymous" functionality but EquivalentTypes doesn't.


    if (BCSYM::AreTypesEqual(T1, T2, false /* Don't dig into arrays and generic bindings. We will dig later. */))
    {
        return true;
    }

    Vtypes TypeCode1 = T1->GetVtype();
    Vtypes TypeCode2 = T2->GetVtype();

    if (TypeCode1 == TypeCode2)
    {
        // An enum type has the type code of its underlying type,
        // and so needs to be filtered here.
        if (T1->IsEnum() || T2->IsEnum())
        {
            // references to enums nested in generic types will be generic bindings
            //
            if (T1->IsGenericTypeBinding() && T2->IsGenericTypeBinding())
            {
                GenericTypeBinding *Binding1 = T1->PGenericTypeBinding();
                GenericTypeBinding *Binding2 = T2->PGenericTypeBinding();

                bool ret =
                    EquivalentTypeBindings(
                        Binding1,
                        Binding2,
                        IgnoreProjectEquivalence,
                        ProjectForT1,
                        ProjectForT2
                        IDE_ARG(ConsiderOpenGenericParamsEqual));

                VPASSERT(IgnoreProjectEquivalence || IDE_CODE(true ||) ret == BCSYM::AreTypesEqual(T1,T2),"EquivalentTypes/AreTypesEqual mismatch for generic enums; please tell Microsoft");
                return ret;
            }
        }
        else
        {
            switch (TypeCode1)
            {
                case t_array:
                {
                    ArrayType *Array1 = T1->PArrayType();
                    ArrayType *Array2 = T2->PArrayType();

                    if (Array1->GetRank() != Array2->GetRank())
                    {
                        goto Mismatch;
                    }

                    // Check element types.


                    bool ret=
                        EquivalentTypes(
                            GetElementType(Array1),
                            GetElementType(Array2),
                            IgnoreProjectEquivalence,
                            ProjectForT1,
                            ProjectForT2
                            IDE_ARG(ConsiderOpenGenericParamsEqual));

                    VPASSERT(IgnoreProjectEquivalence || IDE_CODE(true ||) ret == BCSYM::AreTypesEqual(T1,T2), "EquivalentTypes/AreTypesEqual mismatch for arrays; please tell Microsoft");
                    return ret;
                }

                case t_generic:

                    goto Mismatch;

                case t_struct:
                case t_ref:

                    if (T1->IsGenericTypeBinding() && T2->IsGenericTypeBinding())
                    {
                        GenericTypeBinding *Binding1 = T1->PGenericTypeBinding();
                        GenericTypeBinding *Binding2 = T2->PGenericTypeBinding();

                        bool ret =
                            EquivalentTypeBindings(
                                Binding1,
                                Binding2,
                                IgnoreProjectEquivalence,
                                ProjectForT1,
                                ProjectForT2
                                IDE_ARG(ConsiderOpenGenericParamsEqual));

                        VPASSERT(IgnoreProjectEquivalence || IDE_CODE(true ||) ret == BCSYM::AreTypesEqual(T1,T2), "EquivalentTypes/AreTypesEqual mismatch for ref/struct generic bindings: please tell Microsoft");
                        return ret;
                    }

                    goto Mismatch;

                case t_ptr:
                    VSFAIL("Unexpected t_ptr in EquivalentTypes: please tell Microsoft");
            }

            VPASSERT(true == BCSYM::AreTypesEqual(T1,T2), "EquivalentTypes/AreTypesEqual mismatch for equal typecodes; please tell Microsoft");
            return true;
        }
    }

Mismatch:

    if (IgnoreProjectEquivalence &&
        (T1->IsContainer() && !T1->IsGenericTypeBinding()) &&
        (T2->IsContainer() && !T2->IsGenericTypeBinding()) &&
        T1->GetKind() == T2->GetKind())
    {
        bool ret =
            ArePossiblyEquivalentIgnoringProjectEquivalence(
                T1->PContainer(),
                T2->PContainer(),
                ProjectForT1,
                ProjectForT2);

        VPASSERT(IDE_CODE(true ||) ret == BCSYM::AreTypesEqual(T1,T2), "EquivalentTypes/AreTypesEqual mismatch for mismatch; please tell Microsoft");
        return ret;
    }

    VPASSERT(IDE_CODE(true ||) false == BCSYM::AreTypesEqual(T1,T2), "EquivalentTypes/AreTypesEqual mismatch for endcase; please tell Microsoft");
    return false;
}

bool TypeHelpers::EquivalentTypeBindings
(
    GenericTypeBinding *Binding1,
    GenericTypeBinding *Binding2,
    // Consider types as equivalent based on their project equivalence.
    bool IgnoreProjectEquivalence,
    // The project containing a component of type Binding1 that was considered
    // for a successful match ignoring project equivalence.
    _Out_opt_ CompilerProject **ProjectForBinding1,
    // The project containing a component of type Binding2 that was considered
    // for a successful match ignoring project equivalence.
    _Out_opt_ CompilerProject **ProjectForBinding2
    IDE_ARG(bool ConsiderOpenGenericParamsEqual)
)
{
    if (Binding1 == Binding2)
    {
        return true;
    }

    if (!Binding1 || !Binding2)
    {
        return false;
    }

    if (!EquivalentTypes(
            Binding1->GetGenericType(),
            Binding2->GetGenericType(),
            IgnoreProjectEquivalence,
            ProjectForBinding1,
            ProjectForBinding2
            IDE_ARG(ConsiderOpenGenericParamsEqual)))
    {
        return false;
    }

    while (Binding1 && Binding2)
    {
        if (Binding1->GetArgumentCount() != Binding2->GetArgumentCount())
        {
            // This condition is required because it could potentially result when ignoring project
            // equivalence, otherwise it seems improbable that this condition could ever fail.
            return false;
        }

        for (unsigned ArgumentIndex = 0; ArgumentIndex < Binding1->GetArgumentCount(); ArgumentIndex++)
        {
            BCSYM * Argument1 = Binding1->GetArgument(ArgumentIndex);
            BCSYM * Argument2 = Binding2->GetArgument(ArgumentIndex);

#if IDE 
            if (ConsiderOpenGenericParamsEqual &&
                Argument1 && Argument1->IsGenericParam() &&
                Argument2 && Argument2->IsGenericParam())
            {
                continue;
            }
#endif

            if (!EquivalentTypes(
                    Argument1,
                    Argument2,
                    IgnoreProjectEquivalence,
                    ProjectForBinding1,
                    ProjectForBinding2
                    IDE_ARG(ConsiderOpenGenericParamsEqual)))
            {
                return false;
            }
        }

        Binding1 = Binding1->GetParentBinding();
        Binding2 = Binding2->GetParentBinding();
    }

    return Binding1 == NULL && Binding2 == NULL;
}


bool TypeHelpers::EquivalentTypeBindings(_In_ DynamicArray<GenericTypeBinding *> &Bindings)
{
    // Note that the same binding might occur multiple times, it the same
    // binding is reimplemented by different classes in the hierarchy or
    // inherited multiple times by different interfaces in the hierarchy.
    // So the "> 1" check is not enough to check for duplicates. Instead
    // we need to do type comparisons.

    if (Bindings.Count() <= 1)
    {
        return true;
    }

    GenericTypeBinding *FirstBinding = Bindings.Element(0);

    VSASSERT(FirstBinding, "NULL binding unexpected!!!");

    for(unsigned Index = 1; Index < Bindings.Count(); Index++)
    {
        GenericTypeBinding *Binding = Bindings.Element(Index);

        VSASSERT(Binding, "NULL binding unexpected!!!");

        if (!EquivalentTypes(FirstBinding, Binding))
        {
            return false;
        }
    }

    return true;
}

bool TypeHelpers::ArePossiblyEquivalentIgnoringProjectEquivalence(
    BCSYM_Container * Container1,
    BCSYM_Container * Container2,
    _Out_opt_ CompilerProject * * ProjectForContainer1,
    _Out_opt_ CompilerProject * * ProjectForContainer2)
{
    AssertIfNull(Container1);
    AssertIfNull(Container2);

    AssertIfNull(ProjectForContainer1);
    AssertIfNull(ProjectForContainer2);

    // Do not initialize ProjectForContainer1 and ProjectForContainer2 to NULL.
    // Only set if match. Callers depend on this behavior.

    // 



    BCSYM_Container *Current1 = Container1;
    BCSYM_Container *Current2 = Container2;

    while (Current1 && Current2)
    {
        if (Current1->GetKind() != Current2->GetKind())
        {
            return false;
        }

        if (Current1->IsNamespace())
        {
            if (!StringPool::IsEqual(Current1->GetQualifiedName(), Current2->GetQualifiedName()))
            {
                return false;
            }

            // Match
            Current1 = NULL;
            Current2 = NULL;
            break;
        }

        if (!StringPool::IsEqual(Current1->GetName(), Current2->GetName()) ||
            Current1->GetGenericParamCount() != Current2->GetGenericParamCount())
        {
            return false;
        }

        Current1 = Current1->GetContainer();
        Current2 = Current2->GetContainer();
    }

    // Mismatch
    if (Current1 || Current2)
    {
        return false;
    }

    CompilerFile *File1 = Container1->GetCompilerFile();
    CompilerFile *File2 = Container2->GetCompilerFile();

    if (!File1 || !File2)
    {
        return false;
    }

    CompilerProject *Project1 = File1->GetCompilerProject();
    CompilerProject *Project2 = File2->GetCompilerProject();

    if (!Project1 || !Project2)
    {
        return false;
    }

    if (!AreAssembliesEqualByName(Project1, Project2))
    {
        return false;
    }

    if (ProjectForContainer1)
    {
        *ProjectForContainer1 = Project1;
    }

    if (ProjectForContainer2)
    {
        *ProjectForContainer2 = Project2;
    }

    return true;
}

// Returns true if type T is/inherits from/implements IEnumerable(Of U), and U is/inherits from/implements TArg

bool TypeHelpers::IsCompatibleWithGenericEnumerableType(
    Type * T,
    Type * TArg,
    _In_ Symbols &SymbolCreator,
    CompilerHost * Host)
{
    if (Host->GetFXSymbolProvider()->IsTypeAvailable(FX::GenericIEnumerableType))
    {
        DynamicArray<GenericTypeBinding *> GenericIEnumerableBindings;

        // Find all bindings of IEnumerable(Of ...) which are compatible with type T,
        // and store them in GenericIEnumerableBindings

        if (IsOrInheritsFromOrImplements(
                T,
                Host->GetFXSymbolProvider()->GetType(FX::GenericIEnumerableType),
                SymbolCreator,
                false,
                &GenericIEnumerableBindings,
                Host))
        {
            // Iterate over each binding and determine whether TArg is compatible with
            // the bound argument of IEnumerable(Of TEnumArg)

            for (unsigned i = 0; i < GenericIEnumerableBindings.Count(); i++)
            {
                Type *TEnumArg = GenericIEnumerableBindings.Element(i)->PGenericTypeBinding()->GetArgument(0);

                if (IsOrInheritsFromOrImplements(TEnumArg, TArg, Host))
                {
                    return true;
                }
            }
        }
    }

    // The above code does not handle array of T, i.e. array of T does not get caught by
    // IE(of T) so check it explicitly.

    if (T->IsArrayType() &&
        T->PArrayType()->GetRank() == 1 &&
        IsOrInheritsFromOrImplements(GetElementType(T->PArrayType()), TArg, Host))
        return true;

    return false;
}

bool TypeHelpers::IsNullableType(Type * T)
{
    if (T && T->IsGenericTypeBinding())
    {
        CompilerProject *Project = T->PGenericTypeBinding()->GetContainingProject();
        AssertIfNull(Project);
        if (Project)
        {
            return IsNullableType(T, Project->GetCompilerHost());
        }
    }
    return false;
}

bool TypeHelpers::IsNullableTypeWithContantainerContext(
    Type * T,
    BCSYM_Container * ContainerContext)
{
    CompilerHost *pCompilerHost = NULL;

    if (!T || !T->IsGenericTypeBinding())
    {
        return false;
    }

    // Context is not allways passed in, for instance in debugger.
    if (ContainerContext)
    {
        SourceFile *pFile = ContainerContext->GetSourceFile();
        // Won't have a file yet 
        if (pFile)
        {
            pCompilerHost  = pFile->GetCompilerHost();
        }
    }
    if (!pCompilerHost)
    {
        // Project is null in the debugger.
        CompilerProject *pProject = T->PGenericTypeBinding()->GetContainingProject();
        if (pProject)
        {
            pCompilerHost = pProject->GetCompilerHost();
        }
    }

    // This method handles null for pCompilerHost.
    return IsNullableType(T, pCompilerHost);
}

/******************************************************************************
;IsEmbeddableInteropType

Is the parameter a type to which No-PIA linking behavior applies?
This function accepts any symbol, though there's no reason to use it unless you 
think the symbol is a type. It will return true if the parameter is a type which 
comes from a project which was imported using the EmbedInteropTypes option, aka 
the command line /link switch. 
This function does NOT attempt to verify that the type has the attributes 
necessary for successful linking: it merely identifies the type of reference 
used to import its project. This is all we need in order to determine whether to 
attempt the special No-PIA linking behavior. If the type is badly formed, 
PEBuilder should produce an appropriate error when attempting to emit the 
embedded local type.
******************************************************************************/
bool TypeHelpers::IsEmbeddableInteropType
(
 _In_ Type *T
 )
{
    if (!T || !T->IsNamedRoot() || !T->IsType())
    {
        return false;
    }
    CompilerProject *pContainerProject = T->PNamedRoot()->GetContainingProject();
    if (!pContainerProject)
    {
        return false;
    }
    // Locate the project being compiled.
    // Find the reference to the type's container's project.
    // Determine whether the reference uses the NoPIA flag.
    Compiler *pCompiler = T->PNamedRoot()->GetCompiler();
    AssertIfNull(pCompiler);
    CompilerProject *pBuildProject = pCompiler->GetProjectBeingCompiled();
    // We will always have a project when building, but some IDE intellisense
    // type situations don't have a project. In this case it really doesn't matter
    // whether we offer NoPIA behavior, so we'll just bail out.
    if (!pBuildProject)
    {
        return false;
    }
    // Find out whether the build project has an embedded reference to the container.
    return pBuildProject->HasEmbeddedReferenceTo(pContainerProject);
}

/******************************************************************************
;IsEmbeddedLocalType

Is the parameter a type to which unification applies?
This function accepts any symbol, though there's no reason to use it unless you 
think the symbol might be a type. It will return true if the symbol is an imported 
local type, meaning a placeholder which needs to be unified back to some canonical 
version of the same type. Expected usage of this function is narrowly limited to 
code involved in the type unification system: specifically, the implementation of 
BCSYM_NamedType, and the name overload resolution in NameSemantics.

A type qualifies as a unifiable local type if it meets these criteria:
- It must have a TypeIdentifierAttribute. This is what identifies it as a local 
  type at all, and may contain the name and scope of the canonical type.
- If it is an interface, it must have a ComImportAttribute.
- It must not be located in the current project, that is, the one being compiled. 
  The unification behavior applies only to types which have been imported from 
  some other assembly.

Dev10 #748923	
A type temporarily marked as embedded for the purpose of providing identity was erroneously 
dismissed as a non candidate for a canonical type. These types are tricky - in context of one 
project they should be considered as embedded, but in context of declaring project they shouldn't be. 
When we are doing name lookup, the types are coming from declaring projects, 
the fIgnoreTemporaryEmbeddedStatus flag is added to enforce this semantics in name lookup. 
******************************************************************************/
bool TypeHelpers::IsEmbeddedLocalType
(
 _In_ Type *T
#if IDE         
, bool fIgnoreTemporaryEmbeddedStatus /*= false*/
#endif
)
{
    // !!! THIS FUNCTION SHOULD BE VERY VERY FAST !!!

    // Is the parameter a type to which unification applies?
    // It must be a ComImport type with a TypeIdentifier attribute, and it must not
    // be located in the current project - unification applies only to types referenced from
    // other projects.
    if (!T || 
        T->IsGenericBinding() ||  // We do not embed generic bindings, let's filter them out early
        !T->IsContainer() )
    {
        return false;
    }

    return IsEmbeddedLocalType(
        T->PContainer()
#if IDE         
        , fIgnoreTemporaryEmbeddedStatus
#endif
        );
}



/******************************************************************************
;IsEmbeddedLocalType

Is the parameter a type to which unification applies?
This function accepts any symbol, though there's no reason to use it unless you 
think the symbol might be a type. It will return true if the symbol is an imported 
local type, meaning a placeholder which needs to be unified back to some canonical 
version of the same type. Expected usage of this function is narrowly limited to 
code involved in the type unification system: specifically, the implementation of 
BCSYM_NamedType, and the name overload resolution in NameSemantics.

A type qualifies as a unifiable local type if it meets these criteria:
- It must have a TypeIdentifierAttribute. This is what identifies it as a local 
  type at all, and may contain the name and scope of the canonical type.
- If it is an interface, it must have a ComImportAttribute.
- It must not be located in the current project, that is, the one being compiled. 
  The unification behavior applies only to types which have been imported from 
  some other assembly.
  
Dev10 #748923	
A type temporarily marked as embedded for the purpose of providing identity was erroneously 
dismissed as a non candidate for a canonical type. These types are tricky - in context of one 
project they should be considered as embedded, but in context of declaring project they shouldn't be. 
When we are doing name lookup, the types are coming from declaring projects, 
the fIgnoreTemporaryEmbeddedStatus flag is added to enforce this semantics in name lookup. 
******************************************************************************/
bool TypeHelpers::IsEmbeddedLocalType
(
 _In_ BCSYM_Container* pContainer
#if IDE         
, bool fIgnoreTemporaryEmbeddedStatus /*= false*/
#endif
)
{
    // !!! THIS FUNCTION SHOULD BE VERY VERY FAST !!!

    // Is the parameter a type to which unification applies?
    // It must be a ComImport type with a TypeIdentifier attribute, and it must not
    // be located in the current project - unification applies only to types referenced from
    // other projects.

    // This check should be equivalent to the one performed in [BCSYM * BCSYM::DigThroughNamedType()].
    // This check should be equivalent to the one performed in [TypeHelpers::IsMarkedAsEmbeddedLocalType] as well.

    WellKnownAttrVals * pWellKnownAttrVals = pContainer->GetPWellKnownAttrVals();
    
    if (!pWellKnownAttrVals->GetTypeIdentifierData())
    {
#if IDE     
        if (fIgnoreTemporaryEmbeddedStatus || !pContainer->TreatAsEmbeddedLocalType())
#endif            
        {
            return false;
        }
    }

    if ( pContainer->IsInterface() )
    {
        if (!pWellKnownAttrVals->GetComImportData())
        {
            return false;
        }
    }
    else if ( !pContainer->IsClass() )
    {
        return false; // this is not a BCSYM_Class and not an interface, can't be embedded.
    }

    CompilerFile *pFile = pContainer->GetCompilerFile();

    if ( pFile == NULL )
    {
        return false;
    }

    CompilerProject *pContainingProject = pFile->GetCompilerProject();

    if (!pContainingProject)
    {
        return false;
    }
    
    CompilerProject *pBuildProject = pContainingProject->GetCompiler()->GetProjectBeingCompiled();
    if (!pBuildProject)
    {
        return false;
    }

    if (pContainingProject == pBuildProject) 
    {
        return false;
    }

    return true; 
}






/******************************************************************************
;ParseGuid

Parse a string containing a guid value. We must accept any of the five formats
the System.Guid string constructor accepts, but since we are in unmanaged code
we cannot actually invoke that constructor. There does not appear to be any
unmanaged guid parser which accepts all of these formats, so we must parse the 
data ourselves. The System.Guid constructor is documented here:
    <http://msdn.microsoft.com/en-us/library/aa328604(VS.71).aspx>
If the string matches one of these formats, we will return true and populate
the GUID structure pointed at by the 'out' parameter. If the string does not 
match any of the formats, we will return false, and leave the out param alone.
******************************************************************************/
static bool ParseGuid
(
    _In_z_ const WCHAR *GuidIn, 
    _Out_ GUID *out
)
{
    if (!GuidIn || !out)
    {
        return false;
    }
    bool result = false;
    // The System.Guid constructor accepts guid strings with leading or trailing spaces, so we must
    // accept them, too. (The System.Guid documentation does not mention this detail, but it can be
    // observed in the disassembly of mscorlib.dll.) We'll skip over any leading spaces on the input, 
    // and once we've detected the length we'll decrement until the last character is no longer a 
    // space. This gives us just the chunk in the middle.
    while (iswspace(*GuidIn))
    {
        GuidIn++;
    }
    size_t guidlen = wcslen(GuidIn);
    while (guidlen > 0 && iswspace(GuidIn[guidlen - 1]))
    {
        guidlen--;
    }
    // Four of the guid formats have fixed lengths; the fifth has a variable length, but will always 
    // be longer than any of the fixed-length formats. We will use IIDFromString to parse all of the
    // fixed-width formats. It only accepts strings of the form "{00000000-0000-0000-0000-000000000000}",
    // but we can easily copy characters from the other types into that form.
    if (guidlen == 32 || guidlen == 36 || guidlen == 38) 
    {
        WCHAR GuidFmt[40];
        errno_t copyErr = 0;
        switch (guidlen) {
            case 32: {
                // 32 contiguous digits: 
                // "00000000000000000000000000000000"
                GuidFmt[0] = L'{';
                copyErr = wmemcpy_s(&GuidFmt[1], _countof(GuidFmt) - 1, &GuidIn[0], 8);
                AssertIfFalse(0 == copyErr);
                GuidFmt[9] = L'-';
                copyErr = wmemcpy_s(&GuidFmt[10], _countof(GuidFmt) - 10, &GuidIn[8], 4);
                AssertIfFalse(0 == copyErr);
                GuidFmt[14] = L'-';
                copyErr = wmemcpy_s(&GuidFmt[15], _countof(GuidFmt) - 15, &GuidIn[12], 4);
                AssertIfFalse(0 == copyErr);
                GuidFmt[19] = L'-';
                copyErr = wmemcpy_s(&GuidFmt[20], _countof(GuidFmt) - 20, &GuidIn[16], 4);
                AssertIfFalse(0 == copyErr);
                GuidFmt[24] = L'-';
                copyErr = wmemcpy_s(&GuidFmt[25], _countof(GuidFmt) - 25, &GuidIn[20], 12);
                AssertIfFalse(0 == copyErr);
                GuidFmt[37] = L'}';
            } break;
            case 36: {
                // Groups of 8, 4, 4, 4, and 12 digits, with hyphens between the groups:
                // "00000000-0000-0000-0000-000000000000"
                GuidFmt[0] = L'{';
                copyErr = wmemcpy_s(&GuidFmt[1], _countof(GuidFmt) - 1, GuidIn, 36);
                AssertIfFalse(0 == copyErr);
                GuidFmt[37] = L'}';
            } break;
            case 38: {
                // Groups of digits with hyphens, enclosed in either parentheses or braces:
                // "{00000000-0000-0000-0000-000000000000}"
                // "(00000000-0000-0000-0000-000000000000)"
                copyErr = wmemcpy_s(&GuidFmt[0], _countof(GuidFmt), GuidIn, 38);
                AssertIfFalse(0 == copyErr);
                if (GuidFmt[0] == L'(' && GuidFmt[37] == L')')
                {
                    GuidFmt[0] = L'{';
                    GuidFmt[37] = L'}';
                }
            } break;
        }
        GuidFmt[38] = L'\0';
        result = SUCCEEDED(IIDFromString(GuidFmt, out));
    }
    else if (guidlen > 38)
    {
        // The long GUID format, from <http://msdn.microsoft.com/en-us/library/96ff78dc.aspx>:
        // Groups of 8, 4, and 4 digits, and a subset of eight groups of 2 digits, with each group 
        // prefixed by "0x" or "0X", and separated by commas. The entire GUID, as well as the subset,
        // is enclosed in matching braces: 
        // {0xdddddddd, 0xdddd, 0xdddd,{0xdd,0xdd,0xdd,0xdd,0xdd,0xdd,0xdd,0xdd}} 
        // All braces, commas, and "0x" prefixes are required. All embedded spaces are ignored. All 
        // leading zeroes in a group are ignored. The digits shown in a group are the maximum number 
        // of meaningful digits that can appear in that group. You can specify from 1 to the number of 
        // digits shown for a group. The specified digits are assumed to be the low order digits of the 
        // group. 
        
        const WCHAR *format = L"{0xd,0xd,0xd,{0xd,0xd,0xd,0xd,0xd,0xd,0xd,0xd}}";
        unsigned long number[11]={0,0,0,0,0,0,0,0,0,0,0};
        bool found[11]={false,false,false,false,false,false,false,false,false,false,false};
        unsigned long mask[11]={0xFFFFFFF,0xFFF,0xFFF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF};
        AssertIfFalse(_countof(number) == _countof(found) && _countof(number) == _countof(mask));
        unsigned int num = 0;
        
        const WCHAR *current = GuidIn;
        while (*current && *format)
        {
            AssertIfFalse(!iswspace(*format));
            WCHAR ch = tolower(*current);
            // Skip past all whitespace, no matter where it appears. This is a weird way to handle whitespace
            // but it matches the System.Guid constructor, which simply strips out all spaces before parsing.
            if (iswspace(ch))
            {
                // Do nothing.
            }
            else if (*format == L'd')
            {
                AssertIfFalse(num < _countof(number));
                AssertIfFalse(num < _countof(found));
                AssertIfFalse(num < _countof(mask));
                // Look for a hex digit. If we find one, shift it in as the new low nibble on the 
                // current input number. If we do not find a hex digit, move to the next format character.
                // Check each field before we shift in the new digit to make sure we will not overflow.
                if (ch >= L'0' && ch <= L'9')
                {
                    if (0 != (number[num] & ~mask[num]))
                    {
                       break;
                    }
                    number[num] = ((number[num] << 4) | (ch - L'0' + 0x0));
                    found[num] = true;
                }
                else if (ch >= L'a' && ch <= L'f')
                {
                    if (0 != (number[num] & ~mask[num]))
                    {
                       break; 
                    }
                    number[num] = ((number[num] << 4) | (ch - L'a' + 0xa));
                    found[num] = true;
                }
                else
                {
                    // If we found at least one digit in this group, we will move on to the next format 
                    // character, and to the next number group; otherwise, this is a mismatch.
                    if (found[num])
                    {
                        format++;
                        num++;
                        continue;
                    }
                    else
                    {
                       break;
                    }
                }
            }
            else
            {
                // Match the input character to the format character. If they match, move on to the
                // next format character. If they are not equal, bail out; this is an error.
                if (ch == *format)
                {
                    format++;
                }
                else
                {
                   break;
                }
            }
            // Move on to the next input character.
            current++;
        }
        // If we reached the end of the format string and the input string at the same time, the parse
        // must have been successful. If we bailed out early due to mismatch, there would still be format
        // characters left. If we reached the end of the format string, and the input string contains 
        // trailing spaces, we'll simply ignore them; if the input string still contains some other kind
        // of characters, that indicates a mismatch (too much input for the format).
        while (iswspace(*current))
        {
            current++;
        }
        if (L'\0' == *format && L'\0' == *current)
        {
            // This is a valid GUID string. Populate the output structure with the values we discovered.
            result = true;
            out->Data1 = number[0];
            out->Data2 = (unsigned short)number[1];
            out->Data3 = (unsigned short)number[2];
            for (int i=0; i<8; i++)
            {
                out->Data4[i] = (unsigned char)number[3+i];
            }
        }
    }
    return result;
}

/******************************************************************************
;GetSymbolGuid

Helper function for AreTypeIdentitiesEquivalent. Given a type, retrieve the
value of its GuidAttribute as a GUID struct. Returns true if the type has a
GuidAttribute whose value validates; false otherwise.
******************************************************************************/
static bool GetSymbolGuid
(
    _In_ BCSYM_NamedRoot *entity, 
    _Out_ GUID *out
)
{
    AssertIfNull(entity);
    AssertIfNull(out);
    if (!entity || !out) 
    {
        return false;
    }
    bool Result = false;
    WCHAR *GuidIn = NULL;
    if (entity->GetPWellKnownAttrVals()->GetGuidData(&GuidIn))
    {
        Result = ParseGuid(GuidIn, out);
    }
    return Result;
}

/******************************************************************************
;LookupTypeIdentityValues

Helper function for AreTypeIdentitiesEquivalent. Given a type, 
retrieve the scope-GUID and type name used for unification. If the type is an 
embedded local type, it will have a TypeIdentifierAttribute with parameter
values, and its parameters are the values we will use. If the type is a normal 
type, we must look the values up from the symbol itself. The scope-GUID is the 
containing assembly's GUID, and the type name in that case is simply the type 
symbol's own name.
******************************************************************************/
static bool LookupTypeIdentityValues
(
    _In_ BCSYM *ptyp,
    _Out_ GUID *pScopeGuid, 
    _Out_ WCHAR **pName
)
{
    AssertIfNull(ptyp);
    AssertIfFalse(ptyp->IsNamedRoot());
    AssertIfNull(pScopeGuid);
    AssertIfNull(pName);
    WCHAR *pScopeGuidStr = NULL;
    if (!ptyp->GetPWellKnownAttrVals()->GetTypeIdentifierData(&pScopeGuidStr, pName))
    {
        *pName = ptyp->PNamedRoot()->GetQualifiedName(); // Dev10 #735359 The name used for identity check should be a fully qualified name. 

        FileIterator files(ptyp->PNamedRoot()->GetContainingProject());
        while (CompilerFile *pFile = files.Next())
        {
            BCSYM_Namespace *psymNamespace = pFile->GetRootNamespace();
            AssertIfNull(psymNamespace);
            if (psymNamespace && psymNamespace->GetPWellKnownAttrVals()->GetGuidData(&pScopeGuidStr))
            {
                break;
            }
        }
    }
    // If we have a guid string, attempt to turn it into a GUID structure.
    return (*pName != NULL) && 
        ParseGuid(pScopeGuidStr, pScopeGuid);
}

/*****************************************************************************
;AreTypeIdentitiesEquivalent

Local copies of PIA types have special type-identity semantics. These types can be recognized by the 
presence of a TypeIdentifierAttribute. If a type contains such an attribute, we regard it as a local 
copy of an original type, imported from some other assembly. Local types must have the same qualified 
name and GUID as the original type, so we can use simple string comparisons to match them. Equivalence 
is transitive, so this comparison applies if either type or both has the TypeIdentifierAttribute.
This fixes bug #513539. This has nothing to do with the diamond-reference problem, despite the use of
a qualified-name comparison in both places (see the comment in TypeHelpers::EquivalentTypes). That is
a very narrowly-defined situation where we end up with different pointers to the same type; in the
TypeIdentifier situation we are dealing with here, there are two completely different .NET types which 
are considered to be semantically equivalent because they both stand for the same external COM type.
******************************************************************************/
bool TypeHelpers::AreTypeIdentitiesEquivalent
(
    _In_ BCSYM *ptyp1, 
    _In_ BCSYM *ptyp2
)
{
    // !!! THIS FUNCTION SHOULD BE VERY VERY FAST !!!
    
    if (!ptyp1 || 
        ptyp1->IsGenericBinding() ||  // We do not embed generic bindings, let's filter them out early
        !ptyp1->IsContainer() )
    {
        return false;
    }

    if (!ptyp2 || 
        ptyp2->IsGenericBinding() ||  // We do not embed generic bindings, let's filter them out early
        !ptyp2->IsContainer() )
    {
        return false;
    }

    return AreTypeIdentitiesEquivalent(ptyp1->PContainer(), ptyp2->PContainer());
}

/*****************************************************************************
;AreTypeIdentitiesEquivalent

Local copies of PIA types have special type-identity semantics. These types can be recognized by the 
presence of a TypeIdentifierAttribute. If a type contains such an attribute, we regard it as a local 
copy of an original type, imported from some other assembly. Local types must have the same qualified 
name and GUID as the original type, so we can use simple string comparisons to match them. Equivalence 
is transitive, so this comparison applies if either type or both has the TypeIdentifierAttribute.
This fixes bug #513539. This has nothing to do with the diamond-reference problem, despite the use of
a qualified-name comparison in both places (see the comment in TypeHelpers::EquivalentTypes). That is
a very narrowly-defined situation where we end up with different pointers to the same type; in the
TypeIdentifier situation we are dealing with here, there are two completely different .NET types which 
are considered to be semantically equivalent because they both stand for the same external COM type.
******************************************************************************/
bool TypeHelpers::AreTypeIdentitiesEquivalent
(
    _In_ BCSYM_Container *ptyp1, 
    _In_ BCSYM_Container *ptyp2
)
{
    // !!! THIS FUNCTION SHOULD BE VERY VERY FAST !!!
    
    // First test: does type equivalence even potentially apply to this situation?
    if (!TypeHelpers::IsEmbeddedLocalType(ptyp1) && !TypeHelpers::IsEmbeddedLocalType(ptyp2))
    {
        // Neither has a TypeIdentifierAttribute.
        return false;
    }

    // Second test: are the fully qualified names equal?
    if (0 != StringPool::Compare(
            ptyp1->GetQualifiedName(), 
            ptyp2->GetQualifiedName()))
    {
        // Names do not match.
        return false;
    }
 
    // The third test depends on the type of type. We cannot unify dissimilar types of types,
    // and the rules are different for interfaces than for other types: if they have a Guid attribute,
    // we will use that guid instead of the assembly guid.
    if (ptyp1->IsInterface() && ptyp2->IsInterface())
    {
        // Two interfaces must both have a GuidAttribute and the values of the guids must match.
        GUID guid1;
        GUID guid2;
        bool hasGuid1 = GetSymbolGuid(ptyp1->PInterface(), &guid1);
        bool hasGuid2 = GetSymbolGuid(ptyp2->PInterface(), &guid2);
        if (hasGuid1 || hasGuid2)
        {
            return hasGuid1 && hasGuid2 && IsEqualGUID(guid1, guid2);
        }
    }
    bool fEquivalent = false;
    if ((ptyp1->IsInterface() && ptyp2->IsInterface()) ||
        (ptyp1->IsStruct() && ptyp2->IsStruct()) ||
        (ptyp1->IsEnum() && ptyp2->IsEnum()) ||
        (ptyp1->IsDelegate() && ptyp2->IsDelegate()))
    {
        // The types must have matching names and assembly GUIDs.
        // A local type stores its canonical type's name and assembly guid values in its
        // TypeIdentifierAttribute.
        GUID type1Guid;
        WCHAR *type1Name = NULL;
        GUID type2Guid;
        WCHAR *type2Name = NULL;
        bool hasGuid1 = LookupTypeIdentityValues(ptyp1, &type1Guid, &type1Name);
        bool hasGuid2 = LookupTypeIdentityValues(ptyp2, &type2Guid, &type2Name);
        fEquivalent = hasGuid1 && hasGuid2 &&
            IsEqualGUID(type1Guid, type2Guid) && 
            type1Name != NULL && type2Name != NULL &&
            (0 == CompareString32(type1Name, type2Name));
    }
    else
    {
        // Identity semantics do not apply to any other types.
    }
    return fEquivalent;
}


// Dev10 #735384
// Check if passed in type uses an embedded type in context of either passed in project.
bool TypeHelpers::IsOrUsingEmbeddedType
(
    BCSYM *pType,            
    CompilerProject * pContextProject,
    CompilerProject * pUseProject
)
{
    // skip bad input
    if (pType == NULL || pType->IsBad())
    {
        return false;
    }

    if (pType->IsPointerType())
    {
        pType = DescendToPossiblyEmbeddedType(pType->PPointerType()->GetRawRoot());
    }

    if (pType->IsGenericTypeBinding())
    {
        BCSYM_GenericTypeBinding * pBinding = pType->PGenericTypeBinding();
    
        do
        {
            unsigned ArgumentCount = pBinding->GetArgumentCount();

            for (unsigned ArgumentIndex = 0; ArgumentIndex < ArgumentCount; ArgumentIndex++)
            {
                BCSYM * pArgumentType = pBinding->GetArguments()[ArgumentIndex];

                if (pArgumentType != NULL && pArgumentType->IsNamedType())
                {
                    pArgumentType = pArgumentType->PNamedType()->GetSymbol();
                }
                
                if (IsOrUsingEmbeddedType(pArgumentType, pContextProject, pUseProject))
                {
                    return true;
                }
            }

            pBinding = pBinding->GetParentBinding();
        }
        while (pBinding != NULL);
    }
    else if (pType->IsContainer())
    {
        BCSYM_Container * pContainer = pType->PContainer();
            
        if (TypeHelpers::IsMarkedAsEmbeddedLocalType(pContainer))
        {
            return true; 
        }
        else
        {
            CompilerProject * pContainerProject = NULL;
            CompilerFile *pContainerFile = pContainer->GetCompilerFile();

            if ( pContainerFile != NULL )
            {
                pContainerProject = pContainerFile->GetCompilerProject();
            }

            if ( pContainerProject != NULL )
            {
                if (!pContextProject->IsMetaData() && 
                    pContextProject != pContainerProject && 
                    pContextProject->HasEmbeddedReferenceTo(pContainerProject))
                {
                    return true;
                }
                else if (pContextProject != pUseProject &&
                    pUseProject != pContainerProject && 
                    pUseProject->HasEmbeddedReferenceTo(pContainerProject))
                {
                    return true;
                }
            }

        }
    }
    else if (pType->IsArrayType())
    {
        return IsOrUsingEmbeddedType(DescendToPossiblyEmbeddedType(pType->PArrayType()->GetRawRoot()), pContextProject, pUseProject);
    }

    return false;
}



// Dev10 #735384
// A helper function to report an error
void ReportIllegalUseOfEmbeddedType
(
    CompilerHost * pCompilerHost,
    ErrorTable * pErrorTable,
    Location *pReferencingLocation,
    ERRID errid,
    BCSYM * pErrorArg 
)
{
    StringBuffer buffer;
    
    WCHAR * typeName = pErrorTable->ExtractErrorName
            (
                pErrorArg,
                NULL, // ContainingContext
                buffer,
                NULL, // GenericBindingContext
                false, // FormatAsExtensionMethod
                NULL, // FixedTypeArgumentBitVector
                pCompilerHost
            );

    if (!pErrorTable->HasThisErrorWithLocationAndParameters(
                errid, 
                pReferencingLocation,
                typeName))
    {
        pErrorTable->CreateError(
            errid,
            pReferencingLocation,
            typeName);
    }
}



// Dev10 #735384
// Check classes for improper use of embedded types.
void TypeHelpers::CheckClassesForUseOfEmbeddedTypes
(
    BCSYM *pType,            
    CompilerProject * pUseProject,
    ErrorTable * pErrorTable,
    Location *pReferencingLocation 
)
{
    // skip bad input
    if (pType == NULL || pType->IsBad())
    {
        return;
    }

    if (pType->IsPointerType())
    {
        pType = pType->PPointerType()->GetRoot()->DigThroughAlias();
    }

    if (pType->IsGenericTypeBinding())
    {
        BCSYM_GenericTypeBinding * pBinding = pType->PGenericTypeBinding();
    
        do
        {
            unsigned ArgumentCount = pBinding->GetArgumentCount();

            for (unsigned ArgumentIndex = 0; ArgumentIndex < ArgumentCount; ArgumentIndex++)
            {
                BCSYM * pArgumentType = pBinding->GetArgument(ArgumentIndex);

                CheckClassesForUseOfEmbeddedTypes(
                    pArgumentType, 
                    pUseProject, 
                    pErrorTable, 
                    pReferencingLocation
                    );
            }

            pBinding = pBinding->GetParentBinding();
        }
        while (pBinding != NULL);
    }
    else if (pType->IsContainer())
    {
        if (pType->IsClass())
        {
            BCSYM_Class * pClass = pType->PClass();
            CompilerProject * pContainingProject = pClass->GetContainingProject();
            
            if (pContainingProject != pUseProject)
            {
                // Checking for a class that inherits from generic instantiation closed over a local type
                BCSYM * pBase = pClass->GetBaseClass();

                if (pBase != NULL && !pBase->IsBad())  
                {
                    do
                    {
                        if (pBase->IsGenericTypeBinding())
                        {
                            if (IsOrUsingEmbeddedType(pBase, pContainingProject, pUseProject))
                            {
                                ReportIllegalUseOfEmbeddedType(
                                    pUseProject->GetCompilerHost(),
                                    pErrorTable,
                                    pReferencingLocation,
                                    ERRID_CannotUseGenericBaseTypeAcrossAssemblyBoundaries,
                                    pClass);
                                    
                                break;
                            }
                            
                            pBase = pBase->PGenericTypeBinding()->GetGenericType();
                        }

                        // if the base is in the same project, check its base as well
                        BCSYM_Class * pBaseClass = pBase->PClass();
                        
                        if (pBaseClass->GetContainingProject() == pContainingProject)
                        {
                            pBase = pBaseClass->GetBaseClass();
                        }
                        else
                        {
                            pBase = NULL;
                        }
                    }
                    while(pBase != NULL && !pBase->IsBad());
                }

                // Check signature for an imported delegate
                if (pClass->IsDelegate())
                {
                    BCSYM_NamedRoot * pInvoke =  GetInvokeFromDelegate(pClass, pUseProject->GetCompiler());

                    if (pInvoke != NULL && pInvoke->IsProc())
                    {
                        CheckProcForUseOfEmbeddedTypes(
                            pInvoke->PProc(),            
                            pUseProject,
                            pContainingProject,        
                            pErrorTable,
                            pReferencingLocation);
                    }
                }
            }
        }
    }
    else if (pType->IsArrayType())
    {
        CheckClassesForUseOfEmbeddedTypes(
            pType->PArrayType()->GetRoot(), 
            pUseProject, 
            pErrorTable, 
            pReferencingLocation
            );
    }
    else if (pType->IsGenericParam())
    {
        // nothing to check
    }
    else
    {
        VSFAIL("Not a type symbol.");
    }
}


// Dev10 #735384
// Check generic instantiations for improper use of embedded types.
void TypeHelpers::CheckGenericsForUseOfEmbeddedTypes
(
    BCSYM *pType,            
    CompilerProject * pUseProject,
    CompilerProject * pImportedFromProject,
    ErrorTable * pErrorTable,
    Location *pReferencingLocation 
)
{
    // skip bad input
    if (pType == NULL || pType->IsBad())
    {
        return;
    }

    if (pType->IsPointerType())
    {
        pType = DescendToPossiblyEmbeddedType(pType->PPointerType()->GetRawRoot());
    }

    if (pType->IsGenericTypeBinding())
    {
        BCSYM_GenericTypeBinding * pTypeBinding = pType->PGenericTypeBinding();
        BCSYM_NamedRoot * pGenericType = pTypeBinding->GetGenericType();

        if (pGenericType != NULL && 
            !pGenericType->IsBad() && 
            pGenericType->GetContainingProject() != pUseProject)
        {
            if (pGenericType->IsClass())
            {
                if (IsOrUsingEmbeddedType(pType, pImportedFromProject, pUseProject))
                {
                    ReportIllegalUseOfEmbeddedType(
                        pUseProject->GetCompilerHost(),
                        pErrorTable,
                        pReferencingLocation,
                        ERRID_CannotUseGenericTypeAcrossAssemblyBoundaries,
                        pType);
                }
            }
            else
            {
                do
                {
                    unsigned ArgumentCount = pTypeBinding->GetArgumentCount();

                    for (unsigned ArgumentIndex = 0; ArgumentIndex < ArgumentCount; ArgumentIndex++)
                    {
                        BCSYM * pArgumentType = pTypeBinding->GetArgument(ArgumentIndex);

                        CheckGenericsForUseOfEmbeddedTypes(
                            pArgumentType, 
                            pUseProject, 
                            pImportedFromProject,
                            pErrorTable, 
                            pReferencingLocation
                            );
                    }

                    pTypeBinding = pTypeBinding->GetParentBinding();
                }
                while (pTypeBinding != NULL);
            }
        }
    }
    else if (pType->IsArrayType())
    {
        CheckGenericsForUseOfEmbeddedTypes(
            DescendToPossiblyEmbeddedType(pType->PArrayType()->GetRawRoot()), 
            pUseProject, 
            pImportedFromProject,
            pErrorTable, 
            pReferencingLocation
            );
    }
    else if (pType->IsGeneric())
    {
        CheckGenericParametersForUseOfEmbeddedTypes(
            pType->GetFirstGenericParam(),            
            pUseProject,
            pImportedFromProject, 
            pErrorTable,
            pReferencingLocation);
    }
}

// Dev10 #735384
// Check proc for improper use of embedded types.
void TypeHelpers::CheckProcForUseOfEmbeddedTypes
(
    BCSYM_Proc *pProc,            
    CompilerProject * pUseProject,
    CompilerProject * pImportedFromProject,        
    ErrorTable * pErrorTable,
    Location *pReferencingLocation 
)
{
    BCSYM * pType = DescendToPossiblyEmbeddedType(pProc->GetRawType());

    // check return type
    TypeHelpers::CheckTypeForUseOfEmbeddedTypes(
        pType,            
        pUseProject,
        pImportedFromProject, 
        pErrorTable,
        pReferencingLocation);
    
    // check types of parameters
    BCSYM_Param   *   pParam;

    for (pParam = pProc->GetFirstParam();
         pParam;
         pParam = pParam->GetNext())
    {
        pType = DescendToPossiblyEmbeddedType(pParam->GetRawType());
            
        TypeHelpers::CheckTypeForUseOfEmbeddedTypes(
            pType,            
            pUseProject,
            pImportedFromProject, 
            pErrorTable,
            pReferencingLocation);
    }

    // Check type constraints for generic methods
    TypeHelpers::CheckGenericParametersForUseOfEmbeddedTypes(
        pProc->GetFirstGenericParam(),            
        pUseProject,
        pImportedFromProject, 
        pErrorTable,
        pReferencingLocation);

    // We want to check the containing type.  If this is a shared procedure,
    // we've already checked the base symbol.
    if (!pProc->IsShared())
    {
        TypeHelpers::CheckGenericsForUseOfEmbeddedTypes(
            pProc->GetContainer(),            
            pUseProject,
            pImportedFromProject, 
            pErrorTable,
            pReferencingLocation);
    }
}

// Check generic parameter constraints for embedded types
void TypeHelpers::CheckGenericParametersForUseOfEmbeddedTypes
(
    BCSYM_GenericParam *pFirstGenericParam,
    CompilerProject * pUseProject,
    CompilerProject * pImportedFromProject,
    ErrorTable * pErrorTable,
    Location *pReferencingLocation 
)
{
    BCSYM_GenericParam *pGenericParam = pFirstGenericParam;
    while (pGenericParam != NULL)
    {
        BCSYM_GenericTypeConstraint * pConstraint = pGenericParam->GetTypeConstraints();

        while (pConstraint != NULL)
        {
            BCSYM *pType = DescendToPossiblyEmbeddedType(pConstraint->GetRawType());

            TypeHelpers::CheckTypeForUseOfEmbeddedTypes(
                pType,            
                pUseProject,
                pImportedFromProject, 
                pErrorTable,
                pReferencingLocation);

            pConstraint = pConstraint->Next();
        }

        pGenericParam = pGenericParam->GetNextParam();
    }
}

// Dev10 #735384
// Check type for improper use of embedded types.
void TypeHelpers::CheckTypeForUseOfEmbeddedTypes
(
    BCSYM *pType,            
    CompilerProject * pUseProject,
    CompilerProject * pImportedFromProject,
    ErrorTable * pErrorTable,
    Location *pReferencingLocation 
)
{
    if (pType != NULL && !pType->IsBad())
    {
        TypeHelpers::CheckClassesForUseOfEmbeddedTypes(
            pType,            
            pUseProject,
            pErrorTable,
            pReferencingLocation);

        TypeHelpers::CheckGenericsForUseOfEmbeddedTypes(
            pType,            
            pUseProject,
            pImportedFromProject, 
            pErrorTable,
            pReferencingLocation);
    }
}

// Dev10 #735384
// Dig through Aliases and NamedType, but without embedded type substitution.
BCSYM * TypeHelpers::DescendToPossiblyEmbeddedType(BCSYM * pType)
{
    while (pType != NULL && pType->IsAlias())
    {
        pType = pType->PAlias()->GetAliasedSymbol();
    }

    if (pType != NULL && pType->IsNamedType())
    {
        pType = pType->PNamedType()->GetSymbol();
    }

    return pType;
}


