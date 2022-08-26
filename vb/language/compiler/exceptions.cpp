//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Exception helpers.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

#if !IDE 
// VSWhidbey DCR[406053] Honor /errorreport:send based on Watson API ErrorReportingCheckOptIn() check.
#include <util_watson.cpp>
#endif

// This should come from msodw.h but the version we have is x86 only`
#define WATSON_INSTALLED_REG_VAL_IA64 "DW0201"
#define WATSON_INSTALLED_REG_SUBPATH_IA64 L"Software\\Wow6432Node\\Microsoft\\PCHealth\\ErrorReporting\\DW\\Installed"

// ---- headers why don't they have a Unicode version?
#if _WIN64
#define WATSON_INSTALLED_REG_VAL_W      DW_Y(WATSON_INSTALLED_REG_VAL_IA64)
#define WATSON_SUBPATH                  WATSON_INSTALLED_REG_SUBPATH_IA64
#else
#define WATSON_INSTALLED_REG_VAL_W      DW_Y(WATSON_INSTALLED_REG_VAL)
#define WATSON_SUBPATH                  L"Software\\" DW_Y(DEFAULT_SUBPATH) L"\\Installed"
#endif

#ifdef FEATURE_CORESYSTEM
WCHAR* _wpgmptr = NULL; 
#endif

//
// A structure to hold information we communicate to Watson.
//

typedef struct DwSyncObjects_s
{
    HANDLE hEventAlive;
    HANDLE hEventDone;
    HANDLE hMutex;
    HANDLE hFileMap;
    DWSharedMem *pDWSharedMem;
} DWSYNCOBJECTS, *LPDWSYNCOBJECTS;

//
// Static fields to hold information that should be passed to Watson. Yes,
// this means that it's shared by the entire process, but when an exception
// occurs, we have no access to a Compiler object or anything, so here we
// are.
//

static bool g_fAdditionalFiles = false;
static WCHAR g_wszAdditionalFiles[MAX_PATH];
static WATSON_TYPE g_WatsonType = WATSON_Prompt;
static LCID g_WatsonLcid = -1;
static bool g_WatsonDisplayed = false;

//============================================================================
// Set the process-wide behavior of Watson. Default is to prompt.
//============================================================================
void SetWatsonType(WATSON_TYPE WatsonType, LCID WatsonLcid, _In_opt_z_ WCHAR *wszAdditionalFiles)
{
    g_WatsonType = WatsonType;
    g_WatsonLcid = WatsonLcid;

    if (wszAdditionalFiles)
    {
        g_wszAdditionalFiles[0] = '\0';
        g_fAdditionalFiles = true;
        StringCchCopyW(g_wszAdditionalFiles, DIM(g_wszAdditionalFiles), wszAdditionalFiles);
    }
}

//============================================================================
// Find the full path to dw.exe
//============================================================================
bool GetDWFullPath(_In_opt_count_(cchDwInstallDir)WCHAR *wszDwInstallDir, int cchDwInstallDir)
{
    DWORD lRes = 0;
    HKEY hkey = 0;
    DWORD dw = 0;
    DWORD dwSize = 0;

    // return when szDWOmstallDir is not being alloted.
    if (wszDwInstallDir != NULL && cchDwInstallDir >= MAX_PATH)
    {
        // Try getting full path from the registry.
        //
        // We can assume the W versions in this function because DW20 only
        // works on Unicode platforms (enforced in WatsonFilter).
        //
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, WATSON_SUBPATH,
                          0, KEY_READ, &hkey) == ERROR_SUCCESS)
        {
            dwSize = cchDwInstallDir * sizeof(wszDwInstallDir[0]);
            dw = RegQueryValueExW(hkey, WATSON_INSTALLED_REG_VAL_W,
                                  NULL, NULL, (LPBYTE)wszDwInstallDir, &dwSize);
            RegCloseKey(hkey);

            if (dw == ERROR_SUCCESS)
            {
                if (GetFileAttributesW(wszDwInstallDir) != DWORD(~0))
                {
                    // dw.exe is in szDwInstallDir
                    return true;
                }
            }
        }
    }

    return false;
}

