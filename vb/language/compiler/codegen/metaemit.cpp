//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Manage emitting metadata.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

// VB Defines IfFailGo's label as 'Error' whereas csharp and alink define it as 'ErrExit'. While importing the
// implementation of the helper switch to that definition and restore it after that.
#undef IfFailGo
#define IfFailGo(EXPR) IfFailGoto(EXPR, ErrExit)
#include "MetaEmitHelper.cpp"
#undef IfFailGo
#define IfFailGo(EXPR) IfFailGoto(EXPR, Error)

static const DWORD rgPEKind[] =
{
    peILonly,                   // platformAgnostic
    peILonly | pe32BitRequired, // platformX86
    peILonly | pe32Plus,        // platformIA64
    peILonly | pe32Plus,        // platformAMD64
    peILonly,                   // platformARM
    peILonly | pe32BitPreferred,// platform32BitPerferred     
};

static const DWORD rgMachine[] =
{
    IMAGE_FILE_MACHINE_I386,    // platformAgnostic
    IMAGE_FILE_MACHINE_I386,    // platformX86
    IMAGE_FILE_MACHINE_IA64,    // platformIA64
    IMAGE_FILE_MACHINE_AMD64,   // platformAMD64
    IMAGE_FILE_MACHINE_ARMNT,   // platformARM
    IMAGE_FILE_MACHINE_I386,    // platformAnycpu32bitpreferred
};

//
// Structures used to store and retriev information in the global
// token hash table.
//
// We need to make sure that these keys will always be unique.  It would
// really ---- to send one key to the hash table that just happens to look
// like another key and get back garbage.  I think these keys are ok
// with the addition of the dummy NULL string in ArrayKey.
//

//-------------------------------------------------------------------------------------------------
//
// The key for a type reference.
//
struct TypeRefKey
{
    // The name of the type.
    STRING *m_pstrTypeName;

    // The definition for this type.
    mdTypeDef m_tkContainer;

    // The above token is relative to this project.
    CompilerProject *m_pProject;
};

#pragma warning(disable: 4200)

//-------------------------------------------------------------------------------------------------
//
// Abstracts the information needed to uniquely identify a member in
// the token cache.
//
struct MetaMemberRef
{
    // The name of the member.
    STRING *m_pstrMemberName;

    // The token for the import type that contains the member.
    mdTypeRef m_tr;

    // The size of the member's signature.
    unsigned m_cbSizeSig;

    // Signature layout is:
    //     o  Calling convention (1 byte)
    //     o  Number of args (1-4 bytes, encoded)
    //     o  Return type
    //     o  Arg types
    //
    BYTE m_pSignature[0];
};

//-------------------------------------------------------------------------------------------------
//
// The key for an array in "the structure".  We encode this based
// of the signature to avoid problems with symbols going away and
// getting recreated as something else.
//
struct ArrayKey
{
    // This should always be NULL to prevent accidental collisions
    // between different kinds of keys.
    //
    STRING *m_pstrDummy;

    // The size of the arrays's signature.
    unsigned m_cbSizeSig;

    // Signature layout is:
    //     o  Calling convention (1 byte)
    //     o  Number of args (1-4 bytes, encoded)
    //     o  Return type
    //     o  Arg types
    //
    BYTE m_pSignature[0];
};

struct GenericTypeInstantiation
{
    // The name of the type.
    STRING *m_pstrMemberName;

    // The token for the unbound type.
    mdTypeRef m_TypeRef;

    // The size of the signature.
    unsigned m_cbSizeSig;

    BYTE m_pSignature[0];
};

struct GenericMethodInstantiation
{
    // The name of the method.
    STRING *m_pstrMemberName;

    // The token for the generic method.
    mdMemberRef m_MemberRef;

    // The size of the signature.
    unsigned m_cbSizeSig;

    BYTE m_pSignature[0];
};

#pragma warning(default: 4200)

static
DWORD MapPlatformKindToPEKind(PlatformKinds PlatformKind)
{
    VSASSERT(PlatformKind <= Platform_Max, "Unexpected platform kind!!!");

#pragma prefast(suppress: 26010 26011, "Function is only called with valid enums");
    return rgPEKind[PlatformKind];
}

static
DWORD MapPlatformKindToMachineKind(PlatformKinds PlatformKind)
{
    VSASSERT(PlatformKind <= Platform_Max, "Unexpected platform kind!!!");

#pragma prefast(suppress: 26010 26011, "Function is only called with valid enums");
    return rgMachine[PlatformKind];
}

//****************************************************************************
// Lifetime
//****************************************************************************

//============================================================================
// Constructors / Destructor
//============================================================================

MetaEmit::MetaEmit
(
    _In_ Builder *pBuilder,
    _In_ CompilerProject *pCompilerProject,
    CompilationCaches *pCompilationCaches
)
: m_nraScratch(NORLSLOC)
, m_nraSignature(NORLSLOC)
, m_pCompilationCaches(pCompilationCaches)
{
    Init(pBuilder, pCompilerProject, NULL, NULL, mdTokenNil, mdTokenNil);
}

MetaEmit::MetaEmit
(
    _In_ Builder *pBuilder,
    _In_ CompilerProject *pCompilerProject,
    IALink2 *pALink,
    IALink3 *pALink3,
    mdToken mdAssemblyOrModule,
    mdFile mdFileForModule
)
: m_nraScratch(NORLSLOC)
, m_nraSignature(NORLSLOC)
, m_pCompilationCaches(NULL)
{
    Init(pBuilder, pCompilerProject, pALink, pALink3, mdAssemblyOrModule, mdFileForModule);
}

void MetaEmit::Init
(
    _In_ Builder *pBuilder,                 // Project-level datastructures
    _In_ CompilerProject *pCompilerProject,// Project we are building for.
    IALink2 *pALink,                     // Helper ALink
    IALink3 *pALink3,                   // Helper ALink3
    mdToken mdAssemblyOrModule,         // Assembly or module token emitting to
    mdFile mdFileForModule              // File or module emitting to
)
{
    m_pCompiler = pBuilder->m_pCompiler;
    m_pBuilder = pBuilder;
    m_pmdEmit = pBuilder->m_pmdEmit;
    m_pMetaEmitHelper = new MetaEmitHelper();
    m_pMetaEmitHelper->Initialize(m_pmdEmit);
    m_pCompilerProject = pCompilerProject;
    m_pALink = pALink;
    m_pALink3 = pALink3;
    m_mdAssemblyOrModule = mdAssemblyOrModule;
    m_mdFileForModule = mdFileForModule;
    m_SignatureToken = mdTokenNil;

    m_bIsHighEntropyVA = pCompilerProject != NULL ? pCompilerProject->IsHighEntropyVA() : true;


    // Release ALink when destroyed if no ALink passed in
    m_fReleaseALink = (!pALink);
}

MetaEmit::~MetaEmit()
{
    // Close and release ALink if created
    if (m_fReleaseALink && m_pALink)
    {
        IfFailThrow(m_pALink->CloseAssembly(m_mdAssemblyOrModule));

        RELEASE(m_pALink);
    }

    if (m_pMetaEmitHelper)
    {
        delete m_pMetaEmitHelper;
        m_pMetaEmitHelper = NULL;
    }
}

//****************************************************************************
// Helpers for the PE builder.
//****************************************************************************

//****************************************************************************
// Type references
//****************************************************************************

//============================================================================
// Get a typeref token to point to the given class.
//============================================================================
mdTypeRef MetaEmit::DefineTypeRefBySymbol
(
    BCSYM *ptyp,                   // [in] The type to create a reference to.
    Location *pReferencingLocation // [in] the location in the code that is referring to this symbol.
)
{
    mdTypeRef         tr = mdTypeRefNil;

    if (ptyp->IsPointerType())
    {
        ptyp = ptyp->PPointerType()->GetRoot()->DigThroughAlias();
    }

    if (ptyp->IsGenericTypeBinding())
    {
        tr = DefineGenericTypeInstantiation(ptyp->PGenericTypeBinding(), pReferencingLocation);
    }
    else if (ptyp->IsContainer())
    {
        tr = DefineTypeRefByContainer(ptyp->PContainer(), pReferencingLocation);
    }
    else if (ptyp->IsArrayType())
    {
        // We can use a typespec instead of a typeref
        tr = DefineTypeSpec(ptyp->PArrayType(), pReferencingLocation);
    }
    else if (ptyp->IsGenericParam())
    {
        tr = GetToken(ptyp->PGenericParam());
    }
    else
    {
        VSFAIL("Not a type symbol.");
    }

    VSASSERT(!IsNilToken(tr), "Bad COM+ output!");

    return tr;
}

//========================================================================
// Defines a TypeDef or a TypeRef to a Container.  All of the type reference
// methods should bottom out here because TypeRefs here are defined by name
// and it is faster to define TypeRefs by name than by token.
//========================================================================

mdTypeRef MetaEmit::DefineTypeRefByContainer
(
    BCSYM_Container *pType,
    Location *pReferencingLocation
)
{
    VSASSERT(!pType->IsNamespace(), "Bad container.");

    mdTypeRef tr;
    CompilerProject *pProject = pType->GetContainingProject();

    if (m_pBuilder->m_pCompilerProject == pProject)
    {
        tr = (mdTypeRef)GetTypeDef(pType);
    }
    else
    {
         TypeRefKey key;

        mdTypeRef *ptr;
        mdTypeRef tkContainer;

        BCSYM_Container *pContainer = pType->GetContainingClassOrInterface();

        VSASSERT(pType->GetEmittedName(), "Must have an emitted name to write to metadata.");

        STRING *pstrTypeName =
            pType->GetContainer()->IsNamespace() ?
                ConcatNameSpaceAndName(m_pCompiler, pType->GetNameSpace(), pType->GetEmittedName()) :
                pType->GetEmittedName();


#ifdef _WIN64
        // In 64 bits build, there is garbage in between the structure, thus causing hash finds to fail
        // which in turn causes multiple duplicate typerefs to be emitted resulting in Peverify errors.
        // So, in order to avoid the garbage, always init with 0s.
        //
        memset(&key, 0, sizeof(key));

#endif //_WIN64

        // Build a key.
        key.m_pstrTypeName = pstrTypeName;

        // Key off the token rather than the symbol as the symbol may move
        // between compilations, the token never will.
        //
        if (pContainer)
        {
            key.m_tkContainer = (mdTypeDef)GetTypeDef(pContainer);
        }
        else
        {
            key.m_tkContainer = mdTypeDefNil;
        }

        // The above typedef is relative to this project.
        key.m_pProject = pProject;

        // Look in the hash table.
        ptr = (mdTypeRef *)m_pBuilder->m_ProjectHashTable.Find(key);

        if (ptr)
        {
            tr = *ptr;

            if (m_TrackReferrencesToEmbeddableInteropTypes)
            {
                m_ReferredToEmbeddableInteropType = m_ReferredToEmbeddableInteropType || m_pCompilerProject->IsCachedPiaTypeRef(pType);
            }
        }
        else
        {
#if IDE 
            CompilerProject * pLocalTypeProject = NULL;
#endif                
        
            VSASSERT(pProject != m_pBuilder->m_pCompilerProject, "Should have been pre-loaded.");


            if (!pContainer && TypeHelpers::IsEmbeddableInteropType(pType))
            {
                // Dev10 #697784 Define type for embedded (NoPIA) type.
                ThrowIfFalse(m_pBuilder->IsPEBuilder());
                PEBuilder *pPEBuilder = (PEBuilder*)m_pBuilder;
                
                pPEBuilder->DefineContainer(pType);
                tr = pPEBuilder->GetTypeDef(pType);

                // Record type with Nil TypeRef token  
                PEBuilder::PiaTypeDefInfo info;
                info.m_DefinitionOrder = pPEBuilder->m_NoPiaTypeDefToTypeRefMap.Count();
                info.m_TypeRef = mdTypeRefNil;
                pPEBuilder->m_NoPiaTypeDefToTypeRefMap.SetValue(pType, info);
            }
            else
            {
                if (pContainer)
                {
                    // We have a nested type, recurse to get the container token.
                    tkContainer = DefineTypeRefBySymbol(pContainer, pReferencingLocation);
                }
                else
                {
#if IDE             
                    if (m_pCompilerProject->GetTrackUnsubstitutedLocalTypes() && 
                        m_pCompilerProject->m_UnsubstitutedLocalTypes.GetValue(pType, &pLocalTypeProject) && pLocalTypeProject != NULL)
                    {
                        // This is an unsubstituted local type. For scenarios with Project to Project references, the project for the local type may be
                        // different from the symbol's project because we don't create symbols for compiler generated local types and are using symbols 
                        // for canonical types instead.
                        pProject = pLocalTypeProject;
                    }
#endif                
                    
                    // A non-nested type, get the token for the reference.
                    tkContainer = LookupProjectRef(pProject, pType, pReferencingLocation);

                    // VSW#127698: LookupProjectRef can return a nil token. At this point there isn't much we
                    // can do, since it's most likely an out of memory issue, so we just create a value that
                    // represents a "valid" token.

                    if (IsNilToken(tkContainer))
                    {
                        // HACK: Microsoft This is an arbitrary token value. It is just as arbitrary as returning the token
                        // for Sytem.Object like we did before, but it keeps us from entering an infinite recursion.
                        // We should consider going through the compiler sources and handling mdTokenNil
                        // gracefully, as it is a value that we can get back line in the above bug.
                        tr = mdTypeRefNil + 1;

                        goto GotToken;
                    }
                }

                // Fill in the type ref.
                //
                VSASSERT(!IsNilToken(tkContainer), "Attempt to emit non-scoped TypeRef.");
                IfFailThrow(m_pmdEmit->DefineTypeRefByName(tkContainer,
                                                        pstrTypeName,
                                                        &tr));
            }

GotToken:

#if IDE 
            pLocalTypeProject = NULL;
#endif                

            // Add it to the cache.
            m_pBuilder->m_ProjectHashTable.Add(key, tr);

            if (TypeHelpers::IsEmbeddableInteropType(pType))
            {
                m_pCompilerProject->CachePiaTypeRef(pType, pReferencingLocation, m_pBuilder->m_pErrorTable);

                if (m_TrackReferrencesToEmbeddableInteropTypes)
                {
                    m_ReferredToEmbeddableInteropType = true;
                }
            }
            else if (TypeHelpers::IsEmbeddedLocalType(pType)  
#if IDE 
                    || (m_pCompilerProject->GetTrackUnsubstitutedLocalTypes() &&
                            m_pCompilerProject->m_UnsubstitutedLocalTypes.GetValue(pType, &pLocalTypeProject) && pLocalTypeProject != NULL)
#endif
                )
            {
                // We must never emit a reference to a local type embedded in another assembly.
                // Instead, we should emit a reference to the canonical type, or to our own local type.
                // We will only attempt to emit such a typeref when this project does not have its own
                // reference to the assembly containing the canonical type. This is an error.
                m_pBuilder->m_pErrorTable->CreateError(
                    ERRID_AbsentReferenceToPIA1,
                    pType->GetLocation(),
                    pType->GetName());
            }

#if IDE
            if (m_pBuilder->IsENCBuilder())
            {
                static_cast<ENCBuilder *>(m_pBuilder)->NewProjectHashTableEntry(&key, sizeof(key), &tr, sizeof(tr));
            }
#endif
        }
    }

    return tr;
}

//========================================================================
// Looks up a reference to a project.  The reference must have already
// been created via MetaEmit::DefineProjectRef.
//
// If the lookup fails, pstrTypeName is used in the error text, and
// mdTokenNil is returned.
//========================================================================
mdAssemblyRef MetaEmit::LookupProjectRef
(
    CompilerProject *pProject,
    BCSYM_NamedRoot *pType,
    Location *pReferencingLocation
)
{
    mdAssemblyRef *ptkAssemblyRef;

    // Find reference and return
    ptkAssemblyRef = (mdAssemblyRef *)m_pBuilder->m_ProjectHashTable.Find(pProject);

    if (ptkAssemblyRef == NULL)
    {
        // Define AssemblyRef or report error if looking up unreferenced project
        if (m_pCompilerProject->IsProjectReferenced(pProject))
        {
            // Define AssemblyRef
            return DefineProjectRef(pProject);
        }
        else
        {
            // Check for versioning issues

            ReferenceIterator References(m_pCompilerProject);
            bool NonEquivalentOnlyDueToLowerVersion = false;

            if (pProject->IsMetaData())
            {
                while (CompilerProject *pReference = References.Next())
                {
                    if (!pReference->IsMetaData())
                    {
                        continue;
                    }

                    AssemblyComparisonResult AssemblyCompare =
                        AssemblyIdentity::CompareAssemblies(
                                pReference->GetAssemblyIdentity(),
                                pProject->GetAssemblyIdentity(),
                                NULL,
                                NULL,
                                &NonEquivalentOnlyDueToLowerVersion,
                                m_pCompiler,
                                m_pCompilerProject->GetCompilerHost());

                    switch (AssemblyCompare)
                    {
                        case ACR_EquivalentFullMatch:
                        case ACR_EquivalentFXUnified:
                            {
                                mdAssemblyRef tkAssemblyRef = LookupProjectRef(pReference, pType, pReferencingLocation);

                                // Map both the projects that have exactly the same identity to the same assembly ref.
                                if (!IsNilToken(tkAssemblyRef))
                                {
                                    m_pBuilder->m_ProjectHashTable.Add(pProject, tkAssemblyRef);
#if IDE
                                    if (m_pBuilder->IsENCBuilder())
                                    {
                                        static_cast<ENCBuilder *>(m_pBuilder)->NewProjectHashTableEntry(
                                                                                    &pProject,
                                                                                    sizeof(pProject),
                                                                                    &tkAssemblyRef,
                                                                                    sizeof(tkAssemblyRef));
                                    }
#endif
                                }

                                return tkAssemblyRef;
                            }

                        case ACR_EquivalentWeakNamed:
                        case ACR_EquivalentUnified:
                            {
                                // 

                                // Project '|1' requires a reference to version '|2' of assembly '|3', but references version '|4' of assembly '|3'.
                                // Reference to version '|2' emitted.

                                m_pBuilder->m_pErrorTable->CreateError(
                                    WRNID_SxSHigherIndirectRefEmitted4,
                                    NULL,
                                    m_pCompilerProject->GetErrorName(),
                                    pProject->GetAssemblyIdentity()->GetAssemblyVersion(),
                                    pProject->GetAssemblyIdentity()->GetAssemblyName(),
                                    pReference->GetAssemblyIdentity()->GetAssemblyVersion());

                                return DefineProjectRef(pProject);
                            }

                        case ACR_NonEquivalent:

                            if (NonEquivalentOnlyDueToLowerVersion)
                            {
                                // Project '|1' requires a reference to version '|2' of assembly '|3', but references version '|4' of assembly '|3'.

                                m_pBuilder->m_pErrorTable->CreateError(
                                    ERRID_SxSLowerVerIndirectRefNotEmitted4,
                                    NULL,
                                    m_pCompilerProject->GetErrorName(),
                                    pProject->GetAssemblyIdentity()->GetAssemblyVersion(),
                                    pProject->GetAssemblyIdentity()->GetAssemblyName(),
                                    pReference->GetAssemblyIdentity()->GetAssemblyVersion());

                                return mdTokenNil;
                            }
                    }
                }
            }

            if (pReferencingLocation && pReferencingLocation->IsValid())
            {
                ReportSmartReferenceError(
                    (pProject->IsMetaData() ? ERRID_IndirectUnreferencedAssembly3 : ERRID_IndirectUnreferencedProject2),
                    m_pCompilerProject,
                    pProject,
                    m_pCompiler,
                    m_pBuilder->m_pErrorTable,
                    pProject->GetFileName(),
                    pReferencingLocation,
                    pProject->GetErrorName(),
                    pType ? pType->GetQualifiedName(true, NULL, true) : STRING_CONST(m_pCompiler, EmptyString),
                    (pProject->IsMetaData() ? pProject->GetFileName() : NULL));     // path to DLL for ERRID_IndirectUnreferencedAssembly3
            }
            else
            {
                ReportSmartReferenceError(
                    (pProject->IsMetaData() ? ERRID_IndirectUnreferencedAssembly4 : ERRID_IndirectUnreferencedProject3),
                    m_pCompilerProject,
                    pProject,
                    m_pCompiler,
                    m_pBuilder->m_pErrorTable,
                    pProject->GetFileName(),
                    NULL,
                    m_pBuilder->m_pCompilerProject->GetErrorName(),
                    pProject->GetErrorName(),
                    pType ? pType->GetQualifiedName(true, NULL, true) : STRING_CONST(m_pCompiler, EmptyString),
                    (pProject->IsMetaData() ? pProject->GetFileName() : NULL));   // path to DLL for ERRID_IndirectUnreferencedAssembly4
            }

            return mdTokenNil;
        }
    }
    else
    {
        return *ptkAssemblyRef;
    }
}


//========================================================================
// Define a reference to a project.
//========================================================================
mdAssemblyRef MetaEmit::DefineProjectRef
(
    CompilerProject *pProject
)
{
    VSASSERT(pProject, "Bad project");

    mdAssemblyRef tkAssemblyRef;

    // Can't already be in the hash table
    VSASSERT(m_pBuilder->m_ProjectHashTable.Find(pProject) == NULL,
            "Project already in hash table!");

    // Emit the reference and store in the hash table
    tkAssemblyRef = ALinkEmitProjectRef(pProject);

    // Don't add a nil token to the hash, otherwise we won't report the error
    // later on.
    if (!IsNilToken(tkAssemblyRef))
    {
        m_pBuilder->m_ProjectHashTable.Add(pProject, tkAssemblyRef);
#if IDE
        if (m_pBuilder->IsENCBuilder())
        {
            static_cast<ENCBuilder *>(m_pBuilder)->NewProjectHashTableEntry(&pProject, sizeof(pProject), &tkAssemblyRef, sizeof(tkAssemblyRef));
        }
#endif
    }

    return tkAssemblyRef;
}


//========================================================================
// Caching metadataemit wrapper around "DefineModuleRef"
//========================================================================

mdModuleRef MetaEmit::DefineModuleRef
(
    _In_z_ STRING *pstrModuleName
)
{
    mdModuleRef *pModuleRef, tkModuleRef;

    // See if it's already in the hash table
    pModuleRef = (mdModuleRef *)m_pBuilder->m_ProjectHashTable.Find(pstrModuleName);

    if (pModuleRef)
    {
        tkModuleRef = *pModuleRef;
    }
    else
    {
        // Need to define module ref and add to the hash table
        IfFailThrow(m_pmdEmit->DefineModuleRef(pstrModuleName, &tkModuleRef));

        m_pBuilder->m_ProjectHashTable.Add(pstrModuleName, tkModuleRef);
#if IDE
        if (m_pBuilder->IsENCBuilder())
        {
            static_cast<ENCBuilder *>(m_pBuilder)->NewProjectHashTableEntry(&pstrModuleName, sizeof(pstrModuleName), &tkModuleRef, sizeof(tkModuleRef));
        }
#endif
    }

    return tkModuleRef;
}

//****************************************************************************
// Member declaration
//****************************************************************************


// Describes context for CacheAllMembersOfImplementedEmbeddableInterface function.
struct ImplementsContext
{
public:
    ImplementsContext
    (
        Compiler * pCompiler, 
        CompilerProject * pProject, 
        Location * pReferencingLocation, 
        ErrorTable * pErrorTable)
    {
        m_pCompiler = pCompiler;
        m_pProject = pProject; 
        m_pReferencingLocation = pReferencingLocation; 
        m_pErrorTable = pErrorTable;
    }

    Compiler * m_pCompiler;
    CompilerProject * m_pProject; 
    Location * m_pReferencingLocation; 
    ErrorTable * m_pErrorTable;
};

// Given an interface, cache all its members if it is embeddable.
// Do the same for all implemented interfaces as well.
static void
CacheAllMembersOfImplementedEmbeddableInterface
(
    BCSYM * pImplSymbol,
    ImplementsContext & context
)
{
    AssertIfFalse(pImplSymbol->IsInterface());

    if (pImplSymbol->IsInterface())
    {
        BCSYM_Interface * pInterface = pImplSymbol->PInterface();

        if (TypeHelpers::IsEmbeddableInteropType(pInterface))
        {
            DefineableMembersInAContainer memIter(context.m_pCompiler, pInterface);

            while (BCSYM *pSymbol = memIter.Next())
            {
                // Skip hidden things that aren't meant to be called or implemented. 
                // This also should make sure that VTable gaps are emitted properly.
                if (pSymbol->IsNamedRoot() && !pSymbol->PNamedRoot()->DoesntRequireImplementation())
                {
                    context.m_pProject->CachePiaTypeMemberRef(pSymbol->PNamedRoot(), context.m_pReferencingLocation, context.m_pErrorTable);
                }
            }

            BCITER_ImplInterfaces implList(pInterface);

            while(BCSYM_Implements *----l = implList.GetNext())
            {
                CacheAllMembersOfImplementedEmbeddableInterface(----l->GetCompilerRoot(), context);
            }
        }
    }
}

//============================================================================
// Get an array of typerefs that point to the implemented interfaces.
//============================================================================

mdTypeRef *MetaEmit::DefineImplements
(
    BCITER_ImplInterfaces *----lList,
    mdToken ComClassInterface,
    bool fEmbedAllMembersOfImplementedEmbeddableInterfaces /*= false*/
)
{
    unsigned cImpls;
    BCSYM_Implements *----l;
    mdTypeRef *rgtr;

    ----lList->Reset();

    // Count the number of implemented interfaces.
    cImpls = 0;
    while(----l = ----lList->GetNext())
    {
        // Redundant implements are allowed with no errors when the redundant implements
        // are specified in different partial types. But need to ensure that they are
        // emitted only once.
        //
        // Devdiv 35011:
        // Should not emit the Re-implemented interfaces in order for the CLR interface layout
        // algorithm to provide VB semantics with respect to interface implementation.
        // Unless all members are reimplemented (Bug #104767 - DevDiv Bugs)
        if (----l->IsRedundantImplements() ||
            (----l->IsReimplementingInterface() && !----l->AreAllMembersReimplemented()))
        {
            continue;
        }

        cImpls++;
        if (cImpls==0)
            return NULL; // Overflow
    }

    // Add one for the com class interface
    if (!IsNilToken(ComClassInterface))
    {
        cImpls++; // Will catch overflow in next section.
    }

    if (!cImpls)
    {
        return NULL;
    }

    // Create the return array.
    if (cImpls + 1 == 0 || 
        !VBMath::TryMultiply((cImpls + 1), sizeof(mdTypeRef)))
    {
        return NULL; // Overflow
    }
    rgtr = (mdTypeRef *)m_nraScratch.Alloc((cImpls + 1) * sizeof(mdTypeRef));

    // The ComClassInterface must be the 1st implemented interface
    cImpls = 0;
    if (!IsNilToken(ComClassInterface))
    {
        rgtr[cImpls] = (mdTypeRef)ComClassInterface;
        cImpls++;
    }

    // Fill it all in.
    ----lList->Reset();
    while(----l = ----lList->GetNext())
    {
        // Redundant implements are allowed with no errors when the redundant implements
        // are specified in different partial types. But need to ensure that they are
        // emitted only once.
        //
        // Devdiv 35011:
        // Should not emit the Re-implemented interfaces in order for the CLR interface layout
        // algorithm to provide VB semantics with respect to interface implementation.
        // Unless all members are reimplemented (Bug #104767 - DevDiv Bugs)
        if (----l->IsRedundantImplements() ||
            (----l->IsReimplementingInterface() && !----l->AreAllMembersReimplemented()))
        {
            continue;
        }
        
        BCSYM * pImplSymbol = ----l->GetCompilerRoot();
        Location * pImplLocation = ----l->GetLocation();
        rgtr[cImpls++] = DefineTypeRefBySymbol(pImplSymbol, pImplLocation);

        // Dev10 #680929 Make sure to embed all virtual members for "base" interfaces.
        // We want VTable to be filled with available members.
        if (fEmbedAllMembersOfImplementedEmbeddableInterfaces)
        {
            CacheAllMembersOfImplementedEmbeddableInterface(
                pImplSymbol, 
                ImplementsContext(m_pCompiler, m_pCompilerProject, pImplLocation, m_pBuilder->m_pErrorTable));
        }
    }

    // The last entry is Nil so we know the end of the array of tokens.
    rgtr[cImpls] = mdTypeDefNil;

    return rgtr;
}


