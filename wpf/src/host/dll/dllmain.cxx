//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Implements the main entry point of the PresentationShim used for hosting
//     Windows Client Applications in the browser
//
//  History:
//     2002/06/12-murrayw
//          Created
//     2003/06/03-kusumav
//          Ported to WCP
//     2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#include "Precompiled.hxx"
#include "DllMain.hxx"
#include "OleDocument.hxx"
#include "OleInPlaceActiveObject.hxx"
#include "OleDocumentView.hxx"
#include "ErrorPage.hxx"
#include "Detours.hxx"
#include "HostSupport.h"
#include "MessageFilter.hxx"
#include "..\inc\ShimDllInterface.hxx"
#include "..\inc\registry.hxx"


// This macro will keep the types in sync between the LoadHistoryHelper and SaveHistoryHelper methods.
#define STARTUP_URI_LENGTH_TYPE ULONG

//******************************************************************************
//
//   global variables
//
//******************************************************************************
HINSTANCE  g_hInstance;
DWORD      g_mainThreadId;
HANDLE     g_hNoHostTimer = 0;

BOOL g_bFilterInputImplemented = TRUE;

COleDocument* g_pOleDoc = NULL;

CComModule _Module;


//******************************************************************************
//
//   Forward definitions
//
//******************************************************************************

BOOL IsKeyReservedForFrame(const MSG *pMSG, bool &fCallDefWindowProc);
HRESULT GetApplicationIdentity(LPCWSTR pwzUri, LPCWSTR pwzDeploymentManifestPath, CString& strApplicationIdentity);

/*+-------------------------------------------------------------------------*
 *
 * DllMain
 *
 *+-------------------------------------------------------------------------*/
extern "C"
BOOL
DllMain(HINSTANCE hInstance, ULONG reason, CONTEXT*)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hInstance;
        g_mainThreadId = GetCurrentThreadId();

        DisableThreadLibraryCalls(hInstance);

        // LIBID_ATLLib: This is a dummy LIBID. Without it, asserts fail in AtlCom.h when an
        // IAxWinAmbientDispatchEx call is handled from the hosted HTMLDocument object.
        // The cause is explained here: http://support.microsoft.com/kb/832687
        // ("You may receive the "ATLASSERT" error message while hosting an ActiveX control in a COM DLL
        //  that is statically linked to the ATL in Visual C++ .NET or in Visual C++ 2005").
        // If we ever need IAxWinAmbientDispatchEx working, the real solution outlined in this article
        // can be implemented.
        _Module.Init(0, hInstance, &LIBID_ATLLib);

        if(FAILED(Detours::Init()))
            return FALSE;
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        Detours::Uninit();
    }

    return TRUE;
}

STDAPI DllRegisterServer()
{
    HRESULT hr = _Module.RegisterServer(false/*no typelib*/);
    if(SUCCEEDED(hr))
    {
        hr = _Module.UpdateRegistryFromResource(L"RegistrySetup", true);
    }
    return hr;
}
STDAPI DllUnregisterServer()
{
    _Module.UnregisterServer(false/*no typelib*/);
    return _Module.UpdateRegistryFromResource(L"RegistrySetup", false);
}

