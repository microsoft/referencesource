//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Implements the ClassFactory of PresentationHost.
//
// History:
//      2002/06/12-murrayw
//          Created
//      2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#include "Precompiled.hxx"
#include "ClassFactory.hxx"
#include "HostShim.hxx"
#include "Main.hxx"

DWORD CClassFactory::s_dwRegistrationToken[] = {0,0};
LPCLSID CClassFactory::s_clsIdActivated = NULL;

//******************************************************************************
//
//   CClassFactory::CClassFactory
//
//******************************************************************************

CClassFactory::CClassFactory(__in REFCLSID clsId)
    : m_clsId(clsId)
{
    m_ObjRefCount = 0;
}

//******************************************************************************
//
//   CClassFactory::~CClassFactory
//
//******************************************************************************

CClassFactory::~CClassFactory()
{
}


//******************************************************************************
//
// CClassFactory::RegisterClassObjects
//
// This method creates ClassFactory objects and registers their objects based
// on the flags. We don't hold on to the factories; OLE has them, and when
// we Revoke them, OLE will Release them and they will be deleted.
//******************************************************************************

HRESULT CClassFactory::RegisterClassObjects()
{
    HRESULT hr = S_OK;

    ASSERT(s_dwRegistrationToken[0] == 0);
    ASSERT(s_dwRegistrationToken[1] == 0);

#ifdef DOCUMENT_SHIM
    CClassFactory* pFactory = new CClassFactory(CLSID_DocObjPackage);
    CK_ALLOC(pFactory);
    CKHR(pFactory->RegisterClassObject(s_dwRegistrationToken[0]));
#endif

#ifdef APPLICATION_SHIM
    CClassFactory* pFactory = new CClassFactory(CLSID_DocObjXaml);
    CK_ALLOC(pFactory);
    CKHR(pFactory->RegisterClassObject(s_dwRegistrationToken[0]));

    pFactory = new CClassFactory(CLSID_DocObjXapp);
    CK_ALLOC(pFactory);
    CKHR(pFactory->RegisterClassObject(s_dwRegistrationToken[1]));
#endif

Cleanup:
    if (hr != S_OK)
    {
        CClassFactory::RevokeAllClassObjects();
    }

    return hr;
}

//******************************************************************************
//
// CClassFactory::RevokeAllClassObjects
//
// Revoke any registered ClassObjects, and delete the ClassFactories.
// This doesn't check any return values, nor does it return anything,
// because this is a cleanup step, and if it fails, it really isn't
// actionable.
//******************************************************************************

void CClassFactory::RevokeAllClassObjects()
{
    for (UINT tokenIndex = 0; tokenIndex < MAX_TOKENS; ++tokenIndex)
    {
        if (s_dwRegistrationToken[tokenIndex])
        {
            CoRevokeClassObject(s_dwRegistrationToken[tokenIndex]);
            s_dwRegistrationToken[tokenIndex] = 0;
        }
    }
}

//******************************************************************************
//
//  CClassFactory::RegisterClassObject
//
//  This method registers the ClsObject that this is offering.
//******************************************************************************

HRESULT CClassFactory::RegisterClassObject(DWORD& dwRegistrationToken)
{
    HRESULT hr = S_OK;

    LPUNKNOWN pUnknown = reinterpret_cast<LPUNKNOWN> (this);

    //register the MIME-type handler class object
    CKHR(CoRegisterClassObject(m_clsId,
                               pUnknown,
                               CLSCTX_LOCAL_SERVER,
                               REGCLS_SINGLEUSE,
                               &dwRegistrationToken));
Cleanup:
    if (hr != S_OK && dwRegistrationToken)
    {
        CoRevokeClassObject(dwRegistrationToken);
        dwRegistrationToken = 0;
    }
    return hr;
}

//******************************************************************************
//
//  CClassFactory::QueryInterface
//
//******************************************************************************

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, 
                                           __out LPVOID *ppReturn)
{
    HRESULT hr = S_OK;

    CK_PARG(ppReturn);

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppReturn = reinterpret_cast<LPUNKNOWN>
                       (reinterpret_cast<LPCLASSFACTORY>(this));

        m_ObjRefCount++;
    }
    else if (IsEqualIID(riid, IID_IClassFactory))
    {
        *ppReturn = reinterpret_cast<LPCLASSFACTORY>(this);
        m_ObjRefCount++;
    }   
    else
    {
        *ppReturn = NULL;
        hr = E_NOINTERFACE;
    }

Cleanup:

    return hr;
}                                             

//******************************************************************************
//
//   CClassFactory::AddRef
//
//******************************************************************************

STDMETHODIMP_(DWORD) CClassFactory::AddRef()
{
    return ++m_ObjRefCount;
}

//******************************************************************************
//
//   CClassFactory::Release
//
//******************************************************************************

STDMETHODIMP_(DWORD) CClassFactory::Release()
{
    if (--m_ObjRefCount == 0)
    {
        delete this;
        return 0;
    }
       
    return m_ObjRefCount;
}

//******************************************************************************
//
//   CClassFactory::CreateInstance
//
//******************************************************************************

STDMETHODIMP CClassFactory::CreateInstance(__in LPUNKNOWN pUnknown, 
                                           REFIID riid, 
                                           __out LPVOID *ppObject)
{
    EventWriteWpfHostUm_ClassFactoryCreateInstance();

    HRESULT hr = S_OK;
    CHostShim * pHostShim = NULL;

    CK_PARG(ppObject);

    *ppObject = NULL;

    if (pUnknown != NULL)
    {
        hr = CLASS_E_NOAGGREGATION;
        goto Cleanup;
    }

    pHostShim = new CHostShim();
    CK_ALLOC(pHostShim);

    CKHR(pHostShim->QueryInterface(riid, ppObject));

    SetActivatedClsId(m_clsId);

Cleanup:

    if (FAILED(hr))
    {
        DEL_PTR(pHostShim);
    }

    // Revoke any class factories that may have been created.
    // Current implementation only creates one.
    CClassFactory::RevokeAllClassObjects();

    return hr;
}

//******************************************************************************
//
//   CClassFactory::LockServer
//
//******************************************************************************

STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    HRESULT hr = S_OK;

    if (fLock)
    {
        InterlockedIncrement(&g_ServerRefCount);
    }
    else
    {
        InterlockedDecrement(&g_ServerRefCount);
        
        if (g_ServerRefCount <= 0)
        {
            PostQuitMessage(ERROR_SUCCESS);
        }
    }

    return hr;
}

