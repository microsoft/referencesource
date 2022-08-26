//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Helpers used to create symbols.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

#if DEBUG
static unsigned s_cPassCount = 0;
#endif  // DEBUG

#if TRACK_BCSYMHASH
ULONG g_AllocatedBuckets = 0;
ULONG g_UsedBuckets = 0;
ULONG g_TotalSymbols = 0;
#endif

//
// The following table is used to figure out how we need to allocate/unallocate
// the different symbol types.  It is indexed by the symbol/type kind being allocated.
//

const static
struct
{
    unsigned m_cbSize;            // Amount of memory needed to allocate this
    WCHAR *m_sz;                  // The name (used by DumpSymbolTree) of the bilkind.
}
SymbolSizeInfo[] =
{
#define DEF_BCSYM(x, y)  { sizeof(BCSYM_##x), WIDE(y) },
#include "TypeTables.h"
#undef DEF_BCSYM
};

#pragma warning (push)
#if DEBUG
#pragma prefast(disable: 22109, "Only debug builds have VTables, so disable this warning in debug builds.")
#endif

static BCSYM_GenericBadNamedRoot GenericBadNamedRoot = BCSYM_GenericBadNamedRoot();
static BCSYM_VoidType GenericVoidType = BCSYM_VoidType();
#pragma warning (pop)

/**************************************************************************************************
;Constructor
***************************************************************************************************/
Symbols::Symbols
(
    Compiler *CompilerInstance, // [in] instance of the compiler
    _In_ NorlsAllocator *Allocator,_In_  // Allocator we will use when creating symbols
    LineMarkerTable *LineMarkerTable,_In_  // [in] Line Marker table we will put allocated symbols decl info into
    GenericBindingCache *GenericBindingCache
)
: m_Allocator( Allocator )
, m_LineMarkerTable( LineMarkerTable )
, m_CompilerInstance( CompilerInstance )
, m_GenericBindingCache( GenericBindingCache )
{
}

Operator *
Symbols::LiftUserDefinedOperator
(
    _In_ BCSYM_UserDefinedOperator * pSource,
    GenericTypeBinding * pGenericBindingContext,
    Semantics * pSemantics,    
    CompilerHost * pCompilerHost
)
{

    BCSYM_UserDefinedOperator * ret = (BCSYM_UserDefinedOperator *)AllocSymbol(SYM_UserDefinedOperator, false, 0);

    {
        #if FV_TRACK_MEMORY && IDE

        unsigned long totalSize =  ret->m_totalSize;
        BackupValue<unsigned long> backup_totalSize(&(ret->m_totalSize));

        #endif

        bool tmp_hasLocation = ret->HasLocation();
        bool tmp_isLocationInherited = ret->IsLocationInherited();
                
        memcpy(ret, pSource, sizeof(BCSYM_UserDefinedOperator));
        ret->SetHasLocation(tmp_hasLocation);
        ret->SetIsLocationInherited(tmp_isLocationInherited);
    }

    ret->SetXMLDocNode(NULL);
    ret->SetNextInSymbolList(NULL);

    ret->SetAssociatedPropertyDef(NULL);
    ret->SetHandlerList(NULL);
    ret->SetImplementsList(NULL);
    ret->SetOverriddenProcRaw(NULL);
    ret->SetGenericParams(NULL);
    ret->SetBoundTree(NULL);
    ret->SetToken(0);
    ret->SetComClassToken(0);
    ret->SetAttributesEmitted(false);
    
    ret->SetReturnTypeParam(LiftParameter(ret->GetReturnTypeParam(), pCompilerHost));

    ret->SetFirstParam(LiftOperatorParameters(ret->GetFirstParam(), pGenericBindingContext, pSemantics, pCompilerHost));
    
    if (pSemantics->ShouldLiftType(ret->GetRawType()->DigThroughAlias(), pGenericBindingContext, pCompilerHost))
    {
        ret->SetType(LiftType(ret->GetRawType()->DigThroughAlias(), pCompilerHost));
    }
    ret->SetOperatorMethod(CreateLiftedOperatorImplementation(ret, pSource->GetOperatorMethod(), pCompilerHost));

    return ret;
}


BCSYM_Param *
Symbols::LiftOperatorParameters
(
    BCSYM_Param * pFirstParam,
    GenericTypeBinding * pGenericBindingContext, 
    Semantics * pSemantics,
    CompilerHost * pCompilerHost
)
{

    BCSYM_Param * pRet = NULL;
    BCSYM_Param * pLast = NULL;

    while (pFirstParam)
    {
        BCSYM_Param * pCur = NULL;
        if
        (
            pSemantics->ShouldLiftParameter(pFirstParam, pGenericBindingContext, pCompilerHost)
        )
        {
            pCur = LiftParameter(pFirstParam, pCompilerHost);
        }
        else
        {
            pCur = CloneParameter(pFirstParam, pCompilerHost);
        }

        if (pLast)
        {
            pLast->SetNext(pCur);
        }
        else
        {
            pRet = pCur;
        }

        pLast = pCur;
        pFirstParam = pFirstParam->GetNext();
    }

    return pRet;
}


BCSYM_Param *
Symbols::LiftParameter
(
    _In_opt_ BCSYM_Param * pParam,
    CompilerHost * pCompilerHost
)
{
    ThrowIfNull(pCompilerHost);
    if (pParam)
    {
        BCSYM_Param * pRet = CloneParameter(pParam, pCompilerHost);
        pRet->SetType(LiftType(pRet->GetRawType()->DigThroughAlias(), pCompilerHost));

        return pRet;
    }
    else
    {
        return NULL;
    }
}

BCSYM_Param *
Symbols::CloneParameter
(
    _In_opt_ BCSYM_Param * pParam,
    CompilerHost * pCompilerHost
)
{
    ThrowIfNull(pCompilerHost);
    if (pParam)
    {

        BCSYM_Param * pRet = AllocParameter(false, false);
        memcpy(pRet, pParam, sizeof(BCSYM_Param));
        pRet->SetNext(NULL);
        pRet->SetToken(0);
        pRet->SetComClassToken(0);
        pRet->SetAreAttributesEmitted(false);

        return pRet;
    }
    else
    {
        return NULL;
    }
}

BCSYM *
Symbols::LiftType
(
    BCSYM * pType,
    CompilerHost * pCompilerHost)
{
    if (! pType)
    {
        return NULL;
    }
    else
    {
        pType = pType->DigThroughAlias();
        if
        (
            TypeHelpers::IsNullableType(pType,pCompilerHost) ||
            TypeHelpers::IsReferenceType(pType) ||
            TypeHelpers::IsVoidType(pType) ||
            IsRestrictedType(pType, pCompilerHost)
        )
        {
            return pType;
        }
        else if (pCompilerHost->GetFXSymbolProvider()->IsTypeAvailable(FX::GenericNullableType))
        {
            return
                GetGenericBinding
                (
                    pType->IsBad(),
                    pCompilerHost->GetFXSymbolProvider()->GetType(FX::GenericNullableType),
                    &pType,
                    1,
                    NULL,
                    true
                );
        }
        else
        {
            return GetGenericBadNamedRoot();
        }
    }
}


BCSYM_LiftedOperatorMethod *
Symbols::CreateLiftedOperatorImplementation
(
    BCSYM_UserDefinedOperator * pliftedOperator,
    _In_ BCSYM_MethodDecl * pAssociatedMethodOfSource,
    CompilerHost * pCompilerHost
)
{
    BCSYM_LiftedOperatorMethod * ret = (BCSYM_LiftedOperatorMethod *)AllocSymbol(SYM_LiftedOperatorMethod, false, 0);

    {
        #if FV_TRACK_MEMORY && IDE

            unsigned long totalSize =  ret->m_totalSize;
            BackupValue<unsigned long> backup_totalSize(&(ret->m_totalSize));
        
        #endif

        #if DEBUG
        
            BackupProperty<BCSYM, BilKind, &BCSYM::SetSkKind> backup_kind(ret, ret->GetSkKind());

        #else

            BackupProperty<BCSYM, unsigned __int8, &BCSYM::SetSkKind> backup_kind(ret, ret->GetSkKind());

        #endif

        bool tmp_hasLocation = ret->HasLocation();
        bool tmp_isLocationInherited = ret->IsLocationInherited();
                
        memcpy((BCSYM_MethodDecl *)ret, (BCSYM_MethodDecl *)pAssociatedMethodOfSource, sizeof(BCSYM_MethodDecl));
        ret->SetHasLocation(tmp_hasLocation);
        ret->SetIsLocationInherited(tmp_isLocationInherited);
    }


    ret->SetHandlerList(NULL);
    ret->SetImplementsList(NULL);
    ret->SetOverriddenProcRaw(NULL);
    ret->SetOverriddenProcLastRaw(NULL);
    ret->SetGenericParams(NULL);
    ret->SetBoundTree(NULL);   
    ret->SetToken(0);
    ret->SetComClassToken(0);
    ret->SetAttributesEmitted(false);   
    ret->SetAssociatedOperatorDef(NULL);  //need to clear this before setting new value
    


    ret->SetAssociatedOperatorDef(pliftedOperator);
    ret->SetActualProc(pAssociatedMethodOfSource);
    ret->SetFirstParam(pliftedOperator->GetFirstParam());
    ret->SetReturnTypeParam(pliftedOperator->GetReturnTypeParam());
    ret->SetType(pliftedOperator->GetRawType());


    return ret;
}

/**************************************************************************************************
;GetGenericBadNamedRoot

Returns the static Generic bad named root symbol
***************************************************************************************************/
BCSYM_GenericBadNamedRoot *
Symbols::GetGenericBadNamedRoot()
{
    return &GenericBadNamedRoot;
}

/**************************************************************************************************
;GetVoidType

Returns the static void type symbol
***************************************************************************************************/
BCSYM_SimpleType *
Symbols::GetVoidType()
{
    return &GenericVoidType;
}

//****************************************************************************
// Wrappers.
//****************************************************************************

/**************************************************************************************************
;MakePtrType

Creates a Pointer symbol that points to the provided type.
Note: Use the static types if the root itself is a static type.
***************************************************************************************************/
BCSYM_PointerType *Symbols::MakePtrType
(
    BCSYM *Type // [in] The type symbol that we want to create a pointer symbol to
)
{
    // PointerSymbol to bad is still bad.
    if (Type && Type->IsBad())
    {
        return (BCSYM_PointerType *)Type;
    }

    // Create a wrapper type and stuff the pointer type in there.
    BCSYM_PointerType *NewPointerSymbol = (BCSYM_PointerType *)AllocSymbol(SYM_PointerType, Type->HasLocation(), 0);
    NewPointerSymbol->SetVtype(t_ptr);
    NewPointerSymbol->SetRoot(Type);
    if (Type && Type->HasLocation())
    {
        NewPointerSymbol->SetLocation( Type->GetLocation());
    }
    return NewPointerSymbol;
}


/**************************************************************************************************
;GetBadNamedRoot

Creates a bad named root symbol
***************************************************************************************************/
BCSYM_NamedRoot *Symbols::GetBadNamedRoot
(
    _In_opt_z_ STRING *Name, // [in] the name for this bad named root
    _In_opt_z_ STRING *Namespace, // [in] the namespace the bad named root will live in
    DECLFLAGS DeclarationFlags, // [in] whether the symbol should be Hidden
    BindspaceType BindingSpace, // [in] the binding space this symbol lives in
    ERRID ErrorId, // [in] the error to associate with this bad named root ( may be zero )
    _In_opt_z_ STRING *ErrorString, // [in] An error string that will be reported in BCSYM_NamedRoot::ReportError() if ErrorId is set ( may be NULL )
    _Inout_ SymbolList *OwningSymbolList, // [in] the symbol list to attach this bad named root to ( may be NULL )
    bool HasLocation // defaults to false.  Whether to create location information for this symbol
)
{
    // Create the symbol.
    BCSYM_NamedRoot *NewBadNamedRoot = (BCSYM_NamedRoot *)AllocSymbol( SYM_GenericBadNamedRoot, HasLocation, 0 /* no extra bytes */ );

    InitBadNamedRoot(
        NewBadNamedRoot,
        Name,
        Namespace,
        DeclarationFlags,
        BindingSpace,
        ErrorId,
        ErrorString);

    // Link it on the list.
    OwningSymbolList->AddToFront(NewBadNamedRoot);

    return NewBadNamedRoot;
}

void Symbols::InitBadNamedRoot
(
    _Out_ BCSYM_NamedRoot *NewBadNamedRoot,   // [in] the bad named root to init
    _In_opt_z_ STRING *Name, // [in] the name for this bad named root
    _In_opt_z_ STRING *Namespace, // [in] the namespace the bad named root will live in
    DECLFLAGS DeclarationFlags, // [in] whether the symbol should be Hidden
    BindspaceType BindingSpace, // [in] the binding space this symbol lives in
    ERRID ErrorId, // [in] the error to associate with this bad named root ( may be zero )
    _In_opt_z_ STRING *ErrorString // [in] An error string that will be reported in BCSYM_NamedRoot::ReportError() if ErrorId is set ( may be NULL )
)
{
    // Fill it in.
    NewBadNamedRoot->SetBadName(Name);
    NewBadNamedRoot->SetName(Name);
    NewBadNamedRoot->SetEmittedName(Name);
    NewBadNamedRoot->SetBadNameSpace(Namespace);
    NewBadNamedRoot->SetNameSpace(Namespace);
    NewBadNamedRoot->SetBadExtra(ErrorString);
    NewBadNamedRoot->SetAccess(AccessOfFlags(DeclarationFlags));
    NewBadNamedRoot->SetIsHidden(!!(DeclarationFlags & DECLF_Hidden));
    NewBadNamedRoot->SetBindingSpace(BindingSpace);
    NewBadNamedRoot->SetIsBadRaw(true);
    NewBadNamedRoot->SetStateForBadness(CS_NoState);
    NewBadNamedRoot->SetErrid(ErrorId);
}

/**************************************************************************************************
;GetNamedType

Creates a NamedType symbol
***************************************************************************************************/
BCSYM_NamedType *Symbols::GetNamedType
(
    Location *LocationInfo, // [in] location information for the named type being built
    BCSYM_NamedRoot *Context, // [in] the context in which the named type is defined ( like the class it is defined in, etc. )
    unsigned NumDotDelimitedNames, // [in] If the name of the named type is Bob.Foo this would be 2
    _Inout_ _Deref_prepost_opt_valid_ BCSYM_NamedType **NamedTypeList // [optional out] If provided, the built type is put at the head of this list
)
{
    // Create the symbol.
    BCSYM_NamedType *NewNamedType = NULL;

    if (VBMath::TryMultiply(NumDotDelimitedNames, sizeof(void *)))
    {
        NewNamedType = (BCSYM_NamedType *)AllocSymbol( 
                            SYM_NamedType, 
                            LocationInfo != NULL && LocationInfo->IsValid(), 
                            VBMath::Multiply(NumDotDelimitedNames, sizeof(void *)));
    }

    // Fill it in.
    NewNamedType->SetNameArray(NewNamedType->GetFirstDotDelimitedNamesPtr());
    NewNamedType->SetNameCount(NumDotDelimitedNames);
    NewNamedType->SetContext(Context);
    NewNamedType->SetIsGlobalNameSpaceBased(false); // not known yet, set by default to false

    if ( LocationInfo && LocationInfo->IsValid())
    {
        NewNamedType->SetLocation( LocationInfo );
    }

    // Tack it on the head of the list
    if ( NamedTypeList )
    {
        NewNamedType->SetNext(*NamedTypeList);
        *NamedTypeList = NewNamedType;
    }
    return NewNamedType;
}


/**************************************************************************************************
;GetArrayType

Builds an Array Type symbol
***************************************************************************************************/
BCSYM_ArrayType * // The built array symbol
Symbols::GetArrayType
(
    unsigned Rank, // [in] the number of dimensions
    BCSYM *ElementType // [in] the type of the array
)
{
    VSASSERT( !ElementType->IsPointerType(), "Can't have an array type that is a pointer.");

    BCSYM_ArrayType *ArrayType = (BCSYM_ArrayType *)AllocSymbol( SYM_ArrayType, false /* no location */, 0 /* no extra bytes */ );

    // Stuff in the values.
    ArrayType->SetRank(Rank);
    ArrayType->SetVtype(t_array);
    ArrayType->SetElementType(ElementType);

    return ArrayType;
}


BCSYM_ArrayLiteralType *
Symbols::GetArrayLiteralType
(
    unsigned rank,
    BCSYM * pElementType,
    ConstIterator<ILTree::Expression *> &elements,
    const Location & literalLoc
)
{
    ThrowIfNull(pElementType);
    VSASSERT( !pElementType->IsPointerType(), "Can't have an array type that is a pointer.");

    BCSYM_ArrayLiteralType * pRet = 
        (BCSYM_ArrayLiteralType *)
        AllocSymbol
        (
            SYM_ArrayLiteralType, 
            false, //no location
            0 // no extra bytes
        );

    // Stuff in the values.
    pRet->SetRank(rank);
    pRet->SetVtype(t_array);
    pRet->SetElementType(pElementType);
    pRet->SetLiteralLocation(literalLoc);

    while (elements.MoveNext())
    {
        pRet->AddElement(elements.Current());
    }

    return pRet;
}

