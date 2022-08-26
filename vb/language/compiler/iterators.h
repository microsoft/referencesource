//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Project and file iterators.
//
//-------------------------------------------------------------------------------------------------

#pragma once

class Compiler;
class CompilerProject;
class CompilerFile;
class SourceFile;
class MetaDataFile;
class AssemblyRefCollection;
struct AssemblyRefInfo;

//*****************************************************************************
// Iterate over all CompilerHosts
//*****************************************************************************

struct CompilerHostsIterator
{
    CompilerHostsIterator(Compiler * pCompiler)
    {
        if (pCompiler)
        {
            m_slIter.Init(pCompiler->GetCompilerHosts());
        }
        else
        {
            m_slIter.Init(NULL);
        }
#if IDE 
        if (GetCompilerSharedState()->GetWaitForStateCompilerHost())
        {
            pPreferredCompilerHost = GetCompilerSharedState()->GetWaitForStateCompilerHost();
            ShouldReturnPreferredHost = true;
        }
        else
        {
            pPreferredCompilerHost = NULL;
            ShouldReturnPreferredHost = false;
        }
#endif
    }

    CompilerHost * Next()
    {
#if IDE 
        if (pPreferredCompilerHost)
        {
            if (ShouldReturnPreferredHost)
            {
                ShouldReturnPreferredHost = false;
                return pPreferredCompilerHost;
            }
            else
            {
                CompilerHost *pCompilerHost = m_slIter.Next();

                // Skip this host if it's the preferred host we returned at the beginning
                if (pCompilerHost && pCompilerHost == pPreferredCompilerHost)
                {
                    pCompilerHost = m_slIter.Next();
                }

                return pCompilerHost;
            }
        }
        else
        {
            return m_slIter.Next();
        }
#else
        return m_slIter.Next();
#endif
    }

    void Reset()
    {
#if IDE 
        if (pPreferredCompilerHost)
        {
            ShouldReturnPreferredHost = true;
        }
#endif

        m_slIter.Reset();
    }

private:
    CSingleListIter<CompilerHost> m_slIter;

#if IDE 
    // Indicate whether there is a preferred CompilerHost being used for WaitUntilState
    bool ShouldReturnPreferredHost;
    CompilerHost *pPreferredCompilerHost;
#endif
};

//*****************************************************************************
// Iterate over all loaded projects
//*****************************************************************************

struct ProjectIterator
{
    ProjectIterator(CompilerHost *pCompilerHost)
    {
        Init(pCompilerHost);
    }

    void Init(CompilerHost *pCompilerHost)
    {
        if (pCompilerHost)
        {
            m_slIter.Init(pCompilerHost->GetProjectList());
        }
        else
        {
            m_slIter.Init(NULL);
        }
    }

    CompilerProject *Next()
    {
        return m_slIter.Next();
    }

    void Reset()
    {
        m_slIter.Reset();
    }

private:
    CSingleListIter<CompilerProject> m_slIter;
};

//*****************************************************************************
// Iterate over all projects in the Solution
//*****************************************************************************

struct AllProjectsIterator
{
    AllProjectsIterator(Compiler *pCompiler)
    : m_hostIter(pCompiler)
    , m_projIter(m_hostIter.Next())
    {
    }

    void Reset()
    {
        m_hostIter.Reset();
        m_projIter.Init(m_hostIter.Next());
    }

    CompilerProject *Next()
    {
        CompilerProject *pProject = m_projIter.Next();

        while (pProject == NULL)
        {
            CompilerHost *pNextCompilerHost = m_hostIter.Next();

            if (!pNextCompilerHost)
            {
                break;
            }

            m_projIter.Init(pNextCompilerHost);
            pProject = m_projIter.Next();
        }

        return pProject;
    }

private:
    CompilerHostsIterator m_hostIter;
    ProjectIterator       m_projIter;
};

//****************************************************************************
// An iterator over all of the files in the project.
//****************************************************************************

struct FileIterator
{
    FileIterator() 
    {
    }

    FileIterator(_In_opt_ CompilerProject * pProject)
    {
        Init(pProject);
    }

