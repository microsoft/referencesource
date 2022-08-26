/***
*io.h - declarations for low-level file handling and I/O functions
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file contains the function declarations for the low-level
*       file handling and I/O functions.
*
*       [Public]
*
****/

#pragma once

#ifndef _INC_IO
#define _INC_IO

#include <crtdefs.h>

/*
 * Currently, all MS C compilers for Win32 platforms default to 8 byte
 * alignment.
 */
#pragma pack(push,_CRT_PACKING)


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#ifndef _FSIZE_T_DEFINED
typedef unsigned long _fsize_t; /* Could be 64 bits for Win32 */
#define _FSIZE_T_DEFINED
#endif  /* _FSIZE_T_DEFINED */

#ifndef _FINDDATA_T_DEFINED

struct _finddata32_t {
        unsigned    attrib;
        __time32_t  time_create;    /* -1 for FAT file systems */
        __time32_t  time_access;    /* -1 for FAT file systems */
        __time32_t  time_write;
        _fsize_t    size;
        char        name[260];
};

struct _finddata32i64_t {
        unsigned    attrib;
        __time32_t  time_create;    /* -1 for FAT file systems */
        __time32_t  time_access;    /* -1 for FAT file systems */
        __time32_t  time_write;
        __int64     size;
        char        name[260];
};

struct _finddata64i32_t {
        unsigned    attrib;
        __time64_t  time_create;    /* -1 for FAT file systems */
        __time64_t  time_access;    /* -1 for FAT file systems */
        __time64_t  time_write;
        _fsize_t    size;
        char        name[260];
};

struct __finddata64_t {
        unsigned    attrib;
        __time64_t  time_create;    /* -1 for FAT file systems */
        __time64_t  time_access;    /* -1 for FAT file systems */
        __time64_t  time_write;
        __int64     size;
        char        name[260];
};

#ifdef _USE_32BIT_TIME_T
#define _finddata_t     _finddata32_t
#define _finddatai64_t  _finddata32i64_t

#define _findfirst      _findfirst32
#define _findnext       _findnext32
#define _findfirsti64   _findfirst32i64
#define _findnexti64     _findnext32i64

#else  /* _USE_32BIT_TIME_T */
#define _finddata_t     _finddata64i32_t
#define _finddatai64_t  __finddata64_t

#define _findfirst      _findfirst64i32
#define _findnext       _findnext64i32
#define _findfirsti64   _findfirst64
#define _findnexti64    _findnext64

#endif  /* _USE_32BIT_TIME_T */


#define _FINDDATA_T_DEFINED
#endif  /* _FINDDATA_T_DEFINED */

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

/* File attribute constants for _findfirst() */

#define _A_NORMAL       0x00    /* Normal file - No read/write restrictions */
#define _A_RDONLY       0x01    /* Read only file */
#define _A_HIDDEN       0x02    /* Hidden file */
#define _A_SYSTEM       0x04    /* System file */
#define _A_SUBDIR       0x10    /* Subdirectory */
#define _A_ARCH         0x20    /* Archive file */

/* function prototypes */

#ifndef _SIZE_T_DEFINED
#ifdef _WIN64
typedef unsigned __int64    size_t;
#else  /* _WIN64 */
typedef _W64 unsigned int   size_t;
#endif  /* _WIN64 */
#define _SIZE_T_DEFINED
#endif  /* _SIZE_T_DEFINED */