//============================================================================
// Define a set of generic parameters.
//============================================================================

void MetaEmit::DefineGenericParamsForType
(
    BCSYM_Container *PossiblyGeneric,
    mdToken GenericToken
)
{
    unsigned ParamCount;

    DefineGenericParamsNoEmit(PossiblyGeneric, true, ParamCount);

    EmitGenericParams(PossiblyGeneric, GenericToken, true, ParamCount);
}

void MetaEmit::DefineGenericParamsNoEmit
(
    BCSYM_NamedRoot *PossiblyGeneric,
    bool IncludeParentParams,
    _Out_ unsigned &ParamCount
)
{
    ParamCount = 0;

    if (!IsGeneric(PossiblyGeneric) &&
        (!IncludeParentParams || !IsGenericOrHasGenericParent(PossiblyGeneric->GetParent())))
    {
        // The symbol is not generic
        return;
    }

    DefineGenericParamTokens(PossiblyGeneric, IncludeParentParams, ParamCount);
}

//============================================================================
// Define the tokens for a set of generic parameters.
//============================================================================

void MetaEmit::DefineGenericParamTokens
(
    BCSYM_NamedRoot *PossiblyGeneric,
    bool IncludeParentParams,
    _Inout_ unsigned &ParamIndex,
    bool IsParent
)
{
    if (IncludeParentParams)
    {
        if (!PossiblyGeneric->IsType())
        {
            return;
        }

        DefineGenericParamTokens(PossiblyGeneric->GetParent(), true, ParamIndex, true);
    }

    // Set the metadata position for the type parameters and emit their tokens
    //
    // Note: Define all the parameter tokens before binding the constraints
    // because the parameters can be used in bindings of the constraint types.

    for (BCSYM_GenericParam *Param = PossiblyGeneric->GetFirstGenericParam(); Param; Param = Param->GetNextParam())
    {
        if (!IsParent)
        {
            Param->SetMetaDataPosition(ParamIndex);

            // Define the token for the parameter.

            Signature ParamSignature;
            StartNewSignature(&ParamSignature);
            EncodeType(Param, NULL);

            mdTypeRef ParamToken;
            IfFailThrow(m_pmdEmit->GetTokenFromTypeSpec(GetSignature(), GetSigSize(), &ParamToken));

            ReleaseSignature(&ParamSignature);

            if (m_pBuilder->IsPEBuilder())
            {
                SetToken(Param, ParamToken);
            }
        }

        ParamIndex++;
    }
}

void MetaEmit::EmitGenericParams
(
    BCSYM_NamedRoot *PossiblyGeneric,
    mdToken GenericToken,
    bool IncludeParentParams,
    unsigned ParamCount
)
{
    // No generic params to emit
    //
    if (ParamCount == 0)
    {
        return;
    }

#if DEBUG
    unsigned ParamIndex = 0;
#endif DEBUG

    EmitGenericParamsHelper(
        PossiblyGeneric,
        GenericToken,
        IncludeParentParams
#if DEBUG
        ,ParamIndex
#endif
        );
}

void MetaEmit::EmitGenericParamsHelper
(
    BCSYM_NamedRoot *PossiblyGeneric,
    mdToken GenericToken,
    bool IncludeParentParams
#if DEBUG
    ,unsigned &ParamIndex
#endif DEBUG
)
{
    // IncludeParentParams is false for generic methods because for them,
    // we don't need  emit the enclosing types' type parameters
    //
    if (IncludeParentParams)
    {
        if (!PossiblyGeneric->IsType())
        {
            return;
        }

        EmitGenericParamsHelper(
            PossiblyGeneric->GetParent(),
            GenericToken,
            true
#if DEBUG
            , ParamIndex
#endif DEBUG
            );
    }

    for (BCSYM_GenericParam *Param = PossiblyGeneric->GetFirstGenericParam(); Param; Param = Param->GetNextParam())
    {
        VSASSERT(ParamIndex == Param->GetMetaDataPosition(), "Generic parameter metadata positions are confused.");

        EmitGenericParam(GenericToken, Param);

#if DEBUG
        ParamIndex++;
#endif DEBUG
    }
}

void MetaEmit::EmitGenericParam
(
    mdToken GenericToken,
    BCSYM_GenericParam *GenericParam
)
{
    // Count the number of constraints
    //
    unsigned TypeConstraintCount = 0;
    BCSYM_GenericConstraint *Constraint;
    DWORD Flags = 0;

    switch (GenericParam->GetVariance())
    {
        case Variance_In:
            Flags |= gpContravariant;
            break;
        case Variance_Out:
            Flags |= gpCovariant;
            break;
        case Variance_None:
            Flags |= gpNonVariant;
            break;
        default:
            VSFAIL("Unexpected variance in meta-emit");
    }

    for (Constraint = GenericParam->GetConstraints(); Constraint; Constraint = Constraint->Next())
    {
        if (Constraint->IsNewConstraint())
        {
            Flags |= gpDefaultConstructorConstraint;
        }
        else if (Constraint->IsReferenceConstraint())
        {
            Flags |= gpReferenceTypeConstraint;
        }
        else if (Constraint->IsValueConstraint())
        {
            Flags |= gpNotNullableValueTypeConstraint;
        }
        else    // Type constraint
        {
            VSASSERT(Constraint->IsGenericTypeConstraint(), "Unexpected constraint kind!!!");

            // Only type constraints are emitted as tokens, the rest are emitted as flags. So count only the
            // type constraints.

            TypeConstraintCount++;
        }
    }

    // Allocate memory to hold the constraint tokens plus a Nil token to represent the end.
    //
    const unsigned ScratchConstraintLimit = 8;

    mdToken ConstraintsScratch[ScratchConstraintLimit];
    mdToken *TypeConstraints = ConstraintsScratch;

    NorlsMark ScratchMark;

    // Note we need an extra slot in the array to NULL terminate the array, hence the +1
    //
    if ((TypeConstraintCount + 1) > ScratchConstraintLimit)
    {
        m_nraScratch.Mark(&ScratchMark);

        IfFalseThrow(TypeConstraintCount + 1 >= 1);
        TypeConstraints = (mdToken *)m_nraScratch.Alloc(VBMath::Multiply(
            sizeof(mdToken), 
            (TypeConstraintCount + 1)));
    }

    // Collect the constraints into an array
    //
    unsigned Index = 0;

    for (BCSYM_GenericTypeConstraint *TypeConstraint = GenericParam->GetTypeConstraints();
         TypeConstraint;
         TypeConstraint = TypeConstraint->Next())
    {
#pragma prefast(suppress: 26010, "TypeConstraints is correctly allocated above")
        TypeConstraints[Index++] = DefineTypeRefBySymbol(TypeConstraint->GetType(), TypeConstraint->GetLocation());
    }

    // NULL termination for the constraint array
    //
    TypeConstraints[TypeConstraintCount] = 0;

    mdGenericParam GenericParamToken;
    IfFailThrow(m_pmdEmit->DefineGenericParam(GenericToken,
                                              GenericParam->GetMetaDataPosition(),
                                              Flags,                        // Flags that describe some of the constraints
                                              GenericParam->GetName(),
                                              0,                            // For future use
                                              TypeConstraints,
                                              &GenericParamToken));

    VSASSERT(!IsNilToken(GenericParamToken), "emitting generic param failed!!!");

    if ((TypeConstraintCount + 1) > ScratchConstraintLimit)
    {
        m_nraScratch.Free(&ScratchMark);
    }
}

//============================================================================
// Write out meta data for a method
//============================================================================

void MetaEmit::DefineMethod
(
    BCSYM_Proc *pproc
)
{
    mdMethodDef       methdefBody;
    STRING        *   pstrName;
    BCSYM_Param   *   pparam;
    mdParamDef        paramdef;
    ULONG             ulParamSeq = 1;
    unsigned          ulen=0;
    Signature         signature;
    BCSYM_Container * pParent = pproc->GetContainer();

    VSASSERT(!pproc->IsProperty() && !pproc->IsEventDecl(), "Wrong type!");

    // If this method is a partial declaration, just remove it from the image.

    if( pproc->IsPartialMethodDeclaration() )
    {
        return;
    }

    //
    // Set up the flags.
    //

    DWORD dwMethodFlags = 0;
    DWORD dwImplFlags = 0;

    if (pParent && pParent->IsDelegate())
    {
        dwImplFlags |= miRuntime;
    }
    else if (pproc->GetContainingClass() && pproc->GetContainingClass()->GetPWellKnownAttrVals()->GetComImportData())
    {
        dwImplFlags |= miRuntime | miManaged | miInternalCall;
    }
    else
    {
        dwImplFlags |= miIL | miManaged;
    }

    if (pproc->IsSyntheticMethod())
    {
        // Add/remove for events in classes are synchronized
        if ((pproc->PSyntheticMethod()->GetSyntheticKind() == SYNTH_AddEvent ||
             pproc->PSyntheticMethod()->GetSyntheticKind() == SYNTH_RemoveEvent ||
             pproc->PSyntheticMethod()->GetSyntheticKind() == SYNTH_WithEventsSet) &&
            pParent->IsClass() &&
            !pParent->PClass()->IsStruct())
        {
            dwImplFlags |= miSynchronized;
        }
    }

    if (pproc->IsMustOverrideKeywordUsed())
    {
        dwMethodFlags |= mdAbstract;
    }

    if (pproc->IsShared())
    {
        dwMethodFlags |= mdStatic;
    }

    if ( pproc->IsVirtual() )
    {
        dwMethodFlags |= mdVirtual;

        if (pproc->CheckAccessOnOverride())
        {
            // Disallow users from overriding this method if they lack the accessbility to do so.
            dwMethodFlags |= mdCheckAccessOnOverride;
        }

        // If there is a shadowing method between an overriding method and the
        // method it overrides, we have to emit it as a newslot and then emit
        // an explicit methodimpl. If we don't, we'll end up overriding the shadowed
        // method.
        if ((!pproc->IsOverridesKeywordUsed() &&
             !pproc->IsNotOverridableKeywordUsed() &&
             !pproc->IsOverrides()) ||
            (pproc->IsOverrides() &&
             pproc->OverridesRequiresMethodImpl()))
        {
            VSASSERT(!pproc->IsShared(), "How did a Shared method get marked as Overriding another?");
            dwMethodFlags |= mdNewSlot;
        }
    }

    if (pproc->IsPreserveSig())
    {
        dwImplFlags |= miPreserveSig;
    }

    if (pproc->IsNotOverridableKeywordUsed() ||
        ((pproc->IsImplementing() ||
            pproc->IsImplementingComClassInterface()) &&
         !pproc->IsOverridesKeywordUsed() &&
         !pproc->IsMustOverrideKeywordUsed() &&
         !pproc->IsOverridableKeywordUsed() &&
         !pproc->IsNotOverridableKeywordUsed()))
    {
        dwMethodFlags |= mdFinal;
    }

    if (pproc->IsSpecialName())
    {
        dwMethodFlags |= mdSpecialName;
    }

    if (pproc->IsOverloadsKeywordUsed() && !pproc->IsShadowing())
    {
        dwMethodFlags |= mdHideBySig;
    }

    // Declare.
    if (pproc->IsDllDeclare())
    {
         dwMethodFlags |= mdStatic | mdPinvokeImpl;
         dwImplFlags |= miPreserveSig;
    }

    // Set the access.
    switch(pproc->GetAccess())
    {
    case ACCESS_Public:
        dwMethodFlags |= mdPublic;
        break;

    case ACCESS_Protected:
        dwMethodFlags |= mdFamily;
        break;

    case ACCESS_Private:
        dwMethodFlags |= mdPrivate;
        break;

    case ACCESS_Friend:
        dwMethodFlags |= mdAssem;
        break;

    case ACCESS_ProtectedFriend:
        dwMethodFlags |= mdFamORAssem;
        break;

    case ACCESS_CompilerControlled:
        dwMethodFlags |= mdPrivateScope;
        break;

    default:
        VSFAIL("Unexpected access.");
    }

    //
    // Do the emitting.
    //

    unsigned GenericParamCount;
    DefineGenericParamsNoEmit(pproc, false, GenericParamCount);

    // Emit the method.
    pstrName = pproc->GetEmittedName();

    StartNewSignature(&signature);
    EncodeMethodSignature(pproc, NULL);

    IfFailThrow(m_pmdEmit->DefineMethod(GetTypeDef(pParent),
                                        pstrName,
                                        dwMethodFlags,
                                        GetSignature(),
                                        GetSigSize(),
                                        0,
                                        dwImplFlags,
                                        &methdefBody));

    ReleaseSignature(&signature);

#if IDE 
    // Remember this token so we can delete it on next decompile
    m_pBuilder->CacheMetaToken(pproc, methdefBody);
#endif

    EmitGenericParams(pproc, methdefBody, false, GenericParamCount);

    // iterating through each parameter to generate parameter property
    if (pproc->GetCompilerType() != NULL)
    {
        paramdef = mdParamDefNil;

        if ((pproc->IsDllDeclare() &&
             pproc->GetCompilerType()->IsObject()) ||
            pproc->GetReturnTypeParam() ||
            pproc->GetPWellKnownAttrVals()->GetMarshalAsData())
        {
            DWORD dwParamFlags = 0;
            IfFailThrow(m_pmdEmit->DefineParam(methdefBody,
                                               0,      // (which param) retval == 0
                                               NULL,   // Optional param name
                                               dwParamFlags,
                                               ELEMENT_TYPE_VOID,
                                               NULL,   // pValue
                                               0,      // length of pValue in bytes
                                               &paramdef));

            VSASSERT(paramdef != mdParamDefNil, "");

            PCCOR_SIGNATURE marshalAsType;
            ULONG cbMarshalAsType = 0;
            if (pproc->GetPWellKnownAttrVals()->GetMarshalAsData(&marshalAsType, &cbMarshalAsType))
            {
                IfFailThrow(m_pmdEmit->SetFieldMarshal(paramdef, marshalAsType, cbMarshalAsType));
            }

            if (pproc->GetReturnTypeParam())
            {
                if (m_pBuilder->IsPEBuilder())
                {
                    SetToken(pproc->GetReturnTypeParam(), pproc, paramdef);
                }
            }
        }
    }

    for (pparam = pproc->GetFirstParam();
         pparam;
         pparam = pparam->GetNext(), ulParamSeq++)
    {
        BCSYM_Expression  * pbcexpr = NULL;
        DWORD     dwConst = ELEMENT_TYPE_VOID;
        void    * pConst = NULL;
        ULONG     cbConst = 0;

        if (pparam->IsParamWithValue())
        {
            pbcexpr = pparam->PParamWithValue()->GetExpression();
        }

        if (pbcexpr)
        {
            // parameter has default value
            ConstantValue DefaultValue = pbcexpr->GetValue();
            ConvertConstant(&DefaultValue, &dwConst, &pConst, &cbConst);
        }

        // Set the flags.
        DWORD dwParamFlags = 0;

        if (pparam->IsOptional())
        {
            dwParamFlags |= pdOptional;
        }
        if (pparam->GetPWellKnownAttrVals()->GetInData())
        {
            dwParamFlags |= pdIn;
        }
        if (pparam->GetPWellKnownAttrVals()->GetOutData())
        {
            dwParamFlags |= pdOut;
        }
        
        // Define the metadata.
        IfFailThrow(m_pmdEmit->DefineParam(methdefBody,
                                           ulParamSeq,
                                           pparam->GetName(),
                                           dwParamFlags,
                                           dwConst,
                                           pConst,
                                           cbConst,    // length of pConst in bytes
                                           &paramdef));

        VSASSERT(!IsNilToken(paramdef), "Bad COM+ output.");

        PCCOR_SIGNATURE marshalAsType = NULL;
        ULONG cbMarshalAsType = 0;
        if (pparam->GetPWellKnownAttrVals()->GetMarshalAsData(&marshalAsType, &cbMarshalAsType))
        {
            IfFailThrow(m_pmdEmit->SetFieldMarshal(paramdef, marshalAsType, cbMarshalAsType));
        }

        if (m_pBuilder->IsPEBuilder())
        {
            SetToken(pparam, pproc, paramdef);
        }
#if IDE
        else if (m_pBuilder->IsENCBuilder())
        {
            // Track the parameter token during ENC codegen so that attributes can be
            // emitted on the parameter later
            m_pBuilder->AsENCBuilder()->CacheParameterToken(pparam, paramdef);
        }
#endif IDE

        if (pproc->IsDllDeclare())
        {
            BCSYM *ptyp = pparam->GetCompilerType();
            COR_SIGNATURE csig = NATIVE_TYPE_VOID;

            // If the parameter doesn't already have a MarshalAsAttribute
            // then set the special field marshalling for String types
            if (!pparam->GetPWellKnownAttrVals()->GetMarshalAsData())
            {
                if (ptyp->GetVtype() == t_string)
                {
                    csig = NATIVE_TYPE_BYVALSTR;
                }
                else if (ptyp->GetVtype() == t_ptr &&
                         ptyp->PPointerType()->GetRoot()->GetVtype() == t_string)
                {
                    BCSYM_DllDeclare *pdll = pproc->PDllDeclare();
                    switch (pdll->GetDeclareType())
                    {
                        default:
                        case DECLARE_Ansi:
                            csig = NATIVE_TYPE_ANSIBSTR;
                            break;

                        case DECLARE_Unicode:
                            csig = NATIVE_TYPE_BSTR;
                            break;

                        case DECLARE_Auto:
                            csig = NATIVE_TYPE_TBSTR;
                            break;
                    }
                }

                if (csig != NATIVE_TYPE_VOID)
                {
                    // HACK: Marshal the byref string as a byval to the API
                    IfFailThrow(m_pmdEmit->SetFieldMarshal(paramdef,
                                                           &csig,
                                                           sizeof(csig)));
                }
            }
        }
    }

    if (m_pBuilder->IsPEBuilder())
    {
        SetToken(pproc, methdefBody);
    }

    if (pproc->IsDllDeclare())
    {
        unsigned invflags = pmCallConvWinapi | pmSupportsLastError;
        STRING * entrypoint = NULL;
        BCSYM_DllDeclare *pdll = pproc->PDllDeclare();
        mdModuleRef mrDLL;

        switch(pdll->GetDeclareType())
        {
            default:
            case DECLARE_Ansi:
                invflags |= pmCharSetAnsi | pmNoMangle;
                break;

            case DECLARE_Unicode:
                invflags |= pmCharSetUnicode | pmNoMangle;
                break;

            case DECLARE_Auto:
                invflags |= pmCharSetAuto;
                break;
        }

        entrypoint = pdll->GetAliasName();
        if (!entrypoint)
        {
            entrypoint = pdll->GetEmittedName();
        }

        mrDLL = DefineModuleRef(pdll->GetLibName());
        IfFailThrow(m_pmdEmit->DefinePinvokeMap(methdefBody, invflags, entrypoint, mrDLL));
    }
}

/******************************************************************************
;DefineVtableGap

When embedding interop types, we skip unreferenced interface members in order
to avoid pulling in otherwise-unreferenced types found on the unused methods' 
signatures. In order to help the CLR keep the local type's vtable lined up
with the original type's vtable, we will emit special gap fillers to count off
the number of omitted members. These gap-fillers take the form of specially
named methods. These methods have no parameters or special attributes, and are
never referenced by name, so there is no reason to create a BCSYM_Proc for 
them; we will just emit the appropriate metadata directly.
******************************************************************************/
void MetaEmit::DefineVtableGap
(
 BCSYM_Container *pContainer,
 unsigned int vtableGapIndex,
 unsigned int vtableGapSize
)
{
    AssertIfNull(pContainer);
    AssertIfFalse(TypeHelpers::IsEmbeddableInteropType(pContainer));
    
    StringBuffer gapName;
    gapName.AppendPrintf(L"_VtblGap%d_%d", vtableGapIndex, vtableGapSize);
    STRING *gapNameStr = m_pCompiler->AddString(gapName.GetString());

    // From IMetaDataEmit::DefineMethod documentation (http://msdn.microsoft.com/en-us/library/ms230861(VS.100).aspx)
    // ----------------------
    // In the case where one or more slots need to be skipped, such as to preserve parity with a COM interface layout, 
    // a dummy method is defined to take up the slot or slots in the v-table; set the dwMethodFlags to the mdRTSpecialName 
    // value of the CorMethodAttr enumeration and specify the name as:
    //
    // _VtblGap<SequenceNumber><_CountOfSlots>
    //
    // where SequenceNumber is the sequence number of the method and CountOfSlots is the number of slots to skip in the v-table. 
    // If CountOfSlots is omitted, 1 is assumed.
    // ----------------------
    DWORD dwImplFlags = miRuntime | miManaged;
    DWORD dwMethodFlags = mdPublic | mdRTSpecialName; 
    mdMethodDef methdefBody = mdTokenNil;
    Signature signature;
    StartNewSignature(&signature);
    // See MetaEmit::EncodeMethodSignature: a method signature consists of the
    // calling convention, the number of parameters, and the method return type.
    EncodeInteger(IMAGE_CEE_CS_CALLCONV_DEFAULT);
    EncodeInteger(0);
    EncodeInteger(ELEMENT_TYPE_VOID);
    IfFailThrow(m_pmdEmit->DefineMethod(GetTypeDef(pContainer),
                                        gapNameStr,
                                        dwMethodFlags,
                                        GetSignature(),
                                        GetSigSize(),
                                        0,
                                        dwImplFlags,
                                        &methdefBody));
    ReleaseSignature(&signature);
}

//============================================================================
// Write out meta data for a com class interface method
//============================================================================
void MetaEmit::DefineComClassInterfaceMethod
(
    BCSYM_NamedRoot *pnamed,
    mdTypeDef       typedefInterface
)
{
    mdMethodDef       methdefBody;
    STRING        *   pstrName;
    BCSYM_Param   *   pparam;
    mdParamDef        paramdef;
    ULONG             ulParamSeq = 1;
    unsigned          ulen=0;
    Signature         signature;
    BCSYM_Proc  *     pproc;

    // Ignore properties. We'll emit them separately.
    if (pnamed->IsProperty())
    {
        return;
    }

    // Use the Proc for methods and events
    pproc = pnamed->DigThroughAlias()->PProc();
    pstrName = pproc->GetEmittedName();

    //
    // Set up the flags.
    //

    DWORD dwMethodFlags = mdAbstract | mdVirtual | mdCheckAccessOnOverride;
    DWORD dwImplFlags = miIL | miManaged;

    VSASSERT(!pproc->IsShared(), "com class members cannot be shared");

    // If there is a shadowing method between an overriding method and the
    // method it overrides, we have to emit it as a newslot and then emit
    // an explicit methodimpl. If we don't, we'll end up overriding the shadowed
    // method.
    if ((!pproc->IsOverridesKeywordUsed() &&
         !pproc->IsNotOverridableKeywordUsed() &&
         !pproc->IsOverrides()) ||
        (pproc->IsOverrides() &&
         pproc->OverridesRequiresMethodImpl()))
    {
        dwMethodFlags |= mdNewSlot;
    }

    if (pproc->IsSpecialName())
    {
        dwMethodFlags |= mdSpecialName;
    }

    // Set the access.
    VSASSERT(pproc->GetAccess() == ACCESS_Public, "com class members must be public");
    dwMethodFlags |= mdPublic;

    //
    // Do the emitting.
    //

    // Emit the method.
    VSASSERT(!IsNilToken(typedefInterface), "com class interface has to have already been defined!");

    StartNewSignature(&signature);
    EncodeMethodSignature(pproc, NULL);

    IfFailThrow(m_pmdEmit->DefineMethod(typedefInterface,
                                        pstrName,
                                        dwMethodFlags,
                                        GetSignature(),
                                        GetSigSize(),
                                        0,
                                        dwImplFlags,
                                        &methdefBody));

    // Save the method def on the actual named root that was passed in --
    // not on the invoke method.
    if (m_pBuilder->IsPEBuilder())
    {
        Symbols::SetComClassToken(pproc, methdefBody);
    }

    ReleaseSignature(&signature);

#if IDE 
    // Remember this token so we can delete it on next decompile
    m_pBuilder->CacheMetaToken(pproc, methdefBody);
#endif

    // iterating through each parameter to generate parameter property

    if (pproc->GetCompilerType() != NULL)
    {
        paramdef = mdParamDefNil;

        if (pproc->GetReturnTypeParam())
        {
            IfFailThrow(m_pmdEmit->DefineParam(methdefBody,
                                               0,      // (which param) retval == 0
                                               NULL,   // Optional param name
                                               0,      // Optional param falgs
                                               ELEMENT_TYPE_VOID,
                                               NULL,   // pValue
                                               0,      // length of pValue in bytes
                                               &paramdef));

            VSASSERT(paramdef != mdParamDefNil, "");

            if (pproc->GetReturnTypeParam())
            {
                if (m_pBuilder->IsPEBuilder())
                {
                    Symbols::SetComClassToken(pproc->GetReturnTypeParam(), paramdef);
                }
            }
        }
    }

    // Go through each param
    for (pparam = pproc->GetFirstParam();
         pparam;
         pparam = pparam->GetNext(), ulParamSeq++)
    {
        BCSYM_Expression  * pbcexpr = NULL;
        DWORD     dwConst = ELEMENT_TYPE_VOID;
        void    * pConst = NULL;
        ULONG     cbConst = 0;

        if (pparam->IsParamWithValue())
        {
            pbcexpr = pparam->PParamWithValue()->GetExpression();
        }

        if (pbcexpr)
        {
            // parameter has default value
            ConstantValue DefaultValue = pbcexpr->GetValue();
            ConvertConstant(&DefaultValue, &dwConst, &pConst, &cbConst);
        }

        // Set the flags.
        DWORD dwParamFlags = 0;

        if (pparam->IsOptional())
        {
            dwParamFlags |= pdOptional;
        }

        // Define the metadata.
        IfFailThrow(m_pmdEmit->DefineParam(methdefBody,
                                           ulParamSeq,
                                           pparam->GetName(),
                                           dwParamFlags,
                                           dwConst,
                                           pConst,
                                           cbConst,    // length of pConst in bytes
                                           &paramdef));

        VSASSERT(!IsNilToken(paramdef), "Bad COM+ output.");

        if (m_pBuilder->IsPEBuilder())
        {
            Symbols::SetComClassToken(pparam, paramdef);
        }
    }
}


void MetaEmit::DefineComClassInterfaceProperty
(
    BCSYM_NamedRoot *pnamed,
    mdTypeDef       typedefInterface
)
{
    STRING*         pstrName;
    mdMethodDef     methdefGet = mdMethodDefNil;
    mdMethodDef     methdefSet = mdMethodDefNil;
    mdProperty      prop;

    if (!pnamed->GetBindingSpace() || !pnamed->DigThroughAlias()->IsProperty())
    {
         return;
    }

    BCSYM_Property *pProperty = pnamed->DigThroughAlias()->PProperty();
    pstrName = pProperty->GetEmittedName();

    Signature sig;
    StartNewSignature(&sig);

    EncodePropertySignature(pProperty);

    mdMethodDef
        mdGet = mdMethodDefNil,
        mdSet = mdMethodDefNil;

    BCSYM_Proc *pGet = pProperty->GetProperty();

    if (pGet)
    {
        mdGet = (mdMethodDef)GetComClassToken(pGet);
    }

    BCSYM_Proc *pSet = pProperty->SetProperty();

    if (pSet)
    {
        mdSet = (mdMethodDef)GetComClassToken(pSet);
    }

    // Define the property.
    IfFailThrow(m_pmdEmit->DefineProperty(typedefInterface,     // [IN] the class/interface on which the property is being defined
                                          pstrName,             // [IN] Name of the property
                                          0,                    // [IN] CorPropertyAttr
                                          GetSignature(),       // [IN] the required type signature
                                          GetSigSize(),         // [IN] the size of the type signature blob
                                          ELEMENT_TYPE_VOID,    // [IN] flag for value type. selected ELEMENT_TYPE_*
                                          NULL,                 // [IN] constant value
                                          0,                    // [IN] size of constant value (string, in wide chars)
                                          mdSet,                // [IN] optional setter of the property
                                          mdGet,                // [IN] optional getter of the property
                                          NULL,                 // [IN] an optional array of other methods
                                          &prop));              // [OUT] output property token

    if (m_pBuilder->IsPEBuilder())
    {
        Symbols::SetComClassToken(pProperty, prop);
    }

#if IDE 
    // Remember this token so we can delete it on next decompile
    m_pBuilder->CacheMetaToken(pProperty,prop);
#endif

    ReleaseSignature(&sig);
}