    void Init(_In_opt_ CompilerProject * pcompproj);

    CompilerFile *Next();

private:
    CompilerFile *m_pfileNext;
};

//****************************************************************************
// An iterator over all of the source files in the project.
//****************************************************************************

struct SourceFileIterator
{
    SourceFileIterator() 
    {
    }

    SourceFileIterator(_In_opt_ CompilerProject * pProject)
    {
        Init(pProject);
    }

    void Init(_In_opt_ CompilerProject * pcompproj);

    SourceFile * Next();

private:
    CompilerFile *m_pfileNext;
};

//****************************************************************************
// An iterator over all of the metadata files in the project.
//****************************************************************************

struct MetaDataFileIterator
{
    MetaDataFileIterator() 
    {
    }

    MetaDataFileIterator(_In_opt_ CompilerProject * pProject)
    {
        Init(pProject);
    }

    void Init(_In_opt_ CompilerProject * pcompproj);

    MetaDataFile * Next();

private:
    CompilerFile *m_pfileNext;
};

//*****************************************************************************
// Iterate over all files in all projects
//*****************************************************************************

struct AllFilesIterator
{
    AllFilesIterator(Compiler * pCompiler)
    : m_projIter(pCompiler)
    {
        Reset();
    }

    CompilerFile * Next()
    {
        CompilerFile *pFile = m_fileIter.Next();

        while (pFile == NULL && m_pCurrentProject != NULL)
        {
            m_pCurrentProject = m_projIter.Next();
            m_fileIter.Init(m_pCurrentProject);

            pFile = m_fileIter.Next();
        }

        return pFile;
    }

    void Reset()
    {
        m_projIter.Reset();
        m_pCurrentProject = m_projIter.Next();
        m_fileIter.Init(m_pCurrentProject);
    }

private:
    CompilerProject        *m_pCurrentProject;
    AllProjectsIterator     m_projIter;
    FileIterator            m_fileIter;
};

//*****************************************************************************
// Iterate over all files in all projects in one host.
//*****************************************************************************

struct AllFilesInAHostIterator
{
    AllFilesInAHostIterator(CompilerHost *pCompilerHost)
    : m_projIter(pCompilerHost)
    {
        Reset();
    }

    CompilerFile *Next()
    {
        CompilerFile *pFile = m_fileIter.Next();

        while (pFile == NULL && m_pCurrentProject != NULL)
        {
            m_pCurrentProject = m_projIter.Next();
            m_fileIter.Init(m_pCurrentProject);

            pFile = m_fileIter.Next();
        }

        return pFile;
    }

    void Reset()
    {
        m_projIter.Reset();
        m_pCurrentProject = m_projIter.Next();
        m_fileIter.Init(m_pCurrentProject);
    }

private:
    CompilerProject        *m_pCurrentProject;
    ProjectIterator         m_projIter;
    FileIterator            m_fileIter;
};

//*****************************************************************************
// Iterate over all source files in all projects
//*****************************************************************************

struct AllSourceFilesIterator
{
    AllSourceFilesIterator(Compiler *pCompiler)
    : m_fileiter(pCompiler)
    {
        Reset();
    }

    SourceFile *Next()
    {
        CompilerFile *pFile;

        while (pFile = m_fileiter.Next())
        {
            if (pFile->IsSourceFile())
            {
                return pFile->PSourceFile();
            }
        }

        return NULL;
    }

    void Reset()
    {
        m_fileiter.Reset();
    }

private:
     AllFilesIterator m_fileiter;
};

//*****************************************************************************
// Iterate over all source files in all projects
//*****************************************************************************

struct AllSourceFilesInAHostIterator
{
    AllSourceFilesInAHostIterator(CompilerHost *pCompilerHost)
    : m_fileiter(pCompilerHost)
    {
        Reset();
    }

    SourceFile *Next()
    {
        CompilerFile *pFile;

        while (pFile = m_fileiter.Next())
        {
            if (pFile->IsSourceFile())
            {
                return pFile->PSourceFile();
            }
        }

        return NULL;
    }

    void Reset()
    {
        m_fileiter.Reset();
    }

private:
     AllFilesInAHostIterator m_fileiter;
};

