//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Helper methods for working with Types
//
//-------------------------------------------------------------------------------------------------

#pragma once

// ----------------------------------------------------------------------------
// This namespace provides helper methods to manipulate Type*.
// ----------------------------------------------------------------------------

class TypeHelpers
{

public:

    //
    // These methods are inlined.
    //

    static Type * GetVoidType();
    static Type * GetElementType(ArrayType * pType);
    static Type * GetReferencedType(PointerType * pType);
    static bool IsByteType(Type * pType);
    static bool IsSignedByteType(Type * pType);
    static bool IsShortType(Type * pType);
    static bool IsUnsignedShortType(Type * pType);
    static bool IsIntegerType(Type * pType);
    static bool IsUnsignedIntegerType(Type * pType);
    static bool IsLongType(Type * pType);
    static bool IsUnsignedLongType(Type * pType);
    static bool IsIntegralType(Type * pType);
    static bool IsUnsignedType(Type * pType);
    static bool IsDecimalType(Type * pType);
    static bool IsSingleType(Type * pType);
    static bool IsDoubleType(Type * pType);
    static bool IsFloatingType(Type * pType);
    static bool IsNumericType(Type * pType);
    static bool IsDateType(Type * pType);
    static bool IsBooleanType(Type * pType);
    static bool IsCharType(Type * pType);
    static bool IsStringType(Type * pType);
    static bool IsArrayType(Type * pType);
	static bool IsVoidArrayLiteralType(Type * pType);
    static bool IsCharArrayRankOne(Type * pType);
    static bool IsUndefinedType(Type * pType);
    static bool IsRecordType(Type * pType);
    static bool IsPointerType(Type * pType);
    static bool IsVoidType(Type * pType);
    static bool IsRootObjectType(Type * pType);
    static bool IsIntrinsicType(Type * pType);
    static bool IsEnumType(Type * pType);
    static bool IsIntrinsicOrEnumType(Type * pType);
    static bool IsInterfaceType(Type * pType);
    static bool IsClassType(Type * pType);
    static bool IsClassOrRecordType(Type * pType);
    static bool IsClassOrInterfaceType(Type * pType);
    static bool IsClassInterfaceOrRecordType(Type * pType);
    static bool IsClassInterfaceRecordOrGenericParamType(Type * pType);
    static bool IsValueType(Type * pType);
    static bool IsPrimitiveValueType(Type * pType);
    static bool IsDelegateType(Type * pType);
    static bool IsStrictSupertypeOfConcreteDelegate(Type * pType, FX::FXSymbolProvider *symbolProvider);
    static bool IsGenericParameter(Type * pType);
    static bool IsGenericTypeBinding(Type * pType);
    static bool IsValueTypeOrGenericParameter(Type * pType);
    static bool IsRootArrayType(Type * pType, CompilerHost * Host);
    static bool IsRootValueType ( Type *T, CompilerHost * Host);
    static bool IsOrInheritsFrom(Type * Derived, Type * Base);
    static bool IsOrInheritsFromOrImplements(Type * Derived, Type * Base, CompilerHost * Host);
    static bool IsCompatibleWithGenericEnumerableType(Type * pType, CompilerHost * Host);
    static bool IsCompatibleWithTypeOrGenericEnumerableType(
        Type * pType,
        Type * TArg,
        _In_ Symbols &SymbolCreator,
        CompilerHost * Host);

    static bool IsConstrainedToNullable
    (
        Type * pType, 
        CompilerHost * pHost
    );
    
    static bool IsNullableType(
        Type * pType,
        CompilerHost * Host);
    static bool IsSystemNullableClass(
        Type * pType,
        CompilerHost * Host);
    static Type * GetElementTypeOfNullable(
        Type * pType,
        CompilerHost * Host);
    static Type * GetElementTypeOfNullable(Type * pType);
    static bool IsFunctionPointerType(Type *pType, CompilerHost * Host);
    static bool IsEnumerableType(Type * pType, CompilerHost * Host);
    static bool IsGenericEnumerableType(Type * pType, CompilerHost * Host);
    static bool IsEnumeratorType(Type * pType, CompilerHost * Host);
    static bool IsGenericEnumeratorType(Type * pType, CompilerHost * Host);
    static bool IsGenericEnumerableBindingType(
        Type * pType,
        Type * BoundArgument,
        CompilerHost * Host);
    static bool IsDBNullType(Type * pType, CompilerHost * Host);
    static bool IsEmbeddableInteropType(_In_ Type *pType);
    
