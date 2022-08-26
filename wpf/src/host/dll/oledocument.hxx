//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Defines the OleDocument class of the PresentationHost
//
//  History:
//     2002/06/12-murrayw
//          Created
//     2003/06/30-Microsoft
//          Ported ByteRangeDownloader to WCP
//     2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#pragma once 

#include "HostServices.h"
#include "ProgressPage.hxx"

class COleObject;
struct IWpfHostSupport;
struct IHostBrowser;
struct IWebBrowserEventsService;
class COleInPlaceActiveObject;
class CConnectionPointContainer;
class CWebBrowserEventSink;

#define TOPWINDOW_CLASS_NAME     L"DocObject_Top_Class"


/**************************************************************************

   Global Helper Functions

**************************************************************************/
STDMETHODIMP CTExec(__in_ecount_opt(1) IUnknown *pUnk,
                    __in_ecount_opt(1) const GUID * pguidCmdGroup,
                    DWORD  nCmdID,
                    DWORD  nCmdexecopt,
                    __in_ecount_opt(1) VARIANTARG * pvarargIn,
                    __inout_ecount_opt(1) VARIANTARG * pvarargOut);

/**************************************************************************

   COleDocument class definition

**************************************************************************/

class COleDocument : public IOleDocument, public IDispatch, public IOleCommandTarget, public IBrowserCallbackServices, public IHlinkTarget
{
friend class BootStrapper;
friend class COleDocumentView;
friend class COleInPlaceObject;
friend class COleInPlaceActiveObject;
friend class CTravelLogClient;
friend class CBrowserHostService;
friend class ByteWrapperImpl;

public:
    COleDocument();
    ~COleDocument();

    // IUnknown methods
    STDMETHODIMP QueryInterface( __in_ecount(1) REFIID, __deref_out_ecount(1) LPVOID*);
    STDMETHODIMP_(DWORD) AddRef();
    STDMETHODIMP_(DWORD) Release();

    // IOleDocument methods
    STDMETHODIMP CreateView(__in_ecount_opt(1) IOleInPlaceSite*, __in_ecount_opt(1) IStream*, DWORD, __deref_out_ecount(1) IOleDocumentView**);
    STDMETHODIMP GetDocMiscStatus(__out_ecount(1) DWORD*);
    STDMETHODIMP EnumViews(__deref_out_ecount(1) IEnumOleDocumentViews**, __deref_out_ecount(1) IOleDocumentView**);

    //IDispatch methods
    STDMETHODIMP GetTypeInfoCount(__out_ecount(1) UINT *);        
    STDMETHODIMP GetTypeInfo(UINT, LCID, __deref_out_ecount(1) ITypeInfo **);        
    STDMETHODIMP GetIDsOfNames( __in_ecount(1) REFIID, __deref_in_ecount(cNames) LPOLESTR *, UINT cNames, LCID, __out_ecount(cNames) DISPID *);
    STDMETHODIMP Invoke( DISPID dispidMember , __in_ecount(1) REFIID, LCID, WORD, __in_ecount(1) DISPPARAMS  *, __out_ecount(1) VARIANT *, __out_ecount_opt(1) EXCEPINFO *, __out_ecount_opt(1) UINT *);

    //IOleCommandTarget methods
    STDMETHODIMP QueryStatus(__in_ecount(1) const GUID *, ULONG cCmds, __inout_ecount(cCmds) OLECMD *, __in_ecount_opt(1) OLECMDTEXT *);
    STDMETHODIMP Exec(__in_ecount_opt(1) const GUID *, DWORD, DWORD, __in_ecount_opt(1) VARIANT *, __out_ecount_opt(1) VARIANT *);

