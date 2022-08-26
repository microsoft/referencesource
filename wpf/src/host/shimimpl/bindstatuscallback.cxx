//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Implements the BindStatusCallback class of PresentationHost.
//
//  History
//      2002/06/22-murrayw
//          Created
//      2003/06/30-Microsoft
//          Ported ByteRangeDownloader to WCP
//      2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#include "Precompiled.hxx"
#include "BindStatusCallback.hxx"
#include "HostShim.hxx"
#include "ShimUtilities.hxx"

#if DEBUG
static WCHAR * rgBS_Notifications[] = 
{
    L"no data",
    L"BINDSTATUS_FINDINGRESOURCE",
    L"BINDSTATUS_CONNECTING",
    L"BINDSTATUS_REDIRECTING",
    L"BINDSTATUS_BEGINDOWNLOADDATA",
    L"BINDSTATUS_DOWNLOADINGDATA",
    L"BINDSTATUS_ENDDOWNLOADDATA",
    L"BINDSTATUS_BEGINDOWNLOADCOMPONENTS",
    L"BINDSTATUS_INSTALLINGCOMPONENTS",
    L"BINDSTATUS_ENDDOWNLOADCOMPONENTS",
    L"BINDSTATUS_USINGCACHEDCOPY",
    L"BINDSTATUS_SENDINGREQUEST",
    L"BINDSTATUS_CLASSIDAVAILABLE",
    L"BINDSTATUS_MIMETYPEAVAILABLE",
    L"BINDSTATUS_CACHEFILENAMEAVAILABLE",
    L"BINDSTATUS_BEGINSYNCOPERATION",
    L"BINDSTATUS_ENDSYNCOPERATION",
    L"BINDSTATUS_BEGINUPLOADDATA",
    L"BINDSTATUS_UPLOADINGDATA",
    L"BINDSTATUS_ENDUPLOADDATA",
    L"BINDSTATUS_PROTOCOLCLASSID",
    L"BINDSTATUS_ENCODING",
    L"BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE",
    L"BINDSTATUS_CLASSINSTALLLOCATION",
    L"BINDSTATUS_DECODING",
    L"BINDSTATUS_LOADINGMIMEHANDLER",
    L"BINDSTATUS_CONTENTDISPOSITIONATTACH",
    L"BINDSTATUS_FILTERREPORTMIMETYPE",
    L"BINDSTATUS_CLSIDCANINSTANTIATE",
    L"BINDSTATUS_IUNKNOWNAVAILABLE",
    L"BINDSTATUS_DIRECTBIND",
    L"BINDSTATUS_RAWMIMETYPE",
    L"BINDSTATUS_PROXYDETECTING",
    L"BINDSTATUS_ACCEPTRANGES",
    L"BINDSTATUS_COOKIE_SENT",
    L"BINDSTATUS_COMPACT_POLICY_RECEIVED",
    L"BINDSTATUS_COOKIE_SUPPRESSED",
    L"BINDSTATUS_COOKIE_STATE_UNKNOWN",
    L"BINDSTATUS_COOKIE_STATE_ACCEPT",
    L"BINDSTATUS_COOKIE_STATE_REJECT",
    L"BINDSTATUS_COOKIE_STATE_PROMPT",
    L"BINDSTATUS_COOKIE_STATE_LEASH",
    L"BINDSTATUS_COOKIE_STATE_DOWNGRADE",
    L"BINDSTATUS_POLICY_HREF",
    L"BINDSTATUS_P3P_HEADER",
    L"BINDSTATUS_SESSION_COOKIE_RECEIVED",
    L"BINDSTATUS_PERSISTENT_COOKIE_RECEIVED",
    L"BINDSTATUS_SESSION_COOKIES_ALLOWED",
    L""
};
#endif  // DEBUG

//******************************************************************************
//
//   CBindStatusCallback::CBindStatusCallback
//
//******************************************************************************

