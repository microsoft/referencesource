/***
*malloc.h - declarations and definitions for memory allocation functions
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Contains the function declarations for memory allocation functions;
*       also defines manifest constants and types used by the heap routines.
*       [System V]
*
*       [Public]
*
****/

#pragma once

#ifndef _INC_MALLOC
#define _INC_MALLOC

#include <crtdefs.h>

/*
 * Currently, all MS C compilers for Win32 platforms default to 8 byte
 * alignment.
 */
#pragma pack(push,_CRT_PACKING)

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/* Maximum heap request the heap manager will attempt */

#ifdef _WIN64
#define _HEAP_MAXREQ    0xFFFFFFFFFFFFFFE0
#else  /* _WIN64 */
#define _HEAP_MAXREQ    0xFFFFFFE0
#endif  /* _WIN64 */

/* _STATIC_ASSERT is for enforcing boolean/integral conditions at compile time. */

#ifndef _STATIC_ASSERT
#define _STATIC_ASSERT(expr) typedef char __static_assert_t[ (expr) ]
#endif  /* _STATIC_ASSERT */

/* Constants for _heapchk/_heapset/_heapwalk routines */

#define _HEAPEMPTY      (-1)
#define _HEAPOK         (-2)
#define _HEAPBADBEGIN   (-3)
#define _HEAPBADNODE    (-4)
#define _HEAPEND        (-5)
#define _HEAPBADPTR     (-6)
#define _FREEENTRY      0
#define _USEDENTRY      1

#ifndef _HEAPINFO_DEFINED
typedef struct _heapinfo {
        int * _pentry;
        size_t _size;
        int _useflag;
        } _HEAPINFO;
#define _HEAPINFO_DEFINED
#endif  /* _HEAPINFO_DEFINED */

#define _mm_free(a)      _aligned_free(a)
#define _mm_malloc(a, b)    _aligned_malloc(a, b)

/* Function prototypes */

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma push_macro("calloc")
#pragma push_macro("free")
#pragma push_macro("malloc")
#pragma push_macro("realloc")
#pragma push_macro("_recalloc")
#pragma push_macro("_aligned_free")
#pragma push_macro("_aligned_malloc")
#pragma push_macro("_aligned_offset_malloc")
#pragma push_macro("_aligned_realloc")
#pragma push_macro("_aligned_recalloc")
#pragma push_macro("_aligned_offset_realloc")
#pragma push_macro("_aligned_offset_recalloc")
#pragma push_macro("_aligned_msize")
#pragma push_macro("_freea")
#undef calloc
#undef free
#undef malloc
#undef realloc
#undef _recalloc
#undef _aligned_free
#undef _aligned_malloc
#undef _aligned_offset_malloc
#undef _aligned_realloc
#undef _aligned_recalloc
#undef _aligned_offset_realloc
#undef _aligned_offset_recalloc
#undef _aligned_msize
#undef _freea
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

