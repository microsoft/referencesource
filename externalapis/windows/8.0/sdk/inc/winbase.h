#include <winapifamily.h>

/************************************************************************
*                                                                       *
*   winbase.h -- This module defines the 32-Bit Windows Base APIs       *
*                                                                       *
*   Copyright (c) Microsoft Corp. All rights reserved.                  *
*                                                                       *
************************************************************************/
#ifndef _WINBASE_
#define _WINBASE_


#if defined(_MSC_VER)
#if _MSC_VER > 1000
#pragma once
#endif
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4001) /* nonstandard extension : single line comment */
#pragma warning(disable:4201) /* nonstandard extension used : nameless struct/union */
#pragma warning(disable:4214) /* nonstandard extension used : bit field types other then int */
#endif // defined(_MSC_VER)

#ifdef _MAC
#include <macwin32.h>
#endif //_MAC

#include <apisetcconv.h>
#include <minwinbase.h>

//
// APISET contracts
//

#include <processenv.h>
#include <fileapi.h>
#include <debugapi.h>
#include <utilapiset.h>
#include <handleapi.h>
#include <errhandlingapi.h>
#include <fibersapi.h>
#include <namedpipeapi.h>
#include <profileapi.h>
#include <heapapi.h>
#include <ioapiset.h>
#include <synchapi.h>
#include <interlockedapi.h>
#include <processthreadsapi.h>
#include <sysinfoapi.h>
#include <memoryapi.h>
#include <threadpoollegacyapiset.h>
#include <threadpoolapiset.h>
#include <bemapiset.h>
#include <jobapi.h>
#include <wow64apiset.h>
#include <libloaderapi.h>
#include <securitybaseapi.h>
#include <namespaceapi.h>
#include <systemtopologyapi.h>
#include <processtopologyapi.h>
#include <securityappcontainer.h>
#include <realtimeapiset.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma region Application Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)

/*
 * Compatibility macros
 */

#define DefineHandleTable(w)            ((w),TRUE)
#define LimitEmsPages(dw)
#define SetSwapAreaSize(w)              (w)
#define LockSegment(w)                  GlobalFix((HANDLE)(w))
#define UnlockSegment(w)                GlobalUnfix((HANDLE)(w))

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

#define GetCurrentTime()                GetTickCount()

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#pragma region Application Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)

#define Yield()

#define FILE_BEGIN           0
#define FILE_CURRENT         1
#define FILE_END             2

#define WAIT_FAILED ((DWORD)0xFFFFFFFF)
#define WAIT_OBJECT_0       ((STATUS_WAIT_0 ) + 0 )

#define WAIT_ABANDONED         ((STATUS_ABANDONED_WAIT_0 ) + 0 )
#define WAIT_ABANDONED_0       ((STATUS_ABANDONED_WAIT_0 ) + 0 )

#define WAIT_IO_COMPLETION                  STATUS_USER_APC

#define SecureZeroMemory RtlSecureZeroMemory
#define CaptureStackBackTrace RtlCaptureStackBackTrace

//
// File creation flags must start at the high end since they
// are combined with the attributes
//

//
//  These are flags supported through CreateFile (W7) and CreateFile2 (W8 and beyond)
//

#define FILE_FLAG_WRITE_THROUGH         0x80000000
#define FILE_FLAG_OVERLAPPED            0x40000000
#define FILE_FLAG_NO_BUFFERING          0x20000000
#define FILE_FLAG_RANDOM_ACCESS         0x10000000
#define FILE_FLAG_SEQUENTIAL_SCAN       0x08000000
#define FILE_FLAG_DELETE_ON_CLOSE       0x04000000
#define FILE_FLAG_BACKUP_SEMANTICS      0x02000000
#define FILE_FLAG_POSIX_SEMANTICS       0x01000000
#define FILE_FLAG_SESSION_AWARE         0x00800000
#define FILE_FLAG_OPEN_REPARSE_POINT    0x00200000
#define FILE_FLAG_OPEN_NO_RECALL        0x00100000
#define FILE_FLAG_FIRST_PIPE_INSTANCE   0x00080000

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)

//
//  These are flags supported only through CreateFile2 (W8 and beyond)
//
//  Due to the multiplexing of file creation flags, file attribute flags and
//  security QoS flags into a single DWORD (dwFlagsAndAttributes) parameter for
//  CreateFile, there is no way to add any more flags to CreateFile. Additional
//  flags for the create operation must be added to CreateFile2 only
//

#define FILE_FLAG_OPEN_REQUIRING_OPLOCK 0x00040000

#endif



#if(_WIN32_WINNT >= 0x0400)
//
// Define possible return codes from the CopyFileEx callback routine
//

#define PROGRESS_CONTINUE   0
#define PROGRESS_CANCEL     1
#define PROGRESS_STOP       2
#define PROGRESS_QUIET      3

//
// Define CopyFileEx callback routine state change values
//

#define CALLBACK_CHUNK_FINISHED         0x00000000
#define CALLBACK_STREAM_SWITCH          0x00000001

//
// Define CopyFileEx option flags
//

#define COPY_FILE_FAIL_IF_EXISTS              0x00000001
#define COPY_FILE_RESTARTABLE                 0x00000002
#define COPY_FILE_OPEN_SOURCE_FOR_WRITE       0x00000004
#define COPY_FILE_ALLOW_DECRYPTED_DESTINATION 0x00000008

//
//  Gap for private copyfile flags
//

#if (_WIN32_WINNT >= 0x0600)
#define COPY_FILE_COPY_SYMLINK                0x00000800
#define COPY_FILE_NO_BUFFERING                0x00001000
#endif


#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)

//
//  CopyFile2 flags
//

#define COPY_FILE_REQUEST_SECURITY_PRIVILEGES        0x00002000
#define COPY_FILE_RESUME_FROM_PAUSE                  0x00004000


#define COPY_FILE_NO_OFFLOAD                         0x00040000

#endif

#endif /* _WIN32_WINNT >= 0x0400 */

#if (_WIN32_WINNT >= 0x0500)
//
// Define ReplaceFile option flags
//

#define REPLACEFILE_WRITE_THROUGH       0x00000001
#define REPLACEFILE_IGNORE_MERGE_ERRORS 0x00000002

#if (_WIN32_WINNT >= 0x0600)
#define REPLACEFILE_IGNORE_ACL_ERRORS   0x00000004
#endif

#endif // #if (_WIN32_WINNT >= 0x0500)

//
// Define the NamedPipe definitions
//


//
// Define the dwOpenMode values for CreateNamedPipe
//

#define PIPE_ACCESS_INBOUND         0x00000001
#define PIPE_ACCESS_OUTBOUND        0x00000002
#define PIPE_ACCESS_DUPLEX          0x00000003

//
// Define the Named Pipe End flags for GetNamedPipeInfo
//

#define PIPE_CLIENT_END             0x00000000
#define PIPE_SERVER_END             0x00000001

//
// Define the dwPipeMode values for CreateNamedPipe
//

#define PIPE_WAIT                   0x00000000
#define PIPE_NOWAIT                 0x00000001
#define PIPE_READMODE_BYTE          0x00000000
#define PIPE_READMODE_MESSAGE       0x00000002
#define PIPE_TYPE_BYTE              0x00000000
#define PIPE_TYPE_MESSAGE           0x00000004
#define PIPE_ACCEPT_REMOTE_CLIENTS  0x00000000
#define PIPE_REJECT_REMOTE_CLIENTS  0x00000008

//
// Define the well known values for CreateNamedPipe nMaxInstances
//

#define PIPE_UNLIMITED_INSTANCES    255

//
// Define the Security Quality of Service bits to be passed
// into CreateFile
//

#define SECURITY_ANONYMOUS          ( SecurityAnonymous      << 16 )
#define SECURITY_IDENTIFICATION     ( SecurityIdentification << 16 )
#define SECURITY_IMPERSONATION      ( SecurityImpersonation  << 16 )
#define SECURITY_DELEGATION         ( SecurityDelegation     << 16 )

#define SECURITY_CONTEXT_TRACKING  0x00040000
#define SECURITY_EFFECTIVE_ONLY    0x00080000

#define SECURITY_SQOS_PRESENT      0x00100000
#define SECURITY_VALID_SQOS_FLAGS  0x001F0000


//
//  File structures
//

#if(_WIN32_WINNT >= 0x0400)
typedef VOID (WINAPI *PFIBER_START_ROUTINE)(
    LPVOID lpFiberParameter
    );
typedef PFIBER_START_ROUTINE LPFIBER_START_ROUTINE;
#endif /* _WIN32_WINNT >= 0x0400 */

//
// FailFast Exception Flags
//

#define FAIL_FAST_GENERATE_EXCEPTION_ADDRESS    0x1
#define FAIL_FAST_NO_HARD_ERROR_DLG             0x2

#if defined(_X86_)
typedef PLDT_ENTRY LPLDT_ENTRY;
#else
typedef LPVOID LPLDT_ENTRY;
#endif

//
// Serial provider type.
//

#define SP_SERIALCOMM    ((DWORD)0x00000001)

//
// Provider SubTypes
//

#define PST_UNSPECIFIED      ((DWORD)0x00000000)
#define PST_RS232            ((DWORD)0x00000001)
#define PST_PARALLELPORT     ((DWORD)0x00000002)
#define PST_RS422            ((DWORD)0x00000003)
#define PST_RS423            ((DWORD)0x00000004)
#define PST_RS449            ((DWORD)0x00000005)
#define PST_MODEM            ((DWORD)0x00000006)
#define PST_FAX              ((DWORD)0x00000021)
#define PST_SCANNER          ((DWORD)0x00000022)
#define PST_NETWORK_BRIDGE   ((DWORD)0x00000100)
#define PST_LAT              ((DWORD)0x00000101)
#define PST_TCPIP_TELNET     ((DWORD)0x00000102)
#define PST_X25              ((DWORD)0x00000103)

//
// Provider capabilities flags.
//

#define PCF_DTRDSR        ((DWORD)0x0001)
#define PCF_RTSCTS        ((DWORD)0x0002)
#define PCF_RLSD          ((DWORD)0x0004)
#define PCF_PARITY_CHECK  ((DWORD)0x0008)
#define PCF_XONXOFF       ((DWORD)0x0010)
#define PCF_SETXCHAR      ((DWORD)0x0020)
#define PCF_TOTALTIMEOUTS ((DWORD)0x0040)
#define PCF_INTTIMEOUTS   ((DWORD)0x0080)
#define PCF_SPECIALCHARS  ((DWORD)0x0100)
#define PCF_16BITMODE     ((DWORD)0x0200)

//
// Comm provider settable parameters.
//

#define SP_PARITY         ((DWORD)0x0001)
#define SP_BAUD           ((DWORD)0x0002)
#define SP_DATABITS       ((DWORD)0x0004)
#define SP_STOPBITS       ((DWORD)0x0008)
#define SP_HANDSHAKING    ((DWORD)0x0010)
#define SP_PARITY_CHECK   ((DWORD)0x0020)
#define SP_RLSD           ((DWORD)0x0040)

//
// Settable baud rates in the provider.
//

#define BAUD_075          ((DWORD)0x00000001)
#define BAUD_110          ((DWORD)0x00000002)
#define BAUD_134_5        ((DWORD)0x00000004)
#define BAUD_150          ((DWORD)0x00000008)
#define BAUD_300          ((DWORD)0x00000010)
#define BAUD_600          ((DWORD)0x00000020)
#define BAUD_1200         ((DWORD)0x00000040)
#define BAUD_1800         ((DWORD)0x00000080)
#define BAUD_2400         ((DWORD)0x00000100)
#define BAUD_4800         ((DWORD)0x00000200)
#define BAUD_7200         ((DWORD)0x00000400)
#define BAUD_9600         ((DWORD)0x00000800)
#define BAUD_14400        ((DWORD)0x00001000)
#define BAUD_19200        ((DWORD)0x00002000)
#define BAUD_38400        ((DWORD)0x00004000)
#define BAUD_56K          ((DWORD)0x00008000)
#define BAUD_128K         ((DWORD)0x00010000)
#define BAUD_115200       ((DWORD)0x00020000)
#define BAUD_57600        ((DWORD)0x00040000)
#define BAUD_USER         ((DWORD)0x10000000)

//
// Settable Data Bits
//

#define DATABITS_5        ((WORD)0x0001)
#define DATABITS_6        ((WORD)0x0002)
#define DATABITS_7        ((WORD)0x0004)
#define DATABITS_8        ((WORD)0x0008)
#define DATABITS_16       ((WORD)0x0010)
#define DATABITS_16X      ((WORD)0x0020)

//
// Settable Stop and Parity bits.
//

#define STOPBITS_10       ((WORD)0x0001)
#define STOPBITS_15       ((WORD)0x0002)
#define STOPBITS_20       ((WORD)0x0004)
#define PARITY_NONE       ((WORD)0x0100)
#define PARITY_ODD        ((WORD)0x0200)
#define PARITY_EVEN       ((WORD)0x0400)
#define PARITY_MARK       ((WORD)0x0800)
#define PARITY_SPACE      ((WORD)0x1000)

typedef struct _COMMPROP {
    WORD wPacketLength;
    WORD wPacketVersion;
    DWORD dwServiceMask;
    DWORD dwReserved1;
    DWORD dwMaxTxQueue;
    DWORD dwMaxRxQueue;
    DWORD dwMaxBaud;
    DWORD dwProvSubType;
    DWORD dwProvCapabilities;
    DWORD dwSettableParams;
    DWORD dwSettableBaud;
    WORD wSettableData;
    WORD wSettableStopParity;
    DWORD dwCurrentTxQueue;
    DWORD dwCurrentRxQueue;
    DWORD dwProvSpec1;
    DWORD dwProvSpec2;
    WCHAR wcProvChar[1];
} COMMPROP,*LPCOMMPROP;

//
// Set dwProvSpec1 to COMMPROP_INITIALIZED to indicate that wPacketLength
// is valid before a call to GetCommProperties().
//
#define COMMPROP_INITIALIZED ((DWORD)0xE73CF52E)

typedef struct _COMSTAT {
    DWORD fCtsHold : 1;
    DWORD fDsrHold : 1;
    DWORD fRlsdHold : 1;
    DWORD fXoffHold : 1;
    DWORD fXoffSent : 1;
    DWORD fEof : 1;
    DWORD fTxim : 1;
    DWORD fReserved : 25;
    DWORD cbInQue;
    DWORD cbOutQue;
} COMSTAT, *LPCOMSTAT;

//
// DTR Control Flow Values.
//
#define DTR_CONTROL_DISABLE    0x00
#define DTR_CONTROL_ENABLE     0x01
#define DTR_CONTROL_HANDSHAKE  0x02

//
// RTS Control Flow Values
//
#define RTS_CONTROL_DISABLE    0x00
#define RTS_CONTROL_ENABLE     0x01
#define RTS_CONTROL_HANDSHAKE  0x02
#define RTS_CONTROL_TOGGLE     0x03

typedef struct _DCB {
    DWORD DCBlength;      /* sizeof(DCB)                     */
    DWORD BaudRate;       /* Baudrate at which running       */
    DWORD fBinary: 1;     /* Binary Mode (skip EOF check)    */
    DWORD fParity: 1;     /* Enable parity checking          */
    DWORD fOutxCtsFlow:1; /* CTS handshaking on output       */
    DWORD fOutxDsrFlow:1; /* DSR handshaking on output       */
    DWORD fDtrControl:2;  /* DTR Flow control                */
    DWORD fDsrSensitivity:1; /* DSR Sensitivity              */
    DWORD fTXContinueOnXoff: 1; /* Continue TX when Xoff sent */
    DWORD fOutX: 1;       /* Enable output X-ON/X-OFF        */
    DWORD fInX: 1;        /* Enable input X-ON/X-OFF         */
    DWORD fErrorChar: 1;  /* Enable Err Replacement          */
    DWORD fNull: 1;       /* Enable Null stripping           */
    DWORD fRtsControl:2;  /* Rts Flow control                */
    DWORD fAbortOnError:1; /* Abort all reads and writes on Error */
    DWORD fDummy2:17;     /* Reserved                        */
    WORD wReserved;       /* Not currently used              */
    WORD XonLim;          /* Transmit X-ON threshold         */
    WORD XoffLim;         /* Transmit X-OFF threshold        */
    BYTE ByteSize;        /* Number of bits/byte, 4-8        */
    BYTE Parity;          /* 0-4=None,Odd,Even,Mark,Space    */
    BYTE StopBits;        /* 0,1,2 = 1, 1.5, 2               */
    char XonChar;         /* Tx and Rx X-ON character        */
    char XoffChar;        /* Tx and Rx X-OFF character       */
    char ErrorChar;       /* Error replacement char          */
    char EofChar;         /* End of Input character          */
    char EvtChar;         /* Received Event character        */
    WORD wReserved1;      /* Fill for now.                   */
} DCB, *LPDCB;

typedef struct _COMMTIMEOUTS {
    DWORD ReadIntervalTimeout;          /* Maximum time between read chars. */
    DWORD ReadTotalTimeoutMultiplier;   /* Multiplier of characters.        */
    DWORD ReadTotalTimeoutConstant;     /* Constant in milliseconds.        */
    DWORD WriteTotalTimeoutMultiplier;  /* Multiplier of characters.        */
    DWORD WriteTotalTimeoutConstant;    /* Constant in milliseconds.        */
} COMMTIMEOUTS,*LPCOMMTIMEOUTS;

typedef struct _COMMCONFIG {
    DWORD dwSize;               /* Size of the entire struct */
    WORD wVersion;              /* version of the structure */
    WORD wReserved;             /* alignment */
    DCB dcb;                    /* device control block */
    DWORD dwProviderSubType;    /* ordinal value for identifying
                                   provider-defined data structure format*/
    DWORD dwProviderOffset;     /* Specifies the offset of provider specific
                                   data field in bytes from the start */
    DWORD dwProviderSize;       /* size of the provider-specific data field */
    WCHAR wcProviderData[1];    /* provider-specific data */
} COMMCONFIG,*LPCOMMCONFIG;

//
//


#define FreeModule(hLibModule) FreeLibrary((hLibModule))
#define MakeProcInstance(lpProc,hInstance) (lpProc)
#define FreeProcInstance(lpProc) (lpProc)

/* Global Memory Flags */
#define GMEM_FIXED          0x0000
#define GMEM_MOVEABLE       0x0002
#define GMEM_NOCOMPACT      0x0010
#define GMEM_NODISCARD      0x0020
#define GMEM_ZEROINIT       0x0040
#define GMEM_MODIFY         0x0080
#define GMEM_DISCARDABLE    0x0100
#define GMEM_NOT_BANKED     0x1000
#define GMEM_SHARE          0x2000
#define GMEM_DDESHARE       0x2000
#define GMEM_NOTIFY         0x4000
#define GMEM_LOWER          GMEM_NOT_BANKED
#define GMEM_VALID_FLAGS    0x7F72
#define GMEM_INVALID_HANDLE 0x8000

#define GHND                (GMEM_MOVEABLE | GMEM_ZEROINIT)
#define GPTR                (GMEM_FIXED | GMEM_ZEROINIT)

#define GlobalLRUNewest( h )    ((HANDLE)(h))
#define GlobalLRUOldest( h )    ((HANDLE)(h))
#define GlobalDiscard( h )      GlobalReAlloc( (h), 0, GMEM_MOVEABLE )

/* Flags returned by GlobalFlags (in addition to GMEM_DISCARDABLE) */
#define GMEM_DISCARDED      0x4000
#define GMEM_LOCKCOUNT      0x00FF

typedef struct _MEMORYSTATUS {
    DWORD dwLength;
    DWORD dwMemoryLoad;
    SIZE_T dwTotalPhys;
    SIZE_T dwAvailPhys;
    SIZE_T dwTotalPageFile;
    SIZE_T dwAvailPageFile;
    SIZE_T dwTotalVirtual;
    SIZE_T dwAvailVirtual;
} MEMORYSTATUS, *LPMEMORYSTATUS;


//
// NUMA values
//
#define NUMA_NO_PREFERRED_NODE ((DWORD) -1)

//
// Process dwCreationFlag values
//

#define DEBUG_PROCESS                     0x00000001
#define DEBUG_ONLY_THIS_PROCESS           0x00000002
#define CREATE_SUSPENDED                  0x00000004
#define DETACHED_PROCESS                  0x00000008

#define CREATE_NEW_CONSOLE                0x00000010
#define NORMAL_PRIORITY_CLASS             0x00000020
#define IDLE_PRIORITY_CLASS               0x00000040
#define HIGH_PRIORITY_CLASS               0x00000080

#define REALTIME_PRIORITY_CLASS           0x00000100
#define CREATE_NEW_PROCESS_GROUP          0x00000200
#define CREATE_UNICODE_ENVIRONMENT        0x00000400
#define CREATE_SEPARATE_WOW_VDM           0x00000800

#define CREATE_SHARED_WOW_VDM             0x00001000
#define CREATE_FORCEDOS                   0x00002000
#define BELOW_NORMAL_PRIORITY_CLASS       0x00004000
#define ABOVE_NORMAL_PRIORITY_CLASS       0x00008000

#define INHERIT_PARENT_AFFINITY           0x00010000
#define INHERIT_CALLER_PRIORITY           0x00020000    // Deprecated
#define CREATE_PROTECTED_PROCESS          0x00040000
#define EXTENDED_STARTUPINFO_PRESENT      0x00080000

#define PROCESS_MODE_BACKGROUND_BEGIN     0x00100000
#define PROCESS_MODE_BACKGROUND_END       0x00200000

#define CREATE_BREAKAWAY_FROM_JOB         0x01000000
#define CREATE_PRESERVE_CODE_AUTHZ_LEVEL  0x02000000
#define CREATE_DEFAULT_ERROR_MODE         0x04000000
#define CREATE_NO_WINDOW                  0x08000000

#define PROFILE_USER                      0x10000000
#define PROFILE_KERNEL                    0x20000000
#define PROFILE_SERVER                    0x40000000
#define CREATE_IGNORE_SYSTEM_DEFAULT      0x80000000

//
// Thread dwCreationFlag values
//

//#define CREATE_SUSPENDED                  0x00000004

#define STACK_SIZE_PARAM_IS_A_RESERVATION   0x00010000    // Threads only

//
// Priority flags
//

#define THREAD_PRIORITY_LOWEST          THREAD_BASE_PRIORITY_MIN
#define THREAD_PRIORITY_BELOW_NORMAL    (THREAD_PRIORITY_LOWEST+1)
#define THREAD_PRIORITY_NORMAL          0
#define THREAD_PRIORITY_HIGHEST         THREAD_BASE_PRIORITY_MAX
#define THREAD_PRIORITY_ABOVE_NORMAL    (THREAD_PRIORITY_HIGHEST-1)
#define THREAD_PRIORITY_ERROR_RETURN    (MAXLONG)

#define THREAD_PRIORITY_TIME_CRITICAL   THREAD_BASE_PRIORITY_LOWRT
#define THREAD_PRIORITY_IDLE            THREAD_BASE_PRIORITY_IDLE

#define THREAD_MODE_BACKGROUND_BEGIN    0x00010000
#define THREAD_MODE_BACKGROUND_END      0x00020000

//
// GetFinalPathNameByHandle
//

#define VOLUME_NAME_DOS  0x0      //default
#define VOLUME_NAME_GUID 0x1
#define VOLUME_NAME_NT   0x2
#define VOLUME_NAME_NONE 0x4

#define FILE_NAME_NORMALIZED 0x0  //default
#define FILE_NAME_OPENED     0x8

//
// JIT Debugging Info. This structure is defined to have constant size in
// both the emulated and native environment.
//

typedef struct _JIT_DEBUG_INFO {
    DWORD dwSize;
    DWORD dwProcessorArchitecture;
    DWORD dwThreadID;
    DWORD dwReserved0;
    ULONG64 lpExceptionAddress;
    ULONG64 lpExceptionRecord;
    ULONG64 lpContextRecord;
} JIT_DEBUG_INFO, *LPJIT_DEBUG_INFO;

typedef JIT_DEBUG_INFO JIT_DEBUG_INFO32, *LPJIT_DEBUG_INFO32;
typedef JIT_DEBUG_INFO JIT_DEBUG_INFO64, *LPJIT_DEBUG_INFO64;

#if !defined(MIDL_PASS)
typedef PEXCEPTION_RECORD LPEXCEPTION_RECORD;
typedef PEXCEPTION_POINTERS LPEXCEPTION_POINTERS;
#endif

#define DRIVE_UNKNOWN     0
#define DRIVE_NO_ROOT_DIR 1
#define DRIVE_REMOVABLE   2
#define DRIVE_FIXED       3
#define DRIVE_REMOTE      4
#define DRIVE_CDROM       5
#define DRIVE_RAMDISK     6


#ifndef _MAC
#define GetFreeSpace(w)                 (0x100000L)
#else
WINBASEAPI DWORD WINAPI GetFreeSpace(_In_ UINT);
#endif


#define FILE_TYPE_UNKNOWN   0x0000
#define FILE_TYPE_DISK      0x0001
#define FILE_TYPE_CHAR      0x0002
#define FILE_TYPE_PIPE      0x0003
#define FILE_TYPE_REMOTE    0x8000


#define STD_INPUT_HANDLE    ((DWORD)-10)
#define STD_OUTPUT_HANDLE   ((DWORD)-11)
#define STD_ERROR_HANDLE    ((DWORD)-12)

#define NOPARITY            0
#define ODDPARITY           1
#define EVENPARITY          2
#define MARKPARITY          3
#define SPACEPARITY         4

#define ONESTOPBIT          0
#define ONE5STOPBITS        1
#define TWOSTOPBITS         2

#define IGNORE              0       // Ignore signal
#define INFINITE            0xFFFFFFFF  // Infinite timeout

//
// Baud rates at which the communication device operates
//

#define CBR_110             110
#define CBR_300             300
#define CBR_600             600
#define CBR_1200            1200
#define CBR_2400            2400
#define CBR_4800            4800
#define CBR_9600            9600
#define CBR_14400           14400
#define CBR_19200           19200
#define CBR_38400           38400
#define CBR_56000           56000
#define CBR_57600           57600
#define CBR_115200          115200
#define CBR_128000          128000
#define CBR_256000          256000

//
// Error Flags
//

#define CE_RXOVER           0x0001  // Receive Queue overflow
#define CE_OVERRUN          0x0002  // Receive Overrun Error
#define CE_RXPARITY         0x0004  // Receive Parity Error
#define CE_FRAME            0x0008  // Receive Framing error
#define CE_BREAK            0x0010  // Break Detected
#define CE_TXFULL           0x0100  // TX Queue is full
#define CE_PTO              0x0200  // LPTx Timeout
#define CE_IOE              0x0400  // LPTx I/O Error
#define CE_DNS              0x0800  // LPTx Device not selected
#define CE_OOP              0x1000  // LPTx Out-Of-Paper
#define CE_MODE             0x8000  // Requested mode unsupported

#define IE_BADID            (-1)    // Invalid or unsupported id
#define IE_OPEN             (-2)    // Device Already Open
#define IE_NOPEN            (-3)    // Device Not Open
#define IE_MEMORY           (-4)    // Unable to allocate queues
#define IE_DEFAULT          (-5)    // Error in default parameters
#define IE_HARDWARE         (-10)   // Hardware Not Present
#define IE_BYTESIZE         (-11)   // Illegal Byte Size
#define IE_BAUDRATE         (-12)   // Unsupported BaudRate

//
// Events
//

#define EV_RXCHAR           0x0001  // Any Character received
#define EV_RXFLAG           0x0002  // Received certain character
#define EV_TXEMPTY          0x0004  // Transmitt Queue Empty
#define EV_CTS              0x0008  // CTS changed state
#define EV_DSR              0x0010  // DSR changed state
#define EV_RLSD             0x0020  // RLSD changed state
#define EV_BREAK            0x0040  // BREAK received
#define EV_ERR              0x0080  // Line status error occurred
#define EV_RING             0x0100  // Ring signal detected
#define EV_PERR             0x0200  // Printer error occured
#define EV_RX80FULL         0x0400  // Receive buffer is 80 percent full
#define EV_EVENT1           0x0800  // Provider specific event 1
#define EV_EVENT2           0x1000  // Provider specific event 2

//
// Escape Functions
//

#define SETXOFF             1       // Simulate XOFF received
#define SETXON              2       // Simulate XON received
#define SETRTS              3       // Set RTS high
#define CLRRTS              4       // Set RTS low
#define SETDTR              5       // Set DTR high
#define CLRDTR              6       // Set DTR low
#define RESETDEV            7       // Reset device if possible
#define SETBREAK            8       // Set the device break line.
#define CLRBREAK            9       // Clear the device break line.

//
// PURGE function flags.
//
#define PURGE_TXABORT       0x0001  // Kill the pending/current writes to the comm port.
#define PURGE_RXABORT       0x0002  // Kill the pending/current reads to the comm port.
#define PURGE_TXCLEAR       0x0004  // Kill the transmit queue if there.
#define PURGE_RXCLEAR       0x0008  // Kill the typeahead buffer if there.

#define LPTx                0x80    // Set if ID is for LPT device

//
// Modem Status Flags
//
#define MS_CTS_ON           ((DWORD)0x0010)
#define MS_DSR_ON           ((DWORD)0x0020)
#define MS_RING_ON          ((DWORD)0x0040)
#define MS_RLSD_ON          ((DWORD)0x0080)

//
// WaitSoundState() Constants
//

#define S_QUEUEEMPTY        0
#define S_THRESHOLD         1
#define S_ALLTHRESHOLD      2

//
// Accent Modes
//

#define S_NORMAL      0
#define S_LEGATO      1
#define S_STACCATO    2

//
// SetSoundNoise() Sources
//

#define S_PERIOD512   0     // Freq = N/512 high pitch, less coarse hiss
#define S_PERIOD1024  1     // Freq = N/1024
#define S_PERIOD2048  2     // Freq = N/2048 low pitch, more coarse hiss
#define S_PERIODVOICE 3     // Source is frequency from voice channel (3)
#define S_WHITE512    4     // Freq = N/512 high pitch, less coarse hiss
#define S_WHITE1024   5     // Freq = N/1024
#define S_WHITE2048   6     // Freq = N/2048 low pitch, more coarse hiss
#define S_WHITEVOICE  7     // Source is frequency from voice channel (3)

#define S_SERDVNA     (-1)  // Device not available
#define S_SEROFM      (-2)  // Out of memory
#define S_SERMACT     (-3)  // Music active
#define S_SERQFUL     (-4)  // Queue full
#define S_SERBDNT     (-5)  // Invalid note
#define S_SERDLN      (-6)  // Invalid note length
#define S_SERDCC      (-7)  // Invalid note count
#define S_SERDTP      (-8)  // Invalid tempo
#define S_SERDVL      (-9)  // Invalid volume
#define S_SERDMD      (-10) // Invalid mode
#define S_SERDSH      (-11) // Invalid shape
#define S_SERDPT      (-12) // Invalid pitch
#define S_SERDFQ      (-13) // Invalid frequency
#define S_SERDDR      (-14) // Invalid duration
#define S_SERDSR      (-15) // Invalid source
#define S_SERDST      (-16) // Invalid state

#define NMPWAIT_WAIT_FOREVER            0xffffffff
#define NMPWAIT_NOWAIT                  0x00000001
#define NMPWAIT_USE_DEFAULT_WAIT        0x00000000

#define FS_CASE_IS_PRESERVED            FILE_CASE_PRESERVED_NAMES
#define FS_CASE_SENSITIVE               FILE_CASE_SENSITIVE_SEARCH
#define FS_UNICODE_STORED_ON_DISK       FILE_UNICODE_ON_DISK
#define FS_PERSISTENT_ACLS              FILE_PERSISTENT_ACLS
#define FS_VOL_IS_COMPRESSED            FILE_VOLUME_IS_COMPRESSED
#define FS_FILE_COMPRESSION             FILE_FILE_COMPRESSION
#define FS_FILE_ENCRYPTION              FILE_SUPPORTS_ENCRYPTION

#define OF_READ             0x00000000
#define OF_WRITE            0x00000001
#define OF_READWRITE        0x00000002
#define OF_SHARE_COMPAT     0x00000000
#define OF_SHARE_EXCLUSIVE  0x00000010
#define OF_SHARE_DENY_WRITE 0x00000020
#define OF_SHARE_DENY_READ  0x00000030
#define OF_SHARE_DENY_NONE  0x00000040
#define OF_PARSE            0x00000100
#define OF_DELETE           0x00000200
#define OF_VERIFY           0x00000400
#define OF_CANCEL           0x00000800
#define OF_CREATE           0x00001000
#define OF_PROMPT           0x00002000
#define OF_EXIST            0x00004000
#define OF_REOPEN           0x00008000

#define OFS_MAXPATHNAME 128
typedef struct _OFSTRUCT {
    BYTE cBytes;
    BYTE fFixedDisk;
    WORD nErrCode;
    WORD Reserved1;
    WORD Reserved2;
    CHAR szPathName[OFS_MAXPATHNAME];
} OFSTRUCT, *LPOFSTRUCT, *POFSTRUCT;

#ifndef NOWINBASEINTERLOCK

#ifndef _NTOS_

#if defined(_M_IA64) && !defined(RC_INVOKED)

#define InterlockedIncrement _InterlockedIncrement
#define InterlockedIncrementAcquire _InterlockedIncrement_acq
#define InterlockedIncrementRelease _InterlockedIncrement_rel
#define InterlockedDecrement _InterlockedDecrement
#define InterlockedDecrementAcquire _InterlockedDecrement_acq
#define InterlockedDecrementRelease _InterlockedDecrement_rel
#define InterlockedExchange _InterlockedExchange
#define InterlockedExchangeAdd _InterlockedExchangeAdd
#define InterlockedCompareExchange _InterlockedCompareExchange
#define InterlockedCompareExchangeAcquire _InterlockedCompareExchange_acq
#define InterlockedCompareExchangeRelease _InterlockedCompareExchange_rel
#define InterlockedExchangePointer _InterlockedExchangePointer
#define InterlockedCompareExchangePointer _InterlockedCompareExchangePointer
#define InterlockedCompareExchangePointerRelease _InterlockedCompareExchangePointer_rel
#define InterlockedCompareExchangePointerAcquire _InterlockedCompareExchangePointer_acq

#define InterlockedIncrement64 _InterlockedIncrement64
#define InterlockedDecrement64 _InterlockedDecrement64
#define InterlockedExchange64 _InterlockedExchange64
#define InterlockedExchangeAcquire64 _InterlockedExchange64_acq
#define InterlockedExchangeAdd64 _InterlockedExchangeAdd64
#define InterlockedCompareExchange64 _InterlockedCompareExchange64
#define InterlockedCompareExchangeAcquire64 _InterlockedCompareExchange64_acq
#define InterlockedCompareExchangeRelease64 _InterlockedCompareExchange64_rel
#define InterlockedCompare64Exchange128     _InterlockedCompare64Exchange128
#define InterlockedCompare64ExchangeAcquire128  _InterlockedCompare64Exchange128_acq
#define InterlockedCompare64ExchangeRelease128  _InterlockedCompare64Exchange128_rel

#define InterlockedOr _InterlockedOr
#define InterlockedOrAcquire _InterlockedOr_acq
#define InterlockedOrRelease _InterlockedOr_rel
#define InterlockedOr8 _InterlockedOr8
#define InterlockedOr8Acquire _InterlockedOr8_acq
#define InterlockedOr8Release _InterlockedOr8_rel
#define InterlockedOr16 _InterlockedOr16
#define InterlockedOr16Acquire _InterlockedOr16_acq
#define InterlockedOr16Release _InterlockedOr16_rel
#define InterlockedOr64 _InterlockedOr64
#define InterlockedOr64Acquire _InterlockedOr64_acq
#define InterlockedOr64Release _InterlockedOr64_rel
#define InterlockedXor _InterlockedXor
#define InterlockedXorAcquire _InterlockedXor_acq
#define InterlockedXorRelease _InterlockedXor_rel
#define InterlockedXor8 _InterlockedXor8
#define InterlockedXor8Acquire _InterlockedXor8_acq
#define InterlockedXor8Release _InterlockedXor8_rel
#define InterlockedXor16 _InterlockedXor16
#define InterlockedXor16Acquire _InterlockedXor16_acq
#define InterlockedXor16Release _InterlockedXor16_rel
#define InterlockedXor64 _InterlockedXor64
#define InterlockedXor64Acquire _InterlockedXor64_acq
#define InterlockedXor64Release _InterlockedXor64_rel
#define InterlockedAnd _InterlockedAnd
#define InterlockedAndAcquire _InterlockedAnd_acq
#define InterlockedAndRelease _InterlockedAnd_rel
#define InterlockedAnd8 _InterlockedAnd8
#define InterlockedAnd8Acquire _InterlockedAnd8_acq
#define InterlockedAnd8Release _InterlockedAnd8_rel
#define InterlockedAnd16 _InterlockedAnd16
#define InterlockedAnd16Acquire _InterlockedAnd16_acq
#define InterlockedAnd16Release _InterlockedAnd16_rel
#define InterlockedAnd64 _InterlockedAnd64
#define InterlockedAnd64Acquire _InterlockedAnd64_acq
#define InterlockedAnd64Release _InterlockedAnd64_rel

LONG
__cdecl
InterlockedOr (
    _Inout_ LONG volatile *Destination,
    _In_    LONG Value
    );

LONG
__cdecl
InterlockedOrAcquire (
    _Inout_ LONG volatile *Destination,
    _In_    LONG Value
    );

LONG
__cdecl
InterlockedOrRelease (
    _Inout_ LONG volatile *Destination,
    _In_    LONG Value
    );

char
__cdecl
InterlockedOr8 (
    _Inout_ char volatile *Destination,
    _In_    char Value
    );

char
__cdecl
InterlockedOr8Acquire (
    _Inout_ char volatile *Destination,
    _In_    char Value
    );

char
__cdecl
InterlockedOr8Release (
    _Inout_ char volatile *Destination,
    _In_    char Value
    );

SHORT
__cdecl
InterlockedOr16(
    _Inout_ SHORT volatile *Destination,
    _In_    SHORT Value
    );

SHORT
__cdecl
InterlockedOr16Acquire (
    _Inout_ SHORT volatile *Destination,
    _In_    SHORT Value
    );

SHORT
__cdecl
InterlockedOr16Release (
    _Inout_ SHORT volatile *Destination,
    _In_    SHORT Value
    );

LONGLONG
__cdecl
InterlockedOr64 (
    _Inout_ LONGLONG volatile *Destination,
    _In_    LONGLONG Value
    );

LONGLONG
__cdecl
InterlockedOr64Acquire (
    _Inout_ LONGLONG volatile *Destination,
    _In_    LONGLONG Value
    );

LONGLONG
__cdecl
InterlockedOr64Release (
    _Inout_ LONGLONG volatile *Destination,
    _In_    LONGLONG Value
    );

LONG
__cdecl
InterlockedXor (
    _Inout_ LONG volatile *Destination,
    _In_    LONG Value
    );

LONG
__cdecl
InterlockedXorAcquire (
    _Inout_ LONG volatile *Destination,
    _In_    LONG Value
    );

