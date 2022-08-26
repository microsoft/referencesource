//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Implements the BootStrapper class of PresentationHost.
//
//  History
//      2002/06/25-murrayw
//          Created
//      2003/06/30-Microsoft
//          Ported ByteRangeDownloader to WCP
//      2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#include "Precompiled.hxx"
#include "Bootstrapper.hxx"
#include "OleDocument.hxx"
#include <fxver.h>

//******************************************************************************
//
// The following definitions are obtained by doing a #import of mscorlib.dll and
// copying over the required interface definitions.  If there are any breaking
// changes to these interfaces, we need to regenerate the definitions. 
//
//******************************************************************************

struct __declspec(uuid("05f696dc-2b29-3663-ad8b-c4389cf2a713"))
_AppDomain : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_ToString (
        /*[out,retval]*/ BSTR * pRetVal ) = 0;
      virtual HRESULT __stdcall Equals (
        /*[in]*/ VARIANT other,
        /*[out,retval]*/ VARIANT_BOOL * pRetVal ) = 0;
      virtual HRESULT __stdcall GetHashCode (
        /*[out,retval]*/ long * pRetVal ) = 0;
      virtual HRESULT __stdcall GetType (
        /*[out,retval]*/ struct _Type * * pRetVal ) = 0;
      virtual HRESULT __stdcall InitializeLifetimeService (
        /*[out,retval]*/ VARIANT * pRetVal ) = 0;
      virtual HRESULT __stdcall GetLifetimeService (
        /*[out,retval]*/ VARIANT * pRetVal ) = 0;
      virtual HRESULT __stdcall get_Evidence (
        /*[out,retval]*/ struct _Evidence * * pRetVal ) = 0;
      virtual HRESULT __stdcall add_DomainUnload (
        /*[in]*/ struct _EventHandler * value ) = 0;
      virtual HRESULT __stdcall remove_DomainUnload (
        /*[in]*/ struct _EventHandler * value ) = 0;
      virtual HRESULT __stdcall add_AssemblyLoad (
        /*[in]*/ struct _AssemblyLoadEventHandler * value ) = 0;
      virtual HRESULT __stdcall remove_AssemblyLoad (
        /*[in]*/ struct _AssemblyLoadEventHandler * value ) = 0;
      virtual HRESULT __stdcall add_ProcessExit (
        /*[in]*/ struct _EventHandler * value ) = 0;
      virtual HRESULT __stdcall remove_ProcessExit (
        /*[in]*/ struct _EventHandler * value ) = 0;
      virtual HRESULT __stdcall add_TypeResolve (
        /*[in]*/ struct _ResolveEventHandler * value ) = 0;
      virtual HRESULT __stdcall remove_TypeResolve (
        /*[in]*/ struct _ResolveEventHandler * value ) = 0;
      virtual HRESULT __stdcall add_ResourceResolve (
        /*[in]*/ struct _ResolveEventHandler * value ) = 0;
      virtual HRESULT __stdcall remove_ResourceResolve (
        /*[in]*/ struct _ResolveEventHandler * value ) = 0;
      virtual HRESULT __stdcall add_AssemblyResolve (
        /*[in]*/ struct _ResolveEventHandler * value ) = 0;
      virtual HRESULT __stdcall remove_AssemblyResolve (
        /*[in]*/ struct _ResolveEventHandler * value ) = 0;
      virtual HRESULT __stdcall add_UnhandledException (
        /*[in]*/ struct _UnhandledExceptionEventHandler * value ) = 0;
      virtual HRESULT __stdcall remove_UnhandledException (
        /*[in]*/ struct _UnhandledExceptionEventHandler * value ) = 0;
      virtual HRESULT __stdcall DefineDynamicAssembly (
        /*[in]*/ struct _AssemblyName * name,
        /*[in]*/ enum AssemblyBuilderAccess access,
        /*[out,retval]*/ struct _AssemblyBuilder * * pRetVal ) = 0;
      virtual HRESULT __stdcall DefineDynamicAssembly_2 (
        /*[in]*/ struct _AssemblyName * name,
        /*[in]*/ enum AssemblyBuilderAccess access,
        /*[in]*/ BSTR dir,
        /*[out,retval]*/ struct _AssemblyBuilder * * pRetVal ) = 0;
      virtual HRESULT __stdcall DefineDynamicAssembly_3 (
        /*[in]*/ struct _AssemblyName * name,
        /*[in]*/ enum AssemblyBuilderAccess access,
        /*[in]*/ struct _Evidence * Evidence,
        /*[out,retval]*/ struct _AssemblyBuilder * * pRetVal ) = 0;
      virtual HRESULT __stdcall DefineDynamicAssembly_4 (
        /*[in]*/ struct _AssemblyName * name,
        /*[in]*/ enum AssemblyBuilderAccess access,
        /*[in]*/ struct _PermissionSet * requiredPermissions,
        /*[in]*/ struct _PermissionSet * optionalPermissions,
        /*[in]*/ struct _PermissionSet * refusedPermissions,
        /*[out,retval]*/ struct _AssemblyBuilder * * pRetVal ) = 0;
      virtual HRESULT __stdcall DefineDynamicAssembly_5 (
        /*[in]*/ struct _AssemblyName * name,
        /*[in]*/ enum AssemblyBuilderAccess access,
        /*[in]*/ BSTR dir,
        /*[in]*/ struct _Evidence * Evidence,
        /*[out,retval]*/ struct _AssemblyBuilder * * pRetVal ) = 0;
      virtual HRESULT __stdcall DefineDynamicAssembly_6 (
        /*[in]*/ struct _AssemblyName * name,
        /*[in]*/ enum AssemblyBuilderAccess access,
        /*[in]*/ BSTR dir,
        /*[in]*/ struct _PermissionSet * requiredPermissions,
        /*[in]*/ struct _PermissionSet * optionalPermissions,
        /*[in]*/ struct _PermissionSet * refusedPermissions,
        /*[out,retval]*/ struct _AssemblyBuilder * * pRetVal ) = 0;
      virtual HRESULT __stdcall DefineDynamicAssembly_7 (
        /*[in]*/ struct _AssemblyName * name,
        /*[in]*/ enum AssemblyBuilderAccess access,
        /*[in]*/ struct _Evidence * Evidence,
        /*[in]*/ struct _PermissionSet * requiredPermissions,
        /*[in]*/ struct _PermissionSet * optionalPermissions,
        /*[in]*/ struct _PermissionSet * refusedPermissions,
        /*[out,retval]*/ struct _AssemblyBuilder * * pRetVal ) = 0;
      virtual HRESULT __stdcall DefineDynamicAssembly_8 (
        /*[in]*/ struct _AssemblyName * name,
        /*[in]*/ enum AssemblyBuilderAccess access,
        /*[in]*/ BSTR dir,
        /*[in]*/ struct _Evidence * Evidence,
        /*[in]*/ struct _PermissionSet * requiredPermissions,
        /*[in]*/ struct _PermissionSet * optionalPermissions,
        /*[in]*/ struct _PermissionSet * refusedPermissions,
        /*[out,retval]*/ struct _AssemblyBuilder * * pRetVal ) = 0;
      virtual HRESULT __stdcall DefineDynamicAssembly_9 (
        /*[in]*/ struct _AssemblyName * name,
        /*[in]*/ enum AssemblyBuilderAccess access,
        /*[in]*/ BSTR dir,
        /*[in]*/ struct _Evidence * Evidence,
        /*[in]*/ struct _PermissionSet * requiredPermissions,
        /*[in]*/ struct _PermissionSet * optionalPermissions,
        /*[in]*/ struct _PermissionSet * refusedPermissions,
        /*[in]*/ VARIANT_BOOL IsSynchronized,
        /*[out,retval]*/ struct _AssemblyBuilder * * pRetVal ) = 0;
      virtual HRESULT __stdcall CreateInstance (
        /*[in]*/ BSTR AssemblyName,
        /*[in]*/ BSTR typeName,
        /*[out,retval]*/ struct _ObjectHandle * * pRetVal ) = 0;
      virtual HRESULT __stdcall CreateInstanceFrom (
        /*[in]*/ BSTR assemblyFile,
        /*[in]*/ BSTR typeName,
        /*[out,retval]*/ struct _ObjectHandle * * pRetVal ) = 0;
      virtual HRESULT __stdcall CreateInstance_2 (
        /*[in]*/ BSTR AssemblyName,
        /*[in]*/ BSTR typeName,
        /*[in]*/ SAFEARRAY * activationAttributes,
        /*[out,retval]*/ struct _ObjectHandle * * pRetVal ) = 0;
      virtual HRESULT __stdcall CreateInstanceFrom_2 (
        /*[in]*/ BSTR assemblyFile,
        /*[in]*/ BSTR typeName,
        /*[in]*/ SAFEARRAY * activationAttributes,
        /*[out,retval]*/ struct _ObjectHandle * * pRetVal ) = 0;
      virtual HRESULT __stdcall CreateInstance_3 (
        /*[in]*/ BSTR AssemblyName,
        /*[in]*/ BSTR typeName,
        /*[in]*/ VARIANT_BOOL ignoreCase,
        /*[in]*/ enum BindingFlags bindingAttr,
        /*[in]*/ struct _Binder * Binder,
        /*[in]*/ SAFEARRAY * args,
        /*[in]*/ struct _CultureInfo * culture,
        /*[in]*/ SAFEARRAY * activationAttributes,
        /*[in]*/ struct _Evidence * securityAttributes,
        /*[out,retval]*/ struct _ObjectHandle * * pRetVal ) = 0;
      virtual HRESULT __stdcall CreateInstanceFrom_3 (
        /*[in]*/ BSTR assemblyFile,
        /*[in]*/ BSTR typeName,
        /*[in]*/ VARIANT_BOOL ignoreCase,
        /*[in]*/ enum BindingFlags bindingAttr,
        /*[in]*/ struct _Binder * Binder,
        /*[in]*/ SAFEARRAY * args,
        /*[in]*/ struct _CultureInfo * culture,
        /*[in]*/ SAFEARRAY * activationAttributes,
        /*[in]*/ struct _Evidence * securityAttributes,
        /*[out,retval]*/ struct _ObjectHandle * * pRetVal ) = 0;
      virtual HRESULT __stdcall Load (
        /*[in]*/ struct _AssemblyName * assemblyRef,
        /*[out,retval]*/ struct _Assembly * * pRetVal ) = 0;
      virtual HRESULT __stdcall Load_2 (
        /*[in]*/ BSTR assemblyString,
        /*[out,retval]*/ struct _Assembly * * pRetVal ) = 0;
      virtual HRESULT __stdcall Load_3 (
        /*[in]*/ SAFEARRAY * rawAssembly,
        /*[out,retval]*/ struct _Assembly * * pRetVal ) = 0;
      virtual HRESULT __stdcall Load_4 (
        /*[in]*/ SAFEARRAY * rawAssembly,
        /*[in]*/ SAFEARRAY * rawSymbolStore,
        /*[out,retval]*/ struct _Assembly * * pRetVal ) = 0;
      virtual HRESULT __stdcall Load_5 (
        /*[in]*/ SAFEARRAY * rawAssembly,
        /*[in]*/ SAFEARRAY * rawSymbolStore,
        /*[in]*/ struct _Evidence * securityEvidence,
        /*[out,retval]*/ struct _Assembly * * pRetVal ) = 0;
      virtual HRESULT __stdcall Load_6 (
        /*[in]*/ struct _AssemblyName * assemblyRef,
        /*[in]*/ struct _Evidence * assemblySecurity,
        /*[out,retval]*/ struct _Assembly * * pRetVal ) = 0;
      virtual HRESULT __stdcall Load_7 (
        /*[in]*/ BSTR assemblyString,
        /*[in]*/ struct _Evidence * assemblySecurity,
        /*[out,retval]*/ struct _Assembly * * pRetVal ) = 0;
      virtual HRESULT __stdcall ExecuteAssembly (
        /*[in]*/ BSTR assemblyFile,
        /*[in]*/ struct _Evidence * assemblySecurity,
        /*[out,retval]*/ long * pRetVal ) = 0;
      virtual HRESULT __stdcall ExecuteAssembly_2 (
        /*[in]*/ BSTR assemblyFile,
        /*[out,retval]*/ long * pRetVal ) = 0;
      virtual HRESULT __stdcall ExecuteAssembly_3 (
        /*[in]*/ BSTR assemblyFile,
        /*[in]*/ struct _Evidence * assemblySecurity,
        /*[in]*/ SAFEARRAY * args,
        /*[out,retval]*/ long * pRetVal ) = 0;
      virtual HRESULT __stdcall get_FriendlyName (
        /*[out,retval]*/ BSTR * pRetVal ) = 0;
      virtual HRESULT __stdcall get_BaseDirectory (
        /*[out,retval]*/ BSTR * pRetVal ) = 0;
      virtual HRESULT __stdcall get_RelativeSearchPath (
        /*[out,retval]*/ BSTR * pRetVal ) = 0;
      virtual HRESULT __stdcall get_ShadowCopyFiles (
        /*[out,retval]*/ VARIANT_BOOL * pRetVal ) = 0;
      virtual HRESULT __stdcall GetAssemblies (
        /*[out,retval]*/ SAFEARRAY * * pRetVal ) = 0;
      virtual HRESULT __stdcall AppendPrivatePath (
        /*[in]*/ BSTR Path ) = 0;
      virtual HRESULT __stdcall ClearPrivatePath ( ) = 0;
      virtual HRESULT __stdcall SetShadowCopyPath (
        /*[in]*/ BSTR s ) = 0;
      virtual HRESULT __stdcall ClearShadowCopyPath ( ) = 0;
      virtual HRESULT __stdcall SetCachePath (
        /*[in]*/ BSTR s ) = 0;
      virtual HRESULT __stdcall SetData (
        /*[in]*/ BSTR name,
        /*[in]*/ VARIANT data ) = 0;
      virtual HRESULT __stdcall GetData (
        /*[in]*/ BSTR name,
        /*[out,retval]*/ VARIANT * pRetVal ) = 0;
      virtual HRESULT __stdcall SetAppDomainPolicy (
        /*[in]*/ struct _PolicyLevel * domainPolicy ) = 0;
      virtual HRESULT __stdcall SetThreadPrincipal (
        /*[in]*/ struct IPrincipal * principal ) = 0;
      virtual HRESULT __stdcall SetPrincipalPolicy (
        /*[in]*/ enum PrincipalPolicy policy ) = 0;
      virtual HRESULT __stdcall DoCallBack (
        /*[in]*/ struct _CrossAppDomainDelegate * theDelegate ) = 0;
      virtual HRESULT __stdcall get_DynamicDirectory (
        /*[out,retval]*/ BSTR * pRetVal ) = 0;
};


