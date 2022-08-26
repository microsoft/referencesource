//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      Implements GetUrlForEvents.
//
// History:
//      2001/09/05-michdav          Created
//      2005/01/19-kusumav          Changed from CHtmlWindow to CTravelLogClient
//      2007/09/20-Microsoft          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

/**************************************************************************
   #include statements
**************************************************************************/

#include "Precompiled.hxx"
#include "TravelLogClient.hxx"
#include "OleDocument.hxx"
#include "DllMain.hxx"

/**************************************************************************

   CTravelLogClient::CTravelLogClient()
   
**************************************************************************/

CTravelLogClient::CTravelLogClient(__in COleDocument *pOleDoc)
{
    m_pOleDoc       = pOleDoc;    
    _bstrURL        = NULL;
    _bstrLocation   = NULL;
    _bstrShortcut   = NULL;
    _bstrFrameName  = NULL;
}

/**************************************************************************

   CTravelLogClient::~CTravelLogClient()
   
**************************************************************************/

CTravelLogClient::~CTravelLogClient() 
{ 
    if (_bstrURL)
        SysFreeString(_bstrURL);
    if (_bstrLocation)
        SysFreeString(_bstrLocation);
    if (_bstrShortcut)
        SysFreeString(_bstrShortcut);
    if (_bstrFrameName)
        SysFreeString(_bstrFrameName);
}

/**************************************************************************

   CTravelLogClient::QueryInterface()
   
**************************************************************************/

STDMETHODIMP CTravelLogClient::QueryInterface(REFIID riid, __out LPVOID *ppReturn)
{
    return m_pOleDoc->QueryInterface(riid, ppReturn);
}

/**************************************************************************

   CTravelLogClient::AddRef()
   
**************************************************************************/

STDMETHODIMP_(ULONG) CTravelLogClient::AddRef()
{
    return m_pOleDoc->AddRef();
}

/**************************************************************************

   CTravelLogClient::Release()
   
**************************************************************************/

STDMETHODIMP_(ULONG) CTravelLogClient::Release()
{
    return m_pOleDoc->Release();
}


// IWebBrowserEventsUrlService
STDMETHODIMP CTravelLogClient::GetUrlForEvents(__out BSTR * pBstr) 
{  
    *pBstr = SysAllocString(_bstrURL);
    if (*pBstr != NULL)
    {
        return S_OK; 
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}    

STDMETHODIMP CTravelLogClient::FindWindowByIndex(DWORD dwID, __out_opt IUnknown ** ppunk) 
{
    if (!ppunk)
        return E_INVALIDARG;

    // dwID = 0 in some cases when this method is called by shdocvw. 
    // Return immediately, if that is the case.
    if (dwID == 0)
        return E_FAIL;

    // Ask the managed app if this entry is invokable.
    // NOTE: The given id may belong to a different instance of PresentationHost. 
    // See Journal._journalEntryId for explanation of how we deal with this.
    if (m_pOleDoc->ApplicationHelper() && 
        m_pOleDoc->ApplicationHelper()->CanInvokeJournalEntry((int)dwID) != 0)
    {
        *ppunk = (ITravelLogClient*)this;
        //We are handing out an interface via a COM call,so AddRef per COM rules.
        //And Trident does the same as well.
        AddRef();
        return S_OK;
    }
    
    *ppunk = NULL;
    return E_FAIL;
}
    
//typedef struct _WINDOWDATA
//    {
//    DWORD dwWindowID;
//    UINT uiCP;
//    LPITEMIDLIST pidl;
//    /* [string] */ LPOLESTR lpszUrl;
//    /* [string] */ LPOLESTR lpszUrlLocation;
//    /* [string] */ LPOLESTR lpszTitle;
//    }     WINDOWDATA;

STDMETHODIMP CTravelLogClient::GetWindowData(__in IStream* pStream, __out LPWINDOWDATA pWindowData) 
{
    HRESULT hr = S_OK;
    LARGE_INTEGER  liZero = {0, 0};

    int    entryId   = -1;
    LPWSTR tempUrl          = NULL;
    LPWSTR tempTitle        = NULL;

    if (!pStream)
        CKHR(CreateStreamOnHGlobal(NULL, TRUE, &pStream));

    CKHR(pStream->Seek(liZero, STREAM_SEEK_SET, NULL));

    CKHR(SaveHistoryHelper(pStream, &entryId, &tempUrl, &tempTitle));

    pWindowData->dwWindowID = (DWORD)entryId;

    // Assumption: The URL and Title strings are allocated [by managed code] with CoTaskMemAlloc().
    pWindowData->lpszUrl = tempUrl; 
    //Inserted journal entries (Journal.AddEntry) may not always have a title
    pWindowData->lpszTitle = tempTitle;
    
    // no locations for now
    pWindowData->lpszUrlLocation = NULL;

Cleanup:
    return hr;
}
    
STDMETHODIMP CTravelLogClient::LoadHistoryPosition(__in LPOLESTR, DWORD) 
{
    // no local anchors right now, so we
    // don't need to implement this.
    return S_OK;
}
