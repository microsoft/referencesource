//============================================================================
// A virtual memory allocation wrapper.  This is used to make sure that we
// don't leak any of this memory.
//
// This allocator assumes that the memory it was allocated out of is zero
// initialized.
//============================================================================

#pragma once

struct BCVirtualAlloc
{
    //
    // Constructors/Destructors.
    //

    BCVirtualAlloc();
    ~BCVirtualAlloc();

    //
    // Helpers to make sure that we don't try to allocate bad sizes.
    //

    // Get the page size.
    static size_t CbPageSize()
    {
        return m_cbSizePage;
    }

    // Adjust the size of memory to reserve.
    static
    size_t CbSizeReserve(size_t cbSize)
    {
        // Make sure we always alloc something.
        cbSize = cbSize ? cbSize : 1;

        return(cbSize + m_cbAllocationGranularity - 1)
              & ~(m_cbAllocationGranularity - 1);
    }

    // Adjust the size of memory to allocate.
    static
    size_t CbSizeAlloc(size_t cbSize)
    {
        // Make sure we always alloc something.
        cbSize = cbSize ? cbSize : 1;
        return(cbSize + m_cbSizePage - 1) & ~(m_cbSizePage - 1);
    }

    // Minumum allocation granularity.
    static
    size_t CbAllocationGranularity()
    {
        return m_cbAllocationGranularity;
    }

    //
    // Address space manipulation methods.
    //

    // Reserve some address space to allocate in.
    void * ReserveAddressSpace(size_t cbSize);

    // Unreserve the space.
    void FreeAddressSpace(void * pv);

    // Commit memory inside of an address space.
    void CommitMemory(
        void * pv,
        size_t cbSize);

    // Uncommit the space.
    void DecommitMemory(
        void * pv,
        size_t cbSize);

    // Change the read-onlyness of a page of memory.
    void ProtectMemory(
        void * pv,
        size_t cbSize,
        DWORD dwPageAccess);

#if DEBUG
    // Reset the passcount on a chunk of memory.
    void DebMarkMemory(void * pv);
#else !DEBUG
    void DebMarkMemory(void * pv) 
    {
    }
#endif !DEBUG

private:

    // The following members are used to determine the size of
    // the pages we allocate.
    //
    static size_t m_cbSizePage;
    static size_t m_cbAllocationGranularity;

#if DEBUG

    unsigned m_passCount;

    // This structure holds the information about the allocated
    // memory spaces.
    //
    struct AddressSpace
    {
        AddressSpace *m_pNextSpace; // Next field in the linked list.

        void *m_pvAddressSpace;     // The address of the beginning of the space
        size_t m_cbAddressSpace;  // The size of the reserved space

        size_t m_iPassCount;      // Which allocation is this?
    };

    // List of allocated virutal spaces.
    AddressSpace *m_pAllocatedAddressSpaces;

#endif // DEBUG

};


/* this is an internal class that shouldn't be used directly.
  Use the TEMPBUF macro instead
*/
class TempBuffer {
public:
    TempBuffer(
        size_t stacksize,
        size_t size,
        void * stackbuffer)
    {
        if (size <= stacksize)
        {
          m_pcBuffer = stackbuffer;
          m_where_allocated = LOCAL;
        }
        else
        {
          m_pcBuffer = CoTaskMemAlloc(size);
          m_where_allocated = HEAP;
        }
    }

    ~TempBuffer()
    {
        if (m_where_allocated == HEAP) {
          CoTaskMemFree(m_pcBuffer);
        }
    }

    void * GetBuffer()
    {
        return m_pcBuffer;
    }

private:
    enum alloc_kind {LOCAL, HEAP};
    alloc_kind m_where_allocated;
    void* m_pcBuffer;
};

/* Macro to declare and efficiently manage a temporary buffer.
 * Var is the variable name
 * Type is its type
 * size is the actual (known at runtime) size of the buffer
 * Example:
 * TEMPBUF(tempchars, char*, 2*len(initchars));
 *
 * NOTE: you must check the pointer to the buffer for NULL in case
 *       the allocator wasn't able to allocate the memory.
*/
#pragma warning(disable:6304)//,"It is OK for address-of to be a NOP for temp buffers.")

#define TEMPBUF(var,type,size)                                          \
    char __TempBuf##var[120];                                           \
    TempBuffer __TempBuffer##var(120,(size),(char*)&__TempBuf##var);    \
    type var = (type)__TempBuffer##var.GetBuffer();

#pragma warning(default:6304)//,"It is OK for address-of to be a NOP for temp buffers.")



