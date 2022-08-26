//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Page Heap management.  Ruthlessly stolen from the C# team.  Please notify Microsoft and Microsoft
//  about any changes to this file.  It is likely the change will need to be mirrored in the C#
//  implementation
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

/* The biggest trickiness with the page heap is that it is inefficient
* to allocate single pages from the operating system - NT will allocate
* only on 64K boundaries, so allocating a 4k page is needlessly inefficient.
* We use the ability to reserve then commit pages to reserve moderately
* large chunks of memory (a PageArena), then commit pages in the arena.
* This also allows us to track pages allocated and detect leaks.
*/

/*
* static data members
*/
size_t PageHeap::pageSize;         // The system page size.
int PageHeap::pageShift;           // log2 of the page size
bool PageHeap::reliableCommit;     // MEM_COMMIT reliable?

size_t GetSystemPageSize()
{
    static size_t g_pageSize;

    if (!g_pageSize)
    {
        // Get the system page size.
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        g_pageSize = sysinfo.dwPageSize;
    }

    return g_pageSize;
}

void PageHeap::StaticInit()
{
    // First time through -- get system page size.
    if (!PageHeap::pageSize)
    {
        // Get the system page size.
        PageHeap::pageSize = GetSystemPageSize();

        // Determine the page shift.
        int shift = 0;
        size_t size = PageHeap::pageSize;
        while (size != 1)
        {
            shift += 1;
            size >>= 1;
        }
        PageHeap::pageShift = shift;

        VSASSERT((size_t)(1 << PageHeap::pageShift) == PageHeap::pageSize, "Invalid");

        OSVERSIONINFO osvi;
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
#pragma warning ( disable : 4996 )
        BOOL ok = GetVersionEx(&osvi);
#pragma warning ( default : 4996 )
        VSASSERT(ok, "Invalid");
        reliableCommit = ok && osvi.dwMajorVersion >= 5;
    }
}

void PageHeap::PageArena::FreeAddressSpace()
{
    VirtualFree(pages, 0, MEM_RELEASE);
    pages = NULL;
    size = 0;
}

bool PageHeap::PageArena::HasUsedPages() const
{
    for (unsigned int i = 0; i < PAGES_PER_ARENA / BITS_DWORD; i++)
    {
        if (used[i])
            return true;
    }

    return false;
}

PageHeap::PageHeap() :
m_pageCurUse(0),
m_pageMaxUse(0),
m_pageCurReserve(0),
m_pageMaxReserve(0),
arenaList(NULL),
arenaLast(NULL),
singlePageArenaList(NULL),
singlePageArenaLast(NULL),
whatIsProtected(ProtectedEntityFlags::Nothing)
{
    CTinyGate gate (&lock ); // Acquire the lock
    StaticInit();
}

/*
* Destructor. Free everything.
*/
PageHeap::~PageHeap()
{
    CTinyGate gate (&lock ); // Acquire the lock
    FreeAllPages();
}

void PageHeap::ShrinkUnusedResources()
{
    //Microsoft the ordering of these is important. First free all possible
    //pages. That may result in an unused arena which are harvested by the second call.
    DecommitUnusedPages();
    FreeUnusedArenas();

#if NRLSTRACK
    // see where our remaining pages were allocated from
    CComBSTR str;
    long count = 0;
    long totmem = 1; // for thousands of allocators, this is very expensive on idle thread. Thus just get totals.
    str = g_NrlsAllocTracker->GetAllocatorStatusReport(&count, &totmem);
#endif
}

