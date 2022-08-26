//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implements the project-level compilation logic.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

#if IDE 
#define ENCDUMPTASKLISTKEY      L"VBENCSUITESDUMPTASKLIST"
#endif

#if IDE 
#define TMH_FILENAME "compilerproject.tmh"
#include "..\Include\EnableLogging.def"
#endif

// This is for fopen() and fclose()
#if ID_TEST
#include <stdio.h>

// Low-level file IO for dumping Unicode XML
#include <io.h>
#include <FCNTL.H>
#include <SYS\STAT.H>
#endif  // ID_TEST

// The list of predefined public keys used by Microsoft Frameworks assemblies.
// Used to detect framework assemblies in order to avoid loading private types from them.
static WCHAR FxPublicKeys[][17] =
{
    L"b77a5c561934e089", // BCL/CLR
    L"b03f5f7f11d50a3a", // .NET Framework
    L"7cec85d7bea7798e", // Silverlight (core CLR platform Cert 115 key)
    L"31bf3856ad364e35", // Windows
    L"71e9bce111e9429c", // Office
    L"89845dcd8080cc91", // SQL Servier
    L"03689116d3a4ae33", // Test key
    L"33aea4d316916803", // Test key
};

LiftedUserDefinedOperatorCache::LiftedUserDefinedOperatorCache(NorlsAllocator * pNorls) :
    m_cache(pNorls)
{
    ThrowIfNull(pNorls);
}

bool
LiftedUserDefinedOperatorCache::LookupInCache
(
    BCSYM_UserDefinedOperator * pSourceOperator,       //[in] - the source operator to look for
    BCSYM_UserDefinedOperator ** ppCacheValue          //[out] - stores the cached lifted operator, may be null
)
{
    Node * pNode = NULL;

    if
    (
        m_cache.Find(&Key(pSourceOperator), &pNode)
    )
    {
        if (ppCacheValue)
        {
            *ppCacheValue = pNode->pLiftedOperator;
        }
        return true;
    }
    else
    {
        return false;
    }

}

void
LiftedUserDefinedOperatorCache::AddEntry
(
    BCSYM_UserDefinedOperator * pSourceOperator,       //[in] - the source operator
    BCSYM_UserDefinedOperator * pCacheValue            //[in] - the lifted operator created from pSourceOperator to store in the cache.
)
{
    Node * pNode = NULL;

    m_cache.Insert(&Key(pSourceOperator), &pNode);
    pNode->pLiftedOperator = pCacheValue;
}

void LiftedUserDefinedOperatorCache::Clear()
{
    m_cache.Clear();
}

NorlsAllocator * LiftedUserDefinedOperatorCache::GetNorlsAllocator()
{
    return m_cache.GetNorlsAllocator();
}

LiftedUserDefinedOperatorCache::Key::Key() :
    pSourceOperator(NULL)
{
}

LiftedUserDefinedOperatorCache::Key::Key(BCSYM_UserDefinedOperator * pSrcOp) :
    pSourceOperator(pSrcOp)
{
}

bool LiftedUserDefinedOperatorCache::Key::operator <(const Key &other) const
{
    return pSourceOperator < other.pSourceOperator;
}

int LiftedUserDefinedOperatorCache::KeyOperations::compare(
    const Key * pKey1,
    const Key * pKey2)
{
    if (*pKey1 < * pKey2)
    {
        return -1;
    }
    else if (*pKey2 < * pKey1)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void LiftedUserDefinedOperatorCache::NodeOperations::copy(
    Node * pNodeDest,
    const Node * pNodeSrc)
{
    pNodeDest->pLiftedOperator = pNodeSrc->pLiftedOperator;
}

//forward declaration
bool IsEqual(
    DynamicArray<VbCompilerWarningItemLevel> * memberWarningsLevelTable,
    DWORD countWrnTable,
    VbCompilerWarningItemLevel optionWarningsLevelTable[]);

ExtensionMethodLookupCacheKeyObject::ExtensionMethodLookupCacheKeyObject()
{
    memset(this, 0, sizeof(ExtensionMethodLookupCacheKeyObject));
    m_type = KeyObject_Uninitialized;
}

ExtensionMethodLookupCacheKeyObject::ExtensionMethodLookupCacheKeyObject(
    SourceFile * pSourceFile,
    BCSYM_Container * pContext)
{
    memset(this, 0, sizeof(ExtensionMethodLookupCacheKeyObject));
    m_type = KeyObject_SourceFileSymbol;
    m_pSourceFile = pSourceFile;
    m_pContext = pContext;
}

ExtensionMethodLookupCacheKeyObject::ExtensionMethodLookupCacheKeyObject(BCSYM_NamedRoot * pSymbol)
{
    memset(this, 0, sizeof(ExtensionMethodLookupCacheKeyObject));
    m_type = KeyObject_Symbol;
    m_pSimpleObject = pSymbol;
}

ExtensionMethodLookupCacheKeyObject::ExtensionMethodLookupCacheKeyObject(SourceFile * pSourceFile)
{
    memset(this, 0, sizeof(ExtensionMethodLookupCacheKeyObject));
    m_type = KeyObject_SourceFile;
    m_pSimpleObject = pSourceFile;
}

int ExtensionMethodLookupCacheKeyObject::Compare(
    const ExtensionMethodLookupCacheKeyObject * pLeft,
    const ExtensionMethodLookupCacheKeyObject * pRight)
{
    return memcmp(pLeft, pRight, sizeof(ExtensionMethodLookupCacheKeyObject));
}

ExtensionMethodNameLookupCache::Key::Key(
    _In_ STRING * pMethodName,
    const ExtensionMethodLookupCacheKeyObject &keyObject,
    ExtensionMethodLookupCacheUsageType usageType) :
    m_pMethodName(pMethodName),
    m_keyObject(keyObject),
    m_usageType(usageType)
{
}

ExtensionMethodNameLookupCache::Key::Key() :
    m_pMethodName(NULL),
    m_keyObject(),
    m_usageType(Usage_Invalid)
{
}

int ExtensionMethodNameLookupCache::KeyOperations::compare(
    const Key * pKey1,
    const Key * pKey2)
{
    if (pKey1 && ! pKey2)
    {
        return 1;
    }
    else if (!pKey1 && pKey2)
    {
        return -1;
    }
    else if (!pKey1 && ! pKey2)
    {
        return 0;
    }
    else if (pKey1 == pKey2)
    {
        return 0;
    }
    else
    {
        STRING_INFO * leftStrInfo = StringPool::Pstrinfo(pKey1->m_pMethodName);
        STRING_INFO * rightStrInfo = StringPool::Pstrinfo(pKey2->m_pMethodName);

        if (leftStrInfo < rightStrInfo)
        {
            return -1;
        }
        else if (rightStrInfo < leftStrInfo)
        {
            return 1;
        }
        else  if (pKey1->m_usageType < pKey2->m_usageType)
        {
            return -1;
        }
        else if (pKey2->m_usageType < pKey1->m_usageType)
        {
            return 1;
        }
        else
        {
            return
                ExtensionMethodLookupCacheKeyObject::Compare
                (
                    &pKey1->m_keyObject,
                    &pKey2->m_keyObject
                );
        }
    }
}

void ExtensionMethodNameLookupCache::NodeOperations::copy(
    Node * pNodeDest,
    const Node * pNodeSrc)
{
    pNodeDest->pCacheEntry = pNodeSrc->pCacheEntry;
}

ExtensionMethodNameLookupCache::ExtensionMethodNameLookupCache(NorlsAllocator * pnorls) :
    m_cache(pnorls)
{
}

bool ExtensionMethodNameLookupCache::LookupInCache(
    _In_ STRING * pMethodName,
    const ExtensionMethodLookupCacheKeyObject &keyObject,
    ExtensionMethodLookupCacheUsageType usageType,
    ExtensionMethodLookupCacheEntry * * ppOutCacheValue)
{
    Node * pNode = NULL;
    bool ret = m_cache.Find(&Key(pMethodName, keyObject, usageType), &pNode);

    if (ret && pNode && ppOutCacheValue)
    {
        *ppOutCacheValue = pNode->pCacheEntry;
    }

    return ret;
}

void ExtensionMethodNameLookupCache::AddEntry(
    _In_ STRING * pMethodName,
    const ExtensionMethodLookupCacheKeyObject &keyObject,
    ExtensionMethodLookupCacheUsageType usageType,
    ExtensionMethodLookupCacheEntry * pCacheValue)
{
    Node * pNode = NULL;
    m_cache.Insert(&Key(pMethodName, keyObject, usageType), &pNode);

    if  (pNode)
    {
        pNode->pCacheEntry = pCacheValue;
    }
}

void ExtensionMethodNameLookupCache::Clear()
{
    m_cache.Clear();
}

NorlsAllocator * ExtensionMethodNameLookupCache::GetNorlsAllocator()
{
    return m_cache.GetNorlsAllocator();
}

CompilationCaches::CompilationCaches():
    m_nrlsCachedData(NORLSLOC),
    m_LookupCache(&m_nrlsCachedData),
    m_ExtensionMethodLookupCache(&m_nrlsCachedData),  
    m_LiftedOperatorCache(&m_nrlsCachedData),
    m_MergedNamespaceCache(&m_nrlsCachedData)
{
#if IDE  
    VSASSERT(GetCompilerSharedState()->IsInMainThread(), "GetCompilerSharedState()->IsInMainThread()");
#endif IDE  
}

#if IDE  
CompilationCaches::CompilationCaches(NorlsAllocator * pnorls):
    m_nrlsCachedData(NORLSLOC),
    m_LookupCache(pnorls),
    m_ExtensionMethodLookupCache(pnorls),  
    m_LiftedOperatorCache(pnorls),
    m_MergedNamespaceCache(pnorls)
{
    VSASSERT(GetCompilerSharedState()->IsInMainThread(), "GetCompilerSharedState()->IsInMainThread()");
    ThrowIfNull(pnorls);
}
#endif IDE  

CompilationCaches::~CompilationCaches()
{
    ClearCompilationCaches();
}


// Can be called fore/bdg and dbg threads.
void CompilationCaches::ClearCompilationCaches(int  cacheType /*= 0*/)
{
#if IDE
    VSASSERT(GetCompilerSharedState()->IsInMainThread(), "GetCompilerSharedState()->IsInMainThread()");
    if (cacheType == 0)
    {
        cacheType  = CompCacheType_AllCaches ;
        
    }
#if DEBUG        
// if you really want to validate them when cleared
//    ValidateCaches();

    // change this value to 1 in debugger: (g_Switch_fCompCaches).m_fSet
    if (VSFSWITCH(fCompCaches))
    {
        DumpStats();
    }
    
#endif DEBUG
    if (cacheType  & CompCacheType_LookUp)
    {
        m_LookupCache.Clear();
    }
    if (cacheType  & CompCacheType_Extension)
    {
        m_ExtensionMethodLookupCache.Clear();
    }
    if (cacheType  & CompCacheType_LiftedUDFOp )
    {
        m_LiftedOperatorCache.Clear();
    }
    if (cacheType  & CompCacheType_NSRing )
    {
        m_MergedNamespaceCache.Clear();
    }

    if (cacheType == CompCacheType_AllCaches )
    {
        m_nrlsCachedData.FreeHeap();
    }

#endif IDE

}

#if IDE
#if DEBUG
void CompilationCaches::ValidateCaches(int  cacheType /*= 0*/)
{
    VSASSERT(GetCompilerSharedState()->IsInMainThread(), "GetCompilerSharedState()->IsInMainThread()");
    if (cacheType == 0)
    {
        cacheType  = CompCacheType_AllCaches ;
        
    }
    
    if (cacheType  & CompCacheType_LookUp)
    {
        m_LookupCache.ValidateNodes();
    }
    if (cacheType  & CompCacheType_Extension)
    {
        m_ExtensionMethodLookupCache.m_cache.ValidateNodes();
    }
    if (cacheType  & CompCacheType_LiftedUDFOp )
    {
        m_LiftedOperatorCache.m_cache.ValidateNodes();
    }
    if (cacheType  & CompCacheType_NSRing )
    {
        m_MergedNamespaceCache.ValidateNodes();
    }

}

#if NRLSTRACK

bool AllocatorLifeTimeOk(NorlsAllocator *pRecordedAlloc, NorlsAllocator *pCompCacheAllocator )
{
    bool fRetval = true;
    if (pRecordedAlloc != pCompCacheAllocator)
    {
        if (CompareNoCase(pRecordedAlloc->m_szFile,L"compilerfile.cpp") != 0) // compilerfile.cpp(31) as allocator is ok)
        {
            fRetval = false;
        }
    }
    return fRetval;
    
}
#endif NRLSTRACK


template<> void LookupTree::ValidateNodes(void)
{
#if NRLSTRACK
    LookupTree::Iterator oLookupTreeIterator(this);
    LookupNode *pLookupNode;

    for (long i = 0 ; (pLookupNode = oLookupTreeIterator.Next()) ; i++)
    {
        if (pLookupNode->Result)
        {
            if (!AllocatorLifeTimeOk(pLookupNode->Result->GetRecordedAllocator(), m_pNoReleaseAllocator))
            {
                VSASSERT(false,"LookupTree Wrong allocator");
            }
        }
    }
#endif NRLSTRACK
}


template<> void ExtensionMethodNameLookupCache::tree_type::ValidateNodes(void)
{
#if NRLSTRACK
    ExtensionMethodNameLookupCache::tree_type::Iterator oExtensionMethodNameLookupCache(this);
    ExtensionMethodNameLookupCache::Node *pExtensionMethodLookupNode;

    for (long i = 0 ; (pExtensionMethodLookupNode = oExtensionMethodNameLookupCache.Next()) ; i++)
    {
        if (pExtensionMethodLookupNode->pCacheEntry)
        {
            if (pExtensionMethodLookupNode->pCacheEntry->m_method)
            {
                if (!AllocatorLifeTimeOk(pExtensionMethodLookupNode->pCacheEntry->m_method->GetRecordedAllocator(), m_pNoReleaseAllocator))
                {
                    VSASSERT(false,"ExtensionMethodLookupCache Wrong allocator");
                }
            }
            ListValueIterator<CacheEntryAndRelativePrecedenceInfo, NorlsAllocWrapper> iterator(&pExtensionMethodLookupNode->pCacheEntry->m_parentCacheEntries);

            while (iterator.MoveNext())
            {
                CacheEntryAndRelativePrecedenceInfo loopVar = iterator.Current();
                if (loopVar.m_pCacheEntry->m_method)
                {
                    if (!AllocatorLifeTimeOk(loopVar.m_pCacheEntry->m_method->GetRecordedAllocator(), m_pNoReleaseAllocator))
                    {
                        VSASSERT(false,"ExtensionMethodLookupCache parentcache Wrong allocator");
                    }
                }
            }
        }
    }
#endif NRLSTRACK
}

template<> void LiftedUserDefinedOperatorCache::tree_type::ValidateNodes(void)
{
#if NRLSTRACK
    LiftedUserDefinedOperatorCache::tree_type::Iterator oLiftedUserDefinedOperatorCacheIterator(this);
    LiftedUserDefinedOperatorCache::Node *pLiftedUserDefinedOperatorCacheNode;

    for (long i = 0 ; (pLiftedUserDefinedOperatorCacheNode = oLiftedUserDefinedOperatorCacheIterator.Next()) ; i++)
    {
        if (pLiftedUserDefinedOperatorCacheNode->pLiftedOperator)
        {
            if (!AllocatorLifeTimeOk(pLiftedUserDefinedOperatorCacheNode->pLiftedOperator->GetRecordedAllocator(), m_pNoReleaseAllocator))
            {
                VSASSERT(false,"pNamespaceRingTree Wrong allocator");
            }
        }
    }
#endif NRLSTRACK
}

template<> void NamespaceRingTree::ValidateNodes(void)
{
#if NRLSTRACK
    NamespaceRingTree::Iterator oNamespaceRingTreeIterator(this);
    NamespaceRingNode *pNamespaceRingNode;

    for (long i = 0 ; (pNamespaceRingNode = oNamespaceRingTreeIterator.Next()) ; i++)
    {
        if (pNamespaceRingNode->MergedHash)
        {
            if (!AllocatorLifeTimeOk(pNamespaceRingNode->MergedHash->GetRecordedAllocator(), m_pNoReleaseAllocator))
            {
                VSASSERT(false,"pNamespaceRingTree Wrong allocator");
            }
        }
    }
#endif NRLSTRACK
}

#define TEMPBUFSIZE   2048* sizeof(WCHAR)      

CComBSTR  LookupTree::GetNodeDump(_In_opt_ bool fOutputDebugWindow)
{
    CComBSTR bstrResult;
    WCHAR szBuffer[TEMPBUFSIZE];

    LookupTree::Iterator oLookupTreeIterator(this);
    LookupNode *pLookupNode;
    
    for (long i = 0 ; (pLookupNode = oLookupTreeIterator.Next()) ; i++)
    {
        StringBuffer sb;
        if (pLookupNode->Result)
        {
            sb.AppendString(L"Found '");
            pLookupNode->Result->GetBasicRep(GetCompilerPackage(), NULL, &sb);
            sb.AppendString(L"'");
        }

        StringCchPrintfW(szBuffer, sizeof(szBuffer), L" %4d  Sym='%ws'  Prj=(%ws): Scope=' %ws'%ws\r\n", i, 
            pLookupNode->key.Name->m_spelling.m_str,
            pLookupNode->key.m_pProject->GetFileNameWithNoPath(),
            static_cast<Namespace *>( pLookupNode->key.Scope)->GetSimpleName(),
            sb.GetString()
            );

        if (fOutputDebugWindow)
        {
            DebPrintf("%S", szBuffer);
        }
        bstrResult.Append(szBuffer);
                
    }
    return bstrResult;
}

CComBSTR ExtensionMethodNameLookupCache::tree_type::GetNodeDump(_In_opt_ bool fOutputDebugWindow)
{
    CComBSTR bstrResult;
    WCHAR szBuffer[TEMPBUFSIZE];

    ExtensionMethodNameLookupCache::tree_type::Iterator oExtensionMethodNameLookupCacheIterator(this);
    ExtensionMethodNameLookupCache::Node *pNode;

    for (long i = 0 ; (pNode = oExtensionMethodNameLookupCacheIterator.Next()) ; i++)
    {
        StringBuffer sb;
        switch(pNode->key.m_keyObject.m_type)
        {
            case KeyObject_SourceFileSymbol:
                {
                    sb.AppendString(pNode->key.m_keyObject.m_pContext->GetSimpleName());
                    sb.AppendString(L": ");
                    STRING *szFile= pNode->key.m_keyObject.m_pSourceFile->GetFileName();
                    PathStripPathW(szFile);
                    sb.AppendString(szFile);
                }
                break;
                
            case KeyObject_SourceFile:
                {
                    STRING *szFile = static_cast<SourceFile *>( pNode->key.m_keyObject.m_pSimpleObject)->GetFileName();
                    PathStripPathW(szFile);
                    sb.AppendString(szFile);
                }
                break;
                
            case KeyObject_Symbol:
                sb.AppendString(static_cast<BCSYM_NamedRoot *>( pNode->key.m_keyObject.m_pSimpleObject)->GetSimpleName());
                break;
                
            default:
                VSASSERT(false,"bad keyobject type for ExtensionMethodNameLookupCache");
        }
        StringBuffer sbFound;
        if (pNode->pCacheEntry)
        {
            sbFound.AppendString(L"Found '");
            if (pNode->pCacheEntry->m_method)
            {
                pNode->pCacheEntry->m_method->GetBasicRep(GetCompilerPackage(), NULL, &sbFound);
            }
            sbFound.AppendString(L"'");
        }

       
        StringCchPrintfW(szBuffer, sizeof(szBuffer), L" %4d %d:%d  Meth='%ws'  '%ws' %ws\r\n", i, 
            pNode->key.m_usageType,
            pNode->key.m_keyObject.m_type, 
            pNode->key.m_pMethodName, 
            sb.GetString(),
            sbFound.GetString());

        if (fOutputDebugWindow)
        {
            DebPrintf("%S", szBuffer);
        }
        bstrResult.Append(szBuffer);
    }
  return bstrResult;
}

CComBSTR LiftedUserDefinedOperatorCache::tree_type::GetNodeDump(_In_opt_ bool fOutputDebugWindow)
{
    CComBSTR bstrResult;
    WCHAR szBuffer[TEMPBUFSIZE];

    LiftedUserDefinedOperatorCache::tree_type::Iterator oLiftedUserDefinedOperatorCacheIterator(this);
    LiftedUserDefinedOperatorCache::Node *pNode;

    for (long i = 0 ; (pNode = oLiftedUserDefinedOperatorCacheIterator.Next()) ; i++)
    {
        StringBuffer sb, sbFound;
        pNode->key.pSourceOperator->GetBasicRep(GetCompilerPackage(), NULL, &sb);
        if (pNode->pLiftedOperator)
        {
            pNode->pLiftedOperator->GetBasicRep(GetCompilerPackage(), NULL, &sbFound);
        }
       
        StringCchPrintfW(szBuffer, sizeof(szBuffer), L" %4d '%ws'  '%ws' \r\n", i, 
            sb.GetString(),
            sbFound.GetString()
            );

        if (fOutputDebugWindow)
        {
            DebPrintf("%S", szBuffer);
        }
        bstrResult.Append(szBuffer);
    }
    return bstrResult;
}

CComBSTR NamespaceRingTree::GetNodeDump(_In_opt_ bool fOutputDebugWindow)
{
    CComBSTR bstrResult;
    WCHAR szBuffer[TEMPBUFSIZE];
    
    NamespaceRingTree::Iterator  oNamespaceRingTreeIterator(this);
    NamespaceRingNode *pNamespaceRingNode;
    for (long i = 0 ; (pNamespaceRingNode = oNamespaceRingTreeIterator.Next()) ; i++)
    {
        StringBuffer sb;
        sb.AppendString(pNamespaceRingNode->key.NamespaceRing->GetSimpleName());
        DebPrintf(" %4d %S \n", i, sb.GetString());
       
        StringCchPrintfW(szBuffer, sizeof(szBuffer), L" %4d %ws \r\n", i, 
            sb.GetString()
            );

        if (fOutputDebugWindow)
        {
            DebPrintf("%S", szBuffer);
        }
        bstrResult.Append(szBuffer);
    }
    return bstrResult;
}

void CompilationCaches::DumpStats()
{
    DebPrintf("Lookup  ");
    m_LookupCache.DumpTreeStats();
    m_LookupCache.GetNodeDump(true);
        
    DebPrintf("ExtMthd ");
    m_ExtensionMethodLookupCache.m_cache.DumpTreeStats();
    m_ExtensionMethodLookupCache.m_cache.GetNodeDump(true);
    
    DebPrintf("LftdOp  ");
    m_LiftedOperatorCache.m_cache.DumpTreeStats();
    m_LiftedOperatorCache.m_cache.GetNodeDump(true);

    DebPrintf("MrgdNsp ");
    m_MergedNamespaceCache.DumpTreeStats();
    m_MergedNamespaceCache.GetNodeDump(true);

    m_LookupCache.ClearTreeStats();
    m_ExtensionMethodLookupCache.m_cache.ClearTreeStats();
    m_LiftedOperatorCache.m_cache.ClearTreeStats();
    m_MergedNamespaceCache.ClearTreeStats();
    
}

#endif DEBUG
#endif IDE


// helper function Can be called fore/bdg and dbg threads. For background, should return NULL
CompilationCaches *
GetCompilerCompilationCaches
(
)
{

    CompilationCaches  *pCompilationCaches  = NULL;
#if IDE
    if (GetCompilerSharedState()->IsInMainThread())
    {

        pCompilationCaches = GetCompilerPackage()->GetCompilationCaches();

// VALIDATE_Caches is expensive. 
#if DEBUG_VALIDATE_Caches
//        g_Switch_fCompCaches.m_fSet = 1;
        pCompilationCaches->ValidateCaches();
#endif DEBUG_VALIDATE_Caches
    }
#endif IDE
    return pCompilationCaches;
}



#if IDE 
FireOnStatementChangedList::FireOnStatementChangedList() 
: m_NoReleaseAllocator(NORLSLOC)
{

}

#endif

//****************************************************************************
// Lifetime
//****************************************************************************

//============================================================================
// CompilerProject Constructor
//============================================================================

CompilerProject::CompilerProject(Compiler * pCompiler)
: m_ErrorTable(pCompiler, this, NULL)
, m_norlsHashTables(NORLSLOC)
, m_Friends(&m_norlsHashTables)
, m_nraSymbols(NORLSLOC)
, m_ShouldImportPrivates(true) // default to the safe setting (import everything)
, m_CheckedAllMetadataFilesForAccessibility(false)

#if IDE 
, m_nraProjectLifetime(NORLSLOC)
, m_treeFiles(&m_nraProjectLifetime)
#endif
, m_pCompiler(pCompiler)
, m_fCodeModule(false)
, m_fIsCompilationArraySorted(false)
, m_mdassembly(mdAssemblyNil)
, m_pCompilerHost(NULL)
#if IDE 
, m_fGetUnusedReferences(false)
, m_eventBoundState("Bound event", pCompiler)
, m_eventTypesEmittedState("TypesEmitted event", pCompiler)
, m_eventCompiledState("Compiled event", pCompiler)
, m_eventDeclaredState("Declared Event", pCompiler)
#endif // IDE 
#if IDE 
, m_pCodeModel(NULL)
, m_pExternalCodeModel(NULL)
#endif
, m_pImage(NULL)
, m_pStreamPDB(NULL)
, m_Builder(pCompiler, this)
#if IDE 
, m_pSinkSite( NULL )
, m_pENCBuilder(NULL)
, m_fIsInDebugBreak(false)
, m_pTaskProvider(NULL)
, IsProjectLevelInformationReady(false)
, m_fQueriedForContextualIntellisenseFilter(false)
, m_pContextualIntellisenseFilter(NULL)
, m_iDeclChangeIndex(1)
, m_iBoundChangeIndex(1)
, m_iSourceChangeIndex(1) // We start at 1 because we need to be different at startup.
, m_cPostedMessageCount(0)
, m_fIsValid(true)
#endif IDE
, m_cRefs( 1 )
, m_fIsTooLong(false)
, m_projectSeen(false)
, m_projectSortInProgress(false)
, m_fDeclaredToBoundCompilationInProgress(false)
, m_fHaveImportsBeenResolved(false)
#if DEBUG
, m_fAreImportsBeingResolved(false)
#endif DEBUG
, m_fProjectClaimsCLSCompliance(false)
#if IDE 
,m_fProjectClaimedCLSCompliancePreviously(false)
,m_pFileThatCausedProjectsCLSComplianceClaim(NULL)
,m_fProjectAndFilesDecompToBoundStarted(false)
,m_fProjectAndFilesDecompToDeclaredStarted(false)
,m_fEmbeddedResourcesEmitted(false)
,m_FileChangedNotificationCounter(0)
,m_fCachedEntryPoints(false)
,m_fShouldSuspendNotifications(false)
,m_IsPosted_ProjectInBindable(false)
,m_IsPosted_FireOnStatementChanged(false)
,m_IsPosted_FlagXmlSchemas(false)
,m_fILEmitTasksSkippedDuringDebugging(false)
,m_fEnableVBReferenceChangedService(false)
,m_fVBReferenceChangedServiceQueried(false)
,m_BoundMethodBodyCache(pCompiler)
#endif IDE 
#if IDE
,m_fNeedtoUpdateENCMetaData(false)
,m_fIsSnippetProject(false)
,m_fContainedLanguageProject(false)
,m_fVenusProject(false)
,m_fVenusAppCodeProject(false)
#endif
,m_Imports(pCompiler, this)
,m_PlatformKind(Platform_Agnostic)
,m_AssemblyIdentity(pCompiler, this)
,m_ModuleAssemblyIdentity(pCompiler, this)
,m_fNoStandardLibraries(true)
,m_pFileWithAssemblyKeyFileAttr(NULL)
,m_fKeyFileFromAttr(false)
,m_pFileWithAssemblyKeyNameAttr(NULL)
,m_fKeyNameFromAttr(false)
,m_pFileWithAssemblyVersionAttr(NULL)
,m_pstrProjectSettingForKeyFileName(NULL)
,m_pstrProjectSettingForKeyContainerName(NULL)
,m_pstrUacManifestFile(NULL)
,m_pstrFileNameNoPath(NULL)
,m_pstrFileNameDuringPrevNoPathReq(NULL)
,m_pstrPEPath(NULL)
,m_nrlsLookupCaches(NORLSLOC)
,m_LookupCache(&m_nrlsLookupCaches)
,m_ImportsCache(&m_nrlsLookupCaches)
,m_ExtensionMethodLookupCache(&m_nrlsLookupCaches)
,m_LiftedOperatorCache(&m_nrlsLookupCaches)
,m_SourceFileCache(pCompiler)
,m_LangVersion(LANGUAGE_CURRENT)
,m_PotentiallyEmbedsPiaTypes(false)
#if IDE 
,m_IDEExtensionMethodCache(pCompiler)
,m_XmlSchemas(this)
,m_isExpressionEditorProject(false)
#endif IDE
,m_bIsHighEntropyVA(false)
{
    DebCheckNoBackgroundThreads(m_pCompiler);
    m_projectId = ++m_lastProjectId;

#if !IDE 
    m_pReferredToEmbeddableInteropType = NULL;
#endif

    m_subsystemVersion.major = 0;
    m_subsystemVersion.minor = 0;

    //
    // Assume that we have no scope.
    //
}

LONG CompilerProject::m_lastProjectId = 0;

//===========================================================================
// Remove all namespaces created by files within the project.
//===========================================================================
void
CompilerProject::RemoveNamespaces()
{
#if IDE 
    for (unsigned int cs = CS_NoState; cs < CS_MAX; cs++)
    {
        CompilerFile * pCompilerFile;
        CDoubleListForwardIter<CompilerFile> iter(&m_dlFiles[cs]);

        while (pCompilerFile = iter.Next())
        {
            pCompilerFile->RemoveNamespaces();
        }
    }
#endif
}


//============================================================================
// CompilerProject Destructor
//============================================================================

CompilerProject::~CompilerProject()
{
    Invalidate();
}

//============================================================================
// Invalidate all resources of CompilerProject
//============================================================================

void CompilerProject::Invalidate()
{
    CheckInMainThread();

    VSASSERT(m_punkProject == NULL, "Disconnect() was not called!");

    HRESULT hr = NOERROR;

#if IDE 
    if (!m_fIsValid)
    {
        return;
    }

    m_fIsValid = false;

    BackgroundCompilerLock compilerLock(this);

    // Make sure the list of projects to compile gets reset.
    // Microsoft: Right now, unit tests don't create a project.
    if ( GetCompilerHost() != NULL )
    {
        GetCompilerHost()->DestroyProjectCompilationList();
        GetCompilerHost()->ClearRuntimeHelperSymbolCaches(this);
    }

    GetCompilerHost()->GetFXSymbolProvider()->RemoveFXTypesForProject(this);

    if( m_pSinkSite )
    {
        m_pSinkSite->Zombie();
        // m_pSinkSite might be null since zombie sends a close
        // event which might end up in the sink site being released
        // and in it's dtor it sets this value equal to NULL.
        // do it again here just to be sure.
        m_pSinkSite = NULL;
    }

    if (m_pCodeModel)
    {
        VSFAIL("CodeModel wan't correctly destroyed");
        m_pCodeModel->Invalidate();
        m_pCodeModel->Release();
        m_pCodeModel = NULL;
    }
    
    if (m_pExternalCodeModel)
    {
        m_pExternalCodeModel->Invalidate();
        m_pExternalCodeModel->Release();
        m_pExternalCodeModel = NULL;
    }

    if (m_pstrPEName)
    {
        GetCompiler()->UnwatchFile(m_pstrPEName);
    }

    // During decompilation, the current project might also be added to the nodes to demote list by the decompilation
    // logic. During non-batch edits, the decompilation is done immediately and the list is cleaned up by this point of
    // time. But in batch edits, the list is persisted so that decompilation can be done all at once later on. So in batch
    // edits, before removing the project and its files, any pending decompilations are committed immediately, so that
    // this project and its files are decompiled correctly and removed from the pending nodes list. Note that this will
    // also commit the pending decompilations for other projects too which is fine because the background compiler is
    // not restarted and they will stay decompiled. Bug VSWhidbey 386301.
    //
    // Note the null check for compiler package. Apparently, when only the VB expression evaluator is loaded, then there
    // is no CompilerPackage, but instead a Compiler is created.
    //
    if (GetCompilerPackage() && GetCompilerPackage()->IsInBatch())
    {
        GetCompilerPackage()->CommitEditListDemotions(true, true, this);
    }

    if (GetCompilerPackage())
    {
        GetCompilerPackage()->ClearSchemasInProject(this);
    }

#endif IDE


    //
    // Unregister with the project manager.
    //

    ReleaseMyCollectionInfo();

    m_slAppliedSecurityAttributes.Clear();


    // Project-level imports do not have to be explicitly freed.
    // They are freed when the destructor for the m_Imports is invoked
    // when the compilerproject object is going away.


    //
    // Release any scopes.
    //

    RELEASE(m_pVBRuntimeProject);
    
    RELEASE(m_pAssemblyImport);
    RELEASE(m_pStreamPDB);

    ::CoTaskMemFree(m_pImage);

    //
    // Release any memory.
    //
#ifdef EXTENDED_WARNING_GRANULARITY
    CleanWarningIdAndStringTable();
#endif
    //
    // Unregister with the project manager.
    //

    m_daMyGroupCollectionInfo.Destroy();

    // NOTE: Bug4421, minke,1/27/99, (?) Need to delete all files in lists of m_pfileList and
    // m_dlFiles[0] to m_dlFiles[3]
    // Microsoft: We need to keep track of files unlinked so that we can remove them
    // from the edit list.
#if IDE 
    DynamicArray< CComPtr<CompilerFile> > linkedFiles;
#endif

    for (unsigned int cs = CS_NoState; cs < CS_MAX; cs++)
    {
        CompilerFile * pCompilerFile;
        CDoubleListForwardIter<CompilerFile> iter(&m_dlFiles[cs]);

        while (pCompilerFile = iter.Next())
        {
            // Release our reference to the file.
#if IDE 
            if (pCompilerFile->IsSourceFile())
            {

                linkedFiles.AddElement( pCompilerFile );
                // Unlink the file from the project and release it's reference.
                pCompilerFile->PSourceFile()->ReleaseFileFromProject(true /* SkipDemoteNodesCheck */);
            }
            else
#endif IDE
            {
                delete pCompilerFile;
            }
        }
    }

#if IDE 
    // This is needed to handle circular implicit references besides other scenarios.
    for(ULONG Index = 0; Index < m_daImplicitReferencesOnMe.Count(); Index++)
    {
        // There should only be one project per assembly ref usually.
        AssemblyRefReferencedProjectsIterator Iter(m_daImplicitReferencesOnMe.Element(Index));
        while (CompilerProject *pReferencedProject = Iter.Next())
        {
            if (pReferencedProject == this)
            {
                Iter.SetCurrentProject(NULL);
                break;
            }
        }
    }
#endif IDE

    // ReleaseFileFromProject() above will cause DependenyManager to walk the list of files in the CompilerHost,
    // which will invoke an iteration over all projects in the host, so we can't call RemoveProject until we are
    // done iterating over the projects.
    GetCompilerHost()->RemoveProject(this);

#if IDE 
    // Unwatch and release any resource files this project has associated with it
    UnwatchAllResourceFiles();
    m_daResources.Reset();

    // Remove the old Tasklist items
    if (m_pTaskProvider)
    {
        m_pTaskProvider->UnregisterTaskProvider();
        RELEASE(m_pTaskProvider);
    }

    // By the time the destructor is called for metadata projects, reference list should be empty
#if DEBUG
    if (IsMetaData())
    {
        VSASSERT(!m_daReferences.Count(), "Metadata project still has references while destroying!");
    }
#endif DEBUG

#endif IDE

    // Close the source file cache
    m_SourceFileCache.Close();

#if IDE 
    m_daImplicitReferencesOnMe.Collapse();

    ReleaseAllReferences(&m_daReferences);

    if (m_pENCBuilder)
    {
        delete m_pENCBuilder;
        m_pENCBuilder = NULL;
    }

    // Microsoft: Unwatch files above may cause this project to be added back
    // to the demote list. This is bad because then the destroyed project
    // will be in the list. So here, we must demote again.

    if (GetCompilerPackage())
    {
        GetCompilerSharedState()->GetCommitEditList()->RemoveProjectFromList( this );

        // Microsoft:
        // We also need to remove any files in this project that were added
        // back to the demote list.

        for( UINT i = 0; i < linkedFiles.Count(); i += 1 )
        {
            GetCompilerSharedState()->GetCommitEditList()->RemoveFileFromList( linkedFiles.Element( i ) );
            linkedFiles.Element( i ) = NULL;
        }

        // Microsoft:
        // Verify that we are no longer on the edit list. We should never be in a situation
        // where the destroyed project is in the commit list.
        GetCompilerSharedState()->GetCommitEditList()->VerifyProjectNotInList(this);
    }

    if (m_srpVBReferenceChangedService)
    {
        m_srpVBReferenceChangedService.Release();
    }

#endif IDE

    // Clear the friends list.
    ClearFriends();


}

void CompilerProject::ReleaseMyCollectionInfo()
{
    MyGroupCollectionInfo* arrayOfMyGroupCollectionInfo = m_daMyGroupCollectionInfo.Array();
    ULONG count = m_daMyGroupCollectionInfo.Count();
    for (unsigned i = 0 ; i < count; i++)
    {
        if (arrayOfMyGroupCollectionInfo[i].m_groupClass)
        {
            arrayOfMyGroupCollectionInfo[i].m_groupClass->GetPWellKnownAttrVals()->ResetMyGroupCollectionData();
        }
    }

    m_daMyGroupCollectionInfo.Destroy();
}
//============================================================================
// Populates the project with the information in the build engine
//============================================================================

HRESULT CompilerProject::InitProject
(
    _In_z_ STRING *pstrFileName,
    IUnknown *punkProject,
    IVsHierarchy *pHierarchy,
    CompilerHost *pCompilerHost
)
{
    CheckInMainThread();
    DebCheckNoBackgroundThreads(m_pCompiler);

    // Weak pointer. Do not AddRef.
    m_punkProject = punkProject;

    //
    // Set the project's name.
    //
    m_pstrFileName = pstrFileName;

#if IDE 

    //
    // Stash the hierarchy
    //

    if (pHierarchy)
    {
        m_pHierarchy = pHierarchy;
        m_pHierarchy->AddRef();

        CComPtr<Project> spProject;
        CReferenceUtils::GetProjectExtensibilityObject(m_pHierarchy, &spProject);

        CComBSTR bstrKind;
        if (SUCCEEDED(spProject->get_Kind(&bstrKind)))
        {
            // Covert prjKindVenusProject (char *) to a BSTR to do the comparision
            m_fVenusProject = (bstrKind == CComBSTR(prjKindVenusProject));
        }

        SetTargetFrameworkMoniker(pHierarchy);
    }
#endif

    m_pCompilerHost = pCompilerHost;

    // Make sure the project gets included in the list of projects to compile.
    GetCompilerHost()->DestroyProjectCompilationList();

    //
    // Add the default references.
    //
    AddRequiredLibraries();

#if IDE  
    
    // Hostable Editor check
    if (!m_isExpressionEditorProject && GetCompilerPackage()->GetSite())
    { 
        CComObject<CCodeModel> *pCodeModel = NULL;
        if (SUCCEEDED(CComObject<CCodeModel>::CreateInstance(&pCodeModel)) &&
            pCodeModel)
        {
            pCodeModel->AddRef();

            if (FAILED(pCodeModel->Init(this)))
            {
                pCodeModel->Invalidate();
                pCodeModel->Release();
                pCodeModel = NULL;
            }

            m_pCodeModel = pCodeModel;
        }

        // Init the task provider
        IfFailThrow(CompilerTaskProvider::CreateInstance(this, &m_pTaskProvider));
        IfFailThrow(m_pTaskProvider->RegisterTaskProvider());
    }
#endif IDE

    return S_OK; // any failures are going to throw.
}

//============================================================================
// Private helper for CompilerProject::GetAssemblyImport. Creates the
// assembly importer if not already created.
//============================================================================

void CompilerProject::CreateAssemblyImport
(
)
{
    HRESULT  hr = S_OK;

    CComPtr<IMetaDataAssemblyImport> spAssemblyImport;
    CComPtr<IMetaDataDispenserEx>    spDataDispEx;

    //
    // Get an IMetaDataAssemblyImport on the metadata scope
    //
    hr = GetCompilerHost()->OpenScope(&spDataDispEx, m_pstrFileName, ofReadOnly, IID_IMetaDataAssemblyImport, (IUnknown **)&spAssemblyImport);

    if ( FAILED(hr) )
    {
        // We're trying to import a bogus file -- it doesn't contain any metadata!
        // Report the error and eat it so we can continue compilation.
        m_ErrorTable.CreateErrorWithError(ERRID_BadRefLib1, NULL, hr, m_pstrFileName);
        return;
    }

    spAssemblyImport.CopyTo(&m_pAssemblyImport);

}

//---------------------------------------------------------------------------
// Given a directory or filename, add a directory or filename to the path.
// This is like WszCat, only it makes sure that a '\\' separator is added
// if necessary.
//---------------------------------------------------------------------------
STRING* PathCatName(Compiler *pCompiler, _In_z_ WCHAR* wszPath, _In_z_ const WCHAR* wszName)
{
    StringBuffer FileName;
    // Find end of path
    WCHAR* wszEnd = wszPath + wcslen(wszPath);
    FileName.AppendString(wszPath);

    // Add separator if necessary
    if (wszEnd > wszPath && wszEnd[-1] != WIDE('\\'))
    {
        FileName.AppendChar('\\');
    }

    FileName.AppendString(wszName);

    return pCompiler->AddString(&FileName);
}

//============================================================================
// Private helper for CompilerProject::InitWithMetaData.  If the given file
// contains an assembly manifest that has metadata-containing FileRefs,
// create a new MetaDataFile for each FileRef and add to this CompilerProject.
//============================================================================

HRESULT CompilerProject::AddAssemblyFileRefs
(
    _In_z_ STRING *pstrFileName,
    bool    bAssembly
)
{
    HRESULT  hr = NOERROR;
    CComPtr<IMetaDataImport> srpMetaDataImport;
    bool fHasAssemblyManifest;
    HCORENUM enumFile = 0;
    DWORD    cFiles   = 0;
    size_t nRequiredBuffer = StringPool::StringLength(pstrFileName)+1;
    TEMPBUF(wszPath, WCHAR *, nRequiredBuffer * sizeof(WCHAR));
    IfNullGo(wszPath);

    mdAssembly mdassembly = mdAssemblyNil;

    //
    // Save directory of the file we're importing from
    //
    wcscpy_s(wszPath, nRequiredBuffer, pstrFileName);
    PathMakeDir(wszPath);
    
    //
    // Get COM+ IMetaDataDispenserEx.
    //
    if ( !m_pAssemblyImport )
    {
        CreateAssemblyImport();
    }

    // This may fail silently, in which case just skip.
    if (!m_pAssemblyImport)
    {
        goto Error;
    }

    // See if we really have a good IMetaDataAssemblyImport.  If
    // this fails, we're importing from a code module.
    hr = m_pAssemblyImport->GetAssemblyFromScope(&mdassembly);
    fHasAssemblyManifest = SUCCEEDED(hr);

    // Remember if we actually have a code module instead of an assembly.
    m_fCodeModule = !fHasAssemblyManifest;

    // Emit errors if the metadata type doesn't match the requested type
    if (bAssembly && !fHasAssemblyManifest)
    {
        m_ErrorTable.CreateError(ERRID_MetaDataIsNotAssembly, NULL, m_pstrFileName);
        RELEASE(m_pAssemblyImport);
        goto Error; // The error has been reported; fail silently.
    }
    else if (!bAssembly && fHasAssemblyManifest)
    {
        m_ErrorTable.CreateError(ERRID_MetaDataIsNotModule, NULL, m_pstrFileName);
        RELEASE(m_pAssemblyImport);
        goto Error; // The error has been reported; fail silently.
    }

    // Store the assembly token for this assembly on its project
    m_mdassembly = mdassembly;

    //
    // Enum the assembly's referenced files and add them to the CompilerProject.
    //
    do {
        mdFile      rgmdfile[16];

        IfFailGo(m_pAssemblyImport->EnumFiles(&enumFile, rgmdfile, _countof(rgmdfile), &cFiles));

        for (DWORD iFile = 0; iFile < cFiles; iFile++)
        {
            MetaDataFile *pmetadatafile;
            DWORD         dwFlags;
            DWORD         cchName;
            STRING       *pstrNameFull;

            // Just get the filename and flags
#pragma prefast(suppress: 26017, "cFiles is bounded by _countof(rgmdfile)")
            IfFailGo(m_pAssemblyImport->GetFileProps(rgmdfile[iFile], NULL, 0, &cchName, NULL, NULL, &dwFlags));

            IfFalseGo(cchName + 1 >= cchName, E_OUTOFMEMORY);
            IfFalseGo(VBMath::TryMultiply((cchName + 1), sizeof(WCHAR)), E_OUTOFMEMORY);

            TEMPBUF(wszNameRelative, WCHAR *, (cchName+1) * sizeof(WCHAR));
            IfNullGo(wszNameRelative);

            // Just get the filename and flags
#pragma prefast(suppress: 26017, "cFiles is bounded by _countof(rgmdfile)")
            IfFailGo(m_pAssemblyImport->GetFileProps(rgmdfile[iFile], wszNameRelative, cchName, &cchName, NULL, NULL, &dwFlags));

            // Don't bother importing resource-only files
            if (dwFlags & ffContainsNoMetaData)
            {
                continue;
            }

            // Make assembly-relative path into fully qualified path
            pstrNameFull = PathCatName(m_pCompiler, wszPath, wszNameRelative);

            // Create MetaDataFile and add to CompilerProject
            IfFailGo(MetaDataFile::Create(this, pstrNameFull, &pmetadatafile));
            pmetadatafile->m_cs = CS_NoState;

            // Note that InsertLast is important here because there is code that assumes that the first file
            // is the primary file.
            //
            m_dlFiles[CS_NoState].InsertLast(pmetadatafile);
        }
    } while (cFiles > 0);

    m_pAssemblyImport->CloseEnum(enumFile);

    // Save some information about the assembly that we can use to find
    // it later.
    //
    if (fHasAssemblyManifest)
    {
        NorlsAllocator nra(NORLSLOC);
        ULONG cchName;
        WCHAR *wszName;
        ASSEMBLYMETADATA amd;
        ULONG cbOriginator = 0;
        const void *pvOriginator = NULL;
        DWORD flags = 0;

        memset(&amd, 0, sizeof(amd));

        IfFailThrow(m_pAssemblyImport->GetAssemblyProps(mdassembly,   // [IN] The Assembly for which to get the properties.
                                                         NULL,        // [OUT] Pointer to the Originator blob.
                                                         NULL,        // [OUT] Count of bytes in the Originator Blob.
                                                         NULL,        // [OUT] Hash Algorithm.
                                                         NULL,        // [OUT] Buffer to fill with name.
                                                         0,           // [IN] Size of buffer in wide chars.
                                                         &cchName,    // [OUT] Actual # of wide chars in name.
                                                         &amd,        // [OUT] Assembly MetaData.
                                                         NULL));      // [OUT] Flags.

        wszName = (WCHAR *)nra.Alloc(VBMath::Multiply(cchName, sizeof(WCHAR)));

        if (amd.cbLocale > 0)
        {
            amd.szLocale = (WCHAR *)nra.Alloc(VBMath::Multiply(amd.cbLocale, sizeof(WCHAR)));
        }

        IfFailThrow(m_pAssemblyImport->GetAssemblyProps(mdassembly,        // [IN] The Assembly for which to get the properties.
                                                         &pvOriginator,    // [OUT] Pointer to the Originator blob.
                                                         &cbOriginator,    // [OUT] Count of bytes in the Originator Blob.
                                                         NULL,        // [OUT] Hash Algorithm.
                                                         wszName,     // [OUT] Buffer to fill with name.
                                                         cchName,     // [IN] Size of buffer in wide chars.
                                                         NULL,        // [OUT] Actual # of wide chars in name.
                                                         &amd,        // [OUT] Assembly MetaData.
                                                         &flags));    // [OUT] Flags.

        VSASSERT(cbOriginator == 0 || pvOriginator != NULL, "Bad state.");

        m_AssemblyIdentity.SetAssemblyIdentity(m_pCompiler->AddString(wszName),
                                               m_pCompiler->AddString(amd.szLocale),
                                               (BYTE *)pvOriginator,
                                               cbOriginator,
                                               amd.usMajorVersion,
                                               amd.usMinorVersion,
                                               amd.usBuildNumber,
                                               amd.usRevisionNumber,
                                               flags);
        
        // Get the MVID
        IfFailThrow(m_pAssemblyImport->QueryInterface(IID_IMetaDataImport, (void **)&srpMetaDataImport));
        IfFailThrow(srpMetaDataImport->GetScopeProps(NULL, 0, NULL, m_AssemblyIdentity.GetMVID()));

        // Is this a wrapper for a COM typelib ?
        // We will use this information when deciding whether to allocate extra location data for imported
        // type members, since the No-PIA system needs ordering information when emitting local types.
        HRESULT hrTypeLibAttr =
            srpMetaDataImport->GetCustomAttributeByName(mdassembly,
                                                        INTEROP_IMPORTEDFROMTYPELIB_TYPE_W,
                                                        NULL,
                                                        NULL);

        if (hrTypeLibAttr == S_OK)
        {
            m_AssemblyIdentity.SetImportedFromCOMTypeLib();
        }
        else
        {
            IfFailThrow(hrTypeLibAttr);
        }
        
        // The other identifier for a potential source of embeddable types is the PrimaryInteropAssembly
        // attribute, so we will check for that flag too.
        HRESULT hrPIAAttr =
            srpMetaDataImport->GetCustomAttributeByName(mdassembly,
                                                        PRIMARYINTEROPASSEMBLYATTRIBUTE,
                                                        NULL,
                                                        NULL);

        if (hrPIAAttr == S_OK)
        {
            m_AssemblyIdentity.SetPrimaryInteropAssembly();
        }
        else
        {
            IfFailThrow(hrPIAAttr);
        }
    }

    // If we get here, we are OK.
    hr = S_OK;

Error:

    return hr;
}


//============================================================================
// Creates MetaData files for the assembly as well as for each module the
// assembly is built with (if any).
//============================================================================

HRESULT CompilerProject::InitMetaDataFiles
(
    _In_z_ STRING *pstrFileName,
    bool    bAssembly
)
{
    CheckInMainThread();
    DebCheckNoBackgroundThreads(m_pCompiler);

    HRESULT hr = NOERROR;

    MetaDataFile *pfile = NULL;

    // Create CompilerFile
    IfFailGo(MetaDataFile::Create(this, pstrFileName, &pfile));

    pfile->m_cs = CS_NoState;
    m_dlFiles[CS_NoState].InsertLast(pfile);

    //
    // If this is an assembly, add any FileRefs that need to be imported
    //
    IfFailGo(AddAssemblyFileRefs(pstrFileName, bAssembly));

Error:
    return hr;
}

bool IsManifestModule(IMetaDataImport *pImport)
{
    CComPtr<IMetaDataAssemblyImport> spAssembly;

    if ( SUCCEEDED(ComUtil::QI(pImport , IID_IMetaDataAssemblyImport, &spAssembly)) )
    {
        mdAssembly token;
        return SUCCEEDED(spAssembly->GetAssemblyFromScope(&token));
    }

    return false;
}

HRESULT CompilerProject::InitMetaDataFiles
(
    _In_z_ STRING *pstrFileName,
    _In_ IMetaDataImport *pImport
)
{
    CheckInMainThread();
    DebCheckNoBackgroundThreads(m_pCompiler);

    HRESULT hr = NOERROR;
    VBHeapPtr<MetaDataFile> spFile;
    CComPtr<IMetaDataAssemblyImport> spAssemblyImport;

    IfFailGo( MetaDataFile::Create(this, pstrFileName, pImport, &spFile) ); 
    VSASSERT(GetPrimaryModule(), "PrimaryModule should be set by this point");

    spFile->m_cs = CS_NoState;
    m_dlFiles[CS_NoState].InsertLast(spFile.Detach());

    // The typical compiler scenario is to load the IMetaDataAssemblyImport interface
    // based on the full path of the assembly on disk.  This logic breaks down in certain
    // scenarios such as the EE where the assembly may be located on a different 
    // computer altogether.  
    //
    // In this case though we already have the IMetaDataImport so just QI for the 
    // IMetaDataAssemblyImport interface
    if ( SUCCEEDED(ComUtil::QI(pImport, IID_IMetaDataAssemblyImport, (void**)&spAssemblyImport)) )
    {
        spAssemblyImport.CopyTo(&m_pAssemblyImport);
    }

    //
    // If this is an assembly, add any FileRefs that need to be imported
    //
    IfFailGo( AddAssemblyFileRefs(pstrFileName, IsManifestModule(pImport)) );
    
Error:
    return hr;
}

//============================================================================
// Populates a project with the classes defined in a metadata file.
//============================================================================

void CompilerProject::InitWithMetaData
(
    _In_z_ STRING *pstrFileName,
    bool   bAssembly,
    CompilerHost *pCompilerHost,
    bool    bDefaultLibrary
)
{
    CheckInMainThread();
    DebCheckNoBackgroundThreads(m_pCompiler);

    HRESULT hr = NOERROR;

    ITypeLib *ptlib = NULL;
    BSTR bstrName = NULL;

#if IDE 
    BackgroundCompilerLock compilerLock(this);
#endif IDE

    //
    // Set the project's name.
    //
    m_pstrFileName  = pstrFileName;
    m_pCompilerHost = pCompilerHost;
    IfFailGo(InitMetaDataFiles(pstrFileName, bAssembly));

    // Make sure the project gets included in the list of projects to compile.
    GetCompilerHost()->DestroyProjectCompilationList();

    // Set the warning level to normal.
    m_WarningLevel = WARN_Regular;

    // Set up the projects.
    SetCompState(CS_NoState);
#if IDE 
    SetDecompilationState(CS_NoState);
#endif

    // This is a metadata project.
    m_fMetaData = true;

    // Metadata projects are always ready to compile.
    m_fInitialized = true;

#if IDE
    // Create the RootCodeNamespace Object
    CComObject<CExternalCodeModel> *pCodeModel = NULL;
    if (SUCCEEDED(CComObject<CExternalCodeModel>::CreateInstance(&pCodeModel)) && pCodeModel)
    {
        pCodeModel->AddRef();

        if (FAILED(pCodeModel->Init(this)))
        {
            pCodeModel->Invalidate();
            pCodeModel->Release();
            pCodeModel = NULL;
        }

        m_pExternalCodeModel = pCodeModel;
    }

    if (!bDefaultLibrary && !m_isExpressionEditorProject && GetCompilerPackage()->GetSite())
    {
        // Init the task provider
        IfFailThrow(CompilerTaskProvider::CreateInstance(this, &m_pTaskProvider));
        IfFailThrow(m_pTaskProvider->RegisterTaskProvider());

        // Some errors for metadata files and project (eg: file not found, etc) are
        // placed in the error table before the task list provider is setup. So
        // once the task list provider is setup, we need to check if any such errors
        // exist and indicate that the task list needs to be refreshed.
        //
        // Another approach would be to create the task list provider early, but it
        // maybe riskier because lots of stuff in the project is uninitialized at
        // that point of time.

        MetaDataFileIterator Iter(this);
        while (CompilerFile *pFile = Iter.Next())
        {
            if (pFile->GetErrorTable()->HasErrorsAtStep(CS_NoStep))
            {
                m_pTaskProvider->TaskListNeedsRefresh();
                goto TaskListRefereshDetected;
            }
        }

        if (GetErrorTable()->HasErrorsAtStep(CS_NoStep))
        {
            m_pTaskProvider->TaskListNeedsRefresh();
        }

TaskListRefereshDetected:;
    }
#endif IDE

Error:
    ;

    SysFreeString(bstrName);
    RELEASE(ptlib);
}

//============================================================================
//
// Initializes the CompilerProject with the specified IMetaDataImport
//
//============================================================================
HRESULT
CompilerProject::InitWithMetaData (
    _In_z_ STRING *pstrFileName,
    _In_ CompilerHost *pCompilerHost,
    _In_ IMetaDataImport *pImport)
{
    CheckInMainThread();
    DebCheckNoBackgroundThreads(m_pCompiler);

    HRESULT hr = NOERROR;

#if IDE 
    BackgroundCompilerLock compilerLock(this);
#endif IDE

    //
    // Set the project's name.
    //
    m_pstrFileName  = pstrFileName;
    m_pCompilerHost = pCompilerHost;
    IfFailGo(InitMetaDataFiles(pstrFileName, pImport));

    // Make sure the project gets included in the list of projects to compile.
    GetCompilerHost()->DestroyProjectCompilationList();

    // Set the warning level to normal.
    m_WarningLevel = WARN_Regular;

    // Set up the projects.
    SetCompState(CS_NoState);
#if IDE 
    SetDecompilationState(CS_NoState);
#endif

    // This is a metadata project.
    m_fMetaData = true;

    // Metadata projects are always ready to compile.
    m_fInitialized = true;

#if IDE 
    // Create the RootCodeNamespace Object
    CComObject<CExternalCodeModel> *pCodeModel = NULL;
    if (SUCCEEDED(CComObject<CExternalCodeModel>::CreateInstance(&pCodeModel)) && pCodeModel)
    {
        pCodeModel->AddRef();

        if (FAILED(pCodeModel->Init(this)))
        {
            pCodeModel->Invalidate();
            pCodeModel->Release();
            pCodeModel = NULL;
        }

        m_pExternalCodeModel = pCodeModel;
    }
#endif IDE

Error:
    return hr;
}

//============================================================================
// Binds the assemblyrefs in all the files of a metadata project.
//============================================================================

void CompilerProject::BindAssemblyRefs(ActionKindEnum action)
{
    if (!this->IsMetaData())
    {
        return;
    }

    MetaDataFileIterator Files(this);

    while (MetaDataFile *pFile = Files.Next())
    {
        pFile->BindAssemblyRefs(action);
    }
}

//****************************************************************************
// IUnknown implementation
//****************************************************************************

STDMETHODIMP CompilerProject::QueryInterface
(
    REFIID riid,
    void **ppv
)
{
    void *pv = NULL;

    if (riid == IID_IUnknown)
    {
        // Microsoft 2/4/03:  Do what ATL would do in this case:  VSWhidbey 56848
        pv = static_cast<IUnknown*>(static_cast<IVbCompilerProject *>(this));
    }
    else if (riid == IID_IVbCompilerProject)
    {
        pv = static_cast<IVbCompilerProject *>(this);
    }
    else if (riid == IID_IVbVbaCompiler)
    {
        pv = static_cast<IVbVbaCompiler *>(this);
    }
    else if (riid == IID_IVsENCRebuildableProjectCfg2)
    {
        pv = static_cast<IVsENCRebuildableProjectCfg2 *>(this);
    }
#if IDE
    else if (riid == IID_IVsLanguageServiceBuildErrorReporter)
    {
        pv = static_cast<IVsLanguageServiceBuildErrorReporter *>(this);
    }
    else if (riid == IID_IVsLanguageServiceBuildErrorReporter2)
    {
        pv = static_cast<IVsLanguageServiceBuildErrorReporter2 *>(this);
    }
#endif IDE
    else if (riid == IID_CompilerProject)
    {
        // 






        pv = this;
    }
    else
    {
        return E_NOINTERFACE;
    }

    ((IUnknown *)pv)->AddRef();

    *ppv = pv;
    return NOERROR;
}


STDMETHODIMP_(ULONG) CompilerProject::AddRef()
{
    return InterlockedIncrement((long *)&m_cRefs);
}

STDMETHODIMP_(ULONG) CompilerProject::Release()
{
    ULONG cRefs;
    
#if IDE
    bool invalidate = false;
    {
        CompilerIdeLock spLock(GetMessageCriticalSection());
        cRefs = InterlockedDecrement((long *)&m_cRefs);
        invalidate = (cRefs == m_cPostedMessageCount);
    }
    if (invalidate)
    {
        Invalidate();
    }
#else
    cRefs = InterlockedDecrement((long *)&m_cRefs);
#endif IDE

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


//****************************************************************************
// SetVBRuntimeOptions 
//****************************************************************************
HRESULT CompilerProject::SetVBRuntimeOptions
(
    VbCompilerOptions *pCompilerOptions
)
{

    HRESULT hr = S_OK;
    VBRUNTIME_KIND newVbRuntimeKind = g_CompilingTheVBRuntime ? NoRuntime : pCompilerOptions->vbRuntimeKind;        

    // 
    if ((m_vbRuntimeKind == newVbRuntimeKind) && 
        (m_vbRuntimeKind != DefaultRuntime || (m_pVBRuntimeProject && m_pVBRuntimeProject == GetCompilerHost()->GetDefaultVBRuntimeProject())) &&
        (m_vbRuntimeKind != SpecifiedRuntime || 
            (m_pVBRuntimeProject && (0 == StringPool::Compare(m_pVBRuntimeProject->GetFileName(), GetCompiler()->AddString(pCompilerOptions->wszSpecifiedVBRuntime)))))
       )
    {
        return hr;
    }
    else
    {
        m_vbRuntimeKind = newVbRuntimeKind;
        
        if (m_pVBRuntimeProject != NULL)
        {
            RemoveMetaDataReferenceInternal(m_pVBRuntimeProject->GetFileName());
            RELEASE(m_pVBRuntimeProject);
        }

        switch (m_vbRuntimeKind)
        {
            case NoRuntime:         // corresponds to /vbruntime-               
                RemoveSolutionExtension(SolutionExtension::VBCoreExtension);
                break;
            
            case DefaultRuntime:    // corresponds to /vbruntime+
                IfFailRet(m_pCompilerHost->LoadDefaultVBRuntimeLibrary(this));           
                AddReference(m_pVBRuntimeProject, REFERENCEFLAG_REQUIRED);
                RemoveSolutionExtension(SolutionExtension::VBCoreExtension);
                            
                break;

            case SpecifiedRuntime:  // corresponds to /vbruntime:<path>
                IfFailRet(m_pCompilerHost->LoadSpecifiedVBRuntimeLibrary(
                    this, 
                    pCompilerOptions->wszSpecifiedVBRuntime));
                AddReference(m_pVBRuntimeProject, REFERENCEFLAG_REQUIRED);
                RemoveSolutionExtension(SolutionExtension::VBCoreExtension);
                break;

            case EmbeddedRuntime:   // corresponds to /vbruntime*
                AddSolutionExtension(SolutionExtension::VBCoreExtension);            
                break;
        }
    }
    
    return hr;
}

//============================================================================
// Set or reset the compiler options.
//============================================================================
STDMETHODIMP CompilerProject::SetCompilerOptions
(
    VbCompilerOptions *pCompilerOptions
)
{
    // Note: Never call this method from VB code directly - call the Internal version.
    VB_VERIFY_NOT_REENTRANT();
    return SetCompilerOptionsInternal(pCompilerOptions);
}

STDMETHODIMP CompilerProject::SetCompilerOptionsInternal
(
    VbCompilerOptions *pCompilerOptions
)
{
    CheckInMainThread();
    VB_ENTRY();

#if IDE 
    bool IsNeedToUpdateHiddenRegions = false;
#endif IDE

    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();

    // The following block keeps compilerLock in scope.
#if IDE 
    {
        BackgroundCompilerLock compilerLock(this);
#endif
        STRING *pstrStartupType;
        STRING *pstrCondComp;
        STRING *pstrPEName;
        STRING *pstrXMLDocFileName = NULL;
        STRING *pstrStrongNameKeyFile;
        STRING *pstrDefaultNamespace;
        STRING *pstrStrongNameContainer;

#if IDE 
        // Some option changes require a decompilation.  Remember what state
        // we want to decompile to (CS_MAX means no decompilation required).
        // 


        CompilationState compstateDecompile = CS_MAX;
        bool fNeedToDemoteAllNonFXMetadataProjects = false;

        FireOnChange(FALSE, PRJ_COMPILEROPTIONS);
#endif IDE

        //========================================================================
        // We can now safely compile this project.
        //========================================================================

        if (!m_fInitialized)
        {
            m_fInitialized = true;

            // Make sure the list of projects to compile gets reset if the value of m_fInitialized is changed.
            m_pCompilerHost->DestroyProjectCompilationList();

#if IDE 
            // Need to demote all Non-FX metadata projects in case any of them has an assemblyref corresponding
            // to the identity of this project.
            fNeedToDemoteAllNonFXMetadataProjects = true;
#endif IDE
        }

#if IDE 
        //========================================================================
        // Detect whether this project is a venus app code project
        //========================================================================

        // 


        if (IsVenusProject() &&
            !m_fVenusAppCodeProject &&
            pCompilerOptions->bEnableIncrementalCompilation)
        {
            m_fVenusAppCodeProject = true;

            // Make sure the list of projects to compile gets reset if the value of m_fInitialized is changed.
            m_pCompilerHost->DestroyProjectCompilationList();

            // Need to demote all Non-FX metadata projects in case any of them has an assemblyref corresponding
            // to the identity of this project.
            fNeedToDemoteAllNonFXMetadataProjects = true;
        }

        // Note that also an assumption here is that once a project has been marked as App_code, it will always
        // be treated as App_Code, even setting bEnableIncrementalCompilation to false will no longer matter.
        // This is to avoid more code churn and risk.
#endif IDE

        //========================================================================
        // Set the output type
        //========================================================================

#if IDE 
        if (pCompilerOptions->OutputType != m_OutputType)
        {
            // This state needs to be < CS_Bound so that we can refresh the task list
            // RAID #117846
            compstateDecompile = min(compstateDecompile, CS_Declared);
        }
#endif IDE

        m_OutputType = pCompilerOptions->OutputType;

        // Remember if we're building a code module instead of an assembly
        m_fCodeModule = (pCompilerOptions->OutputType == OUTPUT_Module);

        //========================================================================
        // Init startup type
        //========================================================================

        pstrStartupType = m_pCompiler->AddString(pCompilerOptions->wszStartup);

#if IDE 
        if (!StringPool::IsEqual(pstrStartupType, m_pstrStartupType))
        {
            // We need to go all of the way down because the current startup
            // symbol is marked as such and this is the easiest way to clear
            // the flag.
            //
            compstateDecompile = CS_NoState;
        }
#endif IDE

        m_pstrStartupType = pstrStartupType;

        //========================================================================
        // Default Namespace
        //========================================================================

        pstrDefaultNamespace = m_pCompiler->AddString(pCompilerOptions->wszDefaultNamespace);

#if IDE 
        if (!StringPool::IsEqual(pstrDefaultNamespace, m_pstrDefaultNamespace))
        {
            compstateDecompile = CS_NoState;
        }
#endif IDE

        m_pstrDefaultNamespace = pstrDefaultNamespace;


        //========================================================================
        // Conditional compilation constants.
        //========================================================================

        pstrCondComp = m_pCompiler->AddString(pCompilerOptions->wszCondComp);

#if IDE 
        if (!StringPool::IsEqual(pstrCondComp, m_pstrConditionalCompilationConstants))
        {
            compstateDecompile = CS_NoState;

            // Discard the conditional compilation scope to force creation of a new one.
            SetProjectLevelCondCompSymbols(NULL);

            // We need to clear any parse tree caches since this could change parse trees
            ClearParseTreeCaches();
        }
#endif IDE

        m_pstrConditionalCompilationConstants = pstrCondComp;

        //========================================================================
        // Options
        //========================================================================

#if IDE 
        if ((bool)pCompilerOptions->bOptionCompareText != m_fOptionCompareText)
        {
            compstateDecompile = CS_NoState;
        }
#endif IDE

        m_fOptionCompareText = pCompilerOptions->bOptionCompareText;

#if IDE 
        if ((bool)pCompilerOptions->bOptionExplicitOff != m_fOptionExplicitOff)
        {
            compstateDecompile = CS_NoState;
        }
#endif IDE

        m_fOptionExplicitOff = pCompilerOptions->bOptionExplicitOff;

#if IDE 
        if ((bool)pCompilerOptions->bOptionStrictOff != m_fOptionStrictOff)
        {
            compstateDecompile = CS_NoState;
        }
#endif IDE

        m_fOptionStrictOff = pCompilerOptions->bOptionStrictOff;

#if IDE 
        if ((bool)pCompilerOptions->bOptionInferOff != m_fOptionInferOff)
        {
            compstateDecompile = CS_NoState;
        }
#endif IDE

        m_fOptionInferOff = pCompilerOptions->bOptionInferOff;

        //========================================================================
        // Int checks -- this only requires going down to TypesEmitted because
        // we just need to recompile the IL
        //========================================================================

#if IDE 
        if ((bool)pCompilerOptions->bRemoveIntChecks != m_fRemoveIntChecks)
        {
            compstateDecompile = min(compstateDecompile, CS_TypesEmitted);
        }
#endif IDE

        m_fRemoveIntChecks = pCompilerOptions->bRemoveIntChecks;

        //========================================================================
        // Add the PE path.
        //========================================================================

        // VBA in-memory case - do not set anything here
        if (pCompilerOptions->wszOutputPath == NULL &&
            pCompilerOptions->wszTemporaryPath == NULL)
        {
            pstrPEName = m_pCompiler->AddString(pCompilerOptions->wszExeName);

            if (!StringPool::IsEqual(pstrPEName, m_pstrPEName))
            {
                // Make sure the list of projects to compile gets reset because changing the assembly name
                // of a project can change the project dependencies.
                m_pCompilerHost->DestroyProjectCompilationList();

#if IDE 
                // Non-FX metadata projects can reference source projects. Change in PE name can cause a
                // change in AssemblyRef binding, so need to decompile these metadata projects.
                fNeedToDemoteAllNonFXMetadataProjects = true;
#endif IDE
            }

            m_fInMemoryCompilation = true;

            // If pCompilerOptions->wszXMLDocName is NULL or empty string, then the feature is OFF.
            if (pCompilerOptions->wszXMLDocName && CompareCase(pCompilerOptions->wszXMLDocName, L"") != 0)
            {
                pstrXMLDocFileName = m_pCompiler->AddString(pCompilerOptions->wszXMLDocName);
            }
        }
        else        // normal VB code path
        {
            VSASSERT(pCompilerOptions->wszOutputPath[wcslen(pCompilerOptions->wszOutputPath)-1] == L'\\', "Directory path must have trailing \\");
            pstrPEName = m_pCompiler->ConcatStrings(pCompilerOptions->wszOutputPath, pCompilerOptions->wszExeName);

            // If pCompilerOptions->wszXMLDocName is NULL or empty string, then the feature is OFF.
            if (pCompilerOptions->wszXMLDocName && CompareCase(pCompilerOptions->wszXMLDocName, L"") != 0)
            {
                pstrXMLDocFileName = m_pCompiler->AddString(pCompilerOptions->wszXMLDocName);
            }

            m_fInMemoryCompilation = false;
#if IDE 
            if (!StringPool::IsEqual(pstrXMLDocFileName, m_pstrXMLDocCommentFileName))
            {
                compstateDecompile = CS_NoState;
                IsNeedToUpdateHiddenRegions = true;
            }

            if (!StringPool::IsEqual(pstrPEName, m_pstrPEName))
            {
                compstateDecompile = min(compstateDecompile, CS_TypesEmitted);

                // Make sure the list of projects to compile gets reset because changing the assembly name
                // of a project can change the project dependencies.
                m_pCompilerHost->DestroyProjectCompilationList();

                // Non-FX metadata projects can reference source projects. Change in PE name can cause a
                // change in AssemblyRef binding, so need to decompile these metadata projects.
                fNeedToDemoteAllNonFXMetadataProjects = true;
            }
#endif IDE
        }

#if IDE 
        // If we were watching a different file then unwatch it, but if we were watching the same file continue
        // watching it because a file change can happen between the unwatch and the subsequent watch a few lines
        // below. This caused Dev10 822594.
        bool fileChanged = !m_pstrPEName || CompareFilenames(pstrPEName, m_pstrPEName) != 0;
        if (fileChanged && m_pstrPEName)
        {
            GetCompilerPackage()->UnwatchFile(m_pstrPEName);
        }

        if (fileChanged && pstrPEName)
        {
            GetCompiler()->WatchFile(pstrPEName);
        }

#endif IDE

        m_pstrPEName = pstrPEName;
        m_pstrXMLDocCommentFileName = pstrXMLDocFileName;

#if IDE 
        // Because the CLR now implements IMetaDataAssemblyEmit::SetAssemblyProps
        // and we take advantage of this in metaemit, we can decompile to bound.
        //
        // If compstateForAssemblyChange is changed, then all uses need to be
        // reviewed, since it is now used more extensively.
        CompilationState compstateForAssemblyChange = CS_DecompileOnAssemblyManifestChange;

        // We need to decompile severely when the assembly identity changes because
        // they may participate in friend relationships.
        CompilationState compstateForAssemblyIdentityChange = CS_NoState;

#endif IDE

        STRING *pstrPEPath = m_pCompiler->AddString(pCompilerOptions->wszOutputPath);
        if (!StringPool::IsEqual(m_pstrPEPath, pstrPEPath))
        {
            m_pstrPEPath = pstrPEPath;
            GetAssemblyIdentity()->SetOutputAssemblyPath(m_pstrPEPath);

            // Since the output path change could change the absolute paths for key files with relative paths,
            // we need to decompile as though the key file changed.
#if IDE 
            compstateDecompile = min(compstateDecompile, compstateForAssemblyChange);

            // Make sure the list of projects to compile gets reset because changing the assembly identity
            // of a project can change the project dependencies.
            m_pCompilerHost->DestroyProjectCompilationList();

            // Non-FX metadata projects can reference source projects. Change in assembly identity can cause a
            // change in AssemblyRef binding, so need to decompile these metadata projects.
            fNeedToDemoteAllNonFXMetadataProjects = true;
#endif IDE
        }

        // Set the scope name and assembly name.
        VSASSERT(pCompilerOptions->wszExeName != NULL, "Missing EXE name in compiler options!");
        m_pstrScopeName = m_pCompiler->AddString(pCompilerOptions->wszExeName);

        //========================================================================
        // BEGIN: Set the assembly information.
        //========================================================================

#if IDE 
        STRING *OldWin32ResFile = m_pstrWin32ResourceFile;
        STRING *OldWin32IconFile = m_pstrWin32IconFile;
#endif IDE
        m_pstrWin32ResourceFile = m_pCompiler->AddString(pCompilerOptions->wszWin32ResFile);
        m_pstrWin32IconFile = m_pCompiler->AddString(pCompilerOptions->wszIconFile);
#if IDE 
        if (!StringPool::IsEqual(m_pstrWin32ResourceFile, OldWin32ResFile) ||
            !StringPool::IsEqual(m_pstrWin32IconFile, OldWin32IconFile))
        {
            compstateDecompile = min(compstateDecompile, compstateForAssemblyChange);
        }
#endif IDE

        // Key-pair file used to create strong-named (public) assemblies
        pstrStrongNameKeyFile = m_pCompiler->AddString(pCompilerOptions->wszStrongNameKeyFile);

        // UAC manifest file
        STRING *OldUacManifestFile = m_pstrUacManifestFile;
        m_pstrUacManifestFile = pCompilerOptions->wszUacManifestFile ?
            m_pCompiler->AddString(pCompilerOptions->wszUacManifestFile) :
            NULL;
#if IDE 
        if ( !StringPool::IsEqual(m_pstrUacManifestFile, OldUacManifestFile) )
        {
            compstateDecompile = min(compstateDecompile, compstateForAssemblyChange);
        }
#endif IDE

        if (StringPool::IsEqual(pstrStrongNameKeyFile, STRING_CONST(m_pCompiler, EmptyString)))
        {
            // Important to set this to NULL here for venus scenario.
            pstrStrongNameKeyFile = NULL;
        }

        if (!StringPool::IsEqual(pstrStrongNameKeyFile, GetProjectSettingForKeyFileName()))
        {
#if IDE 
            compstateDecompile = min(compstateDecompile, compstateForAssemblyIdentityChange);

            // Make sure the list of projects to compile gets reset because changing the assembly identity
            // of a project can change the project dependencies.
            m_pCompilerHost->DestroyProjectCompilationList();

            // Non-FX metadata projects can reference source projects. Change in assembly identity can cause a
            // change in AssemblyRef binding, so need to decompile these metadata projects.
            fNeedToDemoteAllNonFXMetadataProjects = true;
#endif IDE
            SetProjectSettingForKeyFileName(pstrStrongNameKeyFile);
        }


        // Crypto container used to create strong-named (public) assemblies
        pstrStrongNameContainer = m_pCompiler->AddString(pCompilerOptions->wszStrongNameContainer);

        if (StringPool::IsEqual(pstrStrongNameContainer, STRING_CONST(m_pCompiler, EmptyString)))
        {
            // Important to set this to NULL here for venus scenario.
            pstrStrongNameContainer = NULL;
        }

        if (!StringPool::IsEqual(pstrStrongNameContainer, GetProjectSettingForKeyContainerName()))
        {
#if IDE 
            compstateDecompile = min(compstateDecompile, compstateForAssemblyIdentityChange);

            // Make sure the list of projects to compile gets reset because changing the assembly identity
            // of a project can change the project dependencies.
            m_pCompilerHost->DestroyProjectCompilationList();

            // Non-FX metadata projects can reference source projects. Change in assembly identity can cause a
            // change in AssemblyRef binding, so need to decompile these metadata projects.
            fNeedToDemoteAllNonFXMetadataProjects = true;
#endif IDE
            SetProjectSettingForKeyContainerName(pstrStrongNameContainer);
        }

#if IDE 
        if ((bool)pCompilerOptions->bDelaySign != m_fDelaySign)
        {
            compstateDecompile = min(compstateDecompile, compstateForAssemblyIdentityChange);
        }
#endif IDE

        // Delay-sign the strong-name
        m_fDelaySign = pCompilerOptions->bDelaySign;


        //========================================================================
        // END: Set the assembly information.
        //========================================================================

#if IDE 
        if (m_fGenerateDebugCode != (bool)pCompilerOptions->bGenerateSymbolInfo)
        {
            compstateDecompile = min(compstateDecompile, CS_TypesEmitted);
        }
#endif IDE
        m_fGenerateDebugCode = pCompilerOptions->bGenerateSymbolInfo;

#if IDE 
        if (m_fGeneratePDB != (m_fGenerateDebugCode || pCompilerOptions->bGeneratePdbOnly))
        {
            compstateDecompile = min(compstateDecompile, CS_TypesEmitted);
        }
#endif IDE
        m_fGeneratePDB = m_fGenerateDebugCode || pCompilerOptions->bGeneratePdbOnly;

#if IDE 
        if (m_fGenerateOptimalIL != (bool)pCompilerOptions->bOptimize)
        {
            compstateDecompile = min(compstateDecompile, CS_TypesEmitted);
        }
#endif IDE
        m_fGenerateOptimalIL = pCompilerOptions->bOptimize;

#if IDE 
        // 


        bool fNewGenerateENCableCode = m_fGenerateDebugCode && !m_fGenerateOptimalIL && CComQIPtr<IVsENCRebuildableProjectCfg2>(GetHierarchy());

        if (m_fGenerateENCableCode != fNewGenerateENCableCode)
        {
            // When enabling ENC code then the project must go all the way to no state.
            // This is because th exml helper class is a transient symbol in the optimize case but
            // always emitted in the ENC case so the symbol can not be shared across compilations.

            compstateDecompile = min(compstateDecompile, fNewGenerateENCableCode ? CS_NoState : CS_TypesEmitted);
        }

        m_fGenerateENCableCode = fNewGenerateENCableCode;
#else
        // The command-line compiler will never generate code capable of doing ENC.
        m_fGenerateENCableCode = false;
#endif IDE

#if IDE 
        if (m_WarningLevel != pCompilerOptions->WarningLevel)
        {
            compstateDecompile = CS_NoState;
        }
#endif IDE
        m_WarningLevel = pCompilerOptions->WarningLevel;

        if (pCompilerOptions->cWarnItems)
        {
            VSASSERT(pCompilerOptions->warningsLevelTable, "Bad compiler option: Null warnings level table ");
#if IDE 
            if (!IsEqual(&m_daWarningsLevelTable, pCompilerOptions->cWarnItems, pCompilerOptions->warningsLevelTable))
            {
                compstateDecompile = CS_NoState;
            }
#endif IDE
            m_daWarningsLevelTable.Collapse();
            for (DWORD i = 0; i < pCompilerOptions->cWarnItems; i++)
            {
                m_daWarningsLevelTable.Add() = pCompilerOptions->warningsLevelTable[i];
            }
        }
        else
        {
            // empty new table
#if IDE 
            if (m_daWarningsLevelTable.Count())
            {
                compstateDecompile = CS_NoState;
            }
#endif IDE
            m_daWarningsLevelTable.Collapse();
        }


        //========================================================================
        // Set the preferred platform type for the assembly.
        //========================================================================
        PlatformKinds PlatformKind = MapStringToPlatformKind(pCompilerOptions->wszPlatformType);
#if IDE 
        if (m_PlatformKind != PlatformKind)
        {
            // Although decompilation till CS_TypeEmitted is enough for now, once Alink work
            // if completed in M3.3 and we do the remaining work, then decompilation till
            // CS_Bound will be required. So decompiling to CS_Bound.
            //
            compstateDecompile = min(compstateDecompile, CS_Bound);
        }
#endif IDE
        m_PlatformKind = PlatformKind;

        //========================================================================
        // Set the preferred load address of generated binary.
        //========================================================================
#if IDE 
        if (m_dwLoadAddress != pCompilerOptions->dwLoadAddress)
        {
            compstateDecompile = min(compstateDecompile, CS_TypesEmitted);
        }
#endif IDE
        m_dwLoadAddress = pCompilerOptions->dwLoadAddress;

        //========================================================================
        // Set the preferred file alignment of generated binary.
        //========================================================================
#if IDE 
        if (m_dwAlign != pCompilerOptions->dwAlign)
        {
            compstateDecompile = min(compstateDecompile, CS_TypesEmitted);
        }
#endif IDE
        m_dwAlign = pCompilerOptions->dwAlign;

        //========================================================================
        // Set the default code page of loaded files
        //
        // BEWARE! This option is only use by the command-line compiler. If the
        // IDE tries to use this option, this won't necessarily work correctly
        // because decompiling a file does not requre the project to reload the
        // text.
        //========================================================================
#if IDE 
        if (m_dwDefaultCodePage != pCompilerOptions->dwDefaultCodePage)
        {
            VSFAIL("This should not be set in the IDE!");
        }
#endif IDE
        m_dwDefaultCodePage = pCompilerOptions->dwDefaultCodePage;


        //========================================================================
        // Handle standard libraries
        //========================================================================
#if IDE 
        if ((bool)pCompilerOptions->bNoStandardLibs != m_fNoStandardLibraries)
        {
            if (pCompilerOptions->bNoStandardLibs)
            {
                if (RemoveStandardLibraries())
                {
                    compstateDecompile = min(compstateDecompile, CS_Declared);
                }
            }
            else
            {
                if (AddStandardLibraries())
                {
                    compstateDecompile = min(compstateDecompile, CS_Declared);
                }
            }
        }
#else
        if (!pCompilerOptions->bNoStandardLibs)
        {
            AddStandardLibraries();
        }
#endif IDE

        m_fNoStandardLibraries = pCompilerOptions->bNoStandardLibs;

        if (m_fCodeModule)
        {
            // Not an assembly, so has no annotated assembly name
            m_AssemblyIdentity.InitAssemblyIdentityInfo(m_pCompiler, m_pCompilerHost, this);
        }
        else
        {
            m_AssemblyIdentity.SetAssemblyName(m_pCompiler->AddStringWithLen(pCompilerOptions->wszExeName,
                               PathFindExt(pCompilerOptions->wszExeName) - pCompilerOptions->wszExeName));
        }

        // Get the language version.
        if (pCompilerOptions->langVersion != m_LangVersion)
        {
            m_LangVersion = pCompilerOptions->langVersion;
#if IDE 
            compstateDecompile = CS_NoState;
#endif IDE
        }

#if IDE 

        // If required, do decompile now

        if (fNeedToDemoteAllNonFXMetadataProjects)
        {
            EditFilter::DecompileReferencingMetadataProjectsToNoState(this);
        }

        if (compstateDecompile != CS_MAX)
        {
            EditFilter::DecompileProjectNoCommit(this, compstateDecompile);
            EditFilter::DecompileReferencingProjectsNoCommit(
                this,
                (compstateDecompile > CS_Declared) ? compstateDecompile : CS_Declared,
                (compstateDecompile > CS_Declared || fNeedToDemoteAllNonFXMetadataProjects) ? CS_MAX :CS_Declared);

            GetCompilerPackage()->CommitEditListDemotions(true, false, this);
        }

#endif IDE

        m_bIsHighEntropyVA = pCompilerOptions->bHighEntropyVA;

        if (!pCompilerOptions->wszSubsystemVersion)
        {
            bool fIsArmOrAppContainerExeOrWinMDObj = (MapStringToPlatformKind(pCompilerOptions->wszPlatformType) == Platform_ARM || pCompilerOptions->OutputType == OUTPUT_AppContainerEXE || pCompilerOptions->OutputType == OUTPUT_WinMDObj);
            if (fIsArmOrAppContainerExeOrWinMDObj)
            {
                m_subsystemVersion.major = 6;
                m_subsystemVersion.minor = 2;
            }
            else
            {
                m_subsystemVersion.major = 4;
                m_subsystemVersion.minor = 0;
            }
        }
        else
        {
            auto string_to_natural = [](const wchar_t* str, const wchar_t** end) -> long
            {
                // Get a natural number using wcstol semantics but if the first character
                // is not a decimal digit then signal an error by setting errno to ERANGE.
                // This disallows leading whitespace, unary + or - operator prefixes and
                // empty strings.
                //
                // Don't disallow leading zeros because then otherwise you cannot write
                // subsystem versions in the form "6.02" which is how they're sometimes
                // presented even though the leading zero on the minor version is
                // superfluous.
                long result = wcstol(str, const_cast<wchar_t**>(end), 10);
                if (!isdigit(*str))
                {
                    errno = ERANGE;
                }
                return result;
            };

            long major = 0;
            long minor = 0;
            LPCWSTR pszTemp = pCompilerOptions->wszSubsystemVersion;
            
            errno = 0;
            major = string_to_natural(pszTemp, &pszTemp);

            if (errno != ERANGE && *pszTemp == L'.')
            {
                minor = string_to_natural(pszTemp+1, &pszTemp);
            }

            VSASSERT((major >= 0 && minor >= 0) || errno == ERANGE, "string_to_natural shouldn't successfully return negative numbers");
            if (*pszTemp != L'\0' || errno == ERANGE || major > 65535 || minor > 65535)
            {
                m_ErrorTable.CreateError(ERRID_InvalidSubsystemVersion, NULL, pCompilerOptions->wszSubsystemVersion);

                major = 4;
                minor = 0;
            }

            m_subsystemVersion.major = major;
            m_subsystemVersion.minor = minor;
        }

#if IDE 

        FireOnChange(TRUE, PRJ_COMPILEROPTIONS);

        // If the XMLDoc file name changes, that means that the XMLDoc was either enables or disabled, since the
        // user does not have the option of providing the file name. This is why all hidden regions will need to
        // get regenerated from scratch if this happens.
        if (IsNeedToUpdateHiddenRegions)
        {
            // If need to update HiddenRegions, we loop over all files with a view and update their hidden regions.
            CompilerFile *pCompilerFile;
            FileIterator fileiter(this);

            while (pCompilerFile = fileiter.Next())
            {
                if (pCompilerFile->IsSourceFile() && pCompilerFile->PSourceFile()->GetSourceFileView())
                {
                    if (pCompilerFile->PSourceFile()->GetParseTreeService())
                    {
                        pCompilerFile->PSourceFile()->GetParseTreeService()->ClearParseTreeCache();
                    }

                    pCompilerFile->PSourceFile()->GetSourceFileView()->UpdateCommentRegions();
                }
            }
        }


        // We can now let My Extensibility Service listen to Reference changes
        // The expression editor never uses the My Extensibility Service.
        m_fEnableVBReferenceChangedService = !m_isExpressionEditorProject;    

        if (!m_isExpressionEditorProject)
        {
            GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->SetCompilerOptions(
                GetProjectId(),
                reinterpret_cast<CodeSenseVBCompilerOptions*>(pCompilerOptions));
        }

#endif IDE

        IfFailRet(SetVBRuntimeOptions(pCompilerOptions));

#if IDE

    }

#endif

    ValidateOptions();

    VB_EXIT_NORETURN();

    RRETURN( hr );
}

void CompilerProject::ValidateOptions()
{
    if (m_PlatformKind == Platform_Anycpu32bitpreferred && (OutputIsLibrary() || OutputIsModule() || OutputIsNone()))
    {
        m_ErrorTable.CreateError(ERRID_LibAnycpu32bitPreferredConflict,NULL);
    }
    else
    {
        ThrowIfFalse( m_PlatformKind != Platform_Anycpu32bitpreferred || OutputIsExe());
    }

    return;
}

//============================================================================
// set the module assembly name.
//============================================================================

STDMETHODIMP CompilerProject::SetModuleAssemblyName( LPCWSTR wszName )
{
    if( wszName == NULL ) return( E_INVALIDARG );
    STRING* strName = m_pCompiler->AddString( wszName );

    if( FillInAssemblyIdentity( strName, &m_ModuleAssemblyIdentity ) )
    {

#if IDE

        if (!m_isExpressionEditorProject)
        {
            GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->SetModuleAssemblyName(
                GetProjectId(),
                CComBSTR(wszName));
        }

#endif

        return( S_OK );
    }
    else
    {
        return( E_FAIL );
    }
}

//============================================================================
// called before or after a project is adding its files when loading.
//============================================================================

STDMETHODIMP CompilerProject::SetBackgroundCompilerPriorityNormal()
{
    CheckInMainThread();
#if IDE 
    GetCompiler()->SetPriority(THREAD_PRIORITY_NORMAL);

    if (!m_isExpressionEditorProject)
    {
        GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->SetBackgroundCompilerPriorityNormal(
            GetProjectId());
    }

#endif
    return S_OK;
}

STDMETHODIMP CompilerProject::SetBackgroundCompilerPriorityLow()
{
    CheckInMainThread();
#if IDE 
    // 

    GetCompiler()->SetPriority(THREAD_PRIORITY_LOWEST);

    if (!m_isExpressionEditorProject)
    {
        GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->SetBackgroundCompilerPriorityLow(
            GetProjectId());
    }

#endif
    return S_OK;
}

//============================================================================
// called each time a file is added to the project (via Add Item or
//  during project open).
//============================================================================
STDMETHODIMP CompilerProject::AddFile
(
    LPCWSTR wszFilePath,
    VSITEMID itemid,
    BOOL fAddDuringOpen
)
{
    // Note: Never call this method from VB code directly - call the Internal version.
    VB_VERIFY_NOT_REENTRANT();
    return AddFileInternal(wszFilePath, itemid, fAddDuringOpen);
}

STDMETHODIMP CompilerProject::AddFileInternal
(
    LPCWSTR wszFilePath,
    VSITEMID itemid,
    BOOL fAddDuringOpen
)
{
    CheckInMainThread();
    SourceFile *pfile = NULL;

#if IDE 
    IVsProject * pproj = NULL;
#endif

    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();

    VB_ENTRY();

#if IDE 
    BackgroundCompilerLock compilerLock(this);

    // if the itemid that we were passed is 0 and we have a project hierarchy pointer, we can get
    // the item id from it.
    if (itemid == 0 && m_pHierarchy && wszFilePath)
    {
        BOOL fFound;
        VSDOCUMENTPRIORITY priority;

        IfFailGo(m_pHierarchy->QueryInterface(IID_IVsProject, (void **)&pproj));

        if (pproj != NULL)
        {
            IfFailGo(pproj->IsDocumentInProject(wszFilePath,
                                                &fFound,
                                                &priority,
                                                &itemid));
            RELEASE(pproj);

            if (fFound == FALSE)
            {
                itemid = 0;
            }

        }
    }

#endif

    //
    // Prepare the addition
    //

    IfFailGo(SourceFile::Create(this,
                                wszFilePath,
                                NULL,           // real file, no in-memory buffer
                                0,
                                itemid,
                                &pfile));

    //
    // Commit the addition
    //

    pfile->m_cs = CS_NoState;
    m_dlFiles[CS_NoState].InsertLast(pfile);

#if IDE 

    pfile->SetDecompilationState(pfile->m_cs);

    if( pfile->IsSourceFile() )
    {
        FireOnFile(CLXN_Add, pfile->GetCodeFile());
    }


    // Demote the whole project to Declared. (Symbols are still valid)
    EditFilter::DecompileProject(this, CS_Declared);

    // But make the project in NoState so that the new file is dealt with
    EditFilter::DecompileProjectOnly(this, CS_NoState);

    // AssemblyVersionAttribute may have changed -- decompile
    // all referencing projects to avoid stale AssemblyRefs.
    // We need to decompile referencing projects to CS_Declared so that
    // their binding gets reprocessed. VS525421
    EditFilter::DecompileReferencingProjects(this, CS_Declared, CS_Declared);

    // This code is supporting the scenario where a file is added into CompilerProject
    // but is already open in the running documents table.  When that occurs we must 
    // create a SourceFileView instance for the document so that the Language Service 
    // will be hooked up to the buffer.  A couple of scenarios where this can occur
    // are 
    //  - Rename a file extension from anything else to .vb
    //  - Rename a file 
    //  - Save a Zero Impact Project (ZIP)
    //  - Venus (breaking light speed is possible in a Venus app)
    //  - WPF Generated XAML files (Something.g.vb) rely on this
    if (!fAddDuringOpen && wszFilePath && m_pHierarchy && pfile->IsSourceFile())
    {
        CComPtr<IVsTextLines> pDocData = NULL;
        CComPtr<IVsCodeWindow> pDocView = NULL;
        FindDocDataAndDocViewByMoniker(m_pHierarchy,wszFilePath,&pDocData,&pDocView);
        if (pDocData != NULL)
        {
            // CCodeWindowManager::AddAdornments will create always ensure a SourceFileView exists
            // for every open vb file.  If we find a SourceFileView, we should update its sourcefile.
            // We may not find a SourceFileView if the file is a .cs or .txt file or open in the invisible
            // editor
            SourceFileView* pSourceFileView =  SourceFileView::GetFromIVsTextLines(pDocData);
            if (pSourceFileView)
            {
                pSourceFileView->UpdateViewData(pfile);
            }
            else
            {
                // IVsTextLines interface can be implemented by a package other than VS managed editor. In this case, we'll not be able to create SourceFileView
                // off IVsTextLines correctly. Thus we need to check first if we can work with passed IVsTextLines object. Otherwise, we cannot create a view for the given
                // text buffer.
                CComPtr<IVxTextBuffer> spIVxTextBuffer;
                if (VxUtil::TryConvertVsTextLinesToVxTextBuffer(pDocData, __ref spIVxTextBuffer))
                {
                    SourceFileView::CreateView(pfile,pDocData,pDocView);
                }
            }
        }
    }

    if (!m_isExpressionEditorProject)
    {
        GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->AddFile(
            GetProjectId(),
            CComBSTR(wszFilePath),
            itemid,
            fAddDuringOpen);
    }

#endif IDE

    pfile = NULL;


    VB_EXIT_NORETURN();

Error:
#if IDE 
    RELEASE(pfile);
    RELEASE(pproj);
#else !IDE
    delete pfile;
#endif !IDE

    RRETURN( hr );
}

//============================================================================
// Removes a file from the project.
//============================================================================

STDMETHODIMP CompilerProject::RemoveFile
(
    LPCWSTR wszFilePath,
    VSITEMID itemid
)
{
    // Note: Never call this method from VB code directly - call the Internal version.
    VB_VERIFY_NOT_REENTRANT();
    return RemoveFileInternal( wszFilePath, itemid );
}

STDMETHODIMP CompilerProject::RemoveFileInternal
(
    LPCWSTR wszFilePath,
    VSITEMID itemid
)
{
#if !IDE

    VSFAIL("Unexpected call.");

    return E_NOTIMPL;

#else IDE

    CheckInMainThread();

    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();

    VB_ENTRY1(E_FAIL);
    CompilerFile *pCompilerFile;

    FileIterator fileiter(this);

    while (pCompilerFile = fileiter.Next())
    {
        // Note that we also check the filepath. There are transient cases during a saveas
        // where the project system will add a file with a differnt path but the same itemid.
        if (pCompilerFile->IsSourceFile() 
            && pCompilerFile->PSourceFile()->GetItemId() == itemid 
            && CompareFilenames(pCompilerFile->PSourceFile()->GetFileName(), wszFilePath) == 0)
        {
            //
            // Commit the removal
            //
            RemoveCompilerFile(pCompilerFile);

            // AssemblyVersionAttribute may have changed -- decompile
            // all referencing projects to avoid stale AssemblyRefs.
            // We need to decompile referencing projects to CS_Declared so that
            // their binding gets reprocessed. VS525421
            EditFilter::DecompileReferencingProjects(this, CS_Declared, CS_Declared);

            // Break out.
            hr = S_OK;
            break;
        }
    }

    if (!m_isExpressionEditorProject)
    {
        GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->RemoveFile(
            GetProjectId(),
            CComBSTR(wszFilePath),
            itemid);
    }

    VB_EXIT_NORETURN();

    RRETURN(hr);

#endif IDE
}

//============================================================================
// Renames a file from the project.
//============================================================================

STDMETHODIMP CompilerProject::RenameFile
(
    LPCWSTR wszOldFileName,
    LPCWSTR wszNewFileName,
    VSITEMID itemid
)
{
    // Note: Never call this method from VB code directly - call the Internal version.
    VB_VERIFY_NOT_REENTRANT();
    return RenameFileInternal( wszOldFileName, wszNewFileName, itemid );
}

STDMETHODIMP CompilerProject::RenameFileInternal
(
    LPCWSTR wszOldFileName,
    LPCWSTR wszNewFileName,
    VSITEMID itemid
)
{
#if !IDE

    VSFAIL("Unexpected call.");

    return E_NOTIMPL;

#else IDE

    CheckInMainThread();

    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();

    VB_ENTRY1(E_FAIL);
    CompilerFile *pCompilerFile;

    FileIterator fileiter(this);
    IfFalseGo(wszOldFileName && wszNewFileName, E_INVALIDARG);
    while (pCompilerFile = fileiter.Next())
    {
        if (pCompilerFile->IsSourceFile() && pCompilerFile->PSourceFile()->GetItemId() == itemid)
        {
            if (pCompilerFile->PSourceFile()->IsValidSourceFile())
            {
                BackgroundCompilerLock compilerLock(this);

                pCompilerFile->PSourceFile()->SignalSourceChange();

                // Tell the edit filter about the edit which will
                // decompile the necessary classes
                EditFilter::DecompileFile(pCompilerFile->PSourceFile(), CS_NoState);

                // Rename it from the project.
                pCompilerFile->PSourceFile()->RenameFileFromProject(wszNewFileName);

                // Restart the background compiler.
                compilerLock.Start();

                // Remove the old Tasklist items
                GetCompiler()->DoRefreshTaskList();

                hr = S_OK;
                break;
            }
        }
    }

    if (!m_isExpressionEditorProject)
    {
        GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->RenameFile(
            GetProjectId(),
            CComBSTR(wszOldFileName),
            CComBSTR(wszNewFileName),
            itemid);
    }

    VB_EXIT_NORETURN();

Error:
    RRETURN(hr);

#endif IDE
}

//============================================================================
// Removes a file from the project.
//============================================================================

STDMETHODIMP CompilerProject::RemoveFileByName
(
    LPCWSTR wszPath
)
{
    // Note: Never call this method from VB code directly - call the Internal version.
    VB_VERIFY_NOT_REENTRANT();
    return RemoveFileByNameInternal( wszPath );
}

STDMETHODIMP CompilerProject::RemoveFileByNameInternal
(
    LPCWSTR wszPath
)
{
#if !IDE 

    VSFAIL("Unexpected call.");

    return E_NOTIMPL;

#else IDE 

    CheckInMainThread();

    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();

    VB_ENTRY1(E_FAIL);
    CompilerFile *pCompilerFile;

    FileIterator fileiter(this);

    while (pCompilerFile = fileiter.Next())
    {
        if (pCompilerFile->IsSourceFile() &&
            CompareFilenames(pCompilerFile->PSourceFile()->GetFileName(), wszPath) == 0)
        {
            //
            // Commit the removal
            //
            RemoveCompilerFile(pCompilerFile);

            // Break out.
            hr = S_OK;
            break;
        }
    }

    if (!m_isExpressionEditorProject)
    {
        GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->RemoveFileByName(
            GetProjectId(),
            CComBSTR(wszPath));
    }

    VB_EXIT_NORETURN();

    RRETURN(hr);

#endif !IDE 
}

//============================================================================
// called by vba each time a buffer is added to the project
//============================================================================
STDMETHODIMP CompilerProject::AddBuffer
(
    _In_count_(dwLen) WCHAR *pwszBuffer,
    DWORD dwLen,
    _In_z_ LPCWSTR wszMkr,
    VSITEMID itemid,
    BOOL     fAdvise,
    BOOL     fShowErrorsInTaskList
)
{
    return AddBufferHelper(pwszBuffer,dwLen,wszMkr,itemid,fAdvise,fShowErrorsInTaskList,NULL);
}

HRESULT CompilerProject::AddBufferHelper
(
    _In_bytecount_(dwLen) WCHAR *pwszBuffer,
    _In_ DWORD dwLen,
    _In_z_ LPCWSTR wszMkr,
    _In_ VSITEMID itemid,
    _In_ BOOL     fAdvise,
    _In_ BOOL     fShowErrorsInTaskList,
    _Out_ SourceFile **ppFile,
    _In_opt_ bool fWatchFile, // = true, // file system watcher
    _In_opt_ SolutionExtensionEnum nSolutionExtension // = SolutionExtensionNone // if it's a My template 
    
)
{
    SourceFile *pfile = NULL;

#if IDE 
    IVsProject * pproj = NULL;
#endif

    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();

    VB_ENTRY();
#if IDE 
    BackgroundCompilerLock compilerLock(this);

    // if the itemid that we were passed is 0 and we have a project hierarchy pointer, we can get
    // the item id from it.
    if (itemid == 0 && m_pHierarchy)
    {
        BOOL fFound;
        VSDOCUMENTPRIORITY priority;

        IfFailGo(m_pHierarchy->QueryInterface(IID_IVsProject, (void **)&pproj));

        if (pproj != NULL)
        {
            IfFailGo(pproj->IsDocumentInProject(wszMkr,
                                                &fFound,
                                                &priority,
                                                &itemid));
            RELEASE(pproj);

            if (fFound == FALSE)
            {
                itemid = 0;
            }

        }
    }
#endif

    //
    // Prepare the addition
    //

    IfFailGo(SourceFile::Create(this,
                                wszMkr, // pass the moniker in lieu of a filename.
                                pwszBuffer,
                                dwLen,
                                itemid,
                                &pfile,
                                fWatchFile,
                                nSolutionExtension                                
                                ));

    pfile->m_cs = CS_NoState;
    m_dlFiles[CS_NoState].InsertLast(pfile);

#if IDE 
    pfile->SetDecompilationState(pfile->m_cs);
    pfile->SetShowErrorsInTaskList(fShowErrorsInTaskList);

    if( fAdvise && pfile->IsSourceFile() )
    {
        FireOnFile(CLXN_Add, pfile->GetCodeFile());
    }

    // Demote the whole project to Declared. (Symbols are still valid)
    EditFilter::DecompileProject(this, CS_Declared);

    // But make the project is in NoState so that the new file is dealt with
    EditFilter::DecompileProjectOnly(this, CS_NoState);
    if (!m_isExpressionEditorProject)
    {
        GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->AddBuffer(
            GetProjectId(),
            CComBSTR(pwszBuffer),
            dwLen,
            CComBSTR(wszMkr),
            itemid,
            fAdvise,
            fShowErrorsInTaskList);
    }

#endif IDE
    if (ppFile)
    {
        *ppFile = pfile;
    }
    // NULL'ing this out prevents the Release below
    pfile = NULL;

    VB_EXIT_NORETURN();

Error:
#if IDE 
    RELEASE(pfile);
    RELEASE(pproj);
#else !IDE
    delete pfile;
#endif !IDE

    RRETURN( hr );
} // AddBuffer

//============================================================================
// Allows vba/script to pass in memory-backed IStream for PDB symbol store
//============================================================================
STDMETHODIMP CompilerProject::SetStreamForPDB(IStream * pStreamPDB)
{
    if (m_pStreamPDB)
    {
        m_pStreamPDB->Release();
    }

    m_pStreamPDB = pStreamPDB;

    if (m_pStreamPDB)
    {
        m_pStreamPDB->AddRef();
    }

#if IDE

    if (!m_isExpressionEditorProject)
    {
        GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->SetStreamForPDB(
            GetProjectId(),
            pStreamPDB);
    }

#endif IDE

    return NOERROR;
} // AddIStreamForPDB

//============================================================================
// Add a reference to another project within the current solution
//============================================================================

STDMETHODIMP CompilerProject::AddEmbeddedProjectReference
(
    IVbCompilerProject *pReferencedCompilerProject
)
{
    CheckInMainThread();
    return AddProjectReferenceInternal(pReferencedCompilerProject, REFERENCEFLAG_MANUAL | REFERENCEFLAG_EMBEDDED);
}

STDMETHODIMP CompilerProject::AddProjectReference
(
    IVbCompilerProject *pReferencedCompilerProject
)
{
    // Note: Never call this method from VB code directly - call the Internal version.
    VB_VERIFY_NOT_REENTRANT();
    return AddProjectReferenceInternal( pReferencedCompilerProject );
}

STDMETHODIMP CompilerProject::AddProjectReferenceInternal
(
    IVbCompilerProject *pReferencedCompilerProject
)
{
    CheckInMainThread();
    return AddProjectReferenceInternal(pReferencedCompilerProject, REFERENCEFLAG_MANUAL);
}

HRESULT CompilerProject::AddProjectReferenceInternal
(
    IVbCompilerProject *pReferencedCompilerProject,
    UINT ReferenceFlags
)
{
    VB_ENTRY();
    
    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();
    CompilerProject *pReferencedProject = static_cast<CompilerProject *>(pReferencedCompilerProject);

    if (pReferencedProject == this)
    {
        //Can't add a project reference to yourself.
        return E_FAIL;
    }

    //Make sure the referenced project shares the same CompilerHost
    if (GetCompilerHost() != pReferencedProject->GetCompilerHost())
    {
#if IDE 
        return (HrMakeReplWithError(STRID_REF_ReferenceToWrongTargetType, E_FAIL, pReferencedProject->GetAssemblyName()));
#else
        return E_FAIL;
#endif
    }

#if IDE 
    if ( HasDirectOrIndirectReferenceOnMe(pReferencedProject))
    {
        return (HrMakeReplWithError(STRID_REF_CircularReference, E_FAIL, pReferencedProject->GetAssemblyName()));
    }


    BackgroundCompilerLock compilerLock(this);
#endif IDE

    // Add the reference.
    AddReference(pReferencedProject, ReferenceFlags);

#if IDE
    if (!m_isExpressionEditorProject)
    {
        if ((ReferenceFlags & REFERENCEFLAG_EMBEDDED) != 0)
        {
            GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->AddEmbeddedProjectReference(
                GetProjectId(),
                pReferencedProject->GetProjectId());
        }
        else
        {
            GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->AddProjectReference(
                GetProjectId(),
                pReferencedProject->GetProjectId());
        }
    }

#endif IDE

    VB_EXIT_NORETURN();

#if DEBUG
    CompilerProject::VerifyNoDuplicateReferences();
#endif DEBUG

    RRETURN( hr );
}

//============================================================================
// Removes a reference to another project in the current solution.
//============================================================================

STDMETHODIMP CompilerProject::RemoveProjectReference
(
    IVbCompilerProject* pReferencedCompilerProject
)
{
    // Note: Never call this method from VB code directly - call the Internal version.
    VB_VERIFY_NOT_REENTRANT();
    return RemoveProjectReferenceInternal( pReferencedCompilerProject );
}

STDMETHODIMP CompilerProject::RemoveProjectReferenceInternal
(
    IVbCompilerProject* pReferencedCompilerProject
)
{
#if IDE 
    CheckInMainThread();

    VB_ENTRY1(E_FAIL);

    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();

    CompilerProject *pReferencedProject = static_cast<CompilerProject *>(pReferencedCompilerProject);

    if (pReferencedProject && !pReferencedProject->IsMetaData())
    {
        // First, try to find the referenced project in the references list.
        for (ULONG i = 0; i < m_daReferences.Count(); ++i)
        {
            if (m_daReferences.Element(i).m_pCompilerProject == pReferencedProject)
            {
                bool Removed = false;
                hr = RemoveOneReference(i, &Removed);
                break;
            }
        }
    }

    if (!m_isExpressionEditorProject)
    {
        GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->RemoveProjectReference(
            GetProjectId(),
            pReferencedProject->GetProjectId());
    }

    VB_EXIT_NORETURN();

#if DEBUG
        CompilerProject::VerifyNoDuplicateReferences();
#endif DEBUG

    RRETURN( hr );
#else
    RRETURN( E_NOTIMPL );
#endif IDE
}

//============================================================================
// Adds a MetaData reference.
//============================================================================
STDMETHODIMP CompilerProject::AddMetaDataReference
(
    LPCWSTR wszFilePath,
    BOOL bAssembly
)
{
    // Note: Never call this method from VB code directly - call the Internal version.
    VB_VERIFY_NOT_REENTRANT();

    CheckInMainThread();
    return AddMetaDataReferenceInternal(wszFilePath, bAssembly, REFERENCEFLAG_MANUAL, NULL);
}

//============================================================================
// Adds an embedded MetaData reference.
//============================================================================
STDMETHODIMP CompilerProject::AddEmbeddedMetaDataReference
(
    LPCWSTR wszFilePath
)
{
    // Note: Never call this method from VB code directly - call the Internal version.
    VB_VERIFY_NOT_REENTRANT();

    CheckInMainThread();
    return AddMetaDataReferenceInternal(wszFilePath, TRUE, REFERENCEFLAG_MANUAL | REFERENCEFLAG_EMBEDDED, NULL);
}

#if IDE
// Updates assembly reference path to support multi-targeting scenarios.
STRING * CompilerProject::FixUpReferencePathForMultiTargeting(_In_z_ STRING * strFilePath)
{
    ThrowIfNull(strFilePath);

    // Check if we are adding a reference to one of the "special" assemblies.
    // We use a trick in Dev10 where we merge CompilerHost-s for 4.0 Full and 4.0 Client profiles.
    // There is a set of assemblies (see list below) which can only exist in single instance
    // inside compiler host (i.e. for each assembly identity with these names there can only
    // be one assembly in CompilerHost). To make that happen is we load such assemblies from
    // the Full profile and redirect Client profile assemblies to the ones loaded from Full.

    LPCTSTR wszFileName = PathFindFileName(strFilePath);
    if (!wszFileName ||
        CompareFilenames(wszFileName, L"mscorlib.dll") &&
        CompareFilenames(wszFileName, L"Microsoft.VisualBasic.dll") &&
        CompareFilenames(wszFileName, L"System.Core.dll") &&
        CompareFilenames(wszFileName, L"System.dll"))
    {
        return strFilePath;
    }

    // Check if project is targeting the Client profile.
    const CComBSTR bstrTargetFrameworkMoniker = this->GetTargetFrameworkMoniker();
    TargetFrameworkMonikerParts parts;
    if (FAILED(VspParseTargetFrameworkMoniker(bstrTargetFrameworkMoniker, parts)) || 
        parts.m_strSubtype != L"Client")
    {
        return strFilePath;
    }

    // Project is targeting Client profile - lets try to resolve same assembly, but in the Full profile.
    CString fullTargetFrameworkMoniker;
    parts.m_strSubtype = L"";
    if (FAILED(VspMakeTargetFrameworkMoniker(parts, fullTargetFrameworkMoniker)))
    {
        return strFilePath;
    }

    STRING * strFullTargetPath = GetCompilerPackage()->ResolveAssemblyPathInTargetFramework(
        wszFileName, 
        fullTargetFrameworkMoniker);

    return strFullTargetPath ? strFullTargetPath : strFilePath;
}

void CompilerProject::SetTargetFrameworkMoniker(_In_opt_ IVsHierarchy * pVsHierarchy)
{
    CComVariant targetFramework;
    if (pVsHierarchy &&
        SUCCEEDED(pVsHierarchy->GetProperty(
            VSITEMID_ROOT, 
            VSHPROPID_TargetFrameworkMoniker, 
            &targetFramework)) &&
        V_VT(&targetFramework) == VT_BSTR)
    {
        m_bstrTargetFrameworkMoniker = V_BSTR(&targetFramework);
    }
}
#endif IDE

HRESULT CompilerProject::AddMetaDataReferenceInternal
(
    LPCWSTR wszFilePath,
    bool bAssembly,
    UINT ReferenceFlags,
    bool* pAdded
)
{
    VSASSERT( !(ReferenceFlags & REFERENCEFLAG_EMBEDDED) || bAssembly, "Embedded references must be assembly references" );
    bool Added = false;

    CompilerProject *pProject = NULL;

    VB_ENTRY();

    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();

    STRING *pstrFileName;
    bool bCreatedNewMetadataProject;

#if IDE 
    BackgroundCompilerLock compilerLock(this);

    auto pstrOriginalFileName = m_pCompiler->AddString(wszFilePath);
    // Translate paths to special asemblies in case of Client framework profiles.
    pstrFileName = FixUpReferencePathForMultiTargeting(pstrOriginalFileName);
    if (!StringPool::IsEqual(pstrOriginalFileName, pstrFileName))
    {
        // Remember that path was translated for Code Model's IVBCodeTypeLocation2.
        m_useAlternatePathForSpecialAssemblies = true;
    }
#else
    pstrFileName = m_pCompiler->AddString(wszFilePath);
#endif IDE
   
    // Get the referenced project.
    hr = m_pCompiler->CreateProjectWithMetaData(pstrFileName, bAssembly, GetCompilerHost(),
                                                &bCreatedNewMetadataProject,
                                                (ReferenceFlags & (REFERENCEFLAG_STANDARD | REFERENCEFLAG_REQUIRED)),
                                                &pProject);

    if (!pProject || FAILED(hr))
    {
        // the project is NULL, then we should have already generated the error, so just return.
        goto Error; // The error has been reported; fail silently.
    }

    if (!pProject->IsInitialized())
    {
        // 


        hr = HrMakeRepl(ERRID_BadMetaDataReference1, wszFilePath);
        goto Error; // The error has been reported; fail silently.
    }

    // Add the reference.
    Added = AddReference(pProject, ReferenceFlags);

#if IDE 
    if (m_useAlternatePathForSpecialAssemblies)
    {
        // Remember initial file path to use in Object Browser search.
        // This is useful in case it was changed by FixUpReferencePathForMultiTargeting.
        pProject->m_pstrAlternateBrowserSearchPath = pstrOriginalFileName;
    }

    if (Added)
    {
        if (bCreatedNewMetadataProject)
        {
            // Decompile metadata projects that might reference this metadata project to NoState
            EditFilter::DecompileReferencingMetadataProjectsToNoState(pProject);

            // If assembly resolves in target framework of this project, set assembly's target
            // framework so it can be used for meta-data reference disambiguation.
            if (this->GetHierarchy() && m_bstrTargetFrameworkMoniker.Length() > 0)
            {
                LPCWSTR wszFileNameOnly = PathFindFileName(pstrFileName);
                STRING * strPathInTargetFramework = GetCompilerPackage()->ResolveAssemblyPathInTargetFramework(
                    wszFileNameOnly, 
                    m_bstrTargetFrameworkMoniker);
                if (!CompareFilenames(pstrFileName, strPathInTargetFramework))
                {
                    pProject->SetTargetFrameworkMoniker(this->GetHierarchy());
                }
            }
        }

        if (m_fEnableVBReferenceChangedService)
        {
            EnsureVBReferenceChangedService();
            if (m_srpVBReferenceChangedService)
            {
                m_srpVBReferenceChangedService->ReferenceAdded(
                    m_pHierarchy,
                    CComBSTR(pProject->m_pstrFileName),
                    CComBSTR(pProject->m_AssemblyIdentity.GetAssemblyName()),
                    CComBSTR(pProject->m_AssemblyIdentity.GetAssemblyVersion()),
                    CComBSTR(pProject->m_AssemblyIdentity.GetAssemblyInfoString())
                );
            }
        }

    }
#endif IDE

    if (!Added && (ReferenceFlags != 0))
    {
        // The reference can be added more than once as both manual and standard.
        // Make sure the appropriate flags are set for this one.
        unsigned index;
        if (FindReferenceIndex(pProject, &index))
        {
            ReferencedProject &rProject = m_daReferences.Element(index);
            rProject.m_fManual |= ((ReferenceFlags & REFERENCEFLAG_MANUAL) != 0);
            rProject.m_fStandard |= ((ReferenceFlags & REFERENCEFLAG_STANDARD) != 0);

            // reference can be added more than once using /l and /r. Last one wins.
            rProject.m_fEmbedded = (ReferenceFlags & REFERENCEFLAG_EMBEDDED) != 0;
        }
   }

#if IDE

    if (!m_isExpressionEditorProject)
    {
        if ((ReferenceFlags & REFERENCEFLAG_EMBEDDED) != 0)
        {
            GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->AddEmbeddedMetaDataReference(
                GetProjectId(),
                CComBSTR(wszFilePath));
        }
        else
        {
            GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->AddMetaDataReference(
                GetProjectId(),
                CComBSTR(wszFilePath),
                bAssembly);
        }
    }

#endif IDE

    VB_EXIT_NORETURN();

Error:
#if DEBUG
    CompilerProject::VerifyNoDuplicateReferences();
#endif DEBUG

    if (pAdded != NULL)
    {
        *pAdded = Added;
    }

    RELEASE(pProject);
    RRETURN(hr);
}

#if HOSTED

//============================================================================
// Creates a project for Type Scope and Adds a reference to it.
//============================================================================

STDMETHODIMP CompilerProject::AddTypeScopeReference(CompilerProject **ppCreatedCompilerProject)
{
    // Note: Never call this method from VB code directly - call the Internal version.
    VB_VERIFY_NOT_REENTRANT();

    CheckInMainThread();
    return AddTypeScopeReferenceInternal(ppCreatedCompilerProject);
}

//============================================================================
// Creates a project for Type Scope and Adds a reference to it.
//============================================================================

HRESULT CompilerProject::AddTypeScopeReferenceInternal(CompilerProject **ppCreatedCompilerProject)
{
    CompilerProject *pProject = NULL;

    VB_ENTRY();

    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();

    STRING *pstrFileName;
    bool bCreatedNewMetadataProject;
    bool Added;

    pstrFileName = m_pCompiler->AddString(L"TypeScopeProject");

    // Get the referenced project.
    hr = m_pCompiler->CreateTypeScopeProject(pstrFileName, GetCompilerHost(),
                                                &bCreatedNewMetadataProject,
                                                &pProject);

    if (!pProject || FAILED(hr))
    {
        // the project is NULL, then we should have already generated the error, so just return.
        goto Error; // The error has been reported; fail silently.
    }

    if (!pProject->IsInitialized())
    {
        // 


        hr = HrMakeRepl(ERRID_BadMetaDataReference1, pstrFileName);
        goto Error; // The error has been reported; fail silently.
    }

    // Add the reference.
    Added = AddReference(pProject);
    *ppCreatedCompilerProject = pProject;

    VB_EXIT_NORETURN();

Error:
#if DEBUG
    CompilerProject::VerifyNoDuplicateReferences();
#endif DEBUG

    RELEASE(pProject);
    RRETURN(hr);
}

STDMETHODIMP CompilerProject::AddTypeScopeFile()
{
    HRESULT hr = NOERROR;
    MetaDataFile* pFile = NULL;

    // Create CompilerFile
    IfFailGo(MetaDataFile::Create(this, L"TypeScopeFile", &pFile));

    pFile->m_cs = CS_NoState;
    m_dlFiles[CS_NoState].InsertLast(pFile);

Error:
    return hr;
}

//============================================================================
// Initializes a project to act as the context for Type Scope.
//============================================================================

void CompilerProject::InitForTypeScope
(
    CompilerHost *pCompilerHost
)
{
    CheckInMainThread();
    DebCheckNoBackgroundThreads(m_pCompiler);

    HRESULT hr = NOERROR;

    m_pstrFileName  = m_pCompiler->AddString(L"TypeScope");
    m_pCompilerHost = pCompilerHost;

    m_AssemblyIdentity.SetAssemblyIdentity(m_pstrFileName,
                                               m_pCompiler->AddString(L"1033"),
                                               NULL,
                                               0,
                                               0,
                                               0,
                                               0,
                                               0,
                                               0);

    // Make sure the project gets included in the list of projects to compile.
    GetCompilerHost()->DestroyProjectCompilationList();

    // Set the warning level to normal.
    m_WarningLevel = WARN_Regular;

    // Set up the projects.
    SetCompState(CS_NoState);

    // This is a metadata project.
    m_fMetaData = true;

    // Metadata projects are always ready to compile.
    m_fInitialized = true;

}

#endif

//============================================================================
// Removes a reference to a MetaData file.
//============================================================================

STDMETHODIMP CompilerProject::RemoveMetaDataReference
(
    LPCWSTR wszFileName
)
{
    // Note: Never call this method from VB code directly - call the Internal version.
    VB_VERIFY_NOT_REENTRANT();
    return RemoveMetaDataReferenceInternal( wszFileName );
}

STDMETHODIMP CompilerProject::RemoveMetaDataReferenceInternal
(
    LPCWSTR wszFileName
)
{
#if IDE 
    CheckInMainThread();
    bool Removed = false;

    VB_ENTRY1(E_FAIL);

    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();

    // Optimized for the more common case:
    // Background stopped even for cases when the reference may not be removed
    // because the more common case is finding a reference that can be removed.
    //
    BackgroundCompilerLock compilerLock(this);

    // Release the assembly importer so we don't hang on to the
    // files. See VS bug 153063. They will be recreated on demand.
    for (ULONG i = 0; i < m_daReferences.Count(); ++i)
    {
        CompilerProject *pCompilerProjectToRemove = m_daReferences.Element(i).m_pCompilerProject;

        if (pCompilerProjectToRemove->IsMetaData() &&
            (CompareFilenames(pCompilerProjectToRemove->GetFileName(), wszFileName) == 0 ||
             (UseAlternatePathForSpecialAssemblies() && 
              CompareFilenames(pCompilerProjectToRemove->GetAlternateBrowserSearchPath(), wszFileName) == 0)))
        {
            CComBSTR bstrFileName;
            CComBSTR bstrAssemblyName;
            CComBSTR bstrAssemblyVersion;
            CComBSTR bstrAssemblyInfo;
            if (m_fEnableVBReferenceChangedService)
            {
                bstrFileName = CComBSTR(pCompilerProjectToRemove->m_pstrFileName);
                bstrAssemblyName = CComBSTR(pCompilerProjectToRemove->m_AssemblyIdentity.GetAssemblyName());
                bstrAssemblyVersion = CComBSTR(pCompilerProjectToRemove->m_AssemblyIdentity.GetAssemblyVersion());
                bstrAssemblyInfo = CComBSTR(pCompilerProjectToRemove->m_AssemblyIdentity.GetAssemblyInfoString());
            }

            hr = RemoveOneReference(i, &Removed);

            if (Removed && m_fEnableVBReferenceChangedService)
            {
                EnsureVBReferenceChangedService();
                if (m_srpVBReferenceChangedService)
                {
                    m_srpVBReferenceChangedService->ReferenceRemoved(
                        m_pHierarchy, bstrFileName, bstrAssemblyName, bstrAssemblyVersion, bstrAssemblyInfo);
                }
            }

            break;
        }
    }

    if (!m_isExpressionEditorProject)
    {
        GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->RemoveMetaDataReference(
            GetProjectId(),
            CComBSTR(wszFileName));
    }

    VB_EXIT_NORETURN();

#if DEBUG
    CompilerProject::VerifyNoDuplicateReferences();
#endif DEBUG

    RRETURN( hr );
#else
    RRETURN( E_NOTIMPL );
#endif IDE
}

//============================================================================
// Removes one reference, P-2-P or file.
//============================================================================
STDMETHODIMP CompilerProject::RemoveOneReference
(
    ULONG ReferenceIndex,
    bool *pRemoved
)
{
#if IDE 
    VB_ENTRY1(E_FAIL);
    *pRemoved = false;

    if (ReferenceIndex >= m_daReferences.Count())
    {
        return E_INVALIDARG;
    }

    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();

    ReferencedProject *pReferencedProjectToRemove = &m_daReferences.Element(ReferenceIndex);
    CompilerProject *pReferenceToRemove = pReferencedProjectToRemove->m_pCompilerProject;
    bool isEmbedded = pReferencedProjectToRemove->m_fEmbedded;

    if (pReferencedProjectToRemove)
    {
        if (!pReferencedProjectToRemove->m_fStandard && !pReferencedProjectToRemove->m_fRequired)
        {
            BackgroundCompilerLock compilerLock(this);

            CObjectBrowserLibraryMan * pLibMan = GetCompilerPackage()->AliasObjectBrowserLibraryManager();

            if (pLibMan)
            {
                CAssemblyReferenceManager *pAsmLibMgr = pLibMan->GetAssemblyReferenceManager();

                if (pAsmLibMgr)
                {
                    pAsmLibMgr->RemoveReference(pReferenceToRemove);
                }
            }


            if (GetCompilerPackage()->GetIntellidocLoadAndIndexService())
            {
                GetCompilerPackage()->GetIntellidocLoadAndIndexService()->RemoveIntellidocFromCache(pReferenceToRemove);
            }

            // Removing references can change the order of compilation.
            GetCompilerHost()->DestroyProjectCompilationList();
            pReferenceToRemove->RemoveReferenceOnMe(this);

            EditFilter::DecompileProjectNoCommit(this, CS_Declared);
            EditFilter::DecompileReferencingProjectsNoCommit(this, CS_Declared, CS_Declared);

            if (pReferenceToRemove->IsMetaData() && pReferenceToRemove->m_daReferencesOnMe.Count() == 0)
            {
                EditFilter::DecompileReferencingProjectsNoCommit(pReferenceToRemove, CS_MAX, CS_Declared);
            }

            if (!GetCompilerPackage()->IsInBatch())
            {
                GetCompilerPackage()->CommitEditListDemotions(true, false, this);
            }

            m_daReferences.Remove(ReferenceIndex);
            RELEASE(pReferenceToRemove);

            // Dev10 #735384
            if (isEmbedded)
            {
                // Recalculate PotentiallyEmbedsPiaTypes flag
                AssertIfTrue(IsMetaData());
                SetPotentiallyEmbedsPiaTypes(false);                
                
                unsigned count = m_daReferences.Count();
                for( unsigned i=0; i < count; i++)
                {
                    if (m_daReferences.Element(i).m_fEmbedded)
                    {
                        SetPotentiallyEmbedsPiaTypes(true);
                        break;
                    }
                }
            }

            // Some solution extensions are dependent upon certain references
            CheckSolutionExtensions();

            *pRemoved = true;
            hr = S_OK;
        }
        else
        {
            // If we were trying to remove a standard reference, then we will not do it, but we should
            // change its state to be not manual.
            if (pReferencedProjectToRemove->m_fStandard)
            {
                pReferencedProjectToRemove->m_fManual = false;
            }

            hr = S_FALSE;
        }
    }

    VB_EXIT_NORETURN();

#if DEBUG
    CompilerProject::VerifyNoDuplicateReferences();
#endif DEBUG

    RRETURN( hr );
#else
    RRETURN( E_NOTIMPL );
#endif
}

//============================================================================
// Removes all P-2-P and MetaData references.
//============================================================================

STDMETHODIMP CompilerProject::RemoveAllReferences()
{
     // Note: Never call this method from VB code directly - call the Internal version.
    VB_VERIFY_NOT_REENTRANT();
    return RemoveAllReferencesInternal();
}

STDMETHODIMP CompilerProject::RemoveAllReferencesInternal()
{
#if IDE 
    CheckInMainThread();

    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();

    VSASSERT(!GetCompilerPackage()->IsInBatch(), "Can't be in a batch edit");

    VB_ENTRY();
    BackgroundCompilerLock compilerLock(this);

    for (ULONG i = m_daReferences.Count() - 1; i >= 0 && SUCCEEDED(hr); --i)
    {
        bool Removed = false;
        hr = CompilerProject::RemoveOneReference(i, &Removed);
    }

    if (!m_isExpressionEditorProject)
    {
        GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->RemoveAllReferences(
            GetProjectId());
    }

    VB_EXIT_NORETURN();

    RRETURN( hr );
#else
    RRETURN( E_NOTIMPL );
#endif
}

//============================================================================
// Releases the MetaData CompilerProject.  Used when juggling a project's references
// (e.g., DeleteAllMetaDataReferences during building, and when CopyLocal notifications
// are received).
//============================================================================
#if IDE 
void CompilerProject::ReleaseMetaDataProject()
{
    VSASSERT(IsMetaData(), "Must be MetaData project.");
    VSASSERT(this != GetCompilerHost()->GetComPlusProject() &&
             !this->IsDefaultVBRuntime(),
             "Cannot release COM+ or VB Runtime.");

    m_mdassembly = mdAssemblyNil;

    // Dev10 #735384
    SetPotentiallyEmbedsPiaTypes(false);

    // Step #1: Drop the assembly importer on the file
    ReleaseAssemblyImport();

    // Step #2: Make each individual file importer let go
    m_AssemblyIdentity.ResetAssemblyIdentity(m_pCompiler, m_pCompilerHost, this);

    // Delete all metadata files becasue we want to recreate the
    // Metadata importer later.
    for (unsigned int cs = CS_NoState; cs < CS_MAX; cs++)
    {
        CDoubleListForwardIter<CompilerFile> iter(&m_dlFiles[cs]);

        while (CompilerFile *pCompilerFile = iter.Next())
        {
            delete pCompilerFile;
        }
    }

    m_pfileList = NULL;
    m_CheckedAllMetadataFilesForAccessibility = false; // when we rebuild this project we'll have to take a fresh look at the metadatafiles and their accessibility attributes
}
#endif IDE


//============================================================================
// Add a namespace import
//============================================================================

STDMETHODIMP CompilerProject::AddImport
(
    LPCWSTR wszNameSpace
)
{
    // Note: Never call this method from VB code directly - call the Internal version.
    VB_VERIFY_NOT_REENTRANT();
    return AddImportInternal( wszNameSpace );
}

STDMETHODIMP CompilerProject::AddImportInternal
(
    LPCWSTR wszNameSpace
)
{
    VSASSERT(wszNameSpace, "Bad param!");

    ImportedTarget * pitImport = NULL;

    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();

    VB_ENTRY();
#if IDE 
    // Stop the compiler.
    BackgroundCompilerLock compilerLock(this);
#endif IDE

    m_Imports.AddImport(wszNameSpace);

#if IDE 
    // Decompiled the project to declared state.
    EditFilter::DecompileProject(this, CS_Declared);

    if (!m_isExpressionEditorProject)
    {   
        GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->AddImport(
            GetProjectId(),
            CComBSTR(wszNameSpace));
    }

#endif // IDE

    VB_EXIT_GO();
    return hr;

Error:
    if(pitImport)
        delete pitImport;

    return hr;
}

////============================================================================
//// Remove a namespace import
////============================================================================
//STDMETHODIMP CompilerProject::GetImports
//(
//    LPCWSTR *wszNameSpace
//)
//{
//
//}

//============================================================================
// Remove a namespace import
//============================================================================
STDMETHODIMP CompilerProject::IsImported
(
    LPCWSTR wszNameSpace,
    bool *pIsImported
)
{
    CheckInMainThread();
    VSASSERT(wszNameSpace, "Bad param!");
    *pIsImported= false;

    VB_ENTRY();

    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();

    *pIsImported = m_Imports.IsImported(wszNameSpace);
    VB_EXIT();
}




//============================================================================
// Remove a namespace import
//============================================================================

STDMETHODIMP CompilerProject::DeleteImport
(
    LPCWSTR wszNameSpace
)
{
    // Note: Never call this method from VB code directly - call the Internal version.
    VB_VERIFY_NOT_REENTRANT();
    return DeleteImportInternal( wszNameSpace );
}

STDMETHODIMP CompilerProject::DeleteImportInternal
(
    LPCWSTR wszNameSpace
)
{
    CheckInMainThread();
    VSASSERT(wszNameSpace, "Bad param!");

    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();

    VB_ENTRY();
#if IDE 
    // Stop the compiler
    BackgroundCompilerLock compilerLock(this);
#endif IDE

    m_Imports.DeleteImport(wszNameSpace);

#if IDE 
    // Decompiled the project to declared state.
    EditFilter::DecompileProject(this, CS_Declared);

    if (!m_isExpressionEditorProject)
    {
        GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->DeleteImport(
            GetProjectId(),
            CComBSTR(wszNameSpace));
    }

#endif

    VB_EXIT();
}


//============================================================================
// Remove a namespace import
//============================================================================

STDMETHODIMP CompilerProject::DeleteAllImports()
{
    // Note: Never call this method from VB code directly - call the Internal version.
    VB_VERIFY_NOT_REENTRANT();
    return DeleteAllImportsInternal();
}

STDMETHODIMP CompilerProject::DeleteAllImportsInternal()
{
    CheckInMainThread();
    VB_ENTRY();
#if IDE 
    // Stop the compiler
    BackgroundCompilerLock compilerLock(this);
#endif IDE

    m_Imports.DeleteAllImports();

#if IDE 
    // Decompiled the project to declared state.
    EditFilter::DecompileProject(this, CS_Declared);

    if (!m_isExpressionEditorProject)
    {
        GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->DeleteAllImports(
            GetProjectId());
    }

#endif

    VB_EXIT();
}

//============================================================================
// Add a reference to a resource file
//============================================================================
STDMETHODIMP CompilerProject::AddResourceReference
(
    LPCWSTR wszFileLocation,
    LPCWSTR wszName,
    BOOL    fPublic,
    BOOL    fEmbed
)
{
    VB_ENTRY();
    CheckInMainThread();

    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();

#if IDE 
    BackgroundCompilerLock compilerLock(this);
#endif IDE

#if (IDE ) && ID_TEST
    if (VBFTESTSWITCH(fDumpFileChangeNotifications))
    {
        WCHAR wszFile[MAX_PATH];
        DebPrintf("CompilerProject::AddResourceReference(%S)\n",
            GetCanonicalFileName(wszFileLocation, wszFile, DIM(wszFile)));
    }
#endif // IDE && ID_TEST

    Resource *presource;

    // The new resource is added to the end of the dynamic array m_daResources.
    // Note that this is assumed by MetaEmit::AlinkEmitResources.
    presource = &m_daResources.Add();

    presource->m_pstrFile        = m_pCompiler->AddString(wszFileLocation);
    presource->m_pstrName        = m_pCompiler->AddString(wszName);
    presource->m_fPublic         = fPublic;
    presource->m_fEmbed          = fEmbed;

#if IDE 
    // Resources are written only in the second half of the IDE
    // compilation i.e. when writing the image to disk. So if
    // the image has not yet been written to disk, then there is
    // no decompilation to be done.
    //
    if (!fEmbed ||                      // Cannot delay emit linked resources
        m_fEmbeddedResourcesEmitted)    // Embedded resource already emitted, so need to
                                        // decompile for the new embedded resource to be
                                        // emitted.
    {
        // Decompile to regenerate the embedded/linked resource files
        EditFilter::DecompileProject(this, CS_DecompileOnResourceChange);
    }

    // Watch this file for future changes
    IfFailThrow(GetCompiler()->WatchFile(presource->m_pstrFile));
    if (!m_isExpressionEditorProject)
    {
        GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->AddResourceReference(
            GetProjectId(),
            CComBSTR(wszFileLocation),
            CComBSTR(wszName),
            fPublic,
            fEmbed);
    }

#endif IDE

    VB_EXIT();
}


//============================================================================
// Removes all resource file references.
// 



STDMETHODIMP CompilerProject::DeleteAllResourceReferences()
{
#if IDE 
    CheckInMainThread();
    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();

    VSASSERT(GetCompilerPackage()->IsInBatch(),
              "We have to be in batch mode for this to work");

#if ID_TEST
    if (VBFTESTSWITCH(fDumpFileChangeNotifications))
    {
        DebPrintf("CompilerProject::DeleteAllResourceReferences\n");
    }
#endif

    // Only decompile if we have resources to delete.
    if (m_daResources.Count())
    {
        // Decompile to regenerate the embedded/linked resource files
        EditFilter::DecompileProject(this, CS_DecompileOnResourceChange);

        // Unwatch and release all of this project's resource files
        UnwatchAllResourceFiles();
        m_daResources.Reset();
    }

#if IDE

    if (!m_isExpressionEditorProject)
    {
        GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->DeleteAllResourceReferences(
            GetProjectId());
    }

#endif

    return NOERROR;
#else
    VSFAIL("How did we get here?");
    return E_NOTIMPL;
#endif
}


//============================================================================
// Unwatches all resource files associated with this project.
//============================================================================
#if IDE 
void CompilerProject::UnwatchAllResourceFiles()
{
    Resource *prgResource = m_daResources.Array();
    ULONG count = m_daResources.Count();

    for (ULONG i = 0; i < count; i++)
    {
        IfFailThrow(GetCompiler()->UnwatchFile(prgResource[i].m_pstrFile));
    }
}
#endif IDE


//============================================================================
// Asks for file change notifications to be sent if any of this project's
// (embedded or linked) resource files have changed on disk.
//============================================================================
#if IDE 
void CompilerProject::SyncAllResourceFiles()
{
    Resource *prgResource = m_daResources.Array();
    ULONG count = m_daResources.Count();

    for (ULONG i = 0; i < count; i++)
    {
        IfFailThrow(GetCompilerPackage()->SyncFile(prgResource[i].m_pstrFile));
    }
}
#endif IDE

//============================================================================
// Notification that a "build" is starting. Since the compiler
//  is always running in the background, this might not mean
//  anything more than to disable some UI.
//============================================================================

STDMETHODIMP CompilerProject::StartBuild(IVsOutputWindowPane* pVsOutputWindowPane, BOOL fRebuildAll)
{
#if !IDE
    return E_NOTIMPL;
#else IDE
    CheckInMainThread();

    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();

    VB_ENTRY();

    // This check used to happen in StartProjectCompile but in a Rebuild scenario
    // the DecompileProject code below will first stop the bg compile which aborts the 
    // CompileAllProjects required by a WaitUntilState call. So we must check before 
    // doing any bg compiler stops.
    if (GetCompilerSharedState()->IsInWaitUntilState())
    {
        // This indicates we're in a WaitUntilState and we've processed a request
        // from the project system to compile a single project. But we cant do this
        // until the WaitUntilState call completes.
        return E_PENDING;
    }

    // If fRebuildAll is true, then we want to force recompilation
    // of everything -- decompile the entire project to NoState.
    if (fRebuildAll)
    {
        EditFilter::DecompileProject(this, CS_NoState);
    }

    hr = GetCompilerPackage()->StartProjectCompile(this);

    if (!m_isExpressionEditorProject)
    {
        GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->StartBuild(
            GetProjectId(),
            pVsOutputWindowPane,
            fRebuildAll);
    }

    VB_EXIT();

#endif IDE
}

//============================================================================
// This is called if the user wishes to stop a "build". Since the
//  compiler will always be running, its only effect might be to
//  re-enable UI that is disabled during a build.
//============================================================================

STDMETHODIMP CompilerProject::StopBuild()
{
#if !IDE
    return E_NOTIMPL;
#else IDE
    CheckInMainThread();

    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();

    VB_ENTRY();
    GetCompilerPackage()->EndProjectCompilation(CompilerState_Aborted);

    if (!m_isExpressionEditorProject)
    {
        GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->StopBuild(
            GetProjectId());
    }

    VB_EXIT();

    return NOERROR;
#endif IDE
}

//============================================================================
// Disconnects from the project and event source, etc.
//============================================================================
//. Note: When a UI project is closed, this function is called
//      Call of this function means: I do not need you any more. You may go away.
STDMETHODIMP_(void) CompilerProject::Disconnect()
{
    CheckInMainThread();

    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();

    VB_ENTRY();
    if (m_punkProject != NULL)
    {
        // Do not Release. We didn't AddRef.
        m_punkProject = NULL;

        // Unregister with the project manager.
#if !IDE
        GetCompilerHost()->RemoveProject(this);
#else IDE
        DisconnectInternal();
#endif IDE
    }
    VB_EXIT_NORETURN();
}

#if IDE 
void CompilerProject::DisconnectInternal()
{
    BackgroundCompilerLock compilerLock(this);
    GetCompilerHost()->DestroyProjectCompilationList();

    if (!m_isExpressionEditorProject)
    {
        // Normally we forward project system events on to the out-of-process
        // workspace after the language service does all of its work. In this
        // case we need to forward the event while we can still get the hierarchy.
        GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->Disconnect(
            GetProjectId());
    }

    //Invalidate all the sourcefiles for this project
    SourceFileIterator sourcefiles(this);
    SourceFile *pSourceFile;
    while ((pSourceFile = sourcefiles.Next()) != NULL)
    {
        pSourceFile->Invalidate();
    }

    // Release the CodeModel Object.
    if (m_pCodeModel)
    {
        m_pCodeModel->Invalidate();
        m_pCodeModel->Release();
        m_pCodeModel = NULL;
    }

    if( m_pSinkSite )
    {
        m_pSinkSite->Zombie();
        // m_pSinkSite might be null since zombie sends a close
        // event which might end up in the sink site being released
        // and in it's dtor it sets this value equal to NULL.
        // do it again here just to be sure.
        m_pSinkSite = NULL;
    }

    if (NULL != m_pHierarchy)
    {
        //Remove the Hierarchy from the CompilerPackage's cache since it's going away.
        GetCompilerPackage()->RemoveCachedHierarchy(m_pHierarchy);

        m_pHierarchy->Release();
        m_pHierarchy = NULL;
    }

    if (NULL != m_pContextualIntellisenseFilter)
    {
        m_pContextualIntellisenseFilter->Release();
        m_pContextualIntellisenseFilter = NULL;
    }

    m_XmlSchemas.Close();

    // [#201599] Remove all tasks for this project immediately on disconnect
    if (m_pTaskProvider)
    {
        m_pTaskProvider->UnregisterTaskProvider();
        RELEASE(m_pTaskProvider);
    }

    // tell listeners we are going away
    FireOnClose();

    // One project being removed
    //
    if (!GetCompilerPackage()->IsSolutionClosing())
    {
        // Decompile whole world (except mscorlib.dll) including VB runtime to CS_NoState.
        // The whole world needs to be decompiled because some source projects might
        // indirectly depend on metadata references of the current project which is
        // being removed through other metadata references.
        //
        EditFilter::DecompileReferencingProjects(this, CS_Declared, CS_Declared);
    }
    else
    {
        // Close solution, i.e. all projects going away. Of the special metadata references
        // mscorlib and vb runtime that will still be kept until devenv is shutdown, the VB
        // runtime needs to be decompiled to CS_NoState in case it depends on other DLLs like
        // System.DLL (it already depends on System.DLL today) that went away when the source
        // projects referencing them went away.
        //
        if (m_pVBRuntimeProject && m_pVBRuntimeProject->GetCompState() > CS_NoState)
        {
            m_pVBRuntimeProject->_DemoteMetaDataProjectToNoState(false);    // Don't Release the IMetaDataImporter.
        }

        // Bug fix for Dev10 SP1 67634
        // We need to demote the default vb runtime on host, otherwise the default vb runtime could be 
        // in a bad state, such that it is in the compiled state but its references are not resolved.

        CompilerProject *pDefaultVBRuntimeProjectOnHost = m_pCompilerHost->GetDefaultVBRuntimeProject();
        if (pDefaultVBRuntimeProjectOnHost && 
            pDefaultVBRuntimeProjectOnHost != m_pVBRuntimeProject &&
            pDefaultVBRuntimeProjectOnHost->GetCompState() > CS_NoState)
        {
            pDefaultVBRuntimeProjectOnHost->_DemoteMetaDataProjectToNoState(false);
        }        
    }

    GetCompilerHost()->RemoveProject(this);
}
#endif // IDE 

#if IDE 
void CompilerProject::FireOnClose()
{
    IVbProjectEvents **rgArray = m_daFileInsertionEventListeners.Array();
    int iListener;
    int cListeners = m_daFileInsertionEventListeners.Count();

    if (cListeners == 0)
        return;
    // when told to close
    // we go backwards because the listeners will probably remove themselves
    for (iListener = cListeners - 1; iListener >= 0; iListener--)
    {
        rgArray[iListener]->OnClose();
    }


}
void CompilerProject::FireOnFile(VBCOLLECTION vbc, ICodeFile *picf)
{
    IVbProjectEvents **rgArray = m_daFileInsertionEventListeners.Array();
    int iListener;
    int cListeners = m_daFileInsertionEventListeners.Count();

    if (cListeners == 0)
        return;
    for (iListener = cListeners - 1; iListener >= 0; iListener--)
    {
        rgArray[iListener]->OnFile(vbc, picf);
    }

}
void CompilerProject::FireOnChange(BOOL f, VBPROJECTNOTIFY vbprjn)
{
    IVbProjectEvents **rgArray = m_daFileInsertionEventListeners.Array();
    int iListener;
    int cListeners = m_daFileInsertionEventListeners.Count();


    if (cListeners == 0)
        return;
    for (iListener = cListeners - 1; iListener >= 0; iListener--)
    {
        rgArray[iListener]->OnChange(f, vbprjn);
    }

}
#endif
//=============================================================================
// Helper to sort BSTRs
//=============================================================================

int _cdecl CompilerProject::SortBSTRs
(
    const void *arg1,
    const void *arg2
)
{
    PCWSTR bstr1 = (*(PCWSTR *)arg1);
    PCWSTR bstr2 = (*(PCWSTR *)arg2);

    return CompareNoCase(bstr1, bstr2);
}

//============================================================================
// Determine if this is a Form class, in which case it could have a
// synthetic sub Main
//============================================================================
bool CompilerProject::CouldHaveSyntheticSubMain
(
    BCSYM_Class *pClass,
    Compiler *pCompiler
)
{
    BCSYM_Class *CurrentCandidate = pClass;

    while(!CurrentCandidate->IsObject())
    {
        if (StringPool::IsEqual(STRING_CONST(pCompiler, SystemWindowsForms), CurrentCandidate->GetNameSpace()) &&
            StringPool::IsEqual(STRING_CONST(pCompiler, Form), CurrentCandidate->GetName()))
        {
            return true;
        }

        BCSYM *BaseClass = CurrentCandidate->GetBaseClass();
        if (BaseClass && BaseClass->IsClass())
        {
            CurrentCandidate = BaseClass->PClass();
        }
        else
        {
            break;              // Bad Base class found, bail out
        }
    }

    return false;
}

//============================================================================
// Determines if this class is visible from the outside world.
//============================================================================
bool CompilerProject::IsClassVisible
(
    BCSYM_NamedRoot *pClass
)
{
    while (pClass && pClass->GetKind() == SYM_Class)
    {
        if (pClass->GetAccess() == ACCESS_Private)
        {
            return false;
        }

        pClass = pClass->GetParent();
    }

    return true;
}

/*****************************************************************************
;FindAllMainsInAClass

Builds an array of Sub Mains that are in the supplied class. The array will
contain any method (Sub or Function) which is called "Main". Some of these
methods may be valid entry points, other may not. The reason we gather all
Mains is that the caller may want to take that list and generate error messages.
The caller can also choose to allow or dis-allow digging into base classes.

Valid Entry Points are:

    Sub Main()
    Sub Main(ByVal args() As String)
    Function Main() As Integer
    Function Main(ByVal args() As String) As Integer

*****************************************************************************/
void CompilerProject::FindAllMainsInAClass
(
    BCSYM_Class *ClassToCheck,                  // [in] class to look for Sub Mains in.
    DynamicArray<SubMainInfo> *SubMainList,     // [in/out] Array or Sub Mains found in this class
    bool        AllowDiggingIntoBaseClasses     // [in] Are we allowed to check base classes or not?
)
{
    // if the inherited class has a shadowing main,
    // we don't dig into any base classes.

    VB_ENTRY();
    if (ClassToCheck && !ClassToCheck->IsBad() && ClassToCheck->GetContainingClass()->GetKind() == SYM_Class)
    {
        // If class is private, don't look into it. Stop right here.
        if (!IsClassVisible(ClassToCheck))
        {
            return;
        }

        // Look for a "Sub Main" that can be used as an entry point.
        BCSYM_NamedRoot *CandidateMain = ClassToCheck->SimpleBind(NULL, STRING_CONST(m_pCompiler, Main)); // Has a "main"?

        // Iterate over all main procs found
        while (CandidateMain)
        {
            if (CandidateMain->DigThroughAlias()->IsMethodImpl() || CandidateMain->DigThroughAlias()->IsSyntheticMethod())
            {
                // If we found a synthetic Sub Main but the class isn't a form, then ignore it.
                if (CandidateMain->IsSyntheticMethod() &&
                    CandidateMain->PSyntheticMethod()->GetSyntheticKind() == SYNTH_FormMain &&
                    !SeeIfWindowsFormClass(CandidateMain->PNamedRoot()->GetContainingClass(), m_pCompiler))
                {
                    CandidateMain = CandidateMain->GetNextOfSameName();        // Get next Sub Main
                    continue;
                }

                BCSYM_Proc *CandidateMainProc = CandidateMain->DigThroughAlias()->PProc();
                SubMainInfo SubMainInfoEntry;

                bool IsAccessible       = false;       // Accessibility of Sub Main (Private, Public, etc.)
                bool IsValidParams      = false;       // Valid params are either none or "ByVal args() As String"
                bool IsValidReturnType  = true;        // Whether or not Main returns nothing or an Integer (the only 2 valid cases)

                // Set all Main entry fields, and add it to the array of Mains.
                SubMainInfoEntry.m_SubMainSymbol    = CandidateMainProc;
                SubMainInfoEntry.m_ParentSymbol     = CandidateMainProc->GetParent();

                if (ClassToCheck->IsStdModule())
                {
                    IsAccessible = CandidateMainProc->GetAccess() > ACCESS_Private;
                }
                else
                {
                    IsAccessible = CandidateMainProc->IsShared() && CandidateMainProc->GetAccess() > ACCESS_Private;
                }

                BCSYM_Param *SubMainParam = CandidateMainProc->GetFirstParam();

                // Zero params is valid for Sub Main
                if (SubMainParam)
                {
                    // One array of strings param is also vaild entry for Sub Main
                    if (!SubMainParam->GetNext())
                    {
                        VSASSERT(SubMainParam->GetType(), "Param Symbol for Sub Main(...) is horked!");

                        if (SubMainParam->GetType()->IsArrayType() &&
                            SubMainParam->GetType()->PArrayType()->GetRank() == 1 &&
                            SubMainParam->GetType()->PArrayType()->GetRoot()->GetVtype() == t_string)
                        {
                            IsValidParams = true;
                        }
                    }
                }
                else
                {
                    IsValidParams = true;
                }

                // Function Main() can only return an Integer for it to be a valid Entry point
                if (CandidateMainProc->GetType() && CandidateMainProc->GetType()->GetVtype() != t_i4)
                {
                    IsValidReturnType = false;
                }

                SubMainInfoEntry.m_IsValidEntryPoint = IsAccessible && IsValidReturnType && IsValidParams;

                if (SubMainInfoEntry.m_IsValidEntryPoint &&
                    IsGenericOrHasGenericParent(CandidateMainProc))
                {
                    // Generic methods or methods in generic types are not valid entry points
                    //
                    SubMainInfoEntry.m_IsValidEntryPoint = false;

                    // badness only due to genericness
                    //
                    SubMainInfoEntry.m_IsInvalidOnlyDueToGenericness = true;
                }
                else
                {
                    SubMainInfoEntry.m_IsInvalidOnlyDueToGenericness = false;
                }

                // If this proc is shadowing, then we don't need to look into base classes. Note that we need
                // to consider both shadowing by name and shadowing by signature
                if (CandidateMainProc->IsShadowing() &&
                   (!CandidateMainProc->IsOverloadsKeywordUsed() ||
                   (IsValidParams && IsValidReturnType)))
                {
                    AllowDiggingIntoBaseClasses = false;
                }

                // This is actually safe becasue we are doing a bit-wise copy of the struct inside AddElement()
                SubMainList->AddElement(SubMainInfoEntry);
            }
            else
            {
                // If main is not a proc, we still want to make sure that we are not digging into base classes
                // of the name "main" is shadows in this class
                if (CandidateMain->DigThroughAlias()->IsNamedRoot() && CandidateMain->DigThroughAlias()->PNamedRoot()->IsShadowing())
                {
                    AllowDiggingIntoBaseClasses = false;
                }
            }

            CandidateMain = CandidateMain->GetNextOfSameName();        // Get next Sub Main
        }

        // Check base classes for Sub Main, if we need to.
        if (AllowDiggingIntoBaseClasses)
        {
            BCSYM *BaseClass = ClassToCheck->PClass()->GetBaseClass();

            if (BaseClass && BaseClass->IsClass() && !BaseClass->IsBad())
            {
                FindAllMainsInAClass(BaseClass->PClass(), SubMainList, true);
            }
        }
    }

    VB_EXIT_NORETURN();
}

/*****************************************************************************
;FindClassMain

Find Sub Main in a particular class. If an error table is provided, this
function will generate an error message if the StartupString could not be
resolved.

Returns: True if we succesed in finding the class, or false if we didn't
*****************************************************************************/
bool CompilerProject::FindClassMain
(
    _In_z_ STRING *StartupString,                    // [in] The startup class qualified name
    DynamicArray<SubMainInfo> *SubMainList,   // [in/out] Array of Sub Mains found
    ErrorTable *pErrorTable                   // [in] Error table to use to report errors (can be NULL)
)
{
    VB_ENTRY();
    ULONG ulItems = 0;
    STRING **rgpstrEntryPoint = NULL;
    BCSYM_Class *ClassToCheck = NULL;
    NorlsAllocator nraSymbols(NORLSLOC);

    bool fBadName = false;
    Location Loc;
    Loc.Invalidate();
    BCSYM_NamedRoot *pBoundSymbol;

    IfFailThrow( BreakUpQualifiedName(m_pCompiler, &rgpstrEntryPoint, &nraSymbols, &ulItems, StartupString, DOT_DELIMITER));

    if (!ulItems || !rgpstrEntryPoint || !m_pCompiler->GetUnnamedNamespace(this))
    {
        if (pErrorTable)
        {
            pErrorTable->CreateError(ERRID_StartupCodeNotFound1, NULL, StartupString);
        }

        return false;
    }

    pBoundSymbol =
        Semantics::EnsureNamedRoot
        (
            Semantics::InterpretQualifiedName
            (
                rgpstrEntryPoint,
                ulItems,
                NULL,
                NULL,
                m_pCompiler->GetUnnamedNamespace(this)->GetHash(),
                NameSearchIgnoreImports | NameSearchIgnoreExtensionMethods,
                Loc,
                NULL,
                m_pCompiler,
                m_pCompilerHost,
                NULL, // Compilation Caches
                NULL,
                true,           // perform obsolete checks
                fBadName
            )
        );


    if (fBadName || !pBoundSymbol)
    {
        if (pErrorTable)
        {
            pErrorTable->CreateError(ERRID_StartupCodeNotFound1, NULL, StartupString);
        }

        return false;
    }

    // We can't use IsClass() because BCSYM_Delegate is also a class, but it can't be the startup Main
    if (pBoundSymbol && pBoundSymbol->GetKind() == SYM_Class)
    {
        ClassToCheck = pBoundSymbol->PClass();
    }
    else
    {
        if (pErrorTable)
        {
            pErrorTable->CreateError(ERRID_StartupCodeNotFound1, NULL, StartupString);
        }

        return false;
    }

    // error checking
    if (!ClassToCheck || ClassToCheck->IsBad())
    {
        return false;
    }

    FindAllMainsInAClass(ClassToCheck, SubMainList, true);

    VB_EXIT_NORETURN();

    return true;
}

/*****************************************************************************
;FindAllSubMainInProject

Find Sub Main in the project
*****************************************************************************/
void CompilerProject::FindAllSubMainInProject
(
    DynamicArray<SubMainInfo> *SubMainList,  // [in/out] Array or Sub Mains found
    bool IncludeWinFormMains
)
{
    BCSYM_Proc* pMainProc = NULL;            // will point to the main BCSYM when we find one
    BCSYM_Proc* pGoodMainProc = NULL;        // This is the real main that we use if we only find one

    VB_ENTRY();
    // Haul through the list of files in this project
    CompilerFile *pFile;
    FileIterator fileiter(this);

    while (pFile = fileiter.Next())
    {
        AllContainersInFileIterator pNamespaceSymbolList(pFile);

        // Haul through the namespaces level symbols for this file.  There is an important assumption here: That the classes,
        // and their nested clases, embedded in the namespaces in the file, are all percolated out to be in this list.  If this
        // is not the case in the future, this code will need to change to walk the containers to examine the nested classes.
        // Presently, the nested namespaces don't show up in the containers which is fine since the classes inside those namespaces
        // are getting bumped out to be in the namespace symbol list.
        while (BCSYM_NamedRoot *pNamespaceSymbol = pNamespaceSymbolList.Next())
        {
            if (pNamespaceSymbol->DigThroughAlias()->IsClass() &&
                !pNamespaceSymbol->DigThroughAlias()->PClass()->IsEnum() &&
                !pNamespaceSymbol->DigThroughAlias()->PClass()->IsDelegate())
            {
                ULONG NumberOfMainsBeforeCheckingTheClass = SubMainList->Count();

                BCSYM_Class *ClassToCheck = pNamespaceSymbol->DigThroughAlias()->PClass();
                FindAllMainsInAClass(ClassToCheck, SubMainList, false);

                // If we are just trying to get the list of Sub Mains in the project, we need to consider
                // WindForm mains. Basicly, we need to fake the fact that this class has a valid Sub Main.
                // this is because when we are going to bindable, we generate the syntheitc main, but we need
                // to know that this form is the startup form first.
                if (IncludeWinFormMains && CouldHaveSyntheticSubMain(ClassToCheck, m_pCompiler))
                {
                    bool ShouldWeAddFormMain = true;
                    ULONG NumberOfMainsAfterCheckingTheClass = SubMainList->Count();

                    // Find a valid Sub Main.
                    for (unsigned int i = NumberOfMainsBeforeCheckingTheClass; i < NumberOfMainsAfterCheckingTheClass; ++i)
                    {
                        if (SubMainList->Array()[i].m_IsValidEntryPoint)
                        {
                            ShouldWeAddFormMain = false;
                        }
                    }

                    if (ShouldWeAddFormMain)
                    {
                        SubMainInfo SubMainInfoEntry;
                        SubMainInfoEntry.m_SubMainSymbol      = NULL;
                        SubMainInfoEntry.m_ParentSymbol       = ClassToCheck;
                        SubMainInfoEntry.m_IsValidEntryPoint  = true;
                        SubMainInfoEntry.m_IsInvalidOnlyDueToGenericness = false;

                        SubMainList->AddElement(SubMainInfoEntry);
                    }
                }
            }
        }
    }

    VB_EXIT_NORETURN();
}

//============================================================================
// Lists all classes with Sub Main marked as shared (entry points). If called
// with cItems = 0 and pcActualItems != NULL, GetEntryPointsList returns in
// pcActualItems the number of items available. When called with cItems != 0,
// GetEntryPointsList assumes that there is enough space in bstrList[] for that
// many items, and fills up the array with those items (up to maximum available).
// Returns in pcActualItems the actual number of items that could be put in the
// array (this can be > or < cItems). Assumes that the caller takes care of
// array allocation and de-allocation.
//============================================================================
STDMETHODIMP CompilerProject::GetEntryPointsList
(
    ULONG cItems,           // [in] Number of items to return
    BSTR bstrList[],        // [in out] This is where BSTRs are put in
    ULONG *pcActualItems    // [out] Actual number of items found
)
{
#if IDE 
    return GetEntryPointsList(false, cItems, bstrList, pcActualItems);
#else !IDE
    return NOERROR;
#endif !IDE
}

#if IDE 

// Lists all Form classes with an entry point. If called with cItems = 0 and
// pcActualItems != NULL, GetEntryPointsList returns in pcActualItems the number
// of items available. When called with cItems != 0, GetEntryPointsList assumes
// that there is enough space in strList[] for that many items, and fills up the
// array with those items (up to maximum available).  Returns in pcActualItems
// the actual number of items that could be put in the array (this can be > or
// < cItems). Assumes that the caller takes care of array allocation and de-allocation.
STDMETHODIMP CompilerProject::GetFormEntryPointsList
(
    ULONG cItems,           // [in] Number of items to return
    BSTR bstrList[],        // [in out] This is where BSTRs are put in
    ULONG *pcActualItems    // [out] Actual number of items found
)
{
    return GetEntryPointsList(true, cItems, bstrList, pcActualItems);
}

HRESULT CompilerProject::GetEntryPointsList
(
    bool fFormsOnly,        // [in] Form entry points only?
    ULONG cItems,           // [in] Number of items to return
    BSTR bstrList[],        // [in out] This is where BSTRs are put in
    ULONG *pcActualItems    // [out] Actual number of items found
)
{
    CheckInMainThread();

    ULONG actualCount = 0;                          // Number of BSTRs allocated
    *pcActualItems = 0;

    VB_ENTRY();
    if ((cItems == 0) && (pcActualItems == NULL))
    {
        return E_INVALIDARG;                          // Bad arguments
    }

    AssertIfFalse(cItems == 0 || bstrList);

    hr = CacheEntryPointsList();

    if (S_OK == hr)
    {
        // Find all valid Sub Mains.
        for (unsigned int i = 0; i < m_daEntryPoints.Count(); ++i)
        {
            const SubMainInfo &info = m_daEntryPoints.Array()[i];

            if (info.m_IsValidEntryPoint && (!fFormsOnly || CanFormClassHaveEntryPoint(info.m_ParentSymbol)))
            {
                ++(*pcActualItems);

                if (cItems > 0 && cItems > actualCount)
                {
                    IfNullGo(bstrList[actualCount] =
                        SysAllocString(info.m_ParentSymbol->GetQualifiedName()));

                    ++actualCount; // increment number added to list
                }
            }
        }
    }

    VB_EXIT_NORETURN();

Error:
    // Fall through expected
    // If failed then free-up all already allocated BSTRs before returning.
    if (actualCount > 0)
    {
        if (FAILED(hr))
        {
            do
            {
                SysFreeString(bstrList[--actualCount]);
            } while(actualCount);
        }
        else
        {
            // We want to sort the array of BSTRs so that the startup suite will be cool
            qsort(bstrList, actualCount, sizeof(BSTR), SortBSTRs);
        }
    }

    RRETURN(hr);
}

bool CompilerProject::CanFormClassHaveEntryPoint(BCSYM_NamedRoot *pNamed)
{
    return pNamed->IsClass() &&
           SeeIfWindowsFormClass(pNamed->PClass(), pNamed->GetCompiler()) &&
           pNamed->GetContainer() &&
           pNamed->GetContainer()->IsNamespace() &&
           Bindable::CanBeMyGroupCollectionMember(pNamed->PClass(), pNamed->GetCompiler());
}

HRESULT CompilerProject::CacheEntryPointsList()
{
    HRESULT hr = S_OK;

    if (!m_fCachedEntryPoints)
    {
        IfFailRet(WaitUntilBindableState(WaitStatePumping::PumpingAllowed, WaitStateCancel::CancelAllowed)); // Make sure there is something to search first

        FindAllSubMainInProject(&m_daEntryPoints,
                                true);       // Do include Microsoft in the list

        m_fCachedEntryPoints = true;
    }

    return hr;
}

#endif IDE

//============================================================================
// Between these calls, the project will notify the compiler of multiple
//  file changes. This is really just an optimization so the compiler
//  can interrupt the background compile thread once instead of having
//  to interrupt it every time the project adds a file to the list of
//  compiled things (which happens once for each file during project
//  load).
//============================================================================

STDMETHODIMP CompilerProject::StartEdit()
{
#if IDE 
    CheckInMainThread();

    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();

#if DEBUG
    CompilerProject::VerifyNoDuplicateReferences();
#endif DEBUG

    VB_ENTRY();
    IfFailGo(GetCompilerPackage()->StartBatchEdit());

    if (!m_isExpressionEditorProject)
    {
        GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->StartEdit(
            GetProjectId());
    }

    VB_EXIT_NORETURN();

Error:
    RRETURN( hr );
#else !IDE
    return NOERROR;
#endif !IDE
}

//============================================================================
// Between these calls, the project will notify the compiler of multiple
//  file changes. This is really just an optimization so the compiler
//  can interrupt the background compile thread once instead of having
//  to interrupt it every time the project adds a file to the list of
//  compiled things (which happens once for each file during project
//  load).
//  Using this will also cause the whole project to decompile
//============================================================================

STDMETHODIMP CompilerProject::FinishEdit()
{
#if IDE 
    CheckInMainThread();

    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();

    VB_ENTRY();
#if DEBUG
    CompilerProject::VerifyNoDuplicateReferences();
#endif DEBUG

    IfFailGo(GetCompilerPackage()->EndBatchEdit(this));

    if (!m_isExpressionEditorProject)
    {
        GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->FinishEdit(
            GetProjectId());
    }

    VB_EXIT_NORETURN();

Error:
    RRETURN( hr );
#else !IDE
    return NOERROR;
#endif !IDE
}

//============================================================================
// Keep track of the interface where we send build status to.
//============================================================================

STDMETHODIMP CompilerProject::AdviseBuildStatusCallback
(
    IVbBuildStatusCallback *pIVbBuildStatusCallback,
    DWORD *pdwCookie
)
{
#if IDE 
    CheckInMainThread();
    VB_ENTRY();
    VSASSERT(m_pBuildStatusCallBack == NULL, "Already set.");

    m_pBuildStatusCallBack = pIVbBuildStatusCallback;
    m_pBuildStatusCallBack->AddRef();

    VB_EXIT();

#endif IDE

    *pdwCookie = 0;

    return NOERROR;
}

//============================================================================
// Don't give status any more.
//============================================================================

STDMETHODIMP CompilerProject::UnadviseBuildStatusCallback
(
    DWORD dwCookie
)
{
#if IDE 
    CheckInMainThread();
    VSASSERT(dwCookie == 0, "Bad cookie.");

    RELEASE(m_pBuildStatusCallBack);

#endif IDE

    return NOERROR;
}

//============================================================================
// Notification that a "build" is starting. Since the compiler
//  is always running in the background, this might not mean
//  anything more than to disable some UI.
//============================================================================

STDMETHODIMP CompilerProject::ENCRebuild
(
    IUnknown *pIDebugProgram,
    IUnknown ** ppENCUpdate
)
{
    CheckInMainThread();
    HRESULT hr = NOERROR;
    VSASSERT(FALSE,"ENCRebuild() is obsolete!");
    return hr;
}

STDMETHODIMP CompilerProject::StartDebugging()
{
    CheckInMainThread();
    HRESULT hr = NOERROR;
    VSASSERT(FALSE,"StartDebugging() is obsolete!");
    return hr;
}

STDMETHODIMP CompilerProject::StopDebugging()
{
    CheckInMainThread();
    HRESULT hr = NOERROR;
    VSASSERT(FALSE,"StopDebugging() is obsolete!");
    return hr;
}

STDMETHODIMP CompilerProject::GetPEImage( void **ppImage )
{
    CheckInMainThread();
    *ppImage = m_pImage;
    m_pImage = NULL; // Pass ownership to the caller.
    return S_OK;
}

STDMETHODIMP CompilerProject::AddApplicationObjectVariable
(
    LPCOLESTR wszClassName,
    LPCOLESTR wszMemberName
)
{
    return E_NOTIMPL;
}

//============================================================================
// Remove all of the above variables.
//============================================================================

STDMETHODIMP CompilerProject::RemoveAllApplicationObjectVariables()
{
    return E_NOTIMPL;
}

// Called when the project is renamed. Returns NOERROR and changes the name
// of the project if successful, or returns E_FAIL without changing the name
// of the project if not.
STDMETHODIMP CompilerProject::RenameProject(LPCWSTR wszNewProjectName)
{
    CheckInMainThread();
#if !IDE
    VSFAIL("Function available only in IDE compiler!");
    return E_FAIL;
#else

    VSASSERT(!IsMetaData(), "Why are we asked to rename a MetaData project?");

    //
    // Set the project's name to the new name.
    //
    STRING *pstrNewFileName = m_pCompiler->AddString(wszNewProjectName);

    if (pstrNewFileName)
    {
        if (!m_isExpressionEditorProject)
        {
            GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->RenameProject(
                GetProjectId(),
                CComBSTR(wszNewProjectName));
        }

        m_pstrFileName = pstrNewFileName;
        return NOERROR;
    }
    else
    {
        // Failure case.
        return E_FAIL;
    }
#endif
}

// Blocks the foreground thread until the background is in bound state for this project.
STDMETHODIMP CompilerProject::WaitUntilBound()
{
    CheckInMainThread();
#if !IDE
    VSFAIL("Function available only in IDE compiler!");
    return E_FAIL;
#else
    return WaitUntilBindableState(WaitStatePumping::PumpingAllowed, WaitStateCancel::CancelAllowed);
#endif IDE
}

STDMETHODIMP CompilerProject::RenameDefaultNamespace(BSTR bstrNewName)
{
    CheckInMainThread();
#if IDE
    VB_ENTRY();
    hr = E_FAIL;
    ULONG cchNewName = SysStringLen(bstrNewName);
    if (cchNewName == 0)
    {
        return E_INVALIDARG;
    }

    STRING *strDefaultNamespace = GetDefaultNamespace();
    if (strDefaultNamespace == NULL || StringPool::StringLength(strDefaultNamespace) == 0)
    {
        return S_FALSE;
    }

    STRING *strSafeNewName = MakeSafeName(m_pCompiler, m_pCompiler->AddString(bstrNewName), false);

    if (!IsMetaData() && m_pfileList)
    {
        // The whole project needs to be bound for this scenario.
        if (SUCCEEDED(WaitUntilBindableState(WaitStatePumping::PumpingAllowed, WaitStateCancel::CancelAllowed)))
        {
            if (m_pfileList->GetRootNamespace())
            {
                SymbolLocatorResult SymbolToRename;

                SymbolToRename.InitFromNamedRoot(
                    m_pfileList->GetRootNamespace(),
                    this,
                    true); // Allow rootnamespace renames

                hr = CRenameSymbol::Rename(
                        &SymbolToRename,
                        strDefaultNamespace,
                        StringPool::StringLength(strDefaultNamespace),
                        strSafeNewName,
                        StringPool::StringLength(strSafeNewName),
                        RSF_NotifyVS,
                        NULL,
                        NULL,
                        S_FALSE);
            }
        }
    }

    if (!m_isExpressionEditorProject)
    {
        GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->RenameDefaultNamespace(
            GetProjectId(),
            bstrNewName);
    }

    VB_EXIT();

#else
    return E_NOTIMPL;
#endif
}

STDMETHODIMP CompilerProject::GetDefaultReferences
(
    ULONG cElements,
    BSTR *rgbstrReferences,
    ULONG *cActualReferences
)
{
    CheckInMainThread();
    VerifyOutPtr(cActualReferences);

    VB_ENTRY();
    hr = E_FAIL;
    *cActualReferences = 0;

    ULONG cReferences = m_daReferences.Count();
    ULONG cDefaultReferences = 0;
    ULONG iIndex= 0;

    for (iIndex = 0; iIndex < cReferences; iIndex++)
    {
        if (m_daReferences.Element(iIndex).m_fStandard || m_daReferences.Element(iIndex).m_fRequired)
        {
            if (rgbstrReferences && cDefaultReferences < cElements)
            {
                rgbstrReferences[cDefaultReferences] =
                    SysAllocString(m_daReferences.Element(iIndex).m_pCompilerProject->GetFileName());
            }

            cDefaultReferences++;
        }
    }

    if (rgbstrReferences != NULL && cElements < cDefaultReferences)
    {
        *cActualReferences = cElements;
        hr = S_FALSE;
    }
    else
    {
        *cActualReferences = cDefaultReferences;
        hr = S_OK;
    }

    VB_EXIT();
}

// Disable posting compiler messages to avoid filling up the message queue.
STDMETHODIMP CompilerProject::SuspendPostedNotifications
(
)
{
    CheckInMainThread();
#if IDE 
    m_fShouldSuspendNotifications = true;

    if (!m_isExpressionEditorProject)
    {
        GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->SuspendPostedNotifications(
            GetProjectId());
    }

    return NO_ERROR;
#else
    return E_NOTIMPL;
#endif IDE
}

// Enable posting compiler messages.
STDMETHODIMP CompilerProject::ResumePostedNotifications
(
)
{
    CheckInMainThread();
#if IDE 
    m_fShouldSuspendNotifications = false;

    if (!m_isExpressionEditorProject)
    {
        GetCompilerPackage()->GetManagedServices()->GetOutOfProcessWorkspace()->ResumePostedNotifications(
            GetProjectId());
    }

    return NO_ERROR;
#else
    return E_NOTIMPL;
#endif IDE
}

#ifdef EXTENDED_WARNING_GRANULARITY
    //warnings granularity support

UINT CompilerProject::m_wrnIds[]=
{
    #define ERRID(NAME,ID,STRING)
    #define ERRID_NOLOC(NAME,ID,STRING) ERRID(NAME,ID,STRING)
    #define WRNID(NAME,ID,STRING) (ID),
    #include "Errors.inc"
    #undef ERRID
    #undef ERRID_NOLOC
    #undef WRNID
};

STDMETHODIMP CompilerProject::GetWarningIdAndStringTable(/*out*/UINT* count, /*out*/VbCompilerWarningIdAndString** pTable)
{
    HRESULT hr = NOERROR;

    UINT dim =  DIM(m_wrnIds);

    if (count)
        *count = dim;

    if (pTable)
    {
        if (!m_WarningIdAndStringTable)
        {
            // load the cache if this is first call.
            m_WarningIdAndStringTable = VBAllocator::AllocateArray<VbCompilerWarningIdAndString>(dim);

            for (UINT i = 0 ; i < dim; i++)
            {
                RawStringBuffer wszBuffer;
                m_WarningIdAndStringTable[i].dwWrnId = m_wrnIds[i];
                WCHAR wszTemp[CCH_MAX_LOADSTRING];
                IfFailGo(ResLoadString(m_wrnIds[i], wszTemp, DIM(wszTemp)));
                wszBuffer.Allocate(wcslen(wszTemp));
                m_WarningIdAndStringTable[i].wszDescr = wszBuffer.Detach();
                memcpy((void*)m_WarningIdAndStringTable[i].wszDescr, wszTemp, size);
            }
        }

        VbCompilerWarningIdAndString* rgTable = (VbCompilerWarningIdAndString*)CoTaskMemAlloc(dim * sizeof(VbCompilerWarningIdAndString));
        memcpy(rgTable, m_WarningIdAndStringTable, dim * sizeof(VbCompilerWarningIdAndString));

        *pTable = rgTable;
    }

Error:
    return hr;
}

STDMETHODIMP CompilerProject::ValidateWrnId(/*in*/DWORD wrnId, /*out*/BOOL* bIsValid)
{
    // template binsearch is not worth to use it here.
    UINT dim =  DIM(m_wrnIds);
    BOOL found = false;
    int low,high,mid;
    low = 0;
    high = dim;
    mid = 0;
    while (low <= high)
    {
        mid = (low + high)/2;
        if (m_wrnIds[mid] == wrnId)
        {
            found = true;
            break;
        }
        else if (m_wrnIds[mid] > wrnId)
        {
            high = mid - 1;
        }
        else
        {
            low = mid + 1;
        }
    }
    if (bIsValid)
        *bIsValid = found;
    return NOERROR;
}
#endif

//============================================================================
// Add this assembly to the list of unprocessed friends that we have.
// We only handle in source here, because metafiles add their
// friends directly.
//============================================================================

void CompilerProject::AddUnprocessedFriend( _In_z_ STRING* strAssembly, SourceFile* pSourceFile, Location* pLoc )
{
    AssertIfNull( strAssembly );
    AssertIfNull( pSourceFile );
    AssertIfNull( pLoc );

    if(!pSourceFile->m_pDaUnprocessedFriends)
    {
        pSourceFile->m_pDaUnprocessedFriends = 
            new DynamicArray< UnprocessedFriend >();

        if(!pSourceFile->m_pDaUnprocessedFriends)
        {
            VbThrow(E_OUTOFMEMORY);
        }
    }

    UnprocessedFriend& ufriend = pSourceFile->m_pDaUnprocessedFriends->Add();
    ufriend.strAssembly = strAssembly;
    ufriend.pLocation = pLoc;
}

//============================================================================
// Check whether the given project is declared as a friend of the current
// project. We may allow opportunistic binding if the friend declaration is
// strongly typed but we haven't obtained a key for the target project yet.
// The given project should be the project we are currently binding.
// Note that when we call this, we are compiling potentialFriendProject.
//============================================================================

bool CompilerProject::CheckFriendship( CompilerProject* potentialFriendProject )
{
    AssertIfNull( potentialFriendProject );

    bool bRet = false;

    AssemblyIdentity** ppDeclaring = NULL;
    AssemblyIdentity* pReferencing = NULL;

    if( potentialFriendProject->IsCodeModule() )
    {
        if( potentialFriendProject->GetModuleAssemblyIdentity()->GetAssemblyName() != NULL )
        {
            ppDeclaring = m_Friends.HashFind(
                StringPool::Pstrinfo( potentialFriendProject->GetModuleAssemblyIdentity()->GetAssemblyName() ) );
            pReferencing = potentialFriendProject->GetModuleAssemblyIdentity();
        }
    }
    else if( potentialFriendProject->GetAssemblyIdentity()->GetAssemblyName() != NULL )
    {
        ppDeclaring = m_Friends.HashFind(
            StringPool::Pstrinfo( potentialFriendProject->GetAssemblyIdentity()->GetAssemblyName() ) );
        pReferencing = potentialFriendProject->GetAssemblyIdentity();
    }

    if( ppDeclaring != NULL )
    {
        AssertIfNull( pReferencing );

        do
        {
            AssemblyIdentity& ref = **ppDeclaring;

            // Check if potentialFriendProject is delaysign. DelaySign will always imply that binding
            // is allowed; if the user signs with an incorrect key later, it will be a runtime error.

            if( potentialFriendProject->GetDelaySign() )
            {
                bRet = true;
                break;
            }
            else if( ref.IsAssemblyWithSameNameAndPKToken( pReferencing ) )
            {
                bRet = true;
                break;
            }
            else if( ref.IsStrongNameAssembly() && pReferencing->IsStrongNameAssembly() )
            {
                // Here, the name matched but this PK did not. Maybe another one will match?
                bRet = false;
            }
            else if( ref.IsStrongNameAssembly() && !pReferencing->IsStrongNameAssembly() )
            {
                if( potentialFriendProject->IsDeclaredToBoundCompilationInProgress() )
                {
                    // Here we don't have a public key token, so we don't know if we will ever get one.
                    // Just reject the binding, and record this as a "missed binding opportunity."

                    AssertIfTrue( potentialFriendProject->IsMetaData() );
                    potentialFriendProject->AddMissedBindingOpportunity( GetAssemblyName(), &ref );
                    bRet = false;
                }
                else
                {
                    // We are binding method bodies now, and the attributes have been ----ed.
                    // This means that the potentialFriendProject has no public key and thus
                    // won't ever match.

                    bRet = false;
                    break;
                }
            }
            else
            {
                // Strong name referencing a weak name assembly. This error will be caught
                // later. Ignore for now.

                bRet = false;
            }

            ppDeclaring = m_Friends.FindNext();
        } while( ppDeclaring != NULL );
    }

    return( bRet );
}

//============================================================================
// Verify the declaration of the friend assembly.
// We will ensure that only the name and the PKT is set; any other values
// will result in an error.
//============================================================================

bool CompilerProject::VerifyFriendDeclaration(
        _In_z_ STRING* pstrAssembly,
        SourceFile* pSourceFile,
        Location* pLoc
        )
{
    AssertIfNull( pstrAssembly );
    AssertIfNull( pSourceFile );
    AssertIfNull( pLoc );

    bool bRet = false;

    // Parse the string, and examine in the data.
    CComPtr< IAssemblyName > spName;
    HRESULT hr = AssemblyIdentity::ParseAssemblyString( pstrAssembly, &spName );

    if( hr != S_OK || spName == NULL )
    {
        SourceFile::GetCurrentErrorTableForFile( pSourceFile )->CreateError(
                ERRID_FriendAssemblyNameInvalid,
                pLoc,
                pstrAssembly
                );
        goto Error;
    }

    // Verify that we do not have any non-required values.
    // NOTE: GetProperty() is a wierd API. If you pass in no buffer space, then it will
    // return a failure code with ERROR_INSUFFICIENT_BUFFER if the requested key exists.
    // If the requested key does not exist, it will return S_OK.

    DWORD dwSize = 0;

    if( FAILED( hr = spName->GetProperty( ASM_NAME_MAJOR_VERSION, NULL, &dwSize ) ) ||
        FAILED( hr = spName->GetProperty( ASM_NAME_MINOR_VERSION, NULL, &dwSize ) ) ||
        FAILED( hr = spName->GetProperty( ASM_NAME_BUILD_NUMBER, NULL, &dwSize ) ) ||
        FAILED( hr = spName->GetProperty( ASM_NAME_REVISION_NUMBER, NULL, &dwSize ) ) ||
        FAILED( hr = spName->GetProperty( ASM_NAME_CULTURE, NULL, &dwSize ) ) ||
        FAILED( hr = spName->GetProperty( ASM_NAME_ARCHITECTURE, NULL, &dwSize ) ) )
    {
        if( hr != HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) )
        {
            // We have an invalid friend - just issue a warning.

            SourceFile::GetCurrentErrorTableForFile( pSourceFile )->CreateError(
                    ERRID_FriendAssemblyNameInvalid,
                    pLoc,
                    pstrAssembly
                    );
            goto Error;
        }

        // We have a valid friend but it has tokens we don't like, such as
        // Version, or Culture.

        SourceFile::GetCurrentErrorTableForFile( pSourceFile )->CreateError(
                ERRID_FriendAssemblyBadArguments,
                pLoc,
                pstrAssembly
                );
        goto Error;
    }

    AssertIfFalse( dwSize == 0 );

    // Ok, we have a good friend declaration. Verify that if the current project
    // is strongly typed, then we must have a PKT.

    if( SUCCEEDED( hr = spName->GetProperty( ASM_NAME_PUBLIC_KEY, NULL, &dwSize ) ) &&
        (
          !StringPool::IsNullOrEmpty( GetProjectSettingForKeyFileName() ) ||
          !StringPool::IsNullOrEmpty( GetProjectSettingForKeyContainerName() ) ||
          GetDelaySign() ||
          GetFileWithAssemblyKeyFileAttr() != NULL ||
          GetFileWithAssemblyKeyNameAttr() != NULL ||
          !StringPool::IsNullOrEmpty( GetKeyFileName() ) ||
          !StringPool::IsNullOrEmpty( GetKeyContainerName() )
        )
      )
    {
        SourceFile::GetCurrentErrorTableForFile( pSourceFile )->CreateError(
                ERRID_FriendAssemblyStrongNameRequired,
                pLoc,
                pstrAssembly
                );
        goto Error;
    }

    // Finally check for a badly formed attribute.

    if( FAILED( hr ) && hr != HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) )
    {
        SourceFile::GetCurrentErrorTableForFile( pSourceFile )->CreateError(
                ERRID_FriendAssemblyNameInvalid,
                pLoc,
                pstrAssembly
                );
        goto Error;
    }

    bRet = true;

Error:

    return( bRet );
}

//============================================================================
// Take an assembly string and create an assembly identity in the
// [in/out] pAssembly variable.
//============================================================================

bool CompilerProject::FillInAssemblyIdentity(
        _In_z_ STRING* strAssembly,
        AssemblyIdentity* pAssembly // in/out.
        )
{
    AssertIfNull( strAssembly );
    AssertIfNull( pAssembly );

    bool bRet = false;
    CComPtr< IAssemblyName > spName;
    HRESULT hr = AssemblyIdentity::ParseAssemblyString( strAssembly, &spName );

    WCHAR* pszName = NULL;
    BYTE* sbPrimaryKey = NULL;

    if( hr == S_OK && spName != NULL )
    {
        DWORD hiVersion = 0;
        DWORD loVersion = 0;

        hr = spName->GetVersion( &hiVersion, &loVersion );

        if( FAILED( hr ) )
        {
            // Ignore error.
        }

        DWORD cchCount = 0;

        hr = spName->GetProperty( ASM_NAME_NAME, NULL, &cchCount );
        if( hr != HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) )
        {
            goto Error;
        }

        pszName = new (zeromemory) WCHAR[ cchCount ];
        if( pszName == NULL )
        {
            hr = E_OUTOFMEMORY;
            goto Error;
        }

        hr = spName->GetProperty( ASM_NAME_NAME, pszName, &cchCount );
        if( FAILED( hr ) )
        {
            goto Error;
        }

        DWORD dwSize = 0;
        hr = spName->GetProperty( ASM_NAME_PUBLIC_KEY, NULL, &dwSize );

        if( hr == HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) )
        {
            sbPrimaryKey = new (zeromemory )BYTE[ dwSize ];
            hr = spName->GetProperty( ASM_NAME_PUBLIC_KEY, sbPrimaryKey, &dwSize );
            if( FAILED( hr ) )
            {
                goto Error;
            }
        }

        STRING* strName = m_pCompiler->AddString( pszName );
        pAssembly->InitAssemblyIdentityInfo( m_pCompiler, m_pCompilerHost, this );

        pAssembly->SetAssemblyIdentity(
                strName,
                NULL,
                sbPrimaryKey,
                dwSize,
                HIWORD( hiVersion ),
                LOWORD( hiVersion ),
                HIWORD( loVersion ),
                LOWORD( loVersion ),
                afPublicKey
                );

        bRet = true;
    }

Error:

    delete [] sbPrimaryKey;
    delete [] pszName;

    return( bRet );
}

//============================================================================
// Get the AssemblyIdentity for this string and add it to our list.
//============================================================================

void CompilerProject::AddFriend(
        _In_z_ STRING* strAssembly,
        SourceFile* pSourceFile,
        Location* pLoc
        )
{
    AssertIfNull( strAssembly );

    AssemblyIdentity* pAssembly = new (zeromemory) AssemblyIdentity( m_pCompiler, m_pCompilerHost );

    if( pAssembly != NULL && FillInAssemblyIdentity( strAssembly, pAssembly ) )
    {
        AssemblyIdentity** ppAlreadyExists = m_Friends.HashFind( StringPool::Pstrinfo( pAssembly->GetAssemblyName() ) );
        while ( ppAlreadyExists != nullptr )
        {
            if ( !memcmp(*ppAlreadyExists, pAssembly, sizeof(AssemblyIdentity)) )
            {
                delete pAssembly; // We've already have this assembly in the list.
                break;
            }
            ppAlreadyExists = m_Friends.FindNext();
        } 

        if (ppAlreadyExists == nullptr)
        {
            m_Friends.HashAdd( StringPool::Pstrinfo( pAssembly->GetAssemblyName() ), pAssembly );
        }
    }
    else
    {
        if( pAssembly != NULL ) delete pAssembly;

        // Meta files won't have location reported for errors.

        if( pSourceFile != NULL )
        {
            SourceFile::GetCurrentErrorTableForFile( pSourceFile )->CreateError(
                    ERRID_FriendAssemblyNameInvalid,
                    pLoc,
                    strAssembly
                    );
        }
        else
        {
            m_ErrorTable.CreateError(
                    ERRID_FriendAssemblyNameInvalid,
                    NULL,
                    strAssembly
                    );
        }
    }
}


void CompilerProject::VerifyExtensionAttributeDefinition()
{
    SourceFileIterator iter(this);

    SourceFile * pCur = NULL;

    NorlsAllocator nraScratch(NORLSLOC);

    while (pCur = iter.Next())
    {
        BCSYM_ApplAttr * pAttribute = pCur->GetExtensionAttributeApplication();

        if (pAttribute)
        {
            if (!Semantics::GetExtensionAttributeCtor(m_pCompiler, m_pCompilerHost, pCur, &nraScratch, NULL))
            {
                Attribute::ReportError(pCur->GetErrorTable(), ERRID_ExtensionAttributeInvalid, pAttribute);
            }
            break;
        }
    }
}

/******************************************************************************
;VerifyEmbeddedReferenceAttributes

Check each embedded reference and make sure that the target library has the
attributes we need. Each /linked PIA must have a GuidAttribute and an
ImportedFromTypeLibAttribute. If the library lacks either of these, we must
report an error. The spec does not require that the program actually use any
types from the assembly; we have to check each /linked assembly regardless.
******************************************************************************/
void CompilerProject::VerifyEmbeddedReferenceAttributes()
{
    ReferencedProject *rgref = m_daReferences.Array();
    for (ULONG i = 0; i < m_daReferences.Count(); i++)
    {
        if (rgref[i].m_fEmbedded)
        {
            bool bHasGuid = false;
            bool bHasImportedFromTypeLib = false;
            bool bHasPrimaryInteropAssembly = false;
            CompilerProject *proj = rgref[i].m_pCompilerProject;
            AssertIfNull(proj);
            if (!proj)
            {
                continue;
            }
            FileIterator files(proj);
            while (CompilerFile *pFile = files.Next())
            {
                BCSYM_Namespace *psymNamespace = pFile->GetRootNamespace();
                AssertIfNull(psymNamespace);
                if (psymNamespace)
                {
                    WellKnownAttrVals *pAttrs = psymNamespace->GetPWellKnownAttrVals();
                    WCHAR *assemblyGuid = NULL;
                    bHasGuid |= pAttrs->GetGuidData(&assemblyGuid);
                    WCHAR *tlbFile = NULL;
                    bHasImportedFromTypeLib |= pAttrs->GetImportedFromTypeLibData(&tlbFile);
                    bHasPrimaryInteropAssembly |= pAttrs->GetPrimaryInteropAssemblyData();
                }
            }

            // The referenced project doesn't contain the required NoPIA attributes.
            // Report the errors for source projects, or for error-free metadata projects.
            if (!bHasGuid && !proj->IsMetadataProjectWithErrors())
            {
                m_ErrorTable.CreateError(ERRID_PIAHasNoAssemblyGuid1, NULL, proj->GetAssemblyName(), GUIDATTRIBUTE_NAME);
            }
            if (!bHasImportedFromTypeLib && !bHasPrimaryInteropAssembly && !proj->IsMetadataProjectWithErrors())
            {
                m_ErrorTable.CreateError(ERRID_PIAHasNoTypeLibAttribute1, 
                                         NULL, 
                                         proj->GetAssemblyName(), 
                                         IMPORTEDFROMTYPELIBATTRIBUTE_NAME, 
                                         PRIMARYINTEROPASSEMBLYATTRIBUTE_NAME);
            }

            // The metadata importer uses the project assemblyidentity's IsImportedFromCOMTypeLib flag or 
            // IsPrimaryInteropAssembly flag to determine whether it should record extra location information 
            // for type members, expecting that this flag must must be set for any embeddable assembly. We'll 
            // validate this assumption now:
            if (proj->IsMetaData())
            {
                AssertIfFalse(bHasImportedFromTypeLib == proj->GetAssemblyIdentity()->IsImportedFromCOMTypeLib());
                AssertIfFalse(bHasPrimaryInteropAssembly == proj->GetAssemblyIdentity()->IsPrimaryInteropAssembly());
            }
        }
    }
}


//============================================================================
// Go through each of the unprocessed friends and verify them. This should
// be called after all attributes are ----ed.
//============================================================================
void CompilerProject::ProcessFriendDeclarations()
{
    SourceFileIterator iter(this);

    SourceFile * pCur = NULL;

    while (pCur = iter.Next())
    {
        if (pCur->m_pDaUnprocessedFriends)
        {
            for( UINT i = 0; i < pCur->m_pDaUnprocessedFriends->Count(); i += 1 )
            {
                UnprocessedFriend& ufriend = pCur->m_pDaUnprocessedFriends->Element( i );

                if( VerifyFriendDeclaration( ufriend.strAssembly, pCur, ufriend.pLocation ) )
                {
                    AddFriend( ufriend.strAssembly, pCur, ufriend.pLocation );
                }
            }
        }
    }
}

//============================================================================
// Add an opportunistic binding.
//============================================================================

void CompilerProject::AddMissedBindingOpportunity(
        _In_z_ STRING* strDeclaringAssemblyName,
        AssemblyIdentity* FriendDeclaration
        )
{
    AssertIfFalse( IsDeclaredToBoundCompilationInProgress() );
    AssertIfNull( strDeclaringAssemblyName );
    AssertIfNull( FriendDeclaration );

    // Don't add this if it's already in the list.

    for( UINT i = 0; i < m_TempMissedBindingOpportunities.Count(); i += 1 )
    {
        MissedBindings& TempBinding = m_TempMissedBindingOpportunities.Element( i );
        if( TempBinding.DeclaringAssemblyName == strDeclaringAssemblyName &&
            TempBinding.FriendDeclaration == FriendDeclaration )
        {
            return;
        }
    }

    for( UINT i = 0; i < m_MissedBindingOpportunities.Count(); i += 1 )
    {
        MissedBindings& TempBinding = m_MissedBindingOpportunities.Element( i );
        if( TempBinding.DeclaringAssemblyName == strDeclaringAssemblyName &&
            TempBinding.FriendDeclaration == FriendDeclaration )
        {
            return;
        }
    }

    MissedBindings& Binding = m_TempMissedBindingOpportunities.Add();
    Binding.DeclaringAssemblyName = strDeclaringAssemblyName;
    Binding.FriendDeclaration = FriendDeclaration;
}

//============================================================================
// Commit by moving the items in the temp list to the real list.
//============================================================================

void CompilerProject::CommitMissedBindingOpportunity()
{
    for( UINT i = 0; i < m_TempMissedBindingOpportunities.Count(); i += 1 )
    {
        MissedBindings& TempBinding = m_TempMissedBindingOpportunities.Element( i );
        MissedBindings& Binding = m_MissedBindingOpportunities.Add();
        Binding.DeclaringAssemblyName = TempBinding.DeclaringAssemblyName;
        Binding.FriendDeclaration = TempBinding.FriendDeclaration;
    }
    m_TempMissedBindingOpportunities.Reset();
}

//============================================================================
// Check through the missed binding opportunities and see if any of them
// are real. If any are, display an error.
//============================================================================

void CompilerProject::VerifyMissedBindingOpportunities()
{
    AssertIfFalse( IsDeclaredToBoundCompilationInProgress() );

    if( GetAssemblyIdentity()->IsStrongNameAssembly() )
    {
        for( UINT i = 0; i < m_MissedBindingOpportunities.Count(); i += 1 )
        {
            MissedBindings& TempBinding = m_MissedBindingOpportunities.Element( i );

            if( TempBinding.FriendDeclaration->IsAssemblyWithSameNameAndPKToken( GetAssemblyIdentity() ) )
            {
                m_ErrorTable.CreateError(
                        ERRID_FriendAssemblyRejectBinding,
                        NULL,
                        TempBinding.DeclaringAssemblyName,
                        TempBinding.FriendDeclaration->GetAssemblyName()
                        );
            }
        }
    }

    // Clear the missed bindings list, we don't need it anymore.

    m_MissedBindingOpportunities.Reset();
}


//============================================================================
// Clean up the friend list that we have.
//============================================================================

void CompilerProject::ClearFriends()
{
    if( m_Friends.NumEntries() > 0 )
    {
        m_Friends.ResetHashIterator();
        AssemblyIdentity **ppAssembly = NULL;

        while( ( ppAssembly = m_Friends.IterateHash() ) != NULL )
        {
            AssertIfNull( *ppAssembly );
            delete *ppAssembly;
        }

        m_Friends.Clear();
    }

    // The missed binding opportunities should be cleared in VerifyMissedBindingOpportunities
    // however, we could have aborted and bypassed VerifyMissedBindingOpportunities, so we clear it now

    ClearAllMissedBindingOpportunities();

}

//****************************************************************************
// Background compilation methods.
//****************************************************************************

#if IDE 
//============================================================================
// Block until the compiler brings all files in this project to the given state.
//============================================================================

HRESULT CompilerProject::WaitUntilState(CompilationState csState, WaitStatePumpingEnum ShouldPump, WaitStateCancelEnum CanCancel)
{
    CompilerSharedState *pCompilerSharedState = GetCompilerSharedState();

    if (!IsInitialized() || !pCompilerSharedState || m_pCompiler == NULL)
    {
        return E_FAIL;
    }

    if (pCompilerSharedState->IsInWaitUntilState())
    {
        // Re-entrancy. To avoid re-entrant bugs we return E_PENDING to the caller.
        return E_PENDING;
    }

    WaitStateGuard waitStateGuard;

    VB_ENTRY();
  
    if ( GetCompState() < csState )
    {
        if (!GetCompilerPackage()->IsSolutionBuild())
        {
            m_pCompiler->StartBackgroundThreadIfNeeded();

            if (GetCompilerSharedState()->IsBackgroundCompilationBlocked() || GetCompilerSharedState()->IsBackgroundCompilationStopping())
            {
                return E_FAIL;
            }
        }
        else if (pCompilerSharedState->IsCompilingSpecificProject())
        {
            // We're in the middle of a Soln build and someone wants bound information for a project
            // The project being built may not be this one so we need to ensure it gets built.
            if (pCompilerSharedState->IsBackgroundCompilationBlocked() && !GetCompilerPackage()->GetCompileToDiskPending())
            {
                // The background thread should never be stopped during an explicit Build (except during the CompileToDisk portion)
                // We must throw here otherwise we will have a hang.
                VbThrow(E_FAIL);
            }

            // Make sure the current CompileOneProject finishes
            if (FAILED(hr = GetCompilerPackage()->WaitForCompileOneProject(CanCancel)))
            {
                return hr;
            }

            //If the project that was just built was actually us we'll now be above Bound
            if (GetCompState() >= csState )
            {
                return hr;
            }

            // Set a flag to avoid the optimization that skips CompileAllProjects during
            // a soln build.
            pCompilerSharedState->SetBackgroundCompileRequested(true);
        }
        else
        {
            // In this case we are in the middle of a solution build and no specific project is requested
            // for compilation.  Ideally the background thread should be running at this point because we
            // we are in the middle of a solution build.  However we can easily get into the case where
            // parts of the IDE set a BackgroundCompilerLock and then later on call WaitUntilState on 
            // a particular project.  The lock will block the background thread and cause us to deadlock.
            // Our only choice is to fail here
            if (pCompilerSharedState->IsBackgroundCompilationBlocked() || pCompilerSharedState->IsBackgroundCompilationStopping())
            {
                return E_FAIL;
            }
        }

        // This call will ensure the background compiler will get our project to
        // the desired state as fast as possible by skipping unrelated projects
        CompilerHost *pCompilerHost = GetCompilerHost();
        if (pCompilerHost)
        {
            DynamicArray<CompilerProject *> pProjects;
            pProjects.AddElement(this);
            pCompilerHost->OptimizeProjectCompilationListHelper(&pProjects, csState);
        }

        ChangeCursor<IDC_WAITCURSOR> *pwaitCursor = 0;
        // for edit hosting scenarios we don't want to show wait cursor
        if (!m_isExpressionEditorProject && GetCompilerPackage()->GetSite())
        {
            pwaitCursor = new ChangeCursor<IDC_WAITCURSOR>();
        }

        // Wait for our state to be signaled
        switch (csState)
        {
            case CS_Declared:
                hr = m_eventDeclaredState.WaitForSignal(ShouldPump, CanCancel);
                break;

            case CS_Bound:
                hr = m_eventBoundState.WaitForSignal(ShouldPump, CanCancel);
                break;

            case CS_TypesEmitted:
                hr = m_eventTypesEmittedState.WaitForSignal(ShouldPump, CanCancel);
                break;

            case CS_Compiled:
                hr = m_eventCompiledState.WaitForSignal(ShouldPump, CanCancel);
                break;

            default:
                VSFAIL("Invalid CompilationState passed to WaitUntilState");
                break;
        }
        
        // We're now finshed with the Preferred Project so we clear it to prevent
        // subsequent CompileAllProjects doing the optimization.
        if (pCompilerHost)
        {
            pCompilerHost->SetProjectToWaitOn(NULL);
            pCompilerHost->SetProjectCompilationStateToWaitOn(CS_NoState);
        }

        if (pwaitCursor)
        {
            delete pwaitCursor;
        }
    }
    VB_EXIT_NORETURN();

    
    RRETURN( hr );
}

//============================================================================
// Get the contextual intellisense filter object
//============================================================================
IVsContextualIntellisenseFilter *CompilerProject::GetContextualIntellisenseFilter()
{
    if (!m_fQueriedForContextualIntellisenseFilter)
    {
        // Only ask for the filter if we have not asked before
        CComQIPtr<IVsProject> spVSProject(m_pHierarchy);
        CComPtr<IServiceProvider> spProvider;
        if (spVSProject && SUCCEEDED(spVSProject->GetItemContext(VSITEMID_ROOT, &spProvider)))
        {
            if (FAILED(spProvider->QueryService(IID_IVsContextualIntellisenseFilter, IID_IVsContextualIntellisenseFilter, (void **) &m_pContextualIntellisenseFilter)))
            {
                m_pContextualIntellisenseFilter = NULL;
            }
        }

        m_fQueriedForContextualIntellisenseFilter = true;
    }

    return m_pContextualIntellisenseFilter;
}

//============================================================================
// Get the internal CodeModel Object
//============================================================================
HRESULT CompilerProject::GetInternalCodeModel(CCodeModel **ppCodeModel)
{
    VerifyOutPtr(ppCodeModel);

    HRESULT hr = E_FAIL;

    *ppCodeModel = NULL;

    if (m_pCodeModel)
    {
        *ppCodeModel = m_pCodeModel;
        (*ppCodeModel)->AddRef();

        hr = S_OK;
    }

    return hr;
}

//============================================================================
// Get the CodeModel Interface Object
//============================================================================
HRESULT CompilerProject::GetCodeModel(CodeModel **ppCodeModel)
{
    VerifyOutPtr(ppCodeModel)

    HRESULT hr = E_FAIL;

    *ppCodeModel = NULL;

    if (m_pCodeModel)
    {
        hr = m_pCodeModel->QueryInterface(
            IID_CodeModel,
            (void **) ppCodeModel);
    }

    return hr;
}

//============================================================================
// Gets the ExternalCode Model Object
//============================================================================
HRESULT CompilerProject::GetExternalCodeModel(CExternalCodeModel **ppCodeModel)
{
    VerifyOutPtr(ppCodeModel);
 
    HRESULT hr = E_FAIL;

    *ppCodeModel = NULL;

    if (m_pExternalCodeModel)
    {
        *ppCodeModel = m_pExternalCodeModel;
        (*ppCodeModel)->AddRef();

        hr = S_OK;
    }

    return hr;
}

//============================================================================
// Lazily determines whether Xml intellisense is enabled and schemas are
// available for compilation.  This must be called on the foreground thread,
// as it queries for VS services.
//============================================================================
XmlIntellisenseSchemas * CompilerProject::GetIntellisenseSchemas()
{
    CheckInMainThread();
    return UpdateXmlSchemas() ? &m_XmlSchemas : NULL;
}

//============================================================================
// Update m_XmlSchemas based on XmlHelperExtension and IsMetaData.
// It returns whether m_XmlSchemas is open or not. 
//============================================================================
bool CompilerProject::UpdateXmlSchemas()
{
    CheckInMainThread();

    // If Xml features are enabled, then enable Xml intellisense
    if (m_rgfExtensionAdded[SolutionExtension::XmlHelperExtension] && !IsMetaData())
    {
        if (!m_XmlSchemas.IsOpen())
        {
            IVsHierarchy * pHierarchy = this->GetHierarchy();
            IServiceProvider * pSite = GetCompilerPackage()->GetSite();
            if (pHierarchy != NULL && pSite != NULL)
            {
                CComPtr<IVsSolution> pSolution;
                if (SUCCEEDED(pSite->QueryService(SID_SVsSolution, IID_IVsSolution, (void **) &pSolution)) &&
                    pSolution != NULL)
                {
                    GUID guidProject;
                    if (SUCCEEDED(pSolution->GetGuidOfProject(pHierarchy, &guidProject)) &&
                        guidProject != GUID_NULL)
                    {
                        m_XmlSchemas.Open(guidProject);
                        m_XmlSchemas.Refresh();
                    }
                }
            }
            else if (m_isExpressionEditorProject)
            {
                // Allow basic XML completion without real project in Expression Editor scenarios.
                m_XmlSchemas.Open(GUID_NULL);
            }
        }        
    }
    else
    {
        m_XmlSchemas.Close();
    }
     
    return m_XmlSchemas.IsOpen();
}


//============================================================================
// Lazily determines if whether Xml intellisense is enabled and schemas 
// are available for compilation. If not, flags the schemas
// available for compilation.  This must be called on the foreground thread,
// as it queries for VS services.
//============================================================================
XmlIntellisenseSchemas * CompilerProject::GetOrFlagIntellisenseSchemas()
{
    CheckInMainThread();

    // If Xml features are enabled, then enable Xml intellisense
    if (m_rgfExtensionAdded[SolutionExtension::XmlHelperExtension] && !IsMetaData())
    {
        if (!m_XmlSchemas.IsOpen())
        {
            GetCompilerPackage()->FlagXmlSchemas(this, true);
            return NULL;
        }
        return &m_XmlSchemas;
    }

    m_XmlSchemas.Close();
    return NULL;
}

//============================================================================
// Return true if intellisense schemas are enabled and need to be initialized.
//============================================================================
bool CompilerProject::NeedToInitIntellisenseSchemas()
{
    CheckInMainThread();
    return m_rgfExtensionAdded[SolutionExtension::XmlHelperExtension] && !m_XmlSchemas.IsOpen();
}

//============================================================================
// Gets whether this project is a Web Misc files project or an ASPX page
// inside a WAP project. Such projects should be ignored for the purposes
// of Rename commands.
//============================================================================
bool CompilerProject::IsVenusMiscProjectOrPageInWapProject()
{
    IVsHierarchy * pHierarchy = this->GetHierarchy();
    if (pHierarchy)
    {
        CComQIPtr<IVsWebApplicationProject> spWapProject(pHierarchy);
        if (spWapProject && this->IsContainedLanguageProject())
        {
            // Treat language projects contained in WAP projects as misc files project.
            return true;
        }

        if (IsVenusMiscProject())
        {
            return true;
        }
    }
    return false;
}

//============================================================================
// Gets whether this project is a Web Misc files project
//============================================================================
bool CompilerProject::IsVenusMiscProject()
{
    IVsHierarchy * pHierarchy = this->GetHierarchy();
    if (pHierarchy)
    {
        CComQIPtr<IPersist> spPersist(pHierarchy);
        if (spPersist)
        {
            // Venus misc files show up to us as regular projects, so check the guid.
            CLSID clsidFromHierarchy = CLSID_NULL;
            if (SUCCEEDED(spPersist->GetClassID(&clsidFromHierarchy)) &&
                clsidFromHierarchy == CLSID_MiscellaneousFilesProject)
            {
                return true;
            }
        }
    }
    return false;
}

//============================================================================
// Initialize Xml intellisense schema objects.
//============================================================================
void XmlIntellisenseSchemas::Open
(
    GUID ProjectGuid
)
{
    if (ProjectGuid == GUID_NULL)
    {
        // In Expression Editor we don't have a real project to work with, but we
        // still want this object initialized so general XML axis IntelliSense works.
        m_IsOpen = true;
    }
    else
    {
        CComPtr<IXmlIntellisenseService> Service;
        HRESULT hr;

        // Try to get the XmlIntellisenseService
        hr = GetCompilerPackage()->GetSite()->QueryService(
                SID_SXmlIntellisenseService,
                IID_IXmlIntellisenseService,
                (void**)&Service);

        if (SUCCEEDED(hr))
        {
            // Create the IXmlIntellisenseSchemas instance
            IfFailThrow(Service->CreateSchemas(ProjectGuid, &m_Schemas));       // Must succeed
            IfFailThrow(m_Schemas->get_CompiledEvent(&m_SchemasCompiledEvent)); // Must succeed

            m_FirstErrorSource = NULL;
            m_IsOpen = true;
        }
    }
}

//============================================================================
// Close down all Xml intellisense schema objects.  If a background thread is currently executing, it's just finishes
// in the background without any effect on foreground.
//============================================================================
void XmlIntellisenseSchemas::Close()
{
    // Ensure that the compiler package will not attempt to reference the schemas now that they're closed
    GetCompilerPackage()->FlagXmlSchemas(NULL, false);

    m_Schemas.Release();
    m_IsRefreshing = false;
    m_IsOpen = false;
}

//============================================================================
// This method will be called periodically from the idle loop in order to
// refresh the schema intellisense information.  This method will return
// true once a Refresh has been completed. The next call to Refresh() will
// cause a new compilation to be initiated.
//============================================================================
bool XmlIntellisenseSchemas::Refresh()
{
    if (!m_Schemas)
    {
        // Ignore schema operations if no data is connected (Expression Editor case).
        m_IsRefreshing = false;
    }
    else 
    {
        VB_SET_REENTRANCY_RETURN_RETRYLATER();
        if (!m_IsRefreshing)
        {
            // Start background compile
            m_Schemas->AsyncCompile();
            m_IsRefreshing = true;
        }
        // Check to see if compilation is complete (event will be signaled if so)
        else if (WaitForSingleObject(m_SchemasCompiledEvent, 0) != WAIT_TIMEOUT)
        {
            // Done compiling.  Get most up to date error information.
            BSTR bstrFirstErrorSource;
            IfFailThrow(m_Schemas->get_FirstErrorSource(&bstrFirstErrorSource));

            // Only refresh task list if error source is different
            STRING *Source = m_Project->GetCompiler()->AddStringWithLen(bstrFirstErrorSource, ::SysStringLen(bstrFirstErrorSource));

            if (m_FirstErrorSource != Source)
            {
                m_FirstErrorSource = Source;
                m_Project->GetCompilerTaskProvider()->TaskListNeedsRefresh();
            }
            m_IsRefreshing = false;
        }

    }
    return !m_IsRefreshing;
}

//============================================================================
// If background compilation is complete, then return the list of resulting
// members.  If the list is ready and no errors occurred, then true will be
// returned.
//============================================================================
bool XmlIntellisenseSchemas::GetMembers(IXmlIntellisenseMemberList ** ppXmlMembers)
{
    if (!m_Schemas)
    {
        // Ignore schema operations if no data is connected (Expression Editor case).
        return false;
    }

    if (!m_IsRefreshing || Refresh())
    {
        // Return successfully compiled members (members will be null if error occurred)
        IfFailThrow(m_Schemas->get_MemberList(ppXmlMembers));
        return *ppXmlMembers != NULL;
    }

    return false;
}

//============================================================================
// If background compilation is complete, then return the list of all
// target namespaces that are implemented by any schema in the final schema
// set.  If the list is ready and no errors occurred, then true will be
// returned.
//============================================================================
bool XmlIntellisenseSchemas::GetTargetNamespaces(SAFEARRAY ** ppTargetNamespaces)
{
    if (!m_Schemas)
    {
        // Ignore schema operations if no data is connected (Expression Editor case).
        return false;
    }

    if (!m_IsRefreshing || Refresh())
    {
        // Return target namespaces (list will be null if error occurred)
        IfFailThrow(m_Schemas->get_TargetNamespaces(ppTargetNamespaces));
        return *ppTargetNamespaces != NULL;
    }

    return false;
}

//============================================================================
// If background compilation is complete, then show element with given
// full name in the XML Schema Browser.
//============================================================================
void XmlIntellisenseSchemas::ShowInXmlSchemaExplorer(
    _In_opt_z_ STRING * pstrNamespace, 
    _In_opt_z_ STRING * pstrLocalName,
    _Out_ BOOL &fElementFound, 
    _Out_ BOOL &fNamespaceFound)
{
    fElementFound = FALSE;
    fNamespaceFound = FALSE;

    if (!m_Schemas)
    {
        // Ignore schema operations if no data is connected (Expression Editor case).
        return;
    }

    if (!m_IsRefreshing || Refresh())
    {
        CComBSTR bstrNamespace(pstrNamespace);
        CComBSTR bstrLocalName(pstrLocalName);
        m_Schemas->ShowInXmlSchemaExplorer(bstrNamespace, bstrLocalName, &fElementFound, &fNamespaceFound);
    }
}

#endif IDE

//============================================================================
// This function determines if the current project has any errors up to this point.
//
// It is expensive and inefficient and should be avoided if possible.
//============================================================================
bool CompilerProject::HasErrors()
{
    // Do not check metadata project references because they may
    // have circular references
    if (!IsMetaData())
    {
        CompilerProject *pproject = NULL;

        // Check the referenced projects.
        ReferenceIterator iter(this);

        while (pproject = iter.Next())
        {
            if (pproject->HasErrors())
            {
                return true;
            }
        }
    }

    CompilerFile *pfile = NULL;

    // Check our files.
    for (pfile = m_pfileList; pfile; pfile = pfile->m_pfileNext)
    {
        if (pfile->HasActiveErrors())
        {
            return true;
        }
    }

    // Check the files here.
    return m_ErrorTable.HasErrors();
}

//============================================================================
// This function should only be called by the compiler during compilation.
//
// It is used to check for errors when going from Bound to Compiled, and it is
// optimized for use in the background thread and to allow caching of the data.
//============================================================================
bool CompilerProject::HasErrors_AllowCaching()
{
    ErrorCacheNode *pErrorCacheNode = NULL;
    CompilerProject *pThisProject = this;

    AssertIfFalse(m_cs >= CS_Bound);

    // Node should already exist
    if (m_pCompilerHost->GetProjectErrorCacheTree()->Find(&pThisProject, &pErrorCacheNode))
    {
        AssertIfFalse(pErrorCacheNode->ReferenceErrorsSet);

        if (pErrorCacheNode->ErrorsSet)
        {
            // Use the cached error flag if it has been set.
            return pErrorCacheNode->HasErrors;
        }
        else if (pErrorCacheNode->ReferenceErrorsSet)
        {
            // Use the cached reference error flag if it has been set or'd with our current errors.
            return pErrorCacheNode->HasReferenceErrors || HasErrorsInFilesOrTable();
        }
    }

    // Safety fallback.
    VSFAIL("Project should already exist in the tree");

    return HasErrorsInReferences() || HasErrorsInFilesOrTable();
}

// =========================================================
// This function isoptimized for use by the compiler during
// very specific stages of compilation. It is not a general helper.
// =========================================================
bool CompilerProject::CacheErrorsInReferences()
{
    ErrorCacheNode *pErrorCacheNode = NULL;
    CompilerProject *pThisProject = this;
    bool HasErrors = true;

    // Insert node into the tree.
    bool NodeInserted = m_pCompilerHost->GetProjectErrorCacheTree()->Insert(&pThisProject, &pErrorCacheNode);

    if (NodeInserted || !pErrorCacheNode->ReferenceErrorsSet)
    {
        HasErrors = HasErrorsInReferences();

        if (pErrorCacheNode)
        {
            // Cache if there are errors in any non-metadata references.
            pErrorCacheNode->HasReferenceErrors = HasErrors;
            pErrorCacheNode->ReferenceErrorsSet = true;

            // Initialize current project error state to false
            pErrorCacheNode->HasErrors = false;
            pErrorCacheNode->ErrorsSet = false;
        }
    }
    else
    {
        if (pErrorCacheNode)
        {
            HasErrors = pErrorCacheNode->HasReferenceErrors;
        }
    }

    return HasErrors;
}

// =========================================================
// This function isoptimized for use by the compiler during
// very specific stages of compilation. It is not a general helper.
// =========================================================
bool CompilerProject::CacheErrorsInCurrentProject()
{
    ErrorCacheNode *pErrorCacheNode = NULL;
    CompilerProject *pThisProject = this;

    // Node should already exist.
    if (!m_pCompilerHost->GetProjectErrorCacheTree()->Find(&pThisProject, &pErrorCacheNode))
    {
        VSFAIL("Project should already exist in the tree");
    }

    bool HasErrors = false;

    if (pErrorCacheNode)
    {
        AssertIfFalse(pErrorCacheNode->ReferenceErrorsSet);

        if (pErrorCacheNode->HasReferenceErrors)
        {
            // No need to look if our references have errors.
            HasErrors = true;
        }
        else
        {
            // Look for errors in the project files.
            HasErrors = HasErrorsInFilesOrTable();
        }

        pErrorCacheNode->HasErrors = HasErrors;
        pErrorCacheNode->ErrorsSet = true;
    }
    else
    {
        // Safety fallback.

        VSFAIL("We should have a cached node already");

        HasErrors = HasErrorsInReferences() || HasErrorsInFilesOrTable();
    }

    return HasErrors;
}

// =========================================================
// This function isoptimized for use by the compiler during
// very specific stages of compilation. It is not a general helper.
// =========================================================
bool CompilerProject::HasErrorsInReferences()
{
    // Do not check metadata project references because they may
    // have circular references
    if (!IsMetaData())
    {
        CompilerProject *pproject = NULL;

        // Check the referenced projects.
        ReferenceIterator iter(this);

        while (pproject = iter.Next())
        {
            if (pproject->HasErrors_AllowCaching())
            {
                return true;
            }
        }
    }

    return false;
}

// =========================================================
// This function isoptimized for use by the compiler during
// very specific stages of compilation. It is not a general helper.
// =========================================================
bool CompilerProject::HasErrorsInFilesOrTable()
{
    CompilerFile *pfile = NULL;

    // Check our files.
    for (pfile = m_pfileList; pfile; pfile = pfile->m_pfileNext)
    {
        if (pfile->HasActiveErrorsNoLock())
        {
            return true;
        }
    }

    // Check the files here.
    return m_ErrorTable.HasErrorsNoLock();
}

#if IDE 

//============================================================================
// Does this project depent on this name?
//============================================================================

bool CompilerProject::DoesProjectDependOnName
(
    _In_z_ STRING *pstrName,
    CompilationState *pcs
)
{
    VSASSERT(!IsMetaData(), "Project level name dependencies unexpected for metadata projects!!!");

    // The only place where projects depend on names at the moment is in
    // project level imports. Just do a ---- walk through the list because
    // odds are we aren't going to have that many imports.

    *pcs = CS_Bound; // Imports are always bound going to Bound
    ImportedTarget *pitCurrent = m_Imports.GetAllImportedTargets();

    while (pitCurrent)
    {
        if (!pitCurrent->m_IsXml)
        {
            for (unsigned i = 0; i < pitCurrent->m_NumDotDelimitedNames; i++)
            {
                if (StringPool::IsEqual(pstrName, pitCurrent->m_DotDelimitedNames[i]))
                    return true;
            }
        }

        pitCurrent = pitCurrent->m_pNext;
    }

    return false;
}
#endif IDE

//============================================================================
// Get the root of the output file name for the project.  (i.e.
// c:\foo\bar.vbproj will return "Bar").
//============================================================================

HRESULT CompilerProject::GetRootOfPEFileName(_Deref_out_opt_z_ STRING **ppstrName)
{
    HRESULT hr = NOERROR;

    WCHAR *wszName, *wszExtension;

    *ppstrName = NULL;

    if (m_pstrPEName)
    {
        wszName = PathFindName(m_pstrPEName);
        wszExtension = PathFindExt(wszName);

        *ppstrName = m_pCompiler->AddStringWithLen(wszName,
                                                   wszExtension - wszName);
    }

    return hr;
}

STRING *CompilerProject::GetFileNameWithNoPath()
{
    if (m_pstrFileName != m_pstrFileNameDuringPrevNoPathReq)
    {
        m_pstrFileNameDuringPrevNoPathReq = m_pstrFileName;

        m_pstrFileNameNoPath =
            m_pstrFileName ?
                m_pCompiler->AddString(PathFindName(m_pstrFileName)) :
                NULL;
    }

    return m_pstrFileNameNoPath;
}

//============================================================================
// Decide how much work is needed to compile this project.
//============================================================================

unsigned CompilerProject::CountStepsToCompile()
{
    CompilationState csInit = GetCompState();

    //========================================================================
    // Figure out how much work we have to do.
    //========================================================================
    unsigned cDo = 0;

    // If we're not already in compiled state
    if (m_cs != CS_Compiled)
    {
        // We always check each state, each of which is a "step".
        cDo = CS_Compiled - CS_NoState;
        CompilationState cs;

        for (cs = CS_NoState; cs < CS_Compiled; cs = (CompilationState)(cs + 1))
        {
            CDoubleListForwardIter<CompilerFile> iter(&m_dlFiles[cs]);

            cDo += (CS_Compiled - cs) * m_dlFiles[cs].NumberOfEntries();
        }
    }

    return cDo;
}

//============================================================================
// Do the work to compile this project to Declared.
//============================================================================
// Return true if aborted, false otherwise

bool
CompilerProject::CompileToDeclared
(
)
{
    DebCheckInCompileThread(m_pCompiler);

    bool fAborted = false;
    CompilationState csInit = GetCompState();

    // If we're already in compiled, don't do anything
    // It isn't possible to return here if the project is a state >= Bound but
    // not Compiled, because that messes up the count of completed compile steps.
    if (csInit == CS_Compiled)
    {
        return fAborted;
    }

    // Remember which project is currently being compiled
    UpdateProjectBeingCompiled projectBeingCompiled(m_pCompiler, this);
    m_fCompiledSuccessfully = false;


    if (csInit < CS_Declared)
    {
#if IDE 
#if LOGTHREADEVENTS
        if (!m_fMetaData)
        {
            DebThreadLogEvent(("Compiler thread No_state->Declared = %S\n",GetFileName()));
        }
#endif LOGTHREADEVENTS
        LOG_INFO(
            VB_COMPILATION,
            "Start promoting project %S (CurState = %d) to Declared...",
            GetFileName(),
            csInit
            );
#endif
#if DEBUG
        // Metadata project could have circular references and so this is not
        // always possible
        //
        if (!IsMetaData())
        {
            // Verify that all of our referenced projects are compiled.
            ReferenceIterator references(this);
            CompilerProject *pReference;

            while (pReference = references.Next())
            {
                if (pReference->IsInitialized())
                {
                    VSASSERT(pReference->GetCompState() >= CS_Declared, "A referenced project is not in Declared state.");
                }
            }
        }
#endif DEBUG

        VSASSERT(GetCompState() < CS_Declared, "How did the compilation state change unexpectedly ?");

        IfTrueAbort(_PromoteToDeclared());

        VSASSERT(GetCompState() == CS_Declared, "Expected CS_Declared!!!");

#if IDE 
        // The project is now in declared state.  Release the mutex lock
        // in case someone is blocked waiting for declared state.
        //
        m_iDeclChangeIndex++;
        m_eventDeclaredState.Signal();


        GetCompilerSharedState()->RefreshTaskList();
#endif IDE

#if IDE 
        LOG_INFO(
            VB_COMPILATION,
            "Done promoting project %S to Declared (fAbort = %d).",
            GetFileName(),
            fAborted
            );
#endif
    }

    // Tick only if compiled directly, not if compiled indirectly on demand
    // when compiling another project.
    //
    if (!projectBeingCompiled.GetPrevious())
    {
        FinishCompileStep(m_pCompiler);    // Project now in CS_Declared
    }

Abort:

    return fAborted;
}

//============================================================================
// Do the work to compile this project to Bound.
//============================================================================
// Return true if aborted, false otherwise

bool
CompilerProject::CompileFromDeclaredToBound
(
)
{
    DebCheckInCompileThread(m_pCompiler);

    bool fAborted = false;
    CompilationState csInit = GetCompState();

    // If we're already in compiled, don't do anything
    // It isn't possible to return here if the project is a state >= Bound but
    // not Compiled, because that messes up the count of completed compile steps.
    if (csInit == CS_Compiled)
    {
        return fAborted;
    }

    UpdateProjectBeingCompiled projectBeingCompiled(m_pCompiler, this);
    m_fCompiledSuccessfully = false;

#if DEBUG
    // Verify that all of our referenced projects are compiled, except for metadata->metadata
    // references, which do not need to be in bound state
    if (!IsMetaData())
    {
        ReferenceIterator references(this);
        CompilerProject *pReference;

        while (pReference = references.Next())
        {
            if (pReference->IsInitialized())
            {
                VSASSERT(pReference->GetCompState() >= CS_Bound, "A referenced project is not in Bound state.");
            }
        }
    }
#endif DEBUG

    if (csInit < CS_Bound)
    {
#if IDE 
#if LOGTHREADEVENTS
        if (!m_fMetaData)
        {
            DebThreadLogEvent(("Compiler thread Decl->Bound= %S\n",GetFileName()));
        }
#endif LOGTHREADEVENTS
        LOG_INFO(
            VB_COMPILATION,
            "Start promoting project %S (CurState = %d) to Bound...",
            GetFileName(),
            csInit
            );
#endif

        IfTrueAbort(_PromoteToBound());

#if IDE 
        // The project is now in Bound state.  Release the mutex lock
        // in case someone is blocked waiting for Bound state.
        //
        m_iBoundChangeIndex++;
        m_eventBoundState.Signal();

        GetCompilerSharedState()->RefreshTaskList();

#endif IDE

#if ID_TEST

#pragma warning (push)
#pragma warning (disable:6237) // In retail builds, VSFSWITCH(fDumpXML) will always be 0
        if (VSFSWITCH(fDumpXML) && !IsMetaData())
        {
            DumpXML();
        }
#pragma warning (pop)

#endif ID_TEST

#if IDE 
        LOG_INFO(
            VB_COMPILATION,
            "Done promoting project %S to Bound (fAbort = %d).",
            GetFileName(),
            fAborted
            );
#endif
    }

    // Tick only if compiled directly, not if compiled indirectly on demand
    // when compiling another project.
    //
    if (!projectBeingCompiled.GetPrevious())
    {
        FinishCompileStep(m_pCompiler);    // Project now in CS_Bound
    }

Abort:

    return fAborted;
}

//============================================================================
// Do the work to compile this project from Bound to Compiled.
//============================================================================
// Return true if aborted, false otherwise

bool
CompilerProject::CompileFromBoundToCompiled
(
)
{
    DebCheckInCompileThread(m_pCompiler);


    bool fAborted = false;
    CompilationState csInit = GetCompState();

    // We need to call this to make sure that this project gets added to the cache
    // regardless of whether it will be compiled or not.
    CacheErrorsInReferences();

    // If we're already Compiled, don't do anything
    if (csInit == CS_Compiled)
    {
        // If the project is already compiled, cache the errors.
        CacheErrorsInCurrentProject();

        return fAborted;
    }

#if IDE 
    LOG_INFO( VB_COMPILATION, "Start promoting project %S (CurState = %d) to Compiled...", GetFileName(), csInit );
#endif

    VSASSERT(csInit >= CS_Bound, "Compiling from Bound to Compiled expects a project in Bound state.");

    UpdateProjectBeingCompiled projectBeingCompiled(m_pCompiler, this);

    m_fCompiledSuccessfully = false;

    // If the output string length is too long, we'll choke down the road when we do anything with ALink.
    // Try to catch it here.
    if (GetPEName() && StringPool::StringLength(GetPEName()) >= _MAX_PATH)
    {
        if (!m_fIsTooLong)
        {
            m_fIsTooLong = true; // Don't spit out the same error.
            m_ErrorTable.CreateError(ERRID_AlinkManifestFilepathTooLong, NULL, GetPEName());

#if IDE 
            GetCompilerSharedState()->RefreshTaskList();
#endif
        }
#if IDE 
        if (GetCompilerPackage())
        {
            GetCompilerSharedState()->GetMessages()->AbortCompile();
        }
#endif IDE

        fAborted = true;
        goto Abort;
    }
    else if (m_fIsTooLong)
    {
        // Get rid of any errors that we might have.
        m_ErrorTable.DeleteSpecificError(ERRID_AlinkManifestFilepathTooLong);
        m_fIsTooLong = false;
    }

    //========================================================================
    // Bring all of the modules to TypesEmitted.
    //========================================================================

    if (csInit < CS_TypesEmitted)
    {
#if LOGTHREADEVENTS
        if (!m_fMetaData)
        {
            DebThreadLogEvent(("Compiler thread Bound->TypesEmitted= %S\n", GetFileName()));
        }
#endif LOGTHREADEVENTS
        IfTrueAbort(_PromoteToTypesEmitted());
    }

#if IDE 
        // The project is now in compiled state.  Release the mutex lock
        // in case someone is blocked waiting for compiled state.
        //
        m_eventTypesEmittedState.Signal();
#endif IDE

#if IDE 

    GetCompilerSharedState()->RefreshTaskList();

#endif IDE

    FinishCompileStep(m_pCompiler);    // Project now in CS_TypesEmitted

    //========================================================================
    // Bring all of the modules to compiled state.
    //========================================================================

    if (csInit < CS_Compiled)
    {
#if LOGTHREADEVENTS
        if (!m_fMetaData)
        {
            DebThreadLogEvent(("Compiler thread Bound->CS_Compiled= %S\n", GetFileName()));
        }
#endif LOGTHREADEVENTS
        IfTrueAbort(_PromoteToCompiled());

#if IDE 
        // The project is now in compiled state.  Release the mutex lock
        // in case someone is blocked waiting for compiled state.
        //
        m_eventCompiledState.Signal();
#endif IDE
    }

    // Tick only if compiled directly, not if compiled indirectly on demand
    // when compiling another project.
    //
    if (!projectBeingCompiled.GetPrevious())
    {
        FinishCompileStep(m_pCompiler);    // Project now in CS_Compiled
    }

    // This compilation will fail if we have any errors or if any of
    // our referenced projects have any errors.
    //
    m_fCompiledSuccessfully = !CacheErrorsInCurrentProject();

Abort:

    
#if IDE 
    // We may have marked errors as stale during compilation.  We should
    // refresh our task list items at the end of compilation to make sure
    // quick fixes are generated.
    if (m_pTaskProvider)
    {
         m_pTaskProvider->TaskListNeedsRefresh();
    }

    LOG_INFO( VB_COMPILATION, "Done promoting project %S to Compiled (fAbort = %d).", GetFileName(), fAborted );
#endif

    return fAborted;
}

//============================================================================
// Do all of the work to compile this project.
//============================================================================
// Return true if aborted, false otherwise

bool
CompilerProject::Compile
(
)
{
    bool fAborted = false;

    IfTrueAbort(CompileToDeclared());
    IfTrueAbort(CompileFromDeclaredToBound());
    IfTrueAbort(CompileFromBoundToCompiled());

    // Any errors in the CompilerProject should have been merged
    // out of the CS_NoStep error bucket into the bucket for the step
    // when the error was generated.  If this assert fires,
    // it means the error merging didn't happen and the error
    // is at risk for being dropped on the floor.
    VSASSERT(!m_ErrorTable.HasErrorsThroughStep(CS_NoStep), "Error not merged correctly?");

Abort:
    return fAborted;
}

//============================================================================
// Ensure that the bases and implements of a metadata project are resolved
//============================================================================

void CompilerProject::EnsureBasesAndImplementsLoaded()
{
    VSASSERT(IsMetaData(), "must be metadata!");
    VSASSERT(GetCompState() >= CS_Declared, "metadata symbols not yet built!!!");

    if (!m_fBasesAndImplementsLoaded)
    {
        CompilerFile *pfile;

        // This used to be in the loop below.  Why?
        m_fBasesAndImplementsLoaded = true;

        for (pfile = m_dlFiles[GetCompState()].GetFirst(); pfile; pfile = pfile->Next())
        {
            MetaImport::ImportBaseAndImplements(pfile->GetCompiler()->GetUnnamedNamespace(pfile), true);
       }
    }
}

//============================================================================
// Do all of the post-compile work.
//============================================================================

void CompilerProject::FinishCompile()
{
    VSASSERT(m_fCompiledSuccessfully, "Shouldn't be called if we weren't succesful.");

#if DEBUG
    if (VSFSWITCH(fTestCrash))
    {
        // Test Watson handling
        VbThrow(E_OUTOFMEMORY);
    }
#endif

#if IDE 
    // The IDE compiler defers final compilation to this point.
    // Check if there is anything to compile to disk.
    if (m_Builder.NeedsCompileToDisk())
    {
        ErrorTable errors(m_pCompiler, this, NULL);
        m_Builder.CompileToDisk(&m_fCompiledSuccessfully, &errors);
        m_fNeedtoUpdateENCMetaData = true;
        m_ErrorTable.Merge(&errors, CS_GeneratedCode, false);
    }
#endif
}

bool CompilerProject::BeginCompileToDiskAsync()
{
    VSASSERT(m_fCompiledSuccessfully, "Shouldn't be called if we weren't succesful.");

    bool result = true;
#if DEBUG
    if (VSFSWITCH(fTestCrash))
    {
        // Test Watson handling
        VbThrow(E_OUTOFMEMORY);
    }
#endif

#if IDE 
    // The IDE compiler defers final compilation to this point.
    // Check if there is anything to compile to disk.
    if (!m_Builder.NeedsCompileToDisk())
    {
        return false;
    }

    result = m_Builder.CompileToDiskAsync();
#endif

    return result;
}

#if IDE 
void CompilerProject::EndCompileToDiskAsync(bool fCompiledSuccessfully, ErrorTable *errors)
{
    m_fCompiledSuccessfully = fCompiledSuccessfully;
    m_fNeedtoUpdateENCMetaData = true;
    m_ErrorTable.Merge(errors, CS_GeneratedCode, false);
}

//============================================================================
// Clear any ParseTreeCache's we might have for sourcefiles in this project
//============================================================================
void CompilerProject::ClearParseTreeCaches()
{
    SourceFileIterator sfi(this);
    SourceFile* pfile;
    SourceFileView* pFileView;
    while ( pfile = sfi.Next() )
    {
        pFileView = pfile->GetSourceFileView();
        if (pFileView)
        {
            ParseTreeService* pts = pFileView->GetParseTreeService();
            if(pts)
              pts->ClearParseTreeCache();
        }
    }

    ParseTreeCacheDataPool& pool = GetCompilerPackage()->GetParseTreeCacheDataPool();
    pool.Clear();
}

//============================================================================
// Actual removal of a compiler file.
//============================================================================


void CompilerProject::RemoveCompilerFile(CompilerFile* pCompilerFile)
{
    BackgroundCompilerLock compilerLock(this);

    SourceFile * pSourceFile = pCompilerFile->PSourceFile();

    // VSW#535416: Keep this object alive until the end of the function
    CComPtr<SourceFile> spSourceFile(pSourceFile);

    //
    // Unfortunately, we need to rebuild the world, not just the
    // files dependent on the file being removed.  Why?
    //
    // Assume we're removing file A, and no one is dependent on it.
    // But assume that file B issued an error because of a name
    // conflict between A and B.  Unless we decompile the entire
    // project, B won't get decompiled and its errors will never
    // be cleared.
    //
    // Removing files is rare, though, so perhaps this isn't as
    // bad as it seems.
    //
    if (GetCompilerSharedState()->IsInRenameSymbol())
    {
        EditFilter::DecompileProject(this, CS_Declared);
        GetCompilerPackage()->GetDecompileQueue()->Enqueue(this, CS_NoState);
    }
    else
    {
        EditFilter::DecompileProject(this, CS_NoState);
    }

    // Even if in batch mode, need to ensure that the demotions have
    // been completed before the file is released.
    //
    if (GetCompilerPackage()->IsInBatch())
    {
        GetCompilerPackage()->CommitEditListDemotions(true, true, this);
    }

    FireOnFile(CLXN_Remove, pSourceFile->GetCodeFile());

    // Unlink it from the project and release it's reference.
    pSourceFile->ReleaseFileFromProject();

    compilerLock.Start();

    // Remove the old Tasklist items
    GetCompiler()->DoRefreshTaskList();
}
#endif // IDE

//****************************************************************************
// CompilerProject implementation
//****************************************************************************

//****************************************************************************
// Project compilation methods.
//
// These methods should only be called as a side-effect of compiling
// a file.
//****************************************************************************

//============================================================================
// Do the work to bring the project to declared state.
//============================================================================

bool CompilerProject::_PromoteToDeclared()
{
    bool fAborted = false;
    CompilerFile *pfile = NULL;
    ErrorTable errors(m_pCompiler, this, NULL);

    // Delete any stale project-level errors from the last time we passed through this state
    m_ErrorTable.DeleteAllWithThisStateOrHigher(CS_Declared);

    // VSW#23631
    // Make sure this error is removed from the error table
    m_ErrorTable.DeleteSpecificError(ERRID_AlinkManifestFilepathTooLong);
    m_fIsTooLong = false;

    if (!IsMetaData())
    {
        // Create the project level cond comp symbols, overwriting any that were created by the parser on the UI thread
        IfFailThrow(CreateProjectLevelCondCompSymbols(&errors));
    }

    STRING *DefaultNamespace = GetDefaultNamespace();
    if ( DefaultNamespace && wcscmp( DefaultNamespace, L"" ) != 0 )
    {
        // Report an error if the namespace is bogus
        bool Error;
        Parser NameParser( &m_nraSymbols, m_pCompiler, GetCompilerHost(), false, GetCompilingLanguageVersion());
        STRING* SafeQualifiedName = MakeSafeQualifiedName(m_pCompiler, DefaultNamespace);
        ParseTree::Name *DefaultNamespaceTree = NameParser.ParseName( Error, SafeQualifiedName);
        STRING* ParsedName = MakeSafeQualifiedName(m_pCompiler, StringFromParseTreeName(m_pCompiler,DefaultNamespaceTree));
        if ( Error ||
            !StringPool::IsEqual(SafeQualifiedName, ParsedName))
        {
            errors.CreateError( ERRID_BadNamespaceName1, NULL, DefaultNamespace );
            m_pstrDefaultNamespace = ParsedName;
        }
    }

    // NOTE NOTE NOTE: You should not be doing ANY allocations using the
    // project-level allocator past this point, or else your data will go
    // away on demotion to Declared
    m_nraSymbols.Mark(&m_markDeclared);

    //========================================================================
    // Bring each file to declared.
    //========================================================================

    // Move all of the files in the project to the next state.
    while (pfile = m_dlFiles[CS_NoState].GetFirst())
    {
        // Check for the compiler interruption BEFORE each
        // module's state promotion.  This prevents a situation
        // where all modules are promoted, but project-level
        // promotion hasn't happened yet.
        //
#if IDE 
        IfTrueAbort(CheckStop(NULL));
#endif IDE

        // Do the promotion.
        IfTrueAbort(pfile->_StepToBuiltSymbols());

        // Mark this module as being in declared state.
        VSASSERT(pfile->m_cs == CS_NoState, "State changed unexpectedly.");

        pfile->m_cs = CS_Declared;
#if IDE 
        pfile->SetDecompilationState(pfile->m_cs);

        // Signal that the file has reached declared
        pfile->m_eventDeclaredState.Signal();

        // Increment our count of compilation step changes
        GetCompilerHost()->IncrementCompilationStepCount();

        if (pfile->IsSourceFile())
        {
            GetCompilerSharedState()->GetMessages()->FileInDeclared(pfile->PSourceFile());
        }
#endif
        VSDEBUGPRINTIF(VSFSWITCH(fDumpStateChanges), "%S promoted to CS_Declared.\n", pfile->m_pstrFileName);

        m_dlFiles[CS_NoState].Remove(pfile);
        if (pfile->IsSolutionExtension())
        {
            m_dlFiles[CS_Declared].InsertFirst(pfile);
        }
        else
        {
            m_dlFiles[CS_Declared].InsertLast(pfile);
        }

        FinishCompileStep(m_pCompiler);
    }

    // The project is now in declared state
    VSDEBUGPRINTIF(VSFSWITCH(fDumpStateChanges), "Project %S promoted to CS_Declared.\n", m_pstrPEName);

    m_ErrorTable.Merge(&errors, CS_BuiltSymbols);

    SetCompState(CS_Declared);
#if IDE 
    SetDecompilationState(m_cs);
#endif

Abort:

#if IDE 

    // Aborting a compile requires that we backtrack on some of our work.  The
    // good news is that we only need to handle the single file that we were
    // processing when the compilation was aborted -- moving a file from "almost
    // declared" back to "no state" won't take any other files with it.
    // That's why this code is so much simpler than the abort code in
    // CompilerProject::_PromoteToBound.
    //
    if (fAborted)
    {
        VSASSERT(pfile != NULL, "How did we abort without being in the middle of processing a file?");

        if (pfile->IsSourceFile() && pfile->m_cs == CS_NoState)
        {
            // Discard the backing field lists we got part way through building
            pfile->PSourceFile()->DiscardStaticLocalsLists();

            pfile->PSourceFile()->GetXMLDocFile()->DemoteXMLDocFile(CS_NoState);

            // Reset the root namespace for this SourceFile.
            pfile->PSourceFile()->SetRootNamespace(NULL);

            // Clear the error table of any errors generated during this step
            //
            pfile->PSourceFile()->GetCurrentErrorTable()->DeleteAll();
        }
    }

#endif IDE

    return fAborted;
}


//============================================================================
// Do the work to bring the project to Bound state.
//============================================================================

bool CompilerProject::_PromoteToBound()
{
    bool fAborted = false;
    CompilerFile *pfile;

    // This array holds a list of all sourcefiles that got promoted from Declared to Bound state.
#if IDE 
    DynamicArray<CompilerFile*> ListOfPromotedSourceFiles;
#endif

    ClearAllMissedBindingOpportunities();
    SetDeclaredToBoundCompilationInProgress(true);

    //========================================================================
    // Reset the binding space for everybody.
    //========================================================================

    //========================================================================
    // Binds to project level imports.
    //========================================================================

    ResolveProjectLevelImports(NULL);

    // NOTE NOTE NOTE: You should not be doing ANY allocations using the
    // project-level allocator past this point, or else your data will go
    // away on demotion to Bound
    m_nraSymbols.Mark(&m_markBound);

#if IDE 
    if (!m_fMetaData)
    {
        IsProjectLevelInformationReady = true;
        // Let everyone know that things have changed if we weren't able to
        // when we initially decompiled
        GetCompilerSharedState()->GetMessages()->FireOnChangedEvents(this);
    }
#endif

    if (IsMetaData())
    {
#if IDE 
        // Rebind the assembly refs that bound to VB projects using the now available Version and PK
        // information that has been extracted from assembly attributes.

        MetaDataFileIterator Files(this);

        while (MetaDataFile *pFile = Files.Next())
        {
            AssemblyRefCollection *pAssemblyRefs = pFile->GetAssemblyRefs();
            ULONG cAssemblyRefs = pAssemblyRefs->GetAssemblyRefCount();

            ERRID ErrorID = 0;
            StringBuffer ErrorString;

            ERRID ErrorIDUsed = 0;
            StringBuffer ErrorStringUsed;

            for(ULONG Index = 0; Index < cAssemblyRefs; Index++)
            {
                AssemblyRefInfo *pCurrentRef = pAssemblyRefs->GetAssemblyRefInfo(Index);

                if (pCurrentRef->m_fCausesReferenceCycle ||
                    !pCurrentRef->m_pReferencedProject ||
                    pCurrentRef->m_pReferencedProject->IsMetaData())
                {
                    continue;
                }

                CompilerProject *pReference = NULL;

                pCurrentRef->m_ErrorID = 0;
                pCurrentRef->m_pstrErrorExtra = NULL;

                ErrorIDUsed = 0;
                ErrorStringUsed.Clear();

                AssemblyRefReferencedProjectsIterator Iter(pCurrentRef);
                while (CompilerProject *pCurrentProj = Iter.Next())
                {
                    ErrorID = 0;

                    if (MetaDataFile::CanBindToVBProject(
                            pCurrentRef,
                            pCurrentProj,
                            m_pCompiler,
                            m_pCompilerHost,
                            false,  // do full match - Name, PK, Version, etc.
                            &ErrorID,
                            &ErrorString))
                    {
                        if (!pReference)
                        {
                            pReference = pCurrentProj;

                            // Set this particular project as the main match for this assembly ref.
                            Iter.SetCurrentProject(pCurrentRef->m_pReferencedProject);
                            pCurrentRef->m_pReferencedProject = pReference;
                        }
                        else
                        {
                            pCurrentRef->m_ErrorID = ERRID_ReferencedProjectsAmbiguous4;

                            if (ErrorID != 0)
                            {
                                ErrorString.Clear();
                            }

                            ResLoadStringRepl(
                                ERRID_ReferencedProjectsAmbiguous4,
                                &ErrorString,
                                pCurrentRef->m_Identity.GetAssemblyIdentityString(),
                                L"|1",      // This will be replaced later when the error is reported.
                                GetErrorProjectName(pReference),
                                GetErrorProjectName(pCurrentProj));

                            pCurrentRef->m_pstrErrorExtra = m_pCompiler->AddString(ErrorString.GetString());

                            ErrorID = 0;
                            ErrorString.Clear();

                            break;
                        }
                    }

                    if (ErrorID != 0)
                    {
                        ErrorIDUsed = ErrorID;
                        ErrorStringUsed.Copy(&ErrorString);
                        ErrorString.Clear();
                    }
                }

                if (!pReference)
                {
                    if (ErrorIDUsed != 0)
                    {
                        pCurrentRef->m_ErrorID = ErrorIDUsed;
                        pCurrentRef->m_pstrErrorExtra = m_pCompiler->AddString(ErrorStringUsed.GetString());
                    }
                    // 



                    else if (!pCurrentRef->m_fVenusPartialNameMatch ||
                             !pCurrentRef->m_pReferencedProject ||
                             !pCurrentRef->m_pReferencedProject->IsVenusAppCodeProject() ||
                             pCurrentRef->m_ErrorID != 0)
                    {
                        // No match ?  Then we have no reference to this assembly.
                        pCurrentRef->m_ErrorID = ERRID_UnreferencedAssembly3;
                        pCurrentRef->m_pstrErrorExtra = pCurrentRef->m_Identity.GetAssemblyIdentityString();
                    }
                }
            }
        }
#endif IDE

        // Bind bases and implemented interfaces.
        EnsureBasesAndImplementsLoaded();
    }

    //========================================================================
    // Bind the sourcefiles
    //========================================================================    
    CompilerFile *pNextFile = m_dlFiles[CS_Declared].GetFirst();
    bool fAnyDelayBinding = false;
    while (pNextFile)
    {
        pfile = pNextFile;

        // Need to get hold of the next file now, because the pfile could be
        // removed by FinishPromoteToBoundForFile from the declared list and
        // put in the bound list, so changing the next.
        //
        pNextFile = pfile->Next();

        // Check for the compiler interruption BEFORE each
        // module's state promotion.  This prevents a situation
        // where all modules are promoted, but the Bound-state
        // mutex hasn't yet been unlocked.
        //
#if IDE 
        IfTrueAbort(CheckStop(NULL));
#endif IDE

        // Do the promotion.
        // But, wait to Generate My.* Collections before concluding
        IfTrueAbort(pfile->_StepToBoundSymbols());

        fAnyDelayBinding |= pfile->IsSourceFile() && pfile->PSourceFile()->DelayBindingCompletion();

        if (!pfile->IsSourceFile() ||
            !pfile->PSourceFile()->DelayBindingCompletion())
        {
            FinishPromoteToBoundForFile(
                pfile
#if IDE 
                ,ListOfPromotedSourceFiles
#endif IDE
                );
        }
    }

    VerifyExtensionAttributeDefinition();

    VerifyEmbeddedReferenceAttributes();

    //========================================================================
    // Process any friend declarations that were ----ed in bindable.
    //========================================================================

    ProcessFriendDeclarations();

    //========================================================================
    // Verify any of the bindings that were missed, and issue an error
    // where appropriate.
    //========================================================================

    VerifyMissedBindingOpportunities();

    //========================================================================
    // Generate MY* collections in the classes with MyCollectionAtribute
    //========================================================================

    // **** No Abort area


    // When a source file decompiles from Compiled/TypesEmmited/Bound to some state bellow bound, then
    // all the files that have MyGroupCollection do decompile to Declared ( see editfilter.cpp(DecompileFileNoCommit())
    // However there are instances like P1 has project to procect references to P1 and a minor edit is done in P2. P1 can be
    // brought to declare while all the source files are keept in bound state. No source file is DelayedBinding so there is no need to
    // inject the My* properties.
    if (m_daMyGroupCollectionInfo.Count() != 0 && fAnyDelayBinding )
    {
        // scan for candidate classes. Can be found in declared with delayed binding or in
        // bound, types emmited ... files

        MyGroupCollectionInfo* arrayOfMyGroupCollectionInfo = m_daMyGroupCollectionInfo.Array();
        ULONG count = m_daMyGroupCollectionInfo.Count();

        // clean existing lists of members then repopulate the groups by a new scan
        for ( unsigned i = 0; i < count; i++)
        {
#if DEBUG
            if (arrayOfMyGroupCollectionInfo[i].m_daMyGroupMembers.Count())
            {
                VSDEBUGPRINTIF(
                    VSFSWITCH(fMyGroupAndDefaultInst),
                    "MyGroup: removing the previous members of MyGroup class: %S\n", arrayOfMyGroupCollectionInfo[i].m_groupClass->GetName());
            }
#endif

            arrayOfMyGroupCollectionInfo[i].m_daMyGroupMembers.Reset();
        }

        for ( int i = CS_Declared; i < CS_MAX; i++)
        {
            for (pfile = m_dlFiles[i].GetFirst(); pfile; pfile = pfile->Next())
            {
                if (pfile->IsSourceFile())
                {
                    VSASSERT( (i > CS_Declared || pfile->PSourceFile()->DelayBindingCompletion()),
                        "A My file in declared state should be bound delayed");
                    Bindable::ScanAndLoadMyGroupCollectionMembers(pfile->PSourceFile()->GetUnnamedNamespace(), this);
                }
            }
        }
        //walk the groups and generate synthetic members;
        for ( unsigned i = 0; i < count; i++)
        {
            Bindable::GenSyntheticMyGroupCollectionProperties(&arrayOfMyGroupCollectionInfo[i], this);
        }
    }

    // complete the rollback mark on allocator and CS_BoundSymbols status for each remaining file
    for (pfile = m_dlFiles[CS_Declared].GetFirst(); pfile; pfile = m_dlFiles[CS_Declared].GetFirst())
    {
        // Check for the compiler interruption BEFORE each
        // module's state promotion.  This prevents a situation
        // where all modules are promoted, but the Bound-state
        // mutex hasn't yet been unlocked.
        //
//#if IDE 
//        IfTrueAbort(CheckStop(NULL));
//#endif IDE

        // Only source file should need to be delay bound
        //
        VSASSERT( pfile->IsSourceFile(),
                        "only source file should get here!!!");
        if ( pfile->PSourceFile()->DelayBindingCompletion() )
        {
            FinishPromoteToBoundForFile(
                pfile
#if IDE 
                ,ListOfPromotedSourceFiles
#endif IDE
                );
        }
    }
    //**** End of No Abort Area

    //========================================================================
    // Some debug stats.
    //========================================================================

#if DEBUG
    if (VSFSWITCH(fDumpBasicRep) && !IsMetaData() && m_pCompiler->GetUnnamedNamespace(this))
    {
        DebPrintf("//============================================================================\n\n");
        Symbols::DebShowBasicRep(m_pCompiler, m_pCompiler->GetUnnamedNamespace(this), 0);
        DebPrintf("\n//============================================================================\n\n");
    }

    VSDEBUGPRINTIF(VSFSWITCH(fDumpStateChanges), "Project %S promoted to CS_Bound.\n", m_pstrPEName);

#endif DEBUG

    SetCompState(CS_Bound);

#if IDE 
    SetDecompilationState(m_cs);

    // Signal that project has reached bindable.
    GetCompilerSharedState()->GetMessages()->ProjectInBindable(this);
#endif

Abort:

#if IDE 
    for (ULONG i = 0; i < ListOfPromotedSourceFiles.Count(); ++i)
    {
        // Let the drop downs know that this file has reached bound state.  Note that UI consumers who
        // expect the entire project to be bound should listen to ProjectInBindable because the
        // project may not be entirely bound.
        GetCompilerSharedState()->GetMessages()->FileInBindable((ListOfPromotedSourceFiles.Array()[i])->PSourceFile());
    }
#endif

    SetDeclaredToBoundCompilationInProgress(false);

#if IDE 
    if (fAborted)
    {
        ClearProjectDeclaredToBoundData();
    }

    // The IDE has it's own compliation cache which includes a merged namespace cache. It's possible that the
    // cache may include namespaces that also exist in this newly bound project (for instance unnamed namespace).
    // Normally IDE clears the cache on decompilation, but in ASL scenario as new projects get bound the cache
    // might become stale. We need to let IDE know that the cache should be refreshed to include possibly new 
    // namespaces from this project (see Dev11 386116).
    GetCompilerSharedState()->SetIDECompilationCachesNeedRefresh(true);
#endif

    return fAborted;
}

void
CompilerProject::ResolveProjectLevelImports
(
    CompilationCaches *pCompilationCaches
)
{
    if (!this ||
        HaveImportsBeenResolved())
    {
        return;
    }

    VSASSERT(!AreImportsBeingResolved(), "Project import dependency cycles unexpected!!!");

#if DEBUG
    SetImportsBeingResolved(true);
#endif

    // Delete any stale project-level errors from the last time we passed through this state
    m_ErrorTable.DeleteAllWithThisStateOrHigher(CS_Bound);

    m_Imports.ReBuildAllImportTargets();

    // Resolve imports and merge errors
    //
    if (m_Imports.GetAllImportedTargets())
    {
        Bindable::ResolveImportsList(
            m_Imports.GetAllImportedTargets(),                      // List of project level imports
            NULL,                                                   // No associated source file
            this,                                                   // Associated project
            &m_nraSymbols,                                          // Allocator
            NULL,                                                   // No associated line marker table
            m_Imports.GetImportsErrorLog(),                         // use the special imports error table to put any imports errors in
            GetCompiler(),
            pCompilationCaches);                                    // The current compiler instance
    }

#if DEBUG
    SetImportsBeingResolved(false);
#endif

    SetImportsBeenResolved(true);

#if IDE 
    VSASSERT(this->IsBoundCompilationInProgress(), "Project level imports resolution unexpected!!!");
#endif

    // Note that this needs to be done only after setting ImportedResolve to true,
    // else there could possibly be circular dependencies.
    //
    if (m_Imports.GetAllImportedTargets())
    {
        Symbols SymbolFactory(GetCompiler(), &m_nraSymbols, NULL);
        ErrorTable *ImportsErrorLog = m_Imports.GetImportsErrorLog();

        for(ImportedTarget *Import = m_Imports.GetAllImportedTargets();
            Import;
            Import = Import->m_pNext)
        {
            Bindable::CheckGenericConstraintsForImportsTarget(
                Import,
                m_Imports.GetImportsErrorLog(),                     // use the special imports error table to put any imports errors in
                GetCompilerHost(),
                GetCompiler(),
                &SymbolFactory,
                pCompilationCaches);
        }
    }

    m_Imports.DumpImportsErrors(&m_ErrorTable);

    m_ErrorTable.MergeNoStepErrors(CS_BoundSymbols);
}

void CompilerProject::SetProjectSettingForKeyFileName
(
    _In_opt_z_ STRING *pstrKeyFileName
)
{
    m_pstrProjectSettingForKeyFileName = pstrKeyFileName;

    if (StringPool::IsEqual(pstrKeyFileName, STRING_CONST(m_pCompiler, EmptyString)))
    {
        pstrKeyFileName = NULL;
    }

    if (pstrKeyFileName)
    {
        m_AssemblyIdentity.SetKeyFileName(pstrKeyFileName);
        m_fKeyNameFromAttr = false;
    }
    else
    {
        if (m_pFileWithAssemblyKeyFileAttr)
        {
            m_AssemblyIdentity.SetKeyFileName(m_pFileWithAssemblyKeyFileAttr->GetKeyFileName());
            m_fKeyNameFromAttr = true;
        }
        else
        {
            m_AssemblyIdentity.SetKeyFileName(NULL);
            m_fKeyNameFromAttr = false;
        }
    }
}

void CompilerProject::SetProjectSettingForKeyContainerName
(
    _In_opt_z_ STRING *pstrKeyContainerName
)
{
    m_pstrProjectSettingForKeyContainerName = pstrKeyContainerName;

    if (StringPool::IsEqual(pstrKeyContainerName, STRING_CONST(m_pCompiler, EmptyString)))
    {
        pstrKeyContainerName = NULL;
    }

    if (pstrKeyContainerName)
    {
        m_AssemblyIdentity.SetKeyContainerName(pstrKeyContainerName);
        m_fKeyNameFromAttr = false;
    }
    else
    {
        if (m_pFileWithAssemblyKeyNameAttr)
        {
            m_AssemblyIdentity.SetKeyContainerName(m_pFileWithAssemblyKeyNameAttr->GetKeyContainerName());
            m_fKeyNameFromAttr = true;
        }
        else
        {
            m_AssemblyIdentity.SetKeyContainerName(NULL);
            m_fKeyNameFromAttr = false;
        }
    }
}

void
CompilerProject::FinishPromoteToBoundForFile
(
    CompilerFile *pfile
    // This array holds a list of all sourcefiles that got promoted from Declared to Bound state.
#if IDE 
    ,DynamicArray<CompilerFile*> &ListOfPromotedSourceFiles
#endif
)
{
    pfile->CompleteStepToBoundSymbols();

    // Mark this module as being in Bound state.
    VSASSERT(pfile->m_cs == CS_Declared, "State changed unexpectedly.");
    pfile->m_cs = CS_Bound;

    VSDEBUGPRINTIF(VSFSWITCH(fDumpStateChanges), "%S promoted to CS_Bound.\n", pfile->m_pstrFileName);

    m_dlFiles[CS_Declared].Remove(pfile);


#if IDE 
    pfile->SetDecompilationState(pfile->m_cs);

    // Increment our count of compilation step changes
    GetCompilerHost()->IncrementCompilationStepCount();

    if (pfile->IsSourceFile())
    {
        pfile->PSourceFile()->m_iCurrentBoundChangeIndex++;
        pfile->PSourceFile()->m_iCurrentBindingChangeIndex++;

        // Add the file to the list of promoted files so that the FileInbindable event can be fired later.
        ListOfPromotedSourceFiles.AddElement(pfile);
    }
#endif

    m_dlFiles[CS_Bound].InsertLast(pfile);

    FinishCompileStep(m_pCompiler);
}


//============================================================================
// Do the work to bring the project to Compiled state.
//============================================================================

bool CompilerProject::_PromoteToTypesEmitted()
{
    VSASSERT(m_cs < CS_TypesEmitted, "Attempt to promote a project to TypesEmitted that is already in this state.");

    bool fAborted = false;

    // Delete any stale project-level errors from the last time we passed through this state
    m_ErrorTable.DeleteAllWithThisStateOrHigher(CS_TypesEmitted);

    // NOTE NOTE NOTE: You should not be doing ANY allocations using the
    // project-level allocator past this point, or else your data will go
    // away on demotion to TypesEmitted
    m_nraSymbols.Mark(&m_markTypesEmitted);

    if (!m_fMetaData)
    {
#if HOSTED
        // Fix for Dev10 bug 722855 - to reduce the size of hosted compiler to help
        // reduce the size of the .Net FX download. This is accomplished in a low
        // risk manner by compiling out the below code which helps with parts of
        // PEBuilder (and maybe some portions of MetaEmit too) to not be linked in.

        VSFAIL("Compilation code path not valid for hosted compiler!");
        VbThrow(E_UNEXPECTED);
#else

        // Perform the CLS-compliance tests on the project
        ErrorTable CLSComplianceErrors(m_pCompiler, this, NULL);
        CheckCLSCompliance(&CLSComplianceErrors);

        // Merge the CLS Compliance project level errors
        m_ErrorTable.Merge(&CLSComplianceErrors, CS_CheckCLSCompliance);

        // Check for any casing mismatches for namespaces across the different
        // files in the project and warn.
        CheckForNamespaceNameCasingMismatches();

        //========================================================================
        // Emit the metadata for this project.  This call must closely manage
        // the order we promote stuff so we allow it to do the final promotions.
        //========================================================================

        ErrorTable EmitMetadataErrors(m_pCompiler, this, NULL);
        IfTrueAbort(m_Builder.EmitTypeMetadata(&EmitMetadataErrors));

        // Merge the errors.
        m_ErrorTable.Merge(&EmitMetadataErrors, CS_EmitTypeMembers);

#if IDE 
        // Reset the flags that indicates whether any emitting compilation tasks
        // were skipped for this project.
        m_fILEmitTasksSkippedDuringDebugging = false;
#endif IDE

#endif HOSTED
    }
    else
    {
        CompilerFile *pFile;

        // Promote all files in this project from CS_Bound to CS_TypesEmitted
        for (pFile = m_dlFiles[CS_Bound].GetFirst(); pFile; pFile = m_dlFiles[CS_Bound].GetFirst())
        {
            m_dlFiles[CS_Bound].Remove(pFile);
            m_dlFiles[CS_TypesEmitted].InsertLast(pFile);
            pFile->m_cs = CS_TypesEmitted;
#if IDE 
            pFile->SetDecompilationState(pFile->m_cs);

            // Increment our count of compilation step changes
            GetCompilerHost()->IncrementCompilationStepCount();
#endif
            FinishCompileStep(m_pCompiler);
        }
    }

    SetCompState(CS_TypesEmitted);

#if !HOSTED
Abort:
#endif

#if IDE

    // Aborting a compile requires that we backtrack on some of our work.  Bummer.
    if (fAborted)
    {
        for (CompilerFile *pFile = m_dlFiles[CS_Bound].GetFirst(); pFile; pFile = pFile->Next())
        {
            if (pFile->IsSourceFile())
            {
                pFile->PSourceFile()->_DemoteFromPossiblyIncompleteTypesEmittedState();
            }
        }

        EditFilter::DecompileProjectNoCommit(this, CS_Bound);
    }

#endif IDE

    return fAborted;
}

//============================================================================
// Do the work to bring the project to Compiled state.
//============================================================================
bool CompilerProject::_PromoteToCompiled()
{
    bool fAborted = false;
    bool fGeneratedOutput = false;
    CompilerFile *pFile = NULL;

    // Delete any stale project-level errors from the last time we passed through this state
    m_ErrorTable.DeleteAllWithThisStateOrHigher(CS_Compiled);

    if (!m_fMetaData)
    {
#if HOSTED
        // Fix for Dev10 bug 722855 - to reduce the size of hosted compiler to help
        // reduce the size of the .Net FX download. This is accomplished in a low
        // risk manner by compiling out the below code which helps with parts of
        // PEBuilder (and maybe some portions of MetaEmit too) to not be linked in.

        VSFAIL("Compilation code path not valid for hosted compiler!");
        VbThrow(E_UNEXPECTED);
#else

        PEBuilder::CompileType compileType;
#if IDE 
        compileType = PEBuilder::CompileType_DeferWritePE;
#else
        compileType = PEBuilder::CompileType_Full;
#endif IDE

        ErrorTable errors(m_pCompiler, this, NULL);
        BYTE **ppbImage = NULL;

        if (m_fInMemoryCompilation)
        {
            // Free old PE memory image before creating a new one.
            ::CoTaskMemFree(m_pImage);
            m_pImage = NULL;

            ppbImage = (BYTE **)&m_pImage;
            compileType = PEBuilder::CompileType_InMemory;
        }

        // The PE builder does all of this work.  Defer to it.
        IfTrueAbort(m_Builder.Compile(compileType, &fGeneratedOutput, &errors, ppbImage));

        // Merge the errors.
        m_ErrorTable.Merge(&errors, CS_GeneratedCode);

#endif HOSTED
    }
    else
    {
        // Promote each file from CS_TypesEmitted to CS_Compiled
        for (pFile = m_dlFiles[CS_TypesEmitted].GetFirst(); pFile; pFile = m_dlFiles[CS_TypesEmitted].GetFirst())
        {
            // We do this manually here.
            IfTrueAbort(pFile->_StepToEmitMethodBodies());

            // Mark this module as being in compiled state.
            VSASSERT(pFile->m_cs == CS_TypesEmitted, "State changed unexpectedly.");
            pFile->m_cs = CS_Compiled;
#if IDE 
            pFile->SetDecompilationState(pFile->m_cs);

            // Increment our count of compilation step changes
            GetCompilerHost()->IncrementCompilationStepCount();
#endif
            VSDEBUGPRINTIF(VSFSWITCH(fDumpStateChanges), "%S promoted to CS_Compiled.\n", pFile->m_pstrFileName);

            m_dlFiles[CS_TypesEmitted].Remove(pFile);
            m_dlFiles[CS_Compiled].InsertLast(pFile);

            FinishCompileStep(m_pCompiler);
        }
    }

    SetCompState(CS_Compiled);
#if IDE 
    SetDecompilationState(m_cs);
#endif

#if DEBUG
    if ((VSFSWITCH(fDumpSymbols) && !IsMetaData()) || (VSFSWITCH(fDumpNonVBSymbols) && IsMetaData()))
    {
        if (m_pCompiler->GetUnnamedNamespace(this))
        {
            BCSYM_Namespace *StartNamespace, *CurrentNamespace;

            if (IsMetaData())
            {
                EnsureBasesAndImplementsLoaded();
            }

            StartNamespace = CurrentNamespace = m_pCompiler->GetUnnamedNamespace(this);

            DebPrintf("//============================================================================\n\n");

            do
            {
                Symbols::DebShowSymbols(CurrentNamespace, m_pCompiler);
                CurrentNamespace = CurrentNamespace->GetNextNamespaceInSameProject();
            } while(StartNamespace != CurrentNamespace);

            DebPrintf("\n//============================================================================\n\n");
        }
    }

    VSDEBUGPRINTIF(VSFSWITCH(fDumpStateChanges), "Project %S promoted to CS_Compiled.\n", m_pstrPEName);
#endif // DEBUG

Abort:

#if IDE 

    // Aborting a compile requires that we backtrack on some of our work.  Bummer.
    if (fAborted)
    {
        // 






        // For going from TypesEmitted to Compiled, we need to unstep whatever
        // step we've been working on.
        //
        for (pFile = m_dlFiles[CS_TypesEmitted].GetFirst(); pFile; pFile = pFile->Next())
        {
            if (pFile->IsSourceFile())
            {
                pFile->PSourceFile()->_DemoteFromPossiblyIncompleteCompiledState();
            }
        }

        // 

        {
            // Since there is no way to delete emitted metadata,
            // decompile to below CS_TypesEmitted to restarted emitting metadata.

            EditFilter::DecompileProjectNoCommit(this, CS_Bound);
        }
    }

#endif IDE

    return fAborted;
}

void CompilerProject::CheckCLSCompliance
(
    ErrorTable *pProjectLevelErrors
)
{
    VSASSERT(!IsMetaData(),
                "Metadata projects should not be checked for CLS compliance!!!");

    CompilerFile *pFile;

#if IDE 
    // Only redo CLS Compliance checking for the whole projects if this setting
    // changed at the project level, else retain whatever checking was done
    // during the previous transition to TypesEmitted.
    //
    // Note: If any file goes below bound, this will anyway get redone for that
    // file.

    bool DidProjectClaimCLSCompliancePreviously =
        this->ProjectClaimedCLSCompliancePreviously();

    bool DoesProjectClaimCLSComplianceCurrently =
        this->ProjectClaimsCLSCompliance();

    bool NeedToReStartCLSComplianceCheckingForWholeProject =
        DidProjectClaimCLSCompliancePreviously != DoesProjectClaimCLSComplianceCurrently;

    if (NeedToReStartCLSComplianceCheckingForWholeProject)
    {
        this->SetProjectClaimedCLSCompliancePreviously(DoesProjectClaimCLSComplianceCurrently);

        // If need to restart CLS Compliance checking for the whole
        // project, then invalidate all the CLS Compliance state
        // for all the files in the project. This is also invalidated
        // when a file goes down to Bound.
        //
        for (pFile = this->m_dlFiles[CS_Bound].GetFirst(); pFile; pFile = pFile->Next())
        {
            VSASSERT(pFile->IsSourceFile(), "Non-source file in source project.");

            pFile->PSourceFile()->ClearCLSComplianceState();
        }
    }
#endif IDE

    // Verify root namespace name for cls compliance
    //
    CLSComplianceChecker::VerifyNameofProjectRootNamespaceIsCLSCompliant(
    this,
    pProjectLevelErrors);

    // Verify all the files for cls compliance
    //
    for (pFile = this->m_dlFiles[CS_Bound].GetFirst(); pFile; pFile = pFile->Next())
    {
        VSASSERT(pFile->IsSourceFile(), "Non-source file in source project.");

#if IDE 
        if (!pFile->PSourceFile()->IsCLSComplianceCheckingDone())
#endif
        {
            CLSComplianceChecker::VerifyCLSCompliance(pFile->PSourceFile());

            // No other errors are expected during this step, so as well as merge the errors here
            pFile->PSourceFile()->MergeCurrentErrors();

#if IDE 
            pFile->PSourceFile()->SetIsCLSComplianceCheckingDone(true);
#endif
        }
    }

    // Merge again in case there were partial types because the errors for the partial types
    // are generated when processing the file containing the main type. It is better to do this
    // here than exposing the partial type complexity to the different core compiler tasks.
    //
    for (pFile = this->m_dlFiles[CS_Bound].GetFirst(); pFile; pFile = pFile->Next())
    {
        VSASSERT(pFile->IsSourceFile(), "Non-source file in source project.");

        if (pFile->PSourceFile()->HasPartialTypes())
        {
            bool const fPurgeExisitingErrorsWithThisStep = true;
            pFile->PSourceFile()->MergeCurrentErrors(!fPurgeExisitingErrorsWithThisStep);
        }

        // We do this manually here.
        pFile->_StepToCheckCLSCompliance();
    }
}

void CompilerProject::CheckForNamespaceNameCasingMismatches()
{
    NorlsAllocator TempAllocator(NORLSLOC);
    NamedRootTree CheckedNamespaceRings(&TempAllocator);

    DynamicArray<SourceFile *> *pdaSourceFilesSorted = this->GetFileCompilationArray();

    for(ULONG i = 0; i < pdaSourceFilesSorted->Count(); i++)
    {
        if (pdaSourceFilesSorted->Element(i)->GetUnnamedNamespace())
        {
            // Dev11#372606: users can declare "Namespace Global", and so we have to start our search at global
            CheckForNamespaceNameCasingMismatches(pdaSourceFilesSorted->Element(i)->GetUnnamedNamespace(), &CheckedNamespaceRings, pdaSourceFilesSorted->Element(i)->GetRootNamespace());
        }
    }
}

void CompilerProject::CheckForNamespaceNameCasingMismatches
(
    BCSYM_Namespace *pNamespaceToCheck,
    NamedRootTree *CheckedNamespaceRings,
    BCSYM_Namespace *pRootNamespace
)
{

    for(BCSYM_Container *pContainer = pNamespaceToCheck->GetNestedTypes();
        pContainer;
        pContainer = pContainer->GetNextNestedType())
    {

        if (!pContainer->IsNamespace())
        {
            continue;
        }

        // Check only if this namespace has not been checked when checking other source files.
        //
        BCSYM_Namespace *pFirstNamespace = pContainer->PNamespace();
        BCSYM_NamedRoot *pCurrentRing = pFirstNamespace->GetNamespaceRing();
        if (!CheckedNamespaceRings->Find(&pCurrentRing))
        {
            CheckedNamespaceRings->Insert(&pCurrentRing);

            // VS2010 behavior was to pick the first name in the namespace ring (usually the lexically last
            // one that the user wrote), declare this as Canonical, and give a warning for every different
            // capitalization of the namespace that was made in the current project. Also, VS2010 only
            // ever searched the descendents of the root namespace; never the root namespace itself nor
            // any of its peers. (there weren't any peers in the absence of Namespace Global!)
            // e.g. rootnamespace="", "Namespace System : ... : Namespace SYSTEM"  ' gives a warning on "System"
            //
            // Ideally we'd like the canonical capitalization to be [1] "the one defined in external projects",
            // or failing that [2] "the one specified in /rootnamespace", or failing that [3] "the first one
            // lexically declared".
            // But [1] would lead to breaking changes (e.g. rootnamespace="" and user just writes "Namespace SYSTEM").
            // For [2], we'll do a workaround for this specific case
            // And [3] is just too nondeterministic to risk messing with.
            //
            // Algorithm for [2]: if we're in a ring that includes the root namespace, then pick it.
            BCSYM_Namespace *pCanonicalNamespace = pFirstNamespace;
            do
            {
                if (pCanonicalNamespace == pRootNamespace) break;
                pCanonicalNamespace = pCanonicalNamespace->GetNextNamespace();
            } while (pCanonicalNamespace != pFirstNamespace);


            SpellingInfo * pCanonicalSpelling = pCanonicalNamespace->GetSpellings();
            SpellingInfo defaultSpellingInfo;
            if (pCanonicalSpelling == NULL)
            {
                // In the case of the project root namespace, its spelling hasn't been entered, so we'll synthesize it here
                defaultSpellingInfo.m_Location.SetLocationToHidden();
                defaultSpellingInfo.m_pSpelling = pCanonicalNamespace->GetName();
                pCanonicalSpelling = &defaultSpellingInfo;
            }

            BCSYM_Namespace * pCurrentNamespace = pFirstNamespace;
            bool foundError = false;
            do
            {
                if (pCurrentNamespace->GetSourceFile() && pCurrentNamespace->GetSourceFile()->GetCompilerProject() == this)
                {
                    SpellingInfoIterator iterator(pCurrentNamespace->GetSpellings());

                    SpellingInfo * pSpellingInfo;

                    while (pSpellingInfo = iterator.Next())
                    {
                        if (!StringPool::IsEqualCaseSensitive(pSpellingInfo->m_pSpelling, pCanonicalSpelling->m_pSpelling ))
                        {
                            // "Casing for the Namespace name '|1' does not match the casing for the Namespace name '|2' in file '|3'."
                            pCurrentNamespace->GetSourceFile()->GetCurrentErrorTable()->CreateError
                            (
                                WRNID_NamespaceCaseMismatch3,
                                &pSpellingInfo->m_Location,
                                pSpellingInfo->m_pSpelling,
                                pCanonicalSpelling->m_pSpelling,
                                (pCanonicalNamespace->GetSourceFile()->IsSolutionExtension() || pCanonicalSpelling->m_Location.IsHidden() ?
                                    L"<Project Settings>" :
                                    pCanonicalNamespace->GetSourceFile()->GetFileName()
                                )
                            );
                            // 



                            // Warning should only be reported for the first mismatch for each namespace to
                            // avoid reporting a large number of warnings in projects with many files.
                            // This is by design.
                            //
                            foundError = true;
                            break;

                        }
                    }
                }

                pCurrentNamespace = pCurrentNamespace->GetNextNamespace();
            } while (pCurrentNamespace != pFirstNamespace && !foundError);
        }

        // Recursively check the child namespaces too.
        CheckForNamespaceNameCasingMismatches(pFirstNamespace, CheckedNamespaceRings, pRootNamespace);
    }
}

//****************************************************************************
// Project decompilation methods.
//
// These methods should never be called directly, only as a side-effect of
// decompiling a file.
//****************************************************************************

#if IDE 

//============================================================================
// Demote all referencing projects to the desired state.
//============================================================================

void CompilerProject::DemoteReferencingProjects
(
    CompilationState CompState
)
{
    CompilerProject *pCompilerProject = NULL;

    ReverseReferenceIterator RefIterator(this);
    while (pCompilerProject = RefIterator.Next())
    {
        pCompilerProject->_DemoteProject(CompState);
    }
}

//============================================================================
// We're about to demote one of this project's file to CS_NoState.  So
// throw away the project-level hash table.
//============================================================================

void CompilerProject::_DemoteToNoState()
{
    IfFalseThrow(GetCompilerSharedState()->IsBackgroundCompilationBlocked());
    _DemoteToDeclared();    // First step down to next state

    if (m_cs <= CS_NoState)
    {
        return;     // Already in state -- nothing left to do
    }

    m_iDeclChangeIndex++;
    m_eventDeclaredState.Unsignal();

    // Throw away the conditional compilation symbol.  We need to do this to
    // make sure that any errors in the CondCompSym list are reported
    // the next time we try to get to CS_Declared.  See VS RAID 211541.
    SetProjectLevelCondCompSymbols(NULL);

    // Throw the hash table and symbols away.
    m_nraSymbols.FreeHeap();

    // Decompilation of the VB runtime means the symbols in the remapping table
    // are stale and need to be cleared. If there is no VB runtime project, assume
    // that we're building the VB runtime and the current project is the VB runtime.
    if (GetCompilerHost()->GetDefaultVBRuntimeProject() == NULL)
    {
        GetCompilerHost()->ClearRuntimeCallRemapTable();
    }
 
    VSASSERT(m_cs == CS_Declared, "Unexpected state change");
    SetCompState(CS_NoState);

    // Need to demote source projects to CS_Declared when a project it references
    // is decompiled to either to CS_NoState. This special decompilation logic
    // seems to be needed because dependencies are not tracked for project level
    // imports.
    // 







    DemoteReferencingProjects(CS_Declared);

    // Attribute changes will cause us to go to NoState.
    // Clear the friends list.
    ClearFriends();

    VSDEBUGPRINTIF(VSFSWITCH(fDumpStateChanges), "Project %S demoted to CS_NoState.\n", m_pstrPEName);
}

//============================================================================
// Mark the project as no longer being in Bound state.
//============================================================================

void CompilerProject::_DemoteToDeclared()
{
    IfFalseThrow(GetCompilerSharedState()->IsBackgroundCompilationBlocked());
    _DemoteToBound();    // First step down to next state

    if (m_cs <= CS_Declared)
    {
        VSASSERT(!HaveImportsBeenResolved(), "Partially bound source project unexpected!!!");

        return;     // Already below state -- nothing left to do
    }

    m_iBoundChangeIndex++;
    m_eventBoundState.Unsignal();

    // We only clear this cache when demoting to Declared.  Compilation from Declared to Bound
    // may be interrupted, in which case SourceFiles which have already been promoted to Bound
    // will not be recompiled.  This means extension methods defined in those SourceFiles would
    // be missing from our cache if we cleared it below in ClearProjectDeclaredToBoundData.
    m_ExtensionMethodExistsCache.Clear();

    ClearCachedBoundMethodBodies();
    ClearProjectDeclaredToBoundData();

    VSASSERT( GetCompilerHost() != NULL, "Expected to still have a compiler host when demoting to declared" );
    GetCompilerHost()->ClearRuntimeHelperSymbolCaches(this);

    VSASSERT(m_cs == CS_Bound, "Unexpected state change");
    SetCompState(CS_Declared);

    // Tell compiler package that this project was decompiled to declared.
    GetCompilerPackage()->OnProjectDemotedToDeclared(this);

    VSDEBUGPRINTIF(VSFSWITCH(fDumpStateChanges), "Project %S demoted to CS_Declared.\n", m_pstrPEName);
}

void CompilerProject::ClearProjectDeclaredToBoundData()
{
    VSASSERT(m_cs == CS_Declared || m_cs == CS_Bound, "Unexpected state change");

    IsProjectLevelInformationReady = false;

    m_nraSymbols.Free(&m_markDeclared);

    // - Project level imports are cleared before binding, so no need to clear here.

#if DEBUG
    SetImportsBeingResolved(false);
#endif

    SetImportsBeenResolved(false);

    if (m_fCachedEntryPoints)
    {
        m_daEntryPoints.Reset();
        m_fCachedEntryPoints = false;
    }

    ClearIDEExtensionMethodCache();
}

//============================================================================
// Mark the project as no longer being in TypesEmitted state.
//============================================================================

void CompilerProject::_DemoteToBound()
{
    IfFalseThrow(GetCompilerSharedState()->IsBackgroundCompilationBlocked());

    // when demoting, we could have a Bound method cached in m_BoundMethodBodyMRUList
    // which could have been using CompilationCaches, which get cleared on every decompile.
    // thus we need to clear the cache via ClearCachedBoundMethodBodies
    //(see bug Devdiv 841426)
    ClearCachedBoundMethodBodies();


    _DemoteToTypesEmitted();    // First step down to next state
    
    // Since _PromoteToTypesEmitted() can be aborted, we should
    // conservatively run the demotion logic if m_cs == CS_Bound
    if (m_cs < CS_Bound)
    {
        return;     // Already below state -- nothing left to do
    }

    m_eventTypesEmittedState.Unsignal();

    m_nraSymbols.Free(&m_markBound);


    // If we have no files then destroy our PE State state.
    if (!m_pfileList)
    {
        m_Builder.Destroy();
    }

    // Forget about all the PIA type references that come from declarations.
    // These references occur when the project is in state CS_Bound on the way to CS_TypesEmitted, so we 
    // need to clear them on the way back to CS_Bound. This fixes bug #510410.
    m_DeclarationPiaTypeRefCache.Clear();
    m_DeclarationPiaMemberCache.Clear();

    if (m_cs == CS_Bound)
    {
        return;
    }

    VSASSERT(m_cs == CS_TypesEmitted, "Unexpected state change");
    SetCompState(CS_Bound);

    VSDEBUGPRINTIF(VSFSWITCH(fDumpStateChanges), "Project %S demoted to CS_Bound.\n", m_pstrPEName);
}

//============================================================================
// Mark the project as no longer being in compiled state.
//============================================================================

void CompilerProject::_DemoteToTypesEmitted()
{
    // No next state to step down to first....
    IfFalseThrow(GetCompilerSharedState()->IsBackgroundCompilationBlocked());
    
    // Since _PromoteToCompiled() can be aborted, we should
    // conservatively run the demotion logic if m_cs == CS_TypesEmitted
    if (m_cs < CS_TypesEmitted)
    {
        return;     // Already below state -- nothing left to do
    }

    m_eventCompiledState.Unsignal();

    m_nraSymbols.Free(&m_markTypesEmitted);

    // Get rid of existing PEBuilder statue
    m_Builder.HandleDemoteToTypesEmitted();

    // Clear cache of unused references
    m_strUnusedReferences.Clear();

    // Clear the embedded resource emitted flag.
    m_fEmbeddedResourcesEmitted = false;

    // Empty the cache of PIA types referenced from method bodies.
    m_MethodBodyPiaTypeRefCache.Clear();
    m_MethodBodyPiaMemberCache.Clear();

    if (m_cs == CS_TypesEmitted)
    {
        return;
    }

    VSASSERT(m_cs == CS_Compiled, "Unexpected state change");
    SetCompState(CS_TypesEmitted);

    VSDEBUGPRINTIF(VSFSWITCH(fDumpStateChanges), "Project %S demoted to CS_TypesEmitted.\n", m_pstrPEName);
}

//============================================================================
// Process a project level demotion
//============================================================================

void CompilerProject::_DemoteProject
(
    CompilationState cs // Compilation state to demote to
)
{
    VSASSERT(cs != CS_Compiled, "Invalid parameter");
    IfFalseThrow(GetCompilerSharedState()->IsBackgroundCompilationBlocked());

    CompilationState CurrentState = GetCompState();

    if (CurrentState > cs)
    {
        if (IsMetaData())
        {
            // Metadata projects currently cannot decompile to CS_Declared, but are instead
            // decompiled to CS_NoState.
            // 

            if (cs < CS_Bound)
            {
                _DemoteMetaDataProjectToNoState(false);
            }
            else
            {
                SetCompState(cs);
            }
        }
        else
        {
            // Any decompilation will cause us to reset this flag.
            m_Builder.ImageOnDiskIsNotUpToDate();

            switch(cs)
            {
            case CS_TypesEmitted:
#if IDE 
                LOG_INFO( VB_DECOMPILATION, L"Start decompiling project %S to TypesEmitted...", GetFileName() );
#endif
                _DemoteToTypesEmitted();
#if IDE 
                LOG_INFO( VB_DECOMPILATION, L"Done decompiling project %S to TypesEmitted.", GetFileName() );
#endif
                break;

            case CS_Bound:
#if IDE 
                LOG_INFO( VB_DECOMPILATION, L"Start decompiling project %S to Bound...", GetFileName() );
#endif
                _DemoteToBound();
#if IDE 
                LOG_INFO( VB_DECOMPILATION, L"Done decompiling project %S to Bound.", GetFileName() );
#endif
                break;

            case CS_Declared:
#if IDE 
                LOG_INFO( VB_DECOMPILATION, L"Start decompiling project %S to Declared...", GetFileName() );
#endif
                _DemoteToDeclared();
#if IDE 
                LOG_INFO( VB_DECOMPILATION, L"Done decompiling project %S to Declared.", GetFileName() );
#endif
                break;

            case CS_NoState:
#if IDE 
                LOG_INFO( VB_DECOMPILATION, L"Start decompiling project %S to NoState...", GetFileName() );
#endif
                _DemoteToNoState();
#if IDE 
                LOG_INFO( VB_DECOMPILATION, L"Done decompiling project %S to NoState.", GetFileName() );
#endif
                break;
            }
        }
    }

#if !defined( VBDBGEE ) 
    // Record the decompilation.
    if( g_pCompilerPackage->IsDecompilationTestHookSet() )
    {
        g_pCompilerPackage->GetDecompilationTestHook()->AddProjectDecompilationState( this, m_cs );
    }
#endif // !VBDBGEE

}

//============================================================================
// Process a project level demotion
// if a pfile is passed in, it's because it's being moved between two states.
// If there's no pfile, don't try moving it (AddFile Scenario)
//============================================================================

void CompilerProject::_ProcessDemotion
(
    CompilerFile *pfile, // File we're demoting
    CompilationState cs // Compilation state to demote to
)
{
    _DemoteProject(cs);

    if (pfile)
    {
        // Process the file.
        m_dlFiles[cs + 1].Remove(pfile);
        m_dlFiles[cs].InsertLast(pfile);
    }
}

//============================================================================
//============================================================================
void CompilerProject::_DemoteAllMetaDataProjectsToNoState()
{
    // Decompile MetaData projects to NoState.
    IfFalseThrow(GetCompilerSharedState()->IsBackgroundCompilationBlocked());
    CompilerProject *pCompilerProject = NULL;
    ProjectIterator projects(GetCompilerHost());

    while (pCompilerProject = projects.Next())
    {
        if (pCompilerProject->IsMetaData() &&
            pCompilerProject != pCompilerProject->GetCompilerHost()->GetComPlusProject())
        {
            EditFilter::DecompileProjectNoCommit(pCompilerProject, CS_NoState); // Does not release the MetaDataImporter
            EditFilter::DecompileReferencingProjectsNoCommit(pCompilerProject, CS_Declared, CS_Declared);
        }
    }

    GetCompilerPackage()->CommitEditListDemotions(true, false, this);
}

//============================================================================
// Demotes a metadata project to CS_NoState.
// Like the its _DemoteXXX cousins, this should not be called directly.
//============================================================================
void CompilerProject::_DemoteMetaDataProjectToNoState(bool ReleaseMetaDataImport)
{
    VSASSERT(IsMetaData(), "Must be metadata project!");
    // Decompiling mscorlib not supported!!!  Crash now to get good Watson data rather than 
    // have the IDE continue execution a few more milliseconds making it hard to diagnose.
    IfFalseThrow(this->GetCompilerHost()->GetComPlusProject() != this);
    IfFalseThrow(GetCompilerSharedState()->IsBackgroundCompilationBlocked());

    // Optimize for common case of single file assembly.
    if (this->GetCompState() == CS_NoState &&
        !ReleaseMetaDataImport &&
        (this->m_pfileList == NULL ||
         (this->m_pfileList &&
          !this->m_pfileList->GetNextFile() &&
          this->m_pfileList->GetCompState() == CS_NoState)))
    {
        return;
    }
    
    // Do generic project decompilation
    _DemoteToNoState();

    // 


    CompilationState cs;
    for (cs = (CompilationState)(CS_NoState + 1);
         cs <= CS_Compiled;
         cs = (CompilationState)(cs + 1))
    {
        CDoubleListForwardIter<CompilerFile> iter(&m_dlFiles[cs]);
        CompilerFile *pCompilerFile;

        while (pCompilerFile = iter.Next())
        {
            m_dlFiles[cs].Remove(pCompilerFile);
            m_dlFiles[CS_NoState].InsertLast(pCompilerFile);

            // Clear any errors if there is a tasklist provider (non-mscorlib.dll or msvb.dll)
            if (m_pTaskProvider)
            {
                pCompilerFile->m_ErrorTable.DeleteAllWithThisStateOrHigher(CS_NoState);
            }

            // This cleanup is fairly random.  Am I missing anything?
            pCompilerFile->m_cs = CS_NoState;
            pCompilerFile->RemoveNamespaces();
            pCompilerFile->m_SymbolList.Clear();
            pCompilerFile->m_DeclaredToBoundGenericBindingCache.ClearCache();
            pCompilerFile->SymbolStorage()->FreeHeap();
            pCompilerFile->PMetaDataFile()->_DemoteToNoState();
            pCompilerFile->ClearingBindingStatus();
        }
    }

    GetCompilerHost()->GetFXSymbolProvider()->RemoveFXTypesForProject(this);

    // Decompilation of the VB runtime means the symbols in the remapping table
    // are stale and need to be cleared.
    if (IsDefaultVBRuntime())
    {
        GetCompilerHost()->ClearRuntimeCallRemapTable();
    }

    if (ReleaseMetaDataImport && (this != GetCompilerHost()->GetComPlusProject()) && (!this->IsDefaultVBRuntime()))
    {
        ReleaseMetaDataProject();

        // Bug VSWhidbey 439036.
        // If a file change notification for a metadata file showed up before its transition to
        // bound, then the error table was not being cleared resulting in stale errors.
        m_ErrorTable.DeleteAll();

        VSASSERT(!m_fCodeModule, "IDE compiler can never reference a .Net Module");
        InitMetaDataFiles(GetFileName(), true); /* IDE compiler can never reference a module */

        GetCompilerHost()->DestroyProjectCompilationList();
    }

    // Clear fields that are specific to metadata projects
    m_fBasesAndImplementsLoaded = false;

    VSASSERT(m_cs == CS_NoState, "Unexpected state.");
}
#endif IDE

//=============================================================================
// Create default and declared project level conditional compilation symbols.
//=============================================================================

HRESULT CompilerProject::CreateProjectLevelCondCompSymbols
(
    ErrorTable *pErrorTable
)
{
    Symbols      symbols(m_pCompiler, &m_nraSymbols, NULL);
    BCSYM_Hash   *phash;
    StringBuffer TempCCConstants;

    //
    // Create the project level cond comp scope and the hash table
    //

    BCSYM_CCContainer * tempProjectConditionalCompilationScope = symbols.AllocCCContainer(false, NULL);

    // This creates a hash table of default size
    symbols.GetConditionalCompilationScope(tempProjectConditionalCompilationScope);
    phash = tempProjectConditionalCompilationScope->GetHash();

    //
    // Create user-declared symbols from project properties
    //
    STRING *CCConstants = m_pstrConditionalCompilationConstants;

    // First, put in the version string
    TempCCConstants.AppendString(WIDE("VBC_VER = 11.0,"));

    // Next, put in the target type
    TempCCConstants.AppendString(WIDE("TARGET = "));

    switch(GetOutputType())
    {
        case OUTPUT_ConsoleEXE:
            TempCCConstants.AppendString(WIDE("\"exe\""));
            break;

        case OUTPUT_WindowsEXE:
            TempCCConstants.AppendString(WIDE("\"winexe\""));
            break;

        // OUTPUT_None behaves just like OUTPUT_Library except for the fact that it doesn't
        // generate a PE image.
        case OUTPUT_None:
        case OUTPUT_Library:
            TempCCConstants.AppendString(WIDE("\"library\""));
            break;

        case OUTPUT_Module:
            TempCCConstants.AppendString(WIDE("\"module\""));
            break;

        case OUTPUT_AppContainerEXE:
            TempCCConstants.AppendString(WIDE("\"appcontainerexe\""));
            break;

        case OUTPUT_WinMDObj:
            TempCCConstants.AppendString(WIDE("\"winmdobj\""));
            break;

        default:
            VSFAIL("Unexpected output type!");
            TempCCConstants.AppendString(WIDE("\"\""));
            break;
    }

    if (CCConstants && *CCConstants)
    {
        TempCCConstants.AppendString(WIDE(","));
        TempCCConstants.AppendSTRING(CCConstants);
    }

    CCConstants = m_pCompiler->AddString(&TempCCConstants);

    ErrorTable CCErrors(m_pCompiler, this, NULL);

    VB_ENTRY();
    Scanner tokenStream(m_pCompiler, CCConstants, wcslen(CCConstants), 0, 0, 0);
    Parser      parser(&m_nraSymbols, m_pCompiler, GetCompilerHost(), false, GetCompilingLanguageVersion());

    hr = parser.ParseProjectLevelCondCompDecls(&tokenStream,
                                               &symbols,
                                               phash,
                                               &CCErrors);

    SetProjectLevelCondCompSymbols(tempProjectConditionalCompilationScope);

    VB_EXIT_GO();

    if (pErrorTable)
    {
        pErrorTable->MergeProjectLevelCCErrors(&CCErrors, CCConstants);
    }

Error:
    RRETURN( hr );
}

#if DEBUG

//============================================================================
// Verify that each reference exists only once in the list of references.
//============================================================================
void CompilerProject::VerifyNoDuplicateReferences()
{
    // The idea here is that we want to make sure that there are no duplicates in the
    // reverse reference array for any project that we are referencing. When FinishEdit
    // is complete, we should have a well structured references list, without any duplicates.
    ReferenceIterator ReferencedProjects(this);
    CompilerProject *pReferencedCompilerProject;

    while (pReferencedCompilerProject = ReferencedProjects.Next())
    {
        ReverseReferenceIterator RefIterator(pReferencedCompilerProject);
        CompilerProject *pReverseReferencedProject;
        unsigned NumberOfReferences = 0;

        while (pReverseReferencedProject = RefIterator.Next())
        {
            if (pReverseReferencedProject == this)
            {
                ++NumberOfReferences;
                VSASSERT(NumberOfReferences < 2, L"Reverse reference array has duplicate items!");
            }
        }
    }
}

//============================================================================
// Makes sure that the m_daReferences and m_daReferencesOnMy agree.
//============================================================================
void CompilerProject::ValidateAllReferences()
{
    ProjectIterator ProjectIter(GetCompilerHost());
    CompilerProject* pProject;

    while (pProject = ProjectIter.Next())
    {
        ReferenceIterator RefIterator(pProject);
        CompilerProject *pReferencedProject;

        // First verify the references, then verify the reverse references.
        while (pReferencedProject = RefIterator.Next())
        {
            ReverseReferenceIterator RevRefIterator(pReferencedProject);
            CompilerProject *pOriginalProject;

            while (pOriginalProject = RevRefIterator.Next())
            {
                if (pOriginalProject == pProject)
                {
                    break;
                }
            }

            VSASSERT(pOriginalProject != NULL, "References are bad!");
        }

        ReverseReferenceIterator RevRefIterator(pProject);
        CompilerProject *pReverseReferencedProject;

        // then verify the reverse references.
        while (pReverseReferencedProject = RevRefIterator.Next())
        {
            ReferenceIterator RefIterator2(pReverseReferencedProject);
            CompilerProject *pOriginalProject;

            while (pOriginalProject = RefIterator2.Next())
            {
                if (pOriginalProject == pProject)
                {
                    break;
                }
            }

            VSASSERT(pOriginalProject != NULL, "References are bad!");
        }
    }
}
#endif DEBUG

//****************************************************************************
// IVbVbaCompiler implementation
//****************************************************************************
STDMETHODIMP CompilerProject::GetXMLFromSourceFile(LPCWSTR pwszFileName,
                                                   VARIANT_BOOL fDeclsOnly,
                                                   BSTR* pbstrXML)
{
    HRESULT hr = S_OK;

    STRING *pstrFileName;
    ICodeFile* pCodeFile = NULL;
    FileIterator fileiter(this);
    CompilerFile *pfile;
    BOOL fFound = FALSE;

    if (!pwszFileName || !pbstrXML)
    {
        return E_POINTER;
    }

    *pbstrXML = NULL;

    pstrFileName = m_pCompiler->AddString(pwszFileName);

    while (pfile = fileiter.Next())
    {
        if (pfile->IsSourceFile())
        {
            SourceFile *pSourceFile = pfile->PSourceFile();

            if (StringPool::IsEqual(pSourceFile->GetFileName(), pstrFileName))
            {
                fFound = TRUE;
                pCodeFile = pSourceFile->GetCodeFile();
                if (pCodeFile)
                {
                    if (VARIANT_TRUE == fDeclsOnly)
                    {
                        hr = pCodeFile->GetDeclarationStatements(pbstrXML);
                    }
                    else
                    {
                        hr = pCodeFile->GetAllStatements(pbstrXML);
                    }
                }
                else
                {
                    VSASSERT(pCodeFile, "Should always get a code file back right?");
                }
            }
        }
    }

    if (!fFound)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


//===============================================================================
// Provides a mechanism for Vba to obtain error information from a
// sycnronous compile.
// The returned array of errors is IMalloc memory that the caller
// is responsible for freeing.  WARNING: the count of errors returned
// may be less than the number of elements allocated in the error array.
// That is, there may be extra "junk" entries in the array, so use the
// count returned here rather than trying to deduce it from the IMalloc
// memory.
// (Microsoft)
//===============================================================================
STDMETHODIMP CompilerProject::GetVbaErrors(UINT* pcErrors, // on return, a count of errors.
                                           VbError** ppErrors) // on return an array of VbErrors.
{
    VB_ENTRY();
    if ( !ppErrors || !pcErrors )
    {
        hr = E_POINTER;
        goto Error;
    }

    *ppErrors = NULL;
    *pcErrors = 0;

    // Bail if we have no errors.
    if ( CompiledSuccessfully() )
    {
        hr = S_FALSE;
        goto Error;
    }

    // Get our errors.
    UINT cErrors = 0;
    UINT cWarnings = 0;

    GetCompiler()->BuildErrorList(ppErrors, &cErrors, &cWarnings);

    // Ensure that the count we send back matches the number of VbErrors in the ppErrors array
    *pcErrors = (cErrors + cWarnings);

    VB_EXIT_NORETURN();

    VSASSERT( *ppErrors, "Should have some errors" );

    // Fallthrough ok.
Error:
    if ( FAILED( hr ) )
    {
        if (pcErrors) *pcErrors = 0;
        if (ppErrors) *ppErrors = NULL;
    }

    return hr;
}

//===============================================================================
// Provides a way for Vba to determine whether the last compile was
// successful.  VARIANT_TRUE on success, VARIANT_FALSE if the compile failed.
// (Microsoft)
//===============================================================================
STDMETHODIMP CompilerProject::CompileSucceeded( VARIANT_BOOL* pfCompileSucceeded )
{
    HRESULT hr = S_OK;

    if ( !pfCompileSucceeded )
    {
        hr = E_POINTER;
        goto Error;
    }

    if ( CompiledSuccessfully() )
    {
        *pfCompileSucceeded = VARIANT_TRUE;
    }
    else
    {
        *pfCompileSucceeded = VARIANT_FALSE;
    }

    // Fallthrough ok.
Error:
    return hr;
}

#if IDE 
bool CompilerProject::IsDebuggingInProgress()
{
    return m_pCompilerHost->IsDebuggingInProgress();
}
#endif IDE

//---------------------------------------------------------------------------
// IVsENCRebuildableProjectCfg2
//---------------------------------------------------------------------------
STDMETHODIMP CompilerProject::StartDebuggingPE(void)
{
    VB_ENTRY();
#if IDE
    DWORD dwItems[2];
    dwItems[0] = STRM_VB_DebuggerCompilerStateOrigin::NewStart;
    dwItems[1] = GetCompState();

    IDEHelpers::SQMStreamAddMultipleItems(DATAID_STRM_VB_DEBUGGER_COMPILERSTATE, dwItems, DIM(dwItems));

    if (m_fGenerateENCableCode)
    {
        CompilerPackage *pPackage = GetCompilerPackage();

        //Process any pending file change notifications that may not have been processed in Idle time.        
        pPackage->ProcessFileChanges(false);

        if (SUCCEEDED(WaitUntilTypesEmittedState(WaitStatePumping::PumpingAllowed, WaitStateCancel::CancelDisallowed)))
        {
            if ((m_fCompiledSuccessfully || !HasErrors()))
            {
                BackgroundCompilerLock compilerLock(this);

                if (0 == m_pCompilerHost->GetDebuggingProjectCount())
                {
                    m_pENCBuilder = new ENCBuilder(m_pCompiler,this);
                    IfFalseGo(m_pENCBuilder,E_OUTOFMEMORY);

                    m_pCompilerHost->SetDebuggingInProgress(true);
                }
                else
                {
                    m_pENCBuilder = new ENCBuilder(m_pCompiler,this);
                    IfFalseGo(m_pENCBuilder,E_OUTOFMEMORY);
                }

                m_pCompilerHost->IncrementDebuggingProjectCount();

                m_pENCBuilder->CopyMetaToken(&m_Builder);
                m_fNeedtoUpdateENCMetaData = false;
            }
        }
    }
Error:;

#else
    hr = E_NOTIMPL;
#endif //!IDE
    VB_EXIT();
}

//-------------------------------------------------------------------------------------------------
//
// Called when the user enters break mode and some part of the debugging state points to this 
// project as being relevant.  This will be called for every break state on the PE
//
// Note: This is break state not break point.  It will happen essentially any time the debugger is 
// stopped and part of the debugging stack points into this project
//
//-------------------------------------------------------------------------------------------------
STDMETHODIMP 
CompilerProject::EnterBreakStateOnPE
(
    ENC_BREAKSTATE_REASON encBreakReason,
    ENC_ACTIVE_STATEMENT *pActiveStatements,
    UINT32 cActiveStatements
)
{
    VB_ENTRY();
#if IDE 
    IfFalseGo(m_pENCBuilder,S_OK);
    IfFailRet(WaitUntilBindableState(WaitStatePumping::PumpingAllowed, WaitStateCancel::CancelAllowed)); // VSWhidbey[537519] Make sure there is something to search first
    m_fIsInDebugBreak = true;
    m_pENCBuilder->EnterENCSession(encBreakReason);
    if (cActiveStatements && pActiveStatements)
    {
        IfFailGo(m_pENCBuilder->InitActiveStatements(pActiveStatements, cActiveStatements));
    }
Error:;

#else
    hr = E_NOTIMPL;
#endif //!IDE
    VB_EXIT();
}

#if IDE

//---------------------------------------------------------------------------
// IVsLanguageServiceBuildErrorReporter
//---------------------------------------------------------------------------

STDMETHODIMP CompilerProject::ReportError
(
    BSTR bstrErrorMessage,
    BSTR bstrErrorId,
    VSTASKPRIORITY nPriority,
    long iLine,
    long iColumn,
    BSTR bstrFileName
)
{
    return ReportError2(bstrErrorMessage, bstrErrorId, nPriority, iLine, iColumn, -1, -1, bstrFileName);
}

//---------------------------------------------------------------------------
// IVsLanguageServiceBuildErrorReporter2
//---------------------------------------------------------------------------

STDMETHODIMP CompilerProject::ReportError2
(
    BSTR bstrErrorMessage,
    BSTR bstrErrorId,
    VSTASKPRIORITY nPriority,
    long iStartLine,
    long iStartColumn,
    long iEndLine,
    long iEndColumn,
    BSTR bstrFileName
)
{
    VB_ENTRY();

    // If we don't have a location, don't handle the error
    IfFalseGo(iStartLine >= 0 && iStartColumn >= 0, E_NOTIMPL);

    // Verify that the error id string starts with 'BC'
    // for VB errors or 'WME' for WinMDExp errors.
    long errorId = 0;
    bool fIsWME = false;
    UINT len = ::SysStringLen(bstrErrorId);
    if (len >= 2 &&
        bstrErrorId[0] == L'B' &&
        bstrErrorId[1] == L'C')
    {
        errorId = _wtol(bstrErrorId + 2);
    }
    else if (len >= 3 &&
             bstrErrorId[0] == L'W' &&
             bstrErrorId[1] == L'M' &&
             bstrErrorId[2] == L'E')
    {
        errorId = _wtol(bstrErrorId + 3);
        fIsWME = true;
    }

    // Don't handle this if we can't determine the error id
    IfFalseGo(errorId != 0, E_NOTIMPL);

    SourceFile* pSourceFile = FindSourceFileByName(GetCompiler()->AddString(bstrFileName));
    IfFalseGo(pSourceFile != NULL, E_NOTIMPL);

    if (fIsWME)
    {
        pSourceFile->AddWinMDExpBuildError(bstrErrorMessage, errorId, nPriority, iStartLine, iStartColumn, iEndLine, iEndColumn);
    }
    else
    {
        pSourceFile->AddOutOfProcVbcBuildError(bstrErrorMessage, errorId, nPriority, iStartLine, iStartColumn, iEndLine, iEndColumn);
    }

Error:;
    VB_EXIT();
}

STDMETHODIMP CompilerProject::ClearErrors()
{
    VB_ENTRY();

    bool fAnyErrorsRemoved = false;
    SourceFileIterator iterator(this);

    while (SourceFile* pSourceFile = iterator.Next())
    {
        if (pSourceFile->ClearOutOfProcBuildErrors())
        {
            fAnyErrorsRemoved = true;
        }
    }

    if (m_pTaskProvider && fAnyErrorsRemoved)
    {
        // Tell the task provider that the list of tasks has changed
        m_pTaskProvider->TaskListNeedsRefresh();

        // Remove stale task list items now (since the underlying CompileError was deleted)
        GetCompiler()->DoRefreshTaskList();
    }

    VB_EXIT();
}

#endif IDE

STDMETHODIMP CompilerProject::GetPEBuildTimeStamp
(
    FILETIME* pTimeStamp
)
{
    return E_NOTIMPL;
}

STDMETHODIMP CompilerProject::EncApplySucceeded(HRESULT hrApplyResult)
{
    VB_ENTRY();
#if IDE

    IfFailGo(m_pENCBuilder->PostDeltaGenFixup(&m_daDeltaGenFixupItems));
    m_daDeltaGenFixupItems.Destroy();
Error:;

#else
    hr = E_NOTIMPL;
#endif //!IDE

    VB_EXIT();
}

STDMETHODIMP CompilerProject::BuildForEnc
(
    IUnknown *pUpdatePE
)
{
    VB_ENTRY();
#if IDE

    if (pUpdatePE)
    {
        CComPtr<IDebugUpdateInMemoryPE> pUpdateInMem;
        m_daDeltaGenFixupItems.Destroy();

        IfFalseGo(pUpdatePE,E_INVALIDARG);
        IfFailGo(pUpdatePE->QueryInterface(IID_IDebugUpdateInMemoryPE, (void**)&pUpdateInMem));
        IfFalseGo(m_pENCBuilder,S_OK);

        IfFailGo(m_pENCBuilder->DeltaGen(pUpdateInMem, &m_daDeltaGenFixupItems));
    }
    else
    {
       hr = E_INVALIDARG;
    }
Error:;

#else
    hr = E_NOTIMPL;
#endif //!IDE

    VB_EXIT();
}

//-------------------------------------------------------------------------------------------------
//
// Called when we are leaving break mode and contining stepping in the IDE
//
//-------------------------------------------------------------------------------------------------
STDMETHODIMP CompilerProject::ExitBreakStateOnPE(void)
{
#if IDE
    VB_ENTRY();
    IfFalseGo(m_pENCBuilder,S_OK);

    // ENC suite tasklist dump helper
    DumpTasklistForENCSuites();

    m_pENCBuilder->LeaveENCSession();
    m_fIsInDebugBreak = false;
Error:;

    VB_EXIT();
#else
    return E_NOTIMPL;
#endif //!IDE
}

STDMETHODIMP CompilerProject::StopDebuggingPE( void)
{
#if IDE 
    VB_ENTRY();
    if (m_pENCBuilder)
    {
        // 


        BackgroundCompilerLock compilerLock(this);
        m_pENCBuilder->LeaveENCSession();

        if (1 == m_pCompilerHost->GetDebuggingProjectCount())
        {
            m_pCompilerHost->SetDebuggingInProgress(false);

            ProjectIterator Iter(m_pCompilerHost);
            CompilerProject *pProject;

            while (pProject = Iter.Next())
            {
                if (pProject->IsMetaData())
                {
                    continue;
                }

                if (pProject->m_pENCBuilder)
                {
                    bool WasEditPerformed = pProject->m_pENCBuilder->WasEditPerformed();

                    pProject->m_pENCBuilder->ClearMetaToken();
                    delete pProject->m_pENCBuilder;
                    pProject->m_pENCBuilder = NULL;

                    // 







                    if (WasEditPerformed)
                    {
                        ::DeleteFile(GetPEName()); // VSWhidbey[434778, 401743] When Builder needs to compile to disk, the disk file could become out of sync with background compiler.
                    }
                }

                if (pProject->GetCompState() >= CS_TypesEmitted && pProject->m_fILEmitTasksSkippedDuringDebugging)
                {
                    // m_ILEmitTasksSkipped is set during the transition from CS_TypesEmitted to
                    // CS_Compiled. So if m_ILEmitTasksSkipped is set, we should decompile to
                    // CS_Bound, because decompilation to CS_TypesEmitted after starting CS_Compiled
                    // is not allowed.
                    EditFilter::DecompileProject(pProject, CS_Bound);
                }
            }
        }
        m_pCompilerHost->DecrementDebuggingProjectCount();
    }

    VB_EXIT();
#else
    return E_NOTIMPL;
#endif //IDE
}

STDMETHODIMP CompilerProject::GetENCBuildState
(
    ENC_BUILD_STATE *pENCBuildState
)
{
#if IDE
    VB_ENTRY();
    DWORD dwItems[2];
    dwItems[0] = STRM_VB_DebuggerCompilerStateOrigin::EnC;
    dwItems[1] = GetCompState();

    IDEHelpers::SQMStreamAddMultipleItems(DATAID_STRM_VB_DEBUGGER_COMPILERSTATE, dwItems, DIM(dwItems));

    IfFalseGo(pENCBuildState,E_INVALIDARG);
    *pENCBuildState = ENC_NOT_MODIFIED;
    IfFalseGo(m_pENCBuilder,S_OK);
    IfFailGo(m_pENCBuilder->GetENCBuildState(pENCBuildState));
Error:;

    VB_EXIT();
#else
    return E_NOTIMPL;
#endif //IDE
}

STDMETHODIMP CompilerProject::GetCurrentActiveStatementPosition
(
    UINT32 id,
    TextSpan *ptsNewPosition
)
{
#if IDE 
    VB_ENTRY();
    IfFalseGo(ptsNewPosition,E_INVALIDARG);
    IfFalseGo(m_pENCBuilder,S_FALSE);
    IfFailGo(m_pENCBuilder->GetCurrentActiveStatementPosition(id,ptsNewPosition));
Error:;

    VB_EXIT();
#else
    return E_NOTIMPL;
#endif //IDE
}

STDMETHODIMP CompilerProject::GetPEidentity
(
    GUID *pMVID,
    BSTR *pbstrPEName
)
{
#if IDE 
    VB_ENTRY();
    // This is a hack to get around the problem of having to OpenScope() the assembly even if the caller is not
    // interested in the assembly name or the MVID. In this case, all they want to know if we support ENC or not.
    // See bug VSWhidbey 278265 for details.
    if (!pMVID && !pbstrPEName)
    {
        if (!m_pstrPEName)
        {
            hr = E_FAIL;
        }
    }
    else
    {
        if (m_pstrPEName)
        {
            CComPtr<IMetaDataDispenserEx> pimdDataDispEx;
            CComPtr<IMetaDataImport2> pimdImport;

            hr = GetCompilerHost()->OpenScope(&pimdDataDispEx, m_pstrPEName, ofReadOnly, IID_IMetaDataImport2, (IUnknown **)&pimdImport);

            if (S_OK == hr)
            {
                if (pMVID)
                {
                    hr = pimdImport->GetScopeProps(NULL,0,NULL,pMVID);
                }
            }

            if (pbstrPEName)
            {
                *pbstrPEName = SysAllocString(m_pstrPEName);
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }

    VB_EXIT();
#else
    return E_NOTIMPL;
#endif //!IDE
}

STDMETHODIMP CompilerProject::GetExceptionSpanCount
(
    UINT32 *pcExceptionSpan
)
{
#if IDE 
    VB_ENTRY();
    IfFalseGo(pcExceptionSpan,E_INVALIDARG);
    *pcExceptionSpan = 0;
    IfFalseGo(m_pENCBuilder,S_FALSE);
    IfFailGo(m_pENCBuilder->GetExceptionSpanCount(pcExceptionSpan));
Error:;

    VB_EXIT();
#else
    return E_NOTIMPL;
#endif //IDE
}

STDMETHODIMP CompilerProject::GetExceptionSpans
(
    UINT32 celt,
    ENC_EXCEPTION_SPAN *rgelt,
    ULONG *pceltFetched
)
{
#if IDE 
    VB_ENTRY();
    IfFalseGo(rgelt && celt,E_INVALIDARG);
    IfFalseGo(m_pENCBuilder,S_FALSE);
    IfFailGo(m_pENCBuilder->GetExceptionSpans(celt,rgelt,pceltFetched));
Error:;

    VB_EXIT();
#else
    return E_NOTIMPL;
#endif //IDE
}

STDMETHODIMP CompilerProject::GetCurrentExceptionSpanPosition
(
    UINT32 id,
    TextSpan *ptsNewPosition
)
{
#if IDE 
    VB_ENTRY();
    IfFalseGo(ptsNewPosition,E_INVALIDARG);
    IfFalseGo(m_pENCBuilder,S_FALSE);
    IfFailGo(m_pENCBuilder->GetCurrentExceptionSpanPosition(id,ptsNewPosition));
Error:;

    VB_EXIT();
#else
    return E_NOTIMPL;
#endif //IDE
}

#if IDE 
// StartGetUnusedReferences
//
// Clears reference usage information, decompiles current project and
// referencing projects to Declared and sets flag to track reference
// usage through compilation.
//
void CompilerProject::StartGetUnusedReferences()
{
    VSASSERT(!m_fGetUnusedReferences, "Already getting unused references!");

    BackgroundCompilerLock compilerLock(this);

    // Clear reference usages
    ReferencedProject *rgRefs = m_daReferences.Array();
    unsigned cEntries = m_daReferences.Count();

    for (unsigned iRef = 0; iRef < cEntries; iRef++)
    {
        rgRefs[iRef].m_fUsed = false;
    }

    VSDEBUGPRINTIF(VSFSWITCH(fDumpReferenceUsage), "Reference usage cleared for %S\n", GetAssemblyName());

    // Start tracking reference usage
    m_fGetUnusedReferences = true;

    EditFilter::DecompileProjectAndReferencingProjects(this, CS_Declared);
}

void CompilerProject::StopGetUnusedReferences()
{
    m_fGetUnusedReferences = false;
}

bool CompilerProject::IsGettingUnusedReferences()
{
    return m_fGetUnusedReferences;
}

// GetUnusedReferences
// (Helper for CompilerPackage::GetUnusedReferences)
//
// Get a NULL delimited string of the paths of the unused references in the project.
// Note, this function blocks until compiler project has reached compiled state.
//
// [out] pReferencePaths: Pointer to returned string of unused references
// [out] cchLength: Length of returned string
//
HRESULT CompilerProject::GetUnusedReferences
(
    _Out_ _Deref_post_cap_(*cchLength) WCHAR   **pReferencePaths,
    ULONG   *cchLength
)
{
    VSASSERT(pReferencePaths, "Invalid stringbuffer arg.");
    VSASSERT(cchLength, "Invalid length arg.");

    ULONG   cchReturnLength;
    HRESULT hr = E_FAIL;

    // See if we already cached the unused references
    cchReturnLength = m_strUnusedReferences.GetStringLength();
    if (!cchReturnLength)
    {
        if (m_fGetUnusedReferences && GetCompState() == CS_Compiled) // Done calculating unused references
        {
            // Get references of current project
            ReferencedProject    *rgref = m_daReferences.Array();
            unsigned                    cEntries = m_daReferences.Count();

            // Add a delimiter to the buffer --
            // Always put an entry in the string, even if there are no unused references
            // so we know we've already calculated this
            m_strUnusedReferences.AppendChar(UCH_NULL);

            // Now iterate to find unused references
            for (unsigned iRef = 0; iRef < cEntries ; iRef++)
            {
                if (!rgref[iRef].m_fUsed)
                {
                    // Get reference's CopmilerProject
                    CompilerProject *pRef = rgref[iRef].m_pCompilerProject;

                    // Skip required and standard libraries
                    if (!rgref[iRef].m_fRequired && !rgref[iRef].m_fStandard)
                    {
                        // Add reference path to buffer.  Path stored in different places for project & metadata references
                        if (pRef->IsMetaData())
                        {
                            m_strUnusedReferences.AppendSTRING(pRef->GetFileName());
                        }
                        else
                        {
                            m_strUnusedReferences.AppendSTRING(pRef->GetPEName());
                        }

                        // Add delimiter
                        m_strUnusedReferences.AppendChar(UCH_NULL);
#if DEBUG
                        // Dump debug info
                        if (VSFSWITCH(fDumpReferenceUsage))
                        {
                            // Unused reference found
                            DebPrintf("Unused: reference %S -> %S\n",
                                GetAssemblyName(),
                                rgref[iRef].m_pCompilerProject->m_pstrFileName,
                                m_cs);
                        }
#endif DEBUG
                    }
                }
            }

            // return string length
            cchReturnLength = m_strUnusedReferences.GetStringLength();
        }
    }

    // Return unused references string & length
    // Empty string indicates project has either not started or not finished compiling with m_fTrackUsedReferences
    *pReferencePaths = m_strUnusedReferences.GetString();

    *cchLength = cchReturnLength;

    return S_OK;
}
#endif IDE

//========================================================================
// Does the current project reference this one?
//========================================================================

bool CompilerProject::IsProjectReferenced
(
    CompilerProject *pProject,
    bool fAllowAssemblyIdentityMatch
)
{
#if DEBUG
    bool OrigReturnedValue = false;

    ReferenceIterator iter(this);
    CompilerProject *pCurrent;

    while (pCurrent = iter.Next())
    {
        if (pCurrent == pProject ||
            (fAllowAssemblyIdentityMatch &&
                pProject->IsMetaData() &&
                pCurrent->IsMetaData() &&
                StringPool::IsEqual(
                    pProject->GetAssemblyIdentity()->GetAssemblyIdentityString(),
                    pCurrent->GetAssemblyIdentity()->GetAssemblyIdentityString())))
        {
            OrigReturnedValue = true;
            break;
        }
    }
#endif DEBUG

    // Should be enabled once we get rid of "RemoveAllMetaDataReferences" Hack.
    bool ReturnedValue = false;

    // Note: The only reason we have two cases is for performance. We always want
    // to iterate over the shorter list of references.

    if (this->m_daReferences.Count() < pProject->m_daReferencesOnMe.Count())
    {
        // This project's references are less than pProject's reverse references.
        // We iterate over the references of this project because it is the shorter list.
        ReferenceIterator RefIterator(this);
        CompilerProject *pReferencedProject;

        while (pReferencedProject = RefIterator.Next())
        {
            if (pReferencedProject == pProject ||
                (fAllowAssemblyIdentityMatch &&
                    pProject->IsMetaData() &&
                    pReferencedProject->IsMetaData() &&
                    StringPool::IsEqual(
                        pProject->GetAssemblyIdentity()->GetAssemblyIdentityString(),
                        pReferencedProject->GetAssemblyIdentity()->GetAssemblyIdentityString())))
            {
                ReturnedValue = true;
                break;
            }
        }
    }
    else
    {
        // pProject's reverse references are shorted (or equal) to this project's references,
        // use the shorter list (pProject's reverse references).
        ReverseReferenceIterator RevRefIterator(pProject);
        CompilerProject *pReferencingProject;

        while (pReferencingProject = RevRefIterator.Next())
        {
            if (pReferencingProject == this)
            {
                ReturnedValue = true;
                break;
            }
        }

        // Duplicated code to allow for above optimization of looking in the smaller list for the
        // common cases.

        if (!ReturnedValue && fAllowAssemblyIdentityMatch)
        {
            ReferenceIterator RefIterator(this);
            CompilerProject *pReferencedProject;

            while (pReferencedProject = RefIterator.Next())
            {
                if (pProject->IsMetaData() &&
                    pReferencedProject->IsMetaData() &&
                    StringPool::IsEqual(
                        pProject->GetAssemblyIdentity()->GetAssemblyIdentityString(),
                        pReferencedProject->GetAssemblyIdentity()->GetAssemblyIdentityString()))
                {
                    ReturnedValue = true;
                    break;
                }
            }
        }
    }

    VSASSERT(OrigReturnedValue == ReturnedValue, "They should be the same!");
    return ReturnedValue;
}

#if IDE 
bool CompilerProject::IsReferencedByReferencingVBProjects
(
    CompilerProject *pProject,
    CompilerProject **ppFirstReferencingProject,
    CompilerProject **ppFirstReferencingVenusProject
)
{
    //

    bool fResult = false;
    ReverseReferenceIterator RevRefIterator(this);
    CompilerProject *pReferencingProject;

    while (pReferencingProject = RevRefIterator.Next())
    {
        if (pReferencingProject->IsProjectReferenced(pProject))
        {
            fResult = true;

            if (ppFirstReferencingProject && !*ppFirstReferencingProject)
            {
                *ppFirstReferencingProject = pReferencingProject;
            }

            if (ppFirstReferencingVenusProject == NULL)
            {
                break;
            }

            if (pReferencingProject->IsVenusProject())
            {
                *ppFirstReferencingVenusProject = pReferencingProject;
                break;
            }
        }
    }

    return fResult;
}

CompilerProject *CompilerProject::GetFirstReferencingVenusProject()
{
    ReverseReferenceIterator RevRefIterator(this);
    CompilerProject *pReferencingProject;

    while (pReferencingProject = RevRefIterator.Next())
    {
        if (pReferencingProject->IsVenusProject())
        {
            return pReferencingProject;
        }
    }

    return NULL;
}

#endif IDE

#if ID_TEST

//========================================================================
// Dumps the string to a file.
//========================================================================
void CompilerProject::DumpXMLToFile(Compiler *pCompiler, _In_opt_z_ WCHAR *pszFileName,
                                    _In_opt_z_ const WCHAR *pszFileExtention, _In_opt_z_ WCHAR *pwszBuffer)
{
    if (!pszFileName || !pwszBuffer || !pszFileExtention)
    {
        return;             // Nothing to do
    }

    long originalFileNameLength = (long)wcslen(pszFileName);
    long fileExtentionLength = (long)wcslen(pszFileExtention);

    // First, we need to create a new file name.
    size_t nRequiredBuffer = originalFileNameLength + fileExtentionLength + 2;
    WCHAR *pXMLFileName = new (zeromemory) WCHAR[nRequiredBuffer];

    if (!pXMLFileName)
    {
        VSASSERT(false, "How come we can't create a new file name");
        return;
    }

    // find the last period
    long i;
    for (i = originalFileNameLength ; i >= 0 && pszFileName[i] != L'.' ; --i);

    if (i < 0)
    {
        // No period was found
        // Copy the whole thing
        i = originalFileNameLength;
        wcsncpy_s(pXMLFileName, nRequiredBuffer, pszFileName, i);
        pXMLFileName[i++] = L'.';
    }
    else
    {
        // Copy the part before the period
        wcsncpy_s(pXMLFileName, nRequiredBuffer, pszFileName, (size_t) ++i);
    }

    // Make up a new file name using the new extension
    wcsncpy_s(&pXMLFileName[i], nRequiredBuffer - i, pszFileExtention, fileExtentionLength);

    pXMLFileName[i + fileExtentionLength] = L'\0';

    // DONOT remove this code. It is used for dumping Unicode XML.
#if DEBUG
    if (false)
    {
        // Dump Unicode file here
        int FileID = _wopen(pXMLFileName, _O_WRONLY | _O_CREAT, _S_IWRITE);
        // pFile = _wfopen(pXMLFileName, L"w");

        if(FileID == -1)
        {
#pragma warning (push)
#pragma warning (disable:6255) // This is Debug code, so this warning is ignorable
            DebPrintf("Cannot create \"%s\", Cause = %d\n", ANSISTR(pXMLFileName), errno);
#pragma warning (pop)

            delete []pXMLFileName;
            return;
        }

        int NumberOfCharacterWritten = _write(FileID, pwszBuffer,(UINT) (wcslen(pwszBuffer) * sizeof(WCHAR)));
        _close(FileID);
    }
    else
#endif
    {
        // Dump ANSI file here
        FILE *pFile = NULL;

#pragma warning (push)
#pragma warning (disable:6255) // This is test/Debug code, so this warning is ignorable
        if (fopen_s(&pFile, ANSISTR(pXMLFileName), "w") != 0 || !pFile)
        {
            DebPrintf("Cannot create \"%s\", returning.\n", ANSISTR(pXMLFileName));
            delete []pXMLFileName;
            return;
        }
#pragma warning (pop)

        ULONG LengthOfBuffer = (ULONG) wcslen(pwszBuffer);

        // We don't want to use ANISTR() for large strings because it messes up the stack (observed in CodeGen suite)
        char *pdest = (char *)new (zeromemory) char[LengthOfBuffer + 1];

        // And we want to save non-unicode strings to the file...
        ULONG j;
        for (j = 0; j < LengthOfBuffer; ++j)
        {
            pdest[j] = (char)pwszBuffer[j];
        }

        pdest[j] = '\0';

        fprintf(pFile, "%s", pdest);
        delete []pdest;
        fclose(pFile);
    }

    delete []pXMLFileName;
}

//========================================================================
// Dumps the XML for all of the files in this project.
//========================================================================

void CompilerProject::DumpXML()
{
    HRESULT hr;

    FileIterator files(this);
    CompilerFile *pfile;

    while (pfile = files.Next())
    {
        if (pfile->IsSourceFile() && !pfile->IsSolutionExtension())
        {
            BSTR bstrText = NULL;

            hr = pfile->PSourceFile()->GetCodeFile()->GetDeclarationStatements(&bstrText);

            if (FAILED(hr) || !bstrText)
            {
                DebPrintf("//============================================================================\n");
                DebPrintf("%S\n\nFAILED!\n", pfile->GetFileName());
                DebPrintf("\n//============================================================================\n\n");
            }
            else
            {
                DebPrintf("//============================================================================\n");
                DebPrintf("%S\n\n", pfile->GetFileName());

                // We don't want to use ANISTR() for large strings because it messes up the stack (observed in CodeGen suite)
                if (bstrText)
                {
                    char *pdest = (char *)new (zeromemory) char[SysStringLen(bstrText) + 1];

                    WszCpyToSz(pdest, bstrText);
                    DebDoPrint(pdest);
                    delete []pdest;
                }

                DebPrintf("\n//============================================================================\n\n");

                SysFreeString(bstrText);
                bstrText = NULL;

                // Dump the XML for each of the methods defined in this file.  Start by counting the number
                // of methods.
                //
                AllContainersInFileIterator containers(pfile, true);
                BCSYM_Container *pContainer;
                unsigned cMethods = 0;

                while (pContainer = containers.Next())
                {
                    BCITER_CHILD children;
                    children.Init(pContainer, false, true);

                    BCSYM_NamedRoot *pNamed;

                    while (pNamed = children.GetNext())
                    {
                        if (pNamed->IsMethodImpl())
                        {
                            cMethods++;
                        }
                    }
                }

                // Alloc an array big enough to hold all of the methods.
                BCSYM_MethodImpl **rgpMethods = VBAllocator::AllocateArray<BCSYM_MethodImpl*>(cMethods);

                // Fill it.
                containers.Init(pfile, true);
                cMethods = 0;

                while (pContainer = containers.Next())
                {
                    BCITER_CHILD children(pContainer, false, true);
                    BCSYM_NamedRoot *pNamed;

                    while (pNamed = children.GetNext())
                    {
                        if (pNamed->IsMethodImpl())
                        {
#pragma prefast(disable:26000, "No overflow here. We are using the same loop to calculate the isze of the array")
                            rgpMethods[cMethods++] = pNamed->PMethodImpl();
                        }
                    }
                }

                // Sort the list in file definition order.
                extern int _cdecl SortSymbolsByLocation(const void *arg1, const void *arg2);

                qsort(rgpMethods, cMethods, sizeof(BCSYM_MethodImpl *), SortSymbolsByLocation);

                // Dump the XML for each of these methods.
                unsigned iMethod;

                for (iMethod = 0; iMethod < cMethods; iMethod++)
                {
                    StringBuffer sbSignature, sbText;
                    sbText.SetAllocationSize(StringBuffer::AllocSize::Grande);

                    rgpMethods[iMethod]->GetBasicRep(m_pCompiler, NULL, &sbSignature);

                    hr = XMLGen::XMLRunBodies(m_pCompiler, pfile->PSourceFile(), &sbText, rgpMethods[iMethod]);

                    DebPrintf("//============================================================================\n");
                    DebPrintf("%S\n\n", sbSignature.GetString());

                    if (FAILED(hr))
                    {
                        DebPrintf("FAILED!\n", sbSignature.GetString());
                    }
                    else
                    {
                        // We don't want to use ANISTR() for large strings because it messes up the stack (observed in CodeGen suite)
                        if (sbText.GetString())
                        {
                            char *pdest = (char *)new (zeromemory) char[sbText.GetStringLength() + 1];

                            WszCpyToSz(pdest, sbText.GetString());
                            DebDoPrint(pdest);
                            delete []pdest;
                        }
                    }

                    DebPrintf("\n//============================================================================\n\n");
                }

                // Freeup the array of methods
                VBFree(rgpMethods);
            }

        }
    }
}

#endif ID_TEST

//****************************************************************************
// CreateCodeModel
// Purpose: Return an instance of the CodeModel.
//****************************************************************************
STDMETHODIMP CompilerProject::CreateCodeModel(void *pProject, void *pProjectItem, void **ppCodeModel)
{
#if IDE
    CheckInMainThread();
    VB_ENTRY();
    hr = E_FAIL;
    *ppCodeModel = NULL;

    hr = GetCodeModel((CodeModel **) ppCodeModel);

    VB_EXIT();

#else
    return E_NOTIMPL;
#endif  // IDE
}


//****************************************************************************
// CreateFileCodeModel
// Purpose: Creates a codemodel instance
// returns an instance of CodeModel class
//****************************************************************************
STDMETHODIMP CompilerProject::CreateFileCodeModel(void *pProjectExtensibility, void *pProjectItemExtensibility, void** ppfcm)
{
#if IDE 
    CheckInMainThread();
    VB_ENTRY1(E_INVALIDARG);
    *ppfcm = NULL;

    ProjectItem *pProjectItem = (ProjectItem *) pProjectItemExtensibility;

    if (pProjectItem)
    {
        CComBSTR bstrFileName;

        if (SUCCEEDED(pProjectItem->get_FileNames(1, &bstrFileName)) && bstrFileName)
        {
            SourceFileIterator FileIter(this);
            SourceFile *pSourceFile;

            while (pSourceFile = FileIter.Next())
            {
                if (pSourceFile->GetFileName() &&
                    CompareFilenames(
                        pSourceFile->GetFileName(),
                        bstrFileName) == 0)
                {
                    hr = pSourceFile->GetFileCodeModel((FileCodeModel **) ppfcm);
                    break;
                }
            }
        }
    }

    VB_EXIT();

#else
    return E_NOTIMPL;
#endif  // IDE
}

//****************************************************************************
// GetMethodFromLine
// for the given itemid and line, return name of method that is at (contains)
// that line.  Used for "little F5" processing in VSA.
//****************************************************************************
STDMETHODIMP CompilerProject::GetMethodFromLine
(
    VSITEMID itemid,    // Item we are searching for
    long iLine,         // Line number to search for function
    BSTR *pBstrProcName,// [out] Procedure name
    BSTR *pBstrClassName// [out] Class Name
)
{
#if IDE
    CheckInMainThread();
    HRESULT hr = NOERROR;

    CompilerFile *pCompilerFile;

    FileIterator fileiter(this);

    while (pCompilerFile = fileiter.Next())
    {
        if (pCompilerFile->IsSourceFile() && pCompilerFile->PSourceFile()->GetItemId() == itemid)
        {
            BCSYM_MethodImpl *pSymMethod;

            hr = pCompilerFile->PSourceFile()->GetMethodContainingLine(iLine, &pSymMethod);
            if (pSymMethod != NULL)
            {
                BCSYM_Container *pContainer;
                STRING *pstrClassName;

                pContainer = pSymMethod->GetContainer();
                pstrClassName = pContainer->GetQualifiedName(false);

                *pBstrProcName = ::SysAllocString(pSymMethod->GetName());
                *pBstrClassName = ::SysAllocString(pstrClassName);

                // will be freed by caller!!!
            }

            break;
        }
    }

//Error:
    return hr;

#else
    return E_NOTIMPL;
#endif  // IDE
}
#if IDE 
STDMETHODIMP CompilerProject::AdviseFileInsertion(
    IVbProjectEvents * pSinkP,
    IAdviseSite **ppAdviseSite,         // OUT: can be NULL
    VSCOOKIE *     pEventCookie )
{
    HRESULT hr = NOERROR;
    if( pSinkP == NULL )
        return( E_INVALIDARG );
    if( pEventCookie == NULL )
        return( E_INVALIDARG );

    m_daFileInsertionEventListeners.Add() = pSinkP;
    pSinkP->AddRef();

    *pEventCookie = (VSCOOKIE)pSinkP;

    if (ppAdviseSite)
    {
        CompilerProjectSite * pSite = new CompilerProjectSite( this, pSinkP );
        if (pSite == NULL)
            hr = E_OUTOFMEMORY;
        *ppAdviseSite = pSite;
    }


    return hr;
}
STDMETHODIMP CompilerProject::UnadviseFileInsertion(
    VSCOOKIE EventCookie)
{
    VB_ENTRY();
    IVbProjectEvents **rgArray = m_daFileInsertionEventListeners.Array();
    unsigned iEvent, cEvents = m_daFileInsertionEventListeners.Count();

    for (iEvent = 0; iEvent < cEvents; iEvent++)
    {
        if ((VSCOOKIE)rgArray[iEvent] == EventCookie)
        {
            rgArray[iEvent]->Release();

            m_daFileInsertionEventListeners.Remove(iEvent);
            break;
        }
    }

    VB_EXIT();
}


//////////////////////////////////////////////////////////////////////
//
//  Returns:
//      1. both docdata and docview
//      2. only docdata
//      3. neither docdata nor docview
//      There is no chance for docview without docdata
//
///////////////////////////////////////////////////////////////////////
STDMETHODIMP CompilerProject::FindDocDataAndDocViewByMoniker
(
    IVsHierarchy * pHier,
    LPCOLESTR pszMkDocument,
    IVsTextLines ** ppDoc,
    IVsCodeWindow ** ppView
)
{
    HRESULT hr = NOERROR;
    VSCOOKIE docCookie = VSCOOKIE_NIL;
    ULONG lFetch = 0;

    IVsRunningDocumentTable* pRDT = GetCompilerPackage()->GetRunningDocumentTable();
    IServiceProvider * pSP = GetCompilerPackage()->GetSite();

    IfFalseGo(pHier && pszMkDocument, E_INVALIDARG);
    IfFalseGo(ppDoc && ppView, E_INVALIDARG);
    *ppDoc = NULL;
    *ppView = NULL;

    if (pRDT != NULL)
    {
        CComPtr<IUnknown> spunkDocData;
        VSRDTFLAGS flags;

        if (SUCCEEDED(pRDT->FindAndLockDocument(RDT_NoLock, pszMkDocument, NULL, NULL, NULL, &docCookie)) &&
            SUCCEEDED(pRDT->GetDocumentInfo(docCookie, &flags, NULL, NULL, NULL, NULL, NULL, NULL)))
        {
            if (flags & RDT_PendingInitialization)
            {
                return E_PENDING;
            }

            CComPtr<IVsHierarchy> spTargetHierarchy(pHier);
            CComPtr<IVsHierarchy> spHierarchy;

            if (SUCCEEDED(pRDT->GetDocumentInfo(docCookie, NULL, NULL, NULL, NULL, &spHierarchy, NULL, &spunkDocData)))
            {
                if ((spHierarchy == NULL || spTargetHierarchy.IsEqualObject(spHierarchy)) && spunkDocData)
                {
                    spunkDocData->QueryInterface(IID_IVsTextLines,(void **)ppDoc);
                }
                else
                {
                    docCookie = VSCOOKIE_NIL;
                }
            }
        }
    }
    if (pSP != NULL && docCookie != VSCOOKIE_NIL)
    {
        CComPtr<IVsUIShell> pUIShell = NULL;
        CComPtr<IEnumWindowFrames> pWinFrameEnum = NULL;
        if (SUCCEEDED(pSP->QueryService(SID_SVsUIShell,&pUIShell)) &&
            SUCCEEDED(pUIShell->GetDocumentWindowEnum(&pWinFrameEnum)))
        {
            CComPtr<IVsWindowFrame> pWin = NULL;
            VARIANT var;
            while (S_OK == pWinFrameEnum->Next(1,&pWin,&lFetch))
            {
                VariantInit(&var);

                // Note: More than one window frame can have the same doc cookie (i.e. designer and code editor)
                if (SUCCEEDED(pWin->GetProperty(VSFPROPID_DocCookie, &var)) &&
                    V_VT(&var) == VT_INT_PTR &&
                    V_INT_PTR(&var) == docCookie)
                {
                    VariantClear(&var);
                    VariantInit(&var);
                    if (SUCCEEDED(pWin->GetProperty(VSFPROPID_DocView, &var)))
                    {
                        if (V_VT(&var) == VT_UNKNOWN && V_UNKNOWN(&var))
                        {
                            if (SUCCEEDED(V_UNKNOWN(&var)->QueryInterface(IID_IVsCodeWindow, (void **)ppView)))
                            {
                                VariantClear(&var);
                                break;
                            }
                        }
                    }
                }
                VariantClear(&var);
                pWin.Release();
                pWin = NULL;
                lFetch = 0;
            }
        }
    }
Error:
    return hr;
}


STDMETHODIMP CompilerProject::GetContainedLanguage
(
    LPCWSTR wszMasterBufferPath,
    VSITEMID itemidMasterBuffer,
    IVsTextBufferCoordinator *pBufferCoordinator,
    IVsContainedLanguage **ppContainedLanguage
)
{
    SourceFile *pfile = NULL;
    SourceFileView *pSourceFileView = NULL;
    CComObject<CVBAspx> *pVBAspx = NULL;

    VB_ENTRY();

    auto_ptr<BackgroundCompilerLock> spCompilerLock;

    // This method implements a public interface method, so clear the
    // error info now so caller can't get stale info if the call fails.
    ClearErrorInfo();
    CComPtr<IVsTextLines> pPrimaryBuffer;   //html buffer
    CComPtr<IVsTextLines> pSecondBuffer;    //vb code buffer
    CComBSTR bstrFilePath = wszMasterBufferPath;
    IfFalseGo(wszMasterBufferPath && pBufferCoordinator && itemidMasterBuffer && ppContainedLanguage,E_INVALIDARG);
    IfFailGo(pBufferCoordinator->GetPrimaryBuffer(&pPrimaryBuffer));
    IfFailGo(pBufferCoordinator->GetSecondaryBuffer(&pSecondBuffer));
    *ppContainedLanguage = NULL;

    // Maintaining existing invariant which does not lock the background thread until we complete the
    // above operations
    spCompilerLock.reset(new BackgroundCompilerLock(this));

    // Prepare the addition
    IfFailGo(SourceFile::Create(this,bstrFilePath,NULL,0,itemidMasterBuffer,&pfile));
    if (pfile->IsWatched())
    {
        pfile->UnwatchFile();
    }
    SourceFileView::CreateView(pfile,pSecondBuffer, NULL /*pVsCodeWindow*/, false /*fConnectToDebuggerEvents*/);
    IfFalseGo(pSourceFileView = SourceFileView::GetFromIVsTextLines(pSecondBuffer),E_UNEXPECTED);
    IfFailGo(CComObject<CVBAspx>::CreateInstance(&pVBAspx));
    pVBAspx->AddRef();
    IfFailGo(pVBAspx->Initialize(pSourceFileView,pBufferCoordinator));
    IfFailGo(pVBAspx->QueryInterface(IID_IVsContainedLanguage,(void **)ppContainedLanguage));
    // Commit the addition
    pfile->m_cs = CS_NoState;
    pfile->SetIsAspx(true);
    m_dlFiles[CS_NoState].InsertLast(pfile);
    pfile->SetDecompilationState(pfile->m_cs);
    FireOnFile(CLXN_Add, pfile->GetCodeFile());
    // Demote the whole project to Declared. (Symbols are still valid)
    EditFilter::DecompileProject(this, CS_Declared);
    // But make the project is in NoState so that the new file is dealt with
    EditFilter::DecompileProjectOnly(this, CS_NoState);
    // AssemblyVersionAttribute may have changed -- decompile
    // all referencing projects to avoid stale AssemblyRefs
    EditFilter::DecompileReferencingProjects(this, CS_Bound, CS_Declared);
    pfile = NULL;

    m_fContainedLanguageProject = true;
Error:
    if (hr != NOERROR)
    {
        if (pSourceFileView)
        {
            SourceFileView::ReleaseView(pSourceFileView);
        }
        if (ppContainedLanguage)
        {
            RELEASE(*ppContainedLanguage);
        }
    }

    VB_EXIT_NORETURN();

    RELEASE(pfile);
    RELEASE(pVBAspx);
    return hr;
}

//****************************************************************************
// CompilerProjectSite implementation
//****************************************************************************

// Note that I'm not holding a reference to the CompilerProject. That means
// I rely on the CompilerProject to tell me when it's being destroyed
// (when it's ref count goes to zero). When it tells me, I'll release
// the m_pCompilerProject I'm holding. I'm a zombie now. I'll send an
// event to the listener hopefully the'll do the right thing.
CompilerProjectSite::CompilerProjectSite(
    CompilerProject * const pCompilerProjectP,
    IVbProjectEvents *      pSinkP
) :
    m_pCompilerProject( pCompilerProjectP ),
    m_srpSink         ( pSinkP )
{
    VSASSERT( m_pCompilerProject, "Invalid pointer" );
    VSASSERT( m_pCompilerProject->m_pSinkSite == NULL, "Expect Only one per project" );

    m_pCompilerProject->m_pSinkSite = this;

    m_ulRefs = 1;
}

CompilerProjectSite::~CompilerProjectSite()
{
    if( m_pCompilerProject )
    {
        // It's OK if it has allready been NULLed by the CompilerProject's
        // dtor. So we can't really assert for anything.
        m_pCompilerProject->m_pSinkSite = NULL;
    }
}

STDMETHODIMP CompilerProjectSite::QueryInterface( REFIID, void ** )
{
    return( E_NOTIMPL );
}

STDMETHODIMP_( ULONG ) CompilerProjectSite::AddRef()
{
    return( ++m_ulRefs );
}

STDMETHODIMP_( ULONG ) CompilerProjectSite::Release()
{
    if( --m_ulRefs )
        return( m_ulRefs );

    delete( this );
    return( 0 );
}

STDMETHODIMP CompilerProjectSite::GetSink( REFIID riidP, IUnknown ** ppVP )
{
    if( m_srpSink != NULL )
        return( m_srpSink->QueryInterface( riidP, (void**)ppVP ) );

    return( E_FAIL );
}

STDMETHODIMP CompilerProjectSite::GetHost( REFIID riidP, IUnknown ** ppVP )
{
    if( m_pCompilerProject )
        return( m_pCompilerProject->QueryInterface( riidP, (void**)ppVP ) );

    return( E_ABORT ); // We are a zombie, sorry.
}

STDMETHODIMP CompilerProjectSite::UnAdvise()
{
    // We could potentially re-advise in the future so let's not
    // release the compilerproject pointer we have yet. Either way
    // m_pCompilerProject might be NULL if we got sombied.
    m_srpSink.Release();
    return( S_OK );
}

// Since we don't hold a ref on the compilerproject we make
// sure that the compiler call's this method when it is being
// destroyed. At that point this site is a zombie.
void CompilerProjectSite::Zombie()
{

    if( m_pCompilerProject )
    {
        // Callee is not supposed to AddRef and or store the pointer
        // since it is about to be deleted
        if( m_pCompilerProject->IsMetaData() )
        {
            OnAssembly( CLXN_Remove, m_pCompilerProject );
        } else
        {
            OnProject( CLXN_Remove, m_pCompilerProject);
        }
        m_pCompilerProject = NULL;
    }

    if( m_srpSink )
        m_srpSink->OnClose();


    // Careful, the listener might release us in OnClose. We would have
    // been re-entered on or dtor and we'll be dead at this point.
}

#endif // IDE

//=============================================================================
// Adds a project to my reverse references list.
//=============================================================================

void CompilerProject::AddReferenceOnMe(CompilerProject *pProject)
{
    m_daReferencesOnMe.Add() = pProject;
}

//=============================================================================
// Removes a project from my reverse references list.
//=============================================================================

void CompilerProject::RemoveReferenceOnMe(CompilerProject *pProject)
{
    ReverseReferenceIterator RefIterator(this);
    CompilerProject *pCompilerProject;

    while (pCompilerProject = RefIterator.Next())
    {
        if (pCompilerProject == pProject)
        {
            m_daReferencesOnMe.Remove(RefIterator.GetCurrentIndex());
            break;
        }
    }
}

//=============================================================================
// Adds a project to my reverse implicit references list.
//=============================================================================
#if IDE 
void CompilerProject::AddImplicitReferenceOnMe(AssemblyRefInfo *pAssemblyRefInfo)
{
    VSASSERT(pAssemblyRefInfo &&
             pAssemblyRefInfo->m_pReferencingProject &&
             pAssemblyRefInfo->m_pReferencingProject->IsMetaData(), "Implicit reference from non-metadata project unexpected!!!");

    m_daImplicitReferencesOnMe.Add() = pAssemblyRefInfo;
}

//=============================================================================
// Removes a project from my reverse implicit references list.
//=============================================================================
void CompilerProject::RemoveImplicitReferenceOnMe(AssemblyRefInfo *pAssemblyRefInfo)
{
    VSASSERT(pAssemblyRefInfo &&
             pAssemblyRefInfo->m_pReferencingProject &&
             pAssemblyRefInfo->m_pReferencingProject->IsMetaData(), "Implicit reference from non-metadata project unexpected!!!");

    for(ULONG Index = m_daImplicitReferencesOnMe.Count(); Index > 0; Index--)
    {
        if (m_daImplicitReferencesOnMe.Element(Index - 1) == pAssemblyRefInfo)
        {
            m_daImplicitReferencesOnMe.Remove(Index - 1);
            // Do not break, there might be multiple assembly refs for the same project.
        }
    }
}

void CompilerProject::SyncSourceFiles()
{
    SourceFileIterator sourcefiles(this);
    SourceFile *pFile = NULL;

    // Loop through the source files and make sure we've been informed about
    // any of them that have changed.
    //
    while(pFile = sourcefiles.Next())
    {
#if ID_TEST
        if (VBFTESTSWITCH(fDumpFileChangeNotifications))
        {
            WCHAR wszFile[MAX_PATH];
            DebPrintf("CompilerPackage::StartProjectCompile: '%S' %s being watched\n",
                GetCanonicalFileName(pFile->m_pstrFileName, wszFile, DIM(wszFile)),
                pFile->m_isWatched ? "is" : "is not");
        }
#endif
        if (!pFile->HasSourceFileView())
        {
            // SyncFile will only update a file if we are watching it.
            if (pFile->IsWatched())
            {
                GetCompilerPackage()->SyncFile(pFile->m_pstrFileName);
            }
        }
        else
        {
            pFile->GetSourceFileView()->FlushPendingUpdate();
            pFile->GetSourceFileView()->PurgeChangeInput();
        }
    }
}
#endif IDE

// This method must be called on the foreground thread
void CompilerProject::CheckSolutionExtensions()
{
    // If System.Xml.dll and System.Xml.Linq.dll and System.Query.dll are loaded, then create the XmlHelper extension
    bool foundXml = false;
    bool foundXLinq = false;
    bool foundQuery = false;
    for (ULONG Index = 0; Index < m_daReferences.Count(); Index++)
    {
        STRING *AssemblyName = m_daReferences.Element(Index).m_pCompilerProject->GetAssemblyName();
        if (AssemblyName == STRING_CONST(m_pCompiler, SystemXml))
        {
            foundXml = true;
        }
        else if (AssemblyName == STRING_CONST(m_pCompiler, SystemXmlLinq))
        {
            foundXLinq = true;
        }
        else if (AssemblyName == STRING_CONST(m_pCompiler, SystemCore))
        {
            foundQuery = true;
        }
    }

    if (m_rgfExtensionAdded[SolutionExtension::XmlHelperExtension] != (foundXml && foundXLinq && foundQuery))
    {
        ToggleSolutionExtension(SolutionExtension::XmlHelperExtension);
    }


    bool LoadMyExtension = true;

#if DEBUG
    const LPCWSTR wszEnvironmentVar = L"NOMY";
    GetEnvironmentVariable( wszEnvironmentVar, NULL, 0);
    LoadMyExtension = ::GetLastError() == ERROR_ENVVAR_NOT_FOUND; // When NO_MY is defined, don't load the extension.
    LoadMyExtension &= !(bool)VSFSWITCH(fDebugNoMy); // also when /debug.nomy is on the command-line, don't load the extension
#endif

#if IDE
    // The expression editor never uses the My Extension.
    if (m_isExpressionEditorProject)
    {
        LoadMyExtension = false;
    }
#endif

    // Only load the My template extension if we have a vb runtime reference.

    if ( LoadMyExtension &&
        !g_CompilingTheVBRuntime &&
        m_pVBRuntimeProject != NULL &&
        (m_vbRuntimeKind == DefaultRuntime || m_vbRuntimeKind == SpecifiedRuntime) &&
        !m_rgfExtensionAdded[SolutionExtension::MyTemplateExtension])
    {
        ToggleSolutionExtension(SolutionExtension::MyTemplateExtension);
    }

}

void CompilerProject::AddSolutionExtension(SolutionExtensionEnum Extension)
{
    if( !m_rgfExtensionAdded[Extension] )
    {
        ToggleSolutionExtension(Extension);
    }

    return;
}

void CompilerProject::RemoveSolutionExtension(SolutionExtensionEnum Extension)
{
    if( m_rgfExtensionAdded[Extension] )
    {
        ToggleSolutionExtension(Extension);
    }

    return;
}

void CompilerProject::ToggleSolutionExtension(SolutionExtensionEnum Extension)
{
    if (Extension >= 0 && Extension < _countof(m_rgfExtensionAdded))
    {
        SolutionExtensionData *pData = m_pCompilerHost->GetSolutionExtension(Extension);
        if (pData->m_psbCode == NULL ) // Since we can now use any dll as the vb runtime, we may not have a my template available
        {
            return;
        }

        VB_ENTRY();
        if (m_rgfExtensionAdded[Extension])
        {
            // Remove the extension file
            RemoveFileByNameInternal(pData->m_pstrFileName);
        }
        else
        {
            // Add the extension file
            SourceFile *pFile = NULL;
            IfFailThrow(AddBufferHelper(
                pData->m_psbCode->GetString(), 
                pData->m_psbCode->GetStringLength()*sizeof(WCHAR), 
                pData->m_pstrFileName, 
                VSITEMID_NIL, 
                false, 
                false, 
                &pFile,
                false, //fWatchFile
                Extension // SolutionExtension
                ));
            VSASSERT(pFile,"Unexpected.");
            if (pFile)
            {
                pFile->SetSolutionExtension(pData);
            }
        }

        // Toggle the flag
        m_rgfExtensionAdded[Extension] = !m_rgfExtensionAdded[Extension];

#if IDE
        // Force update the schemas if we disabled the xml helper so the IDE
        // doesn't try to use the invalid schemas.
        if (Extension == SolutionExtension::XmlHelperExtension &&
            m_XmlSchemas.IsOpen())
        {
            VSASSERT(!m_rgfExtensionAdded[Extension], "How can xml schemas be enabled if they were already opened?");
            UpdateXmlSchemas();
        }
#endif
        VB_EXIT_NORETURN();
    }
}

bool CompilerProject::AddReference
(
    CompilerProject *pProjectReferenced
)
{
    return AddReference(pProjectReferenced, REFERENCEFLAG_MANUAL);
}

bool CompilerProject::AddReference
(
    CompilerProject *pProjectReferenced,
    UINT ReferenceFlags
)
{
    // Do we already have a reference to this?
    if (IsProjectReferenced(pProjectReferenced))
    {
        // Dev11 Bug 52100
        // CheckSolutionExtensions() also depends on some other
        // conditions. So at this brach, even the project has been added,
        // but other conditions may change, so we need to call CheckSolutionExtensions()
        // again.
        CheckSolutionExtensions();
        return false;
    }

    // Add the reference to the list.
    // Note refAndPath->m_fUsed already set to 0 by DynamicArray::Add()
    ReferencedProject *pRef  = &m_daReferences.Add();
    pRef->m_pCompilerProject = pProjectReferenced;
    pRef->m_fManual   = (ReferenceFlags & REFERENCEFLAG_MANUAL)   != 0;
    pRef->m_fStandard = (ReferenceFlags & REFERENCEFLAG_STANDARD) != 0;
    pRef->m_fRequired = (ReferenceFlags & REFERENCEFLAG_REQUIRED) != 0;
    pRef->m_fEmbedded = (ReferenceFlags & REFERENCEFLAG_EMBEDDED) != 0;

    // Dev10 #735384
    if (pRef->m_fEmbedded)
    {
        SetPotentiallyEmbedsPiaTypes(true);
    }

    pProjectReferenced->AddRef();
    pProjectReferenced->AddReferenceOnMe(this);

    // Some solution extensions are dependent upon certain references
    CheckSolutionExtensions();

#if IDE 
    CObjectBrowserLibraryMan * pLibMan = GetCompilerPackage()->AliasObjectBrowserLibraryManager();

    if (pLibMan && pProjectReferenced->IsMetaData())
    {
        CAssemblyReferenceManager *pAsmLibMgr = pLibMan->GetAssemblyReferenceManager();

        if (pAsmLibMgr)
        {
            pAsmLibMgr->AddReference(pProjectReferenced);
        }
    }

    if (pProjectReferenced->IsMetaData() && GetCompilerPackage()->GetIntellidocLoadAndIndexService()) 
    {
        MetaDataFile * pMetaDataFile = pProjectReferenced->GetPrimaryModule();
        
        GetCompilerPackage()->GetIntellidocLoadAndIndexService()->AddIntellidocToCache(pMetaDataFile, this);
    }

    if (GetCompilerPackage()->IsInBatch())
    {
        EditFilter::DecompileProjectAndReferencingProjectsNoCommit(this, CS_Declared);
    }
    else
    {
        EditFilter::DecompileProjectAndReferencingProjects(this, CS_Declared);
    }
    
    // Adding a reference can change the order of compilation.
    GetCompilerHost()->DestroyProjectCompilationList();

#endif IDE

#if DEBUG
    ValidateAllReferences();
#endif DEBUG

    return true;
}

//=============================================================================
// ReleaseOneReference is a helper for ReleaseReferences
// and ReleaseAllReferences.
//
// If fShrink is false, the DynamicArray is not compressed after the
// reference is removed, so the caller is responsible for cleaning up
// that array slot.
//=============================================================================
void CompilerProject::ReleaseOneReference
(
    DynamicArray<ReferencedProject> *prgRefs,
    unsigned iRef,      // Index of reference to remove
    bool fShrink        // Shrink the DynamicArray after deleting ref?
#if IDE 
    , CAssemblyReferenceManager *pAsmLibMgr // Optional ref manager (if NULL, we will fetch it)
#endif
)
{
    VSASSERT(prgRefs, "Where's the reference array?");

    ReferencedProject *rgref = prgRefs->Array();
    unsigned c = prgRefs->Count();

    VSASSERT(iRef < c, "Out of bounds");

    // Delete the reference.  This may cause it to go away.
    CompilerProject *pCompilerProject = rgref[iRef].m_pCompilerProject;

    if (pCompilerProject)
    {
        pCompilerProject->RemoveReferenceOnMe(this);

#if IDE

        // If caller didn't supply assembly ref manager, go get it now
        if (pAsmLibMgr == NULL)
        {
            CObjectBrowserLibraryMan *pLibMan = GetCompilerPackage()->AliasObjectBrowserLibraryManager();
            if (pLibMan)
            {
                pAsmLibMgr = pLibMan->GetAssemblyReferenceManager();
            }
        }

        if (pAsmLibMgr)
        {
            pAsmLibMgr->RemoveReference(pCompilerProject);
        }
#endif

#if IDE 

        if (GetCompilerPackage()->GetIntellidocLoadAndIndexService())
        {
            GetCompilerPackage()->GetIntellidocLoadAndIndexService()->RemoveIntellidocFromCache(pCompilerProject);
        }

        // If a metadata project is going away, then we need to decompile the projects that implicitly
        // reference this metadata project.
        //
        if (pCompilerProject->IsMetaData() && pCompilerProject->m_daReferencesOnMe.Count() == 0)
        {
            EditFilter::DecompileReferencingProjects(pCompilerProject, CS_MAX, CS_Declared);
        }
#endif

        pCompilerProject->Release();
    }

    if (fShrink)
    {
        prgRefs->Remove(iRef);

#if DEBUG
    ValidateAllReferences();
#endif DEBUG
    }
}

void CompilerProject::ReleaseAllReferences(DynamicArray<ReferencedProject>* pArray)
{
    return ReleaseReferences(pArray, false /*standardLibsOnly*/);
}

void CompilerProject::ReleaseReferences
(
    DynamicArray<ReferencedProject>* pArray,
    bool standardLibsOnly
)
{
    VSASSERT(pArray, "Where's the reference array?");

    ReferencedProject *rgref = pArray->Array();
    unsigned c = pArray->Count();

    if (c == 0)
    {
        return; // Nothing to do
    }

#if IDE 
    CObjectBrowserLibraryMan*  pLibMan = GetCompilerPackage()->AliasObjectBrowserLibraryManager();
    CAssemblyReferenceManager* pAsmLibMgr = NULL;

    if (pLibMan)
    {
        pAsmLibMgr = pLibMan->GetAssemblyReferenceManager();
    }
#endif

    GetCompilerHost()->DestroyProjectCompilationList();


    // Delete the references.  This may cause some of them to go away.
    // When removing references from the list, we'll either shrink the
    // list one by one or reset it all at once.
    bool shrinkOne = false; // controls shrinking the array one-by-one

    for (unsigned i = 0; i < c; (shrinkOne)? c-- : i++)
    {
        ReferencedProject *pref = &rgref[i];
        bool releaseOne = true;  // default to release-all
        shrinkOne = false;       // default to no-shrink

        // ignore null projects
        if (pref->m_pCompilerProject == NULL)
        {
            continue;
        }

        // If releasing standard libs only, reset the standard flag
        // and check if the reference is now unused.
        if (standardLibsOnly)
        {
#if IDE 
            if (pref->m_fStandard)
            {
                pref->m_fStandard = false;
            }

            // remove this one if it is no longer in use at all
            releaseOne = (!pref->m_fRequired && !pref->m_fStandard && !pref->m_fManual);
            shrinkOne  = releaseOne;
#else
            // Non-IDE. Don't release standard refs. See assert at top of function.
            VSFAIL("Release standard libs is only supported in the IDE.");
            break;
#endif // IDE
        }

        if (releaseOne)
        {
            // We pass shrinkOne = false because we'll be doing pArray->Reset at the end.
            // No sense doing an O(n^2) shrink when releasing all references.
#if IDE
            ReleaseOneReference(pArray, i, shrinkOne, pAsmLibMgr);
#else
            ReleaseOneReference(pArray, i, shrinkOne);
#endif
        }
    }

    // If we didn't delete just standard libs, then remove all at once.
    if (!standardLibsOnly)
    {
        pArray->Reset();
    }

#if DEBUG
    ValidateAllReferences();
#endif DEBUG
}

bool CompilerProject::HasEmbeddedReferenceTo(CompilerProject *pProject)
{
    ReferencedProject *rgref = m_daReferences.Array();
    ULONG cRef = m_daReferences.Count();
    for (ULONG iRef = 0; iRef < cRef; iRef++)
    {
        if (pProject == rgref[iRef].m_pCompilerProject)
        {
            return rgref[iRef].m_fEmbedded;
        }
    }
    return false;
}

//
// Reference usage tracking implementations
//
// FindReferenceIndex
// (Helper for SetReferenceUsage)
//
// Searches through m_daReferences for
// pProjectReferenced and returns index if found.
//
// [in]  pProjectReferenced: CompilerProject to find
// [out] iRef: Index in m_daReferences of entry found.  Only set if iRef != NULL.
//
// Returns: true if pProjectReferenced found, false otherwise
//
bool CompilerProject::FindReferenceIndex
(
    CompilerProject          *pProjectReferenced,
    unsigned                 *iRef
)
{
    VSASSERT(iRef, "Bad reference index return variable.");

    // Make sure we have a project to find
    if (pProjectReferenced)
    {
        ReferencedProject *rgref = m_daReferences.Array();
        unsigned cEntries = m_daReferences.Count();

        // Search through references of current project for indicated reference
        for (unsigned iIndex = 0; iIndex < cEntries; iIndex++)
        {
            if (rgref[iIndex].m_pCompilerProject == pProjectReferenced)
            {
                *iRef = iIndex;
                return true;
            }
        }
    }

    return false;
}

#if IDE 

// SetReferenceUsage
// (IDE Only)
//
// Finds and flags a referenced project as used
//
// [in]  pProjectReferenced: Referenced project to record usage for
//
void CompilerProject::SetReferenceUsage
(
    CompilerProject        *pProjectReferenced
)
{
    // Special case ignore mscorlib and Microsoft.VisualBasic since they are very
    // common & we will never report these references as unused
    if (!pProjectReferenced ||
        pProjectReferenced == GetCompilerHost()->GetComPlusProject() ||
        pProjectReferenced->IsDefaultVBRuntime())
    {
        return;
    }

    VSASSERT(m_cs < CS_Compiled, "Finding references after compiled?");

    // Get reference index from CompilerProject
    unsigned iRef;
    if (!FindReferenceIndex(pProjectReferenced, &iRef))
    {
        // Project not referenced
        return;
    }

#if DEBUG
    // Print out debug information when first usage is found
    if (VSFSWITCH(fDumpReferenceUsage) && !m_daReferences.Element(iRef).m_fUsed)
    {
        DebPrintf("    Reference usage: %S -> %S (promoting to ",
            GetAssemblyName(),
            pProjectReferenced->m_pstrFileName);

        DumpCompilationState((CompilationState) (m_cs + 1));

        DebPrintf(").\n");
    }
#endif DEBUG

    // Set global usage flag
    m_daReferences.Element(iRef).m_fUsed = true;
}


// SetReferenceUsageForImplementsAndInterface
// (Helper for SetReferenceUsageForSymbol)
//
// Flags referenced projects containing symbol's hierarchy of implemented interfaces as used
//
// [in]  DependedOn: symbol/interface that is used
//
void CompilerProject::SetReferenceUsageForImplementsAndInterface
(
    BCSYM_NamedRoot *DependedOn
)
{
    VSASSERT(DependedOn->IsContainer(), "Interface not a container?");

    // Don't walk interfaces if we haven't resolved them.
    // This can only happen if the base is defined in the same project, since all
    // dependent projects are compiled first.  That means a dependency will be
    // set on the bases eventually
    if (Bindable::GetStatusOfResolveBases(DependedOn->PContainer()) == Bindable::Done)
    {
        BCITER_ImplInterfaces ImplInterfacesIter(DependedOn->PContainer());

        while (BCSYM_Implements *Implements = ImplInterfacesIter.GetNext())
        {
            BCSYM *Interface = Implements->GetInterfaceIfExists();

            if (Interface && !Interface->IsBad())
            {
                VSASSERT(Interface->IsNamedRoot(), "Interface not a named root?");

                // Set usage on interface
                SetReferenceUsage(Interface->PNamedRoot()->GetContainingProject());

                // Recurse
                SetReferenceUsageForImplementsAndInterface(Interface->PNamedRoot());
            }
        }
    }
}

// SetReferenceUsageForType
// (Helper for SetReferenceUsageForSymbol)
//
// Digs through a type and flags referenced projects associated with type as used
//
// [in]  DependedOnType: symbol who's type is used.  OK if NULL.
//
void CompilerProject::SetReferenceUsageForType
(
    BCSYM *DependedOnType
)
{
    if (DependedOnType)
    {
        DependedOnType = DependedOnType->ChaseToNamedType();
        AssertIfNull(DependedOnType);

        if (DependedOnType)
        {
            if (DependedOnType->IsNamedType())
            {
                DependedOnType = DependedOnType->PNamedType()->GetSymbol();

                // Has this type been resolved yet?
                //
                // The type is not resolved in some cases where this named type is also
                // defined in the current/same project. In these cases, it is acceptable
                // to skip recording this usage now, because it will be recorded eventually
                // when this named type is bound.

                if (NULL == DependedOnType)
                {
                    return;
                }
            }

            if (DependedOnType->IsNamedRoot())
            {
                SetReferenceUsageForSymbol(DependedOnType->PNamedRoot());
            }
        }
    }
}

// SetReferenceUsageForSymbol
// (IDE Only)
//
// Flags referenced projects containing a symbol, its base class & implements, and parameters
// and return value as used
//
// [in]  DependedOn: Symbol that is used
//
void CompilerProject::SetReferenceUsageForSymbol
(
    BCSYM_NamedRoot *DependedOn
)
{
    VSASSERT(DependedOn, "Bad argument.");

    // Do not track usages unless we are calculating unused references
    // Don't log reference usage on bad symbols
    if (!IsGettingUnusedReferences() || DependedOn->IsBad())
    {
        return;
    }

    // Always uses on the symbol's project
    SetReferenceUsage(DependedOn->GetContainingProject());

    // If symbol has type, also uses on type's project
    if (DependedOn->IsMember())
    {
        SetReferenceUsageForType(DependedOn->PMember()->GetRawType());
    }

    if (DependedOn->IsNamespace())
    {
        // The only time we will be doing this is for unused Imports of namespaces.
        // Go ahead and iterate rest of namespaces in ring and set usage on the projects
        // since we don't really know which one is being imported.
        BCSYM_Namespace *Namespace = DependedOn->PNamespace();
        BCSYM_Namespace *CurrentNamespace = Namespace;

        while ((CurrentNamespace = CurrentNamespace->GetNextNamespace())
            && CurrentNamespace != Namespace)
        {
            SetReferenceUsage(CurrentNamespace->GetContainingProject());
        }
    }
    else if (DependedOn->IsClass() || DependedOn->IsInterface())
    {
        // If it is a class or interface, then set usage on all its bases and implements as well
        if (DependedOn->IsClass())
        {
            BCSYM *BaseClass = DependedOn;

            // Only dig into bases that we have resolved already (don't trigger resolution here.)
            while (Bindable::GetStatusOfResolveBases(BaseClass->PContainer()) == Bindable::Done
                && (BaseClass = BaseClass->PClass()->GetBaseClass())
                && !BaseClass->IsBad())
            {
                VSASSERT(BaseClass->IsClass(), "Base class is not a class?");
                VSASSERT(BaseClass->PClass()->GetContainingProject(), "Class without a project?");

                // Set usage on base class project
                SetReferenceUsage(BaseClass->PClass()->GetContainingProject());
            }
        }

        // Set usage on all projects of implemented interfaces
        SetReferenceUsageForImplementsAndInterface(DependedOn);
    }
    else if (DependedOn->IsProc())
    {
        // set usage on parameter types of procs.  Return type already set by
        // setting usage on DependedOn's type above.
        BCITER_Parameters ParamIter(DependedOn->PProc());

        while (BCSYM_Param *Param = ParamIter.PparamNext())
        {
            SetReferenceUsageForType(Param->GetRawType());
        }
    }

    if (DependedOn->IsDelegate())
    {
        // For delegates, set usage on the invoke method.
        //
        // Though the params and return type are usually already marked because of the dependency
        // logged on the invoke or add_e synthetic methods, this is not always the case.
        // For example: We have an event with delegate type and one parameter.  The delegate type is
        // defined in lib1.dll. The parameter's is defined in lib2.dll. In an app we use addhandler to
        // add a handler for the event, passing the address of a funtion with a parameter of generic type.
        // Here add_e only has the parameter with generic type and not the type defined in lib2.dll.
        // Hence, for such cases, we need to walk the invoke method.
        BCSYM_NamedRoot *InvokeMethod = GetInvokeFromDelegate(DependedOn, m_pCompiler);

        // Bug VSWhidbey 498465. InvokeMethod is NULL for System.MulticastDelegate.
        //
        if (InvokeMethod)
        {
            SetReferenceUsageForSymbol(InvokeMethod);
        }
    }

}
#endif IDE

//
// Returns true if the given metadata file is represented by this CompilerProject.
//
bool CompilerProject::FMetadataProjectRepresentsFile
(
    _In_z_ STRING *pstrFileName
)
{
    // Can only call this function on a metadata project
    VSASSERT(IsMetaData(), "Not a metadata project!");
    VSASSERT(pstrFileName != NULL, "Where's the file name?");

    return (CompareFilenames(GetFileName(), pstrFileName) == 0);
}

//=============================================================================
// Name to use when reporting errors against this project.
// Note that this does NOT return a STRING, just a WCHAR*.
//=============================================================================
WCHAR *CompilerProject::GetErrorName()
{
    // Try assembly name first, then file name (without path).
    if (GetAssemblyName())
    {
        return GetAssemblyName();
    }

    VSASSERT(IsMetaData(), "Must be MetaData project.");
    VSASSERT(GetFileName() != NULL, "Project has no assembly name or file name?");
    return PathFindName(GetFileName());
}

//============================================================================
// Retrieves the XML for the whole project.
//============================================================================

HRESULT CompilerProject::GetXMLDocForProject(BaseWriter *pXMLDocForProject)
{
    VB_ENTRY();
    pXMLDocForProject->AppendString(XML_VERSION);

    XMLDocStartTag(pXMLDocForProject, XMLDoc_Doc);
    {
        if (!OutputIsModule())
        {
            XMLDocStartTag(pXMLDocForProject, XMLDoc_Assembly);
            {
                XMLDocStartTag(pXMLDocForProject, XMLDoc_Name);
                {
                    WCHAR *ValidXMLString = NULL;

                    // Check to make sure that any names we generate in the XML are valid names. We can do that by using XMLGen's
                    // function.
                    XMLGen::CheckForEntityReferences(GetAssemblyName(), &ValidXMLString, false);
                    pXMLDocForProject->AppendString(ValidXMLString);

                    delete[] ValidXMLString;
                }
                XMLDocEndTag(pXMLDocForProject, XMLDoc_Name);
            }
            XMLDocEndTag(pXMLDocForProject, XMLDoc_Assembly);
        }

        pXMLDocForProject->AppendChar(UCH_CR);
        pXMLDocForProject->AppendChar(UCH_LF);

        XMLDocStartTag(pXMLDocForProject, XMLDoc_Members);
        {
            SourceFileIterator ProjectFiles(this);
            SourceFile* pFile;

            // Loop over all source files and get the XML needed to spit into the XML doc file.
            while (pFile = ProjectFiles.Next())
            {
                // Get the XML for the file.
                hr = pFile->GetXMLDocFile()->GetXMLDocForFile(pXMLDocForProject);

                if (FAILED(hr))
                {
                    break;
                }
            }
        }
        XMLDocEndTag(pXMLDocForProject, XMLDoc_Members);
    }
    XMLDocEndTag(pXMLDocForProject, XMLDoc_Doc);
    VB_EXIT();
}

//============================================================================
// Write out the XMLDoc file to the indicated filename.
//============================================================================
void CompilerProject::WriteToXMLDocFile()
{
    HANDLE hFile = CreateFileW(
        GetXMLDocFileName(),
        GENERIC_READ | GENERIC_WRITE,
        0, NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        DWORD dwLastError = GetLastError();

        // We use hr to generate a rich error message.
        m_ErrorTable.CreateErrorWithError(WRNID_XMLCannotWriteToXMLDocFile2, NULL, HRESULT_FROM_WIN32(dwLastError), GetXMLDocFileName());
    }
    else
    {
        DWORD BytesWritten = 0;
        bool ShouldDeleteFile = false;

        if (!WriteFile(hFile, UTF_8_BYTE_ORDER_MARKER, UTF_8_BYTE_ORDER_MARKER_LENGTH, &BytesWritten, NULL))
        {
            DWORD dwLastError = GetLastError();

            // We use hr to generate a rich error message.
            m_ErrorTable.CreateErrorWithError(WRNID_XMLCannotWriteToXMLDocFile2, NULL, HRESULT_FROM_WIN32(dwLastError), GetXMLDocFileName());
            ShouldDeleteFile = true;
        }
        else
        {
            UTF8FileWriter XMLDocForProject(hFile, m_pCompiler);
            VB_ENTRY();
            hr = GetXMLDocForProject(&XMLDocForProject);
            VB_EXIT_NORETURN();

            if (FAILEDHR(hr))
            {
                m_ErrorTable.CreateErrorWithError(WRNID_XMLCannotWriteToXMLDocFile2, NULL, hr, GetXMLDocFileName());
                ShouldDeleteFile = true;
            }

        }
        CloseHandle(hFile);

        if (ShouldDeleteFile)
        {
            DeleteFileW(GetXMLDocFileName());
        }
    }
}

//============================================================================
// Write out the XMLDoc file to the indicated filename.
//============================================================================

void CompilerProject::GenerateXMLDocFile()
{
    VSASSERT(GetXMLDocFileName(), L"XMLDoc file name can't be null here!");

    // First, remove the file we are just about to generate. This makes it possilbe for us to add the file or remove it
    // depending on the state of the XML doc comments in the code, without having any stale files around. We ignore the
    // return value from DeleteFile because the file may not exist in the first place.
    if (!DeleteFile(GetXMLDocFileName()))
    {
        // Can't delete the file
        DWORD dwLastError = GetLastError();

        if (dwLastError == ERROR_FILE_NOT_FOUND)
        {
            // It is OK if the file doesn't exist.
            WriteToXMLDocFile();
        }
        else
        {
            // We use hr to generate a rich error message.
            m_ErrorTable.CreateErrorWithError(WRNID_XMLCannotWriteToXMLDocFile2, NULL, HRESULT_FROM_WIN32(dwLastError), GetXMLDocFileName());
        }
    }
    else
    {
        // Deleting the file was OK, go ahead and process the XML
        WriteToXMLDocFile();
    }
}


#if DEBUG
//=============================================================================
// DEBUG-only helper that returns the project's name.  The returned string
// has no intrinsic semantic value -- it's just cobbled-together bits
// and pieces that make debugging a little bit easier.
//=============================================================================
STRING *CompilerProject::DebGetName()
{
    if (this == NULL)
    {
        return NULL;
    }

    if (this == GetCompilerHost()->GetComPlusProject())
    {
        return m_pCompiler->AddString(L"COM+");
    }

    if (this->IsDefaultVBRuntime())
    {
        return m_pCompiler->AddString(L"VB Runtime");
    }

    WCHAR *pwszName = NULL;
    int cch;
    WCHAR *pwszBuf = NULL;;
    NorlsAllocator nraTemp(NORLSLOC);

    if (IsMetaData())
    {
        pwszName = GetFileName() ? GetFileName() : L"<unknown-file>";
        cch = (int) wcslen(pwszName) + 100;
        pwszBuf = (WCHAR *)nraTemp.Alloc(cch * sizeof(WCHAR));
        swprintf_s(pwszBuf, cch, L"MD-%p %s", this, pwszName);
    }
    else
    {
        pwszName = GetAssemblyName() ? GetAssemblyName() : L"<unknown-assembly>";
        cch = (int) wcslen(pwszName) + 100;
        pwszBuf = (WCHAR *)nraTemp.Alloc(cch * sizeof(WCHAR));
        swprintf_s(pwszBuf, cch, L"VB-%p %s", this, pwszName);
    }

    return m_pCompiler->AddString(pwszBuf);
}

#endif

#if FV_TRACK_MEMORY

void CollectSymbolTableInfo
(
    BCSYM *Symbol,
    int *SymbolCount
)
{
    if (!Symbol)
        return;

    SymbolCount[Symbol->GetKind()]++;

    if (Symbol->IsContainer() && Symbol->PContainer()->AreChildrenLoaded())
    {
        BCSYM_Container *Container = Symbol->PContainer();
        BCITER_CHILD MemberIterator(Container);

        CollectSymbolTableInfo(Container->GetHash(), SymbolCount);
        CollectSymbolTableInfo(Container->GetUnBindableChildrenHash(), SymbolCount);

        for (BCSYM_NamedRoot *Member = MemberIterator.GetNext();
             Member;
             Member = MemberIterator.GetNext())
        {
            CollectSymbolTableInfo(Member, SymbolCount);
        }
    }

    if (Symbol->IsProc())
    {
        CollectSymbolTableInfo(Symbol->PProc()->GetGenericParamsHash(), SymbolCount);

        for (BCSYM_Param *Param = Symbol->PProc()->GetFirstParam();
             Param;
             Param = Param->GetNext())
        {
            CollectSymbolTableInfo(Param, SymbolCount);
        }
    }

    if (Symbol->IsProperty())
    {
        CollectSymbolTableInfo(Symbol->PProperty()->GetProperty(), SymbolCount);
        CollectSymbolTableInfo(Symbol->PProperty()->SetProperty(), SymbolCount);
    }

    if (Symbol->IsClass())
    {
        CollectSymbolTableInfo(Symbol->PClass()->GetBackingFieldsForStatics(), SymbolCount);
        CollectSymbolTableInfo(Symbol->PClass()->GetGenericParamsHash(), SymbolCount);
    }

    if (Symbol->IsInterface())
    {
        CollectSymbolTableInfo(Symbol->PInterface()->GetGenericParamsHash(), SymbolCount);
    }
}

void
CompilerProject::DumpProjectMemoryUse
(
)
{
    FileIterator Iter(this);

    for (CompilerFile *File = Iter.Next(); File; File = Iter.Next())
    {
        int SymbolCount[SYM_Max];
        NorlsAllocatorDescriptor *pDescriptor = File->SymbolStorage()->GetDescriptor();

        memset(SymbolCount, 0, SYM_Max * sizeof(int));

        CollectSymbolTableInfo(File->GetUnnamedNamespace(), SymbolCount);

        DebPrintf("\"%S\" (Official)", File->GetFileName());

        for (int SymType = 0; SymType < SYM_Max; SymType++)
        {
            DebPrintf(",%d", SymbolCount[SymType]);
        }

        DebPrintf("\n");

        DebPrintf("\"%S\" (Allocated)", File->GetFileName());

        for (int SymType = 0; SymType < SYM_Max; SymType++)
        {
            DebPrintf(",%d", pDescriptor->m_SymbolCount[SymType]);
        }

        DebPrintf("\n");

        DebPrintf("\"%S\" (Extra)", File->GetFileName());

        for (int SymType = 0; SymType < SYM_Max; SymType++)
        {
            DebPrintf(",%d", pDescriptor->m_SymbolExtraBytes[SymType]);
        }

        DebPrintf("\n");
    }
}

#endif

SecAttrInfo * CompilerProject::CreateSecAttrsForSymbol
(
    BCSYM *pSymbol,
    mdToken tkSymbol,
    ULONG cSecAttrMax
)
{
    AssertIfNull(pSymbol);
    AssertIfFalse(cSecAttrMax > 0);

    SecAttrInfo *pSecAttrInfo = (SecAttrInfo *)m_nraSymbols.Alloc(sizeof SecAttrInfo);

    pSecAttrInfo->pSymbol = pSymbol;
    pSecAttrInfo->tkSymbol = tkSymbol;
    pSecAttrInfo->prgSecAttr = (COR_SECATTR *)m_nraSymbols.Alloc(sizeof(COR_SECATTR) * cSecAttrMax);
    pSecAttrInfo->prgSecAttrErrorInfo = (SecAttrErrorInfo *)m_nraSymbols.Alloc(sizeof(SecAttrErrorInfo) * cSecAttrMax);

    pSecAttrInfo->cSecAttrMax = cSecAttrMax;
    pSecAttrInfo->cSecAttr = 0;

    m_slAppliedSecurityAttributes.InsertLast(pSecAttrInfo);

    return pSecAttrInfo;
}

void CompilerProject::SaveSecAttr
(
    SecAttrInfo *pSecAttrInfo,
    mdMemberRef tkCtor,
    BYTE *pbCustomAttrBlob,
    ULONG cbCustomAttrBlob,
    _In_z_ STRING *pstrAttrName,
    Location &Loc,
    BCSYM_NamedRoot *pNamedContext
)
{
    AssertIfNull(pSecAttrInfo);
    AssertIfTrue(IsNilToken(tkCtor));
    AssertIfNull(pstrAttrName);
    AssertIfNull(pNamedContext);

    IfFalseThrow(pSecAttrInfo->cSecAttr < pSecAttrInfo->cSecAttrMax);

    ULONG Current = pSecAttrInfo->cSecAttr;

    pSecAttrInfo->prgSecAttr[Current].tkCtor = tkCtor;

    if (cbCustomAttrBlob == 0)
    {
        pSecAttrInfo->prgSecAttr[Current].pCustomAttribute = NULL;
    }
    else
    {
        BYTE *pbNewBlob = (BYTE *)m_nraSymbols.Alloc((size_t)cbCustomAttrBlob);
        memcpy(pbNewBlob, pbCustomAttrBlob, (size_t)cbCustomAttrBlob);
        pSecAttrInfo->prgSecAttr[Current].pCustomAttribute = pbNewBlob;
    }

    pSecAttrInfo->prgSecAttr[Current].cbCustomAttribute = cbCustomAttrBlob;

    pSecAttrInfo->prgSecAttrErrorInfo[Current].pstrAttrName = pstrAttrName;
    pSecAttrInfo->prgSecAttrErrorInfo[Current].Loc = Loc;
    pSecAttrInfo->prgSecAttrErrorInfo[Current].pContext = pNamedContext;

    pSecAttrInfo->cSecAttr++;
}

#if IDE
void CompilerProject::UpdateENCMetaDataInfo()
{
    if (m_fNeedtoUpdateENCMetaData && m_pENCBuilder && GetCompState() == CS_Compiled)
    {
        VSASSERT(!m_pENCBuilder->WasEditPerformed(), "ENC Builder should not pick up changes made in design mode");
        this->StopDebuggingPE();
        this->StartDebuggingPE();
        m_fNeedtoUpdateENCMetaData = false;
    }
}
#endif


/* Start - Implementation for ProjectLevelImportsList */

CompilerProject::ProjectLevelImportsList::ProjectLevelImportsList
(
    Compiler *pCompiler,
    CompilerProject *pOwningCompilerProject
)
    : m_pCompiler(pCompiler),
      m_pOwningCompilerProject(pOwningCompilerProject),
      m_ImportsAllocator(NORLSLOC),
      m_ImportsErrorLog(pCompiler, pOwningCompilerProject, NULL),
      m_pProjectLevelImportInfos(NULL),
      m_pProjectLevelImportTargets(NULL)
{
}



void
CompilerProject::ProjectLevelImportsList::AddImport
(
    LPCWSTR ImportString
)
{
    CheckInMainThread();

    // Allocate the ProjectLevelImportInfo
    //
    VBHeapPtr<ProjectLevelImportInfo> pProjectImport(1);

    // Store the string length
    //
    pProjectImport->m_StrLen = (unsigned)wcslen(ImportString);

    VSASSERT(pProjectImport->m_StrLen > 0, "Empty import string unexpected!!!");

    // Allocate and copy the imports string

    size_t bufferSize;
    pProjectImport->m_OriginalImportString = RawStringBufferUtil::AllocateNonNullCharacterCount(pProjectImport->m_StrLen, &bufferSize);
    memcpy(pProjectImport->m_OriginalImportString, ImportString, bufferSize);

    pProjectImport->m_Next = m_pProjectLevelImportInfos;
    m_pProjectLevelImportInfos = pProjectImport.Detach();
}

ImportedTarget * CompilerProject::ProjectLevelImportsList::BuildImportTarget
(
    const WCHAR * ImportString,
    unsigned ImportStringLength,
    unsigned ImportIndex,
    ImportedTarget * PreviousTarget
)
{
    // Parse the imports string
    Scanner
        Scanner(
            m_pCompiler,
            ImportString,
            ImportStringLength,
            0,
            ImportIndex,      // Setting the start line number to be Index of the import info helps with
                              // morphing error info for project level imports to be more user friendly
            0);

    NorlsAllocator ParseTreeAllocator(NORLSLOC);
    Parser Parser(&ParseTreeAllocator, m_pCompiler, m_pOwningCompilerProject->GetCompilerHost(), false, m_pOwningCompilerProject->GetCompilingLanguageVersion());
    ParseTree::ImportDirective *ParsedImport = NULL;

    bool ErrorInImport = false;

    ParsedImport =
        Parser.ParseOneImportsDirective(
            &Scanner,
            &m_ImportsErrorLog,
            ErrorInImport);


    if (ErrorInImport)
    {
        return NULL;
    }

    // Build the imports target from the parsed imports tree

    ImportedTarget *ImportTarget =
        Declared::BuildProjectLevelImport(
            ParsedImport,
            &m_ImportsAllocator,
            &m_ImportsErrorLog,
            m_pOwningCompilerProject);

    if (ImportTarget)
    {
        if (PreviousTarget)
        {
            PreviousTarget->m_pNext = ImportTarget;
        }
        else
        {
            m_pProjectLevelImportTargets = ImportTarget;
        }
        return ImportTarget;
    }

    return PreviousTarget;
}

void CompilerProject::ProjectLevelImportsList::ReBuildAllImportTargets()
{
    // Dev10 #696874: We need to reparse project level imports every time because
    // this recreates NamedTypes that might be referring to obsolete symbols as context, etc.

    m_ImportsAllocator.FreeHeap();
    m_ImportsErrorLog.DeleteAll();

    m_pProjectLevelImportTargets = NULL;

    ImportedTarget * ImportTarget = NULL;
    unsigned ImportIndex = 0;
    bool EmptyXmlPrefixDefined = false;

    // Ensure that default imports have been added before building
    // Bug 45901: Do not add default Xml imports to a metadata project
    if (!m_pOwningCompilerProject->IsMetaData())
    {
        WCHAR XmlNamespace[] = L"<xmlns:xml=\"http://www.w3.org/XML/1998/namespace\">";
        ImportTarget = BuildImportTarget(XmlNamespace, DIM(XmlNamespace) - 1, 0, ImportTarget);

        WCHAR XmlNsNamespace[] = L"<xmlns:xmlns=\"http://www.w3.org/2000/xmlns/\">";
        ImportTarget = BuildImportTarget(XmlNsNamespace, DIM(XmlNsNamespace) - 1, 0, ImportTarget);
    }

    // Note when building the Import targets, they are in the same order
    // in the list as the import strings. This is useful to report
    // dupe errors against the correct entity.

    for (ProjectLevelImportInfo *pCurrentImportInfo = m_pProjectLevelImportInfos;
         pCurrentImportInfo;
         pCurrentImportInfo = pCurrentImportInfo->m_Next)
    {

        ImportTarget = BuildImportTarget(pCurrentImportInfo->m_OriginalImportString, pCurrentImportInfo->m_StrLen, ImportIndex, ImportTarget);
        ImportIndex++;

        if (!EmptyXmlPrefixDefined &&
            ImportTarget &&
            ImportTarget->m_IsXml &&
            ImportTarget->m_pstrAliasName &&
            StringPool::StringLength(ImportTarget->m_pstrAliasName) == 0)
        {
            EmptyXmlPrefixDefined = true;
        }
    }

    if (!EmptyXmlPrefixDefined &&
        !m_pOwningCompilerProject->IsMetaData()) // Bug #93847 - DevDiv Bugs : see comment about Bug 45901 above
    {
        WCHAR XmlNsNamespace[] =  L"<xmlns=\"\">";
        BuildImportTarget(XmlNsNamespace, DIM(XmlNsNamespace) - 1, 0, ImportTarget);
    }

}

bool CompilerProject::ProjectLevelImportsList::IsImported
(
    LPCWSTR ImportString
)
{
    // This is not very efficient, but not a common scenario. So no point
    // using up more space in the data structures to make this scenario
    // faster.
    bool RetVal = false;
    ProjectLevelImportInfo **ppReferenceToCurrentInfo = &m_pProjectLevelImportInfos;

    for (ProjectLevelImportInfo *pCurrentImportInfo = m_pProjectLevelImportInfos;
         pCurrentImportInfo;
         pCurrentImportInfo = pCurrentImportInfo->m_Next)
    {
        if (wcscmp(ImportString, pCurrentImportInfo->m_OriginalImportString) == 0)
        {
            RetVal = true;
            break;
        }

        ppReferenceToCurrentInfo = &pCurrentImportInfo->m_Next;
    }
    return RetVal;

}

void CompilerProject::ProjectLevelImportsList::DeleteImport
(
    LPCWSTR ImportString
)
{
    // This is not very efficient, but not a common scenario. So no point
    // using up more space in the data structures to make this scenario
    // faster.
    ProjectLevelImportInfo **ppReferenceToCurrentInfo = &m_pProjectLevelImportInfos;

    // Delete the import info
    //
	ProjectLevelImportInfo *pCurrentImportInfo;
    for (pCurrentImportInfo = m_pProjectLevelImportInfos;
         pCurrentImportInfo;
         pCurrentImportInfo = pCurrentImportInfo->m_Next)
    {
        if (wcscmp(ImportString, pCurrentImportInfo->m_OriginalImportString) == 0)
        {
            // Remove the item from the list and free it
            *ppReferenceToCurrentInfo = pCurrentImportInfo->m_Next;
            VBFree(pCurrentImportInfo->m_OriginalImportString);
            VBFree(pCurrentImportInfo);
            break;
        }

        ppReferenceToCurrentInfo = &pCurrentImportInfo->m_Next;
    }

    VSASSERT(pCurrentImportInfo, "We did not find the import for the namespace!");
}

void CompilerProject::ProjectLevelImportsList::DeleteAllImports()
{
    ProjectLevelImportInfo *pCurrentImportInfo = m_pProjectLevelImportInfos;
    ProjectLevelImportInfo *pPrevImportInfo;

    // Delete all the imports infos

    while (pCurrentImportInfo)
    {
        pPrevImportInfo = pCurrentImportInfo;
        pCurrentImportInfo = pCurrentImportInfo->m_Next;

        VBFree(pPrevImportInfo->m_OriginalImportString);
        VBFree(pPrevImportInfo);
    }

    m_pProjectLevelImportInfos = NULL;

    // Delete all the import targets

    m_ImportsAllocator.FreeHeap();
    m_ImportsErrorLog.DeleteAll();

    m_pProjectLevelImportTargets = NULL;
}

bool CompilerProject::ProjectLevelImportsList::ErrorsInImports()
{
   return m_ImportsErrorLog.HasErrors() ||
          m_ImportsErrorLog.HasWarnings();
}

CompilerProject::ProjectLevelImportsList::~ProjectLevelImportsList()
{
    DeleteAllImports();
}

void CompilerProject::ProjectLevelImportsList::DumpImportsErrors
(
    ErrorTable *ErrorTableToDumpErrorsIn
)
{
    if (!ErrorsInImports())
    {
        return;
    }

    ErrorTableToDumpErrorsIn->MergeProjectLevelImportErrors(&m_ImportsErrorLog);
}

WCHAR *CompilerProject::ProjectLevelImportsList::GetOriginalImportStringByIndex(unsigned Index)
{
    unsigned CurrentIndex = 0;
    ProjectLevelImportInfo *pCurrentImportInfo;
    for (pCurrentImportInfo = m_pProjectLevelImportInfos;
         pCurrentImportInfo;
         pCurrentImportInfo = pCurrentImportInfo->m_Next)
    {
        if (CurrentIndex == Index)
        {
            break;
        }

        CurrentIndex++;
    }

    VSASSERT(pCurrentImportInfo, "Wrong imports Index!!!");

    return pCurrentImportInfo->m_OriginalImportString;
}

/* End - Implementation for ProjectLevelImportsList */

WARNING_LEVEL CompilerProject::GetWarningLevel
(
 ERRID errid
)
{
    if (GetDefaultWarningLevel() == WARN_None)
    {
        return WARN_None;
    }
    if (m_daWarningsLevelTable.Count())
    {
        // check the table of individual warning levels
        // 

        ULONG cWarnItems = m_daWarningsLevelTable.Count();
        for (ULONG i = 0; i < cWarnItems; i++)
        {
            if ((DWORD)errid == m_daWarningsLevelTable.Array()[i].dwWrnId)
            {
                return m_daWarningsLevelTable.Array()[i].WarningLevel;
            }
        }
    }
    return GetDefaultWarningLevel();
}

#ifdef EXTENDED_WARNING_GRANULARITY
void CompilerProject::CleanWarningIdAndStringTable()
{
    if (m_WarningIdAndStringTable)
    {
        UINT dim =  DIM(m_wrnIds);
        for (UINT i = 0 ; i < dim; i++)
        {
            if (m_WarningIdAndStringTable[i].wszDescr)
            {
                VBFree(m_pCompiler, (void*)m_WarningIdAndStringTable[i].wszDescr);
            }
        }
        VBFree(m_pCompiler, m_WarningIdAndStringTable);
        m_WarningIdAndStringTable = NULL;
    }
}
#endif

bool IsEqual
(
 DynamicArray<VbCompilerWarningItemLevel>* memberWarningsLevelTable,
 DWORD countWrnTable,
 VbCompilerWarningItemLevel optionWarningsLevelTable[]
)
{
    // compare two tables of individual warning levels.
    // Normally the tables should be in the same order. But this is to hard
    // to impose on the external callers, so the code to deal with different orderings is provided.
    // For now, the tables are small (10-20 items). O(n*n) compare is good enough. Consider to move to a
    // O(n*Log(n)) alghorithm based on a presorting when the tables get really big.
    if (countWrnTable != memberWarningsLevelTable->Count())
    {
        return false;
    }
    if (countWrnTable == 0)
    {
        return true;
    }
    bool isOptionMatch = false;
    for (DWORD i = 0; i < countWrnTable; i++)
    {
        // get member i of the first table
        isOptionMatch = false;
        //first try as the tables are in the same order (of dwWrnId)
        if (memberWarningsLevelTable->Array()[i].dwWrnId == optionWarningsLevelTable[i].dwWrnId)
        {
            if (memberWarningsLevelTable->Array()[i].WarningLevel == optionWarningsLevelTable[i].WarningLevel )
            {
                isOptionMatch = true;
                continue;
            }
            else
            {
                return false;
            }
        }

        // different order: Try all the members of the second table
        for (DWORD j = 0; j < countWrnTable; j++)
        {
            if (memberWarningsLevelTable->Array()[i].dwWrnId == optionWarningsLevelTable[j].dwWrnId)
            {
                if (memberWarningsLevelTable->Array()[i].WarningLevel == optionWarningsLevelTable[j].WarningLevel )
                {
                    isOptionMatch = true;
                    break;
                }
                else
                {
                    return false;
                }
            }
        }
        if (!isOptionMatch)
        {
            // this is kind of bad. In the current implementation the table of configurable warnings
            // is fixed. It is the same in command line and project. In a recompilation the same warnId's should be sent.
            // If we relax somehow this restriction then different tables as warnID's would make sense
            VSFAIL("Unexpected different warnings level tables as warnId's");
            return false;
        }
    }
    VSASSERT(isOptionMatch, "How warning options could be different here");
    return isOptionMatch;

}


bool CompilerProject::AddRequiredLibraries()
{
    bool added = false;
    unsigned index = 0;
    CompilerProject *pProject = NULL;

    pProject = GetCompilerHost()->GetComPlusProject();

    if (!FindReferenceIndex(pProject, &index))
    {
        added |= AddReference(pProject, REFERENCEFLAG_REQUIRED);
    }

    return added;
}

bool CompilerProject::AddStandardLibraries()
{
    //-- Get the /sdkpath from the compiler host
    STRING *path = NULL;
    HRESULT hr = GetCompilerHost()->GetSdkPath(&path);

    if (FAILED(hr))
    {
        m_ErrorTable.CreateErrorWithError(ERRID_CantFindCORSystemDirectory, NULL, hr);
        return false;
    }

    //
    // Search for and add each reference
    //
    bool        referenceAdded = false;
    ULONG       libs = GetCompilerHost()->GetStandardLibraries()->Count();
    STRING**    standardLibs = GetCompilerHost()->GetStandardLibraries()->Array();

    for (ULONG i = 0; i < libs; i++)
    {
        STRING* pstrFileName;

        if (!m_pCompiler->FindFileInPath(standardLibs[i], path, &pstrFileName))
        {
            m_ErrorTable.CreateError(WRNID_CannotFindStandardLibrary1, NULL, standardLibs[i]);
            continue;
        }

        bool added;

        if (FAILED(AddMetaDataReferenceInternal(pstrFileName, true, REFERENCEFLAG_STANDARD, &added)))
        {
            // error already reported in AddReference
            continue;
        }

        referenceAdded |= added;
    }

    return referenceAdded;
}

void CompilerProject::ClearFileCompilationArray()
{
    CheckInMainThread();

#if IDE 
    VSASSERT(GetCompilerSharedState()->IsBackgroundCompilationBlocked(), "Background Compilation should have been stopped");
#endif

    m_fIsCompilationArraySorted = false;
    m_daFileCompilationArray.Collapse();
}

DynamicArray<SourceFile *> *CompilerProject::GetFileCompilationArray()
{
    if (!m_fIsCompilationArraySorted)
    {
        SourceFile *pSourceFile = NULL;
        SourceFileIterator Files(this);

        while (pSourceFile = Files.Next())
        {
            m_daFileCompilationArray.AddElement(pSourceFile);
        }

        qsort(m_daFileCompilationArray.Array(), m_daFileCompilationArray.Count(), sizeof(SourceFile *), SortSourceFileByName);

        m_fIsCompilationArraySorted = true;
    }

    return &m_daFileCompilationArray;
}

#if IDE 
void CompilerProject::AddFileToTree
(
    CompilerFile *pFile
)
{
    AssertIfNull(pFile);

    STRING *strFileName = pFile->GetFileName();
    CompilerFileTreeNode *pCompilerFileTreeNode = NULL;

    if (m_treeFiles.Insert(&strFileName, &pCompilerFileTreeNode))
    {
        pCompilerFileTreeNode->pFile = pFile;
    }
    else
    {
        VSFAIL("The file already exists in the project");
    }
}

void CompilerProject::RemoveFileFromTree
(
    CompilerFile *pFile
)
{
    AssertIfNull(pFile);

    STRING *strFileName = pFile->GetFileName();

    m_treeFiles.Remove(&strFileName);
}

CompilerFile *CompilerProject::FindFileByName
(
    _In_opt_z_ STRING *strFileName
)
{
    if (strFileName)
    {
        CompilerFileTreeNode *pCompilerFileTreeNode = NULL;

        if (m_treeFiles.Find(&strFileName, &pCompilerFileTreeNode))
        {
            return pCompilerFileTreeNode->pFile;
        }
    }

    return NULL;
}

SourceFile *CompilerProject::FindSourceFileByName
(
    _In_opt_z_ STRING *strFileName
)
{
    CompilerFile *pCompilerFile = FindFileByName(strFileName);


    if (pCompilerFile && pCompilerFile->IsSourceFile())
    {
        return pCompilerFile->PSourceFile();
    }

    return NULL;
}

bool CompilerProject::RemoveStandardLibraries()
{
    CheckInMainThread();

    ULONG count = m_daReferences.Count();
    ReleaseReferences(&m_daReferences, true /*standardLibsOnly*/);
    return (count != m_daReferences.Count());
}


bool CompilerProject::HasDirectOrIndirectReferenceOnMe(CompilerProject *pReferencedProject)
{
    if( this->IsMetaData() || pReferencedProject->IsMetaData())
    {
        return false;
    }

    ReverseReferenceIterator RefIterator(this);
    CompilerProject *pProject = NULL;
    while (pProject = RefIterator.Next())
    {
        if( pProject == pReferencedProject ||
            pProject->HasDirectOrIndirectReferenceOnMe(pReferencedProject))
        {
            return true;
        }
    }
    return false;

}

#endif


#if !IDE
bool CompilerProject::AddExternalChecksum
(
    _In_z_ const WCHAR *ExternalChecksumFileName,
    _In_z_ const WCHAR *ExternalChecksumGuid,
    _In_z_ const WCHAR *ExternalChecksumVal,
    ERRID &error
)
{
    bool result = true;
    ExternalChecksum temp;
    temp.FileName = m_pCompiler->AddString(ExternalChecksumFileName);

    //parse guid

    size_t tmpLen = wcslen(ExternalChecksumGuid);

    WCHAR * pTmpStr = new (zeromemory) WCHAR[tmpLen+ 1];
    wcsncpy_s(pTmpStr, tmpLen + 1, ExternalChecksumGuid, tmpLen);
    pTmpStr[tmpLen] = 0;

    result = SUCCEEDED(IIDFromString(pTmpStr, &temp.Guid));

    delete [] pTmpStr;

    if (!result)
    {
        error = WRNID_BadGUIDFormatExtChecksum;
        return false;
    }

    // check the value is well formated
    DWORD checksumValLen = (DWORD)wcslen(ExternalChecksumVal);

    if ((checksumValLen & 0x01))
    {
        // odd number of hex digits
        error = WRNID_BadChecksumValExtChecksum;
        return false;
    }
    temp.cbChecksumData = checksumValLen / 2;
    for (DWORD k = 0; k < temp.cbChecksumData; k++)
    {
        WCHAR cHi = ExternalChecksumVal[2*k];
        WCHAR cLo = ExternalChecksumVal[2*k+1];
        if (!IsHexDigit(cHi) || !IsHexDigit(cLo))
        {
            error = WRNID_BadChecksumValExtChecksum;
            return false;

        }
    }

    // check for a previous entry for this file. Duplicates in the source files
    // are ok if they carry the same checksum info. Do not add duplicates in the list though..
    ULONG count = m_daExternalChecksum.Count();
    ExternalChecksum *extChecksumArray = m_daExternalChecksum.Array();
    bool fSeen = false;
    for (unsigned i = 0 ; i < count; i++)
    {
        if ( StringPool::IsEqual(extChecksumArray[i].FileName, temp.FileName))
        {
            fSeen = true;
            if (extChecksumArray[i].Guid != temp.Guid)
            {
                error = WRNID_MultipleDeclFileExtChecksum;
                return false;
            }
            // check the checksum values are matching.
            // avoid to allocate unnecesary memory: do an additional walk of the string before the proper conversion.
            if (extChecksumArray[i].cbChecksumData != temp.cbChecksumData)
            {
                // different sizes
                error = WRNID_MultipleDeclFileExtChecksum;
                return false;
            }

            // check the hex encoding matches the in memory checksum data
            for (DWORD k = 0; k < extChecksumArray[i].cbChecksumData; k++)
            {
                WCHAR cH = ExternalChecksumVal[2*k];
                WCHAR cL = ExternalChecksumVal[2*k+1];

                if ((BYTE)((IntegralLiteralCharacterValue(cH) << 4) | IntegralLiteralCharacterValue(cL)) !=
                      extChecksumArray[i].pbChecksumData[k])
                {
                    error = WRNID_MultipleDeclFileExtChecksum;
                    return false;
                }
            }
        }
    }

    if (!fSeen)
    {
        temp.pbChecksumData = (BYTE*)m_nraSymbols.Alloc(temp.cbChecksumData);
        for (DWORD k = 0; k < temp.cbChecksumData; k++)
        {
            WCHAR cH = ExternalChecksumVal[2*k];
            WCHAR cL = ExternalChecksumVal[2*k+1];
            temp.pbChecksumData[k] = (BYTE)((IntegralLiteralCharacterValue(cH) << 4) | IntegralLiteralCharacterValue(cL));
        }
        ExternalChecksum &newEntry = m_daExternalChecksum.Add();
        newEntry.FileName = temp.FileName;
        newEntry.Guid = temp.Guid;
        newEntry.cbChecksumData = temp.cbChecksumData;
        newEntry.pbChecksumData = temp.pbChecksumData;
    }
    return true;
}
#endif

#if IDE 

void CompilerProject::DumpTasklistForENCSuites()
{
    CComPtr<IVsShell> srpShell;
    CComVariant var;
    CRegKey hKeyRoot;
    CRegKey hKeyDebugger;
    if (SUCCEEDED(GetCompilerPackage()->GetSite()->QueryService(SID_SVsShell, IID_IVsShell, (void**)&srpShell)) &&
        SUCCEEDED(srpShell->GetProperty(VSSPROPID_VirtualRegistryRoot, &var)) &&
        var.vt == VT_BSTR && var.bstrVal != NULL &&
        hKeyRoot.Open(HKEY_CURRENT_USER, var.bstrVal, KEY_READ) == ERROR_SUCCESS && hKeyRoot &&
        hKeyDebugger.Open(hKeyRoot, L"Debugger", KEY_READ) == ERROR_SUCCESS && hKeyDebugger && hKeyDebugger)
    {
        DWORD dwValueType = 0;
        DWORD cbValueSize = 0;
        if (::RegQueryValueEx(hKeyDebugger, ENCDUMPTASKLISTKEY, NULL, &dwValueType, NULL, &cbValueSize) == ERROR_SUCCESS && dwValueType == REG_SZ)
        {
            StringBuffer sbLogFilePath;
            sbLogFilePath.AllocateSizeAndSetLength(cbValueSize);
            if (::RegQueryValueEx(hKeyDebugger, ENCDUMPTASKLISTKEY, NULL, &dwValueType, (LPBYTE)sbLogFilePath.GetString(), &cbValueSize) == ERROR_SUCCESS)
            {
                IDEHelpers::DumpTaskListToFile(this, sbLogFilePath.GetString(), ENCID_First, ENCID_Last);
            }
        }
    }
}

STRING *CompilerProject::GetAssemblyVersionFromDisk()
{
    if (!m_pstrPEName)
    {
        return NULL;
    }

    WIN32_FIND_DATA wfd;
    HANDLE hFindOutput = ::FindFirstFile(m_pstrPEName, &wfd);
    if (hFindOutput == INVALID_HANDLE_VALUE)
    {
        return NULL;
    }
    ::FindClose(hFindOutput);

    STRING *pstrAssemblyVersion = NULL;

    HRESULT  hr = NOERROR;
    IMetaDataAssemblyImport *pimdAssemblyImport = NULL;
    IMetaDataDispenserEx    *pimdDataDispEx = NULL;
    mdAssembly mdassembly = mdAssemblyNil;
    ASSEMBLYMETADATA amd = {0};

    IfFailGo(GetCompilerHost()->OpenScope(&pimdDataDispEx, m_pstrPEName, ofReadOnly, IID_IMetaDataAssemblyImport, (IUnknown **)&pimdAssemblyImport));
    IfFailGo(pimdAssemblyImport->GetAssemblyFromScope(&mdassembly));
    IfFailGo(pimdAssemblyImport->GetAssemblyProps(mdassembly,   // [IN] The Assembly for which to get the properties.
                                                    NULL,       // [OUT] Pointer to the Originator blob.
                                                    NULL,       // [OUT] Count of bytes in the Originator Blob.
                                                    NULL,       // [OUT] Hash Algorithm.
                                                    NULL,       // [OUT] Buffer to fill with name.
                                                    0,          // [IN] Size of buffer in wide chars.
                                                    NULL,       // [OUT] Actual # of wide chars in name.
                                                    &amd,       // [OUT] Assembly MetaData.
                                                    NULL));     // [OUT] Flags.
    WCHAR wszVersionString[24];
    StringCchPrintf(wszVersionString, 24, L"%hu.%hu.%hu.%hu", amd.usMajorVersion, amd.usMinorVersion, amd.usBuildNumber, amd.usRevisionNumber);
    pstrAssemblyVersion = m_pCompiler->AddString(wszVersionString);

Error:

    RELEASE(pimdAssemblyImport);
    RELEASE(pimdDataDispEx);

    return pstrAssemblyVersion;
}

//
// ;EnsureVBReferenceChangedService
// Attempt to initialize m_srpVBReferenceChangedService if needed.
//
void CompilerProject::EnsureVBReferenceChangedService()
{
    if (!m_isExpressionEditorProject && !m_fVBReferenceChangedServiceQueried)
    {
        if (GetCompilerPackage() && GetCompilerPackage()->GetSite())
        {
            // Query service to get the VBReferenceChangedService.
            HRESULT hr = GetCompilerPackage()->GetSite()->QueryService(
                SID_SVBReferenceChangedService,
                IID_IVBReferenceChangedService,
                (void**)&m_srpVBReferenceChangedService);
        }
        m_fVBReferenceChangedServiceQueried = true;
    }
}
#endif IDE

// Is the passed type cached in the PIA type ref cache.
bool CompilerProject::IsCachedPiaTypeRef(BCSYM* pType)
{
    AssertIfNull(pType);

    if (pType->IsClass() && !pType->IsDelegate() && !pType->IsInterface() && !pType->IsStruct() && !pType->IsEnum())
    {
        // classes are not no-pia types
        return false;
    }

    if (CS_TypesEmitted <= m_cs)
    {
        return m_DeclarationPiaTypeRefCache.Contains(pType) || m_MethodBodyPiaTypeRefCache.Contains(pType);
    }
    else
    {
        AssertIfFalse(m_MethodBodyPiaTypeRefCache.Count() == 0);
        return m_DeclarationPiaTypeRefCache.Contains(pType);
    }
}

BCSYM_Interface *CompilerProject::LookupComEventSourceInterface(BCSYM_Container *pType, bool suppressErrors, Location *pReferencingLocation, ErrorTable *pErrorTable)
{
    AssertIfNull(pType);
    WellKnownAttrVals::ComEventInterfaceData ceid;
    if (pType->GetPWellKnownAttrVals()->GetComEventInterfaceData(&ceid))
    {
        bool NameIsBad = false;
        Type *SourceInterface = NULL;

        if (ceid.m_SourceInterface != NULL)
        {
            SourceInterface = CompilerHost::GetSymbolForQualifiedName(
                ceid.m_SourceInterface,
                pType->GetContainingProject(),
                m_pCompiler,
                GetCompilerHost(),
                NULL);
        }
        
        // In order to qualify, we must have located the type in the target scope, and it must be an interface.
        if (SourceInterface != NULL && !SourceInterface->IsBad() && SourceInterface->IsNamedRoot() &&
            SourceInterface->PNamedRoot()->GetContainingProject() == pType->GetContainingProject())
        {
            if (SourceInterface->IsInterface())
            {
                AssertIfFalse(TypeHelpers::IsEmbeddableInteropType(SourceInterface));
                return SourceInterface->PInterface();
            }
            else if (!suppressErrors)
            {
                pErrorTable->CreateError(
                    ERRID_SourceInterfaceMustBeInterface,
                    pReferencingLocation,
                    pType->GetQualifiedName(true, NULL, true),
                    ceid.m_SourceInterface);
            }
        }
        else if (!suppressErrors)
        {
            pErrorTable->CreateError(
                ERRID_SourceInterfaceMustExist,
                pReferencingLocation,
                pType->GetQualifiedName(true, NULL, true),
                ceid.m_SourceInterface ?  ceid.m_SourceInterface : L"");
        }
    }
    // We will return above if the interface matches. Returning here means that either there was no source
    // interface, or it did not pass the test; in any case we will only return NULL or a valid sourceinterface.
    return NULL;
}


bool * CompilerProject::Get_pReferredToEmbeddableInteropType()
{
#if IDE 
    bool **ppBool = VBTLSThreadData::GetpReferredToEmbeddableInteropType();
    return *ppBool; 
#else
    return m_pReferredToEmbeddableInteropType; 
#endif
}

void CompilerProject::Set_pReferredToEmbeddableInteropType(bool * pValue)
{
#if IDE 
    bool ** ppBool = VBTLSThreadData::GetpReferredToEmbeddableInteropType();
    *ppBool = pValue;
#else
    m_pReferredToEmbeddableInteropType = pValue; 
#endif
}


#if IDE 

bool CompilerProject::GetTrackUnsubstitutedLocalTypes()
{
    bool * pBool = VBTLSThreadData::GetTrackUnsubstitutedLocalTypes();
    return *pBool;
}

void CompilerProject::SetTrackUnsubstitutedLocalTypes(bool value)
{
    bool * pBool = VBTLSThreadData::GetTrackUnsubstitutedLocalTypes();
    *pBool = value;
}
#endif



void CompilerProject::CachePiaTypeRef(BCSYM_Container* pType, Location *pReferencingLocation, ErrorTable *pErrorTable)
{
    AssertIfNull(pType);
    AssertIfTrue(IsMetaData());
    AssertIfFalse(TypeHelpers::IsEmbeddableInteropType(pType));
    AssertIfNull(pErrorTable);
    bool added = false;

    if (pType->IsClass() && !pType->IsDelegate() && !pType->IsInterface() && !pType->IsStruct() && !pType->IsEnum())
    {
        pErrorTable->CreateError(
            ERRID_CannotLinkClassWithNoPIA1,
            pReferencingLocation,
            pType->GetName());
        return;
    }

    // We have created a local type reference for this symbol.
    // If we are going to Declared state, we should store the symbol in the declaration
    // typeref cache (and the methodbody cache should be empty).
    // If we are going to Compiled state, we should check to make sure the symbol is not
    // already present in the declaration cache, and store the symbol in the method body cache.
    if (CS_TypesEmitted <= m_cs)
    {
        if (!m_DeclarationPiaTypeRefCache.Contains(pType))
        {
            if (!m_MethodBodyPiaTypeRefCache.Contains(pType))
            {
                m_MethodBodyPiaTypeRefCache.Add(pType);
                added = true;
            }
        }
    }
    else
    {
        if (!m_DeclarationPiaTypeRefCache.Contains(pType))
        {
            IfFalseThrow(m_MethodBodyPiaTypeRefCache.Count() == 0);
            m_DeclarationPiaTypeRefCache.Add(pType);
            added = true;
        }
    }
    
    // We should only track dependencies the first time we encounter a type. Our dependency links
    // are bidirectional, so we would recurse forever if we did not return here. Besides,
    // there's no reason to duplicate work we have already completed.
    if (!added) 
    {
        return;
    }

    if (IsBad(pType))
    {
        Semantics::ReportBadType(pType, *pReferencingLocation, m_pCompiler, pErrorTable);
    }
    
    // We do not support nesting embedded types.
    // Dev10 #780182 Make sure we catch types nested into classes and interfaces.
    if (pType->GetContainingClassOrInterface())
    {
        pErrorTable->CreateError(
            ERRID_InvalidInteropType,
            pReferencingLocation,
            pType->GetQualifiedName(true, NULL, true));
    }

    // MetaImport has some kind of demand-loading scheme for type members. We must
    // ensure that members get loaded for No-PIA types that have been referenced,
    // because we may need to emit these members even if the members themselves have
    // not been used: I'm not sure why this is a good idea, but the spec requires it.
    pType->PContainer()->EnsureChildrenLoaded();
    
    if (pType->IsInterface())
    {
        if (pType->PInterface()->IsGeneric())
        {
            pErrorTable->CreateError(
                ERRID_CannotEmbedInterfaceWithGeneric,
                pReferencingLocation,
                pType->GetName());            
        }
        else
        {
            BCITER_ImplInterfaces implList(pType);
            while(BCSYM_Implements *----l = implList.GetNext())
            {
                BCSYM *implSym = ----l->GetCompilerRoot();
                if (TypeHelpers::IsEmbeddableInteropType(implSym))
                {
                    CachePiaTypeRef(implSym->PContainer(), pReferencingLocation, pErrorTable);
                } 
                else if (implSym != NULL && IsBad(implSym))
                {
                    Semantics::ReportBadType(implSym, *pReferencingLocation, m_pCompiler, pErrorTable);
                }
            }
        }
    }
    else
    {
        // If the type is not an interface, we will include all of its members, whether they are referenced 
        // or not. 
        int errorCount = 0;
        DefineableMembersInAContainer memIter(m_pCompiler, pType);
        while (BCSYM *Symbol = memIter.Next())
        {
            if (Symbol->IsNamedRoot())
            {
                CachePiaTypeMemberRef(Symbol->PNamedRoot(), pReferencingLocation, pErrorTable, errorCount);
            }
        }
    }
    
    // If this type has a ComEventInterface attribute, we must go find its associated source interface and
    // make sure we embed it as well. We can't assume there will be a direct reference to this type, but
    // must include it so it can be found at runtime.
    BCSYM_Interface *ComSourceInterface = LookupComEventSourceInterface(pType, false, pReferencingLocation, pErrorTable);
    if (ComSourceInterface)
    {
        CachePiaTypeRef(ComSourceInterface, pReferencingLocation, pErrorTable);
    }

    WCHAR *pwszDefaultMember;
    if ( (pType->IsInterface() || pType->IsStruct()) &&
          pType->GetPWellKnownAttrVals()->GetDefaultMemberData(&pwszDefaultMember) &&
          pwszDefaultMember != NULL)
    {
        int errorCount = 0;
        for ( BCSYM_NamedRoot* Symbol = pType->GetHash()->SimpleBind(m_pCompiler->AddString(pwszDefaultMember));
             Symbol != NULL;
             Symbol = Symbol->GetNextOfSameName())
        {
            CachePiaTypeMemberRef(Symbol, pReferencingLocation, pErrorTable, errorCount);
        }
    }
}

// Increments the error count for each error that's found.  Only displays a message for the first such error.
void CompilerProject::CachePiaTypeMemberRef(BCSYM_NamedRoot *psymNamed, Location *pReferencingLocation, ErrorTable *pErrorTable, int & errorCount)
{
    AssertIfNull(psymNamed);
    AssertIfTrue(IsMetaData()); 
    AssertIfNull(pErrorTable);
    
    // Make sure the item can be embedded at all: we don't allow types or anything that cannot be a class member.
    // Filtering these out here saves us a lot of grief later on down the pipeline. Dev10 bug #676896.
    if (psymNamed->IsClass() || !psymNamed->IsMember())
    {
        if (errorCount == 0)
        {
            pErrorTable->CreateError(
                ERRID_InvalidInteropType,
                pReferencingLocation,
                psymNamed->GetContainer()->GetQualifiedName(true, NULL, true));
        }
        errorCount++;
        return;
    }

    if (IdentifyPiaTypeMember(psymNamed))
    {
        return;
    }

    bool chkMember = psymNamed->GetContainer()->IsStruct() && !psymNamed->GetContainer()->IsEnum();
#if IDE 
    // When building debug versions in the IDE, the compiler will insert some extra members
    // that support ENC. These make no sense in local types, so we will skip them. We have to
    // check for them explicitly or they will trip the member-validity check that follows.
    if (psymNamed->IsSyntheticMethod() || (psymNamed->IsVariable() && psymNamed->PVariable()->IsENCTrackingList()))
    {
        chkMember = false;
    }
#endif
    if (chkMember && (!psymNamed->IsVariable() || psymNamed->PVariable()->IsShared() || psymNamed->PVariable()->GetAccess() != ACCESS_Public))
    {
        // Structures may contain only public instance fields. They may not have shared fields,
        // because the whole point of No-PIA is that we will not have a reference to the
        // assembly containing the field. (This was related to Dev10 bug #540028.) Nor may they
        // have methods, events, properties, or any type of member but a field.
        if (errorCount == 0)
        {
            pErrorTable->CreateError(
                ERRID_InvalidStructMemberNoPIA1,
                pReferencingLocation,
                psymNamed->GetContainer()->GetQualifiedName(true, NULL, true));
        }
        errorCount++;
    }

    // Add the member ref to the appropriate hash set.
    if (CS_TypesEmitted <= m_cs)
    {
        m_MethodBodyPiaMemberCache.Add(psymNamed);
    }
    else
    {
        m_DeclarationPiaMemberCache.Add(psymNamed);
    }

    if (IsBad(psymNamed))
    {
        Semantics::ReportBadDeclaration(psymNamed, *pReferencingLocation, m_pCompiler, pErrorTable);
    }

    // It is possible that we have found a reference to a member before
    // encountering a reference to its container; make sure the container gets included.
    CachePiaTypeRef(psymNamed->GetContainer(), pReferencingLocation, pErrorTable);

    // All members may have a type. For fields, this is simply the field type; for procs,
    // this is the return type.
    if (psymNamed->IsMember())
    {
        // Dev10 817262 - if we have a BCSYM_Pointer, BCSYM_Array, or BCSYM_NamedType (say we are looking at an embedded type), we need
        // to get at the actual type of the pointer/array/namedtype so that we can cache it
        BCSYM_Member *member = psymNamed->PMember();
        BCSYM *pMemberType = member->GetType()->ChaseToNamedType()->DigThroughNamedType();
 
        if (TypeHelpers::IsEmbeddableInteropType(pMemberType))
        {
            CachePiaTypeRef(pMemberType->PContainer(), pReferencingLocation, pErrorTable);
        }
        else if (pMemberType != NULL && IsBad(pMemberType))
        {
            Semantics::ReportBadType(pMemberType, *pReferencingLocation, m_pCompiler, pErrorTable);
        }
    }

    if (psymNamed->IsProc())
    {
        // Procs may also have parameters. This includes methods, properties, and property accessors.
        BCSYM_Proc *proc = psymNamed->PProc();
        for (BCSYM_Param *scanParam = proc->GetFirstParam(); scanParam; scanParam = scanParam->GetNext())
        {
            // Dev10 817262 - if we have a BCSYM_Pointer, BCSYM_Array, or BCSYM_NamedType (say we are looking at an embedded type), we need
            // to get at the actual type of the pointer/array/namedtype so that we can cache it
            BCSYM *pParamType = scanParam->GetType()->ChaseToNamedType()->DigThroughNamedType();
            
            if (TypeHelpers::IsEmbeddableInteropType(pParamType))
            {
                BCSYM_Container* paramType = pParamType->PContainer();
                CachePiaTypeRef(paramType, pReferencingLocation, pErrorTable);
            }
            else if (pParamType != NULL && IsBad(pParamType))
            {
                Semantics::ReportBadType(pParamType, *pReferencingLocation, m_pCompiler, pErrorTable);
            }
        }

        // If this proc happens to belong to a property, we should include the property as well.
        if (proc->IsPropertyGet() || proc->IsPropertySet())
        {
            BCSYM_Property *prop = proc->GetAssociatedPropertyDef();
            AssertIfNull(prop);
            AssertIfFalse(TypeHelpers::IsEmbeddableInteropType(prop->GetParent()));
            CachePiaTypeMemberRef(prop, pReferencingLocation, pErrorTable, errorCount);
        }
        
        if (proc->IsProperty())
        {
            BCSYM_Property *prop = proc->PProperty();
            // We should generate both getter and setter for this property.
            // Otherwise, we could generate a property which contained a reference 
            // to a member that was never otherwise emitted.
            if (prop->GetProperty())
            {
                CachePiaTypeMemberRef(prop->GetProperty(), pReferencingLocation, pErrorTable, errorCount);
            }
            if (prop->SetProperty())
            {
                CachePiaTypeMemberRef(prop->SetProperty(), pReferencingLocation, pErrorTable, errorCount);
            }
        }
        
        if (proc->CreatedByEventDecl())
        {
            BCSYM_EventDecl *pED = proc->CreatedByEventDecl();
            AssertIfNull( pED );
            CachePiaTypeMemberRef(pED, pReferencingLocation, pErrorTable, errorCount);
        }
    }

    if (psymNamed->IsProperty())
    {
        BCSYM_Property *pProp = psymNamed->PProperty();

        CachePiaTypeMemberRef(pProp, pReferencingLocation, pErrorTable, errorCount);
        if (pProp->GetProperty())
        {
             CachePiaTypeMemberRef(pProp->GetProperty(), pReferencingLocation, pErrorTable, errorCount);
        } 
        if (pProp->SetProperty())
        {
             CachePiaTypeMemberRef(pProp->SetProperty(), pReferencingLocation, pErrorTable, errorCount);
        }
    }
    
    if (psymNamed->IsEventDecl())
    {
        BCSYM_EventDecl *pED = psymNamed->PEventDecl();
        BCSYM *pEventDelegate = pED->GetDelegate();
        if (pEventDelegate && TypeHelpers::IsEmbeddableInteropType(pEventDelegate))
        {
            CachePiaTypeRef(pEventDelegate->PContainer(), pReferencingLocation, pErrorTable);
        }
        if (pED->GetProcAdd())
        {
            CachePiaTypeMemberRef(pED->GetProcAdd(), pReferencingLocation, pErrorTable, errorCount);
        }
        if (pED->GetProcRemove())
        {
            CachePiaTypeMemberRef(pED->GetProcRemove(), pReferencingLocation, pErrorTable, errorCount);
        }
        if (pED->GetProcFire())
        {
            CachePiaTypeMemberRef(pED->GetProcFire(), pReferencingLocation, pErrorTable, errorCount);
        }
    
        // If the event happens to belong to a class with a ComEventInterfaceAttribute, there will also be
        // a paired method living on its source interface. The ComAwareEventInfo class expects to find this 
        // method through reflection. If we embed an event, therefore, we must ensure that the associated source
        // interface method is also included, even if it is not otherwise referenced in the embedding project.
        
        BCSYM_Interface *SourceInterface = LookupComEventSourceInterface(psymNamed->GetContainer(), true, pReferencingLocation, pErrorTable);
        if (SourceInterface)
        {
            bool NameIsBad = false;
            BCSYM_NamedRoot *backingMember = SourceInterface->GetHash()->SimpleBind(psymNamed->GetName());
            if (backingMember && backingMember->IsMethodDecl())
            {
                CachePiaTypeMemberRef(backingMember, pReferencingLocation, pErrorTable, errorCount);
            }
            else
            {
                pErrorTable->CreateError(
                    ERRID_EventNoPIANoBackingMember,
                    pReferencingLocation,
                    SourceInterface->GetQualifiedName(true, NULL, true),
                    psymNamed->GetName(),
                    psymNamed->GetQualifiedName());
            }
        }
    }
}

unsigned int CompilerProject::CountPiaCacheEntries(void)
{
    return m_MethodBodyPiaMemberCache.Count() +
        m_DeclarationPiaMemberCache.Count() +
        m_MethodBodyPiaTypeRefCache.Count() +
        m_DeclarationPiaTypeRefCache.Count();
}

// Get amount of cached types.
unsigned int CompilerProject::CountPiaTypeCacheEntries(void)
{
    return m_MethodBodyPiaTypeRefCache.Count() +
        m_DeclarationPiaTypeRefCache.Count();
}


PiaTypeIterator CompilerProject::GetPiaTypeIterator()
{
    HashSetIterator<BCSYM*> decls(&m_DeclarationPiaTypeRefCache);
    HashSetIterator<BCSYM*> methods(&m_MethodBodyPiaTypeRefCache);
    PiaTypeIterator out(decls, methods);
    return out;
}

bool CompilerProject::IdentifyPiaTypeMember(BCSYM* psymMember)
{
    // Is there a reference to this member?
    if (m_MethodBodyPiaMemberCache.Contains(psymMember))
    {
        return true;
    }
    return m_DeclarationPiaMemberCache.Contains(psymMember);
}

bool PiaTypeIterator::MoveNext()
{
    if (m_DeclarationTypeList.MoveNext())
    {
        return true;
    } 
    else
    {
        useMethodBodyList = true;
        return m_MethodBodyTypeList.MoveNext();
    }
}

BCSYM* PiaTypeIterator::Current()
{
    if (useMethodBodyList)
    {
        return m_MethodBodyTypeList.Current();
    }
    else
    {
        return m_DeclarationTypeList.Current();
    }

}

// Dev10 #735384
// Used to determine if we should do some validation for embedded types 
// to implement "pay for play" approach. 
// The answer is True if there is a chance that we can run into an embedded type.
bool CompilerProject::NeedToCheckUseOfEmbeddedTypes()
{
    if (PotentiallyEmbedsPiaTypes())
    {
        return true;
    }

    unsigned count = m_daReferences.Count();
    for( unsigned i=0; i < count; i++)
    {
        CompilerProject * pReferencedProject = m_daReferences.Element(i).m_pCompilerProject;
        if (pReferencedProject != NULL && pReferencedProject->PotentiallyEmbedsPiaTypes())
        {
            return true;
        }
    }

    return false;
}

bool CompilerProject::IsDefaultVBRuntime()
{
    return this && m_pCompilerHost->GetDefaultVBRuntimeProject() == this;
}