void MetaEmit::DefineClassLayout
(
 BCSYM_Container *pContainer,
 mdTypeDef tkContainer,
 bool fExplicitLayout
 )
{
    AssertIfNull(pContainer);
    AssertIfFalse(pContainer->IsStruct());

    DWORD dwPackSize = 0;
    ULONG ulClassSize = 0;

    // Dev10 #702321
    bool fPackClassSize = pContainer->GetPWellKnownAttrVals()->GetPackClassSize(dwPackSize, ulClassSize);
    
    if (!fPackClassSize)
    {
        if (!fExplicitLayout)
        {
            return;
        }
    
        CComPtr<IMetaDataImport> pimdImport;

        // Dev10 #702321 Read pack and class sizes  from Metadata, by this time they could be set
        // through custom attributes.
        IfFailThrow(m_pmdEmit->QueryInterface(IID_IMetaDataImport, (void **)&pimdImport));

        ULONG cFieldOffset = 0;

        HRESULT hr = pimdImport->GetClassLayout(
            tkContainer,
            &dwPackSize,
            NULL,
            0,
            &cFieldOffset,
            &ulClassSize);

        if (hr != CLDB_E_RECORD_NOTFOUND)
        {
            IfFailThrow(hr);
        }
        else
        {
            // The metadata API expects us to specify 0 for the packing size and class size when using
            // explicit structure layout.
            dwPackSize = 0;
            ulClassSize = 0;
        }

        pimdImport.Release();
    }

    ULONG fieldCount = 0;

    if (fExplicitLayout)
    {
        DefineableMembersInAContainer countIter(m_pCompiler, pContainer);
        // Count up the field members which have explicit layout attributes.
        while (BCSYM *Symbol = countIter.Next())
        {
            if (Symbol->IsVariable())
            {
                fieldCount++;
            }
        }
    }

    // Reserve an extra slot for the list terminator.
    VBHeapPtr<COR_FIELD_OFFSET> offsets(fieldCount+1);
    ULONG fieldIndex = 0;

    if (fExplicitLayout)
    {
        DefineableMembersInAContainer fieldIter(m_pCompiler, pContainer);
        while (BCSYM *Symbol = fieldIter.Next())
        {
            if (Symbol->IsVariable())
            {
                int fieldOffset = 0;
                if (Symbol->PVariable()->GetPWellKnownAttrVals()->GetFieldOffsetData(&fieldOffset))
                {
                    AssertIfFalse(fieldIndex < fieldCount);
                    offsets[fieldIndex].ridOfField = GetToken(Symbol->PVariable());
                    offsets[fieldIndex].ulOffset = fieldOffset;
                    fieldIndex++;
                }
            }
        }
    }

    // It is not enough to terminate this array with a blank entry; we must use a special
    // magic nil constant, which is not actually equal to mdTokenNil, or SetClassLayout
    // will return an argument error.
    offsets[fieldIndex].ridOfField = mdFieldDefNil;
    offsets[fieldIndex].ulOffset = 0;

    if (fieldIndex > 0 || fPackClassSize)
    {
        IfFailThrow( m_pmdEmit->SetClassLayout(tkContainer, dwPackSize, offsets, ulClassSize));
    }
}

//============================================================================
// Write out meta data for a variable
//============================================================================
void MetaEmit::DefineField
(
    BCSYM_Variable *pvar
)
{
    mdFieldDef        flddef;
    STRING        *   pstrName = pvar->GetEmittedName();
    DWORD             dwConst = ELEMENT_TYPE_VOID;
    void            * pConst = NULL;
    ULONG             cbConst = 0;
    BCSYM_Expression * pexpr = NULL;
    Signature         signature;

    if (pvar->GetVarkind() != VAR_Const && pvar->GetVarkind() != VAR_Member)
    {
        VSASSERT(pvar->GetVarkind() != VAR_Param && pvar->GetVarkind(), "Can't have these.");
        return;
    }

    StartNewSignature(&signature);

    EncodeFieldSignature(pvar, NULL);

    if (pvar->GetVarkind() == VAR_Const)
    {
        pexpr = pvar->PVariableWithValue()->GetExpression();
    }

    if (pexpr)
    {
        // evaluate
        ConstantValue ConstValue = pexpr->GetValue();
        ConvertConstant(&ConstValue, &dwConst, &pConst, &cbConst);
    }

    // Set up the flags.
    DWORD dwFieldFlags = 0;

    if (pvar->GetVarkind() == VAR_Const)
    {
        if (pvar->GetType() == m_pCompilerProject->GetCompilerHost()->GetFXSymbolProvider()->GetType(t_decimal) ||
            pvar->GetType() == m_pCompilerProject->GetCompilerHost()->GetFXSymbolProvider()->GetType(t_date))
        {
            // Decimal and date constant fields are special because there is no
            // way to express their value in metadata. So other languages
            // don't barf, we make them initonly rather than literal
            dwFieldFlags |= fdInitOnly;
        }
        else
        {
            dwFieldFlags |= fdLiteral;
        }

        dwFieldFlags |= fdStatic;
    }
    else
    {
        if (pvar->IsShared())
        {
            dwFieldFlags |= fdStatic;
        }
    }

    if (pvar->IsReadOnly())
    {
        dwFieldFlags |= fdInitOnly;
    }

    switch(pvar->GetAccess())
    {
    case ACCESS_Public:
        dwFieldFlags |= fdPublic;
        break;

    case ACCESS_Protected:
        dwFieldFlags |= fdFamily;
        break;

    case ACCESS_Private:
        dwFieldFlags |= fdPrivate;
        break;

    case ACCESS_Friend:
        dwFieldFlags |= fdAssembly;
        break;

    case ACCESS_ProtectedFriend:
        dwFieldFlags |= fdFamORAssem;
        break;

    default:
        VSFAIL("Unexpected access.");
    }

    if ( pvar->IsStaticLocalBackingField() && pvar->PStaticLocalBackingField()->GetProcDefiningStatic() != NULL )
    {
        dwFieldFlags |= fdSpecialName; // mark backing fields for statics so they don't show up in intellisense, etc.

        // This is a backing field for a static local.  Munge the name using the signature of the function in which it was defined
        // so the debugger can ---- the name
        Signature BackupCurrentSig;
        StartNewSignature(&BackupCurrentSig);

        BCSYM_Proc *ProcDefiningStatic = pvar->PStaticLocalBackingField()->GetProcDefiningStatic();
        EncodeMethodSignature(ProcDefiningStatic, NULL);

        StringBuffer EncodedSig;
        for (unsigned i=0; i < m_cbSignature; i++)
        {
            WCHAR HexDigits[3]; // e.g. "FF" + trailing NULL
            IfFailThrow(StringCchPrintfW(HexDigits, DIM(HexDigits), L"%X", m_pbSignature[i]));

            EncodedSig.AppendString( HexDigits );
        }

        STRING *ProcName;
        if ( ProcDefiningStatic->IsAnyConstructor())
        {
            ProcName = ProcDefiningStatic->IsShared() ? STRING_CONST( m_pCompiler, SharedConstructor ) : STRING_CONST( m_pCompiler, Constructor );
        }
        else
        {
            ProcName = ProcDefiningStatic->GetName();
        }

        pstrName = m_pCompiler->ConcatStrings(VBStaticPrefix,
                                              ProcName,
                                              VBStaticDelimeter,
                                              EncodedSig.GetString(),
                                              VBStaticDelimeter,
                                              pstrName);

        ReleaseSignature(&BackupCurrentSig);
    }

    // Emit the metadata.
    IfFailThrow(m_pmdEmit->DefineField(GetTypeDef(pvar->GetContainer()),
                                       pstrName,
                                       dwFieldFlags,
                                       GetSignature(),
                                       GetSigSize(),
                                       dwConst,
                                       pConst,
                                       cbConst,        // length of pConst in bytes.  For ELEMENT_TYPE_STRING it's the length of the string
                                       &flddef));

    ReleaseSignature(&signature);

    PCCOR_SIGNATURE marshalAsType;
    ULONG cbMarshalAsType = 0;
    if (pvar->GetPWellKnownAttrVals()->GetMarshalAsData(&marshalAsType, &cbMarshalAsType))
    {
        IfFailThrow(m_pmdEmit->SetFieldMarshal(flddef, marshalAsType, cbMarshalAsType));
    }

    if (m_pBuilder->IsPEBuilder())
    {
        SetToken(pvar, flddef);
    }

#if IDE 
    // Remember this token so we can delete it on next decompile
    m_pBuilder->CacheMetaToken(pvar,flddef);
#endif
}


//============================================================================
// Write out meta data for an event
//============================================================================
void MetaEmit::DefineEvent
(
    BCSYM_EventDecl *pEvent
)
{
    mdEvent mdEvent;
    mdTypeRef mdDelegateType;

    mdDelegateType = DefineTypeRefBySymbol(pEvent->GetDelegate(), pEvent->GetLocation());

    IfFailThrow(m_pmdEmit->DefineEvent(GetTypeDef(pEvent->GetContainer()),
                                       pEvent->GetEmittedName(),
                                       0,
                                       mdDelegateType,
                                       GetToken(pEvent->GetProcAdd()),
                                       GetToken(pEvent->GetProcRemove()),
                                       pEvent->GetProcFire() ?
                                            GetToken(pEvent->GetProcFire()) :
                                            mdMethodDefNil,
                                       NULL,
                                       &mdEvent));

    if (m_pBuilder->IsPEBuilder())
    {
        SetToken(pEvent, mdEvent);
    }

#if IDE 
    // Remember this token so we can delete it on next decompile
    m_pBuilder->CacheMetaToken(pEvent,mdEvent);
#endif
}

//============================================================================
// Write out meta data for a property.
//============================================================================
void MetaEmit::DefineProperty
(
    BCSYM_Property *pProperty
)
{
    mdProperty      prop;
    mdMethodDef     mdGet = mdMethodDefNil, mdSet = mdMethodDefNil;
    Signature       sig;

    StartNewSignature(&sig);
    EncodePropertySignature(pProperty);
    BCSYM_Proc *pGet = pProperty->GetProperty();

    if (pGet)
    {
        mdGet = GetToken(pGet);
    }

    BCSYM_Proc *pSet = pProperty->SetProperty();

    if (pSet)
    {
        mdSet = GetToken(pSet);
    }

    // Define the property.
    IfFailThrow(m_pmdEmit->DefineProperty(GetTypeDef(pProperty->GetContainer()),// [IN] the class/interface on which the property is being defined
                                          pProperty->GetEmittedName(), // [IN] Name of the property
                                          0,                    // [IN] CorPropertyAttr
                                          GetSignature(),       // [IN] the required type signature
                                          GetSigSize(),         // [IN] the size of the type signature blob
                                          ELEMENT_TYPE_VOID,    // [IN] flag for value type. selected ELEMENT_TYPE_*
                                          NULL,                 // [IN] constant value
                                          0,                    // [IN] size of constant value (string, in wide chars)
                                          mdSet,                // [IN] optional setter of the property
                                          mdGet,                // [IN] optional getter of the property
                                          NULL,                 // [IN] an optional array of other methods
                                          &prop));              // [OUT] output property token

    if (m_pBuilder->IsPEBuilder())
    {
        SetToken(pProperty, prop);
    }

#if IDE 
    // Remember this token so we can delete it on next decompile
    m_pBuilder->CacheMetaToken(pProperty,prop);
#endif

    ReleaseSignature(&sig);
}

//========================================================================
// Hook up a method that implements part of an interface with the
// interface member it implements.
//========================================================================

void MetaEmit::DefineInterfaceImplementation
(
    BCSYM_Proc *pProc
)
{
    mdTypeDef tkTypeDef = (mdTypeDef)GetTypeDef(pProc->GetContainer());

    // Set implementation if we're implementing a com interface
    if (pProc->IsImplementingComClassInterface())
    {
        if (pProc->IsMethodImpl())
        {
            IfFailThrow(m_pmdEmit->DefineMethodImpl(
                                    tkTypeDef,
                                    GetToken(pProc),
                                    GetComClassToken(pProc)));
        }
        else if (pProc->IsProperty())
        {
            BCSYM_Property *pProperty = pProc->PProperty();

            if (pProperty->GetProperty() && GetComClassToken(pProperty->GetProperty()))
            {
                IfFailThrow(m_pmdEmit->DefineMethodImpl(
                                        tkTypeDef,
                                        GetToken(pProperty->GetProperty()),
                                        GetComClassToken(pProperty->GetProperty())));
            }

            if (pProperty->SetProperty() && GetComClassToken(pProperty->SetProperty()))
            {
                IfFailThrow(m_pmdEmit->DefineMethodImpl(
                                            tkTypeDef,
                                            GetToken(pProperty->SetProperty()),
                                            GetComClassToken(pProperty->SetProperty())));
            }
        }
    }

    BCSYM_ImplementsList *pImplements = pProc->GetImplementsList();

    // Set implementation if we're implementing another interface
    for (; pImplements; pImplements = pImplements->GetNext())
    {
        BCSYM_Proc  * pprocImplement = pImplements->GetImplementedMember()->PProc();
        BCSYM_GenericBinding *pGenericBinding = pImplements->GetGenericBindingContext();

        if (pProc->IsMethodImpl() || pProc->IsSyntheticMethod())
        {
            IfFailThrow(m_pmdEmit->DefineMethodImpl(tkTypeDef,
                                                    GetToken(pProc),
                                                    DefineMemberRefBySymbol(pprocImplement, pGenericBinding, pProc->GetLocation())));
        }
        else if (pProc->IsProperty())
        {
            BCSYM_Property *pProperty = pProc->PProperty();
            BCSYM_Property *pImplementedProperty = pprocImplement->PProperty();

            if (pProperty->GetProperty())
            {
                IfFailThrow(m_pmdEmit->DefineMethodImpl(tkTypeDef,
                                                        GetToken(pProperty->GetProperty()),
                                                        DefineMemberRefBySymbol(pImplementedProperty->GetProperty(), pGenericBinding, pProperty->GetLocation())));
            }

            if (pProperty->SetProperty())
            {
                IfFailThrow(m_pmdEmit->DefineMethodImpl(tkTypeDef,
                                                        GetToken(pProperty->SetProperty()),
                                                        DefineMemberRefBySymbol(pImplementedProperty->SetProperty(), pGenericBinding, pProperty->GetLocation())));
            }
        }
        else
        {
            BCSYM_EventDecl *pEvent = pProc->PEventDecl();
            BCSYM_EventDecl *pImplementedEvent = pprocImplement->PEventDecl();

            if (pEvent->GetProcAdd())
            {
                IfFailThrow(m_pmdEmit->DefineMethodImpl(tkTypeDef,
                                                        GetToken(pEvent->GetProcAdd()),
                                                        DefineMemberRefBySymbol(pImplementedEvent->GetProcAdd(), pGenericBinding, pEvent->GetLocation())));
            }

            if (pEvent->GetProcRemove())
            {
                IfFailThrow(m_pmdEmit->DefineMethodImpl(tkTypeDef,
                                                        GetToken(pEvent->GetProcRemove()),
                                                        DefineMemberRefBySymbol(pImplementedEvent->GetProcRemove(), pGenericBinding, pEvent->GetLocation())));
            }

            // Note the event fire method is always private and does not implement any interface method
        }
    }


    // If there is a shadowing method between an overriding method and the
    // method it overrides, we have to emit it as a newslot and then emit
    // an explicit methodimpl. If we don't, we'll end up overriding the shadowed
    // method.
    if (pProc->IsOverrides() && pProc->OverridesRequiresMethodImpl())
    {
        Symbols symbols(m_pCompiler, &m_nraScratch, NULL);

        BCSYM_GenericBinding *pGenericBinding = DeriveGenericBindingForMemberReference(
            pProc->GetContainer(),
            pProc->OverriddenProc(),
            symbols,
            m_pCompilerProject->GetCompilerHost());

        IfFailThrow(m_pmdEmit->DefineMethodImpl(tkTypeDef,
                                                GetToken(pProc),
                                                DefineMemberRefBySymbol(pProc->OverriddenProc(), pGenericBinding, pProc->GetLocation())));
    }
}

//****************************************************************************
// Method references
//****************************************************************************

//========================================================================
// Define a reference to a method from a symbol (which has a method def)
//========================================================================

mdMemberRef MetaEmit::DefineMemberRefBySymbol
(
    BCSYM_NamedRoot *pnamed,
    BCSYM_GenericBinding *pBinding,
    Location *pReferencingLocation,  // the location in the code that is referring to this symbol.
                                     // Used for squiggle location if an error occurs when referring to this symbol.
    ErrorTable *pAlternateErrorTable // Alternate error table, could be NULL. Useful for reporting errors in different files for partial classes.
)
{
    // Backup the default error table so that it can be restored later.
    ErrorTable *pOrigErrorTable = m_pBuilder->m_pErrorTable;

    if (pAlternateErrorTable)
    {
        m_pBuilder->m_pErrorTable = pAlternateErrorTable;
    }

    mdMemberRef tkMemberRef;

    pnamed = pnamed->DigThroughAlias()->PNamedRoot();

    // If this is a fake member added to implement some Windows Runtime interface then
    // get the original member (and the generic binding) from the interface.
    if (pnamed &&
        pnamed->IsProc() &&
        pnamed->PProc()->IsFakeWindowsRuntimeMember())
    {
        pBinding = pnamed->PProc()->GetImplementsList()->GetGenericBindingContext();
        pnamed = pnamed->PProc()->GetImplementsList()->GetImplementedMember();
    }

    if (pBinding)
    {
        // This is a member of an instantiation of a generic type, or an instantiation of
        // a generic method.

        BCSYM_GenericTypeBinding *TypeBinding =
            pBinding->IsGenericTypeBinding() ?
                pBinding->PGenericTypeBinding() :
                pBinding->GetParentBinding();

        if (TypeBinding)
        {
            // This is a member of an instantation of a generic type.

            Signature signature;
            StartNewSignature(&signature);

            mdTypeRef mdTypeRefParent = DefineTypeRefBySymbol(TypeBinding, pReferencingLocation);

            bool old_m_TrackReferrencesToEmbeddableInteropTypes = m_TrackReferrencesToEmbeddableInteropTypes;
            bool old_m_ReferredToEmbeddableInteropType = m_ReferredToEmbeddableInteropType;

#if IDE 
            bool save_m_TrackUnsubstitutedLocalTypes = m_pCompilerProject->GetTrackUnsubstitutedLocalTypes();
            ThrowIfTrue(save_m_TrackUnsubstitutedLocalTypes);
            ThrowIfFalse(m_pCompilerProject->m_UnsubstitutedLocalTypes.Count() == 0);

            m_pCompilerProject->SetTrackUnsubstitutedLocalTypes(true);
#endif

            ThrowIfTrue(m_TrackReferrencesToEmbeddableInteropTypes); // Shouldn't be getting into a nested case.

            m_TrackReferrencesToEmbeddableInteropTypes = true;
            m_ReferredToEmbeddableInteropType = false;

            // Dev10 #751770
            bool* save_m_pReferredToEmbeddableInteropType = m_pCompilerProject->Get_pReferredToEmbeddableInteropType();
            ThrowIfTrue(save_m_pReferredToEmbeddableInteropType); // Shouldn't be getting into a nested case.
            m_pCompilerProject->Set_pReferredToEmbeddableInteropType(&m_ReferredToEmbeddableInteropType);

            if (TypeHelpers::IsEmbeddableInteropType(pnamed->GetParent()))
            {
                m_pCompilerProject->CachePiaTypeMemberRef(pnamed, pReferencingLocation, m_pBuilder->m_pErrorTable);
                m_ReferredToEmbeddableInteropType = true;
            }

            EncodeMetaMemberRef(pnamed, mdTypeRefParent);

            if (pnamed->IsProc())
            {
                EncodeMethodSignature(pnamed->PProc(), pReferencingLocation);
            }
            else if (pnamed->IsMember())
            {
                EncodeFieldSignature(pnamed->PMember(), pReferencingLocation);
            }

            EncodeFinishMetaMemberRef();

            // Get the reference.
            tkMemberRef = DefineMemberRefByName(GetMetaMemberRef(), GetSigSize(), pnamed, mdTypeRefParent,
                                            !m_ReferredToEmbeddableInteropType); // Dev10 #676210: Can't use DefineImportMember when embeddable type is in the picture

            m_TrackReferrencesToEmbeddableInteropTypes = old_m_TrackReferrencesToEmbeddableInteropTypes;
            m_ReferredToEmbeddableInteropType = old_m_ReferredToEmbeddableInteropType;

#if IDE 
            m_pCompilerProject->m_UnsubstitutedLocalTypes.Clear();
            m_pCompilerProject->SetTrackUnsubstitutedLocalTypes(save_m_TrackUnsubstitutedLocalTypes);
#endif

            m_pCompilerProject->Set_pReferredToEmbeddableInteropType(save_m_pReferredToEmbeddableInteropType);

            ReleaseSignature(&signature);

            // Remember this token so we can delete it on next decompile
            // 
        }
        else
        {
            tkMemberRef = DefineMemberRefBySymbol(pnamed, NULL, pReferencingLocation);
        }

        if (pnamed->IsGeneric() && !pBinding->IsGenericTypeBinding())
        {
            // This is an instantiation of a generic method.

            tkMemberRef = DefineGenericMethodInstantiation(pnamed, pBinding, tkMemberRef, pReferencingLocation);
        }
    }
    else if (m_pBuilder->m_pCompilerProject == pnamed->GetContainingProject() && pBinding == NULL)
    {
        VSASSERT(GetToken(pnamed), "Must be defined.");

        tkMemberRef = (mdMemberRef)GetToken(pnamed);
    }
    else
    {
        Signature signature;

        StartNewSignature(&signature);

        // 





        BCSYM_Container * pParent = pnamed->GetParent()->PContainer();
        mdTypeRef mdTypeRefParent = DefineTypeRefBySymbol(pParent, pReferencingLocation);

        bool old_m_TrackReferrencesToEmbeddableInteropTypes = m_TrackReferrencesToEmbeddableInteropTypes;
        bool old_m_ReferredToEmbeddableInteropType = m_ReferredToEmbeddableInteropType;

#if IDE 
        bool save_m_TrackUnsubstitutedLocalTypes = m_pCompilerProject->GetTrackUnsubstitutedLocalTypes();
        ThrowIfTrue(save_m_TrackUnsubstitutedLocalTypes);
        ThrowIfFalse(m_pCompilerProject->m_UnsubstitutedLocalTypes.Count() == 0);

        m_pCompilerProject->SetTrackUnsubstitutedLocalTypes(true);
#endif

        ThrowIfTrue(m_TrackReferrencesToEmbeddableInteropTypes); // Shouldn't be getting into a nested case.

        m_TrackReferrencesToEmbeddableInteropTypes = true;
        m_ReferredToEmbeddableInteropType = false;

        // Dev10 #751770
        bool* save_m_pReferredToEmbeddableInteropType = m_pCompilerProject->Get_pReferredToEmbeddableInteropType();
        ThrowIfTrue(save_m_pReferredToEmbeddableInteropType); // Shouldn't be getting into a nested case.
        m_pCompilerProject->Set_pReferredToEmbeddableInteropType(&m_ReferredToEmbeddableInteropType);

        if (TypeHelpers::IsEmbeddableInteropType(pParent))
        {
            m_pCompilerProject->CachePiaTypeMemberRef(pnamed, pReferencingLocation, m_pBuilder->m_pErrorTable);
            m_ReferredToEmbeddableInteropType = true;

            // Dev10 #697784 This must be a TypeDef token for one of our embeddable (PIA) types,
            // we need to use corresponding TypeRef token. According to CLR folks, it would be
            // a violation to parent a MemberRef to a TypeDef.
            ThrowIfFalse(TypeFromToken(mdTypeRefParent) == mdtTypeDef);
            ThrowIfFalse(m_pBuilder->IsPEBuilder());
            
            PEBuilder *pPEBuilder = (PEBuilder*)m_pBuilder;
            PEBuilder::PiaTypeDefInfo info;

            ThrowIfFalse(pPEBuilder->m_NoPiaTypeDefToTypeRefMap.GetValue(pParent, &info));
            
            if (IsNilToken(info.m_TypeRef))
            {
                // We must use the metadata importer to locate the containing module.
                CComPtr<IMetaDataImport2> pMDImport = NULL;
                mdTypeRef tkContainer;
                
                IfFailThrow(m_pmdEmit->QueryInterface(IID_IMetaDataImport2, (void **)&pMDImport));
                IfFailThrow(pMDImport->GetModuleFromScope(&tkContainer));
                VSASSERT(!IsNilToken(tkContainer), "Bad COM+ output!");

                pMDImport.Release();

                STRING *pstrTypeName =
                    pParent->GetContainer()->IsNamespace() ?
                        ConcatNameSpaceAndName(m_pCompiler, pParent->GetNameSpace(), pParent->GetEmittedName()) :
                        pParent->GetEmittedName();

                IfFailThrow(m_pmdEmit->DefineTypeRefByName(tkContainer,
                                                        pstrTypeName,
                                                        &mdTypeRefParent));

                info.m_TypeRef = mdTypeRefParent;
                pPEBuilder->m_NoPiaTypeDefToTypeRefMap.SetValue(pParent, info);
            }
            else
            {
                mdTypeRefParent = info.m_TypeRef;
            }
        }

        EncodeMetaMemberRef(pnamed, mdTypeRefParent);

        if (pnamed->IsProc())
        {
            EncodeMethodSignature(pnamed->PProc(), pReferencingLocation);
        }
        else
        {
            EncodeFieldSignature(pnamed->PMember(), pReferencingLocation);
        }

        EncodeFinishMetaMemberRef();

        // Get the reference.
        tkMemberRef = DefineMemberRefByName(GetMetaMemberRef(), GetSigSize(), pnamed, mdTypeRefParent, 
                                            !m_ReferredToEmbeddableInteropType); // Dev10 #676210: Can't use DefineImportMember when embeddable type is in the picture

        m_TrackReferrencesToEmbeddableInteropTypes = old_m_TrackReferrencesToEmbeddableInteropTypes;
        m_ReferredToEmbeddableInteropType = old_m_ReferredToEmbeddableInteropType;
        
#if IDE 
        m_pCompilerProject->m_UnsubstitutedLocalTypes.Clear();
        m_pCompilerProject->SetTrackUnsubstitutedLocalTypes(save_m_TrackUnsubstitutedLocalTypes);
#endif

        m_pCompilerProject->Set_pReferredToEmbeddableInteropType(save_m_pReferredToEmbeddableInteropType);

        ReleaseSignature(&signature);
    }

    // Restore the default error table.
    m_pBuilder->m_pErrorTable = pOrigErrorTable;

    return tkMemberRef;
}

//============================================================================
// Define a runtime member reference
//============================================================================

