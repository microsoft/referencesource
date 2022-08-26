/***
*crtdbg.h - Supports debugging features of the C runtime library.
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Support CRT debugging features.
*
*       [Public]
*
****/

#pragma once

#include <crtdefs.h>

#ifndef _INC_CRTDBG
#define _INC_CRTDBG

#pragma pack(push,_CRT_PACKING)
#pragma push_macro("new")
#undef new

/* Define NULL here since we depend on it and for back-compat
*/
#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else  /* __cplusplus */
#define NULL    ((void *)0)
#endif  /* __cplusplus */
#endif  /* NULL */

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

 /****************************************************************************
 *
 * Debug Reporting
 *
 ***************************************************************************/

typedef void *_HFILE; /* file handle pointer */

#define _CRT_WARN           0
#define _CRT_ERROR          1
#define _CRT_ASSERT         2
#define _CRT_ERRCNT         3

#define _CRTDBG_MODE_FILE      0x1
#define _CRTDBG_MODE_DEBUG     0x2
#define _CRTDBG_MODE_WNDW      0x4
#define _CRTDBG_REPORT_MODE    -1

#if defined (_M_IX86)
#define _CRTDBG_INVALID_HFILE ((_HFILE)-1)
#define _CRTDBG_HFILE_ERROR   ((_HFILE)-2)
#define _CRTDBG_FILE_STDOUT   ((_HFILE)-4)
#define _CRTDBG_FILE_STDERR   ((_HFILE)-5)
#define _CRTDBG_REPORT_FILE   ((_HFILE)-6)
#else  /* defined (_M_IX86) */
#define _CRTDBG_INVALID_HFILE ((_HFILE)(__int64)-1)
#define _CRTDBG_HFILE_ERROR   ((_HFILE)(__int64)-2)
#define _CRTDBG_FILE_STDOUT   ((_HFILE)(__int64)-4)
#define _CRTDBG_FILE_STDERR   ((_HFILE)(__int64)-5)
#define _CRTDBG_REPORT_FILE   ((_HFILE)(__int64)-6)
#endif  /* defined (_M_IX86) */

#if !defined (_M_CEE_PURE)
typedef int (__cdecl * _CRT_REPORT_HOOK)(int, char *, int *);
typedef int (__cdecl * _CRT_REPORT_HOOKW)(int, wchar_t *, int *);
#else  /* !defined (_M_CEE_PURE) */
typedef int (__clrcall * _CRT_REPORT_HOOK)(int, char *, int *);
typedef int (__clrcall * _CRT_REPORT_HOOKW)(int, wchar_t *, int *);
#endif  /* !defined (_M_CEE_PURE) */

#if defined (_M_CEE)
typedef int (__clrcall *_CRT_REPORT_HOOK_M)(int, char *, int *);
typedef int (__clrcall *_CRT_REPORT_HOOKW_M)(int, wchar_t *, int *);
#endif  /* defined (_M_CEE) */

#define _CRT_RPTHOOK_INSTALL  0
#define _CRT_RPTHOOK_REMOVE   1

 /****************************************************************************
 *
 * Heap
 *
 ***************************************************************************/

 /****************************************************************************
 *
 * Client-defined allocation hook
 *
 ***************************************************************************/

#define _HOOK_ALLOC     1
#define _HOOK_REALLOC   2
#define _HOOK_FREE      3

#if !defined (_M_CEE_PURE)
typedef int (__cdecl * _CRT_ALLOC_HOOK)(int, void *, size_t, int, long, const unsigned char *, int);
#else  /* !defined (_M_CEE_PURE) */
typedef int (__clrcall * _CRT_ALLOC_HOOK)(int, void *, size_t, int, long, const unsigned char *, int);
#endif  /* !defined (_M_CEE_PURE) */
#if defined (_M_CEE)
typedef int (__clrcall * _CRT_ALLOC_HOOK_M)(int, void *, size_t, int, long, const unsigned char *, int);
#endif  /* defined (_M_CEE) */

 /****************************************************************************
 *
 * Memory management
 *
 ***************************************************************************/

/*
 * Bit values for _crtDbgFlag flag:
 *
 * These bitflags control debug heap behavior.
 */

#define _CRTDBG_ALLOC_MEM_DF        0x01  /* Turn on debug allocation */
#define _CRTDBG_DELAY_FREE_MEM_DF   0x02  /* Don't actually free memory */
#define _CRTDBG_CHECK_ALWAYS_DF     0x04  /* Check heap every alloc/dealloc */
#define _CRTDBG_RESERVED_DF         0x08  /* Reserved - do not use */
#define _CRTDBG_CHECK_CRT_DF        0x10  /* Leak check/diff CRT blocks */
#define _CRTDBG_LEAK_CHECK_DF       0x20  /* Leak check at program exit */

/*
 * Some bit values for _crtDbgFlag which correspond to frequencies for checking
 * the the heap.
 */
#define _CRTDBG_CHECK_EVERY_16_DF   0x00100000  /* check heap every 16 heap ops */
#define _CRTDBG_CHECK_EVERY_128_DF  0x00800000  /* check heap every 128 heap ops */
#define _CRTDBG_CHECK_EVERY_1024_DF 0x04000000  /* check heap every 1024 heap ops */
/*
We do not check the heap by default at this point because the cost was too high
for some applications. You can still turn this feature on manually.
*/
#define _CRTDBG_CHECK_DEFAULT_DF    0

#define _CRTDBG_REPORT_FLAG         -1    /* Query bitflag status */

#define _BLOCK_TYPE(block)          (block & 0xFFFF)
#define _BLOCK_SUBTYPE(block)       (block >> 16 & 0xFFFF)


 /****************************************************************************
 *
 * Memory state
 *
 ***************************************************************************/