/**************************************************************************************************
;GetConstantExpression

Creates a constant expression symbol
***************************************************************************************************/
BCSYM_Expression *Symbols::GetConstantExpression
(
    Location *LocationInfo, // [in] location information for the new constant expression symbol
    BCSYM_NamedRoot *Context, // [in] where this symbol was defined (referring declaration)
    _Inout_opt_ _Deref_prepost_opt_valid_ BCSYM_Expression **ConstExpressionListHead,  // [in/out] the head of the list
    _Inout_opt_ _Deref_prepost_opt_valid_ BCSYM_Expression **ConstExpressionListTail, // [in/out] the tail of the list - the new expression symbol is added to the end
    _In_opt_z_ STRING *EnumElementName, // [in] a variable name to substitute in fron of ExpressionString - used by Declared::MakeEnum() to build an expression like ("Blue+1") (may be NULL)
    _In_opt_count_(ExpressionStringLength+1) _Pre_z_ const WCHAR *ExpressionString, // [in] the text of the constant expression (may be NULL - appended to EnumElementName )
    size_t ExpressionStringLength // [in] number of characters in ExpressionString ( send -1 to have this calculated )
)
{
    SafeInt<size_t> ExtraBytes = 0;

    // An expression string requires more space.  Figure out how much.
    if ( ExpressionString )
    {
        if ( ExpressionStringLength == -1 ) // do we need to figure it out?
        {
            ExpressionStringLength = wcslen(ExpressionString);
        }

        IfFalseThrow(VBMath::TryMultiply(ExpressionStringLength, sizeof(WCHAR)));

        // Count the characters we need.
        if ( EnumElementName )
        {
            ExtraBytes = StringPool::StringLength( EnumElementName ) + 1;
        }

        ExtraBytes += ExpressionStringLength;
        ExtraBytes++;

        ExtraBytes *= sizeof(WCHAR); // Convert to bytes.
    }

    VSASSERT( LocationInfo, "Must have a location" );

    // Create the symbol.
    BCSYM_Expression *NewExpressionSymbol = (BCSYM_Expression *)AllocSymbol(SYM_Expression, LocationInfo->IsValid(), ExtraBytes.Value());

    // Fill it in.
    NewExpressionSymbol->SetReferringDeclaration(Context);

    if (LocationInfo->IsValid())
    {
        NewExpressionSymbol->SetLocation(LocationInfo);
    }

    // The legacy philosphy was that we would never call constructors - that a symbol allocator should do all the
    // work of setting field defaults.  Since this symbol won't ever get New'd, we need to do the work the embedded
    // ConstantValue object (m_Value) constructor would do. Bizarrely, in DEBUG the symbol does get 'new'd in AllocSymbol()
    // so this problem   #310842
    ConstantValue cur_value = NewExpressionSymbol->GetValue();
    cur_value.TypeCode = t_bad;
    cur_value.Mask.First = 0;
    cur_value.Mask.Second = 0;
    NewExpressionSymbol->SetValue(cur_value);

    // Fill in the contant expression text
    if (ExpressionString)
    {
        WCHAR *ExpressionText = NewExpressionSymbol->GetExpressionTextRaw();

        // If they provided an element name, substitute it in the expression.  This is so we can generate a string
        // like "Blue+1" where "Blue" is the EnumElementName and "+1" is the ExpressionText
        if ( EnumElementName )
        {
            size_t EnumElementNameLength = StringPool::StringLength( EnumElementName );
#pragma prefast(suppress: 26018, "StringPool::StringLength does bound the string by its size")
            memcpy(ExpressionText, EnumElementName, EnumElementNameLength * sizeof(WCHAR));
            ExpressionText += EnumElementNameLength;
            *ExpressionText++ = WIDE(' ');
        }
        memcpy(ExpressionText, ExpressionString, ExpressionStringLength * sizeof(WCHAR));
        ExpressionText[ ExpressionStringLength ] = 0;
    }

    // Link the new expression symbol on the end of the list.
    if ( ConstExpressionListHead && ConstExpressionListTail )
    {
        if ( !*ConstExpressionListHead )
        {
            *ConstExpressionListHead = *ConstExpressionListTail = NewExpressionSymbol;
        }
        else
        {
            (*ConstExpressionListTail)->SetNext(NewExpressionSymbol);
            *ConstExpressionListTail = NewExpressionSymbol;
        }
    }
    return NewExpressionSymbol;
}

/**************************************************************************************************
;GetFixedExpression

Creates an expression symbol that represents a fixed value
***************************************************************************************************/
BCSYM_Expression *Symbols::GetFixedExpression
(
    _In_ ConstantValue *Value // [in] the value the created expression will represent
)
{
    // Create the symbol.
    BCSYM_Expression *NewExpressionSymbol = (BCSYM_Expression *)AllocSymbol(SYM_Expression, false, 0);

    if (Value->TypeCode == t_string && Value->String.Spelling)
    {
        // Copy the spelling so that it lives as long as the symbol.
        WCHAR *Spelling = (WCHAR *)m_Allocator->Alloc((Value->String.LengthInCharacters + 1) * sizeof(WCHAR));
        VSASSERT(wcslen(Value->String.Spelling) <= Value->String.LengthInCharacters, "Invalid state");
        memcpy(Spelling, Value->String.Spelling, (Value->String.LengthInCharacters + 1) * sizeof(WCHAR));
        Value->String.Spelling = Spelling;
    }

    // Save the value.
    NewExpressionSymbol->SetValue(*Value);
    NewExpressionSymbol->SetIsEvaluated(true);

    return NewExpressionSymbol;
}

/**************************************************************************************************
;GetAlias

Creates an Alias for a symbol.  The reason we have Aliases is so that we can have a symbol in
multiple symbol tables.  The reason we have to have an Alias to do it is because of a bad design
decision which uses the Next pointer inside of a symbol to hook up to the other symbols in a hash.
What should have happened was that a Hash table should have provided that mechanism by wrapping
things that get put in a hash but it doesnt, so to avoid hammering the next pointer and unlinking
a symbol out of a table, we create this wrapper here instead and this wrapper can then be put in
a hash.
***************************************************************************************************/
BCSYM_Alias *Symbols::GetAlias
(
    _In_z_ STRING *AliasName, // [in] the name for this alias
    DECLFLAGS AliasDeclFlags, // [in] the flags for the alias
    BCSYM *AliasedSymbol, // [in] the symbol being aliased
    _Inout_ SymbolList *ListOfSymbols, // [in] If provided, the built alias is attatched to this list ( may be null )
    bool DigThroughAliases // [in] This should only be set to false, when constructing the merged hash to speed up compilation.
)
{
    return GetAlias(NULL, AliasName, AliasDeclFlags, AliasedSymbol, ListOfSymbols, DigThroughAliases);
}

/**************************************************************************************************
;GetAlias

Creates an Alias for a symbol.  The reason we have Aliases is so that we can have a symbol in
multiple symbol tables.  The reason we have to have an Alias to do it is because of a bad design
decision which uses the Next pointer inside of a symbol to hook up to the other symbols in a hash.
What should have happened was that a Hash table should have provided that mechanism by wrapping
things that get put in a hash but it doesnt, so to avoid hammering the next pointer and unlinking
a symbol out of a table, we create this wrapper here instead and this wrapper can then be put in
a hash.
***************************************************************************************************/
BCSYM_Alias *Symbols::GetAlias
(
    Location *pLoc,  // [in] optional location for this alias
    _In_z_ STRING *AliasName, // [in] the name for this alias
    DECLFLAGS AliasDeclFlags, // [in] the flags for the alias
    BCSYM *AliasedSymbol, // [in] the symbol being aliased
    _Inout_ SymbolList *ListOfSymbols, // [in] If provided, the built alias is attatched to this list ( may be null )
    bool DigThroughAliases // [in] This should only be set to false, when constructing the merged hash to speed up compilation.
)
{
    if (DigThroughAliases)
    {
        while (AliasedSymbol->IsAlias())
        {
            AliasedSymbol = AliasedSymbol->PAlias()->GetAliasedSymbol();
        }
    }

    // Create the symbol.
    BCSYM_Alias *NewAliasSymbol = (BCSYM_Alias *)AllocSymbol(SYM_Alias, pLoc != NULL, 0);

    // Fill it in.
    NewAliasSymbol->SetName(AliasName);
    NewAliasSymbol->SetEmittedName(AliasName);
    NewAliasSymbol->SetAccess(AccessOfFlags( AliasDeclFlags ));
    NewAliasSymbol->SetIsHidden(!!(AliasDeclFlags & DECLF_Hidden));
    NewAliasSymbol->SetAliasedSymbol(AliasedSymbol);

    if ( pLoc )
    {
        NewAliasSymbol->SetLocation( pLoc );
    }

    // Set the bindingspace of the alias to whatever is the bindingspace it points to.
    if (AliasedSymbol->IsNamedRoot())
    {
        NewAliasSymbol->SetBindingSpace(AliasedSymbol->PNamedRoot()->GetBindingSpace());
    }
    else if (AliasedSymbol->IsSimpleType())
    {
        NewAliasSymbol->SetBindingSpace(BINDSPACE_Type);
    }
    else
    {
        VSFAIL("Can't alias to this symbol type.");
    }

    ListOfSymbols->AddToFront(NewAliasSymbol); // Link it on the list.
    return NewAliasSymbol;
}

/**************************************************************************************************
;GetAliasOfSymbol

Creates an alias from an existing symbol.
***************************************************************************************************/
BCSYM_Alias *Symbols::GetAliasOfSymbol
(
    BCSYM_NamedRoot *SymbolToAlias, // [in] the symbol you want to create an alias for
    ACCESS Access, // [in] the access you want the alias symbol to have
    _Inout_ SymbolList *ListOfSymbols, // [in] the list to append the alias to ( may be null )
    bool DigThroughAliases // [in] This should only be set to false, when constructing the merged hash to speed up compilation.
)
{
    // Set up the decl flags.
    DECLFLAGS AccessFlags = DECLF_Hidden;

    if (Access == ACCESS_Public)
    {
        AccessFlags |= DECLF_Public;
    }
    else if (Access == ACCESS_Friend)
    {
        AccessFlags |= DECLF_Friend;
    }
    else if (Access == ACCESS_Protected)
    {
        AccessFlags |= DECLF_Protected;
    }
    else if (Access == ACCESS_ProtectedFriend)
    {
        AccessFlags |= DECLF_ProtectedFriend;
    }
    else if (Access == ACCESS_CompilerControlled)
    {
        AccessFlags |= DECLF_CompilerControlled;
    }
    else
    {
        AccessFlags |= DECLF_Private;
    }

    // Create an alias that points to this type.
    return GetAlias(SymbolToAlias->GetName(), AccessFlags, SymbolToAlias, ListOfSymbols, DigThroughAliases);
}

/**************************************************************************************************
;GetImplements

Creates a symbol for an Implements statement
***************************************************************************************************/
void Symbols::GetImplements
(
    Location *LocationInfo, // [in] the location information for the implements symbol (may be NULL)
    _In_opt_z_ STRING *pstrName, // [in] name for the implements symbol
    BCSYM *psym, // [in] the symbol that is being implemented
    _Inout_ _Deref_prepost_valid_ BCSYM_Implements **ImplementsList // [in/out] the created implements symbol is put on the end of this list (may be NULL)
)
{
    // Create the symbol.
    BCSYM_Implements *NewImplementsSymbol = (BCSYM_Implements *)AllocSymbol(SYM_Implements, LocationInfo != NULL && LocationInfo->IsValid(), 0);

    // Fill it in.
    NewImplementsSymbol->SetName(pstrName);
    NewImplementsSymbol->SetRawRoot(psym);

    if (LocationInfo && LocationInfo->IsValid())
    {
        NewImplementsSymbol->SetLocation(LocationInfo);
    }

    // Link it on the list at the end
    if (*ImplementsList)
    {
        BCSYM_Implements *CurrentImplements = *ImplementsList;

        while (CurrentImplements->GetNext())
        {
            CurrentImplements = CurrentImplements->GetNext();
        }
        CurrentImplements->SetNext(NewImplementsSymbol);
    }
    else
    {
        *ImplementsList = NewImplementsSymbol;
    }
}

/**************************************************************************************************
;GetVariable

Fill out a pre-allocated variable symbol
***************************************************************************************************/
void Symbols::GetVariable
(
    const Location *LocationInfo, // [in] location in the text for the symbol being built (may be null)
    _In_z_ STRING *VariableName, // [in] the name for this variable symbol
    _In_z_ STRING *EmittedName, // [in] the emitted name for this variable symbol
    DECLFLAGS VarDeclFlags, // [in] the flags that describe the access, shared, static, etc. nature of this variable
    VARIABLEKIND VariableKind, // [in] what kind of variable this is (local, param, withevents, etc.)
    BCSYM *VariableType, // [in] the type of this variable
    BCSYM_Expression *InitializerExpression, // [in] Initializer value (may be null)
    _Inout_ SymbolList *ListOfSymbols, // [in] built variable put on this list if provided (may not be null)
    _Inout_ BCSYM_Variable *VariableSymbol // [in] the pre-allocated variable symbol
)
{
    VSASSERT(VariableName && StringPool::StringLength(VariableName), "Must have a name.");

    // Fill in variable stuff.
    VariableSymbol->SetName(VariableName);
    VariableSymbol->SetEmittedName(EmittedName);
    VariableSymbol->SetIsBracketed(!!(VarDeclFlags & DECLF_Bracketed));
    VariableSymbol->SetAccess(AccessOfFlags(VarDeclFlags));
    VariableSymbol->SetIsHidden(!!(VarDeclFlags & DECLF_Hidden));
    VariableSymbol->SetIsSpecialName(!!(VarDeclFlags & DECLF_SpecialName));
    VariableSymbol->SetType(VariableType);
    VariableSymbol->SetStatic(VarDeclFlags & DECLF_Static);
    VariableSymbol->SetIsShared(!!(VarDeclFlags & DECLF_Shared));
    VariableSymbol->SetReadOnly(VarDeclFlags & (DECLF_ReadOnly | DECLF_LocalInUsingReadOnly) );
    VariableSymbol->SetShadowsKeywordUsed(!!(VarDeclFlags & DECLF_ShadowsKeywordUsed));
    VariableSymbol->SetNew(VarDeclFlags & DECLF_New);
    VariableSymbol->SetExplicitlyTyped (!(VarDeclFlags & DECLF_NotTyped));
    VariableSymbol->SetImplicitDecl(VarDeclFlags & DECLF_NotDecled);
    VariableSymbol->SetIsWithEvents(VarDeclFlags & DECLF_WithEvents);
    VariableSymbol->SetVarkind(VariableKind);
    VariableSymbol->SetIsMyGenerated(!!(VarDeclFlags & DECLF_MyGroupGenerated));
    VariableSymbol ->SetIsAutoPropertyBackingField(!!(VarDeclFlags & DECLF_AutoProperty));

    if (VariableSymbol->GetVarkind() == VAR_WithEvents)
    {
        // The compiler will not bind to WithEvents variables.  We do this so the WithEvents variable won't collide
        // with the property by the same name that we create for WithEvent vars which does the auto-event hookup
        VariableSymbol->SetBindingSpace(BINDSPACE_IgnoreSymbol);
    }
    else
    {
        VariableSymbol->SetBindingSpace(BINDSPACE_Normal);
    }

    if (LocationInfo)
    {
        VariableSymbol->SetLocation(LocationInfo);
    }

    // Fill in the initial value if available
    if (InitializerExpression)
    {
        if ( VariableSymbol->IsVariableWithArraySizes())
        {
            ((BCSYM_VariableWithValue*)VariableSymbol->PVariableWithArraySizes())->SetExpression(InitializerExpression);
        }
        else
        {
            ((BCSYM_VariableWithValue*)VariableSymbol->PVariableWithValue())->SetExpression(InitializerExpression);
        }
    }

    // Link it on the list.
    ListOfSymbols->AddToFront(VariableSymbol);
}

