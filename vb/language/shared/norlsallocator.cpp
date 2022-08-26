//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implements No Release Allocator.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"


/*
 * We have special allocation rules in the compiler, where everything
 * should be allocated out of a heap associated with the compiler
 * instance we are in. Prevent people from using new/delete or
 * malloc/free.
 *<EMAIL>
 *************** REMOVED 7/6/99 [randyki] *****************</EMAIL>
 *
 * This is a great idea, but in fact the above rules are not 100% true.
 * There are some allocations which "outlive" compiler instances (the
 * compiler has been made multi-instance since the rules were made)
 * such as allocations from the name table, and the name table itself
 * (among other COM objects whose references are provided to parties
 * outside the compiler, such as source modules, source data objects,
 * etc.)
 *
 * These allocations are now made from the module's default heap, using
 * VSMEM's allocation tracking mechanism for standard leak detection,
 * etc.
 */
 

NorlsAllocator::NorlsAllocator( 
#if NRLSTRACK
     _In_ WCHAR *szFile
    , _In_ long nLineNo
#else
    _In_ int nUnused 

#endif NRLSTRACK
) :
    m_heapPage(g_pvbNorlsManager->GetPageHeap()),
    m_depth(0)
{
#if NRLSTRACK
    m_szFile = PathFindFileName(szFile);
    m_nLineNo = nLineNo;
    static long g_nSerialNo = 0;
    m_dwCreatedThreadId = GetCurrentThreadId();
    m_nSerialNo = g_NrlsAllocTracker->AddAllocator(this);
    m_nSeqNo= 0;
    m_nTotalAllocated = 0;
    m_dwAllocThreadId = 0;
#if NRLSTRACK_GETSTACKS    
    m_pLastBlock = NULL;
#endif NRLSTRACK_GETSTACKS    
#endif NRLSTRACK
    
    Init(ProtectedEntityFlags::Other);
}

NorlsAllocator::NorlsAllocator( 
#if NRLSTRACK
     _In_ WCHAR *szFile, _In_ long nLineNo,
#else
    _In_ int nUnused,
#endif NRLSTRACK
    _In_ PageHeap& heapToAllocateFrom
) :
    m_heapPage(heapToAllocateFrom),
    m_depth(0)
{
#if NRLSTRACK
    m_szFile = PathFindFileName(szFile);
    m_nLineNo = nLineNo;
    static long g_nSerialNo = 0;
    m_dwCreatedThreadId = GetCurrentThreadId();
    m_nSerialNo = g_NrlsAllocTracker->AddAllocator(this);
    m_nTotalAllocated = 0;
    m_dwAllocThreadId = 0;
#if NRLSTRACK_GETSTACKS    
    m_pLastBlock = NULL;
#endif NRLSTRACK_GETSTACKS    
#endif NRLSTRACK
    Init(ProtectedEntityFlags::Other);
}

/*
NorlsAllocator::NorlsAllocator(PageHeap& pageHeap, const WCHAR* heapName) : m_heapPage(pageHeap)
{
    Init(heapName, ProtectedEntityFlags::Other);
}

NorlsAllocator::NorlsAllocator(PageHeap& pageHeap, const WCHAR* heapName, ProtectedEntityFlagsEnum entity) : m_heapPage(pageHeap)
{
    Init(heapName, entity);
}
*/

void NorlsAllocator::Init(ProtectedEntityFlagsEnum entity)
{
    nextFree = limitFree = NULL;
    pageList = pageLast = NULL;
    allowReadOnlyDirectives = false;
    anyPageMarkedReadOnly = false;
    inAllowingWrite = false;
    this->entity = entity;
}

NorlsAllocator::~NorlsAllocator()
{
#if NRLSTRACK
    g_NrlsAllocTracker->RemoveAllocator(this);
#if NRLSTRACK_GETSTACKS
    if (g_vbCommonHeap != TrackingHeapToUse ())
    {
//        VsDebHeapDestroy(TrackingHeapToUse (), true);
    }
        
#endif NRLSTRACK_GETSTACKS

#endif NRLSTRACK
    FreeHeap();
}

/*
 * Allocate (duplicate) a wide char string.
 */
