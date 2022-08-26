/**************************************************************************\
*
* Copyright (C) 2002 by Microsoft Corporation.  All rights reserved.
*
* Module Name:
*
*   RmSdkWrapper.cpp
*
* Abstract:  
*
*   This unmanaged DLL is build to fool the DRM SDK into thinking that it works 
*   with a sign trusted component 
*
* Revision History:
*
*   07/02/2002 IgorBel
*
*   08/15/2002 LGolding
*       In the DRM SDK drop of 07/17/2002, msdrmc.dll and udmgr.dll have been
*       replaced and combined into msdrm.dll.
*
*   07/14/2005 Igorbel 
*       Added back for the public Managed RM APIs to use 

\**************************************************************************/

#include "Precompiled.hxx"


// This wrapper calls DRM SDK through LoadLibrary and GetProcAddress
// We are not linking in DRM SDK through the LIBs in order to make sure 
// that NDPHOST.dll can be loaded in scenarious where having DRM SDK installed is not a requirement

typedef HRESULT (*DRM_INIT_ENVIRONMENT )(
								IN DRMSECURITYPROVIDERTYPE eSecurityProviderType,
								IN DRMSPECTYPE eSpecification,
								__in_opt PWSTR wszSecurityProvider,
								__in_opt PWSTR wszManifestCredentials,
								__in     PWSTR wszMachineCredentials,
								OUT DRMENVHANDLE* phEnv,
								OUT DRMHANDLE* phDefaultLibrary);

typedef HRESULT (* DRM_CREATE_BOUND_LICENSE)(
								IN DRMENVHANDLE hEnv,
								IN DRMBOUNDLICENSEPARAMS* pParams,
								__in PWSTR wszLicenseChain,
								OUT DRMHANDLE* phBoundLicense,
								OUT DRMHANDLE* phErrorLog);

typedef HRESULT (* DRM_CREATE_LICENSE_STORAGE_SESSION)(
								IN  DRMENVHANDLE hEnv,               // Environment Handle ( o/p of DRMInitEnvironment)
								IN  DRMHANDLE    hDefaultLibrary,    // Default Library Handle (o/p of DRMInitEnvironment)
								IN  DRMHSESSION  hClient,            // Client session
								IN  UINT         uFlags,             // Reserved
								__in  PWSTR        wszIssuanceLicense, // IssuanceLicense
								OUT DRMHSESSION* phLicenseStorage);

typedef HRESULT (*DRM_CREATE_ENABLING_BITS_DECRYPTOR)(
								IN DRMHANDLE hBoundLicense,
								__in_opt PWSTR wszRight,
								IN DRMHANDLE hAuxLib,
								__in_opt PWSTR wszAuxPlug,
								OUT DRMHANDLE* phDecryptor);

typedef HRESULT (* DRM_CREATE_ENABLING_BITS_ENCRYPTOR)(
								IN DRMHANDLE hBoundLicense,
								__in_opt PWSTR wszRight,
								IN DRMHANDLE hAuxLib,
								__in_opt PWSTR wszAuxPlug,
								OUT DRMHANDLE* phEncryptor);

typedef HRESULT (* DRM_ENCRYPT)(
								IN DRMHANDLE hCryptoProvider,
								IN UINT iPosition,
								IN UINT cNumInBytes,
								IN BYTE* pbInData,
								IN OUT UINT* pcNumOutBytes,
								OUT BYTE* pbOutData);
					
typedef HRESULT (* DRM_DECRYPT)(
								IN DRMHANDLE hCryptoProvider,
								IN UINT iPosition,
								IN UINT cNumInBytes,
								IN BYTE* pbInData,
								IN OUT UINT* pcNumOutBytes,
								OUT BYTE* pbOutData);

typedef HRESULT (* DRM_GET_INFO)(
								__in DRMHANDLE handle,
								__in PWSTR wszAttribute,
								__in DRMENCODINGTYPE* peEncoding,
								__inout UINT* pcBuffer,
								__out_ecount(*pcBuffer) BYTE* pbBuffer);

typedef HRESULT (* DRM_GET_BOUND_LICENSE_OBJECT_COUNT)(
								IN DRMHANDLE hQueryRoot,
								__in PWSTR wszSubObjectType,
								OUT UINT* pcSubObjects);

typedef HRESULT (* DRM_GET_BOUND_LICENSE_OBJECT)(
								IN DRMHANDLE hQueryRoot,
								__in PWSTR wszSubObjectType,
								IN UINT iWhich,
								OUT DRMHANDLE* phSubObject);

