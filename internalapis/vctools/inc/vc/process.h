/***
*process.h - definition and declarations for process control functions
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file defines the modeflag values for spawnxx calls.
*       Also contains the function argument declarations for all
*       process control related routines.
*
*       [Public]
*
****/

#pragma once

#ifndef _INC_PROCESS
#define _INC_PROCESS

#include <crtdefs.h>


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/* modeflag values for _spawnxx routines */

#define _P_WAIT         0
#define _P_NOWAIT       1
#define _OLD_P_OVERLAY  2
#define _P_NOWAITO      3
#define _P_DETACH       4

#define _P_OVERLAY      2

/* Action codes for _cwait(). The action code argument to _cwait is ignored
   on Win32 though it is accepted for compatibilty with old MS CRT libs */
#define _WAIT_CHILD      0
#define _WAIT_GRANDCHILD 1


/* function prototypes */

#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP
_CRTIMP uintptr_t __cdecl _beginthread (_In_ void (__cdecl * _StartAddress) (void *),
        _In_ unsigned _StackSize, _In_opt_ void * _ArgList);
_CRTIMP void __cdecl _endthread(void);
_CRTIMP uintptr_t __cdecl _beginthreadex(_In_opt_ void * _Security, _In_ unsigned _StackSize,
        _In_ unsigned (__stdcall * _StartAddress) (void *), _In_opt_ void * _ArgList,
        _In_ unsigned _InitFlag, _Out_opt_ unsigned * _ThrdAddr);
_CRTIMP void __cdecl _endthreadex(_In_ unsigned _Retval);
#endif  /* _CRT_USE_WINAPI_FAMILY_DESKTOP_APP */

#ifndef _CRT_TERMINATE_DEFINED
#define _CRT_TERMINATE_DEFINED
_CRTIMP __declspec(noreturn) void __cdecl exit(_In_ int _Code);
_CRTIMP __declspec(noreturn) void __cdecl _exit(_In_ int _Code);
_CRTIMP __declspec(noreturn) void __cdecl abort(void);
#endif  /* _CRT_TERMINATE_DEFINED */

_CRTIMP void __cdecl _cexit(void);
_CRTIMP void __cdecl _c_exit(void);

#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP
_CRTIMP int __cdecl _getpid(void);
_CRTIMP intptr_t __cdecl _cwait(_Out_opt_ int * _TermStat, _In_ intptr_t _ProcHandle, _In_ int _Action);
_CRTIMP intptr_t __cdecl _execl(_In_z_ const char * _Filename, _In_z_ const char * _ArgList, ...);
_CRTIMP intptr_t __cdecl _execle(_In_z_ const char * _Filename, _In_z_ const char * _ArgList, ...);
_CRTIMP intptr_t __cdecl _execlp(_In_z_ const char * _Filename, _In_z_ const char * _ArgList, ...);
_CRTIMP intptr_t __cdecl _execlpe(_In_z_ const char * _Filename, _In_z_ const char * _ArgList, ...);
_CRTIMP intptr_t __cdecl _execv(_In_z_ const char * _Filename, _In_z_ const char * const * _ArgList);
_CRTIMP intptr_t __cdecl _execve(_In_z_ const char * _Filename, _In_z_ const char * const * _ArgList, _In_opt_z_ const char * const * _Env);
_CRTIMP intptr_t __cdecl _execvp(_In_z_ const char * _Filename, _In_z_ const char * const * _ArgList);
_CRTIMP intptr_t __cdecl _execvpe(_In_z_ const char * _Filename, _In_z_ const char * const * _ArgList, _In_opt_z_ const char * const * _Env);
_CRTIMP intptr_t __cdecl _spawnl(_In_ int _Mode, _In_z_ const char * _Filename, _In_z_ const char * _ArgList, ...);
_CRTIMP intptr_t __cdecl _spawnle(_In_ int _Mode, _In_z_ const char * _Filename, _In_z_ const char * _ArgList, ...);
_CRTIMP intptr_t __cdecl _spawnlp(_In_ int _Mode, _In_z_ const char * _Filename, _In_z_ const char * _ArgList, ...);
_CRTIMP intptr_t __cdecl _spawnlpe(_In_ int _Mode, _In_z_ const char * _Filename, _In_z_ const char * _ArgList, ...);
_CRTIMP intptr_t __cdecl _spawnv(_In_ int _Mode, _In_z_ const char * _Filename, _In_z_ const char * const * _ArgList);
_CRTIMP intptr_t __cdecl _spawnve(_In_ int _Mode, _In_z_ const char * _Filename, _In_z_ const char * const * _ArgList,
        _In_opt_z_ const char * const * _Env);
