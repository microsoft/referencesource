//------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Implements the minimal set of interfaces required for
//     the version-independent hosting shim.
//
// History:
//      2005/05/09 - Microsoft
//          Created
//      2007/09/20 - Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#include "PreCompiled.hxx"
#include "HostShim.hxx"
#include "Version.hxx"
#include "VersionFactory.hxx"
#include "PersistFile.hxx"
#include "PersistMoniker.hxx"
#include "PersistHistory.hxx"
#include "ClassFactory.hxx"
#include "ShimUtilities.hxx"
#include "Main.hxx"
#include "DeploymentManifest.hxx"
#include "ApplicationManifest.hxx"
#include "MarkupVersion.hxx"
#include "WatsonReporting.hxx"

#include "..\inc\ShimDllInterface.hxx"
#include "..\inc\registry.hxx"


static LPCWSTR const LATEST_VERSION = L"0.0.0";
static LPCWSTR const VERSION_4 = L"4.0.0";

CHostShim* CHostShim::m_pInstance = NULL;

CHostShim::CHostShim():
    m_strErrorMessageToDisplay(24000)
{
    m_ObjRefCount = 0;
    m_pInstance = this;
    m_pInnerObject = NULL;
    m_pUnkSite = NULL;
    m_pVersion = NULL;
    // 
    m_pPersistMoniker = new CPersistMoniker(this);
    m_pPersistFile = new CPersistFile(this);
    m_pPersistHistory = new CPersistHistory(this);
    m_pOleObjectDelegate = NULL; 
    m_pPendingClientSite = NULL;
    m_pByteWrapperImpl = NULL;
    m_pByteRangeDownloaderService = NULL;
    m_pHistoryStream = m_pDocumentStream = NULL;
    m_mimeType = MimeType_Unknown;
    m_bAllowPersistFileLoad = m_bAllowPersistFileLoadSet = false;
    m_activationType = Unknown;
    m_bInvokeFrameworkBootstrapper = false;

    m_hDownloadCompletedEvent = ::CreateEvent(
                    NULL,               // Default security, not inherited.
                    TRUE,               // Manual reset event.
                    TRUE,               // Initially signaled.
                    NULL                // No name.
                    );
}

CHostShim::~CHostShim()
{
    if (m_pVersion && m_pVersion->IsAttached())
    {
        m_pVersion->Deactivate();
    }

    m_pInstance = NULL;

    delete m_pPersistMoniker;
    delete m_pPersistFile;
    delete m_pPersistHistory;

    ReleaseInterface(m_pUnkSite);

    m_pInstance = NULL;
    m_pInnerObject = NULL;
    m_pPersistFile = NULL;
    m_pPersistHistory = NULL;
    m_pPersistMoniker = NULL;
    m_pOleObjectDelegate = NULL;
    m_pPendingClientSite = NULL;


    // Do not close m_hDownloadCompletedEvent here, destroy it in DllMain
    // to avoid race condition.

    PostQuitMessage(ERROR_SUCCESS);
}

CHostShim* CHostShim::GetInstance()
{
     return m_pInstance; 
}

CVersion* CHostShim::GetValidVersion()
{
    CHostShim* pHostShim = GetInstance();
    if (!pHostShim)
        return NULL;
    
    CVersion* pVersion = pHostShim->GetVersion();
    if (!pVersion)
        return NULL;
    
    if (!pVersion->IsValid())
        return NULL;
    
    return pVersion;
}

//******************************************************************************
//
// IUnknown Implementation
//
//******************************************************************************

