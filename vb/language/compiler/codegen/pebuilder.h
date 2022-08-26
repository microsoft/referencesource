//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  PE builder - manages creating the final PE.
//
//-------------------------------------------------------------------------------------------------

#pragma once

struct ArrayValue;
struct PEInfo;
struct Text;
class  ErrorTable;
struct Resource;
struct BlockScope;
struct SecAttrInfo;

interface ISymUnmanagedWriter;
interface ISymUnmanagedReader;
interface ISymUnmanagedDocumentWriter;

class PEBuilder;

#if IDE
class ENCBuilder;
#endif
typedef FixedSizeHashTable<FIXED_SIZE_HASHTABLE_SIZE, mdTokenKey, mdToken> TokenMappingTable;
typedef HRESULT(__stdcall *TCLRCreateInstance)(REFCLSID clsid, REFIID riid, LPVOID *ppInterface);

// How big to make the hash tables?  We may want to make them dynamic.
#define HASH_TABLE_SIZE     256

// The length of a crypto key name, BC_" + 10 hex digits + "_" + 10 hex digits + NUL = 25 characters
#define KEY_CONTAINER_NAME_LENGTH   25

#if IDE 
typedef UINT64 SymbolHash;
#endif IDE

enum BuilderType
{
    BuilderType_PEBuilder,
    BuilderType_ENCBuilder
};

struct CodeGenInfo
{
    ILTree::ILNode * m_pBoundTree;
    BCSYM_Proc * m_pProc;
    SourceFile * m_pSourceFile;

    CodeGenInfo();
    CodeGenInfo(const CodeGenInfo & src);
    CodeGenInfo(ILTree::ILNode * pBoundTree, BCSYM_Proc * pProc, SourceFile * pSourceFile);
};

struct PDBForwardProcCacheNode : public RedBlackNodeBaseT<BCSYM_Namespace*>
{
    BCSYM_Proc *m_pForwardProc;
};

struct ProcCacheNodeOperations : public EmptyNodeOperationsT<PDBForwardProcCacheNode>
{
    void copy(PDBForwardProcCacheNode *pNodeDest, const PDBForwardProcCacheNode *pNodeSrc)
    {
        pNodeDest->m_pForwardProc = pNodeSrc->m_pForwardProc;
    }
};

typedef RedBlackTreeT
<
    BCSYM_Namespace *,
    SimpleKeyOperationsT<BCSYM_Namespace*>,
    PDBForwardProcCacheNode,
    ProcCacheNodeOperations
> PDBForwardProcCache;

struct ConsolidatedModulesNode :
    public RedBlackNodeBaseT<BCSYM_NamespaceRing *>
{

    ConsolidatedModulesNode() :
        Modules(NULL),
        cModules(0)
    {
    };

    BCSYM_Class **Modules;
    ULONG cModules;
};

struct ConsolidatedModulesNodeOperations :
    public EmptyNodeOperationsT<ConsolidatedModulesNode>
{
    void copy(ConsolidatedModulesNode *pNodeDest, const ConsolidatedModulesNode *pNodeSrc)
    {
        pNodeDest->Modules = pNodeSrc->Modules;
        pNodeDest->cModules = pNodeSrc->cModules;
    }
};

typedef RedBlackTreeT<
    BCSYM_NamespaceRing *,
    SimpleKeyOperationsT<BCSYM_NamespaceRing *>,
    ConsolidatedModulesNode,
    ConsolidatedModulesNodeOperations> ConsolidatedModulesTree;

//Factor out all data and code from PEBuilder so it will be shared by our
//EnCBuilder.
class Builder
{
public:
    bool IsPEBuilder() { return BuilderType_PEBuilder == m_BuilderType; }
    bool IsENCBuilder() { return BuilderType_ENCBuilder == m_BuilderType; }

#if IDE
    ENCBuilder *AsENCBuilder();
#endif IDE

    mdToken GetComClassToken(BCSYM_NamedRoot * pNamed);
    mdToken GetTypeDef(BCSYM_Container * pContainer);
    mdToken GetToken(BCSYM_NamedRoot * pNamed, bool remap = true);


