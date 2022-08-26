/***
*stdio.h - definitions/declarations for standard I/O routines
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file defines the structures, values, macros, and functions
*       used by the level 2 I/O ("standard I/O") routines.
*       [ANSI/System V]
*
*       [Public]
*
****/

#pragma once

#ifndef _INC_STDIO
#define _INC_STDIO

#include <crtdefs.h>

/*
 * Currently, all MS C compilers for Win32 platforms default to 8 byte
 * alignment.
 */
#pragma pack(push,_CRT_PACKING)

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */


/* Buffered I/O macros */

#define BUFSIZ  512

#ifdef _CRTBLD
/*
 * Real default size for stdio buffers
 */
#define _INTERNAL_BUFSIZ    4096
#define _SMALL_BUFSIZ       512
#endif  /* _CRTBLD */

/*
 * Default number of supported streams. _NFILE is confusing and obsolete, but
 * supported anyway for backwards compatibility.
 */
#define _NFILE      _NSTREAM_

#define _NSTREAM_   512

/*
 * Number of entries in _iob[] (declared below). Note that _NSTREAM_ must be
 * greater than or equal to _IOB_ENTRIES.
 */
#define _IOB_ENTRIES 20

#define EOF     (-1)


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


/* Directory where temporary files may be created. */

#define _P_tmpdir   "\\"
#define _wP_tmpdir  L"\\"

/* L_tmpnam = length of string _P_tmpdir
 *            + 1 if _P_tmpdir does not end in "/" or "\", else 0
 *            + 12 (for the filename string)
 *            + 1 (for the null terminator)
 * L_tmpnam_s = length of string _P_tmpdir
 *            + 1 if _P_tmpdir does not end in "/" or "\", else 0
 *            + 16 (for the filename string)
 *            + 1 (for the null terminator)
 */
#define L_tmpnam   (sizeof(_P_tmpdir) + 12)
#if __STDC_WANT_SECURE_LIB__
#define L_tmpnam_s (sizeof(_P_tmpdir) + 16)
#endif  /* __STDC_WANT_SECURE_LIB__ */



/* Seek method constants */

#define SEEK_CUR    1
#define SEEK_END    2
#define SEEK_SET    0


#define FILENAME_MAX    260
#define FOPEN_MAX       20
#define _SYS_OPEN       20
#define TMP_MAX         32767  /* SHRT_MAX */
#if __STDC_WANT_SECURE_LIB__
#define TMP_MAX_S       _TMP_MAX_S
#define _TMP_MAX_S      2147483647 /* INT_MAX */
#endif  /* __STDC_WANT_SECURE_LIB__ */

/* Define NULL pointer value */
#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else  /* __cplusplus */
#define NULL    ((void *)0)
#endif  /* __cplusplus */
#endif  /* NULL */

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


/* Define file position type */

#ifndef _FPOS_T_DEFINED
typedef __int64 fpos_t;
#define _FPOS_T_DEFINED
#endif  /* _FPOS_T_DEFINED */

#ifndef _STDSTREAM_DEFINED
#define stdin  (&__iob_func()[0])
#define stdout (&__iob_func()[1])
#define stderr (&__iob_func()[2])
#define _STDSTREAM_DEFINED
#endif  /* _STDSTREAM_DEFINED */

#define _IOREAD         0x0001
#define _IOWRT          0x0002

#define _IOFBF          0x0000
#define _IOLBF          0x0040
#define _IONBF          0x0004

#define _IOMYBUF        0x0008
#define _IOEOF          0x0010
#define _IOERR          0x0020
#define _IOSTRG         0x0040
#define _IORW           0x0080

/* constants used by _set_output_format */
#define _TWO_DIGIT_EXPONENT 0x1

/* Function prototypes */

#ifndef _STDIO_DEFINED

_Check_return_ _CRTIMP int __cdecl _filbuf(_Inout_ FILE * _File );
_Check_return_opt_ _CRTIMP int __cdecl _flsbuf(_In_ int _Ch, _Inout_ FILE * _File);

_Check_return_ _CRTIMP FILE * __cdecl _fsopen(_In_z_ const char * _Filename, _In_z_ const char * _Mode, _In_ int _ShFlag);

_CRTIMP void __cdecl clearerr(_Inout_ FILE * _File);
#if __STDC_WANT_SECURE_LIB__
_Check_return_wat_ _CRTIMP errno_t __cdecl clearerr_s(_Inout_ FILE * _File );
#endif  /* __STDC_WANT_SECURE_LIB__ */
_Check_return_opt_ _CRTIMP int __cdecl fclose(_Inout_ FILE * _File);
_Check_return_opt_ _CRTIMP int __cdecl _fcloseall(void);

_Check_return_ _CRTIMP FILE * __cdecl _fdopen(_In_ int _FileHandle, _In_z_ const char * _Mode);

_Check_return_ _CRTIMP int __cdecl feof(_In_ FILE * _File);
_Check_return_ _CRTIMP int __cdecl ferror(_In_ FILE * _File);
_Check_return_opt_ _CRTIMP int __cdecl fflush(_Inout_opt_ FILE * _File);
_Check_return_opt_ _CRTIMP int __cdecl fgetc(_Inout_ FILE * _File);
_Check_return_opt_ _CRTIMP int __cdecl _fgetchar(void);
_Check_return_opt_ _CRTIMP int __cdecl fgetpos(_Inout_ FILE * _File , _Out_ fpos_t * _Pos);
_Check_return_opt_ _CRTIMP char * __cdecl fgets(_Out_writes_z_(_MaxCount) char * _Buf, _In_ int _MaxCount, _Inout_ FILE * _File);