CBindStatusCallback::CBindStatusCallback()
{
    DPF(8, _T("CBindStatusCallback::CBindStatusCallback(%08X)"), this);
    
    m_nObjRefCount     = 0;
    m_pStream          = NULL;
    m_pflb             = NULL;
    m_nBytesTotal      = 0;
    m_nBytesRead       = 0;
    m_fTerminatedFLB   = FALSE;
    m_Mime             = MimeType_Unknown;
    m_pib              = NULL;
    m_hrBindResult     = S_FALSE;
    
    //
    m_hMimeArrivedEvent = ::CreateEvent(
                    NULL,               // Default security, not inherited.
                    TRUE,               // Manual reset event.
                    FALSE,              // Initially unsignaled.
                    NULL                // No name.
                    );

    m_hManifestAvailableEvent = ::CreateEvent(
                    NULL,               // Default security, not inherited.
                    TRUE,               // Manual reset event.
                    FALSE,              // Initially unsignaled.
                    NULL                // No name.
                    );

    ::ResetEvent(CHostShim::GetInstance()->GetDownloadCompletedEvent());

    m_pDownloadInfo = NULL;
}

CBindStatusCallback::~CBindStatusCallback()
{
    DPF(8, _T("CBindStatusCallback::~CBindStatusCallback(%08X)"), this);

    if (m_hMimeArrivedEvent != NULL)
    {
        ::CloseHandle(m_hMimeArrivedEvent);
        m_hMimeArrivedEvent = NULL;
    }

    if (m_hManifestAvailableEvent != NULL)
    {
        ::CloseHandle(m_hManifestAvailableEvent);
        m_hManifestAvailableEvent = NULL;
    }

    ReleaseInterface(m_pStream);
    ReleaseInterface(m_pflb);

    //DASSERT(m_pib == NULL);
    ReleaseInterface(m_pib);
}

//******************************************************************************
//
// CBindStatusCallback::QueryInterface(REFIID riid, 
//                                     LPVOID *ppReturn)
//   
//******************************************************************************

STDMETHODIMP CBindStatusCallback::QueryInterface(REFIID riid, 
                                                 __out LPVOID *ppReturn)
{
    HRESULT  hr = E_NOINTERFACE;

    DPF(8, _T("CBindStatusCallback::QueryInterface(%08X)"), this);

    *ppReturn = NULL;

    if (riid == IID_IUnknown)
    {
        *ppReturn = this;
    }
    else if (riid == IID_IBindStatusCallback)
    {
        *ppReturn = this;
    }

    if (*ppReturn)
    {
        ((LPUNKNOWN)*ppReturn)->AddRef();
        hr = S_OK;
    }

    return hr;
}

//******************************************************************************
//
// CBindStatusCallback::AddRef()
//
//******************************************************************************

STDMETHODIMP_(ULONG) CBindStatusCallback::AddRef()
{
    m_nObjRefCount++;
    DPF(10, _T("CBindStatusCallback::AddRef(%08X)- %d"), this, m_nObjRefCount);
    return m_nObjRefCount;
}

//******************************************************************************
//
// CBindStatusCallback::Release()
//
//******************************************************************************

STDMETHODIMP_(ULONG) CBindStatusCallback::Release()
{
    m_nObjRefCount--;

    DPF(10, _T("CBindStatusCallback::Release(%08X)- %d"), this, m_nObjRefCount);
    
    if (m_nObjRefCount == 0)
    {
        delete this;
        return 0;
    }

    return m_nObjRefCount;
}

//******************************************************************************
//
// CBindStatusCallback::OnStartBinding(DWORD dwReserved, IBinding* pib)
//
//******************************************************************************

HRESULT CBindStatusCallback::OnStartBinding(DWORD /* dwReserved */,
                                            __in IBinding* pib)
{
    DPF(8, _T("CBindStatusCallback::OnStartBinding(%08X)"), this);
    HRESULT hr = S_OK;

    CK_PARG(pib);

    m_pib = pib;
    m_pib->AddRef();
    
Cleanup:

    return hr;
}

//******************************************************************************
//
// CBindStatusCallback::GetPriority(LONG *pnPriority)
//
//******************************************************************************

HRESULT CBindStatusCallback::GetPriority(__out_opt LONG*)
{
    DPF(8, _T("CBindStatusCallback::GetPriority(%08X)"), this);

    // Applications that implement the IBindStatusCallback interface can return E_UNIMPL or
    // S_OK if they are not interested in receiving this notification.

    return E_NOTIMPL;
}

//******************************************************************************
//
// CBindStatusCallback::OnLowResource(DWORD dwReserved)
//
//******************************************************************************

HRESULT CBindStatusCallback::OnLowResource(DWORD) // dwReserved)
{
    DPF(8, _T("CBindStatusCallback::OnLowResource(%08X)"), this);
    return S_OK;
}

