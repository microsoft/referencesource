/******************************************************************************
 *
 * File: util_watson.cpp
 *
 * Description: implementation for util_watson.h
 ******************************************************************************/

#include "prefast.h"

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#ifndef STRICT
#define STRICT
#endif

#define WIN32_LEAN_AND_MEAN
#include "windows.h"

#include "msodw.h"
#include "util_watson.h"

#define ERRORREPORT_VERSION L"9.0"

static ErrorReportingMode s_ErrorReportingMode = ErrorReportingNone;
static wchar_t const * s_szErrorReportingDlls = L"";
static wchar_t const * s_szFormalAppName = L"";
static bool s_fExit = true;
static bool s_fDebug = false;

// internals
static DWSharedMem *        WatsonSharedMem = NULL;
static SECURITY_ATTRIBUTES  WatsonSA;
static HANDLE               WatsonSharedMemHandle = NULL;

ErrorReportingMode ErrorReportingGetMode() 
{
    return s_ErrorReportingMode;
}

bool ErrorReportingSetMode(ErrorReportingMode ErrorReportingMode) 
{
    s_ErrorReportingMode = ErrorReportingMode;
    return true;
}

bool ErrorReportingSetDlls(wchar_t const * szErrorReportingDlls)
{
    s_szErrorReportingDlls = szErrorReportingDlls;
    return true;
}

bool ErrorReportingSetFormalAppName(wchar_t const * szFormalAppName)
{
    s_szFormalAppName = szFormalAppName;
    return true;
}

bool ErrorReportingSetExit(bool fExit)
{
    s_fExit = fExit;
    return true;
}

bool ErrorReportingSetDebug(bool fDebug)
{
    s_fDebug = fDebug;
    return true;
}

//------------------------------------------------------------------------------
// ErrorReportingDisabled
//    returns true if ErrorReportingFilter will not invoke Watson
//------------------------------------------------------------------------------
bool ErrorReportingDisabled()
{
    switch (s_ErrorReportingMode) {
        case ErrorReportingQueue:
        case ErrorReportingPrompt:
            return false;
        case ErrorReportingSend:
            return !ErrorReportingCheckOptIn();
        case ErrorReportingNone:
        default:
            return true;
    };
}

//--------------------------------------------------------------------------
// ErrorReportingCheckOptIn
//   Check a registry key to see if the user has allowed us to automatically
//   send error reports to microsoft.
//--------------------------------------------------------------------------
bool ErrorReportingCheckOptIn()
{

// Fix for warnings as errors when building against WinBlue build 9444.0.130614-1739
// warning C4996: 'GetVersion': was declared deprecated
// externalapis\windows\8.1\sdk\inc\sysinfoapi.h(110)
// Deprecated. Use VerifyVersionInfo* or IsWindows* macros from VersionHelpers.
#pragma warning( disable : 4996 )

    // Get the OS version: we should defer to system error reporting policy 
    // on Vista instead of checking the VS error reporting policy in the
    // registry. To do this, we simply return 'true' here to let the 
    // subsequent code use the system error reporting policy.
    if (LOBYTE(GetVersion()) >= 6)
    {
        return true;
    }

#pragma warning( default: 4996 )
    
    HKEY hkey;

    if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_CURRENT_USER,
                                       L"Software\\Microsoft\\VisualStudio\\" ERRORREPORT_VERSION L"\\General",
                                       0,
                                       KEY_READ,
                                       &hkey))
    {
        DWORD keyvalue;
        DWORD dw;
        DWORD dwType;
        DWORD dwSize = sizeof(keyvalue);
        dw = RegQueryValueExW(hkey, L"UserCanAutoSendErrorReport", NULL, &dwType,
                              (LPBYTE)&keyvalue, &dwSize);

        RegCloseKey(hkey);

        if ( dw == ERROR_SUCCESS )
        {
            if ((dwType == REG_DWORD) && (dwSize == sizeof(DWORD)))
            {
                return keyvalue == 1;
            }
        }
    }

    return false;
}

