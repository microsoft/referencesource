//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  CHeapPtr for VB
//
//-------------------------------------------------------------------------------------------------

#pragma once

template <typename T>
class VBHeapPtr :
    public CHeapPtrBase<T, VBAllocator>
{
public:
    VBHeapPtr() throw() : CHeapPtrBase<T, VBAllocator>()
    {
    }
    VBHeapPtr(VBHeapPtr<T>& p) throw() :
        CHeapPtrBase<T, VBAllocator>(p)
    {
    }
    explicit VBHeapPtr(T* p) throw() :
        CHeapPtrBase<T, VBAllocator>(p)
    {
    }
    explicit VBHeapPtr(size_t elements)
    {
        Allocate(elements);
    }

    VBHeapPtr<T>& operator=(VBHeapPtr<T>& p) throw()
    {
        CHeapPtrBase<T, VBStandardallocator>::operator=(p);
        return *this;
    }

    // Allocate a buffer with the given number of elements.  Allocator
    // will succeed or throw
    void Allocate(size_t nElements = 1) throw()
    {
        SafeInt<size_t> total = nElements;
        total *= sizeof(T);

        // Uses VBAllocator::Allocate which will succeed or throw
        AllocateBytes(total.Value());
    }

    // Reallocate the buffer to hold a given number of elements. Allocator
    // will succeed or throw
    void Reallocate(size_t nElements) throw()
    {
        SafeInt<size_t> total = nElements;
        total *= sizeof(T);

        // Uses VBAllocator::Allocate which will succeed or throw
        ReallocateBytes(total.Value()); 
    }
};