/* Memory block identification */
#define _FREE_BLOCK      0
#define _NORMAL_BLOCK    1
#define _CRT_BLOCK       2
#define _IGNORE_BLOCK    3
#define _CLIENT_BLOCK    4
#define _MAX_BLOCKS      5

#if !defined (_M_CEE_PURE)
typedef void (__cdecl * _CRT_DUMP_CLIENT)(void *, size_t);
#else  /* !defined (_M_CEE_PURE) */
typedef void (__clrcall * _CRT_DUMP_CLIENT)(void *, size_t);
#endif  /* !defined (_M_CEE_PURE) */
#if defined (_M_CEE)
typedef void (__clrcall * _CRT_DUMP_CLIENT_M)(void *, size_t);
#endif  /* defined (_M_CEE) */

struct _CrtMemBlockHeader;
typedef struct _CrtMemState
{
        struct _CrtMemBlockHeader * pBlockHeader;
        size_t lCounts[_MAX_BLOCKS];
        size_t lSizes[_MAX_BLOCKS];
        size_t lHighWaterCount;
        size_t lTotalCount;
} _CrtMemState;


 /****************************************************************************
 *
 * Declarations, prototype and function-like macros
 *
 ***************************************************************************/

/* _STATIC_ASSERT is for enforcing boolean/integral conditions at compile time.
   Since it is purely a compile-time mechanism that generates no code, the check
   is left in even if _DEBUG is not defined. */

#ifndef _STATIC_ASSERT
#define _STATIC_ASSERT(expr) typedef char __static_assert_t[ (expr) ]
#endif  /* _STATIC_ASSERT */

#ifndef _DEBUG

 /****************************************************************************
 *
 * Debug OFF
 * Debug OFF
 * Debug OFF
 *
 ***************************************************************************/

/*  We allow our basic _ASSERT macros to be overridden by pre-existing definitions.
    This is not the ideal mechanism, but is helpful in some scenarios and helps avoid
    multiple definition problems */

#ifndef _ASSERT
#define _ASSERT(expr) ((void)0)
#endif  /* _ASSERT */

#ifndef _ASSERTE
#define _ASSERTE(expr) ((void)0)
#endif  /* _ASSERTE */

#ifndef _ASSERT_EXPR
#define _ASSERT_EXPR(expr, expr_str) ((void)0)
#endif  /* _ASSERT_EXPR */

#ifndef _ASSERT_BASE
#define _ASSERT_BASE _ASSERT_EXPR
#endif  /* _ASSERT_BASE */

#define _RPT0(rptno, msg)
#define _RPTW0(rptno, msg)

#define _RPT1(rptno, msg, arg1)
#define _RPTW1(rptno, msg, arg1)

#define _RPT2(rptno, msg, arg1, arg2)
#define _RPTW2(rptno, msg, arg1, arg2)

#define _RPT3(rptno, msg, arg1, arg2, arg3)
#define _RPTW3(rptno, msg, arg1, arg2, arg3)

#define _RPT4(rptno, msg, arg1, arg2, arg3, arg4)
#define _RPTW4(rptno, msg, arg1, arg2, arg3, arg4)

#define _RPT5(rptno, msg, arg1, arg2, arg3, arg4, arg5)
#define _RPTW5(rptno, msg, arg1, arg2, arg3, arg4, arg5)


#define _RPTF0(rptno, msg)
#define _RPTFW0(rptno, msg)

#define _RPTF1(rptno, msg, arg1)
#define _RPTFW1(rptno, msg, arg1)

#define _RPTF2(rptno, msg, arg1, arg2)
#define _RPTFW2(rptno, msg, arg1, arg2)

#define _RPTF3(rptno, msg, arg1, arg2, arg3)
#define _RPTFW3(rptno, msg, arg1, arg2, arg3)

#define _RPTF4(rptno, msg, arg1, arg2, arg3, arg4)
#define _RPTFW4(rptno, msg, arg1, arg2, arg3, arg4)

#define _RPTF5(rptno, msg, arg1, arg2, arg3, arg4, arg5)
#define _RPTFW5(rptno, msg, arg1, arg2, arg3, arg4, arg5)

#define _malloc_dbg(s, t, f, l)         malloc(s)
#define _calloc_dbg(c, s, t, f, l)      calloc(c, s)
#define _realloc_dbg(p, s, t, f, l)     realloc(p, s)
#define _recalloc_dbg(p, c, s, t, f, l) _recalloc(p, c, s)
#define _expand_dbg(p, s, t, f, l)      _expand(p, s)
#define _free_dbg(p, t)                 free(p)
#define _msize_dbg(p, t)                _msize(p)

#define _aligned_msize_dbg(p, a, o)                         _aligned_msize(p, a, o)
#define _aligned_malloc_dbg(s, a, f, l)                     _aligned_malloc(s, a)
#define _aligned_realloc_dbg(p, s, a, f, l)                 _aligned_realloc(p, s, a)
#define _aligned_recalloc_dbg(p, c, s, a, f, l)             _aligned_recalloc(p, c, s, a)
#define _aligned_free_dbg(p)                                            _aligned_free(p)
#define _aligned_offset_malloc_dbg(s, a, o, f, l)           _aligned_offset_malloc(s, a, o)
#define _aligned_offset_realloc_dbg(p, s, a, o, f, l)       _aligned_offset_realloc(p, s, a, o)
#define _aligned_offset_recalloc_dbg(p, c, s, a, o, f, l)   _aligned_offset_recalloc(p, c, s, a, o)

#define _malloca_dbg(s, t, f, l)        _malloca(s)
#define _freea_dbg(p, t)                _freea(p)