LONG
__cdecl
InterlockedXorRelease (
    _Inout_ LONG volatile *Destination,
    _In_    LONG Value
    );

char
__cdecl
InterlockedXor8 (
    _Inout_ char volatile *Destination,
    _In_    char Value
    );

char
__cdecl
InterlockedXor8Acquire (
    _Inout_ char volatile *Destination,
    _In_    char Value
    );

char
__cdecl
InterlockedXor8Release (
    _Inout_ char volatile *Destination,
    _In_    char Value
    );

SHORT
__cdecl
InterlockedXor16(
    _Inout_ SHORT volatile *Destination,
    _In_    SHORT Value
    );

SHORT
__cdecl
InterlockedXor16Acquire (
    _Inout_ SHORT volatile *Destination,
    _In_    SHORT Value
    );

SHORT
__cdecl
InterlockedXor16Release (
    _Inout_ SHORT volatile *Destination,
    _In_    SHORT Value
    );

LONGLONG
__cdecl
InterlockedXor64 (
    _Inout_ LONGLONG volatile *Destination,
    _In_    LONGLONG Value
    );

LONGLONG
__cdecl
InterlockedXor64Acquire (
    _Inout_ LONGLONG volatile *Destination,
    _In_    LONGLONG Value
    );

LONGLONG
__cdecl
InterlockedXor64Release (
    _Inout_ LONGLONG volatile *Destination,
    _In_    LONGLONG Value
    );

LONG
__cdecl
InterlockedAnd (
    _Inout_ LONG volatile *Destination,
    _In_    LONG Value
    );

LONG
__cdecl
InterlockedAndAcquire (
    _Inout_ LONG volatile *Destination,
    _In_    LONG Value
    );

LONG
__cdecl
InterlockedAndRelease (
    _Inout_ LONG volatile *Destination,
    _In_    LONG Value
    );

char
__cdecl
InterlockedAnd8 (
    _Inout_ char volatile *Destination,
    _In_    char Value
    );

char
__cdecl
InterlockedAnd8Acquire (
    _Inout_ char volatile *Destination,
    _In_    char Value
    );

char
__cdecl
InterlockedAnd8Release (
    _Inout_ char volatile *Destination,
    _In_    char Value
    );

SHORT
__cdecl
InterlockedAnd16(
    _Inout_ SHORT volatile *Destination,
    _In_    SHORT Value
    );

SHORT
__cdecl
InterlockedAnd16Acquire (
    _Inout_ SHORT volatile *Destination,
    _In_    SHORT Value
    );

SHORT
__cdecl
InterlockedAnd16Release (
    _Inout_ SHORT volatile *Destination,
    _In_    SHORT Value
    );

LONGLONG
__cdecl
InterlockedAnd64 (
    _Inout_ LONGLONG volatile *Destination,
    _In_    LONGLONG Value
    );

LONGLONG
__cdecl
InterlockedAnd64Acquire (
    _Inout_ LONGLONG volatile *Destination,
    _In_    LONGLONG Value
    );

LONGLONG
__cdecl
InterlockedAnd64Release (
    _Inout_ LONGLONG volatile *Destination,
    _In_    LONGLONG Value
    );

LONGLONG
__cdecl
InterlockedIncrement64 (
    _Inout_ LONGLONG volatile *Addend
    );

LONGLONG
__cdecl
InterlockedDecrement64 (
    _Inout_ LONGLONG volatile *Addend
    );

LONG
__cdecl
InterlockedIncrementAcquire (
    _Inout_ LONG volatile *Addend
    );

LONG
__cdecl
InterlockedDecrementAcquire (
    _Inout_ LONG volatile *Addend
    );

LONG
__cdecl
InterlockedIncrementRelease (
    _Inout_ LONG volatile *Addend
    );

LONG
__cdecl
InterlockedDecrementRelease (
    _Inout_ LONG volatile *Addend
    );

LONGLONG
__cdecl
InterlockedExchange64 (
    _Inout_ LONGLONG volatile *Target,
    _In_    LONGLONG Value
    );

LONGLONG
__cdecl
InterlockedExchangeAcquire64 (
    _Inout_ LONGLONG volatile *Target,
    _In_    LONGLONG Value
    );

LONGLONG
__cdecl
InterlockedExchangeAdd64 (
    _Inout_ LONGLONG volatile *Addend,
    _In_    LONGLONG Value
    );

LONGLONG
__cdecl
InterlockedCompareExchange64 (
    _Inout_ LONGLONG volatile *Destination,
    _In_    LONGLONG ExChange,
    _In_    LONGLONG Comperand
    );

LONGLONG
__cdecl
InterlockedCompareExchangeAcquire64 (
    _Inout_ LONGLONG volatile *Destination,
    _In_    LONGLONG ExChange,
    _In_    LONGLONG Comperand
    );

LONGLONG
__cdecl
InterlockedCompareExchangeRelease64 (
    _Inout_ LONGLONG volatile *Destination,
    _In_    LONGLONG ExChange,
    _In_    LONGLONG Comperand
    );

LONG64
__cdecl
InterlockedCompare64Exchange128(
    _Inout_ LONG64 volatile * Destination,
    _In_ LONG64 ExchangeHigh,
    _In_ LONG64 ExchangeLow,
    _In_ LONG64 Comperand
    );

LONG64
__cdecl
InterlockedCompare64ExchangeAcquire128(
    _Inout_ LONG64 volatile * Destination,
    _In_ LONG64 ExchangeHigh,
    _In_ LONG64 ExchangeLow,
    _In_ LONG64 Comperand
    );

LONG64
__cdecl
InterlockedCompare64ExchangeRelease128(
    _Inout_ LONG64 volatile * Destination,
    _In_ LONG64 ExchangeHigh,
    _In_ LONG64 ExchangeLow,
    _In_ LONG64 Comperand
    );

LONG
__cdecl
InterlockedIncrement (
    _Inout_ LONG volatile *lpAddend
    );

LONG
__cdecl
InterlockedDecrement (
    _Inout_ LONG volatile *lpAddend
    );

LONG
__cdecl
InterlockedExchange (
    _Inout_ LONG volatile *Target,
    _In_    LONG Value
    );

LONG
__cdecl
InterlockedExchangeAdd (
    _Inout_ LONG volatile *Addend,
    _In_    LONG Value
    );

LONG
__cdecl
InterlockedCompareExchange (
    _Inout_ LONG volatile *Destination,
    _In_    LONG ExChange,
    _In_    LONG Comperand
    );

LONG
__cdecl
InterlockedCompareExchangeRelease (
    _Inout_ LONG volatile *Destination,
    _In_    LONG ExChange,
    _In_    LONG Comperand
    );

LONG
__cdecl
InterlockedCompareExchangeAcquire (
    _Inout_ LONG volatile *Destination,
    _In_    LONG ExChange,
    _In_    LONG Comperand
    );

PVOID
__cdecl
InterlockedExchangePointer (
    _Inout_  PVOID volatile *Target,
    _In_opt_ PVOID Value
    );

PVOID
__cdecl
InterlockedCompareExchangePointer (
    _Inout_  PVOID volatile *Destination,
    _In_opt_ PVOID ExChange,
    _In_opt_ PVOID Comperand
    );

PVOID
__cdecl
InterlockedCompareExchangePointerAcquire (
    _Inout_  PVOID volatile *Destination,
    _In_opt_ PVOID Exchange,
    _In_opt_ PVOID Comperand
    );

PVOID
__cdecl
InterlockedCompareExchangePointerRelease (
    _Inout_  PVOID volatile *Destination,
    _In_opt_ PVOID Exchange,
    _In_opt_ PVOID Comperand
    );


#if !defined(MIDL_PASS)

#if !defined (InterlockedAnd)

#define InterlockedAnd InterlockedAnd_Inline

FORCEINLINE
LONG
InterlockedAnd_Inline (
    _Inout_ LONG volatile *Target,
    _In_    LONG Set
    )
{
    LONG i;
    LONG j;

    j = *Target;
    do {
        i = j;
        j = InterlockedCompareExchange(Target,
                                       i & Set,
                                       i);

    } while (i != j);

    return j;
}

#endif

#if !defined (InterlockedOr)

#define InterlockedOr InterlockedOr_Inline

FORCEINLINE
LONG
InterlockedOr_Inline (
    _Inout_ LONG volatile *Target,
    _In_    LONG Set
    )
{
    LONG i;
    LONG j;

    j = *Target;
    do {
        i = j;
        j = InterlockedCompareExchange(Target,
                                       i | Set,
                                       i);

    } while (i != j);

    return j;
}

#endif

#if !defined (InterlockedXor)

#define InterlockedXor InterlockedXor_Inline

FORCEINLINE
LONG
InterlockedXor_Inline (
    _Inout_ LONG volatile *Target,
    _In_    LONG Set
    )
{
    LONG i;
    LONG j;

    j = *Target;
    do {
        i = j;
        j = InterlockedCompareExchange(Target,
                                       i ^ Set,
                                       i);

    } while (i != j);

    return j;
}

#endif

#if !defined (InterlockedAnd64)

#define InterlockedAnd64 InterlockedAnd64_Inline

FORCEINLINE
LONGLONG
InterlockedAnd64_Inline (
    _Inout_ LONGLONG volatile *Destination,
    _In_    LONGLONG Value
    )
{
    LONGLONG Old;

    do {
        Old = *Destination;
    } while (InterlockedCompareExchange64(Destination,
                                          Old & Value,
                                          Old) != Old);

    return Old;
}

#endif

#if !defined (InterlockedOr64)

#define InterlockedOr64 InterlockedOr64_Inline

FORCEINLINE
LONGLONG
InterlockedOr64_Inline (
    _Inout_ LONGLONG volatile *Destination,
    _In_    LONGLONG Value
    )
{
    LONGLONG Old;

    do {
        Old = *Destination;
    } while (InterlockedCompareExchange64(Destination,
                                          Old | Value,
                                          Old) != Old);

    return Old;
}

#endif

#if !defined (InterlockedXor64)

#define InterlockedXor64 InterlockedXor64_Inline

FORCEINLINE
LONGLONG
InterlockedXor64_Inline (
    _Inout_ LONGLONG volatile *Destination,
    _In_    LONGLONG Value
    )
{
    LONGLONG Old;

    do {
        Old = *Destination;
    } while (InterlockedCompareExchange64(Destination,
                                          Old ^ Value,
                                          Old) != Old);

    return Old;
}

#endif

#if !defined (InterlockedBitTestAndSet)

#define InterlockedBitTestAndSet InterlockedBitTestAndSet_Inline

FORCEINLINE
BOOLEAN
InterlockedBitTestAndSet_Inline (
    _Inout_ LONG volatile *Base,
    _In_ LONG Bit
    )
{
    LONG tBit;

    tBit = 1<<(Bit & (sizeof (*Base)*8-1));
    return (BOOLEAN)((InterlockedOr(&Base[Bit/(sizeof(*Base)*8)], tBit)&tBit) != 0);
}

#endif

#if !defined (InterlockedBitTestAndReset)

#define InterlockedBitTestAndReset InterlockedBitTestAndReset_Inline

FORCEINLINE
BOOLEAN
InterlockedBitTestAndReset_Inline (
    _Inout_ LONG volatile *Base,
    _In_ LONG Bit
    )
{
    LONG tBit;

    tBit = 1<<(Bit & (sizeof (*Base)*8-1));
    return (BOOLEAN)((InterlockedAnd(&Base[Bit/(sizeof(*Base)*8)], ~tBit)&tBit) != 0);
}

#endif

#if !defined (InterlockedBitTestAndComplement)

#define InterlockedBitTestAndComplement InterlockedBitTestAndComplement_Inline

FORCEINLINE
BOOLEAN
InterlockedBitTestAndComplement_Inline (
    _Inout_ LONG volatile *Base,
    _In_ LONG Bit
    )
{
    LONG tBit;

    tBit = 1<<(Bit & (sizeof (*Base)*8-1));
    return (BOOLEAN)((InterlockedXor(&Base[Bit/(sizeof(*Base)*8)], tBit)&tBit) != 0);
}

#endif
#endif

#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedIncrement_acq)
#pragma intrinsic(_InterlockedIncrement_rel)
#pragma intrinsic(_InterlockedDecrement)
#pragma intrinsic(_InterlockedDecrement_acq)
#pragma intrinsic(_InterlockedDecrement_rel)
#pragma intrinsic(_InterlockedExchange)
#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedCompareExchange)
#pragma intrinsic(_InterlockedCompareExchange_acq)
#pragma intrinsic(_InterlockedCompareExchange_rel)
#pragma intrinsic(_InterlockedExchangePointer)
#pragma intrinsic(_InterlockedCompareExchangePointer)
#pragma intrinsic(_InterlockedCompareExchangePointer_acq)
#pragma intrinsic(_InterlockedCompareExchangePointer_rel)
#pragma intrinsic(_InterlockedIncrement64)
#pragma intrinsic(_InterlockedDecrement64)
#pragma intrinsic(_InterlockedExchange64)
#pragma intrinsic(_InterlockedExchange64_acq)
#pragma intrinsic(_InterlockedCompareExchange64)
#pragma intrinsic(_InterlockedCompareExchange64_acq)
#pragma intrinsic(_InterlockedCompareExchange64_rel)
#pragma intrinsic(_InterlockedExchangeAdd64)
#pragma intrinsic (_InterlockedOr)
#pragma intrinsic (_InterlockedOr_acq)
#pragma intrinsic (_InterlockedOr_rel)
#pragma intrinsic (_InterlockedOr8)
#pragma intrinsic (_InterlockedOr8_acq)
#pragma intrinsic (_InterlockedOr8_rel)
#pragma intrinsic (_InterlockedOr16)
#pragma intrinsic (_InterlockedOr16_acq)
#pragma intrinsic (_InterlockedOr16_rel)
#pragma intrinsic (_InterlockedOr64)
#pragma intrinsic (_InterlockedOr64_acq)
#pragma intrinsic (_InterlockedOr64_rel)
#pragma intrinsic (_InterlockedXor)
#pragma intrinsic (_InterlockedXor_acq)
#pragma intrinsic (_InterlockedXor_rel)
#pragma intrinsic (_InterlockedXor8)
#pragma intrinsic (_InterlockedXor8_acq)
#pragma intrinsic (_InterlockedXor8_rel)
#pragma intrinsic (_InterlockedXor16)
#pragma intrinsic (_InterlockedXor16_acq)
#pragma intrinsic (_InterlockedXor16_rel)
#pragma intrinsic (_InterlockedXor64)
#pragma intrinsic (_InterlockedXor64_acq)
#pragma intrinsic (_InterlockedXor64_rel)
#pragma intrinsic (_InterlockedAnd)
#pragma intrinsic (_InterlockedAnd_acq)
#pragma intrinsic (_InterlockedAnd_rel)
#pragma intrinsic (_InterlockedAnd8)
#pragma intrinsic (_InterlockedAnd8_acq)
#pragma intrinsic (_InterlockedAnd8_rel)
#pragma intrinsic (_InterlockedAnd16)
#pragma intrinsic (_InterlockedAnd16_acq)
#pragma intrinsic (_InterlockedAnd16_rel)
#pragma intrinsic (_InterlockedAnd64)
#pragma intrinsic (_InterlockedAnd64_acq)
#pragma intrinsic (_InterlockedAnd64_rel)

#elif defined(_M_AMD64) && !defined(RC_INVOKED)


LONG
InterlockedAnd (
    _Inout_ LONG volatile *Destination,
    _In_ LONG Value
    );

LONG
InterlockedOr (
    _Inout_ LONG volatile *Destination,
    _In_ LONG Value
    );

LONG
InterlockedXor (
    _Inout_ LONG volatile *Destination,
    _In_ LONG Value
    );

LONG
InterlockedIncrement (
    _Inout_ LONG volatile *Addend
    );

LONG
InterlockedDecrement (
    _Inout_ LONG volatile *Addend
    );

LONG
InterlockedExchange (
    _Inout_ LONG volatile *Target,
    _In_ LONG Value
    );

LONG
InterlockedExchangeAdd (
    _Inout_ LONG volatile *Addend,
    _In_ LONG Value
    );

LONG
InterlockedCompareExchange (
    _Inout_ LONG volatile *Destination,
    _In_ LONG ExChange,
    _In_ LONG Comperand
    );

_Ret_writes_(_Inexpressible_(Unknown)) PVOID
InterlockedCompareExchangePointer (
    _Inout_ _At_(*Destination,
        _Pre_writable_byte_size_(_Inexpressible_(Unknown))
        _Post_writable_byte_size_(_Inexpressible_(Unknown)))
             PVOID volatile *Destination,
    _In_opt_ PVOID Exchange,
    _In_opt_ PVOID Comperand
    );

_Ret_writes_(_Inexpressible_(Unknown)) PVOID
InterlockedExchangePointer (
    _Inout_ _At_(*Target,
        _Pre_writable_byte_size_(_Inexpressible_(Unknown))
        _Post_writable_byte_size_(_Inexpressible_(Unknown)))
             PVOID volatile *Target,
    _In_opt_ PVOID Value
    );

LONG64
InterlockedAnd64 (
    _Inout_ LONG64 volatile *Destination,
    _In_ LONG64 Value
    );

LONG64
InterlockedOr64 (
    _Inout_ LONG64 volatile *Destination,
    _In_ LONG64 Value
    );

LONG64
InterlockedXor64 (
    _Inout_ LONG64 volatile *Destination,
    _In_ LONG64 Value
    );

LONG64
InterlockedIncrement64 (
    _Inout_ LONG64 volatile *Addend
    );

LONG64
InterlockedDecrement64 (
    _Inout_ LONG64 volatile *Addend
    );

LONG64
InterlockedExchange64 (
    _Inout_ LONG64 volatile *Target,
    _In_ LONG64 Value
    );

LONG64
InterlockedExchangeAdd64 (
    _Inout_ LONG64 volatile *Addend,
    _In_ LONG64 Value
    );

LONG64
InterlockedCompareExchange64 (
    _Inout_ LONG64 volatile *Destination,
    _In_ LONG64 ExChange,
    _In_ LONG64 Comperand
    );

#pragma intrinsic(_InterlockedAnd)
#pragma intrinsic(_InterlockedOr)
#pragma intrinsic(_InterlockedXor)
#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedDecrement)
#pragma intrinsic(_InterlockedExchange)
#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedCompareExchange)
#pragma intrinsic(_InterlockedExchangePointer)
#pragma intrinsic(_InterlockedCompareExchangePointer)
#pragma intrinsic(_InterlockedAnd64)
#pragma intrinsic(_InterlockedOr64)
#pragma intrinsic(_InterlockedXor64)
#pragma intrinsic(_InterlockedIncrement64)
#pragma intrinsic(_InterlockedDecrement64)
#pragma intrinsic(_InterlockedExchange64)
#pragma intrinsic(_InterlockedExchangeAdd64)
#pragma intrinsic(_InterlockedCompareExchange64)

#if _MSC_FULL_VER >= 140041204

#define InterlockedAnd8 _InterlockedAnd8
#define InterlockedOr8 _InterlockedOr8
#define InterlockedXor8 _InterlockedXor8
#define InterlockedAnd16 _InterlockedAnd16
#define InterlockedOr16 _InterlockedOr16
#define InterlockedXor16 _InterlockedXor16

char
InterlockedAnd8 (
    _Inout_ char volatile *Destination,
    _In_ char Value
    );

char
InterlockedOr8 (
    _Inout_ char volatile *Destination,
    _In_ char Value
    );

char
InterlockedXor8 (
    _Inout_ char volatile *Destination,
    _In_ char Value
    );

SHORT
InterlockedAnd16(
    _Inout_ SHORT volatile *Destination,
    _In_ SHORT Value
    );

SHORT
InterlockedOr16(
    _Inout_ SHORT volatile *Destination,
    _In_ SHORT Value
    );

SHORT
InterlockedXor16(
    _Inout_ SHORT volatile *Destination,
    _In_ SHORT Value
    );

#pragma intrinsic (_InterlockedAnd8)
#pragma intrinsic (_InterlockedOr8)
#pragma intrinsic (_InterlockedXor8)
#pragma intrinsic (_InterlockedAnd16)
#pragma intrinsic (_InterlockedOr16)
#pragma intrinsic (_InterlockedXor16)

#endif

#elif defined(_M_ARM) && !defined(RC_INVOKED)

#define InterlockedAnd _InterlockedAnd
#define InterlockedOr _InterlockedOr
#define InterlockedXor _InterlockedXor

#define InterlockedIncrement _InterlockedIncrement
#define InterlockedDecrement _InterlockedDecrement

#define InterlockedExchange _InterlockedExchange
#define InterlockedExchangeAdd _InterlockedExchangeAdd
#define InterlockedExchangePointer _InterlockedExchangePointer

#define InterlockedCompareExchange _InterlockedCompareExchange
#define InterlockedCompareExchangePointer _InterlockedCompareExchangePointer

#define InterlockedAnd64 _InterlockedAnd64
#define InterlockedOr64 _InterlockedOr64
#define InterlockedXor64 _InterlockedXor64
#define InterlockedIncrement64 _InterlockedIncrement64
#define InterlockedDecrement64 _InterlockedDecrement64
#define InterlockedExchange64 _InterlockedExchange64
#define InterlockedExchangeAdd64 _InterlockedExchangeAdd64
#define InterlockedCompareExchange64 _InterlockedCompareExchange64

LONG
InterlockedIncrement (
    _Inout_ LONG volatile *Addend
    );

LONG
InterlockedDecrement (
    _Inout_ LONG volatile *Addend
    );

LONG
InterlockedExchange (
    _Inout_ LONG volatile *Target,
    _In_ LONG Value
    );

LONG
InterlockedExchangeAdd (
    _Inout_ LONG volatile *Addend,
    _In_ LONG Value
    );

LONG
InterlockedCompareExchange (
    _Inout_ LONG volatile *Destination,
    _In_ LONG ExChange,
    _In_ LONG Comperand
    );

PVOID
InterlockedCompareExchangePointer (
    _Inout_  PVOID volatile *Destination,
    _In_opt_ PVOID Exchange,
    _In_opt_ PVOID Comperand
    );

PVOID
InterlockedExchangePointer (
    _Inout_  PVOID volatile *Target,
    _In_opt_ PVOID Value
    );

LONG64
InterlockedAnd64 (
    _Inout_ LONG64 volatile *Destination,
    _In_ LONG64 Value
    );

LONG64
InterlockedOr64 (
    _Inout_ LONG64 volatile *Destination,
    _In_ LONG64 Value
    );

LONG64
InterlockedXor64 (
    _Inout_ LONG64 volatile *Destination,
    _In_ LONG64 Value
    );

LONG64
InterlockedIncrement64 (
    _Inout_ LONG64 volatile *Addend
    );

LONG64
InterlockedDecrement64 (
    _Inout_ LONG64 volatile *Addend
    );

LONG64
InterlockedExchange64 (
    _Inout_ LONG64 volatile *Target,
    _In_ LONG64 Value
    );

LONG64
InterlockedExchangeAdd64(
    _Inout_ LONG64 volatile *Addend,
    _In_    LONG64 Value
    );

LONG64
InterlockedCompareExchange64 (
    _Inout_ LONG64 volatile *Destination,
    _In_ LONG64 ExChange,
    _In_ LONG64 Comperand
    );

#pragma intrinsic(_InterlockedAnd64)
#pragma intrinsic(_InterlockedOr64)
#pragma intrinsic(_InterlockedXor64)
#pragma intrinsic(_InterlockedIncrement64)
#pragma intrinsic(_InterlockedDecrement64)
#pragma intrinsic(_InterlockedExchange64)
#pragma intrinsic(_InterlockedExchangeAdd64)
#pragma intrinsic(_InterlockedCompareExchange64)

#elif defined(_M_ARM64) && !defined(RC_INVOKED)

#define InterlockedAnd _InterlockedAnd
#define InterlockedOr _InterlockedOr
#define InterlockedXor _InterlockedXor

#define InterlockedIncrement _InterlockedIncrement
#define InterlockedDecrement _InterlockedDecrement

#define InterlockedExchange _InterlockedExchange
#define InterlockedExchangeAdd _InterlockedExchangeAdd
#define InterlockedExchangePointer _InterlockedExchangePointer

#define InterlockedCompareExchange _InterlockedCompareExchange
#define InterlockedCompareExchangePointer _InterlockedCompareExchangePointer

#define InterlockedAnd64 _InterlockedAnd64
#define InterlockedOr64 _InterlockedOr64
#define InterlockedXor64 _InterlockedXor64
#define InterlockedIncrement64 _InterlockedIncrement64
#define InterlockedDecrement64 _InterlockedDecrement64
#define InterlockedExchange64 _InterlockedExchange64
#define InterlockedExchangeAdd64 _InterlockedExchangeAdd64
#define InterlockedCompareExchange64 _InterlockedCompareExchange64

LONG
InterlockedIncrement (
    _Inout_ LONG volatile *Addend
    );

LONG
InterlockedDecrement (
    _Inout_ LONG volatile *Addend
    );

LONG
InterlockedExchange (
    _Inout_ LONG volatile *Target,
    _In_ LONG Value
    );

LONG
InterlockedExchangeAdd (
    _Inout_ LONG volatile *Addend,
    _In_ LONG Value
    );

LONG
InterlockedCompareExchange (
    _Inout_ LONG volatile *Destination,
    _In_ LONG ExChange,
    _In_ LONG Comperand
    );

PVOID
InterlockedCompareExchangePointer (
    _Inout_  PVOID volatile *Destination,
    _In_opt_ PVOID Exchange,
    _In_opt_ PVOID Comperand
    );

PVOID
InterlockedExchangePointer (
    _Inout_  PVOID volatile *Target,
    _In_opt_ PVOID Value
    );

LONG64
InterlockedAnd64 (
    _Inout_ LONG64 volatile *Destination,
    _In_ LONG64 Value
    );

LONG64
InterlockedOr64 (
    _Inout_ LONG64 volatile *Destination,
    _In_ LONG64 Value
    );

LONG64
InterlockedXor64 (
    _Inout_ LONG64 volatile *Destination,
    _In_ LONG64 Value
    );

LONG64
InterlockedIncrement64 (
    _Inout_ LONG64 volatile *Addend
    );

LONG64
InterlockedDecrement64 (
    _Inout_ LONG64 volatile *Addend
    );

LONG64
InterlockedExchange64 (
    _Inout_ LONG64 volatile *Target,
    _In_ LONG64 Value
    );

LONG64
InterlockedExchangeAdd64(
    _Inout_ LONG64 volatile *Addend,
    _In_    LONG64 Value
    );

LONG64
InterlockedCompareExchange64 (
    _Inout_ LONG64 volatile *Destination,
    _In_ LONG64 ExChange,
    _In_ LONG64 Comperand
    );

#pragma intrinsic(_InterlockedAnd64)
#pragma intrinsic(_InterlockedOr64)
#pragma intrinsic(_InterlockedXor64)
#pragma intrinsic(_InterlockedIncrement64)
#pragma intrinsic(_InterlockedDecrement64)
#pragma intrinsic(_InterlockedExchange64)
#pragma intrinsic(_InterlockedExchangeAdd64)
#pragma intrinsic(_InterlockedCompareExchange64)

#else           // X86 interlocked definitions

#if !defined(MIDL_PASS)

#if (_WIN32_WINNT >= 0x0502)

FORCEINLINE
LONGLONG
InterlockedAnd64 (
    _Inout_ LONGLONG volatile *Destination,
    _In_    LONGLONG Value
    )
{
    LONGLONG Old;

    do {
        Old = *Destination;
    } while (InterlockedCompareExchange64(Destination,
                                          Old & Value,
                                          Old) != Old);

    return Old;
}

FORCEINLINE
LONGLONG
InterlockedOr64 (
    _Inout_ LONGLONG volatile *Destination,
    _In_    LONGLONG Value
    )
{
    LONGLONG Old;

    do {
        Old = *Destination;
    } while (InterlockedCompareExchange64(Destination,
                                          Old | Value,
                                          Old) != Old);

    return Old;
}

FORCEINLINE
LONGLONG
InterlockedXor64 (
    _Inout_ LONGLONG volatile *Destination,
    _In_    LONGLONG Value
    )
{
    LONGLONG Old;

    do {
        Old = *Destination;
    } while (InterlockedCompareExchange64(Destination,
                                          Old ^ Value,
                                          Old) != Old);

    return Old;
}

FORCEINLINE
LONGLONG
InterlockedIncrement64 (
    _Inout_ LONGLONG volatile *Addend
    )
{
    LONGLONG Old;

    do {
        Old = *Addend;
    } while (InterlockedCompareExchange64(Addend,
                                          Old + 1,
                                          Old) != Old);

    return Old + 1;
}

FORCEINLINE
LONGLONG
InterlockedDecrement64 (
    _Inout_ LONGLONG volatile *Addend
    )
{
    LONGLONG Old;

    do {
        Old = *Addend;
    } while (InterlockedCompareExchange64(Addend,
                                          Old - 1,
                                          Old) != Old);

    return Old - 1;
}

FORCEINLINE
LONGLONG
InterlockedExchange64 (
    _Inout_ LONGLONG volatile *Target,
    _In_    LONGLONG Value
    )
{
    LONGLONG Old;

    do {
        Old = *Target;
    } while (InterlockedCompareExchange64(Target,
                                          Value,
                                          Old) != Old);

    return Old;
}

FORCEINLINE
LONGLONG
InterlockedExchangeAdd64(
    _Inout_ LONGLONG volatile *Addend,
    _In_    LONGLONG Value
    )
{
    LONGLONG Old;

    do {
        Old = *Addend;
    } while (InterlockedCompareExchange64(Addend,
                                          Old + Value,
                                          Old) != Old);

    return Old;
}

#endif


#endif

//
// Use a function for C++ so X86 will generate the same errors as RISC.
//

#ifdef __cplusplus

FORCEINLINE
_Ret_writes_(_Inexpressible_(Unknown)) PVOID
#if !defined(_M_CEE_PURE)
__cdecl
#endif
__InlineInterlockedCompareExchangePointer (
    _Inout_ _At_(*Destination,
        _Pre_writable_byte_size_(_Inexpressible_(Unknown))
        _Post_writable_byte_size_(_Inexpressible_(Unknown)))
             PVOID volatile *Destination,
    _In_opt_ PVOID ExChange,
    _In_opt_ PVOID Comperand
    )
{
    return((PVOID)(LONG_PTR)InterlockedCompareExchange((LONG volatile *)Destination, (LONG)(LONG_PTR)ExChange, (LONG)(LONG_PTR)Comperand));
}

#define InterlockedCompareExchangePointer __InlineInterlockedCompareExchangePointer

#else

#define InterlockedCompareExchangePointer(Destination, ExChange, Comperand) \
    (PVOID)(LONG_PTR)InterlockedCompareExchange((LONG volatile *)(Destination), (LONG)(LONG_PTR)(ExChange), (LONG)(LONG_PTR)(Comperand))

#endif /* __cplusplus */

#define InterlockedIncrementAcquire InterlockedIncrement
#define InterlockedIncrementRelease InterlockedIncrement
#define InterlockedDecrementAcquire InterlockedDecrement
#define InterlockedDecrementRelease InterlockedDecrement
#define InterlockedIncrementAcquire InterlockedIncrement
#define InterlockedIncrementRelease InterlockedIncrement
#define InterlockedCompareExchangeAcquire InterlockedCompareExchange
#define InterlockedCompareExchangeRelease InterlockedCompareExchange
#define InterlockedCompareExchangeAcquire64 InterlockedCompareExchange64
#define InterlockedCompareExchangeRelease64 InterlockedCompareExchange64
#define InterlockedCompareExchangePointerAcquire InterlockedCompareExchangePointer
#define InterlockedCompareExchangePointerRelease InterlockedCompareExchangePointer

#endif /* X86 | IA64 */

#endif /* _NTOS_ */

#endif /* NOWINBASEINTERLOCK */

#define UnlockResource(hResData) ((hResData), 0)
#define MAXINTATOM 0xC000
#define MAKEINTATOM(i)  (LPTSTR)((ULONG_PTR)((WORD)(i)))
#define INVALID_ATOM ((ATOM)0)

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

int
#if !defined(_MAC)
#if defined(_M_CEE_PURE)
__clrcall
#else
WINAPI
#endif
#else
CALLBACK
#endif
WinMain (
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd
    );

int
#if defined(_M_CEE_PURE)
__clrcall
#else
WINAPI
#endif
wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd
    );

WINBASEAPI
_Success_(return != NULL)
_Post_writable_byte_size_(dwBytes)
HGLOBAL
WINAPI
GlobalAlloc(
    _In_ UINT uFlags,
    _In_ SIZE_T dwBytes
    );

WINBASEAPI
_Success_(return != NULL)
_Post_writable_byte_size_(dwBytes)
HGLOBAL
WINAPI
GlobalReAlloc (
    _In_ HGLOBAL hMem,
    _In_ SIZE_T dwBytes,
    _In_ UINT uFlags
    );

WINBASEAPI
SIZE_T
WINAPI
GlobalSize (
    _In_ HGLOBAL hMem
    );

WINBASEAPI
UINT
WINAPI
GlobalFlags (
    _In_ HGLOBAL hMem
    );

WINBASEAPI
_Ret_maybenull_
LPVOID
WINAPI
GlobalLock (
    _In_ HGLOBAL hMem
    );

WINBASEAPI
_Ret_maybenull_
HGLOBAL
WINAPI
GlobalHandle (
    _In_ LPCVOID pMem
    );

WINBASEAPI
BOOL
WINAPI
GlobalUnlock(
    _In_ HGLOBAL hMem
    );

WINBASEAPI
_Ret_maybenull_
HGLOBAL
WINAPI
GlobalFree(
    _Pre_opt_valid_ HGLOBAL hMem
    );

WINBASEAPI
SIZE_T
WINAPI
GlobalCompact(
    _In_ DWORD dwMinFree
    );

WINBASEAPI
VOID
WINAPI
GlobalFix(
    _In_ HGLOBAL hMem
    );

WINBASEAPI
VOID
WINAPI
GlobalUnfix(
    _In_ HGLOBAL hMem
    );

WINBASEAPI
LPVOID
WINAPI
GlobalWire(
    _In_ HGLOBAL hMem
    );

WINBASEAPI
BOOL
WINAPI
GlobalUnWire(
    _In_ HGLOBAL hMem
    );

__drv_preferredFunction("GlobalMemoryStatusEx","Deprecated. See MSDN for details")
WINBASEAPI
VOID
WINAPI
GlobalMemoryStatus(
    _Out_ LPMEMORYSTATUS lpBuffer
    );

WINBASEAPI
_Success_(return != NULL)
_Post_writable_byte_size_(uBytes)
HLOCAL
WINAPI
LocalAlloc(
    _In_ UINT uFlags,
    _In_ SIZE_T uBytes
    );

WINBASEAPI
_Success_(return != NULL)
_Post_writable_byte_size_(uBytes)
HLOCAL
WINAPI
LocalReAlloc(
    _In_ HLOCAL hMem,
    _In_ SIZE_T uBytes,
    _In_ UINT uFlags
    );

WINBASEAPI
_Ret_maybenull_
LPVOID
WINAPI
LocalLock(
    _In_ HLOCAL hMem
    );

WINBASEAPI
_Ret_maybenull_
HLOCAL
WINAPI
LocalHandle(
    _In_ LPCVOID pMem
    );

WINBASEAPI
BOOL
WINAPI
LocalUnlock(
    _In_ HLOCAL hMem
    );

WINBASEAPI
SIZE_T
WINAPI
LocalSize(
    _In_ HLOCAL hMem
    );

WINBASEAPI
UINT
WINAPI
LocalFlags(
    _In_ HLOCAL hMem
    );

WINBASEAPI
HLOCAL
WINAPI
LocalFree(
    _Pre_opt_valid_ HLOCAL hMem
    );

WINBASEAPI
SIZE_T
WINAPI
LocalShrink(
    _In_ HLOCAL hMem,
    _In_ UINT cbNewSize
    );

WINBASEAPI
SIZE_T
WINAPI
LocalCompact(
    _In_ UINT uMinFree
    );

#if (_WIN32_WINNT >= 0x0601)

WINBASEAPI
BOOL
WINAPI
GetProcessorSystemCycleTime (
    _In_ USHORT Group,
    _Out_writes_bytes_to_opt_(*ReturnedLength, *ReturnedLength) PSYSTEM_PROCESSOR_CYCLE_TIME_INFORMATION Buffer,
    _Inout_ PDWORD ReturnedLength
    );

#endif // (_WIN32_WINNT >= 0x0601)

#if (_WIN32_WINNT >= 0x0600)

WINBASEAPI
_Post_writable_byte_size_(dwSize)
LPVOID
WINAPI
VirtualAllocExNuma(
    _In_     HANDLE hProcess,
    _In_opt_ LPVOID lpAddress,
    _In_     SIZE_T dwSize,
    _In_     DWORD  flAllocationType,
    _In_     DWORD  flProtect,
    _In_     DWORD  nndPreferred
    );

#endif // (_WIN32_WINNT >= 0x0600)

#if (_WIN32_WINNT >= 0x0601)

WINBASEAPI
BOOL
WINAPI
GetPhysicallyInstalledSystemMemory(
    _Out_ PULONGLONG TotalMemoryInKilobytes
    );

#endif // (_WIN32_WINNT >= 0x0601)

// GetBinaryType return values.

#define SCS_32BIT_BINARY    0
#define SCS_DOS_BINARY      1
#define SCS_WOW_BINARY      2
#define SCS_PIF_BINARY      3
#define SCS_POSIX_BINARY    4
#define SCS_OS216_BINARY    5
#define SCS_64BIT_BINARY    6

#if defined(_WIN64)
# define SCS_THIS_PLATFORM_BINARY SCS_64BIT_BINARY
#else
# define SCS_THIS_PLATFORM_BINARY SCS_32BIT_BINARY
#endif

WINBASEAPI
BOOL
WINAPI
GetBinaryTypeA(
    _In_  LPCSTR lpApplicationName,
    _Out_ LPDWORD  lpBinaryType
    );
WINBASEAPI
BOOL
WINAPI
GetBinaryTypeW(
    _In_  LPCWSTR lpApplicationName,
    _Out_ LPDWORD  lpBinaryType
    );
#ifdef UNICODE
#define GetBinaryType  GetBinaryTypeW
#else
#define GetBinaryType  GetBinaryTypeA
#endif // !UNICODE

WINBASEAPI
_Success_(return != 0 && return < cchBuffer)
DWORD
WINAPI
GetShortPathNameA(
    _In_ LPCSTR lpszLongPath,
    _Out_writes_to_opt_(cchBuffer, return + 1) LPSTR  lpszShortPath,
    _In_ DWORD cchBuffer
    );
#ifndef UNICODE
#define GetShortPathName  GetShortPathNameA
#endif

#if _WIN32_WINNT >= 0x0600

WINBASEAPI
_Success_(return != 0 && return < cchBuffer)
DWORD
WINAPI
GetLongPathNameTransactedA(
    _In_     LPCSTR lpszShortPath,
    _Out_writes_to_opt_(cchBuffer, return + 1) LPSTR  lpszLongPath,
    _In_     DWORD cchBuffer,
    _In_     HANDLE hTransaction
    );
