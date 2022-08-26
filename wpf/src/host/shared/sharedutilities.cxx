//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//    This file contains methods shared between PresentationStartup
//    and PresentationHost
//
//  History
//      2005/5/4-Microsoft
//          Moved browser invoke methods to a shared location
//      2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#include "Precompiled.hxx"
#include "SharedUtilities.hxx"
#include "WatsonReportingShared.hxx"
#include "..\inc\Definitions.hxx"
#include "..\inc\Registry.hxx"

#include <knownfolders.h>
#include <tchar.h>
#include <StrSafe.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>


//*****************************************************************************
// InvokeBrowser
// This function calls ShellExecuteEx to open the provided URL with Internet Explorer
//
// Parameters:
//   __in pwzUrl    the URL to be launched
//
// Return: 
//   S_OK - success
//   otherwise - failed. In typical cases, ShellExecuteEx() displays an error message.
//*****************************************************************************

HRESULT InvokeBrowser(__in LPCWSTR pwzUrl)
{
    EventWriteWpfHostUm_InvokingBrowser(pwzUrl);

    HRESULT hr = S_OK;
    SHELLEXECUTEINFO ShellExecInfo = { 0 };
    LPWSTR pszIEPath = new WCHAR[MAX_PATH];

    CHECK_HR(GetInternetExplorerPath(pszIEPath));

    ShellExecInfo.cbSize = sizeof(ShellExecInfo);
    ShellExecInfo.fMask = SEE_MASK_NOASYNC;
    ShellExecInfo.lpFile = pszIEPath;
    ShellExecInfo.lpParameters = pwzUrl;
    ShellExecInfo.nShow = SW_SHOWDEFAULT;
    
    if (!ShellExecuteEx(&ShellExecInfo))
    {
        CHECK_HR(HRESULT_FROM_WIN32(GetLastError()));
    }

Cleanup:
    delete[] pszIEPath;
    return hr;
}

/**************************************************************************
// GetInternetExplorerPath
//   Helper that finds the full path for IE
//
// Remarks
//   We use PathAppend below instead of the safer PathCchAppend for 
//   Vista and Win7 compatibility.
**************************************************************************/
HRESULT GetInternetExplorerPath(_Out_writes_(MAX_PATH) LPWSTR pszPath)
{
#pragma prefast(push)
#pragma prefast(disable:25025, "We don't know of a better API to use in place of PathAppend. The OACR spreadsheet and MSDN do not suggest any either.")

    HRESULT hr = S_OK;
    BOOL fResult = TRUE;

    BOOL fIsWow64Process = FALSE;
    IsWow64Process(GetCurrentProcess(), &fIsWow64Process);

    if (fIsWow64Process)
    {
        if (!ExpandEnvironmentStringsW(L"%ProgramW6432%\\Internet Explorer", pszPath, MAX_PATH))
        {
            CHECK_HR(HRESULT_FROM_WIN32(GetLastError()));
        }

        CHECK_BOOL(PathAppend(pszPath, L"IEXPLORE.EXE"));
    }
    else
    {
        PWSTR pszProgramFilesPath = nullptr;

        CHECK_HR(SHGetKnownFolderPath(FOLDERID_ProgramFiles, 0, nullptr, &pszProgramFilesPath));

        hr = StringCchCopyW(pszPath, MAX_PATH, pszProgramFilesPath);
        CoTaskMemFree(pszProgramFilesPath);
        CHECK_HR(hr);

        CHECK_BOOL(PathAppend(pszPath, L"Internet Explorer"));
        CHECK_BOOL(PathAppend(pszPath, L"IEXPLORE.EXE"));
    }

Cleanup:

    if (!SUCCEEDED(hr) || !fResult)
    {
        StringCchCopy(pszPath, 1, L"");
    }

    if (SUCCEEDED(hr) && !fResult)
    {
        hr = E_FAIL;
    }

    return hr;

#pragma prefast(pop)
}

/**************************************************************************
   QueryService - helper that queries the browser for a service
**************************************************************************/
STDMETHODIMP
QueryService( __in_ecount(1) IUnknown* pUnk,
              __in_ecount(1) REFGUID guidService,
              __in_ecount(1) REFIID iid,
              __deref_out_ecount(1) void ** pResult)
{
    HRESULT hr = S_OK;
    ::IServiceProvider * pSP = NULL;

    CK_PARG(pResult);
    *pResult = NULL;
    CK_PARG(pUnk);

    CKHR(pUnk->QueryInterface(IID_IServiceProvider, (void**)&pSP));
    CKHR(pSP->QueryService(guidService, iid, pResult));
Cleanup:
    if (pSP)
    {
        pSP->Release();
    }
    return hr;
}

