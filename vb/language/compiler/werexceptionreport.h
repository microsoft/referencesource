//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
//-------------------------------------------------------------------------------------------------

#pragma once
#include <werapi.h>

// WerApidl.h
typedef HRESULT (WINAPI * WerReportCreate_t)(
    _In_  PCWSTR pwzEventType,
    _In_ WER_REPORT_TYPE repType,
    _In_opt_ PWER_REPORT_INFORMATION pReportInformation,
    _Out_ HREPORT *phReportHandle);

extern WerReportCreate_t g_pfnWerReportCreate;
#define WerReportCreate (*g_pfnWerReportCreate)

typedef HRESULT (WINAPI *WerReportSetParameter_t)(
    _In_  HREPORT hReportHandle,
    _In_  DWORD dwparamID,
    _In_opt_  PCWSTR pwzName,
    _In_  PCWSTR pwzValue);

extern WerReportSetParameter_t g_pfnWerReportSetParameter;
#define WerReportSetParameter (*g_pfnWerReportSetParameter)

typedef HRESULT (WINAPI *WerReportAddFile_t)(
    _In_  HREPORT hReportHandle,
    _In_  PCWSTR pwzPath,
    _In_  WER_FILE_TYPE repFileType,
    _In_  DWORD  dwFileFlags);

extern WerReportAddFile_t g_pfnWerReportAddFile;
#define WerReportAddFile (*g_pfnWerReportAddFile)

typedef HRESULT  (WINAPI *WerReportSetUIOption_t)(
    _In_  HREPORT hReportHandle,
    _In_  WER_REPORT_UI repUITypeID,
    _In_  PCWSTR pwzValue);

extern WerReportSetUIOption_t g_pfnWerReportSetUIOption;
#define WerReportSetUIOption (*g_pfnWerReportSetUIOption)

typedef HRESULT (WINAPI *WerReportAddSecondaryParameter_t)(
    _In_  HREPORT hReportHandle,
    _In_  PCWSTR pwzKey,
    _In_opt_ PCWSTR pwzValue);

extern WerReportAddSecondaryParameter_t g_pfnWerReportAddSecondaryParameter;
#define WerReportAddSecondaryParameter (*g_pfnWerReportAddSecondaryParameter)

typedef HRESULT (WINAPI *WerReportSubmit_t)(
    _In_ HREPORT hReportHandle,
    _In_ WER_CONSENT consent,
    _In_ DWORD  dwFlags,
    _Out_opt_ PWER_SUBMIT_RESULT pSubmitResult);

extern WerReportSubmit_t g_pfnWerReportSubmit;
#define WerReportSubmit (*g_pfnWerReportSubmit)

typedef HRESULT  (WINAPI *WerReportAddDump_t)(
    _In_ HREPORT hReportHandle,
    _In_ HANDLE  hProcess,
    _In_opt_ HANDLE hThread,
    _In_ WER_DUMP_TYPE dumpType,
    _In_opt_  PWER_EXCEPTION_INFORMATION pExceptionParam,
    _In_opt_ PWER_DUMP_CUSTOM_OPTIONS pDumpCustomOptions,
    _In_ DWORD dwFlags);

extern WerReportAddDump_t g_pfnWerReportAddDump;
#define WerReportAddDump (*g_pfnWerReportAddDump)

typedef HRESULT (WINAPI *WerReportCloseHandle_t)(
    _In_ HREPORT hReportHandle);

extern WerReportCloseHandle_t g_pfnWerReportCloseHandle;
#define WerReportCloseHandle (*g_pfnWerReportCloseHandle)
// end WerApidl.h

// WerUtilString.h

class UtilString
{
    PWSTR       m_pwszData;
    size_t      m_cchTotalSize;
    size_t      m_cchLength;

public:

    UtilString();
    ~UtilString();
    HRESULT Copy(_In_opt_ PCSTR pszString);
    HRESULT Copy(_In_opt_ PCWSTR pwszString);
    HRESULT Copy(_In_ const UtilString& strString);
    HRESULT CopyRawChars(_In_ PCWSTR pRawChars, _In_ size_t cchMaxChars);
    HRESULT Append(_In_opt_ PCWSTR pwszString);
    HRESULT Append(_In_ const UtilString& strString);
    HRESULT __cdecl Sprintf(_In_  _Printf_format_string_ LPCWSTR pwszFormat, ...);
    HRESULT __cdecl vSprintf(_In_ _Printf_format_string_ LPCWSTR pwzFormat, _In_ va_list args);
    operator PCWSTR() const;
    UtilString& operator=(_In_ UtilString&);
    size_t  GetLength() const;
    HRESULT AllocateMemory(_In_ UINT uBufferSize);
    PCWSTR GetData() const;
    HRESULT Replace(_In_  PCWSTR  pwszToFind, _In_  PCWSTR  pwszWith, _Out_opt_ PINT piReplacedCount = NULL);
    BOOL IsSet();
    BOOL IsEmpty();
    VOID UnInitialize();
    HRESULT AppendNewLine();
    HRESULT Attach(_In_ PCWSTR pwszData);
    HRESULT Trim(_In_opt_ PCWSTR pwszChars = NULL);
    PWSTR   Detach();
    HRESULT Truncate(_In_ DWORD dwLength);
    HRESULT GetWindowsDirectory();
    HRESULT GetSystemDirectory();
    DWORD GetHash(_In_ BOOL bCaseSensitive);
    static HRESULT GetUnicodeString(_In_  PCSTR   pszString, _Out_ PWSTR *ppwszBuffer);
    static PCWSTR StrStrI(_In_ PCWSTR pwszString, _In_  PCWSTR pwszMatch);
};
// end WerUitlString.h


// WerUtils.h

#include <tlhelp32.h>

HRESULT UtilGetFileInfo(
    _In_ PCWSTR pwszFilePath,
    _Out_opt_ VS_FIXEDFILEINFO * pFixedFileInfoIN,
    _In_opt_count_(dwNumElements)PCWSTR predefResStrings[],
    _Out_opt_cap_(dwNumElements)UtilString strVersionValues[],
    _In_ DWORD dwNumElements);

HRESULT UtilReadExceptionInformationFromExceptionPointer(
    _In_ HANDLE hProcess,
    _In_ PVOID pvPepAddress,
    _Out_ PEXCEPTION_RECORD pExceptionRecord,
    _Out_opt_ PCONTEXT pContext = NULL);

HRESULT UtilGetModuleNameFromEntry(
    _In_ HANDLE hProcess,
    _In_ PMODULEENTRY32 mod,
    _Inout_ UtilString * pwstrPath);

PCWSTR UtilPathTail(_In_ PCWSTR Path);

// end WerUtils.h

// WerCrashSignature.h


///////////////////////////////// Defines /////////////////////////////////////

//
// Ensure watson stage 1 story, truncate long module names
//
#define MAX_MODULE_NAME 64

///////////////////////////////////////////////////////////////////////////////

class CCrashSignature
{
    HINSTANCE m_hInstance;

    UtilString m_strAppPath;
    UtilString m_strAppName;
    WORD m_wAppVer[4];
    DWORD m_dwAppTimeStamp;

    UtilString m_strAppVer;
    UtilString m_strAppTimeStamp;

    UtilString m_strModPath;
    UtilString m_strModName;
    WORD m_wModVer[4];
    DWORD m_dwModTimeSTamp;
    UtilString m_strModVer;
    UtilString m_strModTimeStamp;


    DWORD_PTR m_exceptionOffset;
    DWORD m_dwExceptionCode;
    DWORD_PTR m_exceptionAddress;

    UtilString m_strExceptionOffset;
    UtilString m_strExceptionCode;


    HRESULT SetModuleInfo(
        _In_ HANDLE hProcess,
        _In_ PMODULEENTRY32 pMod,
        _Inout_ UtilString *pstrPath,
        _Inout_ UtilString *pstrName,
        _Out_cap_(4) WORD arVer[4],
        _Out_ PDWORD pdwTimeStamp
        );

    HRESULT GetExceptionCodeAndAddress(
        _In_ HANDLE hProcess,
        _In_ PEXCEPTION_POINTERS pep,
        _Out_ PDWORD pdwExceptionCode,
        _Out_ PDWORD_PTR pExceptionAddress
        );


public:

    CCrashSignature();
    HRESULT GenerateSignature(
        _In_ HANDLE hProcess,
        _In_opt_ HINSTANCE hInstance,
        _In_opt_ HANDLE hThread,
        _In_opt_ PEXCEPTION_POINTERS pep,
        _In_ DWORD_PTR exceptionAddress = 0);

    HRESULT AddSignaturetoReport(_In_ HREPORT hReport);

    PCWSTR GetAppPath();

    PCWSTR GetModPath();

    PCWSTR GetAppName();

    PCWSTR GetModName();

    const PWORD GetAppVer();

    PCWSTR GetAppVerString();

    const PWORD GetModVer();

    PCWSTR GetModVerString();

    DWORD GetAppTimeStamp();

    PCWSTR GetAppTimeStampString();

    DWORD GetModTimeStamp();

    PCWSTR GetModTimeStampString();

    DWORD_PTR GetFaultingOffset();

    PCWSTR GetFaultingOffsetString();

    DWORD GetExceptionCode();

    PCWSTR GetExceptionCodeString();

    HRESULT LogEvent();
};
// End WerCrashSignature.h

// WerDefConst.h

/////////////////////////////////////// Defines ///////////////////////////////

//
// This is used when we are not able to determine the faulting module
//
#define UNKNOWN_MODULE L"unknown"

//
// This is used when we are not able to determine the exception code
// This is what the watson server uses for unknown exception code
//
#define UNKNOWN_EXCEPTION_CODE L"bbbbbbb4"

//
// Event Type constant for system crashes
//
#define BLUESCREEN_EVENT        L"BLUESCREEN"

//
// Event Type constant for Legacy
//
#define LEGACY_EVENT            L"LEGACY"

#define EVENTLOG_SOURCE_CRASH   L"Application Error"
#define EVENTLOG_SOURCE_WER     L"Windows Error Reporting"

//
// Stage request urls for various types of reports
//
#define APPCRASH_REQ_S1     L"/StageOne"
#define APPCRASH_REQ_S2_64  L"/dw/stagetwo64.asp"
#define APPCRASH_REQ_S2_32  L"/dw/stagetwo.asp"
#define GENERIC_REQ_S1      L"/StageOne/Generic"
#define GENERIC_REQ_S2      L"/dw/generictwo.asp"
#define KERNELMODE_REQ_S2   L"/dw/bluetwo.asp"

//
// The following may be set in the report's state keys to override the way we
// generate urls to hit the watson server.
//
// Watson.Stage2 override must always be set for the override to occur.
//
// Watson.Stage1 is optional; if it is not set, the stage 1 url is skipped
// while reporting.
//
#define WATSON_STATE_STAGE1_OVERRIDE            L"Watson.Stage1"
#define WATSON_STATE_STAGE2_OVERRIDE            L"Watson.Stage2"

//
// If a report was uploaded headlessly but we required 2nd level consent, the
// report will have this state parameter set.
//
#define HEADLESS_UPLOADER_STATE_2NDLEVEL_CONSENT  L"Headless.2ndLevelConsentNeeded"

#define SEC_PARAM_TOKEN_TYPE                    L"Token"
#define SEC_PARAM_LCID                          L"LCID"
#define SEC_PARAM_BRAND                         L"Brand"
#define SEC_PARAM_NO_SECOND_LEVEL_KEY           L"No2nd"
#define SEC_PARAM_NO_SECOND_LEVEL_VAL_NO_HEAP   L"1"
#define SEC_PARAM_NO_SECOND_LEVEL_VAL_NO_DATA   L"2"

#define PROCESS_TERMINATION_EXITCODE 0XFF

// Count of elements in an array
#define ARRAYCOUNT(ar) (sizeof(ar)/sizeof(ar[0]))

#define IsStringSet(x) (x != NULL && x[0] != 0)

#define WER_ASSERT(x)

#define SafeFreeAndNull(x) if ((x) != NULL) { free(x); x = NULL; }