WINBASEAPI
_Success_(return != 0 && return < cchBuffer)
DWORD
WINAPI
GetLongPathNameTransactedW(
    _In_     LPCWSTR lpszShortPath,
    _Out_writes_to_opt_(cchBuffer, return + 1) LPWSTR  lpszLongPath,
    _In_     DWORD cchBuffer,
    _In_     HANDLE hTransaction
    );
#ifdef UNICODE
#define GetLongPathNameTransacted  GetLongPathNameTransactedW
#else
#define GetLongPathNameTransacted  GetLongPathNameTransactedA
#endif // !UNICODE

#endif // _WIN32_WINNT >= 0x0600

WINBASEAPI
BOOL
WINAPI
GetProcessAffinityMask(
    _In_  HANDLE hProcess,
    _Out_ PDWORD_PTR lpProcessAffinityMask,
    _Out_ PDWORD_PTR lpSystemAffinityMask
    );

WINBASEAPI
BOOL
WINAPI
SetProcessAffinityMask(
    _In_ HANDLE hProcess,
    _In_ DWORD_PTR dwProcessAffinityMask
    );

WINBASEAPI
BOOL
WINAPI
GetProcessIoCounters(
    _In_  HANDLE hProcess,
    _Out_ PIO_COUNTERS lpIoCounters
    );

WINBASEAPI
BOOL
WINAPI
GetProcessWorkingSetSize(
    _In_  HANDLE hProcess,
    _Out_ PSIZE_T lpMinimumWorkingSetSize,
    _Out_ PSIZE_T lpMaximumWorkingSetSize
    );

WINBASEAPI
BOOL
WINAPI
SetProcessWorkingSetSize(
    _In_ HANDLE hProcess,
    _In_ SIZE_T dwMinimumWorkingSetSize,
    _In_ SIZE_T dwMaximumWorkingSetSize
    );

WINBASEAPI
__analysis_noreturn
VOID
WINAPI
FatalExit(
    _In_ int ExitCode
    );

WINBASEAPI
BOOL
WINAPI
SetEnvironmentStringsA(
    _In_ _Pre_ _NullNull_terminated_ LPCH NewEnvironment
    );
#ifndef UNICODE
#define SetEnvironmentStrings  SetEnvironmentStringsA
#endif

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#pragma region Application Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)

WINBASEAPI
VOID
WINAPI
RaiseFailFastException(
    _In_opt_ PEXCEPTION_RECORD pExceptionRecord,
    _In_opt_ PCONTEXT pContextRecord,
    _In_ DWORD dwFlags
    );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

#if(_WIN32_WINNT >= 0x0400)

//
// Fiber creation flags
//

#define FIBER_FLAG_FLOAT_SWITCH 0x1     // context switch floating point

WINBASEAPI
_Ret_maybenull_
LPVOID
WINAPI
CreateFiber(
    _In_     SIZE_T dwStackSize,
    _In_     LPFIBER_START_ROUTINE lpStartAddress,
    _In_opt_ LPVOID lpParameter
    );

WINBASEAPI
_Ret_maybenull_
LPVOID
WINAPI
CreateFiberEx(
    _In_     SIZE_T dwStackCommitSize,
    _In_     SIZE_T dwStackReserveSize,
    _In_     DWORD dwFlags,
    _In_     LPFIBER_START_ROUTINE lpStartAddress,
    _In_opt_ LPVOID lpParameter
    );

WINBASEAPI
VOID
WINAPI
DeleteFiber(
    _In_ LPVOID lpFiber
    );

WINBASEAPI
_Ret_maybenull_
LPVOID
WINAPI
ConvertThreadToFiber(
    _In_opt_ LPVOID lpParameter
    );

WINBASEAPI
_Ret_maybenull_
LPVOID
WINAPI
ConvertThreadToFiberEx(
    _In_opt_ LPVOID lpParameter,
    _In_     DWORD dwFlags
    );

#if (_WIN32_WINNT >= 0x0501)

WINBASEAPI
BOOL
WINAPI
ConvertFiberToThread(
    VOID
    );

#endif

WINBASEAPI
VOID
WINAPI
SwitchToFiber(
    _In_ LPVOID lpFiber
    );

//
// UMS begin
//

#if (_WIN32_WINNT >= 0x0601) && !defined(MIDL_PASS)

#define UMS_VERSION RTL_UMS_VERSION

typedef void *PUMS_CONTEXT;

typedef void *PUMS_COMPLETION_LIST;

typedef enum _RTL_UMS_THREAD_INFO_CLASS UMS_THREAD_INFO_CLASS, *PUMS_THREAD_INFO_CLASS;

typedef enum _RTL_UMS_SCHEDULER_REASON UMS_SCHEDULER_REASON;

typedef PRTL_UMS_SCHEDULER_ENTRY_POINT PUMS_SCHEDULER_ENTRY_POINT;

typedef struct _UMS_SCHEDULER_STARTUP_INFO {

    //
    // UMS Version the application was built to. Should be set to UMS_VERSION
    //
    ULONG UmsVersion;

    //
    // Completion List to associate the new User Scheduler to.
    //
    PUMS_COMPLETION_LIST CompletionList;

    //
    // A pointer to the application-defined function that represents the starting
    // address of the Sheduler.
    //
    PUMS_SCHEDULER_ENTRY_POINT SchedulerProc;

    //
    // pointer to a variable to be passed to the scheduler uppon first activation.
    //
    PVOID SchedulerParam;

} UMS_SCHEDULER_STARTUP_INFO, *PUMS_SCHEDULER_STARTUP_INFO;

typedef struct _UMS_SYSTEM_THREAD_INFORMATION {
    ULONG UmsVersion;
    union {
        struct {
            ULONG IsUmsSchedulerThread : 1;
            ULONG IsUmsWorkerThread : 1;
        } DUMMYSTRUCTNAME;
        ULONG ThreadUmsFlags;
    } DUMMYUNIONNAME;
} UMS_SYSTEM_THREAD_INFORMATION, *PUMS_SYSTEM_THREAD_INFORMATION;

_Must_inspect_result_
WINBASEAPI
BOOL
WINAPI
CreateUmsCompletionList(
    _Outptr_ PUMS_COMPLETION_LIST* UmsCompletionList
    );

WINBASEAPI
BOOL
WINAPI
DequeueUmsCompletionListItems(
    _In_ PUMS_COMPLETION_LIST UmsCompletionList,
    _In_ DWORD WaitTimeOut,
    _Out_ PUMS_CONTEXT* UmsThreadList
    );

WINBASEAPI
BOOL
WINAPI
GetUmsCompletionListEvent(
    _In_ PUMS_COMPLETION_LIST UmsCompletionList,
    _Inout_ PHANDLE UmsCompletionEvent
    );

WINBASEAPI
BOOL
WINAPI
ExecuteUmsThread(
    _Inout_ PUMS_CONTEXT UmsThread
    );

WINBASEAPI
BOOL
WINAPI
UmsThreadYield(
    _In_ PVOID SchedulerParam
    );

WINBASEAPI
BOOL
WINAPI
DeleteUmsCompletionList(
    _In_ PUMS_COMPLETION_LIST UmsCompletionList
    );

WINBASEAPI
PUMS_CONTEXT
WINAPI
GetCurrentUmsThread(
    VOID
    );

WINBASEAPI
PUMS_CONTEXT
WINAPI
GetNextUmsListItem(
    _Inout_ PUMS_CONTEXT UmsContext
    );

WINBASEAPI
BOOL
WINAPI
QueryUmsThreadInformation(
    _In_ PUMS_CONTEXT UmsThread,
    _In_ UMS_THREAD_INFO_CLASS UmsThreadInfoClass,
    _Out_writes_bytes_to_(UmsThreadInformationLength, *ReturnLength) PVOID UmsThreadInformation,
    _In_ ULONG UmsThreadInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

WINBASEAPI
BOOL
WINAPI
SetUmsThreadInformation(
    _In_ PUMS_CONTEXT UmsThread,
    _In_ UMS_THREAD_INFO_CLASS UmsThreadInfoClass,
    _In_ PVOID UmsThreadInformation,
    _In_ ULONG UmsThreadInformationLength
    );

WINBASEAPI
BOOL
WINAPI
DeleteUmsThreadContext(
    _In_ PUMS_CONTEXT UmsThread
    );

WINBASEAPI
BOOL
WINAPI
CreateUmsThreadContext(
    _Outptr_ PUMS_CONTEXT *lpUmsThread
    );

WINBASEAPI
BOOL
WINAPI
EnterUmsSchedulingMode(
    _In_ PUMS_SCHEDULER_STARTUP_INFO SchedulerStartupInfo
    );

WINBASEAPI
BOOL
WINAPI
GetUmsSystemThreadInformation(
    _In_ HANDLE ThreadHandle,
    _Inout_ PUMS_SYSTEM_THREAD_INFORMATION SystemThreadInfo
    );

#endif // (_WIN32_WINNT >= 0x0601) && !defined(MIDL_PASS)

//
// UMS end
//

#endif /* _WIN32_WINNT >= 0x0400 */

WINBASEAPI
DWORD_PTR
WINAPI
SetThreadAffinityMask(
    _In_ HANDLE hThread,
    _In_ DWORD_PTR dwThreadAffinityMask
    );

#if(_WIN32_WINNT >= 0x0400)
WINBASEAPI
DWORD
WINAPI
SetThreadIdealProcessor(
    _In_ HANDLE hThread,
    _In_ DWORD dwIdealProcessor
    );
#endif /* _WIN32_WINNT >= 0x0400 */

//
// Thread information classes.
//

typedef enum _THREAD_INFORMATION_CLASS {
    ThreadMemoryPriority,
    ThreadAbsoluteCpuPriority,
    ThreadInformationClassMax
} THREAD_INFORMATION_CLASS;

//
// Process information classes.
//

typedef enum _PROCESS_INFORMATION_CLASS {
    ProcessMemoryPriority,
    ProcessInformationClassMax
} PROCESS_INFORMATION_CLASS;

#if (_WIN32_WINNT >= 0x0602)

WINBASEAPI
BOOL
WINAPI
GetThreadInformation (
    _In_ HANDLE hThread,
    _In_ THREAD_INFORMATION_CLASS ThreadInformationClass,
    _Out_writes_bytes_(ThreadInformationSize) LPVOID ThreadInformation,
    _In_ DWORD ThreadInformationSize
    );

WINBASEAPI
BOOL
WINAPI
SetThreadInformation (
    _In_ HANDLE hThread,
    _In_ THREAD_INFORMATION_CLASS ThreadInformationClass,
    _In_reads_bytes_(ThreadInformationSize) LPVOID ThreadInformation,
    _In_ DWORD ThreadInformationSize
    );

WINBASEAPI
BOOL
WINAPI
GetProcessInformation (
    _In_ HANDLE hProcess,
    _In_ PROCESS_INFORMATION_CLASS ProcessInformationClass,
    _Out_writes_bytes_(ProcessInformationSize) LPVOID ProcessInformation,
    _In_ DWORD ProcessInformationSize
    );

WINBASEAPI
BOOL
WINAPI
SetProcessInformation (
    _In_ HANDLE hProcess,
    _In_ PROCESS_INFORMATION_CLASS ProcessInformationClass,
    _In_reads_bytes_(ProcessInformationSize) LPVOID ProcessInformation,
    _In_ DWORD ProcessInformationSize
    );

//
// Define memory priorities.
//

#define MEMORY_PRIORITY_LOWEST           0
#define MEMORY_PRIORITY_VERY_LOW         1
#define MEMORY_PRIORITY_LOW              2
#define MEMORY_PRIORITY_MEDIUM           3
#define MEMORY_PRIORITY_BELOW_NORMAL     4
#define MEMORY_PRIORITY_NORMAL           5

typedef struct _MEMORY_PRIORITY_INFORMATION {
    ULONG MemoryPriority;
} MEMORY_PRIORITY_INFORMATION, *PMEMORY_PRIORITY_INFORMATION;

#endif // _WIN32_WINNT >= 0x0602

#if (_WIN32_WINNT >= 0x0600)

#define PROCESS_DEP_ENABLE                          0x00000001
#define PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION     0x00000002

WINBASEAPI
BOOL
WINAPI
SetProcessDEPPolicy(
    _In_ DWORD dwFlags
    );

WINBASEAPI
BOOL
WINAPI
GetProcessDEPPolicy(
    _In_ HANDLE hProcess,
    _Out_ LPDWORD lpFlags,
    _Out_ PBOOL lpPermanent
    );

#endif // _WIN32_WINNT >= 0x0600

WINBASEAPI
BOOL
WINAPI
SetProcessPriorityBoost(
    _In_ HANDLE hProcess,
    _In_ BOOL bDisablePriorityBoost
    );

WINBASEAPI
BOOL
WINAPI
GetProcessPriorityBoost(
    _In_  HANDLE hProcess,
    _Out_ PBOOL  pDisablePriorityBoost
    );

WINBASEAPI
BOOL
WINAPI
RequestWakeupLatency(
    _In_ LATENCY_TIME latency
    );

WINBASEAPI
BOOL
WINAPI
IsSystemResumeAutomatic(
    VOID
    );

#if _WIN32_WINNT >= 0x0501

WINBASEAPI
BOOL
WINAPI
GetThreadIOPendingFlag(
    _In_  HANDLE hThread,
    _Out_ PBOOL  lpIOIsPending
    );

#endif // (_WIN32_WINNT >= 0x0501)

WINBASEAPI
BOOL
WINAPI
GetThreadSelectorEntry(
    _In_  HANDLE hThread,
    _In_  DWORD dwSelector,
    _Out_ LPLDT_ENTRY lpSelectorEntry
    );

WINBASEAPI
EXECUTION_STATE
WINAPI
SetThreadExecutionState(
    _In_ EXECUTION_STATE esFlags
    );

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)

//
// Power Request APIs
//

typedef REASON_CONTEXT POWER_REQUEST_CONTEXT, *PPOWER_REQUEST_CONTEXT, *LPPOWER_REQUEST_CONTEXT;

WINBASEAPI
HANDLE
WINAPI
PowerCreateRequest (
    _In_ PREASON_CONTEXT Context
    );

WINBASEAPI
BOOL
WINAPI
PowerSetRequest (
    _In_ HANDLE PowerRequest,
    _In_ POWER_REQUEST_TYPE RequestType
    );

WINBASEAPI
BOOL
WINAPI
PowerClearRequest (
    _In_ HANDLE PowerRequest,
    _In_ POWER_REQUEST_TYPE RequestType
    );

#endif // (_WIN32_WINNT >= _WIN32_WINNT_WIN7)

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#pragma region Application Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)

#ifdef _M_CEE_PURE
#define GetLastError System::Runtime::InteropServices::Marshal::GetLastWin32Error
#endif

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

#if !defined(RC_INVOKED) // RC warns because "WINBASE_DECLARE_RESTORE_LAST_ERROR" is a bit long.
//#if _WIN32_WINNT >= 0x0501 || defined(WINBASE_DECLARE_RESTORE_LAST_ERROR)
#if defined(WINBASE_DECLARE_RESTORE_LAST_ERROR)

WINBASEAPI
VOID
WINAPI
RestoreLastError(
    _In_ DWORD dwErrCode
    );

typedef VOID (WINAPI* PRESTORE_LAST_ERROR)(DWORD);
#define RESTORE_LAST_ERROR_NAME_A      "RestoreLastError"
#define RESTORE_LAST_ERROR_NAME_W     L"RestoreLastError"
#define RESTORE_LAST_ERROR_NAME   TEXT("RestoreLastError")

#endif
#endif

#define HasOverlappedIoCompleted(lpOverlapped) (((DWORD)(lpOverlapped)->Internal) != STATUS_PENDING)

#if (_WIN32_WINNT >= 0x0600)

//
// The following flags allows an application to change
// the semantics of IO completion notification.
//

//
// Don't queue an entry to an associated completion port if returning success
// synchronously.
//
#define FILE_SKIP_COMPLETION_PORT_ON_SUCCESS    0x1

//
// Don't set the file handle event on IO completion.
//
#define FILE_SKIP_SET_EVENT_ON_HANDLE           0x2

WINBASEAPI
BOOL
WINAPI
SetFileCompletionNotificationModes(
    _In_ HANDLE FileHandle,
    _In_ UCHAR Flags
    );

WINBASEAPI
BOOL
WINAPI
SetFileIoOverlappedRange(
    _In_ HANDLE FileHandle,
    _In_ PUCHAR OverlappedRangeStart,
    _In_ ULONG Length
    );

#endif // _WIN32_WINNT >= 0x0600

#define SEM_FAILCRITICALERRORS      0x0001
#define SEM_NOGPFAULTERRORBOX       0x0002
#define SEM_NOALIGNMENTFAULTEXCEPT  0x0004
#define SEM_NOOPENFILEERRORBOX      0x8000

//
// Thread error mode support
//

WINBASEAPI
DWORD
WINAPI
GetThreadErrorMode(
    VOID
    );

WINBASEAPI
BOOL
WINAPI
SetThreadErrorMode(
    _In_ DWORD dwNewMode,
    _In_opt_ LPDWORD lpOldMode
    );

#if !defined(MIDL_PASS)

#if (_WIN32_WINNT >= 0x0600)

WINBASEAPI
BOOL
WINAPI
Wow64GetThreadContext(
    _In_    HANDLE hThread,
    _Inout_ PWOW64_CONTEXT lpContext
    );

WINBASEAPI
BOOL
WINAPI
Wow64SetThreadContext(
    _In_ HANDLE hThread,
    _In_ CONST WOW64_CONTEXT *lpContext
    );

#endif // (_WIN32_WINNT >= 0x0600)

#if (_WIN32_WINNT >= 0x0601)

WINBASEAPI
BOOL
WINAPI
Wow64GetThreadSelectorEntry(
    _In_ HANDLE hThread,
    _In_ DWORD dwSelector,
    _Out_ PWOW64_LDT_ENTRY lpSelectorEntry
    );

#endif // (_WIN32_WINNT >= 0x0601)

#endif // !defined(MIDL_PASS)

#if (_WIN32_WINNT >= 0x0600)

WINBASEAPI
DWORD
WINAPI
Wow64SuspendThread(
    _In_ HANDLE hThread
    );

#endif // (_WIN32_WINNT >= 0x0600)

WINBASEAPI
BOOL
WINAPI
DebugSetProcessKillOnExit(
    _In_ BOOL KillOnExit
    );

WINBASEAPI
BOOL
WINAPI
DebugBreakProcess (
    _In_ HANDLE Process
    );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#pragma region Application Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)

#if (_WIN32_WINNT >= 0x0403)
#define CRITICAL_SECTION_NO_DEBUG_INFO  RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO
#endif

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

WINBASEAPI
BOOL
WINAPI
PulseEvent(
    _In_ HANDLE hEvent
    );

WINBASEAPI
DWORD
WINAPI
WaitForMultipleObjects(
    _In_ DWORD nCount,
    _In_reads_(nCount) CONST HANDLE *lpHandles,
    _In_ BOOL bWaitAll,
    _In_ DWORD dwMilliseconds
    );

WINBASEAPI
ATOM
WINAPI
GlobalDeleteAtom(
    _In_ ATOM nAtom
    );

WINBASEAPI
BOOL
WINAPI
InitAtomTable(
    _In_ DWORD nSize
    );

WINBASEAPI
ATOM
WINAPI
DeleteAtom(
    _In_ ATOM nAtom
    );

WINBASEAPI
UINT
WINAPI
SetHandleCount(
    _In_ UINT uNumber
    );

WINBASEAPI
BOOL
WINAPI
RequestDeviceWakeup(
    _In_ HANDLE hDevice
    );

WINBASEAPI
BOOL
WINAPI
CancelDeviceWakeupRequest(
    _In_ HANDLE hDevice
    );

WINBASEAPI
BOOL
WINAPI
GetDevicePowerState(
    _In_  HANDLE hDevice,
    _Out_ BOOL *pfOn
    );

WINBASEAPI
BOOL
WINAPI
SetMessageWaitingIndicator(
    _In_ HANDLE hMsgIndicator,
    _In_ ULONG ulMsgCount
    );


WINBASEAPI
BOOL
WINAPI
SetFileShortNameA(
    _In_ HANDLE hFile,
    _In_ LPCSTR lpShortName
    );
WINBASEAPI
BOOL
WINAPI
SetFileShortNameW(
    _In_ HANDLE hFile,
    _In_ LPCWSTR lpShortName
    );
#ifdef UNICODE
#define SetFileShortName  SetFileShortNameW
#else
#define SetFileShortName  SetFileShortNameA
#endif // !UNICODE

#define HANDLE_FLAG_INHERIT             0x00000001
#define HANDLE_FLAG_PROTECT_FROM_CLOSE  0x00000002

#define HINSTANCE_ERROR 32

WINBASEAPI
DWORD
WINAPI
LoadModule(
    _In_ LPCSTR lpModuleName,
    _In_ LPVOID lpParameterBlock
    );


__drv_preferredFunction("CreateProcess","Deprecated. See MSDN for details")
WINBASEAPI
UINT
WINAPI
WinExec(
    _In_ LPCSTR lpCmdLine,
    _In_ UINT uCmdShow
    );

WINBASEAPI
BOOL
WINAPI
ClearCommBreak(
    _In_ HANDLE hFile
    );

WINBASEAPI
BOOL
WINAPI
ClearCommError(
    _In_      HANDLE hFile,
    _Out_opt_ LPDWORD lpErrors,
    _Out_opt_ LPCOMSTAT lpStat
    );

WINBASEAPI
BOOL
WINAPI
SetupComm(
    _In_ HANDLE hFile,
    _In_ DWORD dwInQueue,
    _In_ DWORD dwOutQueue
    );

WINBASEAPI
BOOL
WINAPI
EscapeCommFunction(
    _In_ HANDLE hFile,
    _In_ DWORD dwFunc
    );

WINBASEAPI
_Success_(return != FALSE)
BOOL
WINAPI
GetCommConfig(
    _In_      HANDLE hCommDev,
    _Out_writes_bytes_opt_(*lpdwSize) LPCOMMCONFIG lpCC,
    _Inout_   LPDWORD lpdwSize
    );

WINBASEAPI
BOOL
WINAPI
GetCommMask(
    _In_  HANDLE hFile,
    _Out_ LPDWORD lpEvtMask
    );

WINBASEAPI
BOOL
WINAPI
GetCommProperties(
    _In_    HANDLE hFile,
    _Inout_ LPCOMMPROP lpCommProp
    );

WINBASEAPI
BOOL
WINAPI
GetCommModemStatus(
    _In_  HANDLE hFile,
    _Out_ LPDWORD lpModemStat
    );

WINBASEAPI
BOOL
WINAPI
GetCommState(
    _In_  HANDLE hFile,
    _Out_ LPDCB lpDCB
    );

WINBASEAPI
BOOL
WINAPI
GetCommTimeouts(
    _In_  HANDLE hFile,
    _Out_ LPCOMMTIMEOUTS lpCommTimeouts
    );

WINBASEAPI
BOOL
WINAPI
PurgeComm(
    _In_ HANDLE hFile,
    _In_ DWORD dwFlags
    );

WINBASEAPI
BOOL
WINAPI
SetCommBreak(
    _In_ HANDLE hFile
    );

WINBASEAPI
BOOL
WINAPI
SetCommConfig(
    _In_ HANDLE hCommDev,
    _In_reads_bytes_(dwSize) LPCOMMCONFIG lpCC,
    _In_ DWORD dwSize
    );

WINBASEAPI
BOOL
WINAPI
SetCommMask(
    _In_ HANDLE hFile,
    _In_ DWORD dwEvtMask
    );

WINBASEAPI
BOOL
WINAPI
SetCommState(
    _In_ HANDLE hFile,
    _In_ LPDCB lpDCB
    );

WINBASEAPI
BOOL
WINAPI
SetCommTimeouts(
    _In_ HANDLE hFile,
    _In_ LPCOMMTIMEOUTS lpCommTimeouts
    );

WINBASEAPI
BOOL
WINAPI
TransmitCommChar(
    _In_ HANDLE hFile,
    _In_ char cChar
    );

WINBASEAPI
BOOL
WINAPI
WaitCommEvent(
    _In_        HANDLE hFile,
    _Inout_     LPDWORD lpEvtMask,
    _Inout_opt_ LPOVERLAPPED lpOverlapped
    );


WINBASEAPI
DWORD
WINAPI
SetTapePosition(
    _In_ HANDLE hDevice,
    _In_ DWORD dwPositionMethod,
    _In_ DWORD dwPartition,
    _In_ DWORD dwOffsetLow,
    _In_ DWORD dwOffsetHigh,
    _In_ BOOL bImmediate
    );

WINBASEAPI
DWORD
WINAPI
GetTapePosition(
    _In_  HANDLE hDevice,
    _In_  DWORD dwPositionType,
    _Out_ LPDWORD lpdwPartition,
    _Out_ LPDWORD lpdwOffsetLow,
    _Out_ LPDWORD lpdwOffsetHigh
    );

WINBASEAPI
DWORD
WINAPI
PrepareTape(
    _In_ HANDLE hDevice,
    _In_ DWORD dwOperation,
    _In_ BOOL bImmediate
    );

WINBASEAPI
DWORD
WINAPI
EraseTape(
    _In_ HANDLE hDevice,
    _In_ DWORD dwEraseType,
    _In_ BOOL bImmediate
    );

WINBASEAPI
DWORD
WINAPI
CreateTapePartition(
    _In_ HANDLE hDevice,
    _In_ DWORD dwPartitionMethod,
    _In_ DWORD dwCount,
    _In_ DWORD dwSize
    );

WINBASEAPI
DWORD
WINAPI
WriteTapemark(
    _In_ HANDLE hDevice,
    _In_ DWORD dwTapemarkType,
    _In_ DWORD dwTapemarkCount,
    _In_ BOOL bImmediate
    );

WINBASEAPI
DWORD
WINAPI
GetTapeStatus(
    _In_ HANDLE hDevice
    );

WINBASEAPI
DWORD
WINAPI
GetTapeParameters(
    _In_    HANDLE hDevice,
    _In_    DWORD dwOperation,
    _Inout_ LPDWORD lpdwSize,
    _Out_writes_bytes_(*lpdwSize) LPVOID lpTapeInformation
    );

#define GET_TAPE_MEDIA_INFORMATION 0
#define GET_TAPE_DRIVE_INFORMATION 1

WINBASEAPI
DWORD
WINAPI
SetTapeParameters(
    _In_ HANDLE hDevice,
    _In_ DWORD dwOperation,
    _In_ LPVOID lpTapeInformation
    );

#define SET_TAPE_MEDIA_INFORMATION 0
#define SET_TAPE_DRIVE_INFORMATION 1

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#pragma region Application Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)

WINBASEAPI
int
WINAPI
MulDiv(
    _In_ int nNumber,
    _In_ int nNumerator,
    _In_ int nDenominator
    );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

typedef enum _DEP_SYSTEM_POLICY_TYPE {
    DEPPolicyAlwaysOff = 0,
    DEPPolicyAlwaysOn,
    DEPPolicyOptIn,
    DEPPolicyOptOut,
    DEPTotalPolicyCount
} DEP_SYSTEM_POLICY_TYPE;

#if (NTDDI_VERSION >= NTDDI_WINXPSP3)

WINBASEAPI
DEP_SYSTEM_POLICY_TYPE
WINAPI
GetSystemDEPPolicy(
    VOID
    );

#endif // (NTDDI_VERSION >= NTDDI_WINXPSP3)

#if _WIN32_WINNT >= 0x0501

WINBASEAPI
BOOL
WINAPI
GetSystemRegistryQuota(
    _Out_opt_ PDWORD pdwQuotaAllowed,
    _Out_opt_ PDWORD pdwQuotaUsed
    );

BOOL
WINAPI
GetSystemTimes(
    _Out_opt_ LPFILETIME lpIdleTime,
    _Out_opt_ LPFILETIME lpKernelTime,
    _Out_opt_ LPFILETIME lpUserTime
    );

#endif // (_WIN32_WINNT >= 0x0501)

//
// Routines to convert back and forth between system time and file time
//

WINBASEAPI
BOOL
WINAPI
FileTimeToDosDateTime(
    _In_  CONST FILETIME *lpFileTime,
    _Out_ LPWORD lpFatDate,
    _Out_ LPWORD lpFatTime
    );

WINBASEAPI
BOOL
WINAPI
DosDateTimeToFileTime(
    _In_  WORD wFatDate,
    _In_  WORD wFatTime,
    _Out_ LPFILETIME lpFileTime
    );

WINBASEAPI
BOOL
WINAPI
SetSystemTimeAdjustment(
    _In_ DWORD dwTimeAdjustment,
    _In_ BOOL  bTimeAdjustmentDisabled
    );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion


#pragma region Application Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)

#if !defined(MIDL_PASS)
WINBASEAPI
_Success_(return != 0)
DWORD
WINAPI
FormatMessageA(
    _In_     DWORD dwFlags,
    _In_opt_ LPCVOID lpSource,
    _In_     DWORD dwMessageId,
    _In_     DWORD dwLanguageId,
    _Out_    LPSTR lpBuffer,
    _In_     DWORD nSize,
    _In_opt_ va_list *Arguments
    );
WINBASEAPI
_Success_(return != 0)
DWORD
WINAPI
FormatMessageW(
    _In_     DWORD dwFlags,
    _In_opt_ LPCVOID lpSource,
    _In_     DWORD dwMessageId,
    _In_     DWORD dwLanguageId,
    _Out_    LPWSTR lpBuffer,
    _In_     DWORD nSize,
    _In_opt_ va_list *Arguments
    );
#ifdef UNICODE
#define FormatMessage  FormatMessageW
#else
#define FormatMessage  FormatMessageA
#endif // !UNICODE