//--------------------------------------------------------------------------
// FGetDWExe
//   Recover the location of the Watson executable from the registry
//--------------------------------------------------------------------------
// shared reg values between DW and DW COM EXE

#define DEFAULT_SUBPATH_W L"Microsoft\\PCHealth\\ErrorReporting\\DW"
#define QUEUE_REG_SUBPATH_W  L"Software\\" DEFAULT_SUBPATH_W
#define WATSON_INSTALLED_REG_SUBPATH_W QUEUE_REG_SUBPATH_W L"\\Installed"

# if !defined(WATSON_INSTALLED_REG_SUBPATH_WIN64_W)
# define WATSON_INSTALLED_REG_SUBPATH_WIN64_W L"Software\\Wow6432node\\" DEFAULT_SUBPATH_W L"\\Installed"
# endif

#if !defined(WATSON_INSTALLED_REG_VAL_WIN64)
#define WATSON_INSTALLED_REG_VAL_WIN64 L"DW0201"
#endif

static bool FGetDWExe(_Out_z_cap_(cch) wchar_t *szExe, DWORD cch)
{
    HKEY hkey;
    DWORD dw;
    DWORD dwSize;

    // We should always call this with szExe and MAX_PATH

    if (NULL == szExe || cch < MAX_PATH)
    {
        return false;
    }

    // Try getting full path from the registry

    dw = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
#if _WIN64
                      WATSON_INSTALLED_REG_SUBPATH_WIN64_W,
#else
                      WATSON_INSTALLED_REG_SUBPATH_W,
#endif
                      0, KEY_READ, &hkey);

    if (dw != ERROR_SUCCESS) {
        return false;
    }

    DWORD type;
    dwSize = cch * sizeof(wchar_t);
    dw = RegQueryValueExW(hkey,
#if _WIN64
                         WATSON_INSTALLED_REG_VAL_WIN64,
#else
                         DW_Y(WATSON_INSTALLED_REG_VAL),
#endif
                         NULL, &type, (LPBYTE) szExe, &dwSize);
    RegCloseKey(hkey);

    // Make sure it is zero terminated
    szExe[cch - 1] = '\0';

    if (dw != ERROR_SUCCESS) {
        return false;
    }

    if (type == REG_EXPAND_SZ) {
        dw = ExpandEnvironmentStringsW(szExe, szExe, cch);

        if ((dw > cch) || (dw == 0)) {
            return false;
        }
    } else if (type != REG_SZ) {
        return false;
    }

    return true;
}


