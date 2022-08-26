//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Compatible with the CRT Allocator Pattern
//
//-------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------
//
// Compatible with the CRT Allocator Pattern
//

class VBAllocator
{
public:
    static 
    void* Reallocate(
        void* p, 
        size_t nBytes) throw()
    {
        return VBRealloc(p, nBytes);
    }

    static 
    void* Allocate(size_t nBytes) throw()
    {
        return VBAlloc(nBytes);
    }

    static 
    void Free(void* p) throw()
    {
        VBFree(p);
    }

    template <typename T>
    static 
    T* AllocateArray(size_t elementCount)
    {
        size_t total = VBMath::Multiply(elementCount, sizeof(T));
        return (T*)Allocate(total);
    }

    template <typename T>
    static 
    T* AllocateArray(
        size_t elementCount, 
        _Out_opt_ size_t *resultingSize)
    {
        size_t total = VBMath::Multiply(elementCount, sizeof(T));
        if ( resultingSize )
        {
            *resultingSize = total;
        }
        return (T*)Allocate(total);
    }

#if DEBUG
        
    static 
    void * ReallocInternal(
        HANDLE heap,
        void * pv,
        DWORD flags,
        SIZE_T byteSize,
        LPCSTR pszFile,
        UINT line,
        DWORD instance,
        LPCSTR pszExtra)
    {
        if (pv)
        {
            return VsDebugReallocInternal(heap, pv, flags, byteSize, pszFile, line, instance, pszExtra);
        }
        else
        {
            return VsDebugAllocInternal(heap, flags, byteSize, pszFile, line, instance, pszExtra);
        }
    }

#else 

    static 
    void * ReallocInternal(
        HANDLE heap,
        void * pv,
        DWORD flags,
        SIZE_T byteSize)
    {
        if (pv)
        {
            return HeapReAlloc(heap, flags, pv, byteSize);
        }
        else
        {
            return HeapAlloc(heap, flags, byteSize);
        }
    }   
#endif
};