mdMemberRef MetaEmit::DefineRTMemberRef
(
    RuntimeMembers rtHelper,
    ErrorTable *pErrorTable,
    Location *pErrorLocation
)
{
    RuntimeMemberDescriptor * prtdesc = &(g_rgRTLangMembers[rtHelper]);
    mdMemberRef tkMemberRef;
    BCSYM_NamedRoot *pRuntimeMember;

    VerifyRTLangEnum(rtHelper);

#if DEBUG // validate table names for runtime members match the symbol name
    // Force TargetLibrary to be different only the first time.
    static VBTargetLibraryType TargetLibrary = (VBTargetLibraryType) (m_pCompilerProject->GetCompilerHost()->GetVbLibraryType() - 1);

    // Verify that the library type hasn't changed since last time we were here.
    if (TargetLibrary != m_pCompilerProject->GetCompilerHost()->GetVbLibraryType())
    {
        // Remember TargetLibrary Type
        TargetLibrary = m_pCompilerProject->GetCompilerHost()->GetVbLibraryType();
        RuntimeVersion rtVersion = m_pCompilerProject->GetCompilerHost()->GetRuntimeVersion();

        for (unsigned i = 1; i < (unsigned)MaxRuntimeMember; i++)
        {
            // UV Support: Verify only members that are supported for the specified target.
            if (g_rgRTLangMembers[i].TargetLibrary & TargetLibrary && g_rgRTLangMembers[i].VersionIntroduced <= rtVersion)
            {
#if !IDE 
                // These are private entries accessible only by the debugger when used in the IDE
                if (((RuntimeMembers)i != ArrayOfCharToStringMember) &&
                    ((RuntimeMembers)i != CharAndCountToStringMember) &&
                    ((RuntimeMembers)i != ArrayOfCharToStringMemberSubset))
                {
#endif !IDE

                if (rtVersion > RuntimeVersion1 &&
                   ((RuntimeMembers)i == ArrayOfCharToStringV1Member ||
                    (RuntimeMembers)i == CharAndCountToStringV1Member ||
                    (RuntimeMembers)i == ArrayOfCharToStringV1MemberSubset))
                {
                    continue;
                }

                pRuntimeMember = m_pCompilerProject->GetCompilerHost()->GetSymbolForRuntimeMember((RuntimeMembers)i, m_pCompilationCaches);
                if ( pRuntimeMember != NULL )
                {
                    VSASSERT(wcscmp(pRuntimeMember->GetName(), g_rgRTLangMembers[i].wszName) == 0, "Names don't match");
                }
#if !IDE 
                }
#endif !IDE
            }
        }
    }
#endif // DEBUG

    if (rtHelper >= 0 &&
        rtHelper < _countof(m_pBuilder->m_rgRTMemRef))
    {
       if (m_pBuilder->m_rgRTMemRef[rtHelper] == mdTokenNil)
       {
            pRuntimeMember = m_pCompilerProject->GetCompilerHost()->GetSymbolForRuntimeMember(rtHelper, m_pCompilationCaches, pErrorTable, pErrorLocation);
            if ( pRuntimeMember == NULL )
            {
                return mdTokenNil; // we may not have a runtime or the runtime for the platform may not define this symbol.
            }
            // If we want to emit a call to a runtime get/set property, select it here.

            if (prtdesc->usFlags & RTF_GET_PROP)
            {
                pRuntimeMember = pRuntimeMember->PProperty()->GetProperty();
            }

            // Create and cache the memberref to the runtime helper
            tkMemberRef = DefineMemberRefBySymbol(pRuntimeMember, NULL, NULL);
            m_pBuilder->m_rgRTMemRef[ rtHelper ] = tkMemberRef;
       }
       return m_pBuilder->m_rgRTMemRef[rtHelper];
    }

    return mdTokenNil;
}

//========================================================================
// Caching metadataemit wrapper around "DefineMemberRef"
//
// !!! Note, this function does not perform member caching for NoPIA embedded types !!!
//========================================================================

mdMemberRef MetaEmit::DefineMemberRefByName
(
    _In_ MetaMemberRef *pmmr,
    size_t cbSizeRef,
    BCSYM_NamedRoot *psymNamed, // OPTIONAL
    mdTypeRef mdTypeRefParent,   // OPTIONAL
    bool canUseDefineImportMember // = true  - it is ok to use m_pmdEmit->DefineImportMember to create the reference
)
{
    mdMemberRef *pmrFound = NULL;

    //disable caching for static locals  (bug DevDiv 44120).
    //Consider to implement a caching mechanism based on the munged name
    if(psymNamed && !(psymNamed->IsStaticLocalBackingField()))
    {
        pmrFound = (mdMemberRef *)m_pBuilder->m_ProjectHashTable.FindWithSize(pmmr, cbSizeRef);
    }
    mdMemberRef mr;

    // The caller must either pass in both or neither of the optional params,
    // but never just one or the other
    VSASSERT((psymNamed == NULL) == IsNilToken(mdTypeRefParent),
        "Must pass in both or neither optional param, not a mixture");

    if (pmrFound)
    {
        mr = *pmrFound;
    }
    else
    {
        IMetaDataAssemblyImport *pimdAssemblyImport = NULL; // Not AddRef'ed
        CComPtr<IMetaDataAssemblyImport> srpAssemblyImportToRelease; // don't use this, exists only for releasing correctly
        CompilerProject *pproj = NULL;

        // Need to ensure that in the ENC cases, the assembly import is got from the ENC Builder
        //
        if (psymNamed)
        {
            pproj = psymNamed->GetContainingProject();

#if IDE
            if (m_pBuilder &&
                m_pBuilder->IsENCBuilder() &&
                !pproj->IsMetaData())
            {
                if (pproj->m_pENCBuilder && pproj->m_pENCBuilder->m_pmdEmit)
                {
                    IfFailThrow(
                        pproj->m_pENCBuilder->m_pmdEmit->QueryInterface(IID_IMetaDataAssemblyImport, (void **)&srpAssemblyImportToRelease));
                    pimdAssemblyImport = srpAssemblyImportToRelease;
                }
            }
            else
#endif IDE
            {
                pimdAssemblyImport = pproj->GetAssemblyImport();
            }
        }

        bool generateImportRef = pimdAssemblyImport && canUseDefineImportMember;

        if (generateImportRef)
        {
            bool fImportFromCodeModule = pproj->IsCodeModule();

            IMetaDataImport *pimdImport = NULL;            // AddRef'ed
            IMetaDataAssemblyEmit *pimdAssemblyEmit = NULL; // AddRef'ed


            // If the symbol comes from a sub-module of a multi-file assembly, then get
            // the IMetaDataImport2 from the module. Otherwise, just get it from the project.
            CompilerFile *pfile = psymNamed->GetContainingCompilerFile();
            if (pproj->IsMetaData() && pfile->IsMetaDataFile() &&
                (pproj->GetFileName() != pfile->GetFileName()))
            {
                pimdImport = pfile->PMetaDataFile()->GetImport();
                IfNullThrow(pimdImport);
                pimdImport->AddRef();
            }
            else
            {
                IfFailThrow(pimdAssemblyImport->QueryInterface(IID_IMetaDataImport, (void **)&pimdImport));
            }

            if (FEmitAssembly())
            {
                IfFailThrow(m_pmdEmit->QueryInterface(IID_IMetaDataAssemblyEmit, (void **)&pimdAssemblyEmit));
            }

            TypeForwardersResolver resolver(&MetaEmit::ResolveTypeForwarders, m_pCompiler, psymNamed->GetContainingCompilerFile());

            // Define the reference.
            IfFailThrow(m_pMetaEmitHelper->DefineImportMember(
                fImportFromCodeModule ? NULL : pimdAssemblyImport,     // [IN] Assemby containing the Member.
                NULL,                   // [IN] Hash Blob for Assembly.
                0,                      // [IN] Count of bytes.
                pimdImport,             // [IN] Import scope, with member.
                GetToken(psymNamed),    // [IN] Member in import scope.
                fImportFromCodeModule ? NULL : pimdAssemblyEmit,       // [IN] Assembly into which the Member is imported.
                mdTypeRefParent,        // [IN] Classref or classdef in emit scope.
                &resolver,              // [IN] Functor for resolving type forwarders.
                &mr));                  // [OUT] Put member ref here.

            RELEASE(pimdImport);
            RELEASE(pimdAssemblyEmit);
        }
        else
        {
            // Define the reference.
            IfFailThrow(m_pmdEmit->DefineMemberRef(pmmr->m_tr,
                                                   pmmr->m_pstrMemberName,
                                                   (COR_SIGNATURE *)pmmr->m_pSignature,
                                                   pmmr->m_cbSizeSig,
                                                   &mr));
        }

        //disable caching for static locals  (bug DevDiv 44120).
        //Consider to implement a caching mechanism based on the munged name
        if(psymNamed && !(psymNamed->IsStaticLocalBackingField()))
        {
            m_pBuilder->m_ProjectHashTable.AddWithSize(pmmr, cbSizeRef, mr);
        }

#if IDE
        if (m_pBuilder->IsENCBuilder())
        {
            static_cast<ENCBuilder *>(m_pBuilder)->NewProjectHashTableEntry(pmmr, cbSizeRef, &mr, sizeof(mr));
        }
#endif
    }

    ASSERT(!IsNilToken(mr), "Bogus token!");
    return mr;
}

//============================================================================
// Caching metadataemit wrapper around "GetTokenFromTypeSpec"
//============================================================================

mdTypeSpec
MetaEmit::DefineGenericTypeInstantiation
(
    BCSYM_GenericTypeBinding *pBinding,
    Location *pReferencingLocation
)
{
    Signature GenericSignature;
    StartNewSignature(&GenericSignature);

    BCSYM_NamedRoot *pGeneric = pBinding->GetGeneric();

    EncodeTypeInstantiation(pGeneric->GetEmittedName(), DefineTypeRefBySymbol(pGeneric, pReferencingLocation));

    EncodeType(pBinding, pReferencingLocation);

    EncodeFinishTypeInstantiation();

    mdTypeSpec TypeSpec;
    mdTypeSpec *CachedTypeSpec = (mdTypeSpec *)m_pBuilder->m_ProjectHashTable.FindWithSize(GetTypeInstantiation(), GetSigSize());

    if (CachedTypeSpec)
    {
        TypeSpec = *CachedTypeSpec;
    }
    else
    {
        IfFailThrow(m_pmdEmit->GetTokenFromTypeSpec(GetTypeInstantiation()->m_pSignature, GetTypeInstantiation()->m_cbSizeSig, &TypeSpec));

        m_pBuilder->m_ProjectHashTable.AddWithSize(GetTypeInstantiation(), GetSigSize(), TypeSpec);
#if IDE
        if (m_pBuilder->IsENCBuilder())
        {
            static_cast<ENCBuilder *>(m_pBuilder)->NewProjectHashTableEntry(GetTypeInstantiation(), GetSigSize(), &TypeSpec, sizeof(TypeSpec));
        }
#endif
    }

    ReleaseSignature(&GenericSignature);

    return TypeSpec;
}


mdMethodSpec
MetaEmit::DefineGenericMethodInstantiation
(
    BCSYM_NamedRoot *pMethod,
    BCSYM_GenericBinding *pBinding,
    mdMemberRef UnboundMemberRef,
    Location *pReferencingLocation
)
{
    Signature signature;
    StartNewSignature(&signature);

    EncodeMethodInstantiation(pMethod->GetEmittedName(), UnboundMemberRef);

    EncodeInteger(IMAGE_CEE_CS_CALLCONV_INSTANTIATION);
    EncodeInteger(pBinding->GetArgumentCount());

    EncodeGenericArguments(pBinding, 0, false, pReferencingLocation);

    EncodeFinishMethodInstantiation();

    mdMethodSpec MethodSpec;
    mdMethodSpec *CachedMethodSpec;

    CachedMethodSpec = (mdMethodSpec *)m_pBuilder->m_ProjectHashTable.FindWithSize(GetMethodInstantiation(), GetSigSize());

    if (CachedMethodSpec)
    {
        MethodSpec = *CachedMethodSpec;
    }
    else
    {
        IfFailThrow(m_pmdEmit->DefineMethodSpec(UnboundMemberRef, GetMethodInstantiation()->m_pSignature, GetMethodInstantiation()->m_cbSizeSig, &MethodSpec));

        m_pBuilder->m_ProjectHashTable.AddWithSize(GetMethodInstantiation(), GetSigSize(), MethodSpec);
#if IDE
        if (m_pBuilder->IsENCBuilder())
        {
            static_cast<ENCBuilder *>(m_pBuilder)->NewProjectHashTableEntry(GetMethodInstantiation(), GetSigSize(), &MethodSpec, sizeof(MethodSpec));
        }
#endif
    }

    ReleaseSignature(&signature);

    return MethodSpec;
}

//****************************************************************************
// Array methods.
//****************************************************************************

//============================================================================
// Caching metadataemit wrapper around "GetTokenFromTypeSpec"
//============================================================================

mdTypeSpec MetaEmit::DefineTypeSpec
(
    BCSYM_ArrayType *parray,
    Location *pReferencingLocation
)
{
    mdTypeSpec tkTypeSpec;
    ArrayValue *pValue;

    Signature signature;

    StartNewSignature(&signature);

    EncodeArrayKey();
    EncodeType(parray, pReferencingLocation);
    EncodeFinishArrayKey();

    pValue = (ArrayValue *)m_pBuilder->m_ProjectHashTable.FindWithSize(GetArrayKey(), GetSigSize());

    if (pValue && pValue->m_tkTypeSpec)
    {
        tkTypeSpec = pValue->m_tkTypeSpec;
    }
    else
    {
        if (!pValue)
        {
            ArrayValue value;
            memset(&value, 0, sizeof(value));

            pValue = (ArrayValue *)m_pBuilder->m_ProjectHashTable.AddWithSize(GetArrayKey(), GetSigSize(), value);
#if IDE
            if (m_pBuilder->IsENCBuilder())
            {
                static_cast<ENCBuilder *>(m_pBuilder)->NewProjectHashTableEntry(GetArrayKey(), GetSigSize(), &value, sizeof(value));
            }
#endif
        }

        IfFailThrow(m_pmdEmit->GetTokenFromTypeSpec((COR_SIGNATURE *)GetArrayKey()->m_pSignature,
                                                    GetArrayKey()->m_cbSizeSig,
                                                    &tkTypeSpec));

        pValue->m_tkTypeSpec = tkTypeSpec;
    }

    ReleaseSignature(&signature);

    return tkTypeSpec;
}

//============================================================================
// Define a member reference to the array constructor
//============================================================================

mdMemberRef MetaEmit::DefineArrayCtorRef
(
    BCSYM_ArrayType * parr,
    Location *pReferencingLocation
)
{
    mdMemberRef     tkMemberRef;

    unsigned        curDim;
    unsigned        cDims = parr->GetRank();
    mdTypeRef       tr;

    ArrayValue    * pValue;

    Signature       signature;

    StartNewSignature(&signature);

    EncodeArrayKey();
    EncodeType(parr, pReferencingLocation);
    EncodeFinishArrayKey();

    // Check the cache.
    pValue = (ArrayValue *)m_pBuilder->m_ProjectHashTable.FindWithSize(GetArrayKey(), GetSigSize());

    if (pValue && pValue->m_refs[ARRAY_Ctor])
    {
        tkMemberRef = pValue->m_refs[ARRAY_Ctor];
    }
    else
    {
        if (!pValue)
        {
            ArrayValue value;
            memset(&value, 0, sizeof(value));

            pValue = (ArrayValue *)m_pBuilder->m_ProjectHashTable.AddWithSize(GetArrayKey(), GetSigSize(), value);
#if IDE
            if (m_pBuilder->IsENCBuilder())
            {
                static_cast<ENCBuilder *>(m_pBuilder)->NewProjectHashTableEntry(GetArrayKey(), GetSigSize(), &value, sizeof(value));
            }
#endif
        }

        ReleaseSignature(&signature);
        StartNewSignature(&signature);

        // Get the structure.
        tr = DefineTypeRefBySymbol(parr, pReferencingLocation);

        EncodeMetaMemberRef(STRING_CONST(m_pCompiler, Constructor), tr);

        // Get the member.
        EncodeInteger(IMAGE_CEE_CS_CALLCONV_DEFAULT | IMAGE_CEE_CS_CALLCONV_HASTHIS);
        EncodeInteger(cDims);
        EncodeInteger(ELEMENT_TYPE_VOID);

        for (curDim = 0; curDim < cDims; curDim++)
        {
            EncodeInteger(ELEMENT_TYPE_I4);
        }

        EncodeFinishMetaMemberRef();

        // Get the value.
        tkMemberRef = DefineMemberRefByName(GetMetaMemberRef(), GetSigSize(), NULL, mdTypeRefNil);

        pValue->m_refs[ARRAY_Ctor] = tkMemberRef;
    }

    ReleaseSignature(&signature);

    return tkMemberRef;
}

//============================================================================
// Define a member reference to an array load
//============================================================================

mdMemberRef MetaEmit::DefineArrayLoadRef
(
    BCSYM_ArrayType * parr,
    unsigned cDims,
    Location *pReferencingLocation
)
{
    unsigned        curDim;
    BCSYM         * psymType;
    ArrayValue    * pValue;
    mdTypeRef       tr;
    mdMemberRef     tkMemberRef;

    Signature       signature;

    StartNewSignature(&signature);

    EncodeArrayKey();
    EncodeType(parr, pReferencingLocation);
    EncodeFinishArrayKey();

    // Check the cache.
    pValue = (ArrayValue *)m_pBuilder->m_ProjectHashTable.FindWithSize(GetArrayKey(), GetSigSize());

    if (pValue && pValue->m_refs[ARRAY_LoadRef])
    {
        tkMemberRef = pValue->m_refs[ARRAY_LoadRef];
    }
    else
    {
        if (!pValue)
        {
            ArrayValue value;
            memset(&value, 0, sizeof(value));

            pValue = (ArrayValue *)m_pBuilder->m_ProjectHashTable.AddWithSize(GetArrayKey(), GetSigSize(), value);
#if IDE
            if (m_pBuilder->IsENCBuilder())
            {
                static_cast<ENCBuilder *>(m_pBuilder)->NewProjectHashTableEntry(GetArrayKey(), GetSigSize(), &value, sizeof(value));
            }
#endif
        }

        ReleaseSignature(&signature);
        StartNewSignature(&signature);

        psymType = parr->GetRoot();

        // Get the structure.
        tr = DefineTypeRefBySymbol(parr, pReferencingLocation);
        EncodeMetaMemberRef(STRING_CONST(m_pCompiler, ArrayGet), tr);

        // Get the signature.
        EncodeInteger(IMAGE_CEE_CS_CALLCONV_DEFAULT | IMAGE_CEE_CS_CALLCONV_HASTHIS);
        EncodeInteger(cDims);
        EncodeType(psymType, pReferencingLocation);

        for (curDim = 0; curDim < cDims; curDim++)
        {
            EncodeInteger(ELEMENT_TYPE_I4);
        }

        EncodeFinishMetaMemberRef();

        // Get the value.
        tkMemberRef = DefineMemberRefByName(GetMetaMemberRef(),
                                            GetSigSize(),
                                            NULL, mdTypeRefNil);

        pValue->m_refs[ARRAY_LoadRef] = tkMemberRef;
    }

    ReleaseSignature(&signature);

    return tkMemberRef;
}

//============================================================================
// Define a member reference to an array load
//============================================================================

mdMemberRef MetaEmit::DefineArrayLoadAddrRef
(
    BCSYM_ArrayType * parr,
    unsigned cDims,
    Location *pReferencingLocation
)
{
    unsigned        curDim;
    BCSYM         * psymType;
    ArrayValue    * pValue;
    mdTypeRef       tr;
    mdMemberRef     tkMemberRef;

    Signature       signature;

    StartNewSignature(&signature);

    EncodeArrayKey();
    EncodeType(parr, pReferencingLocation);
    EncodeFinishArrayKey();

    // Check the cache.
    pValue = (ArrayValue *)m_pBuilder->m_ProjectHashTable.FindWithSize(GetArrayKey(), GetSigSize());

    if (pValue && pValue->m_refs[ARRAY_LoadAddrRef])
    {
        tkMemberRef = pValue->m_refs[ARRAY_LoadAddrRef];
    }
    else
    {
        if (!pValue)
        {
            ArrayValue value;
            memset(&value, 0, sizeof(value));

            pValue = (ArrayValue *)m_pBuilder->m_ProjectHashTable.AddWithSize(GetArrayKey(), GetSigSize(), value);
#if IDE
            if (m_pBuilder->IsENCBuilder())
            {
                static_cast<ENCBuilder *>(m_pBuilder)->NewProjectHashTableEntry(GetArrayKey(), GetSigSize(), &value, sizeof(value));
            }
#endif
        }

        ReleaseSignature(&signature);
        StartNewSignature(&signature);

        psymType = parr->GetRoot();

        // Get the structure.
        tr = DefineTypeRefBySymbol(parr, pReferencingLocation);
        EncodeMetaMemberRef(STRING_CONST(m_pCompiler, ArrayAddress), tr);

        // Get the signature.
        EncodeInteger(IMAGE_CEE_CS_CALLCONV_DEFAULT | IMAGE_CEE_CS_CALLCONV_HASTHIS);
        EncodeInteger(cDims);
        EncodeInteger(ELEMENT_TYPE_BYREF);
        EncodeType(psymType, pReferencingLocation);

        for (curDim = 0; curDim < cDims; curDim++)
        {
            EncodeInteger(ELEMENT_TYPE_I4);
        }

        EncodeFinishMetaMemberRef();

        // Get the value.
        tkMemberRef = DefineMemberRefByName(GetMetaMemberRef(),
                                            GetSigSize(),
                                            NULL, mdTypeRefNil);

        pValue->m_refs[ARRAY_LoadAddrRef] = tkMemberRef;
    }

    ReleaseSignature(&signature);

    return tkMemberRef;
}

//============================================================================
// Define a member reference to an array store
//============================================================================

mdMemberRef MetaEmit::DefineArrayStoreRef
(
    BCSYM_ArrayType * parr,
    unsigned cDims,
    Location *pReferencingLocation
)
{
    unsigned        curDim;
    BCSYM         * psymType;
    ArrayValue    * pValue;
    mdTypeRef       tr;
    mdMemberRef     tkMemberRef;

    Signature       signature;

    StartNewSignature(&signature);

    // Check the cache.
    EncodeArrayKey();
    EncodeType(parr, pReferencingLocation);
    EncodeFinishArrayKey();

    pValue = (ArrayValue *)m_pBuilder->m_ProjectHashTable.FindWithSize(GetArrayKey(), GetSigSize());

    if (pValue && pValue->m_refs[ARRAY_StoreRef])
    {
        tkMemberRef = pValue->m_refs[ARRAY_StoreRef];
    }
    else
    {
        if (!pValue)
        {
            ArrayValue value;
            memset(&value, 0, sizeof(value));

            pValue = (ArrayValue *)m_pBuilder->m_ProjectHashTable.AddWithSize(GetArrayKey(), GetSigSize(), value);
#if IDE
            if (m_pBuilder->IsENCBuilder())
            {
                static_cast<ENCBuilder *>(m_pBuilder)->NewProjectHashTableEntry(GetArrayKey(), GetSigSize(), &value, sizeof(value));
            }
#endif
        }

        ReleaseSignature(&signature);
        StartNewSignature(&signature);

        psymType = parr->GetRoot();

        // Get the structure.
        tr = DefineTypeRefBySymbol(parr, pReferencingLocation);
        EncodeMetaMemberRef(STRING_CONST(m_pCompiler, ArraySet), tr);

        // Get the signature.
        EncodeInteger(IMAGE_CEE_CS_CALLCONV_DEFAULT | IMAGE_CEE_CS_CALLCONV_HASTHIS);
        EncodeInteger(cDims + 1);
        EncodeInteger(ELEMENT_TYPE_VOID);

        for (curDim = 0; curDim < cDims; curDim++)
        {
            EncodeInteger(ELEMENT_TYPE_I4);
        }

        EncodeType(psymType, pReferencingLocation);

        EncodeFinishMetaMemberRef();

        // Get the value.
        tkMemberRef = DefineMemberRefByName(GetMetaMemberRef(),
                                            GetSigSize(),
                                            NULL, mdTypeRefNil);

        pValue->m_refs[ARRAY_StoreRef] = tkMemberRef;
    }

    ReleaseSignature(&signature);

    return tkMemberRef;
}

//****************************************************************************
// Generic data.
//****************************************************************************

//============================================================================
// Add a constant string to the .rdata section
//============================================================================

mdString MetaEmit::AddDataString
(
    _In_count_(cchLen) const WCHAR * wsz,
    size_t cchLen
)
{
    mdString tkString;

    VSASSERT(cchLen >= 0, "Expected character count to be valid.");

    IfFailThrow(m_pmdEmit->DefineUserString(wsz, (unsigned)cchLen, &tkString));

    return tkString;
}

//****************************************************************************
// Image Building
//****************************************************************************

//============================================================================
// Clear list of exception handlers
//============================================================================
void MetaEmit::ClearExceptions()
{
    m_daExceptions.Collapse();
    m_fNeedBigExceptions = false;
    m_oExceptionTable = 0;
}

//============================================================================
// Add an exception handling clause to the method.
//============================================================================

void MetaEmit::AddEHClause
(
    CorExceptionFlag flags,
    unsigned         ulTryOffset,
    unsigned         ulTryLength,
    unsigned         ulHandlerOffset,
    unsigned         ulHandlerLength,
    unsigned         ulFilterOffset,
    mdTypeRef        typrefType
)
{
    VSASSERT(m_daExceptions.Count() || !m_fNeedBigExceptions, "Method never flushed.");

    // Add the information to the array.
    IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT *pException = &m_daExceptions.Add();

    pException->Flags = flags;
    pException->TryOffset = ulTryOffset;
    pException->TryLength = ulTryLength;
    pException->HandlerOffset = ulHandlerOffset;
    pException->HandlerLength = ulHandlerLength;

    if (ulFilterOffset == 0)
    {
        pException->ClassToken = typrefType;
    }
    else
    {
        pException->FilterOffset = ulFilterOffset;
    }

    // Will anything not fit in a small exception?
    if (!m_fNeedBigExceptions)
    {
        // TryOffset is 16 bits
        if (pException->TryOffset > 0xFFFF)
        {
            m_fNeedBigExceptions = true;
        }

        // TryLengh is 8 bits
        else if (pException->TryLength > 0xFF)
        {
            m_fNeedBigExceptions = true;
        }

        // HandlerOffset is 16 bits
        else if (pException->HandlerOffset > 0xFFFF)
        {
            m_fNeedBigExceptions = true;
        }

        // HandlerLength is 8 bits.
        else if (pException->HandlerLength > 0xFF && pException->HandlerLength != -1)
        {
            m_fNeedBigExceptions = true;
        }

        // count of exceptions won't fit in a small EH section
        else if (m_daExceptions.Count() > 20)
        {
            m_fNeedBigExceptions = true;
        }
    }
}

//============================================================================
// Gets the size of the block of memory needed to hold the image.
//============================================================================

