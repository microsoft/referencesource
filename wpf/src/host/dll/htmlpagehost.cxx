//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Helpers for hosting HTMLDocument ("Trident"). See .hxx.
//
//  History:
//      2007/12/xx   Microsoft     Created, stuff mostly factored out of ErrorPage.cxx
//      2009/08/07   Microsoft     Implemented IInternetSecurityManager to enable running script under
//                                  Enhanced Security Configuration.
//
//------------------------------------------------------------------------

#include "Precompiled.hxx"
#include "HTMLPageHost.hxx"
#include "OleDocument.hxx"
#include "HostSupport.h"

class ATL_NO_VTABLE CMSHTMLHost :
    public CAxHostWindow,
    public IInternetSecurityManager
{
public:
    HRESULT CreateHTMLDocumentControl(
        HWND hHostWindow, __in_opt IStream *pInitStream,  __deref_out IHTMLDocument2 **ppHtmlDoc)
    {
        ASSERT(hHostWindow);
        HRESULT hr;
        CComPtr<IUnknown> spUnkControl;
        DWORD docHostFlags;

        AtlAxWinInit();

        const wchar_t * const HTMLDocumentCLSID = L"{25336920-03F9-11cf-8FD0-00AA00686F13}";
        CKHR(CreateControlLicEx(HTMLDocumentCLSID, hHostWindow, pInitStream, &spUnkControl, IID_NULL, 0, 0));
        CKHR(spUnkControl->QueryInterface(__uuidof(IHTMLDocument2), (void**)ppHtmlDoc));

        // Setting DOCHOSTUIFLAG_THEME, primarily to get buttons with rounded corners.
        // Apparently having a manifest reference to Microsoft.Windows.Common-Controls v6.0.0.0 is not enough.
        get_DocHostFlags(&docHostFlags);
        put_DocHostFlags(docHostFlags | DOCHOSTUIFLAG_THEME);

    Cleanup:
        return hr;
    }

    DECLARE_NOT_AGGREGATABLE(CMSHTMLHost)

    BEGIN_COM_MAP(CMSHTMLHost)
       COM_INTERFACE_ENTRY(IInternetSecurityManager)
       COM_INTERFACE_ENTRY_CHAIN(CAxHostWindow)
    END_COM_MAP( )

//IServiceProvider override
	STDMETHOD(QueryService)(REFGUID rsid, REFIID riid, void** ppvObj) 
    {
        if(IsEqualGUID(rsid, SID_SInternetSecurityManager))
			return QueryInterface(riid, ppvObj);
        return __super::QueryService(rsid, riid, ppvObj);
    }

//IInternetSecurityManager:
    STDMETHOD(SetSecuritySite)(/* [unique][in] */ __RPC__in_opt IInternetSecurityMgrSite *pSite) 
        { return INET_E_DEFAULT_ACTION; }
    STDMETHOD(GetSecuritySite)(/* [out] */ __RPC__deref_out_opt IInternetSecurityMgrSite **ppSite) 
        { return INET_E_DEFAULT_ACTION; }
    STDMETHOD(MapUrlToZone)(/* [in] */ __RPC__in LPCWSTR pwszUrl,
            /* [out] */ __RPC__out DWORD *pdwZone,
            /* [in] */ DWORD dwFlags) 
        { return INET_E_DEFAULT_ACTION; }
    STDMETHOD(GetSecurityId)(/* [in] */ __RPC__in LPCWSTR pwszUrl,
            /* [size_is][out] */ __RPC__out_ecount_full(*pcbSecurityId) BYTE *pbSecurityId,
            /* [out][in] */ __RPC__inout DWORD *pcbSecurityId,
            /* [in] */ DWORD_PTR dwReserved)
        { return INET_E_DEFAULT_ACTION; }
    STDMETHOD(ProcessUrlAction)( /* [in] */ __RPC__in LPCWSTR pwszUrl,
            /* [in] */ DWORD dwAction,
            /* [size_is][out] */ __RPC__out_ecount_full(cbPolicy) BYTE *pPolicy,
            /* [in] */ DWORD cbPolicy,
            /* [unique][in] */ __RPC__in_opt BYTE *pContext,
            /* [in] */ DWORD cbContext,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwReserved)
    {
        // Override the script blocking of Windows Server's Enhanced Security Configuration.
        // This also helps the marginally interesting case of script disabled for the Internet zone.
        // (Both the the progress page and error page are treated as if they come from the Internet zone.)
        if (dwAction == URLACTION_SCRIPT_RUN)
        {
            *(DWORD *)pPolicy = URLPOLICY_ALLOW;
            return S_OK;
        }
        return INET_E_DEFAULT_ACTION;
    }
    STDMETHOD(QueryCustomPolicy)( /* [in] */ __RPC__in LPCWSTR pwszUrl,
            /* [in] */ __RPC__in REFGUID guidKey,
            /* [size_is][size_is][out] */ __RPC__deref_out_ecount_full_opt(*pcbPolicy) BYTE **ppPolicy,
            /* [out] */ __RPC__out DWORD *pcbPolicy,
            /* [in] */ __RPC__in BYTE *pContext,
            /* [in] */ DWORD cbContext,
            /* [in] */ DWORD dwReserved)
        { return INET_E_DEFAULT_ACTION; }
    STDMETHOD(SetZoneMapping)( /* [in] */ DWORD dwZone,
            /* [in] */ __RPC__in LPCWSTR lpszPattern,
            /* [in] */ DWORD dwFlags)
        { return INET_E_DEFAULT_ACTION; }
    STDMETHOD(GetZoneMappings)( /* [in] */ DWORD dwZone,
            /* [out] */ __RPC__deref_out_opt IEnumString **ppenumString,
            /* [in] */ DWORD dwFlags)
        { return INET_E_DEFAULT_ACTION; }
};

