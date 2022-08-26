//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Defines the OleObject class of PresentationHost.
//
//  History:
//     2002/06/12-murrayw
//          Created
//     2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#pragma once 

//******************************************************************************
//
//   Forward declarations
//
//******************************************************************************

class COleDocument;

class COleObject : public IOleObject
{
private:

    COleDocument *m_pOleDoc;   
    SIZEL m_extent;
    static BOOL s_bInitialized;

public:

    COleObject::COleObject(__in COleDocument *pOleDoc);
    COleObject::~COleObject();

    //IUnknown methods
    STDMETHODIMP QueryInterface(REFIID, __out LPVOID*);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    //IOleObject methods
    STDMETHODIMP SetClientSite(__in LPOLECLIENTSITE);
    STDMETHODIMP Advise(__in LPADVISESINK, __out LPDWORD);
    STDMETHODIMP SetHostNames(LPCOLESTR, LPCOLESTR);
    STDMETHODIMP DoVerb(LONG, __in_opt LPMSG, __in LPOLECLIENTSITE, LONG, HWND, LPCRECT);
    STDMETHODIMP GetExtent(DWORD, LPSIZEL);
    STDMETHODIMP Update();
    STDMETHODIMP Close(DWORD);
    STDMETHODIMP Unadvise(DWORD);
    STDMETHODIMP EnumVerbs(LPENUMOLEVERB*);
    STDMETHODIMP GetClientSite(__out LPOLECLIENTSITE*);
    STDMETHODIMP SetMoniker(DWORD, LPMONIKER);
    STDMETHODIMP GetMoniker(DWORD, DWORD, __out LPMONIKER*);
    STDMETHODIMP InitFromData(LPDATAOBJECT, BOOL, DWORD);
    STDMETHODIMP GetClipboardData(DWORD, __out LPDATAOBJECT*);
    STDMETHODIMP IsUpToDate();
    STDMETHODIMP GetUserClassID(__out CLSID*);
    STDMETHODIMP GetUserType(DWORD, __out LPOLESTR*);
    STDMETHODIMP SetExtent(DWORD, LPSIZEL);
    STDMETHODIMP EnumAdvise(__out LPENUMSTATDATA*);
    STDMETHODIMP GetMiscStatus(DWORD, __out LPDWORD);
    STDMETHODIMP SetColorScheme(LPLOGPALETTE);
};