_Check_return_ _CRTIMP int __cdecl _access(_In_z_ const char * _Filename, _In_ int _AccessMode);
_Check_return_wat_ _CRTIMP errno_t __cdecl _access_s(_In_z_ const char * _Filename, _In_ int _AccessMode);
_Check_return_ _CRTIMP int __cdecl _chmod(_In_z_ const char * _Filename, _In_ int _Mode);
/* note that the newly added _chsize_s takes a 64 bit value */
_Check_return_ _CRTIMP int __cdecl _chsize(_In_ int _FileHandle, _In_ long _Size);
_Check_return_wat_ _CRTIMP errno_t __cdecl _chsize_s(_In_ int _FileHandle,_In_ __int64 _Size);
_Check_return_opt_ _CRTIMP int __cdecl _close(_In_ int _FileHandle);
_Check_return_opt_ _CRTIMP int __cdecl _commit(_In_ int _FileHandle);
_Check_return_ _CRT_INSECURE_DEPRECATE(_sopen_s) _CRTIMP int __cdecl _creat(_In_z_ const char * _Filename, _In_ int _PermissionMode);
_Check_return_ _CRTIMP int __cdecl _dup(_In_ int _FileHandle);
_Check_return_ _CRTIMP int __cdecl _dup2(_In_ int _FileHandleSrc, _In_ int _FileHandleDst);
_Check_return_ _CRTIMP int __cdecl _eof(_In_ int _FileHandle);
_Check_return_ _CRTIMP long __cdecl _filelength(_In_ int _FileHandle);
_Check_return_ _CRTIMP intptr_t __cdecl _findfirst32(_In_z_ const char * _Filename, _Out_ struct _finddata32_t * _FindData);
_Check_return_ _CRTIMP int __cdecl _findnext32(_In_ intptr_t _FindHandle, _Out_ struct _finddata32_t * _FindData);
_Check_return_opt_ _CRTIMP int __cdecl _findclose(_In_ intptr_t _FindHandle);
_Check_return_ _CRTIMP int __cdecl _isatty(_In_ int _FileHandle);
_CRTIMP int __cdecl _locking(_In_ int _FileHandle, _In_ int _LockMode, _In_ long _NumOfBytes);
_Check_return_opt_ _CRTIMP long __cdecl _lseek(_In_ int _FileHandle, _In_ long _Offset, _In_ int _Origin);
_Check_return_wat_ _CRTIMP errno_t __cdecl _mktemp_s(_Inout_updates_z_(_Size) char * _TemplateName, _In_ size_t _Size);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_0(errno_t, _mktemp_s, _Prepost_z_ char, _TemplateName)
_Check_return_ __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0(char *, __RETURN_POLICY_DST, _CRTIMP, _mktemp, _Inout_z_, char, _TemplateName)
#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP
_Check_return_ _CRTIMP int __cdecl _pipe(_Inout_updates_(2) int * _PtHandles, _In_ unsigned int _PipeSize, _In_ int _TextMode);
#endif  /* _CRT_USE_WINAPI_FAMILY_DESKTOP_APP */
_Check_return_ _CRTIMP int __cdecl _read(_In_ int _FileHandle, _Out_writes_bytes_(_MaxCharCount) void * _DstBuf, _In_ unsigned int _MaxCharCount);

#ifndef _CRT_DIRECTORY_DEFINED
#define _CRT_DIRECTORY_DEFINED
_CRTIMP int __cdecl remove(_In_z_ const char * _Filename);
_Check_return_ _CRTIMP int __cdecl rename(_In_z_ const char * _OldFilename, _In_z_ const char * _NewFilename);
_CRTIMP int __cdecl _unlink(_In_z_ const char * _Filename);
#if !__STDC__
_CRT_NONSTDC_DEPRECATE(_unlink) _CRTIMP int __cdecl unlink(_In_z_ const char * _Filename);
#endif  /* !__STDC__ */
#endif  /* _CRT_DIRECTORY_DEFINED */

_Check_return_ _CRTIMP int __cdecl _setmode(_In_ int _FileHandle, _In_ int _Mode);
_Check_return_ _CRTIMP long __cdecl _tell(_In_ int _FileHandle);
_CRT_INSECURE_DEPRECATE(_umask_s) _CRTIMP int __cdecl _umask(_In_ int _Mode);
_Check_return_wat_ _CRTIMP errno_t __cdecl _umask_s(_In_ int _NewMode, _Out_ int * _OldMode);
_CRTIMP int __cdecl _write(_In_ int _FileHandle, _In_reads_bytes_(_MaxCharCount) const void * _Buf, _In_ unsigned int _MaxCharCount);

