//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Import metadata into the symbol table.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

//-------------------------------------------------------------------------------------------------
//
// Constructor.
//
MetaImport::MetaImport
(
    Compiler *pCompiler,
    MetaDataFile *pMetaDataFile,
    SymbolList *pSymbolList
)
: m_pCompiler(pCompiler)
, m_nraScratch(NORLSLOC)
, m_pMetaDataFile(pMetaDataFile)
, m_pCompilerHost(pMetaDataFile->GetCompilerHost())
, m_Symbols(pCompiler, pMetaDataFile->SymbolStorage(), NULL, pMetaDataFile->GetCurrentGenericBindingCache())
, m_pmdImport(pMetaDataFile->GetImport())
, m_pmdImport2(pMetaDataFile->GetImport2())
, m_pwinmdImport(pMetaDataFile->GetWinMDImport())
, m_pSymbolList(pSymbolList)
, m_rgmembers(NULL)
, m_pcontainer(NULL)
, m_cMembers(0)
, m_cchNameMax(0)
, m_td(mdTypeDefNil)
, m_CurrentMethodGenericParams(NULL)
{
}

//****************************************************************************
// Main entrypoints.
//****************************************************************************

//============================================================================
// Import all of the bases and implements of the type.
//============================================================================

void MetaImport::ImportBaseAndImplements
(
    BCSYM_Container *Type,
    bool LoadNestedTypes
)
{
    Compiler *Compiler = Type->GetCompiler();
    CompilerIdeLock spLock(Compiler->GetMetaImportCritSec());

    if (Type->IsNamespace())
    {
        BCITER_CHILD Children;
        BCSYM_NamedRoot *Child;

        VSASSERT(LoadNestedTypes, "huh?");

        Children.Init(Type);

        while (Child = Children.GetNext())
        {
            if (Child->IsContainer())
            {
                ImportBaseAndImplements(Child->PContainer(), true);
            }
        }
    }
    else
    {
        CompilerFile *pFile = Type->GetCompilerFile();

        // The bases for types in source projects will already be bound.
        if (!pFile->IsMetaDataFile())
        {
            return;
        }

        MetaDataFile *pMetaDataFile = pFile->PMetaDataFile();
#if IDE
        pMetaDataFile->SetBindingStarted();
#endif IDE

        MetaImport import(Compiler, pMetaDataFile, NULL);
        import.DoImportBaseAndImplements(Type);

        if (LoadNestedTypes)
        {
            BCSYM_Container *Child;

            VSASSERT(!Type->AreChildrenLoaded() ||
                      Type->IsBad() ||
                      Type->IsEnum(), "can't be loaded yet!");

            for (Child = *Type->GetNestedTypeList();
                 Child;
                 Child = Child->GetNextInSymbolList()->PContainer())
            {
                ImportBaseAndImplements(Child->PContainer(), true);
            }
        }
    }
}

//========================================================================
// Import a single class into the symbol table.  This is used for
// demand loading.
//========================================================================

void MetaImport::ImportTypeChildren
(
    BCSYM_Container *pContainer
)
{
    Compiler *pCompiler = pContainer->GetCompiler();
    CompilerIdeLock spLock(pCompiler->GetMetaImportCritSec());

    // If this container is already imported, do nothing.  We need to
    // repeat this check in case the other thread loaded this between
    // when we checked before and now.
    //
    if (!pContainer->AreChildrenLoaded() && !pContainer->AreChildrenLoading())
    {
        MetaDataFile *pMetaDataFile = pContainer->GetMetaDataFile();
        MetaImport import(pCompiler, pMetaDataFile, NULL);

        import.DoImportTypeChildren(pContainer);
    }
}

//========================================================================
// Import new children on the type.  This is used by the EE to load members
// that are added or removed during ENC
//========================================================================
void 
MetaImport::UpdateImportedChildren( _In_ BCSYM_Container* pContainer )
{
    ThrowIfNull(pContainer);
    ThrowIfFalse(pContainer->AreChildrenLoaded());

    Compiler *pCompiler = pContainer->GetCompiler();
    CompilerIdeLock spLock(pCompiler->GetMetaImportCritSec());

    MetaDataFile *pMetaDataFile = pContainer->GetMetaDataFile();
    MetaImport import(pCompiler, pMetaDataFile, NULL);

    import.DoUpdateImportedTypeChildren(pContainer);
}


BCSYM*
MetaImport::DecodeTypeInDeclaration
(
    BCSYM_NamedRoot *pNamed,
    const COR_SIGNATURE *pSignature)
{
    Compiler *pCompiler = pNamed->GetCompiler();
    CompilerIdeLock spLock(pCompiler->GetMetaImportCritSec());
    BCSYM* pRetType;

    MetaDataFile *pMetaDataFile = pNamed->GetCompilerFile()->PMetaDataFile(); 
    MetaImport import(pCompiler, pMetaDataFile, NULL);
    bool isBad = false;

    if ( pNamed->IsProc() )
    {
        import.m_CurrentMethodGenericParams = pNamed->PProc()->GetFirstGenericParam();
        import.m_pcontainer = pNamed->GetContainer();
    }

    pRetType = import.DecodeType(&pSignature, isBad);
    if ( isBad )
    {
        return NULL;
    }

    return pRetType;
}

//****************************************************************************
// Import project-level information.
//****************************************************************************

//============================================================================
// Import all of the project-level symbols out of a metadata database.
//============================================================================

void MetaImport::ImportTypes
(
    Compiler *pCompiler,
    MetaDataFile *pMetaDataFile,
    SymbolList *pSymbolList,
    ErrorTable *pErrorTable
)
{
    CompilerIdeLock spLock(pCompiler->GetMetaImportCritSec());

    if (!pMetaDataFile->GetImport())
    {
        // We don't have an importer, so just stop.
        return;
    }

    //
    // Do the load.
    //

    MetaImport import(pCompiler, pMetaDataFile, pSymbolList);

    import.DoImportTypes();
}

//============================================================================
// Does the actual work of importing the metadata.
//============================================================================

void MetaImport::DoImportTypes()
{
    unsigned long cchScope;
    WCHAR *wszScope;
    STRING *pstrScope;
    bool fMissingTypes = false;

    MetaType *rgtypes;
    unsigned iType, cTypes;
    MetaTypeFwd *rgtypeFwds;
    unsigned iTypeFwd, cTypeFwds;

#ifdef _WIN64
    DynamicFixedSizeHashTable<size_t, TokenHashValue> *pTokenHash;
    DynamicFixedSizeHashTable<size_t, AttrIdentity *> *pAttrTokenHash;
#else
    DynamicFixedSizeHashTable<mdTypeDef, TokenHashValue> *pTokenHash;
    DynamicFixedSizeHashTable<mdToken, AttrIdentity *> *pAttrTokenHash;
#endif
    DynamicFixedSizeHashTable<STRING *, BCSYM_Container *> *pNameHash;
    DynamicFixedSizeHashTable<STRING *, BCSYM_TypeForwarder *> *pNameHashForTypeFwds;

    //
    // Set the name of the MetaDataFile to be the name of the
    // scope.
    //

    IfFailThrow(m_pmdImport->GetScopeProps(NULL,
                                           0,
                                           &cchScope,
                                           NULL));

    if (cchScope)
    {
        SafeInt<unsigned long> safeScope(cchScope);
        safeScope *= sizeof(WCHAR);
        wszScope = (WCHAR *)m_nraScratch.Alloc(safeScope.Value());

        IfFailThrow(m_pmdImport->GetScopeProps(wszScope,
                                               cchScope,
                                               NULL,
                                               NULL));

        pstrScope = m_pCompiler->AddString(wszScope);
    }
    else
    {
        pstrScope = m_pCompiler->AddString(L"");
    }

    // Remember the scope name to help when debugging.
    m_pMetaDataFile->SetName(pstrScope);
    if (m_pMetaDataFile->GetProject()->GetScopeName() == NULL)
    {
        m_pMetaDataFile->GetProject()->SetScopeName(pstrScope);
    }

    // Set up the attribute hash
#ifdef _WIN64
    pAttrTokenHash = (DynamicFixedSizeHashTable<size_t, AttrIdentity *> *)
        m_pMetaDataFile->SymbolStorage()->Alloc(sizeof(DynamicFixedSizeHashTable<size_t, AttrIdentity *>));
#else
    pAttrTokenHash = (DynamicFixedSizeHashTable<mdToken, AttrIdentity *> *)
        m_pMetaDataFile->SymbolStorage()->Alloc(sizeof(DynamicFixedSizeHashTable<mdToken, AttrIdentity *>));
#endif
    pAttrTokenHash->Init(m_pMetaDataFile->SymbolStorage(), 16);
    m_pMetaDataFile->SetAttributeTokenHash(pAttrTokenHash);

    CheckWhetherToImportInaccessibleMembers();

    //
    // Load all of the classes contained in the metadata.
    //

    LoadTypes(&rgtypes, &cTypes, &fMissingTypes);

    //
    // Generate the symbol list.
    //

#ifdef _WIN64
    pTokenHash = (DynamicFixedSizeHashTable<size_t, TokenHashValue> *)
        m_pMetaDataFile->SymbolStorage()->Alloc(sizeof(DynamicFixedSizeHashTable<size_t, TokenHashValue>));
#else
    pTokenHash = (DynamicFixedSizeHashTable<mdToken, TokenHashValue> *)
        m_pMetaDataFile->SymbolStorage()->Alloc(sizeof(DynamicFixedSizeHashTable<mdToken, TokenHashValue>));
#endif
    pTokenHash->Init(m_pMetaDataFile->SymbolStorage(), (cTypes / 8) + 1);

    pNameHash = (DynamicFixedSizeHashTable<STRING *, BCSYM_Container *> *)
    m_pMetaDataFile->SymbolStorage()->Alloc(sizeof(DynamicFixedSizeHashTable<STRING *, BCSYM_Container *>));
    pNameHash->Init(m_pMetaDataFile->SymbolStorage(), (cTypes / 8) + 1);

    for (iType = 0; iType < cTypes; iType++)
    {
        BCSYM_Container *pContainer = rgtypes[iType].m_pContainer;
        BCSYM *pSym = pContainer;
        mdTypeDef tdNestingContainer = 0;
        mdTypeDef td;

        if (!pContainer)
        {
            continue;
        }

        td = (mdTypeDef)pContainer->GetToken();

        TokenHashValue thv;
        thv.pSym = pSym;
        thv.fTypeForward = false;

#ifdef _WIN64
        {
        size_t tdTemp = td;
        pTokenHash->HashAdd(tdTemp, thv);
        }
#else
        pTokenHash->HashAdd(td, thv);
#endif

        // Get the token for the class that we're nested inside of.
        tdNestingContainer = rgtypes[iType].m_tdNestingContainer;

        // If the symbol is nested than we want to link it off its parent
        // and store an unbindable alias off the symbol list.  We don't
        // have to put it in the hash table as we'll never try
        // to access it directly.
        //
        if (tdNestingContainer)
        {
            BCSYM_Container *pNestingContainer;
            BCSYM_Container **ppContainer;
            MetaType *pType = FindType(tdNestingContainer, rgtypes, cTypes, fMissingTypes);

            //
            // We store the list of nested classes
            // in the containing class while it is unloaded.  It will be
            // stored in the hash table for the container when it is
            // finally loaded.
            //

            pNestingContainer = pType ? pType->m_pContainer : NULL;

            // If we didn't find the type then it was contained inside
            // of a private type.  Just let it go.
            //
            if (!pNestingContainer)
            {
                continue;
            }
            else
            {
                VSASSERT(pNestingContainer->GetToken() == tdNestingContainer, "Don't match?");
            }

            // Hang the real symbol off the parent symbol.
            ppContainer = pNestingContainer->GetNestedTypeList();

            Symbols::SetNext(pContainer, *ppContainer);
            *ppContainer = pContainer;

            // make the child know who its parent is
            Symbols::SetParent(pContainer, pNestingContainer);
        }
        else
        {
            STRING *name;

            // Otherwise just add the symbol to the list & hashes
            m_pSymbolList->AddToFront(pContainer);

            if (pContainer->IsIntrinsicType())
            {
                name = pContainer->GetEmittedName();
            }
            else
            {
                name = pContainer->GetName();
            }

            pNameHash->HashAdd(name, pContainer);
        }
    }

    // Load all the type forwarders in this metadata file, but only if it is the primary module.
    //
    if (m_pMetaDataFile->GetProject()->GetPrimaryModule() == m_pMetaDataFile)
    {
        LoadTypeForwarders(&rgtypeFwds, &cTypeFwds);

        pNameHashForTypeFwds = (DynamicFixedSizeHashTable<STRING *, BCSYM_TypeForwarder *> *)
            m_pMetaDataFile->SymbolStorage()->Alloc(sizeof(DynamicFixedSizeHashTable<STRING *, BCSYM_TypeForwarder *>));
        pNameHashForTypeFwds->Init(m_pMetaDataFile->SymbolStorage(), (cTypes / 8) + 1);

        for (iTypeFwd = 0; iTypeFwd < cTypeFwds; iTypeFwd++)
        {
            STRING *name;

            if (!rgtypeFwds[iTypeFwd].m_pTypeFwd)
            {
                continue;
            }

            m_pSymbolList->AddToFront(rgtypeFwds[iTypeFwd].m_pTypeFwd);

            name = rgtypeFwds[iTypeFwd].m_pTypeFwd->GetName();
            pNameHashForTypeFwds->HashAdd(name, rgtypeFwds[iTypeFwd].m_pTypeFwd);
        }
    }
    else
    {
        pNameHashForTypeFwds = NULL;
    }

    // Create the type parameters of generic types. This occurs after loading the types so that
    // type parents are available
    //
    for (iType = 0; iType < cTypes; iType++)
    {
        BCSYM_Container *pContainer = rgtypes[iType].m_pContainer;

        // If the name isn't mangled, then it can't be a generic type.
        if (!pContainer)
            continue;

        SetupGenericParameters(pContainer, pContainer->GetToken());

        // See if this was marked with the special StandardModuleAttribute.
        // If so, back-patch some of the class's data members.
        //
        // Modules have be classes
        // Modules cannot be nested types
        // Modules cannot be generic
        //
        if (!rgtypes[iType].m_pContainer->IsBad() &&
            IsClassOnly(rgtypes[iType].m_pContainer) &&
            IsNilToken(rgtypes[iType].m_tdNestingContainer) &&
            !rgtypes[iType].m_pContainer->IsGeneric() &&
            rgtypes[iType].m_pContainer->GetPWellKnownAttrVals()->GetStandardModuleData())
        {
            Symbols::SetClassAsStandardModule(rgtypes[iType].m_pContainer->PClass());
        }
    }

    m_pMetaDataFile->SetTokenHash(pTokenHash);
    m_pMetaDataFile->SetNameHash(pNameHash);
    m_pMetaDataFile->SetNameHashForTypeFwds(pNameHashForTypeFwds);
}

DECLFLAGS MetaImport::MapTypeFlagsToDeclFlags
(
    unsigned long TypeFlags,
    bool fCodeModule    // Are we importing a type from a code module?
)
{
    DECLFLAGS Flags = 0;

#if IDE
    VSASSERT(!fCodeModule, "How can the IDE be importing a code module?");
#endif

    // Because we need to know about inheritance, we may be importing types
    // that are inaccessible, in which case, we need to mark them as closely
    // as possible to the original (notice that we import friends in other assemblies
    // as friends). The reason for this is that it helps us correctly resolve
    // overloading (hide by sig), shadowing, overriding from the imported metadata
    // assembly's perspective.
    //
    // ***
    // *** NOTE!!  If this code changes, update the switch statement in MetaImport::LoadFieldDef
    // *** NOTE!!  and MetaImport::LoadMethodDef
    // ***
    switch(TypeFlags & tdVisibilityMask)
    {
        case tdNestedAssembly:
#if !IDE
            // VBC is importing from a code module, but the rest of the compiler would
            // be confused by DECLF_Friend that spans multiple CompilerProjects, so
            // treat this as Public.
            if (fCodeModule)
            {
                Flags |= DECLF_Public;
            }
            else
#endif
            {
                Flags |= DECLF_Friend;
            }
            break;

        case tdNestedFamORAssem:
#if !IDE
            // VBC is importing a code module, and FamORAssem == FamORTrue == True,
            // so mark as Public.
            if (fCodeModule)
            {
                Flags |= DECLF_Public;
            }
            else
#endif
            {
                // Type is imported from another assembly, and FamORAssem == FamORFalse == Fam,
                // so mark as Protected.
                Flags |= DECLF_ProtectedFriend;
            }
            break;

        case tdNestedFamANDAssem:
#if !IDE
            // VBC is importing a code module, and FamANDAssem == FamANDTrue == Fam,
            // so mark as Protected.
            if (fCodeModule)
            {
                Flags |= DECLF_Protected;
            }
            else
#endif
            {
                // VB can't represent this accurately, so treat it as friend
                Flags |= DECLF_Friend;
            }
            break;

        case tdNestedPrivate:
            Flags |= DECLF_Private;
            break;

        case tdPublic:
        case tdNestedPublic:
            Flags |= DECLF_Public;
            break;

        case tdNestedFamily:
            Flags |= DECLF_Protected;
            break;

        // VSW 298027, VSW 280397:
        // Make sure we can import friend classes when importing a codemodule
        case tdNotPublic:
#if !IDE
            // VBC is importing a code module, so treat "Friend" as public.
            if (fCodeModule)
            {
                Flags |= DECLF_Public;
            }
            else
#endif
            {
                // IDE doesn't support codemodules, so treat this as friend.
                Flags |= DECLF_Friend;
            }
            break;

        default:
            VSFAIL("unexpected!");
            Flags |= DECLF_Private;
            break;
    }

    if (TypeFlags & tdSealed)
    {
        Flags |= DECLF_NotInheritable;
    }

    //CLR can generate Abstract & Sealed types. We treat as Sealed.
    if (TypeFlags & tdAbstract && !(TypeFlags & tdSealed))
    {
        Flags |= DECLF_MustInherit; // abstract
    }

    // Types always shadow
    Flags |= DECLF_ShadowsKeywordUsed;

    return Flags;
}

MetaImport::MetaType *MetaImport::FindType
(
    mdTypeDef td,
    _In_ MetaType *rgtypes,
    unsigned cTypes,
    bool fMissingTypes
)
{
    int iType;

    // In the 99% case, there's going to be a 1-to-1 mapping from token to
    // entry in the array.
    if (!fMissingTypes)
    {
        return &rgtypes[RidFromToken(td) - 2];
    }

    // Normally, iType is just two less than the Rid of the token. However, in some
    // C++ incremental compilation cases, types may be marked as "deleted" which means
    // that they show up in the total count but are skipped over by EnumTypeDefs. So
    // we're going to have to search manually. This is unfortunate, but should be very,
    // very rare.
    for (iType = 0; iType < (int)cTypes; iType++)
    {
        if (rgtypes[iType].m_pContainer && rgtypes[iType].m_pContainer->GetToken() == td)
            return &rgtypes[iType];
    }

    // Couldn't find it.
    return NULL;
}

//============================================================================
// Loads the metatype array.  This only creates a skeleton symbol for each
// imported type.  These symbols are filled in on-demand via EnsureLoaded.
//============================================================================

void MetaImport::LoadTypes
(
    MetaType **prgtypes,
    unsigned *pcTypes,
    _Out_ bool *pfMissingTypes
)
{
    CorEnum hEnum(m_pmdImport);
    mdTypeDef td;
    MetaType *rgtypes;
    unsigned long cTypes;
    unsigned long iType, cTypeDefs;

    //
    // Figure out how many types there are.
    //
    *pfMissingTypes = false;

    // Open the enum.
    IfFailThrow(m_pmdImport->EnumTypeDefs(&hEnum,
                                          NULL,
                                          0,
                                          NULL));

    // Get the count.
    IfFailThrow(m_pmdImport->CountEnum(hEnum, &cTypes));

    // If ctypes is 0, no need to do any of this, but we can't just fall
    // through and call the scratch Alloc -- an alloc of 0 rightfully
    // causes an assert.  So just set rgtypes to NULL and then fall through.
    if (cTypes == 0)
    {
        rgtypes = NULL;
    }
    else
    {
        rgtypes = (MetaType *)m_nraScratch.Alloc(VBMath::Multiply(cTypes, sizeof(MetaType)));
    }

    //
    // Load each type's properties.
    //

    for (iType = 0; iType < cTypes; iType++)
    {
        mdToken tkExtends;
        STRING *pstrName;
        STRING *pstrNameSpace;
        DECLFLAGS DeclFlags;
        bool IsClass;

        IfFailThrow(m_pmdImport->EnumTypeDefs(&hEnum,
                                              &td,
                                              1,
                                              &cTypeDefs));

        // Normally, iType is just two less than the Rid of the token. However, in some
        // C++ incremental compilation cases, types may be marked as "deleted" which means
        // that they show up in the total count but are skipped over by EnumTypeDefs.
        if (iType != RidFromToken(td) - 2)
            *pfMissingTypes = true;

        // We're done.
        if (!cTypeDefs)
        {
            break;
        }

        // Get the properties for this type.
        if (!GetTypeDefProps(rgtypes, cTypes, *pfMissingTypes, td, &IsClass, &pstrName, &pstrNameSpace, &DeclFlags, &tkExtends, &rgtypes[iType].m_tdNestingContainer))
            continue;

        // Create the appropriate symbol.
        if (!IsClass)
        {
            // Fill in the symbol.
            rgtypes[iType].m_pContainer = m_Symbols.GetInterface(NULL,
                                   pstrName,
                                   pstrName,
                                   pstrNameSpace,
                                   m_pMetaDataFile,
                                   DeclFlags,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL);
        }
        // Normal class.
        else
        {
            // NOTE: We don't bother to check for Enums or Structures here
            // because we'll get to that when we resolve the bases.

            // Fill in the symbol.
            rgtypes[iType].m_pContainer = m_Symbols.AllocClass(false);

            m_Symbols.GetClass(NULL,
                               pstrName,
                               pstrName,
                               pstrNameSpace,
                               m_pMetaDataFile,
                               NULL,
                               NULL,
                               DeclFlags,
                               t_bad,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               rgtypes[iType].m_pContainer->PClass());
        }

        if (!rgtypes[iType].m_pContainer->IsBad())
        {
            Attribute::----AllAttributesOnToken(td,
                             m_pmdImport,
                             m_Symbols.GetNorlsAllocator(),
                             m_pCompiler,
                             m_pMetaDataFile->GetProject(),
                             m_pMetaDataFile,
                             rgtypes[iType].m_pContainer);

            // See if this was marked with the special StandardModuleAttribute.
            // If so, back-patch some of the class's data members.
            if (rgtypes[iType].m_pContainer->IsClass())
            {
                WellKnownAttrVals *pWellKnownAttributesOnClass = rgtypes[iType].m_pContainer->GetPWellKnownAttrVals();
                if ( pWellKnownAttributesOnClass->GetStandardModuleData())
                {
                    Symbols::SetClassAsStandardModule(rgtypes[iType].m_pContainer->PClass());
                }

                if ( pWellKnownAttributesOnClass->GetExtensionData())
                {
                    rgtypes[iType].m_pContainer->PClass()->SetContainsExtensionMethods();
                }
            }
        }

        // The compiler maintains a cache of well-known FX symbols for use in semantics and codegen.
        // We need to cache the intrinsics, some types from System.Linq.Query and Windows.Foundation,
        // and various other types from MSCORLIB.DLL. See if this symbol may be one we are interested
        // in caching.
        bool ConsiderCachingThisSymbol = false;

        // We used to consider caching all symbols from mscorlib and a few specific namespaces but with
        // the immersive profile, the types can be in different assemblies and the list of all namespaces
        // to check is quite big. So we can restrict it by just checking to see if the namespace begins with
        // System or Windows.Foundation. All the predefined types' namespaces begin with these.
        // Extract the first part of the namespace and check if it's equal to System.
        STRING *pBaseNamespace = pstrNameSpace;
        if (pstrNameSpace)
        {
            WCHAR *pFirstDot = wcschr(pstrNameSpace, L'.');
            if (pFirstDot)
            {
                pBaseNamespace = m_pCompiler->AddStringWithLen(pstrNameSpace, pFirstDot - pstrNameSpace);
            }
        }

        if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComDomain), pBaseNamespace) ||
            StringPool::IsEqual(STRING_CONST(m_pCompiler, WindowsFoundationDomain), pstrNameSpace))
        {
            ConsiderCachingThisSymbol = true;
        }

        // If the symbol we are importing is a mscorlib.dll type or from System.Linq.Query*.*, then ask
        // the FXSymbolProvider to cache it.  The FXSymbolProvider is the last word on what actually
        // gets cached.  All we have done so far is determine whether it may be interesting to try to cache it.
        // We decide if it is interesting to cache before falling into this code so that we don't
        // inadvertantly cache a type that has a fully-qualified name that the FXSymbolProvider is willing to
        // cache, yet may not come from mscorlib.dll  We relax this test for the types in System.Linq.Query.dll
        // because in that case we don't know what the dll name will necessairly be. 
        if (ConsiderCachingThisSymbol)
        {
            if (!m_pCompilerHost->GetFXSymbolProvider()->LoadType(
                    pstrNameSpace, rgtypes[iType].m_pContainer, m_pMetaDataFile->GetProject()))
            {
                if (!rgtypes[iType].m_pContainer->IsBad() &&
                    rgtypes[iType].m_pContainer->IsClass() &&
                    m_pCompilerHost->GetFXSymbolProvider()->LoadIntrinsicType(
                        pstrNameSpace,
                        rgtypes[iType].m_pContainer->PClass()))
                {
                    if (!StringPool::IsEqual(pstrName, rgtypes[iType].m_pContainer->GetName()))
                    {
                        // The container we imported will have the name
                        // set to the VB intrinsic name, so create an alias
                        // for the CLR name.
                        m_Symbols.GetAlias(pstrName,
                                           DECLF_Public,
                                           rgtypes[iType].m_pContainer,
                                           m_pSymbolList);
                    }
                }
            }
        }

        // Mark this as being delay loaded.
        if (!rgtypes[iType].m_pContainer->IsBad())
        {
            Symbols::SetContainerDelayLoadInformation(rgtypes[iType].m_pContainer,
                                                      td,
                                                      tkExtends,
                                                      m_pMetaDataFile);
        }
    }

    // Remember how many we actually have.
    *pcTypes = iType;
    *prgtypes = rgtypes;
}

//============================================================================
// Loads the metatypefwds array.  This only creates a skeleton symbol for each
// imported type forwarder.  The fowarded to destination will be filled in
// later.
//============================================================================

void MetaImport::LoadTypeForwarders
(
    MetaTypeFwd **prgtypeFwds,
    unsigned *pcTypeFwds
)
{
    CorEnum hEnum(m_pmdImport);
    CComPtr<IMetaDataAssemblyImport> srpAssemblyImport;

    mdExportedType tkExp;
    MetaTypeFwd *rgtypeFwds;
    unsigned long cTypeFwds;
    unsigned long iType, cTypeFwdsRet;

    IfFailThrow(m_pmdImport->QueryInterface(IID_IMetaDataAssemblyImport,
                                            (void **)&srpAssemblyImport));

    //
    // Figure out how many types there are.
    //

    // Open the enum.
    IfFailThrow(srpAssemblyImport->EnumExportedTypes(&hEnum,
                                                     NULL,
                                                     0,
                                                     NULL));

    // Get the count.
    IfFailThrow(m_pmdImport->CountEnum(hEnum, &cTypeFwds));

    // If ctypes is 0, no need to do any of this, but we can't just fall
    // through and call the scratch Alloc -- an alloc of 0 rightfully
    // causes an assert.  So just set rgtypes to NULL and then fall through.
    if (cTypeFwds == 0)
    {
        rgtypeFwds = NULL;
    }
    else
    {
        rgtypeFwds = (MetaTypeFwd *)m_nraScratch.Alloc(VBMath::Multiply(
            cTypeFwds, 
            sizeof(MetaTypeFwd)));
    }

    //
    // Load each exported type's properties.
    //

    for (iType = 0; iType < cTypeFwds; iType++)
    {
        STRING *pstrName;
        STRING *pstrNamespace;
        mdAssemblyRef tkDestAssemblyRef;

        IfFailThrow(srpAssemblyImport->EnumExportedTypes(&hEnum,
                                                         &tkExp,
                                                         1,
                                                         &cTypeFwdsRet));

        // We're done.
        if (!cTypeFwdsRet)
        {
            break;
        }

        // Get the properties for this type fowarder.
        if (!GetTypeForwarderProps(srpAssemblyImport, tkExp, &pstrName, &pstrNamespace, &tkDestAssemblyRef))
        {
            continue;
        }

        // Create a BadNamedRoot to represent the forwarder.
        rgtypeFwds[iType].m_pTypeFwd = m_Symbols.GetTypeForwarder(pstrName,
                                                                  pstrNamespace,
                                                                  tkDestAssemblyRef,
                                                                  NULL);

        // 

    }

    // Remember how many we actually have.
    *pcTypeFwds = iType;
    *prgtypeFwds = rgtypeFwds;
}

