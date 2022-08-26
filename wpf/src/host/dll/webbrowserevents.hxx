// +-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Implements an event sink for the IWebBrowser2 events
//
//  History:
//     2006/08/30   Microsoft     Created
//     2007/09/20   Microsoft     Ported Windows->DevDiv. See SourcesHistory.txt.
//
// ------------------------------------------------------------------------

#pragma once 


class ATL_NO_VTABLE CWebBrowserEventSink: 
    public CComObjectRoot,
    public IDispEventSimpleImpl<1, CWebBrowserEventSink, &DIID_DWebBrowserEvents2>,
    public IUnknown
{
public:
    HRESULT Attach(COleDocument *pOleDocument, IWebBrowser2 *pWebBrowser);
    void Detach();

BEGIN_COM_MAP(CWebBrowserEventSink)
   COM_INTERFACE_ENTRY(IUnknown)
END_COM_MAP( )

BEGIN_SINK_MAP(CWebBrowserEventSink)
    SINK_ENTRY_INFO(1, DIID_DWebBrowserEvents2, DISPID_ONQUIT, OnQuit, &OnQuitFnInfo )
END_SINK_MAP()

private:
    static _ATL_FUNC_INFO OnQuitFnInfo;

    CComPtr<IOleDocument> m_spOleDocument;
    CComPtr<IWebBrowser2> m_spWebBrowser;

// DWebBrowserEvents2:
    void _stdcall OnQuit();
};