/**************************************************************************************************
;GetProc

Fill out a pre-allocated Procedure symbol
***************************************************************************************************/
void Symbols::GetProc
(
    Location *LocationInfo, // [in] location in text from which this symbol is generated (may be NULL)
    _In_z_ STRING *ProcName, // [in] name for this procedure symbol
    _In_z_ STRING *EmittedName, // [in] name that should be emitted to com+ for this proc
    CodeBlockLocation *CodeBlockLoc, // [in] location of the code
    CodeBlockLocation *ProcBlockLocation, // [in] location of the proc
    DECLFLAGS ProcFlags, // [in] access and keyword usage ( overloads, etc. ) for this proc
    BCSYM *ReturnType, // [in] for FUNCTIONS, the return type.  NULL for PROCEDUREs
    BCSYM_Param *ParameterList, // [in] the list of parameter symbols for the proc
    BCSYM_Param *ReturnParameter, // [in] the param for return type, if there are attributes, NULL otherwise
    _In_opt_z_ STRING *LibraryName, // [in] the libary name
    _In_opt_z_ STRING *AliasName, // [in] the name for the alias to this symbol
    SyntheticKind SynthKind, // [in] if this is a synthetic method, identifies which kind
    _Inout_ BCSYM_GenericParam *ListOfGenericParameters, // [in] type parameters of the method
    _Inout_ SymbolList *ListOfSymbols, // [in] the proc is added to this list if provided (may be null)
    _Inout_ BCSYM_Proc *Procedure // [in] the pre-allocated procedure symbol that we are to fill in
)
{
    VSASSERT(ProcName && StringPool::StringLength(ProcName), "Must have a name.");

    // Fill it in.
    Procedure->SetName(ProcName);

    // Change the name of constructors from "new" to be ".ctor" to prevent collisions with "[new]"
    if ( ProcFlags & DECLF_Constructor )
    {
        Procedure->SetName(( ProcFlags & DECLF_Shared ?
                                     STRING_CONST( m_CompilerInstance, SharedConstructor ) :
                                     STRING_CONST( m_CompilerInstance, Constructor )));
    }

    Procedure->SetEmittedName(EmittedName);
    Procedure->SetBindingSpace(BINDSPACE_Normal);
    Procedure->SetAccess(AccessOfFlags(ProcFlags));
    Procedure->SetIsBracketed(!!(ProcFlags & DECLF_Bracketed));
    Procedure->SetIsProcedure(!!(ProcFlags & DECLF_Function));
    Procedure->SetIsPropertyGet(!!(ProcFlags & DECLF_PropGet));
    Procedure->SetIsPropertySet(!!(ProcFlags & DECLF_PropSet));
    Procedure->SetIsHidden(!!(ProcFlags & DECLF_Hidden));
    Procedure->SetIsSpecialName(!!(ProcFlags & DECLF_SpecialName));
    Procedure->SetIsMyGenerated(!!(ProcFlags & DECLF_MyGroupGenerated));
    Procedure->SetOverloadsKeywordUsed(!!(ProcFlags & DECLF_OverloadsKeywordUsed));
    Procedure->SetOverridesKeywordUsed (!!(ProcFlags & DECLF_OverridesKeywordUsed));
    Procedure->SetOverridableKeywordUsed(!!(ProcFlags & DECLF_Overridable));
    Procedure->SetNotOverridableKeywordUsed(!!(ProcFlags & DECLF_NotOverridable));
    Procedure->SetMustOverrideKeywordUsed(!!(ProcFlags & DECLF_MustOverride));
    Procedure->SetIsAnyConstructor(!!(ProcFlags & DECLF_Constructor));
    Procedure->SetType(ReturnType);
    Procedure->SetIsShared(!!(ProcFlags & DECLF_Shared));
    Procedure->SetIsImplementing(!!(ProcFlags & DECLF_Implementing));
    Procedure->SetShadowsKeywordUsed(!!(ProcFlags & DECLF_ShadowsKeywordUsed));
    Procedure->SetDoesntRequireImplementation(!!(ProcFlags & DECLF_ImplementionNotRequired));
    Procedure->SetFirstParam(ParameterList);
    Procedure->SetReturnTypeParam(ReturnParameter);
    Procedure->SetIsEventAccessor(!!(ProcFlags & (DECLF_EventAddMethod | DECLF_EventRemoveMethod | DECLF_EventFireMethod)));
    Procedure->SetIsPartialMethodDeclaration(!!(ProcFlags & DECLF_Partial));
    Procedure->SetCheckAccessOnOverride(true);
    Procedure->SetAsyncKeywordUsed(!!(ProcFlags & DECLF_Async));
    Procedure->SetIteratorKeywordUsed(!!(ProcFlags & DECLF_Iterator));

    if (LocationInfo)
    {
        Procedure->SetLocation(LocationInfo);
    }

    // Handle any per-proc attributes.
    if (Procedure->IsMethodImpl())
    {
        BCSYM_MethodImpl* MethodImpl = Procedure->PMethodImpl();

        if (CodeBlockLoc)
        {
            MethodImpl->SetCodeBlock( CodeBlockLoc );
        }
        if (ProcBlockLocation)
        {
            MethodImpl->SetProcBlock( ProcBlockLocation );
        }
    }
    else if (Procedure->IsDllDeclare())
    {
        BCSYM_DllDeclare *DllDeclare = Procedure->PDllDeclare();

        DllDeclare->SetLibName(LibraryName);
        DllDeclare->SetAliasName(AliasName);
        if (!!(ProcFlags & DECLF_Unicode))
        {
            DllDeclare->SetDeclareType(DECLARE_Unicode);
        }
        else if (!!(ProcFlags & DECLF_Ansi))
        {
            DllDeclare->SetDeclareType(DECLARE_Ansi);
        }
        else if (!!(ProcFlags & DECLF_Auto))
        {
            DllDeclare->SetDeclareType(DECLARE_Auto);
        }
    }
    else if (Procedure->IsSyntheticMethod())
    {
        Procedure->PSyntheticMethod()->SetSyntheticKind(SynthKind);
    }

    SetGenericParams(ListOfGenericParameters, Procedure);

    // Link it on the list.
    ListOfSymbols->AddToFront(Procedure);
}

/**************************************************************************************************
;GetParam

Allocates and defines a parameter symbol
***************************************************************************************************/

BCSYM_Param *Symbols::GetParam
(
    Location *LocationInfo, // [in] location in text from which this symbol is generated (may be NULL)
    _In_z_ STRING *ParamName, // [in] name of the parameter
    BCSYM *ParamType, // [in] type for the parameter
    PARAMFLAGS ParamFlags, // [in] flags describing the parameter
    BCSYM_Expression *InitialValueExpr, // [in] initializer value for the parameter (may be NULL)
    _Inout_opt_ BCSYM_Param **ParamListHead, // [in/out] head of the list parameters are added to. (parms added to end of list)
    _Inout_opt_ BCSYM_Param **ParamListTail, // [in/out] tail of the list parameters are added to.
    bool IsReturnType // [in] does this param represent the return type of a function?
)
{
    // Make sure no own added a new PARAMF_* flag with out updating Symbols::CalculateInitialParamFlags
    VSASSERT(0 == (ParamFlags & (~(PARAMF_ByVal | PARAMF_ByRef | PARAMF_ParamArray | PARAMF_Optional | PARAMF_MarshalAsObject))), "Updated PARAMFFlAGS without updating Symbols::GetParam() and Symbols::CalculateInitialFlags");
    VSASSERT(sizeof(BCSYM_Param) == sizeof(BCSYM_ParamWithValue), "BCSYM_Param & BCSYM_ParamWithValue have to be the same size!");

    // Create the symbol.
    BilKind bk = InitialValueExpr ? SYM_ParamWithValue : SYM_Param;
    BCSYM_Param *NewParam = (BCSYM_Param *)AllocSymbol(bk, LocationInfo != NULL && LocationInfo->IsValid() , 0);

    // Fill it in.
    NewParam->SetName(ParamName);
    NewParam->SetType(ParamType);
    NewParam->SetIsParamArray(ParamFlags & PARAMF_ParamArray);
    NewParam->SetIsOptional(ParamFlags & PARAMF_Optional);
    NewParam->SetIsMarshaledAsObject(ParamFlags & PARAMF_MarshalAsObject);
    NewParam->SetIsByRefKeywordUsed(ParamFlags & PARAMF_ByRef);
    NewParam->SetIsOptionCompare(false);
    NewParam->SetIsReturnType(IsReturnType);

    if (InitialValueExpr)
    {
        NewParam->PParamWithValue()->SetExpression(InitialValueExpr);
    }

    if (LocationInfo && LocationInfo->IsValid())
    {
        NewParam->SetLocation(LocationInfo);
    }

    // If we don't have the last element in the list, just link this onto the front of the list.
    if (!ParamListTail)
    {
        if (ParamListHead)
        {
            NewParam->SetNext(*ParamListHead);
            *ParamListHead = NewParam;
        }
    }
    else
    {
        if (!*ParamListHead)
        {
            *ParamListHead = *ParamListTail = NewParam;
        }
        else
        {
            (*ParamListTail)->SetNext(NewParam);
            *ParamListTail = NewParam;
        }
    }

    return NewParam;
}

/**************************************************************************************************
;SetClassAsStandardModule

Marks a class symbol as being a Standard Module.
***************************************************************************************************/
void Symbols::SetClassAsStandardModule
(
    _Out_ BCSYM_Class *Class // [in] class to mark as being a standard module
)
{
    Class->SetIsStdModule(true);
}

/**************************************************************************************************
;GetClass

Fills out a preallocated Class symbol
***************************************************************************************************/
void Symbols::GetClass
(
    Location *LocationInfo, // [in] location in text from which this symbol is generated (may be NULL)
    _In_z_ STRING *ClassName, // [in] name of the class
    _In_z_ STRING *ClassEmittedName, // [in] the name we emit to com+
    _In_z_ STRING *NamespaceName, // [in] the name of the namespace the class is defined in
    CompilerFile *OwningFile, // [in] the file where this class definition came from
    BCSYM *BaseClass, // [in] the base class that this class extends
    BCSYM_Implements *ImplementsList, // [in] the list of interfaces this class claims to implement
    DECLFLAGS DeclFlags, // [in] declaration flags
    Vtypes UnderlyingType, // [in] underlying type for enum
    _Out_opt_ BCSYM_Variable *pvarMe, // [in] storage for the Me variable, if one is desired
    SymbolList *ListOfClassMembers, // [in] the list of class member symbols that will be put in the hash table for the class
    SymbolList *ListOfUnBindableClassMembers, // [in] the list of class member symbols that cannot be bound to, will be put in a hash table fore the class
    _Inout_ BCSYM_GenericParam *ListOfGenericParameters, // [in] the list of type parameter symbols
    _Inout_ SymbolList *ListOfSymbols, // [in/out] this class will be added to this list (may be null)
    _Inout_ BCSYM_Class *ClassSymbol // [in] the class symbol that is being filled out
)
{
    VSASSERT( ClassName && StringPool::StringLength(ClassName), "Must have a name." );
    VSASSERT( NamespaceName, "Must have a namespace." );
    VSASSERT( DeclFlags & DECLF_Enum || UnderlyingType == t_bad, "why are you setting the type?" );
    VSASSERT( !(DeclFlags & DECLF_Enum) || UnderlyingType != t_bad, "what's the type?");

#if DEBUG
    if (NamespaceName)
    {
        WCHAR *wszUnqualName = wcsrchr(NamespaceName, L'.');
        VSASSERT(!wszUnqualName || wcslen(wszUnqualName + 1), "Can't have an empty compound namespace name!");
    }
#endif DEBUG

    // Fill it in.
    ClassSymbol->SetIsStruct(!!(DeclFlags & DECLF_Structure));
    ClassSymbol->SetIsStdModule(!!(DeclFlags & DECLF_Module));
    ClassSymbol->SetIsDelegate(!!(DeclFlags & DECLF_Delegate));
    if (DeclFlags & DECLF_Enum)
    {
        ClassSymbol->SetIsEnum(!!(DeclFlags & DECLF_Enum));
        ClassSymbol->SetVtype(UnderlyingType);
    }
    else
    {
        ClassSymbol->SetVtype(t_bad);
    }
    ClassSymbol->SetName(ClassName);
    ClassSymbol->SetEmittedName(ClassEmittedName);
    ClassSymbol->SetBindingSpace(BINDSPACE_Type);
    ClassSymbol->SetNameSpace(NamespaceName);
    ClassSymbol->SetAccess(AccessOfFlags(DeclFlags));
    ClassSymbol->SetCompilerFile(OwningFile);
    ClassSymbol->SetRawBase(BaseClass);
    ClassSymbol->SetIsHidden(!!(DeclFlags & (DECLF_Hidden | DECLF_Anonymous)));
    ClassSymbol->SetImpList(ImplementsList);
    ClassSymbol->SetShadowsKeywordUsed(!!(DeclFlags & DECLF_ShadowsKeywordUsed));
    // The m_AllMustOverridesSatisfied situation won't be fully known until bindable::DetermineIfConcreteClass() where the inheritance tree is traced
    // but we need some information right now and this at least catches the obvious case where the class is clearly abstract ( it has MustOverride methods in it )
    ClassSymbol->SetMustOverridesSatisifed(!(DeclFlags & DECLF_HasMustOverrideMethod));
    ClassSymbol->SetNotInheritable(!!(DeclFlags & DECLF_NotInheritable));
    ClassSymbol->SetMustInherit(!!(DeclFlags & DECLF_MustInherit)); // Microsoft: Hack Alert!  CLASSF_MustInherit == DECLF_BaseOnly or this wouldn't work.  I'm forced
    // down this primrose path by the existing design (this is apparently being done on access flags, as well).  I will clean this up when I get the chance to fix it for
    // all of Declared.
    ClassSymbol->SetIsPartialType(!!(DeclFlags & DECLF_Partial ));
    ClassSymbol->SetPartialKeywordUsed(!!(DeclFlags & DECLF_Partial ));
    ClassSymbol->SetIsAnonymous(!!(DeclFlags & DECLF_Anonymous));

    // If the symbol has one, set up "me".
    if (pvarMe)
    {
        pvarMe->SetSkKind(SYM_Variable);
        GetVariable(NULL, 
                    m_CompilerInstance->TokenToString(tkME), 
                    m_CompilerInstance->TokenToString(tkME), 
                    DECLF_Public | DECLF_Dim, 
                    VAR_Param,
                    ClassSymbol,
                    NULL,
                    NULL,
                    pvarMe);
        pvarMe->SetIsMe(true);
        ClassSymbol->SetMe(pvarMe);
    }

    // Set the location.
    if (LocationInfo)
    {
        ClassSymbol->SetLocation(LocationInfo);
    }

    // Add the class members to the hash for the class.
    BCSYM_Hash *BindableChildren = NULL;
    if (ListOfClassMembers)
    {
        BindableChildren = GetHashTable(ClassSymbol->GetNameSpace(), ClassSymbol, true, 0, ListOfClassMembers);
    }

    BCSYM_Hash *UnBindableChildren = NULL;

    if (ListOfUnBindableClassMembers)
    {
        UnBindableChildren =
            GetHashTable(
                ClassSymbol->GetNameSpace(),
                    ClassSymbol,
                    true,
                    0,
                    ListOfUnBindableClassMembers);
    }

    SetGenericParams(ListOfGenericParameters, ClassSymbol);

    ClassSymbol->SetHashes(BindableChildren, UnBindableChildren);

    // Link the class on the list of built symbols.
    ListOfSymbols->AddToFront(ClassSymbol);
}


//============================================================================
// Create an applied attribute symbol.
//============================================================================

BCSYM_ApplAttr *Symbols::GetApplAttr(BCSYM_NamedType * pAttributeName,
                                     Location * ploc,
                                     bool IsAssembly,
                                     bool IsModule)
{
    BCSYM_ApplAttr * papplattr;

    // Allocate the symbol.
    papplattr = (BCSYM_ApplAttr *)AllocSymbol( SYM_ApplAttr, ploc ? true : false, 0);

    papplattr->SetAttrClass(pAttributeName);
    papplattr->SetIsAssembly(IsAssembly);
    papplattr->SetIsModule(IsModule);
    if (ploc)
    {
        papplattr->SetLocation(ploc);
    }

    return papplattr;
}

//============================================================================
// Create a module-level conditional compilation constants scope.
//============================================================================

void Symbols::GetConditionalCompilationScope
(
    _Out_ BCSYM_Container *pcont
)
{
    pcont->SetBindingSpace(BINDSPACE_Normal);
    pcont->SetHash(GetHashTable(NULL, pcont, true, 16, NULL));
}

// Assign a list of generic parameters to an owner.

void
Symbols::SetGenericParams
(
    _Out_opt_ BCSYM_GenericParam *ListOfGenericParameters,
    BCSYM_NamedRoot *Owner
)
{
    if (ListOfGenericParameters)
    {
        GenericParams *GenericParameters = NULL;

        if (Owner->IsClass())
        {
            GenericParameters = Owner->PClass()->GetGenericParams();
        }
        else if (Owner->IsInterface())
        {
            GenericParameters = Owner->PInterface()->GetGenericParams();
        }
        else if (Owner->IsProc())
        {
            GenericParameters = Owner->PProc()->GetGenericParams();
        }

        // Set this only if not already set
        //
        if (!GenericParameters)
        {
            GenericParameters = (GenericParams *)m_Allocator->Alloc(sizeof(GenericParams));

            SymbolList GenericParamsSymbolList;
            for (BCSYM_GenericParam *GenericParam = ListOfGenericParameters; GenericParam; GenericParam = GenericParam->GetNextParam())
            {
                GenericParamsSymbolList.AddToFront(GenericParam);
            }

            GenericParameters->m_GenericParamsHash = GetHashTable(Owner->GetNameSpace(), Owner, true, 0, &GenericParamsSymbolList);
            GenericParameters->m_FirstGenericParam = ListOfGenericParameters;

            if (Owner->IsClass())
            {
                 Owner->PClass()->SetGenericParams(GenericParameters);
            }
            else if (Owner->IsInterface())
            {
                 Owner->PInterface()->SetGenericParams(GenericParameters);
            }
            else if (Owner->IsProc())
            {
                 Owner->PProc()->SetGenericParams(GenericParameters);
            }
        }
    }
}



/**************************************************************************************************
;GetInterface

Fill out a Interface symbol.  If you provide preallocated memory, we fill out that memory as an
Interface.  If you don't, the memory is allocated for you.
***************************************************************************************************/
BCSYM_Interface *Symbols::GetInterface
(
    Location *ploc,                     // Location
    _In_z_ STRING *pstrName,                   // Name
    _In_z_ STRING *pstrEmittedName,            // Emmitted name for the symbol
    _In_z_ STRING *pstrNameSpace,              // Namespace we are declared in
    CompilerFile *pfile,                // Associated compiler file
    DECLFLAGS uFlags,                   // Declaration flags
    BCSYM_Implements *----lList,        // the list of interfaces inherited by the one we are creating
    SymbolList *psymlistChildren,       // the list of events, methods, etc. discovered in the interface
    SymbolList *psymlistUnBindableChildren, // the list of get_, set_ for properties and other unbindable symbols in the interface
    _Inout_ BCSYM_GenericParam *ListOfGenericParameters, // [in] the list of type parameter symbols
    _Inout_opt_ SymbolList *psymlist,               // add this interface to this list of symbols
    _Out_opt_ BCSYM_Interface *pAllocatedMemory   // the created interface lives here
)
{
    BCSYM_Interface *pinterface;

    VSASSERT(pstrName && StringPool::StringLength(pstrName), "Must have a name.");
    VSASSERT(pstrNameSpace, "Must have a namespace.");

#if DEBUG
    if (pstrNameSpace)
    {
        WCHAR *wszUnqualName = wcsrchr(pstrNameSpace, L'/');
        VSASSERT(!wszUnqualName || wcslen(wszUnqualName + 1), "Can't have an empty compound namespace name!");
    }
#endif DEBUG

    if ( !pAllocatedMemory )
    {
        // They didn't provide any memory to use - allocate our own memory to hold the interface
        pinterface = (BCSYM_Interface *)AllocSymbol(SYM_Interface, ploc != NULL, 0);
    }
    else // The caller has provided the memory to use for this interface
    {
        pinterface = pAllocatedMemory;
    }

    // Fill it in.
    pinterface->SetName(pstrName);
    pinterface->SetEmittedName(pstrEmittedName);
    pinterface->SetNameSpace(pstrNameSpace);
    pinterface->SetBindingSpace(BINDSPACE_Type);
    pinterface->SetAccess(AccessOfFlags(uFlags));
    pinterface->SetIsHidden(!!(uFlags & DECLF_Hidden));
    pinterface->SetImpList(----lList);
    pinterface->SetCompilerFile(pfile);
    pinterface->SetShadowsKeywordUsed(!!(uFlags & DECLF_ShadowsKeywordUsed));

    if (ploc)
    {
        pinterface->SetLocation(ploc);
    }

    // Add the children.

    BCSYM_Hash *BindableChildren = NULL;
    if (psymlistChildren)
    {
        BindableChildren = GetHashTable(pinterface->GetNameSpace(), pinterface, true, 0, psymlistChildren);
    }

    BCSYM_Hash *UnBindableChildren = NULL;

    if (psymlistUnBindableChildren)
    {
        UnBindableChildren =
            GetHashTable(
                pinterface->GetNameSpace(),
                    pinterface,
                    true,
                    0,
                    psymlistUnBindableChildren);
    }

    SetGenericParams(ListOfGenericParameters, pinterface);

    pinterface->SetHashes(BindableChildren, UnBindableChildren);


    // Link it on the list.
    psymlist->AddToFront(pinterface);

    return pinterface;
}