void
MetaImport::SetupGenericParameters
(
    BCSYM_NamedRoot *Symbol,
    mdToken SymbolToken
)
{
    // Backward compatibility with the Everett CLR
    if (m_pCompilerHost->GetRuntimeVersion() >= RuntimeVersion2)
    {
        AssertIfNull(m_pmdImport2);

        if (Symbol == NULL || Symbol->IsBad())
        {
            return;
        }

        HCORENUM EnumTypeParams = 0;

        // CLS requires that each nested type first declare all the generic parameters that its
        // immediate parent does. And it must declare them exactly the same way (up to renaming), including
        // the same constraints and the same variance annotations. This is vaguely specified in
        // ECMA $I.10.7.1 and CLS rule 42. Indeed, in the VB compiler, the BCSYM structure for a nested type
        // doesn't even bother declaring its parent's generic parameters -- on the assumption that CLS holds.
        //
        // Note on the issue of renaming... if we import ".interface IOuter(Of T As New)" which has a nested
        // interface ".interface IInner(Of U As New, T)" then it's allowed by the CLS, because the U
        // in IInner matches the T in IOuter. That's fine. We can still import it. VB consumers of the
        // class will instantiate it as IOuter(Of T=Animal).IInner(Of T=Mineral), and intellisense &c.
        // will happily pick up the fact that both parameters have name "T". After all, the fact that
        // the first parameter of IInner is called "U" is of interest only to the internals of the interface;
        // never to VB consumers.
        //
        // For our validation of input, we will check that variance and constraints flags match, but we
        // won't bother checking that constraints match. That seems like too much engineering and code
        // complexity to solve a problem that never arises in practice. (The complexity is because
        // constraints have to be resolved later, after all types have been loaded, so it'd be a
        // non-local check.)
        //
        

        // Find out how many generic parameters there are.
        unsigned long TypeParamCount = 0;
        IfFailThrow(m_pmdImport2->EnumGenericParams(&EnumTypeParams, SymbolToken, NULL, 0, &TypeParamCount));
        IfFailThrow(m_pmdImport->CountEnum(EnumTypeParams, &TypeParamCount));
        m_pmdImport->CloseEnum(EnumTypeParams);
        EnumTypeParams = 0;

        // And how many generic params the container has.
        unsigned long ContainerTypeParamCount = 0;
        BCSYM_NamedRoot *Container = Symbol->GetParent();
        if (Symbol->IsContainer() && Container && (Container->IsClass() || Container->IsInterface()))
        {
            HCORENUM EnumContainerTypeParams = 0;
            IfFailThrow(m_pmdImport2->EnumGenericParams(&EnumContainerTypeParams, Container->GetToken(), NULL, 0, &ContainerTypeParamCount));
            IfFailThrow(m_pmdImport->CountEnum(EnumContainerTypeParams, &ContainerTypeParamCount));
            m_pmdImport->CloseEnum(EnumContainerTypeParams);
        }


        const unsigned MaxScratchTypeParams = 8;
        //
        // Set up one buffer for the type's generic params
        mdToken TypeParamTokensScratch[MaxScratchTypeParams];
        mdToken *TypeParamTokens = TypeParamTokensScratch;
        if (TypeParamCount > MaxScratchTypeParams)
        {
            TypeParamTokens = (mdToken *)m_nraScratch.Alloc(VBMath::Multiply(TypeParamCount, sizeof(mdToken)));
        }
        IfFailThrow(m_pmdImport2->EnumGenericParams(
                        &EnumTypeParams,
                        SymbolToken,
                        TypeParamTokens,
                        TypeParamCount,
                        &TypeParamCount));
        //
        // And another buffer of the container's generic params (to compare them)
        // We're going to compare the things metadata flags against its container's metadata flags.
        // This was easiest to implement. It would have been possible to compare the flags after
        // they've already been parsed and stored in BCSYM structures, but constraint flags are
        // stored in a list of pointers rather than a bitmask and are a bit awkward to work with.
        mdToken ContainerTypeParamTokensScratch[MaxScratchTypeParams];
        mdToken *ContainerTypeParamTokens = ContainerTypeParamTokensScratch;
        if (ContainerTypeParamCount > MaxScratchTypeParams)
        {
            ContainerTypeParamTokens = (mdToken *)m_nraScratch.Alloc(VBMath::Multiply(ContainerTypeParamCount, sizeof(mdToken)));
        }
        if (ContainerTypeParamCount > 0)
        {
            HCORENUM EnumContainerTypeParams = 0;
            IfFailThrow(m_pmdImport2->EnumGenericParams(
                        &EnumContainerTypeParams,
                        Container->GetToken(),
                        ContainerTypeParamTokens,
                        ContainerTypeParamCount,
                        &ContainerTypeParamCount));
            m_pmdImport->CloseEnum(EnumContainerTypeParams);
        }


        // Now we're going to loop through all type parameters.
        // For 0 <= TypeParamIndex < ContainerTypeParamCount, we'll check that it's a copy of the parent.
        // For ContainerTypeParamCount <= TypeParamIndex < TypeParamCount, we'll add it to our list
        // of constructed generic params.
        //
        bool GenericsViolateCLS = false;
        BCSYM_GenericParam *FirstGenericParam = NULL;
        BCSYM_GenericParam **GenericParamTarget = &FirstGenericParam;
        
        GenericsViolateCLS |= (TypeParamCount < ContainerTypeParamCount);

   
        for (unsigned TypeParamIndex = 0; TypeParamIndex < min(TypeParamCount,ContainerTypeParamCount); TypeParamIndex++)
        {

#pragma prefast(suppress: 26010, "TypeParamTokens is correctly allocated above")
            mdGenericParam md = (mdGenericParam)TypeParamTokens[TypeParamIndex];
#pragma prefast(suppress: 26010, "ContainerTypeParamTokens is correctly allocated above")
            mdGenericParam mdContainer = (mdGenericParam)ContainerTypeParamTokens[TypeParamIndex];
            DWORD Flags, ContainerFlags;

            IfFailThrow(m_pmdImport2->GetGenericParamProps(md,NULL,&Flags,NULL,NULL,NULL,0,NULL));
            IfFailThrow(m_pmdImport2->GetGenericParamProps(mdContainer,NULL,&ContainerFlags,NULL,NULL,NULL,0,NULL));

            GenericsViolateCLS |= ((Flags & gpSpecialConstraintMask) != (ContainerFlags & gpSpecialConstraintMask));
            GenericsViolateCLS |= ((Flags & gpVarianceMask) != (ContainerFlags & gpVarianceMask));
        }



        for (unsigned TypeParamIndex = ContainerTypeParamCount; TypeParamIndex < TypeParamCount; TypeParamIndex++)
        {
            // 


            const unsigned MaxParamNameLength = 512;
            WCHAR ParamName[MaxParamNameLength];
            unsigned long ParamNameLength;
            DWORD Flags = 0;

#pragma prefast(suppress: 26010, "TypeParamTokens is correctly allocated above")
            IfFailThrow(
                m_pmdImport2->GetGenericParamProps(
                    (mdGenericParam)TypeParamTokens[TypeParamIndex],
                    NULL,
                    &Flags,
                    NULL,
                    NULL,
                    ParamName,
                    MaxParamNameLength,
                    &ParamNameLength));

            Variance_Kind Variance;
            switch (Flags & gpVarianceMask)
            {
                case gpCovariant:
                    Variance = Variance_Out;
                    break;
                case gpContravariant:
                    Variance = Variance_In;
                    break;
                case gpNonVariant:
                    Variance = Variance_None;
                    break;
                default:
                    Variance = Variance_None;
                    // The CLI design of variance to date has been that if a compiler doesn't understand
                    // variance, then it can safely ignore it, i.e. treat the imported metadata as
                    // invariant. We trust that this design will continue into the future should there
                    // be any additional weirdo variance kinds introduced.
                    ;
            }

            BCSYM_GenericParam *Param =
                m_Symbols.GetGenericParam(
                    NULL,
                    m_pCompiler->AddString(ParamName),
                    TypeParamIndex - ContainerTypeParamCount,
                    Symbol->IsProc(),
                    Variance
                    );

            Param->SetMetaDataPosition(TypeParamIndex);
            Symbols::SetToken(Param, TypeParamTokens[TypeParamIndex]);

            // Build the "New", "Class" and "Structure" constraints from the flags
            SetupGenericParameterNonTypeConstraints(Param, Flags);

            *GenericParamTarget = Param;
            GenericParamTarget = Param->GetNextParamTarget();
        }

        if (FirstGenericParam!=NULL)
        {
            m_Symbols.SetGenericParams(FirstGenericParam, Symbol);
        }
        m_pmdImport->CloseEnum(EnumTypeParams);


        // Unmangle the generic type name
        //
        if (Symbol->IsType() && TypeParamCount > ContainerTypeParamCount)
        {
            unsigned GenericParamCountForThisType = TypeParamCount - ContainerTypeParamCount;

            Symbol->SetName(
                GetActualTypeNameFromEmittedTypeName(
                    Symbol->GetEmittedName(),
                    GenericParamCountForThisType,
                    m_pCompiler));
        }

        // Mark as bad if necessary
        //
        if (GenericsViolateCLS)
        {
            StringBuffer TextBuffer;
            Symbol->GetBasicRep(m_pCompiler, NULL, &TextBuffer, NULL, NULL, false);
            STRING *strSymbol = m_pCompiler->AddString(TextBuffer.GetString());
            //
            Symbol->SetIsBad();
            Symbol->SetErrid(ERRID_NestingViolatesCLS1);
            Symbol->SetBadName(strSymbol);
            Symbol->SetBadNameSpace(Symbol->GetNameSpace());
        }

    }
}


void
MetaImport::SetupGenericParameterTypeConstraints
(
    BCSYM_GenericParam *ListOfGenericParameters,
    BCSYM_NamedRoot **ppBadTypeSymbol
)
{
    // Backwards compatibility with the Everett CLR
    if (m_pCompilerHost->GetRuntimeVersion() >= RuntimeVersion2)
    {
        AssertIfNull(m_pmdImport2);

        for (BCSYM_GenericParam *Param = ListOfGenericParameters; Param; Param = Param->GetNextParam())
        {
            HCORENUM EnumConstraints = 0;

            // Find out how many constraints there are for this generic param.

            unsigned long ConstraintCount = 0;
            IfFailThrow(m_pmdImport2->EnumGenericParamConstraints(&EnumConstraints, Param->GetToken(), NULL, 0, &ConstraintCount));
            IfFailThrow(m_pmdImport->CountEnum(EnumConstraints, &ConstraintCount));
            m_pmdImport->CloseEnum(EnumConstraints);
            EnumConstraints = 0;

            if (ConstraintCount == 0)
            {
                // The generic param does not have any constraints
                //
                continue;
            }

            const unsigned MaxScratchConstraints = 8;
            mdGenericParamConstraint ConstraintTokensScratch[MaxScratchConstraints];
            mdGenericParamConstraint *ConstraintTokens = ConstraintTokensScratch;

            if (ConstraintCount > MaxScratchConstraints)
            {
                ConstraintTokens = (mdGenericParamConstraint *)m_nraScratch.Alloc(VBMath::Multiply(
                    ConstraintCount, 
                    sizeof(mdGenericParamConstraint)));
            }

            IfFailThrow(
                m_pmdImport2->EnumGenericParamConstraints(
                    &EnumConstraints,
                    Param->GetToken(),
                    ConstraintTokens,
                    ConstraintCount,
                    &ConstraintCount));

            // The Non-Type constraints if any will already have been setup earlier

            BCSYM_GenericConstraint *Constraints = NULL;
            BCSYM_GenericConstraint **ConstraintTarget = &Constraints;

            bool IsValueConstraintSet = Param->HasValueConstraint();

            for (unsigned ConstraintIndex = 0; ConstraintIndex < ConstraintCount; ConstraintIndex++)
            {
                mdGenericParam ParamToken;
                mdToken ConstraintToken;
                BCSYM *ConstraintType;

#pragma prefast(suppress: 26010, "ConstraintTokens is correctly allocated above")
                IfFailThrow(
                    m_pmdImport2->GetGenericParamConstraintProps(
                        ConstraintTokens[ConstraintIndex],
                        &ParamToken,
                        &ConstraintToken));

                ConstraintType = GetTypeOfToken(ConstraintToken, 0);
                VSASSERT(ConstraintType, "How can we not find this?");

                // Dev10 #680929: Be ready to deal with NamedType wrapper here. 
                if ( ConstraintType!=NULL && ConstraintType->IsNamedType() )
                {
                    ConstraintType = ConstraintType->PNamedType()->GetSymbol();

                    ThrowIfNull(ConstraintType);
                }

                // Make sure that ValueTypes don't show up as constraints. We should have
                // set this up earlier as a special constraint.
                if (!IsValueConstraintSet || 
                    (ConstraintType && ConstraintType != m_pCompilerHost->GetFXSymbolProvider()->GetType(FX::ValueTypeType)))
                {
                    *ConstraintTarget = m_Symbols.GetGenericTypeConstraint(NULL, ConstraintType);

                    if (ConstraintType->IsBad())
                    {
                        (*ConstraintTarget)->SetIsBadConstraint(true);

                        if (*ppBadTypeSymbol == NULL)
                        {
                            *ppBadTypeSymbol = ConstraintType->PNamedRoot();
                        }
                    }

                    ConstraintTarget = (*ConstraintTarget)->GetNextConstraintTarget();
                }
            }

            *ConstraintTarget = Param->GetConstraints();
            Param->SetConstraints(Constraints);

            m_pmdImport->CloseEnum(EnumConstraints);
        }
    }
}

void
MetaImport::SetupGenericParameterNonTypeConstraints
(
    _Inout_ BCSYM_GenericParam *Parameter,
    DWORD GenericParamFlags
)
{
    VSASSERT(
        m_pCompilerHost->GetRuntimeVersion() >= RuntimeVersion2,
        "Generic parameter cracking shouldn't happen for the Everett CLR");

    if (GenericParamFlags == gpNoSpecialConstraint)
    {
        return;
    }

    BCSYM_GenericConstraint *Constraints = NULL;
    BCSYM_GenericConstraint **ConstraintTarget = &Constraints;

    // Only allow the new constraint if the ValueTypeConstraint is not applied.
    // This is a workaround to correctly import C# constraints..
    if (GenericParamFlags & gpDefaultConstructorConstraint && !(GenericParamFlags & gpNotNullableValueTypeConstraint))
    {
        *ConstraintTarget = m_Symbols.GetGenericNonTypeConstraint(NULL, BCSYM_GenericNonTypeConstraint::ConstraintKind_New);
        ConstraintTarget = (*ConstraintTarget)->GetNextConstraintTarget();
    }

    if (GenericParamFlags & gpReferenceTypeConstraint)
    {
        *ConstraintTarget = m_Symbols.GetGenericNonTypeConstraint(NULL, BCSYM_GenericNonTypeConstraint::ConstraintKind_Ref);
        ConstraintTarget = (*ConstraintTarget)->GetNextConstraintTarget();
    }

    if (GenericParamFlags & gpNotNullableValueTypeConstraint)
    {
        *ConstraintTarget = m_Symbols.GetGenericNonTypeConstraint(NULL, BCSYM_GenericNonTypeConstraint::ConstraintKind_Value);
        ConstraintTarget = (*ConstraintTarget)->GetNextConstraintTarget();
    }

    *ConstraintTarget = Parameter->GetConstraints();
    Parameter->SetConstraints(Constraints);
}

BCSYM *
MetaImport::BuildGenericBinding
(
    BCSYM *Generic,
    unsigned long ArgumentCount,
    _In_count_(ArgumentCount) BCSYM **Arguments,
    bool AllocAndCopyArgumentsToNewList
)
{
    if (Generic->IsGenericBadNamedRoot())
    {
        return Generic;
    }

    for(unsigned long Index = 0; Index < ArgumentCount; Index++)
    {
        VSASSERT(Arguments[Index] != NULL, "NULL type argument unexpected!!!");

        if (Arguments[Index]->IsGenericBadNamedRoot())
        {
            // 


            // Propagate badness
            return Arguments[Index];
        }
    }

    unsigned long StartIndex = 0;
    return
        BuildGenericBindingHelper(
            Generic,
            ArgumentCount,
            Arguments,
            StartIndex,
            AllocAndCopyArgumentsToNewList);
}

BCSYM *
MetaImport::BuildGenericBindingHelper
(
    BCSYM *Generic,
    unsigned long ArgumentCount,
    _In_count_(ArgumentCount) BCSYM **Arguments,
    _Inout_ unsigned long &StartIndex,
    bool AllocAndCopyArgumentsToNewList
)
{
    VSASSERT(Generic, "NULL generic unexpected!!!");
    VSASSERT(!Generic->IsGenericBadNamedRoot(), "Bad NamedRoot unexpected!!!");

    BCSYM_GenericTypeBinding *ParentBinding = NULL;
    unsigned GenericParamCount = Generic->GetGenericParamCount();

    VSASSERT(ArgumentCount != 0, "Generic with zero type arguments unexpected!!!");

    VSASSERT(GenericParamCount <= ArgumentCount,
                "Generic instantiation with more type arguments than type parameters unexpected!!!");

    if (GenericParamCount < ArgumentCount)
    {
        ParentBinding =
            BuildGenericBindingHelper(
                Generic->PNamedRoot()->GetContainer(),
                ArgumentCount - GenericParamCount,
                Arguments,
                StartIndex,
                AllocAndCopyArgumentsToNewList)->PGenericTypeBinding();
    }

    VSASSERT(GenericParamCount <= ArgumentCount - StartIndex, "Generic instantiation import lost!!!");

    BCSYM_GenericTypeBinding *Result =
        m_Symbols.GetGenericBinding(
            false,
            Generic->PNamedRoot(),
            GenericParamCount > 0 ? Arguments + StartIndex : NULL,
            GenericParamCount,
            ParentBinding,
            AllocAndCopyArgumentsToNewList)->PGenericTypeBinding();

    StartIndex += GenericParamCount;

    return Result;
}

//============================================================================
//============================================================================
BCSYM *
MetaImport::BindTypeInVBProject
(
    CompilerProject *pVBProject,
    _In_z_ STRING *pstrNamespace,
    _In_z_ STRING *pstrTypeName,
    unsigned cTypeArity
)
{
    ULONG ulItems = 0;
    STRING **rgpstrQualifiedName = NULL;
    NorlsAllocator nraSymbols(NORLSLOC);
    Symbols symbols(m_pCompiler, &nraSymbols, NULL);

    // First, breakup the qualified name
    if (FAILED(BreakUpQualifiedName(m_pCompiler, &rgpstrQualifiedName, &nraSymbols, &ulItems, pstrNamespace, DOT_DELIMITER)))
    {
        return NULL;
    }

    BCSYM_Namespace *pNamespace = FindNameSpaceInProject(pVBProject, rgpstrQualifiedName, ulItems);

    if (pNamespace)
    {
        return
            LookupInNamespace(
                pstrTypeName,
                pNamespace,
                false,  // Do not match against namespaces
                true,
                cTypeArity);  // Match against types.
    }

    return NULL;
}

//============================================================================
//============================================================================
BCSYM_Namespace *
MetaImport::FindNameSpaceInProject
(
    CompilerProject *pProject,
    _In_count_(TotalNumberOfQualifiedNameComponents) STRING **rgpstrQualifiedName,
    ULONG TotalNumberOfQualifiedNameComponents
)
{
    BCSYM_Namespace *pCurrentNamespace = m_pCompiler->GetUnnamedNamespace(pProject);

    for(ULONG CurrentIndex = 0; CurrentIndex < TotalNumberOfQualifiedNameComponents; CurrentIndex++)
    {
        if (!pCurrentNamespace)
        {
            break;
        }

        pCurrentNamespace =
            LookupInNamespace(
                rgpstrQualifiedName[CurrentIndex],
                pCurrentNamespace,
                true,                       // Match against namespaces
                false,                      // Do not match against types
                0)->PNamespace();
    }

    return pCurrentNamespace;
}

BCSYM_NamedRoot *
MetaImport::LookupInNamespace
(
    _In_z_ STRING *pstrName,
    BCSYM_Namespace *pNamespace,
    bool fMatchNamespace,
    bool fMatchType,
    unsigned cTypeArity
)
{
    Namespace *pCurrentNamespace = pNamespace;
    BCSYM_NamedRoot *pMatchingSymbol = NULL;

    do
    {
        BCSYM_NamedRoot *pSymbol = pCurrentNamespace->SimpleBind(NULL, pstrName);

        if (pSymbol)
        {
            if (fMatchNamespace && pSymbol->IsNamespace())
            {
                pMatchingSymbol = pSymbol;
                break;
            }

            if (fMatchType &&
                pSymbol->IsType() &&
                pSymbol->GetGenericParamCount() == cTypeArity)
            {
                pMatchingSymbol = pSymbol;
                break;
            }
        }

        pCurrentNamespace = pCurrentNamespace->GetNextNamespaceInSameProject();
    }
    while (pCurrentNamespace != pNamespace);

    if (pMatchingSymbol && pMatchingSymbol->IsBad())
    {
        // 
        pMatchingSymbol = NULL;
    }

    return pMatchingSymbol;
}

//============================================================================
// Get the properties for a type def.
//============================================================================

bool MetaImport::GetTypeDefProps
(
    MetaType *rgtypes,
    unsigned cTypes,
    bool fMissingTypes,
    mdTypeDef tdef,
    bool *pIsClass,
    _Deref_opt_out_z_ STRING **ppstrName,
    _Deref_opt_out_z_ STRING **ppstrNameSpace,
    DECLFLAGS *pDeclFlags,
    mdToken *ptkExtends,
    mdTypeDef *ptdNestingContainer
)
{
    unsigned long cchTypeName, cchTypeName2;
    WCHAR *wszTypeName = NULL;
    WCHAR wsz[256];
    DWORD dwFlags;
    bool IsNested = false;

    STRING *pstrNameSpace, *pstrUnqualName;

    HRESULT hr = m_pmdImport->GetTypeDefProps(
            tdef,
            wsz,
            sizeof(wsz) / sizeof(WCHAR),
            &cchTypeName,
            &dwFlags,
            ptkExtends);

    if(CLDB_E_INDEX_NOTFOUND == hr)
    {
        ASSERT(m_pMetaDataFile != NULL && m_pMetaDataFile->GetErrorTable() != NULL, "why m_pMetaFile is NULL or errortable is NULL?");
        m_pMetaDataFile->GetErrorTable()->CreateErrorWithError(
            ERRID_BadMetaFile,
            NULL, 
            hr,
            m_pMetaDataFile->GetName());
        return false;
    }
    else
    {
        IfFailThrow(hr);
    }

    *pDeclFlags = MapTypeFlagsToDeclFlags(dwFlags, m_pMetaDataFile->GetProject()->IsCodeModule());
    *pIsClass = !(dwFlags & tdInterface);

    switch(dwFlags & tdVisibilityMask)
    {
    case tdNestedPrivate:
    case tdNestedFamORAssem:
    case tdNestedFamily:
    case tdNestedPublic:
    case tdNestedAssembly:
    case tdNestedFamANDAssem:
        IsNested = true;
        break;
    }

    if (cchTypeName > (sizeof(wsz) / sizeof(WCHAR)))
    {
        // Allocate room for the typename string.
        wszTypeName = (WCHAR *)m_nraScratch.Alloc(VBMath::Multiply(cchTypeName, sizeof(WCHAR)));

        // Get the fields.
        IfFailThrow(m_pmdImport->GetTypeDefProps(tdef,
                                                 wszTypeName,
                                                 cchTypeName,
                                                 &cchTypeName2,
                                                 &dwFlags,
                                                 ptkExtends));

        VSASSERT(cchTypeName == cchTypeName2, "COM+ lied.");
    }
    else
    {
        wszTypeName = wsz;
    }

    SplitTypeName(m_pCompiler, wszTypeName, &pstrNameSpace, &pstrUnqualName);

    // Save the strings.
    if (ppstrName)
        *ppstrName = pstrUnqualName;
    if (ppstrNameSpace)
        *ppstrNameSpace = pstrNameSpace;

    if (IsNested)
    {
        IfFailThrow(m_pmdImport->GetNestedClassProps(tdef, ptdNestingContainer));

        // Sneaky trick here. There's no rule that I know of that says that a compiler
        // has to guarantee that it emits the outer type for a nested type before the
        // nested type. However, most compilers do. So if we should have seen the outer
        // container first, we can figure out if the outer container was skipped because
        // it wasn't accessible and use that here.
        if (RidFromToken(tdef) > RidFromToken(*ptdNestingContainer))
        {
            MetaType *pType = FindType(*ptdNestingContainer, rgtypes, cTypes, fMissingTypes);

            if (!pType || !pType->m_pContainer)
                return false;
        }
    }

    return true;
}

//============================================================================
// Returns true and get the properties if the exported type is a valid type
// forwarder, else returns false.
//============================================================================

bool MetaImport::GetTypeForwarderProps
(
    CComPtr<IMetaDataAssemblyImport> srpAssemblyImport,
    mdExportedType tkExp,
    _Deref_opt_out_z_ STRING **ppstrName,
    _Deref_opt_out_z_ STRING **ppstrNameSpace,
    mdAssemblyRef *ptkAssemblyRef
)
{
    unsigned long cchTypeName, cchTypeName2;
    WCHAR *wszTypeName = NULL;
    WCHAR wsz[256];
    DWORD dwFlags;
    bool IsNested = false;

    STRING *pstrNameSpace, *pstrUnqualName;

    IfFailThrow(srpAssemblyImport->GetExportedTypeProps(tkExp,
                                                        wsz,
                                                        DIM(wsz),
                                                        &cchTypeName,
                                                        ptkAssemblyRef,
                                                        NULL,
                                                        &dwFlags));

    // Ignore non-type forwarders.
    //
    // Ignore type-forwarders with destination other than Assemblyref because they
    // are anyway unsupported for whidbey by the clr. This results in ignoring
    // type forwarders for nested types too whose destination is mdtExportedType.
    // This is intended because nested types will be found through the forwarding
    // information of the parent type.
    //
    if (!(dwFlags & tdForwarder) ||
        TypeFromToken(*ptkAssemblyRef) != mdtAssemblyRef)
    {
        return false;
    }

    if (cchTypeName > DIM(wsz))
    {
        // Allocate room for the typename string.
        wszTypeName = (WCHAR *)m_nraScratch.Alloc(VBMath::Multiply(cchTypeName, sizeof(WCHAR)));

        // Get the fields.
        IfFailThrow(srpAssemblyImport->GetExportedTypeProps(tkExp,
                                                            wszTypeName,
                                                            cchTypeName,
                                                            &cchTypeName2,
                                                            ptkAssemblyRef,
                                                            NULL,
                                                            &dwFlags));

        VSASSERT(cchTypeName == cchTypeName2, "COM+ lied.");
    }
    else
    {
        wszTypeName = wsz;
    }

    SplitTypeName(m_pCompiler, wszTypeName, &pstrNameSpace, &pstrUnqualName);

    // Save the strings.
    if (ppstrName)
        *ppstrName = pstrUnqualName;
    if (ppstrNameSpace)
        *ppstrNameSpace = pstrNameSpace;

    return true;
}

//============================================================================
// Does the actual work of importing the base and implements
//============================================================================
void MetaImport::DoImportBaseAndImplements
(
    _In_ BCSYM_Container *Type
)
{
    BCSYM_Container *OldContainer = m_pcontainer;

    VSASSERT(!Type->AreBaseAndImplementsLoading(), "bad PE!");

    if (Type->AreBaseAndImplementsLoaded() || Type->AreBaseAndImplementsLoading())
    {
        return;
    }
    else
    {
        VSASSERT(!Type->IsObject(),
                 "Object should be marked as bases and implements already loaded (since it has none).");
    }

    m_pcontainer = Type;
    Symbols::SetBaseAndImplementsLoading(Type, true);
    BCSYM_NamedRoot *pBadTypeSymbol = NULL;

    // Bind the constraint types for any generic parameters. This has to occur after
    // the parent of the type is available and all the types have been imported,
    // so that binding is possible.
    //
    SetupGenericParameterTypeConstraints(Type->GetFirstGenericParam(), &pBadTypeSymbol);

    if (pBadTypeSymbol)
    {
        TransferBadSymbol(Type, pBadTypeSymbol);
    }

    switch(Type->GetKind())
    {
    case SYM_Interface:
        {
            BCSYM_Implements *Implements;
            BCSYM_NamedRoot *BadType = NULL;

            GetImplementsList(Type->PInterface()->GetTypeDef(), &Implements, &BadType, false);

            if (BadType)
            {
                // Whoops, bad. Transfer badness
                TransferBadSymbol(Type, BadType);

                if (Type->GetErrid() == ERRID_UnreferencedAssembly3)
                    Type->SetErrid(ERRID_UnreferencedAssemblyImplements3);
                else if (Type->GetErrid() == ERRID_UnreferencedModule3)
                    Type->SetErrid(ERRID_UnreferencedModuleImplements3);
            }
            else
            {
                Symbols::SetImplements(Type->PInterface(), Implements);
            }
        }
        break;

    case SYM_Class:
        {
            BCSYM_Class *Class = Type->PClass();
            mdTypeDef TypeToken = Class->GetTypeDef();
            BCSYM_Implements *Implements;
            BCSYM *BaseType = NULL;
            BCSYM_NamedRoot *BadType = NULL;

            mdTypeRef BaseClassToken = Class->GetBaseClassToken();

            // Short circuit the most common lookup case
            if (m_pMetaDataFile->GetProject() == m_pCompilerHost->GetComPlusProject() &&
                BaseClassToken == m_pCompilerHost->GetFXSymbolProvider()->GetType(FX::ObjectType)->GetToken())
            {
                BaseType = m_pCompilerHost->GetFXSymbolProvider()->GetType(FX::ObjectType);
            }
            else
            {
                // Microsoft:
                // For multitargeting, you may be in a scenario where you reference two different
                // mscorlibs. Only one is the "ComPlusProject", and the other is not. So there will
                // be at least one System.Object that falls in here. This can also happen if you manage
                // to force two mscorlibs to load (the project system is supposed to prevent that).
                // So, we will still assert, but we will NOT call GetTypeOfToken, since that will
                // throw an exception in retail.

                VSASSERT(!IsNilToken(BaseClassToken), "Class doesn't have a base class! If you are multitargeting, this can be because you are loading two System.Object - unsupported. Make sure only one instance of mscorlib is loaded.");

                if (!IsNilToken(BaseClassToken))
                {
                    BaseType = GetTypeOfToken(BaseClassToken, 0, true);
                }

                if (!BaseType)
                    BaseType = m_pCompilerHost->GetFXSymbolProvider()->GetType(FX::ObjectType);
            }

            VSASSERT(BaseType, "must have a base class!");

            // attempt to fail gracefully
            if (BaseType == NULL)
            {
                BaseType = m_pCompilerHost->GetFXSymbolProvider()->GetType(FX::ObjectType);
            }

            if (BaseType->IsBad())
            {
                // Whoops, bad. Transfer badness
                TransferBadSymbol(Type, BaseType->PNamedRoot());

                if (Type->GetErrid() == ERRID_UnreferencedAssembly3)
                    Type->SetErrid(ERRID_UnreferencedAssemblyBase3);
                else if (Type->GetErrid() == ERRID_UnreferencedModule3)
                    Type->SetErrid(ERRID_UnreferencedModuleBase3);

                goto Exit;
            }

            VSASSERT(BaseType->IsClass(), "must be class!");

            if (BaseType == m_pCompilerHost->GetFXSymbolProvider()->GetType(FX::EnumType))
            {
                // Enum
                Class->SetIsEnum(true);
            }
            else if (BaseType == m_pCompilerHost->GetFXSymbolProvider()->GetType(FX::MultiCastDelegateType) ||
                     BaseType == m_pCompilerHost->GetFXSymbolProvider()->GetType(FX::DelegateType))
            {
                // Delegate
                Class->SetIsDelegate(true);
            }
            else if (BaseType == m_pCompilerHost->GetFXSymbolProvider()->GetType(FX::ValueTypeType) &&
                     !(Class == m_pCompilerHost->GetFXSymbolProvider()->GetType(FX::EnumType)))
            {
                // Struct
                Class->SetIsStruct(true);
            }

            // Get the implemented interfaces
            GetImplementsList(TypeToken, &Implements, &BadType, true);

            if (BadType)
            {
                // Whoops, bad. Transfer badness
                TransferBadSymbol(Type, BadType);

                if (Type->GetErrid() == ERRID_UnreferencedAssembly3)
                    Type->SetErrid(ERRID_UnreferencedAssemblyImplements3);
                else if (Type->GetErrid() == ERRID_UnreferencedModule3)
                    Type->SetErrid(ERRID_UnreferencedModuleImplements3);

                goto Exit;
            }

            Symbols::SetBaseClass(Class, BaseType);

            Symbols::SetImplements(Class, Implements);
        }
        break;

    case SYM_GenericBadNamedRoot:
    case SYM_TypeForwarder:
        VSASSERT(Type->IsBad(), "must be bad!");
        break;

    default:
        VSFAIL("How did we get here?");
    }

Exit:
    m_pcontainer = OldContainer;
    Symbols::SetBaseAndImplementsLoaded(Type);
    Symbols::SetBaseAndImplementsLoading(Type, false);

    if (Type->IsClass())
    {
        Bindable::DetermineIfAttribute(Type->PClass(), m_pCompilerHost);
    }

#if DEBUG
    if (Type->IsInterface() &&
        m_pCompilerHost->GetFXSymbolProvider()->IsTypeAvailable(FX::GenericIListType) &&
        Type->PInterface() == m_pCompilerHost->GetFXSymbolProvider()->GetType(FX::GenericIListType))
    {
        VerifyGenericIListBases(Type->PInterface());
    }
#endif DEBUG
}