static bool InitializeWatsonSharedMem() 
{
    WatsonSharedMemHandle = CreateFileMappingW(INVALID_HANDLE_VALUE, &WatsonSA, PAGE_READWRITE, 0,
                                                 sizeof(DWSharedMem), NULL);

    if (WatsonSharedMemHandle == NULL) {
        return false;
    }

    WatsonSharedMem = (DWSharedMem *)MapViewOfFileEx(WatsonSharedMemHandle,
                                         FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0, nullptr);

    if (WatsonSharedMem == NULL) {
        CloseHandle(WatsonSharedMemHandle);
        WatsonSharedMemHandle = NULL;
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
// ErrorReportingFilter
//   Invokes Watson error reporting on all windows exceptions and then exits
//   with error code 1.
//------------------------------------------------------------------------------
LONG _cdecl ErrorReportingFilter(struct _EXCEPTION_POINTERS* exceptionInfo)
{
    // heartbeat event DW signals per EVENT_TIMEOUT; if we stop receiving it,
    // we kill ourselves
    static HANDLE hEventAlive = NULL;

    HANDLE hEventDone = NULL;  // event DW signals when done
    HANDLE hMutex = NULL;       // to protect the signaling of EventDone
    HANDLE hProc = NULL;

    // these are static to avoid extra stack usage in case of a stack overflow
    static wchar_t szCommandLine[MAX_PATH];
    static wchar_t szWatsonApplicationName[MAX_PATH];

    // metainfo for dr. watson process
    static STARTUPINFOW si;
    static PROCESS_INFORMATION pi;

    LONG iReturn = EXCEPTION_EXECUTE_HANDLER;

    if ( hEventAlive ) 
    {
        // we're handling an exception within the filter itself
        return iReturn;
    }

    if ( ErrorReportingDisabled() )
    {
        return iReturn;
    }

    WatsonSA.nLength = sizeof(SECURITY_ATTRIBUTES);
    WatsonSA.lpSecurityDescriptor = NULL;
    WatsonSA.bInheritHandle = TRUE;

    if ( WatsonSharedMem == NULL ) {
        // User didn't initialize the shared memory when the app start
        // Try to see if we can do it now, if not, return.
        if (!InitializeWatsonSharedMem()) {
            return iReturn;
        }
    }

    // get name of Watson executable.
    // use the registry to determine where DW is installed
    if (!FGetDWExe(szWatsonApplicationName, _countof(szWatsonApplicationName))) {
        return iReturn;
    }

    hEventAlive = CreateEventW(&WatsonSA, FALSE, FALSE, NULL);
    hEventDone = CreateEventW(&WatsonSA, FALSE, FALSE, NULL);
    hMutex = CreateMutexW(&WatsonSA, FALSE, NULL);

    HANDLE hCurrentProcess = GetCurrentProcess();

    if (!DuplicateHandle(hCurrentProcess, 
                         hCurrentProcess, 
                         hCurrentProcess, 
                         &hProc,
                         PROCESS_ALL_ACCESS, 
                         TRUE, 
                         0))
    {
        goto LClean;
        // error handling code
    }

    if (hEventAlive == NULL || hEventDone == NULL || hMutex == NULL || hProc == NULL)
    {
        goto LClean;
        // error handling code
    }

    // setup interface structure in the shared memory block
    //
    memset(WatsonSharedMem, 0, sizeof(DWSharedMem));
    WatsonSharedMem->hProc = hProc;
    WatsonSharedMem->pid = GetCurrentProcessId();
    WatsonSharedMem->tid = GetCurrentThreadId();

    // handles for communicating w/ the error reporter
    WatsonSharedMem->hEventAlive = hEventAlive;
    WatsonSharedMem->hEventDone = hEventDone;
    WatsonSharedMem->hMutex = hMutex;
    WatsonSharedMem->dwSize = sizeof(*WatsonSharedMem);
    WatsonSharedMem->dwVersion = DW_CURRENT_VERSION;

    // supply information about what caused the crash
    WatsonSharedMem->pep = exceptionInfo;
    WatsonSharedMem->eip = (DWORD_PTR)exceptionInfo->ExceptionRecord->ExceptionAddress;

    {
        // Fix for warnings as errors when building against WinBlue build 9444.0.130614-1739
        // warning C4996: 'GetVersion': was declared deprecated
        // externalapis\windows\8.1\sdk\inc\sysinfoapi.h(110)
        // Deprecated. Use VerifyVersionInfo* or IsWindows* macros from VersionHelpers.
#pragma warning( disable : 4996 )

        // Get the OS version: this is necessary as Vista doesn't allow fDwuNoEventUI

        bool fIsVistaOrLater = LOBYTE(GetVersion()) >= 6;

#pragma warning( default : 4996 )

        // set behavior flags
        switch (s_ErrorReportingMode) {
        case ErrorReportingQueue:
            WatsonSharedMem->bfDWRFlags = fDwrForceOfflineMode;
            WatsonSharedMem->bfDWUFlags = fIsVistaOrLater ? 0 : fDwuNoEventUI;
            break;
        case ErrorReportingPrompt:
            WatsonSharedMem->bfDWRFlags = 0;
            WatsonSharedMem->bfDWUFlags = 0;
            break;
        case ErrorReportingSend:
            WatsonSharedMem->bfDWRFlags = 0;
            WatsonSharedMem->bfDWUFlags = fIsVistaOrLater ? 0 : fDwuNoEventUI;
            break;
        case ErrorReportingNone:
        default:
            // should never be here
            goto LClean;
        };
    }

    WatsonSharedMem->bfDWLFlags = 0;
    WatsonSharedMem->bfDWEFlags = fDweDefaultQuit | (s_fDebug? 0 : fDweIgnoreAeDebug);
    WatsonSharedMem->bfDWMFlags = 0;

    // we only allow the user to quit (not restart or ignore or recover)
    WatsonSharedMem->bfmsoctdsOffer = msoctdsQuit;

    // if the user chose quit, Dr. Watson allows us to kill ourselves.
    WatsonSharedMem->bfmsoctdsLetRun = msoctdsQuit | (s_fDebug? msoctdsDebug : 0);

    wcsncpy_s(WatsonSharedMem->uib.wzGeneral_AppName, DW_APPNAME_LENGTH,
              s_szFormalAppName, _TRUNCATE);

    // name of app
    // TEMP (Microsoft): We will remove the deprecation for _wpgmptr, see VSW#438137
#pragma warning(push)
#pragma warning(disable:4996)   
    wcsncpy_s(WatsonSharedMem->wzModuleFileName, DW_MAX_PATH, _wpgmptr, _TRUNCATE);
#pragma warning(pop)

    {
        // use default for this value
        const char szPidRegKey[] = "HKLM\\Software\\Microsoft\\Internet Explorer\\Registration\\DigitalProductID";
        memcpy_s(WatsonSharedMem->szPIDRegKey, DW_MAX_PIDREGKEY, szPidRegKey, sizeof(szPidRegKey));
    }
    wcsncpy_s(WatsonSharedMem->wzDotDataDlls, DW_MAX_PATH, s_szErrorReportingDlls, _TRUNCATE);

    // create command line

    swprintf_s(szCommandLine, _countof(szCommandLine),
#ifdef _WIN64
                  DW_EXE L" -x -s %I64u",
#else
                  DW_EXE L" -x -s %u",
#endif
                  (ULONG_PTR)WatsonSharedMemHandle);

    // ready, get memset
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);

    // go!
    if (!CreateProcessW(szWatsonApplicationName, szCommandLine, NULL, NULL, TRUE,
                       CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS, NULL,
                       NULL, &si, &pi))
    {
        goto LClean;
    }
        
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    while (true)
    {
        if (WaitForSingleObject(hEventAlive, DW_TIMEOUT_VALUE )
                                            == WAIT_OBJECT_0)
        {
            if (WaitForSingleObject(hEventDone, 1) == WAIT_OBJECT_0)
            {
                break;
            }
            continue;
        }

        // we timed-out waiting for DW to respond, try to quit
        DWORD status = WaitForSingleObject(hMutex, DW_TIMEOUT_VALUE);
        if (status == WAIT_TIMEOUT)
        {
            break; // either DW's hung or crashed, we must carry on
        }
        else if (status == WAIT_ABANDONED)
        {
            ReleaseMutex(hMutex);
            break;
        }
        else
        {
            // DW has not woken up?
            if (WaitForSingleObject(hEventAlive, 1) != WAIT_OBJECT_0)
            // tell DW we're through waiting for its sorry self
            {
                SetEvent(hEventDone);
                ReleaseMutex(hMutex);
                break;
            }
            else
            {
                // are we done?
                if (WaitForSingleObject(hEventDone, 1) == WAIT_OBJECT_0)
                {
                    ReleaseMutex(hMutex);
                    break;
                }
            }
            ReleaseMutex(hMutex);
        }
    }

    if (WatsonSharedMem->msoctdsResult & msoctdsDebug) {
        iReturn = EXCEPTION_CONTINUE_SEARCH;
    }

LClean:
    if (hEventDone)
        CloseHandle(hEventDone);
    if (hMutex)
        CloseHandle(hMutex);
    if (hProc)
        CloseHandle(hProc);

    if (hEventAlive) {
        CloseHandle(hEventAlive);
        hEventAlive = NULL;
    }
    if (WatsonSharedMem) {
        UnmapViewOfFile(WatsonSharedMem);
        WatsonSharedMem = NULL;
    }
    if (WatsonSharedMemHandle) {
        CloseHandle(WatsonSharedMemHandle);
        WatsonSharedMemHandle = NULL;
    }

    if (s_fExit) {
        exit(1);
    }

    return iReturn;
}


//------------------------------------------------------------------------------
// ErrorReportingInitialize
//   Must be called at the startup of your application, before any exceptions,
//   to set up Watson reporting
//------------------------------------------------------------------------------
void ErrorReportingInitialize() 
{
    WatsonSA.nLength = sizeof(SECURITY_ATTRIBUTES);
    WatsonSA.lpSecurityDescriptor = NULL;
    WatsonSA.bInheritHandle = TRUE;

    InitializeWatsonSharedMem();
}

#if defined(_X86_)
/*---------------------------------------------------------------------------
   This assembly block exists to acquire enough stack space for an
   unhandled exception filter to do all of the work it needs to do after
   a stack overflow operation.  For other exceptions, we just jump directly
   into the normal Watson reporting code at ErrorReportingFilter.

   It is in assembly to minimize the stack space and because it needs to
   munge the stack pointer.
---------------------------------------------------------------- MichMarc -*/
#pragma warning(disable:4414)
#define NEWSTACKSIZE (4096*128)

__declspec(----) LONG _cdecl StackExtendingFilter(PEXCEPTION_POINTERS /* pExceptionInfo */, filterfunc_t * /* filter */)
{
    /* if (pExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW) */
    __asm mov eax, [esp+4]   /* pExceptionInfo  */
    __asm mov ecx, [eax]     /* ExceptionRecord */
    __asm cmp dword ptr [ecx], 0xC00000FD   /* ->ExceptionCode == EXCEPTION_STACK_OVERFLOW */
    __asm jne NEAR JMP_TO_FILTER

    /* pStack = VirtualAlloc(NULL, NEWSTACKSIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);*/
    __asm push PAGE_READWRITE            /* Protection */
    __asm push MEM_RESERVE | MEM_COMMIT  /* Allocation */
    __asm push NEWSTACKSIZE              /* Size       */
    __asm push 0                         /* Address    */
    __asm call dword ptr [VirtualAlloc]

    /* if (pStack == NULL) */
    /*   goto GeneralUnhandledExceptionFilterEx;    // punt -- we're ----ed anyway */
    __asm test eax, eax
    __asm jz   NEAR JMP_TO_FILTER

    /* pStack += NEWSTACKSIZE;   // pStack is now at the top of the stack */
    __asm lea  eax, [eax+NEWSTACKSIZE]

    /* edx = pExceptionInfo */
    __asm mov  edx, [esp+4]
    __asm mov  ecx, [esp+8]

    /* Push old esp on new stack; point stack pointer to new stack */
    __asm mov  [eax-4], esp
    __asm mov  [eax-8], esi
    __asm mov  esi, eax
    __asm lea  esp, [eax-16]

    /* ErrorReportingFilter(pExceptionInfo); */
    __asm push edx
    __asm call ecx

    __asm mov edx, esi
    __asm mov esi, [edx-8]
    __asm mov esp, [edx-4]

    __asm ret
JMP_TO_FILTER:
    __asm mov ecx, [esp+8]
    __asm jmp ecx
};

#pragma warning(default:4414)

#else
// NYI on non-x86 platforms
LONG StackExtendingFilter(PEXCEPTION_POINTERS pExceptionInfo, filterfunc_t * filter)
{
    return filter(pExceptionInfo);
}
#endif // _X86_