#if DEBUG

#define MAX_DEBUG_STRING       1024
#define ASSERT_BANNER_STRING   _T("************************************************************")

DWORD g_dwDebugLevel = 0; 

//*************************************************************************
// Author:   twillie
// Created:  02/17/2005
// Abstract: This function sets the debug level.
// Parameters:
//   pszModule - [in] String used to look up section in win.ini.
// Return: void
//*************************************************************************
void DebugInit(__in LPTSTR pszModule)
{
    // Set Global Debug Level
    g_dwDebugLevel = GetProfileInt(pszModule, _T("debug"), 0);
} // DebugInit


void DebugInit(__in DWORD dwLevel)
{
    // Set Global Debug Level
    g_dwDebugLevel = dwLevel;
} // DebugInit

//*************************************************************************
// Author:   twillie
// Created:  02/17/2005
// Abstract: This function is dumps string.  It checks level to see if debug
//           out is allowed for level.
// Parameters:
//   dwLevel - [in] debug level
//   psz - [in] formatted string
//   ... - [in] variable arguments for formatted string
// Return: void
//*************************************************************************
void DebugPrintf(__in DWORD dwLevel, __in LPTSTR pszFormat, ...)
{
    va_list ap;
    TCHAR   szBuffer[MAX_DEBUG_STRING];

    // check to see if we accept this level.  Otherwise return.
    if (dwLevel > g_dwDebugLevel)
    {
        return;
    }
    
    // setup variable args for formatted string
    va_start(ap, pszFormat);
    
    // format string
    ::StringCchVPrintf(szBuffer, MAX_DEBUG_STRING, pszFormat, ap);

    // output string with return.
    OutputDebugString(szBuffer);
    OutputDebugString(_T("\n"));
    
    // cleanup variable args
    va_end(ap);
} // DebugPrintf


//*************************************************************************
// Author:   twillie
// Created:  02/17/2005
// Abstract: Custom assert function.  Leverages DPF.  NO UI.
// Parameters:
//   pszFile - [in] file where assert was fired from
//   nLine - [in] line number in file
//   pszCondition - [in] condition which tripped assert
// Return: void
//*************************************************************************
void 
DebugAssert(__in LPTSTR pszFile, int nLine, __in LPTSTR pszCondition)
{
    TCHAR szBuffer[MAX_DEBUG_STRING];

    // Build the debug stream message.
    ::StringCchPrintf(szBuffer, 
                      MAX_DEBUG_STRING, 
                      _T("ASSERTION FAILED! File %s Line %d: %s"), 
                      pszFile, 
                      nLine, 
                      pszCondition);

    // Output the message
    DebugPrintf(0, ASSERT_BANNER_STRING);
    DebugPrintf(0, szBuffer);
    DebugPrintf(0, ASSERT_BANNER_STRING);
} // DebugAssert()
#endif // DEBUG


//******************************************************************************
//
// TerminateIfNotConnectedCallback()
//
// This is is a callback that will cause the current process to terminate if
// the wait timed out. 
//
//******************************************************************************

VOID CALLBACK TerminateIfNoHostCallback(PVOID, BOOL)
{
    // Unlike a wait callback, this will not get called if the timer event
    // is deleted.
    if(!IsDebuggerPresent())
    {
        TerminateProcess(GetCurrentProcess(), ERROR_NO_HOST);
    }
}

//******************************************************************************
//
// TerminateIfNoHost()
//
// This method sets up a TimerQueue event. If the event fires before it can be
// cancelled, that means that the activation took too long.
//   
//******************************************************************************

HANDLE TerminateIfNoHost(DWORD dwWaitTimeSeconds)
{
    HANDLE hNoHostTimer = 0;

    if (CreateTimerQueueTimer(
        &hNoHostTimer,
        NULL,
        (WAITORTIMERCALLBACK) TerminateIfNoHostCallback, 
        NULL,
        dwWaitTimeSeconds * 1000, 
        0,
        WT_EXECUTEONLYONCE))
    {
        return hNoHostTimer;
    }
    else
    {
        return NULL;
    }
}

