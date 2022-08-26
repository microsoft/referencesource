//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implements the file-level compilation logic.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

//****************************************************************************
//****************************************************************************
//****************************************************************************
//
// MetaDataFile implementation
//
//****************************************************************************
//****************************************************************************
//****************************************************************************

HRESULT MetaDataFile::CreateCore (
    _In_ CompilerProject *pProject,
    _In_z_ WCHAR *wszMetaFile,
    _Out_ MetaDataFile **ppFile )
{
    VerifyInPtr(pProject);
    VerifyInPtr(wszMetaFile);
    VerifyOutPtr(ppFile);

    HRESULT hr = NOERROR;
    Compiler *pCompiler = pProject->GetCompiler();
    CheckInMainThread();
    DebCheckNoBackgroundThreads(pCompiler);

    VBHeapPtr<MetaDataFile> spFile(new (zeromemory) MetaDataFile(pCompiler));
    IfFailGo(spFile->Init(FILE_MetaData, pProject));

    //
    // Store information about the file.
    //

    spFile->m_pstrFileName = spFile->m_pCompiler->AddString(wszMetaFile);
    *ppFile = spFile.Detach();
Error:
    return hr;
}

HRESULT MetaDataFile::Create
(
    _In_ CompilerProject *pProject,
    _In_z_ WCHAR *wszMetaFile,
    _Out_ MetaDataFile **ppfile
)
{
    HRESULT hr = NOERROR;
    VBHeapPtr<MetaDataFile> spFile;

    IfFailGo( CreateCore(pProject, wszMetaFile, &spFile) );

    // Create the IMetadata* interfaces for this metadata file.
    // Need to get the importer so early so that other file changes
    // do not affect this operation resulting in consistency
    // at different points of time.
    spFile->CreateImport();

#if IDE 
    IfFailGo(spFile->WatchFile());
    pProject->AddFileToTree(spFile);
#endif

    *ppfile = spFile.Detach();

Error:
    return hr;
}

HRESULT
MetaDataFile::Create(
    _In_ CompilerProject *pProject,
    _In_z_ WCHAR* wszMetaFile,
    _In_ IMetaDataImport* pImport,
    _Out_ MetaDataFile **ppFile)
{
    HRESULT hr = NOERROR;
    VBHeapPtr<MetaDataFile> spFile;
    CComPtr<IMetaDataImport2> spImport2;
    CComPtr<IMetaDataWinMDImport> spWinMDImport;

    IfFailGo( CreateCore(pProject, wszMetaFile, &spFile) );
    IfFailGo( ComUtil::QI(pImport, IID_IMetaDataImport2, &spImport2) );
    // Ignore if this interface is not found.
    ComUtil::QI(pImport, IID_IMetaDataWinMDImport, &spWinMDImport);

    pImport->AddRef();
    spFile->m_pmdImport = pImport;
    spFile->m_pmdImport2 = spImport2.Detach();
    spFile->m_pwinmdImport = spWinMDImport.Detach();

#if IDE 
    IfFailGo(spFile->WatchFile());
    pProject->AddFileToTree(spFile);
#endif

    *ppFile = spFile.Detach();
Error:
    return hr;
}

//============================================================================
// Destructor.
//============================================================================

MetaDataFile::~MetaDataFile()
{
    ReleaseImport();

#if IDE 
    // 

    // Remove the containing project from the implicit reverse reference list.
    ULONG cAssemblyRefs =
        this->m_AssemblyRefs.GetAssemblyRefCount();

    for(ULONG Index = 0; Index < cAssemblyRefs; Index++)
    {
        AssemblyRefInfo *pAssemblyRefInfo =
            this->m_AssemblyRefs.GetAssemblyRefInfo(Index);

        // There should only be one project per assembly ref usually.
        AssemblyRefReferencedProjectsIterator Iter(pAssemblyRefInfo);
        while (CompilerProject *pReferencedProject = Iter.Next())
        {
            pReferencedProject->RemoveImplicitReferenceOnMe(pAssemblyRefInfo);
        }
    }
#endif IDE
}

HRESULT MetaDataFile::LoadAssemblyRefs()
{
    HRESULT hr = NOERROR;

    if (m_fAssemblyRefsLoaded)
    {
        RRETURN(hr);
    }

    IMetaDataImport *pmdImport;
    CComPtr<IMetaDataAssemblyImport> srpmdAssemblyImport;

    HCORENUM hEnum = 0;
    ULONG cAssemblyRefs = 0;
    ULONG iRef = 0;

    bool fCanIndexIntoAssemblyRefList = true;

    m_AssemblyRefs.InitAssemblyRefList(0);
    m_AssemblyRefs.SetCanIndexIntoAssemblyRefList(false);

    // Get the IMetaDataImport for the metadata file.
    pmdImport = GetImport();
    IfNullGo(pmdImport);

    // Get the IAssemblyImport interface from the IMetaDataImport interface.
    IfFailGo(pmdImport->QueryInterface(IID_IMetaDataAssemblyImport, (void **)&srpmdAssemblyImport));

    // Enumerate over the assembly refs, build their identities and store them for the metadata file.

    // Open the enum.
    IfFailGo(srpmdAssemblyImport->EnumAssemblyRefs(&hEnum,
                                                   NULL,
                                                   0,
                                                   NULL));

    // Get the count.
    IfFailGo(pmdImport->CountEnum(hEnum, &cAssemblyRefs));

    if (cAssemblyRefs != 0)
    {
        m_AssemblyRefs.InitAssemblyRefList(cAssemblyRefs);
    }


    // Load each AssemblyRef's properties.

    for(iRef = 0; iRef < cAssemblyRefs; iRef++)
    {
        mdAssemblyRef tkAssemRef;
        ULONG cAssemblyRefsRead = 0;

        IfFailGo(srpmdAssemblyImport->EnumAssemblyRefs(&hEnum,
                                                       &tkAssemRef,
                                                       1,
                                                       &cAssemblyRefsRead));

        if (cAssemblyRefsRead == 0)
        {
            VSFAIL("Unexpected end when enumerating assembly refs!!!");
            break;
        }

        if (RidFromToken(tkAssemRef) != (iRef + 1))
        {
            // Compiler can handle this, but we still assert.
            VSFAIL("Out of order assembly ref unexpected!!!");
            fCanIndexIntoAssemblyRefList = false;
        }

        AssemblyRefInfo &pAssemblyRefInfo = m_AssemblyRefs.AddNew();

        IfFailGo(GetAssemblyRefProps(srpmdAssemblyImport,
                                     tkAssemRef,
                                     &pAssemblyRefInfo));

    }

    m_AssemblyRefs.SetCanIndexIntoAssemblyRefList(fCanIndexIntoAssemblyRefList);

Error:

    if (hEnum != 0)
    {
        srpmdAssemblyImport->CloseEnum(hEnum);
    }

    m_fAssemblyRefsLoaded = true;

    RRETURN(hr);
}

