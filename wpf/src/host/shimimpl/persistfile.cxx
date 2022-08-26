//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Defines the PersistFile class of PresentationHost
//
//  History:
//     2002/06/12-murrayw
//          Created
//     2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#include "Precompiled.hxx"
#include "PersistFile.hxx"
#include "HostShim.hxx"
#include "ShimUtilities.hxx"

//******************************************************************************
//
// CPersistFile::CPersistFile()
//   
//******************************************************************************

CPersistFile::CPersistFile(__in CHostShim* pHostShim)
{
    m_pHostShim  = pHostShim;   
}

//******************************************************************************
//
// CPersistFile::~CPersistFile()
//   
//******************************************************************************

CPersistFile::~CPersistFile() 
{
}

//******************************************************************************
//
// CPersistFile::QueryInterface()
//
//******************************************************************************

STDMETHODIMP CPersistFile::QueryInterface(REFIID riid, __out LPVOID *ppReturn)
{
    return m_pHostShim->QueryInterface(riid, ppReturn);
}

//******************************************************************************
//
// CPersistFile::AddRef()
//  
//******************************************************************************

STDMETHODIMP_(ULONG) CPersistFile::AddRef()
{
    return m_pHostShim->AddRef();
}

//******************************************************************************
//
// CPersistFile::Release()
//   
//******************************************************************************

STDMETHODIMP_(ULONG) CPersistFile::Release()
{
    return m_pHostShim->Release();
}

//******************************************************************************
//
// CPersistFile::GetClassID()
//   
//******************************************************************************

STDMETHODIMP CPersistFile::GetClassID(__out LPCLSID pClassID)
{
    return m_pHostShim->GetClassID(pClassID);
}

//******************************************************************************
//
// CPersistFile::Save()
//   
//******************************************************************************

STDMETHODIMP CPersistFile::Save(__in_ecount(MAX_PATH+1) LPCOLESTR, BOOL)
{
    // File->Save is enabled/disabled by IOleCommandTarget->QueryStatus like any other menu item
    // However, once enabled it calls IPersistFile->Save rather than IOleCommandtarget->Exec.  
    // To make it work like the other menu items we simply turn around and call Exec ourselves.
    HRESULT hr = S_OK;
    IOleCommandTarget* pCmdTarget = NULL;
    CKHR(QueryInterface(IID_IOleCommandTarget, (VOID**)&pCmdTarget));
    CKHR(pCmdTarget->Exec(NULL, OLECMDID_SAVE, NULL, NULL, NULL));

Cleanup:
    ReleaseInterface(pCmdTarget);
    return hr;
}

//******************************************************************************
//
// CPersistFile::SaveCompleted()
//   
//******************************************************************************

STDMETHODIMP CPersistFile::SaveCompleted(LPCOLESTR pwszFile)
{
    return E_NOTIMPL;
}

//******************************************************************************
//
// CPersistFile::Load()
//
//    This method is called if file is local.
//   
//******************************************************************************

STDMETHODIMP CPersistFile::Load(__in_ecount(MAX_PATH + 1) LPCOLESTR pwszFile, DWORD dwMode)
{
    HRESULT     hr          = S_OK;
    CString strMimeType;
    MimeType    mimeType    = MimeType_Unknown;
    
    EventWriteWpfHostUm_IPersistFileLoad(pwszFile);

    m_pHostShim->SetActivationType(FileActivation);

    if (m_pHostShim->GetAllowPersistFileLoad() == FALSE && m_pHostShim->IsAllowPersistFileLoadSet())
    {
        return E_FAIL;
    }

    CK_PARG(pwszFile);
    if (!*pwszFile)
        return E_INVALIDARG;

    m_pHostShim->SetStartupUri(pwszFile);
    m_pHostShim->SetLocalDeploymentManifestPath(pwszFile);

    const DWORD SZMIMESIZE_MAX = 128;
    WCHAR wzBuffer[SZMIMESIZE_MAX + 1];
    DWORD dwCharCount = SZMIMESIZE_MAX;

    CKHR(AssocQueryString(
        0, // ASSOCF flags,
        ASSOCSTR_CONTENTTYPE, // ASSOCSTR str,
        PathFindExtension(pwszFile), // LPCTSTR pszAssoc,
        0, // LPCTSTR pszExtra,
        wzBuffer, // LPTSTR pszOut,
        &dwCharCount // DWORD *pcchOut
    ));

    mimeType = GetMimeTypeFromString(wzBuffer);

    m_pHostShim->SetMimeType(mimeType);

    // error out if we don't have an application or container
    if (!IsValidMimeType(mimeType))
    {
        CKHR(E_FAIL);
    }

    // Note: Execute() triggers Watson on any failure.
    m_pHostShim->Execute();
    hr = S_OK;

Cleanup:
    return hr;
}

//******************************************************************************
//
// CPersistFile::IsDirty()
//   
//******************************************************************************

STDMETHODIMP CPersistFile::IsDirty()
{
    return S_FALSE;
}

//******************************************************************************
//
// CPersistFile::GetCurFile()
//   
//******************************************************************************

STDMETHODIMP CPersistFile::GetCurFile(__out LPOLESTR *ppwszOut)
{
    return E_NOTIMPL;
}
