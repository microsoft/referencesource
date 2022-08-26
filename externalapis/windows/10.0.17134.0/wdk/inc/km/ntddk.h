/*++ BUILD Version: 0184    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

 ntddk.h

Abstract:

    This module defines the NT types, constants, and functions that are
    exposed to device drivers.

Revision History:

--*/

#ifndef _NTDDK_
#define _NTDDK_

#if !defined(_NTHAL_) && !defined(_NTIFS_)
#define _NTDDK_INCLUDED_
#define _DDK_DRIVER_
#endif

#ifndef RC_INVOKED
#if _MSC_VER < 1300
#error Compiler version not supported by Windows DDK
#endif
#endif // RC_INVOKED

#define NT_INCLUDED
#define _CTYPE_DISABLE_MACROS

#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning(disable:4115) // named type definition in parentheses
#pragma warning(disable:4201) // nameless struct/union
#pragma warning(disable:4214) // bit field types other than int
#pragma warning(disable:4625) // copy constructor was implicitly defined as deleted
#pragma warning(disable:4626) // assignment operator was implicitly defined as deleted
#pragma warning(disable:4668) // #if not_defined treated as #if 0
#pragma warning(disable:4820) // padding added

#include <wdm.h>
#include <excpt.h>
#include <ntdef.h>
#include <ntstatus.h>
#include <bugcodes.h>
#include <ntiologc.h>

#ifdef __cplusplus
extern "C" {
#endif


//
// Define types that are not exported.
//

typedef struct _BUS_HANDLER *PBUS_HANDLER;
typedef struct _CALLBACK_OBJECT *PCALLBACK_OBJECT;
typedef struct _DEVICE_HANDLER_OBJECT *PDEVICE_HANDLER_OBJECT;
#if defined(_NTHAL_INCLUDED_)
typedef struct _KPROCESS *PEPROCESS;
typedef struct _ETHREAD *PETHREAD;
typedef struct _KAFFINITY_EX *PKAFFINITY_EX;
#elif defined(_NTIFS_INCLUDED_)
typedef struct _KPROCESS *PEPROCESS;
typedef struct _KTHREAD *PETHREAD;
#else
typedef struct _EPROCESS *PEPROCESS;
typedef struct _ETHREAD *PETHREAD;
#endif
typedef struct _EJOB *PEJOB;
#if (NTDDI_VERSION >= NTDDI_WIN10_RS1)
typedef struct _EJOB *PESILO;
typedef struct _SILO_MONITOR *PSILO_MONITOR;
#endif
typedef struct _IO_TIMER *PIO_TIMER;
typedef struct _KINTERRUPT *PKINTERRUPT;
typedef struct _KTHREAD *PKTHREAD, *PRKTHREAD;
typedef struct _OBJECT_TYPE *POBJECT_TYPE;
typedef struct _PEB *PPEB;
typedef struct _IMAGE_NT_HEADERS *PIMAGE_NT_HEADERS32;
typedef struct _IMAGE_NT_HEADERS64 *PIMAGE_NT_HEADERS64;
#ifdef _WIN64
typedef PIMAGE_NT_HEADERS64 PIMAGE_NT_HEADERS;
#else
typedef PIMAGE_NT_HEADERS32 PIMAGE_NT_HEADERS;
#endif

#define PsGetCurrentProcess IoGetCurrentProcess

#if (NTDDI_VERSION >= NTDDI_VISTA)
#if defined(_X86_) || defined(_AMD64_)
extern NTSYSAPI volatile CCHAR KeNumberProcessors;
#else
extern NTSYSAPI CCHAR KeNumberProcessors;
#endif
#elif (NTDDI_VERSION >= NTDDI_WINXP)
extern NTSYSAPI CCHAR KeNumberProcessors;
#else
extern PCCHAR KeNumberProcessors;
#endif

extern POBJECT_TYPE *PsProcessType;
extern POBJECT_TYPE *PsThreadType;
extern POBJECT_TYPE *PsJobType;
#if (NTDDI_VERSION >= NTDDI_WIN10_RS1)
extern POBJECT_TYPE *PsSiloContextPagedType;
extern POBJECT_TYPE *PsSiloContextNonPagedType;
#endif

#include <mce.h>

#ifndef FAR
#define FAR
#endif


#ifdef _X86_

//
// Some intrinsics have a redundant __cdecl calling-convention specifier when
// not compiled with /clr:pure.
//

#if defined(_M_CEE_PURE)

#define CDECL_NON_WVMPURE

#else

#define CDECL_NON_WVMPURE __cdecl

#endif


//
// Disable these two pragmas that evaluate to "sti" "cli" on x86 so that driver
// writers to not leave them inadvertantly in their code.
//

#if !defined(MIDL_PASS)
#if !defined(RC_INVOKED)

#if _MSC_VER >= 1200
#pragma warning(push)
#endif // _MSC_VER >= 1200
//#pragma warning(disable:4164)   // disable C4164 warning so that apps that
                                // build with /Od don't get weird errors !

#if _MSC_VER >= 1200
#pragma warning( pop )
#else
#pragma warning( default:4164 ) // reenable C4164 warning
#endif // _MSC_VER >= 1200

#endif // !defined(MIDL_PASS)
#endif // !defined(RC_INVOKED)



//
// Size of kernel mode stack.
//

#define KERNEL_STACK_SIZE 12288

//
// Define size of large kernel mode stack for callbacks.
//

#define KERNEL_LARGE_STACK_SIZE 61440

//
// Define number of pages to initialize in a large kernel stack.
//

#define KERNEL_LARGE_STACK_COMMIT 12288

#ifdef _X86_


#if !defined(MIDL_PASS) && defined(_M_IX86)

#if !defined(_M_CEE_PURE) && !defined(_M_HYBRID_X86_ARM64)

#pragma prefast(push)
#pragma warning(push)
#pragma prefast(disable: 6001 28113, "The barrier variable is accessed only to create a side effect.")
#pragma warning(disable: 4793)
FORCEINLINE
VOID
MemoryBarrier (
    VOID
    )
{
    LONG Barrier;

    InterlockedOr(&Barrier, 0);
    return;
}

#pragma warning(pop)
#pragma prefast(pop)

#endif /* !_M_CEE_PURE || !_M_HYBRID_X86_ARM64*/

#if !defined(_M_HYBRID_X86_ARM64)

//
// Define constants for use with _mm_prefetch.
//

#define _MM_HINT_T0     1
#define _MM_HINT_T1     2
#define _MM_HINT_T2     3
#define _MM_HINT_NTA    0

VOID
_mm_prefetch (
    _In_ CHAR CONST *a,
    _In_ int sel
    );

#pragma intrinsic(_mm_prefetch)

//
// PreFetchCacheLine level defines.
//

#define PF_TEMPORAL_LEVEL_1 _MM_HINT_T0
#define PF_TEMPORAL_LEVEL_2 _MM_HINT_T1
#define PF_TEMPORAL_LEVEL_3 _MM_HINT_T2
#define PF_NON_TEMPORAL_LEVEL_ALL _MM_HINT_NTA

#define PreFetchCacheLine(l, a)  _mm_prefetch((CHAR CONST *) a, l)
#define PrefetchForWrite(p)
#define ReadForWriteAccess(p) (*(p))

#if !defined(_MANAGED)

//
// Define function to read the value of a performance counter.
//

#define ReadPMC __readpmc

ULONG64
__readpmc (
    _In_ ULONG Counter
    );

#pragma intrinsic(__readpmc)

//
// Define function to read the value of the time stamp counter
//

#define ReadTimeStampCounter() __rdtsc()

ULONG64
__rdtsc (
    VOID
    );

#pragma intrinsic(__rdtsc)

#endif // !defined(_MANAGED)

#endif // !defined(_M_HYBRID_X86_ARM64)



#endif // !defined(MIDL_PASS) && defined(_M_IX86)



//
//  Define the size of the 80387 save area, which is in the context frame.
//

#define SIZE_OF_80387_REGISTERS      80

//
// The following flags control the contents of the CONTEXT structure.
//

#if !defined(RC_INVOKED)

#define CONTEXT_i386    0x00010000L    // this assumes that i386 and
#define CONTEXT_i486    0x00010000L    // i486 have identical context records



#define CONTEXT_CONTROL         (CONTEXT_i386 | 0x00000001L) // SS:SP, CS:IP, FLAGS, BP
#define CONTEXT_INTEGER         (CONTEXT_i386 | 0x00000002L) // AX, BX, CX, DX, SI, DI
#define CONTEXT_SEGMENTS        (CONTEXT_i386 | 0x00000004L) // DS, ES, FS, GS
#define CONTEXT_FLOATING_POINT  (CONTEXT_i386 | 0x00000008L) // 387 state
#define CONTEXT_DEBUG_REGISTERS (CONTEXT_i386 | 0x00000010L) // DB 0-3,6,7
#define CONTEXT_EXTENDED_REGISTERS  (CONTEXT_i386 | 0x00000020L) // cpu specific extensions

#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_INTEGER |\
                      CONTEXT_SEGMENTS)

#define CONTEXT_ALL             (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS | \
                                 CONTEXT_FLOATING_POINT | CONTEXT_DEBUG_REGISTERS | \
                                 CONTEXT_EXTENDED_REGISTERS)

#define CONTEXT_XSTATE          (CONTEXT_i386 | 0x00000040L)

#define CONTEXT_EXCEPTION_ACTIVE    0x08000000L
#define CONTEXT_SERVICE_ACTIVE      0x10000000L
#define CONTEXT_EXCEPTION_REQUEST   0x40000000L
#define CONTEXT_EXCEPTION_REPORTING 0x80000000L



#endif // !defined(RC_INVOKED)

typedef struct _FLOATING_SAVE_AREA {
    ULONG   ControlWord;
    ULONG   StatusWord;
    ULONG   TagWord;
    ULONG   ErrorOffset;
    ULONG   ErrorSelector;
    ULONG   DataOffset;
    ULONG   DataSelector;
    UCHAR   RegisterArea[SIZE_OF_80387_REGISTERS];
    ULONG   Spare0;
} FLOATING_SAVE_AREA;

typedef FLOATING_SAVE_AREA *PFLOATING_SAVE_AREA;



#include "pshpack4.h"

//
// Context Frame
//
//  This frame has a several purposes: 1) it is used as an argument to
//  NtContinue, 2) is is used to constuct a call frame for APC delivery,
//  and 3) it is used in the user level thread creation routines.
//
//  The layout of the record conforms to a standard call frame.
//

typedef struct _CONTEXT {

    //
    // The flags values within this flag control the contents of
    // a CONTEXT record.
    //
    // If the context record is used as an input parameter, then
    // for each portion of the context record controlled by a flag
    // whose value is set, it is assumed that that portion of the
    // context record contains valid context. If the context record
    // is being used to modify a threads context, then only that
    // portion of the threads context will be modified.
    //
    // If the context record is used as an IN OUT parameter to capture
    // the context of a thread, then only those portions of the thread's
    // context corresponding to set flags will be returned.
    //
    // The context record is never used as an OUT only parameter.
    //

    ULONG ContextFlags;

    //
    // This section is specified/returned if CONTEXT_DEBUG_REGISTERS is
    // set in ContextFlags.  Note that CONTEXT_DEBUG_REGISTERS is NOT
    // included in CONTEXT_FULL.
    //

    ULONG   Dr0;
    ULONG   Dr1;
    ULONG   Dr2;
    ULONG   Dr3;
    ULONG   Dr6;
    ULONG   Dr7;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_FLOATING_POINT.
    //

    FLOATING_SAVE_AREA FloatSave;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_SEGMENTS.
    //

    ULONG   SegGs;
    ULONG   SegFs;
    ULONG   SegEs;
    ULONG   SegDs;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_INTEGER.
    //

    ULONG   Edi;
    ULONG   Esi;
    ULONG   Ebx;
    ULONG   Edx;
    ULONG   Ecx;
    ULONG   Eax;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_CONTROL.
    //

    ULONG   Ebp;
    ULONG   Eip;
    ULONG   SegCs;              // MUST BE SANITIZED
    ULONG   EFlags;             // MUST BE SANITIZED
    ULONG   Esp;
    ULONG   SegSs;

    //
    // This section is specified/returned if the ContextFlags word
    // contains the flag CONTEXT_EXTENDED_REGISTERS.
    // The format and contexts are processor specific
    //

    UCHAR   ExtendedRegisters[MAXIMUM_SUPPORTED_EXTENSION];

} CONTEXT;

typedef CONTEXT *PCONTEXT;

#include "poppack.h"



#endif //_X86_


#endif // _X86_



#ifdef _AMD64_

//
// Size of kernel mode stack.
//

#define KERNEL_STACK_SIZE 0x6000

//
// Define size of large kernel mode stack for callbacks.
//

#define KERNEL_LARGE_STACK_SIZE 0x12000

//
// Define number of pages to initialize in a large kernel stack.
//

#define KERNEL_LARGE_STACK_COMMIT KERNEL_STACK_SIZE

//
// Define the size of the stack used for processing an MCA exception.
//

#define KERNEL_MCA_EXCEPTION_STACK_SIZE 0x2000

//
// The following values specify the type of access in the first parameter
// of the exception record whan the exception code specifies an access
// violation.
//

#define EXCEPTION_READ_FAULT 0          // exception caused by a read
#define EXCEPTION_WRITE_FAULT 1         // exception caused by a write
#define EXCEPTION_EXECUTE_FAULT 8       // exception caused by an instruction fetch


//
// The following flags control the contents of the CONTEXT structure.
//

#if !defined(RC_INVOKED)

#define CONTEXT_AMD64   0x00100000L



#define CONTEXT_CONTROL         (CONTEXT_AMD64 | 0x00000001L)
#define CONTEXT_INTEGER         (CONTEXT_AMD64 | 0x00000002L)
#define CONTEXT_SEGMENTS        (CONTEXT_AMD64 | 0x00000004L)
#define CONTEXT_FLOATING_POINT  (CONTEXT_AMD64 | 0x00000008L)
#define CONTEXT_DEBUG_REGISTERS (CONTEXT_AMD64 | 0x00000010L)

#define CONTEXT_FULL            (CONTEXT_CONTROL | CONTEXT_INTEGER | \
                                 CONTEXT_FLOATING_POINT)

#define CONTEXT_ALL             (CONTEXT_CONTROL | CONTEXT_INTEGER | \
                                 CONTEXT_SEGMENTS | CONTEXT_FLOATING_POINT | \
                                 CONTEXT_DEBUG_REGISTERS)

#define CONTEXT_XSTATE          (CONTEXT_AMD64 | 0x00000040L)

#if defined(XBOX_SYSTEMOS)

#define CONTEXT_KERNEL_DEBUGGER     0x04000000L

#endif

#define CONTEXT_EXCEPTION_ACTIVE    0x08000000L
#define CONTEXT_SERVICE_ACTIVE      0x10000000L
#define CONTEXT_EXCEPTION_REQUEST   0x40000000L
#define CONTEXT_EXCEPTION_REPORTING 0x80000000L



#endif // !defined(RC_INVOKED)

//
// Define initial MxCsr and FpCsr control.
//

#define INITIAL_MXCSR 0x1f80            // initial MXCSR value
#define INITIAL_FPCSR 0x027f            // initial FPCSR value


//
// Context Frame
//
//  This frame has a several purposes: 1) it is used as an argument to
//  NtContinue, 2) it is used to constuct a call frame for APC delivery,
//  and 3) it is used in the user level thread creation routines.
//
//
// The flags field within this record controls the contents of a CONTEXT
// record.
//
// If the context record is used as an input parameter, then for each
// portion of the context record controlled by a flag whose value is
// set, it is assumed that that portion of the context record contains
// valid context. If the context record is being used to modify a threads
// context, then only that portion of the threads context is modified.
//
// If the context record is used as an output parameter to capture the
// context of a thread, then only those portions of the thread's context
// corresponding to set flags will be returned.
//
// CONTEXT_CONTROL specifies SegSs, Rsp, SegCs, Rip, and EFlags.
//
// CONTEXT_INTEGER specifies Rax, Rcx, Rdx, Rbx, Rbp, Rsi, Rdi, and R8-R15.
//
// CONTEXT_SEGMENTS specifies SegDs, SegEs, SegFs, and SegGs.
//
// CONTEXT_FLOATING_POINT specifies Xmm0-Xmm15.
//
// CONTEXT_DEBUG_REGISTERS specifies Dr0-Dr3 and Dr6-Dr7.
//

typedef struct DECLSPEC_ALIGN(16) _CONTEXT {

    //
    // Register parameter home addresses.
    //
    // N.B. These fields are for convience - they could be used to extend the
    //      context record in the future.
    //

    ULONG64 P1Home;
    ULONG64 P2Home;
    ULONG64 P3Home;
    ULONG64 P4Home;
    ULONG64 P5Home;
    ULONG64 P6Home;

    //
    // Control flags.
    //

    ULONG ContextFlags;
    ULONG MxCsr;

    //
    // Segment Registers and processor flags.
    //

    USHORT SegCs;
    USHORT SegDs;
    USHORT SegEs;
    USHORT SegFs;
    USHORT SegGs;
    USHORT SegSs;
    ULONG EFlags;

    //
    // Debug registers
    //

    ULONG64 Dr0;
    ULONG64 Dr1;
    ULONG64 Dr2;
    ULONG64 Dr3;
    ULONG64 Dr6;
    ULONG64 Dr7;

    //
    // Integer registers.
    //

    ULONG64 Rax;
    ULONG64 Rcx;
    ULONG64 Rdx;
    ULONG64 Rbx;
    ULONG64 Rsp;
    ULONG64 Rbp;
    ULONG64 Rsi;
    ULONG64 Rdi;
    ULONG64 R8;
    ULONG64 R9;
    ULONG64 R10;
    ULONG64 R11;
    ULONG64 R12;
    ULONG64 R13;
    ULONG64 R14;
    ULONG64 R15;

    //
    // Program counter.
    //

    ULONG64 Rip;

    //
    // Floating point state.
    //

    union {
        XMM_SAVE_AREA32 FltSave;
        struct {
            M128A Header[2];
            M128A Legacy[8];
            M128A Xmm0;
            M128A Xmm1;
            M128A Xmm2;
            M128A Xmm3;
            M128A Xmm4;
            M128A Xmm5;
            M128A Xmm6;
            M128A Xmm7;
            M128A Xmm8;
            M128A Xmm9;
            M128A Xmm10;
            M128A Xmm11;
            M128A Xmm12;
            M128A Xmm13;
            M128A Xmm14;
            M128A Xmm15;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;

    //
    // Vector registers.
    //

    M128A VectorRegister[26];
    ULONG64 VectorControl;

    //
    // Special debug control registers.
    //

    ULONG64 DebugControl;
    ULONG64 LastBranchToRip;
    ULONG64 LastBranchFromRip;
    ULONG64 LastExceptionToRip;
    ULONG64 LastExceptionFromRip;
} CONTEXT, *PCONTEXT;





#endif // _AMD64_




#ifdef _ARM_

//
// Size of kernel mode stack.
//

#define KERNEL_STACK_SIZE         0x3000

//
// Define size of large kernel mode stack for callbacks.
//

#define KERNEL_LARGE_STACK_SIZE   0xF000

//
// Define number of pages to initialize in a large kernel stack.
//

#define KERNEL_LARGE_STACK_COMMIT KERNEL_STACK_SIZE

//
// Define the size of the stack used for processing an MCA exception.
//

#define KERNEL_MCA_EXCEPTION_STACK_SIZE 0x2000

//
// The following values specify the type of access in the first parameter
// of the exception record whan the exception code specifies an access
// violation.
//

#define EXCEPTION_READ_FAULT 0          // exception caused by a read
#define EXCEPTION_WRITE_FAULT 1         // exception caused by a write
#define EXCEPTION_EXECUTE_FAULT 8       // exception caused by an instruction fetch


//
// The following flags control the contents of the CONTEXT structure.
//

#if !defined(RC_INVOKED)

#define CONTEXT_ARM   0x00200000L



#define CONTEXT_CONTROL (CONTEXT_ARM | 0x1L)
#define CONTEXT_INTEGER (CONTEXT_ARM | 0x2L)
#define CONTEXT_FLOATING_POINT  (CONTEXT_ARM | 0x4L)
#define CONTEXT_DEBUG_REGISTERS (CONTEXT_ARM | 0x8L)

#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_FLOATING_POINT)

#define CONTEXT_ALL (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_FLOATING_POINT | CONTEXT_DEBUG_REGISTERS)

#define CONTEXT_EXCEPTION_ACTIVE 0x8000000L
#define CONTEXT_SERVICE_ACTIVE 0x10000000L
#define CONTEXT_EXCEPTION_REQUEST 0x40000000L
#define CONTEXT_EXCEPTION_REPORTING 0x80000000L

//
// This flag is set by the unwinder if it has unwound to a call
// site, and cleared whenever it unwinds through a trap frame.
// It is used by language-specific exception handlers to help
// differentiate exception scopes during dispatching.
//

#define CONTEXT_UNWOUND_TO_CALL 0x20000000



#endif // !defined(RC_INVOKED)

//
// Define initial Cpsr/Fpscr value
//

#define INITIAL_CPSR 0x10
#define INITIAL_FPSCR 0

//
// Specify the number of breakpoints and watchpoints that the OS
// will track. Architecturally, ARM supports up to 16. In practice,
// however, almost no one implements more than 4 of each.
//

#define ARM_MAX_BREAKPOINTS     8
#define ARM_MAX_WATCHPOINTS     1

//
// Context Frame
//
//  This frame has a several purposes: 1) it is used as an argument to
//  NtContinue, 2) it is used to constuct a call frame for APC delivery,
//  and 3) it is used in the user level thread creation routines.
//
//
// The flags field within this record controls the contents of a CONTEXT
// record.
//
// If the context record is used as an input parameter, then for each
// portion of the context record controlled by a flag whose value is
// set, it is assumed that that portion of the context record contains
// valid context. If the context record is being used to modify a threads
// context, then only that portion of the threads context is modified.
//
// If the context record is used as an output parameter to capture the
// context of a thread, then only those portions of the thread's context
// corresponding to set flags will be returned.
//
// CONTEXT_CONTROL specifies Sp, Lr, Pc, and Cpsr
//
// CONTEXT_INTEGER specifies R0-R12
//
// CONTEXT_FLOATING_POINT specifies Q0-Q15 / D0-D31 / S0-S31
//
// CONTEXT_DEBUG_REGISTERS specifies up to 16 of DBGBVR, DBGBCR, DBGWVR,
//      DBGWCR.
//

typedef struct _NEON128 {
    ULONGLONG Low;
    LONGLONG High;
} NEON128, *PNEON128;

typedef struct DECLSPEC_ALIGN(8) _CONTEXT {

    //
    // Control flags.
    //

    ULONG ContextFlags;

    //
    // Integer registers
    //

    ULONG R0;
    ULONG R1;
    ULONG R2;
    ULONG R3;
    ULONG R4;
    ULONG R5;
    ULONG R6;
    ULONG R7;
    ULONG R8;
    ULONG R9;
    ULONG R10;
    ULONG R11;
    ULONG R12;

    //
    // Control Registers
    //

    ULONG Sp;
    ULONG Lr;
    ULONG Pc;
    ULONG Cpsr;

    //
    // Floating Point/NEON Registers
    //

    ULONG Fpscr;
    ULONG Padding;
    union {
        NEON128 Q[16];
        ULONGLONG D[32];
        ULONG S[32];
    } DUMMYUNIONNAME;

    //
    // Debug registers
    //

    ULONG Bvr[ARM_MAX_BREAKPOINTS];
    ULONG Bcr[ARM_MAX_BREAKPOINTS];
    ULONG Wvr[ARM_MAX_WATCHPOINTS];
    ULONG Wcr[ARM_MAX_WATCHPOINTS];

    ULONG Padding2[2];

} CONTEXT, *PCONTEXT;


#endif // _ARM_


#if defined(_ARM64_) || defined(_CHPE_X86_ARM64_)

#if defined(_ARM64_)
//
// Size of kernel mode stack.
//
// ARM64_WORKITEM: temporally bump up the kernel stack size until optimized build is ready.

#define KERNEL_STACK_SIZE         0x8000

//
// Define size of large kernel mode stack for callbacks.
//

#define KERNEL_LARGE_STACK_SIZE   0x12000

//
// Define number of pages to initialize in a large kernel stack.
//

#define KERNEL_LARGE_STACK_COMMIT KERNEL_STACK_SIZE

//
// Define the size of the stack used for processing an MCA exception.
//

#define KERNEL_MCA_EXCEPTION_STACK_SIZE 0x2000

#endif

//
// The following values specify the type of access in the first parameter
// of the exception record whan the exception code specifies an access
// violation.
//

#define EXCEPTION_READ_FAULT 0          // exception caused by a read
#define EXCEPTION_WRITE_FAULT 1         // exception caused by a write
#define EXCEPTION_EXECUTE_FAULT 8       // exception caused by an instruction fetch



//
// Define initial Cpsr/Fpscr value
//

#define INITIAL_CPSR 0x10
#define INITIAL_FPSCR 0



#endif // defined(_ARM64_) || defined(_CHPE_X86_ARM64_)



//
// The following flags control the contents of the CONTEXT structure.
//

#if !defined(RC_INVOKED)

#define CONTEXT_ARM64   0x00400000L



#define CONTEXT_ARM64_CONTROL (CONTEXT_ARM64 | 0x1L)
#define CONTEXT_ARM64_INTEGER (CONTEXT_ARM64 | 0x2L)
#define CONTEXT_ARM64_FLOATING_POINT  (CONTEXT_ARM64 | 0x4L)
#define CONTEXT_ARM64_DEBUG_REGISTERS (CONTEXT_ARM64 | 0x8L)
#define CONTEXT_ARM64_X18 (CONTEXT_ARM64 | 0x10L)

//
// CONTEXT_ARM64_X18 is not part of CONTEXT_ARM64_FULL because in NT user-mode
// threads, x18 contains a pointer to the TEB and should generally not be set
// without intending to.
//

#define CONTEXT_ARM64_FULL (CONTEXT_ARM64_CONTROL | CONTEXT_ARM64_INTEGER | CONTEXT_ARM64_FLOATING_POINT)
#define CONTEXT_ARM64_ALL  (CONTEXT_ARM64_CONTROL | CONTEXT_ARM64_INTEGER | CONTEXT_ARM64_FLOATING_POINT | CONTEXT_ARM64_DEBUG_REGISTERS | CONTEXT_ARM64_X18)

#if defined(_ARM64_)

#define CONTEXT_CONTROL CONTEXT_ARM64_CONTROL
#define CONTEXT_INTEGER CONTEXT_ARM64_INTEGER
#define CONTEXT_FLOATING_POINT CONTEXT_ARM64_FLOATING_POINT
#define CONTEXT_DEBUG_REGISTERS CONTEXT_ARM64_DEBUG_REGISTERS
#define CONTEXT_FULL CONTEXT_ARM64_FULL
#define CONTEXT_ALL CONTEXT_ARM64_ALL

#define CONTEXT_EXCEPTION_ACTIVE    0x08000000L
#define CONTEXT_SERVICE_ACTIVE      0x10000000L
#define CONTEXT_EXCEPTION_REQUEST   0x40000000L
#define CONTEXT_EXCEPTION_REPORTING 0x80000000L

#endif

#if defined(_ARM64_) || defined(_CHPE_X86_ARM64_) || defined(_X86_)

//
// This flag is set by the unwinder if it has unwound to a call
// site, and cleared whenever it unwinds through a trap frame.
// It is used by language-specific exception handlers to help
// differentiate exception scopes during dispatching.
//

#define CONTEXT_UNWOUND_TO_CALL 0x20000000
#define CONTEXT_RET_TO_GUEST    0x04000000

#endif



#endif // !defined(RC_INVOKED)



//
// Specify the number of breakpoints and watchpoints that the OS
// will track. Architecturally, ARM64 supports up to 16. In practice,
// however, almost no one implements more than 4 of each.
//

#define ARM64_MAX_BREAKPOINTS     8
#define ARM64_MAX_WATCHPOINTS     2

//
// Context Frame
//
//  This frame has a several purposes: 1) it is used as an argument to
//  NtContinue, 2) it is used to constuct a call frame for APC delivery,
//  and 3) it is used in the user level thread creation routines.
//
//
// The flags field within this record controls the contents of a CONTEXT
// record.
//
// If the context record is used as an input parameter, then for each
// portion of the context record controlled by a flag whose value is
// set, it is assumed that that portion of the context record contains
// valid context. If the context record is being used to modify a threads
// context, then only that portion of the threads context is modified.
//
// If the context record is used as an output parameter to capture the
// context of a thread, then only those portions of the thread's context
// corresponding to set flags will be returned.
//
// CONTEXT_CONTROL specifies FP, LR, SP, PC, and CPSR
//
// CONTEXT_INTEGER specifies X0-X28
//
// CONTEXT_FLOATING_POINT specifies Fpcr, Fpsr and Q0-Q31 / D0-D31 / S0-S31
//
// CONTEXT_DEBUG_REGISTERS specifies up to 16 of DBGBVR, DBGBCR, DBGWVR,
//      DBGWCR.
//

typedef union _ARM64_NT_NEON128 {
    struct {
        ULONGLONG Low;
        LONGLONG High;
    } DUMMYSTRUCTNAME;
    double D[2];
    float S[4];
    USHORT H[8];
    UCHAR B[16];
} ARM64_NT_NEON128, *PARM64_NT_NEON128;

#if defined(_ARM64_)

typedef ARM64_NT_NEON128 NEON128, *PNEON128;

#endif

#if defined(_ARM64_)

#pragma push_macro("_ARM64_NT_CONTEXT")
#undef _ARM64_NT_CONTEXT
#define _ARM64_NT_CONTEXT _CONTEXT

#pragma push_macro("ARM64_NT_NEON128")
#undef ARM64_NT_NEON128
#define ARM64_NT_NEON128 NEON128

#endif

typedef struct DECLSPEC_ALIGN(16) _ARM64_NT_CONTEXT {

    //
    // Control flags.
    //

    /* +0x000 */ ULONG ContextFlags;

    //
    // Integer registers
    //

    /* +0x004 */ ULONG Cpsr;       // NZVF + DAIF + CurrentEL + SPSel
    /* +0x008 */ union {
                    struct {
                        ULONG64 X0;
                        ULONG64 X1;
                        ULONG64 X2;
                        ULONG64 X3;
                        ULONG64 X4;
                        ULONG64 X5;
                        ULONG64 X6;
                        ULONG64 X7;
                        ULONG64 X8;
                        ULONG64 X9;
                        ULONG64 X10;
                        ULONG64 X11;
                        ULONG64 X12;
                        ULONG64 X13;
                        ULONG64 X14;
                        ULONG64 X15;
                        ULONG64 X16;
                        ULONG64 X17;
                        ULONG64 X18;
                        ULONG64 X19;
                        ULONG64 X20;
                        ULONG64 X21;
                        ULONG64 X22;
                        ULONG64 X23;
                        ULONG64 X24;
                        ULONG64 X25;
                        ULONG64 X26;
                        ULONG64 X27;
                        ULONG64 X28;
    /* +0x0f0 */        ULONG64 Fp;
    /* +0x0f8 */        ULONG64 Lr;
                    } DUMMYSTRUCTNAME;
                    ULONG64 X[31];
                 } DUMMYUNIONNAME;
    /* +0x100 */ ULONG64 Sp;
    /* +0x108 */ ULONG64 Pc;

    //
    // Floating Point/NEON Registers
    //

    /* +0x110 */ ARM64_NT_NEON128 V[32];
    /* +0x310 */ ULONG Fpcr;
    /* +0x314 */ ULONG Fpsr;

    //
    // Debug registers
    //

    /* +0x318 */ ULONG Bcr[ARM64_MAX_BREAKPOINTS];
    /* +0x338 */ ULONG64 Bvr[ARM64_MAX_BREAKPOINTS];
    /* +0x378 */ ULONG Wcr[ARM64_MAX_WATCHPOINTS];
    /* +0x380 */ ULONG64 Wvr[ARM64_MAX_WATCHPOINTS];
    /* +0x390 */

} ARM64_NT_CONTEXT, *PARM64_NT_CONTEXT;

#if defined(_ARM64_)

#undef ARM64_NT_NEON128
#pragma pop_macro("ARM64_NT_NEON128")

#undef _ARM64_NT_CONTEXT
#pragma pop_macro("_ARM64_NT_CONTEXT")

typedef ARM64_NT_CONTEXT CONTEXT, *PCONTEXT;

#endif





//
// Well known SID definitions for lookup.
//

typedef enum {

    WinNullSid                                  = 0,
    WinWorldSid                                 = 1,
    WinLocalSid                                 = 2,
    WinCreatorOwnerSid                          = 3,
    WinCreatorGroupSid                          = 4,
    WinCreatorOwnerServerSid                    = 5,
    WinCreatorGroupServerSid                    = 6,
    WinNtAuthoritySid                           = 7,
    WinDialupSid                                = 8,
    WinNetworkSid                               = 9,
    WinBatchSid                                 = 10,
    WinInteractiveSid                           = 11,
    WinServiceSid                               = 12,
    WinAnonymousSid                             = 13,
    WinProxySid                                 = 14,
    WinEnterpriseControllersSid                 = 15,
    WinSelfSid                                  = 16,
    WinAuthenticatedUserSid                     = 17,
    WinRestrictedCodeSid                        = 18,
    WinTerminalServerSid                        = 19,
    WinRemoteLogonIdSid                         = 20,
    WinLogonIdsSid                              = 21,
    WinLocalSystemSid                           = 22,
    WinLocalServiceSid                          = 23,
    WinNetworkServiceSid                        = 24,
    WinBuiltinDomainSid                         = 25,
    WinBuiltinAdministratorsSid                 = 26,
    WinBuiltinUsersSid                          = 27,
    WinBuiltinGuestsSid                         = 28,
    WinBuiltinPowerUsersSid                     = 29,
    WinBuiltinAccountOperatorsSid               = 30,
    WinBuiltinSystemOperatorsSid                = 31,
    WinBuiltinPrintOperatorsSid                 = 32,
    WinBuiltinBackupOperatorsSid                = 33,
    WinBuiltinReplicatorSid                     = 34,
    WinBuiltinPreWindows2000CompatibleAccessSid = 35,
    WinBuiltinRemoteDesktopUsersSid             = 36,
    WinBuiltinNetworkConfigurationOperatorsSid  = 37,
    WinAccountAdministratorSid                  = 38,
    WinAccountGuestSid                          = 39,
    WinAccountKrbtgtSid                         = 40,
    WinAccountDomainAdminsSid                   = 41,
    WinAccountDomainUsersSid                    = 42,
    WinAccountDomainGuestsSid                   = 43,
    WinAccountComputersSid                      = 44,
    WinAccountControllersSid                    = 45,
    WinAccountCertAdminsSid                     = 46,
    WinAccountSchemaAdminsSid                   = 47,
    WinAccountEnterpriseAdminsSid               = 48,
    WinAccountPolicyAdminsSid                   = 49,
    WinAccountRasAndIasServersSid               = 50,
    WinNTLMAuthenticationSid                    = 51,
    WinDigestAuthenticationSid                  = 52,
    WinSChannelAuthenticationSid                = 53,
    WinThisOrganizationSid                      = 54,
    WinOtherOrganizationSid                     = 55,
    WinBuiltinIncomingForestTrustBuildersSid    = 56,
    WinBuiltinPerfMonitoringUsersSid            = 57,
    WinBuiltinPerfLoggingUsersSid               = 58,
    WinBuiltinAuthorizationAccessSid            = 59,
    WinBuiltinTerminalServerLicenseServersSid   = 60,
    WinBuiltinDCOMUsersSid                      = 61,
    WinBuiltinIUsersSid                         = 62,
    WinIUserSid                                 = 63,
    WinBuiltinCryptoOperatorsSid                = 64,
    WinUntrustedLabelSid                        = 65,
    WinLowLabelSid                              = 66,
    WinMediumLabelSid                           = 67,
    WinHighLabelSid                             = 68,
    WinSystemLabelSid                           = 69,
    WinWriteRestrictedCodeSid                   = 70,
    WinCreatorOwnerRightsSid                    = 71,
    WinCacheablePrincipalsGroupSid              = 72,
    WinNonCacheablePrincipalsGroupSid           = 73,
    WinEnterpriseReadonlyControllersSid         = 74,
    WinAccountReadonlyControllersSid            = 75,
    WinBuiltinEventLogReadersGroup              = 76,
    WinNewEnterpriseReadonlyControllersSid      = 77,
    WinBuiltinCertSvcDComAccessGroup            = 78,
    WinMediumPlusLabelSid                       = 79,
    WinLocalLogonSid                            = 80,
    WinConsoleLogonSid                          = 81,
    WinThisOrganizationCertificateSid           = 82,
    WinApplicationPackageAuthoritySid           = 83,
    WinBuiltinAnyPackageSid                     = 84,
    WinCapabilityInternetClientSid              = 85,
    WinCapabilityInternetClientServerSid        = 86,
    WinCapabilityPrivateNetworkClientServerSid  = 87,
    WinCapabilityPicturesLibrarySid             = 88,
    WinCapabilityVideosLibrarySid               = 89,
    WinCapabilityMusicLibrarySid                = 90,
    WinCapabilityDocumentsLibrarySid            = 91,
    WinCapabilitySharedUserCertificatesSid      = 92,
    WinCapabilityEnterpriseAuthenticationSid    = 93,
    WinCapabilityRemovableStorageSid            = 94,
    WinBuiltinRDSRemoteAccessServersSid         = 95,
    WinBuiltinRDSEndpointServersSid             = 96,
    WinBuiltinRDSManagementServersSid           = 97,
    WinUserModeDriversSid                       = 98,
    WinBuiltinHyperVAdminsSid                   = 99,
    WinAccountCloneableControllersSid           = 100,
    WinBuiltinAccessControlAssistanceOperatorsSid = 101,
    WinBuiltinRemoteManagementUsersSid          = 102,
    WinAuthenticationAuthorityAssertedSid       = 103,
    WinAuthenticationServiceAssertedSid         = 104,
    WinLocalAccountSid                          = 105,
    WinLocalAccountAndAdministratorSid          = 106,
    WinAccountProtectedUsersSid                 = 107,
    WinCapabilityAppointmentsSid                = 108,
    WinCapabilityContactsSid                    = 109,
    WinAccountDefaultSystemManagedSid           = 110,
    WinBuiltinDefaultSystemManagedGroupSid      = 111,
    WinBuiltinStorageReplicaAdminsSid           = 112,
    WinAccountKeyAdminsSid                      = 113,
    WinAccountEnterpriseKeyAdminsSid            = 114,
    WinAuthenticationKeyTrustSid                = 115,
    WinAuthenticationKeyPropertyMFASid          = 116,
    WinAuthenticationKeyPropertyAttestationSid  = 117,
    WinAuthenticationFreshKeyAuthSid            = 118,
    WinBuiltinDeviceOwnersSid                   = 119,
} WELL_KNOWN_SID_TYPE;

//
// Unsolicited Input is obsolete and unused.
//

#define SE_UNSOLICITED_INPUT_PRIVILEGE      (6L)


//
// Base signing levels.
//

typedef UCHAR SE_SIGNING_LEVEL, *PSE_SIGNING_LEVEL;

#define SE_SIGNING_LEVEL_UNCHECKED         0x00000000
#define SE_SIGNING_LEVEL_UNSIGNED          0x00000001
#define SE_SIGNING_LEVEL_ENTERPRISE        0x00000002
#define SE_SIGNING_LEVEL_CUSTOM_1          0x00000003
#define SE_SIGNING_LEVEL_AUTHENTICODE      0x00000004
#define SE_SIGNING_LEVEL_CUSTOM_2          0x00000005
#define SE_SIGNING_LEVEL_STORE             0x00000006
#define SE_SIGNING_LEVEL_CUSTOM_3          0x00000007
#define SE_SIGNING_LEVEL_ANTIMALWARE       SE_SIGNING_LEVEL_CUSTOM_3
#define SE_SIGNING_LEVEL_MICROSOFT         0x00000008
#define SE_SIGNING_LEVEL_CUSTOM_4          0x00000009
#define SE_SIGNING_LEVEL_CUSTOM_5          0x0000000A
#define SE_SIGNING_LEVEL_DYNAMIC_CODEGEN   0x0000000B
#define SE_SIGNING_LEVEL_WINDOWS           0x0000000C
#define SE_SIGNING_LEVEL_CUSTOM_7          0x0000000D
#define SE_SIGNING_LEVEL_WINDOWS_TCB       0x0000000E
#define SE_SIGNING_LEVEL_CUSTOM_6          0x0000000F

//
// Image signature types.
//

typedef enum _SE_IMAGE_SIGNATURE_TYPE
{
    SeImageSignatureNone = 0,
    SeImageSignatureEmbedded,
    SeImageSignatureCache,
    SeImageSignatureCatalogCached,
    SeImageSignatureCatalogNotCached,
    SeImageSignatureCatalogHint,
    SeImageSignaturePackageCatalog,
} SE_IMAGE_SIGNATURE_TYPE, *PSE_IMAGE_SIGNATURE_TYPE;


#ifndef _NTLSA_IFS_


//
// All of this stuff (between the Ifndef _NTLSA_AUDIT_ and its endif) were not
// present in NTIFS prior to Windows Server 2003 SP1. All of the definitions however
// exist down to windows 2000 (except for the few exceptions noted in the code).
//

#ifndef _NTLSA_AUDIT_
#define _NTLSA_AUDIT_

/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Data types related to Auditing                                      //
//                                                                     //
/////////////////////////////////////////////////////////////////////////


//
// The following enumerated type is used between the reference monitor and
// LSA in the generation of audit messages.  It is used to indicate the
// type of data being passed as a parameter from the reference monitor
// to LSA.  LSA is responsible for transforming the specified data type
// into a set of unicode strings that are added to the event record in
// the audit log.
//

typedef enum _SE_ADT_PARAMETER_TYPE {

    SeAdtParmTypeNone = 0,          //Produces 1 parameter
                                    //Received value:
                                    //
                                    //  None.
                                    //
                                    //Results in:
                                    //
                                    //  a unicode string containing "-".
                                    //
                                    //Note:  This is typically used to
                                    //       indicate that a parameter value
                                    //       was not available.
                                    //

    SeAdtParmTypeString,            //Produces 1 parameter.
                                    //Received Value:
                                    //
                                    //  Unicode String (variable length)
                                    //
                                    //Results in:
                                    //
                                    //  No transformation.  The string
                                    //  entered into the event record as
                                    //  received.
                                    //
                                    // The Address value of the audit info
                                    // should be a pointer to a UNICODE_STRING
                                    // structure.



    SeAdtParmTypeFileSpec,          //Produces 1 parameter.
                                    //Received value:
                                    //
                                    //  Unicode string containing a file or
                                    //  directory name.
                                    //
                                    //Results in:
                                    //
                                    //  Unicode string with the prefix of the
                                    //  file's path replaced by a drive letter
                                    //  if possible.
                                    //




    SeAdtParmTypeUlong,             //Produces 1 parameter
                                    //Received value:
                                    //
                                    //  Ulong
                                    //
                                    //Results in:
                                    //
                                    //  Unicode string representation of
                                    //  unsigned integer value.


    SeAdtParmTypeSid,               //Produces 1 parameter.
                                    //Received value:
                                    //
                                    //  SID (variable length)
                                    //
                                    //Results in:
                                    //
                                    //  String representation of SID
                                    //




    SeAdtParmTypeLogonId,           //Produces 4 parameters.
                                    //Received Value:
                                    //
                                    //  LUID (fixed length)
                                    //
                                    //Results in:
                                    //
                                    //  param 1: Sid string
                                    //  param 2: Username string
                                    //  param 3: domain name string
                                    //  param 4: Logon ID (Luid) string


    SeAdtParmTypeNoLogonId,         //Produces 3 parameters.
                                    //Received value:
                                    //
                                    //  None.
                                    //
                                    //Results in:
                                    //
                                    //  param 1: "-"
                                    //  param 2: "-"
                                    //  param 3: "-"
                                    //  param 4: "-"
                                    //
                                    //Note:
                                    //
                                    //  This type is used when a logon ID
                                    //  is needed, but one is not available
                                    //  to pass.  For example, if an
                                    //  impersonation logon ID is expected
                                    //  but the subject is not impersonating
                                    //  anyone.
                                    //

    SeAdtParmTypeAccessMask,        //Produces 1 parameter with formatting.
                                    //Received value:
                                    //
                                    //  ACCESS_MASK followed by
                                    //  a Unicode string.  The unicode
                                    //  string contains the name of the
                                    //  type of object the access mask
                                    //  applies to.  The event's source
                                    //  further qualifies the object type.
                                    //
                                    //Results in:
                                    //
                                    //  formatted unicode string built to
                                    //  take advantage of the specified
                                    //  source's parameter message file.
                                    //
                                    //Note:
                                    //
                                    //  An access mask containing three
                                    //  access types for a Widget object
                                    //  type (defined by the Foozle source)
                                    //  might end up looking like:
                                    //
                                    //      %%1062\n\t\t%1066\n\t\t%%601
                                    //
                                    //  The %%numbers are signals to the
                                    //  event viewer to perform parameter
                                    //  substitution before display.
                                    //



    SeAdtParmTypePrivs,             //Produces 1 parameter with formatting.
                                    //Received value:
                                    //
                                    //Results in:
                                    //
                                    //  formatted unicode string similar to
                                    //  that for access types.  Each priv
                                    //  will be formatted to be displayed
                                    //  on its own line.  E.g.,
                                    //
                                    //      %%642\n\t\t%%651\n\t\t%%655
                                    //

    SeAdtParmTypeObjectTypes,       //Produces 10 parameters with formatting.
                                    //Received value:
                                    //
                                    // Produces a list a stringized GUIDS along
                                    // with information similar to that for
                                    // an access mask.

    SeAdtParmTypeHexUlong,          //Produces 1 parameter
                                    //Received value:
                                    //
                                    //  Ulong
                                    //
                                    //Results in:
                                    //
                                    //  Unicode string representation of
                                    //  unsigned integer value in hexadecimal.

// In W2k this value did not exist, it was ParmTypeLUID

    SeAdtParmTypePtr,               //Produces 1 parameter
                                    //Received value:
                                    //
                                    //  pointer
                                    //
                                    //Results in:
                                    //
                                    //  Unicode string representation of
                                    //  unsigned integer value in hexadecimal.

//
// Everything below exists only in Windows XP and greater
//

    SeAdtParmTypeTime,              //Produces 2 parameters
                                    //Received value:
                                    //
                                    //  LARGE_INTEGER
                                    //
                                    //Results in:
                                    //
                                    // Unicode string representation of
                                    // date and time.

                                    //
    SeAdtParmTypeGuid,              //Produces 1 parameter
                                    //Received value:
                                    //
                                    //  GUID pointer
                                    //
                                    //Results in:
                                    //
                                    // Unicode string representation of GUID
                                    // {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
                                    //

//
// Everything below exists only in Windows Server 2003 and Greater
//

    SeAdtParmTypeLuid,              //
                                    //Produces 1 parameter
                                    //Received value:
                                    //
                                    // LUID
                                    //
                                    //Results in:
                                    //
                                    // Hex LUID
                                    //

    SeAdtParmTypeHexInt64,          //Produces 1 parameter
                                    //Received value:
                                    //
                                    //  64 bit integer
                                    //
                                    //Results in:
                                    //
                                    //  Unicode string representation of
                                    //  unsigned integer value in hexadecimal.

    SeAdtParmTypeStringList,        //Produces 1 parameter
                                    //Received value:
                                    //
                                    // ptr to LSAP_ADT_STRING_LIST
                                    //
                                    //Results in:
                                    //
                                    // Unicode string representation of
                                    // concatenation of the strings in the list

    SeAdtParmTypeSidList,           //Produces 1 parameter
                                    //Received value:
                                    //
                                    // ptr to LSAP_ADT_SID_LIST
                                    //
                                    //Results in:
                                    //
                                    // Unicode string representation of
                                    // concatenation of the SIDs in the list

    SeAdtParmTypeDuration,          //Produces 1 parameters
                                    //Received value:
                                    //
                                    //  LARGE_INTEGER
                                    //
                                    //Results in:
                                    //
                                    // Unicode string representation of
                                    // a duration.

    SeAdtParmTypeUserAccountControl,//Produces 3 parameters
                                    //Received value:
                                    //
                                    // old and new UserAccountControl values
                                    //
                                    //Results in:
                                    //
                                    // Unicode string representations of
                                    // the flags in UserAccountControl.
                                    // 1 - old value in hex
                                    // 2 - new value in hex
                                    // 3 - difference as strings

    SeAdtParmTypeNoUac,             //Produces 3 parameters
                                    //Received value:
                                    //
                                    // none
                                    //
                                    //Results in:
                                    //
                                    // Three dashes ('-') as unicode strings.

    SeAdtParmTypeMessage,           //Produces 1 Parameter
                                    //Received value:
                                    //
                                    //  ULONG (MessageNo from msobjs.mc)
                                    //
                                    //Results in:
                                    //
                                    // Unicode string representation of
                                    // %%MessageNo which the event viewer
                                    // will replace with the message string
                                    // from msobjs.mc

    SeAdtParmTypeDateTime,          //Produces 1 Parameter
                                    //Received value:
                                    //
                                    //  LARGE_INTEGER
                                    //
                                    //Results in:
                                    //
                                    // Unicode string representation of
                                    // date and time (in _one_ string).

    SeAdtParmTypeSockAddr,          // Produces 2 parameters
                                    //
                                    // Received value:
                                    //
                                    // pointer to SOCKADDR_IN/SOCKADDR_IN6
                                    // structure
                                    //
                                    // Results in:
                                    //
                                    // param 1: IP address string
                                    // param 2: Port number string
                                    //

//
// Everything below this exists only in Windows Server 2008 and greater
//

    SeAdtParmTypeSD,                // Produces 1 parameters
                                    //
                                    // Received value:
                                    //
                                    // pointer to SECURITY_DESCRIPTOR
                                    // structure. This HAS to appear in pairs.
                                    // The first parameter will represent the
                                    // old SD and the second parameter will
                                    // represent the New SD
                                    //
                                    // Results in:
                                    //
                                    // SDDL string representation of SD
                                    //

    SeAdtParmTypeLogonHours,        // Produces 1 parameters
                                    //
                                    // Received value:
                                    //
                                    // pointer to LOGON_HOURS
                                    // structure
                                    //
                                    // Results in:
                                    //
                                    // String representation of allowed logon hours
                                    //

    SeAdtParmTypeLogonIdNoSid,      //Produces 3 parameters.
                                    //Received Value:
                                    //
                                    //  LUID (fixed length)
                                    //
                                    //Results in:
                                    //
                                    //  param 1: Username string
                                    //  param 2: domain name string
                                    //  param 3: Logon ID (Luid) string

    SeAdtParmTypeUlongNoConv,       // Produces 1 parameter.
                                    // Received Value:
                                    // Ulong
                                    //
                                    //Results in:
                                    // Not converted to string
                                    //

    SeAdtParmTypeSockAddrNoPort,    // Produces 1 parameter
                                    //
                                    // Received value:
                                    //
                                    // pointer to SOCKADDR_IN/SOCKADDR_IN6
                                    // structure
                                    //
                                    // Results in:
                                    //
                                    // param 1: IPv4/IPv6 address string
                                    //
//
// Everything below this exists only in Windows Server 2008 and greater
//

    SeAdtParmTypeAccessReason,      // Produces 1 parameters
                                    //
                                    // Received value:
                                    //
                                    // pointer to SE_ADT_ACCESS_REASON structure
                                    //
                                    // Results in:
                                    //
                                    // String representation of the access reason.
                                    //
//
// Everything below this exists only in Windows Server 2012 and greater
//

    SeAdtParmTypeStagingReason,     // Produces 1 parameters
                                    //
                                    // Received value:
                                    //
                                    // pointer to SE_ADT_ACCESS_REASON structure
                                    //
                                    // Results in:
                                    //
                                    // String representation of Staging policy's
                                    // access reason.
                                    //

    SeAdtParmTypeResourceAttribute, // Produces 1 parameters
                                    //
                                    // Received value:
                                    //
                                    // pointer to SECURITY_DESCRIPTOR
                                    // structure
                                    //
                                    // Results in:
                                    //
                                    // SDDL string representation of the
                                    // Resource Attribute ACEs in the SD
                                    //

    SeAdtParmTypeClaims,            // Produces 1 parameters
                                    //
                                    // Received value:
                                    //
                                    // pointer to the structure -
                                    // CLAIM_SECURITY_ATTRIBUTES_INFORMATION
                                    // structure
                                    //
                                    // Results in:
                                    //
                                    // Claims information as attributes, value
                                    // pairs
                                    //

    SeAdtParmTypeLogonIdAsSid,      // Produces 4 parameters.
                                    // Received Value:
                                    //
                                    //  SID  (variable length)
                                    //
                                    //Results in:
                                    //
                                    //  param 1: Sid string (based on SID and not derived from the LUID)
                                    //  param 2: -
                                    //  param 3: -
                                    //  param 4: -

    SeAdtParmTypeMultiSzString,     //Produces 1 parameter
                                    //Received value:
                                    //
                                    // PZZWSTR string
                                    //
                                    //Results in:
                                    //
                                    // Unicode string with each null replaced with /r/n

    SeAdtParmTypeLogonIdEx,         //Produces 4 parameters.
                                    //Received Value:
                                    //
                                    //  LUID (fixed length)
                                    //
                                    //Results in:
                                    //
                                    //  param 1: Sid string
                                    //  param 2: Username string
                                    //  param 3: domain name string
                                    //  param 4: Logon ID (Luid) string

} SE_ADT_PARAMETER_TYPE, *PSE_ADT_PARAMETER_TYPE;

#ifndef GUID_DEFINED
#include <guiddef.h>
#endif /* GUID_DEFINED */

typedef struct _SE_ADT_OBJECT_TYPE {
    GUID ObjectType;
    USHORT Flags;
#define SE_ADT_OBJECT_ONLY 0x1
    USHORT Level;
    ACCESS_MASK AccessMask;
} SE_ADT_OBJECT_TYPE, *PSE_ADT_OBJECT_TYPE;

typedef struct _SE_ADT_PARAMETER_ARRAY_ENTRY {

    SE_ADT_PARAMETER_TYPE Type;
    ULONG Length;
    ULONG_PTR Data[2];
    PVOID Address;

} SE_ADT_PARAMETER_ARRAY_ENTRY, *PSE_ADT_PARAMETER_ARRAY_ENTRY;


typedef struct _SE_ADT_ACCESS_REASON{
    ACCESS_MASK AccessMask;
    ULONG  AccessReasons[32];
    ULONG  ObjectTypeIndex;
    ULONG AccessGranted;
    PSECURITY_DESCRIPTOR SecurityDescriptor;    // multple SDs may be stored here in self-relative way.
} SE_ADT_ACCESS_REASON, *PSE_ADT_ACCESS_REASON;

typedef struct _SE_ADT_CLAIMS {

    ULONG Length;
    PCLAIMS_BLOB Claims; // one claim blob will be stored here in self-relative way

} SE_ADT_CLAIMS, *PSE_ADT_CLAIMS;

//
// Structure that will be passed between the Reference Monitor and LSA
// to transmit auditing information.
//

#define SE_MAX_AUDIT_PARAMETERS 32
#define SE_MAX_GENERIC_AUDIT_PARAMETERS 28

typedef struct _SE_ADT_PARAMETER_ARRAY {

    ULONG CategoryId;
    ULONG AuditId;
    ULONG ParameterCount;
    ULONG Length;
    USHORT FlatSubCategoryId;
    USHORT Type;
    ULONG Flags;
    SE_ADT_PARAMETER_ARRAY_ENTRY Parameters[ SE_MAX_AUDIT_PARAMETERS ];

} SE_ADT_PARAMETER_ARRAY, *PSE_ADT_PARAMETER_ARRAY;

typedef struct _SE_ADT_PARAMETER_ARRAY_EX {

    ULONG CategoryId;
    ULONG AuditId;
    ULONG Version;
    ULONG ParameterCount;
    ULONG Length;
    USHORT FlatSubCategoryId;
    USHORT Type;
    ULONG Flags;
    SE_ADT_PARAMETER_ARRAY_ENTRY Parameters[ SE_MAX_AUDIT_PARAMETERS ];

} SE_ADT_PARAMETER_ARRAY_EX, *PSE_ADT_PARAMETER_ARRAY_EX;


#define SE_ADT_PARAMETERS_SELF_RELATIVE     0x00000001
#define SE_ADT_PARAMETERS_SEND_TO_LSA       0x00000002
#define SE_ADT_PARAMETER_EXTENSIBLE_AUDIT   0x00000004
#define SE_ADT_PARAMETER_GENERIC_AUDIT      0x00000008
#define SE_ADT_PARAMETER_WRITE_SYNCHRONOUS  0x00000010


//
// This macro only existed in Windows Server 2008 and after
//

#define LSAP_SE_ADT_PARAMETER_ARRAY_TRUE_SIZE(AuditParameters)    \
     ( sizeof(SE_ADT_PARAMETER_ARRAY) -                           \
       sizeof(SE_ADT_PARAMETER_ARRAY_ENTRY) *                     \
       (SE_MAX_AUDIT_PARAMETERS - AuditParameters->ParameterCount) )

#endif // _NTLSA_AUDIT_


#endif // _NTLSA_IFS_


#ifndef _RTL_RUN_ONCE_DEF
#define _RTL_RUN_ONCE_DEF

//
// Run once
//

#define RTL_RUN_ONCE_INIT {0}   // Static initializer

//
// Run once flags
//

#define RTL_RUN_ONCE_CHECK_ONLY     0x00000001UL
#define RTL_RUN_ONCE_ASYNC          0x00000002UL
#define RTL_RUN_ONCE_INIT_FAILED    0x00000004UL

//
// The context stored in the run once structure must leave the following number
// of low order bits unused.
//

#define RTL_RUN_ONCE_CTX_RESERVED_BITS 2

typedef union _RTL_RUN_ONCE {       
    PVOID Ptr;                      
} RTL_RUN_ONCE, *PRTL_RUN_ONCE;     

typedef
_Function_class_(RTL_RUN_ONCE_INIT_FN)
_IRQL_requires_same_
ULONG /* LOGICAL */
NTAPI
RTL_RUN_ONCE_INIT_FN (
    _Inout_ PRTL_RUN_ONCE RunOnce,
    _Inout_opt_ PVOID Parameter,
    _Inout_opt_ PVOID *Context
    );
typedef RTL_RUN_ONCE_INIT_FN *PRTL_RUN_ONCE_INIT_FN;



#endif // _RTL_RUN_ONCE_DEF



#if (NTDDI_VERSION >= NTDDI_LONGHORN)

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlRunOnceInitialize (
    _Out_ PRTL_RUN_ONCE RunOnce
    );

_IRQL_requires_max_(APC_LEVEL)
_Maybe_raises_SEH_exception_
NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceExecuteOnce (
    _Inout_ PRTL_RUN_ONCE RunOnce,
    _In_ __callback PRTL_RUN_ONCE_INIT_FN InitFn,
    _Inout_opt_ PVOID Parameter,
    _Outptr_opt_result_maybenull_ PVOID *Context
    );

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceBeginInitialize (
    _Inout_ PRTL_RUN_ONCE RunOnce,
    _In_ ULONG Flags,
    _Outptr_opt_result_maybenull_ PVOID *Context
    );

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceComplete (
    _Inout_ PRTL_RUN_ONCE RunOnce,
    _In_ ULONG Flags,
    _In_opt_ PVOID Context
    );

#endif // NTDDI_VERSION >= NTDDI_LONGHORN


//
// This enumerated type is used as the function return value of the function
// that is used to search the tree for a key. FoundNode indicates that the
// function found the key. Insert as left indicates that the key was not found
// and the node should be inserted as the left child of the parent. Insert as
// right indicates that the key was not found and the node should be inserted
//  as the right child of the parent.
//
typedef enum _TABLE_SEARCH_RESULT{
    TableEmptyTree,
    TableFoundNode,
    TableInsertAsLeft,
    TableInsertAsRight
} TABLE_SEARCH_RESULT;

//
//  The results of a compare can be less than, equal, or greater than.
//

typedef enum _RTL_GENERIC_COMPARE_RESULTS {
    GenericLessThan,
    GenericGreaterThan,
    GenericEqual
} RTL_GENERIC_COMPARE_RESULTS;

//
//  Define the Avl version of the generic table package.  Note a generic table
//  should really be an opaque type.  We provide routines to manipulate the structure.
//
//  A generic table is package for inserting, deleting, and looking up elements
//  in a table (e.g., in a symbol table).  To use this package the user
//  defines the structure of the elements stored in the table, provides a
//  comparison function, a memory allocation function, and a memory
//  deallocation function.
//
//  Note: the user compare function must impose a complete ordering among
//  all of the elements, and the table does not allow for duplicate entries.
//

//
// Add an empty typedef so that functions can reference the
// a pointer to the generic table struct before it is declared.
//

struct _RTL_AVL_TABLE;

//
//  The comparison function takes as input pointers to elements containing
//  user defined structures and returns the results of comparing the two
//  elements.
//

typedef
_IRQL_requires_same_
_Function_class_(RTL_AVL_COMPARE_ROUTINE)
RTL_GENERIC_COMPARE_RESULTS
NTAPI
RTL_AVL_COMPARE_ROUTINE (
    _In_ struct _RTL_AVL_TABLE *Table,
    _In_ PVOID FirstStruct,
    _In_ PVOID SecondStruct
    );
typedef RTL_AVL_COMPARE_ROUTINE *PRTL_AVL_COMPARE_ROUTINE;

//
//  The allocation function is called by the generic table package whenever
//  it needs to allocate memory for the table.
//

typedef
_IRQL_requires_same_
_Function_class_(RTL_AVL_ALLOCATE_ROUTINE)
__drv_allocatesMem(Mem)
PVOID
NTAPI
RTL_AVL_ALLOCATE_ROUTINE (
    _In_ struct _RTL_AVL_TABLE *Table,
    _In_ CLONG ByteSize
    );
typedef RTL_AVL_ALLOCATE_ROUTINE *PRTL_AVL_ALLOCATE_ROUTINE;

//
//  The deallocation function is called by the generic table package whenever
//  it needs to deallocate memory from the table that was allocated by calling
//  the user supplied allocation function.
//

typedef
_IRQL_requires_same_
_Function_class_(RTL_AVL_FREE_ROUTINE)
VOID
NTAPI
RTL_AVL_FREE_ROUTINE (
    _In_ struct _RTL_AVL_TABLE *Table,
    _In_ __drv_freesMem(Mem) _Post_invalid_ PVOID Buffer
    );
typedef RTL_AVL_FREE_ROUTINE *PRTL_AVL_FREE_ROUTINE;

//
//  The match function takes as input the user data to be matched and a pointer
//  to some match data, which was passed along with the function pointer.  It
//  returns TRUE for a match and FALSE for no match.
//
//  RTL_AVL_MATCH_FUNCTION returns
//      STATUS_SUCCESS if the IndexRow matches
//      STATUS_NO_MATCH if the IndexRow does not match, but the enumeration should
//          continue
//      STATUS_NO_MORE_MATCHES if the IndexRow does not match, and the enumeration
//          should terminate
//


typedef
_IRQL_requires_same_
_Function_class_(RTL_AVL_MATCH_FUNCTION)
NTSTATUS
NTAPI
RTL_AVL_MATCH_FUNCTION (
    _In_ struct _RTL_AVL_TABLE *Table,
    _In_ PVOID UserData,
    _In_ PVOID MatchData
    );
typedef RTL_AVL_MATCH_FUNCTION *PRTL_AVL_MATCH_FUNCTION;

//
//  Define the balanced tree links and Balance field.  (No Rank field
//  defined at this time.)
//
//  Callers should treat this structure as opaque!
//
//  The root of a balanced binary tree is not a real node in the tree
//  but rather points to a real node which is the root.  It is always
//  in the table below, and its fields are used as follows:
//
//      Parent      Pointer to self, to allow for detection of the root.
//      LeftChild   NULL
//      RightChild  Pointer to real root
//      Balance     Undefined, however it is set to a convenient value
//                  (depending on the algorithm) prior to rebalancing
//                  in insert and delete routines.
//

typedef struct _RTL_BALANCED_LINKS {
    struct _RTL_BALANCED_LINKS *Parent;
    struct _RTL_BALANCED_LINKS *LeftChild;
    struct _RTL_BALANCED_LINKS *RightChild;
    CHAR Balance;
    UCHAR Reserved[3];
} RTL_BALANCED_LINKS;
typedef RTL_BALANCED_LINKS *PRTL_BALANCED_LINKS;


//
//  To use the generic table package the user declares a variable of type
//  GENERIC_TABLE and then uses the routines described below to initialize
//  the table and to manipulate the table.  Note that the generic table
//  should really be an opaque type.
//

typedef struct _RTL_AVL_TABLE {
    RTL_BALANCED_LINKS BalancedRoot;
    PVOID OrderedPointer;
    ULONG WhichOrderedElement;
    ULONG NumberGenericTableElements;
    ULONG DepthOfTree;
    PRTL_BALANCED_LINKS RestartKey;
    ULONG DeleteCount;
    PRTL_AVL_COMPARE_ROUTINE CompareRoutine;
    PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine;
    PRTL_AVL_FREE_ROUTINE FreeRoutine;
    PVOID TableContext;
} RTL_AVL_TABLE;
typedef RTL_AVL_TABLE *PRTL_AVL_TABLE;

//
//  The procedure InitializeGenericTable takes as input an uninitialized
//  generic table variable and pointers to the three user supplied routines.
//  This must be called for every individual generic table variable before
//  it can be used.
//

#if (NTDDI_VERSION >= NTDDI_WINXP)
NTSYSAPI
VOID
NTAPI
RtlInitializeGenericTableAvl (
    _Out_ PRTL_AVL_TABLE Table,
    _In_ PRTL_AVL_COMPARE_ROUTINE CompareRoutine,
    _In_ PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine,
    _In_ PRTL_AVL_FREE_ROUTINE FreeRoutine,
    _In_opt_ PVOID TableContext
    );
#endif // NTDDI_VERSION >= NTDDI_WINXP

//
//  The function InsertElementGenericTable will insert a new element
//  in a table.  It does this by allocating space for the new element
//  (this includes AVL links), inserting the element in the table, and
//  then returning to the user a pointer to the new element.  If an element
//  with the same key already exists in the table the return value is a pointer
//  to the old element.  The optional output parameter NewElement is used
//  to indicate if the element previously existed in the table.  Note: the user
//  supplied Buffer is only used for searching the table, upon insertion its
//  contents are copied to the newly created element.  This means that
//  pointer to the input buffer will not point to the new element.
//

#if (NTDDI_VERSION >= NTDDI_WINXP)
NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTableAvl (
    _In_ PRTL_AVL_TABLE Table,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ CLONG BufferSize,
    _Out_opt_ PBOOLEAN NewElement
    );
#endif // NTDDI_VERSION >= NTDDI_WINXP

//
//  The function InsertElementGenericTableFull will insert a new element
//  in a table.  It does this by allocating space for the new element
//  (this includes AVL links), inserting the element in the table, and
//  then returning to the user a pointer to the new element.  If an element
//  with the same key already exists in the table the return value is a pointer
//  to the old element.  The optional output parameter NewElement is used
//  to indicate if the element previously existed in the table.  Note: the user
//  supplied Buffer is only used for searching the table, upon insertion its
//  contents are copied to the newly created element.  This means that
//  pointer to the input buffer will not point to the new element.
//  This routine is passed the NodeOrParent and SearchResult from a
//  previous RtlLookupElementGenericTableFull.
//

#if (NTDDI_VERSION >= NTDDI_WINXP)
NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTableFullAvl (
    _In_ PRTL_AVL_TABLE Table,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ CLONG BufferSize,
    _Out_opt_ PBOOLEAN NewElement,
    _In_ PVOID NodeOrParent,
    _In_ TABLE_SEARCH_RESULT SearchResult
    );
#endif // NTDDI_VERSION >= NTDDI_WINXP

//
//  The function DeleteElementGenericTable will find and delete an element
//  from a generic table.  If the element is located and deleted the return
//  value is TRUE, otherwise if the element is not located the return value
//  is FALSE.  The user supplied input buffer is only used as a key in
//  locating the element in the table.
//

#if (NTDDI_VERSION >= NTDDI_WINXP)
NTSYSAPI
BOOLEAN
NTAPI
RtlDeleteElementGenericTableAvl (
    _In_ PRTL_AVL_TABLE Table,
    _In_ PVOID Buffer
    );
#endif // NTDDI_VERSION >= NTDDI_WINXP

//
//  The function DeleteElementGenericTableAvxEx deletes the element specified
//  by the NodeOrParent pointer. This element user data pointer must have first
//  been obtained with RtlLookupElementGenericTableFull.
//

#if (NTDDI_VERSION >= NTDDI_WIN8)
NTSYSAPI
VOID
NTAPI
RtlDeleteElementGenericTableAvlEx (
    _In_ PRTL_AVL_TABLE Table,
    _In_ PVOID NodeOrParent
    );
#endif // NTDDI_VERSION >= NTDDI_WIN8


//
//  The function LookupElementGenericTable will find an element in a generic
//  table.  If the element is located the return value is a pointer to
//  the user defined structure associated with the element, otherwise if
//  the element is not located the return value is NULL.  The user supplied
//  input buffer is only used as a key in locating the element in the table.
//

#if (NTDDI_VERSION >= NTDDI_WINXP)
_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableAvl (
    _In_ PRTL_AVL_TABLE Table,
    _In_ PVOID Buffer
    );
#endif // NTDDI_VERSION >= NTDDI_WINXP

//
//  The function LookupElementGenericTableFull will find an element in a generic
//  table.  If the element is located the return value is a pointer to
//  the user defined structure associated with the element.  If the element is not
//  located then a pointer to the parent for the insert location is returned.  The
//  user must look at the SearchResult value to determine which is being returned.
//  The user can use the SearchResult and parent for a subsequent FullInsertElement
//  call to optimize the insert.
//

#if (NTDDI_VERSION >= NTDDI_WINXP)
NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableFullAvl (
    _In_ PRTL_AVL_TABLE Table,
    _In_ PVOID Buffer,
    _Out_ PVOID *NodeOrParent,
    _Out_ TABLE_SEARCH_RESULT *SearchResult
    );
#endif // NTDDI_VERSION >= NTDDI_WINXP

//
//  The function EnumerateGenericTable will return to the caller one-by-one
//  the elements of of a table.  The return value is a pointer to the user
//  defined structure associated with the element.  The input parameter
//  Restart indicates if the enumeration should start from the beginning
//  or should return the next element.  If the are no more new elements to
//  return the return value is NULL.  As an example of its use, to enumerate
//  all of the elements in a table the user would write:
//
//      for (ptr = EnumerateGenericTable(Table, TRUE);
//           ptr != NULL;
//           ptr = EnumerateGenericTable(Table, FALSE)) {
//              :
//      }
//
//  NOTE:   This routine does not modify the structure of the tree, but saves
//          the last node returned in the generic table itself, and for this
//          reason requires exclusive access to the table for the duration of
//          the enumeration.
//

#if (NTDDI_VERSION >= NTDDI_WINXP)
_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableAvl (
    _In_ PRTL_AVL_TABLE Table,
    _In_ BOOLEAN Restart
    );
#endif // NTDDI_VERSION >= NTDDI_WINXP

//
//  The function EnumerateGenericTableWithoutSplaying will return to the
//  caller one-by-one the elements of of a table.  The return value is a
//  pointer to the user defined structure associated with the element.
//  The input parameter RestartKey indicates if the enumeration should
//  start from the beginning or should return the next element.  If the
//  are no more new elements to return the return value is NULL.  As an
//  example of its use, to enumerate all of the elements in a table the
//  user would write:
//
//      RestartKey = NULL;
//      for (ptr = EnumerateGenericTableWithoutSplaying(Table, &RestartKey);
//           ptr != NULL;
//           ptr = EnumerateGenericTableWithoutSplaying(Table, &RestartKey)) {
//              :
//      }
//
//  If RestartKey is NULL, the package will start from the least entry in the
//  table, otherwise it will start from the last entry returned.
//
//  NOTE:   This routine does not modify either the structure of the tree
//          or the generic table itself, but must insure that no deletes
//          occur for the duration of the enumeration, typically by having
//          at least shared access to the table for the duration.
//

#if (NTDDI_VERSION >= NTDDI_WINXP)
_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableWithoutSplayingAvl (
    _In_ PRTL_AVL_TABLE Table,
    _Inout_ PVOID *RestartKey
    );
#endif // NTDDI_VERSION >= NTDDI_WINXP

//
//  RtlLookupFirstMatchingElementGenericTableAvl will return the left-most
//  element in the tree matching the data in Buffer.  If, for example, the tree
//  contains filenames there may exist several that differ only in case. A case-
//  blind searcher can use this routine to position himself in the tree at the
//  first match, and use an enumeration routine (such as RtlEnumerateGenericTableWithoutSplayingAvl
//  to return each subsequent match.
//

#if (NTDDI_VERSION >= NTDDI_WINXP)
_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlLookupFirstMatchingElementGenericTableAvl (
    _In_ PRTL_AVL_TABLE Table,
    _In_ PVOID Buffer,
    _Out_ PVOID *RestartKey
    );
#endif // NTDDI_VERSION >= NTDDI_WINXP

//
//  The function EnumerateGenericTableLikeADirectory will return to the
//  caller one-by-one the elements of of a table.  The return value is a
//  pointer to the user defined structure associated with the element.
//  The input parameter RestartKey indicates if the enumeration should
//  start from the beginning or should return the next element.  If the
//  are no more new elements to return the return value is NULL.  As an
//  example of its use, to enumerate all of the elements in a table the
//  user would write:
//
//      RestartKey = NULL;
//      for (ptr = EnumerateGenericTableLikeADirectory(Table, &RestartKey, ...);
//           ptr != NULL;
//           ptr = EnumerateGenericTableLikeADirectory(Table, &RestartKey, ...)) {
//              :
//      }
//
//  If RestartKey is NULL, the package will start from the least entry in the
//  table, otherwise it will start from the last entry returned.
//
//  NOTE:   This routine does not modify either the structure of the tree
//          or the generic table itself.  The table must only be acquired
//          shared for the duration of this call, and all synchronization
//          may optionally be dropped between calls.  Enumeration is always
//          correctly resumed in the most efficient manner possible via the
//          IN OUT parameters provided.
//
//  ******  Explain NextFlag.  Directory enumeration resumes from a key
//          requires more thought.  Also need the match pattern and IgnoreCase.
//          Should some structure be introduced to carry it all?
//

#if (NTDDI_VERSION >= NTDDI_WINXP)
_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableLikeADirectory (
    _In_ PRTL_AVL_TABLE Table,
    _In_opt_ PRTL_AVL_MATCH_FUNCTION MatchFunction,
    _In_opt_ PVOID MatchData,
    _In_ ULONG NextFlag,
    _Inout_ PVOID *RestartKey,
    _Inout_ PULONG DeleteCount,
    _In_ PVOID Buffer
    );
#endif // NTDDI_VERSION >= NTDDI_WINXP

//
// The function GetElementGenericTable will return the i'th element
// inserted in the generic table.  I = 0 implies the first element,
// I = (RtlNumberGenericTableElements(Table)-1) will return the last element
// inserted into the generic table.  The type of I is ULONG.  Values
// of I > than (NumberGenericTableElements(Table)-1) will return NULL.  If
// an arbitrary element is deleted from the generic table it will cause
// all elements inserted after the deleted element to "move up".

#if (NTDDI_VERSION >= NTDDI_WINXP)
_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlGetElementGenericTableAvl (
    _In_ PRTL_AVL_TABLE Table,
    _In_ ULONG I
    );
#endif // NTDDI_VERSION >= NTDDI_WINXP

//
// The function NumberGenericTableElements returns a ULONG value
// which is the number of generic table elements currently inserted
// in the generic table.

#if (NTDDI_VERSION >= NTDDI_WINXP)
NTSYSAPI
ULONG
NTAPI
RtlNumberGenericTableElementsAvl (
    _In_ PRTL_AVL_TABLE Table
    );
#endif // NTDDI_VERSION >= NTDDI_WINXP

//
//  The function IsGenericTableEmpty will return to the caller TRUE if
//  the input table is empty (i.e., does not contain any elements) and
//  FALSE otherwise.
//

//
// Generic extensions for using generic structures with the avl libraries.
//
#if (NTDDI_VERSION >= NTDDI_WINXP)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlIsGenericTableEmptyAvl (
    _In_ PRTL_AVL_TABLE Table
    );
#endif // NTDDI_VERSION >= NTDDI_WINXP

//
//  As an aid to allowing existing generic table users to do (in most
//  cases) a single-line edit to switch over to Avl table use, we
//  have the following defines and inline routine definitions which
//  redirect calls and types.  Note that the type override (performed
//  by #define below) will not work in the unexpected event that someone
//  has used a pointer or type specifier in their own #define, since
//  #define processing is one pass and does not nest.  The __inline
//  declarations below do not have this limitation, however.
//
//  To switch to using Avl tables, add the following line before your
//  includes:
//
//  #define RTL_USE_AVL_TABLES 0
//

#ifdef RTL_USE_AVL_TABLES

#undef PRTL_GENERIC_COMPARE_ROUTINE
#undef RTL_GENERIC_COMPARE_ROUTINE
#undef PRTL_GENERIC_ALLOCATE_ROUTINE
#undef RTL_GENERIC_ALLOCATE_ROUTINE
#undef PRTL_GENERIC_FREE_ROUTINE
#undef RTL_GENERIC_FREE_ROUTINE
#undef RTL_GENERIC_TABLE
#undef PRTL_GENERIC_TABLE

#define PRTL_GENERIC_COMPARE_ROUTINE PRTL_AVL_COMPARE_ROUTINE
#define RTL_GENERIC_COMPARE_ROUTINE RTL_AVL_COMPARE_ROUTINE
#define PRTL_GENERIC_ALLOCATE_ROUTINE PRTL_AVL_ALLOCATE_ROUTINE
#define RTL_GENERIC_ALLOCATE_ROUTINE RTL_AVL_ALLOCATE_ROUTINE
#define PRTL_GENERIC_FREE_ROUTINE PRTL_AVL_FREE_ROUTINE
#define RTL_GENERIC_FREE_ROUTINE RTL_AVL_FREE_ROUTINE
#define RTL_GENERIC_TABLE RTL_AVL_TABLE
#define PRTL_GENERIC_TABLE PRTL_AVL_TABLE

#define RtlInitializeGenericTable               RtlInitializeGenericTableAvl
#define RtlInsertElementGenericTable            RtlInsertElementGenericTableAvl
#define RtlInsertElementGenericTableFull        RtlInsertElementGenericTableFullAvl
#define RtlDeleteElementGenericTable            RtlDeleteElementGenericTableAvl
#define RtlLookupElementGenericTable            RtlLookupElementGenericTableAvl
#define RtlLookupElementGenericTableFull        RtlLookupElementGenericTableFullAvl
#define RtlEnumerateGenericTable                RtlEnumerateGenericTableAvl
#define RtlEnumerateGenericTableWithoutSplaying RtlEnumerateGenericTableWithoutSplayingAvl
#define RtlGetElementGenericTable               RtlGetElementGenericTableAvl
#define RtlNumberGenericTableElements           RtlNumberGenericTableElementsAvl
#define RtlIsGenericTableEmpty                  RtlIsGenericTableEmptyAvl

#endif // RTL_USE_AVL_TABLES


//
//  Define the splay links and the associated manipuliation macros and
//  routines.  Note that the splay_links should be an opaque type.
//  Routine are provided to traverse and manipulate the structure.
//

typedef struct _RTL_SPLAY_LINKS {
    struct _RTL_SPLAY_LINKS *Parent;
    struct _RTL_SPLAY_LINKS *LeftChild;
    struct _RTL_SPLAY_LINKS *RightChild;
} RTL_SPLAY_LINKS;
typedef RTL_SPLAY_LINKS *PRTL_SPLAY_LINKS;


#if !defined(MIDL_PASS) && !defined(SORTPP_PASS)

FORCEINLINE
VOID
RtlInitializeSplayLinks (
    _Out_ PRTL_SPLAY_LINKS Links
    )
//++
//
//  The procedure InitializeSplayLinks takes as input a pointer to
//  splay link and initializes its substructure.  All splay link nodes must
//  be initialized before they are used in the different splay routines and
//  macros.
//
//--
{
    Links->Parent = Links;
    Links->LeftChild = NULL;
    Links->RightChild = NULL;
}

#endif // !defined(MIDL_PASS) && !defined(SORTPP_PASS)

//
//  The macro function Parent takes as input a pointer to a splay link in a
//  tree and returns a pointer to the splay link of the parent of the input
//  node.  If the input node is the root of the tree the return value is
//  equal to the input value.
//
//  PRTL_SPLAY_LINKS
//  RtlParent (
//      PRTL_SPLAY_LINKS Links
//      );
//

#define RtlParent(Links) (           \
    (PRTL_SPLAY_LINKS)(Links)->Parent \
    )

//
//  The macro function LeftChild takes as input a pointer to a splay link in
//  a tree and returns a pointer to the splay link of the left child of the
//  input node.  If the left child does not exist, the return value is NULL.
//
//  PRTL_SPLAY_LINKS
//  RtlLeftChild (
//      PRTL_SPLAY_LINKS Links
//      );
//

#define RtlLeftChild(Links) (           \
    (PRTL_SPLAY_LINKS)(Links)->LeftChild \
    )

//
//  The macro function RightChild takes as input a pointer to a splay link
//  in a tree and returns a pointer to the splay link of the right child of
//  the input node.  If the right child does not exist, the return value is
//  NULL.
//
//  PRTL_SPLAY_LINKS
//  RtlRightChild (
//      PRTL_SPLAY_LINKS Links
//      );
//

#define RtlRightChild(Links) (           \
    (PRTL_SPLAY_LINKS)(Links)->RightChild \
    )

//
//  The macro function IsRoot takes as input a pointer to a splay link
//  in a tree and returns TRUE if the input node is the root of the tree,
//  otherwise it returns FALSE.
//
//  BOOLEAN
//  RtlIsRoot (
//      PRTL_SPLAY_LINKS Links
//      );
//

#define RtlIsRoot(Links) (                          \
    (RtlParent(Links) == (PRTL_SPLAY_LINKS)(Links)) \
    )

//
//  The macro function IsLeftChild takes as input a pointer to a splay link
//  in a tree and returns TRUE if the input node is the left child of its
//  parent, otherwise it returns FALSE.
//
//  BOOLEAN
//  RtlIsLeftChild (
//      PRTL_SPLAY_LINKS Links
//      );
//

#define RtlIsLeftChild(Links) (                                   \
    (RtlLeftChild(RtlParent(Links)) == (PRTL_SPLAY_LINKS)(Links)) \
    )

//
//  The macro function IsRightChild takes as input a pointer to a splay link
//  in a tree and returns TRUE if the input node is the right child of its
//  parent, otherwise it returns FALSE.
//
//  BOOLEAN
//  RtlIsRightChild (
//      PRTL_SPLAY_LINKS Links
//      );
//

#define RtlIsRightChild(Links) (                                   \
    (RtlRightChild(RtlParent(Links)) == (PRTL_SPLAY_LINKS)(Links)) \
    )


#if !defined(MIDL_PASS) && !defined(SORTPP_PASS)

FORCEINLINE
VOID
RtlInsertAsLeftChild (
    _Inout_ PRTL_SPLAY_LINKS ParentLinks,
    _Inout_ PRTL_SPLAY_LINKS ChildLinks
    )
//++
//
//  The procedure InsertAsLeftChild takes as input a pointer to a splay
//  link in a tree and a pointer to a node not in a tree.  It inserts the
//  second node as the left child of the first node.  The first node must not
//  already have a left child, and the second node must not already have a
//  parent.
//
//--
{
    ParentLinks->LeftChild = ChildLinks;
    ChildLinks->Parent = ParentLinks;
}

FORCEINLINE
VOID
RtlInsertAsRightChild (
    _Inout_ PRTL_SPLAY_LINKS ParentLinks,
    _Inout_ PRTL_SPLAY_LINKS ChildLinks
    )
//++
//
//  The procedure InsertAsRightChild takes as input a pointer to a splay
//  link in a tree and a pointer to a node not in a tree.  It inserts the
//  second node as the right child of the first node.  The first node must not
//  already have a right child, and the second node must not already have a
//  parent.
//
//--
{
    ParentLinks->RightChild = ChildLinks;
    ChildLinks->Parent = ParentLinks;
}

#endif // !defined(MIDL_PASS) && !defined(SORTPP_PASS)


//
//  The Splay function takes as input a pointer to a splay link in a tree
//  and splays the tree.  Its function return value is a pointer to the
//  root of the splayed tree.
//

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlSplay (
    _Inout_ PRTL_SPLAY_LINKS Links
    );
#endif

//
//  The Delete function takes as input a pointer to a splay link in a tree
//  and deletes that node from the tree.  Its function return value is a
//  pointer to the root of the tree.  If the tree is now empty, the return
//  value is NULL.
//


#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlDelete (
    _In_ PRTL_SPLAY_LINKS Links
    );
#endif

//
//  The DeleteNoSplay function takes as input a pointer to a splay link in a tree,
//  the caller's pointer to the root of the tree and deletes that node from the
//  tree.  Upon return the caller's pointer to the root node will correctly point
//  at the root of the tree.
//
//  It operationally differs from RtlDelete only in that it will not splay the tree.
//

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
VOID
NTAPI
RtlDeleteNoSplay (
    _In_ PRTL_SPLAY_LINKS Links,
    _Inout_ PRTL_SPLAY_LINKS *Root
    );
#endif

//
//  The SubtreeSuccessor function takes as input a pointer to a splay link
//  in a tree and returns a pointer to the successor of the input node of
//  the substree rooted at the input node.  If there is not a successor, the
//  return value is NULL.
//

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_Must_inspect_result_
NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlSubtreeSuccessor (
    _In_ PRTL_SPLAY_LINKS Links
    );
#endif

//
//  The SubtreePredecessor function takes as input a pointer to a splay link
//  in a tree and returns a pointer to the predecessor of the input node of
//  the substree rooted at the input node.  If there is not a predecessor,
//  the return value is NULL.
//

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_Must_inspect_result_
NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlSubtreePredecessor (
    _In_ PRTL_SPLAY_LINKS Links
    );
#endif

//
//  The RealSuccessor function takes as input a pointer to a splay link
//  in a tree and returns a pointer to the successor of the input node within
//  the entire tree.  If there is not a successor, the return value is NULL.
//

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_Must_inspect_result_
NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlRealSuccessor (
    _In_ PRTL_SPLAY_LINKS Links
    );
#endif

//
//  The RealPredecessor function takes as input a pointer to a splay link
//  in a tree and returns a pointer to the predecessor of the input node
//  within the entire tree.  If there is not a predecessor, the return value
//  is NULL.
//

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_Must_inspect_result_
NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlRealPredecessor (
    _In_ PRTL_SPLAY_LINKS Links
    );
#endif


//
//  Define the generic table package.  Note a generic table should really
//  be an opaque type.  We provide routines to manipulate the structure.
//
//  A generic table is package for inserting, deleting, and looking up elements
//  in a table (e.g., in a symbol table).  To use this package the user
//  defines the structure of the elements stored in the table, provides a
//  comparison function, a memory allocation function, and a memory
//  deallocation function.
//
//  Note: the user compare function must impose a complete ordering among
//  all of the elements, and the table does not allow for duplicate entries.
//

//
//  Do not do the following defines if using Avl
//

#ifndef RTL_USE_AVL_TABLES

//
// Add an empty typedef so that functions can reference the
// a pointer to the generic table struct before it is declared.
//

struct _RTL_GENERIC_TABLE;

//
//  The comparison function takes as input pointers to elements containing
//  user defined structures and returns the results of comparing the two
//  elements.
//

typedef
_IRQL_requires_same_
_Function_class_(RTL_GENERIC_COMPARE_ROUTINE)
RTL_GENERIC_COMPARE_RESULTS
NTAPI
RTL_GENERIC_COMPARE_ROUTINE (
    _In_ struct _RTL_GENERIC_TABLE *Table,
    _In_ PVOID FirstStruct,
    _In_ PVOID SecondStruct
    );
typedef RTL_GENERIC_COMPARE_ROUTINE *PRTL_GENERIC_COMPARE_ROUTINE;

//
//  The allocation function is called by the generic table package whenever
//  it needs to allocate memory for the table.
//

typedef
_IRQL_requires_same_
_Function_class_(RTL_GENERIC_ALLOCATE_ROUTINE)
__drv_allocatesMem(Mem)
PVOID
NTAPI
RTL_GENERIC_ALLOCATE_ROUTINE (
    _In_ struct _RTL_GENERIC_TABLE *Table,
    _In_ CLONG ByteSize
    );
typedef RTL_GENERIC_ALLOCATE_ROUTINE *PRTL_GENERIC_ALLOCATE_ROUTINE;

//
//  The deallocation function is called by the generic table package whenever
//  it needs to deallocate memory from the table that was allocated by calling
//  the user supplied allocation function.
//

typedef
_IRQL_requires_same_
_Function_class_(RTL_GENERIC_FREE_ROUTINE)
VOID
NTAPI
RTL_GENERIC_FREE_ROUTINE (
    _In_ struct _RTL_GENERIC_TABLE *Table,
    _In_ __drv_freesMem(Mem) _Post_invalid_ PVOID Buffer
    );
typedef RTL_GENERIC_FREE_ROUTINE *PRTL_GENERIC_FREE_ROUTINE;

//
//  To use the generic table package the user declares a variable of type
//  GENERIC_TABLE and then uses the routines described below to initialize
//  the table and to manipulate the table.  Note that the generic table
//  should really be an opaque type.
//

typedef struct _RTL_GENERIC_TABLE {
    PRTL_SPLAY_LINKS TableRoot;
    LIST_ENTRY InsertOrderList;
    PLIST_ENTRY OrderedPointer;
    ULONG WhichOrderedElement;
    ULONG NumberGenericTableElements;
    PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine;
    PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine;
    PRTL_GENERIC_FREE_ROUTINE FreeRoutine;
    PVOID TableContext;
} RTL_GENERIC_TABLE;
typedef RTL_GENERIC_TABLE *PRTL_GENERIC_TABLE;

//
//  The procedure InitializeGenericTable takes as input an uninitialized
//  generic table variable and pointers to the three user supplied routines.
//  This must be called for every individual generic table variable before
//  it can be used.
//

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
VOID
NTAPI
RtlInitializeGenericTable (
    _Out_ PRTL_GENERIC_TABLE Table,
    _In_ PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine,
    _In_ PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine,
    _In_ PRTL_GENERIC_FREE_ROUTINE FreeRoutine,
    _In_opt_ PVOID TableContext
    );
#endif

//
//  The function InsertElementGenericTable will insert a new element
//  in a table.  It does this by allocating space for the new element
//  (this includes splay links), inserting the element in the table, and
//  then returning to the user a pointer to the new element.  If an element
//  with the same key already exists in the table the return value is a pointer
//  to the old element.  The optional output parameter NewElement is used
//  to indicate if the element previously existed in the table.  Note: the user
//  supplied Buffer is only used for searching the table, upon insertion its
//  contents are copied to the newly created element.  This means that
//  pointer to the input buffer will not point to the new element.
//

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTable (
    _In_ PRTL_GENERIC_TABLE Table,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ CLONG BufferSize,
    _Out_opt_ PBOOLEAN NewElement
    );
#endif

//
//  The function InsertElementGenericTableFull will insert a new element
//  in a table.  It does this by allocating space for the new element
//  (this includes splay links), inserting the element in the table, and
//  then returning to the user a pointer to the new element.  If an element
//  with the same key already exists in the table the return value is a pointer
//  to the old element.  The optional output parameter NewElement is used
//  to indicate if the element previously existed in the table.  Note: the user
//  supplied Buffer is only used for searching the table, upon insertion its
//  contents are copied to the newly created element.  This means that
//  pointer to the input buffer will not point to the new element.
//  This routine is passed the NodeOrParent and SearchResult from a
//  previous RtlLookupElementGenericTableFull.
//

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTableFull (
    _In_ PRTL_GENERIC_TABLE Table,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ CLONG BufferSize,
    _Out_opt_ PBOOLEAN NewElement,
    _In_ PVOID NodeOrParent,
    _In_ TABLE_SEARCH_RESULT SearchResult
    );
#endif

//
//  The function DeleteElementGenericTable will find and delete an element
//  from a generic table.  If the element is located and deleted the return
//  value is TRUE, otherwise if the element is not located the return value
//  is FALSE.  The user supplied input buffer is only used as a key in
//  locating the element in the table.
//

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
BOOLEAN
NTAPI
RtlDeleteElementGenericTable (
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ PVOID Buffer
    );
#endif

//
//  The function LookupElementGenericTable will find an element in a generic
//  table.  If the element is located the return value is a pointer to
//  the user defined structure associated with the element, otherwise if
//  the element is not located the return value is NULL.  The user supplied
//  input buffer is only used as a key in locating the element in the table.
//

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTable (
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ PVOID Buffer
    );
#endif

//
//  The function LookupElementGenericTableFull will find an element in a generic
//  table.  If the element is located the return value is a pointer to
//  the user defined structure associated with the element.  If the element is not
//  located then a pointer to the parent for the insert location is returned.  The
//  user must look at the SearchResult value to determine which is being returned.
//  The user can use the SearchResult and parent for a subsequent FullInsertElement
//  call to optimize the insert.
//

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableFull (
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ PVOID Buffer,
    _Out_ PVOID *NodeOrParent,
    _Out_ TABLE_SEARCH_RESULT *SearchResult
    );
#endif

//
//  The function EnumerateGenericTable will return to the caller one-by-one
//  the elements of of a table.  The return value is a pointer to the user
//  defined structure associated with the element.  The input parameter
//  Restart indicates if the enumeration should start from the beginning
//  or should return the next element.  If the are no more new elements to
//  return the return value is NULL.  As an example of its use, to enumerate
//  all of the elements in a table the user would write:
//
//      for (ptr = EnumerateGenericTable(Table, TRUE);
//           ptr != NULL;
//           ptr = EnumerateGenericTable(Table, FALSE)) {
//              :
//      }
//
//
//  PLEASE NOTE:
//
//      If you enumerate a GenericTable using RtlEnumerateGenericTable, you
//      will flatten the table, turning it into a sorted linked list.
//      To enumerate the table without perturbing the splay links, use
//      RtlEnumerateGenericTableWithoutSplaying

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTable (
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ BOOLEAN Restart
    );
#endif

//
//  The function EnumerateGenericTableWithoutSplaying will return to the
//  caller one-by-one the elements of of a table.  The return value is a
//  pointer to the user defined structure associated with the element.
//  The input parameter RestartKey indicates if the enumeration should
//  start from the beginning or should return the next element.  If the
//  are no more new elements to return the return value is NULL.  As an
//  example of its use, to enumerate all of the elements in a table the
//  user would write:
//
//      RestartKey = NULL;
//      for (ptr = EnumerateGenericTableWithoutSplaying(Table, &RestartKey);
//           ptr != NULL;
//           ptr = EnumerateGenericTableWithoutSplaying(Table, &RestartKey)) {
//              :
//      }
//
//  If RestartKey is NULL, the package will start from the least entry in the
//  table, otherwise it will start from the last entry returned.
//
//
//  Note that unlike RtlEnumerateGenericTable, this routine will NOT perturb
//  the splay order of the tree.
//

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableWithoutSplaying (
    _In_ PRTL_GENERIC_TABLE Table,
    _Inout_ PVOID *RestartKey
    );
#endif

//
// The function GetElementGenericTable will return the i'th element
// inserted in the generic table.  I = 0 implies the first element,
// I = (RtlNumberGenericTableElements(Table)-1) will return the last element
// inserted into the generic table.  The type of I is ULONG.  Values
// of I > than (NumberGenericTableElements(Table)-1) will return NULL.  If
// an arbitrary element is deleted from the generic table it will cause
// all elements inserted after the deleted element to "move up".

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlGetElementGenericTable(
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ ULONG I
    );
#endif

//
// The function NumberGenericTableElements returns a ULONG value
// which is the number of generic table elements currently inserted
// in the generic table.

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
ULONG
NTAPI
RtlNumberGenericTableElements(
    _In_ PRTL_GENERIC_TABLE Table
    );
#endif

//
//  The function IsGenericTableEmpty will return to the caller TRUE if
//  the input table is empty (i.e., does not contain any elements) and
//  FALSE otherwise.
//

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlIsGenericTableEmpty (
    _In_ PRTL_GENERIC_TABLE Table
    );
#endif

#endif // RTL_USE_AVL_TABLES



//
// The hash table header structure can either be allocated
// by the caller, or by the hash table creation function itself.
// This flag indicates what was done at creation time.
//
#define RTL_HASH_ALLOCATED_HEADER            0x00000001

//
// The RTL_HASH_RESERVED_SIGNATURE is the signature used internally for
// enumerators. A caller can never assign this signature to
// valid entries.
//
#define RTL_HASH_RESERVED_SIGNATURE 0


typedef struct _RTL_DYNAMIC_HASH_TABLE_ENTRY {
    LIST_ENTRY Linkage;
    ULONG_PTR Signature;
} RTL_DYNAMIC_HASH_TABLE_ENTRY, *PRTL_DYNAMIC_HASH_TABLE_ENTRY;


//
// Some components want to see the actual signature and can use
// this macro to encapsulate that operation.
//
#define HASH_ENTRY_KEY(x)    ((x)->Signature)


//
// Brief background on each of the parameters and their
// justification:
// 1. ChainHead stores the pointer to a bucket. This is needed since
//    our hash chains are doubly-linked circular lists, and there is
//    is no way to determine whether we've reached the end of the
//    chain unless we store the pointer to the bucket itself. This
//    is particularly used in walking the sub-list of entries returned
//    by a lookup. We need to know when the sub-list has been
//    completely returned.
// 2. PrevLinkage stores a pointer to the entry before the entry
//    under consideration. The reason for storing the previous entry
//    instead of the entry itself is for cases where a lookup fails
//    and PrevLinkage actually stores the entry that would have been
//    the previous entry, had the looked up entry existed. This can
//    then be used to actually insert the entry at that place.
// 3. Signature is used primarily as a safety check in insertion.
//    This field must match the Signature of the entry being inserted.
//

typedef struct _RTL_DYNAMIC_HASH_TABLE_CONTEXT {
    PLIST_ENTRY ChainHead;
    PLIST_ENTRY PrevLinkage;
    ULONG_PTR Signature;
} RTL_DYNAMIC_HASH_TABLE_CONTEXT, *PRTL_DYNAMIC_HASH_TABLE_CONTEXT;

typedef struct _RTL_DYNAMIC_HASH_TABLE_ENUMERATOR {
    union {
       RTL_DYNAMIC_HASH_TABLE_ENTRY HashEntry;
       PLIST_ENTRY CurEntry;
    };
    PLIST_ENTRY ChainHead;
    ULONG BucketIndex;
} RTL_DYNAMIC_HASH_TABLE_ENUMERATOR, *PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR;

typedef struct _RTL_DYNAMIC_HASH_TABLE {

    // Entries initialized at creation
    ULONG Flags;
    ULONG Shift;

    // Entries used in bucket computation.
    ULONG TableSize;
    ULONG Pivot;
    ULONG DivisorMask;

    // Counters
    ULONG NumEntries;
    ULONG NonEmptyBuckets;
    ULONG NumEnumerators;

    // The directory. This field is for internal use only.
    PVOID Directory;

} RTL_DYNAMIC_HASH_TABLE, *PRTL_DYNAMIC_HASH_TABLE;


//
// Inline functions first.
//
#if !defined(MIDL_PASS) && !defined(SORTPP_PASS)

#if (NTDDI_VERSION >= NTDDI_WIN7)
FORCEINLINE
VOID
RtlInitHashTableContext(
    _Inout_ PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context
    )
{
    Context->ChainHead = NULL;
    Context->PrevLinkage = NULL;
}
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
FORCEINLINE
VOID
RtlInitHashTableContextFromEnumerator(
    _Inout_ PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context,
    _In_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    )
{
    Context->ChainHead = Enumerator->ChainHead;
    Context->PrevLinkage = Enumerator->HashEntry.Linkage.Blink;
}
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
FORCEINLINE
void
RtlReleaseHashTableContext(
    _Inout_ PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context
    )
{
    UNREFERENCED_PARAMETER(Context);
    return;
}
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
FORCEINLINE
ULONG
RtlTotalBucketsHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable
    )
{
    return HashTable->TableSize;
}
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
FORCEINLINE
ULONG
RtlNonEmptyBucketsHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable
    )
{
    return HashTable->NonEmptyBuckets;
}
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
FORCEINLINE
ULONG
RtlEmptyBucketsHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable
    )
{
    return HashTable->TableSize - HashTable->NonEmptyBuckets;
}
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
FORCEINLINE
ULONG
RtlTotalEntriesHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable
    )
{
    return HashTable->NumEntries;
}
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
FORCEINLINE
ULONG
RtlActiveEnumeratorsHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable
    )
{
    return HashTable->NumEnumerators;
}
#endif

#endif // !defined(MIDL_PASS) && !defined(SORTPP_PASS)

//
// Almost all the hash functions take in a Context.
// If a valid context is passed in, it will be used
// in executing the operation if possible. If a
// blank context is passed in, it will be initialized
// appropriately.
//

#if (NTDDI_VERSION >= NTDDI_WIN7)
_Must_inspect_result_
_Success_(return != 0)
NTSYSAPI
BOOLEAN
NTAPI
RtlCreateHashTable(
    _Inout_ _When_(NULL == *HashTable, _At_(*HashTable, __drv_allocatesMem(Mem)))
        PRTL_DYNAMIC_HASH_TABLE *HashTable,
    _In_ ULONG Shift,
    _Reserved_ ULONG Flags
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN8)
_Must_inspect_result_
_Success_(return != 0)
NTSYSAPI
BOOLEAN
NTAPI
RtlCreateHashTableEx(
    _Inout_ _When_(NULL == *HashTable, _At_(*HashTable, __drv_allocatesMem(Mem)))
        PRTL_DYNAMIC_HASH_TABLE *HashTable,
    _In_ ULONG InitialSize,
    _In_ ULONG Shift,
    _Reserved_ ULONG Flags
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTSYSAPI
VOID
NTAPI
RtlDeleteHashTable(
    _In_ _When_((HashTable->Flags & RTL_HASH_ALLOCATED_HEADER), __drv_freesMem(Mem) _Post_invalid_)
        PRTL_DYNAMIC_HASH_TABLE HashTable
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTSYSAPI
BOOLEAN
NTAPI
RtlInsertEntryHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _In_ __drv_aliasesMem PRTL_DYNAMIC_HASH_TABLE_ENTRY Entry,
    _In_ ULONG_PTR Signature,
    _Inout_opt_ PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTSYSAPI
BOOLEAN
NTAPI
RtlRemoveEntryHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _In_ PRTL_DYNAMIC_HASH_TABLE_ENTRY Entry,
    _Inout_opt_ PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
_Must_inspect_result_
NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlLookupEntryHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _In_ ULONG_PTR Signature,
    _Out_opt_ PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
_Must_inspect_result_
NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlGetNextEntryHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _In_ PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTSYSAPI
BOOLEAN
NTAPI
RtlInitEnumerationHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _Out_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
_Must_inspect_result_
NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlEnumerateEntryHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _Inout_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTSYSAPI
VOID
NTAPI
RtlEndEnumerationHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _Inout_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTSYSAPI
BOOLEAN
NTAPI
RtlInitWeakEnumerationHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _Out_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
_Must_inspect_result_
NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlWeaklyEnumerateEntryHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _Inout_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTSYSAPI
VOID
NTAPI
RtlEndWeakEnumerationHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _Inout_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
NTSYSAPI
BOOLEAN
NTAPI
RtlInitStrongEnumerationHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _Out_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
_Must_inspect_result_
NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlStronglyEnumerateEntryHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _Inout_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
NTSYSAPI
VOID
NTAPI
RtlEndStrongEnumerationHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _Inout_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WIN7)
NTSYSAPI
BOOLEAN
NTAPI
RtlExpandHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTSYSAPI
BOOLEAN
NTAPI
RtlContractHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable
    );
#endif




#if defined (_MSC_VER) && ( _MSC_VER >= 900 )

PVOID
_ReturnAddress (
    VOID
    );

#pragma intrinsic(_ReturnAddress)

#endif


#if (defined(_M_AMD64) || defined(_M_IA64)) && !defined(_REALLY_GET_CALLERS_CALLER_)

#define RtlGetCallersAddress(CallersAddress, CallersCaller) \
    *CallersAddress = (PVOID)_ReturnAddress(); \
    *CallersCaller = NULL;

#else

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
VOID
NTAPI
RtlGetCallersAddress(
    _Out_ PVOID *CallersAddress,
    _Out_ PVOID *CallersCaller
    );

#endif

#endif


#if (NTDDI_VERSION >= NTDDI_WIN2K)

//
// Reserve byte 1 of the RtlWalkFrameChain flags for
// specifying a number of frames to skip.
//

#define RTL_STACK_WALKING_MODE_FRAMES_TO_SKIP_SHIFT     8

NTSYSAPI
ULONG
NTAPI
RtlWalkFrameChain (
    _Out_writes_(Count - (Flags >> RTL_STACK_WALKING_MODE_FRAMES_TO_SKIP_SHIFT)) PVOID *Callers,
    _In_ ULONG Count,
    _In_ ULONG Flags
    );
#endif


#if !defined(MIDL_PASS)
#if (NTDDI_VERSION >= NTDDI_WIN7)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
ULONG64
NTAPI
RtlGetEnabledExtendedFeatures(
    _In_ ULONG64 FeatureMask
    );
#endif
#endif

#if !defined(MIDL_PASS)
//
// 

#if (NTDDI_VERSION >= NTDDI_WIN10_RS3)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
ULONG64
NTAPI
RtlGetEnabledExtendedAndSupervisorFeatures(
    _In_ ULONG64 FeatureMask
    );

_IRQL_requires_same_
_Ret_maybenull_
_Success_(return != NULL)
NTSYSAPI
PVOID
NTAPI
RtlLocateSupervisorFeature(
    _In_ PXSAVE_AREA_HEADER XStateHeader,
    _In_range_(XSTATE_AVX, MAXIMUM_XSTATE_FEATURES - 1) ULONG FeatureId,
    _Out_opt_ PULONG Length
    );
#endif
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlCharToInteger (
    _In_z_ PCSZ String,
    _In_opt_ ULONG Base,
    _Out_ PULONG Value
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
VOID
NTAPI
RtlCopyString(
    _Out_ PSTRING DestinationString,
    _In_opt_ const STRING * SourceString
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
CHAR
NTAPI
RtlUpperChar (
    _In_ CHAR Character
    );
#endif



#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
LONG
NTAPI
RtlCompareString(
    _In_ const STRING * String1,
    _In_ const STRING * String2,
    _In_ BOOLEAN CaseInSensitive
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlEqualString(
    _In_ const STRING * String1,
    _In_ const STRING * String2,
    _In_ BOOLEAN CaseInSensitive
    );
#endif



#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlUpperString(
    _Inout_ PSTRING DestinationString,
    _In_ const STRING * SourceString
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlPrefixUnicodeString(
    _In_ PCUNICODE_STRING String1,
    _In_ PCUNICODE_STRING String2,
    _In_ BOOLEAN CaseInSensitive
    );
#endif

#if (NTDDI_VERSION >= NTDDI_THRESHOLD)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlSuffixUnicodeString(
    _In_ PCUNICODE_STRING String1,
    _In_ PCUNICODE_STRING String2,
    _In_ BOOLEAN CaseInSensitive
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(PASSIVE_LEVEL)
_When_(AllocateDestinationString, _Must_inspect_result_)
NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeString(
    _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
    _When_(!AllocateDestinationString, _Inout_)
        PUNICODE_STRING DestinationString,
    _In_ PCUNICODE_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );
#endif

#if !defined(MIDL_PASS)
#if defined(_AMD64_) || defined(_ARM_) || defined(_ARM64_) || defined(_IA64_) || defined(_CHPE_X86_ARM64_)
//
// Large Integer divide - 64-bits / 64-bits -> 64-bits
//

DECLSPEC_DEPRECATED_DDK         // Use native __int64 math
__drv_preferredFunction("compiler support for 64 bit", "Obsolete")
__inline
LARGE_INTEGER
NTAPI_INLINE
RtlLargeIntegerDivide (
    _In_ LARGE_INTEGER Dividend,
    _In_ LARGE_INTEGER Divisor,
    _Out_opt_ PLARGE_INTEGER Remainder
    )
{
    LARGE_INTEGER Quotient;

    Quotient.QuadPart = Dividend.QuadPart / Divisor.QuadPart;
    if (ARGUMENT_PRESENT(Remainder)) {
        Remainder->QuadPart = Dividend.QuadPart % Divisor.QuadPart;
    }

    return Quotient;
}

#else
//
// Large Integer divide - 64-bits / 64-bits -> 64-bits
//

#if (NTDDI_VERSION >= NTDDI_WIN2K)
DECLSPEC_DEPRECATED_DDK         // Use native __int64 math
__drv_preferredFunction("compiler support for 64 bit", "Obsolete")
NTSYSAPI
LARGE_INTEGER
NTAPI
RtlLargeIntegerDivide (
    _In_ LARGE_INTEGER Dividend,
    _In_ LARGE_INTEGER Divisor,
    _Out_opt_ PLARGE_INTEGER Remainder
    );
#endif

#endif // defined(_AMD64_) || defined(_ARM_) || defined(_ARM64_) || defined(_IA64_)
#endif // !defined(MIDL_PASS)


//
// BOOLEAN
// RtlEqualLuid(
//      PLUID L1,
//      PLUID L2
//      );

#define RtlEqualLuid(L1, L2) (((L1)->LowPart == (L2)->LowPart) && \
                              ((L1)->HighPart  == (L2)->HighPart))

//
// BOOLEAN
// RtlIsZeroLuid(
//      PLUID L1
//      );
//
#define RtlIsZeroLuid(L1) ((BOOLEAN) (((L1)->LowPart | (L1)->HighPart) == 0))



#if !defined(MIDL_PASS)

FORCEINLINE
LUID
NTAPI_INLINE
RtlConvertLongToLuid(
    _In_ LONG Long
    )
{
    LUID TempLuid;
    LARGE_INTEGER TempLi;

    TempLi.QuadPart = Long;
    TempLuid.LowPart = TempLi.u.LowPart;
    TempLuid.HighPart = TempLi.u.HighPart;
    return(TempLuid);
}

FORCEINLINE
LUID
NTAPI_INLINE
RtlConvertUlongToLuid(
    _In_ ULONG Ulong
    )
{
    LUID TempLuid;

    TempLuid.LowPart = Ulong;
    TempLuid.HighPart = 0;
    return(TempLuid);
}
#endif


#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlMapGenericMask(
    _Inout_ PACCESS_MASK AccessMask,
    _In_ PGENERIC_MAPPING GenericMapping
    );
#endif

//
// Routine for converting from a volume device object to a DOS name.
//

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_When_(NTDDI_VERSION >= NTDDI_WINXP,
    __drv_preferredFunction("IoVolumeDeviceToDosName",
    "Obsolete on WINXP and above"))
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlVolumeDeviceToDosName(
    _In_ PVOID VolumeDeviceObject,
    _Out_ PUNICODE_STRING DosName
    );
#endif

//

NTSYSAPI
ULONG
NTAPI
DbgPrompt (
    _In_z_ PCCH Prompt,
    _Out_writes_bytes_(Length) PCH Response,
    _In_ ULONG Length
    );

//

#if (NTDDI_VERSION >= NTDDI_VISTA)

NTSYSAPI
BOOLEAN
NTAPI
RtlGetProductInfo(
    _In_  ULONG  OSMajorVersion,
    _In_  ULONG  OSMinorVersion,
    _In_  ULONG  SpMajorVersion,
    _In_  ULONG  SpMinorVersion,
    _Out_ PULONG ReturnedProductType
    );

#endif



#if (NTDDI_VERSION >= NTDDI_WIN8) && !defined(MIDL_PASS)

NTSYSAPI
NTSTATUS
NTAPI
RtlIsUntrustedObject (
    _In_opt_ HANDLE Handle,
    _In_opt_ PVOID Object,
    _Out_ PBOOLEAN UntrustedObject
    );

#endif // (NTDDI_VERSION >= NTDDI_WIN8) && !defined(MIDL_PASS)


#if (NTDDI_VERSION >= NTDDI_WIN10_RS1)
NTSYSAPI
ULONG
NTAPI
RtlGetActiveConsoleId(
    VOID
    );
#endif // NTDDI_VERSION >= NTDDI_RS1

#if (NTDDI_VERSION >= NTDDI_WIN10_RS1)
NTSYSAPI
ULONGLONG
NTAPI
RtlGetConsoleSessionForegroundProcessId(
    VOID
    );
#endif // NTDDI_VERSION >= NTDDI_WIN10_RS1

#if (NTDDI_VERSION >= NTDDI_WIN10_RS1)
NTSYSAPI
ULONG
NTAPI
RtlGetSuiteMask(
    VOID
    );
#endif // NTDDI_VERSION >= NTDDI_WIN10_RS1

#if (NTDDI_VERSION >= NTDDI_WIN10_RS1)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlIsMultiSessionSku (
    VOID
    );
#endif // NTDDI_VERSION >= NTDDI_WIN10_RS1

#if (NTDDI_VERSION >= NTDDI_WIN10_RS4)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlIsStateSeparationEnabled (
    VOID
    );

typedef enum _STATE_LOCATION_TYPE {
    LocationTypeRegistry = 0,
    LocationTypeFileSystem = 1,
    LocationTypeMaximum = 2
} STATE_LOCATION_TYPE;

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlGetPersistedStateLocation (
    _In_                        PCWSTR    SourceID,
    _In_opt_                    PCWSTR    CustomValue,
    _In_opt_                    PCWSTR    DefaultPath,
    _In_                        STATE_LOCATION_TYPE StateLocationType,
    _Out_writes_bytes_to_opt_(BufferLengthIn, *BufferLengthOut)
                                PWCHAR   TargetPath,
    _In_                        ULONG    BufferLengthIn,
    _Out_opt_                   PULONG   BufferLengthOut
    );

#endif // NTDDI_VERSION >= NTDDI_WIN10_RS3

#if (NTDDI_VERSION >= NTDDI_WIN10_RS1)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlIsMultiUsersInSessionSku (
    VOID
    );
#endif // NTDDI_VERSION >= NTDDI_WIN10_RS1


#if (NTDDI_VERSION >= NTDDI_WIN10_RS1)
NTSYSAPI
BOOLEAN
NTAPI
RtlGetNtProductType(
    _Out_ PNT_PRODUCT_TYPE NtProductType
    );
#endif // NTDDI_VERSION >= NTDDI_WIN10_RS1

#if (NTDDI_VERSION >= NTDDI_WIN10_RS2)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
PCWSTR
NTAPI
RtlGetNtSystemRoot (
    VOID
    );
#endif // NTDDI_VERSION >= NTDDI_WIN10_RS2


//
// Flush routines for DAX mapped files
//

#if (NTDDI_VERSION >= NTDDI_WIN10_RS2) && defined(_AMD64_)
typedef struct _NV_MEMORY_RANGE {
    VOID *BaseAddress;
    SIZE_T Length;
} NV_MEMORY_RANGE, *PNV_MEMORY_RANGE;

//
// Flags for RtlFlushNonVolatileMemory and RtlFlushNonVolatileMemoryRanges
//

#define FLUSH_NV_MEMORY_IN_FLAG_NO_DRAIN    (0x00000001)

//
// Default token is used to call the flush and drain routines without the
// RtlGetNonVolatileToken call, for callers who know the details about the
// region they are flushing.
//

#define FLUSH_NV_MEMORY_DEFAULT_TOKEN       (ULONG_PTR)(-1)

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlGetNonVolatileToken (
    _In_reads_bytes_(Size) PVOID NvBuffer,
    _In_ SIZE_T Size,
    _Outptr_ PVOID *NvToken
    );

_IRQL_requires_max_(DPC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlFreeNonVolatileToken (
    _In_ PVOID NvToken
    );

_IRQL_requires_max_(DPC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlFlushNonVolatileMemory (
    _In_ PVOID NvToken,
    _In_reads_bytes_(Size) PVOID NvBuffer,
    _In_ SIZE_T Size,
    _In_ ULONG Flags
    );

_IRQL_requires_max_(DPC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlDrainNonVolatileFlush (
    _In_ PVOID NvToken
    );

_IRQL_requires_max_(DPC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlWriteNonVolatileMemory (
    _In_ PVOID NvToken,
    _Out_writes_bytes_(Size) VOID UNALIGNED *NvDestination,
    _In_reads_bytes_(Size) VOID UNALIGNED *Source,
    _In_ SIZE_T Size,
    _In_ ULONG Flags
    );

_IRQL_requires_max_(DPC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlFlushNonVolatileMemoryRanges (
    _In_ PVOID NvToken,
    _In_reads_(NumRanges) PNV_MEMORY_RANGE NvRanges,
    _In_ SIZE_T NumRanges,
    _In_ ULONG Flags
    );
#endif // (NTDDI_VERSION >= NTDDI_RS2) && defined(_AMD64_)


//
// Correlation Vector Routines.
//

#if (NTDDI_VERSION >= NTDDI_WIN10_RS2)


#define RTL_CORRELATION_VECTOR_STRING_LENGTH 129
#define RTL_CORRELATION_VECTOR_VERSION_1 ((CHAR)1)
#define RTL_CORRELATION_VECTOR_VERSION_2 ((CHAR)2)
#define RTL_CORRELATION_VECTOR_VERSION_CURRENT RTL_CORRELATION_VECTOR_VERSION_2

#define RTL_CORRELATION_VECTOR_V1_PREFIX_LENGTH (16)
#define RTL_CORRELATION_VECTOR_V1_LENGTH (64)

#define RTL_CORRELATION_VECTOR_V2_PREFIX_LENGTH (22)
#define RTL_CORRELATION_VECTOR_V2_LENGTH (128)

typedef struct CORRELATION_VECTOR {
    CHAR Version;
    CHAR Vector[RTL_CORRELATION_VECTOR_STRING_LENGTH];
} CORRELATION_VECTOR;

typedef CORRELATION_VECTOR *PCORRELATION_VECTOR;

#define TraceLoggingCORRELATION_VECTOR(cv) TraceLoggingString((cv).Vector, "__TlgCV__")

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeCorrelationVector(
    _Out_ PCORRELATION_VECTOR CorrelationVector,
    _In_  int Version,
    _In_opt_  const GUID * Guid
    );


NTSYSAPI
NTSTATUS
NTAPI
RtlIncrementCorrelationVector(
    _Inout_ PCORRELATION_VECTOR CorrelationVector
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlExtendCorrelationVector(
    _Inout_ PCORRELATION_VECTOR CorrelationVector
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlValidateCorrelationVector(
    _In_ PCORRELATION_VECTOR Vector
    );

#endif // NTDDI_VERSION >= NTDDI_RS2


#if (NTDDI_VERSION >= NTDDI_WIN10_RS4)

typedef struct _CUSTOM_SYSTEM_EVENT_TRIGGER_CONFIG {
    //
    // Size of the structure in bytes
    //
    ULONG Size;

    //
    // Guid used to identify background task to trigger
    //
    PCWSTR TriggerId;

} CUSTOM_SYSTEM_EVENT_TRIGGER_CONFIG, *PCUSTOM_SYSTEM_EVENT_TRIGGER_CONFIG;

#if !defined(MIDL_PASS)
FORCEINLINE
VOID
CUSTOM_SYSTEM_EVENT_TRIGGER_INIT(
    _Out_    PCUSTOM_SYSTEM_EVENT_TRIGGER_CONFIG Config,
    _In_opt_ PCWSTR TriggerId
    )
{
    RtlZeroMemory(Config, sizeof(CUSTOM_SYSTEM_EVENT_TRIGGER_CONFIG));

    Config->Size = sizeof(CUSTOM_SYSTEM_EVENT_TRIGGER_CONFIG);
    Config->TriggerId = TriggerId;
}
#endif // !defined(MIDL_PASS)

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
RtlRaiseCustomSystemEventTrigger(
    _In_ PCUSTOM_SYSTEM_EVENT_TRIGGER_CONFIG TriggerConfig
    );

#endif // NTDDI_VERSION >= NTDDI_WIN10_RS4

//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers.
//

#define DEVICE_TYPE ULONG

#define FILE_DEVICE_BEEP                0x00000001
#define FILE_DEVICE_CD_ROM              0x00000002
#define FILE_DEVICE_CD_ROM_FILE_SYSTEM  0x00000003
#define FILE_DEVICE_CONTROLLER          0x00000004
#define FILE_DEVICE_DATALINK            0x00000005
#define FILE_DEVICE_DFS                 0x00000006
#define FILE_DEVICE_DISK                0x00000007
#define FILE_DEVICE_DISK_FILE_SYSTEM    0x00000008
#define FILE_DEVICE_FILE_SYSTEM         0x00000009
#define FILE_DEVICE_INPORT_PORT         0x0000000a
#define FILE_DEVICE_KEYBOARD            0x0000000b
#define FILE_DEVICE_MAILSLOT            0x0000000c
#define FILE_DEVICE_MIDI_IN             0x0000000d
#define FILE_DEVICE_MIDI_OUT            0x0000000e
#define FILE_DEVICE_MOUSE               0x0000000f
#define FILE_DEVICE_MULTI_UNC_PROVIDER  0x00000010
#define FILE_DEVICE_NAMED_PIPE          0x00000011
#define FILE_DEVICE_NETWORK             0x00000012
#define FILE_DEVICE_NETWORK_BROWSER     0x00000013
#define FILE_DEVICE_NETWORK_FILE_SYSTEM 0x00000014
#define FILE_DEVICE_NULL                0x00000015
#define FILE_DEVICE_PARALLEL_PORT       0x00000016
#define FILE_DEVICE_PHYSICAL_NETCARD    0x00000017
#define FILE_DEVICE_PRINTER             0x00000018
#define FILE_DEVICE_SCANNER             0x00000019
#define FILE_DEVICE_SERIAL_MOUSE_PORT   0x0000001a
#define FILE_DEVICE_SERIAL_PORT         0x0000001b
#define FILE_DEVICE_SCREEN              0x0000001c
#define FILE_DEVICE_SOUND               0x0000001d
#define FILE_DEVICE_STREAMS             0x0000001e
#define FILE_DEVICE_TAPE                0x0000001f
#define FILE_DEVICE_TAPE_FILE_SYSTEM    0x00000020
#define FILE_DEVICE_TRANSPORT           0x00000021
#define FILE_DEVICE_UNKNOWN             0x00000022
#define FILE_DEVICE_VIDEO               0x00000023
#define FILE_DEVICE_VIRTUAL_DISK        0x00000024
#define FILE_DEVICE_WAVE_IN             0x00000025
#define FILE_DEVICE_WAVE_OUT            0x00000026
#define FILE_DEVICE_8042_PORT           0x00000027
#define FILE_DEVICE_NETWORK_REDIRECTOR  0x00000028
#define FILE_DEVICE_BATTERY             0x00000029
#define FILE_DEVICE_BUS_EXTENDER        0x0000002a
#define FILE_DEVICE_MODEM               0x0000002b
#define FILE_DEVICE_VDM                 0x0000002c
#define FILE_DEVICE_MASS_STORAGE        0x0000002d
#define FILE_DEVICE_SMB                 0x0000002e
#define FILE_DEVICE_KS                  0x0000002f
#define FILE_DEVICE_CHANGER             0x00000030
#define FILE_DEVICE_SMARTCARD           0x00000031
#define FILE_DEVICE_ACPI                0x00000032
#define FILE_DEVICE_DVD                 0x00000033
#define FILE_DEVICE_FULLSCREEN_VIDEO    0x00000034
#define FILE_DEVICE_DFS_FILE_SYSTEM     0x00000035
#define FILE_DEVICE_DFS_VOLUME          0x00000036
#define FILE_DEVICE_SERENUM             0x00000037
#define FILE_DEVICE_TERMSRV             0x00000038
#define FILE_DEVICE_KSEC                0x00000039
#define FILE_DEVICE_FIPS                0x0000003A
#define FILE_DEVICE_INFINIBAND          0x0000003B
#define FILE_DEVICE_VMBUS               0x0000003E
#define FILE_DEVICE_CRYPT_PROVIDER      0x0000003F
#define FILE_DEVICE_WPD                 0x00000040
#define FILE_DEVICE_BLUETOOTH           0x00000041
#define FILE_DEVICE_MT_COMPOSITE        0x00000042
#define FILE_DEVICE_MT_TRANSPORT        0x00000043
#define FILE_DEVICE_BIOMETRIC           0x00000044
#define FILE_DEVICE_PMI                 0x00000045
#define FILE_DEVICE_EHSTOR              0x00000046
#define FILE_DEVICE_DEVAPI              0x00000047
#define FILE_DEVICE_GPIO                0x00000048
#define FILE_DEVICE_USBEX               0x00000049
#define FILE_DEVICE_CONSOLE             0x00000050
#define FILE_DEVICE_NFP                 0x00000051
#define FILE_DEVICE_SYSENV              0x00000052
#define FILE_DEVICE_VIRTUAL_BLOCK       0x00000053
#define FILE_DEVICE_POINT_OF_SERVICE    0x00000054
#define FILE_DEVICE_STORAGE_REPLICATION 0x00000055
#define FILE_DEVICE_TRUST_ENV           0x00000056
#define FILE_DEVICE_UCM                 0x00000057
#define FILE_DEVICE_UCMTCPCI            0x00000058
#define FILE_DEVICE_PERSISTENT_MEMORY   0x00000059
#define FILE_DEVICE_NVDIMM              0x0000005a
#define FILE_DEVICE_HOLOGRAPHIC         0x0000005b
#define FILE_DEVICE_SDFXHCI             0x0000005c

//
// Macro definition for defining IOCTL and FSCTL function control codes.  Note
// that function codes 0-2047 are reserved for Microsoft Corporation, and
// 2048-4095 are reserved for customers.
//

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

//
// Macro to extract device type out of the device io control code
//
#define DEVICE_TYPE_FROM_CTL_CODE(ctrlCode)     (((ULONG)(ctrlCode & 0xffff0000)) >> 16)

//
// Macro to extract buffering method out of the device io control code
//
#define METHOD_FROM_CTL_CODE(ctrlCode)          ((ULONG)(ctrlCode & 3))

//
// Define the method codes for how buffers are passed for I/O and FS controls
//

#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3

//
// Define some easier to comprehend aliases:
//   METHOD_DIRECT_TO_HARDWARE (writes, aka METHOD_IN_DIRECT)
//   METHOD_DIRECT_FROM_HARDWARE (reads, aka METHOD_OUT_DIRECT)
//

#define METHOD_DIRECT_TO_HARDWARE       METHOD_IN_DIRECT
#define METHOD_DIRECT_FROM_HARDWARE     METHOD_OUT_DIRECT

//
// Define the access check value for any access
//
//
// The FILE_READ_ACCESS and FILE_WRITE_ACCESS constants are also defined in
// ntioapi.h as FILE_READ_DATA and FILE_WRITE_DATA. The values for these
// constants *MUST* always be in sync.
//
//
// FILE_SPECIAL_ACCESS is checked by the NT I/O system the same as FILE_ANY_ACCESS.
// The file systems, however, may add additional access checks for I/O and FS controls
// that use this value.
//


#define FILE_ANY_ACCESS                 0
#define FILE_SPECIAL_ACCESS    (FILE_ANY_ACCESS)
#define FILE_READ_ACCESS          ( 0x0001 )    // file & pipe
#define FILE_WRITE_ACCESS         ( 0x0002 )    // file & pipe


#if (NTDDI_VERSION >= NTDDI_WIN7)
#define FILE_CHARACTERISTICS_EXPECT_ORDERLY_REMOVAL_EX     0x00004000
#define FILE_CHARACTERISTICS_EXPECT_SURPRISE_REMOVAL_EX    0x00008000
#define FILE_CHARACTERISTICS_REMOVAL_POLICY_MASK_EX \
    (FILE_CHARACTERISTICS_EXPECT_ORDERLY_REMOVAL_EX | \
     FILE_CHARACTERISTICS_EXPECT_SURPRISE_REMOVAL_EX)

#define FILE_CHARACTERISTICS_EXPECT_ORDERLY_REMOVAL_DEPRECATED 0x00000200
#define FILE_CHARACTERISTICS_EXPECT_SURPRISE_REMOVAL_DEPRECATED 0x00000300
#define FILE_CHARACTERISTICS_REMOVAL_POLICY_MASK_DEPRECATED 0x00000300

#else
#define FILE_CHARACTERISTICS_EXPECT_ORDERLY_REMOVAL     0x00000200
#define FILE_CHARACTERISTICS_EXPECT_SURPRISE_REMOVAL    0x00000300
#define FILE_CHARACTERISTICS_REMOVAL_POLICY_MASK        0x00000300

#define FILE_CHARACTERISTICS_EXPECT_ORDERLY_REMOVAL_EX FILE_CHARACTERISTICS_EXPECT_ORDERLY_REMOVAL
#define FILE_CHARACTERISTICS_EXPECT_SURPRISE_REMOVAL_EX FILE_CHARACTERISTICS_EXPECT_SURPRISE_REMOVAL
#define FILE_CHARACTERISTICS_REMOVAL_POLICY_MASK_EX FILE_CHARACTERISTICS_REMOVAL_POLICY_MASK
#endif

//
// flags specified here will be propagated up and down a device stack
// after FDO and all filter devices are added, but before the device
// stack is started
//

#define FILE_CHARACTERISTICS_PROPAGATED (   FILE_REMOVABLE_MEDIA   | \
                                            FILE_READ_ONLY_DEVICE  | \
                                            FILE_FLOPPY_DISKETTE   | \
                                            FILE_WRITE_ONCE_MEDIA  | \
                                            FILE_DEVICE_SECURE_OPEN | \
                                            FILE_CHARACTERISTIC_CSV | \
                                            FILE_PORTABLE_DEVICE )


typedef struct _FILE_ALIGNMENT_INFORMATION {
    ULONG AlignmentRequirement;
} FILE_ALIGNMENT_INFORMATION, *PFILE_ALIGNMENT_INFORMATION;

//
// This is also used for FileNormalizedNameInformation
//

typedef struct _FILE_NAME_INFORMATION {
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;

typedef struct _FILE_ATTRIBUTE_TAG_INFORMATION {
    ULONG FileAttributes;
    ULONG ReparseTag;
} FILE_ATTRIBUTE_TAG_INFORMATION, *PFILE_ATTRIBUTE_TAG_INFORMATION;

typedef struct _FILE_DISPOSITION_INFORMATION {
    BOOLEAN DeleteFile;
} FILE_DISPOSITION_INFORMATION, *PFILE_DISPOSITION_INFORMATION;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN10_RS1)
#define FILE_DISPOSITION_DO_NOT_DELETE              0x00000000
#define FILE_DISPOSITION_DELETE                     0x00000001
#define FILE_DISPOSITION_POSIX_SEMANTICS            0x00000002
#define FILE_DISPOSITION_FORCE_IMAGE_SECTION_CHECK  0x00000004
#define FILE_DISPOSITION_ON_CLOSE                   0x00000008

typedef struct _FILE_DISPOSITION_INFORMATION_EX {
    ULONG Flags;
} FILE_DISPOSITION_INFORMATION_EX, *PFILE_DISPOSITION_INFORMATION_EX;
#endif

typedef struct _FILE_END_OF_FILE_INFORMATION {
    LARGE_INTEGER EndOfFile;
} FILE_END_OF_FILE_INFORMATION, *PFILE_END_OF_FILE_INFORMATION;

typedef struct _FILE_VALID_DATA_LENGTH_INFORMATION {
    LARGE_INTEGER ValidDataLength;
} FILE_VALID_DATA_LENGTH_INFORMATION, *PFILE_VALID_DATA_LENGTH_INFORMATION;

//
// NtQuery[Set]VolumeInformationFile types:
//
//  FILE_FS_LABEL_INFORMATION
//  FILE_FS_VOLUME_INFORMATION
//  FILE_FS_SIZE_INFORMATION
//  FILE_FS_FULL_SIZE_INFORMATION
//  FILE_FS_SECTOR_SIZE_INFORMATION
//  FILE_FS_OBJECTID_INFORMATION
//  FILE_FS_DEVICE_INFORMATION
//  FILE_FS_ATTRIBUTE_INFORMATION
//  FILE_FS_DRIVER_PATH_INFORMATION
//  FILE_FS_VOLUME_FLAGS_INFORMATION
//  FILE_FS_CONTROL_INFORMATION
//

typedef struct _FILE_FS_LABEL_INFORMATION {
    ULONG VolumeLabelLength;
    WCHAR VolumeLabel[1];
} FILE_FS_LABEL_INFORMATION, *PFILE_FS_LABEL_INFORMATION;

typedef struct _FILE_FS_VOLUME_INFORMATION {
    LARGE_INTEGER VolumeCreationTime;
    ULONG VolumeSerialNumber;
    ULONG VolumeLabelLength;
    BOOLEAN SupportsObjects;
    WCHAR VolumeLabel[1];
} FILE_FS_VOLUME_INFORMATION, *PFILE_FS_VOLUME_INFORMATION;

typedef struct _FILE_FS_SIZE_INFORMATION {
    LARGE_INTEGER TotalAllocationUnits;
    LARGE_INTEGER AvailableAllocationUnits;
    ULONG SectorsPerAllocationUnit;
    ULONG BytesPerSector;
} FILE_FS_SIZE_INFORMATION, *PFILE_FS_SIZE_INFORMATION;

typedef struct _FILE_FS_FULL_SIZE_INFORMATION {
    LARGE_INTEGER TotalAllocationUnits;
    LARGE_INTEGER CallerAvailableAllocationUnits;
    LARGE_INTEGER ActualAvailableAllocationUnits;
    ULONG SectorsPerAllocationUnit;
    ULONG BytesPerSector;
} FILE_FS_FULL_SIZE_INFORMATION, *PFILE_FS_FULL_SIZE_INFORMATION;

#if (_WIN32_WINNT >= _WIN32_WINNT_WINTHRESHOLD)
typedef struct _FILE_FS_METADATA_SIZE_INFORMATION {
    LARGE_INTEGER TotalMetadataAllocationUnits;
    ULONG SectorsPerAllocationUnit;
    ULONG BytesPerSector;
} FILE_FS_METADATA_SIZE_INFORMATION, *PFILE_FS_METADATA_SIZE_INFORMATION;
#endif

//
//  Flag definitions for FILE_FS_SECTOR_SIZE_INFORMATION structure
//

//
//  If this flag is set then the partition is correctly aligned with the
//  physical sector size of the device for optimial performance.
//

//
//  If SET the device has EVEN alignment
//  If NOT SET the device has ODD alignment
//

#define SSINFO_FLAGS_ALIGNED_DEVICE                 0x00000001

//
//  If SET the partition is properly aligned with the alignment of the device
//  If NOT SET the parition is NOT properly aligned with the partition which can
//      lead to performance issues on this device
//

#define SSINFO_FLAGS_PARTITION_ALIGNED_ON_DEVICE    0x00000002

//
//  If SET the device reports that it does not have a seek penalty (usually this means an SSD)
//  If NOT SET the device reports it does have a seek penalty or the property could not be queried
//

#define SSINFO_FLAGS_NO_SEEK_PENALTY                0x00000004

//
//  If SET the device supports the TRIM operation
//  If NOT SET the device does not support trim operations or the property could not be queried
//

#define SSINFO_FLAGS_TRIM_ENABLED                   0x00000008

//
//  If SET this is a byte addressable storage device (i.e. a storage class memory or a persistent
//  memory device).
//  If NOT SET this is not a byte addressable storage device.
//
//  NOTE: File systems like NTFS and ReFS can support byte addressable mode where a file is directly
//  mapped to the physical address that the storage device expose, if the hardware supports
//  byte addressing (which is what this flag indicates) and the volume type allows byte
//  addressing.  To know if the volume type and the file system supports byte addressing, check
//  that FileSystemAttributes of FileFsAttributeInformation has FILE_DAX_VOLUME set.
//

#define SSINFO_FLAGS_BYTE_ADDRESSABLE               0x00000010


//
//  If this value is set for the Sector and Parition alignment
//  fields then it means the alignment is not known and the
//  alignment flags have no meaning
//
#define SSINFO_OFFSET_UNKNOWN (0xffffffff)

typedef struct _FILE_FS_SECTOR_SIZE_INFORMATION {
    ULONG LogicalBytesPerSector;
    ULONG PhysicalBytesPerSectorForAtomicity;
    ULONG PhysicalBytesPerSectorForPerformance;
    ULONG FileSystemEffectivePhysicalBytesPerSectorForAtomicity;
    ULONG Flags;
    ULONG ByteOffsetForSectorAlignment;
    ULONG ByteOffsetForPartitionAlignment;
} FILE_FS_SECTOR_SIZE_INFORMATION, *PFILE_FS_SECTOR_SIZE_INFORMATION;

typedef struct _FILE_FS_OBJECTID_INFORMATION {
    UCHAR ObjectId[16];
    UCHAR ExtendedInfo[48];
} FILE_FS_OBJECTID_INFORMATION, *PFILE_FS_OBJECTID_INFORMATION;

//
// AVIO IOCTLS.
//

#define IOCTL_AVIO_ALLOCATE_STREAM      CTL_CODE(FILE_DEVICE_AVIO, 1, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_AVIO_FREE_STREAM          CTL_CODE(FILE_DEVICE_AVIO, 2, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_AVIO_MODIFY_STREAM        CTL_CODE(FILE_DEVICE_AVIO, 3, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)


//
// Define types of bus information.
//

typedef enum _BUS_DATA_TYPE {
    ConfigurationSpaceUndefined = -1,
    Cmos,
    EisaConfiguration,
    Pos,
    CbusConfiguration,
    PCIConfiguration,
    VMEConfiguration,
    NuBusConfiguration,
    PCMCIAConfiguration,
    MPIConfiguration,
    MPSAConfiguration,
    PNPISAConfiguration,
    SgiInternalConfiguration,
    MaximumBusDataType
} BUS_DATA_TYPE, *PBUS_DATA_TYPE;

typedef struct _KEY_NAME_INFORMATION {
    ULONG   NameLength;
    WCHAR   Name[1];            // Variable length string
} KEY_NAME_INFORMATION, *PKEY_NAME_INFORMATION;

typedef struct _KEY_CACHED_INFORMATION {
    LARGE_INTEGER LastWriteTime;
    ULONG   TitleIndex;
    ULONG   SubKeys;
    ULONG   MaxNameLen;
    ULONG   Values;
    ULONG   MaxValueNameLen;
    ULONG   MaxValueDataLen;
    ULONG   NameLength;
} KEY_CACHED_INFORMATION, *PKEY_CACHED_INFORMATION;

typedef struct _KEY_VIRTUALIZATION_INFORMATION {
    ULONG   VirtualizationCandidate : 1; // Tells whether the key is part of the virtualization namespace scope (only HKLM\Software for now)
    ULONG   VirtualizationEnabled   : 1; // Tells whether virtualization is enabled on this key. Can be 1 only if above flag is 1.
    ULONG   VirtualTarget           : 1; // Tells if the key is a virtual key. Can be 1 only if above 2 are 0. Valid only on the virtual store key handles.
    ULONG   VirtualStore            : 1; // Tells if the key is a part of the virtual sore path. Valid only on the virtual store key handles.
    ULONG   VirtualSource           : 1; // Tells if the key has ever been virtualized, Can be 1 only if VirtualizationCandidate is 1
    ULONG   Reserved                : 27;
} KEY_VIRTUALIZATION_INFORMATION, *PKEY_VIRTUALIZATION_INFORMATION;

typedef struct _KEY_LAYER_INFORMATION {
    ULONG   IsTombstone      : 1;
    ULONG   IsSupersedeLocal : 1;
    ULONG   IsSupersedeTree  : 1;
    ULONG   ClassIsInherited : 1;
    ULONG   Reserved         : 28;
} KEY_LAYER_INFORMATION, *PKEY_LAYER_INFORMATION;

//
// Thread Environment Block (and portable part of Thread Information Block)
//

//
//  NT_TIB - Thread Information Block - Portable part.
//
//      This is the subsystem portable part of the Thread Information Block.
//      It appears as the first part of the TEB for all threads which have
//      a user mode component.
//
//



typedef struct _EXCEPTION_REGISTRATION_RECORD {
    struct _EXCEPTION_REGISTRATION_RECORD *Next;
    PEXCEPTION_ROUTINE Handler;
} EXCEPTION_REGISTRATION_RECORD;

typedef EXCEPTION_REGISTRATION_RECORD *PEXCEPTION_REGISTRATION_RECORD;

typedef struct _NT_TIB {
    struct _EXCEPTION_REGISTRATION_RECORD *ExceptionList;
    PVOID StackBase;
    PVOID StackLimit;
    PVOID SubSystemTib;
#if defined(_MSC_EXTENSIONS)
    union {
        PVOID FiberData;
        ULONG Version;
    };
#else
    PVOID FiberData;
#endif
    PVOID ArbitraryUserPointer;
    struct _NT_TIB *Self;
} NT_TIB;
typedef NT_TIB *PNT_TIB;

//
// 32 and 64 bit specific version for wow64 and the debugger
//
typedef struct _NT_TIB32 {
    ULONG ExceptionList;
    ULONG StackBase;
    ULONG StackLimit;
    ULONG SubSystemTib;

#if defined(_MSC_EXTENSIONS)
    union {
        ULONG FiberData;
        ULONG Version;
    };
#else
    ULONG FiberData;
#endif

    ULONG ArbitraryUserPointer;
    ULONG Self;
} NT_TIB32, *PNT_TIB32;

typedef struct _NT_TIB64 {
    ULONG64 ExceptionList;
    ULONG64 StackBase;
    ULONG64 StackLimit;
    ULONG64 SubSystemTib;

#if defined(_MSC_EXTENSIONS)
    union {
        ULONG64 FiberData;
        ULONG Version;
    };

#else
    ULONG64 FiberData;
#endif

    ULONG64 ArbitraryUserPointer;
    ULONG64 Self;
} NT_TIB64, *PNT_TIB64;

//
// Process Information Classes
//

//@[comment("MVI_tracked")]
typedef enum _PROCESSINFOCLASS {
    ProcessBasicInformation                      = 0,
    ProcessQuotaLimits                           = 1,
    ProcessIoCounters                            = 2,
    ProcessVmCounters                            = 3,
    ProcessTimes                                 = 4,
    ProcessBasePriority                          = 5,
    ProcessRaisePriority                         = 6,
    ProcessDebugPort                             = 7,
    ProcessExceptionPort                         = 8,
    ProcessAccessToken                           = 9,
    ProcessLdtInformation                        = 10,
    ProcessLdtSize                               = 11,
    ProcessDefaultHardErrorMode                  = 12,
    ProcessIoPortHandlers                        = 13,   // Note: this is kernel mode only
    ProcessPooledUsageAndLimits                  = 14,
    ProcessWorkingSetWatch                       = 15,
    ProcessUserModeIOPL                          = 16,
    ProcessEnableAlignmentFaultFixup             = 17,
    ProcessPriorityClass                         = 18,
    ProcessWx86Information                       = 19,
    ProcessHandleCount                           = 20,
    ProcessAffinityMask                          = 21,
    ProcessPriorityBoost                         = 22,
    ProcessDeviceMap                             = 23,
    ProcessSessionInformation                    = 24,
    ProcessForegroundInformation                 = 25,
    ProcessWow64Information                      = 26,
    ProcessImageFileName                         = 27,
    ProcessLUIDDeviceMapsEnabled                 = 28,
    ProcessBreakOnTermination                    = 29,
    ProcessDebugObjectHandle                     = 30,
    ProcessDebugFlags                            = 31,
    ProcessHandleTracing                         = 32,
    ProcessIoPriority                            = 33,
    ProcessExecuteFlags                          = 34,
    ProcessTlsInformation                        = 35,
    ProcessCookie                                = 36,
    ProcessImageInformation                      = 37,
    ProcessCycleTime                             = 38,
    ProcessPagePriority                          = 39,
    ProcessInstrumentationCallback               = 40,
    ProcessThreadStackAllocation                 = 41,
    ProcessWorkingSetWatchEx                     = 42,
    ProcessImageFileNameWin32                    = 43,
    ProcessImageFileMapping                      = 44,
    ProcessAffinityUpdateMode                    = 45,
    ProcessMemoryAllocationMode                  = 46,
    ProcessGroupInformation                      = 47,
    ProcessTokenVirtualizationEnabled            = 48,
    ProcessOwnerInformation                      = 49,
    ProcessWindowInformation                     = 50,
    ProcessHandleInformation                     = 51,
    ProcessMitigationPolicy                      = 52,
    ProcessDynamicFunctionTableInformation       = 53,
    ProcessHandleCheckingMode                    = 54,
    ProcessKeepAliveCount                        = 55,
    ProcessRevokeFileHandles                     = 56,
    ProcessWorkingSetControl                     = 57,
    ProcessHandleTable                           = 58,
    ProcessCheckStackExtentsMode                 = 59,
    ProcessCommandLineInformation                = 60,
    ProcessProtectionInformation                 = 61,
    ProcessMemoryExhaustion                      = 62,
    ProcessFaultInformation                      = 63,
    ProcessTelemetryIdInformation                = 64,
    ProcessCommitReleaseInformation              = 65,
    ProcessReserved1Information                  = 66,
    ProcessReserved2Information                  = 67,
    ProcessSubsystemProcess                      = 68,
    ProcessInPrivate                             = 70,
    ProcessRaiseUMExceptionOnInvalidHandleClose  = 71,
    ProcessSubsystemInformation                  = 75,
    ProcessWin32kSyscallFilterInformation        = 79,
    ProcessEnergyTrackingState                   = 82,
    MaxProcessInfoClass                                // MaxProcessInfoClass should always be the last enum
} PROCESSINFOCLASS;

//
// Thread Information Classes
//

//@[comment("MVI_tracked")]
typedef enum _THREADINFOCLASS {
    ThreadBasicInformation          = 0,
    ThreadTimes                     = 1,
    ThreadPriority                  = 2,
    ThreadBasePriority              = 3,
    ThreadAffinityMask              = 4,
    ThreadImpersonationToken        = 5,
    ThreadDescriptorTableEntry      = 6,
    ThreadEnableAlignmentFaultFixup = 7,
    ThreadEventPair_Reusable        = 8,
    ThreadQuerySetWin32StartAddress = 9,
    ThreadZeroTlsCell               = 10,
    ThreadPerformanceCount          = 11,
    ThreadAmILastThread             = 12,
    ThreadIdealProcessor            = 13,
    ThreadPriorityBoost             = 14,
    ThreadSetTlsArrayAddress        = 15,   // Obsolete
    ThreadIsIoPending               = 16,
    ThreadHideFromDebugger          = 17,
    ThreadBreakOnTermination        = 18,
    ThreadSwitchLegacyState         = 19,
    ThreadIsTerminated              = 20,
    ThreadLastSystemCall            = 21,
    ThreadIoPriority                = 22,
    ThreadCycleTime                 = 23,
    ThreadPagePriority              = 24,
    ThreadActualBasePriority        = 25,
    ThreadTebInformation            = 26,
    ThreadCSwitchMon                = 27,   // Obsolete
    ThreadCSwitchPmu                = 28,
    ThreadWow64Context              = 29,
    ThreadGroupInformation          = 30,
    ThreadUmsInformation            = 31,   // UMS
    ThreadCounterProfiling          = 32,
    ThreadIdealProcessorEx          = 33,
    ThreadCpuAccountingInformation  = 34,
    ThreadSuspendCount              = 35,
    ThreadActualGroupAffinity       = 41,
    ThreadDynamicCodePolicyInfo     = 42,
    ThreadSubsystemInformation      = 45,

    MaxThreadInfoClass              = 50,
} THREADINFOCLASS;

#define THREAD_CSWITCH_PMU_DISABLE  FALSE
#define THREAD_CSWITCH_PMU_ENABLE   TRUE


//
// Process Information Structures
//



//
// Page/memory priorities.
//

#define MEMORY_PRIORITY_LOWEST           0
#define MEMORY_PRIORITY_VERY_LOW         1
#define MEMORY_PRIORITY_LOW              2
#define MEMORY_PRIORITY_MEDIUM           3
#define MEMORY_PRIORITY_BELOW_NORMAL     4
#define MEMORY_PRIORITY_NORMAL           5



//
// Page priority information. Used with ProcessPagePriority
// and ThreadPagePriority.
//

typedef struct _PAGE_PRIORITY_INFORMATION {
    ULONG PagePriority;
} PAGE_PRIORITY_INFORMATION, *PPAGE_PRIORITY_INFORMATION;

//
// PageFaultHistory Information
//  NtQueryInformationProcess using ProcessWorkingSetWatch
//

typedef struct _PROCESS_WS_WATCH_INFORMATION {
    PVOID FaultingPc;
    PVOID FaultingVa;
} PROCESS_WS_WATCH_INFORMATION, *PPROCESS_WS_WATCH_INFORMATION;

//
// Basic and Extended Basic Process Information
//  NtQueryInformationProcess using ProcessBasicInformation
//

typedef struct _PROCESS_BASIC_INFORMATION {
    NTSTATUS ExitStatus;
    PPEB PebBaseAddress;
    ULONG_PTR AffinityMask;
    KPRIORITY BasePriority;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION,*PPROCESS_BASIC_INFORMATION;

typedef struct _PROCESS_EXTENDED_BASIC_INFORMATION {
    SIZE_T Size;    // Ignored as input, written with structure size on output
    PROCESS_BASIC_INFORMATION BasicInfo;
    union {
        ULONG Flags;
        struct {
            ULONG IsProtectedProcess : 1;
            ULONG IsWow64Process : 1;
            ULONG IsProcessDeleting : 1;
            ULONG IsCrossSessionCreate : 1;
            ULONG IsFrozen : 1;
            ULONG IsBackground : 1;
            ULONG IsStronglyNamed : 1;
            ULONG IsSecureProcess : 1;
            ULONG IsSubsystemProcess : 1;
            ULONG SpareBits : 23;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} PROCESS_EXTENDED_BASIC_INFORMATION, *PPROCESS_EXTENDED_BASIC_INFORMATION;


//
// Process Device Map information
//  NtQueryInformationProcess using ProcessDeviceMap
//  NtSetInformationProcess using ProcessDeviceMap
//

typedef struct _PROCESS_DEVICEMAP_INFORMATION {
    union {
        struct {
            HANDLE DirectoryHandle;
        } Set;
        struct {
            ULONG DriveMap;
            UCHAR DriveType[ 32 ];
        } Query;
    } DUMMYUNIONNAME;
} PROCESS_DEVICEMAP_INFORMATION, *PPROCESS_DEVICEMAP_INFORMATION;

typedef struct _PROCESS_DEVICEMAP_INFORMATION_EX {
    union {
        struct {
            HANDLE DirectoryHandle;
        } Set;
        struct {
            ULONG DriveMap;
            UCHAR DriveType[ 32 ];
        } Query;
    } DUMMYUNIONNAME;
    ULONG Flags;    // specifies that the query type
} PROCESS_DEVICEMAP_INFORMATION_EX, *PPROCESS_DEVICEMAP_INFORMATION_EX;

//
// PROCESS_DEVICEMAP_INFORMATION_EX flags
//
#define PROCESS_LUID_DOSDEVICES_ONLY 0x00000001

//
// Multi-User Session specific Process Information
//  NtQueryInformationProcess using ProcessSessionInformation
//

typedef struct _PROCESS_SESSION_INFORMATION {
    ULONG SessionId;
} PROCESS_SESSION_INFORMATION, *PPROCESS_SESSION_INFORMATION;

typedef struct _PROCESS_HANDLE_TRACING_ENABLE {
    ULONG Flags;
} PROCESS_HANDLE_TRACING_ENABLE, *PPROCESS_HANDLE_TRACING_ENABLE;

typedef struct _PROCESS_HANDLE_TRACING_ENABLE_EX {
    ULONG Flags;
    ULONG TotalSlots;
} PROCESS_HANDLE_TRACING_ENABLE_EX, *PPROCESS_HANDLE_TRACING_ENABLE_EX;

#define PROCESS_HANDLE_EXCEPTIONS_ENABLED 0x00000001ul

#define PROCESS_HANDLE_RAISE_UM_EXCEPTION_ON_INVALID_HANDLE_CLOSE_DISABLED 0x00000000ul
#define PROCESS_HANDLE_RAISE_UM_EXCEPTION_ON_INVALID_HANDLE_CLOSE_ENABLED  0x00000001ul

#define PROCESS_HANDLE_TRACING_MAX_STACKS 16

typedef struct _PROCESS_HANDLE_TRACING_ENTRY {
    HANDLE Handle;
    CLIENT_ID ClientId;
    ULONG Type;
    PVOID Stacks[PROCESS_HANDLE_TRACING_MAX_STACKS];
} PROCESS_HANDLE_TRACING_ENTRY, *PPROCESS_HANDLE_TRACING_ENTRY;

typedef struct _PROCESS_HANDLE_TRACING_QUERY {
    HANDLE Handle;
    ULONG  TotalTraces;
    PROCESS_HANDLE_TRACING_ENTRY HandleTrace[1];
} PROCESS_HANDLE_TRACING_QUERY, *PPROCESS_HANDLE_TRACING_QUERY;

//
// Process Quotas
//  NtQueryInformationProcess using ProcessQuotaLimits
//  NtQueryInformationProcess using ProcessPooledQuotaLimits
//  NtSetInformationProcess using ProcessQuotaLimits
//



typedef struct _QUOTA_LIMITS {
    SIZE_T PagedPoolLimit;
    SIZE_T NonPagedPoolLimit;
    SIZE_T MinimumWorkingSetSize;
    SIZE_T MaximumWorkingSetSize;
    SIZE_T PagefileLimit;
    LARGE_INTEGER TimeLimit;
} QUOTA_LIMITS, *PQUOTA_LIMITS;

#define QUOTA_LIMITS_HARDWS_MIN_ENABLE  0x00000001
#define QUOTA_LIMITS_HARDWS_MIN_DISABLE 0x00000002
#define QUOTA_LIMITS_HARDWS_MAX_ENABLE  0x00000004
#define QUOTA_LIMITS_HARDWS_MAX_DISABLE 0x00000008
#define QUOTA_LIMITS_USE_DEFAULT_LIMITS 0x00000010

typedef union _RATE_QUOTA_LIMIT {
    ULONG RateData;
    struct {
        ULONG RatePercent : 7;
        ULONG Reserved0   : 25;
    } DUMMYSTRUCTNAME;
} RATE_QUOTA_LIMIT, *PRATE_QUOTA_LIMIT;

typedef struct _QUOTA_LIMITS_EX {
    SIZE_T PagedPoolLimit;
    SIZE_T NonPagedPoolLimit;
    SIZE_T MinimumWorkingSetSize;
    SIZE_T MaximumWorkingSetSize;
    SIZE_T PagefileLimit;               // Limit expressed in pages
    LARGE_INTEGER TimeLimit;
    SIZE_T WorkingSetLimit;             // Limit expressed in pages
    SIZE_T Reserved2;
    SIZE_T Reserved3;
    SIZE_T Reserved4;
    ULONG  Flags;
    RATE_QUOTA_LIMIT CpuRateLimit;
} QUOTA_LIMITS_EX, *PQUOTA_LIMITS_EX;



//
// Process I/O Counters
//  NtQueryInformationProcess using ProcessIoCounters
//


typedef struct _IO_COUNTERS {
    ULONGLONG  ReadOperationCount;
    ULONGLONG  WriteOperationCount;
    ULONGLONG  OtherOperationCount;
    ULONGLONG ReadTransferCount;
    ULONGLONG WriteTransferCount;
    ULONGLONG OtherTransferCount;
} IO_COUNTERS;
typedef IO_COUNTERS *PIO_COUNTERS;



//
// Process Virtual Memory Counters
//  NtQueryInformationProcess using ProcessVmCounters
//

typedef struct _VM_COUNTERS {
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
} VM_COUNTERS;
typedef VM_COUNTERS *PVM_COUNTERS;

typedef struct _VM_COUNTERS_EX {
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivateUsage;
} VM_COUNTERS_EX;

typedef VM_COUNTERS_EX *PVM_COUNTERS_EX;

typedef struct _VM_COUNTERS_EX2 {
    VM_COUNTERS_EX CountersEx;
    SIZE_T PrivateWorkingSetSize;
    ULONGLONG SharedCommitUsage;
} VM_COUNTERS_EX2, *PVM_COUNTERS_EX2;


#define MAX_HW_COUNTERS 16
#define THREAD_PROFILING_FLAG_DISPATCH  0x00000001

typedef enum _HARDWARE_COUNTER_TYPE {
    PMCCounter,
    MaxHardwareCounterType
} HARDWARE_COUNTER_TYPE, *PHARDWARE_COUNTER_TYPE;


typedef struct _HARDWARE_COUNTER {
    HARDWARE_COUNTER_TYPE Type;
    ULONG Reserved;
    ULONG64 Index;
} HARDWARE_COUNTER, *PHARDWARE_COUNTER;

//
// Process mitigation policy information
//  NtSetInformationProcess using ProcessMitigationPolicy
//


typedef enum _PROCESS_MITIGATION_POLICY {
    ProcessDEPPolicy,
    ProcessASLRPolicy,
    ProcessDynamicCodePolicy,
    ProcessStrictHandleCheckPolicy,
    ProcessSystemCallDisablePolicy,
    ProcessMitigationOptionsMask,
    ProcessExtensionPointDisablePolicy,
    ProcessControlFlowGuardPolicy,
    ProcessSignaturePolicy,
    ProcessFontDisablePolicy,
    ProcessImageLoadPolicy,
    ProcessSystemCallFilterPolicy,
    ProcessPayloadRestrictionPolicy,
    ProcessChildProcessPolicy,
    MaxProcessMitigationPolicy
} PROCESS_MITIGATION_POLICY, *PPROCESS_MITIGATION_POLICY;

//
// N.B.  High entropy mode is read only and can only be set at creation time
//       and not via the ProcessMitigationPolicy APIs.
//

typedef struct _PROCESS_MITIGATION_ASLR_POLICY {
    union {
        ULONG Flags;
        struct {
            ULONG EnableBottomUpRandomization : 1;
            ULONG EnableForceRelocateImages : 1;
            ULONG EnableHighEntropy : 1;
            ULONG DisallowStrippedImages : 1;
            ULONG ReservedFlags : 28;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} PROCESS_MITIGATION_ASLR_POLICY, *PPROCESS_MITIGATION_ASLR_POLICY;

typedef struct _PROCESS_MITIGATION_DEP_POLICY {
    union {
        ULONG Flags;
        struct {
            ULONG Enable : 1;
            ULONG DisableAtlThunkEmulation : 1;
            ULONG ReservedFlags : 30;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    BOOLEAN Permanent;
} PROCESS_MITIGATION_DEP_POLICY, *PPROCESS_MITIGATION_DEP_POLICY;

typedef struct _PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY {
    union {
        ULONG Flags;
        struct {
            ULONG RaiseExceptionOnInvalidHandleReference : 1;
            ULONG HandleExceptionsPermanentlyEnabled : 1;
            ULONG ReservedFlags : 30;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY, *PPROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY;

typedef struct _PROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY {
    union {
        ULONG Flags;
        struct {
            ULONG DisallowWin32kSystemCalls : 1;
            ULONG AuditDisallowWin32kSystemCalls : 1;
            ULONG ReservedFlags : 30;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} PROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY, *PPROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY;

typedef struct _PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY {
    union {
        ULONG Flags;
        struct {
            ULONG DisableExtensionPoints : 1;
            ULONG ReservedFlags : 31;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY, *PPROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY;

typedef struct _PROCESS_MITIGATION_DYNAMIC_CODE_POLICY {
    union {
        ULONG Flags;
        struct {
            ULONG ProhibitDynamicCode : 1;
            ULONG AllowThreadOptOut : 1;
            ULONG AllowRemoteDowngrade : 1;
            ULONG AuditProhibitDynamicCode : 1;
            ULONG ReservedFlags : 28;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} PROCESS_MITIGATION_DYNAMIC_CODE_POLICY, *PPROCESS_MITIGATION_DYNAMIC_CODE_POLICY;

typedef struct _PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY {
    union {
        ULONG Flags;
        struct {
            ULONG EnableControlFlowGuard : 1;
            ULONG EnableExportSuppression : 1;
            ULONG StrictMode : 1;
            ULONG ReservedFlags : 29;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY, *PPROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY;

typedef struct _PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY {
    union {
        ULONG Flags;
        struct {
            ULONG MicrosoftSignedOnly : 1;
            ULONG StoreSignedOnly : 1;
            ULONG MitigationOptIn : 1;
            ULONG AuditMicrosoftSignedOnly : 1;
            ULONG AuditStoreSignedOnly : 1;
            ULONG ReservedFlags : 27;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY, *PPROCESS_MITIGATION_BINARY_SIGNATURE_POLICY;

typedef struct _PROCESS_MITIGATION_FONT_DISABLE_POLICY {
    union {
        ULONG Flags;
        struct {
            ULONG DisableNonSystemFonts     : 1;
            ULONG AuditNonSystemFontLoading : 1;
            ULONG ReservedFlags             : 30;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} PROCESS_MITIGATION_FONT_DISABLE_POLICY, *PPROCESS_MITIGATION_FONT_DISABLE_POLICY;

typedef struct _PROCESS_MITIGATION_IMAGE_LOAD_POLICY {
    union {
        ULONG Flags;
        struct {
            ULONG NoRemoteImages : 1;
            ULONG NoLowMandatoryLabelImages : 1;
            ULONG PreferSystem32Images : 1;
            ULONG AuditNoRemoteImages : 1;
            ULONG AuditNoLowMandatoryLabelImages : 1;
            ULONG ReservedFlags : 27;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} PROCESS_MITIGATION_IMAGE_LOAD_POLICY, *PPROCESS_MITIGATION_IMAGE_LOAD_POLICY;

typedef struct _PROCESS_MITIGATION_SYSTEM_CALL_FILTER_POLICY {
    union {
        ULONG Flags;
        struct {
            ULONG FilterId: 4;
            ULONG ReservedFlags : 28;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} PROCESS_MITIGATION_SYSTEM_CALL_FILTER_POLICY, *PPROCESS_MITIGATION_SYSTEM_CALL_FILTER_POLICY;

typedef struct _PROCESS_MITIGATION_PAYLOAD_RESTRICTION_POLICY {
    union {
        ULONG Flags;
        struct {
            ULONG EnableExportAddressFilter     : 1;
            ULONG AuditExportAddressFilter      : 1;

            ULONG EnableExportAddressFilterPlus : 1;
            ULONG AuditExportAddressFilterPlus  : 1;

            ULONG EnableImportAddressFilter     : 1;
            ULONG AuditImportAddressFilter      : 1;

            ULONG EnableRopStackPivot           : 1;
            ULONG AuditRopStackPivot            : 1;

            ULONG EnableRopCallerCheck          : 1;
            ULONG AuditRopCallerCheck           : 1;

            ULONG EnableRopSimExec              : 1;
            ULONG AuditRopSimExec               : 1;

            ULONG ReservedFlags                 : 20;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} PROCESS_MITIGATION_PAYLOAD_RESTRICTION_POLICY, *PPROCESS_MITIGATION_PAYLOAD_RESTRICTION_POLICY;

typedef struct _PROCESS_MITIGATION_CHILD_PROCESS_POLICY {
    union {
        ULONG Flags;
        struct {
            ULONG NoChildProcessCreation : 1;
            ULONG AuditNoChildProcessCreation : 1;
            ULONG AllowSecureProcessCreation : 1;
            ULONG ReservedFlags : 29;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} PROCESS_MITIGATION_CHILD_PROCESS_POLICY, *PPROCESS_MITIGATION_CHILD_PROCESS_POLICY;



//
// Process KeepAlive Count
// NtQueryInformationProcess using ProcessKeepAliveCount
//
typedef struct _PROCESS_KEEPALIVE_COUNT_INFORMATION {
    ULONG WakeCount;
    ULONG NoWakeCount;
} PROCESS_KEEPALIVE_COUNT_INFORMATION, *PPROCESS_KEEPALIVE_COUNT_INFORMATION;

//
// Process File Handle Revocation
// NtSetInformationProcess using ProcessRevokeFileHandles
//
typedef struct _PROCESS_REVOKE_FILE_HANDLES_INFORMATION {
    UNICODE_STRING TargetDevicePath;
} PROCESS_REVOKE_FILE_HANDLES_INFORMATION, *PPROCESS_REVOKE_FILE_HANDLES_INFORMATION;

//
// Process Read/WriteVm Logging
// NtQueryInformationProcess using ProcessEnableReadWriteVmLogging
//
#define PROCESS_READWRITEVM_LOGGING_ENABLE_READVM       0x01
#define PROCESS_READWRITEVM_LOGGING_ENABLE_READVM_V     1UL
#define PROCESS_READWRITEVM_LOGGING_ENABLE_WRITEVM      0x02L
#define PROCESS_READWRITEVM_LOGGING_ENABLE_WRITEVM_V    2UL

typedef union _PROCESS_READWRITEVM_LOGGING_INFORMATION {
    UCHAR Flags;
    struct {
        UCHAR EnableReadVmLogging : 1;
        UCHAR EnableWriteVmLogging : 1;
        UCHAR Unused : 6;
    };
} PROCESS_READWRITEVM_LOGGING_INFORMATION, *PPROCESS_READWRITEVM_LOGGING_INFORMATION;

//
// Process Pooled Quota Usage and Limits
//  NtQueryInformationProcess using ProcessPooledUsageAndLimits
//

typedef struct _POOLED_USAGE_AND_LIMITS {
    SIZE_T PeakPagedPoolUsage;
    SIZE_T PagedPoolUsage;
    SIZE_T PagedPoolLimit;
    SIZE_T PeakNonPagedPoolUsage;
    SIZE_T NonPagedPoolUsage;
    SIZE_T NonPagedPoolLimit;
    SIZE_T PeakPagefileUsage;
    SIZE_T PagefileUsage;
    SIZE_T PagefileLimit;
} POOLED_USAGE_AND_LIMITS;
typedef POOLED_USAGE_AND_LIMITS *PPOOLED_USAGE_AND_LIMITS;

//
// Process Security Context Information
//  NtSetInformationProcess using ProcessAccessToken
// PROCESS_SET_ACCESS_TOKEN access to the process is needed
// to use this info level.
//

typedef struct _PROCESS_ACCESS_TOKEN {

    //
    // Handle to Primary token to assign to the process.
    // TOKEN_ASSIGN_PRIMARY access to this token is needed.
    //

    HANDLE Token;

    //
    // Handle to the initial thread of the process.
    // A process's access token can only be changed if the process has
    // no threads or a single thread that has not yet begun execution.
    //
    // N.B. This field is unused.
    //

    HANDLE Thread;

} PROCESS_ACCESS_TOKEN, *PPROCESS_ACCESS_TOKEN;

//
// Process Exception Port Information
//  NtSetInformationProcess using ProcessExceptionPort
// PROCESS_SET_PORT access to the process is needed
// to use this info level.
//

#define PROCESS_EXCEPTION_PORT_ALL_STATE_BITS     0x00000003UL
#define PROCESS_EXCEPTION_PORT_ALL_STATE_FLAGS    ((ULONG_PTR)((1UL << PROCESS_EXCEPTION_PORT_ALL_STATE_BITS) - 1))

typedef struct _PROCESS_EXCEPTION_PORT {

    //
    // Handle to the exception port. No particular access required.
    //

    _In_ HANDLE ExceptionPortHandle;

    //
    // Miscellaneous state flags to be cached along with the exception
    // port in the kernel.
    //

    _Inout_ ULONG StateFlags;

} PROCESS_EXCEPTION_PORT, *PPROCESS_EXCEPTION_PORT;

//
// Process/Thread System and User Time
//  NtQueryInformationProcess using ProcessTimes
//  NtQueryInformationThread using ThreadTimes
//

typedef struct _KERNEL_USER_TIMES {
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER ExitTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
} KERNEL_USER_TIMES;
typedef KERNEL_USER_TIMES *PKERNEL_USER_TIMES;

//
// Process/Thread subsystem information
//  NtQueryInformationProcess using ProcessSubsystemInformation
//  NtQueryInformationThread using ThreadSubsystemInformation
//

typedef enum _SUBSYSTEM_INFORMATION_TYPE {
    SubsystemInformationTypeWin32 = 0,
    SubsystemInformationTypeWSL = 1,
    MaxSubsystemInformationType
} SUBSYSTEM_INFORMATION_TYPE, *PSUBSYSTEM_INFORMATION_TYPE;

//
// Process resource throttling information
//  NtQueryInformationProcess using ProcessPowerThrottlingState
//

#define POWER_THROTTLING_PROCESS_CURRENT_VERSION 1

#define POWER_THROTTLING_PROCESS_EXECUTION_SPEED 0x1
#define POWER_THROTTLING_PROCESS_DELAYTIMERS 0x2

#define POWER_THROTTLING_PROCESS_VALID_FLAGS ((POWER_THROTTLING_PROCESS_EXECUTION_SPEED | \
                                               POWER_THROTTLING_PROCESS_DELAYTIMERS))

typedef struct _POWER_THROTTLING_PROCESS_STATE {
    ULONG Version;
    ULONG ControlMask;
    ULONG StateMask;
} POWER_THROTTLING_PROCESS_STATE, *PPOWER_THROTTLING_PROCESS_STATE;

#define POWER_THROTTLING_THREAD_CURRENT_VERSION 1

#define POWER_THROTTLING_THREAD_EXECUTION_SPEED 0x1

#define POWER_THROTTLING_THREAD_VALID_FLAGS (POWER_THROTTLING_THREAD_EXECUTION_SPEED)

typedef struct _POWER_THROTTLING_THREAD_STATE {
    ULONG Version;
    ULONG ControlMask;
    ULONG StateMask;
} POWER_THROTTLING_THREAD_STATE, *PPOWER_THROTTLING_THREAD_STATE;

//@[comment("MVI_tracked")]
__kernel_entry NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenProcess (
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PCLIENT_ID ClientId
    );

#define NTKERNELAPI DECLSPEC_IMPORT     


#if defined(_X86_) 



#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_saves_
_IRQL_raises_(DISPATCH_LEVEL)
_DECL_HAL_KE_IMPORT
KIRQL
KeRaiseIrqlToDpcLevel (
    VOID
    );
#endif



#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_saves_
_DECL_HAL_KE_IMPORT
KIRQL
KeRaiseIrqlToSynchLevel (
    VOID
    );
#endif


#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_)



#define KeQueryTickCount(CurrentCount) { \
    KSYSTEM_TIME volatile *_TickCount = *((PKSYSTEM_TIME *)(&KeTickCount)); \
    for (;;) {                                                              \
        (CurrentCount)->HighPart = _TickCount->High1Time;                   \
        (CurrentCount)->LowPart = _TickCount->LowPart;                      \
        if ((CurrentCount)->HighPart == _TickCount->High2Time) break;       \
        YieldProcessor();                                                   \
    }                                                                       \
}



#else

VOID
NTAPI
KeQueryTickCount (
    _Out_ PLARGE_INTEGER CurrentCount
    );

#endif // defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_)



//
// Processor Control Region Structure Definition
//

#define PCR_MINOR_VERSION 1
#define PCR_MAJOR_VERSION 1

typedef struct _KPCR {

//
// Start of the architecturally defined section of the PCR. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//
// Certain fields in the TIB are not used in kernel mode. These include the
// stack limit, subsystem TIB, fiber data, arbitrary user pointer, and the
// self address of then PCR itself (another field has been added for that
// purpose). Therefore, these fields are overlaid with other data to get
// better cache locality.
//

    union {
        NT_TIB  NtTib;
        struct {
            struct _EXCEPTION_REGISTRATION_RECORD *Used_ExceptionList;
            PVOID Used_StackBase;
            ULONG MxCsr;
            PVOID TssCopy;
            ULONG ContextSwitches;
            KAFFINITY SetMemberCopy;
            PVOID Used_Self;
        };
    };

    struct _KPCR *SelfPcr;              // flat address of this PCR
    struct _KPRCB *Prcb;                // pointer to Prcb
    KIRQL   Irql;                       // do not use 3 bytes after this as
                                        // HALs assume they are zero.
    ULONG   IRR;
    ULONG   IrrActive;
    ULONG   IDR;
    PVOID   KdVersionBlock;

    struct _KIDTENTRY *IDT;
    struct _KGDTENTRY *GDT;
    struct _KTSS      *TSS;
    USHORT  MajorVersion;
    USHORT  MinorVersion;
    KAFFINITY SetMember;
    ULONG   StallScaleFactor;
    UCHAR   SpareUnused;
    UCHAR   Number;



    UCHAR   Spare0;
    UCHAR   SecondLevelCacheAssociativity;
    ULONG   VdmAlert;
    ULONG   KernelReserved[14];         // For use by the kernel
    ULONG   SecondLevelCacheSize;
    ULONG   HalReserved[16];            // For use by Hal


} KPCR, *PKPCR;


//
// i386 Specific portions of Mm component.
//

//
// Define the number of bits to shift to right justify the Page Directory Index
// field of a PTE.
//

#define PDI_SHIFT 21
#define PPI_SHIFT 30

//
// Define the number of bits to shift to right justify the Page Table Index
// field of a PTE.
//

#define PTI_SHIFT 12


//
// Define the number of PTEs and PDEs per page.
//

#define PTE_PER_PAGE 512
#define PDE_PER_PAGE 512

//
// Define the highest user address and user probe address.
//

extern NTKERNELAPI PVOID MmHighestUserAddress;
extern NTKERNELAPI PVOID MmSystemRangeStart;
extern NTKERNELAPI ULONG MmUserProbeAddress;

#define MM_HIGHEST_USER_ADDRESS MmHighestUserAddress
#define MM_SYSTEM_RANGE_START MmSystemRangeStart

#if defined(_LOCAL_COPY_USER_PROBE_ADDRESS_)

#define MM_USER_PROBE_ADDRESS _LOCAL_COPY_USER_PROBE_ADDRESS_

#if defined(__CONVERGED_WIN32K_DRIVER__)
extern DLLBASEEXPORT ULONG _LOCAL_COPY_USER_PROBE_ADDRESS_;
#else
extern ULONG _LOCAL_COPY_USER_PROBE_ADDRESS_;
#endif

#else

#define MM_USER_PROBE_ADDRESS MmUserProbeAddress

#endif

#define MM_KSEG0_BASE       MM_SYSTEM_RANGE_START
#define MM_SYSTEM_SPACE_END 0xFFFFFFFF

//
// The lowest user address reserves the low 64k.
//

#define MM_LOWEST_USER_ADDRESS (PVOID)0x10000


//
// Prototypes for architectural specific versions of Exi386 Api
//

//
// Interlocked result type is portable, but its values are machine specific.
// Constants for value are in i386.h, mips.h, etc.
//

typedef enum _INTERLOCKED_RESULT {
    ResultNegative = RESULT_NEGATIVE,
    ResultZero     = RESULT_ZERO,
    ResultPositive = RESULT_POSITIVE
} INTERLOCKED_RESULT;

NTKERNELAPI
INTERLOCKED_RESULT
FASTCALL
Exfi386InterlockedIncrementLong (
    _Inout_ _Interlocked_operand_ LONG volatile *Addend
    );

NTKERNELAPI
INTERLOCKED_RESULT
FASTCALL
Exfi386InterlockedDecrementLong (
    _Inout_ _Interlocked_operand_ LONG volatile *Addend
    );

NTKERNELAPI
ULONG
FASTCALL
Exfi386InterlockedExchangeUlong (
    _Inout_ _Interlocked_operand_ ULONG volatile *Target,
    _In_ ULONG Value
    );


#if !defined(MIDL_PASS) && defined(_M_IX86)

//
// i386 function definitions
//



#pragma warning(push)


//
// Get the current processor number
//

FORCEINLINE
ULONG
NTAPI
KeGetCurrentProcessorNumber(VOID)
{
    return (ULONG) __readfsbyte (FIELD_OFFSET (KPCR, Number));
}


#pragma warning(pop)


#endif // !defined(MIDL_PASS) && defined(_M_IX86)



#endif // defined(_X86_)



// Use the following for kernel mode runtime checks of X86 system architecture

#ifdef _X86_

#ifdef IsNEC_98
#undef IsNEC_98
#endif

#ifdef IsNotNEC_98
#undef IsNotNEC_98
#endif

#ifdef SetNEC_98
#undef SetNEC_98
#endif

#ifdef SetNotNEC_98
#undef SetNotNEC_98
#endif

#define IsNEC_98     (SharedUserData->AlternativeArchitecture == NEC98x86)
#define IsNotNEC_98  (SharedUserData->AlternativeArchitecture != NEC98x86)
#define SetNEC_98    SharedUserData->AlternativeArchitecture = NEC98x86
#define SetNotNEC_98 SharedUserData->AlternativeArchitecture = StandardDesign

#endif

#if defined(_AMD64_) 

//
// Processor Control Region Structure Definition
//

#define PCR_MINOR_VERSION 1
#define PCR_MAJOR_VERSION 1

typedef struct _KPCR {

//
// Start of the architecturally defined section of the PCR. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//
// Certain fields in the TIB are not used in kernel mode. These include the
// exception list, stack base, stack limit, subsystem TIB, fiber data, and
// the arbitrary user pointer. Therefore, these fields are overlaid with
// other data to get better cache locality.
//
// N.B. The offset to the PRCB in the PCR is fixed for all time.
//

    union {
        NT_TIB NtTib;
        struct {
            union _KGDTENTRY64 *GdtBase;
            struct _KTSS64 *TssBase;
            ULONG64 UserRsp;
            struct _KPCR *Self;
            struct _KPRCB *CurrentPrcb;
            PKSPIN_LOCK_QUEUE LockArray;
            PVOID Used_Self;
        };
    };

    union _KIDTENTRY64 *IdtBase;
    ULONG64 Unused[2];
    KIRQL Irql;
    UCHAR SecondLevelCacheAssociativity;
    UCHAR ObsoleteNumber;
    UCHAR Fill0;
    ULONG Unused0[3];
    USHORT MajorVersion;
    USHORT MinorVersion;
    ULONG StallScaleFactor;
    PVOID Unused1[3];

    ULONG KernelReserved[15];
    ULONG SecondLevelCacheSize;
    ULONG HalReserved[16];
    ULONG Unused2;
    PVOID KdVersionBlock;
    PVOID Unused3;
    ULONG PcrAlign1[24];


} KPCR, *PKPCR;

//
// Exception frame
//
//  This frame is established when handling an exception. It provides a place
//  to save all nonvolatile registers. The volatile registers will already
//  have been saved in a trap frame.
//
// N.B. The exception frame has a built in exception record capable of
//      storing information for four parameter values. This exception
//      record is used exclusively within the trap handling code.
//

typedef struct _KEXCEPTION_FRAME {

//
// Home address for the parameter registers.
//

    ULONG64 P1Home;
    ULONG64 P2Home;
    ULONG64 P3Home;
    ULONG64 P4Home;
    ULONG64 P5;
    ULONG64 Spare1;

//
// Saved nonvolatile floating registers.
//

    M128A Xmm6;
    M128A Xmm7;
    M128A Xmm8;
    M128A Xmm9;
    M128A Xmm10;
    M128A Xmm11;
    M128A Xmm12;
    M128A Xmm13;
    M128A Xmm14;
    M128A Xmm15;

//
// Kernel callout frame variables.
//

    ULONG64 TrapFrame;
    ULONG64 OutputBuffer;
    ULONG64 OutputLength;
    ULONG64 Spare2;

//
// Saved MXCSR when a thread is interrupted in kernel mode via a dispatch
// interrupt.
//

    ULONG64 MxCsr;

//
// Saved nonvolatile register - not always saved.
//

    ULONG64 Rbp;

//
// Saved nonvolatile registers.
//

    ULONG64 Rbx;
    ULONG64 Rdi;
    ULONG64 Rsi;
    ULONG64 R12;
    ULONG64 R13;
    ULONG64 R14;
    ULONG64 R15;

//
// EFLAGS and return address.
//

    ULONG64 Return;
} KEXCEPTION_FRAME, *PKEXCEPTION_FRAME;

//
// Trap frame
//
// This frame is established when handling a trap. It provides a place to
// save all volatile registers. The nonvolatile registers are saved in an
// exception frame or through the normal C calling conventions for saved
// registers.
//

typedef struct _KTRAP_FRAME {

//
// Home address for the parameter registers.
//

    ULONG64 P1Home;
    ULONG64 P2Home;
    ULONG64 P3Home;
    ULONG64 P4Home;
    ULONG64 P5;

//
// Previous processor mode (system services only) and previous IRQL
// (interrupts only).
//

    KPROCESSOR_MODE PreviousMode;
    KIRQL PreviousIrql;

//
// Page fault load/store indicator.
//

        UCHAR FaultIndicator;

//
// Exception active indicator.
//
//    0 - interrupt frame.
//    1 - exception frame.
//    2 - service frame.
//

    UCHAR ExceptionActive;

//
// Floating point state.
//

    ULONG MxCsr;

//
//  Volatile registers.
//
// N.B. These registers are only saved on exceptions and interrupts. They
//      are not saved for system calls.
//

    ULONG64 Rax;
    ULONG64 Rcx;
    ULONG64 Rdx;
    ULONG64 R8;
    ULONG64 R9;
    ULONG64 R10;
    ULONG64 R11;

//
// Gsbase is only used if the previous mode was kernel.
//
// GsSwap is only used if the previous mode was user.
//

    union {
        ULONG64 GsBase;
        ULONG64 GsSwap;
    };

//
// Volatile floating registers.
//
// N.B. These registers are only saved on exceptions and interrupts. They
//      are not saved for system calls.
//

    M128A Xmm0;
    M128A Xmm1;
    M128A Xmm2;
    M128A Xmm3;
    M128A Xmm4;
    M128A Xmm5;

//
// First parameter, page fault address, context record address if user APC
// bypass.
//

    union {
        ULONG64 FaultAddress;
        ULONG64 ContextRecord;
    };

//
//  Debug registers.
//

    ULONG64 Dr0;
    ULONG64 Dr1;
    ULONG64 Dr2;
    ULONG64 Dr3;
    ULONG64 Dr6;
    ULONG64 Dr7;

//
// Special debug registers.
//

    struct {
        ULONG64 DebugControl;
        ULONG64 LastBranchToRip;
        ULONG64 LastBranchFromRip;
        ULONG64 LastExceptionToRip;
        ULONG64 LastExceptionFromRip;
    };

//
//  Segment registers
//

    USHORT SegDs;
    USHORT SegEs;
    USHORT SegFs;
    USHORT SegGs;

//
// Previous trap frame address.
//

    ULONG64 TrapFrame;

//
// Saved nonvolatile registers RBX, RDI and RSI. These registers are only
// saved in system service trap frames.
//

    ULONG64 Rbx;
    ULONG64 Rdi;
    ULONG64 Rsi;

//
// Saved nonvolatile register RBP. This register is used as a frame
// pointer during trap processing and is saved in all trap frames.
//

    ULONG64 Rbp;

//
// Information pushed by hardware.
//
// N.B. The error code is not always pushed by hardware. For those cases
//      where it is not pushed by hardware a dummy error code is allocated
//      on the stack.
//

    union {
        ULONG64 ErrorCode;
        ULONG64 ExceptionFrame;
    };

    ULONG64 Rip;
    USHORT SegCs;
    UCHAR Fill0;
    UCHAR Logging;
    USHORT Fill1[2];
    ULONG EFlags;
    ULONG Fill2;
    ULONG64 Rsp;
    USHORT SegSs;
    USHORT Fill3;
    ULONG Fill4;
} KTRAP_FRAME, *PKTRAP_FRAME;

typedef struct _KUMS_CONTEXT_HEADER {
    ULONG64 P1Home;
    ULONG64 P2Home;
    ULONG64 P3Home;
    ULONG64 P4Home;
    PVOID StackTop;
    ULONG64 StackSize;
    ULONG64 RspOffset;
    ULONG64 Rip;
    PXMM_SAVE_AREA32 FltSave;
#define KUMS_UCH_VOLATILE_BIT (0)
#define KUMS_UCH_VOLATILE_MASK (1ULL << KUMS_UCH_VOLATILE_BIT)
    union {
        struct {
            ULONG64 Volatile : 1;
            ULONG64 Reserved : 63;
        };
        ULONG64 Flags;
    };
    PKTRAP_FRAME TrapFrame;
    PKEXCEPTION_FRAME ExceptionFrame;
    struct _KTHREAD *SourceThread;
    ULONG64 Return;
} KUMS_CONTEXT_HEADER, *PKUMS_CONTEXT_HEADER;


//
// Define the last branch control MSR address.
//

extern NTKERNELAPI ULONG KeLastBranchMSR;

//
// Define the highest user address and user probe address.
//

extern const NTKERNELAPI PVOID MmHighestUserAddress;
extern const NTKERNELAPI PVOID MmSystemRangeStart;
extern const NTKERNELAPI ULONG64 MmUserProbeAddress;

#define MM_HIGHEST_USER_ADDRESS MmHighestUserAddress
#define MM_SYSTEM_RANGE_START MmSystemRangeStart

//
// Allow non-kernel components to capture the user probe address and use a
// local copy for efficiency.
//

#if defined(_LOCAL_COPY_USER_PROBE_ADDRESS_)

#define MM_USER_PROBE_ADDRESS _LOCAL_COPY_USER_PROBE_ADDRESS_

#if defined(__CONVERGED_WIN32K_DRIVER__)
extern DLLBASEEXPORT ULONG64 _LOCAL_COPY_USER_PROBE_ADDRESS_;
#else
extern ULONG64 _LOCAL_COPY_USER_PROBE_ADDRESS_;
#endif

#else

#define MM_USER_PROBE_ADDRESS MmUserProbeAddress

#endif

//
// The lowest user address reserves the low 64k.
//

#define MM_LOWEST_USER_ADDRESS (PVOID)(LONG_PTR)0x10000


//
// Intrinsic functions
//

#if defined(_M_AMD64) && !defined(RC_INVOKED)  && !defined(MIDL_PASS)

//
// The following routines are provided for backward compatibility with old
// code. They are no longer the preferred way to accomplish these functions.
//

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExInterlockedIncrementLong)      // Use InterlockedIncrement
#pragma deprecated(ExInterlockedDecrementLong)      // Use InterlockedDecrement
#pragma deprecated(ExInterlockedExchangeUlong)      // Use InterlockedExchange
#endif

#define RESULT_ZERO 0
#define RESULT_NEGATIVE 1
#define RESULT_POSITIVE 2

typedef enum _INTERLOCKED_RESULT {
    ResultNegative = RESULT_NEGATIVE,
    ResultZero = RESULT_ZERO,
    ResultPositive = RESULT_POSITIVE
} INTERLOCKED_RESULT;

#define ExInterlockedDecrementLong(Addend, Lock)                            \
    _ExInterlockedDecrementLong(Addend)

_Post_satisfies_(return >= 0 && return <= 2)
__forceinline
LONG
_ExInterlockedDecrementLong (
    _Inout_ _Interlocked_operand_ PLONG Addend
    )

{

    LONG Result;

    Result = InterlockedDecrement(Addend);
    if (Result < 0) {
        return ResultNegative;

    } else if (Result > 0) {
        return ResultPositive;

    } else {
        return ResultZero;
    }
}

#define ExInterlockedIncrementLong(Addend, Lock)                            \
    _ExInterlockedIncrementLong(Addend)

_Post_satisfies_(return >= 0 && return <= 2)
__forceinline
LONG
_ExInterlockedIncrementLong (
    _Inout_ _Interlocked_operand_ PLONG Addend
    )

{

    LONG Result;

    Result = InterlockedIncrement(Addend);
    if (Result < 0) {
        return ResultNegative;

    } else if (Result > 0) {
        return ResultPositive;

    } else {
        return ResultZero;
    }
}

#define ExInterlockedExchangeUlong(Target, Value, Lock)                     \
    _ExInterlockedExchangeUlong(Target, Value)

__forceinline
ULONG
_ExInterlockedExchangeUlong (
    _Inout_ _Interlocked_operand_ PULONG Target,
    _In_ ULONG Value
    )

{

    return (ULONG)InterlockedExchange((PLONG)Target, (LONG)Value);
}

#endif // defined(_M_AMD64) && !defined(RC_INVOKED)  && !defined(MIDL_PASS)


#if !defined(MIDL_PASS) && defined(_M_AMD64)

//
// AMD646 function prototype definitions
//


//
// Get address of current processor block.
//

__forceinline
PKPCR
KeGetPcr (
    VOID
    )

{
    return (PKPCR)__readgsqword(FIELD_OFFSET(KPCR, Self));
}


#if (NTDDI_VERSION < NTDDI_WIN7) || !defined(NT_PROCESSOR_GROUPS)

//
// Get the current legacy processor number
//

__forceinline
ULONG
KeGetCurrentProcessorNumber (
    VOID
    )

{

    return (ULONG)__readgsbyte(0x184);
}

#endif


#endif // !defined(MIDL_PASS) && defined(_M_AMD64)


#endif // defined(_AMD64_)


//
// Platform specific kernel functions to raise and lower IRQL.
//


#if defined(_AMD64_) && !defined(MIDL_PASS)

_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_saves_
_IRQL_raises_(DISPATCH_LEVEL)
__forceinline
KIRQL
KeRaiseIrqlToDpcLevel (
    VOID
    )

/*++

Routine Description:

    This function raises the current IRQL to DPC_LEVEL and returns the
    previous IRQL.

Arguments:

    None.

Return Value:

    The previous IRQL is returned as the function value.

--*/

{

    return KfRaiseIrql(DISPATCH_LEVEL);
}

_IRQL_saves_
_IRQL_raises_(12)
__forceinline
KIRQL
KeRaiseIrqlToSynchLevel (
    VOID
    )

/*++

Routine Description:

    This function raises the current IRQL to SYNCH_LEVEL and returns the
    previous IRQL.

Arguments:

Return Value:

    The previous IRQL is returned as the function value.

--*/

{

    return KfRaiseIrql(12);
}


#endif // defined(_AMD64_) && !defined(MIDL_PASS)

#if defined(_ARM_) 

#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_)



#define KeQueryTickCount(CurrentCount) { \
    KSYSTEM_TIME volatile *_TickCount = *((PKSYSTEM_TIME *)(&KeTickCount)); \
    for (;;) {                                                              \
        (CurrentCount)->HighPart = _TickCount->High1Time;                   \
        (CurrentCount)->LowPart = _TickCount->LowPart;                      \
        if ((CurrentCount)->HighPart == _TickCount->High2Time) break;       \
        YieldProcessor();                                                   \
    }                                                                       \
}



#else

VOID
NTAPI
KeQueryTickCount (
    OUT PLARGE_INTEGER CurrentCount
    );

#endif // defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_)



//
// ARM IDT Entry Structure Definition
//
// ARM does not have a native IDT format and does not vector interrupt
// exceptions.
//

//
// Processor Control Region Structure Definition
//

#define PCR_MINOR_VERSION 1
#define PCR_MAJOR_VERSION 1

typedef struct _KPCR {

//
// Start of the architecturally defined section of the PCR. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//
// Certain fields in the TIB are not used in kernel mode. These include the
// exception list, stack base, stack limit, subsystem TIB, fiber data, and
// the arbitrary user pointer. Therefore, these fields are overlaid with
// other data to get better cache locality.
//
// N.B. The offset to the PRCB in the PCR is fixed for all time.
//

    union {
        NT_TIB NtTib;
        struct {
            ULONG TibPad0[2];
            PVOID Spare1;
            struct _KPCR *Self;
            struct _KPRCB *CurrentPrcb;
            PKSPIN_LOCK_QUEUE LockArray;
            PVOID Used_Self;
        };
    };

    KIRQL CurrentIrql;
    UCHAR SecondLevelCacheAssociativity;
    ULONG Unused0[3];
    USHORT MajorVersion;
    USHORT MinorVersion;
    ULONG StallScaleFactor;
    PVOID Unused1[3];

    ULONG KernelReserved[15];
    ULONG SecondLevelCacheSize;
    union {
        USHORT SoftwareInterruptPending; // Software Interrupt Pending Flag
        struct {
            UCHAR ApcInterrupt;          // 0x01 if APC int pending
            UCHAR DispatchInterrupt;     // 0x01 if dispatch int pending
        };
    };

    USHORT InterruptPad;
    ULONG HalReserved[32];
    PVOID KdVersionBlock;
    PVOID Unused3;
    ULONG PcrAlign1[8];


} KPCR, *PKPCR;

//
// Exception frame
//
//  This frame is established when handling an exception. It provides a place
//  to save all nonvolatile registers. The volatile registers will already
//  have been saved in a trap frame.
//
// N.B. The exception frame is preceded in the trap handling code by
//      an exception record capable of storing information for four
//      parameter values.
//

typedef struct _KEXCEPTION_FRAME {

    ULONG Param5;

//
// Kernel callout frame variables.
//

    ULONG TrapFrame;
    ULONG OutputBuffer;
    ULONG OutputLength;

//
// Saved nonvolatile registers.
//

    ULONG Pad;
    ULONG R4;
    ULONG R5;
    ULONG R6;
    ULONG R7;
    ULONG R8;
    ULONG R9;
    ULONG R10;
    ULONG R11;

    ULONG Return;

} KEXCEPTION_FRAME, *PKEXCEPTION_FRAME;

//
// VFP State -- specially organized collection of VFP state that
// is used in context switching. The volatile section is also used
// in trap frame handling and lazy VFP enabling.
//

typedef struct _KARM_VFP_STATE
{
    struct _KARM_VFP_STATE *Link;           // link to next state entry
    ULONG Fpscr;                            // FPSCR register
    ULONG Reserved;                         // reserved for future use
    ULONG Reserved2;                        // reserved for future use
    ULONGLONG VfpD[32];                     // All D registers (0-31)
} KARM_VFP_STATE, *PKARM_VFP_STATE;

//
// Trap frame
//
// This frame is established when handling a trap. It provides a place to
// save all volatile registers. The nonvolatile registers are saved in an
// exception frame or through the normal C calling conventions for saved
// registers.
//
// Code to generate the trap frame is found in ntos\ke\arm\trap.asm.
//

#define KTRAP_FRAME_ARGUMENTS (14 * 4)       // up to 14 in-memory syscall args

typedef struct _KTRAP_FRAME {

//
// Page fault information (page faults only)
// Previous trap frame address (system services only)
//
// Organized this way to allow first couple words to be used
// for scratch space in the general case
//

    ULONG Arg3;                         // page faults only
    ULONG FaultStatus;                  // page faults only
    union {
        ULONG FaultAddress;             // page faults only
        ULONG TrapFrame;                // system services only
    };
    ULONG Reserved;                     // always valid, internal use

//
// Exception active indicator.
//
//    0 - interrupt frame.
//    1 - exception frame.
//    2 - service frame.
//

    UCHAR ExceptionActive;              // always valid
    UCHAR ContextFromKFramesUnwound;    // set if KeContextFromKFrames created this frame
    UCHAR DebugRegistersValid;          // always valid
    union {
        KPROCESSOR_MODE PreviousMode;   // system services only
        KIRQL PreviousIrql;             // interrupts only
    };

//
// The ARM architecture does not have an architectural trap frame.  On
// an exception or interrupt, the processor switches to an
// exception-specific processor mode in which at least the LR and SP
// registers are banked.  Software is responsible for preserving
// registers which reflect the processor state in which the
// exception occurred rather than any intermediate processor modes.
//

//
// Volatile floating point state is dynamically allocated; this
// pointer may be NULL if the FPU was not enabled at the time the
// trap was taken.
//

    PKARM_VFP_STATE VfpState;

//
// Debug registers
//

    ULONG Bvr[ARM_MAX_BREAKPOINTS];
    ULONG Bcr[ARM_MAX_BREAKPOINTS];
    ULONG Wvr[ARM_MAX_WATCHPOINTS];
    ULONG Wcr[ARM_MAX_WATCHPOINTS];

//
// Volatile registers R0-R3, R12, and the SP, LR
//

    ULONG R0;
    ULONG R1;
    ULONG R2;
    ULONG R3;
    ULONG R12;
    ULONG Sp;
    ULONG Lr;

//
// R11/PC for a frame chain, plus the saved CPSR
//

    ULONG R11;
    ULONG Pc;
    ULONG Cpsr;

} KTRAP_FRAME, *PKTRAP_FRAME;


//
// ARM Specific portions of Mm component.
//

#define PDE_BASE 0xC0300000
#define PTE_BASE 0xC0000000

#define PTE_TOP  0xC03FFFFF
#define PDE_TOP  0xC0300FFF

//
// Define the number of bits to shift to right justify the Page Directory Index
// field of a PTE.
//

#define PDI_SHIFT 22

//
// Define the number of bits to shift to right justify the Page Table Index
// field of a PTE.
//

#define PTI_SHIFT 12

//
// Define the number of PTEs and PDEs per page.
//

#define PTE_PER_PAGE (PAGE_SIZE / sizeof(ULONG))
#define PDE_PER_PAGE (PAGE_SIZE / sizeof(ULONG))

//
// Define the highest user address and user probe address.
//

extern NTKERNELAPI PVOID MmHighestUserAddress;
extern NTKERNELAPI PVOID MmSystemRangeStart;
extern NTKERNELAPI ULONG MmUserProbeAddress;

#define MM_HIGHEST_USER_ADDRESS MmHighestUserAddress
#define MM_SYSTEM_RANGE_START MmSystemRangeStart

//
// Allow non-kernel components to capture the user probe address and use a
// local copy for efficiency.
//

#if defined(_LOCAL_COPY_USER_PROBE_ADDRESS_)

#define MM_USER_PROBE_ADDRESS _LOCAL_COPY_USER_PROBE_ADDRESS_

#if defined(__CONVERGED_WIN32K_DRIVER__)
extern DLLBASEEXPORT ULONG _LOCAL_COPY_USER_PROBE_ADDRESS_;
#else
extern ULONG _LOCAL_COPY_USER_PROBE_ADDRESS_;
#endif

#else

#define MM_USER_PROBE_ADDRESS MmUserProbeAddress

#endif

#define MM_KSEG0_BASE       MM_SYSTEM_RANGE_START
#define MM_SYSTEM_SPACE_END 0xFFFFFFFF

//
// The lowest user address reserves the low 64k.
//

#define MM_LOWEST_USER_ADDRESS (PVOID)(LONG_PTR)0x10000


//
// Intrinsic functions
//

#if defined(_M_ARM) && !defined(RC_INVOKED)  && !defined(MIDL_PASS)

//
// The following routines are provided for backward compatibility with old
// code. They are no longer the preferred way to accomplish these functions.
//

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExInterlockedIncrementLong)      // Use InterlockedIncrement
#pragma deprecated(ExInterlockedDecrementLong)      // Use InterlockedDecrement
#pragma deprecated(ExInterlockedExchangeUlong)      // Use InterlockedExchange
#endif

#define RESULT_ZERO 0
#define RESULT_NEGATIVE 1
#define RESULT_POSITIVE 2

typedef enum _INTERLOCKED_RESULT {
    ResultNegative = RESULT_NEGATIVE,
    ResultZero = RESULT_ZERO,
    ResultPositive = RESULT_POSITIVE
} INTERLOCKED_RESULT;

#define ExInterlockedDecrementLong(Addend, Lock)                            \
    _ExInterlockedDecrementLong(Addend)

_Post_satisfies_(return >= 0 && return <= 2)
__forceinline
LONG
_ExInterlockedDecrementLong (
    _Inout_ _Interlocked_operand_ PLONG Addend
    )

{

    LONG Result;

    Result = InterlockedDecrement(Addend);
    if (Result < 0) {
        return ResultNegative;

    } else if (Result > 0) {
        return ResultPositive;

    } else {
        return ResultZero;
    }
}

#define ExInterlockedIncrementLong(Addend, Lock)                            \
    _ExInterlockedIncrementLong(Addend)

_Post_satisfies_(return >= 0 && return <= 2)
__forceinline
LONG
_ExInterlockedIncrementLong (
    _Inout_ _Interlocked_operand_ PLONG Addend
    )

{

    LONG Result;

    Result = InterlockedIncrement(Addend);
    if (Result < 0) {
        return ResultNegative;

    } else if (Result > 0) {
        return ResultPositive;

    } else {
        return ResultZero;
    }
}

#define ExInterlockedExchangeUlong(Target, Value, Lock)                     \
    _ExInterlockedExchangeUlong(Target, Value)

__forceinline
ULONG
_ExInterlockedExchangeUlong (
    _Inout_ _Interlocked_operand_ PULONG Target,
    _In_ ULONG Value
    )

{

    return (ULONG)InterlockedExchange((PLONG)Target, (LONG)Value);
}

#endif // defined(_M_ARM) && !defined(RC_INVOKED)  && !defined(MIDL_PASS)


#define CP15_PCR_RESERVED_MASK 0xFFF
#define KIPCR() ((ULONG_PTR)(_MoveFromCoprocessor(CP15_TPIDRPRW)) & ~CP15_PCR_RESERVED_MASK)



#if !defined(MIDL_PASS) && defined(_M_ARM)

//
// ARM function prototype definitions
//



//
// Get address of current processor block.
//

CFORCEINLINE
PKPCR
KeGetPcr (
    VOID
    )

{
    return (PKPCR)(ULONG_PTR)KIPCR();
}


#if (NTDDI_VERSION < NTDDI_WIN7) || !defined(NT_PROCESSOR_GROUPS)

//
// Get the current processor number
//

__forceinline
ULONG
KeGetCurrentProcessorNumber (
    VOID
    )

{

    return *(PUCHAR)(KIPCR() + 0x580);
}

#endif


#endif // !defined(MIDL_PASS) && defined(_M_ARM)


#endif // defined(_ARM_)


//
// Platform specific kernel functions to raise and lower IRQL.
//


#if defined(_ARM_) && !defined(MIDL_PASS)

_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_saves_
_IRQL_raises_(DISPATCH_LEVEL)
__forceinline
KIRQL
KeRaiseIrqlToDpcLevel (
    VOID
    )

/*++

Routine Description:

    This function raises the current IRQL to DPC_LEVEL and returns the
    previous IRQL.

Arguments:

    None.

Return Value:

    The previous IRQL is returned as the function value.

--*/

{

    return KfRaiseIrql(DISPATCH_LEVEL);
}

_IRQL_saves_
_IRQL_raises_(12)
__forceinline
KIRQL
KeRaiseIrqlToSynchLevel (
    VOID
    )

/*++

Routine Description:

    This function raises the current IRQL to SYNCH_LEVEL and returns the
    previous IRQL.

Arguments:

Return Value:

    The previous IRQL is returned as the function value.

--*/

{

    return KfRaiseIrql(12);
}


#endif // defined(_ARM_) && !defined(MIDL_PASS)

#if defined(_ARM64_) 

//
// ARM64 IDT Entry Structure Definition
//
// ARM64 does not have a native IDT format and does not vector interrupt
// exceptions.
//

//
// Processor Control Region Structure Definition
//

#define PCR_MINOR_VERSION 1
#define PCR_MAJOR_VERSION 1

typedef struct _KPCR {

//
// Start of the architecturally defined section of the PCR. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//
// Certain fields in the TIB are not used in kernel mode. These include the
// exception list, stack base, stack limit, subsystem TIB, fiber data, and
// the arbitrary user pointer. Therefore, these fields are overlaid with
// other data to get better cache locality.
//
// N.B. The offset to the PRCB in the PCR is fixed for all time.
//

    union {
        NT_TIB NtTib;                   // +000
        struct {
            PVOID TibPad0[2];           // +000
            PVOID Spare1;               // +010
            struct _KPCR *Self;         // +018
            PVOID PcrReserved0;         // +020
            PKSPIN_LOCK_QUEUE LockArray;// +028
            PVOID Used_Self;            // +030
        };
    };

    KIRQL CurrentIrql;                  // +038
    UCHAR SecondLevelCacheAssociativity;// +039
    UCHAR Pad1[2];                      // +03A
    USHORT MajorVersion;                // +03C
    USHORT MinorVersion;                // +03E
    ULONG StallScaleFactor;             // +040

    ULONG SecondLevelCacheSize;         // +044
    union {
        USHORT SoftwareInterruptPending;// +048 -- Software Interrupt Pending Flag
        struct {
            UCHAR ApcInterrupt;         // +048 -- 0x01 if APC int pending
            UCHAR DispatchInterrupt;    // +049 -- 0x01 if dispatch int pending
        };
    };
    USHORT InterruptPad;                // +04A

    ULONG64 PanicStorage[6];            // +050 -- Must be 16-byte aligned
    PVOID KdVersionBlock;               // +080
    PVOID HalReserved[15];              // +088


} KPCR, *PKPCR;

//
// Exception frame
//
//  This frame is established when handling an exception. It provides a place
//  to save all nonvolatile registers. The volatile registers will already
//  have been saved in a trap frame.
//
// N.B. The exception frame is preceded in the trap handling code by
//      an exception record capable of storing information for four
//      parameter values.
//

typedef struct _KEXCEPTION_FRAME {

//
// Saved nonvolatile registers.
//

    ULONG64 X19;
    ULONG64 X20;
    ULONG64 X21;
    ULONG64 X22;
    ULONG64 X23;
    ULONG64 X24;
    ULONG64 X25;
    ULONG64 X26;
    ULONG64 X27;
    ULONG64 X28;
    ULONG64 Fp;

    ULONG64 Return;

} KEXCEPTION_FRAME, *PKEXCEPTION_FRAME;

//
// VFP State -- specially organized collection of VFP state that
// is used in context switching. The volatile section is also used
// in trap frame handling and lazy VFP enabling.
//

typedef struct _KARM64_VFP_STATE
{
    struct _KARM64_VFP_STATE *Link;         // link to next state entry
    ULONG Fpcr;                             // FPCR register
    ULONG Fpsr;                             // FPSR register
    NEON128 V[32];                          // All V registers (0-31)
} KARM64_VFP_STATE, *PKARM64_VFP_STATE;

//
// Trap frame
//
// This frame is established when handling a trap. It provides a place to
// save all volatile registers. The nonvolatile registers are saved in an
// exception frame or through the normal C calling conventions for saved
// registers.
//
// Code to generate the trap frame is found in ntos\ke\arm\trap.asm.
//

#define KTRAP_FRAME_ARGUMENTS (10 * 8)       // up to 10 in-memory syscall args

#define KTRAP_FRAME_SIGNATURE ('prTK')       // KTrap Frame signature

typedef struct _KTRAP_FRAME {

//
// Exception active indicator.
//
//    0 - interrupt frame.
//    1 - exception frame.
//    2 - service frame.
//

    /* +0x000 */ UCHAR ExceptionActive;              // always valid
    /* +0x001 */ UCHAR ContextFromKFramesUnwound;    // set if KeContextFromKFrames created this frame
    /* +0x002 */ UCHAR DebugRegistersValid;          // always valid
    /* +0x003 */ union {
                     KPROCESSOR_MODE PreviousMode;   // system services only
                     KIRQL PreviousIrql;             // interrupts only
                 };

//
// Page fault information (page faults only)
// Previous trap frame address (system services only)
//
// Organized this way to allow first couple words to be used
// for scratch space in the general case
//

    /* +0x004 */ ULONG Reserved;                       // internal use
    /* +0x008 */ union {
                     ULONG64 FaultAddress;             // page faults only
                     ULONG64 TrapFrame;                // system services only
                 };

//
// The ARM architecture does not have an architectural trap frame.  On
// an exception or interrupt, the processor switches to an
// exception-specific processor mode in which at least the LR and SP
// registers are banked.  Software is responsible for preserving
// registers which reflect the processor state in which the
// exception occurred rather than any intermediate processor modes.
//

//
// Volatile floating point state is dynamically allocated; this
// pointer may be NULL if the FPU was not enabled at the time the
// trap was taken.
//

    /* +0x010 */ PKARM64_VFP_STATE VfpState;

//
// Debug registers
//

    /* +0x018 */ ULONG Bcr[ARM64_MAX_BREAKPOINTS];
    /* +0x038 */ ULONG64 Bvr[ARM64_MAX_BREAKPOINTS];
    /* +0x078 */ ULONG Wcr[ARM64_MAX_WATCHPOINTS];
    /* +0x080 */ ULONG64 Wvr[ARM64_MAX_WATCHPOINTS];

//
// Volatile registers X0-X17, and the FP, SP, LR
//

    /* +0x090 */ ULONG Spsr;
    /* +0x094 */ ULONG Esr;
    /* +0x098 */ ULONG64 Sp;
    /* +0x0A0 */ union {
        ULONG64 X[19];
        struct {
            ULONG64 X0;
            ULONG64 X1;
            ULONG64 X2;
            ULONG64 X3;
            ULONG64 X4;
            ULONG64 X5;
            ULONG64 X6;
            ULONG64 X7;
            ULONG64 X8;
            ULONG64 X9;
            ULONG64 X10;
            ULONG64 X11;
            ULONG64 X12;
            ULONG64 X13;
            ULONG64 X14;
            ULONG64 X15;
            ULONG64 X16;
            ULONG64 X17;
            ULONG64 X18;
        };
    };
    /* +0x138 */ ULONG64 Lr;
    /* +0x140 */ ULONG64 Fp;
    /* +0x148 */ ULONG64 Pc;
    /* +0x150 */

} KTRAP_FRAME, *PKTRAP_FRAME;


//
// Define the highest user address and user probe address.
//

extern const NTKERNELAPI PVOID MmHighestUserAddress;
extern const NTKERNELAPI PVOID MmSystemRangeStart;
extern const NTKERNELAPI ULONG64 MmUserProbeAddress;

#define MM_HIGHEST_USER_ADDRESS MmHighestUserAddress
#define MM_SYSTEM_RANGE_START MmSystemRangeStart

//
// Allow non-kernel components to capture the user probe address and use a
// local copy for efficiency.
//

#if defined(_LOCAL_COPY_USER_PROBE_ADDRESS_)

#define MM_USER_PROBE_ADDRESS _LOCAL_COPY_USER_PROBE_ADDRESS_

#if defined(__CONVERGED_WIN32K_DRIVER__)
extern DLLBASEEXPORT ULONG_PTR _LOCAL_COPY_USER_PROBE_ADDRESS_;
#else
extern ULONG_PTR _LOCAL_COPY_USER_PROBE_ADDRESS_;
#endif

#else

#define MM_USER_PROBE_ADDRESS MmUserProbeAddress

#endif

//
// The lowest user address reserves the low 64k.
//

#define MM_LOWEST_USER_ADDRESS (PVOID)(LONG_PTR)0x10000


//
// Intrinsic functions
//

#if defined(_M_ARM64) && !defined(RC_INVOKED)  && !defined(MIDL_PASS)

//
// The following routines are provided for backward compatibility with old
// code. They are no longer the preferred way to accomplish these functions.
//

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExInterlockedIncrementLong)      // Use InterlockedIncrement
#pragma deprecated(ExInterlockedDecrementLong)      // Use InterlockedDecrement
#pragma deprecated(ExInterlockedExchangeUlong)      // Use InterlockedExchange
#endif

#define RESULT_ZERO 0
#define RESULT_NEGATIVE 1
#define RESULT_POSITIVE 2

typedef enum _INTERLOCKED_RESULT {
    ResultNegative = RESULT_NEGATIVE,
    ResultZero = RESULT_ZERO,
    ResultPositive = RESULT_POSITIVE
} INTERLOCKED_RESULT;

#define ExInterlockedDecrementLong(Addend, Lock)                            \
    _ExInterlockedDecrementLong(Addend)

_Post_satisfies_(return >= 0 && return <= 2)
__forceinline
LONG
_ExInterlockedDecrementLong (
    _Inout_ _Interlocked_operand_ PLONG Addend
    )

{

    LONG Result;

    Result = InterlockedDecrement(Addend);
    if (Result < 0) {
        return ResultNegative;

    } else if (Result > 0) {
        return ResultPositive;

    } else {
        return ResultZero;
    }
}

#define ExInterlockedIncrementLong(Addend, Lock)                            \
    _ExInterlockedIncrementLong(Addend)

_Post_satisfies_(return >= 0 && return <= 2)
__forceinline
LONG
_ExInterlockedIncrementLong (
    _Inout_ _Interlocked_operand_ PLONG Addend
    )

{

    LONG Result;

    Result = InterlockedIncrement(Addend);
    if (Result < 0) {
        return ResultNegative;

    } else if (Result > 0) {
        return ResultPositive;

    } else {
        return ResultZero;
    }
}

#define ExInterlockedExchangeUlong(Target, Value, Lock)                     \
    _ExInterlockedExchangeUlong(Target, Value)

__forceinline
ULONG
_ExInterlockedExchangeUlong (
    _Inout_ _Interlocked_operand_ PULONG Target,
    _In_ ULONG Value
    )

{

    return (ULONG)InterlockedExchange((PLONG)Target, (LONG)Value);
}

#endif // defined(_M_ARM64) && !defined(RC_INVOKED)  && !defined(MIDL_PASS)


#define ARM64_PCR_RESERVED_MASK 0xFFF
#define KIPCR() ((ULONG_PTR)(_ReadStatusReg(ARM64_TPIDR_EL1)) & ~ARM64_PCR_RESERVED_MASK)



#if !defined(MIDL_PASS) && defined(_M_ARM64)

//
// ARM64 function prototype definitions
//



//
// Get address of current processor block.
//

CFORCEINLINE
PKPCR
KeGetPcr (
    VOID
    )

{
    return (PKPCR)__getReg(18);
}


#if (NTDDI_VERSION < NTDDI_WIN7) || !defined(NT_PROCESSOR_GROUPS)

//
// Get the current processor number
//

__forceinline
ULONG
KeGetCurrentProcessorNumber (
    VOID
    )

{

    return (ULONG)__readx18byte(0x980);
}

#endif


#endif // !defined(MIDL_PASS) && defined(_M_ARM64)


#endif // defined(_ARM64_)


//
// Platform specific kernel functions to raise and lower IRQL.
//


#if defined(_ARM64_) && !defined(MIDL_PASS)

_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_saves_
_IRQL_raises_(DISPATCH_LEVEL)
__forceinline
KIRQL
KeRaiseIrqlToDpcLevel (
    VOID
    )

/*++

Routine Description:

    This function raises the current IRQL to DPC_LEVEL and returns the
    previous IRQL.

Arguments:

    None.

Return Value:

    The previous IRQL is returned as the function value.

--*/

{

    return KfRaiseIrql(DISPATCH_LEVEL);
}

_IRQL_saves_
_IRQL_raises_(12)
__forceinline
KIRQL
KeRaiseIrqlToSynchLevel (
    VOID
    )

/*++

Routine Description:

    This function raises the current IRQL to SYNCH_LEVEL and returns the
    previous IRQL.

Arguments:

Return Value:

    The previous IRQL is returned as the function value.

--*/

{

    return KfRaiseIrql(12);
}


#endif // defined(_ARM64_) && !defined(MIDL_PASS)


//
// Firmware Table provider definitions
//

typedef enum _SYSTEM_FIRMWARE_TABLE_ACTION {
    SystemFirmwareTable_Enumerate,
    SystemFirmwareTable_Get
} SYSTEM_FIRMWARE_TABLE_ACTION;

typedef struct _SYSTEM_FIRMWARE_TABLE_INFORMATION {
    ULONG                           ProviderSignature;
    SYSTEM_FIRMWARE_TABLE_ACTION    Action;
    ULONG                           TableID;
    ULONG                           TableBufferLength;
    UCHAR                           TableBuffer[ANYSIZE_ARRAY];
} SYSTEM_FIRMWARE_TABLE_INFORMATION, *PSYSTEM_FIRMWARE_TABLE_INFORMATION;

typedef
NTSTATUS
(__cdecl *PFNFTH) (
    _Inout_ PSYSTEM_FIRMWARE_TABLE_INFORMATION SystemFirmwareTableInfo
    );

typedef struct _SYSTEM_FIRMWARE_TABLE_HANDLER {
    ULONG       ProviderSignature;
    BOOLEAN     Register;
    PFNFTH      FirmwareTableHandler;
    PVOID       DriverObject;
} SYSTEM_FIRMWARE_TABLE_HANDLER, *PSYSTEM_FIRMWARE_TABLE_HANDLER;

// end_access

//
// Timer APC routine definition.
//

typedef
VOID
(*PTIMER_APC_ROUTINE) (
    _In_ PVOID TimerContext,
    _In_ ULONG TimerLowValue,
    _In_ LONG TimerHighValue
    );

//
// Data structures used by NtSetTimerEx
//

typedef enum _TIMER_SET_INFORMATION_CLASS {
    TimerSetCoalescableTimer,
    MaxTimerInfoClass  // MaxTimerInfoClass should always be the last enum
} TIMER_SET_INFORMATION_CLASS;

#if (NTDDI_VERSION >= NTDDI_WIN7)

typedef struct _TIMER_SET_COALESCABLE_TIMER_INFO {
    _In_ LARGE_INTEGER DueTime;
    _In_opt_ PTIMER_APC_ROUTINE TimerApcRoutine;
    _In_opt_ PVOID TimerContext;
    _In_opt_ struct _COUNTED_REASON_CONTEXT *WakeContext;
    _In_opt_ ULONG Period;
    _In_ ULONG TolerableDelay;
    _Out_opt_ PBOOLEAN PreviousState;
} TIMER_SET_COALESCABLE_TIMER_INFO, *PTIMER_SET_COALESCABLE_TIMER_INFO;

#endif // (NTDDI_VERSION >= NTDDI_WIN7)


//
//  Driver Verifier Definitions
//

typedef
ULONG_PTR
(*PDRIVER_VERIFIER_THUNK_ROUTINE) (
    _In_ PVOID Context
    );

//
//  This structure is passed in by drivers that want to thunk callers of
//  their exports.
//

typedef struct _DRIVER_VERIFIER_THUNK_PAIRS {
    PDRIVER_VERIFIER_THUNK_ROUTINE  PristineRoutine;
    PDRIVER_VERIFIER_THUNK_ROUTINE  NewRoutine;
} DRIVER_VERIFIER_THUNK_PAIRS, *PDRIVER_VERIFIER_THUNK_PAIRS;

// begin_verifier_config
//
//  Driver Verifier flags.
//

#define DRIVER_VERIFIER_SPECIAL_POOLING             0x0001
#define DRIVER_VERIFIER_FORCE_IRQL_CHECKING         0x0002
#define DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES  0x0004
#define DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS      0x0008
#define DRIVER_VERIFIER_IO_CHECKING                 0x0010

// end_verifier_config

//
// Known extended CPU state feature BITs
//
// 0    x87
// 1    SSE
// 2    AVX
// 3    BNDREGS (B0.LB-B3.LB B0.UB-B3.UB)
// 4    BNDCSR  (BNDCFGU + BNDSTATUS)       Persistent
// 5    KMASK   (KMASK [63:0][0-7])
// 6    ZMM_H   (ZMM_H[511:256][0-15])
// 7    ZMM     (ZMM[511:0][16-31])
// 8    IPT                                 Supervisor
//
// 62   LWP                                 Persistent
//
// 63   RZ0                                 Reserved
//

#define XSTATE_LEGACY_FLOATING_POINT        (0)
#define XSTATE_LEGACY_SSE                   (1)
#define XSTATE_GSSE                         (2)
#define XSTATE_AVX                          (XSTATE_GSSE)
#define XSTATE_MPX_BNDREGS                  (3)
#define XSTATE_MPX_BNDCSR                   (4)
#define XSTATE_AVX512_KMASK                 (5)
#define XSTATE_AVX512_ZMM_H                 (6)
#define XSTATE_AVX512_ZMM                   (7)
#define XSTATE_IPT                          (8)
#define XSTATE_LWP                          (62)
#define MAXIMUM_XSTATE_FEATURES             (64)

//
// Known extended CPU state feature MASKs
//

#define XSTATE_MASK_LEGACY_FLOATING_POINT   (1ui64 << (XSTATE_LEGACY_FLOATING_POINT))
#define XSTATE_MASK_LEGACY_SSE              (1ui64 << (XSTATE_LEGACY_SSE))
#define XSTATE_MASK_LEGACY                  (XSTATE_MASK_LEGACY_FLOATING_POINT | \
                                             XSTATE_MASK_LEGACY_SSE)

#define XSTATE_MASK_GSSE                    (1ui64 << (XSTATE_GSSE))
#define XSTATE_MASK_AVX                     (XSTATE_MASK_GSSE)
#define XSTATE_MASK_MPX                     ((1ui64 << (XSTATE_MPX_BNDREGS)) | \
                                             (1ui64 << (XSTATE_MPX_BNDCSR)))

#define XSTATE_MASK_AVX512                  ((1ui64 << (XSTATE_AVX512_KMASK)) | \
                                             (1ui64 << (XSTATE_AVX512_ZMM_H)) | \
                                             (1ui64 << (XSTATE_AVX512_ZMM)))

#define XSTATE_MASK_IPT                     (1ui64 << (XSTATE_IPT))
#define XSTATE_MASK_LWP                     (1ui64 << (XSTATE_LWP))

#define XSTATE_MASK_ALLOWED                 (XSTATE_MASK_LEGACY | \
                                             XSTATE_MASK_AVX | \
                                             XSTATE_MASK_MPX | \
                                             XSTATE_MASK_AVX512 | \
                                             XSTATE_MASK_IPT | \
                                             XSTATE_MASK_LWP)

#define XSTATE_MASK_PERSISTENT              ((1ui64 << (XSTATE_MPX_BNDCSR)) | \
                                             XSTATE_MASK_LWP)

//
// Flags associated with compaction mask
//

#define XSTATE_COMPACTION_ENABLE            (63)
#define XSTATE_COMPACTION_ENABLE_MASK       (1ui64 << (XSTATE_COMPACTION_ENABLE))

#define XSTATE_ALIGN_BIT                    (1)
#define XSTATE_ALIGN_MASK                   (1ui64 << (XSTATE_ALIGN_BIT))

#define XSTATE_CONTROLFLAG_XSAVEOPT_MASK    (1)
#define XSTATE_CONTROLFLAG_XSAVEC_MASK      (2)
#define XSTATE_CONTROLFLAG_VALID_MASK       (XSTATE_CONTROLFLAG_XSAVEOPT_MASK | \
                                             XSTATE_CONTROLFLAG_XSAVEC_MASK)

//
// Extended processor state configuration
//

typedef struct _XSTATE_FEATURE {
    ULONG Offset;
    ULONG Size;
} XSTATE_FEATURE, *PXSTATE_FEATURE;

typedef struct _XSTATE_CONFIGURATION {
    // Mask of all enabled features
    ULONG64 EnabledFeatures;

    // Mask of volatile enabled features
    ULONG64 EnabledVolatileFeatures;

    // Total size of the save area for user states
    ULONG Size;

    // Control Flags
    union {
        ULONG ControlFlags;
        struct
        {
            ULONG OptimizedSave : 1;
            ULONG CompactionEnabled : 1;
        };
    };

    // List of features
    XSTATE_FEATURE Features[MAXIMUM_XSTATE_FEATURES];

    // Mask of all supervisor features
    ULONG64 EnabledSupervisorFeatures;

    // Mask of features that require start address to be 64 byte aligned
    ULONG64 AlignedFeatures;

    // Total size of the save area for user and supervisor states
    ULONG AllFeatureSize;

    // List which holds size of each user and supervisor state supported by CPU        
    ULONG AllFeatures[MAXIMUM_XSTATE_FEATURES];

} XSTATE_CONFIGURATION, *PXSTATE_CONFIGURATION;


//
// Define data shared between kernel and user mode.
//
// N.B. User mode has read only access to this data
//

#ifdef _MAC
#pragma warning( disable : 4121)
#endif

//
// WARNING: This structure must have exactly the same layout for 32- and
//    64-bit systems. The layout of this structure cannot change and new
//    fields can only be added at the end of the structure (unless a gap
//    can be exploited). Deprecated fields cannot be deleted. Platform
//    specific fields are included on all systems.
//
//    Layout exactness is required for Wow64 support of 32-bit applications
//    on Win64 systems.
//
//    The layout itself cannot change since this structure has been exported
//    in ntddk, ntifs.h, and nthal.h for some time.
//
// Define NX support policy values.
//

#define NX_SUPPORT_POLICY_ALWAYSOFF     0
#define NX_SUPPORT_POLICY_ALWAYSON      1
#define NX_SUPPORT_POLICY_OPTIN         2
#define NX_SUPPORT_POLICY_OPTOUT        3

//
// SEH chain validation policies.
//
// N.B. These constants must not be changed because the ldr relies on their
//      semantic meaning.
//

#define SEH_VALIDATION_POLICY_ON        0
#define SEH_VALIDATION_POLICY_OFF       1
#define SEH_VALIDATION_POLICY_TELEMETRY 2
#define SEH_VALIDATION_POLICY_DEFER     3

//
// Global shared data flags and manipulation macros.
//

#define SHARED_GLOBAL_FLAGS_ERROR_PORT_V                0x0
#define SHARED_GLOBAL_FLAGS_ERROR_PORT                  \
    (1UL << SHARED_GLOBAL_FLAGS_ERROR_PORT_V)

#define SHARED_GLOBAL_FLAGS_ELEVATION_ENABLED_V         0x1
#define SHARED_GLOBAL_FLAGS_ELEVATION_ENABLED           \
    (1UL << SHARED_GLOBAL_FLAGS_ELEVATION_ENABLED_V)

#define SHARED_GLOBAL_FLAGS_VIRT_ENABLED_V              0x2
#define SHARED_GLOBAL_FLAGS_VIRT_ENABLED                \
    (1UL << SHARED_GLOBAL_FLAGS_VIRT_ENABLED_V)

#define SHARED_GLOBAL_FLAGS_INSTALLER_DETECT_ENABLED_V  0x3
#define SHARED_GLOBAL_FLAGS_INSTALLER_DETECT_ENABLED    \
    (1UL << SHARED_GLOBAL_FLAGS_INSTALLER_DETECT_ENABLED_V)

#define SHARED_GLOBAL_FLAGS_LKG_ENABLED_V               0x4
#define SHARED_GLOBAL_FLAGS_LKG_ENABLED                 \
    (1UL << SHARED_GLOBAL_FLAGS_LKG_ENABLED_V)

#define SHARED_GLOBAL_FLAGS_DYNAMIC_PROC_ENABLED_V      0x5
#define SHARED_GLOBAL_FLAGS_DYNAMIC_PROC_ENABLED        \
    (1UL << SHARED_GLOBAL_FLAGS_DYNAMIC_PROC_ENABLED_V)

#define SHARED_GLOBAL_FLAGS_CONSOLE_BROKER_ENABLED_V    0x6
#define SHARED_GLOBAL_FLAGS_CONSOLE_BROKER_ENABLED      \
    (1UL << SHARED_GLOBAL_FLAGS_CONSOLE_BROKER_ENABLED_V)

#define SHARED_GLOBAL_FLAGS_SECURE_BOOT_ENABLED_V       0x7
#define SHARED_GLOBAL_FLAGS_SECURE_BOOT_ENABLED         \
    (1UL << SHARED_GLOBAL_FLAGS_SECURE_BOOT_ENABLED_V)

#define SHARED_GLOBAL_FLAGS_MULTI_SESSION_SKU_V         0x8
#define SHARED_GLOBAL_FLAGS_MULTI_SESSION_SKU           \
    (1UL << SHARED_GLOBAL_FLAGS_MULTI_SESSION_SKU_V)

#define SHARED_GLOBAL_FLAGS_MULTIUSERS_IN_SESSION_SKU_V 0x9
#define SHARED_GLOBAL_FLAGS_MULTIUSERS_IN_SESSION_SKU   \
    (1UL << SHARED_GLOBAL_FLAGS_MULTIUSERS_IN_SESSION_SKU_V)

#define SHARED_GLOBAL_FLAGS_STATE_SEPARATION_ENABLED_V 0xA
#define SHARED_GLOBAL_FLAGS_STATE_SEPARATION_ENABLED   \
    (1UL << SHARED_GLOBAL_FLAGS_STATE_SEPARATION_ENABLED_V)

#define EX_INIT_BITS(Flags, Bit) \
    *((Flags)) |= (Bit)             // Safe to use before concurrently accessible

#define EX_TEST_SET_BIT(Flags, Bit) \
    InterlockedBitTestAndSet ((PLONG)(Flags), (Bit))

#define EX_TEST_CLEAR_BIT(Flags, Bit) \
    InterlockedBitTestAndReset ((PLONG)(Flags), (Bit))

//
// Define legal values for the SystemCall member.
//

#define SYSTEM_CALL_SYSCALL 0
#define SYSTEM_CALL_INT_2E  1

//
// Define flags for QPC bypass information. None of these flags may be set
// unless bypass is enabled. This is for compat with existing code which
// compares this value to zero to detect bypass enablement.
//

#define SHARED_GLOBAL_FLAGS_QPC_BYPASS_ENABLED (0x01)
#define SHARED_GLOBAL_FLAGS_QPC_BYPASS_USE_HV_PAGE (0x02)
#define SHARED_GLOBAL_FLAGS_QPC_BYPASS_USE_MFENCE (0x10)
#define SHARED_GLOBAL_FLAGS_QPC_BYPASS_USE_LFENCE (0x20)
#define SHARED_GLOBAL_FLAGS_QPC_BYPASS_A73_ERRATA (0x40)
#define SHARED_GLOBAL_FLAGS_QPC_BYPASS_USE_RDTSCP (0x80)

//@[comment("MVI_tracked")]
typedef struct _KUSER_SHARED_DATA {

    //
    // Current low 32-bit of tick count and tick count multiplier.
    //
    // N.B. The tick count is updated each time the clock ticks.
    //

    ULONG TickCountLowDeprecated;
    ULONG TickCountMultiplier;

    //
    // Current 64-bit interrupt time in 100ns units.
    //

    volatile KSYSTEM_TIME InterruptTime;

    //
    // Current 64-bit system time in 100ns units.
    //

    volatile KSYSTEM_TIME SystemTime;

    //
    // Current 64-bit time zone bias.
    //

    volatile KSYSTEM_TIME TimeZoneBias;

    //
    // Support image magic number range for the host system.
    //
    // N.B. This is an inclusive range.
    //

    USHORT ImageNumberLow;
    USHORT ImageNumberHigh;

    //
    // Copy of system root in unicode.
    //
    // N.B. This field must be accessed via the RtlGetNtSystemRoot API for 
    //      an accurate result.
    //

    WCHAR NtSystemRoot[260];

    //
    // Maximum stack trace depth if tracing enabled.
    //

    ULONG MaxStackTraceDepth;

    //
    // Crypto exponent value.
    //

    ULONG CryptoExponent;

    //
    // Time zone ID.
    //

    ULONG TimeZoneId;
    ULONG LargePageMinimum;

    //
    // This value controls the AIT Sampling rate.
    //

    ULONG AitSamplingValue;

    //
    // This value controls switchback processing.
    //

    ULONG AppCompatFlag;

    //
    // Current Kernel Root RNG state seed version
    //

    ULONGLONG RNGSeedVersion;

    //
    // This value controls assertion failure handling.
    //

    ULONG GlobalValidationRunlevel;

    volatile LONG TimeZoneBiasStamp;

    //
    // The shared collective build number undecorated with C or F.
    // GetVersionEx hides the real number
    //

    ULONG NtBuildNumber;

    //
    // Product type.
    //
    // N.B. This field must be accessed via the RtlGetNtProductType API for 
    //      an accurate result.
    //

    NT_PRODUCT_TYPE NtProductType;
    BOOLEAN ProductTypeIsValid;
    BOOLEAN Reserved0[1];
    USHORT NativeProcessorArchitecture;

    //
    // The NT Version.
    //
    // N. B. Note that each process sees a version from its PEB, but if the
    //       process is running with an altered view of the system version,
    //       the following two fields are used to correctly identify the
    //       version
    //

    ULONG NtMajorVersion;
    ULONG NtMinorVersion;

    //
    // Processor features.
    //

    BOOLEAN ProcessorFeatures[PROCESSOR_FEATURE_MAX];

    //
    // Reserved fields - do not use.
    //

    ULONG Reserved1;
    ULONG Reserved3;

    //
    // Time slippage while in debugger.
    //

    volatile ULONG TimeSlip;

    //
    // Alternative system architecture, e.g., NEC PC98xx on x86.
    //

    ALTERNATIVE_ARCHITECTURE_TYPE AlternativeArchitecture;

    //
    // Boot sequence, incremented for each boot attempt by the OS loader.
    //

    ULONG BootId;

    //
    // If the system is an evaluation unit, the following field contains the
    // date and time that the evaluation unit expires. A value of 0 indicates
    // that there is no expiration. A non-zero value is the UTC absolute time
    // that the system expires.
    //

    LARGE_INTEGER SystemExpirationDate;

    //
    // Suite support.
    //
    // N.B. This field must be accessed via the RtlGetSuiteMask API for 
    //      an accurate result.
    //

    ULONG SuiteMask;

    //
    // TRUE if a kernel debugger is connected/enabled.
    //

    BOOLEAN KdDebuggerEnabled;

    //
    // Mitigation policies.
    //

    union
    {
        UCHAR MitigationPolicies;
        struct
        {
            UCHAR NXSupportPolicy : 2;
            UCHAR SEHValidationPolicy : 2;
            UCHAR CurDirDevicesSkippedForDlls : 2;
            UCHAR Reserved : 2;
        };
    };

    //
    // Two bytes of padding here -- offsets 0x2d6, 0x2d7
    //

    UCHAR Reserved6[2];

    //
    // Current console session Id. Always zero on non-TS systems.
    //
    // N.B. This field must be accessed via the RtlGetActiveConsoleId API for an
    //      accurate result.
    //

    volatile ULONG ActiveConsoleId;

    //
    // Force-dismounts cause handles to become invalid. Rather than always
    // probe handles, a serial number of dismounts is maintained that clients
    // can use to see if they need to probe handles.
    //

    volatile ULONG DismountCount;

    //
    // This field indicates the status of the 64-bit COM+ package on the
    // system. It indicates whether the Itermediate Language (IL) COM+
    // images need to use the 64-bit COM+ runtime or the 32-bit COM+ runtime.
    //

    ULONG ComPlusPackage;

    //
    // Time in tick count for system-wide last user input across all terminal
    // sessions. For MP performance, it is not updated all the time (e.g. once
    // a minute per session). It is used for idle detection.
    //

    ULONG LastSystemRITEventTickCount;

    //
    // Number of physical pages in the system. This can dynamically change as
    // physical memory can be added or removed from a running system.
    //

    ULONG NumberOfPhysicalPages;

    //
    // True if the system was booted in safe boot mode.
    //

    BOOLEAN SafeBootMode;

    //
    // Virtualization flags
    //

    UCHAR VirtualizationFlags;

    //
    // Reserved (available for reuse).
    //

    UCHAR Reserved12[2];

    //
    // This is a packed bitfield that contains various flags concerning
    // the system state. They must be manipulated using interlocked
    // operations.
    //
    // N.B. DbgMultiSessionSku must be accessed via the RtlIsMultiSessionSku
    //      API for an accurate result
    //

    union {
        ULONG SharedDataFlags;
        struct {

            //
            // The following bit fields are for the debugger only. Do not use.
            // Use the bit definitions instead.
            //

            ULONG DbgErrorPortPresent       : 1;
            ULONG DbgElevationEnabled       : 1;
            ULONG DbgVirtEnabled            : 1;
            ULONG DbgInstallerDetectEnabled : 1;
            ULONG DbgLkgEnabled             : 1;
            ULONG DbgDynProcessorEnabled    : 1;
            ULONG DbgConsoleBrokerEnabled   : 1;
            ULONG DbgSecureBootEnabled      : 1;
            ULONG DbgMultiSessionSku        : 1;
            ULONG DbgMultiUsersInSessionSku : 1;
            ULONG DbgStateSeparationEnabled : 1;
            ULONG SpareBits                 : 21;
        } DUMMYSTRUCTNAME2;
    } DUMMYUNIONNAME2;

    ULONG DataFlagsPad[1];

    //
    // Depending on the processor, the code for fast system call will differ,
    // Stub code is provided pointers below to access the appropriate code.
    //
    // N.B. The following field is only used on 32-bit systems.
    //

    ULONGLONG TestRetInstruction;
    LONGLONG QpcFrequency;

    //
    // On AMD64, this value is initialized to a nonzero value if the system
    // operates with an altered view of the system service call mechanism.
    //

    ULONG SystemCall;

    //
    // Reserved, available for reuse.
    //

    ULONG SystemCallPad0;
    ULONGLONG SystemCallPad[2];

    //
    // The 64-bit tick count.
    //

    union {
        volatile KSYSTEM_TIME TickCount;
        volatile ULONG64 TickCountQuad;
        struct {
            ULONG ReservedTickCountOverlay[3];
            ULONG TickCountPad[1];
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME3;

    //
    // Cookie for encoding pointers system wide.
    //

    ULONG Cookie;
    ULONG CookiePad[1];

    //
    // Client id of the process having the focus in the current
    // active console session id.
    //
    // N.B. This field must be accessed via the
    //      RtlGetConsoleSessionForegroundProcessId API for an accurate result.
    //

    LONGLONG ConsoleSessionForegroundProcessId;

    //
    // N.B. The following data is used to implement the precise time
    //      services. It is aligned on a 64-byte cache-line boundary and
    //      arranged in the order of typical accesses.
    //
    // Placeholder for the (internal) time update lock.
    //

    ULONGLONG TimeUpdateLock;

    //
    // The performance counter value used to establish the current system time.
    //

    ULONGLONG BaselineSystemTimeQpc;

    //
    // The performance counter value used to compute the last interrupt time.
    //

    ULONGLONG BaselineInterruptTimeQpc;

    //
    // The scaled number of system time seconds represented by a single
    // performance count (this value may vary to achieve time synchronization).
    //

    ULONGLONG QpcSystemTimeIncrement;

    //
    // The scaled number of interrupt time seconds represented by a single
    // performance count (this value is constant after the system is booted).
    //

    ULONGLONG QpcInterruptTimeIncrement;

    //
    // The scaling shift count applied to the performance counter system time
    // increment.
    //

    UCHAR QpcSystemTimeIncrementShift;

    //
    // The scaling shift count applied to the performance counter interrupt time
    // increment.
    //

    UCHAR QpcInterruptTimeIncrementShift;

    //
    // The count of unparked processors.
    //

    USHORT UnparkedProcessorCount;
    
    //
    // A bitmask of enclave features supported on this system.
    //
    
    ULONG EnclaveFeatureMask[4];

    //
    // Current coverage round for telemetry based coverage.
    //

    ULONG TelemetryCoverageRound;

    //
    // The following field is used for ETW user mode global logging
    // (UMGL).
    //

    USHORT UserModeGlobalLogger[16];

    //
    // Settings that can enable the use of Image File Execution Options
    // from HKCU in addition to the original HKLM.
    //

    ULONG ImageFileExecutionOptions;

    //
    // Generation of the kernel structure holding system language information
    //

    ULONG LangGenerationCount;

    //
    // Reserved (available for reuse).
    //

    ULONGLONG Reserved4;

    //
    // Current 64-bit interrupt time bias in 100ns units.
    //

    volatile ULONGLONG InterruptTimeBias;

    //
    // Current 64-bit performance counter bias, in performance counter units
    // before the shift is applied.
    //

    volatile ULONGLONG QpcBias;

    //
    // Number of active processors and groups.
    //

    ULONG ActiveProcessorCount;
    volatile UCHAR ActiveGroupCount;

    //
    // Reserved (available for re-use).
    //

    UCHAR Reserved9;

    union {
        USHORT QpcData;
        struct {

            //
            // A boolean indicating whether performance counter queries
            // can read the counter directly (bypassing the system call).
            //

            volatile UCHAR QpcBypassEnabled;

            //
            // Shift applied to the raw counter value to derive the
            // QPC count.
            //

            UCHAR QpcShift;
        };
    };

    LARGE_INTEGER TimeZoneBiasEffectiveStart;
    LARGE_INTEGER TimeZoneBiasEffectiveEnd;

    //
    // Extended processor state configuration
    //

    XSTATE_CONFIGURATION XState;

} KUSER_SHARED_DATA, *PKUSER_SHARED_DATA;

//
// Mostly enforce earlier comment about the stability and
// architecture-neutrality of this struct.
//

#if !defined(__midl) && !defined(MIDL_PASS)

//
// Assembler logic assumes a zero value for syscall and a nonzero value for
// int 2e, and that no other values exist presently for the SystemCall field.
//

C_ASSERT(SYSTEM_CALL_SYSCALL == 0);
C_ASSERT(SYSTEM_CALL_INT_2E == 1);

//
// The overall size can change, but it must be the same for all architectures.
//

C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, TickCountLowDeprecated) == 0x0);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, TickCountMultiplier) == 0x4);
C_ASSERT(__alignof(KSYSTEM_TIME) == 4);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, InterruptTime) == 0x08);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, SystemTime) == 0x014);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, TimeZoneBias) == 0x020);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, ImageNumberLow) == 0x02c);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, ImageNumberHigh) == 0x02e);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, NtSystemRoot) == 0x030);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, MaxStackTraceDepth) == 0x238);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, CryptoExponent) == 0x23c);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, TimeZoneId) == 0x240);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, LargePageMinimum) == 0x244);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, AitSamplingValue) == 0x248);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, AppCompatFlag) == 0x24c);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, RNGSeedVersion) == 0x250);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, GlobalValidationRunlevel) == 0x258);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, TimeZoneBiasStamp) == 0x25c);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, NtBuildNumber) == 0x260);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, NtProductType) == 0x264);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, ProductTypeIsValid) == 0x268);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, NativeProcessorArchitecture) == 0x26a);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, NtMajorVersion) == 0x26c);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, NtMinorVersion) == 0x270);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, ProcessorFeatures) == 0x274);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, Reserved1) == 0x2b4);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, Reserved3) == 0x2b8);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, TimeSlip) == 0x2bc);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, AlternativeArchitecture) == 0x2c0);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, SystemExpirationDate) == 0x2c8);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, SuiteMask) == 0x2d0);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, KdDebuggerEnabled) == 0x2d4);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, MitigationPolicies) == 0x2d5);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, ActiveConsoleId) == 0x2d8);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, DismountCount) == 0x2dc);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, ComPlusPackage) == 0x2e0);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, LastSystemRITEventTickCount) == 0x2e4);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, NumberOfPhysicalPages) == 0x2e8);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, SafeBootMode) == 0x2ec);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, VirtualizationFlags) == 0x2ed);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, Reserved12) == 0x2ee);

#if defined(_MSC_EXTENSIONS)

C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, SharedDataFlags) == 0x2f0);

#endif

C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, TestRetInstruction) == 0x2f8);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, QpcFrequency) == 0x300);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, SystemCall) == 0x308);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, SystemCallPad0) == 0x30c);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, SystemCallPad) == 0x310);

#if defined(_MSC_EXTENSIONS)

C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, TickCount) == 0x320);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, TickCountQuad) == 0x320);

#endif

C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, Cookie) == 0x330);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, ConsoleSessionForegroundProcessId) == 0x338);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, TimeUpdateLock) == 0x340);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, BaselineSystemTimeQpc) == 0x348);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, BaselineInterruptTimeQpc) == 0x350);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, QpcSystemTimeIncrement) == 0x358);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, QpcInterruptTimeIncrement) == 0x360);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, QpcSystemTimeIncrementShift) == 0x368);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, QpcInterruptTimeIncrementShift) == 0x369);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, UnparkedProcessorCount) == 0x36a);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, EnclaveFeatureMask) == 0x36c);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, TelemetryCoverageRound) == 0x37c);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, UserModeGlobalLogger) == 0x380);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, ImageFileExecutionOptions) == 0x3a0);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, LangGenerationCount) == 0x3a4);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, Reserved4) == 0x3a8);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, InterruptTimeBias) == 0x3b0);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, QpcBias) == 0x3b8);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, ActiveProcessorCount) == 0x3c0);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, ActiveGroupCount) == 0x3c4);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, Reserved9) == 0x3c5);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, QpcData) == 0x3c6);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, QpcBypassEnabled) == 0x3c6);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, QpcShift) == 0x3c7);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, TimeZoneBiasEffectiveStart) == 0x3c8);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, TimeZoneBiasEffectiveEnd) == 0x3d0);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, XState) == 0x3d8);
C_ASSERT(sizeof(KUSER_SHARED_DATA) == 0x708);

#endif /* __midl | MIDL_PASS */

#ifdef _MAC
#pragma warning(default:4121)
#endif

#define CmResourceTypeMaximum             8
//
// Declaration of the structure for the PcCard ISA IRQ map
//

typedef struct _CM_PCCARD_DEVICE_DATA {
    UCHAR Flags;
    UCHAR ErrorCode;
    USHORT Reserved;
    ULONG BusData;
    ULONG DeviceId;
    ULONG LegacyBaseAddress;
    UCHAR IRQMap[16];
} CM_PCCARD_DEVICE_DATA, *PCM_PCCARD_DEVICE_DATA;

// Definitions for Flags

#define PCCARD_MAP_ERROR        0x01
#define PCCARD_DEVICE_PCI       0x10

#define PCCARD_SCAN_DISABLED    0x01
#define PCCARD_MAP_ZERO         0x02
#define PCCARD_NO_TIMER         0x03
#define PCCARD_NO_PIC           0x04
#define PCCARD_NO_LEGACY_BASE   0x05
#define PCCARD_DUP_LEGACY_BASE  0x06
#define PCCARD_NO_CONTROLLERS   0x07

#ifndef _ARC_DDK_
#define _ARC_DDK_

//
// Define configuration routine types.
//
// Configuration information.
//

typedef enum _CONFIGURATION_TYPE {
    ArcSystem,
    CentralProcessor,
    FloatingPointProcessor,
    PrimaryIcache,
    PrimaryDcache,
    SecondaryIcache,
    SecondaryDcache,
    SecondaryCache,
    EisaAdapter,
    TcAdapter,
    ScsiAdapter,
    DtiAdapter,
    MultiFunctionAdapter,
    DiskController,
    TapeController,
    CdromController,
    WormController,
    SerialController,
    NetworkController,
    DisplayController,
    ParallelController,
    PointerController,
    KeyboardController,
    AudioController,
    OtherController,
    DiskPeripheral,
    FloppyDiskPeripheral,
    TapePeripheral,
    ModemPeripheral,
    MonitorPeripheral,
    PrinterPeripheral,
    PointerPeripheral,
    KeyboardPeripheral,
    TerminalPeripheral,
    OtherPeripheral,
    LinePeripheral,
    NetworkPeripheral,
    SystemMemory,
    DockingInformation,
    RealModeIrqRoutingTable,
    RealModePCIEnumeration,
    MaximumType
} CONFIGURATION_TYPE, *PCONFIGURATION_TYPE;

#endif // _ARC_DDK_

//
// Several routines have an architecture specific implementation. Generate
// an error if a supported target is not defined.
//

#if !(defined(_X86_) || defined(_AMD64_) || defined(_ARM_) || defined(_ARM64_))

#error "No target architecture defined"

#endif

#if (NTDDI_VERSION < NTDDI_WIN7) || defined(_X86_) || !defined(NT_PROCESSOR_GROUPS)

#define SINGLE_GROUP_LEGACY_API 1

#endif


//

#if defined(_X86_) || defined(_AMD64_)

#define PAUSE_PROCESSOR YieldProcessor();

#elif defined(_ARM_) || defined(_ARM64_)

#define PAUSE_PROCESSOR __yield();

#endif


#if (NTDDI_VERSION >= NTDDI_WS03SP1)
NTKERNELAPI
NTSTATUS
KeInitializeCrashDumpHeader(
    _In_ ULONG DumpType,
    _In_ ULONG Flags,
    _Out_writes_bytes_(BufferSize) PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_opt_ PULONG BufferNeeded
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTKERNELAPI
VOID
KeSetImportanceDpc (
    _Inout_ PRKDPC Dpc,
    _In_ KDPC_IMPORTANCE Importance
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K) && defined(SINGLE_GROUP_LEGACY_API)
NTKERNELAPI
VOID
KeSetTargetProcessorDpc (
    _Inout_ PRKDPC Dpc,
    _In_ CCHAR Number
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
LONG
KePulseEvent (
    _Inout_ PRKEVENT Event,
    _In_ KPRIORITY Increment,
    _In_ BOOLEAN Wait
    );
#endif


#define MAXIMUM_EXPANSION_SIZE (KERNEL_LARGE_STACK_SIZE - (PAGE_SIZE / 2))

typedef
_IRQL_requires_same_
_Function_class_(EXPAND_STACK_CALLOUT)
VOID
(NTAPI EXPAND_STACK_CALLOUT) (
    _In_opt_ PVOID Parameter
    );

typedef EXPAND_STACK_CALLOUT *PEXPAND_STACK_CALLOUT;

#if (NTDDI_VERSION >= NTDDI_WS03SP1)
_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
KeExpandKernelStackAndCallout (
    _In_ PEXPAND_STACK_CALLOUT Callout,
    _In_opt_ PVOID Parameter,
    _In_ SIZE_T Size
    );

#endif



#if (NTDDI_VERSION >= NTDDI_VISTA)
_Must_inspect_result_
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
__drv_reportError("DISPATCH_LEVEL is only supported on Windows 7 or later.")
NTKERNELAPI
NTSTATUS
KeExpandKernelStackAndCalloutEx (
    _In_ PEXPAND_STACK_CALLOUT Callout,
    _In_opt_ PVOID Parameter,
    _In_ SIZE_T Size,
    _In_ BOOLEAN Wait,
    _In_opt_ PVOID Context
    );
#endif



#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
LONG
KeSetBasePriorityThread (
    _Inout_ PKTHREAD Thread,
    _In_ LONG Increment
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WIN2K)
_Acquires_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
KeEnterCriticalRegion (
    VOID
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_Releases_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
KeLeaveCriticalRegion (
    VOID
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WS03SP1)
_Acquires_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
KeEnterGuardedRegion (
    VOID
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WS03SP1)
_Releases_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
KeLeaveGuardedRegion (
    VOID
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
BOOLEAN
KeAreApcsDisabled (
    VOID
    );
#endif



#if (NTDDI_VERSION >= NTDDI_WIN2K)
__drv_preferredFunction("error logging or driver shutdown",
    "Whenever possible, all kernel-mode components should log an error and "
    "continue to run, rather than calling KeBugCheck")
NTKERNELAPI
DECLSPEC_NORETURN
VOID
NTAPI
KeBugCheck (
    _In_ ULONG BugCheckCode
    );
#endif



#if !defined(_ARM64_)

#if (NTDDI_VERSION >= NTDDI_WS03)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
BOOLEAN
KeInvalidateAllCaches (
    VOID
    );
#endif

#endif

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
FASTCALL
KeInvalidateRangeAllCaches (
    _In_ PVOID BaseAddress,
    _In_ ULONG Length
    );


#if (NTDDI_VERSION >= NTDDI_WIN2K) && defined(SINGLE_GROUP_LEGACY_API)
NTKERNELAPI
KAFFINITY
KeQueryActiveProcessors (
    VOID
    );
#endif

#if (NTDDI_VERSION >= NTDDI_VISTA) && defined(SINGLE_GROUP_LEGACY_API)
NTKERNELAPI
ULONG
KeQueryActiveProcessorCount (
    _Out_opt_ PKAFFINITY ActiveProcessors
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTKERNELAPI
ULONG
KeQueryActiveProcessorCountEx (
    _In_ USHORT GroupNumber
    );
#endif

#if (NTDDI_VERSION >= NTDDI_LONGHORN) && defined(SINGLE_GROUP_LEGACY_API)
NTKERNELAPI
ULONG
KeQueryMaximumProcessorCount (
    VOID
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTKERNELAPI
ULONG
KeQueryMaximumProcessorCountEx (
    _In_ USHORT GroupNumber
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTKERNELAPI
USHORT
KeQueryActiveGroupCount (
    VOID
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTKERNELAPI
USHORT
KeQueryMaximumGroupCount (
    VOID
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTKERNELAPI
KAFFINITY
KeQueryGroupAffinity (
    _In_ USHORT GroupNumber
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTKERNELAPI
ULONG
KeGetCurrentProcessorNumberEx (
    _Out_opt_ PPROCESSOR_NUMBER ProcNumber
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTKERNELAPI
VOID
KeQueryNodeActiveAffinity (
    _In_ USHORT NodeNumber,
    _Out_opt_ PGROUP_AFFINITY Affinity,
    _Out_opt_ PUSHORT Count
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTKERNELAPI
USHORT
KeQueryNodeMaximumProcessorCount (
    _In_ USHORT NodeNumber
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTKERNELAPI
USHORT
KeQueryHighestNodeNumber (
    VOID
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTKERNELAPI
USHORT
KeGetCurrentNodeNumber (
    VOID
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
_IRQL_requires_max_(DISPATCH_LEVEL)
_When_(_Old_(*Length) == 0, _Post_satisfies_(return < 0))
NTKERNELAPI
NTSTATUS
KeQueryLogicalProcessorRelationship (
    _In_opt_ PPROCESSOR_NUMBER ProcessorNumber,
    _In_ LOGICAL_PROCESSOR_RELATIONSHIP RelationshipType,
    _Out_writes_bytes_opt_(*Length) PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Information,
    _Always_(_Inout_) PULONG Length
    );
#endif

_IRQL_requires_min_(DISPATCH_LEVEL)
NTKERNELAPI
LOGICAL
KeShouldYieldProcessor (
    VOID
    );


#if (NTDDI_VERSION >= NTDDI_WIN7)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
KeSetHardwareCounterConfiguration (
    _In_reads_(Count) PHARDWARE_COUNTER CounterArray,
    _In_ ULONG Count
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
KeQueryHardwareCounterConfiguration (
    _Out_writes_to_(MaximumCount, *Count) PHARDWARE_COUNTER CounterArray,
    _In_ ULONG MaximumCount,
    _Out_ PULONG Count
    );
#endif


#define CP_GET_SUCCESS  0
#define CP_GET_NODATA   1
#define CP_GET_ERROR    2


#if defined(POOL_TAGGING)
#define ExFreePool(a) ExFreePoolWithTag (a,0)
#endif

//
// PROTECTED_POOL is deprecated, do not use.
//

#define PROTECTED_POOL 0x0


#if (NTDDI_VERSION >= NTDDI_WIN2K)

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
DECLSPEC_NORETURN
VOID
ExRaiseDatatypeMisalignment (
    VOID
    );

#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
DECLSPEC_NORETURN
VOID
ExRaiseAccessViolation (
    VOID
    );

#endif


//
// Zone Allocation
//

typedef struct _ZONE_SEGMENT_HEADER {
    SINGLE_LIST_ENTRY SegmentList;
    PVOID Reserved;
} ZONE_SEGMENT_HEADER, *PZONE_SEGMENT_HEADER;

typedef struct _ZONE_HEADER {
    SINGLE_LIST_ENTRY FreeList;
    SINGLE_LIST_ENTRY SegmentList;
    ULONG BlockSize;
    ULONG TotalSegmentSize;
} ZONE_HEADER, *PZONE_HEADER;

#if (NTDDI_VERSION >= NTDDI_WIN2K)

__drv_preferredFunction("lookaside lists instead", "Obsolete")
DECLSPEC_DEPRECATED_DDK
NTKERNELAPI
NTSTATUS
ExInitializeZone(
    _Out_ PZONE_HEADER Zone,
    _In_ ULONG BlockSize,
    _Inout_ PVOID InitialSegment,
    _In_ ULONG InitialSegmentSize
    );

#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)

__drv_preferredFunction("lookaside lists instead", "Obsolete")
DECLSPEC_DEPRECATED_DDK
NTKERNELAPI
NTSTATUS
ExExtendZone(
    _Inout_ PZONE_HEADER Zone,
    _Inout_ PVOID Segment,
    _In_ ULONG SegmentSize
    );

#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)

_IRQL_requires_max_(DISPATCH_LEVEL)
__drv_preferredFunction("lookaside lists instead", "Obsolete")
DECLSPEC_DEPRECATED_DDK
NTKERNELAPI
NTSTATUS
ExInterlockedExtendZone(
    _Inout_ PZONE_HEADER Zone,
    _Inout_ PVOID Segment,
    _In_ ULONG SegmentSize,
    _Inout_ _Requires_lock_not_held_(*_Curr_) PKSPIN_LOCK Lock
    );

#endif

//++
//
// PVOID
// ExAllocateFromZone(
//     IN PZONE_HEADER Zone
//     )
//
// Routine Description:
//
//     This routine removes an entry from the zone and returns a pointer to it.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage from which the
//         entry is to be allocated.
//
// Return Value:
//
//     The function value is a pointer to the storage allocated from the zone.
//
//--

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExAllocateFromZone)
#endif

#define ExAllocateFromZone(Zone) \
    (PVOID)((Zone)->FreeList.Next); \
    if ( (Zone)->FreeList.Next ) (Zone)->FreeList.Next = (Zone)->FreeList.Next->Next

//++
//
// PVOID
// ExFreeToZone(
//     IN PZONE_HEADER Zone,
//     IN PVOID Block
//     )
//
// Routine Description:
//
//     This routine places the specified block of storage back onto the free
//     list in the specified zone.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage to which the
//         entry is to be inserted.
//
//     Block - Pointer to the block of storage to be freed back to the zone.
//
// Return Value:
//
//     Pointer to previous block of storage that was at the head of the free
//         list.  NULL implies the zone went from no available free blocks to
//         at least one free block.
//
//--

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExFreeToZone)
#endif

#define ExFreeToZone(Zone,Block)                                    \
    ( ((PSINGLE_LIST_ENTRY)(Block))->Next = (Zone)->FreeList.Next,  \
      (Zone)->FreeList.Next = ((PSINGLE_LIST_ENTRY)(Block)),        \
      ((PSINGLE_LIST_ENTRY)(Block))->Next                           \
    )

//++
//
// BOOLEAN
// ExIsFullZone(
//     IN PZONE_HEADER Zone
//     )
//
// Routine Description:
//
//     This routine determines if the specified zone is full or not.  A zone
//     is considered full if the free list is empty.
//
// Arguments:
//
//     Zone - Pointer to the zone header to be tested.
//
// Return Value:
//
//     TRUE if the zone is full and FALSE otherwise.
//
//--

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExIsFullZone)
#endif

#define ExIsFullZone(Zone) \
    ( (Zone)->FreeList.Next == (PSINGLE_LIST_ENTRY)NULL )

//++
//
// PVOID
// ExInterlockedAllocateFromZone(
//     IN PZONE_HEADER Zone,
//     IN PKSPIN_LOCK Lock
//     )
//
// Routine Description:
//
//     This routine removes an entry from the zone and returns a pointer to it.
//     The removal is performed with the specified lock owned for the sequence
//     to make it MP-safe.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage from which the
//         entry is to be allocated.
//
//     Lock - Pointer to the spin lock which should be obtained before removing
//         the entry from the allocation list.  The lock is released before
//         returning to the caller.
//
// Return Value:
//
//     The function value is a pointer to the storage allocated from the zone.
//
//--

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExInterlockedAllocateFromZone)
#endif

#define ExInterlockedAllocateFromZone(Zone,Lock) \
    (PVOID) ExInterlockedPopEntryList( &(Zone)->FreeList, Lock )

//++
//
// PVOID
// ExInterlockedFreeToZone(
//     IN PZONE_HEADER Zone,
//     IN PVOID Block,
//     IN PKSPIN_LOCK Lock
//     )
//
// Routine Description:
//
//     This routine places the specified block of storage back onto the free
//     list in the specified zone.  The insertion is performed with the lock
//     owned for the sequence to make it MP-safe.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage to which the
//         entry is to be inserted.
//
//     Block - Pointer to the block of storage to be freed back to the zone.
//
//     Lock - Pointer to the spin lock which should be obtained before inserting
//         the entry onto the free list.  The lock is released before returning
//         to the caller.
//
// Return Value:
//
//     Pointer to previous block of storage that was at the head of the free
//         list.  NULL implies the zone went from no available free blocks to
//         at least one free block.
//
//--

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExInterlockedFreeToZone)
#endif

#define ExInterlockedFreeToZone(Zone,Block,Lock) \
    ExInterlockedPushEntryList( &(Zone)->FreeList, ((PSINGLE_LIST_ENTRY) (Block)), Lock )


//++
//
// BOOLEAN
// ExIsObjectInFirstZoneSegment(
//     IN PZONE_HEADER Zone,
//     IN PVOID Object
//     )
//
// Routine Description:
//
//     This routine determines if the specified pointer lives in the zone.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage to which the
//         object may belong.
//
//     Object - Pointer to the object in question.
//
// Return Value:
//
//     TRUE if the Object came from the first segment of zone.
//
//--

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExIsObjectInFirstZoneSegment)
#endif

#define ExIsObjectInFirstZoneSegment(Zone,Object) ((BOOLEAN)     \
    (((PUCHAR)(Object) >= (PUCHAR)(Zone)->SegmentList.Next) &&   \
     ((PUCHAR)(Object) < (PUCHAR)(Zone)->SegmentList.Next +      \
                         (Zone)->TotalSegmentSize))              \
)


//
//  ntddk.h stole the entrypoints we wanted so fix them up here.
//

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExInitializeResource)            // use ExInitializeResourceLite
#pragma deprecated(ExAcquireResourceShared)         // use ExAcquireResourceSharedLite
#pragma deprecated(ExAcquireResourceExclusive)      // use ExAcquireResourceExclusiveLite
#pragma deprecated(ExReleaseResourceForThread)      // use ExReleaseResourceForThreadLite
#pragma deprecated(ExConvertExclusiveToShared)      // use ExConvertExclusiveToSharedLite
#pragma deprecated(ExDeleteResource)                // use ExDeleteResourceLite
#pragma deprecated(ExIsResourceAcquiredExclusive)   // use ExIsResourceAcquiredExclusiveLite
#pragma deprecated(ExIsResourceAcquiredShared)      // use ExIsResourceAcquiredSharedLite
#pragma deprecated(ExIsResourceAcquired)            // use ExIsResourceAcquiredSharedLite
#endif
#define ExInitializeResource ExInitializeResourceLite
#define ExAcquireResourceShared ExAcquireResourceSharedLite
#define ExAcquireResourceExclusive ExAcquireResourceExclusiveLite
#define ExReleaseResourceForThread ExReleaseResourceForThreadLite
#define ExConvertExclusiveToShared ExConvertExclusiveToSharedLite
#define ExDeleteResource ExDeleteResourceLite
#define ExIsResourceAcquiredExclusive ExIsResourceAcquiredExclusiveLite
#define ExIsResourceAcquiredShared ExIsResourceAcquiredSharedLite
#define ExIsResourceAcquired ExIsResourceAcquiredSharedLite


//
// UUID Generation
//

typedef GUID UUID;

#if (NTDDI_VERSION >= NTDDI_WIN2K)

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
ExUuidCreate(
    _Out_ UUID *Uuid
    );

#endif

//
// Priority increment definitions.  The comment for each definition gives
// the names of the system services that use the definition when satisfying
// a wait.
//

//
// Priority increment used when satisfying a wait on an executive event
// (NtPulseEvent and NtSetEvent)
//

#define EVENT_INCREMENT                 1

//
// Priority increment when no I/O has been done.  This is used by device
// and file system drivers when completing an IRP (IoCompleteRequest).
//

#define IO_NO_INCREMENT                 0


//
// Priority increment for completing CD-ROM I/O.  This is used by CD-ROM device
// and file system drivers when completing an IRP (IoCompleteRequest)
//

#define IO_CD_ROM_INCREMENT             1

//
// Priority increment for completing disk I/O.  This is used by disk device
// and file system drivers when completing an IRP (IoCompleteRequest)
//

#define IO_DISK_INCREMENT               1



//
// Priority increment for completing keyboard I/O.  This is used by keyboard
// device drivers when completing an IRP (IoCompleteRequest)
//

#define IO_KEYBOARD_INCREMENT           6


//
// Priority increment for completing mailslot I/O.  This is used by the mail-
// slot file system driver when completing an IRP (IoCompleteRequest).
//

#define IO_MAILSLOT_INCREMENT           2


//
// Priority increment for completing mouse I/O.  This is used by mouse device
// drivers when completing an IRP (IoCompleteRequest)
//

#define IO_MOUSE_INCREMENT              6


//
// Priority increment for completing named pipe I/O.  This is used by the
// named pipe file system driver when completing an IRP (IoCompleteRequest).
//

#define IO_NAMED_PIPE_INCREMENT         2

//
// Priority increment for completing network I/O.  This is used by network
// device and network file system drivers when completing an IRP
// (IoCompleteRequest).
//

#define IO_NETWORK_INCREMENT            2


//
// Priority increment for completing parallel I/O.  This is used by parallel
// device drivers when completing an IRP (IoCompleteRequest)
//

#define IO_PARALLEL_INCREMENT           1

//
// Priority increment for completing serial I/O.  This is used by serial device
// drivers when completing an IRP (IoCompleteRequest)
//

#define IO_SERIAL_INCREMENT             2

//
// Priority increment for completing sound I/O.  This is used by sound device
// drivers when completing an IRP (IoCompleteRequest)
//

#define IO_SOUND_INCREMENT              8

//
// Priority increment for completing video I/O.  This is used by video device
// drivers when completing an IRP (IoCompleteRequest)
//

#define IO_VIDEO_INCREMENT              1

//
// Priority increment used when satisfying a wait on an executive semaphore
// (NtReleaseSemaphore)
//

#define SEMAPHORE_INCREMENT             1


#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTKERNELAPI
BOOLEAN
MmIsThisAnNtAsSystem (
    VOID
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WIN2K)
_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
NTKERNELAPI
NTSTATUS
MmMapUserAddressesToPage (
    _In_reads_bytes_(NumberOfBytes) PVOID BaseAddress,
    _In_ SIZE_T NumberOfBytes,
    _In_ PVOID PageAddress
    );
#endif


typedef struct _PHYSICAL_MEMORY_RANGE {
    PHYSICAL_ADDRESS BaseAddress;
    LARGE_INTEGER NumberOfBytes;
} PHYSICAL_MEMORY_RANGE, *PPHYSICAL_MEMORY_RANGE;

#if (NTDDI_VERSION >= NTDDI_WIN2K)

#if (NTDDI_VERSION >= NTDDI_WIN10_RS2)
#define MM_ADD_PHYSICAL_MEMORY_ALREADY_ZEROED       0x1
#endif

_IRQL_requires_max_ (PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
MmAddPhysicalMemory (
    _In_ PPHYSICAL_ADDRESS StartAddress,
    _Inout_ PLARGE_INTEGER NumberOfBytes
    );
#endif

typedef NTSTATUS (*PMM_ROTATE_COPY_CALLBACK_FUNCTION) (
    _In_ PMDL DestinationMdl,
    _In_ PMDL SourceMdl,
    _In_ PVOID Context
    );

typedef enum _MM_ROTATE_DIRECTION {
    MmToFrameBuffer,
    MmToFrameBufferNoCopy,
    MmToRegularMemory,
    MmToRegularMemoryNoCopy,
    MmMaximumRotateDirection
} MM_ROTATE_DIRECTION, *PMM_ROTATE_DIRECTION;

#if (NTDDI_VERSION >= NTDDI_VISTA)
_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
NTSTATUS
MmRotatePhysicalView (
    _In_ PVOID VirtualAddress,
    _Inout_ PSIZE_T NumberOfBytes,
    _In_opt_ PMDL NewMdl,
    _In_ MM_ROTATE_DIRECTION Direction,
    _In_ PMM_ROTATE_COPY_CALLBACK_FUNCTION CopyFunction,
    _In_opt_ PVOID Context
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_ (PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
MmRemovePhysicalMemory (
    _In_ PPHYSICAL_ADDRESS StartAddress,
    _Inout_ PLARGE_INTEGER NumberOfBytes
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_ (PASSIVE_LEVEL)
NTKERNELAPI
PPHYSICAL_MEMORY_RANGE
MmGetPhysicalMemoryRanges (
    VOID
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN10_RS1)

#define MM_SYSTEM_PARTITION_OBJECT                  NULL
#define MM_CURRENT_PROCESS_PARTITION_OBJECT         ((PVOID) MAXULONG_PTR)

_IRQL_requires_max_ (PASSIVE_LEVEL)
NTKERNELAPI
PPHYSICAL_MEMORY_RANGE
MmGetPhysicalMemoryRangesEx (
    _In_opt_ PVOID PartitionObject
    );

#endif


#if (NTDDI_VERSION >= NTDDI_WIN2K)
_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
NTKERNELAPI
_Out_writes_bytes_opt_ (NumberOfBytes) PVOID
MmMapVideoDisplay (
    _In_ PHYSICAL_ADDRESS PhysicalAddress,
    _In_ SIZE_T NumberOfBytes,
    _In_ MEMORY_CACHING_TYPE CacheType
     );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_ (APC_LEVEL)
NTKERNELAPI
VOID
MmUnmapVideoDisplay (
     _In_reads_bytes_ (NumberOfBytes) PVOID BaseAddress,
     _In_ SIZE_T NumberOfBytes
     );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTKERNELAPI
PHYSICAL_ADDRESS
MmGetPhysicalAddress (
    _In_ PVOID BaseAddress
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WINBLUE)
_IRQL_requires_max_ (DISPATCH_LEVEL)
NTKERNELAPI
NTSTATUS
MmGetCacheAttribute (
    _In_ PHYSICAL_ADDRESS PhysicalAddress,
    _Out_ MEMORY_CACHING_TYPE *CacheType
    );

typedef struct _MM_COPY_ADDRESS {
    union {
        PVOID VirtualAddress;
        PHYSICAL_ADDRESS PhysicalAddress;
    };
} MM_COPY_ADDRESS, *PMMCOPY_ADDRESS;

#define MM_COPY_MEMORY_PHYSICAL             0x1
#define MM_COPY_MEMORY_VIRTUAL              0x2

_IRQL_requires_max_ (APC_LEVEL)
NTKERNELAPI
NTSTATUS
MmCopyMemory (
    _In_ PVOID TargetAddress,
    _In_ MM_COPY_ADDRESS SourceAddress,
    _In_ SIZE_T NumberOfBytes,
    _In_ ULONG Flags,
    _Out_ PSIZE_T NumberOfBytesTransferred
    );

#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTKERNELAPI
PVOID
MmGetVirtualForPhysical (
    _In_ PHYSICAL_ADDRESS PhysicalAddress
    );
#endif



#if (NTDDI_VERSION >= NTDDI_WIN2K)
_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
_When_ (return != NULL, _Post_writable_byte_size_ (NumberOfBytes)) PVOID
MmAllocateContiguousMemory (
    _In_ SIZE_T NumberOfBytes,
    _In_ PHYSICAL_ADDRESS HighestAcceptableAddress
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
_When_ (return != NULL, _Post_writable_byte_size_ (NumberOfBytes)) PVOID
MmAllocateContiguousMemorySpecifyCache (
    _In_ SIZE_T NumberOfBytes,
    _In_ PHYSICAL_ADDRESS LowestAcceptableAddress,
    _In_ PHYSICAL_ADDRESS HighestAcceptableAddress,
    _In_opt_ PHYSICAL_ADDRESS BoundaryAddressMultiple,
    _In_ MEMORY_CACHING_TYPE CacheType
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)



typedef ULONG NODE_REQUIREMENT;

#define MM_ANY_NODE_OK          0x80000000



#endif

#if (NTDDI_VERSION >= NTDDI_VISTA)
_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
_When_ (return != NULL, _Post_writable_byte_size_ (NumberOfBytes)) PVOID
MmAllocateContiguousMemorySpecifyCacheNode (
    _In_ SIZE_T NumberOfBytes,
    _In_ PHYSICAL_ADDRESS LowestAcceptableAddress,
    _In_ PHYSICAL_ADDRESS HighestAcceptableAddress,
    _In_opt_ PHYSICAL_ADDRESS BoundaryAddressMultiple,
    _In_ MEMORY_CACHING_TYPE CacheType,
    _In_ NODE_REQUIREMENT PreferredNode
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN8)
_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
_When_ (return != NULL, _Post_writable_byte_size_ (NumberOfBytes)) PVOID
MmAllocateContiguousNodeMemory (
    _In_ SIZE_T NumberOfBytes,
    _In_ PHYSICAL_ADDRESS LowestAcceptableAddress,
    _In_ PHYSICAL_ADDRESS HighestAcceptableAddress,
    _In_opt_ PHYSICAL_ADDRESS BoundaryAddressMultiple,
    _In_ ULONG Protect,
    _In_ NODE_REQUIREMENT PreferredNode
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
MmFreeContiguousMemory (
    _In_ PVOID BaseAddress
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_ (DISPATCH_LEVEL)
NTKERNELAPI
VOID
MmFreeContiguousMemorySpecifyCache (
    _In_reads_bytes_ (NumberOfBytes) PVOID BaseAddress,
    _In_ SIZE_T NumberOfBytes,
    _In_ MEMORY_CACHING_TYPE CacheType
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WIN2K)
_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
_Out_writes_bytes_opt_ (NumberOfBytes) PVOID
MmAllocateNonCachedMemory (
    _In_ SIZE_T NumberOfBytes
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
MmFreeNonCachedMemory (
    _In_reads_bytes_ (NumberOfBytes) PVOID BaseAddress,
    _In_ SIZE_T NumberOfBytes
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
BOOLEAN
MmIsAddressValid (
    _In_ PVOID VirtualAddress
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
__drv_preferredFunction("(see documentation)", "Obsolete")
DECLSPEC_DEPRECATED_DDK
NTKERNELAPI
BOOLEAN
MmIsNonPagedSystemAddressValid (
    _In_ PVOID VirtualAddress
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
MmLockPagableSectionByHandle (
    _In_ PVOID ImageSectionHandle
    );
#endif


//
// Note that even though this function prototype
// says "HANDLE", MmSecureVirtualMemory does NOT return
// anything resembling a Win32-style handle.  The return
// value from this function can ONLY be used with MmUnsecureVirtualMemory.
//
#if (NTDDI_VERSION >= NTDDI_WIN2K)
_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
__drv_reportError("Caution: MmSecureVirtualMemory ensures the specified VA "
	"range protections cannot be tightened - but accesses to the memory can "
	"still fail and so they must be protected by try-except.")
NTKERNELAPI
HANDLE
MmSecureVirtualMemory (
    __in_data_source(USER_MODE) _In_reads_bytes_ (Size) PVOID Address,
    _In_  __in_data_source(USER_MODE) SIZE_T Size,
    _In_ ULONG ProbeMode
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
MmUnsecureVirtualMemory (
    _In_ HANDLE SecureHandle
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN8)

#define MM_SYSTEM_VIEW_EXCEPTIONS_FOR_INPAGE_ERRORS 0x1

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
NTKERNELAPI
NTSTATUS
MmMapViewInSystemSpaceEx (
    _In_ PVOID Section,
    _Outptr_result_bytebuffer_ (*ViewSize) PVOID *MappedBase,
    _Inout_ PSIZE_T ViewSize,
    _Inout_ PLARGE_INTEGER SectionOffset,
    _In_ ULONG_PTR Flags
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
//@[comment("MVI_tracked")]
_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
NTKERNELAPI
NTSTATUS
MmMapViewInSystemSpace (
    _In_ PVOID Section,
    _Outptr_result_bytebuffer_ (*ViewSize) PVOID *MappedBase,
    _Inout_ PSIZE_T ViewSize
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
//@[comment("MVI_tracked")]
_IRQL_requires_max_ (APC_LEVEL)
NTKERNELAPI
NTSTATUS
MmUnmapViewInSystemSpace (
    _In_ PVOID MappedBase
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN8)
_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
NTKERNELAPI
NTSTATUS
MmMapViewInSessionSpaceEx (
    _In_ PVOID Section,
    _Outptr_result_bytebuffer_ (*ViewSize) PVOID *MappedBase,
    _Inout_ PSIZE_T ViewSize,
    _Inout_ PLARGE_INTEGER SectionOffset,
    _In_ ULONG_PTR Flags
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
NTKERNELAPI
NTSTATUS
MmMapViewInSessionSpace (
    _In_ PVOID Section,
    _Outptr_result_bytebuffer_ (*ViewSize) PVOID *MappedBase,
    _Inout_ PSIZE_T ViewSize
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_ (APC_LEVEL)
NTKERNELAPI
NTSTATUS
MmUnmapViewInSessionSpace (
    _In_ PVOID MappedBase
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WS03)
_Must_inspect_result_
_IRQL_requires_max_ (PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
MmCreateMirror (
    VOID
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
BOOLEAN
SeSinglePrivilegeCheck(
    _In_ LUID PrivilegeValue,
    _In_ KPROCESSOR_MODE PreviousMode
    );
#endif


extern NTKERNELAPI PEPROCESS PsInitialSystemProcess;


#if !defined(_PSGETCURRENTTHREAD_)

#define _PSGETCURRENTTHREAD_

_IRQL_requires_max_(DISPATCH_LEVEL)
CFORCEINLINE
PETHREAD
PsGetCurrentThread (
    VOID
    )

/*++

Routine Description:

    This function returns a pointer to the current executive thread object.

Arguments:

    None.

Return Value:

    A pointer to the current executive thread object.

--*/

{

    return (PETHREAD)KeGetCurrentThread();
}

#endif


typedef struct _PS_CREATE_NOTIFY_INFO {
    _In_ SIZE_T Size;
    union {
        _In_ ULONG Flags;
        struct {
            _In_ ULONG FileOpenNameAvailable : 1;
            _In_ ULONG IsSubsystemProcess : 1;
            _In_ ULONG Reserved : 30;
        };
    };
    _In_ HANDLE ParentProcessId;
    _In_ CLIENT_ID CreatingThreadId;
    _Inout_ struct _FILE_OBJECT *FileObject;
    _In_ PCUNICODE_STRING ImageFileName;
    _In_opt_ PCUNICODE_STRING CommandLine;
    _Inout_ NTSTATUS CreationStatus;
} PS_CREATE_NOTIFY_INFO, *PPS_CREATE_NOTIFY_INFO;

typedef
VOID
(*PCREATE_PROCESS_NOTIFY_ROUTINE)(
    _In_ HANDLE ParentId,
    _In_ HANDLE ProcessId,
    _In_ BOOLEAN Create
    );

#if (NTDDI_VERSION >= NTDDI_WIN2K)
//@[comment("MVI_tracked")]
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
PsSetCreateProcessNotifyRoutine(
    _In_ PCREATE_PROCESS_NOTIFY_ROUTINE NotifyRoutine,
    _In_ BOOLEAN Remove
    );
#endif

typedef
VOID
(*PCREATE_PROCESS_NOTIFY_ROUTINE_EX) (
    _Inout_ PEPROCESS Process,
    _In_ HANDLE ProcessId,
    _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
    );

#if (NTDDI_VERSION >= NTDDI_VISTASP1)
//@[comment("MVI_tracked")]
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
PsSetCreateProcessNotifyRoutineEx (
    _In_ PCREATE_PROCESS_NOTIFY_ROUTINE_EX NotifyRoutine,
    _In_ BOOLEAN Remove
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN10_RS2)
typedef enum _PSCREATEPROCESSNOTIFYTYPE {
    PsCreateProcessNotifySubsystems = 0
} PSCREATEPROCESSNOTIFYTYPE;

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
PsSetCreateProcessNotifyRoutineEx2 (
    _In_ PSCREATEPROCESSNOTIFYTYPE NotifyType,
    _In_ PVOID NotifyInformation,
    _In_ BOOLEAN Remove
    );
#endif

typedef
VOID
(*PCREATE_THREAD_NOTIFY_ROUTINE)(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_ BOOLEAN Create
    );

#if (NTDDI_VERSION >= NTDDI_WIN2K)
//@[comment("MVI_tracked")]
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
PsSetCreateThreadNotifyRoutine(
    _In_ PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
typedef enum _PSCREATETHREADNOTIFYTYPE {
    PsCreateThreadNotifyNonSystem = 0,
    PsCreateThreadNotifySubsystems = 1
} PSCREATETHREADNOTIFYTYPE;

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
PsSetCreateThreadNotifyRoutineEx(
    _In_ PSCREATETHREADNOTIFYTYPE NotifyType,
    _In_ PVOID NotifyInformation
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)
NTKERNELAPI
NTSTATUS
PsRemoveCreateThreadNotifyRoutine (
    _In_ PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine
    );
#endif

//
// Structures for Load Image Notify
//

#define IMAGE_ADDRESSING_MODE_32BIT     3

typedef struct _IMAGE_INFO {
    union {
        ULONG Properties;
        struct {
            ULONG ImageAddressingMode  : 8;  // Code addressing mode
            ULONG SystemModeImage      : 1;  // System mode image
            ULONG ImageMappedToAllPids : 1;  // Image mapped into all processes
            ULONG ExtendedInfoPresent  : 1;  // IMAGE_INFO_EX available
            ULONG MachineTypeMismatch  : 1;  // Architecture type mismatch
            ULONG ImageSignatureLevel  : 4;  // Signature level
            ULONG ImageSignatureType   : 3;  // Signature type
            ULONG ImagePartialMap      : 1;  // Nonzero if entire image is not mapped
            ULONG Reserved             : 12;
        };
    };
    PVOID       ImageBase;
    ULONG       ImageSelector;
    SIZE_T      ImageSize;
    ULONG       ImageSectionNumber;
} IMAGE_INFO, *PIMAGE_INFO;

typedef struct _IMAGE_INFO_EX {
    SIZE_T              Size;
    IMAGE_INFO          ImageInfo;
    struct _FILE_OBJECT *FileObject;
} IMAGE_INFO_EX, *PIMAGE_INFO_EX;

typedef
VOID
(*PLOAD_IMAGE_NOTIFY_ROUTINE)(
    _In_opt_ PUNICODE_STRING FullImageName,
    _In_ HANDLE ProcessId,                // pid into which image is being mapped
    _In_ PIMAGE_INFO ImageInfo
    );

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
PsSetLoadImageNotifyRoutine(
    _In_ PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN10_RS3)

#define PS_IMAGE_NOTIFY_CONFLICTING_ARCHITECTURE            0x1

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
PsSetLoadImageNotifyRoutineEx(
    _In_ PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine,
    _In_ ULONG_PTR Flags
    );

#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
PsRemoveLoadImageNotifyRoutine(
    _In_ PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTKERNELAPI
HANDLE
PsGetCurrentProcessId(
    VOID
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
HANDLE
PsGetCurrentThreadId(
    VOID
    );
#endif


//
// Prefetch information.
//

#if (NTDDI_VERSION >= NTDDI_VISTA)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
PsSetCurrentThreadPrefetching (
    _In_ BOOLEAN Prefetching
    );

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
PsIsCurrentThreadPrefetching (
    VOID
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WINXP)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
LONGLONG
PsGetProcessCreateTimeQuadPart(
    _In_ PEPROCESS Process
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN10_RS2)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
ULONGLONG
PsGetProcessStartKey(
    _In_ PEPROCESS Process
    );
#endif

//@[comment("MVI_tracked")]
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
PsGetProcessExitStatus(
    _In_ PEPROCESS Process
    );

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
PsGetThreadExitStatus(
    _In_ PETHREAD Thread
    );

#if (NTDDI_VERSION >= NTDDI_WINXP)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
HANDLE
PsGetProcessId(
    _In_ PEPROCESS Process
    );

//@[comment("MVI_tracked")]
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
HANDLE
PsGetThreadId(
    _In_ PETHREAD Thread
     );
#endif

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
PVOID
PsGetThreadProperty(
    _In_ PETHREAD Thread,
    _In_ ULONG_PTR Key,
    _In_ ULONG Flags
    );

#if (NTDDI_VERSION >= NTDDI_WS03)
//@[comment("MVI_tracked")]
NTKERNELAPI
HANDLE
PsGetThreadProcessId(
    _In_ PETHREAD Thread
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
NTKERNELAPI
LONGLONG
PsGetThreadCreateTime(
    _In_ PETHREAD Thread
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WS03SP1)

NTKERNELAPI
PVOID
PsGetCurrentThreadTeb(
    VOID
    );

#endif


//



#if (NTDDI_VERSION >= NTDDI_WIN10_RS1)

typedef
VOID
(NTAPI *SILO_CONTEXT_CLEANUP_CALLBACK)(
    _In_ PVOID SiloContext
    );

typedef
NTSTATUS
(NTAPI *SILO_MONITOR_CREATE_CALLBACK)(
    _In_ PESILO Silo
    );

typedef
VOID
(NTAPI *SILO_MONITOR_TERMINATE_CALLBACK)(
    _In_ PESILO Silo
    );

#define SILO_MONITOR_REGISTRATION_VERSION (1)

typedef struct _SILO_MONITOR_REGISTRATION {
    UCHAR Version;
    BOOLEAN MonitorHost;
    BOOLEAN MonitorExistingSilos;
    UCHAR Reserved[5];
    union {
        PUNICODE_STRING DriverObjectName;
        PUNICODE_STRING ComponentName;
    };
    SILO_MONITOR_CREATE_CALLBACK CreateCallback;
    SILO_MONITOR_TERMINATE_CALLBACK TerminateCallback;
} SILO_MONITOR_REGISTRATION, *PSILO_MONITOR_REGISTRATION;

NTKERNELAPI
NTSTATUS
PsGetJobSilo(
    _In_ PEJOB Job,
    _Outptr_ PESILO *Silo
    );

NTKERNELAPI
NTSTATUS
PsGetJobServerSilo(
    _In_opt_ PEJOB Job,
    _Outptr_ PESILO *ServerSilo
    );

NTKERNELAPI
PESILO
PsGetEffectiveServerSilo(
    _In_ PESILO Silo
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
PESILO
PsAttachSiloToCurrentThread(
    _In_ PESILO Silo
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
PsDetachSiloFromCurrentThread(
    _In_ PESILO PreviousSilo
    );

NTKERNELAPI
BOOLEAN
PsIsHostSilo(
    _In_ PESILO Silo
    );

NTKERNELAPI
PESILO
PsGetHostSilo(
    VOID
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
PESILO
PsGetCurrentSilo(
    VOID
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
PESILO
PsGetCurrentServerSilo(
    VOID
    );

DECLSPEC_DEPRECATED_DDK
NTKERNELAPI
PUNICODE_STRING
PsGetCurrentServerSiloName(
    VOID
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
BOOLEAN
PsIsCurrentThreadInServerSilo(
    VOID
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
NTSTATUS
PsAcquireSiloHardReference(
    _In_ PESILO Silo
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
PsReleaseSiloHardReference(
    _In_ PESILO Silo
    );

#define PS_INVALID_SILO_CONTEXT_SLOT (0xffffffffUL)

NTKERNELAPI
_Must_inspect_result_
NTSTATUS
PsAllocSiloContextSlot(
    _In_ ULONG_PTR Reserved,
    _Out_ ULONG *ReturnedContextSlot
   );

NTKERNELAPI
_Check_return_
NTSTATUS
PsFreeSiloContextSlot(
    _In_ ULONG ContextSlot
    );

NTKERNELAPI
_Must_inspect_result_
NTSTATUS
PsCreateSiloContext(
    _In_ PESILO Silo,
    _In_ ULONG Size,
    _In_ POOL_TYPE PoolType,
    _In_opt_ SILO_CONTEXT_CLEANUP_CALLBACK ContextCleanupCallback,
    _Outptr_result_bytebuffer_(Size) PVOID *ReturnedSiloContext
    );

NTKERNELAPI
_Check_return_
NTSTATUS
PsInsertSiloContext(
    _In_ PESILO Silo,
    _In_ ULONG ContextSlot,
    _In_ PVOID SiloContext
   );

NTKERNELAPI
_Check_return_
NTSTATUS
PsReplaceSiloContext(
    _In_ PESILO Silo,
    _In_ ULONG ContextSlot,
    _In_ PVOID NewSiloContext,
    _Outptr_opt_result_maybenull_ PVOID *OldSiloContext
   );

_IRQL_requires_max_(HIGH_LEVEL)
NTKERNELAPI
/* _Check_return_ */
NTSTATUS
PsGetSiloContext(
    _In_ PESILO Silo,
    _In_ ULONG ContextSlot,
    _Outptr_result_nullonfailure_ PVOID *ReturnedSiloContext
   );

NTKERNELAPI
_Check_return_
NTSTATUS
PsRemoveSiloContext(
    _In_ PESILO Silo,
    _In_ ULONG ContextSlot,
    _Outptr_opt_result_maybenull_ PVOID *RemovedSiloContext
   );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
PsReferenceSiloContext(
    _In_ PVOID SiloContext
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
PsDereferenceSiloContext(
    _In_ PVOID SiloContext
    );

NTKERNELAPI
_Check_return_
NTSTATUS
PsInsertPermanentSiloContext(
    _In_ PESILO Silo,
    _In_ ULONG ContextSlot,
    _In_ PVOID SiloContext
   );

NTKERNELAPI
_Check_return_
NTSTATUS
PsMakeSiloContextPermanent(
    _In_ PESILO Silo,
    _In_ ULONG ContextSlot
    );

_IRQL_requires_max_(HIGH_LEVEL)
NTKERNELAPI
/* _Check_return_ */
NTSTATUS
PsGetPermanentSiloContext(
    _In_ PESILO Silo,
    _In_ ULONG ContextSlot,
    _Outptr_result_nullonfailure_ PVOID *ReturnedSiloContext
    );

NTKERNELAPI
_Must_inspect_result_
NTSTATUS
PsRegisterSiloMonitor(
    _In_ PSILO_MONITOR_REGISTRATION Registration,
    _Outptr_ PSILO_MONITOR *ReturnedMonitor
    );

NTKERNELAPI
_Must_inspect_result_
NTSTATUS
PsStartSiloMonitor(
    _In_ PSILO_MONITOR Monitor
    );

NTKERNELAPI
ULONG
PsGetSiloMonitorContextSlot(
    _In_ PSILO_MONITOR Monitor
    );

NTKERNELAPI
VOID
PsUnregisterSiloMonitor(
    _In_ _Post_invalid_ PSILO_MONITOR Monitor
    );

NTKERNELAPI
ULONG
PsGetServerSiloServiceSessionId(
    _In_ PESILO Silo
    );

NTKERNELAPI
ULONG
PsGetServerSiloActiveConsoleId(
    _In_ PESILO Silo
    );

NTKERNELAPI
VOID
PsTerminateServerSilo(
    _In_ PESILO ServerSilo,
    _In_ NTSTATUS ExitStatus
    );

#endif

#if (NTDDI_VERSION >= NTDDI_WIN10_RS3)

PESILO
PsGetParentSilo(
    _In_opt_ PEJOB Job
    );

#endif
//
// Directory control minor function codes
//

#define IRP_MN_QUERY_DIRECTORY              0x01
#define IRP_MN_NOTIFY_CHANGE_DIRECTORY      0x02
#define IRP_MN_NOTIFY_CHANGE_DIRECTORY_EX   0x03

//
// File system control minor function codes.  Note that "user request" is
// assumed to be zero by both the I/O system and file systems.  Do not change
// this value.
//

#define IRP_MN_USER_FS_REQUEST          0x00
#define IRP_MN_MOUNT_VOLUME             0x01
#define IRP_MN_VERIFY_VOLUME            0x02
#define IRP_MN_LOAD_FILE_SYSTEM         0x03
#define IRP_MN_TRACK_LINK               0x04    // To be obsoleted soon
#define IRP_MN_KERNEL_CALL              0x04

//
// Lock control minor function codes
//

#define IRP_MN_LOCK                     0x01
#define IRP_MN_UNLOCK_SINGLE            0x02
#define IRP_MN_UNLOCK_ALL               0x03
#define IRP_MN_UNLOCK_ALL_BY_KEY        0x04

//
// Flush minor function codes
//

#define IRP_MN_FLUSH_AND_PURGE           0x01
#if (NTDDI_VERSION >= NTDDI_WIN8)
#define IRP_MN_FLUSH_DATA_ONLY           0x02    //see FLUSH_FLAGS_FILE_DATA_ONLY for definition of how this works
#define IRP_MN_FLUSH_NO_SYNC             0x03    //see FLUSH_FLAGS_NO_SYNC for definition of how this works
#endif
#if (NTDDI_VERSION >= NTDDI_WIN10_RS1)
#define IRP_MN_FLUSH_DATA_SYNC_ONLY      0x04    //see FLUSH_FLAGS_FILE_DATA_SYNC_ONLY for definition of how this works
#endif


//
// Read and Write minor function codes for file systems supporting Lan Manager
// software.  All of these subfunction codes are invalid if the file has been
// opened with FO_NO_INTERMEDIATE_BUFFERING.  They are also invalid in combi-
// nation with synchronous calls (Irp Flag or file open option).
//
// Note that "normal" is assumed to be zero by both the I/O system and file
// systems.  Do not change this value.
//

#define IRP_MN_NORMAL                   0x00
#define IRP_MN_DPC                      0x01
#define IRP_MN_MDL                      0x02
#define IRP_MN_COMPLETE                 0x04
#define IRP_MN_COMPRESSED               0x08

#define IRP_MN_MDL_DPC                  (IRP_MN_MDL | IRP_MN_DPC)
#define IRP_MN_COMPLETE_MDL             (IRP_MN_COMPLETE | IRP_MN_MDL)
#define IRP_MN_COMPLETE_MDL_DPC         (IRP_MN_COMPLETE_MDL | IRP_MN_DPC)

#define IRP_MN_QUERY_LEGACY_BUS_INFORMATION 0x18

#define IO_CHECK_CREATE_PARAMETERS      0x0200
#define IO_ATTACH_DEVICE                0x0400

//
//  This flag is only meaningful to IoCreateFileSpecifyDeviceObjectHint.
//  FileHandles created using IoCreateFileSpecifyDeviceObjectHint with this
//  flag set will bypass ShareAccess checks on this file.
//

#define IO_IGNORE_SHARE_ACCESS_CHECK    0x0800  // Ignores share access checks on opens.

//
// Define callout routine type for use in IoQueryDeviceDescription().
//

typedef NTSTATUS (*PIO_QUERY_DEVICE_ROUTINE)(
    _In_ PVOID Context,
    _In_ PUNICODE_STRING PathName,
    _In_ INTERFACE_TYPE BusType,
    _In_ ULONG BusNumber,
    _In_ PKEY_VALUE_FULL_INFORMATION *BusInformation,
    _In_ CONFIGURATION_TYPE ControllerType,
    _In_ ULONG ControllerNumber,
    _In_ PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    _In_ CONFIGURATION_TYPE PeripheralType,
    _In_ ULONG PeripheralNumber,
    _In_ PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    );


// Defines the order of the information in the array of
// PKEY_VALUE_FULL_INFORMATION.
//

typedef enum _IO_QUERY_DEVICE_DATA_FORMAT {
    IoQueryDeviceIdentifier = 0,
    IoQueryDeviceConfigurationData,
    IoQueryDeviceComponentInformation,
    IoQueryDeviceMaxData
} IO_QUERY_DEVICE_DATA_FORMAT, *PIO_QUERY_DEVICE_DATA_FORMAT;

//
// Define driver reinitialization routine type.
//

typedef
VOID
DRIVER_REINITIALIZE (
    _In_ struct _DRIVER_OBJECT *DriverObject,
    _In_opt_ PVOID Context,
    _In_ ULONG Count
    );

typedef DRIVER_REINITIALIZE *PDRIVER_REINITIALIZE;


typedef struct _CONTROLLER_OBJECT {
    CSHORT Type;
    CSHORT Size;
    PVOID ControllerExtension;
    KDEVICE_QUEUE DeviceWaitQueue;

    ULONG Spare1;
    LARGE_INTEGER Spare2;

} CONTROLLER_OBJECT, *PCONTROLLER_OBJECT;

#define DO_VERIFY_VOLUME                    0x00000002      
#define DO_BUFFERED_IO                      0x00000004      
#define DO_EXCLUSIVE                        0x00000008      
#define DO_DIRECT_IO                        0x00000010      
#define DO_MAP_IO_BUFFER                    0x00000020      
#define DO_DEVICE_HAS_NAME                  0x00000040      
#define DO_DEVICE_INITIALIZING              0x00000080      
#define DO_SYSTEM_BOOT_PARTITION            0x00000100      
#define DO_LONG_TERM_REQUESTS               0x00000200      
#define DO_NEVER_LAST_DEVICE                0x00000400      
#define DO_SHUTDOWN_REGISTERED              0x00000800      
#define DO_BUS_ENUMERATED_DEVICE            0x00001000      
#define DO_POWER_PAGABLE                    0x00002000      
#define DO_POWER_INRUSH                     0x00004000      
#define DO_LOW_PRIORITY_FILESYSTEM          0x00010000      
#define DO_SUPPORTS_TRANSACTIONS            0x00040000      
#define DO_FORCE_NEITHER_IO                 0x00080000      
#define DO_VOLUME_DEVICE_OBJECT             0x00100000      
#define DO_SYSTEM_SYSTEM_PARTITION          0x00200000      
#define DO_SYSTEM_CRITICAL_PARTITION        0x00400000      
#define DO_DISALLOW_EXECUTE                 0x00800000
#define DO_DEVICE_TO_BE_RESET               0x04000000      
#define DO_DEVICE_IRP_REQUIRES_EXTENSION    0x08000000      
#define DO_DAX_VOLUME                       0x10000000      
#define DRVO_REINIT_REGISTERED          0x00000008
#define DRVO_INITIALIZED                0x00000010
#define DRVO_BOOTREINIT_REGISTERED      0x00000020
#define DRVO_LEGACY_RESOURCES           0x00000040

//
// The following structure is used by drivers that are initializing to
// determine the number of devices of a particular type that have already
// been initialized.  It is also used to track whether or not the AtDisk
// address range has already been claimed.  Finally, it is used by the
// NtQuerySystemInformation system service to return device type counts.
//

typedef struct _CONFIGURATION_INFORMATION {

    //
    // This field indicates the total number of disks in the system.  This
    // number should be used by the driver to determine the name of new
    // disks.  This field should be updated by the driver as it finds new
    // disks.
    //

    ULONG DiskCount;                // Count of hard disks thus far
    ULONG FloppyCount;              // Count of floppy disks thus far
    ULONG CdRomCount;               // Count of CD-ROM drives thus far
    ULONG TapeCount;                // Count of tape drives thus far
    ULONG ScsiPortCount;            // Count of SCSI port adapters thus far
    ULONG SerialCount;              // Count of serial devices thus far
    ULONG ParallelCount;            // Count of parallel devices thus far

    //
    // These next two fields indicate ownership of one of the two IO address
    // spaces that are used by WD1003-compatible disk controllers.
    //

    BOOLEAN AtDiskPrimaryAddressClaimed;    // 0x1F0 - 0x1FF
    BOOLEAN AtDiskSecondaryAddressClaimed;  // 0x170 - 0x17F

    //
    // Indicates the structure version, as anything value belong this will have been added.
    // Use the structure size as the version.
    //

    ULONG Version;

    //
    // Indicates the total number of medium changer devices in the system.
    // This field will be updated by the drivers as it determines that
    // new devices have been found and will be supported.
    //

    ULONG MediumChangerCount;

} CONFIGURATION_INFORMATION, *PCONFIGURATION_INFORMATION;

#if !(defined(USE_DMA_MACROS) && (defined(_NTDDK_) || defined(_NTDRIVER_)) || defined(_WDM_INCLUDED_))

#if (NTDDI_VERSION >= NTDDI_WIN2K)
DECLSPEC_DEPRECATED_DDK                 // Use AllocateAdapterChannel
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_min_(DISPATCH_LEVEL)
__drv_preferredFunction("AllocateAdapterChannel","obsolete")
NTKERNELAPI
NTSTATUS
IoAllocateAdapterChannel(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG NumberOfMapRegisters,
    _In_ PDRIVER_CONTROL ExecutionRoutine,
    _In_ PVOID Context
    );
#endif

#endif // !(defined(USE_DMA_MACROS) && (defined(_NTDDK_) || defined(_NTDRIVER_)) || defined(_WDM_INCLUDED_))

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_min_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
IoAllocateController(
    _In_ PCONTROLLER_OBJECT ControllerObject,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PDRIVER_CONTROL ExecutionRoutine,
    _In_opt_ PVOID Context
    );
#endif

//++
//
// VOID
// IoAssignArcName(
//     _In_ PUNICODE_STRING ArcName,
//     _In_ PUNICODE_STRING DeviceName
//     )
//
// Routine Description:
//
//     This routine is invoked by drivers of bootable media to create a symbolic
//     link between the ARC name of their device and its NT name.  This allows
//     the system to determine which device in the system was actually booted
//     from since the ARC firmware only deals in ARC names, and NT only deals
//     in NT names.
//
// Arguments:
//
//     ArcName - Supplies the Unicode string representing the ARC name.
//
//     DeviceName - Supplies the name to which the ARCname refers.
//
// Return Value:
//
//     None.
//
//--

#define IoAssignArcName( ArcName, DeviceName ) (  \
    IoCreateSymbolicLink( (ArcName), (DeviceName) ) )

#if (NTDDI_VERSION >= NTDDI_WIN2K)
DECLSPEC_DEPRECATED_DDK                 // Use Pnp or IoReprtDetectedDevice
__drv_preferredFunction("(see documentation)", "Obsolete")
NTKERNELAPI
NTSTATUS
IoAssignResources (
    _In_ PUNICODE_STRING RegistryPath,
    _In_opt_ PUNICODE_STRING DriverClassName,
    _In_ PDRIVER_OBJECT DriverObject,
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PIO_RESOURCE_REQUIREMENTS_LIST RequestedResources,
    _Inout_ PCM_RESOURCE_LIST *AllocatedResources
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WIN2K)
DECLSPEC_DEPRECATED_DDK                 // Use IoAttachDeviceToDeviceStack
__drv_preferredFunction("IoAttachDeviceToDeviceStack", "Obsolete")
NTKERNELAPI
NTSTATUS
IoAttachDeviceByPointer(
    _In_ PDEVICE_OBJECT SourceDevice,
    _In_ PDEVICE_OBJECT TargetDevice
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
PCONTROLLER_OBJECT
IoCreateController(
    _In_ ULONG Size
    );
#endif


//++
//
// VOID
// IoDeassignArcName(
//     _In_ PUNICODE_STRING ArcName
//     )
//
// Routine Description:
//
//     This routine is invoked by drivers to deassign an ARC name that they
//     created to a device.  This is generally only called if the driver is
//     deleting the device object, which means that the driver is probably
//     unloading.
//
// Arguments:
//
//     ArcName - Supplies the ARC name to be removed.
//
// Return Value:
//
//     None.
//
//--

#define IoDeassignArcName( ArcName ) (  \
    IoDeleteSymbolicLink( (ArcName) ) )

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
IoDeleteController(
    _In_ PCONTROLLER_OBJECT ControllerObject
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_min_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
IoFreeController(
    _In_ PCONTROLLER_OBJECT ControllerObject
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)          
_IRQL_requires_max_(PASSIVE_LEVEL)                
NTKERNELAPI                                 
PCONFIGURATION_INFORMATION                  
IoGetConfigurationInformation( VOID );      
#endif                                      

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
PGENERIC_MAPPING
IoGetFileObjectGenericMapping(
    VOID
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTKERNELAPI
VOID
IoCancelFileOpen(
    _In_ PDEVICE_OBJECT  DeviceObject,
    _In_ PFILE_OBJECT    FileObject
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
PIRP
IoMakeAssociatedIrp(
    _In_ PIRP Irp,
    _In_ CCHAR StackSize
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
PIRP
IoMakeAssociatedIrpEx(
    _In_ PIRP Irp,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ CCHAR StackSize
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
DECLSPEC_DEPRECATED_DDK                 // Use IoGetDeviceProperty
__drv_preferredFunction("IoGetDeviceProperty", "Obsolete")
NTKERNELAPI
NTSTATUS
IoQueryDeviceDescription(
    _In_opt_ PINTERFACE_TYPE BusType,
    _In_opt_ PULONG BusNumber,
    _In_opt_ PCONFIGURATION_TYPE ControllerType,
    _In_opt_ PULONG ControllerNumber,
    _In_opt_ PCONFIGURATION_TYPE PeripheralType,
    _In_opt_ PULONG PeripheralNumber,
    _In_ PIO_QUERY_DEVICE_ROUTINE CalloutRoutine,
    _Inout_opt_ PVOID Context
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
IoRaiseHardError(
    _In_ PIRP Irp,
    _In_opt_ PVPB Vpb,
    _In_ PDEVICE_OBJECT RealDeviceObject
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
IoRaiseInformationalHardError(
    _In_ NTSTATUS ErrorStatus,
    _In_opt_ PUNICODE_STRING String,
    _In_opt_ PKTHREAD Thread
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
BOOLEAN
IoSetThreadHardErrorMode(
    _In_ BOOLEAN EnableHardErrors
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
IoRegisterBootDriverReinitialization(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDRIVER_REINITIALIZE DriverReinitializationRoutine,
    _In_opt_ PVOID Context
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
IoRegisterDriverReinitialization(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDRIVER_REINITIALIZE DriverReinitializationRoutine,
    _In_opt_ PVOID Context
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WIN2K)
DECLSPEC_DEPRECATED_DDK                 // Use IoReportResourceForDetection
__drv_preferredFunction("IoReportResourceForDetection if needed", "Obsolete")
NTKERNELAPI
NTSTATUS
IoReportResourceUsage(
    _In_opt_ PUNICODE_STRING DriverClassName,
    _In_  PDRIVER_OBJECT DriverObject,
    _In_reads_bytes_opt_(DriverListSize) PCM_RESOURCE_LIST DriverList,
    _In_opt_ ULONG DriverListSize,
    _In_opt_  PDEVICE_OBJECT DeviceObject,
    _In_reads_bytes_opt_(DeviceListSize) PCM_RESOURCE_LIST DeviceList,
    _In_opt_ ULONG DeviceListSize,
    _In_  BOOLEAN OverrideConflict,
    _Out_ PBOOLEAN ConflictDetected
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WS03SP1)
//@[comment("MVI_tracked")]
BOOLEAN
IoTranslateBusAddress(
    _In_  INTERFACE_TYPE InterfaceType,
    _In_  ULONG BusNumber,
    _In_  PHYSICAL_ADDRESS BusAddress,
    _Inout_ PULONG AddressSpace,
    _Out_ PPHYSICAL_ADDRESS TranslatedAddress
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
IoSetHardErrorOrVerifyDevice(
    _In_ PIRP Irp,
    _In_ PDEVICE_OBJECT DeviceObject
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
FASTCALL
HalExamineMBR(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG SectorSize,
    _In_ ULONG MBRTypeIdentifier,
    _Outptr_result_bytebuffer_maybenull_(_Inexpressible_("Depends on MBRTypeIdentifier")) PVOID *Buffer
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)
DECLSPEC_DEPRECATED_DDK                 // Use IoReadPartitionTableEx
_IRQL_requires_max_(PASSIVE_LEVEL)
__drv_preferredFunction("IoReadPartitionTableEx", "Obsolete")
NTKERNELAPI
NTSTATUS
FASTCALL
IoReadPartitionTable(
    _In_  PDEVICE_OBJECT DeviceObject,
    _In_  ULONG SectorSize,
    _In_  BOOLEAN ReturnRecognizedPartitions,
    _Out_ struct _DRIVE_LAYOUT_INFORMATION **PartitionBuffer
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)
DECLSPEC_DEPRECATED_DDK                 // Use IoSetPartitionInformationEx
_IRQL_requires_max_(PASSIVE_LEVEL)
__drv_preferredFunction("IoSetPartitionInformationEx", "Obsolete")
NTKERNELAPI
NTSTATUS
FASTCALL
IoSetPartitionInformation(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG SectorSize,
    _In_ ULONG PartitionNumber,
    _In_ ULONG PartitionType
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WINXP)
DECLSPEC_DEPRECATED_DDK                 // Use IoWritePartitionTableEx
_IRQL_requires_max_(PASSIVE_LEVEL)
__drv_preferredFunction("IoWritePartitionTableEx", "Obsolete")
NTKERNELAPI
NTSTATUS
FASTCALL
IoWritePartitionTable(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG SectorSize,
    _In_ ULONG SectorsPerTrack,
    _In_ ULONG NumberOfHeads,
    _In_ struct _DRIVE_LAYOUT_INFORMATION *PartitionBuffer
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)
NTKERNELAPI
NTSTATUS
IoCreateDisk(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ struct _CREATE_DISK* Disk
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
IoReadPartitionTableEx(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Outptr_ struct _DRIVE_LAYOUT_INFORMATION_EX** DriveLayout
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
IoWritePartitionTableEx(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_reads_(_Inexpressible_(FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EX, PartitionEntry[0]))) struct _DRIVE_LAYOUT_INFORMATION_EX* DriveLayout
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
IoSetPartitionInformationEx(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG PartitionNumber,
    _In_ struct _SET_PARTITION_INFORMATION_EX* PartitionInfo
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)
NTKERNELAPI
NTSTATUS
IoVerifyPartitionTable(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ BOOLEAN FixErrors
    );
#endif

typedef struct _DISK_SIGNATURE {
    ULONG PartitionStyle;
    union {
        struct {
            ULONG Signature;
            ULONG CheckSum;
        } Mbr;

        struct {
            GUID DiskId;
        } Gpt;
    };
} DISK_SIGNATURE, *PDISK_SIGNATURE;

#if (NTDDI_VERSION >= NTDDI_WINXP)
NTKERNELAPI
NTSTATUS
IoReadDiskSignature(
    _In_  PDEVICE_OBJECT DeviceObject,
    _In_  ULONG BytesPerSector,
    _Out_ PDISK_SIGNATURE Signature
    );
#endif



#if (NTDDI_VERSION >= NTDDI_WINXP)
NTKERNELAPI
NTSTATUS
IoVolumeDeviceToDosName(
    _In_  PVOID           VolumeDeviceObject,
    _Out_ _When_(return==0,
          _At_(DosName->Buffer, __drv_allocatesMem(Mem)))
          PUNICODE_STRING DosName
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN8)
_Must_inspect_result_
NTKERNELAPI
NTSTATUS
IoVolumeDeviceToGuidPath(
    _In_  PVOID           VolumeDeviceObject,
    _Out_ _At_(GuidPath->Buffer,
            __drv_allocatesMem(Mem)
            _Post_notnull_)
          PUNICODE_STRING GuidPath
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN8)
NTKERNELAPI
NTSTATUS
IoVolumeDeviceToGuid(
    _In_  PVOID           VolumeDeviceObject,
    _Out_ GUID            *Guid
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN10)
NTKERNELAPI
NTSTATUS
IoVolumeDeviceNameToGuid(
    _In_  PUNICODE_STRING VolumeDeviceName,
    _Out_ GUID            *Guid
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN10)
_Must_inspect_result_
NTKERNELAPI
NTSTATUS
IoVolumeDeviceNameToGuidPath(
    _In_  PUNICODE_STRING VolumeDeviceName,
    _Out_ _At_(GuidPath->Buffer,
            __drv_allocatesMem(Mem)
            _Post_notnull_)
          PUNICODE_STRING GuidPath
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WINXP)
NTKERNELAPI
NTSTATUS
IoSetSystemPartition(
    _In_ PUNICODE_STRING VolumeNameString
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WINXP)
NTKERNELAPI
NTSTATUS
IoCreateFileSpecifyDeviceObjectHint(
    _Out_ PHANDLE FileHandle,
    _In_  ACCESS_MASK DesiredAccess,
    _In_  POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _In_  ULONG FileAttributes,
    _In_  ULONG ShareAccess,
    _In_  ULONG Disposition,
    _In_  ULONG CreateOptions,
    _In_opt_ PVOID EaBuffer,
    _In_  ULONG EaLength,
    _In_  CREATE_FILE_TYPE CreateFileType,
    _In_opt_ PVOID InternalParameters,
    _In_  ULONG Options,
    _In_opt_ PVOID DeviceObject
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN10_RS1)
typedef struct _IO_FOEXT_SILO_PARAMETERS {

    ULONG Length;
    union {
        struct {
            ULONG HasHardReference : 1;
            ULONG SpareFlags : 31;
        };
        ULONG Flags;
    };
    PESILO SiloContext;

} IO_FOEXT_SILO_PARAMETERS, *PIO_FOEXT_SILO_PARAMETERS;

PIO_FOEXT_SILO_PARAMETERS
IoGetSiloParameters (
    _In_ PFILE_OBJECT FileObject
    );

_Ret_maybenull_
PESILO
IoGetSilo (
    _In_ PFILE_OBJECT FileObject
    );
#endif

typedef struct _TXN_PARAMETER_BLOCK {

    USHORT Length;              // sizeof( TXN_PARAMETER_BLOCK )
    USHORT TxFsContext;         // this is mini version of the requested file
    PVOID  TransactionObject;   // referenced pointer to KTRANSACTION

} TXN_PARAMETER_BLOCK, *PTXN_PARAMETER_BLOCK;

//
//  This value should be used in the TxFsContext member of the
//  TXN_PARAMETER_BLOCK in the absence of a specific miniversion.
//

#define TXF_MINIVERSION_DEFAULT_VIEW        (0xFFFE)

#if (NTDDI_VERSION >= NTDDI_VISTA)
PTXN_PARAMETER_BLOCK
IoGetTransactionParameterBlock (
    _In_ PFILE_OBJECT FileObject
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

//
// ECP context for creating a user mode process. Licensing and antimalware use
// the ECP to determine if the target process can be launched.
//

typedef struct _CREATE_USER_PROCESS_ECP_CONTEXT {
    //
    //  This must be set to the size of this structure.
    //

    USHORT Size;

    //
    //  This must be set to zero.
    //

    USHORT Reserved;

    //
    // The access token of the process that is getting created.
    //

    PACCESS_TOKEN AccessToken;

} CREATE_USER_PROCESS_ECP_CONTEXT, *PCREATE_USER_PROCESS_ECP_CONTEXT;

// {E0E429FF-6DDC-4E65-AAB6-45D05A038A08}
DEFINE_GUID(GUID_ECP_CREATE_USER_PROCESS, 0xe0e429ff, 0x6ddc, 0x4e65, 0xaa, 0xb6, 0x45, 0xd0, 0x5a, 0x3, 0x8a, 0x8);

#endif // NTDDI_VERSION >= NTDDI_WINTHRESHOLD

#if (NTDDI_VERSION >= NTDDI_WIN7)
typedef struct _OPLOCK_KEY_ECP_CONTEXT *POPLOCK_KEY_ECP_CONTEXT;

POPLOCK_KEY_ECP_CONTEXT
IoGetOplockKeyContext (
    _In_ PFILE_OBJECT FileObject
    );
#endif // NTDDI_VERSION >= NTDDI_WIN7

#if (NTDDI_VERSION >= NTDDI_WIN8)
//
//  This structure holds parent and target oplock keys.  This is what gets returned
//  to drivers who call  IoGetOplockKeyContextEx.
//

typedef struct _OPLOCK_KEY_CONTEXT {

    USHORT Version;        //  OPLOCK_KEY_VERSION_*
    USHORT Flags;          //  OPLOCK_KEY_FLAG_*

    GUID ParentOplockKey;

    GUID TargetOplockKey;

    ULONG Reserved;

} OPLOCK_KEY_CONTEXT, *POPLOCK_KEY_CONTEXT;

#define OPLOCK_KEY_VERSION_WIN7     0x0001
#define OPLOCK_KEY_VERSION_WIN8     0x0002

#define OPLOCK_KEY_FLAG_PARENT_KEY  0x0001
#define OPLOCK_KEY_FLAG_TARGET_KEY  0x0002

POPLOCK_KEY_CONTEXT
IoGetOplockKeyContextEx (
    _In_ PFILE_OBJECT FileObject
    );
#endif // NTDDI_VERSION >= NTDDI_WIN8

typedef struct _IO_DRIVER_CREATE_CONTEXT {
    CSHORT Size;
    struct _ECP_LIST *ExtraCreateParameter;
    PVOID DeviceObjectHint;
    PTXN_PARAMETER_BLOCK TxnParameters;
#if (NTDDI_VERSION >= NTDDI_WIN10_RS1)
    PESILO SiloContext;
#endif
} IO_DRIVER_CREATE_CONTEXT, *PIO_DRIVER_CREATE_CONTEXT;

#define IO_DRIVER_CREATE_CONTEXT_IS_MIN_SIZE(DriverContext) \
    (DriverContext->Size >= (FIELD_OFFSET(IO_DRIVER_CREATE_CONTEXT, TxnParameters) + sizeof(DriverContext->TxnParameters)))

#define IO_DRIVER_CREATE_CONTEXT_CONTAINS_SILO_CONTEXT(DriverContext) \
    (DriverContext->Size >= (FIELD_OFFSET(IO_DRIVER_CREATE_CONTEXT, SiloContext) + sizeof(DriverContext->SiloContext)))

#if (NTDDI_VERSION >= NTDDI_WIN10_RS1)
#define IO_USE_AMBIENT_SILO    ((PESILO)1)
#endif

VOID
FORCEINLINE
IoInitializeDriverCreateContext(
    PIO_DRIVER_CREATE_CONTEXT DriverContext
    )
{
    // Initialize the context
    RtlZeroMemory(DriverContext, sizeof(IO_DRIVER_CREATE_CONTEXT));
    DriverContext->Size = sizeof(IO_DRIVER_CREATE_CONTEXT);
#if (NTDDI_VERSION >= NTDDI_WIN10_RS1)
    DriverContext->SiloContext = IO_USE_AMBIENT_SILO;
#endif
}

#if (NTDDI_VERSION >= NTDDI_VISTA)
NTKERNELAPI
NTSTATUS
IoCreateFileEx(
    _Out_ PHANDLE FileHandle,
    _In_  ACCESS_MASK DesiredAccess,
    _In_  POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _In_  ULONG FileAttributes,
    _In_  ULONG ShareAccess,
    _In_  ULONG Disposition,
    _In_  ULONG CreateOptions,
    _In_opt_ PVOID EaBuffer,
    _In_  ULONG EaLength,
    _In_  CREATE_FILE_TYPE CreateFileType,
    _In_opt_ PVOID InternalParameters,
    _In_  ULONG Options,
    _In_opt_ PIO_DRIVER_CREATE_CONTEXT DriverContext
    );

NTSTATUS
IoSetIrpExtraCreateParameter(
    _Inout_ PIRP Irp,
    _In_ struct _ECP_LIST *ExtraCreateParameter
    );

VOID
IoClearIrpExtraCreateParameter(
    _Inout_ PIRP Irp
    );

NTSTATUS
IoGetIrpExtraCreateParameter(
    _In_ PIRP Irp,
    _Outptr_result_maybenull_ struct _ECP_LIST **ExtraCreateParameter
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN10_RS2)
NTKERNELAPI
NTSTATUS
IoQueryInformationByName (
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _Out_writes_bytes_(Length) PVOID FileInformation,
    _In_ ULONG Length,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _In_ ULONG Options,
    _In_opt_ PIO_DRIVER_CREATE_CONTEXT DriverContext
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)
NTKERNELAPI
NTSTATUS
IoAttachDeviceToDeviceStackSafe(
    _In_  PDEVICE_OBJECT SourceDevice,
    _In_  PDEVICE_OBJECT TargetDevice,
    _Outptr_ PDEVICE_OBJECT *AttachedToDeviceObject
    );
#endif



#if (NTDDI_VERSION >= NTDDI_WIN2KSP3)
NTKERNELAPI
BOOLEAN
IoIsFileOriginRemote(
    _In_ PFILE_OBJECT FileObject
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2KSP3)
NTKERNELAPI
NTSTATUS
IoSetFileOrigin(
    _In_ PFILE_OBJECT FileObject,
    _In_ BOOLEAN Remote
    );
#endif

#if (NTDDI_VERSION >= NTDDI_VISTA)
BOOLEAN
IoIsFileObjectIgnoringSharing (
  _In_ PFILE_OBJECT FileObject
);
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTSTATUS
IoSetFileObjectIgnoreSharing (
  _In_ PFILE_OBJECT FileObject
);
#endif


#if (NTDDI_VERSION >= NTDDI_WS03)
NTKERNELAPI
IO_PAGING_PRIORITY
FASTCALL
IoGetPagingIoPriority(
    _In_ PIRP Irp
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WIN8)

//
// BDCB_IMAGE_INFORMATION ImageFlags.
//

#define BDCB_IMAGEFLAGS_FAILED_CODE_INTEGRITY   (1UL << 0)

typedef enum _BDCB_CALLBACK_TYPE {
    BdCbStatusUpdate,
    BdCbInitializeImage,
} BDCB_CALLBACK_TYPE, *PBDCB_CALLBACK_TYPE;

typedef enum _BDCB_CLASSIFICATION {
    BdCbClassificationUnknownImage,
    BdCbClassificationKnownGoodImage,
    BdCbClassificationKnownBadImage,
    BdCbClassificationKnownBadImageBootCritical,
    BdCbClassificationEnd,
} BDCB_CLASSIFICATION, *PBDCB_CLASSIFICATION;

typedef enum _BDCB_STATUS_UPDATE_TYPE {
    BdCbStatusPrepareForDependencyLoad,
    BdCbStatusPrepareForDriverLoad,
    BdCbStatusPrepareForUnload,
} BDCB_STATUS_UPDATE_TYPE, *PBDCB_STATUS_UPDATE_TYPE;

typedef struct _BDCB_STATUS_UPDATE_CONTEXT {
    BDCB_STATUS_UPDATE_TYPE StatusType;
} BDCB_STATUS_UPDATE_CONTEXT, *PBDCB_STATUS_UPDATE_CONTEXT;

typedef struct _BDCB_IMAGE_INFORMATION {
    BDCB_CLASSIFICATION Classification;
    ULONG ImageFlags;
    UNICODE_STRING ImageName;
    UNICODE_STRING RegistryPath;
    UNICODE_STRING CertificatePublisher;
    UNICODE_STRING CertificateIssuer;
    PVOID ImageHash;
    PVOID CertificateThumbprint;
    ULONG ImageHashAlgorithm;
    ULONG ThumbprintHashAlgorithm;
    ULONG ImageHashLength;
    ULONG CertificateThumbprintLength;
} BDCB_IMAGE_INFORMATION, *PBDCB_IMAGE_INFORMATION;

typedef
VOID
BOOT_DRIVER_CALLBACK_FUNCTION (
    _In_opt_ PVOID CallbackContext,
    _In_ BDCB_CALLBACK_TYPE Classification,
    _Inout_ PBDCB_IMAGE_INFORMATION ImageInformation
    );

typedef BOOT_DRIVER_CALLBACK_FUNCTION *PBOOT_DRIVER_CALLBACK_FUNCTION;
#endif

#if (NTDDI_VERSION >= NTDDI_WIN8)
PVOID
IoRegisterBootDriverCallback (
    _In_ PBOOT_DRIVER_CALLBACK_FUNCTION CallbackFunction,
    _In_opt_ PVOID CallbackContext
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN8)
VOID
IoUnregisterBootDriverCallback (
    _In_ PVOID CallbackHandle
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN8)
NTSTATUS
IoGetActivityIdIrp (
    _In_ PIRP Irp,
    _Out_ LPGUID Guid
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN8)
NTSTATUS
IoSetActivityIdIrp (
    _Inout_ PIRP Irp,
    _In_opt_ LPCGUID Guid
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN8)
NTSTATUS
IoPropagateActivityIdToThread(
    _In_ PIRP Irp,
    _Out_ LPGUID PropagatedId,
    _Outptr_ LPCGUID *OriginalId
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN8)
LPCGUID
IoSetActivityIdThread (
    _In_ LPCGUID ActivityId
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN8)
VOID
IoClearActivityIdThread (
    _In_ LPCGUID OriginalId
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN8)
LPCGUID
IoGetActivityIdThread (
    VOID
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN8)
VOID
IoTransferActivityId (
    _In_ LPCGUID ActivityId,
    _In_ LPCGUID RelatedActivityId
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WINBLUE)

NTSTATUS
IoGetFsZeroingOffset (
    _In_ PIRP Irp,
    _Out_ PULONG ZeroingOffset
    );

NTSTATUS
IoSetFsZeroingOffsetRequired (
    _Inout_ PIRP Irp
    );

NTSTATUS
IoSetFsZeroingOffset (
    _Inout_ PIRP Irp,
    _In_ ULONG ZeroingOffset
    );

#endif



#if (NTDDI_VERSION >= NTDDI_WIN8)
NTKERNELAPI
LOGICAL
IoIsValidIrpStatus (
    IN NTSTATUS Status
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN8)
NTKERNELAPI
NTSTATUS
IoIncrementKeepAliveCount(
    _Inout_ PFILE_OBJECT FileObject,
    _Inout_ PEPROCESS Process
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN8)
NTKERNELAPI
NTSTATUS
IoDecrementKeepAliveCount(
    _Inout_ PFILE_OBJECT FileObject,
    _Inout_ PEPROCESS Process
    );
#endif


#if (NTDDI_VERSION >= NTDDI_WIN8)
NTKERNELAPI
PEPROCESS
IoGetInitiatorProcess(
    _In_ PFILE_OBJECT FileObject
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN8)
NTKERNELAPI
VOID
IoSetMasterIrpStatus(
    _Inout_ PIRP MasterIrp,
    _In_ NTSTATUS Status
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WINBLUE)
NTKERNELAPI
NTSTATUS
IoQueryFullDriverPath(
    _In_ PDRIVER_OBJECT DriverObject,
    _Out_ PUNICODE_STRING FullPath
    );
#endif

typedef struct _AGP_TARGET_BUS_INTERFACE_STANDARD {
    //
    // generic interface header
    //
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;

    //
    // config munging routines
    //
    PGET_SET_DEVICE_DATA SetBusData;
    PGET_SET_DEVICE_DATA GetBusData;
    UCHAR CapabilityID;  // 2 (AGPv2 host) or new 0xE (AGPv3 bridge)

} AGP_TARGET_BUS_INTERFACE_STANDARD, *PAGP_TARGET_BUS_INTERFACE_STANDARD;


#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTKERNELAPI
NTSTATUS
IoReportDetectedDevice(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ INTERFACE_TYPE LegacyBusType,
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _In_opt_ PCM_RESOURCE_LIST ResourceList,
    _In_opt_ PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirements,
    _In_ BOOLEAN ResourceAssigned,
    _Inout_ PDEVICE_OBJECT *DeviceObject
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTKERNELAPI
NTSTATUS
IoReportRootDevice(
    _In_ PDRIVER_OBJECT DriverObject
    );
#endif

//
// Device location interface declarations
//
typedef
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
(*PGET_LOCATION_STRING) (
    _Inout_opt_ PVOID Context,
    _Outptr_
    _At_(*LocationStrings,
        _When_(return == 0, __drv_allocatesMem(Mem)))
    PZZWSTR *LocationStrings
    );

typedef struct _PNP_LOCATION_INTERFACE {
    //
    // generic interface header
    //
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;

    //
    // interface specific entry
    //
    PGET_LOCATION_STRING GetLocationString;

} PNP_LOCATION_INTERFACE, *PPNP_LOCATION_INTERFACE;

//
// Resource arbiter declarations
//

typedef enum _ARBITER_ACTION {
    ArbiterActionTestAllocation,
    ArbiterActionRetestAllocation,
    ArbiterActionCommitAllocation,
    ArbiterActionRollbackAllocation,
    ArbiterActionQueryAllocatedResources,
    ArbiterActionWriteReservedResources,
    ArbiterActionQueryConflict,
    ArbiterActionQueryArbitrate,
    ArbiterActionAddReserved,
    ArbiterActionBootAllocation
} ARBITER_ACTION, *PARBITER_ACTION;

typedef struct _ARBITER_CONFLICT_INFO {
    //
    // The device object owning the device that is causing the conflict
    //
    PDEVICE_OBJECT OwningObject;

    //
    // The start of the conflicting range
    //
    ULONGLONG Start;

    //
    // The end of the conflicting range
    //
    ULONGLONG End;

} ARBITER_CONFLICT_INFO, *PARBITER_CONFLICT_INFO;

//
// The parameters for those actions
//

typedef struct _ARBITER_TEST_ALLOCATION_PARAMETERS {

    //
    // Doubly linked list of ARBITER_LIST_ENTRY's
    //
    _Inout_ PLIST_ENTRY ArbitrationList;

    //
    // The size of the AllocateFrom array
    //
    _In_ ULONG AllocateFromCount;

    //
    // Array of resource descriptors describing the resources available
    // to the arbiter for it to arbitrate
    //
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR AllocateFrom;

} ARBITER_TEST_ALLOCATION_PARAMETERS, *PARBITER_TEST_ALLOCATION_PARAMETERS;


typedef struct _ARBITER_RETEST_ALLOCATION_PARAMETERS {

    //
    // Doubly linked list of ARBITER_LIST_ENTRY's
    //
    _Inout_ PLIST_ENTRY ArbitrationList;

    //
    // The size of the AllocateFrom array
    //
    _In_ ULONG AllocateFromCount;

    //
    // Array of resource descriptors describing the resources available
    // to the arbiter for it to arbitrate
    //
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR AllocateFrom;

} ARBITER_RETEST_ALLOCATION_PARAMETERS, *PARBITER_RETEST_ALLOCATION_PARAMETERS;

typedef struct _ARBITER_BOOT_ALLOCATION_PARAMETERS {

    //
    // Doubly linked list of ARBITER_LIST_ENTRY's
    //
    _Inout_ PLIST_ENTRY ArbitrationList;

} ARBITER_BOOT_ALLOCATION_PARAMETERS, *PARBITER_BOOT_ALLOCATION_PARAMETERS;


typedef struct _ARBITER_QUERY_ALLOCATED_RESOURCES_PARAMETERS {

    //
    // The resources that are currently allocated
    //
    _Out_ PCM_PARTIAL_RESOURCE_LIST *AllocatedResources;

} ARBITER_QUERY_ALLOCATED_RESOURCES_PARAMETERS, *PARBITER_QUERY_ALLOCATED_RESOURCES_PARAMETERS;

typedef struct _ARBITER_QUERY_CONFLICT_PARAMETERS {

    //
    // This is the device we are trying to find a conflict for
    //
    _In_ PDEVICE_OBJECT PhysicalDeviceObject;

    //
    // This is the resource to find the conflict for
    //
    _In_ PIO_RESOURCE_DESCRIPTOR ConflictingResource;

    //
    // Number of devices conflicting on the resource
    //
    _Out_ PULONG ConflictCount;

    //
    // Pointer to array describing the conflicting device objects and ranges
    //
    _Out_ PARBITER_CONFLICT_INFO *Conflicts;

} ARBITER_QUERY_CONFLICT_PARAMETERS, *PARBITER_QUERY_CONFLICT_PARAMETERS;

typedef struct _ARBITER_QUERY_ARBITRATE_PARAMETERS {

    //
    // Doubly linked list of ARBITER_LIST_ENTRY's - should have
    // only one entry
    //
    _In_ PLIST_ENTRY ArbitrationList;

} ARBITER_QUERY_ARBITRATE_PARAMETERS, *PARBITER_QUERY_ARBITRATE_PARAMETERS;

typedef struct _ARBITER_ADD_RESERVED_PARAMETERS {

    //
    // Doubly linked list of ARBITER_LIST_ENTRY's - should have
    // only one entry
    //
    _In_ PDEVICE_OBJECT ReserveDevice;

} ARBITER_ADD_RESERVED_PARAMETERS, *PARBITER_ADD_RESERVED_PARAMETERS;


typedef struct _ARBITER_PARAMETERS {

    union {

        ARBITER_TEST_ALLOCATION_PARAMETERS              TestAllocation;
        ARBITER_RETEST_ALLOCATION_PARAMETERS            RetestAllocation;
        ARBITER_BOOT_ALLOCATION_PARAMETERS              BootAllocation;
        ARBITER_QUERY_ALLOCATED_RESOURCES_PARAMETERS    QueryAllocatedResources;
        ARBITER_QUERY_CONFLICT_PARAMETERS               QueryConflict;
        ARBITER_QUERY_ARBITRATE_PARAMETERS              QueryArbitrate;
        ARBITER_ADD_RESERVED_PARAMETERS                 AddReserved;

    } Parameters;

} ARBITER_PARAMETERS, *PARBITER_PARAMETERS;

typedef enum _ARBITER_REQUEST_SOURCE {

    ArbiterRequestUndefined = -1,
    ArbiterRequestLegacyReported,   // IoReportResourceUsage
    ArbiterRequestHalReported,      // IoReportHalResourceUsage
    ArbiterRequestLegacyAssigned,   // IoAssignResources
    ArbiterRequestPnpDetected,      // IoReportResourceForDetection
    ArbiterRequestPnpEnumerated     // IRP_MN_QUERY_RESOURCE_REQUIREMENTS

} ARBITER_REQUEST_SOURCE;


typedef enum _ARBITER_RESULT {

    ArbiterResultUndefined = -1,
    ArbiterResultSuccess,
    ArbiterResultExternalConflict, // This indicates that the request can never be solved for devices in this list
    ArbiterResultNullRequest       // The request was for length zero and thus no translation should be attempted

} ARBITER_RESULT;

//
// ARBITER_FLAG_BOOT_CONFIG - this indicates that the request is for the
// resources assigned by the firmware/BIOS.  It should be succeeded even if
// it conflicts with another devices boot config.
//

#define ARBITER_FLAG_BOOT_CONFIG 0x00000001

//
// ARBITER_FLAG_ROOT_ENUM - This indicates that the associated device was root-
// enumerated.  This might happen either because somebody force-installed a
// PnP driver without a bus driver or because we've associated a stand-in device
// for an old NT-4-style non-PnP driver.
//

#define ARBITER_FLAG_ROOT_ENUM   0x00000002

//
// ARBITER_FLAG_OTHER_ENUM - This indicates that the device was not a candidate
// for ARBITER_FLAG_ROOT_ENUM.  This exists as its own flag to differentiate the
// case where the status of the device has not yet been determined.
//

#define ARBITER_FLAG_OTHER_ENUM  0x00000004

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTKERNELAPI
NTSTATUS
IoReportResourceForDetection(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_reads_bytes_opt_(DriverListSize) PCM_RESOURCE_LIST DriverList,
    _In_opt_ ULONG DriverListSize,
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_reads_bytes_opt_(DeviceListSize) PCM_RESOURCE_LIST DeviceList,
    _In_opt_ ULONG DeviceListSize,
    _Out_ PBOOLEAN ConflictDetected
    );
#endif

typedef struct _ARBITER_LIST_ENTRY {

    //
    // This is a doubly linked list of entries for easy sorting
    //
    LIST_ENTRY ListEntry;

    //
    // The number of alternative allocation
    //
    ULONG AlternativeCount;

    //
    // Pointer to an array of resource descriptors for the possible allocations
    //
    _Field_size_(AlternativeCount)
    PIO_RESOURCE_DESCRIPTOR Alternatives;

    //
    // The device object of the device requesting these resources.
    //
    PDEVICE_OBJECT PhysicalDeviceObject;

    //
    // Indicates where the request came from
    //
    ARBITER_REQUEST_SOURCE RequestSource;

    //
    // Flags these indicate a variety of things (use ARBITER_FLAG_*)
    //
    ULONG Flags;

    //
    // Space to aid the arbiter in processing the list it is initialized to 0 when
    // the entry is created.  The system will not attempt to interpret it.
    //
    LONG_PTR WorkSpace;

    //
    // Interface Type, Slot Number and Bus Number from Resource Requirements list,
    // used only for reverse identification.
    //
    INTERFACE_TYPE InterfaceType;
    ULONG SlotNumber;
    ULONG BusNumber;

    //
    // A pointer to a descriptor to indicate the resource that was allocated.
    // This is allocated by the system and filled in by the arbiter in response to an
    // ArbiterActionTestAllocation.
    //
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Assignment;

    //
    // Pointer to the alternative that was chosen from to provide the assignment.
    // This is filled in by the arbiter in response to an ArbiterActionTestAllocation.
    //
    PIO_RESOURCE_DESCRIPTOR SelectedAlternative;

    //
    // The result of the operation
    // This is filled in by the arbiter in response to an ArbiterActionTestAllocation.
    //
    ARBITER_RESULT Result;

} ARBITER_LIST_ENTRY, *PARBITER_LIST_ENTRY;

//
// The arbiter's entry point
//

typedef
NTSTATUS
(*PARBITER_HANDLER) (
    _Inout_opt_ PVOID Context,
    _In_ ARBITER_ACTION Action,
    _Inout_ PARBITER_PARAMETERS Parameters
    );

//
// Arbiter interface
//

#define ARBITER_PARTIAL   0x00000001


typedef struct _ARBITER_INTERFACE {

    //
    // Generic interface header
    //
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;

    //
    // Entry point to the arbiter
    //
    PARBITER_HANDLER ArbiterHandler;

    //
    // Other information about the arbiter, use ARBITER_* flags
    //
    ULONG Flags;

} ARBITER_INTERFACE, *PARBITER_INTERFACE;

//
// The directions translation can take place in
//

typedef enum _RESOURCE_TRANSLATION_DIRECTION {
    TranslateChildToParent,
    TranslateParentToChild
} RESOURCE_TRANSLATION_DIRECTION;

//
// Translation functions
//

typedef
NTSTATUS
(*PTRANSLATE_RESOURCE_HANDLER)(
    _Inout_opt_ PVOID Context,
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    _In_ RESOURCE_TRANSLATION_DIRECTION Direction,
    _In_opt_ ULONG AlternativesCount,
    _In_reads_opt_(AlternativesCount) IO_RESOURCE_DESCRIPTOR Alternatives[],
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
);

typedef
NTSTATUS
(*PTRANSLATE_RESOURCE_REQUIREMENTS_HANDLER)(
    _Inout_opt_ PVOID Context,
    _In_ PIO_RESOURCE_DESCRIPTOR Source,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _Out_ PULONG TargetCount,
    _Out_writes_(*TargetCount) PIO_RESOURCE_DESCRIPTOR *Target
);

//
// Translator Interface
//

typedef struct _TRANSLATOR_INTERFACE {
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    PTRANSLATE_RESOURCE_HANDLER TranslateResources;
    PTRANSLATE_RESOURCE_REQUIREMENTS_HANDLER TranslateResourceRequirements;
} TRANSLATOR_INTERFACE, *PTRANSLATOR_INTERFACE;

//
// The following function prototypes are for HAL routines with a prefix of Hal.
//
// General functions.
//

typedef
BOOLEAN
(*PHAL_RESET_DISPLAY_PARAMETERS) (
    _In_ ULONG Columns,
    _In_ ULONG Rows
    );

#if (NTDDI_VERSION >= NTDDI_WIN2K)
DECLSPEC_DEPRECATED_DDK
NTHALAPI
VOID
HalAcquireDisplayOwnership (
    _In_ PHAL_RESET_DISPLAY_PARAMETERS ResetDisplayParameters
    );
#endif

                                                
#if defined(_M_IX86) || defined(_M_AMD64)       
                                                
#define HalGetDmaAlignmentRequirement() 1L      
                                                
#elif defined(_M_ARM) || defined(_M_ARM64)      
                                                
#define HalGetDmaAlignmentRequirement() 1L      
                                                
#endif                                          
                                                
#if defined(_ARM_) || defined(_ARM64_)          
                                                
#endif                                          
                                                
//
// I/O driver configuration functions.
//
#if !defined(NO_LEGACY_DRIVERS)
#if (NTDDI_VERSION >= NTDDI_WIN2K)
DECLSPEC_DEPRECATED_DDK                 // Use Pnp or IoReportDetectedDevice
__drv_preferredFunction("(see documentation)", "Obsolete")
NTHALAPI
NTSTATUS
HalAssignSlotResources (
    _In_ PUNICODE_STRING RegistryPath,
    _In_ PUNICODE_STRING DriverClassName OPTIONAL,
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ INTERFACE_TYPE BusType,
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _Inout_ PCM_RESOURCE_LIST *AllocatedResources
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
DECLSPEC_DEPRECATED_DDK                 // Use Pnp or IoReportDetectedDevice
_IRQL_requires_max_(PASSIVE_LEVEL)
__drv_preferredFunction(
    "IoReportDetectedDevice and IoReportResourceForDetection", "Obsolete")
NTHALAPI
ULONG
HalGetInterruptVector (
    _In_ INTERFACE_TYPE  InterfaceType,
    _In_ ULONG BusNumber,
    _In_ ULONG BusInterruptLevel,
    _In_ ULONG BusInterruptVector,
    _Out_ PKIRQL Irql,
    _Out_ PKAFFINITY Affinity
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
DECLSPEC_DEPRECATED_DDK                 // Use IRP_MN_QUERY_INTERFACE and IRP_MN_READ_CONFIG
_When_(BusDataType!=0,  /* Cmos == 0 */
    __drv_preferredFunction(
    "IRP_MN_QUERY_INTERFACE and IRP_MN_WRITE_CONFIG requests",
    "Obsolete except for BusDataType==Cmos"))
NTHALAPI
ULONG
HalSetBusData (
    _In_ BUS_DATA_TYPE BusDataType,
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    );
#endif

#endif // NO_LEGACY_DRIVERS

#if (NTDDI_VERSION >= NTDDI_WIN2K)
DECLSPEC_DEPRECATED_DDK                 // Use IRP_MN_QUERY_INTERFACE and IRP_MN_READ_CONFIG
_When_(BusDataType!=0, /* Cmos == 0 */
    __drv_preferredFunction(
    "IRP_MN_QUERY_INTERFACE and IRP_MN_WRITE_CONFIG requests",
    "Obsolete except for BusDataType==Cmos"))
NTHALAPI
ULONG
HalSetBusDataByOffset (
    _In_ BUS_DATA_TYPE BusDataType,
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
DECLSPEC_DEPRECATED_DDK                 // Use IRP_MN_QUERY_INTERFACE and IRP_MN_READ_CONFIG
__drv_preferredFunction("(see documentation)", "Obsolete")
NTHALAPI
BOOLEAN
HalTranslateBusAddress (
    _In_ INTERFACE_TYPE  InterfaceType,
    _In_ ULONG BusNumber,
    _In_ PHYSICAL_ADDRESS BusAddress,
    _Inout_ PULONG AddressSpace,
    _Out_ PPHYSICAL_ADDRESS TranslatedAddress
    );
#endif

//
// Values for AddressSpace parameter of HalTranslateBusAddress
//
//      0x0         - Memory space
//      0x1         - Port space
//      0x2 - 0x1F  - Address spaces specific for Alpha
//                      0x2 - UserMode view of memory space
//                      0x3 - UserMode view of port space
//                      0x4 - Dense memory space
//                      0x5 - reserved
//                      0x6 - UserMode view of dense memory space
//                      0x7 - 0x1F - reserved
//

#if !defined(XBOX_SYSTEMOS)

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTHALAPI
PVOID
HalAllocateCrashDumpRegisters (
    _In_ PADAPTER_OBJECT AdapterObject,
    _Inout_ PULONG NumberOfMapRegisters
    );
#endif

#endif

#if !defined(XBOX_SYSTEMOS)

#if (NTDDI_VERSION >= NTDDI_WIN8)

typedef enum _HAL_DMA_CRASH_DUMP_REGISTER_TYPE {
    HalDmaCrashDumpRegisterSet1 = 0,
    HalDmaCrashDumpRegisterSet2,
    HalDmaCrashDumpRegisterSetMax
} HAL_DMA_CRASH_DUMP_REGISTER_TYPE, *PHAL_DMA_CRASH_DUMP_REGISTER_TYPE;


NTSTATUS
HalDmaAllocateCrashDumpRegistersEx (
    _In_ PADAPTER_OBJECT Adapter,
    _In_ ULONG NumberOfMapRegisters,
    _In_ HAL_DMA_CRASH_DUMP_REGISTER_TYPE Type,
    _Out_ PVOID *MapRegisterBase,
    _Out_ PULONG MapRegistersAvailable
    );


NTSTATUS
HalDmaFreeCrashDumpRegistersEx (
    _In_ PADAPTER_OBJECT Adapter,
    _In_ HAL_DMA_CRASH_DUMP_REGISTER_TYPE Type
    );
#endif

#endif

#if !defined(NO_LEGACY_DRIVERS)
#if (NTDDI_VERSION >= NTDDI_WIN2K)
DECLSPEC_DEPRECATED_DDK                 // Use IRP_MN_QUERY_INTERFACE and IRP_MN_READ_CONFIG
_When_(BusDataType!=0,
    __drv_preferredFunction(
        "IRP_MN_QUERY_INTERFACE and IRP_MN_READ_CONFIG requests",
        "Obsolete except for BusDataType==Cmos"))
NTHALAPI
ULONG
HalGetBusData (
    _In_ BUS_DATA_TYPE BusDataType,
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    );
#endif
#endif // NO_LEGACY_DRIVERS

#if (NTDDI_VERSION >= NTDDI_WIN2K)
DECLSPEC_DEPRECATED_DDK                 // Use IRP_MN_QUERY_INTERFACE and IRP_MN_READ_CONFIG
_When_(BusDataType!=0,
    __drv_preferredFunction("IRP_MN_QUERY_INTERFACE",
    "Obsolete except for BusDataType==Cmos"))
NTHALAPI
ULONG
HalGetBusDataByOffset (
    _In_ BUS_DATA_TYPE BusDataType,
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
DECLSPEC_DEPRECATED_DDK                 // Use IoGetDmaAdapter
_IRQL_requires_max_(PASSIVE_LEVEL)
__drv_preferredFunction("IoGetDmaAdapter", "Obsolete")
NTHALAPI
PADAPTER_OBJECT
HalGetAdapter (
    _In_ PDEVICE_DESCRIPTION DeviceDescription,
    _Out_ PULONG NumberOfMapRegisters
    );
#endif

//
// System beep functions.
//
#if !defined(NO_LEGACY_DRIVERS)
#if (NTDDI_VERSION >= NTDDI_WIN2K)
DECLSPEC_DEPRECATED_DDK
NTHALAPI
BOOLEAN
HalMakeBeep(
    _In_ ULONG Frequency
    );
#endif
#endif // NO_LEGACY_DRIVERS

//
// The following function prototypes are for HAL routines with a prefix of Io.
//
// DMA adapter object functions.
//

typedef
PBUS_HANDLER
(FASTCALL *pHalHandlerForBus) (
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG          BusNumber
    );
typedef
VOID
(FASTCALL *pHalReferenceBusHandler) (
    _In_ PBUS_HANDLER   BusHandler
    );

//-----------------------------------------------------------------------------
//      HAL Function dispatch
//

typedef enum _HAL_QUERY_INFORMATION_CLASS {
    HalInstalledBusInformation,
    HalProfileSourceInformation,
    HalInformationClassUnused1,
    HalPowerInformation,
    HalProcessorSpeedInformation,
    HalCallbackInformation,
    HalMapRegisterInformation,
    HalMcaLogInformation,               // Machine Check Abort Information
    HalFrameBufferCachingInformation,
    HalDisplayBiosInformation,
    HalProcessorFeatureInformation,
    HalNumaTopologyInterface,
    HalErrorInformation,                // General MCA, CMC, CPE Error Information.
    HalCmcLogInformation,               // Processor Corrected Machine Check Information
    HalCpeLogInformation,               // Corrected Platform Error Information
    HalQueryMcaInterface,
    HalQueryAMLIIllegalIOPortAddresses,
    HalQueryMaxHotPlugMemoryAddress,
    HalPartitionIpiInterface,
    HalPlatformInformation,
    HalQueryProfileSourceList,
    HalInitLogInformation,
    HalFrequencyInformation,
    HalProcessorBrandString,
    HalHypervisorInformation,
    HalPlatformTimerInformation,
    HalAcpiAuditInformation,
    HalIrtInformation,
    HalSecondaryInterruptInformation,
    HalParkingPageInformation,
    HalNumaRangeTableInformation,
    HalChannelTopologyInformation,
    HalExternalCacheInformation,
    HalQueryDebuggerInformation,
    HalFwBootPerformanceInformation,
    HalFwS3PerformanceInformation,
    HalGetChannelPowerInformation,
    HalQueryStateElementInformation,
    HalPsciInformation,
    HalInterruptControllerInformation,
    HalQueryIommuReservedRegionInformation,
    HalQueryArmErrataInformation,
    HalQueryProcessorEfficiencyInformation,
    HalQueryAcpiWakeAlarmSystemPowerStateInformation,
    HalQueryProfileNumberOfCounters,
    HalQueryHyperlaunchEntrypoint,
    HalHardwareWatchdogInformation,
    HalDmaRemappingInformation,
    HalQueryRuntimeServicesBlockInformation,
    // information levels >= 0x8000000 reserved for OEM use
} HAL_QUERY_INFORMATION_CLASS, *PHAL_QUERY_INFORMATION_CLASS;


typedef enum _HAL_SET_INFORMATION_CLASS {
    HalProfileSourceInterval,
    HalProfileSourceInterruptHandler,  // Register performance monitor interrupt callback
    HalMcaRegisterDriver,              // Register Machine Check Abort driver
    HalKernelErrorHandler,
    HalCmcRegisterDriver,              // Register Processor Corrected Machine Check driver
    HalCpeRegisterDriver,              // Register Corrected Platform  Error driver
    HalMcaLog,
    HalCmcLog,
    HalCpeLog,
    HalGenerateCmcInterrupt,           // Used to test CMC
    HalProfileSourceTimerHandler,      // Resister profile timer interrupt callback
    HalEnlightenment,
    HalProfileDpgoSourceInterruptHandler,  // Register performance monitor interrupt callback for dpgo
    HalRegisterSecondaryInterruptInterface,
    HalSetChannelPowerInformation,
    HalI386ExceptionChainTerminatorInformation, // Set x86 SEHOP exception chain terminator
    HalSetResetParkDisposition,        // Set whether to park processors on reset (LOGICAL)
    HalSetPsciSuspendMode,
    HalSetHvciEnabled,
    HalSetProcessorTraceInterruptHandler, // Register performance monitor interrupt callback for Intel Processor Trace
    HalProfileSourceAdd,
    HalProfileSourceRemove
} HAL_SET_INFORMATION_CLASS, *PHAL_SET_INFORMATION_CLASS;




typedef
NTSTATUS
(*pHalQuerySystemInformation)(
    _In_ HAL_QUERY_INFORMATION_CLASS  InformationClass,
    _In_ ULONG     BufferSize,
    _Out_writes_bytes_to_(BufferSize, *ReturnedLength) PVOID Buffer,
    _Out_ PULONG   ReturnedLength
    );


typedef
NTSTATUS
(*pHalSetSystemInformation)(
    _In_ HAL_SET_INFORMATION_CLASS    InformationClass,
    _In_ ULONG     BufferSize,
    _In_ PVOID     Buffer
    );


typedef
VOID
(FASTCALL *pHalExamineMBR)(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG SectorSize,
    _In_ ULONG MBRTypeIdentifier,
    _Out_ PVOID *Buffer
    );

typedef
NTSTATUS
(FASTCALL *pHalIoReadPartitionTable)(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG SectorSize,
    _In_ BOOLEAN ReturnRecognizedPartitions,
    _Out_ struct _DRIVE_LAYOUT_INFORMATION **PartitionBuffer
    );

typedef
NTSTATUS
(FASTCALL *pHalIoSetPartitionInformation)(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG SectorSize,
    _In_ ULONG PartitionNumber,
    _In_ ULONG PartitionType
    );

typedef
NTSTATUS
(FASTCALL *pHalIoWritePartitionTable)(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG SectorSize,
    _In_ ULONG SectorsPerTrack,
    _In_ ULONG NumberOfHeads,
    _In_ struct _DRIVE_LAYOUT_INFORMATION *PartitionBuffer
    );

typedef
NTSTATUS
(*pHalQueryBusSlots)(
    _In_ PBUS_HANDLER         BusHandler,
    _In_ ULONG                BufferSize,
    _Out_ PULONG              SlotNumbers,
    _Out_ PULONG              ReturnedLength
    );

typedef
NTSTATUS
(*pHalInitPnpDriver)(
    VOID
    );


typedef struct _PM_DISPATCH_TABLE {
    ULONG   Signature;
    ULONG   Version;
    PVOID   Function[1];
} PM_DISPATCH_TABLE, *PPM_DISPATCH_TABLE;



typedef
NTSTATUS
(*pHalInitPowerManagement)(
    _In_ PPM_DISPATCH_TABLE  PmDriverDispatchTable,
    _Out_ PPM_DISPATCH_TABLE *PmHalDispatchTable
    );


typedef
struct _DMA_ADAPTER *
(*pHalGetDmaAdapter)(
    _In_opt_ PVOID PhysicalDeviceObject,
    _In_ struct _DEVICE_DESCRIPTION *DeviceDescriptor,
    _Out_ PULONG NumberOfMapRegisters
    );


typedef
NTSTATUS
(*pHalGetInterruptTranslator)(
    _In_ INTERFACE_TYPE ParentInterfaceType,
    _In_ ULONG ParentBusNumber,
    _In_ INTERFACE_TYPE BridgeInterfaceType,
    _In_ USHORT Size,
    _In_ USHORT Version,
    _Out_ PTRANSLATOR_INTERFACE Translator,
    _Out_ PULONG BridgeBusNumber
    );


typedef
BOOLEAN
(*pHalTranslateBusAddress)(
    _In_ INTERFACE_TYPE  InterfaceType,
    _In_ ULONG BusNumber,
    _In_ PHYSICAL_ADDRESS BusAddress,
    _Inout_ PULONG AddressSpace,
    _Out_ PPHYSICAL_ADDRESS TranslatedAddress
    );

typedef
NTSTATUS
(*pHalAssignSlotResources) (
    _In_ PUNICODE_STRING RegistryPath,
    _In_ PUNICODE_STRING DriverClassName OPTIONAL,
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ INTERFACE_TYPE BusType,
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _Inout_ PCM_RESOURCE_LIST *AllocatedResources
    );

typedef
VOID
(*pHalHaltSystem) (
    VOID
    );

typedef
BOOLEAN
(*pHalResetDisplay) (
    VOID
    );



typedef struct _MAP_REGISTER_ENTRY {
    PVOID   MapRegister;
    BOOLEAN WriteToDevice;
} MAP_REGISTER_ENTRY, *PMAP_REGISTER_ENTRY;




typedef
UCHAR
(*pHalVectorToIDTEntry) (
    ULONG Vector
);

typedef
BOOLEAN
(*pHalFindBusAddressTranslation) (
    _In_ PHYSICAL_ADDRESS BusAddress,
    _Inout_ PULONG AddressSpace,
    _Out_ PPHYSICAL_ADDRESS TranslatedAddress,
    _Inout_ PULONG_PTR Context,
    _In_ BOOLEAN NextBus
    );

typedef
NTSTATUS
(*pHalStartMirroring)(
    VOID
    );

typedef
NTSTATUS
(*pHalEndMirroring)(
    _In_ ULONG PassNumber
    );

typedef
NTSTATUS
(*pHalMirrorPhysicalMemory)(
    _In_ PHYSICAL_ADDRESS PhysicalAddress,
    _In_ LARGE_INTEGER NumberOfBytes
    );

typedef
NTSTATUS
(*pHalMirrorVerify)(
    _In_ PHYSICAL_ADDRESS PhysicalAddress,
    _In_ LARGE_INTEGER NumberOfBytes
    );





typedef struct {
    UCHAR Type;  //CmResourceType
    BOOLEAN Valid;

    union {
        UCHAR Reserved[2];

        struct {
            UCHAR BitWidth;
            UCHAR AccessSize;
        };
    };

    PUCHAR    TranslatedAddress;
    ULONG     Length;
} DEBUG_DEVICE_ADDRESS, *PDEBUG_DEVICE_ADDRESS;

typedef struct {
    PHYSICAL_ADDRESS  Start;
    PHYSICAL_ADDRESS  MaxEnd;
    PVOID             VirtualAddress;
    ULONG             Length;
    BOOLEAN           Cached;
    BOOLEAN           Aligned;
} DEBUG_MEMORY_REQUIREMENTS, *PDEBUG_MEMORY_REQUIREMENTS;

typedef enum {
    KdNameSpacePCI,
    KdNameSpaceACPI,
    KdNameSpaceAny,
    KdNameSpaceNone,

    //
    // Maximum namespace enumerator.
    //

    KdNameSpaceMax,
} KD_NAMESPACE_ENUM, *PKD_NAMESPACE_ENUM;

typedef enum {
    KdConfigureDeviceAndContinue,
    KdSkipDeviceAndContinue,
    KdConfigureDeviceAndStop,
    KdSkipDeviceAndStop,
} KD_CALLBACK_ACTION, *PKD_CALLBACK_ACTION;

//
// Debug transport specific data for use by the transport.
//

typedef struct _DEBUG_TRANSPORT_DATA {
    ULONG HwContextSize;
    BOOLEAN UseSerialFraming;
} DEBUG_TRANSPORT_DATA, *PDEBUG_TRANSPORT_DATA;

#define MAXIMUM_DEBUG_BARS 6

#define DBG_DEVICE_FLAG_HAL_SCRATCH_ALLOCATED 0x01
#define DBG_DEVICE_FLAG_BARS_MAPPED           0x02
#define DBG_DEVICE_FLAG_SCRATCH_ALLOCATED     0x04
#define DBG_DEVICE_FLAG_UNCACHED_MEMORY       0x08
#define DBG_DEVICE_FLAG_SYNTHETIC             0x10

typedef struct _DEBUG_DEVICE_DESCRIPTOR {
    ULONG     Bus;
    ULONG     Slot;
    USHORT    Segment;
    USHORT    VendorID;
    USHORT    DeviceID;
    UCHAR     BaseClass;
    UCHAR     SubClass;
    UCHAR     ProgIf;
    union {
        UCHAR     Flags;
        struct {
            UCHAR DbgHalScratchAllocated : 1;
            UCHAR DbgBarsMapped : 1;
            UCHAR DbgScratchAllocated : 1;
        };
    };
    BOOLEAN   Initialized;
    BOOLEAN   Configured;
    DEBUG_DEVICE_ADDRESS BaseAddress[MAXIMUM_DEBUG_BARS];
    DEBUG_MEMORY_REQUIREMENTS Memory;
    USHORT    PortType;
    USHORT    PortSubtype;
    PVOID     OemData;
    ULONG     OemDataLength;
    KD_NAMESPACE_ENUM NameSpace;
    PWCHAR    NameSpacePath;
    ULONG     NameSpacePathLength;
    ULONG     TransportType;
    DEBUG_TRANSPORT_DATA TransportData;
} DEBUG_DEVICE_DESCRIPTOR, *PDEBUG_DEVICE_DESCRIPTOR;



typedef struct _PCI_DEBUGGING_DEVICE_IN_USE {
    USHORT Segment;
    ULONG Bus;
    ULONG Slot;
} PCI_DEBUGGING_DEVICE_IN_USE, *PPCI_DEBUGGING_DEVICE_IN_USE;

typedef struct _ACPI_DEBUGGING_DEVICE_IN_USE {
    ULONG NameSpacePathLength;
    WCHAR NameSpacePath[ANYSIZE_ARRAY];
} ACPI_DEBUGGING_DEVICE_IN_USE, *PACPI_DEBUGGING_DEVICE_IN_USE;

typedef struct _DEBUGGING_DEVICE_IN_USE {
    KD_NAMESPACE_ENUM NameSpace;
    ULONG StructureLength;
    union {
        ACPI_DEBUGGING_DEVICE_IN_USE AcpiDevice;
        PCI_DEBUGGING_DEVICE_IN_USE PciDevice;
    };
} DEBUGGING_DEVICE_IN_USE, *PDEBUGGING_DEVICE_IN_USE;

typedef struct _DEBUGGING_DEVICE_IN_USE_INFORMATION {
    ULONG DeviceCount;
    DEBUGGING_DEVICE_IN_USE Device[ANYSIZE_ARRAY];
} DEBUGGING_DEVICE_IN_USE_INFORMATION, *PDEBUGGING_DEVICE_IN_USE_INFORMATION;



typedef
KD_CALLBACK_ACTION
(*PDEBUG_DEVICE_FOUND_FUNCTION) (
    PDEBUG_DEVICE_DESCRIPTOR Device
    );

typedef
NTSTATUS
(*pKdEnumerateDebuggingDevices) (
    _In_ PVOID LoaderBlock,
    _Inout_ PDEBUG_DEVICE_DESCRIPTOR Device,
    _In_ PDEBUG_DEVICE_FOUND_FUNCTION Callback
    );

typedef
NTSTATUS
(*pKdSetupPciDeviceForDebugging)(
    _In_    PVOID                     LoaderBlock,   OPTIONAL
    _Inout_ PDEBUG_DEVICE_DESCRIPTOR  PciDevice
);

typedef
NTSTATUS
(*pKdReleasePciDeviceForDebugging)(
    _Inout_ PDEBUG_DEVICE_DESCRIPTOR  PciDevice
);

typedef
NTSTATUS
(*pKdSetupIntegratedDeviceForDebugging)(
    _In_    PVOID                     LoaderBlock,   OPTIONAL
    _Inout_ PDEBUG_DEVICE_DESCRIPTOR  IntegratedDevice
);

typedef
NTSTATUS
(*pKdReleaseIntegratedDeviceForDebugging)(
    _Inout_ PDEBUG_DEVICE_DESCRIPTOR  IntegratedDevice
);

typedef
PVOID
(*pKdGetAcpiTablePhase0)(
    _In_ struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
    _In_ ULONG Signature
    );

typedef
VOID
(*pKdCheckPowerButton)(
    VOID
    );

typedef
VOID
(*pHalEndOfBoot)(
    VOID
    );

typedef
PVOID
(*pKdMapPhysicalMemory64)(
    _In_ PHYSICAL_ADDRESS PhysicalAddress,
    _In_ ULONG NumberPages,
    _In_ BOOLEAN FlushCurrentTLB
    );

typedef
VOID
(*pKdUnmapVirtualAddress)(
    _In_ PVOID VirtualAddress,
    _In_ ULONG NumberPages,
    _In_ BOOLEAN FlushCurrentTLB
    );

typedef
ULONG
(*pKdGetPciDataByOffset)(
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length
    );

typedef
ULONG
(*pKdSetPciDataByOffset)(
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length
    );

typedef
PVOID
(*pHalGetAcpiTable)(
    _In_ ULONG Signature,
    _In_opt_ PCSTR OemId,
    _In_opt_ PCSTR OemTableId
    );

typedef
VOID
(*PCI_ERROR_HANDLER_CALLBACK)(
    VOID
    );

typedef
VOID
(*pHalSetPciErrorHandlerCallback)(
    _In_ PCI_ERROR_HANDLER_CALLBACK Callback
    );




typedef struct {
    ULONG                           Version;
    pHalQuerySystemInformation      HalQuerySystemInformation;
    pHalSetSystemInformation        HalSetSystemInformation;
    pHalQueryBusSlots               HalQueryBusSlots;
    ULONG                           Spare1;
    pHalExamineMBR                  HalExamineMBR;
    pHalIoReadPartitionTable        HalIoReadPartitionTable;
    pHalIoSetPartitionInformation   HalIoSetPartitionInformation;
    pHalIoWritePartitionTable       HalIoWritePartitionTable;

    pHalHandlerForBus               HalReferenceHandlerForBus;
    pHalReferenceBusHandler         HalReferenceBusHandler;
    pHalReferenceBusHandler         HalDereferenceBusHandler;

    pHalInitPnpDriver               HalInitPnpDriver;
    pHalInitPowerManagement         HalInitPowerManagement;

    pHalGetDmaAdapter               HalGetDmaAdapter;
    pHalGetInterruptTranslator      HalGetInterruptTranslator;

    pHalStartMirroring              HalStartMirroring;
    pHalEndMirroring                HalEndMirroring;
    pHalMirrorPhysicalMemory        HalMirrorPhysicalMemory;
    pHalEndOfBoot                   HalEndOfBoot;
    pHalMirrorVerify                HalMirrorVerify;

    pHalGetAcpiTable                HalGetCachedAcpiTable;
    pHalSetPciErrorHandlerCallback  HalSetPciErrorHandlerCallback;

} HAL_DISPATCH, *PHAL_DISPATCH;



#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_)

extern  PHAL_DISPATCH   HalDispatchTable;
#define HALDISPATCH     HalDispatchTable

#else

extern  HAL_DISPATCH    HalDispatchTable;
#define HALDISPATCH     (&HalDispatchTable)

#endif

#define HAL_DISPATCH_VERSION        4

#define HalDispatchTableVersion         HALDISPATCH->Version
#define HalQuerySystemInformation       HALDISPATCH->HalQuerySystemInformation
#define HalSetSystemInformation         HALDISPATCH->HalSetSystemInformation
#define HalQueryBusSlots                HALDISPATCH->HalQueryBusSlots

#define HalReferenceHandlerForBus       HALDISPATCH->HalReferenceHandlerForBus
#define HalReferenceBusHandler          HALDISPATCH->HalReferenceBusHandler
#define HalDereferenceBusHandler        HALDISPATCH->HalDereferenceBusHandler

#define HalInitPnpDriver                HALDISPATCH->HalInitPnpDriver
#define HalInitPowerManagement          HALDISPATCH->HalInitPowerManagement

#define HalGetDmaAdapter                HALDISPATCH->HalGetDmaAdapter
#define HalGetInterruptTranslator       HALDISPATCH->HalGetInterruptTranslator

#define HalStartMirroring               HALDISPATCH->HalStartMirroring
#define HalEndMirroring                 HALDISPATCH->HalEndMirroring
#define HalMirrorPhysicalMemory         HALDISPATCH->HalMirrorPhysicalMemory
#define HalEndOfBoot                    HALDISPATCH->HalEndOfBoot
#define HalMirrorVerify                 HALDISPATCH->HalMirrorVerify

#define HalGetCachedAcpiTable           HALDISPATCH->HalGetCachedAcpiTable
#define HalSetPciErrorHandlerCallback   HALDISPATCH->HalSetPciErrorHandlerCallback


//
// HAL System Information Structures.
//

// for the information class "HalInstalledBusInformation"
typedef struct _HAL_BUS_INFORMATION{
    INTERFACE_TYPE  BusType;
    BUS_DATA_TYPE   ConfigurationType;
    ULONG           BusNumber;
    ULONG           Reserved;
} HAL_BUS_INFORMATION, *PHAL_BUS_INFORMATION;

// for the information class "HalDispayBiosInformation"
typedef enum _HAL_DISPLAY_BIOS_INFORMATION {
    HalDisplayInt10Bios,
    HalDisplayEmulatedBios,
    HalDisplayNoBios
} HAL_DISPLAY_BIOS_INFORMATION, *PHAL_DISPLAY_BIOS_INFORMATION;

// for the information class "HalPowerInformation"
typedef struct _HAL_POWER_INFORMATION {
    ULONG   TBD;
} HAL_POWER_INFORMATION, *PHAL_POWER_INFORMATION;

// for the information class "HalProcessorSpeedInformation"
typedef struct _HAL_PROCESSOR_SPEED_INFO {
    ULONG   ProcessorSpeed;
} HAL_PROCESSOR_SPEED_INFORMATION, *PHAL_PROCESSOR_SPEED_INFORMATION;

// for the information class "HalCallbackInformation"
typedef struct _HAL_CALLBACKS {
    PCALLBACK_OBJECT  SetSystemInformation;
    PCALLBACK_OBJECT  BusCheck;
} HAL_CALLBACKS, *PHAL_CALLBACKS;

// for the information class "HalProcessorFeatureInformation"
typedef struct _HAL_PROCESSOR_FEATURE {
    ULONG UsableFeatureBits;
} HAL_PROCESSOR_FEATURE;



typedef
NTSTATUS
(*PHALIOREADWRITEHANDLER)(
    _In_      BOOLEAN fRead,
    _In_      ULONG dwAddr,
    _In_      ULONG dwSize,
    _Inout_  PULONG pdwData
    );



// for the information class "HalQueryIllegalIOPortAddresses"
typedef struct _HAL_AMLI_BAD_IO_ADDRESS_LIST
{
    ULONG                   BadAddrBegin;
    ULONG                   BadAddrSize;
    ULONG                   OSVersionTrigger;
    PHALIOREADWRITEHANDLER  IOHandler;
} HAL_AMLI_BAD_IO_ADDRESS_LIST, *PHAL_AMLI_BAD_IO_ADDRESS_LIST;




//
// HalQueryMcaInterface
//

typedef
VOID
(*PHALMCAINTERFACELOCK)(
    VOID
    );

typedef
VOID
(*PHALMCAINTERFACEUNLOCK)(
    VOID
    );

typedef
NTSTATUS
(*PHALMCAINTERFACEREADREGISTER)(
    _In_     UCHAR    BankNumber,
    _Inout_ PVOID    Exception
    );


typedef struct _HAL_MCA_INTERFACE {
    PHALMCAINTERFACELOCK            Lock;
    PHALMCAINTERFACEUNLOCK          Unlock;
    PHALMCAINTERFACEREADREGISTER    ReadRegister;
} HAL_MCA_INTERFACE;

typedef enum {
    ApicDestinationModePhysical = 1,
    ApicDestinationModeLogicalFlat,
    ApicDestinationModeLogicalClustered,
    ApicDestinationModeUnknown
} HAL_APIC_DESTINATION_MODE, *PHAL_APIC_DESTINATION_MODE;


#if defined(_AMD64_)

struct _KTRAP_FRAME;
struct _KEXCEPTION_FRAME;

typedef
ERROR_SEVERITY
(*PDRIVER_EXCPTN_CALLBACK) (
    _In_ PVOID Context,
    _In_ struct _KTRAP_FRAME *TrapFrame,
    _In_ struct _KEXCEPTION_FRAME *ExceptionFrame,
    _In_ PMCA_EXCEPTION Exception
);

#endif

#if defined(_ARM_) || defined(_ARM64_)

struct _KTRAP_FRAME;
struct _KEXCEPTION_FRAME;

typedef PVOID PCMC_EXCEPTION;
typedef PVOID PCPE_EXCEPTION;

typedef
VOID
(*PDRIVER_EXCPTN_CALLBACK) (
    _In_ PVOID Context,
    _In_ struct _KTRAP_FRAME *TrapFrame,
    _In_ struct _KEXCEPTION_FRAME *ExceptionFrame,
    _In_ PVOID Exception
);

#endif

#if defined(_X86_)

typedef
VOID
(*PDRIVER_EXCPTN_CALLBACK) (
    _In_ PVOID Context,
    _In_ PMCA_EXCEPTION BankLog
);

#endif

typedef PDRIVER_EXCPTN_CALLBACK  PDRIVER_MCA_EXCEPTION_CALLBACK;


//
// Structure to record the callbacks from driver
//

typedef struct _MCA_DRIVER_INFO {
    PDRIVER_MCA_EXCEPTION_CALLBACK ExceptionCallback;
    PKDEFERRED_ROUTINE             DpcCallback;
    PVOID                          DeviceContext;
} MCA_DRIVER_INFO, *PMCA_DRIVER_INFO;



typedef struct _HAL_ERROR_INFO {
    ULONG     Version;                 // Version of this structure
    ULONG     InitMaxSize;             // Maximum size of the INIT record.
    ULONG     McaMaxSize;              // Maximum size of a Machine Check Abort record
    ULONG     McaPreviousEventsCount;  // Flag indicating previous or early-boot MCA event logs.
    ULONG     McaCorrectedEventsCount; // Number of corrected MCA events since boot.      approx.
    ULONG     McaKernelDeliveryFails;  // Number of Kernel callback failures.             approx.
    ULONG     McaDriverDpcQueueFails;  // Number of OEM MCA Driver Dpc queuing failures. approx.
    ULONG     McaReserved;
    ULONG     CmcMaxSize;              // Maximum size of a Corrected Machine  Check record
    ULONG     CmcPollingInterval;      // In units of seconds
    ULONG     CmcInterruptsCount;      // Number of CMC interrupts.                       approx.
    ULONG     CmcKernelDeliveryFails;  // Number of Kernel callback failures.             approx.
    ULONG     CmcDriverDpcQueueFails;  // Number of OEM CMC Driver Dpc queuing failures. approx.
    ULONG     CmcGetStateFails;        // Number of failures in getting  the log from FW.
    ULONG     CmcClearStateFails;      // Number of failures in clearing the log from FW.
    ULONG     CmcReserved;
    ULONGLONG CmcLogId;                // Last seen record identifier.
    ULONG     CpeMaxSize;              // Maximum size of a Corrected Platform Event record
    ULONG     CpePollingInterval;      // In units of seconds
    ULONG     CpeInterruptsCount;      // Number of CPE interrupts.                       approx.
    ULONG     CpeKernelDeliveryFails;  // Number of Kernel callback failures.             approx.
    ULONG     CpeDriverDpcQueueFails;  // Number of OEM CPE Driver Dpc queuing failures. approx.
    ULONG     CpeGetStateFails;        // Number of failures in getting  the log from FW.
    ULONG     CpeClearStateFails;      // Number of failures in clearing the log from FW.
    ULONG     CpeInterruptSources;     // Number of SAPIC Platform Interrupt Sources
    ULONGLONG CpeLogId;                // Last seen record identifier.
    ULONGLONG KernelReserved[4];
} HAL_ERROR_INFO, *PHAL_ERROR_INFO;



#define HAL_MCE_INTERRUPTS_BASED ((ULONG)-1)
#define HAL_MCE_DISABLED          ((ULONG)0)

//
// Known values for HAL_ERROR_INFO.CmcPollingInterval.
//

#define HAL_CMC_INTERRUPTS_BASED  HAL_MCE_INTERRUPTS_BASED
#define HAL_CMC_DISABLED          HAL_MCE_DISABLED

//
// Known values for HAL_ERROR_INFO.CpePollingInterval.
//

#define HAL_CPE_INTERRUPTS_BASED  HAL_MCE_INTERRUPTS_BASED
#define HAL_CPE_DISABLED          HAL_MCE_DISABLED

#define HAL_MCA_INTERRUPTS_BASED  HAL_MCE_INTERRUPTS_BASED
#define HAL_MCA_DISABLED          HAL_MCE_DISABLED



//
// Driver Callback type for the information class "HalCmcRegisterDriver"
//

typedef
VOID
(*PDRIVER_CMC_EXCEPTION_CALLBACK) (
    _In_ PVOID            Context,
    _In_ PCMC_EXCEPTION   CmcLog
);

//
// Driver Callback type for the information class "HalCpeRegisterDriver"
//

typedef
VOID
(*PDRIVER_CPE_EXCEPTION_CALLBACK) (
    _In_ PVOID            Context,
    _In_ PCPE_EXCEPTION   CmcLog
);


//
//
// Structure to record the callbacks from driver
//

typedef struct _CMC_DRIVER_INFO {
    PDRIVER_CMC_EXCEPTION_CALLBACK ExceptionCallback;
    PKDEFERRED_ROUTINE             DpcCallback;
    PVOID                          DeviceContext;
} CMC_DRIVER_INFO, *PCMC_DRIVER_INFO;

typedef struct _CPE_DRIVER_INFO {
    PDRIVER_CPE_EXCEPTION_CALLBACK ExceptionCallback;
    PKDEFERRED_ROUTINE             DpcCallback;
    PVOID                          DeviceContext;
} CPE_DRIVER_INFO, *PCPE_DRIVER_INFO;

typedef struct _HAL_PLATFORM_INFORMATION {
    ULONG PlatformFlags;
} HAL_PLATFORM_INFORMATION, *PHAL_PLATFORM_INFORMATION;



//
// Non-deprecated platform flags are carried over from the IPPT table
// definition if appropriate.
//

#define HAL_PLATFORM_DISABLE_WRITE_COMBINING      0x01L     // Deprecated
#define HAL_PLATFORM_DISABLE_PTCG                 0x04L     // Deprecated
#define HAL_PLATFORM_DISABLE_UC_MAIN_MEMORY       0x08L     // Deprecated
#define HAL_PLATFORM_ENABLE_WRITE_COMBINING_MMIO  0x10L     // Deprecated
#define HAL_PLATFORM_ACPI_TABLES_CACHED           0x20L



#if defined(_WIN64) || defined(_ARM_)

//
// Use __inline DMA macros (hal.h)
//
#ifndef USE_DMA_MACROS
#define USE_DMA_MACROS
#endif

//
// Only PnP drivers!
//
#ifndef NO_LEGACY_DRIVERS
#define NO_LEGACY_DRIVERS
#endif

#endif // _WIN64

#if defined(USE_DMA_MACROS) && !defined(_NTHAL_) && (defined(_NTDDK_) || defined(_NTDRIVER_)) || defined(_WDM_INCLUDED_) 

#else

//
// DMA adapter object functions.
//
#if (NTDDI_VERSION >= NTDDI_WIN2K)
DECLSPEC_DEPRECATED_DDK                 // Use AllocateAdapterChannel
NTHALAPI
NTSTATUS
HalAllocateAdapterChannel(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PWAIT_CONTEXT_BLOCK Wcb,
    _In_ ULONG NumberOfMapRegisters,
    _In_ PDRIVER_CONTROL ExecutionRoutine
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
DECLSPEC_DEPRECATED_DDK                 // Use AllocateCommonBuffer
__drv_preferredFunction("AllocateCommonBuffer","Obsolete")
NTHALAPI
PVOID
HalAllocateCommonBuffer(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ ULONG Length,
    _Out_ PPHYSICAL_ADDRESS LogicalAddress,
    _In_ BOOLEAN CacheEnabled
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
DECLSPEC_DEPRECATED_DDK                 // Use FreeCommonBuffer
__drv_preferredFunction("FreeCommonBuffer","Obsolete")
NTHALAPI
VOID
HalFreeCommonBuffer(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ ULONG Length,
    _In_ PHYSICAL_ADDRESS LogicalAddress,
    _In_ PVOID VirtualAddress,
    _In_ BOOLEAN CacheEnabled
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
DECLSPEC_DEPRECATED_DDK                 // Use ReadDmaCounter
__drv_preferredFunction("ReadDmaCounter","Obsolete")
NTHALAPI
ULONG
HalReadDmaCounter(
    _In_ PADAPTER_OBJECT AdapterObject
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
DECLSPEC_DEPRECATED_DDK                 // Use FlushAdapterBuffers
__drv_preferredFunction("FlushAdapterBuffers","Obsolete")
NTHALAPI
BOOLEAN
IoFlushAdapterBuffers(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PMDL Mdl,
    _In_ PVOID MapRegisterBase,
    _In_ PVOID CurrentVa,
    _In_ ULONG Length,
    _In_ BOOLEAN WriteToDevice
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
DECLSPEC_DEPRECATED_DDK                 // Use FreeAdapterChannel
__drv_preferredFunction("FreeAdapterChannel","Obsolete")
NTHALAPI
VOID
IoFreeAdapterChannel(
    _In_ PADAPTER_OBJECT AdapterObject
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
DECLSPEC_DEPRECATED_DDK                 // Use FreeMapRegisters
__drv_preferredFunction("FreeMapRegisters","Obsolete")
NTHALAPI
VOID
IoFreeMapRegisters(
   _In_ PADAPTER_OBJECT AdapterObject,
   _In_ PVOID MapRegisterBase,
   _In_ ULONG NumberOfMapRegisters
   );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
DECLSPEC_DEPRECATED_DDK                 // Use MapTransfer
__drv_preferredFunction("MapTransfer","Obsolete")
NTHALAPI
PHYSICAL_ADDRESS
IoMapTransfer(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PMDL Mdl,
    _In_ PVOID MapRegisterBase,
    _In_ PVOID CurrentVa,
    _Inout_ PULONG Length,
    _In_ BOOLEAN WriteToDevice
    );
#endif

#endif // USE_DMA_MACROS && (_NTDDK_ || _NTDRIVER_)



#if (NTDDI_VERSION >= NTDDI_WIN2K)
DECLSPEC_DEPRECATED_DDK
NTSTATUS
HalGetScatterGatherList (               // Use GetScatterGatherList
    _In_ PADAPTER_OBJECT DmaAdapter,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PMDL Mdl,
    _In_ PVOID CurrentVa,
    _In_ ULONG Length,
    _In_ PDRIVER_LIST_CONTROL ExecutionRoutine,
    _In_ PVOID Context,
    _In_ BOOLEAN WriteToDevice
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
DECLSPEC_DEPRECATED_DDK                 // Use PutScatterGatherList
VOID
HalPutScatterGatherList (
    _In_ PADAPTER_OBJECT DmaAdapter,
    _In_ PSCATTER_GATHER_LIST ScatterGather,
    _In_ BOOLEAN WriteToDevice
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
DECLSPEC_DEPRECATED_DDK                 // Use PutDmaAdapter
VOID
HalPutDmaAdapter(
    _In_ PADAPTER_OBJECT DmaAdapter
    );
#endif

#if !defined(XBOX_SYSTEMOS)

#if (NTDDI_VERSION >= NTDDI_WIN7)

typedef struct _WHEA_ERROR_SOURCE_DESCRIPTOR *PWHEA_ERROR_SOURCE_DESCRIPTOR;
typedef struct _WHEA_ERROR_RECORD *PWHEA_ERROR_RECORD;

NTHALAPI
VOID
HalBugCheckSystem (
    _In_ PWHEA_ERROR_SOURCE_DESCRIPTOR ErrorSource,
    _In_ PWHEA_ERROR_RECORD ErrorRecord
    );

#elif (NTDDI_VERSION >= NTDDI_VISTA)

typedef struct _WHEA_ERROR_RECORD *PWHEA_ERROR_RECORD;

NTHALAPI
VOID
HalBugCheckSystem (
    _In_ PWHEA_ERROR_RECORD ErrorRecord
    );

#endif

#endif


typedef enum _PHYSICAL_COUNTER_RESOURCE_DESCRIPTOR_TYPE {
    ResourceTypeSingle = 0,
    ResourceTypeRange,
    ResourceTypeExtendedCounterConfiguration,
    ResourceTypeOverflow,
    ResourceTypeMax
} PHYSICAL_COUNTER_RESOURCE_DESCRIPTOR_TYPE;

/*++

Physical Counter Resource Descriptor Types:

    Describes the format of a physical counter resource.

    PhysicalCounterResourceTypeSingle - The descriptor specifies a single
        physical counter in the u.CounterIndex member.

    PhysicalCounterResourceTypeRange - The descriptor specifies a range of
        counter indices in the u.Range member.

    PhysicalCounterResourceTypeExtendedConfiguration - The descriptor specifies
        an extended counter configuration register address in the
        u.ExtendedRegisterAddress member. Only used on Intel NetBurst systems.

    PhysicalCounterResourceTypeOverflow - The descriptor specifies a counter
        overflow interrupt.

--*/

typedef struct _PHYSICAL_COUNTER_RESOURCE_DESCRIPTOR {
    PHYSICAL_COUNTER_RESOURCE_DESCRIPTOR_TYPE Type;
    ULONG Flags;
    union {
        ULONG CounterIndex;
        ULONG ExtendedRegisterAddress;
        struct {
            ULONG Begin;
            ULONG End;
        } Range;
    } u;
} PHYSICAL_COUNTER_RESOURCE_DESCRIPTOR, *PPHYSICAL_COUNTER_RESOURCE_DESCRIPTOR;

/*++

Physical Counter Resource Descriptor:

    Data structure used to describe physical counter resources on the platform.

Fields:

    Type - Supplies the type of counter resource described.

    Flags - Reserved for future use.

    CounterIndex - Supplies a physical counter index.

    ExtendedRegisterAddress - Supplies an extended configuration register index.

    Range - Supplies a range of counter indices or register addresses.

--*/

typedef struct _PHYSICAL_COUNTER_RESOURCE_LIST {
    ULONG Count;
    PHYSICAL_COUNTER_RESOURCE_DESCRIPTOR Descriptors[ANYSIZE_ARRAY];
} PHYSICAL_COUNTER_RESOURCE_LIST, *PPHYSICAL_COUNTER_RESOURCE_LIST;

/*++

Physical Counter Resource List:

    Data structure used to report or request a set of physical counter
    resources on the platform.

Fields:

    Count - Supplies the number of physical counter resources in the list.

    Descriptors - Supplies the a variable length array of physical counter
        descriptors.

--*/

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTHALAPI
NTSTATUS
HalAllocateHardwareCounters (
    _In_reads_opt_(GroupCount) PGROUP_AFFINITY GroupAffinty,
    _In_ ULONG GroupCount,
    _In_opt_ PPHYSICAL_COUNTER_RESOURCE_LIST ResourceList,
    _Out_ PHANDLE CounterSetHandle
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTHALAPI
NTSTATUS
HalFreeHardwareCounters (
    _In_ HANDLE CounterSetHandle
    );
#endif

//
// Determine if there is a complete device failure on an error.
//

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_Must_inspect_result_
NTKERNELAPI
BOOLEAN
FsRtlIsTotalDeviceFailure(
    _In_ NTSTATUS Status
    );

#define FsRtlIsChecksumError(STS) ((BOOLEAN)!FsRtlIsTotalDeviceFailure(STS))
#endif



//
// AGP Capabilities
//
typedef struct _PCI_AGP_CAPABILITY {

    PCI_CAPABILITIES_HEADER Header;

    USHORT  Minor:4;
    USHORT  Major:4;
    USHORT  Rsvd1:8;

    struct _PCI_AGP_STATUS {
        ULONG   Rate:3;
        ULONG   Agp3Mode:1;
        ULONG   FastWrite:1;
        ULONG   FourGB:1;
        ULONG   HostTransDisable:1;
        ULONG   Gart64:1;
        ULONG   ITA_Coherent:1;
        ULONG   SideBandAddressing:1;                   // SBA
        ULONG   CalibrationCycle:3;
        ULONG   AsyncRequestSize:3;
        ULONG   Rsvd1:1;
        ULONG   Isoch:1;
        ULONG   Rsvd2:6;
        ULONG   RequestQueueDepthMaximum:8;             // RQ
    } AGPStatus;

    struct _PCI_AGP_COMMAND {
        ULONG   Rate:3;
        ULONG   Rsvd1:1;
        ULONG   FastWriteEnable:1;
        ULONG   FourGBEnable:1;
        ULONG   Rsvd2:1;
        ULONG   Gart64:1;
        ULONG   AGPEnable:1;
        ULONG   SBAEnable:1;
        ULONG   CalibrationCycle:3;
        ULONG   AsyncReqSize:3;
        ULONG   Rsvd3:8;
        ULONG   RequestQueueDepth:8;
    } AGPCommand;

} PCI_AGP_CAPABILITY, *PPCI_AGP_CAPABILITY;

//
// An AGPv3 Target must have an extended capability,
// but it's only present for a Master when the Isoch
// bit is set in its status register
//
typedef enum _EXTENDED_AGP_REGISTER {
    IsochStatus,
    AgpControl,
    ApertureSize,
    AperturePageSize,
    GartLow,
    GartHigh,
    IsochCommand
} EXTENDED_AGP_REGISTER, *PEXTENDED_AGP_REGISTER;

typedef struct _PCI_AGP_ISOCH_STATUS {
    ULONG ErrorCode: 2;
    ULONG Rsvd1: 1;
    ULONG Isoch_L: 3;
    ULONG Isoch_Y: 2;
    ULONG Isoch_N: 8;
    ULONG Rsvd2: 16;
} PCI_AGP_ISOCH_STATUS, *PPCI_AGP_ISOCH_STATUS;

typedef struct _PCI_AGP_CONTROL {
    ULONG Rsvd1: 7;
    ULONG GTLB_Enable: 1;
    ULONG AP_Enable: 1;
    ULONG CAL_Disable: 1;
    ULONG Rsvd2: 22;
} PCI_AGP_CONTROL, *PPCI_AGP_CONTROL;

typedef struct _PCI_AGP_APERTURE_PAGE_SIZE {
    USHORT PageSizeMask: 11;
    USHORT Rsvd1: 1;
    USHORT PageSizeSelect: 4;
} PCI_AGP_APERTURE_PAGE_SIZE, *PPCI_AGP_APERTURE_PAGE_SIZE;

typedef struct _PCI_AGP_ISOCH_COMMAND {
    USHORT Rsvd1: 6;
    USHORT Isoch_Y: 2;
    USHORT Isoch_N: 8;
} PCI_AGP_ISOCH_COMMAND, *PPCI_AGP_ISOCH_COMMAND;

typedef struct PCI_AGP_EXTENDED_CAPABILITY {

    PCI_AGP_ISOCH_STATUS IsochStatus;

//
// Target only ----------------<<-begin->>
//
    PCI_AGP_CONTROL AgpControl;
    USHORT ApertureSize;
    PCI_AGP_APERTURE_PAGE_SIZE AperturePageSize;
    ULONG GartLow;
    ULONG GartHigh;
//
// ------------------------------<<-end->>
//

    PCI_AGP_ISOCH_COMMAND IsochCommand;

} PCI_AGP_EXTENDED_CAPABILITY, *PPCI_AGP_EXTENDED_CAPABILITY;


#define PCI_AGP_RATE_1X     0x1
#define PCI_AGP_RATE_2X     0x2
#define PCI_AGP_RATE_4X     0x4


//
// PCI-X Bridge Capability
//

//
// Values for BusModeFrequency in the SecondaryStatus register
//
#define PCIX_MODE_CONVENTIONAL_PCI  0x0
#define PCIX_MODE1_66MHZ            0x1
#define PCIX_MODE1_100MHZ           0x2
#define PCIX_MODE1_133MHZ           0x3
#define PCIX_MODE2_266_66MHZ        0x9
#define PCIX_MODE2_266_100MHZ       0xA
#define PCIX_MODE2_266_133MHZ       0xB
#define PCIX_MODE2_533_66MHZ        0xD
#define PCIX_MODE2_533_100MHZ       0xE
#define PCIX_MODE2_533_133MHZ       0xF

//
// Values for the Version in the SecondaryStatus register
//
#define PCIX_VERSION_MODE1_ONLY     0x0
#define PCIX_VERSION_MODE2_ECC      0x1
#define PCIX_VERSION_DUAL_MODE_ECC  0x2

typedef struct _PCIX_BRIDGE_CAPABILITY {

    PCI_CAPABILITIES_HEADER Header;

    union {
        struct {
            USHORT Bus64Bit:1;
            USHORT Bus133MHzCapable:1;
            USHORT SplitCompletionDiscarded:1;
            USHORT UnexpectedSplitCompletion:1;
            USHORT SplitCompletionOverrun:1;
            USHORT SplitRequestDelayed:1;
            USHORT BusModeFrequency:4;  // PCIX_MODE_x
            USHORT Rsvd:2;
            USHORT Version:2;           // PCIX_VERSION_x
            USHORT Bus266MHzCapable:1;
            USHORT Bus533MHzCapable:1;
        } DUMMYSTRUCTNAME;
        USHORT AsUSHORT;
    } SecondaryStatus;

    union {
        struct {
            ULONG FunctionNumber:3;
            ULONG DeviceNumber:5;
            ULONG BusNumber:8;
            ULONG Device64Bit:1;
            ULONG Device133MHzCapable:1;
            ULONG SplitCompletionDiscarded:1;
            ULONG UnexpectedSplitCompletion:1;
            ULONG SplitCompletionOverrun:1;
            ULONG SplitRequestDelayed:1;
            ULONG Rsvd:7;
            ULONG DIMCapable:1;
            ULONG Device266MHzCapable:1;
            ULONG Device533MHzCapable:1;
        } DUMMYSTRUCTNAME;
        ULONG AsULONG;
    } BridgeStatus;

    USHORT UpstreamSplitTransactionCapacity;
    USHORT UpstreamSplitTransactionLimit;

    USHORT DownstreamSplitTransactionCapacity;
    USHORT DownstreamSplitTransactionLimit;

    union {
        struct {
            ULONG SelectSecondaryRegisters:1;
            ULONG ErrorPresentInOtherBank:1;
            ULONG AdditionalCorrectableError:1;
            ULONG AdditionalUncorrectableError:1;
            ULONG ErrorPhase:3;
            ULONG ErrorCorrected:1;
            ULONG Syndrome:8;
            ULONG ErrorFirstCommand:4;
            ULONG ErrorSecondCommand:4;
            ULONG ErrorUpperAttributes:4;
            ULONG ControlUpdateEnable:1;
            ULONG Rsvd:1;
            ULONG DisableSingleBitCorrection:1;
            ULONG EccMode:1;
        } DUMMYSTRUCTNAME;
        ULONG AsULONG;
    } EccControlStatus;

    ULONG EccFirstAddress;
    ULONG EccSecondAddress;
    ULONG EccAttribute;

} PCIX_BRIDGE_CAPABILITY, *PPCIX_BRIDGE_CAPABILITY;



//
// PCI to PCI Bridge Subsystem ID Capability
//
typedef struct _PCI_SUBSYSTEM_IDS_CAPABILITY {

    PCI_CAPABILITIES_HEADER Header;
    USHORT Reserved;
    USHORT SubVendorID;
    USHORT SubSystemID;

} PCI_SUBSYSTEM_IDS_CAPABILITY, *PPCI_SUBSYSTEM_IDS_CAPABILITY;

//
// PCI Advanced Features Capability
//

typedef struct _PCI_ADVANCED_FEATURES_CAPABILITY {

    PCI_CAPABILITIES_HEADER Header;
    UCHAR Length;

    union {
        struct {
            UCHAR FunctionLevelResetSupported:1;
            UCHAR TransactionsPendingSupported:1;
            UCHAR Rsvd:6;
        } DUMMYSTRUCTNAME;

        UCHAR AsUCHAR;
    } Capabilities;

    union {
        struct {
            UCHAR InitiateFunctionLevelReset:1;
            UCHAR Rsvd:7;
        } DUMMYSTRUCTNAME;

        UCHAR AsUCHAR;
    } Control;

    union {
        struct {
            UCHAR TransactionsPending:1;
            UCHAR Rsvd:7;
        } DUMMYSTRUCTNAME;

        UCHAR AsUCHAR;
    } Status;

} PCI_ADVANCED_FEATURES_CAPABILITY, *PPCI_ADVANCED_FEATURES_CAPABILITY;

//
// _OSC is used by OSPM to query the capabilities of a device and to
// communicate the features supported by the device driver to the platform.
// The _OSC interface for PCI host bridge devices that originate PCI, PCI-X or
// PCI Express hierarchies is identified by a UUID of {33db4d5b-1ff7-401c-9657-
// 7441c03dd766}. A revision ID of 1 indicates that the capabilities buffer is
// composed of 3 DWORDs.
// The first DWORD is common across all OSC implementations and includes status
// and error information.
// The second DWORD (Support Field) provides information regarding OS supported
// features.
// The third DWORD (Control Field) is used to submit request for control of
// associated features. If any bits in the control field are returned cleared,
// then the respective feature is unsupported by the platform and must not be
// enabled.
// According to the PCI Firmware Specification a machine with multiple host
// bridge devices should report the same capabilities for all host bridges
// and also negotiate control of the features in the same way.
//

#define OSC_FIRMWARE_FAILURE                            0x02
#define OSC_UNRECOGNIZED_UUID                           0x04
#define OSC_UNRECOGNIZED_REVISION                       0x08
#define OSC_CAPABILITIES_MASKED                         0x10

#define PCI_ROOT_BUS_OSC_METHOD_CAPABILITY_REVISION     0x01

//
// The following declarations pertain to the second and third DWORD in
// evaluation of _OSC for PCI host bridge devices.
//

typedef struct _PCI_ROOT_BUS_OSC_SUPPORT_FIELD {
    union {
        struct {
            ULONG ExtendedConfigOpRegions:1;
            ULONG ActiveStatePowerManagement:1;
            ULONG ClockPowerManagement:1;
            ULONG SegmentGroups:1;
            ULONG MessageSignaledInterrupts:1;
            ULONG OptimizedBufferFlushAndFill:1;
            ULONG AspmOptionality:1;
            ULONG Reserved:25;
        } DUMMYSTRUCTNAME;
        ULONG AsULONG;
    } u;
} PCI_ROOT_BUS_OSC_SUPPORT_FIELD, *PPCI_ROOT_BUS_OSC_SUPPORT_FIELD;

typedef struct _PCI_ROOT_BUS_OSC_CONTROL_FIELD {
    union {
        struct {
            ULONG ExpressNativeHotPlug:1;
            ULONG ShpcNativeHotPlug:1;
            ULONG ExpressNativePME:1;
            ULONG ExpressAdvancedErrorReporting:1;
            ULONG ExpressCapabilityStructure:1;
            ULONG LatencyToleranceReporting:1;
            ULONG Reserved:26;
        } DUMMYSTRUCTNAME;
        ULONG AsULONG;
    } u;
} PCI_ROOT_BUS_OSC_CONTROL_FIELD, *PPCI_ROOT_BUS_OSC_CONTROL_FIELD;

//
// The following comes from the PCI Firmware Specification, version 3.1.  It
// defines bus capabilities as reported by an ACPI BIOS.
//

typedef struct _PCI_FIRMWARE_BUS_CAPS {
    USHORT Type;
    USHORT Length; // must be 16
    struct {
        UCHAR SixtyFourBitDevice:1;
        UCHAR PciXMode1EccCapable:1;
        UCHAR DeviceIdMessagingCapable:1;
        UCHAR ObffWakeSignalCapable:1;
        UCHAR Reserved1:4;
    } DUMMYSTRUCTNAME;
    UCHAR CurrentSpeedAndMode;
    UCHAR SupportedSpeedsAndModesLowByte;
    UCHAR SupportedSpeedsAndModesHighByte;
    UCHAR Voltage;
    UCHAR Reserved2[7];
} PCI_FIRMWARE_BUS_CAPS, *PPCI_FIRMWARE_BUS_CAPS;

typedef struct _PCI_FIRMWARE_BUS_CAPS_RETURN_BUFFER {
    USHORT Version;
    USHORT Status;
    ULONG  Length;
    PCI_FIRMWARE_BUS_CAPS Caps;
} PCI_FIRMWARE_BUS_CAPS_RETURN_BUFFER, *PPCI_FIRMWARE_BUS_CAPS_RETURN_BUFFER;

//
// An enumerator for the PCI physical and electrical interface.
//



typedef enum _PCI_HARDWARE_INTERFACE {

    PciConventional,
    PciXMode1,
    PciXMode2,
    PciExpress

} PCI_HARDWARE_INTERFACE, *PPCI_HARDWARE_INTERFACE;



typedef enum {

    BusWidth32Bits,
    BusWidth64Bits

} PCI_BUS_WIDTH;

typedef struct _PCI_ROOT_BUS_HARDWARE_CAPABILITY {

    //
    // Describes the secondary side of a PCI root bus.
    //

    PCI_HARDWARE_INTERFACE SecondaryInterface;

    //
    // These additional capabilities are available when each of the following
    // is true.
    // 1. The secondary side of a PCI root bus operates in conventional or
    //    PCI-X mode.
    // 2. The PCI root bus has a hardware ID or compatible ID of PNP0A03.
    // 3. A _DSM function 4 is defined for the root bus.
    //

    struct {

        //
        // This boolean indicates if the remaining fields describing the bus
        // capabilities are valid or not.
        //

        BOOLEAN BusCapabilitiesFound;


        //
        // Provides information on current and supported speeds/modes.
        //

        ULONG CurrentSpeedAndMode;
        ULONG SupportedSpeedsAndModes;

        //
        // Describes the root bus capability on forwarding of Device ID message
        // transactions.
        //

        BOOLEAN DeviceIDMessagingCapable;

        //
        // Provides the width for a PCI interface.
        //

        PCI_BUS_WIDTH SecondaryBusWidth;
    } DUMMYSTRUCTNAME;

    //
    // Fields describing features supported as well as control for them from
    // the bios.
    //

    PCI_ROOT_BUS_OSC_SUPPORT_FIELD OscFeatureSupport;
    PCI_ROOT_BUS_OSC_CONTROL_FIELD OscControlRequest;
    PCI_ROOT_BUS_OSC_CONTROL_FIELD OscControlGranted;

} PCI_ROOT_BUS_HARDWARE_CAPABILITY, *PPCI_ROOT_BUS_HARDWARE_CAPABILITY;

//
// PCI Express Capability
//



typedef union _PCI_EXPRESS_CAPABILITIES_REGISTER {

    struct {

        USHORT CapabilityVersion:4;
        USHORT DeviceType:4;               // PCI_EXPRESS_DEVICE_TYPE
        USHORT SlotImplemented:1;
        USHORT InterruptMessageNumber:5;
        USHORT Rsvd:2;
    } DUMMYSTRUCTNAME;

    USHORT AsUSHORT;

} PCI_EXPRESS_CAPABILITIES_REGISTER, *PPCI_EXPRESS_CAPABILITIES_REGISTER;

typedef union _PCI_EXPRESS_DEVICE_CAPABILITIES_REGISTER {

    struct {

        ULONG MaxPayloadSizeSupported:3;     // EXPRESS_MAX_PAYLOAD_SIZE
        ULONG PhantomFunctionsSupported:2;
        ULONG ExtendedTagSupported:1;
        ULONG L0sAcceptableLatency:3;        // EXPRESS_L0S_LATENCY
        ULONG L1AcceptableLatency:3;         // EXPRESS_L1_LATENCY
        ULONG Undefined:3;
        ULONG RoleBasedErrorReporting:1;
        ULONG Rsvd1:2;
        ULONG CapturedSlotPowerLimit:8;
        ULONG CapturedSlotPowerLimitScale:2;
        ULONG FunctionLevelResetCapability:1;
        ULONG Rsvd2:3;
    } DUMMYSTRUCTNAME;

    ULONG AsULONG;

} PCI_EXPRESS_DEVICE_CAPABILITIES_REGISTER, *PPCI_EXPRESS_DEVICE_CAPABILITIES_REGISTER;

//
// The low 3 bits of the PCI Express device control register dictate whether
// a device that implements AER routes error messages to the root complex.
// This mask is used when programming the AER bits in the device control
// register.
//

#define PCI_EXPRESS_AER_DEVICE_CONTROL_MASK 0x07;

typedef union _PCI_EXPRESS_DEVICE_CONTROL_REGISTER {

    struct {

        USHORT CorrectableErrorEnable:1;
        USHORT NonFatalErrorEnable:1;
        USHORT FatalErrorEnable:1;
        USHORT UnsupportedRequestErrorEnable:1;
        USHORT EnableRelaxedOrder:1;
        USHORT MaxPayloadSize:3;                 // EXPRESS_MAX_PAYLOAD_SIZE
        USHORT ExtendedTagEnable:1;
        USHORT PhantomFunctionsEnable:1;
        USHORT AuxPowerEnable:1;
        USHORT NoSnoopEnable:1;
        USHORT MaxReadRequestSize:3;             // EXPRESS_MAX_PAYLOAD_SIZE
        USHORT BridgeConfigRetryEnable:1;
    } DUMMYSTRUCTNAME;

    //
    // Hack to allow alias of BridgeConfigRetryEnable bit for FLR.
    //

    struct {
        USHORT :15;
        USHORT InitiateFunctionLevelReset:1;
    } DUMMYSTRUCTNAME2;

    USHORT AsUSHORT;

} PCI_EXPRESS_DEVICE_CONTROL_REGISTER, *PPCI_EXPRESS_DEVICE_CONTROL_REGISTER;

//
// The low 4 bits of the PCI Express device status register hold AER device
// status. This mask is used when programming the AER bits in the device status
// register.
//

#define PCI_EXPRESS_AER_DEVICE_STATUS_MASK 0x0F;

typedef union _PCI_EXPRESS_DEVICE_STATUS_REGISTER {

    struct {

        USHORT CorrectableErrorDetected:1;
        USHORT NonFatalErrorDetected:1;
        USHORT FatalErrorDetected:1;
        USHORT UnsupportedRequestDetected:1;
        USHORT AuxPowerDetected:1;
        USHORT TransactionsPending:1;
        USHORT Rsvd:10;
    } DUMMYSTRUCTNAME;

    USHORT AsUSHORT;

} PCI_EXPRESS_DEVICE_STATUS_REGISTER, *PPCI_EXPRESS_DEVICE_STATUS_REGISTER;

typedef union _PCI_EXPRESS_LINK_CAPABILITIES_REGISTER {

    struct {

        ULONG MaximumLinkSpeed:4;
        ULONG MaximumLinkWidth:6;
        ULONG ActiveStatePMSupport:2;   // EXPRESS_ASPM_CONFIG
        ULONG L0sExitLatency:3;         // EXPRESS_L0S_LATENCY
        ULONG L1ExitLatency:3;          // EXPRESS_L1_LATENCY
        ULONG ClockPowerManagement:1;
        ULONG SurpriseDownErrorReportingCapable:1;
        ULONG DataLinkLayerActiveReportingCapable:1;
        ULONG LinkBandwidthNotificationCapability:1;
        ULONG AspmOptionalityCompliance:1;
        ULONG Rsvd:1;
        ULONG PortNumber:8;
    } DUMMYSTRUCTNAME;

    ULONG AsULONG;

} PCI_EXPRESS_LINK_CAPABILITIES_REGISTER, *PPCI_EXPRESS_LINK_CAPABILITIES_REGISTER;

typedef union _PCI_EXPRESS_LINK_CONTROL_REGISTER {

    struct {

        USHORT ActiveStatePMControl:2;    // EXPRESS_ASPM_CONFIG
        USHORT Rsvd1:1;
        USHORT ReadCompletionBoundary:1;  // EXPRESS_RCB
        USHORT LinkDisable:1;
        USHORT RetrainLink:1;
        USHORT CommonClockConfig:1;
        USHORT ExtendedSynch:1;
        USHORT EnableClockPowerManagement:1;
        USHORT Rsvd2:7;
    } DUMMYSTRUCTNAME;

    USHORT AsUSHORT;

} PCI_EXPRESS_LINK_CONTROL_REGISTER, *PPCI_EXPRESS_LINK_CONTROL_REGISTER;

typedef union _PCI_EXPRESS_LINK_STATUS_REGISTER {

    struct {

        USHORT LinkSpeed:4;
        USHORT LinkWidth:6;
        USHORT Undefined:1;
        USHORT LinkTraining:1;
        USHORT SlotClockConfig:1;
        USHORT DataLinkLayerActive:1;
        USHORT Rsvd:2;
    } DUMMYSTRUCTNAME;

    USHORT AsUSHORT;

} PCI_EXPRESS_LINK_STATUS_REGISTER, *PPCI_EXPRESS_LINK_STATUS_REGISTER;

typedef union _PCI_EXPRESS_SLOT_CAPABILITIES_REGISTER {

    struct {

        ULONG AttentionButtonPresent:1;
        ULONG PowerControllerPresent:1;
        ULONG MRLSensorPresent:1;
        ULONG AttentionIndicatorPresent:1;
        ULONG PowerIndicatorPresent:1;
        ULONG HotPlugSurprise:1;
        ULONG HotPlugCapable:1;
        ULONG SlotPowerLimit:8;
        ULONG SlotPowerLimitScale:2;
        ULONG ElectromechanicalLockPresent:1;
        ULONG NoCommandCompletedSupport:1;
        ULONG PhysicalSlotNumber:13;
    } DUMMYSTRUCTNAME;

    ULONG AsULONG;

} PCI_EXPRESS_SLOT_CAPABILITIES_REGISTER, *PPCI_EXPRESS_SLOT_CAPABILITIES_REGISTER;

typedef union _PCI_EXPRESS_SLOT_CONTROL_REGISTER {

    struct {

        USHORT AttentionButtonEnable:1;
        USHORT PowerFaultDetectEnable:1;
        USHORT MRLSensorEnable:1;
        USHORT PresenceDetectEnable:1;
        USHORT CommandCompletedEnable:1;
        USHORT HotPlugInterruptEnable:1;
        USHORT AttentionIndicatorControl:2;  // EXPRESS_INDICATOR_STATE
        USHORT PowerIndicatorControl:2;      // EXPRESS_INDICATOR_STATE
        USHORT PowerControllerControl:1;     // EXPRESS_POWER_STATE
        USHORT ElectromechanicalLockControl:1;
        USHORT DataLinkStateChangeEnable:1;
        USHORT Rsvd:3;
    } DUMMYSTRUCTNAME;

    USHORT AsUSHORT;

} PCI_EXPRESS_SLOT_CONTROL_REGISTER, *PPCI_EXPRESS_SLOT_CONTROL_REGISTER;

typedef union _PCI_EXPRESS_SLOT_STATUS_REGISTER {

    struct {

        USHORT AttentionButtonPressed:1;
        USHORT PowerFaultDetected:1;
        USHORT MRLSensorChanged:1;
        USHORT PresenceDetectChanged:1;
        USHORT CommandCompleted:1;
        USHORT MRLSensorState:1;        // EXPRESS_MRL_STATE
        USHORT PresenceDetectState:1;   // EXPRESS_CARD_PRESENCE
        USHORT ElectromechanicalLockEngaged:1;
        USHORT DataLinkStateChanged:1;
        USHORT Rsvd:7;
    } DUMMYSTRUCTNAME;

    USHORT AsUSHORT;

} PCI_EXPRESS_SLOT_STATUS_REGISTER, *PPCI_EXPRESS_SLOT_STATUS_REGISTER;

typedef union _PCI_EXPRESS_ROOT_CONTROL_REGISTER {

    struct {

        USHORT CorrectableSerrEnable:1;
        USHORT NonFatalSerrEnable:1;
        USHORT FatalSerrEnable:1;
        USHORT PMEInterruptEnable:1;
        USHORT CRSSoftwareVisibilityEnable:1;
        USHORT Rsvd:11;
    } DUMMYSTRUCTNAME;

    USHORT AsUSHORT;

} PCI_EXPRESS_ROOT_CONTROL_REGISTER, *PPCI_EXPRESS_ROOT_CONTROL_REGISTER;

typedef union _PCI_EXPRESS_ROOT_CAPABILITIES_REGISTER {

    struct {

        USHORT CRSSoftwareVisibility:1;
        USHORT Rsvd:15;
    } DUMMYSTRUCTNAME;

    USHORT AsUSHORT;

} PCI_EXPRESS_ROOT_CAPABILITIES_REGISTER, *PPCI_EXPRESS_ROOT_CAPABILITIES_REGISTER;

typedef union _PCI_EXPRESS_ROOT_STATUS_REGISTER {

    struct {

        ULONG PMERequestorId:16;  // PCI_EXPRESS_REQUESTOR_ID
        ULONG PMEStatus:1;
        ULONG PMEPending:1;
        ULONG Rsvd:14;
    } DUMMYSTRUCTNAME;

    ULONG AsULONG;

} PCI_EXPRESS_ROOT_STATUS_REGISTER, *PPCI_EXPRESS_ROOT_STATUS_REGISTER;

typedef union _PCI_EXPRESS_DEVICE_CAPABILITIES_2_REGISTER {

    struct {

        ULONG CompletionTimeoutRangesSupported:4;
        ULONG CompletionTimeoutDisableSupported:1;
        ULONG AriForwardingSupported:1;
        ULONG AtomicOpRoutingSupported:1;
        ULONG AtomicOpCompleterSupported32Bit:1;
        ULONG AtomicOpCompleterSupported64Bit:1;
        ULONG CASCompleterSupported128Bit:1;
        ULONG NoROEnabledPRPRPassing:1;
        ULONG LTRMechanismSupported:1;
        ULONG TPHCompleterSupported:2;
        ULONG Rsvd:4;
        ULONG OBFFSupported:2;
        ULONG ExtendedFmtFieldSuported:1;
        ULONG EndEndTLPPrefixSupported:1;
        ULONG MaxEndEndTLPPrefixes:2;
        ULONG Rsvd2:8;
    } DUMMYSTRUCTNAME;

    ULONG AsULONG;

} PCI_EXPRESS_DEVICE_CAPABILITIES_2_REGISTER, *PPCI_EXPRESS_DEVICE_CAPABILITIES_2_REGISTER;

typedef union _PCI_EXPRESS_DEVICE_CONTROL_2_REGISTER {

    struct {

        USHORT CompletionTimeoutValue:4;
        USHORT CompletionTimeoutDisable:1;
        USHORT AriForwardingEnable:1;
        USHORT AtomicOpRequesterEnable:1;
        USHORT AtomicOpEgresBlocking:1;
        USHORT IDORequestEnable:1;
        USHORT IDOCompletionEnable:1;
        USHORT LTRMechanismEnable:1;
        USHORT Rsvd:2;
        USHORT OBFFEnable:2;
        USHORT EndEndTLPPrefixBlocking:1;
    } DUMMYSTRUCTNAME;

    USHORT AsUSHORT;

} PCI_EXPRESS_DEVICE_CONTROL_2_REGISTER, *PPCI_EXPRESS_DEVICE_CONTROL_2_REGISTER;

typedef union _PCI_EXPRESS_DEVICE_STATUS_2_REGISTER {

    struct {

        USHORT Rsvd:16;
    } DUMMYSTRUCTNAME;

    USHORT AsUSHORT;

} PCI_EXPRESS_DEVICE_STATUS_2_REGISTER, *PPCI_EXPRESS_DEVICE_STATUS_2_REGISTER;

//
// PCI Express Capability
//

typedef struct _PCI_EXPRESS_CAPABILITY {

    PCI_CAPABILITIES_HEADER Header;
    PCI_EXPRESS_CAPABILITIES_REGISTER ExpressCapabilities;

    PCI_EXPRESS_DEVICE_CAPABILITIES_REGISTER DeviceCapabilities;

    PCI_EXPRESS_DEVICE_CONTROL_REGISTER DeviceControl;
    PCI_EXPRESS_DEVICE_STATUS_REGISTER DeviceStatus;

    PCI_EXPRESS_LINK_CAPABILITIES_REGISTER LinkCapabilities;

    PCI_EXPRESS_LINK_CONTROL_REGISTER LinkControl;
    PCI_EXPRESS_LINK_STATUS_REGISTER LinkStatus;

    PCI_EXPRESS_SLOT_CAPABILITIES_REGISTER SlotCapabilities;

    PCI_EXPRESS_SLOT_CONTROL_REGISTER SlotControl;
    PCI_EXPRESS_SLOT_STATUS_REGISTER SlotStatus;

    PCI_EXPRESS_ROOT_CONTROL_REGISTER RootControl;
    PCI_EXPRESS_ROOT_CAPABILITIES_REGISTER RootCapabilities;

    PCI_EXPRESS_ROOT_STATUS_REGISTER RootStatus;

    PCI_EXPRESS_DEVICE_CAPABILITIES_2_REGISTER DeviceCapabilities2;
    PCI_EXPRESS_DEVICE_CONTROL_2_REGISTER DeviceControl2;
    PCI_EXPRESS_DEVICE_STATUS_2_REGISTER DeviceStatus2;

} PCI_EXPRESS_CAPABILITY, *PPCI_EXPRESS_CAPABILITY;



typedef enum {

    MRLClosed = 0,
    MRLOpen

} PCI_EXPRESS_MRL_STATE;

typedef enum {

    SlotEmpty = 0,
    CardPresent

} PCI_EXPRESS_CARD_PRESENCE;

typedef enum {

    IndicatorOn = 1,
    IndicatorBlink,
    IndicatorOff

} PCI_EXPRESS_INDICATOR_STATE;

typedef enum {

    PowerOn = 0,
    PowerOff

} PCI_EXPRESS_POWER_STATE;

typedef enum {

    NoAspmSupport = 0,
    L0sEntrySupport = 1,
    L1EntrySupport = 2,
    L0sAndL1EntrySupport = 3

} PCI_EXPRESS_ASPM_SUPPORT;

typedef enum {

    L0sAndL1EntryDisabled,
    L0sEntryEnabled,
    L1EntryEnabled,
    L0sAndL1EntryEnabled

} PCI_EXPRESS_ASPM_CONTROL;

typedef enum {

    L0s_Below64ns = 0,
    L0s_64ns_128ns,
    L0s_128ns_256ns,
    L0s_256ns_512ns,
    L0s_512ns_1us,
    L0s_1us_2us,
    L0s_2us_4us,
    L0s_Above4us

} PCI_EXPRESS_L0s_EXIT_LATENCY;

typedef enum {

    L1_Below1us = 0,
    L1_1us_2us,
    L1_2us_4us,
    L1_4us_8us,
    L1_8us_16us,
    L1_16us_32us,
    L1_32us_64us,
    L1_Above64us

} PCI_EXPRESS_L1_EXIT_LATENCY;



typedef enum {

    PciExpressEndpoint = 0,
    PciExpressLegacyEndpoint,
    PciExpressRootPort = 4,
    PciExpressUpstreamSwitchPort,
    PciExpressDownstreamSwitchPort,
    PciExpressToPciXBridge,
    PciXToExpressBridge,
    PciExpressRootComplexIntegratedEndpoint,
    PciExpressRootComplexEventCollector

} PCI_EXPRESS_DEVICE_TYPE;



typedef enum {

    MaxPayload128Bytes = 0,
    MaxPayload256Bytes,
    MaxPayload512Bytes,
    MaxPayload1024Bytes,
    MaxPayload2048Bytes,
    MaxPayload4096Bytes

} PCI_EXPRESS_MAX_PAYLOAD_SIZE;

typedef union _PCI_EXPRESS_PME_REQUESTOR_ID {

    struct {

        USHORT FunctionNumber:3;
        USHORT DeviceNumber:5;
        USHORT BusNumber:8;
    } DUMMYSTRUCTNAME;

    USHORT AsUSHORT;

} PCI_EXPRESS_PME_REQUESTOR_ID, *PPCI_EXPRESS_PME_REQUESTOR_ID;


//
// Portable portion of HAL & HAL bus extender definitions for ----ANDLER
// BusData for installed PCI buses.
//

typedef VOID
(*PciPin2Line) (
    _In_ struct _BUS_HANDLER  *BusHandler,
    _In_ struct _BUS_HANDLER  *RootHandler,
    _In_ PCI_SLOT_NUMBER      SlotNumber,
    _In_ PPCI_COMMON_CONFIG   PciData
    );

typedef VOID
(*PciLine2Pin) (
    _In_ struct _BUS_HANDLER  *BusHandler,
    _In_ struct _BUS_HANDLER  *RootHandler,
    _In_ PCI_SLOT_NUMBER      SlotNumber,
    _In_ PPCI_COMMON_CONFIG   PciNewData,
    _In_ PPCI_COMMON_CONFIG   PciOldData
    );

typedef VOID
(*PciReadWriteConfig) (
    _In_ struct _BUS_HANDLER *BusHandler,
    _In_ PCI_SLOT_NUMBER Slot,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length
    );

#define PCI_DATA_TAG            ' ICP'
#define PCI_DATA_VERSION        1

typedef struct _PCIBUSDATA {
    ULONG                   Tag;
    ULONG                   Version;
    PciReadWriteConfig      ReadConfig;
    PciReadWriteConfig      WriteConfig;
    PciPin2Line             Pin2Line;
    PciLine2Pin             Line2Pin;
    PCI_SLOT_NUMBER         ParentSlot;
    PVOID                   Reserved[4];
} PCIBUSDATA, *PPCIBUSDATA;

//
// Latency Tolerance Reporting (LTR) Capability
//

typedef union _PCI_EXPRESS_LTR_MAX_LATENCY_REGISTER {

    struct {

        ULONG MaxSnoopLatencyValue:10;
        ULONG MaxSnoopLatencyScale:3;
        ULONG Rsvd:2;
        ULONG MaxSnoopRequirement:1;

        ULONG MaxNoSnoopLatencyValue:10;
        ULONG MaxNoSnoopLatencyScale:3;
        ULONG Rsvd2:2;
        ULONG MaxNoSnoopRequirement:1;
    } DUMMYSTRUCTNAME;

    ULONG AsULONG;

} PCI_EXPRESS_LTR_MAX_LATENCY_REGISTER, *PPCI_EXPRESS_LTR_MAX_LATENCY_REGISTER;

typedef struct _PCI_EXPRESS_LTR_CAPABILITY {

    PCI_EXPRESS_ENHANCED_CAPABILITY_HEADER Header;

    PCI_EXPRESS_LTR_MAX_LATENCY_REGISTER Latency;

} PCI_EXPRESS_LTR_CAPABILITY, *PPCI_EXPRESS_LTR_CAPABILITY;

//
// Transaction Processing Hint (TPH) Capability
//

#define PCI_EXPRESS_TPH_ST_LOCATION_NONE 0
#define PCI_EXPRESS_TPH_ST_LOCATION_TPH_CAPABILITY 1
#define PCI_EXPRESS_TPH_ST_LOCATION_MSIX_TABLE 2
#define PCI_EXPRESS_TPH_ST_LOCATION_RESERVED 3

typedef union _PCI_EXPRESS_TPH_REQUESTER_CAPABILITY_REGISTER {

    struct {

        ULONG NoStModeSupported:1;
        ULONG InteruptVectorModeSupported:1;
        ULONG DeviceSpecificModeSupported:1;
        ULONG Rsvd:5;
        ULONG ExtendedTPHRequesterSupported:1;
        ULONG StTableLocation:2;
        ULONG Rsvd2:5;
        ULONG StTableSize:11;
        ULONG Rsvd3:5;
    } DUMMYSTRUCTNAME;

    ULONG AsULONG;

} PCI_EXPRESS_TPH_REQUESTER_CAPABILITY_REGISTER, *PPCI_EXPRESS_TPH_REQUESTER_CAPABILITY_REGISTER;

typedef union _PCI_EXPRESS_TPH_REQUESTER_CONTROL_REGISTER {

    struct {

        ULONG StModeSelect:3;
        ULONG Rsvd:5;
        ULONG TphRequesterEnable:2;
        ULONG Rsvd2:22;
    } DUMMYSTRUCTNAME;

    ULONG AsULONG;

} PCI_EXPRESS_TPH_REQUESTER_CONTROL_REGISTER, *PPCI_EXPRESS_TPH_REQUESTER_CONTROL_REGISTER;

typedef union _PCI_EXPRESS_TPH_ST_TABLE_ENTRY {

    struct {

        USHORT LowerEntry:8;
        USHORT UpperEntry:8;
    } DUMMYSTRUCTNAME;

    USHORT AsUSHORT;

} PCI_EXPRESS_TPH_ST_TABLE_ENTRY, *PPCI_EXPRESS_TPH_ST_TABLE_ENTRY;

typedef struct _PCI_EXPRESS_TPH_REQUESTER_CAPABILITY {

    PCI_EXPRESS_ENHANCED_CAPABILITY_HEADER Header;

    PCI_EXPRESS_TPH_REQUESTER_CAPABILITY_REGISTER RequesterCapability;
    PCI_EXPRESS_TPH_REQUESTER_CONTROL_REGISTER RequesterControl;

} PCI_EXPRESS_TPH_REQUESTER_CAPABILITY, *PPCI_EXPRESS_TPH_REQUESTER_CAPABILITY;

//
// L1 PM Substates Capability
//

typedef union _PCI_EXPRESS_L1_PM_SS_CAPABILITIES_REGISTER {

    struct {
        ULONG PciPmL12Supported:1;
        ULONG PciPmL11Supported:1;
        ULONG AspmL12Supported:1;
        ULONG AspmL11Supported:1;
        ULONG L1PmSsSupported:1;
        ULONG Rsvd:3;
        ULONG PortCommonModeRestoreTime:8;
        ULONG PortTPowerOnScale:2;
        ULONG Rsvd2:1;
        ULONG PortTPowerOnValue:5;
        ULONG Rsvd3:8;
    } DUMMYSTRUCTNAME;

    ULONG AsULONG;

} PCI_EXPRESS_L1_PM_SS_CAPABILITIES_REGISTER, *PPCI_EXPRESS_L1_PM_SS_CAPABILITIES_REGISTER;

typedef union _PCI_EXPRESS_L1_PM_SS_CONTROL_1_REGISTER {

    struct {
        ULONG PciPmL12Enabled:1;
        ULONG PciPmL11Enabled:1;
        ULONG AspmL12Enabled:1;
        ULONG AspmL11Enabled:1;
        ULONG Rsvd:4;
        ULONG CommonModeRestoreTime:8;
        ULONG LtrL12ThresholdValue:10;
        ULONG Rsvd2:3;
        ULONG LtrL12ThresholdScale:3;
    } DUMMYSTRUCTNAME;

    ULONG AsULONG;

} PCI_EXPRESS_L1_PM_SS_CONTROL_1_REGISTER, *PPCI_EXPRESS_L1_PM_SS_CONTROL_1_REGISTER;

typedef union _PCI_EXPRESS_L1_PM_SS_CONTROL_2_REGISTER {

    struct {

        ULONG TPowerOnScale:2;
        ULONG Rsvd:1;
        ULONG TPowerOnValue:5;
        ULONG Rsvd2:24;
    } DUMMYSTRUCTNAME;

    ULONG AsULONG;

} PCI_EXPRESS_L1_PM_SS_CONTROL_2_REGISTER, *PPCI_EXPRESS_L1_PM_SS_CONTROL_2_REGISTER;

typedef struct _PCI_EXPRESS_L1_PM_SS_CAPABILITY {

    PCI_EXPRESS_ENHANCED_CAPABILITY_HEADER Header;

    PCI_EXPRESS_L1_PM_SS_CAPABILITIES_REGISTER L1PmSsCapabilities;
    PCI_EXPRESS_L1_PM_SS_CONTROL_1_REGISTER L1PmSsControl1;
    PCI_EXPRESS_L1_PM_SS_CONTROL_2_REGISTER L1PmSsControl2;

} PCI_EXPRESS_L1_PM_SS_CAPABILITY, *PPCI_EXPRESS_L1_PM_SS_CAPABILITY;

typedef union _PCI_EXPRESS_RESIZABLE_BAR_CAPABILITY_REGISTER {
    struct {
        ULONG Rsvd:4;
        ULONG SizesSupported:20;
        ULONG Rsvd2:8;
    } DUMMYSTRUCTNAME;

    ULONG AsULONG;

} PCI_EXPRESS_RESIZABLE_BAR_CAPABILITY_REGISTER, *PPCI_EXPRESS_RESIZABLE_BAR_CAPABILITY_REGISTER;

typedef union _PCI_EXPRESS_RESIZABLE_BAR_CONTROL_REGISTER {
    struct {

        ULONG BarIndex:3;
        ULONG Rsvd:2;
        ULONG NumberOfResizableBars:3;
        ULONG BarSize:5;
        ULONG Rsvd2:19;
    } DUMMYSTRUCTNAME;

    ULONG AsULONG;

} PCI_EXPRESS_RESIZABLE_BAR_CONTROL_REGISTER, *PPCI_EXPRESS_RESIZABLE_BAR_CONTROL_REGISTER;

typedef struct _PCI_EXPRESS_RESIZABLE_BAR_ENTRY {
    PCI_EXPRESS_RESIZABLE_BAR_CAPABILITY_REGISTER Capability;
    PCI_EXPRESS_RESIZABLE_BAR_CONTROL_REGISTER Control;
} PCI_EXPRESS_RESIZABLE_BAR_ENTRY, *PPCI_EXPRESS_RESIZABLE_BAR_ENTRY;

typedef struct _PCI_EXPRESS_RESIZABLE_BAR_CAPABILITY {

    PCI_EXPRESS_ENHANCED_CAPABILITY_HEADER Header;

    PCI_EXPRESS_RESIZABLE_BAR_ENTRY Entry[6];

} PCI_EXPRESS_RESIZABLE_BAR_CAPABILITY, *PPCI_EXPRESS_RESIZABLE_BAR_CAPABILITY;

//
// PCI Express Designated Vendor Specific Capability
//

typedef union _PCI_EXPRESS_DESIGNATED_VENDOR_SPECIFIC_HEADER_1 {
    struct {
        ULONG DvsecVendorId:16;
        ULONG DvsecVersion:4;
        ULONG DvsecLength:12;
    } DUMMYSTRUCTNAME;

    ULONG AsULONG;

} PCI_EXPRESS_DESIGNATED_VENDOR_SPECIFIC_HEADER_1, *PPCI_EXPRESS_DESIGNATED_VENDOR_SPECIFIC_HEADER_1;

typedef union _PCI_EXPRESS_DESIGNATED_VENDOR_SPECIFIC_HEADER_2 {
    struct {
        USHORT DvsecId:16;
    } DUMMYSTRUCTNAME;

    USHORT AsUSHORT;

} PCI_EXPRESS_DESIGNATED_VENDOR_SPECIFIC_HEADER_2, *PPCI_EXPRESS_DESIGNATED_VENDOR_SPECIFIC_HEADER_2;

typedef struct _PCI_EXPRESS_DESIGNATED_VENDOR_SPECIFIC_CAPABILITY {
    PCI_EXPRESS_ENHANCED_CAPABILITY_HEADER Header;
    PCI_EXPRESS_DESIGNATED_VENDOR_SPECIFIC_HEADER_1 DvsecHeader1;
    PCI_EXPRESS_DESIGNATED_VENDOR_SPECIFIC_HEADER_2 DvsecHeader2;
    USHORT DvsecRegisters[1];

} PCI_EXPRESS_DESIGNATED_VENDOR_SPECIFIC_CAPABILITY, *PPCI_EXPRESS_DESIGNATED_VENDOR_SPECIFIC_CAPABILITY;

#define PCI_INVALID_ALTERNATE_FUNCTION_NUMBER 0xFF


#ifndef _PCIINTRF_X_
#define _PCIINTRF_X_

//
// PCI Bus interface
//

typedef
_Function_class_(PCI_READ_WRITE_CONFIG)
_IRQL_requires_max_(HIGH_LEVEL)
ULONG 
PCI_READ_WRITE_CONFIG (
    _In_ PVOID Context,
    _In_ ULONG BusOffset,
    _In_ ULONG Slot,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length
    );

typedef PCI_READ_WRITE_CONFIG *PPCI_READ_WRITE_CONFIG;

typedef
_Function_class_(PCI_PIN_TO_LINE)
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID 
PCI_PIN_TO_LINE (
    _In_ PVOID Context,
    _In_ PPCI_COMMON_CONFIG PciData
    );

typedef PCI_PIN_TO_LINE *PPCI_PIN_TO_LINE;

typedef
_Function_class_(PCI_LINE_TO_PIN)
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
PCI_LINE_TO_PIN (
    _In_ PVOID Context,
    _In_ PPCI_COMMON_CONFIG PciNewData,
    _In_ PPCI_COMMON_CONFIG PciOldData
    );

typedef PCI_LINE_TO_PIN *PPCI_LINE_TO_PIN;

typedef
_Function_class_(PCI_ROOT_BUS_CAPABILITY)
_IRQL_requires_(PASSIVE_LEVEL)
VOID
PCI_ROOT_BUS_CAPABILITY (
    _In_ PVOID Context,
    _Out_ PPCI_ROOT_BUS_HARDWARE_CAPABILITY HardwareCapability
    );

typedef PCI_ROOT_BUS_CAPABILITY *PPCI_ROOT_BUS_CAPABILITY;

typedef
_Function_class_(PCI_EXPRESS_WAKE_CONTROL)
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
PCI_EXPRESS_WAKE_CONTROL (
    _In_ PVOID Context,
    _In_ BOOLEAN EnableWake
    );

typedef PCI_EXPRESS_WAKE_CONTROL *PPCI_EXPRESS_WAKE_CONTROL;

typedef
_Function_class_(PCI_PREPARE_MULTISTAGE_RESUME)
_IRQL_requires_max_(HIGH_LEVEL)
VOID
PCI_PREPARE_MULTISTAGE_RESUME (
    _In_ PVOID Context
    );

typedef PCI_PREPARE_MULTISTAGE_RESUME *PPCI_PREPARE_MULTISTAGE_RESUME;


typedef struct _PCI_BUS_INTERFACE_STANDARD {
    //
    // generic interface header
    //
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    //
    // standard PCI bus interfaces
    //
    PPCI_READ_WRITE_CONFIG ReadConfig;
    PPCI_READ_WRITE_CONFIG WriteConfig;
    PPCI_PIN_TO_LINE PinToLine;
    PPCI_LINE_TO_PIN LineToPin;
    PPCI_ROOT_BUS_CAPABILITY RootBusCapability;
    PPCI_EXPRESS_WAKE_CONTROL ExpressWakeControl;
    PPCI_PREPARE_MULTISTAGE_RESUME PrepareMultistageResume;

} PCI_BUS_INTERFACE_STANDARD, *PPCI_BUS_INTERFACE_STANDARD;

#define PCI_BUS_INTERFACE_STANDARD_VERSION 2
#define PCI_BUS_INTERFACE_STANDARD_VERSION_1_LENGTH \
    FIELD_OFFSET(PCI_BUS_INTERFACE_STANDARD, PrepareMultistageResume)


#endif

//
// Define exported ZwXxx routines to device drivers.
//

#if (NTDDI_VERSION >= NTDDI_WIN2K)
//@[comment("MVI_tracked")]
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationThread (
    _In_ HANDLE ThreadHandle,
    _In_ THREADINFOCLASS ThreadInformationClass,
    _In_reads_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(PASSIVE_LEVEL)
_When_(return=0, __drv_allocatesMem(TimerObject))
NTSTATUS
ZwCreateTimer (
    _Out_ PHANDLE TimerHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ TIMER_TYPE TimerType
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
ZwOpenTimer (
    _Out_ PHANDLE TimerHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
ZwCancelTimer (
    _In_ HANDLE TimerHandle,
    _Out_opt_ PBOOLEAN CurrentState
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
ZwSetTimer (
    _In_ HANDLE TimerHandle,
    _In_ PLARGE_INTEGER DueTime,
    _In_opt_ PTIMER_APC_ROUTINE TimerApcRoutine,
    _In_opt_ PVOID TimerContext,
    _In_ BOOLEAN ResumeTimer,
    _In_opt_ LONG Period,
    _Out_opt_ PBOOLEAN PreviousState
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
ZwSetTimerEx (
    _In_ HANDLE TimerHandle,
    _In_ TIMER_SET_INFORMATION_CLASS TimerSetInformationClass,
    _Inout_updates_bytes_opt_(TimerSetInformationLength) PVOID TimerSetInformation,
    _In_ ULONG TimerSetInformationLength
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryVolumeInformationFile(
    _In_ HANDLE FileHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _Out_writes_bytes_(Length) PVOID FsInformation,
    _In_ ULONG Length,
    _In_ FS_INFORMATION_CLASS FsInformationClass
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwDeviceIoControlFile(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwDisplayString(
    _In_ PUNICODE_STRING String
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwPowerInformation(
    _In_ POWER_INFORMATION_LEVEL InformationLevel,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength
    );
#endif


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwAllocateLocallyUniqueId(
    _Out_ PLUID Luid
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
//@[comment("MVI_tracked")]
NTSYSAPI
NTSTATUS
NTAPI
ZwTerminateProcess (
    _In_opt_ HANDLE ProcessHandle,
    _In_ NTSTATUS ExitStatus
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwOpenProcess (
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PCLIENT_ID ClientId
    );


//------------------------------------------------------ WHEA_ERROR_SOURCE_TYPE

#define WHEA_PHYSICAL_ADDRESS LARGE_INTEGER

//
// This enumeration defines the various types of error sources that a platform
// can expose to the operating system.
//

typedef enum _WHEA_ERROR_SOURCE_TYPE {
    WheaErrSrcTypeMCE          = 0x00,    // Machine Check Exception
    WheaErrSrcTypeCMC          = 0x01,    // Corrected Machine Check
    WheaErrSrcTypeCPE          = 0x02,    // Corrected Platform Error
    WheaErrSrcTypeNMI          = 0x03,    // Non-Maskable Interrupt
    WheaErrSrcTypePCIe         = 0x04,    // PCI Express Error
    WheaErrSrcTypeGeneric      = 0x05,    // Other types of error sources
    WheaErrSrcTypeINIT         = 0x06,    // IA64 INIT Error Source
    WheaErrSrcTypeBOOT         = 0x07,    // BOOT Error Source
    WheaErrSrcTypeSCIGeneric   = 0x08,    // SCI-based generic error source
    WheaErrSrcTypeIPFMCA       = 0x09,    // Itanium Machine Check Abort
    WheaErrSrcTypeIPFCMC       = 0x0a,    // Itanium Machine check
    WheaErrSrcTypeIPFCPE       = 0x0b,    // Itanium Corrected Platform Error
    WheaErrSrcTypeGenericV2    = 0x0c,    // Other types of error sources v2
    WheaErrSrcTypeSCIGenericV2 = 0x0d,    // SCI-based GHESv2
    WheaErrSrcTypeMax
} WHEA_ERROR_SOURCE_TYPE, *PWHEA_ERROR_SOURCE_TYPE;

//
// Error sources have a runtime state associated with them. The following are
// the valid states for an error source.
//

typedef enum _WHEA_ERROR_SOURCE_STATE {
    WheaErrSrcStateStopped = 0x01,
    WheaErrSrcStateStarted = 0x02
} WHEA_ERROR_SOURCE_STATE, *PWHEA_ERROR_SOURCE_STATE;

#define WHEA_ERROR_SOURCE_DESCRIPTOR_VERSION_10          10

#define WHEA_MAX_MC_BANKS                                32

#define WHEA_ERROR_SOURCE_FLAG_FIRMWAREFIRST             0x00000001
#define WHEA_ERROR_SOURCE_FLAG_GLOBAL                    0x00000002
#define WHEA_ERROR_SOURCE_FLAG_PREALLOCATE_PER_PROCESSOR 0x00000004
#define WHEA_ERROR_SOURCE_FLAG_DEFAULTSOURCE             0x80000000

#define WHEA_ERROR_SOURCE_DESCRIPTOR_TYPE_XPFMCE         0
#define WHEA_ERROR_SOURCE_DESCRIPTOR_TYPE_XPFCMC         1
#define WHEA_ERROR_SOURCE_DESCRIPTOR_TYPE_XPFNMI         2
#define WHEA_ERROR_SOURCE_DESCRIPTOR_TYPE_IPFMCA         3
#define WHEA_ERROR_SOURCE_DESCRIPTOR_TYPE_IPFCMC         4
#define WHEA_ERROR_SOURCE_DESCRIPTOR_TYPE_IPFCPE         5
#define WHEA_ERROR_SOURCE_DESCRIPTOR_TYPE_AERROOTPORT    6
#define WHEA_ERROR_SOURCE_DESCRIPTOR_TYPE_AERENDPOINT    7
#define WHEA_ERROR_SOURCE_DESCRIPTOR_TYPE_AERBRIDGE      8
#define WHEA_ERROR_SOURCE_DESCRIPTOR_TYPE_GENERIC        9
#define WHEA_ERROR_SOURCE_DESCRIPTOR_TYPE_GENERIC_V2     10

#define WHEA_XPF_MC_BANK_STATUSFORMAT_IA32MCA            0
#define WHEA_XPF_MC_BANK_STATUSFORMAT_Intel64MCA         1
#define WHEA_XPF_MC_BANK_STATUSFORMAT_AMD64MCA           2

#define WHEA_NOTIFICATION_TYPE_POLLED                    0
#define WHEA_NOTIFICATION_TYPE_EXTERNALINTERRUPT         1
#define WHEA_NOTIFICATION_TYPE_LOCALINTERRUPT            2
#define WHEA_NOTIFICATION_TYPE_SCI                       3
#define WHEA_NOTIFICATION_TYPE_NMI                       4
#define WHEA_NOTIFICATION_TYPE_CMCI                      5
#define WHEA_NOTIFICATION_TYPE_MCE                       6
#define WHEA_NOTIFICATION_TYPE_GPIO_SIGNAL               7
#define WHEA_NOTIFICATION_TYPE_ARMV8_SEA                 8
#define WHEA_NOTIFICATION_TYPE_ARMV8_SEI                 9
#define WHEA_NOTIFICATION_TYPE_EXTERNALINTERRUPT_GSIV    10
#define WHEA_NOTIFICATION_TYPE_SDEI                      11

#include <pshpack1.h>

//------------------------------------------------ WHEA_ERROR_SOURCE_DESCRIPTOR

typedef union _WHEA_NOTIFICATION_FLAGS {
    struct {
        USHORT PollIntervalRW:1;
        USHORT SwitchToPollingThresholdRW:1;
        USHORT SwitchToPollingWindowRW:1;
        USHORT ErrorThresholdRW:1;
        USHORT ErrorThresholdWindowRW:1;
        USHORT Reserved:11;
    } DUMMYSTRUCTNAME;
    USHORT AsUSHORT;
} WHEA_NOTIFICATION_FLAGS, *PWHEA_NOTIFICATION_FLAGS;

typedef union _XPF_MC_BANK_FLAGS {
    struct {
        UCHAR ClearOnInitializationRW:1;
        UCHAR ControlDataRW:1;
        UCHAR Reserved:6;
    } DUMMYSTRUCTNAME;
    UCHAR AsUCHAR;
} XPF_MC_BANK_FLAGS, *PXPF_MC_BANK_FLAGS;

typedef union _XPF_MCE_FLAGS {
    struct {
        ULONG MCG_CapabilityRW:1;
        ULONG MCG_GlobalControlRW:1;
        ULONG Reserved:30;
    } DUMMYSTRUCTNAME;
    ULONG AsULONG;
} XPF_MCE_FLAGS, *PXPF_MCE_FLAGS;

typedef union _AER_ROOTPORT_DESCRIPTOR_FLAGS {
    struct {
        USHORT UncorrectableErrorMaskRW:1;
        USHORT UncorrectableErrorSeverityRW:1;
        USHORT CorrectableErrorMaskRW:1;
        USHORT AdvancedCapsAndControlRW:1;
        USHORT RootErrorCommandRW:1;
        USHORT Reserved:11;
    } DUMMYSTRUCTNAME;
    USHORT AsUSHORT;
} AER_ROOTPORT_DESCRIPTOR_FLAGS, *PAER_ROOTPORT_DESCRIPTOR_FLAGS;

typedef union _AER_ENDPOINT_DESCRIPTOR_FLAGS {
    struct {
        USHORT UncorrectableErrorMaskRW:1;
        USHORT UncorrectableErrorSeverityRW:1;
        USHORT CorrectableErrorMaskRW:1;
        USHORT AdvancedCapsAndControlRW:1;
        USHORT Reserved:12;
    } DUMMYSTRUCTNAME;
    USHORT AsUSHORT;
} AER_ENDPOINT_DESCRIPTOR_FLAGS, *PAER_ENDPOINT_DESCRIPTOR_FLAGS;

typedef union _AER_BRIDGE_DESCRIPTOR_FLAGS {
    struct {
        USHORT UncorrectableErrorMaskRW:1;
        USHORT UncorrectableErrorSeverityRW:1;
        USHORT CorrectableErrorMaskRW:1;
        USHORT AdvancedCapsAndControlRW:1;
        USHORT SecondaryUncorrectableErrorMaskRW:1;
        USHORT SecondaryUncorrectableErrorSevRW:1;
        USHORT SecondaryCapsAndControlRW:1;
        USHORT Reserved:9;
    } DUMMYSTRUCTNAME;
    USHORT AsUSHORT;
} AER_BRIDGE_DESCRIPTOR_FLAGS, *PAER_BRIDGE_DESCRIPTOR_FLAGS;

//
// The following structure is used to describe how a given error source reports
// errors to the OS.
//

typedef struct _WHEA_NOTIFICATION_DESCRIPTOR {
    UCHAR Type;
    UCHAR Length;
    WHEA_NOTIFICATION_FLAGS Flags;

    union {
        struct {
            ULONG PollInterval;
        } Polled;

        struct {
            ULONG PollInterval;
            ULONG Vector;
            ULONG SwitchToPollingThreshold;
            ULONG SwitchToPollingWindow;
            ULONG ErrorThreshold;
            ULONG ErrorThresholdWindow;
        } Interrupt;

        struct {
            ULONG PollInterval;
            ULONG Vector;
            ULONG SwitchToPollingThreshold;
            ULONG SwitchToPollingWindow;
            ULONG ErrorThreshold;
            ULONG ErrorThresholdWindow;
        } LocalInterrupt;

        struct {
            ULONG PollInterval;
            ULONG Vector;
            ULONG SwitchToPollingThreshold;
            ULONG SwitchToPollingWindow;
            ULONG ErrorThreshold;
            ULONG ErrorThresholdWindow;
        } Sci;

        struct {
            ULONG PollInterval;
            ULONG Vector;
            ULONG SwitchToPollingThreshold;
            ULONG SwitchToPollingWindow;
            ULONG ErrorThreshold;
            ULONG ErrorThresholdWindow;
        } Nmi;

        struct {
            ULONG PollInterval;
            ULONG Vector;
            ULONG SwitchToPollingThreshold;
            ULONG SwitchToPollingWindow;
            ULONG ErrorThreshold;
            ULONG ErrorThresholdWindow;
        } Sea;

        struct {
            ULONG PollInterval;
            ULONG Vector;
            ULONG SwitchToPollingThreshold;
            ULONG SwitchToPollingWindow;
            ULONG ErrorThreshold;
            ULONG ErrorThresholdWindow;
        } Sei;

        struct {
            ULONG PollInterval;
            ULONG Vector;
            ULONG SwitchToPollingThreshold;
            ULONG SwitchToPollingWindow;
            ULONG ErrorThreshold;
            ULONG ErrorThresholdWindow;
        } Gsiv;
    } u;
} WHEA_NOTIFICATION_DESCRIPTOR, *PWHEA_NOTIFICATION_DESCRIPTOR;

//
// The following structure describes an XPF machine check bank. It identifies
// the bank with a BankNumber and it contains information that is used to
// configure the bank. MCE and CMC error sources make use of this descriptor
// to describe and configure each bank.
//

typedef struct _WHEA_XPF_MC_BANK_DESCRIPTOR {
    UCHAR BankNumber;
    BOOLEAN ClearOnInitialization;
    UCHAR StatusDataFormat;
    XPF_MC_BANK_FLAGS Flags;
    ULONG ControlMsr;
    ULONG StatusMsr;
    ULONG AddressMsr;
    ULONG MiscMsr;
    ULONGLONG ControlData;
} WHEA_XPF_MC_BANK_DESCRIPTOR, *PWHEA_XPF_MC_BANK_DESCRIPTOR;

//
// The following structure describes an XPF platform's machine check exception
// error source mechanism. The information represented in this structure tells
// the OS how to configure the platform's MCE error source.
//

typedef struct _WHEA_XPF_MCE_DESCRIPTOR {
    USHORT Type;
    UCHAR Enabled;
    UCHAR NumberOfBanks;
    XPF_MCE_FLAGS Flags;
    ULONGLONG MCG_Capability;
    ULONGLONG MCG_GlobalControl;
    WHEA_XPF_MC_BANK_DESCRIPTOR Banks[WHEA_MAX_MC_BANKS];
} WHEA_XPF_MCE_DESCRIPTOR, *PWHEA_XPF_MCE_DESCRIPTOR;

//
// The following structure describes an XPF platform's corrected machine check
// error source mechanism. The information represented in this structure tells
// the OS how to configure the platform's CMC error source.
//

typedef struct _WHEA_XPF_CMC_DESCRIPTOR {
    USHORT Type;
    BOOLEAN Enabled;
    UCHAR NumberOfBanks;
    ULONG Reserved;
    WHEA_NOTIFICATION_DESCRIPTOR Notify;
    WHEA_XPF_MC_BANK_DESCRIPTOR Banks[WHEA_MAX_MC_BANKS];
} WHEA_XPF_CMC_DESCRIPTOR, *PWHEA_XPF_CMC_DESCRIPTOR;

typedef struct _WHEA_PCI_SLOT_NUMBER {
    union {
        struct {
            ULONG DeviceNumber:5;
            ULONG FunctionNumber:3;
            ULONG Reserved:24;
        } bits;
        ULONG AsULONG;
    } u;
} WHEA_PCI_SLOT_NUMBER, *PWHEA_PCI_SLOT_NUMBER;

//
// The following structure describes an XPF platform's non-maskable interrupt
// error source mechanism. The information represented in this structure tells
// the OS how to configure the platform's NMI error source.
//

typedef struct _WHEA_XPF_NMI_DESCRIPTOR {
    USHORT Type;
    BOOLEAN Enabled;
} WHEA_XPF_NMI_DESCRIPTOR, *PWHEA_XPF_NMI_DESCRIPTOR;

//
// The following structure describes a platform's PCI Express AER root port
// error source. The information represented in this structure tells the OS how
// to configure the root port's AER settings.
//

typedef struct _WHEA_AER_ROOTPORT_DESCRIPTOR {
    USHORT Type;
    BOOLEAN Enabled;
    UCHAR Reserved;
    ULONG BusNumber;
    WHEA_PCI_SLOT_NUMBER Slot;
    USHORT DeviceControl;
    AER_ROOTPORT_DESCRIPTOR_FLAGS Flags;
    ULONG UncorrectableErrorMask;
    ULONG UncorrectableErrorSeverity;
    ULONG CorrectableErrorMask;
    ULONG AdvancedCapsAndControl;
    ULONG RootErrorCommand;
} WHEA_AER_ROOTPORT_DESCRIPTOR, *PWHEA_AER_ROOTPORT_DESCRIPTOR;

//
// The following structure describes a platform's PCI Express AER endpoint
// error source. The information represented in this structure tells the OS how
// to configure the device's AER settings.
//

typedef struct _WHEA_AER_ENDPOINT_DESCRIPTOR {
    USHORT Type;
    BOOLEAN Enabled;
    UCHAR Reserved;
    ULONG BusNumber;
    WHEA_PCI_SLOT_NUMBER Slot;
    USHORT DeviceControl;
    AER_ENDPOINT_DESCRIPTOR_FLAGS Flags;
    ULONG UncorrectableErrorMask;
    ULONG UncorrectableErrorSeverity;
    ULONG CorrectableErrorMask;
    ULONG AdvancedCapsAndControl;
} WHEA_AER_ENDPOINT_DESCRIPTOR, *PWHEA_AER_ENDPOINT_DESCRIPTOR;

//
// The following structure describes a platform's PCI Express AER bridge
// error source. The information represented in this structure tells the OS how
// to configure the bridge's AER settings.
//

typedef struct _WHEA_AER_BRIDGE_DESCRIPTOR {
    USHORT Type;
    BOOLEAN Enabled;
    UCHAR Reserved;
    ULONG BusNumber;
    WHEA_PCI_SLOT_NUMBER Slot;
    USHORT DeviceControl;
    AER_BRIDGE_DESCRIPTOR_FLAGS Flags;
    ULONG UncorrectableErrorMask;
    ULONG UncorrectableErrorSeverity;
    ULONG CorrectableErrorMask;
    ULONG AdvancedCapsAndControl;
    ULONG SecondaryUncorrectableErrorMask;
    ULONG SecondaryUncorrectableErrorSev;
    ULONG SecondaryCapsAndControl;
} WHEA_AER_BRIDGE_DESCRIPTOR, *PWHEA_AER_BRIDGE_DESCRIPTOR;

//
// The following structure describes a generic error source to the OS. Using
// the information in this structure the OS is able to configure a handler for
// the generic error source.
//

typedef struct _WHEA_GENERIC_ERROR_DESCRIPTOR {

    //
    // Type is WHEA_ERROR_SOURCE_DESCRIPTOR_TYPE_GENERIC.
    //

    USHORT Type;

    //
    // This field is reserved.
    //

    UCHAR Reserved;

    //
    // Indicates whether the generic error source is to be enabled.
    //

    UCHAR Enabled;

    //
    // Length of the error status block.
    //

    ULONG ErrStatusBlockLength;

    //
    // If this generic error source relates back to another error source, keep
    // it's identifier here.
    //

    ULONG RelatedErrorSourceId;

    //
    // The following 5 fields have the same layout as a GEN_ADDR structure. They
    // describe the address at which the OS reads error status information
    // from the error source.
    //

    UCHAR ErrStatusAddressSpaceID;
    UCHAR ErrStatusAddressBitWidth;
    UCHAR ErrStatusAddressBitOffset;
    UCHAR ErrStatusAddressAccessSize;
    WHEA_PHYSICAL_ADDRESS ErrStatusAddress;

    //
    // Notify describes how the generic error source notifies the OS that error
    // information is available.
    //

    WHEA_NOTIFICATION_DESCRIPTOR Notify;

} WHEA_GENERIC_ERROR_DESCRIPTOR, *PWHEA_GENERIC_ERROR_DESCRIPTOR;

typedef struct _WHEA_GENERIC_ERROR_DESCRIPTOR_V2 {

    //
    // Type is WHEA_ERROR_SOURCE_DESCRIPTOR_TYPE_GENERIC_V2.
    //

    USHORT Type;

    //
    // This field is reserved.
    //

    UCHAR Reserved;

    //
    // Indicates whether the generic error source is to be enabled.
    //

    UCHAR Enabled;

    //
    // Length of the error status block.
    //

    ULONG ErrStatusBlockLength;

    //
    // If this generic error source relates back to another error source, keep
    // it's identifier here.
    //

    ULONG RelatedErrorSourceId;

    //
    // The following 5 fields have the same layout as a GEN_ADDR structure. They
    // describe the address at which the OS reads error status information
    // from the error source.
    //

    UCHAR ErrStatusAddressSpaceID;
    UCHAR ErrStatusAddressBitWidth;
    UCHAR ErrStatusAddressBitOffset;
    UCHAR ErrStatusAddressAccessSize;
    WHEA_PHYSICAL_ADDRESS ErrStatusAddress;
    
    //
    // Notify describes how the generic error source notifies the OS that error
    // information is available.
    //

    WHEA_NOTIFICATION_DESCRIPTOR Notify;

    //
    // The following 5 fields have the same layout as a GEN_ADDR structure. They
    // describe the address at which the OS will acknoledge the consumption of the
    // error status block.
    //

    UCHAR ReadAckAddressSpaceID;
    UCHAR ReadAckAddressBitWidth;
    UCHAR ReadAckAddressBitOffset;
    UCHAR ReadAckAddressAccessSize;
    WHEA_PHYSICAL_ADDRESS ReadAckAddress;
    ULONGLONG ReadAckPreserveMask;
    ULONGLONG ReadAckWriteMask;

} WHEA_GENERIC_ERROR_DESCRIPTOR_V2, *PWHEA_GENERIC_ERROR_DESCRIPTOR_V2;

typedef struct _WHEA_IPF_MCA_DESCRIPTOR {
    USHORT Type;
    UCHAR Enabled;
    UCHAR Reserved;
} WHEA_IPF_MCA_DESCRIPTOR, *PWHEA_IPF_MCA_DESCRIPTOR;

typedef struct _WHEA_IPF_CMC_DESCRIPTOR {
    USHORT Type;
    UCHAR Enabled;
    UCHAR Reserved;
} WHEA_IPF_CMC_DESCRIPTOR, *PWHEA_IPF_CMC_DESCRIPTOR;

typedef struct _WHEA_IPF_CPE_DESCRIPTOR {
    USHORT Type;
    UCHAR Enabled;
    UCHAR Reserved;
} WHEA_IPF_CPE_DESCRIPTOR, *PWHEA_IPF_CPE_DESCRIPTOR;

typedef struct _WHEA_ERROR_SOURCE_DESCRIPTOR {
    ULONG Length;                                              // +00 (0)
    ULONG Version;                                             // +04 (4)
    WHEA_ERROR_SOURCE_TYPE Type;                               // +08 (8)
    WHEA_ERROR_SOURCE_STATE State;                             // +0C (12)
    ULONG MaxRawDataLength;                                    // +10 (16)
    ULONG NumRecordsToPreallocate;                             // +14 (20)
    ULONG MaxSectionsPerRecord;                                // +18 (24)
    ULONG ErrorSourceId;                                       // +1C (28)
    ULONG PlatformErrorSourceId;                               // +20 (32)
    ULONG Flags;                                               // +24 (36)

    union {                                                    // +28 (40)
        WHEA_XPF_MCE_DESCRIPTOR XpfMceDescriptor;
        WHEA_XPF_CMC_DESCRIPTOR XpfCmcDescriptor;
        WHEA_XPF_NMI_DESCRIPTOR XpfNmiDescriptor;
        WHEA_IPF_MCA_DESCRIPTOR IpfMcaDescriptor;
        WHEA_IPF_CMC_DESCRIPTOR IpfCmcDescriptor;
        WHEA_IPF_CPE_DESCRIPTOR IpfCpeDescriptor;
        WHEA_AER_ROOTPORT_DESCRIPTOR AerRootportDescriptor;
        WHEA_AER_ENDPOINT_DESCRIPTOR AerEndpointDescriptor;
        WHEA_AER_BRIDGE_DESCRIPTOR AerBridgeDescriptor;
        WHEA_GENERIC_ERROR_DESCRIPTOR GenErrDescriptor;
        WHEA_GENERIC_ERROR_DESCRIPTOR_V2 GenErrDescriptorV2;
    } Info;

} WHEA_ERROR_SOURCE_DESCRIPTOR, *PWHEA_ERROR_SOURCE_DESCRIPTOR;

//
// WHEA PFA Policy Type
//

#define    WHEA_DISABLE_OFFLINE        0
#define    WHEA_MEM_PERSISTOFFLINE     1
#define    WHEA_MEM_PFA_DISABLE        2
#define    WHEA_MEM_PFA_PAGECOUNT      3
#define    WHEA_MEM_PFA_THRESHOLD      4
#define    WHEA_MEM_PFA_TIMEOUT        5
#define    WHEA_DISABLE_DUMMY_WRITE    6

#include <poppack.h>


//
// The general format of the common platform error record is illustrated below.
// A record consists of a header; followed by one or more section descriptors;
// and for each descriptor, an associated section which may contain either error
// or informational data.
//
// The record may include extra buffer space to allow for the dynamic addition
// of error sections descriptors and bodies, as well as for dynamically
// increasing the size of existing sections.
//
// +---------------------------------------------+
// | Record Header                               |
// |   SectionCount == N                         |
// +---------------------------------------------+
// | Section Descriptor 1                        |
// |   Offset, size                              | ---+
// +---------------------------------------------+    |
// | Section Descriptor 2                        |    |
// |   Offset, size                              | ---+---+
// +---------------------------------------------+    |   |
// |                                             |    |   |
// | ....                                        |    |   |
// |                                             |    |   |
// +---------------------------------------------+    |   |
// | Section Descriptor N                        | ---+---+---+
// |   Offset, size                              |    |   |   |
// +---------------------------------------------+    |   |   |
// |                     Buffer space for adding |    |   |   |
// |                   more section descriptors. |    |   |   |
// +---------------------------------------------|    |   |   |
// | Section 1                                   | <--+   |   |
// |                                             |        |   |
// +---------------------------------------------+        |   |
// | Section 2                                   | <------+   |
// |                                             |            |
// +---------------------------------------------+            |
// |                                             |            |
// |                                             |            |
// | ....                                        |            |
// |                                             |            |
// |                                             |            |
// +---------------------------------------------+            |
// | Section N                                   | <----------+
// |                                             |
// +---------------------------------------------+
// |                                             |
// |                                             |
// |                                             |
// |                     Buffer space for adding |
// |                        more section bodies. |
// |                                             |
// |                                             |
// |                                             |
// +---------------------------------------------+
//

// -------------------------------------------- Specification validation macros

//
// The following macro implements a compile-time check for the offset and length
// of the specified structure member. This can be used to validate the defined
// structures against the specification.
//

#define CPER_FIELD_CHECK(type, field, offset, length) \
    C_ASSERT(((FIELD_OFFSET(type, field) == (offset)) && \
              (RTL_FIELD_SIZE(type, field) == (length))))

#include <pshpack1.h>

//---------------------------------- Downlevel GUID variable name compatibility

#if WHEA_DOWNLEVEL_TYPE_NAMES

#define PROCESSOR_GENERIC_SECTION_GUID          PROCESSOR_GENERIC_ERROR_SECTION_GUID
#define X86_PROCESSOR_SPECIFIC_SECTION_GUID     XPF_PROCESSOR_ERROR_SECTION_GUID
#define IPF_PROCESSOR_SPECIFIC_SECTION_GUID     IPF_PROCESSOR_ERROR_SECTION_GUID
#define ARM_PROCESSOR_SPECIFIC_SECTION_GUID     ARM_PROCESSOR_ERROR_SECTION_GUID
#define PLATFORM_MEMORY_SECTION_GUID            MEMORY_ERROR_SECTION_GUID
#define PCIEXPRESS_SECTION_GUID                 PCIEXPRESS_ERROR_SECTION_GUID
#define PCIX_BUS_SECTION_GUID                   PCIXBUS_ERROR_SECTION_GUID
#define PCIX_COMPONENT_SECTION_GUID             PCIXDEVICE_ERROR_SECTION_GUID
#define IPF_SAL_RECORD_REFERENCE_SECTION_GUID   FIRMWARE_ERROR_RECORD_REFERENCE_GUID

#endif

//------------------------------------------ Common Platform Error Record types

//
// These types are used in several of the common platform error record
// structures.
//

typedef union _WHEA_REVISION {
    struct {
        UCHAR MinorRevision;
        UCHAR MajorRevision;
    } DUMMYSTRUCTNAME;
    USHORT AsUSHORT;
} WHEA_REVISION, *PWHEA_REVISION;

typedef enum _WHEA_ERROR_SEVERITY {
    WheaErrSevRecoverable   = 0,
    WheaErrSevFatal         = 1,
    WheaErrSevCorrected     = 2,
    WheaErrSevInformational = 3
} WHEA_ERROR_SEVERITY, *PWHEA_ERROR_SEVERITY;

typedef union _WHEA_TIMESTAMP {
    struct {
        ULONGLONG Seconds:8;
        ULONGLONG Minutes:8;
        ULONGLONG Hours:8;
        ULONGLONG Precise:1;
        ULONGLONG Reserved:7;
        ULONGLONG Day:8;
        ULONGLONG Month:8;
        ULONGLONG Year:8;
        ULONGLONG Century:8;
    } DUMMYSTRUCTNAME;
    LARGE_INTEGER AsLARGE_INTEGER;
} WHEA_TIMESTAMP, *PWHEA_TIMESTAMP;

typedef union _WHEA_PERSISTENCE_INFO {
    struct {
        ULONGLONG Signature:16;
        ULONGLONG Length:24;
        ULONGLONG Identifier:16;
        ULONGLONG Attributes:2;
        ULONGLONG DoNotLog:1;
        ULONGLONG Reserved:5;
    } DUMMYSTRUCTNAME;
    ULONGLONG AsULONGLONG;
} WHEA_PERSISTENCE_INFO, *PWHEA_PERSISTENCE_INFO;

#define ERRTYP_INTERNAL                 0x01 // 1
#define ERRTYP_BUS                      0x10 // 16
#define ERRTYP_MEM                      0x04 // 4
#define ERRTYP_TLB                      0x05 // 5
#define ERRTYP_CACHE                    0x06 // 6
#define ERRTYP_FUNCTION                 0x07 // 7
#define ERRTYP_SELFTEST                 0x08 // 8
#define ERRTYP_FLOW                     0x09 // 9
#define ERRTYP_MAP                      0x11 // 17
#define ERRTYP_IMPROPER                 0x12 // 18
#define ERRTYP_UNIMPL                   0x13 // 19
#define ERRTYP_LOSSOFLOCKSTEP           0x14 // 20
#define ERRTYP_RESPONSE                 0x15 // 21
#define ERRTYP_PARITY                   0x16 // 22
#define ERRTYP_PROTOCOL                 0x17 // 23
#define ERRTYP_PATHERROR                0x18 // 24
#define ERRTYP_TIMEOUT                  0x19 // 25
#define ERRTYP_POISONED                 0x1A // 26

typedef union _WHEA_ERROR_STATUS {
    ULONGLONG ErrorStatus;
    struct {
        ULONGLONG Reserved1:8;
        ULONGLONG ErrorType:8;
        ULONGLONG Address:1;
        ULONGLONG Control:1;
        ULONGLONG Data:1;
        ULONGLONG Responder:1;
        ULONGLONG Requester:1;
        ULONGLONG FirstError:1;
        ULONGLONG Overflow:1;
        ULONGLONG Reserved2:41;
    } DUMMYSTRUCTNAME;
} WHEA_ERROR_STATUS, *PWHEA_ERROR_STATUS;

//---------------------------------------------------- WHEA_ERROR_RECORD_HEADER

typedef union _WHEA_ERROR_RECORD_HEADER_VALIDBITS {
    struct {
        ULONG PlatformId:1;
        ULONG Timestamp:1;
        ULONG PartitionId:1;
        ULONG Reserved:29;
    } DUMMYSTRUCTNAME;
    ULONG AsULONG;
} WHEA_ERROR_RECORD_HEADER_VALIDBITS, *PWHEA_ERROR_RECORD_HEADER_VALIDBITS;

#define WHEA_ERROR_RECORD_VALID_PLATFORMID           0x00000001
#define WHEA_ERROR_RECORD_VALID_TIMESTAMP            0x00000002
#define WHEA_ERROR_RECORD_VALID_PARTITIONID          0x00000004

typedef union _WHEA_ERROR_RECORD_HEADER_FLAGS {
    struct {
        ULONG Recovered:1;
        ULONG PreviousError:1;
        ULONG Simulated:1;
        ULONG Reserved:29;
    } DUMMYSTRUCTNAME;
    ULONG AsULONG;
} WHEA_ERROR_RECORD_HEADER_FLAGS, *PWHEA_ERROR_RECORD_HEADER_FLAGS;

#define WHEA_ERROR_RECORD_FLAGS_RECOVERED            0x00000001
#define WHEA_ERROR_RECORD_FLAGS_PREVIOUSERROR        0x00000002
#define WHEA_ERROR_RECORD_FLAGS_SIMULATED            0x00000004

typedef struct _WHEA_ERROR_RECORD_HEADER {
    ULONG Signature;
    WHEA_REVISION Revision;
    ULONG SignatureEnd;
    USHORT SectionCount;
    WHEA_ERROR_SEVERITY Severity;
    WHEA_ERROR_RECORD_HEADER_VALIDBITS ValidBits;
    _Field_range_(>=, (sizeof(WHEA_ERROR_RECORD_HEADER)
                       + (SectionCount
                          * sizeof(WHEA_ERROR_RECORD_SECTION_DESCRIPTOR))))
        ULONG Length;
    WHEA_TIMESTAMP Timestamp;
    GUID PlatformId;
    GUID PartitionId;
    GUID CreatorId;
    GUID NotifyType;
    ULONGLONG RecordId;
    WHEA_ERROR_RECORD_HEADER_FLAGS Flags;
    WHEA_PERSISTENCE_INFO PersistenceInfo;
    UCHAR Reserved[12];
} WHEA_ERROR_RECORD_HEADER, *PWHEA_ERROR_RECORD_HEADER;

//
// Distinguished values used in the common platform error record header
// signature.
//

#define WHEA_ERROR_RECORD_SIGNATURE         'REPC'
#define WHEA_ERROR_RECORD_REVISION          0x0210
#define WHEA_ERROR_RECORD_SIGNATURE_END     0xFFFFFFFF

//
// Validate the error record header structure against the definitions in the
// UEFI specification.
//

CPER_FIELD_CHECK(WHEA_ERROR_RECORD_HEADER, Signature,         0,  4);
CPER_FIELD_CHECK(WHEA_ERROR_RECORD_HEADER, Revision,          4,  2);
CPER_FIELD_CHECK(WHEA_ERROR_RECORD_HEADER, SignatureEnd,      6,  4);
CPER_FIELD_CHECK(WHEA_ERROR_RECORD_HEADER, SectionCount,     10,  2);
CPER_FIELD_CHECK(WHEA_ERROR_RECORD_HEADER, Severity,         12,  4);
CPER_FIELD_CHECK(WHEA_ERROR_RECORD_HEADER, ValidBits,        16,  4);
CPER_FIELD_CHECK(WHEA_ERROR_RECORD_HEADER, Length,           20,  4);
CPER_FIELD_CHECK(WHEA_ERROR_RECORD_HEADER, Timestamp,        24,  8);
CPER_FIELD_CHECK(WHEA_ERROR_RECORD_HEADER, PlatformId,       32, 16);
CPER_FIELD_CHECK(WHEA_ERROR_RECORD_HEADER, PartitionId,      48, 16);
CPER_FIELD_CHECK(WHEA_ERROR_RECORD_HEADER, CreatorId,        64, 16);
CPER_FIELD_CHECK(WHEA_ERROR_RECORD_HEADER, NotifyType,       80, 16);
CPER_FIELD_CHECK(WHEA_ERROR_RECORD_HEADER, RecordId,         96,  8);
CPER_FIELD_CHECK(WHEA_ERROR_RECORD_HEADER, Flags,           104,  4);
CPER_FIELD_CHECK(WHEA_ERROR_RECORD_HEADER, PersistenceInfo, 108,  8);
CPER_FIELD_CHECK(WHEA_ERROR_RECORD_HEADER, Reserved,        116, 12);

//---------------------------------------- WHEA_ERROR_RECORD_SECTION_DESCRIPTOR

typedef union _WHEA_ERROR_RECORD_SECTION_DESCRIPTOR_FLAGS {
    struct {
        ULONG Primary:1;
        ULONG ContainmentWarning:1;
        ULONG Reset:1;
        ULONG ThresholdExceeded:1;
        ULONG ResourceNotAvailable:1;
        ULONG LatentError:1;
        ULONG Propagated:1;
        ULONG Reserved:25;
    } DUMMYSTRUCTNAME;
    ULONG AsULONG;
} WHEA_ERROR_RECORD_SECTION_DESCRIPTOR_FLAGS,
    *PWHEA_ERROR_RECORD_SECTION_DESCRIPTOR_FLAGS;

#define WHEA_SECTION_DESCRIPTOR_FLAGS_PRIMARY            0x00000001
#define WHEA_SECTION_DESCRIPTOR_FLAGS_CONTAINMENTWRN     0x00000002
#define WHEA_SECTION_DESCRIPTOR_FLAGS_RESET              0x00000004
#define WHEA_SECTION_DESCRIPTOR_FLAGS_THRESHOLDEXCEEDED  0x00000008
#define WHEA_SECTION_DESCRIPTOR_FLAGS_RESOURCENA         0x00000010
#define WHEA_SECTION_DESCRIPTOR_FLAGS_LATENTERROR        0x00000020
#define WHEA_SECTION_DESCRIPTOR_FLAGS_PROPAGATED         0x00000040

typedef union _WHEA_ERROR_RECORD_SECTION_DESCRIPTOR_VALIDBITS {
    struct {
        UCHAR FRUId:1;
        UCHAR FRUText:1;
        UCHAR Reserved:6;
    } DUMMYSTRUCTNAME;
    UCHAR AsUCHAR;
} WHEA_ERROR_RECORD_SECTION_DESCRIPTOR_VALIDBITS,
    *PWHEA_ERROR_RECORD_SECTION_DESCRIPTOR_VALIDBITS;

typedef struct _WHEA_ERROR_RECORD_SECTION_DESCRIPTOR {
    ULONG SectionOffset;
    ULONG SectionLength;
    WHEA_REVISION Revision;
    WHEA_ERROR_RECORD_SECTION_DESCRIPTOR_VALIDBITS ValidBits;
    UCHAR Reserved;
    WHEA_ERROR_RECORD_SECTION_DESCRIPTOR_FLAGS Flags;
    GUID SectionType;
    GUID FRUId;
    WHEA_ERROR_SEVERITY SectionSeverity;
    CCHAR FRUText[20];
} WHEA_ERROR_RECORD_SECTION_DESCRIPTOR, *PWHEA_ERROR_RECORD_SECTION_DESCRIPTOR;

#define WHEA_ERROR_RECORD_SECTION_DESCRIPTOR_REVISION   0x0300

#if WHEA_DOWNLEVEL_TYPE_NAMES

#define WHEA_SECTION_DESCRIPTOR_REVISION \
    WHEA_ERROR_RECORD_SECTION_DESCRIPTOR_REVISION

#endif

//
// Validate the error record section descriptor structure against the
// definitions in the UEFI specification.
//

CPER_FIELD_CHECK(WHEA_ERROR_RECORD_SECTION_DESCRIPTOR, SectionOffset,    0,  4);
CPER_FIELD_CHECK(WHEA_ERROR_RECORD_SECTION_DESCRIPTOR, SectionLength,    4,  4);
CPER_FIELD_CHECK(WHEA_ERROR_RECORD_SECTION_DESCRIPTOR, Revision,         8,  2);
CPER_FIELD_CHECK(WHEA_ERROR_RECORD_SECTION_DESCRIPTOR, ValidBits,       10,  1);
CPER_FIELD_CHECK(WHEA_ERROR_RECORD_SECTION_DESCRIPTOR, Reserved,        11,  1);
CPER_FIELD_CHECK(WHEA_ERROR_RECORD_SECTION_DESCRIPTOR, Flags,           12,  4);
CPER_FIELD_CHECK(WHEA_ERROR_RECORD_SECTION_DESCRIPTOR, SectionType,     16, 16);
CPER_FIELD_CHECK(WHEA_ERROR_RECORD_SECTION_DESCRIPTOR, FRUId,           32, 16);
CPER_FIELD_CHECK(WHEA_ERROR_RECORD_SECTION_DESCRIPTOR, SectionSeverity, 48,  4);
CPER_FIELD_CHECK(WHEA_ERROR_RECORD_SECTION_DESCRIPTOR, FRUText,         52, 20);

//----------------------------------------------------------- WHEA_ERROR_RECORD

typedef _Struct_size_bytes_(Header.Length) struct _WHEA_ERROR_RECORD {
    WHEA_ERROR_RECORD_HEADER Header;
    _Field_size_(Header.SectionCount)
        WHEA_ERROR_RECORD_SECTION_DESCRIPTOR SectionDescriptor[ANYSIZE_ARRAY];
} WHEA_ERROR_RECORD, *PWHEA_ERROR_RECORD;

//
// Validate the error record structure against the definitions in the UEFI
// specification.
//

CPER_FIELD_CHECK(WHEA_ERROR_RECORD, Header,              0,  128);
CPER_FIELD_CHECK(WHEA_ERROR_RECORD, SectionDescriptor, 128,   72);

//---------------------------------------- WHEA_PROCESSOR_GENERIC_ERROR_SECTION

#define GENPROC_PROCTYPE_XPF                 0
#define GENPROC_PROCTYPE_IPF                 1
#define GENPROC_PROCTYPE_ARM                 2

#define GENPROC_PROCISA_X86                  0
#define GENPROC_PROCISA_IPF                  1
#define GENPROC_PROCISA_X64                  2
#define GENPROC_PROCISA_ARM32                4
#define GENPROC_PROCISA_ARM64                8

#define GENPROC_PROCERRTYPE_UNKNOWN          0
#define GENPROC_PROCERRTYPE_CACHE            1
#define GENPROC_PROCERRTYPE_TLB              2
#define GENPROC_PROCERRTYPE_BUS              4
#define GENPROC_PROCERRTYPE_MAE              8

#define GENPROC_OP_GENERIC                   0
#define GENPROC_OP_DATAREAD                  1
#define GENPROC_OP_DATAWRITE                 2
#define GENPROC_OP_INSTRUCTIONEXE            3

#define GENPROC_FLAGS_RESTARTABLE            0x01
#define GENPROC_FLAGS_PRECISEIP              0x02
#define GENPROC_FLAGS_OVERFLOW               0x04
#define GENPROC_FLAGS_CORRECTED              0x08

typedef union _WHEA_PROCESSOR_FAMILY_INFO {
    struct {
        ULONG Stepping:4;
        ULONG Model:4;
        ULONG Family:4;
        ULONG ProcessorType:2;
        ULONG Reserved1:2;
        ULONG ExtendedModel:4;
        ULONG ExtendedFamily:8;
        ULONG Reserved2:4;
        ULONG Reserved3;
    } DUMMYSTRUCTNAME;
    ULONGLONG AsULONGLONG;
} WHEA_PROCESSOR_FAMILY_INFO, *PWHEA_PROCESSOR_FAMILY_INFO;

typedef union _WHEA_PROCESSOR_GENERIC_ERROR_SECTION_VALIDBITS {
    struct {
        ULONGLONG ProcessorType:1;
        ULONGLONG InstructionSet:1;
        ULONGLONG ErrorType:1;
        ULONGLONG Operation:1;
        ULONGLONG Flags:1;
        ULONGLONG Level:1;
        ULONGLONG CPUVersion:1;
        ULONGLONG CPUBrandString:1;
        ULONGLONG ProcessorId:1;
        ULONGLONG TargetAddress:1;
        ULONGLONG RequesterId:1;
        ULONGLONG ResponderId:1;
        ULONGLONG InstructionPointer:1;
        ULONGLONG Reserved:51;
    } DUMMYSTRUCTNAME;
    ULONGLONG ValidBits;
} WHEA_PROCESSOR_GENERIC_ERROR_SECTION_VALIDBITS,
  *PWHEA_PROCESSOR_GENERIC_ERROR_SECTION_VALIDBITS;

typedef struct _WHEA_PROCESSOR_GENERIC_ERROR_SECTION {
    WHEA_PROCESSOR_GENERIC_ERROR_SECTION_VALIDBITS ValidBits;
    UCHAR ProcessorType;
    UCHAR InstructionSet;
    UCHAR ErrorType;
    UCHAR Operation;
    UCHAR Flags;
    UCHAR Level;
    USHORT Reserved;
    ULONGLONG CPUVersion;
    UCHAR CPUBrandString[128];
    ULONGLONG ProcessorId;
    ULONGLONG TargetAddress;
    ULONGLONG RequesterId;
    ULONGLONG ResponderId;
    ULONGLONG InstructionPointer;
} WHEA_PROCESSOR_GENERIC_ERROR_SECTION, *PWHEA_PROCESSOR_GENERIC_ERROR_SECTION;

//
// Define alternate type name for downlevel source compatibility.
//

#if WHEA_DOWNLEVEL_TYPE_NAMES

typedef WHEA_PROCESSOR_GENERIC_ERROR_SECTION_VALIDBITS
    WHEA_GENERIC_PROCESSOR_ERROR_VALIDBITS,
    *PWHEA_GENERIC_PROCESSOR_ERROR_VALIDBITS;

typedef WHEA_PROCESSOR_GENERIC_ERROR_SECTION
    WHEA_GENERIC_PROCESSOR_ERROR, *PWHEA_GENERIC_PROCESSOR_ERROR;

#endif

//
// Validate the processor generic error section structure against the
// definitions in the UEFI  specification.
//

CPER_FIELD_CHECK(WHEA_PROCESSOR_GENERIC_ERROR_SECTION, ValidBits,            0,   8);
CPER_FIELD_CHECK(WHEA_PROCESSOR_GENERIC_ERROR_SECTION, ProcessorType,        8,   1);
CPER_FIELD_CHECK(WHEA_PROCESSOR_GENERIC_ERROR_SECTION, InstructionSet,       9,   1);
CPER_FIELD_CHECK(WHEA_PROCESSOR_GENERIC_ERROR_SECTION, ErrorType,           10,   1);
CPER_FIELD_CHECK(WHEA_PROCESSOR_GENERIC_ERROR_SECTION, Operation,           11,   1);
CPER_FIELD_CHECK(WHEA_PROCESSOR_GENERIC_ERROR_SECTION, Flags,               12,   1);
CPER_FIELD_CHECK(WHEA_PROCESSOR_GENERIC_ERROR_SECTION, Level,               13,   1);
CPER_FIELD_CHECK(WHEA_PROCESSOR_GENERIC_ERROR_SECTION, Reserved,            14,   2);
CPER_FIELD_CHECK(WHEA_PROCESSOR_GENERIC_ERROR_SECTION, CPUVersion,          16,   8);
CPER_FIELD_CHECK(WHEA_PROCESSOR_GENERIC_ERROR_SECTION, CPUBrandString,      24, 128);
CPER_FIELD_CHECK(WHEA_PROCESSOR_GENERIC_ERROR_SECTION, ProcessorId,        152,   8);
CPER_FIELD_CHECK(WHEA_PROCESSOR_GENERIC_ERROR_SECTION, TargetAddress,      160,   8);
CPER_FIELD_CHECK(WHEA_PROCESSOR_GENERIC_ERROR_SECTION, RequesterId,        168,   8);
CPER_FIELD_CHECK(WHEA_PROCESSOR_GENERIC_ERROR_SECTION, ResponderId,        176,   8);
CPER_FIELD_CHECK(WHEA_PROCESSOR_GENERIC_ERROR_SECTION, InstructionPointer, 184,   8);

//-------------------------------------------- WHEA_XPF_PROCESSOR_ERROR_SECTION

//
// x86/x64 cache check structure.
//

#define XPF_CACHE_CHECK_TRANSACTIONTYPE_INSTRUCTION     0
#define XPF_CACHE_CHECK_TRANSACTIONTYPE_DATAACCESS      1
#define XPF_CACHE_CHECK_TRANSACTIONTYPE_GENERIC         2

#define XPF_CACHE_CHECK_OPERATION_GENERIC               0
#define XPF_CACHE_CHECK_OPERATION_GENREAD               1
#define XPF_CACHE_CHECK_OPERATION_GENWRITE              2
#define XPF_CACHE_CHECK_OPERATION_DATAREAD              3
#define XPF_CACHE_CHECK_OPERATION_DATAWRITE             4
#define XPF_CACHE_CHECK_OPERATION_INSTRUCTIONFETCH      5
#define XPF_CACHE_CHECK_OPERATION_PREFETCH              6
#define XPF_CACHE_CHECK_OPERATION_EVICTION              7
#define XPF_CACHE_CHECK_OPERATION_SNOOP                 8

typedef union _WHEA_XPF_CACHE_CHECK {
    struct {
        ULONGLONG TransactionTypeValid:1;
        ULONGLONG OperationValid:1;
        ULONGLONG LevelValid:1;
        ULONGLONG ProcessorContextCorruptValid:1;
        ULONGLONG UncorrectedValid:1;
        ULONGLONG PreciseIPValid:1;
        ULONGLONG RestartableIPValid:1;
        ULONGLONG OverflowValid:1;
        ULONGLONG ReservedValid:8;

        ULONGLONG TransactionType:2;
        ULONGLONG Operation:4;
        ULONGLONG Level:3;
        ULONGLONG ProcessorContextCorrupt:1;
        ULONGLONG Uncorrected:1;
        ULONGLONG PreciseIP:1;
        ULONGLONG RestartableIP:1;
        ULONGLONG Overflow:1;

        ULONGLONG Reserved:34;
    } DUMMYSTRUCTNAME;
    ULONGLONG XpfCacheCheck;
} WHEA_XPF_CACHE_CHECK, *PWHEA_XPF_CACHE_CHECK;

//
// x86/x64 TLB check structure.
//

#define XPF_TLB_CHECK_TRANSACTIONTYPE_INSTRUCTION     0
#define XPF_TLB_CHECK_TRANSACTIONTYPE_DATAACCESS      1
#define XPF_TLB_CHECK_TRANSACTIONTYPE_GENERIC         2

#define XPF_TLB_CHECK_OPERATION_GENERIC               0
#define XPF_TLB_CHECK_OPERATION_GENREAD               1
#define XPF_TLB_CHECK_OPERATION_GENWRITE              2
#define XPF_TLB_CHECK_OPERATION_DATAREAD              3
#define XPF_TLB_CHECK_OPERATION_DATAWRITE             4
#define XPF_TLB_CHECK_OPERATION_INSTRUCTIONFETCH      5
#define XPF_TLB_CHECK_OPERATION_PREFETCH              6

typedef union _WHEA_XPF_TLB_CHECK {
    struct {
        ULONGLONG TransactionTypeValid:1;
        ULONGLONG OperationValid:1;
        ULONGLONG LevelValid:1;
        ULONGLONG ProcessorContextCorruptValid:1;
        ULONGLONG UncorrectedValid:1;
        ULONGLONG PreciseIPValid:1;
        ULONGLONG RestartableIPValid:1;
        ULONGLONG OverflowValid:1;
        ULONGLONG ReservedValid:8;

        ULONGLONG TransactionType:2;
        ULONGLONG Operation:4;
        ULONGLONG Level:3;
        ULONGLONG ProcessorContextCorrupt:1;
        ULONGLONG Uncorrected:1;
        ULONGLONG PreciseIP:1;
        ULONGLONG RestartableIP:1;
        ULONGLONG Overflow:1;
        ULONGLONG Reserved:34;
    } DUMMYSTRUCTNAME;
    ULONGLONG XpfTLBCheck;
} WHEA_XPF_TLB_CHECK, *PWHEA_XPF_TLB_CHECK;

//
// x86/x64 bus check structure.
//

#define XPF_BUS_CHECK_TRANSACTIONTYPE_INSTRUCTION     0
#define XPF_BUS_CHECK_TRANSACTIONTYPE_DATAACCESS      1
#define XPF_BUS_CHECK_TRANSACTIONTYPE_GENERIC         2

#define XPF_BUS_CHECK_OPERATION_GENERIC               0
#define XPF_BUS_CHECK_OPERATION_GENREAD               1
#define XPF_BUS_CHECK_OPERATION_GENWRITE              2
#define XPF_BUS_CHECK_OPERATION_DATAREAD              3
#define XPF_BUS_CHECK_OPERATION_DATAWRITE             4
#define XPF_BUS_CHECK_OPERATION_INSTRUCTIONFETCH      5
#define XPF_BUS_CHECK_OPERATION_PREFETCH              6

#define XPF_BUS_CHECK_PARTICIPATION_PROCORIGINATED    0
#define XPF_BUS_CHECK_PARTICIPATION_PROCRESPONDED     1
#define XPF_BUS_CHECK_PARTICIPATION_PROCOBSERVED      2
#define XPF_BUS_CHECK_PARTICIPATION_GENERIC           3

#define XPF_BUS_CHECK_ADDRESS_MEMORY                  0
#define XPF_BUS_CHECK_ADDRESS_RESERVED                1
#define XPF_BUS_CHECK_ADDRESS_IO                      2
#define XPF_BUS_CHECK_ADDRESS_OTHER                   3

typedef union _WHEA_XPF_BUS_CHECK {
    struct {
        ULONGLONG TransactionTypeValid:1;
        ULONGLONG OperationValid:1;
        ULONGLONG LevelValid:1;
        ULONGLONG ProcessorContextCorruptValid:1;
        ULONGLONG UncorrectedValid:1;
        ULONGLONG PreciseIPValid:1;
        ULONGLONG RestartableIPValid:1;
        ULONGLONG OverflowValid:1;
        ULONGLONG ParticipationValid:1;
        ULONGLONG TimeoutValid:1;
        ULONGLONG AddressSpaceValid:1;
        ULONGLONG ReservedValid:5;

        ULONGLONG TransactionType:2;
        ULONGLONG Operation:4;
        ULONGLONG Level:3;
        ULONGLONG ProcessorContextCorrupt:1;
        ULONGLONG Uncorrected:1;
        ULONGLONG PreciseIP:1;
        ULONGLONG RestartableIP:1;
        ULONGLONG Overflow:1;
        ULONGLONG Participation:2;
        ULONGLONG Timeout:1;
        ULONGLONG AddressSpace:2;
        ULONGLONG Reserved:29;
    } DUMMYSTRUCTNAME;
    ULONGLONG XpfBusCheck;
} WHEA_XPF_BUS_CHECK, *PWHEA_XPF_BUS_CHECK;

//
// x86/x64 micro-architecture specific check structure.
//

#define XPF_MS_CHECK_ERRORTYPE_NOERROR               0
#define XPF_MS_CHECK_ERRORTYPE_UNCLASSIFIED          1
#define XPF_MS_CHECK_ERRORTYPE_MCROMPARITY           2
#define XPF_MS_CHECK_ERRORTYPE_EXTERNAL              3
#define XPF_MS_CHECK_ERRORTYPE_FRC                   4
#define XPF_MS_CHECK_ERRORTYPE_INTERNALUNCLASSIFIED  5

typedef union _WHEA_XPF_MS_CHECK {
    struct {
        ULONGLONG ErrorTypeValid:1;
        ULONGLONG ProcessorContextCorruptValid:1;
        ULONGLONG UncorrectedValid:1;
        ULONGLONG PreciseIPValid:1;
        ULONGLONG RestartableIPValid:1;
        ULONGLONG OverflowValid:1;
        ULONGLONG ReservedValue:10;

        ULONGLONG ErrorType:3;
        ULONGLONG ProcessorContextCorrupt:1;
        ULONGLONG Uncorrected:1;
        ULONGLONG PreciseIP:1;
        ULONGLONG RestartableIP:1;
        ULONGLONG Overflow:1;
        ULONGLONG Reserved:40;
    } DUMMYSTRUCTNAME;
    ULONGLONG XpfMsCheck;
} WHEA_XPF_MS_CHECK, *PWHEA_XPF_MS_CHECK;

//
// x86/x64 Processor Error Information Structure.
//

typedef union _WHEA_XPF_PROCINFO_VALIDBITS {
    struct {
        ULONGLONG CheckInfo:1;
        ULONGLONG TargetId:1;
        ULONGLONG RequesterId:1;
        ULONGLONG ResponderId:1;
        ULONGLONG InstructionPointer:1;
        ULONGLONG Reserved:59;
    } DUMMYSTRUCTNAME;
    ULONGLONG ValidBits;
} WHEA_XPF_PROCINFO_VALIDBITS, *PWHEA_XPF_PROCINFO_VALIDBITS;

typedef struct _WHEA_XPF_PROCINFO {
    GUID CheckInfoId;
    WHEA_XPF_PROCINFO_VALIDBITS ValidBits;
    union {
        WHEA_XPF_CACHE_CHECK CacheCheck;
        WHEA_XPF_TLB_CHECK TlbCheck;
        WHEA_XPF_BUS_CHECK BusCheck;
        WHEA_XPF_MS_CHECK MsCheck;
        ULONGLONG AsULONGLONG;
    } CheckInfo;
    ULONGLONG TargetId;
    ULONGLONG RequesterId;
    ULONGLONG ResponderId;
    ULONGLONG InstructionPointer;
} WHEA_XPF_PROCINFO, *PWHEA_XPF_PROCINFO;

//
// x86/x64 Processor Context Information Structure.
//

typedef struct _WHEA_X86_REGISTER_STATE {
    ULONG Eax;
    ULONG Ebx;
    ULONG Ecx;
    ULONG Edx;
    ULONG Esi;
    ULONG Edi;
    ULONG Ebp;
    ULONG Esp;
    USHORT Cs;
    USHORT Ds;
    USHORT Ss;
    USHORT Es;
    USHORT Fs;
    USHORT Gs;
    ULONG Eflags;
    ULONG Eip;
    ULONG Cr0;
    ULONG Cr1;
    ULONG Cr2;
    ULONG Cr3;
    ULONG Cr4;
    ULONGLONG Gdtr;
    ULONGLONG Idtr;
    USHORT Ldtr;
    USHORT Tr;
} WHEA_X86_REGISTER_STATE, *PWHEA_X86_REGISTER_STATE;

typedef struct DECLSPEC_ALIGN(16) _WHEA128A {
    ULONGLONG Low;
    LONGLONG High;
} WHEA128A, *PWHEA128A;

#if defined(_MSC_VER)
#if (_MSC_VER >= 1200)
#pragma warning(push)
#pragma warning(disable:4324) // structure padded due to __declspec(align())
#endif
#endif

typedef struct _WHEA_X64_REGISTER_STATE {
    ULONGLONG Rax;
    ULONGLONG Rbx;
    ULONGLONG Rcx;
    ULONGLONG Rdx;
    ULONGLONG Rsi;
    ULONGLONG Rdi;
    ULONGLONG Rbp;
    ULONGLONG Rsp;
    ULONGLONG R8;
    ULONGLONG R9;
    ULONGLONG R10;
    ULONGLONG R11;
    ULONGLONG R12;
    ULONGLONG R13;
    ULONGLONG R14;
    ULONGLONG R15;
    USHORT Cs;
    USHORT Ds;
    USHORT Ss;
    USHORT Es;
    USHORT Fs;
    USHORT Gs;
    ULONG Reserved;
    ULONGLONG Rflags;
    ULONGLONG Eip;
    ULONGLONG Cr0;
    ULONGLONG Cr1;
    ULONGLONG Cr2;
    ULONGLONG Cr3;
    ULONGLONG Cr4;
    ULONGLONG Cr8;
    WHEA128A Gdtr;
    WHEA128A Idtr;
    USHORT Ldtr;
    USHORT Tr;
} WHEA_X64_REGISTER_STATE, *PWHEA_X64_REGISTER_STATE;

#if defined(_MSC_VER)
#if (_MSC_VER >= 1200)
#pragma warning(pop)
#endif
#endif

#define XPF_CONTEXT_INFO_UNCLASSIFIEDDATA       0
#define XPF_CONTEXT_INFO_MSRREGISTERS           1
#define XPF_CONTEXT_INFO_32BITCONTEXT           2
#define XPF_CONTEXT_INFO_64BITCONTEXT           3
#define XPF_CONTEXT_INFO_FXSAVE                 4
#define XPF_CONTEXT_INFO_32BITDEBUGREGS         5
#define XPF_CONTEXT_INFO_64BITDEBUGREGS         6
#define XPF_CONTEXT_INFO_MMREGISTERS            7

typedef struct _WHEA_XPF_CONTEXT_INFO {
    USHORT RegisterContextType;
    USHORT RegisterDataSize;
    ULONG MSRAddress;
    ULONGLONG MmRegisterAddress;

    //
    // UCHAR RegisterData[ANYSIZE_ARRAY];
    //

} WHEA_XPF_CONTEXT_INFO, *PWHEA_XPF_CONTEXT_INFO;

//
// x86/x64 Processor Error Section
//

typedef union _WHEA_XPF_PROCESSOR_ERROR_SECTION_VALIDBITS {
    struct {
        ULONGLONG LocalAPICId:1;
        ULONGLONG CpuId:1;
        ULONGLONG ProcInfoCount:6;
        ULONGLONG ContextInfoCount:6;
        ULONGLONG Reserved:50;
    } DUMMYSTRUCTNAME;
    ULONGLONG ValidBits;
} WHEA_XPF_PROCESSOR_ERROR_SECTION_VALIDBITS,
  *PWHEA_XPF_PROCESSOR_ERROR_SECTION_VALIDBITS;

typedef struct _WHEA_XPF_PROCESSOR_ERROR_SECTION {
    WHEA_XPF_PROCESSOR_ERROR_SECTION_VALIDBITS ValidBits;
    ULONGLONG LocalAPICId;
    UCHAR CpuId[48];

    //
    // WHEA_XPF_PROCINFO ProcInfo[ANYSIZE_ARRAY];
    // WHEA_XPF_CONTEXT_INFO ContextInfo[ANYSIZE_ARRAY];
    //

    UCHAR VariableInfo[ANYSIZE_ARRAY];
} WHEA_XPF_PROCESSOR_ERROR_SECTION, *PWHEA_XPF_PROCESSOR_ERROR_SECTION;

//
// Define alternate type names for downlevel source compatibility.
//

#if WHEA_DOWNLEVEL_TYPE_NAMES

typedef struct WHEA_XPF_PROCESSOR_ERROR_SECTION_VALIDBITS
    WHEA_XPF_PROCESSOR_ERROR_VALIDBITS, *PWHEA_XPF_PROCESSOR_ERROR_VALIDBITS;

typedef struct WHEA_XPF_PROCESSOR_ERROR_SECTION
    WHEA_XPF_PROCESSOR_ERROR, *PWHEA_XPF_PROCESSOR_ERROR;

#endif

//
// Validate the x86/x64 processor error section structures against the
// definitions in the UEFI  specification.
//

CPER_FIELD_CHECK(WHEA_XPF_PROCINFO, CheckInfoId,         0, 16);
CPER_FIELD_CHECK(WHEA_XPF_PROCINFO, ValidBits,          16,  8);
CPER_FIELD_CHECK(WHEA_XPF_PROCINFO, CheckInfo,          24,  8);
CPER_FIELD_CHECK(WHEA_XPF_PROCINFO, TargetId,           32,  8);
CPER_FIELD_CHECK(WHEA_XPF_PROCINFO, RequesterId,        40,  8);
CPER_FIELD_CHECK(WHEA_XPF_PROCINFO, ResponderId,        48,  8);
CPER_FIELD_CHECK(WHEA_XPF_PROCINFO, InstructionPointer, 56,  8);

CPER_FIELD_CHECK(WHEA_X86_REGISTER_STATE, Eax,       0,   4);
CPER_FIELD_CHECK(WHEA_X86_REGISTER_STATE, Ebx,       4,   4);
CPER_FIELD_CHECK(WHEA_X86_REGISTER_STATE, Ecx,       8,   4);
CPER_FIELD_CHECK(WHEA_X86_REGISTER_STATE, Edx,      12,   4);
CPER_FIELD_CHECK(WHEA_X86_REGISTER_STATE, Esi,      16,   4);
CPER_FIELD_CHECK(WHEA_X86_REGISTER_STATE, Edi,      20,   4);
CPER_FIELD_CHECK(WHEA_X86_REGISTER_STATE, Ebp,      24,   4);
CPER_FIELD_CHECK(WHEA_X86_REGISTER_STATE, Esp,      28,   4);
CPER_FIELD_CHECK(WHEA_X86_REGISTER_STATE, Cs,       32,   2);
CPER_FIELD_CHECK(WHEA_X86_REGISTER_STATE, Ds,       34,   2);
CPER_FIELD_CHECK(WHEA_X86_REGISTER_STATE, Ss,       36,   2);
CPER_FIELD_CHECK(WHEA_X86_REGISTER_STATE, Es,       38,   2);
CPER_FIELD_CHECK(WHEA_X86_REGISTER_STATE, Fs,       40,   2);
CPER_FIELD_CHECK(WHEA_X86_REGISTER_STATE, Gs,       42,   2);
CPER_FIELD_CHECK(WHEA_X86_REGISTER_STATE, Eflags,   44,   4);
CPER_FIELD_CHECK(WHEA_X86_REGISTER_STATE, Eip,      48,   4);
CPER_FIELD_CHECK(WHEA_X86_REGISTER_STATE, Cr0,      52,   4);
CPER_FIELD_CHECK(WHEA_X86_REGISTER_STATE, Cr1,      56,   4);
CPER_FIELD_CHECK(WHEA_X86_REGISTER_STATE, Cr2,      60,   4);
CPER_FIELD_CHECK(WHEA_X86_REGISTER_STATE, Cr3,      64,   4);
CPER_FIELD_CHECK(WHEA_X86_REGISTER_STATE, Cr4,      68,   4);
CPER_FIELD_CHECK(WHEA_X86_REGISTER_STATE, Gdtr,     72,   8);
CPER_FIELD_CHECK(WHEA_X86_REGISTER_STATE, Idtr,     80,   8);
CPER_FIELD_CHECK(WHEA_X86_REGISTER_STATE, Ldtr,     88,   2);
CPER_FIELD_CHECK(WHEA_X86_REGISTER_STATE, Tr,       90,   2);

CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, Rax,       0,   8);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, Rbx,       8,   8);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, Rcx,      16,   8);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, Rdx,      24,   8);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, Rsi,      32,   8);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, Rdi,      40,   8);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, Rbp,      48,   8);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, Rsp,      56,   8);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, R8,       64,   8);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, R9,       72,   8);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, R10,      80,   8);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, R11,      88,   8);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, R12,      96,   8);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, R13,      104,  8);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, R14,      112,  8);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, R15,      120,  8);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, Cs,       128,  2);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, Ds,       130,  2);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, Ss,       132,  2);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, Es,       134,  2);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, Fs,       136,  2);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, Gs,       138,  2);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, Reserved, 140,  4);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, Rflags,   144,  8);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, Eip,      152,  8);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, Cr0,      160,  8);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, Cr1,      168,  8);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, Cr2,      176,  8);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, Cr3,      184,  8);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, Cr4,      192,  8);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, Cr8,      200,  8);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, Gdtr,     208, 16);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, Idtr,     224, 16);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, Ldtr,     240,  2);
CPER_FIELD_CHECK(WHEA_X64_REGISTER_STATE, Tr,       242,  2);

CPER_FIELD_CHECK(WHEA_XPF_CONTEXT_INFO, RegisterContextType,  0, 2);
CPER_FIELD_CHECK(WHEA_XPF_CONTEXT_INFO, RegisterDataSize,     2, 2);
CPER_FIELD_CHECK(WHEA_XPF_CONTEXT_INFO, MSRAddress,           4, 4);
CPER_FIELD_CHECK(WHEA_XPF_CONTEXT_INFO, MmRegisterAddress,    8, 8);

CPER_FIELD_CHECK(WHEA_XPF_PROCESSOR_ERROR_SECTION, ValidBits,     0,  8);
CPER_FIELD_CHECK(WHEA_XPF_PROCESSOR_ERROR_SECTION, LocalAPICId,   8,  8);
CPER_FIELD_CHECK(WHEA_XPF_PROCESSOR_ERROR_SECTION, CpuId,        16, 48);
CPER_FIELD_CHECK(WHEA_XPF_PROCESSOR_ERROR_SECTION, VariableInfo, 64, ANYSIZE_ARRAY);

//--------------------------------------------------- WHEA_MEMORY_ERROR_SECTION

typedef union _WHEA_MEMORY_ERROR_SECTION_VALIDBITS {
    struct {
        ULONGLONG ErrorStatus:1;
        ULONGLONG PhysicalAddress:1;
        ULONGLONG PhysicalAddressMask:1;
        ULONGLONG Node:1;
        ULONGLONG Card:1;
        ULONGLONG Module:1;
        ULONGLONG Bank:1;
        ULONGLONG Device:1;
        ULONGLONG Row:1;
        ULONGLONG Column:1;
        ULONGLONG BitPosition:1;
        ULONGLONG RequesterId:1;
        ULONGLONG ResponderId:1;
        ULONGLONG TargetId:1;
        ULONGLONG ErrorType:1;
        ULONGLONG RankNumber:1;
        ULONGLONG CardHandle:1;
        ULONGLONG ModuleHandle:1;
        ULONGLONG ExtendedRow:1;
        ULONGLONG BankGroup:1;
        ULONGLONG BankAddress:1;
        ULONGLONG ChipIdentification:1;
        ULONGLONG Reserved:42;
    } DUMMYSTRUCTNAME;
    ULONGLONG ValidBits;
} WHEA_MEMORY_ERROR_SECTION_VALIDBITS,
  *PWHEA_MEMORY_ERROR_SECTION_VALIDBITS;

#define WHEA_MEMERRTYPE_UNKNOWN                 0x00
#define WHEA_MEMERRTYPE_NOERROR                 0x01
#define WHEA_MEMERRTYPE_SINGLEBITECC            0x02
#define WHEA_MEMERRTYPE_MULTIBITECC             0x03
#define WHEA_MEMERRTYPE_SINGLESYMCHIPKILL       0x04
#define WHEA_MEMERRTYPE_MULTISYMCHIPKILL        0x05
#define WHEA_MEMERRTYPE_MASTERABORT             0x06
#define WHEA_MEMERRTYPE_TARGETABORT             0x07
#define WHEA_MEMERRTYPE_PARITYERROR             0x08
#define WHEA_MEMERRTYPE_WATCHDOGTIMEOUT         0x09
#define WHEA_MEMERRTYPE_INVALIDADDRESS          0x0A
#define WHEA_MEMERRTYPE_MIRRORBROKEN            0x0B
#define WHEA_MEMERRTYPE_MEMORYSPARING           0x0C

typedef struct _WHEA_MEMORY_ERROR_SECTION {
    WHEA_MEMORY_ERROR_SECTION_VALIDBITS ValidBits;
    WHEA_ERROR_STATUS ErrorStatus;
    ULONGLONG PhysicalAddress;
    ULONGLONG PhysicalAddressMask;
    USHORT Node;
    USHORT Card;
    USHORT Module;
    USHORT Bank;
    USHORT Device;
    USHORT Row;
    USHORT Column;
    USHORT BitPosition;
    ULONGLONG RequesterId;
    ULONGLONG ResponderId;
    ULONGLONG TargetId;
    UCHAR ErrorType;
    UCHAR Extended;
    USHORT RankNumber;
    USHORT CardHandle;
    USHORT ModuleHandle;
} WHEA_MEMORY_ERROR_SECTION, *PWHEA_MEMORY_ERROR_SECTION;

//
// Define alternate names allowing for downlevel source compatibility.
//

#if WHEA_DOWNLEVEL_TYPE_NAMES

typedef WHEA_MEMORY_ERROR_SECTION_VALIDBITS
    WHEA_MEMORY_ERROR_VALIDBITS, *PWHEA_MEMORY_ERROR_VALIDBITS;

typedef WHEA_MEMORY_ERROR_SECTION
    WHEA_MEMORY_ERROR, *PWHEA_MEMORY_ERROR;

#endif

//
// Validate the memory error section structures against the definitions in the
// UEFI  specification.
//

CPER_FIELD_CHECK(WHEA_MEMORY_ERROR_SECTION, ValidBits,            0, 8);
CPER_FIELD_CHECK(WHEA_MEMORY_ERROR_SECTION, ErrorStatus,          8, 8);
CPER_FIELD_CHECK(WHEA_MEMORY_ERROR_SECTION, PhysicalAddress,     16, 8);
CPER_FIELD_CHECK(WHEA_MEMORY_ERROR_SECTION, PhysicalAddressMask, 24, 8);
CPER_FIELD_CHECK(WHEA_MEMORY_ERROR_SECTION, Node,                32, 2);
CPER_FIELD_CHECK(WHEA_MEMORY_ERROR_SECTION, Card,                34, 2);
CPER_FIELD_CHECK(WHEA_MEMORY_ERROR_SECTION, Module,              36, 2);
CPER_FIELD_CHECK(WHEA_MEMORY_ERROR_SECTION, Bank,                38, 2);
CPER_FIELD_CHECK(WHEA_MEMORY_ERROR_SECTION, Device,              40, 2);
CPER_FIELD_CHECK(WHEA_MEMORY_ERROR_SECTION, Row,                 42, 2);
CPER_FIELD_CHECK(WHEA_MEMORY_ERROR_SECTION, Column,              44, 2);
CPER_FIELD_CHECK(WHEA_MEMORY_ERROR_SECTION, BitPosition,         46, 2);
CPER_FIELD_CHECK(WHEA_MEMORY_ERROR_SECTION, RequesterId,         48, 8);
CPER_FIELD_CHECK(WHEA_MEMORY_ERROR_SECTION, ResponderId,         56, 8);
CPER_FIELD_CHECK(WHEA_MEMORY_ERROR_SECTION, TargetId,            64, 8);
CPER_FIELD_CHECK(WHEA_MEMORY_ERROR_SECTION, ErrorType,           72, 1);

//----------------------------------------------- WHEA_PCIEXPRESS_ERROR_SECTION

typedef union _WHEA_PCIEXPRESS_ERROR_SECTION_VALIDBITS {
    struct {
        ULONGLONG PortType:1;
        ULONGLONG Version:1;
        ULONGLONG CommandStatus:1;
        ULONGLONG DeviceId:1;
        ULONGLONG DeviceSerialNumber:1;
        ULONGLONG BridgeControlStatus:1;
        ULONGLONG ExpressCapability:1;
        ULONGLONG AerInfo:1;
        ULONGLONG Reserved:56;
    } DUMMYSTRUCTNAME;
    ULONGLONG ValidBits;
} WHEA_PCIEXPRESS_ERROR_SECTION_VALIDBITS,
  *PWHEA_PCIEXPRESS_ERROR_SECTION_VALIDBITS;

typedef struct _WHEA_PCIEXPRESS_DEVICE_ID {
    USHORT VendorID;
    USHORT DeviceID;
    ULONG ClassCode:24;
    ULONG FunctionNumber:8;
    ULONG DeviceNumber:8;
    ULONG Segment:16;
    ULONG PrimaryBusNumber:8;
    ULONG SecondaryBusNumber:8;
    ULONG Reserved1:3;
    ULONG SlotNumber:13;
    ULONG Reserved2:8;
} WHEA_PCIEXPRESS_DEVICE_ID, *PWHEA_PCIEXPRESS_DEVICE_ID;

typedef union _WHEA_PCIEXPRESS_VERSION {
    struct {
        UCHAR MinorVersion;
        UCHAR MajorVersion;
        USHORT Reserved;
    } DUMMYSTRUCTNAME;
    ULONG AsULONG;
} WHEA_PCIEXPRESS_VERSION, *PWHEA_PCIEXPRESS_VERSION;

typedef union _WHEA_PCIEXPRESS_COMMAND_STATUS {
    struct {
        USHORT Command;
        USHORT Status;
    } DUMMYSTRUCTNAME;
    ULONG AsULONG;
} WHEA_PCIEXPRESS_COMMAND_STATUS, *PWHEA_PCIEXPRESS_COMMAND_STATUS;

typedef union _WHEA_PCIEXPRESS_BRIDGE_CONTROL_STATUS {
    struct {
        USHORT BridgeSecondaryStatus;
        USHORT BridgeControl;
    } DUMMYSTRUCTNAME;
    ULONG AsULONG;
} WHEA_PCIEXPRESS_BRIDGE_CONTROL_STATUS,
    *PWHEA_PCIEXPRESS_BRIDGE_CONTROL_STATUS;

typedef enum _WHEA_PCIEXPRESS_DEVICE_TYPE {
    WheaPciExpressEndpoint = 0,
    WheaPciExpressLegacyEndpoint,
    WheaPciExpressRootPort = 4,
    WheaPciExpressUpstreamSwitchPort,
    WheaPciExpressDownstreamSwitchPort,
    WheaPciExpressToPciXBridge,
    WheaPciXToExpressBridge,
    WheaPciExpressRootComplexIntegratedEndpoint,
    WheaPciExpressRootComplexEventCollector
} WHEA_PCIEXPRESS_DEVICE_TYPE;

typedef struct _WHEA_PCIEXPRESS_ERROR_SECTION {
    WHEA_PCIEXPRESS_ERROR_SECTION_VALIDBITS ValidBits;
    WHEA_PCIEXPRESS_DEVICE_TYPE PortType;
    WHEA_PCIEXPRESS_VERSION Version;
    WHEA_PCIEXPRESS_COMMAND_STATUS CommandStatus;
    ULONG Reserved;
    WHEA_PCIEXPRESS_DEVICE_ID DeviceId;
    ULONGLONG DeviceSerialNumber;
    WHEA_PCIEXPRESS_BRIDGE_CONTROL_STATUS BridgeControlStatus;
    UCHAR ExpressCapability[60];
    UCHAR AerInfo[96];
} WHEA_PCIEXPRESS_ERROR_SECTION, *PWHEA_PCIEXPRESS_ERROR_SECTION;

#if WHEA_DOWNLEVEL_TYPE_NAMES

typedef WHEA_PCIEXPRESS_ERROR_SECTION_VALIDBITS
    WHEA_PCIEXPRESS_ERROR_VALIDBITS,
    *PWHEA_PCIEXPRESS_ERROR_VALIDBITS;

typedef WHEA_PCIEXPRESS_ERROR_SECTION
    WHEA_PCIEXPRESS_ERROR, *PWHEA_PCIEXPRESS_ERROR;

#endif

//
// Validate the PCI Express error section structures against the definitions
// in the UEFI  specification.
//

CPER_FIELD_CHECK(WHEA_PCIEXPRESS_ERROR_SECTION, ValidBits,             0,  8);
CPER_FIELD_CHECK(WHEA_PCIEXPRESS_ERROR_SECTION, PortType,              8,  4);
CPER_FIELD_CHECK(WHEA_PCIEXPRESS_ERROR_SECTION, Version,              12,  4);
CPER_FIELD_CHECK(WHEA_PCIEXPRESS_ERROR_SECTION, CommandStatus,        16,  4);
CPER_FIELD_CHECK(WHEA_PCIEXPRESS_ERROR_SECTION, Reserved,             20,  4);
CPER_FIELD_CHECK(WHEA_PCIEXPRESS_ERROR_SECTION, DeviceId,             24, 16);
CPER_FIELD_CHECK(WHEA_PCIEXPRESS_ERROR_SECTION, DeviceSerialNumber,   40,  8);
CPER_FIELD_CHECK(WHEA_PCIEXPRESS_ERROR_SECTION, BridgeControlStatus,  48,  4);
CPER_FIELD_CHECK(WHEA_PCIEXPRESS_ERROR_SECTION, ExpressCapability,    52, 60);
CPER_FIELD_CHECK(WHEA_PCIEXPRESS_ERROR_SECTION, AerInfo,             112, 96);

//-------------------------------------------------- WHEA_PCIXBUS_ERROR_SECTION

#define PCIXBUS_ERRTYPE_UNKNOWN             0x0000
#define PCIXBUS_ERRTYPE_DATAPARITY          0x0001
#define PCIXBUS_ERRTYPE_SYSTEM              0x0002
#define PCIXBUS_ERRTYPE_MASTERABORT         0x0003
#define PCIXBUS_ERRTYPE_----IMEOUT          0x0004
#define PCIXBUS_ERRTYPE_MASTERDATAPARITY    0x0005
#define PCIXBUS_ERRTYPE_ADDRESSPARITY       0x0006
#define PCIXBUS_ERRTYPE_COMMANDPARITY       0x0007

typedef union _WHEA_PCIXBUS_ERROR_SECTION_VALIDBITS {
    struct {
        ULONGLONG ErrorStatus:1;
        ULONGLONG ErrorType:1;
        ULONGLONG BusId:1;
        ULONGLONG BusAddress:1;
        ULONGLONG BusData:1;
        ULONGLONG BusCommand:1;
        ULONGLONG RequesterId:1;
        ULONGLONG CompleterId:1;
        ULONGLONG TargetId:1;
        ULONGLONG Reserved:55;
    } DUMMYSTRUCTNAME;
    ULONGLONG ValidBits;
} WHEA_PCIXBUS_ERROR_SECTION_VALIDBITS, *PWHEA_PCIXBUS_ERROR_SECTION_VALIDBITS;

typedef union _WHEA_PCIXBUS_ID {
    struct {
        UCHAR BusNumber;
        UCHAR BusSegment;
    } DUMMYSTRUCTNAME;
    USHORT AsUSHORT;
} WHEA_PCIXBUS_ID, *PWHEA_PCIXBUS_ID;

typedef union _WHEA_PCIXBUS_COMMAND {
    struct {
        ULONGLONG Command:56;
        ULONGLONG PCIXCommand:1;
        ULONGLONG Reserved:7;
    } DUMMYSTRUCTNAME;
    ULONGLONG AsULONGLONG;
} WHEA_PCIXBUS_COMMAND, *PWHEA_PCIXBUS_COMMAND;

typedef struct _WHEA_PCIXBUS_ERROR_SECTION {
    WHEA_PCIXBUS_ERROR_SECTION_VALIDBITS ValidBits;
    WHEA_ERROR_STATUS ErrorStatus;
    USHORT ErrorType;
    WHEA_PCIXBUS_ID BusId;
    ULONG Reserved;
    ULONGLONG BusAddress;
    ULONGLONG BusData;
    WHEA_PCIXBUS_COMMAND BusCommand;
    ULONGLONG RequesterId;
    ULONGLONG CompleterId;
    ULONGLONG TargetId;
} WHEA_PCIXBUS_ERROR_SECTION, *PWHEA_PCIXBUS_ERROR_SECTION;

#if WHEA_DOWNLEVEL_TYPE_NAMES

typedef WHEA_PCIXBUS_ERROR_SECTION_VALIDBITS
    WHEA_PCIXBUS_ERROR_VALIDBITS,
    *PWHEA_PCIXBUS_ERROR_VALIDBITS;

typedef WHEA_PCIXBUS_ERROR_SECTION
    WHEA_PCIXBUS_ERROR, *PWHEA_PCIXBUS_ERROR;

#endif

CPER_FIELD_CHECK(WHEA_PCIXBUS_ERROR_SECTION, ValidBits,    0, 8);
CPER_FIELD_CHECK(WHEA_PCIXBUS_ERROR_SECTION, ErrorStatus,  8, 8);
CPER_FIELD_CHECK(WHEA_PCIXBUS_ERROR_SECTION, ErrorType,   16, 2);
CPER_FIELD_CHECK(WHEA_PCIXBUS_ERROR_SECTION, BusId,       18, 2);
CPER_FIELD_CHECK(WHEA_PCIXBUS_ERROR_SECTION, Reserved,    20, 4);
CPER_FIELD_CHECK(WHEA_PCIXBUS_ERROR_SECTION, BusAddress,  24, 8);
CPER_FIELD_CHECK(WHEA_PCIXBUS_ERROR_SECTION, BusData,     32, 8);
CPER_FIELD_CHECK(WHEA_PCIXBUS_ERROR_SECTION, BusCommand,  40, 8);
CPER_FIELD_CHECK(WHEA_PCIXBUS_ERROR_SECTION, RequesterId, 48, 8);
CPER_FIELD_CHECK(WHEA_PCIXBUS_ERROR_SECTION, CompleterId, 56, 8);
CPER_FIELD_CHECK(WHEA_PCIXBUS_ERROR_SECTION, TargetId,    64, 8);

//----------------------------------------------- WHEA_PCIXDEVICE_ERROR_SECTION

typedef union _WHEA_PCIXDEVICE_ERROR_SECTION_VALIDBITS {
    struct {
        ULONGLONG ErrorStatus:1;
        ULONGLONG IdInfo:1;
        ULONGLONG MemoryNumber:1;
        ULONGLONG IoNumber:1;
        ULONGLONG RegisterDataPairs:1;
        ULONGLONG Reserved:59;
    } DUMMYSTRUCTNAME;
    ULONGLONG ValidBits;
} WHEA_PCIXDEVICE_ERROR_SECTION_VALIDBITS,
  *PWHEA_PCIXDEVICE_ERROR_SECTION_VALIDBITS;

typedef struct _WHEA_PCIXDEVICE_ID {
    USHORT VendorId;
    USHORT DeviceId;
    ULONG ClassCode:24;
    ULONG FunctionNumber:8;
    ULONG DeviceNumber:8;
    ULONG BusNumber:8;
    ULONG SegmentNumber:8;
    ULONG Reserved1:8;
    ULONG Reserved2;
} WHEA_PCIXDEVICE_ID, *PWHEA_PCIXDEVICE_ID;

typedef struct WHEA_PCIXDEVICE_REGISTER_PAIR {
    ULONGLONG Register;
    ULONGLONG Data;
} WHEA_PCIXDEVICE_REGISTER_PAIR, *PWHEA_PCIXDEVICE_REGISTER_PAIR;

typedef struct _WHEA_PCIXDEVICE_ERROR_SECTION {
    WHEA_PCIXDEVICE_ERROR_SECTION_VALIDBITS ValidBits;
    WHEA_ERROR_STATUS ErrorStatus;
    WHEA_PCIXDEVICE_ID IdInfo;
    ULONG MemoryNumber;
    ULONG IoNumber;
    WHEA_PCIXDEVICE_REGISTER_PAIR RegisterDataPairs[ANYSIZE_ARRAY];
} WHEA_PCIXDEVICE_ERROR_SECTION, *PWHEA_PCIXDEVICE_ERROR_SECTION;

#if WHEA_DOWNLEVEL_TYPE_NAMES

typedef WHEA_PCIXDEVICE_ERROR_SECTION_VALIDBITS
    WHEA_PCIXDEVICE_ERROR_VALIDBITS, *PWHEA_PCIXDEVICE_ERROR_VALIDBITS;

typedef WHEA_PCIXDEVICE_ERROR_SECTION
    WHEA_PCIXDEVICE_ERROR, *PWHEA_PCIXDEVICE_ERROR;

#endif

CPER_FIELD_CHECK(WHEA_PCIXDEVICE_ERROR_SECTION, ValidBits,          0,  8);
CPER_FIELD_CHECK(WHEA_PCIXDEVICE_ERROR_SECTION, ErrorStatus,        8,  8);
CPER_FIELD_CHECK(WHEA_PCIXDEVICE_ERROR_SECTION, IdInfo,            16, 16);
CPER_FIELD_CHECK(WHEA_PCIXDEVICE_ERROR_SECTION, MemoryNumber,      32,  4);
CPER_FIELD_CHECK(WHEA_PCIXDEVICE_ERROR_SECTION, IoNumber,          36,  4);
CPER_FIELD_CHECK(WHEA_PCIXDEVICE_ERROR_SECTION, RegisterDataPairs, 40, 16);

//---------------------------------------- WHEA_FIRMWARE_ERROR_RECORD_REFERENCE

#define WHEA_FIRMWARE_RECORD_TYPE_IPFSAL 0

typedef struct _WHEA_FIRMWARE_ERROR_RECORD_REFERENCE {
    UCHAR Type;
    UCHAR Reserved[7];
    ULONGLONG FirmwareRecordId;
} WHEA_FIRMWARE_ERROR_RECORD_REFERENCE, *PWHEA_FIRMWARE_ERROR_RECORD_REFERENCE;

#if WHEA_DOWNLEVEL_TYPE_NAMES

typedef WHEA_FIRMWARE_ERROR_RECORD_REFERENCE
    WHEA_FIRMWARE_RECORD, *PWHEA_FIRMWARE_RECORD;

#endif

CPER_FIELD_CHECK(WHEA_FIRMWARE_ERROR_RECORD_REFERENCE, Type,             0,  1);
CPER_FIELD_CHECK(WHEA_FIRMWARE_ERROR_RECORD_REFERENCE, Reserved,         1,  7);
CPER_FIELD_CHECK(WHEA_FIRMWARE_ERROR_RECORD_REFERENCE, FirmwareRecordId, 8,  8);

//
// This is the start of the Microsoft specific extensions to the Common Platform
// Error Record specification. This is in accordance with Appendix N, section
// 2.3 of the Unified Extensible Firmware Interface specification, which allows
// the specification of non-standard section bodies.
//

//------------------------------------------------------------- XPF_MCA_SECTION

typedef union _MCG_STATUS {
    struct {
        ULONG RestartIpValid:1;
        ULONG ErrorIpValid:1;
        ULONG MachineCheckInProgress:1;
        ULONG Reserved1:29;
        ULONG Reserved2;
    } DUMMYSTRUCTNAME;
    ULONGLONG QuadPart;
} MCG_STATUS, *PMCG_STATUS;

typedef union _MCI_STATUS {
    struct {
        USHORT McaErrorCode;
        USHORT ModelErrorCode;
        ULONG OtherInformation : 23;
        ULONG ActionRequired : 1;
        ULONG Signalling : 1;
        ULONG ContextCorrupt : 1;
        ULONG AddressValid : 1;
        ULONG MiscValid : 1;
        ULONG ErrorEnabled : 1;
        ULONG UncorrectedError : 1;
        ULONG StatusOverFlow : 1;
        ULONG Valid : 1;
    } DUMMYSTRUCTNAME;
    ULONG64 QuadPart;
} MCI_STATUS, *PMCI_STATUS;

typedef enum _WHEA_CPU_VENDOR {
    WheaCpuVendorOther = 0,
    WheaCpuVendorIntel,
    WheaCpuVendorAmd
} WHEA_CPU_VENDOR, *PWHEA_CPU_VENDOR;

#define WHEA_XPF_MCA_EXTREG_MAX_COUNT            24
#define WHEA_XPF_MCA_SECTION_VERSION_2           2
#define WHEA_XPF_MCA_SECTION_VERSION             WHEA_XPF_MCA_SECTION_VERSION_2

typedef struct _WHEA_XPF_MCA_SECTION {
    ULONG               VersionNumber;
    WHEA_CPU_VENDOR     CpuVendor;
    LARGE_INTEGER       Timestamp;
    ULONG               ProcessorNumber;
    MCG_STATUS          GlobalStatus;
    ULONGLONG           InstructionPointer;
    ULONG               BankNumber;
    MCI_STATUS          Status;
    ULONGLONG           Address;
    ULONGLONG           Misc;
    ULONG               ExtendedRegisterCount;
    ULONG               ApicId;
    ULONGLONG           ExtendedRegisters[WHEA_XPF_MCA_EXTREG_MAX_COUNT];
} WHEA_XPF_MCA_SECTION, *PWHEA_XPF_MCA_SECTION;

//------------------------------------------------------ WHEA_NMI_ERROR_SECTION

typedef union _WHEA_NMI_ERROR_SECTION_FLAGS {
    struct {
        ULONG HypervisorError:1;
        ULONG Reserved:31;
    } DUMMYSTRUCTNAME;
    ULONG AsULONG;
} WHEA_NMI_ERROR_SECTION_FLAGS, *PWHEA_NMI_ERROR_SECTION_FLAGS;

typedef struct _WHEA_NMI_ERROR_SECTION {
    UCHAR Data[8];
    WHEA_NMI_ERROR_SECTION_FLAGS Flags;
} WHEA_NMI_ERROR_SECTION, *PWHEA_NMI_ERROR_SECTION;

//------------------------------------------------------ WHEA_ARM_PROCESSOR_ERROR_SECTION

typedef union _WHEA_ARM_PROCESSOR_ERROR_SECTION_VALID_BITS {
    struct {
        ULONG MPIDR:1;
        ULONG AffinityLevel:1;
        ULONG RunningState:1;
        ULONG VendorSpecificInfo:1;
        ULONG Reserved:28;
    } DUMMYSTRUCTNAME;
    ULONG AsULONG;
} WHEA_ARM_PROCESSOR_ERROR_SECTION_VALID_BITS,
  *PWHEA_ARM_PROCESSOR_ERROR_SECTION_VALID_BITS;

typedef struct _WHEA_ARM_PROCESSOR_ERROR_SECTION {
    WHEA_ARM_PROCESSOR_ERROR_SECTION_VALID_BITS ValidBits;
    USHORT ErrorInformationStructures;
    USHORT ContextInformationStructures;
    ULONG SectionLength;
    UCHAR ErrorAffinityLevel;
    UCHAR Reserved[3];
    ULONGLONG MPIDR_EL1;
    ULONGLONG MIDR_EL1;
    ULONG RunningState;
    ULONG PSCIState;
    UCHAR Data[1];
} WHEA_ARM_PROCESSOR_ERROR_SECTION, *PWHEA_ARM_PROCESSOR_ERROR_SECTION;

CPER_FIELD_CHECK(WHEA_ARM_PROCESSOR_ERROR_SECTION, ValidBits,                    0,    4);
CPER_FIELD_CHECK(WHEA_ARM_PROCESSOR_ERROR_SECTION, ErrorInformationStructures,   4,    2);
CPER_FIELD_CHECK(WHEA_ARM_PROCESSOR_ERROR_SECTION, ContextInformationStructures, 6,    2);
CPER_FIELD_CHECK(WHEA_ARM_PROCESSOR_ERROR_SECTION, SectionLength,                8,    4);
CPER_FIELD_CHECK(WHEA_ARM_PROCESSOR_ERROR_SECTION, ErrorAffinityLevel,           12,   1);
CPER_FIELD_CHECK(WHEA_ARM_PROCESSOR_ERROR_SECTION, Reserved,                     13,   3);
CPER_FIELD_CHECK(WHEA_ARM_PROCESSOR_ERROR_SECTION, MPIDR_EL1,                    16,   8);
CPER_FIELD_CHECK(WHEA_ARM_PROCESSOR_ERROR_SECTION, MIDR_EL1,                     24,   8);
CPER_FIELD_CHECK(WHEA_ARM_PROCESSOR_ERROR_SECTION, RunningState,                 32,   4);
CPER_FIELD_CHECK(WHEA_ARM_PROCESSOR_ERROR_SECTION, PSCIState,                    36,   4);
CPER_FIELD_CHECK(WHEA_ARM_PROCESSOR_ERROR_SECTION, Data,                         40,   1);

//------------------------------------------------------ WHEA_ARM_PROCESSOR_ERROR_INFORMATION

typedef union _WHEA_ARM_PROCESSOR_ERROR_INFORMATION_VALID_BITS {
    struct {
        USHORT MultipleError:1;
        USHORT Flags:1;
        USHORT ErrorInformation:1;
        USHORT VirtualFaultAddress:1;
        USHORT PhysicalFaultAddress:1;
        USHORT Reserved:11;
    } DUMMYSTRUCTNAME;
    USHORT AsUSHORT;
} WHEA_ARM_PROCESSOR_ERROR_INFORMATION_VALID_BITS,
  *PWHEA_ARM_PROCESSOR_ERROR_INFORMATION_VALID_BITS;

typedef union _WHEA_ARM_CACHE_ERROR_VALID_BITS {
    struct {
        USHORT TransactionType:1;
        USHORT Operation:1;
        USHORT Level:1;
        USHORT ProcessorContextCorrupt:1;
        USHORT Corrected:1;
        USHORT PrecisePC:1;
        USHORT RestartablePC:1;
        USHORT Reserved:9;
    } DUMMYSTRUCTNAME;
    USHORT AsUSHORT;
} WHEA_ARM_CACHE_ERROR_VALID_BITS, *PWHEA_ARM_CACHE_ERROR_VALID_BITS;

typedef struct _WHEA_ARM_CACHE_ERROR {
    WHEA_ARM_CACHE_ERROR_VALID_BITS ValidationBit;
    UCHAR TransactionType:2;
    UCHAR Operation:4;
    UCHAR Level:3;
    UCHAR ProcessorContextCorrupt:1;
    UCHAR Corrected:1;
    UCHAR PrecisePC:1;
    UCHAR RestartablePC:1;
    ULONGLONG Reserved:35;
} WHEA_ARM_CACHE_ERROR, *PWHEA_ARM_CACHE_ERROR;

typedef union _WHEA_ARM_TLB_ERROR_VALID_BITS {
    struct {
        USHORT TransactionType:1;
        USHORT Operation:1;
        USHORT Level:1;
        USHORT ProcessorContextCorrupt:1;
        USHORT Corrected:1;
        USHORT PrecisePC:1;
        USHORT RestartablePC:1;
        USHORT Reserved:9;
    } DUMMYSTRUCTNAME;
    USHORT AsUSHORT;
} WHEA_ARM_TLB_ERROR_VALID_BITS, *PWHEA_ARM_TLB_ERROR_VALID_BITS;

typedef struct _WHEA_ARM_TLB_ERROR {
    WHEA_ARM_TLB_ERROR_VALID_BITS ValidationBit;
    UCHAR TransactionType:2;
    UCHAR Operation:4;
    UCHAR Level:3;
    UCHAR ProcessorContextCorrupt:1;
    UCHAR Corrected:1;
    UCHAR PrecisePC:1;
    UCHAR RestartablePC:1;
    ULONGLONG Reserved:36;
} WHEA_ARM_TLB_ERROR, *PWHEA_ARM_TLB_ERROR;

typedef union _WHEA_ARM_BUS_ERROR_VALID_BITS {
    struct {
        USHORT TransactionType:1;
        USHORT Operation:1;
        USHORT Level:1;
        USHORT ProcessorContextCorrupt:1;
        USHORT Corrected:1;
        USHORT PrecisePC:1;
        USHORT RestartablePC:1;
        USHORT ParticipationType:1;
        USHORT Timeout:1;
        USHORT AddressSpace:1;
        USHORT MemoryAttributes:1;
        USHORT AccessMode:1;
        USHORT Reserved:4;
    } DUMMYSTRUCTNAME;
    USHORT AsUSHORT;
} WHEA_ARM_BUS_ERROR_VALID_BITS, *PWHEA_ARM_BUS_ERROR_VALID_BITS;

typedef struct _WHEA_ARM_BUS_ERROR {
    WHEA_ARM_BUS_ERROR_VALID_BITS ValidationBit;
    UCHAR TransactionType:2;
    UCHAR Operation:4;
    UCHAR Level:3;
    UCHAR ProcessorContextCorrupt:1;
    UCHAR Corrected:1;
    UCHAR PrecisePC:1;
    UCHAR RestartablePC:1;
    UCHAR ParticipationType:2;
    UCHAR TimeOut:1;
    UCHAR AddressSpace:2;
    USHORT MemoryAccessAttributes:9;
    UCHAR AccessMode:1;
    ULONG Reserved:20;
} WHEA_ARM_BUS_ERROR, *PWHEA_ARM_BUS_ERROR;

typedef union _WHEA_ARM_PROCESSOR_ERROR {
    WHEA_ARM_CACHE_ERROR CacheError;
    WHEA_ARM_TLB_ERROR TlbError;
    WHEA_ARM_BUS_ERROR BusError;
    ULONGLONG AsULONGLONG;
} WHEA_ARM_PROCESSOR_ERROR, *PWHEA_ARM_PROCESSOR_ERROR;

typedef struct _WHEA_ARM_PROCESSOR_ERROR_INFORMATION {
    UCHAR Version;
    UCHAR Length;
    WHEA_ARM_PROCESSOR_ERROR_INFORMATION_VALID_BITS ValidationBit;
    UCHAR Type;
    USHORT MultipleError;
    UCHAR Flags;
    ULONGLONG ErrorInformation;
    ULONGLONG VirtualFaultAddress;
    ULONGLONG PhysicalFaultAddress;
} WHEA_ARM_PROCESSOR_ERROR_INFORMATION, *PWHEA_ARM_PROCESSOR_ERROR_INFORMATION;

CPER_FIELD_CHECK(WHEA_ARM_PROCESSOR_ERROR_INFORMATION, Version,                 0,   1);
CPER_FIELD_CHECK(WHEA_ARM_PROCESSOR_ERROR_INFORMATION, Length,                  1,   1);
CPER_FIELD_CHECK(WHEA_ARM_PROCESSOR_ERROR_INFORMATION, ValidationBit,           2,   2);
CPER_FIELD_CHECK(WHEA_ARM_PROCESSOR_ERROR_INFORMATION, Type,                    4,   1);
CPER_FIELD_CHECK(WHEA_ARM_PROCESSOR_ERROR_INFORMATION, MultipleError,           5,   2);
CPER_FIELD_CHECK(WHEA_ARM_PROCESSOR_ERROR_INFORMATION, Flags,                   7,   1);
CPER_FIELD_CHECK(WHEA_ARM_PROCESSOR_ERROR_INFORMATION, ErrorInformation,        8,   8);
CPER_FIELD_CHECK(WHEA_ARM_PROCESSOR_ERROR_INFORMATION, VirtualFaultAddress,    16,   8);
CPER_FIELD_CHECK(WHEA_ARM_PROCESSOR_ERROR_INFORMATION, PhysicalFaultAddress,  24,   8);

//------------------------------------------------------ WHEA_ARM_PROCESSOR_ERROR_CONTEXT_INFORMATION_HEADER

typedef union _WHEA_ARM_PROCESSOR_ERROR_CONTEXT_INFORMATION_HEADER_FLAGS {
    struct {
        ULONG ExceptionLevel:1;
        ULONG NonSecure:1;
        ULONG AArch64:1;
        ULONG Reserved:29;
    } DUMMYSTRUCTNAME;
    ULONG AsULONG;
} WHEA_ARM_PROCESSOR_ERROR_CONTEXT_INFORMATION_HEADER_FLAGS,
  *PWHEA_ARM_PROCESSOR_ERROR_CONTEXT_INFORMATION_HEADER_FLAGS;

typedef struct _WHEA_ARMV8_AARCH32_GPRS {
    ULONG R0;
    ULONG R1;
    ULONG R2;
    ULONG R3;
    ULONG R4;
    ULONG R5;
    ULONG R6;
    ULONG R7;
    ULONG R8;
    ULONG R9;
    ULONG R10;
    ULONG R11;
    ULONG R12;
    ULONG R13; // SP
    ULONG R14; // LR
    ULONG R15; // PC
} WHEA_ARMV8_AARCH32_GPRS, *PWHEA_ARMV8_AARCH32_GPRS;

typedef struct _WHEA_ARM_AARCH32_EL1_CSR {
    ULONG DFAR;
    ULONG DFSR;
    ULONG IFAR;
    ULONG ISR;
    ULONG MAIR0;
    ULONG MAIR1;
    ULONG MIDR;
    ULONG MPIDR;
    ULONG NMRR;
    ULONG PRRR;
    ULONG SCTLR; // NS
    ULONG SPSR;
    ULONG SPSR_abt;
    ULONG SPSR_fiq;
    ULONG SPSR_irq;
    ULONG SPSR_svc;
    ULONG SPSR_und;
    ULONG TPIDRPRW;
    ULONG TPIDRURO;
    ULONG TPIDRURW;
    ULONG TTBCR;
    ULONG TTBR0;
    ULONG TTBR1;
    ULONG DACR;
} WHEA_ARM_AARCH32_EL1_CSR, *PWHEA_ARM_AARCH32_EL1;

typedef struct _WHEA_ARM_AARCH32_EL2_CSR {
    ULONG ELR_hyp;
    ULONG HAMAIR0;
    ULONG HAMAIR1;
    ULONG HCR;
    ULONG HCR2;
    ULONG HDFAR;
    ULONG HIFAR;
    ULONG HPFAR;
    ULONG HSR;
    ULONG HTCR;
    ULONG HTPIDR;
    ULONG HTTBR;
    ULONG SPSR_hyp;
    ULONG VTCR;
    ULONG VTTBR;
    ULONG DACR32_EL2;
} WHEA_ARM_AARCH32_EL2_CSR, *PWHEA_ARM_AARCH32_EL2_CSR;

typedef struct _WHEA_ARM_AARCH32_SECURE_CSR {
    ULONG SCTLR;
    ULONG SPSR_mon;
} WHEA_ARM_AARCH32_SECURE_CSR, *PWHEA_ARM_AARCH32_SECURE_CSR;

typedef struct _WHEA_ARMV8_AARCH64_GPRS {
    ULONGLONG X0;
    ULONGLONG X1;
    ULONGLONG X2;
    ULONGLONG X3;
    ULONGLONG X4;
    ULONGLONG X5;
    ULONGLONG X6;
    ULONGLONG X7;
    ULONGLONG X8;
    ULONGLONG X9;
    ULONGLONG X10;
    ULONGLONG X11;
    ULONGLONG X12;
    ULONGLONG X13;
    ULONGLONG X14;
    ULONGLONG X15;
    ULONGLONG X16;
    ULONGLONG X17;
    ULONGLONG X18;
    ULONGLONG X19;
    ULONGLONG X20;
    ULONGLONG X21;
    ULONGLONG X22;
    ULONGLONG X23;
    ULONGLONG X24;
    ULONGLONG X25;
    ULONGLONG X26;
    ULONGLONG X27;
    ULONGLONG X28;
    ULONGLONG X29;
    ULONGLONG X30;
    ULONGLONG SP;
} WHEA_ARMV8_AARCH64_GPRS, *PWHEA_ARMV8_AARCH64_GPRS;

typedef struct _WHEA_ARM_AARCH64_EL1_CSR {
    ULONGLONG ELR_EL1;
    ULONGLONG ESR_EL2;
    ULONGLONG FAR_EL1;
    ULONGLONG ISR_EL1;
    ULONGLONG MAIR_EL1;
    ULONGLONG MIDR_EL1;
    ULONGLONG MPIDR_EL1;
    ULONGLONG SCTLR_EL1;
    ULONGLONG SP_EL0;
    ULONGLONG SP_EL1;
    ULONGLONG SPSR_EL1;
    ULONGLONG TCR_EL1;
    ULONGLONG TPIDR_EL0;
    ULONGLONG TPIDR_EL1;
    ULONGLONG TPIDRRO_EL0;
    ULONGLONG TTBR0_EL1;
    ULONGLONG TTBR1_EL1;
} WHEA_ARM_AARCH64_EL1_CSR, *PWHEA_ARM_AARCH64_EL1_CSR;

typedef struct _WHEA_ARM_AARCH64_EL2_CSR {
    ULONGLONG ELR_EL2;
    ULONGLONG ESR_EL2;
    ULONGLONG FAR_EL2;
    ULONGLONG HACR_EL2;
    ULONGLONG HCR_EL2;
    ULONGLONG HPFAR_EL2;
    ULONGLONG MAIR_EL2;
    ULONGLONG SCTLR_EL2;
    ULONGLONG SP_EL2;
    ULONGLONG SPSR_EL2;
    ULONGLONG TCR_EL2;
    ULONGLONG TPIDR_EL2;
    ULONGLONG TTBR0_EL2;
    ULONGLONG VTCR_EL2;
    ULONGLONG VTTBR_EL2;
} WHEA_ARM_AARCH64_EL2_CSR, *PWHEA_ARM_AARCH64_EL2_CSR;

typedef struct _WHEA_ARMV8_AARCH64_EL3_CSR {
    ULONGLONG ELR_EL3;
    ULONGLONG ESR_EL3;
    ULONGLONG FAR_EL3;
    ULONGLONG MAIR_EL3;
    ULONGLONG SCTLR_EL3;
    ULONGLONG SP_EL3;
    ULONGLONG SPSR_EL3;
    ULONGLONG TCR_EL3;
    ULONGLONG TPIDR_EL3;
    ULONGLONG TTBR0_EL3;
} WHEA_ARMV8_AARCH64_EL3_CSR, *PWHEA_ARMV8_AARCH64_EL3_CSR;

typedef struct _WHEA_ARM_MISC_CSR {
    USHORT MRSEncoding;
    ULONGLONG Value;
} WHEA_ARM_MISC_CSR, *PWHEA_ARM_MISC_CSR;

typedef struct _WHEA_ARM_PROCESSOR_ERROR_CONTEXT_INFORMATION_HEADER {
    USHORT Version;
    USHORT RegisterContextType;
    ULONG RegisterArraySize;
    UCHAR RegisterArray[1];
} WHEA_ARM_PROCESSOR_ERROR_CONTEXT_INFORMATION_HEADER,
  *PWHEA_ARM_PROCESSOR_ERROR_CONTEXT_INFORMATION_HEADER;

CPER_FIELD_CHECK(WHEA_ARM_PROCESSOR_ERROR_CONTEXT_INFORMATION_HEADER, Version,               0,    2);
CPER_FIELD_CHECK(WHEA_ARM_PROCESSOR_ERROR_CONTEXT_INFORMATION_HEADER, RegisterContextType,   2,    2);
CPER_FIELD_CHECK(WHEA_ARM_PROCESSOR_ERROR_CONTEXT_INFORMATION_HEADER, RegisterArraySize,     4,    4);
CPER_FIELD_CHECK(WHEA_ARM_PROCESSOR_ERROR_CONTEXT_INFORMATION_HEADER, RegisterArray,         8,    1);

#include <poppack.h>


//-------------------------------------- Standard Error Notification Type GUIDs

/* 2dce8bb1-bdd7-450e-b9ad-9cf4ebd4f890 */
DEFINE_GUID(CMC_NOTIFY_TYPE_GUID,
            0x2dce8bb1, 0xbdd7, 0x450e, 0xb9, 0xad,
            0x9c, 0xf4, 0xeb, 0xd4, 0xf8, 0x90);

/* 4e292f96-d843-4a55-a8c2-d481f27ebeee */
DEFINE_GUID(CPE_NOTIFY_TYPE_GUID,
            0x4e292f96, 0xd843, 0x4a55, 0xa8, 0xc2,
            0xd4, 0x81, 0xf2, 0x7e, 0xbe, 0xee);

/* e8f56ffe-919c-4cc5-ba88-65abe14913bb */
DEFINE_GUID(MCE_NOTIFY_TYPE_GUID,
            0xe8f56ffe, 0x919c, 0x4cc5, 0xba, 0x88,
            0x65, 0xab, 0xe1, 0x49, 0x13, 0xbb);

/* cf93c01f-1a16-4dfc-b8bc-9c4daf67c104 */
DEFINE_GUID(PCIe_NOTIFY_TYPE_GUID,
            0xcf93c01f, 0x1a16, 0x4dfc, 0xb8, 0xbc,
            0x9c, 0x4d, 0xaf, 0x67, 0xc1, 0x04);

/* cc5263e8-9308-454a-89d0-340bd39bc98e */
DEFINE_GUID(INIT_NOTIFY_TYPE_GUID,
            0xcc5263e8, 0x9308, 0x454a, 0x89, 0xd0,
            0x34, 0x0b, 0xd3, 0x9b, 0xc9, 0x8e);

/* 5bad89ff-b7e6-42c9-814a-cf2485d6e98a */
DEFINE_GUID(NMI_NOTIFY_TYPE_GUID,
            0x5bad89ff, 0xb7e6, 0x42c9, 0x81, 0x4a,
            0xcf, 0x24, 0x85, 0xd6, 0xe9, 0x8a);

/* 3d61a466-ab40-409a-a698-f362d464b38f */
DEFINE_GUID(BOOT_NOTIFY_TYPE_GUID,
            0x3d61a466, 0xab40, 0x409a, 0xa6, 0x98,
            0xf3, 0x62, 0xd4, 0x64, 0xb3, 0x8f);

/* 9a78788a-bbe8-11e4-809e-67611e5d46b0 */
DEFINE_GUID(SEA_NOTIFY_TYPE_GUID,
            0x9a78788a, 0xbbe8, 0x11e4, 0x80, 0x9e,
            0x67, 0x61, 0x1e, 0x5d, 0x46, 0xb0);

/* 5c284c81-b0ae-4e87-a322-b04c85624323 */
DEFINE_GUID(SEI_NOTIFY_TYPE_GUID,
            0x5c284c81, 0xb0ae, 0x4e87, 0xa3, 0x22,
            0xb0, 0x4c, 0x85, 0x62, 0x43, 0x23);

/* 09a9d5ac-5204-4214-96e5-94992e752bcd */
DEFINE_GUID(PEI_NOTIFY_TYPE_GUID,
            0x09a9D5ac, 0x5204, 0x4214, 0x96, 0xe5,
            0x94, 0x99, 0x2e, 0x75, 0x2b, 0xcd);

//------------------------------------------- Standard Error Section type GUIDs

/* 9876ccad-47b4-4bdb-b65e-16f193c4f3db */
DEFINE_GUID(PROCESSOR_GENERIC_ERROR_SECTION_GUID,
            0x9876ccad, 0x47b4, 0x4bdb, 0xb6, 0x5e,
            0x16, 0xf1, 0x93, 0xc4, 0xf3, 0xdb);

/* dc3ea0b0-a144-4797-b95b-53fa242b6e1d */
DEFINE_GUID(XPF_PROCESSOR_ERROR_SECTION_GUID,
            0xdc3ea0b0, 0xa144, 0x4797, 0xb9, 0x5b,
            0x53, 0xfa, 0x24, 0x2b, 0x6e, 0x1d);

/* e429faf1-3cb7-11d4-bca7-0080c73c8881 */
DEFINE_GUID(IPF_PROCESSOR_ERROR_SECTION_GUID,
            0xe429faf1, 0x3cb7, 0x11d4, 0xbc, 0xa7,
            0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81);

/* e19e3d16-bc11-11e4-9caa-c2051d5d46b0 */
DEFINE_GUID(ARM_PROCESSOR_ERROR_SECTION_GUID,
            0xe19e3d16, 0xbc11, 0x11e4, 0x9c, 0xaa,
            0xc2, 0x05, 0x1d, 0x5d, 0x46, 0xb0);

/* a5bc1114-6f64-4ede-b863-3e83ed7c83b1 */
DEFINE_GUID(MEMORY_ERROR_SECTION_GUID,
            0xa5bc1114, 0x6f64, 0x4ede, 0xb8, 0x63,
            0x3e, 0x83, 0xed, 0x7c, 0x83, 0xb1);

/* d995e954-bbc1-430f-ad91-b44dcb3c6f35 */
DEFINE_GUID(PCIEXPRESS_ERROR_SECTION_GUID,
            0xd995e954, 0xbbc1, 0x430f, 0xad, 0x91,
            0xb4, 0x4d, 0xcb, 0x3c, 0x6f, 0x35);

/* c5753963-3b84-4095-bf78-eddad3f9c9dd */
DEFINE_GUID(PCIXBUS_ERROR_SECTION_GUID,
            0xc5753963, 0x3b84, 0x4095, 0xbf, 0x78,
            0xed, 0xda, 0xd3, 0xf9, 0xc9, 0xdd);

/* eb5e4685-ca66-4769-b6a2-26068b001326 */
DEFINE_GUID(PCIXDEVICE_ERROR_SECTION_GUID,
            0xeb5e4685, 0xca66, 0x4769, 0xb6, 0xa2,
            0x26, 0x06, 0x8b, 0x00, 0x13, 0x26);

/* 81212a96-09ed-4996-9471-8d729c8e69ed */
DEFINE_GUID(FIRMWARE_ERROR_RECORD_REFERENCE_GUID,
            0x81212a96, 0x09ed, 0x4996, 0x94, 0x71,
            0x8d, 0x72, 0x9c, 0x8e, 0x69, 0xed);

//-------------------------------------- Processor check information type GUIDs

/* a55701f5-e3ef-43de-ac72-249b573fad2c */
DEFINE_GUID(WHEA_CACHECHECK_GUID,
            0xa55701f5, 0xe3ef, 0x43de, 0xac, 0x72,
            0x24, 0x9b, 0x57, 0x3f, 0xad, 0x2c);

/* fc06b535-5e1f-4562-9f25-0a3b9adb63c3 */
DEFINE_GUID(WHEA_TLBCHECK_GUID,
            0xfc06b535, 0x5e1f, 0x4562, 0x9f, 0x25,
            0x0a, 0x3b, 0x9a, 0xdb, 0x63, 0xc3);

/* 1cf3f8b3-c5b1-49a2-aa59-5eef92ffa63c */
DEFINE_GUID(WHEA_BUSCHECK_GUID,
            0x1cf3f8b3, 0xc5b1, 0x49a2, 0xaa, 0x59,
            0x5e, 0xef, 0x92, 0xff, 0xa6, 0x3c);

/* 48ab7f57-dc34-4f6c-a7d3-b0b5b0a74314 */
DEFINE_GUID(WHEA_MSCHECK_GUID,
            0x48ab7f57, 0xdc34, 0x4f6c, 0xa7, 0xd3,
            0xb0, 0xb5, 0xb0, 0xa7, 0x43, 0x14);

//
// This is the start of the Microsoft specific extensions to the Common Platform
// Error Record specification. This is in accordance with Appendix N, section
// 2.3 of the Unified Extensible Firmware Interface specification, which allows
// the specification of non-standard section bodies.
//

//---------------------------------------------------- Microsoft record creator

/* cf07c4bd-b789-4e18-b3c4-1f732cb57131 */
DEFINE_GUID(WHEA_RECORD_CREATOR_GUID,
            0xcf07c4bd,
            0xb789, 0x4e18,
            0xb3, 0xc4, 0x1f, 0x73, 0x2c, 0xb5, 0x71, 0x31);

//--------------------------------------- Microsoft specific notification types

/* 3e62a467-ab40-409a-a698-f362d464b38f */
DEFINE_GUID(GENERIC_NOTIFY_TYPE_GUID,
            0x3e62a467,
            0xab40, 0x409a,
            0xa6, 0x98, 0xf3, 0x62, 0xd4, 0x64, 0xb3, 0x8f);

//-------------------------------------- Microsoft specific error section types

/* 6f3380d1-6eb0-497f-a578-4d4c65a71617 */
DEFINE_GUID(IPF_SAL_RECORD_SECTION_GUID,
            0x6f3380d1,
            0x6eb0, 0x497f,
            0xa5, 0x78, 0x4d, 0x4c, 0x65, 0xa7, 0x16, 0x17);

/* 8a1e1d01-42f9-4557-9c33-565e5cc3f7e8 */
DEFINE_GUID(XPF_MCA_SECTION_GUID,
            0x8a1e1d01,
            0x42f9, 0x4557,
            0x9c, 0x33, 0x56, 0x5e, 0x5c, 0xc3, 0xf7, 0xe8);

/* e71254e7-c1b9-4940-ab76-909703a4320f */
DEFINE_GUID(NMI_SECTION_GUID,
            0xe71254e7,
            0xc1b9, 0x4940,
            0xab, 0x76, 0x90, 0x97, 0x03, 0xa4, 0x32, 0x0f);

/* e71254e8-c1b9-4940-ab76-909703a4320f */
DEFINE_GUID(GENERIC_SECTION_GUID,
            0xe71254e8,
            0xc1b9, 0x4940,
            0xab, 0x76, 0x90, 0x97, 0x03, 0xa4, 0x32, 0x0f);

/* e71254e9-c1b9-4940-ab76-909703a4320f */
DEFINE_GUID(WHEA_ERROR_PACKET_SECTION_GUID,
            0xe71254e9,
            0xc1b9, 0x4940,
            0xab, 0x76, 0x90, 0x97, 0x03, 0xa4, 0x32, 0x0f);


#if defined(_NTPSHEDDLL_)

#define NTPSHEDAPI

#else

#define NTPSHEDAPI DECLSPEC_IMPORT

#endif

#include <pshpack1.h>

//----------------------------------------------------------- WHEA_ERROR_PACKET

typedef enum _WHEA_ERROR_TYPE {
    WheaErrTypeProcessor = 0,
    WheaErrTypeMemory,
    WheaErrTypePCIExpress,
    WheaErrTypeNMI,
    WheaErrTypePCIXBus,
    WheaErrTypePCIXDevice,
    WheaErrTypeGeneric
} WHEA_ERROR_TYPE, *PWHEA_ERROR_TYPE;

typedef union _WHEA_ERROR_PACKET_FLAGS {
    struct {
        ULONG PreviousError:1;
        ULONG Reserved1:1;
        ULONG HypervisorError:1;
        ULONG Simulated:1;
        ULONG PlatformPfaControl:1;
        ULONG PlatformDirectedOffline:1;
        ULONG Reserved2:26;
    } DUMMYSTRUCTNAME;
    ULONG AsULONG;
} WHEA_ERROR_PACKET_FLAGS, *PWHEA_ERROR_PACKET_FLAGS;

typedef enum _WHEA_ERROR_PACKET_DATA_FORMAT {
    WheaDataFormatIPFSalRecord = 0,
    WheaDataFormatXPFMCA,
    WheaDataFormatMemory,
    WheaDataFormatPCIExpress,
    WheaDataFormatNMIPort,
    WheaDataFormatPCIXBus,
    WheaDataFormatPCIXDevice,
    WheaDataFormatGeneric,
    WheaDataFormatMax
} WHEA_ERROR_PACKET_DATA_FORMAT, *PWHEA_ERROR_PACKET_DATA_FORMAT;

typedef enum _WHEA_RAW_DATA_FORMAT {
    WheaRawDataFormatIPFSalRecord = 0x00,
    WheaRawDataFormatIA32MCA,
    WheaRawDataFormatIntel64MCA,
    WheaRawDataFormatAMD64MCA,
    WheaRawDataFormatMemory,
    WheaRawDataFormatPCIExpress,
    WheaRawDataFormatNMIPort,
    WheaRawDataFormatPCIXBus,
    WheaRawDataFormatPCIXDevice,
    WheaRawDataFormatGeneric,
    WheaRawDataFormatMax
} WHEA_RAW_DATA_FORMAT, *PWHEA_RAW_DATA_FORMAT;

typedef struct _WHEA_ERROR_PACKET_V1 {
    ULONG                   Signature;                          // +0x00 (0)
    WHEA_ERROR_PACKET_FLAGS Flags;                              // +0x04 (4)
    ULONG                   Size;                               // +0x08 (8)
    ULONG                   RawDataLength;                      // +0x0C (12)
    ULONGLONG               Reserved1;                          // +0x10 (16)
    ULONGLONG               Context;                            // +0x18 (24)
    WHEA_ERROR_TYPE         ErrorType;                          // +0x20 (32)
    WHEA_ERROR_SEVERITY     ErrorSeverity;                      // +0x24 (36)
    ULONG                   ErrorSourceId;                      // +0x28 (40)
    WHEA_ERROR_SOURCE_TYPE  ErrorSourceType;                    // +0x2C (44)
    ULONG                   Reserved2;                          // +0x30 (48)
    ULONG                   Version;                            // +0x34 (52)
    ULONGLONG               Cpu;                                // +0x38 (56)
    union {
        WHEA_PROCESSOR_GENERIC_ERROR_SECTION    ProcessorError; // +0x40 (64)
        WHEA_MEMORY_ERROR_SECTION               MemoryError;
        WHEA_NMI_ERROR_SECTION                  NmiError;
        WHEA_PCIEXPRESS_ERROR_SECTION           PciExpressError;
        WHEA_PCIXBUS_ERROR_SECTION              PciXBusError;
        WHEA_PCIXDEVICE_ERROR_SECTION           PciXDeviceError;
    } u;
    WHEA_RAW_DATA_FORMAT     RawDataFormat;                     // +0x110 (272)
    ULONG                    RawDataOffset;                     // +0x114 (276)
    UCHAR                    RawData[1];                        // +0x118 (280)

} WHEA_ERROR_PACKET_V1, *PWHEA_ERROR_PACKET_V1;

#define WHEA_ERROR_PACKET_V1_SIGNATURE  'tPrE'
#define WHEA_ERROR_PACKET_V1_VERSION    2

typedef struct _WHEA_ERROR_PACKET_V2 {
    ULONG Signature;
    ULONG Version;
    ULONG Length;
    WHEA_ERROR_PACKET_FLAGS Flags;
    WHEA_ERROR_TYPE ErrorType;
    WHEA_ERROR_SEVERITY ErrorSeverity;
    ULONG ErrorSourceId;
    WHEA_ERROR_SOURCE_TYPE ErrorSourceType;
    GUID NotifyType;
    ULONGLONG Context;
    WHEA_ERROR_PACKET_DATA_FORMAT DataFormat;
    ULONG Reserved1;
    ULONG DataOffset;
    ULONG DataLength;
    ULONG PshedDataOffset;
    ULONG PshedDataLength;
    // UCHAR Data[ANYSIZE_ARRAY];
    // UCHAR PshedData[ANYSIZE_ARRAY];
} WHEA_ERROR_PACKET_V2, *PWHEA_ERROR_PACKET_V2;

CPER_FIELD_CHECK(WHEA_ERROR_PACKET_V2, Signature,         0,   4);
CPER_FIELD_CHECK(WHEA_ERROR_PACKET_V2, Version,           4,   4);
CPER_FIELD_CHECK(WHEA_ERROR_PACKET_V2, Length,            8,   4);
CPER_FIELD_CHECK(WHEA_ERROR_PACKET_V2, Flags,            12,   4);
CPER_FIELD_CHECK(WHEA_ERROR_PACKET_V2, ErrorType,        16,   4);
CPER_FIELD_CHECK(WHEA_ERROR_PACKET_V2, ErrorSeverity,    20,   4);
CPER_FIELD_CHECK(WHEA_ERROR_PACKET_V2, ErrorSourceId,    24,   4);
CPER_FIELD_CHECK(WHEA_ERROR_PACKET_V2, ErrorSourceType,  28,   4);
CPER_FIELD_CHECK(WHEA_ERROR_PACKET_V2, NotifyType,       32,  16);
CPER_FIELD_CHECK(WHEA_ERROR_PACKET_V2, Context,          48,   8);
CPER_FIELD_CHECK(WHEA_ERROR_PACKET_V2, DataFormat,       56,   4);
CPER_FIELD_CHECK(WHEA_ERROR_PACKET_V2, Reserved1,        60,   4);
CPER_FIELD_CHECK(WHEA_ERROR_PACKET_V2, DataOffset,       64,   4);
CPER_FIELD_CHECK(WHEA_ERROR_PACKET_V2, DataLength,       68,   4);
CPER_FIELD_CHECK(WHEA_ERROR_PACKET_V2, PshedDataOffset,  72,   4);
CPER_FIELD_CHECK(WHEA_ERROR_PACKET_V2, PshedDataLength,  76,   4);

#define WHEA_ERROR_PACKET_V2_SIGNATURE 'AEHW'
#define WHEA_ERROR_PACKET_V2_VERSION   3

#if (NTDDI_VERSION >= NTDDI_WIN7)

#define WHEA_ERROR_PACKET_SIGNATURE     WHEA_ERROR_PACKET_V2_SIGNATURE
#define WHEA_ERROR_PACKET_VERSION       WHEA_ERROR_PACKET_V2_VERSION
typedef struct _WHEA_ERROR_PACKET_V2    WHEA_ERROR_PACKET, *PWHEA_ERROR_PACKET;

#else

#define WHEA_ERROR_PACKET_SIGNATURE     WHEA_ERROR_PACKET_V1_SIGNATURE
#define WHEA_ERROR_PACKET_VERSION       WHEA_ERROR_PACKET_V1_VERSION
#define WHEA_ERROR_PKT_SIGNATURE        WHEA_ERROR_PACKET_SIGNATURE
#define WHEA_ERROR_PKT_VERSION          WHEA_ERROR_PACKET_VERSION
typedef struct _WHEA_ERROR_PACKET_V1    WHEA_ERROR_PACKET, *PWHEA_ERROR_PACKET;

#endif

//---------------------------------------------------------- WHEA_GENERIC_ERROR

//
// These structure define the data format that must be used by error sources
// when reporting errors of the generic error type.
//

typedef union _WHEA_GENERIC_ERROR_BLOCKSTATUS {
    struct {
        ULONG UncorrectableError:1;
        ULONG CorrectableError:1;
        ULONG MultipleUncorrectableErrors:1;
        ULONG MultipleCorrectableErrors:1;
        ULONG ErrorDataEntryCount:10;
        ULONG Reserved:18;
    } DUMMYSTRUCTNAME;
    ULONG AsULONG;
} WHEA_GENERIC_ERROR_BLOCKSTATUS, *PWHEA_GENERIC_ERROR_BLOCKSTATUS;

typedef struct _WHEA_GENERIC_ERROR {
    WHEA_GENERIC_ERROR_BLOCKSTATUS BlockStatus;
    ULONG RawDataOffset;
    ULONG RawDataLength;
    ULONG DataLength;
    WHEA_ERROR_SEVERITY ErrorSeverity;
    UCHAR Data[1];
} WHEA_GENERIC_ERROR, *PWHEA_GENERIC_ERROR;

typedef struct _WHEA_GENERIC_ERROR_DATA_ENTRY_V1 {
    GUID SectionType;
    WHEA_ERROR_SEVERITY ErrorSeverity;
    WHEA_REVISION Revision;
    UCHAR ValidBits;
    UCHAR Flags;
    ULONG ErrorDataLength;
    GUID FRUId;
    UCHAR FRUText[20];
    UCHAR Data[1];
} WHEA_GENERIC_ERROR_DATA_ENTRY_V1, *PWHEA_GENERIC_ERROR_DATA_ENTRY_V1;

typedef struct _WHEA_GENERIC_ERROR_DATA_ENTRY_V2 {
    GUID SectionType;
    WHEA_ERROR_SEVERITY ErrorSeverity;
    WHEA_REVISION Revision;
    UCHAR ValidBits;
    UCHAR Flags;
    ULONG ErrorDataLength;
    GUID FRUId;
    UCHAR FRUText[20];
    WHEA_TIMESTAMP Timestamp;
    UCHAR Data[1];
} WHEA_GENERIC_ERROR_DATA_ENTRY_V2, *PWHEA_GENERIC_ERROR_DATA_ENTRY_V2;

#define WHEA_GENERIC_ENTRY_V2_VERSION   0x300

//
// Use V2 Generic Data Entry.
//

#define WHEA_GENERIC_ENTRY_VERSION      WHEA_GENERIC_ENTRY_V2_VERSION
typedef struct _WHEA_GENERIC_ERROR_DATA_ENTRY_V2    WHEA_GENERIC_ERROR_DATA_ENTRY, *PWHEA_GENERIC_ERROR_DATA_ENTRY;

#include <poppack.h>


//----------------------------------------------- WheaGetErrPacketFromErrRecord

_Must_inspect_result_
__inline
PWHEA_ERROR_PACKET
WheaGetErrPacketFromErrRecord (
    _In_ PWHEA_ERROR_RECORD Record
    )

/*++

Routine Description:

    This routine will search out the error packet contained within an error
    record and return a reference to it.

Arguments:

    Record - Supplies a pointer to the error record to be searched.

Return Value:

    If successful, a pointer to the error packet.

    NULL otherwise.

--*/

{

    PWHEA_ERROR_PACKET Packet;
    PWHEA_ERROR_RECORD_SECTION_DESCRIPTOR Descriptor;
    ULONG Section;
    ULONG SizeRequired;

    Packet = NULL;
    if (Record->Header.Signature != WHEA_ERROR_RECORD_SIGNATURE) {
        goto GetErrPacketFromErrRecordEnd;
    }

    //
    // Calculate the size required for the header and section descriptors.
    // Ensure that at least these will be properly contained within the extent
    // of the error record.
    //

    SizeRequired = sizeof(WHEA_ERROR_RECORD_HEADER) +
        (sizeof(WHEA_ERROR_RECORD_SECTION_DESCRIPTOR) *
         Record->Header.SectionCount);

    if (Record->Header.Length < SizeRequired) {
        goto GetErrPacketFromErrRecordEnd;
    }

    //
    // Step through the section descriptors looking for the error packet. If the
    // error packet descriptor is found, ensure that the error packet section is
    // properly contained within the extent of the error record.
    //

    Descriptor = &Record->SectionDescriptor[0];
    for (Section = 0; Section < Record->Header.SectionCount; Section += 1) {

        if (RtlCompareMemory(&Descriptor->SectionType,
                             &WHEA_ERROR_PACKET_SECTION_GUID,
                             sizeof(GUID)) == sizeof(GUID)) {

                SizeRequired = Descriptor->SectionOffset +
                    Descriptor->SectionLength;

                if (Record->Header.Length < SizeRequired) {
                    goto GetErrPacketFromErrRecordEnd;
                }

                Packet = (PWHEA_ERROR_PACKET)
                    (((PUCHAR)Record) + Descriptor->SectionOffset);

#pragma warning(suppress: 26019)
                if (Packet->Signature != WHEA_ERROR_PACKET_SIGNATURE) {
                    Packet = NULL;
                }

                goto GetErrPacketFromErrRecordEnd;
        }

        Descriptor += 1;
    }

GetErrPacketFromErrRecordEnd:
    return Packet;
}

//------------------------------------------- WHEA_ERROR_INJECTION_CAPABILITIES

//
// PSHED plug-ins use this structure to communicate error injection capabilities
// to the operating system.
//

typedef union _WHEA_ERROR_INJECTION_CAPABILITIES {
    struct {
        ULONG ProcessorCorrectable:1;                   // 0x00000001
        ULONG ProcessorUncorrectableNonFatal:1;         // 0x00000002
        ULONG ProcessorUncorrectableFatal:1;            // 0x00000004
        ULONG MemoryCorrectable:1;                      // 0x00000008
        ULONG MemoryUncorrectableNonFatal:1;            // 0x00000010
        ULONG MemoryUncorrectableFatal:1;               // 0x00000020
        ULONG PCIExpressCorrectable:1;                  // 0x00000040
        ULONG PCIExpressUncorrectableNonFatal:1;        // 0x00000080
        ULONG PCIExpressUncorrectableFatal:1;           // 0x00000100
        ULONG PlatformCorrectable:1;                    // 0x00000200
        ULONG PlatformUncorrectableNonFatal:1;          // 0x00000400
        ULONG PlatformUncorrectableFatal:1;             // 0x00000800
        ULONG IA64Corrected:1;                          // 0x00001000
        ULONG IA64Recoverable:1;                        // 0x00002000
        ULONG IA64Fatal:1;                              // 0x00004000
        ULONG IA64RecoverableCache:1;                   // 0x00008000
        ULONG IA64RecoverableRegFile:1;                 // 0x00010000
        ULONG Reserved:15;
    } DUMMYSTRUCTNAME;
    ULONG AsULONG;
} WHEA_ERROR_INJECTION_CAPABILITIES, *PWHEA_ERROR_INJECTION_CAPABILITIES;

#define INJECT_ERRTYPE_PROCESSOR_CORRECTABLE                    0x00000001
#define INJECT_ERRTYPE_PROCESSOR_UNCORRECTABLENONFATAL          0x00000002
#define INJECT_ERRTYPE_PROCESSOR_UNCORRECTABLEFATAL             0x00000004
#define INJECT_ERRTYPE_MEMORY_CORRECTABLE                       0x00000008
#define INJECT_ERRTYPE_MEMORY_UNCORRECTABLENONFATAL             0x00000010
#define INJECT_ERRTYPE_MEMORY_UNCORRECTABLEFATAL                0x00000020
#define INJECT_ERRTYPE_PCIEXPRESS_CORRECTABLE                   0x00000040
#define INJECT_ERRTYPE_PCIEXPRESS_UNCORRECTABLENONFATAL         0x00000080
#define INJECT_ERRTYPE_PCIEXPRESS_UNCORRECTABLEFATAL            0x00000100
#define INJECT_ERRTYPE_PLATFORM_CORRECTABLE                     0x00000200
#define INJECT_ERRTYPE_PLATFORM_UNCORRECTABLENONFATAL           0x00000400
#define INJECT_ERRTYPE_PLATFORM_UNCORRECTABLEFATAL              0x00000800

//----------------------------------------------------------- In-use Page Offline Callbacks

#if defined (_AMD64_)

typedef
BOOLEAN
(*PFN_IN_USE_PAGE_OFFLINE_NOTIFY) (
    _In_ PFN_NUMBER Page,
    _In_ BOOLEAN PlatformDirected,
    _In_ BOOLEAN Poisoned,
    _In_ PVOID Context
    );

NTKERNELAPI
NTSTATUS
WheaRegisterInUsePageOfflineNotification (
    _In_ PFN_IN_USE_PAGE_OFFLINE_NOTIFY Callback,
    _In_opt_ PVOID Context
    );

NTKERNELAPI
NTSTATUS
WheaUnregisterInUsePageOfflineNotification (
    _In_ PFN_IN_USE_PAGE_OFFLINE_NOTIFY Callback
    );

#endif // _AMD64_

typedef struct _WHEA_RECOVERY_CONTEXT {
    union {
        struct {
            ULONG_PTR Address;
            BOOLEAN Consumed;
            UINT16 ErrorCode;
            BOOLEAN ErrorIpValid;
            BOOLEAN RestartIpValid;
        } MemoryError;
    };
    UINT64 PartitionId;  //HV_PARTITION_ID
    UINT32 VpIndex;      //HV_VP_INDEX

} WHEA_RECOVERY_CONTEXT, *PWHEA_RECOVERY_CONTEXT;

#if !defined(XBOX_SYSTEMOS)

typedef
NTSTATUS
(HVL_WHEA_ERROR_NOTIFICATION) (
    _In_ PWHEA_RECOVERY_CONTEXT RecoveryContext,
    _In_ BOOLEAN PlatformDirected,
    _In_ BOOLEAN Poisoned
    );

typedef HVL_WHEA_ERROR_NOTIFICATION *PHVL_WHEA_ERROR_NOTIFICATION;

extern PHVL_WHEA_ERROR_NOTIFICATION HvlpWheaErrorNotificationCallback;

NTKERNELAPI
NTSTATUS
HvlRegisterWheaErrorNotification (
    _In_ PHVL_WHEA_ERROR_NOTIFICATION Callback
    );

NTKERNELAPI
NTSTATUS
HvlUnregisterWheaErrorNotification(
    _In_ PHVL_WHEA_ERROR_NOTIFICATION Callback
    );

#endif


//------------------------------------------------ PSHED Plug-in Callback Types

_Must_inspect_result_
typedef
NTSTATUS
(*PSHED_PI_GET_ALL_ERROR_SOURCES) (
    _Inout_opt_ PVOID PluginContext,
    _Inout_ PULONG Count,
    _Inout_updates_bytes_(*Length) PWHEA_ERROR_SOURCE_DESCRIPTOR *ErrorSrcs,
    _Inout_ PULONG Length
    );

_Must_inspect_result_
typedef
NTSTATUS
(*PSHED_PI_GET_ERROR_SOURCE_INFO) (
    _Inout_opt_ PVOID PluginContext,
    _Inout_ PWHEA_ERROR_SOURCE_DESCRIPTOR ErrorSource
    );

_Must_inspect_result_
typedef
NTSTATUS
(*PSHED_PI_SET_ERROR_SOURCE_INFO) (
    _Inout_opt_ PVOID PluginContext,
    _In_ PWHEA_ERROR_SOURCE_DESCRIPTOR ErrorSource
    );

typedef
NTSTATUS
 (*PSHED_PI_ENABLE_ERROR_SOURCE) (
    _Inout_opt_ PVOID PluginContext,
    _In_ PWHEA_ERROR_SOURCE_DESCRIPTOR ErrorSource
    );

typedef
NTSTATUS
 (*PSHED_PI_DISABLE_ERROR_SOURCE) (
    _Inout_opt_ PVOID PluginContext,
    _In_ PWHEA_ERROR_SOURCE_DESCRIPTOR ErrorSource
    );

typedef
NTSTATUS
(*PSHED_PI_WRITE_ERROR_RECORD) (
    _Inout_opt_ PVOID PluginContext,
    _In_ ULONG Flags,
    _In_ ULONG RecordLength,
    _In_reads_bytes_(RecordLength) PWHEA_ERROR_RECORD ErrorRecord
    );

_Must_inspect_result_
typedef
NTSTATUS
(*PSHED_PI_READ_ERROR_RECORD) (
    _Inout_opt_ PVOID PluginContext,
    _In_ ULONG Flags,
    _In_ ULONGLONG ErrorRecordId,
    _Out_ PULONGLONG NextErrorRecordId,
    _Inout_ PULONG RecordLength,
    _Out_writes_bytes_(*RecordLength) PWHEA_ERROR_RECORD ErrorRecord
    );

typedef
NTSTATUS
(*PSHED_PI_CLEAR_ERROR_RECORD) (
    _Inout_opt_ PVOID PluginContext,
    _In_ ULONG Flags,
    _In_ ULONGLONG ErrorRecordId
    );

typedef
NTSTATUS
(*PSHED_PI_RETRIEVE_ERROR_INFO) (
    _Inout_opt_ PVOID PluginContext,
    _In_ PWHEA_ERROR_SOURCE_DESCRIPTOR ErrorSource,
    _In_ ULONGLONG BufferLength,
    _Inout_updates_bytes_(BufferLength) PWHEA_ERROR_PACKET Packet
    );

typedef
NTSTATUS
(*PSHED_PI_FINALIZE_ERROR_RECORD) (
    _Inout_opt_ PVOID PluginContext,
    _In_ PWHEA_ERROR_SOURCE_DESCRIPTOR ErrorSource,
    _In_ ULONG BufferLength,
    _Inout_updates_bytes_(BufferLength) PWHEA_ERROR_RECORD ErrorRecord
    );

typedef
NTSTATUS
(*PSHED_PI_CLEAR_ERROR_STATUS) (
    _Inout_opt_ PVOID PluginContext,
    _In_ PWHEA_ERROR_SOURCE_DESCRIPTOR ErrorSource,
    _In_ ULONG BufferLength,
    _In_reads_bytes_(BufferLength) PWHEA_ERROR_RECORD ErrorRecord
    );

_Must_inspect_result_
typedef
NTSTATUS
(*PSHED_PI_ATTEMPT_ERROR_RECOVERY) (
    _Inout_opt_ PVOID PluginContext,
    _In_ ULONG BufferLength,
    _In_reads_bytes_(BufferLength) PWHEA_ERROR_RECORD ErrorRecord
    );

_Must_inspect_result_
typedef
NTSTATUS
(*PSHED_PI_GET_INJECTION_CAPABILITIES) (
    _Inout_opt_ PVOID PluginContext,
    _Out_ PWHEA_ERROR_INJECTION_CAPABILITIES Capabilities
    );

_Must_inspect_result_
typedef
NTSTATUS
(*PSHED_PI_INJECT_ERROR) (
    _Inout_opt_ PVOID PluginContext,
    _In_ ULONGLONG ErrorType,
    _In_ ULONGLONG Parameter1,
    _In_ ULONGLONG Parameter2,
    _In_ ULONGLONG Parameter3,
    _In_ ULONGLONG Parameter4
    );

//--------------------------------------- WHEA_PSHED_PLUGIN_REGISTRATION_PACKET

typedef struct _WHEA_PSHED_PLUGIN_CALLBACKS {
    PSHED_PI_GET_ALL_ERROR_SOURCES GetAllErrorSources;
    PVOID Reserved;
    PSHED_PI_GET_ERROR_SOURCE_INFO GetErrorSourceInfo;
    PSHED_PI_SET_ERROR_SOURCE_INFO SetErrorSourceInfo;
    PSHED_PI_ENABLE_ERROR_SOURCE EnableErrorSource;
    PSHED_PI_DISABLE_ERROR_SOURCE DisableErrorSource;
    PSHED_PI_WRITE_ERROR_RECORD WriteErrorRecord;
    PSHED_PI_READ_ERROR_RECORD ReadErrorRecord;
    PSHED_PI_CLEAR_ERROR_RECORD ClearErrorRecord;
    PSHED_PI_RETRIEVE_ERROR_INFO RetrieveErrorInfo;
    PSHED_PI_FINALIZE_ERROR_RECORD FinalizeErrorRecord;
    PSHED_PI_CLEAR_ERROR_STATUS ClearErrorStatus;
    PSHED_PI_ATTEMPT_ERROR_RECOVERY AttemptRecovery;
    PSHED_PI_GET_INJECTION_CAPABILITIES GetInjectionCapabilities;
    PSHED_PI_INJECT_ERROR InjectError;
} WHEA_PSHED_PLUGIN_CALLBACKS, *PWHEA_PSHED_PLUGIN_CALLBACKS;

typedef struct _WHEA_PSHED_PLUGIN_REGISTRATION_PACKET {
    ULONG Length;
    ULONG Version;
    PVOID Context;
    ULONG FunctionalAreaMask;
    ULONG Reserved;
    WHEA_PSHED_PLUGIN_CALLBACKS Callbacks;
} WHEA_PSHED_PLUGIN_REGISTRATION_PACKET,
  *PWHEA_PSHED_PLUGIN_REGISTRATION_PACKET;

#define WHEA_PLUGIN_REGISTRATION_PACKET_VERSION 0x00010000

//
// These defines specify the values of the bits in the functional area mask
// field of the PSHED plug-in registration packet.
//

#define PshedFADiscovery              0x00000001
#define PshedFAErrorSourceControl     0x00000002
#define PshedFAErrorRecordPersistence 0x00000004
#define PshedFAErrorInfoRetrieval     0x00000008
#define PshedFAErrorRecovery          0x00000010
#define PshedFAErrorInjection         0x00000020

//------------------------------------------------------ PSHED Plug-in services

#define WHEA_WRITE_FLAG_DUMMY 0x00000001

//
// The following services are exported by the PSHED for use by PSHED plug-ins.
//

#if (NTDDI_VERSION >= NTDDI_WS08)
_IRQL_requires_max_(DISPATCH_LEVEL)
__drv_allocatesMem(Mem)
_Post_writable_byte_size_(Size)
_Must_inspect_result_
NTPSHEDAPI
PVOID
PshedAllocateMemory (
    _In_ ULONG Size
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WS08)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTPSHEDAPI
VOID
PshedFreeMemory (
    _In_ __drv_freesMem(Mem) PVOID Address
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WS08)
NTPSHEDAPI
BOOLEAN
PshedIsSystemWheaEnabled (
    VOID
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WS08)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTPSHEDAPI
NTSTATUS
PshedRegisterPlugin (
    _Inout_ PWHEA_PSHED_PLUGIN_REGISTRATION_PACKET Packet
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WS08)
NTPSHEDAPI
BOOLEAN
PshedSynchronizeExecution (
    _In_ PWHEA_ERROR_SOURCE_DESCRIPTOR ErrorSource,
    _In_ PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    _In_ PVOID SynchronizeContext
    );
#endif

//----------------------------------------------- Error record access functions

_Must_inspect_result_
__inline
BOOLEAN
WheaIsValidErrorRecordSignature (
    _In_ PWHEA_ERROR_RECORD Record
    )

/*++

Routine Description:

    This routine will compare the error record signature with the proper values
    and signal whether it is correct or not.

Arguments:

    Record - Supplies a pointer to the error record.

Return Value:

    TRUE if the error record signature is correct.

    FALSE otherwise.

--*/

{

    BOOLEAN Valid;

    if ((Record->Header.Signature == WHEA_ERROR_RECORD_SIGNATURE) &&
        (Record->Header.Revision.AsUSHORT == WHEA_ERROR_RECORD_REVISION) &&
        (Record->Header.SignatureEnd == WHEA_ERROR_RECORD_SIGNATURE_END)) {

        Valid = TRUE;

    } else {
        Valid = FALSE;
    }

    return Valid;
}

_Must_inspect_result_
__inline
NTSTATUS
WheaFindErrorRecordSection (
    _In_ PWHEA_ERROR_RECORD Record,
    _In_ const GUID *SectionType,
    _Out_ PWHEA_ERROR_RECORD_SECTION_DESCRIPTOR *SectionDescriptor,
    _Out_opt_ PVOID *SectionData
    )

/*++

Routine Description:

    This routine provides a means to search an error record for a specific
    section.

Arguments:

    Record - Supplies a pointer to the error record.

    SectionType - Supplies a GUID specifying the section being sought. This may
        be any standard common platform error record or implementation specific
        section type.

    Descriptor - Supplies a location in which a pointer to the descriptor for
        the found section is returned.

    Section - Supplies an optional location in which a pointer to the found
        section is returned.

Return Value:

    STATUS_SUCCESS if the specified section is found.

    STATUS_NOT_FOUND if the specified section is not found.

    STATUS_INVALID_PARAMETER if the record does not appear well formed or the
        context parameter is null in cases where it is required.

--*/

{

    NTSTATUS Status;
    PWHEA_ERROR_RECORD_SECTION_DESCRIPTOR Descriptor;
    ULONG Index;
    ULONG MinimumLength;

    if ((Record == NULL) ||
        (SectionType == NULL) ||
        (SectionDescriptor == NULL) ||
        (WheaIsValidErrorRecordSignature(Record) == FALSE) ||
        (Record->Header.SectionCount == 0)) {

        Status = STATUS_INVALID_PARAMETER;
        goto FindErrorRecordSectionEnd;
    }

    //
    // Ensure that the supplied record is at least as long as required to store
    // the descriptors for the sections supposedly in the record.
    //

    MinimumLength = sizeof(WHEA_ERROR_RECORD_HEADER) +
        (Record->Header.SectionCount *
         sizeof(WHEA_ERROR_RECORD_SECTION_DESCRIPTOR));

    if (Record->Header.Length < MinimumLength) {
        Status = STATUS_INVALID_PARAMETER;
        goto FindErrorRecordSectionEnd;
    }

    //
    // Iterate through the record searching for the section in question.
    //

    Descriptor = &Record->SectionDescriptor[0];
    for (Index = 0; Index < Record->Header.SectionCount; Index += 1) {
        if (RtlCompareMemory(&Descriptor->SectionType,
                             SectionType,
                             sizeof(GUID)) == sizeof(GUID)) {

            break;
        }

        Descriptor += 1;
    }

    if (Index >= Record->Header.SectionCount) {
        Status = STATUS_NOT_FOUND;
        goto FindErrorRecordSectionEnd;
    }

    //
    // If the descriptor describes a section that is not completely contained
    // within the record then the record is invalid.
    //

    if ((Descriptor->SectionOffset + Descriptor->SectionLength) >
        Record->Header.Length) {

        Status = STATUS_INVALID_PARAMETER;
        goto FindErrorRecordSectionEnd;
    }

    //
    // Return the descriptor and optionally a pointer to the section itself.
    //

    *SectionDescriptor = Descriptor;
    if (SectionData != NULL) {
        *SectionData = (PVOID)(((PUCHAR)Record) + Descriptor->SectionOffset);
    }

    Status = STATUS_SUCCESS;

FindErrorRecordSectionEnd:
    return Status;
}

_Must_inspect_result_
__inline
NTSTATUS
WheaFindNextErrorRecordSection (
    _In_ PWHEA_ERROR_RECORD Record,
    _Inout_ ULONG *Context,
    _Out_ PWHEA_ERROR_RECORD_SECTION_DESCRIPTOR *SectionDescriptor,
    _Out_opt_ PVOID *SectionData
    )

/*++

Routine Description:

    This routine allows the caller to iterate through the sections in an error
    record.

Arguments:

    Record - Supplies a pointer to the error record.

    Context - Supplies a pointer to a variable that maintains the current state
        of the search. This variable should be zero for the first call, and the
        same variable should be used in subsequent calls to enumerate the next
        sections in the record.

    Descriptor - Supplies a location in which a pointer to the descriptor for
        the found section is returned.

    Section - Supplies an optional location in which a pointer to the found
        section is returned.

Return Value:

    STATUS_SUCCESS if the specified section is found.

    STATUS_NOT_FOUND if the specified section is not found.

    STATUS_INVALID_PARAMETER if the record does not appear well formed or a
        required parameter is null.

--*/

{

    NTSTATUS Status;
    PWHEA_ERROR_RECORD_SECTION_DESCRIPTOR Descriptor;
    ULONG Index;
    ULONG MinimumLength;

    if ((Record == NULL) ||
        (Context == NULL) ||
        (SectionDescriptor == NULL) ||
        (WheaIsValidErrorRecordSignature(Record) == FALSE) ||
        (Record->Header.SectionCount == 0)) {

        Status = STATUS_INVALID_PARAMETER;
        goto FindNextErrorRecordSectionEnd;
    }

    //
    // Ensure that the supplied record is at least as long as required to store
    // the descriptors for the sections supposedly in the record.
    //

    MinimumLength = sizeof(WHEA_ERROR_RECORD_HEADER) +
        (Record->Header.SectionCount *
         sizeof(WHEA_ERROR_RECORD_SECTION_DESCRIPTOR));

    if (Record->Header.Length < MinimumLength) {
        Status = STATUS_INVALID_PARAMETER;
        goto FindNextErrorRecordSectionEnd;
    }

    //
    // If the index is greater than the number of sections, then it has been
    // incorrectly fabricated by the caller or the record had section removed
    // during the enumeration. Either way, this is different to the case where
    // there are no sections left.
    //

    Index = *Context;
    if (Index > Record->Header.SectionCount) {
        Status = STATUS_INVALID_PARAMETER;
        goto FindNextErrorRecordSectionEnd;
    }

    if (Index == Record->Header.SectionCount) {
        Status = STATUS_NOT_FOUND;
        goto FindNextErrorRecordSectionEnd;
    }

    Descriptor = &Record->SectionDescriptor[Index];

    //
    // If the descriptor describes a section that is not completely contained
    // within the record then the record is invalid.
    //

    if ((Descriptor->SectionOffset + Descriptor->SectionLength) >
        Record->Header.Length) {

        Status = STATUS_INVALID_PARAMETER;
        goto FindNextErrorRecordSectionEnd;
    }

    *Context = Index + 1;
    *SectionDescriptor = Descriptor;
    if (SectionData != NULL) {
        *SectionData = (PVOID)(((PUCHAR)Record) + Descriptor->SectionOffset);
    }

    Status = STATUS_SUCCESS;

FindNextErrorRecordSectionEnd:
    return Status;
}

//
// SOC Subsystem bugcheck reporting information
//
typedef enum _SOC_SUBSYSTEM_TYPE {
    SOC_SUBSYS_WIRELESS_MODEM = 0,
    SOC_SUBSYS_AUDIO_DSP = 1,
    SOC_SUBSYS_WIRELSS_CONNECTIVITY = 2,
    SOC_SUBSYS_SENSORS = 3,
    SOC_SUBSYS_COMPUTE_DSP = 4,


    //
    // Subsystem types starting from 0x10000 are reserved for SoC vendor use.
    //

    SOC_SUBSYS_VENDOR_DEFINED = 0x10000
} SOC_SUBSYSTEM_TYPE, *PSOC_SUBSYSTEM_TYPE;


typedef struct _SOC_SUBSYSTEM_FAILURE_DETAILS {
    SOC_SUBSYSTEM_TYPE SubsysType;
    ULONG64 FirmwareVersion;
    ULONG64 HardwareVersion;
    ULONG   UnifiedFailureRegionSize;
    CHAR    UnifiedFailureRegion[1];
} SOC_SUBSYSTEM_FAILURE_DETAILS, *PSOC_SUBSYSTEM_FAILURE_DETAILS;



#ifdef __cplusplus
}
#endif

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4115)
#pragma warning(default:4201)
#pragma warning(default:4214)
#endif

#endif // _NTDDK_

