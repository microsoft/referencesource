//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implements the project-level logic of the VB compiler.
//
//-------------------------------------------------------------------------------------------------

#pragma once

#if IDE
class CAssemblyReferenceManager;
class CVBErrorFixGen;
class CCodeModel;
class CExternalCodeModel;
class ENCBuilder;
struct DeltaGenFixupItem;
enum ENCSTATUS;

#endif IDE

#define FRIEND_HASHTABLE_SIZE 256



//========================================================================
//
// Structures
//
//========================================================================

struct SubsystemVersion
{
    DWORD major;
    DWORD minor;
};

struct ExtensionMethodLookupCacheEntry;

enum ExtensionMethodLookupCacheUsageType
{
    Usage_Invalid,
    Usage_ReferenceObject,
    Usage_InsideObject
};

struct CacheEntryAndRelativePrecedenceInfo
{
    unsigned long m_relativePrecedenceInfo;
    ExtensionMethodLookupCacheEntry * m_pCacheEntry;

    CacheEntryAndRelativePrecedenceInfo
    (
        ExtensionMethodLookupCacheEntry * pCacheEntry= NULL,
        unsigned long relativePrecedenceInfo = 0
    ) :
        m_relativePrecedenceInfo(relativePrecedenceInfo),
        m_pCacheEntry(pCacheEntry)
    {
    }


};

struct CacheEntryAndBool
{
    ExtensionMethodLookupCacheEntry * m_pCacheEntry;
    bool m_increasePrecedenceLevel;

    CacheEntryAndBool
    (
        ExtensionMethodLookupCacheEntry * pCacheEntry = NULL,
        bool increasePrecedenceLevel = false
    ) :
        m_pCacheEntry(pCacheEntry),
        m_increasePrecedenceLevel(increasePrecedenceLevel)
    {
    }
};

struct ExtensionMethodLookupCacheEntry
{
    ExtensionMethodLookupCacheEntry(const NorlsAllocWrapper & alloc) :
        m_method(NULL),
        m_parentCacheEntries(alloc)
    {
    }

    void AddParentEntry(ExtensionMethodLookupCacheEntry * pParentEntry, unsigned long relativePrecedenceLevel)
    {
        ThrowIfNull(pParentEntry);
        m_parentCacheEntries.Add(CacheEntryAndRelativePrecedenceInfo(pParentEntry, relativePrecedenceLevel));
    }

    //Stores a pointer to the first overload within the type the cache entry
    //is associated with. If the cache entry is not associated with a type
    //(because for example it's associated with a namespace import and only has pointers to parent entries) then
    //this will be null.
    BCSYM_Proc * m_method;
    //A list of parent cache entries. These are the "parent scopes" that contain the scope this cache entry is associated with.
    List<CacheEntryAndRelativePrecedenceInfo, NorlsAllocWrapper> m_parentCacheEntries;
};

enum ExtensionMethodLookupCacheKeyObjectType
{
    KeyObject_Uninitialized,
    KeyObject_SourceFileSymbol,
    KeyObject_Symbol,
    KeyObject_SourceFile,
};


//Represents a "key object" in the extension method lookup cache.
//It corresponds to a description of the scope that included a particular set of extension methods.
//This, along with a name and a usage type flag form the key for entries in the cache.
//A key object is essetnally a type that can be a pointer to either:
//     1) A symbol
//     2) A source file
//     3) A compiler project
//     4) A pair including poth a source file and a symbol
//It is represented by a union and a type flag. For all type flags other than
//KeyObject_SourceFileSymbol a simple void * is used. For KeyObject_SourceFileSymbol
//a structre containing both a SourceFile * and a Symbol * is used.
//An instance of this class, along with an ExtensionMethodLookupCacheUsageType flag
//together define the scopes in which extension methods may be introduced into a program.
//This type is used to refer to the object granting access, and the usage flag is used to indicate
//how the object is used (namespace and module symbols can be used in either an "imports" or a
//"contained in" manner, and both usage types introduce extension methods in a different way).
struct ExtensionMethodLookupCacheKeyObject
{
    ExtensionMethodLookupCacheKeyObjectType m_type;
    union
    {
        void * m_pSimpleObject;

        struct
        {
            SourceFile * m_pSourceFile;
            BCSYM_NamedRoot * m_pContext;
        };
    };

    ExtensionMethodLookupCacheKeyObject();

    ExtensionMethodLookupCacheKeyObject
    (
        SourceFile * pSourceFile,
        BCSYM_Container* pContext
    );


    ExtensionMethodLookupCacheKeyObject
    (
        BCSYM_NamedRoot * pSymbol
    );

    ExtensionMethodLookupCacheKeyObject
    (
        SourceFile * pSourceFile
    );


    static int Compare
    (
        const ExtensionMethodLookupCacheKeyObject * pLeft,
        const ExtensionMethodLookupCacheKeyObject * pRight
    );
};

//The extension method name lookup cache.
//It is keyed on the following:
//     STRING * Name
//                    - the name of the extension method being searched for
//     ExtensionMethodLookupCacheKeyObject keyObject
//                    - the object that is bringing extension methods into scope.
//                       This can be a containing container, a source file, a "compiler project", etc.
//     ExtensionMethodLookupCacheUsageType usageType
//                    - a flag indicating how the "key object" is being used. This is necessary because
//                       different cache entries are needed for extension method calls inside of a module or namespace
//                       then for references to modules. The reason for this is because calls inside of a module will
//                       have a parent entry that points at the entry for the symbol's containing namespace, where as
//                       references to the symbol through an import do not.
//
//The values in the cache are ExtensionMethodLookupCacheEntry * values.
//
//The cache its self is hieararchal. Every entry has pointers to its parent entries. This way, the cache entry for a reference to namespace "Ns1" is shared
//by all files that import that namespace. Similarly, the cache entry for extension methods inside the default namespace of a project is shared by all
//top level containers in every source file in the project. This design leads to a high degree of cache reuse, which dramatically reduces compile time for
//large projects.
//
//This is why it is necessary to differentiate between "SourceFile + Symbol" key objects and individual Sourcefile or Symbol key objects.
//A SourceFile +Symbol key object is the usually the "root" (or "leaf" depending on point of view) where name lookup starts.
//Its's parent cache entries are the cache entries for the "Symbol" and the cache entries for the "SourceFile".  Namspace "Symbol" cache entries wil be reused
//by all types contained in it, and "SourceFile" cache entries will be reused by all containers defined in the source file.
class ExtensionMethodNameLookupCache
{
    friend class CompilationCaches;
    friend class CVbCompilerCompCache;
public:
    ExtensionMethodNameLookupCache
    (
        NorlsAllocator * pnorls
    );

    bool
    LookupInCache
    (
        _In_ STRING * pMethodName,
        const ExtensionMethodLookupCacheKeyObject   & keyObject,
        ExtensionMethodLookupCacheUsageType usageType,
        ExtensionMethodLookupCacheEntry ** ppOutCacheValue
    );

    void
    AddEntry
    (
        _In_ STRING * pMethodName,
        const ExtensionMethodLookupCacheKeyObject   & keyObject,
        ExtensionMethodLookupCacheUsageType usageType,
        ExtensionMethodLookupCacheEntry * pCacheValue
    );

    void Clear();
    NorlsAllocator * GetNorlsAllocator();


private:
    struct Key
    {
        STRING * m_pMethodName;
        ExtensionMethodLookupCacheKeyObject m_keyObject;
        ExtensionMethodLookupCacheUsageType m_usageType;

        Key
        (
            _In_ STRING * pMethodName,
            const ExtensionMethodLookupCacheKeyObject & keyObject,
            ExtensionMethodLookupCacheUsageType usageType
        );

        Key();
    };
public:
    struct Node :
        public RedBlackNodeBaseT<Key>
    {
        ExtensionMethodLookupCacheEntry * pCacheEntry;
    };
private:
    struct KeyOperations :
        public SimpleKeyOperationsT<Key>
    {
        int compare(const Key *pKey1, const Key * pKey2);
    };

    struct NodeOperations :
        public EmptyNodeOperationsT<Node>
    {
        void copy(Node *pNodeDest, const Node *pNodeSrc);
    };
public:
    typedef RedBlackTreeT<Key, KeyOperations, Node, NodeOperations> tree_type;
private:
    tree_type m_cache;
};


struct ErrorCacheNode :
    RedBlackNodeBaseT<CompilerProject *>
{
    bool HasErrors : 1;
    bool HasReferenceErrors : 1;
    bool ErrorsSet : 1;
    bool ReferenceErrorsSet : 1;
};

struct ErrorCacheNodeOperations :
    EmptyNodeOperationsT<ErrorCacheNode>
{
    void copy(ErrorCacheNode *pNodeDest, ErrorCacheNode *pNodeSrc)
    {
        pNodeDest->HasErrors = pNodeSrc->HasErrors;
        pNodeDest->ErrorsSet = pNodeSrc->ErrorsSet;
        pNodeDest->HasReferenceErrors = pNodeSrc->HasReferenceErrors;
        pNodeDest->ReferenceErrorsSet = pNodeSrc->ReferenceErrorsSet;
    }
};

typedef RedBlackTreeT
<
    CompilerProject *,
    SimpleKeyOperationsT<CompilerProject *>,
    ErrorCacheNode,
    ErrorCacheNodeOperations
> ProjectErrorCacheTree;

struct ResolvedImportsNode :
    public RedBlackNodeBaseT<STRPTR>
{
    BCSYM_NamedRoot *BoundImport;
};

struct ResolvedImportsNodeOperations :
    public EmptyNodeOperationsT<ResolvedImportsNode>
{
    void copy(ResolvedImportsNode *pNodeDest, const ResolvedImportsNode *pNodeSrc)
    {
        pNodeDest->BoundImport = pNodeSrc->BoundImport;
    }
};

typedef RedBlackTreeT<
    STRPTR,
    STRINGOperations,
    ResolvedImportsNode,
    ResolvedImportsNodeOperations>
    ResolvedImportsTree;

class LiftedUserDefinedOperatorCache
{
    friend class CompilationCaches;
    friend class CVbCompilerCompCache;
public:

    LiftedUserDefinedOperatorCache
    (
        NorlsAllocator * pNorls
    );

    bool
    LookupInCache
    (
        BCSYM_UserDefinedOperator * pSourceOperator,       //[in] - the source operator to look for
        BCSYM_UserDefinedOperator ** ppCacheValue          //[out] - stores the cached lifted operator, may be null
    );

    void
    AddEntry
    (
        BCSYM_UserDefinedOperator * pSourceOperator,       //[in] - the source operator
        BCSYM_UserDefinedOperator * pCacheValue            //[in] - the lifted operator created from pSourceOperator to store in the cache.
    );

    void Clear();
    NorlsAllocator * GetNorlsAllocator();
private:
    struct Key
    {
        BCSYM_UserDefinedOperator * pSourceOperator;

        Key();
        Key
        (
            BCSYM_UserDefinedOperator * pSrcOp
        );

        bool operator <(const Key & other) const;
    };
public:
    struct Node :
        public RedBlackNodeBaseT<Key>
    {
        BCSYM_UserDefinedOperator * pLiftedOperator;
    };
private:
    struct KeyOperations :
        public SimpleKeyOperationsT<Key>
    {
        int compare(const Key *pKey1, const Key * pKey2);
    };

    struct NodeOperations :
        public EmptyNodeOperationsT<Node>
    {
        void copy(Node *pNodeDest, const Node *pNodeSrc);
    };
public:
    typedef RedBlackTreeT<Key, KeyOperations, Node, NodeOperations> tree_type;
private:
    tree_type m_cache;
};


struct LookupKey
{
    LookupKey()
    {
        memset(this, 0, sizeof(LookupKey));
    }

    void InitKey
    (
        _In_z_ STRING *Name,
        BCSYM_Namespace *Namespace,
        unsigned __int64 Flags,
        unsigned BindspaceMask,
        int GenericTypeArity,
        bool IgnoreImports,
        bool IgnoreModules,
        CompilerProject *pProject
    )
    {
        this->Name = StringPool::Pstrinfo(Name);

        // This key is basically used to minimize the number of lookups that occur inside namespaces. We are basically storing
        // the results of a name lookup for a given namespace. In order to achieve maximum efficiency we fold namespaces
        // together except for the case when we are looking in the Unnamed namespace AND Considering Imports.

        // The second condition should be removed if we ever allow imports in namespaces other than the unnamed one.
        if (IgnoreImports || StringPool::StringLength(Namespace->GetName()) > 0)
        {
            this->Scope = Namespace->GetNamespaceRing();
        }
        else
        {
            this->Scope = Namespace;
        }

        this->Flags = Flags;
        this->BindspaceMask = BindspaceMask;
        this->GenericTypeArity = GenericTypeArity;
        this->IgnoreFlags = BoolToInt(IgnoreImports) | (BoolToInt(IgnoreModules) << 1);
        this->m_pProject = pProject;
    }

    STRING_INFO *Name;
    void *Scope;
    unsigned __int64 Flags;
    unsigned BindspaceMask;
    int GenericTypeArity;
    unsigned IgnoreFlags;
    CompilerProject *m_pProject;
};

struct LookupKeyOperations :
    public SimpleKeyOperationsT<LookupKey>
{
    int compare(const LookupKey *pKey1, const LookupKey *pKey2)
    {
        return memcmp(pKey1, pKey2, sizeof(LookupKey));
    }
};

struct LookupNode :
    public RedBlackNodeBaseT<LookupKey>
{
    BCSYM_NamedRoot *Result;
    BCSYM_GenericBinding *GenericBindingContext;
};

struct LookupNodeOperations :
    public EmptyNodeOperationsT<LookupNode>
{
    void copy(LookupNode *pNodeDest, const LookupNode *pNodeSrc)
    {
        pNodeDest->Result = pNodeSrc->Result;
        pNodeDest->GenericBindingContext = pNodeSrc->GenericBindingContext;
    }
};

typedef RedBlackTreeT<
    LookupKey,
    LookupKeyOperations,
    LookupNode,
    LookupNodeOperations> LookupTree;

