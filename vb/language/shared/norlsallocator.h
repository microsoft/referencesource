//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implements No Release Allocator.
//
//-------------------------------------------------------------------------------------------------

#pragma once


//to track nrlsalloc locations, #define NRLSTRACK 
// set NRLSTRACK_GETSTACKS to 1 to track NRLSAllocations and the call stacks. Uses lots of mem: very expensive

//#define NRLSTRACK  DEBUG &&  !defined( VBDBGEE )  && !defined(NDEBUG) && !defined(BUILDING_VBC) && !defined(BUILDING_VB7TO8)
//#define NRLSTRACK_GETSTACKS NRLSTRACK

#if NRLSTRACK

// we're tracking allocations.

#include <hash_set>
using namespace stdext;

// we're tracking allocations.

struct VBMemoryInUse
{
    long nHeapTotalBytes;
    long nHeapTotalAllocs;
    
    long nNrlsTotalBytes;
    long nNrlsTotalAllocs;
    
};

class NrlsAllocTracker
{
public:
    NrlsAllocTracker():
        g_nSerialNo(0)
    {
    }
    ~NrlsAllocTracker()
    {
    }
    long AddAllocator(NorlsAllocator *pAllocator);

    void RemoveAllocator(NorlsAllocator *pAllocator);

    CComBSTR GetAllocatorStatusReport(_Out_ long *pnCnt, _Out_ long *pnTotMem);
    VBMemoryInUse & GetVBMemoryStats(_In_ LPCSTR strDesc);
   

private:
    typedef std::list<NorlsAllocator *> listNorls;
    listNorls m_set;
    CTinyLock m_lock;
    long g_nSerialNo;
    VBMemoryInUse m_VBMemoryInUse ;
};


extern NrlsAllocTracker *g_NrlsAllocTracker;
#if NRLSTRACK_GETSTACKS    
    typedef struct {
        void * m_ptrNextBlock; // linked list of allocations: point to prior one
        size_t m_size; // size of the nrls data
        void * m_data; // ptr to the actual NRLS data
    } NrlsHeapData;
#endif NRLSTRACK_GETSTACKS    

#endif NRLSTRACK



// Holds a page of memory being used by the NorlsAllocator.
struct NorlsPage
{
#if DEBUG
    size_t sentinalStart;
#endif

    NorlsPage * next;      // next page in use.
    BYTE * firstAvail;  // first available byte for allocation.
    BYTE * limitAvail;  // limit for allocation.

#if DEBUG
    size_t sentinalEnd; // Buffer overflow check
#endif
};

class NorlsMark
{
private:
    friend class NorlsAllocator;

    NorlsPage * page;      // page.
    BYTE * nextFree;    // first free location within the page.
    unsigned depth;

#if NRLSTRACK  
    long m_nTotalAllocatedAtMark;
#if NRLSTRACK_GETSTACKS    
    NrlsHeapData * m_pLastBlockAtMark;           // ptr to linked list of alloc'd Heap blocks so VSAssert CHeapSpy can watch Nrlsallocs
#endif NRLSTRACK_GETSTACKS    
#endif //NRLSTRACK  

};


#if NRLSTRACK  
#define NORLSLOC __WFILE__, __LINE__
#else
#define NORLSLOC -1
#endif

class NorlsAllocator
{
friend class NrlsAllocTracker;    
public:
    NEW_CTOR_SAFE()

    class AllowingWrite
    {
    private:
        NorlsAllocator * heap;
        bool alreadyInAllowingWrite;
    public:
        AllowingWrite(NorlsAllocator * heap)
        {
            this->heap = heap;
            if (heap)
            {
                alreadyInAllowingWrite = heap->inAllowingWrite;
                heap->inAllowingWrite = true;
            }
            else
            {
                alreadyInAllowingWrite = false;
            }
        }
        ~AllowingWrite()
        {
            if (!alreadyInAllowingWrite && heap)
            {
                heap->inAllowingWrite = false;
            }
        }
        bool OwnsHeapWriteability()
        {
            return !alreadyInAllowingWrite;
        }
    };