#if defined(_M_CEE)
#undef FormatMessage
__inline
DWORD
FormatMessage(
    DWORD dwFlags,
    LPCVOID lpSource,
    DWORD dwMessageId,
    DWORD dwLanguageId,
    LPTSTR lpBuffer,
    DWORD nSize,
    va_list *Arguments
    )
{
#ifdef UNICODE
    return FormatMessageW(
#else
    return FormatMessageA(
#endif
        dwFlags,
        lpSource,
        dwMessageId,
        dwLanguageId,
        lpBuffer,
        nSize,
        Arguments
        );
}
#endif  /* _M_CEE */
#endif  /* MIDL_PASS */

#define FORMAT_MESSAGE_IGNORE_INSERTS  0x00000200
#define FORMAT_MESSAGE_FROM_STRING     0x00000400
#define FORMAT_MESSAGE_FROM_HMODULE    0x00000800
#define FORMAT_MESSAGE_FROM_SYSTEM     0x00001000
#define FORMAT_MESSAGE_ARGUMENT_ARRAY  0x00002000
#define FORMAT_MESSAGE_MAX_WIDTH_MASK  0x000000FF

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

//
// FORMAT_MESSAGE_ALLOCATE_BUFFER requires use of LocalFree which is in the
// "Desktop Family" only.
//

#define FORMAT_MESSAGE_ALLOCATE_BUFFER 0x00000100


WINBASEAPI
BOOL
WINAPI
GetNamedPipeInfo(
    _In_      HANDLE hNamedPipe,
    _Out_opt_ LPDWORD lpFlags,
    _Out_opt_ LPDWORD lpOutBufferSize,
    _Out_opt_ LPDWORD lpInBufferSize,
    _Out_opt_ LPDWORD lpMaxInstances
    );

WINBASEAPI
HANDLE
WINAPI
CreateMailslotA(
    _In_     LPCSTR lpName,
    _In_     DWORD nMaxMessageSize,
    _In_     DWORD lReadTimeout,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );
WINBASEAPI
HANDLE
WINAPI
CreateMailslotW(
    _In_     LPCWSTR lpName,
    _In_     DWORD nMaxMessageSize,
    _In_     DWORD lReadTimeout,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );
#ifdef UNICODE
#define CreateMailslot  CreateMailslotW
#else
#define CreateMailslot  CreateMailslotA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
GetMailslotInfo(
    _In_      HANDLE hMailslot,
    _Out_opt_ LPDWORD lpMaxMessageSize,
    _Out_opt_ LPDWORD lpNextSize,
    _Out_opt_ LPDWORD lpMessageCount,
    _Out_opt_ LPDWORD lpReadTimeout
    );

WINBASEAPI
BOOL
WINAPI
SetMailslotInfo(
    _In_ HANDLE hMailslot,
    _In_ DWORD lReadTimeout
    );

//
// File Encryption API
//

WINADVAPI
BOOL
WINAPI
EncryptFileA(
    _In_ LPCSTR lpFileName
    );
WINADVAPI
BOOL
WINAPI
EncryptFileW(
    _In_ LPCWSTR lpFileName
    );
#ifdef UNICODE
#define EncryptFile  EncryptFileW
#else
#define EncryptFile  EncryptFileA
#endif // !UNICODE

WINADVAPI
BOOL
WINAPI
DecryptFileA(
    _In_       LPCSTR lpFileName,
    _Reserved_ DWORD dwReserved
    );
WINADVAPI
BOOL
WINAPI
DecryptFileW(
    _In_       LPCWSTR lpFileName,
    _Reserved_ DWORD dwReserved
    );
#ifdef UNICODE
#define DecryptFile  DecryptFileW
#else
#define DecryptFile  DecryptFileA
#endif // !UNICODE

//
//  Encryption Status Value
//

#define FILE_ENCRYPTABLE                0
#define FILE_IS_ENCRYPTED               1
#define FILE_SYSTEM_ATTR                2
#define FILE_ROOT_DIR                   3
#define FILE_SYSTEM_DIR                 4
#define FILE_UNKNOWN                    5
#define FILE_SYSTEM_NOT_SUPPORT         6
#define FILE_USER_DISALLOWED            7
#define FILE_READ_ONLY                  8
#define FILE_DIR_DISALLOWED             9

WINADVAPI
BOOL
WINAPI
FileEncryptionStatusA(
    _In_  LPCSTR lpFileName,
    _Out_ LPDWORD  lpStatus
    );
WINADVAPI
BOOL
WINAPI
FileEncryptionStatusW(
    _In_  LPCWSTR lpFileName,
    _Out_ LPDWORD  lpStatus
    );
#ifdef UNICODE
#define FileEncryptionStatus  FileEncryptionStatusW
#else
#define FileEncryptionStatus  FileEncryptionStatusA
#endif // !UNICODE

//
// Currently defined recovery flags
//

#define EFS_USE_RECOVERY_KEYS  (0x1)

typedef
DWORD
(WINAPI *PFE_EXPORT_FUNC)(
    _In_reads_bytes_(ulLength) PBYTE pbData,
    _In_opt_ PVOID pvCallbackContext,
    _In_     ULONG ulLength
    );

typedef
DWORD
(WINAPI *PFE_IMPORT_FUNC)(
    _Out_writes_bytes_to_(*ulLength, *ulLength) PBYTE pbData,
    _In_opt_ PVOID pvCallbackContext,
    _Inout_  PULONG ulLength
    );


//
//  OpenRaw flag values
//

#define CREATE_FOR_IMPORT  (1)
#define CREATE_FOR_DIR     (2)
#define OVERWRITE_HIDDEN   (4)
#define EFSRPC_SECURE_ONLY (8)


WINADVAPI
DWORD
WINAPI
OpenEncryptedFileRawA(
    _In_        LPCSTR lpFileName,
    _In_        ULONG    ulFlags,
    _Outptr_ PVOID   *pvContext
    );
WINADVAPI
DWORD
WINAPI
OpenEncryptedFileRawW(
    _In_        LPCWSTR lpFileName,
    _In_        ULONG    ulFlags,
    _Outptr_ PVOID   *pvContext
    );
#ifdef UNICODE
#define OpenEncryptedFileRaw  OpenEncryptedFileRawW
#else
#define OpenEncryptedFileRaw  OpenEncryptedFileRawA
#endif // !UNICODE

WINADVAPI
DWORD
WINAPI
ReadEncryptedFileRaw(
    _In_     PFE_EXPORT_FUNC pfExportCallback,
    _In_opt_ PVOID           pvCallbackContext,
    _In_     PVOID           pvContext
    );

WINADVAPI
DWORD
WINAPI
WriteEncryptedFileRaw(
    _In_     PFE_IMPORT_FUNC pfImportCallback,
    _In_opt_ PVOID           pvCallbackContext,
    _In_     PVOID           pvContext
    );

WINADVAPI
VOID
WINAPI
CloseEncryptedFileRaw(
    _In_ PVOID           pvContext
    );

//
// _l Compat Functions
//

WINBASEAPI
int
WINAPI
lstrcmpA(
    _In_ LPCSTR lpString1,
    _In_ LPCSTR lpString2
    );
WINBASEAPI
int
WINAPI
lstrcmpW(
    _In_ LPCWSTR lpString1,
    _In_ LPCWSTR lpString2
    );
#ifdef UNICODE
#define lstrcmp  lstrcmpW
#else
#define lstrcmp  lstrcmpA
#endif // !UNICODE

WINBASEAPI
int
WINAPI
lstrcmpiA(
    _In_ LPCSTR lpString1,
    _In_ LPCSTR lpString2
    );
WINBASEAPI
int
WINAPI
lstrcmpiW(
    _In_ LPCWSTR lpString1,
    _In_ LPCWSTR lpString2
    );
#ifdef UNICODE
#define lstrcmpi  lstrcmpiW
#else
#define lstrcmpi  lstrcmpiA
#endif // !UNICODE

WINBASEAPI
_Out_
LPSTR
WINAPI
lstrcpynA(
    _Out_writes_(iMaxLength) LPSTR lpString1,
    _In_ LPCSTR lpString2,
    _In_ int iMaxLength
    );
WINBASEAPI
_Out_
LPWSTR
WINAPI
lstrcpynW(
    _Out_writes_(iMaxLength) LPWSTR lpString1,
    _In_ LPCWSTR lpString2,
    _In_ int iMaxLength
    );
#ifdef UNICODE
#define lstrcpyn  lstrcpynW
#else
#define lstrcpyn  lstrcpynA
#endif // !UNICODE

#if defined(DEPRECATE_SUPPORTED)
#pragma warning(push)
#pragma warning(disable:4995)
#endif

WINBASEAPI
LPSTR
WINAPI
lstrcpyA(
    _Out_writes_(_String_length_(lpString2) + 1) LPSTR lpString1, // deprecated: annotation is as good as it gets
    _In_  LPCSTR lpString2
    );
WINBASEAPI
LPWSTR
WINAPI
lstrcpyW(
    _Out_writes_(_String_length_(lpString2) + 1) LPWSTR lpString1, // deprecated: annotation is as good as it gets
    _In_  LPCWSTR lpString2
    );
#ifdef UNICODE
#define lstrcpy  lstrcpyW
#else
#define lstrcpy  lstrcpyA
#endif // !UNICODE

WINBASEAPI
LPSTR
WINAPI
lstrcatA(
    _Inout_updates_z_(_String_length_(lpString1) + _String_length_(lpString2) + 1) LPSTR lpString1, // deprecated: annotation is as good as it gets
    _In_    LPCSTR lpString2
    );
WINBASEAPI
LPWSTR
WINAPI
lstrcatW(
    _Inout_updates_z_(_String_length_(lpString1) + _String_length_(lpString2) + 1) LPWSTR lpString1, // deprecated: annotation is as good as it gets
    _In_    LPCWSTR lpString2
    );
#ifdef UNICODE
#define lstrcat  lstrcatW
#else
#define lstrcat  lstrcatA
#endif // !UNICODE

#if defined(DEPRECATE_SUPPORTED)
#pragma warning(pop)
#endif

WINBASEAPI
int
WINAPI
lstrlenA(
    _In_ LPCSTR lpString
    );
WINBASEAPI
int
WINAPI
lstrlenW(
    _In_ LPCWSTR lpString
    );
#ifdef UNICODE
#define lstrlen  lstrlenW
#else
#define lstrlen  lstrlenA
#endif // !UNICODE

WINBASEAPI
HFILE
WINAPI
OpenFile(
    _In_    LPCSTR lpFileName,
    _Inout_ LPOFSTRUCT lpReOpenBuff,
    _In_    UINT uStyle
    );

WINBASEAPI
HFILE
WINAPI
_lopen(
    _In_ LPCSTR lpPathName,
    _In_ int iReadWrite
    );

WINBASEAPI
HFILE
WINAPI
_lcreat(
    _In_ LPCSTR lpPathName,
    _In_ int  iAttribute
    );

WINBASEAPI
UINT
WINAPI
_lread(
    _In_ HFILE hFile,
    _Out_writes_bytes_to_(uBytes, return) LPVOID lpBuffer,
    _In_ UINT uBytes
    );

WINBASEAPI
UINT
WINAPI
_lwrite(
    _In_ HFILE hFile,
    _In_reads_bytes_(uBytes) LPCCH lpBuffer,
    _In_ UINT uBytes
    );

WINBASEAPI
long
WINAPI
_hread(
    _In_ HFILE hFile,
    _Out_writes_bytes_to_(lBytes, return) LPVOID lpBuffer,
    _In_ long lBytes
    );

WINBASEAPI
long
WINAPI
_hwrite(
    _In_ HFILE hFile,
    _In_reads_bytes_(lBytes) LPCCH lpBuffer,
    _In_ long lBytes
    );

WINBASEAPI
HFILE
WINAPI
_lclose(
    _In_ HFILE hFile
    );

WINBASEAPI
LONG
WINAPI
_llseek(
    _In_ HFILE hFile,
    _In_ LONG lOffset,
    _In_ int iOrigin
    );

WINADVAPI
BOOL
WINAPI
IsTextUnicode(
    _In_reads_bytes_(iSize) CONST VOID* lpv,
    _In_        int iSize,
    _Inout_opt_ LPINT lpiResult
    );

#if(_WIN32_WINNT >= 0x0400)
WINBASEAPI
DWORD
WINAPI
SignalObjectAndWait(
    _In_ HANDLE hObjectToSignal,
    _In_ HANDLE hObjectToWaitOn,
    _In_ DWORD dwMilliseconds,
    _In_ BOOL bAlertable
    );
#endif /* _WIN32_WINNT >= 0x0400 */

WINBASEAPI
BOOL
WINAPI
BackupRead(
    _In_    HANDLE hFile,
    _Out_writes_bytes_to_(nNumberOfBytesToRead, *lpNumberOfBytesRead) LPBYTE lpBuffer,
    _In_    DWORD nNumberOfBytesToRead,
    _Out_   LPDWORD lpNumberOfBytesRead,
    _In_    BOOL bAbort,
    _In_    BOOL bProcessSecurity,
    _Inout_ LPVOID *lpContext
    );

WINBASEAPI
BOOL
WINAPI
BackupSeek(
    _In_    HANDLE hFile,
    _In_    DWORD  dwLowBytesToSeek,
    _In_    DWORD  dwHighBytesToSeek,
    _Out_   LPDWORD lpdwLowByteSeeked,
    _Out_   LPDWORD lpdwHighByteSeeked,
    _Inout_ LPVOID *lpContext
    );

WINBASEAPI
BOOL
WINAPI
BackupWrite(
    _In_    HANDLE hFile,
    _In_reads_bytes_(nNumberOfBytesToWrite) LPBYTE lpBuffer,
    _In_    DWORD nNumberOfBytesToWrite,
    _Out_   LPDWORD lpNumberOfBytesWritten,
    _In_    BOOL bAbort,
    _In_    BOOL bProcessSecurity,
    _Inout_ LPVOID *lpContext
    );

//
//  Stream id structure
//
typedef struct _WIN32_STREAM_ID {
        DWORD          dwStreamId ;
        DWORD          dwStreamAttributes ;
        LARGE_INTEGER  Size ;
        DWORD          dwStreamNameSize ;
        WCHAR          cStreamName[ ANYSIZE_ARRAY ] ;
} WIN32_STREAM_ID, *LPWIN32_STREAM_ID ;

//
//  Stream Ids
//

#define BACKUP_INVALID          0x00000000
#define BACKUP_DATA             0x00000001
#define BACKUP_EA_DATA          0x00000002
#define BACKUP_SECURITY_DATA    0x00000003
#define BACKUP_ALTERNATE_DATA   0x00000004
#define BACKUP_LINK             0x00000005
#define BACKUP_PROPERTY_DATA    0x00000006
#define BACKUP_OBJECT_ID        0x00000007
#define BACKUP_REPARSE_DATA     0x00000008
#define BACKUP_SPARSE_BLOCK     0x00000009
#define BACKUP_TXFS_DATA        0x0000000a


//
//  Stream Attributes
//

#define STREAM_NORMAL_ATTRIBUTE         0x00000000
#define STREAM_MODIFIED_WHEN_READ       0x00000001
#define STREAM_CONTAINS_SECURITY        0x00000002
#define STREAM_CONTAINS_PROPERTIES      0x00000004
#define STREAM_SPARSE_ATTRIBUTE         0x00000008

//
// Dual Mode API below this line. Dual Mode Structures also included.
//

#define STARTF_USESHOWWINDOW       0x00000001
#define STARTF_USESIZE             0x00000002
#define STARTF_USEPOSITION         0x00000004
#define STARTF_USECOUNTCHARS       0x00000008
#define STARTF_USEFILLATTRIBUTE    0x00000010
#define STARTF_RUNFULLSCREEN       0x00000020  // ignored for non-x86 platforms
#define STARTF_FORCEONFEEDBACK     0x00000040
#define STARTF_FORCEOFFFEEDBACK    0x00000080
#define STARTF_USESTDHANDLES       0x00000100

#if(WINVER >= 0x0400)

#define STARTF_USEHOTKEY           0x00000200
#define STARTF_TITLEISLINKNAME     0x00000800
#define STARTF_TITLEISAPPID        0x00001000
#define STARTF_PREVENTPINNING      0x00002000
#endif /* WINVER >= 0x0400 */

#if (_WIN32_WINNT >= 0x0600)

typedef struct _STARTUPINFOEXA {
    STARTUPINFOA StartupInfo;
    LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList;
} STARTUPINFOEXA, *LPSTARTUPINFOEXA;
typedef struct _STARTUPINFOEXW {
    STARTUPINFOW StartupInfo;
    LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList;
} STARTUPINFOEXW, *LPSTARTUPINFOEXW;
#ifdef UNICODE
typedef STARTUPINFOEXW STARTUPINFOEX;
typedef LPSTARTUPINFOEXW LPSTARTUPINFOEX;
#else
typedef STARTUPINFOEXA STARTUPINFOEX;
typedef LPSTARTUPINFOEXA LPSTARTUPINFOEX;
#endif // UNICODE

#endif // (_WIN32_WINNT >= 0x0600)

#define SHUTDOWN_NORETRY                0x00000001

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
OpenMutexA(
    _In_ DWORD dwDesiredAccess,
    _In_ BOOL bInheritHandle,
    _In_ LPCSTR lpName
    );
#ifndef UNICODE
#define OpenMutex  OpenMutexA
#endif

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
CreateSemaphoreA(
    _In_opt_ LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    _In_     LONG lInitialCount,
    _In_     LONG lMaximumCount,
    _In_opt_ LPCSTR lpName
    );
WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
CreateSemaphoreW(
    _In_opt_ LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    _In_     LONG lInitialCount,
    _In_     LONG lMaximumCount,
    _In_opt_ LPCWSTR lpName
    );
#ifdef UNICODE
#define CreateSemaphore  CreateSemaphoreW
#else
#define CreateSemaphore  CreateSemaphoreA
#endif // !UNICODE

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
OpenSemaphoreA(
    _In_ DWORD dwDesiredAccess,
    _In_ BOOL bInheritHandle,
    _In_ LPCSTR lpName
    );
#ifndef UNICODE
#define OpenSemaphore  OpenSemaphoreA
#endif

#if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
CreateWaitableTimerA(
    _In_opt_ LPSECURITY_ATTRIBUTES lpTimerAttributes,
    _In_     BOOL bManualReset,
    _In_opt_ LPCSTR lpTimerName
    );
WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
CreateWaitableTimerW(
    _In_opt_ LPSECURITY_ATTRIBUTES lpTimerAttributes,
    _In_     BOOL bManualReset,
    _In_opt_ LPCWSTR lpTimerName
    );
#ifdef UNICODE
#define CreateWaitableTimer  CreateWaitableTimerW
#else
#define CreateWaitableTimer  CreateWaitableTimerA
#endif // !UNICODE

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
OpenWaitableTimerA(
    _In_ DWORD dwDesiredAccess,
    _In_ BOOL bInheritHandle,
    _In_ LPCSTR lpTimerName
    );
#ifndef UNICODE
#define OpenWaitableTimer  OpenWaitableTimerA
#endif

#if (_WIN32_WINNT >= 0x0600)

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
CreateSemaphoreExA(
    _In_opt_    LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    _In_        LONG lInitialCount,
    _In_        LONG lMaximumCount,
    _In_opt_    LPCSTR lpName,
    _Reserved_  DWORD dwFlags,
    _In_        DWORD dwDesiredAccess
    );
#ifndef UNICODE
#define CreateSemaphoreEx  CreateSemaphoreExA
#endif

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
CreateWaitableTimerExA(
    _In_opt_ LPSECURITY_ATTRIBUTES lpTimerAttributes,
    _In_opt_ LPCSTR lpTimerName,
    _In_     DWORD dwFlags,
    _In_     DWORD dwDesiredAccess
    );
#ifndef UNICODE
#define CreateWaitableTimerEx  CreateWaitableTimerExA
#endif

#endif /* (_WIN32_WINNT >= 0x0600) */

#endif /* (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400) */

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
CreateFileMappingA(
    _In_     HANDLE hFile,
    _In_opt_ LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    _In_     DWORD flProtect,
    _In_     DWORD dwMaximumSizeHigh,
    _In_     DWORD dwMaximumSizeLow,
    _In_opt_ LPCSTR lpName
    );
#ifndef UNICODE
#define CreateFileMapping  CreateFileMappingA
#endif

#if _WIN32_WINNT >= 0x0600

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
CreateFileMappingNumaA(
    _In_     HANDLE hFile,
    _In_opt_ LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    _In_     DWORD flProtect,
    _In_     DWORD dwMaximumSizeHigh,
    _In_     DWORD dwMaximumSizeLow,
    _In_opt_ LPCSTR lpName,
    _In_     DWORD nndPreferred
    );

#ifndef UNICODE
#define CreateFileMappingNuma  CreateFileMappingNumaA
#endif

#endif // _WIN32_WINNT >= 0x0600

WINBASEAPI
HANDLE
WINAPI
OpenFileMappingA(
    _In_ DWORD dwDesiredAccess,
    _In_ BOOL bInheritHandle,
    _In_ LPCSTR lpName
    );
#ifndef UNICODE
#define OpenFileMapping  OpenFileMappingA
#endif

WINBASEAPI
_Success_(return != 0 && return <= nBufferLength)
DWORD
WINAPI
GetLogicalDriveStringsA(
    _In_ DWORD nBufferLength,
    _Out_writes_to_opt_(nBufferLength, return + 1) LPSTR lpBuffer
    );
#ifndef UNICODE
#define GetLogicalDriveStrings  GetLogicalDriveStringsA
#endif


WINBASEAPI
_Ret_maybenull_
HMODULE
WINAPI
LoadLibraryA(
    _In_ LPCSTR lpLibFileName
    );
WINBASEAPI
_Ret_maybenull_
HMODULE
WINAPI
LoadLibraryW(
    _In_ LPCWSTR lpLibFileName
    );
#ifdef UNICODE
#define LoadLibrary  LoadLibraryW
#else
#define LoadLibrary  LoadLibraryA
#endif // !UNICODE

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#pragma region Application Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)

#if (_WIN32_WINNT >= 0x0602)

WINBASEAPI
_Ret_maybenull_
HMODULE
WINAPI
LoadPackagedLibrary (
    _In_       LPCWSTR lpwLibFileName,
    _Reserved_ DWORD Reserved
    );

#endif // _WIN32_WINNT >= 0x0602

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

#if (_WIN32_WINNT >= 0x0600)

#define PROCESS_NAME_NATIVE     0x00000001

WINBASEAPI
BOOL
WINAPI
QueryFullProcessImageNameA(
    _In_ HANDLE hProcess,
    _In_ DWORD dwFlags,
    _Out_writes_to_(*lpdwSize, *lpdwSize) LPSTR lpExeName,
    _Inout_ PDWORD lpdwSize
    );
WINBASEAPI
BOOL
WINAPI
QueryFullProcessImageNameW(
    _In_ HANDLE hProcess,
    _In_ DWORD dwFlags,
    _Out_writes_to_(*lpdwSize, *lpdwSize) LPWSTR lpExeName,
    _Inout_ PDWORD lpdwSize
    );
#ifdef UNICODE
#define QueryFullProcessImageName  QueryFullProcessImageNameW
#else
#define QueryFullProcessImageName  QueryFullProcessImageNameA
#endif // !UNICODE

#endif

#if (_WIN32_WINNT >= 0x0600)

//
// Extended process and thread attribute support
//

#define PROC_THREAD_ATTRIBUTE_NUMBER    0x0000FFFF
#define PROC_THREAD_ATTRIBUTE_THREAD    0x00010000  // Attribute may be used with thread creation
#define PROC_THREAD_ATTRIBUTE_INPUT     0x00020000  // Attribute is input only
#define PROC_THREAD_ATTRIBUTE_ADDITIVE  0x00040000  // Attribute may be "accumulated," e.g. bitmasks, counters, etc.


#ifndef _USE_FULL_PROC_THREAD_ATTRIBUTE
typedef enum _PROC_THREAD_ATTRIBUTE_NUM {
    ProcThreadAttributeParentProcess        = 0,
    ProcThreadAttributeHandleList           = 2,
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
    ProcThreadAttributeGroupAffinity        = 3,
    ProcThreadAttributePreferredNode        = 4,
    ProcThreadAttributeIdealProcessor       = 5,
    ProcThreadAttributeUmsThread            = 6,
    ProcThreadAttributeMitigationPolicy     = 7,
#endif
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
    ProcThreadAttributeSecurityCapabilities = 9,
#endif
} PROC_THREAD_ATTRIBUTE_NUM;
#endif

#define ProcThreadAttributeValue(Number, Thread, Input, Additive) \
    (((Number) & PROC_THREAD_ATTRIBUTE_NUMBER) | \
     ((Thread != FALSE) ? PROC_THREAD_ATTRIBUTE_THREAD : 0) | \
     ((Input != FALSE) ? PROC_THREAD_ATTRIBUTE_INPUT : 0) | \
     ((Additive != FALSE) ? PROC_THREAD_ATTRIBUTE_ADDITIVE : 0))

#define PROC_THREAD_ATTRIBUTE_PARENT_PROCESS \
    ProcThreadAttributeValue (ProcThreadAttributeParentProcess, FALSE, TRUE, FALSE)
#define PROC_THREAD_ATTRIBUTE_HANDLE_LIST \
    ProcThreadAttributeValue (ProcThreadAttributeHandleList, FALSE, TRUE, FALSE)

#endif // (_WIN32_WINNT >= 0x0600)

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
#define PROC_THREAD_ATTRIBUTE_GROUP_AFFINITY \
    ProcThreadAttributeValue (ProcThreadAttributeGroupAffinity, TRUE, TRUE, FALSE)
#define PROC_THREAD_ATTRIBUTE_PREFERRED_NODE \
    ProcThreadAttributeValue (ProcThreadAttributePreferredNode, FALSE, TRUE, FALSE)
#define PROC_THREAD_ATTRIBUTE_IDEAL_PROCESSOR \
    ProcThreadAttributeValue (ProcThreadAttributeIdealProcessor, TRUE, TRUE, FALSE)
#define PROC_THREAD_ATTRIBUTE_UMS_THREAD \
    ProcThreadAttributeValue (ProcThreadAttributeUmsThread, TRUE, TRUE, FALSE)
#define PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY \
    ProcThreadAttributeValue (ProcThreadAttributeMitigationPolicy, FALSE, TRUE, FALSE)
#endif

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
#define PROC_THREAD_ATTRIBUTE_SECURITY_CAPABILITIES \
    ProcThreadAttributeValue (ProcThreadAttributeSecurityCapabilities, FALSE, TRUE, FALSE)
#endif

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
//
// Define legacy creation mitigation policy options, which are straight
// bitmasks.  Bits 0-5 are legacy bits.
//

#define PROCESS_CREATION_MITIGATION_POLICY_DEP_ENABLE            0x01
#define PROCESS_CREATION_MITIGATION_POLICY_DEP_ATL_THUNK_ENABLE  0x02
#define PROCESS_CREATION_MITIGATION_POLICY_SEHOP_ENABLE          0x04
#endif


#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
//
// Define mandatory ASLR options.  Mandatory ASLR forcibly rebases images that
// are not dynamic base compatible by acting as though there were an image base
// collision at load time.
//
// Note that 'require relocations' mode refuses load of images that do not have
// a base relocation section.
//

#define PROCESS_CREATION_MITIGATION_POLICY_FORCE_RELOCATE_IMAGES_MASK                     (0x00000003 <<  8)
#define PROCESS_CREATION_MITIGATION_POLICY_FORCE_RELOCATE_IMAGES_DEFER                    (0x00000000 <<  8)
#define PROCESS_CREATION_MITIGATION_POLICY_FORCE_RELOCATE_IMAGES_ALWAYS_ON                (0x00000001 <<  8)
#define PROCESS_CREATION_MITIGATION_POLICY_FORCE_RELOCATE_IMAGES_ALWAYS_OFF               (0x00000002 <<  8)
#define PROCESS_CREATION_MITIGATION_POLICY_FORCE_RELOCATE_IMAGES_ALWAYS_ON_REQ_RELOCS     (0x00000003 <<  8)

//
// Define heap terminate on corruption options.  Note that 'always off' does
// not override the default opt-in for binaries with current subsystem versions
// set in the image header.
//
// Heap terminate on corruption is user mode enforced.
//

#define PROCESS_CREATION_MITIGATION_POLICY_HEAP_TERMINATE_MASK                            (0x00000003 << 12)
#define PROCESS_CREATION_MITIGATION_POLICY_HEAP_TERMINATE_DEFER                           (0x00000000 << 12)
#define PROCESS_CREATION_MITIGATION_POLICY_HEAP_TERMINATE_ALWAYS_ON                       (0x00000001 << 12)
#define PROCESS_CREATION_MITIGATION_POLICY_HEAP_TERMINATE_ALWAYS_OFF                      (0x00000002 << 12)
#define PROCESS_CREATION_MITIGATION_POLICY_HEAP_TERMINATE_RESERVED                        (0x00000003 << 12)

//
// Define bottom up randomization (includes stack randomization) options,
// i.e. randomization of the lowest user address.
//

#define PROCESS_CREATION_MITIGATION_POLICY_BOTTOM_UP_ASLR_MASK                            (0x00000003 << 16)
#define PROCESS_CREATION_MITIGATION_POLICY_BOTTOM_UP_ASLR_DEFER                           (0x00000000 << 16)
#define PROCESS_CREATION_MITIGATION_POLICY_BOTTOM_UP_ASLR_ALWAYS_ON                       (0x00000001 << 16)
#define PROCESS_CREATION_MITIGATION_POLICY_BOTTOM_UP_ASLR_ALWAYS_OFF                      (0x00000002 << 16)
#define PROCESS_CREATION_MITIGATION_POLICY_BOTTOM_UP_ASLR_RESERVED                        (0x00000003 << 16)

//
// Define high entropy bottom up randomization.  Note that high entropy bottom
// up randomization is effective if and only if bottom up ASLR is also enabled.
//
// N.B.  High entropy mode is only meaningful for native 64-bit processes.  in
//       high entropy mode, up to 1TB of bottom up variance is enabled.
//

#define PROCESS_CREATION_MITIGATION_POLICY_HIGH_ENTROPY_ASLR_MASK                         (0x00000003 << 20)
#define PROCESS_CREATION_MITIGATION_POLICY_HIGH_ENTROPY_ASLR_DEFER                        (0x00000000 << 20)
#define PROCESS_CREATION_MITIGATION_POLICY_HIGH_ENTROPY_ASLR_ALWAYS_ON                    (0x00000001 << 20)
#define PROCESS_CREATION_MITIGATION_POLICY_HIGH_ENTROPY_ASLR_ALWAYS_OFF                   (0x00000002 << 20)
#define PROCESS_CREATION_MITIGATION_POLICY_HIGH_ENTROPY_ASLR_RESERVED                     (0x00000003 << 20)

//
// Define handle checking enforcement options.  Handle checking enforcement
// causes an exception to be raised immediately on a bad handle reference,
// versus simply returning a failure status from the handle reference.
//

#define PROCESS_CREATION_MITIGATION_POLICY_STRICT_HANDLE_CHECKS_MASK                      (0x00000003 << 24)
#define PROCESS_CREATION_MITIGATION_POLICY_STRICT_HANDLE_CHECKS_DEFER                     (0x00000000 << 24)
#define PROCESS_CREATION_MITIGATION_POLICY_STRICT_HANDLE_CHECKS_ALWAYS_ON                 (0x00000001 << 24)
#define PROCESS_CREATION_MITIGATION_POLICY_STRICT_HANDLE_CHECKS_ALWAYS_OFF                (0x00000002 << 24)
#define PROCESS_CREATION_MITIGATION_POLICY_STRICT_HANDLE_CHECKS_RESERVED                  (0x00000003 << 24)

//
// Define win32k system call disable options.  Win32k system call disable
// prevents a process from making Win32k calls.
//

#define PROCESS_CREATION_MITIGATION_POLICY_WIN32K_SYSTEM_CALL_DISABLE_MASK                (0x00000003 << 28)
#define PROCESS_CREATION_MITIGATION_POLICY_WIN32K_SYSTEM_CALL_DISABLE_DEFER               (0x00000000 << 28)
#define PROCESS_CREATION_MITIGATION_POLICY_WIN32K_SYSTEM_CALL_DISABLE_ALWAYS_ON           (0x00000001 << 28)
#define PROCESS_CREATION_MITIGATION_POLICY_WIN32K_SYSTEM_CALL_DISABLE_ALWAYS_OFF          (0x00000002 << 28)
#define PROCESS_CREATION_MITIGATION_POLICY_WIN32K_SYSTEM_CALL_DISABLE_RESERVED            (0x00000003 << 28)

//
// Define the extension point disable options.  Extension point disable allows
// a process to opt-out of loading various arbitrary extension point DLLs.
//

#define PROCESS_CREATION_MITIGATION_POLICY_EXTENSION_POINT_DISABLE_MASK                   (0x00000003ui64 << 32)
#define PROCESS_CREATION_MITIGATION_POLICY_EXTENSION_POINT_DISABLE_DEFER                  (0x00000000ui64 << 32)
#define PROCESS_CREATION_MITIGATION_POLICY_EXTENSION_POINT_DISABLE_ALWAYS_ON              (0x00000001ui64 << 32)
#define PROCESS_CREATION_MITIGATION_POLICY_EXTENSION_POINT_DISABLE_ALWAYS_OFF             (0x00000002ui64 << 32)
#define PROCESS_CREATION_MITIGATION_POLICY_EXTENSION_POINT_DISABLE_RESERVED               (0x00000003ui64 << 32)
#endif


WINBASEAPI
BOOL
WINAPI
GetProcessShutdownParameters(
    _Out_ LPDWORD lpdwLevel,
    _Out_ LPDWORD lpdwFlags
    );

WINBASEAPI
VOID
WINAPI
FatalAppExitA(
    _In_ UINT uAction,
    _In_ LPCSTR lpMessageText
    );
WINBASEAPI
VOID
WINAPI
FatalAppExitW(
    _In_ UINT uAction,
    _In_ LPCWSTR lpMessageText
    );
#ifdef UNICODE
#define FatalAppExit  FatalAppExitW
#else
#define FatalAppExit  FatalAppExitA
#endif // !UNICODE

WINBASEAPI
VOID
WINAPI
GetStartupInfoA(
    _Out_ LPSTARTUPINFOA lpStartupInfo
    );
#ifndef UNICODE
#define GetStartupInfo  GetStartupInfoA
#endif

#if defined(_M_CEE)
#undef GetEnvironmentVariable
__inline
DWORD
GetEnvironmentVariable(
    _In_opt_ LPCTSTR lpName,
    _Out_writes_to_opt_(nSize, return + 1) LPTSTR lpBuffer,
    _In_ DWORD nSize
    )
{
#ifdef UNICODE
    return GetEnvironmentVariableW(
#else
    return GetEnvironmentVariableA(
#endif
        lpName,
        lpBuffer,
        nSize
        );
}
#endif  /* _M_CEE */

#if defined(_M_CEE)
#undef SetEnvironmentVariable
__inline
BOOL
SetEnvironmentVariable(
    LPCTSTR lpName,
    LPCTSTR lpValue
    )
{
#ifdef UNICODE
    return SetEnvironmentVariableW(
#else
    return SetEnvironmentVariableA(
#endif
        lpName,
        lpValue
        );
}
#endif  /* _M_CEE */

WINBASEAPI
DWORD
WINAPI
GetFirmwareEnvironmentVariableA(
    _In_ LPCSTR lpName,
    _In_ LPCSTR lpGuid,
    _Out_writes_bytes_to_opt_(nSize, return) PVOID pBuffer,
    _In_ DWORD    nSize
    );
WINBASEAPI
DWORD
WINAPI
GetFirmwareEnvironmentVariableW(
    _In_ LPCWSTR lpName,
    _In_ LPCWSTR lpGuid,
    _Out_writes_bytes_to_opt_(nSize, return) PVOID pBuffer,
    _In_ DWORD    nSize
    );
#ifdef UNICODE
#define GetFirmwareEnvironmentVariable  GetFirmwareEnvironmentVariableW
#else
#define GetFirmwareEnvironmentVariable  GetFirmwareEnvironmentVariableA
#endif // !UNICODE

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)

WINBASEAPI
DWORD
WINAPI
GetFirmwareEnvironmentVariableExA(
    _In_ LPCSTR lpName,
    _In_ LPCSTR lpGuid,
    _Out_writes_bytes_to_opt_(nSize, return) PVOID pBuffer,
    _In_ DWORD    nSize,
    _Out_opt_ PDWORD pdwAttribubutes
    );
WINBASEAPI
DWORD
WINAPI
GetFirmwareEnvironmentVariableExW(
    _In_ LPCWSTR lpName,
    _In_ LPCWSTR lpGuid,
    _Out_writes_bytes_to_opt_(nSize, return) PVOID pBuffer,
    _In_ DWORD    nSize,
    _Out_opt_ PDWORD pdwAttribubutes
    );
#ifdef UNICODE
#define GetFirmwareEnvironmentVariableEx  GetFirmwareEnvironmentVariableExW
#else
#define GetFirmwareEnvironmentVariableEx  GetFirmwareEnvironmentVariableExA
#endif // !UNICODE

#endif

WINBASEAPI
BOOL
WINAPI
SetFirmwareEnvironmentVariableA(
    _In_ LPCSTR lpName,
    _In_ LPCSTR lpGuid,
    _In_reads_bytes_opt_(nSize) PVOID pValue,
    _In_ DWORD    nSize
    );
WINBASEAPI
BOOL
WINAPI
SetFirmwareEnvironmentVariableW(
    _In_ LPCWSTR lpName,
    _In_ LPCWSTR lpGuid,
    _In_reads_bytes_opt_(nSize) PVOID pValue,
    _In_ DWORD    nSize
    );
#ifdef UNICODE
#define SetFirmwareEnvironmentVariable  SetFirmwareEnvironmentVariableW
#else
#define SetFirmwareEnvironmentVariable  SetFirmwareEnvironmentVariableA
#endif // !UNICODE

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)

WINBASEAPI
BOOL
WINAPI
SetFirmwareEnvironmentVariableExA(
    _In_ LPCSTR lpName,
    _In_ LPCSTR lpGuid,
    _In_reads_bytes_opt_(nSize) PVOID pValue,
    _In_ DWORD    nSize,
    _In_ DWORD    dwAttributes
    );
WINBASEAPI
BOOL
WINAPI
SetFirmwareEnvironmentVariableExW(
    _In_ LPCWSTR lpName,
    _In_ LPCWSTR lpGuid,
    _In_reads_bytes_opt_(nSize) PVOID pValue,
    _In_ DWORD    nSize,
    _In_ DWORD    dwAttributes
    );
#ifdef UNICODE
#define SetFirmwareEnvironmentVariableEx  SetFirmwareEnvironmentVariableExW
#else
#define SetFirmwareEnvironmentVariableEx  SetFirmwareEnvironmentVariableExA
#endif // !UNICODE

#endif

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)

WINBASEAPI
BOOL
WINAPI
GetFirmwareType (
    _Inout_ PFIRMWARE_TYPE FirmwareType
    );


WINBASEAPI
BOOL
WINAPI
IsNativeVhdBoot (
    _Out_ PBOOL NativeVhdBoot
    );

#endif // _WIN32_WINNT >= _WIN32_WINNT_WIN8

WINBASEAPI
_Ret_maybenull_
HRSRC
WINAPI
FindResourceA(
    _In_opt_ HMODULE hModule,
    _In_     LPCSTR lpName,
    _In_     LPCSTR lpType
    );
WINBASEAPI
_Ret_maybenull_
HRSRC
WINAPI
FindResourceW(
    _In_opt_ HMODULE hModule,
    _In_     LPCWSTR lpName,
    _In_     LPCWSTR lpType
    );
#ifdef UNICODE
#define FindResource  FindResourceW
#else
#define FindResource  FindResourceA
#endif // !UNICODE

WINBASEAPI
_Ret_maybenull_
HRSRC
WINAPI
FindResourceExA(
    _In_opt_ HMODULE hModule,
    _In_     LPCSTR lpType,
    _In_     LPCSTR lpName,
    _In_     WORD    wLanguage
    );
#ifndef UNICODE
#define FindResourceEx  FindResourceExA
#endif

WINBASEAPI
BOOL
WINAPI
EnumResourceTypesA(
    _In_opt_ HMODULE hModule,
    _In_     ENUMRESTYPEPROCA lpEnumFunc,
    _In_     LONG_PTR lParam
    );
WINBASEAPI
BOOL
WINAPI
EnumResourceTypesW(
    _In_opt_ HMODULE hModule,
    _In_     ENUMRESTYPEPROCW lpEnumFunc,
    _In_     LONG_PTR lParam
    );
#ifdef UNICODE
#define EnumResourceTypes  EnumResourceTypesW
#else
#define EnumResourceTypes  EnumResourceTypesA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
EnumResourceNamesA(
    _In_opt_ HMODULE hModule,
    _In_     LPCSTR lpType,
    _In_     ENUMRESNAMEPROCA lpEnumFunc,
    _In_     LONG_PTR lParam
    );
WINBASEAPI
BOOL
WINAPI
EnumResourceNamesW(
    _In_opt_ HMODULE hModule,
    _In_     LPCWSTR lpType,
    _In_     ENUMRESNAMEPROCW lpEnumFunc,
    _In_     LONG_PTR lParam
    );
#ifdef UNICODE
#define EnumResourceNames  EnumResourceNamesW
#else
#define EnumResourceNames  EnumResourceNamesA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
EnumResourceLanguagesA(
    _In_opt_ HMODULE hModule,
    _In_     LPCSTR lpType,
    _In_     LPCSTR lpName,
    _In_     ENUMRESLANGPROCA lpEnumFunc,
    _In_     LONG_PTR lParam
    );
WINBASEAPI
BOOL
WINAPI
EnumResourceLanguagesW(
    _In_opt_ HMODULE hModule,
    _In_     LPCWSTR lpType,
    _In_     LPCWSTR lpName,
    _In_     ENUMRESLANGPROCW lpEnumFunc,
    _In_     LONG_PTR lParam
    );
#ifdef UNICODE
#define EnumResourceLanguages  EnumResourceLanguagesW
#else
#define EnumResourceLanguages  EnumResourceLanguagesA
#endif // !UNICODE

WINBASEAPI
HANDLE
WINAPI
BeginUpdateResourceA(
    _In_ LPCSTR pFileName,
    _In_ BOOL bDeleteExistingResources
    );
WINBASEAPI
HANDLE
WINAPI
BeginUpdateResourceW(
    _In_ LPCWSTR pFileName,
    _In_ BOOL bDeleteExistingResources
    );
#ifdef UNICODE
#define BeginUpdateResource  BeginUpdateResourceW
#else
#define BeginUpdateResource  BeginUpdateResourceA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
UpdateResourceA(
    _In_ HANDLE hUpdate,
    _In_ LPCSTR lpType,
    _In_ LPCSTR lpName,
    _In_ WORD wLanguage,
    _In_reads_bytes_opt_(cb) LPVOID lpData,
    _In_ DWORD cb
    );
WINBASEAPI
BOOL
WINAPI
UpdateResourceW(
    _In_ HANDLE hUpdate,
    _In_ LPCWSTR lpType,
    _In_ LPCWSTR lpName,
    _In_ WORD wLanguage,
    _In_reads_bytes_opt_(cb) LPVOID lpData,
    _In_ DWORD cb
    );
#ifdef UNICODE
#define UpdateResource  UpdateResourceW
#else
#define UpdateResource  UpdateResourceA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
EndUpdateResourceA(
    _In_ HANDLE hUpdate,
    _In_ BOOL   fDiscard
    );
WINBASEAPI
BOOL
WINAPI
EndUpdateResourceW(
    _In_ HANDLE hUpdate,
    _In_ BOOL   fDiscard
    );
#ifdef UNICODE
#define EndUpdateResource  EndUpdateResourceW
#else
#define EndUpdateResource  EndUpdateResourceA
#endif // !UNICODE

#define ATOM_FLAG_GLOBAL 0x2

WINBASEAPI
ATOM
WINAPI
GlobalAddAtomA(
    _In_opt_ LPCSTR lpString
    );
WINBASEAPI
ATOM
WINAPI
GlobalAddAtomW(
    _In_opt_ LPCWSTR lpString
    );
#ifdef UNICODE
#define GlobalAddAtom  GlobalAddAtomW
#else
#define GlobalAddAtom  GlobalAddAtomA
#endif // !UNICODE

WINBASEAPI
ATOM
WINAPI
GlobalAddAtomExA(
    _In_opt_ LPCSTR lpString,
    _In_ DWORD Flags
    );
WINBASEAPI
ATOM
WINAPI
GlobalAddAtomExW(
    _In_opt_ LPCWSTR lpString,
    _In_ DWORD Flags
    );
#ifdef UNICODE
#define GlobalAddAtomEx  GlobalAddAtomExW
#else
#define GlobalAddAtomEx  GlobalAddAtomExA
#endif // !UNICODE

WINBASEAPI
ATOM
WINAPI
GlobalFindAtomA(
    _In_opt_ LPCSTR lpString
    );
WINBASEAPI
ATOM
WINAPI
GlobalFindAtomW(
    _In_opt_ LPCWSTR lpString
    );
#ifdef UNICODE
#define GlobalFindAtom  GlobalFindAtomW
#else
#define GlobalFindAtom  GlobalFindAtomA
#endif // !UNICODE

WINBASEAPI
UINT
WINAPI
GlobalGetAtomNameA(
    _In_ ATOM nAtom,
    _Out_writes_to_(nSize, return + 1) LPSTR lpBuffer,
    _In_ int nSize
    );
WINBASEAPI
UINT
WINAPI
GlobalGetAtomNameW(
    _In_ ATOM nAtom,
    _Out_writes_to_(nSize, return + 1) LPWSTR lpBuffer,
    _In_ int nSize
    );
#ifdef UNICODE
#define GlobalGetAtomName  GlobalGetAtomNameW
#else
#define GlobalGetAtomName  GlobalGetAtomNameA
#endif // !UNICODE

WINBASEAPI
ATOM
WINAPI
AddAtomA(
    _In_opt_ LPCSTR lpString
    );
WINBASEAPI
ATOM
WINAPI
AddAtomW(
    _In_opt_ LPCWSTR lpString
    );
#ifdef UNICODE
#define AddAtom  AddAtomW
#else
#define AddAtom  AddAtomA
#endif // !UNICODE

WINBASEAPI
ATOM
WINAPI
FindAtomA(
    _In_opt_ LPCSTR lpString
    );
WINBASEAPI
ATOM
WINAPI
FindAtomW(
    _In_opt_ LPCWSTR lpString
    );
#ifdef UNICODE
#define FindAtom  FindAtomW
#else
#define FindAtom  FindAtomA
#endif // !UNICODE

WINBASEAPI
UINT
WINAPI
GetAtomNameA(
    _In_ ATOM nAtom,
    _Out_writes_to_(nSize, return + 1) LPSTR lpBuffer,
    _In_ int nSize
    );
WINBASEAPI
UINT
WINAPI
GetAtomNameW(
    _In_ ATOM nAtom,
    _Out_writes_to_(nSize, return + 1) LPWSTR lpBuffer,
    _In_ int nSize
    );
#ifdef UNICODE
#define GetAtomName  GetAtomNameW
#else
#define GetAtomName  GetAtomNameA
#endif // !UNICODE

WINBASEAPI
UINT
WINAPI
GetProfileIntA(
    _In_ LPCSTR lpAppName,
    _In_ LPCSTR lpKeyName,
    _In_ INT nDefault
    );
WINBASEAPI
UINT
WINAPI
GetProfileIntW(
    _In_ LPCWSTR lpAppName,
    _In_ LPCWSTR lpKeyName,
    _In_ INT nDefault
    );
#ifdef UNICODE
#define GetProfileInt  GetProfileIntW
#else
#define GetProfileInt  GetProfileIntA
#endif // !UNICODE

WINBASEAPI
DWORD
WINAPI
GetProfileStringA(
    _In_opt_ LPCSTR lpAppName,
    _In_opt_ LPCSTR lpKeyName,
    _In_opt_ LPCSTR lpDefault,
    _Out_writes_to_opt_(nSize, return + 1) LPSTR lpReturnedString,
    _In_     DWORD nSize
    );