// Merging of the namespace symbols happens at the compilerhost level. While calculating the
// merged hash of a namespace ring, only symbols in the current CompilerHost are added to this
// merged hash. So the key in this table should include the CompilerHost as well.
struct NamespaceRingKey
{
    NamespaceRingKey()
    {
        memset(this, 0, sizeof(NamespaceRingKey));
    }

    void InitKey(_In_ CompilerHost *pCompilerHost, _In_ BCSYM_NamespaceRing *pNamespaceRing)
    {
        this->CompilerHost = pCompilerHost;
        this->NamespaceRing = pNamespaceRing;
    }

    CompilerHost *CompilerHost;
    BCSYM_NamespaceRing *NamespaceRing;
};

struct NamespaceRingKeyOperations :
    public SimpleKeyOperationsT<NamespaceRingKey>
{
    int compare(_In_ const NamespaceRingKey *pKey1, _In_ const NamespaceRingKey *pKey2)
    {
        return memcmp(pKey1, pKey2, sizeof(NamespaceRingKey));
    }
};

struct NamespaceRingNode :
    public RedBlackNodeBaseT<NamespaceRingKey>
{

    NamespaceRingNode() :
        MergedHash(NULL),
        ContainsExtensionMethods(false),
        ContainsModules(false),
        IteratedForExtensionMethods(false),
        IteratedForModules(false),
        HasBeenMerged(false)
    {
    };

    BCSYM_Hash *MergedHash;
    bool ContainsExtensionMethods : 1;
    bool ContainsModules : 1;
    bool IteratedForExtensionMethods : 1;
    bool IteratedForModules : 1;
    bool HasBeenMerged : 1;
};

struct NamespaceRingNodeOperations :
    public EmptyNodeOperationsT<NamespaceRingNode>
{
    void copy(NamespaceRingNode *pNodeDest, const NamespaceRingNode *pNodeSrc)
    {
        pNodeDest->MergedHash = pNodeSrc->MergedHash;
        pNodeDest->ContainsExtensionMethods = pNodeSrc->ContainsExtensionMethods;
        pNodeDest->ContainsModules = pNodeSrc->ContainsModules;
        pNodeDest->IteratedForExtensionMethods = pNodeSrc->IteratedForExtensionMethods;
        pNodeDest->IteratedForModules = pNodeSrc->IteratedForModules;
        pNodeDest->HasBeenMerged = pNodeSrc->HasBeenMerged;
    }
};

typedef RedBlackTreeT<
    NamespaceRingKey,
    NamespaceRingKeyOperations,
    NamespaceRingNode,
    NamespaceRingNodeOperations> NamespaceRingTree;

class CompilationCaches
{
    friend class CVbCompilerCompCache; // test hook
    
public:
    CompilationCaches();
#if IDE  
    // This constructor is only used when we don't want to allocate on the
    // default allocator because we need the allocated symbols to outlive 
    // this CompilationCaches instance.  The memory allocated by pnorls must
    // be freed by the caller that created this CompilationCaches instance.
    CompilationCaches(NorlsAllocator * pnorls);
#endif IDE  

    ~CompilationCaches();


    LookupTree *GetLookupCache()
    {
        return &m_LookupCache;
    }

    ExtensionMethodNameLookupCache *GetExtensionMethodLookupCache()
    {
        return &m_ExtensionMethodLookupCache;
    }

    LiftedUserDefinedOperatorCache *GetLiftedOperatorCache()
    {
        return &m_LiftedOperatorCache;
    }

    NamespaceRingTree *GetMergedNamespaceCache()
    {
        return &m_MergedNamespaceCache;
    }
    //Using 0 to represnet CompCacheType cacheType = CompCacheType_AllCaches 
    // can't use it directly, because of forward declares and it's only in IDE
    void ClearCompilationCaches(int cacheType = 0);
#if DEBUG
#if IDE
    void ValidateCaches(int cacheType = 0);
    void DumpStats();
#endif IDE
#endif DEBUG

private:
    LookupTree m_LookupCache;
    ExtensionMethodNameLookupCache m_ExtensionMethodLookupCache;
    LiftedUserDefinedOperatorCache m_LiftedOperatorCache;
    NamespaceRingTree m_MergedNamespaceCache;
    NorlsAllocator m_nrlsCachedData;
};


// helper function that forwards to CompilerPackage, if available
CompilationCaches *GetCompilerCompilationCaches();



//
// This struct is used to track one resource request
//
struct Resource
{
    STRING *m_pstrFile;         // Full path to resource file
    STRING *m_pstrName;         // Name of the resource
    bool    m_fPublic;          // Public or private?
    bool    m_fEmbed;           // Embed or link?
};

#if IDE 


struct BoundMethodBodyNode
{
    BoundMethodBodyNode()
    {
        memset(this, 0, sizeof(BoundMethodBodyNode));
    }

    ILTree::ProcedureBlock *pBound;
    ParseTree::MethodBodyStatement *pUnbound;
    CLinkRangeIter<TransientSymbol> TransientSymbolIter;
    ULONG lCachedSourceChangeIndex;
    ULONG lCachedBoundChangeIndex;
    bool MergeAnonymousTypes;
};

class BoundMethodBodyCache : public IdeBoundMethodDataCache
{
public:
    BoundMethodBodyCache(Compiler* pCompiler) : 
      m_BoundMethodBodyTransients(pCompiler, &m_norlsAllocator)
    {

    }

    virtual __override void Clear()
    {
        m_BoundMethodBodies.clear();
        m_BoundMethodBodyTransients.Clear();
        IdeBoundMethodDataCache::m_norlsAllocator.FreeHeap();
    }

    TransientSymbolStore* GetBoundMethodBodyTransients()
    {
        return &m_BoundMethodBodyTransients;
    }

    std::map<BCSYM_Proc *, BoundMethodBodyNode> * GetBoundMethodBodies()
    {
        return &m_BoundMethodBodies;
    }

private:
    TransientSymbolStore m_BoundMethodBodyTransients;
    std::map<BCSYM_Proc *, BoundMethodBodyNode> m_BoundMethodBodies;
};

struct FireOnStatementChanged
{
    FireOnStatementChanged *m_pNext;
    SourceFile *m_pFile;

};

class FireOnStatementChangedList : ZeroInit<FireOnStatementChangedList>
{
public:
    FireOnStatementChangedList();
    ~FireOnStatementChangedList()
    {
        FireOnStatementChanged *pNode;
        for (pNode = m_pHeadNode; pNode; pNode = pNode->m_pNext)
        {
            pNode->m_pFile->Release();
        }
        m_NoReleaseAllocator.FreeHeap();
        m_pHeadNode = NULL;
    }

    void AddFileToList(SourceFile *pFile)
    {
        // Create a new node.
        FireOnStatementChanged *pNode = (FireOnStatementChanged *)m_NoReleaseAllocator.Alloc(sizeof(FireOnStatementChanged));
        pNode->m_pFile = pFile;
        pNode->m_pNext = m_pHeadNode;
        pNode->m_pFile->AddRef();

        m_pHeadNode = pNode;
    }

    void FireOnStatementChangedEvents()
    {
        FireOnStatementChanged *pNode;
        for (pNode = m_pHeadNode; pNode; pNode = pNode->m_pNext)
        {
            if (pNode->m_pFile->GetCompState() >= CS_Bound)
            {
                if (pNode->m_pFile->IsNeedToFireOnStatementsChangedEvents())
                {
                    if (!pNode->m_pFile->IsSolutionExtension())
                    {
                        pNode->m_pFile->GetCodeFile()->OnStatementChanged();
                    }
                    pNode->m_pFile->SetNeedToFireOnStatementsChangedEvents(false);
                }
            }
            pNode->m_pFile->Release();
        }

        m_NoReleaseAllocator.FreeHeap();
        m_pHeadNode = NULL;
    }

private:

    NorlsAllocator m_NoReleaseAllocator;

    // The head of the tree.
    FireOnStatementChanged *m_pHeadNode;

};

class XmlIntellisenseSchemas
{
public:
    XmlIntellisenseSchemas(CompilerProject * Project)
    {
        m_Project = Project;
    }

    void Open(GUID ProjectGuid);
    bool IsOpen()
    {
        return m_IsOpen;
    }
    void Close();

    bool Refresh();
    void CancelRefresh()
    {
        m_IsRefreshing = false;
    }

    bool GetMembers(IXmlIntellisenseMemberList ** ppXmlMembers);
    bool GetTargetNamespaces(SAFEARRAY ** ppTargetNamespaces);

    STRING * GetErrorSource()
    {
        return m_FirstErrorSource;
    }

    bool IsEmpty()
    {
        BOOL fIsEmpty = TRUE;
        return !m_Schemas || FAILED(m_Schemas->get_IsEmpty(&fIsEmpty)) || fIsEmpty;
    }

    void ShowInXmlSchemaExplorer(
        _In_opt_z_ STRING * pstrNamespace, 
        _In_opt_z_ STRING * pstrLocalName, 
        _Out_ BOOL &fElementFound,
        _Out_ BOOL &fNamespaceFound);

private:
    CompilerProject *m_Project;
    CComPtr<IXmlIntellisenseSchemas> m_Schemas;
    HANDLE m_SchemasCompiledEvent;
    STRING *m_FirstErrorSource;
    bool m_IsOpen;
    bool m_IsRefreshing;
};

#endif


struct SolutionExtensionData
{
    SolutionExtensionEnum m_extension;
    StringBuffer *m_psbCode;
    STRING *m_pstrName;
    STRING *m_pstrFileName;
};

//=============================================================================
// Holds the info accosiated with a Sub Main. Please note that either
// m_SubMainSymbol or m_ParentSymbol is non-NULL. We use m_ParentSymbol when
// we don't have a BCSYM_Proc for Main (i.e. faking main for Microsoft)
//
// Valid Entry Points are:
//
// Sub Main()
// Sub Main(ByVal args() As String)
// Function Main() As Integer
// Function Main(ByVal args() As String) As Integer
//
//=============================================================================

struct SubMainInfo
{
    BCSYM_Proc      *m_SubMainSymbol;       // BCSYM for Sub Main
    BCSYM_NamedRoot *m_ParentSymbol;        // BCSYM for Container of Sub Main (used for Microsoft)
    bool             m_IsValidEntryPoint;   // Is this Main a valid entry point?
    bool             m_IsInvalidOnlyDueToGenericness;   // Is this Main invalid only because it is either
                                                        // a generic method or nested in a generic type ?
};

class MyGroupCollectionInfo
{
public:
    MyGroupCollectionInfo(Compiler *pCompiler)
    {
        
    }

    ~MyGroupCollectionInfo()
    {
        m_daMyGroupMembers.Destroy();
    }

    BCSYM_Class* m_groupClass;                      // the group class
    DynamicArray<BCSYM_Class *> m_daMyGroupMembers; // list of members
};


//========================================================================
//
// CompilerProjectSite
//
// This class is used to hose the event sink (when files come and go
// from project). Used by the Object browser internal data structures.
//
//========================================================================
#if IDE 
class CompilerProject; // Forward declare.

#if DEBUG
#define IDC_WAITCURSOR IDC_NO       // Use slashed circle in debug builds to show us when we're waiting
#else
#define IDC_WAITCURSOR IDC_WAIT     // Std hourglass for retail builds
#endif

// 
class CompilerProjectSite :
    public IAdviseSite
{
public:
             CompilerProjectSite( CompilerProject * const, IVbProjectEvents * );
    virtual ~CompilerProjectSite();

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef ();
    STDMETHODIMP_(ULONG) Release();

    // IAdviseSite methods
    STDMETHODIMP UnAdvise();
    STDMETHODIMP GetSink( REFIID, IUnknown ** );
    STDMETHODIMP GetHost( REFIID, IUnknown ** );

    // Custom methods
    IVbProjectEvents * AliasSink() { return( m_srpSink ); };
    void               Zombie();

private:
    ULONG                     m_ulRefs;
    CompilerProject *         m_pCompilerProject; // Aliased pointer.
    CComPtr<IVbProjectEvents> m_srpSink;
};

#endif // IDE

//========================================================================
//
// SecAttrErrorInfo, SecAttrInfo
//
// These structs are used to store the security attributes applied in a
// CompilerProject so that they can be emitted after the manifest for the
// CompilerProject is emitted. This is required so that any security attributes
// defined in a project when used in the same project are emitted correctly.
// Bug VSWhidbey 320892.
//========================================================================
struct SecAttrErrorInfo
{
    STRING *pstrAttrName;                   // Name of the security attribute that was applied.
    Location Loc;                           // Location where the security attribute was applied
    BCSYM_NamedRoot *pContext;              // The named context where the security attribute was applied. Eg: Partial Class component, Method, etc.
};

struct SecAttrInfo : CSingleLink<SecAttrInfo>
{
    BCSYM *pSymbol;                         // The symbol on which these security attributes are defined.
    mdToken tkSymbol;                       // Token for pSymbol.

    COR_SECATTR *prgSecAttr;                // Information needed by the clr metadata API to emit the security attributes.
    SecAttrErrorInfo *prgSecAttrErrorInfo;  // Information needed by the VB compiler to report errors on the applied security attributes.

    ULONG cSecAttrMax;                      // The max number of applied security attributes that can be stored in this node.
    ULONG cSecAttr;                         // The number of applied secruity attributes actually stored in this node. cSecAttr <= cSecAttrMax;
};
//========================================================================

//========================================================================
//
// ReferencedProject
//
// This struct represents a referenced CompilerProject.
//========================================================================
struct ReferencedProject
{
    CompilerProject  *m_pCompilerProject;
    bool              m_fUsed;
    bool              m_fManual;        // reference added manually by project or cmd-line
    bool              m_fStandard;      // reference is standard reference (system.dll)
    bool              m_fRequired;      // reference is required reference
    bool              m_fEmbedded;      // reference is a NoPIA reference.
};

// CompilerProject::AddReference flags
// CompilerProject::AddMetadataReferenceInternal flags
#define REFERENCEFLAG_MANUAL    0x0001   // added by user
#define REFERENCEFLAG_STANDARD  0x0002   // added as standard lib
#define REFERENCEFLAG_REQUIRED  0x0004   // default ref (mscorlib, msvbrun)
#define REFERENCEFLAG_EMBEDDED  0x0008   // embed the reference?


