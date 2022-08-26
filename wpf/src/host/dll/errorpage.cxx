//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Implements the ErrorPage class of PresentationHost.
//
//  History:
//     2005/10/10 - Microsoft
//          Created (factored out of OleDocument.cxx)
//     2006/03/22 - Microsoft
//          Reworked to host an HTMLDocument and show the error page in it while keeping the process running.
//     2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#include "Precompiled.hxx"
#include "ErrorPage.hxx"
#include "HostSupport.h"
#include "DllMain.hxx"
#include "OleDocument.hxx"
#include "HTMLPageHost.hxx"
#include "Utilities.hxx"
#include "..\inc\Registry.hxx"


const unsigned MaxErrorMsgChars = 24000;
#define IE7_ERROR_PAGE L"IE7ErrorPage.html" // resource name = source filename
#define ERROR_PAGE L"ErrorPage.html" // resource name = source filename



//**
//** IMPORTANT: All this code has to be able to work on any thread. (But concurrent calls are not 
//**    expected and not supported.) See the DLL-exported function ProcessUnhandledException(). 
//**


//
// CErrorPageEventSink

class ATL_NO_VTABLE CErrorPageEventSink: 
    public CComObjectRoot,
    public IDispEventSimpleImpl<1, CErrorPageEventSink, &DIID_HTMLAnchorEvents>,
    public IPropertyNotifySink
{
public:
    HRESULT Init(IHTMLDocument2 *pErrorPageDoc)
    {
        // The Click event from the Restart link can be wired only when the document is loaded.
        // This notification comes through IPropertyNotifySink.
        ASSERT(pErrorPageDoc);
        m_spErrorPageDoc = pErrorPageDoc;
        m_fEventsSetUp = false;
        DWORD cookie;
        return AtlAdvise(pErrorPageDoc, this, IID_IPropertyNotifySink, &cookie);
    }

BEGIN_COM_MAP(CErrorPageEventSink)
   COM_INTERFACE_ENTRY(IPropertyNotifySink)
END_COM_MAP( )

BEGIN_SINK_MAP(CErrorPageEventSink)
   SINK_ENTRY_INFO(1, DIID_HTMLAnchorEvents, DISPID_HTMLELEMENTEVENTS_ONCLICK, OnRestart, &OnRestartFnInfo )
END_SINK_MAP()

private:
    CComPtr<IHTMLDocument2> m_spErrorPageDoc;
    bool m_fEventsSetUp;

    static _ATL_FUNC_INFO OnRestartFnInfo;

    VARIANT_BOOL _stdcall OnRestart()
    {
        CErrorPage::RestartApplication();
        return VARIANT_TRUE; // The result is actually ignored. Script in the HTML page cancels default handling.
    }

// IPropertyNotifySink:
    STDMETHOD(OnChanged)(DISPID dispID)
    {
        if(dispID == DISPID_READYSTATE && !m_fEventsSetUp)
        {
            HRESULT hr;

            CComQIPtr<IHTMLDocument3> spHtmlDoc(m_spErrorPageDoc);
            CComPtr<IHTMLElement> spRestartElem;
            CKHR(spHtmlDoc->getElementById(CComBSTR(L"Restart"), &spRestartElem));
            CHECK_POINTER(spRestartElem);
            CKHR(DispEventAdvise(spRestartElem, &DIID_HTMLAnchorEvents));

            m_fEventsSetUp = true;

            // Propagate the error page title to the host browser. This helps testing, and some people
            // think it is better for the user too. 
            CComPtr<IHostBrowser> spHostBrowser;
            hr = g_pOleDoc->GetWebBrowserForCurrentThread(&spHostBrowser);
            if(hr == S_FALSE)
                goto Cleanup; // No browser; maybe it has just starting closing the DocObject.
            ASSERT(hr == S_OK);
            CComPtr<IHTMLElement> spTitleElem;
            spHtmlDoc->getElementById(CComBSTR(L"title"), &spTitleElem);
            ASSERT(spTitleElem);
            CComBSTR title;
            spTitleElem->get_innerText(&title);
            hr = spHostBrowser->SetTitle(title);
            // This may fail if the browser is closing right at this moment.
            (void)hr;
        }
    Cleanup:
        return S_OK;
    }

    STDMETHOD(OnRequestEdit)(DISPID dispID)
    {
        return E_NOTIMPL;
    }
};