/******************************************************************************
 CHostShim::QueryInterface
******************************************************************************/
STDMETHODIMP CHostShim::QueryInterface( __in_ecount(1) REFIID riid, __deref_out_ecount(1) LPVOID *ppReturn)
{
    HRESULT hr = E_NOINTERFACE;
    CHECK_POINTER(ppReturn);

    *ppReturn = NULL;

    if (riid == IID_IUnknown)
    {
        *ppReturn = (IUnknown*) ((IOleObject*) this);
    }
    else if (riid == IID_IStdMarshalInfo)
    {
        *ppReturn = (IStdMarshalInfo*)this;
    }
    else if (riid == IID_IPersist)
    {
        // Doesn't matter which persist interface since all of them update
        // the clsid on the doc and defer the GetClassID call to the doc
        *ppReturn = (IPersist *) m_pPersistMoniker;
    }
    else if (riid == IID_IPersistFile)
    {
        *ppReturn = m_pPersistFile;
    }
    else if (riid == IID_IPersistMoniker)
    {
        *ppReturn = m_pPersistMoniker;

        /*
        If marshaling of IPersistMoniker fails, IE will fall back to IPersistFile activation by passing 
        the location of the .xbap file from the cache. This doesn't work because the deployment manifest is 
        separated from the rest of the application. (But it's okay for XPSViewer; hence the conditional
        compilation.) 

        The registry keys for IPersistMoniker are apparently susceptible to an ACL anomaly similar to that
        described in WatsonReporting.hxx: There is no entry for the Users group, only Administrators and 
        SYSTEM. Causes are still unknown, but there have been real cases confirmed. Even on some good 
        computers permission inheritance for the main IPersistMoniker key is disabled. This is a likely 
        precondition for the real problem.
        */
        static bool fTestedMarshaling = false;
        if(!fTestedMarshaling)
        {
            fTestedMarshaling = true;
            CComPtr<IStream> spMonikerStream;
            if(FAILED(CoMarshalInterThreadInterfaceInStream(IID_IPersistMoniker, m_pPersistMoniker, &spMonikerStream)))
            {
                TriggerWatson(ActivationProblemIds::IPersistMonikerMarshalingFailed);
            }
            else
            {
                CoReleaseMarshalData(spMonikerStream);
            }
        }
    }
    else if (riid == IID_IPersistHistory)
    {
        *ppReturn = m_pPersistHistory;
    }
    else if(riid == IID_IHlinkTarget)
    {
        if (!GetVersion())
        {
            *ppReturn = (IHlinkTarget*)this;
        }
    }
    else if (riid == IID_IOleObject)
    {
        if (m_bInvokeFrameworkBootstrapper || !GetVersion())
        {
            *ppReturn = (IOleObject*) this;
        }
        // else: query inner object [below]
    }
    else if (riid == IID_IObjectWithSite)
    {
        *ppReturn = (IObjectWithSite*)this;
    }

    if (*ppReturn)
    {
        AddRef();
        hr = S_OK;
    }
    else
    {
        if (m_pInnerObject)
        {
            return m_pInnerObject->QueryInterface(riid, ppReturn);
        }
    }
Cleanup:   
    return hr;
}                                             

/******************************************************************************
 CHostShim::AddRef
******************************************************************************/
STDMETHODIMP_(DWORD) CHostShim::AddRef()
{
    return InterlockedIncrement(&m_ObjRefCount);
}

/******************************************************************************
 CHostShim::Release
******************************************************************************/
STDMETHODIMP_(DWORD) CHostShim::Release()
{
    DWORD uRefs = InterlockedDecrement(&m_ObjRefCount);
    
    if (uRefs == 0)
    {
#ifdef DEBUG
        OutputDebugString(L"CHostShim::Release()\n");
#endif
        delete this;
        return 0;
    }
   
    return uRefs;
}

//******************************************************************************
//
// IStdMarshalInfo Implementation
//
//******************************************************************************

// See host\Proxy\BrowserInprocHandler.hxx for explanation of the in-proc handler.
HRESULT CHostShim::GetClassForHandler(DWORD dwDestContext, void*, CLSID * pClsid)
{
    if(dwDestContext == MSHCTX_LOCAL) // marshaling to another process?
    {
        *pClsid = CLSID_PresentationHostInprocHandler;
        return S_OK;
    }
    return E_FAIL;
}

//******************************************************************************
//
// IObjectWithSite Implementation
//
//******************************************************************************

HRESULT CHostShim::SetSite(IUnknown *pUnkSite)
{
    ReplaceInterface(m_pUnkSite, pUnkSite);
    return S_OK;
}

HRESULT CHostShim::GetSite(REFIID riid, /* [iid_is][out] */ void **ppvSite)
{
    *ppvSite = 0;
    if(!m_pUnkSite)
        return E_FAIL; // per IObjectWithSite specification
    return m_pUnkSite->QueryInterface(riid, ppvSite);
}


/******************************************************************************
  CHostShim::GetClassID()
******************************************************************************/
STDMETHODIMP CHostShim::GetClassID(__out_ecount(1) LPCLSID pClassID)
{
    HRESULT hr = S_OK;
    CK_PARG(pClassID);

    *pClassID = *CClassFactory::GetActivatedClsId();

Cleanup:
    return hr;
}

