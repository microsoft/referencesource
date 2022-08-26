//------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Defines the interface to the deployment manifest
//
// History:
//      2005/05/09 - Microsoft     Created
//      2007/09/20   Microsoft     Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------
#pragma once

class CDeploymentManifest
{
public:
    CDeploymentManifest(__in LPCWSTR pswzDeploymentUri, __in LPCWSTR pswzPath);
    ~CDeploymentManifest() {}

    HRESULT Read();

private:
    HRESULT Parse();

public:
    STRING_PROP(Uri);
    STRING_PROP(CleanUri);
    STRING_PROP(Path);
    STRING_PROP(ApplicationIdentity);
    STRING_PROP(ProcessorArchitecture);
    STRING_PROP(ApplicationManifestCodebase);
    STRING_PROP(DeploymentProvider);

private:
    CString     m_strUri;
    CString     m_strCleanUri;
    CString     m_strPath;
    CString     m_strApplicationIdentity;
    CString     m_strProcessorArchitecture;
    CString     m_strApplicationManifestCodebase;
    CString     m_strDeploymentProvider;
};