WINBASEAPI
DWORD
WINAPI
GetProfileStringW(
    _In_opt_ LPCWSTR lpAppName,
    _In_opt_ LPCWSTR lpKeyName,
    _In_opt_ LPCWSTR lpDefault,
    _Out_writes_to_opt_(nSize, return + 1) LPWSTR lpReturnedString,
    _In_     DWORD nSize
    );
#ifdef UNICODE
#define GetProfileString  GetProfileStringW
#else
#define GetProfileString  GetProfileStringA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
WriteProfileStringA(
    _In_opt_ LPCSTR lpAppName,
    _In_opt_ LPCSTR lpKeyName,
    _In_opt_ LPCSTR lpString
    );
WINBASEAPI
BOOL
WINAPI
WriteProfileStringW(
    _In_opt_ LPCWSTR lpAppName,
    _In_opt_ LPCWSTR lpKeyName,
    _In_opt_ LPCWSTR lpString
    );
#ifdef UNICODE
#define WriteProfileString  WriteProfileStringW
#else
#define WriteProfileString  WriteProfileStringA
#endif // !UNICODE

WINBASEAPI
DWORD
WINAPI
GetProfileSectionA(
    _In_ LPCSTR lpAppName,
    _Out_writes_to_opt_(nSize, return + 1) LPSTR lpReturnedString,
    _In_ DWORD nSize
    );
WINBASEAPI
DWORD
WINAPI
GetProfileSectionW(
    _In_ LPCWSTR lpAppName,
    _Out_writes_to_opt_(nSize, return + 1) LPWSTR lpReturnedString,
    _In_ DWORD nSize
    );
#ifdef UNICODE
#define GetProfileSection  GetProfileSectionW
#else
#define GetProfileSection  GetProfileSectionA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
WriteProfileSectionA(
    _In_ LPCSTR lpAppName,
    _In_ LPCSTR lpString
    );
WINBASEAPI
BOOL
WINAPI
WriteProfileSectionW(
    _In_ LPCWSTR lpAppName,
    _In_ LPCWSTR lpString
    );
#ifdef UNICODE
#define WriteProfileSection  WriteProfileSectionW
#else
#define WriteProfileSection  WriteProfileSectionA
#endif // !UNICODE

WINBASEAPI
UINT
WINAPI
GetPrivateProfileIntA(
    _In_     LPCSTR lpAppName,
    _In_     LPCSTR lpKeyName,
    _In_     INT nDefault,
    _In_opt_ LPCSTR lpFileName
    );
WINBASEAPI
UINT
WINAPI
GetPrivateProfileIntW(
    _In_     LPCWSTR lpAppName,
    _In_     LPCWSTR lpKeyName,
    _In_     INT nDefault,
    _In_opt_ LPCWSTR lpFileName
    );
#ifdef UNICODE
#define GetPrivateProfileInt  GetPrivateProfileIntW
#else
#define GetPrivateProfileInt  GetPrivateProfileIntA
#endif // !UNICODE