_ATL_FUNC_INFO CErrorPageEventSink::OnRestartFnInfo = {CC_STDCALL, VT_BOOL, 0, { } };



/////////////////////////////////////////////////////////////////////////////////////////////
// CErrorPage

__declspec(align(4)) volatile LONG CErrorPage::s_fIsActive;
DWORD CErrorPage::s_threadId;
HWND CErrorPage::s_hWnd, CErrorPage::s_hRBW;

void CErrorPage::Display(__in_ecount(1) BSTR pErrorMsg)
{
    if(InterlockedCompareExchange(&s_fIsActive, true, false))
        return;

    bool fOnMainThread = GetCurrentThreadId() == g_mainThreadId;
    // If the browser is currently blocked, activating us, the main thread should not be blocked.
    // Otherwise, the browser's UI remains unresponsive.
    /*





*/
    if(fOnMainThread 
        /*&& 
        (!g_pOleDoc->GetWebBrowser() || 
          g_pOleDoc->GetWebBrowser()->get_ReadyState(&browserState) == S_OK && browserState != READYSTATE_LOADING)*/)
    {
        // The code that kills the RootBrowserWindow can also kill the native progress page, since it also
        // appears as a child of the top native window, but it's more reliable to do it explicitly.
        if(g_pOleDoc->GetProgressPage())
        {
            g_pOleDoc->GetProgressPage()->Hide();
        }

        DisplayOnAnyThread(pErrorMsg);
    }
    else
    {   // We're on a thread other than the main one, or the main one should not be blocked. Either way,
        // the error page is run on a new thread. In the first case, this is needed because the current thread
        // may be a managed one and is thus very likely initialized to be in the MTA, but OLE can run only
        // in an STA.
        bool fReleaseCallingThread = fOnMainThread;
        if(fReleaseCallingThread) 
        {
            BSTR errorMsgCopy = ::SysAllocString(pErrorMsg);
            if(errorMsgCopy)
            {
                pErrorMsg = errorMsgCopy;
            }
        }
        unsigned threadId;
        HANDLE hThread = (HANDLE)_beginthreadex(0, 0, 
            reinterpret_cast<unsigned (__stdcall *)(void*)>(CErrorPage::DisplayOnAnyThread), pErrorMsg, 0, &threadId);
        if(!fReleaseCallingThread)
        {
            WaitForSingleObject(hThread, INFINITE);
        }
    }
}

void CErrorPage::DisplayOnAnyThread(__in_ecount(1) BSTR pErrorMsg)
{
#ifdef DEBUG
#pragma warning(disable:4509)
    __try
    {
#endif 

    DisplayOnAnyThreadImpl(pErrorMsg);

#ifdef DEBUG
    }
    // The msg box is in the filter expression so that it's run before the stack is unwound.
    __except (MessageBox(g_pOleDoc ? g_pOleDoc->GetTopWindow() : 0,
        L"An exception occurred in PresentationHostDLL!CErrorPage.\n"
        L"** This is a bug. **\nPlease report it to wpfdev, preferably with a\n"
        L"memory dump of PresentationHost.exe (or XPSViewer.exe)\n"
        L"taken before closing this.", 0, 0),
        false)
    {
    }
#pragma warning(default:4509)
#endif 
}