    //IBrowserCallbackServices methods
    STDMETHODIMP        OnBeforeShowNavigationWindow();
    STDMETHODIMP        PostReadyStateChange(int);
    STDMETHODIMP        DelegateNavigation(__in BSTR pUrl, __in_opt BSTR bstrTargetName = NULL, __in_opt BSTR bstrHeaders = NULL);    
    STDMETHODIMP_(long) UpdateAddressBar(__in BSTR pUrl);
    STDMETHODIMP        UpdateBackForwardState();   
    STDMETHODIMP        UpdateTravelLog(BOOL addNewEntry);
    STDMETHODIMP_(long) UpdateProgress(__int64 cBytesCompleted, __int64 cBytesTotal);
    STDMETHODIMP_(long) ChangeDownloadState(VARIANT_BOOL fIsDownloading);    
    STDMETHODIMP_(long) IsDownlevelPlatform();
    STDMETHODIMP_(long) IsHostedInIE();
    STDMETHODIMP_(long) IsAvalonTopLevel();
    STDMETHODIMP_(long) IsShuttingDown();
    STDMETHODIMP_(VARIANT_BOOL) TabOut(VARIANT_BOOL forward); 
    STDMETHODIMP        ProcessUnhandledException(__in BSTR pErrorMsg);
    STDMETHODIMP        GetOleClientSite(__out_ecount(1) IUnknown** ppOleClientSite);
    STDMETHODIMP        UpdateCommands();
    STDMETHODIMP        CreateWebBrowserControlInBrowserProcess(__deref_out IWebBrowser2 **ppWebBrowser);

    // IHlinkTarget methods
    STDMETHODIMP SetBrowseContext(__in_ecount_opt(1) IHlinkBrowseContext *pihlbc);
    STDMETHODIMP GetBrowseContext(__deref_out_opt IHlinkBrowseContext **ppihlbc);
    STDMETHODIMP Navigate(DWORD grfHLNF,__in_ecount_opt(INTERNET_MAX_URL_LENGTH+1) LPCWSTR pwzJumpLocation);
    STDMETHODIMP GetMoniker(__in_opt LPCWSTR pwzLocation, DWORD dwAssign, __deref_out_opt IMoniker **ppimkLocation);
    STDMETHODIMP GetFriendlyName( __in_opt LPCWSTR pwzLocation, __deref_out_opt LPWSTR *ppwzFriendlyName);
    
    //
    // Managed Interop Helpers
    //
    HRESULT InitApplicationServer();
    // CAUTION: ApplicationHelper() will return NULL when the error page is active.
    IBrowserHostServices* ApplicationHelper() { return m_pAppBrowserHostServices; }
    void ReleaseApplicationHelper() { ReleaseInterface(m_pAppBrowserHostServices); }
    HRESULT EnsureTopWindow();
    void RunServerAsync();
    void RunServerHelper(); //[Watson on failure]
    bool    IsAppLoaded();

    // Initialization and helper functions

    HRESULT Initialize(MimeType mimeType, bool bDebugMode);
    HRESULT SetClientSite(IOleClientSite *pSite);
    IOleClientSite *GetClientSite() { return m_spOleClientSite; }
    void InitDocHost(); //[Watson on failure]
    STDMETHODIMP UninitDocHost();
    void SetOuterObject(__in_ecount(1) LPUNKNOWN pOuterObject) { m_pOuterObject = pOuterObject; }
    void SetErrorMessageToDisplay(LPCWSTR msg) { m_errorMessageToDisplay = msg; }
    void OnStartupCallback();
    HRESULT AvalonExecute(__in_ecount_opt(1) IStream *loadStream = NULL);
    void PostShutdown();
    bool Deactivating() { return m_bDeactivating != 0; }
    bool GetHostingFlag(HostingFlags flag) const;

    STRING_PROP(StartupUri);
    STRING_PROP(DebugSecurityZoneURL);
    STRING_PROP(StartupLocation);
    STRING_PROP(ApplicationIdentity);

    void SetPersistHistory(IPersistHistory* pPersistHistory) { m_pPersistHistory = pPersistHistory; }
    LPSTREAM GetHistoryStream() { return m_pHistoryStream; }
    HRESULT SetHistoryStream(__in_ecount_opt(1) LPSTREAM pStream);
    void ReleaseHistoryStream();

