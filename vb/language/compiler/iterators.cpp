//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Project and file iterators.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

//****************************************************************************
// File iterator implementation
//****************************************************************************

//============================================================================
// Initialize or reinitialize the iterator.
//============================================================================

void FileIterator::Init(_In_opt_ CompilerProject * pcompproj)
{
    if (pcompproj != NULL)
    {
        m_pfileNext = pcompproj->m_pfileList;
    }
    else
    {
        m_pfileNext =  NULL;
    }
}

//============================================================================
// Gets the next file or returns NULL if there are no more.
//============================================================================

CompilerFile *FileIterator::Next()
{
    CompilerFile *pfileRet = m_pfileNext;

    if (m_pfileNext)
    {
        m_pfileNext = m_pfileNext->GetNextFile();
    }

    return pfileRet;
}

//****************************************************************************
// SourceFile iterator implementation
//****************************************************************************

//============================================================================
// Initialize or reinitialize the iterator.
//============================================================================

void SourceFileIterator::Init
(
    _In_opt_ CompilerProject *pcompproj
)
{
    if (pcompproj != NULL)
    {
        m_pfileNext = pcompproj->m_pfileList;
    }
    else
    {
        m_pfileNext =  NULL;
    }
}

//============================================================================
// Gets the next file or returns NULL if there are no more.
//============================================================================

SourceFile *SourceFileIterator::Next()
{
    CompilerFile *pfileRet = m_pfileNext;

    while (pfileRet && !pfileRet->IsSourceFile())
    {
        pfileRet = pfileRet->GetNextFile();
    }

    if (pfileRet)
    {
        m_pfileNext = pfileRet->GetNextFile();
    }
    else
    {
        m_pfileNext = NULL;
    }

    return pfileRet ? pfileRet->PSourceFile() : NULL;
}

//****************************************************************************
// MetaDataFile iterator implementation
//****************************************************************************

//============================================================================
// Initialize or reinitialize the iterator.
//============================================================================

void MetaDataFileIterator::Init
(
    _In_opt_ CompilerProject *pcompproj
)
{
    if (pcompproj != NULL)
    {
        m_pfileNext = pcompproj->m_pfileList;
    }
    else
    {
        m_pfileNext =  NULL;
    }
}

//============================================================================
// Gets the next file or returns NULL if there are no more.
//============================================================================

MetaDataFile *MetaDataFileIterator::Next()
{
    CompilerFile *pfileRet = m_pfileNext;

    while (pfileRet && !pfileRet->IsMetaDataFile())
    {
        pfileRet = pfileRet->GetNextFile();
    }

    if (pfileRet)
    {
        m_pfileNext = pfileRet->GetNextFile();
    }
    else
    {
        m_pfileNext = NULL;
    }

    return pfileRet ? pfileRet->PMetaDataFile() : NULL;
}

//*****************************************************************************
// Iterate over all metadata files in all projects
//*****************************************************************************


//*****************************************************************************
// Iterator for all types defined in a file.
//*****************************************************************************

//============================================================================
// Initialize or reinitialize the iterator.
//============================================================================

void AllContainersInFileIterator::Init
(
    CompilerFile *pFile,
    bool fIncludePartialContainers,
    bool fIncludeTransientSymbols
)
{
    if (pFile != NULL)
    {
        SymbolList *pList = pFile->GetNamespaceLevelSymbolList();

        m_pNext = pList->GetFirst();
    }
    else
    {
        m_pNext =  NULL;
    }

    m_fIncludePartialContainers = fIncludePartialContainers;
    m_fIncludeTransientSymbols = fIncludeTransientSymbols;
}

//============================================================================
// Gets the next file or returns NULL if there are no more.
//============================================================================

BCSYM_Container *AllContainersInFileIterator::Next()
{
    BCSYM_NamedRoot *pDug;

    while(m_pNext)
    {
        pDug = m_pNext->DigThroughAlias()->PNamedRoot();

        m_pNext = m_pNext->GetNextInSymbolList();

        if (!m_fIncludeTransientSymbols && pDug->IsTransient())
        {
            continue;
        }

        if (pDug->IsContainer() &&
            (m_fIncludePartialContainers || !pDug->PContainer()->IsPartialTypeAndHasMainType()))
        {
            return pDug->PContainer();
        }
    }

    return NULL;
}

//*****************************************************************************
// Iterator for all types defined in a file in declaration order.
//*****************************************************************************

//============================================================================
// Initialize or reinitialize the iterator.
//============================================================================

void AllContainersInFileInOrderIterator::Init
(
    SourceFile *pFile,
    bool fIncludePartialContainers,
    bool fIncludeTransientTypes
)
{

    // Free the memory.
    m_nra.FreeHeap();

    if (pFile != NULL)
    {
        SymbolList *pList = pFile->GetNamespaceLevelSymbolList();
        unsigned cContainers = pList->GetCount();
        BCSYM_Container **ppContainer;
        BCSYM_NamedRoot *pCurrent, *pContainer;

        IfFalseThrow(cContainers + 1 > cContainers);
        ppContainer = m_ppNext = (BCSYM_Container **)m_nra.Alloc(VBMath::Multiply(
            (cContainers + 1), 
            sizeof(BCSYM_Container *)));

        for (pCurrent = pList->GetFirst(); pCurrent; pCurrent = pCurrent->GetNextInSymbolList())
        {

            pContainer = pCurrent->DigThroughAlias()->PNamedRoot();

            if (pContainer->IsContainer() &&
                (fIncludePartialContainers || !pContainer->PContainer()->IsPartialTypeAndHasMainType()) &&
                (fIncludeTransientTypes || !pContainer->IsTransientClass())
               )
            {
                *ppContainer = pContainer->PContainer();
                ppContainer++;
            }
        }

        // Sort the list in file definition order.
        extern int _cdecl SortSymbolsByLocation(const void *arg1, const void *arg2);

        qsort(m_ppNext, ppContainer - m_ppNext, sizeof(BCSYM_Container *), SortSymbolsByLocation);
    }
    else
    {
        m_ppNext =  NULL;
    }
}

