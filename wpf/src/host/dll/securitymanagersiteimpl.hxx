//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description: This is the declaration of the InternetSecurityManagerSite
//               It is required to ensure that the security prompt is modal
//          
//  History:
//     2005/06/14 - Created the file 
//     2007/09/20 - Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#pragma once 


class SecurityManagerSiteImpl : IInternetSecurityMgrSite
{
     
public:
     SecurityManagerSiteImpl();
     SecurityManagerSiteImpl(__in_ecount_opt(1) COleDocument* pOleDoc);
     
     // IUnknown 
     STDMETHODIMP QueryInterface( __in_ecount(1) REFIID, __deref_out_ecount(1) LPVOID*);
     STDMETHODIMP_(DWORD) AddRef();
     STDMETHODIMP_(DWORD) Release();

     // IInternetSecurityMgrSite
     STDMETHODIMP EnableModeless(__in BOOL fEnable); 
     STDMETHODIMP GetWindow(__deref_out_ecount(1) HWND *pHwnd);

private:
     COleDocument* m_pOleDoc;
     long          m_ObjRefCount; 
     HWND          m_TopLevelWindow;
};