HRESULT MetaDataFile::GetAssemblyRefProps
(
    IMetaDataAssemblyImport *pAssemblyImport,
    mdAssemblyRef tkAssemRef,
    AssemblyRefInfo *pAssemblyRefInfo
)
{
    HRESULT hr = NOERROR;
    unsigned long cchAssemblyName;
    STRING *pstrAssemblyName;
    const void *pvAssemblyHash = NULL;
    ULONG cbAssemblyHash = 0;
    BYTE* pvPublicKey = NULL;
    ULONG cbPublicKey = 0;
    WCHAR wszCulture[_MAX_PATH + 1];
    ASSEMBLYMETADATA amd;
    DWORD dwFlags = 0;
    WCHAR wsz[256];
    WCHAR *wszLongAssemblyName = NULL;

    wszCulture[0] = L'\0';

    memset(&amd, 0, sizeof(amd));
    amd.szLocale = &wszCulture[0];
    amd.cbLocale = _MAX_PATH;

    // Get the identity for the assembly ref

    IfFailGo(pAssemblyImport->GetAssemblyRefProps(tkAssemRef,                     // [IN] The AssemblyRef for which to get the properties.
                                                  (const void **)&pvPublicKey,    // [OUT] Pointer to the Originator blob.
                                                  &cbPublicKey,                   // [OUT] Count of bytes in the Originator Blob.
                                                  wsz,                            // [OUT] Buffer to fill with name.
                                                  DIM(wsz),                       // [IN] Size of buffer in wide chars.
                                                  &cchAssemblyName,               // [OUT] Actual # of wide chars in name.
                                                  &amd,                           // [OUT] Assembly MetaData.
                                                  &pvAssemblyHash,                // [OUT] Hash blob.
                                                  &cbAssemblyHash,                // [OUT] Count of bytes in the hash blob.
                                                  &dwFlags));                     // [OUT] Flags.

    if (cchAssemblyName > DIM(wsz))
    {
        wszLongAssemblyName = VBAllocator::AllocateArray<WCHAR>(cchAssemblyName);

        IfFailGo(pAssemblyImport->GetAssemblyRefProps(tkAssemRef,                 // [IN] The AssemblyRef for which to get the properties.
                                                      NULL,                       // [OUT] Pointer to the Originator blob.
                                                      NULL,                       // [OUT] Count of bytes in the Originator Blob.
                                                      wszLongAssemblyName,        // [OUT] Buffer to fill with name.
                                                      cchAssemblyName,            // [IN] Size of buffer in wide chars.
                                                      NULL,                       // [OUT] Actual # of wide chars in name.
                                                      NULL,                       // [OUT] Assembly MetaData.
                                                      NULL,                       // [OUT] Hash blob.
                                                      NULL,                       // [OUT] Count of bytes in the hash blob.
                                                      &dwFlags));                 // [OUT] Flags.

        pstrAssemblyName = m_pCompiler->AddString(wszLongAssemblyName);
        delete[] wszLongAssemblyName;
    }
    else
    {
        pstrAssemblyName = m_pCompiler->AddString(wsz);
    }

    pAssemblyRefInfo->m_tkAssemblyRef = tkAssemRef;

    AssemblyIdentity *Identity = &pAssemblyRefInfo->m_Identity;
    Identity->InitAssemblyIdentityInfo(m_pCompiler, GetCompilerHost(), NULL);

    // Set the assembly identity.
    Identity->SetAssemblyIdentity(pstrAssemblyName,
                                  m_pCompiler->AddString(amd.szLocale),
                                  pvPublicKey,
                                  cbPublicKey,
                                  amd.usMajorVersion,
                                  amd.usMinorVersion,
                                  amd.usBuildNumber,
                                  amd.usRevisionNumber,
                                  dwFlags);

Error:

    if (wszLongAssemblyName != NULL)
    {
        delete[] wszLongAssemblyName;
    }

    RRETURN(hr);
}

void MetaDataFile::BindAssemblyRefs(ActionKindEnum action)
{
    LoadAssemblyRefs();

    ULONG cAssemblyRefs = m_AssemblyRefs.GetAssemblyRefCount();
    for(ULONG Index = 0; Index < cAssemblyRefs; Index++)
    {
        AssemblyRefInfo *pAssemblyRefInfo = m_AssemblyRefs.GetAssemblyRefInfo(Index);

        // VSWhidbey 555915: CS_Bound check to avoid rebinding a possibly in-use assembly ref.
        // This is not ideal. But given the non-optimal nature of decompilation for metadata
        // projects, this is possibly the safest in terms of functionaly and performance risk
        // for now.
        //
        if (m_pProject->GetCompState() < CS_Bound || ActionKind::Force == action)
        {
            BindAssemblyRef(pAssemblyRefInfo);
        }

#if IDE 
        // Add to the implicit references on me list.
        AssemblyRefReferencedProjectsIterator Iter(pAssemblyRefInfo);
        while (CompilerProject *pCurrent = Iter.Next())
        {
            // Note that some projects contain assemblyrefs to themselves. In these cases, to avoid extra complexity
            // to avoid decompiling mscorlib etc., such assemblyrefs are not added to the ImplicitReferenceOnMe list.
            if (pCurrent != m_pProject)
            {
                pCurrent->AddImplicitReferenceOnMe(pAssemblyRefInfo);
            }
        }
#endif IDE
    }
}