    static bool IsEmbeddedLocalType(
        _In_ Type *pType
#if IDE         
        , bool fIgnoreTemporaryEmbeddedStatus = false
#endif
        );

    static bool IsEmbeddedLocalType(
        _In_ BCSYM_Container *pType
#if IDE         
        , bool fIgnoreTemporaryEmbeddedStatus = false
#endif
        );

    static bool IsMarkedAsEmbeddedLocalType(_In_ BCSYM_Container *pContainer);
    static bool AreTypeIdentitiesEquivalent(_In_ Type *ptyp1, _In_ Type *ptyp2);
    static bool AreTypeIdentitiesEquivalent(_In_ BCSYM_Container *ptyp1, _In_ BCSYM_Container *ptyp2);
    static bool IsOrUsingEmbeddedType
    (
        BCSYM *pType,            
        CompilerProject * pContextProject,
        CompilerProject * pUseProject
    );

    static void CheckClassesForUseOfEmbeddedTypes
    (
        BCSYM *pType,            
        CompilerProject * pUseProject,
        ErrorTable * pErrorTable,
        Location *pReferencingLocation 
    );

    static void CheckGenericsForUseOfEmbeddedTypes
    (
        BCSYM *pType,            
        CompilerProject * pUseProject,
        CompilerProject * pImportedFromProject,        
        ErrorTable * pErrorTable,
        Location *pReferencingLocation 
    );

    static void CheckProcForUseOfEmbeddedTypes
    (
        BCSYM_Proc *pProc,            
        CompilerProject * pUseProject,
        CompilerProject * pImportedFromProject,        
        ErrorTable * pErrorTable,
        Location *pReferencingLocation 
    );

    static void CheckTypeForUseOfEmbeddedTypes
    (
        BCSYM *pType,            
        CompilerProject * pUseProject,
        CompilerProject * pImportedFromProject,
        ErrorTable * pErrorTable,
        Location *pReferencingLocation 
    );

    static void CheckGenericParametersForUseOfEmbeddedTypes
    (
        BCSYM_GenericParam *pFirstGenericParam,
        CompilerProject * pUseProject,
        CompilerProject * pImportedFromProject,
        ErrorTable * pErrorTable,
        Location *pReferencingLocation 
    );

    static BCSYM * DescendToPossiblyEmbeddedType(BCSYM * pType);
    

    //
    //
    // These methods are not inlined.
    //
    //

    static bool IsBadType(Type * pType);  // checks whether an error in the user's VB program makes the type not-allowed

	static bool IsKnownType(Type * pType);  // for use only in asserts, to ensure consistency in functions that think they can handle every conceivable type

    static bool HasVariance(Type *pType);  // checks pType has any generic parameters declared as "In" or "Out"

    static bool IsReferenceType(Type * pType);

    // In checking for inheritance/implementation, some cases wish to require implementation or inheritance of
    // a specific constructions of a generic base type or implemented interface, and some cases don't care. For
    // the cases that don't care, chasing through generic bindings can eliminate the generation of spurious
    // generic bindings.
    //
    // Also there are some cases that require all the bindings of a given generic type that are either inherited
    // or implemented by a type. So in those cases, MatchingBaseGenericBindings can be passed in to collect these
    // bindings. Given a raw generic, all the bindings will be found. If a generic binding is given, then the set
    // will only contain the this binding if found. Note that there might be duplicates entries in the return set
    // because for example both a derived class and the base class might implement the same binding of an interface
    // in which case this binding is found twice and added each time to the set. The callers are responsible for
    // ----ing out the dupes because it may not be needed or would usually be more efficient based on the
    // requirements of the different callers.

