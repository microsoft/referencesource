//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implements the page manager for our NorlsAllocator instances
//
//-------------------------------------------------------------------------------------------------

#pragma once

/* Page allocation heap. Allocates and frees pages
* or groups of pages. Allocation must be a multiple
* of the system page size. Memory is not zeroed.
*/

#define SPREAD_ALLOCATIONS  // allocate after the previous allocation instead of looking from the
                            // beginning of each arena

#define PAGES_PER_ARENA 128              // 4k system page size => 128*4k = 512KB per arena
#define BIGALLOC_SIZE   (128 * 1024)    // more than this alloc (128KB) is not done from an arena.

#define DWORD_BIT_SHIFT 5        // log2 of bits in a DWORD.
#define BITS_DWORD      (1 << DWORD_BIT_SHIFT)
#define DWORD_BIT_MASK  (BITS_DWORD - 1)

extern size_t GetSystemPageSize();
class PageHeap
{
public:

    enum PageArenaType
    {
        Normal,
        LargeAllocation,
        SinglePageAllocation
    };

    // Store information about a memory arena we allocate pages from.
    struct PageArena
    {


        PageArena* nextArena;  // the arena.
        void* pages;           // the pages in the arena.
        size_t size;            // size of the arena.
        PageArenaType type;        // large allocs and single page allocs have special cased codepaths
        DWORD used[PAGES_PER_ARENA / BITS_DWORD];        // bit map of in-use pages in this arena.
        DWORD committed[PAGES_PER_ARENA / BITS_DWORD];   // bit map of committed pages in this arena.


        bool OwnsPage(const void* p) const
        {
            return (p >= pages && p < (BYTE*)pages + size);
        }

        bool IsPageCommitted (unsigned iPage) const
        {
            return TestPage(committed, iPage);
        }
        bool IsPageUsed (unsigned iPage) const
        {
            return TestPage(used, iPage);
        }

        void MarkPageUsed (unsigned iPage)
        {
            SetPage(used, iPage);
        }

        void MarkPageCommitted (unsigned iPage)
        {
            SetPage(committed, iPage);
        }

        void ClearPageUsed (unsigned iPage)
        {
            ClearPage(used, iPage);
        }

        void ClearPageCommitted (unsigned iPage)
        {
            ClearPage(committed, iPage);
        }

        bool HasUsedPages() const;

        size_t GetAddressSpaceSize() const
        {
            return size;
        }

        void FreeAddressSpace();
        void* AllocPages(unsigned int cPages, PageHeap& parent);
        void* AllocPagesHelper(int iPage, unsigned int cPages, PageHeap& parent);

    protected:

#ifdef SPREAD_ALLOCATIONS
        int m_iStartNextAlloc;  // used in PageArena::AllocPages
#endif

        int LookForPages(unsigned int cPages, int indexPageBegin, int indexLastValidPage);

        void SetPage(_Out_cap_(PAGES_PER_ARENA >> DWORD_BIT_SHIFT) DWORD bitvector[], unsigned index)
        {
            VSASSERT(index < PAGES_PER_ARENA, "Invalid");
            bitvector[index >> DWORD_BIT_SHIFT] |= (1 << (index & DWORD_BIT_MASK));
        }

        bool TestPage(DWORD const bitvector[], unsigned index) const
        {
            VSASSERT(index < PAGES_PER_ARENA, "Invalid");
            return bitvector[index >> DWORD_BIT_SHIFT] & (1 << (index & DWORD_BIT_MASK));
        }

        void ClearPage(_Out_cap_(PAGES_PER_ARENA >> DWORD_BIT_SHIFT) DWORD bitvector[], unsigned index) 
        {
            VSASSERT(index < PAGES_PER_ARENA, "Invalid");
            bitvector[index >> DWORD_BIT_SHIFT] &= ~(1 << (index & DWORD_BIT_MASK));
        }
    };

    struct SinglePageArena : public PageArena
    {
        int freePageStack[PAGES_PER_ARENA]; // stack of free pages available in singlePageAlloc case
        int topOfFreePageStack;	            // -1 when there are no free pages, will initially be PAGES_PER_ARENA -1

        SinglePageArena();

        int NumberOfFreePagesAvailable()
        {
            return topOfFreePageStack + 1;
        }
    };

    PageHeap();
    ~PageHeap();

    static void StaticInit();

    void* AllocPages( _In_ size_t sz);
    void FreePages(ProtectedEntityFlagsEnum entity, _Post_invalid_ void* p, size_t sz);
    void FreeAllPages(bool checkLeaks = true);

    // When previously committed pages are freed, they are merely marked
    // as unused, not decommitted. Call this to decommit any unused pages.
    bool DecommitUnusedPages();
    // After all unsused pages in an arena are decommitted (by calling DecommitUnusedPages),
    // the arena may be left with no used pages. In that case it can be freed,
    // returning address space to the process. Call this to release any unsued arenas.
    void FreeUnusedArenas();
    // Microsoft this call first decommits unused pages and then frees unused arenas.
    void ShrinkUnusedResources();

    static size_t pageSize;         // The system page size.

    unsigned GetCurrentUseSize() const
    {
        return (unsigned)(m_pageCurUse * pageSize);
    }
    unsigned GetMaxUseSize() const
    {
        return (unsigned)(m_pageMaxUse * pageSize);
    }
    unsigned GetCurrentReserveSize() const
    {
        return (unsigned)(m_pageCurReserve * pageSize);
    }
    unsigned GetMaxReserveSize() const
    {
        return (unsigned)(m_pageMaxReserve * pageSize);
    }

    PageArena* FindArena(const void * p);

private:
    CTinyLock lock;             // This is the lock mechanism for thread safety.

    PageArena* CreateArena(PageArenaType type, size_t sz);

    template <typename T>
    void RemoveArena(const T* goingAway, T*& containingArenaList, T*& containingArenaListLast);
    bool DecommitUnusedPagesFromArenaList(PageArena* list);

    void FreePagesHelper(ProtectedEntityFlagsEnum entity, PageArena* arena, _Post_invalid_ void* p, size_t sz);

    // special case allocation/free behavior for large allocations
    void * LargeAlloc(size_t sz);
    void LargeFree(void * p, size_t sz);

    // special case allocation/free behavior for single page allocations
    void* SinglePageAlloc();
    void SinglePageFree(ProtectedEntityFlagsEnum entity, _Post_invalid_ void* p);

    SinglePageArena* singlePageArenaList; // List of memory arenas exclusively for single page allocs
    SinglePageArena* singlePageArenaLast; // Last memory arena in list.
    
    // used to efficiently find the arena a freed memory address belonged to
    std::map<void *, SinglePageArena*> addressToSinglePageArenaMap;  
    // used to efficiently find an arena to make a new allocation from
    std::queue<SinglePageArena*> singlePageArenasWithFreePages;

    PageArena* arenaList;          // List of memory arenas.
    PageArena* arenaLast;          // Last memory arena in list.

    ProtectedEntityFlagsEnum whatIsProtected;

    static int pageShift;           // log2 of the page size
    static bool reliableCommit;     // Commit of memory protects it correctly even if already committed

    size_t m_pageCurUse, m_pageMaxUse;
    size_t m_pageCurReserve, m_pageMaxReserve;
};