// Microsoft Search a segment of pages in an arena for cPages of contiguous free pages.
int PageHeap::PageArena::LookForPages(unsigned int cPages, int indexPageBegin, int indexLastValidPage)
{
    unsigned int cPagesRemaining = cPages;
    while (true)
    {
        // Scan to see if cPages pages starting at indexPageBegin are not in use.
        if (!(indexPageBegin & DWORD_BIT_MASK))
        {
            // The current page is on a DWORD boundary, use an optimized check.
            // Search this area for free pages. First, find a dword that isn't all used.
            // This loop quickly skips (32 at a time) the used pages.
            int dwIndexByDword;
            for (dwIndexByDword = indexPageBegin / BITS_DWORD; 
                dwIndexByDword <= indexLastValidPage / BITS_DWORD; 
                ++dwIndexByDword)
            {
                if (used[dwIndexByDword] != 0xFFFFFFFF)
                {
                    break;      // not all used.
                }
                else
                {
                    //all of these pages are used, reset the counter.
                    cPagesRemaining = cPages;
                }
            }
            indexPageBegin = dwIndexByDword * BITS_DWORD;
        }

        //Did the loop take us beyond the last valid page, if so
        //this allocation request can't be fulfilled from this range.
        if (indexPageBegin > indexLastValidPage)
        {
            return -1;
        }

        if (IsPageUsed(indexPageBegin))
        {
            cPagesRemaining = cPages;
            indexPageBegin++;
        }
        else
        {
            indexPageBegin++;
            cPagesRemaining--;


            if (!cPagesRemaining)
            {
                return indexPageBegin - cPages;
            }
        }
    }

    return -1;
}

void * PageHeap::PageArena::AllocPagesHelper(int iPage, unsigned int cPages, PageHeap& parent)
{
    size_t cBytes = cPages << pageShift;
    void* p = (BYTE *)pages + (iPage << pageShift);    // Calculate address of allocation.
    bool allCommitted = true;

    for (unsigned i = 0; i < cPages && allCommitted; i++)
    {
        if (! (IsPageCommitted(iPage + i)))
            allCommitted = false;
    }

    //  Commit the pages from the OS if needed.
    if (!allCommitted)
    {
        if (VirtualAlloc(p, cBytes, MEM_COMMIT, PAGE_READWRITE) != p)
        {
            // If the system couldn't commit the page then we're in
            // trouble. Presumably it is out of physical memory. Here we
            // make a last-ditch effort to continue by freeing up unused
            // pages in hope that we'll then be able to commit.
            if (!parent.DecommitUnusedPages() ||
                VirtualAlloc(p, cBytes, MEM_COMMIT, PAGE_READWRITE) != p)
            {
                VbThrow(GetLastHResultError());
            }
        }
    }
    if (!reliableCommit || allCommitted)
    {
        // On Win9X the above call to VirtualAlloc does not leave the memory writeable
        PageProtect::AllowWrite(ProtectedEntityFlags::UnusedMemory, p, cBytes);
    }

    // Mark them as in use and committed.
    unsigned int c = cPages;
    while (c--)
    {
        VSASSERT(! (IsPageUsed(iPage)), "Invalid");

        MarkPageUsed(iPage);
        MarkPageCommitted(iPage);

        ++iPage;
    }

#ifdef DEBUG
    // Make sure they aren't zero filled.
    memset(p, 0xCC, cBytes);
#endif //DEBUG

    return p;
}

void* PageHeap::PageArena::AllocPages(unsigned int cPages, PageHeap& parent)
{
    //Microsoft If SPREAD_ALLOCATIONS defined, minimize the reuse of address space.
    //Do this so that pointers to address space that has been allocated
    //and then freed more likely point to either decommitted or protected pages.
    //Otherwise, if we continue to reuse the same address space, dangling ptrs 
    //will very possibly be pointing to accessible memory allowing read and
    //write through them to bogus data.


    unsigned int iWhereToBeginPageSearch =
#ifdef SPREAD_ALLOCATIONS
        m_iStartNextAlloc;  //begin search at page after last allocation
#else
        0;
#endif

    int iPage = LookForPages((unsigned int)cPages, iWhereToBeginPageSearch, PAGES_PER_ARENA - 1);

#ifdef SPREAD_ALLOCATIONS
    if (-1 == iPage)
    {
        //previous search began at position N at which point cPages - 1 may have
        //been free, but the next page was used and therefore the search didn't
        //succeed. Therefore look from beginning to N + cPages - 1 so as not to
        //leave a potential hole of cPages - 1.
        iPage = LookForPages((unsigned int)cPages, 0, 
            min(m_iStartNextAlloc + (unsigned int)cPages - 1, PAGES_PER_ARENA - 1));
    }
#endif

    if (-1 != iPage)
    {
#ifdef SPREAD_ALLOCATIONS
        // Success, we found contiguous free pages.
        m_iStartNextAlloc = iPage + cPages;
#endif

        void* p = AllocPagesHelper(iPage, cPages, parent);

        // Return the address of the allocated pages.
        return p;
    }
    else
    {
        return NULL;
    }
}


