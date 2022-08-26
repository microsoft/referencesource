//+------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Defines CTravelLogClient.
//
// History:
//      2001/09/05-michdav          Created
//      2005/01/19-kusumav          Changed from CHtmlWindow to CTravelLogClient
//      2007/09/20-Microsoft          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//--------------------------------------------------------------------------------

#pragma once

#include <tlogstg.h>

/**************************************************************************

   CTravelLogClient class definition

**************************************************************************/

class CTravelLogClient : public ITravelLogClient, public IWebBrowserEventsUrlService
{    
friend class COleDocument;
private:
    COleDocument *m_pOleDoc;        
public:
    CTravelLogClient::CTravelLogClient(__in COleDocument *);
    CTravelLogClient::~CTravelLogClient();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID, __out LPVOID*);
    STDMETHODIMP_(ULONG)AddRef();
    STDMETHODIMP_(ULONG)Release();

    // IWebBrowserEventsUrlService
    STDMETHODIMP GetUrlForEvents(__out BSTR * pBstr);

    // ITravelLogClient
    STDMETHODIMP FindWindowByIndex(DWORD, __out_opt IUnknown **);
    STDMETHODIMP GetWindowData(__in IStream*, __out LPWINDOWDATA);
    STDMETHODIMP LoadHistoryPosition(__in LPOLESTR, DWORD);
    
private:
    BSTR _bstrURL;
    BSTR _bstrLocation;
    BSTR _bstrShortcut;
    BSTR _bstrFrameName;
};