    static bool IsOrInheritsFrom(
        Type *Derived,
        Type *Base,
        _In_ Symbols &SymbolCreator,
        bool ChaseThroughGenericBindings,
        _Inout_opt_ DynamicArray<GenericTypeBinding *> *MatchingBaseGenericBindings,
        // Consider types as equivalent based on their project equivalence.
        bool IgnoreProjectEquivalence = false,      
        // The project containing a component of type Derived that was considered
        // for a successful match ignoring project equivalence.
        _Out_opt_ CompilerProject **ProjectForDerived = NULL, 
        // The project containing a component of type Base that was considered
        // for a successful match ignoring project equivalence.
        _Out_opt_ CompilerProject **ProjectForBase = NULL);

    // This is a helper for a specialized compare required for IsOrInheritsFrom

    static bool Type1EquivalentToType2OrItsRawGeneric (
        Type *Type1,
        Type *Type2,
        // Consider types as equivalent based on their project equivalence.
        bool IgnoreProjectEquivalence = false,
        // The project containing a component of type Type1 that was considered
        // for a successful match ignoring project equivalence.
        _Out_opt_ CompilerProject **ProjectForType1 = NULL,   
        // The project containing a component of type Type2 that was considered
        // for a successful match ignoring project equivalence.
        _Out_opt_ CompilerProject **ProjectForType2 = NULL);

    static bool IsOrInheritsFromOrImplements(
        Type *Derived,
        Type *Base,
        _In_ Symbols &SymbolCreator,
        bool ChaseThroughGenericBindings,
        _Inout_opt_ DynamicArray<GenericTypeBinding *> *MatchingBaseGenericBindings,
        CompilerHost *CompilerHost,
        // Consider types as equivalent based on their project equivalence.
        bool IgnoreProjectEquivalence = false,
        // The project containing a component of type Derived that was considered
        // for a successful match ignoring project equivalence.
        _Out_opt_ CompilerProject **ProjectForDerived = NULL, 
        // The project containing a component of type Base that was considered
        // for a successful match ignoring project equivalence.
        _Out_opt_ CompilerProject **ProjectForBase = NULL);

    static bool Implements(
        Type *Derived,
        Type *Base,
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
    );

    static Type * GetBaseClass(
        Type *Derived,
        // If the base class refers to generic arguments of the derived class, a fresh
        // generic binding will be created.
        _In_ Symbols &SymbolCreator);

    static Type * GetReturnTypeOfDelegateType(Type *DelegateType, Compiler *pCompiler); // return Void if it's a Sub, or NULL if it's not a concrete delegate

    static bool EquivalentTypes
    (
        Type *T1,
        Type *T2,
        // Consider types as equivalent based on their project equivalence.
        bool IgnoreProjectEquivalence = false,
        // The project containing a component of type T1 that was considered
        // for a successful match ignoring project equivalence.
        _Out_opt_ CompilerProject **ProjectForT1 = NULL,
        // The project containing a component of type T2 that was considered
        // for a successful match ignoring project equivalence.
        _Out_opt_ CompilerProject **ProjectForT2 = NULL
        IDE_ARG(bool ConsiderOpenGenericParamsEqual = false)
    );

    static bool EquivalentTypeBindings
    (
        GenericTypeBinding *Binding1,
        GenericTypeBinding *Binding2,
        // Consider types as equivalent based on their project equivalence.
        bool IgnoreProjectEquivalence = false,
        // The project containing a component of type Binding1 that was considered
        // for a successful match ignoring project equivalence.
        _Out_opt_ CompilerProject **ProjectForBinding1 = NULL,
        // The project containing a component of type Binding2 that was considered
        // for a successful match ignoring project equivalence.
        _Out_opt_ CompilerProject **ProjectForBinding2 = NULL
        IDE_ARG(bool ConsiderOpenGenericParamsEqual = false)
    );

    static bool EquivalentTypeBindings(_In_ DynamicArray<GenericTypeBinding *> &Bindings);