#define _strdup_dbg(s, t, f, l)         _strdup(s)
#define _wcsdup_dbg(s, t, f, l)         _wcsdup(s)
#define _mbsdup_dbg(s, t, f, l)         _mbsdup(s)
#define _tempnam_dbg(s1, s2, t, f, l)   _tempnam(s1, s2)
#define _wtempnam_dbg(s1, s2, t, f, l)  _wtempnam(s1, s2)
#define _fullpath_dbg(s1, s2, le, t, f, l)  _fullpath(s1, s2, le)
#define _wfullpath_dbg(s1, s2, le, t, f, l) _wfullpath(s1, s2, le)
#define _getcwd_dbg(s, le, t, f, l)     _getcwd(s, le)
#define _wgetcwd_dbg(s, le, t, f, l)    _wgetcwd(s, le)
#define _getdcwd_dbg(d, s, le, t, f, l)     _getdcwd(d, s, le)
#define _wgetdcwd_dbg(d, s, le, t, f, l)    _wgetdcwd(d, s, le)
#define _getdcwd_lk_dbg(d, s, le, t, f, l)  _getdcwd(d, s, le)
#define _wgetdcwd_lk_dbg(d, s, le, t, f, l) _wgetdcwd(d, s, le)
#define _dupenv_s_dbg(ps1, size, s2, t, f, l)  _dupenv_s(ps1, size, s2)
#define _wdupenv_s_dbg(ps1, size, s2, t, f, l) _wdupenv_s(ps1, size, s2)

#define _CrtSetReportHook(f)                ((_CRT_REPORT_HOOK)0)
#define _CrtGetReportHook()                 ((_CRT_REPORT_HOOK)0)
#define _CrtSetReportHook2(t, f)            ((int)0)
#define _CrtSetReportHookW2(t, f)           ((int)0)
#define _CrtSetReportMode(t, f)             ((int)0)
#define _CrtSetReportFile(t, f)             ((_HFILE)0)

#define _CrtDbgBreak()                      ((void)0)

#define _CrtSetBreakAlloc(a)                ((long)0)

#define _CrtSetAllocHook(f)                 ((_CRT_ALLOC_HOOK)0)
#define _CrtGetAllocHook()                  ((_CRT_ALLOC_HOOK)0)

#define _CrtCheckMemory()                   ((int)1)
#define _CrtSetDbgFlag(f)                   ((int)0)
#define _CrtDoForAllClientObjects(f, c)     ((void)0)
#define _CrtIsValidPointer(p, n, r)         ((int)1)
#define _CrtIsValidHeapPointer(p)           ((int)1)
#define _CrtIsMemoryBlock(p, t, r, f, l)    ((int)1)
#define _CrtReportBlockType(p)              ((int)-1)

#define _CrtSetDumpClient(f)                ((_CRT_DUMP_CLIENT)0)
#define _CrtGetDumpClient()                 ((_CRT_DUMP_CLIENT)0)

#define _CrtMemCheckpoint(s)                ((void)0)
#define _CrtMemDifference(s1, s2, s3)       ((int)0)
#define _CrtMemDumpAllObjectsSince(s)       ((void)0)
#define _CrtMemDumpStatistics(s)            ((void)0)
#define _CrtDumpMemoryLeaks()               ((int)0)
#define _CrtSetDebugFillThreshold(t)        ((size_t)0)

#define _CrtSetCheckCount(f)                ((int)0)
#define _CrtGetCheckCount()                 ((int)0)

#else  /* _DEBUG */


 /****************************************************************************
 *
 * Debug ON
 * Debug ON
 * Debug ON
 *
 ***************************************************************************/


/* Define _MRTIMP */

#ifndef _MRTIMP
#if defined(MRTDLL) && defined(_CRTBLD)
#define _MRTIMP __declspec(dllexport)
#else  /* defined(MRTDLL) && defined(_CRTBLD) */
#define _MRTIMP __declspec(dllimport)
#endif  /* defined(MRTDLL) && defined(_CRTBLD) */
#endif  /* _MRTIMP */

/* Define _CRTIMP */

#ifndef _CRTIMP
#if defined(CRTDLL) && defined(_CRTBLD)
#define _CRTIMP __declspec(dllexport)
#else  /* defined(CRTDLL) && defined(_CRTBLD) */
#ifdef _DLL
#define _CRTIMP __declspec(dllimport)
#else  /* _DLL */
#define _CRTIMP
#endif  /* _DLL */
#endif  /* defined(CRTDLL) && defined(_CRTBLD) */
#endif  /* _CRTIMP */

 /****************************************************************************
 *
 * Debug Reporting
 *
 ***************************************************************************/

#ifdef _CRTBLD
#if defined (_DLL) && defined (_M_IX86)
/* Retained for compatibility with VC++ 5.0 and earlier versions */
_CRTIMP long * __cdecl __p__crtAssertBusy(void);
#endif  /* defined (_DLL) && defined (_M_IX86) */
#endif  /* _CRTBLD */

#if !defined (_M_CEE_PURE)
_CRTIMP extern long _crtAssertBusy;
#endif  /* !defined (_M_CEE_PURE) */

#if !defined (_M_CEE_PURE)
_CRTIMP _CRT_REPORT_HOOK __cdecl _CrtGetReportHook(
    void
    );
#endif  /* !defined (_M_CEE_PURE) */

/* _CrtSetReportHook[[W]2]:
 * For IJW, we need 2 versions: 1 for clrcall and one for cdecl.
 * For pure and native, we just need clrcall and cdecl, respectively.
 */
#if !defined (_M_CEE_PURE)
_CRTIMP _CRT_REPORT_HOOK __cdecl _CrtSetReportHook(
    _In_opt_ _CRT_REPORT_HOOK _PFnNewHook
        );

_CRTIMP int __cdecl _CrtSetReportHook2(
        _In_ int _Mode,
        _In_opt_ _CRT_REPORT_HOOK _PFnNewHook
        );

_CRTIMP int __cdecl _CrtSetReportHookW2(
        _In_ int _Mode,
        _In_opt_ _CRT_REPORT_HOOKW _PFnNewHook
        );
#endif  /* !defined (_M_CEE_PURE) */