//============================================================================
// Get the LCID for Watson. First, we check for an explicit LCID in the
// command-line compiler. If that's not set, we fall back to the VS regkey.
//============================================================================
void GetDWLCID(DWSharedMem *pDWSharedMem)
{
    HKEY hkey = 0;
    DWORD dw = 0;
    DWORD regType = 0;
    DWORD data = 0;
    DWORD cbData = sizeof(DWORD);

    if (g_WatsonLcid != -1)
    {
        pDWSharedMem->lcidUI = g_WatsonLcid;
        return;
    }

    // try getting full path from the registry
    //
    // We can assume the W versions in this function because DW20 only
    // works on Unicode platforms (enforced in WatsonFilter).
    //
    if (RegOpenKeyExW(HKEY_CURRENT_USER, LREGKEY_VISUALSTUDIOROOT L"\\General",
                      0, KEY_READ, &hkey) == ERROR_SUCCESS)
    {
        dw = RegQueryValueExW(hkey, L"UILanguage", NULL, &regType,(LPBYTE) &data , &cbData);
        RegCloseKey(hkey);

        // Try our best to ensure that the data is correct
        if (dw == ERROR_SUCCESS && cbData == sizeof(DWORD) && regType == REG_DWORD)
        {
            pDWSharedMem->lcidUI = data;
        }
        else
        {
            //If we cannot get the LCID, let just set it to ENU
            pDWSharedMem->lcidUI = 1033;
        }
    }
    else
    {
        // If we cannot get the LCID, let just set it to ENU
        pDWSharedMem->lcidUI = 1033;
    }
}

//============================================================================
// Release all of the objects in a DWSYNCOBJECTS struct
//============================================================================
void ReleaseDWSyncObjects(LPDWSYNCOBJECTS pDWSyncObjects)
{
    if (pDWSyncObjects->hEventAlive)
        CloseHandle(pDWSyncObjects->hEventAlive);

    if (pDWSyncObjects->hEventDone)
        CloseHandle(pDWSyncObjects->hEventDone);

    if (pDWSyncObjects->hMutex)
        CloseHandle(pDWSyncObjects->hMutex);

    if (pDWSyncObjects->pDWSharedMem)
        UnmapViewOfFile(pDWSyncObjects->pDWSharedMem);

    if (pDWSyncObjects->hFileMap)
        CloseHandle(pDWSyncObjects->hFileMap);
}