//============================================================================
// Gets the next file or returns NULL if there are no more.
//============================================================================

BCSYM_Container *AllContainersInFileInOrderIterator::Next()
{
    BCSYM_Container *pDug;

    if (m_ppNext && *m_ppNext)
    {
        pDug = *m_ppNext;

        m_ppNext++;

        return pDug;
    }

    return NULL;
}

void AllEmitableTypesInFileInOrderIterator::Init
(
    SourceFile *pFile
)
{
    // Free the memory previously used
    m_pListOfEmitableTypes.Destroy();

    if (pFile != NULL && pFile->GetUnnamedNamespace())
    {
        int NestingLevel = 0;
        GetEmitableTypesInContainer(
            pFile->GetUnnamedNamespace(),
            NestingLevel);

        // Sort the list in file definition order.
        extern int _cdecl SortEmitableTypesByLocationAndNestingLevels(const void *arg1, const void *arg2);

        qsort(m_pListOfEmitableTypes.Array(), m_pListOfEmitableTypes.Count(), sizeof(EmitableTypeWrapper), SortEmitableTypesByLocationAndNestingLevels);
    }

    m_CurrentTypeIndex = 0;
}

int _cdecl SortEmitableTypesByLocationAndNestingLevels(const void *arg1, const void *arg2)
{
    typedef AllEmitableTypesInFileInOrderIterator::EmitableTypeWrapper EmitableTypeWrapper;

    EmitableTypeWrapper *TypeWrapper1, *TypeWrapper2;

    TypeWrapper1 = (EmitableTypeWrapper *)arg1;
    TypeWrapper2 = (EmitableTypeWrapper *)arg2;

    if (int NestingLevelCmp = (TypeWrapper1->m_NestingLevel - TypeWrapper2->m_NestingLevel))
    {
        return NestingLevelCmp;
    }

    // if types are at the same nested level, then continue sorting by location
    //
    return SortSymbolsByLocation(&TypeWrapper1->m_pEmitableType, &TypeWrapper2->m_pEmitableType);
}

void AllEmitableTypesInFileInOrderIterator::GetEmitableTypesInContainer
(
    BCSYM_Container *pContainer,
    _Inout_ int &NestingLevel
)
{
    if (pContainer->IsPartialTypeAndHasMainType())
    {
        return;
    }

    if (!pContainer->IsNamespace())
    {
        EmitableTypeWrapper Wrapper = { pContainer, NestingLevel };

        m_pListOfEmitableTypes.AddElement(Wrapper);
    }

    NestingLevel++;

    BCSYM_Container *pCurrentPartialContainer = pContainer;
    do
    {
        for(BCSYM_Container *pNestedContainer = pCurrentPartialContainer->GetNestedTypes();
            pNestedContainer;
            pNestedContainer = pNestedContainer->GetNextNestedType())
        {
            GetEmitableTypesInContainer(
                pNestedContainer,
                NestingLevel);
        }
    }
    while (pCurrentPartialContainer = pCurrentPartialContainer->GetNextPartialType());

    NestingLevel--;

    return;
}

BCSYM_Container *AllEmitableTypesInFileInOrderIterator::Next()
{
    if (m_CurrentTypeIndex >= m_pListOfEmitableTypes.Count())
    {
        return NULL;
    }

    return m_pListOfEmitableTypes.Element(m_CurrentTypeIndex++).m_pEmitableType;
}

//*****************************************************************************
// Iterator for all of the members of a container that need to be emitted
// into metadata.
//*****************************************************************************

//============================================================================
// Initialize or reinitialize the iterator.
//============================================================================

void SortedMembersInAContainer::Init
(
    BCSYM_Container *pContainer,
    int (__cdecl *pfnCompare) (const void *arg1, const void *arg2)
)
{
    // Free the memory.
    m_nra.FreeHeap();

    if (pContainer != NULL)
    {
        BCITER_CHILD_ALL bichild(pContainer);
        unsigned cMembers = bichild.GetCount();
        BCSYM_NamedRoot *pRoot;
        BCSYM_NamedRoot **ppRoot;

        IfFalseThrow(cMembers + 1 > cMembers);
        ppRoot = m_ppNext = (BCSYM_NamedRoot **)m_nra.Alloc(VBMath::Multiply(
            (cMembers + 1), 
            sizeof(BCSYM_NamedRoot *)));

        while (pRoot = bichild.GetNext())
        {
            *ppRoot = pRoot;
            ppRoot++;
        }

        qsort(m_ppNext, ppRoot - m_ppNext, sizeof(BCSYM_NamedRoot *), pfnCompare);
    }
    else
    {
        m_ppNext = NULL;
    }
}

//============================================================================
// Gets the next file or returns NULL if there are no more.
//============================================================================

BCSYM_NamedRoot *SortedMembersInAContainer::Next()
{
    BCSYM_NamedRoot *pDug;

    if (m_ppNext && *m_ppNext)
    {
        pDug = *m_ppNext;

        m_ppNext++;

        return pDug;
    }

    return NULL;
}