#if defined(_M_CEE)
#undef GetPrivateProfileInt
__inline
UINT
GetPrivateProfileInt(
    LPCTSTR lpAppName,
    LPCTSTR lpKeyName,
    INT nDefault,
    LPCTSTR lpFileName
    )
{
#ifdef UNICODE
    return GetPrivateProfileIntW(
#else
    return GetPrivateProfileIntA(
#endif
        lpAppName,
        lpKeyName,
        nDefault,
        lpFileName
        );
}
#endif  /* _M_CEE */

WINBASEAPI
DWORD
WINAPI
GetPrivateProfileStringA(
    _In_opt_ LPCSTR lpAppName,
    _In_opt_ LPCSTR lpKeyName,
    _In_opt_ LPCSTR lpDefault,
    _Out_writes_to_opt_(nSize, return + 1) LPSTR lpReturnedString,
    _In_     DWORD nSize,
    _In_opt_ LPCSTR lpFileName
    );
WINBASEAPI
DWORD
WINAPI
GetPrivateProfileStringW(
    _In_opt_ LPCWSTR lpAppName,
    _In_opt_ LPCWSTR lpKeyName,
    _In_opt_ LPCWSTR lpDefault,
    _Out_writes_to_opt_(nSize, return + 1) LPWSTR lpReturnedString,
    _In_     DWORD nSize,
    _In_opt_ LPCWSTR lpFileName
    );
#ifdef UNICODE
#define GetPrivateProfileString  GetPrivateProfileStringW
#else
#define GetPrivateProfileString  GetPrivateProfileStringA
#endif // !UNICODE

#if defined(_M_CEE)
#undef GetPrivateProfileString
__inline
DWORD
GetPrivateProfileString(
    LPCTSTR lpAppName,
    LPCTSTR lpKeyName,
    LPCTSTR lpDefault,
    LPTSTR lpReturnedString,
    DWORD nSize,
    LPCTSTR lpFileName
    )
{
#ifdef UNICODE
    return GetPrivateProfileStringW(
#else
    return GetPrivateProfileStringA(
#endif
        lpAppName,
        lpKeyName,
        lpDefault,
        lpReturnedString,
        nSize,
        lpFileName
        );
}
#endif  /* _M_CEE */

WINBASEAPI
BOOL
WINAPI
WritePrivateProfileStringA(
    _In_opt_ LPCSTR lpAppName,
    _In_opt_ LPCSTR lpKeyName,
    _In_opt_ LPCSTR lpString,
    _In_opt_ LPCSTR lpFileName
    );
WINBASEAPI
BOOL
WINAPI
WritePrivateProfileStringW(
    _In_opt_ LPCWSTR lpAppName,
    _In_opt_ LPCWSTR lpKeyName,
    _In_opt_ LPCWSTR lpString,
    _In_opt_ LPCWSTR lpFileName
    );
#ifdef UNICODE
#define WritePrivateProfileString  WritePrivateProfileStringW
#else
#define WritePrivateProfileString  WritePrivateProfileStringA
#endif // !UNICODE

WINBASEAPI
DWORD
WINAPI
GetPrivateProfileSectionA(
    _In_     LPCSTR lpAppName,
    _Out_writes_to_opt_(nSize, return + 1) LPSTR lpReturnedString,
    _In_     DWORD nSize,
    _In_opt_ LPCSTR lpFileName
    );
WINBASEAPI
DWORD
WINAPI
GetPrivateProfileSectionW(
    _In_     LPCWSTR lpAppName,
    _Out_writes_to_opt_(nSize, return + 1) LPWSTR lpReturnedString,
    _In_     DWORD nSize,
    _In_opt_ LPCWSTR lpFileName
    );
#ifdef UNICODE
#define GetPrivateProfileSection  GetPrivateProfileSectionW
#else
#define GetPrivateProfileSection  GetPrivateProfileSectionA
#endif // !UNICODE

#if defined(_M_CEE)
#undef GetPrivateProfileSection
__inline
DWORD
GetPrivateProfileSection(
    LPCTSTR lpAppName,
    LPTSTR lpReturnedString,
    DWORD nSize,
    LPCTSTR lpFileName
    )
{
#ifdef UNICODE
    return GetPrivateProfileSectionW(
#else
    return GetPrivateProfileSectionA(
#endif
        lpAppName,
        lpReturnedString,
        nSize,
        lpFileName
        );
}
#endif  /* _M_CEE */

WINBASEAPI
BOOL
WINAPI
WritePrivateProfileSectionA(
    _In_opt_ LPCSTR lpAppName,
    _In_opt_ LPCSTR lpString,
    _In_opt_ LPCSTR lpFileName
    );
WINBASEAPI
BOOL
WINAPI
WritePrivateProfileSectionW(
    _In_opt_ LPCWSTR lpAppName,
    _In_opt_ LPCWSTR lpString,
    _In_opt_ LPCWSTR lpFileName
    );
#ifdef UNICODE
#define WritePrivateProfileSection  WritePrivateProfileSectionW
#else
#define WritePrivateProfileSection  WritePrivateProfileSectionA
#endif // !UNICODE


WINBASEAPI
DWORD
WINAPI
GetPrivateProfileSectionNamesA(
    _Out_writes_to_opt_(nSize, return + 1) LPSTR lpszReturnBuffer,
    _In_     DWORD nSize,
    _In_opt_ LPCSTR lpFileName
    );
WINBASEAPI
DWORD
WINAPI
GetPrivateProfileSectionNamesW(
    _Out_writes_to_opt_(nSize, return + 1) LPWSTR lpszReturnBuffer,
    _In_     DWORD nSize,
    _In_opt_ LPCWSTR lpFileName
    );
#ifdef UNICODE
#define GetPrivateProfileSectionNames  GetPrivateProfileSectionNamesW
#else
#define GetPrivateProfileSectionNames  GetPrivateProfileSectionNamesA
#endif // !UNICODE

#if defined(_M_CEE)
#undef GetPrivateProfileSectionNames
__inline
DWORD
GetPrivateProfileSectionNames(
    LPTSTR lpszReturnBuffer,
    DWORD nSize,
    LPCTSTR lpFileName
    )
{
#ifdef UNICODE
    return GetPrivateProfileSectionNamesW(
#else
    return GetPrivateProfileSectionNamesA(
#endif
        lpszReturnBuffer,
        nSize,
        lpFileName
        );
}
#endif  /* _M_CEE */

WINBASEAPI
BOOL
WINAPI
GetPrivateProfileStructA(
    _In_     LPCSTR lpszSection,
    _In_     LPCSTR lpszKey,
    _Out_writes_bytes_opt_(uSizeStruct) LPVOID   lpStruct,
    _In_     UINT     uSizeStruct,
    _In_opt_ LPCSTR szFile
    );
WINBASEAPI
BOOL
WINAPI
GetPrivateProfileStructW(
    _In_     LPCWSTR lpszSection,
    _In_     LPCWSTR lpszKey,
    _Out_writes_bytes_opt_(uSizeStruct) LPVOID   lpStruct,
    _In_     UINT     uSizeStruct,
    _In_opt_ LPCWSTR szFile
    );
#ifdef UNICODE
#define GetPrivateProfileStruct  GetPrivateProfileStructW
#else
#define GetPrivateProfileStruct  GetPrivateProfileStructA
#endif // !UNICODE

#if defined(_M_CEE)
#undef GetPrivateProfileStruct
__inline
BOOL
GetPrivateProfileStruct(
    LPCTSTR lpszSection,
    LPCTSTR lpszKey,
    LPVOID   lpStruct,
    UINT     uSizeStruct,
    LPCTSTR szFile
    )
{
#ifdef UNICODE
    return GetPrivateProfileStructW(
#else
    return GetPrivateProfileStructA(
#endif
        lpszSection,
        lpszKey,
        lpStruct,
        uSizeStruct,
        szFile
        );
}
#endif  /* _M_CEE */

WINBASEAPI
BOOL
WINAPI
WritePrivateProfileStructA(
    _In_     LPCSTR lpszSection,
    _In_     LPCSTR lpszKey,
    _In_reads_bytes_opt_(uSizeStruct) LPVOID lpStruct,
    _In_     UINT     uSizeStruct,
    _In_opt_ LPCSTR szFile
    );
WINBASEAPI
BOOL
WINAPI
WritePrivateProfileStructW(
    _In_     LPCWSTR lpszSection,
    _In_     LPCWSTR lpszKey,
    _In_reads_bytes_opt_(uSizeStruct) LPVOID lpStruct,
    _In_     UINT     uSizeStruct,
    _In_opt_ LPCWSTR szFile
    );
#ifdef UNICODE
#define WritePrivateProfileStruct  WritePrivateProfileStructW
#else
#define WritePrivateProfileStruct  WritePrivateProfileStructA
#endif // !UNICODE

WINBASEAPI
DWORD
WINAPI
GetTempPathA(
    _In_ DWORD nBufferLength,
    _Out_writes_to_opt_(nBufferLength, return + 1) LPSTR lpBuffer
    );
#ifndef UNICODE
#define GetTempPath  GetTempPathA
#endif

WINBASEAPI
UINT
WINAPI
GetTempFileNameA(
    _In_ LPCSTR lpPathName,
    _In_ LPCSTR lpPrefixString,
    _In_ UINT uUnique,
    _Out_writes_(MAX_PATH) LPSTR lpTempFileName
    );
#ifndef UNICODE
#define GetTempFileName  GetTempFileNameA
#endif

#if defined(_M_CEE)
#undef GetTempFileName
__inline
UINT
GetTempFileName(
    LPCTSTR lpPathName,
    LPCTSTR lpPrefixString,
    UINT uUnique,
    LPTSTR lpTempFileName
    )
{
#ifdef UNICODE
    return GetTempFileNameW(
#else
    return GetTempFileNameA(
#endif
        lpPathName,
        lpPrefixString,
        uUnique,
        lpTempFileName
        );
}
#endif  /* _M_CEE */

#if !defined(RC_INVOKED) // RC warns because "WINBASE_DECLARE_GET_SYSTEM_WOW64_DIRECTORY" is a bit long.
#if _WIN32_WINNT >= 0x0501 || defined(WINBASE_DECLARE_GET_SYSTEM_WOW64_DIRECTORY)

WINBASEAPI
_Success_(return != 0 && return < uSize)
UINT
WINAPI
GetSystemWow64DirectoryA(
    _Out_writes_to_opt_(uSize, return + 1) LPSTR lpBuffer,
    _In_ UINT uSize
    );
WINBASEAPI
_Success_(return != 0 && return < uSize)
UINT
WINAPI
GetSystemWow64DirectoryW(
    _Out_writes_to_opt_(uSize, return + 1) LPWSTR lpBuffer,
    _In_ UINT uSize
    );
#ifdef UNICODE
#define GetSystemWow64Directory  GetSystemWow64DirectoryW
#else
#define GetSystemWow64Directory  GetSystemWow64DirectoryA
#endif // !UNICODE

WINBASEAPI
BOOLEAN
WINAPI
Wow64EnableWow64FsRedirection (
    _In_ BOOLEAN Wow64FsEnableRedirection
    );

//
// for GetProcAddress
//
typedef UINT (WINAPI* PGET_SYSTEM_WOW64_DIRECTORY_A)(_Out_writes_to_opt_(uSize, return + 1) LPSTR lpBuffer, _In_ UINT uSize);
typedef UINT (WINAPI* PGET_SYSTEM_WOW64_DIRECTORY_W)(_Out_writes_to_opt_(uSize, return + 1) LPWSTR lpBuffer, _In_ UINT uSize);

//
// GetProcAddress only accepts GET_SYSTEM_WOW64_DIRECTORY_NAME_A_A,
// GET_SYSTEM_WOW64_DIRECTORY_NAME_W_A, GET_SYSTEM_WOW64_DIRECTORY_NAME_T_A.
// The others are if you want to use the strings in some other way.
//
#define GET_SYSTEM_WOW64_DIRECTORY_NAME_A_A      "GetSystemWow64DirectoryA"
#define GET_SYSTEM_WOW64_DIRECTORY_NAME_A_W     L"GetSystemWow64DirectoryA"
#define GET_SYSTEM_WOW64_DIRECTORY_NAME_A_T TEXT("GetSystemWow64DirectoryA")
#define GET_SYSTEM_WOW64_DIRECTORY_NAME_W_A      "GetSystemWow64DirectoryW"
#define GET_SYSTEM_WOW64_DIRECTORY_NAME_W_W     L"GetSystemWow64DirectoryW"
#define GET_SYSTEM_WOW64_DIRECTORY_NAME_W_T TEXT("GetSystemWow64DirectoryW")

#ifdef UNICODE
#define GET_SYSTEM_WOW64_DIRECTORY_NAME_T_A GET_SYSTEM_WOW64_DIRECTORY_NAME_W_A
#define GET_SYSTEM_WOW64_DIRECTORY_NAME_T_W GET_SYSTEM_WOW64_DIRECTORY_NAME_W_W
#define GET_SYSTEM_WOW64_DIRECTORY_NAME_T_T GET_SYSTEM_WOW64_DIRECTORY_NAME_W_T
#else
#define GET_SYSTEM_WOW64_DIRECTORY_NAME_T_A GET_SYSTEM_WOW64_DIRECTORY_NAME_A_A
#define GET_SYSTEM_WOW64_DIRECTORY_NAME_T_W GET_SYSTEM_WOW64_DIRECTORY_NAME_A_W
#define GET_SYSTEM_WOW64_DIRECTORY_NAME_T_T GET_SYSTEM_WOW64_DIRECTORY_NAME_A_T
#endif

#endif // _WIN32_WINNT >= 0x0501
#endif

#if defined(_M_CEE)
#undef SetCurrentDirectory
__inline
BOOL
SetCurrentDirectory(
    LPCTSTR lpPathName
    )
{
#ifdef UNICODE
    return SetCurrentDirectoryW(
#else
    return SetCurrentDirectoryA(
#endif
        lpPathName
        );
}
#endif  /* _M_CEE */

#if defined(_M_CEE)
#undef GetCurrentDirectory
__inline
DWORD
GetCurrentDirectory(
    DWORD nBufferLength,
    LPTSTR lpBuffer
    )
{
#ifdef UNICODE
    return GetCurrentDirectoryW(
#else
    return GetCurrentDirectoryA(
#endif
        nBufferLength,
        lpBuffer
        );
}
#endif  /* _M_CEE */

#if _WIN32_WINNT >= 0x0502

WINBASEAPI
BOOL
WINAPI
SetDllDirectoryA(
    _In_opt_ LPCSTR lpPathName
    );
WINBASEAPI
BOOL
WINAPI
SetDllDirectoryW(
    _In_opt_ LPCWSTR lpPathName
    );
#ifdef UNICODE
#define SetDllDirectory  SetDllDirectoryW
#else
#define SetDllDirectory  SetDllDirectoryA
#endif // !UNICODE

WINBASEAPI
_Success_(return != 0 && return < nBufferLength)
DWORD
WINAPI
GetDllDirectoryA(
    _In_ DWORD nBufferLength,
    _Out_writes_to_opt_(nBufferLength, return + 1) LPSTR lpBuffer
    );
WINBASEAPI
_Success_(return != 0 && return < nBufferLength)
DWORD
WINAPI
GetDllDirectoryW(
    _In_ DWORD nBufferLength,
    _Out_writes_to_opt_(nBufferLength, return + 1) LPWSTR lpBuffer
    );
#ifdef UNICODE
#define GetDllDirectory  GetDllDirectoryW
#else
#define GetDllDirectory  GetDllDirectoryA
#endif // !UNICODE

#endif // _WIN32_WINNT >= 0x0502

#define BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE 0x1
#define BASE_SEARCH_PATH_DISABLE_SAFE_SEARCHMODE 0x10000
#define BASE_SEARCH_PATH_PERMANENT 0x8000
#define BASE_SEARCH_PATH_INVALID_FLAGS ~0x18001

WINBASEAPI
BOOL
WINAPI
SetSearchPathMode (
    _In_ DWORD Flags
    );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#pragma region Application Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)

#if defined(_M_CEE)
#undef CreateDirectory
__inline
BOOL
CreateDirectory(
    LPCTSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
#ifdef UNICODE
    return CreateDirectoryW(
#else
    return CreateDirectoryA(
#endif
        lpPathName,
        lpSecurityAttributes
        );
}
#endif  /* _M_CEE */

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

WINBASEAPI
BOOL
WINAPI
CreateDirectoryExA(
    _In_     LPCSTR lpTemplateDirectory,
    _In_     LPCSTR lpNewDirectory,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );
WINBASEAPI
BOOL
WINAPI
CreateDirectoryExW(
    _In_     LPCWSTR lpTemplateDirectory,
    _In_     LPCWSTR lpNewDirectory,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );
#ifdef UNICODE
#define CreateDirectoryEx  CreateDirectoryExW
#else
#define CreateDirectoryEx  CreateDirectoryExA
#endif // !UNICODE

#if _WIN32_WINNT >= 0x0600

WINBASEAPI
BOOL
WINAPI
CreateDirectoryTransactedA(
    _In_opt_ LPCSTR lpTemplateDirectory,
    _In_     LPCSTR lpNewDirectory,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_     HANDLE hTransaction
    );
WINBASEAPI
BOOL
WINAPI
CreateDirectoryTransactedW(
    _In_opt_ LPCWSTR lpTemplateDirectory,
    _In_     LPCWSTR lpNewDirectory,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_     HANDLE hTransaction
    );
#ifdef UNICODE
#define CreateDirectoryTransacted  CreateDirectoryTransactedW
#else
#define CreateDirectoryTransacted  CreateDirectoryTransactedA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
RemoveDirectoryTransactedA(
    _In_ LPCSTR lpPathName,
    _In_     HANDLE hTransaction
    );
WINBASEAPI
BOOL
WINAPI
RemoveDirectoryTransactedW(
    _In_ LPCWSTR lpPathName,
    _In_     HANDLE hTransaction
    );
#ifdef UNICODE
#define RemoveDirectoryTransacted  RemoveDirectoryTransactedW
#else
#define RemoveDirectoryTransacted  RemoveDirectoryTransactedA
#endif // !UNICODE

WINBASEAPI
_Success_(return != 0 && return < nBufferLength)
DWORD
WINAPI
GetFullPathNameTransactedA(
    _In_            LPCSTR lpFileName,
    _In_            DWORD nBufferLength,
    _Out_writes_to_opt_(nBufferLength, return + 1) LPSTR lpBuffer,
    _Outptr_opt_ LPSTR *lpFilePart,
    _In_            HANDLE hTransaction
    );
WINBASEAPI
_Success_(return != 0 && return < nBufferLength)
DWORD
WINAPI
GetFullPathNameTransactedW(
    _In_            LPCWSTR lpFileName,
    _In_            DWORD nBufferLength,
    _Out_writes_to_opt_(nBufferLength, return + 1) LPWSTR lpBuffer,
    _Outptr_opt_ LPWSTR *lpFilePart,
    _In_            HANDLE hTransaction
    );
#ifdef UNICODE
#define GetFullPathNameTransacted  GetFullPathNameTransactedW
#else
#define GetFullPathNameTransacted  GetFullPathNameTransactedA
#endif // !UNICODE

#endif // _WIN32_WINNT >= 0x0600

#define DDD_RAW_TARGET_PATH         0x00000001
#define DDD_REMOVE_DEFINITION       0x00000002
#define DDD_EXACT_MATCH_ON_REMOVE   0x00000004
#define DDD_NO_BROADCAST_SYSTEM     0x00000008
#define DDD_LUID_BROADCAST_DRIVE    0x00000010

WINBASEAPI
BOOL
WINAPI
DefineDosDeviceA(
    _In_     DWORD dwFlags,
    _In_     LPCSTR lpDeviceName,
    _In_opt_ LPCSTR lpTargetPath
    );
#ifndef UNICODE
#define DefineDosDevice  DefineDosDeviceA
#endif

WINBASEAPI
DWORD
WINAPI
QueryDosDeviceA(
    _In_opt_ LPCSTR lpDeviceName,
    _Out_writes_to_opt_(ucchMax, return) LPSTR lpTargetPath,
    _In_     DWORD ucchMax
    );
#ifndef UNICODE
#define QueryDosDevice  QueryDosDeviceA
#endif

#define EXPAND_LOCAL_DRIVES

#if _WIN32_WINNT >= 0x0600

WINBASEAPI
HANDLE
WINAPI
CreateFileTransactedA(
    _In_       LPCSTR lpFileName,
    _In_       DWORD dwDesiredAccess,
    _In_       DWORD dwShareMode,
    _In_opt_   LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_       DWORD dwCreationDisposition,
    _In_       DWORD dwFlagsAndAttributes,
    _In_opt_   HANDLE hTemplateFile,
    _In_       HANDLE hTransaction,
    _In_opt_   PUSHORT pusMiniVersion,
    _Reserved_ PVOID  lpExtendedParameter
    );
WINBASEAPI
HANDLE
WINAPI
CreateFileTransactedW(
    _In_       LPCWSTR lpFileName,
    _In_       DWORD dwDesiredAccess,
    _In_       DWORD dwShareMode,
    _In_opt_   LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_       DWORD dwCreationDisposition,
    _In_       DWORD dwFlagsAndAttributes,
    _In_opt_   HANDLE hTemplateFile,
    _In_       HANDLE hTransaction,
    _In_opt_   PUSHORT pusMiniVersion,
    _Reserved_ PVOID  lpExtendedParameter
    );
#ifdef UNICODE
#define CreateFileTransacted  CreateFileTransactedW
#else
#define CreateFileTransacted  CreateFileTransactedA
#endif // !UNICODE

#endif // _WIN32_WINNT >= 0x0600


#if _WIN32_WINNT >= 0x0502

WINBASEAPI
HANDLE
WINAPI
ReOpenFile(
    _In_ HANDLE  hOriginalFile,
    _In_ DWORD   dwDesiredAccess,
    _In_ DWORD   dwShareMode,
    _In_ DWORD   dwFlagsAndAttributes
    );

#endif // _WIN32_WINNT >= 0x0502


#if _WIN32_WINNT >= 0x0600

WINBASEAPI
BOOL
WINAPI
SetFileAttributesTransactedA(
    _In_     LPCSTR lpFileName,
    _In_     DWORD dwFileAttributes,
    _In_     HANDLE hTransaction
    );
WINBASEAPI
BOOL
WINAPI
SetFileAttributesTransactedW(
    _In_     LPCWSTR lpFileName,
    _In_     DWORD dwFileAttributes,
    _In_     HANDLE hTransaction
    );
#ifdef UNICODE
#define SetFileAttributesTransacted  SetFileAttributesTransactedW
#else
#define SetFileAttributesTransacted  SetFileAttributesTransactedA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
GetFileAttributesTransactedA(
    _In_  LPCSTR lpFileName,
    _In_  GET_FILEEX_INFO_LEVELS fInfoLevelId,
    _Out_writes_bytes_(sizeof(WIN32_FILE_ATTRIBUTE_DATA)) LPVOID lpFileInformation,
    _In_     HANDLE hTransaction
    );
WINBASEAPI
BOOL
WINAPI
GetFileAttributesTransactedW(
    _In_  LPCWSTR lpFileName,
    _In_  GET_FILEEX_INFO_LEVELS fInfoLevelId,
    _Out_writes_bytes_(sizeof(WIN32_FILE_ATTRIBUTE_DATA)) LPVOID lpFileInformation,
    _In_     HANDLE hTransaction
    );
#ifdef UNICODE
#define GetFileAttributesTransacted  GetFileAttributesTransactedW
#else
#define GetFileAttributesTransacted  GetFileAttributesTransactedA
#endif // !UNICODE

#endif // _WIN32_WINNT >= 0x0600

WINBASEAPI
DWORD
WINAPI
GetCompressedFileSizeA(
    _In_  LPCSTR lpFileName,
    _Out_opt_ LPDWORD  lpFileSizeHigh
    );
WINBASEAPI
DWORD
WINAPI
GetCompressedFileSizeW(
    _In_  LPCWSTR lpFileName,
    _Out_opt_ LPDWORD  lpFileSizeHigh
    );
#ifdef UNICODE
#define GetCompressedFileSize  GetCompressedFileSizeW
#else
#define GetCompressedFileSize  GetCompressedFileSizeA
#endif // !UNICODE

#if _WIN32_WINNT >= 0x0600

WINBASEAPI
DWORD
WINAPI
GetCompressedFileSizeTransactedA(
    _In_      LPCSTR lpFileName,
    _Out_opt_ LPDWORD  lpFileSizeHigh,
    _In_      HANDLE hTransaction
    );
WINBASEAPI
DWORD
WINAPI
GetCompressedFileSizeTransactedW(
    _In_      LPCWSTR lpFileName,
    _Out_opt_ LPDWORD  lpFileSizeHigh,
    _In_      HANDLE hTransaction
    );
#ifdef UNICODE
#define GetCompressedFileSizeTransacted  GetCompressedFileSizeTransactedW
#else
#define GetCompressedFileSizeTransacted  GetCompressedFileSizeTransactedA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
DeleteFileTransactedA(
    _In_     LPCSTR lpFileName,
    _In_     HANDLE hTransaction
    );
WINBASEAPI
BOOL
WINAPI
DeleteFileTransactedW(
    _In_     LPCWSTR lpFileName,
    _In_     HANDLE hTransaction
    );
#ifdef UNICODE
#define DeleteFileTransacted  DeleteFileTransactedW
#else
#define DeleteFileTransacted  DeleteFileTransactedA
#endif // !UNICODE

#endif // _WIN32_WINNT >= 0x0600

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#pragma region Application Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)

#if defined(_M_CEE)
#undef DeleteFile
__inline
BOOL
DeleteFile(
    LPCTSTR lpFileName
    )
{
#ifdef UNICODE
    return DeleteFileW(
#else
    return DeleteFileA(
#endif
        lpFileName
        );
}
#endif  /* _M_CEE */

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

#if _WIN32_WINNT >= 0x0501

WINBASEAPI
BOOL
WINAPI
CheckNameLegalDOS8Dot3A(
    _In_      LPCSTR lpName,
    _Out_writes_opt_(OemNameSize) LPSTR lpOemName,
    _In_      DWORD OemNameSize,
    _Out_opt_ PBOOL pbNameContainsSpaces OPTIONAL,
    _Out_     PBOOL pbNameLegal
    );
WINBASEAPI
BOOL
WINAPI
CheckNameLegalDOS8Dot3W(
    _In_      LPCWSTR lpName,
    _Out_writes_opt_(OemNameSize) LPSTR lpOemName,
    _In_      DWORD OemNameSize,
    _Out_opt_ PBOOL pbNameContainsSpaces OPTIONAL,
    _Out_     PBOOL pbNameLegal
    );
#ifdef UNICODE
#define CheckNameLegalDOS8Dot3  CheckNameLegalDOS8Dot3W
#else
#define CheckNameLegalDOS8Dot3  CheckNameLegalDOS8Dot3A
#endif // !UNICODE

#endif // (_WIN32_WINNT >= 0x0501)

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#if(_WIN32_WINNT >= 0x0400)

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

#if _WIN32_WINNT >= 0x0600

WINBASEAPI
HANDLE
WINAPI
FindFirstFileTransactedA(
    _In_       LPCSTR lpFileName,
    _In_       FINDEX_INFO_LEVELS fInfoLevelId,
    _Out_writes_bytes_(sizeof(WIN32_FIND_DATAA)) LPVOID lpFindFileData,
    _In_       FINDEX_SEARCH_OPS fSearchOp,
    _Reserved_ LPVOID lpSearchFilter,
    _In_       DWORD dwAdditionalFlags,
    _In_       HANDLE hTransaction
    );
WINBASEAPI
HANDLE
WINAPI
FindFirstFileTransactedW(
    _In_       LPCWSTR lpFileName,
    _In_       FINDEX_INFO_LEVELS fInfoLevelId,
    _Out_writes_bytes_(sizeof(WIN32_FIND_DATAW)) LPVOID lpFindFileData,
    _In_       FINDEX_SEARCH_OPS fSearchOp,
    _Reserved_ LPVOID lpSearchFilter,
    _In_       DWORD dwAdditionalFlags,
    _In_       HANDLE hTransaction
    );
#ifdef UNICODE
#define FindFirstFileTransacted  FindFirstFileTransactedW
#else
#define FindFirstFileTransacted  FindFirstFileTransactedA
#endif // !UNICODE

#endif

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#endif /* _WIN32_WINNT >= 0x0400 */

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)


WINBASEAPI
BOOL
WINAPI
CopyFileA(
    _In_ LPCSTR lpExistingFileName,
    _In_ LPCSTR lpNewFileName,
    _In_ BOOL bFailIfExists
    );
WINBASEAPI
BOOL
WINAPI
CopyFileW(
    _In_ LPCWSTR lpExistingFileName,
    _In_ LPCWSTR lpNewFileName,
    _In_ BOOL bFailIfExists
    );
#ifdef UNICODE
#define CopyFile  CopyFileW
#else
#define CopyFile  CopyFileA
#endif // !UNICODE

#if defined(_M_CEE)
#undef CopyFile
__inline
BOOL
CopyFile(
    LPCTSTR lpExistingFileName,
    LPCTSTR lpNewFileName,
    BOOL bFailIfExists
    )
{
#ifdef UNICODE
    return CopyFileW(
#else
    return CopyFileA(
#endif
        lpExistingFileName,
        lpNewFileName,
        bFailIfExists
        );
}
#endif  /* _M_CEE */

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#if(_WIN32_WINNT >= 0x0400)

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

typedef
DWORD
(WINAPI *LPPROGRESS_ROUTINE)(
    _In_     LARGE_INTEGER TotalFileSize,
    _In_     LARGE_INTEGER TotalBytesTransferred,
    _In_     LARGE_INTEGER StreamSize,
    _In_     LARGE_INTEGER StreamBytesTransferred,
    _In_     DWORD dwStreamNumber,
    _In_     DWORD dwCallbackReason,
    _In_     HANDLE hSourceFile,
    _In_     HANDLE hDestinationFile,
    _In_opt_ LPVOID lpData
    );

WINBASEAPI
BOOL
WINAPI
CopyFileExA(
    _In_        LPCSTR lpExistingFileName,
    _In_        LPCSTR lpNewFileName,
    _In_opt_    LPPROGRESS_ROUTINE lpProgressRoutine,
    _In_opt_    LPVOID lpData,
    _When_(pbCancel != NULL, _Pre_satisfies_(*pbCancel == FALSE))
    _Inout_opt_ LPBOOL pbCancel,
    _In_        DWORD dwCopyFlags
    );
WINBASEAPI
BOOL
WINAPI
CopyFileExW(
    _In_        LPCWSTR lpExistingFileName,
    _In_        LPCWSTR lpNewFileName,
    _In_opt_    LPPROGRESS_ROUTINE lpProgressRoutine,
    _In_opt_    LPVOID lpData,
    _When_(pbCancel != NULL, _Pre_satisfies_(*pbCancel == FALSE))
    _Inout_opt_ LPBOOL pbCancel,
    _In_        DWORD dwCopyFlags
    );
#ifdef UNICODE
#define CopyFileEx  CopyFileExW
#else
#define CopyFileEx  CopyFileExA
#endif // !UNICODE

#if _WIN32_WINNT >= 0x0600

WINBASEAPI
BOOL
WINAPI
CopyFileTransactedA(
    _In_     LPCSTR lpExistingFileName,
    _In_     LPCSTR lpNewFileName,
    _In_opt_ LPPROGRESS_ROUTINE lpProgressRoutine,
    _In_opt_ LPVOID lpData,
    _In_opt_ LPBOOL pbCancel,
    _In_     DWORD dwCopyFlags,
    _In_     HANDLE hTransaction
    );
WINBASEAPI
BOOL
WINAPI
CopyFileTransactedW(
    _In_     LPCWSTR lpExistingFileName,
    _In_     LPCWSTR lpNewFileName,
    _In_opt_ LPPROGRESS_ROUTINE lpProgressRoutine,
    _In_opt_ LPVOID lpData,
    _In_opt_ LPBOOL pbCancel,
    _In_     DWORD dwCopyFlags,
    _In_     HANDLE hTransaction
    );
#ifdef UNICODE
#define CopyFileTransacted  CopyFileTransactedW
#else
#define CopyFileTransacted  CopyFileTransactedA
#endif // !UNICODE

#endif // _WIN32_WINNT >= 0x0600

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#pragma region Application Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)

//
// 


#if _WIN32_WINNT >= 0x0601

typedef enum _COPYFILE2_MESSAGE_TYPE {
     COPYFILE2_CALLBACK_NONE = 0,
     COPYFILE2_CALLBACK_CHUNK_STARTED,
     COPYFILE2_CALLBACK_CHUNK_FINISHED,
     COPYFILE2_CALLBACK_STREAM_STARTED,
     COPYFILE2_CALLBACK_STREAM_FINISHED,
     COPYFILE2_CALLBACK_POLL_CONTINUE,
     COPYFILE2_CALLBACK_ERROR,
     COPYFILE2_CALLBACK_MAX,
} COPYFILE2_MESSAGE_TYPE;

typedef enum _COPYFILE2_MESSAGE_ACTION {
    COPYFILE2_PROGRESS_CONTINUE = 0,
    COPYFILE2_PROGRESS_CANCEL,
    COPYFILE2_PROGRESS_STOP,
    COPYFILE2_PROGRESS_QUIET,
    COPYFILE2_PROGRESS_PAUSE,
} COPYFILE2_MESSAGE_ACTION;

typedef enum _COPYFILE2_COPY_PHASE {
    COPYFILE2_PHASE_NONE = 0,
    COPYFILE2_PHASE_PREPARE_SOURCE,
    COPYFILE2_PHASE_PREPARE_DEST,
    COPYFILE2_PHASE_READ_SOURCE,
    COPYFILE2_PHASE_WRITE_DESTINATION,
    COPYFILE2_PHASE_SERVER_COPY,
    COPYFILE2_PHASE_NAMEGRAFT_COPY,
    // ... etc phases.
    COPYFILE2_PHASE_MAX,
} COPYFILE2_COPY_PHASE;

#define COPYFILE2_MESSAGE_COPY_OFFLOAD     (0x00000001L)

typedef struct COPYFILE2_MESSAGE {

    COPYFILE2_MESSAGE_TYPE  Type;
    DWORD                   dwPadding;

    union {

        struct {
            DWORD           dwStreamNumber; // monotonically increasing stream number
            DWORD           dwReserved;
            HANDLE           hSourceFile; // handle to the source stream
            HANDLE           hDestinationFile; // handle to the destination stream
            ULARGE_INTEGER  uliChunkNumber; // monotonically increasing chunk number
            ULARGE_INTEGER  uliChunkSize;  // size of the copied chunk
            ULARGE_INTEGER  uliStreamSize; // size of the current stream
            ULARGE_INTEGER  uliTotalFileSize; // size of all streams for this file
        } ChunkStarted;

        struct {
            DWORD           dwStreamNumber; // monotonically increasing stream number
            DWORD           dwFlags;
            HANDLE           hSourceFile; // handle to the source stream
            HANDLE           hDestinationFile; // handle to the destination stream
            ULARGE_INTEGER  uliChunkNumber; // monotonically increasing chunk number
            ULARGE_INTEGER  uliChunkSize;  // size of the copied chunk
            ULARGE_INTEGER  uliStreamSize; // size of the current stream
            ULARGE_INTEGER  uliStreamBytesTransferred; // bytes copied for this stream so far
            ULARGE_INTEGER  uliTotalFileSize; // size of all streams for this file
            ULARGE_INTEGER  uliTotalBytesTransferred; // total bytes copied so far
        } ChunkFinished;

        struct {
            DWORD           dwStreamNumber;
            DWORD           dwReserved;
            HANDLE           hSourceFile; // handle to the source stream
            HANDLE           hDestinationFile; // handle to the destination stream
            ULARGE_INTEGER  uliStreamSize; // size of this stream
            ULARGE_INTEGER  uliTotalFileSize; // total size of all streams for this file
        } StreamStarted;

        struct {
            DWORD           dwStreamNumber;
            DWORD           dwReserved;
            HANDLE           hSourceFile; // handle to the source stream
            HANDLE           hDestinationFile; // handle to the destination stream
            ULARGE_INTEGER  uliStreamSize;
            ULARGE_INTEGER  uliStreamBytesTransferred;
            ULARGE_INTEGER  uliTotalFileSize;
            ULARGE_INTEGER  uliTotalBytesTransferred;
        } StreamFinished;

        struct {
            DWORD           dwReserved;
        } PollContinue;

        struct {
            COPYFILE2_COPY_PHASE    CopyPhase;
            DWORD                   dwStreamNumber;
            HRESULT                 hrFailure;
            DWORD                   dwReserved;
            ULARGE_INTEGER          uliChunkNumber;
            ULARGE_INTEGER          uliStreamSize;
            ULARGE_INTEGER          uliStreamBytesTransferred;
            ULARGE_INTEGER          uliTotalFileSize;
            ULARGE_INTEGER          uliTotalBytesTransferred;
        } Error;

    } Info;

} COPYFILE2_MESSAGE;

typedef
COPYFILE2_MESSAGE_ACTION (CALLBACK *PCOPYFILE2_PROGRESS_ROUTINE)(
  _In_      const COPYFILE2_MESSAGE     *pMessage,
  _In_opt_  PVOID                       pvCallbackContext
);

typedef struct COPYFILE2_EXTENDED_PARAMETERS {
  DWORD                         dwSize;
  DWORD                         dwCopyFlags;
  BOOL                          *pfCancel;
  PCOPYFILE2_PROGRESS_ROUTINE   pProgressRoutine;
  PVOID                         pvCallbackContext;
} COPYFILE2_EXTENDED_PARAMETERS;

WINBASEAPI
HRESULT
WINAPI
CopyFile2(
  _In_      PCWSTR                          pwszExistingFileName,
  _In_      PCWSTR                          pwszNewFileName,
  _In_opt_  COPYFILE2_EXTENDED_PARAMETERS   *pExtendedParameters
);

#endif // _WIN32_WINNT >= 0x0601

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion

#endif /* _WIN32_WINNT >= 0x0400 */

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

WINBASEAPI
BOOL
WINAPI
MoveFileA(
    _In_ LPCSTR lpExistingFileName,
    _In_ LPCSTR lpNewFileName
    );
WINBASEAPI
BOOL
WINAPI
MoveFileW(
    _In_ LPCWSTR lpExistingFileName,
    _In_ LPCWSTR lpNewFileName
    );
#ifdef UNICODE
#define MoveFile  MoveFileW
#else
#define MoveFile  MoveFileA
#endif // !UNICODE

#if defined(_M_CEE)
#undef MoveFile
__inline
BOOL
MoveFile(
    LPCTSTR lpExistingFileName,
    LPCTSTR lpNewFileName
    )
{
#ifdef UNICODE
    return MoveFileW(
#else
    return MoveFileA(
#endif
        lpExistingFileName,
        lpNewFileName
        );
}
#endif  /* _M_CEE */

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#pragma region Application Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)

WINBASEAPI
BOOL
WINAPI
MoveFileExA(
    _In_     LPCSTR lpExistingFileName,
    _In_opt_ LPCSTR lpNewFileName,
    _In_     DWORD    dwFlags
    );
WINBASEAPI
BOOL
WINAPI
MoveFileExW(
    _In_     LPCWSTR lpExistingFileName,
    _In_opt_ LPCWSTR lpNewFileName,
    _In_     DWORD    dwFlags
    );
#ifdef UNICODE
#define MoveFileEx  MoveFileExW
#else
#define MoveFileEx  MoveFileExA
#endif // !UNICODE

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

#if (_WIN32_WINNT >= 0x0500)
WINBASEAPI
BOOL
WINAPI
MoveFileWithProgressA(
    _In_     LPCSTR lpExistingFileName,
    _In_opt_ LPCSTR lpNewFileName,
    _In_opt_ LPPROGRESS_ROUTINE lpProgressRoutine,
    _In_opt_ LPVOID lpData,
    _In_     DWORD dwFlags
    );
WINBASEAPI
BOOL
WINAPI
MoveFileWithProgressW(
    _In_     LPCWSTR lpExistingFileName,
    _In_opt_ LPCWSTR lpNewFileName,
    _In_opt_ LPPROGRESS_ROUTINE lpProgressRoutine,
    _In_opt_ LPVOID lpData,
    _In_     DWORD dwFlags
    );
#ifdef UNICODE
#define MoveFileWithProgress  MoveFileWithProgressW
#else
#define MoveFileWithProgress  MoveFileWithProgressA
#endif // !UNICODE
#endif // (_WIN32_WINNT >= 0x0500)

#if (_WIN32_WINNT >= 0x0600)
WINBASEAPI
BOOL
WINAPI
MoveFileTransactedA(
    _In_     LPCSTR lpExistingFileName,
    _In_opt_ LPCSTR lpNewFileName,
    _In_opt_ LPPROGRESS_ROUTINE lpProgressRoutine,
    _In_opt_ LPVOID lpData,
    _In_     DWORD dwFlags,
    _In_     HANDLE hTransaction
    );
WINBASEAPI
BOOL
WINAPI
MoveFileTransactedW(
    _In_     LPCWSTR lpExistingFileName,
    _In_opt_ LPCWSTR lpNewFileName,
    _In_opt_ LPPROGRESS_ROUTINE lpProgressRoutine,
    _In_opt_ LPVOID lpData,
    _In_     DWORD dwFlags,
    _In_     HANDLE hTransaction
    );
#ifdef UNICODE
#define MoveFileTransacted  MoveFileTransactedW
#else
#define MoveFileTransacted  MoveFileTransactedA
#endif // !UNICODE
#endif // (_WIN32_WINNT >= 0x0600)

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#pragma region Application Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)

#define MOVEFILE_REPLACE_EXISTING       0x00000001
#define MOVEFILE_COPY_ALLOWED           0x00000002
#define MOVEFILE_DELAY_UNTIL_REBOOT     0x00000004
#define MOVEFILE_WRITE_THROUGH          0x00000008
#if (_WIN32_WINNT >= 0x0500)
#define MOVEFILE_CREATE_HARDLINK        0x00000010
#define MOVEFILE_FAIL_IF_NOT_TRACKABLE  0x00000020
#endif // (_WIN32_WINNT >= 0x0500)

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)


#if (_WIN32_WINNT >= 0x0500)

WINBASEAPI
BOOL
WINAPI
ReplaceFileA(
    _In_       LPCSTR lpReplacedFileName,
    _In_       LPCSTR lpReplacementFileName,
    _In_opt_   LPCSTR lpBackupFileName,
    _In_       DWORD    dwReplaceFlags,
    _Reserved_ LPVOID   lpExclude,
    _Reserved_ LPVOID  lpReserved
    );
WINBASEAPI
BOOL
WINAPI
ReplaceFileW(
    _In_       LPCWSTR lpReplacedFileName,
    _In_       LPCWSTR lpReplacementFileName,
    _In_opt_   LPCWSTR lpBackupFileName,
    _In_       DWORD    dwReplaceFlags,
    _Reserved_ LPVOID   lpExclude,
    _Reserved_ LPVOID  lpReserved
    );
#ifdef UNICODE
#define ReplaceFile  ReplaceFileW
#else
#define ReplaceFile  ReplaceFileA
#endif // !UNICODE
#endif // (_WIN32_WINNT >= 0x0500)


#if (_WIN32_WINNT >= 0x0500)
//
// API call to create hard links.
//

WINBASEAPI
BOOL
WINAPI
CreateHardLinkA(
    _In_       LPCSTR lpFileName,
    _In_       LPCSTR lpExistingFileName,
    _Reserved_ LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );
WINBASEAPI
BOOL
WINAPI
CreateHardLinkW(
    _In_       LPCWSTR lpFileName,
    _In_       LPCWSTR lpExistingFileName,
    _Reserved_ LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );
#ifdef UNICODE
#define CreateHardLink  CreateHardLinkW
#else
#define CreateHardLink  CreateHardLinkA
#endif // !UNICODE

#endif // (_WIN32_WINNT >= 0x0500)

#if (_WIN32_WINNT >= 0x0600)
//
// API call to create hard links.
//

WINBASEAPI
BOOL
WINAPI
CreateHardLinkTransactedA(
    _In_       LPCSTR lpFileName,
    _In_       LPCSTR lpExistingFileName,
    _Reserved_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_       HANDLE hTransaction
    );
WINBASEAPI
BOOL
WINAPI
CreateHardLinkTransactedW(
    _In_       LPCWSTR lpFileName,
    _In_       LPCWSTR lpExistingFileName,
    _Reserved_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_       HANDLE hTransaction
    );
#ifdef UNICODE
#define CreateHardLinkTransacted  CreateHardLinkTransactedW
#else
#define CreateHardLinkTransacted  CreateHardLinkTransactedA
#endif // !UNICODE

#endif // (_WIN32_WINNT >= 0x0600)


#if (_WIN32_WINNT >= 0x0501)

//
// API call to enumerate for streams within a file
//

typedef enum _STREAM_INFO_LEVELS {

    FindStreamInfoStandard,
    FindStreamInfoMaxInfoLevel

} STREAM_INFO_LEVELS;

typedef struct _WIN32_FIND_STREAM_DATA {

    LARGE_INTEGER StreamSize;
    WCHAR cStreamName[ MAX_PATH + 36 ];

} WIN32_FIND_STREAM_DATA, *PWIN32_FIND_STREAM_DATA;

WINBASEAPI
HANDLE
WINAPI
FindFirstStreamW(
    _In_       LPCWSTR lpFileName,
    _In_       STREAM_INFO_LEVELS InfoLevel,
    _Out_writes_bytes_(sizeof(WIN32_FIND_STREAM_DATA)) LPVOID lpFindStreamData,
    _Reserved_ DWORD dwFlags
    );

#if _WIN32_WINNT >= 0x0600
WINBASEAPI
HANDLE
WINAPI
FindFirstStreamTransactedW (
    _In_       LPCWSTR lpFileName,
    _In_       STREAM_INFO_LEVELS InfoLevel,
    _Out_writes_bytes_(sizeof(WIN32_FIND_STREAM_DATA)) LPVOID lpFindStreamData,
    _Reserved_ DWORD dwFlags,
    _In_       HANDLE hTransaction
    );
#endif // _WIN32_WINNT >= 0x600

WINBASEAPI
BOOL
APIENTRY
FindNextStreamW(
    _In_  HANDLE hFindStream,
    _Out_writes_bytes_(sizeof(WIN32_FIND_STREAM_DATA)) LPVOID lpFindStreamData
    );
#endif // (_WIN32_WINNT >= 0x0501)

#if _WIN32_WINNT >= 0x0600

WINBASEAPI
HANDLE
WINAPI
FindFirstFileNameW (
    _In_    LPCWSTR lpFileName,
    _In_    DWORD dwFlags,
    _Inout_ LPDWORD StringLength,
    _Out_writes_(*StringLength) PWSTR LinkName
    );

WINBASEAPI
BOOL
APIENTRY
FindNextFileNameW (
    _In_    HANDLE hFindStream,
    _Inout_ LPDWORD StringLength,
    _Out_writes_(*StringLength) PWSTR LinkName
    );

WINBASEAPI
HANDLE
WINAPI
FindFirstFileNameTransactedW (
    _In_     LPCWSTR lpFileName,
    _In_     DWORD dwFlags,
    _Inout_  LPDWORD StringLength,
    _Out_writes_(*StringLength) PWSTR LinkName,
    _In_opt_ HANDLE hTransaction
    );

#endif

WINBASEAPI
HANDLE
WINAPI
CreateNamedPipeA(
    _In_     LPCSTR lpName,
    _In_     DWORD dwOpenMode,
    _In_     DWORD dwPipeMode,
    _In_     DWORD nMaxInstances,
    _In_     DWORD nOutBufferSize,
    _In_     DWORD nInBufferSize,
    _In_     DWORD nDefaultTimeOut,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );
#ifndef UNICODE
#define CreateNamedPipe  CreateNamedPipeA
#endif

WINBASEAPI
BOOL
WINAPI
GetNamedPipeHandleStateA(
    _In_      HANDLE hNamedPipe,
    _Out_opt_ LPDWORD lpState,
    _Out_opt_ LPDWORD lpCurInstances,
    _Out_opt_ LPDWORD lpMaxCollectionCount,
    _Out_opt_ LPDWORD lpCollectDataTimeout,
    _Out_writes_opt_(nMaxUserNameSize) LPSTR lpUserName,
    _In_      DWORD nMaxUserNameSize
    );
WINBASEAPI
BOOL
WINAPI
GetNamedPipeHandleStateW(
    _In_      HANDLE hNamedPipe,
    _Out_opt_ LPDWORD lpState,
    _Out_opt_ LPDWORD lpCurInstances,
    _Out_opt_ LPDWORD lpMaxCollectionCount,
    _Out_opt_ LPDWORD lpCollectDataTimeout,
    _Out_writes_opt_(nMaxUserNameSize) LPWSTR lpUserName,
    _In_      DWORD nMaxUserNameSize
    );
#ifdef UNICODE
#define GetNamedPipeHandleState  GetNamedPipeHandleStateW
#else
#define GetNamedPipeHandleState  GetNamedPipeHandleStateA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
CallNamedPipeA(
    _In_  LPCSTR lpNamedPipeName,
    _In_reads_bytes_opt_(nInBufferSize) LPVOID lpInBuffer,
    _In_  DWORD nInBufferSize,
    _Out_writes_bytes_to_opt_(nOutBufferSize, *lpBytesRead) LPVOID lpOutBuffer,
    _In_  DWORD nOutBufferSize,
    _Out_ LPDWORD lpBytesRead,
    _In_  DWORD nTimeOut
    );
WINBASEAPI
BOOL
WINAPI
CallNamedPipeW(
    _In_  LPCWSTR lpNamedPipeName,
    _In_reads_bytes_opt_(nInBufferSize) LPVOID lpInBuffer,
    _In_  DWORD nInBufferSize,
    _Out_writes_bytes_to_opt_(nOutBufferSize, *lpBytesRead) LPVOID lpOutBuffer,
    _In_  DWORD nOutBufferSize,
    _Out_ LPDWORD lpBytesRead,
    _In_  DWORD nTimeOut
    );
#ifdef UNICODE
#define CallNamedPipe  CallNamedPipeW
#else
#define CallNamedPipe  CallNamedPipeA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
WaitNamedPipeA(
    _In_ LPCSTR lpNamedPipeName,
    _In_ DWORD nTimeOut
    );
#ifndef UNICODE
#define WaitNamedPipe  WaitNamedPipeA
#endif

#if (_WIN32_WINNT >= 0x0600)


WINBASEAPI
BOOL
WINAPI
GetNamedPipeClientComputerNameA(
    _In_ HANDLE Pipe,
    _Out_writes_bytes_(ClientComputerNameLength)  LPSTR ClientComputerName,
    _In_ ULONG ClientComputerNameLength
    );

#ifndef UNICODE
#define GetNamedPipeClientComputerName  GetNamedPipeClientComputerNameA
#endif

WINBASEAPI
BOOL
WINAPI
GetNamedPipeClientProcessId(
    _In_ HANDLE Pipe,
    _Out_ PULONG ClientProcessId
    );

WINBASEAPI
BOOL
WINAPI
GetNamedPipeClientSessionId(
    _In_ HANDLE Pipe,
    _Out_ PULONG ClientSessionId
    );

WINBASEAPI
BOOL
WINAPI
GetNamedPipeServerProcessId(
    _In_ HANDLE Pipe,
    _Out_ PULONG ServerProcessId
    );

WINBASEAPI
BOOL
WINAPI
GetNamedPipeServerSessionId(
    _In_ HANDLE Pipe,
    _Out_ PULONG ServerSessionId
    );

#endif // (_WIN32_WINNT >= 0x0600)

WINBASEAPI
BOOL
WINAPI
SetVolumeLabelA(
    _In_opt_ LPCSTR lpRootPathName,
    _In_opt_ LPCSTR lpVolumeName
    );
WINBASEAPI
BOOL
WINAPI
SetVolumeLabelW(
    _In_opt_ LPCWSTR lpRootPathName,
    _In_opt_ LPCWSTR lpVolumeName
    );
#ifdef UNICODE
#define SetVolumeLabel  SetVolumeLabelW
#else
#define SetVolumeLabel  SetVolumeLabelA
#endif // !UNICODE

WINBASEAPI
VOID
WINAPI
SetFileApisToOEM( VOID );

WINBASEAPI
VOID
WINAPI
SetFileApisToANSI( VOID );

WINBASEAPI
BOOL
WINAPI
AreFileApisANSI( VOID );

WINBASEAPI
BOOL
WINAPI
GetVolumeInformationA(
    _In_opt_  LPCSTR lpRootPathName,
    _Out_writes_opt_(nVolumeNameSize) LPSTR lpVolumeNameBuffer,
    _In_      DWORD nVolumeNameSize,
    _Out_opt_ LPDWORD lpVolumeSerialNumber,
    _Out_opt_ LPDWORD lpMaximumComponentLength,
    _Out_opt_ LPDWORD lpFileSystemFlags,
    _Out_writes_opt_(nFileSystemNameSize) LPSTR lpFileSystemNameBuffer,
    _In_      DWORD nFileSystemNameSize
    );

#ifndef UNICODE
#define GetVolumeInformation  GetVolumeInformationA
#endif

#if (_WIN32_WINNT >= 0x0600)

WINBASEAPI
BOOL
WINAPI
SetFileBandwidthReservation(
    _In_  HANDLE  hFile,
    _In_  DWORD   nPeriodMilliseconds,
    _In_  DWORD   nBytesPerPeriod,
    _In_  BOOL    bDiscardable,
    _Out_ LPDWORD lpTransferSize,
    _Out_ LPDWORD lpNumOutstandingRequests
    );

WINBASEAPI
BOOL
WINAPI
GetFileBandwidthReservation(
    _In_  HANDLE  hFile,
    _Out_ LPDWORD lpPeriodMilliseconds,
    _Out_ LPDWORD lpBytesPerPeriod,
    _Out_ LPBOOL  pDiscardable,
    _Out_ LPDWORD lpTransferSize,
    _Out_ LPDWORD lpNumOutstandingRequests
    );

#endif // (_WIN32_WINNT >= 0x0600)

//
// Event logging APIs
//

WINADVAPI
BOOL
WINAPI
ClearEventLogA (
    _In_     HANDLE hEventLog,
    _In_opt_ LPCSTR lpBackupFileName
    );
WINADVAPI
BOOL
WINAPI
ClearEventLogW (
    _In_     HANDLE hEventLog,
    _In_opt_ LPCWSTR lpBackupFileName
    );
#ifdef UNICODE
#define ClearEventLog  ClearEventLogW
#else
#define ClearEventLog  ClearEventLogA
#endif // !UNICODE

WINADVAPI
BOOL
WINAPI
BackupEventLogA (
    _In_ HANDLE hEventLog,
    _In_ LPCSTR lpBackupFileName
    );
WINADVAPI
BOOL
WINAPI
BackupEventLogW (
    _In_ HANDLE hEventLog,
    _In_ LPCWSTR lpBackupFileName
    );
#ifdef UNICODE
#define BackupEventLog  BackupEventLogW
#else
#define BackupEventLog  BackupEventLogA
#endif // !UNICODE

WINADVAPI
BOOL
WINAPI
CloseEventLog (
    _In_ HANDLE hEventLog
    );

WINADVAPI
BOOL
WINAPI
DeregisterEventSource (
    _In_ HANDLE hEventLog
    );

WINADVAPI
BOOL
WINAPI
NotifyChangeEventLog(
    _In_ HANDLE  hEventLog,
    _In_ HANDLE  hEvent
    );

WINADVAPI
BOOL
WINAPI
GetNumberOfEventLogRecords (
    _In_  HANDLE hEventLog,
    _Out_ PDWORD NumberOfRecords
    );

WINADVAPI
BOOL
WINAPI
GetOldestEventLogRecord (
    _In_  HANDLE hEventLog,
    _Out_ PDWORD OldestRecord
    );

WINADVAPI
HANDLE
WINAPI
OpenEventLogA (
    _In_opt_ LPCSTR lpUNCServerName,
    _In_     LPCSTR lpSourceName
    );
WINADVAPI
HANDLE
WINAPI
OpenEventLogW (
    _In_opt_ LPCWSTR lpUNCServerName,
    _In_     LPCWSTR lpSourceName
    );
#ifdef UNICODE
#define OpenEventLog  OpenEventLogW
#else
#define OpenEventLog  OpenEventLogA
#endif // !UNICODE

WINADVAPI
HANDLE
WINAPI
RegisterEventSourceA (
    _In_opt_ LPCSTR lpUNCServerName,
    _In_     LPCSTR lpSourceName
    );
WINADVAPI
HANDLE
WINAPI
RegisterEventSourceW (
    _In_opt_ LPCWSTR lpUNCServerName,
    _In_     LPCWSTR lpSourceName
    );
#ifdef UNICODE
#define RegisterEventSource  RegisterEventSourceW
#else
#define RegisterEventSource  RegisterEventSourceA
#endif // !UNICODE

WINADVAPI
HANDLE
WINAPI
OpenBackupEventLogA (
    _In_opt_ LPCSTR lpUNCServerName,
    _In_     LPCSTR lpFileName
    );
WINADVAPI
HANDLE
WINAPI
OpenBackupEventLogW (
    _In_opt_ LPCWSTR lpUNCServerName,
    _In_     LPCWSTR lpFileName
    );
#ifdef UNICODE
#define OpenBackupEventLog  OpenBackupEventLogW
#else
#define OpenBackupEventLog  OpenBackupEventLogA
#endif // !UNICODE

WINADVAPI
BOOL
WINAPI
ReadEventLogA (
    _In_  HANDLE     hEventLog,
    _In_  DWORD      dwReadFlags,
    _In_  DWORD      dwRecordOffset,
    _Out_writes_bytes_to_(nNumberOfBytesToRead, *pnBytesRead) LPVOID     lpBuffer,
    _In_  DWORD      nNumberOfBytesToRead,
    _Out_ DWORD      *pnBytesRead,
    _Out_ DWORD      *pnMinNumberOfBytesNeeded
    );
WINADVAPI
BOOL
WINAPI
ReadEventLogW (
    _In_  HANDLE     hEventLog,
    _In_  DWORD      dwReadFlags,
    _In_  DWORD      dwRecordOffset,
    _Out_writes_bytes_to_(nNumberOfBytesToRead, *pnBytesRead) LPVOID     lpBuffer,
    _In_  DWORD      nNumberOfBytesToRead,
    _Out_ DWORD      *pnBytesRead,
    _Out_ DWORD      *pnMinNumberOfBytesNeeded
    );
#ifdef UNICODE
#define ReadEventLog  ReadEventLogW
#else
#define ReadEventLog  ReadEventLogA
#endif // !UNICODE

WINADVAPI
BOOL
WINAPI
ReportEventA (
    _In_     HANDLE     hEventLog,
    _In_     WORD       wType,
    _In_     WORD       wCategory,
    _In_     DWORD      dwEventID,
    _In_opt_ PSID       lpUserSid,
    _In_     WORD       wNumStrings,
    _In_     DWORD      dwDataSize,
    _In_reads_opt_(wNumStrings) LPCSTR *lpStrings,
    _In_reads_bytes_opt_(dwDataSize) LPVOID lpRawData
    );
WINADVAPI
BOOL
WINAPI
ReportEventW (
    _In_     HANDLE     hEventLog,
    _In_     WORD       wType,
    _In_     WORD       wCategory,
    _In_     DWORD      dwEventID,
    _In_opt_ PSID       lpUserSid,
    _In_     WORD       wNumStrings,
    _In_     DWORD      dwDataSize,
    _In_reads_opt_(wNumStrings) LPCWSTR *lpStrings,
    _In_reads_bytes_opt_(dwDataSize) LPVOID lpRawData
    );
#ifdef UNICODE
#define ReportEvent  ReportEventW
#else
#define ReportEvent  ReportEventA
#endif // !UNICODE


#define EVENTLOG_FULL_INFO      0

typedef struct _EVENTLOG_FULL_INFORMATION
{
    DWORD    dwFull;
}
EVENTLOG_FULL_INFORMATION, *LPEVENTLOG_FULL_INFORMATION;

WINADVAPI
BOOL
WINAPI
GetEventLogInformation (
    _In_  HANDLE     hEventLog,
    _In_  DWORD      dwInfoLevel,
    _Out_writes_bytes_to_(cbBufSize, *pcbBytesNeeded) LPVOID lpBuffer,
    _In_  DWORD      cbBufSize,
    _Out_ LPDWORD    pcbBytesNeeded
    );

#if (_WIN32_WINNT >= 0x0602)

//
// Operation prefetch API.
//

#define OPERATION_API_VERSION                   1
typedef ULONG OPERATION_ID;

//
// OperationStart() parameters.
//

typedef struct _OPERATION_START_PARAMETERS {
    ULONG Version;
    OPERATION_ID OperationId;
    ULONG Flags;
} OPERATION_START_PARAMETERS, *POPERATION_START_PARAMETERS;

#define OPERATION_START_TRACE_CURRENT_THREAD    0x1

//
// OperationEnd() parameters.
//

typedef struct _OPERATION_END_PARAMETERS {
    ULONG Version;
    OPERATION_ID OperationId;
    ULONG Flags;
} OPERATION_END_PARAMETERS, *POPERATION_END_PARAMETERS;

#define OPERATION_END_DISCARD                   0x1

WINADVAPI
BOOL
WINAPI
OperationStart (
    _In_ OPERATION_START_PARAMETERS* OperationStartParams
    );

WINADVAPI
BOOL
WINAPI
OperationEnd (
    _In_ OPERATION_END_PARAMETERS* OperationEndParams
    );

#endif // _WIN32_WINNT >= 0x0602

//
//
// Security APIs
//


WINADVAPI
BOOL
WINAPI
AccessCheckAndAuditAlarmA (
    _In_     LPCSTR SubsystemName,
    _In_opt_ LPVOID HandleId,
    _In_     LPSTR ObjectTypeName,
    _In_opt_ LPSTR ObjectName,
    _In_     PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_     DWORD DesiredAccess,
    _In_     PGENERIC_MAPPING GenericMapping,
    _In_     BOOL ObjectCreation,
    _Out_    LPDWORD GrantedAccess,
    _Out_    LPBOOL AccessStatus,
    _Out_    LPBOOL pfGenerateOnClose
    );
#ifndef UNICODE
#define AccessCheckAndAuditAlarm  AccessCheckAndAuditAlarmA
#endif

#if(_WIN32_WINNT >= 0x0500)

WINADVAPI
BOOL
WINAPI
AccessCheckByTypeAndAuditAlarmA (
    _In_     LPCSTR SubsystemName,
    _In_     LPVOID HandleId,
    _In_     LPCSTR ObjectTypeName,
    _In_opt_ LPCSTR ObjectName,
    _In_     PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PSID PrincipalSelfSid,
    _In_     DWORD DesiredAccess,
    _In_     AUDIT_EVENT_TYPE AuditType,
    _In_     DWORD Flags,
    _Inout_updates_opt_(ObjectTypeListLength) POBJECT_TYPE_LIST ObjectTypeList,
    _In_     DWORD ObjectTypeListLength,
    _In_     PGENERIC_MAPPING GenericMapping,
    _In_     BOOL ObjectCreation,
    _Out_    LPDWORD GrantedAccess,
    _Out_    LPBOOL AccessStatus,
    _Out_    LPBOOL pfGenerateOnClose
    );
#ifndef UNICODE
#define AccessCheckByTypeAndAuditAlarm  AccessCheckByTypeAndAuditAlarmA
#endif

WINADVAPI
BOOL
WINAPI
AccessCheckByTypeResultListAndAuditAlarmA (
    _In_     LPCSTR SubsystemName,
    _In_     LPVOID HandleId,
    _In_     LPCSTR ObjectTypeName,
    _In_opt_ LPCSTR ObjectName,
    _In_     PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PSID PrincipalSelfSid,
    _In_     DWORD DesiredAccess,
    _In_     AUDIT_EVENT_TYPE AuditType,
    _In_     DWORD Flags,
    _Inout_updates_opt_(ObjectTypeListLength) POBJECT_TYPE_LIST ObjectTypeList,
    _In_     DWORD ObjectTypeListLength,
    _In_     PGENERIC_MAPPING GenericMapping,
    _In_     BOOL ObjectCreation,
    _Out_writes_(ObjectTypeListLength)       LPDWORD GrantedAccess,
    _Out_writes_(ObjectTypeListLength)       LPDWORD AccessStatusList,
    _Out_    LPBOOL pfGenerateOnClose
    );
#ifndef UNICODE
#define AccessCheckByTypeResultListAndAuditAlarm  AccessCheckByTypeResultListAndAuditAlarmA
#endif

WINADVAPI
BOOL
WINAPI
AccessCheckByTypeResultListAndAuditAlarmByHandleA (
    _In_     LPCSTR SubsystemName,
    _In_     LPVOID HandleId,
    _In_     HANDLE ClientToken,
    _In_     LPCSTR ObjectTypeName,
    _In_opt_ LPCSTR ObjectName,
    _In_     PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PSID PrincipalSelfSid,
    _In_     DWORD DesiredAccess,
    _In_     AUDIT_EVENT_TYPE AuditType,
    _In_     DWORD Flags,
    _Inout_updates_opt_(ObjectTypeListLength) POBJECT_TYPE_LIST ObjectTypeList,
    _In_     DWORD ObjectTypeListLength,
    _In_     PGENERIC_MAPPING GenericMapping,
    _In_     BOOL ObjectCreation,
    _Out_writes_(ObjectTypeListLength)       LPDWORD GrantedAccess,
    _Out_writes_(ObjectTypeListLength)       LPDWORD AccessStatusList,
    _Out_    LPBOOL pfGenerateOnClose
    );
#ifndef UNICODE
#define AccessCheckByTypeResultListAndAuditAlarmByHandle  AccessCheckByTypeResultListAndAuditAlarmByHandleA
#endif
#endif //(_WIN32_WINNT >= 0x0500)

WINADVAPI
BOOL
WINAPI
ObjectOpenAuditAlarmA (
    _In_     LPCSTR SubsystemName,
    _In_     LPVOID HandleId,
    _In_     LPSTR ObjectTypeName,
    _In_opt_ LPSTR ObjectName,
    _In_     PSECURITY_DESCRIPTOR pSecurityDescriptor,
    _In_     HANDLE ClientToken,
    _In_     DWORD DesiredAccess,
    _In_     DWORD GrantedAccess,
    _In_opt_ PPRIVILEGE_SET Privileges,
    _In_     BOOL ObjectCreation,
    _In_     BOOL AccessGranted,
    _Out_    LPBOOL GenerateOnClose
    );
#ifndef UNICODE
#define ObjectOpenAuditAlarm  ObjectOpenAuditAlarmA
#endif

WINADVAPI
BOOL
WINAPI
ObjectPrivilegeAuditAlarmA (
    _In_ LPCSTR SubsystemName,
    _In_ LPVOID HandleId,
    _In_ HANDLE ClientToken,
    _In_ DWORD DesiredAccess,
    _In_ PPRIVILEGE_SET Privileges,
    _In_ BOOL AccessGranted
    );
#ifndef UNICODE
#define ObjectPrivilegeAuditAlarm  ObjectPrivilegeAuditAlarmA
#endif

WINADVAPI
BOOL
WINAPI
ObjectCloseAuditAlarmA (
    _In_ LPCSTR SubsystemName,
    _In_ LPVOID HandleId,
    _In_ BOOL GenerateOnClose
    );
#ifndef UNICODE
#define ObjectCloseAuditAlarm  ObjectCloseAuditAlarmA
#endif

WINADVAPI
BOOL
WINAPI
ObjectDeleteAuditAlarmA (
    _In_ LPCSTR SubsystemName,
    _In_ LPVOID HandleId,
    _In_ BOOL GenerateOnClose
    );
#ifndef UNICODE
#define ObjectDeleteAuditAlarm  ObjectDeleteAuditAlarmA
#endif

WINADVAPI
BOOL
WINAPI
PrivilegedServiceAuditAlarmA (
    _In_ LPCSTR SubsystemName,
    _In_ LPCSTR ServiceName,
    _In_ HANDLE ClientToken,
    _In_ PPRIVILEGE_SET Privileges,
    _In_ BOOL AccessGranted
    );
#ifndef UNICODE
#define PrivilegedServiceAuditAlarm  PrivilegedServiceAuditAlarmA
#endif

#if(_WIN32_WINNT >= 0x0601)
WINADVAPI
BOOL
WINAPI
AddConditionalAce (
    _Inout_ PACL pAcl,
    _In_    DWORD dwAceRevision,
    _In_    DWORD AceFlags,
    _In_    UCHAR AceType,
    _In_    DWORD AccessMask,
    _In_    PSID pSid,
    _In_    PWCHAR ConditionStr,
    _Out_ DWORD *ReturnLength
    );
#endif /* _WIN32_WINNT >=  0x0601 */

WINADVAPI
BOOL
WINAPI
SetFileSecurityA (
    _In_ LPCSTR lpFileName,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR pSecurityDescriptor
    );
#ifndef UNICODE
#define SetFileSecurity  SetFileSecurityA
#endif

WINADVAPI
BOOL
WINAPI
GetFileSecurityA (
    _In_  LPCSTR lpFileName,
    _In_  SECURITY_INFORMATION RequestedInformation,
    _Out_writes_bytes_to_opt_(nLength, *lpnLengthNeeded) PSECURITY_DESCRIPTOR pSecurityDescriptor,
    _In_  DWORD nLength,
    _Out_ LPDWORD lpnLengthNeeded
    );
#ifndef UNICODE
#define GetFileSecurity  GetFileSecurityA
#endif

#if(_WIN32_WINNT >= 0x0400)
WINBASEAPI
BOOL
WINAPI
ReadDirectoryChangesW(
    _In_        HANDLE hDirectory,
    _Out_writes_bytes_to_(nBufferLength, *lpBytesReturned) LPVOID lpBuffer,
    _In_        DWORD nBufferLength,
    _In_        BOOL bWatchSubtree,
    _In_        DWORD dwNotifyFilter,
    _Out_opt_   LPDWORD lpBytesReturned,
    _Inout_opt_ LPOVERLAPPED lpOverlapped,
    _In_opt_    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    );
#endif /* _WIN32_WINNT >= 0x0400 */

#if _WIN32_WINNT >= 0x0600

WINBASEAPI
_Ret_maybenull_ __out_data_source(FILE)
LPVOID
WINAPI
MapViewOfFileExNuma(
    _In_     HANDLE hFileMappingObject,
    _In_     DWORD dwDesiredAccess,
    _In_     DWORD dwFileOffsetHigh,
    _In_     DWORD dwFileOffsetLow,
    _In_     SIZE_T dwNumberOfBytesToMap,
    _In_opt_ LPVOID lpBaseAddress,
    _In_     DWORD nndPreferred
    );

#endif // _WIN32_WINNT >= 0x0600

WINBASEAPI
BOOL
WINAPI
IsBadReadPtr(
    _In_opt_ CONST VOID *lp,
    _In_     UINT_PTR ucb
    );

WINBASEAPI
BOOL
WINAPI
IsBadWritePtr(
    _In_opt_ LPVOID lp,
    _In_     UINT_PTR ucb
    );

WINBASEAPI
BOOL
WINAPI
IsBadHugeReadPtr(
    _In_opt_ CONST VOID *lp,
    _In_     UINT_PTR ucb
    );

WINBASEAPI
BOOL
WINAPI
IsBadHugeWritePtr(
    _In_opt_ LPVOID lp,
    _In_     UINT_PTR ucb
    );

WINBASEAPI
BOOL
WINAPI
IsBadCodePtr(
    _In_opt_ FARPROC lpfn
    );

WINBASEAPI
BOOL
WINAPI
IsBadStringPtrA(
    _In_opt_ LPCSTR lpsz,
    _In_     UINT_PTR ucchMax
    );
WINBASEAPI
BOOL
WINAPI
IsBadStringPtrW(
    _In_opt_ LPCWSTR lpsz,
    _In_     UINT_PTR ucchMax
    );
#ifdef UNICODE
#define IsBadStringPtr  IsBadStringPtrW
#else
#define IsBadStringPtr  IsBadStringPtrA
#endif // !UNICODE

WINADVAPI
_Success_(return != FALSE) BOOL
WINAPI
LookupAccountSidA(
    _In_opt_ LPCSTR lpSystemName,
    _In_ PSID Sid,
    _Out_writes_to_opt_(*cchName, *cchName + 1) LPSTR Name,
    _Inout_  LPDWORD cchName,
    _Out_writes_to_opt_(*cchReferencedDomainName, *cchReferencedDomainName + 1) LPSTR ReferencedDomainName,
    _Inout_ LPDWORD cchReferencedDomainName,
    _Out_ PSID_NAME_USE peUse
    );
WINADVAPI
_Success_(return != FALSE) BOOL
WINAPI
LookupAccountSidW(
    _In_opt_ LPCWSTR lpSystemName,
    _In_ PSID Sid,
    _Out_writes_to_opt_(*cchName, *cchName + 1) LPWSTR Name,
    _Inout_  LPDWORD cchName,
    _Out_writes_to_opt_(*cchReferencedDomainName, *cchReferencedDomainName + 1) LPWSTR ReferencedDomainName,
    _Inout_ LPDWORD cchReferencedDomainName,
    _Out_ PSID_NAME_USE peUse
    );
#ifdef UNICODE
#define LookupAccountSid  LookupAccountSidW
#else
#define LookupAccountSid  LookupAccountSidA
#endif // !UNICODE

WINADVAPI
_Success_(return != FALSE) BOOL
WINAPI
LookupAccountNameA(
    _In_opt_ LPCSTR lpSystemName,
    _In_     LPCSTR lpAccountName,
    _Out_writes_bytes_to_opt_(*cbSid, *cbSid) PSID Sid,
    _Inout_  LPDWORD cbSid,
    _Out_writes_to_opt_(*cchReferencedDomainName, *cchReferencedDomainName + 1) LPSTR ReferencedDomainName,
    _Inout_  LPDWORD cchReferencedDomainName,
    _Out_    PSID_NAME_USE peUse
    );
WINADVAPI
_Success_(return != FALSE) BOOL
WINAPI
LookupAccountNameW(
    _In_opt_ LPCWSTR lpSystemName,
    _In_     LPCWSTR lpAccountName,
    _Out_writes_bytes_to_opt_(*cbSid, *cbSid) PSID Sid,
    _Inout_  LPDWORD cbSid,
    _Out_writes_to_opt_(*cchReferencedDomainName, *cchReferencedDomainName + 1) LPWSTR ReferencedDomainName,
    _Inout_  LPDWORD cchReferencedDomainName,
    _Out_    PSID_NAME_USE peUse
    );
#ifdef UNICODE
#define LookupAccountName  LookupAccountNameW
#else
#define LookupAccountName  LookupAccountNameA
#endif // !UNICODE

#if _WIN32_WINNT >= 0x0601

WINADVAPI
_Success_(return != FALSE) BOOL
WINAPI
LookupAccountNameLocalA(
    _In_     LPCSTR lpAccountName,
    _Out_writes_bytes_to_opt_(*cbSid, *cbSid) PSID Sid,
    _Inout_  LPDWORD cbSid,
    _Out_writes_to_opt_(*cchReferencedDomainName, *cchReferencedDomainName + 1) LPSTR ReferencedDomainName,
    _Inout_  LPDWORD cchReferencedDomainName,
    _Out_    PSID_NAME_USE peUse
    );
WINADVAPI
_Success_(return != FALSE) BOOL
WINAPI
LookupAccountNameLocalW(
    _In_     LPCWSTR lpAccountName,
    _Out_writes_bytes_to_opt_(*cbSid, *cbSid) PSID Sid,
    _Inout_  LPDWORD cbSid,
    _Out_writes_to_opt_(*cchReferencedDomainName, *cchReferencedDomainName + 1) LPWSTR ReferencedDomainName,
    _Inout_  LPDWORD cchReferencedDomainName,
    _Out_    PSID_NAME_USE peUse
    );
#ifdef UNICODE
#define LookupAccountNameLocal  LookupAccountNameLocalW
#else
#define LookupAccountNameLocal  LookupAccountNameLocalA
#endif // !UNICODE

WINADVAPI
_Success_(return != FALSE) BOOL
WINAPI
LookupAccountSidLocalA(
    _In_ PSID Sid,
    _Out_writes_to_opt_(*cchName, *cchName + 1) LPSTR Name,
    _Inout_  LPDWORD cchName,
    _Out_writes_to_opt_(*cchReferencedDomainName, *cchReferencedDomainName + 1) LPSTR ReferencedDomainName,
    _Inout_ LPDWORD cchReferencedDomainName,
    _Out_ PSID_NAME_USE peUse
    );
WINADVAPI
_Success_(return != FALSE) BOOL
WINAPI
LookupAccountSidLocalW(
    _In_ PSID Sid,
    _Out_writes_to_opt_(*cchName, *cchName + 1) LPWSTR Name,
    _Inout_  LPDWORD cchName,
    _Out_writes_to_opt_(*cchReferencedDomainName, *cchReferencedDomainName + 1) LPWSTR ReferencedDomainName,
    _Inout_ LPDWORD cchReferencedDomainName,
    _Out_ PSID_NAME_USE peUse
    );
#ifdef UNICODE
#define LookupAccountSidLocal  LookupAccountSidLocalW
#else
#define LookupAccountSidLocal  LookupAccountSidLocalA
#endif // !UNICODE

#else // _WIN32_WINNT >= 0x0601

#define LookupAccountNameLocalA(n, s, cs, d, cd, u) \
    LookupAccountNameA(NULL, n, s, cs, d, cd, u)
#define LookupAccountNameLocalW(n, s, cs, d, cd, u) \
    LookupAccountNameW(NULL, n, s, cs, d, cd, u)
#ifdef UNICODE
#define LookupAccountNameLocal  LookupAccountNameLocalW
#else
#define LookupAccountNameLocal  LookupAccountNameLocalA
#endif // !UNICODE

#define LookupAccountSidLocalA(s, n, cn, d, cd, u)  \
    LookupAccountSidA(NULL, s, n, cn, d, cd, u)
#define LookupAccountSidLocalW(s, n, cn, d, cd, u)  \
    LookupAccountSidW(NULL, s, n, cn, d, cd, u)
#ifdef UNICODE
#define LookupAccountSidLocal  LookupAccountSidLocalW
#else
#define LookupAccountSidLocal  LookupAccountSidLocalA
#endif // !UNICODE

#endif // _WIN32_WINNT >= 0x0601

WINADVAPI
BOOL
WINAPI
LookupPrivilegeValueA(
    _In_opt_ LPCSTR lpSystemName,
    _In_     LPCSTR lpName,
    _Out_    PLUID   lpLuid
    );
WINADVAPI
BOOL
WINAPI
LookupPrivilegeValueW(
    _In_opt_ LPCWSTR lpSystemName,
    _In_     LPCWSTR lpName,
    _Out_    PLUID   lpLuid
    );
#ifdef UNICODE
#define LookupPrivilegeValue  LookupPrivilegeValueW
#else
#define LookupPrivilegeValue  LookupPrivilegeValueA
#endif // !UNICODE

WINADVAPI
_Success_(return != FALSE) BOOL
WINAPI
LookupPrivilegeNameA(
    _In_opt_ LPCSTR lpSystemName,
    _In_     PLUID   lpLuid,
    _Out_writes_to_opt_(*cchName, *cchName + 1) LPSTR lpName,
    _Inout_  LPDWORD cchName
    );
WINADVAPI
_Success_(return != FALSE) BOOL
WINAPI
LookupPrivilegeNameW(
    _In_opt_ LPCWSTR lpSystemName,
    _In_     PLUID   lpLuid,
    _Out_writes_to_opt_(*cchName, *cchName + 1) LPWSTR lpName,
    _Inout_  LPDWORD cchName
    );
#ifdef UNICODE
#define LookupPrivilegeName  LookupPrivilegeNameW
#else
#define LookupPrivilegeName  LookupPrivilegeNameA
#endif // !UNICODE

WINADVAPI
_Success_(return != FALSE) BOOL
WINAPI
LookupPrivilegeDisplayNameA(
    _In_opt_ LPCSTR lpSystemName,
    _In_     LPCSTR lpName,
    _Out_writes_to_opt_(*cchDisplayName, *cchDisplayName + 1) LPSTR lpDisplayName,
    _Inout_  LPDWORD cchDisplayName,
    _Out_    LPDWORD lpLanguageId
    );
WINADVAPI
_Success_(return != FALSE) BOOL
WINAPI
LookupPrivilegeDisplayNameW(
    _In_opt_ LPCWSTR lpSystemName,
    _In_     LPCWSTR lpName,
    _Out_writes_to_opt_(*cchDisplayName, *cchDisplayName + 1) LPWSTR lpDisplayName,
    _Inout_  LPDWORD cchDisplayName,
    _Out_    LPDWORD lpLanguageId
    );
#ifdef UNICODE
#define LookupPrivilegeDisplayName  LookupPrivilegeDisplayNameW
#else
#define LookupPrivilegeDisplayName  LookupPrivilegeDisplayNameA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
BuildCommDCBA(
    _In_  LPCSTR lpDef,
    _Out_ LPDCB lpDCB
    );
WINBASEAPI
BOOL
WINAPI
BuildCommDCBW(
    _In_  LPCWSTR lpDef,
    _Out_ LPDCB lpDCB
    );
#ifdef UNICODE
#define BuildCommDCB  BuildCommDCBW
#else
#define BuildCommDCB  BuildCommDCBA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
BuildCommDCBAndTimeoutsA(
    _In_  LPCSTR lpDef,
    _Out_ LPDCB lpDCB,
    _Out_ LPCOMMTIMEOUTS lpCommTimeouts
    );
WINBASEAPI
BOOL
WINAPI
BuildCommDCBAndTimeoutsW(
    _In_  LPCWSTR lpDef,
    _Out_ LPDCB lpDCB,
    _Out_ LPCOMMTIMEOUTS lpCommTimeouts
    );
#ifdef UNICODE
#define BuildCommDCBAndTimeouts  BuildCommDCBAndTimeoutsW
#else
#define BuildCommDCBAndTimeouts  BuildCommDCBAndTimeoutsA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
CommConfigDialogA(
    _In_     LPCSTR lpszName,
    _In_opt_ HWND hWnd,
    _Inout_  LPCOMMCONFIG lpCC
    );
WINBASEAPI
BOOL
WINAPI
CommConfigDialogW(
    _In_     LPCWSTR lpszName,
    _In_opt_ HWND hWnd,
    _Inout_  LPCOMMCONFIG lpCC
    );
#ifdef UNICODE
#define CommConfigDialog  CommConfigDialogW
#else
#define CommConfigDialog  CommConfigDialogA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
GetDefaultCommConfigA(
    _In_    LPCSTR lpszName,
    _Out_writes_bytes_to_(*lpdwSize, *lpdwSize) LPCOMMCONFIG lpCC,
    _Inout_ LPDWORD lpdwSize
    );
WINBASEAPI
BOOL
WINAPI
GetDefaultCommConfigW(
    _In_    LPCWSTR lpszName,
    _Out_writes_bytes_to_(*lpdwSize, *lpdwSize) LPCOMMCONFIG lpCC,
    _Inout_ LPDWORD lpdwSize
    );
#ifdef UNICODE
#define GetDefaultCommConfig  GetDefaultCommConfigW
#else
#define GetDefaultCommConfig  GetDefaultCommConfigA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
SetDefaultCommConfigA(
    _In_ LPCSTR lpszName,
    _In_reads_bytes_(dwSize) LPCOMMCONFIG lpCC,
    _In_ DWORD dwSize
    );
WINBASEAPI
BOOL
WINAPI
SetDefaultCommConfigW(
    _In_ LPCWSTR lpszName,
    _In_reads_bytes_(dwSize) LPCOMMCONFIG lpCC,
    _In_ DWORD dwSize
    );
#ifdef UNICODE
#define SetDefaultCommConfig  SetDefaultCommConfigW
#else
#define SetDefaultCommConfig  SetDefaultCommConfigA
#endif // !UNICODE

#ifndef _MAC
#define MAX_COMPUTERNAME_LENGTH 15
#else
#define MAX_COMPUTERNAME_LENGTH 31
#endif

WINBASEAPI
_Success_(return != 0)
BOOL
WINAPI
GetComputerNameA (
    _Out_writes_to_opt_(*nSize, *nSize + 1) LPSTR lpBuffer,
    _Inout_ LPDWORD nSize
    );
WINBASEAPI
_Success_(return != 0)
BOOL
WINAPI
GetComputerNameW (
    _Out_writes_to_opt_(*nSize, *nSize + 1) LPWSTR lpBuffer,
    _Inout_ LPDWORD nSize
    );
#ifdef UNICODE
#define GetComputerName  GetComputerNameW
#else
#define GetComputerName  GetComputerNameA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
SetComputerNameA (
    _In_ LPCSTR lpComputerName
    );
WINBASEAPI
BOOL
WINAPI
SetComputerNameW (
    _In_ LPCWSTR lpComputerName
    );
#ifdef UNICODE
#define SetComputerName  SetComputerNameW
#else
#define SetComputerName  SetComputerNameA
#endif // !UNICODE


#if (_WIN32_WINNT >= 0x0500)

WINBASEAPI
BOOL
WINAPI
SetComputerNameExA (
    _In_ COMPUTER_NAME_FORMAT NameType,
    _In_ LPCTSTR lpBuffer
    );
#ifndef UNICODE
#define SetComputerNameEx SetComputerNameExA
#endif


WINBASEAPI
_Success_(return != FALSE)
BOOL
WINAPI
DnsHostnameToComputerNameA (
    _In_    LPCSTR Hostname,
    _Out_writes_to_opt_(*nSize, *nSize + 1) LPSTR ComputerName,
    _Inout_ LPDWORD nSize
    );
WINBASEAPI
_Success_(return != FALSE)
BOOL
WINAPI
DnsHostnameToComputerNameW (
    _In_    LPCWSTR Hostname,
    _Out_writes_to_opt_(*nSize, *nSize + 1) LPWSTR ComputerName,
    _Inout_ LPDWORD nSize
    );
#ifdef UNICODE
#define DnsHostnameToComputerName  DnsHostnameToComputerNameW
#else
#define DnsHostnameToComputerName  DnsHostnameToComputerNameA
#endif // !UNICODE

#endif // _WIN32_WINNT

WINADVAPI
BOOL
WINAPI
GetUserNameA (
    _Out_writes_to_opt_(*pcbBuffer, *pcbBuffer) LPSTR lpBuffer,
    _Inout_ LPDWORD pcbBuffer
    );
WINADVAPI
BOOL
WINAPI
GetUserNameW (
    _Out_writes_to_opt_(*pcbBuffer, *pcbBuffer) LPWSTR lpBuffer,
    _Inout_ LPDWORD pcbBuffer
    );
#ifdef UNICODE
#define GetUserName  GetUserNameW
#else
#define GetUserName  GetUserNameA
#endif // !UNICODE

//
// Logon Support APIs
//

#define LOGON32_LOGON_INTERACTIVE       2
#define LOGON32_LOGON_NETWORK           3
#define LOGON32_LOGON_BATCH             4
#define LOGON32_LOGON_SERVICE           5
#define LOGON32_LOGON_UNLOCK            7
#if(_WIN32_WINNT >= 0x0500)
#define LOGON32_LOGON_NETWORK_CLEARTEXT 8
#define LOGON32_LOGON_NEW_CREDENTIALS   9
#endif // (_WIN32_WINNT >= 0x0500)

#define LOGON32_PROVIDER_DEFAULT    0
#define LOGON32_PROVIDER_WINNT35    1
#if(_WIN32_WINNT >= 0x0400)
#define LOGON32_PROVIDER_WINNT40    2
#endif /* _WIN32_WINNT >= 0x0400 */
#if(_WIN32_WINNT >= 0x0500)
#define LOGON32_PROVIDER_WINNT50    3
#endif // (_WIN32_WINNT >= 0x0500)
#if(_WIN32_WINNT >= 0x0600)
#define LOGON32_PROVIDER_VIRTUAL    4
#endif // (_WIN32_WINNT >= 0x0600)



WINADVAPI
BOOL
WINAPI
LogonUserA (
    _In_        LPCSTR lpszUsername,
    _In_opt_    LPCSTR lpszDomain,
    _In_opt_    LPCSTR lpszPassword,
    _In_        DWORD dwLogonType,
    _In_        DWORD dwLogonProvider,
    _Outptr_ PHANDLE phToken
    );
WINADVAPI
BOOL
WINAPI
LogonUserW (
    _In_        LPCWSTR lpszUsername,
    _In_opt_    LPCWSTR lpszDomain,
    _In_opt_    LPCWSTR lpszPassword,
    _In_        DWORD dwLogonType,
    _In_        DWORD dwLogonProvider,
    _Outptr_ PHANDLE phToken
    );
#ifdef UNICODE
#define LogonUser  LogonUserW
#else
#define LogonUser  LogonUserA
#endif // !UNICODE

WINADVAPI
BOOL
WINAPI
LogonUserExA (
    _In_            LPCSTR lpszUsername,
    _In_opt_        LPCSTR lpszDomain,
    _In_opt_        LPCSTR lpszPassword,
    _In_            DWORD dwLogonType,
    _In_            DWORD dwLogonProvider,
    _Outptr_opt_ PHANDLE phToken,
    _Outptr_opt_ PSID  *ppLogonSid,
    _Outptr_opt_result_bytebuffer_all_(*pdwProfileLength) PVOID *ppProfileBuffer,
    _Out_opt_       LPDWORD pdwProfileLength,
    _Out_opt_       PQUOTA_LIMITS pQuotaLimits
    );
WINADVAPI
BOOL
WINAPI
LogonUserExW (
    _In_            LPCWSTR lpszUsername,
    _In_opt_        LPCWSTR lpszDomain,
    _In_opt_        LPCWSTR lpszPassword,
    _In_            DWORD dwLogonType,
    _In_            DWORD dwLogonProvider,
    _Outptr_opt_ PHANDLE phToken,
    _Outptr_opt_ PSID  *ppLogonSid,
    _Outptr_opt_result_bytebuffer_all_(*pdwProfileLength) PVOID *ppProfileBuffer,
    _Out_opt_       LPDWORD pdwProfileLength,
    _Out_opt_       PQUOTA_LIMITS pQuotaLimits
    );
#ifdef UNICODE
#define LogonUserEx  LogonUserExW
#else
#define LogonUserEx  LogonUserExA
#endif // !UNICODE


#if(_WIN32_WINNT >= 0x0600)


#endif // (_WIN32_WINNT >= 0x0600)

WINADVAPI
BOOL
WINAPI
CreateProcessAsUserA (
    _In_opt_    HANDLE hToken,
    _In_opt_    LPCSTR lpApplicationName,
    _Inout_opt_ LPSTR lpCommandLine,
    _In_opt_    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    _In_opt_    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    _In_        BOOL bInheritHandles,
    _In_        DWORD dwCreationFlags,
    _In_opt_    LPVOID lpEnvironment,
    _In_opt_    LPCSTR lpCurrentDirectory,
    _In_        LPSTARTUPINFOA lpStartupInfo,
    _Out_       LPPROCESS_INFORMATION lpProcessInformation
    );
#ifndef UNICODE
#define CreateProcessAsUser  CreateProcessAsUserA
#endif


#if(_WIN32_WINNT >= 0x0500)

//
// LogonFlags
//
#define LOGON_WITH_PROFILE              0x00000001
#define LOGON_NETCREDENTIALS_ONLY       0x00000002
#define LOGON_ZERO_PASSWORD_BUFFER      0x80000000

WINADVAPI
_Must_inspect_result_ BOOL
WINAPI
CreateProcessWithLogonW(
    _In_        LPCWSTR lpUsername,
    _In_opt_    LPCWSTR lpDomain,
    _In_        LPCWSTR lpPassword,
    _In_        DWORD dwLogonFlags,
    _In_opt_    LPCWSTR lpApplicationName,
    _Inout_opt_ LPWSTR lpCommandLine,
    _In_        DWORD dwCreationFlags,
    _In_opt_    LPVOID lpEnvironment,
    _In_opt_    LPCWSTR lpCurrentDirectory,
    _In_        LPSTARTUPINFOW lpStartupInfo,
    _Out_       LPPROCESS_INFORMATION lpProcessInformation
      );

WINADVAPI
_Must_inspect_result_ BOOL
WINAPI
CreateProcessWithTokenW(
    _In_        HANDLE hToken,
    _In_        DWORD dwLogonFlags,
    _In_opt_    LPCWSTR lpApplicationName,
    _Inout_opt_ LPWSTR lpCommandLine,
    _In_        DWORD dwCreationFlags,
    _In_opt_    LPVOID lpEnvironment,
    _In_opt_    LPCWSTR lpCurrentDirectory,
    _In_        LPSTARTUPINFOW lpStartupInfo,
    _Out_       LPPROCESS_INFORMATION lpProcessInformation
      );

#endif // (_WIN32_WINNT >= 0x0500)

WINADVAPI
BOOL
WINAPI
IsTokenUntrusted(
    _In_ HANDLE TokenHandle
    );

//
// Thread pool API's
//

#if (_WIN32_WINNT >= 0x0500)

WINBASEAPI
BOOL
WINAPI
RegisterWaitForSingleObject(
    _Outptr_ PHANDLE phNewWaitObject,
    _In_        HANDLE hObject,
    _In_        WAITORTIMERCALLBACK Callback,
    _In_opt_    PVOID Context,
    _In_        ULONG dwMilliseconds,
    _In_        ULONG dwFlags
    );

WINBASEAPI
_Must_inspect_result_
BOOL
WINAPI
UnregisterWait(
    _In_ HANDLE WaitHandle
    );

WINBASEAPI
BOOL
WINAPI
BindIoCompletionCallback (
    _In_ HANDLE FileHandle,
    _In_ LPOVERLAPPED_COMPLETION_ROUTINE Function,
    _In_ ULONG Flags
    );

WINBASEAPI
HANDLE
WINAPI
SetTimerQueueTimer(
    _In_opt_ HANDLE TimerQueue,
    _In_     WAITORTIMERCALLBACK Callback,
    _In_opt_ PVOID Parameter,
    _In_     DWORD DueTime,
    _In_     DWORD Period,
    _In_     BOOL PreferIo
    );

WINBASEAPI
_Must_inspect_result_
BOOL
WINAPI
CancelTimerQueueTimer(
    _In_opt_ HANDLE TimerQueue,
    _In_     HANDLE Timer
    );

WINBASEAPI
_Must_inspect_result_
BOOL
WINAPI
DeleteTimerQueue(
    _In_ HANDLE TimerQueue
    );

#if (_WIN32_WINNT >= 0x0600)

#if !defined(MIDL_PASS)

FORCEINLINE
VOID
InitializeThreadpoolEnvironment(
    _Out_ PTP_CALLBACK_ENVIRON pcbe
    )
{
    TpInitializeCallbackEnviron(pcbe);
}

FORCEINLINE
VOID
SetThreadpoolCallbackPool(
    _Inout_ PTP_CALLBACK_ENVIRON pcbe,
    _In_    PTP_POOL             ptpp
    )
{
    TpSetCallbackThreadpool(pcbe, ptpp);
}

FORCEINLINE
VOID
SetThreadpoolCallbackCleanupGroup(
    _Inout_  PTP_CALLBACK_ENVIRON              pcbe,
    _In_     PTP_CLEANUP_GROUP                 ptpcg,
    _In_opt_ PTP_CLEANUP_GROUP_CANCEL_CALLBACK pfng
    )
{
    TpSetCallbackCleanupGroup(pcbe, ptpcg, pfng);
}

FORCEINLINE
VOID
SetThreadpoolCallbackRunsLong(
    _Inout_ PTP_CALLBACK_ENVIRON pcbe
    )
{
    TpSetCallbackLongFunction(pcbe);
}

FORCEINLINE
VOID
SetThreadpoolCallbackLibrary(
    _Inout_ PTP_CALLBACK_ENVIRON pcbe,
    _In_    PVOID                mod
    )
{
    TpSetCallback----WithDll(pcbe, mod);
}

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)

FORCEINLINE
VOID
SetThreadpoolCallbackPriority(
    _Inout_ PTP_CALLBACK_ENVIRON pcbe,
    _In_    TP_CALLBACK_PRIORITY Priority
    )
{
    TpSetCallbackPriority(pcbe, Priority);
}

#endif

FORCEINLINE
VOID
SetThreadpoolCallbackPersistent(
    _Inout_ PTP_CALLBACK_ENVIRON pcbe
    )
{
    TpSetCallbackPersistent(pcbe);
}

FORCEINLINE
VOID
DestroyThreadpoolEnvironment(
    _Inout_ PTP_CALLBACK_ENVIRON pcbe
    )
{
    TpDestroyCallbackEnviron(pcbe);
}

#endif // !defined(MIDL_PASS)

//
//  Private Namespaces support
//

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
CreatePrivateNamespaceA(
    _In_opt_ LPSECURITY_ATTRIBUTES lpPrivateNamespaceAttributes,
    _In_     LPVOID lpBoundaryDescriptor,
    _In_     LPCSTR lpAliasPrefix
    );

#ifndef UNICODE
#define CreatePrivateNamespace CreatePrivateNamespaceA
#else
#define CreatePrivateNamespace CreatePrivateNamespaceW
#endif

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
OpenPrivateNamespaceA(
    _In_     LPVOID lpBoundaryDescriptor,
    _In_     LPCSTR lpAliasPrefix
    );

#ifndef UNICODE
#define OpenPrivateNamespace OpenPrivateNamespaceA
#else
#define OpenPrivateNamespace OpenPrivateNamespaceW
#endif


//
//  Boundary descriptors support
//

WINBASEAPI
_Ret_maybenull_
HANDLE
APIENTRY
CreateBoundaryDescriptorA(
    _In_ LPCSTR Name,
    _In_ ULONG Flags
    );

#ifndef UNICODE
#define CreateBoundaryDescriptor CreateBoundaryDescriptorA
#else
#define CreateBoundaryDescriptor CreateBoundaryDescriptorW
#endif


WINBASEAPI
BOOL
WINAPI
AddIntegrityLabelToBoundaryDescriptor(
    _Inout_ HANDLE * BoundaryDescriptor,
    _In_ PSID IntegrityLabel
    );


#endif // _WIN32_WINNT >= 0x0600

#endif // _WIN32_WINNT >= 0x0500


#if(_WIN32_WINNT >= 0x0400)
//
// Plug-and-Play API's
//

#define HW_PROFILE_GUIDLEN         39      // 36-characters plus NULL terminator
#define MAX_PROFILE_LEN            80

#define DOCKINFO_UNDOCKED          (0x1)
#define DOCKINFO_DOCKED            (0x2)
#define DOCKINFO_USER_SUPPLIED     (0x4)
#define DOCKINFO_USER_UNDOCKED     (DOCKINFO_USER_SUPPLIED | DOCKINFO_UNDOCKED)
#define DOCKINFO_USER_DOCKED       (DOCKINFO_USER_SUPPLIED | DOCKINFO_DOCKED)

typedef struct tagHW_PROFILE_INFOA {
    DWORD  dwDockInfo;
    CHAR   szHwProfileGuid[HW_PROFILE_GUIDLEN];
    CHAR   szHwProfileName[MAX_PROFILE_LEN];
} HW_PROFILE_INFOA, *LPHW_PROFILE_INFOA;
typedef struct tagHW_PROFILE_INFOW {
    DWORD  dwDockInfo;
    WCHAR  szHwProfileGuid[HW_PROFILE_GUIDLEN];
    WCHAR  szHwProfileName[MAX_PROFILE_LEN];
} HW_PROFILE_INFOW, *LPHW_PROFILE_INFOW;
#ifdef UNICODE
typedef HW_PROFILE_INFOW HW_PROFILE_INFO;
typedef LPHW_PROFILE_INFOW LPHW_PROFILE_INFO;
#else
typedef HW_PROFILE_INFOA HW_PROFILE_INFO;
typedef LPHW_PROFILE_INFOA LPHW_PROFILE_INFO;
#endif // UNICODE


WINADVAPI
BOOL
WINAPI
GetCurrentHwProfileA (
    _Out_ LPHW_PROFILE_INFOA  lpHwProfileInfo
    );
WINADVAPI
BOOL
WINAPI
GetCurrentHwProfileW (
    _Out_ LPHW_PROFILE_INFOW  lpHwProfileInfo
    );
#ifdef UNICODE
#define GetCurrentHwProfile  GetCurrentHwProfileW
#else
#define GetCurrentHwProfile  GetCurrentHwProfileA
#endif // !UNICODE
#endif /* _WIN32_WINNT >= 0x0400 */

WINBASEAPI
BOOL
WINAPI
VerifyVersionInfoA(
    _Inout_ LPOSVERSIONINFOEXA lpVersionInformation,
    _In_    DWORD dwTypeMask,
    _In_    DWORDLONG dwlConditionMask
    );
WINBASEAPI
BOOL
WINAPI
VerifyVersionInfoW(
    _Inout_ LPOSVERSIONINFOEXW lpVersionInformation,
    _In_    DWORD dwTypeMask,
    _In_    DWORDLONG dwlConditionMask
    );
#ifdef UNICODE
#define VerifyVersionInfo  VerifyVersionInfoW
#else
#define VerifyVersionInfo  VerifyVersionInfoA
#endif // !UNICODE


#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

// DOS and OS/2 Compatible Error Code definitions returned by the Win32 Base
// API functions.
//

#include <winerror.h>
#include <timezoneapi.h>

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

/* Abnormal termination codes */

#define TC_NORMAL       0
#define TC_HARDERR      1
#define TC_GP_TRAP      2
#define TC_SIGNAL       3

#if(WINVER >= 0x0400)
//
// Power Management APIs
//

#define AC_LINE_OFFLINE                 0x00
#define AC_LINE_ONLINE                  0x01
#define AC_LINE_BACKUP_POWER            0x02
#define AC_LINE_UNKNOWN                 0xFF

#define BATTERY_FLAG_HIGH               0x01
#define BATTERY_FLAG_LOW                0x02
#define BATTERY_FLAG_CRITICAL           0x04
#define BATTERY_FLAG_CHARGING           0x08
#define BATTERY_FLAG_NO_BATTERY         0x80
#define BATTERY_FLAG_UNKNOWN            0xFF

#define BATTERY_PERCENTAGE_UNKNOWN      0xFF

#define BATTERY_LIFE_UNKNOWN        0xFFFFFFFF

typedef struct _SYSTEM_POWER_STATUS {
    BYTE ACLineStatus;
    BYTE BatteryFlag;
    BYTE BatteryLifePercent;
    BYTE Reserved1;
    DWORD BatteryLifeTime;
    DWORD BatteryFullLifeTime;
}   SYSTEM_POWER_STATUS, *LPSYSTEM_POWER_STATUS;

WINBASEAPI
BOOL
WINAPI
GetSystemPowerStatus(
    _Out_ LPSYSTEM_POWER_STATUS lpSystemPowerStatus
    );

WINBASEAPI
BOOL
WINAPI
SetSystemPowerState(
    _In_ BOOL fSuspend,
    _In_ BOOL fForce
    );

#endif /* WINVER >= 0x0400 */

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)

_Function_class_(BAD_MEMORY_CALLBACK_ROUTINE)
typedef
VOID
WINAPI
BAD_MEMORY_CALLBACK_ROUTINE(
    VOID
    );

typedef BAD_MEMORY_CALLBACK_ROUTINE *PBAD_MEMORY_CALLBACK_ROUTINE;

WINBASEAPI
PVOID
WINAPI
RegisterBadMemoryNotification(
    _In_ PBAD_MEMORY_CALLBACK_ROUTINE Callback
    );

WINBASEAPI
BOOL
WINAPI
UnregisterBadMemoryNotification(
    _In_ PVOID RegistrationHandle
    );

#define MEHC_PATROL_SCRUBBER_PRESENT  0x1

WINBASEAPI
BOOL
WINAPI
GetMemoryErrorHandlingCapabilities(
    _Out_ PULONG Capabilities
    );

#endif

#if (_WIN32_WINNT >= 0x0500)
//
// Very Large Memory API Subset
//

WINBASEAPI
BOOL
WINAPI
AllocateUserPhysicalPages(
    _In_    HANDLE hProcess,
    _Inout_ PULONG_PTR NumberOfPages,
    _Out_writes_to_(*NumberOfPages, *NumberOfPages) PULONG_PTR PageArray
    );

#if _WIN32_WINNT >= 0x0600

WINBASEAPI
BOOL
WINAPI
AllocateUserPhysicalPagesNuma(
    _In_    HANDLE hProcess,
    _Inout_ PULONG_PTR NumberOfPages,
    _Out_writes_to_(*NumberOfPages, *NumberOfPages) PULONG_PTR PageArray,
    _In_    DWORD nndPreferred
    );

#endif // _WIN32_WINNT >= 0x0600

WINBASEAPI
BOOL
WINAPI
FreeUserPhysicalPages(
    _In_    HANDLE hProcess,
    _Inout_ PULONG_PTR NumberOfPages,
    _In_reads_(*NumberOfPages) PULONG_PTR PageArray
    );

WINBASEAPI
BOOL
WINAPI
MapUserPhysicalPages(
    _In_ PVOID VirtualAddress,
    _In_ ULONG_PTR NumberOfPages,
    _In_reads_opt_(NumberOfPages) PULONG_PTR PageArray
    );

WINBASEAPI
BOOL
WINAPI
MapUserPhysicalPagesScatter(
    _In_reads_(NumberOfPages) PVOID *VirtualAddresses,
    _In_ ULONG_PTR NumberOfPages,
    _In_reads_opt_(NumberOfPages) PULONG_PTR PageArray
    );

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
CreateJobObjectA(
    _In_opt_ LPSECURITY_ATTRIBUTES lpJobAttributes,
    _In_opt_ LPCSTR lpName
    );
WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
CreateJobObjectW(
    _In_opt_ LPSECURITY_ATTRIBUTES lpJobAttributes,
    _In_opt_ LPCWSTR lpName
    );
#ifdef UNICODE
#define CreateJobObject  CreateJobObjectW
#else
#define CreateJobObject  CreateJobObjectA
#endif // !UNICODE

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
OpenJobObjectA(
    _In_ DWORD dwDesiredAccess,
    _In_ BOOL bInheritHandle,
    _In_ LPCSTR lpName
    );
WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
OpenJobObjectW(
    _In_ DWORD dwDesiredAccess,
    _In_ BOOL bInheritHandle,
    _In_ LPCWSTR lpName
    );
#ifdef UNICODE
#define OpenJobObject  OpenJobObjectW
#else
#define OpenJobObject  OpenJobObjectA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
AssignProcessToJobObject(
    _In_ HANDLE hJob,
    _In_ HANDLE hProcess
    );

WINBASEAPI
BOOL
WINAPI
TerminateJobObject(
    _In_ HANDLE hJob,
    _In_ UINT uExitCode
    );

WINBASEAPI
BOOL
WINAPI
QueryInformationJobObject(
    _In_opt_  HANDLE hJob,
    _In_      JOBOBJECTINFOCLASS JobObjectInformationClass,
    _Out_writes_bytes_to_(cbJobObjectInformationLength, *lpReturnLength) LPVOID lpJobObjectInformation,
    _In_      DWORD cbJobObjectInformationLength,
    _Out_opt_ LPDWORD lpReturnLength
    );

WINBASEAPI
BOOL
WINAPI
SetInformationJobObject(
    _In_ HANDLE hJob,
    _In_ JOBOBJECTINFOCLASS JobObjectInformationClass,
    _In_reads_bytes_(cbJobObjectInformationLength) LPVOID lpJobObjectInformation,
    _In_ DWORD cbJobObjectInformationLength
    );

WINBASEAPI
BOOL
WINAPI
CreateJobSet (
    _In_ ULONG NumJob,
    _In_reads_(NumJob) PJOB_SET_ARRAY UserJobSet,
    _In_ ULONG Flags);

WINBASEAPI
HANDLE
WINAPI
FindFirstVolumeA(
    _Out_writes_(cchBufferLength) LPSTR lpszVolumeName,
    _In_ DWORD cchBufferLength
    );
#ifndef UNICODE
#define FindFirstVolume FindFirstVolumeA
#endif

WINBASEAPI
BOOL
WINAPI
FindNextVolumeA(
    _Inout_ HANDLE hFindVolume,
    _Out_writes_(cchBufferLength) LPSTR lpszVolumeName,
    _In_    DWORD cchBufferLength
    );
#ifndef UNICODE
#define FindNextVolume FindNextVolumeA
#endif

WINBASEAPI
HANDLE
WINAPI
FindFirstVolumeMountPointA(
    _In_ LPCSTR lpszRootPathName,
    _Out_writes_(cchBufferLength) LPSTR lpszVolumeMountPoint,
    _In_ DWORD cchBufferLength
    );
WINBASEAPI
HANDLE
WINAPI
FindFirstVolumeMountPointW(
    _In_ LPCWSTR lpszRootPathName,
    _Out_writes_(cchBufferLength) LPWSTR lpszVolumeMountPoint,
    _In_ DWORD cchBufferLength
    );
#ifdef UNICODE
#define FindFirstVolumeMountPoint FindFirstVolumeMountPointW
#else
#define FindFirstVolumeMountPoint FindFirstVolumeMountPointA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
FindNextVolumeMountPointA(
    _In_ HANDLE hFindVolumeMountPoint,
    _Out_writes_(cchBufferLength) LPSTR lpszVolumeMountPoint,
    _In_ DWORD cchBufferLength
    );
WINBASEAPI
BOOL
WINAPI
FindNextVolumeMountPointW(
    _In_ HANDLE hFindVolumeMountPoint,
    _Out_writes_(cchBufferLength) LPWSTR lpszVolumeMountPoint,
    _In_ DWORD cchBufferLength
    );
#ifdef UNICODE
#define FindNextVolumeMountPoint FindNextVolumeMountPointW
#else
#define FindNextVolumeMountPoint FindNextVolumeMountPointA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
FindVolumeMountPointClose(
    _In_ HANDLE hFindVolumeMountPoint
    );

WINBASEAPI
BOOL
WINAPI
SetVolumeMountPointA(
    _In_ LPCSTR lpszVolumeMountPoint,
    _In_ LPCSTR lpszVolumeName
    );
WINBASEAPI
BOOL
WINAPI
SetVolumeMountPointW(
    _In_ LPCWSTR lpszVolumeMountPoint,
    _In_ LPCWSTR lpszVolumeName
    );
#ifdef UNICODE
#define SetVolumeMountPoint  SetVolumeMountPointW
#else
#define SetVolumeMountPoint  SetVolumeMountPointA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
DeleteVolumeMountPointA(
    _In_ LPCSTR lpszVolumeMountPoint
    );
#ifndef UNICODE
#define DeleteVolumeMountPoint  DeleteVolumeMountPointA
#endif

WINBASEAPI
BOOL
WINAPI
GetVolumeNameForVolumeMountPointA(
    _In_ LPCSTR lpszVolumeMountPoint,
    _Out_writes_(cchBufferLength) LPSTR lpszVolumeName,
    _In_ DWORD cchBufferLength
    );
#ifndef UNICODE
#define GetVolumeNameForVolumeMountPoint  GetVolumeNameForVolumeMountPointA
#endif

WINBASEAPI
BOOL
WINAPI
GetVolumePathNameA(
    _In_ LPCSTR lpszFileName,
    _Out_writes_(cchBufferLength) LPSTR lpszVolumePathName,
    _In_ DWORD cchBufferLength
    );
#ifndef UNICODE
#define GetVolumePathName  GetVolumePathNameA
#endif

#endif

#if(_WIN32_WINNT >= 0x0501)

WINBASEAPI
BOOL
WINAPI
GetVolumePathNamesForVolumeNameA(
    _In_  LPCSTR lpszVolumeName,
    _Out_writes_to_opt_(cchBufferLength, *lpcchReturnLength) _Post_ _NullNull_terminated_ LPCH lpszVolumePathNames,
    _In_  DWORD cchBufferLength,
    _Out_ PDWORD lpcchReturnLength
    );

#ifndef UNICODE
#define GetVolumePathNamesForVolumeName  GetVolumePathNamesForVolumeNameA
#endif

#endif // (_WIN32_WINNT >= 0x0501)

#if (_WIN32_WINNT >= 0x0500) || (_WIN32_FUSION >= 0x0100) || ISOLATION_AWARE_ENABLED

#define ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID    (0x00000001)
#define ACTCTX_FLAG_LANGID_VALID                    (0x00000002)
#define ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID        (0x00000004)
#define ACTCTX_FLAG_RESOURCE_NAME_VALID             (0x00000008)
#define ACTCTX_FLAG_SET_PROCESS_DEFAULT             (0x00000010)
#define ACTCTX_FLAG_APPLICATION_NAME_VALID          (0x00000020)
#define ACTCTX_FLAG_SOURCE_IS_ASSEMBLYREF           (0x00000040)
#define ACTCTX_FLAG_HMODULE_VALID                   (0x00000080)

typedef struct tagACTCTXA {
    ULONG       cbSize;
    DWORD       dwFlags;
    LPCSTR      lpSource;
    USHORT      wProcessorArchitecture;
    LANGID      wLangId;
    LPCSTR      lpAssemblyDirectory;
    LPCSTR      lpResourceName;
    LPCSTR      lpApplicationName;
    HMODULE     hModule;
} ACTCTXA, *PACTCTXA;
typedef struct tagACTCTXW {
    ULONG       cbSize;
    DWORD       dwFlags;
    LPCWSTR     lpSource;
    USHORT      wProcessorArchitecture;
    LANGID      wLangId;
    LPCWSTR     lpAssemblyDirectory;
    LPCWSTR     lpResourceName;
    LPCWSTR     lpApplicationName;
    HMODULE     hModule;
} ACTCTXW, *PACTCTXW;
#ifdef UNICODE
typedef ACTCTXW ACTCTX;
typedef PACTCTXW PACTCTX;
#else
typedef ACTCTXA ACTCTX;
typedef PACTCTXA PACTCTX;
#endif // UNICODE

typedef const ACTCTXA *PCACTCTXA;
typedef const ACTCTXW *PCACTCTXW;
#ifdef UNICODE
typedef PCACTCTXW PCACTCTX;
#else
typedef PCACTCTXA PCACTCTX;
#endif // UNICODE



WINBASEAPI
HANDLE
WINAPI
CreateActCtxA(
    _In_ PCACTCTXA pActCtx
    );
WINBASEAPI
HANDLE
WINAPI
CreateActCtxW(
    _In_ PCACTCTXW pActCtx
    );
#ifdef UNICODE
#define CreateActCtx  CreateActCtxW
#else
#define CreateActCtx  CreateActCtxA
#endif // !UNICODE

WINBASEAPI
VOID
WINAPI
AddRefActCtx(
    _Inout_ HANDLE hActCtx
    );


WINBASEAPI
VOID
WINAPI
ReleaseActCtx(
    _Inout_ HANDLE hActCtx
    );

WINBASEAPI
BOOL
WINAPI
ZombifyActCtx(
    _Inout_ HANDLE hActCtx
    );


_Success_(return)
WINBASEAPI
BOOL
WINAPI
ActivateActCtx(
    _Inout_opt_ HANDLE hActCtx,
    _Out_   ULONG_PTR *lpCookie
    );


#define DEACTIVATE_ACTCTX_FLAG_FORCE_EARLY_DEACTIVATION (0x00000001)

_Success_(return)
WINBASEAPI
BOOL
WINAPI
DeactivateActCtx(
    _In_ DWORD dwFlags,
    _In_ ULONG_PTR ulCookie
    );

WINBASEAPI
BOOL
WINAPI
GetCurrentActCtx(
    _Outptr_ HANDLE *lphActCtx);


typedef struct tagACTCTX_SECTION_KEYED_DATA_2600 {
    ULONG cbSize;
    ULONG ulDataFormatVersion;
    PVOID lpData;
    ULONG ulLength;
    PVOID lpSectionGlobalData;
    ULONG ulSectionGlobalDataLength;
    PVOID lpSectionBase;
    ULONG ulSectionTotalLength;
    HANDLE hActCtx;
    ULONG ulAssemblyRosterIndex;
} ACTCTX_SECTION_KEYED_DATA_2600, *PACTCTX_SECTION_KEYED_DATA_2600;
typedef const ACTCTX_SECTION_KEYED_DATA_2600 * PCACTCTX_SECTION_KEYED_DATA_2600;

typedef struct tagACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA {
    PVOID lpInformation;
    PVOID lpSectionBase;
    ULONG ulSectionLength;
    PVOID lpSectionGlobalDataBase;
    ULONG ulSectionGlobalDataLength;
} ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA, *PACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA;
typedef const ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA *PCACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA;

typedef struct tagACTCTX_SECTION_KEYED_DATA {
    ULONG cbSize;
    ULONG ulDataFormatVersion;
    PVOID lpData;
    ULONG ulLength;
    PVOID lpSectionGlobalData;
    ULONG ulSectionGlobalDataLength;
    PVOID lpSectionBase;
    ULONG ulSectionTotalLength;
    HANDLE hActCtx;
    ULONG ulAssemblyRosterIndex;
// 2600 stops here
    ULONG ulFlags;
    ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA AssemblyMetadata;
} ACTCTX_SECTION_KEYED_DATA, *PACTCTX_SECTION_KEYED_DATA;
typedef const ACTCTX_SECTION_KEYED_DATA * PCACTCTX_SECTION_KEYED_DATA;

#define FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX (0x00000001)
#define FIND_ACTCTX_SECTION_KEY_RETURN_FLAGS   (0x00000002)
#define FIND_ACTCTX_SECTION_KEY_RETURN_ASSEMBLY_METADATA (0x00000004)



_Success_(return)
WINBASEAPI
BOOL
WINAPI
FindActCtxSectionStringA(
    _In_       DWORD dwFlags,
    _Reserved_ const GUID *lpExtensionGuid,
    _In_       ULONG ulSectionId,
    _In_       LPCSTR lpStringToFind,
    _Out_      PACTCTX_SECTION_KEYED_DATA ReturnedData
    );
_Success_(return)
WINBASEAPI
BOOL
WINAPI
FindActCtxSectionStringW(
    _In_       DWORD dwFlags,
    _Reserved_ const GUID *lpExtensionGuid,
    _In_       ULONG ulSectionId,
    _In_       LPCWSTR lpStringToFind,
    _Out_      PACTCTX_SECTION_KEYED_DATA ReturnedData
    );
#ifdef UNICODE
#define FindActCtxSectionString  FindActCtxSectionStringW
#else
#define FindActCtxSectionString  FindActCtxSectionStringA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
FindActCtxSectionGuid(
    _In_       DWORD dwFlags,
    _Reserved_ const GUID *lpExtensionGuid,
    _In_       ULONG ulSectionId,
    _In_opt_   const GUID *lpGuidToFind,
    _Out_      PACTCTX_SECTION_KEYED_DATA ReturnedData
    );


#if !defined(RC_INVOKED) /* RC complains about long symbols in #ifs */
#if !defined(ACTIVATION_CONTEXT_BASIC_INFORMATION_DEFINED)

typedef struct _ACTIVATION_CONTEXT_BASIC_INFORMATION {
    HANDLE  hActCtx;
    DWORD   dwFlags;
} ACTIVATION_CONTEXT_BASIC_INFORMATION, *PACTIVATION_CONTEXT_BASIC_INFORMATION;

typedef const struct _ACTIVATION_CONTEXT_BASIC_INFORMATION *PCACTIVATION_CONTEXT_BASIC_INFORMATION;

#define ACTIVATION_CONTEXT_BASIC_INFORMATION_DEFINED 1

#endif // !defined(ACTIVATION_CONTEXT_BASIC_INFORMATION_DEFINED)
#endif

#define QUERY_ACTCTX_FLAG_USE_ACTIVE_ACTCTX (0x00000004)
#define QUERY_ACTCTX_FLAG_ACTCTX_IS_HMODULE (0x00000008)
#define QUERY_ACTCTX_FLAG_ACTCTX_IS_ADDRESS (0x00000010)
#define QUERY_ACTCTX_FLAG_NO_ADDREF         (0x80000000)



//
// switch (ulInfoClass)
//
//  case ActivationContextBasicInformation:
//    pvSubInstance == NULL
//    pvBuffer is of type PACTIVATION_CONTEXT_BASIC_INFORMATION
//
//  case ActivationContextDetailedInformation:
//    pvSubInstance == NULL
//    pvBuffer is of type PACTIVATION_CONTEXT_DETAILED_INFORMATION
//
//  case AssemblyDetailedInformationInActivationContext:
//    pvSubInstance is of type PULONG
//      *pvSubInstance < ACTIVATION_CONTEXT_DETAILED_INFORMATION::ulAssemblyCount
//    pvBuffer is of type PACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION
//
//  case FileInformationInAssemblyOfAssemblyInActivationContext:
//    pvSubInstance is of type PACTIVATION_CONTEXT_QUERY_INDEX
//      pvSubInstance->ulAssemblyIndex < ACTIVATION_CONTEXT_DETAILED_INFORMATION::ulAssemblyCount
//      pvSubInstance->ulFileIndexInAssembly < ACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION::ulFileCount
//    pvBuffer is of type PASSEMBLY_FILE_DETAILED_INFORMATION
//
//  case RunlevelInformationInActivationContext :
//    pvSubInstance == NULL
//    pvBuffer is of type PACTIVATION_CONTEXT_RUN_LEVEL_INFORMATION
//
// String are placed after the structs.
//
_Success_(return)
WINBASEAPI
BOOL
WINAPI
QueryActCtxW(
    _In_      DWORD dwFlags,
    _In_      HANDLE hActCtx,
    _In_opt_  PVOID pvSubInstance,
    _In_      ULONG ulInfoClass,
    _Out_writes_bytes_to_opt_(cbBuffer, *pcbWrittenOrRequired) PVOID pvBuffer,
    _In_      SIZE_T cbBuffer,
    _Out_opt_ SIZE_T *pcbWrittenOrRequired
    );

typedef _Success_(return) BOOL (WINAPI * PQUERYACTCTXW_FUNC)(
    _In_      DWORD dwFlags,
    _In_      HANDLE hActCtx,
    _In_opt_  PVOID pvSubInstance,
    _In_      ULONG ulInfoClass,
    _Out_writes_bytes_to_opt_(cbBuffer, *pcbWrittenOrRequired) PVOID pvBuffer,
    _In_      SIZE_T cbBuffer,
    _Out_opt_ SIZE_T *pcbWrittenOrRequired
    );

#endif // (_WIN32_WINNT > 0x0500) || (_WIN32_FUSION >= 0x0100) || ISOLATION_AWARE_ENABLED


#if _WIN32_WINNT >= 0x0501

WINBASEAPI
DWORD
WINAPI
WTSGetActiveConsoleSessionId(
    VOID
    );

#endif // (_WIN32_WINNT >= 0x0501)

#if _WIN32_WINNT >= 0x0601

WINBASEAPI
WORD
WINAPI
GetActiveProcessorGroupCount(
    VOID
    );

WINBASEAPI
WORD
WINAPI
GetMaximumProcessorGroupCount(
    VOID
    );

WINBASEAPI
DWORD
WINAPI
GetActiveProcessorCount(
    _In_ WORD GroupNumber
    );

WINBASEAPI
DWORD
WINAPI
GetMaximumProcessorCount(
    _In_ WORD GroupNumber
    );

#endif // (_WIN32_WINNT >=0x0601)

//
// NUMA Information routines.
//

WINBASEAPI
BOOL
WINAPI
GetNumaProcessorNode(
    _In_  UCHAR Processor,
    _Out_ PUCHAR NodeNumber
    );

#if _WIN32_WINNT >= 0x0601

WINBASEAPI
BOOL
WINAPI
GetNumaNodeNumberFromHandle(
    _In_  HANDLE hFile,
    _Out_ PUSHORT NodeNumber
    );

#endif // (_WIN32_WINNT >=0x0601)

#if _WIN32_WINNT >= 0x0601

WINBASEAPI
BOOL
WINAPI
GetNumaProcessorNodeEx(
    _In_  PPROCESSOR_NUMBER Processor,
    _Out_ PUSHORT NodeNumber
    );

#endif // (_WIN32_WINNT >=0x0601)

WINBASEAPI
BOOL
WINAPI
GetNumaNodeProcessorMask(
    _In_  UCHAR Node,
    _Out_ PULONGLONG ProcessorMask
    );

WINBASEAPI
BOOL
WINAPI
GetNumaAvailableMemoryNode(
    _In_  UCHAR Node,
    _Out_ PULONGLONG AvailableBytes
    );

#if _WIN32_WINNT >= 0x0601

WINBASEAPI
BOOL
WINAPI
GetNumaAvailableMemoryNodeEx(
    _In_  USHORT Node,
    _Out_ PULONGLONG AvailableBytes
    );

#endif // (_WIN32_WINNT >=0x0601)

#if (_WIN32_WINNT >= 0x0600)

WINBASEAPI
BOOL
WINAPI
GetNumaProximityNode(
    _In_  ULONG ProximityId,
    _Out_ PUCHAR NodeNumber
    );

#endif

#if _WIN32_WINNT >= 0x0601

WINBASEAPI
BOOL
WINAPI
GetNumaProximityNodeEx(
    _In_  ULONG ProximityId,
    _Out_ PUSHORT NodeNumber
    );

#endif // (_WIN32_WINNT >=0x0601)

//
// Application restart and data recovery callback
//
typedef DWORD (WINAPI *APPLICATION_RECOVERY_CALLBACK)(PVOID pvParameter);

//
// Max length of commandline in characters (including the NULL character that can be registered for restart)
//
#define RESTART_MAX_CMD_LINE    1024

//
// Do not restart the process for termination due to application crashes
//
#define RESTART_NO_CRASH        1

//
// Do not restart the process for termination due to application hangs
//
#define RESTART_NO_HANG         2

//
// Do not restart the process for termination due to patch installations
//
#define RESTART_NO_PATCH        4

//
// Do not restart the process when the system is rebooted due to patch installations
//
#define RESTART_NO_REBOOT        8

#define RECOVERY_DEFAULT_PING_INTERVAL  5000
#define RECOVERY_MAX_PING_INTERVAL      (5 * 60 * 1000)

#if (_WIN32_WINNT >= 0x0600)

WINBASEAPI
HRESULT
WINAPI
RegisterApplicationRecoveryCallback(
    _In_  APPLICATION_RECOVERY_CALLBACK pRecoveyCallback,
    _In_opt_  PVOID pvParameter,
    _In_ DWORD dwPingInterval,
    _In_ DWORD dwFlags
    );

WINBASEAPI
HRESULT
WINAPI
UnregisterApplicationRecoveryCallback(void);

WINBASEAPI
HRESULT
WINAPI
RegisterApplicationRestart(
    _In_opt_ PCWSTR pwzCommandline,
    _In_ DWORD dwFlags
    );

WINBASEAPI
HRESULT
WINAPI
UnregisterApplicationRestart(void);

WINBASEAPI
HRESULT
WINAPI
GetApplicationRecoveryCallback(
    _In_  HANDLE hProcess,
    _Out_ APPLICATION_RECOVERY_CALLBACK* pRecoveryCallback,
    _Outptr_opt_result_maybenull_ PVOID* ppvParameter,
    _Out_opt_ PDWORD pdwPingInterval,
    _Out_opt_ PDWORD pdwFlags
    );

WINBASEAPI
HRESULT
WINAPI
GetApplicationRestartSettings(
    _In_ HANDLE hProcess,
    _Out_writes_opt_(*pcchSize) PWSTR pwzCommandline,
    _Inout_ PDWORD pcchSize,
    _Out_opt_ PDWORD pdwFlags
    );

WINBASEAPI
HRESULT
WINAPI
ApplicationRecoveryInProgress(
    _Out_ PBOOL pbCancelled
    );

WINBASEAPI
VOID
WINAPI
ApplicationRecoveryFinished(
    _In_ BOOL bSuccess
    );

#endif // _WIN32_WINNT >= 0x0600

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#if (_WIN32_WINNT >= 0x0600)

#pragma region Application Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)

typedef struct _FILE_BASIC_INFO {
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    DWORD FileAttributes;
} FILE_BASIC_INFO, *PFILE_BASIC_INFO;

typedef struct _FILE_STANDARD_INFO {
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    DWORD NumberOfLinks;
    BOOLEAN DeletePending;
    BOOLEAN Directory;
} FILE_STANDARD_INFO, *PFILE_STANDARD_INFO;

typedef struct _FILE_NAME_INFO {
    DWORD FileNameLength;
    WCHAR FileName[1];
} FILE_NAME_INFO, *PFILE_NAME_INFO;

typedef struct _FILE_RENAME_INFO {
    BOOLEAN ReplaceIfExists;
    HANDLE RootDirectory;
    DWORD FileNameLength;
    WCHAR FileName[1];
} FILE_RENAME_INFO, *PFILE_RENAME_INFO;

typedef struct _FILE_ALLOCATION_INFO {
    LARGE_INTEGER AllocationSize;
} FILE_ALLOCATION_INFO, *PFILE_ALLOCATION_INFO;

typedef struct _FILE_END_OF_FILE_INFO {
    LARGE_INTEGER EndOfFile;
} FILE_END_OF_FILE_INFO, *PFILE_END_OF_FILE_INFO;

typedef struct _FILE_STREAM_INFO {
    DWORD NextEntryOffset;
    DWORD StreamNameLength;
    LARGE_INTEGER StreamSize;
    LARGE_INTEGER StreamAllocationSize;
    WCHAR StreamName[1];
} FILE_STREAM_INFO, *PFILE_STREAM_INFO;

typedef struct _FILE_COMPRESSION_INFO {
    LARGE_INTEGER CompressedFileSize;
    WORD CompressionFormat;
    UCHAR CompressionUnitShift;
    UCHAR ChunkShift;
    UCHAR ClusterShift;
    UCHAR Reserved[3];
} FILE_COMPRESSION_INFO, *PFILE_COMPRESSION_INFO;

typedef struct _FILE_ATTRIBUTE_TAG_INFO {
    DWORD FileAttributes;
    DWORD ReparseTag;
} FILE_ATTRIBUTE_TAG_INFO, *PFILE_ATTRIBUTE_TAG_INFO;

typedef struct _FILE_DISPOSITION_INFO {
    BOOLEAN DeleteFile;
} FILE_DISPOSITION_INFO, *PFILE_DISPOSITION_INFO;

typedef struct _FILE_ID_BOTH_DIR_INFO {
    DWORD NextEntryOffset;
    DWORD FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    DWORD FileAttributes;
    DWORD FileNameLength;
    DWORD EaSize;
    CCHAR ShortNameLength;
    WCHAR ShortName[12];
    LARGE_INTEGER FileId;
    WCHAR FileName[1];
} FILE_ID_BOTH_DIR_INFO, *PFILE_ID_BOTH_DIR_INFO;

typedef struct _FILE_FULL_DIR_INFO {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    WCHAR FileName[1];
} FILE_FULL_DIR_INFO, *PFILE_FULL_DIR_INFO;

typedef enum _PRIORITY_HINT {
      IoPriorityHintVeryLow = 0,
      IoPriorityHintLow,
      IoPriorityHintNormal,
      MaximumIoPriorityHintType
} PRIORITY_HINT;

typedef struct _FILE_IO_PRIORITY_HINT_INFO {
    PRIORITY_HINT PriorityHint;
} FILE_IO_PRIORITY_HINT_INFO, *PFILE_IO_PRIORITY_HINT_INFO;

// Structure and constants must match those in ntioapi_x.w

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)

typedef struct _FILE_ALIGNMENT_INFO {
    ULONG AlignmentRequirement;
} FILE_ALIGNMENT_INFO, *PFILE_ALIGNMENT_INFO;


//
//  Flag definitions for FILE_STORAGE_INFO structure
//

//
//  If this flag is set then the partition is correctly aligned with the
//  physical sector size of the device for optimial performance.
//
#define STORAGE_INFO_FLAGS_ALIGNED_DEVICE                 0x00000001
#define STORAGE_INFO_FLAGS_PARTITION_ALIGNED_ON_DEVICE    0x00000002

//
//  If this value is set for the Sector and Parition alignment
//  fields then it means the alignment is not known and the
//  alignment flags have no meaning
//
#define STORAGE_INFO_OFFSET_UNKNOWN (0xffffffff)

typedef struct _FILE_STORAGE_INFO {
    ULONG LogicalBytesPerSector;
    ULONG PhysicalBytesPerSectorForAtomicity;
    ULONG PhysicalBytesPerSectorForPerformance;
    ULONG FileSystemEffectivePhysicalBytesPerSectorForAtomicity;
    ULONG Flags;
    ULONG ByteOffsetForSectorAlignment;
    ULONG ByteOffsetForPartitionAlignment;
} FILE_STORAGE_INFO, *PFILE_STORAGE_INFO;

//
//  Structure definition for FileIdInfo
//
typedef struct _FILE_ID_INFO {
    ULONGLONG VolumeSerialNumber;
    FILE_ID_128 FileId;
} FILE_ID_INFO, *PFILE_ID_INFO;

//
//  Structure definition for FileIdExtdDirectoryInfo
//
typedef struct _FILE_ID_EXTD_DIR_INFO {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    ULONG ReparsePointTag;
    FILE_ID_128 FileId;
    WCHAR FileName[1];
} FILE_ID_EXTD_DIR_INFO, *PFILE_ID_EXTD_DIR_INFO;

#endif

//
// File Remote protocol info (FileRemoteProtocolInfo)
//

// Protocol generic flags.

#define REMOTE_PROTOCOL_INFO_FLAG_LOOPBACK              0x00000001
#define REMOTE_PROTOCOL_INFO_FLAG_OFFLINE               0x00000002

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
#define REMOTE_PROTOCOL_INFO_FLAG_PERSISTENT_HANDLE     0x00000004
#endif

// Protocol specific SMB2 share capability flags.

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
#define RPI_FLAG_SMB2_SHARECAP_TIMEWARP                0x00000002
#define RPI_FLAG_SMB2_SHARECAP_DFS                     0x00000008
#define RPI_FLAG_SMB2_SHARECAP_CONTINUOUS_AVAILABILITY 0x00000010
#define RPI_FLAG_SMB2_SHARECAP_SCALEOUT                0x00000020
#define RPI_FLAG_SMB2_SHARECAP_CLUSTER                 0x00000040
#endif

// Protocol specific SMB2 server capability flags.

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
#define RPI_SMB2_FLAG_SERVERCAP_DFS                    0x00000001
#define RPI_SMB2_FLAG_SERVERCAP_LEASING                0x00000002
#define RPI_SMB2_FLAG_SERVERCAP_LARGEMTU               0x00000004
#define RPI_SMB2_FLAG_SERVERCAP_MULTICHANNEL           0x00000008
#define RPI_SMB2_FLAG_SERVERCAP_PERSISTENT_HANDLES     0x00000010
#define RPI_SMB2_FLAG_SERVERCAP_DIRECTORY_LEASING      0x00000020
#endif

typedef struct _FILE_REMOTE_PROTOCOL_INFO
{
    // Structure Version
    USHORT StructureVersion;     // 1
    USHORT StructureSize;        // sizeof(FILE_REMOTE_PROTOCOL_INFO)

    ULONG  Protocol;             // Protocol (WNNC_NET_*) defined in winnetwk.h or ntifs.h.

    // Protocol Version & Type
    USHORT ProtocolMajorVersion;
    USHORT ProtocolMinorVersion;
    USHORT ProtocolRevision;

    USHORT Reserved;

    // Protocol-Generic Information
    ULONG  Flags;

    struct {
        ULONG Reserved[8];
    } GenericReserved;

    // Protocol specific information

#if (_WIN32_WINNT < _WIN32_WINNT_WIN8)
    struct {
        ULONG Reserved[16];
    } ProtocolSpecificReserved;
#else
    union {

        struct {

            struct {
                ULONG Capabilities;
            } Server;

            struct {
                ULONG Capabilities;
                ULONG CachingFlags;
            } Share;

        } Smb2;

        ULONG Reserved[16];

    } ProtocolSpecific;

#endif

} FILE_REMOTE_PROTOCOL_INFO, *PFILE_REMOTE_PROTOCOL_INFO;

WINBASEAPI
BOOL
WINAPI
GetFileInformationByHandleEx(
    _In_  HANDLE hFile,
    _In_  FILE_INFO_BY_HANDLE_CLASS FileInformationClass,
    _Out_writes_bytes_(dwBufferSize) LPVOID lpFileInformation,
    _In_  DWORD dwBufferSize
);

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

typedef enum _FILE_ID_TYPE {
      FileIdType,
      ObjectIdType,
      ExtendedFileIdType,
      MaximumFileIdType
} FILE_ID_TYPE, *PFILE_ID_TYPE;

typedef struct FILE_ID_DESCRIPTOR {
    DWORD dwSize;  // Size of the struct
    FILE_ID_TYPE Type; // Describes the type of identifier passed in.
    union {
        LARGE_INTEGER FileId;
        GUID ObjectId;
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
        FILE_ID_128 ExtendedFileId;
#endif
    } DUMMYUNIONNAME;
} FILE_ID_DESCRIPTOR, *LPFILE_ID_DESCRIPTOR;

WINBASEAPI
HANDLE
WINAPI
OpenFileById (
    _In_     HANDLE hVolumeHint,
    _In_     LPFILE_ID_DESCRIPTOR lpFileId,
    _In_     DWORD dwDesiredAccess,
    _In_     DWORD dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_     DWORD dwFlagsAndAttributes
    );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#endif

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

#if (_WIN32_WINNT >= 0x0600)

//
//  Flags to be passed into CREATE_SYMBOLIC_LINK
//

#define SYMBOLIC_LINK_FLAG_DIRECTORY            (0x1)

#define VALID_SYMBOLIC_LINK_FLAGS  SYMBOLIC_LINK_FLAG_DIRECTORY // & whatever other flags we think of!

WINBASEAPI
BOOLEAN
APIENTRY
CreateSymbolicLinkA (
    _In_ LPCSTR lpSymlinkFileName,
    _In_ LPCSTR lpTargetFileName,
    _In_ DWORD dwFlags
    );
WINBASEAPI
BOOLEAN
APIENTRY
CreateSymbolicLinkW (
    _In_ LPCWSTR lpSymlinkFileName,
    _In_ LPCWSTR lpTargetFileName,
    _In_ DWORD dwFlags
    );
#ifdef UNICODE
#define CreateSymbolicLink  CreateSymbolicLinkW
#else
#define CreateSymbolicLink  CreateSymbolicLinkA
#endif // !UNICODE

WINBASEAPI
BOOLEAN
APIENTRY
CreateSymbolicLinkTransactedA (
    _In_     LPCSTR lpSymlinkFileName,
    _In_     LPCSTR lpTargetFileName,
    _In_     DWORD dwFlags,
    _In_     HANDLE hTransaction
    );
WINBASEAPI
BOOLEAN
APIENTRY
CreateSymbolicLinkTransactedW (
    _In_     LPCWSTR lpSymlinkFileName,
    _In_     LPCWSTR lpTargetFileName,
    _In_     DWORD dwFlags,
    _In_     HANDLE hTransaction
    );
#ifdef UNICODE
#define CreateSymbolicLinkTransacted  CreateSymbolicLinkTransactedW
#else
#define CreateSymbolicLinkTransacted  CreateSymbolicLinkTransactedA
#endif // !UNICODE

#endif // (_WIN32_WINNT >= 0x0600)


#if (_WIN32_WINNT >= 0x0600)

WINBASEAPI
BOOL
WINAPI
QueryActCtxSettingsW(
    _In_opt_      DWORD dwFlags,
    _In_opt_      HANDLE hActCtx,
    _In_opt_      PCWSTR settingsNameSpace,
    _In_          PCWSTR settingName,
    _Out_writes_bytes_to_opt_(dwBuffer, *pdwWrittenOrRequired) PWSTR pvBuffer,
    _In_      SIZE_T dwBuffer,
    _Out_opt_ SIZE_T *pdwWrittenOrRequired
    );

#endif

#if (_WIN32_WINNT >= 0x0600)

WINBASEAPI
BOOL
WINAPI
ReplacePartitionUnit (
    _In_ PWSTR TargetPartition,
    _In_ PWSTR SparePartition,
    _In_ ULONG Flags
    );

#endif


#if (_WIN32_WINNT >= 0x0600)

WINBASEAPI
BOOL
WINAPI
AddSecureMemoryCacheCallback(
    _In_ __callback PSECURE_MEMORY_CACHE_CALLBACK pfnCallBack
    );

WINBASEAPI
BOOL
WINAPI
RemoveSecureMemoryCacheCallback(
    _In_ __callback PSECURE_MEMORY_CACHE_CALLBACK pfnCallBack
    );

#endif

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#if (NTDDI_VERSION >= NTDDI_WIN7SP1)

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

_Must_inspect_result_
WINBASEAPI
BOOL
WINAPI
CopyContext(
    _Inout_ PCONTEXT Destination,
    _In_ DWORD ContextFlags,
    _In_ PCONTEXT Source
    );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#pragma region Application Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)

_Success_(return != FALSE)
WINBASEAPI
BOOL
WINAPI
InitializeContext(
    _Out_writes_bytes_opt_(*ContextLength) PVOID Buffer,
    _In_ DWORD ContextFlags,
    _Out_ PCONTEXT* Context,
    _Inout_ PDWORD ContextLength
    );
#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion

#if defined(_AMD64_) || defined(_X86_)

#pragma region Application Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)

WINBASEAPI
DWORD64
WINAPI
GetEnabledXStateFeatures(
    VOID
    );

_Must_inspect_result_
WINBASEAPI
BOOL
WINAPI
GetXStateFeaturesMask(
    _In_ PCONTEXT Context,
    _Out_ PDWORD64 FeatureMask
    );

_Success_(return != NULL)
WINBASEAPI
PVOID
WINAPI
LocateXStateFeature(
    _In_ PCONTEXT Context,
    _In_ DWORD FeatureId,
    _Out_opt_ PDWORD Length
    );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) */
#pragma endregion

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

_Must_inspect_result_
WINBASEAPI
BOOL
WINAPI
SetXStateFeaturesMask(
    _Inout_ PCONTEXT Context,
    _In_ DWORD64 FeatureMask
    );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#endif /* defined(_AMD64_) || defined(_X86_) */

#endif /* (NTDDI_VERSION >= NTDDI_WIN7SP1) */

#if (_WIN32_WINNT >= 0x0601)

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

WINBASEAPI
DWORD
APIENTRY
EnableThreadProfiling(
    _In_ HANDLE ThreadHandle,
    _In_ DWORD Flags,
    _In_ DWORD64 HardwareCounters,
    _Out_ HANDLE *PerformanceDataHandle
    );

WINBASEAPI
DWORD
APIENTRY
DisableThreadProfiling(
    _In_ HANDLE PerformanceDataHandle
    );

WINBASEAPI
DWORD
APIENTRY
QueryThreadProfiling(
    _In_ HANDLE ThreadHandle,
    _Out_ PBOOLEAN Enabled
    );

WINBASEAPI
DWORD
APIENTRY
ReadThreadProfilingData(
    _In_ HANDLE PerformanceDataHandle,
    _In_ DWORD Flags,
    _Out_ PPERFORMANCE_DATA PerformanceData
    );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#endif /* (_WIN32_WINNT >= 0x0601) */



#if !defined(RC_INVOKED) /* RC complains about long symbols in #ifs */
#if defined(ISOLATION_AWARE_ENABLED) && (ISOLATION_AWARE_ENABLED != 0)
#include "winbase.inl"
#endif /* ISOLATION_AWARE_ENABLED */
#endif /* RC */

#ifdef __cplusplus
}
#endif

#if defined (_MSC_VER)
#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4001) /* nonstandard extension : single line comment */
#pragma warning(default:4201) /* nonstandard extension used : nameless struct/union */
#pragma warning(default:4214) /* nonstandard extension used : bit field types other then int */
#endif
#endif



#endif // _WINBASE_

#if !defined(RC_INVOKED)
#if !defined(NOWINBASEINTERLOCK)
#if !defined(_NTOS_)
/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    winbase_interlockedcplusplus.h

Abstract:

    C++ function overloads in place of "manual name mangling".
    This file is meant to be #included by winbase.h or any other file declaring the signed interlocked functions.

Author:

    Jay Krell (JayKrell) April 2002

--*/

#if !defined(RC_INVOKED) /* { */

#if !defined(MICROSOFT_WINDOWS_WINBASE_INTERLOCKED_CPLUSPLUS_H_INCLUDED) /* { */
#define MICROSOFT_WINDOWS_WINBASE_INTERLOCKED_CPLUSPLUS_H_INCLUDED
#if _MSC_VER > 1000
#pragma once
#endif

#if !defined(MIDL_PASS) /* { */

/*
To turn off/hide the contents of this file:
 #define MICROSOFT_WINDOWS_WINBASE_H_DEFINE_INTERLOCKED_CPLUSPLUS_OVERLOADS 0
*/

#if !defined(MICROSOFT_WINDOWS_WINBASE_H_DEFINE_INTERLOCKED_CPLUSPLUS_OVERLOADS)
#define MICROSOFT_WINDOWS_WINBASE_H_DEFINE_INTERLOCKED_CPLUSPLUS_OVERLOADS (_WIN32_WINNT >= 0x0502 || !defined(_WINBASE_))
#endif

#if MICROSOFT_WINDOWS_WINBASE_H_DEFINE_INTERLOCKED_CPLUSPLUS_OVERLOADS  /* { */

#if defined(__cplusplus) /* { */

extern "C++" {

FORCEINLINE
unsigned
InterlockedIncrement(
    _Inout_ _Interlocked_operand_ unsigned volatile *Addend
    )
{
    return (unsigned) InterlockedIncrement((volatile long*) Addend);
}

FORCEINLINE
unsigned long
InterlockedIncrement(
    _Inout_ _Interlocked_operand_ unsigned long volatile *Addend
    )
{
    return (unsigned long) InterlockedIncrement((volatile long*) Addend);
}

#if defined(_WIN64) || ((_WIN32_WINNT >= 0x0502) && defined(_WINBASE_))

FORCEINLINE
unsigned __int64
InterlockedIncrement(
    _Inout_ _Interlocked_operand_ unsigned __int64 volatile *Addend
    )
{
    return (unsigned __int64) InterlockedIncrement64((volatile __int64*) Addend);
}

#endif

FORCEINLINE
unsigned
InterlockedDecrement(
    _Inout_ _Interlocked_operand_ unsigned volatile *Addend
    )
{
    return (unsigned long) InterlockedDecrement((volatile long*) Addend);
}

FORCEINLINE
unsigned long
InterlockedDecrement(
    _Inout_ _Interlocked_operand_ unsigned long volatile *Addend
    )
{
    return (unsigned long) InterlockedDecrement((volatile long*) Addend);
}

#if defined(_WIN64) || ((_WIN32_WINNT >= 0x0502) && defined(_WINBASE_))

FORCEINLINE
unsigned __int64
InterlockedDecrement(
    _Inout_ _Interlocked_operand_ unsigned __int64 volatile *Addend
    )
{
    return (unsigned __int64) InterlockedDecrement64((volatile __int64*) Addend);
}

#endif

FORCEINLINE
unsigned
InterlockedExchange(
    _Inout_ _Interlocked_operand_ unsigned volatile *Target,
    _In_ unsigned Value
    )
{
    return (unsigned) InterlockedExchange((volatile long*) Target, (long) Value);
}

FORCEINLINE
unsigned long
InterlockedExchange(
    _Inout_ _Interlocked_operand_ unsigned long volatile *Target,
    _In_ unsigned long Value
    )
{
    return (unsigned long) InterlockedExchange((volatile long*) Target, (long) Value);
}

#if defined(_WIN64) || ((_WIN32_WINNT >= 0x0502) && defined(_WINBASE_))

FORCEINLINE
unsigned __int64
InterlockedExchange(
    _Inout_ _Interlocked_operand_ unsigned __int64 volatile *Target,
    _In_ unsigned __int64 Value
    )
{
    return (unsigned __int64) InterlockedExchange64((volatile __int64*) Target, (__int64) Value);
}

#endif

FORCEINLINE
unsigned
InterlockedExchangeAdd(
    _Inout_ _Interlocked_operand_ unsigned volatile *Addend,
    _In_ unsigned Value
    )
{
    return (unsigned) InterlockedExchangeAdd((volatile long*) Addend, (long) Value);
}

FORCEINLINE
unsigned
InterlockedExchangeSubtract(
    _Inout_ _Interlocked_operand_ unsigned volatile *Addend,
    _In_ unsigned Value
    )
{
    return (unsigned) InterlockedExchangeAdd((volatile long*) Addend,  - (long) Value);
}

FORCEINLINE
unsigned long
InterlockedExchangeAdd(
    _Inout_ _Interlocked_operand_ unsigned long volatile *Addend,
    _In_ unsigned long Value
    )
{
    return (unsigned long) InterlockedExchangeAdd((volatile long*) Addend, (long) Value);
}

FORCEINLINE
unsigned long
InterlockedExchangeSubtract(
    _Inout_ _Interlocked_operand_ unsigned long volatile *Addend,
    _In_ unsigned long Value
    )
{
    return (unsigned long) InterlockedExchangeAdd((volatile long*) Addend,  - (long) Value);
}

#if defined(_WIN64) || ((_WIN32_WINNT >= 0x0502) && defined(_WINBASE_))

FORCEINLINE
unsigned __int64
InterlockedExchangeAdd(
    _Inout_ _Interlocked_operand_ unsigned __int64 volatile *Addend,
    _In_ unsigned __int64 Value
    )
{
    return (unsigned __int64) InterlockedExchangeAdd64((volatile __int64*) Addend,  (__int64) Value);
}

FORCEINLINE
unsigned __int64
InterlockedExchangeSubtract(
    _Inout_ _Interlocked_operand_ unsigned __int64 volatile *Addend,
    _In_ unsigned __int64 Value
    )
{
    return (unsigned __int64) InterlockedExchangeAdd64((volatile __int64*) Addend,  - (__int64) Value);
}

#endif

FORCEINLINE
unsigned
InterlockedCompareExchange(
    _Inout_ _Interlocked_operand_ unsigned volatile *Destination,
    _In_ unsigned Exchange,
    _In_ unsigned Comperand
    )
{
    return (unsigned) InterlockedCompareExchange((volatile long*) Destination, (long) Exchange, (long) Comperand);
}

FORCEINLINE
unsigned long
InterlockedCompareExchange(
    _Inout_ _Interlocked_operand_ unsigned long volatile *Destination,
    _In_ unsigned long Exchange,
    _In_ unsigned long Comperand
    )
{
    return (unsigned long) InterlockedCompareExchange((volatile long*) Destination, (long) Exchange, (long) Comperand);
}

#if defined(_WIN64) || ((_WIN32_WINNT >= 0x0502) && defined(_WINBASE_))

FORCEINLINE
unsigned __int64
InterlockedCompareExchange(
    _Inout_ _Interlocked_operand_ unsigned __int64 volatile *Destination,
    _In_ unsigned __int64 Exchange,
    _In_ unsigned __int64 Comperand
    )
{
    return (unsigned __int64) InterlockedCompareExchange64((volatile __int64*) Destination, (__int64) Exchange, (__int64) Comperand);
}

FORCEINLINE
unsigned __int64
InterlockedAnd(
    _Inout_ _Interlocked_operand_ unsigned __int64 volatile *Destination,
    _In_ unsigned __int64 Value
    )
{
    return (unsigned __int64) InterlockedAnd64((volatile __int64*) Destination, (__int64) Value);
}

FORCEINLINE
unsigned __int64
InterlockedOr(
    _Inout_ _Interlocked_operand_ unsigned __int64 volatile *Destination,
    _In_ unsigned __int64 Value
    )
{
    return (unsigned __int64) InterlockedOr64((volatile __int64*) Destination, (__int64) Value);
}

FORCEINLINE
unsigned __int64
InterlockedXor(
    _Inout_ _Interlocked_operand_ unsigned __int64 volatile *Destination,
    _In_ unsigned __int64 Value
    )
{
    return (unsigned __int64) InterlockedXor64((volatile __int64*) Destination, (__int64) Value);
}

#endif

} /* extern "C++" */
#endif /* } __cplusplus */

#endif /* } MICROSOFT_WINBASE_H_DEFINE_INTERLOCKED_CPLUSPLUS_OVERLOADS */

#undef MICROSOFT_WINBASE_H_DEFINE_INTERLOCKED_CPLUSPLUS_OVERLOADS
#define MICROSOFT_WINBASE_H_DEFINE_INTERLOCKED_CPLUSPLUS_OVERLOADS 0

#endif /* } MIDL_PASS */
#endif /* } MICROSOFT_WINDOWS_WINBASE_INTERLOCKED_CPLUSPLUS_H_INCLUDED */
#endif /* } RC_INVOKED */
#endif /* _NTOS_ */
#endif /* NOWINBASEINTERLOCK */
#endif /* RC_INVOKED */