//============================================================================
// Initializes a DWSYNCOBJECTS struct
//
// Returns false on an error, else true
//============================================================================
bool InitDWSyncObjects(LPDWSYNCOBJECTS pDWSyncObjects, LPEXCEPTION_POINTERS pExceptionPointers)
{
    SECURITY_ATTRIBUTES SecurityAttributes;
    DWSharedMem *pDWSharedMem;
    HANDLE hProcDW;

    // Setup a default SECURITY_ATTRIBUTES struct
    memset(&SecurityAttributes, 0, sizeof(SECURITY_ATTRIBUTES));
    SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    SecurityAttributes.bInheritHandle = TRUE;

    // Create the shared mem, in the form of mem mapped file, to pass to DW
    pDWSyncObjects->hFileMap = CreateFileMapping(
                        INVALID_HANDLE_VALUE,
                        &SecurityAttributes,
                        PAGE_READWRITE,
                        0,
                        sizeof(DWSharedMem),
                        NULL);

    if (!pDWSyncObjects->hFileMap)
    {
        VSFAIL("Memory mapped file creation failed");
        ReleaseDWSyncObjects(pDWSyncObjects);
        return false;
    }

    pDWSharedMem = static_cast<DWSharedMem*>(MapViewOfFile(
                        pDWSyncObjects->hFileMap,
                        FILE_MAP_READ | FILE_MAP_WRITE,
                        0,
                        0,
                        0));

    if (!pDWSharedMem)
    {
        VSFAIL("Memory mapped file view failed");
        ReleaseDWSyncObjects(pDWSyncObjects);
        return false;
    }

    pDWSyncObjects->pDWSharedMem = pDWSharedMem;

    // get sync objects
    pDWSyncObjects->hEventAlive = CreateEvent(&SecurityAttributes, FALSE, FALSE, NULL);
    pDWSyncObjects->hEventDone = CreateEvent(&SecurityAttributes, FALSE, FALSE, NULL);
    pDWSyncObjects->hMutex = CreateMutex(&SecurityAttributes, FALSE, NULL);

    // Setup interface structure.
#pragma prefast(suppress: 26010, "MapViewOfFile actually maps the allocated size in this case")
    memset(pDWSharedMem, 0, sizeof(DWSharedMem));

    // NOTE:  The duplicated handle does not need to be closed
    if (!DuplicateHandle(GetCurrentProcess(),
                         GetCurrentProcess(),
                         GetCurrentProcess(),
                         &hProcDW,
                         PROCESS_ALL_ACCESS,
                         TRUE,
                         0))
    {
        VSFAIL("Handle duplication failed");
        ReleaseDWSyncObjects(pDWSyncObjects);
        return false;
    }

    if (!hProcDW)
    {
        ReleaseDWSyncObjects(pDWSyncObjects);
        VSFAIL("Duplicated process handle is NULL");
        return false;
    }

    if (!pDWSyncObjects->hEventAlive || !pDWSyncObjects->hEventDone || !pDWSyncObjects->hMutex)
    {
        VSFAIL("Event creation failed");
        ReleaseDWSyncObjects(pDWSyncObjects);
        return false;
    }

    // REQUIRED. These are all based on stuff we have done above or basic process info.
    pDWSharedMem->dwSize = sizeof(DWSharedMem);
    pDWSharedMem->dwVersion = DW_CURRENT_VERSION;
    pDWSharedMem->pid = GetCurrentProcessId();
    pDWSharedMem->tid = GetCurrentThreadId();
    pDWSharedMem->eip = pExceptionPointers ? ((DWORD_PTR) pExceptionPointers->ExceptionRecord->ExceptionAddress) : NULL;
    pDWSharedMem->pep = pExceptionPointers;
    pDWSharedMem->hEventDone = pDWSyncObjects->hEventDone;  // local reference, we use hEventDone for closing later.
    pDWSharedMem->hEventAlive = pDWSyncObjects->hEventAlive;  // local reference, we use hEventAlive for closing later.
    pDWSharedMem->hMutex = pDWSyncObjects->hMutex;   // local reference, we use hMutex for closing later.
    pDWSharedMem->hProc = hProcDW;

    // OPTIONAL. A wide  array of flags can be set here. See client integrators' document for details.
#if IDE 
    if(!GetCompilerPackage())
    {
        pDWSharedMem->bfDWRFlags = fDwrForceToAdminQueue | fDwrForceOfflineMode;
        pDWSharedMem->bfDWUFlags = fDwuNoQueueUI;
    }
    else
    {
        switch (g_WatsonType)
        {
        case WATSON_Prompt:
            break;
        case WATSON_Send:
            pDWSharedMem->bfDWUFlags |= fDwuNoQueueUI;
            pDWSharedMem->bfDWLFlags |= fDwlResponseLog;
            break;
        default:
            VSFAIL("Bad reporting option.");
            __fallthrough;
        case WATSON_Queue:
            pDWSharedMem->bfDWRFlags |= fDwrForceToAdminQueue;
            pDWSharedMem->bfDWUFlags |= fDwuNoEventUI;
            pDWSharedMem->bfDWLFlags |= fDwlResponseLog;
        }
    }
#else
        switch (g_WatsonType)
        {
        case WATSON_Prompt:
            break;
        case WATSON_Send:
            pDWSharedMem->bfDWUFlags |= fDwuNoQueueUI;
            pDWSharedMem->bfDWLFlags |= fDwlResponseLog;
            break;
        default:
            VSFAIL("Bad reporting option.");
            __fallthrough;
        case WATSON_Queue:
            pDWSharedMem->bfDWRFlags |= fDwrForceToAdminQueue;
            pDWSharedMem->bfDWUFlags |= fDwuNoEventUI;
            pDWSharedMem->bfDWLFlags |= fDwlResponseLog;
        }
#endif
    pDWSharedMem->bfDWEFlags = 0;
    pDWSharedMem->bfDWMFlags = 0;

    // Lang ID in use
    GetDWLCID(pDWSharedMem);

    // REQUIRED. Here we choose to see a Restart checkbox. If you don't want it, just send msoctdsQuit.
    // You could send msoctdsQuit | msoctdsRecover if you want to show "Recover my work and restart ..."
    // You don't need to set msoctdsDebug -- DW will look for a JIT and show it itself if needed (unless fDweIgnoreAeDebug is set)
    pDWSharedMem->bfmsoctdsOffer = msoctdsRecover;

    pDWSharedMem->bfmsoctdsNotify = 0;

    // OPTIONAL. Testcrash only wants to hear back if the user hit Quit (or Debug, which doesn't get specified directly).
    // Otherwise, kill the process (in our case, since we offer Restart, DW will do that for us).
    // You may not want to hear back ever, in which case this should be left as 0.
    pDWSharedMem->bfmsoctdsLetRun |= msoctdsRecover;

    // REQUIRED. Since DW 2.0 only runs on Win2K and up, we can feel confident about using W apis directly.
    GetModuleFileNameW(NULL, pDWSharedMem->wzModuleFileName, DW_MAX_PATH);

    // Everybody uses IE's PID to identify unique users
    StringCchCopyA(pDWSharedMem->szPIDRegKey,
                   DIM(pDWSharedMem->szPIDRegKey),
                   "HKLM\\Software\\Microsoft\\Internet Explorer\\Registration\\DigitalProductID");

#if IDE
    const WCHAR wzDotDataDllList[] = L"msvbide.dll\0";

    VSASSERT(DIM(wzDotDataDllList) <= DIM(pDWSharedMem->wzDotDataDlls), "");
    memcpy(pDWSharedMem->wzDotDataDlls, wzDotDataDllList, sizeof(wzDotDataDllList));
#endif

    // Additional Files
    if (g_fAdditionalFiles) {
        StringCchCopyW(pDWSharedMem->wzFilesToKeep,
            DIM(pDWSharedMem->wzFilesToKeep),
            g_wszAdditionalFiles);
    }

    if (!SUCCEEDED(ResLoadString(STRID_WatsonAppName,
                                  pDWSharedMem->uib.wzGeneral_AppName,
                                  DIM(pDWSharedMem->uib.wzGeneral_AppName))))
    {
        StringCchCopyW(pDWSharedMem->uib.wzGeneral_AppName,
            DIM(pDWSharedMem->uib.wzGeneral_AppName),
            L"Microsoft (R) Visual Basic Compiler");
    }

    StringCchCopyW(pDWSharedMem->wzEventLogSource,
                   DIM(pDWSharedMem->wzEventLogSource),
                   pDWSharedMem->uib.wzGeneral_AppName);

    if (!SUCCEEDED(ResLoadString(STRID_WatsonError,
                                  pDWSharedMem->uib.wzMain_Intro_Reg,
                                  DIM(pDWSharedMem->uib.wzMain_Intro_Reg))))
    {
        StringCchCopyW(pDWSharedMem->uib.wzMain_Intro_Reg,
            DIM(pDWSharedMem->uib.wzMain_Intro_Reg),
            L"No information has been lost. Check the compiler output for possible ways to avoid this error.");
    }

    return true;
}

