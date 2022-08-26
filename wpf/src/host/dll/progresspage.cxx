//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Deployment progress page. See .hxx.
//
//  History:
//      2007/12/xx   Microsoft     Created
//
//------------------------------------------------------------------------

#include "Precompiled.hxx"
#include "ProgressPage.hxx"
#include "DllMain.hxx"
#include "OleDocument.hxx" 
#include "HTMLPageHost.hxx"
#include "Utilities.hxx"
#include "psapi.h" // For GetMappedFileName()


CProgressPage::~CProgressPage()
{
    // Normally, the page should be explicitly hidden before interfaces are released.
    // Hide() is called here to help ensure orderly cleanup, just in case.
    Hide();
}

void CProgressPage::Init(IProgressPageHost *pHost)
{
    ASSERT(!m_pHost && pHost);
    m_pHost = pHost;
}

HRESULT GetFilePathFromDevicePath(__in_ecount(MAX_PATH) LPCTSTR pszDevicePath, __out_ecount(MAX_PATH) LPTSTR pszFilePath)
{
    // Translate path with device name to drive letters.

    // Originally we tried to use GetLogicalDriveStrings/QueryDosDevice to map the 
    // path but on server 2003 this fails with an access denied error in QueryDosDevice.
    // This is the way suggested by MSDN here: http://msdn.microsoft.com/en-us/library/aa366789(VS.85).aspx

    // So, since (at least for this release) we always get installed to the Windows directory
    // we can use that as a basis for some string manipulation to convert the device path
    // to a dos path.

    HRESULT hr = S_OK;

    wchar_t *pszWindowsDirName = pszFilePath;
    wchar_t szDevicePath[MAX_PATH];
    CKHR(StringCchCopy(szDevicePath, MAX_PATH, pszDevicePath)); // a copy that I can lower case

    // 1) get windows directory, remove drive letter
    CKHR(SHGetFolderPath(NULL, CSIDL_WINDOWS, NULL, 0, pszFilePath));
    pszWindowsDirName += 3; // trim "c:\"

    _wcslwr_s(pszFilePath, MAX_PATH);
    _wcslwr_s(szDevicePath, MAX_PATH);

    // 2) find the windows directory name in device path
    const wchar_t* pszPath = wcsstr(szDevicePath, pszWindowsDirName);
    if (pszPath == NULL)
    {
        CKHR(E_UNEXPECTED);
    }
    
    // 3) append path (including windows dir) on to original drive letter
    *pszWindowsDirName = '\0';
    CKHR(StringCchCat(pszFilePath, MAX_PATH, pszPath));
    
Cleanup:
    return hr;
}

HRESULT CProgressPage::Show()
{
    if(IsActive())
        return S_FALSE; 

    HRESULT hr;
    HMODULE hModule = NULL;
    CComPtr<IUnknown> spContainerUnknown;
    CComPtr<IMoniker> spMoniker;
    CComPtr<IBindCtx> spBindCtx;

    m_hHostWindow = m_pHost->GetHostWindow();
    if(!m_hHostWindow)
        return E_UNEXPECTED;
    CKHR(CHTMLPageHostingHelper::CreateHTMLDocumentControl(m_hHostWindow, 0, &spContainerUnknown, &m_spHtmlDocument));
    m_spControlHost = spContainerUnknown;

    // Load the progress page from res:// URL via IPersistMoniker. If IHTMLDocument2::put_URL() is called
    // instead, IE 7 decides to create a new top-level window!
    {
    wchar_t progressPageURL[MAX_PATH] = L"res://";
   
    //
    // On WinXP, LoadMUILibrary (and thus LoadResourceDLL) return an HMODULE which 
    // is a handle to a memory mapped file, not a "real" HMODULE.  Because of
    // this we have to jump through some hoops to get the fully qualified filename
    // of the MUI file.
    //

    // Load the associated resource DLL so that we can get its path via GetMappedFileName,
    // this returns us a device path, so convert the device path to a file path.
    hModule = LoadResourceDLL();
    CHECK_NULL_FROM_WIN32(hModule);
    wchar_t devicePath[MAX_PATH];
    CHECK_ZERO_FROM_WIN32(GetMappedFileName(GetCurrentProcess(), hModule, devicePath, MAX_PATH));
    wchar_t filePath[MAX_PATH];
    CKHR(GetFilePathFromDevicePath(devicePath, filePath));

    // The MUI library gets loaded as a memory mapped file, so we want to
    // free it here and call LoadLibrary() on the actual file so that IE
    // can load resources from the root file path.
    FreeMUILibrary(hModule);
    hModule = NULL;
    m_hostDllMuiModule = LoadLibrary(filePath);
    CHECK_NULL_FROM_WIN32(m_hostDllMuiModule);

    CKHR(StringCchCat(progressPageURL, MAX_PATH, filePath));
    CKHR(StringCchCat(progressPageURL, MAX_PATH, L"/ProgressPage.html"));
    CKHR(CreateURLMoniker(NULL, progressPageURL, &spMoniker));
    }
    CKHR(CreateBindCtx(0, &spBindCtx));
    CKHR(CComQIPtr<IPersistMoniker>(m_spHtmlDocument)->Load(true/*try sync*/, spMoniker, spBindCtx, 0));

    CKHR(CComQIPtr<IHTMLDocument>(m_spHtmlDocument)->get_Script(&m_spDocumentScript));
    CKHR(m_spControlHost->SetExternalDispatch(static_cast<IDispatch*>(this)));

Cleanup:
    
    if (hModule)
    {
        FreeMUILibrary(hModule);
        hModule = NULL;
    }

    m_bFailed = FAILED(hr);
    return hr;
}

