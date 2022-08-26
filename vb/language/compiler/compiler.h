//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  The VB compiler main entrypoints.
//
//-------------------------------------------------------------------------------------------------

#pragma once

#define E_INTERRUPTED E_PENDING

#ifndef FEATURE_CORESYSTEM
#define MSCORELIB_DLL       L"mscorlib.dll"
#define SYSTEM_RUNTIME_DLL  L"System.Runtime.dll"
#define VBRUNTIME_NAMESPACE L"Microsoft.VisualBasic"
#define VBRUNTIME_DLL       L"Microsoft.VisualBasic.dll"
#define SYSTEM_DLL          L"System.dll"
#define SYSTEM_LIBRARY_DLL  L"System.Library.dll"
#define SYSTEM_CORE_DLL     L"System.Core.dll"
#else
#define MSCORELIB_DLL       L"mscorlib.ni.dll"
#define SYSTEM_RUNTIME_DLL  L"System.Runtime.ni.dll"
#define VBRUNTIME_NAMESPACE L"Microsoft.VisualBasic"
#define VBRUNTIME_DLL       L"Microsoft.VisualBasic.ni.dll"
#define SYSTEM_DLL          L"System.ni.dll"
#define SYSTEM_LIBRARY_DLL  L"System.Library.dll"
#define SYSTEM_CORE_DLL     L"System.Core.ni.dll"
#endif

#define CORESYS_SYSTEM_DIR  L"c:\\windows\\system32\\"

struct BCVirtualAlloc;
struct NorlsPage;
class SharedContext;
class TextBufferWrapper;
struct IVBHostFactory;

extern bool g_CompilingTheVBRuntime;

int _cdecl CompareVbErrors( const void *arg1, const void *arg2 );

//============================================================================
// Enum for evaluating simple library routines
//============================================================================

enum RuntimeFunction
{
    RuntimeFunctionChr,
    RuntimeFunctionChrW,
    RuntimeFunctionAsc,
    RuntimeFunctionAscW,

    RuntimeFunctionMax
};

enum MappedMember
{
    MappedMemberIsNumericV1,
    MappedMemberIsNumeric,
    MappedMemberCallByNameV1,
    MappedMemberCallByName,
    MappedMemberTypeNameV1,
    MappedMemberTypeName,
    MappedMemberSystemTypeNameV1,
    MappedMemberSystemTypeName,
    MappedMemberVbTypeNameV1,
    MappedMemberVbTypeName,

    MappedMemberMax
};

enum RuntimeVersion
{
    RuntimeVersion1 = 1,
    RuntimeVersion2 // See the note in Compiler.cpp: "we only care about the distinction between pre-Whidbey and >= Whidbey so everything after version 1 is considered Version2"
};


enum RuntimeSymbolCacheStatus
{
    NotCachedYet = 0, // it is important that this be zero as we zero this memory to clear the status
    Cached, // we found the symbol and have cached it
    SymbolNotDefined // we tried to find the symbol but it isn't defined.
};

//****************************************************************************
// CompilerHost is host specific. For each unique VBCompilerHost, a CompilerHost is created and is
// added to the list of hosts which the Compiler knows about. The CompilerHost object holds all
// elements which can differ between hosts.  Everything else is stored in Compiler.
//****************************************************************************
class CompilerHost : public CSingleLink<CompilerHost>
{
    friend class Compiler;
    friend class CompilerPackage;

public:
    NEW_MUST_ZERO()

    CompilerHost(Compiler *pCompiler, IVbCompilerHost *pIVbCompilerHost);

    ~CompilerHost();

    // Return the COM+ project for this host.
    CompilerProject *GetComPlusProject()
    {
        return m_pComPlusProject;
    }

    void SetComPlusProject( _In_ CompilerProject *pProject )
    {
        ThrowIfNull(pProject);
        VSASSERT(!m_pComPlusProject, "Duplicate ComPlus project set");
        m_pComPlusProject = pProject;
        m_pComPlusProject->AddRef();
    }

    HRESULT LoadDefaultVBRuntimeLibrary
    (
        CompilerProject * pCompilerProject
    );


    HRESULT LoadSpecifiedVBRuntimeLibrary
    (
        CompilerProject * pCompilerProject,
        LPCWSTR wzsVBRuntimPath
    );

    // Returns the list of projects to compile for this host.
    CSingleList<CompilerProject> *GetProjectList()
    {
        return &m_slProjects;
    }

    Compiler *GetCompiler()
    {
        return m_pCompiler;
    }

    // OpenScope wrapper
    HRESULT OpenScope(IMetaDataDispenserEx **ppimdDataDispEx, LPCWSTR szFileName, DWORD dwOpenFlags, REFIID riid, IUnknown **ppIUnk);

    RuntimeVersion GetRuntimeVersion()
    {
        return m_RuntimeVersion;
    }

    HRESULT SetRuntimeVersion(CompilerProject *pMSCorLibHost);
    void AutoSetVbLibraryType();

    NamespaceRingTree *GetMergedNamespaceCache()
    {
        return &m_MergedNamespaceCache;
    }

    void ClearLookupCaches();

    // Add a project to the list of projects to compile.
    void InsertProject(CompilerProject *pProject);

    // Remove a project from the list of projects to compile.
    void RemoveProject(CompilerProject *pProject);

    bool IsStarliteHost()
    {
        return (m_PlatformType == TLB_Starlite);
    }