_Check_return_ _CRTIMP __int64 __cdecl _filelengthi64(_In_ int _FileHandle);
_Check_return_ _CRTIMP intptr_t __cdecl _findfirst32i64(_In_z_ const char * _Filename, _Out_ struct _finddata32i64_t * _FindData);
_Check_return_ _CRTIMP intptr_t __cdecl _findfirst64i32(_In_z_ const char * _Filename, _Out_ struct _finddata64i32_t * _FindData);
_Check_return_ _CRTIMP intptr_t __cdecl _findfirst64(_In_z_ const char * _Filename, _Out_ struct __finddata64_t * _FindData);
_Check_return_ _CRTIMP int __cdecl _findnext32i64(_In_ intptr_t _FindHandle, _Out_ struct _finddata32i64_t * _FindData);
_Check_return_ _CRTIMP int __cdecl _findnext64i32(_In_ intptr_t _FindHandle, _Out_ struct _finddata64i32_t * _FindData);
_Check_return_ _CRTIMP int __cdecl _findnext64(_In_ intptr_t _FindHandle, _Out_ struct __finddata64_t * _FindData);
_Check_return_opt_ _CRTIMP __int64 __cdecl _lseeki64(_In_ int _FileHandle, _In_ __int64 _Offset, _In_ int _Origin);
_Check_return_ _CRTIMP __int64 __cdecl _telli64(_In_ int _FileHandle);

_Check_return_wat_ _CRTIMP errno_t __cdecl _sopen_s(_Out_ int * _FileHandle, _In_z_ const char * _Filename,_In_ int _OpenFlag, _In_ int _ShareFlag, _In_ int _PermissionMode);
_Check_return_ errno_t __cdecl _sopen_s_nolock(_Out_ int * _FileHandle, _In_z_ const char * _Filename,_In_ int _OpenFlag, _In_ int _ShareFlag, _In_ int _PermissionMode);

#if !defined (__cplusplus)
_Check_return_ _CRT_INSECURE_DEPRECATE(_sopen_s) _CRTIMP int __cdecl _open(_In_z_ const char * _Filename, _In_ int _OpenFlag, ...);
_Check_return_ _CRT_INSECURE_DEPRECATE(_sopen_s) _CRTIMP int __cdecl _sopen(_In_z_ const char * _Filename, _In_ int _OpenFlag, int _ShareFlag, ...);
#else  /* !defined (__cplusplus) */

/* these function do not validate pmode; use _sopen_s */
extern "C++" _Check_return_ _CRT_INSECURE_DEPRECATE(_sopen_s) _CRTIMP int __cdecl _open(_In_z_ const char * _Filename, _In_ int _Openflag, _In_ int _PermissionMode = 0);
extern "C++" _Check_return_ _CRT_INSECURE_DEPRECATE(_sopen_s) _CRTIMP int __cdecl _sopen(_In_z_ const char * _Filename, _In_ int _Openflag, _In_ int _ShareFlag, _In_ int _PermissionMode = 0);

#endif  /* !defined (__cplusplus) */

#ifndef _WIO_DEFINED

/* wide function prototypes, also declared in wchar.h  */