    mdToken GetToken(BCSYM_Param *pParam, BCSYM_Proc *pContext);
    mdToken GetComClassEventsToken(BCSYM_Class * pClass);
    void SetToken(BCSYM_NamedRoot *pNamed, mdToken tk);
    void SetToken(BCSYM_Param *pParam, BCSYM_Proc *pContext, mdToken tk);
#if IDE 
    HRESULT GetSymbolHashKey(BCSYM_NamedRoot * pnamed, SymbolHash *pHash);
#endif

    IMetaDataAssemblyImport *GetAssemblyImport()
    {
        return m_pmdAssemblyImport;
    }

    HashTable<HASH_TABLE_SIZE> * GetProjectHashTable()
    {
        return &m_ProjectHashTable;
    }
    mdTypeRef * GetRTTypeRef()
    {
        return m_rgRTTypRef;
    }
    mdMemberRef * GetRTMemRef()
    {
        return m_rgRTMemRef;
    }

    ErrorTable * GetErrorTable()
    {
        return m_pErrorTable;
    }

protected:
    Builder(Compiler *pCompiler, CompilerProject *pProject);
    ~Builder() {}

    friend class MetaEmit;

    // Set up the cache for a file.
    void SetSourceFile(SourceFile *pSourceFile) { m_pSourceFile = pSourceFile; }

    // Returns true if we're building an assembly, false if building a module.
    bool FEmitAssembly();

    // Free all of the resources in the PE Builder.
    void ClearCaches();

#if IDE 
    // This function replaces m_pBuilder->m_pdaTokens->Add() or m_pdaTokens->Add()
    // to cache metadata tokens. In EnC, all cached tokens are the same as the tokens
    // we emitted to the debuggee process. In incremental compile, we will delete all
    // cached tokens.
    void CacheMetaToken(BCSYM_NamedRoot * pnamed, mdToken mdtk);
#endif

    // Helper to write Full/Delta PDB
    void WriteBlockScopes(_In_ BlockScope * CurrentScope, ISymUnmanagedWriter *pSymWriter, BCSYM_Proc *pProc, BCSYM_Container *pContainer, mdSignature SignatureToken);
    void WriteImportsLists(ISymUnmanagedWriter *pSymWriter, BCSYM_Proc *pProc, BCSYM_Container *pContainer);
    void WritePDBImportsList(_In_ ImportedTarget *pImportedTarget, ISymUnmanagedWriter *pSymWriter);
    void WriteNoPiaPdbList( _In_ ISymUnmanagedWriter* pSymWriter);

    // Clean up
    void Destroy();

    bool GetModulesInRing(
            _In_    BCSYM_NamespaceRing *pNamespaceRing,
            _Out_   BCSYM_Class ***pppModules,
            _Out_   ULONG *pcModules);
    void ConsolidateModulesForRing(
            _In_    BCSYM_NamespaceRing *pNamespaceRing,
            _Inout_ NorlsAllocator *pAllocator,
            _Inout_ ConsolidatedModulesNode *pConsolidatedModulesNode);

    BuilderType m_BuilderType;

    // Context.
    Compiler *m_pCompiler;

    // The project we're compiling.
    CompilerProject *m_pCompilerProject;

    // Where to store the hash table.  Can we share this anywhere?
    NorlsAllocator m_nra;

    // Table to put compile-time errors into.
    ErrorTable *m_pErrorTable;

    //
    // Project-wide compilation information.
    //

    // This hash table holds all of the "project-level" resources, including
    // the token arrays for each of the files we have already compiled.
    //
    HashTable<HASH_TABLE_SIZE> m_ProjectHashTable;

    // This hash table is used to keep track of the definition order and TypeRef tokens that we create for NoPia embedded 
    // types. 
    struct PiaTypeDefInfo
    {
        unsigned long m_DefinitionOrder;
        mdTypeRef m_TypeRef;
    };
    
    DynamicHashTable<BCSYM_Container*, PiaTypeDefInfo> m_NoPiaTypeDefToTypeRefMap;

    // The interface to which we will write the metadata.
    IMetaDataEmit2 *m_pmdEmit;

