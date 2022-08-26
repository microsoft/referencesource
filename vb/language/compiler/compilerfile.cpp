//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implements the file-level compilation logic.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

#if DEBUG && IDE 
FileLeakDetection s_FileLeakDetection;
#endif

//****************************************************************************
//****************************************************************************
//****************************************************************************
//
// CompilerFile implementation
//
//****************************************************************************
//****************************************************************************
//****************************************************************************

//============================================================================
// CompilerFile constructor
//============================================================================

CompilerFile::CompilerFile(Compiler *pCompiler)
    : m_ErrorTable(pCompiler, NULL, NULL)
    , m_nraSymbols(NORLSLOC)
    , m_pCompiler(pCompiler)
    , m_cs(CS_MAX)
    , m_isUnlinkedFromProject(false)
    , m_pExtensionData(NULL)
#if IDE 
    , m_eventDeclaredState("CompilerFile Declared Event", pCompiler)
    , m_DecompilationState(CS_MAX)
#endif IDE
    , m_DeclaredToBoundGenericBindingCache(&m_nraSymbols)
    , m_NamespaceTree(&m_nraSymbols)
    , m_ContainsExtensionMethods(false)
{
    CheckInMainThread();
}

//============================================================================
// CompilerFile destructor
//============================================================================

CompilerFile::~CompilerFile()
{
    CheckInMainThread();

#if IDE 
    UnwatchFile();
#endif

    // Remove the namespaces this file declared from the circular list.
    RemoveNamespaces();

    UnlinkFromProject();
}

void CompilerFile::UnlinkFromProject()
{
    m_isUnlinkedFromProject = true;

    // Unlink from the project.
    if (m_cs < CS_MAX)
    {
        if (m_pfilePrev)
        {
            m_pfilePrev->m_pfileNext = m_pfileNext;
        }
        else
        {
            m_pProject->m_pfileList = m_pfileNext;
        }

        if (m_pfileNext)
        {
            m_pfileNext->m_pfilePrev = m_pfilePrev;
        }

#if IDE 
        m_pProject->RemoveFileFromTree(this);
#endif
        m_pProject->m_dlFiles[m_cs].Remove(this);
        m_pProject->ClearFileCompilationArray();
        m_cs = CS_MAX;

#if IDE 
        SetDecompilationState(m_cs);
#endif
    }
}

#if HOSTED
void CompilerFile::SetCompState(CompilationState cs)
{
        m_pProject->m_dlFiles[m_cs].Remove(this);
        m_pProject->m_dlFiles[cs].InsertLast(this);
        m_cs = cs;
}
#endif


//============================================================================
// CompilerFile creation method.
//============================================================================

HRESULT CompilerFile::Init(
    FileType ft,
    CompilerProject * pProject)
{
    HRESULT hr = NOERROR;

    //
    // Store file information.
    //

    m_filetype = ft;
    m_pProject = pProject;

    //
    // Reinit the error table.
    //

    m_ErrorTable.Init(
        m_pCompiler,
        m_pProject,
        IsSourceFile() ? PSourceFile()->GetLineMarkerTable() : NULL);

    //
    // Link onto the file list.
    //

    m_pfileNext = pProject->m_pfileList;

    if (pProject->m_pfileList)
    {
        pProject->m_pfileList->m_pfilePrev = this;
    }

    pProject->m_pfileList = this;

    RRETURN( hr );
}

bool CompilerFile::CompileAsNeeded()
{
#if IDE 
    // If generating "ENCable" code under the IDE, then always emit the Xml helper class into the result assembly
    // so that debugging experience is better (i.e. allow Xml features to be used in expressions and immediate window).
    if (m_pProject->GenerateENCableCode())
    {
        return false;
    }
#endif
    // Return true if this file will only be compiled into the solution if something within it is referenced

/*
     
    VB Core can not be marked as CompileAsNeeded for the following reason.
    
    This causes VB Core to be skipped during codegen and transient symbols must be used
    to emit the VB core. Problem is that core methods are referenced during IL generation
    which is too late to introduce transient symbols.
    if (m_pExtensionData)
    {
        switch ( m_pExtensionData->m_extension )
        {
        case SolutionExtension::VBCoreExtension:
        case SolutionExtension::XmlHelperExtension:
            return true;
        }
    }
    return false;
*/
    return m_pExtensionData && m_pExtensionData->m_extension == SolutionExtension::XmlHelperExtension;

}

