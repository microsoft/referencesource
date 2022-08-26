#include "snipch.hpp"
#include <windows.h>
#include <assert.h>
#include <msdasc.h>
#include <corerror.h>
#include <mscoree.h>
#include "sni_FedAuth.hpp"

struct ADALFunctionTable g_ADAL = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
static LONG g_cSNIFedAuthInitialized = 0;

// Reuse the same critical section as is used to initialized Windows Auth libraries - odds of contention
// (and actual impact of contention) are both low, so this shouldn't really present a problem.
extern SNICritSec *g_csSecPackageInitialize;

//----------------------------------------------------------------------------
// NAME: SNISecInitFedAuth
//
// PURPOSE:
//      One-time initialization for the underlying Fed Auth library
//      1) Locates the ADAL registry key and finds the ADAL install location from it.
//      2) Loads the ADAL DLL and function pointers.
// 
// PARAMETERS:
//
// RETURNS:
//      ERROR_SUCCESS on success, other value on failure (either Win32 error)
//
// NOTES:
//      SNI error codes from failures of this function are very important, since the 
//      ADAL library we depend on is installed separately from the installer for the SNI 
//      consumer (e.g., SNAC). SNIE_58 should be used whenever it appears that the 
//      ADAL library is *not installed at all* (e.g., top-level registry key is simply not 
//      present), whereas SNIE_59 should be used if there are some signs that ADAL 
//      is installed, but it appears to be corrupted/invalid/etc (e.g., top-level registry 
//      key is present, but registry values are missing, have unexpected values, etc.,).
//----------------------------------------------------------------------------
DWORD SNISecADALInitialize()
{
    DWORD dwError = ERROR_FAIL;
    HKEY hADALKey = nullptr;

    {
        CAutoSNICritSec a_csInitialize(g_csSecPackageInitialize, SNI_AUTOCS_ENTER);

        #define ADAL_DLL_REG_KEY_PATH "Software\\Microsoft\\MSADALSQL"
        #define ADAL_DLL_REG_KEY "TargetDir"

        // the folder + the DLL name have to fit in MAX_PATH. Note that cbADALPathBufferSize here includes space for the null terminator
        // when retrieving from the registry, but the registry API doesn't guarantee null termination. A correctly-formatted registry
        // entry will have a null terminator, though.
        CHAR szADALPath[MAX_PATH] = "";
        DWORD cbADALPathBufferSize = (sizeof(szADALPath[0]) * (MAX_PATH + 1));
        DWORD dwType = 0;

        // If we've already been initialized, increment the refcount and succeed fast.
        if (g_cSNIFedAuthInitialized > 0)
        {
            g_cSNIFedAuthInitialized++;
            BidTraceU0(SNI_BID_TRACE_ON, SNI_TAG _T("Active Directory Authentication Library for SQL Server is already fully initialized.\n"));
            dwError = ERROR_SUCCESS;
            goto Exit;
        }


        // Otherwise, we must start by finding the full path to the ADAL DLL
        dwError = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            ADAL_DLL_REG_KEY_PATH,
            0,
            KEY_READ,
            &hADALKey);
        if (ERROR_SUCCESS != dwError)
        {
            BidTrace1(ERROR_TAG _T("RegOpenKeyEx for Active Directory Authentication Library for SQL Server registry key failed: %u{WINERR}\n"), dwError);
            SNI_SET_LAST_ERROR(INVALID_PROV,
                SNIE_58,
                dwError);
            goto Exit;
        }

        dwError = RegQueryValueExA(hADALKey,
            ADAL_DLL_REG_KEY,
            0,
            &dwType,
            (BYTE*)szADALPath,
            &cbADALPathBufferSize);

        if (ERROR_SUCCESS != dwError)
        {
            BidTrace1(ERROR_TAG _T("RegQueryValueEx for Active Directory Authentication Library for SQL Server location failed: %u{WINERR}\n"), dwError);
            SNI_SET_LAST_ERROR(INVALID_PROV, SNIE_58, dwError);
            goto Exit;
        }

        if (REG_SZ != dwType && REG_EXPAND_SZ != dwType)
        {
            dwError = ERROR_INVALID_DATA;
            SNI_SET_LAST_ERROR(INVALID_PROV, SNIE_59, dwError);
            BidTrace1(ERROR_TAG _T("RegQueryValueEx for Active Directory Authentication Library for SQL Server location returned unexpected type: %u{DWORD}\n"), dwError);
            goto Exit;
        }

        // The RegQueryValueEx API does not guarantee null termination, so we must null terminate it
        // ourselves to avoid potential overrun. A correctly-installed ADAL would have written a null
        // terminator, though, so we should overwrite the last character in the registry data with the null.
        szADALPath[cbADALPathBufferSize - 1] = '\0';

        g_ADAL.hDll = LoadLibraryA(szADALPath);
        if (nullptr == g_ADAL.hDll)
        {
            dwError = GetLastError();
            SNI_SET_LAST_ERROR(INVALID_PROV, SNIE_60, dwError);
            BidTrace1(ERROR_TAG _T("Loading Active Directory Authentication Library for SQL Server failed: %u{WINERR}\n"), dwError);
            goto Exit;
        }

        LOAD_ADAL_FUNCTION(PFADALCreateAuthenticationContextNoUI, ADALCreateAuthenticationContextNoUI);
        LOAD_ADAL_FUNCTION(PFADALSetOption, ADALSetOption);
        LOAD_ADAL_FUNCTION(PFADALAcquireToken, ADALAcquireToken);
        LOAD_ADAL_FUNCTION(PFADALGetRequestStatus, ADALGetRequestStatus);
        LOAD_ADAL_FUNCTION(PFADALUseUsernamePassword, ADALUseUsernamePassword);
        LOAD_ADAL_FUNCTION(PFADALUseWindowsAuthentication, ADALUseWindowsAuthentication);
        LOAD_ADAL_FUNCTION(PFADALGetAccessToken, ADALGetAccessToken);
        LOAD_ADAL_FUNCTION(PFADALGetErrorDescription, ADALGetErrorDescription);
        LOAD_ADAL_FUNCTION(PFADALGetErrorCode, ADALGetErrorCode);
        LOAD_ADAL_FUNCTION(PFADALDeleteRequest, ADALDeleteRequest);
        LOAD_ADAL_FUNCTION(PFADALReleaseAuthenticationContext, ADALReleaseAuthenticationContext);
        LOAD_ADAL_FUNCTION(PFADALGetAccessTokenExpirationTime, ADALGetAccessTokenExpirationTime);

        g_cSNIFedAuthInitialized++;
        dwError = ERROR_SUCCESS;
    }