//******************************************************************************
//
// CancelTerminateIfNoHost()
//
// This method will set the _hConnectedEvent, which will cause the
// TerminateIfNoHost to complete without terminating.
//   
//******************************************************************************

HRESULT CancelTerminateIfNoHost(HANDLE hNoHostTimer)
{
    HRESULT hr = S_OK;

    if (hNoHostTimer)
    {
        CHECK_BOOL_FROM_WIN32(DeleteTimerQueueTimer(NULL, hNoHostTimer, NULL));
    }

Cleanup:
    return hr;
}

//******************************************************************************
//
// GetRegistryDWORD()
//
// Get a single DWORD from the registry. This is not optimized for calling this
// multiple times on the hive. Returns S_FALSE if the key or value was not found.
//   
//******************************************************************************

HRESULT GetRegistryDWORD(__in HKEY hKey, __in LPWSTR pswzSubKey, __in LPWSTR pswzName, __out DWORD& dwValue, __in DWORD dwDefaultValue)
{
    HRESULT hr = S_OK;
    HKEY hNewKey = 0;

    dwValue = dwDefaultValue;
    CK_PARG(pswzSubKey);
    CK_PARG(pswzName);

    int error = RegOpenKeyEx(hKey, pswzSubKey, 0, KEY_READ, &hNewKey);
    if (error == ERROR_FILE_NOT_FOUND)
    {
        hr = S_FALSE;
        goto Cleanup; // This is OK; the key+subkey was not found
    }
    CHECK_BOOL_FROM_WIN32(error == ERROR_SUCCESS);

    DWORD dwSize = sizeof(dwValue);
    DWORD dwType = REG_DWORD;
    error = RegQueryValueEx(hNewKey, pswzName, NULL, &dwType, (LPBYTE) &dwValue, &dwSize);
    if (error == ERROR_FILE_NOT_FOUND)
    {
        hr = S_FALSE;
        goto Cleanup; // This is OK; the value was not found
    }
    CHECK_BOOL_FROM_WIN32(error == ERROR_SUCCESS);

Cleanup:
    if (hNewKey)
    {
        RegCloseKey(hNewKey);
    }

    return hr;
}

//******************************************************************************
//
// GetRegistryString()
//
// Get a single string from the registry. This is not optimized for calling this
// multiple times on the hive. Returns S_FALSE if the key or value was not found.
//   
//******************************************************************************

HRESULT GetRegistryString(__in HKEY hKey, __in LPWSTR pswzSubKey, __in LPWSTR pswzName, CString& strValue)
{
    HRESULT hr = S_OK;
    HKEY hNewKey = 0;

    strValue.SetValue(NULL);
    CK_PARG(pswzSubKey);
    CK_PARG(pswzName);

    int error = RegOpenKeyEx(hKey, pswzSubKey, 0, KEY_READ, &hNewKey);
    if (error == ERROR_FILE_NOT_FOUND)
    {
        hr = S_FALSE;
        goto Cleanup; // This is OK; the key+subkey was not found
    }
    CHECK_BOOL_FROM_WIN32(error == ERROR_SUCCESS);

    WCHAR wzBuffer[INTERNET_MAX_URL_LENGTH];

    DWORD dwSize = INTERNET_MAX_URL_LENGTH * sizeof(WCHAR);
    DWORD dwType = REG_SZ;
    error = RegQueryValueEx(hNewKey, pswzName, NULL, &dwType, (LPBYTE) wzBuffer, &dwSize);
    if (error == ERROR_FILE_NOT_FOUND || dwType != REG_SZ)
    {
        hr = S_FALSE;
        goto Cleanup; // This is OK; the value was not found
    }

    CHECK_BOOL_FROM_WIN32(error == ERROR_SUCCESS);

    strValue.SetValue(wzBuffer);

Cleanup:
    if (hNewKey)
    {
        RegCloseKey(hNewKey);
    }

    return hr;
}

