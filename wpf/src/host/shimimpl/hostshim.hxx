//------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Implements the minimal set of interfaces required for
//     the version-independent hosting shim.
//
// History:
//      2005/05/09 - Microsoft
//          Created
//      2007/09/20 - Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#pragma once

typedef enum ActivationType
{
    Unknown = 0,
    FileActivation = 0x01,
    MonikerActivation = 0x02,
    HistoryActivation = 0x04
};

class CHostShim : 
    public IOleObject,
    public IStdMarshalInfo,
    public IObjectWithSite,
    public IHlinkTarget
{
public:
    CHostShim();
    ~CHostShim();

    static CHostShim* GetInstance();
    static CVersion* GetValidVersion();

    // IUnknown methods
    STDMETHODIMP QueryInterface( __in_ecount(1) REFIID, __deref_out_ecount(1) LPVOID*);
    STDMETHODIMP_(DWORD) AddRef();
    STDMETHODIMP_(DWORD) Release();

    // IStdMarshalInfo
    STDMETHODIMP GetClassForHandler(DWORD dwDestContext, void * pvDestContext, CLSID * pClsid);

    // IObjectWithSite
    // The IE in-proc handler (CBrowserInprocHandler) passes its implementation of IHostBrowser to SetSite().
    STDMETHODIMP SetSite(IUnknown *pUnkSite);
    STDMETHODIMP GetSite(REFIID riid, /* [iid_is][out] */ void **ppvSite);

    // IHlinkTarget methods
    STDMETHODIMP SetBrowseContext(__in_ecount_opt(1) IHlinkBrowseContext *pihlbc);
    STDMETHODIMP GetBrowseContext(__deref_out_ecount(1) IHlinkBrowseContext **ppihlbc);
    STDMETHODIMP Navigate(DWORD grfHLNF,__in_ecount_opt(INTERNET_MAX_URL_LENGTH+1) LPCWSTR pwzJumpLocation);
    STDMETHODIMP GetMoniker(__in_ecount(1) LPCWSTR pwzLocation, DWORD dwAssign, __deref_out_ecount(1) IMoniker **ppimkLocation);
    STDMETHODIMP GetFriendlyName( __in_ecount(1) LPCWSTR pwzLocation, __deref_out_ecount(1) LPWSTR *ppwzFriendlyName);

    // IOleObject methods

    // When we are being activated from IPersistHistory, the browser unfortunately requests IOleObject and
    // calls SetClientSite before QI'ing for IPersistHistory. Therfore we need this implementation here, which
    // delegates to a "real" object in the DLL. When SetClientSite is called, we hold onto it until we've reached
    // a point where we have the DLL and can call SetClientSite on the "real" OleObject.
    void SetOleObjectDelegate(IOleObject* pOleObjectDelegate);
    IOleObject* GetOleObjectDelegate() { return m_pOleObjectDelegate; }

    virtual STDMETHODIMP SetClientSite(__in LPOLECLIENTSITE pClientSite);
    virtual STDMETHODIMP Advise(__in LPADVISESINK pAdvSink, __out LPDWORD pdwConnection);
    virtual STDMETHODIMP SetHostNames(LPCOLESTR szContainerApp, LPCOLESTR szCOntainerObj);
    virtual STDMETHODIMP DoVerb(LONG iVerb, __in_opt LPMSG lpMsg, __in LPOLECLIENTSITE pActiveSite, LONG lIndex, HWND hwndParent, LPCRECT lprcPosRect); 
    virtual STDMETHODIMP GetExtent(DWORD dwDrawAspect, LPSIZEL psizel);
    virtual STDMETHODIMP Update();
    virtual STDMETHODIMP Close(DWORD dwSaveOption);
    virtual STDMETHODIMP Unadvise(DWORD dwToken);
    virtual STDMETHODIMP EnumVerbs(LPENUMOLEVERB* ppEnumOleVerb);
    virtual STDMETHODIMP GetClientSite(__out LPOLECLIENTSITE* ppClientSite);
    virtual STDMETHODIMP SetMoniker(DWORD dwWhichMoniker, LPMONIKER pmk);
    virtual STDMETHODIMP GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, __out LPMONIKER* ppmk);
    virtual STDMETHODIMP InitFromData(LPDATAOBJECT pDataObject, BOOL fCreation, DWORD dwReserved);
    virtual STDMETHODIMP GetClipboardData(DWORD dwReserved, __out LPDATAOBJECT* ppDataObject);
    virtual STDMETHODIMP IsUpToDate();
    virtual STDMETHODIMP GetUserClassID(__out CLSID* pclsId);
    virtual STDMETHODIMP GetUserType(DWORD dwFormOfType, __out LPOLESTR* pszUserType);
    virtual STDMETHODIMP SetExtent(DWORD dwDrawAspect, LPSIZEL psizel);
    virtual STDMETHODIMP EnumAdvise(__out LPENUMSTATDATA* ppenumAdvise);
    virtual STDMETHODIMP GetMiscStatus(DWORD dwAspect, __out LPDWORD pdwStatus);
    virtual STDMETHODIMP SetColorScheme(LPLOGPALETTE pLogPal);

    // Other public methods
    void Execute(); // Invokes Watson and terminates on failure.
    STDMETHODIMP GetClassID(__out_ecount(1) LPCLSID);

    void SetMimeType(MimeType mimeType) { m_mimeType = mimeType; }
    MimeType GetMimeType() { return m_mimeType; }

    // When IPersistMoniker::Load is called, it will bail for local file activation, to allow 
    // IPersistFile to take it. It does not allow non-local file: activation. In some cases, e.g.
    // being launched from a hyperlink in Word, IPersistMoniker::Load is NOT called, so we have then
    // IsAllowPersistFileLoadSet method.
    void SetAllowPersistFileLoad(BOOL bAllow) { m_bAllowPersistFileLoad = bAllow; m_bAllowPersistFileLoadSet = TRUE; }
    BOOL GetAllowPersistFileLoad() { return m_bAllowPersistFileLoad; }
    BOOL IsAllowPersistFileLoadSet() { return m_bAllowPersistFileLoadSet; }

    void SetByteWrapperImpl(class ByteWrapperImpl* pByteWrapperImpl) { m_pByteWrapperImpl = pByteWrapperImpl; }
    ByteWrapperImpl* GetByteWrapperImpl() { return m_pByteWrapperImpl; }

    void SetByteRangeDownloaderHelper(IByteRangeDownloaderService* pByteRangeDownloaderService) { m_pByteRangeDownloaderService = pByteRangeDownloaderService; }
    IByteRangeDownloaderService* GetByteRangeDownloaderHelper() { return m_pByteRangeDownloaderService; }

    void SetHistoryStream(IStream* pHistoryStream) { m_pHistoryStream = pHistoryStream; }
    LPSTREAM GetHistoryStream() { return m_pHistoryStream; }

    void SetDocumentStream(LPSTREAM pDocumentStream) { m_pDocumentStream = pDocumentStream; }
    LPSTREAM GetDocumentStream() { return m_pDocumentStream; }

    HANDLE GetDownloadCompletedEvent() { return m_hDownloadCompletedEvent; }

    HRESULT SaveToHistory(__in_ecount(1) IStream* pHistoryStream);
    HRESULT LoadFromHistory(__in_ecount(1) IStream* pHistoryStream, __in_ecount(1) IBindCtx* pBindCtx);

    CVersion* GetVersion() { return m_pVersion; }

    void SetErrorMessageToDisplay(LPCWSTR msg) { m_strErrorMessageToDisplay.SetValue(msg); }

    // Plain string properties
    STRING_PROP(StartupUri);
    STRING_PROP(StartupLocation);
    STRING_PROP(LocalDeploymentManifestPath);
    STRING_PROP(RequestedVersion);
    STRING_PROP(ApplicationIdentity);

    ActivationType GetActivationType() { return m_activationType; }
    void SetActivationType(ActivationType activationType) { m_activationType = activationType; }