#if DEBUG
// This check is needed because conversions, constraint checking, etc. assume that
// the only generic interfaces from which Generic.IList inherits from are Generic.ICollection
// and Generic.IEnumerable and their type arguments are the type parameter of Generic.IList.
//
// If this assumption ever changes, then the conversions code, constraint checking code, etc.
// will need to be updated.
//
void
MetaImport::VerifyGenericIListBases
(
    BCSYM_Interface *pInterface
)
{
    for(BCSYM_Implements *pBase = pInterface->GetFirstImplements();
        pBase;
        pBase = pBase->GetNext())
    {
        if (pBase->IsBadImplements() ||
            pBase->GetCompilerRoot()->IsBad() ||
            !pBase->GetCompilerRoot()->IsInterface())
        {
            continue;
        }

        BCSYM *pBaseInterface = pBase->GetCompilerRoot();

        if (pBaseInterface->IsGenericTypeBinding())
        {
            BCSYM_Interface *pGenericBaseInterface =
                pBaseInterface->PGenericTypeBinding()->GetGeneric()->PInterface();

            // This assertion is needed because conversions, constraint checking, etc. assume that
            // the only generic interfaces from which Generic.IList inherits from are Generic.ICollection and
            // Generic.IEnumerable and their type arguments are the type parameter of Generic.IList.
            //
            // If this assumption ever changes, then the conversions code, constraint checking code, etc.
            // will need to be updated.
            //
            VSASSERT(
                (pGenericBaseInterface == m_pCompilerHost->GetFXSymbolProvider()->GetType(FX::GenericIEnumerableType) &&
                    pGenericBaseInterface->GetGenericParamCount() == 1 &&
                    pBaseInterface->PGenericBinding()->GetArgument(0) == pInterface->GetFirstGenericParam()) ||
                (pGenericBaseInterface == m_pCompilerHost->GetFXSymbolProvider()->GetType(FX::GenericICollectionType) &&
                    pGenericBaseInterface->GetGenericParamCount() == 1 &&
                    pBaseInterface->PGenericBinding()->GetArgument(0) == pInterface->GetFirstGenericParam()),
                "Generic.IList inherits from unexpected generic interface!!!");
        }

        VerifyGenericIListBases(pBaseInterface->PInterface());
    }
}
#endif DEBUG

//============================================================================
// Get the implements list.
//
// If we fail to resolve one of the interfaces, then we will return the bad
// named root corresponding to it instead. It would be difficult/impossible
// to report all the resolution failures, so we only report the first one.
//============================================================================

void MetaImport::GetImplementsList
(
    mdTypeDef tdef,
    _Out_opt_ BCSYM_Implements **p----l,
    _Out_ BCSYM_NamedRoot **ppBadNamedRoot,
    bool IgnoreInaccessibleTypes
)
{
    CorEnum hEnum(m_pmdImport);
    // Assume the worst.
    if (p----l)
    {
        *p----l = NULL;
    }

    VSASSERT(ppBadNamedRoot, "have to handle failure!");
    *ppBadNamedRoot = NULL;

    // Load the implements.
    // The first implemented interface is the default interface, if the class has custom attribute: "ClassInterface"
    for (;;)
    {
        mdInterfaceImpl impl;
        unsigned long cImpls;

        mdTypeDef tdef2;
        mdToken tk;

        BCSYM *psym;

        // Get the next implements.
        IfFailThrow(m_pmdImport->EnumInterfaceImpls(&hEnum, tdef, &impl, 1, &cImpls));

        // Are we done?
        if (cImpls == 0)
        {
            break;
        }

        // Get the information about the implements.
        IfFailThrow(m_pmdImport->GetInterfaceImplProps(impl,
                                                       &tdef2,
                                                       &tk));

        VSASSERT(tdef2 == tdef, "GetImplementsList: How can this differ?");

        // Get the symbol for the interface.
        psym = GetTypeOfToken(tk, 0, true);

        // Add the implements symbol.
        if (psym)
        {
            BCSYM * pImplementsSymbol = psym;

            // Dev10 #680929: Be ready to deal with NamedType wrapper here. 
            if ( psym->IsNamedType() )
            {
                psym = psym->PNamedType()->GetSymbol();

                ThrowIfNull(psym);
            }
        
            if (!psym->IsBad())
            {
                if (p----l)
                {
                    m_Symbols.GetImplements(NULL, psym->PNamedRoot()->GetName(), pImplementsSymbol, p----l);
                }
            }
            else if (IgnoreInaccessibleTypes && psym->IsNamedRoot() && !Bindable::IsAccessibleOutsideAssembly(psym->PNamedRoot()))
            {
                // Ignore inaccessible types in the implements list for classes
                // Bug VSWhidbey 487686
            }
            else
            {
                *ppBadNamedRoot = psym->PNamedRoot();
                return;
            }
        }
    }   // for
}

//****************************************************************************
// Load the type-level information.
//****************************************************************************

//============================================================================
// Update the information for a type
//============================================================================

void MetaImport::DoImportTypeChildren
(
    BCSYM_Container *pContainer
)
{
    WCHAR *pwszDefaultMember;
    Vtypes vtUnderlyingType;

    m_pcontainer = pContainer;
    m_td = pContainer->GetTypeDef();

#if FV_TRACK_MEMORY
    DebPrintf("Loading children for '%S.%S' in '%S'.\n", m_pcontainer->GetNameSpace(), m_pcontainer->GetName(), m_pcontainer->GetContainingProject()->GetAssemblyName());
#endif

    // Remember that we're loading to prevent nasty cycles.
    Symbols::SetChildrenLoading(pContainer, true);

    m_pcontainer->GetPWellKnownAttrVals()->GetDefaultMemberData(&pwszDefaultMember);

    // All types contain a list of members.
    LoadChildren(&vtUnderlyingType);

    if (m_pcontainer->IsClass() && m_pcontainer->PClass()->IsEnum())
    {
        CoerceEnumeratorValues(
            vtUnderlyingType,
            ConcatNameSpaceAndName(m_pCompiler, m_pcontainer->GetNameSpace(), m_pcontainer->GetName()));
    }
    
    SymbolList symlist;
    SymbolList unBindableSymList;

    // Load the properties, hooking them up with their implementing methods.
    LoadProperties(&symlist, pwszDefaultMember);

    // Load the events.
    LoadEvents(&symlist);
    
    // If the type is a WinRT type then check to see if we need to fake up any interface
    // implementations.
    if (m_pcontainer->GetPWellKnownAttrVals()->GetWindowsRuntimeImportData() && !m_pcontainer->IsInterface())
    {
        AddFakeWindowsRuntimeInterfaceMembers(&symlist, &unBindableSymList);
    }

    // Finish loading the children.
    FinishChildren(&symlist, &unBindableSymList);

    // Do the per-type initialization.
    switch(m_pcontainer->GetKind())
    {
    case SYM_Interface:
        Bindable::BindMetaDataContainer(m_pcontainer, NULL);
        break;

    case SYM_Class:
        if (m_pcontainer->PClass()->IsEnum())
        {
            m_pcontainer->PClass()->SetVtype(vtUnderlyingType);
        }
        else
        {
            m_pcontainer->GetBindableInstance()->ValidateExtensionAttributeOnMetaDataClass(m_pcontainer->PClass());
            Bindable::BindMetaDataContainer(m_pcontainer, NULL);
        }
        break;

    default:
        VSFAIL("How did we get here?");
    }

    // We're fully loaded.
    Symbols::SetChildrenLoaded(m_pcontainer);

    // We're no longer loading.  Don't bother resetting this if we get an
    // error, we're already horked.
    //
    Symbols::SetChildrenLoading(m_pcontainer, false);
}

//============================================================================
// Load this type's children.
//============================================================================

void MetaImport::LoadChildren
(
    _Out_ Vtypes *pvtUnderlyingType
)
{
    CorEnum hEnum(m_pmdImport);

    *pvtUnderlyingType = t_i4;

    unsigned long cMembers;
    unsigned long iMember, iMemberWrite;
    MetaMember mm;
    mdToken tk;
    unsigned long cMemberDefs;
    bool fIsGenericStructure =
            m_pcontainer->IsClass() &&
            m_pcontainer->PClass()->IsStruct() &&
            (m_pcontainer->IsGeneric() || m_pcontainer->HasGenericParent());
#if IDE 
    // We don't know, when importing an assembly, whether it is going to be embedded (using /link)
    // or referenced (using /reference). In order to successfully embed a type, we need to record extra
    // information about the order of type members. We must therefore record this information for types
    // in any assembly which potentially could be embedded: that is, any assembly imported from a typelib.
    // We can't check the wellknownattrvals yet, because they haven't necessarily been ----ed, but the
    // assemblyidentity happens to record the presence of a typelib attribute, so we can check there.
    // Absence of correct ordering information was behind Dev10 bug #621561.
    bool fAllocLocations = m_pMetaDataFile->GetProject()->GetAssemblyIdentity()->IsImportedFromCOMTypeLib() ||
        m_pMetaDataFile->GetProject()->GetAssemblyIdentity()->IsPrimaryInteropAssembly();
#else
    // In the command-line compiler, we always know whether we intend to /link or /reference an assembly,
    // so we will only allocate locations for types from /linked assemblies.
    bool fAllocLocations = TypeHelpers::IsEmbeddableInteropType(m_pcontainer);
#endif

    //
    // Figure out how many members there are.
    //

    // Open the enum.
    IfFailThrow(m_pmdImport->EnumMembers(&hEnum,
                                         m_td,
                                         NULL,
                                         0,
                                         NULL));

    // Get the count.
    IfFailThrow(m_pmdImport->CountEnum(hEnum, &cMembers));

    // If this is a struct with explicit layout, which we are importing in No-PIA mode
    // (meaning we might have to re-emit it later), grab the struct layout data now.
    // We will store the field offset on each field as a FieldOffsetAttribute.
    DynamicHashTable<mdTokenKey, ULONG, NorlsAllocWrapper> *fieldOffsets = NULL;
    __int16  layout;

    if (fAllocLocations && m_pcontainer->IsStruct())
    {
        DWORD dwPackSize = 0;
        COR_FIELD_OFFSET *rFieldOffset = NULL;
        ULONG cFieldOffset = 0;
        ULONG ulClassSize = 0;

        if (cMembers > 0 &&
            m_pcontainer->GetPWellKnownAttrVals()->GetStructLayoutData(&layout) &&
            tdExplicitLayout == layout)
        {
            rFieldOffset = m_nraScratch.AllocArray<COR_FIELD_OFFSET>(cMembers);
        }

        HRESULT hr = m_pmdImport->GetClassLayout(
            m_td,
            &dwPackSize,
            rFieldOffset,
            rFieldOffset == NULL ? 0 : cMembers,
            &cFieldOffset,
            &ulClassSize);

        if (hr != CLDB_E_RECORD_NOTFOUND)
        {
            IfFailThrow(hr);

            // Dev10 #702321
            if (dwPackSize != 0 || ulClassSize != 0)
            {
                if (m_pcontainer->GetPWellKnownAttrVals() == NULL)
                {
                    m_pcontainer->AllocAttrVals(m_Symbols.GetNorlsAllocator(), CS_MAX, NULL, NULL, true);
                }

                m_pcontainer->GetPWellKnownAttrVals()->SetPackClassSize(dwPackSize, ulClassSize);
            }

            if (rFieldOffset != NULL && cFieldOffset > 0)
            {
                fieldOffsets = new (m_nraScratch)DynamicHashTable<mdTokenKey,ULONG,NorlsAllocWrapper>(
                    &m_nraScratch,
                    DefaultHashFunction<mdTokenKey>(),
                    cFieldOffset);
     
                for (ULONG foIndex = 0; foIndex < cFieldOffset; foIndex++)
                {
                    fieldOffsets->SetValue(rFieldOffset[foIndex].ridOfField, rFieldOffset[foIndex].ulOffset);
                }
            }
        }
    }

    if (cMembers == 0)
    {
        m_cMembers = 0;
        return;
    }

    //
    // Load the member properties.
    //

    m_rgmembers = (SavedMetaMember *)m_nraScratch.Alloc(VBMath::Multiply(
        cMembers, 
        sizeof(SavedMetaMember)));

    for (iMember = iMemberWrite = 0; iMember < cMembers; iMember++)
    {
        // Get the token.
        IfFailThrow(m_pmdImport->EnumMembers(&hEnum,
                                             m_td,
                                             &tk,
                                             1,
                                             &cMemberDefs));

        // Are we done?
        if (!cMemberDefs)
        {
            break;
        }

        // Create the appropriate symbol.
        if (TypeFromToken(tk) == mdtMethodDef)
        {
            // Get the properties for this type.
            if (!GetMemberProps(tk, &mm))
                continue;

            m_rgmembers[iMemberWrite].m_tk = mm.m_tk;
            m_rgmembers[iMemberWrite].m_pnamed = LoadMethodDef(&mm, fAllocLocations);
        }
        else
        {
            VSASSERT(TypeFromToken(tk) == mdtFieldDef, "What else could this be?");

            // Get the properties for this type.
            if (!GetMemberProps(tk, &mm))
            {
                // We want to skip this member, but we need to load members of structures
                // whose type is generic because it's possible you could use this to create
                // a structure cycle, such as with Nullable(Of T)
                if (fIsGenericStructure)
                {
                    bool FieldIsBad = false;

                    DecodeFieldSignature(mm.m_pSignature, mm.m_cbSignature, FieldIsBad);

                    if (FieldIsBad)
                        continue;
                }
                else
                    continue;
            }

            m_rgmembers[iMemberWrite].m_tk = mm.m_tk;
            m_rgmembers[iMemberWrite].m_pnamed = LoadFieldDef(&mm, pvtUnderlyingType, fAllocLocations);

            // If this field is a member of a structure with explicit layout, save the offset value
            // as a pseudo-custom FieldOffset attribute. The metadata docs appear to suggest that the
            // metadata importer will do this for us, but that is not true: the reflection API unpacks
            // pseudocustom attributes but the unmanaged IMetaDataImport interface does not. 
            ULONG fieldOffset = 0;
            if (fieldOffsets && fieldOffsets->GetValue(mm.m_tk, &fieldOffset))
            {
                BCSYM_NamedRoot *pnamed = m_rgmembers[iMemberWrite].m_pnamed;

                if (pnamed->GetPWellKnownAttrVals() == NULL)
                {
                    pnamed->AllocAttrVals(m_Symbols.GetNorlsAllocator(), CS_MAX, NULL, NULL, true);
                }
                pnamed->GetPWellKnownAttrVals()->SetFieldOffsetData(fieldOffset);
            }
        }

        // When importing symbols, we assign locations based on loading order to members of types loaded 
        // from PIAs using "link" mode (the No-PIA feature). When we go to emit local copies of these 
        // interop types, we will iterate through the type members using the DefineableMembersInAContainer
        // class, which sorts its output by location. Storing the input order here allows us to emit copies
        // of these members in the same order, which is essential for compatible field and vtable layout.
        if (fAllocLocations)
        {
            Location dummy;
            dummy.SetLocation(iMemberWrite,1);

            m_rgmembers[iMemberWrite].m_pnamed->SetLocation(&dummy);
        }

         // Next element.
        iMemberWrite++;
    }

    // Remember how many we actually have.
    m_cMembers = iMemberWrite;
}

//============================================================================
// Load the properties, hooking them up with their implementing methods.
//============================================================================

void MetaImport::LoadProperties
(
    SymbolList *ContainerSymbolList,
    _In_opt_z_ WCHAR *DefaultPropertyName
)
{
    CorEnum Enum(m_pmdImport);

    bool FirstDefaultProperty = true;

    //
    // Iterate through all of the properties.
    //

    for (;;)
    {
        mdProperty PropertyToken;
        unsigned long NumberOfProperties;
        mdTypeDef ContainingTypeToken;
        unsigned long NameLength, NameLength2;
        mdMethodDef GetterToken, SetterToken, CheatToken = mdMethodDefNil;
        WCHAR *Name;
        STRING *Spelling;
        BCSYM_Proc *GetterSymbol = NULL, *SetterSymbol = NULL;
        PCCOR_SIGNATURE Signature;
        ULONG SignatureSize;
        BCSYM *ReturnType;
        BCSYM_Param *FirstParameter;
        BCSYM_NamedRoot *PropertySymbol;
        DECLFLAGS PropertyFlags = 0;
        bool PropertyIsBad = false;
        BCSYM_NamedRoot *pBadTypeSymbol = NULL;

        // Open the enum.
        IfFailThrow(m_pmdImport->EnumProperties(&Enum,
                                                m_td,
                                                &PropertyToken,
                                                1,
                                                &NumberOfProperties));

        // Done?
        if (!NumberOfProperties)
        {
            break;
        }

        // Get the size of the properties name.
        IfFailThrow(m_pmdImport->GetPropertyProps(PropertyToken,
                                                  &ContainingTypeToken,
                                                  NULL,
                                                  0,
                                                  &NameLength,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  &SetterToken,
                                                  &GetterToken,
                                                  NULL,
                                                  0,
                                                  NULL));

        VSASSERT(ContainingTypeToken == m_td, "Why isn't this in the right class?");

        if (GetterToken != mdMethodDefNil)
        {
            BCSYM_NamedRoot *pGetter = GetMemberOfToken(GetterToken);
            GetterSymbol = NULL;

            if (pGetter)
            {
                if (pGetter->IsProc())
                {
                    GetterSymbol = pGetter->PProc();

                    Symbols::SetIsPropertyGet(GetterSymbol);
                    CheatToken = GetterToken;
                    PropertyFlags = GetterSymbol->GetDeclFlags();
                }
                else
                {
                    PropertyIsBad = true;
                }
            }
        }

        if (SetterToken != mdMethodDefNil)
        {
            BCSYM_NamedRoot *pSetter = GetMemberOfToken(SetterToken);
            SetterSymbol = NULL;

            if (pSetter)
            {
                if (pSetter->IsProc())
                {
                    SetterSymbol = pSetter->PProc();

                    Symbols::SetIsPropertySet(SetterSymbol);

                    if (!GetterSymbol)
                    {
                        CheatToken = SetterToken;
                        PropertyFlags = SetterSymbol->GetDeclFlags();
                    }

                    if (SetterSymbol->GetFirstParam() == NULL)
                    {
                        // Property Set must have a parameter, otherwise it is something we cannot consume (VSW#32710).
                        PropertyIsBad = true;
                    }
                }
                else
                {
                    PropertyIsBad = true;
                }
            }
        }

        // If the property is inaccessible and in an FX assembly, we may have skipped
        // both accessors. In which case, skip the property

        if (!GetterSymbol && !SetterSymbol)
            continue;

        // Allocate the memory.
        Name = (WCHAR *)m_nraScratch.Alloc(VBMath::Multiply(NameLength, sizeof(WCHAR)));

        // Get all of the interesting information.
        IfFailThrow(m_pmdImport->GetPropertyProps(PropertyToken,  // [IN] property token
                                                  NULL,           // [OUT] typedef containing the property declarion.
                                                  Name,           // [OUT] Property name
                                                  NameLength,     // [IN] the count of wchar of szProperty
                                                  &NameLength2,   // [OUT] actual count of wchar for property name
                                                  NULL,           // [OUT] property flags -- none we care about
                                                  &Signature,     // [OUT] property type. pointing to meta data internal blob
                                                  &SignatureSize, // [OUT] count of bytes in *ppvSig
                                                  NULL,           // [OUT] flag for value type. selected ELEMENT_TYPE_*
                                                  NULL,           // [OUT] constant value
                                                  NULL,           // [OUT] size of constant value in bytes
                                                  NULL,           // [OUT] setter method of the property
                                                  NULL,           // [OUT] getter method of the property
                                                  NULL,           // [OUT] other method of the property
                                                  0,              // [IN] size of rmdOtherMethod
                                                  NULL));         // [OUT] total number of other method of this property

        VSASSERT(NameLength2 == NameLength, "COM+ lied.");

        // Create the name.
        Spelling = m_pCompiler->AddString(Name);

        // Decode the signature.
        //
        // Unfortunately, we have to cheat here. There are no propertydefs hung off
        // of the property, so there's no way for us to find out useful things like
        // optional parameters and paramarrays for the property. So we pass in the
        // property signature here, but one of the accessor's methoddefs and use
        // the values off of them. Sleazy.
        ReturnType =
            DecodeMethodSignature(
                CheatToken,
                Signature,
                SignatureSize,
                true,
                PropertyIsBad,
                &FirstParameter,
                NULL, NULL,
                &pBadTypeSymbol);

        // We don't allow you to deal with functions that return pointers
        if ((ReturnType && ReturnType->IsPointerType()) || pBadTypeSymbol)
        {
            PropertyIsBad = true;
        }

        PropertyFlags &= ~(DECLF_PropGet | DECLF_PropSet | DECLF_SpecialName);
        PropertyFlags |= DECLF_HasRetval | DECLF_Function;

        if (PropertyIsBad &&
            (!pBadTypeSymbol || !pBadTypeSymbol->IsBad() || pBadTypeSymbol->GetErrid() == 0))
        {
            PropertySymbol = m_Symbols.GetBadNamedRoot(Spelling,
                                                       m_pcontainer->GetNameSpace(),
                                                       DECLF_Public,
                                                       (BindspaceType)(BINDSPACE_Normal),
                                                       ERRID_UnsupportedProperty1,
                                                       NULL,
                                                       ContainerSymbolList);

            // 
        }
        else
        {
        
            // Dev10 #675310
            // If there is a setter, clean up Optional modifier from its last argument so that it was possible
            // to implement/override the property.
            if (SetterSymbol)
            {
				BCSYM_Param * parm;
                for (parm = SetterSymbol->GetFirstParam(); parm && parm->GetNext(); parm = parm->GetNext())
                {
                }

                if (parm)
                {
                    parm->SetIsOptional(false);
                }
            }
        
            if (GetterSymbol && SetterSymbol)
            {
                // VB can have accessors of different accessibility levels. The accessor level can be only
                // more restrictive. Only one accessor can be more restrictive
                // Take the less restrictive one.
                PropertyFlags &= ~DECLF_AccessFlags;
                if (Symbols::AccessOfFlags(SetterSymbol->GetDeclFlags()) < Symbols::AccessOfFlags(GetterSymbol->GetDeclFlags()))
                {
                    PropertyFlags |= GetterSymbol->GetDeclFlags() & DECLF_AccessFlags;
                }
                else
                {
                    PropertyFlags |= SetterSymbol->GetDeclFlags() & DECLF_AccessFlags;
                }
            }

            PropertySymbol = m_Symbols.AllocProperty(false);

            m_Symbols.GetProc(NULL,
                              Spelling,
                              Spelling,
                              (CodeBlockLocation*) NULL,
                              (CodeBlockLocation*) NULL,
                              PropertyFlags,
                              ReturnType,
                              FirstParameter,
                              NULL,
                              NULL,
                              NULL,
                              SYNTH_None,
                              NULL,
                              NULL,
                              PropertySymbol->PProperty());
            Symbols::SetToken(PropertySymbol, PropertyToken);

            if (pBadTypeSymbol)
            {
                AssertIfFalse(pBadTypeSymbol->IsBad());
                TransferBadSymbol(PropertySymbol, pBadTypeSymbol);
            }

            DECLFLAGS PropertyBlockFlags = 0;
            if ( GetterSymbol && !SetterSymbol )
            {
                PropertyBlockFlags = DECLF_ReadOnly;
            }
            else if ( SetterSymbol && !GetterSymbol )
            {
                PropertyBlockFlags = DECLF_WriteOnly;
            }

            m_Symbols.GetProperty(NULL,
                                  Spelling,
                                  PropertyBlockFlags,
                                  GetterSymbol,
                                  SetterSymbol,
                                  PropertySymbol->PProperty(),
                                  ContainerSymbolList);

            Symbols::MakeUnbindable(GetterSymbol);
            Symbols::MakeUnbindable(SetterSymbol);

            Attribute::----AllAttributesOnToken(PropertyToken,
                                                 m_pmdImport,
                                                 m_Symbols.GetNorlsAllocator(),
                                                 m_pCompiler,
                                                 m_pMetaDataFile->GetProject(),
                                                 m_pMetaDataFile,
                                                 PropertySymbol);

            // Mark this as being the default, if necessary.
            if (DefaultPropertyName && CompareNoCase(Spelling,  DefaultPropertyName) == 0)
            {
                Symbols::SetIsDefault(PropertySymbol->PProperty());

                // Only point to the first one -- the rest will be overloads
                if (FirstDefaultProperty)
                {
                    // we should tell the container that it has a default
                    Symbols::SetDefaultProperty(m_pcontainer, PropertySymbol->PProperty());
                    FirstDefaultProperty = false;
                }
            }
        }
    }
}

//============================================================================
// Load the events.
//============================================================================