class PiaTypeIterator
{
public:
    PiaTypeIterator(
            HashSetIterator<BCSYM*> methodBodyTypeList,
            HashSetIterator<BCSYM*> declarationTypeList) :                            
            m_MethodBodyTypeList(methodBodyTypeList),
            m_DeclarationTypeList(declarationTypeList),
            useMethodBodyList(false) {}
    bool MoveNext();
    BCSYM* Current();
private:
    HashSetIterator<BCSYM*> m_MethodBodyTypeList;
    HashSetIterator<BCSYM*> m_DeclarationTypeList;
    bool useMethodBodyList;
};

// Forward declaration.
class CompilerHost;

//========================================================================
//
// CompilerProject
//
// This class represents an entire project in the compilation system
//
//========================================================================

class _declspec( uuid( "8B683E87-C664-44b8-A6A4-A405BAA715EE" ) )
CompilerProject :
    // public IUnknown,
    // Microsoft 2/4/03: VSWhidbey 56848:  IUnknown is already implied by the other interfaces, and will
    // cause C4584 to be fired when that warning is turned back on.
    public IVbCompilerProject,
    public IVbVbaCompiler,
    public IVsENCRebuildableProjectCfg2,
#if IDE
    public IVsLanguageServiceBuildErrorReporter,
    public IVsLanguageServiceBuildErrorReporter2,
#endif IDE
    public CSingleLink<CompilerProject>
#if FV_DEADBEEF
    , public Deadbeef<CompilerProject> // Must be last base class!
#endif
{
#if IDE 
    friend CompilerProjectSite;
    friend CVBErrorFixGen;
#endif IDE
    friend CVBDbgee;    //for debug display tool
    friend class IntellidocLoadAndIndexService;
public:

    //========================================================================
    // Lifetime
    //========================================================================

    CompilerProject(Compiler *pCompiler);
    virtual ~CompilerProject();

    // Populates the project with the information exposed in the build engine
    // object.
    //
    HRESULT InitProject(_In_z_ STRING *pstrFileName, IUnknown *punkProject, IVsHierarchy *pHier, CompilerHost *pCompilerHost);

    // Release memory and destroy MyCollection info
    void ReleaseMyCollectionInfo();

    // Creates MetaData files for the assembly as well as for each module the assembly is built with (if any).
    HRESULT InitMetaDataFiles(_In_z_ STRING *pstrFileName, bool bAssembly);

    HRESULT InitMetaDataFiles(_In_z_ STRING *pstrFileName, _In_ IMetaDataImport *pImport); 

    // Populates a project with the classes defined in a metadata file.
    void InitWithMetaData(_In_z_ STRING *pstrFileName, bool bAssembly, CompilerHost *pCompilerHost, bool bDefaultLibrary);

    // Initiale a project based on an existingi IMetaDataImport interface
    HRESULT InitWithMetaData(_In_z_ STRING *pstrFileName, _In_ CompilerHost *pCompilerHost, _In_ IMetaDataImport *pImport);

    // Binds the assemblyrefs in all the files of a metadata project.
    void BindAssemblyRefs(ActionKindEnum action = ActionKind::NoForce);

public:

    //========================================================================
    // IUnknown implementation
    //========================================================================

    // We don't use ATL for this object because it makes debugging a royal
    // pain in the ----.

    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);

    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    //========================================================================
    // IVbCompilerProject implementation
    //========================================================================

    // Set the compiler options.  The strings in this structure are
    // only guaranteed to be alive during this function call.
    //
    STDMETHOD(SetCompilerOptions)(VbCompilerOptions *pCompilerOptions);
    STDMETHOD(SetCompilerOptionsInternal)(VbCompilerOptions *pCompilerOptions);

    // Set the module assembly name option. Not set above since its a VBC
    // only setting.
    //
    STDMETHOD(SetModuleAssemblyName)(LPCWSTR wszName);

    // Useful for reducing the time taken by background threads if heavy
    // main-thread usage is occuring.  Caller is responsible for any balancing.
    STDMETHOD(SetBackgroundCompilerPriorityNormal)();
    STDMETHOD(SetBackgroundCompilerPriorityLow)();

    // Called each time a file is added to the project (via Add Item or
    //  during project open).
    //
    STDMETHOD(AddFile)(LPCWSTR wszFilePath, VSITEMID itemid, BOOL fAddDuringOpen);
    STDMETHOD(AddFileInternal)(LPCWSTR wszFilePath, VSITEMID itemid, BOOL fAddDuringOpen);

    // Remove a file from the project.
    STDMETHOD(RemoveFile)(LPCWSTR wszFilePath, VSITEMID vsid);
    STDMETHOD(RemoveFileInternal)(LPCWSTR wszFilePath, VSITEMID vsid);

    // Rename a file from the project
    STDMETHOD(RenameFile)(LPCWSTR wszOldFileName, LPCWSTR wszNewFileName, VSITEMID itemid);
    STDMETHOD(RenameFileInternal)(LPCWSTR wszOldFileName, LPCWSTR wszNewFileName, VSITEMID itemid);

    // Remove a file from the project.
    STDMETHOD(RemoveFileByName)(LPCWSTR wszPath);
    STDMETHOD(RemoveFileByNameInternal)(LPCWSTR wszPath);

    // called by VBA to introduce an in-memory buffer to the compiler project
    STDMETHOD(AddBuffer)(_In_count_(dwLen) WCHAR *pwszBuffer, DWORD dwLen, _In_z_ LPCWSTR wszMkr, VSITEMID itemid, BOOL fAdvise, BOOL fShowErrorsInTaskList);

    // called by VBA to give memory-backed IStream to ISymWriter for PDB symbol store
    // MetaEmit will AddRef and hold the pointer until metaemit is destroyed
    STDMETHOD(SetStreamForPDB)(IStream * pStreamPDB);

    // Add a reference to another project within the current solution
    STDMETHOD(AddProjectReference)(IVbCompilerProject* pReferencedCompilerProject);
    STDMETHOD(AddProjectReferenceInternal)(IVbCompilerProject* pReferencedCompilerProject);

    // Removes a reference to another project in the current solution.
    STDMETHOD(RemoveProjectReference)(IVbCompilerProject* pReferencedCompilerProject);
    STDMETHOD(RemoveProjectReferenceInternal)(IVbCompilerProject* pReferencedCompilerProject);

    // Add a reference to an Assembly.
    STDMETHOD(AddMetaDataReference)(LPCWSTR wszFilePath, BOOL bAssembly);

#if HOSTED
    // Add a reference to an Assembly.
    STDMETHOD(AddTypeScopeReference)(CompilerProject **ppCreatedCompilerProject);

    STDMETHOD(AddTypeScopeReferenceInternal)(CompilerProject **ppCreatedCompilerProject);

    void InitForTypeScope(CompilerHost *pCompilerHost);
    STDMETHOD(AddTypeScopeFile());
#endif

   // Add an embedded reference to an Assembly.
    STDMETHOD(AddEmbeddedMetaDataReference)(LPCWSTR wszFilePath);
    STDMETHOD(AddEmbeddedProjectReference)(IVbCompilerProject* pReferencedCompilerProject);
    
    // Remove a reference to an Assembly.
    STDMETHOD(RemoveMetaDataReference)(LPCWSTR wszFileName);
    STDMETHOD(RemoveMetaDataReferenceInternal)(LPCWSTR wszFileName);

    // Helper for removing one reference.
    STDMETHOD(RemoveOneReference)(ULONG ReferenceIndex, bool *pRemoved);

    // Delete all references.
    STDMETHOD(RemoveAllReferences)();
    STDMETHOD(RemoveAllReferencesInternal)();

    STDMETHOD(AddImport)(LPCWSTR wszNameSpace);
    STDMETHOD(AddImportInternal)(LPCWSTR wszNameSpace);

    STDMETHOD(IsImported)(LPCWSTR wszNameSpace, bool *pIsImported);

    STDMETHOD(DeleteImport)(LPCWSTR wszNameSpace);
    STDMETHOD(DeleteImportInternal)(LPCWSTR wszNameSpace);
    STDMETHOD(DeleteAllImports)();
    STDMETHOD(DeleteAllImportsInternal)();

    // Add a reference to a resource file
    STDMETHOD(AddResourceReference)(LPCWSTR wszFileLocation, LPCWSTR wszName, BOOL fPublic, BOOL fEmbed);
    STDMETHOD(DeleteAllResourceReferences)();

    // Notification that a "build" is starting. Since the compiler
    //  is always running in the background, this might not mean
    //  anything more than to disable some UI.
    //
    STDMETHOD(StartBuild)(IVsOutputWindowPane* pVsOutputWindowPane, BOOL fRebuildAll);

    // This is called if the user wishes to stop a "build". Since the
    //  compiler will always be running, its only effect might be to
    //  re-enable UI that is disabled during a build.
    //
    STDMETHOD(StopBuild)();

    // Disconnects from the project and event source, etc.
    STDMETHOD_(void,Disconnect)();

    // Lists all classes with Sub Main marked as shared (entry points). If called
    // with cItems = 0 and pcActualItems != NULL, GetEntryPointsList returns in
    // pcActualItems the number of items available. When called with cItems != 0,
    // GetEntryPointsList assumes that there is enough space in strList[] for that
    // many items, and fills up the array with those items (up to maximum available).
    // Returns in pcActualItems the actual number of items that could be put in the
    // array (this can be > or < cItems). Assumes that the caller takes care of
    // array allocation and de-allocation.
    STDMETHOD(GetEntryPointsList)(
            /* [in] */ ULONG cItems,
            /* [in, out, size_is(cItems)] */ BSTR strList[],
            /* [out, optional] */ ULONG *pcActualItems);

    // Between these calls, the project will notify the compiler of multiple
    //  file changes. This is really just an optimization so the compiler
    //  can interrupt the background compile thread once instead of having
    //  to interrupt it every time the project adds a file to the list of
    //  compiled things (which happens once for each file during project
    //  load).
    //
    STDMETHOD(StartEdit)();
    STDMETHOD(FinishEdit)();

#if IDE 

    // Lists all Form classes with an entry point. If called with cItems = 0 and
    // pcActualItems != NULL, GetEntryPointsList returns in pcActualItems the number
    // of items available. When called with cItems != 0, GetEntryPointsList assumes
    // that there is enough space in strList[] for that many items, and fills up the
    // array with those items (up to maximum available).  Returns in pcActualItems
    // the actual number of items that could be put in the array (this can be > or
    // < cItems). Assumes that the caller takes care of array allocation and de-allocation.
    STDMETHOD(GetFormEntryPointsList)(
            /* [in] */ ULONG cItems,
            /* [in, out, size_is(cItems)] */ BSTR strList[],
            /* [out, optional] */ ULONG *pcActualItems);

    // Non-interface helper for DeleteAllResourceReferences.  Used to stop
    // watching all of the (embedded and linked) resource files associated
    // with this project.
    void UnwatchAllResourceFiles();

    // Asks for file change notifications to be sent if any of this
    // project's (embedded or linked) resource files have changed
    // on disk.
    void SyncAllResourceFiles();
#endif IDE

    STDMETHOD(AdviseBuildStatusCallback)(
            /* [in] */ IVbBuildStatusCallback *pIVbBuildStatusCallback,
            /* [out] */ DWORD *pdwCookie);

    STDMETHOD(UnadviseBuildStatusCallback)(
            /* [in] */ DWORD dwCookie);

    STDMETHOD(StartDebugging)();
    STDMETHOD(StopDebugging)();

    STDMETHOD(ENCRebuild)(
            /* [in] */ IUnknown *pIDebugProgram,
            /* [out] */ IUnknown **ppENCUpdate);

    STDMETHOD(GetPEImage)(
    /* [out] */ void **ppImage );

    STDMETHOD(AddApplicationObjectVariable)(LPCOLESTR wszVariableName,
                                            LPCOLESTR wszQualifiedTypeName);

    STDMETHOD(RemoveAllApplicationObjectVariables)();

    // Called when the project is renamed. Returns NOERROR and changes the name
    // of the project if successful, or returns E_FAIL without changing the name
    // of the project if not.
    STDMETHOD(RenameProject)(LPCWSTR wszNewProjectName);

    // Blocks the foreground thread until the background is in bound state for this project.
    _Check_return_ STDMETHOD(WaitUntilBound)();

    // Perfroms a symbolic rename on the dafaultnamespace.
    STDMETHOD(RenameDefaultNamespace)(BSTR bstrDefaultNamespace);

    STDMETHOD(GetDefaultReferences)(ULONG cElements, BSTR *rgbstrReferences, ULONG *cActualReferences);

    // Disable posting compiler messages to avoid filling up the message queue.
    STDMETHOD(SuspendPostedNotifications)();

    // Enable posting compiler messages.
    STDMETHOD(ResumePostedNotifications)();

#ifdef EXTENDED_WARNING_GRANULARITY
    //warnings granularity support
    STDMETHOD(GetWarningIdAndStringTable)(/*out*/UINT* count, /*out*/VbCompilerWarningIdAndString** pTable);
    STDMETHOD(ValidateWrnId)(/*in*/DWORD wrnId, /*out*/BOOL* bIsValid);