void CHostShim::InvokeFrameworkBootstrapperWithFallback()
{
    if(FAILED(InvokeFrameworkBootstrapper()))
    {
        // Need to set dialog owner window. Otherwise it will appear behind the browser's window.
        HWND hwnd = 0;
        CComQIPtr<IOleWindow> spOleWindow(m_pPendingClientSite);
        if(spOleWindow)
        {
            spOleWindow->GetWindow(&hwnd);
        }
        wchar_t prompt[2000];
        if(LoadResourceString(IDS_GETNETFX, prompt, ARRAYSIZE(prompt)) &&
           MessageBox(hwnd, prompt, L"Microsoft .NET Framework Setup", MB_ICONQUESTION | MB_OKCANCEL) == IDOK)
        {
            ShellExecute(hwnd, 0, L"http://go.microsoft.com/fwlink/?linkid=54520", 0, 0, SW_SHOW);
        }
    }
}

// WinFxDocObj is the .NET Framework bootstrapper that ships with IE (for Windows version before Windows 7).
// It's registered and invoked as a DocObject. It only needs to be driven to the IOleObject::DoVerb() step.
HRESULT CHostShim::InvokeFrameworkBootstrapper()
{
    HRESULT hr = S_OK;
    CComPtr<IOleObject> spBootstrapper;
    CComPtr<IBindCtx> spBindCtx;
    CComPtr<IMoniker> spMoniker;
    CComQIPtr<IPersistMoniker> spMonikerLoader;
    CComQIPtr<IPersistFile> spFileLoader;
    CComPtr<IWebBrowser2> spWebBrowser;

    // WinFxDocObj works only in IE - it needs the IWebBrowser2. Fail if this is not IE.
    ASSERT(m_pPendingClientSite);
    CKHR(QueryService(m_pPendingClientSite, SID_SWebBrowserApp, IID_IWebBrowser2, (void**)&spWebBrowser));
    
    CKHR(spBootstrapper.CoCreateInstance(L"Bootstrap.XBAP"));
    // WinFxDocObj uses the client site to get the browser's window. 
    // Quirk: It does this when the client site is reset to null (using the previous reference).
    spBootstrapper->SetClientSite(m_pPendingClientSite);
    spBootstrapper->SetClientSite(0);

    // Pass the startup URL via IPersistMoniker or IPersistFile.
    // Like PresentationHost, WinFxDoc fails IPeristMoniker::Load() for file:// URLs. Just like IE, try 
    // IPersistFile then. Note that calling IPersistMoniker first is needed because that's where WinFxDocObj
    // captures the URL to use it later.
    CKHR(CreateURLMonikerEx(0, m_strStartupUri.GetValue(), &spMoniker, URL_MK_UNIFORM));
    spMonikerLoader = spBootstrapper; //QI
    if(!spMonikerLoader) 
        CKHR(E_NOINTERFACE);
    CKHR(CreateBindCtx(0, &spBindCtx));
    hr = spMonikerLoader->Load(false, spMoniker, spBindCtx, 0);
    if(hr != S_OK)
    {
        if(hr == S_FALSE)
        {
            hr = E_FAIL;
        }
        spFileLoader = spBootstrapper; //QI
        if(spFileLoader && spFileLoader->Load(m_strStartupUri.GetValue(), 0) == S_OK)
            hr = S_OK;
        //else: the result of IPeristMoniker::Load() will be returned/logged.
    }
    CKHR(hr);

    CKHR(spBootstrapper->DoVerb(0, 0, m_pPendingClientSite, 0, 0, 0));

Cleanup:
    return hr;
}