void MetaImport::LoadEvents
(
    SymbolList *psymlist
)
{
    CorEnum hEnum(m_pmdImport);

    unsigned long ulEvents, cchName, cchName2;
    DWORD dwFlags;
    mdEvent ev;
    mdTypeDef td;
    mdMethodDef mdAdd, mdRemove, mdFire;
    mdToken EventType;
    WCHAR wsz[1];
    WCHAR *wszName, * pstrName;
    BCSYM_EventDecl * pevent;
    BCSYM * pdelegate;
    BCSYM_Class * pdelegateClass;
    BCSYM_Proc *pprocAdd = NULL;
    BCSYM_Proc *pprocRemove = NULL;
    BCSYM_Proc *pprocFire= NULL;
    BCSYM_NamedRoot * pInvokeMethod = NULL;
    bool IsInvalidCustomEvent = false;

    for(;;)
    {
        // Open the enum.
        IfFailThrow(m_pmdImport->EnumEvents(&hEnum,
                                            m_td,
                                            &ev,
                                            1,
                                            &ulEvents));

        // Done?
        if (!ulEvents)
        {
            break;
        }

        // Get the size of the event
        IfFailThrow(m_pmdImport->GetEventProps(ev,
                                               &td,
                                               wsz,
                                               0,
                                               &cchName,
                                               NULL,
                                               NULL,
                                               NULL,
                                               NULL,
                                               NULL,
                                               NULL,           // ignore
                                               0,              // other
                                               NULL));         // methods

        VSASSERT(td == m_td, "Why isn't this in the right class?");

        // Allocate the memory. This memory will be released once the
        // m_nraScratch will be released. Is this a good thing, or a bad thing?
        wszName = (WCHAR *)m_nraScratch.Alloc(VBMath::Multiply(cchName, sizeof(WCHAR)));

        // Get the actual event.
        IfFailThrow(m_pmdImport->GetEventProps(ev,            // [IN] event token
                                               NULL,          // [OUT] typedef containing the event declarion.
                                               wszName,       // [OUT] Event name
                                               cchName,       // [IN] the count of wchar of szEvent
                                               &cchName2,     // [OUT] actual count of wchar for event's name
                                               &dwFlags,      // [OUT] Event flags.
                                               &EventType,    // [OUT] EventType class
                                               &mdAdd,        // [OUT] AddOn method of the event
                                               &mdRemove,     // [OUT] RemoveOn method of the event
                                               &mdFire,       // [OUT] Fire method of the event
                                               NULL,          // [OUT] other method of the event
                                               0,             // [IN] size of rmdOtherMethod
                                               NULL));        // [OUT] total number of other method of this event

        VSASSERT(cchName == cchName2, "COM+ lied!");

        // set up the delegate type
        pdelegate = GetTypeOfToken(EventType, 0);

        // Dev10 #768231: When we get NamedType back we need to store it on Event symbol
        BCSYM * pDelegateForEvent = pdelegate;

        // Dev10 #680929: Be ready to deal with NamedType wrapper here. 
        if ( pdelegate != NULL && pdelegate->IsNamedType() )
        {
            pdelegate = pdelegate->PNamedType()->GetSymbol();

            ThrowIfNull(pdelegate);
        }

        if (!pdelegate || !pdelegate->IsNamedRoot())
        {
            continue;
        }

        // set up the Add, Remove and Fire methods
        BCSYM_NamedRoot *pTemp = NULL;

        pprocAdd = NULL; //DevDivBugs#18485
        pTemp = GetMemberOfToken(mdAdd);
        if (pTemp)
        {
            if (pTemp->IsProc())
            {
                pprocAdd = pTemp->PProc();
            }
            else
            {
                IsInvalidCustomEvent = true;
            }
        }

        pprocRemove = NULL; //DevDivBugs#18485
        pTemp = GetMemberOfToken(mdRemove);
        if (pTemp)
        {
            if (pTemp->IsProc())
            {
                pprocRemove = pTemp->PProc();
            }
            else
            {
                IsInvalidCustomEvent = true;
            }
        }

        pprocFire = NULL; //DevDivBugs#18485
        pTemp = GetMemberOfToken(mdFire);
        if (pTemp)
        {
            if (pTemp->IsProc())
            {
                pprocFire = pTemp->PProc();
            }

            // Don't flag an error in this case.
        }

        // Deleted events may not have procadd/procremove in which case we
        // will treat the event as non-existent
        if ((!pprocAdd || !pprocRemove) && !IsInvalidCustomEvent)
        {
            continue;
        }

        pstrName = m_pCompiler->AddString(wszName);

        if (pdelegate->IsBad())
        {
            ERRID errid = pdelegate->PNamedRoot()->GetErrid();

            if (errid == ERRID_UnreferencedAssembly3)
                errid = ERRID_UnreferencedAssemblyEvent3;
            else if (errid == ERRID_UnreferencedModule3)
                errid = ERRID_UnreferencedModuleEvent3;

            m_Symbols.GetBadNamedRoot(pstrName,
                                      m_pcontainer->GetName(),
                                      DECLF_Public,
                                      BINDSPACE_Normal,
                                      errid,
                                      pdelegate->PNamedRoot()->GetBadExtra(),
                                      psymlist);

            Symbols::MakeUnbindable(pprocAdd);
            Symbols::MakeUnbindable(pprocRemove);
            Symbols::MakeUnbindable(pprocFire);

            continue;
        }

        if (!pdelegate->IsClass() || IsInvalidCustomEvent)
        {
            m_Symbols.GetBadNamedRoot(pstrName,
                                      m_pcontainer->GetName(),
                                      DECLF_Public,
                                      BINDSPACE_Normal,
                                      ERRID_UnsupportedType1,
                                      pdelegate->PNamedRoot()->GetName(),
                                      psymlist);

            Symbols::MakeUnbindable(pprocAdd);
            Symbols::MakeUnbindable(pprocRemove);
            Symbols::MakeUnbindable(pprocFire);

            continue;
        }

        pdelegateClass = pdelegate->PClass();
        pdelegateClass->EnsureChildrenLoaded();

        //
        // Create the event.
        //
        // The event has the same signature as the invoke method
        // of the delegate type.
        //

        // get the signature of the Invoke method

        pInvokeMethod = GetInvokeFromDelegate(pdelegateClass, m_pCompiler);

        if (pInvokeMethod == NULL || pInvokeMethod->IsBad())
        {
            m_Symbols.GetBadNamedRoot(pstrName,
                                      m_pcontainer->GetName(),
                                      DECLF_Public,
                                      BINDSPACE_Normal,
                                      pInvokeMethod ? pInvokeMethod->GetErrid() : ERRID_UnsupportedType1,
                                      pInvokeMethod ? pInvokeMethod->GetBadExtra() : pdelegateClass->GetName(),
                                      psymlist);

            Symbols::MakeUnbindable(pprocAdd);
            Symbols::MakeUnbindable(pprocRemove);
            Symbols::MakeUnbindable(pprocFire);

            continue;
        }

        pevent = m_Symbols.AllocEventDecl(false);

        Attribute::----AllAttributesOnToken(ev,
                                             m_pmdImport,
                                             m_Symbols.GetNorlsAllocator(),
                                             m_pCompiler,
                                             m_pMetaDataFile->GetProject(),
                                             m_pMetaDataFile,
                                             pevent);

        m_Symbols.GetProc(NULL,
                          pstrName,
                          pstrName,
                          (CodeBlockLocation*) NULL,
                          (CodeBlockLocation*) NULL,
                          DECLF_Function |
                            (pprocAdd->GetDeclFlags() & ( DECLF_AccessFlags | DECLF_ShadowsKeywordUsed | DECLF_MustOverride | DECLF_OverridesKeywordUsed)) |
                            (pprocAdd->PProc()->IsShared() ? DECLF_Shared : 0), //Events are functions
                          pInvokeMethod->PProc()->GetType(),
                          pdelegate->IsGenericBinding() ?   // for events of generic delegates, the params will be setup later
                            NULL :
                            pInvokeMethod->PProc()->GetFirstParam(),
                          NULL,
                          NULL,
                          NULL,
                          SYNTH_None,
                          NULL,
                          psymlist,
                          pevent);

        // set up the event
        pevent->SetDelegate(pDelegateForEvent);
        pevent->SetProcAdd(pprocAdd);
        pevent->SetProcRemove(pprocRemove);
        pevent->SetProcFire(pprocFire);

        // Detect WinRT event shape:
        // EventRegistrationToken add_foo(TDelegate)
        // void remove_foo(EventRegistrationToken)
        if (pprocAdd->GetType() &&
            pprocAdd->GetType()->IsNamedRoot() &&
            ( StringPool::IsEqual(STRING_CONST(m_pCompiler, EventRegistrationToken), pprocAdd->GetType()->PNamedRoot()->GetName()) ||
                StringPool::IsEqual(STRING_CONST(m_pCompiler, EventRegistrationToken), pprocAdd->GetType()->PNamedRoot()->GetBadName())) &&
            pprocRemove->GetFirstParam() &&
            pprocRemove->GetFirstParam()->GetType() &&
            pprocRemove->GetFirstParam()->GetType()->IsNamedRoot() &&
            (StringPool::IsEqual(STRING_CONST(m_pCompiler, EventRegistrationToken), pprocRemove->GetFirstParam()->GetType()->PNamedRoot()->GetName()) ||
                StringPool::IsEqual(STRING_CONST(m_pCompiler, EventRegistrationToken), pprocRemove->GetFirstParam()->GetType()->PNamedRoot()->GetBadName())))
        {
            pevent->SetIsWindowsRuntimeEvent(true);

            if (pprocAdd->IsBad() || pprocRemove->IsBad())
            {
                BCSYM_NamedRoot* badPproc = pprocAdd->IsBad() ? pprocAdd : pprocRemove;

                // Here we expect both of them will have the same error, if not, we need to figure out which error need to be attached to the event.
                // The following assertion says both of pprocAdd and pprocRemove are bad implies they have the same error. 
                ThrowIfFalse(!(pprocAdd->IsBad() && pprocRemove->IsBad()) || pprocAdd->GetErrid() == pprocRemove->GetErrid());
                
                pevent->SetIsBad();
                pevent->SetBadName(badPproc->GetBadName());             
                pevent->SetBadExtra(badPproc->GetBadExtra());
                pevent->SetBadNameSpace(badPproc->GetBadNameSpace());
                pevent->SetErrid(badPproc->GetErrid());
            }       
        }

        // In case of generic delegates, clone the event params instead of directly using the delegate's Invoke
        // method's params. This is required because for generic delegates, the param types if using the type
        // params will need to be substituted.
        //
        if (pdelegate->IsGenericBinding())
        {
            Bindable::CopyInvokeParamsToEvent(pevent,
                                              pdelegate,
                                              pInvokeMethod->PProc(),
                                              m_pMetaDataFile->SymbolStorage(),
                                              &m_Symbols);
        }

        Symbols::MakeUnbindable(pprocAdd);
        Symbols::MakeUnbindable(pprocRemove);
        Symbols::MakeUnbindable(pprocFire);

        pprocAdd->SetEventThatCreatedSymbol(pevent);
        pprocRemove->SetEventThatCreatedSymbol(pevent);

        if (pprocFire)
        {
            pprocFire->SetEventThatCreatedSymbol(pevent);
        }
    }
}

UserDefinedOperators
MapToUserDefinedOperator
(
    _In_z_ STRING *MethodName,
    int ParameterCount,
    Compiler *Compiler
)
{
    for (UserDefinedOperators Cursor = (UserDefinedOperators)(OperatorUNDEF + 1);
         Cursor < OperatorMAXVALID;
         Cursor = (UserDefinedOperators)(Cursor + 1))
    {
        if (StringPool::IsEqual(MethodName, Compiler->GetOperatorCLSName(Cursor)) &&
            ((ParameterCount == 1 && IsUnaryOperator(Cursor)) ||
                (ParameterCount == 2 && IsBinaryOperator(Cursor))))
        {
            // Match found, so quit loop early.
            return Cursor;
        }
    }

    for (UserDefinedOperators Cursor = (UserDefinedOperators)(OperatorMAXVALID + 1);
         Cursor < OperatorMAX;
         Cursor = (UserDefinedOperators)(Cursor + 1))
    {
        if (StringPool::IsEqual(MethodName, Compiler->GetOperatorCLSName(Cursor)) &&
            ((ParameterCount == 1 && IsUnaryOperator(Cursor)) ||
                (ParameterCount == 2 && IsBinaryOperator(Cursor))))
        {
            // Match found, so quit loop early.
            return Cursor;
        }
    }

    return OperatorUNDEF;
}

BCSYM_UserDefinedOperator *
MetaImport::CreateOperatorSymbol
(
    UserDefinedOperators Operator,
    BCSYM_MethodDecl *Method
)
{
    BCSYM_UserDefinedOperator* OperatorSymbol = CreateOperatorSymbol(m_pcontainer, Operator, Method, 
        &m_Symbols, m_pCompiler);

    // 
    Attribute::----AllAttributesOnToken(Method->GetToken(),
                                         m_pmdImport,
                                         m_Symbols.GetNorlsAllocator(),
                                         m_pCompiler,
                                         m_pMetaDataFile->GetProject(),
                                         m_pMetaDataFile,
                                         OperatorSymbol);

    return OperatorSymbol;
}

BCSYM_UserDefinedOperator *
MetaImport::CreateOperatorSymbol
(
    BCSYM_Container* Container,
    UserDefinedOperators Operator,
    BCSYM_MethodDecl *Method,
    Symbols* Symbols,
    Compiler* Compiler
)
{
    Container->SetHasUserDefinedOperators(true);

    DECLFLAGS uFlags = Method->GetDeclFlags();

    if (Operator == OperatorWiden)
    {
        uFlags |= DECLF_Widening;
    }
    else if (Operator == OperatorNarrow)
    {
        uFlags |= DECLF_Narrowing;
    }

    BCSYM_UserDefinedOperator *OperatorSymbol = Symbols->AllocUserDefinedOperator(false);

    Symbols->GetProc(NULL,
                      Compiler->OperatorToString(Operator),
                      Method->GetEmittedName(),
                      (CodeBlockLocation*) NULL,
                      (CodeBlockLocation*) NULL,
                      uFlags,
                      Method->GetRawType(),
                      Method->GetFirstParam(),
                      NULL,              // ???
                      NULL,
                      NULL,
                      SYNTH_None,
                      // 



                      NULL,
                      NULL,
                      OperatorSymbol);

    OperatorSymbol->SetOperator(Operator);
    OperatorSymbol->SetOperatorMethod(Method);
    Method->SetAssociatedOperatorDef(OperatorSymbol);
    Symbols::MakeUnbindable(OperatorSymbol);

    return OperatorSymbol;
}

//============================================================================
// Finish loading the children.
//============================================================================
void MetaImport::FinishChildren
(
    _Inout_ SymbolList *psymlist,
    _Inout_ SymbolList *pUnBindableSymList
)
{
    unsigned iMember;
    WCHAR *AccessedThroughProperty;
    DynamicArray<BCSYM_Variable *> WithEventsVariables;
    DynamicArray<BCSYM_MethodDecl *> DeferredOperators;

    // Walk the container and create withevents variables and operator symbols.
    for (iMember = 0; iMember < m_cMembers; iMember++)
    {
        BCSYM_NamedRoot *CurrentMember = m_rgmembers[iMember].m_pnamed;

        if (CurrentMember->GetPWellKnownAttrVals()->GetAccessedThroughPropertyData(&AccessedThroughProperty) &&
            CurrentMember->IsVariable())
        {
            BCSYM_Variable *WithEventsVariable;
            Type *WithEventsType = CurrentMember->PVariable()->GetType();
            STRING *WithEventsVariableName = m_pCompiler->AddString(CurrentMember->GetName() + 1);

            // Found a withevents variable. Create the withevents symbol, we'll hook it up in a moment.
            WithEventsVariable = m_Symbols.AllocVariable( false, false );
            m_Symbols.GetVariable( NULL,
                                   WithEventsVariableName,
                                   WithEventsVariableName,
                                   DECLF_WithEvents,
                                   VAR_WithEvents,
                                   WithEventsType,
                                   NULL,
                                   psymlist,   // shouldn't this be the unbindable hash?
                                   WithEventsVariable);

            WithEventsVariables.AddElement(WithEventsVariable);

            // A bad withevents property type means a bad withevents variable.
            if (WithEventsType && WithEventsType->IsNamedRoot() && WithEventsType->PNamedRoot()->IsBadNamedRoot())
            {
                TransferBadSymbol(WithEventsVariable, WithEventsType->PNamedRoot());
            }
        }
        // Determine if this method is an Operator.
        else if (CurrentMember->IsSpecialName() &&
                 CurrentMember->IsMethodDecl() &&
                 CompareNoCaseN(
                    STRING_CONST(m_pCompiler, OperatorPrefix),
                    CurrentMember->GetName(),
                    StringPool::StringLength(STRING_CONST(m_pCompiler, OperatorPrefix))) == 0)
        {
            BCSYM_MethodDecl *CurrentMethod = CurrentMember->PMethodDecl();

            // Perform some additional checks to make sure this is a valid operator.
            // It must be shared and it must be a function (have a return type).
            if (!CurrentMethod->IsShared() || !CurrentMethod->GetRawType())
            {
                continue;
            }

            UserDefinedOperators Operator =
                MapToUserDefinedOperator(CurrentMethod->GetName(), CurrentMethod->GetParameterCount(), m_pCompiler);

            if (Operator != OperatorUNDEF)
            {
                if (OperatorMAXVALID < Operator && Operator < OperatorMAX)
                {
                    DeferredOperators.AddElement(CurrentMethod);
                }
                else
                {
                    pUnBindableSymList->AddToFront(CreateOperatorSymbol(Operator, CurrentMethod));
                }
            }
        }
    }

    // Walk the container and add any nested types to the list.
    BCSYM_Container *pCurrentContainer, *pNextContainer;

    for (pCurrentContainer = *m_pcontainer->GetNestedTypeList();
         pCurrentContainer;
         pCurrentContainer = pNextContainer)
    {
        pNextContainer = pCurrentContainer->GetNextInSymbolList()->PContainer();

        psymlist->AddToFront(pCurrentContainer);
    }

    SymbolList BadSymbolList;

    // Walk the SavedMetaMembers array and add all the non-used symbols to the list.
    for (iMember = 0; iMember < m_cMembers; iMember++)
    {
        BCSYM_NamedRoot *CurrentMember = m_rgmembers[iMember].m_pnamed;

        if (CurrentMember->GetBindingSpace() == BINDSPACE_IgnoreSymbol)
        {
            pUnBindableSymList->AddToFront(CurrentMember);
        }
        else if (CurrentMember->IsGenericBadNamedRoot())
        {
            BadSymbolList.AddToFront(CurrentMember);
        }
        else
        {
            psymlist->AddToFront(CurrentMember);
        }
    }

    BCSYM_Hash *ChildrenHash =
        m_Symbols.GetHashTable(
            m_pcontainer->GetNameSpace(),
            m_pcontainer,
            true,
            psymlist->GetCount() + BadSymbolList.GetCount(),
            &BadSymbolList);

    Symbols::AddSymbolListToHash(ChildrenHash, psymlist, true);

    BCSYM_Hash *UnBindableChildrenHash =
        m_Symbols.GetHashTable(m_pcontainer->GetNameSpace(), m_pcontainer, true, 0, pUnBindableSymList);

    m_pcontainer->SetHashes(ChildrenHash, UnBindableChildrenHash);

    // Walk the container and hookup properties and complete setting up the WithEvent Variables
    for (iMember = 0; iMember < WithEventsVariables.Count(); iMember++)
    {
        BCSYM_NamedRoot *CurrentMember = WithEventsVariables.Element(iMember);

        // 


        BCSYM_NamedRoot *WithEventsProperty = ChildrenHash->SimpleBind(CurrentMember->GetName());

        while (WithEventsProperty && !WithEventsProperty->IsProperty())
        {
            WithEventsProperty = WithEventsProperty->GetNextBound();
        }

        if (WithEventsProperty)
        {
            // The access/sharedness of the withevents variable should be the same as that of the Property
            // accessor.
            CurrentMember->SetAccess(WithEventsProperty->GetAccess());
            CurrentMember->PVariable()->SetIsShared(WithEventsProperty->PProperty()->IsShared());

            WithEventsProperty->SetMemberThatCreatedSymbol(CurrentMember->PVariable());
        }
        else
        {
            // If there is no associated property, delete (ignore) the withevents symbol

            VSASSERT( CurrentMember->GetImmediateParent()->IsHash(),
                            "How can the immediate parent of a member be anything else ?");

            Symbols::RemoveSymbolFromHash(CurrentMember->GetImmediateParent()->PHash(), CurrentMember);
        }
    }

    for (iMember = 0; iMember < DeferredOperators.Count(); iMember++)
    {
        BCSYM_MethodDecl *Deferred = DeferredOperators.Element(iMember);
        UserDefinedOperators Mapped;

        switch (MapToUserDefinedOperator(Deferred->GetName(), Deferred->GetParameterCount(), m_pCompiler))
        {
            case OperatorNot2:        Mapped = OperatorNot; break;
            case OperatorOr2:         Mapped = OperatorOr; break;
            case OperatorAnd2:        Mapped = OperatorAnd; break;
            case OperatorShiftLeft2:  Mapped = OperatorShiftLeft; break;
            case OperatorShiftRight2: Mapped = OperatorShiftRight; break;
            default:
                VSFAIL("unknown operator mapping");
                continue;
        }

        BCSYM_NamedRoot *ExistingOperator = UnBindableChildrenHash->SimpleBind(m_pCompiler->OperatorToString(Mapped));

        while (ExistingOperator)
        {
            if (ExistingOperator->IsUserDefinedOperator())
            {
                unsigned CompareFlags =
                    BCSYM::CompareProcs(
                        ExistingOperator->PUserDefinedOperator()->GetOperatorMethod(),
                        (GenericBinding *)NULL,
                        Deferred,
                        (GenericBinding *)NULL,
                        NULL);

                if (!(CompareFlags & EQ_Shape))
                {
                    goto continuefor;
                }
            }

            ExistingOperator = ExistingOperator->GetNextBound();
        }

        Symbols::AddSymbolToHash(
            UnBindableChildrenHash,
            CreateOperatorSymbol(Mapped, Deferred),
            true,
            false,
            false);

continuefor:;
    }
}

//============================================================================
// Load this Module's member names into the STRING table.
//============================================================================

void MetaImport::ImportMemberNamesInModule
(
    BCSYM_Class *pModule
)
{
    VSASSERT(pModule->IsStdModule(), "Non-Module unexpected!!!");

    CompilerFile *pFile = pModule->GetCompilerFile();
    VSASSERT(pFile->IsMetaDataFile(), "Non-metadata module unexpected!!!");
    MetaDataFile *pMetaDataFile = pFile->PMetaDataFile();

    if (pMetaDataFile->GetCompState() >= CS_Bound)
    {
        // If the metadata file has already reached CS_Bound, then we might as well load the module's members.
        pModule->EnsureChildrenLoaded();
    }
    else
    {
        // If the metadata file has not already reached CS_Bound, then we load the module's member names only.

        Compiler *Compiler = pModule->GetCompiler();
        CompilerIdeLock spLock(Compiler->GetMetaImportCritSec());
        MetaImport import(Compiler, pMetaDataFile, NULL);

        import.DoImportMemberNamesInModule(pModule);
    }
}

//============================================================================
// Load this Module's member names into the STRING table.
//============================================================================

void MetaImport::DoImportMemberNamesInModule
(
    BCSYM_Class *pModule
)
{
    if (pModule->PContainer()->AreChildrenLoaded())
    {
        VSFAIL("Why are member names being imported again after the members have been imported ?");

        // Member names already imported.
        return;
    }

    //
    // Load the nested type names.
    //
    for (BCSYM_Container *pNestedType = *pModule->PContainer()->GetNestedTypeList();
         pNestedType;
         pNestedType = pNestedType->GetNextInSymbolList()->PContainer())
    {
        StringPool::SetDeclaredInModule(pNestedType->GetName());
    }

    //
    // Load the member names.
    //

    CorEnum hEnum(m_pmdImport);

    //
    // Load the Method and Field names.
    //
    {
        ULONG cMembers, cMemberDefs, iMember;
        MetaMember mm;
        mdToken tk;
        m_td = pModule->GetTypeDef();

        // Open the enum.
        IfFailThrow(m_pmdImport->EnumMembers(&hEnum,
                                            pModule->GetTypeDef(),
                                            NULL,
                                            0,
                                            NULL));

        // Get the count.
        IfFailThrow(m_pmdImport->CountEnum(hEnum, &cMembers));

        for (iMember = 0; iMember < cMembers; iMember++)
        {
            // Get the token.
            IfFailThrow(m_pmdImport->EnumMembers(&hEnum,
                                                pModule->GetTypeDef(),
                                                &tk,
                                                1,
                                                &cMemberDefs));

            // Are we done?
            if (!cMemberDefs)
            {
                break;
            }

            // Load the name
            GetMemberProps(tk, &mm);

            if (mm.m_pstrName)
            {
                StringPool::SetDeclaredInModule(mm.m_pstrName);
            }
        }

        hEnum.Close();
    }

    //
    // Load the Property Names.
    //
    {
        mdTypeDef tkContainingTypeToken;
        ULONG cProperties, cPropertyDefs, iProperty;
        ULONG cchPropertyNameLength, cchPropertyNameLength2;
        mdProperty tkProperty;
        WCHAR *wszPropertyName;

        IfFailThrow(m_pmdImport->EnumProperties(&hEnum,
                                                pModule->GetTypeDef(),
                                                NULL,
                                                0,
                                                NULL));

        // Get the count.
        IfFailThrow(m_pmdImport->CountEnum(hEnum, &cProperties));


        for (iProperty = 0; iProperty < cProperties; iProperty++)
        {
            // Get the token.
            IfFailThrow(m_pmdImport->EnumProperties(&hEnum,
                                                    pModule->GetTypeDef(),
                                                    &tkProperty,
                                                    1,
                                                    &cPropertyDefs));

            // Are we done?
            if (!cPropertyDefs)
            {
                break;
            }

            // Get the size of the properties name.
            IfFailThrow(m_pmdImport->GetPropertyProps(tkProperty,
                                                    &tkContainingTypeToken,
                                                    NULL,
                                                    0,
                                                    &cchPropertyNameLength,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    0,
                                                    NULL));

            VSASSERT(tkContainingTypeToken == pModule->GetTypeDef(), "Why is this property in the wrong module?");

            if (cchPropertyNameLength == 0)
            {
                continue;
            }

            // Allocate the memory.
            wszPropertyName = (WCHAR *)m_nraScratch.Alloc(VBMath::Multiply(
                cchPropertyNameLength, 
                sizeof(WCHAR)));

            // Get all of the interesting information.
            IfFailThrow(m_pmdImport->GetPropertyProps(tkProperty,               // [IN] property token
                                                    NULL,                     // [OUT] typedef containing the property declarion.
                                                    wszPropertyName,          // [OUT] Property name
                                                    cchPropertyNameLength,    // [IN] the count of wchar of szProperty
                                                    &cchPropertyNameLength2,  // [OUT] actual count of wchar for property name
                                                    NULL,                     // [OUT] property flags -- none we care about
                                                    NULL,                     // [OUT] property type. pointing to meta data internal blob
                                                    NULL,                     // [OUT] count of bytes in *ppvSig
                                                    NULL,                     // [OUT] flag for value type. selected ELEMENT_TYPE_*
                                                    NULL,                     // [OUT] constant value
                                                    NULL,                     // [OUT] size of constant value in bytes
                                                    NULL,                     // [OUT] setter method of the property
                                                    NULL,                     // [OUT] getter method of the property
                                                    NULL,                     // [OUT] other method of the property
                                                    0,                        // [IN] size of rmdOtherMethod
                                                    NULL));                   // [OUT] total number of other method of this property

            VSASSERT(cchPropertyNameLength2 == cchPropertyNameLength, "COM+ lied.");

            StringPool::SetDeclaredInModule(m_pCompiler->AddString(wszPropertyName));
        }

        hEnum.Close();
    }

    //
    // Load the Event Names.
    //
    {
        mdTypeDef tkContainingTypeToken;
        ULONG cEvents, cEventDefs, iEvent;
        ULONG cchEventNameLength = 0, cchEventNameLength2 = 0;
        mdEvent tkEvent;
        WCHAR *wszEventName;

        IfFailThrow(m_pmdImport->EnumEvents(&hEnum,
                                            pModule->GetTypeDef(),
                                            NULL,
                                            0,
                                            NULL));

        // Get the count.
        IfFailThrow(m_pmdImport->CountEnum(hEnum, &cEvents));

        for (iEvent = 0; iEvent < cEvents; iEvent++)
        {
            // Get the token.
            IfFailThrow(m_pmdImport->EnumEvents(&hEnum,
                                                pModule->GetTypeDef(),
                                                &tkEvent,
                                                1,
                                                &cEventDefs));

            // Are we done?
            if (!cEventDefs)
            {
                break;
            }

            // Get the size of the event name.
            IfFailThrow(m_pmdImport->GetEventProps(tkEvent,
                                                   &tkContainingTypeToken,
                                                   NULL,
                                                   0,
                                                   &cchEventNameLength,
                                                   NULL,
                                                   NULL,
                                                   NULL,
                                                   NULL,
                                                   NULL,
                                                   NULL,
                                                   0,
                                                   NULL));

            VSASSERT(tkContainingTypeToken == pModule->GetTypeDef(), "Why is this event in the wrong module?");

            if (cchEventNameLength == 0)
            {
                continue;
            }

            // Allocate the memory. This memory will be released once the
            // m_nraScratch will be released. Is this a good thing, or a bad thing?
            wszEventName = (WCHAR *)m_nraScratch.Alloc(VBMath::Multiply(
                cchEventNameLength, 
                sizeof(WCHAR)));

            // Get the actual event.
            IfFailThrow(m_pmdImport->GetEventProps(tkEvent,             // [IN] event token
                                                   NULL,                // [OUT] typedef containing the event declarion.
                                                   wszEventName,        // [OUT] Event name
                                                   cchEventNameLength,  // [IN] the count of wchar of szEvent
                                                   &cchEventNameLength2,// [OUT] actual count of wchar for event's name
                                                   NULL,                // [OUT] Event flags.
                                                   NULL,                // [OUT] EventType class
                                                   NULL,                // [OUT] AddOn method of the event
                                                   NULL,                // [OUT] RemoveOn method of the event
                                                   NULL,                // [OUT] Fire method of the event
                                                   NULL,                // [OUT] other method of the event
                                                   0,                   // [IN] size of rmdOtherMethod
                                                   NULL));              // [OUT] total number of other method of this event

            VSASSERT(cchEventNameLength == cchEventNameLength2, "COM+ lied!");

            StringPool::SetDeclaredInModule(m_pCompiler->AddString(wszEventName));
        }

        hEnum.Close();
    }
}

