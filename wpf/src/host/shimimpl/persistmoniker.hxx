//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Defines the PersistMoniker class of PresentationHost.
//
//  History:
//     2002/06/19-murrayw
//          Created
//     2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//     2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#pragma once 

//******************************************************************************
//
// CPersistStorage class definition
//
//******************************************************************************

class CPersistMoniker : public IPersistMoniker
{
public:
    CPersistMoniker(__in CHostShim*);
    ~CPersistMoniker();

    //IUnknown methods
    STDMETHODIMP QueryInterface(REFIID, __out LPVOID*);
    STDMETHODIMP_(DWORD) AddRef();
    STDMETHODIMP_(DWORD) Release();

    //IPersistMoniker methods   
    STDMETHODIMP GetClassID(__out LPCLSID);
    STDMETHODIMP GetCurMoniker(LPMONIKER*);
    STDMETHODIMP Save(LPMONIKER, LPBC, BOOL);
    STDMETHODIMP SaveCompleted(LPMONIKER, LPBC);

    STDMETHODIMP Load(BOOL fFullyAvailable, 
                      __in LPMONIKER pMoniker, 
                      __in LPBC pbc, 
                      DWORD dwReserved);
    
    STDMETHODIMP IsDirty();    

private:
    CHostShim   *m_pHostShim;
};

