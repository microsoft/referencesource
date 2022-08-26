//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Implements the OleObject class of PresentationHost.
//
//  History:
//     2002/06/12-murrayw
//          Created
//     2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#include "Precompiled.hxx"
#include "OleObject.hxx"
#include "OleDocument.hxx"
#include "OleDocumentView.hxx"
#include "ProgressPage.hxx"
#include "Utilities.hxx"
#include "urlmoninterop.hxx"
#include "DllMain.hxx" // g_hNoHostTimer


BOOL COleObject::s_bInitialized = FALSE;

//******************************************************************************
//
// COleObject::COleObject()
//   
//******************************************************************************

COleObject::COleObject(__in COleDocument *pOleDoc):
    m_extent()
{
    m_pOleDoc = pOleDoc;
}

//******************************************************************************
//
// COleObject::~COleObject()
//   
//******************************************************************************

COleObject::~COleObject()
{
}

//******************************************************************************
//
// COleObject::QueryInterface()
//   
//******************************************************************************

STDMETHODIMP COleObject::QueryInterface(REFIID riid, __out LPVOID *ppReturn)
{
    return m_pOleDoc->QueryInterface(riid, ppReturn);
}

//******************************************************************************
//
// COleObject::AddRef()
//   
//******************************************************************************

STDMETHODIMP_(ULONG) COleObject::AddRef()
{
    return m_pOleDoc->AddRef();
}

//******************************************************************************
//
// COleObject::Release()
//   
//******************************************************************************

STDMETHODIMP_(ULONG) COleObject::Release()
{
    return m_pOleDoc->Release();
}

//******************************************************************************
//
// COleObject::DoVerb()
//   
//******************************************************************************