DECLFLAGS MetaImport::MapMethodFlagsToDeclFlags
(
    unsigned long MethodFlags,
    bool fCodeModule    // Are we importing a type from a code module?
)
{
    DECLFLAGS uFlags;

    // Generate the base sets of flags.
    uFlags = DECLF_Function;

    // Note that types imported from a code module fulfill "assembly" accessibility.
    //
    // ***
    // *** NOTE!!  If this code changes, update the switch statement in MapTypeFlagsToDeclFlags
    // *** NOTE!!  and MetaImport::LoadFieldDef!
    // ***
    switch (MethodFlags & mdMemberAccessMask)
    {
        case mdAssem:
#if !IDE
            // VBC is importing from a code module, but the rest of the compiler would
            // be confused by DECLF_Friend that spans multiple CompilerProjects, so
            // treat this as Public.
            if (fCodeModule)
            {
                uFlags |= DECLF_Public;
            }
            else
#endif
            {
                uFlags |= DECLF_Friend;
            }
            break;

        case mdFamORAssem:
#if !IDE
            // VBC is importing a code module, and FamORAssem == FamORTrue == True,
            // so mark as Public.
            if (fCodeModule)
            {
                uFlags |= DECLF_Public;
            }
            else
#endif
            {
                // Type is imported from another assembly, and FamORAssem == FamORFalse == Fam,
                // so mark as Protected.
                uFlags |= DECLF_ProtectedFriend;
            }
            break;

        case mdFamANDAssem:
#if !IDE
            // VBC is importing a code module, and FamANDAssem == FamANDTrue == Fam,
            // so mark as Protected.
            if (fCodeModule)
            {
                uFlags |= DECLF_Protected;
            }
            else
#endif
            {
                // VB can't represent this accurately, so treat it as friend
                uFlags |= DECLF_Friend;
            }
            break;

        case mdPublic:
            uFlags |= DECLF_Public;
            break;

        case mdFamily:
            uFlags |= DECLF_Protected;
            break;

        case mdPrivate:
        case mdPrivateScope:
            uFlags |= DECLF_Private;
            break;
    }

    if (MethodFlags & mdStatic)
    {
        uFlags |= DECLF_Shared;
    }

    if (MethodFlags & mdVirtual)
    {
        if (MethodFlags & mdAbstract)
        {
            VSASSERT( (MethodFlags & mdFinal) == 0, "Metadata method both Abstract and Final");
            // mdNewSlot is ignored when the method is abstract
            // overides, overidable make no sense in this case
            uFlags |= DECLF_MustOverride;

            if (!(MethodFlags & mdNewSlot))
            {
                uFlags |= DECLF_OverridesKeywordUsed;
            }
        }
        else
        {
            // If newslot isn't set, then we're not shadowing.
            if (MethodFlags & mdNewSlot)
            {
                // Functions can be Newslot Virtual Final
                //
                // But we don't want to show these to our users
                // a Overridable NotOverridable because this does
                // not make sense in VB.
                //
                // Also we want to make sure we mark these as
                // DECLF_Implementing so that they are treated
                // as virtual when generating calls to these and
                // callvirt is generated. Callvirt needs to be
                // generated although these are marked as Final
                // for versioning reasons.
                //
                // Also the other effect of using DECLF_Implementing
                // to mark these is consistent because VB too produces
                // such functions when VB functions not marked as
                // Overridable nor overrides are used to implement
                // interface members.
                //
                if (MethodFlags & mdFinal)
                {
                    uFlags |= DECLF_Implementing;
                }
                else
                {
                    uFlags |= DECLF_Overridable;
                }
            }
            else
            {
                uFlags |= DECLF_OverridesKeywordUsed;

                // Functions can be Final Virtual
                if (MethodFlags & mdFinal)
                {
                    uFlags |= DECLF_NotOverridable;
                }
            }
        }
    }

    if ((MethodFlags & mdHideBySig) ||
        // If overrides is present, then Overloads is implicit
        //
        (uFlags & DECLF_OverridesKeywordUsed))
    {
        uFlags |= DECLF_OverloadsKeywordUsed;
    }
    else
    {
        uFlags |= DECLF_ShadowsKeywordUsed;
    }

    return uFlags;
}

//============================================================================
// Create a method symbol from MetaMember information.
//============================================================================

BCSYM_NamedRoot *MetaImport::LoadMethodDef
(
    _In_ MetaMember *pmetamember,
    bool hasLocation
)
{
    DWORD dwAttr = pmetamember->m_dwFlags;
    BCSYM_NamedRoot *MethodSymbol;

    // Everything we need to get for a method.
    STRING *pstrName = pmetamember->m_pstrName;
    STRING *pstrEmittedName = pstrName;

    // Dev10 #849605 Add mdNewSlot for interface methods to prevent crash in Bindable::FindMemberOverriddenByOverridingMember().
    // We don't support "Overrides" modifier for methods declared in interfaces, but the lack of the mdNewSlot flag will put it on the method.
    DWORD dwFlagsToMap = dwAttr | (m_pcontainer->IsInterface() ? mdNewSlot : 0);
    DECLFLAGS uFlags = MapMethodFlagsToDeclFlags(dwFlagsToMap, m_pMetaDataFile->GetProject()->IsCodeModule());

    BCSYM *ReturnType;
    BCSYM_Param *pparamList = NULL;
    bool MethodIsBad = false;
    BCSYM_NamedRoot *pBadTypeSymbol = NULL;
    PCCOR_SIGNATURE returnMarshalAsType = NULL;
    ULONG cbReturnMarshalAsType = 0;

    MethodSymbol = m_Symbols.AllocMethodDecl(hasLocation);

    // If the method is generic, its type parameters must be available before decoding its signature.
    //
    SetupGenericParameters(MethodSymbol, pmetamember->m_tk);

    // Save the previous method generic params context
    //
    BCSYM_GenericParam *PrevMethodGenericParams = m_CurrentMethodGenericParams;
    VSASSERT(PrevMethodGenericParams == NULL, "Recursive method load unexpected!!!");

    m_CurrentMethodGenericParams = MethodSymbol->GetFirstGenericParam();
    SetupGenericParameterTypeConstraints(MethodSymbol->GetFirstGenericParam(), &pBadTypeSymbol);

    // Parse the function signature and set up the parameters and
    // return type.
    //
    ReturnType = DecodeMethodSignature(pmetamember->m_tk,
                                       pmetamember->m_pSignature,
                                       pmetamember->m_cbSignature,
                                       false,
                                       MethodIsBad,
                                       &pparamList,
                                       &returnMarshalAsType,
                                       &cbReturnMarshalAsType,
                                       &pBadTypeSymbol);

    // We don't allow you to deal with functions that return pointers
    if ((ReturnType && ReturnType->IsPointerType()) || pBadTypeSymbol)
    {
        MethodIsBad = true;
    }

    // #282112 - some methods on an interface (like _VtblGap8()) are hidden and doesn't require the user to implement it when they implement the interface.
    // They way we know we have a method that doesn't need to be implemented is that it won't be marked as virtual.  The user can't ever create a method on
    // an interface that isn't virtual but TLBIMP can and does to mark something on an interface that is 'hidden' and isn't meant to be implemented.
    if (m_pcontainer->IsInterface())
    {
        if (!(dwAttr & mdVirtual))
        {
            uFlags |= DECLF_ImplementionNotRequired;
        }

        // No need to explicitly mark interface members as overridable.
        uFlags &= ~DECLF_Overridable;
    }

    // All constructors have a name of either ".ctor" or ".cctor"
    if (dwAttr & mdSpecialName)
    {
        if (StringPool::IsEqual(STRING_CONST(m_pCompiler, Constructor), pstrName) ||
            StringPool::IsEqual(STRING_CONST(m_pCompiler, SharedConstructor), pstrName))
        {
            // Constructor
            uFlags |= DECLF_Constructor | DECLF_SpecialName;
        }
        // Mark that it's got a special name unless it's a let, which
        // really isn't special at all.
        else if (CompareNoCaseN(STRING_CONST(m_pCompiler, LetPrefix), pstrName, StringPool::StringLength(STRING_CONST(m_pCompiler, LetPrefix))) != 0)
        {
            uFlags |= DECLF_SpecialName;
        }
    }

    // We should have everything we need.  Create the method symbol.
    if (MethodIsBad &&
        (!pBadTypeSymbol || !pBadTypeSymbol->IsBad() || pBadTypeSymbol->GetErrid() == 0))
    {
        MethodSymbol = m_Symbols.GetBadNamedRoot(pstrName,
                                                 m_pcontainer->GetNameSpace(),
                                                 DECLF_Public,
                                                 (BindspaceType)(BINDSPACE_Normal),
                                                 ERRID_UnsupportedMethod1,
                                                 NULL,
                                                 NULL,
                                                 hasLocation); // sometimes (e.g. NoPIA) we want location info for BadNamedRoots
    }
    else
    {
        m_Symbols.GetProc(NULL,
                          pstrName,
                          pstrEmittedName,
                          (CodeBlockLocation*) NULL,
                          (CodeBlockLocation*) NULL,
                          uFlags,
                          ReturnType,
                          pparamList,
                          NULL,
                          NULL,
                          NULL,
                          SYNTH_None,
                          NULL,
                          NULL,
                          MethodSymbol->PProc());

        if (pBadTypeSymbol)
        {
            AssertIfFalse(pBadTypeSymbol->IsBad());
            TransferBadSymbol(MethodSymbol, pBadTypeSymbol);
        }

        Attribute::----AllAttributesOnToken(pmetamember->m_tk,
                                             m_pmdImport,
                                             m_Symbols.GetNorlsAllocator(),
                                             m_pCompiler,
                                             m_pMetaDataFile->GetProject(),
                                             m_pMetaDataFile,
                                             MethodSymbol);

        if (cbReturnMarshalAsType > 0)
        {
            if (MethodSymbol->GetPWellKnownAttrVals() == NULL)
            {
                MethodSymbol->AllocAttrVals(m_Symbols.GetNorlsAllocator(), CS_MAX, NULL, NULL, true);
            }
            COR_SIGNATURE *marshalCopy = (COR_SIGNATURE*)m_Symbols.GetNorlsAllocator()->Alloc(cbReturnMarshalAsType);
            memcpy(marshalCopy, returnMarshalAsType, cbReturnMarshalAsType);
            MethodSymbol->GetPWellKnownAttrVals()->SetMarshalAsData(marshalCopy, cbReturnMarshalAsType);
        }
    }

    if (MethodSymbol && MethodSymbol->IsProc())
    {
        MethodSymbol->PProc()->SetCheckAccessOnOverride(0 != (dwAttr & mdCheckAccessOnOverride));
        MethodSymbol->PProc()->SetPreserveSig(0 != (pmetamember->m_dwImplFlags & miPreserveSig));
    }

    // Save the token for future use.
    Symbols::SetToken(MethodSymbol, pmetamember->m_tk);

    // Restore the previous method generic params context
    //
    m_CurrentMethodGenericParams = PrevMethodGenericParams;

    // Return it.
    return MethodSymbol;
}

DECLFLAGS MetaImport::MapFieldFlagsToDeclFlags
(
    unsigned long FieldFlags,
    bool fCodeModule    // Are we importing a type from a code module?
)
{
    DECLFLAGS uFlags;

    // Generate the base sets of flags.
    uFlags = 0;

    // Note that types imported from a code module fulfill "assembly" accessibility.
    //
    // ***
    // *** NOTE!!  If this code changes, update the switch statement in MapTypeFlagsToDeclFlags
    // *** NOTE!!  and MetaImport::LoadMethodDef!
    // ***
    switch (FieldFlags & fdFieldAccessMask)
    {
        case fdAssembly:
#if !IDE
            // VBC is importing from a code module, but the rest of the compiler would
            // be confused by DECLF_Friend that spans multiple CompilerProjects, so
            // treat this as Public.
            if (fCodeModule)
            {
                uFlags |= DECLF_Public;
            }
            else
#endif
            {
                uFlags |= DECLF_Friend;
            }
            break;

        case fdFamORAssem:
#if !IDE
            // VBC is importing a code module, and FamORAssem == FamORTrue == True,
            // so mark as Public.
            if (fCodeModule)
            {
                uFlags |= DECLF_Public;
            }
            else
#endif
            {
                // Type is imported from another assembly, and FamORAssem == FamORFalse == Fam,
                // so mark as Protected.
                uFlags |= DECLF_ProtectedFriend;
            }
            break;

        case fdFamANDAssem:
#if !IDE
            // VBC is importing a code module, and FamANDAssem == FamANDTrue == Fam,
            // so mark as Protected.
            if (fCodeModule)
            {
                uFlags |= DECLF_Protected;
            }
            else
#endif
            {
                // VB can't represent this accurately, so treat it as Friend
                uFlags |= DECLF_Friend;
            }
            break;

        case fdPublic:
            uFlags |= DECLF_Public;
            break;

        case fdFamily:
            uFlags |= DECLF_Protected;
            break;

        case fdPrivate:
        case fdPrivateScope:
            uFlags |= DECLF_Private;
            break;
    }

    if (FieldFlags & fdStatic)
    {
        uFlags |= DECLF_Shared;
    }

    if (FieldFlags & fdLiteral)
    {
        uFlags |= DECLF_Const;
    }

    if (FieldFlags & fdInitOnly)
    {
        uFlags |= DECLF_ReadOnly;
    }

    if (FieldFlags & fdSpecialName)
    {
        // Mark that it has a special name
        uFlags |= DECLF_SpecialName;
    }

    uFlags |= DECLF_ShadowsKeywordUsed;

    return uFlags;
}

//============================================================================
// Create a variable symbol from MetaMember information
//============================================================================

BCSYM_NamedRoot *MetaImport::LoadFieldDef
(
    _In_ MetaMember *pmetamember,
    _Out_ Vtypes *pvtUnderlyingType,  // Underlying type for enum
    bool allocLocation
)
{
    DWORD dwAttr = pmetamember->m_dwFlags;
    BCSYM_NamedRoot *pvar;

    // Everything we need to get for a variable.
    STRING *pstrName = pmetamember->m_pstrName;
    DECLFLAGS uFlags = MapFieldFlagsToDeclFlags(dwAttr, m_pMetaDataFile->GetProject()->IsCodeModule());
    VARIABLEKIND varkind;
    BCSYM *ptyp;
    BCSYM_Expression *pexpr;
    bool FieldIsBad = false;

    // Parse the field's signature.
    ptyp = DecodeFieldSignature(pmetamember->m_pSignature,
                                pmetamember->m_cbSignature,
                                FieldIsBad);

    // We don't allow you to deal with fields that are pointers
    if (ptyp && ptyp->IsPointerType())
    {
        FieldIsBad = true;
    }

    // Get the value.
    pexpr = DecodeValue(pmetamember->m_dwTypeFlags,
                        pmetamember->m_pvValue,
                        pmetamember->m_cbValue,
                        ptyp);

    // Handle special fields
    if (uFlags & DECLF_SpecialName && m_pcontainer->IsEnum())
    {
        // Value field
        VSASSERT(wcscmp(pstrName, L"value__") == 0, "Bad enum member.");

        *pvtUnderlyingType = ptyp->GetVtype();
    }

    // Figure out the varkind.
    varkind = pexpr ? VAR_Const : VAR_Member;

    // We should have everything we need.  Create the field symbol.
    //
    if (!ptyp ||
        (FieldIsBad &&
         (!ptyp->IsBad() || !ptyp->IsNamedRoot() || ptyp->PNamedRoot()->GetErrid() == 0)))
    {
        pvar = m_Symbols.GetBadNamedRoot(pstrName,
                                         m_pcontainer->GetNameSpace(),
                                         DECLF_Public,
                                         (BindspaceType)(BINDSPACE_Normal),
                                         ERRID_UnsupportedField1,
                                         NULL,
                                         NULL,
                                         allocLocation ); // sometimes (e.g. NoPIA) we want location info for BadNamedRoots
    }
    else
    {
        pvar = m_Symbols.AllocVariable(allocLocation, pexpr != NULL);

        m_Symbols.GetVariable(NULL,
                              pstrName,
                              pstrName,
                              uFlags,
                              varkind,
                              ptyp,
                              pexpr,
                              NULL,
                              pvar->PVariable());

        if (ptyp->IsBad())
        {
            TransferBadSymbol(pvar, ptyp->PNamedRoot());
        }

        Attribute::----AllAttributesOnToken(pmetamember->m_tk,
                                             m_pmdImport,
                                             m_Symbols.GetNorlsAllocator(),
                                             m_pCompiler,
                                             m_pMetaDataFile->GetProject(),
                                             m_pMetaDataFile,
                                             pvar);

        // Extract decimal constant, if exists
        DECIMAL DecimalConstant;
        if (pvar->GetPWellKnownAttrVals()->GetDecimalConstantData(&DecimalConstant))
        {
            ConstantValue Value;
            BCSYM_Expression *Expression;

            Value.TypeCode = t_decimal;
            Value.Decimal = DecimalConstant;

            Expression = m_Symbols.GetFixedExpression(&Value);

            // Morph the variable
            Symbols::MorphVariableToConst(pvar->PVariable(), Expression);
        }

        // Extract DateTime constant, if exists
        __int64 DateTimeConstant;
        if (pvar->GetPWellKnownAttrVals()->GetDateTimeConstantData(&DateTimeConstant))
        {
            ConstantValue Value;
            BCSYM_Expression *Expression;

            Value.TypeCode = t_date;
            Value.Integral = DateTimeConstant;

            Expression = m_Symbols.GetFixedExpression(&Value);

            // Morph the variable
            Symbols::MorphVariableToConst(pvar->PVariable(), Expression);
        }

        // Save the token for future use.
        Symbols::SetToken(pvar, pmetamember->m_tk);
    }

    // Save marshalling data if present.
    if (dwAttr & fdHasFieldMarshal)
    {
        PCCOR_SIGNATURE marshalNativeType = NULL;
        ULONG cbMarshalNativeType = 0;
        IfFailThrow(m_pmdImport->GetFieldMarshal(pmetamember->m_tk, &marshalNativeType, &cbMarshalNativeType));
        if (pvar->GetPWellKnownAttrVals() == NULL)
        {
            pvar->AllocAttrVals(m_Symbols.GetNorlsAllocator(), CS_MAX, NULL, NULL, true);
        }
        COR_SIGNATURE *marshalCopy = (COR_SIGNATURE*)m_Symbols.GetNorlsAllocator()->Alloc(cbMarshalNativeType);
        memcpy(marshalCopy, marshalNativeType, cbMarshalNativeType);
        pvar->GetPWellKnownAttrVals()->SetMarshalAsData(marshalCopy, cbMarshalNativeType);
    }

    // Return it.
    return pvar;
}

//============================================================================
// Get the information about a member.
//============================================================================

bool MetaImport::GetMemberProps
(
    mdToken tk,
    MetaMember *pmetamember
)
{
    HRESULT hr = NOERROR;
    unsigned long cchName, cchName2;
    WCHAR wsz[256];
    mdTypeDef td;

    IfFailThrow(m_pmdImport->GetMemberProps(tk,
                                            &td,
                                            wsz,
                                            sizeof(wsz) / sizeof(WCHAR),
                                            &cchName,
                                            &pmetamember->m_dwFlags,
                                            &pmetamember->m_pSignature,
                                            &pmetamember->m_cbSignature,
                                            NULL,
                                            &pmetamember->m_dwImplFlags,
                                            &pmetamember->m_dwTypeFlags,
                                            &pmetamember->m_pvValue,
                                            &pmetamember->m_cbValue));
    
    VSASSERT(td == m_td, "How did this member move?");

    // Save the token.
    pmetamember->m_tk = tk;

    if (cchName > (sizeof(wsz) / sizeof(WCHAR)))
    {
        // Allocate room for the strings.
        VBHeapPtr<WCHAR> wszName(cchName);

        // Get the fields.
        IfFailThrow(m_pmdImport->GetMemberProps(tk,
                                                NULL,
                                                wszName,
                                                cchName,
                                                &cchName2,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL));

        VSASSERT(cchName == cchName2, "COM+ lied.");

        pmetamember->m_pstrName = m_pCompiler->AddString(wszName);
    }
    else
    {
        pmetamember->m_pstrName = m_pCompiler->AddString(wsz);
    }

    CompilerProject *pProject = m_pMetaDataFile->GetProject();
    if (TypeFromToken(tk) == mdtMethodDef)
    {
        // Note: m_pcontainer can be NULL such as when we are just getting the list of names in a vb module. 
        if ( m_pcontainer && m_pcontainer->IsClass()) 
        {
            bool isAbstract = !!(pmetamember->m_dwFlags & mdAbstract);
            bool isVirtual = !!(pmetamember->m_dwFlags & mdVirtual);
            if ((isAbstract || isVirtual) && m_pcontainer->PClass()->IsMustInherit())
            {
                return true; // Always import virtual/abstract methods of abstract classes regardless of accessibility
            }
        }

        if (m_pMetaDataFile->GetCompiler()->SkipTypeMember(pmetamember->m_pstrName, MapMethodFlagsToDeclFlags(pmetamember->m_dwFlags, pProject->IsCodeModule()),
                pProject, tk, m_pMetaDataFile))
        {
            return false; // don't import this method
        }
    }
    else
    {
        VSASSERT(TypeFromToken(tk) == mdtFieldDef, "What else could this be?");

        if (m_pMetaDataFile->GetCompiler()->SkipTypeMember(pmetamember->m_pstrName, MapFieldFlagsToDeclFlags(pmetamember->m_dwFlags, pProject->IsCodeModule()),
                pProject, tk, m_pMetaDataFile))
            return false;
    }

    return true; // import this method
}

//============================================================================
// Find the member represented by a token.
//============================================================================

BCSYM_NamedRoot *MetaImport::GetMemberOfToken
(
    mdToken tk
)
{
    unsigned long iMemberCheck;

    // If we didn't reference anything
    if (IsNilToken(tk))
    {
        return NULL;
    }

    for (iMemberCheck = 0; iMemberCheck < m_cMembers; iMemberCheck++)
    {
        if (m_rgmembers[iMemberCheck].m_tk == tk)
            break;  // Found it!
    }

    if (iMemberCheck == m_cMembers)
    {
        // We might not find tokens if we're importing FX assemblies because
        // we don't import inaccessible members.
        return NULL;
    }
    else
    {
        return m_rgmembers[iMemberCheck].m_pnamed;
    }
}

//============================================================================
// Get the symbol for a typeref token
//============================================================================

BCSYM *MetaImport::GetTypeOfTypeRef
(
    mdTypeRef tk,
    unsigned cTypeArity
)
{
    mdToken         tkResolutionScope;
    WCHAR           wsz[256];
    unsigned long   cchTypeName, cchTypeNameAgain;
    BCSYM *pType;

    STRING *pstrTypeName, *pstrNameSpace;

    STRING *pstrErrorAssembly = NULL;
    STRING *pstrErrorTypeNameSpace = NULL;
    STRING *pstrErrorTypeName = NULL;

    // First, check to see if the typeref token is in our hashtable
#ifdef _WIN64
    DynamicFixedSizeHashTable<size_t, TokenHashValue> *pTokenHash = m_pMetaDataFile->GetTokenHash();
#else
    DynamicFixedSizeHashTable<mdToken, TokenHashValue> *pTokenHash = m_pMetaDataFile->GetTokenHash();
#endif
    TokenHashValue *pthv;

#ifdef _WIN64
    {
    size_t tkWIN64 = tk;
    pthv = pTokenHash->HashFind(tkWIN64);
    }
#else
    pthv = pTokenHash->HashFind(tk);
#endif

    if (pthv && pthv->pSym)
    {
        return pthv->pSym;
    }

    // Get more information about the reference.
    IfFailThrow(m_pmdImport->GetTypeRefProps(tk,
                                             &tkResolutionScope,
                                             wsz,
                                             sizeof(wsz) / sizeof(WCHAR),
                                             &cchTypeName));

    if (cchTypeName > (sizeof(wsz) / sizeof(WCHAR)))
    {
        VBHeapPtr<WCHAR> wszTypeName(cchTypeName);

        IfFailThrow(m_pmdImport->GetTypeRefProps(tk,
                                                 &tkResolutionScope,
                                                 wszTypeName,
                                                 cchTypeName,
                                                 &cchTypeNameAgain));

        VSASSERT(cchTypeName == cchTypeNameAgain, "GetTypeRefProps: COM+ lied.");

        SplitTypeName(m_pCompiler, wszTypeName, &pstrNameSpace, &pstrTypeName);
    }
    else
    {
        SplitTypeName(m_pCompiler, wsz, &pstrNameSpace, &pstrTypeName);
    }

    bool fTypeForward = false;
    pType =
        GetTypeByName(pstrTypeName,
                      pstrNameSpace,
                      cTypeArity,
                      tkResolutionScope,
                      &fTypeForward);

    if (m_pwinmdImport && pType && pType->IsNamedRoot())
    {
        IfFailThrow(m_pwinmdImport->GetUntransformedTypeRefProps(tk,
                                                                 &tkResolutionScope,
                                                                 wsz,
                                                                 sizeof(wsz) / sizeof(WCHAR),
                                                                 &cchTypeName));

        if (cchTypeName > (sizeof(wsz) / sizeof(WCHAR)))
        {
            VBHeapPtr<WCHAR> wszTypeName(cchTypeName);

            IfFailThrow(m_pwinmdImport->GetUntransformedTypeRefProps(tk,
                                                                     &tkResolutionScope,
                                                                     wszTypeName,
                                                                     cchTypeName,
                                                                     &cchTypeNameAgain));

            VSASSERT(cchTypeName == cchTypeNameAgain, "GetUntransformedTypeRefProps: COM+ lied.");

            pType->PNamedRoot()->SetUntransformedName(m_pCompiler->AddString(wszTypeName));
        }
        else
        {
            pType->PNamedRoot()->SetUntransformedName(m_pCompiler->AddString(wsz));
        }
    }

    if ((pType == NULL || pType->IsBad()) &&
        m_pCompiler->ResolveWinRTTypesByName() &&
        TypeFromToken(tkResolutionScope) == mdtAssemblyRef)
    {
        AssemblyRefInfo *pAssemblyRefInfo =
            m_pMetaDataFile->GetAssemblyRefs()->GetAssemblyRefInfoByToken(tkResolutionScope);

        if (!pAssemblyRefInfo)
        {
            VSASSERT(false, "Non-fatal assert: malformed assembly ref encountered.");
        }
        else if (pAssemblyRefInfo->m_Identity.IsWindowsRuntimeAssembly())
        {
            BCSYM* pWinRTType = GetWindowsRuntimeTypeByName(pstrTypeName, pstrNameSpace, cTypeArity);

            if (pWinRTType != NULL &&
                !pWinRTType->IsBad())
            {
                pType = pWinRTType;
            }
        }
    }

    VSASSERT(pType != NULL, "Type resolution failed unexpectedly!!!");

    TokenHashValue thv;
    thv.pSym = pType;
    thv.fTypeForward = fTypeForward;

#ifdef _WIN64
    {
        size_t tkWIN64 = tk;
        pTokenHash->HashAdd(tkWIN64, thv);
    }
#else
    pTokenHash->HashAdd(tk, thv);
#endif

    return pType;
}

