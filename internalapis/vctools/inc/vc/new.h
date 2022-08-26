
/***
*new.h - declarations and definitions for C++ memory allocation functions
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Contains the declarations for C++ memory allocation functions.
*
*       [Public]
*
****/

#pragma once

#ifndef _INC_NEW
#define _INC_NEW

#ifdef __cplusplus

#ifndef _MSC_EXTENSIONS
#include <new>
#endif  /* _MSC_EXTENSIONS */

#include <crtdefs.h>

/* Protect against #define of new */
#pragma push_macro("new")
#undef  new

#ifdef _CRTBLD
#ifndef _USE_OLD_STDCPP
/* Define _CRTIMP2 */
#ifndef _CRTIMP2
#if defined (CRTDLL2)
#define _CRTIMP2 __declspec(dllexport)
#else  /* defined (CRTDLL2) */
#if defined (_DLL) && !defined (_STATIC_CPPLIB)
#define _CRTIMP2 __declspec(dllimport)
#else  /* defined (_DLL) && !defined (_STATIC_CPPLIB) */
#define _CRTIMP2
#endif  /* defined (_DLL) && !defined (_STATIC_CPPLIB) */
#endif  /* defined (CRTDLL2) */
#endif  /* _CRTIMP2 */
#endif  /* _USE_OLD_STDCPP */
#else  /* _CRTBLD */
/* Define _CRTIMP2 */
#ifndef _CRTIMP2
#if defined (_DLL) && !defined (_STATIC_CPPLIB)
#define _CRTIMP2 __declspec(dllimport)
#else  /* defined (_DLL) && !defined (_STATIC_CPPLIB) */
#define _CRTIMP2
#endif  /* defined (_DLL) && !defined (_STATIC_CPPLIB) */
#endif  /* _CRTIMP2 */
#endif  /* _CRTBLD */

#ifdef _MSC_EXTENSIONS
#if defined (_USE_OLD_STDCPP) && defined (_CRTBLD)
typedef void (__cdecl * new_handler) ();
_CRTIMP new_handler __cdecl set_new_handler(_In_opt_ new_handler _NewHandler);
#else  /* defined (_USE_OLD_STDCPP) && defined (_CRTBLD) */

namespace std {

#ifdef _M_CEE_PURE
typedef void (__clrcall * new_handler) ();
#else  /* _M_CEE_PURE */
typedef void (__cdecl * new_handler) ();
#endif  /* _M_CEE_PURE */
#ifdef _M_CEE
typedef void (__clrcall * _new_handler_m) ();
#endif  /* _M_CEE */
_CRTIMP2 new_handler __cdecl set_new_handler(_In_opt_ new_handler _NewHandler) throw();
};

#ifdef _M_CEE
using std::_new_handler_m;
#endif  /* _M_CEE */
using std::new_handler;
using std::set_new_handler;
#endif  /* defined (_USE_OLD_STDCPP) && defined (_CRTBLD) */
#endif  /* _MSC_EXTENSIONS */

#ifndef __NOTHROW_T_DEFINED
#define __NOTHROW_T_DEFINED
namespace std {
        /* placement new tag type to suppress exceptions */
        struct nothrow_t {};

        /* constant for placement new tag */
        extern const nothrow_t nothrow;
};

_Ret_maybenull_ _Post_writable_byte_size_(_Size) void *__CRTDECL operator new(size_t _Size, const std::nothrow_t&) throw();
_Ret_maybenull_ _Post_writable_byte_size_(_Size) void *__CRTDECL operator new[](size_t _Size, const std::nothrow_t&) throw();
void __CRTDECL operator delete(void *, const std::nothrow_t&) throw();
void __CRTDECL operator delete[](void *, const std::nothrow_t&) throw();
#endif  /* __NOTHROW_T_DEFINED */

#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
inline void *__CRTDECL operator new(size_t, void *_Where)
        {return (_Where); }
inline void __CRTDECL operator delete(void *, void *)
        {return; }
#endif  /* __PLACEMENT_NEW_INLINE */


/*
 * new mode flag -- when set, makes malloc() behave like new()
 */

_CRTIMP int __cdecl _query_new_mode( void );
_CRTIMP int __cdecl _set_new_mode( _In_ int _NewMode);

#ifndef _PNH_DEFINED
#ifdef _M_CEE_PURE
typedef int (__clrcall * _PNH)( size_t );
#else  /* _M_CEE_PURE */
typedef int (__cdecl * _PNH)( size_t );
#endif  /* _M_CEE_PURE */
#define _PNH_DEFINED
#endif  /* _PNH_DEFINED */

_CRTIMP _PNH __cdecl _query_new_handler( void );
_CRTIMP _PNH __cdecl _set_new_handler( _In_opt_ _PNH _NewHandler);

#pragma pop_macro("new")

#endif  /* __cplusplus */

#endif  /* _INC_NEW */