//******************************************************************************
//
// CBindStatusCallback::OnProgress(ULONG ulProgress, 
//                                 ULONG ulProgressMax,  
//                                 ULONG ulStatusCode,  
//                                 LPCWSTR szStatusText)
//
//******************************************************************************

HRESULT CBindStatusCallback::OnProgress(ULONG,   // ulProgress, 
                                        ULONG   ulProgressMax,  //ulProgressMax
                                        ULONG   ulCode,   // ulStatusCode,  
                                        LPCWSTR pszText)  // szStatusText)
{
    EventWriteWpfHostUm_BindProgress(ulCode, pszText);

    HRESULT hr = S_OK;

    switch (ulCode)
    {
        case BINDSTATUS_CACHEFILENAMEAVAILABLE:
            DPF(4, _T("CBindStatusCallback::OnProgress(%08X) - BINDSTATUS_CACHEFILENAMEAVAILABLE (%s) "), this, pszText);
            m_cacheFilename.SetValue(pszText);
            ResolveMimeTypeSecondChance(pszText);
            break;

        case BINDSTATUS_MIMETYPEAVAILABLE:
            DPF(4, _T("CBindStatusCallback::OnProgress(%08X) - BINDSTATUS_MIMETYPEAVAILABLE"), this);
            SetMimeTypeString(pszText);
            m_Mime = GetMimeTypeFromString(pszText);
            // DD Dev10 613556 - If a misconfigured web server returns an unknown MIME type,
            // we'll get a second chance to try to map it back to a known MIME type based on
            // the file extension, through ResolveMimeTypeSecondChance. This will set the
            // event regardless of the outcome.
            if (m_Mime != MimeType_Unknown)
            {
                ::SetEvent(m_hMimeArrivedEvent);
            }
            break;

        case BINDSTATUS_BEGINDOWNLOADDATA:
            DPF(4, _T("CBindStatusCallback::OnProgress(%08X) - BINDSTATUS_BEGINDOWNLOADDATA(%d)"), this, ulProgressMax);
            m_nBytesTotal = ulProgressMax;
            break;

        case BINDSTATUS_ENDDOWNLOADDATA:
            DPF(4, _T("CBindStatusCallback::OnProgress(%08X) - BINDSTATUS_ENDDOWNLOADDATA(%d)"), this, ulProgressMax);
            break;

#if DEBUG
        default:
            DPF(4, _T("CBindStatusCallback::OnProgress(%08X) - %s [%d]"), this, rgBS_Notifications[ulCode], ulCode);
            break;
#endif // DEBUG
    }

    return hr;
}

//******************************************************************************
//
// CBindStatusCallback::OnStopBinding(HRESULT hresult, LPCWSTR szError)
//
//******************************************************************************

HRESULT CBindStatusCallback::OnStopBinding(HRESULT hr, LPCWSTR szError)
{
    DPF(8, _T("CBindStatusCallback::OnStopBinding(%08X) - %08X"), this, hr);
    m_hrBindResult = hr; // Useful for Watson dump investigations.
    EventWriteWpfHostUm_OnStopBinding(hr);

    if (IsXps())
    {
        this->BindTerminated(hr);
    }

    if (m_pib)
    {
        m_pib->Release();
        m_pib = NULL;
    }

    // Either on error or on success signal the events so that PersistMoniker can continue.
    ::SetEvent(this->m_hMimeArrivedEvent);
    ::SetEvent(this->m_hManifestAvailableEvent);
    SignalAvalonStartupRelatedEvent(CHostShim::GetInstance()->GetDownloadCompletedEvent());

    return hr;
}