//============================================================================
// Gets the next file or returns NULL if there are no more.
//============================================================================

BCSYM_NamedRoot *MembersInAContainerBase::Next()
{
    BCSYM_NamedRoot *pDug;

    if (m_ppNext && *m_ppNext)
    {
        pDug = *m_ppNext;

        m_ppNext++;

        return pDug;
    }

    return NULL;
}

//============================================================================
// Reset the iterator to the 1st element
//============================================================================
void MembersInAContainerBase::Reset()
{
    m_ppNext = m_ppRoot;
}


//*****************************************************************************
// Iterator for all of the members of a container that need to be emitted
// into metadata for the com class interface.
//*****************************************************************************

//============================================================================
// Check wither this named root is included in this iterator.
//============================================================================
bool ComClassMembersInAContainer::Filter
(
    BCSYM_NamedRoot *pRoot
)
{
    if (pRoot->GetAccess() != ACCESS_Public)
        return false;   // must be Public
    if (!pRoot->IsProc())
        return false;   // must be a procedure
    if (pRoot->IsSyntheticMethod())
        return false;   // can't be a synthetic method
    if (pRoot->IsDllDeclare())
        return false;   // can't be a declare
    if (pRoot->IsEventDecl())
        return false;   // can't be an event

    BCSYM_Proc *pProc = pRoot->DigThroughAlias()->PProc();

    if (pProc->IsAnyConstructor())
        return false;   // can't be a constructor
    if (pProc->IsShared())
        return false;   // can't be shared/static

    bool ComVisible;    // can't be <ComVisible(False)>
    if (pProc->GetPWellKnownAttrVals()->GetCOMVisibleData(&ComVisible) && !ComVisible)
        return false;

    if (pProc->IsProperty()) // Can't be a synthetic property (e.g., SYNTH_WithEventsGet)
    {
        BCSYM_Proc *pProcGet = pProc->PProperty()->GetProperty();
        if (pProcGet != NULL && pProcGet->IsSyntheticMethod())
        {
            return false;
        }
        // Check the setter just for sanity. I'm not expecting ever to find a synthetic readonly property.
        VSASSERT(((pProcGet = pProc->PProperty()->SetProperty()) == NULL) || !pProcGet->IsSyntheticMethod(), "A synthetic readonly property slipped through on a ComClass interface.");
    }

    return true;
}


//============================================================================
// Initialize or reinitialize the iterator.
//============================================================================

void ComClassMembersInAContainer::Init
(
    BCSYM_Container *pContainer
)
{
    // Free the memory.
    m_nra.FreeHeap();

    if (pContainer != NULL)
    {
        BCITER_CHILD bichild(pContainer);
        unsigned cMembers = 0;
        BCSYM_NamedRoot *pRoot;
        BCSYM_NamedRoot **ppRoot;

        while (pRoot = bichild.GetNext())
        {
            if (!Filter(pRoot))
                continue;

            cMembers++;

            if (pRoot->IsProperty())
            {
                BCSYM_Property *pProperty = pRoot->PProperty();

                if (pProperty->GetProperty() && Filter(pProperty->GetProperty()))
                {
                    cMembers++;
                }
                if (pProperty->SetProperty() && Filter(pProperty->SetProperty()))
                {
                    cMembers++;
                }
            }
        }

        IfFalseThrow(cMembers + 1 > cMembers);
        ppRoot = m_ppRoot = m_ppNext = (BCSYM_NamedRoot **)m_nra.Alloc(VBMath::Multiply(
            (cMembers + 1), 
            sizeof(BCSYM_NamedRoot *)));
        m_count = cMembers;

        bichild.Init(pContainer);

        while (pRoot = bichild.GetNext())
        {
            if (!Filter(pRoot))
                continue;

            *ppRoot = pRoot;
            ppRoot++;

            if (pRoot->IsProperty())
            {
                BCSYM_Property *pProperty = pRoot->PProperty();

                if (pProperty->GetProperty() && Filter(pProperty->GetProperty()))
                {
                    *ppRoot = pProperty->GetProperty();
                    ppRoot++;
                }
                if (pProperty->SetProperty() && Filter(pProperty->SetProperty()))
                {
                    *ppRoot = pProperty->SetProperty();
                    ppRoot++;
                }
            }
        }

        // Sort the list in slot number then file definition order.
        extern int _cdecl SortSymbolsByLocation(const void *arg1, const void *arg2);

        qsort(m_ppNext, ppRoot - m_ppNext, sizeof(BCSYM_NamedRoot *), SortSymbolsByLocation);
    }
    else
    {
        m_count = 0;
        m_ppNext = NULL;
        m_ppRoot = NULL;
    }
}


//*****************************************************************************
// Iterator for all of the members of a container that need to be emitted
// into metadata for the com class events interface.
//*****************************************************************************

//============================================================================
// Check wither this named root is included in this iterator.
//============================================================================
bool ComClassEventsInAContainer::Filter
(
    BCSYM_NamedRoot *pRoot
)
{
    if (pRoot->GetAccess() != ACCESS_Public)
        return false;   // must be public
    if (!pRoot->IsEventDecl())
        return false;   // must be an Event

    BCSYM_EventDecl *pEvent = pRoot->DigThroughAlias()->PEventDecl();

    if (pEvent->IsShared())
        return false;   // can't be shared/static

    bool ComVisible;    // can't be <ComVisible(False)>
    if (pEvent->GetPWellKnownAttrVals()->GetCOMVisibleData(&ComVisible) && !ComVisible)
        return false;

    return true;
}


//============================================================================
// Initialize or reinitialize the iterator.
//============================================================================