void MetaDataFile::BindAssemblyRef
(
    AssemblyRefInfo *pAssemblyRefInfo
)
{
    pAssemblyRefInfo->m_pReferencingProject = m_pProject;
    pAssemblyRefInfo->m_pReferencedProject = NULL;
    pAssemblyRefInfo->m_pAmbiguousReferencedProject = NULL;
    pAssemblyRefInfo->m_ErrorID = NOERROR;
    pAssemblyRefInfo->m_pstrErrorExtra = NULL;
    pAssemblyRefInfo->m_fCausesReferenceCycle = false;

#if IDE 
    pAssemblyRefInfo->m_fVenusPartialNameMatch = false;
#endif IDE

    if (pAssemblyRefInfo->m_pPossiblyAmbiguousReferencedVBProjects)
    {
        delete pAssemblyRefInfo->m_pPossiblyAmbiguousReferencedVBProjects;
        pAssemblyRefInfo->m_pPossiblyAmbiguousReferencedVBProjects = NULL;
    }

    STRING *pstrAssemblyName = pAssemblyRefInfo->m_Identity.GetAssemblyName();

    AssemblyComparisonResult CompareResults = ACR_NonEquivalent;

    ERRID OptionalErrorIDUsed = 0;
    StringBuffer OptionaErrorMessage;
    StringBuffer OptionaErrorMessageUsed;

    CompilerHost *pCompilerHost = GetCompilerHost();

    CompilerProject *pFullMatchMetadataProject = NULL;
    CompilerProject *pAmbiguousFullMatchMetadataProject = NULL;

    CompilerProject *pPartialMatchMetadataProject = NULL;
    CompilerProject *pAmbiguousPartialMatchMetadataProject = NULL;
    bool fPartialMatchVersionLower = false;

    CompilerProject *pVBProject = NULL;
    CompilerProject *pPossiblyAmbiguousVBProject = NULL;
    DynamicArray<CompilerProject *> *pPossiblyAmbiguousVBProjects = NULL;

    CompilerProject *pFullMatchReferencingProject = NULL;
    CompilerProject *pAmbiguousFullMatchReferencingProject = NULL;
    CompilerProject *pPartialMatchReferencingProject = NULL;
    CompilerProject *pAmbiguousPartialMatchReferencingProject = NULL;

    bool fFullMatchVenusProjectsInvolved = false;
    bool fPartialMatchVenusProjectsInvolved = false;

    bool fFullMatchWarnForAmbiguity = false;
    bool fPartialMatchWarnForAmbiguity = false;

#if IDE  
    bool fVenusAppCodeProjectsPresentInHost = false;
#endif IDE

    // Loop through all of the projects in the world looking for the right one.
    ProjectIterator Projects(GetCompilerHost());

    // Find the matching project.
    while (CompilerProject *pProject = Projects.Next())
    {
#if IDE  
        if (pProject->IsVenusAppCodeProject())
        {
            fVenusAppCodeProjectsPresentInHost = true;
        }
#endif IDE

        if (!pProject->IsInitialized() ||
            !StringPool::IsEqual(pProject->GetAssemblyName(), pstrAssemblyName))
        {
            continue;
        }

        ERRID OptionalErrorID = 0;

        // We always prefer metadata projects over VB projects.
        if (pProject->IsMetaData())
        {
#if IDE  
            // Dev10 #582765 Do not bind to phantom MetaData projects, projects that have no direct references to them.
            // Indirect references are not ref counted and do not guarantee that the project will be alive long enough.
            if ( pProject->m_daReferencesOnMe.Count() == 0 &&
                //Dev10 #680574 - don't skip mscorlib as we may not attempt to bind it later unless the VB Runtime is decompiled
                pProject != pProject->GetCompilerHost()->GetComPlusProject()) 
            {
                continue;
            }
#endif IDE

            bool NonEquivalentOnlyDueToLowerVersion = false;

            // Make sure it is usable.
            CompareResults = AssemblyIdentity::CompareAssemblies(pProject->GetAssemblyIdentity(),
                                                                    &pAssemblyRefInfo->m_Identity,
                                                                    &OptionalErrorID,
                                                                    &OptionaErrorMessage,
                                                                    &NonEquivalentOnlyDueToLowerVersion,
                                                                    m_pCompiler,
                                                                    pCompilerHost);

            if (CompareResults == ACR_EquivalentFullMatch || CompareResults == ACR_EquivalentFXUnified)
            {
                // Assembly is full match, go ahead and use it.
                if (!pFullMatchMetadataProject)
                {
                    pFullMatchMetadataProject = pProject;
                    // don't break here - go on so that ambiguity could be detected
                }
                else
                {
                    DisambiguateIdenticalReferences(
                        pProject,
                        &pFullMatchMetadataProject,
                        &pFullMatchReferencingProject,
                        &pAmbiguousFullMatchMetadataProject,
                        &pAmbiguousFullMatchReferencingProject,
                        &fFullMatchVenusProjectsInvolved,
                        &fFullMatchWarnForAmbiguity);
                }
            }

            if (!pFullMatchMetadataProject &&
                (CompareResults == ACR_EquivalentUnified || CompareResults == ACR_EquivalentWeakNamed))
            {
                // Assembly is a partial match, use it only if we can't find another assembly with a full match.
                if (!pPartialMatchMetadataProject)
                {
                    pPartialMatchMetadataProject = pProject;
                    fPartialMatchVersionLower = NonEquivalentOnlyDueToLowerVersion;
                }
                else
                {
                    // Is the current partial match better than the previous partial match ?

                    // 1. Higher version that AssemblyRef version wins over lower version than AssemblyRef version.
                    // 2. If both are Higher version than assemblyref version, then the lower of the two wins.
                    // 3. If both are Lower version than assemblyref version, then the higher of the two wins.

                    if (fPartialMatchVersionLower)
                    {
                        if (!NonEquivalentOnlyDueToLowerVersion)
                        {
                            pPartialMatchMetadataProject = pProject;
                            fPartialMatchVersionLower = false;
                            pAmbiguousPartialMatchMetadataProject = NULL;
                            pPartialMatchReferencingProject = NULL;
                            pAmbiguousPartialMatchReferencingProject = NULL;
                            fPartialMatchVenusProjectsInvolved = false;
                            fPartialMatchWarnForAmbiguity = false;
                        }
                        else
                        {
                            LONG VersionDifference =
                                pPartialMatchMetadataProject->GetAssemblyIdentity()->CompareAssemblyVersion(pProject->GetAssemblyIdentity());

                            if (VersionDifference < 0)
                            {
                                pPartialMatchMetadataProject = pProject;
                                pAmbiguousPartialMatchMetadataProject = NULL;
                                pPartialMatchReferencingProject = NULL;
                                pAmbiguousPartialMatchReferencingProject = NULL;
                                fPartialMatchVenusProjectsInvolved = false;
                                fPartialMatchWarnForAmbiguity = false;
                            }
                            else if (VersionDifference == 0)
                            {
                                DisambiguateIdenticalReferences(
                                    pProject,
                                    &pPartialMatchMetadataProject,
                                    &pPartialMatchReferencingProject,
                                    &pAmbiguousPartialMatchMetadataProject,
                                    &pAmbiguousPartialMatchReferencingProject,
                                    &fPartialMatchVenusProjectsInvolved,
                                    &fPartialMatchWarnForAmbiguity);
                            }
                        }
                    }
                    else
                    {
                        if (!NonEquivalentOnlyDueToLowerVersion)
                        {
                            LONG VersionDifference =
                                pPartialMatchMetadataProject->GetAssemblyIdentity()->CompareAssemblyVersion(pProject->GetAssemblyIdentity());

                            if (VersionDifference > 0)
                            {
                                pPartialMatchMetadataProject = pProject;
                                pAmbiguousPartialMatchMetadataProject = NULL;
                                pPartialMatchReferencingProject = NULL;
                                pAmbiguousPartialMatchReferencingProject = NULL;
                                fPartialMatchVenusProjectsInvolved = false;
                                fPartialMatchWarnForAmbiguity = false;
                            }
                            else if (VersionDifference == 0)
                            {
                                DisambiguateIdenticalReferences(
                                    pProject,
                                    &pPartialMatchMetadataProject,
                                    &pPartialMatchReferencingProject,
                                    &pAmbiguousPartialMatchMetadataProject,
                                    &pAmbiguousPartialMatchReferencingProject,
                                    &fPartialMatchVenusProjectsInvolved,
                                    &fPartialMatchWarnForAmbiguity);
                            }
                        }
                    }
                }
            }

            if (OptionalErrorID != 0)
            {
                OptionalErrorIDUsed = OptionalErrorID;
                OptionaErrorMessageUsed.Copy(&OptionaErrorMessage);
                OptionaErrorMessage.Clear();
            }
        }
#if IDE 
        else    // VB Project
        {
            // VB project is less preferred than a metadata project.
            if (!pFullMatchMetadataProject && !pPartialMatchMetadataProject &&
                CanBindToVBProject(
                pAssemblyRefInfo,
                pProject,
                m_pCompiler,
                pCompilerHost,
                true,       // Check only by assembly name (and public key if available)
                NULL,
                NULL))
            {
                if (!pVBProject)
                {
                    pVBProject = pProject;
                }
                else if (!pPossiblyAmbiguousVBProject)
                {
                    pPossiblyAmbiguousVBProject = pProject;
                }
                else
                {
                    if (!pPossiblyAmbiguousVBProjects)
                    {
                        pPossiblyAmbiguousVBProjects = new DynamicArray<CompilerProject *>;
                    }

                    pPossiblyAmbiguousVBProjects->AddElement(pProject);
                }
            }
        }
#endif IDE
    }


    CompilerProject *pReferencedProject = NULL;
    CompilerProject *pAmbiguousReferencedProject = NULL;
    CompilerProject *pReferencingProject = NULL;
    CompilerProject *pAmbiguousReferencingProject = NULL;
    bool fWarnForAmbiguity = false;

    if (pFullMatchMetadataProject)
    {
        pReferencedProject = pFullMatchMetadataProject;
        pAmbiguousReferencedProject = pAmbiguousFullMatchMetadataProject;
        fWarnForAmbiguity = (fFullMatchWarnForAmbiguity && !fFullMatchVenusProjectsInvolved);

        if (fWarnForAmbiguity)
        {
            pReferencingProject = pFullMatchReferencingProject;
            pAmbiguousReferencingProject = pAmbiguousFullMatchReferencingProject;
        }
    }
    else if (pPartialMatchMetadataProject)
    {
        pReferencedProject = pPartialMatchMetadataProject;
        pAmbiguousReferencedProject = pAmbiguousPartialMatchMetadataProject;
        fWarnForAmbiguity = (fPartialMatchWarnForAmbiguity && !fPartialMatchVenusProjectsInvolved);

        if (fWarnForAmbiguity)
        {
            pReferencingProject = pPartialMatchReferencingProject;
            pAmbiguousReferencingProject = pAmbiguousPartialMatchReferencingProject;
        }
    }
    else if (pVBProject)
    {
        pReferencedProject = pVBProject;
        pAmbiguousReferencedProject = pPossiblyAmbiguousVBProject;
    }

#if IDE
    // 




    if (!pReferencedProject && OptionalErrorIDUsed == 0 &&  // Referencing project not found
        fVenusAppCodeProjectsPresentInHost)                 // At least one venus app code project is present in the solution
    {
        pReferencedProject = GetPartialMatchVenusAppCodeProject(pAssemblyRefInfo);

        if (pReferencedProject)
        {
            pAssemblyRefInfo->m_fVenusPartialNameMatch = true;
        }
    }
#endif IDE

    if (!pReferencedProject)
    {
        if (OptionalErrorIDUsed != 0)
        {
            pAssemblyRefInfo->m_ErrorID = OptionalErrorIDUsed;
            pAssemblyRefInfo->m_pstrErrorExtra = m_pCompiler->AddString(OptionaErrorMessageUsed.GetString());
        }
        else
        {
            // No project?  Then we have no reference to this assembly.
            pAssemblyRefInfo->m_ErrorID = ERRID_UnreferencedAssembly3;
            pAssemblyRefInfo->m_pstrErrorExtra = pAssemblyRefInfo->m_Identity.GetAssemblyIdentityString();
        }
    }
    else
    {
        if (pAmbiguousReferencedProject &&
            pAmbiguousReferencedProject->IsMetaData())  // VB projects' ambiguity can only be detected later once the
                                                        // public key, etc. in assembly attributes are determined.
        {
#if IDE
            // Special case for multi-targeting scenario in which Full and Client framework assemblies
            // are both loaded into the same compiler host.
            if (fWarnForAmbiguity && pReferencingProject && pAmbiguousReferencedProject)
            {
                const CComBSTR bstrReferencingMoniker = pReferencingProject->GetTargetFrameworkMoniker();

                if (!CompareString32NoCase(pReferencedProject->GetTargetFrameworkMoniker(), bstrReferencingMoniker))
                {
                    // Target framework moniker matches - don't warn.
                    fWarnForAmbiguity = false;
                }
                else if (!CompareString32NoCase(pAmbiguousReferencedProject->GetTargetFrameworkMoniker(), bstrReferencingMoniker))
                {
                    // Target framework moniker matches - don't warn.
                    fWarnForAmbiguity = false;
                    TemplateUtil::Swap(&pReferencingProject, &pAmbiguousReferencingProject);
                    TemplateUtil::Swap(&pReferencedProject, &pAmbiguousReferencedProject);
                }
            }
#endif

            StringBuffer ErrorString;

            if (fWarnForAmbiguity)
            {
                if (pReferencingProject && pAmbiguousReferencingProject)
                {
                    // "Assembly '|1' requires a reference to assembly '|2', but a suitable reference could not be
                    // found due to ambiguity between '|3' that is referenced by project '|4' and '|5' that is referenced by
                    // project '|6'. Visual Basic compiler has defaulted to '|3'. If both assemblies are identical, try
                    // replacing these references so that both references are from the same location.

                    pAssemblyRefInfo->m_ErrorID = WRNID_ReferencedAssembliesAmbiguous6;

                    ResLoadStringRepl(
                        WRNID_ReferencedAssembliesAmbiguous6,
                        &ErrorString,
                        m_pProject->GetFileName(),
                        pAssemblyRefInfo->m_Identity.GetAssemblyIdentityString(),
                        pReferencedProject->GetFileName(),
                        GetErrorProjectName(pReferencingProject),
                        pAmbiguousReferencedProject->GetFileName(),
                        GetErrorProjectName(pAmbiguousReferencingProject));
                }
                else
                {
                    // "Assembly '|1' requires a reference to assembly '|2', but a suitable reference could not be
                    // found due to ambiguity between '|3' and '|4'."

                    pAssemblyRefInfo->m_ErrorID = WRNID_ReferencedAssembliesAmbiguous4;

                    ResLoadStringRepl(
                        WRNID_ReferencedAssembliesAmbiguous4,
                        &ErrorString,
                        m_pProject->GetFileName(),
                        pAssemblyRefInfo->m_Identity.GetAssemblyIdentityString(),
                        pReferencedProject->GetFileName(),
                        pAmbiguousReferencedProject->GetFileName());
                }

                pAssemblyRefInfo->m_pstrErrorExtra = m_pCompiler->AddString(ErrorString.GetString());
            }
        }

        pAssemblyRefInfo->m_pReferencedProject = pReferencedProject;
        pAssemblyRefInfo->m_pAmbiguousReferencedProject = pAmbiguousReferencedProject;

        if (!pReferencedProject->IsMetaData())
        {
            pAssemblyRefInfo->m_pPossiblyAmbiguousReferencedVBProjects = pPossiblyAmbiguousVBProjects;
            pPossiblyAmbiguousVBProjects = NULL;
        }
    }

    if (pPossiblyAmbiguousVBProjects)
    {
        delete pPossiblyAmbiguousVBProjects;
    }
}