/******************************************************************************
  CHostShim::Execute
  Invokes Watson and terminates on any failure.
******************************************************************************/
void CHostShim::Execute()
{
    DPF(8, _T("CHostShim::Execute"));
    HRESULT hr = S_OK;

    // If we don't have a version object yet...
    if (!m_pVersion)
    {
        // If the requested version hasn't been set yet...
        if (!this->GetRequestedVersion())
        {
            // Get the requested version
            CKHR(DetermineRequestedVersion());
        }

        // Get the version object
        m_pVersion = CVersionFactory::GetVersion(this->GetRequestedVersion());
    }

    // Since GetVersion() falls back to latest version if the requested one is not available, the likeliest
    // reason to get nothing here is that there is no SxS version available. (Only PresentationHost.exe
    // left behind as a shared, permanent component.) Then we try to invoke the WinFxDocObj bootstrapper.
    // Since it's not available starting from Windows 7, fallback is a message box.
    if(!m_pVersion)
    {
        // We need the IOleClientSite to make WinFxDocObj work. So, let the browser keep going with the 
        // activation...
        m_bInvokeFrameworkBootstrapper = true;
    }
    // We have a version object, so if it is unattached, try to attach to it
    else if (!m_pVersion->IsAttached())
    {
		// Displaying an error message via PHDLL is new for v4 and v3.5 SP1 on Windows 7. Unfortunately, we did
		// not account for the possibility of PH v4 finding only PHDLL v3 available and an older one that doesn't
		// have the support for displaying an error message. This can happen if v4 is installed and then uninstalled.
		// As a shared component, PH.exe is only rolled forward. If we proceed in such case, PHDLL will flounder 
		// for a while and eventually crash, in a very undebuggable way. To prevent this, do a timestamp check and
		// fall back to a plain message box (which, unfortunately, tends to pop behind the browser window--no owner.)
		// The assumption is that any (re)release of PHDLL v3 in 2009 or later will have the feature.
		if(m_strErrorMessageToDisplay.GetValue() && m_pVersion->Major() == 3)
		{
			WIN32_FILE_ATTRIBUTE_DATA fileInfo;
			SYSTEMTIME fileTime;
			if(GetFileAttributesEx(m_pVersion->GetLibraryPath(), GetFileExInfoStandard, &fileInfo) &&
				FileTimeToSystemTime(&fileInfo.ftLastWriteTime, &fileTime))
			{
				if(fileTime.wYear < 2009)
				{
					MessageBeep((UINT)-1);
					MessageBox(0, m_strErrorMessageToDisplay.GetValue(), CComBSTR(TARGETNAME), MB_ICONERROR);
					return;
				}
			}
		}

		CKHR(m_pVersion->Attach());

        ActivateParameters parameters;
        parameters.dwSize = sizeof(parameters);
        parameters.pOuterObject = (LPUNKNOWN) ((LPOLEOBJECT) this);
        parameters.pswzUri = GetStartupUri();
        parameters.pswzDebugSecurityZoneURL = g_pswzDebugSecurityZoneURL;
        parameters.pswzApplicationIdentity = GetApplicationIdentity();
        parameters.mime = GetMimeType();
        parameters.pHistoryStream = GetHistoryStream();
        parameters.pDocumentStream = GetDocumentStream();
        parameters.hDownloadCompletedEvent = GetDownloadCompletedEvent();
        parameters.isDebug = g_dwCommandLineFlags & FLAG_DEBUG;
        parameters.pPersistHistory = (IPersistHistory*) m_pPersistHistory;
        parameters.hNoHostTimer = g_hNoHostTimer_shim;
        parameters.pswzErrorMessageToDisplay = m_strErrorMessageToDisplay.GetValue();

        m_pVersion->Activate(&parameters, &m_pInnerObject); //[Watson on failure]

        if (GetActivationType() != FileActivation && GetActivationType() != MonikerActivation)
        {
            IOleObject* pOleObject = NULL;
            CKHR(m_pInnerObject->QueryInterface(IID_IOleObject, (LPVOID*)&pOleObject));
            pOleObject->Release(); // we don't want to addref this because we actually aggregate it.
            SetOleObjectDelegate(pOleObject);
        }
    }

Cleanup:
    HANDLE_ACTIVATION_FAULT(hr);
}

HRESULT CHostShim::SaveToHistory(__in_ecount(1) IStream* pHistoryStream)
{
    return m_pVersion->SaveToHistory(pHistoryStream);
}

HRESULT CHostShim::LoadFromHistory(__in_ecount(1) IStream* pHistoryStream, __in_ecount(1) IBindCtx* pBindCtx)
{
    return m_pVersion->LoadFromHistory(pHistoryStream, pBindCtx);
}


//******************************************************************************
//
//  IHlinkTarget Implementation
//
//******************************************************************************

/******************************************************************************
  CHostShim::SetBrowseContext()
******************************************************************************/
STDMETHODIMP CHostShim::SetBrowseContext(__in_ecount_opt(1) IHlinkBrowseContext *pihlbc)
{
    return E_NOTIMPL;
}