//*****************************************************************************
// Iterate over all metadata files in all projects
//*****************************************************************************

struct AllMetaDataFilesIterator
{
    AllMetaDataFilesIterator(Compiler *pCompiler)
    : m_fileiter(pCompiler)
    {
        Reset();
    }

    MetaDataFile *Next()
    {
        CompilerFile *pFile;

        while (pFile = m_fileiter.Next())
        {
            if (pFile->IsMetaDataFile())
            {
                return pFile->PMetaDataFile();
            }
        }

        return NULL;
    }

    void Reset()
    {
        m_fileiter.Reset();
    }

private:
     AllFilesIterator m_fileiter;
};

//*****************************************************************************
// Iterator for all types defined in a file.
//*****************************************************************************

struct AllContainersInFileIterator
{
    AllContainersInFileIterator()
    {
    }

    AllContainersInFileIterator(
        CompilerFile * pFile,
        bool fIncludePartialContainers = false,
        bool fIncludeTransientSymbols = false)
    {
        Init(pFile, fIncludePartialContainers, fIncludeTransientSymbols);
    }

    void Init(
        CompilerFile * pFile,
        bool fIncludePartialContainers = false,
        bool fIncludeTransientSymbols = false);

    BCSYM_Container *Next();

private:
    BCSYM_NamedRoot *m_pNext;
    bool m_fIncludePartialContainers;
    bool m_fIncludeTransientSymbols;
};

//*****************************************************************************
// Iterator for all types defined in a file in declaration order.
//*****************************************************************************

struct AllContainersInFileInOrderIterator
{
    AllContainersInFileInOrderIterator()
    : m_nra(NORLSLOC)
    {
    }

    AllContainersInFileInOrderIterator(
        SourceFile * pFile,
        bool fIncludePartialContainers = false,
        bool fIncludeTransientTypes = false)
    : m_nra(NORLSLOC)
    {
        Init(pFile, fIncludePartialContainers, fIncludeTransientTypes);
    }

    void Init(
        SourceFile * pFile,
        bool fIncludePartialContainers = false,
        bool fIncludeTransientTypes = false);

    BCSYM_Container *Next();

private:
    NorlsAllocator m_nra;
    BCSYM_Container **m_ppNext;
};

struct AllEmitableTypesInFileInOrderIterator
{
    AllEmitableTypesInFileInOrderIterator(Compiler * pCompiler)
    {
    }

    AllEmitableTypesInFileInOrderIterator(
        Compiler * pCompiler,
        SourceFile * pFile)
    {
        Init(pFile);
    }

    void Init(SourceFile * pFile);

    BCSYM_Container *Next();

    struct EmitableTypeWrapper
    {
        BCSYM_Container *m_pEmitableType;
        int m_NestingLevel;
    };

private:
    void GetEmitableTypesInContainer(
        BCSYM_Container * pContainer,
        _Inout_ int &NestingLevel);

    DynamicArray<EmitableTypeWrapper> m_pListOfEmitableTypes;
    unsigned m_CurrentTypeIndex;
};

//*****************************************************************************
// Iterator for all of the members of a container that need to be emitted
// into metadata.
//*****************************************************************************

struct SortedMembersInAContainer
{
    SortedMembersInAContainer(
        BCSYM_Container * pContainer,
        int(__cdecl * pfnCompare)(const void * arg1,
        const void * arg2))
    : m_nra(NORLSLOC)
    {
        Init(pContainer, pfnCompare);
    }

    void Init(
        BCSYM_Container * pFile,
        int(__cdecl * pfnCompare)(const void * arg1,
        const void * arg2));

    BCSYM_NamedRoot *Next();

private:
    NorlsAllocator m_nra;
    BCSYM_NamedRoot **m_ppNext;
};

struct DefineableMembersInAContainer : public SortedMembersInAContainer
{
    DefineableMembersInAContainer(
        Compiler * pCompiler,
        BCSYM_Container * pContainer)
    : SortedMembersInAContainer(pContainer, SortSymbolsByLocation)
    {
    }
};


//*****************************************************************************
// Abstract base iterator for named items in a container.
// Supports Count/Reset/Next.
//*****************************************************************************