#if IDE 
CompilerProject *MetaDataFile::GetPartialMatchVenusAppCodeProject
(
    AssemblyRefInfo *pAssemblyRefInfo
)
{
    // 














    const unsigned HASH_SUFFIX_LENGTH = 9;              // The ".########" in <* (including dot delimited name)>.########
    const WCHAR APP_CODE_PREFIX[] = L"App_Code";
    const unsigned APP_CODE_PREFIX_LEN = DIM(APP_CODE_PREFIX) - 1;
    const WCHAR APP_SUBCODE_PREFIX[] = L"App_SubCode_";
    const unsigned APP_SUBCODE_PREFIX_LEN = DIM(APP_SUBCODE_PREFIX) - 1;
    const unsigned MIN_ASSEMBLY_NAME_LEN = HASH_SUFFIX_LENGTH + min(APP_CODE_PREFIX_LEN, APP_SUBCODE_PREFIX_LEN);

    STRING *pstrAssemblyRefName = pAssemblyRefInfo->m_Identity.GetAssemblyName();

    if (!pstrAssemblyRefName)
    {
        return NULL;
    }

    size_t cchAssemblyRefNameLength = StringPool::StringLength(pstrAssemblyRefName);

    if (cchAssemblyRefNameLength < MIN_ASSEMBLY_NAME_LEN ||
        pstrAssemblyRefName[cchAssemblyRefNameLength - HASH_SUFFIX_LENGTH] != DOT_DELIMITER ||
        (CompareNoCaseN(pstrAssemblyRefName, APP_CODE_PREFIX, APP_CODE_PREFIX_LEN) != 0 &&
         CompareNoCaseN(pstrAssemblyRefName, APP_SUBCODE_PREFIX, APP_SUBCODE_PREFIX_LEN) != 0))
    {
        return NULL;
    }

    // Loop through all of the projects in the world looking for the right one.
    ProjectIterator ProjectsIter(GetCompilerHost());
    CompilerProject *pPossibleVenusProject;

    while (pPossibleVenusProject = ProjectsIter.Next())
    {
        if (!pPossibleVenusProject->IsInitialized() ||
            pPossibleVenusProject->IsMetaData() ||
            !pPossibleVenusProject->IsVenusAppCodeProject())
        {
            continue;
        }

        STRING *pstrProjectAssemblyName = pPossibleVenusProject->m_AssemblyIdentity.GetAssemblyName();

        if (!pstrProjectAssemblyName ||
            StringPool::StringLength(pstrProjectAssemblyName) != cchAssemblyRefNameLength)
        {
            continue;
        }

        // Compare assembly names till the last "."

        if (CompareNoCaseN(pstrProjectAssemblyName, pstrAssemblyRefName, cchAssemblyRefNameLength - HASH_SUFFIX_LENGTH + 1) == 0)
        {
            // Matching app_code project reference found.

            // Check to make sure that the app_code match is from the correct web
            // (multiple webs could be present in the same solution).
            //
            // Note that no ambiguity checking is done to disambiguate cases where
            // the user might have added the same DLL that references app_code to
            // multiple webs. This is discouraged by the venus team and is an
            // unsupported scenario.

            ReverseReferenceIterator ReferencingProjects(m_pProject);
            CompilerProject *pReferencingProject;

            while (pReferencingProject = ReferencingProjects.Next())
            {
                if (!pReferencingProject->IsInitialized() ||
                    pReferencingProject->IsMetaData() ||
                    !pReferencingProject->IsVenusProject())
                {
                    continue;
                }

                if (pReferencingProject->IsProjectReferenced(pPossibleVenusProject))
                {
                    // Avoid atleast basic cycles here that were not possible previously
                    // with the unique naming, but which can start happening more often now
                    // with the partial match. We are doing this to minimize chances for
                    // issues that can occur because of this in other parts of the compiler.
                    //
                    if (pPossibleVenusProject->IsProjectReferenced(m_pProject))
                    {
                        return NULL;
                    }

                    return pPossibleVenusProject;
                }
            }
        }
    }

    return NULL;
}
#endif IDE

