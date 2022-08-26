//------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Implements a factory for CVersion objects
//
// History:
//      2005/06/20 - Microsoft
//          Created
//      2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#include "PreCompiled.hxx"
#include "VersionFactory.hxx"
#include "Version.hxx"
#include "..\inc\registry.hxx"

CStringMap<CVersion*>* CVersionFactory::m_pVersions = NULL;
CVersion*   CVersionFactory::m_pLatestVersion = NULL;

int CVersionFactory::GetCount()
{
    EnsureVersionList();
    return m_pVersions ? m_pVersions->GetCount() : 0;
}

CVersion* CVersionFactory::GetLatestVersion()
{
    EnsureVersionList();
    return m_pLatestVersion;
}

HRESULT CVersionFactory::EnsureVersionList()
{
    HRESULT hr = S_OK;

    if (m_pVersions == NULL)
    {
        m_pVersions = new CStringMap<CVersion*>();

        CStringMap<CString*> versionMap;
        CKHR(GetVersionMapFromRegistry(HKEY_LOCAL_MACHINE, versionMap));

        CStringMap<CString*>::CStringMapNode* pVersionNode = versionMap.GetRoot();

        m_pLatestVersion = NULL;

        while (pVersionNode != NULL)
        {
            CVersion* pVersion = new CVersion(pVersionNode->GetKey());
            CK_ALLOC(pVersion);
            if (pVersion->IsValid())
            {
                pVersion->SetLibraryPath(pVersionNode->GetValue()->GetValue());
                m_pVersions->Add(pVersion->GetValue(), pVersion);

                if (m_pLatestVersion == NULL || (m_pLatestVersion->CompareTo(pVersion) < 0))
                {
                    m_pLatestVersion = pVersion;
                }
            }
            else
            {
                delete pVersion;
            }

            pVersionNode = pVersionNode->GetNext();
        }
    }

Cleanup:

    return hr;
}

CVersion* CVersionFactory::GetVersion(__in_ecount(1) LPCWSTR pwzRequestedVersion, BOOL bDefaultToLatest)
{
    HRESULT hr = EnsureVersionList();

    if (FAILED(hr))
    {
        return NULL;
    }

    // We make a new version object because it will canonicalize the version string
    CVersion version(pwzRequestedVersion);
    if (version.IsValid())
    {
        // We can't simply call m_pVersions->Find because the version.GetValue returns
        // whatever the string was to create the version and there may or may not be
        // irrelevant leading "v"s involved that we with to ignore.

        CStringMap<CVersion*>::CStringMapNode* pVersionNode = m_pVersions->GetRoot();

        while (pVersionNode)
        {
            if (version.CompareTo(pVersionNode->GetValue()) == 0)
            {
                return pVersionNode->GetValue();
            }

            pVersionNode = pVersionNode->GetNext();
        }
    }

    return bDefaultToLatest ? GetLatestVersion() : NULL;
}
