//
// vcruntime_new.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Declarations and definitions of memory management functions in the VCRuntime.
//
#pragma once

#include <vcruntime.h>

#ifdef __cplusplus
extern "C++" {

#pragma pack(push, _CRT_PACKING)

#pragma warning(push)
#pragma warning(disable: 4985) // attributes not present on previous declaration

#pragma push_macro("new")
#undef new

#ifndef __NOTHROW_T_DEFINED
#define __NOTHROW_T_DEFINED
    namespace std
    {
        struct nothrow_t { };

        #ifdef _CRT_ENABLE_SELECTANY_NOTHROW
            extern __declspec(selectany) nothrow_t const nothrow;
        #else
            extern nothrow_t const nothrow;
        #endif
    }
#endif

_Ret_notnull_ _Post_writable_byte_size_(_Size)
_VCRT_ALLOCATOR void* __CRTDECL operator new(
    size_t _Size
    );

_Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(_Size)
_VCRT_ALLOCATOR void* __CRTDECL operator new(
    size_t                _Size,
    std::nothrow_t const&
    ) throw();

_Ret_notnull_ _Post_writable_byte_size_(_Size)
_VCRT_ALLOCATOR void* __CRTDECL operator new[](
    size_t _Size
    );

_Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(_Size)
_VCRT_ALLOCATOR void* __CRTDECL operator new[](
    size_t                _Size,
    std::nothrow_t const&
    ) throw();

void __CRTDECL operator delete(
    void* _Block
    ) throw();

void __CRTDECL operator delete(
    void* _Block,
    std::nothrow_t const&
    ) throw();

void __CRTDECL operator delete[](
    void* _Block
    ) throw();

void __CRTDECL operator delete[](
    void* _Block,
    std::nothrow_t const&
    ) throw();

void __CRTDECL operator delete(
    void*  _Block,
    size_t _Size
    ) throw();

void __CRTDECL operator delete[](
    void* _Block,
    size_t _Size
    ) throw();

#ifndef __PLACEMENT_NEW_INLINE
    #define __PLACEMENT_NEW_INLINE
    _Ret_notnull_ _Post_writable_byte_size_(_Size)
    inline void* __CRTDECL operator new(size_t _Size, _Writable_bytes_(_Size) void* _Where) throw()
    {
        (void)_Size;
        return _Where;
    }

    inline void __CRTDECL operator delete(void*, void*) throw()
    {
        return;
    }
#endif

#ifndef __PLACEMENT_VEC_NEW_INLINE
    #define __PLACEMENT_VEC_NEW_INLINE
    _Ret_notnull_ _Post_writable_byte_size_(_Size)
    inline void* __CRTDECL operator new[](size_t _Size, _Writable_bytes_(_Size) void* _Where) throw()
    {
        (void)_Size;
        return _Where;
    }

    inline void __CRTDECL operator delete[](void*, void*) throw()
    {
    }
#endif

#pragma pop_macro("new")

#pragma warning(pop)
#pragma pack(pop)

} // extern "C++"
#endif // __cplusplus