    NorlsAllocator(
#if NRLSTRACK
     _In_ WCHAR *szFile, _In_ long nLineNo
#else
    _In_ int nUnused 
#endif NRLSTRACK
    );
    ~NorlsAllocator();

    NorlsAllocator(
#if NRLSTRACK
     _In_ WCHAR *szFile, _In_ long nLineNo,
#else
    _In_ int nUnused,
#endif NRLSTRACK
    _In_ PageHeap& heapToAllocateFrom
    );

    PageHeap& GetPageHeap()
    {
        return m_heapPage;
    }

    void ForbidWrite(void * p, size_t sz)
    {
        VerifyHeapEntity();
        ForbidWriteInternal(p, sz);
    }
    void AllowWrite(void * p, size_t sz)
    {
        VerifyHeapEntity();
        AllowWriteInternal(p, sz);
    }

    void DisallowReadOnlyDirectives()
    {
        VerifyHeapEntity();
        DisallowReadOnlyDirectivesInternal();
    }
    void AllowReadOnlyDirectives()
    {
        VerifyHeapEntity();
        AllowReadOnlyDirectivesInternal();
    }
    bool AllowingReadOnlyDirectives()
    {
        VerifyHeapEntity();
        return AllowingReadOnlyDirectivesInternal();
    }
    void MakeCurrentPageReadOnly()
    {
        VerifyHeapEntity();
        MakeCurrentPageReadOnlyInternal();
    } // Calling Alloc makes it writeable until the next call to MakeReadOnly
    void MakeCurrentPageWriteable()
    {
        VerifyHeapEntity();
        MakeCurrentPageWriteableInternal();
    } // Calling Alloc makes it writeable until the next call to MakeReadOnly
    void MakeAllHeapReadOnly()
    {
        VerifyHeapEntity();
        MakeAllHeapReadOnlyInternal();
    }
    void MakeAllHeapWriteable()
    {
        VerifyHeapEntity();
        MakeAllHeapWriteableInternal();
    }

    bool IsAddressInCurrentPage (const void * addr) const;
    bool IsAddressInHeap (const void * addr) const
    {
        for (NorlsPage * page = pageList; page; page = page->next)
        {
            if (addr >= page && addr < page->limitAvail)
                return true;
        }
        return false;
    }

    void* AllocNonZero(size_t sz); 
    void* Alloc(size_t sz);
    template <typename T>
    T* Alloc()
    {
        return (T*)Alloc(sizeof(T));
    }
    template <typename T>
    T* _fastcall AllocArray(size_t count)
    {
        return (T*)Alloc(VBMath::Multiply(sizeof(T),count));
    }

    void *Resize(
        _In_bytecount_(cbSizeOld) void* pv,
        _In_ size_t cbSizeOld,
        _In_ size_t cbSizeNew);

    void ValidateHeap();
    PWSTR AllocStr(PCWSTR str);
    NorlsMark Mark();
    void Mark(NorlsMark * mark);
    void Free(NorlsMark * mark);
    void FreeHeap();
    size_t CalcCommittedSize () const;
    const WCHAR* GetDebugIdentifier() const;
#if NRLSTRACK
    WCHAR *m_szFile;
    long m_nLineNo;
    DWORD m_dwCreatedThreadId; // the thread on which this allocator was created
    DWORD m_dwAllocThreadId;// the thread on which allocations are occuring
    long m_nSerialNo;        // the allocator's serialno
    long m_nSeqNo;  /// the sequence # of the allocation for this allcoator
    ULONG m_nTotalAllocated;
#if NRLSTRACK_GETSTACKS    
    NrlsHeapData  * m_pLastBlock;           // ptr to linked list of alloc'd Heap blocks so VSAssert CHeapSpy can watch Nrlsallocs
    HANDLE  TrackingHeapToUse();
    static HANDLE g_nraSymbolHeap ;

#endif NRLSTRACK_GETSTACKS    
    
        
#endif NRLSTRACK

private:
    NorlsAllocator(const NorlsAllocator&);
    NorlsAllocator& operator=(const NorlsAllocator&);