/******************************************************************************
  CHostShim::GetBrowseContext()
******************************************************************************/
STDMETHODIMP CHostShim::GetBrowseContext(__deref_out_ecount(1) IHlinkBrowseContext **ppihlbc)
{
    if(ppihlbc)
    {
        *ppihlbc = NULL;
    }
    return E_NOTIMPL;
}

/******************************************************************************
  CHostShim::Navigate()
******************************************************************************/
STDMETHODIMP CHostShim::Navigate(DWORD grfHLNF,__in_ecount_opt(INTERNET_MAX_URL_LENGTH+1) LPCWSTR pwzUri)
{
    HRESULT hr = S_OK;
    SetStartupLocation(pwzUri);

    if (!GetVersion()) // We haven't been QI'ed for IOleObject yet
    {
        CKHR(InvokeBrowser(GetStartupUri()));
        PostQuitMessage(ERROR_SUCCESS);
    }

Cleanup:
    if (FAILED(hr))
    {
        PostQuitMessage(ERROR_INVOKING_BROWSER);
    }
    return hr;
}

/******************************************************************************
  CHostShim::GetMoniker()
******************************************************************************/
STDMETHODIMP CHostShim::GetMoniker(__in_ecount(1) LPCWSTR pwzLocation, DWORD dwAssign, __deref_out_ecount(1) IMoniker **ppimkLocation)
{
    if (ppimkLocation)
    {
        *ppimkLocation = NULL;
    }
    return E_NOTIMPL;
}

/******************************************************************************
  CHostShim::GetFriendlyName()
******************************************************************************/
STDMETHODIMP CHostShim::GetFriendlyName(__in_ecount(1) LPCWSTR pwzLocation, __deref_out_ecount(1) LPWSTR *ppwzFriendlyName)
{
    if(ppwzFriendlyName)
    {
        *ppwzFriendlyName = NULL;
    }
    return E_NOTIMPL;
}

/******************************************************************************
  DoNotLaunchVersion3HostedApplicationInVersion4Runtime()

  Reads DWORD registry value DoNotLaunchVersion3HostedApplicationInVersion4Runtime from 
  HKLM\Software\Microsoft\.NETFramework\Windows Presentation Foundation\Hosting and interprets
  it as a boolean (non-zero == true, zero == false). 

  When this value is set to true, we will load .NET 3.5 based XBAP's in the .NET 3.5 
  runtime, and .NET 4.x based XBAP's in the .NET 4.x runtime. In other words, when this value 
  is set, we will match the runtime to the XBAP's CLR version. 

  When this value is set to false, or when this value is not set (it will default to false), 
  we will always load XBAP's using the latest CLR version, which is part of the 4.x framework.
******************************************************************************/
static bool DoNotLaunchVersion3HostedApplicationInVersion4Runtime()
{
    DWORD dwValue = 0U;
    // Ignore resultant HRESULT - we will default to the assumption that the value is 
    // false on failure
    GetRegistryDWORD(HKEY_LOCAL_MACHINE,
        RegKey_WPF_Hosting,
        RegValue_DoNotLaunchVersion3HostedApplicationInVersion4Runtime,
        /*out*/dwValue,
        /*dwDefaultValue*/ 0U);

    return dwValue != 0;
}