BCSYM *MetaImport::GetTypeByName
(
    _In_z_ STRING *pstrTypeName,
    _In_z_ STRING *pstrNameSpace,
    unsigned cTypeArity,
    mdToken tkResolutionScope,
    _Out_ bool* pfTypeForward
)
{
    // Init out parameters
    if (pfTypeForward)
    {
        *pfTypeForward = false;
    }

    BCSYM *pType = NULL;
    CompilerProject *pReferencedProject = NULL;
    BCSYM_Container *pContainerScope = NULL;
    CompilerProject *pProjectScope = NULL;
    unsigned cTypeArityForNestedType = 0;

    STRING *pstrErrorAssembly = NULL;
    STRING *pstrOrigTypeName = pstrTypeName;

    // The resolution scope should be either a type ref, an assembly or a module.
    if (TypeFromToken(tkResolutionScope) == mdtTypeRef)
    {
        BCSYM *psymContainer = NULL;

        BCSYM_Hash *pLookupHash;
        BCSYM *pBound;

        {
            // Remove the generic name mangling, but only in the case where the arity is non-zero.

            STRING *pstrPossibleTypeName = NULL;

            if (cTypeArity > 0)
            {
                pstrPossibleTypeName = GetActualTypeNameFromEmittedTypeName(pstrTypeName, -1, m_pCompiler, &cTypeArityForNestedType);
            }

            if (cTypeArityForNestedType <= cTypeArity)
            {
                if (pstrPossibleTypeName)
                {
                    pstrTypeName = pstrPossibleTypeName;
                }

                psymContainer = GetTypeOfToken(tkResolutionScope, cTypeArity - cTypeArityForNestedType);

                // Dev10 #680929: Be ready to deal with NamedType wrapper here. 
                if ( psymContainer != NULL && psymContainer->IsNamedType() )
                {
                    psymContainer = psymContainer->PNamedType()->GetSymbol();

                    ThrowIfNull(psymContainer);
                }
            }
        }

        if (!psymContainer  ||  psymContainer->IsBad())
        {
            pType = psymContainer;       // Nothing we can do, propagate the error
            goto Error;
        }

        pContainerScope = psymContainer->PContainer();

        // We could be in the middle of resolving the base classes,
        // in which case we can't load the type
        // to get the hash. In that case, just directly look
        // at the list of nested types
        if (psymContainer->PContainer()->AreChildrenLoaded())
        {
            pLookupHash = psymContainer->PContainer()->GetHash();
        }
        else
        {
            BCSYM_Container *pCurrentContainer, *pNextContainer;

            for (pCurrentContainer = *psymContainer->PContainer()->GetNestedTypeList();
                 pCurrentContainer;
                 pCurrentContainer = pNextContainer)
            {
                pNextContainer = pCurrentContainer->GetNextInSymbolList()->PContainer();

                // We can't compare tokens in this case, as pCurrentContainer->GetToken()
                // will be a TypeDef relative to the referenced project and tk is
                // a TypeRef relative to the current project.  So, compare names instead.
                // See VS RAID 200695 for a good repro of this.
                if (StringPool::IsEqual(pCurrentContainer->GetName(), pstrTypeName) &&
                    pCurrentContainer->GetGenericParamCount() == cTypeArityForNestedType)
                {
                    pType = pCurrentContainer;
                    break;
                }
            }

            goto Error;
        }

        // Find the type.
        pBound = pLookupHash->SimpleBind(pstrTypeName);

        while (pBound &&
               (!pBound->IsContainer() ||
                pBound->PContainer()->GetGenericParamCount() != cTypeArityForNestedType))
        {
            pBound = pBound->PNamedRoot()->GetNextBound();
        }

        if (!pBound)
        {
            goto Error;
        }

        pType = pBound;

        goto Error;
    }
    else if (TypeFromToken(tkResolutionScope) == mdtAssemblyRef)
    {
        AssemblyRefInfo *pAssemblyRefInfo =
            m_pMetaDataFile->GetAssemblyRefs()->GetAssemblyRefInfoByToken(tkResolutionScope);

        if (!pAssemblyRefInfo)
        {
            VSASSERT(false, "Non-fatal assert: malformed assembly ref encountered.");
            goto Error;
        }

        if (pAssemblyRefInfo->m_ErrorID && pAssemblyRefInfo->m_ErrorID < ERRID_last)
        {
            // Note:
            // When the error is ERRID_ReferencedAssemblyCausesCycle3, we use the BadNamespace field to
            // store the assembly identity for the referenced project and store the type name in the
            // fully appended form.

            STRING *pstrErrorTypeName =
                (pAssemblyRefInfo->m_ErrorID != ERRID_ReferencedAssemblyCausesCycle3 ||
                    (!pstrNameSpace || !*pstrNameSpace)) ?
                    pstrTypeName :
                    m_pCompiler->ConcatStrings(pstrNameSpace, L".", pstrTypeName);

            STRING *pstrErrorNamespace =
                (pAssemblyRefInfo->m_ErrorID != ERRID_ReferencedAssemblyCausesCycle3) ?
                    pstrNameSpace :
                    pAssemblyRefInfo->m_Identity.GetAssemblyIdentityString();

            pType = m_Symbols.GetBadNamedRoot(pstrErrorTypeName,
                                                pstrErrorNamespace,
                                                DECLF_Public,
                                                (BindspaceType)(BINDSPACE_Type | BINDSPACE_Normal),
                                                pAssemblyRefInfo->m_ErrorID,
                                                pAssemblyRefInfo->m_pstrErrorExtra,
                                                NULL);

            goto Error;
        }

        pReferencedProject = pAssemblyRefInfo->m_pReferencedProject;
        AssertIfNull(pReferencedProject);
    }
    else if (TypeFromToken(tkResolutionScope) == mdtModuleRef)
    {
        //
        // Get the information we need to find the module.
        //
        VBHeapPtr<WCHAR> wszModuleName;
        unsigned long   cchModuleName;

        STRING *pstrModuleName;
        IfFailThrow(m_pmdImport->GetModuleRefProps(tkResolutionScope,  // [IN] moduleref token.
                                                   NULL,               // [OUT] buffer to fill with the moduleref name.
                                                   0,                  // [IN] size of szName in wide characters.
                                                   &cchModuleName));   // [OUT] actual count of characters in the name.

        wszModuleName.Allocate(cchModuleName);
        IfFailThrow(m_pmdImport->GetModuleRefProps(tkResolutionScope,  // [IN] moduleref token.
                                                   wszModuleName,      // [OUT] buffer to fill with the moduleref name.
                                                   cchModuleName,      // [IN] size of szName in wide characters.
                                                   NULL));             // [OUT] actual count of characters in the name.

        pstrModuleName = m_pCompiler->AddString(wszModuleName);
        pstrErrorAssembly = pstrModuleName;

        // 1. If a module ref is found in a module added to the current compilation unit, then the module ref
        //    is resolved against other modules added to the current compilation unit. This affects only the
        //    command line compiler since modules cannot be added directly to the IDE compiler.
        //
        // 2. If a module ref is found in an DLL (metadata assembly), then the module ref is resolved against
        //    the modules in that assembly.

        MetaDataFile *pFile = NULL;

        if (m_pMetaDataFile->GetProject()->IsCodeModule())
        {
#if !IDE
            // #if !IDE because:
            //  - Direct reference to .Net modules is only allowed in the command line compiler.
            //  - Additionally current decompilation model does not support this linking to other
            //    code modules in the IDE.

            // Loop through all of the metadata .Net module projects looking for the right one.

            ProjectIterator projects(m_pCompilerHost);
            CompilerProject *pProject;

            while (pProject = projects.Next())
            {
                if (pProject->IsCodeModule() &&
                    pProject->GetPrimaryModule() &&
                    StringPool::IsEqual(pProject->GetPrimaryModule()->GetName(), pstrModuleName))
                {
                    pFile = pProject->GetPrimaryModule();
                    break;
                }
            }
#else
            // Unexpected scenario in the IDE.
            AssertIfFalse(false);
#endif !IDE
        }
        else
        {
            // Loop through all of the metadata files in the current assembly looking for the right one.

            MetaDataFileIterator files(m_pMetaDataFile->GetProject());

            while (pFile = files.Next())
            {
                if (StringPool::IsEqual(pFile->GetName(), pstrModuleName))
                {
                    break;
                }
            }
        }

        // No project?  Then we have no reference to this assembly.
        if (!pFile)
        {
            pType = m_Symbols.GetBadNamedRoot(pstrTypeName,
                                              pstrNameSpace,
                                              DECLF_Public,
                                              (BindspaceType)(BINDSPACE_Type | BINDSPACE_Normal),
                                              ERRID_UnreferencedModule3,
                                              pstrModuleName,
                                              NULL);
            goto Error;
        }

        // This results in slightly more work as we'll search right back
        // to the file we have, but it allows us to share the class
        // lookup code.  Modules that are not part of an assembly
        // should be rare so I'm not worried.
        //
        pReferencedProject = pFile->GetProject();
    }
    else
    {
        // The last case is a little bit strange.  Here, the TypeRef's TypeDef
        // lives in the same project as the TypeRef itself.  This is represented
        // as a resolution scope of 0x00000001.
        VSASSERT(TypeFromToken(tkResolutionScope) == mdtModule, "What else can it be?");
        VSASSERT(tkResolutionScope == TokenFromRid(1, mdtModule), "Unexpected mdtModule resolution scope found!");

        pReferencedProject = m_pMetaDataFile->GetProject();
    }

    // We have a project, search through it looking for the right class.
    // We don't have to worry about collisions because assemblies
    // and modules all guarantee that a class name+namespace are
    // unique.

    pProjectScope = pReferencedProject;

    pType = GetTypeByNameInProject(pstrTypeName, pstrNameSpace, cTypeArity, pReferencedProject, pfTypeForward);

Error:
    // Make sure that we return something.
    //
    if (!pType)
    {
        // We can't assert here -- see VS RAID 252261.  Instead, we try
        // to report the assembly/module we were importing from.
        //
        // Depending on what code path we took, one or all of the pstrErrorXXX
        // locals might still be NULL.  The error reporting code can handle
        // a NULL namespace, but replace a null assembly name or type name
        // with "<unknown>"

        STRING *pstrErrorTypeNameSpace = NULL;
        STRING *pstrErrorTypeName = NULL;

        if (!pProjectScope && pContainerScope)
        {
            pProjectScope = pContainerScope->GetContainingProject();
        }

        VSASSERT(pProjectScope, "Unknown project scope unexpected!!!");

        if (!pstrErrorAssembly)
        {
            // Determine the error assembly and the error assembly name.
            //
            if (pProjectScope)
            {
                pstrErrorAssembly = pProjectScope->GetAssemblyName();
            }
            else
            {
                // If the assembly name could not be determined, then set it to the default of <unknown>.
                //
                WCHAR wszUnknown[32];

                if (FAILED(ResLoadString(STRID_Unknown, wszUnknown, DIM(wszUnknown))))
                {
                    // Fall back to the un-localized string
                    wcscpy_s(wszUnknown, _countof(wszUnknown), L"<unknown>");
                }

                pstrErrorAssembly = m_pCompiler->AddString(wszUnknown);
            }
        }

        // Determine the error NameSpace name and the error type name.

        StringBuffer sbTypeName;
        unsigned cCurrentArity;
        if (pContainerScope)
        {
            BCSYM_Container *pNonNestedParentType = pContainerScope;
            while (pNonNestedParentType &&
                    pNonNestedParentType->GetContainer() &&
                    !pNonNestedParentType->GetContainer()->IsNamespace())
            {
                pNonNestedParentType = pNonNestedParentType->GetContainer();
            }

            pstrErrorTypeNameSpace = pNonNestedParentType->GetNameSpace();

            sbTypeName.AppendSTRING(pContainerScope->GetQualifiedName(true, pNonNestedParentType->GetContainer(), true));
            sbTypeName.AppendChar(L'.');

            cCurrentArity = cTypeArityForNestedType;
        }
        else
        {
            pstrErrorTypeNameSpace = pstrNameSpace;

            // Unmangle generic name if not yet done.
            if (cTypeArity > 0 && pstrTypeName == pstrOrigTypeName)
            {
                pstrTypeName = GetActualTypeNameFromEmittedTypeName(pstrTypeName, cTypeArity, m_pCompiler);
            }

            cCurrentArity = cTypeArity;
        }


        sbTypeName.AppendSTRING(pstrTypeName);

        if (cCurrentArity > 0)
        {
            sbTypeName.AppendChar(L'(');
            sbTypeName.AppendSTRING(m_pCompiler->TokenToString(tkOF));
            sbTypeName.AppendChar(L' ');

            unsigned cTypeArityIndex = cCurrentArity - 1;
            while (cTypeArityIndex-- > 0)
            {
                sbTypeName.AppendChar(L',');
            }

            sbTypeName.AppendChar(L')');
        }

        pstrErrorTypeName = m_pCompiler->AddString(sbTypeName.GetString());

        // Create the BadNamedRoots and set the information that needs to be reported in the errors given when
        // these symbols are referenced.
        pType = m_Symbols.GetBadNamedRoot(NULL,
                                            NULL,
                                            DECLF_Private,
                                            BINDSPACE_IgnoreSymbol,
                                            (!pProjectScope || pProjectScope->IsMetaData()) ?
                                            ERRID_TypeRefResolutionError3 :
                                            ERRID_TypeRefFromMetadataToVBUndef,
                                            NULL,
                                            NULL);

        // Set all of the error info for BCSYM_NamedRoot::ReportError
        // to use later.
        pType->PNamedRoot()->SetBadName(pstrErrorTypeName);
        pType->PNamedRoot()->SetBadNameSpace(pstrErrorTypeNameSpace);
        pType->PNamedRoot()->SetBadExtra(pstrErrorAssembly);
    }

    return pType;
}

BCSYM *MetaImport::GetTypeByNameInProject
(
    _In_z_ STRING *pstrTypeName,
    _In_z_ STRING* pstrNameSpace,
    unsigned cTypeArity,
    _In_ CompilerProject *pReferencedProject,
    _Out_ bool* pfTypeForward
)
{
    // Init out parameters
    if (pfTypeForward)
    {
        *pfTypeForward = false;
    }

    BCSYM *pType = NULL;

    if (pReferencedProject->IsMetaData())
    {
        MetaDataFileIterator files(pReferencedProject);
        MetaDataFile *pFile;

        while (pFile = files.Next())
        {
            DynamicFixedSizeHashTable<STRING *, BCSYM_Container *> *pNameHash = pFile->GetNameHash();

            // Bug VSWhidbey 535411.
            if (pNameHash)
            {
                BCSYM_Container **ppSym = pNameHash->HashFind(pstrTypeName);

                while (ppSym)
                {
                    BCSYM_Container *pSym = *ppSym;

                    if (StringPool::IsEqual(pstrTypeName, pSym->GetEmittedName()) &&
                        StringPool::IsEqual(pstrNameSpace, pSym->GetNameSpace()) &&
                        pSym->GetGenericParamCount() == cTypeArity)
                    {
                        pType = pSym;
                        goto BreakOut;
                    }

                    ppSym = pNameHash->FindNext();
                }
            }

            // Look in the type forwarders of the primary module of this assembly, clr does not honor type forwarder
            // in non-primary modules.
            //
            if (!pType &&
                (pFile == pReferencedProject->GetPrimaryModule()) &&
                pFile->GetNameHashForTypeFwds())                // Bug VSWhidbey 535411.
            {
                DynamicFixedSizeHashTable<STRING *, BCSYM_TypeForwarder *> *pNameHashForTypeFwds = pFile->GetNameHashForTypeFwds();
                BCSYM_TypeForwarder **ppTypeFwd = pNameHashForTypeFwds->HashFind(pstrTypeName);

                while (ppTypeFwd)
                {
                    BCSYM_TypeForwarder *pTypeFwd = *ppTypeFwd;

                    if (StringPool::IsEqual(pstrTypeName, pTypeFwd->GetEmittedName()) && StringPool::IsEqual(pstrNameSpace, pTypeFwd->GetNameSpace()))
                    {
                        pType = GetTypeOfTypeForwarder(pTypeFwd, cTypeArity);
                        if (pfTypeForward)
                        {
                            *pfTypeForward = true;
                        }
                        goto BreakOut;
                    }

                    ppTypeFwd = pNameHashForTypeFwds->FindNext();
                }
            }
        }
    }
    else
    {
        // Remove the generic name mangling, but only in the case where the arity is provided.
        //
        if (cTypeArity > 0)
        {
            pstrTypeName = GetActualTypeNameFromEmittedTypeName(pstrTypeName, cTypeArity, m_pCompiler);
        }

        pType = BindTypeInVBProject(pReferencedProject, pstrNameSpace, pstrTypeName, cTypeArity);

        if (pType)
        {
            if (!pType->IsNamedRoot())
            {
                pType = NULL;
            }
        }
    }

BreakOut:

    return pType;
}

BCSYM *MetaImport::GetTypeOfTypeForwarder
(
    BCSYM_TypeForwarder *pTypeFwd,
    unsigned cTypeArity
)
{
    BCSYM *pType;

    if (pTypeFwd->IsTypeForwarderResolved())
    {
        pType = pTypeFwd->GetTypeForwardedTo();
    }
    else
    {
        pType = LoadTypeOfTypeForwarder(pTypeFwd, m_pCompiler, cTypeArity);
    }

    return pType;
}

BCSYM *MetaImport::DoLoadTypeOfTypeForwarder
(
    BCSYM_TypeForwarder *pTypeFwd,
    unsigned cTypeArity
)
{
    if (pTypeFwd->IsTypeForwarderBeingResolved())
    {
        // Cycle detected.
        pTypeFwd->SetErrid(ERRID_TypeFwdCycle2);
        return pTypeFwd;
    }

    if (!pTypeFwd->IsTypeForwarderResolved())
    {
        mdAssemblyRef tkDestAssemRef = pTypeFwd->GetAssemblyRefForwardedTo();

        pTypeFwd->SetTypeForwarderBeingResolved(true);

        BCSYM *pDestType =
            GetTypeByName(pTypeFwd->GetName(),
                          pTypeFwd->GetNameSpace(),
                          cTypeArity,
                          tkDestAssemRef);

        VSASSERT(pDestType != NULL, "NULL type unexpected during metadata type resolution!!!");

        if (pDestType->IsTypeForwarder())
        {
            // Propagate cycle error
            VSASSERT(pDestType->PTypeForwarder()->GetErrid() == ERRID_TypeFwdCycle2, "Non cycle error unexpected!!!");
            pTypeFwd->SetErrid(ERRID_TypeFwdCycle2);
        }

        // Get the name of the destination assembly

        STRING *pstrDestinationAssemblyName = NULL;
        if (pDestType && pDestType->IsContainer())
        {
            pstrDestinationAssemblyName = pDestType->PContainer()->GetContainingProject()->GetAssemblyName();
        }
        else
        {
            HRESULT hr = NOERROR;
            unsigned long cchAssemblyName;
            WCHAR wsz[256];
            WCHAR *wszLongAssemblyName = NULL;
            CComPtr<IMetaDataAssemblyImport> srpAssemblyImport;

            IfFailThrow(m_pmdImport->QueryInterface(IID_IMetaDataAssemblyImport, (void **)&srpAssemblyImport));

            IfFailThrow(srpAssemblyImport->GetAssemblyRefProps(tkDestAssemRef,                // [IN] The AssemblyRef for which to get the properties.
                                                               NULL,                          // [OUT] Pointer to the Originator blob.
                                                               NULL,                          // [OUT] Count of bytes in the Originator Blob.
                                                               wsz,                           // [OUT] Buffer to fill with name.
                                                               DIM(wsz),                      // [IN] Size of buffer in wide chars.
                                                               &cchAssemblyName,              // [OUT] Actual # of wide chars in name.
                                                               NULL,                          // [OUT] Assembly MetaData.
                                                               NULL,                          // [OUT] Hash blob.
                                                               NULL,                          // [OUT] Count of bytes in the hash blob.
                                                               NULL));                        // [OUT] Flags.

            if (cchAssemblyName > DIM(wsz))
            {
                wszLongAssemblyName = VBAllocator::AllocateArray<WCHAR>(cchAssemblyName);

                IfFailThrow(srpAssemblyImport->GetAssemblyRefProps(tkDestAssemRef,                // [IN] The AssemblyRef for which to get the properties.
                                                                   NULL,                          // [OUT] Pointer to the Originator blob.
                                                                   NULL,                          // [OUT] Count of bytes in the Originator Blob.
                                                                   wszLongAssemblyName,           // [OUT] Buffer to fill with name.
                                                                   cchAssemblyName,               // [IN] Size of buffer in wide chars.
                                                                   NULL,                          // [OUT] Actual # of wide chars in name.
                                                                   NULL,                          // [OUT] Assembly MetaData.
                                                                   NULL,                          // [OUT] Hash blob.
                                                                   NULL,                          // [OUT] Count of bytes in the hash blob.
                                                                   NULL));                        // [OUT] Flags.

                pstrDestinationAssemblyName = m_pCompiler->AddString(wszLongAssemblyName);
                delete[] wszLongAssemblyName;
            }
            else
            {
                pstrDestinationAssemblyName = m_pCompiler->AddString(wsz);
            }
        }

        pTypeFwd->SetTypeForwardedTo(pDestType);
        pTypeFwd->SetBadExtra(pstrDestinationAssemblyName);
        pTypeFwd->SetTypeForwarderBeingResolved(false);
        pTypeFwd->SetTypeForwarderResolved(true);
    }

    return pTypeFwd->GetTypeForwardedTo();
}

BCSYM *MetaImport::LoadTypeOfTypeForwarder
(
    BCSYM_TypeForwarder *TypeFwd,
    Compiler *Compiler,
    unsigned cTypeArity
)
{
    CompilerIdeLock spLock(Compiler->GetMetaImportCritSec());
    BCSYM *pType = NULL;

    MetaDataFile *pMetaDataFile = TypeFwd->GetContainer()->GetMetaDataFile();
    MetaImport import(Compiler, pMetaDataFile, NULL);

    pType = import.DoLoadTypeOfTypeForwarder(TypeFwd, cTypeArity);

    return pType;
}

//============================================================================
// Get the symbol for a token
//============================================================================

BCSYM *MetaImport::GetTypeOfToken
(
    mdToken tk,
    unsigned cTypeArity,
    bool ImportBaseAndImplements
)
{
    BCSYM *pType = NULL;

    if (IsNilToken(tk))
    {
        VSFAIL("didn't expect null type ref token");
    }
    else if (TypeFromToken(tk) == mdtTypeDef)
    {
        // If the token type is a "typedef" then the type being referenced
        // must be defined within this module.
#ifdef _WIN64
        DynamicFixedSizeHashTable<size_t, TokenHashValue> *pTokenHash = m_pMetaDataFile->GetTokenHash();
#else
        DynamicFixedSizeHashTable<mdToken, TokenHashValue> *pTokenHash = m_pMetaDataFile->GetTokenHash();
#endif
        TokenHashValue *pthv;

        // Get the symbol for the typedef from the save types in the file. If it's inaccessible,
        // we're going to get NULL, which the caller just has to handle. However, this will ONLY
        // happen when we are importing FX assemblies and you have a type that
        // implements an inaccessible type. Otherwise, access exposure rules
        // protect us in all other cases.
#ifdef _WIN64
        {
        size_t tkTemp = tk;
        pthv = pTokenHash->HashFind(tkTemp);
        }
#else
        pthv = pTokenHash->HashFind(tk);
#endif

        if (!pthv)
        {
            return NULL;
        }

        pType = pthv->pSym;
    }
    else if (TypeFromToken(tk) == mdtTypeSpec)
    {
        // The signature of the typespec
        PCCOR_SIGNATURE pvSig = NULL;
        ULONG cbSig;

        VSASSERT(cTypeArity == 0, "Unexpected type arity for TypeSpec!!!");

        // Get more information about the typespec
        //
        IfFailThrow(m_pmdImport->GetTypeSpecFromToken(tk,
                                                      &pvSig,
                                                      &cbSig));

        AssertIfFalse(cbSig > 0);

        bool TypeIsBad = false;
        pType = DecodeType(&pvSig, TypeIsBad);

        VSASSERT(pType != NULL, "How could this possibly happen?");
    }
    else
    {
        // If its not a typedef or a typespec then it must be a typref. We first
        // have to figure out where the symbol lives and then munge around in
        // there to find it.
        VSASSERT(TypeFromToken(tk) == mdtTypeRef, "What else can it be?");
        pType = GetTypeOfTypeRef((mdTypeRef)tk, cTypeArity);
    }

#if DEBUG
    if (pType && !pType->IsBad())
    {
        int cActualTypeArity = 0;

        if (pType->IsContainer() && !pType->IsGenericBinding())
        {
            for(BCSYM_Container *pPossibleGeneric = pType->PContainer();
                pPossibleGeneric;
                pPossibleGeneric = pPossibleGeneric->GetContainer())
            {
                cActualTypeArity += pPossibleGeneric->GetGenericParamCount();
            }
        }

        VSASSERT(cActualTypeArity == cTypeArity, "Type arity mismatch unexpected!!!");
    }
#endif DEBUG

    if (pType && !pType->IsBad() && pType->IsContainer() && !pType->PContainer()->AreBaseAndImplementsLoaded())
    {
        if (!ImportBaseAndImplements)
        {
            // Handles circular references between metadata projects.

            CompilerProject *pParentProject  =
                pType->PContainer()->GetCompilerFile() ? pType->PContainer()->GetCompilerFile()->GetProject() : NULL;

            if (pParentProject &&
                pParentProject != m_pMetaDataFile->GetProject() &&
                pParentProject->GetCompState() == CS_Declared)
            {
                ImportBaseAndImplements = true;
            }
        }

        if (ImportBaseAndImplements)
        {
            if (pType->PContainer()->AreBaseAndImplementsLoading())
            {
                // Inheritance cycle detected. Break it, else the rest of
                // the compiler will have problems consuming this class.

                return m_Symbols.GetBadNamedRoot(NULL,
                                                NULL,
                                                DECLF_Private,
                                                BINDSPACE_IgnoreSymbol,
                                                ERRID_InheritanceCycleInImportedType1,
                                                NULL,
                                                NULL);
            }

            this->ImportBaseAndImplements(pType->PContainer(), false);
        }
    }

    if (pType != NULL && 
        !pType->IsGenericBinding() &&  // We do not embed generic bindings, let's filter them out early
        pType->IsContainer() &&
        TypeHelpers::IsMarkedAsEmbeddedLocalType(pType->PContainer())) // Dev10 #680929: Ignore relationship between symbol's project and the project being compiled. 
    {

        // Wrap the reference to this local type in a NamedType instance. When referring to this type later, 
        // we will have to dig through the wrapper, and that will give us an opportunity to resolve the local 
        // type against an appropriate canonical type. Assuming that a canonical type exists, the compiler will
        // not have to deal with any direct references to embedded local types.
        STRING *gqn = pType->PNamedRoot()->GetQualifiedName();
        unsigned NameCount = m_pCompiler->CountQualifiedNames(gqn);
        BCSYM_NamedRoot *pContext = pType->PNamedRoot()->GetContainer();
        BCSYM_NamedType *pWrapper = m_Symbols.GetNamedType(pType->GetLocation(), pContext, NameCount, NULL);
        m_pCompiler->SplitQualifiedName(gqn, NameCount, pWrapper->GetNameArray());
        pWrapper->SetSymbol(pType);
        pType = pWrapper;
    }

    return pType;
}


//****************************************************************************
// Signature decoding routines.
//****************************************************************************

//============================================================================
// Decodes a method's signature.
//============================================================================

BCSYM *MetaImport::DecodeMethodSignature
(
    mdMethodDef md,
    const COR_SIGNATURE *pSig,
    unsigned long cbSig,
    bool ForProperty,
    _Inout_ bool &MethodIsBad,
    _Out_ BCSYM_Param **ppparamList,
    _Out_ PCCOR_SIGNATURE *pRetValSig,
    _Out_ ULONG *pcbRetValSig,
    _Inout_ BCSYM_NamedRoot **ppBadTypeSymbol
)
{
#if DEBUG
    COR_SIGNATURE *pSigEnd = (COR_SIGNATURE *)((BYTE *)pSig + cbSig);
#endif

    HRESULT hr;

    unsigned long iParam, cParams, uSequence, cConv;
    BCSYM *ptypReturn;
    mdParamDef pd;
    BCSYM_Param *pparamFirst =  NULL;
    BCSYM_Param *pparamLast = NULL;
    BCSYM_Param *pparamCreated;
    DWORD dwAttr;

    // Format:
    //    calling convention  - 1 byte
    //    argument count      - variable integer
    //    return type         - variable encoded
    //    argument types      - variable encoded
    //

    // Read the calling convention.
    cConv = DecodeInteger(&pSig);
    VSASSERT(pSig != pSigEnd, "Out of buffer.");

    if (cConv == IMAGE_CEE_CS_CALLCONV_VARARG)
    {
        MethodIsBad = true;
    }
    else if (cConv & IMAGE_CEE_CS_CALLCONV_GENERIC)
    {
        // Get the type parameter count.
        DecodeInteger(&pSig);
    }

    // Get the parameter count
    cParams = DecodeInteger(&pSig);
    VSASSERT(pSig != pSigEnd, "Out of buffer.");

    // get the return type
    ptypReturn = DecodeType(&pSig, MethodIsBad);

    if (ptypReturn->IsBad())
    {
        *ppBadTypeSymbol = ptypReturn->PNamedRoot();
    }

    if (ptypReturn->IsVoidType())
    {
        ptypReturn = NULL;
    }

    if (MethodIsBad)
        goto BadSignature;  // Skip the rest of the signature

    // Get the return type data.
    if (ptypReturn != NULL && pRetValSig != NULL)
    {
        hr = m_pmdImport->GetParamForMethodIndex(md, 0, &pd);
        if (SUCCEEDED(hr))
        {
            IfFailThrow(m_pmdImport->GetParamProps(
                pd,
                NULL,
                NULL,
                NULL,
                0,
                NULL,
                &dwAttr,
                NULL,
                NULL,
                NULL));
            if (dwAttr & pdHasFieldMarshal)
            {
                IfFailThrow(m_pmdImport->GetFieldMarshal(pd, pRetValSig, pcbRetValSig));
            }
        }
    }

    // Get all of the parameters.
    for (iParam = 0; iParam < cParams; iParam++)
    {
        // The information we need to create a parameter.
        STRING *pstrName = NULL;
        BCSYM *ptyp;
        PARAMFLAGS uFlags = 0;
        BCSYM_Expression *pconstexpr;
        bool bHasMarshalAs = false;
        PCCOR_SIGNATURE marshalNativeType = NULL;
        ULONG cbMarshalNativeType = 0;

        // Figure out the type.
        VSASSERT(pSig != pSigEnd, "Out of buffer.");

        ptyp = DecodeType(&pSig, MethodIsBad);

        if (*ppBadTypeSymbol == NULL && ptyp->IsBad())
        {
            *ppBadTypeSymbol = ptyp->PNamedRoot();
        }

        if (ptyp->IsVoidType())
            MethodIsBad = true;

        if (MethodIsBad)
            goto BadSignature;

        // Get the parameter information.
        uSequence = iParam + 1;

        // Get the param token
        pd = mdParamDefNil;
        hr = m_pmdImport->GetParamForMethodIndex(md,
                                                 uSequence,
                                                 &pd);
        if (SUCCEEDED(hr))
        {
            WCHAR wsz[256];
            unsigned long cchName;

            DWORD dwTypeFlag;
            const void *pvValue;
            ULONG cbValue;

            IfFailThrow(m_pmdImport->GetParamProps(pd,
                                                   NULL,
                                                   &uSequence,
                                                   wsz,
                                                   sizeof(wsz) / sizeof(WCHAR),
                                                   &cchName,
                                                   &dwAttr,
                                                   &dwTypeFlag,
                                                   &pvValue,
                                                   &cbValue));
            VSASSERT(uSequence == iParam + 1, "Why did this change?");

            // Check for special flags.
            if (dwAttr & pdHasFieldMarshal)
            {
                IfFailThrow(m_pmdImport->GetFieldMarshal(pd, &marshalNativeType, &cbMarshalNativeType));
            }

            if (cchName > (sizeof(wsz) / sizeof(WCHAR)))
            {
                // Allocate space for the name.
                VBHeapPtr<WCHAR> wszName(cchName);
                IfFailThrow(m_pmdImport->GetParamProps(pd,
                                                       NULL,
                                                       NULL,
                                                       wszName,
                                                       cchName,
                                                       NULL,
                                                       NULL,
                                                       NULL,
                                                       NULL,
                                                       NULL));

                // Create the name.
                pstrName = m_pCompiler->AddString(wszName);
            }
            else if (cchName == 0)
            {
                pstrName = STRING_CONST(m_pCompiler, Param);
            }
            else
            {
                pstrName = m_pCompiler->AddString(wsz);
            }

            // Get the default value.
            pconstexpr = DecodeValue(dwTypeFlag, pvValue, cbValue, ptyp);
        }
        else
        {
            // It's valid for the parameter row to be omitted for a parameter which has
            // no data in order to reduce metadata bloat.  In this case it means there
            // is no name and no default expression
            VSASSERT(CLDB_E_RECORD_NOTFOUND == hr, "Non-fatal import failure:  Please file a bug on the VB compiler");
            pstrName = STRING_CONST(m_pCompiler, Param);
            pconstexpr = NULL;
            dwAttr = 0;
        }

        // Set the flags.
        uFlags = 0;

        if (ptyp->IsPointerType())
        {
            uFlags |= PARAMF_ByRef;
        }

        if (dwAttr & pdOptional)
        {
            uFlags |= PARAMF_Optional;

        }

        // An parameter may be marshalled to VB6 Variant or VB6 Object.
        // This affects the default value we need to load for optional parameters
        // (if the user didn't specify a default value, which was possible in VB6),
        // so figure out which type we are marshalling to.

        if (dwAttr & pdHasFieldMarshal)
        {
            // The only native types which map to VB6 Object are Interface, IUnknown, and IDispatch.
            if (cbMarshalNativeType > 0)
            {
                VSASSERT(marshalNativeType, "They told us there was a signature");

                switch ((CorNativeType)*marshalNativeType)
                {
                    case NATIVE_TYPE_INTF:
                    case NATIVE_TYPE_IUNKNOWN:
                    case NATIVE_TYPE_IDISPATCH:
                        uFlags |= PARAMF_MarshalAsObject;
                        break;

                    default:
                        break;// nothing to do since this isn't a VB6 object
                }
            }
        }

        // Create the parameter.
        pparamCreated = m_Symbols.GetParam(NULL,
                                           pstrName,
                                           ptyp,
                                           uFlags,
                                           pconstexpr,
                                           &pparamFirst,
                                           &pparamLast,
                                           false);  // Not a return type param

        if (pd != mdParamDefNil)
        {
            Attribute::----AllAttributesOnToken(pd,
                                                 m_pmdImport,
                                                 m_Symbols.GetNorlsAllocator(),
                                                 m_pCompiler,
                                                 m_pMetaDataFile->GetProject(),
                                                 m_pMetaDataFile,
                                                 pparamCreated);

            // There are a number of pseudocustom attributes we will have to load up and store
            // on the WellKnownAttrVals object. If any of these bits are set, allocate the attrvals.
            if (dwAttr & (pdHasFieldMarshal | pdIn | pdOut))
            {
                if (pparamCreated->GetPWellKnownAttrVals() == NULL)
                {
                    pparamCreated->AllocAttrVals(m_Symbols.GetNorlsAllocator(), CS_MAX, NULL, NULL, true);
                }
            }

            if (dwAttr & pdHasFieldMarshal)
            {
                COR_SIGNATURE *marshalCopy = (COR_SIGNATURE*)m_Symbols.GetNorlsAllocator()->Alloc(cbMarshalNativeType);
                memcpy(marshalCopy, marshalNativeType, cbMarshalNativeType);
                pparamCreated->GetPWellKnownAttrVals()->SetMarshalAsData(marshalCopy, cbMarshalNativeType);
            }
            if (dwAttr & pdIn)
            {
                pparamCreated->GetPWellKnownAttrVals()->SetInData();
            }
            if (dwAttr & pdOut)
            {
                pparamCreated->GetPWellKnownAttrVals()->SetOutData();
            }

            if (pparamCreated->GetPWellKnownAttrVals()->GetOptionCompareData())
            {
                // This parameter is marked to use the OptionCompare setting
                pparamCreated->SetIsOptionCompare(true);
            }

            // Extract decimal constant, if exists
            DECIMAL DecimalConstant;
            if (pparamCreated->GetPWellKnownAttrVals()->GetDecimalConstantData(&DecimalConstant))
            {
                ConstantValue Value;
                BCSYM_Expression *Expression;

                Value.TypeCode = t_decimal;
                Value.Decimal = DecimalConstant;

                Expression = m_Symbols.GetFixedExpression(&Value);

                // Morph the variable
                Symbols::MorphParamToOptional(pparamCreated, Expression);
            }

            // Extract DateTime constant, if exists
            __int64 DateTimeConstant;
            if (pparamCreated->GetPWellKnownAttrVals()->GetDateTimeConstantData(&DateTimeConstant))
            {
                ConstantValue Value;
                BCSYM_Expression *Expression;

                Value.TypeCode = t_date;
                Value.Integral = DateTimeConstant;

                Expression = m_Symbols.GetFixedExpression(&Value);

                // Morph the variable
                Symbols::MorphParamToOptional(pparamCreated, Expression);
            }
        }
    }

    VSASSERT(pSig == pSigEnd, "Not at the end of the buffer.");

    // Return the results.
    *ppparamList = pparamFirst;

BadSignature:
    return ptypReturn;
}