/**************************************************************************************************
;GetProperty

Defines a preallocated property symbol
***************************************************************************************************/
void Symbols::GetProperty(
    Location *Location, // [in] location information for the property
    _In_z_ STRING *PropertyName, // [in] the name of the property
    DECLFLAGS PropertyFlags, // [in] the flags for the property ( readonly, writeonly )
    BCSYM_Proc *GetProperty,  // [in] the Get Property ( may be NULL )
    BCSYM_Proc *SetProperty, // [in] the Set Property ( may be NULL )
    _Out_ BCSYM_Property *PropertySymbol, // [in] the property symbol to fill in
    _Inout_opt_ SymbolList *TheSymbolList // [in] the symbol list to attach the Property symbol to
)
{
    VSASSERT( PropertyName && StringPool::StringLength( PropertyName ), "Must have a name" );
    VSASSERT( PropertySymbol, "You must provide a PropertySymbol" );

    PropertySymbol->SetPropertyName(PropertyName);

    // Set up the Get property
    PropertySymbol->SetGetProperty(GetProperty);
    if ( GetProperty )
    {
        GetProperty->SetAssociatedPropertyDef( PropertySymbol );
    }

    // Set up the Set property
    PropertySymbol->SetSetProperty(SetProperty);
    if ( SetProperty )
    {
        SetProperty->SetAssociatedPropertyDef( PropertySymbol );
    }

    if ( Location )
    {
        PropertySymbol->SetLocation( Location );
    }

    PropertySymbol->SetIsReadOnly(PropertyFlags & DECLF_ReadOnly );
    PropertySymbol->SetIsWriteOnly (PropertyFlags & DECLF_WriteOnly );
    PropertySymbol->SetIsDefault(PropertyFlags & DECLF_Default );

    TheSymbolList->AddToFront(PropertySymbol);
}

/**************************************************************************************************
;GetHashTable

Allocates a Hash Table and hashes the provided list of symbols into it.
***************************************************************************************************/
BCSYM_Hash *Symbols::GetHashTable
(
    _In_opt_z_ STRING *HashName, // [in] name of the hash table
    BCSYM_NamedRoot *HashOwner, // [in] the symbol that owns this hash table
    bool SetParent, // [in] whether to set the parent pointer in each child to the hash
    unsigned NumberOfBuckets, // [in] number of buckets the hash should have
    SymbolList *HashChildren // [in] the symbols to add to the hash table
)
{
    if (NumberOfBuckets == 0)
    {
        if (HashChildren)
        {
            NumberOfBuckets = HashChildren->GetCount();
        }

        if (NumberOfBuckets == 0)
        {
            // Just picked a decently large prime number.
            // this is usually needed for UnBindable member
            // hashes which are empty initially, but may be
            // filled in bindable.
            //
            NumberOfBuckets = 17;
        }
    }

    // Create the symbol
    BCSYM_Hash *HashSymbol = (BCSYM_Hash *)AllocSymbol(SYM_Hash, false, VBMath::Multiply(
        NumberOfBuckets, 
        sizeof(BCSYM_NamedRoot *)));

    // Get the hash table's name.
    if (HashOwner && HashOwner->IsContainer())
    {
        HashName = m_CompilerInstance->ConcatStrings(HashName, WIDE("."));
    }

    // Fill it in.
    HashSymbol->SetName(HashName);
    HashSymbol->SetImmediateParent(HashOwner);
    HashSymbol->SetBindingSpace((BindspaceType)(BINDSPACE_Normal | BINDSPACE_Type));
    HashSymbol->SetCBuckets(NumberOfBuckets);

#if TRACK_BCSYMHASH
    g_AllocatedBuckets += NumberOfBuckets;
#endif

    // Add symbols to the hash
    if ( HashChildren )
    {
        AddSymbolListToHash( HashSymbol, HashChildren, SetParent );
    }

    return HashSymbol;
}


/**************************************************************************************************
;GetNamespace

Allocates a Namespace symbol, populates it, and attaches it to the namespace ring.
***************************************************************************************************/
BCSYM_Namespace *Symbols::GetNamespace
(
    _In_z_ STRING *UnqualifiedName, // [in] the name of the new namespace
    _In_z_ STRING *ParentQualifiedName, // [in] the containing namespace
    BCSYM_NamespaceRing *NamespaceRing, // [in] the built namespace symbol will be attached to this ring
    CompilerFile *OwningFile, // [in] the source file where this namespace was defined
    unsigned NumChildren, // [in] how many children symbols will be added to the namespace
    unsigned NumUnBindableChildren, // [in] how many unbindable children symbols will be added to the namespace
    Location *NamespaceLocation // [in] location information for the Namespace declaration
)
{
    VSASSERT(UnqualifiedName && ParentQualifiedName && NamespaceRing, "Must have valid ring and names.");
    VSASSERT(OwningFile, "Must be associated with a CompilerFile");
    VSASSERT(
        StringPool::IsEqual(
            ConcatNameSpaceAndName(m_CompilerInstance, ParentQualifiedName, UnqualifiedName),
            NamespaceRing->GetName()),
        "Names differ by more than just case.");

    BCSYM_Namespace *NamespaceSymbol = (BCSYM_Namespace *)AllocSymbol(SYM_Namespace, NamespaceLocation != NULL, 0);

    // Populate it
    NamespaceSymbol->SetName(UnqualifiedName);
    NamespaceSymbol->SetNameSpace(ParentQualifiedName);
    NamespaceSymbol->SetBindingSpace(BINDSPACE_Type);
    NamespaceSymbol->SetAccess(ACCESS_Public);
    NamespaceSymbol->SetNamespaceRing(NamespaceRing);
    NamespaceSymbol->SetCompilerFile(OwningFile);

    if (NumChildren != 0xFFFFFFFF) // 0xFFFFFFFF indicates don't allocate hash - will allocate later
    {
        if (NumChildren == 0)
        {
            // The namespace is being created before the number of entries is known. Seventeen seems like a reasonable guess.
            NumChildren = 17;
        }

        // Create the hash table for the namespace
        NamespaceSymbol->SetHash((BCSYM_Hash *)AllocSymbol(SYM_Hash,
                                                        false, // No location info
                                                        NumChildren * sizeof(BCSYM_NamedRoot *)));

        // Fill in the hash table.
        NamespaceSymbol->GetHashRaw()->SetName(UnqualifiedName);
        NamespaceSymbol->GetHashRaw()->SetImmediateParent(NamespaceSymbol);
        NamespaceSymbol->GetHashRaw()->SetBindingSpace((BindspaceType)(BINDSPACE_Normal | BINDSPACE_Type));
        NamespaceSymbol->GetHashRaw()->PHash()->SetCBuckets(NumChildren);
    }

    if (NumUnBindableChildren != 0xFFFFFFFF) // 0xFFFFFFFF indicates don't allocate hash - will allocate later
    {
        if (NumUnBindableChildren == 0)
        {
            // The namespace is being created before the number of entries is known. Eleven seems like a reasonable guess.
            NumUnBindableChildren = 11;
        }

        // Create the hash table for the namespace
        NamespaceSymbol->SetUnBindableChildrenHash((BCSYM_Hash *)AllocSymbol(SYM_Hash,
            false, // No location info
            VBMath::Multiply(
                NumUnBindableChildren, 
                sizeof(BCSYM_NamedRoot *))));

        // Fill in the hash table.
        NamespaceSymbol->GetUnBindableChildrenHashRaw()->SetName(UnqualifiedName);
        NamespaceSymbol->GetUnBindableChildrenHashRaw()->SetImmediateParent(NamespaceSymbol);
        NamespaceSymbol->GetUnBindableChildrenHashRaw()->SetBindingSpace((BindspaceType)(BINDSPACE_IgnoreSymbol));
        NamespaceSymbol->GetUnBindableChildrenHashRaw()->PHash()->SetCBuckets(NumUnBindableChildren);

        NamespaceSymbol->GetUnBindableChildrenHashRaw()->SetIsUnBindableChildrenHash();
    }

    if ( NamespaceLocation )
    {
        NamespaceSymbol->SetLocation( NamespaceLocation );
    }

    OwningFile->GetCompiler()->AddNamespace(NamespaceSymbol);

    return NamespaceSymbol;
}


/**************************************************************************************************
;GetNamespaceRing

Allocates and populates a NamespaceRing symbol
***************************************************************************************************/
BCSYM_NamespaceRing *Symbols::GetNamespaceRing
(
    _In_z_ STRING *QualifiedName, // [in] The fully qualified name of the namespace this ring represents.
    BCSYM_Hash *RingPool // [in] The hash that contains all namespace rings.
)
{
    BCSYM_NamespaceRing *NamespaceRing = (BCSYM_NamespaceRing *)AllocSymbol(SYM_NamespaceRing, false, 0);

    NamespaceRing->SetName(QualifiedName);  // HACK: Use the fully qualified name here because it's used for binding.
    NamespaceRing->SetImmediateParent(RingPool);
    NamespaceRing->SetBindingSpace((BindspaceType)(BINDSPACE_Normal | BINDSPACE_Type));
    NamespaceRing->SetAccess(ACCESS_Public);

    return NamespaceRing;
}

#if IDE 
/**************************************************************************************************
;GetXmlName

Allocates an XmlName symbol, populates it, and returns it.
***************************************************************************************************/
BCSYM_XmlName *Symbols::GetXmlName
(
    BCSYM_XmlNamespace *pXmlNamespace, // [in] the namespace of the xml namespace
    _In_z_ STRING *Name // [in] the name of the new xml namespace
)
{
    BCSYM_Hash *Hash = pXmlNamespace->GetHash();
    Declaration *LocalName = Hash->SimpleBind(Name, SIMPLEBIND_CaseSensitive);

    if (LocalName)
        return LocalName->PXmlName();

    BCSYM_XmlName *XmlName = (BCSYM_XmlName *)AllocSymbol(SYM_XmlName, false, 0);

    XmlName->SetName(Name);
    XmlName->SetNameSpace(pXmlNamespace->GetName());
    XmlName->SetBindingSpace(BINDSPACE_IgnoreSymbol);
    XmlName->SetAccess(ACCESS_Public);

    AddSymbolToHash(Hash, XmlName, true, false, false);

    return XmlName;
}
#endif

/**************************************************************************************************
;GetXmlNamespaceDeclaration

Allocates an XmlNamespaceDeclaration symbol, populates it, and returns it.
***************************************************************************************************/
BCSYM_XmlNamespaceDeclaration *Symbols::GetXmlNamespaceDeclaration
(
    _In_z_ STRING *Name, // [in] the name of the new xml namespace
    CompilerFile *OwningFile, // [in] the source file where this namespace was defined (can be null)
    CompilerProject *Project, // [in] the project where this namespace was defined
    Location *NamespaceLocation, // [in] location information for the Namespace declaration
    bool IsImported  // [in] is this declaration from an imports statement
)
{
    BCSYM_XmlNamespaceDeclaration *XmlNamespace = (BCSYM_XmlNamespaceDeclaration *)AllocSymbol(SYM_XmlNamespaceDeclaration, NamespaceLocation != NULL, 0);
    BCSYM_Namespace *UnnamedNamespace = NULL;

    if (OwningFile)
    {
        UnnamedNamespace = OwningFile->GetUnnamedNamespace();
    }
    else if (m_CompilerInstance && Project)
    {
        UnnamedNamespace = m_CompilerInstance->GetUnnamedNamespace(Project);
    }

    XmlNamespace->SetName(Name);
    XmlNamespace->SetImmediateParent(UnnamedNamespace);
    XmlNamespace->SetBindingSpace(BINDSPACE_IgnoreSymbol);
    XmlNamespace->SetAccess(ACCESS_Public);

    if ( NamespaceLocation )
    {
        XmlNamespace->SetLocation( NamespaceLocation );
    }

    XmlNamespace->SetIsImported(IsImported);

    return XmlNamespace;
}

/**************************************************************************************************
;GetXmlNamespace

Allocates an XmlNamespace symbol, populates it, and returns it.
***************************************************************************************************/
BCSYM_XmlNamespace *Symbols::GetXmlNamespace
(
    _In_z_ STRING *Name, // [in] the name of the new xml namespace
    CompilerFile *OwningFile, // [in] the source file where this namespace was defined (can be null)
    CompilerProject *Project // [in] the project where this namespace was defined
)
{
    BCSYM_XmlNamespace *XmlNamespace = (BCSYM_XmlNamespace *)AllocSymbol(SYM_XmlNamespace, false, 0);
    BCSYM_Namespace *UnnamedNamespace = NULL;

    if (OwningFile)
    {
        UnnamedNamespace = OwningFile->GetUnnamedNamespace();
    }
    else if (m_CompilerInstance && Project)
    {
        UnnamedNamespace = m_CompilerInstance->GetUnnamedNamespace(Project);
    }


    XmlNamespace->SetName(Name);
    XmlNamespace->SetImmediateParent(UnnamedNamespace);
    XmlNamespace->SetBindingSpace(BINDSPACE_IgnoreSymbol);
    XmlNamespace->SetAccess(ACCESS_Public);
    XmlNamespace->SetCompilerFile(OwningFile);

#if IDE 
    unsigned NumChildren = 17;

    // Create the hash table for the namespace
    XmlNamespace->SetHash((BCSYM_Hash *)AllocSymbol(SYM_Hash,
        false, // No location info
        NumChildren * sizeof(BCSYM_NamedRoot *)));

    // Fill in the hash table.
    XmlNamespace->GetHashRaw()->SetName(Name);
    XmlNamespace->GetHashRaw()->SetImmediateParent(XmlNamespace);
    XmlNamespace->GetHashRaw()->SetBindingSpace((BindspaceType)(BINDSPACE_Normal | BINDSPACE_Type));
    XmlNamespace->GetHashRaw()->PHash()->SetCBuckets(NumChildren);
#endif

    return XmlNamespace;
}


//============================================================================
// Create a node in the IMPLEMENTS list
//============================================================================
BCSYM_ImplementsList *Symbols::GetImplementsList
(
    Location * ploc,
    BCSYM * ptyp,
    _In_z_ STRING * pstrProcName
)
{
    BCSYM_ImplementsList * ImplementsSymbol;

    // Create the symbol
    ImplementsSymbol = (BCSYM_ImplementsList *)AllocSymbol(SYM_ImplementsList, !!ploc, 0);

    ImplementsSymbol->SetType(ptyp);
    ImplementsSymbol->SetName(pstrProcName);
    ImplementsSymbol->SetNext(NULL);

    if (ploc)
    {
        ImplementsSymbol->SetLocation(ploc);
    }

    return ImplementsSymbol;
}

BCSYM_GenericNonTypeConstraint *Symbols::GetGenericNonTypeConstraint
(
    Location *Location,
    BCSYM_GenericNonTypeConstraint::ConstraintKind ConstraintKind
)
{
    VSASSERT(ConstraintKind == BCSYM_GenericNonTypeConstraint::ConstraintKind_New ||
             ConstraintKind == BCSYM_GenericNonTypeConstraint::ConstraintKind_Ref ||
             ConstraintKind == BCSYM_GenericNonTypeConstraint::ConstraintKind_Value,
                "Unexpected Non-type constraint kind!!!");

    BCSYM_GenericNonTypeConstraint *Constraint = (BCSYM_GenericNonTypeConstraint *)AllocSymbol(SYM_GenericNonTypeConstraint, !!Location, 0);

    Constraint->SetConstraintKind(ConstraintKind);

    if (Location)
    {
        Constraint->SetLocation(Location);
    }

    return Constraint;
}

BCSYM_GenericTypeConstraint *Symbols::GetGenericTypeConstraint
(
    Location *Location,
    BCSYM *ConstraintType
)
{
    BCSYM_GenericTypeConstraint *Constraint = (BCSYM_GenericTypeConstraint *)AllocSymbol(SYM_GenericTypeConstraint, !!Location, 0);

    Constraint->SetType(ConstraintType);

    if (Location)
    {
        Constraint->SetLocation(Location);
    }

    return Constraint;
}