/*
* Allocate a set of pages from the page heap. Memory is not
* zero-filled.
*/

void * PageHeap::AllocPages(size_t sz)
{
    CTinyGate gate (&lock ); // Acquire the lock

    VSASSERT(sz % pageSize == 0 && sz != 0, "Invalid");     // must be page size multiple.

    m_pageCurUse += sz / pageSize;
    if (m_pageCurUse > m_pageMaxUse)
        m_pageMaxUse = m_pageCurUse;

    // How many pages are being allocated?
    size_t cPages = (sz >> pageShift);

    // Handle Single Page alloc requests in an optimized fashion
    // This case is taken >99% of the time
    if (cPages == 1)
    {
        return SinglePageAlloc();
    }

    // Handle very large allocations differently.
    if (sz > BIGALLOC_SIZE)
    {
        return LargeAlloc(sz);
    }

    void * p;
    PageArena * arena;

    // Check each arena in turn for enough contiguous pages.
    for (arena = arenaList; arena != NULL; arena = arena->nextArena)
    {
        if (arena->type == LargeAllocation)
            continue;           // Large allocation arenas are not interesting.

        if (p = arena->AllocPages((unsigned int)cPages, *this))
        {
            return p;
        }
    }

    // No arenas have enough free space. Create a new arena and allocate
    // at the beginning of that arena.
    arena = CreateArena(Normal, PAGES_PER_ARENA * pageSize);


    p = arena->AllocPages((unsigned int)cPages, *this);

    return p;
}

template <typename T>
void PageHeap::RemoveArena (const T* goingAway, T*& containingArenaList, T*& containingArenaListLast)
{
    TemplateUtil::CompileAssertIsChildOf<PageArena,T>();
    // Remove the arena from the arena list.
    if (containingArenaList == goingAway)
    {
        containingArenaList = (T*) goingAway->nextArena;
        if (containingArenaListLast == goingAway)
            containingArenaListLast = NULL;
    }
    else
    {
        T* arenaPrev;

        // Find arena just before the one we want to remove
        for (arenaPrev = containingArenaList; arenaPrev->nextArena != goingAway; arenaPrev = (T*) arenaPrev->nextArena)
            ;

        VSASSERT(arenaPrev->nextArena == goingAway, "Invalid");
        arenaPrev->nextArena = (T*) goingAway->nextArena;
        if (containingArenaListLast == goingAway)
            containingArenaListLast = arenaPrev;
    }
}

void PageHeap::FreeUnusedArenas()
{
    CTinyGate gate (&lock); 

    PageArena* nextArena = NULL;

    for (PageArena* arena = arenaList; arena != NULL; arena = nextArena)
    {
        nextArena = arena->nextArena;

        if (arena->type == LargeAllocation)
            continue;

        if (!arena->HasUsedPages())
        {
            //unlink from list.
            RemoveArena(arena, arenaList, arenaLast);
            size_t addressSpace = arena->GetAddressSpaceSize();
            arena->FreeAddressSpace();
            m_pageCurReserve -= addressSpace / pageSize;

            delete arena;
        }
    }

    // The only arenas that may potentially be freed are in the queue of arenas with free pages.
    int size = static_cast<int>(singlePageArenasWithFreePages.size()); 
    for(int i = 0; i < size; i++)
    {
        SinglePageArena* arena = singlePageArenasWithFreePages.front();
        singlePageArenasWithFreePages.pop();

        if (arena->NumberOfFreePagesAvailable() == PAGES_PER_ARENA) // all the pages are free
        {
            // Unlink from list and delete the arena
            addressToSinglePageArenaMap.erase(arena->pages);
            RemoveArena(arena, singlePageArenaList, singlePageArenaLast);
            size_t addressSpace = arena->GetAddressSpaceSize();
            arena->FreeAddressSpace();                       // sets arena->pages = NULL
            m_pageCurReserve -= addressSpace / pageSize;

            delete arena;
        }
        else
        {
            singlePageArenasWithFreePages.push(arena);  // add the arena back to the queue if we didn't delete it
        }
    }
}