    static bool ArePossiblyEquivalentIgnoringProjectEquivalence(
        BCSYM_Container * Container1,
        BCSYM_Container * Container2,
        _Out_opt_ CompilerProject * * ProjectForContainer1,
        _Out_opt_ CompilerProject * * ProjectForContainer2);

    // Returns true if type pType is/inherits from/implements IEnumerable(Of U), and U is/inherits from/implements TArg

    static bool IsCompatibleWithGenericEnumerableType(
        Type * pType,
        Type * TArg,
        _In_ Symbols &SymbolCreator,
        CompilerHost * Host);

    static bool IsNullableType(Type * pType);

    // $



    static bool IsNullableTypeWithContantainerContext(
        Type * pType,
        BCSYM_Container * ContainerContext);
};

//
//
// Inline method definitions
//
//

inline Type * TypeHelpers::GetVoidType()
{
    return Symbols::GetVoidType();
}

inline Type * TypeHelpers::GetElementType(ArrayType * pType)
{
    return pType->GetRoot()->DigThroughAlias();
}

inline Type * TypeHelpers::GetReferencedType(PointerType * pType)
{
    return pType->GetRoot()->DigThroughAlias();
}

inline bool TypeHelpers::IsByteType(Type * pType)
{
    return pType->GetVtype() == t_ui1;
}

inline bool TypeHelpers::IsSignedByteType(Type * pType)
{
    return pType->GetVtype() == t_i1;
}

inline bool TypeHelpers::IsShortType(Type * pType)
{
    return pType->GetVtype() == t_i2;
}

inline bool TypeHelpers::IsUnsignedShortType(Type * pType)
{
    return pType->GetVtype() == t_ui2;
}

inline bool TypeHelpers::IsIntegerType(Type * pType)
{
    return pType->GetVtype() == t_i4;
}

inline bool TypeHelpers::IsUnsignedIntegerType(Type * pType)
{
    return pType->GetVtype() == t_ui4;
}

inline bool TypeHelpers::IsLongType(Type * pType)
{
    return pType->GetVtype() == t_i8;
}

inline bool TypeHelpers::IsUnsignedLongType(Type * pType)
{
    return pType->GetVtype() == t_ui8;
}

inline bool TypeHelpers::IsIntegralType(Type * pType)
{
    return ::IsIntegralType(pType->GetVtype());
}

inline bool TypeHelpers::IsUnsignedType(Type * pType)
{
    return ::IsUnsignedType(pType->GetVtype());
}

inline bool TypeHelpers::IsDecimalType(Type * pType)
{
    return pType->GetVtype() == t_decimal;
}

inline bool TypeHelpers::IsSingleType(Type * pType)
{
    return pType->GetVtype() == t_single;
}

inline bool TypeHelpers::IsDoubleType(Type * pType)
{
    return pType->GetVtype() == t_double;
}

inline bool TypeHelpers::IsFloatingType(Type * pType)
{
    Vtypes TypeCode = pType->GetVtype();
    return TypeCode == t_double || TypeCode == t_single;
}

inline bool TypeHelpers::IsNumericType(Type * pType)
{
    return ::IsNumericType(pType->GetVtype());
}

inline bool TypeHelpers::IsDateType(Type * pType)
{
    return pType->GetVtype() == t_date;
}

inline bool TypeHelpers::IsBooleanType(Type * pType)
{
    return pType->GetVtype() == t_bool;
}

inline bool TypeHelpers::IsCharType(Type * pType)
{
    return pType->GetVtype() == t_char;
}

inline bool TypeHelpers::IsStringType(Type * pType)
{
    return pType->GetVtype() == t_string;
}

inline bool TypeHelpers::IsArrayType(Type * pType)
{
    return pType->GetVtype() == t_array;
}

inline bool TypeHelpers::IsVoidArrayLiteralType(Type * pType)
{
	return pType->IsArrayLiteralType() &&
           pType->PArrayLiteralType()->GetRoot()!=NULL &&
           pType->PArrayLiteralType()->GetRoot()->IsVoidType();
}

