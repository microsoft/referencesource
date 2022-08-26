//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  The VB compiler main entrypoints.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

class CompilerSharedState;

extern CompilerSharedState *g_pCompilerSharedState;

static const RuntimeVersion
RuntimeVersionIntroduced[] =
{
    #define DEF_CLRSYM(symbol, clrscope, clrname, innetcf, runtimever)  runtimever,
    #include "Symbols\TypeTables.h"
    #undef DEF_CLRSYM
};

// Indicates whether we are compiling the vb runtime, Microsoft.VisualBasic.dll, as we have some chicken and egg issues to deal with.
bool g_CompilingTheVBRuntime = false;

#define DEFAULT_VBRUNTIME_DLL L"Microsoft.VisualBasic.dll"

// see CLI specification 24.2.6 "#~ stream"
// http://www.ecma-international.org/publications/files/ECMA-ST/Ecma-335.pdf
#define UNCOMPRESSED_METADATA_STREAM "#~" 

#define NETCF_PUBLIC_KEY_TOKEN L"969db8053d3322ac"

HINSTANCE Compiler::m_hinstResource = NULL;
LoadUICallback * Compiler::m_pfnLoadUIDll = NULL;

/*************************************************
Tables mapping Vtypes to CLR name
*************************************************/

const WCHAR *
g_wszFullCLRNameOfVtype[] =
{
    #define DEF_TYPE(type, clrscope, clrname, vbname, size, token, isnumeric, isintegral, isunsigned, isreference, allowoperation, allowconversion)  WIDE(clrscope) WIDE(".") WIDE(clrname),
    #include "Symbols\TypeTables.h"
    #undef DEF_TYPE
};

const WCHAR *s_rgSysCodeName[] =
{
    L"InternalMyTemplate.vb",   // SolutionExtension::MyTemplate
    L"InternalXmlHelper.vb",    // SolutionExtension::XmlHelper
    L"InternalVBCore.vb"        // SolutionExtension::VBCore
};

#define MYTEMPLATE_RESOURCEID          300     //defined in msvbalib\native.rc for RCDATA

CompilerHost::CompilerHost(
    Compiler * pCompiler,
    IVbCompilerHost * pIVbCompilerHost)
    : m_RuntimeVersion(RuntimeVersion2)
    , m_IsAutoLibraryType(false)
    , m_nrlsCompilationCache(NORLSLOC)
    , m_ProjectErrorCacheTree(&m_nrlsCompilationCache)
    , m_MergedNamespaceCache(&m_nrlsCompilationCache)
    , m_FXSymbolProvider(pCompiler)
    , m_EnableFeatures() // no value for now
#if IDE 
    , m_DebuggingProjectCount(0)
    , m_IsDebuggingInProgress(false)
    , m_StepsToCompilePreferredProject(0)
    , m_CurrentCompilationStepCount(0)
    , m_pProjectToWaitOn(NULL)
    , m_pPreferredSourceFile(NULL)
    , m_csProjectToWaitOn(CS_NoState)
#endif IDE
{
    m_pCompiler = pCompiler;
    m_pIVbCompilerHost = pIVbCompilerHost;
    ADDREF(pIVbCompilerHost);

    if (pIVbCompilerHost)
    {
        pIVbCompilerHost->GetTargetLibraryType(&m_PlatformType);
    }
    else
    {
        m_PlatformType = TLB_Desktop;
    }

    ZeroMemory(m_RuntimeMemberSymbolCache, sizeof(BCSYM_NamedRoot*) * MaxRuntimeMember );
    ZeroMemory(m_RuntimeClassSymbolCache, sizeof(BCSYM_Container*) * MaxRuntimeClass );
    ZeroMemory(m_RuntimeClassSymbolCacheStatus, sizeof( RuntimeSymbolCacheStatus ) * MaxRuntimeClass );
}

CompilerHost::~CompilerHost()
{
    RELEASE(m_pComPlusProject);
    RELEASE(m_pDefaultVBRuntimeProject);
    RELEASE(m_pIVbCompilerHost);

    FreeSolutionExtension();

    // Release COM+.
    RELEASE(m_pMetaDataDispenser);
}

//=============================================================================
// OpenScope wrapper
//=============================================================================

HRESULT CompilerHost::OpenScope(IMetaDataDispenserEx **ppimdDataDispEx, LPCWSTR szFileName, DWORD dwOpenFlags, REFIID riid, IUnknown **ppIUnk)
{
    HRESULT hr = NOERROR;

#if IDE 
    if (GetCompilerSharedState()->GetSmartOpenScope())
    {
        return GetCompilerSharedState()->GetSmartOpenScope()->OpenScope(szFileName, dwOpenFlags, riid, ppIUnk);
    }
    else
#endif IDE
    {
        //
        // Get COM+ IMetaDataDispenserEx.
        //
        IfFailGo(GetDispenser(ppimdDataDispEx));

        //
        // Get an IMetaDataAssemblyImport on the metadata scope
        //
        return (*ppimdDataDispEx)->OpenScope(szFileName, dwOpenFlags, riid, ppIUnk);
    }

Error:
    return E_FAIL;
}

void CompilerHost::ClearLookupCaches()
{
    CompilerProject *pCompilerProject = NULL;
    ProjectIterator Projects(this);

    while (pCompilerProject = Projects.Next())
    {
        pCompilerProject->ClearLookupCaches();
    }

    // Clear the caches
    m_ProjectErrorCacheTree.Clear();
    m_MergedNamespaceCache.Clear();

    // Release the memory
    m_nrlsCompilationCache.FreeHeap();
}

//=============================================================================
// Add a project to a CompilerHost.
//=============================================================================

void CompilerHost::InsertProject
(
    CompilerProject *pProject
)
{
    m_slProjects.InsertLast(pProject);
    pProject->m_isRegistered = true;
}

//=============================================================================
// Remove a project from a CompilerHost.
//=============================================================================

void CompilerHost::RemoveProject
(
    CompilerProject *pProject
)
{
    if (pProject->m_isRegistered)
    {
        // Release all the My* data before RemoveNamespaces because UnbindFile() will not be able to walk the types later.
        pProject->ReleaseMyCollectionInfo();

        // Remove the namespaces files in this project declared.
        pProject->RemoveNamespaces();

        pProject->ReleaseAllReferences(&pProject->m_daReferences);

        // Remove it from the list.
        m_slProjects.Remove(pProject);

#if IDE 
        // Make sure the Preferred project is not this one.
        if (m_pProjectToWaitOn == pProject)
        {
            m_pProjectToWaitOn = NULL;
            SetPreferredSourceFile(NULL);
        }

        if (GetCompilerPackage())
        {
            GetCompilerPackage()->ClearSchemasInProject(pProject);

            // The IDE has it's own compliation cache which includes a merged namespace cache. It's possible that the
            // cache may have references to BCSYM_Namespace's which were in this project. We need to blow away this
            // cache or else we might crash if we were to touch this cache and we hadn't decompiled due to an IDE edit.
            GetCompilerPackage()->ClearCompilationCaches();
        }
#endif IDE

        // Remember that we're no longer registered.
        pProject->m_isRegistered = false;
    }
}

//=============================================================================
// See if a project exists with a filename.
//=============================================================================

CompilerProject *CompilerHost::FindProject
(
    _In_opt_z_ STRING *pstrFileName,
    bool fMetaDataProject
#if IDE 
    , IVsHierarchy *pIVsHierarchy
#endif IDE
)
{
    CompilerProject *pProject = NULL;

    if (!pstrFileName)
    {
        return NULL;
    }

#if IDE 
    CComPtr<IVsHierarchy> spHierarchy(pIVsHierarchy);
#endif IDE

    CSingleListIter<CompilerProject> iter(&m_slProjects);

    while (pProject = iter.Next())
    {
        if (fMetaDataProject)
        {
            if (pProject->IsMetaData() && pProject->FMetadataProjectRepresentsFile(pstrFileName))
            {
                return pProject;
            }
        }
        // Otherwise just look up non-metadata projects by name.
        else if (pProject->GetFileName() && StringPool::IsEqual(pstrFileName, pProject->GetFileName())
#if IDE 
                 // For the IDE, it is possible to have two or more VB projects with the same name.
                 // This is why we need to compare the Hierarchy, not just the project name. The Hierarchy is
                 // always NULL for Metadata projects.
                 && spHierarchy.IsEqualObject(pProject->GetHierarchy())
#endif IDE
                )
        {
            return pProject;
        }
    }

    return NULL;
}

#if !IDE
//=============================================================================
// Finds a project which has the same assembly name, PK token and version as
// the project passed in.
//=============================================================================
CompilerProject *CompilerHost::FindProjectWithSameNamePKTokenAndVersion
(
    CompilerProject *pProject
)
{
    ProjectIterator AllProjectsIter(this);

    while (CompilerProject *pCandidateProject = AllProjectsIter.Next())
    {
        // Make sure we are not looking at the same assembly first.
        if (pProject != pCandidateProject)
        {
            // Compare Assembly names and PK tokens.
            if (pProject->GetAssemblyIdentity()->IsAssemblyWithSameNameAndPKToken(pCandidateProject->GetAssemblyIdentity()) &&
                pProject->GetAssemblyIdentity()->CompareAssemblyVersion(pCandidateProject->GetAssemblyIdentity()) == 0)
            {
                return pCandidateProject;
            }
        }
    }

    return NULL;
}
#endif !IDE

//=============================================================================
// Returns metadata project (if any) that's backed by the given file
//=============================================================================
CompilerProject *CompilerHost::FindMetaDataProjectForFile
(
    LPCWSTR wszFileName
)
{
    CompilerProject *pProject = NULL;

    // Loop through the list.
    CSingleListIter<CompilerProject> iter(&m_slProjects);

    while (pProject = iter.Next())
    {
        if (pProject->IsMetaData() && CompareFilenames(pProject->GetFileName(), wszFileName) == 0)
        {
            return pProject;
        }
    }

    return NULL;
}

// ----------------------------------------------------------------------------
// Determine if the dogfooding features are turned on.
// ----------------------------------------------------------------------------

bool CompilerHost::IsDogfoodFeaturesEnabled()
{
    if( m_EnableFeatures.HasValue() )
    {
        return( m_EnableFeatures.GetValue() );
    }
    else
    {
        // Try to load the values from the environment.

        const LPCWSTR wszEnvironmentVar = L"VBC_COMPILER_DOGFOOD_BUILD";
        UINT cch = GetEnvironmentVariableW( wszEnvironmentVar, NULL, 0 );

        if( ::GetLastError() == ERROR_ENVVAR_NOT_FOUND )
        {
            m_EnableFeatures.SetValue( false );
        }
        else
        {
            m_EnableFeatures.SetValue( true );
        }

        return( m_EnableFeatures.GetValue() );
    }
}

//============================================================================
// Generic helper to get the metadata dispenser.
//============================================================================

HRESULT CompilerHost::GetDispenser
(
    IMetaDataDispenserEx **ppMetaDataDispenser
)
{
#ifdef FEATURE_CORESYSTEM
    return E_FAIL;
#else
    HRESULT hr = NOERROR;

    if (!m_pMetaDataDispenser)
    {
#if 1   // Enable this to fix 195596.  Wait until .config files are in place and debugged.
        IfFailGo(LegacyActivationShim::CorBindToCurrentRuntime(
                        NULL,
                        CLSID_CorMetaDataDispenser,
                        IID_IMetaDataDispenserEx,
                        (void **)&m_pMetaDataDispenser));
        ASSERT(m_pMetaDataDispenser != NULL, 
            "Should not be NULL, you may have a bad .config file or bad env variable which sets a wrong version of .net.");
#else   // !195596
        IfFailGo(CoCreateInstance(CLSID_CorMetaDataDispenser,
                                  NULL,
                                  CLSCTX_ALL,
                                  IID_IMetaDataDispenserEx,
                                  (void **)&m_pMetaDataDispenser));
#endif  // !195596
    }

    *ppMetaDataDispenser = m_pMetaDataDispenser;
    m_pMetaDataDispenser->AddRef();

Error:
    return hr;
#endif
}

//============================================================================
// Returns true iff the given file is an assembly.  Note that
// 'false' could mean either that the file is a code module
// or that the file name is just bogus!
//============================================================================
bool CompilerHost::IsFileAnAssembly(_In_z_ STRING *pstrFileName)
{
    HRESULT hr = NOERROR;
    bool fIsAssembly = false;
    IMetaDataAssemblyImport *pimdAssemblyImport = NULL;
    IMetaDataDispenserEx *pimdDataDispEx = NULL;
    mdAssembly mdassembly;

    hr = OpenScope(&pimdDataDispEx, pstrFileName, ofReadOnly, IID_IMetaDataAssemblyImport, (IUnknown **)&pimdAssemblyImport);

    if (FAILED(hr))
    {
        // File is bogus or contains no metadata
    }
    else if (FAILED(pimdAssemblyImport->GetAssemblyFromScope(&mdassembly)))
    {
        // File is a code module
    }
    else
    {
        // File is an assembly
        fIsAssembly = true;
    }

    RELEASE(pimdAssemblyImport);
    RELEASE(pimdDataDispEx);

    return fIsAssembly;
}

#if IDE 
/*****************************************************************************
;ClearRuntimeHelperSymbolCaches

We cache the symbols for the various runtime helpers that we call during compilation.  Those symbols
are cached in the compiler host.  The symbols can come from Microsoft.VisualBasic.dll and MSCorLib.dll
So when the project decompiles, if any metadata references are decompiled, such as happens when a
reference is added, we need to purge any cached symbols for the particular project that is decompiling.
*****************************************************************************/
void CompilerHost::ClearRuntimeHelperSymbolCaches
(
    CompilerProject *ProjectGoingDown // [in] The project that is being decompiled or destroyed
)
{
    // Figure out if the project that is being decompiled contributed any members to our caches
    for ( int projectIndex = 0; projectIndex < MaxRuntimeLibrary; projectIndex++ )
    {
        if ( ProjectGoingDown == m_ListOfProjectsContributingRuntimeHelpers[ projectIndex ] )
        {
            m_ListOfProjectsContributingRuntimeHelpers[ projectIndex ] = NULL; // we are clearing the cached runtime helper symbols for this project so remove this project from the list of projects that contributed runtime members
            // When a project is decompiled, we want to remove all its associated runtime helper symbols from the cache
            for ( int RuntimeSymbol=0; RuntimeSymbol < MaxRuntimeMember; RuntimeSymbol++ )
            {
                if ( m_RuntimeMemberSymbolCache[ RuntimeSymbol ] != NULL &&
                     m_RuntimeMemberSymbolCache[ RuntimeSymbol ]->GetContainingProject() == ProjectGoingDown )
                {
                    m_RuntimeMemberSymbolCache[ RuntimeSymbol ] = NULL;
                }
            }

             // When a project is decompiled, we want to remove all its associated class symbols from the cache
            for ( int RuntimeClass=0; RuntimeClass < MaxRuntimeClass; RuntimeClass++ )
            {
                if ( m_RuntimeClassSymbolCache[ RuntimeClass ] != NULL &&
                     m_RuntimeClassSymbolCache[ RuntimeClass ]->GetContainingProject() == ProjectGoingDown )
                {
                    m_RuntimeClassSymbolCache[ RuntimeClass ] = NULL;
                    m_RuntimeClassSymbolCacheStatus[ RuntimeClass ] = NotCachedYet;
                }
            }
            return; // our work is done here...
        } // if
    } // for
}

//-------------------------------------------------------------------------------------------------
//
// Try and bind the core libraries for the CompilerHost.  This will fail in any circumstance where 
// the background thread is currently blocked by a StopBackgroundThreadCall.  As well as other 
// binding issues
//
//-------------------------------------------------------------------------------------------------
bool
CompilerHost::TryBindCoreLibraries()
{
    HRESULT hr = S_OK; 
    
    IfFailGo( GetComPlusProject()->WaitUntilDeclaredState(
                WaitStatePumping::PumpingDisallowed, 
                WaitStateCancel::CancelDisallowed) );

Error:
    return SUCCEEDED(hr);
}


#endif // IDE

/*****************************************************************************
//
// Clear out the caches for all of the runtime members.
//
*****************************************************************************/
void 
CompilerHost::ClearRuntimeHelperCaches()
{
    ZeroMemory(m_RuntimeCallRemapTable, sizeof(BCSYM_Proc*) * MappedMemberMax);
    ZeroMemory(m_RuntimeMemberSymbolCache, sizeof(BCSYM_NamedRoot*) * MaxRuntimeMember);
    ZeroMemory(m_RuntimeClassSymbolCache, sizeof(BCSYM_Container*) * MaxRuntimeClass);
    ZeroMemory(m_RuntimeClassSymbolCacheStatus, sizeof(RuntimeSymbolCacheStatus) * MaxRuntimeClass);
}

/*****************************************************************************
;GetSymbolForRuntimeClass

Helper function to GetSymbolForRuntimeHelper.  Find the symbol table entry for the class on which the
runtime helper will be found.

    -    bIsDefaultVBRuntimeOrMSCorLib is an out parameter. It returns whether the class is defined in default
          vb runtime or MSCorLib. Since caller may need this information to decide whether to caching the symbol
          or not.

*****************************************************************************/
BCSYM_Container *CompilerHost::GetSymbolForRuntimeClass
(
    RuntimeClasses Class, // [in] the class we want to look up in the symbol table
    CompilationCaches *pCompilationCaches,
    bool *bIsDefaultVBRuntimeOrMSCorLib,
    ErrorTable *pErrorTable,
    Location *pErrorLocation
)
{

    // After VBCore introduced, we only cache for defualt vb runtime.
    // Also bIsDefaultVBRuntimeOrMSCorLib passes the information to the caller, that whether the class is defined
    // in default vb runtime.

    bool isDefualtVBRuntimeOrMSCorLib = true;
        
    RuntimeClassDescriptor *ClassDescriptor = &(g_rgRTLangClasses[Class]);
    CompilerProject * pProjectBeingCompiled = m_pCompiler->GetProjectBeingCompiled();

    if (ClassDescriptor->rtLibrary == VBLibrary && 
        (pProjectBeingCompiled != NULL && pProjectBeingCompiled->GetVBRuntimeKind() != DefaultRuntime))
    {
        isDefualtVBRuntimeOrMSCorLib = false;
    }
    
    // Save the bother of re-looking up the symbol, if possible, as it is an expensive process
    if (isDefualtVBRuntimeOrMSCorLib && m_RuntimeClassSymbolCache[ Class ] != NULL )
    {
        if (bIsDefaultVBRuntimeOrMSCorLib != NULL)
        {
            *bIsDefaultVBRuntimeOrMSCorLib = true;
        }
        return m_RuntimeClassSymbolCache[ Class ];
    }

    // Bail if we are using a runtime that doesn't define this class
    // This happens if they are compiling with /vbruntime- or are using /vbruntime:foo.dll
    // where foo.dll doesn't define all of the vb runtime helpers.
    if (isDefualtVBRuntimeOrMSCorLib && m_RuntimeClassSymbolCacheStatus[ Class ] == SymbolNotDefined )
    {
        if (bIsDefaultVBRuntimeOrMSCorLib != NULL)
        {
            *bIsDefaultVBRuntimeOrMSCorLib = true;
        }
        return NULL;
    }

    // We have to manually bind this name.  Split it up and use the semantic analyzer to find the type.

    CompilerProject *Project = NULL;
    
    if ( ClassDescriptor->rtLibrary == VBLibrary )
    {

        if (pProjectBeingCompiled == NULL)
        { 
            // In Debugger and VBHoseted, pProjectBeingCompiled can be NULL and 
            // the default VB Runtime will be set to m_pDefaultVBRuntimeProject.

            // If current function is called during preparing remapping table, 
            // then default vb runtime should be used.
            if (m_pDefaultVBRuntimeProject != NULL)
            {
                Project = m_pDefaultVBRuntimeProject;
            }
            else
            {
            
                if (bIsDefaultVBRuntimeOrMSCorLib != NULL)
                {
                    *bIsDefaultVBRuntimeOrMSCorLib = isDefualtVBRuntimeOrMSCorLib;
                }
                return NULL;
            }    
        }
		else if (pProjectBeingCompiled->GetVBRuntimeKind() == EmbeddedRuntime ||
			    (pProjectBeingCompiled->GetVBRuntimeKind() == NoRuntime && m_pCompiler->IsCompilingTheVBRuntime()))
        {
            // Only use pProjectBeingCompiled when it is the VB runtime.
            Project = pProjectBeingCompiled;
        }
        else if (pProjectBeingCompiled->GetVBRuntimeKind() == DefaultRuntime ||  
                 pProjectBeingCompiled->GetVBRuntimeKind() == SpecifiedRuntime)
        {
            Project = pProjectBeingCompiled->GetVBRuntimeProject();
        }

    }
    else if ( ClassDescriptor->rtLibrary == COMLibrary )
    {
        Project = this->GetComPlusProject();
    }
    else 
    {
        STRING *ProjectName = m_pCompiler->GetRuntimeLibraryName(ClassDescriptor->rtLibrary);
        CSingleListIter<CompilerProject> ProjectIterator(&m_slProjects);

        while (Project = ProjectIterator.Next())
        {
            if (StringPool::IsEqual(ProjectName, Project->GetAssemblyName()))
            {
                break;
            }
        }
    }

    STRING *ClassName = m_pCompiler->GetRuntimeClassName(Class);
    BCSYM *BoundSymbol = GetSymbolForQualifiedName(ClassName, Project, m_pCompiler, this, pCompilationCaches);

    if (isDefualtVBRuntimeOrMSCorLib)
    {
        if ( BoundSymbol != NULL )
        {
            m_RuntimeClassSymbolCache[ Class ] = BoundSymbol->PContainer();
            m_RuntimeClassSymbolCacheStatus[ Class ] = Cached;
#if IDE 
            m_ListOfProjectsContributingRuntimeHelpers[ ClassDescriptor->rtLibrary ] = Project;
#endif
        }
        else
        {
            // If the symbol hasn't been found in the specified projects and if we are currently compiling a project
            // check to see if we can bind to this name from the current project's context. IF we can then we should 
            // use that symbol. This lets the core types be present in assemblies other than mscorlib and vbruntime.
            // The Immersive profile has moved types around and we need to be resilient to that.
            if (pProjectBeingCompiled != NULL)
            {
                BoundSymbol = GetSymbolForQualifiedName(ClassName, 
                                                        pProjectBeingCompiled, 
                                                        m_pCompiler, 
                                                        this, 
                                                        pCompilationCaches, 
                                                        pErrorTable,
                                                        pErrorLocation);
                
                // Type not found
                if (BoundSymbol == NULL && pErrorTable)
                {
                    pErrorTable->CreateError( ERRID_UndefinedType1, pErrorLocation, ClassName);
                }
                // Type ambiguous errors would have been reported by NameSemantics in the above call.
                else if (pErrorTable && pErrorTable->HasErrors())
                {
                    BoundSymbol = NULL;
                }

                if (BoundSymbol)
                {
                    // The symbol is found in a different assembly. Flag it so that this symbol doesn't get cached.
                    isDefualtVBRuntimeOrMSCorLib = false;
                }
            }
        }
    }

    if (bIsDefaultVBRuntimeOrMSCorLib != NULL)
    {
        *bIsDefaultVBRuntimeOrMSCorLib = isDefualtVBRuntimeOrMSCorLib;
    }

    return BoundSymbol == NULL ? NULL : BoundSymbol->PContainer();
}
/*****************************************************************************
;GetSymbolForQualifiedName
*****************************************************************************/
BCSYM *CompilerHost::GetSymbolForQualifiedName
(
    _In_z_ STRING *pClassName,
    CompilerProject *pProject,
    Compiler *pCompiler,
    CompilerHost *pCompilerHost,
    CompilationCaches *pCompilationCaches,
    ErrorTable *pErrorTable,
    Location *pLocation
)
{

    AssertIfTrue( pCompiler == NULL || pCompilerHost == NULL);

    if ( pProject == NULL )
    {
        return NULL;
    }

#define RUNTIMENAMESIZE     5
    STRING *NameArray[RUNTIMENAMESIZE];  // maximum components in a runtime name.
    unsigned NameCount = pCompiler->CountQualifiedNames(pClassName);
    BCSYM *BoundSymbol = NULL;
    bool IsBadName = false;
    NorlsAllocator Scratch(NORLSLOC);
    STRING **Names = (NameCount <= _countof(NameArray)) ? NameArray : 
        (STRING **)Scratch.Alloc(VBMath::Multiply(sizeof(STRING *), NameCount));
    pCompiler->SplitQualifiedName(pClassName, NameCount, Names);

    BoundSymbol = Semantics::InterpretQualifiedName(Names,
                                                    NameCount,
                                                    NULL,
                                                    NULL,
                                                    pCompiler->GetUnnamedNamespace(pProject)->GetHash(),
                                                    NameSearchTypeReferenceOnly | 
                                                        NameSearchIgnoreImports |    // The goal is to do search by fully qualified name, project level imports may affect the result for source projects.
                                                        NameSearchIgnoreModule,  // The goal is to do search by fully qualified name
                                                    *pLocation,
                                                    pErrorTable,
                                                    pCompiler,
                                                    pCompilerHost,
                                                    pCompilationCaches, // Compilation Caches
                                                    NULL,
                                                    true, // perform obsolete checks
                                                    IsBadName); 


    return BoundSymbol;
}

/*****************************************************************************
;GetSymbolForRuntimeClassCtor

Search the given class for the constructor which matches the given 
RuntimeMemberDecriptor.
*****************************************************************************/
BCSYM_NamedRoot *CompilerHost::GetSymbolForRuntimeClassCtor
(
    BCSYM_Class *pClass,
    unsigned short usFlags,
    Vtypes vtypRet,
    Vtypes *vtypArgs,
    Compiler *pCompiler
)
{
    BCSYM_NamedRoot *BoundSymbol = NULL;

    AssertIfTrue(pClass == NULL || !pClass->IsClass() || pCompiler == NULL);
    
    for ( BoundSymbol = pClass->GetFirstInstanceConstructor(pCompiler);
          BoundSymbol != NULL;
          BoundSymbol = BoundSymbol->PProc()->GetNextInstanceConstructor())
    {
        if (RuntimeMemberDescriptor::DoesMemberMatchDescriptor(BoundSymbol, 
                                                               usFlags,
                                                               vtypRet,
                                                               vtypArgs,
                                                               pCompiler))
        {
           break;
        }
    }
    return BoundSymbol;
}
/*****************************************************************************
;GetSymbolForRuntimeMember

In order to generate a call to a runtime helper, we need to resolve the symbol
table entry for the helper.
*****************************************************************************/
BCSYM_NamedRoot *CompilerHost::GetSymbolForRuntimeMember
(
    RuntimeMembers Member, // [in] which runtime helper we need a symbol table entry for
    CompilationCaches *pCompilationCaches,
    ErrorTable *pErrorTable,
    Location *pErrorLocation
)
{

    RuntimeMemberDescriptor *MemberDescriptor = &( g_rgRTLangMembers[ Member ]);

    // Before VBCore, there is only ONE vb runtime for every project, 
    // it can be either default vb runtime or user specified vb runtime.

    // After VBCore introduced, there can be more than one vb runtime, 
    // We only cache for defualt vb runtime. Otherwise there will be inconsistency. 
    
    bool isDefaultVBRuntimeOrMSCorLib = false;
    
    BCSYM_Container *Class = GetSymbolForRuntimeClass( 
        MemberDescriptor->rtParent, 
        pCompilationCaches, 
        &isDefaultVBRuntimeOrMSCorLib,
        pErrorTable,
        pErrorLocation);

    // First check to see if we have done this work before
    if ( isDefaultVBRuntimeOrMSCorLib && m_RuntimeMemberSymbolCache[ Member ] != NULL )
    {
        return m_RuntimeMemberSymbolCache[ Member ];
    }

    if ( Class == NULL )
    {
        return NULL; // If we have no runtime, or the runtime for the targeted platform doesn't define this type, then bail.
    }

    BCSYM_NamedRoot *BoundSymbol = NULL;

    // Search the containing class for the runtime method.
    if (MemberDescriptor->usFlags & RTF_CONSTRUCTOR)
    {
        BoundSymbol = GetSymbolForRuntimeClassCtor(Class->PClass(), 
                                                   MemberDescriptor->usFlags,
                                                   MemberDescriptor->vtypRet,
                                                   MemberDescriptor->vtypArgs,
                                                   m_pCompiler);
    }
    else
    {
        // 
        STRING *pstrName = m_pCompiler->AddString(MemberDescriptor->wszName);

        // Find the member.
        BoundSymbol = Class->SimpleBind(NULL, pstrName);

        // Make sure the signature matches in case overloads are available.
        while (BoundSymbol)
        {
            if (RuntimeMemberDescriptor::DoesMemberMatchDescriptor(BoundSymbol,
                                                                   MemberDescriptor->usFlags,
                                                                   MemberDescriptor->vtypRet,
                                                                   MemberDescriptor->vtypArgs,
                                                                   m_pCompiler))
            {
                break;
            }
            BoundSymbol = BoundSymbol->GetNextBound();
        }

#if DEBUG
        // This code will ensure that one and only one signature matches
        if (BoundSymbol)
        {
            int cMatch = 0;
            BCSYM_NamedRoot *BoundSymbolTemp = BoundSymbol;

            while (BoundSymbolTemp)
            {
                if (RuntimeMemberDescriptor::DoesMemberMatchDescriptor(BoundSymbolTemp, 
                                                                       MemberDescriptor->usFlags,
                                                                       MemberDescriptor->vtypRet,
                                                                       MemberDescriptor->vtypArgs,
                                                                       m_pCompiler))
                {
                    cMatch++;
                }

                BoundSymbolTemp = BoundSymbolTemp->GetNextBound();
            }

            VSASSERT(cMatch == 1, "Configuration Error:  Expected one signature match.  Make sure the compiler has a matching runtime!");
        }
#endif

    }

    if (isDefaultVBRuntimeOrMSCorLib)
    {
        m_RuntimeMemberSymbolCache[ Member ] = BoundSymbol;
    }    

    // It is possible for us to return NULL.  In previous versions of the product we'd error out.  In the new world, however, this can happen
    // because we are targeting a leaner runtime.  So we return null when we can't find the helper and the caller is expected to deal with
    // that situation accordingly
    return BoundSymbol;
}