HRESULT SetRegistryString( __in HKEY hKey, __in LPCWSTR pswzSubKey, __in LPCWSTR pswzName, CString& strValue )
{
    ASSERT(hKey && pswzSubKey && *pswzSubKey && pswzName && *pswzName);

    HRESULT hr = S_OK;

    //Create or open the requested subkey.
    HKEY openedKey = 0;
    LONG openResult = RegCreateKeyEx(
        hKey,
        pswzSubKey,
        0, // must be zero
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        NULL,
        &openedKey,
        NULL);

    CHECK_BOOL_FROM_WIN32(openResult == ERROR_SUCCESS);

    //Calculate the size of the string in bytes, including the null terminator.  This is the
    //length of the string + 1 times the size of a single character.
    size_t cbString = ((strValue.Length()) + 1) * sizeof(WCHAR); //+1 for null-terminator

    openResult = RegSetValueEx(
        openedKey,
        pswzName,
        0, //reserved, must be zero
        REG_SZ,
        (LPBYTE)strValue.GetValue(),
        (DWORD)cbString );

    CHECK_BOOL_FROM_WIN32(openResult == ERROR_SUCCESS);

Cleanup:
    if (openedKey)
    {
        RegCloseKey(openedKey);
    }

    return hr;
}

// This will convert from c:\... to file://c:/... While an ApplicationIdentity can be
// created from a path, it will not work because ClickOnce needs the file:-scheme for
// consistency. Is a NOP if it was not a path.
// WARNING: This must be done BEFORE StripUri or else the result will be mangled.
HRESULT ConvertPathToFileScheme(__in LPCWSTR pwzUri, __in CString& strCanonicalUri)
{
    WCHAR wzUri[INTERNET_MAX_URL_LENGTH + 1];
    DWORD dwLength = INTERNET_MAX_URL_LENGTH;

    HRESULT hr = S_OK;
    
    strCanonicalUri.SetValue(NULL);

    CK_PARG(pwzUri);

    hr = CoInternetParseUrl(
        pwzUri,
        PARSE_URL_FROM_PATH,
        0,
        wzUri,
        INTERNET_MAX_URL_LENGTH + 1,
        &dwLength,
        0);

    if (hr == S_OK)
    {
        strCanonicalUri.SetValue(wzUri);
    }
    else
    {
        strCanonicalUri.SetValue(pwzUri);
        hr = S_OK;
    }

Cleanup:
    return hr;
}

// StripUri
// Strips user name, password, query and fragment from a URI so that we can make an
// ApplicationIdentity out of it, since GetDeploymentDataFromManifest doesn't do that
// itself.
// WARNING: This must be done AFTER ConvertPathToFileScheme or the resulting URI will be mangled

HRESULT StripUri(__in LPCWSTR pUri, __in CString& strCleanUri)
{
    HRESULT hr = S_OK;

    strCleanUri.SetValue(NULL);

    WCHAR wzScheme[INTERNET_MAX_SCHEME_LENGTH + 1];
    WCHAR wzHostName[INTERNET_MAX_HOST_NAME_LENGTH + 1];
    WCHAR wzUserName[INTERNET_MAX_USER_NAME_LENGTH + 1];
    WCHAR wzPassword[INTERNET_MAX_PASSWORD_LENGTH + 1];
    WCHAR wzUrlPath[INTERNET_MAX_PATH_LENGTH + 1];
    WCHAR wzExtraInfo[INTERNET_MAX_PATH_LENGTH + 1];

    URL_COMPONENTS components = {
        sizeof(components),
        wzScheme, INTERNET_MAX_SCHEME_LENGTH,
        INTERNET_SCHEME_UNKNOWN, // Scheme int value
        wzHostName, INTERNET_MAX_HOST_NAME_LENGTH,
        0, // Port
        wzUserName, INTERNET_MAX_USER_NAME_LENGTH,
        wzPassword, INTERNET_MAX_PASSWORD_LENGTH,
        wzUrlPath, INTERNET_MAX_PATH_LENGTH,
        wzExtraInfo, INTERNET_MAX_PATH_LENGTH};

    CK_PARG(pUri);

    size_t length;
    CKHR(StringCchLengthW(pUri, INTERNET_MAX_URL_LENGTH, &length));

    if (!InternetCrackUrl(pUri, (DWORD) length, 0, &components))
    {
        CKHR(HRESULT_FROM_WIN32(GetLastError()));
    }

    WCHAR wzUri[INTERNET_MAX_URL_LENGTH + 1];
    DWORD dwLength = INTERNET_MAX_URL_LENGTH;

    if (components.nScheme == INTERNET_SCHEME_FILE)
    {
        CKHR(CoInternetParseUrl(
            components.lpszUrlPath,
            PARSE_URL_FROM_PATH,
            0,
            wzUri,
            INTERNET_MAX_URL_LENGTH + 1,
            &dwLength,
            0));
    }
    else
    {
        // For any scheme that is not FILE
        // Remove things that we don't want in our cleaned URI

        components.dwExtraInfoLength = 0;
        components.lpszExtraInfo = NULL;
        components.dwUserNameLength = 0;
        components.lpszUserName = NULL;
        components.dwPasswordLength = 0;
        components.lpszPassword = NULL;

        if (!InternetCreateUrl(&components, ICU_ESCAPE, wzUri, &dwLength))
        {
            CKHR(HRESULT_FROM_WIN32(GetLastError()));
        }
    }

    strCleanUri.SetValue(wzUri);

Cleanup:
    return hr;
}