#define VSAlloc(cb)       LocalAlloc(LMEM_FIXED,cb)
#define VSFree(pv)        LocalFree(pv)

#define TRY_ALLOC(x)    \
__try                     \
{                       \
    x;                  \
}                       \
__except (EXCEPTION_EXECUTE_HANDLER)             \
{                       \
    ;                   \
}

#define STDCALL __stdcall

#define SIZE_64_KB (64 * 1024)


//
// Definations for internal file flags used by the infrastructure.
// WARNING: The high 16 bits of the flag DWORD are reserved for internal use
// only. Any flags defined here should live in the high 16 bits of the flag
//
#define RFF_INTERNAL_MARK_FOR_UPLOAD    (1 << 16)  // file will be added to the 2nd level data collection cab.
#define RFF_INTERNAL_REMOVE_IF_QUEUED   (2 << 16)  // file will be removed if the report is added to the queue.
#define RFF_INTERNAL_APPROVED           (4 << 16)  // file has been marked as approved for sending

#define DPF DbgPrintOut

#define NATIVE_CRASH_SIG_COUNT      8
#define IND_CRASH_APP_NAME          0
#define IND_CRASH_APP_VER           1
#define IND_CRASH_APP_TIMESTAMP     2
#define IND_CRASH_MOD_NAME          3
#define IND_CRASH_MOD_VER           4
#define IND_CRASH_MOD_TIMESTAMP     5
#define IND_CRASH_EXCEPTION_CODE    6
#define IND_CRASH_OFFSET            7


//
// The switches must not have any spaces before or after them
//
#define CRASH_VERTICAL_CMD_LINE_PREFIX L"-u"
#define KERNEL_VERTICAL_CMD_LINE_PREFIX L"-k"
#define CRASH_VERTICAL_EXE L"WerFault.exe"
#define KERNEL_VERTICAL_EXE L"WerFault.exe"


//
// ++++++++++++++++++++++++++++++ Live kernel reporting defines +++++++++++++++
//

//
// The component created a live kernel dump and now wants
// WER to pick it up. It goes to OCA same as blue screens
//
#define LIVE_KERNEL_EVENT               L"LiveKernelEvent"

//
// Default sub folder inside %WINDIR% where the reports live
//
#define LIVE_KERNEL_DEFAULT_LOCATION    L"LiveKernelReports"

//
// The reg key in HKLM that will contain the live kernel reports metadata
//
#define LIVE_KERNEL_REPORT_KEY L"LiveKernelReports"

//
// This value in LIVE_KERNEL_REPORT_KEY will contain the actual path where of the
// directory containing the reports
//
#define LIVE_KERNEL_REPORT_PATH_VALUE L"LiveKernelReportsPath"

//
// For a live kernel report instance whether it is busy or not. This is
// a volatile registry key
//
#define LIVE_KERNEL_REPORT_BUSY_KEY L"Busy"

//
// ----------------------------------------------------------------------------
//

#define ERC_EXECUTABLE_NAME                 L"WerCon.exe"
#define ERC_CMDLINE_DISPLAY_RESPONSE        L"-displayresponse"
#define ERC_CMDLINE_QUEUE_REPORTING         WERMGR_CMDLINE_QUEUE_REPORTING

#define RESOURCEID_WER_COMMON_BEGIN (5000)
#define RESOURCEID_PLUGIN_BEGIN     (5500)

__inline HRESULT ERROR_HR_FROM_WIN32(_In_ HRESULT x)
{
    HRESULT hrTemp__ = __HRESULT_FROM_WIN32(x);

    if (SUCCEEDED(hrTemp__))
    {
        return E_FAIL;
    }

    return hrTemp__;
}

//
// The following state key in the report is used to override the way we generate
// subpaths for reports
//
#define CER_STATE_SUBPATH_OVERRIDE  L"Cer.Subpath"

#define WERMGR_EXE_NAME L"wermgr.exe"

#define WER_DEFAULT_SETTINGS_SUBPATH L"Software\\Microsoft\\WindowsErrorReporting"
#define WER_DEFAULT_SETTINGS_SUBPATH_DEBUG L"Software\\Microsoft\\WindowsErrorReporting\\Debug"
#define WER_DEFAULT_POLICIES_SETTINGS_SUBPATH L"Software\\Policies\\Microsoft\\WindowsErrorReporting"