#endif

    //========================================================================
    // IVbVbaCompiler implementation
    //========================================================================
    STDMETHOD(GetXMLFromSourceFile)(LPCWSTR pwszFileName, VARIANT_BOOL fDeclsOnly, BSTR* pbstrXML);
    STDMETHOD(GetVbaErrors)(UINT* pcErrors, struct VbError** ppErrors);
    STDMETHOD(CompileSucceeded)(VARIANT_BOOL* pfSucceeded);

    //========================================================================
    //IVsENCRebuildableProjectCfg2
    //========================================================================
    STDMETHOD(StartDebuggingPE)( void);
    STDMETHOD(EnterBreakStateOnPE)(ENC_BREAKSTATE_REASON encBreakReason,ENC_ACTIVE_STATEMENT *pActiveStatements,UINT32 cActiveStatements);
    STDMETHOD(BuildForEnc)(IUnknown *pUpdatePE);
    STDMETHOD(ExitBreakStateOnPE)( void);
    STDMETHOD(StopDebuggingPE)( void);
    STDMETHOD(GetENCBuildState)(ENC_BUILD_STATE *pENCBuildState);
    STDMETHOD(GetCurrentActiveStatementPosition)(UINT32 id,TextSpan *ptsNewPosition);
    STDMETHOD(GetPEidentity)(GUID *pMVID,BSTR *pbstrPEName);
    STDMETHOD(GetExceptionSpanCount)(UINT32 *pcExceptionSpan);
    STDMETHOD(GetExceptionSpans)(UINT32 celt, ENC_EXCEPTION_SPAN *rgelt, ULONG *pceltFetched);
    STDMETHOD(GetCurrentExceptionSpanPosition)(UINT32 id, TextSpan *ptsNewPosition);
    STDMETHOD(EncApplySucceeded)( HRESULT hrApplyResult);
    STDMETHOD(GetPEBuildTimeStamp)(FILETIME* pTimeStamp);

#if IDE
    //========================================================================
    //IVsLanguageServiceBuildErrorReporter
    //========================================================================
    STDMETHOD(ReportError)(BSTR bstrErrorMessage, BSTR bstrErrorId, VSTASKPRIORITY nPriority, long iLine, long iColumn, BSTR bstrFileName);
    STDMETHOD(ClearErrors)();

    //========================================================================
    //IVsLanguageServiceBuildErrorReporter2
    //========================================================================
    STDMETHOD(ReportError2)(BSTR bstrErrorMessage, BSTR bstrErrorId, VSTASKPRIORITY nPriority, long iStartLine, long iStartColumn, long iEndLine, long iEndColumn, BSTR bstrFileName);

#endif IDE

    //========================================================================
    // CodeModel and FileCodeModel creation functions
    //========================================================================
    STDMETHOD(CreateCodeModel) (void *pProject, void *pProjectItem, void **pCodeModel);
    STDMETHOD(CreateFileCodeModel) (void *pProject, void *pProjectItem, void **pFileCodeModel);

    // VSA helper (for "little" F5)
    STDMETHOD(GetMethodFromLine)(VSITEMID itemid, long iLine, BSTR *pBstrProcName, BSTR *pBstrClassName);

#if IDE 
    // Helper for Object Browser.
    STDMETHOD(AdviseFileInsertion)( IVbProjectEvents *, IAdviseSite **, VSCOOKIE * );
    STDMETHOD(UnadviseFileInsertion)(VSCOOKIE);
    // reattach IVsCodeWindow/IVsTextLines to sourcefile object
    STDMETHOD(FindDocDataAndDocViewByMoniker)( IVsHierarchy * pHier, LPCOLESTR pszMkDocument, IVsTextLines ** ppDoc, IVsCodeWindow ** ppView );

    //This supports intellesense in run-at-server script block and nuggets
    //like <%/%>,<%#/%>,<% /%>and so on. When this gets called, two IVsTextLines
    //should be created in pBufferCoordinator.
    STDMETHOD(GetContainedLanguage)(LPCWSTR,VSITEMID,IVsTextBufferCoordinator *,IVsContainedLanguage **ppContainedLanguage);


#if IDE 
    void FireOnClose ();
    void FireOnChange( BOOL, VBPROJECTNOTIFY );
    void FireOnFile  ( VBCOLLECTION, ICodeFile * );
    private:
    DynamicArray<IVbProjectEvents *> m_daFileInsertionEventListeners;
    public:

#endif

    void ReleaseCodeModel();

#endif

    // Whether we have scanned all of the metadata files in this project to see if they contain
    // <InternalsVisibleTo()> or PIA attributes
    bool CheckedAllMetadataFilesForAccessibility()
    {
        return m_CheckedAllMetadataFilesForAccessibility;
    }

    // Indicate whether we have scanned all of the metadata files in this project to see if they contain
    // <InternalsVisibleTo()> or PIA attributes
    void SetCheckedAllMetadataFilesForAccessibility( bool checkDone )
    {
        m_CheckedAllMetadataFilesForAccessibility = checkDone;
    }

    // Whether we should import private types/members during MetaImport.  We don't need to import privates
    // if the assembly isn't a NOPIA assembly.
    bool ShouldImportPrivates()
    {
        VSASSERT( m_CheckedAllMetadataFilesForAccessibility, "We don't know the answer to this yet. See MetaImport::CheckWhetherToImportInaccessibleMembers()" );
        return m_ShouldImportPrivates;
    }

    // Indicate whether we need to import private members/types for this referenced assembly
    void SetShouldImportPrivates(bool ShouldImportPrivates)
    {
        m_ShouldImportPrivates = ShouldImportPrivates;
    }

private:

    // Perform all cleanup logic.
    void Invalidate();

    //========================================================================
    // The following methods should only be called from the project and
    // file edit methods and should never be called directly.
    //========================================================================

    friend class CompilerPackage;
    friend class Compiler;
    friend class CompilerHost;
    friend class SharedContext;
    friend class WritePEHelper;
#if HOSTED
    friend class VbHostedCompiler;
    friend class VBHostedSession;
#endif

    // Decide how much work is needed to compile this project.
    unsigned CountStepsToCompile();

    // Do all of the work to compile this project.
    bool Compile();

    // Do the work necessary to compile this project to declared.
    bool CompileToDeclared();

    // Do the work necessary to compile this project to Bound.
    bool CompileFromDeclaredToBound();

    // Do the work necessary to compile this project from Bound to Compiled.
    bool CompileFromBoundToCompiled();


    // Do all of the final compilation work.  This should only be called
    // if the project has no errors.
    //
    void FinishCompile();
    bool BeginCompileToDiskAsync();
    void EndCompileToDiskAsync(bool fCompiledSuccessfully, ErrorTable *errors);

#if IDE 

    // Clear any ParseTree Caches we may have
    void ClearParseTreeCaches();

    // Remove the given compiler file.
    //
    void RemoveCompilerFile(CompilerFile* pCompilerFile);

    HRESULT GetEntryPointsList(bool fFormsOnly, ULONG cItems, BSTR strList[], ULONG *pcActualItems);

    bool CanFormClassHaveEntryPoint(BCSYM_NamedRoot *pNamed);
    HRESULT CacheEntryPointsList();

#endif // IDE

    void ValidateOptions();

public:

    //========================================================================
    // CompilerProject Implementation.
    //========================================================================

    // Should we try to compile this project?
    bool IsInitialized()
    {
        return m_fInitialized;
    }

    // Is this a metadata project?
    bool IsMetaData()
    {
        return m_fMetaData;
    }

    // Returns the CompilerHost for this project.
    CompilerHost *GetCompilerHost()
    {
        return m_pCompilerHost;
    }

    // Returns PEBuilder for this project.
    PEBuilder *GetPEBuilder()
    {
        return &m_Builder;
    }

    // Is this project importing/building a code module?
    bool IsCodeModule()
    {
        return m_fCodeModule;
    }

    MetaDataFile *GetPrimaryModule()
    {
        if (m_pfileList && m_pfileList->IsMetaDataFile())
        {
            return m_pfileList->PMetaDataFile();
        }

        return NULL;
    }
    
    mdAssembly GetAssemblyToken()
    {
        VSASSERT( IsMetaData(),
                        "Why is an assembly token being requested from a non-metadata project ?");

        return m_mdassembly;
    }

#if IDE 
    ULONG GetFileCount()
    {
        return m_treeFiles.Count();
    }
    
    bool AreNotificationsSuspended()
    {
        return m_fShouldSuspendNotifications;
    }
#endif IDE

    // Returns true if the given metadata file is represented by this CompilerProject
    bool FMetadataProjectRepresentsFile(_In_z_ STRING *pstrFileName);

    // Make sure bases and implements resolved
    void EnsureBasesAndImplementsLoaded();

    // If metadata, have the base and implements been resolved?
    bool GetBasesAndImplementsLoaded()
    {
        VSASSERT(IsMetaData(), "BasesAndImplementsLoaded only makes sense on metadata projects");
        return m_fBasesAndImplementsLoaded;
    }

    // Compares 2 BSTRs
    static int _cdecl SortBSTRs(const void *arg1, const void *arg2);

    // Gets the containing compiler.
    Compiler *GetCompiler()
    {
        return m_pCompiler;
    }

    // Synchronously get the project's current compilation state.
    // Remark: Inside CompilerProject, we directly read the member m_cs since we never set the
    //         compilation state from other threads.  We provide synchronized read/write of
    //         the compilation state so that the foreground can read it.
    CompilationState GetCompState()
    {

#if IDE 
        CompilationState cs;

        // Synchronously read compilation state
        m_CompStateCriticalSection.Enter();

        cs = m_cs;

        m_CompStateCriticalSection.Leave();

        return cs;
#else
        return m_cs;
#endif IDE

    }

#if IDE 
    // Returns the least of the current compilation state and the
    // decompilation state. This is used to determine if a project needs
    // to be decompiled. (If a project has already been demoted to a
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

    // Get the List of files that need to have their statement changed event fired
    FireOnStatementChangedList *GetFireStatementChangedList()
    {
        return &m_StatementChangedList;
    }

    bool IsProjectLevelBoundInformationReady()
    {
        return IsProjectLevelInformationReady;
    }

    ULONG GetSourceChangeIndex()
    {
        return m_iSourceChangeIndex;
    }

    void IncrementSourceChangeIndex()
    {
        m_iSourceChangeIndex++;
    }

    ULONG GetCurrentDeclChangeIndex()
    {
        return m_iDeclChangeIndex;
    }

    ULONG GetCurrentBoundChangeIndex()
    {
        return m_iBoundChangeIndex;
    }

    // Waits until the project is in a particular state.
    _Check_return_ HRESULT WaitUntilBindableState(WaitStatePumpingEnum ShouldPump, WaitStateCancelEnum CanCancel) 
    { 
        return WaitUntilState(CS_Bound, ShouldPump, CanCancel); 
    }
    _Check_return_ HRESULT WaitUntilTypesEmittedState(WaitStatePumpingEnum ShouldPump, WaitStateCancelEnum CanCancel) 
    { 
        return WaitUntilState(CS_TypesEmitted, ShouldPump, CanCancel); 
    }
    _Check_return_ HRESULT WaitUntilCompiledState(WaitStatePumpingEnum ShouldPump, WaitStateCancelEnum CanCancel) 
    { 
        return WaitUntilState(CS_Compiled, ShouldPump, CanCancel); 
    }
    _Check_return_ HRESULT WaitUntilDeclaredState(WaitStatePumpingEnum ShouldPump, WaitStateCancelEnum CanCancel) 
    { 
        return WaitUntilState(CS_Declared, ShouldPump, CanCancel); 
    }

    bool DoesProjectDependOnName(_In_z_ STRING *pstrName, CompilationState *pcs);
#endif IDE

    bool HasErrors();

    //-------------------------------------------------------------------------
    // The following functions are optimized for use by the compiler during
    // very specific stages of compilation. They are not general helpers
    // for IDE use.
    bool HasErrors_AllowCaching();

    // Friends.

    void AddUnprocessedFriend( _In_z_ STRING* sAssembly, SourceFile* pSourceFile, Location* pLoc );
    bool CheckFriendship( CompilerProject* potentialFriendProject );
    void AddFriend( _In_z_ STRING* sAssembly, SourceFile* pSourceFile, Location* pLoc );
    bool VerifyFriendDeclaration( _In_z_ STRING* pstrAssembly, SourceFile* pSourceFile, Location* pLoc );
    bool HasFriends()
    {
        return GetFriendAssembliesCount() > 0;
    }
    int GetFriendAssembliesCount()
    {
        return m_Friends.NumEntries();
    }
    void CommitMissedBindingOpportunity();
    void ClearMissedBindingOpportunity()
    {
        m_TempMissedBindingOpportunities.Reset();
    }

    void ClearAllMissedBindingOpportunities()
    {
        m_TempMissedBindingOpportunities.Reset();
        m_MissedBindingOpportunities.Reset();
    }

private:
    _Check_return_ HRESULT WaitUntilState(CompilationState csState, bool ShouldPump = true, bool AllowCancel = true);

#if IDE 
    _Check_return_ HRESULT WaitUntilState(CompilationState csState, WaitStatePumpingEnum ShouldPump, WaitStateCancelEnum CanCancel);
#endif

    HRESULT SetVBRuntimeOptions(VbCompilerOptions *pCompilerOptions);

    bool FillInAssemblyIdentity( _In_z_ STRING* sAssembly, AssemblyIdentity* assembly );
    void VerifyExtensionAttributeDefinition();
    void VerifyEmbeddedReferenceAttributes();
    void ProcessFriendDeclarations();
    void AddMissedBindingOpportunity( _In_z_ STRING* strDeclaringAssemblyName, AssemblyIdentity* FriendDeclaration );
    void VerifyMissedBindingOpportunities();
    void ClearFriends();

    bool IsMetadataProjectWithErrors()
    {
        return IsMetaData() && m_ErrorTable.HasErrors();
    }

private:
    bool CacheErrorsInReferences();
    bool CacheErrorsInCurrentProject();

    bool HasErrorsInReferences();
    bool HasErrorsInFilesOrTable();
    //-------------------------------------------------------------------------

