//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implements the file-level logic of the VB compiler.
//
//-------------------------------------------------------------------------------------------------

#pragma once

#define FIXED_SIZE_HASHTABLE_SIZE   512

class SourceFile;
class MetaDataFile;
class CallGraph;
class CompilationCaches;

interface IMetaDataImport;
interface IMetaDataImport2;
interface IMetaDataAssemblyImport;

#if IDE 
class ENCBuilder;
class ENCProcessor;
enum ENCSTATUS;
struct DeltaGenFixupItem;
#endif

struct METHODLOCATION;
struct FIELDLOCATION;
struct IVbCompilerItem;
struct SolutionExtensionData;

class CompilerProject;
class Compiler;
class CompilerHost;

#if IDE 
class SourceFileView;
class CIntelliSense;
class EditInfoLists;
class EditInfoList;
class ParseTreeService;
class CFileCodeModel;
struct ACTIVESTATEMENT;
enum LifetimeClass;
struct Temporary;
#endif

enum CompilationState;

class BCSYM_NamedRoot;
class BCSYM_MethodImpl;
class BCSYM_Expression;
class BCSYM_NamedType;

namespace ILTree
{
	class ILNode;
}

struct AttrIdentity;

typedef ErrorTable* (*AlternateErrorTableGetter)(SourceFile *File);

#pragma warning(disable: 4200)

enum GetMethodBodyFlags
{
    gmbNone = 0x0,
    gmbIDEBodies = 0x1,
    gmbRecordDependencies = 0x2,
    gmbMergeAnonymousTypes = 0x4,
    gmbIncludeBadExpressions = 0x8,
    gmbUseCache = 0x10,
    gmbLowerTree = 0x20,
};

// Keeps track of the imported namespaces.  We don't use BCSYM_NamedTypes
// because the binding rules for these things are different.
//
struct ImportedTarget
{
    ImportedTarget *m_pNext;

    // Where to report errors. This needs to be a TrackedLocation because it needs to
    // be move around in the file as edits are made.
    TrackedLocation m_loc;

    // Is this an invalid namespace?
    bool m_hasError;

    // Is this a project-level import ?
    bool m_IsProjectLevelImport;

    // Is this an Xml namespace?
    bool m_IsXml;

    // The namespace or class that is imported.
    BCSYM *m_pTarget;

    // The name of the alias for named imports.
    STRING *m_pstrAliasName;
    BCSYM_Alias * m_pAlias;

    // This is the compined name using the array below
    // This prevents needing to recreate this string
    // every time we emit a function
    STRING *m_pstrQualifiedName;

    // The type arguments for generic imports
    NameTypeArguments **m_ppTypeArguments;

    // The name we're referencing.
    unsigned m_NumDotDelimitedNames;
    STRING **m_DotDelimitedNames;
    STRING *m_FirstDotDelimitedNames;
};

#pragma warning(default: 4200)

#if IDE 
struct CompilerFileTreeNode : RedBlackNodeBaseT<STRING *>
{
    CompilerFile *pFile; // Not addref'd
};

struct CompilerFileTreeNodeOperations : EmptyNodeOperationsT<CompilerFileTreeNode>
{
    void copy(
        CompilerFileTreeNode * pNodeDest,
        CompilerFileTreeNode * pNodeSrc)
    {
        pNodeDest->pFile = pNodeSrc->pFile;
    }
};

typedef RedBlackTreeT
<
    STRING *,
    STRINGOperations,
    CompilerFileTreeNode,
    CompilerFileTreeNodeOperations
> CompilerFileTree;

typedef UINT64 SymbolHash;
typedef std::multimap<SymbolHash, mdToken> TokenHashTable;

#endif

struct NamespaceNode : RedBlackNodeBaseT<STRING *>
{
    BCSYM_Namespace *pNamespace;
};

struct NamespaceNodeOperations : EmptyNodeOperationsT<NamespaceNode>
{
    void copy(
        NamespaceNode * pNodeDest,
        NamespaceNode * pNodeSrc)
    {
        pNodeDest->pNamespace = pNodeSrc->pNamespace;
    }
};

typedef RedBlackTreeT
<
    STRING *,
    STRINGOperations,
    NamespaceNode,
    NamespaceNodeOperations
