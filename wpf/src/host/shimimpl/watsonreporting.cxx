//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      [see .hxx]
//
//  History:
//      2007/07/20   Microsoft     Created
//      2007/09/20   Microsoft     Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#include "precompiled.hxx"
#include "WatsonReporting.hxx"
#include <fxver.h>

#include "osversionhelper.h"

#ifdef APPLICATION_SHIM

using namespace WatsonReportingHelper;

// Curiously, XPSViewer seems capable of coping with all the problems detected here. That's because
// it doesn't use ClickOnce and also because if IPersistMoniker marshaling fails, IE's fallback to 
// IPersistFile still works for XPSViewer.
// That's why all this code is conditionally compiled only for PH.exe.

// HKCU\Software\Classes is persisted in a separate file and on many computers permissons are set
// explicitly, not inherited. This must be somehow part of the explanation of how the ACL entry for the
// individual user account can be missing.
HRESULT CheckRegistryAccess()
{
    HKEY hKey;
    int error = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Classes", 0, KEY_READ, &hKey);
    if (error == ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return S_OK;
    }

    // Dev10 bug 707086 - HKCU\SOFTWARE\Classes may not exist on clean XP installations.
    //                    ClickOnce will create it, so don't bother about it here.
    if (error == ERROR_FILE_NOT_FOUND)
    {
        return S_OK;
    }

    HRESULT hr = HRESULT_FROM_WIN32(error);
    TriggerWatson(ActivationProblemIds::HKCUClassesAccessDenied);
    return hr;
}

// Only read access to %LocalAppData% is tested here. ClickOnce needs access to %LocalAppData%\Deployment. 
// Other profile folders may also be susceptible to the ACL anomaly, but unless we get actual reports of
// such cases, it doesn't make much sense to generalize the test. 
// Worth noting is that denied access to the Temporary Internet Files folder does not break PresentationHost
// activation.
HRESULT CheckAppDataFolderAccess()
{
    wchar_t path[MAX_PATH];
    HRESULT hr = SHGetFolderPath(0, CSIDL_LOCAL_APPDATA, 0, SHGFP_TYPE_CURRENT, path);
    if(hr == S_OK)
    {
        StringCchCat(path, ARRAYSIZE(path), L"\\*");
        WIN32_FIND_DATA findData;
        HANDLE hFind = FindFirstFile(path, &findData);
        if(hFind != INVALID_HANDLE_VALUE)
        {
            FindClose(hFind);
        }
        else if(GetLastError() == ERROR_ACCESS_DENIED)
        {
            TriggerWatson(ActivationProblemIds::AppDataFolderAccessDenied);
            hr = E_ACCESSDENIED;
        }
    }
    return hr;
}

void TriggerWatson(LPCWSTR problemId)
{
    HRESULT hr = S_OK;

    // Invoke Watson in manifest mode:
    // http://msdn2.microsoft.com/en-us/library/bb219076.aspx#MicrosoftErrorReporting_AboutManifestMode
    // We rely on the .NET Framework setup to install dw20.exe.

    #define CRLF L"\r\n"

    const wchar_t report[] = CRLF // required blank line
        L"Version=131072" CRLF
        L"ReportingFlags=2048" CRLF // fDwrCriticalEvent
        L"General_AppName=" TEXT(SHIM_COMPONENT_DESCRIPTION) CRLF
        L"FilesToKeep=null" CRLF
        L"EventType=" ACTIVATION_PROBLEM_WATSON_EVENT CRLF //***
        L"P1=" TEXT(TARGETNAME) CRLF
        L"P2=" TEXT(FX_VER_FILEVERSION_STR) CRLF; 
        //P3=OS version - added below
        //P4=problemId - added below

    HANDLE hFile = 0;
    DWORD cBytesWritten;
    wchar_t DWPath[MAX_PATH];
    DWORD cch;
    wchar_t cmdLine[3*MAX_PATH];
    STARTUPINFO startupInfo = { sizeof STARTUPINFO };
    PROCESS_INFORMATION processInfo;
    DWORD exitCode;

    // Create the report file
    wchar_t reportFilepath[MAX_PATH] = { 0 };
    GetTempPath(ARRAYSIZE(reportFilepath), reportFilepath);
    CHECK_ZERO_FROM_WIN32(GetTempFileName(reportFilepath, L"PH", 0, reportFilepath));
    hFile = CreateFile(reportFilepath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    CHECK_BOOL_FROM_WIN32(hFile && hFile != INVALID_HANDLE_VALUE);
    CHECK_BOOL_FROM_WIN32(WriteFile(hFile, report, sizeof report - sizeof L'\0', &cBytesWritten, 0));
    wchar_t strOSVersion[96];
    SYSTEM_INFO sysInfo;
    GetNativeSystemInfo(&sysInfo);
    StringCchPrintf(strOSVersion, ARRAYSIZE(strOSVersion), 
        L"P3=%d.%d.%d.%d.%d%s" CRLF, 
        OSVersion::GetMajorVersion(), OSVersion::GetMinorVersion(), OSVersion::GetBuildNumber(),
        OSVersion::GetServicePackMajor(), OSVersion::GetServicePackMinor(),
        sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? L" (x64)" : L"");
    WriteFile(hFile, strOSVersion, (DWORD)wcslen(strOSVersion)*2, &cBytesWritten, 0);
    if(problemId)
    {
        WriteFile(hFile, L"P4=", 3*sizeof(wchar_t), &cBytesWritten, 0);
        WriteFile(hFile, problemId, (DWORD)wcslen(problemId)*sizeof(*problemId), &cBytesWritten, 0);
        WriteFile(hFile, CRLF, sizeof CRLF - sizeof L'\0', &cBytesWritten, 0);
    }
    CloseHandle(hFile);

    // Invoke dw20.exe -d <report filepath>
    // Using ShellExecute() would be simpler, but it relies heavily on registry keys, and we have an ACL 
    // problem, so ShellExecute() might fail.

    // Apparently DW is installed only as a 32-bit build.
#ifdef _WIN64
    cch = GetEnvironmentVariable(L"CommonProgramFiles(x86)", DWPath, ARRAYSIZE(DWPath));
#else
    cch = GetEnvironmentVariable(L"CommonProgramFiles", DWPath, ARRAYSIZE(DWPath));
#endif
    CKHR(StringCchCopy(DWPath+cch, ARRAYSIZE(DWPath)-cch, L"\\Microsoft Shared\\DW\\dw20.exe"));
    // Conventionally, the first command line item is the executable's filepath.
    CKHR(StringCchPrintf(cmdLine, ARRAYSIZE(cmdLine), L"\"%s\" -d \"%s\"", DWPath, reportFilepath));
    CHECK_BOOL_FROM_WIN32(CreateProcess(DWPath, cmdLine, 0, 0, 0, 0, 0, 0, &startupInfo, &processInfo));
    WaitForSingleObject(processInfo.hProcess, INFINITE);
    CHECK_BOOL_FROM_WIN32(GetExitCodeProcess(processInfo.hProcess, &exitCode));
    hr = exitCode == 0 ? S_OK : E_FAIL;

Cleanup:
    // No cleanup really needed. Mission failed...

    // Fallback: just raise an exception
    if(FAILED(hr))
    {
        ReportFaultAndTerminate(hr, ACTIVATION_PROBLEM_FALLBACK_EXCEPTION_CODE);
    }
}

#endif // APPLICATION_SHIM
