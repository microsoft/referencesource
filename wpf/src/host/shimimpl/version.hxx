//------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Defines the connection to PresentationHostDLL.dll
//
// History:
//      2005/05/09 - Microsoft
//          Created
//      2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#pragma once

class CVersion
{
public:
    CVersion(__in_ecount(1) LPCWSTR pswzVersion);
    ~CVersion() {}

public:
    BOOL IsValid() { return m_bIsValid; }
    BOOL IsAttached() { return m_bIsAttached; }
    HRESULT Attach();

    STRING_PROP(Value);
	DWORD Major() const { return m_dwMajor; }
	DWORD Minor() const { return m_dwMinor; }
    STRING_PROP(LibraryPath);

    int CompareTo(__in_ecount(1) CVersion* pOther);

    // Invokes Watson and terminates on failure
    void Activate(__in_ecount(1) const struct ActivateParameters* pParameters, __deref_out_ecount(1) LPUNKNOWN* ppInner);
    void Deactivate();
    HRESULT ForwardTranslateAccelerator(__in_ecount(1) MSG* pMsg);
    HRESULT SaveToHistory(__in_ecount(1) IStream* pHistoryStream);
    HRESULT LoadFromHistory(__in_ecount(1) IStream* pHistoryStream, __in_ecount(1) IBindCtx* pBindCtx);

private:
    void ParseVersion(__in_ecount(1) LPCWSTR pswzVersion);

private:
    typedef void (*ActivatePfn)(__in_ecount(1) const ActivateParameters* pParameters, __deref_out_ecount(1) LPUNKNOWN* ppInner);
    typedef void (*DeactivatePfn)();
    typedef HRESULT (*ForwardTranslateAcceleratorPfn)(__in_ecount(1) MSG* pMsg, VARIANT_BOOL appUnhandled);
    typedef HRESULT (*SaveToHistoryPfn)(__in_ecount(1) IStream* pHistoryStream);
    typedef HRESULT (*LoadFromHistoryPfn)(__in_ecount(1) IStream* pHistoryStream, __in_ecount(1) IBindCtx* pBindCtx);

    DWORD                           m_dwMajor;
    DWORD                           m_dwMinor;
    DWORD                           m_dwBuild;

    HINSTANCE                       m_hInstance;
    CString                         m_strValue;
    CString                         m_strLibraryPath;

    BOOL                            m_bIsValid;
    BOOL                            m_bIsAttached;
    ActivatePfn                     m_pfnActivate;
    DeactivatePfn                   m_pfnDeactivate;
    ForwardTranslateAcceleratorPfn  m_pfnForwardTranslateAccelerator;
    SaveToHistoryPfn                m_pfnSaveToHistory;
    LoadFromHistoryPfn              m_pfnLoadFromHistory;
};