BCSYM_GenericParam *Symbols::GetGenericParam
(
    Location *Location,
    _In_z_ STRING *Name,
    unsigned Position,
    bool IsGenericMethodParam,
    Variance_Kind Variance
)
{
    BCSYM_GenericParam *Param = (BCSYM_GenericParam *)AllocSymbol(SYM_GenericParam, !!Location, 0);

    Param->SetName(Name);

    if (Location)
    {
        Param->SetLocation(Location);
    }

    Param->SetPosition(Position);
    Param->SetAccess(ACCESS_Public);
    Param->SetBindingSpace(BINDSPACE_Type);
    Param->SetIsGenericMethodParam(IsGenericMethodParam);
    Param->SetVariance(Variance);

    return Param;
}

BCSYM_GenericTypeBinding *Symbols::GetGenericTypeInsantiation
(
    BCSYM_NamedRoot *TypeConstructor,
    Type* ActualTypeArgument
)
{
    BCSYM** argArray = reinterpret_cast<BCSYM**>(GetNorlsAllocator()->Alloc(sizeof(BCSYM*)));
    argArray[0] = ActualTypeArgument;
    return GetGenericBinding(false, TypeConstructor, argArray, 1, NULL)->PGenericTypeBinding();
}


BCSYM_GenericBinding *Symbols::GetGenericBinding
(
    bool IsBad,
    BCSYM_NamedRoot *Generic,
    _In_count_(ArgumentCount) BCSYM *Arguments[],
    unsigned ArgumentCount,
    BCSYM_GenericTypeBinding *ParentBinding,
    bool AllocAndCopyArgumentsToNewList
)
{
    // 

    // Build a binding
    //

    // If bad binding or a location has been specified, then build a binding
    //
    if (IsBad ||
        !m_GenericBindingCache)
    {
        return
            BuildGenericBinding
            (
                IsBad,
                Generic,
                Arguments,
                ArgumentCount,
                ParentBinding,
                NULL,
                AllocAndCopyArgumentsToNewList
            );
    }

    unsigned long BindingCacheKey;

    bool ValidBindingCacheKeyComputed =
        GenericBindingCache::ComputeKeyForGenericBinding(
            Generic,
            Arguments,
            ArgumentCount,
            ParentBinding,
            BindingCacheKey);

    BCSYM_GenericBinding *CachedBinding =
        ValidBindingCacheKeyComputed ?
            m_GenericBindingCache->Find(
                BindingCacheKey,
                Generic,
                Arguments,
                ArgumentCount,
                ParentBinding) :
            NULL;

    if (!CachedBinding)
    {
        CachedBinding =
            BuildGenericBinding
            (
                IsBad,
                Generic,
                Arguments,
                ArgumentCount,
                ParentBinding,
                NULL,
                AllocAndCopyArgumentsToNewList
            );


        if (ValidBindingCacheKeyComputed)
        {
            m_GenericBindingCache->Add(
                BindingCacheKey,
                CachedBinding);
        }
    }

    return CachedBinding;
}


// Dev10 #618745:
// Return new or cached GenericBinding equivalent to the one passed in.
// Used to deal with lifetime management for GenericBindings.
BCSYM_GenericBinding *Symbols::GetEquivalentGenericBinding
(
    BCSYM_GenericBinding * pBinding
)
{
    AssertIfNull(pBinding);

    if ( pBinding == NULL )
    {
        return NULL;
    }

    NorlsAllocator scratch(NORLSLOC);

    // To ensure that we are taking advantage of generic binding cache, 
    // parent binding should be dealt with first.
    BCSYM_GenericTypeBinding *pParentBinding = pBinding->GetParentBinding();

    if ( pParentBinding != NULL )
    {
        pParentBinding = GetEquivalentGenericBinding(pParentBinding)->PGenericTypeBinding();
    }

    unsigned args = pBinding->GetArgumentCount();
    BCSYM **pArguments = NULL;

    if ( args > 0 )
    {
        pArguments = scratch.AllocArray<BCSYM *>(args);

        for (unsigned i=0; i<args; i++)
        {
            BCSYM * pArgument = pBinding->GetArgument(i);
            
            if (pArgument != NULL && pArgument->IsGenericBinding())
            {
                pArguments[i] = GetEquivalentGenericBinding(pArgument->PGenericBinding());
            }
            else 
            {
                pArguments[i] = pArgument;
            }
        }
    }

    AssertIfTrue( pBinding->GetGeneric()->IsGenericBinding() );

    BCSYM_GenericBinding * pNewBinding;
        
    if ( pBinding->HasLocation() )
    {
        pNewBinding = GetGenericBindingWithLocation(
            pBinding->IsBadNamedRoot(), // Symbols::BuildGenericBinding sets this flag
            pBinding->GetGeneric(), 
            pArguments, 
            args, 
            pParentBinding, 
            pBinding->GetLocation(),
            true); // AllocAndCopyArgumentsToNewList, our array is temporary
    }
    else
    {
        pNewBinding = GetGenericBinding(
            pBinding->IsBadNamedRoot(), // Symbols::BuildGenericBinding sets this flag
            pBinding->GetGeneric(), 
            pArguments, 
            args, 
            pParentBinding, 
            true); // AllocAndCopyArgumentsToNewList, our array is temporary
    }

    AssertIfFalse( pBinding->IsBadGenericBinding() == pNewBinding->IsBadGenericBinding() );

    return pNewBinding;
}

BCSYM_GenericBinding *Symbols::GetGenericBindingWithLocation
(
    bool IsBad,
    BCSYM_NamedRoot *Generic,
    _In_count_(ArgumentCount) BCSYM *Arguments[],
    unsigned ArgumentCount,
    BCSYM_GenericTypeBinding *ParentBinding,
    const Location *Loc,
    bool AllocAndCopyArgumentsToNewList
)
{
    // Generic Binding with location info are never cached.
    return
        BuildGenericBinding
            (
                IsBad,
                Generic,
                Arguments,
                ArgumentCount,
                ParentBinding,
                Loc,
                AllocAndCopyArgumentsToNewList
            );
}

BCSYM_GenericBinding *Symbols::BuildGenericBinding
(
    bool IsBad,
    BCSYM_NamedRoot *Generic,
    _In_count_(ArgumentCount) BCSYM *Arguments[],
    unsigned ArgumentCount,
    BCSYM_GenericTypeBinding *ParentBinding,
    const Location *Loc,
    bool AllocAndCopyArgumentsToNewList
)
{
    if (AllocAndCopyArgumentsToNewList && ArgumentCount > 0)
    {
        BCSYM **NewArgumentsList = (BCSYM **)m_Allocator->Alloc(VBMath::Multiply(
            sizeof(BCSYM *), 
            ArgumentCount));
        memcpy(NewArgumentsList, Arguments, sizeof(BCSYM *) * ArgumentCount);

        Arguments = NewArgumentsList;
    }

    BCSYM_GenericBinding *Binding = (BCSYM_GenericBinding *)AllocSymbol(Generic->IsType() ? SYM_GenericTypeBinding : SYM_GenericBinding, Loc ? true : false, 0);

    Binding->SetGeneric(Generic);
    Binding->SetArguments(Arguments);
    Binding->SetArgumentCount(ArgumentCount);
    Binding->SetParentBinding(ParentBinding);
    Binding->SetIsBadRaw(IsBad);
    if (Loc)
    {
        Binding->SetLocation(Loc);
    }

    return Binding;
}


/**************************************************************************************************
;GetHandlesList

Create a HandlesList node
***************************************************************************************************/
BCSYM_HandlesList *Symbols::GetHandlesList
(
    Location *FullyQualifiedNameLocation, // [in] Given Handles WithEventsVar.foo.Event, this is the location of WithEventVar.foo.Event
    _In_ Location *WithEventVarLocation,       // [in] Given Handles WithEventVar.Event this is the location of WithEventVar
    _In_ Location *EventNameLocation,          // [in] Given Handles WithEventVar.Event this is the location of Event
    _In_opt_z_ STRING   *WithEventsVarName,          // [in] Given Handles WithEventVar.Event this is the name of WithEventsVar
    _In_opt_z_ STRING   *EventSourcePropertyName,    // [in] Given Handles WithEventVar.Prop.Event this is the name of Prop
    _In_z_ STRING   *EventName,                  // [in] Given Handles WithEventVar.Event this is the name of Event
    bool     HandlesMyBaseEvent,          // [in] This is true for Hanldes MyBase.Event
    bool     HandlesMyClassEvent,         // [in] This is true for Handles MyClass.Event
    bool     HandlesMeEvent,              // [in] This is true for Handles Me.Event
    BCSYM_MethodImpl *HandlingMethod
)
{
    BCSYM_HandlesList *HandlesListSymbol = (BCSYM_HandlesList *)AllocSymbol( SYM_HandlesList, true, 0 );

    if ( HandlesMyBaseEvent )
    {
        VSASSERT( !WithEventsVarName && !EventSourcePropertyName,
                        "How can these be set for a MyBase Context ?");

        HandlesListSymbol->SetIsMyBase();
    }
    else if ( HandlesMyClassEvent )
    {
        VSASSERT( !WithEventsVarName && !EventSourcePropertyName,
                        "How can these be set for a MyClass Context ?");

        HandlesListSymbol->SetIsMyClass();
    }
    else if ( HandlesMeEvent )
    {
        VSASSERT( !WithEventsVarName && !EventSourcePropertyName,
                        "How can these be set for a Me Context ?");

        HandlesListSymbol->SetIsEventFromMe();
    }
    else
    {
        HandlesListSymbol->SetWithEventsVarName(WithEventsVarName);
        HandlesListSymbol->SetEventSourcePropertyName(EventSourcePropertyName);
    }

    HandlesListSymbol->SetEventName(EventName);
    HandlesListSymbol->SetNextHandles(NULL);

    VSASSERT( EventNameLocation && WithEventVarLocation,
                    "How can locations for withevents var or event name not exists ?");

    HandlesListSymbol->SetLocation( FullyQualifiedNameLocation );

    HandlesListSymbol->GetLocationOfEvent()->SetLocation( EventNameLocation );

    HandlesListSymbol->GetLocationOfWithEventsVar()->SetLocation( WithEventVarLocation );
    HandlesListSymbol->GetLocationOfWithEventsVar()->fIsInSourceFileHashTable = false;
    HandlesListSymbol->GetLocationOfWithEventsVar()->fIsChanged = false;

    if ( m_LineMarkerTable != NULL )
    {
#if IDE
        m_LineMarkerTable->AddDeclLocation( HandlesListSymbol->GetLocationOfWithEventsVar(), NULL );
#endif IDE
    }

    HandlesListSymbol->SetHandlingMethod(HandlingMethod);

    return HandlesListSymbol;
}

/**************************************************************************************************
;GetTypeForwarder

Creates a Type forwarder symbol
***************************************************************************************************/
BCSYM_TypeForwarder *
Symbols::GetTypeForwarder
(
    _In_z_ STRING *Name, // [in] the type name for this forwarder
    _In_z_ STRING *Namespace, // [in] the namespace the type lives in
    mdAssemblyRef tkDestAssemblyRef,   // [in] the ref of the assembly to which this type has been forwarded
    _Inout_opt_ SymbolList *OwningSymbolList // [in] the symbol list to attach this symbol to ( may be NULL )
)
{
    // Create the symbol.
    BCSYM_TypeForwarder *TypeForwarder = (BCSYM_TypeForwarder *)AllocSymbol( SYM_TypeForwarder, false /* no location info */, 0 /* no extra bytes */ );

    InitBadNamedRoot(
        TypeForwarder,
        Name,
        Namespace,
        DECLF_NoFlags,
        BINDSPACE_Type,
        ERRID_ForwardedTypeUnavailable3,
        NULL);

    TypeForwarder->SetAssemblyRefForwardedTo(tkDestAssemblyRef);

    // Link it on the list.
    OwningSymbolList->AddToFront(TypeForwarder);

    return TypeForwarder;
}

BCSYM_ExtensionCallLookupResult  * Symbols::GetExtensionCallLookupResult()
{

    ExtensionCallLookupResult * pExtCall =
        (BCSYM_ExtensionCallLookupResult  *)AllocSymbol
        (
            SYM_ExtensionCallLookupResult ,
            //no location info
            false,
            //0 extra bytes
            0
        );

    return pExtCall;
}

//****************************************************************************
// External helpers.
//****************************************************************************

//============================================================================
// Add a symbol list to a hash table.
//============================================================================

void Symbols::AddSymbolListToHash
(
    _Inout_ BCSYM_Hash *phash,
    _In_opt_ SymbolList *psymlist,
    bool SetParent
)
{
    BCSYM_NamedRoot *pnamed, *pnamedNext;

    // psymlist may be null when we are creating hash tables corresponding to
    // the namespace hierarchy
    if(!psymlist)
        return;


    bool ContainerIsModule = false, ContainerIsNamespace = false;
    BCSYM_NamedRoot *HashParent = phash->GetParent();
    if (HashParent)
    {
        if (HashParent->IsClass())
        {
            ContainerIsModule = HashParent->PClass()->IsStdModule();
        }
        else if (HashParent->IsNamespace())
        {
            ContainerIsNamespace = true;
        }
    }

    for (pnamed = psymlist->GetFirst(); pnamed; pnamed = pnamedNext)
    {
        pnamedNext = pnamed->GetNextInSymbolList();

        AddSymbolToHash(phash, pnamed, SetParent, ContainerIsModule, ContainerIsNamespace);
    }

    // The above has destroyed the list 'psymlist', so set it to an empty list
    // as a precaution against it being used later
    psymlist->Clear();
}

//============================================================================
// Add a symbol to a hash table.
//============================================================================

void Symbols::AddSymbolToHash
(
    _Inout_ BCSYM_Hash *phash,
    _Inout_ BCSYM_NamedRoot *pnamed,
    bool SetParent,
    bool ContainerIsModule,
    bool ContainerIsNamespace
)
{
    STRING *pname = pnamed->GetName();

    if (ContainerIsModule)
    {
        StringPool::SetDeclaredInModule(pname);
    }

    if (ContainerIsNamespace)
    {
        StringPool::AddDeclarationInNamespace(pname, phash->GetParent()->PNamespace());
    }

    // find the correct bucket
    unsigned iBucket = StringPool::HashValOfString(pname) % phash->CBuckets();

    VSASSERT( phash->CBuckets(),
                    "Hash with zero buckets unexpected!!!");

#if TRACK_BCSYMHASH
    if (phash->m_pHashTable[iBucket] == NULL)
    {
        g_UsedBuckets++;
    }
#endif

    // Simply add pSymbol at the front of the bucket.
    pnamed->SetNextInSymbolList(phash->Rgpnamed()[iBucket]);
    phash->Rgpnamed()[iBucket] = pnamed;
    phash->SetCSymbols(phash->CSymbols() + 1);
    IfFalseThrow(phash->CSymbols() > 0);

#if TRACK_BCSYMHASH
    g_TotalSymbols++;
#endif

    // If the added symbol is itself a hash, propagate its count.
    if (pnamed->DigThroughAlias()->IsHash())
    {
        phash->SetCSymbols(phash->CSymbols() + pnamed->DigThroughAlias()->PHash()->CSymbols());
        IfFalseThrow(phash->CSymbols() >= pnamed->DigThroughAlias()->PHash()->CSymbols());
    }

    // set its parent
    if (SetParent)
    {
        pnamed->SetImmediateParent(phash);

        if (pnamed->IsProperty())
        {
            if (pnamed->PProperty()->GetProperty())
            {
                pnamed->PProperty()->GetProperty()->SetImmediateParent(phash);
            }
            if (pnamed->PProperty()->SetProperty())
            {
                pnamed->PProperty()->SetProperty()->SetImmediateParent(phash);
            }
        }

        else if (pnamed->IsEventDecl())
        {
            if (pnamed->PEventDecl()->GetProcAdd())
            {
                pnamed->PEventDecl()->GetProcAdd()->SetImmediateParent(phash);
            }

            if (pnamed->PEventDecl()->GetProcRemove())
            {
                pnamed->PEventDecl()->GetProcRemove()->SetImmediateParent(phash);
            }

            if (pnamed->PEventDecl()->GetProcFire())
            {
                pnamed->PEventDecl()->GetProcFire()->SetImmediateParent(phash);
            }
        }
        else if (pnamed->IsClass() && pnamed->PClass()->IsStdModule())
        {
            // All standard modules in a namespace are on a list so that
            // name lookup can traverse them quickly.

            BCSYM_Namespace *pContainingNamespace = phash->GetParent()->PNamespace();

            pnamed->PClass()->SetNextModule(pContainingNamespace->GetFirstModule());
            pContainingNamespace->SetFirstModule(pnamed->PClass());
        }
    }
}

/*****************************************************************************
;RemoveSymbolFromHash

Removes a symbol from the specified hash table
*****************************************************************************/
BCSYM_NamedRoot * // The removed item (Null if it could not be found)
Symbols::RemoveSymbolFromHash
(
    _Inout_ BCSYM_Hash *Hash, // [in] the hash table we want to remove the symbol from
    BCSYM_NamedRoot *RemoveMe // [in] the symbol to remove from the hash table
)
{
    // Find the correct bucket
    int HashBucket = StringPool::HashValOfString( RemoveMe->GetName()) % Hash->CBuckets();

    // Find our symbol in the bucket
    BCSYM_NamedRoot *PreviousGuy = NULL;
    for ( BCSYM_NamedRoot *Candidate = Hash->Rgpnamed()[ HashBucket ]; Candidate != NULL; Candidate = Candidate->GetNextInSymbolList() )
    {
        if ( Candidate == RemoveMe )
        {
            // Do the remove
            if ( PreviousGuy )
            {
                PreviousGuy->SetNextInSymbolList(Candidate->GetNextInSymbolList());
            }
            else
            {
                Hash->Rgpnamed()[ HashBucket ] = Candidate->GetNextInSymbolList();
            }
            Hash->SetCSymbols(Hash->CSymbols() -1);
            return Candidate;
        }
        PreviousGuy = Candidate;
    }
    return NULL;
}