void CErrorPage::DisplayOnAnyThreadImpl(__in_ecount(1) BSTR pErrorMsg)
{
    HRESULT hr = S_OK;
    CString strStartupUri, strAppIdentity;
    _bstr_t htmlTemplate;
    _bstr_t modulesList;
    CComPtr<IStream> spStream;
    CComPtr<IHTMLDocument2> spHtmlDocument;
    CComQIPtr<IOleInPlaceActiveObject> spInPlaceActiveObject;

    // OleInitialize() is needed to enable clipboard operations (Copy) for the HTMLDocument in the 
    // error page. This call seems to succeed when called on the main thread, which has already
    // been CoInitialize-d.
    CKHR(OleInitialize(0));
    s_threadId = GetCurrentThreadId();

	// If an exception is handled on a worker thread, the main thread may go into shutdown in the meantime.
	// There should be no race condition from here on, because CErrorPage handles OLECMDID_ONUNLOAD directly.
	if(!g_pOleDoc || g_pOleDoc->IsShuttingDown())
		return;

    unsigned cErrorMsgChars = SysStringLen(pErrorMsg);
    ASSERT(cErrorMsgChars > 0);
    DisableTextAreaCloseTag(pErrorMsg);

    CKHR(LoadAndFixUpHtmlTemplate(htmlTemplate));

    // Prepare the values for the placeholders... StringCchPrintf() is called in the end.
    // Assumption: The HTML resource is in Unicode. It contains the following %s placeholders:
    //   0. <base href="res://%s" />. Image resource base. The full path to PHDLL is put there.
    //   1. <body onload="%s">. We put "showhide()" here to make the details visible right after loading.
    //   2. Startup URI
    //   3. Application Identity
    //   4. the stacktrace. The format specifier is "%.*s". The '*' is a placeholder for MaxErrorMsgChars.
    //   5. list of loaded modules
    // There is no attempt to escape markup characters from the error message. The text is within
    // a <textarea> element, and the browser ignores any nested tags. Only DisableTextAreaCloseTag()
    // is called, to prevent markup & script injection.

    wchar_t dllPath[MAX_PATH];
    CHECK_NULL_FROM_WIN32(GetModuleFileName(g_hInstance, dllPath, MAX_PATH));

    CKHR(strStartupUri.SetValue(COleDocument::GetInstance()->GetStartupUri()));
    if(!strStartupUri.GetValue())
        strStartupUri.SetValue(L"");
    DisableTextAreaCloseTag((LPWSTR)strStartupUri.GetValue());

    CKHR(strAppIdentity.SetValue(COleDocument::GetInstance()->GetApplicationIdentity()));
    if(!strAppIdentity.GetValue())
        strAppIdentity.SetValue(L"");
    DisableTextAreaCloseTag((LPWSTR)strAppIdentity.GetValue());

    modulesList = ListLoadedModules();
    DisableTextAreaCloseTag(modulesList);

    unsigned maxHtmlSize = htmlTemplate.length() + 20 +
        MAX_PATH +
        (unsigned)strStartupUri.Length() + (unsigned)strAppIdentity.Length() + 
        min(cErrorMsgChars, MaxErrorMsgChars) +
        modulesList.length();
    wchar_t *pHTML = new wchar_t[maxHtmlSize];
    CHECK_POINTER_ALLOC(pHTML);
    CKHR(StringCchPrintfW(pHTML, maxHtmlSize, (LPCWSTR)htmlTemplate, 
        dllPath,
        GetShowDetailsRegistryFlag() ? L"showhide()" : L"", 
        strStartupUri.GetValue(), strAppIdentity.GetValue(), 
        MaxErrorMsgChars, pErrorMsg, (LPCWSTR)modulesList));
    ASSERT(hr == S_OK);

    // Prevent transitioning into managed code.
    g_pOleDoc->ReleaseApplicationHelper();

    // Reset the browser's state, in case we had an internal navigation going on.
    g_pOleDoc->ChangeDownloadState(false);
    // READYSTATE_COMPLETE is posted to ensure the WebOC's DocumentComplete event is raised, which
    // Media Center in particular relies on to make the WebOC it's hosting visible. 
    // Normally this is done from RootBrowserWindow.OnContentRendered(), but an error may occur 
    // before that.
    g_pOleDoc->PostReadyStateChange(READYSTATE_COMPLETE);

    CKHR(SetUpHostWindow());

    // Host an instance of HTMLDocument and initialize it from a memory stream.
    // (Instead of a CLSID and an IStream, AtlAxCreateControl() accepts input of the form 
    // "HTML:<raw HTML...>", and this sort of works, but script in the HTML is not functional.
    // In ATL 7 this problem doesn't exist, but for now we can use only v3.)
    size_t cbHTML = wcslen(pHTML)*2;
    HGLOBAL hGlobal = GlobalAlloc(GMEM_FIXED, cbHTML);
    CHECK_POINTER_ALLOC(hGlobal);
    memcpy((void*)hGlobal, pHTML, cbHTML);
    CKHR(::CreateStreamOnHGlobal(hGlobal, true, &spStream));
    CKHR(CHTMLPageHostingHelper::CreateHTMLDocumentControl(s_hWnd, spStream, 0, &spHtmlDocument));

    CComObject<CErrorPageEventSink> *pEventSink = new CComObject<CErrorPageEventSink>();
    if(pEventSink)
    {
        pEventSink->Init(spHtmlDocument);
    }

    // Returning from here would lead the CLR to invoke Dr.Watson. To prevent this, we run a local
    // message loop around the error page. When the browser sends us OLECMDID_ONUNLOAD (handled
    // in ExecOleCommand), we ExitProcess.
    // To prevent running any more managed code through the Dispatcher on the main thread, messages
    // destined for it are ----ed. Also for the RootBrowserWindow (even though it shouldn't be 
    // getting any more messages).
    // ISSUE: If the unhandled exception occurred on another thread, the main-thread Dispatcher will
    //      still keep running. And, of course, there can be other managed threads still capable of
    //      running application code. Unloading the entire AppDomain is a possible solution, but it
    //      seems problematic because the main thread is in it, and the abort can fail.
    //
    // To ensure most browser keys are still working, ForwardTranslateAccelerator() is called.
    // Some of the code in it will get E_RPC_WRONG_THREAD when not on the main thread, but that's
    // not critical. To make sure F5 always works, it is handled directly in HandleKeys().
    // Alt+Left/Right and Backspace (==GoBack) are also handled there. We want to go away from the
    // XBAP, not do internal navigation within it. (These keys are similarly special-cased in 
    // RootBrowserWindow to allow navigation away from the application.)
    // Finally, spInPlaceActiveObject->TranslateAccelerator() needs to be called directly for all
    // keys to work. (ATL can't do that, because it subclasses only the host window, but input 
    // messages go to HTMLDocument's window.)
    //
    spInPlaceActiveObject = spHtmlDocument;
    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0))
    {
        if(msg.hwnd == s_hRBW)
            continue;
        if(msg.hwnd && msg.hwnd != s_hWnd)
        {
            // Assumption: Avalon windows, including Dispatcher's message window, have a class name
            // that starts with "HwndWrapper".
            wchar_t className[256];
            if(GetClassName(msg.hwnd, className, ARRAYSIZE(className))
                && wcsncmp(className, L"HwndWrapper", 11) == 0)
            {
                continue;
            }
        }
        if(!CHTMLPageHostingHelper::HandleNavigationKeys(msg)
            && ForwardTranslateAccelerator(&msg, VARIANT_FALSE) != NOERROR
            && spInPlaceActiveObject->TranslateAccelerator(&msg) != S_OK
            && ForwardTranslateAccelerator(&msg, VARIANT_TRUE) != NOERROR)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    ExitProcess((UINT)-1);
 
Cleanup:
    // A lot of work goes into generating the HTML error page. May fail one day.
    // Do a message box fallback, not to lose the error info.
    if(FAILED(hr))
    { 
        MessageBox(0, pErrorMsg, L"XAML Browser Application Error", MB_ICONSTOP);
    }
}