public:

    STRING *GetAssemblyName()
    {
        return m_AssemblyIdentity.GetAssemblyName();
    }

    AssemblyIdentity *GetAssemblyIdentity()
    {
        return &m_AssemblyIdentity;
    }

    AssemblyIdentity *GetModuleAssemblyIdentity()
    {
        VSASSERT( IsCodeModule(), "Should only call this from a code module project!" );
        return &m_ModuleAssemblyIdentity;
    }

    void* GetPublicKey();

    // Contains the name we'll use for the scope when
    // generating the metadata for this project.
    //
    void SetScopeName(_In_z_ STRING *pstrScopeName)
    {
        VSASSERT(m_pstrScopeName == NULL || m_pstrScopeName == pstrScopeName,
            "Trying to change project's scope name!");
        m_pstrScopeName = pstrScopeName;
    }

    STRING *GetScopeName()
    {
        return m_pstrScopeName;
    }

    STRING *GetFileName()
    {
        return m_pstrFileName;
    }

    // Returns the file name with no path from m_strFileName.
    STRING *GetFileNameWithNoPath();

    STRING *GetPEName()
    {
        return m_pstrPEName;
    }

    STRING *GetXMLDocFileName()
    {
        return m_pstrXMLDocCommentFileName;
    }

    STRING *GetPEPath()
    {
        return m_pstrPEPath;
    }

    // Name to use when reporting errors against this project.
    // Note that this does NOT return a STRING, just a WCHAR*.
    WCHAR *GetErrorName();

    bool OptionCompareText()
    {
        return m_fOptionCompareText;
    }

    bool OptionStrictOff()
    {
        return m_fOptionStrictOff;
    }

    bool OptionExplicitOff()
    {
        return m_fOptionExplicitOff;
    }

    bool OptionInferOff()
    {
        return m_fOptionInferOff;
    }

    // True if the XMLDoc comment feature is turned on, false otherwise.
    bool IsXMLDocCommentsOn()
    {
        return (m_pstrXMLDocCommentFileName ? true : false);
    }

    // Retrieves the XML for the whole project.
    HRESULT GetXMLDocForProject(BaseWriter *pXMLDocForProject);

    // Write out the XMLDoc file to the indicated filename.
    void WriteToXMLDocFile();
    void GenerateXMLDocFile();

    bool RemoveIntChecks()
    {
        return m_fRemoveIntChecks;
    }

    bool GetInMemoryCompilation()
    {
        return m_fInMemoryCompilation;
    }

    LANGVERSION GetCompilingLanguageVersion()
    {
        return m_LangVersion;
    }

#if DEBUG
    // DEBUG-only helper that returns the project's name.  The returned string
    // has no intrinsic semantic value -- it's just cobbled-together bits
    // and pieces that make debugging a little bit easier.
    STRING *DebGetName();
#endif

#if FV_TRACK_MEMORY
    void DumpProjectMemoryUse();
#endif

    STRING *GetDefaultNamespace()
    {
        return m_pstrDefaultNamespace;
    }

    void SetDefaultNamespace(_In_z_ STRING* pstrDefaultNamespace)
    {
        m_pstrDefaultNamespace = pstrDefaultNamespace;
    }

    bool CompiledSuccessfully()
    {
        return m_fCompiledSuccessfully;
    }

    HRESULT GetRootOfPEFileName(_Deref_out_opt_z_ STRING **ppstrName);

    STRING *GetStartupType()
    {
        return m_pstrStartupType;
    }

    bool GeneratePDB()
    {
        return m_fGeneratePDB;
    }

    bool GenerateDebugCode()
    {
        return m_fGenerateDebugCode;
    }

    bool GenerateOptimalIL()
    {
        return m_fGenerateOptimalIL;
    }

    bool GenerateENCableCode()
    {
        return m_fGenerateENCableCode;
    }

    WARNING_LEVEL GetDefaultWarningLevel()
    {
        return m_WarningLevel;
    }

    WARNING_LEVEL GetWarningLevel(ERRID errid);

    DynamicArray<VbCompilerWarningItemLevel> *GetWarningLevelTable()
    {
        return &m_daWarningsLevelTable;
    }

    size_t GetLoadAddress()
    {
        return m_dwLoadAddress;
    }

    DWORD GetFileAlignment()
    {
        return m_dwAlign;
    }

    DWORD GetDefaultCodePage()
    {
        return m_dwDefaultCodePage;
    }

    VbCompilerOutputTypes GetOutputType()
    {
        return m_OutputType;
    }

    void SetOutputType(VbCompilerOutputTypes OutputType)
    {
        m_OutputType = OutputType;
    }

    // Some common helpers for handling different returns from GetOutputType
    bool OutputIsNone()   { return m_OutputType == OUTPUT_None; }
    bool OutputIsExe()    { return m_OutputType == OUTPUT_WindowsEXE || m_OutputType == OUTPUT_ConsoleEXE || m_OutputType == OUTPUT_AppContainerEXE; }
    bool OutputIsLibrary(){ return m_OutputType == OUTPUT_Library || m_OutputType == OUTPUT_WinMDObj; }
    bool OutputIsModule() { return m_OutputType == OUTPUT_Module; }

    bool FEmitAssembly()
    {
        // All output types except module build a full assembly
        return GetOutputType() != OUTPUT_Module;
    }

#if IDE 
    bool IsDebuggingInProgress();
#endif IDE

    bool GetNoStandardLibraries()
    {
        return m_fNoStandardLibraries;
    }

    // Returns the symbol that stores the cond comp symbols
    BCSYM_CCContainer *GetProjectLevelCondCompScope()
    {
        return this ? m_ProjectConditionalCompilationScope : NULL;
    }

    IUnknown * GetPunkProject()
    {
        return m_punkProject;
    }

#if IDE 
    IVsHierarchy * GetHierarchy()
    {
        return m_pHierarchy;
    }

    ENCBuilder *GetENCBuilder()
    {
        return m_pENCBuilder;
    }
    bool IsInDebugBreak()
    {
        return m_fIsInDebugBreak;
    }

    bool IsRegistered()
    {
        return m_isRegistered;
    }


    IVsContextualIntellisenseFilter *GetContextualIntellisenseFilter();

    HRESULT GetInternalCodeModel(CCodeModel **ppCodeModel);
    HRESULT GetCodeModel(CodeModel **ppCodeModel);
    HRESULT GetExternalCodeModel(CExternalCodeModel **ppCodeModel);

    XmlIntellisenseSchemas * GetIntellisenseSchemas();
    bool UpdateXmlSchemas();
    XmlIntellisenseSchemas * GetOrFlagIntellisenseSchemas();

    bool IsIntellisenseSchemasOpened()
    {
        return m_XmlSchemas.IsOpen();
    }

    bool NeedToInitIntellisenseSchemas();

    bool IsVenusMiscProject();
    bool IsVenusMiscProjectOrPageInWapProject();

#endif IDE

    IVbBuildStatusCallback *GetBuildStatusCallBack()
    {
        return m_pBuildStatusCallBack;
    }

    IStream *GetPDBStream()
    {
        return m_pStreamPDB;
    }

    // Get the assembly importer.
    IMetaDataAssemblyImport *GetAssemblyImport()
    {
        if (m_fMetaData)
        {
            if (!m_pAssemblyImport)
            {
                CreateAssemblyImport();
            }
            return m_pAssemblyImport;
        }
        else
        {
            return m_Builder.GetAssemblyImport();
        }
    }

    // Release the assembly importer

    void ReleaseAssemblyImport()
    {
        VSASSERT(m_fMetaData, "only works on metadata projects!");
        RELEASE(m_pAssemblyImport);
    }

    // Get the errors table for this project.
    ErrorTable *GetErrorTable()
    {
        return &m_ErrorTable;
    }

    // Get the project-level imports
    ImportedTarget *GetImportedTargets()
    {
        return m_Imports.GetAllImportedTargets();
    }

    // Used by the background thread to ask the foreground to run the upgrade.
    void TellForegroundToUpgrade();


    // Does the current project reference this one?
    bool IsProjectReferenced(CompilerProject *pProject, bool fAllowAssemblyIdentityMatch = false);

#if IDE 
    bool IsReferencedByReferencingVBProjects
    (
        CompilerProject *pProject,
        CompilerProject **ppFirstReferencingProject = NULL,
        CompilerProject **ppFirstReferencingVenusProject = NULL
    );

    CompilerProject *GetFirstReferencingVenusProject();
#endif IDE

    CompilerProject *GetFirstReferencingProject()
    {
        if (m_daReferencesOnMe.Count() > 0)
        {
            return m_daReferencesOnMe.Element(0);
        }

        return NULL;
    }

    // Add a reference between two projects.
    bool AddReference(CompilerProject *pProjectReferenced);
    bool AddReference(CompilerProject *pProjectReferenced, UINT ReferenceFlags);

    // ReleaseOneReference is a helper for ReleaseAllReferences.
    //
    // If fShrink is false, the DynamicArray is not compressed after the
    // reference is removed, so the caller is responsible for cleaning up
    // that array slot.
    //
    void ReleaseOneReference
    (
        DynamicArray<ReferencedProject> *prgRefs,
        unsigned iRef,      // Index of reference to remove
        bool fShrink        // Shrink the DynamicArray after deleting ref?
#if IDE 
        , CAssemblyReferenceManager *pAsmLibMgr // Optional ref manager (if NULL, we will fetch it)
#endif
    );

    void ReleaseAllReferences(DynamicArray<ReferencedProject>* pArray);

    void ReleaseReferences(DynamicArray<ReferencedProject>* pArray, bool RequiredOnly);

    // Add a reference to a metadata project
    HRESULT AddMetaDataReferenceInternal(LPCWSTR wszFilePath, bool bAssembly, UINT ReferenceFlags, bool* pAdded);
    HRESULT AddProjectReferenceInternal(IVbCompilerProject *pReferencedCompilerProject, UINT ReferenceFlags);

    bool AddRequiredLibraries();
    bool AddStandardLibraries();
#if IDE 
    bool RemoveStandardLibraries();
#endif

#if IDE 
    // Reference usage tracking
    //

    // Starts tracking references usage.  Decompile project to Declared and track usages through compilation.
    void StartGetUnusedReferences();

    // Stops tracking reference usages.
    void StopGetUnusedReferences();

    // Are we tracking reference usages?
    bool IsGettingUnusedReferences();

    // Flags referenced projects associated with a symbol, it's bases & implements, and
    // params & return value as used.
    void SetReferenceUsageForSymbol(BCSYM_NamedRoot *DependedOn);

    // Get a lists unused references as a delimited string
    HRESULT GetUnusedReferences(_Out_ _Deref_post_cap_(*cchLength) WCHAR **pReferencePaths, ULONG *cchLength);

    // Releases the MetaData CompilerProject.  Used when juggling a project's references
    // (e.g., DeleteAllMetaDataReferences during building, and when CopyLocal notifications
    // are received).
    void ReleaseMetaDataProject();

    // These methods do all the real work for OnBeforeCopyLocal/OnAfterCopyLocal and
    // OnBeforeDeleteLocal/OnAfterDeleteLocal.
    STDMETHODIMP OnBeforeCopyOrDeleteLocal(LPCWSTR wszFilename);
    STDMETHODIMP OnAfterCopyOrDeleteLocal(LPCWSTR wszFilename);
#endif

    // Routines for determining the entrypoint
    bool CouldHaveSyntheticSubMain(BCSYM_Class *pClass, Compiler *pCompiler);
    bool IsClassVisible(BCSYM_NamedRoot *pClass);
    void FindAllMainsInAClass(BCSYM_Class *ClassToCheck, DynamicArray<SubMainInfo> *SubMainList, bool AllowDiggingIntoBaseClasses);
    bool FindClassMain(_In_z_ STRING *Startup, DynamicArray<SubMainInfo> *SubMainList, ErrorTable *pErrorTable);
    void FindAllSubMainInProject(DynamicArray<SubMainInfo> *SubMainList, bool IncludeWinFormMains);

#if ID_TEST
    // Dumps a string to a file
    static void DumpXMLToFile(Compiler *pCompiler,
                _In_opt_z_ WCHAR *pszFileName, _In_opt_z_ const WCHAR *pszFileExtention, _In_opt_z_ WCHAR *pwszBuffer);
#endif ID_TEST

#if IDE 
    void DisconnectInternal();
#endif

private:

    //****************************************************************************
    // Project compilation/decompilation methods.
    //
    // These methods should never be called directly, only as a side-effect of
    // decompling a file.
    //****************************************************************************

    friend class CompilerFile;
    friend class CommitEditList;
    friend class EditFilter;

    // Promote the project.
    bool _PromoteToDeclared();
    bool _PromoteToBound();
    bool _PromoteToTypesEmitted();
    bool _PromoteToCompiled();

    void
    FinishPromoteToBoundForFile
    (
        CompilerFile *pfile
        // This array holds a list of all sourcefiles that got promoted from Declared to Bound state.
#if IDE 
        ,DynamicArray<CompilerFile*> &ListOfPromotedSourceFiles
#endif
    );

#if IDE 
    void DemoteReferencingProjects(CompilationState CompState);

    // Demote the project.
    void _DemoteToNoState();
    void _DemoteToDeclared();
    void _DemoteToBound();
    void _DemoteToTypesEmitted();

    // Special entrypoint for metadata projects
    void _DemoteMetaDataProjectToNoState(bool ReleaseMetaDataImport);

    // Demotes all metadata projects to No_State
    void _DemoteAllMetaDataProjectsToNoState();

public:
    // Clears the project level information setup during the transition from declared to bound
    void ClearProjectDeclaredToBoundData();
#endif IDE

private:
    void RemoveNamespaces();


    // Process a file that has moved between states.
    void _ProcessPromotion(CompilerFile *pfile, CompilationState cs);

#if IDE 
    void _DemoteProject(CompilationState cs);
    void _ProcessDemotion(CompilerFile *pfile, CompilationState cs);
#endif IDE

#if IDE 
    void DumpTasklistForENCSuites();
#endif

    // Synchronously set the project's current compilation state.
    void SetCompState(CompilationState cs)
    {

#if IDE 
        // Synchronously set compilation state
        m_CompStateCriticalSection.Enter();

        m_cs = cs;

        m_CompStateCriticalSection.Leave();
#else
        m_cs = cs;
#endif IDE

    }

#if IDE 
    STRING *GetAssemblyVersionFromDisk();
#endif

public:
    // Create default and declared project level conditional compilation symbols
    HRESULT CreateProjectLevelCondCompSymbols(ErrorTable *pErrorTablef);
    void ClearProjectLevelCondCompSymbols() { SetProjectLevelCondCompSymbols(NULL); }

