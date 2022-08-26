/***
*wchar.h - declarations for wide character functions
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file contains the types, macros and function declarations for
*       all wide character-related functions.  They may also be declared in
*       individual header files on a functional basis.
*       [ISO]
*
*       Note: keep in sync with ctype.h, stdio.h, stdlib.h, string.h, time.h.
*
*       [Public]
*
****/

#pragma once

#ifndef _INC_WCHAR
#define _INC_WCHAR

#include <crtdefs.h>

#pragma pack(push,_CRT_PACKING)

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/*
 *  According to the standard, WCHAR_MIN and WCHAR_MAX need to be
 *  "constant expressions suitable for use in #if preprocessing directives,
 *  and this expression shall have the same type as would an expression that
 *  is an object of the corresponding type converted according to the integer
 *  promotions".
 */
#define WCHAR_MIN       0x0000
#define WCHAR_MAX       0xffff

#ifndef _VA_LIST_DEFINED
#ifdef _M_CEE_PURE
typedef System::ArgIterator va_list;
#else  /* _M_CEE_PURE */
typedef char *  va_list;
#endif  /* _M_CEE_PURE */
#define _VA_LIST_DEFINED
#endif  /* _VA_LIST_DEFINED */

#ifndef WEOF
#define WEOF (wint_t)(0xFFFF)
#endif  /* WEOF */

#ifndef _FILE_DEFINED
struct _iobuf {
        char *_ptr;
        int   _cnt;
        char *_base;
        int   _flag;
        int   _file;
        int   _charbuf;
        int   _bufsiz;
        char *_tmpfname;
        };
typedef struct _iobuf FILE;
#define _FILE_DEFINED
#endif  /* _FILE_DEFINED */

/* Declare _iob[] array */

#ifndef _STDIO_DEFINED
#ifdef _CRTBLD
/* These functions are for enabling STATIC_CPPLIB functionality */
#if defined (_DLL) && defined (_M_IX86)
/* Retained for compatibility with VC++ 5.0 and earlier versions */
_CRTIMP extern FILE * __cdecl __p__iob(void);
#endif  /* defined (_DLL) && defined (_M_IX86) */
#ifndef _M_CEE_PURE
_CRTIMP extern FILE _iob[];
#endif  /* _M_CEE_PURE */
#endif  /* _CRTBLD */
_CRTIMP FILE * __cdecl __iob_func(void);
#endif  /* _STDIO_DEFINED */

#ifndef _STDSTREAM_DEFINED
#define stdin  (&__iob_func()[0])
#define stdout (&__iob_func()[1])
#define stderr (&__iob_func()[2])
#define _STDSTREAM_DEFINED
#endif  /* _STDSTREAM_DEFINED */

#ifndef _FSIZE_T_DEFINED
typedef unsigned long _fsize_t; /* Could be 64 bits for Win32 */
#define _FSIZE_T_DEFINED
#endif  /* _FSIZE_T_DEFINED */

#ifndef _WFINDDATA_T_DEFINED

struct _wfinddata32_t {
        unsigned    attrib;
        __time32_t  time_create;    /* -1 for FAT file systems */
        __time32_t  time_access;    /* -1 for FAT file systems */
        __time32_t  time_write;
        _fsize_t    size;
        wchar_t     name[260];
};

struct _wfinddata32i64_t {
        unsigned    attrib;
        __time32_t  time_create;    /* -1 for FAT file systems */
        __time32_t  time_access;    /* -1 for FAT file systems */
        __time32_t  time_write;
        __int64     size;
        wchar_t     name[260];
};

struct _wfinddata64i32_t {
        unsigned    attrib;
        __time64_t  time_create;    /* -1 for FAT file systems */
        __time64_t  time_access;    /* -1 for FAT file systems */
        __time64_t  time_write;
        _fsize_t    size;
        wchar_t     name[260];
};

struct _wfinddata64_t {
        unsigned    attrib;
        __time64_t  time_create;    /* -1 for FAT file systems */
        __time64_t  time_access;    /* -1 for FAT file systems */
        __time64_t  time_write;
        __int64     size;
        wchar_t     name[260];
};

#ifdef _USE_32BIT_TIME_T
#define _wfinddata_t    _wfinddata32_t
#define _wfinddatai64_t _wfinddata32i64_t

#define _wfindfirst     _wfindfirst32
#define _wfindnext      _wfindnext32
#define _wfindfirsti64  _wfindfirst32i64
#define _wfindnexti64   _wfindnext32i64

#else  /* _USE_32BIT_TIME_T */
#define _wfinddata_t    _wfinddata64i32_t
#define _wfinddatai64_t _wfinddata64_t

#define _wfindfirst     _wfindfirst64i32
#define _wfindnext      _wfindnext64i32
#define _wfindfirsti64  _wfindfirst64
#define _wfindnexti64   _wfindnext64

#endif  /* _USE_32BIT_TIME_T */

#define _WFINDDATA_T_DEFINED
#endif  /* _WFINDDATA_T_DEFINED */


/* define NULL pointer value */
#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else  /* __cplusplus */
#define NULL    ((void *)0)
#endif  /* __cplusplus */
#endif  /* NULL */

#ifndef _CONST_RETURN
#ifdef __cplusplus
#define _CONST_RETURN  const
#define _CRT_CONST_CORRECT_OVERLOADS
#else  /* __cplusplus */
#define _CONST_RETURN
#endif  /* __cplusplus */
#endif  /* _CONST_RETURN */

/* For backwards compatibility */
#define _WConst_return _CONST_RETURN

#ifndef _CRT_CTYPEDATA_DEFINED
#define _CRT_CTYPEDATA_DEFINED
#ifndef _CTYPE_DISABLE_MACROS
#ifdef _CRTBLD
extern const unsigned short __newctype[];
#if defined (_DLL) && defined (_M_IX86)
/* Retained for compatibility with VC++ 5.0 and earlier versions */
_CRTIMP const unsigned short ** __cdecl __p__pctype(void);
#endif  /* defined (_DLL) && defined (_M_IX86) */
#endif  /* _CRTBLD */

#ifndef __PCTYPE_FUNC
#if defined (_CRT_DISABLE_PERFCRIT_LOCKS) && !defined (_DLL)
#define __PCTYPE_FUNC  _pctype
#else  /* defined (_CRT_DISABLE_PERFCRIT_LOCKS) && !defined (_DLL) */
#define __PCTYPE_FUNC   __pctype_func()
#endif  /* defined (_CRT_DISABLE_PERFCRIT_LOCKS) && !defined (_DLL) */
#endif  /* __PCTYPE_FUNC */

_CRTIMP const unsigned short * __cdecl __pctype_func(void);
#if !defined (_M_CEE_PURE)
_CRTIMP extern const unsigned short *_pctype;
#else  /* !defined (_M_CEE_PURE) */
#define _pctype (__pctype_func())
#endif  /* !defined (_M_CEE_PURE) */
#endif  /* _CTYPE_DISABLE_MACROS */
#endif  /* _CRT_CTYPEDATA_DEFINED */

#ifndef _CRT_WCTYPEDATA_DEFINED
#define _CRT_WCTYPEDATA_DEFINED
#ifndef _CTYPE_DISABLE_MACROS
#if !defined (_M_CEE_PURE)
_CRTIMP extern const unsigned short _wctype[];
#endif  /* !defined (_M_CEE_PURE) */
#ifdef _CRTBLD
extern const unsigned short __newctype[];
#if defined (_DLL) && defined (_M_IX86)
/* Retained for compatibility with VC++ 5.0 and earlier versions */
_CRTIMP const wctype_t ** __cdecl __p__pwctype(void);
#endif  /* defined (_DLL) && defined (_M_IX86) */
#endif  /* _CRTBLD */

_CRTIMP const wctype_t * __cdecl __pwctype_func(void);
#if !defined (_M_CEE_PURE)
_CRTIMP extern const wctype_t *_pwctype;
#else  /* !defined (_M_CEE_PURE) */
#define _pwctype (__pwctype_func())
#endif  /* !defined (_M_CEE_PURE) */
#endif  /* _CTYPE_DISABLE_MACROS */
#endif  /* _CRT_WCTYPEDATA_DEFINED */

/* set bit masks for the possible character types */

#define _UPPER          0x1     /* upper case letter */
#define _LOWER          0x2     /* lower case letter */
#define _DIGIT          0x4     /* digit[0-9] */
#define _SPACE          0x8     /* tab, carriage return, newline, */
                                /* vertical tab or form feed */
#define _PUNCT          0x10    /* punctuation character */
#define _CONTROL        0x20    /* control character */
#define _BLANK          0x40    /* space char */
#define _HEX            0x80    /* hexadecimal digit */

#define _LEADBYTE       0x8000                  /* multibyte leadbyte */
#define _ALPHA          (0x0100|_UPPER|_LOWER)  /* alphabetic character */


/* Function prototypes */

#ifndef _WCTYPE_DEFINED

/* Character classification function prototypes */
/* also declared in ctype.h */

_Check_return_ _CRTIMP int __cdecl iswalpha(_In_ wint_t _C);
_Check_return_ _CRTIMP int __cdecl _iswalpha_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl iswupper(_In_ wint_t _C);
_Check_return_ _CRTIMP int __cdecl _iswupper_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl iswlower(_In_ wint_t _C);
_Check_return_ _CRTIMP int __cdecl _iswlower_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl iswdigit(_In_ wint_t _C);
_Check_return_ _CRTIMP int __cdecl _iswdigit_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl iswxdigit(_In_ wint_t _C);
_Check_return_ _CRTIMP int __cdecl _iswxdigit_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl iswspace(_In_ wint_t _C);
_Check_return_ _CRTIMP int __cdecl _iswspace_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl iswpunct(_In_ wint_t _C);
_Check_return_ _CRTIMP int __cdecl _iswpunct_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl iswblank(_In_ wint_t _C);
_Check_return_ _CRTIMP int __cdecl _iswblank_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl iswalnum(_In_ wint_t _C);
_Check_return_ _CRTIMP int __cdecl _iswalnum_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl iswprint(_In_ wint_t _C);
_Check_return_ _CRTIMP int __cdecl _iswprint_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl iswgraph(_In_ wint_t _C);
_Check_return_ _CRTIMP int __cdecl _iswgraph_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl iswcntrl(_In_ wint_t _C);
_Check_return_ _CRTIMP int __cdecl _iswcntrl_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl iswascii(_In_ wint_t _C);

#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP
_Check_return_ _CRTIMP int __cdecl isleadbyte(_In_ int _C);
_Check_return_ _CRTIMP int __cdecl _isleadbyte_l(_In_ int _C, _In_opt_ _locale_t _Locale);
#endif  /* _CRT_USE_WINAPI_FAMILY_DESKTOP_APP */

_Check_return_ _CRTIMP wint_t __cdecl towupper(_In_ wint_t _C);
_Check_return_ _CRTIMP wint_t __cdecl _towupper_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP wint_t __cdecl towlower(_In_ wint_t _C);
_Check_return_ _CRTIMP wint_t __cdecl _towlower_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl iswctype(_In_ wint_t _C, _In_ wctype_t _Type);
_Check_return_ _CRTIMP int __cdecl _iswctype_l(_In_ wint_t _C, _In_ wctype_t _Type, _In_opt_ _locale_t _Locale);

_Check_return_ _CRTIMP int __cdecl __iswcsymf(_In_ wint_t _C);
_Check_return_ _CRTIMP int __cdecl _iswcsymf_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl __iswcsym(_In_ wint_t _C);
_Check_return_ _CRTIMP int __cdecl _iswcsym_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);

#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP
_CRT_OBSOLETE(iswctype) _CRTIMP int __cdecl is_wctype(_In_ wint_t _C, _In_ wctype_t _Type);
#endif  /* _CRT_USE_WINAPI_FAMILY_DESKTOP_APP */


#define _WCTYPE_DEFINED
#endif  /* _WCTYPE_DEFINED */

#ifndef _WDIRECT_DEFINED

/* also declared in direct.h */

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

#ifndef _WIO_DEFINED

