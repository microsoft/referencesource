//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implements the file-level logic of the VB compiler.
//
//-------------------------------------------------------------------------------------------------

#pragma once


// If you make a change to this structure, please be sure to update
// IDEHelpers::IsBrowsableFile

DECLARE_ENUM(SolutionExtension)
    MyTemplateExtension,
    XmlHelperExtension,
    VBCoreExtension,
    MaxExtension = VBCoreExtension,
    SolutionExtensionNone = MaxExtension + 1
END_ENUM(SolutionExtension)



class SourceFile :
    public CompilerFile
#if FV_DEADBEEF
    , public Deadbeef<SourceFile> // Must be last base class!
#endif
{

public:
    //========================================================================
    // Creation methods.
    //========================================================================

    // Initialize a compilerfile that holds actual VB source code.
    static
    HRESULT Create(
        CompilerProject * pProject,
        _In_z_ const WCHAR * wszFileName,
        _In_opt_bytecount_(dwBufLen)const WCHAR * wszBuffer,
        _In_ DWORD dwBufLen,
        _In_ VSITEMID itemid,
        _Out_ SourceFile ** ppfile,
        _In_opt_ bool fWatchFile = true, // file system watcher
        _In_opt_ SolutionExtensionEnum nSolutionExtension = SolutionExtension::SolutionExtensionNone // if it's a My template 
        );

    NEW_MUST_ZERO()

    SourceFile(
        Compiler * pCompiler,
        CompilerProject * pProject);

    virtual
    ~SourceFile();

    void Invalidate(bool SkipDemoteNodesCheck = true);

    DynamicArray<ImportTrackerEntry> *GetImportTracker()
    {
        return m_pImportTracker;
    }

    ImportedTarget *GetExtraTrackedImport()
    {
        return m_pExtraTrackedImport;
    }

    bool ShouldSemanticsUseExtraImportForExtensionMethods()
    {
        return m_shouldSemanticsUseExtraImportForExtensionMethods;
    }

#if IDE
    void CompileMethodBodiesWithTracking(BCSYM_Container * Container);

    HRESULT RecompileWithImportTracking(
        ImportedTarget * pExtraTrackedImport,
        DynamicArray<ImportTrackerEntry> * pImportTracker,
        ULONG &BoundChangeIndex);


    HRESULT GetFileCodeModel(FileCodeModel **ppFileCodeModel);
    HRESULT GetInternalFileCodeModel(CFileCodeModel **ppFileCodeModel);

    void TransferFileCodeModelTo(SourceFile *pNewFile);
#endif

#if IDE 
    bool GetCurrentDeclTrees(
        NorlsAllocator * pAllocator,
        ParseTree::FileBlockStatement ** ppFileBlockStatement);

    bool HasParseErrors();

    ULONG GetCurrentDeclChangeIndex();
    ULONG GetCurrentBoundChangeIndex();
    ULONG GetCurrentBindingChangeIndex();
    void IncrementCurrentBindingChangeIndex();
    ULONG GetSourceChangeIndex();

    void SignalSourceChange();

    // Get the line marker table.
    LineMarkerTable *GetLineMarkerTable()
    {
        return &m_LineMarkerTable;
    }

    ParseTreeService* GetParseTreeService()
    {
        return m_pParseTreeService;
    }

    //  Remove the source file from the project linked list and release all if it's resources and its connection.
    //  Add it to a "debug" list that will ensure that the final release happens.
    //
    void ReleaseFileFromProject(bool SkipDemoteNodesCheck = false);

    // Rename the file from the project
    void RenameFileFromProject(LPCWSTR wszNewFileName);

    SourceFileView* GetSourceFileView()
    {
        return m_pSourceFileView;
    }

    void SetSourceFileView(SourceFileView * pSourceFileView)
    {
        CompilerIdeLock spLock(m_csView);
        VSASSERT( (m_pSourceFileView == NULL) || (pSourceFileView == NULL),
            "Only one view is supported" );

        m_pSourceFileView = pSourceFileView;
    }

    bool GetAlternateNavigationTarget(_In_z_ const WCHAR *wszRQName, BSTR *pbstrAlternateFilename, Location *plocAlternate);

    // Fires the OnBeforeNavigateToSymbol event and returns true if a client
    // handled the symbol navigation
    bool FireOnBeforeNavigateToSymbol(BCSYM_NamedRoot *pNamed);
    bool FireOnBeforeNavigateToSymbol(_In_z_ const WCHAR *wszRQName);

    //VS#246426
    bool QueryNavigateTo(
        long iStartLine,
        long iStartIndex,
        long iEndLine,
        long iEndIndex,
        bool fRemapInAspx = true);

    HRESULT NavigateTo(
        long iStartLine,
        long iStartIndex,
        long iEndLine,
        long iEndIndex,
        bool fRemapInAspx = true);

    //  Are there any view's to this file?
    //
    bool HasSourceFileView();

    static
    HRESULT FindSourceFileByMoniker(
        LPCOLESTR pszMkDocument,
        SourceFile * * ppSourceFile);

    static
    HRESULT FindSourceFileByMoniker(
        IVsHierarchy * pHier,
        LPCOLESTR pszMkDocument,
        SourceFile * * ppSourceFile);
#else
    LineMarkerTable *GetLineMarkerTable()
    {
        return NULL;
    }
#endif IDE


#if !IDE

    //========================================================================
    // Line Mapping methods ---
    //========================================================================

    // Adds information from a #ExternalSource( "filename", lineNo ) to the mapping table ( line mapping is zero based )
    void SetMappedLineInfo(ParseTree::ExternalSourceDirective * pMap);

    // Given a line in this source file, get its mapped location - note that LineToMap is zero based
    long GetMappedLineInfo(
        long LineToMap,
        bool * pShouldHaveLineDebugInfo,
        _Deref_opt_out_z_ STRING * * ppMappedFileName = NULL);

    // Get the file name for the code at the specified line - note that ForCodeAtLine is zero based
    STRING * GetMappedFileName(
        long ForCodeAtLine,
        bool * pShouldHaveDebug = NULL);
    bool IsMappedFile();

#endif

protected:

    friend class CompilerProject;
    friend class CompilerPackage;
    friend class PEBuilder;
    friend class CommitEditList;
    friend class BCSYM;

    //========================================================================
    // The following methods should only be called from the master
    // project compilation routines.  They should never be called directly.
    //========================================================================

    // Methods that move this file up states.
    virtual bool _StepToBuiltSymbols();
    virtual bool _StepToBoundSymbols();
    virtual bool _StepToCheckCLSCompliance();
    virtual bool _StepToEmitTypes();
    virtual bool _StepToEmitTypeMembers();
    virtual bool _StepToEmitMethodBodies();

    // invoked by _PromoteToBound to complete the task started by _StepToBoundSymbols()
    virtual void CompleteStepToBoundSymbols();

    //========================================================================
    // The following methods should only be called from the master
    // project decompilation routines and themselves.  They should
    // never be called directly.
    //========================================================================

#if IDE 

    // Methods that move a file down to a state.
    void _DemoteToTypesEmitted(bool IsRemoveDependencies);
    void _DemoteToBound();
    void _DemoteToDeclared(bool IsRemoveDependencies);
    void _DemoteToNoState();

    void _DemoteFromPossiblyIncompleteCompiledState();
    void _DemoteFromPossiblyIncompleteTypesEmittedState();

    void DemoteToDeclaredHelper(bool IsRemoveDependencies);

    void DiscardStaticLocalsLists();

    // Helper to send the file sub type to the project system so that file
    // sub type can be displayed
    bool DesignerAttributeChanged(LPCWSTR wszCategory);
    void RegisterFileSubType();

    static
    void ClearCLSComplianceState(BCSYM_Container * pContainer);

public:
    void ClearCLSComplianceState();

    bool IsCLSComplianceCheckingDone()
    {
        return m_IsCLSComplianceCheckingDone;
    }

    void SetIsCLSComplianceCheckingDone(bool IsCheckingDone)
    {
        m_IsCLSComplianceCheckingDone = IsCheckingDone;
    }

    HRESULT WaitUntilDeclaredState();

protected:
    HRESULT WaitUntilState(CompilationState csState, WaitStatePumpingEnum ShouldPump, WaitStateCancelEnum CanCancel);

#endif IDE


public:

    void SetFileContainsAssemblyKeyFileAttr(
        bool fContains,
        _In_opt_z_ STRING * pstrKeyFileName);

    void SetFileContainsAssemblyKeyNameAttr(
        bool fContains,
        _In_opt_z_ STRING * pstrKeyContainerName);

    void SetFileContainsAssemblyVersionAttr(
        bool fContains,
        _In_opt_z_ STRING * pstrVersion);

    STRING * GetKeyFileName()
    {
        return m_pstrKeyFileNameFromAttr;
    }

    STRING *GetKeyContainerName()
    {
        return m_pstrKeyContainerNameFromAttr;
    }

    TextFile *GetTextFile()
    {
        return &m_TextFile;
    }

    bool IsValidSourceFile()
    {
        return !(m_cs == CS_MAX);
    }

    //========================================================================
    // Helpers.
    //========================================================================

    //  Get the symbol that is defined at a location.
    HRESULT GetSymbolOfLocation(
        Location * ploc,
        BCSYM_NamedRoot * * ppsym);

#if DEBUG
    void DumpSymbolsHashTable();
    void DumpSymbolsHashTableStats();
#endif DEBUG

    //  Get the procedure symbol whose body contains a line
    HRESULT GetMethodContainingLine(
        long lLineNum,
        BCSYM_MethodImpl ** pMethodFound);
    
    bool DoesMethodImplContainLine(
        long lLine,
        BCSYM_MethodImpl * pMethodImpl);

    HRESULT ParseCodeBlock(
        Text * ptext,
        NorlsAllocator * pnra,
        ErrorTable * perrortable,
        BCSYM_Container * pConditionalCompilationConstants,
        const CodeBlockLocation * pCodeBlock,
        const CodeBlockLocation * pProcBlock,  // passed only when method def is requested (need sig location for codegen)
        ParseTree::Statement::Opcodes MethodBodyKind,
        ParseTree::MethodBodyStatement ** pptree,
        MethodDeclKind methodDeclKind);

    //  Get the Decl Trees for this file.
    HRESULT GetDeclTrees(
        NorlsAllocator * pnra,
        NorlsAllocator * pnraConditionalCompilaton,
        ErrorTable * pErrorTable,
        BCSYM_Container ** ppcontinerConditionalCompilaton,
        LineMarkerTable * LineMarkerTableForConditionals,
        ParseTree::FileBlockStatement ** ppbiltree);

    // Gather the parse trees for a method body.
    HRESULT GetUnboundMethodBodyTrees(
        BCSYM_Proc * pproc,
        NorlsAllocator * pnra,
        ErrorTable * perrortable,
        Text * ptext,  // may be NULL
        ParseTree::MethodBodyStatement ** pptree);

    // Gather the analyzed trees for a method body.
    HRESULT GetBoundMethodBodyTrees(
        BCSYM_Proc * pproc,
        NorlsAllocator * pnra,
        ErrorTable * perrortable,
        class Cycles * pcycles,
        CallGraph * pCallTracking,
        TransientSymbolStore * pCreatedDuringInterpretation,
        Text * ptext,  // may be NULL
        CompilationCaches * pCompilationCaches,
        DWORD dwFlags,
        ILTree::ILNode ** pptree,
        ParseTree::MethodBodyStatement ** ppUnboundTree = NULL,
        AlternateErrorTableGetter AltErrTablesForConstructor = NULL,
        DynamicArray<BCSYM_Variable *> * pENCMembersToRefresh = NULL);

    HRESULT GetBoundLambdaBodyTrees
    (
        _In_ ParseTree::LambdaBodyStatement *pUnbound,
        NorlsAllocator *pnra,
        ErrorTable *perrortable,
        TransientSymbolStore *pCreatedDuringInterpretation,
        CompilationCaches *pCompilationCaches,
        DWORD dwFlags,
        BCSYM_Class *pContainingClass,
        _Out_ ILTree::StatementLambdaBlock **pptree
    );

    // Compute MD5 hash for PDB check sum
    HRESULT GetCryptHash(
        void * pvHashValue,
        DWORD cbHashSize);

    //========================================================================
    // Accessors.
    //========================================================================

    CodeFile *GetCodeFile()
    {
        return &m_CodeFile;
    }

    XMLDocFile *GetXMLDocFile()
    {
        return &m_XMLDocFile;
    }

#if IDE 
    // Sets or Resets the state of the m_fNeedToFireOnStatementsChangedEvents flag.
    void SetNeedToFireOnStatementsChangedEvents(bool state)
    {
        m_fNeedToFireOnStatementsChangedEvents = state;
    }

    // Returns true if there is a pending change that requires firing the OnStatementsChanged event.
    bool IsNeedToFireOnStatementsChangedEvents()
    {
        return m_fNeedToFireOnStatementsChangedEvents;
    }

    void CopyMetaToken(NorlsAllocator * pnra);
    
    void ClearMetaToken() 
    {
         m_mdENCTokenHashTable.clear(); 
    }

    mdToken GetToken(const SymbolHash &hash)
    {
        mdToken tk;
        if ( !MultimapUtil::TryGetFirstValue(m_mdENCTokenHashTable, hash, __ref tk) )
        {
            tk = mdTokenNil;
        }

        return tk;
    }

    TokenHashTable& GetTokenHashTable()
    {
        return m_mdTokenHashTable;
    }

    TokenHashTable& GetTransientTokenHashTable()
    {
        return m_mdTransientTokenHashTable;
    }

#endif //IDE 

#if IDE 
    TokenHashTable& GetENCTokenHashTable()
    {
        return m_mdENCTokenHashTable;
    }

    void GetMethodLocation(
        ENCBuilder * pENCBuilder,
        bool fUpdate);
    
    void InitENCTemporaryManagers(NorlsAllocator *pnra);
    
    void CacheTemporaryManager(
        _In_ ENCBuilder * pENCBuilder,
        _In_ const SymbolHash &hash,
        _In_ BCSYM_Proc * pProc,
        _In_ Text * pText);

    void ResetTemporaryManagers() 
    {
        m_ENCTemporaryManagers.clear(); 
        m_ENCUsedTemporaryName.clear(); 
    }

    STRING * GetCachedTemporaryName(
        BCSYM_Proc * pProc,
        BCSYM * pType,
        LifetimeClass Lifetime,
        const Location * pTextSpan);

    bool IsCachedTemporaryName(
        BCSYM_Proc * pProc,
        _In_z_ STRING * psTempName);

    void CacheNewTemporaryName(
        BCSYM_Proc * pProc,
        _In_z_ STRING * psNewTempName);

    void GetENCTemporaryManagers( std::vector<TemporaryManager*>& managers );

#endif

    //************************************************************************
    // Text manipulation accessors.
    //************************************************************************

#if IDE 
    VSITEMID GetItemId() const
    {
        return m_itemid;
    }

    bool IsLinkedFile();
    bool IsLinkedFileSource();
    bool IsLinkedFileOrSource();


    IVsHierarchy *GetHierarchy();

    // The Get/SetHasGeneratedMetadata methods are used to
    // force decompilation of files that had their metadata
    // generation suppressed because of errors.  Without
    // this flag, some files might never be forced to go through
    // CS_Bound again, and we wouldn't get a second chance
    // to generate metadata for them.
    bool GetHasGeneratedMetadata()
    {
        return m_fGeneratedMetadata;
    }

    void SetHasGeneratedMetadata(bool fGeneratedMetadata)
    {
        m_fGeneratedMetadata = fGeneratedMetadata;
    }

    // The Get/SetHasGeneratedcode methods are directly analogous
    // to Get/SetHasGeneratedMetadata and are used in the same
    // way -- to force files back through CS_Compiled to give them
    // a chance to generate code that was suppressed the first time
    // around because of errors.
    bool GetHasGeneratedCode()
    {
        return m_fGeneratedCode;
    }

    void SetHasGeneratedCode(bool fGeneratedCode)
    {
        m_fGeneratedCode = fGeneratedCode;
    }

    // Indicates whether errors will be displayed in the task list.
    bool FShowErrorsInTaskList()
    {
        return m_fShowErrorsInTaskList;
    }

    // Determines whether errors will show up in the task list.
    // TRUE shows, FALSE hides.
    void SetShowErrorsInTaskList(bool fShow)
    {
        m_fShowErrorsInTaskList = fShow;
    }

    RefCountedPtr<CompilerIdeLock> LockView()
    {
        return RefCountedPtr<CompilerIdeLock>(new CompilerIdeLock(m_csView));
    }

    // Helper to determine the navigation column for a given line.
    long SourceFile::GetNavigationColumn(long iLine);
#endif IDE

    //************************************************************************
    // SymbolListHashTable update functions. These are used for updating the
    // Locations for hashed symbols.
    //************************************************************************

    // Add a symbol to the hash table
    void AddSymbolToHashTable(
        Location * pLoc,
        BCSYM_NamedRoot * SymbolToAdd);

#if IDE
    // Remove and return a Symbol from the hash table
    BCSYM_NamedRoot *RemoveSymbolFromHashTable(TrackedLocation *pLoc);
#endif IDE

    bool AreImportsBeingResolved()
    {
        return m_AreImportsBeingResolved;
    }

    void SetAreImportsBeingResolved(bool ImportsbeingResolved)
    {
        m_AreImportsBeingResolved = ImportsbeingResolved;
    }

    bool HaveImportsBeenResolved()
    {
        return m_HaveImportsBeenResolved;
    }

    void SetHaveImportsBeenResolved(bool ImportsResolved)
    {
        m_HaveImportsBeenResolved = ImportsResolved;
    }

    bool IsImportedExtensionMethodMetaDataLoaded()
    {
        return m_ImportedExtensionMethodMetaDataLoaded;
    }

    void SetImportedExtensionMethodMetaDataLoaded(bool MetaDataLoaded)
    {
        m_ImportedExtensionMethodMetaDataLoaded = MetaDataLoaded;
    }

    bool OptionStrictOffSeenOnFile()
    {
        return m_OptionStrictOffSeenOnFile;
    }
    void SetOptionStrictOffSeenOnFile(bool v)
    {
        m_OptionStrictOffSeenOnFile = v;
    }

#if IDE 

    // this could be true but the file might still not be completely bound
    // pending MyClass binding, etc.
    bool IsBindingContainersDone()
    {
        return m_BindingStatus_BindingContainersDone;
    }

    void SetBindingContainersDone()
    {
        VSASSERT( m_BindingStatus_Started,
                        "How can binding be done when it has not yet been started ?");
        m_BindingStatus_BindingContainersDone = true;
    }

#endif IDE

    bool DelayBindingCompletion()
    {
        return m_DelayBindingCompletion;
    }

    void SetDelayBindingCompletion(bool fDelayBindingCompletion)
    {
        m_DelayBindingCompletion = fDelayBindingCompletion;
    }

    ErrorTable *GetCurrentErrorTable()
    {
        VSASSERT( m_step <= CS_GeneratedCode,
                    "How can any error be generated after CS_GeneratedCode and where are they being merged ?");

        return &m_CurrentErrorTable;
    }

    static ErrorTable *GetCurrentErrorTableForFile(SourceFile *File)
    {
        return File->GetCurrentErrorTable();
    }

    void MergeCurrentErrors(
        bool fPurgeExistingErrorsWithThisStep = true,
        CompilationSteps Step = (CompilationSteps)(-1))
    {
        if (Step == -1)
        {
            // Merge to the next step
            //
            Step = (CompilationSteps)(m_step + 1);
        }

        VSASSERT( Step < CS_MAXSTEP,
                        "Why are errors being merged in this step ?");

        m_ErrorTable.Merge(&m_CurrentErrorTable, Step, fPurgeExistingErrorsWithThisStep);

        // Need to ensure that the error table is only purged here i.e. at the end of a
        // state (i.e. CS_Declared, CS_Bound, etc.) transition and nowhere else.
        // See Bug VSWhidbey 273696.
        //
        m_CurrentErrorTable.DeleteAll();
    }

    bool HasPartialTypes()
    {
        return m_HasPartialTypes;
    }

    void SetHasPartialTypes()
    {
        m_HasPartialTypes = true;
    }

    // RootNamespace can be NULL for non-existent files specified in project files (VisualStudio7 199349)
    // and in between the time when a file is removed but is still in the VB project (VSWhidbey 213239)
    BCSYM_Namespace *GetRootNamespace()
    {
        return m_RootNamespace;
    }

    void SetRootNamespace(BCSYM_Namespace *RootNamespace)
    {
        m_RootNamespace = RootNamespace;
    }

private:

    // Only declared.cpp should be able to get this.  Everything
    // else needs to work off the namespaces in the class
    // symbols.
    //
    friend class Declared;
    friend class CompilerFile;

    unsigned MapStateToIndex(CompilationState cs)
    {
        VSASSERT(cs >= CS_Declared, "Bad state.");

        return cs - CS_Declared;
    }

    CompilationState MapIndexToState(unsigned i)
    {
        return (CompilationState)(i + CS_Declared);
    }

#if DEBUG
    // This must be the first variable.
    unsigned m_DebFirstSourceFileVariable;
#endif DEBUG

    //
    // Source file information.
    //

#if !IDE // Used for external file line mapping
    ILTree::ExternalSourceDirective * m_pLineMappingTable; // The line mappings table
    unsigned m_NumMappedEntries; // Number of mapped entries we will have in the mapping table
#endif

#if IDE 

    // The Line Marker Table.
    LineMarkerTable m_LineMarkerTable;

    ParseTreeService* m_pParseTreeService;

    //  The item's id, may be NULL.
    VSITEMID m_itemid;

    SourceFileView* m_pSourceFileView;

    TokenHashTable m_mdENCTokenHashTable;
    std::map<SymbolHash, TemporaryManager *> m_ENCTemporaryManagers;
    std::multimap<TemporaryManager *, STRING *> m_ENCUsedTemporaryName;
    bool m_fCachingTemporaryNames;
#endif IDE

    // Used by GetSymbolOfLocation. 
    std::multimap<Location, BCSYM_NamedRoot*> m_SymbolListHashTable;

    // The file
    TextFile m_TextFile;

    // Where we allocated to when going to various states.
    NorlsMark m_nramarkDeclared;
    NorlsMark m_nramarkBound;
    NorlsMark m_nramarkTypesEmitted;

    CodeFile m_CodeFile;

    // Manages XMLDocs for this sourcefile.
    XMLDocFile m_XMLDocFile;

#if IDE 

    // Array of "extra" tokens we've built while compiling toward CS_TypesEmitted.
    // This is a support for EnC to preserve mdTokens of a field, a method, a class, and etc.
    // We can add/replace mdToken when PEBuilder::BuilderState != BS_EditAndContinue or ENCBuilder
    // defines a new member. The hash key, STRING_INFO *, is a qualified symbol name. If mdToken
    // represents a method, we will use its qualified name plus its signature to distinguish overloaded
    // methods.
    //
    TokenHashTable m_mdTokenHashTable;
    TokenHashTable m_mdTransientTokenHashTable;

    // True is we have a change pending change that requires firing the OnStatementsChanged
    // event.
    bool m_fNeedToFireOnStatementsChangedEvents;

    // Did we actually generate all of the metadata & code for this file,
    // or were we short-circuited by a compiler error?
    //
    bool m_fGeneratedMetadata;
    bool m_fGeneratedCode;

    // Whether we have verified if this file needs upgrading
    // or not.
    //
    bool m_isCurrentFileVersion;

    // The following data members are used to implement the
    // designer for a VB class module.
    //

    bool m_isClassDirty;

    // If FALSE, errors will not be propagated to the task list
    // for the given source file.  TRUE on construction.
    bool m_fShowErrorsInTaskList;

    // this could be true but the file might still not be completely bound
    // pending MyClass binding, etc.
    //
    bool m_BindingStatus_BindingContainersDone;

    // Indicate whether this file's compliance checking has been done
    //
    bool m_IsCLSComplianceCheckingDone;

    // Indicates that we are a linked file and registered with the IDE
    bool m_isLinkedAndRegistered;

#endif IDE

    // The strong name key file and key container name specified in assembly level attributes
    // specified in this file.
    STRING *m_pstrKeyFileNameFromAttr;
    STRING *m_pstrKeyContainerNameFromAttr;

    bool m_AreImportsBeingResolved;
    bool m_HaveImportsBeenResolved;
    bool m_DelayBindingCompletion;
    bool m_HasPartialTypes;
    bool m_OptionStrictOffSeenOnFile;

    // Indicates that extension method meta data for imports both source and project level have been loaded
    bool m_ImportedExtensionMethodMetaDataLoaded;    

    // This is a non-locking error table that is then merged after each
    // compilation step with the main locking error table on this source file.
    //
    ErrorTable m_CurrentErrorTable;

    // This is the namespace correspnding to the root namespace name. Note that
    // this is the same as the unnamed namespace if the root namespace name is an
    // empty string, else they are different.
    //
    BCSYM_Namespace *m_RootNamespace;
    CompilerIdeCriticalSection m_csView;

#if IDE 
    
    // CodeModel Objects
    CFileCodeModel *m_pFileCodeModel;

    // Track file changes.
    ULONG m_iSourceChangeIndex;

    // Tracks transitions to/from CS_NoState and CS_Declared
    ULONG m_iCurrentDeclChangeIndex;

    // Tracks transitions to/from CS_Bound and CS_Declared
    ULONG m_iCurrentBoundChangeIndex;

    // Tracks transitions from Declared to Bound and any decompilation
    ULONG m_iCurrentBindingChangeIndex;

    // Used to optimize decompilation from CS_Compiled to CS_TypesEmitted
    //
    bool m_NamedRootsMarkedBadAfterTypesEmitted;
    bool m_HasParseErrors;

    // Only BCSYM_NamedRoot should be able to get at the following
    friend class BCSYM_NamedRoot;

    bool NamedRootsMarkedBadAfterTypesEmitted()
    {
        return m_NamedRootsMarkedBadAfterTypesEmitted;
    }

    void SetNamedRootsMarkedBadAfterTypesEmitted(bool Value)
    {
        m_NamedRootsMarkedBadAfterTypesEmitted = Value;
    }


public:

    void SetShouldReRegisterFileSubType()
    {
        m_ShouldReRegisterFileSubType = true;
    }

private:

    bool m_ShouldReRegisterFileSubType;
    STRING *m_pstrLastSubTypeCategory;
#endif IDE

public:
    void SetExtensionAttributeApplication(BCSYM_ApplAttr * pExtensionAttributeApplication)
    {
        m_pExtensionAttributeApplication = pExtensionAttributeApplication;
    }

    BCSYM_ApplAttr * GetExtensionAttributeApplication()
    {
        return m_pExtensionAttributeApplication;
    }


#if DEBUG
    // This must be last!
    unsigned m_DebLastSourceFileVariable;
#endif DEBUG

    // ENC Processor.
#if IDE 
public:
    void InitENCProcessor( _In_ ENCBuilder* pBuilder );
    void InitENCProcessor(ENCProcessor *pENCProcessor);
    void StopDebugging();
    
    ENCProcessor * GetENCProcessor() 
    {
         return m_pENCProcessor; 
    }
    
    ENCSTATUS GetENCStatus();
    
    ENC_BREAKSTATE_REASON GetENCBreakReason() 
    {
         return m_encBreakReason; 
    }
    
    void BreakModeCommit( bool fCommitOtherFiles);
    
    void SetENCChange(
        bool fIsDirty,
        bool fIsRude);
    
    void SetENCChange( DBGMODE dbgmode );

    void ResetENCState(
        ENC_BREAKSTATE_REASON encBreakReason,
        bool isEnter);

protected:
    bool CheckENCCompilerError();

private:
    ENCProcessor *m_pENCProcessor;
    bool m_fENCChange;
    bool m_fENCRude;
    bool m_fENCEntered;
    ENC_BREAKSTATE_REASON m_encBreakReason;
#endif

    // Message tracking
#if IDE 
public:

    CompilerIdeCriticalSection& GetMessageCriticalSection() 
    {
         return m_MessageCS;
    }

    bool IsPosted_FileInDeclared() 
    {
         return m_IsPosted_FileInDeclared; 
    }
    void SetPosted_FileInDeclared(bool IsPosted) 
    {
         m_IsPosted_FileInDeclared = IsPosted;
    }

    bool IsPosted_FileInBindable() 
    {
         return m_IsPosted_FileInBindable; 
    }
    void SetPosted_FileInBindable(bool IsPosted) 
    {
         m_IsPosted_FileInBindable = IsPosted;
    }

    bool IsPosted_RegisterDesignViewAttribute() 
    {
         return m_IsPosted_RegisterDesignViewAttribute; 
    }
    void SetPosted_RegisterDesignViewAttribute(bool IsPosted) 
    {
         m_IsPosted_RegisterDesignViewAttribute = IsPosted;
    }

    STRING * GetDesignViewAttributeCategory() 
    {
         return m_pstrDesignViewAttributeCategory; 
    }
    void SetDesignViewAttributeCategory(_In_opt_z_ STRING * pstrDesignViewAttributeCategory) 
    {
         m_pstrDesignViewAttributeCategory = pstrDesignViewAttributeCategory; 
    }

private:
    CompilerIdeCriticalSection m_MessageCS;

    bool m_IsPosted_FileInDeclared;
    bool m_IsPosted_FileInBindable;
    bool m_IsPosted_RegisterDesignViewAttribute;

    STRING  *m_pstrDesignViewAttributeCategory;
#endif

    DynamicArray<ImportTrackerEntry> *m_pImportTracker;
    ImportedTarget                   *m_pExtraTrackedImport;
    bool m_shouldSemanticsUseExtraImportForExtensionMethods;

#if IDE

private:

    struct CompileErrorWithOriginalLocation
    {
        CompileError* m_pError;
        Location m_originalLocation;
    };

    struct OutOfProcVbcBuildErrorInfo
    {
        CompileErrorWithOriginalLocation m_errWithOrigLoc;
        bool m_fShowInTaskList;
    };

    static int _cdecl CompareOutOfProcVbcBuildErrorInfo(const void* pInfo1, const void* pInfo2);
    static int _cdecl CompareWinMDExpBuildError(const void* pError1, const void* pError2);
    static int CompareCompileErrorToOutOfProcVbcBuildErrorInfo(const CompileError* pError, const OutOfProcVbcBuildErrorInfo* pInfo);
    static int CompareOutOfProcBuildError(const CompileError* pError1, const CompileError* pError2);

    // Stores the list of out of proc build errors. Before using it, EnsureOutOfProcBuildErrorArrayUpToDate
    // should be called to make sure that the items are valid and sorted
    DynamicArray<OutOfProcVbcBuildErrorInfo> m_daOutOfProcVbcBuildErrors;
    DynamicArray<CompileErrorWithOriginalLocation> m_daWinMDExpBuildErrors;

    bool m_fNeedToUpdateOutOfProcBuildErrorArray;
    void EnsureOutOfProcBuildErrorArrayUpToDate();

    HRESULT AddOutOfProcBuildError(BSTR bstrErrorMessage, long errorId, VSTASKPRIORITY priority, long startLine, long startColumn, long endLine, long endColumn, _Out_ CompileErrorWithOriginalLocation& errWithOrigLoc);

public:

    HRESULT AddOutOfProcVbcBuildError(BSTR bstrErrorMessage, long errorId, VSTASKPRIORITY priority, long startLine, long startColumn, long endLine, long endColumn);
    HRESULT AddWinMDExpBuildError(BSTR bstrErrorMessage, long errorId, VSTASKPRIORITY priority, long startLine, long startColumn, long endLine, long endColumn);
    bool ClearOutOfProcBuildErrors();
    void SignalOutOfProcBuildErrorLocationsUpdated();
    bool HasOutOfProcBuildErrors();
    void GetVisibleErrorsConsideringOutOfProcBuildErrors(
        DynamicArray<CompileError *>* pdaErrorsToReport,
        DynamicArray<const WCHAR *>* pdaErrorMessages,
        DynamicArray<CompileError *>* pdaWMEErrorsToReport);
    void RevertOutOfProcBuildErrorLocations();
    void SaveOutOfProcBuildErrorLocations();

#endif IDE

    // File Load Error Handling
#if IDE 
public:

    bool HasFileLoadError() 
    {
         return m_HasFileLoadError; 
    }
    void SetHasFileLoadError(bool bHasError) 
    {
         m_HasFileLoadError = bHasError; 
    }
    bool IsAspx() 
    {
        return m_IsAspx;
    }
    void SetIsAspx(bool IsAspx) 
    {
        m_IsAspx = IsAspx;
    }

private:
    bool m_HasFileLoadError;
    bool m_IsAspx;

#endif //#if IDE 
    // End File Load Error Handling

#if IDE 
public:

    bool TryGetCachedSourceFileText(
        _In_ NorlsAllocator * pnra,
        __deref_out_ecount_opt(* pcchText) WCHAR * * pwszText,
        _Out_ size_t * pcchText);

    CComPtr<IVsTask> CacheText();

    static void ClearSourceFileCachedTextIfStale();

    IVxTextBuffer * GetTextBuffer() {
        return m_spTextBuffer; 
    }
    HRESULT  SetTextBuffer(IUnknown *pTextBuffer) 
    {
        HRESULT hr = E_INVALIDARG;
        if (pTextBuffer)
        {
            hr = pTextBuffer->QueryInterface(IID_IVxTextBuffer, (void**)&m_spTextBuffer);
            VSASSERT(m_spTextBuffer,"QI for IID_IVxTextBuffer failed");
        }
        return hr;
    }

#if VBC_NEWEDITOR
    void CreateTextBuffer
    (  
        _In_bytecount_(dwBufLen) const WCHAR *wszBuffer,             // in-memory buffer (NULL except in VBA case)
        DWORD dwBufLen                     // length of in-memory buffer
    )
    {
        HRESULT hr = S_OK;
        CComPtr<ITextBufferTest> pTextBufferTest;
        CComPtr<ICorRuntimeHost> spRuntimeHost;
        DWORD retval = 0;
        //hr = spRuntimeHost->ExecuteInDefaultAppDomain(L"C:\\Windows\\Microsoft.NET\\Framework\\v3.5\\Components\\Microsoft.VisualStudio.TextBuffer.dll",
        //                                                                                    L"Microsoft.VisualStudio.Text.Implementation.TextBufferFactory", 
        //                                                                                    L"GetBufferFactory",
        //                                                                                    L"textbuffer",
        //                                                                                    &retval);


        hr = VBCCreateManagedObject(L"C:\\Windows\\Microsoft.NET\\Framework\\v3.5\\Components\\Microsoft.VisualStudio.TextBuffer.dll", 
                                                                L"Microsoft.VisualStudio.Text.Implementation.TextBufferFactory", 
                                                                IID_ITextBufferTest,
                                                                (void**)&pTextBufferTest);

        if (SUCCEEDED(hr) && pTextBufferTest)
        {
            CComPtr<ITextBufferFactory> pTextBufferFactory;
            pTextBufferTest->GetTextBufferFactory(&pTextBufferFactory);
            pTextBufferFactory->CreateBuffer(L"dfdfd", NULL);
        }

    }
#endif

    IHostedCompilerFileNative *GetHostedCompilerFileNative()
    {
        return m_spHostedCompilerFileNative;
    }
    void  SetHostedCompilerFileNative(IUnknown *pHostedCompilerFileNative)
    {
        if (pHostedCompilerFileNative)
        {
            pHostedCompilerFileNative->QueryInterface(IID_IHostedCompilerFileNative, (void **)&m_spHostedCompilerFileNative);
        }
    }

    IHostedCompilerProjectNative * GetHostedCompilerProjectNative()
    {
        return m_spHostedCompilerProjectNative;
    }
    
    void SetHostedCompilerProjectNative(IHostedCompilerProjectNative *pHostedCompilerProjectNative)
    {
        if (pHostedCompilerProjectNative)
        {
            pHostedCompilerProjectNative->QueryInterface(IID_IHostedCompilerProjectNative, (void **)&m_spHostedCompilerProjectNative);
        }
    }
    
private:
    CComPtr<IVxTextBuffer> m_spTextBuffer;
    CComPtr<IHostedCompilerFileNative> m_spHostedCompilerFileNative;
    CComPtr<IHostedCompilerProjectNative> m_spHostedCompilerProjectNative;
#endif

private:
    BCSYM_ApplAttr * m_pExtensionAttributeApplication;

    // Friends.
    // A list of unprocessed friend declarations.
    DynamicArray< UnprocessedFriend > * m_pDaUnprocessedFriends;
    
};

int _cdecl SortSourceFileByName(
    const void * arg1,
    const void * arg2);