private:
    void SetProjectLevelCondCompSymbols(_In_ BCSYM_CCContainer* pScope)
    {
        // Use the following functions to make setting m_ProjectConditionalCompilationScope safe.  Otherwise there is a 
        // possible race condition where the background thread would set m_ProjectConditionalCompilationScope to 
        // partially initialized memory, which the foreground thread could incorrectly read.
        MemoryBarrier();
        InterlockedExchangePointer((PVOID *)&m_ProjectConditionalCompilationScope, pScope);
    }
    void SetDeclaredToBoundCompilationInProgress(bool fDeclaredToBoundCompilationInProgress)
    {
        m_fDeclaredToBoundCompilationInProgress = fDeclaredToBoundCompilationInProgress;
    }

    void CheckCLSCompliance(ErrorTable *pProjectLevelErrors);

    void CheckForNamespaceNameCasingMismatches();
    void CheckForNamespaceNameCasingMismatches(BCSYM_Namespace *pNamespaceToCheck, NamedRootTree *CheckedNamespaceRings, BCSYM_Namespace *pRootNamespace);

#if IDE 
    bool ProjectClaimedCLSCompliancePreviously()
    {
        return m_fProjectClaimedCLSCompliancePreviously;
    }

    void SetProjectClaimedCLSCompliancePreviously(bool fClaimedCLSCompliancePreviously)
    {
        m_fProjectClaimedCLSCompliancePreviously = fClaimedCLSCompliancePreviously;
    }

    // Start of decompilation flags accessors used by EditFilter
    // ==========================================================

    bool ProjectAndFilesDecompToBoundStarted()
    {
        return m_fProjectAndFilesDecompToBoundStarted;
    }

    void SetProjectAndFilesDecompToBoundStarted(bool Started)
    {
        m_fProjectAndFilesDecompToBoundStarted = Started;
    }

    bool ProjectAndFilesDecompToDeclaredStarted()
    {
        return m_fProjectAndFilesDecompToDeclaredStarted;
    }

    void SetProjectAndFilesDecompToDeclaredStarted(bool Started)
    {
        m_fProjectAndFilesDecompToDeclaredStarted = Started;
    }

    // ==========================================================
    // End of decompilation flags accessors used by EditFilter
#endif

#if DEBUG
    void VerifyNoDuplicateReferences();

    void ValidateAllReferences();

    bool AreImportsBeingResolved()
    {
        return m_fAreImportsBeingResolved;
    };

    void SetImportsBeingResolved(bool fBeingResolved)
    {
        m_fAreImportsBeingResolved = fBeingResolved;
    }
#endif DEBUG

    void SetImportsBeenResolved(bool fResolved)
    {
        m_fHaveImportsBeenResolved = fResolved;
    }

public:

    bool HaveImportsBeenResolved()
    {
        return m_fHaveImportsBeenResolved;
    }

    void ResolveProjectLevelImports(CompilationCaches *pCompilationCaches);

    bool IsDeclaredToBoundCompilationInProgress()
    {
        return m_fDeclaredToBoundCompilationInProgress;
    }

    bool ProjectClaimsCLSCompliance()
    {
        return m_fProjectClaimsCLSCompliance;
    }

    void SetProjectClaimsCLSCompliance(bool fClaimsCLSCompliance)
    {
        m_fProjectClaimsCLSCompliance = fClaimsCLSCompliance;
    }

#if IDE 
    CompilerFile *FileThatCausedProjectsCLSComplianceClaim()
    {
        return m_pFileThatCausedProjectsCLSComplianceClaim;
    }

    void SetFileThatCausedProjectsCLSComplianceClaim(CompilerFile *pFile)
    {
        m_pFileThatCausedProjectsCLSComplianceClaim = pFile;
    }

    CompilerProjectSite *m_pSinkSite;

    bool IsSnippetProject()
    {
        return m_fIsSnippetProject;
    }

    void SetSnippetProject()
    {
        m_fIsSnippetProject = true;
    }

    bool IsContainedLanguageProject()
    {
        return m_fContainedLanguageProject;
    }

    bool IsVenusProject()
    {
        return m_fVenusProject;
    }

    bool IsVenusAppCodeProject()
    {
        return m_fVenusAppCodeProject;
    }
#endif IDE

#if IDE 
    CompilerFile *FindFileByName(_In_opt_z_ STRING *strFileName);
    SourceFile *FindSourceFileByName(_In_opt_z_ STRING *strFileName);
    ULONG GetNumFiles() { return m_treeFiles.Count(); }
#endif IDE

    STRING *GetProjectSettingForKeyFileName()
    {
        return m_pstrProjectSettingForKeyFileName;
    }

    void SetProjectSettingForKeyFileName(_In_opt_z_ STRING *pstrKeyFileName);

    STRING *GetProjectSettingForKeyContainerName()
    {
        return m_pstrProjectSettingForKeyContainerName;
    }

    void SetProjectSettingForKeyContainerName(_In_opt_z_ STRING *pstrKeyContainerName);

    bool GetDelaySign()
    {
        return m_fDelaySign;
    }

    void SetDelaySign(bool fValue)
    {
        m_fDelaySign = fValue;
    }

    SourceFile *GetFileWithAssemblyKeyFileAttr()
    {
        return m_pFileWithAssemblyKeyFileAttr;
    }

    void SetFileWithAssemblyKeyFileAttr(SourceFile *pFile, _In_opt_z_ STRING *pstrKeyFileName)
    {
        if (pFile)
        {
            if (!m_AssemblyIdentity.GetKeyFileName() || m_fKeyFileFromAttr)
            {
                m_AssemblyIdentity.SetKeyFileName(pstrKeyFileName);
                m_fKeyFileFromAttr = true;
            }
        }
        else
        {
            if (m_fKeyFileFromAttr)
            {
                m_AssemblyIdentity.SetKeyFileName(NULL);
                m_fKeyFileFromAttr = false;
            }
        }

        m_pFileWithAssemblyKeyFileAttr = pFile;
    }

    SourceFile *GetFileWithAssemblyKeyNameAttr()
    {
        return m_pFileWithAssemblyKeyNameAttr;
    }

    void SetFileWithAssemblyKeyNameAttr(SourceFile *pFile, _In_opt_z_ STRING *pstrKeyName)
    {
        if (pFile)
        {
            if (!m_AssemblyIdentity.GetKeyContainerName() || m_fKeyNameFromAttr)
            {
                m_AssemblyIdentity.SetKeyContainerName(pstrKeyName);
                m_fKeyNameFromAttr = true;
            }
        }
        else
        {
            if (m_fKeyNameFromAttr)
            {
                m_AssemblyIdentity.SetKeyContainerName(NULL);
                m_fKeyNameFromAttr = false;
            }
        }

        m_pFileWithAssemblyKeyNameAttr = pFile;
    }

    SourceFile *GetFileWithAssemblyVersionAttr()
    {
        return m_pFileWithAssemblyVersionAttr;
    }

    void SetFileWithAssemblyKeyVersionAttr(SourceFile *pFile, _In_opt_z_ STRING *pstrVersion)
    {
#if IDE
        VSASSERT(pFile || pstrVersion == NULL, "Version unexpected from NULL file!!!");
#endif IDE
        m_AssemblyIdentity.SetAssemblyVersion(pstrVersion);

        m_pFileWithAssemblyVersionAttr = pFile;
    }

    STRING *GetKeyFileName()
    {
        return m_AssemblyIdentity.GetKeyFileName();
    }

    STRING *GetKeyContainerName()
    {
        return m_AssemblyIdentity.GetKeyContainerName();
    }

    void ClearFileCompilationArray();

    DynamicArray<SourceFile *> *GetFileCompilationArray();

private:

#if ID_TEST

    //****************************************************************************
    // Dump methods.
    //****************************************************************************

    // XML
    void DumpXML();

#endif ID_TEST
    //============================================================================
    // Private helper for CompilerProject::GetAssemblyImport. Creates the
    // assembly importer if it doesn't exist.
    //============================================================================
    void CreateAssemblyImport();

    //============================================================================
    // Private helper for CompilerProject::InitWithMetaData.  If the given file
    // contains an assembly manifest that has metadata-containing FileRefs,
    // create a new MetaDataFile for each FileRef and add to this CompilerProject.
    //============================================================================
    HRESULT AddAssemblyFileRefs(_In_z_ STRING *pstrFileName, bool bAssembly);

#if IDE
public:
    // Returns alternative path for search in Object Browser (to support Multi-Targeting hack).
    STRING * GetAlternateBrowserSearchPath() const
    {
        return m_pstrAlternateBrowserSearchPath;
    }

    // Retreives target framework moniker for this project.
    const CComBSTR & GetTargetFrameworkMoniker() const
    {
        return m_bstrTargetFrameworkMoniker;
    }

    // Returns whether this project has converted references - i.e.
    // referneces to mscorlib etc were retargeted to Full profile assemblies.
    bool UseAlternatePathForSpecialAssemblies() const
    {
        return m_useAlternatePathForSpecialAssemblies;
    }

    bool IsValid() const
    {
        return m_fIsValid;
    }

private:
    // Updates assembly reference path to support multi-targeting scenarios.
    STRING * FixUpReferencePathForMultiTargeting(_In_z_ STRING * strFilePath);
    void SetTargetFrameworkMoniker(_In_opt_ IVsHierarchy * pVsHierarchy);

    CComBSTR m_bstrTargetFrameworkMoniker;
    STRING * m_pstrAlternateBrowserSearchPath;
    bool m_useAlternatePathForSpecialAssemblies;

    // Initially True, False after Invalidate() has been called
    bool m_fIsValid;
#endif IDE

private:

    //
    // Data members.
    //

    friend struct FileIterator;
    friend struct SourceFileIterator;
    friend struct MetaDataFileIterator;
    friend class CompilerFile;
    friend class SourceFile;
    friend class MetaDataFile;
    friend class PEBuilder;
    friend class MetaEmit;

    // Current refcount.
    ULONG m_cRefs;

    // A CompilerProject can exist for a period in time
    // before the project system initializes it.  Don't try
    // to compile it as we'll probably get the compilation wrong.
    //
    bool m_fInitialized;

    // If metadata, have the base and implements been resolved?
    bool m_fBasesAndImplementsLoaded;

    // Is this a metadata project.
    bool m_fMetaData;

    // Is this project importing/building a code module?
    bool m_fCodeModule;

    bool m_fIsCompilationArraySorted;
    DynamicArray<SourceFile *> m_daFileCompilationArray;

    // If metadta, the token of the imported assembly
    mdAssembly m_mdassembly;

    // Whether to import inaccessible types when we import the dll. Defaults to true. 
    // Determined for the project when a metadata project reference is added.
    bool m_ShouldImportPrivates;

    // Whether we have looked at all the metadatafiles in the project and checked them for attributes that affect accessibility
    bool m_CheckedAllMetadataFilesForAccessibility;

#if IDE 

    bool IsProjectLevelInformationReady;
#endif

    // The containing compiler.
    Compiler *m_pCompiler;

    // CompilerHost for which this project belongs.
    CompilerHost *m_pCompilerHost;

    // A unique name for this project.
    STRING *m_pstrFileName;

    // The assembly identity for the project.
    AssemblyIdentity m_AssemblyIdentity;

    // The assembly identity representing the name given to a module.
    AssemblyIdentity m_ModuleAssemblyIdentity;

    // The name of the scope for this assembly.
    STRING *m_pstrScopeName;

    // The final PE name and path.
    STRING *m_pstrPEName;
    STRING *m_pstrPEPath;

    // The file name in m_strFileName without the path.
    STRING *m_pstrFileNameNoPath;
    // The value of m_strFileName when file name with no path was previously requested.
    STRING *m_pstrFileNameDuringPrevNoPathReq;

    // The XMLDoc file name
    STRING *m_pstrXMLDocCommentFileName;

    // The default namespace for types note declared within a Namespace statement
    STRING *m_pstrDefaultNamespace;

    // The Startup type.
    STRING *m_pstrStartupType;

    // The set of conditional compilation constants.
    STRING *m_pstrConditionalCompilationConstants;

    bool m_fOptionCompareText;
    bool m_fOptionExplicitOff;
    bool m_fOptionStrictOff;
    bool m_fOptionInferOff;

    bool m_fInMemoryCompilation;

    // Int overflow checking
    bool m_fRemoveIntChecks;

    // True if this project is fully compiled.
    bool m_fCompiledSuccessfully;

    // True iff this project output has been marked as too long
    bool m_fIsTooLong;

    //
    // Assembly information.
    //

    // Where we're loading the assembly from.
    IMetaDataAssemblyImport *m_pAssemblyImport;

    // We will build a hash table of friends.
    // The key will be the friend's assembly name. Thus, there may be
    // multiple entries in the hash table if multiple friends have the same name
    // but different public keys.
    NorlsAllocator m_norlsHashTables;
    FixedSizeHashTable< FRIEND_HASHTABLE_SIZE, STRING_INFO*, AssemblyIdentity* > m_Friends;

    // This is the public key token of the friend delcaration of a declaring
    // assembly that we hope to opportunistically match. The lifetime of this list
    // is always < the life time of the friend list above.
    // So we can safely store a pointer to the AssemblyIdentity in the friend list.

    struct MissedBindings
    {
        STRING* DeclaringAssemblyName;
        AssemblyIdentity* FriendDeclaration;
    };

    DynamicArray< MissedBindings > m_MissedBindingOpportunities;
    DynamicArray< MissedBindings > m_TempMissedBindingOpportunities;

#if IDE 

    // Threading locks.
    EventSignal m_eventBoundState;
    EventSignal m_eventTypesEmittedState;
    EventSignal m_eventCompiledState;
    EventSignal m_eventDeclaredState;

    // Mutex for compilation state
    CriticalSection m_CompStateCriticalSection;

    // Should we be posting notifications for project?
    bool m_fShouldSuspendNotifications;

#endif IDE

    // Current state.
    CompilationState m_cs;

#if IDE 
    // The compilation state to which the project has been decompiled.
    // (This differs from the current compilation state in that
    // demotion (which actually changes the compilation state) occurs
    // later than decompilation. The decompilation state is used to
    // avoid repeatedly decompiling a project.)
    CompilationState m_DecompilationState;

    // List of files that need to have their StatementChanged event fired
    FireOnStatementChangedList m_StatementChangedList;

    // Decompilation change trackers.
    ULONG m_iDeclChangeIndex;
    ULONG m_iBoundChangeIndex;
    ULONG m_iSourceChangeIndex;