unsigned MetaEmit::ImageSize
(
    unsigned cStackDepth,
    bool hasLocals,
    unsigned cbCodeSize
)
{
    unsigned cbHeaderSize;
    unsigned cbExceptionSize;
    unsigned cbImageSize;

    // Tiny headers are used for methods < 64 bytes and no locals.
    //
    // Note: A method can't be tiny if it has exceptions.
    //
    if (!hasLocals && cStackDepth <= 8 && cbCodeSize < 64 && !m_daExceptions.Count())
    {
        m_fTinyHeader = true;
        cbHeaderSize = sizeof(IMAGE_COR_ILMETHOD_TINY);
    }
    else
    {
        m_fTinyHeader = false;
        cbHeaderSize = sizeof(IMAGE_COR_ILMETHOD_FAT);
    }

    // Figure out the size of the exception handling table.
    if (!m_daExceptions.Count())
    {
        cbExceptionSize = 0;
    }
    else if (m_fNeedBigExceptions)
    {
        cbExceptionSize = VBMath::Convert<unsigned>(
            VBMath::Multiply(
                sizeof(IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT), 
                m_daExceptions.Count()) + 
            sizeof(DWORD));  // HACK 
                             // to calc size correctly. this should be reanalyzed
        IfFalseThrow(cbExceptionSize >= sizeof(DWORD));
    }
    else
    {
        cbExceptionSize = VBMath::Convert<unsigned>(
            VBMath::Multiply(
                sizeof(IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_SMALL), 
                m_daExceptions.Count()) + 
            sizeof(DWORD)); // HACK 
                            // to calc size correctly.  this should be reanalyzed
        IfFalseThrow(cbExceptionSize >= sizeof(DWORD));
    }

    // Add it all up.
    cbImageSize = cbHeaderSize + cbCodeSize;
    IfFalseThrow(cbImageSize >= cbHeaderSize);

    // The exception table must be 4-byte aligned.
    if (cbExceptionSize)
    {
        IfFalseThrow(cbImageSize + 3 >= 3);
        cbImageSize = (cbImageSize + 3) & ~3;

        // Remember where to store the exception handling table.
        m_oExceptionTable = cbImageSize;

        cbImageSize += cbExceptionSize;
        IfFalseThrow(cbImageSize >= cbExceptionSize);
    }

    return cbImageSize;
}


mdSignature MetaEmit::ComputeSignatureToken(
    COR_SIGNATURE *pSigLocals,
    unsigned cbSigLocals
    )
{
    mdSignature SignatureToken = mdTokenNil;
    m_pmdEmit->GetTokenFromSig(
        pSigLocals,
        cbSigLocals,
        &SignatureToken);
    return SignatureToken;
}

//============================================================================
// Emit the header and trailer information for this method.  Returns
// the location to put the IL into.
//============================================================================

BYTE *MetaEmit::EmitImage
(
    _Out_ BYTE *pbImage,
    unsigned cbCodeSize,
    unsigned cStackDepth,
    COR_SIGNATURE *pSigLocals,
    unsigned cbSigLocals
)
{
    BYTE *pbIL;

    //
    // Emit the IL header.
    //

    // Tiny header.
    if (m_fTinyHeader)
    {
        *pbImage = (BYTE)(CorILMethod_TinyFormat | cbCodeSize << 2);

        pbIL = pbImage + 1;
    }

    // Bigger header.
    else
    {
        IMAGE_COR_ILMETHOD_FAT *pHeader = (IMAGE_COR_ILMETHOD_FAT *)pbImage;

        // Fill in our side of the ILMETHOD bargain
        pHeader->Flags = CorILMethod_InitLocals | CorILMethod_FatFormat;
        pHeader->MaxStack = cStackDepth;
        pHeader->CodeSize = cbCodeSize;

        pHeader->Size = sizeof(IMAGE_COR_ILMETHOD_FAT) / 4; 

        if (m_daExceptions.Count())
        {
            pHeader->Flags |= CorILMethod_MoreSects;
        }

        // Fill in the locals.
        if (cbSigLocals)
        {
            // LATER AnthonyL: Do we need to ensure that we don't emit duplicate
            //   : signatures?
            //
            m_SignatureToken = ComputeSignatureToken(pSigLocals, cbSigLocals);
            VSASSERT( m_SignatureToken != mdTokenNil, "Why is the signature token mdTokenNil?");
            pHeader->LocalVarSigTok = m_SignatureToken;
        }
        else
        {
            pHeader->LocalVarSigTok = mdTokenNil;
        }

        pbIL = pbImage + pHeader->Size * 4;
    }

    //
    // Emit the exception handling table.
    //

    if (m_daExceptions.Count())
    {
        pbImage = pbImage + m_oExceptionTable;

        if (m_fNeedBigExceptions)
        {
            IMAGE_COR_ILMETHOD_SECT_EH_FAT *pSection = (IMAGE_COR_ILMETHOD_SECT_EH_FAT *)pbImage;
            DWORD size = VBMath::Multiply(m_daExceptions.Count(), sizeof(IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT));

            pSection->SectFat.Kind = CorILMethod_Sect_EHTable | CorILMethod_Sect_FatFormat;
            // Per CLI Spec II.25.4.5 - Method data section the size is (count * clause_size) + 4
            // (4 is the size of the header data that precedes the Clauses in IMAGE_COR_ILMETHOD_SECT_EH_FAT)
            pSection->SectFat.DataSize = VBMath::Add(size, offsetof(IMAGE_COR_ILMETHOD_SECT_EH_FAT, Clauses));
            memcpy(pSection->Clauses, m_daExceptions.Array(), size);
        }
        else
        {
            IMAGE_COR_ILMETHOD_SECT_EH_SMALL *pSection = (IMAGE_COR_ILMETHOD_SECT_EH_SMALL *)pbImage;
            IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT *pClause = m_daExceptions.Array();

            unsigned i, c = m_daExceptions.Count();
            
            unsigned size = VBMath::Multiply(c, sizeof(IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_SMALL));

            ASSERT((size <= MAXBYTE), "The size should never be this large.");

            pSection->SectSmall.Kind = CorILMethod_Sect_EHTable;
            // Per CLI Spec II.25.4.5 - Method data section the size is (count * clause_size) + 4
            // (4 is the size of the header data that precedes Clauses in IMAGE_COR_ILMETHOD_SECT_EH_SMALL
            // 2 bytes for SectSmall, 2 bytes for reserved WORD)
            pSection->SectSmall.DataSize = VBMath::Add(size, offsetof(IMAGE_COR_ILMETHOD_SECT_EH_SMALL, Clauses));

            for (i = 0; i < c; i++, pClause++)
            {
                VSASSERT((pClause->Flags & ~0xFFFF) == 0, "");
                VSASSERT((pClause->TryOffset & ~0xFFFF) == 0, "");
                VSASSERT((pClause->TryLength & ~0xFF) == 0, "");
                VSASSERT((pClause->HandlerOffset & ~0xFFFF) == 0, "");
                VSASSERT(pClause->HandlerLength == -1 || (pClause->HandlerLength & ~0xFF) == 0, "");

                pSection->Clauses[i].Flags         = pClause->Flags;
                pSection->Clauses[i].TryOffset     = pClause->TryOffset;
                pSection->Clauses[i].TryLength     = pClause->TryLength;
                pSection->Clauses[i].HandlerOffset = pClause->HandlerOffset;
                pSection->Clauses[i].HandlerLength = pClause->HandlerLength;
                pSection->Clauses[i].ClassToken    = pClause->ClassToken;
            }
        }
    }

    return pbIL;
}

//============================================================================
// Frees the image's resources.
//============================================================================

void MetaEmit::FreeImageResources()
{
    m_daExceptions.Destroy();
    m_fNeedBigExceptions = false;
}

//****************************************************************************
// Custom attribute
//****************************************************************************

//============================================================================
// Emit special custom attribute to allow for easier debugging.
// Defines the debuggable attribute (via ALink).
//
// Note that, since this attribute goes on an Assembly token,
// it must be handled by ALink.
//============================================================================

void MetaEmit::ALinkEmitDebuggableAttribute()
{
    CompilerProject *pCompilerProject = m_pBuilder->m_pCompilerProject;

    if (pCompilerProject->GeneratePDB() && FEmitAssembly())
    {
        // These are the values for Enum System.Diagnostics.DebuggableAttribute.DebuggingModes
        const __int32 Default = 0x1;
        const __int32 DisableOptimizations = 0x100;
        const __int32 IgnoreSymbolStoreSequencePoints = 0x2;
        const __int32 EnableEditAndContinue = 0x4;

        __int32 DebuggingMode = Default | IgnoreSymbolStoreSequencePoints;

        if (!pCompilerProject->GenerateOptimalIL())
        {
            DebuggingMode |= DisableOptimizations;
        }

        if (pCompilerProject->GenerateENCableCode())
        {
            DebuggingMode |= EnableEditAndContinue;
        }

        if (!CanEmitAttribute(DebuggableAttributeCtor2, NULL))
        {
            return;
        }

        mdTypeRef mdTypeRefForCtor = DefineRTMemberRef(DebuggableAttributeCtor2);

        unsigned char aBlob[8];
        aBlob[0] = 0x1;
        aBlob[1] = 0x0;
        aBlob[2] = ((unsigned char*)&DebuggingMode)[0];
        aBlob[3] = ((unsigned char*)&DebuggingMode)[1];
        aBlob[4] = ((unsigned char*)&DebuggingMode)[2];
        aBlob[5] = ((unsigned char*)&DebuggingMode)[3];
        aBlob[6] = 0x0;
        aBlob[7] = 0x0;

        VSASSERT(m_pALink, "Emitter created without ALink.");
        IfFailThrow(m_pALink->EmitAssemblyCustomAttribute(
                                        m_mdAssemblyOrModule, // IN - Unique ID for the assembly
                                        m_mdFileForModule,    // IN - file that is defining the property
                                        mdTypeRefForCtor,   // IN - Type of the CustomAttribute.
                                        aBlob,              // IN - The custom value data.
                                        sizeof(aBlob),      // IN - The custom value data length.
                                        false,              // IN - True if the CA is a security attribute
                                        false));            // IN - True if AllowMultiple=true
    }
}

//============================================================================
// Defines the compilation relaxation attribute (via ALink).
// This is defined for NoStringInterning
// Note that, since this attribute goes on an Assembly token,
// it must be handled by ALink.
//============================================================================

void MetaEmit::ALinkEmitCompilationRelaxationsAttribute
(
)
{
    mdTypeRef   mdTypeRefForCtor = mdTypeRefNil;
    unsigned char aBlob[8];
    int           param = 8;

    if (!CanEmitAttribute(CompilationRelaxationsAttributeCtor, NULL))
    {
        return;
    }

    mdTypeRefForCtor = DefineRTMemberRef(CompilationRelaxationsAttributeCtor);

    aBlob[0] = 0x1;
    aBlob[1] = 0x0;
    memcpy(&aBlob[2], &param, sizeof(param));
    aBlob[6] = 0x0;
    aBlob[7] = 0x0;

    VSASSERT(m_pALink, "Emitter created without ALink.");
    IfFailThrow(m_pALink->EmitAssemblyCustomAttribute(
                                    m_mdAssemblyOrModule, // IN - Unique ID for the assembly
                                    m_mdFileForModule,    // IN - file that is defining the property
                                    mdTypeRefForCtor,   // IN - Type of the CustomAttribute.
                                    aBlob,              // IN - The custom value data.
                                    sizeof(aBlob),      // IN - The custom value data length.
                                    false,              // IN - True if the CA is a security attribute
                                    false));            // IN - True if AllowMultiple=true
}

void MetaEmit::ALinkEmitAssemblyFlagsAttribute(int AssmeblyFlags, RuntimeMembers AssemblyFlagsAttributeCtor)
{
    VSASSERT( 
        AssemblyFlagsAttributeCtor == AssemblyFlagsAttributeIntCtor ||
        AssemblyFlagsAttributeCtor == AssemblyFlagsAttributeUIntCtor ||
        AssemblyFlagsAttributeCtor == AssemblyFlagsAttributeAssemblyNameFlagsCtor,
        "The runtime member is not support in this function.");

    mdTypeRef   mdTypeRefForCtor = mdTypeRefNil;
    unsigned char aBlob[8];
    int           param = 8;

    if (!CanEmitAttribute(AssemblyFlagsAttributeCtor, NULL))
    {
        return;
    }

    mdTypeRefForCtor = DefineRTMemberRef(AssemblyFlagsAttributeCtor);

    aBlob[0] = 0x1;
    aBlob[1] = 0x0;
    memcpy(&aBlob[2], &AssmeblyFlags, sizeof(AssmeblyFlags));
    aBlob[6] = 0x0;
    aBlob[7] = 0x0;

    VSASSERT(m_pALink, "Emitter created without ALink.");
    IfFailThrow(m_pALink->EmitAssemblyCustomAttribute(
        m_mdAssemblyOrModule, // IN - Unique ID for the assembly
        m_mdFileForModule,    // IN - file that is defining the property
        mdTypeRefForCtor,   // IN - Type of the CustomAttribute.
        aBlob,              // IN - The custom value data.
        sizeof(aBlob),      // IN - The custom value data length.
        false,              // IN - True if the CA is a security attribute
        false));            // IN - True if AllowMultiple=true
}

//============================================================================
// Defines the RuntimeCompatibility Attribute (via ALink).
// This is defined for catching non-exception type exceptions.
// Note that, since this attribute goes on an Assembly token,
// it must be handled by ALink.
//============================================================================

void MetaEmit::ALinkEmitRuntimeCompatibilityAttribute
(
)
{
    mdTypeRef   mdTypeRefForCtor = mdTypeRefNil;
    unsigned char aBlob[] = {
        0x1, 0x0, 0x1, 0x0, 0x54, 0x2, 0x16,
        'W', 'r', 'a', 'p', 'N', 'o', 'n', 'E', 'x', 'c', 'e', 'p', 't', 'i', 'o', 'n', 'T', 'h', 'r', 'o', 'w', 's',
        0x1 };

    if (!CanEmitAttribute(RuntimeCompatibilityAttributeCtor, NULL))
    {
        return;
    }

    mdTypeRefForCtor = DefineRTMemberRef(RuntimeCompatibilityAttributeCtor);

    VSASSERT(m_pALink, "Emitter created without ALink.");
    IfFailThrow(m_pALink->EmitAssemblyCustomAttribute(
                                    m_mdAssemblyOrModule, // IN - Unique ID for the assembly
                                    m_mdFileForModule,    // IN - file that is defining the property
                                    mdTypeRefForCtor,   // IN - Type of the CustomAttribute.
                                    aBlob,              // IN - The custom value data.
                                    sizeof(aBlob),      // IN - The custom value data length.
                                    false,              // IN - True if the CA is a security attribute
                                    false));            // IN - True if AllowMultiple=true
}

//============================================================================
// Defines a custom attribute whose value is a string.
//
// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
//
// Do NOT call this function from ANYWHERE but PEBuilder::EmitAttributes.
// If you call it while emitting tokens, your attribute will be deleted
// when going to compiled in the IDE. (See PEBuilder::EmitAttributes as for
// why)
//
//============================================================================

mdCustomAttribute MetaEmit::DefineCustomAttributeWithString
(
    mdToken               tkObj,
    mdMemberRef           AttributeCtorMemberRef,
    _In_z_ WCHAR          * pwszStringParam
)
{
    mdCustomAttribute customattr = mdCustomAttributeNil;
    VBHeapPtr<BYTE> pCustomAttr;
    PBYTE pData;
    ULONG cbAlloc;          // Total bytes to allocate
    ULONG cbCompressedStrLenHeader;     // Length of compressed string length header
    BYTE  rgbCompressedStrLenHeader[4];
    int cchUnicode;         // Count of Unicode chars in input string
    int cbUtf8;             // Count of bytes in UTF8-encoded string


    if (pwszStringParam == NULL)
    {
        // NULL strings are encoded as a single byte 0xFF
        cchUnicode = 0;
        cbUtf8 = 0;
        rgbCompressedStrLenHeader[0] = 0xFF;
        cbCompressedStrLenHeader = 1;
    }
    else
    {
        // Get length of Unicode string and size of UTF-8 string.
        // PE arg blob format limits strings to no more than 0x1FFFFFFF bytes,
        // in UTF-8 format, so we can safely represent this as an int.
        size_t strlenTemp = wcslen(pwszStringParam);
        IfFalseThrow(strlenTemp == (int)strlenTemp);
        cchUnicode = (int)strlenTemp;
        cbUtf8 = UTF8LengthOfUnicode(pwszStringParam, cchUnicode);
        IfFalseThrow(cbUtf8==(ULONG)cbUtf8); // We'll be casting this later
        IfFalseThrow(cbUtf8 < 0x1FFFFFFF);

        // Calculate size of compressed string length header
        cbCompressedStrLenHeader = CorSigCompressData(cbUtf8, rgbCompressedStrLenHeader);
    }

    // Alloc space for: attribute header (2 bytes) + string length + string + count of named params (2 bytes)
    IfFalseThrow(cbCompressedStrLenHeader + (ULONG)cbUtf8 >= (ULONG)cbUtf8 || cbCompressedStrLenHeader + (ULONG)cbUtf8 + 2*sizeof(short) >= 2*sizeof(short))
    cbAlloc = sizeof(short) + cbCompressedStrLenHeader + cbUtf8 + sizeof(short);
    pCustomAttr.AllocateBytes(cbAlloc);
    pData = pCustomAttr;

    // Add attribute header (always a 2-byte field with value 1)
    *(short*)pData = 1;

    pData += sizeof(short);

    // Add string length header
#pragma prefast(suppress: 22105 "Properly bound.  We need to fix the above code to be more readable to SAL")
    memcpy(pData, rgbCompressedStrLenHeader, cbCompressedStrLenHeader);
#pragma warning(suppress: 22009)//cbCompressedStrLenHeader is understood to be valid.
    pData += cbCompressedStrLenHeader;

    if (pwszStringParam != NULL)
    {
        // Add UTF-8 string
        UnicodeToUTF8(pwszStringParam, &cchUnicode, (char*)pData, cbUtf8);
        pData += cbUtf8;
    }

    // Named property count
    // To fix alignment faults.
    *(USE_UNALIGNED short*)pData = 0;

    VSASSERT(TypeFromToken(tkObj) != mdtAssembly, "Assembly attributes must be emitted via ALink!");
    IfFailThrow(DefineCustomAttribute(tkObj,
                     AttributeCtorMemberRef,
                     pCustomAttr,
                     cbAlloc,
                     &customattr));

    VSASSERT(!IsNilToken(customattr), "Bad COM+ output!");

    return customattr;
}

mdCustomAttribute MetaEmit::DefineCustomAttributeWithString_String
(
    mdToken               tkObj,
    mdMemberRef           AttributeCtorMemberRef,
    _In_z_ WCHAR          * pwszStringParam1,
    _In_z_ WCHAR          * pwszStringParam2
)
{ 
    // Compute the size of the arg blob we will need to hold these strings.
    // Arg blobs store strings in UTF-8 format, so we have to compute the length
    // these strings will have, in bytes, once they have been converted.
    int string1Chars = VBMath::Convert<int>(wcslen(pwszStringParam1));
    ULONG string1Bytes = UTF8LengthOfUnicode(pwszStringParam1, string1Chars);
    int string2Chars = VBMath::Convert<int>(wcslen(pwszStringParam2));
    ULONG string2Bytes = UTF8LengthOfUnicode(pwszStringParam2, string2Chars);

    // String lengths are represented using a funky UTF-8-style compressed format, where
    // string lengths < 127 bytes occupy one byte, 128 to 16k occupy two bytes, and so on.
    BYTE compressedString1Len[4];
    ULONG lenOfCompressedString1Len = CorSigCompressData(string1Bytes, compressedString1Len);
    BYTE compressedString2Len[4];
    ULONG lenOfCompressedString2Len = CorSigCompressData(string2Bytes, compressedString2Len);

    // Compute the total size of the arg blob and allocate a buffer to hold it.
    // We reserve 2 bytes for the attribute header, then the space for each string, then the
    // count of "named parameters" (which we don't use).
    ULONG argBlobLen = 2;   // size of attribute header (arg blob format version)
    argBlobLen += lenOfCompressedString1Len;
    argBlobLen += string1Bytes;
    argBlobLen += lenOfCompressedString2Len;
    argBlobLen += string2Bytes;
    argBlobLen += 2;        // number of named property assignments (we don't use this)
    VBHeapPtr<BYTE> pArgBlob(argBlobLen);
    PBYTE writer = pArgBlob;

    // Write out the contents of the arg blob.
    // Begin with the header, which consists of a format version.
    *(short*)writer = 1;
    writer += sizeof(short);

    // First string parameter: compressed length, then UTF-8 bytes.
    memcpy(writer, compressedString1Len, lenOfCompressedString1Len);
    writer += lenOfCompressedString1Len;
    UnicodeToUTF8(pwszStringParam1, &string1Chars, (PSTR)writer, string1Bytes);
    writer += string1Bytes;

    // Second string parameter: again, the compressed length, then UTF-8 bytes.
    memcpy(writer, compressedString2Len, lenOfCompressedString2Len);
    writer += lenOfCompressedString2Len;
    UnicodeToUTF8(pwszStringParam2, &string2Chars, (PSTR)writer, string2Bytes);
    writer += string2Bytes;

    // Finish the arg blob with the named property count, which we don't use.
    *(USE_UNALIGNED short*)writer = 0;
    
    mdCustomAttribute customattr = mdCustomAttributeNil;
    VSASSERT(TypeFromToken(tkObj) != mdtAssembly, "Assembly attributes must be emitted via ALink!");
    IfFailThrow(DefineCustomAttribute(tkObj,
                     AttributeCtorMemberRef,
                     pArgBlob,
                     argBlobLen,
                     &customattr));
    VSASSERT(!IsNilToken(customattr), "Bad COM+ output!");
    return customattr;
}

//============================================================================
// Defines a custom attribute whose value is an integer.
//
// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
//
// Do NOT call this function from ANYWHERE but PEBuilder::EmitAttributes.
// If you call it while emitting tokens, your attribute will be deleted
// when going to compiled in the IDE. (See PEBuilder::EmitAttributes as for
// why)
//
//============================================================================

mdCustomAttribute MetaEmit::DefineCustomAttributeWithInt32
(
    mdToken               tkObj,
    mdMemberRef           AttributeCtorMemberRef,
    int                   integerParam
)
{
    mdCustomAttribute customattr = mdCustomAttributeNil;

    unsigned char aBlob[8];
    aBlob[0] = 0x1;
    aBlob[1] = 0x0;
    memcpy(&aBlob[2], &integerParam, sizeof(integerParam));
    aBlob[6] = 0x0;
    aBlob[7] = 0x0;

    IfFailThrow(DefineCustomAttribute(tkObj,
                     AttributeCtorMemberRef,
                     &aBlob[0],
                     sizeof(aBlob),
                     &customattr));

    VSASSERT(!IsNilToken(customattr), "Bad COM+ output!");

    return customattr;
}

//============================================================================
// Defines a custom attribute whose value is an integer.
//
// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
//
// Do NOT call this function from ANYWHERE but PEBuilder::EmitAttributes.
// If you call it while emitting tokens, your attribute will be deleted
// when going to compiled in the IDE. (See PEBuilder::EmitAttributes as for
// why)
//
//============================================================================

mdCustomAttribute MetaEmit::DefineCustomAttributeWithInt16
(
    mdToken               tkObj,
    mdMemberRef           AttributeCtorMemberRef,
    __int16               integerParam
)
{
    mdCustomAttribute customattr = mdCustomAttributeNil;
    
    unsigned char aBlob[6];
    aBlob[0] = 0x1;
    aBlob[1] = 0x0;
    memcpy(&aBlob[2], &integerParam, sizeof(integerParam));
    aBlob[4] = 0x0;
    aBlob[5] = 0x0;

    IfFailThrow(DefineCustomAttribute(tkObj,
                     AttributeCtorMemberRef,
                     &aBlob[0],
                     sizeof(aBlob),
                     &customattr));

    VSASSERT(!IsNilToken(customattr), "Bad COM+ output!");

    return customattr;
}

//============================================================================
// Defines a custom attribute whose value is an bool.
//
// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
//
// Do NOT call this function from ANYWHERE but PEBuilder::EmitAttributes.
// If you call it while emitting tokens, your attribute will be deleted
// when going to compiled in the IDE. (See PEBuilder::EmitAttributes as for
// why)
//
//============================================================================

mdCustomAttribute MetaEmit::DefineCustomAttributeWithBool
(
    mdToken               tkObj,
    mdMemberRef           AttributeCtorMemberRef,
    bool                  boolParam
)
{
    mdCustomAttribute customattr = mdCustomAttributeNil;

    unsigned char aBlob[5];
    aBlob[0] = 0x1;
    aBlob[1] = 0x0;
    aBlob[2] = boolParam;
    aBlob[3] = 0x0;
    aBlob[4] = 0x0;

    IfFailThrow(DefineCustomAttribute(tkObj,
                     AttributeCtorMemberRef,
                     &aBlob[0],
                     sizeof(aBlob),
                     &customattr));

    VSASSERT(!IsNilToken(customattr), "Bad COM+ output!");

    return customattr;
}

//============================================================================
// Defines a custom attribute whose value is an integer64.
//
// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
//
// Do NOT call this function from ANYWHERE but PEBuilder::EmitAttributes.
// If you call it while emitting tokens, your attribute will be deleted
// when going to compiled in the IDE. (See PEBuilder::EmitAttributes as for
// why)
//
//============================================================================

mdCustomAttribute MetaEmit::DefineCustomAttributeWithInt64
(
    mdToken               tkObj,
    mdMemberRef           AttributeCtorMemberRef,
    __int64               integerParam
)
{
    mdCustomAttribute customattr = mdCustomAttributeNil;

    unsigned char aBlob[12];
    aBlob[0] = 0x1;
    aBlob[1] = 0x0;
    memcpy(&aBlob[2], &integerParam, sizeof(integerParam));
    aBlob[10] = 0x0;
    aBlob[11] = 0x0;

    IfFailThrow(DefineCustomAttribute(tkObj,
                     AttributeCtorMemberRef,
                     &aBlob[0],
                     sizeof(aBlob),
                     &customattr));

    VSASSERT(!IsNilToken(customattr), "Bad COM+ output!");

    return customattr;
}


//============================================================================
// Defines the decimal attribute
//
// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
//
// Do NOT call this function from ANYWHERE but PEBuilder::EmitAttributes.
// If you call it while emitting tokens, your attribute will be deleted
// when going to compiled in the IDE. (See PEBuilder::EmitAttributes as for
// why)
//
//============================================================================

mdCustomAttribute MetaEmit::DefineDecimalCustomAttribute
(
    mdToken               tkObj,
    DECIMAL               DecimalValue
)
{
    mdCustomAttribute customattr = mdCustomAttributeNil;
    mdTypeRef   mdTypeRefForCtor = mdTypeRefNil;

    unsigned char aBlob[18];

    if (!CanEmitAttribute(DecimalConstantAttributeCtor, NULL))
    {
        return customattr;
    }

    mdTypeRefForCtor = DefineRTMemberRef(DecimalConstantAttributeCtor);

    aBlob[0] = 0x1;
    aBlob[1] = 0x0;
    aBlob[2] = DecimalValue.scale;
    aBlob[3] = DecimalValue.sign;
    *((unsigned int *)&aBlob[4]) = DecimalValue.Hi32;
    *((unsigned int *)&aBlob[8]) = DecimalValue.Mid32;
    *((unsigned int *)&aBlob[12]) = DecimalValue.Lo32;
    aBlob[16] = 0x0;
    aBlob[17] = 0x0;

    IfFailThrow(DefineCustomAttribute(tkObj,
                     mdTypeRefForCtor,
                     &aBlob[0],
                     sizeof(aBlob),
                     &customattr));

    VSASSERT(!IsNilToken(customattr), "Bad COM+ output!");

    return customattr;
}

//============================================================================
// Defines a object as a blob.
//
// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
//
// Do NOT call this function from ANYWHERE but PEBuilder::EmitAttributes.
// If you call it while emitting tokens, your attribute will be deleted
// when going to compiled in the IDE. (See PEBuilder::EmitAttributes as for
// why)
//
//============================================================================

mdCustomAttribute MetaEmit::DefineCustomAttributeObject
(
    mdToken               tkObj,
    mdMemberRef           AttributeCtorMemberRef,
    ULONG                 cbArgBlob,  // [In] number of bytes
    BYTE*                 argBlob     // [In] array of bytes, which represents the object
)
{
    mdCustomAttribute customattr = mdCustomAttributeNil;
    mdTypeRef   mdTypeRefForCtor = mdTypeRefNil;
    NorlsAllocator nra(NORLSLOC);
    BYTE *aBlob = (BYTE*)nra.Alloc(sizeof(BYTE) * (cbArgBlob + 2));

    // The following captures that if argBlob is NULL then the length of it must be 0
    IfFalseThrow( cbArgBlob == 0 || argBlob != NULL );

    aBlob[0] = 0x1;
    aBlob[1] = 0x0;
    memcpy(&aBlob[2], argBlob, cbArgBlob);

    IfFailThrow(DefineCustomAttribute(tkObj,
                     AttributeCtorMemberRef,
                     &aBlob[0],
                     cbArgBlob + 2,
                     &customattr));

    VSASSERT(!IsNilToken(customattr), "Bad COM+ output!");

    return customattr;
}