/*
* Free pages page to the page heap. The size must be the
* same as when allocated.
*/
void PageHeap::FreePages(ProtectedEntityFlagsEnum entity, _Post_invalid_ void * p, size_t sz)
{
    CTinyGate gate (&lock ); // Acquire the lock

    VSASSERT(sz % pageSize == 0 && sz != 0, "Invalid");     // must be page size multiple.

    m_pageCurUse -= sz / pageSize;

    size_t cPages = (sz >> pageShift);
    if (cPages == 1)
    {
        SinglePageFree(entity, p);
        return;
    }

    // Handle very large allocations differently.
    if (sz > BIGALLOC_SIZE)
    {
        LargeFree(p, sz);
        return ;
    }

    // Find the arena this page is in.
    PageArena * arena = FindArena(p);
    VSASSERT(arena, "Invalid");

    FreePagesHelper(entity, arena, p, sz);
}

void PageHeap::FreePagesHelper(ProtectedEntityFlagsEnum entity, PageArena * arena, void * p, size_t sz)
{
    size_t cPages = (sz >> pageShift);

    // Get page index within this arena, and page count.
    size_t initialPageIndex = ((BYTE *)p - (BYTE *)arena->pages) >> pageShift;

    // Pages must be in-use and committed. Set the pages to not-in-use. We could
    // decommit the pages here, but it is more efficient to keep them around
    // committed because we'll probably want them again. To actually decommit
    // them, call PageHeap::DecommitUnusedPages().
    size_t iPage = initialPageIndex; 
    while (cPages--)
    {
        VSASSERT(arena->used[iPage >> DWORD_BIT_SHIFT] & (1 << (iPage & DWORD_BIT_MASK)), "Invalid");
        VSASSERT(arena->committed[iPage >> DWORD_BIT_SHIFT] & (1 << (iPage & DWORD_BIT_MASK)), "Invalid");

        arena->used[iPage >> DWORD_BIT_SHIFT] &= ~(1 << (iPage & DWORD_BIT_MASK));

        ++iPage;
    }

#ifdef DECOMMIT_ON_FREE
    {
        iPage = initialPageIndex;
        BOOL b = VirtualFree((BYTE *)arena->pages + (iPage << pageShift), sz, MEM_DECOMMIT);
        ASSERT(b);  //Microsoft throw VcsException?
        size_t cPgs = sz >> pageShift;
        if (b)
        {
            while (cPgs--)
            {
                arena->ClearPageCommitted((unsigned int)iPage);
                iPage++;
            }
        }
    }
#else

#ifdef DEBUG
    PageProtect::AllowWrite(entity, p, sz);

    // Fill pages with junk to indicated unused.
    memset(p, 0xAE, sz);
#endif //DEBUG

    if (PageProtect::IsEntityProtected(ProtectedEntityFlags::UnusedMemory))
    {
        PageProtect::ForbidAccess(ProtectedEntityFlags::UnusedMemory, p, sz);
    }
    else
    {
        PageProtect::AllowWrite(entity, p, sz);
    }
#endif
}

/////////////////////////////////////////////////////////////////////////////////
// Allocate a very large allocation. An entire arena is allocated for the allocation.