#endif

    // Holds this project's binding hash table and its symbols.
    NorlsAllocator m_nraSymbols;

    // Does all of the building.
    PEBuilder m_Builder;

    // A straight list of files.
    CompilerFile *m_pfileList;

    // The lists of files by state.  The only way to get a list of all files
    // in the project is to use the file iterator.
    //
    CDoubleList<CompilerFile> m_dlFiles[CS_MAX];

#if IDE 
    // IDE only: the IVsHierarchy for this project
    IVsHierarchy *m_pHierarchy;

    bool m_fQueriedForContextualIntellisenseFilter;
    IVsContextualIntellisenseFilter *m_pContextualIntellisenseFilter;

    ENCBuilder * m_pENCBuilder;
    bool m_fIsInDebugBreak;

    bool m_fIsSnippetProject;
    bool m_fContainedLanguageProject;
    bool m_fVenusProject;
    bool m_fVenusAppCodeProject;

    CompilerFileTree m_treeFiles;
    NorlsAllocator m_nraProjectLifetime;

    XmlIntellisenseSchemas m_XmlSchemas;
#endif IDE

    // Generic pointer into the project.  We use this to navigate to
    // other project interfaces.
    //
    // This pointer is not AddRefed to avoid a reference cycle.
    //
    IUnknown *m_punkProject;

    // The symbol that stores project level cond
    // comp symbols.  This is created when the project
    // is created.  The entire project must be decompiled
    // before this is tossed and rebuilt (i.e. when
    // the project-level CC's change).
    //
    BCSYM_CCContainer *m_ProjectConditionalCompilationScope;

    // Marks for the end of various states, so we can
    // roll back memory
    NorlsMark m_markDeclared;
    NorlsMark m_markBound;
    NorlsMark m_markTypesEmitted;

    // The memory image of the PE if compiling to memory
    void *m_pImage;

    // IStream containing PDB (in-memory / VBA)
    IStream *m_pStreamPDB;      // NOTE: this is passed to metaemit, this object does NOT AddRef

    // Callback to send our status to.
    IVbBuildStatusCallback *m_pBuildStatusCallBack;

    //
    // Links used by the project manager.  There are iterators
    // for these defined in projmgr.h
    //

    friend class Compiler;
    friend struct ProjectIterator;
    friend struct ReferenceIterator;
    friend struct ReverseReferenceIterator;
#if IDE 
    friend struct ReverseImplicitReferenceIterator;
    friend struct ProjectImportsIterator;
#endif IDE

    // The projects referenced by this one.
    //
    DynamicArray<ReferencedProject> m_daReferences;
    
    CompilerProject *m_pVBRuntimeProject;

    VBRUNTIME_KIND m_vbRuntimeKind;

    bool m_bIsHighEntropyVA;

    SubsystemVersion m_subsystemVersion;

public:

    bool IsHighEntropyVA(){ return m_bIsHighEntropyVA;}

    SubsystemVersion GetSubsystemVersion() { return m_subsystemVersion; }
		
    bool IsDefaultVBRuntime();
        
    CompilerProject* GetVBRuntimeProject()
    {
        return m_pVBRuntimeProject; 
    }

    void SetVBRuntimeProject(CompilerProject* pVBRuntimeProject)
    {
        ThrowIfFalse(m_vbRuntimeKind == DefaultRuntime || m_vbRuntimeKind == SpecifiedRuntime);
        RELEASE(m_pVBRuntimeProject);
        m_pVBRuntimeProject = pVBRuntimeProject;
        m_pVBRuntimeProject->AddRef();
        return; 
    }


    VBRUNTIME_KIND GetVBRuntimeKind(){ return m_vbRuntimeKind;}
    
private:
#if IDE 
    // Track references usages -- set while determining unused references
    //
    // This variable is set from the foreground thread and read from the background thread.
    // However, we do not need to set/read this flag synchronously.  When it is set to true,
    // the background thread is stopped (StartGetUnusedReferences()) so it cannot be written
    // and read at the same time.  When it is set to false, if it written and read at the same
    // time, the worst case is one extra call into SetReferenceUsageForSymbol().
    //
    // Using a mutex would be expensive because we would need to grab it for every symbol we
    // process.  Likewise, stopping the compiler when StopGetUnusedReferences() would be
    // expensive and, as seen above, is not necessary.
    bool m_fGetUnusedReferences;

    // Helper functions for reference usage tracking
    //

    // Finds and flags a referenced project as used
    void SetReferenceUsage(CompilerProject *pProjectReferenced);

    // Flags all referenced projects associated with hierarchy of implements as used
    void SetReferenceUsageForImplementsAndInterface(BCSYM_NamedRoot *DependedOn);

    // Digs through a symbol's type and flags referenced projects associated with type as used
    void SetReferenceUsageForType(BCSYM *DependedOnType);

    // NULL delimited string of paths of unused references
    StringBuffer m_strUnusedReferences;


public:
    IHostedCompilerProjectNative *GetHostedCompilerProjectNative()
    {
        return m_spHostedCompilerProjectNative;
    }
    void SetHostedCompilerProjectNative(IHostedCompilerProjectNative *pHostedCompilerProjectNative )
    {
        if (pHostedCompilerProjectNative)
        {
            pHostedCompilerProjectNative->QueryInterface(IID_IHostedCompilerProjectNative, (void **)&m_spHostedCompilerProjectNative);
        }
    }
private:
    CComPtr<IHostedCompilerProjectNative> m_spHostedCompilerProjectNative;
        

#endif IDE

public:
    // Retrieve m_daReferences array index of project specified
    // Sets iRef to found index and returns true, or returns false if not found
    bool FindReferenceIndex(CompilerProject *pProject, unsigned *iRef);
    bool HasEmbeddedReferenceTo(CompilerProject *pProject);

private:
    // The projects that reference this compilerproject
    DynamicArray<CompilerProject *> m_daReferencesOnMe;

    void AddReferenceOnMe(CompilerProject *pProject);
    void RemoveReferenceOnMe(CompilerProject *pProject);

#if IDE 
    void AddFileToTree(CompilerFile *pFile);
    void RemoveFileFromTree(CompilerFile *pFile);
    bool HasDirectOrIndirectReferenceOnMe(CompilerProject *pProject);
#endif

#if IDE 
    // The metadata projects that implicitly reference this compilerproject
    DynamicArray<AssemblyRefInfo *> m_daImplicitReferencesOnMe;

    void AddImplicitReferenceOnMe(AssemblyRefInfo *pAssemblyRefInfo);
    void RemoveImplicitReferenceOnMe(AssemblyRefInfo *pAssemblyRefInfo);

    void SyncSourceFiles();
#endif IDE

    void CheckSolutionExtensions();

    void ToggleSolutionExtension(SolutionExtensionEnum Extension);

    void AddSolutionExtension(SolutionExtensionEnum Extension);
    void RemoveSolutionExtension(SolutionExtensionEnum Extension);

#if HOSTED
public:
#endif
    HRESULT AddBufferHelper(
        _In_bytecount_(dwLen) WCHAR *pwszBuffer, 
        _In_ DWORD dwLen, 
        _In_z_ LPCWSTR wszMkr, 
        _In_ VSITEMID itemid, 
        _In_ BOOL fAdvise, 
        _In_ BOOL fShowErrorsInTaskList, 
        _Out_ SourceFile **ppFile,
        _In_opt_ bool fWatchFile = true, // file system watcher
        _In_opt_ SolutionExtensionEnum nSolutionExtension = SolutionExtension::SolutionExtensionNone // if it's a My template 
        );
#if HOSTED
private:
#endif


    ErrorTable          m_ErrorTable;

    // True if this project is registered in the project manager.
    bool m_isRegistered;

    bool m_fGeneratePDB;
    bool m_fGenerateDebugCode;
    bool m_fGenerateOptimalIL;
    bool m_fGenerateENCableCode;

    // warnings granularity
    WARNING_LEVEL m_WarningLevel;
    DynamicArray<VbCompilerWarningItemLevel> m_daWarningsLevelTable;

#ifdef EXTENDED_WARNING_GRANULARITY
    static UINT m_wrnIds[];     // table with the warning id's
    //cached table that is provided to the VS project for all warnings granularity
    VbCompilerWarningIdAndString* m_WarningIdAndStringTable;
    void CleanWarningIdAndStringTable();
#endif

    //=============================================================================
    // Information about Public key file, public key container and Assembly version
    // that are specified using assembly attributes.
    //=============================================================================

    SourceFile *m_pFileWithAssemblyKeyFileAttr;     // Source file in which the AssemblyKeyFile attr is specified
    bool m_fKeyFileFromAttr;                        // Indicates whether the key file name for this project is set
                                                    // via the assembly level attribute or the project setting.

    SourceFile *m_pFileWithAssemblyKeyNameAttr;     // Source file in which the AssemblyKeyName attr is specified
    bool m_fKeyNameFromAttr;                        // Indicates whether the key container name for this project
                                                    // is set via the assembly level attribute or the project setting.

    SourceFile *m_pFileWithAssemblyVersionAttr;     // Source file in which the AssemblyVersion attr is specified


    //========================================================================
    // Build options - controls what and how we build .
    //========================================================================

    // Preferred load address of generated binary
    size_t m_dwLoadAddress;

    // Preferred file alignment of generated binary
    DWORD m_dwAlign;

    // Default code page for files -- for command-line ONLY
    DWORD m_dwDefaultCodePage;

    // Output type
    VbCompilerOutputTypes m_OutputType;

#if IDE 
    // Indicates that some IL tasks were skipped when compiling this project when
    // debugging is in progress.
    bool m_fILEmitTasksSkippedDuringDebugging;
#endif IDE

    // Name of file/container that holds crypto key to generate strong-named (public)
    // assembly.  You can have either a key file or a key container, but not both.
    //
    // These fields hold the project level settings for the key file and container name.
    // Note that they can also be specified as assembly attributes in which they are
    // not stored here but are stored directly in this project's assembly identity object.
    //
    // IMPORTANT: Do not use these field directly, use the accessors.

    STRING *m_pstrProjectSettingForKeyFileName;         // maps to wszStrongNameKeyFile in CompilerOptions
    STRING *m_pstrProjectSettingForKeyContainerName;    // maps to wszStrongNameContainer in CompilerOptions
    STRING *m_pstrUacManifestFile;                      // maps to wszUacManifestFile in CompilerOptions

    LANGVERSION m_LangVersion;

    bool    m_fDelaySign;
    bool    m_fNoStandardLibraries;

    STRING *m_pstrWin32ResourceFile;
    STRING *m_pstrWin32IconFile;

    // Resources to embed in or attach to this assembly
    DynamicArray<Resource> m_daResources;

#if HOSTED
  public:
#else
  private:
#endif
    struct ProjectLevelImportsList
    {
#if IDE 
        friend struct ProjectImportsIterator;
#endif
      public:

        ProjectLevelImportsList
        (
            Compiler *pCompiler,
            CompilerProject *pOwningCompilerProject
        );

        void AddImport
        (
            LPCWSTR ImportString
        );

        void DeleteImport
        (
            LPCWSTR ImportString
        );

        bool IsImported
        (
            LPCWSTR ImportString
        );

        ImportedTarget* GetAllImportedTargets()
        {
            return m_pProjectLevelImportTargets;
        }

        void DeleteAllImports();

        void ReBuildAllImportTargets();

        ~ProjectLevelImportsList();

        void DumpImportsErrors
        (
            ErrorTable *ErrorTableToDumpErrorsIn
        );

        ErrorTable *GetImportsErrorLog()
        {
            return &m_ImportsErrorLog;
        }

        WCHAR *GetOriginalImportStringByIndex(unsigned Index);

      private:
        struct ProjectLevelImportInfo
        {
#if IDE 
            friend struct ProjectImportsIterator;
#endif

            WCHAR *m_OriginalImportString;
            unsigned m_StrLen;

            ProjectLevelImportInfo *m_Next;
        };

        ImportedTarget* BuildImportTarget
        (
            const WCHAR * ImportString,
            unsigned ImportStringLength,
            unsigned ImportIndex,
            ImportedTarget * PreviousTarget
        );

        bool ErrorsInImports();

        // The list of project level imports.

        ProjectLevelImportInfo *m_pProjectLevelImportInfos;
        ImportedTarget *m_pProjectLevelImportTargets;

        Compiler *m_pCompiler;
        CompilerProject *m_pOwningCompilerProject;
        NorlsAllocator m_ImportsAllocator;
        ErrorTable m_ImportsErrorLog;

    };
  
#if HOSTED
    ProjectLevelImportsList *GetProjectLevelImportsList()
    {
        return &m_Imports;
    }

  private:
#endif HOSTED

    ProjectLevelImportsList m_Imports;

  public:
    WCHAR *GetOriginalImportStringByIndex(unsigned ImportIndex)
    {
        return m_Imports.GetOriginalImportStringByIndex(ImportIndex);
    }

#if IDE 
private:

    // sets all Imported targets m_hasError bit to false. This is needed when Aborting a compilation during
    // a promotion to Bound state.
    void ClearImportTargetErrors()
    {
        for (ImportedTarget *ThisTarget = m_Imports.GetAllImportedTargets(); ThisTarget; ThisTarget = ThisTarget->m_pNext)
        {
            ThisTarget->m_hasError = false;
        }
    }

#endif

public:
    SecAttrInfo *CreateSecAttrsForSymbol
    (
        BCSYM *pSymbol,
        mdToken tkSymbol,
        ULONG cSecAttrMax
    );

    void SaveSecAttr
    (
        SecAttrInfo *pSecAttrInfo,
        mdMemberRef tkCtor,
        BYTE *pbCustomAttrBlob,
        ULONG cbCustomAttrBlob,
        _In_z_ STRING *pstrAttrName,
        Location &Loc,
        BCSYM_NamedRoot *pNamedContext
    );

    CSingleList<SecAttrInfo> *GetSecAttrSetsToEmit()
    {
        return &m_slAppliedSecurityAttributes;
    }