private:
    HRESULT DetermineRequestedVersion();
    void InvokeFrameworkBootstrapperWithFallback();
    HRESULT InvokeFrameworkBootstrapper();

private:
    static CHostShim*   m_pInstance;
    long                m_ObjRefCount;

    IUnknown*           m_pUnkSite;
    LPUNKNOWN           m_pInnerObject;

    CVersion*           m_pVersion;
    class CPersistMoniker* m_pPersistMoniker;
    class CPersistFile*    m_pPersistFile;
    class CPersistHistory* m_pPersistHistory;
    ByteWrapperImpl*    m_pByteWrapperImpl;
    IByteRangeDownloaderService* m_pByteRangeDownloaderService;

    CString             m_strStartupUri;
    CString             m_strStartupLocation;
    CString             m_strLocalDeploymentManifestPath;
    CString             m_strRequestedVersion;
    CString             m_strApplicationIdentity;
    CString             m_strErrorMessageToDisplay; // see ActivateParameters::pswzErrorMessageFromShim

    LPSTREAM            m_pHistoryStream;
    LPSTREAM            m_pDocumentStream;
    MimeType            m_mimeType;
    BOOL                m_bAllowPersistFileLoad;
    BOOL                m_bAllowPersistFileLoadSet;

    HANDLE              m_hDownloadCompletedEvent;  // Event triggers when download is completed
    
    ActivationType      m_activationType;
    bool                m_bInvokeFrameworkBootstrapper;

    IOleObject* m_pOleObjectDelegate;
    LPOLECLIENTSITE m_pPendingClientSite;
};
