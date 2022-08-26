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

#include "Precompiled.hxx"
#include "WebBrowserEvents.hxx"
#include "OleDocument.hxx"


_ATL_FUNC_INFO CWebBrowserEventSink::OnQuitFnInfo = {CC_STDCALL, VT_EMPTY, 0, { } };

HRESULT CWebBrowserEventSink::Attach(COleDocument *pOleDocument, IWebBrowser2 *pWebBrowser)
{
    ASSERT(pOleDocument && pWebBrowser);
    m_spOleDocument = pOleDocument;
    m_spWebBrowser = pWebBrowser;
    return DispEventAdvise(pWebBrowser);
}

void CWebBrowserEventSink::Detach()
{
    if(m_spWebBrowser)
    {
        // Without Unadvise our interfaces won't be released when the browser navigates away.
        DispEventUnadvise(m_spWebBrowser);
        m_spWebBrowser = NULL;
    }
    m_spOleDocument = NULL;
}

void CWebBrowserEventSink::OnQuit()
{
    if(m_spOleDocument)
    {
        static_cast<COleDocument*>(m_spOleDocument.p)->PostShutdown();
        m_spOleDocument = NULL;
    }
    m_spWebBrowser = NULL;
}

