/***
*vadefs.h - defines helper macros for stdarg.h
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This is a helper file for stdarg.h
*
*       [Public]
*
****/

#pragma once

#ifndef _INC_VADEFS
#define _INC_VADEFS

#if !defined (_WIN32)
#error ERROR: Only Win32 target supported!
#endif  /* !defined (_WIN32) */

/*
 * Currently, all MS C compilers for Win32 platforms default to 8 byte
 * alignment.
 */
#undef _CRT_PACKING
#define _CRT_PACKING 8
#pragma pack(push,_CRT_PACKING)

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#ifdef _CRTBLD
#include <cruntime.h>
#endif  /* _CRTBLD */

#if !defined (_W64)
#if !defined (__midl) && (defined (_X86_) || defined (_M_IX86))
#define _W64 __w64
#else  /* !defined (__midl) && (defined (_X86_) || defined (_M_IX86)) */
#define _W64
#endif  /* !defined (__midl) && (defined (_X86_) || defined (_M_IX86)) */
#endif  /* !defined (_W64) */

#ifndef _UINTPTR_T_DEFINED
#ifdef _WIN64
typedef unsigned __int64    uintptr_t;
#else  /* _WIN64 */
typedef _W64 unsigned int   uintptr_t;
#endif  /* _WIN64 */
#define _UINTPTR_T_DEFINED
#endif  /* _UINTPTR_T_DEFINED */

#ifndef _VA_LIST_DEFINED
#ifdef _M_CEE_PURE
typedef System::ArgIterator va_list;
#else  /* _M_CEE_PURE */
typedef char *  va_list;
#endif  /* _M_CEE_PURE */
#define _VA_LIST_DEFINED
#endif  /* _VA_LIST_DEFINED */

#ifdef __cplusplus
#define _ADDRESSOF(v)   ( &reinterpret_cast<const char &>(v) )
#else  /* __cplusplus */
#define _ADDRESSOF(v)   ( &(v) )
#endif  /* __cplusplus */

#if defined (_M_ARM) && !defined (_M_CEE_PURE)
#define _VA_ALIGN       4
#define _SLOTSIZEOF(t)  ( (sizeof(t) + _VA_ALIGN - 1) & ~(_VA_ALIGN - 1) )

#define _APALIGN(t,ap)  ( ((va_list)0 - (ap)) & (__alignof(t) - 1) )

#else  /* defined (_M_ARM) && !defined (_M_CEE_PURE) */
#define _SLOTSIZEOF(t)  (sizeof(t))
#define _APALIGN(t,ap)  (__alignof(t))
#endif  /* defined (_M_ARM) && !defined (_M_CEE_PURE) */

#if defined (_M_CEE_PURE) || (defined (_M_CEE) && !defined (_M_ARM))

extern void __cdecl __va_start(va_list*, ...);
extern void * __cdecl __va_arg(va_list*, ...);
extern void __cdecl __va_end(va_list*);

#define _crt_va_start(ap,v)  ( __va_start(&ap, _ADDRESSOF(v), _SLOTSIZEOF(v), \
                                __alignof(v), _ADDRESSOF(v)) )
#define _crt_va_arg(ap,t)    ( *(t *)__va_arg(&ap, _SLOTSIZEOF(t), \
                                _APALIGN(t,ap), (t *)0) )
#define _crt_va_end(ap)      ( __va_end(&ap) )

#elif defined (_M_IX86)

#define _INTSIZEOF(n)   ( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )

#define _crt_va_start(ap,v)  ( ap = (va_list)_ADDRESSOF(v) + _INTSIZEOF(v) )
#define _crt_va_arg(ap,t)    ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
#define _crt_va_end(ap)      ( ap = (va_list)0 )

#elif defined (_M_ARM)

#ifdef __cplusplus
extern void __cdecl __va_start(va_list*, ...);
#define _crt_va_start(ap,v)  ( __va_start(&ap, _ADDRESSOF(v), _SLOTSIZEOF(v), \
                             _ADDRESSOF(v)) )
#else  /* __cplusplus */
#define _crt_va_start(ap,v)  ( ap = (va_list)_ADDRESSOF(v) + _SLOTSIZEOF(v) )
#endif  /* __cplusplus */

#define _crt_va_arg(ap,t)    (*(t *)((ap += _SLOTSIZEOF(t) + _APALIGN(t,ap)) \
                             - _SLOTSIZEOF(t)))

#define _crt_va_end(ap)      ( ap = (va_list)0 )

#elif defined (_M_X64)


extern void __cdecl __va_start(va_list *, ...);

#define _crt_va_start(ap, x) ( __va_start(&ap, x) )
#define _crt_va_arg(ap, t)   \
    ( ( sizeof(t) > sizeof(__int64) || ( sizeof(t) & (sizeof(t) - 1) ) != 0 ) \
        ? **(t **)( ( ap += sizeof(__int64) ) - sizeof(__int64) ) \
        :  *(t  *)( ( ap += sizeof(__int64) ) - sizeof(__int64) ) )
#define _crt_va_end(ap)      ( ap = (va_list)0 )

#else  /* defined (_M_X64) */

/* A guess at the proper definitions for other platforms */

#define _INTSIZEOF(n)   ( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )

#define _crt_va_start(ap,v)  ( ap = (va_list)_ADDRESSOF(v) + _INTSIZEOF(v) )
#define _crt_va_arg(ap,t)    ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
#define _crt_va_end(ap)      ( ap = (va_list)0 )

#endif  /* defined (_M_X64) */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#pragma pack(pop)

#endif  /* _INC_VADEFS */