HRESULT CProgressPage::Hide()
{
    if(m_spHtmlDocument == NULL) // Hide even if we failed to initialize completely
        return S_FALSE;

    HRESULT hr = S_OK;
    if (m_spControlHost)
    {
        hr = m_spControlHost->AttachControl(NULL, NULL);
    }
    m_spControlHost.Release();
    m_spHtmlDocument.Release();
    m_spDocumentScript.Release();

    if (m_hostDllMuiModule)
    {
        FreeLibrary(m_hostDllMuiModule);
        m_hostDllMuiModule = NULL;
    }

    // **BEWARE**: This call may cause all interface references to be released, which will delete the object.
    if (m_pHost)
    {
        m_pHost->OnProgressPageClosed();
    }

    return hr;
}

HRESULT CProgressPage::TranslateAcceleratorIO(__in MSG *pMsg)
{
    if(m_spHtmlDocument != NULL)
    {
        // When focus is on the progress page, we want at least the basic navigation keys to work.
        // The hosted HTMLDocument object and the top-level browser are not aware of each other...
        if(CHTMLPageHostingHelper::HandleNavigationKeys(*pMsg))
            return S_OK;

        // Crude heuristic: inferring when the HTML page is shown. There is no notification about that.
        // The DHTML onload event usually occurs earlier. We want the progress page to be really shown
        // before we get bogged down in loading the managed assemblies, which on cold start takes a long time.
        // The HTMLDocument window will necessarily get WM_PAINT so that it can show anything. It may, however,
        // not render all the content in response to the first WM_PAINT, because it loads and parses content
        // asynchonously. Practically, it seems to show the basic page text on first render, and then the
        // images may come a little later. This works out just fine for us.
        // See also COleObject::DoVerb().
        if(!m_bRendered && pMsg->message == WM_PAINT && GetParent(pMsg->hwnd) == m_hHostWindow)
		{
			m_bPartialRendered = true; //(Used for debugging if nothing else.)
            if(m_bLoaded)
            {
                m_bRendered = true;
                EventWriteWpfHostUm_ProgressPageShown();
                m_pHost->OnProgressPageRendered();
            }
		}
    }
    return S_FALSE;
}

//[IDispatch implementation begin]

// Notifications coming from script in the HTML page
const LONG DISPID_CANCEL = 7; 
const LONG DISPID_ONLOADED = 8;

HRESULT CProgressPage::GetIDsOfNames(
    REFIID, __in_ecount(cNames) LPOLESTR *pNames, UINT cNames, LCID, __out_ecount(cNames) DISPID* rgDispId)
{
    if(cNames == 1)
    {
        if(wcscmp(pNames[0], L"OnLoaded") == 0)
        {
            *rgDispId = DISPID_ONLOADED;
            return S_OK;
        }
        if(wcscmp(pNames[0], L"Cancel") == 0)
        {
            *rgDispId = DISPID_CANCEL;
            return S_OK;
        }
    }
    return DISP_E_UNKNOWNNAME;
}

STDMETHODIMP CProgressPage::Invoke(DISPID dispidMember,
      __in_ecount(1) REFIID, LCID, WORD, __in_ecount(1) DISPPARAMS *,
      __out_ecount(1) VARIANT * pVarResult,
      __out_ecount_opt(1) EXCEPINFO * pExcepInfo, __out_ecount_opt(1) UINT * puArgErr)
{
    switch(dispidMember)
    {
    case DISPID_ONLOADED:
        m_bLoaded = true;
        // Note this is only 'logical load'. The page is likely not rendered yet. See the WM_PAINT detection.
        return S_OK;
    case DISPID_CANCEL:
        return m_pHost->ExecOLECommand(OLECMDID_STOP);
    default:
        return DISP_E_MEMBERNOTFOUND;
    }
}

//[IDispatch implementation end]

// "Nonstandard extension used : class rvalue used as lvalue"
// This warning occurs when passing &CComVariant(value) as a method parameter.
// It's awkward to declare local varibles for this. Suppressing the warning is safe for input-only VARIANTs.
#pragma warning(disable: 4238) 

HRESULT CProgressPage::ShowProgressMessage(BSTR message)
{
    if(!IsActive())
        return E_UNEXPECTED;
	ASSERT(m_bLoaded); // If not, we'll get DISP_E_UNKNOWNNAME below.
    return m_spDocumentScript.Invoke1(L"ShowProgressMessage", &CComVariant(message));
}

HRESULT CProgressPage::SetApplicationName(BSTR appName)
{
    if(!IsActive())
        return E_UNEXPECTED;
	ASSERT(m_bLoaded); // If not, we'll get DISP_E_UNKNOWNNAME below.
    return m_spDocumentScript.Invoke1(L"SetApplicationName", &CComVariant(appName));
}

HRESULT CProgressPage::SetPublisherName(BSTR publisherName)
{
    if(!IsActive())
        return E_UNEXPECTED;
    return m_spDocumentScript.Invoke1(L"SetPublisherName", &CComVariant(publisherName));
}

HRESULT CProgressPage::OnDownloadProgress(UINT64 bytesDownloaded, UINT64 bytesTotal)
{
    if(!IsActive())
        return E_UNEXPECTED;
    // Passing 64-bit ints to script (or via IDispatch) doesn't seem to work.
    // That's why the contract is to pass KB.
    return m_spDocumentScript.Invoke2(L"OnDownloadProgress", 
        &CComVariant(UINT((bytesDownloaded+512)/1024)), &CComVariant(UINT((bytesTotal+512)/1024)));
}