HRESULT CErrorPage::SetUpHostWindow()
{
    // A new child window is added to our top window. This is done because ATL's control hosting
    // subclasses the given window, which fails if not done on the thread that owns the window.
    // We make the RootBrowserWindow go away so that it doesn't overlap the error page. Destroying it
    // is not an option, because only the main thread  can do that, and even then it may not be a good
    // idea because that would cause more managed code to run (cleanup), which may cause another
    // unhandled exception. The working solution is to convert it to a message-only window.
    // This makes sure it doesn't get any windowing messages anymore, so it shouldn't cause managed 
    // code to run. Surprisingly, calling SetParent with HWND_MESSAGE sends a bunch of messages to the 
    // window. To prevent reentrancy from this, we replace the RBW's WndProc with the system one.

    HRESULT hr;

    HWND hDocObjWin = g_pOleDoc->GetTopWindow();
    CHECK_POINTER(hDocObjWin);
    HWND s_hRBW = GetWindow(hDocObjWin, GW_CHILD);
    if(s_hRBW)
    {
        SetWindowLongPtr(s_hRBW, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&DefWindowProc));
        CHECK_NULL_FROM_WIN32(SetParent(s_hRBW, HWND_MESSAGE));
        ASSERT(!GetWindow(hDocObjWin, GW_CHILD));
    }

    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.lpszClassName = L"ErrorPage";
    ATOM wndClass = RegisterClass(&wc);
    CHECK_NULL_FROM_WIN32(wndClass);
    RECT r;

    GetClientRect(hDocObjWin, &r);
    s_hWnd = CreateWindowEx(0, (LPCWSTR)wndClass, 0, WS_CHILD|WS_VISIBLE, 0, 0, r.right, r.bottom, hDocObjWin, 0, 0, 0);
    CHECK_NULL_FROM_WIN32(s_hWnd);
    // Setting focus ensures keyboard navigation within the error page works.
    SetFocus(s_hWnd);

    hr = S_OK;
