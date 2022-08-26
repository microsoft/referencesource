//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Defines the PersistHistory class of PresentationHost.
//
//  History:
//     2002/06/19-murrayw
//          Created
//     2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#pragma once

//******************************************************************************
//
// CPersistHistory class definition
//
//******************************************************************************

class CPersistHistory : IPersistHistory
{
friend class CHtmlWindow;

private:
    CHostShim      *m_pHostShim;
public:
    CPersistHistory::CPersistHistory(__in CHostShim *);
    CPersistHistory::~CPersistHistory();

    // IUnknown Methods
    STDMETHODIMP QueryInterface(REFIID, __out LPVOID*);
    STDMETHODIMP_(ULONG)AddRef();
    STDMETHODIMP_(ULONG)Release();

    // IPersist Methods
    STDMETHODIMP GetClassID(LPCLSID);

    // IPersistHistory Methods
    STDMETHODIMP LoadHistory(__in IStream *pStream, IBindCtx *pbc);    
    STDMETHODIMP SaveHistory(__in IStream *pStream);
    STDMETHODIMP SetPositionCookie(DWORD dwPositioncookie);
    STDMETHODIMP GetPositionCookie(__out DWORD *pdwPositioncookie);

private:
    STDMETHODIMP WriteToStream(IStream* pOutputStream, DWORD dwData);
    STDMETHODIMP ReadFromStream(IStream* pInputStream, DWORD* pdwData);

};