_Check_return_ _CRTIMP int __cdecl _waccess(_In_z_ const wchar_t * _Filename, _In_ int _AccessMode);
_Check_return_wat_ _CRTIMP errno_t __cdecl _waccess_s(_In_z_ const wchar_t * _Filename, _In_ int _AccessMode);
_Check_return_ _CRTIMP int __cdecl _wchmod(_In_z_ const wchar_t * _Filename, _In_ int _Mode);
_Check_return_ _CRT_INSECURE_DEPRECATE(_wsopen_s) _CRTIMP int __cdecl _wcreat(_In_z_ const wchar_t * _Filename, _In_ int _PermissionMode);
_Check_return_ _CRTIMP intptr_t __cdecl _wfindfirst32(_In_z_ const wchar_t * _Filename, _Out_ struct _wfinddata32_t * _FindData);
_Check_return_ _CRTIMP int __cdecl _wfindnext32(_In_ intptr_t _FindHandle, _Out_ struct _wfinddata32_t * _FindData);
_CRTIMP int __cdecl _wunlink(_In_z_ const wchar_t * _Filename);
_Check_return_ _CRTIMP int __cdecl _wrename(_In_z_ const wchar_t * _OldFilename, _In_z_ const wchar_t * _NewFilename);
_CRTIMP errno_t __cdecl _wmktemp_s(_Inout_updates_z_(_SizeInWords) wchar_t * _TemplateName, _In_ size_t _SizeInWords);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_0(errno_t, _wmktemp_s, wchar_t, _TemplateName)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0(wchar_t *, __RETURN_POLICY_DST, _CRTIMP, _wmktemp, _Inout_z_, wchar_t, _TemplateName)

_Check_return_ _CRTIMP intptr_t __cdecl _wfindfirst32i64(_In_z_ const wchar_t * _Filename, _Out_ struct _wfinddata32i64_t * _FindData);
_Check_return_ _CRTIMP intptr_t __cdecl _wfindfirst64i32(_In_z_ const wchar_t * _Filename, _Out_ struct _wfinddata64i32_t * _FindData);
_Check_return_ _CRTIMP intptr_t __cdecl _wfindfirst64(_In_z_ const wchar_t * _Filename, _Out_ struct _wfinddata64_t * _FindData);
_Check_return_ _CRTIMP int __cdecl _wfindnext32i64(_In_ intptr_t _FindHandle, _Out_ struct _wfinddata32i64_t * _FindData);
_Check_return_ _CRTIMP int __cdecl _wfindnext64i32(_In_ intptr_t _FindHandle, _Out_ struct _wfinddata64i32_t * _FindData);
_Check_return_ _CRTIMP int __cdecl _wfindnext64(_In_ intptr_t _FindHandle, _Out_ struct _wfinddata64_t * _FindData);

_Check_return_wat_ _CRTIMP errno_t __cdecl _wsopen_s(_Out_ int * _FileHandle, _In_z_ const wchar_t * _Filename, _In_ int _OpenFlag, _In_ int _ShareFlag, _In_ int _PermissionFlag);

#if !defined (__cplusplus) || !defined (_M_IX86)

_Check_return_ _CRT_INSECURE_DEPRECATE(_wsopen_s) _CRTIMP int __cdecl _wopen(_In_z_ const wchar_t * _Filename, _In_ int _OpenFlag, ...);
_Check_return_ _CRT_INSECURE_DEPRECATE(_wsopen_s) _CRTIMP int __cdecl _wsopen(_In_z_ const wchar_t * _Filename, _In_ int _OpenFlag, int _ShareFlag, ...);

#else  /* !defined (__cplusplus) || !defined (_M_IX86) */

/* these function do not validate pmode; use _sopen_s */
extern "C++" _CRT_INSECURE_DEPRECATE(_wsopen_s) _CRTIMP int __cdecl _wopen(_In_z_ const wchar_t * _Filename, _In_ int _OpenFlag, _In_ int _PermissionMode = 0);
extern "C++" _CRT_INSECURE_DEPRECATE(_wsopen_s) _CRTIMP int __cdecl _wsopen(_In_z_ const wchar_t * _Filename, _In_ int _OpenFlag, _In_ int _ShareFlag, int _PermissionMode = 0);

#endif  /* !defined (__cplusplus) || !defined (_M_IX86) */

#define _WIO_DEFINED
#endif  /* _WIO_DEFINED */

#ifndef _WLOCALE_DEFINED

/* wide function prototypes, also declared in wchar.h  */

_Check_return_opt_ _CRTIMP wchar_t * __cdecl _wsetlocale(_In_ int _Category, _In_opt_z_ const wchar_t * _Locale);
_Check_return_opt_ _CRTIMP _locale_t __cdecl _wcreate_locale(_In_ int _Category, _In_z_ const wchar_t * _Locale);

#define _WLOCALE_DEFINED
#endif  /* _WLOCALE_DEFINED */

#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP

#ifndef _WPROCESS_DEFINED

/* also declared in process.h */

_CRTIMP intptr_t __cdecl _wexecl(_In_z_ const wchar_t * _Filename, _In_z_ const wchar_t * _ArgList, ...);
_CRTIMP intptr_t __cdecl _wexecle(_In_z_ const wchar_t * _Filename, _In_z_ const wchar_t * _ArgList, ...);
_CRTIMP intptr_t __cdecl _wexeclp(_In_z_ const wchar_t * _Filename, _In_z_ const wchar_t * _ArgList, ...);
_CRTIMP intptr_t __cdecl _wexeclpe(_In_z_ const wchar_t * _Filename, _In_z_ const wchar_t * _ArgList, ...);
_CRTIMP intptr_t __cdecl _wexecv(_In_z_ const wchar_t * _Filename, _In_z_ const wchar_t * const * _ArgList);
_CRTIMP intptr_t __cdecl _wexecve(_In_z_ const wchar_t * _Filename, _In_z_ const wchar_t * const * _ArgList,
        _In_opt_z_ const wchar_t * const * _Env);
_CRTIMP intptr_t __cdecl _wexecvp(_In_z_ const wchar_t * _Filename, _In_z_ const wchar_t * const * _ArgList);
_CRTIMP intptr_t __cdecl _wexecvpe(_In_z_ const wchar_t * _Filename, _In_z_ const wchar_t * const * _ArgList,
        _In_opt_z_ const wchar_t * const * _Env);
_CRTIMP intptr_t __cdecl _wspawnl(_In_ int _Mode, _In_z_ const wchar_t * _Filename, _In_z_ const wchar_t * _ArgList, ...);
_CRTIMP intptr_t __cdecl _wspawnle(_In_ int _Mode, _In_z_ const wchar_t * _Filename, _In_z_ const wchar_t * _ArgList, ...);
_CRTIMP intptr_t __cdecl _wspawnlp(_In_ int _Mode, _In_z_ const wchar_t * _Filename, _In_z_ const wchar_t * _ArgList, ...);
_CRTIMP intptr_t __cdecl _wspawnlpe(_In_ int _Mode, _In_z_ const wchar_t * _Filename, _In_z_ const wchar_t * _ArgList, ...);
_CRTIMP intptr_t __cdecl _wspawnv(_In_ int _Mode, _In_z_ const wchar_t * _Filename, _In_z_ const wchar_t * const * _ArgList);
_CRTIMP intptr_t __cdecl _wspawnve(_In_ int _Mode, _In_z_ const wchar_t * _Filename, _In_z_ const wchar_t * const * _ArgList,
        _In_opt_z_ const wchar_t * const * _Env);
_CRTIMP intptr_t __cdecl _wspawnvp(_In_ int _Mode, _In_z_ const wchar_t * _Filename, _In_z_ const wchar_t * const * _ArgList);
_CRTIMP intptr_t __cdecl _wspawnvpe(_In_ int _Mode, _In_z_ const wchar_t * _Filename, _In_z_ const wchar_t * const * _ArgList,
        _In_opt_z_ const wchar_t * const * _Env);
#ifndef _CRT_WSYSTEM_DEFINED
#define _CRT_WSYSTEM_DEFINED
_CRTIMP int __cdecl _wsystem(_In_opt_z_ const wchar_t * _Command);
#endif  /* _CRT_WSYSTEM_DEFINED */

#define _WPROCESS_DEFINED
#endif  /* _WPROCESS_DEFINED */

#endif  /* _CRT_USE_WINAPI_FAMILY_DESKTOP_APP */

#ifndef _WCTYPE_INLINE_DEFINED

#ifdef _CRTBLD
#define _CRT_WCTYPE_NOINLINE
#else  /* _CRTBLD */
#undef _CRT_WCTYPE_NOINLINE
#endif  /* _CRTBLD */

#if !defined (__cplusplus) || defined (_M_CEE_PURE) || defined (MRTDLL) || defined (_CRT_WCTYPE_NOINLINE)
#define iswalpha(_c)    ( iswctype(_c,_ALPHA) )
#define iswupper(_c)    ( iswctype(_c,_UPPER) )
#define iswlower(_c)    ( iswctype(_c,_LOWER) )
#define iswdigit(_c)    ( iswctype(_c,_DIGIT) )
#define iswxdigit(_c)   ( iswctype(_c,_HEX) )
#define iswspace(_c)    ( iswctype(_c,_SPACE) )
#define iswpunct(_c)    ( iswctype(_c,_PUNCT) )
#define iswblank(_c)    ( iswctype(_c,_BLANK) )
#define iswalnum(_c)    ( iswctype(_c,_ALPHA|_DIGIT) )
#define iswprint(_c)    ( iswctype(_c,_BLANK|_PUNCT|_ALPHA|_DIGIT) )
#define iswgraph(_c)    ( iswctype(_c,_PUNCT|_ALPHA|_DIGIT) )
#define iswcntrl(_c)    ( iswctype(_c,_CONTROL) )
#define iswascii(_c)    ( (unsigned)(_c) < 0x80 )

#define _iswalpha_l(_c,_p)    ( iswctype(_c,_ALPHA) )
#define _iswupper_l(_c,_p)    ( iswctype(_c,_UPPER) )
#define _iswlower_l(_c,_p)    ( iswctype(_c,_LOWER) )
#define _iswdigit_l(_c,_p)    ( iswctype(_c,_DIGIT) )
#define _iswxdigit_l(_c,_p)   ( iswctype(_c,_HEX) )
#define _iswspace_l(_c,_p)    ( iswctype(_c,_SPACE) )
#define _iswpunct_l(_c,_p)    ( iswctype(_c,_PUNCT) )
#define _iswblank_l(_c,_p)    ( iswctype(_c,_BLANK) )
#define _iswalnum_l(_c,_p)    ( iswctype(_c,_ALPHA|_DIGIT) )
#define _iswprint_l(_c,_p)    ( iswctype(_c,_BLANK|_PUNCT|_ALPHA|_DIGIT) )
#define _iswgraph_l(_c,_p)    ( iswctype(_c,_PUNCT|_ALPHA|_DIGIT) )
#define _iswcntrl_l(_c,_p)    ( iswctype(_c,_CONTROL) )

#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP
#ifndef _CTYPE_DISABLE_MACROS
#define isleadbyte(_c)  ( __PCTYPE_FUNC[(unsigned char)(_c)] & _LEADBYTE)
#endif  /* _CTYPE_DISABLE_MACROS */
#endif  /* _CRT_USE_WINAPI_FAMILY_DESKTOP_APP */

#endif  /* !defined (__cplusplus) || defined (_M_CEE_PURE) || defined (MRTDLL) || defined (_CRT_WCTYPE_NOINLINE) */
#define _WCTYPE_INLINE_DEFINED
#endif  /* _WCTYPE_INLINE_DEFINED */



/* define structure for returning status information */

#ifndef _INO_T_DEFINED
typedef unsigned short _ino_t;      /* i-node number (not used on DOS) */
#if !__STDC__
/* Non-ANSI name for compatibility */
typedef unsigned short ino_t;
#endif  /* !__STDC__ */
#define _INO_T_DEFINED
#endif  /* _INO_T_DEFINED */

#ifndef _DEV_T_DEFINED
typedef unsigned int _dev_t;        /* device code */
#if !__STDC__
/* Non-ANSI name for compatibility */
typedef unsigned int dev_t;
#endif  /* !__STDC__ */
#define _DEV_T_DEFINED
#endif  /* _DEV_T_DEFINED */

#ifndef _OFF_T_DEFINED
typedef long _off_t;                /* file offset value */
#if !__STDC__
/* Non-ANSI name for compatibility */
typedef long off_t;
#endif  /* !__STDC__ */
#define _OFF_T_DEFINED
#endif  /* _OFF_T_DEFINED */

#ifndef _STAT_DEFINED

struct _stat32 {
        _dev_t     st_dev;
        _ino_t     st_ino;
        unsigned short st_mode;
        short      st_nlink;
        short      st_uid;
        short      st_gid;
        _dev_t     st_rdev;
        _off_t     st_size;
        __time32_t st_atime;
        __time32_t st_mtime;
        __time32_t st_ctime;
        };

#if !__STDC__
/* Non-ANSI names for compatibility */
struct stat {
        _dev_t     st_dev;
        _ino_t     st_ino;
        unsigned short st_mode;
        short      st_nlink;
        short      st_uid;
        short      st_gid;
        _dev_t     st_rdev;
        _off_t     st_size;
        time_t st_atime;
        time_t st_mtime;
        time_t st_ctime;
        };

#endif  /* !__STDC__ */