struct __declspec(uuid("ea675b47-64e0-3b5f-9be7-f7dc2990730d"))
_ObjectHandle : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_ToString (
        /*[out,retval]*/ BSTR * pRetVal ) = 0;
      virtual HRESULT __stdcall Equals (
        /*[in]*/ VARIANT obj,
        /*[out,retval]*/ VARIANT_BOOL * pRetVal ) = 0;
      virtual HRESULT __stdcall GetHashCode (
        /*[out,retval]*/ long * pRetVal ) = 0;
      virtual HRESULT __stdcall GetType (
        /*[out,retval]*/ struct _Type * * pRetVal ) = 0;
      virtual HRESULT __stdcall GetLifetimeService (
        /*[out,retval]*/ VARIANT * pRetVal ) = 0;
      virtual HRESULT __stdcall InitializeLifetimeService (
        /*[out,retval]*/ VARIANT * pRetVal ) = 0;
      virtual HRESULT __stdcall CreateObjRef (
        /*[in]*/ struct _Type * requestedType,
        /*[out,retval]*/ struct _ObjRef * * pRetVal ) = 0;
      virtual HRESULT __stdcall Unwrap (
        /*[out,retval]*/ VARIANT * pRetVal ) = 0;
};


//******************************************************************************
//
//   BootStrapper::BootStrapper
//
//******************************************************************************

