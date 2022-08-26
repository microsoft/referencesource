//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Defines the ClassFactory of PresentationHost.
//
// History:
//      2002/06/12-murrayw
//          Created
//      2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#pragma once 

//******************************************************************************
//
//    CClassFactory class definition
//
//******************************************************************************

#define MAX_TOKENS 2

class CClassFactory : public IClassFactory
{
protected:
    static DWORD s_dwRegistrationToken[MAX_TOKENS];
    static LPCLSID s_clsIdActivated;

protected:
    REFCLSID m_clsId;
    DWORD m_ObjRefCount;

    HRESULT RegisterClassObject(DWORD& dwRegistrationToken);
    void SetActivatedClsId(__in REFCLSID clsId) { s_clsIdActivated = (LPCLSID) &clsId; }

private:
    CClassFactory(const CClassFactory &src); // not implemented
    void operator=(const CClassFactory &src); // not implemented
public:
    CClassFactory(__in REFCLSID clsId);
    ~CClassFactory();

    // Methods for registering/unregistering CLSIDs
    static HRESULT RegisterClassObjects();
    static void RevokeAllClassObjects();
    static LPCLSID GetActivatedClsId() { return CClassFactory::s_clsIdActivated; }

    //IUnknown methods
    STDMETHODIMP QueryInterface(REFIID, __out LPVOID*);
    STDMETHODIMP_(DWORD) AddRef();
    STDMETHODIMP_(DWORD) Release();

    //IClassFactory methods
    STDMETHODIMP CreateInstance(__in LPUNKNOWN, REFIID, __out LPVOID*);
    STDMETHODIMP LockServer(BOOL);
};
