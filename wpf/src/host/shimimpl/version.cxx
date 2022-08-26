//------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Implements the connection to PHDLL (PresentationHostDLL.dll v3 or PresentationHost_vX.dll for v4+).
//
// History:
//      2005/05/09 - Microsoft
//          Created
//      2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#include "PreCompiled.hxx"
#include "Version.hxx"
#include "ActivationContext.hxx"


CVersion::CVersion(__in_ecount(1) LPCWSTR pswzVersion):
    m_hInstance(0), m_bIsValid(false), m_bIsAttached(false),
    m_dwMajor(0), m_dwMinor(0), m_dwBuild(0),
    m_pfnActivate(0), m_pfnDeactivate(0), m_pfnForwardTranslateAccelerator(0),
    m_pfnSaveToHistory(0), m_pfnLoadFromHistory(0)
{
    if (pswzVersion)
    {
        SetValue(pswzVersion);
        ParseVersion(pswzVersion);
    }
}

int CVersion::CompareTo(__in_ecount(1) CVersion* pOther)
{
    int nResult = this->m_dwMajor - pOther->m_dwMajor;
    
    if (nResult == 0)
    {
        nResult = this->m_dwMinor - pOther->m_dwMinor;
    }

    if (nResult == 0)
    {
        nResult = this->m_dwBuild - pOther->m_dwBuild;
    }

    return nResult;
}

HRESULT CVersion::Attach()
{
    EventWriteWpfHostUm_VersionAttach();

    if(m_bIsValid && GetLibraryPath() != NULL)
    {
        m_hInstance = LoadLibrary(GetLibraryPath());

        if(m_hInstance != NULL)
        {
            m_pfnActivate = (ActivatePfn) GetProcAddress(m_hInstance, "Activate");
            m_pfnDeactivate = (DeactivatePfn) GetProcAddress(m_hInstance, "Deactivate");
            m_pfnForwardTranslateAccelerator = (ForwardTranslateAcceleratorPfn) GetProcAddress(m_hInstance, "ForwardTranslateAccelerator");
            m_pfnSaveToHistory = (SaveToHistoryPfn) GetProcAddress(m_hInstance, "SaveToHistory");
            m_pfnLoadFromHistory = (LoadFromHistoryPfn) GetProcAddress(m_hInstance, "LoadFromHistory");

            m_bIsAttached = 
                m_pfnActivate && 
                m_pfnDeactivate &&
                m_pfnForwardTranslateAccelerator && 
                m_pfnSaveToHistory && 
                m_pfnLoadFromHistory;

            // Dev10.582711 - CRT initialization for PresentationCore v3 fails when hosted by PresentationHost v4.
            // This is an involved issue, with a history going back to v3.5 SP1. 
            // See \\ddindex2\sources2\orc----p\wpf\src\core\crtinit.cpp for the initial issue and hacky 
            // workaround and the above bug for the unfortunate flaw that cropped up in v4.
            // The essence of the patch here is that we apply a Fusion activation context for loading the VC 8 CRT
            // based on the manifest embedded in PHDLL v3, which is the same as what PresentationCore.dll has
            // (as long as they are the same build flavor--this was a caveat about the original workaround too).
            // This activation context needs to be active only when PresentationCore (v3) is doing its 
            // initialization and binding to msvcr80.dll, but it doesn't hurt to keep it active for the whole
            // lifetime of the process. In some sense this gives v3 a more compatible runtime environment 
            // because PH v3 used to have that manifest reference to the VC 8 CRT.
            if(m_dwMajor == 3)
            {
                ActivationContext actctx;
                if(!actctx.Create(GetLibraryPath()) || !actctx.Activate(false))
                {
                    ASSERT(false);
                }
            }
        }
    }

    return m_bIsAttached ? S_OK : E_FAIL;
}

void CVersion::ParseVersion(__in_ecount(1) LPCWSTR pswzVersion)
{
    DWORD dwNumber[3] = {0,0,0};
    // nIndex is used in a loop bounded by a constant (3). Since it is safely bounded we can declare it as __bound to avoid PREFast warnings
    __bound UINT nIndex = 0;

    m_bIsValid = TRUE;
    
    BOOL bGotOpenQuote = FALSE;

    WCHAR* pwcNext = (WCHAR*)pswzVersion;

    // Initial quote is allowed and ignored
    if (*pwcNext == L'\"')
    {
        ++pwcNext;
        bGotOpenQuote = TRUE;
    }

    // Intial "v" is allowed, and ignored
    if (*pwcNext == 'v' || *pwcNext == 'V')
    {
        ++pwcNext;
    }

    while (*pwcNext && nIndex < 3)
    {
        if (*pwcNext == '.')
        {
            ++nIndex;
        }
        else if (iswdigit(*pwcNext))
        {
            dwNumber[nIndex] *= 10;
            dwNumber[nIndex] += *pwcNext - '0';
        }
        else
        {
            // Close quote (if it is the last character) is allowed and ignored
            if (bGotOpenQuote && *pwcNext == L'\"')
            {
                ++pwcNext;
                if (*pwcNext)
                {
                    m_bIsValid = FALSE;
                    break;
                }
            }
            else
            {
                m_bIsValid = FALSE;
                break;
            }
        }
        ++pwcNext;
    }

    if (m_bIsValid)
    {
        m_dwMajor = dwNumber[0];
        m_dwMinor = dwNumber[1];
        m_dwBuild = dwNumber[2];
    }
    else
    {
        m_dwMajor = 0;
        m_dwMinor = 0;
        m_dwBuild = 0;
    }
}

void CVersion::Activate(__in_ecount(1) const ActivateParameters* pParameters, __deref_out_ecount(1) LPUNKNOWN* ppInner)
{
    EventWriteWpfHostUm_VersionActivateStart();

    m_pfnActivate(pParameters, ppInner); //[supposed to invoke Watson on failure]

    EventWriteWpfHostUm_VersionActivateEnd();
}

void CVersion::Deactivate()
{
    m_pfnDeactivate();
}

HRESULT CVersion::ForwardTranslateAccelerator(__in_ecount(1) MSG* pMsg)
{
    return m_pfnForwardTranslateAccelerator(pMsg, VARIANT_FALSE/*the app wasn't given a chance*/);
}

HRESULT CVersion::SaveToHistory(__in_ecount(1) IStream* pHistoryStream)
{
    return m_pfnSaveToHistory(pHistoryStream);
}

HRESULT CVersion::LoadFromHistory(__in_ecount(1) IStream* pHistoryStream, __in_ecount(1) IBindCtx* pBindCtx)
{
    return m_pfnLoadFromHistory(pHistoryStream, pBindCtx);
}
