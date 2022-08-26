//------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Implements the interface to the deployment manifest
//
// History:
//      2005/05/09 - Microsoft     Created
//      2007/09/20   Microsoft     Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#include "PreCompiled.hxx"
#include "DeploymentManifest.hxx"

CDeploymentManifest::CDeploymentManifest(__in LPCWSTR pswzUri, __in LPCWSTR pswzPath)
{
    SetPath(pswzPath);
    SetUri(pswzUri);
}

HRESULT CDeploymentManifest::Read()
{
    HRESULT hr = S_OK;

    CKHR(GetDeploymentDataFromManifest(
        GetUri(), 
        GetPath(), 
        m_strApplicationIdentity, 
        m_strProcessorArchitecture, 
        m_strApplicationManifestCodebase,
        m_strDeploymentProvider));

Cleanup:
    return hr;
}