> NamespaceTree;

//****************************************************************************
// Implements the file-level compiler logic.
//****************************************************************************

class CompilerFile :
#if IDE 
    public IUnknown,
#endif IDE
    public CDoubleLink<CompilerFile>
#if FV_DEADBEEF
    , public Deadbeef<CompilerFile> // Must be last base class!
#endif
{
protected:

    //========================================================================
    // The following methods should only be called from the master
    // project compilation routines.  They should never be called directly.
    //========================================================================

    friend class CompilerProject;
    friend class PEBuilder;
    friend class CVBDbgee;    // this is for the debug expression evaluator

    // Methods that move this file up states.
    virtual bool _StepToBuiltSymbols() = 0;
    virtual bool _StepToBoundSymbols() = 0;
    virtual bool _StepToCheckCLSCompliance() = 0;
    virtual bool _StepToEmitTypes() = 0;
    virtual bool _StepToEmitTypeMembers() = 0;
    virtual bool _StepToEmitMethodBodies() = 0;

    // invoked by _PromoteToBound to complete the task started by _StepToBoundSymbols()
    virtual void CompleteStepToBoundSymbols() = 0;

    BCSYM_Namespace * m_pUnnamedNamespace;
    NamespaceTree m_NamespaceTree;

public:

    //========================================================================
    // Implementation.
    //========================================================================

    virtual ~CompilerFile();

    // Get the errors table for this file.
    ErrorTable *GetErrorTable()
    {
        return &m_ErrorTable;
    }

    bool HasActiveErrors()
    {
        return m_ErrorTable.HasErrorsThroughStep(m_step);
    }

    bool HasActiveErrorsNoLock()
    {
        return m_ErrorTable.HasErrorsThroughStepNoLock(m_step);
    }

    BCSYM_Container *GetConditionalCompilationConstants()
    {
        return m_pConditionalCompilationConstants;
    }

    CompilerProject *GetProject()
    {
        return m_pProject;
    }

    CompilerHost *GetCompilerHost();

    CompilerFile *GetNextFile()
    {
        return m_pfileNext;
    }

    CompilationState GetCompState()
    {
        return m_cs;
    }

    CompilationSteps GetCompStep()
    {
        return m_step;
    }

#if HOSTED
    void SetCompState(CompilationState cs);
#endif

    BCSYM_Namespace * GetNamespace(_In_z_ STRING * strNamespaceName);

    void AddNamespace(
        _In_z_ STRING * strNamespaceName,
        BCSYM_Namespace * pNamespace);

#if IDE 
    CompilationState GetHighestStartedCompState();

    // Returns the least of the current compilation state and the
    // decompilation state. This is used to determine if a file needs
    // to be decompiled. (If a file has already been demoted to a
    // level at or below the level to which it is to be decompiled,
    // there's no need to decompile it.)
    CompilationState GetDecompilationState()
    {
        return m_cs > m_DecompilationState ? m_DecompilationState : m_cs;
    }

    void SetDecompilationState(CompilationState state)
    {
        m_DecompilationState = state;
    }

    bool HasBindingStarted()
    {
        return m_BindingStatus_Started;
    }

    void SetBindingStarted()
    {
        m_BindingStatus_Started = true;
        m_NeedToDecompileFileFromPartiallyBoundState = false;
    }

    bool NeedToDecompileFileFromPartiallyBoundState()
    {
        return m_NeedToDecompileFileFromPartiallyBoundState;
    }

    void SetNeedToDecompileFileFromPartiallyBoundState(
        bool fNeedToDecompileFileFromPartiallyBoundState)
    {
        m_NeedToDecompileFileFromPartiallyBoundState = fNeedToDecompileFileFromPartiallyBoundState;
    }
#endif IDE

#if IDE 


    //  IUnknown methods
    //      Consider: We really should use ATL here. If you don't like debugging ATL try reading ATL Internals.
    //
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

#endif IDE


    void ClearingBindingStatus();

    STRING *GetFileName()
    {
        return m_pstrFileName;
    }

    BCSYM_Namespace *GetUnnamedNamespace()
    {
        return m_pUnnamedNamespace;
    }

    void SetUnnamedNamespace(BCSYM_Namespace *pNamespace) // the default namespace for the file
    {
        m_pUnnamedNamespace = pNamespace;
    }

    BCSYM_Namespace *GetRootNamespace();

    OPTION_FLAGS GetOptionFlags()
    {
        return m_OptionFlags;
    }

    void SetOptionFlags(OPTION_FLAGS OptionFlags)
    {
        m_OptionFlags = OptionFlags;
    }

    void CombineOptionFlags(OPTION_FLAGS OptionFlags)
    {
        m_OptionFlags |= OptionFlags;
    }

    void StripOptionFlags(OPTION_FLAGS OptionFlags)
    {
        m_OptionFlags &= ~OptionFlags;
    }

    bool HasOptionFlags( OPTION_FLAGS OptionFlags)
    {
        return m_OptionFlags & OptionFlags;
    }

    //========================================================================
    // Object locking and unlocking mechanism.
    //========================================================================
    CompilerIdeCriticalSection& GetObjectCriticalSection()
    {
        return m_csFile;
    }

    //========================================================================
    // Derived class accessors.
    //========================================================================

    bool IsSourceFile()
    {
        return m_filetype == FILE_Source;
    }

    bool IsSolutionExtension()
    {
        return m_pExtensionData != NULL;
    }

    void SetSolutionExtension(SolutionExtensionData * pExtensionData)
    {
        m_pExtensionData = pExtensionData;
    }

    SolutionExtensionData *GetSolutionExtension()
    {
        return m_pExtensionData;
    }

    bool CompileAsNeeded();

    SourceFile *PSourceFile()
    {
        VSASSERT(IsSourceFile(), "Bad kind.");
        return (SourceFile *)this;
    }

    bool IsMetaDataFile()
    {
        return m_filetype == FILE_MetaData;
    }

    MetaDataFile *PMetaDataFile()
    {
        VSASSERT(IsMetaDataFile(), "Bad kind.");
        return (MetaDataFile *)this;
    }

    // True if the user has modified the module explicitly.
    // This causes the "Save before close?" dialog to come up.
    bool IsModifiedByUser()
    {
        return m_isModifiedByUser;
    }
    void SetIsModifiedByUser(bool isModifiedByUser)
    {
        m_isModifiedByUser = isModifiedByUser;
    }

    NorlsAllocator *SymbolStorage()
    {
        return &m_nraSymbols;
    }

    SymbolList *GetNamespaceLevelSymbolList()
    {
        return &m_SymbolList;
    }

    Compiler *GetCompiler()
    {
        return m_pCompiler;
    }

    CompilerProject *GetCompilerProject()
    {
        return m_pProject;
    }

    // Remove all namespaces created for the file.
    void RemoveNamespaces();

    // Remove a particular namespace and all namespaces created within it.
    void RemoveNamespaces(BCSYM_Namespace *NamespaceToRemove);

#if IDE 

    // File change notifications.
    HRESULT WatchFile();
    HRESULT UnwatchFile();

    // Is this file being watched for file changes?
    bool IsWatched()
    {
        return m_isWatched;
    }

#endif IDE
    GenericBindingCache* GetCurrentGenericBindingCache();

    bool IsUnlinkedFromProject() 
    {
         return m_isUnlinkedFromProject; 
    }

    bool ContainsExtensionMethods()
    {
        return m_ContainsExtensionMethods;
    }

    void SetContainsExtensionMethods(bool value)
    {
        m_ContainsExtensionMethods = value;
    }

protected:

    CompilerFile(Compiler *pCompiler);

    // Derived classes must call this first thing to set up the tables
    // of this class.
    //
    enum FileType
    {
        FILE_Source,
        FILE_MetaData
    };

    HRESULT Init(
        FileType ft,
        CompilerProject * pProject);

    void UnlinkFromProject();

    //
    // Data members.
    //

#if DEBUG

    friend class FileLeakDetection;

    // This must be first!
    unsigned m_DebFirstCompilerFileVariable;
    WCHAR m_wszDebName[MAX_PATH];

#endif DEBUG

    // Context.
    Compiler *m_pCompiler;

    // Linked list of all files.
    CompilerFile *m_pfileNext;
    CompilerFile *m_pfilePrev;

    // The kind of file we have.
    FileType m_filetype;
    SolutionExtensionData *m_pExtensionData;

    // Holds a lock on the file.
    // Note: This used to be a Mutex but this was allowing re-entracy because a Mutex will pump messages.
    CompilerIdeCriticalSection m_csFile;

    // Containing project.
    CompilerProject *m_pProject;

    // The filename of the source.
    STRING *m_pstrFileName;

    // The current state of the file.
    CompilationState m_cs;

#if IDE 
    // Threading locks.
    EventSignal m_eventDeclaredState;

    // The compilation state to which the file has been decompiled.
    // (This differs from the current compilation state in that
    // demotion (which actually changes the compilation state) occurs
    // later than decompilation. The decompilation state is used to
    // avoid repeatedly decompiling a file.)
    CompilationState m_DecompilationState;

    // Whether this file is being watched for changes
    // or not.
    //
    bool m_isWatched;

#endif IDE

    // What step we're on.
    CompilationSteps m_step;

protected:

    // The Error Table
    ErrorTable m_ErrorTable;

    // Holds all of the symbols defined by this module.
    NorlsAllocator m_nraSymbols;

    // The list of project-level symbols defined by this file.
    SymbolList m_SymbolList;

    // The conditional compilation constants defined in this file.
    BCSYM_Container *m_pConditionalCompilationConstants;

    // The Option flags ( Option Strict, etc. ) set for the SourceFile
    OPTION_FLAGS m_OptionFlags;

#if IDE 
    // Indicates that the binding for a file has started, but may not have completed.
    //
    bool m_BindingStatus_Started;

    // indicates that although the file might not be in CS_Bound, it
    // still needs to be decompiled to declared
    //
    bool m_NeedToDecompileFileFromPartiallyBoundState;

    // Reference count
    ULONG m_cRefs;
#endif IDE

    // the generic bindings cache used during declared to Bound
    //
    GenericBindingCache m_DeclaredToBoundGenericBindingCache;

    //
    // Built as we compile.
    //

    //. True if the user has modified the module explicitly.
    // This causes the "Save before close?" dialog to come up.
    unsigned char m_isModifiedByUser:1;

    // True if UnlinkFromProject has been called on this CompilerFile.
    unsigned char m_isUnlinkedFromProject:1;

#if DEBUG
    // This must be last!
    unsigned m_DebLastCompilerFileVariable;
#endif DEBUG

    bool m_ContainsExtensionMethods;
};