inline bool TypeHelpers::IsCharArrayRankOne(Type * pType)
{
    return
        IsArrayType(pType) &&
        IsCharType(GetElementType(pType->PArrayType())) &&
        pType->PArrayType()->GetRank() == 1;
}

inline bool TypeHelpers::IsUndefinedType(Type * pType)
{
    return pType->GetVtype() == t_UNDEF;
}

// WARNING: IsRecordType(T) returns false for intrinsic value-types (int, bool, ...), even though
// IsClassOrRecordType(T) returns true for them.
inline bool TypeHelpers::IsRecordType(Type * pType)
{
    return pType->GetVtype() == t_struct;
}

inline bool TypeHelpers::IsPointerType(Type * pType)
{
    return pType->GetVtype() == t_ptr;
}

inline bool TypeHelpers::IsVoidType(Type * pType)
{
    return pType->GetVtype() == t_void;
}

inline bool TypeHelpers::IsRootObjectType(Type * pType)
{
    return pType->IsObject();
}

inline bool TypeHelpers::IsIntrinsicType(Type * pType)
{
    return pType->IsIntrinsicType();
}

inline bool TypeHelpers::IsEnumType(Type * pType)
{
    return pType->IsEnum();
}

inline bool TypeHelpers::IsIntrinsicOrEnumType(Type * pType)
{
    return pType->IsIntrinsicType() || pType->IsEnum();
}

inline bool TypeHelpers::IsInterfaceType(Type * pType)
{
    return pType->IsInterface();
}

// IsClassType returns true if pType is a class (and not a record) type, and false otherwise.
// WARNING: IsClassType(T) returns false for intrinsic value-types, but IsClassOrRecordType(T)
// returns true for them.
inline bool TypeHelpers::IsClassType(Type * pType)
{
    return (pType->GetVtype() == t_ref && !pType->IsInterface()) || pType->GetVtype() == t_string;
}


// WARNING: IsClassOrRecordType(T) returns true for intrinsic value-types (e.g. int, bool)
// even though IsClassType(T) and IsRecordType(T) returns false for them.
inline bool TypeHelpers::IsClassOrRecordType(Type * pType)
{
    return pType->IsClass();
}

inline bool TypeHelpers::IsClassOrInterfaceType(Type * pType)
{
    return pType->GetVtype() == t_ref || pType->GetVtype() == t_string;
}

inline bool TypeHelpers::IsClassInterfaceOrRecordType(Type * pType)
{
    return IsClassOrInterfaceType(pType) || IsRecordType(pType);
}

inline bool TypeHelpers::IsClassInterfaceRecordOrGenericParamType(Type * pType)
{
    return IsClassInterfaceOrRecordType(pType) || pType->IsGenericParam();
}

inline bool TypeHelpers::IsValueType(Type * pType)
{
    return (!pType->IsGenericParam() &&
            !IsReferenceType(pType) &&
            !IsVoidType(pType) &&
            !IsBadType(pType)) ||
                (pType->IsGenericParam() &&
                 pType->PGenericParam()->IsValueType());
}


// IsPrimitiveValueType returns true if a type is primitive and value type,
// otherwise returns false.
//  
//     For a given type T,
//
//         IsPrimitiveValueType(T) IIF (  IsValueType(T) AND IsPrimitiveType(T) )
//
// Although IsPrimitive is not implemented in TypeHelpers class, the above can 
// serve as a spec for it.
inline bool TypeHelpers::IsPrimitiveValueType(Type * pType)
{

    Vtypes vtype = pType->GetVtype();
    return vtype==t_bool || vtype==t_i1 || vtype==t_ui1 || vtype==t_i2 || vtype==t_ui2 || 
           vtype==t_i4 || vtype==t_ui4 || vtype==t_i8 || vtype==t_ui8 ||  vtype==t_decimal || 
           vtype==t_single || vtype==t_double || vtype==t_date || vtype==t_char;
}