_CRTIMP int __cdecl _CrtSetReportMode(
        _In_ int _ReportType,
        _In_ int _ReportMode
        );

_CRTIMP _HFILE __cdecl _CrtSetReportFile(
        _In_ int _ReportType,
        _In_opt_ _HFILE _ReportFile
        );

_CRTIMP int __cdecl _CrtDbgReport(
        _In_ int _ReportType,
        _In_opt_z_ const char * _Filename,
        _In_ int _Linenumber,
        _In_opt_z_ const char * _ModuleName,
        _In_opt_z_ const char * _Format,
        ...);

_CRTIMP size_t __cdecl _CrtSetDebugFillThreshold(
        _In_ size_t _NewDebugFillThreshold
        );

#if !defined (_NATIVE_WCHAR_T_DEFINED) && defined (_M_CEE_PURE)
extern "C++"
#endif  /* !defined (_NATIVE_WCHAR_T_DEFINED) && defined (_M_CEE_PURE) */
_CRTIMP int __cdecl _CrtDbgReportW(
        _In_ int _ReportType,
        _In_opt_z_ const wchar_t * _Filename,
        _In_ int _LineNumber,
        _In_opt_z_ const wchar_t * _ModuleName,
        _In_opt_z_ const wchar_t * _Format,
        ...);

/* Asserts */
/* We use !! below to ensure that any overloaded operators used to evaluate expr do not end up at operator || */
#define _ASSERT_EXPR(expr, msg) \
        (void) ((!!(expr)) || \
                (1 != _CrtDbgReportW(_CRT_ASSERT, _CRT_WIDE(__FILE__), __LINE__, NULL, L"%s", msg)) || \
                (_CrtDbgBreak(), 0))

#ifndef _ASSERT
#define _ASSERT(expr)   _ASSERT_EXPR((expr), NULL)
#endif  /* _ASSERT */

#ifndef _ASSERTE
#define _ASSERTE(expr)  _ASSERT_EXPR((expr), _CRT_WIDE(#expr))
#endif  /* _ASSERTE */

/*
We retain _ASSERT_BASE solely for backwards compatibility with those who used it even though they
should not have done so since it was not documented.
*/
#ifndef _ASSERT_BASE
#define _ASSERT_BASE _ASSERT_EXPR
#endif  /* _ASSERT_BASE */

/* Reports with no file/line info */

#define _RPT_BASE(args) \
        (void) ((1 != _CrtDbgReport args) || \
                (_CrtDbgBreak(), 0))

#define _RPT_BASE_W(args) \
        (void) ((1 != _CrtDbgReportW args) || \
                (_CrtDbgBreak(), 0))

#define _RPT0(rptno, msg) \
        _RPT_BASE((rptno, NULL, 0, NULL, "%s", msg))

#define _RPTW0(rptno, msg) \
        _RPT_BASE_W((rptno, NULL, 0, NULL, L"%s", msg))

#define _RPT1(rptno, msg, arg1) \
        _RPT_BASE((rptno, NULL, 0, NULL, msg, arg1))

#define _RPTW1(rptno, msg, arg1) \
        _RPT_BASE_W((rptno, NULL, 0, NULL, msg, arg1))

#define _RPT2(rptno, msg, arg1, arg2) \
        _RPT_BASE((rptno, NULL, 0, NULL, msg, arg1, arg2))

#define _RPTW2(rptno, msg, arg1, arg2) \
        _RPT_BASE_W((rptno, NULL, 0, NULL, msg, arg1, arg2))

#define _RPT3(rptno, msg, arg1, arg2, arg3) \
        _RPT_BASE((rptno, NULL, 0, NULL, msg, arg1, arg2, arg3))

#define _RPTW3(rptno, msg, arg1, arg2, arg3) \
        _RPT_BASE_W((rptno, NULL, 0, NULL, msg, arg1, arg2, arg3))

#define _RPT4(rptno, msg, arg1, arg2, arg3, arg4) \
        _RPT_BASE((rptno, NULL, 0, NULL, msg, arg1, arg2, arg3, arg4))

#define _RPTW4(rptno, msg, arg1, arg2, arg3, arg4) \
        _RPT_BASE_W((rptno, NULL, 0, NULL, msg, arg1, arg2, arg3, arg4))

#define _RPT5(rptno, msg, arg1, arg2, arg3, arg4, arg5) \
        _RPT_BASE((rptno, NULL, 0, NULL, msg, arg1, arg2, arg3, arg4, arg5))

#define _RPTW5(rptno, msg, arg1, arg2, arg3, arg4, arg5) \
        _RPT_BASE_W((rptno, NULL, 0, NULL, msg, arg1, arg2, arg3, arg4, arg5))

/* Reports with file/line info */

#define _RPTF0(rptno, msg) \
        _RPT_BASE((rptno, __FILE__, __LINE__, NULL, "%s", msg))

#define _RPTFW0(rptno, msg) \
        _RPT_BASE_W((rptno, _CRT_WIDE(__FILE__), __LINE__, NULL, L"%s", msg))

#define _RPTF1(rptno, msg, arg1) \
        _RPT_BASE((rptno, __FILE__, __LINE__, NULL, msg, arg1))

#define _RPTFW1(rptno, msg, arg1) \
        _RPT_BASE_W((rptno, _CRT_WIDE(__FILE__), __LINE__, NULL, msg, arg1))

#define _RPTF2(rptno, msg, arg1, arg2) \
        _RPT_BASE((rptno, __FILE__, __LINE__, NULL, msg, arg1, arg2))

#define _RPTFW2(rptno, msg, arg1, arg2) \
        _RPT_BASE_W((rptno, _CRT_WIDE(__FILE__), __LINE__, NULL, msg, arg1, arg2))