#define NT_LIVE_KERNEL_REG_KEY_PATH L"\\Registry\\Machine\\" WER_DEFAULT_SETTINGS_SUBPATH L"\\" LIVE_KERNEL_REPORT_KEY

#define RAC_GUID_REG_KEY L"SOFTWARE\\Microsoft\\Reliability Analysis\\RAC"
#define RAC_GUID_VALUE L"RacAnalysisId"

#define EXLUSION_LIST   L"ExcludedApplications"
#define DEBUG_LIST      L"DebugApplications"

#define INIT_PTR(ptr, value)    \
    if (ptr)                    \
    {                           \
        *ptr = value;           \
    }

#define PFX_INIT_OUT(ptr, value)    \
    if (ptr)                        \
    {                               \
        *ptr = value;               \
    }

//
// Error codes
//
#define WERP_E_NOTINITIALIZED       ((HRESULT)0xE0000000L)
#define WERP_E_ALREADY_REPORTING    ((HRESULT)0xE0000002L)
#define WERP_E_LENGTH_EXCEEDED      ((HRESULT)0xE0000003L)
#define WERP_E_INVALID_STATE        ((HRESULT)0xE0000004L)

// ------------------------------- Env. Variables that are set for the server ------------
//-------------------------------- Server file data requests can use these    ------------
//
// The directory where an crashing application resides, this is set when we get
// the signature for a crash
#define APP_DIR L"appdir"

//
// The directory where an crashing module resides, this is set when we get
// the signature for a crash
#define MOD_DIR L"moddir"

#define SYSTEM_DIR L"systemdir"
#define PROG_FILES L"progfiles"
#define COMMON_FILES L"commonfiles"
#define MY_DOCUMENTS L"mydocuments"

#define NULL_TERM_BUFFER(Buffer) Buffer[ARRAYCOUNT(Buffer) - 1] = NULL;
#define NULL_NULL_TERM_BUFFER(Buffer) {Buffer[ARRAYCOUNT(Buffer) - 1] = NULL; Buffer[ARRAYCOUNT(Buffer) - 2] = NULL;}

#define WER_INVALID_ID 0

#define DEFAULT_EXECUTION_TIME_BEFORE_RESTART_INSECS (10)

//
// Time in msec that we should wait to make sure that the app has been
// really terminated
//
#define TIME_TERMINATE_VERIFY (4000)

// manifest entries mapping for different versions
#define MFM20   0x0001
#define MFM10   0x0002
#define MFM15   0x0004
#define MFMALL  (MFM15 | MFM10 | MFM20)
#define MFM1X   (MFM15 | MFM10)

///////////////////////////// Types ///////////////////////////////////////////

//
// Data type mostly used for for a policy or registry setting
//
typedef enum _DataType
{
    DataTypeDWORD = 0,
    DataTypeString,
    DataTypeBinary,
    DataTypeDWORDLittleEndian,
    DataTypeDWORDBigEndian,
    DataTypeEnvString,
    DataTypeMultiString,
    DataTypeQWORD,
    DataTypeQWORDLittleEndian,
    DataTypeUnknown
} DataType;

typedef const size_t K_SIZE;

typedef unsigned __int64 QWORD;

typedef enum _DPF_LEVEL
{
    dlError,
    dlWarning,
    dlInfo,
    dlSpew
} DPF_LEVEL;

///////////////////////////////////////////////////////////////////////////////


VOID DbgPrintOut(
    _In_ DPF_LEVEL dbgLevel,
    _In_ _Printf_format_string_ PCSTR pszFmt,
    ...);

// end WerDefConst.h

// WerExceptionReport.h
DWORD LoadWerDll();

HRESULT CreateAndSubmitReport(
    _Out_ WER_SUBMIT_RESULT * pSubmitResult,
    _In_ EXCEPTION_POINTERS * pExceptionPointers,
    _In_ PCWSTR wzAdditionalFiles,
    _In_ WATSON_TYPE howToReport);

// end WerException.h