Cleanup:
    return hr;
}

bool CErrorPage::ExecOleCommand(__in_opt const GUID * pguidCmdGroup,
                   DWORD dwCmdId, DWORD /*nCmdExecOpt*/,
                   __in_opt VARIANT * /*pvarArgIn*/, __out_opt VARIANT * /*pvarArgOut*/)
{
    if(!s_fIsActive)
        return false;

    if(pguidCmdGroup == 0)
    {
        switch(dwCmdId)
        {
        case OLECMDID_REFRESH:
            RestartApplication();
            return true;
        case OLECMDID_ONUNLOAD:
            // The title becomes the label of the TravelLog entry for the application.
            // It's been changed to be the title of the error page.
            CComPtr<IHostBrowser> spHostBrowser;
            if(g_pOleDoc->GetWebBrowserForCurrentThread(&spHostBrowser) == S_OK)
            {
                spHostBrowser->SetTitle(g_pOleDoc->GetStartupUri());
            }

            // Because of the premature termination, the browser times out on RPC calls and freezes
            // for a few seconds. Hiding the error page immediately improves the UX somewhat.
            ShowWindow(s_hWnd, SW_HIDE);

            PostThreadMessage(s_threadId, WM_QUIT, (WPARAM)-1, 0);
            return true;
        }
    }

    return false;
}

HRESULT CErrorPage::RestartApplication()
{
    return g_pOleDoc->RestartApplication();
}