#define _RPTF3(rptno, msg, arg1, arg2, arg3) \
        _RPT_BASE((rptno, __FILE__, __LINE__, NULL, msg, arg1, arg2, arg3))

#define _RPTFW3(rptno, msg, arg1, arg2, arg3) \
        _RPT_BASE_W((rptno, _CRT_WIDE(__FILE__), __LINE__, NULL, msg, arg1, arg2, arg3))

#define _RPTF4(rptno, msg, arg1, arg2, arg3, arg4) \
        _RPT_BASE((rptno, __FILE__, __LINE__, NULL, msg, arg1, arg2, arg3, arg4))

#define _RPTFW4(rptno, msg, arg1, arg2, arg3, arg4) \
        _RPT_BASE_W((rptno, _CRT_WIDE(__FILE__), __LINE__, NULL, msg, arg1, arg2, arg3, arg4))

#define _RPTF5(rptno, msg, arg1, arg2, arg3, arg4, arg5) \
        _RPT_BASE((rptno, __FILE__, __LINE__, NULL, msg, arg1, arg2, arg3, arg4, arg5))

#define _RPTFW5(rptno, msg, arg1, arg2, arg3, arg4, arg5) \
        _RPT_BASE_W((rptno, _CRT_WIDE(__FILE__), __LINE__, NULL, msg, arg1, arg2, arg3, arg4, arg5))

#if !defined (_CRT_PORTABLE)
#define _CrtDbgBreak() __debugbreak()
#else  /* !defined (_CRT_PORTABLE) */
_CRTIMP void __cdecl _CrtDbgBreak(
        void
        );
#endif  /* !defined (_CRT_PORTABLE) */

 /****************************************************************************
 *
 * Heap routines
 *
 ***************************************************************************/

#ifdef _CRTDBG_MAP_ALLOC

#define   malloc(s)             _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   calloc(c, s)          _calloc_dbg(c, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   realloc(p, s)         _realloc_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   _recalloc(p, c, s)    _recalloc_dbg(p, c, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   _expand(p, s)         _expand_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   free(p)               _free_dbg(p, _NORMAL_BLOCK)
#define   _msize(p)             _msize_dbg(p, _NORMAL_BLOCK)
#define   _aligned_msize(p, a, o)                   _aligned_msize_dbg(p, a, o)
#define   _aligned_malloc(s, a)                     _aligned_malloc_dbg(s, a, __FILE__, __LINE__)
#define   _aligned_realloc(p, s, a)                 _aligned_realloc_dbg(p, s, a, __FILE__, __LINE__)
#define   _aligned_recalloc(p, c, s, a)             _aligned_recalloc_dbg(p, c, s, a, __FILE__, __LINE__)
#define   _aligned_offset_malloc(s, a, o)           _aligned_offset_malloc_dbg(s, a, o, __FILE__, __LINE__)
#define   _aligned_offset_realloc(p, s, a, o)       _aligned_offset_realloc_dbg(p, s, a, o, __FILE__, __LINE__)
#define   _aligned_offset_recalloc(p, c, s, a, o)   _aligned_offset_recalloc_dbg(p, c, s, a, o, __FILE__, __LINE__)
#define   _aligned_free(p)  _aligned_free_dbg(p)

#define   _malloca(s)        _malloca_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   _freea(p)          _freea_dbg(p, _NORMAL_BLOCK)

#define   _strdup(s)         _strdup_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   _wcsdup(s)         _wcsdup_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   _mbsdup(s)         _strdup_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   _tempnam(s1, s2)   _tempnam_dbg(s1, s2, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   _wtempnam(s1, s2)  _wtempnam_dbg(s1, s2, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   _fullpath(s1, s2, le)     _fullpath_dbg(s1, s2, le, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   _wfullpath(s1, s2, le)    _wfullpath_dbg(s1, s2, le, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   _getcwd(s, le)      _getcwd_dbg(s, le, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   _wgetcwd(s, le)     _wgetcwd_dbg(s, le, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   _getdcwd(d, s, le)  _getdcwd_dbg(d, s, le, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   _wgetdcwd(d, s, le) _wgetdcwd_dbg(d, s, le, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   _dupenv_s(ps1, size, s2)      _dupenv_s_dbg(ps1, size, s2, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   _wdupenv_s(ps1, size, s2)     _wdupenv_s_dbg(ps1, size, s2, _NORMAL_BLOCK, __FILE__, __LINE__)

#if !__STDC__
#define   strdup(s)          _strdup_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   wcsdup(s)          _wcsdup_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   tempnam(s1, s2)    _tempnam_dbg(s1, s2, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   getcwd(s, le)      _getcwd_dbg(s, le, _NORMAL_BLOCK, __FILE__, __LINE__)
#endif  /* !__STDC__ */


#endif  /* _CRTDBG_MAP_ALLOC */

#ifdef _CRTBLD
#if defined (_DLL) && defined (_M_IX86)
/* Retained for compatibility with VC++ 5.0 and earlier versions */
_CRTIMP long * __cdecl __p__crtBreakAlloc(void);
#endif  /* defined (_DLL) && defined (_M_IX86) */
#endif  /* _CRTBLD */

#if !defined (_M_CEE_PURE)
_CRTIMP extern long _crtBreakAlloc;      /* Break on this allocation */
#endif  /* !defined (_M_CEE_PURE) */

_CRTIMP long __cdecl _CrtSetBreakAlloc(
        _In_ long _BreakAlloc
        );

/*
 * Prototypes for malloc, free, realloc, etc are in malloc.h
 */

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size) _CRTIMP void * __cdecl _malloc_dbg(
        _In_ size_t _Size,
        _In_ int _BlockType,
        _In_opt_z_ const char * _Filename,
        _In_ int _LineNumber
        );

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Count*_Size) _CRTIMP void * __cdecl _calloc_dbg(
        _In_ size_t _Count,
        _In_ size_t _Size,
        _In_ int _BlockType,
        _In_opt_z_ const char * _Filename,
        _In_ int _LineNumber
        );