// Invokes Watson and terminates on any failure--see WatsonReportingShared.hxx
void Activate_export(const ActivateParameters* pParameters, __deref_out_ecount(1) LPUNKNOWN* ppInner)
{
    HRESULT hr = S_OK;

    // Initialize ETW
    EventRegisterMicrosoft_Windows_WPF();

    if (pParameters->dwSize < sizeof(ActivateParameters))
    {
        ASSERT(false);
        CKHR(E_INVALIDARG);
    }
    CK_PARG(ppInner);
    *ppInner = NULL;

    MimeType mimeType = pParameters->mime;
    if(mimeType == MimeType_Unknown && pParameters->pswzErrorMessageToDisplay)
    {
        // There's code in the basic DocObject activation that needs a valid MIME type.
        mimeType = MimeType_Application;
    }

    g_hNoHostTimer = pParameters->hNoHostTimer;

    CKHR(CMessageFilter::Register(20000));

    ASSERT(!g_pOleDoc);
    g_pOleDoc = new COleDocument();
    CK_ALLOC(g_pOleDoc);
    CKHR(g_pOleDoc->Initialize(mimeType, pParameters->isDebug));

    *ppInner = (LPUNKNOWN)((LPOLEDOCUMENT)g_pOleDoc);

    // Even though to be a good COM-izen we should AddRef here (since we
    // are returning an interface through an out parameter) we are
    // explictly not doing that because we are trying to set up aggregation
    g_pOleDoc->SetOuterObject(pParameters->pOuterObject);

    g_pOleDoc->SetApplicationIdentity(pParameters->pswzApplicationIdentity);
    g_pOleDoc->SetStartupUri(pParameters->pswzUri);
    g_pOleDoc->SetDebugSecurityZoneURL(pParameters->pswzDebugSecurityZoneURL);
    g_pOleDoc->SetHistoryStream(pParameters->pHistoryStream);
    g_pOleDoc->SetDocumentStream(pParameters->pDocumentStream);
    g_pOleDoc->SetDownloadCompletedEvent(pParameters->hDownloadCompletedEvent);
    g_pOleDoc->SetPersistHistory(pParameters->pPersistHistory);
    g_pOleDoc->SetErrorMessageToDisplay(pParameters->pswzErrorMessageToDisplay);

    // Activation continues with a call from the browser to IOleObject::SetClientSite() and then
    // IOleDocument::Navigate() or IOleObject::DoVerb().

Cleanup:
    HANDLE_ACTIVATION_FAULT(hr);
}

////////////////////

// Base class for "delegates" used to make calls to message filter methods like
// IIOleInPlaceFrame::TranslateAccelerator(MSG&) and IWpfHostSupport::FilterInputMessage(MSG&)
// on any thread.
class CMessageFilterDelegate
{
public:
    virtual HRESULT operator()(const MSG &msg) = 0;
private:
    CComPtr<IStream> m_spMarshalStream;
protected:
    template<typename IFilterObject>
    HRESULT Init(IFilterObject *pFilterObject)
    {
        ASSERT(pFilterObject && GetCurrentThreadId() == g_mainThreadId);
        HRESULT hr = CoMarshalInterThreadInterfaceInStream(__uuidof(IFilterObject), pFilterObject, &m_spMarshalStream);
        return hr;
    }
    ~CMessageFilterDelegate() { }

    // Unmarshals the captured interface proxy on another thread.
    HRESULT UnmarshalFilterObjectOnce(REFIID iid, void **ppObj)
    {
        if(*ppObj)
        {
            ASSERT(!m_spMarshalStream); // already released
            return S_FALSE;
        }
        ASSERT(GetCurrentThreadId() != g_mainThreadId);
        if(!m_spMarshalStream)
            return E_UNEXPECTED;
        HRESULT hr = CoUnmarshalInterface(m_spMarshalStream, iid, ppObj);
        m_spMarshalStream = 0; // release stream
        return hr;
    }
};

class CInPlaceFrameTranslateAcceleratorDelegate: public CMessageFilterDelegate
{
private:
    CComPtr<IOleInPlaceFrame> m_spFrameMTAProxy;
public:
    CInPlaceFrameTranslateAcceleratorDelegate(IOleInPlaceFrame *pFrame)
    {
        HRESULT hr = Init(pFrame);
        ASSERT(hr == S_OK); (void)hr;
    }

    /*override*/ HRESULT operator()(const MSG &msg)
    {
        HRESULT hr = UnmarshalFilterObjectOnce(IID_IOleInPlaceFrame, (void**)&m_spFrameMTAProxy.p);
        if(SUCCEEDED(hr))
        {
            hr = m_spFrameMTAProxy->TranslateAccelerator((LPMSG)&msg, 0);
        }
        return hr;
    }
};
CInPlaceFrameTranslateAcceleratorDelegate *g_pFrameMsgFilter;