typedef HRESULT (* DRM_GET_BOUND_LICENSE_ATTRIBUTE)(
								IN DRMHANDLE hQueryRoot,
								__in PWSTR wszAttribute,
								IN UINT iWhich,
								OUT DRMENCODINGTYPE* peEncoding,
								IN OUT UINT* pcBuffer,
								OUT BYTE* pbBuffer);

typedef HRESULT (*  DRM_GET_SIGNED_ISSUANCE_LICENSE)(
								IN      DRMENVHANDLE hEnv,//Optional.Mandatory for DRM_SIGN_OFFLINE
								IN      DRMPUBHANDLE hIssuanceLicense,
								IN      UINT         uFlags,//DRM_SIGN_ONLINE/DRM_SIGN_OFFLINE/DRM_SIGN_CANCEL
								IN      BYTE*        pbSymKey,
								IN      UINT         cbSymKey,
								__in_opt      PWSTR        wszSymKeyType,
								__in_opt      PWSTR        wszClientLicensorCertificate,//Should be NULL for DRM_SIGN_ONLINE , not NULL otherwise
								IN      DRMCALLBACK  pfnCallback,
								__in_opt      PWSTR        wszURL,//Mandatory if uFlags is DRM_SIGN_ONLINE
								IN      VOID*        pvContext);//Optional

typedef HRESULT (* DRM_GET_SERVICE_LOCATION)(
								IN    DRMHSESSION    hClient,            // Client session
								IN    UINT           uServiceType,       //One of DRM_SERVICE_TYPE
								IN    UINT           uServiceLocation,   //One of DRM_SERVICE_LOCATION
								__in_opt    PWSTR          wszIssuanceLicense, //Optional
								IN OUT UINT*         puServiceURLLength,
								__out_ecount_opt(*puServiceURLLength)   PWSTR          wszServiceURL);

typedef HRESULT (* DRM_CLOSE_ENVIRONMENT_HANDLE)(
                                IN DRMENVHANDLE hEnv);

typedef HRESULT (* DRM_CLOSE_HANDLE)(
                                IN DRMHANDLE hEnv);


HINSTANCE hmsdrmDLL = NULL;               // Handle to DLL

DRM_INIT_ENVIRONMENT pfn_DRMInitEnvironment = NULL;    // Function pointer
DRM_CREATE_BOUND_LICENSE pfn_DRMCreateBoundLicense = NULL; // Function pointer
DRM_CREATE_LICENSE_STORAGE_SESSION pfn_DRMCreateLicenseStorageSession = NULL; // Function pointer
DRM_CREATE_ENABLING_BITS_DECRYPTOR pfn_DRMCreateEnablingBitsDecryptor = NULL;
DRM_CREATE_ENABLING_BITS_ENCRYPTOR pfn_DRMCreateEnablingBitsEncryptor = NULL;
DRM_ENCRYPT pfn_DRMEncrypt = NULL;
DRM_DECRYPT pfn_DRMDecrypt = NULL;
DRM_GET_INFO pfn_DRMGetInfo = NULL;
DRM_GET_BOUND_LICENSE_OBJECT_COUNT pfn_DRMGetBoundLicenseObjectCount = NULL;
DRM_GET_BOUND_LICENSE_OBJECT  pfn_DRMGetBoundLicenseObject = NULL;
DRM_GET_BOUND_LICENSE_ATTRIBUTE  pfn_DRMGetBoundLicenseAttribute = NULL;
DRM_GET_SIGNED_ISSUANCE_LICENSE pfn_DRMGetSignedIssuanceLicense = NULL;
DRM_GET_SERVICE_LOCATION pfn_DRMGetServiceLocation = NULL; 
DRM_CLOSE_ENVIRONMENT_HANDLE pfn_DRMCloseEnvironmentHandle = NULL; 
DRM_CLOSE_HANDLE pfn_DRMCloseHandle = NULL;