_Success_(return!=0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) _CRTIMP void * __cdecl _realloc_dbg(
        _Pre_maybenull_ _Post_invalid_ void * _Memory,
        _In_ size_t _NewSize,
        _In_ int _BlockType,
        _In_opt_z_ const char * _Filename,
        _In_ int _LineNumber
        );

_Success_(return!=0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NumOfElements*_SizeOfElements) _CRTIMP void * __cdecl _recalloc_dbg
(
        _Pre_maybenull_ _Post_invalid_ void * _Memory,
        _In_ size_t _NumOfElements,
        _In_ size_t _SizeOfElements,
        _In_ int _BlockType,
        _In_opt_z_ const char * _Filename,
        _In_ int _LineNumber
);

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) _CRTIMP void * __cdecl _expand_dbg(
        _Pre_notnull_ void * _Memory,
        _In_ size_t _NewSize,
        _In_ int _BlockType,
        _In_opt_z_ const char * _Filename,
        _In_ int _LineNumber
        );

_CRTIMP void __cdecl _free_dbg(
        _Pre_maybenull_ _Post_invalid_ void * _Memory,
        _In_ int _BlockType
        );

_CRTIMP size_t __cdecl _msize_dbg (
        _Pre_notnull_ void * _Memory,
        _In_ int _BlockType
        );

_CRTIMP size_t __cdecl _aligned_msize_dbg (
        _Pre_notnull_ void * _Memory,
        _In_ size_t _Alignment,
        _In_ size_t _Offset
        );

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size) _CRTIMP void * __cdecl _aligned_malloc_dbg(
        _In_ size_t _Size,
        _In_ size_t _Alignment,
        _In_opt_z_ const char * _Filename,
        _In_ int _LineNumber
        );

_Success_(return!=0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) _CRTIMP void * __cdecl _aligned_realloc_dbg(
        _Pre_maybenull_ _Post_invalid_ void * _Memory,
        _In_ size_t _NewSize,
        _In_ size_t _Alignment,
        _In_opt_z_ const char * _Filename,
        _In_ int _LineNumber
        );

_Success_(return!=0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NumOfElements*_SizeOfElements) _CRTIMP void * __cdecl _aligned_recalloc_dbg
(
        _Pre_maybenull_ _Post_invalid_ void * _Memory,
        _In_ size_t _NumOfElements,
        _In_ size_t _SizeOfElements,
        _In_ size_t _Alignment,
        _In_opt_z_ const char * _Filename,
        _In_ int _LineNumber
);

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size) _CRTIMP void * __cdecl _aligned_offset_malloc_dbg(
        _In_ size_t _Size,
        _In_ size_t _Alignment,
        _In_ size_t _Offset,
        _In_opt_z_ const char * _Filename,
        _In_ int _LineNumber
        );

_Success_(return!=0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NewSize) _CRTIMP void * __cdecl _aligned_offset_realloc_dbg(
        _Pre_maybenull_ _Post_invalid_ void * _Memory,
        _In_ size_t _NewSize,
        _In_ size_t _Alignment,
        _In_ size_t _Offset,
        _In_opt_z_ const char * _Filename,
        _In_ int _LineNumber
        );

_Success_(return!=0)
_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_NumOfElements*_SizeOfElements) _CRTIMP void * __cdecl _aligned_offset_recalloc_dbg
(
        _Pre_maybenull_ _Post_invalid_ void * _Memory,
        _In_ size_t _NumOfElements,
        _In_ size_t _SizeOfElements,
        _In_ size_t _Alignment,
        _In_ size_t _Offset,
        _In_opt_z_ const char * _Filename,
        _In_ int _LineNumber
);

_CRTIMP void __cdecl _aligned_free_dbg(
        _Pre_maybenull_ _Post_invalid_ void * _Memory
        );

_Check_return_ _Ret_maybenull_z_ _CRTIMP char * __cdecl _strdup_dbg(
        _In_opt_z_ const char * _Str,
        _In_ int _BlockType,
        _In_opt_z_ const char * _Filename,
        _In_ int _LineNumber
        );

_Check_return_ _Ret_maybenull_z_ _CRTIMP wchar_t * __cdecl _wcsdup_dbg(
        _In_opt_z_ const wchar_t * _Str,
        _In_ int _BlockType,
        _In_opt_z_ const char * _Filename,
        _In_ int _LineNumber
        );

_Check_return_ _Ret_maybenull_z_ _CRTIMP char * __cdecl _tempnam_dbg(
        _In_opt_z_ const char * _DirName,
        _In_opt_z_ const char * _FilePrefix,
        _In_ int _BlockType,
        _In_opt_z_ const char * _Filename,
        _In_ int _LineNumber
        );

_Check_return_ _Ret_maybenull_z_ _CRTIMP wchar_t * __cdecl _wtempnam_dbg(
        _In_opt_z_ const wchar_t * _DirName,
        _In_opt_z_ const wchar_t * _FilePrefix,
        _In_ int _BlockType,
        _In_opt_z_ const char * _Filename,
        _In_ int _LineNumber
        );

_Check_return_ _Ret_maybenull_z_ _CRTIMP char * __cdecl _fullpath_dbg(
        _Out_writes_opt_z_(_SizeInBytes) char * _FullPath,
        _In_z_ const char * _Path,
        _In_ size_t _SizeInBytes,
        _In_ int _BlockType,
        _In_opt_z_ const char * _Filename,
        _In_ int _LineNumber
        );

_Check_return_ _Ret_maybenull_z_ _CRTIMP wchar_t * __cdecl _wfullpath_dbg(
        _Out_writes_opt_z_(_SizeInWords) wchar_t * _FullPath,
        _In_z_ const wchar_t * _Path,
        _In_ size_t _SizeInWords,
        _In_ int _BlockType,
        _In_opt_z_ const char * _Filename,
        _In_ int _LineNumber
        );