inline bool TypeHelpers::IsDelegateType(Type * pType)
{
    return pType->IsDelegate();
}

inline bool TypeHelpers::IsStrictSupertypeOfConcreteDelegate(Type * pType, FX::FXSymbolProvider *symbolProvider)
{
    // Object, System.Delegate and System.MulticastDelegate are the only supertypes
    // of concrete delegates (e.g. Func<Integer>) which aren't themselves concrete delegates.
   return (pType == symbolProvider->GetObjectType() ||
           pType == symbolProvider->GetDelegateType() ||
           pType == symbolProvider->GetMultiCastDelegateType());
}

inline bool TypeHelpers::IsGenericParameter(Type * pType)
{
    return pType->IsGenericParam();
}

inline bool TypeHelpers::IsGenericTypeBinding(Type * pType)
{
    return pType->IsGenericTypeBinding();
}

inline bool TypeHelpers::IsValueTypeOrGenericParameter(Type * pType)
{
    return pType->IsGenericParam() || IsValueType(pType);
}

inline bool TypeHelpers::IsRootArrayType(Type * pType, CompilerHost * Host)
{
    return pType &&
           Host &&
           pType == Host->GetFXSymbolProvider()->GetType(FX::ArrayType);
}

inline bool TypeHelpers::IsRootValueType(Type *pType, CompilerHost * Host)
{
    return pType &&
           Host &&
           pType == Host->GetFXSymbolProvider()->GetType(FX::ValueTypeType);
}

// These version of IsOrInheritsFrom and IsOrInheritsFromOrImplements chase through generic bindings,
// and are appropriate only when either 1) the base type is known not to be a generic binding, or
// 2) the query is in terms of any binding of the base type.

inline bool TypeHelpers::IsOrInheritsFrom(
    Type * Derived,
    Type * Base)
{
    return IsOrInheritsFrom(Derived, Base, *(Symbols *)NULL, true, NULL);
}

inline bool TypeHelpers::IsOrInheritsFromOrImplements(
    Type * Derived,
    Type * Base,
    CompilerHost * Host)
{
    return IsOrInheritsFromOrImplements(Derived, Base, *(Symbols *)NULL, true, NULL, Host);
}

// Returns true if type pType is/inherits from/implements IEnumerable(Of ...)

inline bool TypeHelpers::IsCompatibleWithGenericEnumerableType(
    Type * pType,
    CompilerHost * Host)
{
    return Host->GetFXSymbolProvider()->IsTypeAvailable(FX::GenericIEnumerableType) &&
           IsOrInheritsFromOrImplements(pType, Host->GetFXSymbolProvider()->GetType(FX::GenericIEnumerableType), Host);
}


// Returns true if type pType is/inherits from/implements TArg or if IsCompatibleWithGenericEnumerableType is true

inline bool TypeHelpers::IsCompatibleWithTypeOrGenericEnumerableType(
    Type * pType,
    Type * TArg,
    _In_ Symbols &SymbolCreator,
    CompilerHost * Host)
{
    return IsOrInheritsFromOrImplements(pType, TArg, Host) ||
        IsCompatibleWithGenericEnumerableType(pType, TArg, SymbolCreator, Host);
}

inline bool TypeHelpers::IsNullableType(
    Type * pType,
    CompilerHost * Host)
{
    return (pType && Host &&
           pType->IsGenericTypeBinding() &&
           Host->GetFXSymbolProvider()->IsTypeAvailable(FX::GenericNullableType) &&
           pType->PGenericBinding()->GetGeneric() == Host->GetFXSymbolProvider()->GetType(FX::GenericNullableType));
}


inline bool TypeHelpers::IsSystemNullableClass(
    Type * pType,
    CompilerHost * Host)
{
    return
        pType &&
        Host &&
        Host->GetFXSymbolProvider()->IsTypeAvailable(FX::GenericNullableType) &&
        pType == Host->GetFXSymbolProvider()->GetType(FX::GenericNullableType);
}