_CRTIMP intptr_t __cdecl _spawnvp(_In_ int _Mode, _In_z_ const char * _Filename, _In_z_ const char * const * _ArgList);
_CRTIMP intptr_t __cdecl _spawnvpe(_In_ int _Mode, _In_z_ const char * _Filename, _In_z_ const char * const * _ArgList,
        _In_opt_z_ const char * const * _Env);

#ifndef _CRT_SYSTEM_DEFINED
#define _CRT_SYSTEM_DEFINED
_CRTIMP int __cdecl system(_In_opt_z_ const char * _Command);
#endif  /* _CRT_SYSTEM_DEFINED */

#ifndef _WPROCESS_DEFINED
/* wide function prototypes, also declared in wchar.h  */
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

/*
 * Security check initialization and failure reporting used by /GS security
 * checks.
 */
#if !defined (_M_CEE)
void __cdecl __security_init_cookie(void);
#ifdef _M_IX86
void __fastcall __security_check_cookie(_In_ uintptr_t _StackCookie);
__declspec(noreturn) void __cdecl __report_gsfailure(void);
#else  /* _M_IX86 */
void __cdecl __security_check_cookie(_In_ uintptr_t _StackCookie);
__declspec(noreturn) void __cdecl __report_gsfailure(_In_ uintptr_t _StackCookie);
#endif  /* _M_IX86 */
#endif  /* !defined (_M_CEE) */
extern uintptr_t __security_cookie;

#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP
/* --------- The following functions are OBSOLETE --------- */
/*
 * The Win32 API LoadLibrary, FreeLibrary and GetProcAddress should be used
 * instead.
 */

_CRT_OBSOLETE(LoadLibrary) intptr_t __cdecl _loaddll(_In_z_ char * _Filename);
_CRT_OBSOLETE(FreeLibrary) int __cdecl _unloaddll(_In_ intptr_t _Handle);
_CRT_OBSOLETE(GetProcAddress) int (__cdecl * __cdecl _getdllprocaddr(_In_ intptr_t _Handle, _In_opt_z_ char * _ProcedureName, _In_ intptr_t _Ordinal))(void);

/* --------- The preceding functions are OBSOLETE --------- */
#endif  /* _CRT_USE_WINAPI_FAMILY_DESKTOP_APP */


#ifdef _DECL_DLLMAIN
/*
 * Declare DLL notification (initialization/termination) routines
 *      The preferred method is for the user to provide DllMain() which will
 *      be called automatically by the DLL entry point defined by the C run-
 *      time library code.  If the user wants to define the DLL entry point
 *      routine, the user's entry point must call _CRT_INIT on all types of
 *      notifications, as the very first thing on attach notifications and
 *      as the very last thing on detach notifications.
 */
#ifdef _WINDOWS_
#if defined (MRTDLL) && defined(_CRTBLD)
BOOL __clrcall DllMain(_In_ HANDLE _HDllHandle, _In_ DWORD _Reason, _In_opt_ LPVOID _Reserved);
BOOL _CRT_INIT(_In_ HANDLE _HDllHandle, _In_ DWORD _Reason, _In_opt_ LPVOID _Reserved);
#else  /* defined (MRTDLL) */
BOOL WINAPI DllMain(_In_ HANDLE _HDllHandle, _In_ DWORD _Reason, _In_opt_ LPVOID _Reserved);
BOOL WINAPI _CRT_INIT(_In_ HANDLE _HDllHandle, _In_ DWORD _Reason, _In_opt_ LPVOID _Reserved);
#endif  /* defined (MRTDLL) */
BOOL WINAPI _wCRT_INIT(_In_ HANDLE _HDllHandle, _In_ DWORD _Reason, _In_opt_ LPVOID _Reserved);
extern BOOL (WINAPI * const _pRawDllMain)(HANDLE, DWORD, LPVOID);
#else  /* _WINDOWS_ */
int __stdcall DllMain(_In_ void * _HDllHandle, _In_ unsigned _Reason, _In_opt_ void * _Reserved);
int __stdcall _CRT_INIT(_In_ void * _HDllHandle, _In_ unsigned _Reason, _In_opt_ void * _Reserved);
int __stdcall _wCRT_INIT(_In_ void * _HDllHandle, _In_ unsigned _Reason, _In_opt_ void * _Reserved);
extern int (__stdcall * const _pRawDllMain)(void *, unsigned, void *);
#endif  /* _WINDOWS_ */
#endif  /* _DECL_DLLMAIN */

#if !__STDC__