/******************************************************************************
  CHostShim::DetermineRequestedVersion()
  
  This is where we attempt to determine the requested version.
  For right now we are using a registry lookup hack to determine what version
  of Avalon an app requires. Eventually, this information will be obtained
  from the deployment manifest.
******************************************************************************/
HRESULT CHostShim::DetermineRequestedVersion()
{
    HRESULT hr = S_OK;

    if(m_strErrorMessageToDisplay.GetValue())
    {
        // We are going to activate the DLL just for its error page.
        SetRequestedVersion(LATEST_VERSION);
        return S_OK;
    }

    bool fRequestedVersionIdentified = false;
    if (!DoNotLaunchVersion3HostedApplicationInVersion4Runtime())
    {
        // Always load using v4 runtime
        // Mark the requested version and continue 
        // parsing the deployment manifest. The information 
        // from the manifest is used sometimes in exception
        // strings etc. Also continue enumerating 
        // available CLR versions. 
        CKHR(SetRequestedVersion(VERSION_4));
        fRequestedVersionIdentified = !!SUCCEEDED(hr);
    }

    // To ensure that as much code as possible is tested as often as possible, 
    // disable the optimization below for debug builds.
    // Also disabled when profiling based on a registry setting.
    bool fSingleVersionShortcut = true;
#ifdef DEBUG
    fSingleVersionShortcut = false;
#else
    if(ETW_ENABLED_CHECK(TRACE_LEVEL_INFORMATION))
    {
        DWORD dw;
        ::GetRegistryDWORD(HKEY_LOCAL_MACHINE, RegKey_WPF_Hosting, RegValue_DisableSingleVersionOptimization, /*out*/dw, 0);
        fSingleVersionShortcut = !dw;
    }        
#endif

    if (m_mimeType == MimeType_Application)
    {
        CDeploymentManifest deploymentManifest(GetStartupUri(), GetLocalDeploymentManifestPath());
        hr = deploymentManifest.Read();

        if (SUCCEEDED(hr))
        {
            SetApplicationIdentity(deploymentManifest.GetApplicationIdentity());

            if (fSingleVersionShortcut && CVersionFactory::GetCount() == 1)
            {
                if (!fRequestedVersionIdentified)
                {
                    // If there is only one version of Avalon installed, there isn't any point
                    // in checking the application manifest
                    CKHR(SetRequestedVersion(LATEST_VERSION));
                    fRequestedVersionIdentified = !!SUCCEEDED(hr);
                }
            }
            else 
            {
                CApplicationManifest applicationManifest(GetStartupUri(), deploymentManifest.GetApplicationManifestCodebase());
                applicationManifest.AddRef(); // need to do this because msxml will addref and release, and we still need it

                hr = applicationManifest.Read();

                if (SUCCEEDED(hr))
                {
                    //TEMP WORKAROUND for putting reference to WindowsBase v3 in the app manifest.
                    // (This is done so that the PolicyLevel can be resolved correctly with the additional
                    // permissions we put under HKLM\SOFTWARE\Microsoft\.NETFramework\Security\Policy\Extensions\NamedPermissionSets,
                    // which are still v3 for bkw compatibility.)
                    CVersion ver(applicationManifest.GetRequestedVersion());
                    CVersion verCLR(applicationManifest.GetRequestedClrVersion());
                    const CVersion *pLargerVersion = &ver;
                    if(ver.CompareTo(&verCLR) < 0)
                    {
                        pLargerVersion = &verCLR;
                    }

                    if (!fRequestedVersionIdentified)
                    {
                        hr = SetRequestedVersion(pLargerVersion->GetValue());
                        fRequestedVersionIdentified = !!SUCCEEDED(hr);
                    }
                }
            }
        }

        if (FAILED(hr))
        {
            // We have a failure code (hr) from reading the deployment manifest, 
            // but our eventual goal is to identify a version, and so the hr
            // we should return from this method should correspond to the 
            // success/failure of version selection (and not that of 
            // reading the deployment manifest).
            // 
            // If we already have selected a version (fRequestedVersionIdentified == 
            // true), then set hr = S_OK, otherwise proceed to the call into 
            // SetRequestedVersion and allow CKHR macro's side-effect to update
            // hr appropriately.
            CKHR(S_OK);
            if (!fRequestedVersionIdentified)
            {
                CKHR(SetRequestedVersion(LATEST_VERSION));
                fRequestedVersionIdentified = !!SUCCEEDED(hr);
            }
        }
    }
    else if (m_mimeType == MimeType_Markup)
    {
        if (fSingleVersionShortcut && CVersionFactory::GetCount() == 1)
        {
            if (!fRequestedVersionIdentified)
            {
                // If there is only one version of Avalon installed, there isn't any point
                // in checking the application manifest
                CKHR(SetRequestedVersion(LATEST_VERSION));
                fRequestedVersionIdentified = !!SUCCEEDED(hr);
            }
        }
        else 
        {
            CMarkupVersion markupVersion(GetLocalDeploymentManifestPath());
            markupVersion.AddRef(); // need to do this because msxml will addref and release, and we still need it

            // We don't actually care what the HRESULT is since an error indicates that we are done parsing the XAML
            // file and we should just go with whatever we have even if there was some other error.
            (void) markupVersion.Read(); 

            // Get the highest version for the namespaces that we have (ignorable or not.)
            CVersion* pHighestVersion = NULL;
            CStringMap<CString*>::CStringMapNode* pPrefixNamespaceNode = markupVersion.GetPrefixNamespaces().GetRoot();
            while (pPrefixNamespaceNode)
            {
                CString* pStrNamespace = pPrefixNamespaceNode->GetValue();
                CString* pStrVersion = NULL;

                if (SUCCEEDED(markupVersion.GetNamespaceVersion().Find(pStrNamespace->GetValue(), &pStrVersion)))
                {
                    CVersion* pVersion = CVersionFactory::GetVersion(pStrVersion->GetValue(), FALSE);

                    if (pVersion && (pHighestVersion == NULL || pHighestVersion->CompareTo(pVersion) < 0))
                    {
                        pHighestVersion = pVersion;
                    }
                }
                else
                {
                    ASSERT(FALSE); // Should always have been able to find the version number.
                }

                pPrefixNamespaceNode = pPrefixNamespaceNode->GetNext();
            }

            // We may have a failure code (hr) from iterating through the deployment manifest, 
            // but our eventual goal is to identify a version, and so the hr
            // we should return from this method should correspond to the 
            // success/failure of version selection (and not that of 
            // deployment manifest reading).
            // 
            // If we have already selected a version (fRequestedVersionIdentified == 
            // true), then set hr = S_OK, otherwise proceed to the call into 
            // SetRequestedVersion and allow CKHR macro's side-effect to update
            // hr appropriately.
            CKHR(S_OK);
            if (!fRequestedVersionIdentified)
            {
                if (pHighestVersion)
                {
                    CKHR(SetRequestedVersion(pHighestVersion->GetValue()));
                }
                else
                {
                    CKHR(SetRequestedVersion(LATEST_VERSION));
                }

                fRequestedVersionIdentified = !!SUCCEEDED(hr);
            }
        }
    }
    else
    {
        // Bad MIME-type
        ASSERT(FALSE);
        CKHR(E_FAIL);
    }

Cleanup:
    return hr;
}