HRESULT CErrorPage::LoadAndFixUpHtmlTemplate(__out _bstr_t &htmlTemplate)
{
    HRESULT hr = S_OK;

    HINSTANCE hResourceInstance = NULL;
    HRSRC hHtmlRes = NULL;
    void* pHTMLTemplateRes = NULL;

    // Load the resource DLL and the HTML page resource from it.
    hResourceInstance = LoadResourceDLL();
    CHECK_NULL_FROM_WIN32(hResourceInstance);

    // Determine whether to load IE6 or IE7 template
    const wchar_t *IE7ImageLibName = L"ieframe.dll";
    wchar_t sysDir[MAX_PATH];
    GetSystemDirectory(sysDir, MAX_PATH);
    if(FileExists(sysDir, IE7ImageLibName))
    {
        hHtmlRes = FindResource(hResourceInstance, IE7_ERROR_PAGE, RT_HTML);
    }
    else
    {
        hHtmlRes = FindResource(hResourceInstance, ERROR_PAGE, RT_HTML);
    }

    CHECK_NULL_FROM_WIN32(hHtmlRes);
    
    unsigned cchHtmlTemplateSize = SizeofResource(hResourceInstance, hHtmlRes)/sizeof(wchar_t);

    HGLOBAL hHtmlResLoaded = LoadResource(hResourceInstance, hHtmlRes); // no need to Free
    CHECK_NULL_FROM_WIN32(hHtmlResLoaded);
    pHTMLTemplateRes = LockResource(hHtmlResLoaded);

    // Need to form a null-terminated string and make some tweaks to it, but the returned memory
    // is not writeable; so, a temporary copy is made.
    wchar_t *pHTMLTemplate = new wchar_t[cchHtmlTemplateSize + 1];
    CHECK_POINTER_ALLOC(pHTMLTemplate);
    wmemcpy(pHTMLTemplate, (wchar_t*)pHTMLTemplateRes, cchHtmlTemplateSize);
    pHTMLTemplate[cchHtmlTemplateSize] = 0;

    // To arouse less curiosity, remove the leading comment from the HTML...
    // The "mark of the web" at the very beginning is skipped: 
    // "<!-- saved from url=(0014)about:internet -->"
    wchar_t *p = wcsstr(pHTMLTemplate+5, L"<!--");
    if(p)
    {
        wchar_t *pEnd = wcsstr(p+4, L"-->");
        if(!pEnd)
        {
            CKHR(E_UNEXPECTED);
        }
        wmemset(p, ' ', pEnd-p+3);
    }

    htmlTemplate = pHTMLTemplate;
    delete [] pHTMLTemplate;

Cleanup:
    if (pHTMLTemplateRes)
    {
        UnlockResource(pHTMLTemplateRes);
    }
    if (hResourceInstance)
    {
        FreeMUILibrary(hResourceInstance);
    }

    return hr;
}

bool CErrorPage::GetShowDetailsRegistryFlag()
{
    bool fbShowDetails = false;

    HKEY hKey;

    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKey_WPF_Hosting, 0, KEY_QUERY_VALUE, &hKey))
    {
        DWORD dwValue = 0;
        DWORD dwcbValue = sizeof(dwValue);
        fbShowDetails = (ERROR_SUCCESS == RegQueryValueEx(
                hKey, RegValue_AutoShowXbapErrorDetails, 0, 0, (BYTE*)&dwValue, &dwcbValue)) 
            && dwValue != 0;
        RegCloseKey(hKey);
    }

    return fbShowDetails;
}

// Makes a list of the executable modules loaded in the process, one line per module, 
// showing module name, version and filepath.
_bstr_t CErrorPage::ListLoadedModules()
{
    _bstr_t modList;
    HANDLE hModulesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
    MODULEENTRY32 modEntry;
    modEntry.dwSize = sizeof MODULEENTRY32;
    if(hModulesSnapshot != INVALID_HANDLE_VALUE && Module32First(hModulesSnapshot, &modEntry))
    {
        do {
            const size_t MaxVersionStringLen = 60;
            wchar_t modLine[MAX_MODULE_NAME32+MaxVersionStringLen+MAX_PATH+12];
            BSTR version = GetFileVersion(modEntry.szExePath);
            if(FAILED(StringCbPrintf(modLine, sizeof modLine, L"%s v%.*s - %s\r\n",
                            modEntry.szModule, MaxVersionStringLen, version, modEntry.szExePath)))
            {
                ASSERT(false);
            }
            else
            {
                modList += modLine;
            }
            SysFreeString(version);
        } while(Module32Next(hModulesSnapshot, &modEntry));
    }
    CloseHandle(hModulesSnapshot);
    return modList;
}

// Looks for the HTML "</textarea>" close tag in the given string and disables it. 
// Used to prevent markup & script injection in the generated HTML error page.
void CErrorPage::DisableTextAreaCloseTag(__inout wchar_t *str)
{
    if(!str)
        return;
    wchar_t *p = str;
    while((p = wcschr(p, '<')) != 0)
    {
        // '>' is not matched; the browser allows spaces before it.
        if(p[1] == '/' && _wcsnicmp(p+2, L"textarea", 8) == 0) 
            p[1] = '~';
        p++;
    }
}

void CErrorPage::SetRect(int x, int y, int width, int height)
{
    if(s_hWnd)
    {
        MoveWindow(s_hWnd, x, y, width, height, false);
    }
}

LRESULT CErrorPage::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