Exit:

    // Regardless of success or failure, we're done with the ADAL registry key, if we opened it.
    if (nullptr != hADALKey)
    {
        LONG lTemporaryError = RegCloseKey(hADALKey);
        if (ERROR_SUCCESS != lTemporaryError)
        {
            BidTrace1(ERROR_TAG _T("RegCloseKey(hADALKey) failed: %u{WINERR}\n"), lTemporaryError);
        }
        hADALKey = nullptr;
    }

    // In failure case, roll back partial initialization.
    if (ERROR_SUCCESS != dwError)
    {
        if (nullptr != g_ADAL.hDll)
        {
            if (0 == FreeLibrary(g_ADAL.hDll))
            {
                DWORD dwTemp = GetLastError();
                BidTrace1(ERROR_TAG _T("FreeLibrary for Active Directory Authentication Library for SQL Server handle: %u{WINERR}\n"), dwTemp);
            }
            g_ADAL.hDll = nullptr;
        }
    }

    BidTraceU1(SNI_BID_TRACE_ON, RETURN_TAG _T("%x\n"), dwError);
    return dwError;
 }

BOOL IsTransientError(DWORD adalErrorCode)
{
    return ERROR_SUCCESS != adalErrorCode && (
        ERROR_ADAL_INTERNET_FORCE_RETRY == adalErrorCode
        || ERROR_ADAL_SERVER_ERROR_TEMPORARILY_UNAVAILABLE == adalErrorCode
        || ERROR_HTTP_STATUS_RETRY_WITH == adalErrorCode
        || ERROR_HTTP_STATUS_SERVICE_UNAVAIL == adalErrorCode);
}