_Check_return_ _CRTIMP int __cdecl _fileno(_In_ FILE * _File);

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma push_macro("_tempnam")
#undef _tempnam
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

_Check_return_ _CRTIMP char * __cdecl _tempnam(_In_opt_z_ const char * _DirName, _In_opt_z_ const char * _FilePrefix);

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma pop_macro("_tempnam")
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

_Check_return_opt_ _CRTIMP int __cdecl _flushall(void);
_Check_return_ _CRT_INSECURE_DEPRECATE(fopen_s) _CRTIMP FILE * __cdecl fopen(_In_z_ const char * _Filename, _In_z_ const char * _Mode);
#if __STDC_WANT_SECURE_LIB__
_Check_return_wat_ _CRTIMP errno_t __cdecl fopen_s(_Outptr_result_maybenull_ FILE ** _File, _In_z_ const char * _Filename, _In_z_ const char * _Mode);
#endif  /* __STDC_WANT_SECURE_LIB__ */
_Check_return_opt_ _CRTIMP int __cdecl fprintf(_Inout_ FILE * _File, _In_z_ _Printf_format_string_ const char * _Format, ...);
#if __STDC_WANT_SECURE_LIB__
_Check_return_opt_ _CRTIMP int __cdecl fprintf_s(_Inout_ FILE * _File, _In_z_ _Printf_format_string_ const char * _Format, ...);
#endif  /* __STDC_WANT_SECURE_LIB__ */
_Check_return_opt_ _CRTIMP int __cdecl fputc(_In_ int _Ch, _Inout_ FILE * _File);
_Check_return_opt_ _CRTIMP int __cdecl _fputchar(_In_ int _Ch);
_Check_return_opt_ _CRTIMP int __cdecl fputs(_In_z_ const char * _Str, _Inout_ FILE * _File);
_Check_return_opt_ _CRTIMP size_t __cdecl fread(_Out_writes_bytes_(_ElementSize*_Count) void * _DstBuf, _In_ size_t _ElementSize, _In_ size_t _Count, _Inout_ FILE * _File);
#if __STDC_WANT_SECURE_LIB__
_Check_return_opt_ _CRTIMP size_t __cdecl fread_s(_Out_writes_bytes_(_ElementSize*_Count) void * _DstBuf, _In_ size_t _DstSize, _In_ size_t _ElementSize, _In_ size_t _Count, _Inout_ FILE * _File);
#endif  /* __STDC_WANT_SECURE_LIB__ */
_Check_return_ _CRT_INSECURE_DEPRECATE(freopen_s) _CRTIMP FILE * __cdecl freopen(_In_z_ const char * _Filename, _In_z_ const char * _Mode, _Inout_ FILE * _File);
#if __STDC_WANT_SECURE_LIB__
_Check_return_wat_ _CRTIMP errno_t __cdecl freopen_s(_Outptr_result_maybenull_ FILE ** _File, _In_z_ const char * _Filename, _In_z_ const char * _Mode, _Inout_ FILE * _OldFile);
#endif  /* __STDC_WANT_SECURE_LIB__ */
_Check_return_ _CRT_INSECURE_DEPRECATE(fscanf_s) _CRTIMP int __cdecl fscanf(_Inout_ FILE * _File, _In_z_ _Scanf_format_string_ const char * _Format, ...);
_Check_return_opt_ _CRT_INSECURE_DEPRECATE(_fscanf_s_l) _CRTIMP int __cdecl _fscanf_l(_Inout_ FILE * _File, _In_z_ _Scanf_format_string_params_(0) const char * _Format, _In_opt_ _locale_t _Locale, ...);
#pragma warning(push)
#pragma warning(disable:6530)
#if __STDC_WANT_SECURE_LIB__
_Check_return_opt_ _CRTIMP int __cdecl fscanf_s(_Inout_ FILE * _File, _In_z_ _Scanf_s_format_string_ const char * _Format, ...);
#endif  /* __STDC_WANT_SECURE_LIB__ */
_Check_return_opt_ _CRTIMP int __cdecl _fscanf_s_l(_Inout_ FILE * _File, _In_z_ _Scanf_s_format_string_params_(0) const char * _Format, _In_opt_ _locale_t _Locale, ...);
#pragma warning(pop)
_Check_return_opt_ _CRTIMP int __cdecl fsetpos(_Inout_ FILE * _File, _In_ const fpos_t * _Pos);
_Check_return_opt_ _CRTIMP int __cdecl fseek(_Inout_ FILE * _File, _In_ long _Offset, _In_ int _Origin);
_Check_return_ _CRTIMP long __cdecl ftell(_Inout_ FILE * _File);

_Check_return_opt_ _CRTIMP int __cdecl _fseeki64(_Inout_ FILE * _File, _In_ __int64 _Offset, _In_ int _Origin);
_Check_return_ _CRTIMP __int64 __cdecl _ftelli64(_Inout_ FILE * _File);