void
DisambiguateReferencesByTimestamp
(
    CompilerProject **ppMatch,
    CompilerProject **ppMatchReferencingProject,
    CompilerProject **ppAmbiguousMatch,
    CompilerProject **ppAmbiguousMatchReferencingProject
)
{
    int CompareResult =
        AssemblyIdentity::CompareTimestamps((*ppMatch)->GetAssemblyIdentity(), (*ppAmbiguousMatch)->GetAssemblyIdentity());

    if (CompareResult < 0)
    {
        // If the ambiguous match has a later timestamp, then choose it as the match.

        CompilerProject *pTemp = *ppMatch;
        *ppMatch = *ppAmbiguousMatch;
        *ppAmbiguousMatch = pTemp;

        pTemp = *ppMatchReferencingProject;
        *ppMatchReferencingProject = *ppAmbiguousMatchReferencingProject;
        *ppAmbiguousMatchReferencingProject = pTemp;
    }

    // 


}

// 
void
MetaDataFile::DisambiguateIdenticalReferences
(
    CompilerProject *pNewFound,
    CompilerProject **ppMatch,
    CompilerProject **ppMatchReferencingProject,
    CompilerProject **ppAmbiguousMatch,
    CompilerProject **ppAmbiguousMatchReferencingProject,
    bool *pfVenusProjectsInvolved,
    bool *pfWarn
)
{
    AssertIfNull(pNewFound);
    AssertIfNull(ppMatch);
    AssertIfNull(*ppMatch);
    AssertIfNull(ppMatchReferencingProject);
    AssertIfNull(ppAmbiguousMatch);
    AssertIfNull(ppAmbiguousMatchReferencingProject);
    AssertIfNull(pfWarn);

    AssertIfFalse(pNewFound->IsMetaData());
    AssertIfFalse((*ppMatch)->IsMetaData());
    AssertIfFalse(!*ppAmbiguousMatch || (*ppAmbiguousMatch)->IsMetaData());

#if DEBUG

    AssemblyComparisonResult CompareResults =
                                AssemblyIdentity::CompareAssemblies(
                                                    pNewFound->GetAssemblyIdentity(),
                                                    (*ppMatch)->GetAssemblyIdentity(),
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    m_pCompiler,
                                                    GetCompilerHost());

    AssertIfFalse(CompareResults == ACR_EquivalentFullMatch || CompareResults == ACR_EquivalentFXUnified);
#endif DEBUG

#if IDE 

    CompilerProject *pCurrentProject = m_pProject;
    CompilerProject *pSelectedProject = NULL;
    CompilerProject *pReferencingVenusProject = NULL;
    CompilerProject *pReferencingProject = NULL;
    CompilerProject *pPrevAmbiguousMatch = *ppAmbiguousMatch;

    if (pCurrentProject->IsReferencedByReferencingVBProjects(*ppMatch, &pReferencingProject, &pReferencingVenusProject))
    {
        pSelectedProject = *ppMatch;
        *ppMatchReferencingProject = pReferencingProject;
        *pfVenusProjectsInvolved |= !!pReferencingVenusProject;
    }
    else if (!*ppMatchReferencingProject)
    {
        *ppMatchReferencingProject = (*ppMatch)->GetFirstReferencingProject();
        *pfVenusProjectsInvolved |= !!(*ppMatch)->GetFirstReferencingVenusProject();
    }

    pReferencingProject = NULL;
    pReferencingVenusProject = NULL;
    if (pCurrentProject->IsReferencedByReferencingVBProjects(pNewFound, &pReferencingProject, &pReferencingVenusProject))
    {
        if (!pSelectedProject)
        {
             pSelectedProject = pNewFound;

            *ppMatch = pNewFound;
            *ppAmbiguousMatch = NULL;

            *ppMatchReferencingProject = pReferencingProject;
            *ppAmbiguousMatchReferencingProject = NULL;

            *pfVenusProjectsInvolved = !!pReferencingVenusProject;
            *pfWarn = false;
        }
        else
        {
            // Disambiguate based on timestamp

            *ppAmbiguousMatch = pNewFound;
            *ppAmbiguousMatchReferencingProject = pReferencingProject;
            *pfVenusProjectsInvolved |= !!pReferencingVenusProject;

            DisambiguateReferencesByTimestamp(
                ppMatch,
                ppMatchReferencingProject,
                ppAmbiguousMatch,
                ppAmbiguousMatchReferencingProject);
        }
    }
    else if (!pSelectedProject)
    {
        // Disambiguate based on timestamp

        *ppAmbiguousMatch = pNewFound;
        *ppAmbiguousMatchReferencingProject = pNewFound->GetFirstReferencingProject();
        *pfVenusProjectsInvolved |= !!pNewFound->GetFirstReferencingVenusProject();

        DisambiguateReferencesByTimestamp(
            ppMatch,
            ppMatchReferencingProject,
            ppAmbiguousMatch,
            ppAmbiguousMatchReferencingProject);
    }

    if (*ppAmbiguousMatch && *ppAmbiguousMatch != pPrevAmbiguousMatch)
    {
        // Determine whether the warning should be shown.

        // If any of the references are from venus projects, then don't warn because the
        // venus project system automatically adds the references from temp directories
        // and the user cannot control the location from which the reference is added.
        //

        // If both the references are COM tlbimp wrappers, then don't warn because the
        // project system creates different copies of these wrappers for each project
        // in the solution.
        //
        if (!(*ppMatch)->GetAssemblyIdentity()->IsImportedFromCOMTypeLib() ||
            !(*ppAmbiguousMatch)->GetAssemblyIdentity()->IsImportedFromCOMTypeLib())
        {
            // The MVID for an assembly is different for each build. So if the MVID is
            // identical, assume that the assemblies are identical and don't warn.
            //
            if (AssemblyIdentity::CompareMVIDs((*ppMatch)->GetAssemblyIdentity(), (*ppAmbiguousMatch)->GetAssemblyIdentity()) != 0)
            {
                *pfWarn = true;
            }
        }
    }

#else // else if !IDE

    if (!*ppMatchReferencingProject)
    {
        *ppMatchReferencingProject = (*ppMatch)->GetFirstReferencingProject();
    }

    *ppAmbiguousMatch = pNewFound;
    *ppAmbiguousMatchReferencingProject = pNewFound->GetFirstReferencingProject();

    DisambiguateReferencesByTimestamp(
        ppMatch,
        ppMatchReferencingProject,
        ppAmbiguousMatch,
        ppAmbiguousMatchReferencingProject);

    *pfWarn = true;
#endif IDE
}