BCSYM_Namespace* CompilerFile::GetRootNamespace()
{
    if (IsSourceFile())
    {
        return this->PSourceFile()->GetRootNamespace();
    }
    else
    {
        return GetUnnamedNamespace();
    }
}

BCSYM_Namespace *CompilerFile::GetNamespace(_In_z_ STRING *strNamespaceName)
{
    NamespaceNode *pNamespaceNode = NULL;

    if (m_NamespaceTree.Find(&strNamespaceName, &pNamespaceNode))
    {
        return pNamespaceNode->pNamespace;
    }

    return NULL;
}

void CompilerFile::AddNamespace(_In_z_ STRING *strNamespaceName, BCSYM_Namespace *pNamespace)
{
    NamespaceNode *pNamespaceNode = NULL;

    if (strNamespaceName && pNamespace)
    {
        if (m_NamespaceTree.Insert(&strNamespaceName, &pNamespaceNode))
        {
            pNamespaceNode->pNamespace = pNamespace;
        }
    }
}

CompilerHost *CompilerFile::GetCompilerHost()
{
    VSASSERT(m_pProject, "NULL Project!");
    return m_pProject->GetCompilerHost();
}

#if IDE 

//****************************************************************************
// IUnknown implementation
//****************************************************************************

STDMETHODIMP CompilerFile::QueryInterface
(
    REFIID riid,
    void **ppv
)
{
    void *pv = NULL;

    if (riid == IID_IUnknown)
    {
        pv = static_cast<IUnknown *>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    ((IUnknown *)pv)->AddRef();

    *ppv = pv;

    return NOERROR;
}

STDMETHODIMP_(ULONG) CompilerFile::AddRef()
{
    return InterlockedIncrement((long *)&m_cRefs);
}

STDMETHODIMP_(ULONG) CompilerFile::Release()
{
    ULONG cRefs = InterlockedDecrement((long *)&m_cRefs);

    if (cRefs == 0)
    {
#if DEBUG
        s_FileLeakDetection.Remove(this);
#endif DEBUG

        // Run the destructor on the file to clean up its resources.
        delete(this);
    }

    return cRefs;
}

#endif IDE

void CompilerFile::ClearingBindingStatus()
{
#if IDE 
    m_BindingStatus_Started = false;
    m_NeedToDecompileFileFromPartiallyBoundState = false;
#endif IDE

    if (this->IsSourceFile())
    {
#if IDE 
        this->PSourceFile()->m_BindingStatus_BindingContainersDone = false;
#endif IDE
        this->PSourceFile()->m_DelayBindingCompletion = false;
        this->PSourceFile()->m_HasPartialTypes = false;
    }
}


//============================================================================
//  Remove any namespaces from the circular linked list of namespaces when the file is demoted to NoState.
//
//  This function causes a brute force recursive search through the file level symbol table
//  for any namespaces from this file and removes them from the linked list of namespaces.
//
//============================================================================

void CompilerFile::RemoveNamespaces()
{
    if (m_pUnnamedNamespace)
    {
#if IDE 
        if (IsSourceFile())
        {
            Bindable::DeleteAllBindableInstancesInFile(this->PSourceFile());

            Bindable::ClearProjectAssemblyAttrInfoDueToFile(this->PSourceFile());
        }
#endif

        RemoveNamespaces( m_pUnnamedNamespace );
        m_pUnnamedNamespace = NULL;

        if (IsSourceFile())
        {
            this->PSourceFile()->SetRootNamespace(NULL);
        }

        m_NamespaceTree.Clear();
    }
}

void CompilerFile::RemoveNamespaces(BCSYM_Namespace *NamespaceToRemove)
{
    // First, release the links of any namespaces that are nested in this one
    BCITER_CHILD NamespaceIter;
    NamespaceIter.Init( NamespaceToRemove );

    for ( BCSYM_NamedRoot * NamespaceSymbol = NamespaceIter.GetNext(); NamespaceSymbol; NamespaceSymbol = NamespaceIter.GetNext())
    {
        if ( NamespaceSymbol->IsNamespace())
        {
            RemoveNamespaces( NamespaceSymbol->PNamespace());
        }
    }

    if(NamespaceToRemove)
    {
        GetCompiler()->RemoveNamespace( NamespaceToRemove );
    }
}

#if IDE 

HRESULT CompilerFile::WatchFile()
{
    HRESULT hr = NOERROR;

    if (m_pstrFileName && GetCompilerPackage())
    {
        hr = GetCompilerPackage()->WatchFile(m_pstrFileName);

        if (SUCCEEDED(hr))
        {
            VSASSERT(!m_isWatched, "Should not be watched!");
            m_isWatched = true;
#if ID_TEST
            if (VBFTESTSWITCH(fDumpFileChangeNotifications))
            {
                WCHAR wszFile[MAX_PATH];
                DebPrintf("CompilerFile::WatchFile: '%S' watched\n", GetCanonicalFileName(m_pstrFileName, wszFile, DIM(wszFile)));
            }
#endif ID_TEST
        }
    }

    return hr;
}

HRESULT CompilerFile::UnwatchFile()
{
    HRESULT hr = NOERROR;

    if (m_isWatched && (m_pstrFileName != NULL) && GetCompilerPackage())
    {
        hr = GetCompilerPackage()->UnwatchFile(m_pstrFileName);

        if (SUCCEEDED(hr))
        {
            VSASSERT(m_isWatched, "Should be watched!");
            m_isWatched = false;
#if ID_TEST
            if (VBFTESTSWITCH(fDumpFileChangeNotifications))
            {
                WCHAR wszFile[MAX_PATH];
                DebPrintf("CompilerFile::UnwatchFile: '%S' unwatched\n", GetCanonicalFileName(m_pstrFileName, wszFile, DIM(wszFile)));
            }
#endif ID_TEST
        }
    }

    return hr;
}

CompilationState CompilerFile::GetHighestStartedCompState()
{
    if (m_cs == CS_Declared &&
        IsSourceFile() &&
        PSourceFile()->HasBindingStarted())
    {
        return CS_Bound;
    }

    return m_cs;
}
#endif IDE

GenericBindingCache* CompilerFile::GetCurrentGenericBindingCache()
{
    // It is only worthwhile to cache bindings created in the long term allocator i.e.
    // the ones create when transitioning from declared to bound for sourcefiles and
    // always for metadata files.
    //
    // If ever any caching needs to be done for any state above bindable, then a separate
    // cache needs to be used for that state because the allocator memory used to construct
    // the bindings goes away when demoting from that state.
    //
    if (this &&
        (m_cs == CS_Declared || IsMetaDataFile()))
    {
        return &m_DeclaredToBoundGenericBindingCache;
    }

    return NULL;
}


int _cdecl
SortSourceFileByName
(
    const void *arg1,
    const void *arg2
)
{
    SourceFile *pFile1 = *(SourceFile **) arg1;
    SourceFile *pFile2 = *(SourceFile **) arg2;

    if (pFile1 == pFile2)
    {
        return 0;
    }

    int iCompare = CompareValues(BoolToInt(pFile2->IsSolutionExtension()), BoolToInt(pFile1->IsSolutionExtension()));

    if (iCompare == 0)
    {
        VSASSERT(pFile1->GetFileName() && pFile2->GetFileName(), "How can a source file have a NULL name ?");

        return CompareNoCase(pFile1->GetFileName(), pFile2->GetFileName());
    }

    return iCompare;
}

