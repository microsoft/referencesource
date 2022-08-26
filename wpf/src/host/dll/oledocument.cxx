// +-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Implements the OleDocument class of PresentationHost.
//
//  History:
//     2002/06/12-murrayw
//          Created
//     2003/06/30-Microsoft
//          Ported ByteRangeDownloader to WCP
//     2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//     2007/12/xx-Microsoft
//          Introduced the native progress page, which required changing parts of the activation sequence.
//
// ------------------------------------------------------------------------

#include "Precompiled.hxx"
#include "OleDocument.hxx"
#include "DllMain.hxx"
#include "Bootstrapper.hxx"
#include "OleDocumentView.hxx"
#include "OleInPlaceObject.hxx"
#include "OleInPlaceActiveObject.hxx"
#include "OleObject.hxx"
#include "ConnectionPoint.hxx"
#include "TravelLogClient.hxx"
#include "HostSupport.h"
#include "CookieShim.hxx"
#include "Utilities.hxx"
#include "ErrorPage.hxx"
#include "DisableXappDocuments.hxx"
#include "WebBrowserEvents.hxx"

const UINT CALLBACK_TIMER_ID = 7;

COleDocument* COleDocument::s_pInstance;

/******************************************************************************
 COleDocument::COleDocument()
******************************************************************************/
COleDocument::COleDocument()
{
    DPF(8, _T("COleDocument::COleDocument(%08X)"), this);

    // If any future changes cause IPersistFile to be called without calling
    // IPersistMoniker first, we should allow the load so default to TRUE.
    // We'll reset it to FALSE in IPersistMoniker::Load and only set it to
    // TRUE if we intentionally fail the load in order to load through IPersistFile
    m_bAllowPersistFileLoad = TRUE;
    
    m_bSimpleStatusBar = TRUE;
    m_hwndStatusBar = NULL;

    m_pPersistHistory = NULL;

    m_hostingFlags = m_validHostingFlags = 0;
    m_ReadyState            = READYSTATE_UNINITIALIZED;
    m_CurrentMime           = MimeType_Unknown;

    Assert(s_pInstance == NULL);
    s_pInstance = this;
    m_pOuterObject = NULL;

    m_pWebBrowser = NULL;
    m_pWebBrowserMarshalStream = 0;
    m_pWpfHostSupport = NULL;

    m_bAvalonRunning = m_bDeactivating = m_bPostedShutdown = false;
    m_progMaxBytes = 0;
    m_bViewCreated = false;
    m_hwndTop = NULL;

    m_pByteWrapper = NULL;
    m_pDocumentStream = NULL;
    m_pHistoryStream = NULL;
    m_pOleDocView = NULL;
    m_pOleInPlaceObject = NULL;
    m_pOleInPlaceActiveObject = NULL;
    m_pOleObject = NULL;
    m_pTravelLogClient = NULL;
    m_pCPC = NULL;
    m_pBootStrapper = NULL;
    m_pAppBrowserHostServices = NULL;
    m_pByteRangeDownloaderService = NULL;
    m_pOleAdviseHolder = NULL;
    m_pWebBrowserEventsService = NULL;
    m_pWebBrowserEventSink = NULL;

    m_hMutexDialog = NULL;
    m_hMutexOK = NULL;
    m_hMutexCancel = NULL;

    m_hDownloadCompletedEvent = NULL;

    m_bstrProgressAssemblyName = m_bstrProgressClassName = m_bstrErrorAssemblyName = m_bstrErrorClassName = NULL;
}

/******************************************************************************
 COleDocument::Initialize()
******************************************************************************/
HRESULT COleDocument::Initialize(MimeType mimeType, bool bDebugMode)
{
    HRESULT hr = S_OK;

    ASSERT(mimeType != MimeType_Unknown);
    m_CurrentMime = mimeType;
    SetHostingFlag(hfInDebugMode, bDebugMode);

    m_pOleDocView             = new COleDocumentView(this);
    CK_ALLOC(m_pOleDocView);

    m_pOleInPlaceObject       = new COleInPlaceObject(this);
    CK_ALLOC(m_pOleInPlaceObject);

    m_pOleInPlaceActiveObject = new COleInPlaceActiveObject(this);
    CK_ALLOC(m_pOleInPlaceActiveObject);

    m_pOleObject              = new COleObject(this);
    CK_ALLOC(m_pOleObject);

    m_pTravelLogClient        = new CTravelLogClient(this);
    CK_ALLOC(m_pTravelLogClient);

    m_pCPC                    = new CConnectionPointContainer((IOleDocument*)this);
    CK_ALLOC(m_pCPC);   
    CKHR(m_pCPC->Init());    

    if(m_CurrentMime == MimeType_Application)
    {
        CKHR(m_progressPageHost.Init(this));
        // The progress page will be actually shown either by CCOleObject::DoVerb() if it detects cold start 
        // or by the managed hosting code if it finds that the application is run for the first time.
    }

Cleanup:

    if (FAILED(hr))
    {
        DeletePointers();
    }

    return hr;
}

/******************************************************************************
 COleDocument::~COleDocument()
******************************************************************************/
COleDocument::~COleDocument()
{
    DPF(8, _T("COleDocument::~COleDocument(%08X)"), this);
    DestroyWindow(m_hwndTop);
    if (m_hMutexDialog)
        CloseHandle(m_hMutexDialog);
    if (m_hMutexOK)
        CloseHandle(m_hMutexOK);
    if (m_hMutexCancel)
        CloseHandle(m_hMutexCancel);
    ReleaseHistoryStream();

    // Release the hosted application object
    ReleaseInterface(m_pAppBrowserHostServices);

    // Although the event handle is constructed in CHostShim, close it here
    // to avoid a race condition with the shim being destroyed.
    if (m_hDownloadCompletedEvent != NULL)
    {
        ::CloseHandle(m_hDownloadCompletedEvent);
        m_hDownloadCompletedEvent = NULL;
    }

    // release Stream interface
    ReleaseInterface(m_pHistoryStream);

    ReleaseInterface(m_pByteRangeDownloaderService);
    ReleaseInterface(m_pOleAdviseHolder);
    ReleaseInterface(m_pWebBrowser);
    if(m_pWebBrowserMarshalStream)
    {
        CoReleaseMarshalData(m_pWebBrowserMarshalStream);
        ReleaseInterface(m_pWebBrowserMarshalStream);
    }
    ReleaseInterface(m_pWebBrowserEventsService);
    ReleaseInterface(m_pWpfHostSupport);

    DeletePointers();
}

void COleDocument::DeletePointers()
{
    DEL_PTR(m_pOleDocView);
    DEL_PTR(m_pOleInPlaceObject);
    DEL_PTR(m_pOleInPlaceActiveObject);
    DEL_PTR(m_pOleObject);
    DEL_PTR(m_pTravelLogClient);
    DEL_PTR(m_pCPC);
}

bool COleDocument::GetHostingFlag(HostingFlags flag) const
{
    ASSERT((m_validHostingFlags & flag) == (unsigned)flag);
    return (m_hostingFlags & flag) != 0;
}
void COleDocument::SetHostingFlag(HostingFlags flag, bool value)
{
    ASSERT(flag);
    m_validHostingFlags |= flag;
    if(value)
    {
        m_hostingFlags |= flag;
    }
    else
    {
        m_hostingFlags &= ~flag;
    }
}

HRESULT COleDocument::SetClientSite(IOleClientSite *pSite)
{
    if(m_spOleClientSite)
    {
        m_spBrowserServiceProvider.Release();
        UninitDocHost();
    }
    m_spOleClientSite = pSite;
    if(pSite)
    {
        m_spBrowserServiceProvider = pSite; //QI

        // Connect further with the host browser...
        InitDocHost(); //[Watson on failure]

        // If we return uninitialized when the container queries us, then we will never get called
        // on IOleObject::DoVerb() which is where we start to load our application which will then
        // finally fire READYSTATE_COMPLETE.
        PostReadyStateChange(READYSTATE_INTERACTIVE);
    }
    return S_OK;
}

/******************************************************************************
  COleDocument::AvalonExecute(IStream *loadStream)
******************************************************************************/
HRESULT COleDocument::AvalonExecute(
    __in_ecount_opt(1) IStream *loadStream /*=NULL*/)
{
    HRESULT hr = S_OK;

    CKHR(SetHistoryStream(loadStream));

    if (m_pBootStrapper == NULL)
    {
        m_pBootStrapper = new BootStrapper(this);
        CK_ALLOC(m_pBootStrapper);
    }

    m_pBootStrapper->AvalonExecute(loadStream); //[Watson on failure]

Cleanup:
    return hr;
}
//******************************************************************************
//
// IUnknown Implementation
//
//******************************************************************************

/******************************************************************************
 COleDocument::QueryInterface
******************************************************************************/
STDMETHODIMP COleDocument::QueryInterface(__in_ecount(1) REFIID riid, __deref_out_ecount(1) LPVOID *ppReturn)
{
    HRESULT hr = E_NOINTERFACE;
    *ppReturn = NULL;

    if (riid == IID_IUnknown)
    {
        ASSERT(m_pOuterObject);
        *ppReturn = m_pOuterObject; // to preserve COM identity
    }
    else if (riid == IID_IOleDocument)
    {
        *ppReturn = this;
    }
    else if(riid == IID_IDispatch)
    {
        // Problem returning this interfaces
        *ppReturn = (IDispatch*)this;
    }
    else if (riid == IID_IOleCommandTarget)
    {
        *ppReturn = (IOleCommandTarget*)this;
    }
    else if(riid == IID_IHlinkTarget)
    {
        *ppReturn = (IHlinkTarget*)this;
    }
    else if (riid == IID_IOleDocumentView)
    {
        *ppReturn = m_pOleDocView;
    }
    else if (riid == IID_IOleInPlaceObject)
    {
        *ppReturn = m_pOleInPlaceObject;
    }
    else if (riid == IID_IOleInPlaceActiveObject)
    {
        *ppReturn = m_pOleInPlaceActiveObject;
    }
    else if (riid == IID_IInputObject)
    {
        *ppReturn = (IInputObject*)m_pOleInPlaceActiveObject;
    }
    else if (riid == IID_IOleObject)
    {
        *ppReturn = m_pOleObject;
    }
    else if(riid == IID_IBrowserCallbackServices)
    {
        *ppReturn = (IBrowserCallbackServices*)this;
    }
    else if(riid == IID_IConnectionPointContainer)
    {
        // 
        *ppReturn = m_pCPC;
    }
    else if(riid == IID_ITravelLogClient)
    {
        *ppReturn = (ITravelLogClient*)m_pTravelLogClient;
    }
    else if(riid == IID_IWebBrowserEventsUrlService)
    {
        *ppReturn = (IWebBrowserEventsUrlService*)m_pTravelLogClient;
    }
    else if (riid == IID_IPersistHistory)
    {
        if (m_pPersistHistory)
        {
            *ppReturn = m_pPersistHistory;
        }
    }

    if (*ppReturn)
    {
        AddRef();
        hr = S_OK;
    }
   
    return hr;
}                                             