void ComClassEventsInAContainer::Init
(
    BCSYM_Container *pContainer
)
{
    // Free the memory.
    m_nra.FreeHeap();

    if (pContainer != NULL)
    {
        BCITER_CHILD bichild(pContainer);
        unsigned cMembers = 0;
        BCSYM_NamedRoot *pRoot;
        BCSYM_NamedRoot **ppRoot;

        while (pRoot = bichild.GetNext())
        {
            if (!Filter(pRoot))
                continue;
            cMembers++;
        }

        IfFalseThrow(cMembers + 1 > cMembers);
        ppRoot = m_ppRoot = m_ppNext = (BCSYM_NamedRoot **)m_nra.Alloc(VBMath::Multiply(
            (cMembers + 1), 
            sizeof(BCSYM_NamedRoot *)));
        m_count = cMembers;

        bichild.Init(pContainer);

        while (pRoot = bichild.GetNext())
        {
            if (!Filter(pRoot))
                continue;

            *ppRoot = pRoot;
            ppRoot++;
        }

        // Sort the list in slot number then file definition order.
        extern int _cdecl SortSymbolsByLocation(const void *arg1, const void *arg2);

        qsort(m_ppNext, ppRoot - m_ppNext, sizeof(BCSYM_NamedRoot *), SortSymbolsByLocation);
    }
    else
    {
        m_count = 0;
        m_ppNext = NULL;
        m_ppRoot = NULL;
    }
}


//*****************************************************************************
// Iterator for all of the MethodImpls and SyntheticMethods defined in
// a container.
//*****************************************************************************

//============================================================================
// Initialize or reinitialize the iterator.
//============================================================================

void CompileableMethodsInAContainer::Init
(
    BCSYM_Container *pContainer,
    bool fLookOnlyInCurrentPartialType
)
{
    // Free the memory.
    m_nra.FreeHeap();

    if (pContainer != NULL)
    {
        // Look in only this container and ignore its other partial containers
        //
        BCITER_CHILD_ALL bichild(pContainer, fLookOnlyInCurrentPartialType);

        unsigned cMethods = 0;
        BCSYM_NamedRoot *pRoot;
        BCSYM_Proc **ppProc;

        while (pRoot = bichild.GetNext())
        {
            if (pRoot->IsMethodImpl() ||
                    pRoot->IsSyntheticMethod() ||
                    pRoot->IsProperty() ||
                    pRoot->IsEventDecl())
            {
                cMethods++;
            }
        }

        IfFalseThrow(cMethods + 1 > cMethods);
        ppProc = m_ppNext = (BCSYM_Proc **)m_nra.Alloc(VBMath::Multiply(
            (cMethods + 1), 
            sizeof(BCSYM_Proc *)));

        bichild.Reset();

        while (pRoot = bichild.GetNext())
        {
            if (pRoot->IsMethodImpl() ||
                    pRoot->IsSyntheticMethod() ||
                    pRoot->IsProperty() ||
                    pRoot->IsEventDecl())
            {
                *ppProc = pRoot->PProc();
                ppProc++;
            }
        }

        // Sort the list in file definition order.
        extern int _cdecl SortSymbolsByLocation(const void *arg1, const void *arg2);

        qsort(m_ppNext, ppProc - m_ppNext, sizeof(BCSYM_Proc *), SortSymbolsByLocation);
    }
    else
    {
        m_ppNext = NULL;
    }
}

//============================================================================
// Gets the next file or returns NULL if there are no more.
//============================================================================

BCSYM_Proc *CompileableMethodsInAContainer::Next()
{
    BCSYM_Proc *pDug;

    if (m_ppNext && *m_ppNext)
    {
        pDug = *m_ppNext;

        m_ppNext++;

        return pDug;
    }

    return NULL;
}

//*****************************************************************************
// Iterate over all of the projects that this one references.
//*****************************************************************************
void ReferenceIterator::Init(_In_ CompilerProject *pProject)
{
    m_rgref = pProject->m_daReferences.Array();
    m_cEntries = pProject->m_daReferences.Count();
    m_iCurrent = 0;
}

CompilerProject *ReferenceIterator::Next()
{
    ReferencedProject* pRef = NextReference();
    return pRef ? pRef->m_pCompilerProject : NULL;
}

ReferencedProject* 
ReferenceIterator::NextReference()
{
    if (m_iCurrent < m_cEntries)
    {
        return &(m_rgref[m_iCurrent++]);
    }

    return NULL;
}

//*****************************************************************************
// Iterate over the projects that reference this one.
//*****************************************************************************

void ReverseReferenceIterator::Init(_In_ CompilerProject *pProject)
{
    m_ReverseProjectReferenceArray = pProject->m_daReferencesOnMe.Array();
    m_cEntries = pProject->m_daReferencesOnMe.Count();
    m_iCurrent = 0;
}

//*****************************************************************************
// Iterate over the assembly refs in this metadata project
//*****************************************************************************

AssemblyRefInfo *AssemblyRefIterator::Next()
{
    while (m_pCurrentFile)
    {
        if (NULL == m_pAssemblyRefsForCurrentFile)
        {
            m_pAssemblyRefsForCurrentFile = m_pCurrentFile->GetAssemblyRefs();
            m_CurrentIndex = 0;
        }
        else
        {
            m_CurrentIndex++;
        }

        if (m_CurrentIndex < m_pAssemblyRefsForCurrentFile->GetAssemblyRefCount())
        {
            return m_pAssemblyRefsForCurrentFile->GetAssemblyRefInfo(m_CurrentIndex);
        }

        // Done enumerating over the assembly refs in the current file. Advance to the
        // next metadata file.

        m_pAssemblyRefsForCurrentFile = NULL;
        m_pCurrentFile = m_MetadataFileIter.Next();
    }

    return NULL;
}