class CWpfHostSupportFilterInputMessageDelegate: public CMessageFilterDelegate
{
private:
    CComPtr<IWpfHostSupport> m_spWpfHostSupportMTAProxy;
public:
    CWpfHostSupportFilterInputMessageDelegate(IWpfHostSupport *pWpfHostSupport)
    {
        HRESULT hr = Init(pWpfHostSupport);
        ASSERT(hr == S_OK); (void)hr;
    }

    /*override*/ HRESULT operator()(const MSG &msg)
    {
        HRESULT hr = UnmarshalFilterObjectOnce(IID_IWpfHostSupport, (void**)&m_spWpfHostSupportMTAProxy.p);
        if(SUCCEEDED(hr))
        {
            hr = m_spWpfHostSupportMTAProxy->FilterInputMessage((LPMSG)&msg);
        }
        return hr;
    }
};
CWpfHostSupportFilterInputMessageDelegate *g_pHostMsgFilter;


// Helper class used by ForwardTranslateAccelerator() to make calls to message filters on a different
// thread in order to avoid loss of keyboard messages. COM's standard message loop during outbound
// calls from the STA is replaced with one that keeps input messages in the queue and still allows
// reentrant calls.
class CMessageFilterMarshaler
{
    // Auto-reset event used by the thread-pool thread to signal that the call to the message filter
    //  has completed.
    static HANDLE s_hMsgFilterEvent;

    struct CFilterCallParams
    {
        CMessageFilterDelegate *m_pMsgFilter;
        const MSG *m_pMsg;
        HRESULT m_filterResult;
    };

    CMessageFilterMarshaler() { }

    static DWORD WINAPI CallMessageFilterThreadProc(LPVOID pData)
    {
        ASSERT(pData);
        CFilterCallParams &callParams = *(CFilterCallParams*)pData;
        ASSERT(callParams.m_pMsgFilter && callParams.m_pMsg);
        callParams.m_filterResult = (*callParams.m_pMsgFilter)(*callParams.m_pMsg);

        // Reentrancy is not expected. The event should be non-signaled.
        ASSERT(WaitForSingleObject(s_hMsgFilterEvent, 0/*no wait, just test*/) == WAIT_TIMEOUT);
        // Release the main thread.
        BOOL res = SetEvent(s_hMsgFilterEvent);
        ASSERT(res);
        return res;
    }

public:
    static HRESULT Call(CMessageFilterDelegate &filter, const MSG &msg)
    {
        HRESULT hr = S_FALSE;

        if(!s_hMsgFilterEvent)
        {
            s_hMsgFilterEvent = CreateEvent(0, false, false, 0); // non-signaled, auto-reset
            CHECK_NULL_FROM_WIN32(s_hMsgFilterEvent);
        }

        CFilterCallParams callParams;
        callParams.m_pMsgFilter = &filter;
        callParams.m_pMsg = &msg;
        // The event should remain non-signaled at all times.
        ASSERT(WaitForSingleObject(s_hMsgFilterEvent, 0/*no wait, just test*/) == WAIT_TIMEOUT);
        CHECK_BOOL_FROM_WIN32(QueueUserWorkItem(CallMessageFilterThreadProc, &callParams, 0));

        // Loop in lieu of ole32!ModalLoop() and ole32!CCliModalLoop::BlockFn().

        // The extra msg info needs to be preserved, in case there is some nested message dispatching.
        // In particular, it's known to be needed by the stylus input system.
        LPARAM msgInfo = GetMessageExtraInfo();
        for(;;)
        {
            // Only messages used by COM to dispatch STA (reentrant) calls are dispatched here.
            // Also paint messages. Calls to the filter method can block on a modal dialog. Moving
            // the dialog requres repaining of our windows. Actually, only Windows Server 2003 seems
            // to require that. XP and Vista seem to dispatch paint messages during the below call to
            // PeekMessage(), even though the requested message range is different and paint messages
            // are posted, not sent!
            DWORD res = MsgWaitForMultipleObjects(
                1, &s_hMsgFilterEvent, false/*== wait for msg or event*/, INFINITE,
                QS_SENDMESSAGE | QS_POSTMESSAGE | QS_PAINT);
            if(res == WAIT_OBJECT_0)
            {
                hr = callParams.m_filterResult;
                break;
            }
            ASSERT(res == 1); // message available
            MSG msg;
            const UINT WM_OLE_FIRST = WM_USER, WM_OLE_LAST = WM_USER + 8;
            // The target window is not checked. So, we might dispatch a little more than
            // intended, but this won't necessarily cause a problem. Whoever posts messages rarely
            // makes hard assumptions about when they will be dispatched.
            // COM message windows have names like "OleMainThreadWndClass" and "OleObjectRpcWindow".
            // In a similar situation, the CLR enumarates all such windows and dispatches only their
            // messages. See sources\ndp\clr\src\vm\threads.cpp, NonNT5WaitRoutine(). This could be
            // done here as well should it ever become necessary.
            while(PeekMessage(&msg, 0, WM_OLE_FIRST, WM_OLE_LAST, PM_REMOVE))
            {
                DispatchMessage(&msg);
            }
            while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE | PM_QS_PAINT))
            {
                DispatchMessage(&msg);
            }
        }
        SetMessageExtraInfo(msgInfo);

    Cleanup:
        return hr;
    }
};
HANDLE CMessageFilterMarshaler::s_hMsgFilterEvent;