/* Non-ANSI names for compatibility */

#define P_WAIT          _P_WAIT
#define P_NOWAIT        _P_NOWAIT
#define P_OVERLAY       _P_OVERLAY
#define OLD_P_OVERLAY   _OLD_P_OVERLAY
#define P_NOWAITO       _P_NOWAITO
#define P_DETACH        _P_DETACH
#define WAIT_CHILD      _WAIT_CHILD
#define WAIT_GRANDCHILD _WAIT_GRANDCHILD

#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP

/* current declarations */
_CRT_NONSTDC_DEPRECATE(_cwait) _CRTIMP intptr_t __cdecl cwait(_Out_opt_ int * _TermStat, _In_ intptr_t _ProcHandle, _In_ int _Action);
_CRT_NONSTDC_DEPRECATE(_execl) _CRTIMP intptr_t __cdecl execl(_In_z_ const char * _Filename, _In_z_ const char * _ArgList, ...);
_CRT_NONSTDC_DEPRECATE(_execle) _CRTIMP intptr_t __cdecl execle(_In_z_ const char * _Filename, _In_z_ const char * _ArgList, ...);
_CRT_NONSTDC_DEPRECATE(_execlp) _CRTIMP intptr_t __cdecl execlp(_In_z_ const char * _Filename, _In_z_ const char * _ArgList, ...);
_CRT_NONSTDC_DEPRECATE(_execlpe) _CRTIMP intptr_t __cdecl execlpe(_In_z_ const char * _Filename, _In_z_ const char * _ArgList, ...);
_CRT_NONSTDC_DEPRECATE(_execv) _CRTIMP intptr_t __cdecl execv(_In_z_ const char * _Filename, _In_z_ const char * const * _ArgList);
_CRT_NONSTDC_DEPRECATE(_execve) _CRTIMP intptr_t __cdecl execve(_In_z_ const char * _Filename, _In_z_ const char * const * _ArgList, _In_opt_z_ const char * const * _Env);
_CRT_NONSTDC_DEPRECATE(_execvp) _CRTIMP intptr_t __cdecl execvp(_In_z_ const char * _Filename, _In_z_ const char * const * _ArgList);
_CRT_NONSTDC_DEPRECATE(_execvpe) _CRTIMP intptr_t __cdecl execvpe(_In_z_ const char * _Filename, _In_z_ const char * const * _ArgList, _In_opt_z_ const char * const * _Env);
_CRT_NONSTDC_DEPRECATE(_spawnl) _CRTIMP intptr_t __cdecl spawnl(_In_ int, _In_z_ const char * _Filename, _In_z_ const char * _ArgList, ...);
_CRT_NONSTDC_DEPRECATE(_spawnle) _CRTIMP intptr_t __cdecl spawnle(_In_ int, _In_z_ const char * _Filename, _In_z_ const char * _ArgList, ...);
_CRT_NONSTDC_DEPRECATE(_spawnlp) _CRTIMP intptr_t __cdecl spawnlp(_In_ int, _In_z_ const char * _Filename, _In_z_ const char * _ArgList, ...);
_CRT_NONSTDC_DEPRECATE(_spawnlpe) _CRTIMP intptr_t __cdecl spawnlpe(_In_ int, _In_z_ const char * _Filename, _In_z_ const char * _ArgList, ...);
_CRT_NONSTDC_DEPRECATE(_spawnv) _CRTIMP intptr_t __cdecl spawnv(_In_ int, _In_z_ const char * _Filename, _In_z_ const char * const * _ArgList);
_CRT_NONSTDC_DEPRECATE(_spawnve) _CRTIMP intptr_t __cdecl spawnve(_In_ int, _In_z_ const char * _Filename, _In_z_ const char * const * _ArgList,
        _In_opt_z_ const char * const * _Env);
_CRT_NONSTDC_DEPRECATE(_spawnvp) _CRTIMP intptr_t __cdecl spawnvp(_In_ int, _In_z_ const char * _Filename, _In_z_ const char * const * _ArgList);
_CRT_NONSTDC_DEPRECATE(_spawnvpe) _CRTIMP intptr_t __cdecl spawnvpe(_In_ int, _In_z_ const char * _Filename, _In_z_ const char * const * _ArgList,
        _In_opt_z_ const char * const * _Env);

_CRT_NONSTDC_DEPRECATE(_getpid) _CRTIMP int __cdecl getpid(void);

#endif  /* _CRT_USE_WINAPI_FAMILY_DESKTOP_APP */

#endif  /* !__STDC__ */

#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif  /* _INC_PROCESS */