/******************************************************************************
 COleDocument::AddRef
******************************************************************************/
STDMETHODIMP_(DWORD) COleDocument::AddRef()
{
    return m_pOuterObject->AddRef();
}

/******************************************************************************
 COleDocument::Release
******************************************************************************/
STDMETHODIMP_(DWORD) COleDocument::Release()
{
    return m_pOuterObject->Release();
}

//******************************************************************************
//
// IOleDocument Implementation
//
//******************************************************************************

/******************************************************************************
 COleDocument::CreateView()
******************************************************************************/
STDMETHODIMP COleDocument::CreateView(__in_ecount_opt(1) IOleInPlaceSite *pInPlaceSite, 
                                      __in_ecount_opt(1) IStream *pStream, 
                                      DWORD dwReserved, 
                                      __deref_out_ecount(1) IOleDocumentView **ppOleDocumentView)
{
    HRESULT  hr = E_FAIL;

    CK_PARG(ppOleDocumentView);
    
    // NULL the view pointer
    *ppOleDocumentView = NULL;

    // we only support one view, so fail if the view already exists
    if (!m_bViewCreated)
    {
        // AddRef since we are giving away the pointer
        m_pOleDocView->AddRef();

        // if we were given a site, set this as the site for the view we just created
        if (pInPlaceSite) 
        {
            m_pOleDocView->SetInPlaceSite(pInPlaceSite);
        }

        // if given a stream to initialize from, initialize our view state
        if (pStream) 
        {
            m_pOleDocView->ApplyViewState(pStream);
        }

        *ppOleDocumentView = m_pOleDocView;

        m_bViewCreated = TRUE;           
        hr = S_OK;
    }
     
Cleanup:  
    return hr;
}

/******************************************************************************
 COleDocument::GetDocMiscStatus()
******************************************************************************/
STDMETHODIMP COleDocument::GetDocMiscStatus(__out_ecount(1) DWORD *pdwStatus)
{
    HRESULT hr = S_OK;

    CK_PARG(pdwStatus);

    *pdwStatus = 0;

Cleanup:
    return hr;
}

/******************************************************************************
 COleDocument::EnumViews()
******************************************************************************/
STDMETHODIMP COleDocument::EnumViews(__deref_out_ecount(1) IEnumOleDocumentViews **ppEnum, 
                                     __deref_out_ecount(1) IOleDocumentView **ppView)
{

    if(ppEnum)
    {    
        *ppEnum = NULL;
    }
    if(ppView)
    {
        *ppView = NULL;
    }

    return E_NOTIMPL;
}

//******************************************************************************
//
//  IDispatch Implementation
//
//******************************************************************************
//
// Right now the only client for this is shdocvw, which
// uses it to get our readystate. consequently, that's the
// only part of IDispatch currently implemented. we should
// consider doing a full-on implementation if other people
// are going to use this.
//

/******************************************************************************
  COleDocument::GetTypeInfoCount()
******************************************************************************/
STDMETHODIMP COleDocument::GetTypeInfoCount(__out_ecount(1) UINT * pctinfo)
{
    if (pctinfo)
    {
        *pctinfo = 0;
    }
    return E_NOTIMPL;
}
        
/******************************************************************************
  COleDocument::GetTypeInfo()
******************************************************************************/
STDMETHODIMP COleDocument::GetTypeInfo(UINT, LCID, __deref_out_ecount(1) ITypeInfo ** typeInfo)
{
    if (typeInfo)
    {
        *typeInfo = NULL;
    }
    return E_NOTIMPL;
}

/******************************************************************************
  COleDocument::GetIDsOfNames()
******************************************************************************/
STDMETHODIMP COleDocument::GetIDsOfNames(__in_ecount(1) REFIID, __deref_in_ecount(cNames) LPOLESTR *, UINT cNames, LCID, __out_ecount(cNames) DISPID* rgDispId)
{
    return E_NOTIMPL;
}

/******************************************************************************
  COleDocument::Invoke()
******************************************************************************/
STDMETHODIMP COleDocument::Invoke(DISPID dispidMember,
                                  __in_ecount(1) REFIID, LCID, WORD, __in_ecount(1) DISPPARAMS *,
                                  __out_ecount(1) VARIANT * pVarResult,
                                  __out_ecount_opt(1) EXCEPINFO * pExcepInfo, __out_ecount_opt(1) UINT * puArgErr)
{
    HRESULT hr = DISP_E_MEMBERNOTFOUND;

    CK_PARG(pVarResult);
    
    switch(dispidMember)
    {
    case DISPID_READYSTATE:
        {
            V_VT(pVarResult) = VT_I4;
            V_I4(pVarResult) = m_ReadyState;
            hr = S_OK;
            break;
        }
    }

Cleanup:
    return hr;
}

LRESULT COleDocument::QueryStatus(HMENU hMenu)
{
    if (hMenu == NULL)
        return 1;

    // Comparing menu handles instead of getting menus by Pos (LOWORD(lParam)) is better long term, 
    // If the order of the menus is changed, or container/obj adds more menus etc, this will ensure 
    // the code still works correctly. 

    // Return 1 if this is not one of our menus
    if (GetMenuFromID(m_pOleDocView->m_hDocObjMenu, IDR_EDITMENU) != hMenu &&
        GetMenuFromID(m_pOleDocView->m_hDocObjMenu, IDR_VIEWMENU) != hMenu)
        return 1;

    return QueryStatusHelper(hMenu);

}

LRESULT COleDocument::QueryStatusHelper(HMENU hMenu)
{
    OLECMD *cmdArray    = NULL;
    HRESULT hr;

    // Separator is also part of the Count, it has a unique system value and will get ignored by our code
    UINT cCmds;
    CKHR(IntToUInt(GetMenuItemCount(hMenu), &cCmds));

    // handle sub-menus
    for (UINT index=0; index<cCmds; index++)
    {
        HMENU subMenu = GetSubMenu(hMenu, index);
        if (subMenu != NULL)
        {
            QueryStatusHelper(subMenu);
        }
    }

    cmdArray = new OLECMD[cCmds];

    // Fill the OLECMD array with the MenuIDs
    for (UINT i = 0; i < cCmds; i++)
    {
        // 


        cmdArray[i].cmdID   = GetMenuItemID(hMenu, i);
        cmdArray[i].cmdf    = 0;
    }
    
    // If the call failed, we should fall through and disable the menu items
    QueryStatus((const GUID*)&CGID_ApplicationCommands, cCmds, cmdArray, NULL);

    UpdateMenuItems(hMenu, cCmds, cmdArray);

    delete [] cmdArray;

Cleanup:
    if (FAILED(hr))
    {
        return 1;
    }
    // We handled it
    return 0;
}

void COleDocument::Exec(DWORD dwCmdId)
{
    Exec((const GUID*)&CGID_ApplicationCommands, dwCmdId, 0, NULL, NULL);
}

//******************************************************************************
//
// IOleCommandTarget Implementation
//
//******************************************************************************

bool IsRecognizedCommandGroup(const GUID * pguidCmdGroup)
{
    return pguidCmdGroup == NULL /*standard OLE commands*/
        || *pguidCmdGroup == CGID_ApplicationCommands || *pguidCmdGroup == CGID_EditingCommands;
}

/******************************************************************************
  COleDocument::QueryStatus()
******************************************************************************/
STDMETHODIMP
COleDocument::QueryStatus(__in_ecount(1) const GUID * pguidCmdGroup,
                          ULONG cCmds,
                          __inout_ecount(cCmds) OLECMD *prgCmds,
                          __in_ecount_opt(1) OLECMDTEXT * /*pCmdText*/)
{
    HRESULT             hr      = OLECMDERR_E_UNKNOWNGROUP;
    ULONG               i       = 0;

    CK_PARG(prgCmds);
 
    EventWriteWpfHostUm_OleCmdQueryStatusStart(pguidCmdGroup ? pguidCmdGroup->Data1 : 0, prgCmds->cmdID);

    /***IMPORTANT:
      Make sure to return allowed and appropriate values according to the specification of 
      IOleCommandTarget::QueryStatus(). In particular:
        - OLECMDF_SUPPORTED without OLECMDF_ENABLED should not be blindly returned for 
            unrecognized commands.
        - Some code in IE treats OLECMDERR_E_xxx differently from generic failures.
        - E_NOTIMPL is not an acceptable return value.

    !!! Use only documented commands from the CGID_Explorer and CGID_ShellDocView groups.
    */ 

    if (this->ApplicationHelper() != NULL && IsRecognizedCommandGroup(pguidCmdGroup))
    {
        for (i=0; i < cCmds; i++)
        {
            DWORD dwFlags = 0;
            // if this is one of the 2 of our menu items that we do not forward to managed
            // code then we want them to always be enabled.
            if ((prgCmds[i].cmdID == IDM_VIEW_STATUSBAR || prgCmds[i].cmdID == IDM_VIEW_FULLSCREEN)
                && pguidCmdGroup != NULL && IsEqualGUID(CGID_ApplicationCommands,*pguidCmdGroup))
            {
                prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                hr = S_OK;
            }
            else if (prgCmds[i].cmdID == OLECMDID_PRINTPREVIEW && pguidCmdGroup == NULL)
            {
                // Print Preview is not supported because after menu merging it 
                // inexplicably gains the keyboard shortcut Ctrl-V
                prgCmds[i].cmdf = 0;
                hr = S_OK;
            }
            else
            {
                CKHR(ApplicationHelper()->QueryStatus((GUID *)pguidCmdGroup, prgCmds[i].cmdID, &dwFlags));
                prgCmds[i].cmdf = dwFlags;
            }
        }
    }

Cleanup:
    EventWriteWpfHostUm_OleCmdQueryStatusEnd(prgCmds ? prgCmds->cmdf : DWORD(-1), hr);

    return hr;
}