class MembersInAContainerBase
{
public:
    MembersInAContainerBase()
    : m_nra(NORLSLOC)
    , m_count(0)
    , m_ppNext(NULL)
    , m_ppRoot(NULL)
    {
    }

    // Abstract method. The derived class must implement Init
    // and its constructor should call it.
    virtual
    void Init(BCSYM_Container * pContainer) = 0;

    void Reset();
    BCSYM_NamedRoot *Next();

    UINT Count()
    {
        return m_count;
    }

protected:
    NorlsAllocator m_nra;
    BCSYM_NamedRoot **m_ppRoot;
    BCSYM_NamedRoot **m_ppNext;
    UINT m_count;
};


//*****************************************************************************
// Iterator for all of the members of a container that need to be emitted
// into metadata for the com class interface
//*****************************************************************************

class ComClassMembersInAContainer :
    public MembersInAContainerBase
{
public:
    ComClassMembersInAContainer(
        BCSYM_Container * pContainer)
    : MembersInAContainerBase()
    {
        Init(pContainer);
    }

    virtual
    void Init(BCSYM_Container * pContainer);

    static
    bool Filter(BCSYM_NamedRoot * pRoot);
};

//*****************************************************************************
// Iterator for all of the members of a container that need to be emitted
// into metadata for the com class events interface
//*****************************************************************************

class ComClassEventsInAContainer :
    public MembersInAContainerBase
{
public:
    ComClassEventsInAContainer(
        BCSYM_Container * pContainer)
    : MembersInAContainerBase()
    {
        Init(pContainer);
    }

    virtual
    void Init(BCSYM_Container * pFile);

    static
    bool Filter(BCSYM_NamedRoot * pRoot);
};


//*****************************************************************************
// Iterator for all of the MethodImpls and SyntheticMethods defined in
// a container.
//*****************************************************************************

struct CompileableMethodsInAContainer
{
    CompileableMethodsInAContainer()
    : m_nra(NORLSLOC)
    {
    }

    CompileableMethodsInAContainer(
        BCSYM_Container * pContainer,
        bool fLookOnlyInCurrentPartialType = false)
    : m_nra(NORLSLOC)
    {
        Init(pContainer, fLookOnlyInCurrentPartialType);
    }

    void Init(
        BCSYM_Container * pFile,
        bool fLookOnlyInCurrentPartialType = false);

    BCSYM_Proc *Next();

private:
    NorlsAllocator m_nra;
    BCSYM_Proc **m_ppNext;
};

//*****************************************************************************
// Iterate over all of the projects that this one references.
//*****************************************************************************

struct ReferenceIterator
{
    // If you use this, you must call Init before Next or NextRef.
    ReferenceIterator() {}

    // This calls Init for you.
    ReferenceIterator(_In_ CompilerProject *pProject)
    {
        Init(pProject);
    }

    void Init(_In_ CompilerProject *pProject);

    unsigned long GetCount()
    {
        return m_cEntries;
    }

    CompilerProject *Next();
    ReferencedProject* NextReference();


private:
    ReferencedProject *m_rgref;
    unsigned m_cEntries;
    unsigned m_iCurrent;
};

//*****************************************************************************
// Iterate over the projects that reference this one.
//*****************************************************************************

struct ReverseReferenceIterator
{
    ReverseReferenceIterator(_In_ CompilerProject *pProject)
    {
        Init(pProject);
    }

    void Init(_In_ CompilerProject *pProject);

    CompilerProject * Next()
    {
        if (m_iCurrent < m_cEntries)
        {
            return m_ReverseProjectReferenceArray[m_iCurrent++];
        }

        return NULL;
    }

    unsigned GetCurrentIndex() 
    {
         return (m_iCurrent > 0) ? (m_iCurrent - 1) : 0; 
    }

private:
    CompilerProject **m_ReverseProjectReferenceArray;
    unsigned m_cEntries;
    unsigned m_iCurrent;
};

//*****************************************************************************
// Iterate over the assembly refs in this metadata project
//*****************************************************************************