BootStrapper::BootStrapper(__in COleDocument* pOleDoc)
{
    m_pOleDoc                       = pOleDoc;
    m_pHost                         = NULL;
}

BootStrapper::~BootStrapper()
{
    if (m_pHost)
    {
        m_pHost->Stop();
        ReleaseInterface(m_pHost);
    }
    m_pOleDoc = NULL;
}

//******************************************************************************
//
//   BootStrapper::AvalonExecute()
//
//******************************************************************************

void BootStrapper::AvalonExecute(__in_opt IStream *loadStream /*=NULL*/)
{
    HRESULT hr = S_OK;
   
    // Interface pointers
    IUnknown            *pAppDomainPunk     = NULL;
    _AppDomain          *pDefaultDomain     = NULL;

    // Application pointers
    _ObjectHandle       *pObjHandle         = NULL;
    VARIANT              v                  = {0};

    // other pointers
    BSTR                 bstrAssembly       = NULL;
    BSTR                 bstrType           = NULL;

    // CLR hosting interfaces
    ICLRMetaHostPolicy  *pMetaHostPolicy    = NULL;
    ICLRRuntimeInfo     *pRuntimeInfo       = NULL;
    ICLRRuntimeHost     *pHost              = NULL;
    ICLRControl         *pCLRControl        = NULL;
    ICLRDomainManager   *pCLRDomainManager  = NULL;

    // Configuration properties for the default AppDomain
    // 
    // Currently we set PARTIAL_TRUST_VISIBLE_ASSEMBLIES to the empty list, which tells the CLR that no
    // conditionally APTCA assemblies are to be enabled in the default AppDomain.  Since this matches what
    // will be the case in the sandboxed AppDomains that will be spun up, the CLR can use this to optimize
    // away the need for some transitive closure walks when sharing code, and also allows us to share code
    // between the default domain and sandboxed domains.
    LPCWSTR pwszDefaultDomainProperties[] =
    {
        L"PARTIAL_TRUST_VISIBLE_ASSEMBLIES"
    };

    LPCWSTR pwszDefaultDomainPropertyValues[] =
    {
        L""
    };

    // Compiler will optimize the multiple inline calls
    if (m_pOleDoc->GetCurrentMime() != MimeType_Xps && 
        m_pOleDoc->GetCurrentMime() != MimeType_Application &&
        m_pOleDoc->GetCurrentMime() != MimeType_Markup)
    {
        CKHR(E_FAIL);
    }

    bstrAssembly = SysAllocString(L"PresentationFramework, Version=" LVER_ASSEMBLY_MANAGED L", Culture=neutral, PublicKeyToken=" LWCP_PUBLIC_KEY_TOKEN L", Custom=null");
    bstrType     = SysAllocString(L"System.Windows.Interop.DocObjHost");

    CK_ALLOC(bstrAssembly);
    CK_ALLOC(bstrType);
    

    //
    // Start the CLR.
    //
    Assert(m_pHost == NULL);

    // Tracing Note: The shim wraps this function in VersionActivateStart-VersionActivateEnd events.
    // The bulk of the time is spent in starting the CLR. In particular, ICorRuntimeHost::Start()
    // causes PresentationFramework.dll and its dependencies to be loaded. This is because our
    // AppDomain Manager comes from PF.

    EventWriteWpfHostUm_StartingCLRStart();

    // Use of the new CLR 4.0 MetaHost APIs to retrieve the latest CLR version within the 4.0.0 band.
    // The METAHOST_POLICY_APPLY_UPGRADE_POLICY flag is what does the trick.
    CKHR(CLRCreateInstance(CLSID_CLRMetaHostPolicy, IID_ICLRMetaHostPolicy, (LPVOID*)&pMetaHostPolicy));

    wchar_t version[40] = L"v4.0.0";
    DWORD cchVersion = ARRAYSIZE(version);

    CKHR(pMetaHostPolicy->GetRequestedRuntime(METAHOST_POLICY_APPLY_UPGRADE_POLICY,
                                              NULL,                    // assembly path
                                              NULL,                    // configuration file stream
                                              version, &cchVersion,    // preferred CLR version
                                              NULL, NULL,              // image version
                                              NULL,                    // config flags
                                              IID_ICLRRuntimeInfo,
                                              (LPVOID*)&pRuntimeInfo));

    CKHR(pRuntimeInfo->SetDefaultStartupFlags(STARTUP_LOADER_OPTIMIZATION_MULTI_DOMAIN_HOST | STARTUP_CONCURRENT_GC,
                                              NULL /* config file */));
    CKHR(pRuntimeInfo->GetInterface(CLSID_CLRRuntimeHost, IID_ICLRRuntimeHost, (LPVOID*)&pHost));

    CKHR(pHost->GetCLRControl(&pCLRControl));
    CKHR(pCLRControl->GetCLRManager(IID_ICLRDomainManager, (LPVOID*)&pCLRDomainManager));
    CKHR(pCLRDomainManager->SetAppDomainManagerType(bstrAssembly,
                                                    L"System.Windows.Interop.PresentationAppDomainManager",
                                                    eInitializeNewDomainFlags_None));
    CKHR(pCLRDomainManager->SetPropertiesForDefaultAppDomain(_countof(pwszDefaultDomainProperties),
                                                             pwszDefaultDomainProperties,
                                                             pwszDefaultDomainPropertyValues));

    CKHR(pHost->Start());

    CKHR(pRuntimeInfo->GetInterface(CLSID_CorRuntimeHost, IID_ICorRuntimeHost, (LPVOID*)&m_pHost));

    CKHR(m_pHost->GetDefaultDomain(&pAppDomainPunk));

    CKHR(pAppDomainPunk->QueryInterface(__uuidof(_AppDomain), (void**) &pDefaultDomain));
    CK_ALLOC(pDefaultDomain);

    EventWriteWpfHostUm_StartingCLREnd();

    //
    // Load and initialize the application.
    //
    Assert(m_pOleDoc->m_pAppBrowserHostServices == NULL);
    //Load the app object thru interop
    CKHR(pDefaultDomain->CreateInstance(bstrAssembly, bstrType, &pObjHandle));
    pObjHandle->Unwrap(&v);

    CKHR(v.pdispVal->QueryInterface(__uuidof(IBrowserHostServices),(void**) &(m_pOleDoc->m_pAppBrowserHostServices)));

    if (m_pOleDoc->m_pAppBrowserHostServices == NULL)
    {
        CKHR(E_FAIL);
    }
    CKHR(m_pOleDoc->InitApplicationServer());

    // Now we can do additional http web requests through ByteRangeDownloader (managed code)
    if (m_pOleDoc->m_pByteWrapper)
    {
        CKHR(v.pdispVal->QueryInterface(__uuidof(IByteRangeDownloaderService),(void**) &(m_pOleDoc->m_pByteRangeDownloaderService)));

        if (m_pOleDoc->m_pByteRangeDownloaderService == NULL)
        {
            CKHR(E_FAIL);
        }
        //



    }

Cleanup:
    HANDLE_ACTIVATION_FAULT(hr);

    if (pDefaultDomain)
    {
        pAppDomainPunk->Release();
    }

    if (pAppDomainPunk)
    {
        pAppDomainPunk->Release();
    }

    if (pCLRDomainManager)
    {
        pCLRDomainManager->Release();
    }

    if (pCLRControl)
    {
        pCLRControl->Release();
    }

    if (pHost)
    {
        pHost->Release();
    }

    if (pRuntimeInfo)
    {
        pRuntimeInfo->Release();
    }

    if (pMetaHostPolicy)
    {
        pMetaHostPolicy->Release();
    }

    if (bstrAssembly != NULL)
    {
        SysFreeString(bstrAssembly);
    }

    if (bstrType != NULL)
    {
        SysFreeString(bstrType);
    }
}