//============================================================================
// Creates the DW.exe process, passing it the DW shared mem
//============================================================================
bool CreateDWProcess(LPDWSYNCOBJECTS pDWSyncObjects)
{
    STARTUPINFOW StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    WCHAR wszDWExePath[MAX_PATH];
    WCHAR wszCommandLine[MAX_PATH*2];

    // Setup basic STARTUPINFO for CreateProcess
    memset(&StartupInfo, 0, sizeof(STARTUPINFOW));
    StartupInfo.cb = sizeof(STARTUPINFOW);
    memset(&ProcessInformation, 0, sizeof(PROCESS_INFORMATION));

    if (!GetDWFullPath(wszDWExePath, DIM(wszDWExePath)))
        return false;

    // Create the command line
    HRESULT hr = S_OK;

#ifdef _WIN64
    hr = StringCchPrintf(wszCommandLine,
            DIM(wszCommandLine),
            L"\"%s\" -x -s %I64u",
            wszDWExePath,
            (unsigned __int64) pDWSyncObjects->hFileMap);
#else
    hr = StringCchPrintf(wszCommandLine,
            DIM(wszCommandLine),
            L"\"%s\" -x -s %u",
            wszDWExePath,
            (DWORD)pDWSyncObjects->hFileMap);
#endif // _WIN64

    if (FAILED(hr))
    {
        return false;
    }

    // Create the DW process
    if (CreateProcessW(
         wszDWExePath,
         wszCommandLine,
         NULL,
         NULL,
         TRUE,
         CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS,
         NULL,
         NULL,
         &StartupInfo,
         &ProcessInformation))
    {
        BOOL bDWRunning = TRUE;

        while (bDWRunning)
        {
            if (WAIT_OBJECT_0 == WaitForSingleObject(
                 pDWSyncObjects->hEventAlive,
                 DW_TIMEOUT_VALUE))
            {
                if (WaitForSingleObject(pDWSyncObjects->hEventDone, 1) == WAIT_OBJECT_0)
                {
                    bDWRunning = FALSE;
                    return true;
                }
                continue;
            }

            // we timed-out waiting for DW to respond, try to quit
            DWORD dw = WaitForSingleObject(pDWSyncObjects->hMutex, DW_TIMEOUT_VALUE);

            if (WAIT_TIMEOUT == dw)
            {
                // either DW's hung or crashed, we must carry on
                bDWRunning = FALSE;
            }
            else if (WAIT_ABANDONED == dw)
            {
                bDWRunning = FALSE;

                ReleaseMutex(pDWSyncObjects->hMutex);
            }
            else
            {
                // See if DW has woken up?
                if (WaitForSingleObject(pDWSyncObjects->hEventAlive, 1) != WAIT_OBJECT_0)
                {
                    // tell DW we're through waiting for it
                    SetEvent(pDWSyncObjects->hEventDone);
                    bDWRunning = FALSE;
                }
                else
                {
                    // check for done
                    if(WaitForSingleObject(pDWSyncObjects->hEventDone, 1) == WAIT_OBJECT_0)
                    {
                        bDWRunning = FALSE;
                    }
                }

                ReleaseMutex(pDWSyncObjects->hMutex);
            }
        }

        CloseHandle(ProcessInformation.hProcess);
        CloseHandle(ProcessInformation.hThread);

        return true;
    }

    return false;
}

