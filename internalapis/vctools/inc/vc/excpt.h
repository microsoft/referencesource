/***
*excpt.h - defines exception values, types and routines
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file contains the definitions and prototypes for the compiler-
*       dependent intrinsics, support functions and keywords which implement
*       the structured exception handling extensions.
*
*       [Public]
*
****/

#pragma once

#ifndef _INC_EXCPT
#define _INC_EXCPT

#include <crtdefs.h>

/*
 * Currently, all MS C compilers for Win32 platforms default to 8 byte
 * alignment.
 */
#pragma pack(push,_CRT_PACKING)

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/*
 * Exception disposition return values.
 */
typedef enum _EXCEPTION_DISPOSITION {
    ExceptionContinueExecution,
    ExceptionContinueSearch,
    ExceptionNestedException,
    ExceptionCollidedUnwind
} EXCEPTION_DISPOSITION;


/*
 * Prototype for SEH support function.
 */

#ifdef _M_IX86

/*
 * Declarations to keep MS C 8 (386/486) compiler happy
 */
struct _EXCEPTION_RECORD;
struct _CONTEXT;

EXCEPTION_DISPOSITION __cdecl _except_handler (
    _In_ struct _EXCEPTION_RECORD *_ExceptionRecord,
    _In_ void * _EstablisherFrame,
    _Inout_ struct _CONTEXT *_ContextRecord,
    _Inout_ void * _DispatcherContext
    );

#elif defined (_M_X64) || defined (_M_ARM)

/*
 * Declarations to keep AMD64 compiler happy
 */
struct _EXCEPTION_RECORD;
struct _CONTEXT;
struct _DISPATCHER_CONTEXT;

#ifndef _M_CEE_PURE

_CRTIMP EXCEPTION_DISPOSITION __C_specific_handler (
    _In_ struct _EXCEPTION_RECORD * ExceptionRecord,
    _In_ void * EstablisherFrame,
    _Inout_ struct _CONTEXT * ContextRecord,
    _Inout_ struct _DISPATCHER_CONTEXT * DispatcherContext
);

#endif  /* _M_CEE_PURE */

#endif  /* defined (_M_X64) || defined (_M_ARM) */


/*
 * Keywords and intrinsics for SEH
 */

#define GetExceptionCode            _exception_code
#define exception_code              _exception_code
#define GetExceptionInformation     (struct _EXCEPTION_POINTERS *)_exception_info
#define exception_info              (struct _EXCEPTION_POINTERS *)_exception_info
#define AbnormalTermination         _abnormal_termination
#define abnormal_termination        _abnormal_termination

unsigned long __cdecl _exception_code(void);
void *        __cdecl _exception_info(void);
int           __cdecl _abnormal_termination(void);


/*
 * Legal values for expression in except().
 */

#define EXCEPTION_EXECUTE_HANDLER       1
#define EXCEPTION_CONTINUE_SEARCH       0
#define EXCEPTION_CONTINUE_EXECUTION    -1

#ifdef _CRTBLD
/*
 * for convenience, define a type name for a pointer to signal-handler
 */

typedef void (__cdecl * _PHNDLR)(int);

/*
 * Exception-action table used by the C runtime to identify and dispose of
 * exceptions corresponding to C runtime errors or C signals.
 */
struct _XCPT_ACTION {

    /*
     * exception code or number. defined by the host OS.
     */
    unsigned long XcptNum;

    /*
     * signal code or number. defined by the C runtime.
     */
    int SigNum;

    /*
     * exception action code. either a special code or the address of
     * a handler function. always determines how the exception filter
     * should dispose of the exception.
     */
    _PHNDLR XcptAction;
};

extern const struct _XCPT_ACTION _XcptActTab[];

/*
 * number of entries in the exception-action table
 */
extern const int _XcptActTabCount;

/*
 * size of exception-action table (in bytes)
 */
extern const int _XcptActTabSize;

/*
 * index of the first floating point exception entry
 */
extern const int _First_FPE_Indx;

/*
 * number of FPE entries
 */
extern const int _Num_FPE;

/*
 * return values and prototype for the exception filter function used in the
 * C startup
 */

struct _EXCEPTION_POINTERS;

int __cdecl __CppXcptFilter(_In_ unsigned long _ExceptionNum, _In_ struct _EXCEPTION_POINTERS * _ExceptionPtr);
int __cdecl _XcptFilter(_In_ unsigned long _ExceptionNum, _In_ struct _EXCEPTION_POINTERS * _ExceptionPtr);

#endif  /* _CRTBLD */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#pragma pack(pop)

#endif  /* _INC_EXCPT */
