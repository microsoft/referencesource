//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Implements the PersistMoniker class of PresentationHost.
//
//  History:
//     2002/06/19-murrayw
//          Created
//     2003/06/30/03-Microsoft
//          Ported ByteRangeDownloader to WCP
//     2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//          Removed dependence on ByteWrapper[Impl] and DownloadInfo for PresentationHost.exe.
//
//------------------------------------------------------------------------

#include "Precompiled.hxx"
#include "PersistMoniker.hxx"
#include "HostShim.hxx"
#include "BindStatusCallback.hxx"
#include "ShimUtilities.hxx"

//******************************************************************************
//
// CPersistMoniker::CPersistMoniker()
//
//******************************************************************************

CPersistMoniker::CPersistMoniker(__in CHostShim *pHostShim)
{
    m_pHostShim  = pHostShim;
}

//*******************************************************************************
//
// CPersistMoniker::~CPersistMoniker()
//
//******************************************************************************

CPersistMoniker::~CPersistMoniker()
{
}

//******************************************************************************
//
// CPersistMoniker::QueryInterface()
//
//******************************************************************************

STDMETHODIMP CPersistMoniker::QueryInterface(REFIID riid, __out LPVOID *ppReturn)
{
    return m_pHostShim->QueryInterface(riid, ppReturn);
}

//******************************************************************************
//
// CPersistMoniker::AddRef()
//
//******************************************************************************

STDMETHODIMP_(ULONG) CPersistMoniker::AddRef()
{
    return m_pHostShim->AddRef();
}

//******************************************************************************
//
// CPersistMoniker::Release()
//
//******************************************************************************

STDMETHODIMP_(ULONG) CPersistMoniker::Release()
{
    return m_pHostShim->Release();
}

//******************************************************************************
//
// CPersistMoniker::GetClassID()
//
//******************************************************************************

STDMETHODIMP CPersistMoniker::GetClassID(__out LPCLSID pClassID)
{
    return m_pHostShim->GetClassID(pClassID);
}

//******************************************************************************
//
// CPersistMoniker::Save()
//
//******************************************************************************

STDMETHODIMP CPersistMoniker::Save(LPMONIKER, LPBC, BOOL)
{
    return S_OK;
}

//******************************************************************************
//
// CPersistMoniker::GetCurMoniker()
//
//******************************************************************************

STDMETHODIMP CPersistMoniker::GetCurMoniker(LPMONIKER*)
{
    return S_OK;
}

//******************************************************************************
//
// CPersistMoniker::SaveCompleted()
//
//******************************************************************************

STDMETHODIMP CPersistMoniker::SaveCompleted(LPMONIKER, LPBC)
{
    return S_OK;
}

#define MAX_SCHEMA 64

//******************************************************************************
//
// CPersistMoniker::Load()
//
//******************************************************************************