bool
MetaDataFile::CanBindToVBProject
(
    AssemblyRefInfo *pAssemblyRefInfo,
    CompilerProject *pProject,
    Compiler *pCompiler,
    CompilerHost *pCompilerHost,
    bool CheckOnlyByNameAndProjectLevelPKInfoIfAvailable,
    ERRID *pOptionalErrorID,
    StringBuffer *pOptionalErrorString
)
{
    VSASSERT(!pProject->IsMetaData(), "Source project expected!!!");

    bool Match = false;

    if (CheckOnlyByNameAndProjectLevelPKInfoIfAvailable)
    {
        if (pProject->GetProjectSettingForKeyFileName() ||
            pProject->GetProjectSettingForKeyContainerName())
        {
            Match = pAssemblyRefInfo->m_Identity.IsAssemblyWithSameNameAndPKToken(pProject->GetAssemblyIdentity());
        }
        else
        {
            Match = StringPool::IsEqual(pAssemblyRefInfo->m_Identity.GetAssemblyName(), pProject->GetAssemblyIdentity()->GetAssemblyName());
        }
    }
    else if (!pAssemblyRefInfo->m_Identity.IsStrongNameAssembly())
    {
        // When matching againt VB projects, for non-strong named identities
        // we ignore version since the assemblies will anyway load and run
        // correctly. If we do not do this, then problems will result for the
        // "*" version cases.

        Match = StringPool::IsEqual(pAssemblyRefInfo->m_Identity.GetAssemblyName(), pProject->GetAssemblyIdentity()->GetAssemblyName());
    }
    else
    {
        // Make sure it is usable.
        AssemblyComparisonResult CompareResults =
            AssemblyIdentity::CompareAssemblies(
                                    pProject->GetAssemblyIdentity(),
                                    &pAssemblyRefInfo->m_Identity,
                                    pOptionalErrorID,
                                    pOptionalErrorString,
                                    NULL,
                                    pCompiler,
                                    pCompilerHost);

        VSASSERT(CompareResults != ACR_EquivalentFXUnified, "Unexpected assembly compare result for VB project!!!");

        if (CompareResults == ACR_EquivalentFullMatch ||
            CompareResults == ACR_EquivalentFXUnified ||
            CompareResults == ACR_EquivalentUnified ||
            CompareResults == ACR_EquivalentWeakNamed)
        {
            Match = true;
        }
    }

    return Match;
}