void* PageHeap::LargeAlloc(size_t sz)
{
    CTinyGate gate (&lock); // Acquire the lock
    // Create an arena for this large allocation.
    PageArena* newArena = CreateArena(LargeAllocation, sz);

#ifdef DEBUG
    // Make sure they aren't zero filled.
    memset(newArena->pages, 0xCC, sz);
#endif //DEBUG

    return newArena->pages;
}

/*
* Free a large allocation made via LargeAlloc.
*/
void PageHeap::LargeFree(void * p, size_t sz)
{
    // Find the arena corresponding to this large allocation.
    CTinyGate gate (&lock); // Acquire the lock
    PageArena * arena = FindArena(p);
    VSASSERT(arena && arena->type == LargeAllocation && arena->pages == p && arena->size == sz, "Invalid");

    m_pageCurReserve -= sz / pageSize;

    // Free the pages.
    BOOL b;
    b = VirtualFree(p, 0, MEM_RELEASE);
    VSASSERT(b, "Invalid");

    // Remove the arena from the arena list.
    if (arenaList == arena)
    {
        arenaList = arena->nextArena;
        if (arenaLast == arena)
            arenaLast = NULL;
    }
    else
    {
        PageArena * arenaPrev;

        // Find arena just before the one we want to remove
        for (arenaPrev = arenaList; arenaPrev->nextArena != arena; arenaPrev = arenaPrev->nextArena)
            ;

        VSASSERT(arenaPrev->nextArena == arena, "Invalid");
        arenaPrev->nextArena = arena->nextArena;
        if (arenaLast == arena)
            arenaLast = arenaPrev;
    }

    // Free the arena structure.
    delete arena;
}

/////////////////////////////////////////////////////////////////////////////////
// Allocate a single page allocation. 

void* PageHeap::SinglePageAlloc()
{
    void * p;
    SinglePageArena * arena;

    if (!singlePageArenasWithFreePages.empty())
    {
        // any arena will work here, taking the first one each time is probably best for locality
        arena = singlePageArenasWithFreePages.front(); 

        // pop the top free page from our stack
        int iPage = arena->freePageStack[arena->topOfFreePageStack];
        arena->topOfFreePageStack--;

        // remove the arena from the singlePageArenasWithFreePages set if we just used the last page
        if (arena->NumberOfFreePagesAvailable() == 0)
        {
            singlePageArenasWithFreePages.pop();
        }

        p = arena->AllocPagesHelper(iPage, 1, *this);

        // Return the address of the allocated pages.
        return p;
    }

    // No arenas have enough free space. Create a new arena and allocate
    // at the beginning of that arena.
    arena = (SinglePageArena*) CreateArena(SinglePageAllocation, PAGES_PER_ARENA * pageSize);

    int iPage = arena->freePageStack[arena->topOfFreePageStack];
    arena->topOfFreePageStack--;

    p = arena->AllocPagesHelper(iPage, 1, *this);

    return p;
}

/*
* Free a single page allocation made via SinglePageAlloc.
*/
void PageHeap::SinglePageFree(ProtectedEntityFlagsEnum entity, _Post_invalid_ void * p)
{
    CTinyGate gate (&lock); // Acquire the lock
    // Find the arena corresponding to free
    // Get the first arena whose starting memory address is greater than p, then go back one arena because
    // p belongs to the closest arena whose first page is <= p, and upper_bound returns the first arena whose
    // page is strictly greater than p.
    SinglePageArena* arena = (--addressToSinglePageArenaMap.upper_bound(p))->second;
    VSASSERT(arena && arena->size == PAGES_PER_ARENA * pageSize && arena->OwnsPage(p), "Invalid");

    // Mark the page as freed
    FreePagesHelper(entity, arena, p, pageSize);

    // push page back on to our free page stack
    int iPage = (int) ((BYTE *)p - (BYTE *)arena->pages) >> pageShift;
    ++arena->topOfFreePageStack;
    VSASSERT(arena->topOfFreePageStack < PAGES_PER_ARENA, "too many pages available");
    arena->freePageStack[arena->topOfFreePageStack] = iPage;

    // add this arena back to our free list if we were full, but now have a single free page
    if (arena->NumberOfFreePagesAvailable() == 1)
    {
        singlePageArenasWithFreePages.push(arena);
    }
}