STDMETHODIMP CPersistMoniker::Load(BOOL fFullyAvailable, 
                                   __in LPMONIKER pMoniker, 
                                   __in LPBC pbc, 
                                   DWORD)
{
    HRESULT hr = S_OK;

    EventWriteWpfHostUm_IPersistMonikerLoadStart();

    m_pHostShim->SetActivationType(MonikerActivation);

    LPOLESTR  pwszURL   = NULL;
    WCHAR     pszResult[INTERNET_MAX_URL_LENGTH + 1];

    CComPtr<CBindStatusCallback> spBindStatusCallback;
    CComPtr<IFillLockBytes> spFlb;
    CComPtr<IMoniker> spMkNew;
    CComPtr<IBindCtx> spBCNew;

    m_pHostShim->SetAllowPersistFileLoad(FALSE);

    CK_PARG(pMoniker);
    CKHR(pMoniker->GetDisplayName(pbc, pMoniker, &pwszURL));
    CK_ALLOC(pwszURL);
    m_pHostShim->SetStartupUri(pwszURL);

    DWORD cchResult;
    CKHR(CoInternetParseUrl(pwszURL,
                            PARSE_SCHEMA,
                            NULL,
                            pszResult,
                            INTERNET_MAX_URL_LENGTH + 1,
                            &cchResult,
                            0));
    // If we are a local file, FAIL to load from the IPersistMoniker 
    // so that we will load from the IPersistFile, instead.
    //
    // 

    if (cchResult == 4 && _wcsnicmp(pszResult,L"file",4) == 0)
    {
        if (fFullyAvailable)
        {
            hr = E_FAIL;     // E_FAIL
        }
        else
        {
            hr = S_FALSE;     // S_FALSE
        }

        //Returning E_NOTIMPL for now to work around a urlmon bug
        //hr = E_NOTIMPL;

        //We will allow IPersistFile::Load only for local files. If IPersistMoniker::Load
        //fails for http urls, urlmon will then call IPersistFile::Load with the 
        //cache filename. It is unsafe and incorrect for us to allow that load since
        //we will grant elevated security privileges and also relative references will 
        //no longer work
        m_pHostShim->SetAllowPersistFileLoad(TRUE);

        goto Cleanup;
    }

    spBindStatusCallback = new CBindStatusCallback();
    CK_ALLOC(spBindStatusCallback);
    //!!! Make sure to set BINDINFO_OPTIONS_ENABLE_UTF8 in all implementations of IBindStatusCallback.
    //!!! It is needed for correct encoding of non-ASCII characters in URL paths, per RFC 3986 & 3987.

    // UrlMon Workaround: UrlMon was returning the wrong file size information for files that
    // were in the cache.  Our past workaround was to delete the cache file, however IE suggested
    // that we create a new Moniker and BindContext before binding to storage.  This would allow
    // us to use the cache file if the content has not expired.
    CKHR(CreateURLMonikerEx(NULL, pwszURL, &spMkNew, URL_MK_UNIFORM));
    CKHR(CreateBindCtx( 0, &spBCNew ));
    CKHR(RegisterBindStatusCallback(spBCNew, spBindStatusCallback, NULL, 0));

    CKHR(spMkNew->BindToStorage(spBCNew,
                                 NULL, 
                                 IID_IStream, 
                                 reinterpret_cast<void**>(&(spBindStatusCallback->m_pStream))));
    if (!FAILED(hr))
    {
        hr = S_OK;
    }

    BOOL fReceivedWmQuit;
    //
    MsgWaitForSingleObject(spBindStatusCallback->m_hMimeArrivedEvent, INFINITE, &fReceivedWmQuit);

    m_pHostShim->SetMimeType(spBindStatusCallback->GetMimeType());

    if ((m_pHostShim->GetMimeType() == MimeType_Application) || 
        (m_pHostShim->GetMimeType() == MimeType_Markup))
    {
        MsgWaitForSingleObject(spBindStatusCallback->m_hManifestAvailableEvent, INFINITE, &fReceivedWmQuit);
        m_pHostShim->SetLocalDeploymentManifestPath(spBindStatusCallback->GetCacheFilename());
        // Trigger DownloadCompleted since downloading is performed in managed code.
        SignalAvalonStartupRelatedEvent(m_pHostShim->GetDownloadCompletedEvent());
    }
    else
    {
        CKHR(E_INVALIDARG);
    }

    // Note: CHostShim::Execute() is called below.
    hr = S_OK;

Cleanup:
    // We purposefully return E_FAIL to cause IPersistFile loading.
    if (!m_pHostShim->GetAllowPersistFileLoad())
    {
        // Handle downloading error from UrlMon
        if (FAILED(hr) && spBindStatusCallback)
        {
            HRESULT bindResult = spBindStatusCallback->GetBindResult();
            if(FAILED(bindResult))
            {
                hr = bindResult;
                wchar_t *pErrMsg = TryDecodeDownloadError(hr);
                if(pErrMsg)
                {
                    // The easiest thing to do is call MessageBox() here. But the browser still has the
                    // foreground window status, so the message box will show minimized or behind.
                    // The solution is to fully activate the DocObject so that it can show the error page.
                    m_pHostShim->SetErrorMessageToDisplay(pErrMsg);
                    LocalFree(pErrMsg);
                    hr = S_OK; // Don't fail. We are activating, after all.
                }
            }
        }

        if(SUCCEEDED(hr))
        {
            // Note: Execute() triggers Watson on any failure.
            m_pHostShim->Execute();
        }
        else
        {
            HANDLE_ACTIVATION_FAULT(hr);
        }
    }

    if (pwszURL != NULL)
    {
        CoTaskMemFree(pwszURL);
    }

    EventWriteWpfHostUm_IPersistMonikerLoadEnd(hr);

    return hr;
}

//******************************************************************************
//
// CPersistMoniker::IsDirty()
//
//******************************************************************************

STDMETHODIMP CPersistMoniker::IsDirty()
{
    return S_FALSE;
}