typedef HRESULT (*GetDeploymentDataFromManifestPfn)(
            __in LPCWSTR pcwzActivationUrl, __in LPCWSTR pcwzPathToDeploymentManifest,
            __in_ecount_opt(*pdwIdentityBufferLength) LPWSTR pwzApplicationIdentity, __in_ecount(1) LPDWORD pdwIdentityBufferLength,
            __in_ecount_opt(*pdwArchitectureBufferLength) LPWSTR pwzProcessorArchitecture, __in_ecount(1) LPDWORD pdwArchitectureBufferLength,
            __in_ecount_opt(*pdwCodebaseBufferLength) LPWSTR pwzApplicationManifestCodebase, __in_ecount(1) LPDWORD pdwCodebaseBufferLength,
            __in_ecount_opt(*pdwProviderBufferLength) LPWSTR pwzDeploymentProvider, __in_ecount(1) LPDWORD pdwProviderBufferLength);

#define DFSHIMDLL L"dfshim.dll"
#define DFSHIMFN "GetDeploymentDataFromManifest"
#define APPID_LENGTH INTERNET_MAX_URL_LENGTH + 1
#define PROCARCH_LENGTH 64
#define APPMAN_LENGTH INTERNET_MAX_URL_LENGTH + 1
#define DEPPRO_LENGTH INTERNET_MAX_URL_LENGTH + 1

// GetDeploymentDataFromManifest
// A wrapper around the call to the ClickOnce API in dfshim.dll.