//============================================================================
// Checks to see if the type is defined. 
//
// It also passes the error table obtained from the symbol or the builder's 
// errortable so that GetSymbolForRuntimeClass can report the right 
// error (not found or ambiguous).
//============================================================================
bool MetaEmit::CanEmitAttribute(RuntimeMembers attributeCtor, BCSYM *pSym)
{
    ErrorTable *pErrorTable = pSym && pSym->IsNamedRoot() ? pSym->PNamedRoot()->GetErrorTableForContext() : GetBuilder()->GetErrorTable();
    Location *pLocation = pSym ? pSym->GetLocation() : NULL;  
    if (IsNilToken(DefineRTMemberRef(attributeCtor, pErrorTable, pLocation)))
    {
        return false;
    }
    return true;
}

//============================================================================
// Defines a custom value as a blob.
//
// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
//
// Do NOT call this function from ANYWHERE but PEBuilder::EmitAttributes.
// If you call it while emitting tokens, your attribute will be deleted
// when going to compiled in the IDE. (See PEBuilder::EmitAttributes as for
// why)
//
//============================================================================

mdCustomAttribute MetaEmit::DefineCustomAttribute
(
    mdToken               tkObj,
    mdMemberRef           AttributeCtorMemberRef
)
{
    mdCustomAttribute customattr = mdCustomAttributeNil;
    mdMemberRef mdMemberRefForCtor = AttributeCtorMemberRef;
    unsigned char aBlob[4];

    aBlob[0] = 0x1;
    aBlob[1] = 0x0;
    aBlob[2] = 0x0;
    aBlob[3] = 0x0;

    IfFailThrow(DefineCustomAttribute(tkObj,
                     mdMemberRefForCtor,
                     &aBlob[0],
                     sizeof(aBlob),
                     &customattr));

    VSASSERT(!IsNilToken(customattr), "Bad COM+ output!");

    return customattr;
}

mdCustomAttribute MetaEmit::DefineBestFitMappingAttribute
(
    mdToken                tkObj,
    bool                   bestFitMapping,
    bool                   throwOnUnmappableChar
)
{
    mdCustomAttribute customattr = mdCustomAttributeNil;
    mdTypeRef   mdTypeRefForCtor = mdTypeRefNil;

    unsigned char aBlob[30];
    // Arg blob format version, 0x0001
    aBlob[0] = 0x01;
    aBlob[1] = 0x00;
    // Boolean parameter value
    aBlob[2] = bestFitMapping;
    // Number of name/value arg pairs, 1
    aBlob[3] = 0x01;
    aBlob[4] = 0x00;
    // Value of ThrowOnUnmappableChar field
    aBlob[5] = SERIALIZATION_TYPE_FIELD;
    aBlob[6] = SERIALIZATION_TYPE_BOOLEAN;
    // field name is encoded in UTF-8, so we should use char not wchar here.
    unsigned char fieldName[] = "ThrowOnUnmappableChar";
    AssertIfFalse(sizeof(fieldName) == 22); // one byte for terminator char
    aBlob[7] = 21;
    memcpy(&aBlob[8], &fieldName, 21);
    // field value is a one-byte boolean
    aBlob[29] = throwOnUnmappableChar;

    if (!CanEmitAttribute(BestFitMappingAttributeCtor, NULL))
    {
        return customattr;
    }
    mdTypeRefForCtor = DefineRTMemberRef(BestFitMappingAttributeCtor);
    
    IfFailThrow(DefineCustomAttribute(
        tkObj,
        mdTypeRefForCtor,
        aBlob, sizeof(aBlob),
        &customattr));
    VSASSERT(!IsNilToken(customattr), "Bad COM+ output!");
    return customattr;
}

HRESULT MetaEmit::DefineCustomAttribute
(
    mdToken     tkObj,                  // [IN] The object to put the value on.
    mdToken     tkType,                 // [IN] Type of the CustomAttribute (TypeRef/TypeDef).
    void const  *pCustomAttribute,      // [IN] The custom value data.
    ULONG       cbCustomAttribute,      // [IN] The custom value data length.
    mdCustomAttribute *pcv              // [OUT] The custom value token value on return.
)
{
    HRESULT hr;

    // Do not remove this Assert Bug 56115 - DevDiv Bugs
    VSASSERT(TypeFromToken(tkObj) != mdtAssembly, "Assembly attributes must be emitted via ALink!");

    hr = m_pmdEmit->DefineCustomAttribute(
                        tkObj,
                        tkType,
                        pCustomAttribute,
                        cbCustomAttribute,
                        pcv);

    return hr;
}


//****************************************************************************
// Signature encoding helpers.
//****************************************************************************

//============================================================================
// Starts a new signature, saving the old one away.
//============================================================================

void MetaEmit::StartNewSignature
(
    _Out_ Signature *psig
)
{
    m_nraSignature.Mark(&psig->m_mark);
    psig->m_pbSignature = m_pbSignature;
    psig->m_cbSignature = m_cbSignature;

    m_pbSignature = NULL;
    m_cbSignature = 0;
}

//============================================================================
// Create a metamemberref.
//============================================================================

void MetaEmit::EncodeMetaMemberRef
(
    _In_z_ STRING *pstrMemberName,
    mdTypeRef tr
)
{
    VSASSERT(!m_pbSignature, "Must be first.");

    // Use FIELD_OFFSET instead of sizeof here
    // sizeof will not return the expected value when using m_pSignature[0] in the structure
    // and packing variations across target platforms causes garbage to be written out
    //
    m_pbSignature = (BYTE *)m_nraSignature.Alloc(FIELD_OFFSET(struct MetaMemberRef, m_pSignature));
    m_cbSignature = FIELD_OFFSET(struct MetaMemberRef, m_pSignature);

    GetMetaMemberRef()->m_pstrMemberName = pstrMemberName;
    GetMetaMemberRef()->m_tr = tr;
}

void MetaEmit::EncodeMetaMemberRef
(
    BCSYM_NamedRoot *psymMember,
    mdTypeRef tr
)
{
    VSASSERT(!m_pbSignature, "Must be first.");
    AssertIfNull(psymMember);

    // Use FIELD_OFFSET instead of sizeof here
    // sizeof will not return the expected value when using m_pSignature[0] in the structure
    // and packing variations across target platforms causes garbage to be written out
    //
    m_pbSignature = (BYTE *)m_nraSignature.Alloc(FIELD_OFFSET(struct MetaMemberRef, m_pSignature));
    m_cbSignature = FIELD_OFFSET(struct MetaMemberRef, m_pSignature);

    GetMetaMemberRef()->m_pstrMemberName = psymMember->GetEmittedName();
    GetMetaMemberRef()->m_tr = tr;
}

//============================================================================
// Create an array key.
//============================================================================

void MetaEmit::EncodeArrayKey()
{
    VSASSERT(!m_pbSignature, "Must be first.");

    m_pbSignature = (BYTE *)m_nraSignature.Alloc(FIELD_OFFSET(struct ArrayKey, m_pSignature));
    m_cbSignature = FIELD_OFFSET(struct ArrayKey, m_pSignature);
}

//============================================================================
// Create a generic method instantiation.
//============================================================================

void MetaEmit::EncodeMethodInstantiation
(
    _In_z_ STRING *Name,
    mdMemberRef UnboundMethod
)
{
    VSASSERT(!m_pbSignature, "Must be first.");

    m_pbSignature = (BYTE *)m_nraSignature.Alloc(FIELD_OFFSET(struct GenericMethodInstantiation, m_pSignature));
    m_cbSignature = FIELD_OFFSET(struct GenericMethodInstantiation, m_pSignature);

    GetMethodInstantiation()->m_pstrMemberName = Name;
    GetMethodInstantiation()->m_MemberRef = UnboundMethod;
}

//============================================================================
// Create a generic type instantiation.
//============================================================================

void MetaEmit::EncodeTypeInstantiation
(
    _In_z_ STRING *Name,
    mdTypeRef UnboundType
)
{
    VSASSERT(!m_pbSignature, "Must be first.");

    m_pbSignature = (BYTE *)m_nraSignature.Alloc(FIELD_OFFSET(struct GenericTypeInstantiation, m_pSignature));
    m_cbSignature = FIELD_OFFSET(struct GenericTypeInstantiation, m_pSignature);

    GetTypeInstantiation()->m_pstrMemberName = Name;
    GetTypeInstantiation()->m_TypeRef = UnboundType;
}

//============================================================================
// Encodes a COM method signature
//============================================================================

void MetaEmit::EncodeMethodSignature
(
    BCSYM_Proc *pproc,
    Location *pReferencingLocation
)
{
    unsigned cc;
    unsigned cParams;

    // Format:
    //    calling convention  - 1 byte
    //    argument count      - variable integer
    //    return type         - variable encoded
    //    argument types      - variable encoded
    //

    cParams = pproc->GetParameterCount();
    cc = IMAGE_CEE_CS_CALLCONV_DEFAULT;

    if (!(pproc->IsShared()))
    {
        cc |= IMAGE_CEE_CS_CALLCONV_HASTHIS;
    }

    if (pproc->IsGeneric())
    {
        cc |= IMAGE_CEE_CS_CALLCONV_GENERIC;
    }

    EncodeInteger(cc);

    if (pproc->IsGeneric())
    {
        BCSYM_GenericParam *GenericParam;
        unsigned GenericParamCount = 0;

        for (GenericParam = pproc->GetFirstGenericParam(); GenericParam; GenericParam = GenericParam->GetNextParam())
        {
            GenericParamCount++;
        }

        EncodeInteger(GenericParamCount);
    }

    EncodeInteger(cParams);

    EncodeType(pproc->GetCompilerType(), pReferencingLocation);

    {
        BCITER_Parameters biParams( pproc );
        BCSYM_Param * pparamNext;
        while (pparamNext = biParams.PparamNext())
        {
            EncodeType(pparamNext->GetCompilerType(),
                       pReferencingLocation,
                       t_UNDEF, // don't have to fill this in
                       (pproc->IsDllDeclare() &&
                        !pparamNext->GetPWellKnownAttrVals()->GetMarshalAsData()) ?
                            stNDirect :
                            stNormal);
        }
    }
}

//============================================================================
// Encodes a COM field signature and creates a MemAlloc'ed
// buffer for the sigature.
//============================================================================

void MetaEmit::EncodeFieldSignature
(
    BCSYM_Member     * psymMem,
    Location         * pReferencingLocation
)
{
    // Format:
    //    calling convention  - 1 byte
    //    field type          - variable encoded
    //

    EncodeInteger(IMAGE_CEE_CS_CALLCONV_FIELD);
    EncodeType(psymMem->GetCompilerType(), pReferencingLocation);
}

//============================================================================
// Encodes a COM property signature and returns it in a buffer allocated in
// the passed-in allocator.
//============================================================================

void MetaEmit::EncodePropertySignature
(
    BCSYM_Proc * pproc
)
{
    unsigned cParams;
    unsigned cc;
    BCITER_Parameters biParams( pproc );
    BCSYM_Param * pparamNext;
    BCSYM_Param * pparamLast = pproc->GetLastParam();

    cParams = pproc->GetParameterCount();

    cc = IMAGE_CEE_CS_CALLCONV_PROPERTY;
    if (!(pproc->IsShared()))
    {
        cc |= IMAGE_CEE_CS_CALLCONV_HASTHIS;
    }

    EncodeInteger(cc);
    EncodeInteger(cParams);

    EncodeType(pproc->GetCompilerType(), pproc->GetRawType()->GetLocation());


    while (pparamNext = biParams.PparamNext())
    {
        EncodeType(pparamNext->GetCompilerType(), pparamNext->GetRawType()->GetLocation());
    }
}

//============================================================================
// Encodes an integer for meta data.
//============================================================================

void MetaEmit::EncodeInteger
(
    unsigned long ul
)
{
    ULONG   cb;     // Bytes used by encoded format (can be 1..4)
    BYTE    rgb[4]; // Bytes to store encoded integer
    BYTE   *pb;

    // We used to inline this computation.  Use CorSigCompressData instead.
    cb = CorSigCompressData(ul, rgb);

    VSASSERT(cb >= 1 && cb <= 4, "EncodeInteger: integer can't be encoded.");
    IfFalseThrow(cb >= 1 && cb <= 4);

    pb = EncodeExtendBuffer(cb);

    // cb value is verified above
#pragma warning(disable:26000)
    memcpy(pb, rgb, cb);
#pragma warning(default:26000)
}

//============================================================================
// Encodes a signed integer for meta data.
//============================================================================

void MetaEmit::EncodeSignedInteger
(
    long l
)
{
    ULONG isSigned = 0;

    if (l < 0)
    {
        isSigned = 0x1;
    }

    if ((l & SIGN_MASK_ONEBYTE) == 0 || (l & SIGN_MASK_ONEBYTE) == SIGN_MASK_ONEBYTE)
    {
        l &= ~SIGN_MASK_ONEBYTE;
    }
    else if ((l & SIGN_MASK_TWOBYTE) == 0 || (l & SIGN_MASK_TWOBYTE) == SIGN_MASK_TWOBYTE)
    {
        l &= ~SIGN_MASK_TWOBYTE;
    }

    else if ((l & SIGN_MASK_FOURBYTE) == 0 || (l & SIGN_MASK_FOURBYTE) == SIGN_MASK_FOURBYTE)
    {
        l &= ~SIGN_MASK_FOURBYTE;
    }
    else
    {
        VSFAIL("EncodeSignedInteger: integer can't be encoded.");
    }

    l = l << 1 | isSigned;

    EncodeInteger(l);
}

//============================================================================
// Encodes a token for meta data.
//============================================================================

void MetaEmit::EncodeToken
(
    mdToken tk
)
{
    RID rid = RidFromToken(tk);
    BOOL isTypRef = 0;
    mdToken ulTyp = TypeFromToken(tk);

    if (rid > 0x3FFFFFF)
    {
        VSFAIL("EncodeToken: token can't be encoded.");
    }

    rid = (rid << 2);

    if (ulTyp == g_tkCorEncodeToken[1])
    {
        rid |= 0x1;
    }
    else if (ulTyp == g_tkCorEncodeToken[2])
    {
        rid |= 0x2;
    }

    EncodeInteger(rid);
}

//============================================================================
// Encodes a TYPE for meta data.
//============================================================================

void MetaEmit::EncodeGenericArguments
(
    BCSYM_GenericBinding *Binding,
    unsigned stType,
    bool EncodeParentArguments,
    Location *pReferencingLocation
)
{
    if (Binding == NULL)
    {
        return;
    }

    if (EncodeParentArguments)
    {
        EncodeGenericArguments(Binding->GetParentBinding(), stType, true, pReferencingLocation);
    }

    unsigned ArgumentCount = Binding->GetArgumentCount();

    for (unsigned ArgumentIndex = 0; ArgumentIndex < ArgumentCount; ArgumentIndex++)
    {
        EncodeType(Binding->GetArgument(ArgumentIndex), pReferencingLocation, t_void, stType & ~stNDirect);
    }
}

void MetaEmit::EncodeType
(
    BCSYM *psymType,
    Location *pReferencingLocation, // the location in the code that is referring to this symbol.
                                    // Used for squiggle location if an error occurs when referring to this symbol.
    Vtypes vtype,                   // default is t_UNDEF
    unsigned stType                 // default is stNormal
)
{
    if (psymType)
    {
        if (psymType->IsEnum())
            vtype = t_struct;
        else
            vtype = psymType->GetVtype();
    }
    else if (vtype == t_UNDEF)
    {
        vtype = t_void;
    }

    switch (vtype)
    {
    case t_void:
        EncodeInteger(ELEMENT_TYPE_VOID);
        break;

    case t_bool:
        EncodeInteger(ELEMENT_TYPE_BOOLEAN);
        break;

    case t_i1:
        EncodeInteger(ELEMENT_TYPE_I1);
        break;

    case t_ui1:
        EncodeInteger(ELEMENT_TYPE_U1);
        break;

    case t_i2:
        EncodeInteger(ELEMENT_TYPE_I2);
        break;

    case t_ui2:
        EncodeInteger(ELEMENT_TYPE_U2);
        break;

    case t_i4:
        EncodeInteger(ELEMENT_TYPE_I4);
        break;

    case t_ui4:
        EncodeInteger(ELEMENT_TYPE_U4);
        break;

    case t_i8:
        EncodeInteger(ELEMENT_TYPE_I8);
        break;

    case t_ui8:
        EncodeInteger(ELEMENT_TYPE_U8);
        break;

    case t_single:
        EncodeInteger(ELEMENT_TYPE_R4);
        break;

    case t_double:
        EncodeInteger(ELEMENT_TYPE_R8);
        break;

    case t_char:
        EncodeInteger(ELEMENT_TYPE_CHAR);
        break;

    case t_string:
        if (stType & stNDirect)
        {
            // PInvoke HACK: In VB, ByVal String reflects changes
            // back through Declares. So under the covers make this
            // a pointer to a string. We'll decorate the parameter when
            // we output the parameter spec.
            EncodeInteger(ELEMENT_TYPE_BYREF);
        }

        EncodeInteger(ELEMENT_TYPE_STRING);
        break;

    case t_date:
    case t_decimal:
        EncodeInteger(ELEMENT_TYPE_VALUETYPE);
        EncodeToken(
            DefineTypeRefBySymbol(
                m_pCompilerProject->GetCompilerHost()->GetFXSymbolProvider()->GetType(vtype)->PContainer(),
                pReferencingLocation));
        break;

    case t_ref:
    case t_struct:
        {
            VSASSERT(psymType, "need a symbol!");

            if (psymType->IsObject())
            {
                EncodeInteger(ELEMENT_TYPE_OBJECT);
            }
            else if (psymType == m_pCompilerProject->GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::IntPtrType))
            {
                EncodeInteger(ELEMENT_TYPE_I);
            }
            else if (psymType == m_pCompilerProject->GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::UIntPtrType))
            {
                EncodeInteger(ELEMENT_TYPE_U);
            }
            else if (psymType->IsGenericTypeBinding())
            {
                BCSYM_GenericTypeBinding *Binding = psymType->PGenericTypeBinding();

                EncodeInteger(ELEMENT_TYPE_GENERICINST);
                EncodeInteger(vtype == t_ref ? ELEMENT_TYPE_CLASS : ELEMENT_TYPE_VALUETYPE);

                EncodeToken(DefineTypeRefBySymbol(Binding->GetGenericType(), pReferencingLocation));

                unsigned ArgumentCount = Binding->GetArgumentCount();

                // Include the arguments of any enclosing generic types.
                for (BCSYM_GenericTypeBinding *ParentBinding = Binding->GetParentBinding();
                     ParentBinding;
                     ParentBinding = ParentBinding->GetParentBinding())
                {
                    ArgumentCount += ParentBinding->GetArgumentCount();
                }

                EncodeInteger(ArgumentCount);

                EncodeGenericArguments(Binding, stType, true, pReferencingLocation);
            }
            else
            {
                BCSYM *psymRealType = psymType->DigThroughAlias()->DigThroughAlias();

                VSASSERT(psymRealType, "Should have an object form for this type.");

                if (vtype == t_struct)
                {
                    EncodeInteger(ELEMENT_TYPE_VALUETYPE);
                }
                else
                {
                    EncodeInteger(ELEMENT_TYPE_CLASS);
                }

                EncodeToken(DefineTypeRefBySymbol(psymRealType->PContainer(), pReferencingLocation));
            }
        }
        break;

    case t_ptr:
        {
            BCSYM * pBase;

            VSASSERT(psymType, "need a symbol!");

            pBase = psymType->PPointerType()->GetRoot();

            EncodeInteger(ELEMENT_TYPE_BYREF);
            if (pBase->GetVtype() == t_string)
            {
                // HACK: Because we need special encoding of ByVal String in
                // PInvoke, handle the string case here, so we never have to
                // worry about distinguishing ByRef String in the PInvoke
                // case above
                EncodeInteger(ELEMENT_TYPE_STRING);
            }
            else
            {
                EncodeType(pBase, pReferencingLocation, vtype, stType);
            }
        }
        break;

    case t_array:
        {
            BCSYM_ArrayType * pfixedrank = psymType->PArrayType();
            VSASSERT(pfixedrank, "This should never be NULL");
            unsigned cDims = pfixedrank->GetRank();

            if (cDims == 1)
            {
                EncodeInteger(ELEMENT_TYPE_SZARRAY);
                // strip off the NDirect bit so the element type won't be byref in the case of string
                EncodeType(pfixedrank->GetRoot(), pReferencingLocation, vtype, stType & ~stNDirect);
            }
            else
            {
                EncodeInteger(ELEMENT_TYPE_ARRAY);
                EncodeType(pfixedrank->GetRoot(), pReferencingLocation, vtype, stType);
                EncodeInteger(cDims);
                EncodeInteger(0); // Count of sized dimensions -- leave unspecified
                EncodeInteger(cDims); // Count of bounded dimensions
                for (unsigned iDim = 0; iDim < cDims; iDim++)
                {
                    EncodeInteger(0);   // Lower bound of dimension iDim
                }
            }
        }

        break;

    case t_generic:

        EncodeInteger(psymType->PGenericParam()->IsGenericMethodParam() ? ELEMENT_TYPE_MVAR : ELEMENT_TYPE_VAR);
        EncodeInteger(psymType->PGenericParam()->GetMetaDataPosition());
        break;

    case t_bad:
        VSFAIL("EncodeType - How did a bad type get through semantic analysis?");
        __fallthrough;

    case t_UNDEF:
    default:
        VSFAIL("EncodeType - unhandled type.");

        VbThrow(
            HrMakeRepl(
                ERRID_InternalCompilerError));
    }
}

//============================================================================
// Extend the current signature buffer.
//============================================================================

BYTE *MetaEmit::EncodeExtendBuffer
(
    size_t cb
)
{
    BYTE *pb;

    IfFalseThrow(m_cbSignature + cb >= cb);

    m_pbSignature = (BYTE *)m_nraSignature.Resize(m_pbSignature,
                                                  m_cbSignature,
                                                  m_cbSignature + cb);

    pb = m_pbSignature + m_cbSignature;

    m_cbSignature += (unsigned)cb;

    return pb;
}

//============================================================================
// Fill in all of the metamemberref fields.
//============================================================================

void MetaEmit::EncodeFinishMetaMemberRef()
{
    GetMetaMemberRef()->m_cbSizeSig = m_cbSignature - FIELD_OFFSET(struct MetaMemberRef, m_pSignature);
}

//============================================================================
// Fill in all of the arraykey.
//============================================================================

void MetaEmit::EncodeFinishArrayKey()
{
    GetArrayKey()->m_cbSizeSig = m_cbSignature - FIELD_OFFSET(struct ArrayKey, m_pSignature);
}

//============================================================================
// Fill in all of the method instantation.
//============================================================================

void MetaEmit::EncodeFinishMethodInstantiation()
{
    GetMethodInstantiation()->m_cbSizeSig = m_cbSignature - FIELD_OFFSET(struct GenericMethodInstantiation, m_pSignature);
}

//============================================================================
// Fill in all of the type instantation.
//============================================================================

void MetaEmit::EncodeFinishTypeInstantiation()
{
    GetTypeInstantiation()->m_cbSizeSig = m_cbSignature - FIELD_OFFSET(struct GenericTypeInstantiation, m_pSignature);
}

//============================================================================
// Revert back to an old signature.
//============================================================================

void MetaEmit::ReleaseSignature
(
    _In_ Signature *psig
)
{
    m_nraSignature.Free(&psig->m_mark);

    m_pbSignature = psig->m_pbSignature;
    m_cbSignature = psig->m_cbSignature;
}

//============================================================================
// Convert a variant to a COM+ constant.
//============================================================================
void MetaEmit::ConvertConstant
(
    _In_ ConstantValue *Value,
    _Out_ DWORD *pdwFlag,
    _Out_ void **ppData,
    _Out_ ULONG *pcbData
)
{
    switch (Value->TypeCode)
    {
    case t_bool:
        *pdwFlag = ELEMENT_TYPE_BOOLEAN;
        *ppData = &Value->Integral; // Note: bools in COM+ are 1 byte
        *pcbData = 1;
        return;

    case t_i1:
        *pdwFlag = ELEMENT_TYPE_I1;
        *ppData = &Value->Integral;
        *pcbData = 1;
        return;

    case t_ui1:
        *pdwFlag = ELEMENT_TYPE_U1;
        *ppData = &Value->Integral;
        *pcbData = 1;
        return;

    case t_i2:
        *pdwFlag = ELEMENT_TYPE_I2;
        *ppData = &Value->Integral;
        *pcbData = 2;
        return;

    case t_ui2:
        *pdwFlag = ELEMENT_TYPE_U2;
        *ppData = &Value->Integral;
        *pcbData = 2;
        return;

    case t_i4:
        *pdwFlag = ELEMENT_TYPE_I4;
        *ppData = &Value->Integral;
        *pcbData = 4;
        return;

    case t_ui4:
        *pdwFlag = ELEMENT_TYPE_U4;
        *ppData = &Value->Integral;
        *pcbData = 4;
        return;

    case t_i8:
        *pdwFlag = ELEMENT_TYPE_I8;
        *ppData = &Value->Integral;
        *pcbData = 8;
        return;

    case t_ui8:
        *pdwFlag = ELEMENT_TYPE_U8;
        *ppData = &Value->Integral;
        *pcbData = 8;
        return;

    case t_single:
        *pdwFlag = ELEMENT_TYPE_R4;
        *ppData = &Value->Single;
        *pcbData = 4;
        return;

    case t_double:
        *pdwFlag = ELEMENT_TYPE_R8;
        *ppData = &Value->Double;
        *pcbData = 8;
        return;

    case t_char:
        *pdwFlag = ELEMENT_TYPE_CHAR;
        *ppData = &Value->Integral;
        *pcbData = 2;
        return;

    case t_string:
        if (Value->String.Spelling)
        {
            *pdwFlag = ELEMENT_TYPE_STRING;
            *ppData = (void *)(Value->String.Spelling);
            // This is actually the length in characters that is wanted for string
            *pcbData = Value->String.LengthInCharacters;
        }
        else
        {
            // String = Nothing
            *pdwFlag = ELEMENT_TYPE_CLASS;
            *ppData = &Value->Integral;
            *pcbData = 4;
        }
        return;

    case t_date:
    case t_decimal:
        // Decimal/date is not representable in the metadata. Therefore, decimal
        // constant values are defined using custom attributes. This will be
        // handled after this is called.
        return;

    case t_ref:
    case t_array:
        // OPTIONAL:
        *pdwFlag = ELEMENT_TYPE_CLASS;
        *ppData = &Value->Integral;
        *pcbData = 4;
        return;

    case t_struct:
    case t_generic:
    {
         if (Value->Integral==0)
         {
             // encoded as nullref
             *pdwFlag = ELEMENT_TYPE_CLASS;
             *ppData = &Value->Integral;
             *pcbData = 4;
         }
         else
         {
             VSFAIL("don't know how to encode a non-default struct/generic");
         }
         return;
    }

    case t_bad:
        // If we have a bad constant, we shouldn't even be emitting metadata.
        VSFAIL("Can't convert bad constant");
        return;

    default:
        VSFAIL("unhandled case");
        return;
    }
}

void MetaEmit::EmitAllSecurityAttributes()
{
    PEBuilder *pPEBuilder = m_pBuilder->IsPEBuilder() ? static_cast<PEBuilder *>(m_pBuilder) : NULL;
    VSASSERT(pPEBuilder, "EmitAllSecurityAttributes() invoked for a builder other than PEBuilder");

    pPEBuilder->EmitAllSecurityAttributes();
}