    // An ordered list of projects to compile.
    // When accessing the count of projects from the foreground thread be
    // sure to use the thread "safe" GetProjectsToCompileCount method.
    DynamicArray<CompilerProject *> *GetProjectsToCompile()
    {
        return &m_daProjectsToCompile;
    }

#if IDE 
    long GetProjectsToCompileCount();
    bool TryBindCoreLibraries();

    // Helpers to track compilation progress during a WaitUntilState
    // These helpers are not locked because the setters are only called from
    // the background thread. With the exception of ResetCompilationStepCount.
    // When ResetCompilationStepCount is called the background thread is stopped.
    ULONG GetTotalStepsToCompilePreferredProject() { return m_StepsToCompilePreferredProject; }
    void SetTotalStepsToCompilePreferredProject(ULONG steps) { m_StepsToCompilePreferredProject = steps; }
    void IncrementCompilationStepCount() { m_CurrentCompilationStepCount++; }
    void ResetCompilationStepCount() { m_CurrentCompilationStepCount = 0; }
    ULONG GetCurrentCompilationStepCount() { return m_CurrentCompilationStepCount; }

    // Helpers that track what the WaitUntilState is waiting for.
    CompilerProject* GetProjectToWaitOn() { return m_pProjectToWaitOn; }

    void SetProjectToWaitOn(_In_opt_ CompilerProject *pProjectToWaitOn) 
    {  
        m_pProjectToWaitOn = pProjectToWaitOn; 
        SetPreferredSourceFile(NULL);
    }

    SourceFile* GetPreferredSourceFile() 
    { 
        return m_pPreferredSourceFile; 
    }

    void SetPreferredSourceFile(_In_opt_ SourceFile* pPreferredSourceFile) 
    { 
        m_pPreferredSourceFile = pPreferredSourceFile; 
    }
    
    CompilationState GetProjectCompilationStateToWaitOn() { return m_csProjectToWaitOn; }
    void SetProjectCompilationStateToWaitOn(CompilationState csProjectToWaitOn) { m_csProjectToWaitOn = csProjectToWaitOn; }

    // Called whenever we want to optimize the background compilation to speed up IDE responsiveness.
    void OptimizeProjectCompilationListHelper(DynamicArray<CompilerProject *> *pPreferredProjects, CompilationState csDesiredState, _In_opt_ SourceFile * pPreferredSourceFile = NULL);
    void RebuildProjectCompilationList(DynamicArray<CompilerProject *> *pPreferredProjects);
    void CalculateStepsToCompilePreferredProject();

#endif

    SolutionExtensionData *GetSolutionExtension(SolutionExtensionEnum extension)
    {
        return &m_rgSolutionExtension[extension];
    }

    void SetSolutionExtension(void *pCode, DWORD dwLen, SolutionExtensionEnum extension);

    void FreeSolutionExtension();

    // Resets the list of projects to compile. This should be done when references change or when projects
    // are added or deleted to/from the solution.
    bool DestroyProjectCompilationList()
    {
        CompilerIdeLock lock(m_csProjects);

        bool WasProjectListBuilt = m_daProjectsToCompile.Count() > 0;

        // Destroy old entries in the array.
        m_daProjectsToCompile.Collapse();

        return WasProjectListBuilt;
    }

    // Returns metadata project (if any) that's backed by the given file
    CompilerProject *FindMetaDataProjectForFile(LPCWSTR wszFileName);

    // Find an existing project.
#if IDE 
    CompilerProject *FindProject(_In_opt_z_ STRING *pstrFileName, bool fMetaDataProject, IVsHierarchy *pIVsHierarchy);
#else
    CompilerProject *FindProject(_In_opt_z_ STRING *pstrFileName, bool fMetaDataProject);
#endif IDE
    CompilerProject *FindProjectWithSameNamePKTokenAndVersion(CompilerProject *pProject);

// Helpers for clearing the runtime symbol cache
#if IDE 
    void ClearRuntimeHelperSymbolCaches(CompilerProject *ProjectGoingDown);
#endif

    void ClearRuntimeHelperCaches();

    // List of standard libraries
    DynamicArray<STRING*> *GetStandardLibraries()
    {
#if !HOSTED
        VSASSERT(m_daStandardLibraries.Array() != NULL,
                "CompilerHost.InitStandardLibraryList has not been called.");
#endif
        return &m_daStandardLibraries;
    }

    bool IsDogfoodFeaturesEnabled();

    //========================================================================
    // Accessors for stored context.
    //========================================================================

    // Generic helper to get the metadata dispenser.
    HRESULT GetDispenser(IMetaDataDispenserEx ** ppMetaDataDispenser);

    static
    BCSYM *GetSymbolForQualifiedName
    (
        _In_z_ STRING *ClassName,
        CompilerProject *Project,
        Compiler *Compiler,
        CompilerHost *CompilerHost,
        CompilationCaches *CompilationCaches,
        ErrorTable *pErrorTable = NULL,
        Location *pLocation = NULL
    );

    static
    BCSYM_NamedRoot *CompilerHost::GetSymbolForRuntimeClassCtor
    (
        BCSYM_Class *pClass,
        unsigned short usFlags,
        Vtypes vtypRet,
        Vtypes *vtypArgs,
        Compiler *pCompiler
    );
    
    // Returns true iff the given file is an assembly. Note that
    // 'false' could mean either that the file is a code module
    // or that the file name is just bogus!
    bool IsFileAnAssembly(_In_z_ STRING *pstrFileName);