HRESULT GetDeploymentDataFromManifest(
    __in LPCWSTR pwzDeploymentUri,
    __in_opt LPCWSTR pwzDeploymentManifestPath,
    __in CString& strApplicationIdentity,
    __in CString& strArchitecture,
    __in CString& strCodebase,
    __in CString& strProvider
    )
{
    EventWriteWpfHostUm_ReadingDeplManifestStart(pwzDeploymentManifestPath ? pwzDeploymentManifestPath : pwzDeploymentUri);
    
    HRESULT hr = S_OK;
    HINSTANCE hInstance = NULL;
    WIN32_FIND_DATA fileData;
    HANDLE hFile = INVALID_HANDLE_VALUE;

    strApplicationIdentity.SetValue(NULL);
    strArchitecture.SetValue(NULL);
    strCodebase.SetValue(NULL);
    strProvider.SetValue(NULL);

    CString strPath;
    CString strCleanUri;
    CString strCanonicalUri;

    CK_PARG(pwzDeploymentUri);


    // Convert to file: scheme has to be done first...
    CKHR(ConvertPathToFileScheme(pwzDeploymentUri, strCanonicalUri));
    // ...otherwise StripUri mangles the uri if it was a path
    CKHR(StripUri(strCanonicalUri.GetValue(), strCleanUri));

    // If the path was not specified, then we need to get it.
    if (pwzDeploymentManifestPath == NULL)
    {
        // 



        ASSERT(false);
        CKHR(E_INVALIDARG);
        /*
        WCHAR wzCacheFileName[MAX_PATH];
        // Note: URLDownloadToCacheFile's 4th param is number of characters, not number of bytes as 
        // stated in the SDK.
        CKHR(URLDownloadToCacheFile(NULL, pwzDeploymentUri, wzCacheFileName, MAX_PATH, 0, NULL));
        strPath.SetValue(wzCacheFileName);
        */
    }
    else
    {
        strPath.SetValue(pwzDeploymentManifestPath);
    }

    // Check to make sure the file length isn't zero. This will cause a crash in GetDeploymentDataFromManifest.
    hFile = FindFirstFile(strPath.GetValue(), &fileData);
    if (hFile == INVALID_HANDLE_VALUE || (fileData.nFileSizeHigh == 0 && fileData.nFileSizeLow == 0))
    {
        CKHR(E_FAIL);
    }

    hInstance = LoadLibrary(DFSHIMDLL);

    if (hInstance != NULL)
    {
        GetDeploymentDataFromManifestPfn load = (GetDeploymentDataFromManifestPfn) GetProcAddress(hInstance, DFSHIMFN);

        CHECK_POINTER(load);

        WCHAR wzApplicationIdentity[APPID_LENGTH];
        DWORD dwApplicationIdentityLength = APPID_LENGTH;
        WCHAR wzProcessorArchitecture[PROCARCH_LENGTH];
        DWORD dwProcessorArchitectureLength = PROCARCH_LENGTH;
        WCHAR wzApplicationManifest[APPMAN_LENGTH];
        DWORD dwApplicationManifestLength = APPMAN_LENGTH;
        WCHAR wzDeploymentProvider[DEPPRO_LENGTH];
        DWORD dwDeploymentProviderLength = DEPPRO_LENGTH;

        CKHR(load(
            strCleanUri.GetValue(), 
            strPath.GetValue(),
            wzApplicationIdentity, &dwApplicationIdentityLength,
            wzProcessorArchitecture, &dwProcessorArchitectureLength,
            wzApplicationManifest, &dwApplicationManifestLength,
            wzDeploymentProvider, &dwDeploymentProviderLength));

        strApplicationIdentity.SetValue(wzApplicationIdentity);
        strArchitecture.SetValue(wzProcessorArchitecture);
        strCodebase.SetValue(wzApplicationManifest);
        strProvider.SetValue(wzDeploymentProvider);
    }
    else
    {
        CKHR(E_FAIL);
    }

    EventWriteWpfHostUm_ReadingDeplManifestEnd(strPath.GetValue());

Cleanup:
    if (hInstance)
    {
        FreeLibrary(hInstance);
    }
    if (hFile != INVALID_HANDLE_VALUE)
    {
        FindClose(hFile);
    }
    return hr;
}

//******************************************************************************
//
// GetRegistryString()
//
// Get a single DWORD from the registry. This is not optimized for calling this
// multiple times on the hive. Returns S_FALSE if the key or value was not found.
//   
//******************************************************************************

HRESULT GetStringMapFromRegistry(__in HKEY hKey, __in LPWSTR pswzSubKey, CStringMap<CString*>& map)
{
    HRESULT hr = S_OK;
    HKEY hNewKey = 0;

    CK_PARG(pswzSubKey);

    int error = RegOpenKeyEx(hKey, pswzSubKey, 0, KEY_READ, &hNewKey);
    if (error == ERROR_FILE_NOT_FOUND)
    {
        hr = S_FALSE;
        goto Cleanup; // This is OK; the key+subkey was not found
    }
    CHECK_BOOL_FROM_WIN32(error == ERROR_SUCCESS);

    WCHAR wzNameBuffer[MAX_PATH];
    WCHAR wzValueBuffer[MAX_PATH];

    DWORD dwType = REG_SZ;
    DWORD dwIndex = 0;

    while (error == ERROR_SUCCESS)
    {
        DWORD cchNameSize = MAX_PATH;
        DWORD cbValueSize = MAX_PATH * sizeof(WCHAR);

        error = RegEnumValueW(
            hNewKey,
            dwIndex++,
            wzNameBuffer, &cchNameSize,
            0,
            &dwType,
            (LPBYTE) wzValueBuffer, &cbValueSize);

        if (error == ERROR_SUCCESS && dwType == REG_SZ)
        {
            CString *pValue = CString::CreateOnHeap(wzValueBuffer);
            CK_ALLOC(pValue);
            map.Add(wzNameBuffer, pValue);
        }
    }

    CHECK_BOOL_FROM_WIN32(error == ERROR_SUCCESS || error == ERROR_NO_MORE_ITEMS);

Cleanup:
    if (hNewKey)
    {
        RegCloseKey(hNewKey);
    }

    return hr;
}