/******************************************************************************
  COleDocument::Exec()
******************************************************************************/
STDMETHODIMP 
COleDocument::Exec(__in_ecount_opt(1) const GUID * pguidCmdGroup,
                   DWORD dwCmdId, DWORD nCmdExecOpt,
                   __in_ecount_opt(1) VARIANT * pvarArgIn,
                   __out_ecount_opt(1) VARIANT * pvarArgOut)
{
    EventWriteWpfHostUm_OleCmdExecStart(pguidCmdGroup ? pguidCmdGroup->Data1 : 0, dwCmdId);

    HRESULT hr = OLECMDERR_E_UNKNOWNGROUP;
    bool handled = false;
    IInternetSecurityManager * pSecurityMgr = NULL;    
    
    if(CErrorPage::IsActive())
    {
        if(CErrorPage::ExecOleCommand(pguidCmdGroup, dwCmdId, nCmdExecOpt, pvarArgIn, pvarArgOut))
            return S_OK;
#ifdef DEBUG
        DebugPrintf(0, L"Ignored OLE command %d in error mode, CmdGroup.Data1=%0X", 
            dwCmdId, pguidCmdGroup ? pguidCmdGroup->Data1 : 0);
#endif
        return OLECMDERR_E_UNKNOWNGROUP;
    }
    if (ApplicationHelper() == NULL)
    {
        hr = OLECMDERR_E_UNKNOWNGROUP;
        goto Cleanup; // This is important for startup profiling. The ETW pair is closed at Cleanup.
    }

    //!!
    //!! Use only documented commands from the CGID_Explorer and CGID_ShellDocView groups.
    //!!

    if (pguidCmdGroup != NULL)
    {
        if (*pguidCmdGroup == CGID_ApplicationCommands)
        {
            handled = HandleMenuCommand(dwCmdId);
        }
        else if (*pguidCmdGroup == CGID_Explorer)
        {
            switch(dwCmdId)
            {
            case 39: // SBCMDID_MIXEDZONE [documented but not in a public header yet]
                {
                /*
                When we are the primary document IE will ask us what zone to display. 
                pvarArg->vt = VT_UI4, specified zone
                pvarArg->vt = VT_NULL, "unknown zone (mixed)"
                Leaving pvarArg unset, "unknown zone"
                */
                CKHR(CoCreateInstance(CLSID_InternetSecurityManager, NULL, CLSCTX_INPROC_SERVER, IID_IInternetSecurityManager, (void**)&pSecurityMgr));
                DWORD zone;
                CKHR(pSecurityMgr->MapUrlToZone(GetStartupUri(), &zone, 0));
                VariantInit(pvarArgOut);
                pvarArgOut->vt = VT_UI4;
                pvarArgOut->ulVal = zone;
                pSecurityMgr->Release();
                pSecurityMgr = NULL;
                handled = true;
                break;
                }
            }
        }
        else if (*pguidCmdGroup == CGID_PresentationHost)
        {
            switch(dwCmdId)
            {
            case PHCMDID_TABINTO:
                {
                /*
                Command sent by the Mozilla plugin's CDocObjectHostWindow::CTabIntoDocObjectWindow::OnSetFocus
                to tab into the doc object. nDir < 0 for backwards; nDir >0 for forward.
                In IE, tabbing in is handled in two different places: COleObject::DoVerb() and 
                COleInPlaceActiveObject::TranslateAcceleratorIO().
                */
                int nDir = nCmdExecOpt;
                Assert(nDir != 0);
                ApplicationHelper()->TabInto(nDir > 0);
                handled = true;
                break;
                }
            }
        }
    }
    else  // Standard OLE command
    {
        switch(dwCmdId)
        {
        case OLECMDID_WINDOWSTATECHANGED:
            // We get this when switching tabs in IE7. The RBW needs to be focused.
            // Also notified when a modal dialog is opened or closed and when going in and out of
            // full-screen mode.
            if(nCmdExecOpt & OLECMDIDF_WINDOWSTATE_USERVISIBLE_VALID)
            {
                // When a modal dialog is created, it may disable the RBW. Trying to set focus would fail then.
                bool active = (nCmdExecOpt & OLECMDIDF_WINDOWSTATE_USERVISIBLE) != 0;
                bool enabled = (nCmdExecOpt & OLECMDIDF_WINDOWSTATE_ENABLED_VALID)
                            && (nCmdExecOpt & OLECMDIDF_WINDOWSTATE_ENABLED);
                if(!active || enabled)
                {
                    m_pOleInPlaceActiveObject->OnFrameWindowActivate(active);
                }
            }
            handled = true;
            break;
        case OLECMDID_ONUNLOAD:
            // It's too early to call PostShutdown() here. The managed code may be needed to handle
            // IPersistHistory::SaveHistory(). The flag is set so that SaveHistoryHelper() knows to save
            // the entire journal (if navigating away).
            m_bDeactivating = true;
            handled = true;
            break;
        }
    }

    // If we do not know how to handle this message we pass the message along to 
    // the managed side where it can be turned into an avalon command and possibly
    // handled.
    if (!handled && IsRecognizedCommandGroup(pguidCmdGroup))
    {
        VARIANT varArgIn;

        VariantInit(&varArgIn);
        if (pvarArgIn != NULL)
        {
            CKHR(VariantCopy(&varArgIn, pvarArgIn));
        }

        hr = (ApplicationHelper())->ExecCommand((GUID *)pguidCmdGroup, dwCmdId, varArgIn);
        goto Cleanup;
    }

    hr = handled ? S_OK : OLECMDERR_E_UNKNOWNGROUP;

Cleanup:
    EventWriteWpfHostUm_OleCmdExecEnd(hr);

    if (pSecurityMgr != NULL)
    {
        pSecurityMgr->Release();
    }

    return hr;
}


/******************************************************************************
  COleDocument::HandleMenuCommand()
******************************************************************************/
bool COleDocument::HandleMenuCommand(DWORD cmdId)
{

    HRESULT  hr;

    // Any menu command that should be handled in native code and not forwarded
    // to managed is handled here.
    switch (cmdId)
    {
    case IDM_VIEW_STATUSBAR:
        CKHR(ToggleItemState(&IWebBrowser2::get_StatusBar, 
                             &IWebBrowser2::put_StatusBar, 
                             IDM_VIEW_STATUSBAR));
        break;

    case IDM_VIEW_FULLSCREEN:
        // Even though the menu item says "full screen" we want to use the TheaterMode api. 
        // True full screen does not include the toolbar at the top and has no access to menu items
        CKHR(ToggleItemState(&IWebBrowser2::get_TheaterMode,
                              &IWebBrowser2::put_TheaterMode,
                              IDM_VIEW_FULLSCREEN));
        break;

    default:
        return false;
    }

Cleanup:
    // If we attempted to handle the command here then we return true so it will not 
    // be forwarded to managed code.  Even if we failed and hr != S_OK we still want
    // to indicate that we were supposed to handle this message.
    return true;
}

/******************************************************************************
  COleDocument::ToggleItemState()
******************************************************************************/
STDMETHODIMP 
COleDocument::ToggleItemState(IWebBrowser2_Get_Method getVal, 
                               IWebBrowser2_Set_Method setVal,
                               DWORD menuItem)
{
    HRESULT hr = S_OK;
    IWebBrowser2        * pWebBrowserTop    = NULL;
    VARIANT_BOOL varTemp;

    CKHR(GetTopLevelBrowser(m_spOleClientSite, &pWebBrowserTop));

    // get current value by invoking the get method
    CKHR(((*pWebBrowserTop).*(getVal))(&varTemp));
    if (varTemp == VARIANT_TRUE)
    {
        // set browser state
        CKHR(((*pWebBrowserTop).*(setVal))(VARIANT_FALSE));
        // set menu item check state
        CheckMenuItem(m_pOleDocView->m_hViewMenu, menuItem, MF_BYCOMMAND | MF_UNCHECKED);
    }
    else 
    {
        // set browser state
        CKHR(((*pWebBrowserTop).*(setVal))(VARIANT_TRUE));
        // set menu item check state
        CheckMenuItem(m_pOleDocView->m_hViewMenu, menuItem, MF_BYCOMMAND | MF_CHECKED);
    }

Cleanup:
    ReleaseInterface(pWebBrowserTop);

    return hr;
}

//******************************************************************************
//
//  IHlinkTarget Implementation
//
//******************************************************************************

/******************************************************************************
  COleDocument::SetBrowseContext()
******************************************************************************/
STDMETHODIMP COleDocument::SetBrowseContext(__in_ecount_opt(1) IHlinkBrowseContext *pihlbc)
{
    return E_NOTIMPL;
}

/******************************************************************************
  COleDocument::GetBrowseContext()
******************************************************************************/
STDMETHODIMP COleDocument::GetBrowseContext(__deref_out_opt IHlinkBrowseContext **ppihlbc)
{
    if(ppihlbc)
    {
        *ppihlbc = NULL;
    }
    return E_NOTIMPL;
}

/******************************************************************************
  COleDocument::Navigate()
******************************************************************************/
STDMETHODIMP COleDocument::Navigate(DWORD grfHLNF,__in_ecount_opt(INTERNET_MAX_URL_LENGTH+1) LPCWSTR pwzJumpLocation)
{
    EventWriteWpfHostUm_IHlinkTargetNavigateStart(pwzJumpLocation);

    HRESULT hr = S_OK;
    
    CKHR(SetStartupLocation(pwzJumpLocation));

    // Normally, IE calls Navigate(), which is expected to initiate UI-Activation. However, on history
    // navigation, IE calls DoVerb() directly and doesn't call Navigate().
    //
    // The Netscape plug-in calls IHlinkTarget::Navigate() before setting the client site.
    // It has to do this because it uses ATL to host PresentationHost as an ActiveX control, and so DoVerb()
    // is called directly, right after setting the client site. But our implemention here needs to have the 
    // URL #fragment before UI activation.
    //
    if(!m_spOleClientSite)
    {
        hr = S_FALSE;
    }
    else
    {
        CKHR(m_pOleObject->DoVerb(OLEIVERB_SHOW, NULL, m_spOleClientSite, 0, NULL, NULL));
    }

    // 






    EventWriteWpfHostUm_IHlinkTargetNavigateEnd();

Cleanup: 
    HANDLE_ACTIVATION_FAULT(hr);
    return hr;
}