_Check_return_ _CRTIMP int __cdecl _waccess(_In_z_ const wchar_t * _Filename, _In_ int _AccessMode);
_Check_return_wat_ _CRTIMP errno_t __cdecl _waccess_s(_In_z_ const wchar_t * _Filename, _In_ int _AccessMode);
_Check_return_ _CRTIMP int __cdecl _wchmod(_In_z_ const wchar_t * _Filename, _In_ int _Mode);
_Check_return_ _CRT_INSECURE_DEPRECATE(_wsopen_s) _CRTIMP int __cdecl _wcreat(_In_z_ const wchar_t * _Filename, _In_ int _PermissionMode);
_Check_return_ _CRTIMP intptr_t __cdecl _wfindfirst32(_In_z_ const wchar_t * _Filename, _Out_ struct _wfinddata32_t * _FindData);
_Check_return_ _CRTIMP int __cdecl _wfindnext32(_In_ intptr_t _FindHandle, _Out_ struct _wfinddata32_t * _FindData);
_CRTIMP int __cdecl _wunlink(_In_z_ const wchar_t * _Filename);
_Check_return_ _CRTIMP int __cdecl _wrename(_In_z_ const wchar_t * _OldFilename, _In_z_ const wchar_t * _NewFilename);
_CRTIMP errno_t __cdecl _wmktemp_s(_Inout_updates_z_(_SizeInWords) wchar_t * _TemplateName, _In_ size_t _SizeInWords);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_0(errno_t, _wmktemp_s, _Prepost_z_ wchar_t, _TemplateName)
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

int  __cdecl __lock_fhandle(_In_ int _Filehandle);
void __cdecl _unlock_fhandle(_In_ int _Filehandle);

#ifdef _CRTBLD
#ifndef _NOT_CRTL_BUILD_
_Check_return_ int __cdecl _chsize_nolock(_In_ int _FileHandle,_In_ __int64 _Size);
_Check_return_opt_ int __cdecl _close_nolock(_In_ int _FileHandle);
_Check_return_opt_ long __cdecl _lseek_nolock(_In_ int _FileHandle, _In_ long _Offset, _In_ int _Origin);
_Check_return_ int __cdecl _setmode_nolock(_In_ int _FileHandle, _In_ int _Mode);
_Check_return_ int __cdecl _read_nolock(_In_ int _FileHandle, _Out_writes_bytes_(_MaxCharCount) void * _DstBuf, _In_ unsigned int _MaxCharCount);
_Check_return_ int __cdecl _write_nolock(_In_ int _FileHandle, _In_reads_bytes_(_MaxCharCount) const void * _Buf, _In_ unsigned int _MaxCharCount);
_Check_return_opt_ __int64 __cdecl _lseeki64_nolock(_In_ int _FileHandle, _In_ __int64 _Offset, _In_ int _Origin);

#if defined (_CRT_DISABLE_PERFCRIT_LOCKS) && !defined (_DLL)
#define _chsize(fh, size)               _chsize_nolock(fh, size)
#define _close(fh)                      _close_nolock(fh)
#define _lseek(fh, offset, origin)      _lseek_nolock(fh, offset, origin)
#define _setmode(fh, mode)              _setmode_nolock(fh, mode)
#define _read(fh, buff, count)          _read_nolock(fh, buff, count)
#define _write(fh, buff, count)         _write_nolock(fh, buff, count)
#define _lseeki64(fh,offset,origin)     _lseeki64_nolock(fh,offset,origin)
#endif  /* defined (_CRT_DISABLE_PERFCRIT_LOCKS) && !defined (_DLL) */

#endif  /* _NOT_CRTL_BUILD_ */
#endif  /* _CRTBLD */

_CRTIMP intptr_t __cdecl _get_osfhandle(_In_ int _FileHandle);
_CRTIMP int __cdecl _open_osfhandle(_In_ intptr_t _OSFileHandle, _In_ int _Flags);

#if !__STDC__

/* Non-ANSI names for compatibility */