    // Returns the symbol for the specified runtime class.
    BCSYM_Container *GetSymbolForRuntimeClass(
        RuntimeClasses Class, 
        CompilationCaches *pCompilationCaches, 
        bool *bIsDefaultVBRuntimeOrMSCorLib = NULL,
        ErrorTable *pErrorTable = NULL,
        Location *pLocation = NULL);
    
    BCSYM_NamedRoot *GetSymbolForRuntimeMember(
        RuntimeMembers Member, 
        CompilationCaches *pCompilationCaches,
        ErrorTable *pErrorTable = NULL,
        Location *pLocation = NULL);
    
    bool IsCachedRuntimeMember( _In_ Declaration* pDecl );

    // Check whether this Class matches the namespace Microsoft.VisualBasic and the class name Name.
    bool IsRuntimeType(_In_z_ STRING* Name, BCSYM_NamedRoot* Class);

    VBTargetLibraryType GetVbLibraryType()
    {
        return m_PlatformType;
    }

    void OutputString(_In_z_ const WCHAR *wszOutput);

    // Helper to print a formatted string.
    void Printf(_In_z_ const WCHAR *wszFmt, ...);

    // Get host SDK path or default CLR system directory
    HRESULT GetSdkPath(_Deref_out_opt_z_ STRING** pPath);

    void ClearRuntimeCallRemapTable()
    {
        memset(&m_RuntimeCallRemapTable, 0, sizeof(m_RuntimeCallRemapTable));
    }

    BCSYM_Proc *RemapRuntimeCall(BCSYM_Proc *RuntimeCall, CompilationCaches *pCompilationCaches);

    ProjectErrorCacheTree *GetProjectErrorCacheTree()
    {
        return &m_ProjectErrorCacheTree;
    }

    FX::FXSymbolProvider* GetFXSymbolProvider()
    {
        return &m_FXSymbolProvider;
    }

    IVbCompilerHost* GetHostInterface() const
    {
        return m_pIVbCompilerHost;
    }

#if IDE 
    unsigned GetDebuggingProjectCount()
    {
        return m_DebuggingProjectCount;
    }

    void IncrementDebuggingProjectCount()
    {
        m_DebuggingProjectCount++;
    }

    void DecrementDebuggingProjectCount()
    {
        AssertIfFalse(m_DebuggingProjectCount > 0);

        m_DebuggingProjectCount--;
    }

    bool IsDebuggingInProgress()
    {
        return m_IsDebuggingInProgress;
    }

    void SetDebuggingInProgress(unsigned DebuggingInProgress)
    {
        m_IsDebuggingInProgress = DebuggingInProgress;
    }
#endif IDE

    void SetDefaultVBRuntimeProject(CompilerProject* pCompilerProject)
    {
        ThrowIfNull(pCompilerProject);
        VSASSERT(!m_pDefaultVBRuntimeProject, "Duplicate VB Runtime project set");
        RELEASE(m_pDefaultVBRuntimeProject);
        m_pDefaultVBRuntimeProject = pCompilerProject;
        m_pDefaultVBRuntimeProject->AddRef();
    }    

    CompilerProject* GetDefaultVBRuntimeProject()
    {
        return m_pDefaultVBRuntimeProject;
    }    


private:
    BCSYM_Proc * GetRuntimeCall(MappedMember Member)
    {
        if (0 <= Member && Member < _countof(m_RuntimeCallRemapTable))
        {
            return m_RuntimeCallRemapTable[Member];
        }
        return NULL;
    }

    // Stores the symbol for a given runtime helper into the remap table
    void SetRuntimeCall(MappedMember Member, BCSYM_NamedRoot *Symbol)
    {
        if (0 <= Member && Member < _countof(m_RuntimeCallRemapTable))
        {
            // Check for NULL - Watson bug DevDiv #91626.  Probably happens when they compile without the fx setup correctly.
            // But it is also goodness for compiler agility because this gets called by the symbol remapper, and who knows if the
            // targeted runtime has the old helpers.
            if ( Symbol != NULL)
            {
                VSASSERT(Symbol->IsProc() == true, "You can't send a non-proc into SetRuntimeCall()");
                m_RuntimeCallRemapTable[Member] = Symbol->PProc();
            }
        }
    }

    void PrepareRemappingTable(CompilationCaches *pCompilationCaches);

    // The COM+ project.
    CompilerProject *m_pComPlusProject;

    // The Default VB runtime project. 
    // In vbc, it is used to cache default vb runtime, which is only loaded once for all project with default runtime option.
    // In VBHosted or debuuger, if project being compiled is not defined then uses m_pDefaultVBRuntimeProject as vb runtime.
    CompilerProject *m_pDefaultVBRuntimeProject;

    // The compiler host interface. A callback to the one who created us.
    IVbCompilerHost *m_pIVbCompilerHost;

    // COM+ instance.
    IMetaDataDispenserEx *m_pMetaDataDispenser;

    // UV Support: under debug store the target library type.
    VBTargetLibraryType m_PlatformType;

    // The list of projects we want to compile and the
    // order we're going to compile them in.
    //
    DynamicArray<CompilerProject *> m_daProjectsToCompile;

#if IDE 
    ULONG m_StepsToCompilePreferredProject;
    ULONG m_CurrentCompilationStepCount;
    CompilerProject* m_pProjectToWaitOn;
    SourceFile * m_pPreferredSourceFile;
    CompilationState m_csProjectToWaitOn;
#endif

