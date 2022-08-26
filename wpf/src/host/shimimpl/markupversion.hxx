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

#include "..\shared\StringMap.hxx"

class CMarkupVersion : ISAXContentHandler
{
public:
    CMarkupVersion(__in LPCWSTR pswzLocalMarkupPath);
    ~CMarkupVersion() {}

    HRESULT Read();

public:
    STRING_PROP(LocalMarkupPath);

    // Return the map of namespace URLs to version number strings.
    CStringMap<CString*>& GetNamespaceVersion() { return m_mapNamespaceVersion; }
    // Return the map of prefixes to namespace URLs. This is read from the markup file.
    CStringMap<CString*>& GetPrefixNamespaces() { return m_mapPrefixNamespace; }
    // Return the map of ignorable namespace URLs to version number strings. This is read from the markup file.
    CStringMap<CString*>& GetIgnorableNamespaces() { return m_mapIgnorableNamespaceVersion; }

private:
    CString m_strLocalMarkupPath;
    long m_refCount;

    CStringMap<DWORD> m_mapPrefixVersion;
    CStringMap<CString*> m_mapNamespaceVersion;
    CStringMap<CString*> m_mapPrefixNamespace;
    CStringMap<CString*> m_mapIgnorableNamespaceVersion;

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
    
    /// <remarks>
    /// Per documentation for ISAXContentHandler::startPrefixMapping, 
    /// <paramref name="cchPrefix" /> represents the length of the prefix 
    /// string <parameref name="pwchPrefix" />, and this value will be -1 
    /// if the prefix string is already a zero-terminated string.
    /// </remarks>
    IFACEMETHODIMP startPrefixMapping(
        __in const wchar_t* pwchPrefix,             // The prefix being mapped
        __in int cchPrefix,                         // Length of the prefix string, or -1 (if zero terminated)
        __in_ecount(cchUri) const wchar_t* pwchUri, // The namespace URI to which the prefix is mapped
        __in int cchUri);                           // Length of the namespace URI string
    
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
};
