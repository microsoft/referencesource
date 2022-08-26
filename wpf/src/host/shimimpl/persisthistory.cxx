//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Defines the PersistHistory class of PresentationHost.
//
//  History:
//     2002/06/19-murrayw
//          Created
//     2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#include "Precompiled.hxx"
#include "PersistHistory.hxx"
#include "HostShim.hxx"
#include "Version.hxx"

// This macro will keep the types in sync between the LoadHistoryHelper and SaveHistoryHelper methods.
#define STARTUP_URI_LENGTH_TYPE ULONG

/******************************************************************************
 CPersistHistory::CPersistHistory()
******************************************************************************/
CPersistHistory::CPersistHistory(CHostShim *pHostShim)
{
    m_pHostShim = pHostShim;
}

/******************************************************************************
 CPersistHistory::~CPersistHistory()
******************************************************************************/
CPersistHistory::~CPersistHistory() 
{
}

/******************************************************************************
 CPersistHistory::QueryInterface()
******************************************************************************/
STDMETHODIMP CPersistHistory::QueryInterface(REFIID riid, LPVOID *ppReturn)
{
    return m_pHostShim->QueryInterface(riid, ppReturn);
}

/******************************************************************************
 CPersistHistory::AddRef()
******************************************************************************/
STDMETHODIMP_(ULONG) CPersistHistory::AddRef()
{
    return m_pHostShim->AddRef();
}

/******************************************************************************
  CPersistHistory::Release()
******************************************************************************/
STDMETHODIMP_(ULONG) CPersistHistory::Release()
{
    return m_pHostShim->Release();
}

/******************************************************************************
 CPersistHistory::GetClassID()
******************************************************************************/
STDMETHODIMP CPersistHistory::GetClassID(LPCLSID pClassID)
{
    return m_pHostShim->GetClassID(pClassID);
}

/******************************************************************************
 CPersistHistory::SetPositionCookie
******************************************************************************/
STDMETHODIMP CPersistHistory::SetPositionCookie(DWORD /*dwPositionCookie*/)
{
    return S_OK;
}

/******************************************************************************
 CPersistHistory::GetPositionCookie
******************************************************************************/
STDMETHODIMP CPersistHistory::GetPositionCookie(__out DWORD* pdwPositionCookie)
{
    HRESULT hr = S_OK;

    CK_PARG(pdwPositionCookie);

    *pdwPositionCookie = 0;

Cleanup:

    return hr;
}

/**************************************************************************
   CPersistHistory::SaveHistory(IStream *pStream)
**************************************************************************/
STDMETHODIMP CPersistHistory::SaveHistory(__in IStream* pSaveStream)
{
    HRESULT hr = S_OK;

    CKHR(m_pHostShim->RequestedVersion()->WriteToStream(pSaveStream));
    CKHR(WriteToStream(pSaveStream, (DWORD) m_pHostShim->GetMimeType()));
    CKHR(m_pHostShim->StartupUri()->WriteToStream(pSaveStream));
    CKHR(m_pHostShim->StartupLocation()->WriteToStream(pSaveStream));
    CKHR(m_pHostShim->ApplicationIdentity()->WriteToStream(pSaveStream));

    if(!m_pHostShim->GetVersion())
    {
        CKHR(E_UNEXPECTED);
    }
    CKHR(m_pHostShim->GetVersion()->SaveToHistory(pSaveStream));

Cleanup:
    return hr;
}

/**************************************************************************
   CPersistHistory::LoadHistory(IStream *pStream, IBindCtx *pbc)
**************************************************************************/
STDMETHODIMP CPersistHistory::LoadHistory(__in IStream* pLoadStream, IBindCtx* pBindCtx)
{
    HRESULT hr = S_OK;

    // When doing navigations inside of an app, history is saved without going through
    // CPersistHistory::SaveHistory, so the information that puts in the stream isn't there.
    // This check is to be sure that we are being activated by IPersistHistory.

    if (!m_pHostShim->GetVersion())
    {
        CKHR(m_pHostShim->RequestedVersion()->ReadFromStream(pLoadStream));

        DWORD mimeType;
        CKHR(ReadFromStream(pLoadStream, &mimeType));
        m_pHostShim->SetMimeType((MimeType)mimeType);

        CKHR(m_pHostShim->StartupUri()->ReadFromStream(pLoadStream));
        CKHR(m_pHostShim->StartupLocation()->ReadFromStream(pLoadStream));
        CKHR(m_pHostShim->ApplicationIdentity()->ReadFromStream(pLoadStream));

        pLoadStream->AddRef();
        m_pHostShim->SetHistoryStream(pLoadStream);

        m_pHostShim->Execute(); //[Watson on error]
    }

    CKHR(m_pHostShim->GetVersion()->LoadFromHistory(pLoadStream, pBindCtx));

Cleanup:
    return hr;
}

STDMETHODIMP CPersistHistory::ReadFromStream(IStream* pInputStream, DWORD* pdwData)
{
    HRESULT hr = S_OK;

    CHECK_POINTER(pdwData);

    ULONG bytesRead = 0;
    CKHR(pInputStream->Read((void*)pdwData, sizeof(*pdwData), &bytesRead));

Cleanup:
    return hr;
}

STDMETHODIMP CPersistHistory::WriteToStream(IStream* pOutputStream, DWORD dwData)
{
    HRESULT hr = S_OK;

    ULONG bytesWritten = 0;
    CKHR(pOutputStream->Write((void*)&dwData, sizeof(dwData), &bytesWritten));

Cleanup:
    return hr;
}