struct AssemblyRefIterator
{
    AssemblyRefIterator(_In_opt_ CompilerProject * pProject = NULL)
    {
        Init(pProject);
    }

    void Init(_In_opt_ CompilerProject * pProject)
    {
        m_pProject = pProject;
        Reset();
    }

    void Reset()
    {
        m_MetadataFileIter.Init(m_pProject);
        m_pCurrentFile = m_MetadataFileIter.Next();
        m_pAssemblyRefsForCurrentFile = NULL;
        m_CurrentIndex = 0;
    }

    AssemblyRefInfo *Next();

private:
    CompilerProject *m_pProject;

    MetaDataFileIterator m_MetadataFileIter;
    MetaDataFile *m_pCurrentFile;
    AssemblyRefCollection *m_pAssemblyRefsForCurrentFile;
    unsigned int m_CurrentIndex;
};

//*****************************************************************************
// Iterate over the projects that this AssemblyRef binds to.
//*****************************************************************************

struct AssemblyRefReferencedProjectsIterator
{
    AssemblyRefReferencedProjectsIterator(_In_ AssemblyRefInfo * pAssemblyRefInfo = NULL)
    {
        Init(pAssemblyRefInfo);
    }

    void Init(_In_ AssemblyRefInfo * pAssemblyRefInfo)
    {
        m_pAssemblyRefInfo = pAssemblyRefInfo;
        Reset();
    }

    void Reset()
    {
        m_fNonArrayProject1Accessed = false;
        m_fNonArrayProject2Accessed = false;
        m_iCurrent = 0;
        m_ppCurrentProject = NULL;
    }

    CompilerProject * Next();

    void SetCurrentProject(CompilerProject * pProject)
    {
        AssertIfNull(m_ppCurrentProject);
        *m_ppCurrentProject = pProject;
    }

private:
    AssemblyRefInfo *m_pAssemblyRefInfo;        // The assembly ref whose projects need to be iterated.
    bool m_fNonArrayProject1Accessed;           // True indicates that AssemblyRefInfo.m_pReferencedProject
                                                // has already been returned by Next().
    bool m_fNonArrayProject2Accessed;           // True indicates that AssemblyRefInfo.m_pAmbiguousReferencedProject
                                                // has already been returned by Next().
    unsigned m_iCurrent;                        // The current index into AssemblyRefInfo.m_pPossiblyAmbiguousReferencedVBProjects.
    CompilerProject **m_ppCurrentProject;       // The address of the project returned by Next().
};

#if IDE 
//*****************************************************************************
// Iterate over the projects that this project implicitly references.
//*****************************************************************************

struct ImplicitReferenceIterator
{
    ImplicitReferenceIterator(CompilerProject * pProject)
    {
        Init(pProject);
    }

    void Init(CompilerProject * pProject)
    {
        m_pProject = pProject;
        Reset();
    }

    void Reset()
    {
        m_AssemblyRefIter.Init(m_pProject);
        m_pCurrentAssemblyRef = m_AssemblyRefIter.Next();
        m_AssemblyRefProjIter.Init(m_pCurrentAssemblyRef);
    }

    CompilerProject * Next();

private:
    CompilerProject *m_pProject;

    AssemblyRefIterator m_AssemblyRefIter;
    AssemblyRefInfo *m_pCurrentAssemblyRef;
    AssemblyRefReferencedProjectsIterator m_AssemblyRefProjIter;
};

//*****************************************************************************
// Iterate over the projects that implicitly reference this one.
//*****************************************************************************

struct ReverseImplicitReferenceIterator
{
    ReverseImplicitReferenceIterator(CompilerProject * pProject);

    CompilerProject * Next()
    {
        while (m_iCurrent < m_cEntries)
        {
            CompilerProject *pProject = m_ReverseAssemblyRefArray[m_iCurrent++]->m_pReferencingProject;

            if (pProject)
            {
                return pProject;
            }
        }

        return NULL;
    }

    AssemblyRefInfo * GetCurrentAssemblyRef()
    {
        if (m_iCurrent > 0)
        {
            return m_ReverseAssemblyRefArray[m_iCurrent - 1];
        }

        return NULL;
    }