HRESULT ForwardTranslateAccelerator_export(MSG* pMsg, VARIANT_BOOL appUnhandled)
{
    return ForwardTranslateAccelerator(pMsg, appUnhandled);
}

HRESULT ForwardTranslateAccelerator(MSG* pMsg, VARIANT_BOOL appUnhandled)
{
    // Translating Accelerators
    // We have to translate accelerators in a specific order.
    //
    // #0 - Start Avalon if custom startup message received.
    // #0.5 - Let an ActiveX control hosted by CColeDocument handle messages.
    // #1 - Let the input filter see the message.
    // #2 - Give our InPlaceActiveObject a chance
    // #3 - For some specific accelerators, pass them directly to the frame
    // #4 - Give OLE a chance
    // #5 - Direct message post for exceptional cases
    //
    // See the #n notes below.
    //
    // If any of the translations returns NOERROR, we're done with the message.
    //
    // ForwardTranslateAccelerator sees any input message first. If the message is not reserved
    // for the browser frame, it is given to the application. If it goes unhandled there,
    // this function gets it again, with appUnhandled=true.
    //
    /*
    **ISSUE**: Making COM calls out of process has the potential to lose keyboard input messages
    (WOSB 1757397). This happens because we are in an STA and COM runs a local message loop while
    waiting for the call to complete. It selectively dispatches some messages but explicitly drops
    key and mouse input ones. (This is presumably done to prevent unexpected reentrancy into the
    caller.) That code is in ole32!CCliModalLoop::HandleWakeForMsg(), com\ole32\com\dcomrem\callctrl.cxx.

    To avoid this problem here, calls to IWpfHostSupport::FilterInputMessage() and IOleInPlaceFrame::
    TranslateAccelerator() are made from a different thread, while the current one waits and only
    dispatches messages used by COM (to enable reentrant calls--this definitely happens). Key input
    messages remain in the queue, thus ensuring reliable type-ahead.

    Alternative solutions that didn't work:
    - Implementing IMessageFilter. If IMessageFilter::MessagePending() dispatches messages, Avalon may
      see them out of order (because it hasn't yet seen the one for which the outgoing call is made).
      Alternatively, PENDINGMSG_WAITNOPROCESS could be returned, but it doesn't work as advertised.
      It has the same effect as PENDINGMSG_WAITDEFPROCESS, which eats input messages.
    - Making the call as input-synchronous (by using the IDL [input_sync] attribute). Then COM won't
      eat input messages. But both current callees need to make calls back into PresentationHost
      before returning, which leads to RPC_E_CANTCALLOUT_ININPUTSYNCCALL or a deadlock because
      reentrant calls are not dispatched.
    */

    COleDocument *pOleDoc = COleDocument::GetInstance();
    Assert(pOleDoc); // Should never be NULL if we are here, processing key messages

    // #0 - A custom WM_NOTIFY message is posted whenever a startup-related event is fired or
    //      asynchronous continuation is needed.
    //      This message was introduced originally for XPSViewer, to avoid blocking IE during UI-Activation.
    //          "This is a custom message and not an input event, but is implemented in
    //          this way to avoid modifying the shim-dll version interface too close to
    //          shipping.  WindowsOSBugs#1580216 will be used to track refactoring this
    //          code in the next release."
    //      There's now the new use for internal callbacks. Once the XPSViewer code (ByteRangeDownloader)
    //      is removed or the suggested refactoring is done, the message could be posted and handled
    //      internally within CCOleDocument. (It owns the "top" DocObject window and has a WndProc.)
    if (pMsg->message == WM_NOTIFY && pMsg->wParam == AVALON_STARTUP_CHECK)
    {
        pOleDoc->OnStartupCallback();
        return NOERROR;
    }

    // #0.5 - Let an ActiveX control hosted by CColeDocument handle messages. This is currently needed for
    //   the HTMLDocument control used to show the deployment progress page. (The unhandled exception page
    //   also uses HTMLDocument, but it runs its own message loop, and from there it calls
    //   ForwardTranslateAccelerator().)
    if (pOleDoc->GetHostedInputObject())
    {
        if (pOleDoc->GetHostedInputObject()->TranslateAcceleratorIO(pMsg) == NOERROR)
            return NOERROR;
    }

    BOOL isKeyboard = pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_IME_KEYLAST;
    BOOL isMouse = pMsg->message >= WM_MOUSEFIRST && pMsg->message <= WM_MOUSELAST;
    BOOL isOther = pMsg->message == WM_INPUT;
    // If it isn't anything that we care about, exit now.
    if (!(isKeyboard || isMouse || isOther))
    {
        return S_FALSE;
    }

    HRESULT hr;

    // #1 - Allow the host's input filter to get all messages first, no matter what.
    if (!appUnhandled && g_bFilterInputImplemented && pOleDoc->GetWpfHostSupport())
    {
        bool isBackspace = pMsg->wParam == VK_BACK &&
            (pMsg->message == WM_KEYDOWN || pMsg->message == WM_KEYUP || pMsg->message == WM_CHAR) &&
            (GetKeyState(VK_CONTROL) >= 0) &&
            (GetKeyState(VK_MENU) >= 0);

        IBrowserHostServices *pAppHelper = pOleDoc->ApplicationHelper();
        if (isBackspace && pAppHelper && pAppHelper->FocusedElementWantsBackspace())
        {
            // Don't send the backspace to the host
            ASSERT(isBackspace);
        }
        else
        {
            if(!g_pHostMsgFilter)
            {
                g_pHostMsgFilter = new CWpfHostSupportFilterInputMessageDelegate(pOleDoc->GetWpfHostSupport());
            }
            hr = CMessageFilterMarshaler::Call(*g_pHostMsgFilter, *pMsg);
            if (hr == NOERROR)
                return NOERROR;
            else if (hr == E_NOTIMPL)
                g_bFilterInputImplemented = FALSE;
        }
    }

    // At this point, since the input filter (if any) has seen the message, all we are concerned
    // about are keyboard and input messages, we don't care about the mouse anymore.
    if (!isKeyboard)
    {
        return S_FALSE;
    }

    COleInPlaceActiveObject* pInPlaceActiveObject = pOleDoc->GetInPlaceActiveObject();
    if (pInPlaceActiveObject)
    {
        // #2 - Try our translator first, even though right now it does nothing. Someday we might
        //      have our own HACCEL table, or pass them on to the hosted app, so do the right thing now.

        if (pInPlaceActiveObject->TranslateAccelerator(pMsg) == NOERROR)
            return NOERROR;
    }

    COleDocumentView* pView = pOleDoc->GetDocumentView();
    if (pView)
    {
        // #3 - Some of the frame accelerators are not in its HACCEL (e.g. F5, Ctrl-D).
        //      OleTranslateAccelerator uses the frame's HACCEL, therefore it can't handle those.
        //      All application-unhandled non-character keys are given to the frame.

        bool fCallDefWindowProc = false;
        if (appUnhandled && (pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN || pMsg->message == WM_SYSCHAR)
            || IsKeyReservedForFrame(pMsg, fCallDefWindowProc))
        {
            if(fCallDefWindowProc)
            {
                DefWindowProc(pMsg->hwnd, pMsg->message, pMsg->wParam, pMsg->lParam);
                return NOERROR;
            }

            IOleInPlaceFrame *pInPlaceFrame = pView->GetInPlaceFrame(); // no AddRef
            if(pInPlaceFrame)
            {
                if(!g_pFrameMsgFilter)
                {
                    g_pFrameMsgFilter = new CInPlaceFrameTranslateAcceleratorDelegate(pInPlaceFrame);
                }
                if (CMessageFilterMarshaler::Call(*g_pFrameMsgFilter, *pMsg) == NOERROR)
                    return NOERROR;
            }
        }

        // #4 - OleTranslateAccelerator MUST be called, even though this application does
        //      not have an accelerator table.  This has to be done in order for the
        //      mnemonics for the top level menu items to work properly, e.g. pressing Alt-E
        //      to open and select the Edit menu.
        //      The hosted application is given first chance to handle its own access keys.
        //      (But F10 is reserved for switching to the frame's menu.)
        //      Alt+Space [access to the window ("system") menu] is also translated here.

        if (appUnhandled && pMsg->message == WM_SYSCHAR
                && pView->GetInPlaceFrame()
                && OleTranslateAccelerator(pView->GetInPlaceFrame(), pView->GetInPlaceFrameInfo(), pMsg) == NOERROR)
            return NOERROR;

        // #5 - F4, to get the address bar's drop-down list, is not recognized by any of the TranslateAccelerator
        //      functions above. Nornally, IE dispatches it straight from its message pump. That's why
        //      the keyboard message is posted to the frame window below.
        //      Fixed in IE7.
        if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_F4)
        {
            bool ctrlKeyDown = GetKeyState(VK_CONTROL) < 0;
            bool shiftKeyDown = GetKeyState(VK_SHIFT) < 0;
            if(!ctrlKeyDown && !shiftKeyDown)
            {
                PostMessage(pView->GetInPlaceFrameInfo()->hwndFrame, pMsg->message, pMsg->wParam, pMsg->lParam);
                return NOERROR;
            }
        }
    }

    return S_FALSE;
}