CompilerProject *AssemblyRefReferencedProjectsIterator::Next()
{
    CompilerProject **ppProject = NULL;

    if (!m_fNonArrayProject1Accessed)
    {
        m_fNonArrayProject1Accessed = true;
        ppProject = &m_pAssemblyRefInfo->m_pReferencedProject;
    }

    if ((!ppProject || !(*ppProject)) &&
        !m_fNonArrayProject2Accessed)
    {
        m_fNonArrayProject2Accessed = true;
        ppProject = &m_pAssemblyRefInfo->m_pAmbiguousReferencedProject;
    }

    if ((!ppProject || !(*ppProject)) &&
        m_pAssemblyRefInfo->m_pPossiblyAmbiguousReferencedVBProjects)
    {
        while ((!ppProject || !(*ppProject)) &&
                m_iCurrent < m_pAssemblyRefInfo->m_pPossiblyAmbiguousReferencedVBProjects->Count())
        {
            ppProject = &m_pAssemblyRefInfo->m_pPossiblyAmbiguousReferencedVBProjects->Element(m_iCurrent++);
        }
    }

    m_ppCurrentProject = ppProject;

    return (ppProject && *ppProject) ? *ppProject : NULL;
}

#if IDE 

//*****************************************************************************
// Iterate over all of the project level imports
//*****************************************************************************
void ProjectImportsIterator::Init(_In_ CompilerProject *pProject)
{
    m_pProjectLevelImportInfo = pProject->m_Imports.m_pProjectLevelImportInfos;

    unsigned int iCount = 0;
    CompilerProject::ProjectLevelImportsList::ProjectLevelImportInfo *pProjectLevelImportInfo = m_pProjectLevelImportInfo;
    while (pProjectLevelImportInfo)
    {
        iCount++;
        pProjectLevelImportInfo = pProjectLevelImportInfo->m_Next;
    }

    m_cEntries = iCount;
}

BSTR ProjectImportsIterator::Next()
{
    if (m_pProjectLevelImportInfo)
    {
        CComBSTR bstrProjectImport(m_pProjectLevelImportInfo->m_StrLen, m_pProjectLevelImportInfo->m_OriginalImportString);
        m_pProjectLevelImportInfo = m_pProjectLevelImportInfo->m_Next;
        return bstrProjectImport.Detach();
    }

    return NULL;
}

#endif

VisibleNamespacesInRingIterator::VisibleNamespacesInRingIterator
(
    BCSYM_NamespaceRing * pRing,
    CompilerProject * pReferencingProject
) :
    m_pCurrent(NULL),
    m_fBeforeStart(true),
    m_pProject(NULL)
{
    Reset(pRing, pReferencingProject);
}

void VisibleNamespacesInRingIterator::SimpleMoveNext()
{
    //Namespaces are defined in rings any should always have a next pointer
    if (m_pCurrent && m_pCurrent->GetNextNamespace() != m_pCurrent->GetNamespaceRing()->GetFirstNamespace())
    {
        m_pCurrent = m_pCurrent->GetNextNamespace();
    }
    else
    {
        m_pCurrent = NULL;
    }
}

bool VisibleNamespacesInRingIterator::MoveNext()
{
    if (m_fBeforeStart)
    {
        m_fBeforeStart = false;
    }
    else
    {
        do
        {
            SimpleMoveNext();
        } while (m_pCurrent && !NamespaceIsAvailable(m_pProject, m_pCurrent));
    }
    return m_pCurrent;
}

BCSYM_Namespace * VisibleNamespacesInRingIterator::Current()
{
    return m_pCurrent;
}

void VisibleNamespacesInRingIterator::Reset
(
    BCSYM_NamespaceRing * pRing,
    CompilerProject * pProject
)
{
    m_pProject = pProject;
    m_fBeforeStart = true;

    if (pRing)
    {
        Assume(pRing->GetFirstNamespace(), L"The provided ring does not have a namespace pointer!");
        Assume(pRing->GetFirstNamespace()->GetNamespaceRing()== pRing, L"The ring's first namespace has an invalid ring pointer!");
        m_pCurrent = pRing->GetFirstNamespace();

        while (!NamespaceIsAvailable(m_pProject, m_pCurrent))
        {
            SimpleMoveNext();
        }
    }
    else
    {
        m_pCurrent = NULL;
    }
}

void VisibleNamespacesInRingIterator::ResetRingOnly(BCSYM_NamespaceRing * pRing)
{
    Reset(pRing, m_pProject);
}

bool VisibleNamespacesInRingIterator::IsCurrentValid()
{
    return ! m_fBeforeStart && m_pCurrent;
}

TypeExtensionIterator::TypeExtensionIterator
(
    BCSYM_Namespace * pNamespace
) :
    m_pCurrent(NULL)
{
    Reset(pNamespace);
}

void TypeExtensionIterator::Reset
(
    BCSYM_Namespace * pNamespace
)
{
    m_pCurrent = NULL;
    m_childIterator.Init(pNamespace, false);
}