//******************************************************************************
//
// GetVersionMapFromRegistry()
//
// SxS version to PresentationHost_vX.dll path mappings are under RegKey_WPF_HostVersions.
// For v3.0, the path to PHDLL is in a key value named "v3.0".
// Starting from v4.0, version entries are subkeys with the version as name.
// The path to PHDLL is the default key value.
// See also the IFinderPHP in the 'super-shim' implementation [the truly shared PresentationHost.exe].
// It first uses the same registry table to locate PHDLL and delegate the real work to it.
//
//******************************************************************************

HRESULT GetVersionMapFromRegistry(__in HKEY hRootKey, /*out*/CStringMap<CString*>& map)
{
    HRESULT hr = S_OK;
    HKEY hHostsKey = 0;
    WCHAR wzValueBuffer[MAX_PATH];
    DWORD cbValueSize = 0;
    DWORD dwType;

    int error = RegOpenKeyEx(hRootKey, RegKey_WPF_HostVersions, 0, KEY_READ, &hHostsKey);
    CHECK_ERROR_CODE(error);

    // Probe for v3.0.
    cbValueSize = sizeof wzValueBuffer;
    error = RegQueryValueEx(hHostsKey, L"v3.0", 0, &dwType, (LPBYTE) wzValueBuffer, &cbValueSize);
    if(error == ERROR_SUCCESS && dwType == REG_SZ)
    {
        CString *pValue = CString::CreateOnHeap(wzValueBuffer);
        CK_ALLOC(pValue);
        map.Add(L"v3.0", pValue);
    }

    // Get the post v3.0 versions
    DWORD dwIndex = 0;
    do {
        WCHAR wzNameBuffer[MAX_PATH];
        DWORD cchNameSize = MAX_PATH;        

        error = RegEnumKey(hHostsKey, dwIndex++, wzNameBuffer, cchNameSize);
        // Make the leading 'v' optional, in case downstream consumers want to allow this too.
        if (error == ERROR_SUCCESS &&
            (tolower(wzNameBuffer[0]) == 'v' && isdigit(wzNameBuffer[1]) || isdigit(wzNameBuffer[0])) )
        {
            HKEY hVersionSubkey = 0;
            // Errors due to opening or reading the sub key are ignored (we will continue to enumerate sub keys).
            int subKeyError = RegOpenKeyEx(hHostsKey, wzNameBuffer, 0, KEY_READ, &hVersionSubkey);
            if(subKeyError == ERROR_SUCCESS)
            {
                cbValueSize = sizeof wzValueBuffer;
                subKeyError = RegQueryValueEx(hVersionSubkey, NULL, 0, &dwType, (LPBYTE) wzValueBuffer, &cbValueSize);
                if(subKeyError == ERROR_SUCCESS && dwType == REG_SZ)
                {
                    CString *pValue = CString::CreateOnHeap(wzValueBuffer);
                    CK_ALLOC(pValue);
                    map.Add(wzNameBuffer, pValue);
                }
                
                RegCloseKey(hVersionSubkey);
            }
        }
    } while (error == ERROR_SUCCESS);
    
    CHECK_BOOL_FROM_WIN32(error == ERROR_NO_MORE_ITEMS);

Cleanup:
    if (hHostsKey)
    {
        RegCloseKey(hHostsKey);
    }

    return hr;
}

//******************************************************************************
//
// SignalAvalonStartupRelatedEvent(HANDLE hEvent)
//
// Signal the given event, and notify the application (by posting a WM_NOTIFY
// message) that an avalon startup related event has been signaled.
//   
//******************************************************************************

void SignalAvalonStartupRelatedEvent(HANDLE hEvent)
{
    ::SetEvent(hEvent);
    PostMessage(NULL, WM_NOTIFY, AVALON_STARTUP_CHECK, NULL);
}

///////////////////////

DWORD MsgWaitForQuitAndMultipleObjects(DWORD cEvents, __in_ecount(cEvents) HANDLE *phEvent, DWORD dwMilliSeconds, __out_ecount(1) BOOL* fWmQuit)
{
    BOOL bDone = TRUE;
    DWORD result = 0;
    //Compiler complains about while(1) usage
    while (bDone == TRUE)
    {
        MSG msg;

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                *fWmQuit = TRUE;
                PostQuitMessage((int) msg.wParam);
                return WAIT_OBJECT_0 + cEvents;
            }
            DispatchMessage(&msg);
        }

        result = MsgWaitForMultipleObjects(cEvents, phEvent, FALSE, dwMilliSeconds, QS_ALLINPUT);

        if (result == (DWORD)WAIT_OBJECT_0 + cEvents)
            continue;
        else
            break;
    }

    *fWmQuit = FALSE;
    //

    if (result < (DWORD)WAIT_OBJECT_0 + cEvents)
    {
        return (result - WAIT_OBJECT_0);
    }

    return WAIT_OBJECT_0 + cEvents;
}