STDMETHODIMP COleObject::DoVerb(LONG iVerb,
                                __in_opt LPMSG lpmsg,
                                __in LPOLECLIENTSITE pActiveSite,
                                LONG lindex,
                                HWND hwndParent,
                                LPCRECT pRect)
{
    HRESULT hr = E_FAIL;

    LPOLEDOCUMENTSITE pOleDocSite = NULL;

    CK_PARG(pActiveSite);

    switch (iVerb)
    {
        case OLEIVERB_SHOW:
        case OLEIVERB_PRIMARY:
        case OLEIVERB_UIACTIVATE:
        case OLEIVERB_INPLACEACTIVATE:
        {
            // We only want to go through the activation sequence once.
            if (!s_bInitialized)
            {
                EventWriteWpfHostUm_UIActivationStart();

                CComQIPtr<IOleInPlaceSite> spInPlaceSite(pActiveSite);
                if(!spInPlaceSite)
                    CKHR(E_NOINTERFACE);

                //try to get the IOleDocumentSite pointer
                if(pActiveSite->QueryInterface(IID_IOleDocumentSite, 
                                                reinterpret_cast<LPVOID*>(&pOleDocSite)) == S_OK)
                {
                    //passing NULL to this will cause the site to call our CreateView method
                    //ActivateMe results in the container calling our methods in this order
                    //  IOleDocument->CreateView(&pView)
                    //  IOleDocumentView->SetInPlaceSite(docsite's IOleInPlaceSite)
                    //  IOleDocumentView->UIActivate(TRUE) 
                    //          menu merging, toolbar negotiation, and re-parent its display window to the window returned through IOleInPlaceSite::GetWindow)
                    //  IOleDocumentView->SetRect()
                    //  IOleDocumentView->Show(TRUE)
                    HANDLE_ACTIVATION_FAULT(pOleDocSite->ActivateMe(NULL));
                }
                else // activated as an ActiveX control (support introduced for the Mozilla plug-in)
                {
                    CKHR(m_pOleDoc->EnsureTopWindow());
                    {
                    CComPtr<IOleDocumentView> spView;
                    CKHR(m_pOleDoc->CreateView(spInPlaceSite, NULL, 0, &spView));
                    spView->SetRect(const_cast<LPRECT>(pRect));
                    }
                }

                HWND hwndParent = 0;
                CKHR(spInPlaceSite->GetWindow(&hwndParent));
                CKHR(TerminateIfHostCrashes(hwndParent));
                // We are at the point where we can safely cancel the TerminateIfNoHost mechanism.
                // The TerminateIfHostCrashes mechanism and normal error handling will take over
                // at this point. This will post a message back to the shim; it works because the 
                // unmanaged message pump is still operating at this point.
                if (g_hNoHostTimer)
                {
                    CKHR(CancelTerminateIfNoHost(g_hNoHostTimer));
                }

                /*
                For better responsiveness on cold start, we want to show the progress page as soon as 
                possible, before getting bogged down in the loading of managed code. However, on warm start
                of an application that's already cached (which is really what mostly developers will be
                experiencing), always showing the progress page is likely to be annoying because it will be
                seen just as a flicker. So, to avoid this, here goes a simple cold-start heuristic. It's 
                not very reliable, because only PHDLL is loaded so far, so we don't have good timing resolution,
                and also because PresentationHost.exe restarts itself, so at the second process start it's not
                really cold anymore. If the heuristic misfires, the managed code will show the progress page
                for certain (but only if the applicaiton is not cached already). If it fires in error, oh well,
                your computer is slow and you get a flicker. 
                
                It will really help if we can detect whether the application would be run for the first time.
                That would be a sufficient condition to show the progress page here. But the Fusion's 
                IIdentityAuthority impl-n seems unable to digest the app identity string that we get from 
                dhshim!GetDeploymentDataFromManifest(). Remains to be investigated.
                */
                if(!m_pOleDoc->GetHostingFlag(hfInDebugMode) && m_pOleDoc->GetProgressPage())
                {
                    FILETIME creationTime, currentTime, unused;
                    GetProcessTimes(GetCurrentProcess(), &creationTime, &unused, &unused, &unused);
                    GetSystemTimeAsFileTime(&currentTime);
                    DWORD runtime_ms = abs(int(currentTime.dwLowDateTime - creationTime.dwLowDateTime))/10/1000;
                    if(runtime_ms > 270 /*|| IsApplicationCached(m_pOleDoc->GetApplicationIdentity()) == S_FALSE*/)
                    {
                        // If the progress page fails to show up for some reason we don't
                        // want to prevent the app from running.
                        m_pOleDoc->GetProgressPage()->Show();
                        // The tricky part now is to (efficiently) wait for the progress page to actually show.
                        // It doesn't help that MSHTML does so much asynchronously (but still mostly or 
                        // entirely on the same thread). RunServerAsync() solves this problem.
                    }
                }

                m_pOleDoc->RunServerAsync();

                s_bInitialized = TRUE;

                EventWriteWpfHostUm_UIActivationEnd();
            }
            else
            {
                // This is received when we are in an HTML frame and tabbing into it.
                if(iVerb == OLEIVERB_UIACTIVATE && lpmsg)
                {
                    int nDir = IsTabCycler(*lpmsg);
                    if(nDir && m_pOleDoc->ApplicationHelper())
                    {
                        m_pOleDoc->ApplicationHelper()->TabInto(nDir > 0);
                    }
                }

                hr = S_FALSE;
            }
        }
        break;

        default:
            break;
    }

Cleanup:

    return hr;
}

//******************************************************************************
//
// COleObject::GetClientSite()
//   
//******************************************************************************

STDMETHODIMP COleObject::GetClientSite(__out LPOLECLIENTSITE *ppClientSite)
{
    HRESULT hr = S_OK;
    CK_PARG(ppClientSite);
    *ppClientSite = m_pOleDoc->GetClientSite();
    if (*ppClientSite)
    {
        (*ppClientSite)->AddRef();
    }
Cleanup:
    return hr;
}

/**************************************************************************

   COleObject::SetClientSite()
   
**************************************************************************/

STDMETHODIMP COleObject::SetClientSite(__in LPOLECLIENTSITE pClientSite)
{   
    HRESULT hr = m_pOleDoc->SetClientSite(pClientSite);
  
    if (pClientSite == NULL)
    {
        UrlmonInterop::ReleaseSecurityManager();
    }

    return hr;
}

//******************************************************************************
//
// COleObject::Advise()
//   
//******************************************************************************