/******************************************************************************
  COleDocument::ValidateSecurityInFrame()
******************************************************************************/
HRESULT COleDocument::ValidateSecurityInFrame()
{
    ASSERT(GetHostingFlag(hfHostedInFrame));

    HRESULT hr = S_OK;
    DWORD   dwPolicy = 0;
    DWORD   dwError = ERROR_CANCELLED; 
    IOleWindow* pOleWindow = NULL;
    HWND hwnd = NULL;        //HWND to container window
    BSTR bstrTopUri = NULL;    //Top level URL 
    IInternetSecurityManager *pSecMgr = NULL;
    IOleCommandTarget* pOleCmdTarget = NULL;
    boolean isTopLevel;
              
    // First check if the security manager needs to have the zone crossing
    // confirmed for mixed content based on the user settings.
    // At this point, trident has already shown the secure to unsecure 
    // redirection or unsecure to secure redirection dialog but phost.exe need
    // to pop the mixed content dialog if we are hosting unsecure frame
    // in a secure page.

    CKHR(IsUrlSecure(GetStartupUri()));
    if (S_OK == hr)
    {
        // going to a secure site, no need to display the dialog
        goto Cleanup;
    }

    // Get the top level URL and parse it to see if it is a secure URL, if it is secure
    // then we pop the necessary dialogs based on security policy. 
    CKHR(m_pWebBrowser->DetermineTopLevel(&isTopLevel, &bstrTopUri));
    // Firefox doesn't return the top-level URL if it's a local filepath and the plug-in is in an 
    // Internet-URL frame.
    if(!(bstrTopUri && *bstrTopUri))
        goto Cleanup;
    CKHR(IsUrlSecure(bstrTopUri));
    if (S_FALSE == hr)
    {
        // the originating site is not secure anyway, no need to show dialog
        hr = S_OK;
        goto Cleanup;
    }
 
    // We are moving from secure(top browser) to unsecure(frame), need to query the security policy and see 
    // if we need to pop the mixed content dialog
         
    CKHR((CoCreateInstance(CLSID_InternetSecurityManager, 
                            NULL, CLSCTX_INPROC_SERVER,
                            IID_IInternetSecurityManager, 
                            (void **)&pSecMgr)));
        
    pSecMgr->ProcessUrlAction(GetStartupUri(),
                               URLACTION_HTML_MIXED_CONTENT, 
                               (BYTE *) &dwPolicy, 
                               sizeof(dwPolicy),
                               NULL,
                               0,
                               PUAF_NOUI,
                               NULL);
            

    if (dwPolicy == URLPOLICY_QUERY)
    {
        //
        // Prompt the user to determine if an action is allowed
        //

        CKHR(m_spOleClientSite->QueryInterface(IID_IOleWindow, (void**)&pOleWindow));
        CKHR(pOleWindow->GetWindow(&hwnd));
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd,&pid);
        
        // Use IE PrcoessId as the name of the mutex coz we want one 
        // dialog box per IE process to match the behavior for mshtml. 
        // the ProcessId returned from GetWindowThreadProcessId is 32bit
        // unsigned long (DWORD) and the range is from 0 to 4,294,967,295

        // 

        
        const size_t cMutexLength = 12; // 10 for pid + 1 'd' for dialog  + 1 null char
        
        WCHAR wszDlgMtxName[cMutexLength]; 
        WCHAR wszCancelMtxName[cMutexLength]; 
        WCHAR wszOKMtxName[cMutexLength];
        CK_PTR(_ultow_s(pid, wszDlgMtxName, 10) == 0);
        size_t length = 0;
        CKHR(StringCchLength(wszDlgMtxName, cMutexLength, &length));
        CKHR(StringCchCat(wszDlgMtxName, cMutexLength, L"d"));

        CK_PTR(_ultow_s(pid, wszOKMtxName, 10) == 0);
        CKHR(StringCchLength(wszOKMtxName, cMutexLength, &length));
        CKHR(StringCchCat(wszOKMtxName, cMutexLength, L"o"));

        CK_PTR(_ultow_s(pid, wszCancelMtxName, 10) == 0);
        CKHR(StringCchLength(wszCancelMtxName, cMutexLength, &length));
        CKHR(StringCchCat(wszCancelMtxName, cMutexLength, L"c"));

        m_hMutexDialog = CreateMutex(NULL, TRUE, wszDlgMtxName); // This mutex ensures only 1 thread pops the dialog
       
        CK_PTR(m_hMutexDialog);
       
        if  (ERROR_ALREADY_EXISTS == GetLastError())
        {
            WaitForSingleObject(m_hMutexDialog, INFINITE);  // Wait until user clicked ok or cancel on dialog
            ReleaseMutex(m_hMutexDialog);

            // At this point, user has selected the action.  
            HANDLE hMutexes[2];
            
            // These mutex objects have been created by first thread already. 
            m_hMutexOK = CreateMutex(NULL, FALSE, wszOKMtxName);
            CK_PTR(m_hMutexOK);
            m_hMutexCancel = CreateMutex(NULL, FALSE, wszCancelMtxName);
            CK_PTR(m_hMutexCancel);
                                        
            ASSERT(ERROR_ALREADY_EXISTS == GetLastError());

            hMutexes[0] = m_hMutexOK;
            hMutexes[1] = m_hMutexCancel;

            // Wait until user clicks ok OR cancel  
            DWORD dwRet = WaitForMultipleObjects(2, 
                                                hMutexes,
                                                FALSE,  //either ok or cancel
                                                INFINITE
                                                );
           
            
            if (WAIT_OBJECT_0 == dwRet)
            {
                dwError = ERROR_SUCCESS;
                ReleaseMutex(hMutexes[0]);
                
            }
            else
            {
                dwError = ERROR_CANCELLED;
                ReleaseMutex(hMutexes[1]);
                CKHR(E_FAIL);
               
            }
             
        }   
        else 
        {
            m_hMutexCancel = CreateMutex(NULL, TRUE, wszCancelMtxName); 
            m_hMutexOK = CreateMutex(NULL, TRUE, wszOKMtxName);

            // Only first thread displays the mixed content dialog
            dwError = InternetErrorDlg(hwnd, 
                                    NULL,      // no request
                                    ERROR_INTERNET_MIXED_SECURITY,
                                    0,         // no flags
                                    NULL       // no extra data
                                    );
            
            ReleaseMutex(m_hMutexDialog); 
            
            if (dwError == ERROR_SUCCESS)
            {
                // User clicked OK to accept mixed content
                // signal other waiting threads OK was pressed          
                ReleaseMutex(m_hMutexOK); 
            }
            else
            {
                // User said No to display mixed content
                ReleaseMutex(m_hMutexCancel);
                CKHR(E_FAIL);
            }
           
        } //if  (ERROR_ALREADY_EXISTS == GetLastError())
        
     }
     else if (dwPolicy == URLPOLICY_ALLOW)
     {
        // Navigate to the app link without showing dialog if setting allows     
        hr = S_OK;
     }
     else 
     {
        // Stop navigating to the link if policy do not allow
        CKHR(E_FAIL);
     } //if (dwPolicy == URLPOLICY_QUERY)
  
           

Cleanup:
     
     ReleaseInterface(pSecMgr);
     ReleaseInterface(pOleWindow);
     ReleaseInterface(pOleCmdTarget);
     SysFreeString(bstrTopUri);
     return hr;
     
}


/******************************************************************************
  COleDocument::GetMoniker()
******************************************************************************/
STDMETHODIMP COleDocument::GetMoniker(__in_opt LPCWSTR pwzLocation, DWORD dwAssign, __deref_out_opt IMoniker **ppimkLocation)
{
    if (ppimkLocation)
    {
        *ppimkLocation = NULL;
    }
    return E_NOTIMPL;
}

/******************************************************************************
  COleDocument::GetFriendlyName()
******************************************************************************/
STDMETHODIMP COleDocument::GetFriendlyName(__in_opt LPCWSTR pwzLocation, __deref_out_opt LPWSTR *ppwzFriendlyName)
{
    if(ppwzFriendlyName)
    {
        *ppwzFriendlyName = NULL;
    }
    return E_NOTIMPL;
}


//******************************************************************************
//
//  IBrowserCallbackServices Implementation
//
//******************************************************************************

HRESULT COleDocument::OnBeforeShowNavigationWindow()
{
    // It's good to keep the progress page shown just until the hosted application starts rendering,
    // to prevent a period of blankness on cold start. But there's also the possibility that we may need to
    // show the deployment canceled/failed page, in the default AppDomain. In either case, a RootBrowserWindow
    // is created. It sends us this notification just before it becomes visible. Thus all scenarios are covered.
    if (GetProgressPage())
    {
        GetProgressPage()->Hide();
    }
    m_pWebBrowser->SetStatusText(L""); // Clear last progress message.
    return S_OK;
}

/******************************************************************************
  COleDocument::PostReadyStateChange()
******************************************************************************/
HRESULT COleDocument::PostReadyStateChange(int rs)
{
    EventWriteWpfHostUm_ReadyStateChanged(rs);

    HRESULT hr = S_OK;
    
    IPropertyNotifySink *pIPNS  = NULL;
    IConnectionPoint    *pCP    = NULL;
    IEnumConnections    *pEnum  = NULL;

    // set our readystate
    m_ReadyState = rs;
    
    // then tell everyone who's interested
    CKHR(m_pCPC->FindConnectionPoint(IID_IPropertyNotifySink, (IConnectionPoint**)&pCP));
    CKHR(pCP->EnumConnections(&pEnum));

    if (!pEnum)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    struct tagCONNECTDATA   conAry[10];
    ULONG  uFetched;

    CKHR(pEnum->Next(10, conAry, &uFetched));

    for (int i = 0; i < (int)uFetched; i++)
    {
        hr = conAry[i].pUnk->QueryInterface(IID_IPropertyNotifySink, (void**)&pIPNS);
        if (SUCCEEDED(hr))
        {
            pIPNS->OnChanged(DISPID_READYSTATE);
            pIPNS->Release();            
        }
        conAry[i].pUnk->Release();
    }
    

Cleanup:
    if (pEnum) pEnum->Release();
    if (pCP) pCP->Release();
    
    return hr;
}