inline Type * TypeHelpers::GetElementTypeOfNullable(
    Type * pType,
    CompilerHost * Host)
{
    return ( IsNullableType(pType, Host) ? pType->PGenericTypeBinding()->GetArgument(0) : pType);
}

inline Type * TypeHelpers::GetElementTypeOfNullable(Type * pType)
{
    return ( IsNullableType(pType) ? pType->PGenericTypeBinding()->GetArgument(0) : pType);
}

inline bool TypeHelpers::IsFunctionPointerType(Type * pType, CompilerHost * Host)
{
    // 


    return pType &&
           Host &&
           (pType == Host->GetFXSymbolProvider()->GetType(FX::UIntPtrType) ||
            pType == Host->GetFXSymbolProvider()->GetType(FX::IntPtrType));
}

inline bool TypeHelpers::IsEnumerableType(Type * pType, CompilerHost * Host)
{
    return IsGenericEnumerableType(pType, Host) ||
           (Host->GetFXSymbolProvider()->IsTypeAvailable(FX::IEnumerableType) &&
            pType == Host->GetFXSymbolProvider()->GetType(FX::IEnumerableType));
}

inline bool TypeHelpers::IsGenericEnumerableType(Type * pType, CompilerHost * Host)
{
    return IsGenericTypeBinding(pType) &&
           Host->GetFXSymbolProvider()->IsTypeAvailable(FX::GenericIEnumerableType) &&
           pType->PGenericTypeBinding()->GetGenericType() == Host->GetFXSymbolProvider()->GetType(FX::GenericIEnumerableType);
}

inline bool TypeHelpers::IsEnumeratorType(Type * pType, CompilerHost * Host)
{
    return IsGenericEnumeratorType(pType, Host) ||
           (Host->GetFXSymbolProvider()->IsTypeAvailable(FX::IEnumeratorType) &&
            pType == Host->GetFXSymbolProvider()->GetType(FX::IEnumeratorType));
}

inline bool TypeHelpers::IsGenericEnumeratorType(Type * pType, CompilerHost * Host)
{
    return IsGenericTypeBinding(pType) &&
           Host->GetFXSymbolProvider()->IsTypeAvailable(FX::GenericIEnumeratorType) &&
           pType->PGenericTypeBinding()->GetGenericType() == Host->GetFXSymbolProvider()->GetType(FX::GenericIEnumeratorType);
}

inline bool TypeHelpers::IsGenericEnumerableBindingType(
    Type * pType,
    Type * BoundArgument,
    CompilerHost * Host)
{
    return IsGenericTypeBinding(pType) &&
           Host->GetFXSymbolProvider()->IsTypeAvailable(FX::GenericIEnumerableType) &&
           pType->PGenericTypeBinding()->GetGenericType() == Host->GetFXSymbolProvider()->GetType(FX::GenericIEnumerableType) &&
           pType->PGenericTypeBinding()->GetArgumentCount() == 1 &&
           pType->PGenericTypeBinding()->GetArgument(0) == BoundArgument;
}

inline bool TypeHelpers::IsDBNullType(Type * pType, CompilerHost * Host)
{
    return pType &&
           Host &&
           pType == Host->GetFXSymbolProvider()->GetType(FX::DBNullType);
}



/******************************************************************************
;IsMarkedAsEmbeddedLocalType

Is the parameter a type with the "local type" attributes on it?

- for an interface, does it have TypeIdentifierAttribute and ComImportAttribute?
- for a struct/delegate/enum, does it have TypeIdentifierAttribute?

This information is used e.g. in meta-import and DigThroughNamedType,
where all types that have "local type" attributes are considered potential
candidates for unification.
******************************************************************************/
inline bool TypeHelpers::IsMarkedAsEmbeddedLocalType
(
 _In_ BCSYM_Container* pContainer
)
{
    // !!! THIS FUNCTION SHOULD BE VERY VERY FAST !!!

    WellKnownAttrVals * pWellKnownAttrVals = pContainer->GetPWellKnownAttrVals();
    
    if (!pWellKnownAttrVals->GetTypeIdentifierData())
    {
        return false;
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

    return true; 
}

