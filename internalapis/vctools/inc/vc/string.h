/***
*string.h - declarations for string manipulation functions
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file contains the function declarations for the string
*       manipulation functions.
*       [ANSI/System V]
*
*       [Public]
*
****/

#pragma once

#ifndef _INC_STRING
#define _INC_STRING

#include <crtdefs.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#ifndef _NLSCMP_DEFINED
#define _NLSCMPERROR    2147483647  /* currently == INT_MAX */
#define _NLSCMP_DEFINED
#endif  /* _NLSCMP_DEFINED */

/* Define NULL pointer value */
#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else  /* __cplusplus */
#define NULL    ((void *)0)
#endif  /* __cplusplus */
#endif  /* NULL */

/* For backwards compatibility */
#define _WConst_return _CONST_RETURN

/* Function prototypes */
#ifndef RC_INVOKED
#ifndef _CRT_MEMORY_DEFINED
#define _CRT_MEMORY_DEFINED
_CRTIMP void *  __cdecl _memccpy( _Out_writes_bytes_opt_(_MaxCount) void * _Dst, _In_ const void * _Src, _In_ int _Val, _In_ size_t _MaxCount);
_Check_return_ _CRTIMP _CONST_RETURN void *  __cdecl memchr( _In_reads_bytes_opt_(_MaxCount) const void * _Buf , _In_ int _Val, _In_ size_t _MaxCount);
_Check_return_ _CRTIMP int     __cdecl _memicmp(_In_reads_bytes_opt_(_Size) const void * _Buf1, _In_reads_bytes_opt_(_Size) const void * _Buf2, _In_ size_t _Size);
_Check_return_ _CRTIMP int     __cdecl _memicmp_l(_In_reads_bytes_opt_(_Size) const void * _Buf1, _In_reads_bytes_opt_(_Size) const void * _Buf2, _In_ size_t _Size, _In_opt_ _locale_t _Locale);
_Check_return_ int     __cdecl memcmp(_In_reads_bytes_(_Size) const void * _Buf1, _In_reads_bytes_(_Size) const void * _Buf2, _In_ size_t _Size);
_CRT_INSECURE_DEPRECATE_MEMORY(memcpy_s)
_Post_equal_to_(_Dst)
_At_buffer_((unsigned char*)_Dst, _Iter_, _Size, _Post_satisfies_(((unsigned char*)_Dst)[_Iter_] == ((unsigned char*)_Src)[_Iter_]))
void *  __cdecl memcpy(_Out_writes_bytes_all_(_Size) void * _Dst, _In_reads_bytes_(_Size) const void * _Src, _In_ size_t _Size);
#if __STDC_WANT_SECURE_LIB__
_CRTIMP errno_t  __cdecl memcpy_s(_Out_writes_bytes_to_opt_(_DstSize, _MaxCount) void * _Dst, _In_ rsize_t _DstSize, _In_reads_bytes_opt_(_MaxCount) const void * _Src, _In_ rsize_t _MaxCount);
#if defined (__cplusplus) && _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_MEMORY
extern "C++"
{
#ifndef _CRT_ENABLE_IF_DEFINED
  #define _CRT_ENABLE_IF_DEFINED
    template<bool _Enable, typename _Ty>
    struct _CrtEnableIf;

    template<typename _Ty>
    struct _CrtEnableIf<true, _Ty>
    {
        typedef _Ty _Type;
    };
#endif  /* _CRT_ENABLE_IF_DEFINED */
    template <size_t _Size, typename _DstType>
    inline
    typename _CrtEnableIf<(_Size > 1), void *>::_Type __cdecl memcpy(_DstType (&_Dst)[_Size], _In_reads_bytes_opt_(_SrcSize) const void *_Src, _In_ size_t _SrcSize) _CRT_SECURE_CPP_NOTHROW
    {
        return memcpy_s(_Dst, _Size * sizeof(_DstType), _Src, _SrcSize) == 0 ? _Dst : 0;
    }
}
#endif  /* defined (__cplusplus) && _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_MEMORY */
#if defined (__cplusplus) && _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES_MEMORY
extern "C++"
{
    template <size_t _Size, typename _DstType>
    inline
    errno_t __CRTDECL memcpy_s(_DstType (&_Dst)[_Size], _In_reads_bytes_opt_(_SrcSize) const void * _Src, _In_ rsize_t _SrcSize) _CRT_SECURE_CPP_NOTHROW
    {
        return memcpy_s(_Dst, _Size * sizeof(_DstType), _Src, _SrcSize);
    }
}
#endif  /* defined (__cplusplus) && _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES_MEMORY */
#endif  /* __STDC_WANT_SECURE_LIB__ */
        _Post_equal_to_(_Dst)
        _At_buffer_((unsigned char*)_Dst, _Iter_, _Size, _Post_satisfies_(((unsigned char*)_Dst)[_Iter_] == _Val))
        void *  __cdecl memset(_Out_writes_bytes_all_(_Size) void * _Dst, _In_ int _Val, _In_ size_t _Size);