//-------------------------------------------------------------------------------------------------
//
// Determine if this symbol is a cached runtime member.  This is very useful for the debugger 
// as it needs to special case functions which are actually runtime helpers
//
//-------------------------------------------------------------------------------------------------
bool 
CompilerHost::IsCachedRuntimeMember( _In_ Declaration* pDecl )
{
    ThrowIfNull(pDecl);

    for ( int i = 0; i < MaxRuntimeMember; i++ )
    {
        if ( m_RuntimeMemberSymbolCache[i] == pDecl )
        {
            return true;
        }
    }

    return false;
}

// ========================================================================================
// Code below is for remapping of versioned functions.
//
// Code compiled with VS7.1 or earlier must run on VS8.0 with no behavior changes.
// However, since VS8.0 introduces several new features which change the behavior of
// Public functions, we introduce the new behavior in a new function and map references
// to the old function onto the new function. In this way, old code continues to run and
// new code gets the new behavior, but the Public function doesn't change at all, from the
// user's point of view.
// Example:
//
//     Given:
//     1. User code snippet: "If IsNumeric(something) Then ..."
//     2. Public Function IsNumeric(o), has VS7.1 behavior.
//     3. Public Function Versioned.IsNumeric(o), has VS8.0 behavior.
//
//     When compiled in VS7.1, the compiler binds to IsNumeric(o).
//     When compiled in VS8.0, the compiler first binds to IsNumeric(o) and then remaps
//     this binding to Versioned.IsNumeric(o) in the code generator so that the VS8.0
//     behavior is selected.
//
// The implementation is simple.  A table holds the proc symbols for methods that need to
// be remapped.  For each call to a proc M, if the symbol M matches a symbol stored in the
// table, then change M to the appropriate mapped symbol N.
//
// ========================================================================================

void CompilerHost::PrepareRemappingTable
(
    CompilationCaches *pCompilationCaches
)
{
    if (!GetRuntimeCall(MappedMemberIsNumericV1))
    {
        SetRuntimeCall(MappedMemberIsNumericV1, GetSymbolForRuntimeMember(IsNumericV1Member, pCompilationCaches));
    }
    if (!GetRuntimeCall(MappedMemberIsNumeric))
    {
        SetRuntimeCall(MappedMemberIsNumeric, GetSymbolForRuntimeMember(IsNumericMember, pCompilationCaches));
    }
    if (!GetRuntimeCall(MappedMemberCallByNameV1))
    {
        SetRuntimeCall(MappedMemberCallByNameV1, GetSymbolForRuntimeMember(CallByNameV1Member, pCompilationCaches));
    }
    if (!GetRuntimeCall(MappedMemberCallByName))
    {
        SetRuntimeCall(MappedMemberCallByName, GetSymbolForRuntimeMember(CallByNameMember, pCompilationCaches));
    }
    if (!GetRuntimeCall(MappedMemberTypeNameV1))
    {
        SetRuntimeCall(MappedMemberTypeNameV1, GetSymbolForRuntimeMember(TypeNameV1Member, pCompilationCaches));
    }
    if (!GetRuntimeCall(MappedMemberTypeName))
    {
        SetRuntimeCall(MappedMemberTypeName, GetSymbolForRuntimeMember(TypeNameMember, pCompilationCaches));
    }
    if (!GetRuntimeCall(MappedMemberSystemTypeNameV1))
    {
        SetRuntimeCall(MappedMemberSystemTypeNameV1, GetSymbolForRuntimeMember(SystemTypeNameV1Member, pCompilationCaches));
    }
    if (!GetRuntimeCall(MappedMemberSystemTypeName))
    {
        SetRuntimeCall(MappedMemberSystemTypeName, GetSymbolForRuntimeMember(SystemTypeNameMember, pCompilationCaches));
    }
    if (!GetRuntimeCall(MappedMemberVbTypeNameV1))
    {
        SetRuntimeCall(MappedMemberVbTypeNameV1, GetSymbolForRuntimeMember(VbTypeNameV1Member, pCompilationCaches));
    }
    if (!GetRuntimeCall(MappedMemberVbTypeName))
    {
        SetRuntimeCall(MappedMemberVbTypeName, GetSymbolForRuntimeMember(VbTypeNameMember, pCompilationCaches));
    }
}

BCSYM_Proc *CompilerHost::RemapRuntimeCall
(
    BCSYM_Proc *RuntimeCall,
    CompilationCaches *pCompilationCaches
)
{

    //After /VBRuntime added, we limit the RemapRuntimeCall only remaps functions from default vb runtime. 
    
    if (RuntimeCall == NULL || 
        m_pDefaultVBRuntimeProject == NULL || 
        (GetCompiler()->GetProjectBeingCompiled() && GetCompiler()->GetProjectBeingCompiled()->GetVBRuntimeKind() != DefaultRuntime))
    {
        return RuntimeCall;
    }    

    if (!GetRuntimeCall(MappedMemberIsNumericV1))
    {
        PrepareRemappingTable(pCompilationCaches);
    }

    if (RuntimeCall == GetRuntimeCall(MappedMemberIsNumericV1))
    {
        return GetRuntimeCall(MappedMemberIsNumeric);
    }

    if (RuntimeCall == GetRuntimeCall(MappedMemberCallByNameV1))
    {
        return GetRuntimeCall(MappedMemberCallByName);
    }

    if (RuntimeCall == GetRuntimeCall(MappedMemberTypeNameV1))
    {
        return GetRuntimeCall(MappedMemberTypeName);
    }

    if (RuntimeCall == GetRuntimeCall(MappedMemberSystemTypeNameV1))
    {
        return GetRuntimeCall(MappedMemberSystemTypeName);
    }

    if (RuntimeCall == GetRuntimeCall(MappedMemberVbTypeNameV1))
    {
        return GetRuntimeCall(MappedMemberVbTypeName);
    }

    // No mapping found, so just return.
    return RuntimeCall;
}


// Check whether this Class matches the namespace Microsoft.VisualBasic
// and the class name Name.
bool CompilerHost::IsRuntimeType
(
    _In_z_ STRING* Name,
    BCSYM_NamedRoot* Class
)
{
    if (StringPool::IsEqual(Class->GetNameSpace(), STRING_CONST(m_pCompiler, MicrosoftVisualBasic)))
    {
        if (StringPool::IsEqual(Class->GetName(), Name))
        {
            return true;
        }
    }

    return false;
}

void CompilerHost::OutputString
(
    _In_z_ const WCHAR *wszOutput
)
{
#if IDE 
    if (GetCompilerPackage())
    {
        GetCompiler()->OutputString(wszOutput);
    }
    else
    {
#endif IDE
        if (m_pIVbCompilerHost)
        {
            m_pIVbCompilerHost->OutputString(wszOutput);
        }
        else
        {
            CHAR szBuffer[8192];

            // Convert the text of the error to multibyte for display...
            if (wszOutput != NULL)
            {
                if (!WideCharToMultiByte(GetConsoleOutputCP(), 0, wszOutput, -1, szBuffer, sizeof (szBuffer), NULL, NULL))
                {
                    szBuffer[sizeof(szBuffer) - 1] = 0;
                }

                printf("%s", szBuffer);
            }
        }
#if IDE 
    }
#endif IDE
}

//============================================================================
// Helper to print a formatted string.
//============================================================================
#pragma warning (push)
#pragma warning (disable:6262) //,"Yes, we're a stack hog.")

void CompilerHost::Printf
(
    _In_z_ const WCHAR *wszFmt,
    ...
)
{
    va_list vaArgPtr;
    va_start(vaArgPtr, wszFmt);

    // We produce some mighty big error messages.
    WCHAR wszBuffer[8192];

    // NULL Terminate the string if it overflows.
    if (_vsnwprintf_s(wszBuffer,
                    (sizeof(wszBuffer) / sizeof(WCHAR) - 1),
                    _TRUNCATE, wszFmt,
                    vaArgPtr) < 0)
    {
        // NULL Terminate the string and append "..." if it overflows.
        int ichLim = sizeof(wszBuffer) / sizeof(WCHAR); 
        wszBuffer[ichLim - 4] = L'.';
        wszBuffer[ichLim - 3] = L'.';
        wszBuffer[ichLim - 2] = L'.';
        wszBuffer[ichLim - 1] = L'\0';
    }

    OutputString(wszBuffer);

    va_end(vaArgPtr);
}
#pragma warning (pop)

#define SYSTEMCODEBASNAME L"17d14f5c-a337-4978-8281-53493378c107"

void CompilerHost::SetSolutionExtension(void *pCode, DWORD dwLen, SolutionExtensionEnum extension)
{
    VB_ENTRY();
    if (dwLen && pCode)
    {
        int iUniLen = MultiByteToWideChar(CP_ACP,0,(LPCSTR)pCode,(int)dwLen,NULL,0);

        if (iUniLen)
        {
            StringBuffer *pstrbuf = new StringBuffer();
            pstrbuf->AllocateSizeAndSetLength(VBMath::Multiply(iUniLen, sizeof(WCHAR)));

            int iLen = MultiByteToWideChar(CP_ACP,0,(LPCSTR)pCode,(int)dwLen,pstrbuf->GetString(),iUniLen);
            VSASSERT(iLen == iUniLen,"Error in converting ansi char to unicode char");

            SolutionExtensionData data;

            data.m_extension = extension;
            data.m_psbCode = pstrbuf;
            if (extension >= 0 && extension < _countof(s_rgSysCodeName))
            {
                data.m_pstrName = m_pCompiler->AddString(s_rgSysCodeName[extension]);
            }
            else
            {
                data.m_pstrName = m_pCompiler->AddString(L"InternalExtensionFile.vb");
            }

            WCHAR wszNum[MaxStringLengthForIntToStringConversion] = {0};
            _ultow_s((extension+1), wszNum, _countof(wszNum), 10);
            data.m_pstrFileName = m_pCompiler->ConcatStrings(SYSTEMCODEBASNAME, wszNum, L".vb");

            m_rgSolutionExtension[extension] = data;
        }
    }
    VB_EXIT_NORETURN();
}

void CompilerHost::FreeSolutionExtension()
{
    for (ULONG i = 0; i <= SolutionExtension::MaxExtension; ++i)
    {
        delete m_rgSolutionExtension[i].m_psbCode;
    }
}

HRESULT CompilerHost::InitStandardLibraryList()
{
    // Add default (hard-coded) list
    // Add System.dll as it is needed for My.
#if !HOSTED
    STRING* pstrSystemDLL = m_pCompiler->AddString(SYSTEM_DLL);
    m_daStandardLibraries.AddElement(pstrSystemDLL);
#endif
    return S_OK;
}

HRESULT CompilerHost::GetSdkPath(_Deref_out_opt_z_ STRING** pPath)
{
    WCHAR*          path = NULL;
    NorlsAllocator  nra(NORLSLOC);
    HRESULT         hr = NOERROR;
    CComBSTR        sdkPath;

    //
    // We'll search either the /sdkpath directory that
    // the user passed in or the COR system directory.
    // Mscorlib.dll and Microsoft.VisualBasic.dll
    // must be on the path or we're just horked.
    //

    *pPath = NULL;

    //-- Get the /sdkpath from the compiler host
    if (m_pIVbCompilerHost != NULL)
    {
        hr = m_pIVbCompilerHost->GetSdkPath(&sdkPath);

        if (SUCCEEDED(hr) && sdkPath.Length() > 0) // GetSdkPath may return an empty path, which we don't want to use
        {
            path = sdkPath;
        }
        else if (FAILED(hr)) 
        {
            // The host doesn't support /sdkpath.
            // We'll go ahead and use LegacyActivationShim::GetCORSystemDirectory below.
            return hr;
        }
    }

    //-- COR System Directory.
    if (path == NULL)
    {
#ifndef FEATURE_CORESYSTEM
        // Get size of path, allocate buffer, get path.
        DWORD charCount = 0;
        LegacyActivationShim::GetCORSystemDirectory(NULL, NULL, &charCount);
        if (charCount > 0)
        {
            path = (WCHAR *)nra.Alloc(charCount * sizeof(WCHAR));
            hr = LegacyActivationShim::GetCORSystemDirectory(path, charCount, &charCount);
        }
        else
        {
            hr = E_FAIL; // empty string is not  a valid path
        }
#else
        path = CORESYS_SYSTEM_DIR; 
#endif
    }

    if (SUCCEEDED(hr))
    {
        VSASSERT(path != NULL, "path should be set if HR is noerror");
        *pPath = GetCompiler()->AddString(path);
    }

    return hr;
}

/*****************************************************************************
;SetRuntimeVersion

Determines what the metadata schema version of mscorlib is and sets our version accordingly.
We had a big problem when Telesto came out because they set their assembly version to 1.0  That blew
the compiler up because we keyed off the assembly version and then died when version 2.0 metadata
information (like generics) came through since we were taking Ver 1 code paths through the compiler.
So we now look at the version of the metadata schema, instead of relying on the assembly version.
*****************************************************************************/
HRESULT CompilerHost::SetRuntimeVersion(
    CompilerProject *pMSCorLibHost // [in] the compiler project for MSCORLIB.Dll
)
{
    HRESULT hr = NO_ERROR;
    CComPtr<IMetaDataTables2> srpTables= NULL;  // IMetaDataTables2 was introduced in Whidbey
    MetaDataFile *pCorLibMetaDataFile = pMSCorLibHost->GetPrimaryModule();
    IMetaDataImport *pImport = pCorLibMetaDataFile->GetImport();
    hr = pImport->QueryInterface(IID_IMetaDataTables2, (void **) &srpTables);

    m_RuntimeVersion = RuntimeVersion2; // assume we are post-Everett

    if FAILED( hr )
    {
        if  ( hr == E_NOINTERFACE ) // If no IMetaDataTables2 then we are Everett
        {
            m_RuntimeVersion = RuntimeVersion1;
            return S_OK;
        }
        else
        {
            return hr; // pass the error on.
        }
    }

    // Read the metadata schema number out of mscorlib.
    // What we care about is whether we are post-Everett, or not, because
    // we use that knowledge to tailor how we treat code paths that deal
    // with generics in the compiler and expression evaluator.

    // #~ stream header (see CLI specification 24.2.6 "#~ stream")
    struct UncompressedStreamHeader
    {
        ULONG       m_ulReserved; // Reserved, must be zero.
        BYTE        m_major;          // Version numbers.
        BYTE        m_minor;
        BYTE        m_heaps;          // Bits for heap sizes.
        BYTE        m_rid;               // log-base-2 of largest rid.
        unsigned __int64    m_maskvalid;   // Bit mask of present table counts.
        unsigned __int64    m_sorted;        // Bit mask of sorted tables.
    };

    // Find the #~ stream which contains the schema version #
    for ( int StreamIndex =0; ; StreamIndex++ )
    {
        const char *pStreamName;
        const void *pStreamData;
        ULONG       DataSizeInBytes;

        // We've succeeded above to get IMetaDataTables2, so if we fail here we may have a corrupted image, or other serious error, so bail.
        IfFailRet( srpTables->GetMetaDataStreamInfo( StreamIndex, &pStreamName, &pStreamData, &DataSizeInBytes ))

        if ( hr == S_FALSE ) // S_FALSE is how you know that we've iterated through all of the streams
        {
            break;
        }

        // Look for the stream contianing the metadata schema version
        if ( pStreamName && strcmp( pStreamName, UNCOMPRESSED_METADATA_STREAM ) == 0 )
        {
            VSASSERT( DataSizeInBytes >= sizeof(struct UncompressedStreamHeader), "ERROR: the size of the uncompressed stream is less than its header size - corrupted image!" );
            if ( DataSizeInBytes >= sizeof( struct UncompressedStreamHeader ))
            {
                struct UncompressedStreamHeader *pUncompressedStreamHeader = (struct UncompressedStreamHeader *)pStreamData;
                // we only care about the distinction between pre-Whidbey and >= Whidbey so everything after version 1 is considered Version2
                m_RuntimeVersion = pUncompressedStreamHeader->m_major == 1 ? RuntimeVersion1 : RuntimeVersion2;
                break;
            }
            else
            {
                return E_FAIL;
            }
        } // found the stream containing schema version info
    } // iterate through the streams
    return S_OK;
}

/*****************************************************************************
;AutoSetVbLibraryType

In the debugging scenario, we don't know what kind of project we are compiling against.  It may be
desktop, compact framework, etc.  In the case of the compact framework (also known as Starlite)
we need to know so we can do appropriate expression evaluation in the debugger.  For instance,
late-binding isn't supported in starlight.  So this function looks at MSCorLib.dll, and uses the public key
token (which isn't supposed to change release to release) to determine if this is a NETCF project we
are debugging against.
*****************************************************************************/
void CompilerHost::AutoSetVbLibraryType()
{
    AssertIfFalse(m_PlatformType == TLB_AutoDetect);

    m_PlatformType = TLB_Desktop;
    m_IsAutoLibraryType = true;

    ULONG StringLength = 0;
    WCHAR *PublicKeyToken = this->m_pComPlusProject->GetAssemblyIdentity()->GetPublicKeyTokenString( &StringLength );
    if ( PublicKeyToken )
    {
        if ( StringLength > 0 )
        {
            if ( _wcsicmp( PublicKeyToken, NETCF_PUBLIC_KEY_TOKEN ) == 0)
            {
                m_PlatformType = TLB_Starlite;
            }
        }
        delete[] PublicKeyToken;
    }
}

#if IDE 
long CompilerHost::GetProjectsToCompileCount()
{
    CompilerIdeLock spLock(m_csProjects);
    return m_daProjectsToCompile.Count();
}

void CompilerHost::OptimizeProjectCompilationListHelper
(
    DynamicArray<CompilerProject *> *pPreferredProjects,
    CompilationState csDesiredState,
    SourceFile * pPreferredSourceFile
)
{
    // Nothing to do if no projects passed in.
    if (!pPreferredProjects)
    {
        return;
    }

    BackgroundCompilerLock compilerLock(this);
    CompilerIdeLock spLock(m_csProjects);

    RebuildProjectCompilationList(pPreferredProjects);

    // Store the preferred project and its state (pick the first one if multiple)
    // These are used when updating the progress dialog.
    SetPreferredSourceFile(pPreferredSourceFile);
    m_csProjectToWaitOn = csDesiredState;

    // Store the CompilerHost that the Preferred Project belongs to.
    // This is used in the Wait dialog to access the preferred project information.
    GetCompilerSharedState()->SetWaitForStateCompilerHost(this);

    // Set a flag to avoid the optimization that skips CompileAllProjects during
    // a soln build.
    GetCompilerSharedState()->SetBackgroundCompileRequested(true);
}

void CompilerHost::RebuildProjectCompilationList(DynamicArray<CompilerProject *> *pPreferredProjects)
{
    bool WasCompilationListReady = DestroyProjectCompilationList();

    m_pCompiler->BuildProjectCompilationList(
        NULL,
        this,
        &m_daProjectsToCompile,
        !WasCompilationListReady,
        pPreferredProjects);

    m_pProjectToWaitOn = pPreferredProjects->Element(0);
}

void CompilerHost::CalculateStepsToCompilePreferredProject()
{
    if (m_pProjectToWaitOn)
    {
        // Now calculate how many steps required to reach the desired state.
        // 1 step = 1 file changing one state. e.g. File1 goes from CS_NoState to CS_Declared = 1 step.
        // Note: This calculation assumes that all files in a given project have the same state as the project.
        ULONG steps = 0;
        for (ULONG i = 0; i < m_daProjectsToCompile.Count(); i++)
        {
            CompilerProject *pCompilerProject = m_daProjectsToCompile.Element(i);

            if (pCompilerProject &&
                pCompilerProject ->GetCompState() < m_csProjectToWaitOn)
            {

                for (CompilationState cs = CS_NoState; cs < m_csProjectToWaitOn; cs = (CompilationState)(cs + 1))
                {
                    CDoubleListForwardIter<CompilerFile> iter(&(pCompilerProject->m_dlFiles[cs]));

                    steps += (m_csProjectToWaitOn - cs) * pCompilerProject->m_dlFiles[cs].NumberOfEntries();
                }

                // Stop counting when we hit the preferred project as the remaining projects
                // will be compiled AFTER this one is done.
                if (m_pProjectToWaitOn == pCompilerProject)
                {
                    break;
                }
            }
        }

        m_StepsToCompilePreferredProject = steps;
    }
    else
    {
        VSFAIL("should be waiting on a project");
        m_StepsToCompilePreferredProject = 0;
    }

    // Reset the step count so we get an accurate count of the steps from when
    // the background thread starts up.
    m_CurrentCompilationStepCount = 0;

}

#endif

//============================================================================
// Get an instance of the compiler.
//============================================================================
STDAPI VBCreateBasicCompiler
(
    BOOL CompilingTheVBRuntime, // True when we are compiling Microsoft.VisualBasic.dll
    LoadUICallback *pfnLoadUI, // callback function for loading UI resources
    IVbCompilerHost* pCompilerHost, // the host to associate with this instance of the compiler
    IVbCompiler **ppNewInstanceOfTheCompiler // [out] the created instance of the compiler
)
{
    VB_ENTRY();
    // pass in pCompilerHost only for the purpose of getting the runtime name in our lookup table.  We'll still have to go through
    // the Registration process below which loads the default libraries and so on.
    Compiler *pCompiler;
    pCompiler = new (zeromemory) Compiler(CompilingTheVBRuntime, pfnLoadUI, pCompilerHost );
    pCompiler->AddRef();

#if IDE 
   pCompiler->RegisterVbCompilerHost(pCompilerHost);
#else
    hr = pCompiler->RegisterVbCompilerHost(pCompilerHost);
#endif

    *ppNewInstanceOfTheCompiler = pCompiler;

    VB_EXIT_NORETURN();

    RRETURN( hr );
}


//============================================================================
// Initialize the compiler.
//============================================================================

Compiler::Compiler
(
    bool CompilingTheVBRuntime, // Are we compiling the VB runtime?
    LoadUICallback *pfnLoadUI, // we load our ui strings via this function
    IVbCompilerHost *pCompilerHost // global state relevant to the project being compiled
)
: m_CompilerHostsList(true)
, m_CLRTypeNameEncoderDecoder(this)
, m_CompilingTheVBRuntime(CompilingTheVBRuntime)
#if IDE 
, m_CLRTypeNameEncoderDecoderIDE(this)
, m_fCompilationInMainThread(false)
, m_fBackgroundThreadNeedsCreating(false)
, m_fBackgroundThreadStoppedForReplaceAll(false)
, ThreadSyncManager("Compiler")
#endif
, m_NamesOfRiskyMSVBFSFunctions(VBAllocWrapper(), DefaultHashFunction<STRING*>(), STRING_CONST_MSVBFSSetAttr +1 - STRING_CONST_MSVBFSDir)
{
#if !IDE 
    m_pProjectBeingCompiled = NULL;
#endif
    HRESULT hr = NOERROR;

#if IDE
    g_pCompilerSharedState = new CompilerSharedState(this);
#endif

    m_pfnLoadUIDll = pfnLoadUI; // can't do this in initialization list because it is static

    // Create the virtual allocators
    m_pStringPool = new (zeromemory) StringPool();

    // Create the eternal no-release allocator  . 
    m_pEternalStorage = new NorlsAllocator(NORLSLOC);

    // Create the namespace ring table.

    #if FV_TRACK_MEMORY && IDE
    m_instrumentation.Init(this);
    #endif

    Symbols NamespaceRingCreator(this, m_pEternalStorage, NULL);
    m_pNamespaceRings = NamespaceRingCreator.GetHashTable(NULL, NULL, false, 101, NULL);

    //
    // Initialize the keywords.
    //


    //
    // Initialize the library call table.
    //
    m_pstrRuntimeFunction[RuntimeFunctionChr]  =  STRING_CONST(this, chr);
    m_pstrRuntimeFunction[RuntimeFunctionChrW] = STRING_CONST(this, chrw);
    m_pstrRuntimeFunction[RuntimeFunctionAsc]  = STRING_CONST(this, asc);
    m_pstrRuntimeFunction[RuntimeFunctionAscW] = STRING_CONST(this, ascw);

    //
    // Initialize the runtime class/library table.
    //
    for (unsigned iLibrary = 0; iLibrary < MaxRuntimeLibrary; iLibrary++)
    {
        m_rgpstrRuntimeLibraries[iLibrary] = AddString(g_rgRTLangLibraries[iLibrary].wszLibraryName);
    }

    for (unsigned iClass = 0; iClass < MaxRuntimeClass; iClass++)
    {
        m_rgpstrRuntimeClasses[iClass] = AddString(g_rgRTLangClasses[iClass].wszClassName);
    }

    //
    // Initialize the operator names table.
    //
    {
        const WCHAR *OperatorCLSNames[] =
        {
            #define DEF_OPERATOR(op, clsname, token, isunary, isbinary, isconversion)  WIDE(clsname),
            #include "Symbols\TypeTables.h"
            #undef DEF_OPERATOR
        };

        for (unsigned op = OperatorUNDEF + 1; op < OperatorMAXVALID; op++)
        {
            m_OperatorCLSNames[op] = AddString(OperatorCLSNames[op]);
        }
        for (unsigned op = OperatorMAXVALID + 1; op < OperatorMAX; op++)
        {
            m_OperatorCLSNames[op] = AddString(OperatorCLSNames[op]);
        }
    }

    if (m_pfnCompareAssemblyIdentity == NULL)
    {
#ifndef FEATURE_CORESYSTEM
        LegacyActivationShim::GetRealProcAddress("CompareAssemblyIdentity", (void**)&m_pfnCompareAssemblyIdentity);
#endif
    }

    if (m_pfnCreateAssemblyNameObject == NULL)
    {
#ifndef FEATURE_CORESYSTEM
        LegacyActivationShim::GetRealProcAddress("CreateAssemblyNameObject", (void**)&m_pfnCreateAssemblyNameObject);
#else
        m_pfnCreateAssemblyNameObject = (PfnCreateAssemblyNameObject) &CreateAssemblyNameObject;
#endif
    }

    // Dev10 #850039 Fill hash set of names of "risky" functions from Microsoft.VisualBasic.FileSystem.
    for (unsigned i = STRING_CONST_MSVBFSDir; i <= STRING_CONST_MSVBFSSetAttr; i++)
    {
        m_NamesOfRiskyMSVBFSFunctions.Add(GetStringConstant(i));
    }
}

//============================================================================
// Terminate the internal compiler data structures.
//============================================================================