#ifndef _CRT_ALLOCATION_DEFINED
#define _CRT_ALLOCATION_DEFINED
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Count*_Size) _CRTIMP _CRT_JIT_INTRINSIC _CRTNOALIAS _CRTRESTRICT    void * __cdecl calloc(_In_ _CRT_GUARDOVERFLOW size_t _Count, _In_ _CRT_GUARDOVERFLOW size_t _Size);
_CRTIMP                     _CRTNOALIAS                                                                             void   __cdecl free(_Pre_maybenull_ _Post_invalid_ void * _Memory);
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size) _CRTIMP _CRT_JIT_INTRINSIC _CRTNOALIAS _CRTRESTRICT                              void * __cdecl malloc(_In_ _CRT_GUARDOVERFLOW size_t _Size);
_Success_(return!=0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) _CRTIMP _CRTNOALIAS _CRTRESTRICT                           void * __cdecl realloc(_Pre_maybenull_ _Post_invalid_ void * _Memory, _In_ _CRT_GUARDOVERFLOW size_t _NewSize);
_Success_(return!=0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Count*_Size) _CRTIMP _CRTNOALIAS _CRTRESTRICT                       void * __cdecl _recalloc(_Pre_maybenull_ _Post_invalid_ void * _Memory, _In_ _CRT_GUARDOVERFLOW size_t _Count, _In_ _CRT_GUARDOVERFLOW size_t _Size);
_CRTIMP                     _CRTNOALIAS                                                                             void   __cdecl _aligned_free(_Pre_maybenull_ _Post_invalid_ void * _Memory);
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size) _CRTIMP _CRTNOALIAS _CRTRESTRICT                              void * __cdecl _aligned_malloc(_In_ _CRT_GUARDOVERFLOW size_t _Size, _In_ size_t _Alignment);
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size) _CRTIMP _CRTNOALIAS _CRTRESTRICT                              void * __cdecl _aligned_offset_malloc(_In_ _CRT_GUARDOVERFLOW size_t _Size, _In_ size_t _Alignment, _In_ size_t _Offset);
_Success_(return!=0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) _CRTIMP _CRTNOALIAS _CRTRESTRICT                              void * __cdecl _aligned_realloc(_Pre_maybenull_ _Post_invalid_ void * _Memory, _In_ _CRT_GUARDOVERFLOW size_t _NewSize, _In_ size_t _Alignment);
_Success_(return!=0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Count*_Size) _CRTIMP _CRTNOALIAS _CRTRESTRICT                       void * __cdecl _aligned_recalloc(_Pre_maybenull_ _Post_invalid_ void * _Memory, _In_ _CRT_GUARDOVERFLOW size_t _Count, _In_ _CRT_GUARDOVERFLOW size_t _Size, _In_ size_t _Alignment);
_Success_(return!=0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) _CRTIMP _CRTNOALIAS _CRTRESTRICT                              void * __cdecl _aligned_offset_realloc(_Pre_maybenull_ _Post_invalid_ void * _Memory, _In_ _CRT_GUARDOVERFLOW size_t _NewSize, _In_ size_t _Alignment, _In_ size_t _Offset);
_Success_(return!=0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Count*_Size) _CRTIMP _CRTNOALIAS _CRTRESTRICT                       void * __cdecl _aligned_offset_recalloc(_Pre_maybenull_ _Post_invalid_ void * _Memory, _In_ _CRT_GUARDOVERFLOW size_t _Count, _In_ _CRT_GUARDOVERFLOW size_t _Size, _In_ size_t _Alignment, _In_ size_t _Offset);
_Check_return_ _CRTIMP                                                  size_t __cdecl _aligned_msize(_Pre_notnull_ void * _Memory, _In_ size_t _Alignment, _In_ size_t _Offset);
#endif  /* _CRT_ALLOCATION_DEFINED */

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma pop_macro("calloc")
#pragma pop_macro("free")
#pragma pop_macro("malloc")
#pragma pop_macro("realloc")
#pragma pop_macro("_recalloc")
#pragma pop_macro("_aligned_free")
#pragma pop_macro("_aligned_malloc")
#pragma pop_macro("_aligned_offset_malloc")
#pragma pop_macro("_aligned_realloc")
#pragma pop_macro("_aligned_recalloc")
#pragma pop_macro("_aligned_offset_realloc")
#pragma pop_macro("_aligned_offset_recalloc")
#pragma pop_macro("_aligned_msize")
#pragma pop_macro("_freea")
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP
_CRTIMP int     __cdecl _resetstkoflw (void);
#endif  /* _CRT_USE_WINAPI_FAMILY_DESKTOP_APP */

#define _MAX_WAIT_MALLOC_CRT 60000

_CRTIMP unsigned long __cdecl _set_malloc_crt_max_wait(_In_ unsigned long _NewValue);


#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma push_macro("_expand")
#pragma push_macro("_msize")
#undef _expand
#undef _msize
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) _CRTIMP void *  __cdecl _expand(_Pre_notnull_ void * _Memory, _In_ size_t _NewSize);
_Check_return_ _CRTIMP size_t  __cdecl _msize(_Pre_notnull_ void * _Memory);

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma pop_macro("_expand")
#pragma pop_macro("_msize")
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

_Ret_notnull_ _Post_writable_byte_size_(_Size) void *          __cdecl _alloca(_In_ size_t _Size);

#if defined (_DEBUG) || defined (_CRT_USE_WINAPI_FAMILY_DESKTOP_APP)
_CRTIMP _CRT_MANAGED_HEAP_DEPRECATE int     __cdecl _heapwalk(_Inout_ _HEAPINFO * _EntryInfo);
_CRTIMP intptr_t __cdecl _get_heap_handle(void);
#endif  /* defined (_DEBUG) || defined (_CRT_USE_WINAPI_FAMILY_DESKTOP_APP) */

