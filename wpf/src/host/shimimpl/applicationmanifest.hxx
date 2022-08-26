//------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Defines the interface to the application manifest
//
// History:
//      2005/05/09 - Microsoft     Created
//      2007/09/20   Microsoft     Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------
#pragma once

class CApplicationManifest : ISAXContentHandler, IBindStatusCallback
{
public:
    CApplicationManifest(__in LPCWSTR pswzDeploymentUri, __in LPCWSTR pswzCodebase);
    ~CApplicationManifest() {}

    HRESULT Read();

public:
    STRING_PROP(DeploymentManifestUri);
    STRING_PROP(Codebase);
    STRING_PROP(Uri);
    STRING_PROP(RequestedVersion);
    STRING_PROP(RequestedClrVersion);

private:
    long m_refCount;

public:
    // IUnknown
    IFACEMETHODIMP QueryInterface(__in REFIID riid, __out void* *ppv);
    IFACEMETHODIMP_(ULONG) AddRef();
    IFACEMETHODIMP_(ULONG) Release();

private:
    // ISAXContentHandler
    IFACEMETHODIMP putDocumentLocator(__in ISAXLocator* /*pLocator*/) {return S_OK; }
    IFACEMETHODIMP startDocument() {return S_OK; }
    IFACEMETHODIMP endDocument() {return S_OK; }
    
    IFACEMETHODIMP startPrefixMapping(
        __in const wchar_t*   /*pwchPrefix*/,
        __in int              /*cchPrefix*/,
        __in const wchar_t*   /*pwchUri*/,
        __in int              /*cchUri*/) {return S_OK; }
    
    IFACEMETHODIMP endPrefixMapping( 
        __in const wchar_t*  /*pwchPrefix*/,
        __in int             /*cchPrefix*/) {return S_OK; }    
    
    /// <remarks> Per documentation for ISAXContentHandler::startElement, 
    /// <paramref name="pwchLocalName" /> string might not be zero terminated. 
    /// </remarks>
    IFACEMETHODIMP startElement(
        __in_ecount(cchNamespaceUri) const wchar_t *pwchNamespaceUri,
        __in int cchNamespaceUri,
        __in_ecount(cchLocalName) const wchar_t *pwchLocalName,
        __in int cchLocalName,
        __in_ecount(cchQName) const wchar_t *pwchQName,
        __in int cchQName,
        __in ISAXAttributes *pAttributes);

    IFACEMETHODIMP endElement( 
        __in const wchar_t* /*pwchNamespaceUri*/,
        __in int /*cchNamespaceUri*/,
        __in const wchar_t* /*pwchLocalName*/,
        __in int /*cchLocalName*/,
        __in const wchar_t* /*pwchQName*/,
        __in int /*cchQName*/) {return S_OK; }

    IFACEMETHODIMP characters( 
        __in const WCHAR* /*pwchChars*/,
        __in int /*cchChars*/) {return S_OK; }

    IFACEMETHODIMP ignorableWhitespace( 
        __in const wchar_t* /*pwchChars*/,
        __in int /*cchChars*/) {return S_OK; }
    
    IFACEMETHODIMP processingInstruction( 
        __in const wchar_t* /*pwchTarget*/,
        __in int /*cchTarget*/,
        __in const wchar_t* /*pwchData*/,
        __in int /*cchData*/) {return S_OK; }
    
    IFACEMETHODIMP skippedEntity( 
        __in const wchar_t* /*pwchName*/,
        __in int /*cchName*/) {return S_OK; }

 public: // IBindStatusCallback implemented virtual functions

    IFACEMETHODIMP OnStartBinding(DWORD, __in IBinding*) {return S_OK;}
    IFACEMETHODIMP GetPriority(__in LONG*) {return S_OK;}
    IFACEMETHODIMP OnLowResource(DWORD) {return S_OK;}
    IFACEMETHODIMP OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, __in LPCWSTR szStatusText);
    IFACEMETHODIMP OnStopBinding(HRESULT, __in LPCWSTR ) {return S_OK;}
    IFACEMETHODIMP GetBindInfo(__in DWORD*, __inout BINDINFO*);
    IFACEMETHODIMP OnDataAvailable(DWORD, DWORD, __in FORMATETC*, __in STGMEDIUM*) {return S_OK;}
    IFACEMETHODIMP OnObjectAvailable(__in REFIID, __in IUnknown*) {return S_OK;}

private:
    CString     m_strDeploymentManifestUri;
    CString     m_strCodebase;
    CString     m_strUri;
    CString     m_strRequestedVersion;
    CString     m_strRequestedClrVersion;

    DWORD       m_dwDeploymentManifestZone;

    IInternetSecurityManager* m_pInternetSecurityManager;
};