    LPSTREAM GetDocumentStream() { return m_pDocumentStream; }
    void SetDocumentStream(__in_ecount_opt(1) LPSTREAM pDocumentStream) { m_pDocumentStream = pDocumentStream; }

    MimeType GetCurrentMime() {return m_CurrentMime;};
    STDMETHODIMP GetClassID(__out_ecount(1) LPCLSID);

    HWND GetTopWindow() { return m_hwndTop; }
    COleInPlaceActiveObject* GetInPlaceActiveObject() { return m_pOleInPlaceActiveObject; }
    COleDocumentView* GetDocumentView() { return m_pOleDocView; }
    INativeProgressPage* GetProgressPage() const { return m_progressPageHost.GetProgressPage(); };
    // That's how the progress page can participate in the message loop. See ForwardTranslateAccelerator()
    // (DllMain.cxx).
    IInputObject* GetHostedInputObject() const { return m_progressPageHost.GetProgressPage(); }

    IWpfHostSupport* GetWpfHostSupport() { return m_pWpfHostSupport; }

    HRESULT ValidateSecurityInFrame();

    IHostBrowser *GetWebBrowser() { return m_pWebBrowser; }
    // Access to IHostBrowser may be needed on a thread other than the main one.
    // This function marshals the interface properly. Caller must release.
    HRESULT GetWebBrowserForCurrentThread(IHostBrowser **ppHostBrowser);
    int GetBrowserProcessIntegrityLevel() const;

    void SetDownloadCompletedEvent(HANDLE hDownloadCompletedEvent) { m_hDownloadCompletedEvent = hDownloadCompletedEvent; }

    HRESULT RestartApplication();

private:

    class CProgressPageHost: public IProgressPageHost
    {
        COleDocument *m_pOleDoc;
        CComObject<CProgressPage> *m_pProgressPage;
    public:
        CProgressPageHost(): m_pOleDoc(0), m_pProgressPage(0) { }
        ~CProgressPageHost();
        HRESULT Init(COleDocument *pOleDoc);
        CProgressPage* GetProgressPage() const { return m_pProgressPage; }

    //IProgressPageHost:
        virtual HWND GetHostWindow() { return m_pOleDoc->GetTopWindow(); }
        virtual void OnProgressPageRendered();
        virtual void OnProgressPageClosed();
        virtual HRESULT ExecOLECommand(UINT cmdId);
    };

    //Helper Functions
    STDMETHODIMP GetWindow( __deref_out_ecount(1) HWND*);
    STDMETHODIMP DeactivateUI();
    STDMETHODIMP DeactivateInPlace();
    STDMETHODIMP OnMenuSelect(HMENU menu);
    STDMETHODIMP OnMenuDeselect();

    void SetHostingFlag(HostingFlags flag, bool value = 1);
    HRESULT QueryBrowserService(REFGUID guidService, REFIID iid, __deref_out_ecount(1) void **pResult);
    STDMETHODIMP InitHostSupport();
    bool HasDownloadCompleted() const;

    // used by IBrowserCallbackServices methods
    STDMETHODIMP_(long) GetOleInPlaceFrameHwnd(__out_ecount(1) HWND *phwnd); 

    // used by IHlinkTarget methods
    STDMETHODIMP NavigateOutside(__in_ecount_opt(INTERNET_MAX_URL_LENGTH+1) LPCWSTR pwzJumpLocation);
    
    LRESULT      QueryStatus(HMENU hMenu);
    LRESULT      QueryStatusHelper(HMENU hMenu);
    void         Exec(DWORD dwCmdId);

    IByteRangeDownloaderService* ByteRangeDownloaderHelper() { return m_pByteRangeDownloaderService; }

    void DeletePointers();

    bool HandleMenuCommand(DWORD cmdId);

