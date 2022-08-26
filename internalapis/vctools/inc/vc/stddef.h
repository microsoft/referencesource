/***
*stddef.h - definitions/declarations for common constants, types, variables
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file contains definitions and declarations for some commonly
*       used constants, types, and variables.
*       [ANSI]
*
*       [Public]
*
****/

#pragma once

#ifndef _INC_STDDEF
#define _INC_STDDEF

#include <crtdefs.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/* Define NULL pointer value */
#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else  /* __cplusplus */
#define NULL    ((void *)0)
#endif  /* __cplusplus */
#endif  /* NULL */

#ifdef __cplusplus
namespace std { typedef decltype(__nullptr) nullptr_t; }
using ::std::nullptr_t;
#endif  /* __cplusplus */

/* Declare reference to errno */
#ifndef _CRT_ERRNO_DEFINED
#define _CRT_ERRNO_DEFINED
_CRTIMP extern int * __cdecl _errno(void);
#define errno   (*_errno())

errno_t __cdecl _set_errno(_In_ int _Value);
errno_t __cdecl _get_errno(_Out_ int * _Value);
#endif  /* _CRT_ERRNO_DEFINED */

/* Define offsetof macro */
#ifdef __cplusplus

#ifdef _WIN64
#define offsetof(s,m)   (size_t)( (ptrdiff_t)&reinterpret_cast<const volatile char&>((((s *)0)->m)) )
#else  /* _WIN64 */
#define offsetof(s,m)   (size_t)&reinterpret_cast<const volatile char&>((((s *)0)->m))
#endif  /* _WIN64 */

#else  /* __cplusplus */

#ifdef _WIN64
#define offsetof(s,m)   (size_t)( (ptrdiff_t)&(((s *)0)->m) )
#else  /* _WIN64 */
#define offsetof(s,m)   (size_t)&(((s *)0)->m)
#endif  /* _WIN64 */

#endif  /* __cplusplus */

_CRTIMP extern unsigned long  __cdecl __threadid(void);
#define _threadid       (__threadid())
_CRTIMP extern uintptr_t __cdecl __threadhandle(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _INC_STDDEF */
