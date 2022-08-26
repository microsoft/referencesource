//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Defines the PersistFile class of PresentationHost
//
//  History:
//     2002/06/12-murrayw
//          Created
//     2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#pragma once 

class CPersistFile : IPersistFile
{
private:
    CHostShim*    m_pHostShim;

public:
    CPersistFile::CPersistFile(__in CHostShim* pHostShim);
    CPersistFile::~CPersistFile();

    STDMETHODIMP QueryInterface(REFIID, __out LPVOID*);
    STDMETHODIMP_(ULONG)AddRef();
    STDMETHODIMP_(ULONG)Release();

    STDMETHODIMP GetClassID(__out LPCLSID);
    STDMETHODIMP Save(__in_ecount(MAX_PATH+1) LPCOLESTR, BOOL);
    STDMETHODIMP SaveCompleted(LPCOLESTR);
    STDMETHODIMP Load(__in_ecount(MAX_PATH + 1) LPCOLESTR, DWORD);
    STDMETHODIMP IsDirty(void);
    STDMETHODIMP GetCurFile(__out LPOLESTR*);
};