private:
    // The security attributes applied on the various symbols in this project.
    // These need to be saved and emitted after the manifest for this project
    // has been emitted. This is required so that any security attributes
    // defined in this project when used in this same project are emitted
    // correctly. Bug VSWhidbey 320892.
    CSingleList<SecAttrInfo> m_slAppliedSecurityAttributes;

#if IDE 
public:
    CompilerTaskProvider * GetCompilerTaskProvider()
    {
        return m_pTaskProvider;
    }

    // The flag needs to be cached, set and reset for specific container binding.
    bool IsBoundCompilationInProgress()
    {
        return m_fDeclaredToBoundCompilationInProgress;
    }

    // The flag needs to be cached, set and reset for specific container binding.
    void SetBoundCompilationInProgress(bool IsCompilationInProgress)
    {
        m_fDeclaredToBoundCompilationInProgress = IsCompilationInProgress;
    }

private:
    CompilerTaskProvider*  m_pTaskProvider;
#endif

    // Temporary source file cache for the IDE compiler.
    // The IDE has to open, read and close source files. The cache
    // is used to prevent having to re-open files that we've already
    // read in since opening a file is very expensive.
#if IDE 
public:

    void IncrementFileChangedNotificationCounter()
    {
        VSASSERT(m_FileChangedNotificationCounter >= 0, "Bad m_FileChangedNotificationCounter state");
        InterlockedIncrement(&m_FileChangedNotificationCounter);
    }

    void DecrementFileChangedNotificationCounter()
    {
        InterlockedDecrement(&m_FileChangedNotificationCounter);
        VSASSERT(m_FileChangedNotificationCounter >= 0, "Bad m_FileChangedNotificationCounter state");
    }

    bool IsZeroFileChangedNotificationCounter()
    {
        VSASSERT(m_FileChangedNotificationCounter >= 0, "Bad m_FileChangedNotificationCounter state");
        return (m_FileChangedNotificationCounter == 0);
    }


private:

    LONG m_FileChangedNotificationCounter;
#endif IDE

public:
    TextFileCache* GetSourceFileCache() { return &m_SourceFileCache; }

    LONG GetProjectId() const { return m_projectId; }

    // Has this project been seen by TopologicalySortProjects()?
    bool IsProjectSeen() { return m_projectSeen; }

    // Sets the PrijectSeen bit.
    void SetProjectSeen(bool IsSeen) { m_projectSeen = IsSeen; }

    // Is this project or its dependencies current being sorted by TopologicalySortProjects() ?
    bool IsProjectSortInProgress() { return m_projectSortInProgress; }

    // Sets the in-progress bit during TopologicalySortProjects().
    void SetProjectSortInProgress(bool InProgress) { m_projectSortInProgress = InProgress; }

    bool IsSolutionExtensionAdded(SolutionExtensionEnum Extension)
    {
        if (Extension >= 0 && Extension < _countof(m_rgfExtensionAdded))
        {
            return m_rgfExtensionAdded[Extension];
        }

        return false;
    }

    DynamicArray<MyGroupCollectionInfo> m_daMyGroupCollectionInfo;
    STRING* m_StartupMyFormFactory;

private:
    TextFileCache m_SourceFileCache;

    bool m_projectSeen;          // This bit is used when building the list of projects to compile.
    bool m_projectSortInProgress;// This bit is used when building the list of projects to compile.
    static LONG m_lastProjectId;
    LONG m_projectId;

    bool m_rgfExtensionAdded[SolutionExtension::MaxExtension + 1];

    // Indicates if currently declared to bound compilation
    // is in progress. This is need to decide if GetHash
    // should trigger the ondemand collapsing of partial
    // types.
    //
    bool m_fDeclaredToBoundCompilationInProgress;

    // Indicate if the project level imports and for
    // this project have been resolved.
    //
    bool m_fHaveImportsBeenResolved;

#if DEBUG
    // Indicate if the project level imports and for
    // this project are being currently resolved.
    //
    bool m_fAreImportsBeingResolved;
#endif DEBUG

    // Indicates if this project claims to be CLS Compliant by
    // means of either assembly level or Module level attributes.
    //
    bool m_fProjectClaimsCLSCompliance;


#if IDE 
    // Indicates if this project claimed to be CLS Compliant previously
    //
    bool m_fProjectClaimedCLSCompliancePreviously;

    // Indicates the file which contains the Assembly level attribute that
    // influenced the project's CLS Compliance.
    //
    // Needed for decompilation
    //
    CompilerFile *m_pFileThatCausedProjectsCLSComplianceClaim;

    // These flags are used by the decompilation logic in EditFilter to avoid
    // calling certain decompilation logic multiple times.
    //
    bool m_fProjectAndFilesDecompToBoundStarted;
    bool m_fProjectAndFilesDecompToDeclaredStarted;

    // Indicates whether any embedded resources have been emitted for this project.
    bool m_fEmbeddedResourcesEmitted;
#endif

public:
    PlatformKinds GetPlatformKind()
    {
        return m_PlatformKind;
    }

private:
    PlatformKinds m_PlatformKind;

#if IDE
private:
    CCodeModel *m_pCodeModel;
    CExternalCodeModel *m_pExternalCodeModel;
#endif

#if IDE
public:
    void UpdateENCMetaDataInfo();

private:
    bool m_fNeedtoUpdateENCMetaData;
    DynamicArray<DeltaGenFixupItem> m_daDeltaGenFixupItems;
#endif
#if !IDE
public:
    struct ExternalChecksum
    {
        STRING *FileName;
        GUID Guid;
        DWORD cbChecksumData;
        BYTE *pbChecksumData;
    };
    DynamicArray<ExternalChecksum> m_daExternalChecksum;
    bool AddExternalChecksum(_In_z_ const WCHAR *ExternalChecksumFileName, _In_z_ const WCHAR *ExternalChecksumGuid, _In_z_ const WCHAR *ExternalChecksumVal, ERRID &error);
#endif

#if IDE 
    DynamicArray<SubMainInfo> m_daEntryPoints;
    bool m_fCachedEntryPoints;
#endif IDE

    // Message tracking
#if IDE 
public:

    CompilerIdeCriticalSection& GetMessageCriticalSection() 
    { 
        return m_MessageCS;
    }

    bool IsPosted_ProjectInBindable() { return m_IsPosted_ProjectInBindable; }
    void SetPosted_ProjectInBindable(bool IsPosted) { m_IsPosted_ProjectInBindable = IsPosted;}

    bool IsPosted_FireOnStatementChanged() { return m_IsPosted_FireOnStatementChanged; }
    void SetPosted_FireOnStatementChanged(bool IsPosted) { m_IsPosted_FireOnStatementChanged = IsPosted;}

    bool IsPosted_FlagXmlSchemas() { 
        return m_IsPosted_FlagXmlSchemas; 
    }

    void SetPosted_FlagXmlSchemas(bool IsPosted) { m_IsPosted_FlagXmlSchemas = IsPosted;}

    void IncrementPostedMessageCount()
    {
        CompilerIdeLock spLock(GetMessageCriticalSection());
        m_cPostedMessageCount++;
        AddRef();
    }

    void DecrementPostedMessageCount()
    {
        {
            CompilerIdeLock spLock(GetMessageCriticalSection());
            m_cPostedMessageCount--;
        }
        Release();
    }

private:
    CompilerIdeCriticalSection m_MessageCS;

    bool m_IsPosted_ProjectInBindable;
    bool m_IsPosted_FireOnStatementChanged;
    bool m_IsPosted_FlagXmlSchemas;
    
    // number of pending messages
    ULONG m_cPostedMessageCount;
#endif


#if IDE 
    // VBReferenceChangedService
private:
    void EnsureVBReferenceChangedService();
    bool m_fEnableVBReferenceChangedService;
    bool m_fVBReferenceChangedServiceQueried;
    CComPtr<IVBReferenceChangedService> m_srpVBReferenceChangedService;
#endif IDE

public:

    ExtensionMethodNameLookupCache * GetExtensionMethodLookupCache()
    {
        return &m_ExtensionMethodLookupCache;
    }

    HashSet<STRING_INFO*> * GetExtensionMethodExistsCache()
    {
        return &m_ExtensionMethodExistsCache;
    }

    LookupTree *GetLookupCache()
    {
        return &m_LookupCache;
    }

    ResolvedImportsTree *GetImportsCache()
    {
        return &m_ImportsCache;
    }

    LiftedUserDefinedOperatorCache * GetLiftedOperatorCache()
    {
        return &m_LiftedOperatorCache;
    }

    void ClearLookupCaches()
    {
        m_LookupCache.Clear();
        m_ImportsCache.Clear();
        m_ExtensionMethodLookupCache.Clear();
        m_LiftedOperatorCache.Clear();
        m_nrlsLookupCaches.FreeHeap();   
    }

    // PIA type reference caches
    void CachePiaTypeRef(BCSYM_Container* psymType, Location *pReferencingLocation, ErrorTable *pErrorTable);
    bool IsCachedPiaTypeRef(BCSYM* psymType);
    void CachePiaTypeMemberRef(BCSYM_NamedRoot *psymMember,
                               Location *pReferencingLocation,
                               ErrorTable *pErrorTable)
    {
        int errorCount = 0;
        CachePiaTypeMemberRef(psymMember, pReferencingLocation, pErrorTable, errorCount);
    }
    void CachePiaTypeMemberRef(BCSYM_NamedRoot *psymMember,
                               Location *pReferencingLocation,
                               ErrorTable *pErrorTable,
                               int & errorCount);
    PiaTypeIterator GetPiaTypeIterator();
    bool IdentifyPiaTypeMember(BCSYM *psymMember);
    unsigned int CountPiaCacheEntries(void);
    unsigned int CountPiaTypeCacheEntries(void);

#if IDE 
    //Add accessor for allocator, transient symbols, and mrulist
    NorlsAllocator *GetBoundProjectAllocator() 
    { 
        return m_BoundMethodBodyCache.GetNorlsAllocator();
    }
    TransientSymbolStore *GetBoundMethodBodyTransients() 
    { 
        return  m_BoundMethodBodyCache.GetBoundMethodBodyTransients();
    }
    std::map<BCSYM_Proc *, BoundMethodBodyNode> *GetBoundMethodBodies() 
    {
        return m_BoundMethodBodyCache.GetBoundMethodBodies();
    }
#endif

private:
    NorlsAllocator m_nrlsLookupCaches;

    LookupTree m_LookupCache;
    ResolvedImportsTree m_ImportsCache;
    ExtensionMethodNameLookupCache m_ExtensionMethodLookupCache;
    // When metadata is loaded, every method that has the extension attribute is added to this cache.  This cache is not used to speed up extension
    // method lookup for names that match extension methods but rather to avoid costly extension method lookup for names that are not extension methods.
    // The right solution is to change where VB checks for extension method.  Currently, it is done too early and evey name lookup checks whether the name is
    // an extension method regarless of whether the name is a method in the class.  See extension method spec for complete details on overload resolution.
    // Note, this cache does not return the extension method symbol, it only answers whether the project contains an extension method with this name.
    HashSet<STRING_INFO*> m_ExtensionMethodExistsCache; // Entry for existence of an extension method with the name    
    LiftedUserDefinedOperatorCache m_LiftedOperatorCache;

    // The declaration type refs are populated when going to Declared state.
    HashSet<BCSYM*> m_DeclarationPiaTypeRefCache;
    HashSet<BCSYM*> m_DeclarationPiaMemberCache;
    // The method-body type refs are populated when going to Compiled state.
    HashSet<BCSYM*> m_MethodBodyPiaTypeRefCache;
    HashSet<BCSYM*> m_MethodBodyPiaMemberCache;

    // Dev10 #735384: This flag is set if there is a chance that this project will embed PIA types. 
    // This flag is optimistic, i.e. we may end up not embedding any types.
    bool m_PotentiallyEmbedsPiaTypes;  

public:
    bool PotentiallyEmbedsPiaTypes()
    {
        return m_PotentiallyEmbedsPiaTypes;
    }

    void SetPotentiallyEmbedsPiaTypes(bool value)
    {
        m_PotentiallyEmbedsPiaTypes = value;
    }

    bool NeedToCheckUseOfEmbeddedTypes();
    
private:
    BCSYM_Interface *LookupComEventSourceInterface(BCSYM_Container *pType, bool suppressErrors, Location *pReferencingLocation, ErrorTable *pErrorTable);

#if IDE 
    // stored in VBTlsThreadData Dev10 #751770 This pointer is not NULL if we need to track the fact of embedded type substitution
#else
    bool * m_pReferredToEmbeddableInteropType; // Dev10 #751770 This pointer is not NULL if we need to track the fact of embedded type substitution
#endif

public:

    bool * Get_pReferredToEmbeddableInteropType();

    void Set_pReferredToEmbeddableInteropType(bool * pValue);


#if IDE 

public:    
    DynamicHashTable<BCSYM*, CompilerProject *> m_UnsubstitutedLocalTypes;

    bool GetTrackUnsubstitutedLocalTypes();
    
    void SetTrackUnsubstitutedLocalTypes(bool value);
#endif

private:
    
#if IDE 

    BoundMethodBodyCache m_BoundMethodBodyCache;

    void ClearCachedBoundMethodBodies()
    {
        m_BoundMethodBodyCache.Clear();
    }

public:

    IDEExtensionMethodCache *GetIDEExtensionMethodCache()
    {
        return &m_IDEExtensionMethodCache;
    }

    void ClearIDEExtensionMethodCache()
    {
        m_IDEExtensionMethodCache.Clear();
    }

    void SetIsExpressionEditorProject() 
    { 
        m_isExpressionEditorProject = true; 
    }

    bool IsExpressionEditorProject()
    {
        return m_isExpressionEditorProject;
    }

private:

    IDEExtensionMethodCache m_IDEExtensionMethodCache;

    bool m_isExpressionEditorProject;
#endif IDE
};

// This is for the object browser.
#define IID_CompilerProject __uuidof( CompilerProject )