Compiler::~Compiler()
{
    // First, destroy the list of hosts. This will release the COM+ and Microsoft.VisualBasic.dll
    // project for each host.
    m_CompilerHostsList.DeleteAll();

    VSASSERT(GetProjectBeingCompiled() == NULL, "How can we still be compiling?");

#if IDE 
    // VS#546896
    // Clear the module's elements before the compiler gets destroyed because the elements are holding
    // onto memory which was allocated by the compiler's allocators (specifically, CComTypeInfoHolder instances).
    _Module.ForceTerminate();
#endif

    // Destroy the string pool.
    delete m_pStringPool;
    m_pStringPool = NULL;

    // Destroy the eternal allocator. (Forever ends here.)
    delete m_pEternalStorage;
    m_pEternalStorage = NULL;

    #if FV_TRACK_MEMORY && IDE
    m_instrumentation.Destroy();
    #endif

    m_pNamespaceRings = NULL;

#if IDE
    if (g_pCompilerSharedState)
    {
        delete g_pCompilerSharedState;
        g_pCompilerSharedState = NULL;
    }
#endif
}

//****************************************************************************
// IUnknown implementation
//****************************************************************************

STDMETHODIMP Compiler::QueryInterface
(
    REFIID riid,
    void **ppv
)
{
    void *pv = NULL;

    if (riid == IID_IUnknown)
    {
        pv = static_cast<IUnknown *>(static_cast<IVbCompiler *>(this));
    }
    else if (riid == IID_IVbCompiler)
    {
        pv = static_cast<IVbCompiler *>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    ((IUnknown *)pv)->AddRef();

    *ppv = pv;
    return NOERROR;
}

STDMETHODIMP_(ULONG) Compiler::AddRef()
{
    return InterlockedIncrement((long *)&m_cRefs);
}

STDMETHODIMP_(ULONG) Compiler::Release()
{
    ULONG cRefs = InterlockedDecrement((long *)&m_cRefs);

    if (cRefs == 0)
    {
        m_cRefs = 1;
        delete this;
    }

    return cRefs;
}

//****************************************************************************
// IVbCompiler implementation
//****************************************************************************

//============================================================================
// Create a compiler that holds a VB project. The new project is created for
// a particular host.
//============================================================================

STDMETHODIMP Compiler::CreateProject
(
    LPCWSTR wszFileName,
    IUnknown *punkProject,
    IVsHierarchy *pHier,
    IVbCompilerHost *pVbCompilerHost,
    IVbCompilerProject **ppCompilerProject
)
{
    return CreateProjectInternal(
        wszFileName,
        punkProject,
        pHier,
        pVbCompilerHost,
        ppCompilerProject,
        false /* isExpressionEditorProject */);
}

STDMETHODIMP Compiler::CreateProjectInternal
(
    _In_ LPCWSTR wszFileName,
    _In_ IUnknown *punkProject,
    _In_ IVsHierarchy *pHier,
    _In_ IVbCompilerHost *pVbCompilerHost,
    _Out_ IVbCompilerProject **ppCompilerProject,
    _In_ bool isExpressionEditorProject
)
{
    CompilerProject *pCompilerProject = NULL;
    VB_ENTRY();

    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();

    CompilerHost *pCompilerHost = NULL;

    // First, find the CompilerHost to create a new prject for. If we
    // cannot find it, then we have a problem.
    if (!FindCompilerHost(pVbCompilerHost, &pCompilerHost))
    {
        VSFAIL("CompilerHost does not exist!");
        *ppCompilerProject = NULL;
        return E_FAIL;
    }

    STRING *pstrFileName;

    *ppCompilerProject = NULL;

    pstrFileName = AddString(wszFileName);

    // Does this already exist?
#if IDE 
    pCompilerProject = pCompilerHost->FindProject(pstrFileName, false /* fMetaDataProject */, pHier);
#else
    pCompilerProject = pCompilerHost->FindProject(pstrFileName, false /* fMetaDataProject */);
#endif IDE

    if (!pCompilerProject)
    {
        pCompilerProject = new (zeromemory) CompilerProject(this);

#if IDE
        // Set the isExpressionEditorProject flag now to prevent unnecessary
        // work such as setting up the My extensions.
        if (isExpressionEditorProject)
        {
            pCompilerProject->SetIsExpressionEditorProject();
        }
#endif

        IfFailGo(pCompilerProject->StartEdit());

        // Add the project to the list of projects under this host.
        pCompilerHost->InsertProject(pCompilerProject);
        IfFailGo(pCompilerProject->InitProject(pstrFileName, punkProject, pHier, pCompilerHost));

        IfFailGo(pCompilerProject->FinishEdit());
        OnProject(CLXN_Add, pCompilerProject);
    }

    *ppCompilerProject = pCompilerProject;
    pCompilerProject = NULL;

    VB_EXIT_NORETURN();

Error:
    if (pCompilerProject)
    {
        pCompilerProject->Release();
    }

    RRETURN( hr );
}

//============================================================================
// Compile all loaded projects.
//============================================================================
STDMETHODIMP Compiler::Compile
(
    ULONG *pcWarnings,
    ULONG *pcErrors,
    VbError **ppErrors
)
{
    HRESULT _hr = S_OK;
    VB_ENTRY();
    bool fHasErrors = false;

    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();

    if (ppErrors)
    {
        *ppErrors = NULL;
    }

   for (CompilerHost *pCurrentCompilerHost = m_CompilerHostsList.GetFirst();
         pCurrentCompilerHost;
         pCurrentCompilerHost = pCurrentCompilerHost->Next())
    {
        // Reset the stats.
        m_cErrors   = 0;
        m_cWarnings = 0;

        DynamicArray<CompilerProject *> *pCurrentSetOfProjectsToCompile = pCurrentCompilerHost->GetProjectsToCompile();

        if (pCurrentSetOfProjectsToCompile->Count() == 0)
        {
            // Build the list of projects to compile.
            IfFailThrow(BuildProjectCompilationList(NULL, pCurrentCompilerHost, pCurrentSetOfProjectsToCompile, true));
        }

        unsigned          cProjects   = pCurrentSetOfProjectsToCompile->Count();
        CompilerProject **rgpProjects = pCurrentSetOfProjectsToCompile->Array();
        unsigned iProject;

        // Walk the list of projects and build them.

        // Clear the error cache
        pCurrentCompilerHost->ClearLookupCaches();

        if (pCurrentCompilerHost->GetComPlusProject())
        {
            // Build mscorlib completely beforehand because other projects need to load
            // children of types in mscorlib during their transition to CS_Declared itself.
            IfTrueGo(pCurrentCompilerHost->GetComPlusProject()->Compile(), E_FAIL);

            VSASSERT(rgpProjects[0] == pCurrentCompilerHost->GetComPlusProject(), "Project compilation list order wrong!!!");
        }

        // Build the symbol table for all projects.
        for (iProject = 1; iProject < cProjects; iProject++)
        {
            IfTrueGo(rgpProjects[iProject]->CompileToDeclared(), E_FAIL);
        }

        // Bind the symbols
        for (iProject = 1; iProject < cProjects; iProject++)
        {
            IfTrueGo(rgpProjects[iProject]->CompileFromDeclaredToBound(), E_FAIL);
        }

        // Compile method bodies
        for (iProject = 1; iProject < cProjects; iProject++)
        {
            IfTrueGo(rgpProjects[iProject]->CompileFromBoundToCompiled(), E_FAIL);

            if (!rgpProjects[iProject]->CompiledSuccessfully())
            {
                fHasErrors = true;
            }
        }

        // Clear the error cache
        pCurrentCompilerHost->ClearLookupCaches();

        // Do the last part of the compile if all projects
        // compiled successfully up to now.
        //
        if (!fHasErrors)
        {
            for (iProject = 0; iProject < cProjects; iProject++)
            {
                rgpProjects[iProject]->FinishCompile();
            }
        }
    }

    BuildErrorList(ppErrors, &m_cErrors, &m_cWarnings);

    VB_EXIT_NORETURN();

Error:
    // We want to report any errors even if the compilation failed
    if (pcErrors)
    {
        *pcErrors = m_cErrors;
    }
    if (pcWarnings)
    {
        *pcWarnings = m_cWarnings;
    }

    RRETURN( hr );
}

// Compiles all projects under one host to Bound state.
// This does not belong to IVbCompiler. Since our expression valuator
// needs to initialize mscorlib.dll and microsoft.visualbasic.dll in
// cpde.dll thread, we should be able to build symbols.
STDMETHODIMP Compiler::CompileToBound
(
    CompilerHost *pCompilerHost,
    ULONG *pcWarnings,
    ULONG *pcErrors,
    VbError **ppErrors
)
{
    HRESULT _hr = S_OK;
    VB_ENTRY();

    ClearErrorInfo();

    if (ppErrors)
    {
        *ppErrors = NULL;
    }

    // Reset the stats.
    m_cErrors   = 0;
    m_cWarnings = 0;

    // Holds the list of projects to compile, in the right order.
    DynamicArray<CompilerProject *> CurrentSetOfProjectsToCompile;

    // Build the list of projects to compile.
    IfFailThrow(BuildProjectCompilationList(NULL, pCompilerHost, &CurrentSetOfProjectsToCompile, true));

    // Walk the list and build it.
    unsigned          cProjects   = CurrentSetOfProjectsToCompile.Count();
    CompilerProject **rgpProjects = CurrentSetOfProjectsToCompile.Array();

    pCompilerHost->ClearLookupCaches();

    if (pCompilerHost->GetComPlusProject())
    {
        // Build mscorlib completely beforehand because other projects need to load
        // children of types in mscorlib during their transition to CS_Declared itself.
        IfTrueGo(pCompilerHost->GetComPlusProject()->Compile(), E_FAIL);

        VSASSERT(rgpProjects[0] == pCompilerHost->GetComPlusProject(), "Project compilation list order wrong!!!");
    }

    for (unsigned iProject = 1; iProject < cProjects; iProject++)
    {
        IfTrueGo(rgpProjects[iProject]->CompileToDeclared(), E_FAIL);
    }

    for (unsigned iProject = 1; iProject < cProjects; iProject++)
    {
        IfTrueGo(rgpProjects[iProject]->CompileFromDeclaredToBound(), E_FAIL);
    }

    BuildErrorList(ppErrors, &m_cErrors, &m_cWarnings);
    VB_EXIT_NORETURN();

Error:
    // We want to report any errors even if the compilation failed
    if (pcErrors)
    {
        *pcErrors = m_cErrors;
    }
    if (pcWarnings)
    {
        *pcWarnings = m_cWarnings;
    }

    RRETURN( hr );
}

//============================================================================
// Set the output level for the compiler.
//============================================================================

STDMETHODIMP Compiler::SetOutputLevel(OUTPUT_LEVEL OutputLevel)
{
    VB_ENTRY();
    m_OutputLevel = OutputLevel;

    VB_EXIT();

    return NOERROR;
}

//============================================================================
// Set the debug switches.
//============================================================================

STDMETHODIMP Compiler::SetDebugSwitches(BOOL dbgSwitches[])
{
#if DEBUG
    VB_ENTRY();
    if (dbgSwitches[DBG_fDumpSymbols])
    {
        VSFSWITCH(fDumpSymbols) = true;
    }
    if (dbgSwitches[DBG_fDumpNonVBSymbols])
    {
        VSFSWITCH(fDumpNonVBSymbols) = true;
    }
    if (dbgSwitches[DBG_fDumpDeclTrees])
    {
        VSFSWITCH(fDumpDeclTrees) = true;
    }
    if (dbgSwitches[DBG_fDumpBoundMethodTrees])
    {
        VSFSWITCH(fDumpBoundMethodTrees) = true;
    }
    if (dbgSwitches[DBG_fDumpCallGraph])
    {
        VSFSWITCH(fDumpCallGraph) = true;
    }
    if (dbgSwitches[DBG_fDumpText])
    {
        VSFSWITCH(fDumpText) = true;
    }
    if (dbgSwitches[DBG_fDumpLineTable])
    {
        VSFSWITCH(fDumpLineTable) = true;
    }
    if (dbgSwitches[DBG_fDumpUnboundMethodTrees])
    {
        VSFSWITCH(fDumpUnboundMethodTrees) = true;
    }
    if (dbgSwitches[DBG_fDumpBasicRep])
    {
        VSFSWITCH(fDumpBasicRep) = true;
    }
    if (dbgSwitches[DBG_fDumpXML])
    {
        VSFSWITCH(fDumpXML) = true;
    }
    if (dbgSwitches[DBG_fDumpFlow])
    {
        VSFSWITCH(fDumpFlow) = true;
    }
    if (dbgSwitches[DBG_fDumpSyntheticCode])
    {
        VSFSWITCH(fDumpSyntheticCode) = true;
    }
    if (dbgSwitches[DBG_fTestCrash])
    {
        VSFSWITCH(fTestCrash) = true;
    }
    if (dbgSwitches[DBG_fDumpClosures])
    {
        VSFSWITCH(fDumpClosures) = true;
    }
    if (dbgSwitches[DBG_fDumpInference])
    {
        VSFSWITCH(fDumpInference) = true;
    }
    if (dbgSwitches[DBG_fDumpOverload])
    {
        VSFSWITCH(fDumpOverload) = true;
    }
    if (dbgSwitches[DBG_fDumpRelaxedDel])
    {
        VSFSWITCH(fDumpRelaxedDel) = true;
    }

    VB_EXIT();
#endif DEBUG

    return NOERROR;
}

/***
*IsValidIdentifier
*Purpose:
*  returns TRUE if the lpstr given is a valid Basic identifier
*
*Exit:
*  Returns EBERR.
***************************************************************************/
STDMETHODIMP Compiler::IsValidIdentifier(_In_z_ WCHAR *wszIdentifier, BOOL *pfIsValid)
{
    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();

    VB_ENTRY();
    VSASSERT(wszIdentifier && pfIsValid, "Invalid state");
    STRING * pIDName = AddString(wszIdentifier);
    tokens tk = TokenOfString(pIDName);
    //VS#252239
    if (!Scanner::IsValidIdentiferOrBracketedIdentifier(wszIdentifier) ||
        TokenIsReserved(tk))
    {
        *pfIsValid = FALSE;
    }
    else
    {
        *pfIsValid = TRUE;
    }

    VB_EXIT();

    return NOERROR;
}

//==============================================================================
// Set the logging options for the compiler
//==============================================================================
STDMETHODIMP Compiler::SetLoggingOptions(DWORD options)
{
    return S_OK;
}

// Finds the CompilerHost associated with pVbCompilerHost. Returns true found, or false if not.
bool Compiler::FindCompilerHost(IVbCompilerHost *pVbCompilerHost, CompilerHost **ppCompilerHost)
{
    bool FoundCompilerHost = false;
    CompilerHost *pCurrentCompilerHost = m_CompilerHostsList.GetFirst();

    for (; pCurrentCompilerHost; pCurrentCompilerHost = pCurrentCompilerHost->Next())
    {
        if (pVbCompilerHost == NULL && pCurrentCompilerHost->m_pIVbCompilerHost == NULL)
        {
            FoundCompilerHost = true;
            break;
        }
        else
        {
            if (pVbCompilerHost != NULL && pCurrentCompilerHost->m_pIVbCompilerHost != NULL)
            {
                //CComPtr<IUnknown> srpRequestedHost = NULL;
                //CComPtr<IUnknown> srpAvailableHost = NULL;

                //if (FAILED(pVbCompilerHost->QueryInterface(IID_IUnknown, (void **)&srpRequestedHost)))
                //{
                //    VSFAIL("QueryInterface FAILED");
                //    break;
                //}

                //if (FAILED(pCurrentCompilerHost->m_pIVbCompilerHost->QueryInterface(IID_IUnknown, (void **)&srpAvailableHost)))
                //{
                //    VSFAIL("QueryInterface FAILED");
                //    break;
                //}

                //if (srpRequestedHost == srpAvailableHost)
                //{
                //    FoundCompilerHost = true;
                //    break;
                //}
                
                if (CompareCompilerHosts(pVbCompilerHost, pCurrentCompilerHost->m_pIVbCompilerHost))
                {
                    FoundCompilerHost = true;
                    break;
                }
            }
        }
    }

    if (ppCompilerHost)
    {
        if (FoundCompilerHost)
        {
            *ppCompilerHost = pCurrentCompilerHost;
        }
        else
        {
            *ppCompilerHost = NULL;
        }
    }

    return FoundCompilerHost;
}

bool Compiler::CompareCompilerHosts(_In_ IVbCompilerHost *pCompilerHost1, _In_ IVbCompilerHost *pCompilerHost2)
{
    // Default to the desktop target type
    VBTargetLibraryType targetType1 = TLB_Desktop;
    VBTargetLibraryType targetType2 = TLB_Desktop;
    
    pCompilerHost1->GetTargetLibraryType(&targetType1);
    pCompilerHost2->GetTargetLibraryType(&targetType2);
    
    // If the target types match compare the SDK paths
    if (targetType2 == targetType1)
    {
        CComBSTR sdkPath1 = NULL;
        CComBSTR sdkPath2 = NULL;

        HRESULT hr = pCompilerHost1->GetSdkPath(&sdkPath1);
        if (FAILED(hr))
        {
            VSFAIL("Missing SDKPath on IVbCompilerHost");
            return false;
        }        
        
        hr = pCompilerHost2->GetSdkPath(&sdkPath2);
        if (FAILED(hr)) 
        {
            VSFAIL("Missing SDKPath on IVbCompilerHost");
            return false;
        }

        if (sdkPath1.Length() == 0 && sdkPath2.Length() == 0)
        {
            VSASSERT(false, "The SDK path is empty, which means this will use a default compiler host");
            return true;
        }
        
        // Remove trailing backslash if present.
        size_t wszLength1 = sdkPath1.Length();
        if (sdkPath1[wszLength1-1]==L'\\')
        {
            sdkPath1[wszLength1-1]=0;
        }
        
        size_t wszLength2 = sdkPath2.Length();
        if (sdkPath2[wszLength2-1]==L'\\')
        {
            sdkPath2[wszLength2-1]=0;
        }                
        
        if (CompareFilenames(sdkPath1, sdkPath2)==0)
        {
            return true;
        }
    }
    return false;
}

bool 
Compiler::UnregisterVbCompilerHost( _In_ IVbCompilerHost *pVbCompilerHost)
{
    ThrowIfNull( pVbCompilerHost );
    CompilerHost *pHost;
    if ( !FindCompilerHost(pVbCompilerHost, &pHost) )
    {
        return false;
    }

    m_CompilerHostsList.Remove(pHost);
    delete pHost;

    return true;
}

// Create a new CompilerHost and Load the default libraries.
HRESULT Compiler::CreateNewCompilerHost(IVbCompilerHost *pIVbCompilerHost)
{
    HRESULT hr = S_OK;
    CompilerHost *pNewCompilerHost = new (zeromemory) CompilerHost(this, pIVbCompilerHost);

    IfFailGo(InitializeDefaultLibraries(pNewCompilerHost));
    IfFailGo(pNewCompilerHost->InitStandardLibraryList());

    m_CompilerHostsList.InsertLast(pNewCompilerHost);

#if IDE

    // Try and bind the core libraries for the host here.  This operation will fail for the first
    // project added to the solution because we will have an outer batch edit in progress from
    // the core project system.  It should work for subsequent projects added to the solution
    pNewCompilerHost->TryBindCoreLibraries();    

#endif

Error:

    if (FAILED(hr))
    {
        delete pNewCompilerHost;
    }

    return hr;
}


//============================================================================
// Registers a new CompilerHost with the Compiler. If a CompilerHost for the
// same host is found, we return that one, otherwise, a new Host is created.
//============================================================================
STDMETHODIMP Compiler::RegisterVbCompilerHost(IVbCompilerHost *pVbCompilerHost)
{
    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();

    if (FindCompilerHost(pVbCompilerHost, NULL))
    {
        return S_FALSE;
    }

    return CreateNewCompilerHost(pVbCompilerHost);
}

//============================================================================
// Sets the Watson behavior for the compiler and informs the compiler about
// any additional files that should be included with the Watson report.
//============================================================================

extern void SetWatsonType(WATSON_TYPE WatsonType, LCID WatsonLcid, _In_opt_z_ WCHAR *wszAdditionalFiles);

STDMETHODIMP Compiler::SetWatsonType(WATSON_TYPE WatsonType, LCID WatsonLcid, _In_opt_z_ WCHAR *wszAdditionalFiles)
{
    VB_ENTRY();
    ::SetWatsonType(WatsonType, WatsonLcid, wszAdditionalFiles);

    VB_EXIT();

    return NOERROR;
}

//****************************************************************************
// Project management.
//****************************************************************************

#if IDE 
//=============================================================================
// See if a project exists with a given id
//=============================================================================
CompilerProject *Compiler::FindProjectById
(
    LONG projectId
)
{
    AllProjectsIterator projectIterator(this);
    CompilerProject *pCompilerProject = NULL;

    while (pCompilerProject = projectIterator.Next())
    {
        if (pCompilerProject->GetProjectId() == projectId)
        {
            return pCompilerProject;
        }
    }

    return NULL;
}
#endif IDE


// Record what project is being compiled
//
// We store it in a thread local to prevent race conditions between the IDE
// and the background compiler (Dev10#716018). This is needed because NoPIA
// uses this value for finding the canonical PIA type given an embedded
// type. While we're compiling one project, we could be looking for
// intellisense in another, and we would get wrong results (at best), or
// wose, corrupted state if we return a PIA type from a different 
// CompilerHost to intellisense.
void Compiler::SetProjectBeingCompiled(CompilerProject *pProjectBeingCompiled)
{
#if IDE 
    CompilerProject **ppCompilerProject =  VBTLSThreadData::GetProjectBeingCompiled();
    *ppCompilerProject = pProjectBeingCompiled;
#else
    m_pProjectBeingCompiled = pProjectBeingCompiled;
#endif
}

    // Return the project that's currently being compiled
CompilerProject *Compiler::GetProjectBeingCompiled()
{
#if IDE 
    // TlsGetValue will result in NULL if this it is not set, exactly what we want to happen.
    CompilerProject **ppCompilerProject =  VBTLSThreadData::GetProjectBeingCompiled();
    return *ppCompilerProject ;
#else
    return m_pProjectBeingCompiled;
#endif
}





//****************************************************************************
// Accessors for stored context.
//****************************************************************************

//============================================================================
// Obtains and returns the satellite DLL that contains our error
// message resources.
//============================================================================

HINSTANCE Compiler::GetResourceDll()
{
    if (m_hinstResource == NULL)
    {
        VSASSERT(m_pfnLoadUIDll != NULL, "NULL callback.");
        IfFailThrow(m_pfnLoadUIDll(&m_hinstResource));
    }

    return m_hinstResource;
}

void Compiler::LoadSolutionExtension(SolutionExtensionEnum extension, HMODULE hModRes, WORD wResourceId, CompilerHost *pCompilerHost)
{
    if (hModRes)
    {
        HRSRC hRes = ::FindResource(hModRes, MAKEINTRESOURCE(wResourceId), RT_RCDATA);
        if (hRes)
        {
            HGLOBAL hGlobal = ::LoadResource(hModRes, hRes);
            DWORD dwByte = ::SizeofResource(hModRes, hRes);
            if (hGlobal && dwByte)
            {
                void *pCode = ::LockResource(hGlobal);
                pCompilerHost->SetSolutionExtension(pCode, dwByte, extension);
            }
        }
    }
}

void Compiler::LoadXmlSolutionExtension(CompilerHost *pCompilerHost)
{
    char XmlHelperCode[] =
        "Namespace My\n"
        "  <Global.System.Diagnostics.DebuggerNonUserCodeAttribute(), _\n"
        "   Global.System.Runtime.CompilerServices.CompilerGeneratedAttribute(), _\n"
        "   Global.System.ComponentModel.EditorBrowsableAttribute(Global.System.ComponentModel.EditorBrowsableState.Never)> _\n"
        "  Friend NotInheritable Class InternalXmlHelper\n"
        "    <Global.System.ComponentModel.EditorBrowsableAttribute(Global.System.ComponentModel.EditorBrowsableState.Never)> _\n"
        "    Private Sub New()\n"
        "    End Sub\n"
        "    <Global.System.Runtime.CompilerServices.ExtensionAttribute()> _\n"
        "    Public Shared Property Value(source As Global.System.Collections.Generic.IEnumerable(Of Global.System.Xml.Linq.XElement)) As String\n"
        "      Get\n"
        "        For Each item As Global.System.Xml.Linq.XElement In source\n"
        "          Return item.Value\n"
        "        Next\n"
        "        Return Nothing\n"
        "      End Get\n"
        "      Set(value As String)\n"
        "        For Each item As Global.System.Xml.Linq.XElement In source\n"
        "          item.Value = value\n"
        "          Exit For\n"
        "        Next\n"
        "      End Set\n"
        "    End Property\n"
        "    <Global.System.Runtime.CompilerServices.ExtensionAttribute()> _\n"
        "    Public Shared Property AttributeValue(source As Global.System.Collections.Generic.IEnumerable(Of Global.System.Xml.Linq.XElement), name As Global.System.Xml.Linq.XName) As String\n"
        "      Get\n"
        "        For Each item As Global.System.Xml.Linq.XElement In source\n"
        "          Return CType(item.Attribute(name), String)\n"
        "        Next\n"
        "        Return Nothing\n"
        "      End Get\n"
        "      Set(value As String)\n"
        "        For Each item As Global.System.Xml.Linq.XElement In source\n"
        "          item.SetAttributeValue(name, value)\n"
        "          Exit For\n"
        "        Next\n"
        "      End Set\n"
        "    End Property\n"
        "    <Global.System.Runtime.CompilerServices.ExtensionAttribute()> _\n"
        "    Public Shared Property AttributeValue(source As Global.System.Xml.Linq.XElement, name As Global.System.Xml.Linq.XName) As String\n"
        "      Get\n"
        "        Return CType(source.Attribute(name), String)\n"
        "      End Get\n"
        "      Set(value As String)\n"
        "        source.SetAttributeValue(name, value)\n"
        "      End Set\n"
        "    End Property\n"
        "    <Global.System.ComponentModel.EditorBrowsableAttribute(Global.System.ComponentModel.EditorBrowsableState.Never)> _\n"
        "    Public Shared Function CreateAttribute(name As Global.System.Xml.Linq.XName, value As Object) As Global.System.Xml.Linq.XAttribute\n"
        "      If value Is Nothing Then\n"
        "        Return Nothing\n"
        "      End If\n"
        "      Return New Global.System.Xml.Linq.XAttribute(name, value)\n"
        "    End Function\n"
        "    <Global.System.ComponentModel.EditorBrowsableAttribute(Global.System.ComponentModel.EditorBrowsableState.Never)> _\n"
        "    Public Shared Function CreateNamespaceAttribute(name As Global.System.Xml.Linq.XName, ns As Global.System.Xml.Linq.XNamespace) As Global.System.Xml.Linq.XAttribute\n"
        "      Dim a As New Global.System.Xml.Linq.XAttribute(name, ns.NamespaceName)\n"
        "      a.AddAnnotation(ns)\n"
        "      Return a\n"
        "    End Function\n"
"        <Global.System.ComponentModel.EditorBrowsableAttribute(Global.System.ComponentModel.EditorBrowsableState.Never)> _ \n"
"        Public Shared Function RemoveNamespaceAttributes(inScopePrefixes() As String, inScopeNs() As Global.System.Xml.Linq.XNamespace, attributes As Global.System.Collections.Generic.List(Of Global.System.Xml.Linq.XAttribute), obj As Object) As Object \n"
"            If obj IsNot Nothing Then \n"
"                Dim elem As Global.System.Xml.Linq.XElement = TryCast(obj, Global.System.Xml.Linq.XElement) \n"
"                If Not elem Is Nothing Then \n"
"                    Return RemoveNamespaceAttributes(inScopePrefixes, inScopeNs, attributes, elem) \n"
"                Else \n"
"                    Dim elems As Global.System.Collections.IEnumerable = TryCast(obj, Global.System.Collections.IEnumerable) \n"
"                    If elems IsNot Nothing Then \n"
"                        Return RemoveNamespaceAttributes(inScopePrefixes, inScopeNs, attributes, elems) \n"
"                    End If \n"
"                End If \n"
"            End If \n"
"            Return obj \n"
"        End Function \n"
"        <Global.System.ComponentModel.EditorBrowsableAttribute(Global.System.ComponentModel.EditorBrowsableState.Never)> _ \n"
"        Public Shared Function RemoveNamespaceAttributes(inScopePrefixes() As String, inScopeNs() As Global.System.Xml.Linq.XNamespace, attributes As Global.System.Collections.Generic.List(Of Global.System.Xml.Linq.XAttribute), obj As Global.System.Collections.IEnumerable) As Global.System.Collections.IEnumerable \n"
"            If obj IsNot Nothing Then \n"
"                Dim elems As Global.System.Collections.Generic.IEnumerable(Of Global.System.Xml.Linq.XElement) = TryCast(obj, Global.System.Collections.Generic.IEnumerable(Of Global.System.Xml.Linq.XElement)) \n"
"                If elems IsNot Nothing Then \n"
"                    Return Global.System.Linq.Enumerable.Select(elems, AddressOf New RemoveNamespaceAttributesClosure(inScopePrefixes, inScopeNs, attributes).ProcessXElement) \n"
"                Else \n"
"                    Return Global.System.Linq.Enumerable.Select(Global.System.Linq.Enumerable.Cast(Of Object)(obj), AddressOf New RemoveNamespaceAttributesClosure(inScopePrefixes, inScopeNs, attributes).ProcessObject) \n"
"                End If \n"
"            End If \n"
"            Return obj \n"
"        End Function \n"
"        <Global.System.Diagnostics.DebuggerNonUserCodeAttribute()> _ \n"
"        <Global.System.Runtime.CompilerServices.CompilerGeneratedAttribute()> _ \n"
"        <Global.System.ComponentModel.EditorBrowsableAttribute(Global.System.ComponentModel.EditorBrowsableState.Never)> _ \n"
"        Private NotInheritable Class RemoveNamespaceAttributesClosure \n"
"            Private ReadOnly m_inScopePrefixes As String() \n"
"            Private ReadOnly m_inScopeNs As Global.System.Xml.Linq.XNamespace() \n"
"            Private ReadOnly m_attributes As Global.System.Collections.Generic.List(Of Global.System.Xml.Linq.XAttribute) \n"
"            <Global.System.ComponentModel.EditorBrowsableAttribute(Global.System.ComponentModel.EditorBrowsableState.Never)> _ \n"
"            Friend Sub New(inScopePrefixes() As String, inScopeNs() As Global.System.Xml.Linq.XNamespace, attributes As Global.System.Collections.Generic.List(Of Global.System.Xml.Linq.XAttribute)) \n"
"                m_inScopePrefixes = inScopePrefixes \n"
"                m_inScopeNs = inScopeNs \n"
"                m_attributes = attributes \n"
"            End Sub \n"
"            <Global.System.ComponentModel.EditorBrowsableAttribute(Global.System.ComponentModel.EditorBrowsableState.Never)> _ \n"
"            Friend Function ProcessXElement(elem As Global.System.Xml.Linq.XElement) As Global.System.Xml.Linq.XElement \n"
"                Return InternalXmlHelper.RemoveNamespaceAttributes(m_inScopePrefixes, m_inScopeNs, m_attributes, elem) \n"
"            End Function \n"
"            <Global.System.ComponentModel.EditorBrowsableAttribute(Global.System.ComponentModel.EditorBrowsableState.Never)> _ \n"
"            Friend Function ProcessObject(obj As Object) As Object \n"
"                Dim elem As Global.System.Xml.Linq.XElement = TryCast(obj, Global.System.Xml.Linq.XElement) \n"
"                If elem IsNot Nothing Then \n"
"                    Return InternalXmlHelper.RemoveNamespaceAttributes(m_inScopePrefixes, m_inScopeNs, m_attributes, elem) \n"
"                Else \n"
"                    Return obj \n"
"                End If \n"
"            End Function \n"
"        End Class \n"
"        <Global.System.ComponentModel.EditorBrowsableAttribute(Global.System.ComponentModel.EditorBrowsableState.Never)> _ \n"
"        Public Shared Function RemoveNamespaceAttributes(inScopePrefixes() As String, inScopeNs() As Global.System.Xml.Linq.XNamespace, attributes As Global.System.Collections.Generic.List(Of Global.System.Xml.Linq.XAttribute), e As Global.System.Xml.Linq.XElement) As Global.System.Xml.Linq.XElement \n"
"            If e IsNot Nothing Then \n"
"                Dim a As Global.System.Xml.Linq.XAttribute = e.FirstAttribute \n"
" \n"
"                While a IsNot Nothing \n"
"                    Dim nextA As Global.System.Xml.Linq.XAttribute = a.NextAttribute \n"
" \n"
"                    If a.IsNamespaceDeclaration() Then \n"
"                        Dim ns As Global.System.Xml.Linq.XNamespace = a.Annotation(Of Global.System.Xml.Linq.XNamespace)() \n"
"                        Dim prefix As String = a.Name.LocalName \n"
" \n"
"                        If ns IsNot Nothing Then \n"
"                            If inScopePrefixes IsNot Nothing AndAlso inScopeNs IsNot Nothing Then \n"
"                                Dim lastIndex As Integer = inScopePrefixes.Length - 1 \n"
" \n"
"                                For i As Integer = 0 To lastIndex \n"
"                                    Dim currentInScopePrefix As String = inScopePrefixes(i) \n"
"                                    Dim currentInScopeNs As Global.System.Xml.Linq.XNamespace = inScopeNs(i) \n"
"                                    If prefix.Equals(currentInScopePrefix) Then \n"
"                                        If ns = currentInScopeNs Then \n"
"                                            'prefix and namespace match.  Remove the unneeded ns attribute \n"
"                                            a.Remove() \n"
"                                        End If \n"
" \n"
"                                        'prefix is in scope but refers to something else.  Leave the ns attribute. \n"
"                                        a = Nothing \n"
"                                        Exit For \n"
"                                    End If \n"
"                                Next \n"
"                            End If \n"
" \n"
"                            If a IsNot Nothing Then \n"
"                                'Prefix is not in scope \n"
"                                'Now check whether it's going to be in scope because it is in the attributes list \n"
" \n"
"                                If attributes IsNot Nothing Then \n"
"                                    Dim lastIndex As Integer = attributes.Count - 1 \n"
"                                    For i As Integer = 0 To lastIndex \n"
"                                        Dim currentA As Global.System.Xml.Linq.XAttribute = attributes(i) \n"
"                                        Dim currentInScopePrefix As String = currentA.Name.LocalName \n"
"                                        Dim currentInScopeNs As Global.System.Xml.Linq.XNamespace = currentA.Annotation(Of Global.System.Xml.Linq.XNamespace)() \n"
"                                        If currentInScopeNs IsNot Nothing Then \n"
"                                            If prefix.Equals(currentInScopePrefix) Then \n"
"                                                If ns = currentInScopeNs Then \n"
"                                                    'prefix and namespace match.  Remove the unneeded ns attribute \n"
"                                                    a.Remove() \n"
"                                                End If \n"
" \n"
"                                                'prefix is in scope but refers to something else.  Leave the ns attribute. \n"
"                                                a = Nothing \n"
"                                                Exit For \n"
"                                            End If \n"
"                                        End If \n"
"                                    Next \n"
"                                End If \n"
" \n"
"                                If a IsNot Nothing Then \n"
"                                    'Prefix is definitely not in scope  \n"
"                                    a.Remove() \n"
"                                    'namespace is not defined either.  Add this attributes list \n"
"                                    attributes.Add(a) \n"
"                                End If \n"
"                            End If \n"
"                        End If \n"
"                    End If \n"
" \n"
"                    a = nextA \n"
"                End While \n"
"            End If \n"
"            Return e \n"
"        End Function \n"
" \n"
"    End Class \n"
"End Namespace \n";

    pCompilerHost->SetSolutionExtension(
        XmlHelperCode,
        DIM(XmlHelperCode) - 1,
        SolutionExtension::XmlHelperExtension);
}

void Compiler::LoadVBCoreSolutionExtension(CompilerHost *pCompilerHost)
{
    char VBCoreCode[] = (     
    "Option Strict On\n"
    "Option Infer On\n"
    "Option Explicit On\n"
    "Option Compare Binary\n"
    "<Assembly: Global.Microsoft.VisualBasic.Embedded()>\n"
    "Namespace Global.Microsoft.VisualBasic\n"
        "Namespace CompilerServices\n"
            "<Global.Microsoft.VisualBasic.Embedded()>\n"
            "<Global.System.Diagnostics.DebuggerNonUserCode(), Global.System.Runtime.CompilerServices.CompilerGenerated()>\n"
            "<Global.System.ComponentModel.EditorBrowsable(Global.System.ComponentModel.EditorBrowsableState.Never)>\n"
            "Friend Class EmbeddedOperators\n"
                "Private Sub New()\n"
                "End Sub\n"
                "Public Shared Function CompareString(Left As String, Right As String, TextCompare As Boolean) As Integer\n"
                    "If Left Is Right Then\n"
                        "Return 0\n"
                    "End If\n"
                    "If Left Is Nothing Then\n"
                        "If Right.Length() = 0 Then\n"
                            "Return 0\n"
                        "End If\n"
                        "Return -1\n"
                    "End If\n"
                    "If Right Is Nothing Then\n"
                        "If Left.Length() = 0 Then\n"
                            "Return 0\n"
                        "End If\n"
                        "Return 1\n"
                    "End If\n"
                    "Dim Result As Integer\n"
                    "If TextCompare Then\n"
                        "Dim OptionCompareTextFlags As Global.System.Globalization.CompareOptions = (Global.System.Globalization.CompareOptions.IgnoreCase Or Global.System.Globalization.CompareOptions.IgnoreWidth Or Global.System.Globalization.CompareOptions.IgnoreKanaType)\n"
                        "Result = Conversions.GetCultureInfo().CompareInfo.Compare(Left, Right, OptionCompareTextFlags)\n"
                    "Else\n"
                        "Result = Global.System.String.CompareOrdinal(Left, Right)\n"
                    "End If\n"
                    "If Result = 0 Then\n"
                        "Return 0\n"
                    "ElseIf Result > 0 Then\n"
                        "Return 1\n"
                    "Else\n"
                        "Return -1\n"
                    "End If\n"
                "End Function\n"
            "End Class\n"
            "<Global.Microsoft.VisualBasic.Embedded()>\n"
            "<Global.System.Diagnostics.DebuggerNonUserCode(), Global.System.Runtime.CompilerServices.CompilerGenerated()>\n"
            "<Global.System.ComponentModel.EditorBrowsable(Global.System.ComponentModel.EditorBrowsableState.Never)>\n"
            "Friend Class Conversions\n"
                "Private Sub New()\n"
                "End Sub\n"
                "Private Shared Function GetEnumValue(Value As Object) As Object\n"
                    "Dim underlyingType = System.Enum.GetUnderlyingType(Value.GetType())\n"
                    "If underlyingType.Equals(GetType(SByte)) Then\n"
                        "Return DirectCast(Value, SByte)\n"
                    "ElseIf underlyingType.Equals(GetType(Byte)) Then\n"
                        "Return DirectCast(Value, Byte)\n"
                    "ElseIf underlyingType.Equals(GetType(Global.System.Int16)) Then\n"
                        "Return DirectCast(Value, Global.System.Int16)\n"
                    "ElseIf underlyingType.Equals(GetType(Global.System.UInt16)) Then\n"
                        "Return DirectCast(Value, Global.System.UInt16)\n"
                    "ElseIf underlyingType.Equals(GetType(Global.System.Int32)) Then\n"
                        "Return DirectCast(Value, Global.System.Int32)\n"
                    "ElseIf underlyingType.Equals(GetType(Global.System.UInt32)) Then\n"
                        "Return DirectCast(Value, Global.System.UInt32)\n"
                    "ElseIf underlyingType.Equals(GetType(Global.System.Int64)) Then\n"
                        "Return DirectCast(Value, Global.System.Int64)\n"
                    "ElseIf underlyingType.Equals(GetType(Global.System.UInt64)) Then\n"
                        "Return DirectCast(Value, Global.System.UInt64)\n"
                    "Else\n"
                        "Throw New Global.System.InvalidCastException\n"
                    "End If\n"
                "End Function\n"
                "Public Shared Function ToBoolean(Value As String) As Boolean\n"
                    "If Value Is Nothing Then\n"
                        "Value = \"\"\n"
                    "End If\n"
                    "Try\n"
                        "Dim loc As Global.System.Globalization.CultureInfo = GetCultureInfo()\n"
                        "If loc.CompareInfo.Compare(Value, Boolean.FalseString, Global.System.Globalization.CompareOptions.IgnoreCase) = 0 Then\n"
                            "Return False\n"
                        "ElseIf loc.CompareInfo.Compare(Value, Boolean.TrueString, Global.System.Globalization.CompareOptions.IgnoreCase) = 0 Then\n"
                            "Return True\n"
                        "End If\n"
                        "Dim i64Value As Global.System.Int64\n"
                        "If IsHexOrOctValue(Value, i64Value) Then\n"
                            "Return CBool(i64Value)\n"
                        "End If\n"
                        "Return CBool(ParseDouble(Value))\n"
                    "Catch e As Global.System.FormatException\n"
                        "Throw New Global.System.InvalidCastException(e.Message, e)\n"
                    "End Try\n"
                "End Function\n"
                "Public Shared Function ToBoolean(Value As Object) As Boolean\n"
                    "If Value Is Nothing Then\n"
                        "Return False\n"
                    "End If\n"
                    "If TypeOf Value Is Global.System.Enum Then\n"
                        "Value = GetEnumValue(Value)\n"
                    "End If\n"
                    "If TypeOf Value Is Boolean Then\n"
                        "Return CBool(DirectCast(Value, Boolean))\n"
                    "ElseIf TypeOf Value Is SByte Then\n"
                        "Return CBool(DirectCast(Value, SByte))\n"
                    "ElseIf TypeOf Value Is Byte Then\n"
                        "Return CBool(DirectCast(Value, Byte))\n"
                    "ElseIf TypeOf Value Is Global.System.Int16 Then\n"
                        "Return CBool(DirectCast(Value, Global.System.Int16))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt16 Then\n"
                        "Return CBool(DirectCast(Value, Global.System.UInt16))\n"
                    "ElseIf TypeOf Value Is Global.System.Int32 Then\n"
                        "Return CBool(DirectCast(Value, Global.System.Int32))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt32 Then\n"
                        "Return CBool(DirectCast(Value, Global.System.UInt32))\n"
                    "ElseIf TypeOf Value Is Global.System.Int64 Then\n"
                        "Return CBool(DirectCast(Value, Global.System.Int64))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt64 Then\n"
                        "Return CBool(DirectCast(Value, Global.System.UInt64))\n"
                    "ElseIf TypeOf Value Is Decimal Then\n"
                        "Return CBool(DirectCast(Value, Global.System.Decimal))\n"
                    "ElseIf TypeOf Value Is Single Then\n"
                        "Return CBool(DirectCast(Value, Single))\n"
                    "ElseIf TypeOf Value Is Double Then\n"
                        "Return CBool(DirectCast(Value, Double))\n"
                    "ElseIf TypeOf Value Is String Then\n"
                        "Return CBool(DirectCast(Value, String))\n"
                    "End If\n"
                    "Throw New Global.System.InvalidCastException()\n"
                "End Function\n"
                "Public Shared Function ToByte(Value As String) As Byte\n"
                    "If Value Is Nothing Then\n"
                        "Return 0\n"
                    "End If\n"
                    "Try\n"
                        "Dim i64Value As Global.System.Int64\n"
                        "If IsHexOrOctValue(Value, i64Value) Then\n"
                            "Return CByte(i64Value)\n"
                        "End If\n"
                        "Return CByte(ParseDouble(Value))\n"
                    "Catch e As Global.System.FormatException\n"
                        "Throw New Global.System.InvalidCastException(e.Message, e)\n"
                    "End Try\n"
                "End Function\n"
                "Public Shared Function ToByte(Value As Object) As Byte\n"
                    "If Value Is Nothing Then\n"
                        "Return 0\n"
                    "End If\n"
                    "If TypeOf Value Is Global.System.Enum Then\n"
                        "Value = GetEnumValue(Value)\n"
                    "End If\n"
                    "If TypeOf Value Is Boolean Then\n"
                        "Return CByte(DirectCast(Value, Boolean))\n"
                    "ElseIf TypeOf Value Is SByte Then\n"
                        "Return CByte(DirectCast(Value, SByte))\n"
                    "ElseIf TypeOf Value Is Byte Then\n"
                        "Return CByte(DirectCast(Value, Byte))\n"
                    "ElseIf TypeOf Value Is Global.System.Int16 Then\n"
                        "Return CByte(DirectCast(Value, Global.System.Int16))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt16 Then\n"
                        "Return CByte(DirectCast(Value, Global.System.UInt16))\n"
                    "ElseIf TypeOf Value Is Global.System.Int32 Then\n"
                        "Return CByte(DirectCast(Value, Global.System.Int32))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt32 Then\n"
                        "Return CByte(DirectCast(Value, Global.System.UInt32))\n"
                    "ElseIf TypeOf Value Is Global.System.Int64 Then\n"
                        "Return CByte(DirectCast(Value, Global.System.Int64))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt64 Then\n"
                        "Return CByte(DirectCast(Value, Global.System.UInt64))\n"
                    "ElseIf TypeOf Value Is Decimal Then\n"
                        "Return CByte(DirectCast(Value, Global.System.Decimal))\n"
                    "ElseIf TypeOf Value Is Single Then\n"
                        "Return CByte(DirectCast(Value, Single))\n"
                    "ElseIf TypeOf Value Is Double Then\n"
                        "Return CByte(DirectCast(Value, Double))\n"
                    "ElseIf TypeOf Value Is String Then\n"
                        "Return CByte(DirectCast(Value, String))\n"
                    "End If\n"
                    "Throw New Global.System.InvalidCastException()\n"
                "End Function\n"
                "<Global.System.CLSCompliant(False)>\n"
                "Public Shared Function ToSByte(Value As String) As SByte\n"
                    "If Value Is Nothing Then\n"
                        "Return 0\n"
                    "End If\n"
                    "Try\n"
                        "Dim i64Value As Global.System.Int64\n"
                        "If IsHexOrOctValue(Value, i64Value) Then\n"
                            "Return CSByte(i64Value)\n"
                        "End If\n"
                        "Return CSByte(ParseDouble(Value))\n"
                    "Catch e As Global.System.FormatException\n"
                        "Throw New Global.System.InvalidCastException(e.Message, e)\n"
                    "End Try\n"
                "End Function\n"
                "<Global.System.CLSCompliant(False)>\n"
                "Public Shared Function ToSByte(Value As Object) As SByte\n"
                    "If Value Is Nothing Then\n"
                        "Return 0\n"
                    "End If\n"
                    "If TypeOf Value Is Global.System.Enum Then\n"
                        "Value = GetEnumValue(Value)\n"
                    "End If\n"
                    "If TypeOf Value Is Boolean Then\n"
                        "Return CSByte(DirectCast(Value, Boolean))\n"
                    "ElseIf TypeOf Value Is SByte Then\n"
                        "Return CSByte(DirectCast(Value, SByte))\n"
                    "ElseIf TypeOf Value Is Byte Then\n"
                        "Return CSByte(DirectCast(Value, Byte))\n"
                    "ElseIf TypeOf Value Is Global.System.Int16 Then\n"
                        "Return CSByte(DirectCast(Value, Global.System.Int16))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt16 Then\n"
                        "Return CSByte(DirectCast(Value, Global.System.UInt16))\n"
                    "ElseIf TypeOf Value Is Global.System.Int32 Then\n"
                        "Return CSByte(DirectCast(Value, Global.System.Int32))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt32 Then\n"
                        "Return CSByte(DirectCast(Value, Global.System.UInt32))\n"
                    "ElseIf TypeOf Value Is Global.System.Int64 Then\n"
                        "Return CSByte(DirectCast(Value, Global.System.Int64))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt64 Then\n"
                        "Return CSByte(DirectCast(Value, Global.System.UInt64))\n"
                    "ElseIf TypeOf Value Is Decimal Then\n"
                        "Return CSByte(DirectCast(Value, Global.System.Decimal))\n"
                    "ElseIf TypeOf Value Is Single Then\n"
                        "Return CSByte(DirectCast(Value, Single))\n"
                    "ElseIf TypeOf Value Is Double Then\n"
                        "Return CSByte(DirectCast(Value, Double))\n"
                    "ElseIf TypeOf Value Is String Then\n"
                        "Return CSByte(DirectCast(Value, String))\n"
                    "End If\n"
                    "Throw New Global.System.InvalidCastException()\n"
                "End Function\n"
                "Public Shared Function ToShort(Value As String) As Short\n"
                    "If Value Is Nothing Then\n"
                        "Return 0\n"
                    "End If\n"
                    "Try\n"
                        "Dim i64Value As Global.System.Int64\n"
                        "If IsHexOrOctValue(Value, i64Value) Then\n"
                            "Return CShort(i64Value)\n"
                        "End If\n"
                        "Return CShort(ParseDouble(Value))\n"
                    "Catch e As Global.System.FormatException\n"
                        "Throw New Global.System.InvalidCastException(e.Message, e)\n"
                    "End Try\n"
                "End Function\n"
                "Public Shared Function ToShort(Value As Object) As Short\n"
                    "If Value Is Nothing Then\n"
                        "Return 0\n"
                    "End If\n"
                    "If TypeOf Value Is Global.System.Enum Then\n"
                        "Value = GetEnumValue(Value)\n"
                    "End If\n"
                    "If TypeOf Value Is Boolean Then\n"
                        "Return CShort(DirectCast(Value, Boolean))\n"
                    "ElseIf TypeOf Value Is SByte Then\n"
                        "Return CShort(DirectCast(Value, SByte))\n"
                    "ElseIf TypeOf Value Is Byte Then\n"
                        "Return CShort(DirectCast(Value, Byte))\n"
                    "ElseIf TypeOf Value Is Global.System.Int16 Then\n"
                        "Return CShort(DirectCast(Value, Global.System.Int16))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt16 Then\n"
                        "Return CShort(DirectCast(Value, Global.System.UInt16))\n"
                    "ElseIf TypeOf Value Is Global.System.Int32 Then\n"
                        "Return CShort(DirectCast(Value, Global.System.Int32))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt32 Then\n"
                        "Return CShort(DirectCast(Value, Global.System.UInt32))\n"
                    "ElseIf TypeOf Value Is Global.System.Int64 Then\n"
                        "Return CShort(DirectCast(Value, Global.System.Int64))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt64 Then\n"
                        "Return CShort(DirectCast(Value, Global.System.UInt64))\n"
                    "ElseIf TypeOf Value Is Decimal Then\n"
                        "Return CShort(DirectCast(Value, Global.System.Decimal))\n"
                    "ElseIf TypeOf Value Is Single Then\n"
                        "Return CShort(DirectCast(Value, Single))\n"
                    "ElseIf TypeOf Value Is Double Then\n"
                        "Return CShort(DirectCast(Value, Double))\n"
                    "ElseIf TypeOf Value Is String Then\n"
                        "Return CShort(DirectCast(Value, String))\n"
                    "End If\n"
                    "Throw New Global.System.InvalidCastException()\n"
                "End Function\n"
                "<Global.System.CLSCompliant(False)>\n"
                "Public Shared Function ToUShort(Value As String) As UShort\n"
                    "If Value Is Nothing Then\n"
                        "Return 0\n"
                    "End If\n"
                    "Try\n"
                        "Dim i64Value As Global.System.Int64\n"
                        "If IsHexOrOctValue(Value, i64Value) Then\n"
                            "Return CUShort(i64Value)\n"
                        "End If\n"
                        "Return CUShort(ParseDouble(Value))\n"
                    "Catch e As Global.System.FormatException\n"
                        "Throw New Global.System.InvalidCastException(e.Message, e)\n"
                    "End Try\n"
                "End Function\n"
                "<Global.System.CLSCompliant(False)>\n"
                "Public Shared Function ToUShort(Value As Object) As UShort\n"
                    "If Value Is Nothing Then\n"
                        "Return 0\n"
                    "End If\n"
                    "If TypeOf Value Is Global.System.Enum Then\n"
                        "Value = GetEnumValue(Value)\n"
                    "End If\n"
                    "If TypeOf Value Is Boolean Then\n"
                        "Return CUShort(DirectCast(Value, Boolean))\n"
                    "ElseIf TypeOf Value Is SByte Then\n"
                        "Return CUShort(DirectCast(Value, SByte))\n"
                    "ElseIf TypeOf Value Is Byte Then\n"
                        "Return CUShort(DirectCast(Value, Byte))\n"
                    "ElseIf TypeOf Value Is Global.System.Int16 Then\n"
                        "Return CUShort(DirectCast(Value, Global.System.Int16))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt16 Then\n"
                        "Return CUShort(DirectCast(Value, Global.System.UInt16))\n"
                    "ElseIf TypeOf Value Is Global.System.Int32 Then\n"
                        "Return CUShort(DirectCast(Value, Global.System.Int32))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt32 Then\n"
                        "Return CUShort(DirectCast(Value, Global.System.UInt32))\n"
                    "ElseIf TypeOf Value Is Global.System.Int64 Then\n"
                        "Return CUShort(DirectCast(Value, Global.System.Int64))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt64 Then\n"
                        "Return CUShort(DirectCast(Value, Global.System.UInt64))\n"
                    "ElseIf TypeOf Value Is Decimal Then\n"
                        "Return CUShort(DirectCast(Value, Global.System.Decimal))\n"
                    "ElseIf TypeOf Value Is Single Then\n"
                        "Return CUShort(DirectCast(Value, Single))\n"
                    "ElseIf TypeOf Value Is Double Then\n"
                        "Return CUShort(DirectCast(Value, Double))\n"
                    "ElseIf TypeOf Value Is String Then\n"
                        "Return CUShort(DirectCast(Value, String))\n"
                    "End If\n"
                    "Throw New Global.System.InvalidCastException()\n"
                "End Function\n"
                "Public Shared Function ToInteger(Value As String) As Integer\n"
                    "If Value Is Nothing Then\n"
                        "Return 0\n"
                    "End If\n"
                    "Try\n"
                        "Dim i64Value As Global.System.Int64\n"
                        "If IsHexOrOctValue(Value, i64Value) Then\n"
                            "Return CInt(i64Value)\n"
                        "End If\n"
                        "Return CInt(ParseDouble(Value))\n"
                    "Catch e As Global.System.FormatException\n"
                        "Throw New Global.System.InvalidCastException(e.Message, e)\n"
                    "End Try\n"
                "End Function\n"
                "Public Shared Function ToInteger(Value As Object) As Integer\n"
                    "If Value Is Nothing Then\n"
                        "Return 0\n"
                    "End If\n"
                    "If TypeOf Value Is Global.System.Enum Then\n"
                        "Value = GetEnumValue(Value)\n"
                    "End If\n"
                    "If TypeOf Value Is Boolean Then\n"
                        "Return CInt(DirectCast(Value, Boolean))\n"
                    "ElseIf TypeOf Value Is SByte Then\n"
                        "Return CInt(DirectCast(Value, SByte))\n"
                    "ElseIf TypeOf Value Is Byte Then\n"
                        "Return CInt(DirectCast(Value, Byte))\n"
                    "ElseIf TypeOf Value Is Global.System.Int16 Then\n"
                        "Return CInt(DirectCast(Value, Global.System.Int16))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt16 Then\n"
                        "Return CInt(DirectCast(Value, Global.System.UInt16))\n"
                    "ElseIf TypeOf Value Is Global.System.Int32 Then\n"
                        "Return CInt(DirectCast(Value, Global.System.Int32))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt32 Then\n"
                        "Return CInt(DirectCast(Value, Global.System.UInt32))\n"
                    "ElseIf TypeOf Value Is Global.System.Int64 Then\n"
                        "Return CInt(DirectCast(Value, Global.System.Int64))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt64 Then\n"
                        "Return CInt(DirectCast(Value, Global.System.UInt64))\n"
                    "ElseIf TypeOf Value Is Decimal Then\n"
                        "Return CInt(DirectCast(Value, Global.System.Decimal))\n"
                    "ElseIf TypeOf Value Is Single Then\n"
                        "Return CInt(DirectCast(Value, Single))\n"
                    "ElseIf TypeOf Value Is Double Then\n"
                        "Return CInt(DirectCast(Value, Double))\n"
                    "ElseIf TypeOf Value Is String Then\n"
                        "Return CInt(DirectCast(Value, String))\n"
                    "End If\n"
                    "Throw New Global.System.InvalidCastException()\n"
                "End Function\n"
                "<Global.System.CLSCompliant(False)>\n"
                "Public Shared Function ToUInteger(Value As String) As UInteger\n"
                    "If Value Is Nothing Then\n"
                        "Return 0\n"
                    "End If\n"
                    "Try\n"
                        "Dim i64Value As Global.System.Int64\n"
                        "If IsHexOrOctValue(Value, i64Value) Then\n"
                            "Return CUInt(i64Value)\n"
                        "End If\n"
                        "Return CUInt(ParseDouble(Value))\n"
                    "Catch e As Global.System.FormatException\n"
                        "Throw New Global.System.InvalidCastException(e.Message, e)\n"
                    "End Try\n"
                "End Function\n"
                "<Global.System.CLSCompliant(False)>\n"
                "Public Shared Function ToUInteger(Value As Object) As UInteger\n"
                    "If Value Is Nothing Then\n"
                        "Return 0\n"
                    "End If\n"
                    "If TypeOf Value Is Global.System.Enum Then\n"
                        "Value = GetEnumValue(Value)\n"
                    "End If\n"
                    "If TypeOf Value Is Boolean Then\n"
                        "Return CUInt(DirectCast(Value, Boolean))\n"
                    "ElseIf TypeOf Value Is SByte Then\n"
                        "Return CUInt(DirectCast(Value, SByte))\n"
                    "ElseIf TypeOf Value Is Byte Then\n"
                        "Return CUInt(DirectCast(Value, Byte))\n"
                    "ElseIf TypeOf Value Is Global.System.Int16 Then\n"
                        "Return CUInt(DirectCast(Value, Global.System.Int16))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt16 Then\n"
                        "Return CUInt(DirectCast(Value, Global.System.UInt16))\n"
                    "ElseIf TypeOf Value Is Global.System.Int32 Then\n"
                        "Return CUInt(DirectCast(Value, Global.System.Int32))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt32 Then\n"
                        "Return CUInt(DirectCast(Value, Global.System.UInt32))\n"
                    "ElseIf TypeOf Value Is Global.System.Int64 Then\n"
                        "Return CUInt(DirectCast(Value, Global.System.Int64))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt64 Then\n"
                        "Return CUInt(DirectCast(Value, Global.System.UInt64))\n"
                    "ElseIf TypeOf Value Is Decimal Then\n"
                        "Return CUInt(DirectCast(Value, Global.System.Decimal))\n"
                    "ElseIf TypeOf Value Is Single Then\n"
                        "Return CUInt(DirectCast(Value, Single))\n"
                    "ElseIf TypeOf Value Is Double Then\n"
                        "Return CUInt(DirectCast(Value, Double))\n"
                    "ElseIf TypeOf Value Is String Then\n"
                        "Return CUInt(DirectCast(Value, String))\n"
                    "End If\n"
                    "Throw New Global.System.InvalidCastException()\n"
                "End Function\n"
                "Public Shared Function ToLong(Value As String) As Long\n"
                    "If (Value Is Nothing) Then\n"
                        "Return 0\n"
                    "End If\n"
                    "Try\n"
                        "Dim i64Value As Global.System.Int64\n"
                        "If IsHexOrOctValue(Value, i64Value) Then\n"
                            "Return CLng(i64Value)\n"
                        "End If\n"
                        "Return CLng(ParseDecimal(Value, Nothing))\n"
                    "Catch e As Global.System.FormatException\n"
                        "Throw New Global.System.InvalidCastException(e.Message, e)\n"
                    "End Try\n"
                "End Function\n"
                "Public Shared Function ToLong(Value As Object) As Long\n"
                    "If Value Is Nothing Then\n"
                        "Return 0\n"
                    "End If\n"
                    "If TypeOf Value Is Global.System.Enum Then\n"
                        "Value = GetEnumValue(Value)\n"
                    "End If\n"
                    "If TypeOf Value Is Boolean Then\n"
                        "Return CLng(DirectCast(Value, Boolean))\n"
                    "ElseIf TypeOf Value Is SByte Then\n"
                        "Return CLng(DirectCast(Value, SByte))\n"
                    "ElseIf TypeOf Value Is Byte Then\n"
                        "Return CLng(DirectCast(Value, Byte))\n"
                    "ElseIf TypeOf Value Is Global.System.Int16 Then\n"
                        "Return CLng(DirectCast(Value, Global.System.Int16))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt16 Then\n"
                        "Return CLng(DirectCast(Value, Global.System.UInt16))\n"
                    "ElseIf TypeOf Value Is Global.System.Int32 Then\n"
                        "Return CLng(DirectCast(Value, Global.System.Int32))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt32 Then\n"
                        "Return CLng(DirectCast(Value, Global.System.UInt32))\n"
                    "ElseIf TypeOf Value Is Global.System.Int64 Then\n"
                        "Return CLng(DirectCast(Value, Global.System.Int64))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt64 Then\n"
                        "Return CLng(DirectCast(Value, Global.System.UInt64))\n"
                    "ElseIf TypeOf Value Is Decimal Then\n"
                        "Return CLng(DirectCast(Value, Global.System.Decimal))\n"
                    "ElseIf TypeOf Value Is Single Then\n"
                        "Return CLng(DirectCast(Value, Single))\n"
                    "ElseIf TypeOf Value Is Double Then\n"
                        "Return CLng(DirectCast(Value, Double))\n"
                    "ElseIf TypeOf Value Is String Then\n"
                        "Return CLng(DirectCast(Value, String))\n"
                    "End If\n"
                    "Throw New Global.System.InvalidCastException()\n"
                "End Function\n"
                "<Global.System.CLSCompliant(False)>\n"
                "Public Shared Function ToULong(Value As String) As ULong\n"
                    "If (Value Is Nothing) Then\n"
                        "Return 0\n"
                    "End If\n"
                    "Try\n"
                        "Dim ui64Value As Global.System.UInt64\n"
                        "If IsHexOrOctValue(Value, ui64Value) Then\n"
                            "Return CULng(ui64Value)\n"
                        "End If\n"
                        "Return CULng(ParseDecimal(Value, Nothing))\n"
                    "Catch e As Global.System.FormatException\n"
                        "Throw New Global.System.InvalidCastException(e.Message, e)\n"
                    "End Try\n"
                "End Function\n"
                "<Global.System.CLSCompliant(False)>\n"
                "Public Shared Function ToULong(Value As Object) As ULong\n"
                    "If Value Is Nothing Then\n"
                        "Return 0\n"
                    "End If\n"
                    "If TypeOf Value Is Global.System.Enum Then\n"
                        "Value = GetEnumValue(Value)\n"
                    "End If\n"
                    "If TypeOf Value Is Boolean Then\n"
                        "Return CULng(DirectCast(Value, Boolean))\n"
                    "ElseIf TypeOf Value Is SByte Then\n"
                        "Return CULng(DirectCast(Value, SByte))\n"
                    "ElseIf TypeOf Value Is Byte Then\n"
                        "Return CULng(DirectCast(Value, Byte))\n"
                    "ElseIf TypeOf Value Is Global.System.Int16 Then\n"
                        "Return CULng(DirectCast(Value, Global.System.Int16))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt16 Then\n"
                        "Return CULng(DirectCast(Value, Global.System.UInt16))\n"
                    "ElseIf TypeOf Value Is Global.System.Int32 Then\n"
                        "Return CULng(DirectCast(Value, Global.System.Int32))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt32 Then\n"
                        "Return CULng(DirectCast(Value, Global.System.UInt32))\n"
                    "ElseIf TypeOf Value Is Global.System.Int64 Then\n"
                        "Return CULng(DirectCast(Value, Global.System.Int64))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt64 Then\n"
                        "Return CULng(DirectCast(Value, Global.System.UInt64))\n"
                    "ElseIf TypeOf Value Is Decimal Then\n"
                        "Return CULng(DirectCast(Value, Global.System.Decimal))\n"
                    "ElseIf TypeOf Value Is Single Then\n"
                        "Return CULng(DirectCast(Value, Single))\n"
                    "ElseIf TypeOf Value Is Double Then\n"
                        "Return CULng(DirectCast(Value, Double))\n"
                    "ElseIf TypeOf Value Is String Then\n"
                        "Return CULng(DirectCast(Value, String))\n"
                    "End If\n"
                    "Throw New Global.System.InvalidCastException()\n"
                "End Function\n"
                "Public Shared Function ToDecimal(Value As Boolean) As Decimal\n"
                    "If Value Then\n"
                        "Return -1D\n"
                    "Else\n"
                        "Return 0D\n"
                    "End If\n"
                "End Function\n"
                "Public Shared Function ToDecimal(Value As String) As Decimal\n"
                    "If Value Is Nothing Then\n"
                        "Return 0D\n"
                    "End If\n"
                    "Try\n"
                        "Dim i64Value As Global.System.Int64\n"
                        "If IsHexOrOctValue(Value, i64Value) Then\n"
                            "Return CDec(i64Value)\n"
                        "End If\n"
                        "Return ParseDecimal(Value, Nothing)\n"
                    "Catch e1 As Global.System.OverflowException\n"
                        "Throw e1\n"
                    "Catch e2 As Global.System.FormatException\n"
                        "Throw New Global.System.InvalidCastException(e2.Message, e2)\n"
                    "End Try\n"
                "End Function\n"
                "Public Shared Function ToDecimal(Value As Object) As Decimal\n"
                    "If Value Is Nothing Then\n"
                        "Return 0D\n"
                    "End If\n"
                    "If TypeOf Value Is Global.System.Enum Then\n"
                        "Value = GetEnumValue(Value)\n"
                    "End If\n"
                    "If TypeOf Value Is Boolean Then\n"
                        "Return CDec(DirectCast(Value, Boolean))\n"
                    "ElseIf TypeOf Value Is SByte Then\n"
                        "Return CDec(DirectCast(Value, SByte))\n"
                    "ElseIf TypeOf Value Is Byte Then\n"
                        "Return CDec(DirectCast(Value, Byte))\n"
                    "ElseIf TypeOf Value Is Global.System.Int16 Then\n"
                        "Return CDec(DirectCast(Value, Global.System.Int16))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt16 Then\n"
                        "Return CDec(DirectCast(Value, Global.System.UInt16))\n"
                    "ElseIf TypeOf Value Is Global.System.Int32 Then\n"
                        "Return CDec(DirectCast(Value, Global.System.Int32))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt32 Then\n"
                        "Return CDec(DirectCast(Value, Global.System.UInt32))\n"
                    "ElseIf TypeOf Value Is Global.System.Int64 Then\n"
                        "Return CDec(DirectCast(Value, Global.System.Int64))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt64 Then\n"
                        "Return CDec(DirectCast(Value, Global.System.UInt64))\n"
                    "ElseIf TypeOf Value Is Decimal Then\n"
                        "Return CDec(DirectCast(Value, Global.System.Decimal))\n"
                    "ElseIf TypeOf Value Is Single Then\n"
                        "Return CDec(DirectCast(Value, Single))\n"
                    "ElseIf TypeOf Value Is Double Then\n"
                        "Return CDec(DirectCast(Value, Double))\n"
                    "ElseIf TypeOf Value Is String Then\n"
                        "Return CDec(DirectCast(Value, String))\n"
                    "End If\n"
                    "Throw New Global.System.InvalidCastException()\n"
                "End Function\n"
                "Private Shared Function ParseDecimal(Value As String, NumberFormat As Global.System.Globalization.NumberFormatInfo) As Decimal\n"
                    "Dim NormalizedNumberFormat As Global.System.Globalization.NumberFormatInfo\n"
                    "Dim culture As Global.System.Globalization.CultureInfo = GetCultureInfo()\n"
                    "If NumberFormat Is Nothing Then\n"
                        "NumberFormat = culture.NumberFormat\n"
                    "End If\n"
                    "NormalizedNumberFormat = GetNormalizedNumberFormat(NumberFormat)\n"
                    "Const flags As Global.System.Globalization.NumberStyles =\n"
                            "Global.System.Globalization.NumberStyles.AllowDecimalPoint Or\n"
                            "Global.System.Globalization.NumberStyles.AllowExponent Or\n"
                            "Global.System.Globalization.NumberStyles.AllowLeadingSign Or\n"
                            "Global.System.Globalization.NumberStyles.AllowLeadingWhite Or\n"
                            "Global.System.Globalization.NumberStyles.AllowThousands Or\n"
                            "Global.System.Globalization.NumberStyles.AllowTrailingSign Or\n"
                            "Global.System.Globalization.NumberStyles.AllowParentheses Or\n"
                            "Global.System.Globalization.NumberStyles.AllowTrailingWhite Or\n"
                            "Global.System.Globalization.NumberStyles.AllowCurrencySymbol\n"
                    "Value = ToHalfwidthNumbers(Value, culture)\n"
                    "Try\n"
                        "Return Global.System.Decimal.Parse(Value, flags, NormalizedNumberFormat)\n"
                    "Catch FormatEx As Global.System.FormatException When Not (NumberFormat Is NormalizedNumberFormat)\n"
                        "Return Global.System.Decimal.Parse(Value, flags, NumberFormat)\n"
                    "Catch Ex As Global.System.Exception\n"
                        "Throw Ex\n"
                    "End Try\n"
                "End Function\n"
                "Private Shared Function GetNormalizedNumberFormat(InNumberFormat As Global.System.Globalization.NumberFormatInfo) As Global.System.Globalization.NumberFormatInfo\n"
                    "Dim OutNumberFormat As Global.System.Globalization.NumberFormatInfo\n"
                    "With InNumberFormat\n"
                        "If (Not .CurrencyDecimalSeparator Is Nothing) AndAlso\n"
                           "(Not .NumberDecimalSeparator Is Nothing) AndAlso\n"
                           "(Not .CurrencyGroupSeparator Is Nothing) AndAlso\n"
                           "(Not .NumberGroupSeparator Is Nothing) AndAlso\n"
                           "(.CurrencyDecimalSeparator.Length = 1) AndAlso\n"
                           "(.NumberDecimalSeparator.Length = 1) AndAlso\n"
                           "(.CurrencyGroupSeparator.Length = 1) AndAlso\n"
                           "(.NumberGroupSeparator.Length = 1) AndAlso\n"
                           "(.CurrencyDecimalSeparator.Chars(0) = .NumberDecimalSeparator.Chars(0)) AndAlso\n"
                           "(.CurrencyGroupSeparator.Chars(0) = .NumberGroupSeparator.Chars(0)) AndAlso\n"
                           "(.CurrencyDecimalDigits = .NumberDecimalDigits) Then\n"
                            "Return InNumberFormat\n"
                        "End If\n"
                    "End With\n"
                    "With InNumberFormat\n"
                        "If (Not .CurrencyDecimalSeparator Is Nothing) AndAlso\n"
                           "(Not .NumberDecimalSeparator Is Nothing) AndAlso\n"
                           "(.CurrencyDecimalSeparator.Length = .NumberDecimalSeparator.Length) AndAlso\n"
                           "(Not .CurrencyGroupSeparator Is Nothing) AndAlso\n"
                           "(Not .NumberGroupSeparator Is Nothing) AndAlso\n"
                           "(.CurrencyGroupSeparator.Length = .NumberGroupSeparator.Length) Then\n"
                            "Dim i As Integer\n"
                            "For i = 0 To .CurrencyDecimalSeparator.Length - 1\n"
                                "If (.CurrencyDecimalSeparator.Chars(i) <> .NumberDecimalSeparator.Chars(i)) Then GoTo MisMatch\n"
                            "Next\n"
                            "For i = 0 To .CurrencyGroupSeparator.Length - 1\n"
                                "If (.CurrencyGroupSeparator.Chars(i) <> .NumberGroupSeparator.Chars(i)) Then GoTo MisMatch\n"
                            "Next\n"
                            "Return InNumberFormat\n"
                        "End If\n"
                    "End With\n"
    "MisMatch:\n"
                    "OutNumberFormat = DirectCast(InNumberFormat.Clone, Global.System.Globalization.NumberFormatInfo)\n"
                    "With OutNumberFormat\n"
                        ".CurrencyDecimalSeparator = .NumberDecimalSeparator\n"
                        ".CurrencyGroupSeparator = .NumberGroupSeparator\n"
                        ".CurrencyDecimalDigits = .NumberDecimalDigits\n"
                    "End With\n"
                    "Return OutNumberFormat\n"
                "End Function\n"
                "Public Shared Function ToSingle(Value As String) As Single\n"
                    "If Value Is Nothing Then\n"
                        "Return 0\n"
                    "End If\n"
                    "Try\n"
                        "Dim i64Value As Global.System.Int64\n"
                        "If IsHexOrOctValue(Value, i64Value) Then\n"
                            "Return CSng(i64Value)\n"
                        "End If\n"
                        "Dim Result As Double = ParseDouble(Value)\n"
                        "If (Result < Global.System.Single.MinValue OrElse Result > Global.System.Single.MaxValue) AndAlso\n"
                           "Not Global.System.Double.IsInfinity(Result) Then\n"
                            "Throw New Global.System.OverflowException\n"
                        "End If\n"
                        "Return CSng(Result)\n"
                    "Catch e As Global.System.FormatException\n"
                        "Throw New Global.System.InvalidCastException(e.Message, e)\n"
                    "End Try\n"
                "End Function\n"
                "Public Shared Function ToSingle(Value As Object) As Single\n"
                    "If Value Is Nothing Then\n"
                        "Return 0\n"
                    "End If\n"
                    "If TypeOf Value Is Global.System.Enum Then\n"
                        "Value = GetEnumValue(Value)\n"
                    "End If\n"
                    "If TypeOf Value Is Boolean Then\n"
                        "Return CSng(DirectCast(Value, Boolean))\n"
                    "ElseIf TypeOf Value Is SByte Then\n"
                        "Return CSng(DirectCast(Value, SByte))\n"
                    "ElseIf TypeOf Value Is Byte Then\n"
                        "Return CSng(DirectCast(Value, Byte))\n"
                    "ElseIf TypeOf Value Is Global.System.Int16 Then\n"
                        "Return CSng(DirectCast(Value, Global.System.Int16))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt16 Then\n"
                        "Return CSng(DirectCast(Value, Global.System.UInt16))\n"
                    "ElseIf TypeOf Value Is Global.System.Int32 Then\n"
                        "Return CSng(DirectCast(Value, Global.System.Int32))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt32 Then\n"
                        "Return CSng(DirectCast(Value, Global.System.UInt32))\n"
                    "ElseIf TypeOf Value Is Global.System.Int64 Then\n"
                        "Return CSng(DirectCast(Value, Global.System.Int64))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt64 Then\n"
                        "Return CSng(DirectCast(Value, Global.System.UInt64))\n"
                    "ElseIf TypeOf Value Is Decimal Then\n"
                        "Return CSng(DirectCast(Value, Global.System.Decimal))\n"
                    "ElseIf TypeOf Value Is Single Then\n"
                        "Return CSng(DirectCast(Value, Single))\n"
                    "ElseIf TypeOf Value Is Double Then\n"
                        "Return CSng(DirectCast(Value, Double))\n"
                    "ElseIf TypeOf Value Is String Then\n"
                        "Return CSng(DirectCast(Value, String))\n"
                    "End If\n"
                    "Throw New Global.System.InvalidCastException()\n"
                "End Function\n"
                "Public Shared Function ToDouble(Value As String) As Double\n"
                    "If Value Is Nothing Then\n"
                        "Return 0\n"
                    "End If\n"
                    "Try\n"
                        "Dim i64Value As Global.System.Int64\n"
                        "If IsHexOrOctValue(Value, i64Value) Then\n"
                            "Return CDbl(i64Value)\n"
                        "End If\n"
                        "Return ParseDouble(Value)\n"
                    "Catch e As Global.System.FormatException\n"
                        "Throw New Global.System.InvalidCastException(e.Message, e)\n"
                    "End Try\n"
                "End Function\n"
                "Public Shared Function ToDouble(Value As Object) As Double\n"
                    "If Value Is Nothing Then\n"
                        "Return 0\n"
                    "End If\n"
                    "If TypeOf Value Is Global.System.Enum Then\n"
                        "Value = GetEnumValue(Value)\n"
                    "End If\n"
                    "If TypeOf Value Is Boolean Then\n"
                        "Return CDbl(DirectCast(Value, Boolean))\n"
                    "ElseIf TypeOf Value Is SByte Then\n"
                        "Return CDbl(DirectCast(Value, SByte))\n"
                    "ElseIf TypeOf Value Is Byte Then\n"
                        "Return CDbl(DirectCast(Value, Byte))\n"
                    "ElseIf TypeOf Value Is Global.System.Int16 Then\n"
                        "Return CDbl(DirectCast(Value, Global.System.Int16))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt16 Then\n"
                        "Return CDbl(DirectCast(Value, Global.System.UInt16))\n"
                    "ElseIf TypeOf Value Is Global.System.Int32 Then\n"
                        "Return CDbl(DirectCast(Value, Global.System.Int32))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt32 Then\n"
                        "Return CDbl(DirectCast(Value, Global.System.UInt32))\n"
                    "ElseIf TypeOf Value Is Global.System.Int64 Then\n"
                        "Return CDbl(DirectCast(Value, Global.System.Int64))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt64 Then\n"
                        "Return CDbl(DirectCast(Value, Global.System.UInt64))\n"
                    "ElseIf TypeOf Value Is Decimal Then\n"
                        "Return CDbl(DirectCast(Value, Global.System.Decimal))\n"
                    "ElseIf TypeOf Value Is Single Then\n"
                        "Return CDbl(DirectCast(Value, Single))\n"
                    "ElseIf TypeOf Value Is Double Then\n"
                        "Return CDbl(DirectCast(Value, Double))\n"
                    "ElseIf TypeOf Value Is String Then\n"
                        "Return CDbl(DirectCast(Value, String))\n"
                    "End If\n"
                    "Throw New Global.System.InvalidCastException()\n"
                "End Function\n"
                "Private Shared Function ParseDouble(Value As String) As Double\n"
                    "Dim NormalizedNumberFormat As Global.System.Globalization.NumberFormatInfo\n"
                    "Dim culture As Global.System.Globalization.CultureInfo = GetCultureInfo()\n"
                    "Dim NumberFormat As Global.System.Globalization.NumberFormatInfo = culture.NumberFormat\n"
                    "NormalizedNumberFormat = GetNormalizedNumberFormat(NumberFormat)\n"
                    "Const flags As Global.System.Globalization.NumberStyles =\n"
                            "Global.System.Globalization.NumberStyles.AllowDecimalPoint Or\n"
                            "Global.System.Globalization.NumberStyles.AllowExponent Or\n"
                            "Global.System.Globalization.NumberStyles.AllowLeadingSign Or\n"
                            "Global.System.Globalization.NumberStyles.AllowLeadingWhite Or\n"
                            "Global.System.Globalization.NumberStyles.AllowThousands Or\n"
                            "Global.System.Globalization.NumberStyles.AllowTrailingSign Or\n"
                            "Global.System.Globalization.NumberStyles.AllowParentheses Or\n"
                            "Global.System.Globalization.NumberStyles.AllowTrailingWhite Or\n"
                            "Global.System.Globalization.NumberStyles.AllowCurrencySymbol\n"
                    "Value = ToHalfwidthNumbers(Value, culture)\n"
                    "Try\n"
                        "Return Global.System.Double.Parse(Value, flags, NormalizedNumberFormat)\n"
                    "Catch FormatEx As Global.System.FormatException When Not (NumberFormat Is NormalizedNumberFormat)\n"
                        "Return Global.System.Double.Parse(Value, flags, NumberFormat)\n"
                    "Catch Ex As Global.System.Exception\n"
                        "Throw Ex\n"
                    "End Try\n"
                "End Function\n"
                "Public Shared Function ToDate(Value As String) As Date\n"
                    "Dim ParsedDate As Global.System.DateTime\n"
                    "Const ParseStyle As Global.System.Globalization.DateTimeStyles =\n"
                        "Global.System.Globalization.DateTimeStyles.AllowWhiteSpaces Or\n"
                        "Global.System.Globalization.DateTimeStyles.NoCurrentDateDefault\n"
                    "Dim Culture As Global.System.Globalization.CultureInfo = GetCultureInfo()\n"
                    "Dim result = Global.System.DateTime.TryParse(ToHalfwidthNumbers(Value, Culture), Culture, ParseStyle, ParsedDate)\n"
                    "If result Then\n"
                        "Return ParsedDate\n"
                    "Else\n"
                        "Throw New Global.System.InvalidCastException()\n"
                    "End If\n"
                "End Function\n"
                "Public Shared Function ToDate(Value As Object) As Date\n"
                    "If Value Is Nothing Then\n"
                        "Return Nothing\n"
                    "End If\n"
                    "If TypeOf Value Is Global.System.DateTime Then\n"
                        "Return CDate(DirectCast(Value, Global.System.DateTime))\n"
                    "ElseIf TypeOf Value Is String Then\n"
                        "Return CDate(DirectCast(Value, String))\n"
                    "End If\n"
                    "Throw New Global.System.InvalidCastException()\n"
                "End Function\n"
                "Public Shared Function ToChar(Value As String) As Char\n"
                    "If (Value Is Nothing) OrElse (Value.Length = 0) Then\n"
                        "Return Global.System.Convert.ToChar(0 And &HFFFFI)\n"
                    "End If\n"
                    "Return Value.Chars(0)\n"
                "End Function\n"
                "Public Shared Function ToChar(Value As Object) As Char\n"
                    "If Value Is Nothing Then\n"
                        "Return Global.System.Convert.ToChar(0 And &HFFFFI)\n"
                    "End If\n"
                    "If TypeOf Value Is Char Then\n"
                        "Return CChar(DirectCast(Value, Char))\n"
                    "ElseIf TypeOf Value Is String Then\n"
                        "Return CChar(DirectCast(Value, String))\n"
                    "End If\n"
                    "Throw New Global.System.InvalidCastException()\n"
                "End Function\n"
                "Public Shared Function ToCharArrayRankOne(Value As String) As Char()\n"
                    "If Value Is Nothing Then\n"
                        "Value = \"\"\n"
                    "End If\n"
                    "Return Value.ToCharArray()\n"
                "End Function\n"
                "Public Shared Function ToCharArrayRankOne(Value As Object) As Char()\n"
                    "If Value Is Nothing Then\n"
                        "Return \"\".ToCharArray()\n"
                    "End If\n"
                    "Dim ArrayValue As Char() = TryCast(Value, Char())\n"
                    "If ArrayValue IsNot Nothing AndAlso ArrayValue.Rank = 1 Then\n"
                        "Return ArrayValue\n"
                    "ElseIf TypeOf Value Is String Then\n"
                            "Return DirectCast(Value, String).ToCharArray()\n"
                    "End If\n"
                    "Throw New Global.System.InvalidCastException()\n"
                "End Function\n"
                "Public Shared Shadows Function ToString(Value As Short) As String\n"
                    "Return Value.ToString()\n"
                "End Function\n"
                "Public Shared Shadows Function ToString(Value As Integer) As String\n"
                    "Return Value.ToString()\n"
                "End Function\n"
                "<Global.System.CLSCompliant(False)>\n"
                "Public Shared Shadows Function ToString(Value As UInteger) As String\n"
                    "Return Value.ToString()\n"
                "End Function\n"
                "Public Shared Shadows Function ToString(Value As Long) As String\n"
                    "Return Value.ToString()\n"
                "End Function\n"
                "<Global.System.CLSCompliant(False)>\n"
                "Public Shared Shadows Function ToString(Value As ULong) As String\n"
                    "Return Value.ToString()\n"
                "End Function\n"
                "Public Shared Shadows Function ToString(Value As Single) As String\n"
                    "Return Value.ToString()\n"
                "End Function\n"
                "Public Shared Shadows Function ToString(Value As Double) As String\n"
                    "Return Value.ToString(\"G\")\n"
                "End Function\n"
                "Public Shared Shadows Function ToString(Value As Date) As String\n"
                    "Dim TimeTicks As Long = Value.TimeOfDay.Ticks\n"
                    "If (TimeTicks = Value.Ticks) OrElse\n"
                        "(Value.Year = 1899 AndAlso Value.Month = 12 AndAlso Value.Day = 30) Then\n"
                        "Return Value.ToString(\"T\")\n"
                    "ElseIf TimeTicks = 0 Then\n"
                        "Return Value.ToString(\"d\")\n"
                    "Else\n"
                        "Return Value.ToString(\"G\")\n"
                    "End If\n"
                "End Function\n"
                "Public Shared Shadows Function ToString(Value As Decimal) As String\n"
                    "Return Value.ToString(\"G\")\n"
                "End Function\n"
                "Public Shared Shadows Function ToString(Value As Object) As String\n"
                    "If Value Is Nothing Then\n"
                        "Return Nothing\n"
                    "Else\n"
                        "Dim StringValue As String = TryCast(Value, String)\n"
                        "If StringValue IsNot Nothing Then\n"
                            "Return StringValue\n"
                        "End If\n"
                    "End If\n"
                    "If TypeOf Value Is Global.System.Enum Then\n"
                        "Value = GetEnumValue(Value)\n"
                    "End If\n"
                    "If TypeOf Value Is Boolean Then\n"
                        "Return CStr(DirectCast(Value, Boolean))\n"
                    "ElseIf TypeOf Value Is SByte Then\n"
                        "Return CStr(DirectCast(Value, SByte))\n"
                    "ElseIf TypeOf Value Is Byte Then\n"
                        "Return CStr(DirectCast(Value, Byte))\n"
                    "ElseIf TypeOf Value Is Global.System.Int16 Then\n"
                        "Return CStr(DirectCast(Value, Global.System.Int16))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt16 Then\n"
                        "Return CStr(DirectCast(Value, Global.System.UInt16))\n"
                    "ElseIf TypeOf Value Is Global.System.Int32 Then\n"
                        "Return CStr(DirectCast(Value, Global.System.Int32))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt32 Then\n"
                        "Return CStr(DirectCast(Value, Global.System.UInt32))\n"
                    "ElseIf TypeOf Value Is Global.System.Int64 Then\n"
                        "Return CStr(DirectCast(Value, Global.System.Int64))\n"
                    "ElseIf TypeOf Value Is Global.System.UInt64 Then\n"
                        "Return CStr(DirectCast(Value, Global.System.UInt64))\n"
                    "ElseIf TypeOf Value Is Decimal Then\n"
                        "Return CStr(DirectCast(Value, Global.System.Decimal))\n"
                    "ElseIf TypeOf Value Is Single Then\n"
                        "Return CStr(DirectCast(Value, Single))\n"
                    "ElseIf TypeOf Value Is Double Then\n"
                        "Return CStr(DirectCast(Value, Double))\n"
                    "ElseIf TypeOf Value Is Char Then\n"
                        "Return CStr(DirectCast(Value, Char))\n"
                    "ElseIf TypeOf Value Is Date Then\n"
                        "Return CStr(DirectCast(Value, Date))\n"
                    "Else\n"
                        "Dim CharArray As Char() = TryCast(Value, Char())\n"
                        "If CharArray IsNot Nothing Then\n"
                            "Return New String(CharArray)\n"
                        "End If\n"
                    "End If\n"
                    "Throw New Global.System.InvalidCastException()\n"
                "End Function\n"
                "Public Shared Shadows Function ToString(Value As Boolean) As String\n"
                    "If Value Then\n"
                        "Return Global.System.Boolean.TrueString\n"
                    "Else\n"
                        "Return Global.System.Boolean.FalseString\n"
                    "End If\n"
                "End Function\n"
                "Public Shared Shadows Function ToString(Value As Byte) As String\n"
                    "Return Value.ToString()\n"
                "End Function\n"
                "Public Shared Shadows Function ToString(Value As Char) As String\n"
                    "Return Value.ToString()\n"
                "End Function\n"
                "Friend Shared Function GetCultureInfo() As Global.System.Globalization.CultureInfo\n"
                    "Return Global.System.Globalization.CultureInfo.CurrentCulture\n"
                "End Function\n"
                "Friend Shared Function ToHalfwidthNumbers(s As String, culture As Global.System.Globalization.CultureInfo) As String\n"
                    "Return s\n"
                "End Function\n"
                "Friend Shared Function IsHexOrOctValue(Value As String, ByRef i64Value As Global.System.Int64) As Boolean\n"
                    "Dim ch As Char\n"
                    "Dim Length As Integer\n"
                    "Dim FirstNonspace As Integer\n"
                    "Dim TmpValue As String\n"
                    "Length = Value.Length\n"
                    "Do While (FirstNonspace < Length)\n"
                        "ch = Value.Chars(FirstNonspace)\n"
                        "If ch = \"&\"c AndAlso FirstNonspace + 2 < Length Then\n"
                            "GoTo GetSpecialValue\n"
                        "End If\n"
                        "If ch <> Strings.ChrW(32) AndAlso ch <> Strings.ChrW(&H3000) Then\n"
                            "Return False\n"
                        "End If\n"
                        "FirstNonspace += 1\n"
                    "Loop\n"
                    "Return False\n"
    "GetSpecialValue:\n"
                    "ch = Global.System.Char.ToLowerInvariant(Value.Chars(FirstNonspace + 1))\n"
                    "TmpValue = ToHalfwidthNumbers(Value.Substring(FirstNonspace + 2), GetCultureInfo())\n"
                    "If ch = \"h\"c Then\n"
                        "i64Value = Global.System.Convert.ToInt64(TmpValue, 16)\n"
                    "ElseIf ch = \"o\"c Then\n"
                        "i64Value = Global.System.Convert.ToInt64(TmpValue, 8)\n"
                    "Else\n"
                        "Throw New Global.System.FormatException\n"
                    "End If\n"
                    "Return True\n"
                "End Function\n"
                "Friend Shared Function IsHexOrOctValue(Value As String, ByRef ui64Value As Global.System.UInt64) As Boolean\n"
                    "Dim ch As Char\n"
                    "Dim Length As Integer\n"
                    "Dim FirstNonspace As Integer\n"
                    "Dim TmpValue As String\n"
                    "Length = Value.Length\n"
                    "Do While (FirstNonspace < Length)\n"
                        "ch = Value.Chars(FirstNonspace)\n"
                        "If ch = \"&\"c AndAlso FirstNonspace + 2 < Length Then\n"
                            "GoTo GetSpecialValue\n"
                        "End If\n"
                        "If ch <> Strings.ChrW(32) AndAlso ch <> Strings.ChrW(&H3000) Then\n"
                            "Return False\n"
                        "End If\n"
                        "FirstNonspace += 1\n"
                    "Loop\n"
                    "Return False\n"
    "GetSpecialValue:\n"
                    "ch = Global.System.Char.ToLowerInvariant(Value.Chars(FirstNonspace + 1))\n"
                    "TmpValue = ToHalfwidthNumbers(Value.Substring(FirstNonspace + 2), GetCultureInfo())\n"
                    "If ch = \"h\"c Then\n"
                        "ui64Value = Global.System.Convert.ToUInt64(TmpValue, 16)\n"
                    "ElseIf ch = \"o\"c Then\n"
                        "ui64Value = Global.System.Convert.ToUInt64(TmpValue, 8)\n"
                    "Else\n"
                        "Throw New Global.System.FormatException\n"
                    "End If\n"
                    "Return True\n"
                "End Function\n"
                "Public Shared Function ToGenericParameter(Of T)(Value As Object) As T\n"
                    "If Value Is Nothing Then\n"
                        "Return Nothing\n"
                    "End If\n"
                    "Dim reflectedType As Global.System.Type = GetType(T)\n"
                    "If Global.System.Type.Equals(reflectedType, GetType(Global.System.Boolean)) Then\n"
                        "Return DirectCast(CObj(CBool(Value)), T)\n"
                    "ElseIf Global.System.Type.Equals(reflectedType, GetType(Global.System.SByte)) Then\n"
                        "Return DirectCast(CObj(CSByte(Value)), T)\n"
                    "ElseIf Global.System.Type.Equals(reflectedType, GetType(Global.System.Byte)) Then\n"
                        "Return DirectCast(CObj(CByte(Value)), T)\n"
                    "ElseIf Global.System.Type.Equals(reflectedType, GetType(Global.System.Int16)) Then\n"
                        "Return DirectCast(CObj(CShort(Value)), T)\n"
                    "ElseIf Global.System.Type.Equals(reflectedType, GetType(Global.System.UInt16)) Then\n"
                        "Return DirectCast(CObj(CUShort(Value)), T)\n"
                    "ElseIf Global.System.Type.Equals(reflectedType, GetType(Global.System.Int32)) Then\n"
                        "Return DirectCast(CObj(CInt(Value)), T)\n"
                    "ElseIf Global.System.Type.Equals(reflectedType, GetType(Global.System.UInt32)) Then\n"
                        "Return DirectCast(CObj(CUInt(Value)), T)\n"
                    "ElseIf Global.System.Type.Equals(reflectedType, GetType(Global.System.Int64)) Then\n"
                        "Return DirectCast(CObj(CLng(Value)), T)\n"
                    "ElseIf Global.System.Type.Equals(reflectedType, GetType(Global.System.UInt64)) Then\n"
                        "Return DirectCast(CObj(CULng(Value)), T)\n"
                    "ElseIf Global.System.Type.Equals(reflectedType, GetType(Global.System.Decimal)) Then\n"
                        "Return DirectCast(CObj(CDec(Value)), T)\n"
                    "ElseIf Global.System.Type.Equals(reflectedType, GetType(Global.System.Single)) Then\n"
                        "Return DirectCast(CObj(CSng(Value)), T)\n"
                    "ElseIf Global.System.Type.Equals(reflectedType, GetType(Global.System.Double)) Then\n"
                        "Return DirectCast(CObj(CDbl(Value)), T)\n"
                    "ElseIf Global.System.Type.Equals(reflectedType, GetType(Global.System.DateTime)) Then\n"
                        "Return DirectCast(CObj(CDate(Value)), T)\n"
                    "ElseIf Global.System.Type.Equals(reflectedType, GetType(Global.System.Char)) Then\n"
                        "Return DirectCast(CObj(CChar(Value)), T)\n"
                    "ElseIf Global.System.Type.Equals(reflectedType, GetType(Global.System.String)) Then\n"
                        "Return DirectCast(CObj(CStr(Value)), T)\n"
                    "Else\n"
                        "Return DirectCast(Value, T)\n"
                    "End If\n"
                "End Function\n"
            "End Class\n"
            "<Global.Microsoft.VisualBasic.Embedded()>\n"
            "<Global.System.Diagnostics.DebuggerNonUserCode(), Global.System.Runtime.CompilerServices.CompilerGenerated()>\n"
            "<Global.System.ComponentModel.EditorBrowsable(Global.System.ComponentModel.EditorBrowsableState.Never)>\n"
            "Friend Class ProjectData\n"
                "Private Sub New()\n"
                "End Sub\n"
                "Public Overloads Shared Sub SetProjectError(ex As Global.System.Exception)\n"
                "End Sub\n"
                "Public Overloads Shared Sub SetProjectError(ex As Global.System.Exception, lErl As Integer)\n"
                "End Sub\n"
                "Public Shared Sub ClearProjectError()\n"
                "End Sub\n"
            "End Class\n"
            "<Global.Microsoft.VisualBasic.Embedded()>\n"
            "<Global.System.Diagnostics.DebuggerNonUserCode(), Global.System.Runtime.CompilerServices.CompilerGenerated()>\n"
            "<Global.System.ComponentModel.EditorBrowsable(Global.System.ComponentModel.EditorBrowsableState.Never)>\n"
            "Friend Class Utils\n"
                "Private Sub New()\n"
                "End Sub\n"
                "Public Shared Function CopyArray(arySrc As Global.System.Array, aryDest As Global.System.Array) As Global.System.Array\n"
                    "If arySrc Is Nothing Then\n"
                        "Return aryDest\n"
                    "End If\n"
                    "Dim lLength As Integer\n"
                    "lLength = arySrc.Length\n"
                    "If lLength = 0 Then\n"
                        "Return aryDest\n"
                    "End If\n"
                    "If aryDest.Rank() <> arySrc.Rank() Then\n"
                        "Throw New Global.System.InvalidCastException()\n"
                    "End If\n"
                    "Dim iDim As Integer\n"
                    "For iDim = 0 To aryDest.Rank() - 2\n"
                        "If aryDest.GetUpperBound(iDim) <> arySrc.GetUpperBound(iDim) Then\n"
                            "Throw New Global.System.ArrayTypeMismatchException()\n"
                        "End If\n"
                    "Next iDim\n"
                    "If lLength > aryDest.Length Then\n"
                        "lLength = aryDest.Length\n"
                    "End If\n"
                    "If arySrc.Rank > 1 Then\n"
                        "Dim LastRank As Integer = arySrc.Rank\n"
                        "Dim lenSrcLastRank As Integer = arySrc.GetLength(LastRank - 1)\n"
                        "Dim lenDestLastRank As Integer = aryDest.GetLength(LastRank - 1)\n"
                        "If lenDestLastRank = 0 Then\n"
                            "Return aryDest\n"
                        "End If\n"
                        "Dim lenCopy As Integer = If(lenSrcLastRank > lenDestLastRank, lenDestLastRank, lenSrcLastRank)\n"
                        "Dim i As Integer\n"
                        "For i = 0 To (arySrc.Length \\ lenSrcLastRank) - 1\n"
                            "Global.System.Array.Copy(arySrc, i * lenSrcLastRank, aryDest, i * lenDestLastRank, lenCopy)\n"
                        "Next i\n"
                    "Else\n"
                        "Global.System.Array.Copy(arySrc, aryDest, lLength)\n"
                    "End If\n"
                    "Return aryDest\n"
                "End Function\n"
            "End Class\n"
            "<Global.Microsoft.VisualBasic.Embedded()>\n"
            "<Global.System.Diagnostics.DebuggerNonUserCode(), Global.System.Runtime.CompilerServices.CompilerGenerated()>\n"
            "<Global.System.ComponentModel.EditorBrowsable(Global.System.ComponentModel.EditorBrowsableState.Never)>\n"
            "Friend Class ObjectFlowControl\n"
                "Friend Class ForLoopControl\n"
                    "Public Shared Function ForNextCheckR4(count As Single, limit As Single, StepValue As Single) As Boolean\n"
                        "If StepValue >= 0 Then\n"
                            "Return count <= limit\n"
                        "Else\n"
                            "Return count >= limit\n"
                        "End If\n"
                    "End Function\n"
                    "Public Shared Function ForNextCheckR8(count As Double, limit As Double, StepValue As Double) As Boolean\n"
                        "If StepValue >= 0 Then\n"
                            "Return count <= limit\n"
                        "Else\n"
                            "Return count >= limit\n"
                        "End If\n"
                    "End Function\n"
                    "Public Shared Function ForNextCheckDec(count As Decimal, limit As Decimal, StepValue As Decimal) As Boolean\n"
                        "If StepValue >= 0 Then\n"
                            "Return count <= limit\n"
                        "Else\n"
                            "Return count >= limit\n"
                        "End If\n"
                    "End Function\n"
                "End Class\n"
            "End Class\n"
            "<Global.Microsoft.VisualBasic.Embedded()>\n"
            "<Global.System.Diagnostics.DebuggerNonUserCode(), Global.System.Runtime.CompilerServices.CompilerGenerated()>\n"
            "<Global.System.ComponentModel.EditorBrowsable(Global.System.ComponentModel.EditorBrowsableState.Never)>\n"
            "Friend Class StaticLocalInitFlag\n"
                "Public State As Short\n"
            "End Class\n"
            "<Global.Microsoft.VisualBasic.Embedded()>\n"
            "<Global.System.Diagnostics.DebuggerNonUserCode(), Global.System.Runtime.CompilerServices.CompilerGenerated()>\n"
            "<Global.System.ComponentModel.EditorBrowsable(Global.System.ComponentModel.EditorBrowsableState.Never)>\n"
            "Friend Class IncompleteInitialization\n"
                "Inherits Global.System.Exception\n"
                "Public Sub New()\n"
                    "MyBase.New()\n"
                "End Sub\n"
            "End Class\n"
            "<Global.Microsoft.VisualBasic.Embedded()>\n"
            "<Global.System.AttributeUsage(Global.System.AttributeTargets.Class, Inherited:=False)>\n"
            "<Global.System.ComponentModel.EditorBrowsable(Global.System.ComponentModel.EditorBrowsableState.Never)>\n"
            "<Global.System.Runtime.CompilerServices.CompilerGenerated()>\n"
            "Friend Class StandardModuleAttribute\n"
                "Inherits Global.System.Attribute\n"
            "End Class\n"
            "<Global.Microsoft.VisualBasic.Embedded()>\n"
            "<Global.System.AttributeUsage(Global.System.AttributeTargets.Class, Inherited:=False)>\n"
            "<Global.System.ComponentModel.EditorBrowsable(Global.System.ComponentModel.EditorBrowsableState.Never)>\n"
            "<Global.System.Runtime.CompilerServices.CompilerGenerated()>\n"
            "Friend Class DesignerGeneratedAttribute\n"
                "Inherits Global.System.Attribute\n"
            "End Class\n"
            "<Global.Microsoft.VisualBasic.Embedded()>\n"
            "<Global.System.AttributeUsage(Global.System.AttributeTargets.Parameter, Inherited:=False)>\n"
            "<Global.System.ComponentModel.EditorBrowsable(Global.System.ComponentModel.EditorBrowsableState.Never)>\n"
            "<Global.System.Runtime.CompilerServices.CompilerGenerated()>\n"
            "Friend Class OptionCompareAttribute\n"
                "Inherits Global.System.Attribute\n"
            "End Class\n"
            "<Global.Microsoft.VisualBasic.Embedded()>\n"
            "<Global.System.AttributeUsage(Global.System.AttributeTargets.Class, Inherited:=False)>\n"
            "<Global.System.ComponentModel.EditorBrowsable(Global.System.ComponentModel.EditorBrowsableState.Never)>\n"
            "<Global.System.Runtime.CompilerServices.CompilerGenerated()>\n"
            "Friend Class OptionTextAttribute\n"
                "Inherits Global.System.Attribute\n"
            "End Class\n"
        "End Namespace\n"
        "<Global.Microsoft.VisualBasic.Embedded()>\n"
        "<Global.System.AttributeUsage(Global.System.AttributeTargets.Class Or System.AttributeTargets.Module Or System.AttributeTargets.Assembly, Inherited:=False)>\n"
        "<Global.System.ComponentModel.EditorBrowsable(Global.System.ComponentModel.EditorBrowsableState.Never)>\n"
        "<Global.System.Runtime.CompilerServices.CompilerGenerated()>\n"
        "Friend Class Embedded\n"
            "Inherits Global.System.Attribute\n"
        "End Class\n"
        "<Global.Microsoft.VisualBasic.Embedded()>\n"
        "<Global.System.AttributeUsage(Global.System.AttributeTargets.Class, Inherited:=False)>\n"
        "<Global.System.ComponentModel.EditorBrowsable(Global.System.ComponentModel.EditorBrowsableState.Never)>\n"
        "<Global.System.Runtime.CompilerServices.CompilerGenerated()>\n"
        "Friend Class HideModuleNameAttribute\n"
            "Inherits Global.System.Attribute\n"
        "End Class\n"
        "<Global.Microsoft.VisualBasic.Embedded(), Global.System.Diagnostics.DebuggerNonUserCode()>\n"
        "<Global.System.Runtime.CompilerServices.CompilerGenerated()>\n"
        "Friend Module Strings\n"
            "Public Function ChrW(CharCode As Integer) As Char\n"
                "If CharCode < -32768 OrElse CharCode > 65535 Then\n"
                    "Throw New Global.System.ArgumentException()\n"
                "End If\n"
                "Return Global.System.Convert.ToChar(CharCode And &HFFFFI)\n"
            "End Function\n"
            "Public Function AscW([String] As String) As Integer\n"
                "If ([String] Is Nothing) OrElse ([String].Length = 0) Then\n"
                    "Throw New Global.System.ArgumentException()\n"
                "End If\n"
                "Return AscW([String].Chars(0))\n"
            "End Function\n"
            "Public Function AscW([String] As Char) As Integer\n"
                "Return AscW([String])\n"
            "End Function\n"
        "End Module\n"
        "<Global.Microsoft.VisualBasic.Embedded(), Global.System.Diagnostics.DebuggerNonUserCode()>\n"
        "<Global.System.Runtime.CompilerServices.CompilerGenerated()>\n"        
        "Friend Module Constants\n"
            "Public Const vbCrLf As String = ChrW(13) & ChrW(10)\n"
            "Public Const vbNewLine As String = ChrW(13) & ChrW(10)\n"
            "Public Const vbCr As String = ChrW(13)\n"
            "Public Const vbLf As String = ChrW(10)\n"
            "Public Const vbBack As String = ChrW(8)\n"
            "Public Const vbFormFeed As String = ChrW(12)\n"
            "Public Const vbTab As String = ChrW(9)\n"
            "Public Const vbVerticalTab As String = ChrW(11)\n"
            "Public Const vbNullChar As String = ChrW(0)\n"
            "Public Const vbNullString As String = Nothing\n"
        "End Module\n"
    "End Namespace\n"
    );
       
    pCompilerHost->SetSolutionExtension(
        VBCoreCode,
        DIM(VBCoreCode) - 1,
        SolutionExtension::VBCoreExtension);

}

//****************************************************************************
// Implementation.
//****************************************************************************

//============================================================================
// Commit all changes to the metadata.  This is overrideen by CompilerPackage.
//============================================================================

HRESULT Compiler::CommitChanges
(
    void *pImage,
    CompilerProject *pCompilerProject
)
{
    return NOERROR;
}

//============================================================================
// This function is needed to create a "default" CompilerHost on the fly. This
// is needed in some IDE cases where we have no idea what host we are targeting.
// An example if this is "devenv foo.vb", where the parser needs to know about
// Com sybmols, but we don't know what platform we are compiling for.
//============================================================================
CompilerHost *Compiler::GetDefaultCompilerHost()
{
    // If there are any CompilerHosts, then return the first one found, otherwise,
    // create a new one and return it.
    CompilerHost *pDefaultCompilerHost = m_CompilerHostsList.GetFirst();

    if (!pDefaultCompilerHost)
    {
        if (FAILED(CreateNewCompilerHost(NULL)))
        {
            VSFAIL("Could not create new compiler host.");
            return NULL;
        }

        pDefaultCompilerHost = m_CompilerHostsList.GetFirst();
        VSASSERT(pDefaultCompilerHost, "Default CompilerHost cannot be NULL.");

    }
#if IDE 
    else
    {
        if (GetCompilerSharedState()->IsInMainThread())
        {
            // Since we don't know who is going to be using this host, we need to make
            // sure it's tables are loaded before we return.
            pDefaultCompilerHost->TryBindCoreLibraries();
        }
    }
#endif IDE

    VSASSERT(pDefaultCompilerHost->GetFXSymbolProvider()->GetType(FX::ObjectType), "We should be able to get COM symbols here.");

    return pDefaultCompilerHost;
}

//============================================================================
// Create a CompilerProject that manages metadata.
//============================================================================

HRESULT Compiler::CreateProjectWithMetaData
(
    _In_z_ STRING *pstrFileName,
    bool    bAssembly,
    CompilerHost *pCompilerHost,
    bool   *pbCreated,
    bool    bDefaultLibrary,
    CompilerProject **ppCreatedCompilerProject
)
{
    CompilerProject *pProject = NULL;
    bool bCreated = false;
    HRESULT hr = S_OK;

    // Does this already exist?
#if IDE 
    pProject = pCompilerHost->FindProject(pstrFileName, true /* fMetaDataProject */, NULL);
#else
    pProject = pCompilerHost->FindProject(pstrFileName, true /* fMetaDataProject */);
#endif IDE

    if (!pProject)
    {
        pProject = new (zeromemory) CompilerProject(this);

        {
            pCompilerHost->InsertProject(pProject);
            pProject->InitWithMetaData(pstrFileName, bAssembly, pCompilerHost, bDefaultLibrary);

 #if !IDE
            // Check to see if a similar project exists (one with the same assembly name and PK token)
            CompilerProject *pSimilarProject = pCompilerHost->FindProjectWithSameNamePKTokenAndVersion(pProject);

            if (pSimilarProject)
            {
                // Generate fatal error and bale out.
                hr = HrMakeRepl(ERRID_DuplicateReference2, pProject->GetAssemblyName(), pstrFileName);
                bCreated = false;
            }
            else
#endif !IDE
            {
                OnAssembly( CLXN_Add, pProject );
                bCreated = true;
            }
        }        
    }
    else
    {
        pProject->AddRef();
    }

    // Optional out parameter
    if (pbCreated)
    {
        *pbCreated = bCreated;
    }

    if (ppCreatedCompilerProject)
    {
        *ppCreatedCompilerProject = pProject;
    }
    return hr;
}

#if HOSTED
//============================================================================
// Creates a CompilerProject for Type Scope.
//============================================================================

HRESULT Compiler::CreateTypeScopeProject
(
    _In_z_ STRING *pstrFileName,
    CompilerHost *pCompilerHost,
    bool   *pbCreated,
    CompilerProject **ppCreatedCompilerProject
)
{
    CompilerProject *pProject = NULL;
    bool bCreated = false;
    HRESULT hr = S_OK;

    pProject = pCompilerHost->FindProject(pstrFileName, true /* fMetaDataProject */);

    if (!pProject)
    {
        pProject = new (zeromemory) CompilerProject(this);

        {
            pCompilerHost->InsertProject(pProject);
            pProject->InitForTypeScope(pCompilerHost);

            // Check to see if a similar project exists (one with the same assembly name and PK token)
            CompilerProject *pSimilarProject = pCompilerHost->FindProjectWithSameNamePKTokenAndVersion(pProject);

            if (pSimilarProject)
            {
                // Generate fatal error and bale out.
                hr = HrMakeRepl(ERRID_DuplicateReference2, pProject->GetAssemblyName(), pstrFileName);
                bCreated = false;
            }
            else
            {
                OnAssembly( CLXN_Add, pProject );
                bCreated = true;
            }
        }
    }
    else
    {
        pProject->AddRef();
    }

    // Optional out parameter
    if (pbCreated)
    {
        *pbCreated = bCreated;
    }

    if (ppCreatedCompilerProject)
    {
        *ppCreatedCompilerProject = pProject;
    }
    return hr;
}

#endif

STRING *Compiler::ConcatStringsArray(unsigned count,
                    _In_count_(count) const WCHAR **wsz,
                    _In_z_ const WCHAR *wszSeparator)
{
    return m_pStringPool->ConcatStringsArray(count, wsz, wszSeparator);
}

STRING *Compiler::ConcatStrings
(
    _In_z_ const WCHAR *wsz1,
    _In_z_ const WCHAR *wsz2,
    _In_opt_z_ const WCHAR *wsz3,
    _In_opt_z_ const WCHAR *wsz4,
    _In_opt_z_ const WCHAR *wsz5,
    _In_opt_z_ const WCHAR *wsz6
)
{
    return m_pStringPool->ConcatStrings(wsz1, wsz2, wsz3, wsz4, wsz5, wsz6);
}

unsigned Compiler::CountQualifiedNames(_In_z_ const WCHAR *pstrName)
{
    VSASSERT(pstrName, "Bad param!");

    // The number of names is the number of dots plus 1.
    unsigned cNames = 0;

    while(*pstrName)
    {
        if(*pstrName == L'.')
            cNames++;

        pstrName++;
    }

    cNames++;

    return cNames;
}

void Compiler::SplitQualifiedName(_In_z_ const WCHAR *pstrName, unsigned cNames, _Out_cap_(cNames) STRING **rgpstrNames)
{
    VSASSERT(pstrName && rgpstrNames, "Bad param!");
    const WCHAR *pstr1 = pstrName;
    const WCHAR *pstr2 = pstrName;
    unsigned i = 0;

    while(*pstr1)
    {
        if(*pstr1 == L'.')
        {
            if (i < cNames)
            {
                rgpstrNames[i++] = AddStringWithLen(pstr2, pstr1-pstr2);
                pstr2 = pstr1 + 1;
            }
            else
            {
                break;
            }
        }

        pstr1++;
    }

    if (i < cNames && *pstr1 == 0)
    {
        rgpstrNames[i] = AddStringWithLen(pstr2, pstr1-pstr2);
    }
    else
    {
        VSFAIL("The number of strings allocated is incorrect!");
    }
}

//============================================================================
// Is this a call to simple library routine
//============================================================================

bool Compiler::IsRuntimeFunctionCall
(
    ILTree::PILNode ptree      // tree to check for VBA library call
)
{
    BCSYM_NamedRoot *pnamed;
    STRING *pstrName;
    int i;

    if (ptree->bilop == SX_CALL && ptree->AsCallExpression().ptreeThis == NULL)
    {
        // it's a static call; check for qualifier
        VSASSERT(ptree->AsCallExpression().Left->bilop == SX_SYM, "");
        if (ptree->AsCallExpression().Left->AsSymbolReferenceExpression().ptreeQual == NULL)
        {
            // no qualifier; check containing class to see if it's VBA
            pnamed = ptree->AsCallExpression().Left->AsSymbolReferenceExpression().pnamed->DigThroughAlias()->PNamedRoot();

            // Hack for building the runtime --
            // If we're not under the EE (GetField() == NULL)
            // And if there is no VB runtime,
            // And the function is in a non-metadata project, then match it.
            CompilerProject* pContainingProject = pnamed->GetContainingProject();
            CompilerFile* pCompilerFile = pnamed->GetContainingCompilerFile();
            if (pContainingProject->IsDefaultVBRuntime()
                || 
                (pContainingProject->GetVBRuntimeKind() == EmbeddedRuntime && 
                pCompilerFile &&
                pCompilerFile->GetSolutionExtension() &&
                pCompilerFile->GetSolutionExtension()->m_extension == SolutionExtension::VBCoreExtension)
                || 
                (pContainingProject->GetVBRuntimeKind() != EmbeddedRuntime &&
                pContainingProject->GetCompilerHost()->GetDefaultVBRuntimeProject() == NULL &&
                pContainingProject->GetVBRuntimeProject() == NULL &&
                !pContainingProject->IsMetaData())
                || 
                (GetProjectBeingCompiled() != NULL &&
                GetProjectBeingCompiled()->GetVBRuntimeKind() == SpecifiedRuntime &&
                GetProjectBeingCompiled()->GetVBRuntimeProject() != NULL
                ))

            {
                // VB Lib; check name
                pstrName = pnamed->GetName();

                // 

                for (i = 0; i < RuntimeFunctionMax; i += 1)
                {
                    if (StringPool::IsEqual(pstrName, m_pstrRuntimeFunction[i]))
                    {
                        VSASSERT(GetProjectBeingCompiled() == NULL || // GetProjectBeingCompiled may be null in hosted compiler
                            !(GetProjectBeingCompiled()->GetVBRuntimeKind() == SpecifiedRuntime &&
                            GetProjectBeingCompiled()->GetVBRuntimeProject() != NULL) ||
                            GetProjectBeingCompiled()->GetVBRuntimeProject() == pContainingProject, "pContainingProject should be the VBRuntime");
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

//============================================================================
// Is this a call to simple library routine
//============================================================================

RuntimeFunction Compiler::WhichRuntimeFunctionCall
(
    ILTree::PILNode ptree      // tree to check for VBA library call
)
{
    VSASSERT(ptree->bilop == SX_CALL && ptree->AsCallExpression().ptreeThis == NULL, "");
    VSASSERT(ptree->AsCallExpression().Left->bilop == SX_SYM, "");
    VSASSERT(ptree->AsCallExpression().Left->AsSymbolReferenceExpression().ptreeQual == NULL, "");

    BCSYM_NamedRoot *pnamed = ptree->AsCallExpression().Left->AsSymbolReferenceExpression().pnamed->DigThroughAlias()->PNamedRoot();
    STRING *pstrName = pnamed->GetName();

    for (int i = 0; i < RuntimeFunctionMax; i += 1)
    {
        if (StringPool::IsEqual(pstrName, m_pstrRuntimeFunction[i]))
            return (RuntimeFunction)i;
    }

    VSFAIL("should find one");
    return (RuntimeFunction)0;
}

//****************************************************************************
// Memory profiling
//****************************************************************************

#if FV_TRACK_MEMORY && IDE

const CHAR  *SymbolName[] =
{
    #define DEF_BCSYM(x, y)  y,
    #include "Symbols\TypeTables.h"
    #undef DEF_BCSYM
};

const CHAR * AttributeName[] =
{
    #define DEF_ATTR_TYPE(name) #name,
    #include "AttributeTypes.h"
    #undef DEF_ATTR_TYPE
};


void Compiler::DumpValues
(
    const char ** name,
    const unsigned long * values,
    const unsigned long count
)
{
    for (int i = 0; i < count; ++i)
    {
        DebPrintf("%-35s, %15u\n", name[i],values[i]);
    }
}


void
Compiler::DumpSolutionMemoryUse
(
)
{
    m_instrumentation.Lock();

    DebPrintf("Dumping solution memory use at end of compilation.\n\n");
    DebPrintf("SizeOf Symbols\n");
    DebPrintf("--------------------------\n");
    const unsigned long * symbolSizeOf = m_instrumentation.GetSymbolSizeOf();
    DumpValues((const char **)SymbolName, symbolSizeOf, SYM_Max);
    DebPrintf("--------------------------\n\n\n");

    DebPrintf("Symbol Counts\n");
    DebPrintf("--------------------------\n");
    const unsigned long * symbolCounts = m_instrumentation.GetSymbolCounts();
    DumpValues((const char **)SymbolName, symbolCounts, SYM_Max);
    DebPrintf("--------------------------\n\n\n");

    DebPrintf("Symbol Memory Usage\n");
    DebPrintf("--------------------------\n");
    const unsigned long * symbolSizes = m_instrumentation.GetSymbolSizes();
    DumpValues((const char **)SymbolName, symbolSizes, SYM_Max);
    DebPrintf("--------------------------\n\n\n");

    DebPrintf("Attribute Usage\n");
    DebPrintf("Total Number of Attribute Structures: %15u\n", m_instrumentation.GetTotalAttributeAllocations());
    DebPrintf("--------------------------\n");
    const unsigned long * attrCounts = m_instrumentation.GetAttributeCounts();
    DumpValues((const char ** )AttributeName, attrCounts, AttrKindSimple_Max);
    DebPrintf("--------------------------\n");

    m_instrumentation.UnLock();
}

#endif

//****************************************************************************
// Private implementation.
//****************************************************************************

//============================================================================
// Helper to find a file by searching the given semicolon-separated search path.
// We don't use the SearchPath winapi for this function because SearchPath will find files in locations we don't want searched
//============================================================================
bool Compiler::FindFileInPath
(
    _In_z_ const WCHAR *wszFileName,     // IN:  File to search for
    _In_opt_z_ const WCHAR *wszSearchPath,   // IN:  Path to search
    _Deref_out_z_ STRING **ppstrFullPath  // OUT: Full Path Name
)
{
    HANDLE hSearch = INVALID_HANDLE_VALUE;
    bool bFound = false;

    // Initialize Out Parameter
    *ppstrFullPath = NULL;

    // Verify wszSearchPath isn't NULL or empty
    if ((wszSearchPath != NULL) && (*wszSearchPath != L'\0'))
    {
        WCHAR *wszCurrentDir;
        WCHAR wszSeparator[] = L";";

        // Microsoft 9/18/2004:  The allocation math here just produces the same size as the original string
        // (including NULL terminator) and so does not need to be checked...
        TEMPBUF(wszSearchPathCopy, WCHAR *, (wcslen(wszSearchPath)+1) * sizeof(WCHAR));
        IfNullThrow(wszSearchPathCopy);
#pragma prefast(suppress: 6387, "wszSearchPathCopy is never NULL")
        wcscpy_s(wszSearchPathCopy, wcslen(wszSearchPath)+1, wszSearchPath);

        WCHAR *tokContext = NULL;
        wszCurrentDir = wcstok_s(wszSearchPathCopy, wszSeparator, &tokContext);

        // Try looking for wszFileName in each directory in wszSearchPath
        while ((wszCurrentDir != NULL) && (*wszCurrentDir != L'\0') && (!bFound))
        {
            // Microsoft 9/18/2004:  ... but this allocation math is a bit more dodgy and should be
            // verified for overflow.
            size_t nAcc = wcslen(wszCurrentDir)+1; // Fine; original size plus its NULL terminator...
            IfFalseThrow((nAcc += wcslen(wszFileName)) >= wcslen(wszFileName) &&
                (nAcc += 2) >= 2 &&
                VBMath::TryMultiply(nAcc, sizeof(WCHAR)));
            TEMPBUF(wszTempSearchPath, WCHAR*, nAcc * sizeof(WCHAR));
            IfNullThrow(wszTempSearchPath);
            // Copy file name, and find location of file part.
            wcscpy_s(wszTempSearchPath, nAcc, wszCurrentDir);

            // Now append the file name
            if (wszTempSearchPath[wcslen(wszTempSearchPath) - 1] != L'\\')
            {
                wcscat_s(wszTempSearchPath, nAcc, L"\\");
            }
            wcscat_s(wszTempSearchPath, nAcc, wszFileName);

            int results = GetFileAttributes( wszTempSearchPath );
            if ( results != INVALID_FILE_ATTRIBUTES && // means the file wasn't found
                 results != FILE_ATTRIBUTE_DIRECTORY ) // does us no good if we found a directory instead of a file
            {
                bFound = true;
                *ppstrFullPath = AddString(wszTempSearchPath);
                break;
            }

            wszCurrentDir = wcstok_s(NULL, wszSeparator, &tokContext);
        }
    }

    return bFound;
}

//============================================================================
// Find a critical DLL.  This will cause the compiler to die if it is not found.
//============================================================================
HRESULT Compiler::FindCriticalDllOnSDKPath
(
    _In_z_ const WCHAR *wszDll,              // IN  : Dll We're looking for
    CompilerHost *pCompilerHost,              // IN : Compiler host that knows the SDK path.
    _Deref_out_z_ STRING **ppstrFileName      // OUT  : Full Path to DLL found
)
{
    STRING* path = NULL;
    HRESULT hr = NOERROR;

    //
    // We'll search either the /sdkpath directory that
    // the user passed in or the COR system directory.
    // Mscorlib.dll and Microsoft.VisualBasic.dll
    // must be on the path or we're just horked.
    //

    //-- Get the /sdkpath from the compiler host
    hr = pCompilerHost->GetSdkPath(&path);

    if (FAILED(hr))
    {
#if IDE 
            HrMakeWithError(ERRID_CantFindCORSystemDirectory, hr);
            return hr;
#else
            return (HrMakeReplWithError(ERRID_CantFindCORSystemDirectory, hr));
#endif IDE
    }

    VSASSERT(path != NULL, "path must be set by here");

    //
    // Search for the DLL
    //

    if (!FindFileInPath(wszDll, path, ppstrFileName))
    {
#if IDE 
            HrMakeReplWithError(ERRID_CantLoadStdLibrary1, E_FAIL, wszDll);
            return E_FAIL;
#else
            return (HrMakeRepl(ERRID_CantLoadStdLibrary1, wszDll));
#endif IDE
    }
    return hr;
}

//============================================================================
// Load the default libraries.
//============================================================================

HRESULT Compiler::LoadDefaultLibraries
(
    bool CompilingTheVBRuntime,
    CompilerHost *pCompilerHost
)
{
    VB_ENTRY();
    STRING *pstrFileName;

    VSASSERT( pCompilerHost != NULL, "We must have a compiler host in order to load the default libraries");

    // Load Core library
    // Traditionally VB compiler has hard-coded the name of mscorlib.dll. In the Immersive profile the
    // library is called System.Runtime.dll. Ideally we should get rid of the dependency on the name and
    // identify the core library as the assembly that contains System.Object. At this point in the compiler,
    // it is too early though as we haven't loaded any types or assemblies. Changing this now is a deep 
    // change. So the hack here is to allow mscorlib or system.runtime and prefer system.runtime if present.
    // There is an extra check to only pick an assembly with no other assembly refs. This is so that is an 
    // user drops a user-defined binary called System.runtime.dll into the fx directory we still want to pick 
    // mscorlib. 
    STRING *pMscorlibName = NULL;
    STRING *pSystemRuntimeName = NULL;
    
    HRESULT hr1 = FindCriticalDllOnSDKPath(MSCORELIB_DLL, pCompilerHost, &pMscorlibName);
    HRESULT hr2 = FindCriticalDllOnSDKPath(SYSTEM_RUNTIME_DLL, pCompilerHost, &pSystemRuntimeName);
    if (FAILED(hr1) && FAILED(hr2))
        return hr1;

    // If both are present
    if (pSystemRuntimeName && pMscorlibName)
    {
        CompilerProject* pTempMscorlib;
        CompilerProject* pTempSystemRuntime;

        // Load both the assemblies
        hr1 = CreateProjectWithMetaData(pMscorlibName, true, pCompilerHost, NULL, true, &pTempMscorlib);
        hr2 = CreateProjectWithMetaData(pSystemRuntimeName, true, pCompilerHost, NULL, true, &pTempSystemRuntime);
        if (FAILED(hr1) && FAILED(hr2))
            return hr1;

        // Prefer the library has no assembly refs.
        if (pTempMscorlib && pTempSystemRuntime)
        {
            pTempMscorlib->BindAssemblyRefs();
            pTempSystemRuntime->BindAssemblyRefs();
            AssemblyRefIterator mscorlibIter(pTempMscorlib);
            AssemblyRefIterator systemruntimeIter(pTempSystemRuntime);
            bool mscorlibHasRefs = mscorlibIter.Next() ? true : false;
            bool systemRuntimeHasRefs = systemruntimeIter.Next() ? true : false;

            pCompilerHost->m_pComPlusProject = systemRuntimeHasRefs ? pTempMscorlib : pTempSystemRuntime;
        }
        else
        {
            pCompilerHost->m_pComPlusProject = pTempMscorlib ? pTempMscorlib : pTempSystemRuntime;
        }

        // Release the project that wasn't picked as the core library so that we can clear the memory.
        if (pTempMscorlib != pCompilerHost->m_pComPlusProject)
        {
            pTempMscorlib->Release();
        }
        if (pTempSystemRuntime != pCompilerHost->m_pComPlusProject)
        {
            pTempSystemRuntime->Release();
        }
    }
    else
    {
        pstrFileName = pMscorlibName ? pMscorlibName : pSystemRuntimeName;
        IfFailRet(CreateProjectWithMetaData(pstrFileName, true, pCompilerHost, NULL, true, &(pCompilerHost->m_pComPlusProject)));    
    }

    IfFailRet(pCompilerHost->SetRuntimeVersion(pCompilerHost->m_pComPlusProject)); // Get the version of MSCORLIB.dll.  We use this to determine our version instead of the vbruntime because they not have a vb runtime on this platform

    // Avoid setting unless necessary. This prevents a copy-on-write that causes extra data pages
    if (g_CompilingTheVBRuntime != CompilingTheVBRuntime)
    {
        g_CompilingTheVBRuntime = CompilingTheVBRuntime;
    }

    LoadXmlSolutionExtension(pCompilerHost);
    LoadVBCoreSolutionExtension(pCompilerHost);

#if HOSTED        
    if (pCompilerHost->GetDefaultVBRuntimeProject() == NULL)
    {
        CompilerProject *dummy = new CompilerProject(this);
        pCompilerHost->LoadDefaultVBRuntimeLibrary(dummy);
        RELEASE(dummy);
    }
#endif

    VB_EXIT();
}

HRESULT CompilerHost::LoadDefaultVBRuntimeLibrary
(
    CompilerProject * pCompilerProject
)
{
    VB_ENTRY();

    if (m_pDefaultVBRuntimeProject!= NULL)
    {
        pCompilerProject->SetVBRuntimeProject(m_pDefaultVBRuntimeProject);
    }
    else
    {
        IfFailRet(LoadSpecifiedVBRuntimeLibrary(
            pCompilerProject,
            NULL));

        SetDefaultVBRuntimeProject(pCompilerProject->GetVBRuntimeProject());
    }

    VB_EXIT();
}



HRESULT CompilerHost::LoadSpecifiedVBRuntimeLibrary
(
    CompilerProject * pCompilerProject,
    LPCWSTR wzsVBRuntimePath
)
{
    VB_ENTRY();

    if (this->GetVbLibraryType() == TLB_AutoDetect)
    {
        // The debugger creates a compilerhost, but that compilerhost shouldn't load ms.vb.dll.  This weird check is here
        // to figure out if the Debugger created us...
        return hr;
    }

    VSASSERT(GetCompiler() != NULL, "Must have m_pCompiler defined");

    STRING *VbRuntimePath = NULL;

    // Load the VB runtime specified.
    if ( wzsVBRuntimePath != NULL)
    {

        VbRuntimePath = GetCompiler()->AddString(wzsVBRuntimePath);

        // See if we can find it on the specified path (or in the current directory if no path is specified)
        int results = GetFileAttributes( VbRuntimePath );
        bool Found = results != INVALID_FILE_ATTRIBUTES && // means the file wasn't found
                     results != FILE_ATTRIBUTE_DIRECTORY; // does us no good if we found a directory instead of a file

        if ( !Found ) // See if they path qualified it and if they did not, then we'll look for it on the SDK path
        {
            WCHAR *Name = PathFindName( VbRuntimePath );
            if ( Name != VbRuntimePath ) // if the name doesn't start the path, then it is path qualified.  We look no further when the path to the file was explicit.
            {
                // We couldn't find their DLL
#if IDE 
                HrMakeReplWithError(ERRID_CantLoadStdLibrary1, E_FAIL, VbRuntimePath);
                return E_FAIL;
#else
                return (HrMakeRepl(ERRID_CantLoadStdLibrary1, VbRuntimePath));
#endif IDE
            }
        } // !Found
    }
    else // Load Default VB Runtime
    {
        VbRuntimePath = GetCompiler()->AddString(DEFAULT_VBRUNTIME_DLL);
        IfFailRet( GetCompiler()->FindCriticalDllOnSDKPath( VbRuntimePath, this, &VbRuntimePath )); // searches the SDK path
    }

#if IDE
    CheckInMainThread();
    BackgroundCompilerLock compilerLock(this);
#endif 

    CompilerProject* pVBRuntimeProject = NULL;

    IfFailRet( GetCompiler()->CreateProjectWithMetaData( 
        VbRuntimePath, 
        true,
        this, 
        NULL, 
        true, 
        &pVBRuntimeProject));

#if IDE
        if (pVBRuntimeProject && pVBRuntimeProject->m_pTaskProvider)
        {
            // Dev11 Bug 370167 - shiqic
            // InitWithMetaData will create a CompilerTaskProvider for each normal CompilerProject. 
			// When vb runtime is added as a normal dll, a CompilerTaskProvider will be attached to vb runtime. 
            // CompilerTaskProvider will hold  a ref on CompilerPackage. This creates a circular reference, 
            //     CompilerPackage->CompilerHost->VBRuntime(CompilerProject)->CompilerTaskProvider->CompilerPackage
            // Circular reference will cause memory leak. For default libraries(mscorlib or MS.VB.dll) CompilerTaskProvider is 
            // not necessary(see InitWithMetaData), so we can safely remove TaskProvider from vb runtime.

            pVBRuntimeProject->m_pTaskProvider->UnregisterTaskProvider();
            RELEASE(pVBRuntimeProject->m_pTaskProvider);
        }
#endif IDE


    pCompilerProject->SetVBRuntimeProject(pVBRuntimeProject);

    if (pVBRuntimeProject)
    {
        if (GetSolutionExtension(SolutionExtension::MyTemplateExtension)->m_psbCode == NULL)
        {
            // Load any solution extensions (e.g. the MY template) which are stored as resources in the runtime DLL
            HMODULE hMod = ::LoadLibraryEx(VbRuntimePath, NULL, LOAD_LIBRARY_AS_DATAFILE);
            if (hMod)
            {
                GetCompiler()->LoadSolutionExtension(SolutionExtension::MyTemplateExtension, hMod, MYTEMPLATE_RESOURCEID, this);
                ::FreeLibrary(hMod);
            }
        }     
    }

    RELEASE(pVBRuntimeProject);
    VB_EXIT();
}

// Applies a DFS on the tree of projects rooted with the project
// passed in. This functions adds projects to the array of projects
// (m_daProjectsToCompile) to be compiled in order such that referenced
// projects are added before projects that reference them.
// Topological sort of projects is very efficient and is much preffered
// over any kind of sorting, for these graph type problems.
//
void Compiler::TopologicalySortProjects
(
    CompilerProject *pProject,
    DynamicArray<CompilerProject *> *pCurrentSetOfProjectsToCompile,
    bool fMetadataToSourceRefInProgress,
    ProjectSortInfo *pSortInfo
)
{
    if (pProject->IsProjectSortInProgress())
    {
        // Cycle detected, but we are interested in cycles involving metadata
        // to source project references only.

        if (fMetadataToSourceRefInProgress)
        {
            GenerateCycleDescIfReferenceCyclePresent(pSortInfo, pProject);
        }

        return;
    }

    if (!pProject->IsProjectSeen())
    {
        // We only process nodes that haven't been seen
        pProject->SetProjectSeen(true);
        pProject->SetProjectSortInProgress(true);

        ReferenceIterator refs(pProject);

        // We then iterate over all projects that are referenced by this
        // project and apply TopologicalySortProjects() on them.
        while (CompilerProject *pReferencedProject = refs.Next())
        {
            // Set up reference cycle detecting information.
            ProjectSortInfo CurrentSortInfo;
            CurrentSortInfo.pReferencingProject = pProject;
            CurrentSortInfo.pNext = NULL;
            CurrentSortInfo.pAssemblyRef = NULL;

            if (pSortInfo)
            {
                CurrentSortInfo.pPrev = pSortInfo;
                pSortInfo->pNext = &CurrentSortInfo;
            }
            else
            {
                CurrentSortInfo.pPrev = NULL;
            }

            TopologicalySortProjects(
                pReferencedProject,
                pCurrentSetOfProjectsToCompile,
                fMetadataToSourceRefInProgress,
                &CurrentSortInfo);

            // Clear reference cycle detecting information.
            if (pSortInfo)
            {
                pSortInfo->pNext = NULL;
            }
        }

        // Iterate over all projects referenced by metadata projects
        // and apply TopologicalySortProjects() on them.
        //
        // 


        if (pProject->IsMetaData())
        {
            MetaDataFileIterator Files(pProject);

            while (MetaDataFile *pFile = Files.Next())
            {
                AssemblyRefCollection *pAssemblyRefs = pFile->GetAssemblyRefs();
                ULONG cAssemblyRefs = pAssemblyRefs->GetAssemblyRefCount();

                for(ULONG Index = 0; Index < cAssemblyRefs; Index++)
                {
                    AssemblyRefInfo *pAssemblyRefInfo = pAssemblyRefs->GetAssemblyRefInfo(Index);

                    // There should only be one project per assembly ref usually.
                    AssemblyRefReferencedProjectsIterator Iter(pAssemblyRefInfo);
                    while (CompilerProject *pReferencedProject = Iter.Next())
                    {
                        // Set up reference cycle detecting information.
                        ProjectSortInfo CurrentSortInfo;
                        CurrentSortInfo.pReferencingProject = pProject;
                        CurrentSortInfo.pNext = NULL;
                        CurrentSortInfo.pAssemblyRef = pAssemblyRefInfo;

                        if (pSortInfo)
                        {
                            CurrentSortInfo.pPrev = pSortInfo;
                            pSortInfo->pNext = &CurrentSortInfo;
                        }
                        else
                        {
                            CurrentSortInfo.pPrev = NULL;
                        }

                        TopologicalySortProjects(
                            pReferencedProject,
                            pCurrentSetOfProjectsToCompile,
                            !pReferencedProject->IsMetaData() ? true : fMetadataToSourceRefInProgress,
                            &CurrentSortInfo);

                        // Clear reference cycle detecting information.
                        if (pSortInfo)
                        {
                            pSortInfo->pNext = NULL;
                        }
                    }
                }
            }
        }

        // Only add the project once we have done its children
        pCurrentSetOfProjectsToCompile->Add() = pProject;

        pProject->SetProjectSortInProgress(false);
    }
}

void Compiler::GenerateCycleDescIfReferenceCyclePresent
(
    ProjectSortInfo *pSortInfo,
    CompilerProject *pProject
)
{
    ProjectSortInfo *pCurrentSortInfo = pSortInfo;
    bool fMetadataToSourceRefPresent = false;
    CompilerProject *pNextProject = pProject;

    // Any metadata to source project reference in cycle ?
    while (pCurrentSortInfo->pReferencingProject != pProject)
    {
        if (!pNextProject->IsMetaData() && pCurrentSortInfo->pReferencingProject->IsMetaData())
        {
            VSASSERT(pCurrentSortInfo->pAssemblyRef, "Assembly ref expected for metadata project!!!");

            // If cycle description already generated for this project, then there is no need
            // to generate again.
            if (!pCurrentSortInfo->pAssemblyRef->m_fCausesReferenceCycle)
            {
                fMetadataToSourceRefPresent = true;
                break;
            }
        }

        pNextProject = pCurrentSortInfo->pReferencingProject;
        pCurrentSortInfo = pCurrentSortInfo->pPrev;
    }

    // Generate cycle desc.
    if (fMetadataToSourceRefPresent)
    {
        ProjectSortInfo *pFirstSortInfo = pCurrentSortInfo;
        do
        {
            pFirstSortInfo = pFirstSortInfo->pPrev;
        } while (pFirstSortInfo->pReferencingProject != pProject);

        do
        {
            if (!pNextProject->IsMetaData() && pCurrentSortInfo->pReferencingProject->IsMetaData())
            {
                VSASSERT(pCurrentSortInfo->pAssemblyRef, "Assembly ref expected for metadata project!!!");

                // If cycle description already generated for this project, then there is no need
                // to generate again.
                if (!pCurrentSortInfo->pAssemblyRef->m_fCausesReferenceCycle)
                {
                    // This error overrides any other errors for this assembly ref.

                    StringBuffer CycleDesc;
                    GetRefCycleDescString(pCurrentSortInfo, pFirstSortInfo, &CycleDesc);

                    // 
                    pCurrentSortInfo->pAssemblyRef->m_pstrErrorExtra = this->AddString(CycleDesc.GetString());
                    pCurrentSortInfo->pAssemblyRef->m_ErrorID = ERRID_ReferencedAssemblyCausesCycle3;
                    pCurrentSortInfo->pAssemblyRef->m_fCausesReferenceCycle = true;
                }
            }

            pNextProject = pCurrentSortInfo->pReferencingProject;
            pCurrentSortInfo = pCurrentSortInfo->pPrev;

        } while (pCurrentSortInfo != pFirstSortInfo);
    }
}

void Compiler::GetRefCycleDescString
(
    ProjectSortInfo *pStartSortInfo,
    ProjectSortInfo *pEndSortInfo,
    StringBuffer *pCycleDesc
)
{
    CompilerProject *pProjPrev = pStartSortInfo->pReferencingProject;
    ProjectSortInfo *pCurrentSortInfo = pStartSortInfo;

    do
    {
        pCurrentSortInfo =
            pCurrentSortInfo->pNext ? pCurrentSortInfo->pNext : pEndSortInfo;

        STRING *pstrFrom = GetErrorProjectName(pProjPrev);
        STRING *pstrTo = GetErrorProjectName(pCurrentSortInfo->pReferencingProject);
        ERRID ErrId = 0;

        if (pProjPrev->IsMetaData())
        {
            if (pCurrentSortInfo->pReferencingProject->IsMetaData())
            {
                ErrId = ERRID_AssemblyRefAssembly2;
            }
            else
            {
                ErrId = ERRID_AssemblyRefProject2;
            }
        }
        else
        {
            if (pCurrentSortInfo->pReferencingProject->IsMetaData())
            {
                ErrId = ERRID_ProjectRefAssembly2;
            }
            else
            {
                ErrId = ERRID_ProjectRefProject2;
            }
        }

        ResLoadStringRepl(
            ErrId,
            pCycleDesc,
            pstrFrom,
            pstrTo);

        pProjPrev = pCurrentSortInfo->pReferencingProject;

    } while (pCurrentSortInfo != pStartSortInfo);
}

//============================================================================
// Determine the set of projects we need to build and
// in what order we have to build them.  If the project
// is not passed in, then compile all of the projects
// in memory.
//
// The project build order is as follows:
//
//  1) All MetaData projects in this Compiler, whether referenced or not
//  2) All Basic projects referenced by pProject
//  3) pProject
//
// Further, projects are ordered so that any referenced project precedes the
// referencing project.
//
// If pProject is NULL, then all Basic projects in this Compiler are
// added to the list.
//============================================================================

HRESULT Compiler::BuildProjectCompilationList
(
    CompilerProject *pProject,                          // [IN] Project to compiler , or NULL to build all projects
    CompilerHost *pCompilerHost,                        // [IN] CompilerHost for this project
    DynamicArray<CompilerProject *> *pProjectsToCompile,// [OUT] An ordered list of projects to compile
    bool BindAssemblyRefs,                              // [IN] Whether assembly refs should be bound or not.
    DynamicArray<CompilerProject *> *pPreferredProjects // [IN] Front load the array with these projects and their dependencies
)
{
    VB_ENTRY();

    // If we haven't been informed as to the location of COM+ yet, do nothing.
    if (!pCompilerHost->GetComPlusProject())
    {
        hr = E_FAIL;
        goto Error;
    }

    ProjectIterator projects(pCompilerHost);
    CompilerProject *pAProject;

    // First loop resets the "IsSeen" bit for all non-metadata projects
    // while adding all metadata projects to the array. This needs to be done
    // before we apply Topological sort to the projects.
    CompilerIdeLock spLock(pCompilerHost->m_csProjects);

#if IDE 
    if (BindAssemblyRefs)
    {
        while (pAProject = projects.Next())
        {
            // Clear all the implicit references for all the projects.
            pAProject->m_daImplicitReferencesOnMe.Collapse();
        }
    }

    projects.Reset();
#endif IDE

    while (pAProject = projects.Next())
    {
        if (pAProject->IsInitialized())
        {
            pAProject->SetProjectSeen(false);

            if (BindAssemblyRefs)
            {
                // 




                pAProject->BindAssemblyRefs();
            }
        }
        else
        {
            // Uninitialized projects shouldn't be visited
            pAProject->SetProjectSeen(true);
        }
    }

    // Always process this special library first because all other projects depend on it.
    pProjectsToCompile->Add() = pCompilerHost->GetComPlusProject();
    pCompilerHost->GetComPlusProject()->SetProjectSeen(true);

    if (pProject)
    {
        VSASSERT(pProject->IsInitialized(), L"How can we try to compile a non-initialized project?");

        // If we are given a project to compile, we only need to process this one sub tree
        // and no more.
        TopologicalySortProjects(pProject, pProjectsToCompile, false, NULL);
    }
    else
    {
        if (pPreferredProjects)
        {
            for (unsigned long Index = 0; Index < pPreferredProjects->Count(); Index++)
            {
                TopologicalySortProjects(pPreferredProjects->Element(Index), pProjectsToCompile, false, NULL);
            }
        }

        projects.Reset();

        // But if we don't have a project to start with, we need to process all projects.
        while(pAProject = projects.Next())
        {
            TopologicalySortProjects(pAProject, pProjectsToCompile, false, NULL);
        }
    }

    VB_EXIT();

Error:

#if DEBUG
    // In DEBUG, we verify that we don't have any duplicate entries in the array of projects to compile.
    unsigned            cProjects = pProjectsToCompile->Count();
    CompilerProject **rgpProjects = pProjectsToCompile->Array();

    // Walk the list and verify that each project exists only once.
    for (unsigned iProject = 0; iProject < cProjects; iProject++)
    {
        for (unsigned jProject = iProject + 1; jProject < cProjects; jProject++)
        {
            VSASSERT(pProjectsToCompile->Element(iProject) != pProjectsToCompile->Element(jProject), "Duplicate projects to compile!");
        }
    }
#endif DEBUG

    return hr;
}

//============================================================================
// Helper to sort errors.
//============================================================================

int _cdecl CompareVbErrors( const void *arg1, const void *arg2 )
{
#if IDE
    BCError *pErr1 = (BCError *)arg1;
    BCError *pErr2 = (BCError *)arg2;
#else
    VbError *pErr1 = (VbError *)arg1;
    VbError *pErr2 = (VbError *)arg2;
#endif

    int iCompare;

#if IDE
    // 0. Sort WinMDExp errors before VB errors
    if (pErr1->m_fIsWME && !pErr2->m_fIsWME)
        return -1;
    else if (!pErr1->m_fIsWME && pErr2->m_fIsWME)
        return 1;

    if (!pErr1->m_fIsWME)
    {
#endif
    // 1. Sort by ERRID_MaximumNumberOfErrors
    // ERRID_MaximumNumberOfErrors should sort to the end
    if (pErr1->dwErrId == ERRID_MaximumNumberOfErrors || pErr1->dwErrId == WRNID_MaximumNumberOfWarnings)
    {
        return 1;
    }
    else if (pErr2->dwErrId == ERRID_MaximumNumberOfErrors || pErr1->dwErrId == WRNID_MaximumNumberOfWarnings)
    {
        return -1;
    }
#if IDE
    }
#endif

    // 2. Sort by file name
    // Errors with no filename sort to the beginning
    if (pErr1->FileName != pErr2->FileName)
    {
        if (!pErr1->FileName)
        {
            return -1;
        }
        else if (!pErr2->FileName)
        {
            return 1;
        }
        else if ((iCompare = wcscmp(pErr1->FileName, pErr2->FileName)) != 0)
        {
            return iCompare;
        }
    }

    // 3. Sort by Line Number
    if (pErr1->dwBeginLine != pErr2->dwBeginLine)
    {
        return pErr1->dwBeginLine - pErr2->dwBeginLine;
    }
    else if (pErr1->dwBeginCol != pErr2->dwBeginCol)
    {
        return pErr1->dwBeginCol - pErr2->dwBeginCol;
    }
    else if (pErr1->dwEndCol != pErr2->dwEndCol)
    {
        return pErr1->dwEndCol - pErr2->dwEndCol;
    }

#if IDE
    if (!pErr1->m_fIsWME)
    {
#endif
    // 4. Sort by ERRID_WarningTreatedAsError
    // ERRID_WarningTreatedAsError should sort after other errors on the same line
    if (pErr1->dwErrId == ERRID_WarningTreatedAsError)
    {
        return 1;
    }
    else if (pErr2->dwErrId == ERRID_WarningTreatedAsError)
    {
        return -1;
    }
#if IDE
    }
#endif

    // 5. Sort by Description
    if (pErr1->Description != pErr2->Description)
    {
        if (!pErr1->Description)
        {
            return -1;
        }
        else if (!pErr2->Description)
        {
            return 1;
        }
        else if ((iCompare = CompareString32NoCase(pErr1->Description, pErr2->Description)) != 0)
        {
            return iCompare;
        }
    }

    // 5. The two error elements are sorted equal
    return 0;
}

//============================================================================
// Get the array of VbErrors describing the compilation errors
//============================================================================

void Compiler::BuildErrorList
(
    VbError **ppErrors,
    unsigned *pcErrors,
    unsigned *pcWarnings
)
{
    DynamicArray<BCError> daErrors;

    unsigned cTotalErrors = 0;
    unsigned cTotalWarnings = 0;

    for (CompilerHost *pCurrentCompilerHost = m_CompilerHostsList.GetFirst();
         pCurrentCompilerHost;
         pCurrentCompilerHost = pCurrentCompilerHost->Next())
    {
        // Build the list.
        CompilerProject **rgpProjects = pCurrentCompilerHost->GetProjectsToCompile()->Array();
        unsigned i, c                 = pCurrentCompilerHost->GetProjectsToCompile()->Count();
        unsigned cErrors = 0;
        unsigned cWarnings = 0;

        for (i = 0; i < c; i++)
        {
            // Build the list of errors for this project.
            BuildErrorListForOneProject(rgpProjects[i], &daErrors, NULL, &cErrors, &cWarnings);
        }

        cTotalErrors += cErrors;
        cTotalWarnings += cWarnings;
    }

    // Do we want to save the errors array?
    if (ppErrors && daErrors.Count())  // We know that we won't overflow here, since erros is constrained to be <= 100.
    {
#if IDE
        *ppErrors = MoveBCErrorArrayToVbErrorArray(&daErrors);
#else
        VbError *rgErrors = NULL;
        size_t cErrors = daErrors.Count();

        if (VBMath::TryMultiply(cErrors, sizeof(VbError)))
        {
            size_t cbSize = cErrors * sizeof(VbError);

            rgErrors = (VbError *)CoTaskMemAlloc(cbSize);

            memcpy(rgErrors, daErrors.Array(), cbSize);
            memset(daErrors.Array(), 0, cbSize);
        }

        *ppErrors = rgErrors;
#endif
    }

    if (pcErrors)
    {
        *pcErrors = cTotalErrors;
    }

    if (pcWarnings)
    {
        *pcWarnings = cTotalWarnings;
    }
}

#if IDE
VbError* Compiler::MoveBCErrorArrayToVbErrorArray(DynamicArray<BCError>* pdaErrors)
{
    VbError *rgErrors = NULL;
    size_t cErrors = pdaErrors->Count();

    if (VBMath::TryMultiply(cErrors, sizeof(VbError)))
    {
        size_t cbSize = cErrors * sizeof(VbError);

        rgErrors = (VbError *)CoTaskMemAlloc(cbSize);

        for (size_t i = 0; i < cErrors; i++)
        {
            VbError& vbError = pdaErrors->Array()[i];
            rgErrors[i] = vbError;
        }

        pdaErrors->Collapse();
    }

    return rgErrors;
}
#endif

void Compiler::BuildErrorListForOneProject
(
    CompilerProject *pProject,
    DynamicArray<BCError> *pdaErrors,
    VbError **ppErrors,
    unsigned *pcErrors,
    unsigned *pcWarnings
)
{
    unsigned cErrors = 0;
    unsigned cWarnings = 0;
    unsigned cLastErrors = pdaErrors->Count();

    CompilerHost *pCompilerHost = pProject->GetCompilerHost();

    // Get the strings for "error" and "warning"
    // Note (Microsoft 09-15-2000): kperry says that the words "error" and "warning"
    // should never be localized for compiler error outputs so that scripts
    // that parse the errors will always work.
    const WCHAR* wszError   = L"error";
    const WCHAR* wszWarning = L"warning";

    // Build the list of errors for this project.
    BuildErrorArray(
        pProject, 
        pdaErrors, 
        NULL
#ifndef MAX_ERRORS_DISABLED        
        , BuildErrorArrayMaxErrors
        , BuildErrorArrayMaxWarnings
#endif        
        );

    // Display any new errors we've found.
    if (pdaErrors->Count() != cLastErrors)
    {
        // Sort the errors.
        qsort(pdaErrors->Array() + cLastErrors,
                pdaErrors->Count() - cLastErrors,
#if IDE
                sizeof(BCError),
#else
                sizeof(VbError),
#endif
                CompareVbErrors);

        BCError *pError = &pdaErrors->Array()[cLastErrors];
        BCError *pErrorMax = pError + pdaErrors->Count() - cLastErrors;

        for (; pError < pErrorMax; pError++)
        {
            // Don't print task items.
            if (pError->dwErrId)
            {
                if (pError->FileName)
                {
                    pCompilerHost->Printf(L"%s", pError->FileName);

                    if (pError->dwBeginLine)
                    {
                        pCompilerHost->Printf(L"(%d)", pError->dwBeginLine);
                    }

                    pCompilerHost->Printf(L" : ");
                }
                else
                {
                    pCompilerHost->Printf(L"vbc : ");  // non-localized component name
                }

                bool fReprotedAsError = ReportedAsError(pError->dwErrId, pProject);
                if (fReprotedAsError)
                    cErrors++;
                else
                    cWarnings++;

                pCompilerHost->Printf(L"%s BC%05d: %s\n",
                        fReprotedAsError ? wszError : wszWarning,
                        pError->dwErrId,
                        pError->Description);

#if !IDE
                // Print the error text, if we have it.
                if (pError->SourceText != NULL
                    && OutputLevel() > OUTPUT_Quiet)
                {
                    const long iLineStart = pError->dwBeginLine - 1;
                    const long iColumnStart = pError->dwBeginCol - 1;
                    const long iLineEnd = pError->dwEndLine - 1;
                    const long iColumnEnd = pError->dwEndCol - 1;
                    const WCHAR* wszLine = pError->SourceText;
                    const WCHAR* wsz = wszLine;
                    StringBuffer lineBuffer;
                    StringBuffer squiggleBuffer;

                    // spit out an initial line feed
                    pCompilerHost->OutputString(L"\n");

                    for (long iLine = iLineStart; iLine <= iLineEnd; iLine++)
                    {
                        // start with empty buffers
                        lineBuffer.Truncate(0);
                        squiggleBuffer.Truncate(0);

                        long iPos = 0;

                        for (wsz = wszLine; *wsz != L'\0'; wsz++, iPos++)
                        {
                            bool inErrorSpan = false;

                            if (iLineStart == iLineEnd)
                            {
                                inErrorSpan = (iPos >= iColumnStart && iPos <= iColumnEnd);
                            }
                            else if ((iLine == iLineStart && iPos >= iColumnStart) ||
                                        (iLine == iLineEnd && iPos <= iColumnEnd) ||
                                        (iLine > iLineStart && iLine < iLineEnd))
                            {
                                inErrorSpan = true;
                            }

                            if (*wsz == L'\n' || *wsz == L'\r')
                            {
                                // If error span consists only of the newline, then squiggle it
                                if (inErrorSpan && iLineStart == iLineEnd && iColumnStart == iColumnEnd)
                                {
                                    squiggleBuffer.AppendChar(L'~');
                                }

                                // EOL. Advance to the next line
                                wszLine = wsz;
                                while (*wszLine == L'\n' || *wszLine == L'\r')
                                {
                                    wszLine++;
                                }
                                break;  // (next iLine)
                            }
                            else if (*wsz == L'\t')
                            {
                                // Emit 4 spaces for a TAB
                                lineBuffer.AppendWithLength(L"    ", 4);

                                if (inErrorSpan)
                                {
                                    squiggleBuffer.AppendWithLength(L"~   ", 4);
                                }
                                else
                                {
                                    squiggleBuffer.AppendWithLength(L"    ", 4);
                                }
                            }
                            else
                            {
                                // Emit the character into the lineBuffer
                                lineBuffer.AppendChar(*wsz);

                                // Emit a space or a squiggle into the squiggleBuffer
                                if (inErrorSpan)
                                {
                                    squiggleBuffer.AppendChar(L'~');
                                }
                                else
                                {
                                    squiggleBuffer.AppendChar(L' ');
                                }
                            }
                        }

                        // Print the lines that we just filled up
                        lineBuffer.AppendChar(L'\n');
                        squiggleBuffer.AppendChar(L'\n');
                        pCompilerHost->OutputString(lineBuffer.GetString());
                        pCompilerHost->OutputString(squiggleBuffer.GetString());
                    }
                }
#endif !IDE
            }
        }
    }

   // Do we want to save the errors array?
    if (ppErrors && pdaErrors->Count())  // We know that we won't overflow here, since erros is constrained to be <= 100.
    {
#if IDE
        *ppErrors = MoveBCErrorArrayToVbErrorArray(pdaErrors);
#else
        VbError *rgErrors = NULL;
        size_t cErrorCount = pdaErrors->Count();

        if (VBMath::TryMultiply(cErrorCount, sizeof(VbError)))
        {
            size_t cbSize = cErrorCount * sizeof(VbError);

            rgErrors = (VbError *)CoTaskMemAlloc(cbSize);

            memcpy(rgErrors, pdaErrors->Array(), cbSize);
            memset(pdaErrors->Array(), 0, cbSize);
        }

        *ppErrors = rgErrors;
#endif
    }

    if (pcErrors)
    {
        *pcErrors = cErrors;
    }

    if (pcWarnings)
    {
        *pcWarnings = cWarnings;
    }
}

void Compiler::BuildErrorListForProjectAndMetadataReferences
(
    CompilerProject *pProject,
    VbError **ppErrors,
    unsigned *pcErrors,
    unsigned *pcWarnings
)
{
    DynamicArray<BCError> daErrors;
    unsigned cTotalErrors = 0;
    unsigned cTotalWarnings = 0;

    BuildErrorListForOneProject(pProject, &daErrors, NULL, &cTotalErrors, &cTotalWarnings);

    ReferenceIterator References(pProject);
    while (CompilerProject *pReferencedProject = References.Next())
    {
        if (pReferencedProject->IsMetaData())
        {
            unsigned cErrors = 0;
            unsigned cWarnings = 0;

            BuildErrorListForOneProject(pReferencedProject, &daErrors, NULL, &cErrors, &cWarnings);

            cTotalErrors += cErrors;
            cTotalWarnings += cWarnings;
        }
    }

    // Do we want to save the errors array?
    if (ppErrors && daErrors.Count())  // We know that we won't overflow here, since erros is constrained to be <= 100.
    {
#if IDE
        *ppErrors = MoveBCErrorArrayToVbErrorArray(&daErrors);
#else
        VbError *rgErrors = NULL;
        size_t cErrors = daErrors.Count();

        if (VBMath::TryMultiply(cErrors, sizeof(VbError)))
        {
            size_t cbSize = cErrors * sizeof(VbError);

            rgErrors = (VbError *)CoTaskMemAlloc(cbSize);

            memcpy(rgErrors, daErrors.Array(), cbSize);
            memset(daErrors.Array(), 0, cbSize);
        }

        *ppErrors = rgErrors;
#endif
    }

    if (pcErrors)
    {
        *pcErrors = cTotalErrors;
    }

    if (pcWarnings)
    {
        *pcWarnings = cTotalWarnings;
    }
}

// Namespace management

bool
Compiler::NamespaceIsAvailable
(
    CompilerProject *LookupProject,
    BCSYM_Namespace *NamespaceToTest
)
{
    CompilerProject *NamespaceProject = NamespaceToTest->GetCompilerFile()->GetProject();

    return
        LookupProject == NamespaceProject ||
            LookupProject->IsProjectReferenced(NamespaceProject);
}

STRING *
Compiler::GetMatchedQualifiedName
(
    _In_z_ STRING *QualifiedName,
    CompilerProject *LookupProject,
    BCSYM_NamespaceRing *NamespaceRing
)
{
    BCSYM_Namespace *CurrentNamespace = NamespaceRing->GetFirstNamespace();
    if (CurrentNamespace)
    {
        do
        {
            if (NamespaceIsAvailable(LookupProject, CurrentNamespace))
            {
                return
                    ConcatNameSpaceAndName(
                        this,
                        CurrentNamespace->GetNameSpace(),
                        CurrentNamespace->GetName());
            }

            CurrentNamespace = CurrentNamespace->GetNextNamespace();
        } while (CurrentNamespace != NamespaceRing->GetFirstNamespace());
    }

    return QualifiedName;
}

STRING *
Compiler::GetMatchedQualifiedName
(
    _In_z_ STRING *QualifiedName,
    CompilerProject *LookupProject
)
{
    return
        GetMatchedQualifiedName(
            QualifiedName,
            LookupProject,
            GetNamespaceRing(QualifiedName));
}

BCSYM_NamespaceRing *
Compiler::GetNamespaceRing
(
    _In_z_ STRING *QualifiedName
)
{
    // Find the appropriate NamespaceRing.

    BCSYM_NamedRoot *NamespaceRingDeclaration = m_pNamespaceRings->SimpleBind(QualifiedName);

    // If no appropriate NamespaceRing exists, create one.

    if (NamespaceRingDeclaration == NULL)
    {
        Symbols NamespaceRingCreator(
            this,
            m_pEternalStorage,
            NULL);

        NamespaceRingDeclaration = NamespaceRingCreator.GetNamespaceRing(QualifiedName, m_pNamespaceRings);

        Symbols::AddSymbolToHash(m_pNamespaceRings, NamespaceRingDeclaration, true, false, false);
    }

    return NamespaceRingDeclaration->PNamespaceRing();
}

BCSYM_Namespace *
Compiler::GetNamespace
(
    _In_z_ STRING *QualifiedName,
    CompilerFile *File,
    Location *Loc,
    unsigned HashBuckets,               // Defaults to zero.
    unsigned UnBindableHashBuckets      // Defaults to zero.
)
{
    // Find the appropriate NamespaceRing.

    BCSYM_NamespaceRing *NamespaceRing = GetNamespaceRing(QualifiedName);
    BCSYM_Namespace *Result = File->GetNamespace(QualifiedName);

    // If no namespace with the appropriate name exists in this file, create one.

    if (Result == NULL)
    {
        // create the new symbol.

        STRING *ParentQualifiedName = NULL;
        STRING *UnqualifiedName = NULL;

        SplitTypeName(this, QualifiedName, &ParentQualifiedName, &UnqualifiedName);

        Symbols NamespaceCreator(
            this,
            File->SymbolStorage(),
            NULL);

        Result =
            NamespaceCreator.GetNamespace(
                UnqualifiedName,
                ParentQualifiedName,
                NamespaceRing,
                File,
                HashBuckets,
                UnBindableHashBuckets,
                Loc);

         //Add the namespace to the file
        File->AddNamespace(QualifiedName, Result);

        // Add the new symbol to its parent's hash table.

        if (StringPool::StringLength(UnqualifiedName) == 0)
        {
            // No parent for an unnamed namespace.

            VSASSERT(
                StringPool::StringLength(ParentQualifiedName) == 0,
                "Attempt to create an unnamed namespace inside a named namespace.");
        }
        else
        {
            BCSYM_Container *Parent =
                StringPool::StringLength(ParentQualifiedName) == 0 ?
                    GetUnnamedNamespace(File) :
                    GetNamespace(ParentQualifiedName, File, Loc);

            Symbols::AddSymbolToHash(Parent->GetHash(), Result, true, false, true);
        }

        // For performance, rotate the ring to point to the newly created Namespace symbol.
        NamespaceRing->SetFirstNamespace(Result);
    }

    return Result;
}

BCSYM_Namespace *
Compiler::GetUnnamedNamespace
(
    CompilerFile *File,
    unsigned HashBuckets,
    unsigned cUnBindableHashBuckets
)
{
    BCSYM_Namespace *Result = File->GetUnnamedNamespace();

    if (Result == NULL)
    {
        STRING *NullName = STRING_CONST(this, EmptyString);

        Result =
            GetNamespace(
                NullName,
                File,
                NULL, // no location
                HashBuckets,
                cUnBindableHashBuckets);

        File->SetUnnamedNamespace(Result);
    }

    return Result;
}

BCSYM_Namespace *
Compiler::GetUnnamedNamespace
(
    CompilerProject *Project
)
{
    if (Project)
    {
        FileIterator Files(Project);
        CompilerFile *pCompilerFile;

        while (pCompilerFile = Files.Next())
        {
            if (pCompilerFile->GetUnnamedNamespace())
            {
                return pCompilerFile->GetUnnamedNamespace();
            }
        }
    }

    return NULL;
}

BCSYM_Namespace *
Compiler::GetUnnamedNamespace
(
)
{
    return m_pNamespaceRings->SimpleBind(STRING_CONST(this, EmptyString))->PNamespaceRing()->GetFirstNamespace();
}


/**************************************************************************************************
;AddNamespace

Add a namespace to the ring of namespaces
***************************************************************************************************/
void
Compiler::AddNamespace
(
    BCSYM_Namespace *Namespace
)
{
    BCSYM_NamespaceRing *Ring = Namespace->GetNamespaceRing();

    if (Ring->GetFirstNamespace())
    {
        Namespace->SetPrevNamespace(Ring->GetFirstNamespace());
        Namespace->SetNextNamespace(Ring->GetFirstNamespace()->GetNextNamespace());

        Namespace->GetNextNamespace()->SetPrevNamespace(Namespace);
        Namespace->GetPrevNamespace()->SetNextNamespace(Namespace);
    }
    else
    {
        Namespace->SetPrevNamespace(Namespace);
        Namespace->SetNextNamespace(Namespace);
    }
}

void
Compiler::RemoveNamespace
(
    BCSYM_Namespace *pNamespace
)
{
    BCSYM_NamespaceRing *Ring = pNamespace->GetNamespaceRing();

#if HOSTED
    if (pNamespace->GetHashRaw() && pNamespace->GetHashRaw()->GetIsHostedDynamicHash())
    {
        //There is no need to verify the current value because this codepath is only invoked when
        // destroying every external-associated source (TypeScope and ScriptScope).
        Ring->SetContainsHostedDynamicHash(false);
    }
#endif

    if (Ring->GetFirstNamespace() == pNamespace)
    {
        // If this is the only namespace in the ring, throw the ring away.

        if (pNamespace->GetNextNamespace() == pNamespace)
        {
            Ring->SetFirstNamespace(NULL);
            return;
        }

        Ring->SetFirstNamespace(pNamespace->GetNextNamespace());
    }

    // Remove this namespace from the ring.

    pNamespace->GetPrevNamespace()->SetNextNamespace(pNamespace->GetNextNamespace());
    pNamespace->GetNextNamespace()->SetPrevNamespace(pNamespace->GetPrevNamespace());
}

BCSYM_Namespace *
Compiler::FindNamespace
(
    _In_z_ STRING *NamespaceName
)
{
    BCSYM_NamedRoot *NamespaceRingDeclaration =
        m_pNamespaceRings->SimpleBind(NamespaceName);

    if (NamespaceRingDeclaration == NULL)
    {
        return NULL;
    }

    return NamespaceRingDeclaration->PNamespaceRing()->GetFirstNamespace();
}

STRING *
Compiler::OperatorToString
(
    UserDefinedOperators op
) const
{
    tokens tkOperator = OperatorTokenTypes(op);
    return m_pStringPool->TokenToString(tkOperator);
}


// First time we hit the idle loop we start the background compiler if it hasn't
// started already.
void Compiler::StartBackgroundThreadIfNeeded(bool fCalledFromIdleLoop)
{
#if IDE 
    CheckInMainThread();

    if (GetCompilerSharedState()->IsBackgroundCompilationBlocked())
    {
        if (m_fBackgroundThreadNeedsCreating  == true)
        {
            m_fBackgroundThreadNeedsCreating = false;
            StartBackgroundThread();
        }

        if (!fCalledFromIdleLoop && m_fBackgroundThreadStoppedForReplaceAll)
        {
            m_fBackgroundThreadStoppedForReplaceAll = false;
            StartBackgroundThread();
        }
    }
#endif
}

#if FV_TRACK_MEMORY && IDE
Instrumentation * Compiler::GetInstrumentation()
{
    return &m_instrumentation;
}
#endif

HRESULT Compiler::CreateXMLDocument(REFIID riid, void **ppXMLDocument)
{
    HRESULT hr = E_FAIL;
    CComCritSecLock<CComAutoCriticalSection> spLock(m_csXMLDOMFactory);
    if (m_srpXMLDOMFactory == NULL)
    {
        hr = CoGetDOMClassObject(IID_IClassFactory, (void **) &m_srpXMLDOMFactory);
    }

    if (m_srpXMLDOMFactory)
    {
        hr = m_srpXMLDOMFactory->CreateInstance(NULL, riid, ppXMLDocument);
    }

    return hr;
}


/*****************************************************************************
;SkipTypeMember

For performance reasons, don't load members of types that we 
shouldn't be able to see anyway from the referencing assembly.

When we do delay loading (via EnsureChildrenLoaded()) we can cut out
the expense of loading inacessible fields/procs when we know we have no chance
of anyone accessing them.

Note that a side-effect of not importing the inaccessible stuff is that
errors which formerly said something was inaccessible will now say that it is
not found, with a note that it may be due to accessibility.

Also note that the debugger needs to override this function in VbeeCompiler
so that during debugging that override will be hardwired to return false so
that ALL types are loaded.  That is necessary since the debugger uses us to
load assemblies and it needs to see everything or else you can't see friend/
private stuff in the watch window, access it during expression eval, etc.
*****************************************************************************/
bool Compiler::SkipTypeMember(
    // the name of the member
    _In_ STRING *pstrMemberName, 
    // the accessibility flags for the member
    _In_ DECLFLAGS MemberDeclFlags, 
    // the project the type belongs to
    _In_ CompilerProject *pProject,
    // token of the type member
    _In_ mdToken Token,
    // the metadata file the token came from
    _In_ MetaDataFile *pMetaDataFile
)
{
    if ( MemberDeclFlags & DECLF_Private ) // don't import private members of classes in most situations...
    {
        // There is one private field that we can't drop: the backing field for a WithEvent variable.  Since that field always starts with '_'
        // by convention we can do a quick check to see if we are even interested in doing the work to determine whether we are looking at
        // a WithEvents field or not.
        // A WithEvents field can be detected by the <AccessedThroughProperty()> attribute that is on it.  If we see that we won't skip the member.
        if ( pstrMemberName && pstrMemberName[0] == L'_' && TypeFromToken(Token) == mdtFieldDef )
        {
            // See if this could be the backing field for an event
            TriState<bool> IsBackingFieldForEvent = Attribute::HasAttribute( pMetaDataFile, Token, attrkindAccessedThroughProperty_String );
            
            // The check for HasValue() is necessary because in some remote/device debugging scenarioes the assembly we are in may
            // not be on disk, in which case HasValue() is false below.  In that case, don't filter anything out since we can't make an
            // informed decision about what attribute is on the field.
            if ( !IsBackingFieldForEvent.HasValue() || IsBackingFieldForEvent.GetValue() == true )
            {
                return false; // we have to provide the event backing field so we can do handles correctly
            }
        }
        return ! pProject->ShouldImportPrivates(); // we check this method because if this is a PIA, for instance, we need to load the private members
    }

    return false;
}