bool TypeExtensionIterator::MoveNext()
{
    BCSYM_NamedRoot * pNamed = NULL;
    do
    {
        pNamed = m_childIterator.GetNext();
        if (pNamed && pNamed->IsContainer())
        {
            BCSYM_Container *pContainer = pNamed->PContainer();

            // Perf: We are only interested in looking at types marked as containing extension methods.  So only load the 
            // type's children if the type says it contains extension methods.

            BCSYM *pContainerAfterDiggingThroughAlias = pContainer->DigThroughAlias();

            if (pContainerAfterDiggingThroughAlias->IsClass())
            {
                if (pContainerAfterDiggingThroughAlias->PClass()->ContainsExtensionMethods())
                {
                    pContainer->EnsureChildrenLoaded();
                }
            }
            else if (!pContainer->GetCompilerFile() ||
                (pContainer->GetCompilerFile()->IsMetaDataFile() && pContainer->GetCompilerFile()->ContainsExtensionMethods()))
            {
                pContainer->EnsureChildrenLoaded();
            }
        }
    }
    while (pNamed && ! pNamed->IsTypeExtension());

    if (pNamed)
    {
        m_pCurrent = pNamed->PClass();
    }
    else
    {
        m_pCurrent = NULL;
    }

    return m_pCurrent;
}

BCSYM_Class * TypeExtensionIterator::Current()
{
    return m_pCurrent;
}

TypeExtensionImportsIterator::TypeExtensionImportsIterator
(
    ImportedTarget * pImportsList
) :
    m_pCurrent(NULL),
    m_fBeforeStart(true)

{
    Reset(pImportsList);
}

bool TypeExtensionImportsIterator::MoveNext()
{
    if (m_fBeforeStart)
    {
        m_fBeforeStart = false;
    }
    else
    {
        do
        {
            m_pCurrent = m_pCurrent->m_pNext;

        }
        while  (m_pCurrent && (!m_pCurrent->m_pTarget || !m_pCurrent->m_pTarget->IsTypeExtension() || m_pCurrent->m_pstrAliasName));
    }

    return m_pCurrent;
}

BCSYM_Class * TypeExtensionImportsIterator::Current()
{
    return m_pCurrent ? m_pCurrent->m_pTarget->PClass() : NULL;
}

void TypeExtensionImportsIterator::Reset(ImportedTarget * pImportsList)
{
    m_pCurrent = pImportsList;

    while
    (
        m_pCurrent &&
        (
            !m_pCurrent->m_pTarget ||
            !m_pCurrent->m_pTarget->IsTypeExtension() ||
            m_pCurrent->m_pstrAliasName
        )
    )
    {
        m_pCurrent = m_pCurrent->m_pNext;
    }
    m_fBeforeStart = true;
}

ImportedNamespaceRingIterator::ImportedNamespaceRingIterator
(
    _In_opt_ ImportedTarget * pImportsList
) :
    m_pCurrent(NULL),
    m_beforeStart(false)
{
    Reset(pImportsList);
}


bool ImportedNamespaceRingIterator::MoveNext()
{
    do
    {
        if (m_beforeStart)
        {
            m_beforeStart = false;
        }
        else if (m_pCurrent)
        {
            m_pCurrent = m_pCurrent->m_pNext;
        }
    } while (m_pCurrent && !TargetIsNamespace(m_pCurrent));

    return m_pCurrent;
}

BCSYM_NamespaceRing * ImportedNamespaceRingIterator::Current()
{
    return
        m_pCurrent->m_pTarget->PNamespace()->GetNamespaceRing();
}

bool ImportedNamespaceRingIterator::TargetIsNamespace(_In_opt_ ImportedTarget * pTarget)
{
    return
        pTarget &&
        pTarget->m_pTarget &&
        pTarget->m_pTarget->IsNamespace() &&
        !pTarget->m_pstrAliasName;
}
void ImportedNamespaceRingIterator::Reset(_In_opt_ ImportedTarget * pImportsList)
{
    m_pCurrent = pImportsList;
    m_beforeStart = true;
}

ExtensionMethodOverloadIterator::ExtensionMethodOverloadIterator
(
    Declaration * pDecl,
    _In_ Semantics * pSemantics
) :
    m_pCurrent(NULL),
    m_pSemantics(NULL),
    m_fBeforeStart(true)
{
    Reset(pDecl, pSemantics);
}

void ExtensionMethodOverloadIterator::Reset(Declaration * pDecl, _In_ Semantics * pSemantics)
{
    m_pCurrent = pDecl;
    m_fBeforeStart = true;
    m_pSemantics = pSemantics;
}

bool ExtensionMethodOverloadIterator::MoveNext()
{
    bool currentIsValid = false;
    while
    (
        m_fBeforeStart ||
        (
            m_pCurrent &&
            !currentIsValid
        )
    )
    {
        if (m_fBeforeStart)
        {
            m_fBeforeStart = false;
        }
        else
        {
            m_pCurrent = m_pCurrent->GetNextOverload();
        }

        currentIsValid = IsValid(m_pCurrent);
    }

    return m_pCurrent;
}

Declaration * ExtensionMethodOverloadIterator::Current()
{
    ThrowIfNull(m_pCurrent);
    Assume(m_pCurrent->IsProc(), L"How can m_pCurrent not be a procedure?");
    return m_pCurrent;
}

bool ExtensionMethodOverloadIterator::IsValid(Declaration * pDecl)
{
    return
        pDecl &&
        pDecl->IsExtensionMethod(m_pSemantics->IsInterpretingForAttributeExpression()) &&
        m_pSemantics->IsAccessible
        (
            pDecl,
            NULL,
            NULL
        );
}