void EnsureLoadLibrary()
{
    if (!hmsdrmDLL )
    {
        hmsdrmDLL  = LoadLibrary(L"msdrm.dll");
    }
	
    if (hmsdrmDLL)
    {
        pfn_DRMInitEnvironment = (DRM_INIT_ENVIRONMENT)GetProcAddress(hmsdrmDLL, "DRMInitEnvironment");

        pfn_DRMCreateBoundLicense = (DRM_CREATE_BOUND_LICENSE)GetProcAddress(hmsdrmDLL, "DRMCreateBoundLicense");

        pfn_DRMCreateEnablingBitsDecryptor = (DRM_CREATE_ENABLING_BITS_DECRYPTOR)GetProcAddress(hmsdrmDLL, "DRMCreateEnablingBitsDecryptor");
    
        pfn_DRMCreateEnablingBitsEncryptor = (DRM_CREATE_ENABLING_BITS_ENCRYPTOR)GetProcAddress(hmsdrmDLL, "DRMCreateEnablingBitsEncryptor");
            
        pfn_DRMEncrypt = (DRM_ENCRYPT)GetProcAddress(hmsdrmDLL, "DRMEncrypt");
    
        pfn_DRMDecrypt = (DRM_DECRYPT)GetProcAddress(hmsdrmDLL, "DRMDecrypt");

        pfn_DRMGetInfo = (DRM_GET_INFO)GetProcAddress(hmsdrmDLL, "DRMGetInfo");
    
        pfn_DRMCreateLicenseStorageSession = (DRM_CREATE_LICENSE_STORAGE_SESSION)GetProcAddress(hmsdrmDLL, "DRMCreateLicenseStorageSession");

        pfn_DRMGetBoundLicenseObjectCount = (DRM_GET_BOUND_LICENSE_OBJECT_COUNT)GetProcAddress(hmsdrmDLL, "DRMGetBoundLicenseObjectCount");

        pfn_DRMGetBoundLicenseObject = (DRM_GET_BOUND_LICENSE_OBJECT)GetProcAddress(hmsdrmDLL, "DRMGetBoundLicenseObject");

        pfn_DRMGetBoundLicenseAttribute = (DRM_GET_BOUND_LICENSE_ATTRIBUTE)GetProcAddress(hmsdrmDLL, "DRMGetBoundLicenseAttribute");

        pfn_DRMGetSignedIssuanceLicense = (DRM_GET_SIGNED_ISSUANCE_LICENSE)GetProcAddress(hmsdrmDLL, "DRMGetSignedIssuanceLicense");

        pfn_DRMGetServiceLocation = (DRM_GET_SERVICE_LOCATION)GetProcAddress(hmsdrmDLL, "DRMGetServiceLocation");
    
        pfn_DRMCloseEnvironmentHandle = (DRM_CLOSE_ENVIRONMENT_HANDLE)GetProcAddress(hmsdrmDLL, "DRMCloseEnvironmentHandle");

        pfn_DRMCloseHandle = (DRM_CLOSE_HANDLE)GetProcAddress(hmsdrmDLL, "DRMCloseHandle");
    }
}

// turn off optimization for all retail and debug builds, in order to satidfy DRM BB stalk walking requirements 
// without this options DRM BB fails on retail builds  
#pragma optimize( "g", off ) 
HRESULT UDAPICALL DRMInitEnvironment (
								IN DRMSECURITYPROVIDERTYPE eSecurityProviderType,
								IN DRMSPECTYPE eSpecification,
								__in_opt PWSTR wszSecurityProvider,
								__in_opt PWSTR wszManifestCredentials,
								__in     PWSTR wszMachineCredentials,
								OUT DRMENVHANDLE* phEnv,
								OUT DRMHANDLE* phDefaultLibrary)
{
    EnsureLoadLibrary();
    
    if (!pfn_DRMInitEnvironment)
    {
        return E_FAIL;
    }

    return pfn_DRMInitEnvironment (
                                                    eSecurityProviderType,
                                                    eSpecification,
                                                    wszSecurityProvider,
                                                    wszManifestCredentials,
                                                    wszMachineCredentials,
                                                    phEnv,
                                                    phDefaultLibrary);
}

HRESULT UDAPICALL DRMCreateBoundLicense (
					                    IN DRMENVHANDLE hEnv,
					                    IN DRMBOUNDLICENSEPARAMS* pParams,
					                    __in PWSTR wszLicenseChain,
					                    OUT DRMHANDLE* phBoundLicense,
					                    OUT DRMHANDLE* phErrorLog)
{
    EnsureLoadLibrary();
    if (!pfn_DRMCreateBoundLicense)
    {
        return E_FAIL;
    }

    return pfn_DRMCreateBoundLicense (
                                                    hEnv,
                                                    pParams,
                                                    wszLicenseChain,
                                                    phBoundLicense,
                                                    phErrorLog); //phErrorLog must be NULL / reserved 
}

