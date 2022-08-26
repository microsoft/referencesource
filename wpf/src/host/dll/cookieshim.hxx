//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Shim for the WinInet cookie APIs used by the managed code and by the WebOC.
//      The calls are delegated to the IHostBrowser implementation in the browser process.
//      By doing this we achieve 3 things:
//          - Sharing session cookies with the browser
//          - Accessing the same WinInet peristent cookie store when IE is in protected mode
//             (Protected-mode IE means low integiry level. There is a separate Low cookie cache. 
//              PresentationHost always runs at medium.)
//          - When hosted in a Mozilla browser, using its cookie store, not WinInet
//
//  History:
//     2007/03/30   Microsoft     Created
//     2007/09/20   Microsoft     Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#pragma once

class CCookieShim
{
public:
    static HRESULT Init(bool isTopLevel, __in_opt LPCWSTR topLevelUri);
    static void Uninit();
    static bool IsApplicationThirdParty() { return s_isApplicationThirdParty; }

private:
    CCookieShim() { }

    static DWORD (WINAPI *s_pfInternetSetCookieEx)(LPCWSTR, __in_opt LPCWSTR, LPCWSTR, DWORD, __in_opt DWORD_PTR);
    static DWORD WINAPI  InternetSetCookieExDetour(LPCWSTR, __in_opt LPCWSTR, LPCWSTR, DWORD, __in_opt DWORD_PTR);
    static BOOL (WINAPI *s_pfInternetGetCookieEx)(
        LPCWSTR, __in_opt LPCWSTR, __out_ecount_opt(*pcchData) LPWSTR, __inout LPDWORD pcchData, DWORD, LPVOID);
    static BOOL WINAPI InternetGetCookieExDetour(
        LPCWSTR, __in_opt LPCWSTR, __out_ecount_opt(*pcchData) LPWSTR, __inout LPDWORD pcchData, DWORD, LPVOID);

    static HRESULT IsThirdPartyUri(LPCWSTR uri);
    static HRESULT GetInternetSecurityManagerForCurrentThread(__deref_out IInternetSecurityManager **ppSecMgr);

    // whether the hosted application is 3rd-party with respect to the top-level document in the browser
    static bool s_isApplicationThirdParty;
};