STDMETHODIMP COleObject::Advise(__in LPADVISESINK pAdvSink, __out LPDWORD pdwConnection)
{
    STD_ASSERT_UNUSED;
/*
    HRESULT hr = S_OK;

    // if we haven't made an OleAdviseHolder yet, make one.
    if (!m_pOleDoc->m_pOleAdviseHolder)
    {
        CreateOleAdviseHolder(&m_pOleDoc->m_pOleAdviseHolder);
    }

    CKHR(m_pOleDoc->m_pOleAdviseHolder->Advise(pAdvSink, pdwConnection));

Cleanup:

    // pass this call onto the OleAdviseHolder.
    return hr;
*/
}

//******************************************************************************
//
// COleObject::Unadvise()
//   
//******************************************************************************

STDMETHODIMP COleObject::Unadvise(DWORD dwConnection)
{
    STD_ASSERT_UNUSED;
/*
    HRESULT hr = S_OK;

    if (!m_pOleDoc->m_pOleAdviseHolder)
    {
        hr = OLE_E_NOCONNECTION;
        goto Cleanup;
    }

    CKHR(m_pOleDoc->m_pOleAdviseHolder->Unadvise(dwConnection));

Cleanup:

    return hr;
*/
}

//******************************************************************************
//
// COleObject::SetHostNames()
//   
//******************************************************************************

STDMETHODIMP COleObject::SetHostNames(LPCOLESTR szContainerApp, 
                                      LPCOLESTR szContainerObj)
{
    return S_OK;
}

//******************************************************************************
//
// COleObject::GetExtent()
//   
//******************************************************************************

STDMETHODIMP COleObject::GetExtent(DWORD dwDrawAspect, LPSIZEL lpsizel)
{
    // See SetExtent()
    *lpsizel = m_extent;
    return S_OK;
}

//******************************************************************************
//
// COleObject::SetExtent()
//   
//******************************************************************************

STDMETHODIMP COleObject::SetExtent(DWORD dwDrawAspect, LPSIZEL lpsizel)
{
    // The ATL control host always calls SetExtent and then GetExtent, without checking the
    // error code returned by GetExtent. So, we have to return it what it gave us. Then this
    // size will be passed to DoVerb().
    m_extent = *lpsizel;
    return S_OK;
}

//******************************************************************************
//
// COleObject::Update()
//   
//******************************************************************************

STDMETHODIMP COleObject::Update()
{
    return S_OK;
}

//******************************************************************************
//
// COleObject::Close()
//   
//******************************************************************************

STDMETHODIMP COleObject::Close(DWORD dwSaveOption)
{
    //close the view
    m_pOleDoc->GetDocumentView()->CloseView(0);
    //close the application object
    m_pOleDoc->PostShutdown();
    return S_OK;
}

//******************************************************************************
//
// COleObject::EnumVerbs()
// From MSDN: The default handler's implementation of IOleObject::EnumVerbs uses 
// the registry to enumerate an object's verbs. If an object application is to 
// use the default handler's implementation, it should return OLE_S_USEREG.
//******************************************************************************

STDMETHODIMP COleObject::EnumVerbs(LPENUMOLEVERB *ppenumOleVerb)
{
    return OLE_S_USEREG;
}

//******************************************************************************
//
//  COleObject::SetMoniker()
//   
//******************************************************************************

STDMETHODIMP COleObject::SetMoniker(DWORD dwWhichMoniker, LPMONIKER pMoniker)
{
    STD_ASSERT_UNUSED;
/*
    LPRUNNINGOBJECTTABLE pRot;
    LPMONIKER pTempMoniker;

    HRESULT hr = S_OK;

    if (!m_pOleDoc->m_pOleClientSite)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (NOERROR != m_pOleDoc->m_pOleClientSite->GetMoniker(OLEGETMONIKER_ONLYIFTHERE, 
                                                           OLEWHICHMK_OBJFULL, 
                                                           &pTempMoniker))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (m_pOleDoc->m_pOleAdviseHolder)
    {
        m_pOleDoc->m_pOleAdviseHolder->SendOnRename(pTempMoniker);
    }

    if (NOERROR == GetRunningObjectTable(0, &pRot))
    {
        if (m_pOleDoc->m_dwRegister)
        {
            pRot->Revoke(m_pOleDoc->m_dwRegister);
        }

        //








*/
}

//******************************************************************************
//
// COleObject::GetMoniker()
//
//******************************************************************************