    // List of loaded projects.
    CSingleList<CompilerProject> m_slProjects;

#if IDE 
    // Count of projects on which StartDebuggingPE has been invoked.
    unsigned m_DebuggingProjectCount;

    // Used to detect that debugging of at least some projects in this
    // compiler host is in progress. This is separate from m_DebuggingProjectCount
    // to avoid threading/locking issues.
    bool m_IsDebuggingInProgress;
#endif IDE

    // A table for holding symbols of versioned VB runtime functions.
    BCSYM_Proc *m_RuntimeCallRemapTable[MappedMemberMax];

    // A cache of runtime helpers (speeds up GetSymbolForRuntimeMember)
    BCSYM_NamedRoot * m_RuntimeMemberSymbolCache[ MaxRuntimeMember ];
    // A cache of runtime classes (speeds up GetSymbolForRuntimeClass)
    BCSYM_Container * m_RuntimeClassSymbolCache[ MaxRuntimeClass ];

    // The runtime helper symbol cache keeps track of runtime helper symbols.  But for
    // alternate vb runtimes, not all of the symbols may be defined.  In the case of /vbruntime-,
    // none are.  So this helps us to know on a cache miss whether it is worth trying to bind the symbol, or not.
    // I'm only looking at classes here (64 or so members) rather than methods (300+ members)
    // It is more likely that classes will be wholesale missing than a method here or there on a class
    // will be missing.  Saves memory for the more common case where we have all runtime members
    // defined and makes it faster for us to know when to bail looking for the symbol
    RuntimeSymbolCacheStatus m_RuntimeClassSymbolCacheStatus[ MaxRuntimeClass ];

#if IDE 
    // We need to know which project contributed symbols to the cache so that when that project goes
    // down, we can clear the related symbols from the cache.
    CompilerProject * m_ListOfProjectsContributingRuntimeHelpers[ MaxRuntimeLibrary ];
#endif

    // template vb source files loaded from microsoft.visualbasic.dll
    // each vb source file takes a slot and source code is shared by
    // all system sourcefile object in all vb projects in this host
    SolutionExtensionData m_rgSolutionExtension[SolutionExtension::MaxExtension + 1];

    // Standard libraries
    DynamicArray<STRING*> m_daStandardLibraries;
    HRESULT InitStandardLibraryList();

    Compiler *m_pCompiler;

    RuntimeVersion m_RuntimeVersion;
    bool m_IsAutoLibraryType;

    ProjectErrorCacheTree m_ProjectErrorCacheTree;
    NamespaceRingTree m_MergedNamespaceCache;
    NorlsAllocator m_nrlsCompilationCache;

    FX::FXSymbolProvider m_FXSymbolProvider;
    CompilerIdeCriticalSection m_csProjects;

    TriState<bool> m_EnableFeatures;
};

//****************************************************************************
// An instance of the VB compiler.  This class will be inherited by
// CompilerPackage when loaded in the IDE.
//****************************************************************************

class Compiler :
     public IVbCompiler
#if IDE 
     , public ThreadSyncManager
#endif
#if FV_DEADBEEF
    , public Deadbeef<Compiler> // Must be last base class!