//============================================================================
// Calls vsw.exe in the event of an unhandled Win32 structured exception
//============================================================================
LONG WINAPI WatsonFilter(EXCEPTION_POINTERS * pExceptionPointers)
{
    DWSYNCOBJECTS DWSyncObjects;
    DWORD dwResult = 0;
    bool fSucceeded = false;

    // This should never fire
    VSASSERT(NULL != pExceptionPointers,
        "Null pointer to exception structure in unhandled exception filter");

    /*
     * If it's a breakpoint, running under the debugger, or pExceptionPointers
     * is NULL, then let the standard windows exception dialog come up; otherwise
     * invoke DW
     */
    if (NULL == pExceptionPointers || TRUE == IsDebuggerPresent() ||
        EXCEPTION_BREAKPOINT == pExceptionPointers->ExceptionRecord->ExceptionCode)
    {
#if DEBUG
        if (VSFSWITCH(fExceptionNoRethrowNoWatson))
        {
            return EXCEPTION_EXECUTE_HANDLER;   // Handle the exception so it doesn't get rethrown to the next caller.
        }
#endif
        return EXCEPTION_CONTINUE_SEARCH;
    }

    if (g_WatsonType == WATSON_None || !W_IsUnicodeSystem())
    {
        return EXCEPTION_EXECUTE_HANDLER;
    }

#if !IDE && !HOSTED

    //For Vista, the command line compiler should use WER.dll. Check for availability.
    // Try to launch WER-style report.

    if (ERROR_SUCCESS == LoadWerDll ())
    {
        HRESULT hr = E_FAIL;
        DWORD dwReportingFlags = 0; //Microsoft figure out what these should be set to. m_pDWSharedMem->bfDWRFlags;
        WER_SUBMIT_RESULT wer_result;

        hr = CreateAndSubmitReport (&wer_result, pExceptionPointers, g_wszAdditionalFiles, g_WatsonType);

        if (SUCCEEDED(hr) && (wer_result == WerReportDebug))
        {
            //Microsoft change this so that our outer filter will know to
            //return CONTINUE_SEARCH.
            pExceptionPointers->ExceptionRecord->ExceptionCode = EXCEPTION_BREAKPOINT;
            return EXCEPTION_CONTINUE_SEARCH;
        }
        else
        {
            return EXCEPTION_EXECUTE_HANDLER;
        }
    }
    else
    {
        // VSWhidbey DCR[406053] Honor /errorreport:send based on Watson API ErrorReportingCheckOptIn() check.
        if (g_WatsonType == WATSON_Send && !ErrorReportingCheckOptIn())
        {
            return EXCEPTION_EXECUTE_HANDLER;
        }
    }

#endif


    if (InitDWSyncObjects(&DWSyncObjects, pExceptionPointers))
    {
        fSucceeded = CreateDWProcess(&DWSyncObjects);

        // store the result, since it will be destroy by DwReleaseSyncObjects
        dwResult = DWSyncObjects.pDWSharedMem->msoctdsResult;

        ReleaseDWSyncObjects(&DWSyncObjects);
    }

    if (!fSucceeded || !(dwResult & msoctdsDebug))
    {
        // Do normal handling, even if DW failed.
        return EXCEPTION_EXECUTE_HANDLER;
    }
    else
    {
        // If they want to debug, don't swallow the exception.
        return EXCEPTION_CONTINUE_SEARCH;
    }
}