    IMetaDataAssemblyImport *m_pmdAssemblyImport;

    // Caches the tokens to runtime types and methods.
    mdTypeRef m_rgRTTypRef[MaxRuntimeClass];
    mdMemberRef m_rgRTMemRef[MaxRuntimeMember];

    //
    // Per-file compilation information
    //

    // The cache of the SourceFile that we're currently emitting.
    SourceFile *m_pSourceFile;

    PDBForwardProcCache *m_pPDBForwardProcCache;
    ConsolidatedModulesTree *m_pModulesCache;
    RefCountedPtr<TokenMappingTable> m_spTokenMappingTable;

#if IDE 
    bool m_bProcessingTransients;
#endif IDE
};

//=============================================================================
// PEBuilder
//
// Manages the process of building an EXE from the symbol table and bound
// trees of this project.  This class stores all of the information needed
// to write this information but "MetaEmit" is responsible for actually
// formatting the information and doing any verification.
//
// The builder manages two different pools of information:
//
//  1) project-level information - this is information that is shared
//     between all of the files and only gets deleted if all files
//     in the project decompile.  It includes all *Refs, any "resources"
//     that we add (i.e. strings, whatnot), module information, assembly
//     information and array specifications.
//
//  2) file-level information - essentially all defs and ComTypes. include
//     custom attributes.
//
// The builder works in three separate passes. This does not use any
// of the _PromoteTo mechanisms as we need to tightly control the order
// that the information is generated in.  It wil be used at the end to tell
// each file that it has successfully moved to a new state.
//
//  1) Walk all files not currently in compiled and add their containers to
//     the metadata.
//
//  2) Walk all of the containers in the same order that they were walked in
//     #1 and emit the definitions for all of their members.
//
//  3) Walk all of the members in any order and emit their method bodies.
//
// The above steps are all done in the background thread and written to
// a temporary PE which is copied to its final location by the
// foreground thread when we're asked to compile.
//=============================================================================