void FreeArenaList(PageHeap::PageArena* list, bool checkLeaks)
{
    PageHeap::PageArena * arena, *nextArena;

    for (arena = list; arena != NULL; arena = nextArena)
    {
        nextArena = arena->nextArena;

        // Check arena for leaks, if desired.
        if (checkLeaks)
        {
            VSASSERT(arena->type != PageHeap::LargeAllocation, "Invalid");        // Large allocation should have been freed by now.

            for (int dwIndex = 0; dwIndex < PAGES_PER_ARENA / BITS_DWORD; ++dwIndex)
            {
                VSASSERT(arena->used[dwIndex] == 0, "Invalid");  // All pages in this arena should be free.
            }
        }

        // Free the pages in the arena.
        BOOL b;
        b = VirtualFree(arena->pages, 0, MEM_RELEASE);
        VSASSERT(b, "Invalid");

        // Free the arena structure.
        delete arena;
    }
}

/*
* Free everything allocated by the page heap; optionally checking for
* leak (memory that hasn't been freed via FreePages).
*/
void PageHeap::FreeAllPages(bool checkLeaks)
{
    CTinyGate gate (&lock ); // Acquire the lock

    FreeArenaList(arenaList, checkLeaks);
    FreeArenaList(singlePageArenaList, checkLeaks);

    m_pageCurUse = m_pageCurReserve = 0;
    arenaList = arenaLast = NULL;
    singlePageArenaList = singlePageArenaLast = NULL;

    addressToSinglePageArenaMap.clear();  
    singlePageArenasWithFreePages = std::queue<SinglePageArena*>();
}

bool PageHeap::DecommitUnusedPagesFromArenaList(PageArena* list)
{
    bool anyDecommitted = false;

    PageArena * arena;
    BOOL b;

    for (arena = list; arena != NULL; arena = arena->nextArena)
    {
        if (arena->type == LargeAllocation)
            continue;

        for (int dwIndex = 0; dwIndex < PAGES_PER_ARENA / BITS_DWORD; ++dwIndex)
        {
            // Can we decommit 32 pages at once with one OS call?
            if (arena->used[dwIndex] == 0 && arena->committed[dwIndex] != 0)
            {
#pragma warning (push)
#pragma warning (disable: 6250)
                b = VirtualFree((BYTE *)arena->pages + ((dwIndex * BITS_DWORD) << pageShift),
                    BITS_DWORD << pageShift,
                    MEM_DECOMMIT);
#pragma warning (pop)
                VSASSERT(b, "Invalid");
                if (b)
                {
                    anyDecommitted = true;
                    arena->committed[dwIndex] = 0;
                }
            }
            else if (arena->used[dwIndex] != arena->committed[dwIndex])
            {
                // Some pages in this group should be decommitted. Check each one individually.
                for (int iPage = dwIndex * BITS_DWORD; iPage < (dwIndex + 1) * BITS_DWORD; ++iPage)
                {
                    if ( ! (arena->used[iPage >> DWORD_BIT_SHIFT] & (1 << (iPage & DWORD_BIT_MASK))) &&
                        (arena->committed[iPage >> DWORD_BIT_SHIFT] & (1 << (iPage & DWORD_BIT_MASK))))
                    {
#pragma warning (push)
#pragma warning (disable: 6250)
                        b = VirtualFree((BYTE *)arena->pages + (iPage << pageShift), pageSize, MEM_DECOMMIT);
#pragma warning (pop)
                        VSASSERT(b, "Invalid");
                        if (b)
                        {
                            anyDecommitted = true;
                            arena->committed[iPage >> DWORD_BIT_SHIFT] &= ~(1 << (iPage & DWORD_BIT_MASK));
                        }
                    }
                }
            }
        }

#ifdef DEBUG
        // At this point, the only committed pages in this arena should be in use.
        for (int dwIndex = 0; dwIndex < PAGES_PER_ARENA / BITS_DWORD; ++dwIndex)
        {
            VSASSERT(arena->used[dwIndex] == arena->committed[dwIndex], "Invalid");
        }
#endif //DEBUG
    }
    return anyDecommitted;
}

