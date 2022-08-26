//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//    This file defines managed functions that we do not want to be
//    part of the exe.
//
//  History
//      2005/05/04-Microsoft
//          Moved InvokeBrowser to a shared location
//      2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#pragma once

#include "String.hxx"
#include "StringMap.hxx"
#include <unknwn.h>
#include <msxml6.h>

HRESULT GetDefaultBrowser(__inout_ecount(nBufferSize)LPCWSTR pszCommandlineToExec, size_t nBufferSize);
HRESULT GetURLFromCommandLine(__in LPWSTR pszCommandStart, __deref_out_ecount(1) LPWSTR* ppszURL);
HRESULT InvokeBrowser(__in LPCWSTR pszURL); 
HRESULT GetInternetExplorerPath(_Out_writes_(MAX_PATH) LPWSTR pszPath);

HRESULT GetRegistryDWORD(__in HKEY hKey, __in LPWSTR pswzSubKey, __in LPWSTR pswzName, __out_ecount(1) DWORD& dwValue, __in DWORD dwDefaultValue);
HRESULT GetRegistryString(__in HKEY hKey, __in LPWSTR pswzSubKey, __in LPWSTR pswzName, CString& strValue);
HRESULT SetRegistryString(__in HKEY hKey, __in LPCWSTR pswzSubKey, __in LPCWSTR pswzName, CString& strValue);
HRESULT GetStringMapFromRegistry(__in HKEY hKey, __in LPWSTR pswzSubKey, CStringMap<CString*>& map);
HRESULT GetVersionMapFromRegistry(__in HKEY hKey, CStringMap<CString*>& map);

DWORD MsgWaitForQuitAndMultipleObjects(DWORD cEvents, __in_ecount(cEvents) HANDLE *phEvents, DWORD dwMilliSeconds, __out_ecount(1) BOOL* fWmQuit);

int GetProcessIntegrityLevel(DWORD processId);

template<typename PF>
bool GetProcAddress(HMODULE hModule, LPCSTR functionName, __out PF *pf)
{
    *pf = reinterpret_cast<PF>(::GetProcAddress(hModule, functionName));
    return *pf != NULL;
}
template<typename PF>
bool GetProcAddress(LPCWSTR moduleName, LPCSTR functionName, __out PF *pf)
{
    HMODULE hMod = GetModuleHandle(moduleName);
    return hMod && GetProcAddress(hMod, functionName, pf);
}

void SignalAvalonStartupRelatedEvent(HANDLE hEvent);

HANDLE TerminateIfNoHost(DWORD dwWaitTime);
HRESULT CancelTerminateIfNoHost(HANDLE hNoHostTimer);

HRESULT GetDeploymentDataFromManifest(
    __in LPCWSTR pwzDeploymentUri,
    __in_opt LPCWSTR pwzDeploymentManifestPath,
    __in CString& strApplicationIdentity,
    __in CString& strArchitecture,
    __in CString& strCodebase,
    __in CString& strProvider
    );

STDMETHODIMP
QueryService( __in_ecount(1) IUnknown* pUnk,
              __in_ecount(1) REFGUID guidService,
              __in_ecount(1) REFIID iid,
              __deref_out_ecount(1) void ** pResult);


struct UriSecurityId
{
    BYTE id[MAX_SIZE_SECURITY_ID];
    DWORD idLen;

    HRESULT Set(LPCWSTR uri, IInternetSecurityManager *pSecMgr)
    {
        idLen = MAX_SIZE_SECURITY_ID;
        return pSecMgr->GetSecurityId(uri, id, &idLen, 0);
    }

    bool operator==(const UriSecurityId &another) const
    {
        // CompareSecurityIds() is wrong in general. It ignores subdomains and port numbers.
        return idLen == another.idLen && memcmp(id, another.id, idLen) == 0;
    }
    bool operator!=(const UriSecurityId &another) const { return !(*this == another); }
};

HRESULT CheckSameDomain(LPCWSTR url1, LPCWSTR url2, IInternetSecurityManager *pSecMgr, bool &sameDomain);

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
/// used. <c>GetValue</c> is intended as a helper function that avoids the need 
/// for this extra step, and returns a <c>CString</c> object
///
/// An overload <i>without</i> <paramref name="cchUri" />, <paramref name="cchLocalName"> parameters
/// might be of interest, given the fact that we <i>always</i> pass null-terminated strings 
/// to this method in our code-base, and the string lengths can be safely calculated during runtime. 
/// Such an overload is not added at present, though it might be of interest in the future.
/// </remarks>
HRESULT GetXmlAttributeValue(__in ISAXAttributes* pAttributes, __in LPCWSTR pwchUri, __in size_t cchUri, __in LPCWSTR pwchLocalName, __in size_t cchLocalName, __inout CString& strValue);

/// <summary>
/// Compile-time length for a string literal
/// </summary>
/// <remarks>
/// This function assumes that the input is a C-style string literal that is null-terminated, 
/// and not an array of type <c>char[]</c> whose last element might be any arbitrary character. 
/// The returned length does not count the (thus presumed) terminating null
/// character
/// </remarks>
template <size_t sz>
inline size_t strlit_len(const char(&)[sz])
{
    return sz - 1;
}

/// <summary>
/// Compile-time length for a wide string literal. 
/// </summary>
/// <remarks>
/// This function assumes that the input is a C-style string literal that is null-terminated, 
/// and not an array of type <c>wchar_t[]</c> whose last element might be any arbitrary character. 
/// The returned length does not count the (thus presumed) terminating null
/// character
/// </remarks>
template <size_t sz>
inline size_t strlit_len(const wchar_t(&)[sz])
{
    return sz - 1;
}

//*********************************************************************************
// Debug Help functions
//*********************************************************************************
#if DEBUG
    void DebugInit(__in LPTSTR pszModule);
    void DebugInit(__in DWORD dwLevel);
    void DebugPrintf(__in DWORD dwLevel, __in LPTSTR pszFormat, ...);
    void DebugAssert(__in LPTSTR pszFile, int nLine, __in LPTSTR pszCondition);
    
    HRESULT ShellExecUri(__in PWSTR pwzUrl);

    #define DPFINIT     DebugInit
    #define DPF         DebugPrintf

#else
    // macro trickery.  anything with DPF in retail becomes a comment.
    #define SLASH /
    #define DPFINIT /SLASH
    #define DPF /SLASH
#endif

// Define a custom nofication id for WM_NOTIFY messages used to
// signal that avalon can be started.
#define AVALON_STARTUP_CHECK 0x01
