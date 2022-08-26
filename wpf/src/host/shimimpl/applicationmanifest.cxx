//------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Implements the interface to the application manifest
//
// History:
//      2005/05/09 - Microsoft     Created
//      2007/09/20   Microsoft     Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#include "PreCompiled.hxx"
#include "ApplicationManifest.hxx"

#define KEY_ASSEMBLY L"WindowsBase"
#define REQUESTED_CLR L"Microsoft.Windows.CommonLanguageRuntime"


CApplicationManifest::CApplicationManifest(__in LPCWSTR pswzUri, __in LPCWSTR pswzCodebase)
{
    SetDeploymentManifestUri(pswzUri);
    SetCodebase(pswzCodebase);
    m_refCount = 0;
    m_pInternetSecurityManager = NULL;
    m_dwDeploymentManifestZone = 0;
}

STDMETHODIMP CApplicationManifest::QueryInterface(const struct _GUID &riid,void ** ppvObject)
{
    *ppvObject = NULL;

    if (riid == IID_IUnknown)
    {
        *ppvObject = static_cast<ISAXContentHandler *>(this);
    }
    else if (riid == __uuidof(ISAXContentHandler))
    {
        *ppvObject = static_cast<ISAXContentHandler *>(this);
    }
    else if (riid == __uuidof(IBindStatusCallback))
    {
        *ppvObject = static_cast<IBindStatusCallback *>(this);
    }

    if (*ppvObject)
    {
        AddRef();
        return S_OK;
    }    
    else 
    {
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(DWORD) CApplicationManifest::AddRef()
{
    return InterlockedIncrement(&m_refCount);
}

STDMETHODIMP_(DWORD) CApplicationManifest::Release()
{
    InterlockedDecrement(&m_refCount);
    if (m_refCount == 0) 
    {
        delete this;
        return 0;
    }
    else 
    {
        return m_refCount;
    }
}

/// <remarks> 
/// Per documentation for ISAXContentHandler::startElement, 
/// <paramref name="pwchLocalName" /> string might not be zero terminated. 
/// The <paramref name="cchLocalName" /> parameter contains the length of 
/// the local name, which we will use to extract the exact number of useful
/// characters from <paramref name="pwchLocalName" />
/// </remarks>
IFACEMETHODIMP CApplicationManifest::startElement(
    __in_ecount(cchNamespaceUri) const wchar_t *pwchNamespaceUri,
    __in int cchNamespaceUri,
    __in_ecount(cchLocalName) const wchar_t *pwchLocalName, // The local name string might not be zero terminated
    __in int cchLocalName,
    __in_ecount(cchQName) const wchar_t *pwchQName,
    __in int cchQName,
    __in ISAXAttributes *pAttributes)
{
    HRESULT hr = S_OK;

    CString strLocalName;
    CString strName, strVersion;

    const wchar_t EMPTY[] = L"";
    const wchar_t NAME[] = L"name";
    const wchar_t VERSION[] = L"version";

    CKHR(strLocalName.SetValue(pwchLocalName, cchLocalName));
    if (strLocalName == L"assemblyIdentity")
    {
        CKHR(GetXmlAttributeValue(pAttributes, EMPTY, strlit_len(EMPTY), NAME, strlit_len(NAME), strName));
        if (strName == KEY_ASSEMBLY)
        {
            CKHR(GetXmlAttributeValue(pAttributes, EMPTY, strlit_len(EMPTY), VERSION, strlit_len(VERSION), strVersion));
            SetRequestedVersion(strVersion.GetValue());
        }
        else if (strName == REQUESTED_CLR)
        {
            CKHR(GetXmlAttributeValue(pAttributes, EMPTY, strlit_len(EMPTY), VERSION, strlit_len(VERSION), strVersion));
            SetRequestedClrVersion(strVersion.GetValue());
        }

        if (GetRequestedVersion() != NULL && GetRequestedClrVersion() != NULL)
        {
            CKHR(E_FAIL); // Stop parsing when we have both manifests
        }
    }

Cleanup:
    return hr;
}

HRESULT CApplicationManifest::Read()
{
    HRESULT hr = S_OK;
    ISAXXMLReader* pReader = NULL;

    // Resolve the deployment manifest URI with the application manifest codebase
    WCHAR wzAppManUri[INTERNET_MAX_URL_LENGTH + 1];
    DWORD dwLength = 0;
    CKHR(CoInternetCombineUrl(
        GetDeploymentManifestUri(),
        GetCodebase(),
        0,
        wzAppManUri,
        INTERNET_MAX_URL_LENGTH,
        &dwLength,
        0));

    CKHR(SetUri(wzAppManUri));

    EventWriteWpfHostUm_ReadingAppManifestStart(wzAppManUri);

    // cocreate the pointer to the COM interface we require
    // CLSID for internet security managed "7b8a2d94-0ac9-11d1-896c-00c04Fb6bfc4"
    // GUID for the interface we want to extract "79eac9ee-baf9-11ce-8c82-00aa004ba90b"
    CKHR(CoCreateInstance(CLSID_InternetSecurityManager, 
                        NULL, CLSCTX_INPROC_SERVER,
                        IID_IInternetSecurityManager, 
                        (void **)&m_pInternetSecurityManager));

    // Compare zones
    m_dwDeploymentManifestZone = 0;
    CKHR(m_pInternetSecurityManager->MapUrlToZone(GetDeploymentManifestUri(), &m_dwDeploymentManifestZone, 0));

    DWORD dwApplicationManifestZone = 0;
    CKHR(m_pInternetSecurityManager->MapUrlToZone(GetUri(), &dwApplicationManifestZone, 0));

    // Manifests must both come from the same zone. This is ClickOnce behavior, and we want
    // to be consistent with it.
    CKHR(m_dwDeploymentManifestZone == dwApplicationManifestZone ? S_OK : E_FAIL);

    // Download the file. The IBindStatusCallback::Progress method will get the new redirected URI,
    // and do zone matching there.
    WCHAR wzCacheFileName[MAX_PATH];
    // Note: URLDownloadToCacheFile's 4th param is number of characters, not number of bytes as 
    // stated in the SDK.
    CKHR(URLDownloadToCacheFile(NULL, GetUri(), wzCacheFileName, MAX_PATH, 0, this));

    CKHR(CoCreateInstance(__uuidof(SAXXMLReader60), NULL, CLSCTX_INPROC_SERVER, __uuidof(ISAXXMLReader), (void**)&pReader));
    CKHR(pReader->putContentHandler(this));
    hr = pReader->parseURL(wzCacheFileName);

    // If we stopped the parse because we found the version, hr will be E_FAIL and
    // the version will be set in the manifest.
    if (hr == E_FAIL && GetRequestedVersion() != NULL)
    {
        hr = S_OK;
    }

    EventWriteWpfHostUm_ReadingAppManifestEnd();

Cleanup:
    SAFERELEASE_POINTER(pReader);
    SAFERELEASE_POINTER(m_pInternetSecurityManager);

    return hr;
}

HRESULT CApplicationManifest::GetBindInfo(__in DWORD*, __inout BINDINFO* pBindInfo)
{
    //!!! Make sure to set BINDINFO_OPTIONS_ENABLE_UTF8 in all implementations of IBindStatusCallback.
    //!!! It is needed for correct encoding of non-ASCII characters in URL paths, per RFC 3986 & 3987.
    pBindInfo->dwOptions = BINDINFO_OPTIONS_ENABLE_UTF8;
    return S_OK;
}

HRESULT CApplicationManifest::OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    HRESULT hr = S_OK;

    if (ulStatusCode == BINDSTATUS_REDIRECTING)
    {
        SetUri(szStatusText);
        DWORD dwApplicationManifestZone = 0;
        CKHR(m_pInternetSecurityManager->MapUrlToZone(GetUri(), &dwApplicationManifestZone, 0));

        // If the zones mismatch, we will stop the download immediately.
        CKHR(m_dwDeploymentManifestZone == dwApplicationManifestZone ? S_OK : E_FAIL);
    }

Cleanup:
    return hr;
}