STDMETHODIMP COleObject::GetMoniker(DWORD dwAssign, 
                                    DWORD dwWhichMoniker,
                                    __out LPMONIKER *ppmk)
{
    STD_ASSERT_UNUSED;
/*
    HRESULT hr = S_OK;

    CK_PARG(ppmk);

    *ppmk = NULL;

    hr = m_pOleDoc->m_pOleClientSite->GetMoniker(OLEGETMONIKER_ONLYIFTHERE, 
                                                 OLEWHICHMK_OBJFULL, 
                                                 ppmk);
Cleanup:

    return hr;
*/
}

//******************************************************************************
//
// COleObject::InitFromData()
//   
//******************************************************************************

STDMETHODIMP COleObject::InitFromData(LPDATAOBJECT pDataObject,
                                      BOOL fCreation,
                                      DWORD dwReserved)
{
    return E_NOTIMPL;
}

//******************************************************************************
//
// COleObject::GetClipboardData()
//   
//******************************************************************************

STDMETHODIMP COleObject::GetClipboardData(DWORD dwReserved,
                                          __out LPDATAOBJECT *ppDataObject)
{
    HRESULT hr = E_NOTIMPL;

    CK_PARG(ppDataObject);

    *ppDataObject = NULL;

Cleanup:

    return hr;
}

//******************************************************************************
//
// COleObject::IsUpToDate()
//   
//******************************************************************************

STDMETHODIMP COleObject::IsUpToDate()
{
    return S_OK;
}

//******************************************************************************
//
// COleObject::GetUserClassID()
//   
//******************************************************************************

STDMETHODIMP COleObject::GetUserClassID(__out CLSID *pClsid)
{
    HRESULT hr = S_OK;

    CKHR(m_pOleDoc->GetClassID(pClsid));

Cleanup:

    return hr;
}

//******************************************************************************
//
// COleObject::GetUserType()
// From MSDN: You can use the implementation provided by the default handler by 
// returning OLE_S_USEREG as your application's implementation of this method. 
//******************************************************************************

STDMETHODIMP COleObject::GetUserType(DWORD dwFormOfType, 
                                     __out LPOLESTR *pszUserType)
{
    return OLE_S_USEREG;
}

//******************************************************************************
//
// COleObject::EnumAdvise()
//   
//******************************************************************************

STDMETHODIMP COleObject::EnumAdvise(__out LPENUMSTATDATA *ppenumAdvise)
{
    STD_ASSERT_UNUSED;
/*
    HRESULT hr = S_OK;

    CK_PARG(ppenumAdvise);

    *ppenumAdvise = NULL;

    // pass on to the OLE Advise holder.
    CKHR(m_pOleDoc->m_pOleAdviseHolder->EnumAdvise(ppenumAdvise));

Cleanup:

    return hr;
*/
}

//******************************************************************************
//
// COleObject::GetMiscStatus()
//   
//******************************************************************************

STDMETHODIMP COleObject::GetMiscStatus(DWORD dwAspect, __out DWORD *pdwStatus)
{
    HRESULT hr = OLE_S_USEREG;

    CK_PARG(pdwStatus);

    // This is what trident does. Should we do this as well?
    //(OLEMISC_INSIDEOUT | \
    // OLEMISC_ACTIVATEWHENVISIBLE | \
    // OLEMISC_RECOMPOSEONRESIZE | \
    // OLEMISC_SUPPORTSMULTILEVELUNDO | \
    // OLEMISC_CANTLINKINSIDE | \
    // OLEMISC_SETCLIENTSITEFIRST)*/
    //
    // right now, we just do SETCLIENTSITEFIRST since this
    // seems to be necessary for shdocvw to do the right
    // thing when loading us from history. that is, we
    // don't want to be loaded via SuperNavigate or
    // IPersistMoniker::Load. In at least one place 
    // shdocvw checks for this flag and proceeds to do things
    // in the right sequence.  we'll consider adding the rest of
    // the flags when we figure out why Trident uses them.

    *pdwStatus = OLEMISC_SETCLIENTSITEFIRST;

    return S_OK;

Cleanup:

    return hr;
}

//******************************************************************************
//
// COleObject::SetColorScheme()
//   
//******************************************************************************

STDMETHODIMP COleObject::SetColorScheme(LPLOGPALETTE lpLogpal)
{
    return S_OK;
}