#if !__STDC__
/* Non-ANSI names for compatibility */
_CRT_NONSTDC_DEPRECATE(_memccpy) _CRTIMP void * __cdecl memccpy(_Out_writes_bytes_opt_(_Size) void * _Dst, _In_reads_bytes_opt_(_Size) const void * _Src, _In_ int _Val, _In_ size_t _Size);
_Check_return_ _CRT_NONSTDC_DEPRECATE(_memicmp) _CRTIMP int __cdecl memicmp(_In_reads_bytes_opt_(_Size) const void * _Buf1, _In_reads_bytes_opt_(_Size) const void * _Buf2, _In_ size_t _Size);
#endif  /* !__STDC__ */

#endif  /* _CRT_MEMORY_DEFINED */
#endif  /* RC_INVOKED */

_Check_return_wat_ _CRTIMP_ALTERNATIVE errno_t __cdecl _strset_s(_Inout_updates_z_(_DstSize) char * _Dst, _In_ size_t _DstSize, _In_ int _Value);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(errno_t, _strset_s, _Prepost_z_ char, _Dest, _In_ int, _Value)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1(char *, __RETURN_POLICY_DST, __EMPTY_DECLSPEC, _strset, _Inout_z_, char, _Dest, _In_ int, _Value)
#if __STDC_WANT_SECURE_LIB__
_Check_return_wat_ _CRTIMP_ALTERNATIVE errno_t __cdecl strcpy_s(_Out_writes_z_(_SizeInBytes) char * _Dst, _In_ rsize_t _SizeInBytes, _In_z_ const char * _Src);
#endif  /* __STDC_WANT_SECURE_LIB__ */
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(errno_t, strcpy_s, _Post_z_ char, _Dest, _In_z_ const char *, _Source)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1(char *, __RETURN_POLICY_DST, __EMPTY_DECLSPEC, strcpy, _Out_writes_z_(_String_length_(_Source) + 1), char, _Dest, _In_z_ const char *, _Source)
#if __STDC_WANT_SECURE_LIB__
_Check_return_wat_ _CRTIMP_ALTERNATIVE errno_t __cdecl strcat_s(_Inout_updates_z_(_SizeInBytes) char * _Dst, _In_ rsize_t _SizeInBytes, _In_z_ const char * _Src);
#endif  /* __STDC_WANT_SECURE_LIB__ */
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(errno_t, strcat_s, char, _Dest, _In_z_ const char *, _Source)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1(char *, __RETURN_POLICY_DST, __EMPTY_DECLSPEC, strcat, _Inout_updates_z_(_String_length_(_Dest) + _String_length_(_Source) + 1), char, _Dest, _In_z_ const char *, _Source)
        _Check_return_ int     __cdecl strcmp(_In_z_ const char * _Str1, _In_z_ const char * _Str2);
        _Check_return_ size_t  __cdecl strlen(_In_z_ const char * _Str);
_Check_return_ _CRTIMP
_When_(_MaxCount > _String_length_(_Str), _Post_satisfies_(return == _String_length_(_Str)))
_When_(_MaxCount <= _String_length_(_Str), _Post_satisfies_(return == _MaxCount))
size_t  __cdecl strnlen(_In_reads_or_z_(_MaxCount) const char * _Str, _In_ size_t _MaxCount);
#if __STDC_WANT_SECURE_LIB__ && !defined (__midl)
_Check_return_ static __inline
_When_(_MaxCount > _String_length_(_Str), _Post_satisfies_(return == _String_length_(_Str)))
_When_(_MaxCount <= _String_length_(_Str), _Post_satisfies_(return == _MaxCount))
size_t  __CRTDECL strnlen_s(_In_reads_or_z_(_MaxCount) const char * _Str, _In_ size_t _MaxCount)
{
    return (_Str==0) ? 0 : strnlen(_Str, _MaxCount);
}
#endif  /* __STDC_WANT_SECURE_LIB__ && !defined (__midl) */
#if __STDC_WANT_SECURE_LIB__
_Check_return_wat_ _CRTIMP errno_t __cdecl memmove_s(_Out_writes_bytes_to_opt_(_DstSize,_MaxCount) void * _Dst, _In_ rsize_t _DstSize, _In_reads_bytes_opt_(_MaxCount) const void * _Src, _In_ rsize_t _MaxCount);
#endif  /* __STDC_WANT_SECURE_LIB__ */

_CRTIMP _CRT_INSECURE_DEPRECATE_MEMORY(memmove_s) void *  __cdecl memmove(_Out_writes_bytes_all_opt_(_Size) void * _Dst, _In_reads_bytes_opt_(_Size) const void * _Src, _In_ size_t _Size);

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma push_macro("_strdup")
#undef _strdup
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

_Check_return_ _CRTIMP char *  __cdecl _strdup(_In_opt_z_ const char * _Src);

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma pop_macro("_strdup")
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