void Deactivate_export()
{
    if (g_pOleDoc)
    {
        delete g_pOleDoc;
        g_pOleDoc = NULL;
    }

    CMessageFilter::Unregister();

    delete g_pHostMsgFilter; g_pHostMsgFilter = 0;
    delete g_pFrameMsgFilter; g_pFrameMsgFilter = 0;
}

HRESULT SaveToHistory_export(__in_ecount(1) IStream* pHistoryStream)
{
    HRESULT hr          = S_OK;
    int     tempIndex   = NULL;
    LPWSTR  tempUrl     = NULL;
    LPWSTR  tempTitle   = NULL;

    CK_PARG(pHistoryStream);

    hr = SaveHistoryHelper(pHistoryStream, &tempIndex, &tempUrl, &tempTitle);
    CoTaskMemFree(tempUrl); CoTaskMemFree(tempTitle);

Cleanup:
    return hr;
}

HRESULT LoadFromHistory_export(IStream* pHistoryStream, IBindCtx* pBindCtx)
{
    HRESULT hr = S_OK;

    CK_PARG(pHistoryStream);

    hr = LoadHistoryHelper(pHistoryStream);

Cleanup:
    return hr;
}



/******************************************************************************
 SaveHistoryHelper

 Note: The return strings must be freed with CoTaskMemFree().
******************************************************************************/
HRESULT SaveHistoryHelper(__in IStream *saveStream,
                                           __out int *entryIndex,
                                           __out LPWSTR *uri,
                                           __out LPWSTR *title)
{
    ULONG           bytesWritten    = 0;
    HRESULT         hr              = S_OK;
    LPCWSTR          szStartupUri    = NULL;

    STARTUP_URI_LENGTH_TYPE    startupUriLen = 0;

    if(!g_pOleDoc->ApplicationHelper())
        return E_FAIL;

    // IMPORTANT if you change the order in which you write data, make the
    // corresponding change in LoadHistoryHelper

    // Store the number of bytes for the manifest path and then the manifest path itself
    szStartupUri = g_pOleDoc->GetStartupUri();
    if (szStartupUri)
    {
        size_t tmpLen = 0;
        CKHR(StringCchLengthW(szStartupUri, INTERNET_MAX_URL_LENGTH + 1, &tmpLen));
        startupUriLen = (ULONG)tmpLen;
    }

    CKHR(saveStream->Write((void*)&startupUriLen, sizeof(startupUriLen), &bytesWritten));
    bytesWritten = 0;
    if(startupUriLen)
        CKHR(saveStream->Write((void*)szStartupUri, startupUriLen * sizeof(WCHAR), &bytesWritten));
    bytesWritten = 0;

    MimeType mime = g_pOleDoc->GetCurrentMime();
    // Now write the mime type information so we can use it to reload the app from history correctly
    CKHR(saveStream->Write((void*)&mime, sizeof(mime), &bytesWritten));

    // Application will save the number of bytes and the actual bytes
    // m_bDeactivating == persistEntireJournal
    // When we are being deactivated, we get one final SaveHistory call and we should
    // persist the entire journal at this point. This codepath happens through IPersistHistory
    // rather than ITravelLogClient and shdocvw marks this entry as always navigable so we will
    // always be able to navigate to this from history.
    CKHR((g_pOleDoc->ApplicationHelper())->SaveHistory(saveStream, g_pOleDoc->Deactivating(), entryIndex, uri, title));

Cleanup:
    return hr;
}