#endif
{
public:
    NEW_MUST_ZERO()

    //========================================================================
    // Creation/Destruction.
    //========================================================================

    Compiler(
        bool CompilingTheVBRuntime,
        LoadUICallback * pfnLoadUI,
        IVbCompilerHost * pCompilerHost);

    virtual
        ~Compiler();

    //========================================================================
    // IUnknown
    //========================================================================

    // We don't use ATL for this object because it makes debugging very hard

    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    //========================================================================
    // IVbCompiler - only these method are exposed outside of the compiler.
    //========================================================================

    // Controller methods.
    STDMETHOD(CreateProject)(LPCWSTR wszFileName, IUnknown *punkProject, IVsHierarchy *pHier, IVbCompilerHost *pVbCompilerHost, IVbCompilerProject **ppCompiler);
    STDMETHOD(Compile)(ULONG *pcWarnings = NULL, ULONG *pcErrors = NULL, struct VbError **ppErrors = NULL);
    // This does not belong to IVbCompiler. Since our expression valuator needs to initialize mscorlib.dll and
    // microsoft.visualbasic.dll in cpde.dll thread, we should be able to build symbols.
    STDMETHOD(CompileToBound)(CompilerHost *pCompilerHost, ULONG *pcWarnings = NULL, ULONG *pcErrors = NULL, struct VbError **ppErrors = NULL);

    // Compile switches.
    STDMETHOD(SetOutputLevel)(OUTPUT_LEVEL OutputLevel);
    STDMETHOD(SetDebugSwitches)(BOOL dbgSwitches[]);

    // Other methods.
    STDMETHOD(IsValidIdentifier)(_In_z_ WCHAR *wszIdentifer, BOOL *pfValid);

    // Registers a IVbCompilerHost with the Compiler. If this host hasn't been seen already, then one will be created.
    STDMETHOD(RegisterVbCompilerHost)(IVbCompilerHost *pVbCompilerHost);

    // Sets Watson behavior
    STDMETHOD(SetWatsonType)(WATSON_TYPE WatsonType, LCID WatsonLcid, _In_opt_z_ WCHAR *wszAdditionalFiles);

    STDMETHOD(StopBackgroundCompiler)() { return E_NOTIMPL; }

    STDMETHOD(StartBackgroundCompiler)() { return E_NOTIMPL; }

    STDMETHOD(SetLoggingOptions)(DWORD options);

    // Find a CompilerHost associated with pIVbCompilerHost. Returns true if found or false if not.
    bool FindCompilerHost(IVbCompilerHost *pIVbCompilerHost, CompilerHost **ppCompilerHost);
    bool UnregisterVbCompilerHost(_In_ IVbCompilerHost *pVbCompilerHost);
    bool CompareCompilerHosts(_In_ IVbCompilerHost *pCompilerHost1, _In_ IVbCompilerHost *pCompilerHost2);

    // Creates a new CompilerHost and adds it to the list of CompilerHosts that the Compiler knows about.
    HRESULT CreateNewCompilerHost(IVbCompilerHost *pIVbCompilerHost);

    // Helper for CreateProject
    STDMETHOD(CreateProjectInternal)(
        _In_ LPCWSTR wszFileName, 
        _In_ IUnknown *punkProject, 
        _In_ IVsHierarchy *pHier, 
        _In_ IVbCompilerHost *pVbCompilerHost, 
        _Out_ IVbCompilerProject **ppCompiler,
        _In_ bool isExpressionEditorProject);

    // Fusion APIs to compare assembly identities.
    typedef HRESULT (__stdcall *PfnCompareAssemblyIdentity)(LPCWSTR pwzAssemblyIdentity1, BOOL fUnified1, LPCWSTR pwzAssemblyIdentity2, BOOL fUnified2, BOOL *pfEquivalent, AssemblyComparisonResult *pResult);
    typedef HRESULT (__stdcall *PfnCreateAssemblyNameObject)(LPASSEMBLYNAME *ppAssemblyNameObj, LPCWSTR szAssemblyName, DWORD dwFlags, LPVOID pvReserved);


    //========================================================================
    // Project management.
    //========================================================================

#if IDE 
    CompilerProject *FindProjectById(LONG projectId);
#endif IDE

#if IDE 
    virtual CompilerPackage *AsCompilerPackage()
    {
        return NULL;
    }
#endif

    void SetProjectBeingCompiled(CompilerProject *pProjectBeingCompiled);

    // Return the project that's currently being compiled
    CompilerProject *GetProjectBeingCompiled();

    //========================================================================
    // Compilation stats.
    //========================================================================

    void AddErrorsAndWarnings(ULONG cErrors, ULONG cWarnings)
    {
        m_cErrors += cErrors;
        m_cWarnings += cWarnings;
    }

    //========================================================================
    // Accessors for stored context.
    //========================================================================

    // Obtains and returns the satellite DLL that contains our error message resources.
    static HINSTANCE GetResourceDll();
    static void LoadSolutionExtension(SolutionExtensionEnum extension, HMODULE hmodResources, WORD wResourceId, CompilerHost *pCompilerHost);
    static void LoadXmlSolutionExtension(CompilerHost *pCompilerHost);
    static void LoadVBCoreSolutionExtension(CompilerHost *pCompilerHost);

    //========================================================================
    // Implementation.
    //========================================================================

    // Commit all changes to the metadata.
    virtual HRESULT CommitChanges(void *pImage, CompilerProject *pCompilerProject);

    OUTPUT_LEVEL OutputLevel()
    {
        return m_OutputLevel;
    }

    // Returns the list of CompilerHosts that have been registered.
    CSingleList<CompilerHost>  *GetCompilerHosts()
    {
        return &m_CompilerHostsList;
    }

    // Returns the first CompilerHost which was created, or if none exist, creates and returns a Desktop host.
    CompilerHost *GetDefaultCompilerHost();

    // Create a compiler project for a metadata file.
    HRESULT CreateProjectWithMetaData(_In_z_ STRING *pstrFileName, bool bAssembly, CompilerHost *pCompilerHost,
                bool *bCreated, bool bDefaultLibrary, CompilerProject **ppCreatedCompilerProject);

#if HOSTED
    HRESULT CreateTypeScopeProject(_In_z_ STRING *pstrFileName, CompilerHost *pCompilerHost, bool *pbCreated, CompilerProject **ppCreatedCompilerProject);
#endif

    //========================================================================
    // String pool accessors.
    //========================================================================

    StringPool* GetStringPool()
    {
        return m_pStringPool;
    }

    StringPoolEntry AddStringPoolEntry(_In_opt_z_ const WCHAR *pwchar);
    STRING *AddString(StringBuffer *pString);
    STRING *AddString(_In_opt_z_ const WCHAR *pwchar);
    STRING *AddStringWithLen(_In_opt_count_(cch) const WCHAR *pwchar, size_t cch);
    STRING *LookupStringWithLen(_In_count_(cchSize) const WCHAR *pwchar, size_t cchSize, bool isCaseSensitive);

    STRING *ConcatStrings(_In_z_ const WCHAR *wsz1,
                          _In_z_ const WCHAR *wsz2,
                          _In_opt_z_ const WCHAR *wsz3 = NULL,
                          _In_opt_z_ const WCHAR *wsz4 = NULL,
                          _In_opt_z_ const WCHAR *wsz5 = NULL,
                          _In_opt_z_ const WCHAR *wsz6 = NULL);

    STRING *ConcatStringsArray(unsigned count,
                        _In_count_(count) const WCHAR **wsz,
                        _In_z_ const WCHAR *wszSeparator);

    // Helpers for dealing with qualified names
    unsigned CountQualifiedNames(_In_z_ const WCHAR *pstrName);
    void SplitQualifiedName(_In_z_ const WCHAR *pstrName, unsigned cNames, _Out_cap_(cNames) STRING **rgpstrNames);

    //========================================================================
    // Keyword accessors
    //========================================================================

    STRING *TokenToString(tokens tk)
    {
        return m_pStringPool->TokenToString(tk);
    }

    static tokens TokenOfString(_In_z_ STRING *pstr)
    {
        return (tokens)StringPool::TokenOfString(pstr);
    }

    STRING *OperatorToString(UserDefinedOperators op) const;

    STRING *GetOperatorCLSName(UserDefinedOperators op) const
    {
        VSASSERT(OperatorUNDEF < op && op < OperatorMAX && op != OperatorMAXVALID, "invalid index");

        if (0 <= op && op < _countof(m_OperatorCLSNames))
        {
            return m_OperatorCLSNames[op];
        }

        return NULL;
    }

    STRING *GetStringConstant(unsigned iString)
    {
        return m_pStringPool->GetStringConstant(iString);
    }

    //========================================================================
    // Well-known functions
    //========================================================================

    bool IsRuntimeFunctionCall(ILTree::PILNode ptree);

    RuntimeFunction WhichRuntimeFunctionCall(ILTree::PILNode ptree);


    //========================================================================
    // Misc context
    //========================================================================

    STRING *GetRuntimeClassName(unsigned iClass)
    {
        if (iClass < _countof(m_rgpstrRuntimeClasses))
        {
            return m_rgpstrRuntimeClasses[iClass];
        }

        return NULL;
    }

    STRING *GetRuntimeLibraryName(unsigned iLibrary)
    {
        if (iLibrary < _countof(m_rgpstrRuntimeLibraries))
        {
            return m_rgpstrRuntimeLibraries[iLibrary];
        }

        return NULL;
    }

    // Get the default or "null" namespace for a particular file.
    // If the namespace does not currently exist,
    // create it with the indicated number of hash buckets.
    BCSYM_Namespace *GetUnnamedNamespace(CompilerFile *pFile, unsigned cHashBuckets = 0, unsigned cUnBindableHashBuckets = 0);

    // Get the default or "null" namespace for some file in a project.
    // This namespace is suitable for lookup (as all instances of a namespace are
    // equivalent), but not for insertion. This API can return NULL.
    BCSYM_Namespace *GetUnnamedNamespace(CompilerProject *pProject);

    // Get any the first of the default or "null" namespaces in the project.
    // This namespace is suitable for lookup (as all instances of a namespace are
    // equivalent), but not for insertion.
    BCSYM_Namespace *GetUnnamedNamespace();

    // GetNamespace is to be called to create a namespace.
    // AddNamespace is to be called when a Namespace instance is created.
    // RemoveNamespace is to be called when decompilation forces a Namespace
    // instance to go out of existence.

    bool NamespaceIsAvailable(CompilerProject *LookupProject, BCSYM_Namespace *NamespaceToTest);
    STRING *GetMatchedQualifiedName(_In_z_ STRING *QualifiedName, CompilerProject *LookupProject, BCSYM_NamespaceRing *NamespaceRing);
    STRING *GetMatchedQualifiedName(_In_z_ STRING *QualifiedName, CompilerProject *LookupProject);

    BCSYM_NamespaceRing *GetNamespaceRing(_In_z_ STRING *QualifiedName);
    BCSYM_Namespace *GetNamespace (_In_z_ STRING *QualifiedName, CompilerFile *pFile, Location *Loc, unsigned HashBuckets = 0, unsigned UnBindableHashBuckets = 0);
    void AddNamespace (BCSYM_Namespace *pNamespace);
    void RemoveNamespace (BCSYM_Namespace *pNamespace);

    // Get the System.Threading namespace, to use for looking up the helper
    // methods for SyncLock. This namespace can be used for lookup but not
    // for insertion. (Getting this namespace in the usual way creates
    // problems--a namespace object allocated for a particular file after that
    // file reaches bindable state gets trashed when the file is demoted to
    // bindable state.)

    BCSYM_Namespace *FindNamespace(_In_z_ STRING *NamespaceName);

    // Find a critical DLL.  This will cause the compiler to die if it is not found.
    HRESULT FindCriticalDllOnSDKPath(
        _In_z_ const WCHAR *wszDll,
        CompilerHost *pCompilerHost,
        _Deref_out_z_ STRING **ppstrFileName);

    // Find the file at the specified path
    bool FindFileInPath(
        _In_z_ const WCHAR *wszFileName,
        _In_opt_z_ const WCHAR *wszSearchPath,
        _Deref_out_z_ STRING **ppstrFullPath);


    // First time we hit the idle loop we start the background compiler if it hasn't
    // started already.
    void StartBackgroundThreadIfNeeded(bool fCalledFromIdleLoop = false);

    HRESULT CreateXMLDocument(REFIID riid, void **ppXMLDocument);

    // Note: this is overridden by VbeeCompiler.  If the method returns true, semantics 
    // will call into IsAccessible and NamespaceContainsAnAccessibleType
    virtual
    bool CanOverrideAccessibilityCheck()
    {
        return false;
    }
    // Note: this is overridden by VbeeCompiler 
    virtual 
    bool IsAccessible( _In_ Declaration* pDecl )
    {
        ThrowIfFalse(false);
        return false;
    }
    // Note: this is overridden by VbeeCompiler 
    virtual 
    bool NamespaceContainsAnAccessibleType ( _In_ BCSYM_Namespace * pNamespace )
    {
        ThrowIfFalse(false);
        return false;
    }
    // Note: this is overridden by VbeeCompiler.
    virtual
    bool IgnoreVolatileModifier()
    {
        return false;
    }

    // Note: these are overridden by VbeeCompiler so that the debugger, which uses us to import types from dlls, can 
    // always return TRUE so that all types are loaded.  The compiler can be more selective for perf reasons.
    virtual bool SkipTypeMember(_In_ STRING *pstrMemberName, _In_ DECLFLAGS MemberDeclFlags, 
                                _In_ CompilerProject *pProject, _In_ mdToken Token, _In_ MetaDataFile *pMetaDataFile );

    // Note: this is overridden by VbeeCompiler so that type resolution will
    // ignore assemblies and resolve WinRT types by name alone.
    virtual bool ResolveWinRTTypesByName()
    {
        return false;
    }


protected:

    // Load the default libraries.
    virtual HRESULT InitializeDefaultLibraries(CompilerHost *pCompilerHost)
    {
        return LoadDefaultLibraries(m_CompilingTheVBRuntime, pCompilerHost);
    }

    HRESULT LoadDefaultLibraries(bool CompilingTheVBRuntime, CompilerHost *pCompilerHost);

    // Structure that holds information required to detect and generate reference cycle
    // errors involving metadata to source project references.
    struct ProjectSortInfo
    {
        AssemblyRefInfo *pAssemblyRef;          // Will be NULL for references not originating
                                                // from metadata projects.
        CompilerProject *pReferencingProject;

        ProjectSortInfo *pPrev;
        ProjectSortInfo *pNext;
    };

    // Helpers to detect and generate reference cycle errors involving metadata to source
    // project references.
    void GenerateCycleDescIfReferenceCyclePresent(ProjectSortInfo *pSortInfo, CompilerProject *pProject);
    void GetRefCycleDescString(ProjectSortInfo *pStartSortInfo, ProjectSortInfo *pEndSortInfo, StringBuffer *pCycleDesc);

    // Applies a DFS on the tree of projects rooted with the project
    // passed in. This functions adds projects to the array of projects
    // (m_daProjectsToCompile) to be compiled in order such that referenced
    // projects are added before projects that reference them.
    // Topological sort of projects is very efficient and is much preffered
    // over any kind of sorting, for these graph type problems.
    //
    void TopologicalySortProjects
    (
        CompilerProject *pProject,
        DynamicArray<CompilerProject *> *pCurrentSetOfProjectsToCompile,
        bool fMetadataToSourceRefInProgress,
        ProjectSortInfo *pSortInfo
    );

public:
    
    // Determine the set of projects we need to build and
    // in what order we have to build them.  If the project
    // is not passed in, then compile all of the projects
    // in memory.
    //
    HRESULT BuildProjectCompilationList
    (
        CompilerProject *pProject,
        CompilerHost *pCompilerHost,
        DynamicArray<CompilerProject *> *pProjectsToCompile,
        bool BindAssemblyRefs,
        DynamicArray<CompilerProject *> *pPreferredProjects = NULL
    );

    CompilerIdeCriticalSection& GetMetaImportCritSec()
    {
        return m_csMetaImport;
    }
    
    void LockMetaImport()
    {
        m_csMetaImport.Lock();
    }
    void UnlockMetaImport()
    {
        m_csMetaImport.Unlock();
    }

    // Create an error list and print the errors.
    void BuildErrorList( struct VbError **ppErrors = NULL, unsigned *pcErrors = NULL, unsigned *pcWarnings = NULL );

    // Create an error list and print the errors for the given project.
    void BuildErrorListForOneProject
    (
        CompilerProject *pProject,
        DynamicArray<BCError> *pdaErrors,
        VbError **ppErrors = NULL,
        unsigned *pcErrors = NULL,
        unsigned *pcWarnings = NULL
    );

    // Create an error list and print the errors for the given project and the metadata projects it references.
    void BuildErrorListForProjectAndMetadataReferences
    (
        CompilerProject *pProject,
        VbError **ppErrors = NULL,
        unsigned *pcErrors = NULL,
        unsigned *pcWarnings = NULL
    );

    PfnCompareAssemblyIdentity GetCompareAssemblyIdentity()
    {
        return m_pfnCompareAssemblyIdentity;
    }

    PfnCreateAssemblyNameObject GetCreateAssemblyNameObject()
    {
         return m_pfnCreateAssemblyNameObject;
    }

    CLRTypeConstantEncoderDecoder *GetCLRTypeNameEncoderDecoder()
    {
        return &m_CLRTypeNameEncoderDecoder;
    }

#if IDE 
    CLRTypeConstantEncoderDecoder *GetCLRTypeNameEncoderDecoderIDE()
    {
        return &m_CLRTypeNameEncoderDecoderIDE;
    }

    bool IsCompilationInMainThread()
    {
        return m_fCompilationInMainThread;
    }

    void SetCompilationInMainThread(bool fInMainThread)
    {
        m_fCompilationInMainThread = fInMainThread;
    }
#endif

    //******************************************************************************
    // these methods have default implementation of nop in the command-line compiler
    // and contain logic in the IDE compiler
    //******************************************************************************
    virtual void FinishCompileStep() {}
    virtual void DoRefreshTaskList() {}
    virtual void _OnAssembly (VBCOLLECTION, CompilerProject * ) {}
    virtual void _OnProject (VBCOLLECTION, CompilerProject * ) {}
    virtual void OutputString(_In_z_ const WCHAR *wszOutput) {}
    virtual HRESULT WatchFile(_In_z_ STRING *pstrFileName) { return S_OK; }
    virtual HRESULT UnwatchFile(_In_z_ STRING *pstrFileName) { return S_OK; }
    virtual void RefreshCommentTokenList() {}


    // Dev10 #850039 
    // Accessor for hash set of names of "risky" functions from Microsoft.VisualBasic.FileSystem.
    HashSet<STRING*> & GetNamesOfRiskyMSVBFSFunctions()
    {
        return m_NamesOfRiskyMSVBFSFunctions;
    }
	
    bool IsCompilingTheVBRuntime() const
    {
        return m_CompilingTheVBRuntime;
    }

    // The method to call to get the resource DLL instance.
    static LoadUICallback *m_pfnLoadUIDll;

#if IDE
private:
    VbError* MoveBCErrorArrayToVbErrorArray(DynamicArray<BCError>* pdaErrors);
#endif

protected:
    // Refcount.
    ULONG m_cRefs;

    bool m_CompilingTheVBRuntime;

    // String pool.
    StringPool *m_pStringPool;

    // The resource DLL instnace.
    static HINSTANCE m_hinstResource;

    // The project currently being compiled
#if IDE 
    // obtained from VBTLSThreadData  In the IDE this must be per-thread. See comment in SetProjectBeingCompiled.
#else
    CompilerProject *m_pProjectBeingCompiled;
#endif

    // The list of CompilerHosts
    CSingleList<CompilerHost> m_CompilerHostsList;
    DynamicArray<CompilerHost *> m_daEECompilerHosts;

    CComBSTR m_bstrEECorPath;

#if DEBUG
    bool m_isProjectListLocked;
    unsigned m_cPassCount;
#endif DEBUG

    // Compilation statistics.
    unsigned m_cErrors;
    unsigned m_cWarnings;

    // Compilation switches.
    OUTPUT_LEVEL m_OutputLevel;

    // List of library calls that can be compiled out.
    STRING *m_pstrRuntimeFunction[RuntimeFunctionMax];

    // Operator name table.
    STRING *m_OperatorCLSNames[OperatorMAX+1];

    // Runtime class table.
    STRING *m_rgpstrRuntimeClasses[MaxRuntimeClass];
    STRING *m_rgpstrRuntimeLibraries[MaxRuntimeLibrary];

    // A table containing a NamespaceRing for every uniquely named namespace.
    BCSYM_Hash *m_pNamespaceRings;
    // Allocator for creating objects that live on the merged hashes on NamespaceRings.
    NorlsAllocator *m_pMergedHashAllocator;

    // Allocator for creating objects that live as long as the Compiler.
    NorlsAllocator *m_pEternalStorage;

    // Need for all compiler projects at various different stage of compilation, so it is
    // best that they all share this through the Compiler object.
    CLRTypeConstantEncoderDecoder m_CLRTypeNameEncoderDecoder;
#if IDE 
    CLRTypeConstantEncoderDecoder m_CLRTypeNameEncoderDecoderIDE;
#endif
    PfnCompareAssemblyIdentity  m_pfnCompareAssemblyIdentity;
    PfnCreateAssemblyNameObject m_pfnCreateAssemblyNameObject;

#if IDE 
    // Indicates that the compilation which by default occurs on the background thread is
    // now occuring in the main thread.
    bool m_fCompilationInMainThread;

    bool m_fBackgroundThreadNeedsCreating;
    bool m_fBackgroundThreadStoppedForReplaceAll;
#endif

    CComPtr<IClassFactory> m_srpXMLDOMFactory;
    CComAutoCriticalSection m_csXMLDOMFactory;
    CompilerIdeCriticalSection m_csMetaImport;

#if FV_TRACK_MEMORY && IDE
protected:
    Instrumentation m_instrumentation;
public:
    Instrumentation * GetInstrumentation();
#endif

    // Dev10 #850039 Hash set of names of "risky" functions from Microsoft.VisualBasic.FileSystem.
    HashSet<STRING*> m_NamesOfRiskyMSVBFSFunctions;

};

inline
StringPoolEntry Compiler::AddStringPoolEntry(_In_opt_z_ const WCHAR * pwchar)
{
    return m_pStringPool->AddStringPoolEntry(pwchar);
}

inline
STRING * Compiler::AddString(_In_opt_z_ const WCHAR * pwchar)
{
    return m_pStringPool->AddString(pwchar);
}

inline
STRING * Compiler::AddString(StringBuffer * pString)
{
    return m_pStringPool->AddString(pString);
}

inline
STRING * Compiler::AddStringWithLen(
    _In_opt_count_(cch)const WCHAR * pwchar,
    size_t cch)
{
    return m_pStringPool->AddStringWithLen(pwchar, cch);
}

inline
STRING * Compiler::LookupStringWithLen(
    _In_count_(cchSize)const WCHAR * pwchar,
    size_t cchSize,
    bool isCaseSensitive)
{
    return m_pStringPool->LookupStringWithLen(pwchar, cchSize, isCaseSensitive);
}