ExtensionMethodCacheEntryExplodingIterator::ExtensionMethodCacheEntryExplodingIterator
(
    Compiler * pCompiler,
    Semantics * pSemantics,
    ExtensionMethodLookupCacheEntry * pCacheEntry,
    Container * pContext
) :
    m_pSemantics(pSemantics),
    m_overloadIterator(NULL, NULL),
    m_precedenceLevel(0),
    m_pContext(NULL)
{
    ThrowIfNull(pCompiler);
    ThrowIfNull(pSemantics);
    Reset(pCacheEntry, pContext);
}

bool ExtensionMethodCacheEntryExplodingIterator::MoveNext()
{
    bool flag = m_overloadIterator.MoveNext();

    while (flag || m_Queue.Count() > 0)
    {
        if (flag)
        {
            if (m_pSemantics->IsAccessible(m_overloadIterator.Current(), m_pContext))
            {
                return true;
            }
            else
            {
                flag = m_overloadIterator.MoveNext();
            }
        }
        else if (m_Queue.Count() > 0)
        {
            CacheEntryAndBool entry = m_Queue.PopOrDequeue();

            if (entry.m_increasePrecedenceLevel)
            {
                m_precedenceLevel += 1;
            }
            ListValueIterator<CacheEntryAndRelativePrecedenceInfo, NorlsAllocWrapper> iterator(&entry.m_pCacheEntry->m_parentCacheEntries);


            bool first = true;
            unsigned long lastRelativePrecedenceLevel = 0;

            while (iterator.MoveNext())
            {
                CacheEntryAndRelativePrecedenceInfo loopVar = iterator.Current();

                m_Queue.PushOrEnqueue(CacheEntryAndBool(loopVar.m_pCacheEntry, first || lastRelativePrecedenceLevel != loopVar.m_relativePrecedenceInfo));
                first = false;
                lastRelativePrecedenceLevel = loopVar.m_relativePrecedenceInfo;
            }

            if (entry.m_pCacheEntry->m_method)
            {
                BCSYM_Class * pClass = entry.m_pCacheEntry->m_method->GetContainingClass();
                if (pClass->GetMainType())
                {
                    pClass = pClass->GetMainType();
                }

                //Because a type may be imported into scope via multiple precedence levels
                //(say once via a type import and once via a namespace import, or once via a
                //source file import and once via a project-level import), it is possible to have
                //mulitple cache entries for the same class. We don't want to consider that
                //class twice in overload resolution, so we keep a set of the types
                //that we have seen already.
                if (! m_typeSet.Contains(pClass))
                {
                    if (m_pSemantics->IsAccessible(pClass, m_pContext))
                    {
                        //Reset the extension method overload iterator
                        //so that it will yield the overloads of the method stored in the cache entry.
                        m_overloadIterator.Reset(entry.m_pCacheEntry->m_method,m_pSemantics);
                        flag = m_overloadIterator.MoveNext();
                    }
                    m_typeSet.Add(pClass);
                }
                else
                {
                    flag = false;
                }
            }

        }
        else
        {
            Assume(false, L"This code should not be reachable!");
        }
    }
    return false;
}

ExtensionCallAndPrecedenceLevel ExtensionMethodCacheEntryExplodingIterator::Current()
{
    return ExtensionCallAndPrecedenceLevel(ViewAsProcedure(m_overloadIterator.Current()), m_precedenceLevel);
}

void ExtensionMethodCacheEntryExplodingIterator::Reset
(
    ExtensionMethodLookupCacheEntry * pCacheEntry,
    Container * pContext
)
{
    m_typeSet.Clear();
    m_Queue.Clear();
    if (pCacheEntry)
    {
        m_Queue.PushOrEnqueue(CacheEntryAndBool(pCacheEntry, false));
    }
    m_precedenceLevel = 0;
    m_pContext = pContext;
    m_overloadIterator.Reset(NULL, NULL);
}


XmlNamespaceImportsIterator::XmlNamespaceImportsIterator
(
    SourceFile * pFile
)
{
    m_pFile = pFile;
    m_pCurrent = NULL;
    m_fInProjectLevel = false;
}

bool XmlNamespaceImportsIterator::MoveNext()
{
    if (m_pCurrent)
    {
        // Move to next import
        m_pCurrent = m_pCurrent->m_pNext;
    }
    else if (!m_fInProjectLevel && m_pFile && m_pFile->HaveImportsBeenResolved())
    {
        // Initially get file-level Xml imports
        m_pCurrent = m_pFile->GetUnnamedNamespace()->GetImports();
    }

    // Find valid xml import
    while (true) {

        if (m_pCurrent)
        {
            if (m_pCurrent->m_IsXml && !m_pCurrent->m_hasError)
            {
                return true;
            }

            m_pCurrent = m_pCurrent->m_pNext;
        }
        else
        {
            // If we've already checked the project level, then done
            if (m_fInProjectLevel)
            {
                return false;
            }

            // Get project level imports
            if (m_pFile)
            {
                m_pCurrent = m_pFile->GetCompilerProject()->GetImportedTargets();
            }

            m_fInProjectLevel = true;
        }
    }
}

GenericBindingArgumentIterator::GenericBindingArgumentIterator
(
    GenericBinding * pGenericBinding
) :
    m_ppArguments(NULL),
    m_nextIndex(NULL),
    m_count(NULL)
{
    ThrowIfNull(pGenericBinding);

    m_ppArguments = pGenericBinding->GetArguments();
    m_nextIndex = 0;
    m_count = pGenericBinding->GetArgumentCount();
}

bool GenericBindingArgumentIterator::MoveNext()
{
    if (m_nextIndex <= m_count)
    {
        ++m_nextIndex;
    }
    return m_nextIndex <= m_count;
}