//-----------------------------------------------------------------------------
// GetProcessIntegrityLevel 
// Returns the integrity level of the process with the given id (Windows Vista and later).
// The value is one of SECURITY_MANDATORY_xxx or something between the predefined levels.
// -1 is returned on error.
//-----------------------------------------------------------------------------
int GetProcessIntegrityLevel(DWORD processId)
{
    int dwIntegrityLevel = -1;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, false, processId);
    if (hProcess)
    {
        HANDLE hToken = 0;
        if(OpenProcessToken(hProcess, TOKEN_QUERY | TOKEN_QUERY_SOURCE, &hToken)) 
        {
            DWORD dwLengthNeeded;
            if (!GetTokenInformation(hToken, TokenIntegrityLevel, NULL, 0, &dwLengthNeeded))
            {
                DWORD dwError = GetLastError();
                if (dwError == ERROR_INSUFFICIENT_BUFFER)
                {
                    PTOKEN_MANDATORY_LABEL pTIL = (PTOKEN_MANDATORY_LABEL)LocalAlloc(0, dwLengthNeeded);
                    if (pTIL != NULL)
                    {
                        if (GetTokenInformation(hToken, TokenIntegrityLevel, 
                             pTIL, dwLengthNeeded, &dwLengthNeeded))
                        {
                            dwIntegrityLevel = (int)*GetSidSubAuthority(pTIL->Label.Sid, 
                                (DWORD)(UCHAR)(*GetSidSubAuthorityCount(pTIL->Label.Sid)-1));
                        }
                        LocalFree(pTIL);
                    }
                }
            }

            CloseHandle(hToken);
        }

        CloseHandle(hProcess);
    }
    return dwIntegrityLevel;
}

HRESULT CheckSameDomain(LPCWSTR url1, LPCWSTR url2, IInternetSecurityManager *pSecMgr, bool &sameDomain)
{
    sameDomain = false;
    HRESULT hr;
    UriSecurityId sid1, sid2;
    CKHR(sid1.Set(url1, pSecMgr));
    CKHR(sid2.Set(url2, pSecMgr));
    sameDomain = sid1 == sid2;
    hr = S_OK;
Cleanup:
    return hr;
}

/// <summary>
/// Finds the attribute by namespace name and returns the attributes' text value. For ambiguous names,
/// this method returns the value from the first namespace
/// </summary>
/// <param name="pAttributes">The <c>ISAXAttributes</c> instance that is searched for the attribute</param>
/// <param name="pwchUri">The namespace URI, or if the namespace has no URI, an empty string</param>
/// <param name="cchUri">The length of the URI string</param>
/// <param name="pwchLocalName">The local name of the attribute</param>
/// <param name="cchLocalName">The length of the local name string</param>
/// <param name="strValue">The string value of the attribute</param>
/// <returns> An <c>HRESULT</c> value indicating success or failure </returns>
/// <remarks>
/// This function delegates to <c>ISAXAttributes::getValueFromName</c>. The 
/// buffer returned by <c>getValueFromName</c> is not is always directly usable - it 
/// often requires a substring to be extracted from it before it can be 
/// used. <c>GetXmlAttributeValue</c> is intended as a helper function that avoids the need
/// for this extra step, and returns a <c>CString</c> object
/// </remarks>
HRESULT GetXmlAttributeValue(__in ISAXAttributes* pAttributes, __in LPCWSTR pwchUri, __in size_t cchUri, __in LPCWSTR pwchLocalName, __in size_t cchLocalName, __inout CString& strValue)
{
    HRESULT hr = S_OK;
    
    LPCWSTR pwzValue = nullptr;
    int nLength = 0;

    CHECK_POINTER_ARG(pAttributes);
    CKHR(pAttributes->getValueFromName(pwchUri, static_cast<int>(cchUri), pwchLocalName, static_cast<int>(cchLocalName), &pwzValue, &nLength));
    CKHR(strValue.SetValue(pwzValue, nLength));
Cleanup:
    return hr;
}