class PEBuilder : ZeroInit<PEBuilder>,
                  public Builder
{
public:
    friend class EndEmitGuard;
    friend class EndEmitGuardForWritePEHelper;
    friend class WritePEHelper;

    PEBuilder(Compiler *pCompiler, CompilerProject *pProject) : Builder(pCompiler, pProject)
    {
        m_BuilderType = BuilderType_PEBuilder;

#if IDE 
        ImageOnDiskIsNotUpToDate();
#endif IDE
    }

    ~PEBuilder();

    void Destroy();

    // Generate the type-defining metadata for the project.
    bool EmitTypeMetadata(ErrorTable *pErrorTable);

    // Compile everything in the project and write the PE.
    typedef enum _CompileType
    {
        CompileType_Full = 0,           // write the PE to disk
        CompileType_InMemory = 1,       // write the PE to memory
        CompileType_DeferWritePE = 2,   // don't write the full PE; need to call CompileToDisk later
        CompileType_FinishWritePE = 3,  // finish writing the PE and PDB; called from CompileToDisk
    } CompileType;

    bool Compile(CompileType type, _Out_ bool *pfGeneratedOutput, ErrorTable *pErrorTable, BYTE **ppImage);

#if IDE 
    SourceFile * OptimizeFileCompilationList(DynamicHashTable<CompilerFile *, bool> & filesToPrefer);

    void CompileToDisk(bool *pfGeneratedOutput, ErrorTable* pErrorTableProject);
    bool CompileToDiskAsync();
    bool NeedsCompileToDisk() const;
    void HandleDemoteToTypesEmitted();

    // Removes the metadata created for this file from MetaEmit.
    void RemoveFileMetadata(SourceFile *pSourceFile);

    // exe or dll on disk is not up-to-date.
    bool IsImageOnDiskUpToDate()
    {
        return m_IsImageOnDiskUpToDate;
    }

    void ImageOnDiskIsNotUpToDate()
    {
        m_IsImageOnDiskUpToDate = false;
    }

    TokenMappingTable *GetTokenMappingTable()
    {
        return m_spTokenMappingTable;
    }

#endif // IDE

    //Used by attr.cpp
    static const int c_cchNameMax = MAX_CLASS_NAME;

    // Loads alink.dll and creates an IALink2 interface
    static void CreateALink(IALink2 **ppIALink);
    static void CreateALink(IALink2 **ppIALink, IALink3 **ppIALink3);

    TransientSymbolStore * GetTransientSymbolStore()
    {
        return &m_Transients;
    }

protected:
    friend class MetaEmit;

    bool CheckConditionalAttributes(BCSYM_ApplAttr *AttributeApplication, SourceFile *SourceFile);

private:

    //========================================================================
    // Private implementation.
    //========================================================================

    // Helper to calculate a container's flags.
    DWORD GetTypeDefFlags(BCSYM_Container *pContainer);

    // Loads the metadata.
    void LoadMetaData();

    // Creates assembly/file refs for all projects referenced by the one being built
    void PreloadAssemblyRefCache();

    // Unloads the metadata without destroying any of our internal tables.
    void UnloadMetaData();

    // Define all of the types in a file.  You must do all of the outer-most
    // (non-nested) types first.
    //
    void DefineContainer(BCSYM_Container *pContainer);

    void DefineContainerProperties(BCSYM_Container *pContainer, _Inout_ MetaEmit *metaemitHelper);

    // Define the members of a container.  The cached file and its hash
    // table must be set up before this call.
    //
    void DefineMembersOfContainer(BCSYM_Container *pContainer, _Inout_ MetaEmit *metaemitHelper);

    void DefinePiaTypes(MetaEmit *metaemitHelper);
    void DefinePiaType(BCSYM_Container *pContainer, MetaEmit *metaemitHelper);
    void DefineMembersOfPiaType(BCSYM_Container *pContainer, _Inout_ MetaEmit *metaemitHelper);
    mdTypeRef* DefineInterfacesOfPiaType(BCSYM_Container *pContainer, MetaEmit *metaemitHelper);
    void EmitPiaTypeAttributes(BCSYM_Container *pContainer, mdToken tokenForSymbol, MetaEmit *metaemitHelper);
    void EmitPiaTypeMemberAttributes(BCSYM *psym, mdToken tokenForSymbol, MetaEmit *metaemitHelper);
    BCSYM_NamedRoot *GetSymbolForAttribute(_In_z_ WCHAR *name, Vtypes vtypArg[], MetaEmit *metaemitHelper);

    // Define one member of a container.  The cached file and its hash
    // table must be set up before this call.
    //
    void PEBuilder::DefineMemberOfContainer(BCSYM_NamedRoot *pMember, _Inout_ MetaEmit *pMetaemitHelper);

    // Define the com class interface and events interface for a given
    // container.
    //
    void DefineComClassInterfaces(BCSYM_Container *pContainer);

    // Define the members of the com class interfaces.
    void DefineMembersOfComClassInterfaces(BCSYM_Class *pClass, _Inout_ MetaEmit *metaemitHelper);

    // Emit DispId attributes for the members of the com class interfaces.
    void EmitComClassDispIdAttributes(BCSYM_Class *pClass, _Inout_ MetaEmit *metaemitHelper);

    // Emit all custom attributes explicitly attached to the given symbol.
    void EmitAttributesAttachedToSymbol(BCSYM *psym,
                                        mdToken tokenForSymbol,
                                        BCSYM_Proc *psymContext,
                                        _Inout_ MetaEmit *metaemitHelper,
                                        bool isNoPIaEmbeddedSymbol);

    // Emit all the security attributes applied in this project.
    void EmitAllSecurityAttributes();

    // Compile any methods in this container.  The cached file and its hash
    // tabe must be set up before this call.
    //
    bool CompileMethodsOfContainer(BCSYM_Container *pContainer,
                                   Text *pText,
                                   _Inout_ PEInfo *pInfo,
                                   _Inout_ MetaEmit *metaemitHelper);

    bool GenerateBoundTreesForContainer(BCSYM_Container * pContainer,
                                        Text * pText,
                                        _Inout_ PEInfo * pInfo,
                                        _Out_ MetaEmit * pMetaemitHelper,
                                        NorlsAllocator * pNraBoundTrees,
                                        _Inout_ DynamicArray<CodeGenInfo>* pCodeGenInfos);

    ILTree::ILNode *GenerateBoundTreeForProc(BCSYM_Proc *pProc,
                                      BCSYM_Container * pContainer,
                                      Text * pText,
                                      _Inout_ PEInfo * pInfo,
                                      MetaEmit * pMetaemitHelper,
                                      NorlsAllocator * pNraBoundTrees,
                                      _Inout_ DynamicArray<CodeGenInfo>* pCodeGenInfos,
                                      Cycles *pCycle,
                                      CallGraph *pCallTracking);

    bool GenerateCodeForContainer(_In_ PEInfo * pInfo,
                                  _Inout_ MetaEmit *pMetaemitHelper,
                                  _In_ DynamicArray<CodeGenInfo>* pCodeGenInfos);

    bool ProcessTransientSymbols(BCSYM_Container * pContainer,
                                 Text *pText,
                                 _Inout_ PEInfo * pInfo,
                                 _Inout_ MetaEmit *pMetaemitHelper,
                                 NorlsAllocator * pNraBoundTrees,
                                 _Inout_ DynamicArray<CodeGenInfo>* pCodeGenInfos);

    // Emit custom attributes attached to this proc and its params.
    //
    void EmitAttributesOnProcAndParams(BCSYM_Proc *pproc, _Inout_ MetaEmit *metaemitHelper, bool isNoPIaEmbeddedSymbol);

    // Emit custom attributes attached to this container or its contents
    //
    bool EmitAttributes(BCSYM_Container *pContainer,
                        _Inout_ PEInfo *pInfo,
                        _Inout_ MetaEmit *metaemitHelper);

    // Emits one COM+ resource.  Returns size of embedded resource (0 for linked).
    ULONG EmitResource(PEInfo *pInfo, Resource *presource, ErrorTable *pErrorTable);

    BCSYM_Proc *DetermineEntrypoint(_Out_ BCSYM_Proc **ppProcDebuggingEntryPoint);
    BCSYM_Proc *DetermineDebuggingEntrypoint(BCSYM_Proc *pProc);

    // Write out the PE for everything we've compiled.  Once this method is
    // called, the builder must be reinitialized before building another
    // PE.
    //
    void WritePE(_Inout_ PEInfo *pInfo, BYTE **ppImage, _Out_ bool *pfGenerated, _Inout_ MetaEmit *metaemitHelper);

    // Write the code blocks for this project to the PE generator.
    void EmitAllCodeBlocks(PEInfo *pInfo);

    void WriteDllCharacteristics(PEInfo *pInfo, WORD DllCharacteristics);

    // Write the code block for this proc to the PE generator.
    void EmitAllCodeBlocksForProc(BCSYM_Proc *pProc, PEInfo *pInfo);

    // Write the PDB.
    HRESULT WritePDB(_In_ PEInfo *pInfo, _Deref_out_ ISymUnmanagedWriter **ppWriter);
    void WritePDB2(_In_ PEInfo *pInfo, ISymUnmanagedWriter* pSymWriter);

    // Write the PDB info for this proc.
    HRESULT WritePDBInfoForProc
    (
        BCSYM_Proc *pProc,
        SourceFile *pSourceFile,
        ISymUnmanagedWriter *pSymWriter,
        ISymUnmanagedDocumentWriter *pDocumentWriter
    );

    HRESULT WritePDBInfoForTokenSourceSpans
    (
        _In_ SourceFile *pSourceFile,
        _In_ ISymUnmanagedWriter5 *pSymWriter5
    );

    HRESULT WritePDBInfoForTokenSourceSpans
    (
        _In_ BCSYM_NamedRoot *pSym,
        _In_ ISymUnmanagedWriter5 *pSymWriter5,
        _In_ SourceFile *pSourceFile
    );

    HRESULT GetPDBDocumentForFileName
    (
        _In_ ISymUnmanagedWriter* pSymWriter,
        _In_z_ STRING* fileName,
        _Out_ ISymUnmanagedDocumentWriter **ppDocument
    );

    void ReleasePDBDocuments();

    // Port SP1 CL 2922610 to VS10
    HRESULT WritePDBInfoForLambdas(SourceFile *pSourceFileBeingProcessed, ISymUnmanagedWriter *pSymWriter,
                                    ISymUnmanagedDocumentWriter *pDocumentWriter);

    // Create the pieces needed to emit an EXE.  EndEmit must always be called
    // even if this method throws an error.
    //
    void BeginEmit(_Out_ PEInfo *pInfo, CompileType compileType);

    // Create ALink object to help emit an EXE.  EndEmit releases this object.
    void CreateALinkForCompile(_Inout_ PEInfo *pInfo, CompileType compileType);

    // Allocate the memory to store the IL for a procedure.  This stores the
    // IL in a no-release allocator in the IDE and stores it directly in the
    // PE in the command-line compiler.
    //
    BYTE *AllocatePEStorageForImage(_In_ PEInfo *pInfo,
                                    BCSYM_Proc *pProc,
                                    unsigned cbImage);

    // Create the debug directory inside of the PE and have it point to
    // our PDB.
    //
    void CreateDebugDirectory(_In_ PEInfo *pInfo, ISymUnmanagedWriter *pSymWriter);

    // Deletes an output file from the disk.
    bool RemoveFile(_In_opt_z_ WCHAR *wszFileName, _In_opt_z_ WCHAR *wszExt = NULL);

    // We're done with the PE emitter.
    void EndEmit(_In_opt_ PEInfo *pInfo);

    bool CompleteCompilationTask(CompilerFile *pFile);

    void EmitCustomAttribute
    (
        BCSYM      * pSymbol,
        mdToken tokenForSymbol,
        BCSYM_ApplAttr * psymApplAttr,
        ErrorTable * pErrorTable,
        _Out_ MetaEmit * pMetaEmitHelper,
        NorlsAllocator * pAlloc,
        SecAttrInfo * pSecAttrInfo,
        BCSYM_NamedRoot * pApplContext
    );

    BCSYM_ApplAttr * FindFirstAttributeWithType
    (
        BCSYM_Param * pParam,
        BCSYM_NamedRoot * pAttributeType
    );

    static TCLRCreateInstance m_pMSCoreeCLRCreateInstance;

    static HRESULT CLRCreateInstance(REFCLSID clsid, REFIID riid, LPVOID *ppInterface);

#if IDE 
    // VSWhidbey[399336] This function will be used to stop emitting ENC Synth function into the metadata.
    bool IsENCSynthFunction(BCSYM_Proc *pProc);
#endif




private:

    //========================================================================
    // Datamembers.
    //========================================================================

    //
    // Metaemit has a very tight integration with this class, so we allow
    // it to party directly on our datastructures.  It builds the project-level
    // hash table, fills in the extra token array and puts the compiled
    // IL into the per-file no-release allocator.
    //

    // The count of files that are currently in Compiled state.  We will
    // free all of the resources held by this class whenever this count
    // hit zero.
    //
    unsigned m_cCompiledFiles;

    // The PE builder can be in one of four states:
    //
    enum BuilderState
    {
        // The builder contains no information, rebuild the PE
        // from scratch.
        //
        BS_Empty,

        // We have finished building the metadata for this PE.  Any other
        // edits will just modify it.
        //
        BS_IncrementalCompile,
    };

    BuilderState m_State;

    // Symbols created during method body compilation
    TransientSymbolStore m_Transients;

    // Methods which should have PDB information.  This includes Lambdas
    // and Resumable methods (Async/Iterators)
    std::map<SourceFile*, std::vector<BCSYM_SyntheticMethod*> > m_SyntheticMethods;

#if IDE
    // PEInfo -- to support CompileToDisk for Compile(CompileType_DeferWritePE)
    PEInfo *m_pInfoDeferWritePE;

    // Is the exe or dll on disk up-to-date?
    bool m_IsImageOnDiskUpToDate;
#endif

    DynamicHashTable<STRING*, ISymUnmanagedDocumentWriter*> m_pdbDocs;
};