Type * GenericBindingArgumentIterator::Current()
{
    ThrowIfFalse(m_nextIndex <= m_count && m_count);
    return m_ppArguments[m_nextIndex - 1];
}

#if IDE 
//*****************************************************************************
// Iterate over the projects that this project implicitly references.
//*****************************************************************************

CompilerProject *ImplicitReferenceIterator::Next()
{
    while (m_pCurrentAssemblyRef)
    {
        CompilerProject *pReferencedProject = m_AssemblyRefProjIter.Next();

        if (pReferencedProject)
        {
            return pReferencedProject;
        }

        // Done enumerating over the projects that matched the current assembly ref.
        // Advance to the next assembly ref.

        m_pCurrentAssemblyRef = m_AssemblyRefIter.Next();
        m_AssemblyRefProjIter.Init(m_pCurrentAssemblyRef);
    }

    return NULL;
}

//*****************************************************************************
// Iterate over the projects that implicitly reference this one.
//*****************************************************************************

ReverseImplicitReferenceIterator::ReverseImplicitReferenceIterator(CompilerProject *pProject)
{
    m_ReverseAssemblyRefArray = pProject->m_daImplicitReferencesOnMe.Array();
    m_cEntries = pProject->m_daImplicitReferencesOnMe.Count();
    m_iCurrent = 0;
}
#endif IDE


PublicReadablePropertiesIterator::PublicReadablePropertiesIterator(BCSYM_Container * pContainer)
{
    Init(pContainer);
}


BCSYM_Property * PublicReadablePropertiesIterator::Next()
{
    BCSYM_NamedRoot * pNamedRoot = NULL;
    while
    (
        (pNamedRoot = m_childIterator.GetNext()) &&
        (
            ! pNamedRoot->IsProperty() ||
            pNamedRoot->GetAccess() != ACCESS_Public ||
            pNamedRoot->PProperty()->IsWriteOnly()
        )
    )
    {
    }

    return pNamedRoot ? pNamedRoot->PProperty() : NULL;
}

void PublicReadablePropertiesIterator::Reset()
{
    m_childIterator.Init(m_pContainer);
}

void PublicReadablePropertiesIterator::Init(BCSYM_Container * pContainer)
{
    m_pContainer = pContainer;
    Reset();
}







bool ArrayElementIterator::MoveNext()
{
    // The input structure looks like this:
    //
    //   {e0,e1}  SX_ArrayLiteral
    //              +-ElementList = SX_LIST
    //                                +-Left = e0
    //                                +-Right = SX_List
    //                                            +- Left = e1
    //                                            +- Right = NULL
    //
    //  {{e0},{e1}} SX_ArrayLiteral
    //                +-ElementList = SX_LIST
    //                                  +-Left = SX_NestedArrayLiteral
    //                                  |          +-ElementList = SX_LIST
    //                                  |                            +-Left = e0
    //                                  |                            +-Right = NULL
    //                                  +-Right = SX_LIST
    //                                              +-Left = SX_NestedArrayLiteral
    //                                              |          +-ElementList = SX_LIST
    //                                              |                            +-Left = e1
    //                                              |                            +-Right = NULL
    //                                              +-Right = NULL
    //
    // The argument "inputElementList" is always an SX_LIST node.
    // The function scans through the Left/Right tree of the SX_LIST iteratively, looking at the "Left" leaves.
    // When it encounters a "SX_NestedArrayLiteral" as one of the Left leaves, then it calls itself
    // recursively with the ElementList of that nested array literal.
    //
    // This rank-2 array literal "{{e0},{e1}}" has two SX_NestedArrayLiterals as in the above tree,
    // and we will populate the output list with "e0" and "e1"
    // This rank-1 array literal "{CType({e0},Integer()), CType({e1},Integer())}" has EXACTLY THE SAME TREE!
    // but we have to populate the output list with "{e0}" and "{e1}", both of them being arrays.
    //
    // What distinguishes the two examples is the "ResultType" attached to each SX_NestedArrayLiteral.
    // In the first case, the ResultType is just Void. In the second case, the ResultType of each one
    // is Integer().
    //
    // Anyway, for our iterator, "m_current" is always on an SX_LIST node such that m_current.Left is the
    // current iterator value.
    // And if we had to recurse into a nested array literal, then the first item we pop of m_stack will
    // be the SX_LIST whose Left is the SX_NESTEDARRAYLITERAL that we were in.

    while (true)
    {
        if (m_start!=NULL)
        {
            VSASSERT(m_start!=NULL, "please construct ArrayElementIterator with a non-null ArrayLiteralExpression");
            m_current = m_start != NULL ? m_start->ElementList : NULL;
            m_start = NULL;
        }
        else if (m_current!=NULL)
        {
            m_current = m_current->Right!=NULL ?  &m_current->Right->AsExpressionWithChildren() : NULL;
        }
        else if (m_current==NULL && m_stack.Count()==0)
        {
            return false;
        }
        
        while (true)
        {
            if (m_current!=NULL && m_current->Left!=NULL && m_current->Left->bilop!=SX_NESTEDARRAYLITERAL)
            {
                return true;
            }
            else if (m_current!=NULL && m_current->Left!=NULL && m_current->Left->bilop==SX_NESTEDARRAYLITERAL)
            {
                m_stack.PushOrEnqueue(m_current);
                m_current = m_current->Left->AsNestedArrayLiteralExpression().ElementList;
            }
            else if (m_current==NULL && m_stack.Count()>0)
            {
                m_current = m_stack.PopOrDequeue();
                break;
            }
            else
            {
                break;
            }
        }
    }
}