_Check_return_ _CRTIMP _CONST_RETURN char *  __cdecl strchr(_In_z_ const char * _Str, _In_ int _Val);
_Check_return_ _CRTIMP int     __cdecl _stricmp(_In_z_  const char * _Str1, _In_z_  const char * _Str2);
_Check_return_ _CRTIMP int     __cdecl _strcmpi(_In_z_  const char * _Str1, _In_z_  const char * _Str2);
_Check_return_ _CRTIMP int     __cdecl _stricmp_l(_In_z_  const char * _Str1, _In_z_  const char * _Str2, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int     __cdecl strcoll(_In_z_  const char * _Str1, _In_z_  const  char * _Str2);
_Check_return_ _CRTIMP int     __cdecl _strcoll_l(_In_z_  const char * _Str1, _In_z_  const char * _Str2, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int     __cdecl _stricoll(_In_z_  const char * _Str1, _In_z_  const char * _Str2);
_Check_return_ _CRTIMP int     __cdecl _stricoll_l(_In_z_  const char * _Str1, _In_z_  const char * _Str2, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int     __cdecl _strncoll  (_In_reads_or_z_(_MaxCount) const char * _Str1, _In_reads_or_z_(_MaxCount) const char * _Str2, _In_ size_t _MaxCount);
_Check_return_ _CRTIMP int     __cdecl _strncoll_l(_In_reads_or_z_(_MaxCount) const char * _Str1, _In_reads_or_z_(_MaxCount) const char * _Str2, _In_ size_t _MaxCount, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int     __cdecl _strnicoll (_In_reads_or_z_(_MaxCount) const char * _Str1, _In_reads_or_z_(_MaxCount) const char * _Str2, _In_ size_t _MaxCount);
_Check_return_ _CRTIMP int     __cdecl _strnicoll_l(_In_reads_or_z_(_MaxCount) const char * _Str1, _In_reads_or_z_(_MaxCount) const char * _Str2, _In_ size_t _MaxCount, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP size_t  __cdecl strcspn(_In_z_  const char * _Str, _In_z_  const char * _Control);
_Check_return_ _CRT_INSECURE_DEPRECATE(_strerror_s) _CRTIMP char *  __cdecl _strerror(_In_opt_z_ const char * _ErrMsg);
_Check_return_wat_ _CRTIMP errno_t __cdecl _strerror_s(_Out_writes_z_(_SizeInBytes) char * _Buf, _In_ size_t _SizeInBytes, _In_opt_z_ const char * _ErrMsg);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(errno_t, _strerror_s, char, _Buffer, _In_opt_z_ const char *, _ErrorMessage)
_Check_return_ _CRT_INSECURE_DEPRECATE(strerror_s) _CRTIMP char *  __cdecl strerror(_In_ int);
#if __STDC_WANT_SECURE_LIB__
_Check_return_wat_ _CRTIMP errno_t __cdecl strerror_s(_Out_writes_z_(_SizeInBytes) char * _Buf, _In_ size_t _SizeInBytes, _In_ int _ErrNum);
#endif  /* __STDC_WANT_SECURE_LIB__ */
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(errno_t, strerror_s, char, _Buffer, _In_ int, _ErrorMessage)
_Check_return_wat_ _CRTIMP errno_t __cdecl _strlwr_s(_Inout_updates_z_(_Size) char * _Str, _In_ size_t _Size);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_0(errno_t, _strlwr_s, _Prepost_z_ char, _String)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0(char *, __RETURN_POLICY_DST, _CRTIMP, _strlwr, _Inout_z_, char, _String)
_Check_return_wat_ _CRTIMP errno_t __cdecl _strlwr_s_l(_Inout_updates_z_(_Size) char * _Str, _In_ size_t _Size, _In_opt_ _locale_t _Locale);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(errno_t, _strlwr_s_l, _Prepost_z_ char, _String, _In_opt_ _locale_t, _Locale)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_EX(char *, __RETURN_POLICY_DST, _CRTIMP, _strlwr_l, _strlwr_s_l, _Inout_updates_z_(_Size) char, _Inout_z_, char, _String, _In_opt_ _locale_t, _Locale)
#if __STDC_WANT_SECURE_LIB__
_Check_return_wat_ _CRTIMP_ALTERNATIVE errno_t __cdecl strncat_s(_Inout_updates_z_(_SizeInBytes) char * _Dst, _In_ rsize_t _SizeInBytes, _In_reads_or_z_(_MaxCount) const char * _Src, _In_ rsize_t _MaxCount);
#endif  /* __STDC_WANT_SECURE_LIB__ */
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(errno_t, strncat_s, _Prepost_z_ char, _Dest, _In_reads_or_z_(_Count) const char *, _Source, _In_ size_t, _Count)
#pragma warning(push)
#pragma warning(disable:6059)
/* prefast noise VSW 489802 */
__DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_EX(char *, __RETURN_POLICY_DST, _CRTIMP, strncat, strncat_s, _Inout_updates_z_(_Size) char, _Inout_updates_z_(_Count), char, _Dest, _In_reads_or_z_(_Count) const char *, _Source, _In_ size_t, _Count)
#pragma warning(pop)
_Check_return_ _CRTIMP int     __cdecl strncmp(_In_reads_or_z_(_MaxCount) const char * _Str1, _In_reads_or_z_(_MaxCount) const char * _Str2, _In_ size_t _MaxCount);
_Check_return_ _CRTIMP int     __cdecl _strnicmp(_In_reads_or_z_(_MaxCount) const char * _Str1, _In_reads_or_z_(_MaxCount) const char * _Str2, _In_ size_t _MaxCount);
_Check_return_ _CRTIMP int     __cdecl _strnicmp_l(_In_reads_or_z_(_MaxCount) const char * _Str1, _In_reads_or_z_(_MaxCount) const char * _Str2, _In_ size_t _MaxCount, _In_opt_ _locale_t _Locale);
#if __STDC_WANT_SECURE_LIB__
_Check_return_wat_ _CRTIMP_ALTERNATIVE errno_t __cdecl strncpy_s(_Out_writes_z_(_SizeInBytes) char * _Dst, _In_ rsize_t _SizeInBytes, _In_reads_or_z_(_MaxCount) const char * _Src, _In_ rsize_t _MaxCount);
#endif  /* __STDC_WANT_SECURE_LIB__ */
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(errno_t, strncpy_s, char, _Dest, _In_reads_or_z_(_Count) const char *, _Source, _In_ size_t, _Count)
__DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_EX(char *, __RETURN_POLICY_DST, _CRTIMP, strncpy, strncpy_s, _Out_writes_z_(_Size) char, _Out_writes_(_Count) _Post_maybez_, char, _Dest, _In_reads_or_z_(_Count) const char *, _Source, _In_ size_t, _Count)
_Check_return_wat_ _CRTIMP_ALTERNATIVE errno_t __cdecl _strnset_s(_Inout_updates_z_(_SizeInBytes) char * _Str, _In_ size_t _SizeInBytes, _In_ int _Val, _In_ size_t _MaxCount);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(errno_t, _strnset_s, _Prepost_z_ char, _Dest, _In_ int, _Val, _In_ size_t, _Count)
__DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_EX(char *, __RETURN_POLICY_DST, _CRTIMP, _strnset, _strnset_s, _Inout_updates_z_(_Size) char, _Inout_updates_z_(_Count), char, _Dest, _In_ int, _Val, _In_ size_t, _Count)
_Check_return_ _CRTIMP _CONST_RETURN char *  __cdecl strpbrk(_In_z_ const char * _Str, _In_z_ const char * _Control);
_Check_return_ _CRTIMP _CONST_RETURN char *  __cdecl strrchr(_In_z_ const char * _Str, _In_ int _Ch);
_CRTIMP char *  __cdecl _strrev(_Inout_z_ char * _Str);
_Check_return_ _CRTIMP size_t  __cdecl strspn(_In_z_ const char * _Str, _In_z_ const char * _Control);
_Check_return_ _Ret_maybenull_ _CRTIMP _CONST_RETURN char *  __cdecl strstr(_In_z_ const char * _Str, _In_z_ const char * _SubStr);
_Check_return_ _CRT_INSECURE_DEPRECATE(strtok_s) _CRTIMP char *  __cdecl strtok(_Inout_opt_z_ char * _Str, _In_z_ const char * _Delim);
#if __STDC_WANT_SECURE_LIB__
_Check_return_ _CRTIMP_ALTERNATIVE char *  __cdecl strtok_s(_Inout_opt_z_ char * _Str, _In_z_ const char * _Delim, _Inout_ _Deref_prepost_opt_z_ char ** _Context);
#endif  /* __STDC_WANT_SECURE_LIB__ */
_Check_return_wat_ _CRTIMP errno_t __cdecl _strupr_s(_Inout_updates_z_(_Size) char * _Str, _In_ size_t _Size);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_0(errno_t, _strupr_s, _Prepost_z_ char, _String)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0(char *, __RETURN_POLICY_DST, _CRTIMP, _strupr, _Inout_z_, char, _String)
_Check_return_wat_ _CRTIMP errno_t __cdecl _strupr_s_l(_Inout_updates_z_(_Size) char * _Str, _In_ size_t _Size, _locale_t _Locale);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(errno_t, _strupr_s_l, _Prepost_z_ char, _String, _locale_t, _Locale)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_EX(char *, __RETURN_POLICY_DST, _CRTIMP, _strupr_l, _strupr_s_l, _Inout_updates_z_(_Size) char, _Inout_z_, char, _String, _In_opt_ _locale_t, _Locale)
_Check_return_opt_ _CRTIMP size_t  __cdecl strxfrm (_Out_writes_opt_(_MaxCount) _Post_maybez_ char * _Dst, _In_z_ const char * _Src, _In_ size_t _MaxCount);
_Check_return_opt_ _CRTIMP size_t  __cdecl _strxfrm_l(_Out_writes_opt_(_MaxCount) _Post_maybez_ char * _Dst, _In_z_ const char * _Src, _In_ size_t _MaxCount, _In_opt_ _locale_t _Locale);

#ifdef __cplusplus
extern "C++" {
#ifndef _CPP_NARROW_INLINES_DEFINED
#define _CPP_NARROW_INLINES_DEFINED
_Check_return_ inline char * __CRTDECL strchr(_In_z_ char * _Str, _In_ int _Ch)
        { return (char*)strchr((const char*)_Str, _Ch); }
_Check_return_ inline char * __CRTDECL strpbrk(_In_z_ char * _Str, _In_z_ const char * _Control)
        { return (char*)strpbrk((const char*)_Str, _Control); }
_Check_return_ inline char * __CRTDECL strrchr(_In_z_ char * _Str, _In_ int _Ch)
        { return (char*)strrchr((const char*)_Str, _Ch); }
_Check_return_ _Ret_maybenull_ inline char * __CRTDECL strstr(_In_z_ char * _Str, _In_z_ const char * _SubStr)
        { return (char*)strstr((const char*)_Str, _SubStr); }
#endif  /* _CPP_NARROW_INLINES_DEFINED */
#ifndef _CPP_MEMCHR_DEFINED
#define _CPP_MEMCHR_DEFINED
_Check_return_ inline void * __CRTDECL memchr(_In_reads_bytes_opt_(_N) void * _Pv, _In_ int _C, _In_ size_t _N)
        { return (void*)memchr((const void*)_Pv, _C, _N); }
#endif  /* _CPP_MEMCHR_DEFINED */
}
#endif  /* __cplusplus */

#if !__STDC__

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma push_macro("strdup")
#undef strdup
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

_Check_return_ _CRT_NONSTDC_DEPRECATE(_strdup) _CRTIMP char * __cdecl strdup(_In_opt_z_ const char * _Src);

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma pop_macro("strdup")
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

/* prototypes for oldnames.lib functions */
_Check_return_ _CRT_NONSTDC_DEPRECATE(_strcmpi) _CRTIMP int __cdecl strcmpi(_In_z_ const char * _Str1, _In_z_ const char * _Str2);
_Check_return_ _CRT_NONSTDC_DEPRECATE(_stricmp) _CRTIMP int __cdecl stricmp(_In_z_ const char * _Str1, _In_z_ const char * _Str2);
_CRT_NONSTDC_DEPRECATE(_strlwr) _CRTIMP char * __cdecl strlwr(_Inout_z_ char * _Str);
_Check_return_ _CRT_NONSTDC_DEPRECATE(_strnicmp) _CRTIMP int __cdecl strnicmp(_In_reads_or_z_(_MaxCount) const char * _Str1, _In_reads_or_z_(_MaxCount) const char * _Str, _In_ size_t _MaxCount);
_CRT_NONSTDC_DEPRECATE(_strnset) _CRTIMP char * __cdecl strnset(_Inout_updates_z_(_MaxCount) char * _Str, _In_ int _Val, _In_ size_t _MaxCount);
_CRT_NONSTDC_DEPRECATE(_strrev) _CRTIMP char * __cdecl strrev(_Inout_z_ char * _Str);
_CRT_NONSTDC_DEPRECATE(_strset)         char * __cdecl strset(_Inout_z_ char * _Str, _In_ int _Val);
_CRT_NONSTDC_DEPRECATE(_strupr) _CRTIMP char * __cdecl strupr(_Inout_z_ char * _Str);

#endif  /* !__STDC__ */


#ifndef _WSTRING_DEFINED

/* wide function prototypes, also declared in wchar.h  */

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma push_macro("_wcsdup")
#undef _wcsdup
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

_Check_return_ _CRTIMP wchar_t * __cdecl _wcsdup(_In_z_ const wchar_t * _Str);

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma pop_macro("_wcsdup")
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

#if __STDC_WANT_SECURE_LIB__
_Check_return_wat_ _CRTIMP_ALTERNATIVE errno_t __cdecl wcscat_s(_Inout_updates_z_(_SizeInWords) wchar_t * _Dst, _In_ rsize_t _SizeInWords, _In_z_ const wchar_t * _Src);
#endif  /* __STDC_WANT_SECURE_LIB__ */
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(errno_t, wcscat_s, wchar_t, _Dest, _In_z_ const wchar_t *, _Source)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1(wchar_t *, __RETURN_POLICY_DST, _CRTIMP, wcscat, _Inout_updates_z_(_String_length_(_Dest) + _String_length_(_Source) + 1), wchar_t, _Dest, _In_z_ const wchar_t *, _Source)
_Check_return_
_When_(return != NULL, _Ret_range_(_Str, _Str+_String_length_(_Str)-1))
_CRTIMP _CONST_RETURN wchar_t * __cdecl wcschr(_In_z_ const wchar_t * _Str, wchar_t _Ch);
_Check_return_ _CRTIMP int __cdecl wcscmp(_In_z_ const wchar_t * _Str1, _In_z_ const wchar_t * _Str2);
#if __STDC_WANT_SECURE_LIB__
_Check_return_wat_ _CRTIMP_ALTERNATIVE errno_t __cdecl wcscpy_s(_Out_writes_z_(_SizeInWords) wchar_t * _Dst, _In_ rsize_t _SizeInWords, _In_z_ const wchar_t * _Src);
#endif  /* __STDC_WANT_SECURE_LIB__ */
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(errno_t, wcscpy_s, wchar_t, _Dest, _In_z_ const wchar_t *, _Source)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1(wchar_t *, __RETURN_POLICY_DST, _CRTIMP, wcscpy, _Out_writes_z_(_String_length_(_Source) + 1), wchar_t, _Dest, _In_z_ const wchar_t *, _Source)
_Check_return_ _CRTIMP size_t __cdecl wcscspn(_In_z_ const wchar_t * _Str, _In_z_ const wchar_t * _Control);
_Check_return_ _CRTIMP size_t __cdecl wcslen(_In_z_ const wchar_t * _Str);
_Check_return_ _CRTIMP
_When_(_MaxCount > _String_length_(_Src), _Post_satisfies_(return == _String_length_(_Src)))
_When_(_MaxCount <= _String_length_(_Src), _Post_satisfies_(return == _MaxCount))
size_t __cdecl wcsnlen(_In_reads_or_z_(_MaxCount) const wchar_t * _Src, _In_ size_t _MaxCount);
#if __STDC_WANT_SECURE_LIB__ && !defined (__midl)
_Check_return_ static __inline
_When_(_MaxCount > _String_length_(_Src), _Post_satisfies_(return == _String_length_(_Src)))
_When_(_MaxCount <= _String_length_(_Src), _Post_satisfies_(return == _MaxCount))
size_t __CRTDECL wcsnlen_s(_In_reads_or_z_(_MaxCount) const wchar_t * _Src, _In_ size_t _MaxCount)
{
    return (_Src == NULL) ? 0 : wcsnlen(_Src, _MaxCount);
}
#endif  /* __STDC_WANT_SECURE_LIB__ && !defined (__midl) */
#if __STDC_WANT_SECURE_LIB__
_Check_return_wat_ _CRTIMP_ALTERNATIVE errno_t __cdecl wcsncat_s(_Inout_updates_z_(_SizeInWords) wchar_t * _Dst, _In_ rsize_t _SizeInWords, _In_reads_or_z_(_MaxCount) const wchar_t * _Src, _In_ rsize_t _MaxCount);
#endif  /* __STDC_WANT_SECURE_LIB__ */
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(errno_t, wcsncat_s, _Prepost_z_ wchar_t, _Dest, _In_reads_or_z_(_Count) const wchar_t *, _Source, _In_ size_t, _Count)
#pragma warning(push)
#pragma warning(disable:6059)
__DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_EX(wchar_t *, __RETURN_POLICY_DST, _CRTIMP, wcsncat, wcsncat_s, _Inout_updates_z_(_Size) wchar_t, _Inout_updates_z_(_Count), wchar_t, _Dest, _In_reads_or_z_(_Count) const wchar_t *, _Source, _In_ size_t, _Count)
#pragma warning(pop)
_Check_return_ _CRTIMP int __cdecl wcsncmp(_In_reads_or_z_(_MaxCount) const wchar_t * _Str1, _In_reads_or_z_(_MaxCount) const wchar_t * _Str2, _In_ size_t _MaxCount);
#if __STDC_WANT_SECURE_LIB__
_Check_return_wat_ _CRTIMP_ALTERNATIVE errno_t __cdecl wcsncpy_s(_Out_writes_z_(_SizeInWords) wchar_t * _Dst, _In_ rsize_t _SizeInWords, _In_reads_or_z_(_MaxCount) const wchar_t * _Src, _In_ rsize_t _MaxCount);
#endif  /* __STDC_WANT_SECURE_LIB__ */
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(errno_t, wcsncpy_s, wchar_t, _Dest, _In_reads_or_z_(_Count) const wchar_t *, _Source, _In_ size_t, _Count)
__DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_EX(wchar_t *, __RETURN_POLICY_DST, _CRTIMP, wcsncpy, wcsncpy_s, _Pre_notnull_ _Post_maybez_ wchar_t, _Out_writes_(_Count) _Post_maybez_, wchar_t, _Dest, _In_reads_or_z_(_Count) const wchar_t *, _Source, _In_ size_t, _Count)
_Check_return_ _CRTIMP _CONST_RETURN wchar_t * __cdecl wcspbrk(_In_z_ const wchar_t * _Str, _In_z_ const wchar_t * _Control);
_Check_return_ _CRTIMP _CONST_RETURN wchar_t * __cdecl wcsrchr(_In_z_ const wchar_t * _Str, _In_ wchar_t _Ch);
_Check_return_ _CRTIMP size_t __cdecl wcsspn(_In_z_ const wchar_t * _Str, _In_z_ const wchar_t * _Control);
_Check_return_ _Ret_maybenull_
_When_(return != NULL, _Ret_range_(_Str, _Str+_String_length_(_Str)-1))
_CRTIMP _CONST_RETURN wchar_t * __cdecl wcsstr(_In_z_ const wchar_t * _Str, _In_z_ const wchar_t * _SubStr);
_Check_return_ _CRT_INSECURE_DEPRECATE(wcstok_s) _CRTIMP wchar_t * __cdecl wcstok(_Inout_opt_z_ wchar_t * _Str, _In_z_ const wchar_t * _Delim);
#if __STDC_WANT_SECURE_LIB__
_Check_return_ _CRTIMP_ALTERNATIVE wchar_t * __cdecl wcstok_s(_Inout_opt_z_ wchar_t * _Str, _In_z_ const wchar_t * _Delim, _Inout_ _Deref_prepost_opt_z_ wchar_t ** _Context);
#endif  /* __STDC_WANT_SECURE_LIB__ */
_Check_return_ _CRT_INSECURE_DEPRECATE(_wcserror_s) _CRTIMP wchar_t * __cdecl _wcserror(_In_ int _ErrNum);
_Check_return_wat_ _CRTIMP errno_t __cdecl _wcserror_s(_Out_writes_opt_z_(_SizeInWords) wchar_t * _Buf, _In_ size_t _SizeInWords, _In_ int _ErrNum);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(errno_t, _wcserror_s, wchar_t, _Buffer, _In_ int, _Error)
_Check_return_ _CRT_INSECURE_DEPRECATE(__wcserror_s) _CRTIMP wchar_t * __cdecl __wcserror(_In_opt_z_ const wchar_t * _Str);
_Check_return_wat_ _CRTIMP errno_t __cdecl __wcserror_s(_Out_writes_opt_z_(_SizeInWords) wchar_t * _Buffer, _In_ size_t _SizeInWords, _In_z_ const wchar_t * _ErrMsg);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(errno_t, __wcserror_s, wchar_t, _Buffer, _In_z_ const wchar_t *, _ErrorMessage)

_Check_return_ _CRTIMP int __cdecl _wcsicmp(_In_z_ const wchar_t * _Str1, _In_z_ const wchar_t * _Str2);
_Check_return_ _CRTIMP int __cdecl _wcsicmp_l(_In_z_ const wchar_t * _Str1, _In_z_ const wchar_t * _Str2, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl _wcsnicmp(_In_reads_or_z_(_MaxCount) const wchar_t * _Str1, _In_reads_or_z_(_MaxCount) const wchar_t * _Str2, _In_ size_t _MaxCount);
_Check_return_ _CRTIMP int __cdecl _wcsnicmp_l(_In_reads_or_z_(_MaxCount) const wchar_t * _Str1, _In_reads_or_z_(_MaxCount) const wchar_t * _Str2, _In_ size_t _MaxCount, _In_opt_ _locale_t _Locale);
_Check_return_wat_ _CRTIMP_ALTERNATIVE errno_t __cdecl _wcsnset_s(_Inout_updates_z_(_SizeInWords) wchar_t * _Dst, _In_ size_t _SizeInWords, _In_ wchar_t _Val, _In_ size_t _MaxCount);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(errno_t, _wcsnset_s, _Prepost_z_ wchar_t, _Dst, wchar_t, _Val, _In_ size_t, _MaxCount)
__DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_EX(wchar_t *, __RETURN_POLICY_DST, _CRTIMP, _wcsnset, _wcsnset_s, _Inout_updates_z_(_Size) wchar_t, _Inout_updates_z_(_MaxCount), wchar_t, _Str, wchar_t, _Val, _In_ size_t, _MaxCount)
_CRTIMP wchar_t * __cdecl _wcsrev(_Inout_z_ wchar_t * _Str);
_Check_return_wat_ _CRTIMP_ALTERNATIVE errno_t __cdecl _wcsset_s(_Inout_updates_z_(_SizeInWords) wchar_t * _Dst, _In_ size_t _SizeInWords, _In_ wchar_t _Value);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(errno_t, _wcsset_s, _Prepost_z_ wchar_t, _Str, wchar_t, _Val)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_EX(wchar_t *, __RETURN_POLICY_DST, _CRTIMP, _wcsset, _wcsset_s, _Inout_updates_z_(_Size) wchar_t, _Inout_z_, wchar_t, _Str, wchar_t, _Val)

_Check_return_wat_ _CRTIMP errno_t __cdecl _wcslwr_s(_Inout_updates_z_(_SizeInWords) wchar_t * _Str, _In_ size_t _SizeInWords);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_0(errno_t, _wcslwr_s, _Prepost_z_ wchar_t, _String)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0(wchar_t *, __RETURN_POLICY_DST, _CRTIMP, _wcslwr, _Inout_z_, wchar_t, _String)
_Check_return_wat_ _CRTIMP errno_t __cdecl _wcslwr_s_l(_Inout_updates_z_(_SizeInWords) wchar_t * _Str, _In_ size_t _SizeInWords, _In_opt_ _locale_t _Locale);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(errno_t, _wcslwr_s_l, _Prepost_z_ wchar_t, _String, _In_opt_ _locale_t, _Locale)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_EX(wchar_t *, __RETURN_POLICY_DST, _CRTIMP, _wcslwr_l, _wcslwr_s_l, _Inout_updates_z_(_Size) wchar_t, _Inout_z_, wchar_t, _String, _In_opt_ _locale_t, _Locale)
_Check_return_wat_ _CRTIMP errno_t __cdecl _wcsupr_s(_Inout_updates_z_(_Size) wchar_t * _Str, _In_ size_t _Size);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_0(errno_t, _wcsupr_s, _Prepost_z_ wchar_t, _String)
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
_Check_return_ _CRTIMP int __cdecl _wcsncoll(_In_reads_or_z_(_MaxCount) const wchar_t * _Str1, _In_reads_or_z_(_MaxCount) const wchar_t * _Str2, _In_ size_t _MaxCount);
_Check_return_ _CRTIMP int __cdecl _wcsncoll_l(_In_reads_or_z_(_MaxCount) const wchar_t * _Str1, _In_reads_or_z_(_MaxCount) const wchar_t * _Str2, _In_ size_t _MaxCount, _In_opt_ _locale_t _Locale);
_Check_return_ _CRTIMP int __cdecl _wcsnicoll(_In_reads_or_z_(_MaxCount) const wchar_t * _Str1, _In_reads_or_z_(_MaxCount) const wchar_t * _Str2, _In_ size_t _MaxCount);
_Check_return_ _CRTIMP int __cdecl _wcsnicoll_l(_In_reads_or_z_(_MaxCount) const wchar_t * _Str1, _In_reads_or_z_(_MaxCount) const wchar_t * _Str2, _In_ size_t _MaxCount, _In_opt_ _locale_t _Locale);

#ifdef __cplusplus
#ifndef _CPP_WIDE_INLINES_DEFINED
#define _CPP_WIDE_INLINES_DEFINED
extern "C++" {
_Check_return_
_When_(return != NULL, _Ret_range_(_Str, _Str+_String_length_(_Str)-1))
        inline wchar_t * __CRTDECL wcschr(_In_z_ wchar_t *_Str, wchar_t _Ch)
        {return ((wchar_t *)wcschr((const wchar_t *)_Str, _Ch)); }
_Check_return_ inline wchar_t * __CRTDECL wcspbrk(_In_z_ wchar_t *_Str, _In_z_ const wchar_t *_Control)
        {return ((wchar_t *)wcspbrk((const wchar_t *)_Str, _Control)); }
_Check_return_ inline wchar_t * __CRTDECL wcsrchr(_In_z_ wchar_t *_Str, _In_ wchar_t _Ch)
        {return ((wchar_t *)wcsrchr((const wchar_t *)_Str, _Ch)); }
_Check_return_ _Ret_maybenull_
_When_(return != NULL, _Ret_range_(_Str, _Str+_String_length_(_Str)-1))
        inline wchar_t * __CRTDECL wcsstr(_In_z_ wchar_t *_Str, _In_z_ const wchar_t *_SubStr)
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
_Check_return_ _CRT_NONSTDC_DEPRECATE(_wcsnicmp) _CRTIMP int __cdecl wcsnicmp(_In_reads_or_z_(_MaxCount) const wchar_t * _Str1, _In_reads_or_z_(_MaxCount) const wchar_t * _Str2, _In_ size_t _MaxCount);
_CRT_NONSTDC_DEPRECATE(_wcsnset) _CRTIMP wchar_t * __cdecl wcsnset(_Inout_updates_z_(_MaxCount) wchar_t * _Str, _In_ wchar_t _Val, _In_ size_t _MaxCount);
_CRT_NONSTDC_DEPRECATE(_wcsrev) _CRTIMP wchar_t * __cdecl wcsrev(_Inout_z_ wchar_t * _Str);
_CRT_NONSTDC_DEPRECATE(_wcsset) _CRTIMP wchar_t * __cdecl wcsset(_Inout_z_ wchar_t * _Str, wchar_t _Val);
_CRT_NONSTDC_DEPRECATE(_wcslwr) _CRTIMP wchar_t * __cdecl wcslwr(_Inout_z_ wchar_t * _Str);
_CRT_NONSTDC_DEPRECATE(_wcsupr) _CRTIMP wchar_t * __cdecl wcsupr(_Inout_z_ wchar_t * _Str);
_Check_return_ _CRT_NONSTDC_DEPRECATE(_wcsicoll) _CRTIMP int __cdecl wcsicoll(_In_z_ const wchar_t * _Str1, _In_z_ const wchar_t * _Str2);

#endif  /* !__STDC__ */

#define _WSTRING_DEFINED
#endif  /* _WSTRING_DEFINED */

#ifdef _CRTBLD
_Check_return_ int __cdecl __ascii_memicmp(_In_reads_bytes_opt_(_Size) const void * _Buf1, _In_reads_bytes_opt_(_Size) const void * _Buf2, _In_ size_t _Size);
_Check_return_ int __cdecl __ascii_stricmp(_In_z_ const char * _Str1, _In_z_ const char * _Str2);
_Check_return_ int __cdecl __ascii_strnicmp(_In_reads_or_z_(_MaxCount) const char * _Str1, _In_reads_or_z_(_MaxCount) const char * _Str2, _In_ size_t _MaxCount);
#endif  /* _CRTBLD */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _INC_STRING */
