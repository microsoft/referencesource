//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      [See .h]
//
//  History:
//     2007/03/30   Microsoft     Created
//     2007/09/20   Microsoft     Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#include "precompiled.hxx"
#include "CookieShim.hxx"
#include "HostSupport.h"
#include "OleDocument.hxx"
#include "urlmoninterop.hxx"
#include "DllMain.hxx" // g_pOleDoc, g_mainThreadId
#include "..\Detours\Detours.h"


// CAutoSetWin32Error helps correctly set the last error from functions using objects with destructors.
// A dtor may do cleanup and thus call Win32 functions that overwrite what was previously set with 
// SetLastError(). Declare the CAutoSetWin32Error instance before any other stack objects in a function.
// This way its dtor will be called after all others and will set the last Win32 error.
class CAutoSetWin32Error
{
    DWORD m_error;
public:
    CAutoSetWin32Error(): 
      m_error(ERROR_SUCCESS) { }
    void SetLastError(DWORD error)
    {
        m_error = error;
    }
    ~CAutoSetWin32Error() 
    {
        // By convention, SetLastError() is called only when returning an error.
        if(m_error != ERROR_SUCCESS)
        {
            ::SetLastError(m_error);
        }
    }
};

////////////////////////////////////////////////////////////////////////////////////////
// CCookieShim

// WinInet must not be delay-loaded in order for these initializations to work correctly.
DWORD (WINAPI *CCookieShim::s_pfInternetSetCookieEx)(
    LPCWSTR, __in_opt LPCWSTR, LPCWSTR, DWORD, __in_opt DWORD_PTR) = &InternetSetCookieExW;
BOOL (WINAPI *CCookieShim::s_pfInternetGetCookieEx)(
    LPCWSTR, __in_opt LPCWSTR, __out_ecount_opt(*pcchData) LPWSTR, __inout LPDWORD pcchData, DWORD, LPVOID) = &InternetGetCookieExW;

bool CCookieShim::s_isApplicationThirdParty;

HRESULT CCookieShim::Init(bool isTopLevel, __in_opt LPCWSTR topLevelUri)
{
    HRESULT hr = S_OK;

    // Detours can be found on http://toolbox. It has a nice help file with it.
    // It is enough to detour InternetSetCookieExW & InternetGetCookieExW because that's what the managed code 
    // and the WebOC use.
    CHECK_ERROR_CODE(DetourTransactionBegin());
    CHECK_ERROR_CODE(DetourUpdateThread(GetCurrentThread()));
    CHECK_ERROR_CODE(DetourAttach((void**)&s_pfInternetSetCookieEx, InternetSetCookieExDetour));
    CHECK_ERROR_CODE(DetourAttach((void**)&s_pfInternetGetCookieEx, InternetGetCookieExDetour));
    CHECK_ERROR_CODE(DetourTransactionCommit());

    s_isApplicationThirdParty = false;
    if(!isTopLevel)
    {
        if(!topLevelUri || !topLevelUri[0])
        { // browser didn't tell us => probably crossing security zones
            s_isApplicationThirdParty = true;
        }
        else
        {
            // Test whether the startup URI is 3rd-party with respect to the top-level URI.
            s_isApplicationThirdParty = IsThirdPartyUri(topLevelUri) != S_FALSE;
        }
    }

Cleanup:
    return hr;
}

void CCookieShim::Uninit()
{
    // Undetouring on shutdown is needed specifically to avoid DevDiv bug 161831: WMNetMgr.dll (part of the
    // Windows Media control) somehow manages to make an (asynchronous) call to InternetGetCookieEx() after 
    // PHDLL is unloaded.
    if(s_pfInternetSetCookieEx != InternetSetCookieEx) // detoured?
    {
        if(DetourTransactionBegin() == NOERROR &&
            DetourUpdateThread(GetCurrentThread()) == NOERROR &&
            DetourDetach((void**)&s_pfInternetSetCookieEx, InternetSetCookieExDetour) == NOERROR &&
            DetourDetach((void**)&s_pfInternetGetCookieEx, InternetGetCookieExDetour) == NOERROR)
        {
            DetourTransactionCommit();
        }
        else
        {
            ASSERT(false);
        }
    }
}