//============================================================================
// Decodes a fields's signature.
//============================================================================

BCSYM *MetaImport::DecodeFieldSignature
(
    const COR_SIGNATURE *pSig,
    unsigned long cbSig,
    _Inout_ bool &FieldIsBad
)
{
#if DEBUG
    COR_SIGNATURE *pSigEnd = (COR_SIGNATURE *)((BYTE *)pSig + cbSig);
#endif

    BCSYM * ptyp;
    unsigned long ulData;

    // Format:
    //    calling convention  - 1 byte
    //    field type          - variable encoded
    //

    // Read the calling convention.
    ulData = DecodeInteger(&pSig);
    VSASSERT(pSig != pSigEnd, "Out of buffer.");

    if (this->m_pCompiler->IgnoreVolatileModifier())
    {
        // Peek for CMOD_REQD.  If it is there then check it is the System.Runtime.CompilerServices.IsVolatile
        // modifier type.  Allow the import if IsVolatile is present, intended only for the EE.
        const COR_SIGNATURE *pSigPeek = pSig;
        unsigned long ulDataPeek = DecodeInteger(&pSigPeek);
        if (ulDataPeek == ELEMENT_TYPE_CMOD_REQD)
        {
            mdToken token = DecodeToken(&pSigPeek);
            BCSYM* ptyp = GetTypeOfToken(token, 0);

            if (ptyp && m_pCompilerHost->GetFXSymbolProvider()->IsTypeAvailable(FX::IsVolatileType) &&
                BCSYM::AreTypesEqual(ptyp, m_pCompilerHost->GetFXSymbolProvider()->GetType(FX::IsVolatileType)))
            {
                // Skip over the IsVolatile CMOD_REQD to allow the import
                pSig = pSigPeek;
            }
        }
    }

    // get the field type
    ptyp = DecodeType(&pSig, FieldIsBad);

    if (ulData != IMAGE_CEE_CS_CALLCONV_FIELD || ptyp->IsVoidType())
        FieldIsBad = true;
    
    //Microsoft I believe this assertion is too strong. I have seen it fire
    //on a seemingly fine signature.
    //VSASSERT((pSig == pSigEnd) || FieldIsBad, "Not at end of buffer.");

    return ptyp;
}

//============================================================================
// Pull an integer out of a signature.
//============================================================================

unsigned long MetaImport::DecodeInteger
(
    const COR_SIGNATURE **ppSig
)
{
    unsigned long cDecoded;
    unsigned long ulDecoded;

    cDecoded = CorSigUncompressData(*ppSig, &ulDecoded);
    VSASSERT(cDecoded != -1, "should be valid.");

    *ppSig = (COR_SIGNATURE *)((BYTE *)*ppSig + cDecoded);

    return ulDecoded;
}

//============================================================================
// Pull an token out of a signature.
//============================================================================

mdToken MetaImport::DecodeToken
(
    const COR_SIGNATURE **ppSig
)
{
    unsigned long cDecoded;
    mdToken tok;

    cDecoded = CorSigUncompressToken(*ppSig, &tok);
    VSASSERT(cDecoded != -1, "should be valid.");

    *ppSig = (COR_SIGNATURE *)((BYTE *)*ppSig + cDecoded);

    return tok;
}

static BCSYM_GenericParam *
GetGenericTypeParam
(
    BCSYM_Container *ContainingType,
    unsigned long ParamMetaDataPosition
)
{
    while (ContainingType && ContainingType->IsType())
    {
        for (BCSYM_GenericParam *Param = ContainingType->GetFirstGenericParam(); Param; Param = Param->GetNextParam())
        {
            if (Param->GetMetaDataPosition() == ParamMetaDataPosition)
            {
                return Param;
            }
        }

        ContainingType = ContainingType->GetContainer();
    }

    // 




    return NULL;
}

static BCSYM_GenericParam *
GetGenericMethodTypeParam
(
    BCSYM_GenericParam *FirstMethodGenericParam,
    unsigned long ParamMetaDataPosition
)
{
    VSASSERT(FirstMethodGenericParam != NULL,
                "Generic method type param reference unexpected in the absence of a generic method context!!!");

    for (BCSYM_GenericParam *Param = FirstMethodGenericParam; Param; Param = Param->GetNextParam())
    {
        if (Param->GetMetaDataPosition() == ParamMetaDataPosition)
        {
            return Param;
        }
    }

    VSFAIL("Failed to find a type parameter during metadata importation.");
    return NULL;
}

void MetaImport::ConsumeTypes(const COR_SIGNATURE ** ppSig, unsigned long ArgumentCount, _Inout_ bool & TypeIsBad)
{
    for (unsigned long i = 0; i < ArgumentCount; ++i)
    {
        DecodeType(ppSig, TypeIsBad);
    }
}

//============================================================================
// Find the type that represents this signature element.
//============================================================================

BCSYM *MetaImport::DecodeType
(
    const COR_SIGNATURE **ppSig,
    _Inout_ bool &TypeIsBad
)
{
    unsigned long ulCode;
    BCSYM *ptyp = NULL;

    // Get the type.
    ulCode = DecodeInteger(ppSig);

    // Switch on the type.
    switch (ulCode)
    {
    // Do not create a type sym for void types. Return NULL instead
    case ELEMENT_TYPE_VOID:
        ptyp = Symbols::GetVoidType();
        break;

    case ELEMENT_TYPE_BOOLEAN:
        ptyp = m_pCompilerHost->GetFXSymbolProvider()->GetType(t_bool);
        break;

    case ELEMENT_TYPE_I1:
        ptyp = m_pCompilerHost->GetFXSymbolProvider()->GetType(t_i1);
        break;

    case ELEMENT_TYPE_U1:
        ptyp = m_pCompilerHost->GetFXSymbolProvider()->GetType(t_ui1);
        break;

    case ELEMENT_TYPE_I2:
        ptyp = m_pCompilerHost->GetFXSymbolProvider()->GetType(t_i2);
        break;

    case ELEMENT_TYPE_U2:
        ptyp = m_pCompilerHost->GetFXSymbolProvider()->GetType(t_ui2);
        break;

    case ELEMENT_TYPE_I4:
        ptyp = m_pCompilerHost->GetFXSymbolProvider()->GetType(t_i4);
        break;

    case ELEMENT_TYPE_U4:
        ptyp = m_pCompilerHost->GetFXSymbolProvider()->GetType(t_ui4);
        break;

    case ELEMENT_TYPE_I8:
        ptyp = m_pCompilerHost->GetFXSymbolProvider()->GetType(t_i8);
        break;

    case ELEMENT_TYPE_U8:
        ptyp = m_pCompilerHost->GetFXSymbolProvider()->GetType(t_ui8);
        break;

    case ELEMENT_TYPE_R4:
        ptyp = m_pCompilerHost->GetFXSymbolProvider()->GetType(t_single);
        break;

    case ELEMENT_TYPE_R8:
        ptyp = m_pCompilerHost->GetFXSymbolProvider()->GetType(t_double);
        break;

    case ELEMENT_TYPE_CHAR:
        ptyp = m_pCompilerHost->GetFXSymbolProvider()->GetType(t_char);
        break;

    case ELEMENT_TYPE_STRING:
        ptyp = m_pCompilerHost->GetFXSymbolProvider()->GetType(t_string);
        break;

    case ELEMENT_TYPE_BYREF:
        ptyp = DecodeType(ppSig, TypeIsBad);

        if (!ptyp->IsBad())
        {
            ptyp = m_Symbols.MakePtrType(ptyp);
        }
        break;

    case ELEMENT_TYPE_VALUETYPE:
    case ELEMENT_TYPE_CLASS:

        ptyp = GetTypeOfToken(DecodeToken(ppSig), 0);
        // some dlls ( i.e. vjslib can contain public methods that can have private types on signature (returns or params)
        // That is allong with upcomming friend assemblies.
        // we have to relax the following assert
        //VSASSERT(ptyp, "How can this not be found?");

        if (!ptyp)
            TypeIsBad = true;

        break;

    case ELEMENT_TYPE_ARRAY:
        {
            unsigned long cDims;
            unsigned long cBounds;
            unsigned long cLBounds;
            unsigned long i;

            ptyp = DecodeType(ppSig, TypeIsBad);

            if (!ptyp->IsBad())
            {
                cDims = DecodeInteger(ppSig);
                cBounds = DecodeInteger(ppSig);

                // If bounds are specified, ignore them -- we don't support it
                if (cBounds)
                {
                    for (i = 0; i < cBounds; i++)
                    {
                        DecodeInteger(ppSig);
                    }
                }

                cLBounds = DecodeInteger(ppSig);

                // Also ignore lower bounds since we don't support anything but zero
                if (cLBounds)
                {
                    for (i = 0; i < cLBounds; i++)
                    {
                        DecodeInteger(ppSig);
                    }
                }

                ptyp = m_Symbols.GetArrayType(cDims, ptyp);
            }
        }
        break;

    case ELEMENT_TYPE_I:
        ptyp = m_pCompilerHost->GetFXSymbolProvider()->GetType(FX::IntPtrType);
        if (!ptyp)
            TypeIsBad = true;
        break;

    case ELEMENT_TYPE_U:
        ptyp = m_pCompilerHost->GetFXSymbolProvider()->GetType(FX::UIntPtrType);
        if (!ptyp)
            TypeIsBad = true;
        break;

    case ELEMENT_TYPE_OBJECT:
        ptyp = m_pCompilerHost->GetFXSymbolProvider()->GetType(FX::ObjectType);
        break;

    case ELEMENT_TYPE_SZARRAY:
        ptyp = DecodeType(ppSig, TypeIsBad);

        if (!ptyp->IsBad())
        {
            ptyp = m_Symbols.GetArrayType(1, ptyp);
        }
        break;

    case ELEMENT_TYPE_CMOD_OPT:
        DecodeToken(ppSig);
        ptyp = DecodeType(ppSig, TypeIsBad);
        break;

    case ELEMENT_TYPE_VAR:
    {
        unsigned long ParamPosition = DecodeInteger(ppSig);

        ptyp = GetGenericTypeParam(m_pcontainer, ParamPosition);


        // NULL type params may be returned for generic types with unmangled names.
        //
        // 



        if (ptyp == NULL)
        {
            TypeIsBad = true;
        }

        break;
    }

    case ELEMENT_TYPE_MVAR:
    {
        unsigned long ParamPosition = DecodeInteger(ppSig);

        ptyp = GetGenericMethodTypeParam(m_CurrentMethodGenericParams, ParamPosition);
        break;
    }

    case ELEMENT_TYPE_GENERICINST:
    {
        unsigned long ElementKind = DecodeInteger(ppSig);
        mdToken tkGeneric = DecodeToken(ppSig);
        unsigned long ArgumentCount = DecodeInteger(ppSig);

        BCSYM *Generic = GetTypeOfToken(tkGeneric, ArgumentCount);

        if (!Generic)
        {
            TypeIsBad = true;
            ConsumeTypes(ppSig, ArgumentCount, TypeIsBad);
        }
        else if (Generic->IsBad())
        {
            ptyp = Generic;
            ConsumeTypes(ppSig, ArgumentCount, TypeIsBad);
        }
        else
        {
            BCSYM **Arguments;
            StackAllocTypeArgumentsIfPossible(Arguments, ArgumentCount, m_Symbols.GetNorlsAllocator(), m_Symbols);

            for (unsigned long ArgumentIndex = 0; ArgumentIndex < ArgumentCount; ArgumentIndex++)
            {
                Arguments[ArgumentIndex] = DecodeType(ppSig, TypeIsBad);

                if (Arguments[ArgumentIndex]->IsBad())
                {
                    ptyp = Arguments[ArgumentIndex];
                    ConsumeTypes(ppSig, ArgumentCount - ArgumentIndex - 1, TypeIsBad);
                    goto TypeArgumentError;
                }
            }

            // The instantiated type might have a generic parent, in which case some or all of the type
            // arguments might actually be for the parent.

            ptyp = BuildGenericBinding(Generic, ArgumentCount, Arguments, ArgumentsStackAllocated);
        }
TypeArgumentError:
        break;
    }

    case ELEMENT_TYPE_END:
        VSFAIL("unexpected ELEMENT_TYPE_END in signature");
        __fallthrough;

    case ELEMENT_TYPE_TYPEDBYREF:
    case ELEMENT_TYPE_FNPTR:
    case ELEMENT_TYPE_CMOD_REQD:
    case ELEMENT_TYPE_SENTINEL:
    case ELEMENT_TYPE_PINNED:
    case ELEMENT_TYPE_PTR:
        TypeIsBad = true;
        break;

    default:
        VSFAIL("DecodeCorSigType - unhandled type.");
        TypeIsBad = true;
    }

    if (TypeIsBad)
    {
        return Symbols::GetGenericBadNamedRoot();
    }
    else
    {
        return ptyp;
    }
}

// undone: Microsoft modified for 64 bit alignment problems.
// We have to change this in a better way.

//============================================================================
// Create a value from COM+ data.
//============================================================================

BCSYM_Expression *MetaImport::DecodeValue
(
    DWORD dwTypeFlag, // Type
    const void *pvValue, // Value
    ULONG cbValue,  // Count of bytes except when it's a string, then it the number of characters
    BCSYM *ptyp  // [in] Return type
)
{
    // OPTIONAL: This function is used to decode both field values and optional parameter defaults.

    if (dwTypeFlag == ELEMENT_TYPE_VOID)
    {
        // There is no value, so return nothing.
        return NULL;
    }

    ConstantValue Value;

    // If the type is bad, we cannot reliably extract the value.
    if (ptyp && !ptyp->IsBad())
    {
        switch (dwTypeFlag)
        {
        case ELEMENT_TYPE_BOOLEAN:
            Value.TypeCode = t_bool;
            Value.Integral = *(USE_UNALIGNED __int8 *)pvValue; // Note bools in COM+ are 1 byte
            break;

        case ELEMENT_TYPE_I1:
            Value.TypeCode = t_i1;
            Value.Integral = *(USE_UNALIGNED __int8 *)pvValue;
            break;

        case ELEMENT_TYPE_U1:
            Value.TypeCode = t_ui1;
            Value.Integral = *(USE_UNALIGNED unsigned __int8 *)pvValue;
            break;

        case ELEMENT_TYPE_I2:
            Value.TypeCode = t_i2;
            Value.Integral = *(USE_UNALIGNED __int16 *)pvValue;
            break;

        case ELEMENT_TYPE_U2:
            Value.TypeCode = t_ui2;
            Value.Integral = *(USE_UNALIGNED unsigned __int16 *)pvValue;
            break;

        case ELEMENT_TYPE_I4:
            if (ptyp->IsPointerType() || (!ptyp->IsEnum() && ptyp->GetVtype() == t_ref))
            {
                BCSYM *proot = ptyp->IsPointerType() ? ptyp->PPointerType()->GetCompilerRoot() : ptyp;

                // 



                if (proot->IsIntrinsicType() ||
                    proot->IsEnum() ||
                    // Adding the test for Object here is necessary in order to
                    // make something like "Optional Byref X As Object = 5" work.
                    proot->IsObject() ||
                    (TypeHelpers::IsNullableType(proot) &&
                        TypeHelpers::IsIntrinsicType(TypeHelpers::GetElementTypeOfNullable(proot)))
                   )
                {
                    Value.TypeCode = t_i4;
                    Value.Integral = *(USE_UNALIGNED __int32 *)pvValue;
                }
                else
                {
                    // COM+ encodes pointer constants as I4's
                    // 
                    Value.TypeCode = t_ref;

                    // Microsoft: Back out part of 604868 to address build lab issue #4093
                    // Probably just need to #ifdef this differently for 64bit, but
                    // will let Cameron decide.
                    //Value.Integral = (__int32)*(USE_UNALIGNED void **)pvValue;
                    Value.Integral = (__int64)*(USE_UNALIGNED void **)pvValue;

                    VSASSERT(Value.Integral == 0, "Non-NULL pointer constant?");
                }
            }
            else
            {
                Value.TypeCode = t_i4;
                Value.Integral = *(USE_UNALIGNED __int32 *)pvValue;
            }
            break;

        case ELEMENT_TYPE_U4:
            Value.TypeCode = t_ui4;
            Value.Integral = *(USE_UNALIGNED unsigned __int32 *)pvValue;
            break;

        case ELEMENT_TYPE_I8:
            if (ptyp->GetVtype() == t_date ||
                (ptyp->IsPointerType() && ptyp->PPointerType()->GetCompilerRoot()->GetVtype() == t_date))
            {
                Value.TypeCode = t_date;
            }
            else
            {
                Value.TypeCode = t_i8;
            }
            Value.Integral = *(USE_UNALIGNED __int64 *)pvValue;
            break;

        case ELEMENT_TYPE_U8:
            Value.TypeCode = t_ui8;
            Value.Integral = *(USE_UNALIGNED __int64 *)pvValue;
            break;

        case ELEMENT_TYPE_R4:
            Value.TypeCode = t_single;
            Value.Single = *(USE_UNALIGNED float *)pvValue;
            break;

        case ELEMENT_TYPE_R8:
            Value.TypeCode = t_double;
            Value.Double = *(USE_UNALIGNED double *)pvValue;
            break;

        case ELEMENT_TYPE_CHAR:
            Value.TypeCode = t_char;
            Value.Integral = *(USE_UNALIGNED WCHAR *)pvValue;
            break;

        case ELEMENT_TYPE_STRING:
        {
            Value.TypeCode = t_string;

            // Dev10 #486931
            if (pvValue == NULL )
            {
                cbValue = 0;
            }
            
            // Special Case: cbValue is the number of characters for strings
            Value.String.LengthInCharacters = cbValue;
            if ((Value.String.LengthInCharacters + 1 < 1)  ||
                (!VBMath::TryMultiply((Value.String.LengthInCharacters + 1), sizeof(WCHAR))) ||
                (cbValue >= (Value.String.LengthInCharacters + 1)))
            {
                VSFAIL("Required size exceeds boundaries.");
                break;
            }

            WCHAR * pTmp = (WCHAR *)m_Symbols.GetNorlsAllocator()->Alloc((Value.String.LengthInCharacters + 1) * sizeof(WCHAR));

            if ( cbValue )
            {
                memcpy(pTmp, (WCHAR *)pvValue, cbValue * sizeof(WCHAR)); // Dev10 #486931 - Do not read more than cbValue characters.
            }
            
            pTmp[cbValue] = '\0';
            Value.String.Spelling = pTmp;
            break;
        }
        case ELEMENT_TYPE_OBJECT:
            // Do we need to handle this because of the new short-form signatures?
            // Or can we just fall-through into the ELEMENT_TYPE_CLASS case?
            VSFAIL("What do we do with ELEMENT_TYPE_OBJECT?");
            break;
            // Fall through

        case ELEMENT_TYPE_CLASS:
            // Default value for type class can only be NULL - i.e., an encoding of VB's "Nothing".
            // Importing default param value defined as "Object = Nothing".
            // ConstantValue doesn't have a good union member to express this, so just see that
            // the imported value and the stored value are all zeros.
            // Note that we can't use "IDispatch *" here, as that is a 64-bit value on
            // Win64, so we instead hard-code this as a 32-bit assert.
            // Hack-A-Rama: Ran into a problem importing sub foo(optional x as string = nothing)
            // We need to know that the 'nothing' should be typed as a string so you'd think we would emit a string as nothing in the first place but
            // apparently although there is a way to represent a NULL string and an Empty string "", there doesn't appear to be a way to represent a
            // NOTHING string.  So we do this instead on import.  See bug VS7#302609 for what led us to do this.

            // Strip away the ByRef to get to the root type (see bug VS7#469628).
            ptyp = ptyp->IsPointerType() ? ptyp->PPointerType()->GetCompilerRoot() : ptyp;

            Value.TypeCode = ptyp->GetVtype() == t_string ?
                                t_string :                      // Hack-a-rama.  The problem is that optional x as string = nothing
                                ptyp->GetVtype() == t_array ?
                                    t_array :                   // Problem with optional x() as Integer = Nothing. Bug VSWhidbey 495611.
                                    t_ref;
            VSASSERT(*(USE_UNALIGNED __int32 *)pvValue == 0, "Can only handle NULL case in import.");
            VSASSERT(Value.Integral == 0, "ConstantValue's CTOR should init to zero.");
            break;

        default:
            VSFAIL("unhandled case");
            break;
        }
    }

    return m_Symbols.GetFixedExpression(&Value);
}

void MetaImport::TransferBadSymbol(_Inout_ BCSYM_NamedRoot *Destination, _In_ BCSYM_NamedRoot *Source)
{
    //DDB # 189265
    //We don't want to transfer badness if the symbol is already marked as bad
    //(otherwise we assert).
    if (! Destination->IsBadNamedRoot())
    {
        Destination->SetIsBadRaw(Source->IsBadNamedRoot());
        Destination->SetStateForBadness(Source->GetStateForBadness());
        Destination->SetErrid(Source->GetErrid());
        Destination->SetBadName(Source->GetBadName());
        Destination->SetBadNameSpace(Source->GetBadNameSpace());
        Destination->SetBadExtra(Source->GetBadExtra());

        // #Bug 131435 - DevDiv Bugs
        // If error number isn't provided and the Source of error is not bindable,
        // mark the current symbol as not bindable
        if(!Destination->GetErrid() && Source->GetBindingSpace() == BINDSPACE_IgnoreSymbol)
        {
            Destination->SetBindingSpace(BINDSPACE_IgnoreSymbol);
        }
    }
}

bool MetaImport::HasDiamondReferenceError(BCSYM *pType)
{
    return
        pType &&
        pType->IsNamedRoot() &&
        pType->PNamedRoot()->IsBad() &&
        pType->PNamedRoot()->GetErrid() == ERRID_TypeRefFromMetadataToVBUndef;
}

ConstantValue CoerceFloatingPointValue(double SourceValue, Vtypes vtTargetType, _Inout_ bool &Overflow)
{
    ConstantValue Value;

    if (IsBooleanType(vtTargetType))
    {
        Value.Integral = SourceValue == 0.0 ?
            0 :
            1;
    }
    else if (IsIntegralType(vtTargetType) || IsCharType(vtTargetType))
    {
        Overflow =
            DetectFloatingToIntegralOverflow(
                SourceValue,
                IsUnsignedType(vtTargetType));

        if (!Overflow)
        {
            Quadword IntegralValue;
            double Temporary;
            double Floor;
            Vtypes vtSourceIntegralType;

            // VB has different rounding behavior than C by default. Ensure we
            // are using the right type of rounding

            Temporary = SourceValue + 0.5;

            // We had a number that was equally close to 2 integers.
            // We need to return the even one.
            Floor = floor(Temporary);

            if (Floor != Temporary || fmod(Temporary, 2.0) == 0)
            {
                IntegralValue = IsUnsignedLongType(vtTargetType) ? ConvertFloatingToUI64(Floor) : (Quadword)Floor;
            }
            else
            {
                IntegralValue = IsUnsignedLongType(vtTargetType) ? ConvertFloatingToUI64(Floor - 1.0) : (Quadword)(Floor - 1.0);
            }

            if (SourceValue < 0)
            {
                vtSourceIntegralType = t_i8;
            }
            else
            {
                vtSourceIntegralType = t_ui8;
            }

            Value.Integral = NarrowIntegralResult(IntegralValue, vtSourceIntegralType, vtTargetType, Overflow);
        }
    }
    else if (vtTargetType == t_single)
    {
        Value.Single = (float) NarrowFloatingResult(SourceValue, vtTargetType, Overflow);
    }
    else if (vtTargetType == t_double)
    {
        Value.Double = SourceValue;
    }

    Value.SetVType(vtTargetType);

    return Value;
}

void MetaImport::CoerceEnumeratorValues(Vtypes vtUnderlyingType, _In_z_ STRING *strEnumName)
{

    unsigned iMemberIndex = 0;

    for (iMemberIndex = 0; iMemberIndex < m_cMembers; iMemberIndex++)
    {
        BCSYM_NamedRoot *pMember = m_rgmembers[iMemberIndex].m_pnamed;

        if (pMember && pMember->IsVariable())
        {
            if (pMember->IsVariableWithValue())
            {
                ConstantValue Value =  pMember->PVariableWithValue()->GetExpression()->GetValue();

                if (vtUnderlyingType != Value.TypeCode)
                {
                    bool Overflow = false;

                    switch (Value.TypeCode)
                    {
                        case t_bool:
                        case t_i1:
                        case t_ui1:
                        case t_i2:
                        case t_ui2:
                        case t_i4:
                        case t_ui4:
                        case t_i8:
                        case t_ui8:
                        case t_char:
                            if (IsIntegralType(vtUnderlyingType) || IsBooleanType(vtUnderlyingType) || IsCharType(vtUnderlyingType))
                            {
                                Value.Integral = NarrowIntegralResult(Value.Integral, Value.TypeCode, vtUnderlyingType, Overflow);
                            }
                            else if (vtUnderlyingType == t_single)
                            {
                                Value.Single = IsUnsignedType(Value.TypeCode) ?
                                    (float)(unsigned __int64)Value.Integral :
                                    (float)Value.Integral;
                            }
                            else if (vtUnderlyingType == t_double)
                            {
                                Value.Double = IsUnsignedType(Value.TypeCode) ?
                                    (float)(unsigned __int64)Value.Integral :
                                    (float)Value.Integral;
                            }
                            break;

                        case t_single:
                            Value = CoerceFloatingPointValue(Value.Single, vtUnderlyingType, Overflow);
                            break;

                        case t_double:
                            Value = CoerceFloatingPointValue(Value.Double, vtUnderlyingType, Overflow);
                            break;
                    }

                    if (!Overflow)
                    {
                        Value.SetVType(vtUnderlyingType);
                        pMember->PVariableWithValue()->GetExpression()->SetValue(Value);
                    }
                    else
                    {
                        pMember->SetIsBad();
                        pMember->SetErrid(ERRID_ExpressionOverflow1);
                        pMember->SetBadExtra(strEnumName);
                    }
                }
            }
        }
        else
        {
            VSFAIL("Why do we have a non-variable type");
        }
    }
}

