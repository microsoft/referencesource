//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description: This file is a common point of interop with the 
//               security Manager. It also stores an instance of
//               security Manager such that we do not need to keep
//               CoCreating it over and over.
//          
//  History:
//     2005/06/14 - Created the file 
//     2007/09/20 - Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#include "Precompiled.hxx"
#include "urlmoninterop.hxx"
#include "SecurityManagerSiteImpl.hxx"

#ifndef MAXDWORD
// From winnt.h
#define MAXDWORD    0xffffffff
#endif


//initialize static variable
IInternetSecurityManager *UrlmonInterop::s_pInternetSecurityManager = NULL;
IInternetSecurityMgrSite *UrlmonInterop::s_pSecurityMgrSiteImpl = NULL;



//call this only when you are done using the securitymanager
HRESULT UrlmonInterop::ReleaseSecurityManager()
{
    ReleaseInterface(UrlmonInterop::s_pInternetSecurityManager);
    ReleaseInterface(UrlmonInterop::s_pSecurityMgrSiteImpl);
    return S_OK;
}

// convenience functionality if you want to get an instance of this pointer
// the expectation is that the consumer of this interface calls release on the interface
// after he/she is done
HRESULT UrlmonInterop::GetSecurityManager(__out_ecount_opt(1)IInternetSecurityManager **ppInternetSecurityManager)
{
    HRESULT hr = S_OK;
    // check for null pointer and send it back
    if(ppInternetSecurityManager == NULL) 
    {
        CKHR(E_INVALIDARG);
    }
    if(s_pInternetSecurityManager == NULL)
    {
        // cocreate the pointer to the COM interface we require
        // CLSID for internet security managed "7b8a2d94-0ac9-11d1-896c-00c04Fb6bfc4"
        // GUID for the interface we want to extract "79eac9ee-baf9-11ce-8c82-00aa004ba90b"
        CKHR(CoCreateInstance(CLSID_InternetSecurityManager, 
                              NULL, CLSCTX_INPROC_SERVER,
                              IID_IInternetSecurityManager, 
                              (void **)&s_pInternetSecurityManager));
    }
    *ppInternetSecurityManager = s_pInternetSecurityManager;
    s_pInternetSecurityManager->AddRef(); 
Cleanup:
    return hr;
}

// This code takes the url action to invoke and calls it after it
// creates an instance of the InternetSecurityManager
HRESULT UrlmonInterop::ProcessUrlActionWrapper(
    DWORD dwurlAction,
    __in_ecount(1) LPOLESTR pUrl,
    __in_ecount_opt(1)COleDocument* pOleDoc,
    __out_ecount(1)BOOL &fAllow)
{
    HRESULT                     hr = S_OK;
    DWORD                       dwPolicy = URLPOLICY_DISALLOW;
    IInternetSecurityManager    *pSecurityManager = NULL;
    Assert(pUrl != NULL);
    CKHR(GetSecurityManager(&pSecurityManager));

    UrlmonInterop::s_pSecurityMgrSiteImpl = (IInternetSecurityMgrSite*)new SecurityManagerSiteImpl(pOleDoc);
    // create an object and register it
    CKHR(RegisterSecurityManagerSite());
   
    //  ProcessUrlAction will do the right thing based on call and registry settings  
    CKHR(pSecurityManager->ProcessUrlAction(pUrl, 
                                                                     dwurlAction,  
                                                                     (BYTE *) &dwPolicy, 
                                                                      sizeof(dwPolicy),
                                                                      NULL,
                                                                      0,
                                                                      PUAF_DEFAULT,
                                                                      NULL));
    //false if user clicked no true otherwise
    fAllow = (dwPolicy == URLPOLICY_ALLOW);
Cleanup:
    ReleaseInterface(pSecurityManager);

    return hr;
}

HRESULT UrlmonInterop::RegisterSecurityManagerSite()
{
    HRESULT hr = S_OK;
    CKHR(UrlmonInterop::s_pInternetSecurityManager->SetSecuritySite((IInternetSecurityMgrSite*)UrlmonInterop::s_pSecurityMgrSiteImpl));
Cleanup:
    return hr;
}