//****************************************************************************
// Compilation methods
//
// The following methods should only be called from the master project
// compilation routines.  They should never be called directly.
//****************************************************************************

//============================================================================
// Ensure that all of the symbols in this file are created.  This only
// returns a catastrophic-type of error.
//============================================================================

bool MetaDataFile::_StepToBuiltSymbols()
{
    DebCheckInCompileThread(m_pCompiler);

    ErrorTable       errorTable(m_pCompiler, m_pProject, NULL);
    NorlsAllocator   nra(NORLSLOC);

    VSASSERT(m_cs == CS_NoState, "Bad state.");

    //
    // Tell the user what's up.
    //

#if !IDE
    if (m_pCompiler->OutputLevel() == OUTPUT_Verbose)
    {
        StringBuffer sbuf;

        IfFailThrow(ResLoadStringRepl(STRID_Loading,
                                      &sbuf,
                                      m_pstrFileName));

        GetCompilerHost()->Printf(L"%s\n", sbuf.GetString());
    }
#endif !IDE
    //
    // If the file isn't loaded yet, try loading it.
    //

    MetaImport::ImportTypes(m_pCompiler, this, &m_SymbolList, &errorTable);

    // Microsoft: This is an inefficient way of doing this.  It reuses the mechanism Akash used to build up the namespace symbols
    // but has a ton of overhead that is no longer needed since the horizontal linking mechanism is now different.  Rewrite this
    // when time allows to use a more efficient mechanism for encapsulating everything in namespace symbols.

    BuildNamespacesForMetaDataFile();

    // Report the warnings resulting from the ambiguity among indirect references.

    ULONG cAssemblyRefs = m_AssemblyRefs.GetAssemblyRefCount();
    for(ULONG Index = 0; Index < cAssemblyRefs; Index++)
    {
        AssemblyRefInfo *pInfo = m_AssemblyRefs.GetAssemblyRefInfo(Index);

        if (pInfo->m_ErrorID == WRNID_ReferencedAssembliesAmbiguous6 ||
            pInfo->m_ErrorID == WRNID_ReferencedAssembliesAmbiguous4)
        {
            errorTable.CreateErrorWithPartiallySubstitutedString(
                pInfo->m_ErrorID,
                pInfo->m_pstrErrorExtra,
                NULL);
        }
    }

    bool fContainsExtensions = false;

    if (this->GetUnnamedNamespace())
    {
        mdAssembly mdAssemblyToken = this->GetProject()->GetAssemblyToken();

        if (mdAssemblyToken != mdAssemblyNil)
        {
            BCSYM_Namespace *UnnamedNamespace = this->GetUnnamedNamespace();

            Attribute::----AllAttributesOnToken(
                mdAssemblyToken,
                GetImport(),
                SymbolStorage(),
                m_pCompiler,
                GetProject(),
                this,
                UnnamedNamespace);

            bool fIsCLSCompliant;
            UnnamedNamespace->GetPWellKnownAttrVals()->GetCLSCompliantData(&fIsCLSCompliant);

            fContainsExtensions = UnnamedNamespace->GetPWellKnownAttrVals()->GetExtensionData();

            // 





            this->GetProject()->SetProjectClaimsCLSCompliance(fIsCLSCompliant);
        }
    }

    SetContainsExtensionMethods(fContainsExtensions);

    //
    // Nothing after this point should be able to fail.  It is now safe
    // to move this module to the next state.
    //

    // Merge the new errors into the error table.
    CompilerIdeLock spLock(GetObjectCriticalSection());
    m_ErrorTable.Merge(&errorTable, CS_BuiltSymbols);
    spLock.Unlock();

    //
    // Dump any errors.
    //

    m_pCompiler->AddErrorsAndWarnings(
        m_ErrorTable.GetErrorCount(),
        m_ErrorTable.GetWarningCount());

    return false;
}

//============================================================================
// NOOOP
//============================================================================

bool MetaDataFile::_StepToBoundSymbols()
{
    DebCheckInCompileThread(m_pCompiler);

    return false;
}

//============================================================================
// NOOOP
//============================================================================
void MetaDataFile::CompleteStepToBoundSymbols()
{
    DebCheckInCompileThread(m_pCompiler);
}

//============================================================================
// NOOP
//============================================================================

bool MetaDataFile::_StepToCheckCLSCompliance()
{
    DebCheckInCompileThread(m_pCompiler);

    return false;
}

//============================================================================
// NOOP
//============================================================================

bool MetaDataFile::_StepToEmitTypes()
{
    DebCheckInCompileThread(m_pCompiler);

    return false;
}

//============================================================================
// NOOP
//============================================================================

bool MetaDataFile::_StepToEmitTypeMembers()
{
    DebCheckInCompileThread(m_pCompiler);

    return false;
}

//============================================================================
// Moves to compiled state...a noop for this file type.
//============================================================================

bool MetaDataFile::_StepToEmitMethodBodies()
{
    DebCheckInCompileThread(m_pCompiler);

    //
    // Not much to do here, each individual symbol is loaded on demand.
    //

    return false;
}

/*****************************************************************************
 ;CreateImport

 Create an IMetaDataImport2 for this file
*****************************************************************************/
HRESULT // Success or catastrophic failure code
MetaDataFile::CreateImport()
{
    HRESULT hr = NOERROR;
    IMetaDataDispenserEx *pmdDisp = NULL;

    //
    // Get COM+ if the file doesn't already have it.  We do this here instead
    // of in DoImportTypes to reduce the optimization effect of the
    // try block.
    //

    VSASSERT(!m_pmdImport, "Why are we creating?");

    //
    // Open the metadata.
    //

    hr = GetCompilerHost()->OpenScope(&pmdDisp, m_pstrFileName, ofReadOnly, IID_IMetaDataImport, (IUnknown **)&m_pmdImport);

    // If we can't open it, give a reasonable error.
    if (FAILED(hr))
    {
        // Report the error.
        m_ErrorTable.CreateErrorWithError(ERRID_BadRefLib1, NULL, hr, m_pstrFileName);

        // We're done.
        RELEASE(pmdDisp);
        RELEASE(m_pmdImport);

        return NOERROR;
    }

    // Attempt to get the IMetaDataImport2 interface
    HRESULT hr2 = m_pmdImport->QueryInterface(IID_IMetaDataImport2, (void **) &m_pmdImport2);

    // Attempt to get the IMetadataWinMDImport interface
    HRESULT hr3 = m_pmdImport->QueryInterface(IID_IMetaDataWinMDImport, (void **) &m_pwinmdImport);

    RELEASE(pmdDisp);

    return hr;
}