//-----------------------------------------------------------------------------
// IOleObject and related methods
//-----------------------------------------------------------------------------

void CHostShim::SetOleObjectDelegate(IOleObject* pOleObjectDelegate) 
{ 
    m_pOleObjectDelegate = pOleObjectDelegate;
    if (m_pPendingClientSite)
    {
        m_pOleObjectDelegate->SetClientSite(m_pPendingClientSite);
        m_pPendingClientSite->Release();
    }
}

STDMETHODIMP CHostShim::SetClientSite(__in LPOLECLIENTSITE pClientSite)
{ 
    if (m_pOleObjectDelegate)
    {
        return m_pOleObjectDelegate->SetClientSite(pClientSite);
    }
    else
    {
        ReplaceInterface(m_pPendingClientSite, pClientSite);

        if(m_bInvokeFrameworkBootstrapper && pClientSite)
        {
            InvokeFrameworkBootstrapperWithFallback();
            return E_FAIL;
        }

        return S_OK;
    }
}

STDMETHODIMP CHostShim::Advise(__in LPADVISESINK pAdvSink, __out LPDWORD pdwConnection)
{ 
    return m_pOleObjectDelegate ? m_pOleObjectDelegate->Advise(pAdvSink, pdwConnection) : E_NOTIMPL; 
}

STDMETHODIMP CHostShim::SetHostNames(LPCOLESTR szContainerApp, LPCOLESTR szCOntainerObj) 
{ 
    return m_pOleObjectDelegate ? m_pOleObjectDelegate->SetHostNames(szContainerApp, szCOntainerObj) : E_NOTIMPL; 
}

STDMETHODIMP CHostShim::DoVerb(LONG iVerb, __in_opt LPMSG lpMsg, __in LPOLECLIENTSITE pActiveSite, LONG lIndex, HWND hwndParent, LPCRECT lprcPosRect)
{ 
    return m_pOleObjectDelegate ? m_pOleObjectDelegate->DoVerb(iVerb, lpMsg, pActiveSite, lIndex, hwndParent, lprcPosRect) : E_NOTIMPL; 
}

STDMETHODIMP CHostShim::GetExtent(DWORD dwDrawAspect, LPSIZEL psizel) 
{ 
    return m_pOleObjectDelegate ? m_pOleObjectDelegate->GetExtent(dwDrawAspect, psizel) : E_NOTIMPL; 
}