#pragma warning(push)
#pragma warning(disable: 4141) /* Using deprecated twice */
_Check_return_ _CRT_NONSTDC_DEPRECATE(_access) _CRTIMP int __cdecl access(_In_z_ const char * _Filename, _In_ int _AccessMode);
_Check_return_ _CRT_NONSTDC_DEPRECATE(_chmod) _CRTIMP int __cdecl chmod(_In_z_ const char * _Filename, int _AccessMode);
_Check_return_ _CRT_NONSTDC_DEPRECATE(_chsize) _CRTIMP int __cdecl chsize(_In_ int _FileHandle, _In_ long _Size);
_Check_return_opt_ _CRT_NONSTDC_DEPRECATE(_close) _CRTIMP int __cdecl close(_In_ int _FileHandle);
_Check_return_ _CRT_NONSTDC_DEPRECATE(_creat) _CRT_INSECURE_DEPRECATE(_sopen_s) _CRTIMP int __cdecl creat(_In_z_ const char * _Filename, _In_ int _PermissionMode);
_Check_return_ _CRT_NONSTDC_DEPRECATE(_dup) _CRTIMP int __cdecl dup(_In_ int _FileHandle);
_Check_return_ _CRT_NONSTDC_DEPRECATE(_dup2) _CRTIMP int __cdecl dup2(_In_ int _FileHandleSrc, _In_ int _FileHandleDst);
_Check_return_ _CRT_NONSTDC_DEPRECATE(_eof) _CRTIMP int __cdecl eof(_In_ int _FileHandle);
_Check_return_ _CRT_NONSTDC_DEPRECATE(_filelength) _CRTIMP long __cdecl filelength(_In_ int _FileHandle);
_Check_return_ _CRT_NONSTDC_DEPRECATE(_isatty) _CRTIMP int __cdecl isatty(_In_ int _FileHandle);
_Check_return_ _CRT_NONSTDC_DEPRECATE(_locking) _CRTIMP int __cdecl locking(_In_ int _FileHandle, _In_ int _LockMode, _In_ long _NumOfBytes);
_Check_return_opt_ _CRT_NONSTDC_DEPRECATE(_lseek) _CRTIMP long __cdecl lseek(_In_ int _FileHandle, _In_ long _Offset, _In_ int _Origin);
_CRT_NONSTDC_DEPRECATE(_mktemp) _CRT_INSECURE_DEPRECATE(_mktemp_s) _CRTIMP char * __cdecl mktemp(_Inout_z_ char * _TemplateName);
_Check_return_ _CRT_NONSTDC_DEPRECATE(_open) _CRT_INSECURE_DEPRECATE(_sopen_s) _CRTIMP int __cdecl open(_In_z_ const char * _Filename, _In_ int _OpenFlag, ...);
_CRT_NONSTDC_DEPRECATE(_read) _CRTIMP int __cdecl read(int _FileHandle, _Out_writes_bytes_(_MaxCharCount) void * _DstBuf, _In_ unsigned int _MaxCharCount);
_Check_return_ _CRT_NONSTDC_DEPRECATE(_setmode) _CRTIMP int __cdecl setmode(_In_ int _FileHandle, _In_ int _Mode);
_CRT_NONSTDC_DEPRECATE(_sopen) _CRT_INSECURE_DEPRECATE(_sopen_s) _CRTIMP int __cdecl sopen(const char * _Filename, _In_ int _OpenFlag, _In_ int _ShareFlag, ...);
_Check_return_ _CRT_NONSTDC_DEPRECATE(_tell) _CRTIMP long __cdecl tell(_In_ int _FileHandle);
_CRT_NONSTDC_DEPRECATE(_umask) _CRT_INSECURE_DEPRECATE(_umask_s) _CRTIMP int __cdecl umask(_In_ int _Mode);
_Check_return_ _CRT_NONSTDC_DEPRECATE(_write) _CRTIMP int __cdecl write(_In_ int _Filehandle, _In_reads_bytes_(_MaxCharCount) const void * _Buf, _In_ unsigned int _MaxCharCount);
#pragma warning(pop)

#endif  /* !__STDC__ */

#ifdef __cplusplus
}
#endif  /* __cplusplus */


#pragma pack(pop)

#endif  /* _INC_IO */