PWSTR NorlsAllocator::AllocStr(PCWSTR str)
{
    if (str == NULL)
        return NULL;

    size_t str_len = wcslen(str);

    // integer overflow
    if (!(str_len < str_len + 1))
    {
        VSASSERT(str_len < str_len + 1, "Invalid");
        return NULL;
    }

    PWSTR strNew = (PWSTR) Alloc((str_len + 1) * sizeof(WCHAR));
    HRESULT hr;
    hr = StringCchCopyW (strNew, str_len + 1, str);
    VSASSERT (SUCCEEDED (hr), "Invalid");
    return strNew;
}


void NorlsAllocator::ForbidWriteInternal(void * p, size_t sz)
{
    VSASSERT (IsAddressInHeap (p), "Invalid");
    PageProtect::ForbidWrite (entity, p, sz);
}

void NorlsAllocator::AllowWriteInternal(void * p, size_t sz)
{
    VSASSERT (IsAddressInHeap (p), "Invalid");
    PageProtect::AllowWrite (entity, p, sz);
}

void NorlsAllocator::SetPageWriteStatus(NorlsPage * page, bool writeable)
{
    SetPageRegionWriteStatus(page, page, writeable);
}

void NorlsAllocator::SetPageRegionWriteStatus(NorlsPage * first, NorlsPage * last, bool writeable)
{
    size_t end = last->limitAvail - ((BYTE*)first);
    if (writeable)
    {
        AllowWriteInternal(first, end);
    }
    else
    {
        anyPageMarkedReadOnly = true;
        ForbidWriteInternal(first, end);
    }
}

bool NorlsAllocator::IsAddressInCurrentPage(const void * addr) const
{
    if (!pageList)
    {
        return false;
    }

    return (addr >= pageLast->firstAvail && addr < pageLast->limitAvail);
}