/*****************************************************************************
 ;BuildNamespacesForMetaDataFile

A MetaData file has a list of symbols, but those symbols aren't encapsulated in namespaces.
Each symbol has a string field that identifies which namespace it belongs in and this builds those
namespaces, parents them according to the namespace hierarchy that develops, and puts the
metadata symbols into those namespaces.  In the end, the symbol table looks like what you
would expect a file that went through Declared to look like.  When we get it to that point,
then we can merge the namespaces from this file against other metadata or Basic files

Microsoft: this whole process should probably be rewritten.  I essentially refactored the way
Akash had done this in order to reuse as much code as possible since we are so close to the
beta.  This mechanism is oriented around the old philosophy that the hiearchy builder was
oriented towards and so is slightly sub-obtimal for the new world.
*****************************************************************************/
HRESULT // Success or catastrophic failure code
MetaDataFile::BuildNamespacesForMetaDataFile()
{
    HRESULT hr=NOERROR;
    CompilerProject *Project = this->GetProject();
    SymbolList  AliasSymbolList, *FileSymbolList = this->GetNamespaceLevelSymbolList(); // the list of symbols defined in the metadata file
    BCSYM_Namespace *NamespaceForSymbol, *LastNamespace=NULL; // Holds the list of symbols that belong to a particular namespace
    STRING *NamespaceOfPreviousSymbol = NULL;
    BCSYM_NamedRoot *NextFileSymbol;
    VBHeapPtr<BCSYM_Namespace*> NamespaceList;
    
    unsigned NamespaceListCount = 0;
    unsigned cNamespaces = FileSymbolList->GetCount();

    if (cNamespaces > 0)
    {
        size_t bufferSize;
        if ( !VBMath::TryMultiply(cNamespaces, sizeof(BCSYM_Namespace), &bufferSize) )
        {
            VSFAIL("Too many file symbols.");
            return E_FAIL;
        }
        
        NamespaceList.AllocateBytes(bufferSize);
    }

    // Go through and figure out which symbols belong to which namespace
    for (BCSYM_NamedRoot *CurrentFileSymbol = FileSymbolList->GetFirst();
         CurrentFileSymbol && NamespaceListCount < cNamespaces;
         CurrentFileSymbol = NextFileSymbol)
    {
        NextFileSymbol = CurrentFileSymbol->GetNextInSymbolList();

        if ( CurrentFileSymbol->GetBindingSpace() != BINDSPACE_IgnoreSymbol )
        {
            // Figure out which namespace this symbol belongs to
            STRING *NamespaceOfCurrentSymbol = STRING_CONST( m_pCompiler, EmptyString );

            // If the symbol is a type, it will be a container and the name of its
            // containing namespace is stored in it.

            if ( CurrentFileSymbol->DigThroughAlias()->IsContainer())
            {
                NamespaceOfCurrentSymbol = CurrentFileSymbol->DigThroughAlias()->PContainer()->GetNameSpace();
            }
            else if ( CurrentFileSymbol->IsBad())
            {
                NamespaceOfCurrentSymbol = CurrentFileSymbol->GetNameSpace();
            }
            else
            {
                // The symbol is contained within something, and so the name of its
                // containing namespace is stored in its container.

                // It makes no sense for this condition to arise.
                VSFAIL("Namespace construction from metadata found a symbol with a container.");
            }

            // If this namespace is the same as the last one we were working with, use it so we don't have to hash to find its symbol list
            if ( LastNamespace && StringPool::IsEqual( NamespaceOfCurrentSymbol, NamespaceOfPreviousSymbol ))
            {
                NamespaceForSymbol = LastNamespace;
            }
            else
            {
                // This fix was ported from VSQFE1087. We need to call GetUnnamedNamespace() if this is the Unnamed namespace.
                // By doing that, we make sure that m_pUnnamedNamespace is set to this namespace. This fix was done by Microsoft.
                if (StringPool::StringLength(NamespaceOfCurrentSymbol) == 0)
                {
                    NamespaceForSymbol = Project->GetCompiler()->GetUnnamedNamespace(this);
                }
                else
                {
                    NamespaceForSymbol = Project->GetCompiler()->GetNamespace( NamespaceOfCurrentSymbol, this, 0 );
                }

                LastNamespace = NamespaceForSymbol;
                NamespaceOfPreviousSymbol = NamespaceOfCurrentSymbol;

                NamespaceList[NamespaceListCount++] = NamespaceForSymbol;
            }

            Symbols::AddSymbolToHash( NamespaceForSymbol->GetHash(), CurrentFileSymbol, true, false, true);

            // Dev10 #832844: Mark namespace if it contains Public or Friend types.
            switch (CurrentFileSymbol->GetAccess())
            {
                case ACCESS_Public:
                    NamespaceForSymbol->SetContainsPublicType(true);
                    break;

                case ACCESS_Friend:
                    NamespaceForSymbol->SetContainsFriendType(true);
                    break;

            }
        }
    } // iterate symbols from file

    for(unsigned Index = 0; Index < NamespaceListCount; Index++)
    {
        // Indicate that the names in all the modules in this namespace are not yet loaded.
        NamespaceList[Index]->ClearNamesInAllModulesLoaded();

        // This will force Binding semantics on it when GetHash is called
        //
        Symbols::SetDelayBindNamespace(NamespaceList[Index]);
    }

    return hr;
}


/*****************************************************************************
;IsPIA

Identify whether this is a PIA assembly because they should have their friend 
and private metadata imported.
*****************************************************************************/
TriState<bool> MetaDataFile::IsPIA()
{
    TriState<bool> HasPiaAttribute;
    mdAssembly AssemblyToken = this->GetProject()->GetAssemblyToken();
    if ( AssemblyToken != mdAssemblyNil && this->GetImport() != NULL) // check GetImport() because remote/device debugging paths under the debugger may not have an importer
    {
        HasPiaAttribute = Attribute::HasAttribute( this, AssemblyToken, attrkindImportedFromTypeLib_String );
        if ( HasPiaAttribute.HasValue() && HasPiaAttribute.GetValue() == false ) // PIAs can be marked in more than one way
        {
            HasPiaAttribute = Attribute::HasAttribute( this, AssemblyToken, attrkindPrimaryInteropAssembly_Int32_Int32 );
        }
    }
    
    // If we were unable to look for the token because GetImport() returned NULL,
    // HasPiaAttribute.HasValue() will be false, indicating that we couldn't check for the attribute.
    return HasPiaAttribute;
}