HRESULT COleDocument::GetWebBrowserForCurrentThread(IHostBrowser **ppWebBrowser)
{
    if(!ppWebBrowser)
        return E_POINTER;
    *ppWebBrowser = 0;
    if(!m_pWebBrowser)
        return S_FALSE;
    if(GetCurrentThreadId() == g_mainThreadId)
    {
        *ppWebBrowser = m_pWebBrowser;
        m_pWebBrowser->AddRef();
        return S_OK;
    }
    else
    {
        ASSERT(m_pWebBrowserMarshalStream);
        LARGE_INTEGER zero = {0};
        m_pWebBrowserMarshalStream->Seek(zero, STREAM_SEEK_SET, 0);
        return CoUnmarshalInterface(m_pWebBrowserMarshalStream, IID_IHostBrowser, (void**)ppWebBrowser);
    }
}

/******************************************************************************
  COleDocument::DelegateNavigation()

  This can be called on any thread.
******************************************************************************/
HRESULT COleDocument::DelegateNavigation(__in BSTR bstrUrl, __in_opt BSTR bstrTargetName, __in_opt BSTR bstrHeaders)
{
    HRESULT hr      = S_OK;
    IHostBrowser *pWebBrowser = 0;
    
    // SysStringLen will return 0 if bstrUrl is NULL
    if (SysStringLen(bstrUrl) == 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    CKHR(GetWebBrowserForCurrentThread(&pWebBrowser));
    // Allow the browser to activate a new window opened with a _blank target.
    CoAllowSetForegroundWindow(pWebBrowser, 0); 
    hr = pWebBrowser->Navigate(bstrUrl, bstrTargetName, bstrHeaders);

Cleanup:
    ReleaseInterface(pWebBrowser);
    return hr;
}

/******************************************************************************
  COleDocument::UpdateAddressBar()
******************************************************************************/
/*bool*/ long COleDocument::UpdateAddressBar(__in BSTR pUrl)
{
    HRESULT hr = S_OK;

    if (m_pWebBrowserEventsService == NULL)
        return (long)FALSE;

    if (m_pTravelLogClient->_bstrURL)
    {
        SysFreeString(m_pTravelLogClient->_bstrURL);
    }

    // SysAllocString will return NULL if pUrl is NULL
    m_pTravelLogClient->_bstrURL = SysAllocString(pUrl);
    CK_ALLOC(m_pTravelLogClient->_bstrURL);

    // Notify container to fire the events for us. This will update the address bar
    // Call this only for top level navigations
    CKHR(m_pWebBrowserEventsService->FireNavigateComplete2Event());
    CKHR(m_pWebBrowserEventsService->FireDocumentCompleteEvent());  

Cleanup:
    return (SUCCEEDED(hr)) ? ((long)TRUE) : ((long)FALSE);
}

/******************************************************************************
  COleDocument::UpdateBackForwardState()
******************************************************************************/
HRESULT COleDocument::UpdateBackForwardState()
{
    // Nothing to do if we are downlevel 
    if (m_pWebBrowserEventsService == NULL)
        return S_OK;

    return CTExec(m_spOleClientSite, NULL, OLECMDID_UPDATEBACKFORWARDSTATE, MSOCMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
}

/******************************************************************************
  COleDocument::UpdateTravelLog()
******************************************************************************/
HRESULT COleDocument::UpdateTravelLog(BOOL addNewEntry)
{
    HRESULT     hr = S_OK;
    VARIANTARG  var;

    if (m_pWebBrowserEventsService == NULL)
        return E_NOTIMPL;

    VariantInit(&var);
    var.vt = VT_I4;
    var.lVal = 0;

    hr = CTExec(m_spOleClientSite, NULL, addNewEntry ? OLECMDID_ADDTRAVELENTRY : OLECMDID_UPDATETRAVELENTRY, 0, &var, NULL);
    return hr;
}

/*******************************************************************************
 COleDocument::UpdateProgress()
*******************************************************************************/
long /*bool*/ COleDocument::UpdateProgress(__int64 cBytesCompleted, __int64 cBytesTotal)
{
    VARIANTARG var;
    HRESULT hr;
    
    // First determine if the max value has changed, and if so, set it. 
    if (cBytesTotal != m_progMaxBytes)
    {
        var.vt = VT_I4;
        var.lVal = (__int32)cBytesTotal;
        CKHR(CTExec(m_spOleClientSite, 
                    NULL, 
                    OLECMDID_SETPROGRESSMAX, 
                    MSOCMDEXECOPT_DONTPROMPTUSER, 
                    &var, 
                    NULL));

        // It's safe to change m_progMaxBytes after CTExec because if CTExec fails,
        // then the progress bar wasn't updated.
        m_progMaxBytes = cBytesTotal;
    }

    // Upon success above, set the current progress value.
    var.vt = VT_I4;
    var.lVal = (__int32)cBytesCompleted;
    CKHR(CTExec(m_spOleClientSite, 
                NULL, 
                OLECMDID_SETPROGRESSPOS, 
                MSOCMDEXECOPT_DONTPROMPTUSER, 
                &var, 
                NULL));

Cleanup:
    return SUCCEEDED(hr) ? (long)true : (long)false;
}

/*******************************************************************************
 COleDocument::ChangeDownloadState()
*******************************************************************************/
long /* bool */ COleDocument::ChangeDownloadState(VARIANT_BOOL fIsDownloading)
{
    VARIANTARG var;
    HRESULT hr;

    // Simply call our CTExec helper to change the status for us.
    var.vt = VT_BOOL;
    var.lVal = fIsDownloading;
    CKHR(CTExec(m_spOleClientSite, 
                NULL, 
                OLECMDID_SETDOWNLOADSTATE, 
                MSOCMDEXECOPT_DONTPROMPTUSER, 
                &var, 
                NULL));

Cleanup:
    return SUCCEEDED(hr) ? (long)true : (long)false;
}

/*******************************************************************************
 COleDocument::GetOleInPlaceFrameHwnd()
*******************************************************************************/
long /*bool*/ COleDocument::GetOleInPlaceFrameHwnd(__out_ecount(1) HWND *phwnd)
{
    bool returnVal = false;
    
    // m_pOleDocView->m_FrameInfo is a struct, so it's never null
    if (m_pOleDocView->m_pInPlaceSite &&
        (m_pOleDocView->m_FrameInfo.hwndFrame))
    {      
        *phwnd = m_pOleDocView->m_FrameInfo.hwndFrame;
        returnVal = true;
    }
    return (long)returnVal;
}

/*******************************************************************************
 COleDocument::IsDownlevelPlatform()
*******************************************************************************/
long /*bool*/ COleDocument::IsDownlevelPlatform()
{
    // m_pWebBrowserEventsService was implemented when support for 
    // docobjs to play into the browser's travellog was added. Hence we 
    // use that to make decisions for features we need like : 
    // updating addr bar, adding travel entries etc.
    return m_pWebBrowserEventsService == NULL;
}

/*******************************************************************************
 COleDocument::IsShuttingDown()

 Check whether the browser is shutting this process down. This could happen
 because the browser is navigating away from us, or is terminating.
*******************************************************************************/
long /*bool*/ COleDocument::IsShuttingDown()
{
    return (long) (m_bDeactivating || m_bPostedShutdown);
}

/*******************************************************************************
 COleDocument::TabOut()

 Called to tab out of the DocObject's window into the next/previous focusable element in the browser.

 When in IE (v7+), *tabbing into* the DocObject is handled by the IInputObject implementation in 
 COleInPlaceActiveObject and by COleObject::DoVerb(). When hosted by the Mozilla plug-in, we use a custom
 command - PHCMDID_TABINTO.
*******************************************************************************/
VARIANT_BOOL COleDocument::TabOut(VARIANT_BOOL forward)
{
    // We first try IHostBrowser::TabOut(). This is initially implemented only by the IE in-proc handler,
    // and it works only for the case when we're hosted inside an HTML frame. (The code there has to be run in
    // the browser process to work.)
    // For all other cases (top-level in IE, hosted by the Mozilla plug-in), we rely on posting VK_TAB to the
    // IOleInPlaceFrame's window. Since this is needed for both browsers, we keep the code here instead of
    // duplicating it in the IHostBrowser::TabOut() implementations.

    CComQIPtr<IHostBrowser2> spHostBrowser(m_pWebBrowser);
    if(spHostBrowser)
    {
        HRESULT res = spHostBrowser->TabOut(forward != VARIANT_FALSE);
        if(res == S_OK)
            return VARIANT_TRUE;
    }

    if (m_pOleDocView->GetInPlaceFrame())
    {
        // There is no API way to move focus to the browser frame; that's why simulating key input.
        // The 'forward' argument is ignored here, but the browser will still be able to get the 
        // current Shift state (for direction).
        MSG msg;
        ZeroMemory(&msg, sizeof msg);
        m_pOleDocView->GetInPlaceFrame()->GetWindow(&msg.hwnd);
        msg.message = WM_KEYDOWN;
        msg.wParam = VK_TAB; 
        HRESULT res = m_pOleDocView->GetInPlaceFrame()->TranslateAccelerator(&msg, 0);
        if(res == S_OK)
            return VARIANT_TRUE;
    }
    return VARIANT_FALSE;
}

bool COleDocument::HasDownloadCompleted() const
{
    ASSERT(m_hDownloadCompletedEvent);
    if (m_hDownloadCompletedEvent)
    {
        DWORD result;
        HANDLE events[] = { m_hDownloadCompletedEvent };
        result = WaitForMultipleObjects(
            1,          // Number of handles in array
            events,     // Handle array
            false,      // FALSE to only check the handles in the array (not extras)
            0);         // Return immediately, do not wait for signaling
        // Check if all events are signaled
        if (result == WAIT_OBJECT_0)
        {
            return true;
        }
    }
    // Events are not setup correctly, do nothing.
    return false;
}

HRESULT COleDocument::ProcessUnhandledException(__in BSTR pErrorMsg)
{
    CErrorPage::Display(pErrorMsg);
    return S_OK;
}

// Get the IOleClientSite. This always returns S_OK unless ppOleClientSite was NULL.

STDMETHODIMP COleDocument::GetOleClientSite(__out_ecount(1) IUnknown** ppOleClientSite)
{
    return m_spOleClientSite.CopyTo(reinterpret_cast<IOleClientSite**>(ppOleClientSite));
}

STDMETHODIMP COleDocument::UpdateCommands()
{
    // Informs the browser of state changes. The browser 
    // can then query the status of the commands whenever convenient. 
    return CTExec(m_spOleClientSite,
                NULL,
                OLECMDID_UPDATECOMMANDS,
                0,
                NULL, 
                NULL);
}

STDMETHODIMP COleDocument::CreateWebBrowserControlInBrowserProcess(__deref_out IWebBrowser2 **ppWebBrowser)
{
    // It's unfortunate, but we have to block this case. The WebOC thinks it's the top-level browser. 
    // Thus, cookies set from its top frame will never have the 3rd party flag. Our cookie shim forces this
    // flag, but not when the WebOC is hosted in the browser process (because it's applied only in 
    // PresentationHost). 
    if(CCookieShim::IsApplicationThirdParty())
        return E_ACCESSDENIED;

    return m_pWebBrowser->CreateWebBrowserControl(ppWebBrowser);
}


//******************************************************************************
//
// Managed Interop Helpers
//
//******************************************************************************

/******************************************************************************
  COleDocument::InitApplicationServer()
******************************************************************************/
HRESULT COleDocument::InitApplicationServer()
{
    HRESULT hr = S_OK;

    // 

    CKHR((ApplicationHelper())->SetBrowserCallback((IBrowserCallbackServices*)this));

    CKHR(EnsureTopWindow());    
    // 
    CKHR(ApplicationHelper()->SetParent((INT_PTR)m_hwndTop));

Cleanup:
    return hr;
}

/******************************************************************************
  COleDocument::EnsureTopWindow()

  The host container asks for our docserver's window during activation, 
  specifically when we call the container's IOleInPlaceUIWindow->SetActiveObject.
  This function creates and returns the window for this call. This was initially 
  implemented since we delay created the RootBrowserWindow object since we waited
  to see if the startup page has a window tag or not. But we pre-create one now 
  since regardless of whether the startup markup page contained 
  a window tag or not, we need to repaint the container window.
  
  For hosting xaml etc, the Application and RootBrowserWindow(RBW) objects should
  already have been created by then so we could potentially return the RBW's handle,
  but for the deploy apps case we have an initial app to show the progress UI
  and a second one which is the main app. Instead of updating the window that the
  container holds on to(if it is even possible), we will create and keep 
  this intermediate window.
******************************************************************************/
HRESULT COleDocument::EnsureTopWindow()
{
    if (m_hwndTop != NULL)
        return S_OK;

    WNDCLASS wc;

    // if our window class has not been registered, then do so
    if(!GetClassInfo(g_hInstance, TOPWINDOW_CLASS_NAME, &wc))
    {
        ZeroMemory(&wc, sizeof(wc));
        wc.style          = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc    = (WNDPROC)WndProc;
        wc.cbClsExtra     = 0;
        wc.cbWndExtra     = 0;
        wc.hInstance      = g_hInstance;
        wc.hIcon          = NULL;
        wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszMenuName   = NULL;
        wc.lpszClassName  = TOPWINDOW_CLASS_NAME;
           
        if (!RegisterClass(&wc))
        {
            return E_FAIL;
        }
    }

    if (m_hwndTop == NULL)
    {
        m_hwndTop = CreateWindowEx(NULL,
                                TOPWINDOW_CLASS_NAME,
                                NULL,
                                WS_OVERLAPPED | WS_CLIPSIBLINGS | WS_DISABLED,
                                0,
                                0,
                                0,
                                0,
                                NULL,
                                NULL,
                                g_hInstance,
                                this);
    }

    return (m_hwndTop == NULL ? E_FAIL : S_OK);
}

HRESULT COleDocument::SetHistoryStream(__in_ecount_opt(1) LPSTREAM pStream)
{
    HRESULT hr = S_OK;
    if (pStream)
    {
        m_pHistoryStream = pStream;
        CKHR(m_pHistoryStream->AddRef());
    }

Cleanup:
    return hr;
}

void COleDocument::ReleaseHistoryStream()
{
    // Releases if non-null and clears the member variable
    ReleaseInterface(m_pHistoryStream);
}

void COleDocument::RunServerAsync()
{
    ASSERT(m_hwndTop); // should be UI-Activated

    // If we decided to show the progress page early (on cold start, or maybe one day if we can test in native
    // code whether an application is (not) cached), give priority to HTMLDocument's internal callbacks.
    // Otherwise, if we plunge into starting the CLR and loading managed code, it would be a long while before
    // the user sees the progress page. Normally, the progress page sends a notification when it's rendered 
    // (via CProgressPageHost). However, this notification is derived from a somewhat crude heuristic--getting
    // the first WM_PAINT message, which may fail. To prevent stalling, which actually tends to happen when
    // debugging, a fallback timer is used that will make the activation continue regardless. The timer delay
    // is long enough for the HTML page to render and long enough to be noticed in profiling if something
    // breaks in the future, but still not too long to be crippling if we don't realize the scheme is broken.
    CProgressPage *pProgressPage = m_progressPageHost.GetProgressPage();
    if(pProgressPage && pProgressPage->IsActive())
    {
        m_startupCallbackTimerId = SetTimer(m_hwndTop, CALLBACK_TIMER_ID, 600, 0);
    }
    else
    {
        PostMessage(NULL, WM_NOTIFY, AVALON_STARTUP_CHECK, NULL);
        // DllMain() will get this message and call OnStartupCallback().
        //[See note on possible refactoring there.]
    }
}

void COleDocument::OnStartupCallback()
{
    if(!m_bAvalonRunning // Call RunServerHelper() only once.
        && m_spOleClientSite) // Make sure we are initialized and not shutting down prematurely.
    {
        if(m_bViewCreated // UI-Activated
            && HasDownloadCompleted()) // possible async downloading in XPSViewer
        {
            RunServerHelper(); //(triggers Watson on error)
        }
    }
}

// Gets the host browser's process integrity level, -1 on error (or before Windows Vista).
int COleDocument::GetBrowserProcessIntegrityLevel() const
{
    ASSERT(m_hwndTop);
    HWND hBrowserWindow = GetParent(m_hwndTop);
    ASSERT(hBrowserWindow);
    DWORD broserProcessId = 0;
    GetWindowThreadProcessId(hBrowserWindow, &broserProcessId);
    return broserProcessId ? GetProcessIntegrityLevel(broserProcessId) : -1;
}

/******************************************************************************
  RunServerHelper()
  Invokes Watson and terminates on failure.
******************************************************************************/
void COleDocument::RunServerHelper()
{
    HRESULT     hr  = S_OK;

    // Set m_bAvalonRunning to notify DllMain not to call CanStartAvalon again
    m_bAvalonRunning = true;
    RECT r;

    BSTR errMsg = 0;
    if(m_errorMessageToDisplay.Length())
    {
        errMsg = m_errorMessageToDisplay;
    }
    else if (IsMimeTypeDisabled((LPOLESTR)GetStartupUri(), this))
    {
        errMsg = LoadResourceString(IDS_APP_TYPE_DISABLED);
    }
    if(errMsg)
    {
        CErrorPage::Display(errMsg);
        return;
    }

    if (GetHostingFlag(hfHostedInFrame))
    {
        CKHR(ValidateSecurityInFrame());
    }

    CKHR(AvalonExecute());

    // During UI-Activation we don't have the managed side of things initialized anymore.
    // This hacklet propagates the window size and visiblity state to DocObjHost once it becomes available.
    m_pOleDocView->GetRect(&r);
    CKHR(m_pOleDocView->SetRect(&r));
    CKHR(m_pOleDocView->Show(true));

    /*
    Determine whether the host browser's process is lower integrity level than ours.
    This is the case of IE 7+ when in 'protected mode' (aka LoRIE). 
    
    It looks simpler to just call ieframe!IEIsProtectedModeProcess() in the browser process (via IHostBrowser),
    but that would create a security flaw: If the browser process is compromised, our in-proc handler could
    be fooled that the process is not low-integrity.
    */
    int browserIL = GetBrowserProcessIntegrityLevel();
    SetHostingFlag(hfIsBrowserLowIntegrityProcess, 
        browserIL != -1 && browserIL < GetProcessIntegrityLevel(GetCurrentProcessId()));

    ASSERT(m_validHostingFlags == hf_firstUnused-1); // all flags assigned

    UINT exitCode = ApplicationHelper()->Run(
        (LPWSTR) GetStartupUri(), 
        (LPWSTR) GetStartupLocation(),
        GetCurrentMime(), 
        (LPWSTR) GetDebugSecurityZoneURL(), 
        (LPWSTR) GetApplicationIdentity(),
        GetDocumentStream(), 
        GetHistoryStream(), 
        m_hostingFlags,
        GetProgressPage(),
        m_bstrProgressAssemblyName,
        m_bstrProgressClassName,
        m_bstrErrorAssemblyName,
        m_bstrErrorClassName,
        m_pWebBrowser);
    CKHR((RETURN_CODE_TO_HR(exitCode)));

Cleanup:
    HANDLE_ACTIVATION_FAULT(hr);

    ReleaseHistoryStream();
}

/**************************************************************************
   IsAppLoaded() 
**************************************************************************/
bool COleDocument::IsAppLoaded()
{
    if (ApplicationHelper() != NULL)
    {
        return ((ApplicationHelper())->IsAppLoaded() == 0 ? false : true);
    }
    return false;
}

//******************************************************************************
//
//      Shdocvw communication methods
//
//******************************************************************************

HRESULT COleDocument::QueryBrowserService(REFGUID guidService, REFIID iid, __deref_out_ecount(1) void **pResult)
{
    HRESULT hr = S_OK;
    if(!m_spBrowserServiceProvider)
    {
        CKHR(E_UNEXPECTED);
    }
    CKHR(m_spBrowserServiceProvider->QueryService(guidService, iid, pResult));
Cleanup:
    return hr;
}

STDMETHODIMP COleDocument::InitHostSupport()
{
    // For robustness, we will ignore any bad HRESULTs from the host, and only do something if it worked.

    HRESULT hr = S_OK;
    IEnumRAWINPUTDEVICE* pEnum = NULL;

    // Query for SID_SWpfHostSupport
    hr = QueryBrowserService(SID_SWpfHostSupport, IID_IWpfHostSupport, (void**)&m_pWpfHostSupport);
    if (SUCCEEDED(hr))
    {
        HRESULT hrInput = m_pWpfHostSupport->GetRawInputDevices(&pEnum);

        if (hrInput == S_OK)
        {
            HRESULT hrEnum = S_OK;

            while (hrEnum == S_OK)
            {
                RAWINPUTDEVICE hid;
                ULONG fetched = 0;
                hrEnum = pEnum->Next(1, &hid, &fetched);
                if (hrEnum == S_OK)
                {
                    // Ignore return value or any errors returned by this
                    (void) RegisterRawInputDevices(&hid, 1, sizeof(RAWINPUTDEVICE));
                }
            }
        }
        // Ignore return value or any errors returned by this
        (void) m_pWpfHostSupport->GetCustomUI(&m_bstrProgressAssemblyName, &m_bstrProgressClassName, &m_bstrErrorAssemblyName, &m_bstrErrorClassName);
    }
    else
    {
        hr = S_OK; // the host doesn't want to talk to us; that's OK.
    }

    SAFERELEASE_POINTER(pEnum);

    return hr;
}

/**************************************************************************
   COleDocument::InitDocHost() -- private
   Invokes Watson and terminates on failure.
**************************************************************************/
// Compile without optimizations to get more reliable IP address logging by the CHKR macros.
// We've seen quite a few Watson failures likely due to this or that interface broken for marshaling.
#pragma optimize("", off)
void COleDocument::InitDocHost()
{
    EventWriteWpfHostUm_InitDocHostStart();

    HRESULT hr = S_OK;
 
    CComPtr<IWebBrowser2> spWB2;
    boolean isTopLevel;
    CComBSTR topLevelURL;

    // If IWebBrowser2 is unavailable, assume we are being activated as an ActiveX control.
    // Initially, this means activated by the Mozilla plug-in.
    // If we get IWebBrowser2, it's either IE or a standalone WebOC in an application ... or the WebOC we are
    // hosting in the IE process for the managed WebBrowser control.
    bool bHostedInIEorWebOC = QueryBrowserService(SID_SWebBrowserApp, IID_IWebBrowser2, (void**)&spWB2) == S_OK;
    bool bHostedInWebOCInIE = false;
    if(bHostedInIEorWebOC)
    {
        CComPtr<IWebBrowser2> spTopLevelBrowser;
        CKHR(GetTopLevelBrowser(m_spOleClientSite, &spTopLevelBrowser));

        // Resolve hfHostedInIE & hfHostedInWebOC.
        //
        // There seems to be no direct, clean way to distinguish the WebOC from real IE. This is a heuristic:
        // IWebBrowser2's get_Parent() returns the same object only for IE's top-level browser. The WebOC 
        // tries to get its container, so it will return either null or some other object.
        CComPtr<IDispatch> spParent;
        spTopLevelBrowser->get_Parent(&spParent);
        if(spTopLevelBrowser.IsEqualObject(spParent))
        {
            SetHostingFlag(hfHostedInIE);
        }
        else
        {
            SetHostingFlag(hfHostedInWebOC);
            /*
            But that's not the whole story. The WebOC may be hosted in IE, "cross-cross-process".
            Then we set hfHostedInIE too. Being aware of this case is important for security.
              - We shouldn't assume top-level status (though the WebOC thinks it's the top-level browser).
              - For cookie access, we'll conservatively assume 3rd-party status. (This happens by virtue of
                IHostBrowser::DetermineTopLevel() returning null top-level URL and passing it to CCookieShim.)
              - We'll block creating a WebOC for the nested DocObject (new restriction for v4).

            In principle, the outer DocObject should site-lock the WebOC to the application's site-of-origin,
            and because we don't allow hosting the WebOC in the IE process when in 3rd-party status (blocked 
            by CreateWebBrowserControlInBrowserProcess()), we know the WebOC's top-frame content is 1st-party 
            relative to the browser's real top-level URL. But because the site locking feature is known not 
            to be infallible and because we don't really need to support such deeply nested compositions, we 
            just block double-nested WebOC hosting (even though technically we have the means to ensure such 
            WebOC ends up in the "top" IE process too--this was supported by v3.5 SP1).
            
            Caveat: If the IE process is compromised, we can't trust the result of the SID_XXWebOCHost query,
              and even detecting that we are in IE might be thrown off. Partial mitigation is that the 
              WebBrowser control also uses hfIsBrowserLowIntegrityProcess in its decision to host the WebOC in 
              the browser process. Since the Low IL detection is done externally, it's reliable.
            */
            CComPtr<IUnknown> spUnk;
            if(QueryService(spTopLevelBrowser, SID_XXWebOCHost, IID_IUnknown, (void**)&spUnk) == S_OK)
            {
                SetHostingFlag(hfHostedInIE);
                bHostedInWebOCInIE = true;
            }
        }

        // The IE in-proc handler (CBrowserInprocHandler) makes its implementation of IHostBrowser available
        // by calling IObjectWithSite::SetSite().
        CComQIPtr<IObjectWithSite> spObjWithSite(m_pOuterObject);
        if(!spObjWithSite)
            CKHR(E_UNEXPECTED);
        CKHR(spObjWithSite->GetSite(IID_IHostBrowser, (void**)&m_pWebBrowser));

        CKHR(CComObject<CWebBrowserEventSink>::CreateInstance(&m_pWebBrowserEventSink));
        m_pWebBrowserEventSink->AddRef();
        CKHR(m_pWebBrowserEventSink->Attach(this, spWB2));

        // Fine to ignore failure to get IShellBrowser, which is only used to provide status bar interaction
        // during menu selection operations. This old feature has been broken due to IE security tightening
        // measures and has caused us Watson failures in the past. See Dev10 696978: it seems the interface
        // registration is broken on some machines (HKCR\Interface\{000214E2-0000-0000-C000-000000000046}).
        CComQIPtr<IShellBrowser> spShellBrowser(spTopLevelBrowser);
        if (spShellBrowser)
        {
            // Ignore this HRESULT because it will fail for the WebOC case, which is OK.
            spShellBrowser->GetControlWindow(FCW_STATUS, &m_hwndStatusBar);
        }
    }
    else
    {
        CKHR(QueryBrowserService(IID_IHostBrowser, IID_IHostBrowser, (void**)&m_pWebBrowser));
        m_hostingFlags |= hfHostedInMozilla;
    }
    m_validHostingFlags |= hfHostedInIEorWebOC | hfHostedInMozilla;

    // COleDocument is single-threaded, but m_pWebBrowser may be needed from another thread.
    hr = CoMarshalInterThreadInterfaceInStream(IID_IHostBrowser, m_pWebBrowser, &m_pWebBrowserMarshalStream);
    ASSERT(hr == S_OK);

    CKHR(m_pWebBrowser->DetermineTopLevel(&isTopLevel, &topLevelURL));
    // Because the WebOC thinks it's a top-level browser, we want bHostedInWebOCInIE to take priority.
    // (Generally, hfHostedInFrame=true is a more restrictive context, so it's safer to assume it in this
    //  kind of ambiguous situation.) The implementation of DetermineTopLevel() should ensure this, but we 
    // enfore it here to be sure.
    ASSERT(!(bHostedInWebOCInIE && isTopLevel));
    isTopLevel = isTopLevel && !bHostedInWebOCInIE;
    SetHostingFlag(hfHostedInFrame, !isTopLevel);

    CKHR(CCookieShim::Init(isTopLevel != false, topLevelURL));

    // Ignoring HRESULT here since we want to work downlevel by not invoking these features from managed code.
    QueryBrowserService(SID_SWebBrowserEventsService, IID_IWebBrowserEventsService, (void**)&m_pWebBrowserEventsService);

    InitHostSupport();

    EventWriteWpfHostUm_InitDocHostEnd();

Cleanup:
    HANDLE_ACTIVATION_FAULT(hr);
}
#pragma optimize("", on) // restore settings

/******************************************************************************
  COleDocument::UninitDocHost()
******************************************************************************/
STDMETHODIMP COleDocument::UninitDocHost()
{     
    CCookieShim::Uninit();

    if (m_pWebBrowser)
    {
        m_pWebBrowser->Release();
        m_pWebBrowser = NULL;        
    }

    if (m_pWebBrowserEventsService)
    {
        m_pWebBrowserEventsService->Release();
        m_pWebBrowserEventsService = NULL;
    }

    if (m_pWebBrowserEventSink)
    {
        m_pWebBrowserEventSink->Detach();
        ReleaseInterface(m_pWebBrowserEventSink);
    }

    return S_OK;

}

/******************************************************************************
  COleDocument::PostShutdown()
******************************************************************************/
void COleDocument::PostShutdown()
{
    if (GetProgressPage())
    {
        GetProgressPage()->Hide(); // just for clean, controlled shutdown
    }

    m_bDeactivating = true;
    if(!m_bPostedShutdown)
    {
        if(ApplicationHelper())
        {
            ApplicationHelper()->PostShutdown();
        }
        else if(CErrorPage::IsActive())
        {
            // In case the host browser/WebOC did not send OLECMDID_ONUNLOAD, we fake it here to ensure
            // the process exits.
            CErrorPage::ExecOleCommand(0, OLECMDID_ONUNLOAD, 0, 0, 0);
        }
        m_bPostedShutdown = true;
    }
}

//******************************************************************************
//
// COleDocument Private Helper Functions
//   
//******************************************************************************

/******************************************************************************
 COleDocument::GetWindow()
******************************************************************************/
STDMETHODIMP COleDocument::GetWindow(__deref_out_ecount(1) HWND* phwnd)
{
    HRESULT hr = S_OK;

    CK_PARG(phwnd);
    (*phwnd) = NULL;

    CKHR(EnsureTopWindow());
    (*phwnd) = m_hwndTop;

Cleanup:

    return hr;
}

/******************************************************************************
  COleDocument::UIDeactivate()
******************************************************************************/
STDMETHODIMP COleDocument::DeactivateUI()
{
    return m_pOleDocView->DeactivateUI();
}

/******************************************************************************
  COleDocumentView::DeactivateInPlace()
******************************************************************************/
STDMETHODIMP COleDocument::DeactivateInPlace()
{
    return m_pOleDocView->DeactivateInPlace();
}

/******************************************************************************
  COleDocument::GetClassID()
  If this method is not implemented correctly, one of the implications is that
  an additional instance of the server (PresentationHost.exe etc.) will be
  created when the browser navigates away from this docobj. This is due to the
  mechanism controlled by the BROWSERFLAG_DONTCACHESERVER flag in shdocvw.
******************************************************************************/
STDMETHODIMP COleDocument::GetClassID(__out_ecount(1) LPCLSID pClassID)
{
    HRESULT hr = S_OK;
    CK_PARG(pClassID);

    if (GetCurrentMime() == MimeType_Application)
    {
        *pClassID = CLSID_DocObjXapp;
    }
    else if (GetCurrentMime() == MimeType_Markup)
    {
        *pClassID = CLSID_DocObjXaml;
    }
    else if (GetCurrentMime() == MimeType_Xps)
    {
        *pClassID = CLSID_DocObjPackage;
    }
    else
    {
        // We must not get to here or else IE will not be nice to us. (See comment above.)
        ASSERT(FALSE);
    }

    return S_OK;

Cleanup:
    return hr;
}

/******************************************************************************
  COleDocument::OnMenuSelect()
******************************************************************************/
STDMETHODIMP COleDocument::OnMenuSelect(HMENU selected)
{    
    // If we don't have a status bar, it does not make sense to do any of this.
    // If we are being hosted in the WebOC, we won't.
    if (m_hwndStatusBar)
    {
        // The browser automatically sets the status bar to simple mode
        // when a menu is opened.  However, when we switch from one of our menus
        // to another we cannot tell if we are leaving the menu entirely or moving
        // do a different menu.  In the case where we are transitioning from one
        // to another we need to put the status bar back into simple mode.
        if (m_bSimpleStatusBar == FALSE)
        {
            SendMessage(m_hwndStatusBar, SB_SIMPLE, 1, 0L);
            m_bSimpleStatusBar = TRUE;
        }

        // From MSDN:
        // To avoid potential problems, all objects being activated in place should 
        // process the WM_MENUSELECT message and call IOleInPlaceFrame::SetStatusText 
        // even if the object does not usually provide status information (in which 
        // case the object can just pass a NULL string for the requested status text).
        //
        // We pass "" here instead of NULL because when passing NULL the help text from
        // other menu items is not cleared.
        m_pWebBrowser->SetStatusText(L"");
    }
    return S_OK;
}

/******************************************************************************
  COleDocument::OnMenuDeselect()
******************************************************************************/
STDMETHODIMP COleDocument::OnMenuDeselect()
{
    // If we don't have a status bar, it does not make sense to do any of this.
    // If we are being hosted in the WebOC, we won't.
    if (m_hwndStatusBar)
    {
        // When leaving a menu we need to take it out of simple mode beause
        // the browser will not do this for us. 
        if (m_bSimpleStatusBar == TRUE)
        {
            SendMessage(m_hwndStatusBar, SB_SIMPLE, 0, 0L);
            m_bSimpleStatusBar = FALSE;
        }
    }

    return S_OK;
}


/**************************************************************************
//
//   Global Helper Functions
//
**************************************************************************/

/**************************************************************************
  CTExec - helper that calls IOleCommandTarget::Exec
**************************************************************************/
STDMETHODIMP
CTExec(
        __in_ecount_opt(1) IUnknown *pUnk,
        __in_ecount_opt(1) const GUID * pguidCmdGroup,
        DWORD  nCmdID,
        DWORD  nCmdexecopt,
        __in_ecount_opt(1) VARIANTARG * pvarargIn,
        __inout_ecount_opt(1) VARIANTARG * pvarargOut)
{
    IOleCommandTarget * pCommandTarget;
    HRESULT             hr = S_OK;

    CK_PARG(pUnk);

    hr = pUnk->QueryInterface(IID_IOleCommandTarget, (void**) &pCommandTarget);
    if (SUCCEEDED(hr))
    {
        hr = pCommandTarget->Exec(
                pguidCmdGroup,
                nCmdID,
                nCmdexecopt,
                pvarargIn,
                pvarargOut);
        pCommandTarget->Release();
    }

Cleanup:
    return hr;
}

/**************************************************************************
   Static Helpers
**************************************************************************/
/*static*/ LRESULT COleDocument::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    COleDocument *pOleDoc = (COleDocument *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch(msg)
    {
    case WM_NCCREATE:
        {
            Assert(pOleDoc == NULL);
            pOleDoc = (COleDocument *) ((LPCREATESTRUCT) lParam)->lpCreateParams;
            Assert(pOleDoc != NULL);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) pOleDoc);
            // We can set it here or set it from the return value of the CreateWindowEx call which
            // generates this WM_NCCREATE message to our WndProc. Opting the latter (for no specific reason)
            // pOleDoc->m_hwndTop = hwnd;

            // Fall through to DefWindowProc
            break;
        }
    case WM_NCDESTROY:
        {
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) NULL);

            // Fall through to DefWindowProc
            break;
        }
    case WM_INITMENUPOPUP:
        {
            // Don't handle system menu calls. 
            if (HIWORD(lParam) == TRUE)
                break;

            if (pOleDoc)
            {
                return pOleDoc->QueryStatus((HMENU)wParam);
            }
            
            // Fall through to DefWindowProc
            break;
        }
    case WM_MENUSELECT:
        {
            UINT uMenuFlags = GET_WM_MENUSELECT_FLAGS(wParam, lParam);
            HMENU hMenu = GET_WM_MENUSELECT_HMENU(wParam, lParam);
            if (!hMenu && LOWORD(uMenuFlags) == 0xffff) // This indicates menu de-select
            {
                pOleDoc->OnMenuDeselect();
                break;
            }
            pOleDoc->OnMenuSelect(hMenu);
            break;
        }
    case WM_COMMAND:
        {
            // wParam
            // The high-order word specifies the notification code if the message is from a control. 
            // If the message is from an accelerator, this value is 1. 
            // If the message is from a menu, this value is zero. 
            // The low-order word specifies the identifier of the menu item, control, or accelerator. 
            //
            // lParam
            // Handle to the control sending the message if the message is from a control. 
            // Otherwise, this parameter is NULL. 

            if ((GET_WM_COMMAND_CMD(wParam, lParam) == 0) && pOleDoc)
            {
                // User selected Menu Separator, handle it, do nothing
                if (LOWORD(wParam) == 0)
                    return 0;

                int cmdID = GET_WM_COMMAND_ID(wParam, lParam);
                if (cmdID >= IDM_MENU_FIRST && cmdID <= IDM_MENU_LAST)
                {
                    pOleDoc->Exec(cmdID);
                    return 0;
                }
            }

            // Fall through to DefWindowProc
            break;
        }
    case WM_TIMER:
        {
            if (wParam == CALLBACK_TIMER_ID)
            {
                BOOL res = KillTimer(hwnd, CALLBACK_TIMER_ID);
                (void)res;
                pOleDoc->OnStartupCallback();
            }
        }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