void NorlsAllocator::SetAllPagesWriteStatus(bool writeable)
{
    if (!PageProtect::IsEntityProtected(this->entity))
    {
        // if the entity type for this heap is not protected,
        // short-circuit before iterating pages
        return ;
    }

    for (NorlsPage * first = pageList; first; first = first->next)
    {
        NorlsPage * end = first;
        PageHeap::PageArena * arena = NULL;
        for (NorlsPage * last = end->next; last; last = last->next)
        {
            if (end->limitAvail == (BYTE*)last)
            {
                if (arena == NULL)
                {
                    arena = m_heapPage.FindArena(first);
                }
                if (arena->OwnsPage(last))
                {
                    end = last;
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
        SetPageRegionWriteStatus(first, end, writeable);
        first = end;
    }
}

void NorlsAllocator::MakeCurrentPageReadOnlyInternal()
{
    if (pageLast && allowReadOnlyDirectives)
    {
        SetPageWriteStatus(pageLast, false);
    }
}


void NorlsAllocator::MakeCurrentPageWriteableInternal()
{
    if (anyPageMarkedReadOnly && pageLast)
    {
        SetPageWriteStatus(pageLast, true);
    }
}


void NorlsAllocator::MakeAllHeapWriteableInternal()
{
    SetAllPagesWriteStatus(true);
    anyPageMarkedReadOnly = false;
}


void NorlsAllocator::MakeAllHeapReadOnlyInternal()
{
    if (!allowReadOnlyDirectives)
        return ;
    SetAllPagesWriteStatus(false);
}

void* NorlsAllocator::AllocNonZero(size_t sz)
{
    size_t roundedUpSize = sz;
    void* result = _AllocNonZero(roundedUpSize);
#if DEBUG
    if (roundedUpSize > sz)
    {
        memset(reinterpret_cast<BYTE*>(result) + sz, 0, roundedUpSize - sz);
    }
#endif

    return result;
}

#if NRLSTRACK_GETSTACKS    
HANDLE NorlsAllocator::g_nraSymbolHeap = NULL;

HANDLE NorlsAllocator::TrackingHeapToUse()
{
    HANDLE heapToUse = g_vbCommonHeap;
    // to track an NRLS allocator using a different heap, do this:
    if (m_nLineNo == 31 && m_szFile[0]=='c') //compilerfile.cpp(31)
    {
        if (g_nraSymbolHeap == NULL)    // create heap 1st time through
        {
            g_nraSymbolHeap = VsDebHeapCreate(NULL, "nraSymbols");
        }
        heapToUse= g_nraSymbolHeap;
    }
    return heapToUse;
}

#endif NRLSTRACK_GETSTACKS    

void* NorlsAllocator::_AllocNonZero(size_t& sz)
{
    AssertIfFalse((size_t)nextFree % sizeof(void*) == 0);
    AssertIfFalse((size_t)limitFree % sizeof(void*) == 0);

    void * p = nextFree;
    if (sz == 0 && p == NULL)
    {
        sz = 1;
    }

    size_t roundSize = VBMath::RoundUpAllocSize(sz);
    size_t available = limitFree - nextFree;
   
    if ( available < roundSize )
    {
        AllocNewPage(sz);
    }

    p = nextFree;
    nextFree += roundSize; // this shouldn't overflow because we got the memory for the request (we would have thrown in AllocNewPage() otherwise)
    
    MakeCurrentPageWriteableInternal();

    AssertIfFalse((size_t)nextFree % sizeof(void*) == 0);
    AssertIfFalse((size_t)limitFree % sizeof(void*) == 0);
    AssertIfFalse((size_t)p % sizeof(void*) == 0);

#if NRLSTRACK
    m_nTotalAllocated += roundSize;
    m_dwAllocThreadId  = 0;
#endif NRLSTRACK
    
    sz = roundSize;
#if NRLSTRACK
    VSASSERT(m_dwAllocThreadId == 0," NorlsAlloc: only 1 thread allowed at a time");
    m_dwAllocThreadId = GetCurrentThreadId();
    m_nSeqNo+=1;


#if NRLSTRACK_GETSTACKS    
    // 
    NrlsHeapData  *pdata = 
        (NrlsHeapData *) VsDebugAllocInternal(TrackingHeapToUse(), 
                    HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS, 
                    (sizeof (NrlsHeapData )), __FILE__, __LINE__, INSTANCE_GLOBAL, NULL);
                    
    pdata->m_ptrNextBlock = m_pLastBlock; // linked list of allocations: point to prior one
    pdata->m_size = sz;  // record the size
    pdata->m_data = p;  // record the actual allocation
    m_pLastBlock = pdata; // track the most recent one
#endif NRLSTRACK_GETSTACKS    
#endif NRLSTRACK
    return p;
}

/*
 * Allocate a new block of memory. This should be VERY FAST and VERY SMALL. 
 *
 * Class and Function Invariants:
 *  1) NorlsAllocator::nextFree is pointer aligned 
 *  2) Memory pointers returned from any allocation method will be pointer aligned
 *  3) Memory pointers returned from any allocation method will be zero'd to size of the 
 *     allocation rounded up to a pointer multiple
 */

void* NorlsAllocator::Alloc(size_t sz)
{
    void* buf = AllocNonZero(sz); //sz may be changed
    return memset(buf, 0, sz);
}


//-------------------------------------------------------------------------------------------------
//
// Attempt to resize the allocation point passed in
//
// Pointer returned will meet invariantes described in ::Alloc's comments
//
//-------------------------------------------------------------------------------------------------
void *
NorlsAllocator::Resize(
    _In_bytecount_(cbSizeOld) void* pv,
    _In_ size_t cbSizeOld,
    _In_ size_t cbSizeNew)
{
    AssertIfFalse((size_t)nextFree % sizeof(void*) == 0);
    AssertIfFalse((size_t)limitFree % sizeof(void*) == 0);
    AssertIfFalse((size_t)pv % sizeof(void*) == 0);

    cbSizeNew = VBMath::RoundUpAllocSize(cbSizeNew);

#if NRLSTRACK
    m_nTotalAllocated += cbSizeNew - cbSizeOld;
#endif NRLSTRACK

    // With a 0 length allocation just return a new allocation.  Nothing to resize
    if ( 0 == cbSizeOld  )
    {
        return Alloc(cbSizeNew);
    }

    // This method will only rellocate if the last allocation occurred in the current
    // page and was the last allocation. 
    BYTE* pCheck = (BYTE*)(((size_t)pv) + VBMath::RoundUpAllocSize(cbSizeOld));
    if ( IsAddressInCurrentPage(pv) && pCheck == nextFree )
    {
        // For a grow scenario, make sure that the new size is within the current page
        if ( cbSizeNew > cbSizeOld )
        {
            BYTE* pEnd = (BYTE*)pv + VBMath::RoundUpAllocSize(cbSizeNew);
            if ( pEnd < limitFree )
            {
#if DEBUG
                // Check to make sure someone didn't overrun a non-word aligned allocation
                BYTE* pCheckStart = reinterpret_cast<BYTE*>(pv)+cbSizeOld;
                while (pCheckStart < pCheck ) 
                {
                    VSASSERT(0 == *pCheckStart, "Buffer overrun");
                    pCheckStart++;
                }
#endif

                size_t diff = (pEnd-nextFree);
                memset(nextFree, 0, diff);
                nextFree = pEnd;

                AssertIfFalse((size_t)nextFree % sizeof(void*) == 0);
                AssertIfFalse((size_t)limitFree % sizeof(void*) == 0);
                AssertIfFalse((size_t)pv % sizeof(void*) == 0);
                return pv;
            }
        }
        else
        {
            nextFree = (BYTE*)pv + VBMath::RoundUpAllocSize(cbSizeNew);

            AssertIfFalse((size_t)nextFree % sizeof(void*) == 0);
            AssertIfFalse((size_t)limitFree % sizeof(void*) == 0);
            AssertIfFalse((size_t)pv % sizeof(void*) == 0);
            return pv;
        }
    }

    // Realloc is not possible.  Simply do an allocation
    void *pNew = Alloc(cbSizeNew);
    memcpy(pNew, pv, cbSizeOld);
    return pNew;
}


/*
 * An allocation request has overflowed the current page.
 * allocate a new page and try again.
 */
void NorlsAllocator::AllocNewPage(size_t sz)
{
    sz = VBMath::RoundUpAllocSize(sz);   // round to appropriate byte boundary.
    NorlsPage * newPage = NewPage(sz);
    VSASSERT(newPage->next == NULL, "Invalid");

    // Set all the memory in this page as available for alloc.
    nextFree = newPage->firstAvail;
    limitFree = newPage->limitAvail;

    // Link the page in to the list.
    if (pageLast)
    {
        MakeCurrentPageWriteableInternal();
        pageLast->next = newPage;
        MakeCurrentPageReadOnlyInternal();
        pageLast = newPage;
    }
    else
    {
        // First page.
        pageList = pageLast = newPage;
    }
}

/*
 * Allocate a new page of memory, with enough size
 * to handle a block of size sz.
 */
NorlsPage * NorlsAllocator::NewPage(size_t sz)
{
    size_t pageSize = PageHeap::pageSize; // Page size.
    size_t allocSize;
    NorlsPage * newPage;

    allocSize = VBMath::RoundUp(sz + sizeof(NorlsPage), pageSize);

    // Allocate the new page.
    newPage = (NorlsPage *) m_heapPage.AllocPages(allocSize);

#if DEBUG
    // Add buffer overflow detection
    newPage->sentinalStart = DEBUGSENTINAL; 
    newPage->sentinalEnd = DEBUGSENTINAL;
#endif

    // Microsoft 'AllocPages' throws if it cannot fulfill the request.
    VSASSERT (newPage, "Invalid");
    // Initialize the new page.
    newPage->next = NULL;
    newPage->firstAvail = (BYTE *)newPage + VBMath::RoundUpAllocSize(sizeof(NorlsPage));
    newPage->limitAvail = ((BYTE *)newPage) + allocSize;

    // We should have enough room in the new page!
    VSASSERT(newPage->limitAvail > newPage->firstAvail,"Invalid");
    VSASSERT((unsigned)(newPage->limitAvail - newPage->firstAvail) >= sz, "Invalid");

#if DEBUG
    VerifyHeap();
#endif

    return newPage;
}

NorlsMark NorlsAllocator::Mark()
{
    NorlsMark mark;
    Mark(&mark);
    return mark;
}

/*
 * Return a mark of the current allocation state, so you
 * can free all memory allocated subsequent.
 */
void NorlsAllocator::Mark(NorlsMark * mark)
{
    m_depth += 1;
    // Record current page and location.
    mark->page = pageLast;
    mark->nextFree = nextFree;
    mark->depth = m_depth;
#if NRLSTRACK  
    mark->m_nTotalAllocatedAtMark = m_nTotalAllocated;
#if NRLSTRACK_GETSTACKS    
    mark->m_pLastBlockAtMark = m_pLastBlock;
#endif NRLSTRACK_GETSTACKS    
#endif //NRLSTRACK  
}

size_t NorlsAllocator::CalcCommittedSize () const
{
    size_t iCommit = 0;
    for (NorlsPage *p = pageList; p; p = p->next)
        iCommit += p->limitAvail - (BYTE *)p;

    return iCommit;
}

/*
 * Free all memory allocated after the mark.
 */
void NorlsAllocator::Free(NorlsMark * mark)
{
    VSASSERT(mark->depth <= m_depth, "Depth check violation on a NorlsMark");

#if NRLSTRACK  
    m_nTotalAllocated=  mark->m_nTotalAllocatedAtMark ;
#if NRLSTRACK_GETSTACKS
    while (m_pLastBlock != mark->m_pLastBlockAtMark) // now walk the list of things to free after the mark
    {
        NrlsHeapData *pTemp = (NrlsHeapData *)m_pLastBlock->m_ptrNextBlock;
        VsDebugFreeInternal(TrackingHeapToUse(), m_pLastBlock);
        m_pLastBlock = pTemp;
    }
    VSASSERT(m_pLastBlock == mark->m_pLastBlockAtMark ," reset to the same lastblock when it was marked");
#endif //NRLSTRACK_GETSTACKS
#endif //NRLSTRACK  


    if (mark->page == NULL)
    {
        // Mark was before anything was allocated.
        FreeHeap();
    }
    else
    {
        NorlsPage * page, *nextPage;

#ifdef DEBUG
        page = pageList;
        while (page != pageLast)
        {
            if (page == mark->page)
                break;
            page = page->next;
        }
        VSASSERT(mark->page == page, "Invalid"); // we should have found the page in the list
        VerifyHeap();
#endif

        // Free all pages after the new last one.
        for (page = mark->page->next; page != NULL; page = nextPage)
        {
            nextPage = page->next;
            m_heapPage.FreePages(entity, page, (BYTE *)page->limitAvail - (BYTE *)page);
        }

        // Reset the last page and location.
        pageLast = mark->page;
        if (anyPageMarkedReadOnly)
        {
            SetPageWriteStatus(pageLast, true);
        }
        pageLast->next = NULL;
        nextFree = mark->nextFree;
        limitFree = pageLast->limitAvail;

#ifdef DEBUG
        // Free rest of page with junk
        memset(nextFree, 0xEE, limitFree - nextFree);
#endif //DEBUG

        MakeCurrentPageReadOnlyInternal();
    }

    m_depth = mark->depth;
}

/*
 * Free the entire heap.
 */
void NorlsAllocator::FreeHeap()
{
    NorlsPage * page, *nextPage;

    VerifyHeap();

    // Free all the pages.
    for (page = pageList; page != NULL; page = nextPage)
    {
        nextPage = page->next;
        m_heapPage.FreePages(entity, page, (BYTE *)page->limitAvail - (BYTE *)page);
    }

    // Reset the allocator.
    m_depth = 0;
    nextFree = limitFree = NULL;
    pageList = pageLast = NULL;

#if NRLSTRACK_GETSTACKS
    while (m_pLastBlock)
    {
        NrlsHeapData *pTemp = (NrlsHeapData *)m_pLastBlock->m_ptrNextBlock;
        VsDebugFreeInternal(TrackingHeapToUse(), m_pLastBlock);
        m_pLastBlock = pTemp;
    }
    m_pLastBlock = NULL;
#endif NRLSTRACK_GETSTACKS
#if NRLSTRACK
    m_nTotalAllocated = 0;
#endif NRLSTRACK

}

void
NorlsAllocator::VerifyHeap()
{
#if DEUBG
    // Make sure we can walk the heap
    for (page = pageList; page != NULL; page = nextPage)
    {
        SehGuard guard;
        try
        {
            VSASSERT(DEBUGSENTINAL==page->sentinalStart, "Bad start sentinal");
            VSASSERT(DEBUGSENTINAL==page->sentinalEnd, "Bad end sentinal");
            nextPage = page->next;
        } 
        catch ( SehException& ) 
        {
            VSASSERT(false, "Exception thrown during heap verification");
        }
    }
#endif
}


#if NRLSTRACK



long NrlsAllocTracker::AddAllocator(NorlsAllocator *pAllocator)
{
    CTinyGate gate(&m_lock);
    
    m_set.push_back(pAllocator);
    InterlockedIncrement(&g_nSerialNo);
    return g_nSerialNo;
}

void NrlsAllocTracker::RemoveAllocator(NorlsAllocator *pAllocator)
{
    CTinyGate gate(&m_lock);
    m_set.remove(pAllocator);
}



#define TEMPBUFSIZE   2048* sizeof(WCHAR)      

CComBSTR 
NrlsAllocTracker::GetAllocatorStatusReport(_Out_ long *pnCnt, _Out_ long *pnTotMem)
{
    CTinyGate gate(&m_lock);
    CComBSTR bstrResult;
    WCHAR szBuffer[TEMPBUFSIZE];
    
    listNorls::iterator it = m_set.begin();
    long nCnt = 0;
    long nTotMem = 0;
    
    for ( ; it != m_set.end() ; ++it)
    {

        NorlsAllocator *pAllocator = *it;
        nCnt++;
        nTotMem += pAllocator->m_nTotalAllocated;

        // pnTotMem   0 means get string report. 1 means just get the totals
        if (*pnTotMem == 0) // if the caller wants the entire report (expensive) or just the totals.
        {
           
            StringCchPrintfW(szBuffer, sizeof(szBuffer), L" %9d,  %4d, %5d, %ws(%d)\r\n", 
                            pAllocator->m_nTotalAllocated, 
                            pAllocator->m_dwCreatedThreadId, 
                            pAllocator->m_nSerialNo,
                            pAllocator->m_szFile, 
                            pAllocator->m_nLineNo);
            bstrResult.Append(szBuffer);
        }

    }
    *pnCnt = nCnt;
    *pnTotMem = nTotMem;
    return bstrResult;
    
}

//#include "..\..\..\vscommon\vsassert\mem.h"
// can't use offsetof because not friend
#define dataoffset  0x8003C //' offset of cCurNumAllocs in CHeapSpy see vsassert mem.cpp

VBMemoryInUse &
NrlsAllocTracker::GetVBMemoryStats(_In_ LPCSTR strDesc)
{
//    CHeapSpy * pSpy = (CHeapSpy * )g_vbCommonHeap;
    
    long *data = (long *)(((char *)g_vbCommonHeap)+dataoffset);

    m_VBMemoryInUse.nHeapTotalAllocs = data[0];
    m_VBMemoryInUse.nHeapTotalBytes = data[1];
    
    listNorls::iterator it = m_set.begin();
    long nCnt = 0;
    long nTotMem = 0;
    
    for ( ; it != m_set.end() ; ++it)
    {

        NorlsAllocator *pAllocator = *it;
        nCnt++;
        nTotMem += pAllocator->m_nTotalAllocated;
    }
    m_VBMemoryInUse.nNrlsTotalAllocs = nCnt;
    m_VBMemoryInUse.nNrlsTotalBytes = nTotMem;

    if (strDesc)
    {
        VsDebugPrintf("%20s   Heap Size %9d  Cnt %6d    Nrls Size %9d Cnt %6d\n", 
            strDesc,
            m_VBMemoryInUse.nHeapTotalBytes,
            m_VBMemoryInUse.nHeapTotalAllocs, 
            m_VBMemoryInUse.nNrlsTotalBytes,
            m_VBMemoryInUse.nNrlsTotalAllocs
            );
    }    
    return m_VBMemoryInUse;
}
#endif NRLSTRACK

