/***
*ctype.h - character conversion macros and ctype macros
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines macros for character classification/conversion.
*       [ANSI/System V]
*
*       [Public]
*
****/

#pragma once

#ifndef _INC_CTYPE
#define _INC_CTYPE

#include <crtdefs.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#ifndef WEOF
#define WEOF (wint_t)(0xFFFF)
#endif  /* WEOF */

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

#ifdef _CRTBLD
#ifndef _CTYPE_DISABLE_MACROS
extern const unsigned char __newclmap[];
extern const unsigned char __new----ap[];
#endif  /* _CTYPE_DISABLE_MACROS */

extern pthreadlocinfo __ptlocinfo;
extern pthreadmbcinfo __ptmbcinfo;
extern int __globallocalestatus;
extern int __locale_changed;
extern struct threadlocaleinfostruct __initiallocinfo;
extern _locale_tstruct __initiallocalestructinfo;
pthreadlocinfo __cdecl __updatetlocinfo(void);
pthreadmbcinfo __cdecl __updatetmbcinfo(void);
#endif  /* _CRTBLD */


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


/* character classification function prototypes */

#ifndef _CTYPE_DEFINED

_Check_return_ _CRTIMP int __cdecl _isctype(_In_ int _C, _In_ int _Type);
_Check_return_ _CRTIMP int __cdecl _isctype_l(_In_ int _C, _In_ int _Type, _In_opt_ _locale_t _Locale);
_Check_return_ _CRT_JIT_INTRINSIC _CRTIMP int __cdecl isalpha(_In_ int _C);
_Check_return_ _CRTIMP int __cdecl _isalpha_l(_In_ int _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRT_JIT_INTRINSIC _CRTIMP int __cdecl isupper(_In_ int _C);
_Check_return_ _CRTIMP int __cdecl _isupper_l(_In_ int _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRT_JIT_INTRINSIC _CRTIMP int __cdecl islower(_In_ int _C);
_Check_return_ _CRTIMP int __cdecl _islower_l(_In_ int _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRT_JIT_INTRINSIC _CRTIMP int __cdecl isdigit(_In_ int _C);
_Check_return_ _CRTIMP int __cdecl _isdigit_l(_In_ int _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl isxdigit(_In_ int _C);
_Check_return_ _CRTIMP int __cdecl _isxdigit_l(_In_ int _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRT_JIT_INTRINSIC _CRTIMP int __cdecl isspace(_In_ int _C);
_Check_return_ _CRTIMP int __cdecl _isspace_l(_In_ int _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl ispunct(_In_ int _C);
_Check_return_ _CRTIMP int __cdecl _ispunct_l(_In_ int _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl isblank(_In_ int _C);
_Check_return_ _CRTIMP int __cdecl _isblank_l(_In_ int _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRT_JIT_INTRINSIC _CRTIMP int __cdecl isalnum(_In_ int _C);
_Check_return_ _CRTIMP int __cdecl _isalnum_l(_In_ int _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl isprint(_In_ int _C);
_Check_return_ _CRTIMP int __cdecl _isprint_l(_In_ int _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl isgraph(_In_ int _C);
_Check_return_ _CRTIMP int __cdecl _isgraph_l(_In_ int _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl iscntrl(_In_ int _C);
_Check_return_ _CRTIMP int __cdecl _iscntrl_l(_In_ int _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRT_JIT_INTRINSIC _CRTIMP int __cdecl toupper(_In_ int _C);
_Check_return_ _CRT_JIT_INTRINSIC _CRTIMP int __cdecl tolower(_In_ int _C);
_Check_return_ _CRT_JIT_INTRINSIC _CRTIMP int __cdecl _tolower(_In_ int _C);
_Check_return_ _CRTIMP int __cdecl _tolower_l(_In_ int _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRT_JIT_INTRINSIC _CRTIMP int __cdecl _toupper(_In_ int _C);
_Check_return_ _CRTIMP int __cdecl _toupper_l(_In_ int _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl __isascii(_In_ int _C);
_Check_return_ _CRTIMP int __cdecl __toascii(_In_ int _C);
_Check_return_ _CRTIMP int __cdecl __iscsymf(_In_ int _C);
_Check_return_ _CRTIMP int __cdecl __iscsym(_In_ int _C);
#define _CTYPE_DEFINED
#endif  /* _CTYPE_DEFINED */

#ifndef _WCTYPE_DEFINED

/* wide function prototypes, also declared in wchar.h  */

/* character classification function prototypes */

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
_Check_return_ _CRTIMP int __cdecl isleadbyte(_In_ int _C);
_Check_return_ _CRTIMP int __cdecl _isleadbyte_l(_In_ int _C, _In_opt_ _locale_t _Locale);
_CRT_OBSOLETE(iswctype) _CRTIMP int __cdecl is_wctype(_In_ wint_t _C, _In_ wctype_t _Type);
#endif  /* _CRT_USE_WINAPI_FAMILY_DESKTOP_APP */

#define _WCTYPE_DEFINED
#endif  /* _WCTYPE_DEFINED */

/* the character classification macro definitions */

#ifndef _CTYPE_DISABLE_MACROS

/*
 * Maximum number of bytes in multi-byte character in the current locale
 * (also defined in stdlib.h).
 */
#ifndef MB_CUR_MAX
#ifdef _CRTBLD
#if defined (_DLL) && defined (_M_IX86)
/* Retained for compatibility with VC++ 5.0 and earlier versions */
_CRTIMP int * __cdecl __p___mb_cur_max(void);
#endif  /* defined (_DLL) && defined (_M_IX86) */
#define __MB_CUR_MAX(ptloci) (ptloci)->mb_cur_max
#endif  /* _CRTBLD */

#if defined (_CRT_DISABLE_PERFCRIT_LOCKS) && !defined (_DLL)
#define MB_CUR_MAX __mb_cur_max
#else  /* defined (_CRT_DISABLE_PERFCRIT_LOCKS) && !defined (_DLL) */
#define MB_CUR_MAX ___mb_cur_max_func()
#endif  /* defined (_CRT_DISABLE_PERFCRIT_LOCKS) && !defined (_DLL) */
#if !defined (_M_CEE_PURE)
/* No data exports in pure code */
_CRTIMP extern int __mb_cur_max;
#else  /* !defined (_M_CEE_PURE) */
#define __mb_cur_max (___mb_cur_max_func())
#endif  /* !defined (_M_CEE_PURE) */
_CRTIMP int __cdecl ___mb_cur_max_func(void);
_CRTIMP int __cdecl ___mb_cur_max_l_func(_locale_t);
#endif  /* MB_CUR_MAX */

/* Introduced to detect error when character testing functions are called
 * with illegal input of integer.
 */
#ifdef _DEBUG
_CRTIMP int __cdecl _chvalidator(_In_ int _Ch, _In_ int _Mask);
#define __chvalidchk(a,b)       _chvalidator(a,b)
#else  /* _DEBUG */
#define __chvalidchk(a,b)       (__PCTYPE_FUNC[(a)] & (b))
#endif  /* _DEBUG */


#ifdef _CRTBLD
#define __ascii_isalpha(c)      ( __chvalidchk(c, _ALPHA))
#define __ascii_isdigit(c)      ( __chvalidchk(c, _DIGIT))
#define __ascii_tolower(c)      ( (((c) >= 'A') && ((c) <= 'Z')) ? ((c) - 'A' + 'a') : (c) )
#define __ascii_toupper(c)      ( (((c) >= 'a') && ((c) <= 'z')) ? ((c) - 'a' + 'A') : (c) )
#define __ascii_iswalpha(c)     ( ('A' <= (c) && (c) <= 'Z') || ( 'a' <= (c) && (c) <= 'z'))
#define __ascii_iswdigit(c)     ( '0' <= (c) && (c) <= '9')
#define __ascii_towlower(c)     ( (((c) >= L'A') && ((c) <= L'Z')) ? ((c) - L'A' + L'a') : (c) )
#define __ascii_towupper(c)     ( (((c) >= L'a') && ((c) <= L'z')) ? ((c) - L'a' + L'A') : (c) )
#endif  /* _CRTBLD */

#if defined (_CRT_DISABLE_PERFCRIT_LOCKS) && !defined (_DLL)
#ifndef __cplusplus
#define isalpha(_c)     (MB_CUR_MAX > 1 ? _isctype(_c,_ALPHA) : __chvalidchk(_c, _ALPHA))
#define isupper(_c)     (MB_CUR_MAX > 1 ? _isctype(_c,_UPPER) : __chvalidchk(_c, _UPPER))
#define islower(_c)     (MB_CUR_MAX > 1 ? _isctype(_c,_LOWER) : __chvalidchk(_c, _LOWER))
#define isdigit(_c)     (MB_CUR_MAX > 1 ? _isctype(_c,_DIGIT) : __chvalidchk(_c, _DIGIT))
#define isxdigit(_c)    (MB_CUR_MAX > 1 ? _isctype(_c,_HEX)   : __chvalidchk(_c, _HEX))
#define isspace(_c)     (MB_CUR_MAX > 1 ? _isctype(_c,_SPACE) : __chvalidchk(_c, _SPACE))
#define ispunct(_c)     (MB_CUR_MAX > 1 ? _isctype(_c,_PUNCT) : __chvalidchk(_c, _PUNCT))
#define isblank(_c)     (MB_CUR_MAX > 1 ? _isctype(_c,_BLANK) : __chvalidchk(_c, _BLANK))
#define isalnum(_c)     (MB_CUR_MAX > 1 ? _isctype(_c,_ALPHA|_DIGIT) : __chvalidchk(_c, (_ALPHA|_DIGIT)))
#define isprint(_c)     (MB_CUR_MAX > 1 ? _isctype(_c,_BLANK|_PUNCT|_ALPHA|_DIGIT) : __chvalidchk(_c, (_BLANK|_PUNCT|_ALPHA|_DIGIT)))
#define isgraph(_c)     (MB_CUR_MAX > 1 ? _isctype(_c,_PUNCT|_ALPHA|_DIGIT) : __chvalidchk(_c, (_PUNCT|_ALPHA|_DIGIT)))
#define iscntrl(_c)     (MB_CUR_MAX > 1 ? _isctype(_c,_CONTROL) : __chvalidchk(_c, _CONTROL))
#endif  /* __cplusplus */
#endif  /* defined (_CRT_DISABLE_PERFCRIT_LOCKS) && !defined (_DLL) */

#ifdef _DEBUG
_CRTIMP int __cdecl _chvalidator_l(_In_opt_ _locale_t, _In_ int _Ch, _In_ int _Mask);
#define _chvalidchk_l(_Char, _Flag, _Locale)  _chvalidator_l(_Locale, _Char, _Flag)
#else  /* _DEBUG */
#define _chvalidchk_l(_Char, _Flag, _Locale)  (_Locale==NULL ? __chvalidchk(_Char, _Flag) : ((_locale_t)_Locale)->locinfo->pctype[_Char] & (_Flag))
#endif  /* _DEBUG */

#ifdef _CRTBLD
#define __ascii_isalpha_l(_Char, _Locale)      ( _chvalidchk_l(_Char, _ALPHA, _Locale))
#define __ascii_isdigit_l(_Char, _Locale)      ( _chvalidchk_l(_Char, _DIGIT, _Locale))
#endif  /* _CRTBLD */

#define _ischartype_l(_Char, _Flag, _Locale)    ( ((_Locale)!=NULL && (((_locale_t)(_Locale))->locinfo->mb_cur_max) > 1) ? _isctype_l(_Char, (_Flag), _Locale) : _chvalidchk_l(_Char,_Flag,_Locale))
#define _isalpha_l(_Char, _Locale)      _ischartype_l(_Char, _ALPHA, _Locale)
#define _isupper_l(_Char, _Locale)      _ischartype_l(_Char, _UPPER, _Locale)
#define _islower_l(_Char, _Locale)      _ischartype_l(_Char, _LOWER, _Locale)
#define _isdigit_l(_Char, _Locale)      _ischartype_l(_Char, _DIGIT, _Locale)
#define _isxdigit_l(_Char, _Locale)     _ischartype_l(_Char, _HEX, _Locale)
#define _isspace_l(_Char, _Locale)      _ischartype_l(_Char, _SPACE, _Locale)
#define _ispunct_l(_Char, _Locale)      _ischartype_l(_Char, _PUNCT, _Locale)
#define _isblank_l(_Char, _Locale)      _ischartype_l(_Char, _BLANK, _Locale)
#define _isalnum_l(_Char, _Locale)      _ischartype_l(_Char, _ALPHA|_DIGIT, _Locale)
#define _isprint_l(_Char, _Locale)      _ischartype_l(_Char, _BLANK|_PUNCT|_ALPHA|_DIGIT, _Locale)
#define _isgraph_l(_Char, _Locale)      _ischartype_l(_Char, _PUNCT|_ALPHA|_DIGIT, _Locale)
#define _iscntrl_l(_Char, _Locale)      _ischartype_l(_Char, _CONTROL, _Locale)

#define _tolower(_Char)    ( (_Char)-'A'+'a' )
#define _toupper(_Char)    ( (_Char)-'a'+'A' )

#define __isascii(_Char)   ( (unsigned)(_Char) < 0x80 )
#define __toascii(_Char)   ( (_Char) & 0x7f )

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
#endif  /* !defined (__cplusplus) || defined (_M_CEE_PURE) || defined (MRTDLL) || defined (_CRT_WCTYPE_NOINLINE) */
#define _WCTYPE_INLINE_DEFINED
#endif  /* _WCTYPE_INLINE_DEFINED */

/* MS C version 2.0 extended ctype macros */

#define __iscsymf(_c)   (isalpha(_c) || ((_c) == '_'))
#define __iscsym(_c)    (isalnum(_c) || ((_c) == '_'))
#define __iswcsymf(_c)  (iswalpha(_c) || ((_c) == '_'))
#define __iswcsym(_c)   (iswalnum(_c) || ((_c) == '_'))

#define _iscsymf_l(_c, _p)   (_isalpha_l(_c, _p) || ((_c) == '_'))
#define _iscsym_l(_c, _p)    (_isalnum_l(_c, _p) || ((_c) == '_'))
#define _iswcsymf_l(_c, _p)  (iswalpha(_c) || ((_c) == '_'))
#define _iswcsym_l(_c, _p)   (iswalnum(_c) || ((_c) == '_'))

#endif  /* _CTYPE_DISABLE_MACROS */


#if !__STDC__

/* Non-ANSI names for compatibility */

#ifndef _CTYPE_DEFINED
_Check_return_ _CRT_NONSTDC_DEPRECATE(__isascii) _CRTIMP int __cdecl isascii(_In_ int _C);
_Check_return_ _CRT_NONSTDC_DEPRECATE(__toascii) _CRTIMP int __cdecl toascii(_In_ int _C);
_Check_return_ _CRT_NONSTDC_DEPRECATE(__iscsymf) _CRTIMP int __cdecl iscsymf(_In_ int _C);
_Check_return_ _CRT_NONSTDC_DEPRECATE(__iscsym) _CRTIMP int __cdecl iscsym(_In_ int _C);
#else  /* _CTYPE_DEFINED */
#define isascii __isascii
#define toascii __toascii
#define iscsymf __iscsymf
#define iscsym  __iscsym
#endif  /* _CTYPE_DEFINED */

#endif  /* !__STDC__ */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _INC_CTYPE */