#if DEBUG && IDE

//****************************************************************************
// Helper to find when a file gets leaked.
//****************************************************************************

class FileLeakDetection
{
public:
    void Add(CompilerFile *pfile)
    {
        if (m_pfileList)
        {
            m_pfileList->m_pfilePrev = pfile;
        }

        pfile->m_pfileNext = m_pfileList;
        pfile->m_pfilePrev = NULL;

        m_pfileList = pfile;
    }

    void Remove(CompilerFile *pfile)
    {
        if (pfile->m_pfilePrev)
        {
            pfile->m_pfilePrev->m_pfileNext = pfile->m_pfileNext;
        }
        else
        {
            m_pfileList = pfile->m_pfileNext;
        }

        if (pfile->m_pfileNext)
        {
            pfile->m_pfileNext->m_pfilePrev = pfile->m_pfilePrev;
        }
    }

    ~FileLeakDetection()
    {
        VSASSERT(!m_pfileList, "At least one CompilerFile has been leaked from the compiler.");
    }

    CompilerFile *m_pfileList;

};

extern FileLeakDetection s_FileLeakDetection;

#endif DEBUG

struct ImportTrackerEntry
{
    BCSYM_NamedRoot  *newSymbolBound;
    bool              fIsCollision;

    Location         location;

    STRING           *pstrCorrection;
    bool              fIsExtensionMethodBindingChange;

    static
    int _cdecl CompareReverse(
        const void * one,
        const void * two)
    {
        return -Location::Compare( &(((ImportTrackerEntry*)one)->location),
                                   &(((ImportTrackerEntry*)two)->location) );
    }
};


struct UnprocessedFriend
{
    STRING* strAssembly;
    Location* pLocation;
};

int _cdecl SortSourceFileByName(
    const void * arg1,
    const void * arg2);

