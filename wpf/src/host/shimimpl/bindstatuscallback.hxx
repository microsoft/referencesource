//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Defines the BindStatusCallback class of PresentationHost.
//
//  History
//      2002/06/22-murrayw
//          Created
//      2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#pragma once 

#define IO_BUFFER_SIZE  4096

class CBindStatusCallback : public IBindStatusCallback
{
public:

    CBindStatusCallback();
    ~CBindStatusCallback();

    LPCWSTR GetCacheFilename() const
    {
        return m_cacheFilename.Length() ? m_cacheFilename.GetValue() : 0; 
    }
    HRESULT GetBindResult() const { return m_hrBindResult; }

    //
    // IUnknown
    //

    STDMETHODIMP QueryInterface(REFIID riid, __out LPVOID* ppReturn);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    //
    // IBindStatusCallback methods
    //

    STDMETHODIMP OnStartBinding(DWORD dwReserved, __in IBinding *pib);
    STDMETHODIMP GetPriority(__out_opt LONG *pnPriority);
    STDMETHODIMP OnLowResource(DWORD dwReserved);
    STDMETHODIMP OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode,
                            LPCWSTR szStatusText);
    STDMETHODIMP OnStopBinding(HRESULT hresult, LPCWSTR /* szError */);
    STDMETHODIMP GetBindInfo(__out DWORD *grfBINDF,
                             __out BINDINFO *pbindinfo);
    STDMETHODIMP OnDataAvailable(DWORD grfBSCF,
                                 DWORD dwSize,
                                 __in FORMATETC *pformatetc,
                                 __in STGMEDIUM  *pstgmed);
    STDMETHODIMP OnObjectAvailable(REFIID riid, __in IUnknown *punk);

    BOOL IsXps() { return (m_Mime == MimeType_Xps); } 
    MimeType GetMimeType() { return m_Mime; }
    STRING_PROP(MimeTypeString);

    //
    // Data members
    //

    IStream*        m_pStream;
    IFillLockBytes* m_pflb;
    HANDLE          m_hMimeArrivedEvent;        // Event triggers when mime arrives
    HANDLE          m_hManifestAvailableEvent;  // Event triggers when the manifest is ready
    class DownloadInfo*   m_pDownloadInfo;
    CString         m_cacheFilename;

    //
    // Data members
    //
    MimeType    m_Mime;
    CString     m_strMimeTypeString;
    ULONG       m_nBytesTotal;
    ULONG       m_nBytesRead;
    BOOL        m_fTerminatedFLB;
    IBinding*   m_pib;
    int         m_nObjRefCount;
    HRESULT     m_hrBindResult;

private:
    void BindTerminated();
    void BindTerminated(HRESULT hr);
    void ResolveMimeTypeSecondChance(LPCWSTR pszLocalFile);
};