#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP
_Check_return_ _CRTIMP int     __cdecl _heapadd(_In_ void * _Memory, _In_ size_t _Size);
_Check_return_ _CRTIMP int     __cdecl _heapchk(void);
_Check_return_ _CRTIMP int     __cdecl _heapmin(void);
_CRTIMP int     __cdecl _heapset(_In_ unsigned int _Fill);
_CRTIMP size_t  __cdecl _heapused(size_t * _Used, size_t * _Commit);
#endif  /* _CRT_USE_WINAPI_FAMILY_DESKTOP_APP */

#define _ALLOCA_S_THRESHOLD     1024
#define _ALLOCA_S_STACK_MARKER  0xCCCC
#define _ALLOCA_S_HEAP_MARKER   0xDDDD

#if defined (_M_IX86)
#define _ALLOCA_S_MARKER_SIZE   8
#elif defined (_M_X64)
#define _ALLOCA_S_MARKER_SIZE   16
#elif defined (_M_ARM)
#define _ALLOCA_S_MARKER_SIZE   8
#elif !defined (RC_INVOKED)
#error Unsupported target platform.
#endif  /* !defined (RC_INVOKED) */

_STATIC_ASSERT(sizeof(unsigned int) <= _ALLOCA_S_MARKER_SIZE);

#if !defined (__midl) && !defined (RC_INVOKED)
#pragma warning(push)
#pragma warning(disable:6540)
__inline void *_MarkAllocaS(_Out_opt_ __crt_typefix(unsigned int*) void *_Ptr, unsigned int _Marker)
{
    if (_Ptr)
    {
        *((unsigned int*)_Ptr) = _Marker;
        _Ptr = (char*)_Ptr + _ALLOCA_S_MARKER_SIZE;
    }
    return _Ptr;
}
#pragma warning(pop)
#endif  /* !defined (__midl) && !defined (RC_INVOKED) */

#if defined (_DEBUG)
#if !defined (_CRTDBG_MAP_ALLOC)
#undef _malloca
#define _malloca(size) \
__pragma(warning(suppress: 6255)) \
        _MarkAllocaS(malloc((size) + _ALLOCA_S_MARKER_SIZE), _ALLOCA_S_HEAP_MARKER)
#endif  /* !defined (_CRTDBG_MAP_ALLOC) */
#else  /* defined (_DEBUG) */
#undef _malloca
#define _malloca(size) \
__pragma(warning(suppress: 6255)) \
    ((((size) + _ALLOCA_S_MARKER_SIZE) <= _ALLOCA_S_THRESHOLD) ? \
        _MarkAllocaS(_alloca((size) + _ALLOCA_S_MARKER_SIZE), _ALLOCA_S_STACK_MARKER) : \
        _MarkAllocaS(malloc((size) + _ALLOCA_S_MARKER_SIZE), _ALLOCA_S_HEAP_MARKER))
#endif  /* defined (_DEBUG) */

#undef _FREEA_INLINE
#ifdef _CRTBLD
#ifndef _CRT_NOFREEA
#define _FREEA_INLINE
#else  /* _CRT_NOFREEA */
#undef _FREEA_INLINE
#endif  /* _CRT_NOFREEA */
#else  /* _CRTBLD */
#define _FREEA_INLINE
#endif  /* _CRTBLD */

#ifdef _FREEA_INLINE
/* _freea must be in the header so that its allocator matches _malloca */
#if !defined (__midl) && !defined (RC_INVOKED)
#if !(defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC))
#undef _freea
__pragma(warning(push))
__pragma(warning(disable: 6014))
_CRTNOALIAS __inline void __CRTDECL _freea(_Pre_maybenull_ _Post_invalid_ void * _Memory)
{
    unsigned int _Marker;
    if (_Memory)
    {
        _Memory = (char*)_Memory - _ALLOCA_S_MARKER_SIZE;
        _Marker = *(unsigned int *)_Memory;
        if (_Marker == _ALLOCA_S_HEAP_MARKER)
        {
            free(_Memory);
        }
#if defined (_ASSERTE)
        else if (_Marker != _ALLOCA_S_STACK_MARKER)
        {
            #pragma warning(suppress: 4548) /* expression before comma has no effect */
            _ASSERTE(("Corrupted pointer passed to _freea", 0));
        }
#endif  /* defined (_ASSERTE) */
    }
}
__pragma(warning(pop))
#endif  /* !(defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)) */
#endif  /* !defined (__midl) && !defined (RC_INVOKED) */
#endif  /* _FREEA_INLINE */

#if !__STDC__
/* Non-ANSI names for compatibility */
#define alloca  _alloca
#endif  /* !__STDC__ */




#ifdef __cplusplus
}
#endif  /* __cplusplus */

#pragma pack(pop)

#endif  /* _INC_MALLOC */