HRESULT LoadHistoryHelper(__in IStream *loadStream)
{
    //Assert(loadStream);

    HRESULT             hr              = S_OK;
    ::STATSTG           stat            = { 0 };
    ULONG               pcbRead         = 0;
    wchar_t           *szStartupUri   = NULL;

    STARTUP_URI_LENGTH_TYPE startupUriLen = 0;

    //Get the size of the stream
    CKHR(loadStream->Stat(&stat, STATFLAG_NONAME));

    //Assert(stat);

    //Trident uses only LowPart as well
    if (stat.cbSize.HighPart || stat.cbSize.LowPart <= sizeof(startupUriLen))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //Read the number of bytes for the manifest filename
    CKHR(loadStream->Read((void*)&startupUriLen, sizeof(startupUriLen), &pcbRead));
    if (startupUriLen > INTERNET_MAX_URL_LENGTH)
    {
        CKHR(E_UNEXPECTED);
    }
    pcbRead = 0;

    //Read the manifest filename if one exists
    if (startupUriLen)
    {
        szStartupUri = new wchar_t[startupUriLen+1];
        CK_ALLOC(szStartupUri);

        //Read the bytes into the unmanaged buffer
        CKHR(loadStream->Read((void*)szStartupUri, startupUriLen * sizeof(wchar_t), &pcbRead));
        szStartupUri[startupUriLen] = '\0';
        pcbRead = 0;

        // If the MimeType is XPS and the Uri refers to a non-file address (http etc)
        // then download the file again.
        if ((g_pOleDoc->GetCurrentMime() == MimeType_Xps) &&
            (_wcsnicmp(szStartupUri, L"file:", 5) != 0))
        {
            LPSTREAM pDocStream = NULL;
            // This will make a blocking call to the szStartupUri and return an IStream to
            // the downloaded file into pDocStream
            CKHR(URLOpenBlockingStream(NULL, szStartupUri, &pDocStream, 0, NULL));
            // Set the IStream of the downloaded file as the Do----enStream.
            g_pOleDoc->SetDocumentStream(pDocStream);
        }
    }

    //Now read the mime type information so we can use it to reload the app from history correctly
    MimeType mime;
    CKHR(loadStream->Read((void*)&mime, sizeof(mime), &pcbRead));

    if (g_pOleDoc->IsAppLoaded())
    {
        //Verify if manifest path is same as the one that exists in the app.
        //If not, unload current app and start the new app?
        CKHR((g_pOleDoc->ApplicationHelper())->LoadHistory(loadStream));
    }

Cleanup:
    delete [] szStartupUri;
    return hr;
}