_Check_return_ _Ret_maybenull_z_ _CRTIMP char * __cdecl _getcwd_dbg(
        _Out_writes_opt_z_(_SizeInBytes) char * _DstBuf,
        _In_ int _SizeInBytes,
        _In_ int _BlockType,
        _In_opt_z_ const char * _Filename,
        _In_ int _LineNumber
        );

_Check_return_ _Ret_maybenull_z_ _CRTIMP wchar_t * __cdecl _wgetcwd_dbg(
        _Out_writes_opt_z_(_SizeInWords) wchar_t * _DstBuf,
        _In_ int _SizeInWords,
        _In_ int _BlockType,
        _In_opt_z_ const char * _Filename,
        _In_ int _LineNumber
        );

_Check_return_ _Ret_maybenull_z_ _CRTIMP char * __cdecl _getdcwd_dbg(
        _In_ int _Drive,
        _Out_writes_opt_z_(_SizeInBytes) char * _DstBuf,
        _In_ int _SizeInBytes,
        _In_ int _BlockType,
        _In_opt_z_ const char * _Filename,
        _In_ int _LineNumber
        );

_Check_return_ _Ret_maybenull_z_ _CRTIMP wchar_t * __cdecl _wgetdcwd_dbg(
        _In_ int _Drive,
        _Out_writes_opt_z_(_SizeInWords) wchar_t * _DstBuf,
        _In_ int _SizeInWords,
        _In_ int _BlockType,
        _In_opt_z_ const char * _Filename,
        _In_ int _LineNumber
        );

_Check_return_ _Ret_maybenull_z_ char * __cdecl _getdcwd_lk_dbg(
        _In_ int _Drive,
        _Out_writes_opt_z_(_SizeInBytes) char * _DstBuf,
        _In_ int _SizeInBytes,
        _In_ int _BlockType,
        _In_opt_z_ const char * _Filename,
        _In_ int _LineNumber
        );

_Check_return_ _Ret_maybenull_z_ wchar_t * __cdecl _wgetdcwd_lk_dbg(
        _In_ int _Drive,
        _Out_writes_opt_z_(_SizeInWords) wchar_t * _DstBuf,
        _In_ int _SizeInWords,
        _In_ int _BlockType,
        _In_opt_z_ const char * _Filename,
        _In_ int _LineNumber
        );

_Check_return_wat_ _CRTIMP errno_t __cdecl _dupenv_s_dbg(
        _Outptr_result_buffer_maybenull_(*_PBufferSizeInBytes) _Outptr_result_z_ char ** _PBuffer,
        _Out_opt_ size_t * _PBufferSizeInBytes,
        _In_z_ const char * _VarName,
        _In_ int _BlockType,
        _In_opt_z_ const char * _Filename,
        _In_ int _LineNumber
        );

_Check_return_wat_ _CRTIMP errno_t __cdecl _wdupenv_s_dbg(
        _Outptr_result_buffer_maybenull_(*_PBufferSizeInWords) _Outptr_result_z_ wchar_t ** _PBuffer,
        _Out_opt_ size_t * _PBufferSizeInWords,
        _In_z_ const wchar_t * _VarName,
        _In_ int _BlockType,
        _In_opt_z_ const char * _Filename,
        _In_ int _LineNumber
        );

#define _malloca_dbg(s, t, f, l)    _malloc_dbg(s, t, f, l)
#define _freea_dbg(p, t)            _free_dbg(p, t)

#if defined (__cplusplus) && defined (_CRTDBG_MAP_ALLOC)
namespace std
{
        using ::_calloc_dbg; using ::_free_dbg; using ::_malloc_dbg; using ::_realloc_dbg;
}
#endif  /* defined (__cplusplus) && defined (_CRTDBG_MAP_ALLOC) */

 /****************************************************************************
 *
 * Client-defined allocation hook
 *
 ***************************************************************************/
#if !defined (_M_CEE_PURE)
_CRTIMP _CRT_ALLOC_HOOK __cdecl _CrtGetAllocHook
(
    void
);
#endif  /* !defined (_M_CEE_PURE) */

/* _CrtSetAllocHook:
 * For IJW, we need 2 versions: 1 for clrcall and one for cdecl.
 * For pure and native, we just need clrcall and cdecl, respectively.
 */
#if !defined (_M_CEE_PURE)
_CRTIMP _CRT_ALLOC_HOOK __cdecl _CrtSetAllocHook
(
    _In_opt_ _CRT_ALLOC_HOOK _PfnNewHook
);
#endif  /* !defined (_M_CEE_PURE) */

 /****************************************************************************
 *
 * Memory management
 *
 ***************************************************************************/

/*
 * Bitfield flag that controls CRT heap behavior
 * Default setting is _CRTDBG_ALLOC_MEM_DF
 */

#ifdef _CRTBLD
#if defined (_DLL) && defined (_M_IX86)
/* Retained for compatibility with VC++ 5.0 and earlier versions */
_CRTIMP int * __cdecl __p__crtDbgFlag(void);
#endif  /* defined (_DLL) && defined (_M_IX86) */
#endif  /* _CRTBLD */

#if !defined (_M_CEE_PURE)
_CRTIMP extern int _crtDbgFlag;
#endif  /* !defined (_M_CEE_PURE) */

_CRTIMP int __cdecl _CrtCheckMemory(
        void
        );

_CRTIMP int __cdecl _CrtSetDbgFlag(
        _In_ int _NewFlag
        );

_CRTIMP void __cdecl _CrtDoForAllClientObjects(
        _In_ void (__cdecl *_PFn)(void *, void *),
        void * _Context
        );

_Check_return_ _CRTIMP int __cdecl _CrtIsValidPointer(
        _In_opt_ const void * _Ptr,
        _In_ unsigned int _Bytes,
        _In_ int _ReadWrite
        );