HRESULT UDAPICALL DRMCreateLicenseStorageSession(
								IN  DRMENVHANDLE hEnv,               // Environment Handle ( o/p of DRMInitEnvironment)
								IN  DRMHANDLE    hDefaultLibrary,    // Default Library Handle (o/p of DRMInitEnvironment)
								IN  DRMHSESSION  hClient,            // Client session
								IN  UINT         uFlags,             // Reserved
								__in  PWSTR        wszIssuanceLicense, // IssuanceLicense
								OUT DRMHSESSION* phLicenseStorage)
{
    EnsureLoadLibrary();
    if (!pfn_DRMCreateLicenseStorageSession)
    {
        return E_FAIL;
    }

    return pfn_DRMCreateLicenseStorageSession(
                                                    hEnv,               // Environment Handle ( o/p of DRMInitEnvironment)
                                                    hDefaultLibrary,    // Default Library Handle (o/p of DRMInitEnvironment)
                                                    hClient,            // Client session
                                                    uFlags,             // Reserved
                                                    wszIssuanceLicense, // IssuanceLicense
                                                    phLicenseStorage);
}

HRESULT UDAPICALL DRMCreateEnablingBitsDecryptor(
								IN DRMHANDLE hBoundLicense,
								__in_opt PWSTR wszRight,
								IN DRMHANDLE hAuxLib,
								__in_opt PWSTR wszAuxPlug,
								OUT DRMHANDLE* phDecryptor)
{
    EnsureLoadLibrary();
    if (!pfn_DRMCreateEnablingBitsDecryptor)
    {
        return E_FAIL;
    }

    return pfn_DRMCreateEnablingBitsDecryptor(
                                                    hBoundLicense,
                                                    wszRight,
                                                    hAuxLib,
                                                    wszAuxPlug,
                                                    phDecryptor);
}

HRESULT UDAPICALL DRMCreateEnablingBitsEncryptor(
								IN DRMHANDLE hBoundLicense,
								__in_opt PWSTR wszRight,
								IN DRMHANDLE hAuxLib,
								__in_opt PWSTR wszAuxPlug,
								OUT DRMHANDLE* phEncryptor)
{
    EnsureLoadLibrary();
    if (!pfn_DRMCreateEnablingBitsEncryptor)
    {
        return E_FAIL;
    }

    return pfn_DRMCreateEnablingBitsEncryptor(
                                                    hBoundLicense,
                                                    wszRight,
                                                    hAuxLib,
                                                    wszAuxPlug, 
                                                    phEncryptor);
}

HRESULT UDAPICALL DRMEncrypt(
								IN DRMHANDLE hCryptoProvider,
								IN UINT iPosition,
								IN UINT cNumInBytes,
								IN BYTE* pbInData,
								IN OUT UINT* pcNumOutBytes,
								OUT BYTE* pbOutData)
{
    EnsureLoadLibrary();
    if (!pfn_DRMEncrypt)
    {
        return E_FAIL;
    }

    return pfn_DRMEncrypt(
                                                    hCryptoProvider,
                                                    iPosition,
                                                    cNumInBytes,
                                                    pbInData,
                                                    pcNumOutBytes,
                                                    pbOutData);
}

HRESULT UDAPICALL DRMDecrypt(
								IN DRMHANDLE hCryptoProvider,
								IN UINT iPosition,
								IN UINT cNumInBytes,
								IN BYTE* pbInData,
								IN OUT UINT* pcNumOutBytes,
								OUT BYTE* pbOutData)
{
    EnsureLoadLibrary();
    if (!pfn_DRMDecrypt)
    {
        return E_FAIL;
    }

    return pfn_DRMDecrypt(
                                                    hCryptoProvider,
                                                    iPosition,
                                                    cNumInBytes,
                                                    pbInData,
                                                    pcNumOutBytes,
                                                    pbOutData);
}

HRESULT UDAPICALL DRMGetInfo(
								__in DRMHANDLE handle,
								__in PWSTR wszAttribute,
								__in DRMENCODINGTYPE* peEncoding,
								__inout UINT* pcBuffer,
								__out_ecount(*pcBuffer) BYTE* pbBuffer)
{
    EnsureLoadLibrary();
    if (!pfn_DRMGetInfo)
    {
        return E_FAIL;
    }

    return pfn_DRMGetInfo(
                                                    handle,
                                                    wszAttribute,
                                                    peEncoding,
                                                    pcBuffer,
                                                    pbBuffer);
}

HRESULT UDAPICALL DRMGetBoundLicenseObjectCount(
								IN DRMHANDLE hQueryRoot,
								__in PWSTR wszSubObjectType,
								OUT UINT* pcSubObjects)
{
    EnsureLoadLibrary();
    if (!pfn_DRMGetBoundLicenseObjectCount)
    {
        return E_FAIL;
    }

    return pfn_DRMGetBoundLicenseObjectCount(
                                                    hQueryRoot,
                                                    wszSubObjectType,
                                                    pcSubObjects);
}