/********************************************************************************
;CollectAssemblyLinks

Given a list of some project's references, create a list containing only the
embedded ones. We will use this list to make sure that none of the indirect
references ALink will pull in are going to collide with these embedded links.
********************************************************************************/
static void CollectAssemblyLinks(ReferencedProject *rgref, ULONG cRef, DynamicArray<AssemblyIdentity*> &list)
{
    list.Reset();
    for (unsigned int iRef = 0; iRef < cRef; iRef++)
    {
        if (rgref[iRef].m_fEmbedded)
        {
            list.AddElement(rgref[iRef].m_pCompilerProject->GetAssemblyIdentity());
        }
    }
}


/********************************************************************************
;ValidateAssemblyLink

We are emitting an assembly and are going to create a reference to some project.
Find each indirect reference this project implies and compare each reference to
the list of embedded references we created earlier, in CollectAssemblyLinks. If
one of this project's indirect references matches up with one of the embedded
references, we need to report a warning: the whole point of linking an assembly
is that we don't want our output assembly to have any reference to it.
********************************************************************************/
static void ValidateAssemblyLink
(
    _In_z_ STRING * pReferencingProjectAssemblyName,
    AssemblyIdentity *pAssemblyIdentityToValidate, 
    DynamicArray<AssemblyIdentity*> &list, 
    Compiler *pCompiler,
    CompilerHost * pCompilerHost,
    ErrorTable *pErrorTable
)
{
    // See whether the referenced assembly exists on our list of linked items.
    // If so, this indirect reference will cause our link to fail, because we'll end up with a
    // reference to the linked item anyway. This is related to bug #480600.
    for (unsigned int iList = 0; iList < list.Count(); iList++)
    {
        AssemblyComparisonResult acr = AssemblyIdentity::CompareAssemblies(
            pAssemblyIdentityToValidate,
            list.Element(iList),
            NULL,
            NULL,
            NULL,
            pCompiler,
            pCompilerHost);
        
        if (acr == ACR_EquivalentFullMatch ||
            acr == ACR_EquivalentWeakNamed)
        {
            pErrorTable->CreateError(
                WRNID_IndirectRefToLinkedAssembly2,
                NULL,
                pAssemblyIdentityToValidate->GetAssemblyName(),
                pReferencingProjectAssemblyName);
        }
    }
}


/********************************************************************************
;ValidateAssemblyLinks

We are emitting an assembly, A, which /references some assembly, B, and 
/links some other assembly, C, so that it can use C's types (by embedding them)
without having an assemblyref to C itself.
We can say that A has an indirect reference to each assembly that B references. 
When we emit A, we will emit assemblyrefs for both direct and indirect references.
In this function, we are looking for the situation where B has an assemblyref to C,
thus giving A an indirect reference to C. When we compile A, we will emit an 
assemblyref to C despite A's use of /link. This is probably not what the developer 
expected - the only reason to use /link is to avoid creating such an assemblyref -
so we will report a warning.
The 'project' parameter in this case is B, and the 'list' contains all of the
assemblies that A has /linked, such as C in our example.
********************************************************************************/
static void ValidateAssemblyLinks
(
 CompilerProject *project, 
 DynamicArray<AssemblyIdentity*> &list, 
 Compiler *compiler,
 ErrorTable *pErrorTable
)
{
    if ( project == NULL )
    {
        return;
    }

    if (project->IsMetaData())
    {
        MetaDataFileIterator iter(project);
        while (MetaDataFile *pMdf = iter.Next())
        {
            AssemblyRefCollection *pArc = pMdf->GetAssemblyRefs();
            for (ULONG iRef = 0; iRef < pArc->GetAssemblyRefCount(); iRef++)
            {
                AssemblyRefInfo *pAri = pArc->GetAssemblyRefInfo(iRef);

                ValidateAssemblyLink
                    (
                        project->GetAssemblyName(),
                        &pAri->m_Identity, 
                        list, 
                        compiler,
                        project->GetCompilerHost(),
                        pErrorTable
                    );
            }
        }
    }
    else
    {
        // Dev10 #652959 For a source project we need to iterate through references too.
        ReferenceIterator ReferencedProjects(project);
        CompilerProject *pReferencedCompilerProject;

        while (pReferencedCompilerProject = ReferencedProjects.Next())
        {
            if (!project->HasEmbeddedReferenceTo(pReferencedCompilerProject))
            {
                ValidateAssemblyLink
                    (
                        project->GetAssemblyName(),
                        pReferencedCompilerProject->GetAssemblyIdentity(), 
                        list, 
                        compiler,
                        project->GetCompilerHost(),
                        pErrorTable
                    );
            }
        }
    }
}

//============================================================================
// Re-import all referenced projects.  We imported these once when generating,
// typerefs but now we have to do it again so new ALink will know about them.
//============================================================================
void MetaEmit::ALinkImportReferences()
{
    ReferencedProject *rgref;
    ULONG cRef;
    ULONG iRef;

    VSASSERT(m_pALink, "Emitter created without ALink.");

    rgref = m_pBuilder->m_pCompilerProject->m_daReferences.Array();
    cRef  = m_pBuilder->m_pCompilerProject->m_daReferences.Count();
    DynamicArray<AssemblyIdentity*> linkedProjectList;
    CollectAssemblyLinks(rgref, cRef, linkedProjectList);

    DebCheckAssemblyOrModuleToken(m_mdAssemblyOrModule);

    for (iRef = 0; iRef < cRef; iRef++)
    {
        CompilerProject *pCompilerProject = rgref[iRef].m_pCompilerProject;

        if (pCompilerProject)
        {
            HRESULT hr;
            STRING *pstrFileName;
            CComPtr<IMetaDataAssemblyImport> spMetaDataImport;
            mdToken mdImportedFile;

            if (pCompilerProject->IsMetaData())
            {
                pstrFileName = pCompilerProject->GetFileName();
                spMetaDataImport = pCompilerProject->GetAssemblyImport();
            }
            else
            {
                pstrFileName = pCompilerProject->GetPEName();

#if IDE
                VSASSERT(!(m_pBuilder && m_pBuilder->IsENCBuilder()), "Can this ever fail?");

                if (m_pBuilder && m_pBuilder->IsENCBuilder())
                {
                    if (pCompilerProject->m_pENCBuilder)
                    {
                        spMetaDataImport = pCompilerProject->m_pENCBuilder->GetAssemblyImport();
                    }
                }
                else
#endif
                {
                    spMetaDataImport = pCompilerProject->m_Builder.GetAssemblyImport();
                }

                VSASSERT(pCompilerProject->GetCompState() == CS_Compiled, "The referenced project should be compiled.");
                VSASSERT(!pCompilerProject->IsCodeModule(), "We shouldn't be building code modules in the IDE.");
            }

            if (!rgref[iRef].m_fEmbedded)
            {
                // We are not going to emit a reference to any embedded assembly, so we don't need to check their
                // indirect references. It's ok to embed an assembly which has a reference to another embedded
                // assembly, and we can count on the existing check for ERRID_IndirectUnreferencedAssembly to ensure
                // that all of an assembly's references have been included in the list. The only case we need to check
                // for is the one where a non-linked assembly has an indirect reference to a linked assembly. 
                // This was Dev10 bug #616590.
                ValidateAssemblyLinks(pCompilerProject, linkedProjectList, m_pCompiler, m_pBuilder->m_pErrorTable);
            }
            VSASSERT(spMetaDataImport != NULL, "Ouch. What happened to the metadata import for this project?");

            hr = m_pALink->ImportFile2(
                    pstrFileName,   // IN - filename of the file to add (must be fully qualified)
                    NULL,           // IN - (optional) filename to copy bound modules to.
                    spMetaDataImport, // IN - (optional) assembly scope of pstrFileName
                    false,           // IN - fSmartImport
                    &mdImportedFile,// OUT- unique ID for the file - it could be an assembly or file
                    NULL,           // OUT- assembly import scope (NULL if the file is not an assembly)
                    NULL);          // OUT- pdwCountOfScopes

            if (!pCompilerProject->IsMetaData())
            {
                // set the PE kind so that ALink can emit the appropriate information and also
                // later use this information to warn on assembly mismatch based on platform
                // type.
                //
                IfFailThrow(m_pALink->SetPEKind(
                                m_mdAssemblyOrModule,                                               // IN - Unique ID for the assembly that this file belongs to
                                mdImportedFile,                                                     // IN - the file within this assembly
                                MapPlatformKindToPEKind(pCompilerProject->GetPlatformKind()),       // IN - The kind of PE (see CorPEKind enum in corhdr.h)
                                MapPlatformKindToMachineKind(pCompilerProject->GetPlatformKind())));// IN - Machine as defined in NT header
            }

            // ALink::ImportFile returns S_FALSE for modules, which need to be added to
            // the assembly via ALink::AddImport.  Only need to do this if building an assembly.
            if (hr == S_FALSE && FEmitAssembly())
            {
                mdFile mdFileImported;
                hr = m_pALink->AddImport(
                            m_mdAssemblyOrModule, // IN - Unique ID for the assembly to this file to
                            mdImportedFile,     // IN - imported file to add
                            ffContainsMetaData, // IN - COM+ FileDef flags
                            &mdFileImported);   // OUT- unique ID for the file (used to uniquely identify it)
            }

            if (FAILED(hr))
            {
                // Report the error, but continue on with next referenced project
                m_pBuilder->m_pErrorTable->CreateErrorWithError(
                                                ERRID_UnableToGenerateRefToMetaDataFile1,
                                                NULL,
                                                hr,
                                                pstrFileName);
            }
        }
    }

}

void MetaEmit::AlinkEmitAssemblyPlatformKind()
{
    IfFailThrow(m_pALink->SetPEKind(
        m_mdAssemblyOrModule,                                                   // IN - Unique ID for the assembly that this file belongs to
        m_mdFileForModule,                                                      // IN - the file within this assembly
        MapPlatformKindToPEKind(m_pCompilerProject->GetPlatformKind()),         // IN - The kind of PE (see CorPEKind enum in corhdr.h)
        MapPlatformKindToMachineKind(m_pCompilerProject->GetPlatformKind())));  // IN - Machine as defined in NT header
}

//============================================================================
// Uses ALink to emit a reference to the given CompilerProject.
//============================================================================
mdToken MetaEmit::ALinkEmitProjectRef
(
    CompilerProject *pCompilerProject
)
{
    HRESULT  hr = NOERROR;
    STRING  *pstrFileName;
    CComPtr<IMetaDataAssemblyImport> spMetaDataImport;
    mdToken  mdAssemblyOrModuleRef = mdTokenNil;
    mdToken  mdImportedFile;

    // First create ALink helper if none
    GetALinkHelper();

    if (pCompilerProject->IsMetaData())
    {
        pstrFileName = pCompilerProject->GetFileName();
        spMetaDataImport = pCompilerProject->GetAssemblyImport();
        VSASSERT(spMetaDataImport != NULL, "Ouch. What happened to the metadata import for this project?");
    }
    else
    {
        // We're going to get the IMetaDataAssemblyImport from the referenced project
        // and give it to ALink instead. This is because even though the
        // other project has progressed to CS_Compiled, it may not have been
        // written to disk yet.
        pstrFileName = pCompilerProject->GetPEName();


#if IDE
        if (m_pBuilder && m_pBuilder->IsENCBuilder())
        {
            // For the ENCBuilder case (delta gen), we reach here if the user added something from a project
            // that doesn't have a project ref in the PE being debugged.  We don't have an
            // assembly import to provide for the project (ALink will read it).
            //
            // Note: Future calls to LookupProjectRef (including subsequent delta gens in the debugging
            //       session) will be aware of the new project ref, so this code path won't be hit again
            //       for the same project within the debugging session
        }
        else
#endif
        {
            spMetaDataImport = pCompilerProject->m_Builder.GetAssemblyImport();
            VSASSERT(spMetaDataImport != NULL, "Ouch. What happened to the metadata import for this project?");
        }

        VSASSERT(pCompilerProject->GetCompState() == CS_Compiled, "The referenced project should be compiled.");
        VSASSERT(!pCompilerProject->IsCodeModule(), "We shouldn't be building code modules in the IDE.");
    }

    // Importing the file just makes ALink remember that the assembly/module
    // we're building will eventually need to make a reference to this file
    // (which is itself a module or assembly)
    hr = m_pALink->ImportFile2(
            pstrFileName,   // IN - filename of the file to add (must be fully qualified)
            NULL,           // IN - (optional) filename to copy bound modules to.
            spMetaDataImport, // IN - (optional) assembly scope for pstrFileName
            false,           // IN - fSmartImport
            &mdImportedFile,// OUT- unique ID for the file - it could be an assembly or file
            NULL,           // OUT- assembly import scope (NULL if the file is not an assembly)
            NULL);          // OUT- pdwCountOfScopes

    if (SUCCEEDED(hr))
    {
        // START HACK:Microsoft - alink hack blessed by Microsoft
        //
        // Set the PE kind to "not a PE" and machine to unknown so that ALink will not check this
        // import later on for platform mismatches.
        //
        // HACK HACK - For lack of a better mechanism to tell alink to suppress platform mismatch
        // for some imports, we need to trick alink telling it that this assembly is not a PE (peNot).
        //
        IfFailThrow(m_pALink->SetPEKind(
                        m_mdAssemblyOrModule,        // IN - Unique ID for the assembly that this file belongs to
                        mdImportedFile,              // IN - the file within this assembly
                        peNot,                       // IN - The kind of PE (see CorPEKind enum in corhdr.h)
                        IMAGE_FILE_MACHINE_UNKNOWN));// IN - Machine as defined in NT header
        // END HACK

        // Calling GetResolutionScope forces ALink to emit the reference now.
        // We use AssemblyIsUBM to trick ALink into not building an assembly.
        // All we want it to do is emit the reference.
        hr = m_pALink->GetResolutionScope(
                    m_mdAssemblyOrModule,   // IN - Unique ID for the assembly
                    m_mdFileForModule,      // IN - file that is needing a reference
                    mdImportedFile,         // IN - token from ImportFile that the type is defined in
                    &mdAssemblyOrModuleRef);// OUT- AssemblyRef or ModuleRef
    }

    if (SUCCEEDED(hr))
    {
        VSASSERT(!IsNilToken(mdAssemblyOrModuleRef), "Bogus ALink output!");
        
        // Dev10 #733519; ensure that a moduleRef isn't returned for assemblies
        mdToken tokenType = TypeFromToken(mdAssemblyOrModuleRef);

        AssertIfFalse(pCompilerProject->IsMetaData() || !pCompilerProject->OutputIsNone() || m_pCompilerProject->OutputIsNone());
        
        if ((pCompilerProject->IsCodeModule() && tokenType != mdtModuleRef) ||
            (!pCompilerProject->IsCodeModule() && tokenType != mdtAssemblyRef))
        {
            // Dev10 #773908 It looks like we don't emit manifest when OutputIsNone,
            // it should be OK to get mdtModuleRef token for source projects in this case.
            if (pCompilerProject->IsMetaData() || !(pCompilerProject->OutputIsNone() && tokenType == mdtModuleRef))
            {
                m_pBuilder->m_pErrorTable->CreateErrorWithError(
                                                ERRID_UnableToGenerateRefToMetaDataFile1,
                                                NULL,
                                                hr,
                                                pstrFileName);
                mdAssemblyOrModuleRef = mdTokenNil;

                VSFAIL("Unexpected tokenType!");
            }
        }
    }
    else
    {
        // Report the error, but continue on with next referenced project
        m_pBuilder->m_pErrorTable->CreateErrorWithError(
                                        ERRID_UnableToGenerateRefToMetaDataFile1,
                                        NULL,
                                        hr,
                                        pstrFileName);
    }

    return mdAssemblyOrModuleRef;
}


//============================================================================
// Emit any linked or embedded COM+ resources (Win32 resources
// are handled separately)
//============================================================================
void MetaEmit::ALinkEmitResources
(
    ICeeFileGen *pFileGen,
    HCEEFILE     hCeeFile,
    HCEESECTION  hCeeSection
#if IDE 
   ,bool         fEmbeddedOnly
#endif IDE
)
{
#if IDE 
    if (fEmbeddedOnly &&
        m_pBuilder->m_pCompilerProject->m_fEmbeddedResourcesEmitted)
    {
        // Return if embedded resources are being requested to be emitted
        // and they have already been emitted previously.
        return;
    }

    VSASSERT(!m_pBuilder->m_pCompilerProject->m_fEmbeddedResourcesEmitted, "Embedded resources already emitted?");
#endif IDE

    // Calc offset of embedded resources, and their total size.  (Linked resources
    // are considered to have a size of zero.)
    ULONG ibResourceOffset;
    ULONG cbResourceTotal = 0;
    IfFailThrow(pFileGen->GetSectionDataLen(hCeeSection, &ibResourceOffset));
    IfFailThrow(pFileGen->GetMethodRVA(hCeeFile, ibResourceOffset, &ibResourceOffset));

    // Some temps to walk through the array of resources
    Resource *presourceFirst = m_pBuilder->m_pCompilerProject->m_daResources.Array();
    Resource *presourceMax  = presourceFirst + m_pBuilder->m_pCompilerProject->m_daResources.Count();
    Resource *presourceNext;

#if (IDE ) && DEBUG
    // Assumption: Any newly introduced resource gets added to the end of the dynamic array m_daResources.
    // Assert for this.
    if (fEmbeddedOnly)
    {
        bool fSeenEmbeddedResource = false;
        for (presourceNext = presourceFirst; presourceNext < presourceMax; presourceNext++)
        {
            if (presourceNext->m_fEmbed)
            {
                fSeenEmbeddedResource = true;
            }
            else
            {
                VSASSERT(!fSeenEmbeddedResource, "New resource should be added at the end of the resource array!!!");
            }
        }
    }
#endif (IDE ) && DEBUG

    // Check for name collisions between any resources in this PE.
    // This is O(n^2), but n should always be very small.
    for (presourceNext = presourceFirst; presourceNext < presourceMax; presourceNext++)
    {
        Resource *presourceCompare;
        for (presourceCompare = presourceNext + 1; presourceCompare < presourceMax; presourceCompare++)
        {
#if IDE 
            // Assumption: Any newly introduced resource gets added to the end of the dynamic array m_daResources.
            // Already assert for this earlier in this function.
            if (fEmbeddedOnly && !presourceCompare->m_fEmbed)
            {
                continue;
            }
#endif IDE

            if (StringPool::IsEqual(presourceCompare->m_pstrName, presourceNext->m_pstrName))
            {
                m_pBuilder->m_pErrorTable->CreateError(
                                                ERRID_DuplicateResourceName1,
                                                NULL,
                                                presourceCompare->m_pstrName);
            }
        }
    }


    // Emit resources one at a time
    for (presourceNext = presourceFirst ; presourceNext < presourceMax; presourceNext++)
    {
#if IDE 
        if (!presourceNext->m_fEmbed)
        {
            if (fEmbeddedOnly)
            {
                continue;
            }
        }
        else
        {
            m_pBuilder->m_pCompilerProject->m_fEmbeddedResourcesEmitted = true;
        }
#endif IDE

        cbResourceTotal += ALinkEmitOneResource(
                                pFileGen,
                                hCeeSection,
                                presourceNext,
                                cbResourceTotal,
                                m_pBuilder->m_pErrorTable);
    }

    // Mark beginning of resource blob within IL section.  (We emitted
    // resources with offsets relative to this mark.)
    //
    IfFailThrow(pFileGen->SetManifestEntry(hCeeFile, cbResourceTotal, ibResourceOffset));
}

//============================================================================
// Emits one embedded or linked resource.  For embedded resources, returns
// the size of the resource; else returns 0.
//============================================================================
ULONG MetaEmit::ALinkEmitOneResource
(
    ICeeFileGen *pFileGen,
    HCEESECTION  hCeeSection,
    _In_ Resource    *presource,
    DWORD        ibThisResource,    // Byte offset of this resource
    ErrorTable  *pErrorTable
)
{
    VSASSERT(pErrorTable != NULL, "ErrorTable required.");
    VSASSERT(m_pALink, "Emitter created without ALink.");

    HRESULT hr = NOERROR;
    ULONG   cbResource = 0;
    DWORD   dwAccess   = presource->m_fPublic ? mrPublic : mrPrivate;

    if (presource->m_fEmbed)
    {
        // We're embedding the resource, so map the file
        // and copy its contents into the PE.
        MapFile mapfile;

        mapfile.Map(presource->m_pstrFile, pErrorTable);
        if (mapfile.FMapped())
        {
            DWORD   cbFile;
            void   *pvBuffer;

            cbFile = mapfile.CbFile();

            // Size of the resource includes 4-byte size prefix
            cbResource = cbFile + sizeof(DWORD);
            IfFalseThrow(cbResource >= cbFile);

            // Write size of resource (in bytes) followed by resource's bits
            hr = pFileGen->GetSectionBlock(hCeeSection, cbResource, 1, &pvBuffer);
            if (SUCCEEDED(hr))
            {
                memcpy(pvBuffer, &cbFile, sizeof(DWORD));
                memcpy((PBYTE)pvBuffer + sizeof(DWORD), mapfile.PbMapAddr(), cbFile);

                hr = m_pALink->EmbedResource(
                    m_mdAssemblyOrModule,     // IN - Unique ID for the assembly
                    m_mdAssemblyOrModule,     // IN - FileToken or AssemblyID of file that has the resource
                    presource->m_pstrName,  // IN - name of resource
                    ibThisResource,         // IN - Offset of resource from RVA
                    dwAccess);              // IN - Flags, mrPublic, mrPrivate, etc.
            }

            if (FAILED(hr))
            {
                m_pBuilder->m_pErrorTable->CreateErrorWithError(
                                                ERRID_UnableToEmbedResourceFile1,
                                                NULL,
                                                hr,
                                                presource->m_pstrFile);
            }
        }
    }
    else
    {
        // We're linking the resource.  ALink handles this entire task.
        hr = m_pALink->LinkResource(
                        m_mdAssemblyOrModule,     // IN - Unique ID for the assembly
                        presource->m_pstrFile,  // IN - name of file
                        NULL,                   // IN - (optional) new filename
                        presource->m_pstrName,  // IN - name of resource
                        dwAccess);              // IN - Flags, mrPublic, mrPrivate, etc.

        if (FAILED(hr))
        {
            m_pBuilder->m_pErrorTable->CreateErrorWithError(
                                            ERRID_UnableToLinkResourceFile1,
                                            NULL,
                                            hr,
                                            presource->m_pstrFile);
        }
    }

    return cbResource;
}


