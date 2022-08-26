/***
*stdarg.h - defines ANSI-style macros for variable argument functions
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file defines ANSI-style macros for accessing arguments
*       of functions which take a variable number of arguments.
*       [ANSI]
*
*       [Public]
*
****/

#pragma once

#ifndef _INC_STDARG
#define _INC_STDARG

#include <crtdefs.h>

#if !defined (_WIN32)
#error ERROR: Only Win32 target supported!
#endif  /* !defined (_WIN32) */

#include <vadefs.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define va_start _crt_va_start
#define va_arg _crt_va_arg
#define va_end _crt_va_end

void _CRTIMP __cdecl _vacopy(_Out_ va_list *, _In_ va_list);
#define va_copy(apd, aps) _vacopy(&(apd), aps)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* _INC_STDARG */