//----------------------------------------------------------------------------
// NAME: SNISecADALGetAccessToken
//
// PURPOSE:
//      Get access token by calling ADAL function loaded in SNISecADALInitialize
// 
// PARAMETERS:
//      LPCWSTR userName
//              AAD username
//      LPCWSTR password
//              AAD user's password
//      LPCWSTR stsURL
//              STS URL
//      LPCWSTR resource
//              Target resource SPN. Here it is SQL Azure SPN
//      GUID&   correlationId
//              correlation id sent to the server
//      LPCWSTR 
//              clientId
//      const bool& fWindowsIntegrated
//              If we are to perform windows integrated authentication
// RETURNS:
//      ERROR_SUCCESS on success, other value on failure (either Win32 error)
//      __deref_opt_out_bcount(*pcbToken) unsigned char **ppbToken
//              The access token get from AAD
//      DWORD& cbToken
//              The length of access token in char
//      _deref_opt_out_ecount LPWSTR(*pcsErrorDescription) *ppsErrorDescription
//              The error description got from AAD
//      DWORD& csErrorDescription
//              The length of error description in char
//      DWORD& pAdalStatus
//              Public facin failing status returned from Adal APIs in SNISecADALGetAccessToken
//      DWORD& state
//              Internal last Adal API called
//      _FILETIME& fileTime
//              The expiration time for the token in FILETIME format.
//----------------------------------------------------------------------------
DWORD SNISecADALGetAccessToken( __in LPCWSTR username,
                                __in LPCWSTR password,
                                __in LPCWSTR stsURL,
                                __in LPCWSTR resource,
                                __in GUID& correlationId,
                                __in LPCWSTR clientId,
                                __in const bool& fWindowsIntegrated,
                                __deref_opt_out_bcount(cbToken) LPWSTR *ppbToken,
                                __out_opt DWORD& cbToken,
                                __deref_opt_out_ecount(csErrorDescription) LPWSTR *ppsErrorDescription,
                                __out_opt DWORD& csErrorDescription,
                                __out DWORD& adalStatus,
                                __out DWORD& state,
                                __out_opt _FILETIME& fileTime)
{
    assert(username != nullptr || fWindowsIntegrated);
    assert(password != nullptr || fWindowsIntegrated);
    assert(stsURL != nullptr);
    assert(resource != nullptr);
    assert(clientId != nullptr);
    assert(ppbToken != nullptr);
    assert(ppsErrorDescription != nullptr);
    
    HADALCONTEXT hContext = nullptr;
    HADALREQUEST hRequest = nullptr;

    bool fToCoUninitializeCOM = false;

    DWORD status = 0;
    cbToken = 0;
    csErrorDescription = 0;
    
    *ppbToken = nullptr;
    *ppsErrorDescription = nullptr;
    
    adalStatus = ERROR_FAIL;
    state = ADALState::Default;

    // Initialize COM, needed for username/password based Federated Authentication and Integrated Windows Authentication.
    // 
    HRESULT hr = CoInitializeEx(NULL /*pvReserved*/, COINIT_MULTITHREADED /*dwCoInit*/);
    state = ADALState::ADALCoInitializeEx;

    if (S_OK == hr || S_FALSE == hr)
    {
        fToCoUninitializeCOM = true;
    }
    else if (RPC_E_CHANGED_MODE != hr)
    {
        status = hr;
        BidTrace1(ERROR_TAG _T("CoInitializeEx failed to initialize COM with Error Code HRESULT = %u.\n"), hr);
        goto Exit;
    }

    // Using non-interactive flow. So calling ADALCreateAuthenticationContextNoUI.
    hContext = g_ADAL.ADALCreateAuthenticationContextNoUI(stsURL, clientId);
    state = ADALState::ADALCreateAuthenticationContextNoUI;

    if (nullptr == hContext)
    {
        status = GetLastError();
        BidTrace1(ERROR_TAG _T("ADALCreateAuthenticationContextNoUI failed to create hContext: %u.\n"), status);
        goto Exit; // Cannot get error description. Goto Exit.
    }

    bool setOptionResult = g_ADAL.ADALSetOption(hContext, AdalOption::EndpointValidationService, AdalOptionValue::Allow);
    state = ADALState::ADALSetOption;

    assert(setOptionResult != FALSE);

    bool setOptionWamResult = g_ADAL.ADALSetOption(hContext, AdalOption::ADAL_OPTION_USE_WAM, AdalOptionValue::Disallow);
    state = ADALState::ADALSetOptionUseWam;

    assert(setOptionWamResult != FALSE);

    // Obtain request handle. Each call to ADALAcquireToken will return new request handle that you need to release by calling ADALDeleteRequest.
    hRequest = g_ADAL.ADALAcquireToken(hContext, resource, &correlationId);
    state = ADALState::ADALAcquireToken;

    if (nullptr == hRequest)
    {
        status = GetLastError();
        BidTrace1(ERROR_TAG _T("ADALAcquireToken failed to create hRequest status: %u.\n"), status);
        goto Exit; // Cannot get error description. Goto Exit.
    }

    status = g_ADAL.ADALGetRequestStatus(hRequest);
    state = ADALState::ADALGetRequestStatusForAcquireToken;

    // Check the status of ADALAcquireToken must be ERROR_ADAL_NEED_CREDENTIAL
    if (ERROR_ADAL_NEED_CREDENTIAL == status)
    {
        if (!fWindowsIntegrated)
        {
            // User can call ADALUseUsernamePassword to use username and password
            g_ADAL.ADALUseUsernamePassword(hRequest, username, password);
            state = ADALState::ADALUseUsernamePassword;

            status = g_ADAL.ADALGetRequestStatus(hRequest);
            state = ADALState::ADALGetRequestStatusForUsernamePassword;

            BidTrace1(ERROR_TAG _T("ADALUseUsernamePassword return status: %u.\n"), status);
        }
        else
        {
            // Use Windows Integrated Authentication.
            g_ADAL.ADALUseWindowsAuthentication(hRequest);
            state = ADALState::ADALUseWindowsIntegrated;

            status = g_ADAL.ADALGetRequestStatus(hRequest);
            state = ADALState::ADALGetRequestStatusForWindowsIntegrated;

            BidTrace1(ERROR_TAG _T("ADALUseWindowsAuthentication return status: %u.\n"), status);
        }

        if (ERROR_SUCCESS == status) 
        {
            // Get the access token from hRequest
            DWORD tokenLength = 0;
            status = g_ADAL.ADALGetAccessToken(hRequest, nullptr, &tokenLength);
            state = ADALState::ADALGetAccessTokenLength;

            CHECK_ADAL_FUNCTION_RETURN_STATUS_FORLENGTH(status, ADALGetAccessToken);

            assert(tokenLength > 0);

            // Add one space for terminator 
            LPWSTR accessToken = NewNoX(gpmo) wchar_t[tokenLength + 1];
            CHECK_MEM_ALLOCATION(accessToken, ADALGetAccessToken);

            status = g_ADAL.ADALGetAccessToken(hRequest, accessToken, &tokenLength);
            state = ADALState::ADALGetAccessToken;

            BidTrace1(ERROR_TAG _T("ADALGetAccessToken status: %u.\n"), status);

            if (ERROR_SUCCESS != status)
            {
                delete[] accessToken;
                goto Exit; // Cannot get error description. Goto Exit.
            }

            assert(nullptr != accessToken);
            assert(0 == accessToken[tokenLength]);

            SYSTEMTIME expirySystemTime = { 0 };
            bool fGetTimeSuccess = g_ADAL.ADALGetAccessTokenExpirationTime(hRequest, &expirySystemTime);
            state = ADALState::ADALGetAccessTokenExpirationTime;
            if (!fGetTimeSuccess)
            {
                status = GetLastError();
                BidTrace1(ERROR_TAG _T("ADALGetAccessTokenExpirationTime failed status: %u.\n"), status);
                delete[] accessToken;
                goto Exit; // Cannot get error description. Goto Exit.
            }
            else if (!(fGetTimeSuccess = SystemTimeToFileTime(const_cast<SYSTEMTIME*>(&expirySystemTime), &fileTime)))
            {
                status = GetLastError();
                BidTrace1(ERROR_TAG _T("SystemTimeToFileTime failed status: %u.\n"), status);
                delete[] accessToken;
                goto Exit; // Cannot get error description. Goto Exit.
            }

            *ppbToken = accessToken;
            cbToken = (tokenLength + 1) * sizeof(wchar_t);
        }
    }
    else
    {
        if (ERROR_SUCCESS == status)
        {
            // Status shouldn't be ERROR_SUCCESS here. If it is, we cannot get access token, so go to exist.
            // Also can't get error description from a success hRequest. 
            status = ERROR_ADAL_UNEXPECTED;
            BidTrace1(ERROR_TAG _T("ADALAcquireToken returns unexpected status: %u.\n"), status);
            goto Exit; // Cannot get error description. Goto Exit.
        }
        // Other error can get error description
    }

    if (ERROR_SUCCESS != status)
    {
        // Enter here only when we can get valid error description.

        // ADALUseUsernamePassword != ERROR_SUCCESS, (ADALAcquireToken != ERROR_SUCCESS and != ERROR_ADAL_NEED_CREDENTIAL)
        // All the other failures can't get error description, go to exit
        // The error description length could be 1.
        
        DWORD errorDescriptionLength = 0;
     
        DWORD statusGetError = g_ADAL.ADALGetErrorDescription(hRequest, nullptr, &errorDescriptionLength);
        state = ADALState::ADALGetErrorDescriptionLength;

        CHECK_ADAL_FUNCTION_RETURN_STATUS_FORLENGTH(statusGetError, ADALGetErrorDescription);

        assert(errorDescriptionLength >= 0); 

        LPWSTR errorDescription = NewNoX(gpmo) wchar_t[errorDescriptionLength + 1];
        CHECK_MEM_ALLOCATION(errorDescription, ADALGetErrorDescription);

        statusGetError = g_ADAL.ADALGetErrorDescription(hRequest, errorDescription, &errorDescriptionLength);
        state = ADALState::ADALGetErrorDescription;

        if (ERROR_SUCCESS != statusGetError)
        {
            delete[] errorDescription;
            BidTrace1(ERROR_TAG _T("ADALGetErrorDescription failed to get errorDescription: %u.\n"), status);
            goto Exit;
        }

        assert(nullptr != errorDescription);
        assert(0 == errorDescription[errorDescriptionLength]);

        *ppsErrorDescription = errorDescription;
        csErrorDescription = errorDescriptionLength;
    }

Exit:
    // If the workflow successfully performed a CoInitializeEx, perform CoUnInitialize.
    if (fToCoUninitializeCOM)
    {
        CoUninitialize();
    }

    // Release memory
    {
        BOOL isSuccessful = FALSE;
        if (nullptr != hRequest)
        {
            isSuccessful = g_ADAL.ADALDeleteRequest(hRequest);

            CHECK_ADAL_FUNCTION_RETURN_STATUS_BOOL(isSuccessful, "ADALDeleteRequest");

            hRequest = nullptr;
        }
        if (nullptr != hContext)
        {
            isSuccessful = g_ADAL.ADALReleaseAuthenticationContext(hContext);

            CHECK_ADAL_FUNCTION_RETURN_STATUS_BOOL(isSuccessful, "ADALReleaseAuthenticationContext");

            hContext = nullptr;
        }
    }

    // Set error category, status, state
    {
        DWORD result = ErrorCategory::OtherError;

        if (ERROR_SUCCESS == status)
        {
            assert(nullptr != *ppbToken);
            assert(0 < cbToken);

            result = ErrorCategory::Success;
            state = ADALState::Success;
        }
        else if (ERROR_ADAL_SERVER_ERROR_INVALID_GRANT == status)
        {
            result = ErrorCategory::InvalidGrant;
        }
        else if (IsTransientError(status))
        {
            result = ErrorCategory::TransientError;
        }

        adalStatus = status;
        return result;
    }
}