//============================================================================
// Emit any module- or assembly-level custom attributes via ALink and COM+.
//============================================================================
void MetaEmit::ALinkEmitAssemblyAttributes()
{
    HRESULT             hr = NOERROR;
    CompilerProject    *pCompilerProject = m_pBuilder->m_pCompilerProject;
    ErrorTable         *pErrorTable      = m_pBuilder->m_pErrorTable;
    NorlsAllocator      nraTemp(NORLSLOC);  // Avoid bloating the PEBuilder's allocator
    CompilerFile       *pCompilerFile;
    int                 cModAttr = 0;       // Total count of module-level attributes
    int                 iModAttr = 0;       // Index of module-level attribute currently being processed
    bool                fAnyAttr = false;   // Did we find any attributes at all?
    BCSYM_Namespace    *psymNamespace;
    BCSYM_ApplAttr     *psymApplAttr;
    bool                fSecurity;
    mdModule            mdModuleForAttrs = mdTokenNil; // Module token for module-level attributes
    STRING             *pstrVersionNumber = NULL;   // Version string handed to ALink

    PEBuilder*          pPEBuilder = m_pBuilder->IsPEBuilder() ? static_cast<PEBuilder *>(m_pBuilder) : NULL;
    VSASSERT(pPEBuilder, "ALinkEmitAssemblyAttributes() invoked for a builder other than PEBuilder");

    // Begin new signature
    NewMetaEmitSignature signature(this);

    // Local struct needed to flag duplicate module-level attributes
    struct ModuleAttr
    {
        BCSYM_ApplAttr *m_pApplAttr;        // The applied attribute
        BCSYM_Class    *m_psymAttrClass;    // The attribute class it resolved to
    };
    ModuleAttr *prgModuleAttr = NULL;

    VSASSERT(pErrorTable != NULL, "ErrorTable required.");

    bool fCompilationRelaxationsSeen = false;
    bool fDebuggableAttributeSeen = false;
    bool fRuntimeCompatibilityAttributeSeen = false;
    bool fExtensionAttributeSeen = false;
    bool fShouldEmitExtensionAttribute = false;

    // Loop through all the SourceFiles and count all of the attributes
    // and module-level attributes.
    CDoubleListForwardIter<CompilerFile> iter(&pCompilerProject->m_dlFiles[CS_Compiled]);
    while (pCompilerFile = iter.Next())
    {
        if (pCompilerFile->PSourceFile()->ContainsExtensionMethods())
        {
            fShouldEmitExtensionAttribute = true;
        }

        psymNamespace = pCompilerFile->PSourceFile()->GetRootNamespace();

        if (psymNamespace)
        {
            // Look for the System.Diagnostics.DebuggableAttribute
            if (psymNamespace->GetPWellKnownAttrVals()->GetDebuggableData())
            {
                fDebuggableAttributeSeen = true;
            }

            if (psymNamespace->GetPWellKnownAttrVals()->GetCompilationRelaxationsData())
            {
                fCompilationRelaxationsSeen = true;
            }

            if (psymNamespace->GetPWellKnownAttrVals()->GetRuntimeCompatibilityData())
            {
                fRuntimeCompatibilityAttributeSeen = true;
            }

            if (psymNamespace->GetPWellKnownAttrVals()->GetExtensionData())
            {
                fExtensionAttributeSeen = true;
            }


            // Loop over all attributes stored on this symbol
            BCITER_ApplAttrs ApplAttrsIter(psymNamespace);

            while (psymApplAttr = ApplAttrsIter.GetNext())
            {
                fAnyAttr = true;
                if (psymApplAttr->IsModule() && pPEBuilder &&
                    pPEBuilder->CheckConditionalAttributes(
                        psymApplAttr,
                        pCompilerFile->PSourceFile()))
                {
                    cModAttr++;
                }
            }
        }
    }


    int AssemblyFlags = pCompilerProject->GetAssemblyIdentity()->GetAssemblyFlags();
#if IDE
    AssemblyFlags |= pCompilerProject->GetOutputType() == OUTPUT_WinMDObj ? afContentType_WindowsRuntime : 0;
#endif
    if (AssemblyFlags != 0)
    {
        ALinkEmitAssemblyFlagsAttribute(AssemblyFlags, pCompilerProject->GetAssemblyIdentity()->GetAssemblyFlagsAttributeCtor());
    }


    if (!fDebuggableAttributeSeen &&
        !pCompilerProject->IsCodeModule())
    {
        ALinkEmitDebuggableAttribute();
    }

    if (!pCompilerProject->IsCodeModule())
    {
        if (!fCompilationRelaxationsSeen &&
            pCompilerProject->GetCompilerHost()->GetFXSymbolProvider()->IsTypeAvailable(FX::CompilationRelaxationsAttributeType))
        {
            ALinkEmitCompilationRelaxationsAttribute();
        }

        if (!fRuntimeCompatibilityAttributeSeen &&
            pCompilerProject->GetCompilerHost()->GetFXSymbolProvider()->IsTypeAvailable(FX::RuntimeCompatibilityAttributeType))
        {
            ALinkEmitRuntimeCompatibilityAttribute();
        }
    }

    if (fShouldEmitExtensionAttribute && ! fExtensionAttributeSeen)
    {
        Declaration * pExtensionAttributeCtor =
            Semantics::GetExtensionAttributeCtor
            (
                m_pCompiler,
                m_pCompilerProject->GetCompilerHost(),
                m_pCompilerProject->m_pfileList->PSourceFile(),
                &nraTemp,
                NULL
            );

        if (pExtensionAttributeCtor)
        {
            unsigned char aBlob[4];

            aBlob[0] = 0x1;
            aBlob[1] = 0x0;
            aBlob[2] = 0x0;
            aBlob[3] = 0x0;

            mdMemberRef attr =
                                            DefineMemberRefBySymbol
                                                        (
                                                                pExtensionAttributeCtor,
                                                                NULL,
                                                                NULL
                                                        ) ;

            // Use ALink to emit the attribute (Bug #56115 - DevDiv Bugs)
            HRESULT hr1 =

                                    m_pALink->EmitAssemblyCustomAttribute(
                                                m_mdAssemblyOrModule, // IN - Unique ID for the assembly
                                                m_mdFileForModule,    // IN - file that is defining the property
                                                attr,   // IN - Type of the CustomAttribute.
                                                &aBlob[0],              // IN - The custom value data.
                                                sizeof(aBlob),      // IN - The custom value data length.
                                                false,              // IN - True if the CA is a security attribute
                                                false);            // IN - True if AllowMultiple=true

            IfFailThrow(hr1);
        }
    }

    if (!fAnyAttr)
    {
        // No work to do
        goto LDone;
    }

    if (cModAttr > 0)
    {
        // Need to query COM+ to get the module token
        IMetaDataImport2 *pMDImport = NULL;
        IfFailThrow(m_pmdEmit->QueryInterface(IID_IMetaDataImport2, (void **)&pMDImport));
        IfFailThrow(pMDImport->GetModuleFromScope(&mdModuleForAttrs));
        VSASSERT(!IsNilToken(mdModuleForAttrs), "Bad COM+ output!");
        RELEASE(pMDImport);

        // Need to keep a list of all of the module-level attributes
        // we've seen so we can report dups
        prgModuleAttr = (ModuleAttr *)nraTemp.Alloc(VBMath::Multiply(
            sizeof(ModuleAttr), 
            cModAttr));
    }

    // Loop through all the SourceFiles of this project and emit all
    // assembly- and module-level attributes found there.
    iter.Reset();
    while (pCompilerFile = iter.Next())
    {
        SourceFile *pSourceFile = pCompilerFile->PSourceFile();

        psymNamespace = pSourceFile->GetRootNamespace();
        VSASSERT(psymNamespace != NULL, "Where's this SourceFile's root name namespace?");

        // Loop over all attributes stored on this symbol
        BCITER_ApplAttrs ApplAttrsIter(psymNamespace);

        while (psymApplAttr = ApplAttrsIter.GetNext())
        {
            mdTypeRef           mdtyperefAttrCtor;

            // Bound tree
            ILTree::Expression     *ptreeExprBound;

            // Need all this ---- to convert the bound tree to a COM+ blob.
            // 

            STRING      *pstrAttrName;
            BCSYM_Class *psymAttrClass;
            BCSYM_Proc  *psymAttrCtor;
            BYTE        *pbArgBlob;
            ULONG        cbArgBlob;
            ULONG        cPosParams;
            ULONG        cbSignature = 0;
            COR_SIGNATURE *pSignature = NULL;
            bool         fError;

            // Need all of these to see if the attribute is AllowMultiple or not
            CorAttributeTargets attrtargets;
            bool fAllowMultiple;
            bool fInherited;

            if (pPEBuilder && !pPEBuilder->CheckConditionalAttributes(psymApplAttr, pSourceFile))
            {
                continue;
            }

            // Get the bound tree for this applied attribute
            ptreeExprBound = psymApplAttr->GetBoundTree();

            // If ptreeExprBound, errors have been reported through pErrorTable
            if (!ptreeExprBound)
            {
                continue;
            }

            Attribute::EncodeBoundTree(&ptreeExprBound->AsAttributeApplicationExpression(), this,
                                    &nraTemp, m_pCompiler, pCompilerProject, false, &pstrAttrName, &psymAttrClass,
                                    &psymAttrCtor, &pbArgBlob, &cbArgBlob, &cPosParams, &pSignature, &cbSignature, &fError, pErrorTable);

            VSASSERT(!fError, "Shouldn't fail on anything that got past semantic analysis.");

            if (psymAttrClass == pCompilerProject->GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::AssemblyVersionAttributeType))
            {
                if (!pCompilerProject->OutputIsModule() &&
                    !pCompilerProject->m_AssemblyIdentity.ErrorInVersionString())
                {
                    // AssemblyVersion attribute for non-modules and correct version string
                    // is output later in ALinkSetAssemblyProps.
                    continue;
                }

                // Report any alink errors in version string.
#if IDE 
               VSASSERT(!pCompilerProject->OutputIsModule(), "IDE compiler does not support building .Net modules!!!");
#endif IDE
            }

            if (psymAttrClass == pCompilerProject->GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::AssemblyFlagsAttributeType))
            {
                continue;
            }

            // Is this a security attribute, or a normal attribute?
            fSecurity = psymAttrClass->IsSecurityAttribute();

            // Get the error table belong to the file in which the error needs to be reported
            ErrorTable *pCurrentErrorTable =
                ApplAttrsIter.GetNamedContextForCurrentApplAttr()->GetErrorTableForContext();

            // Get a reference to the attribute's constructor
            // 
            mdtyperefAttrCtor = DefineMemberRefBySymbol(psymAttrCtor, NULL, psymApplAttr->GetLocation(), pCurrentErrorTable);

            if (psymApplAttr->IsAssembly())
            {
                //
                // All assembly-level attributes are emitted through ALink
                //

                // Microsoft:
                // If we have both file and name, emit only the file attribute.
                // Otherwise, ALINK gets confused.

                if (::wcscmp(psymAttrClass->GetQualifiedName(), ASSEMBLYKEYNAMEATTRIBUTE) == 0 &&
                    pCompilerProject->GetKeyContainerName() != NULL &&
                    pCompilerProject->GetKeyFileName() != NULL)
                {
                    // Skip AssemblyKeyNameAttribute since we will set AssemblyKeyFileAttribute.
                    continue;
                }

                // Determine if this attribute is AllowMultiple
                if (!psymAttrClass->GetPWellKnownAttrVals()->GetAttributeUsageData(
                                                            &attrtargets,
                                                            &fAllowMultiple,
                                                            &fInherited))
                {
                    // Badly formed attribute has already been flagged, so just skip.
                    continue;
                }

                hr = m_pALink->EmitAssemblyCustomAttribute(
                                    m_mdAssemblyOrModule,     // IN - Unique ID for the assembly
                                    m_mdFileForModule,        // IN - file that is defining the property
                                    mdtyperefAttrCtor,      // IN - Type of the CustomAttribute.
                                    pbArgBlob,              // IN - The custom value data.
                                    cbArgBlob,              // IN - The custom value data length.
                                    fSecurity,              // IN - True if the CA is a security attribute
                                    fAllowMultiple);        // IN - True if AllowMultiple=true

                // Duplicate assembly-level attribute with AllowMultiple=false.  However,
                // all instances specified the same parameter values, so emit a warning
                // instead of an error.
                if (hr == META_S_DUPLICATE)
                {
                pCurrentErrorTable->CreateError(
                                        WRNID_DuplicateAssemblyAttribute1,
                                        &ptreeExprBound->Loc,
                                        pstrAttrName);
                }
                else if (FAILED(hr))
                {
                    pCurrentErrorTable->CreateErrorWithError(
                                    ERRID_BadAssemblyAttribute1,
                                    &ptreeExprBound->Loc,
                                    hr,
                                    pstrAttrName);
                }
            }
            else if (!fSecurity)
            {
                mdCustomAttribute customattr;

                VSASSERT(psymApplAttr->IsModule(), "Should be a module-level attribute!");

                // If this has AllowMultiple=false, store for dup checking
                CorAttributeTargets attrtargets2;
                bool fAllowMultiple2;
                bool fInherited2;
                if (psymAttrClass->GetPWellKnownAttrVals()->GetAttributeUsageData(&attrtargets2, &fAllowMultiple2, &fInherited2) &&
                    !fAllowMultiple2)
                {
                    prgModuleAttr[iModAttr].m_pApplAttr = psymApplAttr;
                    prgModuleAttr[iModAttr].m_psymAttrClass = psymAttrClass;

                    //
                    // Now dup check this module-level attribute with all previous ones
                    //
                    for (int iModAttrCheck = 0; iModAttrCheck < iModAttr; iModAttrCheck++)
                    {
                        if (prgModuleAttr[iModAttrCheck].m_psymAttrClass ==
                            prgModuleAttr[iModAttr].m_psymAttrClass)
                        {
                            pCurrentErrorTable->CreateErrorWithSymbol(
                                        ERRID_DuplicateModuleAttribute1,
                                        prgModuleAttr[iModAttrCheck].m_pApplAttr,
                                        prgModuleAttr[iModAttr].m_psymAttrClass->GetErrorName(m_pBuilder->m_pCompiler));
                        }
                    }

                    // Need to delay this increment to avoid lots of "-1"'s in the above loop
                    iModAttr++;
                }

                //
                // All module-level non-security attributes are emitted through COM+
                //
                hr = m_pmdEmit->DefineCustomAttribute(
                                    mdModuleForAttrs,
                                    mdtyperefAttrCtor,
                                    pbArgBlob,
                                    cbArgBlob,
                                    &customattr);

                VSASSERT(!IsNilToken(customattr), "Bad COM+ output!");

                if (FAILED(hr))
                {
                    pCurrentErrorTable->CreateErrorWithError(
                                        ERRID_BadModuleAttribute1,
                                        &ptreeExprBound->Loc,
                                        hr,
                                        pstrAttrName);
                }

            }
            else
            {
                //
                // Module-level security attributes are disallowed by COM+
                //
                pCurrentErrorTable->CreateError(
                                ERRID_ModuleSecurityAttributeNotAllowed1,
                                &ptreeExprBound->Loc,
                                pstrAttrName);
            }

        }   // For all assembly- or module-level attributes in file

    }   // For all files in project

LDone:
    ;
}

//============================================================================
// Set all of the assembly identity properties.  For the most part,
// these come from command-line options.  Do this after ALinkEmitAssemblyAttributes
// so that command-line options over-ride random custom attributes.
// Note that some of the assembly properties end up in the Win32
// resource blob, so do this before ALinkCreateWin32ResFile.
//============================================================================

// If this compile-time assert fails, the switch statement below needs updating.
COMPILE_ASSERT(optLastAssemOption == 20);

void MetaEmit::ALinkSetAssemblyProps()
{
    VSASSERT(m_pALink, "Emitter created without ALink.");

    HRESULT           hr;
    CComVariant       ccomvariant;
    CompilerProject  *pCompilerProject;

    DebCheckAssemblyOrModuleToken(m_mdAssemblyOrModule);

    pCompilerProject = m_pBuilder->m_pCompilerProject;

    //
    // Call SetAssemblyProps for each possible assembly option
    //
    for (int iOpt = 0; iOpt < optLastAssemOption; iOpt++)
    {
        switch (iOpt)
        {
            case optAssemTitle:               // String
            case optAssemDescription:         // String
            case optAssemFileVersion:         // String
            case optAssemSatelliteVer:        // String
            case optAssemSignaturePublicKey:  // String
                break;

            case optAssemConfig:              // String
            case optAssemOS:                  // String "dwOSPlatformId.dwOSMajorVersion.dwOSMinorVersion"
            case optAssemProcessor:           // ULONG
            case optAssemLocale:              // String
                break;

            case optAssemVersion:             // String "Major.Minor.Build.Revision"
                if (!pCompilerProject->OutputIsModule() &&
                    !pCompilerProject->m_AssemblyIdentity.ErrorInVersionString())
                {
                    ccomvariant = pCompilerProject->m_AssemblyIdentity.GetAssemblyVersion();
                }
                break;

            case optAssemCompany:             // String
            case optAssemProduct:             // String
            case optAssemProductVersion:      // String (aka InformationalVersion)
            case optAssemCopyright:           // String
            case optAssemTrademark:           // String
            case optAssemAlgID:               // ULONG
            case optAssemFlags:               // ULONG
                // These are all handled as assembly-level attributes in
                // the program's sources.  No need to special-case them here.
                break;

            case optAssemKeyFile:             // String (filename)
                if (pCompilerProject->GetKeyFileName())
                {
                    ccomvariant = pCompilerProject->GetKeyFileName();
                }
                break;

            case optAssemKeyName:             // String
                // Microsoft: If we have both a file and name, we prefer the file.
                if (pCompilerProject->GetKeyContainerName() && pCompilerProject->GetKeyFileName() == NULL)
                {
                    ccomvariant = pCompilerProject->GetKeyContainerName();
                }
                break;

            case optAssemHalfSign:            // Bool   (aka DelaySign)
                if (pCompilerProject->GetDelaySign())
                {
                    ccomvariant = pCompilerProject->GetDelaySign();
                }
                break;

            default:
                VSFAIL("Need to add code for new ALink AssemblyOptions?");
                break;
        }

        if (ccomvariant.vt != VT_EMPTY)
        {
            hr = m_pALink->SetAssemblyProps(
                            m_mdAssemblyOrModule,
                            FEmitAssembly() ? m_mdAssemblyOrModule : m_mdFileForModule,
                            (AssemblyOptions)iOpt,
                            ccomvariant);

            if (FAILED(hr))
            {
                // ALink does a good job of returning descriptive error
                // strings.  No need to embellish 'em here.
                m_pBuilder->m_pErrorTable->CreateErrorWithError(
                                                ERRID_ErrorSettingManifestOption,
                                                NULL,
                                                hr);
            }

            ccomvariant.Clear();
        }
    }   // For all AssemOptions
}


//============================================================================
// Retrieves the Win32 resource blob created by ALink and saves
// it to a temp file.  The name of the temp file is returned
// via the ppstrTempFile OUT param.
//============================================================================
void MetaEmit::ALinkCreateWin32ResFile
(
    bool                    fDLL,                 // IN:  Are we building a DLL or an EXE?
    _In_z_ STRING           *pstrIconFileName,    // IN:  Optional name of file containing Win32 icon data
    _In_ size_t lengthOfTempFileBuffer,           // IN:  Length of ppstrTempFile buffer
    _Deref_out_z_ WCHAR    *wszTempFileBuffer     // OUT: Temp file that contains saved resource blob
)
{
    VSASSERT(m_pALink, "Emitter created without ALink.");
    VSASSERT(lengthOfTempFileBuffer >= MAX_PATH + 1, "TempFile Buffer is too small");

    HRESULT     hr = NOERROR;
    const void *pvResourceBlob = NULL;
    DWORD       cbResourceBlob;
    DWORD       cbWritten;
    DWORD       dwSizeOfTempPath;
    HANDLE      hFile = INVALID_HANDLE_VALUE;

    // Clear OUT param
    wszTempFileBuffer[0] = NULL;

    // ALink gives a decent error message for bad/missing icon file,
    // so we don't need to do any validation here.

    // Get the Win32 blob from ALink
    hr = m_pALink->GetWin32ResBlob(
                    m_mdAssemblyOrModule, // IN - Unique ID for the assembly
                    m_mdAssemblyOrModule, // IN - file that is using the RES file
                    fDLL,               // IN - Is the file a Dll or EXE?
                    pstrIconFileName,   // IN - icon to insert into the RES blob
                    &pvResourceBlob,    // OUT- Pointer to RES blob
                    &cbResourceBlob);   // OUT- Size of RES blob

    if (FAILED(hr))
    {
        m_pBuilder->m_pErrorTable->CreateErrorWithError(
                            ERRID_ErrorCreatingWin32ResourceFile,
                            NULL,
                            hr);
        hr = NOERROR;
        goto Error;
    }

#pragma warning (push)
#pragma warning (disable:6309 6387) // This call is correct
    dwSizeOfTempPath = GetTempPath(0, NULL);
#pragma warning (pop)

    if (dwSizeOfTempPath)
    {
        TEMPBUF(wszTempFilePath, WCHAR *, (dwSizeOfTempPath) * sizeof(WCHAR));
        IfNullThrow(wszTempFilePath);
        GetTempPath(dwSizeOfTempPath, wszTempFilePath);

        StringBuffer sb;
        sb.AppendString(wszTempFilePath);
        hr = TemporaryFile::GetUniqueTemporaryFileName(sb, L"vbc");

        PCWSTR wszTempFile = sb.GetString();

        if (SUCCEEDED(hr))
        {
            if (sb.GetStringLength() >= MAX_PATH)
            {
                // We could insert "\\?\" at the beginning of the string, however,
                // the file path generated here is passed out to the caller and we
                // cannot be certain that the caller will be able to handle it. In
                // particular, this path ends up being passed out to alink, which
                // likely makes MAX_PATH assumptions. It is reasonable to error
                // here anyway, given that the Windows shell UI will prevent you
                // from creating paths that are too long.
                //
                // This is also consistent with the previous behavior we had with
                // GetTempFileName, and we return the same error it would have.
                hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
            }
            else
            {
                // Open the file.
                //
                // NB: We used to use GetTempFileName to get the temp file name
                // but now get it from a GUID to fix a stress issue.  As a result,
                // the file has not been created yet, so instead of using
                // OPEN_EXISTING here we now use CREATE_ALWAYS.
                hFile = CreateFile(wszTempFile, GENERIC_WRITE, 0, NULL,
                                   CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);

                if (hFile == INVALID_HANDLE_VALUE)
                {
                    hr = GetLastHResultError();
                }
            }
        }

        if (FAILED(hr))
        {
            m_pBuilder->m_pErrorTable->CreateErrorWithError(
                                ERRID_UnableToCreateTempFileInPath1,
                                NULL,
                                hr,
                                wszTempFilePath);
            goto Error;
        }

        // Now write the blob to the file.
        if (WriteFile(hFile, pvResourceBlob, cbResourceBlob, &cbWritten, NULL) == 0 ||
            cbResourceBlob != cbWritten)
        {
            DeleteFile(wszTempFile);

            hr = GetLastHResultError();
            m_pBuilder->m_pErrorTable->CreateErrorWithError(
                                ERRID_ErrorSavingWin32ResourceFile,
                                NULL,
                                hr,
                                wszTempFile);
            goto Error;
        }
        else
        {
            // Everything succeeded -- set the OUT param
            errno_t result = wcscpy_s(wszTempFileBuffer, lengthOfTempFileBuffer, wszTempFile);
            IfFalseThrow(!result && *wszTempFileBuffer);
        }
    }
    else
    {
        hr = GetLastHResultError();
        m_pBuilder->m_pErrorTable->CreateErrorWithError(
                            ERRID_UnableToGetTempPath,
                            NULL,
                            hr);
        goto Error;
    }

Error:
    if (pvResourceBlob != NULL)
    {
        IfFailThrow(m_pALink->FreeWin32ResBlob(&pvResourceBlob));
    }

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
    }
}


//============================================================================
// Emits the assembly's manifest via ALink
//============================================================================
void MetaEmit::ALinkEmitManifest
(
    ICeeFileGen *pFileGen,
    HCEEFILE     hCeeFile,
    HCEESECTION  hCeeSection
)
{
    HRESULT     hr;
    DWORD       cbReserveForSigning = 0;

    VSASSERT(FEmitAssembly(), "Not emitting an assembly?");
    VSASSERT(m_pALink, "Emitter created without ALink.");

    DebCheckAssemblyOrModuleToken(m_mdAssemblyOrModule);

    // Call ALink's EmitManifest to emit the manifest
    hr = m_pALink->EmitManifest(m_mdAssemblyOrModule, &cbReserveForSigning, NULL);
    if (FAILED(hr))
    {
        m_pBuilder->m_pErrorTable->CreateErrorWithError(ERRID_ErrorCreatingManifest, NULL, hr);
    }

    //
    // Reserve section in the file for public key, so it can be signed later.
    //
    if (cbReserveForSigning > 0)
    {
        DWORD cbSection;
        DWORD dwKeyOffset;
        void *pvBuffer;

        IfFailThrow(pFileGen->GetSectionBlock(hCeeSection, cbReserveForSigning, 1, &pvBuffer));
        IfFailThrow(pFileGen->GetSectionDataLen(hCeeSection, &cbSection));
        IfFailThrow(pFileGen->GetMethodRVA(hCeeFile, cbSection - cbReserveForSigning, &dwKeyOffset));
        IfFailThrow(pFileGen->SetStrongNameEntry(hCeeFile, cbReserveForSigning, dwKeyOffset));
    }
}


//============================================================================
// Tell ALink to actually emit the rest of the assembly's info.
//============================================================================
void MetaEmit::ALinkEmitAssembly()
{
    HRESULT hr = NOERROR;

    VSASSERT(FEmitAssembly(), "Must be generating an assembly!");
    DebCheckAssemblyOrModuleToken(m_mdAssemblyOrModule);

    VSASSERT(m_pALink, "Emitter created without ALink.");
    hr = m_pALink->EmitAssembly(m_mdAssemblyOrModule);
    if (FAILED(hr))
    {
        m_pBuilder->m_pErrorTable->CreateErrorWithError(ERRID_UnableToEmitAssembly, NULL, hr);
    }
}

//============================================================================
// Tell ALink to close the internal assembly.
//============================================================================
void MetaEmit::ALinkPrecloseAssembly()
{
    HRESULT hr = NOERROR;

    VSASSERT(FEmitAssembly(), "Must be generating an assembly!");
    DebCheckAssemblyOrModuleToken(m_mdAssemblyOrModule);

    VSASSERT(m_pALink, "Emitter created without ALink.");
    hr = m_pALink->PreCloseAssembly(m_mdAssemblyOrModule);
    if (FAILED(hr))
    {
        m_pBuilder->m_pErrorTable->CreateErrorWithError(ERRID_UnableToEmitAssembly, NULL, hr);
    }
}

//============================================================================
// Tell ALink to sign the assembly.
//============================================================================
void MetaEmit::ALinkSignAssembly()
{
    HRESULT hr = NOERROR;

    DebCheckAssemblyOrModuleToken(m_mdAssemblyOrModule);

    VSASSERT(m_pALink, "Emitter created without ALink.");
    hr = m_pALink->CloseAssembly(m_mdAssemblyOrModule);
    if (FAILED(hr))
    {
        m_pBuilder->m_pErrorTable->CreateErrorWithError(ERRID_UnableToSignAssembly, NULL, hr);
    }
}

void MetaEmit::ALinkEmbedUacManifest()
{
    HRESULT hr = NOERROR;
    VSASSERT(m_pALink, "Emitter created without ALink.");

    const WCHAR *uacFile = m_pCompilerProject->m_pstrUacManifestFile;
    if ( !uacFile )
    {
        // If there's no manifest then just return
        return;
    }

    if ( !m_pALink3 )
    {
        m_pBuilder->m_pErrorTable->CreateError(ERRID_UacALink3Missing, NULL);
        return;
    }

    // Make sure we can access the manifest file.
    //  Value 4 indicates read access
    //  Return of 0 here indicates success
    if ( _waccess(uacFile, 4) )
    {
        m_pBuilder->m_pErrorTable->CreateError(ERRID_UnableToReadUacManifest1, NULL, uacFile);
        return;
    }

    hr = m_pALink3->SetManifestFile(m_pCompilerProject->m_pstrUacManifestFile);
    if (FAILED(hr))
    {
        m_pBuilder->m_pErrorTable->CreateErrorWithError(ERRID_UnableToEmbedUacManifest, NULL, hr);
    }
}


//****************************************************************************
// Internal Helpers
//****************************************************************************

// GetALinkHelper
//
// Creates an ALink helper object in m_pALink that writes to AssemblyIsUBM.
//
void MetaEmit::GetALinkHelper()
{
    // Create new helper ALink if none exists
    if (!m_pALink)
    {
        STRING           *pstrProjPEFile;
        IMetaDataDispenserEx *pMetaDataDispenser = NULL;
        PlatformKinds PlatformKind = m_pCompilerProject->GetPlatformKind();

        IfFailThrow(m_pCompilerProject->GetCompilerHost()->GetDispenser(&pMetaDataDispenser));

        VSASSERT(!m_pCompilerProject->IsMetaData(), "Can't be building a metadata project!");
        pstrProjPEFile = m_pCompilerProject->GetPEName();

        // MetaEmit will need to release ALink when destroyed
        m_fReleaseALink = true;

        // Do create and initialize ALink
        PEBuilder::CreateALink(&m_pALink);

        IfFailThrow(m_pALink->Init(pMetaDataDispenser, NULL));

        // Pass afNoRefHash so we don't waste time generating hashes of
        // referenced assemblies.  See VS RAID 207952.
        IfFailThrow(m_pALink->SetNonAssemblyFlags(afNoRefHash));

        // We play a game here with ALink by passing AssemblyIsUBM.  We do this
        // because we don't want ALink to create a manifest (at least not yet).
        // Microsoft has blessed this hack.
        m_mdAssemblyOrModule = AssemblyIsUBM;

        IfFailThrow(m_pALink->AddFile(
                        m_mdAssemblyOrModule,           // IN - Unique ID for the assembly
                        pstrProjPEFile,                 // IN - filename of the file to add
                        ffContainsMetaData,             // IN - COM+ FileDef flags
                        m_pmdEmit,                      // IN - Emitter interface for file
                        &m_mdFileForModule));           // OUT- unique ID for the file

        RELEASE(pMetaDataDispenser);
    }
}

#if DEBUG
// IsExeceptionsListEmpty
//
// Returns whether or not the private members of MetaEmit are in initial state.  Use to verify that
// private information is being propagated correctly.
//
bool MetaEmit::IsPrivateMembersEmpty()
{
    return !(m_daExceptions.Count() || m_pbSignature);
}

// VerifyALinkAndTokens
//
// Returns whether the private ALink, assembly, and file token members are equal to
// those passed in.
//
bool MetaEmit::VerifyALinkAndTokens
(
    IALink2 *pALink,
    mdToken mdAssemblyOrModule,
    mdFile mdFileForModule
)
{
    return ((m_pALink == pALink) &&
            (m_mdAssemblyOrModule == mdAssemblyOrModule) &&
            (m_mdFileForModule == mdFileForModule));
}
#endif DEBUG

void MetaEmit::ResolveTypeForwarders
(
    _In_ void *pCompilerObj,
    _In_ void *pImportScope,
    mdTypeRef token, 
    _Out_ IMetaDataAssemblyImport **ppNewScope,
    _Out_ const void ** ppbHashValue, 
    _Out_ ULONG *pcbHashValue
)
{
    CompilerFile *pFile = (CompilerFile *) pImportScope;

    if (pFile && pFile->IsMetaDataFile())
    {
        // Searching through the cache of Symbols is enough as we would have had
        // to resolve this tyep before getting to the emit stage.
        bool fTypeForward = false;
        BCSYM *pType = GetTokenFromHash(pFile->PMetaDataFile(), token, &fTypeForward);

        if (fTypeForward &&
            pType && 
            pType->IsNamedRoot() && 
            pType->PNamedRoot()->GetCompilerFile()->IsMetaDataFile() && 
            pType->PNamedRoot()->GetCompilerFile() != pFile)
        {
            *ppNewScope = pType->PNamedRoot()->GetContainingProject()->GetAssemblyImport();
            *ppbHashValue = NULL;
            *pcbHashValue = 0;
        }
    }
}

// Search the hash of a metadata file for the cached symbol of a token.
BCSYM *MetaEmit::GetTokenFromHash
(
    _In_ MetaDataFile *pMetaDataFile,
    mdToken token,
    _Out_ bool* pfTypeForward
)
{
#ifdef _WIN64
    DynamicFixedSizeHashTable<size_t, TokenHashValue> *pTokenHash = pMetaDataFile->GetTokenHash();
#else
    DynamicFixedSizeHashTable<mdToken, TokenHashValue> *pTokenHash = pMetaDataFile->GetTokenHash();
#endif
    TokenHashValue *pthv;

#ifdef _WIN64
    {
    size_t tkTemp = token;
    pthv = pTokenHash->HashFind(tkTemp);
    }
#else
    pthv = pTokenHash->HashFind(token);
#endif

    if (pfTypeForward)
    {
        *pfTypeForward = pthv ? pthv->fTypeForward : false;
    }

    return pthv ? pthv->pSym : NULL;
}

