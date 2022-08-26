//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Deployment progress page. Supersedes the original one implemented in managed code + XAML.
//      Being able to show the progress page before loading any managed code gives much better
//      responsiveness on cold start.
//      For compatibility with Media Center, "custom" deployment UI still has to be supported.
//      (This is done via IWpfHostSupport.)
//      Design document: http://wpf/sites/Arrowhead/Shared%20Documents/Specs/Optimizing%20XBAP%20Progress%20Page.docx
//
//  History:
//      2007/12/xx   Microsoft     Created
//
//------------------------------------------------------------------------

#pragma once

// INativeProgressPage: interface exposed to the managed hosting code, which drives the progress page
// starting from the manifest (re)download done by ClickOnce.
// For simplicity, it's defined straight here, not in IDL.
DECLARE_INTERFACE_IID(INativeProgressPage, "1f681651-1024-4798-af36-119bbe5e5665"): public IUnknown
{
    STDMETHOD(Show)() = 0;
    STDMETHOD(Hide)() = 0;
    STDMETHOD(ShowProgressMessage)(BSTR message) = 0;
    STDMETHOD(SetApplicationName)(BSTR appName) = 0;
    STDMETHOD(SetPublisherName)(BSTR publisherName) = 0;
    STDMETHOD(OnDownloadProgress)(UINT64 bytesDownloaded, UINT64 bytesTotal) = 0;
}; 

// IProgressPageHost is implemented by the native object that owns the CProgressPage instance.
// It's introduced to avoid a direct dependence on COleDocument, which is way too entangled with the rest of
// the world already.
interface IProgressPageHost
{
    virtual HWND GetHostWindow() = 0;
    virtual void OnProgressPageRendered() = 0;
    virtual void OnProgressPageClosed() = 0;
    virtual HRESULT ExecOLECommand(UINT cmdId) = 0;
};

class ATL_NO_VTABLE CProgressPage: 
    public CComObjectRoot,
    public INativeProgressPage,
    public IInputObject, // That's how this object participates in the message pump.
    private IDispatch // Implemented to allow script in the HTML page to call back (via window.external).
{
public:
    CProgressPage(): m_pHost(0), m_hHostWindow(0), m_bLoaded(false), 
		m_bPartialRendered(false), m_bRendered(false), m_bFailed(false) { }
    ~CProgressPage();

    void Init(IProgressPageHost *pHost);

    // Internal invariant: Iff m_spHtmlDocument is non-null, the page is considered 'active', and all other 
    // smart pointers should also be valid. The page may not be fully loaded (m_bLoaded) or 
	// visible (m_bRendered) yet.
    bool IsActive() const { return m_spHtmlDocument != NULL && m_bFailed != TRUE; }

BEGIN_COM_MAP(CProgressPage)
   COM_INTERFACE_ENTRY(INativeProgressPage)
   COM_INTERFACE_ENTRY(IDispatch)
   COM_INTERFACE_ENTRY(IInputObject)
END_COM_MAP( )

// INativeProgressPage:
    STDMETHOD(Show)();
    STDMETHOD(Hide)();
    STDMETHOD(ShowProgressMessage)(BSTR message);
    STDMETHOD(SetApplicationName)(BSTR appName);
    STDMETHOD(SetPublisherName)(BSTR publisherName);
    STDMETHOD(OnDownloadProgress)(UINT64 bytesDownloaded, UINT64 bytesTotal);

//IInputObject:
    STDMETHOD(UIActivateIO)(BOOL fActivate, __in_opt MSG *pMsg) { return E_NOTIMPL; }
    STDMETHOD(HasFocusIO)() { return E_NOTIMPL; }
    STDMETHOD(TranslateAcceleratorIO)(__in MSG *pMsg);

private:
// IDispatch implementaion for script callback
    STDMETHOD(GetTypeInfoCount)(__out_ecount(1) UINT *) { return E_NOTIMPL; }
    STDMETHOD(GetTypeInfo)(UINT, LCID, __deref_out_ecount(1) ITypeInfo **) { return E_NOTIMPL; }
    STDMETHOD(GetIDsOfNames)( REFIID, __in_ecount(cNames) LPOLESTR *, UINT cNames, LCID, __out_ecount(cNames) DISPID *);
    STDMETHOD(Invoke)( DISPID dispidMember , __in_ecount(1) REFIID, LCID, WORD, __in_ecount(1) DISPPARAMS  *, __out_ecount(1) VARIANT *, __out_ecount_opt(1) EXCEPINFO *, __out_ecount_opt(1) UINT *);

    // Note: Make sure to clear all pointers in Hide().
    IProgressPageHost *m_pHost;
    HWND m_hHostWindow;
    bool m_bLoaded, m_bPartialRendered, m_bRendered, m_bFailed;
    CComQIPtr<IAxWinHostWindow> m_spControlHost;
    CComPtr<IHTMLDocument2> m_spHtmlDocument;
    CComPtr<IDispatch> m_spDocumentScript;
    HMODULE m_hostDllMuiModule;
};