/*
* Decommit any pages that aren't in use. Decommits memory that
* can be profitably used by other parts of the system. 
*/
bool PageHeap::DecommitUnusedPages()
{
    CTinyGate gate (&lock ); // Acquire the lock

    bool anyDecommitted = DecommitUnusedPagesFromArenaList(arenaList);
    anyDecommitted |= DecommitUnusedPagesFromArenaList(singlePageArenaList);

    return anyDecommitted;
}

PageHeap::SinglePageArena::SinglePageArena()
{
    // push the arena's list of pages onto the free page stack
    topOfFreePageStack = -1;
    for(int i = 0; i < PAGES_PER_ARENA; i++)
    {
        ++topOfFreePageStack;
        freePageStack[topOfFreePageStack] = i;
    }
}

/*
* Create a new memory arena of a size and reserve or commit pages for it.
* If type == LargeAllocation, set this as a "large allocation" arena and commit
* the memory. If not just reserve the memory.
*/
PageHeap::PageArena * PageHeap::CreateArena(PageArenaType type, size_t sz)
{
    // Allocate an arena and reserve pages for it.
    PageArena* newArena; 
    SinglePageArena* newSinglePageArena = NULL;

    if (type == SinglePageAllocation)
    {
        newSinglePageArena = new (zeromemory) SinglePageArena;

        if (!singlePageArenaList)
        {
            singlePageArenaList = singlePageArenaLast = newSinglePageArena;
        }
        else
        {
            // Add to front of arena list
            newSinglePageArena->nextArena = singlePageArenaList;
            singlePageArenaList = newSinglePageArena;
        }

        newArena = newSinglePageArena;
    }
    else
    {
        newArena = new (zeromemory) PageArena;

        // Add to arena list. For efficiency, large allocation arenas are placed
        // at the end, but regular arenas at the beginning. This ensures that
        // regular allocation are almost always satisfied by the first arena in the list.
        if (!arenaList)
        {
            arenaList = arenaLast = newArena;
        }
        else if (type == LargeAllocation)
        {
            // Add to end of arena list.
            arenaLast->nextArena = newArena;
            arenaLast = newArena;
        }
        else
        {
            // Add to front of arena list
            newArena->nextArena = arenaList;
            arenaList = newArena;
        }
    }

    newArena->pages = VirtualAlloc(0, sz, type == LargeAllocation ? MEM_COMMIT : MEM_RESERVE, PAGE_READWRITE);
    if (!newArena->pages)
    {
        VbThrow(GetLastHResultError());
    }

    if (newSinglePageArena)
    {
        // also add the new SinglePageArena to our indexing data structures
        VSASSERT(addressToSinglePageArenaMap.find(newSinglePageArena->pages) == addressToSinglePageArenaMap.end(), 
            "We shouldn't already have an arena for this address");
        addressToSinglePageArenaMap[newSinglePageArena->pages] = newSinglePageArena;
        singlePageArenasWithFreePages.push(newSinglePageArena);
    }

    m_pageCurReserve += sz / pageSize;
    if (m_pageCurReserve > m_pageMaxReserve)
    {
        m_pageMaxReserve = m_pageCurReserve;
    }

    newArena->size = sz;
    newArena->type = type;

    return newArena;
}

/*
* Find an arena that contains a particular pointer.
*/
PageHeap::PageArena * PageHeap::FindArena(const void * p)
{
    PageArena * arena;

    for (arena = arenaList; arena != NULL; arena = arena->nextArena)
    {
        if (arena->OwnsPage(p))
            return arena;
    }

    VSASSERT(0, "Invalid");      // Should find the arena.
    return NULL;
}
