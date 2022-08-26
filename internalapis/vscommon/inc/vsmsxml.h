#pragma once

#include <msxml2.h>

#define __USE_MSXML6_NAMESPACE__

#include <msxml6.h>

/////////////////////////////////////////////////////////////////////////////
//
// Re-usable XML cocreates with fallback capability
//

// Helper function to cocreate an array of clsids, in order, until one succeeds.
inline static HRESULT 
_CoCreateVersions
(
    /* in */ CLSID * pVersions, // null-terminated array of clsids to try
    /* in */ REFIID riid, // iid to get
    /* out */ LPVOID * ppv, // pointer to get
    /* out */ CLSID * pclsid // clsid that was actually retrieved
)
{
    HRESULT hr = S_OK;

    if (pclsid)
    {
        *pclsid = GUID_NULL;
    }

    for (int i = 0; !IsEqualCLSID(pVersions[i], GUID_NULL); i++)
    {
        hr = ::CoCreateInstance(pVersions[i], NULL, CLSCTX_INPROC_SERVER, riid, ppv);

        if (SUCCEEDED(hr))
        {
            if (pclsid)
            {
                *pclsid = pVersions[i];
            }
            break;
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
// Re-usable XML cocreates with fallback capability
//

// Helper function to cocreate an array of clsids, in order, until one succeeds.
inline static HRESULT 
_CoGetClassObjectVersions
(
    /* in */ CLSID * pVersions, // null-terminated array of clsids to try
    /* in */ REFIID riid, // iid to get
    /* out */ LPVOID * ppv, // pointer to get
    /* out */ CLSID * pclsid // clsid that was actually retrieved
)
{
    HRESULT hr = E_FAIL;

    if (pclsid)
    {
        *pclsid = GUID_NULL;
    }

    for (int i = 0; !IsEqualCLSID(pVersions[i], GUID_NULL); i++)
    {
        hr = ::CoGetClassObject(pVersions[i], CLSCTX_INPROC_SERVER, NULL, riid, ppv);

        if (SUCCEEDED(hr))
        {
            if (pclsid)
            {
                *pclsid = pVersions[i];
            }
            break;
        }
    }

    return hr;
}


//-----------------------------------------------------------------------------
// Public function to create an MSXML6 DOMDocument.  VS 2010 requires MSXML6
// so falling back to MSXML4 or MSXML3 is disabled.
//-----------------------------------------------------------------------------
inline HRESULT CoCreateDOMDocument(/* in */ REFIID riid, // iid to get
                            /* out */ LPVOID * ppv, // pointer to get
                            /* out */ CLSID * pclsid =NULL ) // clsid that was actually retrieved
{
    static CLSID aVersions[] = {MSXML6::CLSID_DOMDocument60, GUID_NULL};

    return _CoCreateVersions(aVersions, riid, ppv, pclsid);
}

//-----------------------------------------------------------------------------
// Public function To get to create an MSXML6 FreeThreadedDOMDocument.
//-----------------------------------------------------------------------------
inline HRESULT CoCreateFreeThreadedDOMDocument(/* in */ REFIID riid, // iid to get
                                        /* out */ LPVOID * ppv, // pointer to get
                                        /* out */ CLSID * pclsid =NULL ) // clsid that was actually retrieved
{
    static CLSID aVersions[] = {MSXML6::CLSID_FreeThreadedDOMDocument60, GUID_NULL};

    return _CoCreateVersions(aVersions, riid, ppv, pclsid);
}

//-----------------------------------------------------------------------------
// Public function to create an MSXML6 ClassFactory.
//-----------------------------------------------------------------------------
inline HRESULT CoGetDOMClassObject(/* in */ REFIID riid, // iid to get
                            /* out */ LPVOID * ppv, // pointer to get
                            /* out */ CLSID * pclsid =NULL ) // clsid that was actually retrieved
{
    static CLSID aVersions[] = {MSXML6::CLSID_DOMDocument60, GUID_NULL};

    return _CoGetClassObjectVersions(aVersions, riid, ppv, pclsid);
}

//-----------------------------------------------------------------------------
// Public function to create an MSXML6 ClassFactory.
//-----------------------------------------------------------------------------
inline HRESULT CoGetFreeThreadedDOMClassObject(/* in */ REFIID riid, // iid to get
                            /* out */ LPVOID * ppv, // pointer to get
                            /* out */ CLSID * pclsid =NULL ) // clsid that was actually retrieved
{
    static CLSID aVersions[] = {MSXML6::CLSID_FreeThreadedDOMDocument60, GUID_NULL};

    return _CoGetClassObjectVersions(aVersions, riid, ppv, pclsid);
}