struct _stat32i64 {
        _dev_t     st_dev;
        _ino_t     st_ino;
        unsigned short st_mode;
        short      st_nlink;
        short      st_uid;
        short      st_gid;
        _dev_t     st_rdev;
        __int64    st_size;
        __time32_t st_atime;
        __time32_t st_mtime;
        __time32_t st_ctime;
        };

struct _stat64i32 {
        _dev_t     st_dev;
        _ino_t     st_ino;
        unsigned short st_mode;
        short      st_nlink;
        short      st_uid;
        short      st_gid;
        _dev_t     st_rdev;
        _off_t     st_size;
        __time64_t st_atime;
        __time64_t st_mtime;
        __time64_t st_ctime;
        };

struct _stat64 {
        _dev_t     st_dev;
        _ino_t     st_ino;
        unsigned short st_mode;
        short      st_nlink;
        short      st_uid;
        short      st_gid;
        _dev_t     st_rdev;
        __int64    st_size;
        __time64_t st_atime;
        __time64_t st_mtime;
        __time64_t st_ctime;
        };

/*
 * We have to have same name for structure and the fuction so as to do the
 * macro magic.we need the structure name and function name the same.
 */
#define __stat64    _stat64

#ifdef _USE_32BIT_TIME_T
#define _fstat      _fstat32
#define _fstati64   _fstat32i64
#define _stat       _stat32
#define _stati64    _stat32i64
#define _wstat      _wstat32
#define _wstati64   _wstat32i64

#else  /* _USE_32BIT_TIME_T */
#define _fstat      _fstat64i32
#define _fstati64   _fstat64
#define _stat       _stat64i32
#define _stati64    _stat64
#define _wstat      _wstat64i32
#define _wstati64   _wstat64

#endif  /* _USE_32BIT_TIME_T */


#define _STAT_DEFINED
#endif  /* _STAT_DEFINED */

#ifndef _WSTAT_DEFINED

/* also declared in wchar.h */

_CRTIMP int __cdecl _wstat32(_In_z_ const wchar_t * _Name, _Out_ struct _stat32 * _Stat);

_CRTIMP int __cdecl _wstat32i64(_In_z_ const wchar_t * _Name, _Out_ struct _stat32i64 * _Stat);
_CRTIMP int __cdecl _wstat64i32(_In_z_ const wchar_t * _Name, _Out_ struct _stat64i32 * _Stat);
_CRTIMP int __cdecl _wstat64(_In_z_ const wchar_t * _Name, _Out_ struct _stat64 * _Stat);

#define _WSTAT_DEFINED
#endif  /* _WSTAT_DEFINED */


#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP

#ifndef _WCONIO_DEFINED

/* wide function prototypes, also declared in conio.h */

#ifndef WEOF
#define WEOF (wint_t)(0xFFFF)
#endif  /* WEOF */