HRESULT COleDocument::RestartApplication()
{
    WCHAR fileUri[INTERNET_MAX_URL_LENGTH];

    // TFS Dev10 452011 - Firefox doesn't accept a "raw" file path as a URI, as passed in below
    // using DelegateNavigation. Therefore we make sure such a path is correctly represented as
    // a file:// URI.
    LPCWSTR url = StartupUri()->GetValue();
    if (!PathIsURL(url))
    {
        DWORD cch = INTERNET_MAX_URL_LENGTH;
        if (SUCCEEDED(UrlCreateFromPath(url, fileUri, &cch, 0)))
        {
            url = fileUri;
        }
    }

    // Beware: This function can be called on any thread. But DelegateNavigation() is fine with that.
    return DelegateNavigation(CComBSTR(url));
}


/////////////////////////////////////////////////////////////////////////////////////////
// COleDocument::CProgressPageHost

HRESULT COleDocument::CProgressPageHost::Init(COleDocument *pOleDoc)
{
    HRESULT hr;
    ASSERT(!m_pOleDoc && pOleDoc);
    m_pOleDoc = pOleDoc;
    CKHR(CComObject<CProgressPage>::CreateInstance(&m_pProgressPage));
    m_pProgressPage->AddRef(); // This is an owning reference.
    m_pProgressPage->Init(this);
Cleanup:
    return hr;
}

COleDocument::CProgressPageHost::~CProgressPageHost()
{
    if(m_pProgressPage)
    {
        // If the page was shown, it should have been explicitly closed, and thus CProgressPageHost should
        // have received OnProgressPageClosed call.
        ASSERT(!m_pProgressPage->IsActive()); 

        ReleaseInterface(m_pProgressPage);
    }
}

void COleDocument::CProgressPageHost::OnProgressPageRendered()
{
    // We can start the heavy lifting of loading managed code... We explanation in RunServerAsync().
    m_pOleDoc->OnStartupCallback();
}

void COleDocument::CProgressPageHost::OnProgressPageClosed()
{
    ReleaseInterface(m_pProgressPage);  // This breaks the reference loop.
}

HRESULT COleDocument::CProgressPageHost::ExecOLECommand(UINT cmdId)
{
    return m_pOleDoc->Exec(0, cmdId, 0, 0, 0);
}