//****************************************************************************
// Internal helpers.
//****************************************************************************

#if TAGSYMBOLENTRYFUNCTION



static LONG g_nSymCounter=0;
static LPVOID g_nSymbolToBreakOn = 0;
void BCSYM::SymbolEntry(char *szFile, int nLineNo, char *func) const
{
    if (this)
    {
        if (this ==g_nSymbolToBreakOn) // set this in debugger: much faster than cond bpts.
        {
            __debugbreak();
        }
        const_cast<BCSYM *>(this)->m_nCallCount++;
        const_cast<BCSYM *>(this)->m_nLastRef = InterlockedIncrement(&g_nSymCounter);
        if (m_nBorn ==0)
        {
            const_cast<BCSYM *>(this)->m_nBorn = m_nLastRef;
            const_cast<BCSYM *>(this)->m_dwThreadId = GetCurrentThreadId();
        }
        else
        {
            if (m_dwThreadId != GetCurrentThreadId())
            {
                if (m_RecordedAllocator &&
                    m_RecordedAllocator->m_nLineNo== 31 && 
                    m_RecordedAllocator->m_szFile[0] == 'C') // //compilerfile.cpp(31)
                {
                    VSASSERT(true,"put a bpt here. symbol born in one world accessed in another");
                }
            }
        }
    }
}

#endif TAGSYMBOLENTRYFUNCTION

/**************************************************************************************************
;AllocSymbol

Allocate the memory for a symbol.
***************************************************************************************************/
BCSYM *Symbols::AllocSymbol
(
    BilKind SymbolKind,
    bool LocationInfoAvail,
    size_t ExtraBytesToAlloc
)
{
    HRESULT hr = NOERROR;

    bool IsValidSize = false;
    size_t BytesToAlloc = 0;

    if (SymbolKind >= 0 && SymbolKind < _countof(SymbolSizeInfo))
    {
        // Calculate how must room to give the symbol.
        BytesToAlloc = SymbolSizeInfo[SymbolKind].m_cbSize + ExtraBytesToAlloc;

        if ( BytesToAlloc >= SymbolSizeInfo[SymbolKind].m_cbSize)
        {
            if (BytesToAlloc + (LocationInfoAvail ? sizeof(TrackedLocation) : 0) >= BytesToAlloc)
            {
                BytesToAlloc += LocationInfoAvail ? sizeof(TrackedLocation) : 0;

                IsValidSize = true;
            }
        }
    }

    // Allocate the symbol.
    BCSYM *AllocatedSymbol = NULL;

    if (IsValidSize)
    {
        AllocatedSymbol = (BCSYM *)m_Allocator->Alloc(BytesToAlloc);
    }

    if (IsValidSize && LocationInfoAvail)
    {
        TrackedLocation* pTrackedLocation = (TrackedLocation*) AllocatedSymbol;
        pTrackedLocation->fIsInSourceFileHashTable = false;
        pTrackedLocation->fIsChanged = false;

        AllocatedSymbol = (BCSYM *)(pTrackedLocation + 1);

        if ( m_LineMarkerTable != NULL )
        {
#if IDE
            m_LineMarkerTable->AddDeclLocation( pTrackedLocation, AllocatedSymbol );
#endif
            if ( SymbolKind == SYM_MethodImpl )
            {
                TrackedCodeBlock * pTrackedCodeBlock = (TrackedCodeBlock *) ((BCSYM_MethodImpl*)AllocatedSymbol)->GetCodeBlock();
                pTrackedCodeBlock->fProcBlock = false;

                TrackedCodeBlock * pTrackedProcBlock = (TrackedCodeBlock *) ((BCSYM_MethodImpl*)AllocatedSymbol)->GetProcBlock();
                pTrackedProcBlock->fProcBlock = true;
#if IDE
                m_LineMarkerTable->AddCodeBlock(pTrackedCodeBlock, AllocatedSymbol);
                m_LineMarkerTable->AddCodeBlock(pTrackedProcBlock, AllocatedSymbol);
#endif
            }
        }
    }

    void *pSymVoid = AllocatedSymbol;

//if you remove this code, for any reason,
//make sure that you go to GetExtensionCallLookupResult() and
//move the constructor invocation out of the #if ! DEBUG block
//because extension call lookup results always need their constructors
//run.
#if DEBUG
	
    switch(SymbolKind)
    {
        case SYM_PointerType:
            new (pSymVoid) BCSYM_PointerType;
            break;

        case SYM_ArrayType:
            new (pSymVoid) BCSYM_ArrayType;
            break;

        case SYM_NamedType:
            new (pSymVoid) BCSYM_NamedType;
            break;

        case SYM_Alias:
            new (pSymVoid) BCSYM_Alias;
            break;

        case SYM_CCContainer:
            new (pSymVoid) BCSYM_CCContainer;
            break;

        case SYM_Class:
            new (pSymVoid) BCSYM_Class;
            break;

        case SYM_MethodImpl:
            new (pSymVoid) BCSYM_MethodImpl;
            break;

        case SYM_SyntheticMethod:
            new (pSymVoid) BCSYM_SyntheticMethod;
            break;

        case SYM_MethodDecl:
            new (pSymVoid) BCSYM_MethodDecl;
            break;

        case SYM_EventDecl:
            new (pSymVoid) BCSYM_EventDecl;
            break;

        case SYM_DllDeclare:
            new (pSymVoid) BCSYM_DllDeclare;
            break;

        case SYM_Param:
            new (pSymVoid) BCSYM_Param;
            break;

        case SYM_ParamWithValue:
            new (pSymVoid) BCSYM_ParamWithValue;
            break;

        case SYM_Variable:
            new (pSymVoid) BCSYM_Variable;
            break;

        case SYM_VariableWithValue:
            new (pSymVoid) BCSYM_VariableWithValue;
            break;

        case SYM_VariableWithArraySizes:
            new (pSymVoid) BCSYM_VariableWithArraySizes;
            break;

        case SYM_CCConstant:
            new (pSymVoid) BCSYM_CCConstant;
            break;

        case SYM_StaticLocalBackingField:
            new (pSymVoid) BCSYM_StaticLocalBackingField;
            break;

        case SYM_Expression:
            new (pSymVoid) BCSYM_Expression;
            break;

        case SYM_Implements:
            new (pSymVoid) BCSYM_Implements;
            break;

        case SYM_Interface:
            new (pSymVoid) BCSYM_Interface;
            break;

        case SYM_Hash:
            new (pSymVoid) BCSYM_Hash;
            break;

        case SYM_HandlesList:
            new (pSymVoid) BCSYM_HandlesList;
            break;

        case SYM_ImplementsList:
            new (pSymVoid) BCSYM_ImplementsList;
            break;

        case SYM_Namespace:
            new (pSymVoid) BCSYM_Namespace;
            break;

        case SYM_NamespaceRing:
            new (pSymVoid) BCSYM_NamespaceRing;
            break;

        case SYM_XmlName:
            new (pSymVoid) BCSYM_XmlName;
            break;

        case SYM_XmlNamespaceDeclaration:
            new (pSymVoid) BCSYM_XmlNamespaceDeclaration;
            break;

        case SYM_XmlNamespace:
            new (pSymVoid) BCSYM_XmlNamespace;
            break;

        case SYM_Property:
            new (pSymVoid) BCSYM_Property;
            break;

        case SYM_ApplAttr:
            new (pSymVoid) BCSYM_ApplAttr;
            break;

        case SYM_UserDefinedOperator:
            new (pSymVoid) BCSYM_UserDefinedOperator;
            break;

        case SYM_GenericBadNamedRoot:
            new (pSymVoid) BCSYM_GenericBadNamedRoot;
            break;

        case SYM_GenericParam:
            new (pSymVoid) BCSYM_GenericParam;
            break;

        case SYM_GenericTypeConstraint:
            new (pSymVoid) BCSYM_GenericTypeConstraint;
            break;

        case SYM_GenericNonTypeConstraint:
            new (pSymVoid) BCSYM_GenericNonTypeConstraint;
            break;

        case SYM_GenericBinding:
            new (pSymVoid) BCSYM_GenericBinding;
            break;

        case SYM_GenericTypeBinding:
            new (pSymVoid) BCSYM_GenericTypeBinding;
            break;

        case SYM_TypeForwarder:
            new (pSymVoid) BCSYM_TypeForwarder;
            break;
        case SYM_ExtensionCallLookupResult :
            new (pSymVoid) BCSYM_ExtensionCallLookupResult (m_Allocator);
            break;
        case SYM_LiftedOperatorMethod:
            new (pSymVoid) BCSYM_LiftedOperatorMethod;
            break;
        case SYM_ArrayLiteralType:
           new (pSymVoid) BCSYM_ArrayLiteralType(m_Allocator);
           break;
        default:
            VSFAIL("Bad type.");
            break;
    }
#endif  // DEBUG

#if !DEBUG
    if (SymbolKind == SYM_ExtensionCallLookupResult)
    {
        new (pSymVoid) BCSYM_ExtensionCallLookupResult (m_Allocator);
    }
    else if (SymbolKind == SYM_ArrayLiteralType)
    {
        new (pSymVoid) BCSYM_ArrayLiteralType(m_Allocator);
    }
#endif
    // Set its initial information.
    AllocatedSymbol->SetSkKind(SymbolKind);
    AllocatedSymbol->SetHasLocation(LocationInfoAvail);

#if FV_TRACK_MEMORY && IDE

    AllocatedSymbol->m_totalSize= BytesToAlloc;
    m_CompilerInstance->GetInstrumentation()->TrackSymbolCreation(AllocatedSymbol);
    m_Allocator->RegisterDestructionCallBack((void (*)(void *))Instrumentation::S_TrackSymbolDestruction, AllocatedSymbol);

#endif


#if NRLSTRACK
    AllocatedSymbol->SetRecordedAllocator(m_Allocator); // record who allocated us
#endif NRLSTRACK

    // Return it.
    return AllocatedSymbol;
}

/**************************************************************************************************
;AllocImportedTarget

Allocate the memory for an ImportsTarget symbol.
***************************************************************************************************/
ImportedTarget *Symbols::AllocImportedTarget
(
    unsigned NumDotDelimitedNames
)
{
    // Allocate the symbol.
    size_t cbSize = sizeof( ImportedTarget ) + VBMath::Multiply(
        NumDotDelimitedNames, 
        sizeof( STRING * ));

    IfFalseThrow(cbSize >= sizeof( ImportedTarget ));
    ImportedTarget *ImportTarget = (ImportedTarget *)m_Allocator->Alloc(cbSize);

    ImportTarget->m_DotDelimitedNames = &ImportTarget->m_FirstDotDelimitedNames;
    ImportTarget->m_loc.fIsInSourceFileHashTable = false;
    ImportTarget->m_loc.fIsChanged = false;

    if ( m_LineMarkerTable != NULL )
    {
#if IDE
        m_LineMarkerTable->AddDeclLocation( &(ImportTarget->m_loc), NULL );
#endif
    }

    // Return it.
    return ImportTarget;
}

//============================================================================
// Set the information into a container that allows us to delay load it.
//============================================================================

void Symbols::SetContainerDelayLoadInformation
(
    _Inout_ BCSYM_Container *Container,
    mdTypeDef TypeDef,
    mdTypeRef Base,
    MetaDataFile *pMetaDataFile
)
{
    Container->SetChildrenNotLoaded(true);
    Container->SetToken(TypeDef);
    Container->SetCompilerFile(pMetaDataFile);

    if (Container->IsClass())
    {
        Container->PClass()->SetBaseClassToken(Base);

        // Object has no base class and no implemented interfaces, so its
        // bases and implements are already "loaded".
        if (!Container->PClass()->IsObject())
        {
            Container->SetBaseAndImplementsNotLoaded(true);
        }
    }
    else
    {
        Container->SetBaseAndImplementsNotLoaded(true);
    }
}

void Symbols::SetDelayBindNamespace
(
    BCSYM_Namespace *Namespace
)
{
    Namespace->SetChildrenNotLoaded(true);
}

//============================================================================
// Morphs a variable into a const
//============================================================================

void Symbols::MorphVariableToConst
(
    _Out_ BCSYM_Variable *Variable,
    BCSYM_Expression *Value
)
{
    Symbols::MorphVariableToVariableWithValue(Variable, Value);
    Variable->SetVarkind(VAR_Const);
}

//============================================================================
// Morphs a variable into a const
//============================================================================

void Symbols::MorphVariableToVariableWithValue
(
    _Out_ BCSYM_Variable *Variable,
    BCSYM_Expression *Value
)
{
    VSASSERT(sizeof(BCSYM_Variable) == sizeof(BCSYM_VariableWithValue), "Must be the same size.");

    Variable->SetSkKind(SYM_VariableWithValue);
    ((BCSYM_VariableWithValue*)Variable)->SetExpression(Value);

#if DEBUG
    new ((void*)Variable) BCSYM_VariableWithValue;
#endif DEBUG
}


//============================================================================
// Morphs a param into an optional param
//============================================================================

void Symbols::MorphParamToOptional
(
    _Out_ BCSYM_Param *Param,
    BCSYM_Expression *Value
)
{
    VSASSERT(sizeof(BCSYM_Param) == sizeof(BCSYM_ParamWithValue), "Must be the same size.");

    Param->SetSkKind(SYM_ParamWithValue);
    Param->SetExpression(Value);

#if DEBUG
    new ((void*)Param) BCSYM_ParamWithValue;
#endif DEBUG
}

//============================================================================
// Resets the error bits within this symbol to false.
// Or Might reset the Overloads and Overrides bits
//============================================================================

void Symbols::ResetFlags(BCSYM_NamedRoot *pnamed, CompilationState State)
{
    VSASSERT(State == CS_TypesEmitted || State == CS_Declared, "Untested state");

    pnamed->ResetBadness(State);

    switch (pnamed->GetKind())
    {
    case SYM_Alias:
        ResetFlags(pnamed->PAlias()->GetSymbol()->PNamedRoot(), State);
        break;

    case SYM_Hash:
        {
            BCITER_HASH_CHILD bichild;
            BCSYM_NamedRoot *pnamedChild;

            (void)bichild.Init(pnamed->PHash());

            while (pnamedChild = bichild.GetNext())
            {
                ResetFlags(pnamedChild, State);
            }
        }

        break;

    case SYM_Class:
    case SYM_Interface:
    case SYM_Namespace:
        {
            BCITER_CHILD_ALL bichild(pnamed->PContainer());
            BCSYM_NamedRoot *pnamedChild;

            while (pnamedChild = bichild.GetNext())
            {
                // Non-recursive, the callers will call this for all the containers
                // required
                //
                if (pnamedChild->IsContainer())
                {
                    continue;
                }

                pnamed->ResetBadness(State);
            }
        }

        break;
    }
}

#if DEBUG
bool
SymbolList::Verify()
{
    BCSYM_NamedRoot *Cursor = m_First;

	unsigned i;
    for (i = 0; i < m_Count; i++)
    {
        if (Cursor == NULL) break;
        Cursor = Cursor->GetNextInSymbolList();
    }

    bool Result = (Cursor == NULL && i == m_Count);
    VSASSERT(Result, "SymbolList integrity compromised.");
    return Result;
}

bool
SymbolList::Contains
(
    BCSYM_NamedRoot *Item
)
{
    BCSYM_NamedRoot *Current = m_First;

    while (Current)
    {
        if (Current == Item)
            return true;

        Current = Current->GetNextInSymbolList();
    }

    return false;
}
#endif

void
SymbolList::AddToFront
(
    _Out_ BCSYM_NamedRoot *NewItem
)
{
    if (this)
    {
        VSASSERT(Verify(), "");
        VSASSERT(NewItem, "addition of bad item to SymbolList");
        NewItem->SetNextInSymbolList(m_First);
        m_First = NewItem;
        m_Count++;
    }
}

void
SymbolList::AddToEnd
(
    _In_ BCSYM_NamedRoot *NewItem
)
{
    if (this)
    {
        VSASSERT(Verify(), "");
        VSASSERT(NewItem, "addition of bad item to SymbolList");

        if (!m_First)
        {
            m_First = NewItem;
        }
        else
        {
            BCSYM_NamedRoot *CurrentItem = m_First;

            while (CurrentItem->GetNextInSymbolList())
            {
                CurrentItem = CurrentItem->GetNextInSymbolList();
            }

            CurrentItem->SetNextInSymbolList(NewItem);
        }
        m_Count++;
    }
}

void
SymbolList::Remove
(
    BCSYM_NamedRoot *ItemToRemove
)
{
    if (this)
    {
        VSASSERT(Verify(), "");
        VSASSERT(ItemToRemove, "Can't remove null item!");

        if (ItemToRemove == m_First)
        {
            m_First = m_First->GetNextInSymbolList();
            m_Count--;
            return;
        }

        BCSYM_NamedRoot *CurrentItem = m_First->GetNextInSymbolList();
        BCSYM_NamedRoot *PreviousItem = m_First;

        while (CurrentItem)
        {
            if (CurrentItem == ItemToRemove)
            {
                PreviousItem->SetNextInSymbolList(CurrentItem->GetNextInSymbolList());
                m_Count--;
                return;
            }
            PreviousItem = CurrentItem;
            CurrentItem = CurrentItem->GetNextInSymbolList();
        }
    }

    VSFAIL("Couldn't find item!");
}

