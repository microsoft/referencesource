// +-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Support code for hosting the WebOC in the browser process (when in IE 7+ Protected Mode).
//      See Framework\System\Windows\Controls\WebBrowser.cs.
//
//  History:
//      04/24/08 - Microsoft - Created
//
//------------------------------------------------------------------------------

#include "Precompiled.hxx"


HRESULT CreateIDispatchSTAForwarder(__in IDispatch *pDispatchDelegate, __deref_out IDispatch **ppForwarder);

// Trivially wraps a given IDispatch implementation. This is how we get cross-context calls to a managed object
// exposing IDispatch to arrive on the same (STA) thread. See full explanation of the problem in the managed
// ActiveXHelper.CreateIDispatchSTAForwarder().
class CIDispatchSTAForwarder: 
    public CComObjectRootEx<CComSingleThreadModel>,
    //public IDispatch, - base of DWebBrowserEvents2; not included to prevent warning C4584
    // Exposing DWebBrowserEvents2 is merely a performance optimization. IE first queries for it and if that 
    // fails just uses IDispatch. DWebBrowserEvents2 actually has no additional vtable methods.
    public DWebBrowserEvents2
{
    CComPtr<IDispatch> m_spDispatchDelegate;
public:
    void Init(IDispatch *pDispatchDelegate) { m_spDispatchDelegate = pDispatchDelegate; }

    BEGIN_COM_MAP(CIDispatchSTAForwarder)
       COM_INTERFACE_ENTRY(IDispatch)
       COM_INTERFACE_ENTRY(DWebBrowserEvents2)
    END_COM_MAP( )

//IDispatch:
    HRESULT STDMETHODCALLTYPE GetTypeInfoCount( 
        /* [out] */ __RPC__out UINT *pctinfo)
    {
        return m_spDispatchDelegate->GetTypeInfoCount(pctinfo);
    }
    HRESULT STDMETHODCALLTYPE GetTypeInfo( 
        /* [in] */ UINT iTInfo,
        /* [in] */ LCID lcid,
        /* [out] */ __RPC__deref_out_opt ITypeInfo **ppTInfo)
    {
        return m_spDispatchDelegate->GetTypeInfo(iTInfo, lcid, ppTInfo);
    }
    HRESULT STDMETHODCALLTYPE GetIDsOfNames( 
        /* [in] */ __RPC__in REFIID riid,
        /* [size_is][in] */ __RPC__in_ecount_full(cNames) LPOLESTR *rgszNames,
        /* [range][in] */ UINT cNames,
        /* [in] */ LCID lcid,
        /* [size_is][out] */ __RPC__out_ecount_full(cNames) DISPID *rgDispId)
    {
        return m_spDispatchDelegate->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispId);
    }
    /* [local] */ HRESULT STDMETHODCALLTYPE Invoke( 
        /* [in] */ DISPID dispIdMember,
        /* [in] */ REFIID riid,
        /* [in] */ LCID lcid,
        /* [in] */ WORD wFlags,
        /* [out][in] */ DISPPARAMS *pDispParams,
        /* [out] */ VARIANT *pVarResult,
        /* [out] */ EXCEPINFO *pExcepInfo,
        /* [out] */ UINT *puArgErr)
    {
        return m_spDispatchDelegate->Invoke(dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    }
};

// DLL-exported function
HRESULT CreateIDispatchSTAForwarder(__in IDispatch *pDispatchDelegate, __deref_out IDispatch **ppForwarder)
{
    if(!pDispatchDelegate || !ppForwarder)
        return E_POINTER;
    CComObject<CIDispatchSTAForwarder> *pForwarder = 0;
    HRESULT hr = CComObject<CIDispatchSTAForwarder>::CreateInstance(&pForwarder);
    if(SUCCEEDED(hr))
    {
        pForwarder->Init(pDispatchDelegate);
        *ppForwarder = pForwarder;
        pForwarder->AddRef();
    }
    return hr;
}