HRESULT UDAPICALL DRMGetBoundLicenseObject(
								IN DRMHANDLE hQueryRoot,
								__in PWSTR wszSubObjectType,
								IN UINT iWhich,
								OUT DRMHANDLE* phSubObject)
{
    EnsureLoadLibrary();
    if (!pfn_DRMGetBoundLicenseObject)
    {
        return E_FAIL;
    }

    return pfn_DRMGetBoundLicenseObject(
                                                    hQueryRoot,
                                                    wszSubObjectType,
                                                    iWhich,
                                                    phSubObject);
}

HRESULT UDAPICALL DRMGetBoundLicenseAttribute(
								IN DRMHANDLE hQueryRoot,
								__in PWSTR wszAttribute,
								IN UINT iWhich,
								OUT DRMENCODINGTYPE* peEncoding,
								IN OUT UINT* pcBuffer,
								OUT BYTE* pbBuffer)
{
    EnsureLoadLibrary();
    if (!pfn_DRMGetBoundLicenseAttribute)
    {
        return E_FAIL;
    }

    return pfn_DRMGetBoundLicenseAttribute(
                                                    hQueryRoot,
                                                    wszAttribute,
                                                    iWhich,
                                                    peEncoding,
                                                    pcBuffer,
                                                    pbBuffer);
}

HRESULT UDAPICALL DRMGetSignedIssuanceLicense(
								IN      DRMENVHANDLE hEnv,//Optional.Mandatory for DRM_SIGN_OFFLINE
								IN      DRMPUBHANDLE hIssuanceLicense,
								IN      UINT         uFlags,//DRM_SIGN_ONLINE/DRM_SIGN_OFFLINE/DRM_SIGN_CANCEL

								IN      BYTE*        pbSymKey,
								IN      UINT         cbSymKey,
								__in_opt      PWSTR        wszSymKeyType,

								__in_opt      PWSTR        wszClientLicensorCertificate,//Should be NULL for DRM_SIGN_ONLINE , not NULL otherwise
								IN      DRMCALLBACK  pfnCallback,
								__in_opt      PWSTR        wszURL,//Mandatory if uFlags is DRM_SIGN_ONLINE

								IN      VOID*        pvContext) //Optional
{
    EnsureLoadLibrary();
    if (!pfn_DRMGetSignedIssuanceLicense)
    {
        return E_FAIL;
    }

    return pfn_DRMGetSignedIssuanceLicense(
                                                    hEnv, 
                                                    hIssuanceLicense,
                                                    uFlags,         
                                                    pbSymKey,
                                                    cbSymKey,
                                                    wszSymKeyType,
                                                    wszClientLicensorCertificate, 
                                                    pfnCallback,
                                                    wszURL,   
                                                    pvContext);
}

HRESULT UDAPICALL DRMGetServiceLocation(
								IN    DRMHSESSION    hClient,            // Client session
								IN    UINT           uServiceType,       //One of DRM_SERVICE_TYPE
								IN    UINT           uServiceLocation,   //One of DRM_SERVICE_LOCATION
								__in_opt    PWSTR          wszIssuanceLicense, //Optional
								IN OUT UINT*         puServiceURLLength,
								__out_ecount_opt(*puServiceURLLength)   PWSTR          wszServiceURL)
{
    EnsureLoadLibrary();
    if (!pfn_DRMGetServiceLocation)
    {
        return E_FAIL;
    }

    return pfn_DRMGetServiceLocation(
                                                    hClient,
                                                    uServiceType,
                                                    uServiceLocation,
                                                    wszIssuanceLicense,
                                                    puServiceURLLength, 
                                                    wszServiceURL);
}

HRESULT UDAPICALL DRMCloseEnvironmentHandle(
                                                    IN DRMENVHANDLE hEnv)
{
    EnsureLoadLibrary();
    if (!pfn_DRMCloseEnvironmentHandle)
    {
        return E_FAIL;
    }

    return pfn_DRMCloseEnvironmentHandle(
                                                    hEnv);
}

HRESULT UDAPICALL DRMCloseHandle(
                                                    IN DRMHANDLE hEnv)
{
    EnsureLoadLibrary();
    if (!pfn_DRMCloseHandle)
    {
        return E_FAIL;
    }

    return pfn_DRMCloseHandle(
                                                    hEnv);
}