//******************************************************************************
//
// CBindStatusCallback::GetBindInfo(DWORD* grfBINDF, BINDINFO* pbindinfo)
// Abstract:
// BINDF_ASYNCHRONOUS - Value that indicates that the moniker should return immediately 
//                      from IMoniker::BindToStorage or IMoniker::BindToObject. The actual 
//                      result of the bind to an object or the bind to storage arrives 
//                      asynchronously. The client is notified through calls to its 
//                      IBindStatusCallback::OnDataAvailable or 
//                      IBindStatusCallback::OnObjectAvailable method. If the client does 
//                      not specify this flag, the bind operation will be synchronous, and 
//                      the client will not receive any data from the bind operation until 
//                      the IMoniker::BindToStorage or IMoniker::BindToObject call returns. 
//
// BINDF_ASYNCSTORAGE - Value that indicates the client application calling the 
//                      IMoniker::BindToStorage method prefers that the storage and stream 
//                      objects returned in IBindStatusCallback::OnDataAvailable return 
//                      E_PENDING when they reference data not yet available through their 
//                      read methods, rather than blocking until the data becomes available. 
//                      This flag applies only to BINDF_ASYNCHRONOUS operations. Note that 
//                      asynchronous stream objects return E_PENDING while data is still 
//                      downloading and return S_FALSE for the end of the file.
//
// BINDF_PULLDATA - Value that indicates the asynchronous moniker allows the client of 
//                  IMoniker::BindToStorage to drive the bind operation by pulling the data, 
//                  rather than having the moniker drive the operation by pushing the data 
//                  to the client. When this flag is specified, new data is only read/downloaded 
//                  after the client finishes downloading all data that is currently available. 
//                  This means data is only downloaded for the client after the client does an 
//                  IStream::Read operation that blocks or returns E_PENDING. When the client 
//                  specifies this flag, it must be sure to read all the data it can, even data 
//                  that is not necessarily available yet. When this flag is not specified, the 
//                  moniker continues downloading data and calls the client with 
//                  IBindStatusCallback::OnDataAvailable whenever new data is available. This 
//                  flag applies only to BINDF_ASYNCHRONOUS bind operations.
//
//******************************************************************************

HRESULT CBindStatusCallback::GetBindInfo(__out DWORD* grfBINDF,
                                         __out BINDINFO* pBindInfo)
{
    DPF(8, _T("CBindStatusCallback::GetBindInfo(%08X)"), this);
    HRESULT hr = S_OK;

    CK_PARG(grfBINDF);
    CK_PARG(pBindInfo);

    *grfBINDF = BINDF_ASYNCHRONOUS
              | BINDF_ASYNCSTORAGE 
#ifdef DOCUMENT_SHIM
              | BINDF_PULLDATA
#endif
                ;
    
    pBindInfo->cbSize       = sizeof(BINDINFO);
    pBindInfo->szExtraInfo  = NULL;
    pBindInfo->grfBindInfoF = 0;
    //!!! Make sure to set BINDINFO_OPTIONS_ENABLE_UTF8 in all implementations of IBindStatusCallback.
    //!!! It is needed for correct encoding of non-ASCII characters in URL paths, per RFC 3986 & 3987.
    pBindInfo->dwOptions    = BINDINFO_OPTIONS_ENABLE_UTF8;
    pBindInfo->dwBindVerb   = BINDVERB_GET;
    pBindInfo->szCustomVerb = NULL;
    
    ::SecureZeroMemory(&pBindInfo->stgmedData, sizeof(STGMEDIUM));

Cleanup:
    return hr;
}

//******************************************************************************
//
// CBindStatusCallback::OnDataAvailable(DWORD grfBSCF, 
//                                      DWORD dwSize, 
//                                      FORMATETC* pFormatetc, 
//                                      STGMEDIUM* pStgmed)
//
//******************************************************************************