DWORD WINAPI CCookieShim::InternetSetCookieExDetour(
    LPCWSTR lpszUrl, __in_opt LPCWSTR lpszCookieName, LPCWSTR lpszCookieData, DWORD flags, __in_opt DWORD_PTR P3PHeader)
{
    CAutoSetWin32Error autoError;
    if(!(lpszUrl && *lpszUrl && lpszCookieData && *lpszCookieData))
    {
        autoError.SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }
    if(flags & ~(INTERNET_COOKIE_THIRD_PARTY | INTERNET_COOKIE_EVALUATE_P3P))
    {
        ASSERT(0);
        autoError.SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    HRESULT hr;
    bool isThirdParty = (flags & INTERNET_COOKIE_THIRD_PARTY) != 0;
    if(!isThirdParty)
    {
        // 3rd-party status has to be forced in two situations:
        // - WebOC hosted in an XBAP. The WebOC always thinks it's a top-level browser, but the XBAP as a whole 
        //  may be 3rd party to the top-level document.
        // - Media fetched from outside the site of origin. The managed CookieHandler doesn't pass the 3rd party
        //  bit, but it assumes that the call to InternetSetCookieEx will be intercepted here and the bit added
        //  if necessary.
        hr = IsThirdPartyUri(lpszUrl);
        if(FAILED(hr))
        {
            autoError.SetLastError(ERROR_GEN_FAILURE);
            return false;
        }
        isThirdParty = hr == S_OK;
    }

    CComPtr<IHostBrowser> spHostBrowser;
    if(g_pOleDoc->GetWebBrowserForCurrentThread(&spHostBrowser) != S_OK)
    {
        autoError.SetLastError(ERROR_VC_DISCONNECTED); // "The session was canceled."
        return false;
    }
    hr = spHostBrowser->SetCookie(
        lpszUrl, lpszCookieName, lpszCookieData, isThirdParty, reinterpret_cast<LPCWSTR>(P3PHeader));
    if(SUCCEEDED(hr))
        return true;
    //! Assuming that the implementation of SetCookie returns E_ACCESSDENIED for a rejected cookie or
    // otherwise HRESULT_FROM_WIN32.
    if(hr == E_ACCESSDENIED)
        return COOKIE_STATE_REJECT;
    autoError.SetLastError(hr & 0xFFFF); 
    return false;
}

BOOL WINAPI CCookieShim::InternetGetCookieExDetour(
        LPCWSTR pUri, __in_opt LPCWSTR pCookieName, __out_ecount_opt(*pcchData) LPWSTR pCookieData, __inout LPDWORD pcchData, DWORD flags, LPVOID)
{
    CAutoSetWin32Error autoError;
    if(!(pUri && *pUri && pcchData))
    {
        autoError.SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }
    // pCookieData can be null on entry. Then only the size of the buffer needed for the cookie data is returned.
    if(flags & ~INTERNET_COOKIE_THIRD_PARTY)
    {
        ASSERT(0);
        autoError.SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    HRESULT hr;
    bool isThirdParty = (flags & INTERNET_COOKIE_THIRD_PARTY) != 0;
    if(!isThirdParty)
    {
        //[See explanation in InternetSetCookieExDetour.]
        hr = IsThirdPartyUri(pUri);
        if(FAILED(hr))
        {
            autoError.SetLastError(ERROR_GEN_FAILURE);
            return false;
        }
        isThirdParty = hr == S_OK;
    }

    CComPtr<IHostBrowser> spHostBrowser;
    if(g_pOleDoc->GetWebBrowserForCurrentThread(&spHostBrowser) != S_OK)
    {
        autoError.SetLastError(ERROR_VC_DISCONNECTED); // "The session was canceled."
        return false;
    }
    CComBSTR cookieData;
    hr = spHostBrowser->GetCookie(pUri, pCookieName, isThirdParty, &cookieData);
    if(FAILED(hr))
    {
        autoError.SetLastError(hr & 0xFFFF); 
        return false;
    }

    unsigned cchData = cookieData.Length()+1; // +1 for terminating '\0'
    if(cchData <= 1 || cchData > CCH_COOKIE_MAX)
    {
        autoError.SetLastError(ERROR_INVALID_DATA);
        return false;
    }
    // Make sure the terminating '\0' is not included in the BSTR's character count. (This is possible and 
    // valid in principle but unexpected in most cases.)
    ASSERT(cookieData[cchData-2] && !cookieData[cchData-1]);

    if(!pCookieData)
    {
        // Return space needed to store cookie data.
        // *2: Kid you not, this is what WinINet returns! (inetcore\wininet\http\cookie.cxx)
        //  And only in this case (and when returning ERROR_INSUFFICIENT_BUFFER--like below). (!#$#$)
        // And the ClickOnce SystemNetDownloader is aware of this doubling and allocates only half that many
        // characters.
        *pcchData = cchData*2;
    }
    else
    {
        if(*pcchData < cchData)
        {
            *pcchData = cchData*2; // see above...
            autoError.SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return false;
        }
        StringCchCopy(pCookieData, *pcchData, cookieData);
        *pcchData = cchData; // WinINet includes the final '\0' in the count.
    }
    return true;
}

HRESULT CCookieShim::GetInternetSecurityManagerForCurrentThread(__deref_out IInternetSecurityManager **ppSecMgr)
{
    if(GetCurrentThreadId() == g_mainThreadId)
        return UrlmonInterop::GetSecurityManager(ppSecMgr);

    static IClassFactory *pSecMgrFactory;
    if(!pSecMgrFactory)
    {
        HRESULT hr = CoGetClassObject(
            CLSID_InternetSecurityManager, CLSCTX_INPROC_SERVER, 0, 
            IID_IClassFactory, (void**)&pSecMgrFactory);
        if(FAILED(hr))
            return hr;
    }
    return pSecMgrFactory->CreateInstance(0, IID_IInternetSecurityManager, (void**)ppSecMgr);
}

HRESULT CCookieShim::IsThirdPartyUri(LPCWSTR uri)
{
    ASSERT(*uri);
    if(s_isApplicationThirdParty)
        return S_OK;

    // Quick test for the startup URI
    LPCWSTR startupUri = g_pOleDoc->GetStartupUri();
    if(_wcsicmp(startupUri, uri) == 0)
        return S_FALSE;

    // The same method is used here as in IE: Get the "security id" of both URIs and then call
    // CompareSecurityIds(). Despite its name, CompareSecurityIds() actually tests for matching 
    // "minimal domains" only. For example, "http://www.abc.com" and "http://images.abc.com" are considered 
    // matching. But "http://abc.com.uk" and "http://cnn.com.uk" are not.
    HRESULT hr;
    static UriSecurityId s_startupUriSId;   
    UriSecurityId sid2;
    CComPtr<IInternetSecurityManager> spSecMgr;
    CKHR(GetInternetSecurityManagerForCurrentThread(&spSecMgr));
    if(!s_startupUriSId.id[0]) // init once, lazily
    {
        CKHR(s_startupUriSId.Set(startupUri, spSecMgr));
    }
    CKHR(sid2.Set(uri, spSecMgr));
    CKHR(CompareSecurityIds(s_startupUriSId.id, s_startupUriSId.idLen, sid2.id, sid2.idLen, 0));
    hr = hr == S_OK ? S_FALSE : S_OK;
Cleanup:
    return hr;
}