//******************************************************************************
//
// IsKeyReservedForFrame
//
// This determines if the key is reserved for the browser frame and
// thus the application should not be given a chance to eat it.
//
//******************************************************************************

BOOL IsKeyReservedForFrame(const MSG *pMSG, bool &fCallDefWindowProc)
{
    fCallDefWindowProc = false;

    //*** Alpha keys are likely to be localizable in Avalon and/or in IE. That's why they should not
    // be hard-coded here. Ctrl+T and Ctrl+W used to be treated as "sacred", but unfortunately can't be.

    // If the high-order bit is 1, the key is down; otherwise, it is up.
    // If the low-order bit is 1, the key is toggled. A key, such as the CAPS LOCK key, is toggled if it is turned on.
    BOOL isCtl = GetKeyState(VK_CONTROL) < 0;
    BOOL isShift = GetKeyState(VK_SHIFT) < 0;

    DWORD dwKeyCode = (DWORD)pMSG->wParam;

    // Control key
    if (isCtl && pMSG->message == WM_KEYDOWN)
    {
        if(!isShift)
        {
            switch (dwKeyCode)
            {
            case VK_F4: // Close tab/window
                return TRUE;
            }
        }

        if(dwKeyCode == VK_TAB) // Ctrl+[Shift+]Tab cycles between tabs.
            return TRUE;
    }

    // Alt key (or F10)
    else if (!isCtl && !isShift)
    {
        if(pMSG->message == WM_SYSKEYDOWN)
        {
            switch (dwKeyCode)
            {
            case VK_HOME:
            //case 'D': // Focus address bar -- This is localizable accelerator, shouldn't be hard-coded. WOSB 1486990.
                return TRUE;
            }
        }

        // 1. The default window procedure activates the window's menu if it sees F10 key down
        // immediately followed by key up.
        // 2. Alt+F4 closes window.
        if((pMSG->message == WM_SYSKEYUP || pMSG->message == WM_SYSKEYDOWN)
            && (dwKeyCode == VK_F4 || dwKeyCode == VK_F10))
        {
            fCallDefWindowProc = true;
            return TRUE;
        }
    }

    // No keyboard state modifiers
    else if (!isCtl && !isShift)
    {
        if (pMSG->message == WM_KEYDOWN)
        {
            switch (dwKeyCode)
            {
            case VK_F4: // get the address bar's dropdown
            case VK_ESCAPE: // stop download [does it really?]
                return TRUE;
            }
        }
    }

    return FALSE;
}

// This export is part of the workaround for exceptions coming from a thread other than the
// main one. See the managed DocObjHost.ProcessUnhandledException().
extern "C"
void __stdcall ProcessUnhandledException(__in_ecount(1) BSTR errorMsg)
{
    CErrorPage::Display(errorMsg);
}