void
SymbolList::RemoveAliased
(
    BCSYM_NamedRoot *ItemToRemove
)
{
    if (this)
    {
        VSASSERT(Verify(), "");
        VSASSERT(ItemToRemove, "Can't remove null item!");

        if (m_First && m_First->IsAlias() && m_First->PAlias()->GetAliasedSymbol() == ItemToRemove)
        {
            m_First = m_First->GetNextInSymbolList();
            m_Count--;
            return;
        }

        BCSYM_NamedRoot *CurrentItem = m_First->GetNextInSymbolList();
        BCSYM_NamedRoot *PreviousItem = m_First;

        while (CurrentItem)
        {
            if (CurrentItem && CurrentItem->IsAlias() && CurrentItem->PAlias()->GetAliasedSymbol() == ItemToRemove)
            {
                PreviousItem->SetNextInSymbolList(CurrentItem->GetNextInSymbolList());
                m_Count--;
                return;
            }
            PreviousItem = CurrentItem;
            CurrentItem = CurrentItem->GetNextInSymbolList();
        }
    }

    VSFAIL("Couldn't find item!");
}

/**************************************************************************************************
;AccessOfFlags

Convert DECLFLAGS to an ACCESS
***************************************************************************************************/
ACCESS Symbols::AccessOfFlags(DECLFLAGS DeclFlags)
{
    switch ( DeclFlags & DECLF_AccessFlags )
    {
        case 0: // no flags = public
        case DECLF_Public:
            return ACCESS_Public;

        case DECLF_Private:
            return ACCESS_Private;

        case DECLF_Protected:
            return ACCESS_Protected;

        case DECLF_Friend:
            return ACCESS_Friend;

        case DECLF_ProtectedFriend:
            return ACCESS_ProtectedFriend;

        case DECLF_CompilerControlled:
            return ACCESS_CompilerControlled;

        default:
            VSFAIL("There can be only one.");
            return ACCESS_Public;
    }
}

/**************************************************************************************************
;MapAccessToDeclFlags

Maps an ACCESS enum to a DECLFLAGS, e.g. maps ACCESS_Public -> DECLF_Public
***************************************************************************************************/
DECLFLAGS // the mapped access flags
Symbols::MapAccessToDeclFlags
(
    ACCESS Access // [in] the access you want mapped to DECFLAGS
)
{
    switch ( Access )
    {
        case ACCESS_Public:
            return DECLF_Public;

        case ACCESS_Friend:
            return DECLF_Friend;

        case ACCESS_Protected:
            return DECLF_Protected;

        case ACCESS_ProtectedFriend:
            return DECLF_ProtectedFriend;

        case ACCESS_Private:
            return DECLF_Private;

        default:
            VSASSERT(false, "Unknown Access");
            return 0;
    }
}

#if DEBUG

//============================================================================
// Dump a symbol and the symbols it contains.
//============================================================================

extern const WCHAR *s_wszVOfVtype[];

static void DebIndent(unsigned cchIndent)
{
    unsigned i;

    for (i = 0; i < cchIndent; i++)
    {
        DebPrintf(" ");
    }
}

void DebShowLocationInfo(BCSYM *psym, bool fLocString)
{
    if (psym->HasLocation())
    {
        Location *ploc = psym->GetLocation();

        if (fLocString)
        {
            DebPrintf(", location = ");
        }

        DebPrintf("<%d,%d>-<%d,%d>",
                  ploc->m_lBegLine,
                  ploc->m_lBegColumn,
                  ploc->m_lEndLine,
                  ploc->m_lEndColumn);
    }
}

void Symbols::DebShowNamedRootFlags(BCSYM_NamedRoot *psym)
{
    DebPrintf("name = %S", psym->GetName());

    if (psym->GetName() != psym->GetEmittedName() && !psym->IsHash())
    {
        DebPrintf(", emitted name = %S", psym->GetEmittedName());
    }

    if (psym->GetImmediateParent())
    {
        BCSYM_NamedRoot *pnamedParent = psym->GetImmediateParent();

        if (pnamedParent->IsHash())
        {
            pnamedParent = pnamedParent->GetImmediateParent();
        }

        DebPrintf(", parent = %S", pnamedParent->GetName());

        if (psym->GetImmediateParent()->IsHash())
        {
            DebPrintf(", containing hash = %S", psym->GetImmediateParent()->GetName());
        }
    }

    if (psym->IsContainer() && psym->PContainer()->GetNameSpace())
    {
        DebPrintf(", namespace = %S", psym->PContainer()->GetNameSpace());
    }

    DebPrintf(", access = ");

    switch(psym->GetRawAccess())
    {
    case ACCESS_Public:
        DebPrintf("public");
        break;

    case ACCESS_Protected:
        DebPrintf("protected");
        break;

    case ACCESS_Friend:
        DebPrintf("friend");
        break;

    case ACCESS_Private:
        DebPrintf("private");
        break;

    case ACCESS_ProtectedFriend:
        DebPrintf("protected friend");
        break;
    }

    DebPrintf(", bindingspace = ");

    bool fFound = false;

    if (psym->GetBindingSpace() & BINDSPACE_Normal)
    {
        DebPrintf("normal");
        fFound = true;
    }
    if (psym->GetBindingSpace() & BINDSPACE_Type)
    {
        if (fFound)
        {
            DebPrintf(".");
        }

        DebPrintf("type");
        fFound = true;
    }

    if (!fFound)
    {
        DebPrintf("unbindable");
    }

    if (psym->IsShadowing())
    {
        DebPrintf(", isShadowing");
    }

    if (psym->IsShadowsKeywordUsed())
    {
        DebPrintf(", isShadowsKeywordUsed");
    }

    DebShowLocationInfo(psym, true);
}

void Symbols::DebShowSymbolReference(Compiler *pCompiler, BCSYM *psym, unsigned cchIndent)
{
    if (psym->IsNamedRoot())
    {
        BCSYM_NamedRoot *pnamed = psym->PNamedRoot();

        DebIndent(cchIndent);
    
        DebPrintf("reference to %S [kind = SYM_%S]", pnamed->GetName(), SymbolSizeInfo[pnamed->GetSkKind()].m_sz);
    }
    else
    {
        DebShowSymbols(psym, pCompiler, cchIndent);
    }
}

static void DebShowContainedMembers(Compiler *pCompiler, BCSYM_Container *pcontainer, unsigned cchIndent)
{
    BCSYM_Hash *psym;

    psym = pcontainer->GetHash();

    Symbols::DebShowSymbols(psym, pCompiler, cchIndent);
}


void Symbols::DebShowProcFlags(BCSYM_Proc *psym)
{
    DebShowNamedRootFlags(psym);

    /* if (psym->m_isUserDefinedOperator)
    {
        DebPrintf(", isUserDefinedOperator");
    } */

    if (psym->IsShared())
    {
        DebPrintf(", isShared");
    }

    if (psym->IsOverloads())
    {
        DebPrintf(", isOverloads");
    }

    if (psym->IsOverloadsKeywordUsed())
    {
        DebPrintf(", isOverloadsKeywordUsed");
    }

    if (psym->GetOverriddenProcRaw())
    {
        DebPrintf(", isOverrides");
    }

    if (psym->IsOverridesKeywordUsed())
    {
        DebPrintf(", isOverridesKeywordUsed");
    }

    if (psym->IsNotOverridableKeywordUsed())
    {
        DebPrintf(", isNotOverridableKeywordUsed");
    }

    if (psym->IsOverridableKeywordUsed())
    {
        DebPrintf(", isOverridableKeywordUsed");
    }

    if (psym->IsMustOverrideKeywordUsed())
    {
        DebPrintf(", isMustOverrideKeywordUsed");
    }
}

void Symbols::DebShowProc(Compiler *pCompiler, BCSYM_Proc *psym, unsigned cchIndent)
{
    if (psym->GetRawType())
    {
        DebIndent(cchIndent);
        DebPrintf("returns ");

        DebShowSymbolReference(pCompiler, psym->GetRawType(), 0);
        DebPrintf("\n");
    }

    BCSYM_Param *pparam = psym->GetFirstParam();

    if (pparam)
    {
        for (; pparam; pparam = pparam->GetNext())
        {
            DebShowSymbols(pparam, pCompiler, cchIndent);
        }
    }
}

void Symbols::DebShowVarFlags(BCSYM_Variable *psym)
{
    DebShowNamedRootFlags(psym);

    DebPrintf(", varkind = ");

    switch(psym->GetVarkind())
    {
    case VAR_Const:
        DebPrintf("const");
        break;

    case VAR_Member:
        DebPrintf("member");
        break;

    case VAR_WithEvents:
        DebPrintf("withevents");
        break;

    case VAR_Local:
        DebPrintf("local");
        break;

    case VAR_Param:
        DebPrintf("param");
        break;

    case VAR_FunctionResult:
        DebPrintf("function result");
        break;

    default:
        VSFAIL("bad kind");
        break;
    }

    if (psym->IsStatic())
    {
        DebPrintf(", isStatic");
    }

    if (psym->IsShared())
    {
        DebPrintf(", isShared");
    }

    if (psym->IsNew())
    {
        DebPrintf(", isNew");
    }

    if (!psym->IsExplicitlyTyped())
    {
        DebPrintf(", notTyped");
    }

    if (psym->IsImplicitDecl())
    {
        DebPrintf(", notDecled");
    }
}

void Symbols::DebShowSymbols(BCSYM *psymbol, Compiler *pCompiler, unsigned cchIndent)
{
    VSASSERT(psymbol != NULL, "Bad symbol tree.");

    if (!psymbol)
    {
        return;
    }

    DebIndent(cchIndent);
    
    switch(psymbol->GetSkKind())
    {

    case SYM_Uninitialized:
    case SYM_CCContainer:
    case SYM_HandlesList:
    case SYM_ImplementsList:
    case SYM_NamespaceRing:
    case SYM_XmlName:
    case SYM_XmlNamespace:
    case SYM_ApplAttr:
    case SYM_GenericParam:
    case SYM_GenericConstraint:
    case SYM_GenericNonTypeConstraint:
    case SYM_GenericTypeConstraint:
    case SYM_GenericBinding:
    case SYM_GenericTypeBinding:
        VSFAIL("NYI");
        break;

    //========================================================================
    // These symbols should be printed onto the same line as their parent.
    //========================================================================

    case SYM_PointerType:

        VSASSERT(cchIndent == 0, "symbol dump error");

        {
            BCSYM_PointerType *ptyp = psymbol->PPointerType();

            DebPrintf("pointer [vtype = %S] to a ", s_wszVOfVtype[ptyp->GetVtype()]);

            DebShowSymbolReference(pCompiler, ptyp->GetRawRoot(), 0);
        }
        break;


    case SYM_ArrayLiteralType:
    case SYM_ArrayType:

        VSASSERT(cchIndent == 0, "symbol dump error");

        {
            BCSYM_ArrayType *ptyp = psymbol->PArrayType();

            DebPrintf("array [%d dimensional] of ", ptyp->GetRank());

            DebShowSymbolReference(pCompiler, ptyp->GetRoot(), 0);
        }
        break;

    case SYM_VoidType:

        VSASSERT(cchIndent == 0, "symbol dump error");

        DebPrintf("void");
        break;

    case SYM_NamedType:

        VSASSERT(cchIndent == 0, "symbol dump error");

        {
            BCSYM_NamedType *ptyp = psymbol->PNamedType();

            DebPrintf("named type [cNames = %d, context = %S, names = ", ptyp->GetNameCount(), ptyp->GetContext()->GetName());

            unsigned iName, cNames = ptyp->GetNameCount();
            STRING **rgpstr = ptyp->GetNameArray();

            for (iName = 0; iName < cNames; iName++)
            {
                if (iName != 0)
                {
                    DebPrintf(".");
                }

                DebPrintf("%S", rgpstr[iName]);
            }

            DebShowLocationInfo(ptyp, true);
            DebPrintf("] ");

            if (ptyp->GetSymbol())
            {
                DebPrintf("to ");
                DebShowSymbolReference(pCompiler, ptyp->GetSymbol(), 0);
            }
        }

        break;

    case SYM_Expression:

        VSASSERT(cchIndent == 0, "symbol dump error");

        {
            BCSYM_Expression *psym = psymbol->PExpression();

            DebPrintf("expression [");

            if (psym->GetReferringDeclaration())
            {
                DebPrintf("context = %S, ", psym->GetReferringDeclaration()->GetName());
            }

            DebPrintf("expression = ");

            #pragma prefast(suppress: 28649, "The pointer can be overwritten, special way of handling this string.")
            if (psym->GetExpressionTextRaw()[0])
            {
                DebPrintf("\"%S\"", psym->GetExpressionTextRaw());
            }
            else
            {
                DebShowLocationInfo(psym, false);
            }

            DebPrintf("]");
        }
        break;

    case SYM_Alias:
        {
            BCSYM_Alias *psym = psymbol->PAlias();

            DebPrintf("alias [");
            DebShowNamedRootFlags(psym);
            DebPrintf("] to ");

            DebShowSymbolReference(pCompiler, psym->GetAliasedSymbol(), 0);
        }

        break;

    //========================================================================
    // All of these symbols should be put on their own line.
    //========================================================================

    case SYM_GenericBadNamedRoot:
    case SYM_TypeForwarder:
        {
            BCSYM_NamedRoot *pbad = psymbol->PNamedRoot();

            VSASSERT(pbad->IsBad(), "how can this not be bad?");

            DebPrintf("bad named root [");
            DebShowNamedRootFlags(pbad);
            DebPrintf(", errid = %d]\n", pbad->GetErrid());
        }
        break;

    case SYM_Implements:
        {
            BCSYM_Implements *psym = psymbol->PImplements();

            DebPrintf("implements [name = %S", psym->GetName());
            DebShowLocationInfo(psym, true);
            DebPrintf("] to ");
            DebShowSymbolReference(pCompiler, psym->GetRawRoot(), 0);

            DebPrintf("\n");
        }

        break;

    case SYM_Namespace:
        {
            BCSYM_Namespace *psym = psymbol->PNamespace();

            psym->EnsureChildrenLoaded();

            DebPrintf("namespace [");
            DebShowNamedRootFlags(psym);

            DebPrintf("]\n");

            DebIndent(cchIndent);
            DebPrintf("{\n");

            DebShowContainedMembers(pCompiler, psym, cchIndent + 2);

            DebIndent(cchIndent);
            DebPrintf("}\n");
        }
        break;

    case SYM_Hash:
        {
            BCSYM_Hash *psym = psymbol->PHash();
            HRESULT hr = NOERROR;

            DebPrintf("hash [");
            DebShowNamedRootFlags(psym);
            DebPrintf("]\n");

            DebIndent(cchIndent);
            DebPrintf("{\n");

            {
                // Manually walk the hash table.
                unsigned iBucket, cBuckets = psym->CBuckets();
                BCSYM_NamedRoot **rgpnamed = psym->Rgpnamed();
                BCSYM_NamedRoot *pnamedBucket;

                for (iBucket = 0; iBucket < cBuckets; iBucket++)
                {
                    for (pnamedBucket = rgpnamed[iBucket]; pnamedBucket; pnamedBucket = pnamedBucket->GetNextInHash())
                    {
                        DebShowSymbols(pnamedBucket, pCompiler, cchIndent + 2);
                    }
                }
            }
            DebIndent(cchIndent);
            DebPrintf("}\n");
        }
        break;

    case SYM_Class:
        {
            BCSYM_Class *pClassSymbol = psymbol->PClass();

            pClassSymbol->EnsureChildrenLoaded();

            if (pClassSymbol->IsEnum())
            {
                DebPrintf("enum [");
                DebShowNamedRootFlags(pClassSymbol);
                DebPrintf("]\n");

                DebIndent(cchIndent);
                DebPrintf("{\n");

                DebShowContainedMembers(pCompiler, pClassSymbol, cchIndent + 2);

                DebIndent(cchIndent);
                DebPrintf("}\n");
            }
            else
            {
                if (!pClassSymbol->IsDelegate())
                    DebPrintf("class [");
                else
                    DebPrintf("delegate [");

                DebShowNamedRootFlags(pClassSymbol);

                DebPrintf(", comparekind = ");

                if (pClassSymbol->IsNotInheritable()) 
                {
                    DebPrintf(", isNotInheritable");
                }

                if (pClassSymbol->IsMustInherit()) // class marked as 'BaseOnly', i.e. abstract
                {
                    DebPrintf(", BaseOnly");
                }
                
                if (pClassSymbol->AreMustOverridesSatisfied()) 
                {
                    DebPrintf(", AllMustOverridesSatisfied" );
                }
                
                if (pClassSymbol->IsStdModule()) 
                {
                    DebPrintf(", StandardModule");
                }

                DebPrintf("]\n");
                DebIndent(cchIndent);
                DebPrintf("{\n");

                if (pClassSymbol->GetRawBase()) 
                {
                    DebIndent(cchIndent + 2);

                    DebPrintf("base is ");
                    DebShowSymbolReference(pCompiler, pClassSymbol->GetRawBase(), 0);
                    DebPrintf("\n");
                }

                if (pClassSymbol->GetDefaultProperty())
                {
                    DebIndent(cchIndent + 2);

                    DebPrintf("default property is ");
                    DebShowSymbolReference(pCompiler, pClassSymbol->GetDefaultProperty(), 0);
                    DebPrintf("\n");
                }
                
                if (pClassSymbol->GetMe())
                {
                    DebIndent(cchIndent + 2);
                    DebPrintf("me is ");
                    DebShowSymbols(pClassSymbol->GetMe(), pCompiler, 0);
                    DebPrintf("\n");
                }

                if (pClassSymbol->GetImpList())
                {
                    BCSYM_Implements *----l = pClassSymbol->GetImpList();

                    for (; ----l; ----l = ----l->GetNext())
                    {
                        DebShowSymbols(----l, pCompiler, cchIndent + 2);
                    }

                    DebPrintf("\n");
                }

                DebShowContainedMembers(pCompiler, pClassSymbol, cchIndent + 2);

                DebIndent(cchIndent);
                DebPrintf("}\n");
            }
        }
        break;

    case SYM_Interface:
        {
            BCSYM_Interface *psym = psymbol->PInterface();

            psym->EnsureChildrenLoaded();

            DebPrintf("interface [");
            DebShowNamedRootFlags(psym);
            DebPrintf("]\n");

            DebIndent(cchIndent);
            DebPrintf("{\n");

            if (psym->GetImpList())
            {
                BCSYM_Implements *----l = psym->GetImpList();

                for (; ----l; ----l = ----l->GetNext())
                {
                    DebShowSymbols(----l, pCompiler, cchIndent + 2);
                }

                DebPrintf("\n");
            }

            if (psym->GetDefaultProperty())
            {
                DebIndent(cchIndent + 2);

                DebPrintf("default property is ");
                DebShowSymbolReference(pCompiler, psym->GetDefaultProperty(), 0);
                DebPrintf("\n\n");
            }

            DebShowContainedMembers(pCompiler, psym, cchIndent + 2);

            DebIndent(cchIndent);
            DebPrintf("}\n");
        }
        break;

    case SYM_Property:
        {
            BCSYM_Property *psym = psymbol->PProperty();

            DebPrintf("property [");
            DebShowProcFlags(psym);
            DebPrintf("]\n");

            DebIndent(cchIndent);
            DebPrintf("{\n");

            if (psym->GetRawType() || psym->GetFirstParam())
            {
                DebPrintf("\n");
                DebShowProc(pCompiler, psym, cchIndent + 2);
            }

            DebIndent(cchIndent);
            DebPrintf("}\n");
        }
        break;

    case SYM_MethodImpl:
        {
            BCSYM_MethodImpl *psym = psymbol->PMethodImpl();

            DebPrintf("method impl [");
            DebShowProcFlags(psym);

            DebPrintf(", uLineBody = %d-%d, oBody = %d-%d]\n",
                      psym->GetCodeBlock()->m_lBegLine,
                      psym->GetCodeBlock()->m_lEndLine,
                      psym->GetCodeBlock()->m_oBegin,
                      psym->GetCodeBlock()->m_oEnd);

            DebIndent(cchIndent);
            DebPrintf("{\n");

            if (psym->GetRawType() || psym->GetFirstParam())
            {
                DebPrintf("\n");
                DebShowProc(pCompiler, psym, cchIndent + 2);
            }

            DebIndent(cchIndent);
            DebPrintf("}\n");
        }
        break;

    case SYM_MethodDecl:
        {
            BCSYM_MethodDecl *psym = psymbol->PMethodDecl();

            DebPrintf("method decl [");
            DebShowProcFlags(psym);
            DebPrintf("]\n");

            DebIndent(cchIndent);
            DebPrintf("{\n");

            DebShowProc(pCompiler, psym, cchIndent + 2);

            DebIndent(cchIndent);
            DebPrintf("}\n");
        }
        break;

    case SYM_UserDefinedOperator:
        {
            BCSYM_UserDefinedOperator *psym = psymbol->PUserDefinedOperator();

            DebPrintf("operator decl [");
            DebShowProcFlags(psym);
            DebPrintf("]\n");

            DebIndent(cchIndent);
            DebPrintf("{\n");

            DebShowProc(pCompiler, psym, cchIndent + 2);

            DebIndent(cchIndent);
            DebPrintf("}\n");
        }
        break;

    case SYM_SyntheticMethod:
        {
            BCSYM_SyntheticMethod *psym = psymbol->PSyntheticMethod();

            DebPrintf("synthetic method [");
            DebShowProcFlags(psym);
            DebPrintf("]\n");

            DebIndent(cchIndent);
            DebPrintf("{\n");

            if (psym->GetRawType() || psym->GetFirstParam())
            {
                DebShowProc(pCompiler, psym, cchIndent + 2);
                DebPrintf("\n");
            }

            DebPrintf("[][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]\r\n");
            DebPrintf("%S", psym->GetCode());
            DebPrintf("[][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]\r\n");

            DebIndent(cchIndent);
            DebPrintf("}\n");
        }
        break;

    case SYM_EventDecl:
        {
            BCSYM_EventDecl *psym = psymbol->PEventDecl();
            bool fNeedMoreLines = psym->GetRawType() || psym->GetFirstParam();

            DebPrintf("event decl [");
            DebShowProcFlags(psym);
            DebPrintf("]\n");

            DebIndent(cchIndent);
            DebPrintf("{\n");

            DebShowProc(pCompiler, psym, cchIndent + 2);

            if (psym->GetRawDelegate())
            {
                if (fNeedMoreLines)
                {
                    DebPrintf("\n");
                }

                DebIndent(cchIndent + 2);

                DebPrintf("delegate type is ");
                DebShowSymbolReference(pCompiler, psym->GetRawDelegate(), 0);
                DebPrintf("\n");

                fNeedMoreLines = true;
            }

            if (psym->GetProcAdd() || psym->GetProcRemove() || psym->GetProcFire())
            {
                if (fNeedMoreLines)
                {
                    DebPrintf("\n");
                }

                if (psym->GetProcAdd())
                {
                    DebIndent(cchIndent + 2);
                    DebPrintf("event add is ");
                    DebShowSymbolReference(pCompiler, psym->GetProcAdd(), 0);
                    DebPrintf("\n");
                }

                if (psym->GetProcRemove())
                {
                    DebIndent(cchIndent + 2);
                    DebPrintf("event remove is ");
                    DebShowSymbolReference(pCompiler, psym->GetProcRemove(), 0);
                    DebPrintf("\n");
                }

                if (psym->GetProcFire())
                {
                    DebIndent(cchIndent + 2);
                    DebPrintf("event fire is ");
                    DebShowSymbolReference(pCompiler, psym->GetProcFire(), 0);
                    DebPrintf("\n");
                }
            }

            DebIndent(cchIndent);
            DebPrintf("}\n");
        }
        break;

    case SYM_DllDeclare:
        {
            BCSYM_DllDeclare *psym = psymbol->PDllDeclare();

            DebPrintf("dll declare [");
            switch (psym->GetDeclareType())
            {
                case DECLARE_Unicode:
                    DebPrintf("unicode ");
                    break;

                case DECLARE_Auto:
                    DebPrintf("auto ");
                    break;

                default:
                case DECLARE_Ansi:
                    DebPrintf("ansi ");
                    break;
            }

            DebShowProcFlags(psym);

            DebPrintf(", lib = %S, alias = %s]\n", psym->GetLibName(), psym->GetAliasName());

            DebIndent(cchIndent);
            DebPrintf("{\n");

            DebShowProc(pCompiler, psym, cchIndent + 2);

            DebIndent(cchIndent);
            DebPrintf("}\n");

        }
        break;

    case SYM_Param:
    case SYM_ParamWithValue:
        {
            BCSYM_Param *psym = psymbol->PParam();

            DebPrintf("param [");
            DebPrintf("name = %S", psym->GetName());

            if (psym->IsOptional())
            {
                DebPrintf(", isOptional");
            }
            if (psym->IsParamArrayRaw())
            {
                DebPrintf(", isParamArray");
            }

            DebShowLocationInfo(psym, true);
            DebPrintf("] as ");
            DebShowSymbolReference(pCompiler, psym->GetRawType(), 0);

            if (psym->IsParamWithValue())
            {
                DebPrintf("= ");
                DebShowSymbols(psym->PParamWithValue()->GetExpression(), pCompiler, 0);
            }

            DebPrintf("\n");
        }

        break;

    case SYM_Variable:
    case SYM_VariableWithValue:
    case SYM_VariableWithArraySizes:
    case SYM_StaticLocalBackingField:
    case SYM_CCConstant:
        {
            BCSYM_Variable *psym = psymbol->PVariable();

            DebPrintf("variable [");
            DebShowVarFlags(psym);
            DebPrintf("] as ");
            DebShowSymbolReference(pCompiler, psym->GetRawType(), 0);

            if (psym->IsVariableWithValue())
            {
                DebPrintf("= ");
                DebShowSymbols(psym->PVariableWithValue()->GetExpression(), pCompiler, 0);
            }
            DebPrintf("\n");
        }
        break;

    default:
        VSFAIL("NYI");
        break;
    }

    return;
}