HRESULT CBindStatusCallback::OnDataAvailable(DWORD grfBSCF, 
                                             DWORD dwSize, 
                                             __in FORMATETC* pFormatetc, 
                                             __in STGMEDIUM* pStgmed)
{
    DPF(8, _T("CBindStatusCallback::OnDataAvailable(%08X) - (%d)"), this, dwSize);
    HRESULT hr = S_OK;

    CK_PARG(pFormatetc);
    CK_PARG(pStgmed);

    // if this is not a container, jump out.
    if (!IsXps())
    {
        goto Cleanup;
    }

    //DASSERT(dwSize <= m_nBytesTotal);

    // on first notification, cache stream handle.
    if (grfBSCF & BSCF_FIRSTDATANOTIFICATION)
    {
        // make sure the type is an IStream
        if ( (pStgmed->tymed == TYMED_ISTREAM) &&
             (pStgmed->pstm != NULL) )
        {
            //DASSERT(m_pStream == NULL);
            m_pStream = pStgmed->pstm;
            m_pStream->AddRef();
        }
    }

    // only read if we have a valid cached stream and
    // we have not already read to the end.
    if ( (m_pStream != NULL) && (m_nBytesTotal > m_nBytesRead) )
    {
        ULONG cbRead;
        ULONG cbWritten;
        BYTE pbArray[IO_BUFFER_SIZE];
                
        // sit in loop reading stream until we are either at the end or 
        // we need to wait for more data
        for(;;)
        {
            cbWritten = 0;
            cbRead = 0;
            hr = m_pStream->Read((void *)pbArray, sizeof(pbArray), &cbRead);
            
            if (SUCCEEDED(hr) || (hr == E_PENDING) )
            {
                // if we read something, track the movement
                if (cbRead > 0)
                {
                    m_nBytesRead += cbRead;
                    m_pflb->FillAppend((void *)pbArray, cbRead, &cbWritten);
                
                    //DASSERT(cbRead == cbWritten);
                }
                
                // handle termination 
                if (m_nBytesTotal == m_nBytesRead)
                {           
                    this->BindTerminated();
                }

                // we have reached the end, usually denoted by S_FALSE
                if (hr == S_FALSE)
                {
                    DPF(4, _T("CBindStatusCallback::OnDataAvailable(%08X) - EOS (%d, %d)"), this, m_nBytesRead, dwSize);
                    hr = S_OK;
                    break;
                }

                // if E_PENDING is returned, get out.
                // we will get another call with more data.
                if (hr == E_PENDING)
                {
                    DPF(4, _T("CBindStatusCallback::OnDataAvailable(%08X) - Pos (%d, %d)"), this, m_nBytesRead, dwSize);
                    hr = S_OK;
                    break;
                }
            }
            // if we failed, do not attempt another read. Return the failed hr 
            else if (FAILED(hr))
            {
                break;
            }
        }
    }

    // on last notification, release stream
    if (grfBSCF & BSCF_LASTDATANOTIFICATION)
    {
        ReleaseInterface(m_pStream);
        m_pStream = NULL;
    }
    
Cleanup:
    return hr;
}

//******************************************************************************
//
// CBindStatusCallback::OnObjectAvailable(REFIID riid, IUnknown* punk)
//
//******************************************************************************

HRESULT CBindStatusCallback::OnObjectAvailable(REFIID riid, __in IUnknown* punk)
{
    DPF(8, _T("CBindStatusCallback::OnObjectAvailable(%08X)"), this);
    HRESULT hr = S_OK;

    CK_PARG(punk);

Cleanup:

    return hr;
}

//******************************************************************************
//
// CBindStatusCallback::BindTerminated()
//
//******************************************************************************

void CBindStatusCallback::BindTerminated()
{
    this->BindTerminated(E_FAIL);
}

//******************************************************************************
//
// CBindStatusCallback::BindTerminated(HRESULT hr)
//
//******************************************************************************

void CBindStatusCallback::BindTerminated(HRESULT hr)
{
    if (!m_fTerminatedFLB)
    {
        m_fTerminatedFLB = TRUE;
        m_pflb->Terminate(SUCCEEDED(hr) ? TRUE: FALSE);
    }
    // Signal the download completed event.
    SignalAvalonStartupRelatedEvent(CHostShim::GetInstance()->GetDownloadCompletedEvent());
}

//******************************************************************************
//
// CBindStatusCallback::ResolveMimeTypeSecondChance()
//
//******************************************************************************

void CBindStatusCallback::ResolveMimeTypeSecondChance(LPCWSTR pszLocalFile)
{
    // DD Dev10 613556 - Some misconfigured web servers return an unknown MIME type during
    // the BINDSTATUS_MIMETYPEAVAILABLE binding phase. This second chance logic uses UrlMon
    // functionality to map the file name of the local download location onto a CLSID which
    // we can use to map back to the MIME type. In practice, UrlMon uses the file extension
    // to perform this mapping in the absence of additional information.

    if (m_Mime == MimeType_Unknown)
    {
        CLSID clsId;

        HRESULT hr = GetClassFileOrMime(0, pszLocalFile, 0, 0, 0, 0, &clsId);
        if (SUCCEEDED(hr))
        {
            if (IsEqualCLSID(clsId, CLSID_DocObjXapp))
            {
                m_Mime = MimeType_Application;
            }
            else if (IsEqualCLSID(clsId, CLSID_DocObjXaml))
            {
                m_Mime = MimeType_Markup;
            }
        }

        ::SetEvent(m_hMimeArrivedEvent);
    }
}