_Check_return_wat_ _CRTIMP errno_t __cdecl _cgetws_s(_Out_writes_to_(_SizeInWords, *_SizeRead) wchar_t * _Buffer, size_t _SizeInWords, _Out_ size_t * _SizeRead);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(errno_t, _cgetws_s, _Post_readable_size_(*_Buffer) wchar_t, _Buffer, size_t *, _SizeRead)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0_CGETS(wchar_t *, _CRTIMP, _cgetws, _Inout_z_, wchar_t, _Buffer)
_Check_return_ _CRTIMP wint_t __cdecl _getwch(void);
_Check_return_ _CRTIMP wint_t __cdecl _getwche(void);
_Check_return_ _CRTIMP wint_t __cdecl _putwch(wchar_t _WCh);
_Check_return_ _CRTIMP wint_t __cdecl _ungetwch(wint_t _WCh);
_Check_return_opt_ _CRTIMP int __cdecl _cputws(_In_z_ const wchar_t * _String);
_Check_return_opt_ _CRTIMP int __cdecl _cwprintf(_In_z_ _Printf_format_string_ const wchar_t * _Format, ...);
_Check_return_opt_ _CRTIMP int __cdecl _cwprintf_s(_In_z_ _Printf_format_string_ const wchar_t * _Format, ...);
_Check_return_opt_ _CRT_INSECURE_DEPRECATE(_cwscanf_s) _CRTIMP int __cdecl _cwscanf(_In_z_ _Scanf_format_string_ const wchar_t * _Format, ...);
_Check_return_opt_ _CRT_INSECURE_DEPRECATE(_cwscanf_s_l) _CRTIMP int __cdecl _cwscanf_l(_In_z_ _Scanf_format_string_params_(0) const wchar_t * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRTIMP int __cdecl _cwscanf_s(_In_z_ _Scanf_s_format_string_ const wchar_t * _Format, ...);
_Check_return_opt_ _CRTIMP int __cdecl _cwscanf_s_l(_In_z_ _Scanf_s_format_string_params_(0) const wchar_t * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRTIMP int __cdecl _vcwprintf(_In_z_ _Printf_format_string_ const wchar_t *_Format, va_list _ArgList);
_Check_return_opt_ _CRTIMP int __cdecl _vcwprintf_s(_In_z_ _Printf_format_string_ const wchar_t *_Format, va_list _ArgList);

_Check_return_opt_ _CRTIMP int __cdecl _cwprintf_p(_In_z_ _Printf_format_string_ const wchar_t * _Format, ...);
_Check_return_opt_ _CRTIMP int __cdecl _vcwprintf_p(_In_z_ _Printf_format_string_ const wchar_t*  _Format, va_list _ArgList);

_CRTIMP int __cdecl _cwprintf_l(_In_z_ _Printf_format_string_params_(0) const wchar_t * _Format, _In_opt_ _locale_t _Locale, ...);
_CRTIMP int __cdecl _cwprintf_s_l(_In_z_ _Printf_format_string_params_(0) const wchar_t * _Format, _In_opt_ _locale_t _Locale, ...);
_CRTIMP int __cdecl _vcwprintf_l(_In_z_ _Printf_format_string_params_(2) const wchar_t *_Format, _In_opt_ _locale_t _Locale, va_list _ArgList);
_CRTIMP int __cdecl _vcwprintf_s_l(_In_z_ _Printf_format_string_params_(2) const wchar_t * _Format, _In_opt_ _locale_t _Locale, va_list _ArgList);
_CRTIMP int __cdecl _cwprintf_p_l(_In_z_ _Printf_format_string_params_(0) const wchar_t * _Format, _In_opt_ _locale_t _Locale, ...);
_CRTIMP int __cdecl _vcwprintf_p_l(_In_z_ _Printf_format_string_params_(2) const wchar_t * _Format, _In_opt_ _locale_t _Locale, va_list _ArgList);

_Check_return_opt_ wint_t __cdecl _putwch_nolock(wchar_t _WCh);
_Check_return_ wint_t __cdecl _getwch_nolock(void);
_Check_return_ wint_t __cdecl _getwche_nolock(void);
_Check_return_opt_ wint_t __cdecl _ungetwch_nolock(wint_t _WCh);

#define _WCONIO_DEFINED
#endif  /* _WCONIO_DEFINED */

#endif  /* _CRT_USE_WINAPI_FAMILY_DESKTOP_APP */


#ifndef _WSTDIO_DEFINED

/* wide function prototypes, also declared in stdio.h  */

#ifndef WEOF
#define WEOF (wint_t)(0xFFFF)
#endif  /* WEOF */

_Check_return_ _CRTIMP FILE * __cdecl _wfsopen(_In_z_ const wchar_t * _Filename, _In_z_ const wchar_t * _Mode, _In_ int _ShFlag);

_Check_return_opt_ _CRTIMP wint_t __cdecl fgetwc(_Inout_ FILE * _File);
_Check_return_opt_ _CRTIMP wint_t __cdecl _fgetwchar(void);
_Check_return_opt_ _CRTIMP wint_t __cdecl fputwc(_In_ wchar_t _Ch, _Inout_ FILE * _File);
_Check_return_opt_ _CRTIMP wint_t __cdecl _fputwchar(_In_ wchar_t _Ch);
_Check_return_ _CRTIMP wint_t __cdecl getwc(_Inout_ FILE * _File);
_Check_return_ _CRTIMP wint_t __cdecl getwchar(void);
_Check_return_opt_ _CRTIMP wint_t __cdecl putwc(_In_ wchar_t _Ch, _Inout_ FILE * _File);
_Check_return_opt_ _CRTIMP wint_t __cdecl putwchar(_In_ wchar_t _Ch);
_Check_return_opt_ _CRTIMP wint_t __cdecl ungetwc(_In_ wint_t _Ch, _Inout_ FILE * _File);

_Check_return_opt_ _CRTIMP wchar_t * __cdecl fgetws(_Out_writes_z_(_SizeInWords) wchar_t * _Dst, _In_ int _SizeInWords, _Inout_ FILE * _File);
_Check_return_opt_ _CRTIMP int __cdecl fputws(_In_z_ const wchar_t * _Str, _Inout_ FILE * _File);
_Check_return_opt_ _CRTIMP wchar_t * __cdecl _getws_s(_Out_writes_z_(_SizeInWords) wchar_t * _Str, _In_ size_t _SizeInWords);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_0(wchar_t *, _getws_s, _Post_z_ wchar_t, _String)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0(wchar_t *, __RETURN_POLICY_SAME, _CRTIMP, _getws, _Pre_notnull_ _Post_z_, wchar_t, _String)
_Check_return_opt_ _CRTIMP int __cdecl _putws(_In_z_ const wchar_t * _Str);

_Check_return_opt_ _CRTIMP int __cdecl fwprintf(_Inout_ FILE * _File, _In_z_ _Printf_format_string_ const wchar_t * _Format, ...);
#if __STDC_WANT_SECURE_LIB__
_Check_return_opt_ _CRTIMP int __cdecl fwprintf_s(_Inout_ FILE * _File, _In_z_ _Printf_format_string_ const wchar_t * _Format, ...);
#endif  /* __STDC_WANT_SECURE_LIB__ */
_Check_return_opt_ _CRTIMP int __cdecl wprintf(_In_z_ _Printf_format_string_ const wchar_t * _Format, ...);
#if __STDC_WANT_SECURE_LIB__
_Check_return_opt_ _CRTIMP int __cdecl wprintf_s(_In_z_ _Printf_format_string_ const wchar_t * _Format, ...);
#endif  /* __STDC_WANT_SECURE_LIB__ */
_Check_return_ _CRTIMP int __cdecl _scwprintf(_In_z_ _Printf_format_string_ const wchar_t * _Format, ...);
_Check_return_opt_ _CRTIMP int __cdecl vfwprintf(_Inout_ FILE * _File, _In_z_ _Printf_format_string_ const wchar_t * _Format, va_list _ArgList);
_Check_return_opt_ _CRTIMP int __cdecl vfwscanf(_Inout_ FILE * _File, _In_z_ _Printf_format_string_ const wchar_t * _Format, va_list _ArgList);
#if __STDC_WANT_SECURE_LIB__
_Check_return_opt_ _CRTIMP int __cdecl vfwprintf_s(_Inout_ FILE * _File, _In_z_ _Printf_format_string_ const wchar_t * _Format, va_list _ArgList);
_Check_return_opt_ _CRTIMP int __cdecl vfwscanf_s(_Inout_ FILE * _File, _In_z_ _Printf_format_string_ const wchar_t * _Format, va_list _ArgList);
#endif  /* __STDC_WANT_SECURE_LIB__ */
_Check_return_opt_ _CRTIMP int __cdecl vwprintf(_In_z_ _Printf_format_string_ const wchar_t * _Format, va_list _ArgList);
_Check_return_opt_ _CRTIMP int __cdecl vwscanf(_In_z_ _Printf_format_string_ const wchar_t * _Format, va_list _ArgList);
#if __STDC_WANT_SECURE_LIB__
_Check_return_opt_ _CRTIMP int __cdecl vwprintf_s(_In_z_ _Printf_format_string_ const wchar_t * _Format, va_list _ArgList);
_Check_return_opt_ _CRTIMP int __cdecl vwscanf_s(_In_z_ _Printf_format_string_ const wchar_t * _Format, va_list _ArgList);
#endif  /* __STDC_WANT_SECURE_LIB__ */

#if __STDC_WANT_SECURE_LIB__
_CRTIMP_ALTERNATIVE int __cdecl swprintf_s(_Out_writes_z_(_SizeInWords) wchar_t * _Dst, _In_ size_t _SizeInWords, _In_z_ _Printf_format_string_ const wchar_t * _Format, ...);
#endif  /* __STDC_WANT_SECURE_LIB__ */
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1_ARGLIST(int, swprintf_s, vswprintf_s, _Post_z_ wchar_t, _Dest, _In_z_ _Printf_format_string_ const wchar_t *, _Format)
#if __STDC_WANT_SECURE_LIB__
_CRTIMP_ALTERNATIVE int __cdecl vswprintf_s(_Out_writes_z_(_SizeInWords) wchar_t * _Dst, _In_ size_t _SizeInWords, _In_z_ _Printf_format_string_ const wchar_t * _Format, va_list _ArgList);
_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl vswscanf_s(_Out_writes_z_(_SizeInWords) wchar_t * _Dst, _In_ size_t _SizeInWords, _In_z_ _Printf_format_string_ const wchar_t * _Format, va_list _ArgList);
#endif  /* __STDC_WANT_SECURE_LIB__ */
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(int, vswprintf_s, _Post_z_ wchar_t, _Dest, _In_z_ _Printf_format_string_ const wchar_t *, _Format, va_list, _Args)
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(int, vswscanf_s, _Post_z_ wchar_t, _Dest, _In_z_ _Printf_format_string_ const wchar_t *, _Format, va_list, _Args)
_Check_return_opt_ _CRTIMP int __cdecl vswscanf(const wchar_t * _srcBuf, _In_z_ _Printf_format_string_ const wchar_t * _Format, va_list _ArgList);

_Check_return_opt_ _CRTIMP int __cdecl _swprintf_c(_Out_writes_z_(_SizeInWords) wchar_t * _DstBuf, _In_ size_t _SizeInWords, _In_z_ _Printf_format_string_ const wchar_t * _Format, ...);
_Check_return_opt_ _CRTIMP int __cdecl _vswprintf_c(_Out_writes_z_(_SizeInWords) wchar_t * _DstBuf, _In_ size_t _SizeInWords, _In_z_ _Printf_format_string_ const wchar_t * _Format, va_list _ArgList);

_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl _snwprintf_s(_Out_writes_z_(_DstSizeInWords) wchar_t * _DstBuf, _In_ size_t _DstSizeInWords, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_ const wchar_t * _Format, ...);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2_ARGLIST(int, _snwprintf_s, _vsnwprintf_s, _Post_z_ wchar_t, _Dest, _In_ size_t, _Count, _In_z_ _Printf_format_string_ const wchar_t *, _Format)
_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl _vsnwprintf_s(_Out_writes_z_(_DstSizeInWords) wchar_t * _DstBuf, _In_ size_t _DstSizeInWords, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_ const wchar_t * _Format, va_list _ArgList);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_3(int, _vsnwprintf_s, _Post_z_ wchar_t, _Dest, _In_ size_t, _Count, _In_z_ _Printf_format_string_ const wchar_t *, _Format, va_list, _Args)
__DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_ARGLIST_EX(int, __RETURN_POLICY_SAME, _CRTIMP, _snwprintf, _vsnwprintf, _Pre_notnull_ _Post_maybez_ wchar_t, _Out_writes_(_Count) _Post_maybez_, wchar_t, _Dest, _In_ size_t, _Count, _In_z_ _Printf_format_string_ const wchar_t *, _Format)

_Check_return_opt_ _CRTIMP int __cdecl _fwprintf_p(_Inout_ FILE * _File, _In_z_ _Printf_format_string_ const wchar_t * _Format, ...);
_Check_return_opt_ _CRTIMP int __cdecl _wprintf_p(_In_z_ _Printf_format_string_ const wchar_t * _Format, ...);
_Check_return_opt_ _CRTIMP int __cdecl _vfwprintf_p(_Inout_ FILE * _File, _In_z_ _Printf_format_string_ const wchar_t * _Format, va_list _ArgList);
_Check_return_opt_ _CRTIMP int __cdecl _vwprintf_p(_In_z_ _Printf_format_string_ const wchar_t * _Format, va_list _ArgList);
_Check_return_opt_ _CRTIMP int __cdecl _swprintf_p(_Out_writes_z_(_MaxCount) wchar_t * _DstBuf, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_ const wchar_t * _Format, ...);
_Check_return_opt_ _CRTIMP int __cdecl _vswprintf_p(_Out_writes_z_(_MaxCount) wchar_t * _DstBuf, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_ const wchar_t * _Format, va_list _ArgList);
_Check_return_ _CRTIMP int __cdecl _scwprintf_p(_In_z_ _Printf_format_string_ const wchar_t * _Format, ...);
_Check_return_ _CRTIMP int __cdecl _vscwprintf_p(_In_z_ _Printf_format_string_ const wchar_t * _Format, va_list _ArgList);

_Check_return_opt_ _CRTIMP int __cdecl _wprintf_l(_In_z_ _Printf_format_string_params_(0) const wchar_t * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRTIMP int __cdecl _wprintf_p_l(_In_z_ _Printf_format_string_params_(0) const wchar_t * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRTIMP int __cdecl _wprintf_s_l(_In_z_ _Printf_format_string_params_(0) const wchar_t * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRTIMP int __cdecl _vwprintf_l(_In_z_ _Printf_format_string_params_(2) const wchar_t * _Format, _In_opt_ _locale_t _Locale, va_list _ArgList);
_Check_return_opt_ _CRTIMP int __cdecl _vwprintf_p_l(_In_z_ _Printf_format_string_params_(2) const wchar_t * _Format, _In_opt_ _locale_t _Locale, va_list _ArgList);
_Check_return_opt_ _CRTIMP int __cdecl _vwprintf_s_l(_In_z_ _Printf_format_string_params_(2) const wchar_t * _Format, _In_opt_ _locale_t _Locale, va_list _ArgList);

_Check_return_opt_ _CRTIMP int __cdecl _fwprintf_l(_Inout_ FILE * _File, _In_z_ _Printf_format_string_params_(0) const wchar_t * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRTIMP int __cdecl _fwprintf_p_l(_Inout_ FILE * _File, _In_z_ _Printf_format_string_params_(0) const wchar_t * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRTIMP int __cdecl _fwprintf_s_l(_Inout_ FILE * _File, _In_z_ _Printf_format_string_params_(0) const wchar_t * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRTIMP int __cdecl _vfwprintf_l(_Inout_ FILE * _File, _In_z_ _Printf_format_string_params_(2) const wchar_t * _Format, _In_opt_ _locale_t _Locale, va_list _ArgList);
_Check_return_opt_ _CRTIMP int __cdecl _vfwprintf_p_l(_Inout_ FILE * _File, _In_z_ _Printf_format_string_params_(2) const wchar_t * _Format, _In_opt_ _locale_t _Locale, va_list _ArgList);
_Check_return_opt_ _CRTIMP int __cdecl _vfwprintf_s_l(_Inout_ FILE * _File, _In_z_ _Printf_format_string_params_(2) const wchar_t * _Format, _In_opt_ _locale_t _Locale, va_list _ArgList);

_Check_return_opt_ _CRTIMP int __cdecl _swprintf_c_l(_Out_writes_z_(_MaxCount) wchar_t * _DstBuf, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_params_(0) const wchar_t * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRTIMP int __cdecl _swprintf_p_l(_Out_writes_z_(_MaxCount) wchar_t * _DstBuf, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_params_(0) const wchar_t * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl _swprintf_s_l(_Out_writes_z_(_DstSize) wchar_t * _DstBuf, _In_ size_t _DstSize, _In_z_ _Printf_format_string_params_(0) const wchar_t * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRTIMP int __cdecl _vswprintf_c_l(_Out_writes_z_(_MaxCount) wchar_t * _DstBuf, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_params_(2) const wchar_t * _Format, _In_opt_ _locale_t _Locale, va_list _ArgList);
_Check_return_opt_ _CRTIMP int __cdecl _vswprintf_p_l(_Out_writes_z_(_MaxCount) wchar_t * _DstBuf, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_params_(2) const wchar_t * _Format, _In_opt_ _locale_t _Locale, va_list _ArgList);
_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl _vswprintf_s_l(_Out_writes_z_(_DstSize) wchar_t * _DstBuf, _In_ size_t _DstSize, _In_z_ _Printf_format_string_params_(2) const wchar_t * _Format, _In_opt_ _locale_t _Locale, va_list _ArgList);

_Check_return_ _CRTIMP int __cdecl _scwprintf_l(_In_z_ _Printf_format_string_params_(0) const wchar_t * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_ _CRTIMP int __cdecl _scwprintf_p_l(_In_z_ _Printf_format_string_params_(0) const wchar_t * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_ _CRTIMP int __cdecl _vscwprintf_p_l(_In_z_ _Printf_format_string_params_(2) const wchar_t * _Format, _In_opt_ _locale_t _Locale, va_list _ArgList);

_Check_return_opt_ _CRT_INSECURE_DEPRECATE(_snwprintf_s_l) _CRTIMP int __cdecl _snwprintf_l(_Out_writes_(_MaxCount) wchar_t * _DstBuf, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_params_(0) const wchar_t * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl _snwprintf_s_l(_Out_writes_z_(_DstSize) wchar_t * _DstBuf, _In_ size_t _DstSize, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_params_(0) const wchar_t * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRT_INSECURE_DEPRECATE(_vsnwprintf_s_l) _CRTIMP int __cdecl _vsnwprintf_l(_Out_writes_(_MaxCount) wchar_t * _DstBuf, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_params_(2) const wchar_t * _Format, _In_opt_ _locale_t _Locale, va_list _ArgList);
_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl _vsnwprintf_s_l(_Out_writes_z_(_DstSize) wchar_t * _DstBuf, _In_ size_t _DstSize, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_params_(2) const wchar_t * _Format, _In_opt_ _locale_t _Locale, va_list _ArgList);

#ifndef _CRT_NON_CONFORMING_SWPRINTFS

#define _SWPRINTFS_DEPRECATED _CRT_DEPRECATE_TEXT("swprintf has been changed to conform with the ISO C standard, adding an extra character count parameter. To use traditional Microsoft swprintf, set _CRT_NON_CONFORMING_SWPRINTFS.")

#else  /* _CRT_NON_CONFORMING_SWPRINTFS */

#define _SWPRINTFS_DEPRECATED

#endif  /* _CRT_NON_CONFORMING_SWPRINTFS */

/* we could end up with a double deprecation, disable warnings 4141 and 4996 */
#pragma warning(push)
#pragma warning(disable:4141 4996)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_ARGLIST_EX(int, __RETURN_POLICY_SAME, _SWPRINTFS_DEPRECATED _CRTIMP, _swprintf, _swprintf_s, _vswprintf, vswprintf_s, _Pre_notnull_ _Post_z_, wchar_t, _Dest, _In_z_ _Printf_format_string_ const wchar_t *, _Format)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_2_ARGLIST_EX(int, __RETURN_POLICY_SAME, _SWPRINTFS_DEPRECATED _CRTIMP, __swprintf_l, __vswprintf_l, _vswprintf_s_l, _Pre_notnull_ _Post_z_ wchar_t, _Out_, wchar_t, _Dest, _In_z_ _Printf_format_string_params_(2) const wchar_t *, _Format, _locale_t, _Plocinfo)
#pragma warning(pop)

#if !defined (RC_INVOKED) && !defined (__midl)
#include <swprintf.inl>
#endif  /* !defined (RC_INVOKED) && !defined (__midl) */

#ifdef _CRT_NON_CONFORMING_SWPRINTFS
#ifndef __cplusplus
#define swprintf _swprintf
#define vswprintf _vswprintf
#define _swprintf_l __swprintf_l
#define _vswprintf_l __vswprintf_l
#endif  /* __cplusplus */
#endif  /* _CRT_NON_CONFORMING_SWPRINTFS */

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma push_macro("_wtempnam")
#undef _wtempnam
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

_Check_return_ _CRTIMP wchar_t * __cdecl _wtempnam(_In_opt_z_ const wchar_t * _Directory, _In_opt_z_ const wchar_t * _FilePrefix);

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma pop_macro("_wtempnam")
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

_Check_return_ _CRTIMP int __cdecl _vscwprintf(_In_z_ _Printf_format_string_ const wchar_t * _Format, va_list _ArgList);
_Check_return_ _CRTIMP int __cdecl _vscwprintf_l(_In_z_ _Printf_format_string_params_(2) const wchar_t * _Format, _In_opt_ _locale_t _Locale, va_list _ArgList);
_Check_return_ _CRT_INSECURE_DEPRECATE(fwscanf_s) _CRTIMP int __cdecl fwscanf(_Inout_ FILE * _File, _In_z_ _Scanf_format_string_ const wchar_t * _Format, ...);
_Check_return_opt_ _CRT_INSECURE_DEPRECATE(_fwscanf_s_l) _CRTIMP int __cdecl _fwscanf_l(_Inout_ FILE * _File, _In_z_ _Scanf_format_string_params_(0) const wchar_t * _Format, _In_opt_ _locale_t _Locale, ...);
#if __STDC_WANT_SECURE_LIB__
_Check_return_opt_ _CRTIMP int __cdecl fwscanf_s(_Inout_ FILE * _File, _In_z_ _Scanf_s_format_string_ const wchar_t * _Format, ...);
#endif  /* __STDC_WANT_SECURE_LIB__ */
_Check_return_opt_ _CRTIMP int __cdecl _fwscanf_s_l(_Inout_ FILE * _File, _In_z_ _Scanf_s_format_string_params_(0) const wchar_t * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_ _CRT_INSECURE_DEPRECATE(swscanf_s) _CRTIMP int __cdecl swscanf(_In_z_ const wchar_t * _Src, _In_z_ _Scanf_format_string_ const wchar_t * _Format, ...);
_Check_return_opt_ _CRT_INSECURE_DEPRECATE(_swscanf_s_l) _CRTIMP int __cdecl _swscanf_l(_In_z_ const wchar_t * _Src, _In_z_ _Scanf_format_string_params_(0) const wchar_t * _Format, _In_opt_ _locale_t _Locale, ...);
#if __STDC_WANT_SECURE_LIB__
_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl swscanf_s(_In_z_ const wchar_t *_Src, _In_z_ _Scanf_s_format_string_ const wchar_t * _Format, ...);
#endif  /* __STDC_WANT_SECURE_LIB__ */
_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl _swscanf_s_l(_In_z_ const wchar_t * _Src, _In_z_ _Scanf_s_format_string_params_(0) const wchar_t * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRT_INSECURE_DEPRECATE(_snwscanf_s) _CRTIMP int __cdecl _snwscanf(_In_reads_(_MaxCount) _Pre_z_ const wchar_t * _Src, _In_ size_t _MaxCount, _In_z_ _Scanf_format_string_ const wchar_t * _Format, ...);
_Check_return_opt_ _CRT_INSECURE_DEPRECATE(_snwscanf_s_l) _CRTIMP int __cdecl _snwscanf_l(_In_reads_(_MaxCount) _Pre_z_ const wchar_t * _Src, _In_ size_t _MaxCount, _In_z_ _Scanf_format_string_params_(0) const wchar_t * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl _snwscanf_s(_In_reads_(_MaxCount) _Pre_z_ const wchar_t * _Src, _In_ size_t _MaxCount, _In_z_ _Scanf_s_format_string_ const wchar_t * _Format, ...);
_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl _snwscanf_s_l(_In_reads_(_MaxCount) _Pre_z_ const wchar_t * _Src, _In_ size_t _MaxCount, _In_z_ _Scanf_s_format_string_params_(0) const wchar_t * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_ _CRT_INSECURE_DEPRECATE(wscanf_s) _CRTIMP int __cdecl wscanf(_In_z_ _Scanf_format_string_ const wchar_t * _Format, ...);
_Check_return_opt_ _CRT_INSECURE_DEPRECATE(_wscanf_s_l) _CRTIMP int __cdecl _wscanf_l(_In_z_ _Scanf_format_string_params_(0) const wchar_t * _Format, _In_opt_ _locale_t _Locale, ...);
#if __STDC_WANT_SECURE_LIB__
_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl wscanf_s(_In_z_ _Scanf_s_format_string_ const wchar_t * _Format, ...);
#endif  /* __STDC_WANT_SECURE_LIB__ */
_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl _wscanf_s_l(_In_z_ _Scanf_s_format_string_params_(0) const wchar_t * _Format, _In_opt_ _locale_t _Locale, ...);

_Check_return_ _CRTIMP FILE * __cdecl _wfdopen(_In_ int _FileHandle , _In_z_ const wchar_t * _Mode);
_Check_return_ _CRT_INSECURE_DEPRECATE(_wfopen_s) _CRTIMP FILE * __cdecl _wfopen(_In_z_ const wchar_t * _Filename, _In_z_ const wchar_t * _Mode);
_Check_return_wat_ _CRTIMP errno_t __cdecl _wfopen_s(_Outptr_result_maybenull_ FILE ** _File, _In_z_ const wchar_t * _Filename, _In_z_ const wchar_t * _Mode);
_Check_return_ _CRT_INSECURE_DEPRECATE(_wfreopen_s) _CRTIMP FILE * __cdecl _wfreopen(_In_z_ const wchar_t * _Filename, _In_z_ const wchar_t * _Mode, _Inout_ FILE * _OldFile);
_Check_return_wat_ _CRTIMP errno_t __cdecl _wfreopen_s(_Outptr_result_maybenull_ FILE ** _File, _In_z_ const wchar_t * _Filename, _In_z_ const wchar_t * _Mode, _Inout_ FILE * _OldFile);

#ifndef _CRT_WPERROR_DEFINED
#define _CRT_WPERROR_DEFINED
_CRTIMP void __cdecl _wperror(_In_opt_z_ const wchar_t * _ErrMsg);
#endif  /* _CRT_WPERROR_DEFINED */

#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP
_Check_return_ _CRTIMP FILE * __cdecl _wpopen(_In_z_ const wchar_t *_Command, _In_z_ const wchar_t * _Mode);
#endif  /* _CRT_USE_WINAPI_FAMILY_DESKTOP_APP */

_CRTIMP int __cdecl _wremove(_In_z_ const wchar_t * _Filename);
_Check_return_wat_ _CRTIMP errno_t __cdecl _wtmpnam_s(_Out_writes_z_(_SizeInWords) wchar_t * _DstBuf, _In_ size_t _SizeInWords);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_0(errno_t, _wtmpnam_s, _Post_z_ wchar_t, _Buffer)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0(wchar_t *, __RETURN_POLICY_DST, _CRTIMP, _wtmpnam, _Pre_maybenull_ _Post_z_, wchar_t, _Buffer)

_Check_return_opt_ _CRTIMP wint_t __cdecl _fgetwc_nolock(_Inout_ FILE * _File);
_Check_return_opt_ _CRTIMP wint_t __cdecl _fputwc_nolock(_In_ wchar_t _Ch, _Inout_ FILE * _File);
_Check_return_opt_ _CRTIMP wint_t __cdecl _ungetwc_nolock(_In_ wint_t _Ch, _Inout_ FILE * _File);

#ifdef _CRTBLD
#define _CRT_GETPUTWCHAR_NOINLINE
#else  /* _CRTBLD */
#undef _CRT_GETPUTWCHAR_NOINLINE
#endif  /* _CRTBLD */

#if !defined (__cplusplus) || defined (_M_CEE_PURE) || defined (_CRT_GETPUTWCHAR_NOINLINE)
#define getwchar()      fgetwc(stdin)
#define putwchar(_c)    fputwc((_c),stdout)
#else  /* !defined (__cplusplus) || defined (_M_CEE_PURE) || defined (_CRT_GETPUTWCHAR_NOINLINE) */
_Check_return_ inline wint_t __CRTDECL getwchar(void)
        {return (fgetwc(stdin)); }   /* stdin */
_Check_return_opt_ inline wint_t __CRTDECL putwchar(_In_ wchar_t _C)
        {return (fputwc(_C, stdout)); }       /* stdout */
#endif  /* !defined (__cplusplus) || defined (_M_CEE_PURE) || defined (_CRT_GETPUTWCHAR_NOINLINE) */

#define getwc(_stm)             fgetwc(_stm)
#define putwc(_c,_stm)          fputwc(_c,_stm)
#define _putwc_nolock(_c,_stm)     _fputwc_nolock(_c,_stm)
#define _getwc_nolock(_c)          _fgetwc_nolock(_c)

#if defined (_CRT_DISABLE_PERFCRIT_LOCKS) && !defined (_DLL)
#define fgetwc(_stm)            _getwc_nolock(_stm)
#define fputwc(_c,_stm)         _putwc_nolock(_c,_stm)
#define ungetwc(_c,_stm)        _ungetwc_nolock(_c,_stm)
#endif  /* defined (_CRT_DISABLE_PERFCRIT_LOCKS) && !defined (_DLL) */

#define _WSTDIO_DEFINED
#endif  /* _WSTDIO_DEFINED */

#ifndef _WSTDLIB_DEFINED

/* wide function prototypes, also declared in stdlib.h  */

_Check_return_wat_ _CRTIMP errno_t __cdecl _itow_s (_In_ int _Val, _Out_writes_z_(_SizeInWords) wchar_t * _DstBuf, _In_ size_t _SizeInWords, _In_ int _Radix);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_1_1(errno_t, _itow_s, _In_ int, _Value, _Post_z_ wchar_t, _Dest, _In_ int, _Radix)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_1_1(wchar_t *, __RETURN_POLICY_DST, _CRTIMP, _itow, _In_ int, _Value, _Pre_notnull_ _Post_z_, wchar_t, _Dest, _In_ int, _Radix)
_Check_return_wat_ _CRTIMP errno_t __cdecl _ltow_s (_In_ long _Val, _Out_writes_z_(_SizeInWords) wchar_t * _DstBuf, _In_ size_t _SizeInWords, _In_ int _Radix);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_1_1(errno_t, _ltow_s, _In_ long, _Value, _Post_z_ wchar_t, _Dest, _In_ int, _Radix)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_1_1(wchar_t *, __RETURN_POLICY_DST, _CRTIMP, _ltow, _In_ long, _Value, _Pre_notnull_ _Post_z_, wchar_t, _Dest, _In_ int, _Radix)
_Check_return_wat_ _CRTIMP errno_t __cdecl _ultow_s (_In_ unsigned long _Val, _Out_writes_z_(_SizeInWords) wchar_t * _DstBuf, _In_ size_t _SizeInWords, _In_ int _Radix);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_1_1(errno_t, _ultow_s, _In_ unsigned long, _Value, _Post_z_ wchar_t, _Dest, _In_ int, _Radix)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_1_1(wchar_t *, __RETURN_POLICY_DST, _CRTIMP, _ultow, _In_ unsigned long, _Value, _Pre_notnull_ _Post_z_, wchar_t, _Dest, _In_ int, _Radix)
_Check_return_ _CRTIMP double __cdecl wcstod(_In_z_ const wchar_t * _Str, _Out_opt_ _Deref_post_z_ wchar_t ** _EndPtr);
_Check_return_ _CRTIMP double __cdecl _wcstod_l(_In_z_ const wchar_t *_Str, _Out_opt_ _Deref_post_z_ wchar_t ** _EndPtr, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP long   __cdecl wcstol(_In_z_ const wchar_t *_Str, _Out_opt_ _Deref_post_z_ wchar_t ** _EndPtr, int _Radix);
_Check_return_ _CRTIMP long   __cdecl _wcstol_l(_In_z_ const wchar_t *_Str, _Out_opt_ _Deref_post_z_ wchar_t **_EndPtr, int _Radix, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP long long __cdecl wcstoll(_In_z_ const wchar_t *_Str, _Out_opt_ _Deref_post_z_ wchar_t ** _EndPtr, int _Radix);
_Check_return_ _CRTIMP long long __cdecl _wcstoll_l(_In_z_ const wchar_t *_Str, _Out_opt_ _Deref_post_z_ wchar_t ** _EndPtr, int _Radix, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP unsigned long __cdecl wcstoul(_In_z_ const wchar_t *_Str, _Out_opt_ _Deref_post_z_ wchar_t ** _EndPtr, int _Radix);
_Check_return_ _CRTIMP unsigned long __cdecl _wcstoul_l(_In_z_ const wchar_t *_Str, _Out_opt_ _Deref_post_z_ wchar_t **_EndPtr, int _Radix, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP unsigned long long __cdecl wcstoull(_In_z_ const wchar_t *_Str, _Out_opt_ _Deref_post_z_ wchar_t ** _EndPtr, int _Radix);
_Check_return_ _CRTIMP unsigned long long __cdecl _wcstoull_l(_In_z_ const wchar_t *_Str, _Out_opt_ _Deref_post_z_ wchar_t ** _EndPtr, int _Radix, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP long double __cdecl wcstold(_In_z_ const wchar_t * _Str, _Out_opt_ _Deref_post_z_ wchar_t ** _EndPtr);
_Check_return_ _CRTIMP long double __cdecl _wcstold_l(_In_z_ const wchar_t * _Str, _Out_opt_ _Deref_post_z_ wchar_t ** _EndPtr, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP float __cdecl wcstof(_In_z_ const wchar_t * _Str, _Out_opt_ _Deref_post_z_ wchar_t ** _EndPtr);
_Check_return_ _CRTIMP float __cdecl _wcstof_l(_In_z_ const wchar_t * _Str, _Out_opt_ _Deref_post_z_ wchar_t ** _EndPtr, _In_opt_ _locale_t _Locale);

#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP

_Check_return_ _CRTIMP _CRT_INSECURE_DEPRECATE(_wdupenv_s) wchar_t * __cdecl _wgetenv(_In_z_ const wchar_t * _VarName);
_Check_return_wat_ _CRTIMP errno_t __cdecl _wgetenv_s(_Out_ size_t * _ReturnSize, _Out_writes_opt_z_(_DstSizeInWords) wchar_t * _DstBuf, _In_ size_t _DstSizeInWords, _In_z_ const wchar_t * _VarName);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_1_1(errno_t, _wgetenv_s, _Out_ size_t *, _ReturnSize, _Post_z_ wchar_t, _Dest, _In_z_ const wchar_t *, _VarName)

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma push_macro("_wdupenv_s")
#undef _wdupenv_s
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

_Check_return_wat_ _CRTIMP errno_t __cdecl _wdupenv_s(_Outptr_result_buffer_maybenull_(*_BufferSizeInWords) _Outptr_result_z_ wchar_t **_Buffer, _Out_opt_ size_t *_BufferSizeInWords, _In_z_ const wchar_t *_VarName);

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma pop_macro("_wdupenv_s")
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */
#ifndef _CRT_WSYSTEM_DEFINED
#define _CRT_WSYSTEM_DEFINED
_CRTIMP int __cdecl _wsystem(_In_opt_z_ const wchar_t * _Command);
#endif  /* _CRT_WSYSTEM_DEFINED */

#endif  /* _CRT_USE_WINAPI_FAMILY_DESKTOP_APP */

_Check_return_ _CRTIMP double __cdecl _wtof(_In_z_ const wchar_t *_Str);
_Check_return_ _CRTIMP double __cdecl _wtof_l(_In_z_ const wchar_t *_Str, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl _wtoi(_In_z_ const wchar_t *_Str);
_Check_return_ _CRTIMP int __cdecl _wtoi_l(_In_z_ const wchar_t *_Str, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP long __cdecl _wtol(_In_z_ const wchar_t *_Str);
_Check_return_ _CRTIMP long __cdecl _wtol_l(_In_z_ const wchar_t *_Str, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP long long __cdecl _wtoll(_In_z_ const wchar_t *_Str);
_Check_return_ _CRTIMP long long __cdecl _wtoll_l(_In_z_ const wchar_t *_Str, _In_opt_ _locale_t _Locale);

_Check_return_wat_ _CRTIMP errno_t __cdecl _i64tow_s(_In_ __int64 _Val, _Out_writes_z_(_SizeInWords) wchar_t * _DstBuf, _In_ size_t _SizeInWords, _In_ int _Radix);
_CRTIMP _CRT_INSECURE_DEPRECATE(_i64tow_s) wchar_t * __cdecl _i64tow(_In_ __int64 _Val, _Pre_notnull_ _Post_z_ wchar_t * _DstBuf, _In_ int _Radix);
_Check_return_wat_ _CRTIMP errno_t __cdecl _ui64tow_s(_In_ unsigned __int64 _Val, _Out_writes_z_(_SizeInWords) wchar_t * _DstBuf, _In_ size_t _SizeInWords, _In_ int _Radix);
_CRTIMP _CRT_INSECURE_DEPRECATE(_ui64tow_s) wchar_t * __cdecl _ui64tow(_In_ unsigned __int64 _Val, _Pre_notnull_ _Post_z_ wchar_t * _DstBuf, _In_ int _Radix);
_Check_return_ _CRTIMP __int64   __cdecl _wtoi64(_In_z_ const wchar_t *_Str);
_Check_return_ _CRTIMP __int64   __cdecl _wtoi64_l(_In_z_ const wchar_t *_Str, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP __int64   __cdecl _wcstoi64(_In_z_ const wchar_t * _Str, _Out_opt_ _Deref_post_z_ wchar_t ** _EndPtr, _In_ int _Radix);
_Check_return_ _CRTIMP __int64   __cdecl _wcstoi64_l(_In_z_ const wchar_t * _Str, _Out_opt_ _Deref_post_z_ wchar_t ** _EndPtr, _In_ int _Radix, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP unsigned __int64  __cdecl _wcstoui64(_In_z_ const wchar_t * _Str, _Out_opt_ _Deref_post_z_ wchar_t ** _EndPtr, _In_ int _Radix);
_Check_return_ _CRTIMP unsigned __int64  __cdecl _wcstoui64_l(_In_z_ const wchar_t *_Str , _Out_opt_ _Deref_post_z_ wchar_t ** _EndPtr, _In_ int _Radix, _In_opt_ _locale_t _Locale);

#define _WSTDLIB_DEFINED
#endif  /* _WSTDLIB_DEFINED */


#ifndef _WSTDLIBP_DEFINED

/* wide function prototypes, also declared in stdlib.h  */

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma push_macro("_wfullpath")
#undef _wfullpath
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

_Check_return_ _CRTIMP wchar_t * __cdecl _wfullpath(_Out_writes_opt_z_(_SizeInWords) wchar_t * _FullPath, _In_z_ const wchar_t * _Path, _In_ size_t _SizeInWords);

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma pop_macro("_wfullpath")
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

_Check_return_wat_ _CRTIMP_ALTERNATIVE errno_t __cdecl _wmakepath_s(_Out_writes_z_(_SizeInWords) wchar_t * _PathResult, _In_ size_t _SizeInWords, _In_opt_z_ const wchar_t * _Drive, _In_opt_z_ const wchar_t * _Dir, _In_opt_z_ const wchar_t * _Filename,
        _In_opt_z_ const wchar_t * _Ext);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_4(errno_t, _wmakepath_s, _Post_z_ wchar_t, _ResultPath, _In_opt_z_ const wchar_t *, _Drive, _In_opt_z_ const wchar_t *, _Dir, _In_opt_z_ const wchar_t *, _Filename, _In_opt_z_ const wchar_t *, _Ext)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_4(void, __RETURN_POLICY_VOID, _CRTIMP, _wmakepath, _Pre_notnull_ _Post_z_, wchar_t, _ResultPath, _In_opt_z_ const wchar_t *, _Drive, _In_opt_z_ const wchar_t *, _Dir, _In_opt_z_ const wchar_t *, _Filename, _In_opt_z_ const wchar_t *, _Ext)
#ifndef _CRT_WPERROR_DEFINED
#define _CRT_WPERROR_DEFINED
_CRTIMP void __cdecl _wperror(_In_opt_z_ const wchar_t * _ErrMsg);
#endif  /* _CRT_WPERROR_DEFINED */

#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP
_Check_return_ _CRTIMP int    __cdecl _wputenv(_In_z_ const wchar_t * _EnvString);
_Check_return_wat_ _CRTIMP errno_t __cdecl _wputenv_s(_In_z_ const wchar_t * _Name, _In_z_ const wchar_t * _Value);
_CRTIMP errno_t __cdecl _wsearchenv_s(_In_z_ const wchar_t * _Filename, _In_z_ const wchar_t * _EnvVar, _Out_writes_z_(_SizeInWords) wchar_t * _ResultPath, _In_ size_t _SizeInWords);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_2_0(errno_t, _wsearchenv_s, _In_z_ const wchar_t *, _Filename, _In_z_ const wchar_t *, _EnvVar, _Post_z_ wchar_t, _ResultPath)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_2_0(void, __RETURN_POLICY_VOID, _CRTIMP, _wsearchenv, _In_z_ const wchar_t *, _Filename, _In_z_ const wchar_t *, _EnvVar, _Pre_notnull_ _Post_z_, wchar_t, _ResultPath)
#endif  /* _CRT_USE_WINAPI_FAMILY_DESKTOP_APP */

_CRT_INSECURE_DEPRECATE(_wsplitpath_s) _CRTIMP void   __cdecl _wsplitpath(_In_z_ const wchar_t * _FullPath, _Pre_maybenull_ _Post_z_ wchar_t * _Drive, _Pre_maybenull_ _Post_z_ wchar_t * _Dir, _Pre_maybenull_ _Post_z_ wchar_t * _Filename, _Pre_maybenull_ _Post_z_ wchar_t * _Ext);
_CRTIMP_ALTERNATIVE errno_t __cdecl _wsplitpath_s(_In_z_ const wchar_t * _FullPath,
                _Out_writes_opt_z_(_DriveSize) wchar_t * _Drive, _In_ size_t _DriveSize,
                _Out_writes_opt_z_(_DirSize) wchar_t * _Dir, _In_ size_t _DirSize,
                _Out_writes_opt_z_(_FilenameSize) wchar_t * _Filename, _In_ size_t _FilenameSize,
                _Out_writes_opt_z_(_ExtSize) wchar_t * _Ext, _In_ size_t _ExtSize);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_SPLITPATH(errno_t, _wsplitpath_s, wchar_t, _Path)

#define _WSTDLIBP_DEFINED
#endif  /* _WSTDLIBP_DEFINED */



#ifndef _WSTRING_DEFINED

/* wide function prototypes, also declared in string.h  */

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma push_macro("_wcsdup")
#undef _wcsdup
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

_Check_return_ _CRTIMP wchar_t * __cdecl _wcsdup(_In_z_ const wchar_t * _Str);

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma pop_macro("_wcsdup")
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

#if __STDC_WANT_SECURE_LIB__
_Check_return_wat_ _CRTIMP_ALTERNATIVE errno_t __cdecl wcscat_s(_Inout_updates_z_(_DstSize) wchar_t * _Dst, _In_ rsize_t _DstSize, const wchar_t * _Src);
#endif  /* __STDC_WANT_SECURE_LIB__ */
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(errno_t, wcscat_s, wchar_t, _Dest, _In_z_ const wchar_t *, _Source)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1(wchar_t *, __RETURN_POLICY_DST, _CRTIMP, wcscat, _Inout_updates_z_(_String_length_(_Dest) + _String_length_(_Source) + 1), wchar_t, _Dest, _In_z_ const wchar_t *, _Source)
_Check_return_ _CRTIMP _CONST_RETURN wchar_t * __cdecl wcschr(_In_z_ const wchar_t * _Str, wchar_t _Ch);
_Check_return_ _CRTIMP int __cdecl wcscmp(_In_z_ const wchar_t * _Str1, _In_z_ const wchar_t * _Str2);
#if __STDC_WANT_SECURE_LIB__
_Check_return_wat_ _CRTIMP_ALTERNATIVE errno_t __cdecl wcscpy_s(_Out_writes_z_(_DstSize) wchar_t * _Dst, _In_ rsize_t _DstSize, _In_z_ const wchar_t * _Src);
#endif  /* __STDC_WANT_SECURE_LIB__ */
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(errno_t, wcscpy_s, _Post_z_ wchar_t, _Dest, _In_z_ const wchar_t *, _Source)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1(wchar_t *, __RETURN_POLICY_DST, _CRTIMP, wcscpy, _Out_writes_z_(_String_length_(_Source) + 1), wchar_t, _Dest, _In_z_ const wchar_t *, _Source)
_Check_return_ _CRTIMP size_t __cdecl wcscspn(_In_z_ const wchar_t * _Str, _In_z_ const wchar_t * _Control);
_Check_return_ _CRTIMP size_t __cdecl wcslen(_In_z_ const wchar_t * _Str);
_Check_return_ _CRTIMP
_When_(_MaxCount > _String_length_(_Src), _Post_satisfies_(return == _String_length_(_Src)))
_When_(_MaxCount <= _String_length_(_Src), _Post_satisfies_(return == _MaxCount))
size_t __cdecl wcsnlen(_In_z_ const wchar_t * _Src, _In_ size_t _MaxCount);
#if __STDC_WANT_SECURE_LIB__
_Check_return_ static __inline
_When_(_MaxCount > _String_length_(_Src), _Post_satisfies_(return == _String_length_(_Src)))
_When_(_MaxCount <= _String_length_(_Src), _Post_satisfies_(return == _MaxCount))
size_t __CRTDECL wcsnlen_s(_In_z_ const wchar_t * _Src, _In_ size_t _MaxCount)
{
    return (_Src == NULL) ? 0 : wcsnlen(_Src, _MaxCount);
}
#endif  /* __STDC_WANT_SECURE_LIB__ */
#if __STDC_WANT_SECURE_LIB__
_Check_return_wat_ _CRTIMP_ALTERNATIVE errno_t __cdecl wcsncat_s(_Inout_updates_z_(_DstSize) wchar_t * _Dst, _In_ rsize_t _DstSize, _In_z_ const wchar_t * _Src, _In_ rsize_t _MaxCount);
#endif  /* __STDC_WANT_SECURE_LIB__ */
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(errno_t, wcsncat_s, _Prepost_z_ wchar_t, _Dest, _In_z_ const wchar_t *, _Source, _In_ size_t, _Count)

#pragma warning(push)
#pragma warning(disable:6059)
__DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_EX(wchar_t *, __RETURN_POLICY_DST, _CRTIMP, wcsncat, wcsncat_s, _Inout_updates_z_(_Size) wchar_t, _Inout_updates_z_(_Count), wchar_t, _Dest, _In_z_ const wchar_t *, _Source, _In_ size_t, _Count)
#pragma warning(pop)

_Check_return_ _CRTIMP int __cdecl wcsncmp(_In_z_ const wchar_t * _Str1, _In_z_ const wchar_t * _Str2, _In_ size_t _MaxCount);
#if __STDC_WANT_SECURE_LIB__
_Check_return_wat_ _CRTIMP_ALTERNATIVE errno_t __cdecl wcsncpy_s(_Out_writes_z_(_DstSize) wchar_t * _Dst, _In_ rsize_t _DstSize, _In_z_ const wchar_t * _Src, _In_ rsize_t _MaxCount);
#endif  /* __STDC_WANT_SECURE_LIB__ */
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(errno_t, wcsncpy_s, wchar_t, _Dest, _In_z_ const wchar_t *, _Source, _In_ size_t, _Count)
__DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_EX(wchar_t *, __RETURN_POLICY_DST, _CRTIMP, wcsncpy, wcsncpy_s, _Out_writes_z_(_Size) wchar_t, _Out_writes_(_Count) _Post_maybez_, wchar_t, _Dest, _In_z_ const wchar_t *, _Source, _In_ size_t, _Count)
_Check_return_ _CRTIMP _CONST_RETURN wchar_t * __cdecl wcspbrk(_In_z_ const wchar_t * _Str, _In_z_ const wchar_t * _Control);
_Check_return_ _CRTIMP _CONST_RETURN wchar_t * __cdecl wcsrchr(_In_z_ const wchar_t * _Str, _In_ wchar_t _Ch);
_Check_return_ _CRTIMP size_t __cdecl wcsspn(_In_z_ const wchar_t * _Str, _In_z_ const wchar_t * _Control);
_Check_return_ _Ret_maybenull_ _CRTIMP _CONST_RETURN wchar_t * __cdecl wcsstr(_In_z_ const wchar_t * _Str, _In_z_ const wchar_t * _SubStr);
_Check_return_ _CRT_INSECURE_DEPRECATE(wcstok_s) _CRTIMP wchar_t * __cdecl wcstok(_Inout_opt_z_ wchar_t * _Str, _In_z_ const wchar_t * _Delim);
_Check_return_ _CRTIMP_ALTERNATIVE wchar_t * __cdecl wcstok_s(_Inout_opt_z_ wchar_t * _Str, _In_z_ const wchar_t * _Delim, _Inout_ _Deref_prepost_opt_z_ wchar_t ** _Context);
_Check_return_ _CRT_INSECURE_DEPRECATE(_wcserror_s) _CRTIMP wchar_t * __cdecl _wcserror(_In_ int _ErrNum);
_Check_return_wat_ _CRTIMP errno_t __cdecl _wcserror_s(_Out_writes_opt_z_(_SizeInWords) wchar_t * _Buf, _In_ size_t _SizeInWords, _In_ int _ErrNum);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(errno_t, _wcserror_s, wchar_t, _Buffer, _In_ int, _Error)
_Check_return_ _CRT_INSECURE_DEPRECATE(__wcserror_s) _CRTIMP wchar_t * __cdecl __wcserror(_In_opt_z_ const wchar_t * _Str);
_Check_return_wat_ _CRTIMP errno_t __cdecl __wcserror_s(_Out_writes_opt_z_(_SizeInWords) wchar_t * _Buffer, _In_ size_t _SizeInWords, _In_z_ const wchar_t * _ErrMsg);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(errno_t, __wcserror_s, wchar_t, _Buffer, _In_z_ const wchar_t *, _ErrorMessage)

_Check_return_ _CRTIMP int __cdecl _wcsicmp(_In_z_ const wchar_t * _Str1, _In_z_ const wchar_t * _Str2);
_Check_return_ _CRTIMP int __cdecl _wcsicmp_l(_In_z_ const wchar_t * _Str1, _In_z_ const wchar_t * _Str2, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl _wcsnicmp(_In_z_ const wchar_t * _Str1, _In_z_ const wchar_t * _Str2, _In_ size_t _MaxCount);
_Check_return_ _CRTIMP int __cdecl _wcsnicmp_l(_In_z_ const wchar_t * _Str1, _In_z_ const wchar_t * _Str2, _In_ size_t _MaxCount, _In_opt_ _locale_t _Locale);
_Check_return_wat_ _CRTIMP_ALTERNATIVE errno_t __cdecl _wcsnset_s(_Inout_updates_z_(_DstSizeInWords) wchar_t * _Dst, _In_ size_t _DstSizeInWords, wchar_t _Val, _In_ size_t _MaxCount);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(errno_t, _wcsnset_s, _Prepost_z_ wchar_t, _Dst, wchar_t, _Val, _In_ size_t, _MaxCount)
__DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_EX(wchar_t *, __RETURN_POLICY_DST, _CRTIMP, _wcsnset, _wcsnset_s, _Inout_updates_z_(_Size) wchar_t, _Inout_updates_z_(_MaxCount), wchar_t, _Str, wchar_t, _Val, _In_ size_t, _MaxCount)
_CRTIMP wchar_t * __cdecl _wcsrev(_Inout_z_ wchar_t * _Str);
_Check_return_wat_ _CRTIMP_ALTERNATIVE errno_t __cdecl _wcsset_s(_Inout_updates_z_(_SizeInWords) wchar_t * _Str, _In_ size_t _SizeInWords, wchar_t _Val);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(errno_t, _wcsset_s, _Prepost_z_ wchar_t, _Str, wchar_t, _Val)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_EX(wchar_t *, __RETURN_POLICY_DST, _CRTIMP, _wcsset, _wcsset_s, _Inout_updates_z_(_Size) wchar_t, _Inout_z_, wchar_t, _Str, wchar_t, _Val)

_Check_return_wat_ _CRTIMP errno_t __cdecl _wcslwr_s(_Inout_updates_z_(_SizeInWords) wchar_t * _Str, _In_ size_t _SizeInWords);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_0(errno_t, _wcslwr_s, wchar_t, _String)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0(wchar_t *, __RETURN_POLICY_DST, _CRTIMP, _wcslwr, _Inout_z_, wchar_t, _String)
_Check_return_wat_ _CRTIMP errno_t __cdecl _wcslwr_s_l(_Inout_updates_z_(_SizeInWords) wchar_t * _Str, _In_ size_t _SizeInWords, _In_opt_ _locale_t _Locale);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(errno_t, _wcslwr_s_l, wchar_t, _String, _In_opt_ _locale_t, _Locale)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_EX(wchar_t *, __RETURN_POLICY_DST, _CRTIMP, _wcslwr_l, _wcslwr_s_l, _Inout_updates_z_(_Size) wchar_t, _Inout_z_, wchar_t, _String, _In_opt_ _locale_t, _Locale)
_Check_return_wat_ _CRTIMP errno_t __cdecl _wcsupr_s(_Inout_updates_z_(_Size) wchar_t * _Str, _In_ size_t _Size);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_0(errno_t, _wcsupr_s, wchar_t, _String)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0(wchar_t *, __RETURN_POLICY_DST, _CRTIMP, _wcsupr, _Inout_z_, wchar_t, _String)
_Check_return_wat_ _CRTIMP errno_t __cdecl _wcsupr_s_l(_Inout_updates_z_(_Size) wchar_t * _Str, _In_ size_t _Size, _In_opt_ _locale_t _Locale);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(errno_t, _wcsupr_s_l, _Prepost_z_ wchar_t, _String, _In_opt_ _locale_t, _Locale)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_EX(wchar_t *, __RETURN_POLICY_DST, _CRTIMP, _wcsupr_l, _wcsupr_s_l, _Inout_updates_z_(_Size) wchar_t, _Inout_z_, wchar_t, _String, _In_opt_ _locale_t, _Locale)
_Check_return_opt_ _CRTIMP size_t __cdecl wcsxfrm(_Out_writes_opt_(_MaxCount) _Post_maybez_ wchar_t * _Dst, _In_z_ const wchar_t * _Src, _In_ size_t _MaxCount);
_Check_return_opt_ _CRTIMP size_t __cdecl _wcsxfrm_l(_Out_writes_opt_(_MaxCount) _Post_maybez_ wchar_t * _Dst, _In_z_ const wchar_t *_Src, _In_ size_t _MaxCount, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl wcscoll(_In_z_ const wchar_t * _Str1, _In_z_ const wchar_t * _Str2);
_Check_return_ _CRTIMP int __cdecl _wcscoll_l(_In_z_ const wchar_t * _Str1, _In_z_ const wchar_t * _Str2, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl _wcsicoll(_In_z_ const wchar_t * _Str1, _In_z_ const wchar_t * _Str2);
_Check_return_ _CRTIMP int __cdecl _wcsicoll_l(_In_z_ const wchar_t * _Str1, _In_z_ const wchar_t *_Str2, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl _wcsncoll(_In_z_ const wchar_t * _Str1, _In_z_ const wchar_t * _Str2, _In_ size_t _MaxCount);
_Check_return_ _CRTIMP int __cdecl _wcsncoll_l(_In_z_ const wchar_t * _Str1, _In_z_ const wchar_t * _Str2, _In_ size_t _MaxCount, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl _wcsnicoll(_In_z_ const wchar_t * _Str1, _In_z_ const wchar_t * _Str2, _In_ size_t _MaxCount);
_Check_return_ _CRTIMP int __cdecl _wcsnicoll_l(_In_z_ const wchar_t * _Str1, _In_z_ const wchar_t * _Str2, _In_ size_t _MaxCount, _In_opt_ _locale_t _Locale);

#ifdef __cplusplus
#ifndef _CPP_WIDE_INLINES_DEFINED
#define _CPP_WIDE_INLINES_DEFINED
extern "C++" {
_Check_return_ inline wchar_t * __CRTDECL wcschr(_In_z_ wchar_t *_Str, wchar_t _Ch)
        {return ((wchar_t *)wcschr((const wchar_t *)_Str, _Ch)); }
_Check_return_ inline wchar_t * __CRTDECL wcspbrk(_In_z_ wchar_t *_Str, _In_z_ const wchar_t *_Control)
        {return ((wchar_t *)wcspbrk((const wchar_t *)_Str, _Control)); }
_Check_return_ inline wchar_t * __CRTDECL wcsrchr(_In_z_ wchar_t *_Str, _In_ wchar_t _Ch)
        {return ((wchar_t *)wcsrchr((const wchar_t *)_Str, _Ch)); }
_Check_return_ _Ret_maybenull_ inline wchar_t * __CRTDECL wcsstr(_In_z_ wchar_t *_Str, _In_z_ const wchar_t *_SubStr)
        {return ((wchar_t *)wcsstr((const wchar_t *)_Str, _SubStr)); }
}
#endif  /* _CPP_WIDE_INLINES_DEFINED */
#endif  /* __cplusplus */

#if !__STDC__

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma push_macro("wcsdup")
#undef wcsdup
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

_Check_return_ _CRT_NONSTDC_DEPRECATE(_wcsdup) _CRTIMP wchar_t * __cdecl wcsdup(_In_z_ const wchar_t * _Str);

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma pop_macro("wcsdup")
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

/* old names */
#define wcswcs wcsstr

/* prototypes for oldnames.lib functions */
_Check_return_ _CRT_NONSTDC_DEPRECATE(_wcsicmp) _CRTIMP int __cdecl wcsicmp(_In_z_ const wchar_t * _Str1, _In_z_ const wchar_t * _Str2);
_Check_return_ _CRT_NONSTDC_DEPRECATE(_wcsnicmp) _CRTIMP int __cdecl wcsnicmp(_In_z_ const wchar_t * _Str1, _In_z_ const wchar_t * _Str2, _In_ size_t _MaxCount);
_CRT_NONSTDC_DEPRECATE(_wcsnset) _CRTIMP wchar_t * __cdecl wcsnset(_Inout_updates_z_(_MaxCount) wchar_t * _Str, _In_ wchar_t _Val, _In_ size_t _MaxCount);
_CRT_NONSTDC_DEPRECATE(_wcsrev) _CRTIMP wchar_t * __cdecl wcsrev(_Inout_z_ wchar_t * _Str);
_CRT_NONSTDC_DEPRECATE(_wcsset) _CRTIMP wchar_t * __cdecl wcsset(_Inout_z_ wchar_t * _Str, wchar_t _Val);
_CRT_NONSTDC_DEPRECATE(_wcslwr) _CRTIMP wchar_t * __cdecl wcslwr(_Inout_z_ wchar_t * _Str);
_CRT_NONSTDC_DEPRECATE(_wcsupr) _CRTIMP wchar_t * __cdecl wcsupr(_Inout_z_ wchar_t * _Str);
_Check_return_ _CRT_NONSTDC_DEPRECATE(_wcsicoll) _CRTIMP int __cdecl wcsicoll(_In_z_ const wchar_t * _Str1, _In_z_ const wchar_t * _Str2);

#endif  /* !__STDC__ */

#define _WSTRING_DEFINED
#endif  /* _WSTRING_DEFINED */

#ifndef _TM_DEFINED
struct tm {
        int tm_sec;     /* seconds after the minute - [0,59] */
        int tm_min;     /* minutes after the hour - [0,59] */
        int tm_hour;    /* hours since midnight - [0,23] */
        int tm_mday;    /* day of the month - [1,31] */
        int tm_mon;     /* months since January - [0,11] */
        int tm_year;    /* years since 1900 */
        int tm_wday;    /* days since Sunday - [0,6] */
        int tm_yday;    /* days since January 1 - [0,365] */
        int tm_isdst;   /* daylight savings time flag */
        };
#define _TM_DEFINED
#endif  /* _TM_DEFINED */

#ifndef _WTIME_DEFINED

/* wide function prototypes, also declared in time.h */

_CRT_INSECURE_DEPRECATE(_wasctime_s) _CRTIMP wchar_t * __cdecl _wasctime(_In_ const struct tm * _Tm);
_CRTIMP errno_t __cdecl _wasctime_s(_Out_writes_(_SizeInWords) _Post_readable_size_(26) wchar_t *_Buf, _In_ size_t _SizeInWords, _In_ const struct tm * _Tm);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(errno_t, _wasctime_s, _Post_readable_size_(26) wchar_t, _Buffer, _In_ const struct tm *, _Time)

_CRT_INSECURE_DEPRECATE(_wctime32_s) _CRTIMP wchar_t * __cdecl _wctime32(_In_ const __time32_t *_Time);
_CRTIMP errno_t __cdecl _wctime32_s(_Out_writes_(_SizeInWords) _Post_readable_size_(26) wchar_t* _Buf, _In_ size_t _SizeInWords, _In_ const __time32_t * _Time);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(errno_t, _wctime32_s, _Post_readable_size_(26) wchar_t, _Buffer, _In_ const __time32_t *, _Time)

_CRTIMP size_t __cdecl wcsftime(_Out_writes_z_(_SizeInWords) wchar_t * _Buf, _In_ size_t _SizeInWords, _In_z_ _Printf_format_string_ const wchar_t * _Format,  _In_ const struct tm * _Tm);
_CRTIMP size_t __cdecl _wcsftime_l(_Out_writes_z_(_SizeInWords) wchar_t * _Buf, _In_ size_t _SizeInWords, _In_z_ _Printf_format_string_ const wchar_t *_Format, _In_ const struct tm *_Tm, _In_opt_ _locale_t _Locale);

_CRTIMP errno_t __cdecl _wstrdate_s(_Out_writes_(_SizeInWords) _Post_readable_size_(9) wchar_t * _Buf, _In_ size_t _SizeInWords);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_0(errno_t, _wstrdate_s, _Post_readable_size_(9) wchar_t, _Buffer)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0(wchar_t *, __RETURN_POLICY_DST, _CRTIMP, _wstrdate, _Out_writes_z_(9), wchar_t, _Buffer)

_CRTIMP errno_t __cdecl _wstrtime_s(_Out_writes_(_SizeInWords) _Post_readable_size_(9) wchar_t * _Buf, _In_ size_t _SizeInWords);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_0(errno_t, _wstrtime_s, _Post_readable_size_(9) wchar_t, _Buffer)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0(wchar_t *, __RETURN_POLICY_DST, _CRTIMP, _wstrtime, _Out_writes_z_(9), wchar_t, _Buffer)

_CRT_INSECURE_DEPRECATE(_wctime64_s) _CRTIMP wchar_t * __cdecl _wctime64(_In_ const __time64_t * _Time);
_CRTIMP errno_t __cdecl _wctime64_s(_Out_writes_(_SizeInWords) _Post_readable_size_(26) wchar_t* _Buf, _In_ size_t _SizeInWords, _In_ const __time64_t *_Time);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(errno_t, _wctime64_s, _Post_readable_size_(26) wchar_t, _Buffer, _In_ const __time64_t *, _Time)

#if !defined (RC_INVOKED) && !defined (__midl)
#include <wtime.inl>
#endif  /* !defined (RC_INVOKED) && !defined (__midl) */

#define _WTIME_DEFINED
#endif  /* _WTIME_DEFINED */


typedef int mbstate_t;
typedef wchar_t _Wint_t;

_CRTIMP wint_t __cdecl btowc(int);
_CRTIMP size_t __cdecl mbrlen(_In_reads_bytes_opt_(_SizeInBytes) _Pre_opt_z_ const char * _Ch, _In_ size_t _SizeInBytes,
                              _Out_opt_ mbstate_t * _State);
_CRTIMP size_t __cdecl mbrtowc(_Pre_maybenull_ _Post_z_ wchar_t * _DstCh, _In_reads_bytes_opt_(_SizeInBytes) _Pre_opt_z_ const char * _SrcCh,
                               _In_ size_t _SizeInBytes, _Out_opt_ mbstate_t * _State);
_CRTIMP errno_t __cdecl mbsrtowcs_s(_Out_opt_ size_t* _Retval, _Out_writes_opt_z_(_Size) wchar_t * _Dst, _In_ size_t _Size, _Inout_ _Deref_prepost_opt_valid_ const char ** _PSrc, _In_ size_t _N, _Out_opt_ mbstate_t * _State);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_1_3(errno_t, mbsrtowcs_s, _Out_opt_ size_t *, _Retval, _Post_z_ wchar_t, _Dest, _Inout_ _Deref_prepost_opt_valid_ const char **, _PSource, _In_ size_t, _Count, _Out_opt_ mbstate_t *, _State)
__DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_3_SIZE(_CRTIMP, mbsrtowcs, _Pre_notnull_ _Post_z_, wchar_t, _Dest, _Inout_ _Deref_prepost_opt_valid_ const char **, _PSrc, _In_ size_t, _Count, _Inout_opt_ mbstate_t *, _State)

_CRTIMP errno_t __cdecl wcrtomb_s(_Out_opt_ size_t * _Retval, _Out_writes_opt_z_(_SizeInBytes) char * _Dst,
        _In_ size_t _SizeInBytes, _In_ wchar_t _Ch, _Out_opt_ mbstate_t * _State);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_1_2(errno_t, wcrtomb_s, _Out_opt_ size_t *, _Retval, _Out_writes_opt_z_(_Size) char, _Dest, _In_ wchar_t, _Source, _Out_opt_ mbstate_t *, _State)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_2_SIZE(_CRTIMP, wcrtomb, _Pre_maybenull_ _Post_z_, char, _Dest, _In_ wchar_t, _Source, _Out_opt_ mbstate_t *, _State)
_CRTIMP errno_t __cdecl wcsrtombs_s(_Out_opt_ size_t * _Retval, _Out_writes_bytes_to_opt_(_SizeInBytes, *_Retval) char * _Dst,
        _In_ size_t _SizeInBytes, _Inout_ _Deref_prepost_z_ const wchar_t ** _Src, _In_ size_t _Size, _Out_opt_ mbstate_t * _State);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_1_3(errno_t, wcsrtombs_s, _Out_opt_ size_t *, _Retval, _Out_writes_opt_z_(_Size) char, _Dest, _Inout_ _Deref_prepost_z_ const wchar_t **, _PSrc, _In_ size_t, _Count, _Out_opt_ mbstate_t *, _State)
__DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_3_SIZE(_CRTIMP, wcsrtombs, _Pre_maybenull_ _Post_z_, char, _Dest, _Inout_ _Deref_prepost_z_ const wchar_t **, _PSource, _In_ size_t, _Count, _Out_opt_ mbstate_t *, _State)
_CRTIMP int __cdecl wctob(_In_ wint_t _WCh);

#ifndef __midl

/* memcpy and memmove are defined just for use in wmemcpy and wmemmove */
_CRTIMP _CRT_INSECURE_DEPRECATE_MEMORY(memmove_s) void *  __cdecl memmove(_Out_writes_bytes_all_opt_(_Size) void * _Dst, _In_reads_bytes_opt_(_Size) const void * _Src, _In_ size_t _Size);
_CRT_INSECURE_DEPRECATE_MEMORY(memcpy_s)
_Post_equal_to_(_Dst)
_At_buffer_((unsigned char*)_Dst, _Iter_, _Size, _Post_satisfies_(((unsigned char*)_Dst)[_Iter_] == ((unsigned char*)_Src)[_Iter_]))
void *  __cdecl memcpy(_Out_writes_bytes_all_(_Size) void * _Dst, _In_reads_bytes_(_Size) const void * _Src, _In_ size_t _Size);
#if __STDC_WANT_SECURE_LIB__
_CRTIMP errno_t __cdecl memcpy_s(_Out_writes_bytes_to_opt_(_DstSize, _MaxCount) void * _Dst, _In_ rsize_t _DstSize, _In_reads_bytes_opt_(_MaxCount) const void * _Src, _In_ rsize_t _MaxCount);
_CRTIMP errno_t __cdecl memmove_s(_Out_writes_bytes_to_opt_(_DstSize, _MaxCount) void * _Dst, _In_ rsize_t _DstSize, _In_reads_bytes_opt_(_MaxCount) const void * _Src, _In_ rsize_t _MaxCount);
#endif  /* __STDC_WANT_SECURE_LIB__ */
__inline int __CRTDECL fwide(_In_opt_ FILE * _F, int _M)
        {(void)_F; return (_M); }
__inline int __CRTDECL mbsinit(_In_opt_ const mbstate_t *_P)
        {return (_P == NULL || *_P == 0); }
__inline _CONST_RETURN wchar_t * __CRTDECL wmemchr(_In_reads_(_N) const wchar_t *_S, _In_ wchar_t _C, _In_ size_t _N)
        {for (; 0 < _N; ++_S, --_N)
                if (*_S == _C)
                        return (_CONST_RETURN wchar_t *)(_S);
        return (0); }
__inline int __CRTDECL wmemcmp(_In_reads_(_N) const wchar_t *_S1, _In_reads_(_N) const wchar_t *_S2, _In_ size_t _N)
        {for (; 0 < _N; ++_S1, ++_S2, --_N)
                if (*_S1 != *_S2)
                        return (*_S1 < *_S2 ? -1 : +1);
        return (0); }

_Post_equal_to_(_S1)
_At_buffer_(_S1, _Iter_, _N, _Post_satisfies_(_S1[_Iter_] == _S2[_Iter_]))
__inline _CRT_INSECURE_DEPRECATE_MEMORY(wmemcpy_s) wchar_t * __CRTDECL wmemcpy(_Out_writes_all_(_N) wchar_t *_S1, _In_reads_(_N) const wchar_t *_S2, _In_ size_t _N)
        {
#pragma warning( push )
#pragma warning( disable : 4996 6386 )
            return (wchar_t *)memcpy(_S1, _S2, _N*sizeof(wchar_t));
#pragma warning( pop )
        }

__inline _CRT_INSECURE_DEPRECATE_MEMORY(wmemmove_s) wchar_t * __CRTDECL wmemmove(_Out_writes_all_opt_(_N) wchar_t *_S1, _In_reads_opt_(_N) const wchar_t *_S2, _In_ size_t _N)
        {
#pragma warning( push )
#pragma warning( disable : 4996 6386 )
#pragma warning( disable : 6387)
                        /* prefast noise VSW 493303 */
            return (wchar_t *)memmove(_S1, _S2, _N*sizeof(wchar_t));
#pragma warning( pop )
        }

#if __STDC_WANT_SECURE_LIB__
errno_t __CRTDECL wmemcpy_s(_Out_writes_to_opt_(_N1, _N) wchar_t *_S1, _In_ rsize_t _N1, _In_reads_opt_(_N) const wchar_t *_S2, rsize_t _N);
errno_t __CRTDECL wmemmove_s(_Out_writes_to_opt_(_N1, _N) wchar_t *_S1, _In_ rsize_t _N1, _In_reads_opt_(_N) const wchar_t *_S2, _In_ rsize_t _N);
#endif  /* __STDC_WANT_SECURE_LIB__ */

_Post_equal_to_(_S)
_At_buffer_(_S, _Iter_, _N, _Post_satisfies_(_S[_Iter_] == _C))
__inline wchar_t * __CRTDECL wmemset(_Out_writes_all_(_N) wchar_t *_S, _In_ wchar_t _C, _In_ size_t _N)
        {
            wchar_t *_Su = _S;
            for (; 0 < _N; ++_Su, --_N)
            {
                *_Su = _C;
            }
            return (_S);
        }

#ifdef __cplusplus
extern "C++" {
inline wchar_t * __CRTDECL wmemchr(_In_reads_(_N) wchar_t *_S, _In_ wchar_t _C, _In_ size_t _N)
        { return (wchar_t *)wmemchr((const wchar_t *)_S, _C, _N); }
}
#endif  /* __cplusplus */
#endif  /* __midl */

#ifdef __cplusplus
}       /* end of extern "C" */
#endif  /* __cplusplus */

#pragma pack(pop)

#endif  /* _INC_WCHAR */