void Symbols::DebShowBasicRep
(
    Compiler *pCompiler,
    BCSYM *psymbol,
    unsigned cchIndent
)
{
    StringBuffer wbuffer;
    char buff[256];         // for debug purposes assume this is the max len
    HRESULT hr = NOERROR;

    DebIndent(cchIndent);

    psymbol->GetBasicRep(pCompiler, NULL, &wbuffer);
    WszCpyToSz(buff, wbuffer.GetString());
    DebPrintf(buff);
    DebPrintf("\n");

    if (psymbol->IsContainer())
    {
        BCITER_CHILD_SORTED bcIterator(psymbol->PContainer());
        BCSYM_NamedRoot * pNamedSym;
        pNamedSym = bcIterator.GetNext();

        while( pNamedSym )
        {
            Symbols::DebShowBasicRep(pCompiler, pNamedSym->DigThroughAlias(), cchIndent + 2);
            pNamedSym = bcIterator.GetNext();
    }
    }
}

#endif  // DEBUG

void GenericBindingCache::Add(unsigned long Key, BCSYM_GenericBinding *Binding)
{
    // Create the hash if not already present.
    // Lazy create here to avoid memory hit for cases where no bindings get created.
    //
    if (!m_Hash)
    {
        m_Hash = new (*m_Allocator) HashForGenericBindingCache(m_Allocator);
    }

#if _WIN64
    m_Hash->HashAdd((size_t&)Key, Binding);
#else
    m_Hash->HashAdd(Key, Binding);
#endif
}

BCSYM *DigThroughNamedTypeIfPossible(BCSYM *PossibleNamedType)
{
    if (PossibleNamedType &&
        PossibleNamedType->IsNamedType() &&
        PossibleNamedType->PNamedType()->GetSymbol())
    {
        return PossibleNamedType->PNamedType()->GetSymbol();
    }

    return PossibleNamedType;
}

BCSYM_GenericBinding* GenericBindingCache::Find
(
    unsigned long Key,
    BCSYM_NamedRoot *Generic,
    _In_count_(ArgumentCount) BCSYM *Arguments[],
    unsigned ArgumentCount,
    BCSYM_GenericTypeBinding *ParentBinding
)
{
    if (!m_Hash)
    {
        return NULL;
    }

#if _WIN64
    for (BCSYM_GenericBinding **Symbol = m_Hash->HashFind((size_t&)Key);
#else
    for (BCSYM_GenericBinding **Symbol = m_Hash->HashFind(Key);
#endif
         Symbol;
         Symbol = m_Hash->FindNext())
    {
        BCSYM_GenericBinding *CachedBinding = *Symbol;

        if (!CachedBinding ||
            CachedBinding->IsBadGenericBinding())
        {
            continue;
        }

        if (CachedBinding->GetGeneric() == Generic &&
            CachedBinding->GetParentBinding() == ParentBinding &&
            CachedBinding->GetArgumentCount() == ArgumentCount)
        {
            for (unsigned Index = 0; Index < ArgumentCount; Index++)
            {
                // fast type compare
                //
                if (DigThroughNamedTypeIfPossible(CachedBinding->GetArguments()[Index]) !=
                        DigThroughNamedTypeIfPossible(Arguments[Index]))
                {
                    goto ArgumentMismatch;
                }
            }

            return CachedBinding;

ArgumentMismatch:;
        }

    }

    return NULL;
}

// This hash method was stolen from our string pool and works
// really good for strings.  We need to verify that it's ok
// for binary ---- as well.
//
unsigned long ComputeKeyForBlob(unsigned long CurrentKey, _In_ void *Blob, unsigned cbSize)
{
    VSASSERT(cbSize % WORD_BYTE_SIZE  == 0, "Must be 4 byte thing.");

    unsigned long hash = CurrentKey;
    unsigned long l;
    unsigned long *pl = (unsigned long *)Blob;

    unsigned long cl = cbSize / WORD_BYTE_SIZE;

    for (; 0 < cl; -- cl, ++ pl)
    {
        l = *pl;

        hash = _lrotl(hash, 2) + ((l >> 9) + l) * 0x10004001;
    }

    return hash;
}

unsigned long ComputeKeyForSymbol(unsigned long CurrentKey, BCSYM *Symbol)
{
    return ComputeKeyForBlob(CurrentKey, &Symbol, sizeof(Symbol));
}

// returns true if it could compute a valid signature, else returns false
//
bool GenericBindingCache::ComputeKeyForGenericBinding
(
    BCSYM_NamedRoot *Generic,
    _In_count_(ArgumentCount) BCSYM *Arguments[],
    unsigned ArgumentCount,
    BCSYM_GenericTypeBinding *ParentBinding,
    _Out_ unsigned long &Key
)
{
    if (Generic->IsBad())
    {
        return false;
    }

    Key = 0;

    for (unsigned Index = 0; Index < ArgumentCount; Index++)
    {
        BCSYM *Argument = Arguments[Index];

        if (!Argument ||
            Argument->IsBad())
        {
            return false;
        }

        Argument = DigThroughNamedTypeIfPossible(Argument);

        // don't cache binding with unresolved type argument
        // because these are going to be very rare
        //
        if (Argument->IsNamedType())
        {
            return false;
        }

        // Array type arguments diable caching of bindings
        // because every use of an array creates a different
        // symbol and so signature would not compare equal
        // anyway. We could possibly do this in a smarter
        // way, by defining the signature defferently, but
        // probably not worth the trouble. So will only this
        // if in the future we determine that this will be
        // useful.
        //
        if (Argument->IsArrayType())
        {
            return false;
        }

        Key = ComputeKeyForSymbol(Key, Argument);
    }

#if _WIN64
    {
        size_t ArgCount = ArgumentCount;

        Key = ComputeKeyForBlob(Key, &ArgCount, sizeof(ArgCount));
    }
#else
    Key = ComputeKeyForBlob(Key, &ArgumentCount, sizeof(ArgumentCount));
#endif

    Key = ComputeKeyForSymbol(Key, Generic);
    Key = ComputeKeyForSymbol(Key, ParentBinding);

    return true;
}

#if 0
// Avoid modifying the symbol table for the following reasons:
//  - Mainly in order to avoid threading issue when the foreground thread is
//      accessing the symbol table.
//  - An additional benefit from not modifying the symbol table, there are
//      no complex decompilation issues to deal with.
//

void Symbols::InsertTransientClassInSymbolTable(BCSYM_Class *pClass)
{
    AssertIfNull(pClass->GetContainer());
    AssertIfNull(pClass->GetImmediateParent());

    BCSYM_Container *pParent = pClass->GetContainer();
    BCSYM_Hash *pParentHash = pClass->GetImmediateParent()->PHash();

    // Add to parent's hash
    AddSymbolToHash(pParentHash, pClass, true, pParent->IsClass() && pParent->PClass()->IsStdModule(), pParent->IsNamespace());

    // Add to parent's nested types list
    pParent->AddToNestedTypesList(pClass);

    // Add to file level type symbol list
    CompilerFile *pContainingFile = pParent->GetCompilerFile();
    AssertIfFalse(pContainingFile);

    if (pContainingFile)
    {
        pContainingFile->GetNamespaceLevelSymbolList()->AddToFront(
                                                            GetAliasOfSymbol(
                                                            pClass,
                                                            pClass->GetAccess(),
                                                            NULL));
    }
}

void Symbols::RemoveTransientClassFromSymbolTable(BCSYM_Class *pClass)
{
    BCSYM_Container *pParent = pClass->GetContainer();

    AssertIfNull(pParent);

    // Remove from parent's hash.
    AssertIfFalse(pClass->m_pnamedParent->IsHash());
    RemoveSymbolFromHash(pClass->m_pnamedParent->PHash(), pClass);

    // Remove from parent's nested types list.
    pParent->RemoveFromNestedTypesList(pClass);

    // Remove from file level type list.
    CompilerFile *pFile = pParent->GetCompilerFile();
    AssertIfNull(pFile);
    if (pFile)
    {
        pFile->GetNamespaceLevelSymbolList()->RemoveAliased(pClass);
    }
}

void Symbols::InsertTransientMemberInSymbolTable(BCSYM_NamedRoot *pMember)
{
    AssertIfNull(pMember->GetContainer());
    AssertIfNull(pMember->GetImmediateParent());

    BCSYM_Container *pParent = pMember->GetContainer();
    BCSYM_Hash *pParentHash = pMember->GetImmediateParent()->PHash();

    // Add to parent's hash
    AddSymbolToHash(pParentHash, pMember, true, pParent->IsClass() && pParent->PClass()->IsStdModule(), pParent->IsNamespace());
}

void Symbols::RemoveTransientMemberFromSymbolTable(BCSYM_NamedRoot *pMember)
{
    // Remove from parent's hash.
    AssertIfFalse(pMember->m_pnamedParent && pMember->m_pnamedParent->IsHash());
    RemoveSymbolFromHash(pMember->m_pnamedParent->PHash(), pMember);
}

void Symbols::InsertTransientSymbolInSymbolTable(BCSYM_NamedRoot *pSymbol)
{
    AssertIfNull(pSymbol);

    if (pSymbol->IsClass())
    {
        AssertIfFalse(pSymbol->IsTransientClass());
        InsertTransientClassInSymbolTable(pSymbol->PClass());
    }
    else
    {
        InsertTransientMemberInSymbolTable(pSymbol);
    }
}

void Symbols::RemoveTransientSymbolFromSymbolTable(BCSYM_NamedRoot *pSymbol)
{
    AssertIfNull(pSymbol);

    if (pSymbol->IsClass())
    {
        AssertIfFalse(pSymbol->IsTransientClass());
        RemoveTransientClassFromSymbolTable(pSymbol->PClass());
    }
    else
    {
        RemoveTransientMemberFromSymbolTable(pSymbol);
    }
}
#endif 0