STDMETHODIMP CHostShim::Update() 
{ 
    return m_pOleObjectDelegate ? m_pOleObjectDelegate->Update() : E_NOTIMPL; 
}

STDMETHODIMP CHostShim::Close(DWORD dwSaveOption) 
{ 
    return m_pOleObjectDelegate ? m_pOleObjectDelegate->Close(dwSaveOption) : E_NOTIMPL; 
}

STDMETHODIMP CHostShim::Unadvise(DWORD dwToken) 
{ 
    return m_pOleObjectDelegate ? m_pOleObjectDelegate->Unadvise(dwToken) : E_NOTIMPL; 
}

STDMETHODIMP CHostShim::EnumVerbs(LPENUMOLEVERB* ppEnumOleVerb) 
{ 
    return m_pOleObjectDelegate ? m_pOleObjectDelegate->EnumVerbs(ppEnumOleVerb) : E_NOTIMPL; 
}

STDMETHODIMP CHostShim::GetClientSite(__out LPOLECLIENTSITE* ppClientSite) 
{ 
    return m_pOleObjectDelegate ? m_pOleObjectDelegate->GetClientSite(ppClientSite) : E_NOTIMPL; 
}

STDMETHODIMP CHostShim::SetMoniker(DWORD dwWhichMoniker, LPMONIKER pmk) 
{ 
    return m_pOleObjectDelegate ? m_pOleObjectDelegate->SetMoniker(dwWhichMoniker, pmk) : E_NOTIMPL; 
}

STDMETHODIMP CHostShim::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, __out LPMONIKER* ppmk) 
{ 
    return m_pOleObjectDelegate ? m_pOleObjectDelegate->GetMoniker(dwAssign, dwWhichMoniker, ppmk) : E_NOTIMPL; 
}

STDMETHODIMP CHostShim::InitFromData(LPDATAOBJECT pDataObject, BOOL fCreation, DWORD dwReserved) 
{ 
    return m_pOleObjectDelegate ? m_pOleObjectDelegate->InitFromData(pDataObject, fCreation, dwReserved) : E_NOTIMPL; 
}

STDMETHODIMP CHostShim::GetClipboardData(DWORD dwReserved, __out LPDATAOBJECT* ppDataObject) 
{ 
    return m_pOleObjectDelegate ? m_pOleObjectDelegate->GetClipboardData(dwReserved, ppDataObject) : E_NOTIMPL; 
}

STDMETHODIMP CHostShim::IsUpToDate() 
{ 
    return m_pOleObjectDelegate ? m_pOleObjectDelegate->IsUpToDate() : S_OK; 
}

STDMETHODIMP CHostShim::GetUserClassID(__out CLSID* pclsId) 
{ 
    return m_pOleObjectDelegate ? m_pOleObjectDelegate->GetUserClassID(pclsId) : E_NOTIMPL; 
}

STDMETHODIMP CHostShim::GetUserType(DWORD dwFormOfType, __out LPOLESTR* pszUserType) 
{ 
    return m_pOleObjectDelegate ? m_pOleObjectDelegate->GetUserType(dwFormOfType, pszUserType) : E_NOTIMPL; 
}

STDMETHODIMP CHostShim::SetExtent(DWORD dwDrawAspect, LPSIZEL psizel) 
{ 
    return m_pOleObjectDelegate ? m_pOleObjectDelegate->SetExtent(dwDrawAspect, psizel) : E_NOTIMPL; 
}

STDMETHODIMP CHostShim::EnumAdvise(__out LPENUMSTATDATA* ppenumAdvise) 
{ 
    return m_pOleObjectDelegate ? m_pOleObjectDelegate->EnumAdvise(ppenumAdvise) : E_NOTIMPL; 
}

STDMETHODIMP CHostShim::GetMiscStatus(DWORD dwAspect, __out LPDWORD pdwStatus)
{ 
    if (m_pOleObjectDelegate)
    {
        return m_pOleObjectDelegate->GetMiscStatus(dwAspect, pdwStatus);
    }
    else
    {
        *pdwStatus = OLEMISC_SETCLIENTSITEFIRST;
        return S_OK;
    }
}

STDMETHODIMP CHostShim::SetColorScheme(LPLOGPALETTE pLogPal) 
{ 
    return m_pOleObjectDelegate ? m_pOleObjectDelegate->SetColorScheme(pLogPal) : S_OK; 
}
