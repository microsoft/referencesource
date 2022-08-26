//+-----------------------------------------------------------------------
//	
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description: This is the impelementation of the InternetSecurityManagerSite
//               It is required to ensure that the security prompt is modal
//          
//  History:
//     2005/06/14 - Created the file 
//     2007/09/20 - Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#include "Precompiled.hxx"
#include "SecurityManagerSiteImpl.hxx"
#include "OleDocument.hxx"

#ifndef MAXDWORD
// From winnt.h
#define MAXDWORD    0xffffffff
#endif

/******************************************************************************
SecurityManagerSiteImpl::SecurityManagerSiteImpl
******************************************************************************/
SecurityManagerSiteImpl::SecurityManagerSiteImpl(__in_ecount_opt(1) COleDocument *pOleDoc):
    m_ObjRefCount(1), m_pOleDoc(pOleDoc), m_TopLevelWindow(0)
{
    if(m_pOleDoc != NULL)
    {
        m_pOleDoc->AddRef();
    }
}

/******************************************************************************
SecurityManagerSiteImpl::SecurityManagerSiteImpl
******************************************************************************/
SecurityManagerSiteImpl::SecurityManagerSiteImpl():
    m_ObjRefCount(1), m_pOleDoc(NULL), m_TopLevelWindow(0)
{
}

//******************************************************************************
//
// IInternetSecurityMgrSite Implementation
//
//******************************************************************************

/******************************************************************************
SecurityManagerSiteImpl::GetWindow
******************************************************************************/
HRESULT SecurityManagerSiteImpl::GetWindow(__deref_out_ecount(1) HWND *pHwnd)
{
    HRESULT hr = S_OK;
    IOleWindow *pOleWindow = NULL;

    if(m_pOleDoc ==  NULL || pHwnd == NULL)
    {
        CKHR(E_FAIL);
    }

    //extract window
    CKHR( m_pOleDoc->GetClientSite()->QueryInterface(IID_IOleWindow, (void**)&pOleWindow));
    CKHR(pOleWindow->GetWindow(pHwnd));

    //store locally and return a copy
    m_TopLevelWindow = *pHwnd;

Cleanup:    
    ReleaseInterface(pOleWindow);
    return hr;

}

/******************************************************************************
SecurityManagerSiteImpl::EnableModeless
******************************************************************************/
HRESULT SecurityManagerSiteImpl::EnableModeless(__in BOOL fEnable)
{
    return S_OK;
}

//******************************************************************************
//
// IUnknown Implementation
//
//******************************************************************************
/******************************************************************************
 SecurityManagerSiteImpl::QueryInterface
******************************************************************************/
STDMETHODIMP  SecurityManagerSiteImpl::QueryInterface( __in_ecount(1) REFIID riid, __deref_out_ecount(1) LPVOID *ppReturn)
{
    HRESULT hr = E_NOINTERFACE;
    *ppReturn = NULL;

    if (riid == IID_IUnknown)
    {
        *ppReturn = (IUnknown*)((IInternetSecurityMgrSite*)this);
    }
    else if (riid == IID_IInternetSecurityMgrSite)
    {
        *ppReturn = this;
    }
    if (*ppReturn)
    {
        AddRef();
        hr = S_OK;
    }
   
    return hr;
}                                             

/******************************************************************************
SecurityManagerSiteImpl::AddRef
******************************************************************************/
STDMETHODIMP_(DWORD) SecurityManagerSiteImpl::AddRef()
{
    return InterlockedIncrement(&m_ObjRefCount);
}

/******************************************************************************
SecurityManagerSiteImpl::Release
******************************************************************************/
STDMETHODIMP_(DWORD)  SecurityManagerSiteImpl::Release()
{
    DWORD uRefs = InterlockedDecrement(&m_ObjRefCount);
    
    if (uRefs == 0)
    {
        ReleaseInterface(m_pOleDoc);
        delete this;
    }
   
    return uRefs;
}