_Check_return_ _CRTIMP int __cdecl _CrtIsValidHeapPointer(
        _In_opt_ const void * _HeapPtr
        );

_CRTIMP int __cdecl _CrtIsMemoryBlock(
        _In_opt_ const void * _Memory,
        _In_ unsigned int _Bytes,
        _Out_opt_ long * _RequestNumber,
        _Out_opt_ char ** _Filename,
        _Out_opt_ int * _LineNumber
        );

_Check_return_ _CRTIMP int __cdecl _CrtReportBlockType(
        _In_opt_ const void * _Memory
        );


 /****************************************************************************
 *
 * Memory state
 *
 ***************************************************************************/

#if !defined (_M_CEE_PURE)
_CRTIMP _CRT_DUMP_CLIENT __cdecl _CrtGetDumpClient
(
    void
);
#endif  /* !defined (_M_CEE_PURE) */

/* _CrtSetDumpClient:
 * For IJW, we need 2 versions: 1 for clrcall and one for cdecl.
 * For pure and native, we just need clrcall and cdecl, respectively.
 */
#if !defined (_M_CEE_PURE)
_CRTIMP _CRT_DUMP_CLIENT __cdecl _CrtSetDumpClient
(
    _In_opt_ _CRT_DUMP_CLIENT _PFnNewDump
);
#endif  /* !defined (_M_CEE_PURE) */

_CRTIMP _CRT_MANAGED_HEAP_DEPRECATE void __cdecl _CrtMemCheckpoint(
        _Out_ _CrtMemState * _State
        );

_CRTIMP _CRT_MANAGED_HEAP_DEPRECATE int __cdecl _CrtMemDifference(
        _Out_ _CrtMemState * _State,
        _In_ const _CrtMemState * _OldState,
        _In_ const _CrtMemState * _NewState
        );

_CRTIMP void __cdecl _CrtMemDumpAllObjectsSince(
        _In_opt_ const _CrtMemState * _State
        );

_CRTIMP void __cdecl _CrtMemDumpStatistics(
        _In_ const _CrtMemState * _State
        );

_CRTIMP int __cdecl _CrtDumpMemoryLeaks(
        void
        );

_CRTIMP int __cdecl _CrtSetCheckCount(
        _In_ int _CheckCount
        );

_CRTIMP int __cdecl _CrtGetCheckCount(
        void
        );

#endif  /* _DEBUG */

#ifdef __cplusplus
}

#ifndef _MFC_OVERRIDES_NEW

extern "C++" {

#ifndef _DEBUG

 /****************************************************************************
 *
 * Debug OFF
 * Debug OFF
 * Debug OFF
 *
 ***************************************************************************/


#pragma warning(suppress: 4985)
_Ret_notnull_ _Post_writable_byte_size_(_Size) void * __CRTDECL operator new[](size_t _Size);

#if !defined (_M_CEE_PURE)
_Ret_notnull_ _Post_writable_byte_size_(_Size) void * __CRTDECL operator new(
        size_t _Size,
        int,
        const char *,
        int
        );

#pragma warning(suppress: 4985)
_Ret_notnull_ _Post_writable_byte_size_(_Size) void * __CRTDECL operator new[](
        size_t _Size,
        int,
        const char *,
        int
        );

void __CRTDECL operator delete[](void *);
void __CRTDECL operator delete(void * _P, int, const char *, int);
void __CRTDECL operator delete[](void * _P, int, const char *, int);

#endif  /* !defined (_M_CEE_PURE) */
#else  /* _DEBUG */

 /****************************************************************************
 *
 * Debug ON
 * Debug ON
 * Debug ON
 *
 ***************************************************************************/

#pragma warning(suppress: 4985)
_Ret_notnull_ _Post_writable_byte_size_(_Size) void * __CRTDECL operator new[](size_t _Size);

_Ret_notnull_ _Post_writable_byte_size_(_Size) void * __CRTDECL operator new(
        size_t _Size,
        int,
        const char *,
        int
        );

#pragma warning(suppress: 4985)
_Ret_notnull_ _Post_writable_byte_size_(_Size) void * __CRTDECL operator new[](
        size_t _Size,
        int,
        const char *,
        int
        );

void __CRTDECL operator delete[](void *);
void __CRTDECL operator delete(void * _P, int, const char *, int);
void __CRTDECL operator delete[](void * _P, int, const char *, int);

#if defined (_CRTDBG_MAP_ALLOC) && defined (_CRTDBG_MAP_ALLOC_NEW)
/* We keep these inlines for back compatibility only;
 * the operator new defined in the debug libraries already calls _malloc_dbg,
 * thus enabling the debug heap allocation functionalities.
 *
 * These inlines do not add any information, due that __FILE__ is expanded
 * to "crtdbg.h", which is not very helpful to the user.
 *
 * The user will need to define _CRTDBG_MAP_ALLOC_NEW in addition to
 * _CRTDBG_MAP_ALLOC to enable these inlines.
 */

_Ret_notnull_ _Post_writable_byte_size_(_Size) inline void * __CRTDECL operator new(size_t _Size)
        { return ::operator new(_Size, _NORMAL_BLOCK, __FILE__, __LINE__); }

_Ret_notnull_ _Post_writable_byte_size_(_Size) inline void* __CRTDECL operator new[](size_t _Size)
        { return ::operator new[](_Size, _NORMAL_BLOCK, __FILE__, __LINE__); }

#endif  /* defined (_CRTDBG_MAP_ALLOC) && defined (_CRTDBG_MAP_ALLOC_NEW) */

#endif  /* _DEBUG */

}

#endif  /* _MFC_OVERRIDES_NEW */

#endif  /* __cplusplus */

#pragma pop_macro("new")
#pragma pack(pop)

#endif  /* _INC_CRTDBG */