    unsigned GetCurrentIndex() 
    {
         return (m_iCurrent > 0) ? (m_iCurrent - 1) : 0; 
    }

private:
    AssemblyRefInfo **m_ReverseAssemblyRefArray;
    unsigned m_cEntries;
    unsigned m_iCurrent;
};

//*****************************************************************************
// Iterate over all of the project level imports
//*****************************************************************************

struct ProjectImportsIterator
{
    // If you use this, you must call Init before Next or NextRef.
    ProjectImportsIterator() {}

    // This calls Init for you.
    ProjectImportsIterator(_In_ CompilerProject *pProject)
    {
        Init(pProject);
    }

    void Init(_In_ CompilerProject *pProject);

    unsigned long GetCount()
    {
        return m_cEntries;
    }

    BSTR Next();

private:
    CompilerProject::ProjectLevelImportsList::ProjectLevelImportInfo *m_pProjectLevelImportInfo;
    unsigned m_cEntries;
};
#endif IDE 

struct NamespacePrecedenceInfo
{
    NamespacePrecedenceInfo(
        BCSYM_Namespace * pNamespace,
        unsigned long precedenceLevel) :
        m_pNamespace(pNamespace),
        m_precedenceLevel(precedenceLevel)
    {
    }

    NamespacePrecedenceInfo() :
        m_pNamespace(NULL),
        m_precedenceLevel(0)
    {
    }

    bool operator <(const NamespacePrecedenceInfo &other) const
    {
        return
            m_pNamespace < other.m_pNamespace ||
            (
                m_pNamespace == other.m_pNamespace &&
                m_precedenceLevel < other.m_precedenceLevel
            );
    }

    bool operator ==(const NamespacePrecedenceInfo &other) const
    {
        return m_pNamespace == other.m_pNamespace && m_precedenceLevel == other.m_precedenceLevel;
    }

    bool operator >(const NamespacePrecedenceInfo &other) const
    {
        return
            m_pNamespace > other.m_pNamespace ||
            (
                m_pNamespace == other.m_pNamespace &&
                m_precedenceLevel > other.m_precedenceLevel
            );
    }

    BCSYM_Namespace * m_pNamespace;
    unsigned long m_precedenceLevel;
};

//Iterates over a namespace ring, yielding all
//of its namespaces that are visibile from the
//a given project
class VisibleNamespacesInRingIterator :
    public IConstIterator<BCSYM_Namespace *>
{
public:
    VisibleNamespacesInRingIterator(
        BCSYM_NamespaceRing * pRing = NULL,
        CompilerProject * pReferencingProject = NULL);

    bool MoveNext();

    BCSYM_Namespace * Current();

    void Reset(
        BCSYM_NamespaceRing * pRing,
        CompilerProject * pReferencingProject);

    void ResetRingOnly(BCSYM_NamespaceRing * pRing);

    bool IsCurrentValid();
private:
    void SimpleMoveNext();
    BCSYM_Namespace * m_pCurrent;
    CompilerProject * m_pProject;
    bool m_fBeforeStart;

};

class TypeExtensionIterator : public IConstIterator<BCSYM_Class *>
{
public:
    TypeExtensionIterator(BCSYM_Namespace * pNamespace = NULL);
    bool MoveNext();
    BCSYM_Class * Current();
    void Reset(BCSYM_Namespace * pNamespace);
private:
    BCITER_CHILD m_childIterator;
    BCSYM_Class * m_pCurrent;
};

class TypeExtensionImportsIterator : public IConstIterator<BCSYM_Class *>
{
public:
    TypeExtensionImportsIterator(ImportedTarget * pImportsList = NULL);
    bool MoveNext();
    BCSYM_Class * Current();
    void Reset(ImportedTarget * pImportsList);
private:
    ImportedTarget * m_pCurrent;
    bool m_fBeforeStart;
};

class ImportedNamespaceRingIterator : public IConstIterator<BCSYM_NamespaceRing *>
{
public:
    ImportedNamespaceRingIterator(_In_opt_ ImportedTarget * pImportsList = NULL);
    bool MoveNext();
    BCSYM_NamespaceRing * Current();
    void Reset(_In_opt_ ImportedTarget * pImportsList);
private:
    bool TargetIsNamespace(_In_opt_ ImportedTarget * pTarget);
    ImportedTarget * m_pCurrent;
    bool m_beforeStart;
};