_Check_return_opt_ _CRTIMP size_t __cdecl fwrite(_In_reads_bytes_(_Size*_Count) const void * _Str, _In_ size_t _Size, _In_ size_t _Count, _Inout_ FILE * _File);
_Check_return_ _CRTIMP int __cdecl getc(_Inout_ FILE * _File);
_Check_return_ _CRTIMP int __cdecl getchar(void);
_Check_return_ _CRTIMP int __cdecl _getmaxstdio(void);
#if __STDC_WANT_SECURE_LIB__
_CRTIMP char * __cdecl gets_s(_Out_writes_z_(_Size) char * _Buf, _In_ rsize_t _Size);
#endif  /* __STDC_WANT_SECURE_LIB__ */
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_0(char *, gets_s, char, _Buffer)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0(char *, __RETURN_POLICY_SAME, _CRTIMP, gets, _Pre_notnull_ _Post_z_ _Out_writes_z_(((size_t)-1)), char, _Buffer)
_Check_return_ int __cdecl _getw(_Inout_ FILE * _File);
#ifndef _CRT_PERROR_DEFINED
#define _CRT_PERROR_DEFINED
_CRTIMP void __cdecl perror(_In_opt_z_ const char * _ErrMsg);
#endif  /* _CRT_PERROR_DEFINED */
#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP
_Check_return_opt_ _CRTIMP int __cdecl _pclose(_Inout_ FILE * _File);
_Check_return_ _CRTIMP FILE * __cdecl _popen(_In_z_ const char * _Command, _In_z_ const char * _Mode);
#endif  /* _CRT_USE_WINAPI_FAMILY_DESKTOP_APP */
_Check_return_opt_ _CRTIMP int __cdecl printf(_In_z_ _Printf_format_string_ const char * _Format, ...);
#if __STDC_WANT_SECURE_LIB__
_Check_return_opt_ _CRTIMP int __cdecl printf_s(_In_z_ _Printf_format_string_ const char * _Format, ...);
#endif  /* __STDC_WANT_SECURE_LIB__ */
_Check_return_opt_ _CRTIMP int __cdecl putc(_In_ int _Ch, _Inout_ FILE * _File);
_Check_return_opt_ _CRTIMP int __cdecl putchar(_In_ int _Ch);
_Check_return_opt_ _CRTIMP int __cdecl puts(_In_z_ const char * _Str);
_Check_return_opt_ _CRTIMP int __cdecl _putw(_In_ int _Word, _Inout_ FILE * _File);
#ifndef _CRT_DIRECTORY_DEFINED
#define _CRT_DIRECTORY_DEFINED
_CRTIMP int __cdecl remove(_In_z_ const char * _Filename);
_Check_return_ _CRTIMP int __cdecl rename(_In_z_ const char * _OldFilename, _In_z_ const char * _NewFilename);
_CRTIMP int __cdecl _unlink(_In_z_ const char * _Filename);
#if !__STDC__
_CRT_NONSTDC_DEPRECATE(_unlink) _CRTIMP int __cdecl unlink(_In_z_ const char * _Filename);
#endif  /* !__STDC__ */
#endif  /* _CRT_DIRECTORY_DEFINED */
_CRTIMP void __cdecl rewind(_Inout_ FILE * _File);
_Check_return_opt_ _CRTIMP int __cdecl _rmtmp(void);
_Check_return_ _CRT_INSECURE_DEPRECATE(scanf_s) _CRTIMP int __cdecl scanf(_In_z_ _Scanf_format_string_ const char * _Format, ...);
_Check_return_opt_ _CRT_INSECURE_DEPRECATE(_scanf_s_l) _CRTIMP int __cdecl _scanf_l(_In_z_ _Scanf_format_string_params_(0) const char * _Format, _In_opt_ _locale_t _Locale, ...);
#pragma warning(push)
#pragma warning(disable:6530)
#if __STDC_WANT_SECURE_LIB__
_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl scanf_s(_In_z_ _Scanf_s_format_string_ const char * _Format, ...);
#endif  /* __STDC_WANT_SECURE_LIB__ */
_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl _scanf_s_l(_In_z_ _Scanf_s_format_string_params_(0) const char * _Format, _In_opt_ _locale_t _Locale, ...);
#pragma warning(pop)
_CRT_INSECURE_DEPRECATE(setvbuf) _CRTIMP void __cdecl setbuf(_Inout_ FILE * _File, _Inout_updates_opt_(BUFSIZ) _Post_readable_size_(0) char * _Buffer);
_Check_return_opt_ _CRTIMP int __cdecl _setmaxstdio(_In_ int _Max);
_Check_return_opt_ _CRTIMP unsigned int __cdecl _set_output_format(_In_ unsigned int _Format);
_Check_return_opt_ _CRTIMP unsigned int __cdecl _get_output_format(void);
_Check_return_opt_ _CRTIMP int __cdecl setvbuf(_Inout_ FILE * _File, _Inout_updates_opt_z_(_Size) char * _Buf, _In_ int _Mode, _In_ size_t _Size);
_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl _snprintf_s(_Out_writes_z_(_SizeInBytes) char * _DstBuf, _In_ size_t _SizeInBytes, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_ const char * _Format, ...);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2_ARGLIST(int, _snprintf_s, _vsnprintf_s, _Post_z_ char, _Dest, _In_ size_t, _MaxCount, _In_z_ _Printf_format_string_ const char *,_Format)
#if __STDC_WANT_SECURE_LIB__
_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl sprintf_s(_Out_writes_z_(_SizeInBytes) char * _DstBuf, _In_ size_t _SizeInBytes, _In_z_ _Printf_format_string_ const char * _Format, ...);
#endif  /* __STDC_WANT_SECURE_LIB__ */
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1_ARGLIST(int, sprintf_s, vsprintf_s, _Post_z_ char, _Dest, _In_z_ _Printf_format_string_ const char *, _Format)
_Check_return_ _CRTIMP int __cdecl _scprintf(_In_z_ _Printf_format_string_ const char * _Format, ...);
_Check_return_ _CRT_INSECURE_DEPRECATE(sscanf_s) _CRTIMP int __cdecl sscanf(_In_z_ const char * _Src, _In_z_ _Scanf_format_string_ const char * _Format, ...);
_Check_return_opt_ _CRT_INSECURE_DEPRECATE(_sscanf_s_l) _CRTIMP int __cdecl _sscanf_l(_In_z_ const char * _Src, _In_z_ _Scanf_format_string_params_(0) const char * _Format, _In_opt_ _locale_t _Locale, ...);
#pragma warning(push)
#pragma warning(disable:6530)
#if __STDC_WANT_SECURE_LIB__
_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl sscanf_s(_In_z_ const char * _Src, _In_z_ _Scanf_s_format_string_ const char * _Format, ...);
#endif  /* __STDC_WANT_SECURE_LIB__ */
_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl _sscanf_s_l(_In_z_ const char * _Src, _In_z_ _Scanf_s_format_string_params_(0) const char * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRT_INSECURE_DEPRECATE(_snscanf_s) _CRTIMP int __cdecl _snscanf(_In_reads_bytes_(_MaxCount) _Pre_z_ const char * _Src, _In_ size_t _MaxCount, _In_z_ _Scanf_format_string_ const char * _Format, ...);
_Check_return_opt_ _CRT_INSECURE_DEPRECATE(_snscanf_s_l) _CRTIMP int __cdecl _snscanf_l(_In_reads_bytes_(_MaxCount) _Pre_z_ const char * _Src, _In_ size_t _MaxCount, _In_z_ _Scanf_format_string_params_(0) const char * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl _snscanf_s(_In_reads_bytes_(_MaxCount) _Pre_z_ const char * _Src, _In_ size_t _MaxCount, _In_z_ _Scanf_s_format_string_ const char * _Format, ...);
_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl _snscanf_s_l(_In_reads_bytes_(_MaxCount) _Pre_z_ const char * _Src, _In_ size_t _MaxCount, _In_z_ _Scanf_s_format_string_params_(0) const char * _Format, _In_opt_ _locale_t _Locale, ...);
#pragma warning(pop)
_Check_return_ _CRT_INSECURE_DEPRECATE(tmpfile_s) _CRTIMP FILE * __cdecl tmpfile(void);
#if __STDC_WANT_SECURE_LIB__
_Check_return_wat_ _CRTIMP errno_t __cdecl tmpfile_s(_Out_opt_ _Deref_post_valid_ FILE ** _File);
_Check_return_wat_ _CRTIMP errno_t __cdecl tmpnam_s(_Out_writes_z_(_Size) char * _Buf, _In_ rsize_t _Size);
#endif  /* __STDC_WANT_SECURE_LIB__ */
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_0(errno_t, tmpnam_s, _Post_z_ char, _Buf)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0(char *, __RETURN_POLICY_DST, _CRTIMP, tmpnam, _Pre_maybenull_ _Post_z_, char, _Buffer)
_Check_return_opt_ _CRTIMP int __cdecl ungetc(_In_ int _Ch, _Inout_ FILE * _File);
_Check_return_opt_ _CRTIMP int __cdecl vfprintf(_Inout_ FILE * _File, _In_z_ _Printf_format_string_ const char * _Format, va_list _ArgList);
_Check_return_opt_ _CRTIMP int __cdecl vfscanf(_Inout_ FILE * _File, _In_z_ _Printf_format_string_ const char * _Format, va_list _ArgList);
#if __STDC_WANT_SECURE_LIB__
_Check_return_opt_ _CRTIMP int __cdecl vfprintf_s(_Inout_ FILE * _File, _In_z_ _Printf_format_string_ const char * _Format, va_list _ArgList);
_Check_return_opt_ _CRTIMP int __cdecl vfscanf_s(_Inout_ FILE * _File, _In_z_ _Printf_format_string_ const char * _Format, va_list _ArgList);
#endif  /* __STDC_WANT_SECURE_LIB__ */
_Check_return_opt_ _CRTIMP int __cdecl vprintf(_In_z_ _Printf_format_string_ const char * _Format, va_list _ArgList);
_Check_return_opt_ _CRTIMP int __cdecl vscanf(_In_z_ _Printf_format_string_ const char * _Format, va_list _ArgList);
#if __STDC_WANT_SECURE_LIB__
_Check_return_opt_ _CRTIMP int __cdecl vprintf_s(_In_z_ _Printf_format_string_ const char * _Format, va_list _ArgList);
_Check_return_opt_ _CRTIMP int __cdecl vscanf_s(_In_z_ _Printf_format_string_ const char * _Format, va_list _ArgList);
#endif  /* __STDC_WANT_SECURE_LIB__ */
_Check_return_opt_ _CRT_INSECURE_DEPRECATE(vsnprintf_s) _CRTIMP int __cdecl vsnprintf(_Out_writes_(_MaxCount) char * _DstBuf, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_ const char * _Format, va_list _ArgList);
#if __STDC_WANT_SECURE_LIB__
_Check_return_opt_ _CRTIMP int __cdecl vsnprintf_s(_Out_writes_z_(_DstSize) char * _DstBuf, _In_ size_t _DstSize, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_ const char * _Format, va_list _ArgList);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_3(int, vsnprintf_s, _Post_z_ char, _Dest, _In_ size_t, _MaxCount, _In_z_ _Printf_format_string_ const char *, _Format, va_list, _Args)
#endif  /* __STDC_WANT_SECURE_LIB__ */
_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl _vsnprintf_s(_Out_writes_z_(_SizeInBytes) char * _DstBuf, _In_ size_t _SizeInBytes, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_ const char * _Format, va_list _ArgList);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_3(int, _vsnprintf_s, _Post_z_ char, _Dest, _In_ size_t, _MaxCount, _In_z_ _Printf_format_string_ const char *, _Format, va_list, _Args)
#pragma warning(push)
#pragma warning(disable:4793)
__DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_ARGLIST_EX(int, __RETURN_POLICY_SAME, _CRTIMP, _snprintf, _vsnprintf, _Pre_notnull_ _Post_maybez_ char, _Out_writes_(_Count) _Post_maybez_, char, _Dest, _In_ size_t, _Count, _In_z_ _Printf_format_string_ const char *, _Format)
#pragma warning(pop)
#if __STDC_WANT_SECURE_LIB__
_CRTIMP_ALTERNATIVE int __cdecl vsprintf_s(_Out_writes_z_(_SizeInBytes) char * _DstBuf, _In_ size_t _SizeInBytes, _In_z_ _Printf_format_string_ const char * _Format, va_list _ArgList);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(int, vsprintf_s, _Post_z_ char, _Dest, _In_z_ _Printf_format_string_ const char *, _Format, va_list, _Args)
_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl vsscanf_s(const char * _Src, _In_z_ _Printf_format_string_ const char * _Format, va_list _ArgList);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(int, vsscanf_s, _Post_z_ const char, _Src, _In_z_ _Printf_format_string_ const char *, _Format, va_list, _Args)
#endif  /* __STDC_WANT_SECURE_LIB__ */
#pragma warning(push)
#pragma warning(disable:4793)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_ARGLIST(int, __RETURN_POLICY_SAME, _CRTIMP, sprintf, vsprintf, _Pre_notnull_ _Post_z_, char, _Dest, _In_z_ _Printf_format_string_ const char *, _Format)
_Check_return_opt_ _CRTIMP int __cdecl vsscanf(const char * _srcBuf, _In_z_ _Printf_format_string_ const char * _Format, va_list _ArgList);
#pragma warning(pop)
_Check_return_ _CRTIMP int __cdecl _vscprintf(_In_z_ _Printf_format_string_ const char * _Format, va_list _ArgList);
_Check_return_opt_ _CRTIMP int __cdecl _snprintf_c(_Out_writes_(_MaxCount) char * _DstBuf, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_ const char * _Format, ...);
_Check_return_opt_ _CRTIMP int __cdecl _vsnprintf_c(_Out_writes_(_MaxCount) char *_DstBuf, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_ const char * _Format, va_list _ArgList);

_Check_return_opt_ _CRTIMP int __cdecl _fprintf_p(_Inout_ FILE * _File, _In_z_ _Printf_format_string_ const char * _Format, ...);
_Check_return_opt_ _CRTIMP int __cdecl _printf_p(_In_z_ _Printf_format_string_ const char * _Format, ...);
_Check_return_opt_ _CRTIMP int __cdecl _sprintf_p(_Out_writes_z_(_MaxCount) char * _Dst, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_ const char * _Format, ...);
_Check_return_opt_ _CRTIMP int __cdecl _vfprintf_p(_Inout_ FILE * _File, _In_z_ _Printf_format_string_ const char * _Format, va_list _ArgList);
_Check_return_opt_ _CRTIMP int __cdecl _vprintf_p(_In_z_ _Printf_format_string_ const char * _Format, va_list _ArgList);
_Check_return_opt_ _CRTIMP int __cdecl _vsprintf_p(_Out_writes_z_(_MaxCount) char * _Dst, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_ const char * _Format, va_list _ArgList);
_Check_return_ _CRTIMP int __cdecl _scprintf_p(_In_z_ _Printf_format_string_ const char * _Format, ...);
_Check_return_ _CRTIMP int __cdecl _vscprintf_p(_In_z_ _Printf_format_string_ const char * _Format, va_list _ArgList);
_CRTIMP int __cdecl _set_printf_count_output(_In_ int _Value);
_CRTIMP int __cdecl _get_printf_count_output(void);

_Check_return_opt_ _CRTIMP int __cdecl _printf_l(_In_z_ _Printf_format_string_params_(0) const char * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRTIMP int __cdecl _printf_p_l(_In_z_ _Printf_format_string_params_(0) const char * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRTIMP int __cdecl _printf_s_l(_In_z_ _Printf_format_string_params_(0) const char * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRTIMP int __cdecl _vprintf_l(_In_z_ _Printf_format_string_params_(2) const char * _Format, _In_opt_ _locale_t _Locale, va_list _ArgList);
_Check_return_opt_ _CRTIMP int __cdecl _vprintf_p_l(_In_z_ _Printf_format_string_params_(2) const char * _Format, _In_opt_ _locale_t _Locale, va_list _ArgList);
_Check_return_opt_ _CRTIMP int __cdecl _vprintf_s_l(_In_z_ _Printf_format_string_params_(2) const char * _Format, _In_opt_ _locale_t _Locale, va_list _ArgList);

_Check_return_opt_ _CRTIMP int __cdecl _fprintf_l(_Inout_ FILE * _File, _In_z_ _Printf_format_string_params_(0) const char * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRTIMP int __cdecl _fprintf_p_l(_Inout_ FILE * _File, _In_z_ _Printf_format_string_params_(0) const char * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRTIMP int __cdecl _fprintf_s_l(_Inout_ FILE * _File, _In_z_ _Printf_format_string_params_(0) const char * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRTIMP int __cdecl _vfprintf_l(_Inout_ FILE * _File, _In_z_ const char * _Format, _In_opt_ _locale_t _Locale, va_list _ArgList);
_Check_return_opt_ _CRTIMP int __cdecl _vfprintf_p_l(_Inout_ FILE * _File, _In_z_ const char * _Format, _In_opt_ _locale_t _Locale, va_list _ArgList);
_Check_return_opt_ _CRTIMP int __cdecl _vfprintf_s_l(_Inout_ FILE * _File, _In_z_ const char * _Format, _In_opt_ _locale_t _Locale, va_list _ArgList);

_Check_return_opt_ _CRT_INSECURE_DEPRECATE(_sprintf_s_l) _CRTIMP int __cdecl _sprintf_l(_Pre_notnull_ _Post_z_ char * _DstBuf, _In_z_ _Printf_format_string_params_(0) const char * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRTIMP int __cdecl _sprintf_p_l(_Out_writes_z_(_MaxCount) char * _DstBuf, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_params_(0) const char * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl _sprintf_s_l(_Out_writes_z_(_DstSize) char * _DstBuf, _In_ size_t _DstSize, _In_z_ _Printf_format_string_params_(0) const char * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRT_INSECURE_DEPRECATE(_vsprintf_s_l) _CRTIMP int __cdecl _vsprintf_l(_Pre_notnull_ _Post_z_ char * _DstBuf, _In_z_ const char * _Format, _In_opt_ _locale_t, va_list _ArgList);
_Check_return_opt_ _CRTIMP int __cdecl _vsprintf_p_l(_Out_writes_z_(_MaxCount) char * _DstBuf, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_params_(2) const char* _Format, _In_opt_ _locale_t _Locale,  va_list _ArgList);
_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl _vsprintf_s_l(_Out_writes_z_(_DstSize) char * _DstBuf, _In_ size_t _DstSize, _In_z_ _Printf_format_string_params_(2) const char * _Format, _In_opt_ _locale_t _Locale, va_list _ArgList);

_Check_return_opt_ _CRTIMP int __cdecl _scprintf_l(_In_z_ _Printf_format_string_params_(0) const char * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRTIMP int __cdecl _scprintf_p_l(_In_z_ _Printf_format_string_params_(0) const char * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRTIMP int __cdecl _vscprintf_l(_In_z_ _Printf_format_string_params_(2) const char * _Format, _In_opt_ _locale_t _Locale, va_list _ArgList);
_Check_return_opt_ _CRTIMP int __cdecl _vscprintf_p_l(_In_z_ _Printf_format_string_params_(2) const char * _Format, _In_opt_ _locale_t _Locale, va_list _ArgList);

_Check_return_opt_ _CRT_INSECURE_DEPRECATE(_snprintf_s_l) _CRTIMP int __cdecl _snprintf_l(_Out_writes_(_MaxCount) char * _DstBuf, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_params_(0) const char * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRTIMP int __cdecl _snprintf_c_l(_Out_writes_(_MaxCount) char * _DstBuf, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_params_(0) const char * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl _snprintf_s_l(_Out_writes_z_(_DstSize) char * _DstBuf, _In_ size_t _DstSize, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_params_(0) const char * _Format, _In_opt_ _locale_t _Locale, ...);
_Check_return_opt_ _CRT_INSECURE_DEPRECATE(_vsnprintf_s_l) _CRTIMP int __cdecl _vsnprintf_l(_Out_writes_(_MaxCount) char * _DstBuf, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_params_(2) const char * _Format, _In_opt_ _locale_t _Locale, va_list _ArgList);
_Check_return_opt_ _CRTIMP int __cdecl _vsnprintf_c_l(_Out_writes_(_MaxCount) char * _DstBuf, _In_ size_t _MaxCount, const char *, _In_opt_ _locale_t _Locale, va_list _ArgList);
_Check_return_opt_ _CRTIMP int __cdecl _vsnprintf_s_l(_Out_writes_z_(_DstSize) char * _DstBuf, _In_ size_t _DstSize, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_params_(2) const char* _Format,_In_opt_ _locale_t _Locale, va_list _ArgList);

#ifndef _WSTDIO_DEFINED

/* wide function prototypes, also declared in wchar.h  */

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
_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl vswscanf_s(const wchar_t * _Src, _In_z_ _Printf_format_string_ const wchar_t * _Format, va_list _ArgList);
#endif  /* __STDC_WANT_SECURE_LIB__ */
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(int, vswprintf_s, _Post_z_ wchar_t, _Dest, _In_z_ _Printf_format_string_ const wchar_t *, _Format, va_list, _Args)
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(int, vswscanf_s, _Post_z_ wchar_t, _Dest, _In_z_ _Printf_format_string_ const wchar_t *, _Format, va_list, _Args)
_Check_return_opt_ _CRTIMP int __cdecl vswscanf(const wchar_t * _srcBuf, _In_z_ _Printf_format_string_ const wchar_t * _Format, va_list _ArgList);

_Check_return_opt_ _CRTIMP int __cdecl _swprintf_c(_Out_writes_z_(_SizeInWords) wchar_t * _DstBuf, _In_ size_t _SizeInWords, _In_z_ _Printf_format_string_ const wchar_t * _Format, ...);
_Check_return_opt_ _CRTIMP int __cdecl _vswprintf_c(_Out_writes_z_(_SizeInWords) wchar_t * _DstBuf, _In_ size_t _SizeInWords, _In_z_ _Printf_format_string_ const wchar_t * _Format, va_list _ArgList);

_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl _snwprintf_s(_Out_writes_z_(_SizeInWords) wchar_t * _DstBuf, _In_ size_t _SizeInWords, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_ const wchar_t * _Format, ...);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2_ARGLIST(int, _snwprintf_s, _vsnwprintf_s, _Post_z_ wchar_t, _Dest, _In_ size_t, _Count, _In_z_ _Printf_format_string_ const wchar_t *, _Format)
_Check_return_opt_ _CRTIMP_ALTERNATIVE int __cdecl _vsnwprintf_s(_Out_writes_z_(_SizeInWords) wchar_t * _DstBuf, _In_ size_t _SizeInWords, _In_ size_t _MaxCount, _In_z_ _Printf_format_string_ const wchar_t * _Format, va_list _ArgList);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_3(int, _vsnwprintf_s, _Post_z_ wchar_t, _Dest, _In_ size_t, _Count, _In_z_ _Printf_format_string_ const wchar_t *, _Format, va_list, _Args)
#pragma warning(push)
#pragma warning(disable:4793)
__DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_ARGLIST_EX(int, __RETURN_POLICY_SAME, _CRTIMP, _snwprintf, _vsnwprintf, _Pre_notnull_ _Post_maybez_ wchar_t, _Out_writes_(_Count) _Post_maybez_, wchar_t, _Dest, _In_ size_t, _Count, _In_z_ _Printf_format_string_ const wchar_t *, _Format)
#pragma warning(pop)

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
#pragma warning(disable:4141 4996 4793)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_ARGLIST_EX(int, __RETURN_POLICY_SAME, _SWPRINTFS_DEPRECATED _CRTIMP, _swprintf, _swprintf_s, _vswprintf, vswprintf_s, _Pre_notnull_ _Post_z_, wchar_t, _Dest, _In_z_ _Printf_format_string_ const wchar_t *, _Format)
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_2_ARGLIST_EX(int, __RETURN_POLICY_SAME, _SWPRINTFS_DEPRECATED _CRTIMP, __swprintf_l, __vswprintf_l, _vswprintf_s_l, _Pre_notnull_ _Post_z_ wchar_t, _Pre_notnull_ _Post_z_, wchar_t, _Dest, _In_z_ _Printf_format_string_params_(2) const wchar_t *, _Format, _locale_t, _Plocinfo)
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
#pragma warning(push)
#pragma warning(disable:6530)
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
#pragma warning(pop)

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
inline _Check_return_ wint_t __CRTDECL getwchar()
        {return (fgetwc(stdin)); }   /* stdin */
inline _Check_return_opt_ wint_t __CRTDECL putwchar(_In_ wchar_t _C)
        {return (fputwc(_C, stdout)); }       /* stdout */
#endif  /* !defined (__cplusplus) || defined (_M_CEE_PURE) || defined (_CRT_GETPUTWCHAR_NOINLINE) */

#define getwc(_stm)             fgetwc(_stm)
#define putwc(_c,_stm)          fputwc(_c,_stm)
#define _putwc_nolock(_c,_stm)     _fputwc_nolock(_c,_stm)
#define _getwc_nolock(_stm)        _fgetwc_nolock(_stm)

#if defined (_CRT_DISABLE_PERFCRIT_LOCKS) && !defined (_DLL)
#define fgetwc(_stm)            _getwc_nolock(_stm)
#define fputwc(_c,_stm)         _putwc_nolock(_c,_stm)
#define ungetwc(_c,_stm)        _ungetwc_nolock(_c,_stm)
#endif  /* defined (_CRT_DISABLE_PERFCRIT_LOCKS) && !defined (_DLL) */

#define _WSTDIO_DEFINED
#endif  /* _WSTDIO_DEFINED */

#define _STDIO_DEFINED
#endif  /* _STDIO_DEFINED */


/* Macro definitions */

#if defined (_CRT_DISABLE_PERFCRIT_LOCKS) && !defined (_DLL)
#define feof(_stream)     ((_stream)->_flag & _IOEOF)
#define ferror(_stream)   ((_stream)->_flag & _IOERR)
#define _fileno(_stream)  ((_stream)->_file)
#define fgetc(_stream)     (--(_stream)->_cnt >= 0 \
                ? 0xff & *(_stream)->_ptr++ : _filbuf(_stream))
#define putc(_c,_stream)  (--(_stream)->_cnt >= 0 \
                ? 0xff & (*(_stream)->_ptr++ = (char)(_c)) :  _flsbuf((_c),(_stream)))
#define getc(_stream)    fgetc(_stream)
#define getchar()         getc(stdin)
#define putchar(_c)       putc((_c),stdout)
#endif  /* defined (_CRT_DISABLE_PERFCRIT_LOCKS) && !defined (_DLL) */


#define _fgetc_nolock(_stream)       (--(_stream)->_cnt >= 0 ? 0xff & *(_stream)->_ptr++ : _filbuf(_stream))
#define _fputc_nolock(_c,_stream)    (--(_stream)->_cnt >= 0 ? 0xff & (*(_stream)->_ptr++ = (char)(_c)) :  _flsbuf((_c),(_stream)))
#define _getc_nolock(_stream)       _fgetc_nolock(_stream)
#define _putc_nolock(_c, _stream)   _fputc_nolock(_c, _stream)
#define _getchar_nolock()           _getc_nolock(stdin)
#define _putchar_nolock(_c)         _putc_nolock((_c),stdout)
#define _getwchar_nolock()          _getwc_nolock(stdin)
#define _putwchar_nolock(_c)        _putwc_nolock((_c),stdout)

_CRTIMP void __cdecl _lock_file(_Inout_ FILE * _File);
_CRTIMP void __cdecl _unlock_file(_Inout_ FILE * _File);

_Check_return_opt_ _CRTIMP int __cdecl _fclose_nolock(_Inout_ FILE * _File);
_Check_return_opt_ _CRTIMP int __cdecl _fflush_nolock(_Inout_opt_ FILE * _File);
_Check_return_opt_ _CRTIMP size_t __cdecl _fread_nolock(_Out_writes_bytes_(_ElementSize*_Count) void * _DstBuf, _In_ size_t _ElementSize, _In_ size_t _Count, _Inout_ FILE * _File);
_Check_return_opt_ _CRTIMP size_t __cdecl _fread_nolock_s(_Out_writes_bytes_(_ElementSize*_Count) void * _DstBuf, _In_ size_t _DstSize, _In_ size_t _ElementSize, _In_ size_t _Count, _Inout_ FILE * _File);
_Check_return_opt_ _CRTIMP int __cdecl _fseek_nolock(_Inout_ FILE * _File, _In_ long _Offset, _In_ int _Origin);
_Check_return_ _CRTIMP long __cdecl _ftell_nolock(_Inout_ FILE * _File);
_Check_return_opt_ _CRTIMP int __cdecl _fseeki64_nolock(_Inout_ FILE * _File, _In_ __int64 _Offset, _In_ int _Origin);
_Check_return_ _CRTIMP __int64 __cdecl _ftelli64_nolock(_Inout_ FILE * _File);
_Check_return_opt_ _CRTIMP size_t __cdecl _fwrite_nolock(_In_reads_bytes_(_Size*_Count) const void * _DstBuf, _In_ size_t _Size, _In_ size_t _Count, _Inout_ FILE * _File);
_Check_return_opt_ _CRTIMP int __cdecl _ungetc_nolock(_In_ int _Ch, _Inout_ FILE * _File);

#if defined (_CRT_DISABLE_PERFCRIT_LOCKS) && !defined (_DLL)
#define fclose(_stm)                                            _fclose_nolock(_stm)
#define fflush(_stm)                                            _fflush_nolock(_stm)
#define fread(_DstBuf, _ElementSize, _Count, _File)             _fread_nolock(_DstBuf, _ElementSize, _Count, _File)
#define fread_s(_DstBuf, _DstSize, _ElementSize, _Count, _File) _fread_nolock_s(_DstBuf, _DstSize, _ElementSize, _Count, _File)
#define fseek(_stm,_offset,_origin)                             _fseek_nolock(_stm,_offset,_origin)
#define ftell(_stm)                                             _ftell_nolock(_stm)
#define _fseeki64(_stm,_offset,_origin)                         _fseeki64_nolock(_stm,_offset,_origin)
#define _ftelli64(_stm)                                         _ftelli64_nolock(_stm)
#define fwrite(_buf,_siz,_cnt,_stm)                             _fwrite_nolock(_buf,_siz,_cnt,_stm)
#define ungetc(_c,_stm)                                         _ungetc_nolock(_c,_stm)
#endif  /* defined (_CRT_DISABLE_PERFCRIT_LOCKS) && !defined (_DLL) */

#if !__STDC__

/* Non-ANSI names for compatibility */

#define P_tmpdir  _P_tmpdir
#define SYS_OPEN  _SYS_OPEN

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma push_macro("tempnam")
#undef tempnam
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

_CRT_NONSTDC_DEPRECATE(_tempnam) _CRTIMP char * __cdecl tempnam(_In_opt_z_ const char * _Directory, _In_opt_z_ const char * _FilePrefix);

#if defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC)
#pragma pop_macro("tempnam")
#endif  /* defined (_DEBUG) && defined (_CRTDBG_MAP_ALLOC) */

_Check_return_opt_ _CRT_NONSTDC_DEPRECATE(_fcloseall) _CRTIMP int __cdecl fcloseall(void);
_Check_return_ _CRT_NONSTDC_DEPRECATE(_fdopen) _CRTIMP FILE * __cdecl fdopen(_In_ int _FileHandle, _In_z_ const char * _Format);
_Check_return_opt_ _CRT_NONSTDC_DEPRECATE(_fgetchar) _CRTIMP int __cdecl fgetchar(void);
_Check_return_ _CRT_NONSTDC_DEPRECATE(_fileno) _CRTIMP int __cdecl fileno(_In_ FILE * _File);
_Check_return_opt_ _CRT_NONSTDC_DEPRECATE(_flushall) _CRTIMP int __cdecl flushall(void);
_Check_return_opt_ _CRT_NONSTDC_DEPRECATE(_fputchar) _CRTIMP int __cdecl fputchar(_In_ int _Ch);
_Check_return_ _CRT_NONSTDC_DEPRECATE(_getw) _CRTIMP int __cdecl getw(_Inout_ FILE * _File);
_Check_return_opt_ _CRT_NONSTDC_DEPRECATE(_putw) _CRTIMP int __cdecl putw(_In_ int _Ch, _Inout_ FILE * _File);
_Check_return_ _CRT_NONSTDC_DEPRECATE(_rmtmp) _CRTIMP int __cdecl rmtmp(void);

#endif  /* !__STDC__ */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#pragma pack(pop)

#endif  /* _INC_STDIO */
