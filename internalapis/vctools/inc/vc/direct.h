/***
*direct.h - function declarations for directory handling/creation
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This include file contains the function declarations for the library
*       functions related to directory handling and creation.
*
*       [Public]
*
****/

#pragma once

#ifndef _INC_DIRECT
#define _INC_DIRECT

#include <crtdefs.h>

/*
 * Currently, all MS C compilers for Win32 platforms default to 8 byte
 * alignment.
 */
#pragma pack(push,_CRT_PACKING)

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP

/* _getdiskfree structure for _getdiskfree() */
#ifndef _DISKFREE_T_DEFINED

struct _diskfree_t {
        unsigned total_clusters;
        unsigned avail_clusters;
        unsigned sectors_per_cluster;
        unsigned bytes_per_sector;
        };

#define _DISKFREE_T_DEFINED
#endif  /* _DISKFREE_T_DEFINED */

#endif  /* _CRT_USE_WINAPI_FAMILY_DESKTOP_APP */

/* function prototypes */

#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma push_macro("_getcwd")
#pragma push_macro("_getdcwd")
#undef _getcwd
#undef _getdcwd
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

_Check_return_ _Ret_maybenull_z_ _CRTIMP char * __cdecl _getcwd(_Out_writes_opt_(_SizeInBytes) char * _DstBuf, _In_ int _SizeInBytes);
_Check_return_ _Ret_maybenull_z_ _CRTIMP char * __cdecl _getdcwd(_In_ int _Drive, _Out_writes_opt_(_SizeInBytes) char * _DstBuf, _In_ int _SizeInBytes);
#define  _getdcwd_nolock  _getdcwd

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma pop_macro("_getcwd")
#pragma pop_macro("_getdcwd")
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

_Check_return_ _CRTIMP int __cdecl _chdir(_In_z_ const char * _Path);

#endif  /* _CRT_USE_WINAPI_FAMILY_DESKTOP_APP */

_Check_return_ _CRTIMP int __cdecl _mkdir(_In_z_ const char * _Path);
_Check_return_ _CRTIMP int __cdecl _rmdir(_In_z_ const char * _Path);

#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP

_Check_return_ _CRTIMP int __cdecl _chdrive(_In_ int _Drive);
_Check_return_ _CRTIMP int __cdecl _getdrive(void);
_Check_return_ _CRTIMP unsigned long __cdecl _getdrives(void);

#ifndef _GETDISKFREE_DEFINED
_Check_return_ _CRTIMP unsigned __cdecl _getdiskfree(_In_ unsigned _Drive, _Out_ struct _diskfree_t * _DiskFree);
#define _GETDISKFREE_DEFINED
#endif  /* _GETDISKFREE_DEFINED */

#endif  /* _CRT_USE_WINAPI_FAMILY_DESKTOP_APP */

#ifndef _WDIRECT_DEFINED

/* wide function prototypes, also declared in wchar.h  */

#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma push_macro("_wgetcwd")
#pragma push_macro("_wgetdcwd")
#undef _wgetcwd
#undef _wgetdcwd
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

_Check_return_ _Ret_maybenull_z_ _CRTIMP wchar_t * __cdecl _wgetcwd(_Out_writes_opt_(_SizeInWords) wchar_t * _DstBuf, _In_ int _SizeInWords);
_Check_return_ _Ret_maybenull_z_ _CRTIMP wchar_t * __cdecl _wgetdcwd(_In_ int _Drive, _Out_writes_opt_(_SizeInWords) wchar_t * _DstBuf, _In_ int _SizeInWords);
#define  _wgetdcwd_nolock  _wgetdcwd

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma pop_macro("_wgetcwd")
#pragma pop_macro("_wgetdcwd")
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

_Check_return_ _CRTIMP int __cdecl _wchdir(_In_z_ const wchar_t * _Path);

#endif  /* _CRT_USE_WINAPI_FAMILY_DESKTOP_APP */

_Check_return_ _CRTIMP int __cdecl _wmkdir(_In_z_ const wchar_t * _Path);
_Check_return_ _CRTIMP int __cdecl _wrmdir(_In_z_ const wchar_t * _Path);

#define _WDIRECT_DEFINED
#endif  /* _WDIRECT_DEFINED */

#if !__STDC__

#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP

/* Non-ANSI names for compatibility */
#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma push_macro("getcwd")
#undef getcwd
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

_Check_return_ _Ret_maybenull_z_ _CRT_NONSTDC_DEPRECATE(_getcwd) _CRTIMP char * __cdecl getcwd(_Out_writes_opt_(_SizeInBytes) char * _DstBuf, _In_ int _SizeInBytes);

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma pop_macro("getcwd")
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

_Check_return_ _CRT_NONSTDC_DEPRECATE(_chdir) _CRTIMP int __cdecl chdir(_In_z_ const char * _Path);

#endif  /* _CRT_USE_WINAPI_FAMILY_DESKTOP_APP */

_Check_return_ _CRT_NONSTDC_DEPRECATE(_mkdir) _CRTIMP int __cdecl mkdir(_In_z_ const char * _Path);
_Check_return_ _CRT_NONSTDC_DEPRECATE(_rmdir) _CRTIMP int __cdecl rmdir(_In_z_ const char * _Path);

#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP
#define diskfree_t  _diskfree_t
#endif  /* _CRT_USE_WINAPI_FAMILY_DESKTOP_APP */

#endif  /* !__STDC__ */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#pragma pack(pop)

#endif  /* _INC_DIRECT */