class Semantics;

class ExtensionMethodOverloadIterator : public IConstIterator<Declaration *>
{
public:
    ExtensionMethodOverloadIterator(
        Declaration * pDecl,
        _In_ Semantics * pSemantics);
    
    bool MoveNext();
    
    Declaration * Current();

    void Reset(
        Declaration * pDecl,
        _In_ Semantics * pSemantics);

protected:
    virtual bool IsValid(Declaration * pDecl);

private:
    Declaration * m_pCurrent;
    Semantics * m_pSemantics;
    bool m_fBeforeStart;
};


//This is essentally a bredth-first-search of an entry in the
//extension method name lookup cache. It takes a cache entry
//and yeilds the sequence of all extension methods
//represented by that cache entry in order of relative precedence.
class ExtensionMethodCacheEntryExplodingIterator :
    public IConstIterator<ExtensionCallAndPrecedenceLevel>
{
public:
    ExtensionMethodCacheEntryExplodingIterator(
        Compiler * pCompiler,
        Semantics * pSemantics,
        ExtensionMethodLookupCacheEntry * pCacheEntry = NULL,
        Container * pContext = NULL);

    bool MoveNext();
    ExtensionCallAndPrecedenceLevel Current();

    //We allow the iterator to be reset by changing the cache
    //entry and the call context, but not the compiler or semantics instance.
    //The compiler can't be changed because the m_Queue and m_typeSet
    //members cannot have their allocators changed, and their allocators
    //user the compiler. We don't have a lot of situtations where
    //we would need to "explode" extension method cache entries
    //using the same iterator instance and different compiler / semantics instances
    //anyways, so this shouldn't be an issue. If you need this behavior then you
    //will have to modfiy the HashSet and Queue classes accordingly (or just use
    //a different iterator instance which would be much easier anyways).
    void Reset(
        ExtensionMethodLookupCacheEntry * pCacheEntry,
        Container * pContext);

private:
    ExtensionMethodOverloadIterator m_overloadIterator;
    Queue<CacheEntryAndBool> m_Queue;
    HashSet<BCSYM_Class *> m_typeSet;
    unsigned long m_precedenceLevel;
    Semantics * m_pSemantics;
    Container * m_pContext;
};

class GenericBindingArgumentIterator : public IConstIterator<Type *>
{
public:
    GenericBindingArgumentIterator(GenericBinding * pGenericBinding);
    bool MoveNext();
    Type * Current();
private:
    BCSYM ** m_ppArguments;
    unsigned long m_nextIndex;
    unsigned long m_count;

};

class XmlNamespaceImportsIterator : public IConstIterator<ImportedTarget *>
{
public:
    XmlNamespaceImportsIterator
    (
        SourceFile * pFile
    );
    bool MoveNext();
    ImportedTarget * Current()
    {
        return m_pCurrent;
    }
private:
    SourceFile * m_pFile;
    ImportedTarget * m_pCurrent;
    bool m_fInProjectLevel;
};

class PublicReadablePropertiesIterator
{
public:
    PublicReadablePropertiesIterator(BCSYM_Container * pContainer = NULL);
    BCSYM_Property * Next();
    void Reset();
    void Init(BCSYM_Container * pContainer);
private:
    BCSYM_Container * m_pContainer;
    BCITER_CHILD m_childIterator;
};





class ArrayElementIterator : public IConstIterator<ILTree::Expression *>
{
private:
    ILTree::ExpressionWithChildren * m_current;
    Filo<ILTree::ExpressionWithChildren *> m_stack;
    ILTree::ArrayLiteralExpression *m_start;
public:    
    ArrayElementIterator(ILTree::ArrayLiteralExpression *pLiteral) : m_stack(), m_current(NULL), m_start(pLiteral) {}
    virtual ILTree::Expression * Current() {ThrowIfNull(m_current); return m_current->Left;}
    virtual bool MoveNext();
};