HRESULT CHTMLPageHostingHelper::CreateHTMLDocumentControl(
    HWND hHostWindow, __in_opt IStream *pInitStream, 
    __deref_opt_out IUnknown **ppAxContainer, __deref_out IHTMLDocument2 **ppHtmlDoc)
{
    HRESULT hr;
    CComObject<CMSHTMLHost> *pHost = 0;
    CKHR(CComObject<CMSHTMLHost>::CreateInstance(&pHost));
    CKHR(pHost->CreateHTMLDocumentControl(hHostWindow, pInitStream, ppHtmlDoc));
    if(ppAxContainer)
    {
        *ppAxContainer = (IOleClientSite*)pHost; pHost->AddRef();
    }
Cleanup:
    if(FAILED(hr))
    {
        delete pHost;
    }
    return hr;
}

bool CHTMLPageHostingHelper::HandleNavigationKeys(const MSG &msg)
{
    COleDocument *pOleDoc = COleDocument::GetInstance();
    DWORD keyCode = (DWORD)msg.wParam;
    switch(msg.message)
    {
    case WM_KEYDOWN:
        if(keyCode == VK_F5)
            return SUCCEEDED(pOleDoc->RestartApplication());
        if(keyCode == VK_ESCAPE)
            return SUCCEEDED(pOleDoc->Exec(0, OLECMDID_STOP, 0, 0, 0));

        if(keyCode != VK_BACK)
            break;
        keyCode = VK_LEFT;
        // fall-through
    case WM_SYSKEYDOWN:
        if(keyCode == VK_LEFT || keyCode == VK_RIGHT)
        {
            CComPtr<IHostBrowser> spWebBrowser;
            if(SUCCEEDED(pOleDoc->GetWebBrowserForCurrentThread(&spWebBrowser)))
            {
                if(keyCode == VK_LEFT)
                {
                    spWebBrowser->GoBack();
                }
                else
                {
                    spWebBrowser->GoForward();
                }
            }
            return true;
        }
    }
    return false;        
}