    void Init(ProtectedEntityFlagsEnum entity);
    void VerifyHeapEntity()
    {
        VSASSERT(entity, "Heap entity must be set before manipulating heap writeability");
    }
    void VerifyHeap();
    void ForbidWriteInternal(void * p, size_t sz);
    void AllowWriteInternal(void * p, size_t sz);

    //allocates memory but doesn't clear it.
    //'sz' [in/out] returns the true allocation size
    void* _AllocNonZero(size_t& sz); 

    void DisallowReadOnlyDirectivesInternal()
    {
        allowReadOnlyDirectives = false;
    }
    void AllowReadOnlyDirectivesInternal()
    {
        allowReadOnlyDirectives = true;
    }
    bool AllowingReadOnlyDirectivesInternal()
    {
        return allowReadOnlyDirectives;
    }
    void MakeCurrentPageReadOnlyInternal(); // Calling Alloc makes it writeable until the next call to MakeReadOnly
    void MakeCurrentPageWriteableInternal(); // Calling Alloc makes it writeable until the next call to MakeReadOnly
    void MakeAllHeapReadOnlyInternal();
    void MakeAllHeapWriteableInternal();

    void SetPageWriteStatus(NorlsPage * page, bool writeable);
    void SetPageRegionWriteStatus(NorlsPage * first, NorlsPage * last, bool writeable);
    void SetAllPagesWriteStatus(bool writeable);

    BYTE * nextFree;                // location of free area
    BYTE * limitFree;               // just beyond end of free area in this page.

    NorlsPage * pageList;              // list of pages used by this allocator.
    NorlsPage * pageLast;              // last page in the list.

    // The source of pages for this heap.
    PageHeap& m_heapPage;
    unsigned m_depth;

    void AllocNewPage(size_t sz);
    NorlsPage * NewPage(size_t sz);

    ProtectedEntityFlagsEnum entity;
    bool allowReadOnlyDirectives;
    bool anyPageMarkedReadOnly;
    bool inAllowingWrite;

#ifdef DEBUG
    inline size_t DebugSize (size_t sz) const
    {
        // integer overflow
        if (!(sz < sz + 1 + sizeof(size_t) + sizeof(void*) + sizeof(void*)))
        {
            VSASSERT(sz < sz + 1 + sizeof(size_t) + sizeof(void*) + sizeof(void*), "Invalid");
            return 0;
        }
        return VBMath::RoundUpAllocSize(sizeof(size_t)) + VBMath::RoundUpAllocSize(sz + 1); // int at beginning for size, plus 1 (or more) bytes of sentinel.
    }
    static const BYTE DEBUGSENTINAL = 0xAE;  // put at end of block to detect overrun.
#endif //DEBUG
};

class NRHeapMarker
{
public:
    NRHeapMarker(NorlsAllocator * h)
    {
        heap = h;
        heap->Mark(&mark);
    }
    ~NRHeapMarker()
    {
        heap->Free(&mark);
    }
private:
    NorlsAllocator * heap;
    NorlsMark mark;
};

class NRHeapWriteMaker
{
public:
    NRHeapWriteMaker(NorlsAllocator * heap) : allowingWrite(heap)
    {
        this->heap = heap;
        if (heap)
        {
            heap->MakeAllHeapWriteable();
        }
    }
    ~NRHeapWriteMaker()
    {
        if (heap && allowingWrite.OwnsHeapWriteability())
        {
            heap->AllowReadOnlyDirectives();
            heap->MakeAllHeapReadOnly();
        }
    }
private:
    NorlsAllocator * heap;
    NorlsAllocator::AllowingWrite allowingWrite;
};

class NRHeapWriteAllower
{
public:
    NRHeapWriteAllower(NorlsAllocator * heap) : allowingWrite(heap)
    {
        this->heap = heap;
        if (heap)
        {
            heap->DisallowReadOnlyDirectives();
        }
    }
    ~NRHeapWriteAllower()
    {
        if (heap && allowingWrite.OwnsHeapWriteability())
        {
            heap->AllowReadOnlyDirectives();
            heap->MakeAllHeapReadOnly();
        }
    }
private:
    NorlsAllocator * heap;
    NorlsAllocator::AllowingWrite allowingWrite;
};