    typedef HRESULT (__stdcall IWebBrowser2::*IWebBrowser2_Get_Method)(VARIANT_BOOL*);
    typedef HRESULT (__stdcall IWebBrowser2::*IWebBrowser2_Set_Method)(VARIANT_BOOL);
    STDMETHODIMP ToggleItemState( IWebBrowser2_Get_Method getVal, 
                                  IWebBrowser2_Set_Method setVal,
                                  DWORD menuItem);

    //
    // Static helpers
    //

    // WndProc and message pump for the window our merged menus are associated with
private:
    static LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
public:
    static COleDocument* GetInstance() { return s_pInstance; }

    //
    // Data
    //
private:
    static COleDocument*        s_pInstance;
    __int64                     m_progMaxBytes;
    BOOL                        m_bViewCreated;
    BOOL                        m_bAllowPersistFileLoad;
    // Combination of HostingFlags
    unsigned                    m_hostingFlags, m_validHostingFlags;
    BOOL                        m_bAvalonRunning;

    HWND                        m_hwndTop;  
    long /*READYSTATE*/         m_ReadyState;
    BOOL                        m_bDeactivating, m_bPostedShutdown;
    UINT_PTR                    m_startupCallbackTimerId;

    MimeType                    m_CurrentMime;
    CString                     m_strStartupUri; 
    CString                     m_strDebugSecurityZoneURL; 
    CString                     m_strStartupLocation;
    CString                     m_strApplicationIdentity;
    CComBSTR                    m_errorMessageToDisplay; // see ActivateParameters::pswzErrorMessageFromShim

    LPSTREAM                    m_pHistoryStream;
    IPersistHistory            *m_pPersistHistory;

    COleDocumentView           *m_pOleDocView;
    COleInPlaceObject          *m_pOleInPlaceObject;
    COleInPlaceActiveObject    *m_pOleInPlaceActiveObject;
    COleObject                 *m_pOleObject;
    CTravelLogClient           *m_pTravelLogClient;
    CConnectionPointContainer  *m_pCPC;
    ByteWrapperImpl            *m_pByteWrapper;

    BootStrapper               *m_pBootStrapper;
    IBrowserHostServices       *m_pAppBrowserHostServices;
    IByteRangeDownloaderService *m_pByteRangeDownloaderService;
    LPSTREAM                    m_pDocumentStream;
    IOleAdviseHolder           *m_pOleAdviseHolder;
    IWpfHostSupport*            m_pWpfHostSupport;
    BSTR                        m_bstrProgressAssemblyName;
    BSTR                        m_bstrProgressClassName;
    BSTR                        m_bstrErrorAssemblyName;
    BSTR                        m_bstrErrorClassName;

    //Container Interfaces 
    CComPtr<IOleClientSite>     m_spOleClientSite;
    CComQIPtr<IServiceProvider> m_spBrowserServiceProvider;
    IWebBrowserEventsService   *m_pWebBrowserEventsService;
    IHostBrowser               *m_pWebBrowser;
    CComObject<CWebBrowserEventSink> *m_pWebBrowserEventSink;

    CProgressPageHost           m_progressPageHost;

    HWND                        m_hwndStatusBar;
    BOOL                        m_bSimpleStatusBar;
    BSTR                        m_bstrStatusString;

    // For aggregation and identity purposes. This will do all of the ref-counting. 
    // The COleDocument object will be destroyed under control of the DLL's Deactivate 
    // method, which will be called by the shim EXE when the CHostShim object is destroyed.
    // This interface should not be released, since it was explicitly not AddRef'ed.
    LPUNKNOWN                   m_pOuterObject;

    IStream                    *m_pWebBrowserMarshalStream;

    HANDLE                      m_hMutexDialog;
    HANDLE                      m_hMutexOK;
    HANDLE                      m_hMutexCancel;
    HANDLE                      m_hDownloadCompletedEvent;  // Signaled when download is complete
};