void 
MetaImport::DoUpdateImportedTypeChildren( _In_ BCSYM_Container* pContainer )
{
    WCHAR *pwszDefaultMember;

    m_pcontainer = pContainer;
    m_td = pContainer->GetTypeDef();

#if FV_TRACK_MEMORY
    DebPrintf("Loading children for '%S.%S' in '%S'.\n", m_pcontainer->GetNameSpace(), m_pcontainer->GetName(), m_pcontainer->GetContainingProject()->GetAssemblyName());
#endif

    // Remember that we're loading to prevent nasty cycles.
    Symbols::SetChildrenLoading(pContainer, true);

    m_pcontainer->GetPWellKnownAttrVals()->GetDefaultMemberData(&pwszDefaultMember);

    // Load any new children of the type
    LoadNewChildren();

    // Load the properties, hooking them up with their implementing methods.
    SymbolList symlist;
    LoadNewProperties(&symlist, pwszDefaultMember);

    // Load the events.
    LoadNewEvents(&symlist);

    // Finish loading the children.
    FinishNewChildren(&symlist);

    // We're no longer loading.  Don't bother resetting this if we get an
    // error, we're already horked.
    //
    Symbols::SetChildrenLoading(m_pcontainer, false);
}

//-------------------------------------------------------------------------------------------------
//
// Load any new children on the type
//
//-------------------------------------------------------------------------------------------------
void 
MetaImport::LoadNewChildren()
{
    CorEnum hEnum(m_pmdImport);

    unsigned long cMembers;
    unsigned long iMember, iMemberWrite;
    bool fIsFirst = true;

    //
    // Figure out how many members there are.
    //

    // Open the enum.
    IfFailThrow(m_pmdImport->EnumMembers(&hEnum,
                                         m_td,
                                         NULL,
                                         0,
                                         NULL));

    // Get the count.
    IfFailThrow(m_pmdImport->CountEnum(hEnum, &cMembers));

    if (cMembers == 0)
    {
        m_cMembers = 0;
        return;
    }

    m_rgmembers = (SavedMetaMember *)m_nraScratch.Alloc(VBMath::Multiply(
        cMembers, 
        sizeof(SavedMetaMember)));

    for (iMember = iMemberWrite = 0; iMember < cMembers; iMember++)
    {
        // Get the token.
        unsigned long foundCount;
        mdToken tk;
        IfFailThrow(m_pmdImport->EnumMembers(&hEnum,
                                             m_td,
                                             &tk,
                                             1,
                                             &foundCount));
        // Are we done?
        if (!foundCount)
        {
            break;
        }

        // Create the appropriate symbol.
        MetaMember mm;
        if ( !GetMemberProps(tk, &mm) )
        {
            continue;
        }

        // Only load this if it is a new member
        if ( m_pcontainer->GetHash()->SimpleBind(mm.m_pstrName) )
        {
            continue;
        }

        if (TypeFromToken(tk) == mdtMethodDef)
        {
            m_rgmembers[iMemberWrite].m_tk = mm.m_tk;
            m_rgmembers[iMemberWrite].m_pnamed = LoadMethodDef(&mm, false);
        }
        else
        {
            VSASSERT(TypeFromToken(tk) == mdtFieldDef, "What else could this be?");

            Vtypes unused;
            m_rgmembers[iMemberWrite].m_tk = mm.m_tk;
            m_rgmembers[iMemberWrite].m_pnamed = LoadFieldDef(&mm, &unused, false);
        }

         // Next element.
        iMemberWrite++;
    }

    // Remember how many we actually have.
    m_cMembers = iMemberWrite;
}

//-------------------------------------------------------------------------------------------------
//
// Load any new properties defined on the type
//
//-------------------------------------------------------------------------------------------------
void 
MetaImport::LoadNewProperties( _Inout_ SymbolList *pSymList, _In_opt_z_ WCHAR *pwszDefaultProperty)
{
    CorEnum hEnum(m_pmdImport);
    WCHAR nameBuffer[256];

    for (;;)
    {
        unsigned long actualNameCch;
        unsigned long found;
        mdProperty propToken;
        mdTypeDef containerToken;
        mdMethodDef getToken, setToken;
        mdMethodDef cheatToken = mdMethodDefNil;
        Procedure* pSet = NULL;
        Procedure* pGet = NULL;
        Type* pReturnType;
        Declaration* pBadTypeSymbol = NULL;
        StringPoolEntry name;
        Parameter* pFirstParam = NULL;
        DECLFLAGS flags = 0;

        IfFailThrow(m_pmdImport->EnumProperties(&hEnum,
                                                m_td,
                                                &propToken,
                                                1,
                                                &found) );
        if ( !found )
        {
            break;
        }

        IfFailThrow( m_pmdImport->GetPropertyProps(
            propToken,
            &containerToken,
            nameBuffer,
            _countof(nameBuffer),
            &actualNameCch,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            &setToken,
            &getToken,
            NULL,
            0,
            NULL) );

        VSASSERT(containerToken == m_td, "Why isn't this in the right class?");

        // Only process new members
        name = m_pCompiler->AddStringPoolEntry(nameBuffer);
        if ( m_pcontainer->GetHash()->SimpleBind(name.GetDataNonConst()))
        {
            continue;
        }

        if ( mdMethodDefNil != getToken )
        {
            Declaration* pDecl = GetMemberOfToken(getToken);
            if ( pDecl && pDecl->IsProc() )
            {
                pGet = pDecl->PProc();
                Symbols::SetIsPropertyGet(pGet);
                flags = pGet->GetDeclFlags();
            }
        }

        if ( mdMethodDefNil != setToken )
        {
            Declaration* pDecl = GetMemberOfToken(setToken);
            if ( pDecl && pDecl->IsProc() )
            {
                pSet = pDecl->PProc();
                Symbols::SetIsPropertySet(pSet);
                flags = pSet->GetDeclFlags();
            }
        }

        // Must have at least a get or set
        if ( !pSet & !pGet )
        {
            continue;
        }

        // Get the signature
        PCCOR_SIGNATURE signature;
        ULONG signatureSize;

        // Get all of the interesting information.
        IfFailThrow(m_pmdImport->GetPropertyProps(propToken,  // [IN] property token
                                                  NULL,           // [OUT] typedef containing the property declarion.
                                                  NULL,           // [OUT] Property name
                                                  0,     // [IN] the count of wchar of szProperty
                                                  NULL,   // [OUT] actual count of wchar for property name
                                                  NULL,           // [OUT] property flags -- none we care about
                                                  &signature,     // [OUT] property type. pointing to meta data internal blob
                                                  &signatureSize, // [OUT] count of bytes in *ppvSig
                                                  NULL,           // [OUT] flag for value type. selected ELEMENT_TYPE_*
                                                  NULL,           // [OUT] constant value
                                                  NULL,           // [OUT] size of constant value in bytes
                                                  NULL,           // [OUT] setter method of the property
                                                  NULL,           // [OUT] getter method of the property
                                                  NULL,           // [OUT] other method of the property
                                                  0,              // [IN] size of rmdOtherMethod
                                                  NULL));         // [OUT] total number of other method of this property
        // Decode the signature.
        //
        // Unfortunately, we have to cheat here. There are no propertydefs hung off
        // of the property, so there's no way for us to find out useful things like
        // optional parameters and paramarrays for the property. So we pass in the
        // property signature here, but one of the accessor's methoddefs and use
        // the values off of them. Sleazy.
        bool isBad = false;
        pReturnType =
            DecodeMethodSignature(
                cheatToken,
                signature,
                signatureSize,
                true,
                isBad,
                &pFirstParam,
                NULL,
                NULL,
                &pBadTypeSymbol);

        // We don't allow you to deal with functions that return pointers
        if ( (pReturnType && pReturnType->IsPointerType()) || pBadTypeSymbol || isBad)
        {
            continue;
        }

        flags &= ~(DECLF_PropGet | DECLF_PropSet | DECLF_SpecialName);
        flags |= DECLF_HasRetval | DECLF_Function;

        if (pGet && pSet)
        {
            // VB can have accessors of different accessibility levels. The accessor level can be only
            // more restrictive. Only one accessor can be more restrictive
            // Take the less restrictive one.
            flags &= ~DECLF_AccessFlags;
            if (Symbols::AccessOfFlags(pSet->GetDeclFlags()) < Symbols::AccessOfFlags(pGet->GetDeclFlags()))
            {
                flags |= pGet->GetDeclFlags() & DECLF_AccessFlags;
            }
            else
            {
                flags |= pSet->GetDeclFlags() & DECLF_AccessFlags;
            }
        }

        Declaration* pProp  = m_Symbols.AllocProperty(false);

        m_Symbols.GetProc(NULL,
                          name.GetDataNonConst(),
                          name.GetDataNonConst(),
                          (CodeBlockLocation*) NULL,
                          (CodeBlockLocation*) NULL,
                          flags,
                          pReturnType,
                          pFirstParam,
                          NULL,
                          NULL,
                          NULL,
                          SYNTH_None,
                          NULL,
                          NULL,
                          pProp->PProperty());
        Symbols::SetToken(pProp, propToken);

        DECLFLAGS PropertyBlockFlags = 0;
        if ( pGet && !pSet)
        {
            PropertyBlockFlags = DECLF_ReadOnly;
        }
        else if ( pSet && !pGet)
        {
            PropertyBlockFlags = DECLF_WriteOnly;
        }

        m_Symbols.GetProperty(NULL,
                              name.GetDataNonConst(),
                              PropertyBlockFlags,
                              pGet,
                              pSet,
                              pProp->PProperty(),
                              pSymList);
        Symbols::MakeUnbindable(pGet);
        Symbols::MakeUnbindable(pSet);

        Attribute::----AllAttributesOnToken(propToken,
                                             m_pmdImport,
                                             m_Symbols.GetNorlsAllocator(),
                                             m_pCompiler,
                                             m_pMetaDataFile->GetProject(),
                                             m_pMetaDataFile,
                                             pProp);
    }
}

void 
MetaImport::LoadNewEvents( _Inout_ SymbolList *psymlist)
{

}

void 
MetaImport::FinishNewChildren( _Inout_ SymbolList* pSymList)
{
    for ( unsigned i = 0; i < m_cMembers; i++ )
    {
        Declaration* pCurrent = m_rgmembers[i].m_pnamed;
        pSymList->AddToFront(pCurrent);
    }

    Symbols::AddSymbolListToHash(m_pcontainer->GetHash(), pSymList, true);
}

/*****************************************************************************
;CheckWhetherToImportInaccessibleMembers

Determine whether we need to import private types/members for this 
metadatafile.  If this a PIA we need to import everything.
*****************************************************************************/
void MetaImport::CheckWhetherToImportInaccessibleMembers()
{
    VSASSERT( m_pMetaDataFile->GetAttributeTokenHash() != NULL, "We need the attribute token hash to be setup by now.");

    CompilerProject *pProject = m_pMetaDataFile->GetProject();

    /* If we haven't checked the project yet then we need to run through all of the metadatafiles in here since
    modules complicate things because if one of them is a PIA we need to import private metadata for this
    project.  We need to look at all of the metadatafiles in the project as a whole before we 
    can know whether we can skip loading friend/private metadata for *this* metadatafile. */
    if ( !pProject->CheckedAllMetadataFilesForAccessibility())
    {
        TriState<bool> SomeMetadataFileHasPiaAttribute;
        MetaDataFileIterator Files(pProject);

        while (MetaDataFile *pFile = Files.Next())
        {
            SomeMetadataFileHasPiaAttribute = pFile->IsPIA();
            if ( !SomeMetadataFileHasPiaAttribute.HasValue() || SomeMetadataFileHasPiaAttribute.GetValue() == true)
            {
                // We know enough at this point to stop looking for the PIA attribute because either we know
                // that we have a PIA assembly so we'll have to load private metadata anyway, or we have an
                // assembly for which we couldn't determine the answer so the safe default is to load everything.
                break; 
            }
        }

        // If we weren't able to query for the PIA attribute (can happen in remote debugging or device debugger scenarios) HasVAlue() 
        // will false, in which case assume that we should import inaccessible members since we can't make an informed decision about
        // whether this is a PIA assembly or not.  Choose the safe default to import everything in that case.
        if ( !SomeMetadataFileHasPiaAttribute.HasValue() || SomeMetadataFileHasPiaAttribute.GetValue() == true )
        {
            pProject->SetShouldImportPrivates( true );
        }
        else
        {
            pProject->SetShouldImportPrivates( false );
        }
        pProject->SetCheckedAllMetadataFilesForAccessibility( true );
    }
}

// Add fake members to the type which implement some winrt interfaces whose
// members don't show up as members of the type itself.
void MetaImport::AddFakeWindowsRuntimeInterfaceMembers(
    _Inout_ SymbolList *pSymList,
    _Inout_ SymbolList *pUnbindableSymList
)
{
    STRINGTree interfaceNames(m_Symbols.GetNorlsAllocator());
    STRINGTree clashingNames(m_Symbols.GetNorlsAllocator());
    DynamicArray<BCSYM *> interfacesToImplement;

    // Figure out what interfaces we need to implement.
    GetInterfacesToImplement(m_pcontainer, interfaceNames, interfacesToImplement);

    // Get all the members that will clash based on signature.
    GetClashingMembers(interfacesToImplement, clashingNames, pSymList);

    // Add interface members which don't clash.
    for (ULONG i=0; i < interfacesToImplement.Count(); i++)
    {
        AddInterfaceMembers(interfacesToImplement.Element(i), clashingNames, pSymList, pUnbindableSymList);
    }
}

// Enumerate through all base interfaces and get interfaces we want to implement.
void MetaImport::GetInterfacesToImplement(
    _In_ BCSYM *pContainer,
    STRINGTree &interfaceNames,
    DynamicArray<BCSYM *> &interfacesToImplement
)
{
    BCSYM_Implements *pImplements = pContainer->GetFirstImplements();
    while (pImplements)
    {
        if (!pImplements->IsBad() && !pImplements->GetRoot()->DigThroughAlias()->IsBad())
        {
            BCSYM *pInterface = pImplements->GetRoot()->DigThroughAlias();

            if (ShouldImplementInterface(pInterface))
            {
                pInterface = ReplaceGenericParametersWithArguments( pInterface, 
                                                                    pContainer->IsGenericBinding()? pContainer->PGenericBinding() : NULL, 
                                                                    m_Symbols);

                STRING *pName = pInterface->GetGlobalQualifiedName(pInterface->IsGenericBinding()? pInterface->PGenericBinding(): NULL);
                if (interfaceNames.Insert(&pName))
                {
                    interfacesToImplement.AddElement(pInterface);
                }
            }

            GetInterfacesToImplement(pInterface, interfaceNames, interfacesToImplement);
        }
        pImplements = pImplements->GetNext();
    }
}

// If the interface happens to be a WinRT Collection interface (that will map to 
// some .NET collection types) then we want to fake up it's members on to the type.
bool MetaImport::ShouldImplementInterface(
    _In_ BCSYM *pInterface)
{

    if (pInterface->IsInterface() &&
        (StringPool::IsEqual(CONCAT_STRING_CONST(m_pCompiler, ComGenericCollectionsDomain, ComIList), pInterface->PNamedRoot()->GetQualifiedName()) ||
         StringPool::IsEqual(CONCAT_STRING_CONST(m_pCompiler, ComGenericCollectionsDomain, ComICollection), pInterface->PNamedRoot()->GetQualifiedName()) ||
         StringPool::IsEqual(CONCAT_STRING_CONST(m_pCompiler, ComGenericCollectionsDomain, ComIDictionary), pInterface->PNamedRoot()->GetQualifiedName()) ||
         StringPool::IsEqual(CONCAT_STRING_CONST(m_pCompiler, ComGenericCollectionsDomain, ComIEnumerable), pInterface->PNamedRoot()->GetQualifiedName()) ||
         StringPool::IsEqual(CONCAT_STRING_CONST(m_pCompiler, ComGenericCollectionsDomain, ComIReadOnlyList), pInterface->PNamedRoot()->GetQualifiedName()) ||
         StringPool::IsEqual(CONCAT_STRING_CONST(m_pCompiler, ComGenericCollectionsDomain, ComIReadOnlyCollection), pInterface->PNamedRoot()->GetQualifiedName()) ||
         StringPool::IsEqual(CONCAT_STRING_CONST(m_pCompiler, ComGenericCollectionsDomain, ComIReadOnlyDictionary), pInterface->PNamedRoot()->GetQualifiedName()) ||
         StringPool::IsEqual(CONCAT_STRING_CONST(m_pCompiler, ComCollectionsDomain, ComIEnumerable), pInterface->PNamedRoot()->GetQualifiedName()) || 
         StringPool::IsEqual(CONCAT_STRING_CONST(m_pCompiler, ComCollectionsDomain, ComIList), pInterface->PNamedRoot()->GetQualifiedName()) ||
         StringPool::IsEqual(CONCAT_STRING_CONST(m_pCompiler, ComCollectionsDomain, ComICollection), pInterface->PNamedRoot()->GetQualifiedName()) ||
         StringPool::IsEqual(CONCAT_STRING_CONST(m_pCompiler, ComSpecializedCollectionsDomain, ComINotifyCollectionChanged), pInterface->PNamedRoot()->GetQualifiedName()) ||
         StringPool::IsEqual(CONCAT_STRING_CONST(m_pCompiler, ComComponentModelDomain, ComINotifyPropertyChanged), pInterface->PNamedRoot()->GetQualifiedName()) 
         ))
    {
        return true;
    }
    return false;
}

// Signarute is a dtring like Add(Integer, String,)
STRING* MetaImport::GetSignature(
    _In_ BCSYM_Proc *pProc,
    _In_ BCSYM_GenericBinding *pGenericBinding)
{
    StringBuffer signature;
    signature.AppendSTRING(pProc->GetName());
    signature.AppendChar(L'(');

    BCITER_Parameters paramIter(pProc);
    while (BCSYM_Param *pParam = paramIter.PparamNext())
    {
        BCSYM *pParamType = ReplaceGenericParametersWithArguments( pParam->GetType(), pGenericBinding, m_Symbols);
        pParamType->GetBasicRep(m_pCompiler, NULL, &signature, pGenericBinding, NULL, false);
        signature.AppendChar(L',');
    }
    signature.AppendChar(L')');
    return m_pCompiler->AddString(signature.GetString());
}

// This function detects if there are any clashes amongst all the methods in this immediate class (stored in m_rgmembers) and 
// events/properties (passed in pSymList, the list of bindable symbols). It adds its output to parameter "clashingMembers".
// A clash is when any method/event/property has the same NAME and FORMAL PARAMETER TYPES as another method/event/property.
// We use a handy shortcut for checking signatures: instead of checking them directly, we use GetSignature() to 
// generate a string, and check the string. This string has the form e.g.
//    methodname(Integer)
//    propertyname(Integer)
//    eventname(Integer)
void MetaImport::GetClashingMembers( 
    DynamicArray<BCSYM *> &interfacesToImplement,
    STRINGTree &clashingMembers,
    SymbolList *pSymList)
{
    STRINGTree allMemberNames(m_Symbols.GetNorlsAllocator());

    // Search the current type's methods for clashes.
    for (unsigned long iMember = 0; iMember < m_cMembers; iMember++)
    {
        BCSYM_NamedRoot *child = m_rgmembers[iMember].m_pnamed;

        if (child->IsProc())
        {
            STRING *pName = GetSignature(child->PProc(), m_pcontainer->IsGenericBinding()? m_pcontainer->PGenericBinding() : NULL);

            if (!allMemberNames.Insert(&pName))
            {
                clashingMembers.Insert(&pName);
            }
        }
    }
    
    // The properties and events of m_pContainer are stored in pSymList and not in m_rgMembers. 
    // Search the current type's properties and events for clashes.
    BCSYM_NamedRoot * pNext = pSymList->GetFirst();
    while(pNext)
    {
        if (pNext->IsProc())
        {
            STRING *pName = GetSignature(pNext->PProc(), m_pcontainer->IsGenericBinding()? m_pcontainer->PGenericBinding() : NULL);

            if (!allMemberNames.Insert(&pName))
            {
                clashingMembers.Insert(&pName);
            }
        }
        pNext = pNext->GetNextInSymbolList();
    }

    // Search the interfaces to be implemented for clashes.
    for (ULONG i=0; i < interfacesToImplement.Count(); i++)
    {
        BCSYM *pInterface = interfacesToImplement.Element(i);
        BCITER_CHILD childIterator(pInterface->PContainer());

        for (BCSYM_NamedRoot *child = childIterator.GetNext();
                child;
                child = childIterator.GetNext())
        {
            STRING *pName = GetSignature(child->PProc(), pInterface->IsGenericBinding()? pInterface->PGenericBinding() : NULL);

            if (!allMemberNames.Insert(&pName))
            {
                clashingMembers.Insert(&pName);
            }
        }
    }
}

// Enumerate through the interface's members and duplicate it's methods and properties
void MetaImport::AddInterfaceMembers(
    _In_ BCSYM *pInterface,
    STRINGTree &clashingMembers,
    _Inout_ SymbolList *pSymList,
    _Inout_ SymbolList *pUnbindableSymList
)
{
    BCITER_CHILD childIterator(pInterface->PContainer());

    for (BCSYM_NamedRoot *child = childIterator.GetNext();
            child;
            child = childIterator.GetNext())
    {
        if (child->IsProc())
        {
            STRING *pName = GetSignature(child->PProc(), pInterface->IsGenericBinding()? pInterface->PGenericBinding() : NULL);

            if (clashingMembers.Insert(&pName))
            {
                if (child->IsEventDecl())
                {
                    BCSYM_EventDecl *pEventSym = m_Symbols.AllocEventDecl(false);
                    pEventSym->SetIsFakeWindowsRuntimeMember(true);
                    CopyEventSymbol(child->PEventDecl(), pInterface, pEventSym, pSymList, pUnbindableSymList);
                }
                else if (child->IsMethodDecl())
                {
                    BCSYM_MethodDecl *pMethodSym = m_Symbols.AllocMethodDecl(false);
                    pMethodSym->SetIsFakeWindowsRuntimeMember(true);
                    CopyMethodSymbol(child->PProc(), pInterface, pMethodSym, pSymList);
                }
                else if (child->IsProperty())
                {
                    BCSYM_Property *pPropSym = m_Symbols.AllocProperty(false);
                    pPropSym->SetIsFakeWindowsRuntimeMember(true);
                    CopyPropertySymbol(child->PProperty(), pInterface, pPropSym, pSymList, pUnbindableSymList);
                }
            }
        }
    }

}

// Fake up a method symbol
void MetaImport::CopyMethodSymbol(
    _In_ BCSYM_Proc *pOrigMethod,
    _In_ BCSYM *pContainer,
    _Inout_ BCSYM_Proc *pNewMethod,
    _Inout_ SymbolList *pSymList
)
{
    BCSYM_Param *pParamFirst = NULL;
    BCSYM_Param *pParamLast = NULL;

    if (!pOrigMethod)
        return;

    BCITER_Parameters paramIter(pOrigMethod);
    while (BCSYM_Param *pSourceParam = paramIter.PparamNext())
    {
        BCSYM *pParamType = ReplaceGenericParametersWithArguments( pSourceParam->GetType(),
                                                                   pContainer->IsGenericBinding()? pContainer->PGenericBinding() : NULL, 
                                                                   m_Symbols);
                    
        m_Symbols.GetParam(pSourceParam->GetLocation(),
                            pSourceParam->GetName(),
                            pParamType,
                            0,
                            NULL,
                            &pParamFirst,
                            &pParamLast,
                            pSourceParam->IsReturnType());

    }

    BCSYM *pReturnType = ReplaceGenericParametersWithArguments( pOrigMethod->GetType(),
                                                                pContainer->IsGenericBinding()? pContainer->PGenericBinding() : NULL, 
                                                                m_Symbols);

    m_Symbols.GetProc(
                    pOrigMethod->GetLocation(),
                    pOrigMethod->GetName(),
                    pOrigMethod->GetEmittedName(),
                    NULL,
                    NULL,
                    pOrigMethod->GetDeclFlags(),
                    pReturnType,
                    pParamFirst,
                    NULL,
                    NULL,
                    NULL,
                    SYNTH_None,
                    pOrigMethod->GetFirstGenericParam(),
                    pSymList,
                    pNewMethod);

    BCSYM_ImplementsList *pImplList = m_Symbols.GetImplementsList(NULL, m_pcontainer, pNewMethod->GetName());
    pImplList->SetImplementedMember(pOrigMethod, pContainer->IsGenericBinding() ? pContainer->PGenericBinding() : NULL);
    pNewMethod->SetImplementsList(pImplList);
}

// Fake up a property
void MetaImport::CopyPropertySymbol(
    _In_ BCSYM_Property *pOrigProp,
    _In_ BCSYM *pContainer,
    _Inout_ BCSYM_Property *pNewProp,
    _Inout_ SymbolList *pSymList,
    _Inout_ SymbolList *pUnbindableSymList
)
{
    // We don't want the setters and getters to be bindable. So add them to the unbindable symlist
    // so that they get inserted into the unbindable hash of the container.
    BCSYM_MethodDecl *pGetMethod = m_Symbols.AllocMethodDecl(false);
    pGetMethod->SetIsFakeWindowsRuntimeMember(true);
    CopyMethodSymbol(pOrigProp->GetProperty(), pContainer, pGetMethod, pUnbindableSymList); 
    Symbols::SetIsPropertyGet(pGetMethod);
    Symbols::MakeUnbindable(pGetMethod);
            
    BCSYM_MethodDecl *pSetMethod = m_Symbols.AllocMethodDecl(false);
    pSetMethod->SetIsFakeWindowsRuntimeMember(true);
    CopyMethodSymbol(pOrigProp->SetProperty(), pContainer, pSetMethod, pUnbindableSymList); 
    Symbols::SetIsPropertySet(pSetMethod);
    Symbols::MakeUnbindable(pSetMethod);

    // A property has a proc of it's own as well. Copy that.
    CopyMethodSymbol(pOrigProp, pContainer, pNewProp, NULL);

    m_Symbols.GetProperty(
                        pOrigProp->GetLocation(),
                        pOrigProp->GetName(),
                        pOrigProp->GetDeclFlags(),
                        pGetMethod,
                        pSetMethod,
                        pNewProp,
                        pSymList);

    // If we have a default property let the container know.
    if (pNewProp->IsDefault() && pContainer->IsContainer())
    {
        m_pcontainer->SetDefaultProperty(pNewProp);
    }
}

// Fake up an Event
void MetaImport::CopyEventSymbol(
    _In_ BCSYM_EventDecl *pOrigEvent,
    _In_ BCSYM *pContainer,
    _Inout_ BCSYM_EventDecl *pNewEvent,
    _Inout_ SymbolList *pSymList,
    _Inout_ SymbolList *pUnbindableSymList
)
{
    CopyMethodSymbol(pOrigEvent, pContainer, pNewEvent, pSymList);
    pNewEvent->SetDelegate(pOrigEvent->GetDelegate());

    // We don't want the add and remove methods to be bindable. So add them to the unbindable symlist
    // so that they get inserted into the unbindable hash of the container.
    BCSYM_MethodDecl *pAddMethod = NULL;
    if (pOrigEvent->GetProcAdd())
    {
        pAddMethod = m_Symbols.AllocMethodDecl(false);
        pAddMethod->SetIsFakeWindowsRuntimeMember(true);
        CopyMethodSymbol(pOrigEvent->GetProcAdd(), pContainer, pAddMethod, pUnbindableSymList); 
        Symbols::MakeUnbindable(pAddMethod);
        pNewEvent->SetProcAdd(pAddMethod);
        pAddMethod->SetEventThatCreatedSymbol(pNewEvent);
    }

    BCSYM_MethodDecl *pRemoveMethod = NULL;
    if (pOrigEvent->GetProcRemove())
    {
        pRemoveMethod = m_Symbols.AllocMethodDecl(false);
        pRemoveMethod->SetIsFakeWindowsRuntimeMember(true);
        CopyMethodSymbol(pOrigEvent->GetProcRemove(), pContainer, pRemoveMethod, pUnbindableSymList); 
        Symbols::MakeUnbindable(pRemoveMethod);
        pNewEvent->SetProcRemove(pRemoveMethod);
        pRemoveMethod->SetEventThatCreatedSymbol(pNewEvent);
    }
    
    BCSYM_MethodDecl *pFireMethod = NULL;
    if (pOrigEvent->GetProcFire())
    {
        pFireMethod = m_Symbols.AllocMethodDecl(false);
        pFireMethod->SetIsFakeWindowsRuntimeMember(true);
        CopyMethodSymbol(pOrigEvent->GetProcFire(), pContainer, pFireMethod, pUnbindableSymList); 
        Symbols::MakeUnbindable(pFireMethod);
        pNewEvent->SetProcFire(pFireMethod);
        pFireMethod->SetEventThatCreatedSymbol(pNewEvent);
    }
}

BCSYM*
MetaImport::GetWindowsRuntimeTypeByName
(
    _In_z_ STRING *pstrTypeName,
    _In_z_ STRING *pstrNameSpace,
    unsigned cTypeArity
)
{
    BCSYM* pType = NULL;

    ProjectIterator iter(m_pCompilerHost);
    while (CompilerProject* pProject = iter.Next())
    {
        if (!pProject->IsMetaData() ||
            !pProject->GetAssemblyIdentity()->IsWindowsRuntimeAssembly())
        {
            continue;
        }

        BCSYM *pTypeInProject = GetTypeByNameInProject(pstrTypeName,
                                                       pstrNameSpace,
                                                       cTypeArity,
                                                       pProject);

        if (pTypeInProject != NULL &&
            !pTypeInProject->IsBad())
        {
            pType = pTypeInProject;
            break;
        }
    }

    return pType;
}
