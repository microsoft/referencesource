//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implementation of VB compiler symbol table.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

#if HOSTED
#include "VBHostedInterop.h"
#endif

#if IDE 
extern bool g_fRunningIntellisenseTest;
#endif IDE

extern const WCHAR *g_wszFullCLRNameOfVtype[];

#if TRACK_BCSYMHASH
ULONG g_HashLookups = 0;
ULONG g_LookupIterations = 0;
ULONG g_LookupHits = 0;
ULONG g_LookupMisses = 0;
#endif

#if DEBUG

#define MAKEBKI(type, stype, named, container, isclass, mem, proc, var, varvalue, decl, param, hasDerivers, expr, impl, GenConstraint, gen, badRoot, array, kind)  \
    { type, stype, named, container, isclass, mem, proc, var, varvalue, decl, param, hasDerivers, expr, impl, GenConstraint, gen, badRoot, array, kind },

#else // !DEBUG

#define MAKEBKI(type, stype, named, container, isclass, mem, proc, var, varvalue, decl, param, hasDerivers, expr, impl, GenConstraint, gen, badRoot, array, kind)  \
    { type, stype, named, container, isclass, mem, proc, var, varvalue, decl, param, hasDerivers ,expr, impl, GenConstraint, gen, badRoot, array },

#endif // !DEBUG

const BilkindInfo BCSYM::s_rgBilkindInfo[] = {

// Each column indicates whether the symbol being defined in that row supports the behavior listed in the
// column header.  Typically this means that the type is derived directly or indirectly from the base class
// related to the column header, e.g. SYM_MethodImpl has the Decl bit set because it supports the behaviors
// of SYM_MethodDecl ( i.e. it is derived from BCSYM_MethodDecl ).  See the struct BilkindInfo {} definition
// in bcsym.h for the members that these bits are associated with.  Note: This table must be ordered the same
// as the order the DEF_BCSYM's are done in biltypes.h since this table is indexed by SYM_* enums

//        Type  SType Named Container IsClass Mbr Proc Var VarValue Decl Param Derivers Expr Impl GenConst gen badRoot Array   kind
    MAKEBKI(0,    0,    0,    0,        0,     0,  0,   0,  0,       0,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_Uninitialized)
    MAKEBKI(1,    1,    0,    0,        0,     0,  0,   0,  0,       0,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_VoidType)
    MAKEBKI(1,    1,    0,    0,        0,     0,  0,   0,  0,       0,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_PointerType)
    MAKEBKI(1,    1,    0,    0,        0,     0,  0,   0,  0,       0,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_NamedType)
    MAKEBKI(1,    0,    1,    0,        0,     0,  0,   0,  0,       0,   0,    1,       0,   0,    0,      0,  1,       0,    SYM_GenericBadNamedRoot)
    MAKEBKI(0,    0,    1,    0,        0,     0,  0,   0,  0,       0,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_Hash)
    MAKEBKI(0,    0,    1,    0,        0,     0,  0,   0,  0,       0,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_Alias)
    MAKEBKI(0,    0,    1,    1,        0,     0,  0,   0,  0,       0,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_CCContainer)
    MAKEBKI(1,    0,    1,    1,        1,     0,  0,   0,  0,       0,   0,    1,       0,   0,    0,      0,  0,       0,    SYM_Class)
    MAKEBKI(0,    0,    1,    0,        0,     1,  1,   0,  0,       1,   0,    0,       0,   1,    0,      0,  0,       0,    SYM_MethodImpl)
    MAKEBKI(0,    0,    1,    0,        0,     1,  1,   0,  0,       0,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_SyntheticMethod)
    MAKEBKI(0,    0,    1,    0,        0,     1,  1,   0,  0,       1,   0,    1,       0,   0,    0,      0,  0,       0,    SYM_MethodDecl)
    MAKEBKI(0,    0,    1,    0,        0,     1,  1,   0,  0,       1,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_EventDecl)
    MAKEBKI(0,    0,    1,    0,        0,     1,  1,   0,  0,       0,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_DllDeclare)
    MAKEBKI(0,    0,    0,    0,        0,     0,  0,   0,  0,       0,   1,    1,       0,   0,    0,      0,  0,       0,    SYM_Param)
    MAKEBKI(0,    0,    0,    0,        0,     0,  0,   0,  0,       0,   1,    0,       0,   0,    0,      0,  0,       0,    SYM_ParamWithValue)
    MAKEBKI(0,    0,    1,    0,        0,     1,  0,   1,  0,       0,   0,    1,       0,   0,    0,      0,  0,       0,    SYM_Variable)
    MAKEBKI(0,    0,    1,    0,        0,     1,  0,   1,  1,       0,   0,    1,       0,   0,    0,      0,  0,       0,    SYM_VariableWithValue)
    MAKEBKI(0,    0,    1,    0,        0,     1,  0,   1,  0,       0,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_VariableWithArraySizes)
    MAKEBKI(0,    0,    1,    0,        0,     1,  0,   1,  1,       0,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_CCConstant)
    MAKEBKI(0,    0,    1,    0,        0,     1,  0,   1,  0,       0,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_StaticLocalBackingField)
    MAKEBKI(0,    0,    0,    0,        0,     0,  0,   0,  0,       0,   0,    0,       1,   0,    0,      0,  0,       0,    SYM_Expression)
    MAKEBKI(0,    0,    0,    0,        0,     0,  0,   0,  0,       0,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_Implements)
    MAKEBKI(1,    0,    1,    1,        0,     0,  0,   0,  0,       0,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_Interface)
    MAKEBKI(0,    0,    0,    0,        0,     0,  0,   0,  0,       0,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_HandlesList)
    MAKEBKI(0,    0,    0,    0,        0,     0,  0,   0,  0,       0,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_ImplementsList)
    MAKEBKI(0,    0,    1,    1,        0,     0,  0,   0,  0,       0,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_Namespace)
    MAKEBKI(0,    0,    1,    0,        0,     0,  0,   0,  0,       0,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_NamespaceRing)
    MAKEBKI(0,    0,    1,    0,        0,     0,  0,   0,  0,       0,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_XmlName)
    MAKEBKI(0,    0,    1,    0,        0,     0,  0,   0,  0,       0,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_XmlNamespaceDeclaration)
    MAKEBKI(0,    0,    1,    1,        0,     0,  0,   0,  0,       0,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_XmlNamespace)
    MAKEBKI(0,    0,    1,    0,        0,     1,  1,   0,  0,       0,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_Property)
    MAKEBKI(1,    1,    0,    0,        0,     0,  0,   0,  0,       0,   0,    1,       0,   0,    0,      0,  0,       1,    SYM_ArrayType)
    MAKEBKI(1,    1,    0,    0,        0,     0,  0,   0,  0,       0,   0,    0,       0,   0,    0,      0,  0,       1,    SYM_ArrayLiteralType)
    MAKEBKI(0,    0,    0,    0,        0,     0,  0,   0,  0,       0,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_ApplAttr)
    MAKEBKI(0,    0,    1,    0,        0,     1,  1,   0,  0,       1,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_UserDefinedOperator)
    MAKEBKI(1,    0,    1,    0,        0,     0,  0,   0,  0,       0,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_GenericParam)
    MAKEBKI(0,    0,    0,    0,        0,     0,  0,   0,  0,       0,   0,    1,       0,   0,    1,      0,  0,       0,    SYM_GenericConstraint)
    MAKEBKI(0,    0,    0,    0,        0,     0,  0,   0,  0,       0,   0,    0,       0,   0,    1,      0,  0,       0,    SYM_GenericTypeConstraint)
    MAKEBKI(0,    0,    0,    0,        0,     0,  0,   0,  0,       0,   0,    0,       0,   0,    1,      0,  0,       0,    SYM_GenericNonTypeConstraint)
    MAKEBKI(0,    0,    1,    0,        0,     0,  0,   0,  0,       0,   0,    1,       0,   0,    0,      1,  0,       0,    SYM_GenericBinding)
    MAKEBKI(1,    0,    1,    1,        0,     0,  0,   0,  0,       0,   0,    0,       0,   0,    0,      1,  0,       0,    SYM_GenericTypeBinding)
    MAKEBKI(1,    0,    1,    0,        0,     0,  0,   0,  0,       0,   0,    0,       0,   0,    0,      0,  1,       0,    SYM_TypeForwarder)
    MAKEBKI(0,    0,    0,    0,        0,     0,  0,   0,  0,       0,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_ExtensionCallLookupResult )
    MAKEBKI(0,    0,    1,    0,        0,     1,  1,   0,  0,       1,   0,    0,       0,   0,    0,      0,  0,       0,    SYM_LiftedOperatorMethod )
};

#undef MAKEBKI

int _cdecl
SortSymbolsByName
(
    const void *arg1,
    const void *arg2
)
{
    return
        CompareNoCase(
            (*(BCSYM_NamedRoot **)arg1)->GetName(),
            (*(BCSYM_NamedRoot **)arg2)->GetName());
}


int __cdecl
SortSymbolsByNameLocaleSensitive
(
    const void *arg1,
    const void *arg2
)
{
    return
        CompareString32NoCase(
            (*(BCSYM_NamedRoot **)arg1)->GetName(),
            (*(BCSYM_NamedRoot **)arg2)->GetName());
}

int _cdecl
SortSymbolsBySourceFile
(
    const void *arg1,
    const void *arg2
)
{
    SourceFile *File1 = (*(BCSYM_NamedRoot **)arg1)->GetSourceFile();
    SourceFile *File2 = (*(BCSYM_NamedRoot **)arg2)->GetSourceFile();

    if (File1 == File2)
    {
        return 0;
    }

    if (!File1)
    {
        return -1;
    }

    if (!File2)
    {
        return 1;
    }

    if (File1->IsSolutionExtension() != File2->IsSolutionExtension())
    {
        return File1->IsSolutionExtension() ? -1 : 1;
    }

    VSASSERT(File1->GetFileName() && File2->GetFileName(), "How can a source file have a NULL name ?");

    return wcscmp(File1->GetFileName(), File2->GetFileName());
}

int _cdecl
SortSymbolsBySourceFileCaseInsensitive
(
    const void *arg1,
    const void *arg2
)
{
    SourceFile *File1 = (*(BCSYM_NamedRoot **)arg1)->GetSourceFile();
    SourceFile *File2 = (*(BCSYM_NamedRoot **)arg2)->GetSourceFile();

    if (File1 == File2)
    {
        return 0;
    }

    if (!File2)
    {
        return 1;
    }

    if (!File1)
    {
        return -1;
    }

    VSASSERT(File1->GetFileName() && File2->GetFileName(), "How can a source file have a NULL name ?");

    return CompareNoCase(File1->GetFileName(), File2->GetFileName());
}

int _cdecl
SortSymbolsByLocationUnstableNamedRoot
(
    const void *arg1,
    const void *arg2
)
{
    BCSYM_NamedRoot *Named1 = (*(BCSYM_NamedRoot **)arg1);
    BCSYM_NamedRoot *Named2 = (*(BCSYM_NamedRoot **)arg2);

    if (!Named2->HasLocation())
    {
        return Named1->HasLocation() ? 1 : 0;
    }

    if (!Named1->HasLocation())
    {
        return -1;
    }

    // Compare the locations.
    return Location::Compare(Named1->GetLocation(), Named2->GetLocation());
}

int _cdecl
SortSymbolsByLocationUnstable
(
    const void *arg1,
    const void *arg2
)
{
    // 



    if (int FileComparisonResult = SortSymbolsBySourceFile(arg1, arg2))
    {
        return FileComparisonResult;
    }

    // Compare the locations.
    return SortSymbolsByLocationUnstableNamedRoot(arg1, arg2);
}

int _cdecl
SortSymbolsByLocationUnstableCaseInsensitive
(
    const void *arg1,
    const void *arg2
)
{
    if (int FileComparisonResult = SortSymbolsBySourceFileCaseInsensitive(arg1, arg2))
    {
        return FileComparisonResult;
    }

    // Compare the locations.
    return SortSymbolsByLocationUnstableNamedRoot(arg1, arg2);
}

int _cdecl
SortSymbolsByLocation
(
    const void *arg1,
    const void *arg2
)
{
    // First sort by location.
    int i = SortSymbolsByLocationUnstable(arg1, arg2);

    // Secondary sort is based on name to make this a stable sort
    if (i == 0)
    {
        i = SortSymbolsByName(arg1, arg2);
    }

    return i;
}

int _cdecl
SortSymbolsByLocationCaseInsensitive
(
    const void *arg1,
    const void *arg2
)
{
    // First sort by location.
    int i = SortSymbolsByLocationUnstableCaseInsensitive(arg1, arg2);

    // Secondary sort is based on name to make this a stable sort
    if (i == 0)
    {
        i = SortSymbolsByName(arg1, arg2);
    }

    return i;
}

int _cdecl
SortSymbolsByNameAndLocation
(
    const void *arg1,
    const void *arg2
)
{
    // First sort by name.
    int i = SortSymbolsByName(arg1, arg2);

    // Secondary sort is based on location.
    if (i == 0)
    {
        i = SortSymbolsByLocationUnstable(arg1, arg2);
    }

    return i;
}

bool BCSYM::IsAttribute()
{
    SymbolEntryFunction;
    return IsClass() && this && PClass()->IsAttribute();
}

bool BCSYM::IsTypeExtension()
{
    SymbolEntryFunction;
    return IsClass() && PClass()->ContainsExtensionMethods();
}


bool BCSYM::IsExtensionMethod
(
    bool IgnoreFlagAndCheckAttrManually // [optional] = false
)
{
    SymbolEntryFunction;
    if (IsProcedure(this))
    {
        Procedure* Proc = ViewAsProcedure(this);
        if (Proc->IsCallableAsExtensionMethod() ||
            (IgnoreFlagAndCheckAttrManually && Attribute::VerifyExtensionAttributeUsage(Proc, NULL, NULL)))
        {
            return true;
        }
    }
    return false;
}

bool BCSYM::IsAnonymousType()
{
    SymbolEntryFunction;
    return IsContainer() && PContainer()->IsAnonymousType();
}

bool BCSYM::IsAnonymousDelegate()
{
    SymbolEntryFunction;
    return IsContainer() && PContainer()->IsAnonymousDelegate();
}

//****************************************************************************
// BCSYM_NamedRoot
//****************************************************************************

// Get the next symbol we bound to.
BCSYM_NamedRoot *BCSYM_NamedRoot::GetNextBound()
{
    SymbolEntryFunction;
    VSASSERT( !IsPartialTypeAndHasMainType(),
                    "Partial type unexpected!!! Should never even have bound to a partial type!!!");

    BCSYM_NamedRoot *pnamedNext = GetNextOfSameName();

    // Check the hash containing this sym for the next sym.
    if (!pnamedNext && GetImmediateParent() && !IsHash() && GetImmediateParent()->IsHash())
    {
        BCSYM *pnamedHashNext = GetImmediateParent()->GetNextBound();

        if (pnamedHashNext)
        {
            pnamedNext = pnamedHashNext->PHash()->SimpleBind(pnamedNext->GetName());
        }

        // Find the next
    }

    return pnamedNext;
}

//============================================================================
// Returns the first BCSYM_NamedRoot with a name of pstrName (or NULL if
// not found).
//
// If binding in a project and public symbols of the requested name are found
// in more than one of its standard modules, then a BadNamedRoot is created
// with enough info to report the ambiguity.
//============================================================================

BCSYM_NamedRoot *BCSYM_NamedRoot::SimpleBind
(
    Symbols *psymbols,             // [in] Used to create BadNamedRoots if necessary.
                                   //      May be NULL if caller is certain an
                                   //      ambiguous name error is impossible.
    _In_z_ STRING *pstrName               // [in] name of symbol to bind
)
{
    SymbolEntryFunction;

    VSASSERT(pstrName != NULL, "can't bind to an empty name");

    BCSYM_NamedRoot *pnamedRet = NULL;

    // No symbol, no binding.
    if (!this) return NULL;

    // Do the correct symbol thing.
    if (IsHash())
    {
        BCSYM_NamedRoot *pnamedPrevFound = NULL;
        bool fFoundProjectLevel = false;

        BCSYM_NamedRoot *pnamedFound = PHash()->SimpleBind(pstrName);
        pnamedRet = pnamedFound;

        for( ; pnamedFound; pnamedFound = pnamedFound->GetNextOfSameName())
        {
            // Figure out if this is a module-level thing or a project-level thing.
            BCSYM_NamedRoot *pnamedTrue = pnamedFound->DigThroughAlias()->PNamedRoot();
            BCSYM_NamedRoot *pnamedParent = pnamedTrue->GetParent()->GetContainingClass();

            bool isThisInModule = pnamedParent && pnamedParent->PClass()->IsStdModule();

            if (pnamedPrevFound)
            {
                // If we've already seen something at project level and this is in a module, ignore this.
                if (isThisInModule && fFoundProjectLevel) continue;

                // If all we've seen is at module level and this is a project thing, ignore the other stuff.
                if (!fFoundProjectLevel && !isThisInModule)
                {
                    pnamedPrevFound = pnamedFound;
                    pnamedRet = pnamedFound;
                    fFoundProjectLevel = true;
                    continue;
                }
            }
            else
            {
                pnamedPrevFound = pnamedFound;
                fFoundProjectLevel = !isThisInModule;
            }
        } // for(;pnamedFound;pnamedFound=pnamedFound->GetNext())
    }
    else if (IsContainer())
    {
        BCSYM_Hash *psymHash;
        BCSYM_NamedRoot *pnamed;

        psymHash = PContainer()->GetHash();
        pnamed = psymHash->PNamedRoot();

        if (pnamed)
        {
            VSASSERT(pnamed->IsHash(), "Don't have a hash table, possible import problem (conflict with m_pNestedTypeList)");
            pnamedRet = pnamed->DigThroughAlias()->PHash()->SimpleBind(pstrName);
        }
    }

    return pnamedRet;
}

ACCESS BCSYM_NamedRoot::GetAccess()
{
    SymbolEntryFunction;
    if (BCSYM_Container *MainType =
            IsPartialTypeAndHasMainType())
    {
        return MainType->GetAccess();
    }
    else
    {
        return (ACCESS)m_access;
    }
}

ACCESS BCSYM_NamedRoot::GetAccessConsideringAssemblyContext
(
    BCSYM_Container *ContainerContext
)
{
    SymbolEntryFunction;
    ACCESS ContextualAccess = GetAccess();

    if (!ContainerContext)
    {
        return ContextualAccess;
    }

    CompilerProject *ProjectContext =
        ContainerContext->GetCompilerFile() ?
            ContainerContext->GetCompilerFile()->GetProject() :
            NULL;

    CompilerFile *FileContainingThisMember = this->GetCompilerFile();

    CompilerProject *ProjectContainingThisMember =
        FileContainingThisMember ?
            FileContainingThisMember->GetProject() :
            NULL;

    // if in the same project, then the the Contextual Context
    // remains the same.
    if (ProjectContainingThisMember == ProjectContext)
    {
        return ContextualAccess;
    }

    // if different projects, Friend and Protected Friend should
    // show up as Private and Protected respectively to anybody
    // outside the project.
    //
    // Microsoft: Do not do this because t gives misleading error messages.
    //if (ContextualAccess == ACCESS_Friend)
    //{
    //    ContextualAccess = ACCESS_Private;
    //}
    //else if (ContextualAccess == ACCESS_ProtectedFriend)
    //{
    //    ContextualAccess = ACCESS_Protected;
    //}

    return ContextualAccess;
}

bool BCSYM_NamedRoot::IsPublicFromAssembly(bool ShouldConsiderFriendness, BCSYM_NamedRoot **ppNonPublic)
{
    SymbolEntryFunction;
    BCSYM_NamedRoot *pNamedRoot = this;
    BCSYM_NamedRoot *pNonPublic = NULL;

    bool HasFriends = ShouldConsiderFriendness && pNamedRoot->GetContainingProject()->HasFriends();

    while (pNamedRoot && !pNonPublic)
    {
        ACCESS SymbolAccess = pNamedRoot->GetAccess();

        switch (SymbolAccess)
        {
            case ACCESS_Public:
                // Do nothing
                break;

            case ACCESS_Friend:
            case ACCESS_ProtectedFriend:
                if (HasFriends)
                {
                    // Do nothing if we have friends
                    break;
                }

            __fallthrough;
            default:
                pNonPublic = pNamedRoot;
        }

        pNamedRoot = pNamedRoot->GetParent();
    }

    if (ppNonPublic)
    {
        *ppNonPublic = pNonPublic;
    }

    return pNonPublic == NULL;
}

    // Be careful when using this function. With the introduction of FriendAssemblies, we will consider
    // the presence of any "InternalsVisibleTo" attribute as a reason to expose "Friend members".

bool BCSYM_NamedRoot::IsOrCouldBePublic()
{
    SymbolEntryFunction;
    VSASSERT( !IsPartialTypeAndHasMainType(),
                    "Partial type unexpected!!!");

    switch (GetAccess())
    {
        case ACCESS_Public:
        case ACCESS_Protected:
        case ACCESS_ProtectedFriend:
            return true;

        case ACCESS_Friend:
            return GetContainingProject()->HasFriends();
    }

    return false;
}

WellKnownAttrVals* BCSYM_NamedRoot::GetPWellKnownAttrVals()
{
    SymbolEntryFunction;
    // The reason we need this is that the well known attributes
    // should be shared by all the partial components of a type
    // irrespective of whether a partial type component has
    // attributes or not
    //
    if (this->IsContainer())
    {
        return this->PContainer()->GetPWellKnownAttrVals();
    }

    // Note: WellKnownAttrs::GetPWellKnownAttrVals is NULL safe
    //
    return GetPAttrVals()->GetPWellKnownAttrVals();
}

// Returns pointer to WellKnownAttrVals structure for the container. Can be NULL.
WellKnownAttrVals* BCSYM_Container::GetPWellKnownAttrVals()
{
    SymbolEntryFunction;
    // The reason we need this is that the well known attributes
    // should be shared by all the partial components of a type
    // irrespective of whether a partial type component has
    // attributes or not
    //
    BCSYM_Container * pMainType = this->IsPartialTypeAndHasMainType();
    
    if ( pMainType != NULL )
    {
        return pMainType->GetPWellKnownAttrVals();
    }

    // Note: WellKnownAttrs::GetPWellKnownAttrVals is NULL safe
    //
    return GetPAttrVals()->GetPWellKnownAttrVals();
}




bool BCSYM_NamedRoot::IsHiddenInEditor(BCSYM_NamedRoot *pParent, CompilerProject *pProject)
{
    SymbolEntryFunction;
    if (IsHidden() || IsBrowsableNever())
    {
        return true;
    }

#if IDE 
    if (pProject)
    {
        // See if the context filter indicates that this symbol should be hidden
        IVsContextualIntellisenseFilter *pFilter = pProject->GetContextualIntellisenseFilter();
        if (pFilter)
        {
            BCSYM_NamedRoot *pNamed = DigThroughAlias()->PNamedRoot();
            BOOL fVisible = TRUE;
            if (IsType())
            {
                // 

                if (FAILED(pFilter->IsTypeVisible(pNamed->GetQualifiedEmittedName(), &fVisible)))
                {
                    fVisible = TRUE;
                }
            }
            else
            {
                if (FAILED(pFilter->IsMemberVisible(pNamed->GetDocCommentSignature(GetCompiler(), NULL, pParent), &fVisible)))
                {
                    fVisible = TRUE;
                }
            }

            if (fVisible == FALSE)
            {
                return true;
            }
        }
    }
#endif IDE

    return false;
}

bool BCSYM_NamedRoot::IsHidden()
{
    SymbolEntryFunction;
    short Flags;

    return
        IsHiddenRaw() ||
        (GetPAttrVals()->GetPWellKnownAttrVals()->GetTypeLibTypeData(&Flags) && (Flags & TYPEFLAG_FHIDDEN)) ||
        (GetPAttrVals()->GetPWellKnownAttrVals()->GetTypeLibFuncData(&Flags) && (Flags & (FUNCFLAG_FHIDDEN | FUNCFLAG_FRESTRICTED)));
}

bool BCSYM_NamedRoot::IsAdvanced()
{
    SymbolEntryFunction;
    VSASSERT( !IsPartialTypeAndHasMainType(),
                    "Partial type unexpected!!!");

    long Browsable;

    if (GetPAttrVals()->GetPWellKnownAttrVals()->GetEditorBrowsableData(&Browsable))
    {
        return (Browsable == 2);
    }
    else
    {
        return false;
    }
}

bool BCSYM_NamedRoot::IsBrowsableNever()
{
    SymbolEntryFunction;
    long Browsable;

    return GetPAttrVals()->GetPWellKnownAttrVals()->GetEditorBrowsableData(&Browsable) && (Browsable == 1);
}

bool BCSYM_NamedRoot::IsBrowsableAlways()
{
    SymbolEntryFunction;
    long lBrowsable;
    return GetPAttrVals()->GetPWellKnownAttrVals()->GetEditorBrowsableData(&lBrowsable) && (lBrowsable == 0);
}

bool BCSYM_NamedRoot::IsObsolete()
{
    SymbolEntryFunction;
    WCHAR *wszDescription;
    bool IsError;

    return GetPAttrVals()->GetPWellKnownAttrVals()->GetObsoleteData(&wszDescription, &IsError);
}

BCSYM_Container* BCSYM_NamedRoot::GetPhysicalContainer()
{
    SymbolEntryFunction;
    BCSYM_NamedRoot *pCurrent = this;

    if (pCurrent->IsHash())
    {
        return pCurrent->PHash()->GetPhysicalContainer();
    }

    while (pCurrent = pCurrent->GetImmediateParent())
    {
        if (pCurrent->IsHash())
        {
            return pCurrent->PHash()->GetPhysicalContainer();
        }

        if (pCurrent->IsContainer())
        {
            return pCurrent->PContainer();
        }
    }

    return NULL;
}

BCSYM_Container* BCSYM_NamedRoot::IsPartialTypeAndHasMainType() const
{
    SymbolEntryFunction;
    if (this->IsContainer())
    {
        return this->PContainer()->IsPartialTypeAndHasMainType();
    }

    return NULL;
}

ErrorTable* BCSYM_NamedRoot::GetErrorTableForContext()
{
    SymbolEntryFunction;
    SourceFile *FileContext = this->GetSourceFile();

    if (!FileContext)
    {
        return NULL;
    }

    return FileContext->GetCurrentErrorTable();
}

bool BCSYM_NamedRoot::HasGenericParent()
{
    SymbolEntryFunction;
    if (this->IsNamespace())
    {
        return false;
    }

    if (this->IsHash())
    {
        return this->PHash()->HasGenericParent();
    }

    for(BCSYM_NamedRoot *Parent = this->GetImmediateParent();
        Parent;
        Parent = Parent->GetImmediateParent())
    {
        if (Parent->IsNamespace())
        {
            return false;
        }

        if (Parent->IsHash())
        {
            return Parent->PHash()->HasGenericParent();
        }

        if (Parent->IsGeneric())
        {
            return true;
        }
    }

    return false;
}

//****************************************************************************
// BCSYM_Hash
//****************************************************************************

//============================================================================
// Returns the first BCSYM_NamedRoot with a name of pstrName (or NULL if
// not found).
//============================================================================

BCSYM_NamedRoot *BCSYM_Hash::SimpleBind
(
    _In_z_ const STRING *pstrName,              // [in] name of symbol to bind
    SimpleBindFlag flags
)
{
    SymbolEntryFunction;
    VSASSERT(pstrName != NULL, "can't bind to an empty name");

    // No hash, no binding.
    if (!this)
    {
        return NULL;
    }

    bool IsCaseSensitive = flags & SIMPLEBIND_CaseSensitive;
    bool fIgnoreOtherPartialTypes = flags & SIMPLEBIND_IgnoreOtherPartialTypes;

    // start binding by starting in the main container's hash
    if (!fIgnoreOtherPartialTypes &&
        GetImmediateParent() &&
        GetImmediateParent()->IsContainer() &&
        // Hash belongs to a partial type or to the main type having
        // partial types
        //
        (GetImmediateParent()->PContainer()->IsPartialType() ||
            GetImmediateParent()->PContainer()->GetNextPartialType()))
    {

        // Find the first hash in the chain of hashes
        // Loop to the first hash i.e. the hash of the main type
        // so that we can start the name search from the beginning.
        //
        BCSYM_Hash *FirstHash;
        for(FirstHash = this;
            FirstHash->GetPrevHash();
            FirstHash = FirstHash->GetPrevHash());

        BCSYM_Hash *LookupHash;
        for(LookupHash = FirstHash;
            LookupHash;
            LookupHash = LookupHash->GetNextHash())
        {
            if (BCSYM_NamedRoot *BoundSymbol =
                    LookupHash->SimpleBind(pstrName, SIMPLEBIND_IgnoreOtherPartialTypes))
            {
                return BoundSymbol;
            }
        }

        return NULL;
    }

    if (!CBuckets())
    {
#if TRACK_BCSYMHASH
        g_LookupMisses++;
#endif

        return NULL;
    }

    // get the correct bucket
    BCSYM_NamedRoot *pBucket = Rgpnamed()[StringPool::HashValOfString(pstrName) % CBuckets()];

    {
        // Search the bucket for a matching symbol
        while(pBucket)
        {
#if TRACK_BCSYMHASH
            g_LookupIterations++;
#endif

            if (StringPool::IsEqual(pstrName, pBucket->GetName(),IsCaseSensitive) &&
                //
                // Ignore partial types marked as IgnoreSymbol, nobody should ever bind to these
                //
                (!pBucket->IsContainer() ||
                    !pBucket->PContainer()->IsPartialType() ||
                    pBucket->GetBindingSpace() != BINDSPACE_IgnoreSymbol))
            {
                break;
            }

            pBucket = pBucket->GetNextInHash();
        }
    }

#if TRACK_BCSYMHASH
    g_HashLookups++;
    g_LookupIterations++;

    if (pBucket)
    {
         g_LookupHits++;
    }
    else
    {
         g_LookupMisses++;
    }
#endif

    
#if HOSTED
    if (pBucket == NULL && GetExternalSymbolSource())
    {
        return GetExternalSymbolSource()->GetSymbolForName(pstrName);
    }
#endif

    return pBucket;
}

bool BCSYM_Hash::HasGenericParent()
{
    SymbolEntryFunction;
    if (m_HasGenericParentBeenDetermined)
    {
        return m_HasGenericParent;
    }

    BCSYM_NamedRoot *Parent = this->GetImmediateParent();

    if (Parent)
    {
        if (Parent->IsGeneric())
        {
            m_HasGenericParent = true;
        }
        else
        {
            m_HasGenericParent = Parent->HasGenericParent();
        }
    }
    else
    {
        m_HasGenericParent = false;
    }

    m_HasGenericParentBeenDetermined = true;
    return m_HasGenericParent;
}

BCSYM_Container * BCSYM_Container::DigUpToModuleOrNamespace()
{
    SymbolEntryFunction;
    BCSYM_Container * pRet = this;

    if (! pRet || pRet->IsNamespace())
    {
        return pRet;
    }

    BCSYM_Container * pNext = pRet->GetContainer();

    while (pNext && ! pNext->IsNamespace())
    {
        pRet = pNext;
        pNext = pNext->GetContainer();
    }


    if (pRet->IsClass() && pRet->PClass()->IsStdModule())
    {
        return pRet;
    }
    else
    {
        //pNext is the parent of pRet...
        //because of the loop above it's either NULL (which shouldn't ever happen)
        //or a namespace.
        ThrowIfNull(pNext);
        return pNext;
    }
}

bool BCSYM_Hash::MergeHashTable(BCSYM_Hash* pMergedHash, Symbols* pSymbolCreator)
{
    SymbolEntryFunction;
    VSASSERT( NULL != pMergedHash, "[in-parameter] pMergedHash cannot be NULL." );
    VSASSERT( NULL != pSymbolCreator, "[in-parameter] pSymbolCreator cannot be NULL." );

    for (unsigned int iBucket = 0; iBucket < CBuckets(); ++iBucket)
    {
        BCSYM_NamedRoot* pBucket = this->Rgpnamed()[ iBucket ];
        while (NULL != pBucket)
        {
            BCSYM_Alias* pAlias = pSymbolCreator->GetAliasOfSymbol(
                pBucket,
                ACCESS_Public,
                NULL,
                false);

            pSymbolCreator->AddSymbolToHash(
                pMergedHash,
                pAlias,
                false,
                false,
                false);

            pBucket = pBucket->GetNextInHash();
        }
    }

#if HOSTED
    if (GetExternalSymbolSource() != NULL)
    {
        ASSERT(pMergedHash->GetExternalSymbolSource() == NULL, "[BCSYM_Hash::MergeHashTable] There should be only one External Symbol Source");
        pMergedHash->SetExternalSymbolSource(GetExternalSymbolSource());
    }
#endif

    return true;
}

// Is this a BASIC container?
bool BCSYM_Container::IsBasic()
{
    SymbolEntryFunction;
    // Assume that anything that doesn't have a file
    // is a BASIC thing.
    //

#if HOSTED
    //VBHosted: Items that have an external symbol are not basic containers.
    return (GetCompilerFile() == NULL || GetCompilerFile()->IsSourceFile()) && !GetExternalSymbol();
#else
    return GetCompilerFile() == NULL || GetCompilerFile()->IsSourceFile();
#endif
}

// Is this the same container as the one specified by the paramenter?
// For namespaces it returns true if both namespaces belong to the same namespace ring.
// For other containers it returns true if it is the same symbol.
bool BCSYM_Container::IsSameContainer(BCSYM_NamedRoot *pContainer)
{
    SymbolEntryFunction;
    if (this == pContainer)
    {
        return true;
    }

    if (!this || !pContainer)
    {
        return false;
    }

    if (IsContainer())
    {
        if (IsNamespace())
        {
            return pContainer->IsNamespace() &&
                        PNamespace()->IsSameNamespace(pContainer->PNamespace());
        }
        else
        {
            // Equivalence of partial types

            if (!pContainer->IsContainer())
            {
                return false;
            }

            BCSYM_Container *MainType;
            if (MainType = pContainer->PContainer()->IsPartialTypeAndHasMainType())
            {
                if (MainType == this ||
                    MainType == this->IsPartialTypeAndHasMainType())
                {
                    return true;
                }
            }
            else if (MainType = this->IsPartialTypeAndHasMainType())
            {
                if (MainType == pContainer)
                {
                    return true;
                }
            }
        }
    }

    return false;
}


// Get the source file that holds this symbol.
SourceFile *BCSYM_Container::GetSourceFile()
{
    SymbolEntryFunction;
    if (GetCompilerFile() && GetCompilerFile()->IsSourceFile())
    {
        return GetCompilerFile()->PSourceFile();
    }

    return NULL;
}

// Get the metadatafile that holds this symbol.
MetaDataFile *BCSYM_Container::GetMetaDataFile()
{
    SymbolEntryFunction;
    return GetCompilerFile()->PMetaDataFile();
}

//****************************************************************************
// BCSYM_Container
//****************************************************************************


// Make sure that the members of this symbol are fully
// initialized.
//
void BCSYM_Container::EnsureChildrenLoaded()
{
    SymbolEntryFunction;

#if DEBUG
    // EnsureChildrenLoaded can be called only after the containing
    // CompilerProject has had its bases and implements loaded.
    //
    // (The exceptions to this are: (1) namespaces can be loaded,
    // and, (2) you can load symbols that live in the metadata
    // project that's currently being compiled.)
    //
    // See VS RAID 244721 for a sample project where this had
    // been happening.
    if (this &&
        !IsNamespace() &&
        GetContainingProject() != NULL &&
        GetContainingProject()->IsMetaData() &&
        GetContainingProject()->GetCompiler()->GetProjectBeingCompiled() != NULL &&
        GetContainingProject()->GetCompiler()->GetProjectBeingCompiled() != GetContainingProject())
    {
        VSASSERT(
            GetContainingProject()->GetBasesAndImplementsLoaded(),
            "Can't load children of symbol in a project that hasn't had bases and implements loaded!");
    }
#endif

    if (this)
    {
        if (IsBasic())
        {
            if (!IsCCContainer())
            {
                // The reason we need to do this is because when binding a type, and there
                // is more than one main type present, we don't want to bind to the one
                // which we are going to mark as duplicate and hence bad in the future. Instead
                // if we detect duplicates before the binding, we are guaranteed to bind to the
                // correct one.
                //
                Bindable::EnsurePartialTypesAreResolved(this, NULL);
            }
        }
        else if (GetChildrenNotLoaded())
        {
            if (IsNamespace())
            {
                Bindable::BindMetaDataContainer(this, NULL);
                SetChildrenNotLoaded(false);
            }
#if HOSTED
            else if (GetExternalSymbol())
            {
                VSASSERT(AreBaseAndImplementsLoaded(), "must load base and implements first!");
                VBHostIntegration::ImportTypeChildren(this);
            }
#endif
            else
            {
                VSASSERT(AreBaseAndImplementsLoaded(), "must load base and implements first!");
                MetaImport::ImportTypeChildren(this);
            }
        }
    }
}

// Get the hash table for this container.
BCSYM_Hash *BCSYM_Container::GetHash()
{
    SymbolEntryFunction;
    EnsureChildrenLoaded();

    return m_phash;
}

// Get the hash table of UnBindable Members for this container.
BCSYM_Hash *BCSYM_Container::GetUnBindableChildrenHash()
{
    SymbolEntryFunction;
    EnsureChildrenLoaded();

    return m_UnBindableChildrenHash;
}

Bindable *BCSYM_Container::GetBindableInstance()
{
    SymbolEntryFunction;
    //VSASSERT( !IsBindingDone(),
    //                "Why is somebody asking for a BindableInstance after Binding is done ?");

    // Bindable Instance should only be needed when a source file is
    // going from Declared to Bindable or when a metadata container's
    // hash is being loaded on demand
    //
#if HOSTED
    VSASSERT( !this->GetSourceFile() ||
              this->GetSourceFile()->GetCompState() == (CS_Bound - 1) ||
              this->GetExternalSymbol(),
                    "Unexpected request for a bindable instance!!!");
#else
    VSASSERT( !this->GetSourceFile() ||
              this->GetSourceFile()->GetCompState() == (CS_Bound - 1),
                    "Unexpected request for a bindable instance!!!");
#endif

    if (!m_BindableInstance)
    {
        CompilerFile *CompilerFile = this->GetCompilerFile();
        SourceFile *SourceFileOfContainer =
            (CompilerFile && CompilerFile->IsSourceFile()) ?
                CompilerFile->PSourceFile() :
                NULL;

        NorlsAllocator *Allocator = CompilerFile->SymbolStorage();

        ErrorTable *ErrorLog = SourceFileOfContainer ?
                                    SourceFileOfContainer->GetCurrentErrorTable() :
                                    NULL;

        //8-29-2005: Microsoft
        //modified the assert to support binding synthetic types
        //during late phases of compilation
#if HOSTED
        VSASSERT( !SourceFileOfContainer ||
                  GetExternalSymbol() ||
                  (SourceFileOfContainer->GetCompState() == (CS_Bound - 1)),
                        "Why is a bindable instance being constructed in any other compilation state (sourcefile) ?");
#else
        VSASSERT( !SourceFileOfContainer ||
                  (SourceFileOfContainer->GetCompState() == (CS_Bound - 1)),
                        "Why is a bindable instance being constructed in any other compilation state (sourcefile) ?");
#endif

        //8-29-2005: Microsoft
        //modified the assert to support binding synthetic types
        //during late phases of compilation
#if HOSTED
        VSASSERT( !SourceFileOfContainer ||
                  GetExternalSymbol() ||
                  (SourceFileOfContainer->GetProject() &&
                        SourceFileOfContainer->GetProject()->GetCompState() == (CS_Bound - 1)),
                        "Why is a bindable instance being constructed in any other compilation state (project) ?");
#else
        VSASSERT( !SourceFileOfContainer ||
                  (SourceFileOfContainer->GetProject() &&
                        SourceFileOfContainer->GetProject()->GetCompState() == (CS_Bound - 1)),
                        "Why is a bindable instance being constructed in any other compilation state (project) ?");
#endif

        CompilerHost *CompilerHost = NULL;

        if (CompilerFile)
        {
            CompilerHost = CompilerFile->GetCompilerHost();
        }

        ThrowIfNull(CompilerHost);

        VSASSERT(CompilerHost, "Bad CompilerHost in BCSYM_Container...");

        // 




        m_BindableInstance = new (zeromemory) Bindable( this,
                                                           SourceFileOfContainer,
                                                           ErrorLog,
                                                           Allocator,
                                                           CompilerHost);

#if IDE 
        if (SourceFileOfContainer &&
            !SourceFileOfContainer->HasBindingStarted())
        {
            SourceFileOfContainer->SetBindingStarted();
        }
#endif
    }

    return m_BindableInstance;
}

void BCSYM_Container::DeleteBindableInstance()
{
    SymbolEntryFunction;
    if (m_BindableInstance)
    {
        delete m_BindableInstance;
    }
    m_BindableInstance = NULL;
}

BCSYM_Container* BCSYM_Container::GetMainType() const
{
    SymbolEntryFunction;
    return
        this->IsClass() ?
            this->PClass()->GetMainType() :
            NULL;
}

BCSYM_Container* BCSYM_Container::GetNextPartialType()
{
    SymbolEntryFunction;
    return
        this->IsClass() ?
            this->PClass()->GetNextPartialType() :
            NULL;
}

void BCSYM_Container::DetermineIfCLSComplianceIsClaimed()
{
    SymbolEntryFunction;
    VSASSERT( !GetCompilerFile() ||
              GetCompilerFile()->GetProject()->GetCompState() >= CS_Bound,
                    "CLS Compliance of a container cannot be determined until its containing project is completely bound!!!");

    if (HasCLSComplianceBeenDetermined())
    {
        return;
    }

    // If Namespace, then get this setting from the project because
    // Namespace cannot have attributes although our implementation
    // causes the root namespace to have attributes.
    //
    if (IsNamespace())
    {
        VSASSERT( GetCompilerFile() &&
                  GetCompilerFile()->GetProject(),
                    "Container with no containing compiler file unexpected !!!");

        CompilerProject *pCurrentProject = GetCompilerFile()->GetProject();

        SetIsCLSCompliant(pCurrentProject->ProjectClaimsCLSCompliance());
        SetHasCLSComplianceBeenDetermined(true);

        return;
    }

    // If System.CLSCompliantAttribute is explicitly specified, use the
    // explicitly specified value
    //
    bool fIsCLSCompliant, fIsInherited;
    if (GetPWellKnownAttrVals()->GetCLSCompliantData(&fIsCLSCompliant, &fIsInherited))
    {
        if (!fIsInherited)
        {
            SetIsCLSCompliant(fIsCLSCompliant);
            SetHasCLSComplianceBeenDetermined(true);
            return;
        }
        else
        {
            // If the CLS Compliance attribute is inherited, then
            //  - we only infer non-CLS compliantness from an inherited CLS Compliant
            //      attribute. The reason is we don't treat a container as CLS compliant
            //      unless it inherits from CLS compliant entities as well as contained
            //      within CLS compliant entities.
            //
            //  - we only infer from an inherited CLS Compliant attribute for metadata
            //      containers. Source containers infer this from their containing type
            //      and an appropriate warning/error is given if their inherit from a
            //      non-CLS Compliant type
            //
            if (!fIsCLSCompliant &&
                Bindable::DefinedInMetaData(this))
            {
                SetIsCLSCompliant(fIsCLSCompliant);
                SetHasCLSComplianceBeenDetermined(true);
                return;
            }
        }
    }

    // If not explicitly specified, then infer from the containing type
    // possibly eventually inferring from the assembly
    //

    BCSYM_Container *pParentContainer = this->GetContainer();

    VSASSERT( pParentContainer,
                "How can a non-namespace not have an enclosing container ?");

    SetIsCLSCompliant(pParentContainer->IsCLSCompliant());
    SetHasCLSComplianceBeenDetermined(true);

    return;
}


#if IDE 
// Dev10 #678696
bool BCSYM_Container::TreatAsEmbeddedLocalType() const
{
    SymbolEntryFunction;
    return (*(VBTLSThreadData::GetTreatAsEmbeddedLocalType()) == this);
}
#endif IDE 



/*
** BCSYM methods - generic type methods
*/

// Is this a type?
// The BCSYM generic type methods can only be called if this is true.
bool BCSYM::IsType()
{
    SymbolEntryFunction;
    if (IsAlias())
        return PAlias()->GetSymbol() == NULL || PAlias()->GetSymbol()->IsType();

#pragma prefast(suppress: 26010 26011, "For valid symbols, GetKind() should be bound by the number of symbol types")
    return s_rgBilkindInfo[GetKind()].m_isType;
}

// Is this symbol an intrinsic type that maps on to a COM+ type?
bool BCSYM::IsIntrinsicType()
{
    SymbolEntryFunction;
    if(IsClass())
    {
        return PClass()->IsIntrinsicType();
    }

    return false;
}

//Obtains the object to use when binding against this symbol's members.
//If the symbol has members that can be bound against it will return
//a non null value in either ppHash or ppGenericParam (exclusively), otherwise both will be null.
//The arguments returned are suitable for use as the second and third arguments to Semantics::InterpretName.
//This method may be called on a null pointer.
void BCSYM::GetBindingSource
(
    CompilerHost * pCompilerHost,
    BCSYM_Hash ** ppHash, //[OUT] pointer to the hash to be returned
    BCSYM_GenericParam ** ppGenericParam //[OUT] pointer to the generic param to be returned
)
{
    SymbolEntryFunction;
    if (ppHash)
    {
        *ppHash = NULL;
    }

    if (ppGenericParam)
    {
        *ppGenericParam = NULL;
    }

    BCSYM * pSym = ChaseToBindableType(pCompilerHost);

    if (pSym->IsContainer() && ppHash)
    {
        *ppHash = pSym->PContainer()->GetHash();
    }
    else if (pSym->IsGenericParam() && ppGenericParam)
    {
        *ppGenericParam = pSym->PGenericParam();
    }
}

//Indicates wether or not the symbol represents an entity that needs to be "unwrapped"
//before it can be bound against.
//This method can be called on a null pointer.
bool BCSYM::IsIndirectlyBindable()
{
    SymbolEntryFunction;
    return
        this && (IsNamedType() || IsPointerType() || IsArrayType());
}

//Chase through a symbol down to something that can be bound against.
//Unlike ChaseToType and ChaseToNamedType, this does not return the element type for array instances, but instead
//returns System.Array
BCSYM * BCSYM::ChaseToBindableType(CompilerHost * pCompilerHost)
{
    SymbolEntryFunction;
    BCSYM * pSymbol = this;

    while (pSymbol->IsIndirectlyBindable())
    {
        if (pSymbol->IsPointerType())
        {
            pSymbol = pSymbol->PPointerType()->GetRawRoot();
        }
        else if (pSymbol->IsNamedType())
        {
            pSymbol = pSymbol->DigThroughNamedType();
        }
        else if (pSymbol->IsArrayType())
        {
            pSymbol = pCompilerHost->GetFXSymbolProvider()->GetType(FX::ArrayType);
        }
        else
        {
            VSFAIL("Control should never reach this point. Did you change IsIndirectlyBindable without changing ChaseToBindableType? This is probably the cause of the error. We will return null to avoid looping forever.");
            pSymbol = NULL;
        }
    }

    return pSymbol;
}


// Get the vtype classification of this type.
Vtypes BCSYM::GetVtype()
{
    SymbolEntryFunction;
    VSASSERT(IsType(), "not a type symbol");

    if (IsClass())
        return PClass()->GetVtype();
    if (IsSimpleType())
        return PSimpleType()->GetVtype();
    if (IsInterface())
        return t_ref;
    if (IsNamedRoot() && PNamedRoot()->IsBad())
        return t_bad;
    if (IsVoidType())
        return t_void;
    if (IsAlias())
        return PAlias()->GetSymbol()->GetVtype();
    if (IsGenericParam())
        return t_generic;
    if (IsGenericTypeBinding())
        return PGenericTypeBinding()->GetGenericType()->GetVtype();

    VSFAIL("unknown type symbol");
    return t_bad;
}

// Is this a structure?
bool BCSYM::IsStruct()
{
    SymbolEntryFunction;
    return IsClass() && PClass()->IsStruct();
}

// Is this a delegate?
bool BCSYM::IsDelegate() const
{
    SymbolEntryFunction;
    return IsClass() && PClass()->IsDelegate();
}

// Is this an enum?
bool BCSYM::IsEnum()
{
    SymbolEntryFunction;
    return IsClass() && PClass()->IsEnum();
}

bool BCSYM::IsLocal()
{
    SymbolEntryFunction;
    return IsNamedRoot() && PNamedRoot()->IsLocal();
}

// Is this a generic type or method?
bool BCSYM::IsGeneric()
{
    SymbolEntryFunction;
    if (IsClass())
    {
        return PClass()->IsGeneric();
    }
    else if (IsInterface())
    {
        return PInterface()->IsGeneric();
    }
    else if (IsProc())
    {
        return PProc()->IsGeneric();
    }

    return false;
}

BCSYM_GenericParam *
BCSYM::GetFirstGenericParam()
{
    SymbolEntryFunction;
    if (IsClass())
    {
        return PClass()->GetFirstGenericParam();
    }
    else if (IsInterface())
    {
        return PInterface()->GetFirstGenericParam();
    }
    else if (IsProc())
    {
        return PProc()->GetFirstGenericParam();
    }

    return NULL;
}

BCSYM_Hash *
BCSYM::GetGenericParamsHash()
{
    SymbolEntryFunction;
    if (IsClass())
    {
        return PClass()->GetGenericParamsHash();
    }
    else if (IsInterface())
    {
        return PInterface()->GetGenericParamsHash();
    }
    else if (IsProc())
    {
        return PProc()->GetGenericParamsHash();
    }

    return NULL;
}

unsigned
BCSYM::GetGenericParamCount()
{
    SymbolEntryFunction;
    unsigned ParamCount = 0;

    for (BCSYM_GenericParam *Parameters = GetFirstGenericParam(); Parameters; Parameters = Parameters->GetNextParam())
    {
        ParamCount++;
    }

    return ParamCount;
}

bool
BCSYM::AreAttributesEmitted()
{
    SymbolEntryFunction;
    if (IsNamedRoot())
    {
        return PNamedRoot()->AreAttributesEmitted();
    }

    if (IsParam())
    {
        return PParam()->AreAttributesEmitted();
    }

    return false;
}

unsigned long
PartialGenericBinding::GetFreeArgumentCount()
{
    ThrowIfNull(m_pFixedTypeArgumentBitVector);
    return m_pFixedTypeArgumentBitVector->BitCount() - m_pFixedTypeArgumentBitVector->SetCount();
}

bool PartialGenericBinding::IsFixed(GenericParameter * pParam)
{
    ThrowIfNull(pParam);
    return m_pFixedTypeArgumentBitVector->BitValue(pParam->GetPosition());
}

GenericBindingInfo::GenericBindingInfo
(
    PartialGenericBinding * pPartialGenericBinding
)  :
    m_pPartialGenericBinding(pPartialGenericBinding),
    m_isPartialBinding(true),
    m_pGenericBinding(NULL),
    m_pTypeArgumentLocations(NULL)
{
}

GenericBindingInfo::GenericBindingInfo
(
    GenericBinding * pGenericBinding
) :
    m_pGenericBinding(pGenericBinding),
    m_isPartialBinding(false),
    m_pTypeArgumentLocations(NULL),
    m_pPartialGenericBinding(NULL)
{
}

GenericBindingInfo::GenericBindingInfo() :
    m_pGenericBinding(NULL),
    m_pTypeArgumentLocations(NULL),
    m_isPartialBinding(false),
    m_pPartialGenericBinding(NULL)
{
}

bool GenericBindingInfo::IsPartialBinding()
{
    return m_isPartialBinding;
}


GenericBindingInfo & GenericBindingInfo::operator =
(
    PartialGenericBinding * pPartialGenericBinding
)
{
    m_pPartialGenericBinding = pPartialGenericBinding;
    m_isPartialBinding = true;
    m_pGenericBinding = NULL;
    return *this;
}

GenericBindingInfo & GenericBindingInfo::operator =
(
    GenericBinding * pGenericBinding
)
{
    m_pGenericBinding = pGenericBinding;
    m_isPartialBinding = false;
    m_pTypeArgumentLocations = NULL;
    m_pPartialGenericBinding = NULL;
    return *this;
}

PartialGenericBinding *
GenericBindingInfo::PPartialGenericBinding
(
    bool throwOnFailure
)
{
    Assume(!throwOnFailure || m_isPartialBinding || (!m_isPartialBinding && !m_pGenericBinding), L"The requested cast is invalid!");
    return m_isPartialBinding ? m_pPartialGenericBinding : NULL;
}

BCSYM_GenericBinding *
GenericBindingInfo::PGenericBinding
(
    bool throwOnFailure
)
{
    Assume(!throwOnFailure || !m_isPartialBinding || (m_isPartialBinding && !m_pPartialGenericBinding) , L"The requested cast is invalid!");
    return m_isPartialBinding ? NULL : m_pGenericBinding;
}

BCSYM_GenericTypeBinding *
GenericBindingInfo::PGenericTypeBinding
(
    bool throwOnFailure
)
{
    GenericBinding * pGenericBinding = PGenericBinding(throwOnFailure);
    Assume(!throwOnFailure || !pGenericBinding || pGenericBinding->IsGenericTypeBinding(), L"The requested cast is invalid");
    return pGenericBinding && pGenericBinding->IsGenericTypeBinding() ? pGenericBinding->PGenericTypeBinding() : NULL;
}


bool GenericBindingInfo::IsNull() const
{
    return !(m_isPartialBinding ? (bool)m_pPartialGenericBinding : (bool)m_pGenericBinding);
}


bool GenericBindingInfo::IsGenericTypeBinding() const
{
    return
        ! m_isPartialBinding &&
        m_pGenericBinding &&
        m_pGenericBinding->IsGenericTypeBinding();
}

unsigned long GenericBindingInfo::FreeTypeArgumentCount() const
{
    if (m_isPartialBinding && m_pPartialGenericBinding)
    {
        return m_pPartialGenericBinding->GetFreeArgumentCount();
    }
    else
    {
        return 0;
    }
}

void
GenericBindingInfo::ApplyExplicitArgumentsToPartialBinding
(
    Type ** ppTypeArguments,
    Location * pTypeArgumentLocations,
    unsigned long TypeArgumentCount,
    Semantics * pSemantics,
    Declaration * pGeneric
)
{
    Assume(m_isPartialBinding && m_pPartialGenericBinding, L"ApplyExplicitArgumentsToPartialBinding cannot be called on non-partial bindings");
    Assume(TypeArgumentCount <= m_pPartialGenericBinding->GetFreeArgumentCount(), L"Too many type arguments supplied to ApplyExplicitArgumentsToPartialBinding");

    if (TypeArgumentCount)
    {
        ThrowIfNull(ppTypeArguments);
        ThrowIfNull(pTypeArgumentLocations);
        ThrowIfNull(pSemantics);
        ThrowIfNull(pSemantics->GetSymbols());
        ThrowIfNull(pGeneric);

        //Step1 - Clone Location and Argument Array
        //Step2 - Merge Arguments
        //Step3 - Convert the binding

        Symbols * pSymbols = pSemantics->GetSymbols();

        Type ** ppClonedArguments = CloneTypeArguments(pSymbols->GetNorlsAllocator());
        Location * pClonedLocations = CloneLocations(pSymbols->GetNorlsAllocator());

        MergeArguments
        (
            ppTypeArguments,
            pTypeArgumentLocations,
            TypeArgumentCount,
            ppClonedArguments,
            pClonedLocations,
            m_pPartialGenericBinding->m_pFixedTypeArgumentBitVector,
            m_pPartialGenericBinding->m_pGenericBinding->GetArgumentCount()
        );

        m_isPartialBinding = false;
        m_pGenericBinding =
            pSymbols->GetGenericBinding
            (
                false,
                pGeneric,
                ppClonedArguments,
                m_pPartialGenericBinding->m_pGenericBinding->GetArgumentCount(),
                NULL,
                false // do not copy the arguments to a new list because we just cloned them
            );

        m_pTypeArgumentLocations = pClonedLocations;
    }
}

void GenericBindingInfo::SetTypeArgumentLocationsAndOldPartialBinding
(
    Location * pTypeArgumentLocations,
    PartialGenericBinding * pOldGenericBinding
)
{
    ThrowIfTrue(m_isPartialBinding);
    ThrowIfTrue(m_pTypeArgumentLocations != NULL);
    m_pTypeArgumentLocations = pTypeArgumentLocations;
    m_pPartialGenericBinding = pOldGenericBinding;
}

void
GenericBindingInfo::MergeArguments
(
    _In_count_(SourceArgumentCount) Type ** ppSourceTypeArguments,
    _In_count_(SourceArgumentCount) Location * pSourceArgumentLocations,
    unsigned long SourceArgumentCount,
    _Inout_count_(DestArgumentCount) Type ** ppDestTypeArguments,
    _Inout_count_(DestArgumentCount) Location * pDestArgumentLocations,
    _In_ IBitVector * pFixedArgumentBitVector,
    unsigned long DestArgumentCount
)
{
    unsigned long srcIndex = 0;
    unsigned long destIndex = 0;

    while (srcIndex < SourceArgumentCount && destIndex < DestArgumentCount)
    {
        if (pFixedArgumentBitVector->BitValue(destIndex))
        {
            ++destIndex;
        }
        else
        {
            ppDestTypeArguments[destIndex] = ppSourceTypeArguments[srcIndex];
            pDestArgumentLocations[destIndex] = pSourceArgumentLocations[srcIndex];
            ++destIndex;
            ++srcIndex;
        }
    }
}

Type ** GenericBindingInfo::CloneTypeArguments(NorlsAllocator * pNorls)
{
    ThrowIfFalse(m_isPartialBinding);
    ThrowIfNull(m_pPartialGenericBinding);

    Type ** ppRet = (Type **)pNorls->Alloc(sizeof(Type *) * m_pPartialGenericBinding->m_pGenericBinding->GetArgumentCount());
    memcpy
    (
        ppRet,
        m_pPartialGenericBinding->m_pGenericBinding->GetArguments(),
        m_pPartialGenericBinding->m_pGenericBinding->GetArgumentCount() * sizeof(Type *)
    );
    return ppRet;
}

Location * GenericBindingInfo::CloneLocations(NorlsAllocator * pNorls)
{
    ThrowIfFalse(m_isPartialBinding);
    ThrowIfNull(m_pPartialGenericBinding);

    Location * pRet = (Location*)pNorls->Alloc(sizeof(Location) * m_pPartialGenericBinding->m_pGenericBinding->GetArgumentCount());
    memcpy
    (
        pRet,
        m_pPartialGenericBinding->m_pTypeArgumentLocations,
        m_pPartialGenericBinding->m_pGenericBinding->GetArgumentCount() * sizeof(Location)
    );
    return pRet;
}

void
GenericBindingInfo::ConvertToFullBindingIfNecessary
(
    Semantics * pSemantics,
    Declaration * pGeneric
)
{
    ThrowIfFalse(pSemantics && pSemantics->GetSymbols());
    if (m_isPartialBinding)
    {
        if (m_pPartialGenericBinding)
        {
            Location * pArgumentLocations = m_pPartialGenericBinding->m_pTypeArgumentLocations;

            m_isPartialBinding = false;
            m_pGenericBinding =
                pSemantics->GetSymbols()->GetGenericBinding
                (
                    false,
                    pGeneric,
                    m_pPartialGenericBinding->m_pGenericBinding->GetArguments(),
                    m_pPartialGenericBinding->m_pGenericBinding->GetArgumentCount(),
                    NULL,
                    true
                );

            m_pTypeArgumentLocations = pArgumentLocations;
        }
        else
        {
            m_isPartialBinding = false;
            m_pTypeArgumentLocations = NULL;
        }
    }
}

bool GenericBindingInfo::operator == (const GenericBindingInfo & src) const
{
    return
        IsNull() && src.IsNull() ||
        (
            m_isPartialBinding == src.m_isPartialBinding &&
            (
                m_isPartialBinding ?
                    m_pPartialGenericBinding == src.m_pPartialGenericBinding :
                    (
                        m_pGenericBinding == src.m_pGenericBinding &&
                        m_pTypeArgumentLocations == src.m_pTypeArgumentLocations
                    )
            )
        );
}

bool GenericBindingInfo::operator != (const GenericBindingInfo & src) const
{
    return ! (*this == src);
}

bool GenericBindingInfo::IsFullMethodBinding()
{
    return
        ! m_isPartialBinding &&
        m_pGenericBinding &&
        ! m_pGenericBinding->IsGenericTypeBinding();
}

BCSYM *
GenericBindingInfo::GetCorrespondingArgument
(
    GenericParameter * pParam
)
{
    ThrowIfTrue(IsNull());

    if (m_isPartialBinding)
    {
        return m_pPartialGenericBinding->m_pGenericBinding->GetCorrespondingArgument(pParam);
    }
    else
    {
        return m_pGenericBinding->GetCorrespondingArgument(pParam);
    }
}

Location *
GenericBindingInfo::GetTypeArgumentLocations()
{
    ThrowIfTrue(IsNull());

    if (m_isPartialBinding)
    {
        return m_pPartialGenericBinding->m_pTypeArgumentLocations;
    }
    else
    {
        return m_pTypeArgumentLocations;
    }
}

GenericBinding *
GenericBindingInfo::GetGenericBindingForErrorText()
{
    if (IsNull())
    {
        return NULL;
    }
    else if (IsPartialBinding())
    {
        return PPartialGenericBinding()->m_pGenericBinding;
    }
    else
    {
        return PGenericBinding();
    }
}

IBitVector *
GenericBindingInfo::GetFixedTypeArgumentBitVector()
{
    if (IsNull() || ! m_pPartialGenericBinding)
    {
        return NULL;
    }
    else
    {
        return m_pPartialGenericBinding->m_pFixedTypeArgumentBitVector;
    }
}

BCSYM_ExtensionCallLookupResult::BCSYM_ExtensionCallLookupResult(NorlsAllocator * pAlloc):
    m_list(NorlsAllocWrapper(pAlloc)),
    m_maxPrecedenceLevel(0),
    m_pInstanceMethodLookupResult(NULL),
    m_pInstanceMethodLookupGenericBinding(NULL),
    m_pInstanceMethodAccessingInstanceType(NULL)
{
    //zero out the memory in our base
    memset((BCSYM *)this, 0, sizeof(BCSYM));

    SetSkKind(SYM_ExtensionCallLookupResult);
}

void BCSYM_ExtensionCallLookupResult::AddProcedure
(
    BCSYM_Proc * pProc,
    unsigned long precedenceLevel,
    PartialGenericBinding * pPartialGenericBinding
)
{
    SymbolEntryFunction;
    Assume(precedenceLevel >= GetMaxPrecedenceLevel(), L"Why is a procedure being added with a lower precedence level than it's predecessors?");

    GetList()->AddLast
    (
        ExtensionCallInfo
        (
            pProc,
            precedenceLevel,
            pPartialGenericBinding
        )
    );
    if (precedenceLevel > GetMaxPrecedenceLevel())
    {
        SetMaxPrecedenceLevel(precedenceLevel);
    }
}

unsigned long BCSYM_ExtensionCallLookupResult::ExtensionMethodCount()
{
    SymbolEntryFunction;
    return GetList()->Count();
}

BCSYM_ExtensionCallLookupResult::iterator_type BCSYM_ExtensionCallLookupResult::GetExtensionMethods()
{
    SymbolEntryFunction;
    return iterator_type(GetList());
}

BCSYM_Proc * BCSYM_ExtensionCallLookupResult::GetFirstExtensionMethod()
{
    SymbolEntryFunction;
    ThrowIfFalse(GetList()->Count());
    return GetList()->GetFirst()->Data().m_pProc;

}

PartialGenericBinding * BCSYM_ExtensionCallLookupResult::GetPartialGenericBindingForFirstExtensionMethod()
{
    SymbolEntryFunction;
    ThrowIfFalse(GetList()->Count());
    return GetList()->GetFirst()->Data().m_pPartialGenericBinding;
}

unsigned long BCSYM_ExtensionCallLookupResult::GetMaxPrecedenceLevel()
{
    SymbolEntryFunction;
    return m_maxPrecedenceLevel;
}

STRING * BCSYM_ExtensionCallLookupResult::GetErrorName(Compiler * pCompiler)
{
    SymbolEntryFunction;
    Assume(GetList()->Count(), L"This method is not valid when the call contains no procedures!");
    return GetList()->GetFirst()->Data().m_pProc->GetErrorName(pCompiler);
}

void BCSYM_ExtensionCallLookupResult::ClearExtensionMethods()
{
    SymbolEntryFunction;
    GetList()->Clear();
}

bool
BCSYM_ExtensionCallLookupResult::CanApplyDefaultPropertyTransformation()
{
    SymbolEntryFunction;
    // Per Paul's e-mail:
    // the default property rule would explicitly be applied if and only if there is a single method,
    // taking into account both instance and extension methods

    if(GetInstanceMethodLookupResult())
    {
        // not a single method
        // If there is an instance method, it isn't single because we have at least one extension method. Overwise we wouldn't have SX_EXTENSION_CALL
        return false;
    }

    iterator_type iterator(GetList());

    if (iterator.MoveNext())
    {
        ExtensionCallInfo current = iterator.Current();
        if
        (
            (current.m_pProc->GetParameterCount() != 1) ||
            IsSub(current.m_pProc) ||
            (
                current.m_pPartialGenericBinding &&
                current.m_pPartialGenericBinding->GetFreeArgumentCount() != 0
            )
        )
        {
            return false;
        }

        if(iterator.MoveNext())
        {
            // not a single method
            return false;
        }
    }

    return true;
}

void
BCSYM_ExtensionCallLookupResult::SetInstanceMethodResults
(
    Declaration * pLookupResult,
    GenericBinding * pGenericBinding,
    Type * pAccessingInstanceType
)
{
    SymbolEntryFunction;
    SetInstanceMethodLookupResult(pLookupResult);
    SetInstanceMethodLookupGenericBinding(pGenericBinding);
    SetInstanceMethodAccessingInstanceType(pAccessingInstanceType);
}

Declaration *
BCSYM_ExtensionCallLookupResult::GetInstanceMethodLookupResult()
{
    SymbolEntryFunction;
    return m_pInstanceMethodLookupResult;
}

GenericBinding *
BCSYM_ExtensionCallLookupResult::GetInstanceMethodLookupGenericBinding()
{
    SymbolEntryFunction;
    return m_pInstanceMethodLookupGenericBinding;
}

Type *
BCSYM_ExtensionCallLookupResult::GetAccessingInstanceTypeOfInstanceMethodLookupResult()
{
    SymbolEntryFunction;
    return m_pInstanceMethodAccessingInstanceType;
}

bool
BCSYM_ExtensionCallLookupResult::IsOverloads()
{
    SymbolEntryFunction;
    return
        m_pInstanceMethodLookupResult ||
        (
            !m_pInstanceMethodLookupResult &&
            ExtensionMethodCount() != 1
        );
}

BCSYM_Implements *
BCSYM::GetFirstImplements
(
)
{
    SymbolEntryFunction;
    if (IsClass())
    {
        return PClass()->GetFirstImplements();
    }
    else if (IsInterface())
    {
        return PInterface()->GetFirstImplements();
    }

    return NULL;
}


BCSYM * 
BCSYM::DigThroughArrayLiteralType
(
    Symbols * pSymbols
)
{   
    SymbolEntryFunction;
    BCSYM * pType = DigThroughAlias();

    if (pType->IsArrayLiteralType())
    {
        BCSYM_ArrayLiteralType * pArrayLiteralType = pType->PArrayLiteralType();
        BCSYM_ArrayType * pRet = pArrayLiteralType->GetCorrespondingArrayType();

        if (! pRet)
        {
            pRet = pSymbols->GetArrayType(pArrayLiteralType->GetRank(), pArrayLiteralType->GetRoot());
            pArrayLiteralType->SetCorrespondingArrayType(pRet);
        }
        ThrowIfNull(pRet);
        return pRet;
    }
    else if (pType->IsPointerType())
    {
        BCSYM * pPointerRoot = pType->PPointerType()->GetRoot();
        BCSYM * pNested = pPointerRoot->DigThroughArrayLiteralType(pSymbols);

        if (pNested != pPointerRoot)
        {
            return pSymbols->MakePtrType(pNested);
        }
    }
    else if (pType->IsGenericTypeBinding())
    {
        BCSYM_GenericTypeBinding * pBinding = pType->PGenericTypeBinding();
        AutoArray<BCSYM *> arguments(new (zeromemory) BCSYM * [pBinding->GetArgumentCount()]);
        bool createNew = false;

        for (unsigned  i = 0; i < pBinding->GetArgumentCount();++i)
        {
            arguments[i] = pBinding->GetArgument(i)->DigThroughArrayLiteralType(pSymbols);
            if (arguments[i] != pBinding->GetArgument(i))
            {
                createNew = true;
            }
        }

        BCSYM_GenericTypeBinding * pParentBinding = pBinding->GetParentBinding() ? pBinding->GetParentBinding()->DigThroughArrayLiteralType(pSymbols)->PGenericTypeBinding() : NULL;

        if (pParentBinding != pBinding->GetParentBinding())
        {
            createNew = true;
        }

        if (createNew)
        {
            pBinding = 
                pSymbols->GetGenericBinding
                (
                    pBinding->IsBad(),
                    pBinding->GetGenericType(),
                    arguments,
                    pBinding->GetArgumentCount(),
                    pParentBinding,
                    true //copy the arguments.
                )->PGenericTypeBinding();
        }

        return pBinding;
    }

    return this;

}


// If this is a named type symbol, dig through to the real type.
// (resolving it first, if it hadn't yet been resolved)
// It is safe to call this on a NULL pointer.
BCSYM * BCSYM::DigThroughNamedType()
{
    SymbolEntryFunction;
    BCSYM *Symbol = this;

    if ( Symbol != NULL && Symbol->GetKind() == SYM_NamedType )
    {
        do
        {
            if (!Symbol->PNamedType()->GetSymbol())
            {
                // If named type not yet resolved, try to resolve it now
                //
                // This is needed because during bindable, some constant
                // expressions in declarations might need to be evaluated
                // for which we might need to dig through the named types
                // in containers which have not yet been through named type
                // resolution.
                //
                Bindable::ResolveNamedTypeIfPossible(
                    Symbol->PNamedType(),
                    NULL);
            }

            Symbol = Symbol->PNamedType()->GetSymbol();

            // [Microsoft] Can we really have a NamedType resolving to yet another NamedType?
            // I couldn't come up with any scenario like that. Adding this Assert because this could make a 
            // difference for NoPia feature.
            AssertIfTrue(Symbol != NULL && Symbol->GetKind() == SYM_NamedType); 
        }
        while (Symbol != NULL && Symbol->GetKind() == SYM_NamedType);

        // If the target is an embedded local type, we should return a reference to
        // the canonical type instead. When importing a project, we will create NamedType
        // references to these embedded local types, instead of returning direct references
        // to those types; this gives us an opportunity now, while digging through the
        // NamedType object, to unify the types.
        if (Symbol != NULL && 
            !Symbol->IsGenericBinding() &&  // We do not embed generic bindings, let's filter them out early
            Symbol->IsContainer() )
        {
            BCSYM_Container * pContainer = Symbol->PContainer();

            CompilerProject * pContainerProject = NULL;
            CompilerFile *pContainerFile = pContainer->GetCompilerFile();

            if ( pContainerFile != NULL )
            {
                pContainerProject = pContainerFile->GetCompilerProject();
            }

            if ( pContainerProject != NULL )
            {
                CompilerProject * pProjectBeingCompiled = pContainerProject->GetCompiler()->GetProjectBeingCompiled();

                if ( pProjectBeingCompiled != NULL )
                {
                    BCSYM_NamedType * pNamedType = this->PNamedType();
                    CompilerProject * pNamedTypeProject = pNamedType->GetContext()->GetContainingProject();

#if DEBUG
                    // Only if we are crossing project boundaries with NamedType, we have to check if we need to look for a 
                    // canonical type. Otherwise, named type binding should have given us the canonical type.
                    bool fFoundCanonicalType = false;

                    AssertIfFalse(
                                pNamedTypeProject == NULL || 
                                pProjectBeingCompiled != pNamedTypeProject ||
                                pProjectBeingCompiled == pContainerProject || 
                                ( pProjectBeingCompiled->IsProjectReferenced(pContainerProject) &&
                                  (!TypeHelpers::IsEmbeddedLocalType(pContainer) || // not a local type
                                   (Semantics::GetCanonicalTypeFromLocalCopy(pContainer, fFoundCanonicalType) == pContainer && !fFoundCanonicalType)) ) // canonical type couldn't be located 
                                );
#endif
            
                    if (pNamedTypeProject != NULL && pProjectBeingCompiled != pNamedTypeProject)
                    {
                        // check if symbol is embedded or going to be embedded into the NamedType's project
                        bool isEmbeddedLocalType = false;
                        
                        if ( pContainerProject == pNamedTypeProject)
                        {
                            // The symbol is defined in the same project as the NamedType.
                            // This check should be equivalent to the one performed in [bool IsEmbeddedLocalType(_In_ BCSYM_Container *pType)]
                            isEmbeddedLocalType = TypeHelpers::IsMarkedAsEmbeddedLocalType(pContainer);

                            AssertIfFalse(TypeHelpers::IsEmbeddedLocalType(pContainer) == isEmbeddedLocalType);
                        }
                        else
                        {
#if !IDE 
                            // All three projects (pProjectBeingCompiled, pNamedTypeProject and pContainerProject) shouldn't be different 
                            // in VBC scenarios or the NamedType's project should be Metadata.
                            AssertIfFalse(pNamedTypeProject->IsMetaData());
#else
                            // This is needed to address Dev10 #678696
                            AssertIfFalse(pNamedTypeProject->IsProjectReferenced(pContainerProject));
                            if (pNamedTypeProject->HasEmbeddedReferenceTo(pContainerProject))
                            {
                                isEmbeddedLocalType = true;
                            }
#endif
                        }

                        if (isEmbeddedLocalType)
                        {
                            AssertIfFalse(pProjectBeingCompiled->IsProjectReferenced(pNamedTypeProject));

                            // Dev10 #751770
                            bool * pReferredToEmbeddableInteropType = pProjectBeingCompiled->Get_pReferredToEmbeddableInteropType();
                            if (pReferredToEmbeddableInteropType != NULL)
                            {            
                                *pReferredToEmbeddableInteropType = true;
                            }

#if IDE 
                            BCSYM_Container ** pAddrTreatAsEmbeddedLocalType = VBTLSThreadData::GetTreatAsEmbeddedLocalType();
                            ThrowIfTrue(*pAddrTreatAsEmbeddedLocalType != NULL); // shouldn't get into a nesting case

                            // This is needed to address Dev10 #678696
                            *pAddrTreatAsEmbeddedLocalType = pContainer;

                            AssertIfFalse (TypeHelpers::IsEmbeddedLocalType(pContainer));
#endif                            

                            bool fFoundCanonicalType = false;
                            Symbol = Semantics::GetCanonicalTypeFromLocalCopy(pContainer, fFoundCanonicalType);

#if IDE 
                            // restore
                            *pAddrTreatAsEmbeddedLocalType = NULL;

                            // This is needed to address Dev10 #678696
                            if (Symbol == pContainer && !fFoundCanonicalType && 
                                pProjectBeingCompiled->GetTrackUnsubstitutedLocalTypes())
                            {
                                CompilerProject * pProject = NULL;
                                
                                // record not substituted local type and its owner
                                if (pProjectBeingCompiled->m_UnsubstitutedLocalTypes.GetValue(Symbol, &pProject))
                                {
                                    // We should be doing this tracking only while emitting member ref signature.
                                    // This means that all local types belong to one project.
                                    ThrowIfFalse(pProject == pNamedTypeProject);
                                }
                                else
                                {
                                    pProjectBeingCompiled->m_UnsubstitutedLocalTypes.SetValue(Symbol, pNamedTypeProject);
                                }
                            }
#endif                            
                        }
                    }
                }
            }
        }
    }

    VSASSERT( !this || Symbol, "A NamedType must have a type." );
    return Symbol;
}


// Get to the root type, digging through everything we can.
// It is safe to call this on a NULL pointer.
BCSYM * BCSYM::ChaseToType()
{
    SymbolEntryFunction;
    BCSYM *psym = this;

    while (psym)
    {
        switch (psym->GetKind())
        {
        case SYM_Alias:
            psym = psym->PAlias()->DigThroughAlias();
            continue;

        case SYM_NamedType:
            psym = psym->PNamedType()->DigThroughNamedType();
            continue;

        case SYM_PointerType:
            psym = psym->PPointerType()->GetRoot();
            continue;

        case SYM_ArrayLiteralType:
        case SYM_ArrayType:
            psym = psym->PArrayType()->GetRoot();
            continue;

        default:
            goto Done;
        }
    }

Done:
    VSASSERT(!this || psym, "Must have a type.");
    return psym;
}

BCSYM * BCSYM::ChaseThroughPointerTypes()
{
    SymbolEntryFunction;
    switch (GetKind())
    {
        case SYM_PointerType:
            return PPointerType()->GetRoot();
        default:
            return this;
    }
}

BCSYM * BCSYM::ChaseThroughPointerTypesAndGenericTypeBindings()
{
    SymbolEntryFunction;
    BCSYM * psym = this;

    while (psym)
    {
        if (psym->GetKind() == SYM_PointerType)
        {
            psym = PPointerType()->GetRawRoot();
        }
        else if (psym->GetKind() == SYM_GenericTypeBinding)
        {
            psym = psym->PGenericTypeBinding()->GetGenericType();
        }
        else
        {
            break;
        }
    }

    return psym;
}

// Get to the raw namedtype, digging through everything we can.
// It is safe to call this on a NULL pointer.
BCSYM * BCSYM::ChaseToNamedType()
{
    SymbolEntryFunction;
    BCSYM *psym = this;

    while (psym)
    {
        switch (psym->GetKind())
        {
        case SYM_Alias:
            psym = psym->PAlias()->DigThroughAlias();
            continue;

        case SYM_PointerType:
            psym = psym->PPointerType()->GetRawRoot();
            continue;

        case SYM_ArrayLiteralType:   
        case SYM_ArrayType:
            psym = psym->PArrayType()->GetRawRoot();
            continue;

        case SYM_NamedType:
        default:
            goto Done;
        }
    }

Done:
    VSASSERT(!this || psym, "Must have a named type.");
    return psym;
}

// Get location of the raw namedtype.
// It is safe to call this on a NULL pointer.
Location * BCSYM::GetTypeReferenceLocation()
{
    SymbolEntryFunction;
    BCSYM *psym = this->ChaseToNamedType();

    VSASSERT(!this || psym, "Must have a named type.");

    return this ? psym->GetLocation() : NULL;
}

bool AreContainersEquivalent
(
    BCSYM_Container *pContainer1,
    BCSYM_Container *pContainer2
)
{
    AssertIfNull(pContainer1);
    AssertIfNull(pContainer2);

    // Optimization for common case.
    //
    if (!StringPool::IsEqual(pContainer1->GetName(), pContainer2->GetName()))
    {
        return false;
    }

    CompilerFile *pFile1 = pContainer1->GetCompilerFile();
    CompilerFile *pFile2 = pContainer2->GetCompilerFile();

    if (!pFile1 || !pFile2)
    {
        return false;
    }

    CompilerProject *pProj1 = pFile1->GetCompilerProject();
    CompilerProject *pProj2 = pFile2->GetCompilerProject();

    if (!pProj1 || !pProj2)
    {
        return false;
    }

    if (!pProj1->IsMetaData() ||
        !pProj2->IsMetaData() ||
        !StringPool::IsEqual(
            pProj1->GetAssemblyIdentity()->GetAssemblyIdentityString(),
            pProj2->GetAssemblyIdentity()->GetAssemblyIdentityString()))
    {
        return false;
    }

    BCSYM_Container *pCurrent1 = pContainer1;
    BCSYM_Container *pCurrent2 = pContainer2;

    while (pCurrent1 && pCurrent2)
    {
        if (pCurrent1->GetKind() != pCurrent2->GetKind())
        {
            return false;
        }

        if (pCurrent1->IsNamespace())
        {
            if (!StringPool::IsEqual(pCurrent1->GetQualifiedName(), pCurrent2->GetQualifiedName()))
            {
                return false;
            }

            // Match
            pCurrent1 = NULL;
            pCurrent2 = NULL;
            break;
        }

        if (!StringPool::IsEqual(pCurrent1->GetName(), pCurrent2->GetName()) ||
            pCurrent1->GetGenericParamCount() != pCurrent2->GetGenericParamCount())
        {
            return false;
        }

        if (pCurrent1->IsClass() &&
            (pCurrent1->PClass()->IsEnum() != pCurrent2->PClass()->IsEnum() ||
                pCurrent1->PClass()->IsStruct() != pCurrent2->PClass()->IsStruct() ||
                pCurrent1->PClass()->IsStdModule() != pCurrent2->PClass()->IsStdModule()))
        {
            return false;
        }

        pCurrent1 = pCurrent1->GetContainer();
        pCurrent2 = pCurrent2->GetContainer();
    }

    return (pCurrent1 == NULL && pCurrent2 == NULL);
}

//static
bool BCSYM::AreTypesEqual(BCSYM *ptyp1, BCSYM *ptyp2)
{
    return AreTypesEqual(ptyp1, ptyp2, true /* Dig into arrays and generic bindings */);
}

//static
bool BCSYM::AreTypesEqual(BCSYM *ptyp1, BCSYM *ptyp2, bool fDigIntoArraysAndGenericBindings)
{
    // This assertion is necessary to avoid needless calls to DigThroughAlias.

    VSASSERT(ptyp1 == ptyp1->DigThroughAlias() && ptyp2 == ptyp2->DigThroughAlias(), "Dig through aliases before calling this function");
    bool fEqual = false;

    // If both are pointing to the same symbol, it's obviously the same type.
    if (ptyp1 == ptyp2)
    {
        fEqual = true;
    }
    else if (!ptyp1 || !ptyp2)
    {
        fEqual = false;
    }
    else if (fDigIntoArraysAndGenericBindings && ptyp1->IsArrayType() && ptyp2->IsArrayType())
    {
        BCSYM_ArrayType *parrtyp1 = ptyp1->PArrayType();
        BCSYM_ArrayType *parrtyp2 = ptyp2->PArrayType();

        // The same array type may have multiple symbols associated with it
        fEqual = (parrtyp1->GetRank() == parrtyp2->GetRank() &&
                  AreTypesEqual(parrtyp1->GetRoot()->DigThroughAlias(), parrtyp2->GetRoot()->DigThroughAlias()));
    }
    else if (fDigIntoArraysAndGenericBindings && ptyp1->IsGenericTypeBinding() && ptyp2->IsGenericTypeBinding())
    {
        fEqual = BCSYM_GenericTypeBinding::AreTypeBindingsEqual(ptyp1->PGenericTypeBinding(), ptyp2->PGenericTypeBinding());
    }

    if (!fEqual && ptyp1 && ptyp2 &&
        ptyp1->IsContainer() && !ptyp1->IsGenericTypeBinding() &&
        ptyp2->IsContainer() && !ptyp2->IsGenericTypeBinding())
    {
        BCSYM_Container * pContainer1 = ptyp1->PContainer();
        BCSYM_Container * pContainer2 = ptyp2->PContainer();
    
        fEqual = AreContainersEquivalent(pContainer1, pContainer2);
#if IDE 
        // Call special matching logic for unmerged anonymous types.
        // Note, that we require both types to be in the transient symbol store.
        // This is done to avoid matching with unregistered clones produced in
        // AnonymousTypeSemantics::GetAnonymousTypeBinding for With context purposes.
        // Not doing that would break the circular AT definition check.
        if (!fEqual &&
            pContainer1->IsAnonymousType() &&
            pContainer2->IsAnonymousType() &&
            pContainer1->IsTransient() &&
            pContainer2->IsTransient() &&
            (pContainer1->IsNonMergedAnonymousType() ||
             pContainer2->IsNonMergedAnonymousType()))
        {
            fEqual = MatchAnonymousType(pContainer1, pContainer2);
        }
#endif

        if (!fEqual && TypeHelpers::AreTypeIdentitiesEquivalent(pContainer1, pContainer2))
        {
            // We will always consider two types to be equivalent if they are marked with appropiate
            // type identity attributes, according to the CLR type unification rules.
            fEqual = true;
        }
    }

#if IDE 
    // Handle the non-merged anonymous delegate case. This might arise e.g. when
    // calling Sub f(Of T)(x as T, y as T) with f(Sub() 1, Sub() 2). At IDE time,
    // they'll each have different delegate types, e.g. VB$AnonymousDelegate_51
    // and VB$AnonymousDelegate_52. The compiler knows that it will merge the two
    // types at a later stage. The IDE needs to pretend they're already merged.
    // Two delegates types will be merged if and only if
    //   (1) they are both anonymous delegates and both have zero generic parameters, or
    //   (2) they are both anonymous delegates and both are generic bindings and
    //       both have equivalent generic bindings.
    // Dev10#495613: we hadn't considered Point(1) previously. Note that it only arose
    // thanks to parameterless statement lambdas; it never occured with function lambdas,
    // and never occured with parameterised statement lambdas.
    if (!fEqual && ptyp1 && ptyp2 &&
        fDigIntoArraysAndGenericBindings &&
        ptyp1->IsContainer() && ptyp1->IsAnonymousDelegate() &&
        ptyp2->IsContainer() && ptyp2->IsAnonymousDelegate() &&
        (ptyp1->IsGenericTypeBinding() == ptyp2->IsGenericTypeBinding()) &&
        (ptyp1->PContainer()->IsNonMergedAnonymousType() ||
         ptyp2->PContainer()->IsNonMergedAnonymousType()) )
    {
        // Note: MatchAnonymousDelegate doesn't look at the arguments, only the anonymous delegate class.
        // so therefore we have to manually compare the binding arguments.
        fEqual = MatchAnonymousDelegate(ptyp1->PContainer(), ptyp2->PContainer()) &&
        (!ptyp1->IsGenericTypeBinding() ||
             BCSYM_GenericBinding::CompareBindingArguments(ptyp1->PGenericTypeBinding(), ptyp2->PGenericTypeBinding()));
    }
#endif

    return fEqual;
}

//static
bool BCSYM_GenericTypeBinding::AreTypeBindingsEqual
(
    BCSYM_GenericTypeBinding *pBinding1,
    BCSYM_GenericTypeBinding *pBinding2
)
{
    if (!BCSYM::AreTypesEqual(pBinding1->GetGenericType(), pBinding2->GetGenericType()))
    {
        return false;
    }

    while (pBinding1 && pBinding2)
    {
        if (!BCSYM_GenericBinding::CompareBindingArguments(pBinding1, pBinding2))
        {
            return false;
        }

        pBinding1 = pBinding1->GetParentBinding();
        pBinding2 = pBinding2->GetParentBinding();
    }

    return pBinding1 == NULL && pBinding2 == NULL;
}

DECLFLAGS  BCSYM_Member::GetDeclFlags()
{
    SymbolEntryFunction;
    DECLFLAGS DeclarationFlags = 0;

    switch(GetAccess())
    {
    case ACCESS_CompilerControlled:
        DeclarationFlags|= DECLF_CompilerControlled;
        break;

    case ACCESS_Private:
        DeclarationFlags |= DECLF_Private;
        break;

    case ACCESS_Protected:
        DeclarationFlags |= DECLF_Protected;
        break;

    case ACCESS_ProtectedFriend:
        DeclarationFlags |= DECLF_ProtectedFriend;
        break;

    case ACCESS_Friend:
        DeclarationFlags |= DECLF_Friend;
        break;

    case ACCESS_Public:
        DeclarationFlags |= DECLF_Public;
        break;

    default:
        VSFAIL("Bad access.");
        break;
    };

    if (IsHidden())
    {
        DeclarationFlags |= DECLF_Hidden;
    }

    if (IsSpecialName())
    {
        DeclarationFlags |= DECLF_SpecialName;
    }

    if (IsShared())
    {
        DeclarationFlags |= DECLF_Shared;
    }

    if (IsShadowsKeywordUsed())
    {
        DeclarationFlags |= DECLF_ShadowsKeywordUsed;
    }

    if (IsProperty())
    {
        BCSYM_Property *pprop = PProperty();

        if (pprop->IsDefault())
            DeclarationFlags |= DECLF_Default;
    }

    if (IsProc())
    {
        BCSYM_Proc *pproc = PProc();

        if (pproc->IsOverridesKeywordUsed())
        {
            DeclarationFlags |= DECLF_OverridesKeywordUsed;
        }

        if (pproc->IsOverloadsKeywordUsed())
        {
            DeclarationFlags |= DECLF_OverloadsKeywordUsed;
        }

        if (pproc->IsMustOverrideKeywordUsed())
        {
            DeclarationFlags |= DECLF_MustOverride;
        }

        if (pproc->IsNotOverridableKeywordUsed())
        {
            DeclarationFlags |= DECLF_NotOverridable;
        }

        if (pproc->IsOverridableKeywordUsed())
        {
            DeclarationFlags |= DECLF_Overridable;
        }

        if (pproc->IsUserDefinedOperator())
        {
            if (pproc->PUserDefinedOperator()->GetOperator() == OperatorWiden)
            {
                DeclarationFlags |= DECLF_Widening;
            }
            else if (pproc->PUserDefinedOperator()->GetOperator() == OperatorNarrow)
            {
                DeclarationFlags |= DECLF_Narrowing;
            }
        }
    }

    if (IsVariable())
    {
        BCSYM_Variable *pvar = PVariable();

        if (pvar->IsStatic())
        {
            DeclarationFlags |= DECLF_Static;
        }

        if (pvar->IsNew())
        {
            DeclarationFlags |= DECLF_New;
        }

        if (pvar->IsReadOnly())
        {
            DeclarationFlags |= DECLF_ReadOnly;
        }

        if ( pvar->IsImplicitDecl() )
        {
            DeclarationFlags |= DECLF_NotDecled;
        }

        switch(pvar->GetVarkind())
        {
        case VAR_WithEvents:
            DeclarationFlags |= DECLF_WithEvents;

            __fallthrough;

        case VAR_Member:
        case VAR_Local:
        case VAR_Param:

            DeclarationFlags |= DECLF_Dim;
            break;

        case VAR_Const:
            DeclarationFlags |= DECLF_Const;
            break;
        }
    }

    return DeclarationFlags;
}

void BCSYM_Variable::SetTemporaryManager(TemporaryManager *tempmgr)
{
    SymbolEntryFunction;
    ThrowIfFalse(IsTemporary());
    ThrowIfNull(tempmgr->GetTemporary(this));   // Make sure we're actually in this temporary manager
    m_temporaryMgr = tempmgr;
}

Temporary *BCSYM_Variable::GetTemporaryInfo()
{
    SymbolEntryFunction;
    ThrowIfFalse(IsTemporary());
    ThrowIfNull(GetTemporaryManager());
    return GetTemporaryManager()->GetTemporary(this);
}

BCSYM_Expression *
BCSYM_VariableWithArraySizes::GetArraySize
(
    unsigned Dimension
)
{
    SymbolEntryFunction;
    return m_ArraySizes[Dimension];
}

bool BCSYM_Param::IsParamArray()
{
    SymbolEntryFunction;
    return IsParamArrayRaw() ||
           GetPAttrVals()->GetPWellKnownAttrVals()->GetParamArrayData();
}

WellKnownAttrVals* BCSYM_Param::GetPWellKnownAttrVals()
{
    SymbolEntryFunction;
    // Note: WellKnownAttrs::GetPWellKnownAttrVals is NULL safe
    return GetPAttrVals()->GetPWellKnownAttrVals();
}

// The type that the compiler uses for type checking and coercion
// semantics.  Among other things, this digs through aliases.
BCSYM *BCSYM_Param::GetCompilerType()
{
    SymbolEntryFunction;
    BCSYM *ptyp = GetType()->DigThroughAlias();
    return ptyp;
}

bool BCSYM_GenericConstraint::IsNewConstraint()
{
    SymbolEntryFunction;
    return IsGenericNonTypeConstraint() &&
           PGenericNonTypeConstraint()->IsNewConstraint();
}

bool BCSYM_GenericConstraint::IsReferenceConstraint()
{
    SymbolEntryFunction;
    return IsGenericNonTypeConstraint() &&
           PGenericNonTypeConstraint()->IsReferenceConstraint();
}

bool BCSYM_GenericConstraint::IsValueConstraint()
{
    SymbolEntryFunction;
    return IsGenericNonTypeConstraint() &&
           PGenericNonTypeConstraint()->IsValueConstraint();
}

BCSYM *BCSYM_PointerType::GetCompilerRoot()
{
    SymbolEntryFunction;
    BCSYM *ptyp = GetRoot()->DigThroughAlias();
    return ptyp;
}


// If this is an alias, named type, com named type, enum,
// dig through the alias to the real type.
// It is safe to call this on a NULL pointer.
//
// Note that this WILL bind named types on demand
//
BCSYM * BCSYM::DigThroughAlias()
{
    SymbolEntryFunction;
    BCSYM *psym = this;

    while (psym)
    {
        switch(psym->GetKind())
        {
        case SYM_NamedType:
            psym = psym->PNamedType()->DigThroughNamedType();
            break;

        case SYM_Alias:
            psym = psym->PAlias()->GetAliasedSymbol();
            break;

        default:
            goto Done;
        }
    }

Done:
    VSASSERT(!this || psym, "Must have a type.");
    return psym;

} // BCSYM::DigThroughAlias

// If this is an alias, named type, com named type, enum,
// dig through the alias to the real type.
//
// Note that this WILL NOT bind named types on demand
//
BCSYM * BCSYM::DigThroughAliasToleratingNullResult()
{
    SymbolEntryFunction;
    BCSYM *psym = this;
    BCSYM_NamedType *pNamedType = NULL;

    while (psym)
    {
        switch(psym->GetKind())
        {
        case SYM_NamedType:

            pNamedType = psym->PNamedType();
            psym = pNamedType->GetSymbol();

            // Dev10 #680929 NamedTypes must be dug through in order to be substituted with corresponding canonical type.
            if (psym != NULL && !psym->IsNamedType())
            {
                psym = pNamedType->DigThroughNamedType();
            }
            
            break;

        case SYM_Alias:
            psym = psym->PAlias()->GetAliasedSymbol();
            break;

        default:
            goto Done;
        }
    }

Done:
    return psym;

} // BCSYM::DigThroughAliasToleratingNullResult

inline bool BCSYM::IsBad() const
{
    SymbolEntryFunction;
    // 



    if (IsArrayType())
    {
        // There are times in declared when this is asked and NamedTypes have not yet been bound.
        // So the extra complexity here when dealing with NamedType element types.
        // Microsoft:2008.09.21: consider moving IsArrayType() && PArrayType()->ChaseToNamedType()->IsBad()
        // into the main list below (for robustness, even though it's never used). If so, also 
        // add an assert VSASSERT(!IsArrayType() || !PArrayType()->ChaseToNamedType()->IsBad(), "unexpected: we shouldn't have created an array of bad element type.");

        BCSYM *ElementType = PArrayType()->ChaseToNamedType();

        return
            ElementType->IsNamedType() ?
                ElementType->PNamedType()->GetSymbol() && ElementType->PNamedType()->GetSymbol()->IsBad() :
                ElementType->IsBad();
    }

    // Dev10#489103: In Orcas, we didn't used to have the IsNamedType() test below.
    // That lead to a number of situations where code had a NamedType which pointed to a
    // GenericBadRoot, and where we tested for NamedType()->IsBad(), and got the answer that
    // it WASN'T bad!!!
    // One situation was that Declared would construct an array of element type named-type-pointing-to-bad-type.
    // Another was that lambda params of bad type would not be considered bad.
    // Another was that we'd check whether a signature could handle an event even though that signature had bad params.
    // For Dev10, we've added the IsNamedType() test below. And we've ----ed down: in ClassifyCLRConversion,
    // we now throw an assert if we're given a bad type. (which used to happen before).
    //
    // Consider: enable the following assert:
    // VSASSERT(!IsNamedType() || !PNamedType()->GetSymbol() || !PNamedType()->GetSymbol()->IsBad(), "unexpected: a named type points to a bad symbol. Notionally this shouldn't happen. The thing that contains the named type should already have been marked as bad, and so it should never even have tried checking the named type for badness.");
    // This is enforces the notion that "if ever a thing (e.g. signature or lambda) has an element
    // with a NamedType which points to a bad type, then that thing itself should already have been marked
    // as bad, and so we shouldn't even be testing whether the NamedType is bad or not."
    // However, there are places where that notion breaks down (e.g. when checking a bad constraint),
    // so other pieces of code would have to be tidied up first.

    return(
        IsNamedRoot() && PNamedRoot()->IsBadNamedRoot() ||
        IsImplements() && PImplements()->IsBadImplements() ||
        IsParam() && PParam()->IsBadParam() ||
        IsHandlesList() && PHandlesList()->IsBadHandlesList() ||
        IsApplAttr() && PApplAttr()->IsBadApplAttr() ||
        IsGenericBinding() && PGenericBinding()->IsBadGenericBinding() ||
        IsNamedType() && PNamedType()->GetSymbol() && PNamedType()->GetSymbol()->IsBad() // Dev10#489103
        );

}

//============================================================================
// Returns the location of the symbol.
//============================================================================

Location* BCSYM::GetLocation()
{
    SymbolEntryFunction;
    if (!HasLocation())
    {
        return NULL;
    }

    TrackedLocation* pTrackedLocation = reinterpret_cast< TrackedLocation*>( this ) - 1;
    return static_cast<Location *>( pTrackedLocation );
}

//============================================================================
// Returns the TrackedLocation of the symbol.
//============================================================================

TrackedLocation* BCSYM::GetTrackedLocation()
{
    SymbolEntryFunction;
    if (!HasLocation())
    {
        return NULL;
    }

    return reinterpret_cast<TrackedLocation*>( this ) - 1;
}

//****************************************************************************
// Implementation of "GetBasicRep".
//****************************************************************************

// Add the "goo" for an array type.
//static
BCSYM *BCSYM::FillInArray(
                    Compiler *pCompiler,
                    BCSYM *pType,
                    StringBuffer *pbuf
                )
{
    AssertIfNull(pType);

    if (pType->IsPointerType())
    {
        pType = pType->PPointerType()->GetRoot();
    }

    while (pType->IsArrayType())
    {
        BCSYM_ArrayType *pArrayType = pType->PArrayType();
        unsigned iDim, cDims = pArrayType->GetRank();

        pbuf->AppendChar(L'(');

        for (iDim = 1; iDim < cDims; iDim++)
        {
            pbuf->AppendChar(L','); // seperate elements in a multi-dimensioned array
        }

        pbuf->AppendChar(L')');

        pType = pArrayType->GetRoot();
    }

    return pType;
}

//static
void
BCSYM::FillInGenericParams
(
    Compiler *pCompiler,
    BCSYM_Container *pContextContainer,
    BCSYM * psym,
    StringBuffer *pbuf,
    BCSYM_GenericBinding *pGenericBindingContext,
    TIPKIND Special,
    IReadonlyBitVector *pGenericParamsToExclude
)
{
    if (psym->IsGeneric())
    {
        bool fOutputtedFirstParam = false;
        unsigned long ulIndex = 0;

        for (BCSYM_GenericParam *TypeParam = psym->GetFirstGenericParam(); TypeParam; TypeParam = TypeParam->GetNextParam())
        {
            if (!pGenericParamsToExclude || !pGenericParamsToExclude->BitValue(ulIndex))
            {
                if (fOutputtedFirstParam)
                {
                    pbuf->AppendString(L", ");
                }
                else
                {
                    pbuf->AppendString(L"(Of ");
                    fOutputtedFirstParam = true;
                }

                TypeParam->GetBasicRep(pCompiler, pContextContainer, pbuf, pGenericBindingContext, NULL, pGenericBindingContext == NULL, Special);
            }

            ulIndex++;
        }

        if (fOutputtedFirstParam)
        {
            pbuf->AppendChar(L')');
        }
    }
}

// Will fill in the parameters for a procedure symbol
//static
void BCSYM::FillInProc
(
    Compiler *pCompiler,
    BCSYM_Container *pContextContainer,
    BCSYM_Proc * pproc,
    StringBuffer *pbuf,
    BCSYM_GenericBinding *pGenericBindingContext,
    TIPKIND Special,
    IReadonlyBitVector *pGenericParamsToExclude,
    _In_opt_z_ STRING* ParameterReplaceString,
    bool FillReturnType
)
{
    FillInGenericParams(pCompiler, pContextContainer, pproc, pbuf, pGenericBindingContext, Special, pGenericParamsToExclude);
    
    FillInParameterAndReturnType(pCompiler, pContextContainer, pproc, pbuf, pGenericBindingContext, Special, ParameterReplaceString, FillReturnType);
}

void BCSYM::FillInParameterAndReturnType
(
    Compiler *pCompiler,
    BCSYM_Container *pContextContainer,
    BCSYM_Proc * pproc,
    StringBuffer *pbuf,
    BCSYM_GenericBinding *pGenericBindingContext,
    TIPKIND Special,
    _In_opt_z_ STRING* ParameterReplaceString,
    bool FillReturnType
)
{
    BCSYM_Param *pparam = NULL;
    BCSYM_Param *pparamNext = NULL;
    bool addParenthesis = true; 

    pparam = pproc->GetFirstParam();

    if (pproc && pproc->IsProperty() && !pparam)
    {
        // No parenthesis if this is a property with no parameters.
        addParenthesis = false;
    }

    if (addParenthesis)
    {
        pbuf->AppendChar(L'(');
    }

    // Skip the first parameter for an extension method call
    if ((Special == TIP_ExtensionCall || Special == TIP_ExtensionAggregateOperator) && pproc->IsExtensionMethod())
    {
        pparam = pparam->GetNext();
    }

    TIPKIND paramTipKind = Special;

    if (Special == TIP_AggregateOperator || Special == TIP_ExtensionAggregateOperator)
    {
        paramTipKind = TIP_AggregateOperatorParam;
    }

    if (ParameterReplaceString == NULL)
    {
        for ( ; pparam; pparam = pparamNext)
        {
            pparamNext = pparam->GetNext();

            pparam->GetBasicRep(pCompiler, pContextContainer, pbuf, pGenericBindingContext, NULL, true, paramTipKind);

            if (pparamNext)
            {
                pbuf->AppendWithLength(WIDE(", "), 2);
            }

            // TIP_AggregateOperatorParam only applies to the first displayed parameter
            if (paramTipKind == TIP_AggregateOperatorParam)
            {
                paramTipKind = TIP_Normal;
            }
        }
    }
    else if(pparam != NULL)
    {
        pbuf->AppendString(ParameterReplaceString);
    }

    if (addParenthesis)
    {
        pbuf->AppendChar(WIDE(')'));
    }

    if (FillReturnType && pproc->GetType())
    {
        pbuf->AppendWithLength(WIDE(" As "), 4);

        BCSYM *pType = pproc->GetType();

        pType->GetBasicRep(pCompiler, pContextContainer, pbuf, pGenericBindingContext, NULL, false);
    }
}

// Will fill in the parameters for a procedure symbol
//static
void BCSYM::FillInAnonymousDelegate
(
    Compiler *pCompiler,
    BCSYM_Container *pContextContainer,
    BCSYM_Class * pAnonymousDelegate,
    StringBuffer *pbuf,
    BCSYM_GenericBinding *pGenericBindingContext
)
{
    if (pAnonymousDelegate)
    {
        // because we do not keep a pointer to the Invoke method, we will have to SimpleBind to it
        BCSYM_NamedRoot *InvokeMethod =
            pAnonymousDelegate->GetHash()->SimpleBind(STRING_CONST(pCompiler, DelegateInvoke));

        if (InvokeMethod && InvokeMethod->IsProc())
        {
            BCSYM_Proc *pProc = InvokeMethod->PProc();

            if (pProc->GetRawType() && !pProc->GetRawType()->IsVoidType())
            {
                pbuf->AppendString(L"<Function");
            }
            else
            {
                pbuf->AppendString(L"<Sub");
            }

            FillInProc(pCompiler, pContextContainer, pProc, pbuf, pGenericBindingContext, TIP_AnonymousDelegate);

            pbuf->AppendChar(L'>');
        }
    }
}


/*****************************************************************************
;GetBasicRep

Generates a text representation of what you would type in VB to get this symbol
The string buffer must be provided by the caller.
*****************************************************************************/
void BCSYM::GetBasicRep
(
    Compiler *CompilerInstance,  // [in] Compiler Context
    BCSYM_Container *ContainerContext, // [in] The container of the symbol we are getting a string rep for.  Used for qualified names, determining default accessiblity, etc. May be NULL.
    StringBuffer *StringRep,     // [in/out] Caller must supply the buffer - we fill it out
    BCSYM_GenericBinding *GenericBindingContext,   // [optional in (default NULL)] The binding context to use to interpret any type parameters.
    _In_opt_z_ STRING *ReplacementName,     // [optional in] If this is a named type symbol, replace the symbols name with this one.
    bool FullExpansion,          // [optional in] True (the default) when requesting the entire type declaration, otherwise false
    TIPKIND Special,             // [optional in] We use this to tweak MyClass,MyBase prefixes and also tweak
                                 // defaul value of Microsoft.Visualbasic.CompareMethod type
    IReadonlyBitVector *pGenericParamsToExclude, // [optional in] If this symbol is generic, which generic params to suppress in the string
    bool QualifyNameInExpansion  // [optional in] Fully qualify names while expanding to full type declaration.
)
{
    SymbolEntryFunction;
    if ( IsIntrinsicType())
    {
        StringRep->AppendSTRING( PNamedRoot()->GetName());
        return;
    }

    switch( GetKind())
    {
        case SYM_Alias:

            if (!ReplacementName)
            {
                ReplacementName = PNamedRoot()->GetName();
            }
            PAlias()->GetSymbol()->GetBasicRep(CompilerInstance, ContainerContext, StringRep, GenericBindingContext, 
                                               ReplacementName, FullExpansion, Special, pGenericParamsToExclude, QualifyNameInExpansion);
            break;

        case SYM_NamedType:
        {
            BCSYM_NamedType *NamedType = PNamedType();
            unsigned NumDelimitedNames = NamedType->GetNameCount();
            STRING **NameArray = NamedType->GetNameArray();
            NameTypeArguments **NamedArguments = NamedType->GetArgumentsArray();

            for ( unsigned i = 0; i < NumDelimitedNames; i++ )
            {
                if (i > 0)
                {
                    StringRep->AppendChar(L'.');
                }

                StringRep->AppendSTRING(NameArray[i]);

                if (NamedArguments && NamedArguments[i])
                {
                    if (NamedArguments[i]->m_ArgumentCount > 0)
                    {
                        StringRep->AppendChar(L'(');
                        StringRep->AppendString(WIDE("Of "));

                        for (unsigned iArgIndex = 0; iArgIndex < NamedArguments[i]->m_ArgumentCount; iArgIndex++)
                        {
                            if (iArgIndex > 0)
                            {
                                StringRep->AppendString(WIDE(", "));
                            }

                            NamedArguments[i]->m_Arguments[iArgIndex]->GetBasicRep(
                                CompilerInstance,
                                ContainerContext,
                                StringRep,
                                NULL,
                                NULL,
                                FullExpansion,
                                Special,
                                pGenericParamsToExclude,
                                QualifyNameInExpansion);
                        }

                        StringRep->AppendChar(L')');
                    }
                }
            }
            break;
        }

        case SYM_PointerType:

            PPointerType()->GetRoot()->GetBasicRep( CompilerInstance, ContainerContext, StringRep, GenericBindingContext, 
                                                    ReplacementName, FullExpansion, Special, pGenericParamsToExclude, QualifyNameInExpansion);
            break;

        case SYM_GenericBadNamedRoot:
        case SYM_TypeForwarder:

            VSASSERT( PNamedRoot()->IsBad(), "how can we not be bad?");

            if ( PNamedRoot()->GetName())
                PNamedRoot()->GetQualifiedName(StringRep);
            else
            {
                ResLoadStringRepl(STRID_BadNamedRoot, StringRep);
            }
            break;

        case SYM_Param:
        case SYM_ParamWithValue:
        {
            // We intentionally don't put ByVal on as it is the default

            if (PParam()->IsOptional())
            {
                StringRep->AppendChar(L'[');
            }

            if (PParam()->IsByRefKeywordUsed())
            {
                StringRep->AppendString( WIDE("ByRef "));
            }

            if (PParam()->IsParamArray())
            {
                StringRep->AppendString( WIDE("ParamArray ")); // to indicate that the final argument is an Optionalarray of Variant elements.
            }

            if (Special == TIP_AggregateOperatorParam)
            {
#if ID_TEST && IDE
                if (g_fRunningIntellisenseTest)
                {
                    StringRep->AppendString(L"<expression> As ");
                }
                else
#endif ID_TEST && IDE
                {
                    ResLoadStringRepl(STRID_Expression, StringRep);
                    StringRep->AppendString(L" As ");
                }

                // Note:  Allocator usage here should be revisited when we generally go through the
                //        function to address NorlsAllocator usages
                NorlsAllocator TempAllocator(NORLSLOC);
                Symbols SymbolFactory(CompilerInstance, &TempAllocator, NULL);

                BCSYM *DelegateType = DigThroughGenericExpressionType(GetParamType(PParam(), GenericBindingContext, &SymbolFactory));

                if (DelegateType && TypeHelpers::IsDelegateType(DelegateType))
                {
                    BCSYM_NamedRoot *InvokeProc = GetInvokeFromDelegate(DelegateType, CompilerInstance);

                    if (InvokeProc &&
                        !InvokeProc->IsBad() &&
                        InvokeProc->IsProc() &&
                        InvokeProc->PProc()->GetType() &&
                        !InvokeProc->PProc()->GetType()->IsVoidType())
                    {
                        InvokeProc->PProc()->GetType()->GetBasicRep(
                            CompilerInstance,
                            ContainerContext,
                            StringRep,
                            DelegateType->IsGenericBinding() ? DelegateType->PGenericBinding() : NULL,
                            NULL,
                            false);
                    }
                }
            }
            else
            {
                if (Special != TIP_AnonymousDelegate && PParam()->GetName())
                {
                    StringRep->AppendSTRING(PParam()->GetName());
                }

                BCSYM *ParamType = PParam()->GetType();

                if (ParamType)
                {
                    // Leave array for the right side if Nullable is present.
                    if (!TypeHelpers::IsNullableTypeWithContantainerContext(ParamType->ChaseToType(), ContainerContext))
                    {
                        ParamType = FillInArray( CompilerInstance, ParamType, StringRep );
                    }

                    if (Special != TIP_AnonymousDelegate)
                    {
                        StringRep->AppendString( WIDE(" As "));
                    }

                    ParamType->GetBasicRep( CompilerInstance, ContainerContext, StringRep, GenericBindingContext, NULL, false );
                }
                else // No type means bad array type here...
                {
                    StringRep->AppendString(L" As <bad array type>");
                }

                if (Special != TIP_AnonymousDelegate && PParam()->IsParamWithValue())
                {
                    StringRep->AppendString( WIDE(" = "));
                    bool fUseText = ParamType && ParamType->IsEnum() ? true : false;
                    if (fUseText && PParam()->IsOptionCompare())
                    {
                        StringBuffer sbCompare;
                        sbCompare.AppendString(WIDE("Microsoft.VisualBasic.CompareMethod."));
                        sbCompare.AppendString(Special == TIP_OptionCompareText ? WIDE("Text") : WIDE("Binary"));
                        StringRep->AppendString(&sbCompare);
                    }
                    else
                    {
                        BCSYM_Expression * pExpr = PParamWithValue()->GetExpression();
                        BCSYM_NamedRoot * pnamedEnum = NULL;
                        //VS#241089, DevDivBugs#20926
                        if (fUseText && pExpr && ParamType->PClass()->GetHash())
                        {
                            ConstantValue value = pExpr->GetValue();
                            if (IsIntegralType(value.TypeCode))
                            {
                                BCITER_CHILD bcIterators;
                                bcIterators.Init(ParamType->PClass());
                                while ((pnamedEnum = bcIterators.GetNext()) != NULL)
                                {
                                    if (pnamedEnum->IsVariableWithValue())
                                    {
                                        BCSYM_Expression * pExprTmp = pnamedEnum->PVariableWithValue()->GetExpression();
                                        ConstantValue valueTmp = pExprTmp->GetValue();
                                        if (valueTmp.TypeCode == value.TypeCode && valueTmp.Integral == value.Integral)
                                        {
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        if (pnamedEnum != NULL)
                        {
                            StringRep->AppendSTRING(ParamType->PClass()->GetName());
                            StringRep->AppendChar(L'.');
                            StringRep->AppendSTRING(pnamedEnum->GetName());
                        }
                        else
                        {
                            StringFromExpression( PParamWithValue()->GetExpression(), StringRep, fUseText );
                        }
                    }
                }
            }

            if (PParam()->IsOptional())
            {
                StringRep->AppendChar(L']');
            }
            break;
        }

        case SYM_StaticLocalBackingField:
        case SYM_Variable:
        case SYM_VariableWithArraySizes:
        case SYM_VariableWithValue:
        {
            // Prints a string of the following format.
            // [Public|Private|...][Shared][Shadows][ReadOnly](Const|Static|(Dim[WithEvents]))<Name>[ As <Type>][ = <Expression>]

            BCSYM_Variable* Variable = PVariable();
            BCSYM_Container* Container = Variable->GetContainer();

            // Enum members don't need any modifiers at all.
            if (!(Variable->IsConstant() && Container && Container->IsEnum()))
            {
                // Append modifiers available on type members only.
                if (!Variable->IsLocal() && !Variable->IsStatic() && !IsStaticLocalBackingField())
                {
                    // Append access modifier.
                    ACCESS Access = Variable->GetAccessConsideringAssemblyContext(ContainerContext);
                    StringRep->AppendSTRING(StringOfAccess(CompilerInstance, Access));
                    StringRep->AppendChar(WIDE(' '));

                    // Append Shared and Shadows.
                    if (Container && Container->IsClass() && !Container->PClass()->IsStdModule())
                    {
                        if (Variable->IsShared() && !Variable->IsConstant())
                        {
                            StringRep->AppendString(WIDE("Shared "));
                        }
                        if (Variable->IsShadowsKeywordUsed() && !Variable->GetContainingProject()->IsMetaData())
                        {
                            StringRep->AppendString(WIDE("Shadows "));
                        }
                    }

                    // Append ReadOnly, but only if it's not also constant (happens with Decimal, DateTime constants).
                    if (Variable->IsReadOnly() && !Variable->IsConstant())
                    {
                        StringRep->AppendString(WIDE("ReadOnly "));
                    }
                }

                // Append "the main" keyword depending on variable kind.
                if (Variable->IsConstant())
                {
                    StringRep->AppendString(WIDE("Const "));
                }
                else if (Variable->IsStatic() || IsStaticLocalBackingField())
                {
                    StringRep->AppendString(WIDE("Static "));
                }
                else
                {
                    if (Variable->IsLocal() && !Variable->IsImplicitDecl())
                    {
                        StringRep->AppendString(WIDE("Dim "));
                    }
                    if (Variable->IsWithEvents())
                    {
                        StringRep->AppendString(WIDE("WithEvents "));
                    }
                }
            }

            // Append variable name (don't print out 'Me' if this is 'MyClass' or 'MyBase).
            switch (Special)
            {
                case TIP_MyBase:
                {
                    StringRep->AppendString(WIDE("MyBase"));
                    break;
                }
                case TIP_MyClass:
                {
                    StringRep->AppendString(WIDE("MyClass"));
                    break;
                }
                default:
                {
                    StringRep->AppendSTRING(Variable->GetName());
                    break;
                }
            }

            // Append variable type, if available and not an implicit Object.
            BCSYM *Type = Variable->GetType();
            if (Type)
            {
                // Leave array for the right side if Nullable is present.
                if (!TypeHelpers::IsNullableTypeWithContantainerContext(Type->ChaseToType(), ContainerContext))
                {
                    Type = FillInArray(CompilerInstance, Type, StringRep);
                }
                if (Variable->IsExplicitlyTyped() || !Type->IsObject())
                {
                    StringRep->AppendString(WIDE(" As "));
                    Type->GetBasicRep(CompilerInstance, ContainerContext, StringRep, GenericBindingContext, NULL, false, Special);
                }
            }

            // NOTE:Microsoft,3/2001,VS#229099, no initial value except for a constant variable.
            if (Variable->IsVariableWithValue() && Variable->IsConstant() && !Variable->IsNew())
            {
                BCSYM_Expression* InitialValue = Variable->PVariableWithValue()->GetExpression();
                if (InitialValue)
                {
                    StringRep->AppendString(WIDE(" = "));
                    StringFromExpression(InitialValue, StringRep);
                }
            }

            break;
        }

        case SYM_EventDecl:
        case SYM_Property:
        case SYM_SyntheticMethod:
        case SYM_MethodDecl:
        case SYM_MethodImpl:
        case SYM_UserDefinedOperator:
        case SYM_LiftedOperatorMethod:
        {
            // There is no symbol for a WithEvents X as Y declaration.  Instead, there is a hidden _X and a hidden Property X to expose it
            // that does the event hookup.  When they hover over a WithEvents X declaration, we are given the Property which the
            // user can't see.  So we need to dig through the property and display the WithEvents variable the user expects.
            BCSYM_Variable *SpoofAsWithEvents = IsProperty() && PProperty()->CreatedByWithEventsDecl() ? PProperty()->CreatedByWithEventsDecl() : NULL;

            // 

            if ( !SpoofAsWithEvents || ( SpoofAsWithEvents && !FullExpansion ))
            {
                if (!ContainerContext || !ContainerContext->IsInterface())
                {
                    ACCESS ContextualAccess =
                        PProc()->GetAccessConsideringAssemblyContext(ContainerContext);

                    StringRep->AppendSTRING(StringOfAccess(CompilerInstance, ContextualAccess));
                    StringRep->AppendChar(WIDE(' '));
                }

                // Procs that live in a standard module are implicitly marked as Shared, but that means that the user should never
                // implicitly mark it as Shared, or even see the Shared specifier in error messages.
                if (PProc()->IsShared() && !(PProc()->GetContainer() &&
                    PProc()->GetContainer()->IsClass() && PProc()->GetContainer()->PClass()->IsStdModule()))
                {
                    // Don't show 'Shared' for an extension call
                    if ((Special != TIP_ExtensionCall && Special != TIP_ExtensionAggregateOperator) ||
                        !PProc()->IsExtensionMethod())
                    {
                        StringRep->AppendString(WIDE( "Shared " ));
                    }
                }

                // Procs that live in a standard module are implicitly marked as Shadows, but that means that the user should never
                // implicitly mark it as Shadows, or even see the Shadows specifier in error messages.
                if (PProc()->IsShadowsKeywordUsed() &&
                    !(PProc()->GetContainer() && PProc()->GetContainer()->IsClass() && PProc()->GetContainer()->PClass()->IsStdModule()) &&
                    !PProc()->GetContainingProject()->IsMetaData())
                {
                    StringRep->AppendString(WIDE( "Shadows " ));
                }

                if (PProc()->IsOverridesKeywordUsed())
                    StringRep->AppendString(WIDE("Overrides "));

                if (PProc()->IsOverridableKeywordUsed())
                    StringRep->AppendString(WIDE("Overridable "));

                if (PProc()->IsNotOverridableKeywordUsed())
                    StringRep->AppendString(WIDE("NotOverridable "));

                if (PProc()->IsMustOverrideKeywordUsed() &&
                    (!ContainerContext || !ContainerContext->IsInterface()))
                {
                    if (PNamedRoot()->GetContainer() && !PNamedRoot()->GetContainer()->IsInterface())
                    {
                        StringRep->AppendString(WIDE("MustOverride "));
                    }
                }

                // VS 475306
                // Suppress overloads keyword for metadata symbols
                if (PProc()->IsOverloadsKeywordUsed() &&
                    // VSW 478217 if we are interpreting under debugger and can't get to the project, supress overload keyword
                    PProc()->GetContainingProject() &&
                    !PProc()->GetContainingProject()->IsMetaData())
                {
                    StringRep->AppendString(WIDE("Overloads "));
                }

                if (PProc()->IsAsyncKeywordUsed())
                {
                    StringRep->AppendString(WIDE("Async "));
                }

                if (PProc()->IsIteratorKeywordUsed())
                {
                    StringRep->AppendString(WIDE("Iterator "));
                }

                if (IsProperty())
                {
                    if (PProperty()->IsReadOnly())
                        StringRep->AppendString(WIDE("ReadOnly "));
                    else if (PProperty()->IsWriteOnly())
                        StringRep->AppendString(WIDE("WriteOnly "));

                    if (PProperty()->IsDefault())
                        StringRep->AppendString(WIDE("Default "));

                    StringRep->AppendString(WIDE("Property "));
                }
                else if (PProc()->IsPropertySet())
                {
                    StringRep->AppendString(WIDE("Property Set "));
                }
                else if (PProc()->IsPropertyGet())
                {
                    StringRep->AppendString(WIDE("Property Get "));
                }
                else if (IsUserDefinedOperator())
                {
                    if (PUserDefinedOperator()->GetOperator() == OperatorWiden)
                    {
                        StringRep->AppendString(WIDE("Widening "));
                    }
                    else if (PUserDefinedOperator()->GetOperator() == OperatorNarrow)
                    {
                        StringRep->AppendString(WIDE("Narrowing "));
                    }
                    StringRep->AppendString(WIDE("Operator "));
                }
                else if (!IsEventDecl())
                {
                    if (!PProc()->GetType())
                        StringRep->AppendString(WIDE("Sub "));
                    else
                        StringRep->AppendString(WIDE("Function "));
                }
                else if (IsEventDecl())
                {
                    if (PEventDecl()->IsBlockEvent())
                    {
                        StringRep->AppendString(WIDE("Custom "));
                    }

                    StringRep->AppendString(WIDE("Event "));
                }

                StringRep->AppendSTRING( PProc()->IsAnyConstructor() ?
                                            CompilerInstance->TokenToString(tkNEW) :
                                            (PProc()->IsPropertySet() || PProc()->IsPropertyGet()) ?
                                                PProc()->GetAssociatedPropertyDef()->GetName() :
                                                PProc()->GetName());
                FillInProc( CompilerInstance, ContainerContext, PProc(), StringRep, GenericBindingContext, Special, pGenericParamsToExclude);
            }
            else
            {
                // We were given a property, but it was a property for a hidden WithEvents variable.  Show the WithEvents var instead.
                SpoofAsWithEvents->GetBasicRep( CompilerInstance, ContainerContext, StringRep, GenericBindingContext, ReplacementName, true, Special, pGenericParamsToExclude, QualifyNameInExpansion);
            }
            break;
        }

        case SYM_DllDeclare:

            StringRep->AppendString(WIDE("Declare "));
            if (PDllDeclare()->GetDeclareType() == DECLARE_Ansi)
            {
                StringRep->AppendString(WIDE("Ansi "));
            }
            else if (PDllDeclare()->GetDeclareType() == DECLARE_Unicode)
            {
                StringRep->AppendString(WIDE("Unicode "));
            }
            else if (PDllDeclare()->GetDeclareType() == DECLARE_Auto)
            {
                StringRep->AppendString(WIDE("Auto "));
            }

            if (!PDllDeclare()->GetType())
            {
                StringRep->AppendString(WIDE("Sub "));
            }
            else
            {
                StringRep->AppendString(WIDE("Function "));
            }
            StringRep->AppendSTRING(PDllDeclare()->GetName());

            if (PDllDeclare()->GetLibName())
            {
                StringRep->AppendString(WIDE(" Lib \""));
                StringRep->AppendSTRING(PDllDeclare()->GetLibName());
                StringRep->AppendString(WIDE("\""));
            }

            if (PDllDeclare()->GetAliasName())
            {
                StringRep->AppendString(WIDE(" Alias \""));
                StringRep->AppendSTRING(PDllDeclare()->GetAliasName());
                StringRep->AppendString(WIDE("\" "));
            }

            FillInProc(CompilerInstance, ContainerContext, PProc(), StringRep, GenericBindingContext, TIP_Normal, pGenericParamsToExclude);
            break;

        case SYM_ArrayLiteralType:
        case SYM_ArrayType:
        {
            BCSYM *pRealType = ChaseToType();

            if (pRealType->IsBad() && Special == TIP_UseNamedTypes)
            {
                pRealType = ChaseToNamedType();
            }

            pRealType->GetBasicRep(CompilerInstance, ContainerContext, StringRep, GenericBindingContext, ReplacementName, FullExpansion, Special, pGenericParamsToExclude, QualifyNameInExpansion);
            FillInArray(CompilerInstance, this, StringRep);
            break;
        }

        case SYM_Implements:

            StringRep->AppendString(WIDE("Implements "));
            StringRep->AppendSTRING(PImplements()->GetName());
            break;

        case SYM_Interface:

            if (FullExpansion)
            {
                StringRep->AppendString(WIDE("Interface "));
                StringRep->AppendSTRING(QualifyNameInExpansion? PInterface()->GetQualifiedName() : PInterface()->GetName());
                FillInGenericParams(CompilerInstance, ContainerContext, this, StringRep, NULL, Special, pGenericParamsToExclude);
            }
            else
            {
                PNamedRoot()->GetQualifiedName(StringRep, true, ContainerContext, true, GenericBindingContext);
            }
            break;

        case SYM_Class:

            if (PClass()->IsObject())
            {
                StringRep->AppendString(WIDE("Object"));
            }
            else if (PContainer()->IsAnonymousType())
            {
                ResLoadStringRepl(STRID_AnonymousType, StringRep);
            }
            else if (PContainer()->IsAnonymousDelegate())
            {
                FillInAnonymousDelegate(CompilerInstance, ContainerContext, PClass(), StringRep, GenericBindingContext);
            }
            else if (FullExpansion)
            {
                if (PClass()->IsDelegate())
                {
                    // because we do not keep a pointer to the Invoke method, we will have to SimpleBind to it
                    BCSYM_NamedRoot *InvokeMethod =
                        PClass()->GetHash()->SimpleBind(STRING_CONST(CompilerInstance, DelegateInvoke));

                    // now put the name of the delegate, followed by the signature of the Invoke
                    StringRep->AppendString(WIDE("Delegate "));

                    if (InvokeMethod && InvokeMethod->IsProc())
                    {
                        StringRep->AppendString(
                            InvokeMethod->PProc()->GetType() ?
                                WIDE("Function ") :
                                WIDE("Sub "));
                        StringRep->AppendSTRING(QualifyNameInExpansion? PClass()->GetQualifiedName() : PClass()->GetName());

                        FillInGenericParams(CompilerInstance, ContainerContext, this, StringRep, GenericBindingContext, TIP_Normal, pGenericParamsToExclude);
                        FillInProc(CompilerInstance, ContainerContext, InvokeMethod->PProc(), StringRep, GenericBindingContext);
                    }
                    else
                    {
                        PNamedRoot()->GetQualifiedName(StringRep, true, ContainerContext);
                    }
                    break;
                }
                else if (PClass()->IsEnum())
                {
                    BCSYM_NamedRoot * pUnderlineType = NULL;
                    StringRep->AppendString(WIDE("Enum "));
                    StringRep->AppendSTRING(QualifyNameInExpansion? PClass()->GetQualifiedName() : PClass()->GetName());
                    if (PClass()->IsIntrinsicType())
                    {
                        pUnderlineType = ContainerContext->GetSourceFile()->GetCompilerHost()->GetFXSymbolProvider()->GetType(PClass()->GetVtype());
                    }
                    else
                    {
                        BCSYM_NamedRoot * pValue = PClass()->SimpleBind(NULL,STRING_CONST(CompilerInstance,EnumValueMember));
                        if (pValue && pValue->IsMember() && pValue->PMember()->GetRawType())
                        {
                            BCSYM * ptyp = pValue->PMember()->GetType();
                            if (ptyp && ptyp->IsNamedRoot())
                            {
                                pUnderlineType = ptyp->PNamedRoot();
                            }
                        }
                    }
                    if (pUnderlineType)
                    {
                        StringRep->AppendString(WIDE(" As "));
                        StringRep->AppendSTRING(QualifyNameInExpansion? pUnderlineType->GetQualifiedName() : pUnderlineType->GetName());
                    }
                    break;
                }
                else if (PClass()->IsStruct())
                {
                    StringRep->AppendString(WIDE("Structure "));
                }
                else if (PClass()->IsStdModule())
                {
                    StringRep->AppendString(WIDE("Module "));
                }
                else
                {
                    StringRep->AppendString(WIDE("Class "));
                }

                StringRep->AppendSTRING(QualifyNameInExpansion? PClass()->GetQualifiedName() : PClass()->GetName());
                FillInGenericParams(CompilerInstance, ContainerContext, this, StringRep, NULL, Special, pGenericParamsToExclude);
            }
            else
            {
                PNamedRoot()->GetQualifiedName(StringRep, true, ContainerContext);
            }
            break;

        case SYM_Namespace:

            if (FullExpansion)
            {
                StringRep->AppendString(WIDE("Namespace "));

                // VSW#161575
                // When there is no name, use Global
                if (StringPool::StringLength(PNamespace()->GetName()) == 0)
                {
                    StringRep->AppendSTRING(CompilerInstance->TokenToString(tkGLOBAL));
                }
                else
                {
                    StringRep->AppendSTRING(QualifyNameInExpansion? PNamespace()->GetQualifiedName() : PNamespace()->GetName());
                }
            }
            else
            {
                PNamedRoot()->GetQualifiedName(StringRep, true, ContainerContext);
            }
            break;

        case SYM_XmlNamespaceDeclaration:

            StringRep->AppendSTRING(PXmlNamespaceDeclaration()->GetName());
            break;

        case SYM_XmlNamespace:

            StringRep->AppendSTRING(PXmlNamespace()->GetName());
            break;

        case SYM_XmlName:
            {
                BCSYM_XmlName *pXmlName = PXmlName();
                STRING *pNamespace  = pXmlName->GetNameSpace();
                if (pNamespace && StringPool::StringLength(pNamespace) != 0)
                {
                    StringRep->AppendChar(L'{');
                    StringRep->AppendSTRING(pNamespace);
                    StringRep->AppendChar(L'}');
                }
                StringRep->AppendSTRING(pXmlName->GetName());
            }
            break;

        case SYM_VoidType:

            break;

        case SYM_CCConstant:
            {
                // A CC constant symbol derives from a variable with value, where the
                // actual information is stored
                BCSYM_VariableWithValue *VarWithValue = PVariableWithValue();

                StringRep->AppendChar('#');
                StringRep->AppendSTRING(CompilerInstance->TokenToString(tkCONST));
                StringRep->AppendChar(' ');
                StringRep->AppendSTRING(VarWithValue->GetName());

                if (VarWithValue->GetExpression())
                {
                    StringRep->AppendString(WIDE(" = "));
                    StringFromExpression(VarWithValue->GetExpression(), StringRep);
                }

                break;
            }

        case SYM_GenericParam:
        {
            if (FullExpansion)
            {
                // "In T As {...}"

                switch (PGenericParam()->GetVariance())
                {
                    case Variance_In:
                        StringRep->AppendString(L"In ");
                        break;
                    case Variance_Out:
                        StringRep->AppendString(L"Out ");
                        break;
                    case Variance_None:
                        break;
                    default:
                        VSFAIL("unexpected variance");
                }

                StringRep->AppendSTRING(PGenericParam()->GetName());

                BCSYM_GenericConstraint *pConstraint = PGenericParam()->GetConstraints();
                long iConstraints = 0;

                while (pConstraint)
                {
                    iConstraints++;
                    pConstraint = pConstraint->Next();
                }

                if (iConstraints > 0)
                {
                    StringRep->AppendString(L" As ");

                    if (iConstraints > 1)
                    {
                        StringRep->AppendChar('{');
                    }

                    pConstraint = PGenericParam()->GetConstraints();
                    long iIndex = 0;

                    while (pConstraint)
                    {
                        if (iIndex > 0)
                        {
                            StringRep->AppendString(L", ");
                        }

                        if (pConstraint->IsGenericTypeConstraint())
                        {
                            pConstraint->PGenericTypeConstraint()->GetType()->
                                GetBasicRep(CompilerInstance, ContainerContext, StringRep, GenericBindingContext, NULL, false, Special);
                        }
                        else if (pConstraint->IsNewConstraint())
                        {
                            StringRep->AppendSTRING(CompilerInstance->TokenToString(tkNEW));
                        }
                        else if (pConstraint->IsReferenceConstraint())
                        {
                            StringRep->AppendSTRING(CompilerInstance->TokenToString(tkCLASS));
                        }
                        else if (pConstraint->IsValueConstraint())
                        {
                            StringRep->AppendSTRING(CompilerInstance->TokenToString(tkSTRUCTURE));
                        }

                        iIndex++;

                        pConstraint = pConstraint->Next();
                    }

                    if (iConstraints > 1)
                    {
                        StringRep->AppendChar('}');
                    }
                }
            }
            else
            {
                BCSYM_GenericParam *GenericParam = PGenericParam();
                BCSYM *GenericArgument = NULL;

                if (GenericBindingContext)
                {
                    GenericArgument = GenericBindingContext->GetCorrespondingArgument(GenericParam);
                }

                if (GenericArgument && GenericArgument != GenericParam)
                {
                    GenericArgument->GetBasicRep(CompilerInstance, ContainerContext, StringRep, NULL, NULL, false, Special);
                }
                else
                {
                    StringRep->AppendSTRING(GenericParam->GetName());
                }
            }

            break;
        }

        case SYM_GenericTypeBinding:
        {
            NorlsAllocator TempAllocator(NORLSLOC);
            Symbols SymbolFactory(CompilerInstance, &TempAllocator, NULL);

            BCSYM_GenericTypeBinding *Binding = ReplaceGenericParametersWithArguments(
                PGenericTypeBinding(),
                GenericBindingContext,
                SymbolFactory)->PGenericTypeBinding();

            VSASSERT(Binding, "Invalid Binding");  // Watson results show we have a null-ref later on GenericType
            BCSYM_NamedRoot *GenericType = Binding->GetGenericType();

            // $


            if (TypeHelpers::IsNullableTypeWithContantainerContext(Binding, ContainerContext) && 
                Binding->GetArgumentCount() == 1)
            {
                Binding->GetArgument(0)->GetBasicRep(CompilerInstance, ContainerContext, StringRep, NULL, NULL, false, Special);
                StringRep->AppendChar(L'?');
            }
            else if (!GenericType)
            {
                VSFAIL("Watson results show we have a null-ref here, but no known repro");
            }
            else if (GenericType->IsAnonymousType())
            {
                ResLoadStringRepl(STRID_AnonymousType, StringRep);
            }
            else if (GenericType->IsAnonymousDelegate())
            {
                FillInAnonymousDelegate(CompilerInstance, ContainerContext, GenericType->PClass(), StringRep, Binding);
            }
            else
            {
                GenericType->GetQualifiedName(StringRep, true, ContainerContext, true, Binding);
            }
            break;
        }

        default:

            VSFAIL("NYI");
    } // switch (kind)
}

// Define common Vb String constants.
// Make sure the numbers match those in the microsoft.visualbasic.dll string constants.
enum
{
    vbNull = 0,
    vbBack = 8,
    vbTab = 9,
    vbLf = 10,
    vbVTab = 11,
    vbFF = 12,
    vbCr = 13
} VbStringConst;

/*****************************************************************************
;IsVbConst
Returns wether or not the given value mathes one of the vb constants.

*****************************************************************************/
//static
bool BCSYM::IsVbConst(
    WCHAR wcChar
)
{
    switch (wcChar)
    {
        case vbNull:
        case vbBack:
        case vbCr:
        case vbLf:
        case vbFF:
        case vbTab:
        case vbVTab:
            return true;
    }

    return false;
}

/*****************************************************************************
;ReplaceStringWithVBConst

Generates a string which replaces special characters with vb contants (vbcr, vbtab, etc).
The string buffer must be provided by the caller.
*****************************************************************************/
//static
void BCSYM::ExpandVbStringConst(
    _In_count_(lLength) const WCHAR *wszString, // [in] the string we want to convert to its visible representation.
    long lLength,   // [in] The string length.
    StringBuffer *StringRep      // [in/out] Caller must supply the buffer - we fill it out
)
{
    VSASSERT(wszString != NULL, "Invalid NULL String");
    if (wszString && lLength > 0)
    {
        long lIndex = 0;
        bool fNotFirst = false;

        while (lIndex < lLength)
        {
            if (fNotFirst)
            {
                StringRep->AppendWithLength(WIDE(" & "), 3);
            }

            switch (wszString[lIndex])
            {
                case vbNull:
                    StringRep->AppendWithLength(WIDE("vbNullChar"), 10);
                    break;

                case vbBack:
                    StringRep->AppendWithLength(WIDE("vbBack"), 6);
                    break;

                case vbCr:
                    if (lIndex + 1 < lLength && wszString[lIndex + 1] == vbLf)
                    {
                        StringRep->AppendWithLength(WIDE("vbCrLf"), 6);
                        lIndex++;
                    }
                    else
                    {
                        StringRep->AppendWithLength(WIDE("vbCr"), 4);
                    }
                    break;

                case vbLf:
                    StringRep->AppendWithLength(WIDE("vbLf"), 4);
                    break;

                case vbFF:
                    StringRep->AppendWithLength(WIDE("vbFormFeed"), 10);
                    break;

                case vbTab:
                    StringRep->AppendWithLength(WIDE("vbTab"), 5);
                    break;

                case vbVTab:
                    StringRep->AppendWithLength(WIDE("vbVerticalTab"), 13);
                    break;


                default:
                    StringRep->AppendChar(WIDE('"'));
                    while (lIndex >= 0 && lIndex < lLength && !IsVbConst(wszString[lIndex]))
                    {
                        StringRep->AppendChar(wszString[lIndex]);
                        lIndex++;
                    }

                    StringRep->AppendChar(WIDE('"'));
                    lIndex--;
            }

            fNotFirst = true;
            lIndex++;
        }
    }
}
/*****************************************************************************
;StringFromExpression

Generates a text representation of an expression symbol
The string buffer must be provided by the caller.
*****************************************************************************/
//static
void BCSYM::StringFromExpression(
    BCSYM_Expression *Expression, // [in] the expression that we want to create a string representation for
    StringBuffer *StringRep,      // [in/out] Caller must supply the buffer - we fill it out
    bool fUseText                 // [in] use string if true no matter of IsEvaluated() = true in enum member case
)
{
#define FirstVisibleChar                  0x0020
#define LastVisibleChar                   0x007E


    WCHAR *ExpressionText = Expression->GetUserDefinedExpressionText();
    bool fEvaluated = Expression->IsEvaluated();
    if (fUseText && ExpressionText && *ExpressionText != WIDE('\0'))
    {
        fEvaluated = false;
    }

    // VSW#137781
    // Show the expression text if the evaluated expression is bad
    if (fEvaluated && Expression->GetValue().TypeCode != t_bad)
    {
        ConstantValue Value = Expression->GetValue();

        if ( Value.TypeCode != t_string )
        {
            WCHAR ValueSpellingScratch[40];
            const WCHAR *ValueSpelling = ValueSpellingScratch;
            size_t ValueSpellingLength = DIM(ValueSpellingScratch);
            BSTR ValueSpellingBstr = NULL;

            switch ( Value.TypeCode )
            {
                case t_bool:
                    ValueSpelling = (Value.Integral) ? L"True" : L"False";
                    break;

                case t_char:
                    if (Value.Integral == 0)
                    {
                        ValueSpelling = L"Nothing";
                    }
                    else if (Value.Integral < FirstVisibleChar || Value.Integral > LastVisibleChar)
                    {
                        if (FAILED(StringCchPrintf(ValueSpellingScratch, ValueSpellingLength, L"ChrW(%u)", (WCHAR)Value.Integral)))
                        {
                            ValueSpellingScratch[0] = UCH_NULL;
                        }
                    }
                    else
                    {
                        ValueSpellingScratch[0] = L'\"';
                        ValueSpellingScratch[1] = (WCHAR)Value.Integral;
                        ValueSpellingScratch[2] = L'\"';
                        ValueSpellingScratch[3] = L'c';
                        ValueSpellingScratch[4] = L'\0';
                    }
                    break;

                case t_i1:
                case t_ui1:
                case t_i2:
                case t_ui2:
                case t_i4:
                    if (FAILED(StringCchPrintf(ValueSpellingScratch, ValueSpellingLength, L"%d", (long)Value.Integral)))
                    {
                        ValueSpellingScratch[0] = UCH_NULL;
                    }
                    break;

                case t_ui4:
                    if (FAILED(StringCchPrintf(ValueSpellingScratch, ValueSpellingLength, L"%u", (unsigned long)Value.Integral)))
                    {
                        ValueSpellingScratch[0] = UCH_NULL;
                    }
                    break;

                case t_i8:
                    _i64tow_s(Value.Integral, ValueSpellingScratch, _countof(ValueSpellingScratch), 10);
                    break;

                case t_ui8:
                    _ui64tow_s((unsigned __int64)Value.Integral, ValueSpellingScratch, _countof(ValueSpellingScratch), 10);
                    break;

                case t_single:
                    IfFailThrow(R4ToBSTR( Value.Single, &ValueSpellingBstr ));
                    ValueSpelling = ValueSpellingBstr;
                    break;

                case t_double:
                    IfFailThrow(R8ToBSTR( Value.Double, &ValueSpellingBstr ));
                    ValueSpelling = ValueSpellingBstr;
                    break;

                case t_ref:
                case t_array:
                    ValueSpelling = L"Nothing";
                    break;

                case t_decimal:
                    IfFailThrow(VarBstrFromDec( &Value.Decimal, LCID_US_ENGLISH, LOCALE_NOUSEROVERRIDE, &ValueSpellingBstr ));
                    ValueSpelling = ValueSpellingBstr;
                    break;

                case t_date:
                    ValueSpellingBstr = BstrFromDateTime(Value.Integral, false);
                    ValueSpelling = ValueSpellingBstr;
                    break;

                default:
                    VSFAIL("StringFromExpression: unknown vtype");
                    ValueSpelling = L"";
                    break;
            } // switch ( Value.TypeCode )

            if (Value.TypeCode == t_date)
            {
                StringRep->AppendChar(WIDE('#'));
            }

            StringRep->AppendString(ValueSpelling);

            if (Value.TypeCode == t_date)
            {
                StringRep->AppendChar(WIDE('#'));
            }

            if (ValueSpellingBstr)
            {
                SysFreeString(ValueSpellingBstr);
            }
        } // if ( Value.TypeCode != t_string )
        else
        {
            if (Value.String.Spelling)
            {
                if (Value.String.LengthInCharacters > 0)
                {
                    ExpandVbStringConst(Value.String.Spelling, Value.String.LengthInCharacters, StringRep);
                }
                else
                {
                    StringRep->AppendWithLength(WIDE("\"\""), 2);
                }
            }
            else
            {
                StringRep->AppendString(L"Nothing");
            }
        }
    }
    else if ( ExpressionText && *ExpressionText != WIDE('\0'))
    {
        // if we have a string, output the string
        StringRep->AppendString( ExpressionText );
    }
}

/*****************************************************************************
;CompareProcs

Returns a bitfield of Equality (see enum Equality in bcsym.h) to describe
the difference between two procs.  EQ_Match if they are the same.
An additional flag instructs this method to ignore overload flag differences
in the case of partial methods.
*****************************************************************************/
//static
unsigned BCSYM::CompareProcs
(
    BCSYM_Proc *Proc1, // [in] Compare this proc
    GenericBindingInfo Binding1,
    BCSYM_Proc *Proc2, // [in] Against this one
    GenericBindingInfo Binding2,
    Symbols *Symbols,
    bool ignoreOverloadDifferences,
    bool ignoreFirstParameterOfProc1,
    bool ignoreFirstParameterOfProc2
)
{
    GenericParamsInferredTypesInfo GenericParamsInferredTypes;

    return
        CompareProcs
        (
            Proc1,
            Binding1,
            Proc2,
            Binding2,
            Symbols,
            &GenericParamsInferredTypes,
            ignoreOverloadDifferences,
            ignoreFirstParameterOfProc1,
            ignoreFirstParameterOfProc2
        );
}

/*****************************************************************************
;CompareProcs

Returns a bitfield of Equality (see enum Equality in bcsym.h) to describe
the difference between two procs.  EQ_Match if they are the same.
An additional flag instructs this method to ignore overload flag differences
in the case of partial methods.
*****************************************************************************/
//static
unsigned BCSYM::CompareProcs
(
    BCSYM_Proc *Proc1, // [in] Compare this proc
    GenericBindingInfo Binding1,
    BCSYM_Proc *Proc2, // [in] Against this one
    GenericBindingInfo Binding2,
    Symbols *Symbols,
    GenericParamsInferredTypesInfo *GenericParamsInferredTypes,
    bool ignoreOverloadDifferences,
    bool ignoreFirstParameterOfProc1,
    bool ignoreFirstParameterOfProc2
)
{
    // For partial methods, we have a special situation, because we can have an overloading
    // relationship between two declarations. Then when the implementation tries to bind,
    // we have a situation where everything matches, except that we have marked the declaration
    // as IsOverrides, and thus, this method returns EQ_Flags. We need to catch this case,
    // and return EQ_Match for it.

    bool FlagsDifferOnlyByOverloads = false;
    bool NonOverloadFlagsExist = false;

    // If the pointers are identical, they refer to the same symbol.
    if (Proc1 == Proc2 && Binding1 == Binding2)
    {
        return EQ_Match;
    }

    //
    // Check the shape of the symbols.
    //
    unsigned ComparisonFlags = EQ_Match;

    // If either is NULL, it's a definite non-match.
    if (!Proc1 && Proc2 || !Proc2 && Proc1)
    {
        return EQ_Shape;
    }

    // Everything matches a bad symbol.
    //
    // Exception: VSWhidbey 553868:
    // Continue comparing metadata procs whose badness is due
    // to bad overloading/shadowing to handle case sensitivity issues,
    // and different overloading rules in other languages.
    //
    if
    (
        (
            Proc1->IsBad() &&
            (
                !Proc1->IsBadMetadataProcOverload() ||
                Bindable::ResolveOverloadingShouldSkipBadMember(Proc1->GetErrid())
            )
        ) ||
        (
            Proc2->IsBad() &&
            (
                !Proc2->IsBadMetadataProcOverload() ||
                Bindable::ResolveOverloadingShouldSkipBadMember(Proc2->GetErrid())
            )
        )
    )
    {
        return ComparisonFlags | EQ_Bad;
    }

    // Check the name.
    if ( !StringPool::IsEqual( Proc1->GetName(), Proc2->GetName()))
    {
        ComparisonFlags |= EQ_Name;
    }

    // Constructor mismatch?
    if ( Proc1->IsSharedConstructor() != Proc2->IsSharedConstructor() ||
         Proc1->IsInstanceConstructor() != Proc2->IsInstanceConstructor())
    {
        return EQ_Shape;
    }

    // Methods with different number of generic type parameters are different
    // enough since they can alway be called explicitly with type arguments
    // and the arity would be enough to disambiguate.
    //
    if ( Proc1->GetGenericParamCount() != Proc2->GetGenericParamCount())
    {
        ComparisonFlags |= EQ_GenericMethodTypeParamCount;
    }

    // Check the flags.
    if ( Proc1->IsShared() != Proc2->IsShared() ||
            Proc1->IsOverrides() != Proc2->IsOverrides() ||
            Proc1->IsMustOverrideKeywordUsed() != Proc2->IsMustOverrideKeywordUsed())
    {
        ComparisonFlags |= EQ_Flags;
        NonOverloadFlagsExist = true;
    }

    if( Proc1->IsOverloads() != Proc2->IsOverloads() )
    {
        ComparisonFlags |= EQ_Flags;
        FlagsDifferOnlyByOverloads = true;
    }

    // Check the invokekind.
    if ( Proc1->IsProperty() != Proc2->IsProperty())
    {
        ComparisonFlags |= EQ_Shape;
    }
    else if ( Proc1->IsProperty() && Proc2->IsProperty())
    {
        BCSYM_Property *Property1 = Proc1->PProperty(), *Property2 = Proc2->PProperty();
        if ( Property1->IsReadOnly()  != Property2->IsReadOnly() ||
                Property1->IsWriteOnly() != Property2->IsWriteOnly())
        {
            ComparisonFlags |= EQ_Property;
        }

        // Check default ( only properties may be marked 'default')
        if ( Property1->IsDefault() != Property2->IsDefault())
        {
            ComparisonFlags |= EQ_Default;
        }
    }

    //
    // Check the parameters and the return value
    //

    unsigned ParamFlags = 0;
    BCSYM_Param *Param1 = Proc1->GetFirstParam(); // first parameter in Proc1
    BCSYM_Param *Param2 = Proc2->GetFirstParam(); // first parameter in Proc2

    if (ignoreFirstParameterOfProc1)
    {
        Assume(Param1, L"ignoreFirstParameterOfProc1 => Proc1->GetFirstParam()");
        Param1 = Param1->GetNext();
    }

    if (ignoreFirstParameterOfProc2)
    {
        Assume(Param2, L"ignoreFirstParameterOfProc2 => Proc2->GetFirstParam()");
        Param2 = Param2->GetNext();
    }


    BCSYM *RetType1 = Proc1->GetType(); // return type for Proc1 or, if this is a property - the property type, or if Event As Delegate, it the Delegate type
    BCSYM *RetType2 = Proc2->GetType(); // return type for Proc2 or, if this is a property - s the property type, or if Event As Delegate, it the Delegate type

    // Funky special rule. When comparing events, if either was declared as a specific
    // delegate type, then the other one has to match that type and we ignore the parameters.
    if ( !Proc1->IsEventDecl() || !Proc2->IsEventDecl() || (!RetType1 && !RetType2))
    {
        while ( TRUE ) // haul through the parameters to see how they compare between the two methods
        {
            if ( !Param1 && !Param2 ) break; // Done comparing parameters - move on to compare return values

            // Compare the two function's versions of this parameter
            ParamFlags = CompareParams( Param1, Binding1, Param2, Binding2, Symbols, GenericParamsInferredTypes );

            if ( ParamFlags & EQ_Shape )
            {
                // Bail out since we discovered a shape difference - Shape differences indicate that we don't want to do further analysis
                return ParamFlags;
            }

            // combine the flags from the parameter comparison with those of the method we are comparing
            ComparisonFlags |= ParamFlags;

            // move on to the next parameter in the list
            if (Param1) Param1 = Param1->GetNext();
            if (Param2) Param2 = Param2->GetNext();
        } // end hauling through the parameters loop

        // Need to compare the Value param for the Setter - #219715 Could import a com thing marked byref and we are implementing byval - need to know they don't match.
        if (Proc1->IsProperty() && Proc1->PProperty()->SetProperty() &&
            Proc2->IsProperty() && Proc2->PProperty()->SetProperty())
        {
            Param1 = Proc1->PProperty()->SetProperty()->GetLastParam();
            Param2 = Proc2->PProperty()->SetProperty()->GetLastParam();

            VSASSERT(Param1 && Param2, "Every Setter must have a Value param");

            ParamFlags = CompareParams( Param1, Binding1, Param2, Binding2, Symbols, GenericParamsInferredTypes );

            // The type of the Value param for the setter is the same the return type of the property.  If there isn't a match, then
            // we should indicate that we have EQ_Return, not EQ_Shape (#256862)
            if (ParamFlags & EQ_Shape)
            {
                // Bail out since we discovered a shape difference - Shape differences indicate that we don't want to do further analysis
                ComparisonFlags |= EQ_Return;
            }
            else if (ParamFlags & EQ_GenericTypeParams)
            {
                ComparisonFlags |= EQ_GenericTypeParamsForReturn;
            }
            else if (ParamFlags & EQ_GenericMethodTypeParams)
            {
                ComparisonFlags |= EQ_GenericMethodTypeParamsForReturn;
            }
            else
            {
                // combine the flags from the parameter comparison with those of the method we are comparing
                ComparisonFlags |= ParamFlags;
            }
        }

    } // Event test


    // ------------ 

    unsigned RetTypeCompareFlags = CompareReferencedTypes( RetType1, Binding1, RetType2, Binding2, Symbols, GenericParamsInferredTypes );

    // Percolate only the type comparison flags that we care about

    ComparisonFlags |= ( RetTypeCompareFlags & ( EQ_Byref | EQ_Bad ));

    if ( RetTypeCompareFlags & EQ_Shape )
    {
        ComparisonFlags |= EQ_Return;
    }
    else if ( RetTypeCompareFlags & EQ_GenericTypeParams)
    {
        ComparisonFlags |= EQ_GenericTypeParamsForReturn;
    }
    else if ( RetTypeCompareFlags & EQ_GenericMethodTypeParams)
    {
        ComparisonFlags |= EQ_GenericMethodTypeParamsForReturn;
    }

    // If the flags only differ by override, and exactly one of the methods (but not both)
    // are partial method declarations, then we let this through.

    // Microsoft: port SP1 CL 2917151 to VS10.

    if( ignoreOverloadDifferences &&
        !NonOverloadFlagsExist &&
        FlagsDifferOnlyByOverloads &&
        ( Proc1->IsPartialMethodDeclaration() || Proc2->IsPartialMethodDeclaration() ) &&
        ( Proc1->IsPartialMethodDeclaration() != Proc2->IsPartialMethodDeclaration() )
      )
    {
        ComparisonFlags &= ~EQ_Flags;
    }

    return ComparisonFlags;
}

/*****************************************************************************
;CompareParams

Returns a bitfield of Equality (see enum Equality in bcsym.h) to describe
the difference between two params.  EQ_Match if they are the same.
*****************************************************************************/
//static
unsigned BCSYM::CompareParams
(
    BCSYM_Param *Param1, // [in] Compare this param
    GenericBindingInfo Binding1,
    BCSYM_Param *Param2, // [in] Against this one
    GenericBindingInfo Binding2,
    Symbols *Symbols
)
{
    GenericParamsInferredTypesInfo GenericParamsInferredTypes;

    return
        CompareParams(
            Param1,
            Binding1,
            Param2,
            Binding2,
            Symbols,
            &GenericParamsInferredTypes);
}

/*****************************************************************************
;CompareParams

Returns a bitfield of Equality (see enum Equality in bcsym.h) to describe
the difference between two params.  EQ_Match if they are the same.
*****************************************************************************/
//static
unsigned BCSYM::CompareParams
(
    BCSYM_Param *Param1, // [in] Compare this param
    GenericBindingInfo Binding1,
    BCSYM_Param *Param2, // [in] Against this one
    GenericBindingInfo Binding2,
    Symbols *Symbols,
    GenericParamsInferredTypesInfo *GenericParamsInferredTypes
)
{
    // If the pointers are identical, they refer to the same symbol.
    if (Param1 == Param2 &&
        (Param1 == NULL || Binding1 == Binding2))
    {
        return EQ_Match;
    }

    //
    // Check the shape of the symbols.
    //
    unsigned ComparisonFlags = EQ_Match;

    // If either is NULL, it's a definite non-match.
    if (Param1 == NULL || Param2 == NULL)
    {
        // Disambiguate whether it is EQ_Shape or more precisely, a difference between optional and nothing
        if (( Param1 && Param1->GetKind() == SYM_ParamWithValue ) ||
            ( Param2 && Param2->GetKind() == SYM_ParamWithValue ))
        {
            return EQ_Optional;
        }
        else
        {
            return EQ_Shape;
        }
    }

    // Everything matches a bad symbol.
    if (Param1->IsBad() || Param2->IsBad())
    {
        return ComparisonFlags | EQ_Bad;
    }

    // FACTOR THE TYPES OF EACH PARAMETER
    unsigned ParmTypeCompareFlags = CompareReferencedTypes( Param1->PParam()->GetType(), Binding1, Param2->PParam()->GetType(), Binding2, Symbols, GenericParamsInferredTypes);
    if ( ParmTypeCompareFlags & EQ_Bad )
    {   // We know the types were not the same bad type ( or we would have got EQ_Match )
        // So we know the types of the params were not the same bad type, thus the shapes of the procs differ.
        ParmTypeCompareFlags &= ~EQ_Bad;
        ParmTypeCompareFlags |= EQ_Shape;
    }

    // PARAMETER NAMES DIFFER?
    if ( !StringPool::IsEqual( Param1->GetName(), Param2->GetName()))
    {
        ComparisonFlags |= EQ_ParamName;
    }

    // PARAMARRAY DIFFERENCES?
    if ( Param1->IsParamArray() != Param2->IsParamArray())
    {
        if ( Param1->GetType()->IsArrayType() || Param2->GetType()->IsArrayType())
        {
            ComparisonFlags |= EQ_ParamArrayVsArray;
        }
        else
        {
            ComparisonFlags |= EQ_Shape;
        }
    }
    // OPTIONAL DIFFERENCES?
    else if ( Param1->IsOptional() != Param2->IsOptional())
    {
        ComparisonFlags |= EQ_Optional;
    }
    // DEFAULT VALUES THE SAME? ( this implies that both were OPTIONAL )
    else if ( Param1->GetKind() == SYM_ParamWithValue && Param2->GetKind() == SYM_ParamWithValue )
    {
        // Don't do this check if either is bad.
        if ( !Param1->PParamWithValue()->GetExpression()->IsBadExpression()
                && !Param2->PParamWithValue()->GetExpression()->IsBadExpression())
        {
            // make sure the types being used for the default are the same
            if ( ParmTypeCompareFlags & ( EQ_Shape | EQ_GenericTypeParams ))
            {
                ComparisonFlags |= EQ_OptionalTypes;

                // don't percolate the difference in the type shape on in this case - the difference we care about it is EQ_OptionalTypes
                ParmTypeCompareFlags &= ~(EQ_Shape | EQ_GenericTypeParams);
            }
            else
            {
                // make sure the values being used for the default are the same - default values are reduced by the time we get here
                ConstantValue ParamValue1 = Param1->PParamWithValue()->GetExpression()->GetValue();
                ConstantValue ParamValue2 = Param2->PParamWithValue()->GetExpression()->GetValue();

                // If either of the params is a generic parameter type, since both the params'
                // substituted types have been determined to be equal, then the default value
                // of the parameter of generic param type should compare equal to the other
                // param's default 0/Nothing value.
                //
                if (Param1->GetType()->IsGenericParam())
                {
                    VSASSERT(ParamValue1.TypeCode == t_ref, "Unexpected default value for optional parameter of generic parameter type!!!");
                    memset(&ParamValue1, 0, sizeof ConstantValue);
                    ParamValue1.TypeCode = ParamValue2.TypeCode;
                }
                else if (Param2->GetType()->IsGenericParam())
                {
                    VSASSERT(ParamValue2.TypeCode == t_ref, "Unexpected default value for optional parameter of generic parameter type!!!");
                    memset(&ParamValue2, 0, sizeof ConstantValue);
                    ParamValue2.TypeCode = ParamValue1.TypeCode;
                }

                if ( !ConstantValue::Equals( ParamValue1, ParamValue2 ))
                {
                    // If the reason we are unequal is because we have two arrays, assume equality ( we factor in the type of the array below )
                    if ( ParamValue1.TypeCode != t_array || ParamValue2.TypeCode != t_array )
                    {
                        ComparisonFlags |= EQ_ParamDefaultVal;
                    }
                }
            }
        }
    }

    return ComparisonFlags | ParmTypeCompareFlags;
}

/*****************************************************************************
;CompareReferencedTypes

Returns a bitfield of Equality (see enum Equality in bcsym.h) to describe
the difference between two types.  EQ_Match if they are the same.
*****************************************************************************/
//static
unsigned
BCSYM::CompareReferencedTypes
(
    BCSYM *Type1,
    GenericBindingInfo Binding1,
    BCSYM *Type2,
    GenericBindingInfo Binding2,
    Symbols *Symbols
)
{
    GenericParamsInferredTypesInfo GenericParamsInferredTypes;

    return
        CompareReferencedTypes(
            Type1,
            Binding1,
            Type2,
            Binding2,
            Symbols,
            &GenericParamsInferredTypes);
}

void BCSYM::BuildReferencedParameterSet
(
    ISet<BCSYM_GenericParam *> * pSet
)
{
    SymbolEntryFunction;
    ThrowIfNull(pSet);

    BCSYM * pType = DigThroughNamedTypeIfPossible(this);
    ThrowIfNull(pType);

    if (pType->IsGenericParam())
    {
        pSet->Add(pType->PGenericParam());
    }
    else if (pType->IsGenericTypeBinding())
    {
        GenericBindingArgumentIterator iterator(pType->PGenericBinding());

        while (iterator.MoveNext())
        {
            iterator.Current()->BuildReferencedParameterSet(pSet);
        }
    }
    else if (pType->IsPointerType())
    {
        TypeHelpers::GetReferencedType(pType->PPointerType())->BuildReferencedParameterSet(pSet);
    }
    else if (pType->IsArrayType())
    {
        TypeHelpers::GetElementType(pType->PArrayType())->BuildReferencedParameterSet(pSet);
    }

}


/*****************************************************************************
;CompareReferencedTypes

Returns a bitfield of Equality (see enum Equality in bcsym.h) to describe
the difference between two types.  EQ_Match if they are the same.
*****************************************************************************/
//static
unsigned
BCSYM::CompareReferencedTypes
(
    BCSYM *Type1,
    GenericBindingInfo Binding1,
    BCSYM *Type2,
    GenericBindingInfo Binding2,
    Symbols *Symbols,
    GenericParamsInferredTypesInfo *GenericParamsInferredTypes
)
{
    // It is perhaps surprising that comparing two types can cause the creation of
    // symbol table nodes.
    //
    // If a type is a generic binding, and that binding includes a generic parameter
    // as an argument, then the type used for the comparison is a (potentially new)
    // generic binding that substitutes an argument from the (separately supplied)
    // binding.
    //
    // For example, consider:
    //
    //    Interface I(Of T)
    //        Sub Fred(P As C(Of T))
    //    End Interface
    //
    //    Delegate Sub Barney(P As C(Of Integer))
    //
    //    Sub Wilma()
    //        Dim V As I(Of Integer)
    //        Dim D As Barney = AddressOf V.Fred
    //    End Sub
    //
    // In interpreting the delegate binding, the compiler will compare the signatures of
    // Fred and Barney, which will involve comparing the types:
    //     -- From Fred's parameter, C(Of T) using the binding I(Of Integer)
    //     -- From Barney's parameter, C(Of Integer) using a null binding
    // Binding C(Of T) using I(Of Integer) as a binding produces C(Of Integer).

    Type1 = ReplaceGenericParametersWithArguments(Type1, Binding1, *Symbols);
    Type2 = ReplaceGenericParametersWithArguments(Type2, Binding2, *Symbols);

    // If the pointers are identical, they refer to the same symbol.
    if (Type1 == Type2)
    {
        return EQ_Match;
    }

    unsigned ComparisonFlags = EQ_Match;

    // Check the shape of the symbols.
    while ( TRUE )
    {
        if (!Type1 && Type2 || !Type2 && Type1)
        {
            return EQ_Shape;
        }

        if (!Type1)
        {
            break; // No more symbols, we're done.
        }

        Type1 = Type1->DigThroughAlias();
        Type2 = Type2->DigThroughAlias();

        // Everything matches a bad symbol.
        if (Type1->IsBad() || Type2->IsBad())
        {
            return ComparisonFlags | EQ_Bad;
        }

        BilKind KindOfType1 = Type1->GetKind();
        BilKind KindOfType2 = Type2->GetKind();

        // BYVAL / BYREF -----------------------------------------

        if ( KindOfType1 != KindOfType2 )
        {

            // ByVal vs. ByRef?
            if (KindOfType1 == SYM_PointerType && KindOfType2 != SYM_PointerType)
            {
                ComparisonFlags |= EQ_Byref;
                Type1 = Type1->PPointerType()->GetRoot();
                KindOfType1 = Type1->GetKind();
            }
            if (KindOfType2 == SYM_PointerType && KindOfType1 != SYM_PointerType)
            {
                ComparisonFlags |= EQ_Byref;
                Type2 = Type2->PPointerType()->GetRoot();
                KindOfType2 = Type2->GetKind();
            }

            if (KindOfType1 == SYM_GenericParam)
            {
                return ComparisonFlags |
                        CompareGenericParamAndType(
                            Type1->PGenericParam(),
                            Binding1,
                            Type2,
                            Binding2,
                            Symbols,
                            GenericParamsInferredTypes);
            }
            else if (KindOfType2 == SYM_GenericParam)
            {
                return ComparisonFlags |
                        CompareGenericParamAndType(
                            Type2->PGenericParam(),
                            Binding2,
                            Type1,
                            Binding1,
                            Symbols,
                            GenericParamsInferredTypes);
            }

            if (KindOfType1 != KindOfType2)
            {
                return EQ_Shape;
            }
        }

        // Compare child symbols.
        switch( KindOfType1 )
        {
            // POINTERS -----------------------------------------

            case SYM_PointerType:

                Type1 = Type1->PPointerType()->GetRoot();
                Type2 = Type2->PPointerType()->GetRoot();
                continue;

            // ARRAYS -----------------------------------------

            case SYM_ArrayLiteralType:
            case SYM_ArrayType:

                if (Type1->PArrayType()->GetRank() != Type2->PArrayType()->GetRank())
                {
                    return EQ_Shape;
                }
                Type1 = Type1->PArrayType()->GetRoot();
                Type2 = Type2->PArrayType()->GetRoot();
                continue;

            // CONTAINERS -----------------------------------------

            case SYM_Class:
            case SYM_Interface:

                if (!BCSYM::AreTypesEqual(Type1, Type2))
                {
                    return EQ_Shape;
                }

                return ComparisonFlags;


            // GENERICS -------------------------------------------

            case SYM_GenericParam:

                return ComparisonFlags |
                        CompareGenericParamAndType(
                            Type1->PGenericParam(),
                            Binding1,
                            Type2,
                            Binding2,
                            Symbols,
                            GenericParamsInferredTypes);

            case SYM_GenericTypeBinding:
            {
                // 

                BCSYM_GenericTypeBinding *TypeBinding1 = Type1->PGenericTypeBinding();
                BCSYM_GenericTypeBinding *TypeBinding2 = Type2->PGenericTypeBinding();

                do
                {
                    unsigned TempFlagsGenericType =
                        CompareReferencedTypes(
                            TypeBinding1->GetGenericType(),
                            (GenericBinding *)NULL,
                            TypeBinding2->GetGenericType(),
                            (GenericBinding *)NULL,
                            Symbols,
                            GenericParamsInferredTypes);

                    if (TempFlagsGenericType & EQ_Shape)
                    {
                        return EQ_Shape;
                    }
                    else if (TempFlagsGenericType & EQ_Bad)
                    {
                        return ComparisonFlags | TempFlagsGenericType;
                    }
                    else
                    {
                        ComparisonFlags = ComparisonFlags | TempFlagsGenericType;
                    }

                    if (TypeBinding1->GetArgumentCount() != TypeBinding2->GetArgumentCount())
                    {
                        return EQ_Shape;
                    }

                    for (unsigned ArgumentIndex = 0; ArgumentIndex < TypeBinding1->GetArgumentCount(); ArgumentIndex++)
                    {
                        unsigned TempFlags =
                            CompareReferencedTypes(
                                TypeBinding1->GetArgument(ArgumentIndex),
                                (GenericBinding *)NULL,     // Types in TypeBinding1 have already been inferred from the Binding1, so no context any more
                                TypeBinding2->GetArgument(ArgumentIndex),
                                (GenericBinding *)NULL,     // Types in TypeBinding2 have already been inferred from the Binding1, so no context any more
                                Symbols,
                                GenericParamsInferredTypes);

                        if (TempFlags & EQ_Shape)
                        {
                            return EQ_Shape;
                        }
                        else if (TempFlags & EQ_Bad)
                        {
                            return ComparisonFlags | TempFlags;
                        }
                        else
                        {
                            ComparisonFlags = ComparisonFlags | TempFlags;
                        }
                    }

                    TypeBinding1 = TypeBinding1->GetParentBinding();
                    TypeBinding2 = TypeBinding2->GetParentBinding();

                } while (TypeBinding1 && TypeBinding2);

                if (TypeBinding1 != NULL ||
                    TypeBinding2 != NULL)
                {
                    return EQ_Shape;
                }

                return ComparisonFlags;
            }

            // JUNK -----------------------------------------

            case SYM_NamedType:
            case SYM_Alias:

                VSFAIL("We should never see these.");
                return ComparisonFlags;

            default:

                VSFAIL("Unknown type or unimplemented compare.");
                return ComparisonFlags;
        } // switch ( KindOfType1 )
    } // loop

    return ComparisonFlags;
}

//static
unsigned
BCSYM::CompareGenericParamAndType
(
    BCSYM_GenericParam *Type1,
    GenericBindingInfo Binding1,
    BCSYM *Type2,
    GenericBindingInfo Binding2,
    Symbols *Symbols,
    GenericParamsInferredTypesInfo *GenericParamsInferredTypes
)
{
    if (Type1 == Type2)
    {
        return EQ_Match;
    }

    // Special treatment for Generic Method's Type Parameters
    //
    // They are equal iff their ordinal numbers match, and they don't
    // match any other types. In order to be consistent, this routine
    // compares them the same way they need to be compared when comparing
    // generic methods.
    //
    if (Type1->IsGenericMethodParam())
    {
        if (Type2->IsGenericParam() &&
            Type2->PGenericParam()->IsGenericMethodParam() &&
            Type2->PGenericParam()->GetPosition() == Type1->GetPosition())
        {
            return EQ_GenericMethodTypeParams;
        }

        return EQ_Shape;
    }
    else if (Type2->IsGenericParam() &&
             Type2->PGenericParam()->IsGenericMethodParam())
    {
        return EQ_Shape;
    }


    // Compare Generic type's type parameters
    //
    // The algorithm that checks for the possible unification of types tracks the following
    // equivalence relationships and checks for violations of these relationships:
    //
    // U <-> V              eg: U <-> V
    // U <-> SolidType      eg: U <-> Int
    // U <-> f(V)           eg: U <-> Cls1(Of V)
    // f(U) <-> g(V)        eg: U() <-> V()()
    //
    // U <-> f(U) is impossible
    //


    BCSYM_GenericParam *LastDependentOnGenericParam1;

    BCSYM *BoundType1 =
        GetInferredTypeForGenericParam(
            GenericParamsInferredTypes,
            Type1->PGenericParam(),
            LastDependentOnGenericParam1);

    VSASSERT(LastDependentOnGenericParam1, "Non NULL DependentOn expected!!!");

    BCSYM *BoundType2;
    BCSYM_GenericParam *LastDependentOnGenericParam2 = NULL;

    if (Type2->IsGenericParam())
    {
        BoundType2 =
            GetInferredTypeForGenericParam(
                GenericParamsInferredTypes,
                Type2->PGenericParam(),
                LastDependentOnGenericParam2);

        VSASSERT(LastDependentOnGenericParam2, "Non NULL DependentOn expected!!!");
    }
    else
    {
        BoundType2 = Type2;
    }

    if (BoundType1)
    {
        if (!BoundType2)
        {
            VSASSERT(LastDependentOnGenericParam2, "Unexpected NULL!!!");

            if (InferTypeForGenericParam(
                    BoundType1,
                    LastDependentOnGenericParam2,
                    GenericParamsInferredTypes))
            {
                return EQ_GenericTypeParams;
            }

            return EQ_Shape;
        }
    }
    else if (BoundType2)    // && !BoundType1
    {
        VSASSERT(LastDependentOnGenericParam1, "Unexpected NULL!!!");

        if (InferTypeForGenericParam(
                BoundType2,
                LastDependentOnGenericParam1,
                GenericParamsInferredTypes))
        {
            return EQ_GenericTypeParams;
        }

        return EQ_Shape;
    }
    else // (!BoundType1 && !BoundType2)
    {
        VSASSERT(LastDependentOnGenericParam1 && LastDependentOnGenericParam2,
                    "Unexpected NULLs!!!");

        if (LastDependentOnGenericParam1 !=
            LastDependentOnGenericParam2)
        {
            if (!InferTypeForGenericParam(
                    LastDependentOnGenericParam2,
                    LastDependentOnGenericParam1,
                    GenericParamsInferredTypes))
            {
                VSFAIL("Inference failure in compare types unexpected!!!");
            }
        }

        return EQ_GenericTypeParams;
    }

    VSASSERT(BoundType1 && BoundType2, "Bound types lost ?");

    return EQ_GenericTypeParams |
           BCSYM::CompareReferencedTypes(
                        BoundType1,
                        Binding1,
                        BoundType2,
                        Binding2,
                        Symbols,
                        GenericParamsInferredTypes);
}

//static
BCSYM *
BCSYM::GetInferredTypeForGenericParam
(
    GenericParamsInferredTypesInfo *GenericParamsInferredTypes,
    BCSYM_GenericParam *GenericParam,
    BCSYM_GenericParam *&LastDependentOnGenericParam
)
{
    LastDependentOnGenericParam = GenericParam;

    BCSYM *DependentOn;
    for(DependentOn = GenericParamsInferredTypes->FindTypeInferredForGenericParam(GenericParam);
        DependentOn && DependentOn->IsGenericParam();
        DependentOn =
            GenericParamsInferredTypes->FindTypeInferredForGenericParam(DependentOn->PGenericParam()))
    {
        LastDependentOnGenericParam = DependentOn->PGenericParam();
    }

    return DependentOn;
}

//static
bool
BCSYM::InferTypeForGenericParam
(
    BCSYM *Type,
    BCSYM_GenericParam *GenericParam,
    GenericParamsInferredTypesInfo *GenericParamsInferredTypes
)
{
    VSASSERT(!GenericParamsInferredTypes->FindTypeInferredForGenericParam(GenericParam),
                    "Generic Param type cannot be inferred multiple times!!!");

    VSASSERT(Type != GenericParam,
                   "A Generic Param cannot be inferred from itself!!!");

    if (Type->IsGenericParam() ||
        !DoesTypeDependOnGenericParam(Type, GenericParam, GenericParamsInferredTypes))
    {
        GenericParamsInferredTypes->AddTypeInferredForGenericParam(GenericParam, Type);
        return true;
    }

    // Inference failed
    //
    return false;
}

//static
bool
BCSYM::DoesTypeDependOnGenericParam
(
    BCSYM *Type,
    BCSYM_GenericParam *GenericParam,
    GenericParamsInferredTypesInfo *GenericParamsInferredTypes
)
{
    BCSYM_GenericParam *Buffer[16];
    unsigned BufferSize = sizeof(Buffer) / sizeof(Buffer[0]); 

    BCSYM_GenericParam **DependentOnGenericParams = Buffer;
    unsigned NumberOfGenericParams = 0;

    if (!RefersToGenericParameter(Type, NULL, NULL, DependentOnGenericParams, BufferSize, &NumberOfGenericParams))
    {
        return false;
    }

    if (NumberOfGenericParams > BufferSize)
    {
        if (!VBMath::TryMultiply(sizeof(BCSYM_GenericParam *), NumberOfGenericParams))
        {
            return false; // Overflow, definitely something weird 
                          // going on -- just return gracefully.
        }

        DependentOnGenericParams =
            (BCSYM_GenericParam **)GenericParamsInferredTypes->GetAllocator()->Alloc(sizeof(BCSYM_GenericParam *) * NumberOfGenericParams);
        BufferSize = NumberOfGenericParams;
        NumberOfGenericParams = 0;

        RefersToGenericParameter(Type, NULL, NULL, DependentOnGenericParams, BufferSize, &NumberOfGenericParams);
    }

    VSASSERT(NumberOfGenericParams > 0, "generic parameter dependency search lost!!!");

    for(unsigned Index = 0; Index < NumberOfGenericParams; Index++)
    {
        BCSYM_GenericParam *DependentOnGenericParam = NULL;

        BCSYM *InferredType =
            GetInferredTypeForGenericParam(
                GenericParamsInferredTypes,
                DependentOnGenericParams[Index],
                DependentOnGenericParam);

        if (DependentOnGenericParam == GenericParam)
        {
            return true;
        }

        if (InferredType &&
            DoesTypeDependOnGenericParam(
                InferredType,
                GenericParam,
                GenericParamsInferredTypes))
        {
            return true;
        }
    }

    return false;
}

#if IDE 
// If the ImmediateParent already exist then just return it
// If not then and we're under the expression evaluator, see if
// we can build up the parent.
BCSYM_NamedRoot *BCSYM_NamedRoot::GetImmediateParentOrBuild()
{
    SymbolEntryFunction;
    // Return the parent
    return GetImmediateParent();

}
#endif IDE 


// Returns true if pnamedMaybeContainer is an ancestor of this.
bool BCSYM_NamedRoot::IsContainedBy(BCSYM_NamedRoot *pnamedMaybeContainer)
{
    SymbolEntryFunction;
    VSASSERT( !IsPartialTypeAndHasMainType(),
                    "Partial type unexpected!!! Should never even have bound to a partial type!!!");

    BCSYM_NamedRoot *pnamed;

    // Visit each ancestor.
    // If one of them is pnamedMaybeContainer, return true.
    for (pnamed = GetParent(); pnamed != NULL; pnamed = pnamed->GetParent())
    {
        if (pnamed == pnamedMaybeContainer)
        {
            return true;
        }
    }

    return false;
}


// Get the next symbol in the overload list.
// This is strictly for use by the binder to do overload resolution.
// If anyone else uses this function for any reason, it's your fault
// when I change this function and your code breaks.  So there.
BCSYM_NamedRoot *BCSYM_NamedRoot::GetNextOverload()
{
    SymbolEntryFunction;
    VSASSERT( !IsPartialTypeAndHasMainType(),
                    "Partial type unexpected!!! Should never even have bound to a partial type!!!");

    BCSYM_NamedRoot *pnamedNext;
    for(pnamedNext = GetNextOfSameName();
        pnamedNext;
        pnamedNext = pnamedNext->GetNextOfSameName())
    {
        if (pnamedNext->GetBindingSpace() != BINDSPACE_IgnoreSymbol)
        {
            break;
        }
    }

    return pnamedNext;
}

// This is similar to GetNextOverload(), but it doesn't discriminate
// based on binding space.  I needed a quick way to wind through a hash
// looking for things of the same name, but not necessairly the same symbol type.
BCSYM_NamedRoot *BCSYM_NamedRoot::GetNextOfSameName()
{
    SymbolEntryFunction;
    BCSYM_NamedRoot *Next;
    for(Next = GetNextInHash();
        Next;
        Next = Next->GetNextInHash())
    {
        if (StringPool::IsEqual(Next->GetName(), GetName()) &&
            //
            // Ignore partial types marked as IgnoreSymbol, nobody should ever bind to these
            //
            (!Next->IsContainer() ||
                !Next->PContainer()->IsPartialType() ||
                Next->GetBindingSpace() != BINDSPACE_IgnoreSymbol))
        {
            return Next;
        }
    }

    // Note: don't use GetContainer here, because we don't want this to
    // work this way for Locals hashes, etc.
    //
    if (GetImmediateParent() &&
        GetImmediateParent()->IsHash())
    {
        for(BCSYM_Hash *NextHash = GetImmediateParent()->PHash()->GetNextHash();
            NextHash;
            NextHash = NextHash->GetNextHash())
        {
            Next =
                NextHash->SimpleBind(GetName(), SIMPLEBIND_IgnoreOtherPartialTypes /* Ignore other partial types' hashes */);

            if (Next)
            {
                return Next;
            }
        }
    }

    return NULL;
}

BCSYM_NamedRoot *BCSYM_NamedRoot::GetNextOfSameName(unsigned BindspaceMask)
{
    SymbolEntryFunction;
    VSASSERT( !IsPartialTypeAndHasMainType(),
                    "Partial type unexpected!!! Should never even have bound to a partial type!!!");

    BCSYM_NamedRoot *pnamedNext;
    for(pnamedNext = GetNextOfSameName();
        pnamedNext;
        pnamedNext = pnamedNext->GetNextOfSameName())
    {
        if (pnamedNext->GetBindingSpace() & BindspaceMask)
        {
            break;
        }
    }

    return pnamedNext;
}

// Get the class that contains this symbol.
// If the symbol is a class, return it (not its containing class).
BCSYM_Class *BCSYM_NamedRoot::GetContainingClass()
{
    SymbolEntryFunction;
    BCSYM_NamedRoot *Candidate = this;

    while (Candidate && !Candidate->IsClass())
    {
        Candidate = Candidate->GetImmediateParentOrBuild();
    }

    return Candidate ? Candidate->PClass() : NULL;
}

// Get the namespace containing this symbol.
BCSYM_Namespace *BCSYM_NamedRoot::GetContainingNamespace()
{
    SymbolEntryFunction;
    BCSYM_NamedRoot *pnamed;
    for(pnamed = this;
        pnamed && !pnamed->IsNamespace();
        pnamed = pnamed->GetImmediateParent())
        ;

    return pnamed ? pnamed->PNamespace() : NULL;
}

// Get the project that contains this symbol.
CompilerProject *BCSYM_NamedRoot::GetContainingProject()
{
    SymbolEntryFunction;
    if (IsContainer() && PContainer()->GetCompilerFile())
    {
        return PContainer()->GetCompilerFile()->GetProject();
    }

    BCSYM_Container *Container = GetContainer();
    if (Container && Container->GetCompilerFile())
    {
        return Container->GetCompilerFile()->GetProject();
    }

    return NULL;
}

// Get the class or interface that contains this symbol.
BCSYM_Container *BCSYM_NamedRoot::GetContainingClassOrInterface()
{
    SymbolEntryFunction;
    if (this == NULL)
    {
        return NULL;
    }

    BCSYM_NamedRoot *pnamed;

    for (pnamed = this ? GetImmediateParent() : NULL;
         pnamed && !pnamed->IsClass() && !pnamed->IsInterface();
         pnamed = pnamed->GetImmediateParentOrBuild())
    {
    }

    return pnamed ? pnamed->PContainer() : NULL;
}

// Gets the containing compiler file (basic or metadata).
//
CompilerFile *BCSYM_NamedRoot::GetContainingCompilerFile()
{
    SymbolEntryFunction;
    BCSYM_Container *pContainer;

    if (IsContainer())
    {
        pContainer = this->PContainer();
    }
    else
    {
        pContainer = GetPhysicalContainer();
    }

    return pContainer ? pContainer->GetCompilerFile() : NULL;
}

// Gets the containing source file if this is a basic
// thing.
//
SourceFile *BCSYM_NamedRoot::GetContainingSourceFileIfBasic()
{
    SymbolEntryFunction;
    CompilerFile *pFile;

    pFile = GetContainingCompilerFile();

    if (pFile && pFile->IsSourceFile())
    {
        return pFile->PSourceFile();
    }
    else
    {
        return NULL;
    }

}

void GetDocCommentSignatureForType(_In_ BCSYM *pType, _In_ Compiler *pCompiler, _In_ StringBuffer *pSignature, _In_opt_ bool fUseUntransformedName = false);

void GetDocCommentSignatureForGenericTypeBinding
(
    _In_ BCSYM_GenericTypeBinding *pGenericTypeBinding,
    _In_ Compiler *pCompiler,
    _In_ StringBuffer *pSignature,
    _In_opt_ bool fUseUntransformedName = false
)
{
    STRING *pstrCurrentName = NULL;

    for (BCSYM_NamedRoot *pNamedCurrent = pGenericTypeBinding->GetGenericType();
         pNamedCurrent;
         pNamedCurrent = pNamedCurrent->GetParent())
    {
        STRING *pstrSymbolName;

        if (pNamedCurrent->IsBad() && !pNamedCurrent->GetName())
        {
            pstrSymbolName = STRING_CONST(pCompiler, BadName);
        }
        else
        {
            if (pNamedCurrent->IsGeneric())
            {
                if (fUseUntransformedName && pNamedCurrent->GetUntransformedName())
                {
                    pSignature->AppendSTRING(pNamedCurrent->GetUntransformedName());
                    return;
                }

                StringBuffer GenericName;
                GenericName.AppendString( pNamedCurrent->GetName());
                GenericName.AppendChar(L'{');

                for (BCSYM_GenericParam *pParam = pNamedCurrent->GetFirstGenericParam();
                     pParam;
                     pParam = pParam->GetNextParam())
                {
                    GetDocCommentSignatureForType(
                        pGenericTypeBinding->GetCorrespondingArgument(pParam) ,
                        pCompiler,
                        &GenericName,
                        fUseUntransformedName);

                    if (pParam->GetNextParam())
                    {
                        GenericName.AppendChar(L',');
                    }
                }

                GenericName.AppendChar(L'}');

                pstrSymbolName = pCompiler->AddStringWithLen(GenericName.GetString(), GenericName.GetStringLength());
            }
            else
            {
                pstrSymbolName =
                    pNamedCurrent->GetEmittedName() ?
                    pNamedCurrent->GetEmittedName() :
                    pNamedCurrent->GetName();
            }
        }

        if (pstrSymbolName && StringPool::StringLength(pstrSymbolName) > 0)   // The root namespace has no name
        {
            pstrCurrentName =
                pstrCurrentName ?
                    pCompiler->ConcatStrings(pstrSymbolName, WIDE("."), pstrCurrentName) :
                    pstrSymbolName;
        }
    }

    if (pstrCurrentName)
    {
        pSignature->AppendSTRING(pstrCurrentName);
    }
}

//
//  Get the Signature for the Type needed to get the XML Doc
//
void GetDocCommentSignatureForType
(
    _In_ BCSYM *pType,
    _In_ Compiler *pCompiler,
    _In_ StringBuffer *pSignature,
    _In_opt_ bool fUseUntransformedName /* = false */
)
{
    Vtypes  vtype = t_bad;
    if (pType)
    {
        if (pType->IsEnum())
            vtype = t_struct;
        else
            vtype = pType->GetVtype();
    }

    switch (vtype)
    {

    case t_void:
        pSignature->AppendString(L"=VOID");
        break;

    case t_bool:
    case t_i1:
    case t_ui1:
    case t_i2:
    case t_ui2:
    case t_i4:
    case t_ui4:
    case t_i8:
    case t_ui8:
    case t_decimal:
    case t_single:
    case t_double:
    case t_date:
    case t_char:
    case t_string:
        pSignature->AppendString(g_wszFullCLRNameOfVtype[vtype]);
        break;

    case t_ref:
    case t_struct:
    case c_class:
    case c_object:
    case c_enum:
    case c_array:
    case c_typehandle:
    case c_exception:
        if (pType->IsGenericTypeBinding())
        {
            GetDocCommentSignatureForGenericTypeBinding(pType->PGenericTypeBinding(), pCompiler, pSignature, fUseUntransformedName);
        }
        else
        {
            pSignature->AppendSTRING(pType->PNamedRoot()->GetQualifiedEmittedName(/* pContextContainer */ NULL, fUseUntransformedName));
        }
        break;

    case t_ptr:
        {
            BCSYM * pBase;
            pBase = pType->PPointerType()->GetRoot();
            GetDocCommentSignatureForType(pBase, pCompiler, pSignature, fUseUntransformedName);
            pSignature->AppendChar(L'@');
            break;
        }

    case t_generic:
        WCHAR wcBuf[21];

        if (FAILED(StringCchPrintfW(wcBuf, DIM(wcBuf), L"%d", pType->PGenericParam()->GetMetaDataPosition())))
        {
            VSFAIL("StringCchPrintfW failed!");

            wcBuf[0] = L'0';
            wcBuf[1] = UCH_NULL;
        }

        pSignature->AppendChar(GenericTypeNameManglingChar);

        if (pType->PGenericParam()->IsGenericMethodParam())
        {
            pSignature->AppendChar(GenericTypeNameManglingChar);
        }

        pSignature->AppendString(wcBuf);
        break;

    case t_array:
        {
            BCSYM_ArrayType * pfixedrank = pType->PArrayType();
            unsigned cDims = pfixedrank->GetRank();

            if (cDims == 1)
            {
                GetDocCommentSignatureForType(pfixedrank->GetRoot(), pCompiler, pSignature, fUseUntransformedName);
                pSignature->AppendString(L"[]");
            }
            else
            {
                GetDocCommentSignatureForType(pfixedrank->GetRoot(), pCompiler, pSignature, fUseUntransformedName);
                pSignature->AppendChar(L'[');
                while (cDims--)
                {
                    pSignature->AppendString(L"0:");
                    if (cDims)
                    {
                        pSignature->AppendChar(L',');
                    }
                }
                pSignature->AppendChar(L']');
            }
        }

        break;

    case t_UNDEF:
    case t_bad:
    default:
        //VS#293884, type is defined in un-referenced/un-imported lib
        pSignature->AppendString(L"=VOID");
        break;
    }
}

// Returns the XMLDoc signature for a proc paratms.
void BCSYM_Proc::GetXMLDocCommentParamSignature
(
    Compiler *pCompiler,
    StringBuffer *pSignature
)
{
    SymbolEntryFunction;
    BCITER_Parameters biParams(this);
    BCSYM_Param *pNextParameter;
    pNextParameter = biParams.PparamNext();

    // Use the untransformeed parameter type names if this symbol came from a winmd.
    bool fUseUntransformedName = 
        this->GetContainingCompilerFile() && 
        this->GetContainingCompilerFile()->IsMetaDataFile() &&
        this->GetContainingCompilerFile()->PMetaDataFile()->GetWinMDImport();

    if (pNextParameter)
    {
        pSignature->AppendChar(L'(');

        while (pNextParameter)
        {
            GetDocCommentSignatureForType(pNextParameter->GetCompilerType(), pCompiler, pSignature, fUseUntransformedName);
            pNextParameter = biParams.PparamNext();
            if (pNextParameter)
            {
                pSignature->AppendChar(L',');
            }
        }

        pSignature->AppendChar(L')');
    }
}

//
//  Get the XML Signature for the Method needed to get the XML Doc
//
void BCSYM_Proc::GetDocCommentSignature
(
    Compiler *pCompiler,
    _In_opt_z_ STRING *pstrName,
    BCSYM_NamedRoot *pParent,
    StringBuffer *pSignature
)
{
    SymbolEntryFunction;
    BCSYM_Container *pParentContainer = GetParent()->PContainer();
    pSignature->AppendString(IsProperty() ? L"P:" : L"M:");

    if (IsAnyConstructor())
    {
        if (!pParent)
        {
            pSignature->AppendSTRING(pParentContainer->GetQualifiedEmittedName());
        }
        else
        {
            pSignature->AppendSTRING(pParent->GetQualifiedEmittedName());
        }

        if (IsInstanceConstructor())
        {
            pSignature->AppendString(L".#ctor");
        }
        else
        {
            pSignature->AppendString(L".#cctor");
        }
    }
    else
    {
        if (pstrName)
        {
            pSignature->AppendSTRING(pstrName);
        }
        else
        {
            if (pParent)
            {
                pSignature->AppendSTRING(pParent->GetQualifiedEmittedName());
            }
            else
            {
                pSignature->AppendSTRING(pParentContainer->GetQualifiedEmittedName());
            }

            pSignature->AppendChar(L'.');

            if (IsUserDefinedOperator())
            {
                pSignature->AppendSTRING(GetEmittedName());
            }
            else
            {
                pSignature->AppendSTRING(GetName());
            }
        }
    }

    GetDocCommentGenericSignature(pSignature);
    GetXMLDocCommentParamSignature(pCompiler, pSignature);

    if (IsConversionOperator(this))
    {
        pSignature->AppendChar(L'~');
        GetDocCommentSignatureForType(GetType(), pCompiler, pSignature);
    }
}

void BCSYM_Proc::GetDocCommentGenericSignature(StringBuffer *pBuffer)
{
    SymbolEntryFunction;
    if (IsGeneric())
    {
        WCHAR wcBuf[21];

        if (FAILED(StringCchPrintfW(wcBuf, DIM(wcBuf), L"%d", GetGenericParamCount())))
        {
            VSFAIL("StringCchPrintfW failed!");

            wcBuf[0] = L'0';
            wcBuf[1] = UCH_NULL;
        }

        pBuffer->AppendMultiCopiesOfAWChar(GenericTypeNameManglingChar, 2);
        pBuffer->AppendString(wcBuf);
    }
}

bool BCSYM_NamedRoot::IsLocal()
{
    SymbolEntryFunction;
    VSASSERT( !IsPartialTypeAndHasMainType(),
                "Partial type unexpected!!!");

    if (IsVariable())
    {
        if (this->PVariable()->IsLambdaMember() && 
            this->PVariable()->GetVarkind() == VAR_Local)
        {
            // This is a local within a lambda so return immediately, no need
            // to look through the parents of the symbol.
            return true;
        }

        BCSYM_NamedRoot *Parent = GetParent();

        if (Parent)
        {
            return Parent->IsProc();
        }
    }

    return false;
}

//
//  Gets the signature that's needed to get the XML Doc
//
STRING *BCSYM_NamedRoot::GetDocCommentSignature
(
    Compiler *pCompiler,
    _In_opt_z_ STRING *pstrEventHandlerName,   // Specified if we want the signature for the method that would handle this event symbol
    BCSYM_NamedRoot *pParent        // Overrides the parent symbol's fully qualified name in the doc comment
)
{
    SymbolEntryFunction;
    StringBuffer DocCommentSignature;

    if (IsProc() && (!IsEventDecl() || pstrEventHandlerName))
    {
        PProc()->GetDocCommentSignature(pCompiler, pstrEventHandlerName, pParent, &DocCommentSignature);
    }
    else
    {
        if (IsEventDecl())
        {
            PEventDecl()->GetDocCommentSignature(pCompiler, pParent, &DocCommentSignature);
        }
        else
        {
            if (IsNamespace())
            {
                DocCommentSignature.AppendString(L"N:");
            }
            else if (IsProperty())
            {
                DocCommentSignature.AppendString(L"P:");
            }
            else if (IsClass() || IsInterface()) // Class, Struct, Enum, Delegate. or Interface
            {
                DocCommentSignature.AppendString(L"T:");
            }
            else if (IsVariable()) // Field
            {
                DocCommentSignature.AppendString(L"F:");
            }
            else
            {
                VSFAIL("Invalid state");
            }

            if (pParent)
            {
                DocCommentSignature.AppendSTRING(pParent->GetQualifiedEmittedName());
                DocCommentSignature.AppendChar(L'.');
                DocCommentSignature.AppendSTRING(GetName());
            }
            else
            {
                DocCommentSignature.AppendSTRING(GetQualifiedEmittedName());
            }
        }
    }

#if DEBUG
    if (VSFSWITCH(fDumpDocCommentString))
    {
        DebPrintf("%S\n", DocCommentSignature.GetString());
    }
#endif

    return pCompiler->AddString(&DocCommentSignature);
}

// ==================================
// returns string for type
// ==================================
STRING *BCSYM_NamedRoot::GetIDEName(Compiler *pCompiler)
{
    SymbolEntryFunction;
    if (IsProc())
    {
        if (PProc()->IsAnyConstructor())
        {
            return pCompiler->TokenToString(tkNEW);
        }
        else if (PProc()->IsUserDefinedOperatorMethod())
        {
            StringBuffer sbTemp;

            sbTemp.AppendSTRING(pCompiler->TokenToString(tkOPERATOR));
            sbTemp.AppendChar(L' ');
            sbTemp.AppendSTRING(PProc()->GetAssociatedOperatorDef()->GetName());

            return pCompiler->AddString(&sbTemp);
        }
    }

    return GetName();
}

BCSYM_Variable *
BCSYM_NamedRoot::CreatedByWithEventsDecl() const 
{
    SymbolEntryFunction;
    if ( m_memberThatCreatedSymbol != NULL &&
         m_memberThatCreatedSymbol->IsVariable() )
    {
        return (m_memberThatCreatedSymbol->PVariable());
    }

    return NULL;
}

BCSYM_Property *
BCSYM_NamedRoot::GetAutoPropertyThatCreatedSymbol()
{
    SymbolEntryFunction;
    if (m_memberThatCreatedSymbol != NULL )
    {
        VSASSERT( m_memberThatCreatedSymbol->IsProperty() && 
                  m_memberThatCreatedSymbol->PProperty()->IsAutoProperty(), 
                  "The member that created this auto property backing field should be an auto property");

        return m_memberThatCreatedSymbol->PProperty();
    }

    return NULL;
}

//static
void
BCSYM_NamedRoot::AppendQualifiedTypeName
(
    StringBuffer &Name,
    BCSYM *pType,
    bool fMakeSafeName,
    BCSYM_Container *pContextContainer,
    bool fUseCLRName,
    _In_opt_count_(cParameterNames) STRING **pGenericMethodParameterNames,
    size_t cParameterNames,
    bool fMakeTypeArgumentsGlobal,
    bool fExpandNullable,
    bool fForceQualificationOfIntrinsicTypes,
    bool fMinimallyQualify
)
{
    if (pType->IsArrayType())
    {
        AppendQualifiedTypeName(
            Name,
            pType->ChaseToType(),
            fMakeSafeName,
            pContextContainer,
            fUseCLRName,
            pGenericMethodParameterNames,
            cParameterNames,
            fMakeTypeArgumentsGlobal,
            fExpandNullable,
            fForceQualificationOfIntrinsicTypes,
            fMinimallyQualify);

        FillInArray(NULL, pType, &Name);
    }

    else if (pType->IsGenericParam())
    {
        STRING *strName = pType->PNamedRoot()->GetName();

        if (pGenericMethodParameterNames && pType->PGenericParam()->IsGenericMethodParam())
        {
            unsigned iPosition = pType->PGenericParam()->GetPosition();

            if (iPosition < cParameterNames)
            {
                strName = pGenericMethodParameterNames[iPosition];
            }
        }

        if (fMakeSafeName)
        {
            Compiler *pCompiler = pType->PNamedRoot()->GetCompiler();

            VSASSERT(pCompiler, "NULL compiler unexpected!!!");

            Name.AppendSTRING(MakeSafeName(pCompiler, strName, false));
        }
        else
        {
            Name.AppendSTRING(strName);
        }
    }
    // "Global.Integer", for example, is not valid.
    else if (pType->IsIntrinsicType() && fMakeTypeArgumentsGlobal && !fUseCLRName)
    {
        Name.AppendSTRING(pType->PNamedRoot()->GetName());
    }

    else if (pType->IsGenericBinding())
    {
        // Generic bindings need special handling because PNamedRoot() chases through
        // a binding to the bound generic.

        if (fMakeTypeArgumentsGlobal)
        {
            Name.AppendSTRING(pType->PGenericBinding()->GetGlobalQualifiedName());
        }
        else
        {
            pType->PGenericBinding()->GetGeneric()->GetQualifiedNameHelper(
                    fMakeSafeName,
                    pContextContainer,
                    fUseCLRName,
                    true,
                    pType->PGenericBinding(),
                    pGenericMethodParameterNames,
                    cParameterNames,
                    fMakeTypeArgumentsGlobal,
                    fExpandNullable,
                    fForceQualificationOfIntrinsicTypes,
                    NULL,
                    &Name,
                    fMinimallyQualify);
        }
    }

    else
    {
        if (fMakeTypeArgumentsGlobal)
        {
            Name.AppendSTRING(pType->PNamedRoot()->GetGlobalQualifiedName());
        }
        else
        {
            pType->PNamedRoot()->GetQualifiedNameHelper(
                    fMakeSafeName,
                    pContextContainer,
                    fUseCLRName,
                    true,
                    NULL,
                    pGenericMethodParameterNames,
                    cParameterNames,
                    fMakeTypeArgumentsGlobal,
                    fExpandNullable,
                    fForceQualificationOfIntrinsicTypes,
                    NULL,
                    &Name,
                    fMinimallyQualify);
        }
    }
}

void BCSYM_NamedRoot::GetQualifiedNameHelper
(
    bool fMakeSafeName,                 // should we bother with adding []?
    BCSYM_Container *pContextContainer, // Context we're in (If NULL, return fully qualified name)
    bool fUseCLRName,                   // Use the CLR Name
    bool fAppendGenericArguments,       // Add generic arguments to generics
    BCSYM_GenericBinding *pGenericBindingContext,   // Provide a binding for generic arguments
    _In_opt_count_(cParameterNames) STRING **pGenericMethodParameterNames, // Provide a way to replace open method bindings.
    size_t cParameterNames,
    bool fMakeTypeArgumentsGlobal,      // Prepend "Global." to types used as generic arguments
    bool fExpandNullable,               // Expand Nullable types as System.Nullable(Of ...)
    bool fForceQualificationOfIntrinsicTypes,        // forces intrinsict types to be qualified...
    _Deref_opt_out_opt_z_ STRING **ppstr,                     // One of ppstr and psb should be non-NULL
    StringBuffer *psb,
    bool fMinimallyQualify,             // Strip off unnecessary qualifications
    bool fUseUntransformedName          // For WinRT types, use the untransformed name
)
{
    SymbolEntryFunction;
    AssertIfFalse((ppstr && !psb) || (!ppstr && psb));

    // BCSYM_GenericBinding really shouldn't be derived from BCSYM_NamedRoot, and so getting here
    // with a generic binding isn't sound.

    VSASSERT(!IsGenericBinding(), "Cannot get the qualified name of a generic binding directly.");

    bool fUseCache = ppstr && pGenericBindingContext == NULL && !fMakeTypeArgumentsGlobal && !fMinimallyQualify && !fUseUntransformedName;

    // Can we use the cached value?
    if (pContextContainer           ==  GetCachedContainer() &&
        fMakeSafeName               ==  IsCachedSafeName() &&
        fUseCLRName                 ==  IsCachedCLRName() &&
        fForceQualificationOfIntrinsicTypes      ==  IsCachedQualifyIntrinsicTypes() &&
        GetCachedQualifiedName()   != NULL &&
        fUseCache &&
        (fAppendGenericArguments == IsCachedAppendGenericArguments() || (!IsGeneric() && !HasGenericParent())))
    {
        *ppstr = GetCachedQualifiedName();
        return;
    }

    // Bad named roots could come here and we might not be able to determine their compiler
    // instance, etc., so the special handling. An example is:
    //      A type from an unreferenced assembly is used in a referenced assembly.
    //
    if ((IsGenericBadNamedRoot() && !GetParent()) || IsGenericParam())
    {
        if (ppstr)
        {
            *ppstr = GetName();
        }
        else
        {
            psb->AppendSTRING(GetName());
        }
        return;
    }

    Compiler *pCompiler = GetCompiler();

    if (fUseCache)
    {
        // Let's cache the conditions under which the last value was calculated,
        // so that we know if we need to recalculate the hash.
        SetCachedContainer(pContextContainer);
        SetCachedSafeName(fMakeSafeName);
        SetCachedCLRName(fUseCLRName);
        SetCachedAppendGenericArguments(fAppendGenericArguments);
        SetCachedQualifyIntrinsicTypes(fForceQualificationOfIntrinsicTypes);
    }

    BCSYM_NamedRoot *pNamedCurrent;
    bool fAfterDot;
    bool fIsIntrinsicType = IsIntrinsicType();

    if (fIsIntrinsicType && !fUseCLRName && !fForceQualificationOfIntrinsicTypes)
    {
        if (ppstr)
        {
            *ppstr = GetName();

            if (fUseCache)
            {
                SetCachedQualifiedName(*ppstr);
            }
        }
        else
        {
            psb->AppendSTRING(GetName());
        }

        return;
    }

    if (IsClass() && PClass()->IsObject() && !fUseCLRName && !fForceQualificationOfIntrinsicTypes)
    {
        if (ppstr)
        {
            *ppstr = pCompiler->TokenToString(tkOBJECT);

            if (fUseCache)
            {
                SetCachedQualifiedName(*ppstr);
            }
        }
        else
        {
            psb->AppendSTRING(pCompiler->TokenToString(tkOBJECT));
        }

        return;
    }

    StringBuffer sbCurrentName;

    if (IsAnonymousType() && !fUseCLRName)
    {
        if (ppstr)
        {
            ResLoadStringRepl(STRID_AnonymousType, &sbCurrentName);
            *ppstr = pCompiler->AddString(&sbCurrentName);

            if (fUseCache)
            {
                SetCachedQualifiedName(*ppstr);
            }
        }
        else
        {
            ResLoadStringRepl(STRID_AnonymousType, psb);
        }

        return;
    }

    if (IsAnonymousDelegate() && !fUseCLRName)
    {
        if (ppstr)
        {
            ResLoadStringRepl(STRID_AnonymousDelegate, &sbCurrentName);
            *ppstr = pCompiler->AddString(&sbCurrentName);

            if (fUseCache)
            {
                SetCachedQualifiedName(*ppstr);
            }
        }
        else
        {
            ResLoadStringRepl(STRID_AnonymousDelegate, psb);
        }

        return;
    }

    if (!fUseCLRName && !fExpandNullable && fAppendGenericArguments && IsGeneric() && pGenericBindingContext)
    {
        CompilerFile *pFile = GetCompilerFile();
        if (pFile && pFile->GetCompilerHost() && TypeHelpers::IsSystemNullableClass(this, pFile->GetCompilerHost()))
        {
            BCSYM_GenericParam *pParam = GetFirstGenericParam();
            AssertIfFalse(pParam && pParam->GetNextParam() == NULL); // Nullable(Of T) <- single param
            if (pParam)
            {
                BCSYM *pArgument = pGenericBindingContext->GetCorrespondingArgument(pParam);
                if (pArgument)
                {
                    AppendQualifiedTypeName(
                        sbCurrentName,
                        pArgument,
                        fMakeSafeName,
                        pContextContainer,
                        fUseCLRName,
                        pGenericMethodParameterNames,
                        cParameterNames,
                        fMakeTypeArgumentsGlobal,
                        fExpandNullable,
                        fForceQualificationOfIntrinsicTypes,
                        fMinimallyQualify);

                    sbCurrentName.AppendChar(L'?');

                    if (ppstr)
                    {
                        *ppstr = pCompiler->AddString(&sbCurrentName);
                        if (fUseCache)
                        {
                            SetCachedQualifiedName(*ppstr);
                        }
                    }
                    else
                    {
                        psb->AppendString(&sbCurrentName);
                    }

                    return;
                }
            }
        }
    }

    if (fUseUntransformedName && GetUntransformedName())
    {
        sbCurrentName.AppendSTRING(GetUntransformedName());
    }
    else
    {
        fAfterDot = true;
        for (pNamedCurrent = this; pNamedCurrent && fAfterDot; pNamedCurrent = pNamedCurrent->GetParent())
        {
            if (pContextContainer && pNamedCurrent->IsContainer() && pNamedCurrent->GetParent())
            {
                BCSYM_Container *CurrentContainer = pContextContainer;

                // Bug: VSWhidbey 111014
                // For entities nested in generics, qualify all the way upto the top most enclosing
                // generic irrespective of the ContextContainer. See bug 111014 for more details on
                // why this is needed.

                if (pGenericBindingContext && pNamedCurrent->HasGenericParent())
                {
                    fAfterDot = true;
                }
                else
                {
                    while (CurrentContainer && !CurrentContainer->IsSameContainer(pNamedCurrent->GetParent()))
                    {
                        CurrentContainer = CurrentContainer->GetContainer();
                    }

                    if (CurrentContainer)
                    {
                        // Ok, so we found out that we have reached a common base container, so
                        // we can stop here.
                        fAfterDot = false;
                    }
                    else
                    {
                        fAfterDot = true;
                    }
                }
            }
            else
            {
            
                // MQ Bug 885280 "Wrong AutoComplete while using key word as a project name"
           
                // When a keyword is a project name, AutoComplete does not wrap it with "[" "]". 
                // If a keyword after a dot, then "[" "]" are not need. But there is always a root namespace(UnnamedNamespace) 
                // for each srouce file and project. The logic in GetQualifiedNameHelper does not know this. 
    
                // fAfterDot should be false, if pNamedCurrent is a direct child from root namespace. 
                // The added ThrowIfFalse ensures that root namespace should have no name. 

                BCSYM_NamedRoot *pNamedCurrentParent = pNamedCurrent->GetParent();
            
                VSASSERT( 
                    ! (pNamedCurrentParent != NULL &&
                       pNamedCurrentParent->IsNamespace() &&
                       pNamedCurrentParent->GetParent() == NULL)
                    ||
                       StringPool::IsEqual(pNamedCurrentParent->GetName(), STRING_CONST(GetCompiler(), EmptyString)),
                    "Expect root namespace here.");
                
                fAfterDot = pNamedCurrentParent && (!pNamedCurrentParent->IsNamespace() || pNamedCurrentParent->GetParent());
            }

            STRING *pstrSymbolName;

            if ( pNamedCurrent->IsBad() && !pNamedCurrent->GetName())
            {
                pstrSymbolName = STRING_CONST(pCompiler, BadName);
            }
            else
            {
                pstrSymbolName = ((fUseCLRName || fIsIntrinsicType) && pNamedCurrent->GetEmittedName()) ?
                                    pNamedCurrent->GetEmittedName() :
                                    (pNamedCurrent->IsProc() && pNamedCurrent->PProc()->IsAnyConstructor()) ?
                                        STRING_CONST( pCompiler, New ) :
                                        pNamedCurrent->GetName();

                if (pNamedCurrent->IsGeneric() && fAppendGenericArguments)
                {
                    // The qualified name occurs in a context that requires a binding of the generic.
                    // The only argument values available are the parameters of the generic, so the
                    // binding is, effectively, open. It so happens that this is correct for the
                    // cases where this occurs--specifically, in the generation of synthetic code for
                    // members of generic classes. However, this seems weak.

                    STRING *pstrSafeSymbolName =
                        fMakeSafeName ?
                        MakeSafeName(pCompiler, pstrSymbolName, fAfterDot) :
                        pstrSymbolName;

                    StringBuffer GenericName;
                    GenericName.AppendString(pstrSafeSymbolName);

                    GenericName.AppendString(L"(Of ");

                    for (BCSYM_GenericParam *pParam = pNamedCurrent->GetFirstGenericParam(); pParam; pParam = pParam->GetNextParam())
                    {
                        // If there is no binding then simulate an open binding and substitute
                        // the parameter as an argument.

                        BCSYM *pArgument =
                            pGenericBindingContext ?
                                pGenericBindingContext->GetCorrespondingArgument(pParam) :
                                pParam;

                        AppendQualifiedTypeName(
                            GenericName,
                            pArgument,
                            fMakeSafeName,
                            pContextContainer,
                            fUseCLRName,
                            pGenericMethodParameterNames,
                            cParameterNames,
                            fMakeTypeArgumentsGlobal,
                            fExpandNullable,
                            fForceQualificationOfIntrinsicTypes,
                            fMinimallyQualify);

                        if (pParam->GetNextParam())
                        {
                            GenericName.AppendString(L", ");
                        }
                    }

                    GenericName.AppendChar(L')');

                    pstrSymbolName = pCompiler->AddStringWithLen(GenericName.GetString(), GenericName.GetStringLength());
                }
            }

            if (pstrSymbolName && StringPool::StringLength(pstrSymbolName))   // The root namespace has no name
            {
                STRING *pstrSafeSymbolName =
                    fMakeSafeName ?
                        MakeSafeName(pCompiler, pstrSymbolName, fAfterDot) :
                        pstrSymbolName;

                if (sbCurrentName.GetStringLength() > 0)
                {
                    sbCurrentName.InsertString(0, L".");
                    sbCurrentName.InsertString(0, pstrSafeSymbolName);
                }
                else
                {
                    sbCurrentName.AppendSTRING(pstrSafeSymbolName);
                }
            }
        }
    }

#if IDE
    if (fMinimallyQualify && pContextContainer)
    {
        VSASSERT(pContextContainer->GetSourceFile(), "missing source file");
        CComBSTR qualifiedName = sbCurrentName.GetString();
        Location invalidLocation;
        invalidLocation.Invalidate();
        unsigned cTypeArity = GetGenericParamCount();

        STRING* minimallyQualifiedName = IDEHelpers::GetMinimallyQualifiedName(
            pContextContainer->GetSourceFile(), 
            qualifiedName, 
            pContextContainer && pContextContainer->GetLocation() != NULL 
                ? *(pContextContainer->GetLocation()) 
                : invalidLocation,
            cTypeArity);
        sbCurrentName.Clear();
        sbCurrentName.AppendSTRING(minimallyQualifiedName);
    }
#endif IDE

    if (ppstr)
    {
        *ppstr = pCompiler->AddString(&sbCurrentName);

        if (fUseCache)
        {
            SetCachedQualifiedName(*ppstr);
        }
    }
    else
    {
        psb->AppendString(&sbCurrentName);
    }

    return;
}

// Gets a name that is guaranteed to bind to this symbol from the context class.
STRING *BCSYM_NamedRoot::GetQualifiedName
(
    bool fMakeSafeName,                 // [Optional(true)] should we bother with adding []?
    BCSYM_Container *pContextContainer, // [Optional(NULL)] Context we're in (If NULL, return fully qualified name)
    bool fAppendGenericArguments,       // [Optional(false)] Add generic arguments to generics
    BCSYM_GenericBinding *pGenericBindingContext,   // [Optional(NULL)] Provide a binding for generic arguments
    bool fMinimallyQualify,             // [Optional(false)] strip off unnecessary qualifications
    _In_opt_count_(cParameterNames) STRING **pGenericMethodParameterNames, // [Optional(NULL)] Provide a way to replace open method bindings.
    size_t cParameterNames,
    bool fMakeTypeArgumentsGlobal,      // [Optional(false)] Prepend "Global." to types used as generic arguments
    bool fExpandNullable,               // [Optional(false)] Expand nullable types as "Nullable(Of <arg>)".
    bool fForceQualificationOfIntrinsicTypes         // [Optional(false)] Alwasy generate the fully qualified form of intrinsic types.
)
{
    SymbolEntryFunction;
    const bool fUseCLRName = false;

    STRING *pstr = NULL;

    GetQualifiedNameHelper(
        fMakeSafeName,
        pContextContainer,
        fUseCLRName,
        fAppendGenericArguments,
        pGenericBindingContext,
        pGenericMethodParameterNames,
        cParameterNames,
        fMakeTypeArgumentsGlobal,
        fExpandNullable,
        fForceQualificationOfIntrinsicTypes,
        &pstr,
        NULL,
        fMinimallyQualify);

    return pstr;
}

// Gets a name that is guaranteed to bind to this symbol from the context class.
void BCSYM_NamedRoot::GetQualifiedName
(
    StringBuffer *psb,
    bool fMakeSafeName,                 // [Optional(true)] should we bother with adding []?
    BCSYM_Container *pContextContainer, // [Optional(NULL)] Context we're in (If NULL, return fully qualified name)
    bool fAppendGenericArguments,       // [Optional(false)] Add generic arguments to generics
    BCSYM_GenericBinding *pGenericBindingContext,   // [Optional(NULL)] Provide a binding for generic arguments
    _In_opt_count_(cParameterNames) STRING **pGenericMethodParameterNames, // [Optional(NULL)] Provide a way to replace open method bindings.
    size_t cParameterNames,
    bool fMakeTypeArgumentsGlobal       // [Optional(false)] Prepend "Global." to types used as generic arguments
)
{
    SymbolEntryFunction;
    const bool fUseCLRName = false;

    GetQualifiedNameHelper(
        fMakeSafeName,
        pContextContainer,
        fUseCLRName,
        fAppendGenericArguments,
        pGenericBindingContext,
        pGenericMethodParameterNames,
        cParameterNames,
        fMakeTypeArgumentsGlobal,
        false, // fExpandNullable
        false, //do not force qualification of intrinsic types...
        NULL,
        psb,
        false); // fMinimallyQualify
}


STRING *BCSYM_NamedRoot::GetQualifiedEmittedName
(
    BCSYM_Container *pContextContainer, // [Optional(NULL)] Context we're in (If NULL, return fully qualified name)
    bool fUseUntransformedName
)
{
    SymbolEntryFunction;
    // Emitted names should not have any VB semantics, hence MakeSafeName should be false
    const bool fMakeSafeName = false;

    // Use CLR name gets the emitted name
    const bool fUseCLRName = true;

    // Global is a VB concept, so should not be part of any emitted name
    const bool fMakeTypeArgumentsGlobal = false;
    
    const bool fAppendGenericArguments = false;

    const bool fMinimallyQualify = false;
    BCSYM_GenericBinding *pGenericBindingContext = NULL;

    STRING *pstr = NULL;

    GetQualifiedNameHelper(
        fMakeSafeName,
        pContextContainer,
        fUseCLRName,
        fAppendGenericArguments,
        pGenericBindingContext,
        NULL,
        0,
        fMakeTypeArgumentsGlobal,
        true, // fExpandNullable
        false, //do not force qualification of intrinsic types...
        &pstr,
        NULL,
        fMinimallyQualify,
        fUseUntransformedName);

    return pstr;
}

void BCSYM_NamedRoot::GetQualifiedEmittedName
(
    StringBuffer *psb,
    BCSYM_Container *pContextContainer // [Optional(NULL)] Context we're in (If NULL, return fully qualified name)
)
{
    SymbolEntryFunction;
    // Emitted names should not have any VB semantics, hence MakeSafeName should be false
    const bool fMakeSafeName = false;

    // Use CLR name gets the emitted name
    const bool fUseCLRName = true;

    // Global is a VB concept, so should not be part of any emitted name
    const bool fMakeTypeArgumentsGlobal = false;

    const bool fAppendGenericArguments = false;

    const bool fMinimallyQualify = false;
    BCSYM_GenericBinding *pGenericBindingContext = NULL;

    GetQualifiedNameHelper(
        fMakeSafeName,
        pContextContainer,
        fUseCLRName,
        fAppendGenericArguments,
        pGenericBindingContext,
        NULL,
        0,
        fMakeTypeArgumentsGlobal,
        true, // fExpandNullable
        false, //do not force qualification of intrinsic types...
        NULL,
        psb,
        fMinimallyQualify);
}

#if IDE 
STRING *BCSYM_NamedRoot::GetSimpleName
(
    bool fAppendGenericArguments,
    BCSYM_GenericBinding *pGenericBinding,
    bool fMakeSafeName,
    bool fAfterDot,
    IReadonlyBitVector *pGenericParamsToExclude
)
{
    SymbolEntryFunction;
    STRING *strSimpleName = fMakeSafeName ?
        MakeSafeName(GetCompiler(), GetName(), fAfterDot) :
        GetName();

    if (fAppendGenericArguments && IsGeneric())
    {
        NorlsAllocator nrlsAllocTemp(NORLSLOC);
        BCSYM *pRealType = this;

        if (IsType() && pGenericBinding)
        {
            Symbols TempSymbols(GetCompiler(), &nrlsAllocTemp, NULL);

            pRealType = ReplaceGenericParametersWithArguments(
                this,
                pGenericBinding,
                TempSymbols);
        }

        StringBuffer sbNameWithArguments;

        sbNameWithArguments.AppendSTRING(strSimpleName);

        BCSYM_GenericParam *pGenericParam = pRealType->GetFirstGenericParam();
        bool fOutputtedFirstParam = false;
        unsigned long ulIndex = 0;

        while (pGenericParam)
        {
            if (!pGenericParamsToExclude || !pGenericParamsToExclude->BitValue(ulIndex))
            {
                if (fOutputtedFirstParam)
                {
                    sbNameWithArguments.AppendSTRING(GetCompiler()->TokenToString(tkComma));
                    sbNameWithArguments.AppendChar(L' ');
                }
                else
                {
                    sbNameWithArguments.AppendSTRING(TokenToString(tkLParen));
                    sbNameWithArguments.AppendSTRING(TokenToString(tkOF));
                    sbNameWithArguments.AppendChar(L' ');

                    fOutputtedFirstParam = true;
                }

                pGenericParam->GetBasicRep(
                    GetCompiler(),
                    NULL,
                    &sbNameWithArguments,
                    pGenericBinding,
                    NULL,
                    false);
            }

            ulIndex++;
            pGenericParam = pGenericParam->GetNextParam();
        }

        if (fOutputtedFirstParam)
        {
            sbNameWithArguments.AppendSTRING(GetCompiler()->TokenToString(tkRParen));
        }

        strSimpleName = GetCompiler()->AddStringWithLen(
            sbNameWithArguments.GetString(),
            sbNameWithArguments.GetStringLength());
    }

    return strSimpleName;
}


STRING *BCSYM_NamedRoot::GetDisplayName
(
    bool fAppendGenericArguments,
    BCSYM_GenericBinding *pGenericBinding,
    bool fMakeSafeName,
    bool fAfterDot,
    bool fQualified
)
{
    SymbolEntryFunction;
    StringBuffer sbDisplayName;

    if (!fQualified)
    {
        if (IsProc() && PProc()->IsAnyConstructor())
        {
            return GetCompiler()->TokenToString(tkNEW);
        }
        else
        {
            return GetSimpleName(fAppendGenericArguments, pGenericBinding, fMakeSafeName, fAfterDot);
        }
    }
    else
    {
        if (IsProc() && PProc()->IsAnyConstructor())
        {
            BCSYM_NamedRoot *pParent = NULL;
            pParent = GetParent();
            if (pParent)
            {
                sbDisplayName.AppendSTRING(pParent->GetQualifiedName(fMakeSafeName, NULL, fAppendGenericArguments));
                sbDisplayName.AppendString(L".");
            }
            sbDisplayName.AppendString(GetCompiler()->TokenToString(tkNEW));

        }
        else
        {
            return GetQualifiedName(fMakeSafeName, NULL, fAppendGenericArguments);
        }
    }

    return GetCompiler()->AddString(&sbDisplayName);
}

#endif

// 




STRING* BCSYM::GetGlobalQualifiedName
(
    BCSYM_GenericBinding *pGenericContext
)
{
    SymbolEntryFunction;
    // A PNamedRoot() call chases through generic bindings so shouldn't be applied to a generic binding here.
    // We never expand the nullable types to nullable(of int) for example, and we do int? instead.

    STRING *FullyQualifiedTypeName =
        IsGenericTypeBinding() ?
            PGenericTypeBinding()->GetGenericType()->GetQualifiedName(true, NULL, true, PGenericBinding(), false, NULL, 0, true) :
            PNamedRoot()->GetQualifiedName(true, NULL, true, pGenericContext, false, NULL, 0, true, false);

    Compiler *pCompiler = PNamedRoot()->GetCompiler();

    // For the purposes of synthetc code gen, we do not want to append global if this
    // symbol is a nullable of intrinsic. For example, the following is incorrect: Global.Date?.
    // We can do this because above we never expand nullable.

    if (IsIntrinsicType() || IsObject() || IsGenericParam() ||
        (TypeHelpers::IsNullableType(this->ChaseToType()) && TypeHelpers::GetElementTypeOfNullable(this->ChaseToType())->IsIntrinsicType()))
    {
        return FullyQualifiedTypeName;
    }
    else
    {
        return
            pCompiler->ConcatStrings(
                pCompiler->TokenToString(tkGLOBAL),
                WIDE("."),
                FullyQualifiedTypeName);
    }
}

void BCSYM_NamedRoot::ReportError
(
    Compiler *pCompiler,
    ErrorTable *perror,
    Location *ploc
)
{
    SymbolEntryFunction;
    VSASSERT(IsBad(), "must be bad!");

    if (!perror)
        return;

    if (this->IsTypeForwarder())
    {
        unsigned cTypeArity = GetGenericParamCount();
        MetaImport::LoadTypeOfTypeForwarder(this->PTypeForwarder(), pCompiler, cTypeArity);
    }

    switch (GetErrid())
    {
    case 0:
        break;

    case ERRID_TypeImportedFromDiffAssemVersions3:
    case ERRID_UnreferencedAssembly3:
    case ERRID_UnreferencedModule3:
    case ERRID_UnreferencedAssemblyEvent3:
    case ERRID_UnreferencedModuleEvent3:
    case ERRID_UnreferencedAssemblyBase3:
    case ERRID_UnreferencedModuleBase3:
    case ERRID_UnreferencedAssemblyImplements3:
    case ERRID_UnreferencedModuleImplements3:
    case ERRID_TypeRefResolutionError3:
        {
            if (!GetBadNameSpace() || !*GetBadNameSpace())
            {
                perror->CreateErrorWithExtra(GetErrid(), GetBadExtra(), ploc, GetBadExtra(), GetBadName());
            }
            else
            {
                perror->CreateErrorWithExtra(GetErrid(), GetBadExtra(), ploc, GetBadExtra(),
                                             pCompiler->ConcatStrings(GetBadNameSpace(), L".", GetBadName()));
            }
        }
        break;

    case ERRID_ReferencedAssemblyCausesCycle3:
        {
            // Note:
            // The type name (i.e. GetBadName()) for this error is already qualified
            // with the namespace name.
            //
            // GetBadNamespace for this error contains the reference assembly identity to be
            // reported in the error.

            perror->CreateError(
                GetErrid(),
                ploc,
                GetBadNameSpace(),      // Assembly identity string for the AssemblyRef
                GetBadName(),           // Qualified name of the type that could not be found
                GetBadExtra());         // Reference cycle description string
        }
        break;

    case ERRID_ReferencedAssembliesAmbiguous4:
    case ERRID_ReferencedAssembliesAmbiguous6:
    case ERRID_ReferencedProjectsAmbiguous4:
        {
            // 



            perror->CreateErrorWithPartiallySubstitutedString(
                GetErrid(),
                GetBadExtra(),
                ploc,
                (!GetBadNameSpace() || !*GetBadNameSpace()) ?
                    GetBadName() :
                    pCompiler->ConcatStrings(GetBadNameSpace(), L".", GetBadName()));
        }
        break;

    case ERRID_SxSIndirectRefHigherThanDirectRef1:
        {
            StringBuffer ReplacementError;

            ResStringRepl(GetBadExtra(), &ReplacementError, GetQualifiedName());
            perror->CreateError(GetErrid(), ploc, ReplacementError.GetString());
        }
        break;

    case ERRID_InheritanceCycleInImportedType1:
        perror->CreateError(GetErrid(), ploc, GetQualifiedName());
        break;

    case ERRID_TypeFwdCycle2:
    case ERRID_ForwardedTypeUnavailable3:
        {
            VSASSERT(this->IsTypeForwarder(), "Non-type forwarder symbol unexpected!!!");
            STRING *pstrDefiningAssemblyName = this->GetContainer()->GetContainingProject()->GetAssemblyIdentity()->GetAssemblyIdentityString();

            if (!GetBadNameSpace() || !*GetBadNameSpace())
            {
                perror->CreateError(GetErrid(), ploc, GetBadName(), pstrDefiningAssemblyName, GetBadExtra());
            }
            else
            {
                perror->CreateError(
                    GetErrid(),
                    ploc,
                    pCompiler->ConcatStrings(GetBadNameSpace(), L".", GetBadName()), pstrDefiningAssemblyName, GetBadExtra());
            }
        }
        break;

    case ERRID_TypeRefFromMetadataToVBUndef:
        {
            if (IsGenericBadNamedRoot())
            {

                // MQ Bug 891543
                // If the current symbol is BCSYM_GenericBadNamedRoot and it is marked by ERRID_TypeRefFromMetadataToVBUndef, GetName() must defined.
                // Since in GetTypeByName, after BCSYM_GenericBadNamedRoot is created, SetBadName is called. So if GetName() returns NULL, then we can use
                // GetBadName()
                
                ThrowIfFalse(GetName() != NULL || GetBadName() != NULL);
                
                perror->CreateError(
                    GetErrid(),
                    ploc,
                    GetName() != NULL ? GetName() : GetBadName(),
                    (GetBadNameSpace() && GetBadNameSpace() != STRING_CONST(pCompiler, EmptyString)) ?
                        pCompiler->ConcatStrings(GetBadNameSpace(), L".", GetBadName()) :
                        GetBadName(),
                    GetBadExtra());
            }
            else
            {            
                perror->CreateError(
                    GetErrid(),
                    ploc,
                    GetName(),
                    (GetBadNameSpace() && GetBadNameSpace() != STRING_CONST(pCompiler, EmptyString)) ?
                        pCompiler->ConcatStrings(GetBadNameSpace(), L".", GetBadName()) :
                        GetBadName(),
                    GetBadExtra());
            }

        }
        break;

    case ERRID_MetadataMembersAmbiguous3:
        {
            BCSYM_Container *pParentContainer = GetContainer();
            STRING *pParentName = pParentContainer->GetQualifiedName();

            perror->CreateError(
                GetErrid(),
                ploc,
                GetName(),
                StringOfSymbol(pCompiler, pParentContainer),
                StringPool::IsEqual(pParentName, STRING_CONST(pCompiler, EmptyString)) ?
                    STRING_CONST(pCompiler, UnnamedNamespaceErrName) :
                    pParentName);
        }
        break;

    case ERRID_ExpressionOverflow1:
        perror->CreateError(GetErrid(), ploc, GetBadExtra());
        break;

    case ERRID_UnableToLoadType1:
    case ERRID_UnableToGetTypeInformationFor1:
    case ERRID_UnsupportedType1:

    default:
        perror->CreateError(GetErrid(), ploc, GetBadName());
        break;
    }
}

void BCSYM_NamedRoot::SetIsBad(SourceFile *pSourceFile)
{
    SymbolEntryFunction;
    if (pSourceFile)
    {
        VSASSERT(!GetContainingCompilerFile() ||
                  pSourceFile == GetContainingCompilerFile(), "Invalid state");
        SetStateForBadness(pSourceFile->GetCompState());
#if IDE 
        if (GetStateForBadness() > CS_TypesEmitted)
        {
            pSourceFile->SetNamedRootsMarkedBadAfterTypesEmitted(true);
        }
#endif IDE
    }
    else
    {
        SetStateForBadness(CS_NoState);
    }
    SetIsBadRaw(true);
}
//
// Mark the symbol as Bad
// and remember what state the file was in when we marked it
//
void BCSYM_NamedRoot::SetIsBad()
{
    if (CompilerFile *File = GetContainingCompilerFile())
    {
        SetStateForBadness(GetContainingCompilerFile()->GetCompState());
    }
    else
    {
        // for metadata symbols
        SetStateForBadness(CS_MAX);
    }

    SetIsBadRaw(true);
}

// Get the containing compiler.
Compiler *BCSYM_NamedRoot::GetCompiler()
{
    SymbolEntryFunction;
#if IDE
    // Under the IDE there's only one compiler which is the compiler package
    // NOTE:Microsoft,5/2002, this assumption is not true. VSEVE#538167 tells
    // us we should check NULL pointer.
    //
    // In the Debugger case, sometimes we might have multiple compilers. In order
    // to handle this correctly, we try to get the specific compiler first and only
    // if one is not present, then fall back to the global compiler package.
    //
    Compiler * pCompiler = NULL;
    
    CompilerProject * pProject = GetContainingProject();
    if (pProject)
    {
        pCompiler = pProject->GetCompiler();
    }

    if (pCompiler == NULL && GetCompilerSharedState()->IsInMainThread())
    {
        pCompiler = (Compiler *)GetCompilerPackage();
        VSASSERT(pCompiler,"No compiler gets loaded.");
    }
    else
    {
        VSASSERT(pCompiler || IsCCContainer() || IsCCConstant() || IsGenericBadNamedRoot(), "No Compiler found for NamedRoot.");
    }
    return pCompiler;
#else
    return GetContainingProject() ? 
        GetContainingProject() ->GetCompiler() : 
        NULL;
#endif
}


// Get corresponding CompilerHost.
CompilerHost *BCSYM_NamedRoot::GetCompilerHost()
{
    SymbolEntryFunction;
    CompilerHost * pCompilerHost = NULL;
    CompilerFile * pCompilerFile = NULL;

    pCompilerFile = GetCompilerFile();

    if(pCompilerFile)
    {
        pCompilerHost = pCompilerFile->GetCompilerHost();
    }
    
#if IDE 
    // In the Debugger case, file may not be set.
    if(!pCompilerHost)
    {
        CompilerProject * pProject = GetContainingProject();

        if (pProject)
        {
            pCompilerHost = pProject->GetCompilerHost();
        }
    }
#endif

    AssertIfNull(pCompilerHost);

    return pCompilerHost;
}


/*
** Generics stuff
*/

bool BCSYM_GenericParam::CanBeInstantiated()
{
    SymbolEntryFunction;
    BCITER_Constraints ConstraintsIter(this, true, this->GetCompiler());

    while (BCSYM_GenericConstraint *Constraint = ConstraintsIter.Next())
    {
        VSASSERT(!Constraint->IsBadConstraint(), "Bad constraint unexpected!!!");

        if ((Constraint->IsNewConstraint() && ConstraintsIter.CurrentGenericParamContext() == this) ||
            Constraint->IsValueConstraint() ||
            (Constraint->IsGenericTypeConstraint() &&
                TypeHelpers::IsValueType(Constraint->PGenericTypeConstraint()->GetType())))
        {
            return true;
        }
    }

    return false;
}

bool BCSYM_GenericParam::IsValueType()
{
    SymbolEntryFunction;
    BCITER_Constraints ConstraintsIter(this, true, this->GetCompiler());

    while (BCSYM_GenericConstraint *Constraint = ConstraintsIter.Next())
    {
        VSASSERT(!Constraint->IsBadConstraint(), "Bad constraint unexpected!!!");

        if (Constraint->IsValueConstraint() ||
            (Constraint->IsGenericTypeConstraint() &&
                TypeHelpers::IsValueType(Constraint->PGenericTypeConstraint()->GetType())))
        {
            return true;
        }
    }

    return false;
}

bool
BCSYM_GenericParam::IsReferenceType
(
)
{
    SymbolEntryFunction;
    // The logic of this function matches TypeVarTypeDesc::ConstrainedAsObjRef() in the CLR's
    // redbits\ndp\clr\src\vm\typedesc.cpp (see devdiv bug # 73604). The logic is as follows:
    //
    // The following must be true for a generic variable to be regarded as "constrained to be a reference type"
    // (1) must have the reference type special constraint "As {..., Class, ...}"
    // (2) or must have a base type constraint which is a reference type other than System.Object, System.ValueType, System.Enum,
    //        "As {..., BaseRefType, ...}"
    // (3) or must be constrained by another generic variable which is constrained to be a reference type
    //        "(of T as U)(of U as BaseRefType)"
    //     nb. for esoteric reasons, mentioned in redbits\...\typedesc.cpp, the case "(of T as U)(of U as Class)"
    //     doesn't guarantee that T is a reference type. That's why checks 2 and 3 are delegated to a recursive
    //     helper function.
    //

    // check (1):
    GenericConstraint * Constraint = GetConstraints();
    while (Constraint)
    {
        if (Constraint->IsReferenceConstraint())
        {
            return true;
        }
        Constraint = Constraint->Next();
    }

    CompilerHost * pCompilerHost = GetCompilerHost();
    Compiler * pCompiler = GetCompiler();
    
    // checks (2) and (3)
    return IsReferenceTypeHelper(this, pCompiler, pCompilerHost);
}

// IsReferenceTypeHelper: implements checks (2) and (3) as described in the comments
// in its caller function IsReferenceType.
//static
bool
BCSYM_GenericParam::IsReferenceTypeHelper
(
    GenericParameter * GenericParam,
    Compiler * Compiler,
    CompilerHost * CompilerHost
)
{
    BCITER_Constraints ConstraintsIter(GenericParam, true, Compiler);

    while (BCSYM_GenericConstraint *Constraint = ConstraintsIter.Next())
    {
        VSASSERT(!Constraint->IsBadConstraint(), "Bad constraint unexpected!!!");

        if (!Constraint->IsGenericTypeConstraint())
        {
            continue;
        }

        Type *T = Constraint->PGenericTypeConstraint()->GetType();

        // (3) Recurse into generic parameters
        if (T->IsGenericParam() && IsReferenceTypeHelper(T->PGenericParam(), Compiler, CompilerHost))
        {
            return true;
        }


        // (2) check if it has a base type constraint that's a ref other than object/valuetype/enum
        // Dev10#413733: remember that an array is ref type too!
        if (!T->IsInterface() && TypeHelpers::IsReferenceType(T) && (T->IsClass() || T->IsArrayType()))
        {
            if (T != CompilerHost->GetFXSymbolProvider()->GetType(FX::ObjectType) &&
                T != CompilerHost->GetFXSymbolProvider()->GetType(FX::ValueTypeType) &&
                T != CompilerHost->GetFXSymbolProvider()->GetType(FX::EnumType))
            {
                return true;
            }
        }
    }

    // If none of the checks (2) or (3) are met, then we can't guarantee that this is a reference type:
    return false;
}

bool BCSYM_GenericParam::HasClassConstraint
(
    bool IgnoreNonReferenceTypes
)
{
    SymbolEntryFunction;
    BCITER_Constraints ConstraintsIter(this, true, this->GetCompiler());

    while (BCSYM_GenericConstraint *Constraint = ConstraintsIter.Next())
    {
        VSASSERT(!Constraint->IsBadConstraint(), "Bad constraint unexpected!!!");

        if (Constraint->IsGenericTypeConstraint() &&
            !Constraint->PGenericTypeConstraint()->GetType()->IsInterface())
        {
            if (!IgnoreNonReferenceTypes ||
                TypeHelpers::IsReferenceType(Constraint->PGenericTypeConstraint()->GetType()))
            {
                return true;
            }
        }
    }

    return false;
}

BCSYM* BCSYM_GenericParam::GetClassConstraint
(
    CompilerHost *CompilerHost,
    NorlsAllocator *Allocator,
    bool ReturnArraysAsSystemArray,
    bool ReturnValuesAsSystemValueTypeOrEnum
)
{
    SymbolEntryFunction;
    // Generic parameters may be constrained by (1) other generic parameters,
    // (2) classes/interfaces as VB allows, (3) any other type as CLR/PEVerify alows.
    //
    // Consider the set of all non-generic-parameter non-interface types reachable from
    // this type through "constrained-as" relations. This set will have a dominant element
    // (i.e. a member of the set to which all other members can be identity- or widening-converted to).
    // That's because Bindable::CheckGenericConstraints has made sure of this.
    //
    // We pick the dominant type as the ClassConstraint.
    //
    // "(Of T As Integer() )" -- the flag "ReturnArraysAsSystemArray" makes us
    // return System.Array as the class-constraint here; otherwise we return Integer() itself.
    //
    // "(Of T As Mystruct )" -- the flag "ReturnValuesAsSystemValueOrEnum" makes
    // us return System.ValueType as the class-constraint here; otherwise we return Mystruct itself.
    //
    // "(Of T As Myenum )" -- the flag "ReturnValuesAsSystemValueOrEnum" makes us
    // return System.EnumType as the class-constraint here; otherwise we return Myenum itself.
    //
    // "(Of T As Integer )" -- the flag "ReturnValuesAsSystemValueOrEnum" makes us
    // return System.ValueType as the class-constraint here; otherwise we return Integer itself.

    BCITER_Constraints ConstraintsIter(this, true, this->GetCompiler());

    BCSYM *ClassTypeConstraint = NULL;

    while (BCSYM_GenericConstraint *Constraint = ConstraintsIter.Next())
    {
        VSASSERT(!Constraint->IsBadConstraint(), "Bad constraint unexpected!!!");

        if (Constraint->IsGenericTypeConstraint() &&
            !Constraint->PGenericTypeConstraint()->GetType()->IsInterface())
        {
            if (ClassTypeConstraint)
            {
                // If there are more than one class constraints (which can happen when atleast one of them
                // if from another type parameter using as a constraint), pick the most derived class as the
                // class constraint. This will help name lookup, intellisense, etc find the members from the
                // perspective of the most derived class.

                Symbols SymbolFactory(this->GetCompiler(), Allocator, NULL, NULL);

                ConversionClass Conversion =
                    ClassifyPredefinedCLRConversion(
                        ClassTypeConstraint,
                        Constraint->PGenericTypeConstraint()->GetType(),
                        SymbolFactory,
                        CompilerHost,
                        ConversionSemantics::Default, // for constraints we don't consider value conversions (e.g. enum->underlying-type)
                        0, // We've simplified the types, so set RecursionCount=0
                        false,
                        NULL,
                        NULL,
                        NULL
                        );

                VSASSERT(Conversion != ConversionError, "Inconsistency in valid class constraints!!!");

                if (Conversion == ConversionWidening)
                {
                    ClassTypeConstraint = Constraint->PGenericTypeConstraint()->GetType();
                }
            }
            else
            {
                ClassTypeConstraint = Constraint->PGenericTypeConstraint()->GetType();
            }
        }
    }

    // "ReturnArraysAsSystemArray" used to be called "DigThroughToClassType".
    // But the only types that don't satisfy BCSYM::IsClass() are arrays, interfaces, generic-params, void and bad.
    // We know we're not getting an interface or a generic-param from the iteration above.
    // So the only non-class conceivable is an array. (void and bad should not appear as class constraints.)
    if (ReturnArraysAsSystemArray &&
        ClassTypeConstraint &&
        !ClassTypeConstraint->IsClass())
    {
        if (ClassTypeConstraint->IsArrayType())
        {
            ClassTypeConstraint = CompilerHost->GetFXSymbolProvider()->GetType(FX::ArrayType);
        }
        else
        {
            VSFAIL("Unexpected class constraint type!!!");
            ClassTypeConstraint = NULL;
        }
    }

    // "ReturnValuesAsSystemValueTypeOrEnum" used to be called "DigThroughToReferenceType"
    // But the only non-reference-types are value types and certain generic-parameters.
    // We know we're not getting generic-parameters from the iteration above.
    // So all this test does is turn value types into System.ValueType or System.Enum.
    if (ReturnValuesAsSystemValueTypeOrEnum &&
        ClassTypeConstraint &&
        TypeHelpers::IsValueType(ClassTypeConstraint))
    {
        if (TypeHelpers::IsEnumType(ClassTypeConstraint))
        {
            ClassTypeConstraint = CompilerHost->GetFXSymbolProvider()->GetType(FX::EnumType);
        }
        else
        {
            ClassTypeConstraint = CompilerHost->GetFXSymbolProvider()->GetType(FX::ValueTypeType);
        }
    }

    return ClassTypeConstraint;
}


BCSYM *
BCSYM_GenericBinding::GetCorrespondingArgument
(
    BCSYM_GenericParam *Param
)
{
    SymbolEntryFunction;
    if (Param->GetParent() == GetGeneric())
    {
        return GetArgument(Param->GetPosition());
    }

    if (GetParentBinding())
    {
        return GetParentBinding()->GetCorrespondingArgument(Param);
    }

    // For Generic methods, lot of times (eg: in bindable - overriding, overloading, etc.),
    // we are processing the raw method along with a type binding and in all those cases,
    // we don't synthesize an open binding for the method, so we deal with the raw generic
    // method directly, hence this is needed.
    //
    VSASSERT(Param->IsGenericMethodParam(), "Failed to find a type argument for a type parameter.");

    return Param->IsGenericMethodParam() ?
            Param :
            NULL;
}

//static
bool BCSYM_GenericBinding::CompareBindingArguments
(
    BCSYM_GenericBinding * pLeft,
    BCSYM_GenericBinding * pRight
)
{
    ThrowIfNull(pLeft);
    ThrowIfNull(pRight);

    if (pLeft->GetArgumentCount() != pRight->GetArgumentCount())
    {
        return false;
    }

    for (unsigned ArgumentIndex = 0; ArgumentIndex < pLeft->GetArgumentCount(); ArgumentIndex++)
    {
        if (!BCSYM::AreTypesEqual(pLeft->GetArgument(ArgumentIndex), pRight->GetArgument(ArgumentIndex)))
        {
            return false;
        }
    }

    return true;

}

//static
bool BCSYM_GenericBinding::AreBindingsEqual
(
    BCSYM_GenericBinding * pLeft,
    BCSYM_GenericBinding * pRight
)
{
    if (BoolToInt(pLeft) ^ BoolToInt(pRight))
    {
        return false;
    }
    else if (pLeft && pRight)
    {
        if(BoolToInt(pLeft->IsGenericTypeBinding()) ^ BoolToInt(pRight->IsGenericTypeBinding()))
        {
            return false;
        }
        else if (pLeft->IsGenericTypeBinding())
        {
            return BCSYM_GenericTypeBinding::AreTypeBindingsEqual(pLeft->PGenericTypeBinding(), pRight->PGenericTypeBinding());
        }
        else
        {
            return CompareBindingArguments(pLeft, pRight);
        }
    }
    else
    {
        return true;
    }
}


/*
** BCSYM_Interface
*/

BCSYM_Implements *BCSYM_Interface::GetFirstImplements()
{
    SymbolEntryFunction;
    VSASSERT(AreBaseAndImplementsLoaded(), "must load base and implements first!");

    // Make sure bases are resolved before giving them to anybody like semantics
    // to search for stuff
    Bindable::ResolveBasesIfNotStarted(this, NULL);

    return GetImpList();
}


// Does this interface derive from pinterfaceMaybeBase?
bool BCSYM_Interface::DerivesFrom(Compiler *pCompiler, BCSYM_Interface *pinterfaceMaybeBase)
{
    SymbolEntryFunction;
    SymbolNode *pslnode;
    NorlsAllocator nra(NORLSLOC);
    Cycles cycles(pCompiler, &nra);

    cycles.AddSymbolToVerify(this);
    cycles.FindCycles();

    for (pslnode = cycles.GetSymbolNodeList()->GetFirst(); pslnode != NULL; pslnode = pslnode->Next())
    {
        if (pslnode->m_Symbol == pinterfaceMaybeBase)
        {
            return true;
        }
    }

    return false;
}

bool BCSYM_Interface::IsDispinterface()
{
    SymbolEntryFunction;
    long Flags;

    GetPAttrVals()->GetPWellKnownAttrVals()->GetInterfaceTypeData(&Flags);

    if (Flags & 0x2)
    {
        return true;
    }
    else
    {
        for ( BCSYM_Implements *BaseInterface = GetFirstImplements(); BaseInterface != NULL; BaseInterface = BaseInterface->GetNext())
        {
            if (BaseInterface->GetCompilerRoot()->IsInterface() &&
                BaseInterface->GetCompilerRoot()->PInterface()->IsDispinterface())
                return true;
        }
    }

    return false;

}

// If member not found:generate a late-bound call to that name
bool BCSYM_Interface::IsExtensible()
{
    SymbolEntryFunction;
    short Flags;

    if (GetPAttrVals()->GetPWellKnownAttrVals()->GetTypeLibTypeData(&Flags) && !(Flags & TYPEFLAG_FNONEXTENSIBLE))
    {
        return true;
    }
    else
    {
        for ( BCSYM_Implements *BaseInterface = GetFirstImplements(); BaseInterface != NULL; BaseInterface = BaseInterface->GetNext())
        {
            if (BaseInterface->GetCompilerRoot()->IsInterface() &&
                BaseInterface->GetCompilerRoot()->PInterface()->IsExtensible())
                return true;
        }
    }

    return false;
}

/**************************************************************************************************
;GetFirstInstanceConstructor

Return the head of the list of constructors in this container
***************************************************************************************************/
BCSYM_Proc *BCSYM_Class::GetFirstInstanceConstructor
(
    Compiler *CompilerInstance // [in] instance of the compiler
)
{
    SymbolEntryFunction;
    BCSYM_NamedRoot *Candidate = SimpleBind( NULL, STRING_CONST( CompilerInstance, Constructor));

    while ( Candidate )
    {
        if ( Candidate->IsProc() && Candidate->PProc()->IsInstanceConstructor())
        {
            return Candidate->PProc();
        }
        Candidate = Candidate->GetNextOfSameName();
    }
    return NULL;
}

/**************************************************************************************************
;GetFirstConstructor

Return the first constructor that doesn't require any parameters
***************************************************************************************************/
BCSYM_Proc *BCSYM_Class::GetFirstInstanceConstructorNoRequiredParameters(Compiler *pCompiler)
{
    SymbolEntryFunction;
    BCSYM_Proc *ConstructorMethod = GetFirstInstanceConstructor(pCompiler);
    while ( ConstructorMethod && ConstructorMethod->GetRequiredParameterCount() != 0 )
    {
        ConstructorMethod = ConstructorMethod->GetNextInstanceConstructor();
    }
    return ConstructorMethod;
}

/**************************************************************************************************
;GetClassConstructor

Returns the container constructor
//If there is no other code to initialize, GetSharedConstructor returns NULL. If I add a line 
//                Dim x as new string
//Then the compiler will create a constructor

***************************************************************************************************/
BCSYM_Proc *BCSYM_Class::GetSharedConstructor
(
    Compiler *CompilerInstance // [in] instance of the compiler
)
{
    SymbolEntryFunction;
    BCSYM_NamedRoot *Candidate = SimpleBind(NULL, STRING_CONST(CompilerInstance, SharedConstructor));

    VSASSERT(Candidate == NULL ||
             (Candidate->IsProc() && Candidate->PProc()->IsSharedConstructor()),
             "unexpected shared constructor situation");

    if (Candidate && Candidate->IsProc())
    {
        return Candidate->PProc();
    }

    return NULL;
}

inline Vtypes BCSYM_Class::GetVtype()
{
    SymbolEntryFunction;
    if (IsIntrinsicType())
        return (Vtypes)GetRawVtype();
    if (IsEnum())
    {
        // We have to do this to make sure that we know what the actual vtype of this is.
        if (!IsBasic())
        {
            // For enums in metadata files, i.e. imported enums, we need to import the
            // children and get the enum's base type (i.e. the underlying type) from the
            // type of the special member field named "value__" in the enum in order to
            // determine the Vtype.
            //
            EnsureChildrenLoaded();
        }
        else
        {
            // For enums in source files, we need to resolve the enum's base type
            // (i.e. the underlying type) in order to determine the Vtype.
            //
            if (!Bindable::DetermineEnumUnderlyingTypeIfPossible(this, NULL))
            {
                // Could not determine yet, try again later
                return t_bad;
            }
        }
        return (Vtypes)GetRawVtype();
    }
    if (IsStruct())
        return t_struct;

    return t_ref;
}

// The base class.
BCSYM *BCSYM_Class::GetBaseClass()
{
    SymbolEntryFunction;
    if (BCSYM_Container *MainType =
            IsPartialTypeAndHasMainType())
    {
        VSASSERT( MainType->IsClass(),
                        "How can a partial class's main type not be a class ?");
        return MainType->PClass()->GetBaseClass();
    }

    VSASSERT( AreBaseAndImplementsLoaded(), "Must load base and implements first!" );

    // Make sure bases are resolved before giving them to anybody like semantics
    // to search for stuff
    Bindable::ResolveBasesIfNotStarted(this, NULL);

    if (IsBaseBad())
    {
        return Symbols::GetGenericBadNamedRoot();
    }

    if ( GetRawBase() && // in the Expression Evaluator the Base might NULL
         IsBaseInvolvedInCycle() == false && // avoid returning the base that will cause us to loop around a circular inheritance hierarchy
         (!GetRawBase()->IsNamedType() || GetRawBase()->PNamedType()->GetSymbol()))
    {
        return GetRawBase()->DigThroughNamedType();
    }
    return NULL;
}

// The base class - a little more checking done to make sure the base isn't bad 1st
BCSYM_Class *BCSYM_Class::GetCompilerBaseClass()
{
    SymbolEntryFunction;
    if (BCSYM_Container *MainType =
            IsPartialTypeAndHasMainType())
    {
        VSASSERT( MainType->IsClass(),
                        "How can a partial class's main type not be a class ?");
        return MainType->PClass()->GetCompilerBaseClass();
    }

#if HOSTED
    if (GetHashRaw() != NULL && GetHashRaw()->GetExternalSymbolSource())
    {
        return NULL;
    }
#endif

    VSASSERT( AreBaseAndImplementsLoaded(), "Must load base and implements first!" );

    // Make sure bases are resolved before giving them to anybody like semantics
    // to search for stuff
    Bindable::ResolveBasesIfNotStarted(this, NULL);

    if (IsBaseBad())
    {
        return NULL;
    }

    // Note: You would think this shouldn't fail because all classes either inherit from Object
    // or have some other base class.  But it can - for instance, when an incomplete inherits
    // statement is committed so you don't want to assert on GetRawBase() or IsObject()

    if ( GetRawBase() &&
         IsBaseInvolvedInCycle() == false && // avoid returning the base that will cause us to loop around a circular inheritance hierarchy
         ( !GetRawBase()->IsNamedType() || ( GetRawBase()->PNamedType()->GetSymbol() && !GetRawBase()->PNamedType()->GetSymbol()->IsBad())) &&
         GetRawBase()->DigThroughNamedType()->IsClass())
    {
        return GetRawBase()->DigThroughNamedType()->PClass();
    }
    return NULL;
}

BCSYM_Implements *BCSYM_Class::GetFirstImplements()
{
    SymbolEntryFunction;
    VSASSERT(AreBaseAndImplementsLoaded(), "must load base and implements first!");
    return GetImpList();
}

// If we are not allowed to create an instance of this class.
bool BCSYM_Class::IsNotCreateable()
{
    SymbolEntryFunction;
    short Flags;

    if (GetPAttrVals()->GetPWellKnownAttrVals()->GetTypeLibTypeData(&Flags))
    {
        return !(Flags & TYPEFLAG_FCANCREATE);
    }
    else
    {
        return false;
    }
}

/**************************************************************************************************
;GetInheritsLocation

Return the location information for the INHERITS line in the class
***************************************************************************************************/
Location *BCSYM_Class::GetInheritsLocation()
{
    SymbolEntryFunction;
    if ( GetRawBase()->HasLocation())
    {
        return GetRawBase()->GetLocation();
    }
    else 
    {
        Location * inheritsLocation = GetInheritsLocationForIntrinsic();
        if (inheritsLocation)
        {
            return inheritsLocation;
        }
    }
    return NULL;
}

/**************************************************************************************************
;SetInheritsLocation

Sometimes you inherit from something that doesn't have Location information in the symbol to store
where on the inherits line it was (Inherits Short, for instance)  So you use this to keep track of
location information for the inherits line in those cases
***************************************************************************************************/
void BCSYM_Class::SetInheritsLocation( Location *Loc, NorlsAllocator *Allocator )
{
    SymbolEntryFunction;
    Location * inheritsLocation = GetInheritsLocationForIntrinsic();
    if ( !inheritsLocation )
    {
        inheritsLocation = (Location *)Allocator->Alloc( sizeof( Location ));
        SetInheritsLocationForIntrinsic(inheritsLocation);
    }
    inheritsLocation->m_lBegLine = Loc->m_lBegLine;
    inheritsLocation->m_lEndLine = Loc->m_lEndLine;
    inheritsLocation->m_lBegColumn = Loc->m_lBegColumn;
    inheritsLocation->m_lEndColumn = Loc->m_lEndColumn;
}

/**************************************************************************************************
;DerivesFrom

Does 'this' class derive from PossibleBaseClass?
***************************************************************************************************/
bool // true - 'this' derives from PossibleBaseClass : false - it does not
BCSYM_Class::DerivesFrom
(
    BCSYM_Class *PossibleBaseClass // [in] Does 'this' derive from PossibleBaseClass
)
{
    SymbolEntryFunction;
    for ( BCSYM *Base = this->GetBaseClass(); Base; Base = Base->PClass()->GetBaseClass())
    {
        if ( Base->IsGenericBadNamedRoot()) return false;
        if ( Base == PossibleBaseClass ) return true;
    }
    return false;
}

/**************************************************************************************************
;StowBackingFieldsForStatics

Stores the list of backing fields for static locals and sets the parents of each to point at this
class
***************************************************************************************************/
void BCSYM_Class::StowBackingFieldsForStatics(
    SymbolList *BackingFieldsList, // [in] the list of backing fields to add to stow
    Symbols *SymbolFactory // [in] the allocator to use
)
{
    SymbolEntryFunction;
    if ( BackingFieldsList->GetCount() > 0 )
    {
        if ( !GetBackingFieldsForStatics())
        {
            SetBackingFieldsForStatics(SymbolFactory->GetHashTable( GetCompiler()->AddString(L"BackingFields"), this,
                                                                          true, // set parent for symbols added to hash
                                                                          0, // have GetHashtable figure out the count
                                                                          BackingFieldsList )
            );
        }
        else
        {
            Symbols::AddSymbolListToHash( GetBackingFieldsForStatics(), BackingFieldsList, true );
        }
    }
}

BCSYM_Hash *BCSYM_Class::GetBackingFieldsForStatics()
{
    SymbolEntryFunction;
    VSASSERT(!this->GetSourceFile() ||
                this->GetSourceFile()->GetCompState() == CS_NoState ||
                Bindable::HavePartialTypesBeenResolvedForGivenType(this),
                    "too early to invoke this!!!");

    return m_BackingFieldsForStaticLocals;
}

//
// Method to return if a class symbol can be used as a custom attribute.
//
bool BCSYM_Class::IsValidAttributeClass
(
    ErrorTable *pErrorTable,
    BCSYM_ApplAttr *psymReportError // If error is found, report error on this symbol
)
{
    SymbolEntryFunction;
    BCITER_CHILD ChildIterator;
    Compiler *pCompiler = GetCompiler();
    bool IsValid = true;    // Assume valid
    CorAttributeTargets attrtargetsValidOn;
    bool fAllowMultiple;
    bool fInherited;

    // Use the cached validity status only when no error reporting is required.
    if (pErrorTable == NULL)
    {
        // See if we've already cached the result
        if (GetAttributeValidity() == ValidAsAttribute)
        {
            return true;
        }

        if (GetAttributeValidity() == InvalidAsAttribute)
        {
            return false;
        }

        VSASSERT(GetAttributeValidity() == ValidityUnknown, "What else can it be?");
    }

    // If pErrorTable is specified, psymReportError must be, too
    VSASSERT((pErrorTable == NULL) == (psymReportError == NULL),
        "Have pErrorTable but not psymReportError (or vice versa)");

    // If psymReportError is specified, the attribute being applied must be "this"
    VSASSERT((psymReportError == NULL) ||
              (psymReportError->GetAttributeSymbol()->PClass() == this),
              "Mismatched attribute symbols!");

    //
    // Check whether it the class is valid and report errors if not.
    //

    // Check to see if the attribute is System.AttributeUsageAttribute.
    // We special-case this because of the chicken-and-egg problem -- we
    // can't really tell if AttributeUsageAttribute is legal until we know
    // if it has an AttributeUsageAttribute attached to it, but we can't
    // test that it has one attached until we have a definition of it.
    // Blech.

    VSASSERT(GetCompilerFile() && GetCompilerFile()->GetCompilerHost(), "Unexpected NULL pointers.");
    if (this == GetCompilerFile()->GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::AttributeUsageAttributeType))
    {
        goto Error;
    }

    // We don't support this attribute yet.
    if (GetCompilerFile()->GetCompilerHost()->GetFXSymbolProvider()->IsTypeAvailable(FX::DefaultCharSetAttributeType) &&
        this == GetCompilerFile()->GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::DefaultCharSetAttributeType))
    {
        IsValid = false;

        pErrorTable->CreateErrorWithSymbol(ERRID_DefaultCharSetAttributeNotSupported, psymReportError, GetName());

        goto Error;
    }

    if (IsStruct())
    {
        IsValid = false;

        // pErrorTable can be NULL, but CreateErrorWithSymbol handles this case
        pErrorTable->CreateErrorWithSymbol(ERRID_AttributeMustBeClassNotStruct1, psymReportError, GetName());

        // User was way off.  No sense reporting more errors in this case.
        goto Error;
    }

    // Check whether it derives from System.Attribute.
    if (GetCompilerFile()->GetCompilerHost()->GetFXSymbolProvider()->IsTypeAvailable(FX::AttributeType) &&
        !DerivesFrom(GetCompilerFile()->GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::AttributeType)->PClass()))
    {
        IsValid = false;
        pErrorTable->CreateErrorWithSymbol(ERRID_AttributeMustInheritSysAttr, psymReportError, GetName());
    }
    // Check for attribute usage only if errors are being checked. In some IDE cases, the attribute usage might
    // not have been propagated yet from the base class, so in order to account for it, allow this here only
    // in the non-IDE cases, i.e. when a errorlog is passed in
    //
    else if (pErrorTable)
    {
        VSASSERT(GetPAttrVals() &&
                 GetPWellKnownAttrVals()->GetAttributeUsageData(&attrtargetsValidOn, &fAllowMultiple, &fInherited),
                    "Any class inheriting from System.Attribute should atleast inherit its AttributeUsage info!!!");

        // Check whether it has System.AttributeUsageAttribute.
        if (GetPAttrVals() == NULL)
        {
            IsValid = false;
            pErrorTable->CreateErrorWithSymbol(ERRID_AttributeMustHaveAttrUsageAttr, psymReportError, GetName());
        }
        else if (GetPAttrVals()->GetPWellKnownAttrVals()->GetAttributeUsageData(&attrtargetsValidOn, &fAllowMultiple, &fInherited))
        {
            // Symbol has a cached AttributeUsage tucked away.  It's legal.
        }
        else
        {
            IsValid = false;
            pErrorTable->CreateErrorWithSymbol(ERRID_AttributeMustHaveAttrUsageAttr, psymReportError, GetName());
        }
    }

    // Check whether the class is abstract.
    if (IsMustInherit())
    {
        IsValid = false;
        pErrorTable->CreateErrorWithSymbol(ERRID_AttributeCannotBeAbstract, psymReportError, GetName());
    }

    // Check whether the class has any 'MustOverride' methods still un-overridden.
    if (!AreMustOverridesSatisfied())
    {
        IsValid = false;
        pErrorTable->CreateErrorWithSymbol(ERRID_AttributeCannotHaveMustOverride, psymReportError, GetName());
    }

Error:
    SetAttributeValidity(IsValid ? ValidAsAttribute : InvalidAsAttribute);
    return IsValid;
}

//
// Return the name of the nested interface that we will generate
// when the user add <ComClass()> to a class.
//
STRING* BCSYM_Class::GetComClassInterfaceName()
{
    SymbolEntryFunction;
    return GetCompiler()->ConcatStrings(L"_", GetName());
}

//
// Return the name of the nested event interface that we will generate
// when the user add <ComClass()> to a class.
//
STRING* BCSYM_Class::GetComClassEventsInterfaceName()
{
    SymbolEntryFunction;
    return GetCompiler()->ConcatStrings(L"__", GetName());
}

//
// Sets the base class to be bad
//
void BCSYM_Class::SetIsBaseBad(bool IsBaseBad)
{
    SymbolEntryFunction;
    // Can only be set for source files.
    //
    // Can only be set to true when moving from declared to bound
    //
    VSASSERT(!IsBaseBad ||
                (this->GetSourceFile() &&
                 this->GetSourceFile()->GetCompState() == CS_Declared), "Base class badness set in wrong comp state!!!");

    m_IsBaseBad = IsBaseBad;
}

Location *NameTypeArguments::GetArgumentLocation(unsigned Index)
{
    if (m_Arguments[Index] && m_Arguments[Index]->IsNamedType())
    {
        return m_Arguments[Index]->GetLocation();
    }

    unsigned NonNamedTypeArgumentsIndex = 0;
    for (unsigned i = 0; i < m_ArgumentCount; i++)
    {
        if (i == Index)
        {
            return &m_TextSpansForNonNamedTypeArguments[NonNamedTypeArgumentsIndex];
        }

        BCSYM *Argument = m_Arguments[i];

        if (!Argument || !Argument->IsNamedType())
        {
            NonNamedTypeArgumentsIndex++;
        }
    }

    VSFAIL("Location of unexpected type argument requested!!!");

    return NULL;
}

Location *NameTypeArguments::GetAllArgumentsLocation(Location *Loc)
{
    if (!Loc)
    {
        return NULL;
    }

    if (m_ArgumentCount == 0)
    {
        Loc->Invalidate();
    }
    else
    {
        Loc->SetLocation(
                GetArgumentLocation(0)->m_lBegLine,
                GetArgumentLocation(0)->m_lBegColumn,
                GetArgumentLocation(m_ArgumentCount - 1)->m_lEndLine,
                GetArgumentLocation(m_ArgumentCount - 1)->m_lEndColumn);
    }

    return Loc;
}


/*
** BCSYM_Proc methods
*/

unsigned BCSYM_Proc::GetParameterCount()
{
    SymbolEntryFunction;
    BCSYM_Param *pparam;
    unsigned cParams = 0;

    for (pparam = GetFirstParam();
         pparam;
         pparam = pparam->GetNext())
    {
        cParams++;
    }

    return cParams;
}

unsigned BCSYM_Proc::GetRequiredParameterCount()
{
    SymbolEntryFunction;
    unsigned NumParams = 0;

    for (BCSYM_Param *Param = GetFirstParam(); NULL != Param; Param = Param->GetNext())
    {
        if (!Param->IsOptional() && !Param->IsParamArray())
        {
            NumParams++;
        }
    }

    return NumParams;
}

BCSYM_Param *BCSYM_Proc::GetParamArrayParameter()
{
    SymbolEntryFunction;
    BCSYM_Param *pparam;

    for (pparam = GetFirstParam(); NULL != pparam; pparam = pparam->GetNext())
    {
        if (pparam->IsParamArray())
            return pparam;
    }

    return NULL;
}

// Optimized version of the above that only walks through the
// parameter list once.
//
void
BCSYM_Proc::GetAllParameterCounts
(
    unsigned &RequiredCount,  // Count of non-optional and non-paramarray parameters.
    unsigned &MaximumCount,   // Raw count of parameters (but does not consider paramarray expansion).
    bool &HasParamArray
)
{
    SymbolEntryFunction;
    RequiredCount = 0;
    MaximumCount = 0;
    HasParamArray = false;

    for (BCSYM_Param *Parameter = GetFirstParam(); Parameter; Parameter = Parameter->GetNext())
    {
        if (Parameter->IsParamArray())
        {
            HasParamArray = true;
        }
        else if (!Parameter->IsOptional())
        {
            RequiredCount++;
        }

        MaximumCount++;
    }
}

BCSYM_Param *BCSYM_Proc::GetLastParam()
{
    SymbolEntryFunction;
    BCSYM_Param *pparam = GetFirstParam();

    if (pparam)
    {
        while (pparam->GetNext())
        {
            pparam = pparam->GetNext();
        }
    }

    return pparam;
}

// Initialize an iteration over this proc's parameters.
void BCSYM_Proc::InitParamIterator(BCITER_Parameters *pbiparam)
{
    SymbolEntryFunction;
    pbiparam->Init(this);
}


// Find a parameter of the specified name.  If found, true is returned and
// *ppparamOut is set to the parameter symbol and *piposOut is set to the
// 0-based index of the parameter's position.  If not found, false is returned.
// Used by named-argument handlers.
bool BCSYM_Proc::GetNamedParam(_In_z_ STRING *pstrName, BCSYM_Param **ppparamOut, UINT *piposOut)
{
    SymbolEntryFunction;
    BCSYM_Param *pparam;
    UINT ipos;

    ipos = 0;
    pparam = GetFirstParam();
    while (pparam != NULL)
    {
        if (StringPool::IsEqual(pparam->GetName(), pstrName))
        {
            *ppparamOut = pparam;
            *piposOut = ipos;
            return true;
        }
        pparam = pparam->GetNext();
        ipos++;
    }

    return false;
} // BCSYM_Proc::GetNamedParam

// Get the next constructor.
BCSYM_Proc *BCSYM_Proc::GetNextInstanceConstructor()
{
    SymbolEntryFunction;
    VSASSERT( IsInstanceConstructor(), "Must be a constructor to get the next one.");

    BCSYM_NamedRoot *pnamed;
    for(pnamed = GetNextOfSameName();
        pnamed;
        pnamed = pnamed->GetNextOfSameName())
    {
        if (pnamed->IsProc() && pnamed->PProc()->IsInstanceConstructor())
        {
            break;
        }
    }

    return pnamed->PProc();
}

#if IDE 
bool BCSYM_Proc::IsENCSynthFunction()
{
    SymbolEntryFunction;
    // SYNTH_AddToENCTrackingList is not listed here because the compiler has
    // to emit it always, like it does for the __ENCList field. The methods
    // listed here are generated on demand by ENCBuilder.
    if (this &&
        IsSyntheticMethod() &&
        (PSyntheticMethod()->GetSyntheticKind() == SYNTH_ENCUpdateHandler ||
         PSyntheticMethod()->GetSyntheticKind() == SYNTH_ENCHiddenRefresh ||
         PSyntheticMethod()->GetSyntheticKind() == SYNTH_ENCSharedHiddenRefresh ||
         PSyntheticMethod()->GetSyntheticKind() == SYNTH_IterateENCTrackingList))
    {
        return true;
    }
    return false;
}
#endif

void
BCSYM_Proc::GenerateFixedArgumentBitVectorFromFirstParameter
(
    IBitVector * pBitVector,
    Compiler * pCompiler
)
{
    SymbolEntryFunction;
    ThrowIfNull(pBitVector);
    ThrowIfNull(pCompiler);

    if (IsGeneric() && IsExtensionMethod() && GetFirstParam())
    {
        ThrowIfNull(GetFirstParam()->GetType());
        HashSet<GenericParameter *> paramsReferencedByFirstArgument;
        GetFirstParam()->GetType()->BuildReferencedParameterSet(&paramsReferencedByFirstArgument);

        HashSetIterator<GenericParameter *> iterator(&paramsReferencedByFirstArgument);

        while (iterator.MoveNext())
        {
            GenericParameter * pParam = iterator.Current();

            ThrowIfNull(pParam);
            ThrowIfFalse(pParam->IsGenericMethodParam()); //Extension methods are not allowed inside generic classes.
            pBitVector->SetBit(pParam->GetPosition());
        }
    }
}

// ==================================
// returns string for type
// ==================================
STRING *BCSYM::GetErrorName(Compiler *pCompiler)
{
    SymbolEntryFunction;
    switch (GetKind())
    {
    case SYM_ArrayLiteralType:    
    case SYM_ArrayType:
        return PArrayType()->GetErrorName(pCompiler);

    case SYM_PointerType:
        return PPointerType()->GetErrorName(pCompiler);

    case SYM_NamedType:
        return PNamedType()->GetHashingName();

    case SYM_VoidType:
        return STRING_CONST(pCompiler, Void);
    case SYM_ExtensionCallLookupResult :
        return PExtensionCallLookupResult()->GetErrorName(pCompiler);
    case SYM_Param:
        return PParam()->GetName();
    case SYM_GenericTypeBinding:
    {
        CompilerProject * pProject = this->PNamedRoot()->GetContainingProject();
        if
        (
            pProject &&
            pProject->GetCompilerHost() &&
            pProject->GetCompilerHost()->GetFXSymbolProvider()->IsTypeAvailable(FX::GenericNullableType) &&
            BCSYM::AreTypesEqual
            (
                pProject->GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::GenericNullableType),
                this->PGenericBinding()->GetGeneric()
            )
        )
        {
            Compiler * pCompilerForProject = pProject->GetCompiler();

            StringBuffer buffer;

            this->GetBasicRep
            (
                pCompilerForProject,
                NULL,
                &buffer,
                NULL,
                NULL,
                false
            );

            return
                pCompilerForProject->AddStringWithLen
                (
                    buffer.GetString(),
                    buffer.GetStringLength()
                );
        }

    }
    __fallthrough;
    default:
        if (IsNamedRoot())
        {
            return IsProc() && PProc()->IsAnyConstructor() ? pCompiler->TokenToString(tkNEW) : PNamedRoot()->GetName();
        }
        else
        {
            VSFAIL("unexpected");
            return NULL;
        }
    }
}

//
// Hash_Iterator
//

/***
*PUBLIC Hash_Iterator::Init()
*Purpose:
*    Initializes an iterator that iterates over all of the symbols
*    of a BCSYM_Hash
*
*Entry:
*    pHash - the symbol to iterate over
*
*Exit:
*    None.
*
*Error:
*    None.
*
***********************************************************************/

void BCITER_HASH_CHILD::Init(BCSYM_Hash *phash)
{
    m_phash = phash;

    m_ihash = 0;
    m_pnamedNext = 0;
}

//============================================================================
// The next element in the hash table.
//============================================================================

BCSYM_NamedRoot* BCITER_HASH_CHILD::GetNext()
{
    // No hash table, nothing to iterate over.
    if (m_phash == NULL)
    {
        return NULL;
    }

    // Loop over the buckets until we find something.
    while (m_pnamedNext == NULL)
    {
        // There are no buckets left.  Find the next hash table.
        if (m_ihash >= m_phash->CBuckets())
            return NULL;

        m_pnamedNext = m_phash->Rgpnamed()[m_ihash++];
    }

    // Return what we have and remember the next one.
    BCSYM_NamedRoot *pnamed = m_pnamedNext;
    m_pnamedNext = pnamed->GetNextInHash();

    return pnamed;
}


//
// BCITER_CHILD
//

/***
*PUBLIC BCITER_CHILD::Init()
*Purpose:
*    Initializes an iterator that iterates over all of the children
*    of a symbol.
*
*Entry:
*    psym - the symbol to iterate over
*
*Exit:
*    None.
*
*Error:
*    None.
*
***********************************************************************/

void BCITER_CHILD::Init(BCSYM_NamedRoot *pnamed, bool fBindToUnBindableMembers, bool fLookOnlyInCurrentPartialType, bool fIncludePartialContainers)
{
    BCSYM_Container *pcontainer;

    if (!pnamed)
    {
        m_pcontainer = 0;
        m_CurrentHash = 0;

        m_pnamedNext = 0;
        return;
    }

    if (pnamed->IsContainer())
    {
        pcontainer = pnamed->PContainer();
    }
    else
    {
        BCSYM_Hash *phash = pnamed->DigThroughAlias()->PHash();
        pcontainer = phash->GetContainer();

        if (phash->IsUnBindableChildrenHash())
        {
            fBindToUnBindableMembers = true;
        }
    }

    if (!fLookOnlyInCurrentPartialType &&
        pcontainer->IsPartialTypeAndHasMainType())
    {
        m_pcontainer = pcontainer->GetMainType();
    }
    else
    {
        m_pcontainer = pcontainer;
    }


    VSASSERT( m_pcontainer, "Expected Non-NULL container!!!");

    m_fBindToUnBindableMembers = fBindToUnBindableMembers;
    m_fLookOnlyInCurrentPartialType = fLookOnlyInCurrentPartialType;

    InitHashIterator();

    m_pnamedNext = 0;
    m_fIncludePartialContainers = fIncludePartialContainers;
}

void BCITER_CHILD::InitWithFile(CompilerFile *pfile, bool fIncludePartialContainers)
{
    m_pcontainer = 0;
    m_CurrentHash = 0;

    m_pnamedNext = pfile->GetNamespaceLevelSymbolList()->GetFirst();

    m_fIncludePartialContainers = fIncludePartialContainers;
}

void BCITER_CHILD::InitHashIterator()
{
    BCSYM_Hash *LookupHash;

    if (m_fBindToUnBindableMembers)
    {
        LookupHash = m_pcontainer->GetUnBindableChildrenHash();
    }
    else
    {
        LookupHash = m_pcontainer->GetHash();
    }

    if (!m_fLookOnlyInCurrentPartialType)
    {
        // Find the first hash in the list of lookup hashes
        // for the partial types
        //
        while (LookupHash->GetPrevHash())
        {
            LookupHash = LookupHash->GetPrevHash();
        }
    }

    m_CurrentHash = LookupHash;
    m_phashIterator.Init(m_CurrentHash);
}

//============================================================================
// The next element in the hash table.
//============================================================================

BCSYM_NamedRoot* BCITER_CHILD::GetNext()
{
    // No hash table, nothing to iterate over.
    if (m_pcontainer == NULL)
    {
        // We may be interating over a SymbolList if InitWithFile() was called. In this
        // case, isolate iteration over a hash table from the SymbolList. The SymbolList
        // cannot have hash tables in it.
        if (m_pnamedNext)
            return GetNextFromList();
        else
            return NULL;
    }

    BCSYM_NamedRoot *pnamed = NULL;
    do
    {
        // Look in other partial components of the given container
        // if asked to do so
        //
        while (!(pnamed = m_phashIterator.GetNext()) &&
                m_CurrentHash &&
                !m_fLookOnlyInCurrentPartialType)
        {
            m_CurrentHash = m_CurrentHash->GetNextHash();

            m_phashIterator.Init(m_CurrentHash);
        }

    // Skip partial containers found if asked to do so
    //
    }
    while (pnamed &&
           pnamed->IsContainer() &&
           !m_fIncludePartialContainers &&
           pnamed->PContainer()->IsPartialTypeAndHasMainType());

    return pnamed;
}

//============================================================================
// The next element in the list.
//============================================================================
BCSYM_NamedRoot *BCITER_CHILD::GetNextFromList(void)
{
    if(!m_pnamedNext)
        return NULL;

    BCSYM_NamedRoot *pnamed;
    while (pnamed = m_pnamedNext->PNamedRoot())
    {
        m_pnamedNext = m_pnamedNext->GetNextInHash();

        BCSYM *pAliasedSymbol = pnamed->DigThroughAlias();
        if (!pAliasedSymbol->IsContainer() ||
            m_fIncludePartialContainers ||
            !pAliasedSymbol->PContainer()->IsPartialTypeAndHasMainType())
        {
            break;
        }
    }

    return pnamed;
}


void BCITER_CHILD_ALL::Reset()
{
    /* Microsoft: I suppose this Assert was considered nice because it means we can validate that a type has members we will be able to import.
       But the problem is that Asserting GetHash() begs the question because it has the side-effect of calling EnsureChildrenLoaded() which creates
       the very hash we are Asserting for.  It is bad to create a difference between debug/retail with regards to what metadata we have imported for a type.
       And we often call this function when the type has no hash table yet, i.e. GetHashRaw() will often be null because we call this routine
       when creating a new iterator.  The iterator (during Init() below) actually creates the hash 
    VSASSERT(m_Container->GetHash(), "Container with no hash!!!"); */

    m_MembersIter.Init(
        m_Container,
        false,  //Don't bind to UnBindable members
        m_fLookOnlyInCurrentPartialType,
        m_fIncludePartialContainers);

    m_CurrentlyBindingToBindableMembers = true;
}


BCSYM_NamedRoot *BCITER_CHILD_ALL::GetNext(void)
{

    BCSYM_NamedRoot *Member = m_MembersIter.GetNext();

    if (!Member)
    {
        if (m_CurrentlyBindingToBindableMembers)
        {
            // May not be present for some containers like namespaces, etc.
            // or even for some containers built up during CDebugParseExpression lookup
            // processing.
            //
            if (m_Container->GetUnBindableChildrenHash())
            {
                m_MembersIter.Init(
                    m_Container,
                    true, // Bind to UnBindable members
                    m_fLookOnlyInCurrentPartialType,
                    m_fIncludePartialContainers);

                Member = m_MembersIter.GetNext();
            }

            m_CurrentlyBindingToBindableMembers = false;
        }
    }

    return Member;
}


unsigned BCITER_CHILD_ALL::GetCount()
{
    BCSYM_Container *Container;
    if (m_Container && m_Container->IsPartialTypeAndHasMainType())
    {
        Container = m_Container->GetMainType();
    }
    else
    {
        Container = m_Container;
    }

    unsigned Count = 0;
    while (Container)
    {
        if (Container->GetHash())
        {
            Count += Container->GetHash()->CSymbols();
        }

        if (Container->GetUnBindableChildrenHash())
        {
            Count += Container->GetUnBindableChildrenHash()->CSymbols();
        }

        Container = Container->GetNextPartialType();
    }

    return Count;
}

//
// BCITER_Handles
//

void BCITER_Handles::Reset()
{
    m_pCurrentMethod = m_pMethod;
    m_pCurrentHandles = NULL;
}

BCSYM_HandlesList* BCITER_Handles::GetNext()
{
    if( m_pCurrentMethod == NULL )
    {
        return( NULL );
    }

    if( m_pCurrentHandles == NULL )
    {
        m_pCurrentHandles = m_pCurrentMethod->GetHandlesList();
    }
    else
    {
        m_pCurrentHandles = m_pCurrentHandles->GetNextHandles();
    }

    // In the case of partial methods, we want an iteration on this method
    // to consider handles clauses from the associated method.

    if( m_pCurrentHandles == NULL &&
        m_pCurrentMethod == m_pMethod &&
        !m_fLookOnlyInCurrentMethod
        )
    {
        if( m_pCurrentMethod->ParticipatesInPartialMethod() )
        {
            m_pCurrentMethod = m_pCurrentMethod->GetAssociatedMethod()->PMethodImpl();
        }
        else
        {
            m_pCurrentMethod = NULL;
        }

        return( GetNext() );
    }

    return( m_pCurrentHandles );
}

//
// BCITER_ImplInterfaces
//

/***
*PUBLIC BCITER_ImplInterfaces::Init()
*Purpose:
*    Initializes an iterator that iterates over all of the class's
*    implemented interfaces.
*
*Entry:
*    pmod - the class symbol.
*
*Exit:
*    None.
*
*Error:
*    None.
*
***********************************************************************/


void BCITER_ImplInterfaces::Init(BCSYM_Container *pClassOrInterface, bool fLookOnlyInCurrentPartialType)
{
    if (!pClassOrInterface)
    {
        m_pInitialClassOrInterface = NULL;
        m_pCurrentClassOrInterface = NULL;

        return;
    }

    VSASSERT( pClassOrInterface->IsClass() || pClassOrInterface->IsInterface(),
                    "Expected only a class or an interface!!!");

    if (!fLookOnlyInCurrentPartialType &&
        pClassOrInterface->IsPartialTypeAndHasMainType())
    {
        m_pInitialClassOrInterface = pClassOrInterface->GetMainType();
    }
    else
    {
        m_pInitialClassOrInterface = pClassOrInterface;
    }

    if (m_pInitialClassOrInterface->IsClass())
    {
        m_----lCur = m_pInitialClassOrInterface->PClass()->GetFirstImplements();
    }
    else
    {
        m_----lCur = m_pInitialClassOrInterface->PInterface()->GetFirstImplements();
    }

    m_pCurrentClassOrInterface = m_pInitialClassOrInterface;
    m_fLookOnlyInCurrentPartialType = fLookOnlyInCurrentPartialType;
}

void BCITER_ImplInterfaces::Reset()
{
    Init(m_pInitialClassOrInterface, m_fLookOnlyInCurrentPartialType);
}

/***
*PUBLIC BCITER_ImplInterfaces::GetNext()
*Purpose:
*    Get the next implemented interface symbol.
*
*Entry:
*    None.
*
*Exit:
*    returns the Implements symbol, or NULL if no more interfaces exist
*      in this class.
*
*Error:
*    None.
*
***********************************************************************/

BCSYM_Implements *BCITER_ImplInterfaces::GetNext()
{

    if (!m_pCurrentClassOrInterface)
    {
        return NULL;
    }

    while (!m_----lCur &&
           !m_fLookOnlyInCurrentPartialType &&
           (m_pCurrentClassOrInterface =
                m_pCurrentClassOrInterface->GetNextPartialType()))
    {
        VSASSERT( m_pCurrentClassOrInterface->IsClass(),
                        "How can a partial type be a non-class ?");

        m_----lCur = m_pCurrentClassOrInterface->PClass()->GetFirstImplements();
    }

    BCSYM_Implements *----l = m_----lCur;
    if (----l)
    {
        m_----lCur = ----l->GetNext();
        return ----l;
    }

    return ----l;
}

bool BCITER_ImplInterfaces::HasImplInterfaces(BCSYM_Container *pClassOrInterface)
{
    VSASSERT( pClassOrInterface->IsClass() || pClassOrInterface->IsInterface(),
                    "Expected only a class or an interface!!!");

    BCITER_ImplInterfaces TempIter(pClassOrInterface);

    return TempIter.GetNext();
}

//
// BCITER_Parameters
//

/***
*PUBLIC BCITER_Parameters::Init()
*Purpose:
*    Initializes an iterator that iterates over all of the specified
*    proc's declared parameters, in order from left to right.  Me and
*    other hidden parameters are not included in the iteration.
*
*Entry:
*    psymProc - the proc whose parameters are to be iterated over.
*
*Exit:
*    None.
*
*Error:
*    None.
*
***********************************************************************/

void BCITER_Parameters::Init(BCSYM_Proc *pproc)
{
    m_pparamFirst = pproc->GetFirstParam();
    m_pparamCur = m_pparamFirst;
}

/***
*PUBLIC BCITER_Parameters::PsymNext()
*Purpose:
*    Get the next parameter in the procedure.
*
*Entry:
*    None.
*
*Exit:
*    returns the parameter symbol, or NULL if no more parameters.
*
*Error:
*    None.
*
***********************************************************************/

BCSYM_Param *BCITER_Parameters::PparamNext()
{
    BCSYM_Param *pparamRet = m_pparamCur;

    if (m_pparamCur != NULL)
    {
        m_pparamCur = pparamRet->GetNext();
    }

    return pparamRet;
}

/***
*PUBLIC BCSYM_Param::operator[]( size_t index )
*Purpose:
*   Get a selected function parameter.
*
*Entry:
*   None.
*
*Exit:
*   Returns the parameter symbol, or NULL if no more parameters.  This
*   function will adjust the iterator to the requested item if it's
*   found or NULL if it's not found.
*
*Error:
*    None.
*
***********************************************************************/

BCSYM_Param* BCITER_Parameters::operator[]( size_t index )
{
    m_pparamCur = m_pparamFirst;
    size_t c = 0;

    while ( (c != index) && (m_pparamCur != NULL) )
    {
        m_pparamCur = m_pparamCur->GetNext();
        c++;
    }

    return m_pparamCur;
}

void BCITER_ApplAttrs::Init(BCSYM *pSymbolHavingAttrs, bool fLookOnlyInImmediateSymbol)
{
    VSASSERT( pSymbolHavingAttrs &&
              (pSymbolHavingAttrs->IsNamedRoot() || pSymbolHavingAttrs->IsParam()),
                    "Unexpected symbol kind: cannot have attributes!!!");

    m_fLookOnlyInImmediateSymbol = fLookOnlyInImmediateSymbol;

    if (!fLookOnlyInImmediateSymbol &&
        pSymbolHavingAttrs->IsContainer() &&
        pSymbolHavingAttrs->PContainer()->IsPartialTypeAndHasMainType())
    {
        m_pSymbolHavingAttrs = pSymbolHavingAttrs->PContainer()->GetMainType();
    }
    else
    {
        m_pSymbolHavingAttrs = pSymbolHavingAttrs;
    }

    // Store the original symbol so that we do not infinitely loop in the
    // case of partial methods and their parameters.

    m_pOriginalSymbol = m_pSymbolHavingAttrs;

    m_pNextApplAttr = NULL;
}

BCSYM_ApplAttr* BCITER_ApplAttrs::GetNext()
{
    if (!m_pSymbolHavingAttrs)
    {
        return NULL;
    }

    if (!m_pNextApplAttr)
    {
        if (m_pSymbolHavingAttrs->GetPAttrVals())
        {
            m_pNextApplAttr =
                m_pSymbolHavingAttrs->GetPAttrVals()->GetPsymApplAttrHead();
        }
        else
        {
            m_pNextApplAttr= NULL;
        }
    }
    else
    {
        m_pNextApplAttr = m_pNextApplAttr->GetNext();
    }

    if (!m_fLookOnlyInImmediateSymbol &&
        !m_pNextApplAttr)
    {
        if (m_pSymbolHavingAttrs->IsContainer())
        {
            m_pSymbolHavingAttrs =
                m_pSymbolHavingAttrs->PContainer()->GetNextPartialType();

            return GetNext();
        }

        // If we have partial methods or their parameters, we need to
        // iterate the attributes on their corresponding symbol.

        else if (
            m_pSymbolHavingAttrs->IsMethodImpl() &&
            m_pSymbolHavingAttrs->PMethodImpl()->ParticipatesInPartialMethod() &&
            m_pSymbolHavingAttrs == m_pOriginalSymbol
            )
        {
            m_pSymbolHavingAttrs =
                m_pSymbolHavingAttrs->PMethodImpl()->GetAssociatedMethod();

            return GetNext();
        }
        else if (
            m_pSymbolHavingAttrs->IsParam() &&
            m_pSymbolHavingAttrs->PParam()->IsPartialMethodParam() &&
            m_pSymbolHavingAttrs == m_pOriginalSymbol
            )
        {
            m_pSymbolHavingAttrs =
                m_pSymbolHavingAttrs->PParam()->GetAssociatedParam();

            return GetNext();
        }
        else
        {
            return NULL;
        }
    }

    return m_pNextApplAttr;
}

BCSYM_NamedRoot* BCITER_ApplAttrs::GetNamedContextForCurrentApplAttr()
{
    if (!m_pNextApplAttr)
    {
        return NULL;
    }

    VSASSERT( m_pSymbolHavingAttrs && m_pSymbolHavingAttrs->GetPAttrVals(),
                    "How can this be NULL when an attribute has been returned ?");

    VSASSERT( m_pSymbolHavingAttrs->IsParam() || m_pSymbolHavingAttrs->IsNamedRoot(),
                    "How can any other kind of symbol have attributes ?");

    if (m_pSymbolHavingAttrs->IsParam())
    {
        return m_pSymbolHavingAttrs->GetPAttrVals()->GetPsymContextOfParamWithApplAttr();
    }
    else
    {
        return m_pSymbolHavingAttrs->PNamedRoot();
    }
}

//
// BCITER_CHILD_SAFE
//


/***
*PUBLIC __BCITER_CHILD_SAFE::Init()
*Purpose:
*    Initializes an iterator that iterates over all of the children
*    of a symbol.  This iterator makes a copy of all of the children
*    and iterates over it so we won't die if the children change.
*
*Entry:
*    psym - the symbol to iterate over
*
*Exit:
*    None.
*
*Error:
*    None.
*
***********************************************************************/

BCITER_CHILD_SAFE::BCITER_CHILD_SAFE(BCSYM_NamedRoot *psym, bool ShouldInit) :
    m_cSyms(0),
    m_iSym(0),
    m_rgpnamed(NULL),
    m_fAllocated(false)
{
    if (ShouldInit)
    {
        Init(psym);
    }
}

BCITER_CHILD_SAFE::~BCITER_CHILD_SAFE()
{
    if (m_rgpnamed && m_fAllocated)
    {
        VBFree(m_rgpnamed);
    }
}

void BCITER_CHILD_SAFE::Init(BCSYM_NamedRoot *psym)
{
    if (m_rgpnamed && m_fAllocated)
    {
        VBFree(m_rgpnamed);
        m_rgpnamed = NULL;
    }

    if (psym)
    {
        VSASSERT( psym->IsHash() || psym->IsContainer(), "unexpected container kind!!!");
        AssertIfTrue(m_rgpnamed);

        m_cSyms = CountSymbols(psym);
        if (m_cSyms > CHILD_ITER_BUF_COUNT)
        {    
            m_rgpnamed = VBAllocator::AllocateArray<BCSYM_NamedRoot*>(m_cSyms);
            m_fAllocated = true;
        }
        else
        {
            m_rgpnamed = m_Syms;
            m_fAllocated = false;
        }
        
        unsigned iSym = 0;
        BCSYM_NamedRoot *pnamed;

        if (psym->IsHash())
        {
            // For each symbol, plop it into the array.

            BCITER_HASH_CHILD hashChildren(psym->PHash());
            while ((pnamed = hashChildren.GetNext()) && iSym < m_cSyms)
            {
                m_rgpnamed[iSym++] = pnamed;
            }
        }
        else
        {
            // For each symbol, plop it into the array.

            BCITER_CHILD bichild(psym->PContainer());
            while ((pnamed = bichild.GetNext()) && iSym < m_cSyms)
            {
                m_rgpnamed[iSym++] = pnamed;
            }
        }

        m_cSyms = iSym;
        m_iSym = 0;
    }
    else
    {
        m_cSyms = 0;
        m_iSym = 0;
    }
}

BCSYM_NamedRoot * BCITER_CHILD_SAFE::GetNext()
{
    BCSYM_NamedRoot *pnamed;

    // The allocation did not fail, iterate over the sorted array.
    // Skip over NULL entries (they got this way by a call to OmitPrevChild).
    while (m_iSym < m_cSyms)
    {
        pnamed = m_rgpnamed[m_iSym++];
        if (pnamed != NULL)
            return pnamed;
    }

    return NULL;
}

/***
* PUBLIC __BILITER_CHILD_SAFE::Reset()
* Purpose:
*   Resets the iteration back to the beginning.  Any symbols
*   previously omitted by OmitPrevChild will now not be returned
*   by PnamedNext.
*************************************************************************/
void BCITER_CHILD_SAFE::Reset()
{
    m_iSym = 0;
}

unsigned BCITER_CHILD_SAFE::CountSymbols(BCSYM_NamedRoot *pnamedHash)
{
    unsigned cSymbols = 0;
    BCSYM_Hash *pHash;

    pnamedHash = pnamedHash->DigThroughAlias()->PNamedRoot();

    if (pnamedHash->IsContainer())
    {
      // Get the hash table to iterate over.
      if (pnamedHash->PContainer()->IsPartialTypeAndHasMainType())
      {
          pHash = pnamedHash->PContainer()->GetMainType()->GetHash();
      }
      else
      {
          pHash = pnamedHash->PContainer()->GetHash();
      }
      pnamedHash = pHash->PNamedRoot();
    }

    while (pnamedHash)
    {
        IfFalseThrow((cSymbols += pnamedHash->DigThroughAlias()->PHash()->CSymbols()) >= pnamedHash->DigThroughAlias()->PHash()->CSymbols());

        pnamedHash = pnamedHash->PHash()->GetNextHash();
    }

    return cSymbols;
}


BCITER_CHILD_SORTED::BCITER_CHILD_SORTED(BCSYM_NamedRoot *psym) :
    BCITER_CHILD_SAFE(psym, false)
{
    Init(psym);
}

//
// Iterate over all of the children of a symbol in name order.
//

/***
*BCITER_CHILD_SORTED::Init()
*Purpose:
*    Initializes an iterator that iterates over all of the children
*    of a symbol, sorted by the name of the symbols.  Symbols without
*    names are not included.
*
*Entry:
*    psym - the symbol to iterate over
*
*Exit:
*    returns TRUE if we can iterate.
*
*Error:
*    None.
*
***********************************************************************/

void BCITER_CHILD_SORTED::Init(BCSYM_NamedRoot *psym)
{
    // Run the base initializer to set everything up.
    BCITER_CHILD_SAFE::Init(psym);

    // Sort the array.
    if (m_cSyms > 1)
    {
        qsort(m_rgpnamed, m_cSyms, sizeof(BCSYM_NamedRoot *), SortSymbolsByNameAndLocation);
    }

    // Ignore the elements on the end of the array that have
    // no name.
    //
    {
        unsigned iName = m_cSyms;

        while (iName > 0 && !m_rgpnamed[iName - 1]->GetName())
            iName--;

        m_cSyms = iName;
    }
}

void BCITER_Classes::Init( CompilerFile *pFile, bool fIncludePartialContainers )
{
    if( pFile->GetCompState() < CS_Declared )
      m_pNext = NULL;
    else
      m_pNext = pFile->GetNamespaceLevelSymbolList()->GetFirst();

    m_fIncludePartialContainers = fIncludePartialContainers;
}

BCSYM_Class *BCITER_Classes::Next()
{
    BCSYM_NamedRoot *pNext = m_pNext;
    while( pNext )
    {
        BCSYM *pAliasedSymbol = pNext->DigThroughAlias();

        if (pAliasedSymbol->IsClass() &&
            (m_fIncludePartialContainers || !pAliasedSymbol->PClass()->IsPartialTypeAndHasMainType()))
        {
            break;
        }

        pNext = pNext->GetNextInSymbolList();
    }

    if( pNext )
    {
        m_pNext = pNext->GetNextInSymbolList();
        return pNext->DigThroughAlias()->PClass();
    }
    else
    {
        m_pNext = NULL;
        return NULL;
    }
}

void BCITER_Constraints::Init
(
    BCSYM_GenericParam *pGenericParam,
    bool fDigThroughTypeParamTypeConstraints,
    Compiler *pCompiler
)
{
    memset(this, 0, sizeof(BCITER_Constraints));

    m_pGenericParam = pGenericParam;
    m_fDigThroughTypeParamTypeConstraints = fDigThroughTypeParamTypeConstraints;
    m_pCompiler = pCompiler;

    m_ppConstraintsInProgress = m_ppConstraintsInProgressBuffer;
    m_MaxAllowedConstraintsInProgress = DIM(m_ppConstraintsInProgressBuffer);
    m_CountOfConstraintsInProgress = 0;

    if (m_pGenericParam)
    {
        VSASSERT(!fDigThroughTypeParamTypeConstraints ||
                 !m_pGenericParam->GetContainer()->GetCompilerFile() ||
                 m_pGenericParam->GetContainer()->GetCompilerFile()->IsMetaDataFile() ||
                 pGenericParam->IsConstraintCycleCheckingDone() ||
                 (pGenericParam->GetContainer() && pGenericParam->GetContainer()->IsBindingDone()), // For UI (Intellisense) - for generic params on partial types.
                    "Digging into type parameter type constraints unexpected before constraint cycle checking!!!");

        m_pNextConstraint = m_pGenericParam->GetConstraints();
    }
}

void BCITER_Constraints::PushIntoConstraintsInProgressList
(
    BCSYM_GenericConstraint *pConstraint
)
{
    if (m_CountOfConstraintsInProgress == m_MaxAllowedConstraintsInProgress)
    {
        m_MaxAllowedConstraintsInProgress = VBMath::Multiply<unsigned int>(2,m_MaxAllowedConstraintsInProgress);

        BCSYM_GenericConstraint **ppConstraints = 
            VBAllocator::AllocateArray<BCSYM_GenericConstraint*>(m_MaxAllowedConstraintsInProgress);
        memcpy(ppConstraints, m_ppConstraintsInProgress, VBMath::Multiply(m_CountOfConstraintsInProgress,sizeof(BCSYM_GenericConstraint *)));

        FreeConstraintsInProgressList();
        m_ppConstraintsInProgress = ppConstraints;
    }

    m_ppConstraintsInProgress[m_CountOfConstraintsInProgress++] = pConstraint;
}

BCSYM_GenericConstraint* BCITER_Constraints::PopFromConstraintsInProgressList()
{
    if (ConstraintsInProgress())
    {
        return m_ppConstraintsInProgress[--m_CountOfConstraintsInProgress];
    }

    return NULL;
}

void BCITER_Constraints::FreeConstraintsInProgressList()
{
    // Free the constraints in progress list, but only if it has been allocated on the heap
    //
    if (m_ppConstraintsInProgress != m_ppConstraintsInProgressBuffer)
    {
        VBFree(m_ppConstraintsInProgress);
        m_ppConstraintsInProgress = NULL;
    }
}

BCSYM_GenericConstraint* BCITER_Constraints::Next()
{
    while (true)
    {
        // Skip bad constraints to avoid walking cycles.
        //
        while (m_pNextConstraint && m_pNextConstraint->IsBadConstraint())
        {
            m_pNextConstraint = m_pNextConstraint->Next();
        }

        if (!m_pNextConstraint)
        {
            if (ConstraintsInProgress())
            {
                VSASSERT(m_fDigThroughTypeParamTypeConstraints, "Inconsistency in constraint iterator!!!");

                m_pNextConstraint = PopFromConstraintsInProgressList();

                VSASSERT(m_pNextConstraint, "Null constraint unexpected!!!");
                m_pNextConstraint = m_pNextConstraint->Next();
                continue;
            }

            return NULL;
        }

        if (m_fDigThroughTypeParamTypeConstraints &&
            m_pNextConstraint->IsGenericTypeConstraint() &&
            m_pNextConstraint->PGenericTypeConstraint()->GetType()->IsGenericParam())
        {
            PushIntoConstraintsInProgressList(m_pNextConstraint);
            m_pNextConstraint =
                m_pNextConstraint->PGenericTypeConstraint()->GetType()->PGenericParam()->GetConstraints();
            continue;
        }

        BCSYM_GenericConstraint *Result = m_pNextConstraint;
        m_pNextConstraint = m_pNextConstraint->Next();

        return Result;
    }

    VSFAIL("Unexpected code path!!!");
    return NULL;
}

bool
ConstantValue::Equals
(
    const ConstantValue &Left,
    const ConstantValue &Right
)
{
    if (Left.TypeCode != Right.TypeCode)
    {
        return false;
    }

    switch (Left.TypeCode)
    {
        case t_bool:

            // VS #236672 (Microsoft):  because VB7 thinks true is 1 and VB6 thinks it is -1, we
            // have to accomodate both.
            return ( Left.Integral != 0 ) == ( Right.Integral != 0 );

        case t_i1:
        case t_ui1:
        case t_i2:
        case t_ui2:
        case t_i4:
        case t_ui4:
        case t_i8:
        case t_ui8:
        case t_date:
        case t_char:

            return Left.Integral == Right.Integral;

        case t_single:

            return Left.Single == Right.Single;

        case t_double:

            return Left.Double == Right.Double;

        case t_decimal:

            return VarDecCmp((LPDECIMAL)&Left.Decimal, (LPDECIMAL)&(Right.Decimal)) == static_cast<HRESULT>(VARCMP_EQ);

        case t_ref:

            // The only possible object value is Nothing.
            return true;

        case t_string:

            // A string constant always has a spelling, unless it is Nothing.

            if (Left.String.Spelling == NULL || Right.String.Spelling == NULL)
            {
                return (Left.String.Spelling == Right.String.Spelling);
            }
            else
            {
                return wcscmp( Left.String.Spelling, Right.String.Spelling) == 0;
            }
    }

    return false;
}

//
// Copy the current constant into the destination
//
void ConstantValue::CopyConstant
(
    ConstantValue *pDestination,  // Destination constant
    NorlsAllocator *pAllocator    // Allocator to use if it's a string
)
{
    *pDestination = *this;
    // Microsoft 9/8/2004:  Added some overflow checks.
    if (TypeCode == t_string && String.Spelling && String.LengthInCharacters+1 > String.LengthInCharacters)
    {
        size_t SizeOfString = VBMath::Multiply((String.LengthInCharacters+1), sizeof(WCHAR));
        WCHAR * pTmp = (WCHAR *)pAllocator->Alloc(SizeOfString);
        memcpy(pTmp, String.Spelling, SizeOfString);
        pDestination->String.Spelling = pTmp;
    }
}

//-------------------------------------------------------------------------------------------------
//
// Create a constant based on the passed in variant
//
//-------------------------------------------------------------------------------------------------
ConstantValue::ConstantValue( _In_ Compiler* pCompiler, _In_ const VARIANT& vt)
{
    switch ( vt.vt)
    {
    case VT_BOOL:
        Integral = VARIANT_TRUE == vt.boolVal ? -1 : 0;
        TypeCode = t_bool;
        break;
    case VT_I1:
        Integral = vt.cVal;
        TypeCode = t_i1;
        break;
    case VT_I2:
        Integral = vt.iVal;
        TypeCode = t_i2;
        break;
    case VT_I4:
        Integral = vt.intVal;
        TypeCode = t_i4;
        break;
    case VT_I8:
        Integral = vt.llVal;
        TypeCode = t_i8;
        break;
    case VT_UI1:
        Integral = vt.bVal;
        TypeCode = t_ui1;
        break;
    case VT_UI2:
        Integral = vt.uiVal;
        TypeCode = t_ui2;
        break;
    case VT_UI4:
        Integral = vt.uintVal;
        TypeCode = t_ui4;
        break;
    case VT_UI8:
        Integral = vt.ulVal;
        TypeCode = t_ui8;
        break;
    case VT_DECIMAL:
        Decimal = vt.decVal;
        TypeCode = t_decimal;
        break;
    case VT_R4:
        Single = vt.fltVal;
        TypeCode = t_single;
        break;
    case VT_R8:
        Double = vt.dblVal;
        TypeCode = t_double;
        break;
    case VT_DATE:
        Integral = vt.llVal;
        TypeCode = t_date;
        break;
    case VT_BSTR:
        String.Spelling = pCompiler->AddString(vt.bstrVal);
        String.LengthInCharacters = ::SysStringLen(vt.bstrVal);
        TypeCode = t_string;
        break;
    default:
        VSFAIL("Unrecognized constant variant");
        Integral = 0;
        TypeCode = t_bool;
        break;
    }
}

//
//  Fill in the Variant from the Constant
//
void ConstantValue::VariantFromConstant(VARIANT &VariantValue)
{
    BSTR bstrConstantString = NULL;

    switch (TypeCode)
    {
    case t_bool:
        VariantValue.vt = VT_BOOL;
        VariantValue.boolVal = (VARIANT_BOOL)Integral;
        break;
    case t_i1:
        VariantValue.vt = VT_I1;
        VariantValue.cVal = (CHAR)Integral;
        break;
    case t_ui1:
        VariantValue.vt = VT_UI1;
        VariantValue.bVal = (BYTE)Integral;
        break;
    case t_i2:
        VariantValue.vt = VT_I2;
        VariantValue.iVal = (SHORT)Integral;
        break;
    case t_ui2:
        VariantValue.vt = VT_UI2;
        VariantValue.uiVal = (USHORT)Integral;
        break;
    case t_i4:
        VariantValue.vt = VT_I4;
        VariantValue.lVal = (INT)Integral;
        break;
    case t_ui4:
        VariantValue.vt = VT_UI4;
        VariantValue.ulVal = (UINT)Integral;
        break;
    case t_i8:
        VariantValue.vt = VT_I8;
        VariantValue.llVal = (LONGLONG)Integral;
        break;
    case t_ui8:
        VariantValue.vt = VT_UI8;
        VariantValue.ullVal = (ULONGLONG)Integral;
        break;
    case t_decimal:
        // !!WARNING!! - Set the VARTYPE *after* the DECIMAL value because they share the same space due to a union.
        VariantValue.decVal = Decimal;
        VariantValue.vt = VT_DECIMAL;
        break;
    case t_single:
        VariantValue.vt = VT_R4;
        VariantValue.fltVal = Single;
        break;
    case t_double:
        VariantValue.vt = VT_R8;
        VariantValue.dblVal = Double;
        break;
    case t_date:
        VariantValue.vt = VT_DATE;
        VariantValue.dblVal = 0.0;  // OleAut's zero'ed date value.

        if (Integral != 0)
        {
            //__int64 TempIntegral = Integral;

            // The following segment of code seems to be transforming the input date value wrong
            // so it is commented out.  Related to VS Whidbey #216992
            /*
            if (TempIntegral < 864000000000) // This is a fix for VB. They want the default day to be 1/1/0001 rathar then 12/30/1899.
                TempIntegral += 599264352000000000; // We could have moved this fix down but we would like to keep the bounds check.

            if (TempIntegral >= 31241376000000000)
            {
                // Currently, our max date == OA's max date (12/31/9999), so we don't
                // need an overflow check in that direction.
                long millis = (int)(TempIntegral / 10000 - 599264352000000000 / 10000);
                if (millis < 0) {
                    long frac = millis % 86400000;
                    if (frac != 0) millis -= (86400000 + frac) * 2;
                }
                VariantValue.dblVal = (double)millis / 86400000;
            }*/

            // Fix for VS Whidbey #216992.  Input value is correct, it does not need to be transformed.
            VariantValue.llVal = Integral;
        }
        break;
    case t_char:
        // Emit char as a UI2, the signature will have the actual type
        VariantValue.vt = VT_UI2;
        VariantValue.uiVal = (unsigned short)Integral;
        break;
    case t_string:
        if (String.Spelling)
        {
            bstrConstantString = SysAllocString(String.Spelling);
        }
        else
        {
            bstrConstantString = SysAllocString(L"");
        }
        IfNullThrow(bstrConstantString);
        VariantValue.bstrVal = bstrConstantString;
        VariantValue.vt = VT_BSTR;
        break;
    case t_ref:
        VSASSERT(Integral == 0, "invalid state");
        VariantValue.vt = VT_I4;
        VariantValue.lVal = 0;
        break;
    default:
        VSFAIL("Invalid state");
        VariantValue.vt = VT_BOOL;
        VariantValue.boolVal = false;
        break;
    }
}


bool BCSYM_Property::ReturnsEventSource( void )
{
    SymbolEntryFunction;
    bool HasAttribute;
    if (GetPAttrVals()->GetPWellKnownAttrVals()->GetPersistContentsData( &HasAttribute ))
    {
        return HasAttribute;
    }
    return false;
}

SpellingInfoIterator::SpellingInfoIterator(SpellingInfo * pRoot) : m_pCurrent(pRoot)
{
}

SpellingInfo * SpellingInfoIterator::Next()
{
    SpellingInfo * pRet = m_pCurrent;

    if (m_pCurrent)
    {
        m_pCurrent = m_pCurrent->m_pNext;
    }

    return pRet;
}


SpellingInfo * BCSYM_Namespace::GetSpellings()
{
    SymbolEntryFunction;
    return m_pSpellingList;
}

void BCSYM_Namespace::AddSpelling(SpellingInfo * pSpelling)
{
    SymbolEntryFunction;
    VSASSERT(pSpelling, "An attempt was made to add a null spelling into the namespace's spelling list");
    if (pSpelling)
    {
        pSpelling->m_pNext = GetSpellings();
        m_pSpellingList = pSpelling;
    }
}

BCSYM_Namespace* BCSYM_Namespace::GetNextNamespaceInSameProject()
{
    SymbolEntryFunction;
    VSASSERT( this->GetCompilerFile(),
                "How can a namespace not be associated with any file ?");

    CompilerProject *CurrentProject =
        this->GetCompilerFile()->GetProject();

    BCSYM_Namespace *NextNamespace;
    for(NextNamespace = GetNextNamespace();
        NextNamespace && NextNamespace != this;
        NextNamespace = NextNamespace->GetNextNamespace())
    {
        VSASSERT( NextNamespace->GetCompilerFile(),
                    "How can a namespace not be associated with any file ?");

        if (NextNamespace->GetCompilerFile()->GetProject() == CurrentProject)
        {
            break;
        }
    }

    VSASSERT( NextNamespace,
                "How can a namespace ring end in a NULL Namespace ?");

    return NextNamespace;
}

void BCSYM_Namespace::SetImports( ImportedTarget *pImportsList )
{
    SymbolEntryFunction;
    m_pImportsList = pImportsList;
}


void BCSYM_Namespace::EnsureMemberNamesInAllModulesLoaded()
{
    SymbolEntryFunction;
    if (GetNamespaceRing()->GetNamesInAllModulesLoaded())
    {
        return;
    }

    BCSYM_Namespace *Current = this;

    do
    {
        if (!Current->m_NamesInAllModulesLoaded)
        {
            // Note that the names of module members in VB code are already loaded when the module's
            // hash is populated. So nothing to do for modules in VB code.
            //
            if (Current->GetCompilerFile() && Current->GetCompilerFile()->IsMetaDataFile())
            {
                for (BCSYM_Class *Module = Current->GetFirstModule(); Module; Module = Module->GetNextModule())
                {
                    MetaImport::ImportMemberNamesInModule(Module);
                }
            }

            Current->m_NamesInAllModulesLoaded = true;
        }

        Current = Current->GetNextNamespace();
    } while (Current != this);

    GetNamespaceRing()->SetNamesInAllModulesLoaded(true);
}

// Helper to check if a type is a valid attribute type
bool IsValidAttributeType(BCSYM *pType, CompilerHost *pCompilerHost)
{
    return ((pType->IsIntrinsicType() && pType->GetVtype() != t_decimal && pType->GetVtype() != t_date) ||
             pType == pCompilerHost->GetFXSymbolProvider()->GetType(FX::TypeType) ||
             pType->IsEnum() ||
             pType == pCompilerHost->GetFXSymbolProvider()->GetType(FX::ObjectType) ||
             // 1-D arrays are allowed
             (pType->IsArrayType() && pType->PArrayType()->GetRank() == 1 &&
                pType->PArrayType()->GetRoot() &&
                IsValidAttributeType(pType->PArrayType()->GetRoot(), pCompilerHost) &&
                !pType->PArrayType()->GetRoot()->IsArrayType()));
}

//============================================================================
// Returns a pointer to the symbol's AttrVals struct.  Vectors to the
// correct subclass's implementation (BCSYM_NamedRoot or BCSYM_Param).
//============================================================================
AttrVals *BCSYM::GetPAttrVals()
{
    SymbolEntryFunction;
    VSASSERT(IsNamedRoot() || IsParam(), "Must be BCSYM_NamedRoot or BCSYM_Param");

    return IsNamedRoot() ? PNamedRoot()->GetPAttrVals() : PParam()->GetPAttrVals();
}

//============================================================================
// Returns a pointer to the symbol's WellKnownAttrVals struct.  Vectors to the
// correct subclass's implementation (BCSYM_NamedRoot or BCSYM_Param).
//============================================================================
WellKnownAttrVals *BCSYM::GetPWellKnownAttrVals()
{
    SymbolEntryFunction;
    VSASSERT(IsNamedRoot() || IsParam(), "Must be BCSYM_NamedRoot or BCSYM_Param");

    return IsNamedRoot() ? PNamedRoot()->GetPWellKnownAttrVals() : PParam()->GetPWellKnownAttrVals();
}

//============================================================================
// Clears the pointer to the symbol's AttrVals struct.  Vectors to the
// correct subclass's implementation (BCSYM_NamedRoot or BCSYM_Param).
//============================================================================
void BCSYM::FreeAttrVals()
{
    SymbolEntryFunction;
    VSASSERT(IsNamedRoot() || IsParam(), "Must be BCSYM_NamedRoot or BCSYM_Param");

    if (IsNamedRoot())
    {
        PNamedRoot()->SetPAttrVals(NULL);
    }
    else
    {
        PParam()->SetPAttrVals(NULL);
    }
}

//============================================================================
// Allocates an AttrVals struct and attaches it to 'this' symbol.
//============================================================================
void BCSYM::AllocAttrVals
(
    NorlsAllocator   *pnra,
    CompilationState  state,        // State we're heading toward (CS_MAX if metadata file)
    SourceFile       *psourcefile,   // File containing this symbol (NULL if metadata file)
    BCSYM_Container  *pcontainercontext,
    bool AllocForWellKnownAttributesToo
)
{
    SymbolEntryFunction;
    AttrVals *pattrvals;

    VSASSERT(IsNamedRoot() || IsParam(), "Must be BCSYM_NamedRoot or BCSYM_Param");
    VSASSERT(GetPAttrVals() == NULL, "Already alloc'ed.");

    pattrvals = (AttrVals *)pnra->Alloc(sizeof(AttrVals));

    if (IsNamedRoot())
    {
        PNamedRoot()->SetPAttrVals(pattrvals);
    }
    else
    {
        PParam()->SetPAttrVals(pattrvals);
    }

    //
    // Need to keep track of what state we're in when we do the alloc so we
    // can do the right thing when we compile & decompile symbols with attributes.
    //
    switch (state)
    {
        case CS_Declared:
            // Add to the CS_Declared list rooted in psourcefile

            VSASSERT(pcontainercontext, "Must have a container context!");

            pattrvals->GetPNon----edData()->m_psymNextWithAttr = pcontainercontext->GetDeclaredAttrListHead();
            pcontainercontext->SetDeclaredAttrListHead(this);

            break;

        case CS_Bound:
            // Add to the CS_Bound list rooted in psourcefile

            VSASSERT(pcontainercontext, "Must have a container context!");
            VSASSERT(pcontainercontext->IsContainer(), "Expected a container!!!");

            pattrvals->GetPNon----edData()->m_psymNextWithAttr = pcontainercontext->GetBoundAttrListHead();
            pcontainercontext->SetBoundAttrListHead(this);

            break;

#if DEBUG
        case CS_NoState:
            VSFAIL("How can we be moving toward CS_NoState?");
            break;

        case CS_Compiled:
            // The only way an alloc can happen in compiled state is
            // if the symbol lives in a metadata project.  Fall through
            // and let the CS_MAX case handle these checks.

            // FallThrough

        case CS_MAX:
            // This means we're allocating for a symbol in a metadata project.
            VSASSERT(psourcefile == NULL, "Should be NULL for metadata allocs.");
            if (IsNamedRoot())
            {
                CompilerFile *pfile = PNamedRoot()->GetContainingCompilerFile();
#if HOSTED
                VSASSERT(pfile == NULL || pfile->IsMetaDataFile()
                            || PNamedRoot()->GetExternalSymbol(),
                            "Should be a metadata file!");
#else
                VSASSERT(pfile == NULL || pfile->IsMetaDataFile(),
                            "Should be a metadata file!");
#endif
            }
            break;

        default:
            VSFAIL("What other states are there?");
            break;
#endif
    }

    if (AllocForWellKnownAttributesToo)
    {
		WellKnownAttrVals* pWellKnownAttrVals = pnra->Alloc<WellKnownAttrVals>();
		pWellKnownAttrVals = new (pWellKnownAttrVals) WellKnownAttrVals(pnra);
        pattrvals->SetPWellKnownAttrVals(pWellKnownAttrVals);
    }
}



//============================================================================
// Returns true if this symbol has the ComImport
//============================================================================

bool BCSYM::IsComImportClass()
{
    SymbolEntryFunction;
    if( this != NULL &&
        ( this->IsClass() || this->IsInterface() ) &&
        this->GetPWellKnownAttrVals()->GetComImportData() )
    {
        return( true );
    }
    else
    {
        return( false );
    }
}

bool BCSYM::CanOverloadWithExtensionMethods()
{
    SymbolEntryFunction;
    return
        this &&
        this->IsProc() &&
        !this->IsEventDecl() &&
        !this->IsProperty();
}

#if HOSTED
void BCSYM::SetExternalSymbol(IUnknown* pExternalSymbol)
{
    SymbolEntryFunction;
    //No Addref, as BCSYMs are allocated with the NorlsAllocator.
    // Must be cleaned up separately.
    ASSERT(m_pExternalSymbol == NULL, "[BCSYM::SetExternalSymbol] m_pExternalSymbol should only be set once.");
    m_pExternalSymbol = pExternalSymbol;
}

IUnknown* BCSYM::GetExternalSymbol()
{
    SymbolEntryFunction;
    return m_pExternalSymbol;
}
#endif

/***************************************************************************************************
;InheritanceWalker

Constructor for InheritanceWalker
***************************************************************************************************/
InheritanceWalker::InheritanceWalker(
    Compiler *CompilerInstance, // [in] instance of the compiler to use for allocations
    BCSYM_Container *ContainerToWalk // [in] the container whose inheritance heirarchy we want to walk
) 
: m_CurrentIdx( 0 )
{
    VSASSERT( CompilerInstance, "Must supply compiler instance" );
    VSASSERT( ContainerToWalk && ContainerToWalk->IsClass() || ContainerToWalk->IsInterface(), "InheritanceWalker only works with classes or interfaces" );

    if ( ContainerToWalk && !ContainerToWalk->IsBad())
    {
        m_InheritanceArray.AddElement( ContainerToWalk );
        // Fill out a dynamic array that holds all the inherited guys for this container
        if ( ContainerToWalk->IsClass())
        {
            BCSYM *BaseClass = ContainerToWalk->PClass()->GetBaseClass();
            while ( BaseClass )
            {
                // The paranoid checking in here is necessary because BadNamedRoots, etc. can come through
                if ( BaseClass->IsClass() && !BaseClass->IsBad())
                {
                    BCSYM_Container *DeClass = BaseClass->PContainer(); // you have to make a temporary because the template takes a reference
                    m_InheritanceArray.AddElement( DeClass );
                    BaseClass = BaseClass->PClass()->GetBaseClass();
                }
                else
                {
                    break;
                }
            }
            return;
        }

        VSASSERT( ContainerToWalk->IsInterface(), "Expecting an interface");
        AddInterfaceBases( ContainerToWalk );
    }
}

/**************************************************************************************************
;AddInterfaceBases

Worker to add base interfaces of an interface to the list
***************************************************************************************************/
void InheritanceWalker::AddInterfaceBases(
    BCSYM_Container *Interface // [in] interface whose bases will be added to the list
)
{
    if(!Interface)
    {
      VSFAIL("AddInterfaceBases - NULL param");
      return;
    }

    // Don't add self since we have already done this in Init() which called us.  Just added base guys
    // Build inheritance tree for an interface - which has multiple inheritance so call a recursive worker
    for ( BCSYM_Implements *Implements = Interface->PInterface()->GetFirstImplements(); Implements; Implements = Implements->GetNext())
    {
        if ( !Implements->IsBadImplements() && !Implements->GetRoot()->IsBad()) // Cycles will have marked these bad if there is a inheritance cycle
        {
            BCSYM_Container *InheritedInterface = Implements->GetRoot()->PContainer(); // need a temporary because the template takes a reference
            m_InheritanceArray.AddElement( InheritedInterface );
            AddInterfaceBases( InheritedInterface );
        }
    }
}

/**************************************************************************************************
;AddToHandlerList

A WithEvents_Set Synthetic method maintains a list of handlers for a particular WithEvent variable
( accessible via the ... ) That list is used to write out AddHandler/RemoveHandler statements later
on.  This adds the information for a handler onto the list for this WithEvents_Set synth method.
***************************************************************************************************/
void BCSYM_Proc::AddToHandlerList
(
    BCSYM_Proc *HandlingProc, // [in] the proc that handles the event
    BCSYM_EventDecl *Event, // [in] the event being handled
    BCSYM_GenericBinding *EventGenericBindingContext, // [in] the GenericBinding for the event being handled
    BCSYM_Property *EventSourceProperty, // [in] the Event source property that provided the WithEvent variable ( usually null )
    NorlsAllocator *Allocator // [in] the allocator to use when allocating Handler lists
)
{
    SymbolEntryFunction;
    HandlerList *NewHandlerList = (HandlerList*)Allocator->Alloc(sizeof(HandlerList));
    NewHandlerList->Event = Event;
    NewHandlerList->EventGenericBindingContext = EventGenericBindingContext;
    NewHandlerList->HandlingProc = HandlingProc;
    NewHandlerList->EventSourceProperty = EventSourceProperty;
    NewHandlerList->Next = GetHandlerList();
    SetHandlerList(NewHandlerList);
}

GenericParamsInferredTypesInfo::GenericParamsInferredTypesInfo()
    : m_Allocator(NORLSLOC)
{
    m_IsInitialized = false;

}

// 

void
GenericParamsInferredTypesInfo::Init()
{
    if (m_IsInitialized)
    {
        return;
    }

    m_InferredTypesHash = new(m_Allocator) GenericParamsInferredTypesHash(&m_Allocator);

    m_IsInitialized = true;
}

void
GenericParamsInferredTypesInfo::AddTypeInferredForGenericParam
(
    BCSYM_GenericParam *GenericParam,
    BCSYM *Type
)
{
    // Ensure initialization has taken place
    //
    Init();

    VSASSERT(GenericParam, "Type being inferred for NULL generic param!!!");

    m_InferredTypesHash->HashAdd(GenericParam, Type);
}

BCSYM *
GenericParamsInferredTypesInfo::FindTypeInferredForGenericParam
(
    BCSYM_GenericParam *GenericParam
)
{
    // Ensure initialization has taken place
    //
    Init();

    VSASSERT(GenericParam, "NULL Generic Param unexpected!!!");

    BCSYM **InferredType =
        m_InferredTypesHash->HashFind(GenericParam);

    return InferredType ?
            *InferredType :
            NULL;
}

#if IDE
void BCSYM_EventDecl::ReplaceDelegateByDeDeclaredDelegate()
{
    m_pDelegate = m_pDeclaredDelegate;

    if (m_DelegateVariable)
    {
        VSASSERT(m_pDeclaredDelegateVariableType, "should be set");
        Symbols::SetType(m_DelegateVariable, m_pDeclaredDelegateVariableType);
    }

    if (m_pprocAdd != NULL && m_pprocAdd->IsSyntheticMethod())
    {       
        Symbols::SetParamType(m_pprocAdd->GetFirstParam(), m_pDeclaredDelegate);
    }

    if (m_pprocRemove != NULL && m_pprocRemove->IsSyntheticMethod())
    {
        Symbols::SetParamType(m_pprocRemove->GetFirstParam(), m_pDeclaredDelegate);
    }
}
#endif

//
//  Get the XML Signature for the Event Decl needed to get the XML Doc
//
void BCSYM_EventDecl::GetDocCommentSignature
(
    Compiler *pCompiler,
    BCSYM_NamedRoot *pParent,
    StringBuffer *pSignature
)
{
    SymbolEntryFunction;
    BCSYM_Container *pParentContainer = GetParent()->PContainer();
    pSignature->AppendString(L"E:");

    if (pParent)
    {
        pSignature->AppendSTRING(pParent->GetQualifiedEmittedName());
        pSignature->AppendChar(L'.');
        pSignature->AppendSTRING(GetName());
    }
    else if (pParentContainer)
    {
        pSignature->AppendSTRING(pParentContainer->GetQualifiedEmittedName());
        pSignature->AppendChar(L'.');
        pSignature->AppendSTRING(GetName());
    }
}

void BCSYM_EventDecl::ConvertToWindowsRuntimeEvent(_In_ CompilerHost *pCompilerHost, _In_ Symbols *pSymbolCreator)
{
    Location *pLoc = this->GetLocation();

    // Check to see if the WinRT types are present.
    if (!pCompilerHost->GetFXSymbolProvider()->IsTypeAvailable(FX::EventRegistrationTokenType))
    {
        pCompilerHost->GetFXSymbolProvider()->ReportTypeMissingErrors(FX::EventRegistrationTokenType, this->GetErrorTableForContext(), *pLoc);
        return;
    }
    
    if (!pCompilerHost->GetFXSymbolProvider()->IsTypeAvailable(FX::EventRegistrationTokenTableType))
    {
        pCompilerHost->GetFXSymbolProvider()->ReportTypeMissingErrors(FX::EventRegistrationTokenTableType, this->GetErrorTableForContext(), *pLoc);
        return;
    }

    if (IsWindowsRuntimeEvent())
    {
        return;
    }

    SetIsWindowsRuntimeEvent(true);
    
    VSASSERT(m_pprocAdd && m_pprocRemove && m_pprocRemove->GetFirstParam(), 
        "The event should already have been initialized");

    BCSYM *pTokenType = pCompilerHost->GetFXSymbolProvider()->GetType(FX::EventRegistrationTokenType);
    m_pprocAdd->SetType(pTokenType);
    m_pprocRemove->GetFirstParam()->SetType(pTokenType);

    // If we have a backing field for this event, then change it's type to EventRegistrationTokenTable<T>
    if (m_DelegateVariable && GetDelegate() && GetDelegate()->IsNamedRoot())
    {
        BCSYM **typeArg = reinterpret_cast<BCSYM**>(pSymbolCreator->GetNorlsAllocator()->Alloc(sizeof(BCSYM*)));
        typeArg[0] = GetDelegate();
        BCSYM_GenericBinding *pTokenTableType = pSymbolCreator->GetGenericBinding(
                                                    false,
                                                    pCompilerHost->GetFXSymbolProvider()->GetType(FX::EventRegistrationTokenTableType),
                                                    typeArg,
                                                    1,
                                                    NULL);
#if IDE
        SetDeclaredDelegateVariableType(m_DelegateVariable->GetRawType());
#endif
        m_DelegateVariable->SetType(pTokenTableType);
    }
}

#if IDE
void BCSYM_EventDecl::RestoreDeclaredDelegateVariableForWinRT()
{
    if (m_DelegateVariable != NULL)
    {
        m_DelegateVariable->SetType(m_pDeclaredDelegateVariableType);
        
        // For WinRT event, compiler morph m_DelegateVariable to SYM_VariableWithValue,
        // here we need to revert it back to SYM_Variable
        ((BCSYM_VariableWithValue*)m_DelegateVariable)->SetExpression(NULL);
        m_DelegateVariable->SetSkKind(SYM_Variable);
        m_DelegateVariable->SetNew(false);      
    }
}
#endif

unsigned long ExtensionCallInfo::GetFreeArgumentCount()
{

    Assume(!m_pPartialGenericBinding || m_pPartialGenericBinding->m_pFixedTypeArgumentBitVector, L"");

    if (m_pPartialGenericBinding)
    {
        return m_pPartialGenericBinding->m_pFixedTypeArgumentBitVector->BitCount() - m_pPartialGenericBinding->m_pFixedTypeArgumentBitVector->SetCount();
    }
    else
    {
        return 0;
    }
}

BCSYM_ArrayLiteralType::BCSYM_ArrayLiteralType
(
    const NorlsAllocWrapper & alloc
) :
    m_elements(alloc),
    m_pRealArrayType(NULL),
    m_literalLocation()
{
}

ConstIterator<ILTree::Expression *> 
BCSYM_ArrayLiteralType::GetElements()
{
    SymbolEntryFunction;
    return m_elements.GetConstIterator();
}

void 
BCSYM_ArrayLiteralType::AddElement(ILTree::Expression * pElement)
{
    SymbolEntryFunction;
    m_elements.Add(pElement);
}

BCSYM_ArrayType * 
BCSYM_ArrayLiteralType::GetCorrespondingArrayType()
{
    SymbolEntryFunction;
    return m_pRealArrayType;
}

void 
BCSYM_ArrayLiteralType::SetCorrespondingArrayType
(
    BCSYM_ArrayType * pArrayType
)
{
    SymbolEntryFunction;
    m_pRealArrayType = pArrayType;
}

void 
BCSYM_ArrayLiteralType::SetLiteralLocation
(
    const Location & loc
)
{
    SymbolEntryFunction;
    m_literalLocation.SetLocation(&loc);
}

const Location & 
BCSYM_ArrayLiteralType::GetLiteralLocation()
{
    SymbolEntryFunction;
    return m_literalLocation;
}

unsigned long
BCSYM_ArrayLiteralType::GetElementCount()  const
{
    SymbolEntryFunction;
    return m_elements.Count();
}
