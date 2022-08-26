//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: sni_sspi.cpp
// @Owner: nantu, petergv
// @Test: milu
//
// <owner current="true" primary="true">nantu</owner>
// <owner current="true" primary="false">petergv</owner>
//
// Purpose:
//
// Notes:
//          
// @EndHeader@
//****************************************************************************

#include "snipch.hpp"
#include "sni_sspi.hpp"
#include "sni_spn.hpp"
#include "tcp.hpp"
#include "SNI_ServiceBindings.hpp"

// maximum # of Buffers that we will require for calling ASC/ISC for Negotiate - 
// one is for the input buffer from the wire, and one is for the Channel Binding Token.
#define NEGOTIATE_BUFFERS_REQUIRED 2

#define SEC_SUCCESS( Status ) ((Status) >= 0)
// Enum for SSPI security package indexes.
typedef enum
{
	NEGOTIATE_SSPI_PACKAGE = 0,
	KERBEROS_SSPI_PACKAGE  = 1,
	NTLM_SSPI_PACKAGE      = 2,
	CSSPKGS                = 3 // Total number of SSPI packages.
} SSPISecurityPackages;

// MSSQLSvc/FQDN:Instance = length("MSSQLSvc/") + NI_MAXHOST + 1 + MAX_INSTANCENAME_LENGTH
// (space for null terminator is included in NI_MAXHOST, per MSDN getnameinfo() example usage and RFC 2553)
const DWORD SNI_MAX_COMPOSED_SPN = 9 + NI_MAXHOST + 1 + MAX_INSTANCENAME_LENGTH;

#define CCHMAXSSP sizeof(_T("negotiate"))

#ifdef UNICODE
#define SEC_TCHAR SEC_WCHAR
#else
#define SEC_TCHAR SEC_CHAR
#endif

#ifndef SNI_BASED_CLIENT
#define NLREGS_ASCII
#include "nlregs.h"
#endif

#ifndef CPL_ASSERT
#define CPL_ASSERT(exp) typedef char __CPL_ASSERT__[(exp)?1:-1]
#endif

// Globals
PSecurityFunctionTable	g_pFuncs = NULL;
_TCHAR *g_rgszSSP[CSSPKGS] = {_T("negotiate"),  _T("kerberos"), _T("ntlm")};
int  g_rgcchSSP[CSSPKGS] = {sizeof(_T("negotiate")), sizeof(_T("kerberos")), sizeof(_T("ntlm"))};
TCHAR g_szSSP[CCHMAXSSP] = {0};

extern ExtendedProtectionPolicy g_eExtendedProtectionLevel;

// Available SSPI packages initialized in SNISecInitPackage.
BOOL g_rgfSSPIPackageAvailable[CSSPKGS] = { false, false, false };

DWORD g_cbMaxToken = 0; 
CredHandle		ghCred;
HINSTANCE		g_hSspiLib = NULL;
extern BOOL		gfIsWin9x;

USHORT g_usFirstIPAllPort = 0;
BOOL g_fSPNInitialized = FALSE;
LONG g_cSNISecPackageInitialized = 0;
SNICritSec *g_csSecPackageInitialize = NULL;		//to serialize initialize/terminate
BOOL g_fSPNRegisterSucceed = FALSE;
static bool s_fServiceBindingsSupported = true;

TCHAR g_szInstanceSPN[SNI_MAX_COMPOSED_SPN] = _T("");

SNI_Sec::SNI_Sec() 
	: m_fCtxt(FALSE),
	  m_PkgId(INVALID),
	  m_pPkgInfo(0),
	  m_dwLastError(ERROR_SUCCESS)
{
	SecInvalidateHandle( &m_hCred );	
}

SNI_Sec::~SNI_Sec()
{
	Assert( !m_fCtxt ); 

	Assert( !SecIsValidHandle( &m_hCred) ); 

	Assert( m_pPkgInfo == NULL);
}

// Deletes structures allocated by SSPI API.  
void SNI_Sec::DeleteSspi()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );

	THREAD_PREEMPTIVE_ON_START (PWAIT_PREEMPTIVE_OS_DELETESECURITYCONTEXT);

	if( m_fCtxt )
	{
		g_pFuncs->DeleteSecurityContext( &m_hCtxt );
		m_fCtxt = FALSE;
	}

	if( SecIsValidHandle( &m_hCred) )
	{
		g_pFuncs->FreeCredentialsHandle(&m_hCred);
		SecInvalidateHandle( &m_hCred );
	}

	if(m_pPkgInfo)
	{
		g_pFuncs->FreeContextBuffer(m_pPkgInfo->PackageInfo );
		delete m_pPkgInfo;
		m_pPkgInfo = NULL;
	}
		
	THREAD_PREEMPTIVE_ON_END
}

DWORD SNI_Sec::SetPkgName()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );
	
	SECURITY_STATUS dwRet = SEC_E_OK;

	if(m_pPkgInfo) 
	{
		g_pFuncs->FreeContextBuffer(m_pPkgInfo->PackageInfo );
		delete m_pPkgInfo;
		m_pPkgInfo = NULL;		
	}

	m_pPkgInfo = NewNoX(gpmo) SecPkgContext_NegotiationInfo;

	if( m_pPkgInfo )
	{	
		ZeroMemory(m_pPkgInfo, sizeof(SecPkgContext_NegotiationInfo));	

		if(SEC_E_OK == (dwRet = g_pFuncs->QueryContextAttributes( &m_hCtxt, 
					  	       SECPKG_ATTR_NEGOTIATION_INFO, 
				  		       m_pPkgInfo)) )
		{
			SetPkgName((LPTSTR)m_pPkgInfo->PackageInfo->Name);
		}
		else
		{
			BidTrace1( ERROR_TAG _T("dwRet: %d\n"), dwRet);
			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwRet );

			delete m_pPkgInfo;		
			m_pPkgInfo = NULL;
		}
	}
	else
	{
		dwRet = ERROR_OUTOFMEMORY;
	}

	m_dwLastError = dwRet;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;
		
}

// Retrieve the SSPI package name used by the security context cached during ISC/ASC.
// See SetPkgName(). 
//
DWORD SNI_Sec::GetSecPkgName(LPTSTR  * ppPkgName)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("ppPkgName: %p{LPTSTR*}\n"), ppPkgName );

	DWORD dwRet = m_dwLastError;
	
	if(ERROR_SUCCESS != dwRet )
	{
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwRet );		
		*ppPkgName = NULL;
	}
	else
	{
		Assert( m_pPkgInfo );
		*ppPkgName = m_pPkgInfo->PackageInfo->Name;
		BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T( "SecPkgName:'%s'\n" ), *ppPkgName);
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;
}

// Retrieve the mutual authentication flags on current SSPI security context. 
//
DWORD SNI_Sec::GetMutualAuth(BOOL* fMutualAuth)
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );

	Assert(fMutualAuth);

	SECURITY_STATUS dwRet = SEC_E_OK;
	SecPkgContext_Flags Flags;
	
	*fMutualAuth = FALSE;

	if( SEC_E_OK == (dwRet = g_pFuncs->QueryContextAttributes( &m_hCtxt, 
			  	       SECPKG_ATTR_FLAGS, 
				       &Flags)))
	{
		*fMutualAuth = ((Flags.Flags & ISC_REQ_MUTUAL_AUTH) != 0);
	}
	else
	{
		BidTrace1( ERROR_TAG _T("dwRet: %d\n"), dwRet);
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwRet );
	}

	BidTraceU2( SNI_BID_TRACE_ON, RETURN_TAG _T("fMutualAuth: %d, %d{WINERR}\n"), *fMutualAuth, dwRet);
	
	return dwRet;
}

DWORD SNI_Sec::CheckServiceBindings(SNIAuthErrStates *pSSPIFailureState)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("pSSPIFailureState: %p{SNIAuthErrStates*}\n"), pSSPIFailureState );
	
	DWORD dwRet = SEC_E_OK;
	// should not even be here unless protection level is at least ALLOWED
	Assert(g_eExtendedProtectionLevel == ExtendedProtectionPolicy_ALLOWED ||
		   g_eExtendedProtectionLevel == ExtendedProtectionPolicy_REQUIRED);

	SecPkgContext_ClientSpecifiedTarget clientTargetSPN = {NULL};
	// If we got the Kerberos package, then there is no need to check Service Bindings.
	if( KERBEROS == m_PkgId )
	{
		BidTraceU0( SNI_BID_TRACE_ON, SNI_TAG _T("Using Kerberos, no need to check Service Bindings\n"));
		dwRet = SEC_E_OK;
		goto Exit;
	}

	// If Service Bindings are not supported and Extended Protection Level > OFF, the Service Binding check must fail.
	if( ! s_fServiceBindingsSupported )
	{
		BidTrace1(ERROR_TAG _T("Extended Protection Level is not OFF and Server OS does not support Service Bindings: 0x%x. Failing authentication.\n"), dwRet);
		*pSSPIFailureState = SNIAUTH_ERRST_SSPIHANDSHAKE_SERVICEBINDINGS_UNSUPPORTED;
		dwRet = SEC_E_BAD_BINDINGS;
		goto Exit;
	}
	
	// Otherwise, we need to verify the target ourselves.
	dwRet = g_pFuncs->QueryContextAttributes(
									&m_hCtxt,
									SECPKG_ATTR_CLIENT_SPECIFIED_TARGET,
									&clientTargetSPN);

	if( SEC_E_UNSUPPORTED_FUNCTION == dwRet )
	{
		// we can skip trying QCA in the future - harmless race condition to set this flag if multiple simultaneous connections hit at startup
		s_fServiceBindingsSupported = false;

		// Log that EP is unsupported and that this is the cause for failure:
		// "Extended Protection for the SQL Server Database Engine is enabled, but the operating system does not support 
		// Extended Protection..."
		scierrlog(26065);

		BidTrace1(ERROR_TAG _T("Extended Protection Level is not OFF and Server OS does not support Service Bindings: 0x%x. Failing authentication.\n"), dwRet);
		*pSSPIFailureState = SNIAUTH_ERRST_SSPIHANDSHAKE_SERVICEBINDINGS_UNSUPPORTED;
		dwRet = SEC_E_BAD_BINDINGS;
		goto Exit;
	}
	else if( SEC_E_TARGET_UNKNOWN == dwRet )
	{
		// SEC_E_TARGET_UNKNOWN indicates the client OS is unpatched (or escape hatched) - REQUIRED => deny access, ALLOWED => grant access.
		if (ExtendedProtectionPolicy_REQUIRED == g_eExtendedProtectionLevel)
		{
			*pSSPIFailureState = SNIAUTH_ERRST_SSPIHANDSHAKE_SERVICEBINDINGS_NULL;
			BidTrace1(ERROR_TAG _T("Extended Protection Level is REQUIRED and Client OS did not send Service Bindings: 0x%x. Failing authentication.\n"), dwRet);
			goto Exit;
		}
		else
		{
			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("Extended Protection Level is ALLOWED and Client OS did not send Service Bindings: 0x%x. Allowing authentication.\n"), dwRet);
			dwRet = SEC_E_OK;
			goto Exit;
		}
	}
	else if( SEC_E_OK == dwRet )
	{
		if( NULL == clientTargetSPN.sTargetName )
		{
			*pSSPIFailureState = SNIAUTH_ERRST_SSPIHANDSHAKE_SERVICEBINDINGS_QUERYCONTEXTATTRIBUTES2;
			// This pointer should always be valid, so this would indicate an SSPI bug. We validate it for our own peace of mind.
			Assert(0 && "QueryContextAttributes for SECPKG_ATTR_CLIENT_SPECIFIED_TARGET returned SEC_E_OK and a NULL sTargetName.");
			BidTrace0(ERROR_TAG _T("QueryContextAttributes for SECPKG_ATTR_CLIENT_SPECIFIED_TARGET returned SEC_E_OK and a NULL sTargetName.\n"));
			dwRet = ERROR_OUTOFMEMORY; // the only reason one can imagine for this to fail...
			goto Exit;
		}
		// Check if client provided empty Service Bindings.
		if( clientTargetSPN.sTargetName[0] == L'\0' )
		{
			// Client OS is patched and client app did not provide a target name. Extended Protection >= ALLOWED, so deny access.
			*pSSPIFailureState = SNIAUTH_ERRST_SSPIHANDSHAKE_SERVICEBINDINGS_EMPTY;
			BidTrace0(ERROR_TAG _T("Client OS sent Service Bindings, but Client app did not provide a target name. Extended Protection level is not OFF. Failing authentication.\n"));
			dwRet = SEC_E_BAD_BINDINGS; 
			goto Exit;
		}
		else
		{
			// Ok, we got an SPN. Match it.
			dwRet = SNI_ServiceBindings::MatchSPN(clientTargetSPN.sTargetName, pSSPIFailureState);
			if( SEC_E_OK != dwRet )
			{
				// SNIAuthErrStates and SNISetLastError already done in SNI_ServiceBindings::MatchSPN
				BidTrace1(ERROR_TAG _T("Client OS sent Service Bindings, Client app provided a target name, but client SPN did not match. Failure state: %d. Denying authentication.\n"), (DWORD) *pSSPIFailureState);
				BidTraceBin(clientTargetSPN.sTargetName, wcslenInWChars(clientTargetSPN.sTargetName) * sizeof(WCHAR));
				goto Exit;
			}
		}
	}
	else
	{
		// real error from QCA.
		*pSSPIFailureState = SNIAUTH_ERRST_SSPIHANDSHAKE_SERVICEBINDINGS_QUERYCONTEXTATTRIBUTES;
		BidTrace1(ERROR_TAG _T("QueryContextAttributes failed with error: 0x%x.\n"), dwRet);
		goto Exit;
	}

	// If we got here, everything checked out ok.
	Assert(SEC_E_OK == dwRet);

Exit:
	if( NULL != clientTargetSPN.sTargetName )
	{
		g_pFuncs->FreeContextBuffer(clientTargetSPN.sTargetName);
		clientTargetSPN.sTargetName = NULL;
	}
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;
}

// Helper for SNISecInitPackageEx to free the AcceptedSPNs list, in cases where it wasn't consumed by SNI_ServiceBindings.
void FreeAcceptedSPNs(__deref_in_ecount_opt(*pdwcAcceptedSPNs) WCHAR ***ppwszAcceptedSPNs, DWORD *pdwcAcceptedSPNs)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("*ppwszAcceptedSPNs: %p{WCHAR **}, ")
								  _T("*pdwcAcceptedSPNs: %d{DWORD}\n"), 
								  *ppwszAcceptedSPNs, 
								  *pdwcAcceptedSPNs);
	Assert(NULL != ppwszAcceptedSPNs);
	Assert(NULL != pdwcAcceptedSPNs);
	
	if( NULL != *ppwszAcceptedSPNs )
	{
		for(DWORD dwIndex=0; dwIndex < *pdwcAcceptedSPNs; dwIndex++)
		{
			delete []((*ppwszAcceptedSPNs)[dwIndex]);
			(*ppwszAcceptedSPNs)[dwIndex] = NULL;
		}
		delete [](*ppwszAcceptedSPNs);
		*ppwszAcceptedSPNs = NULL;
		*pdwcAcceptedSPNs = 0;
	}
	BidTraceU0( SNI_BID_TRACE_ON, RETURN_TAG _T("\n"));
}

DWORD SNISecInitPackageEx(__out DWORD * pcbMaxToken, BOOL fInitializeSPN, BOOL fInitializeServerCredential, ExtendedProtectionPolicy eExtendedProtectionLevel, __in_ecount_opt(dwcAcceptedSPNs) WCHAR **pwszAcceptedSPNs, DWORD dwcAcceptedSPNs)
{
	BidxScopeAutoSNI6( SNIAPI_TAG _T("pcbMaxToken: %p{DWORD*}, ")
								  _T("fInitializeSPN: %d{BOOL}, ")
								  _T("fInitializeServerCredential: %d{BOOL}, ")
								  _T("eExtendedProtectionLevel: %d{ExtendedProtectionLevel}, ")
								  _T("pwszAcceptedSPNs: %p{WCHAR **}, ")
								  _T("dwcAcceptedSPNs: %d{DWORD}\n"), 
								  pcbMaxToken, 
								  fInitializeSPN, 
								  fInitializeServerCredential,
								  eExtendedProtectionLevel, 
								  pwszAcceptedSPNs, 
								  dwcAcceptedSPNs);
	
	PSecPkgInfo 	pkgInfo;
	DWORD			dwRet;

#ifdef SNI_BASED_CLIENT
	//
	//Initialize the sec package only for the first call. Only apply to client to minimize impact for a QFE.
	//SQL HOTFIX: 50000204

	CAutoSNICritSec a_csInitialize( g_csSecPackageInitialize, SNI_AUTOCS_DO_NOT_ENTER );

	a_csInitialize.Enter();

	if( 1 != InterlockedIncrement( &g_cSNISecPackageInitialized ))
	{
		// DEVNOTE: Currently, hitting this code is only possible for SNAC, which does not pass an SPN Approved List for matching Service Bindings.
		// If that changes in the future, we must make a functional decision about how to handle multiple SPN Approved Lists, and we must make
		// sure to properly handle the memory (currently it will be leaked when this API is re-entered).
		Assert(NULL == pwszAcceptedSPNs);
		
		*pcbMaxToken = g_cbMaxToken;		
		BidTraceU2( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}, ")
												_T("g_cSNISecPackageInitialized: %d\n"), 
												ERROR_SUCCESS,
												g_cSNISecPackageInitialized);

		a_csInitialize.Leave(); 
		return ERROR_SUCCESS; 
	}

	Assert ( 1 == g_cSNISecPackageInitialized );

#endif
	if( eExtendedProtectionLevel >= ExtendedProtectionPolicy_INVALID )
	{
		dwRet = ERROR_INVALID_PARAMETER;
		
		Assert(0 && "Invalid ExtendedProtectionPolicy specified.\n");
		BidTrace1( ERROR_TAG _T("Invalid ExtendedProtectionPolicy specified: %d.\n"), dwRet);
		
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwRet );
		goto ErrorExit;
	}
	
	g_eExtendedProtectionLevel = eExtendedProtectionLevel;

	// no need to initialize SNI_ServiceBindings infrastructure unless it will be used - the global variable is only set once, here, so this is safe.
	if( g_eExtendedProtectionLevel > ExtendedProtectionPolicy_OFF )
	{
		dwRet = SNI_ServiceBindings::SetHostNamesAndAcceptedSPNs(pwszAcceptedSPNs, dwcAcceptedSPNs);
		// Calling SetHostNamesAndAcceptedSPNs hands these off to SNI_ServiceBindings responsibility. NULL them out so we don't try to free them later
		pwszAcceptedSPNs = NULL;
		dwcAcceptedSPNs = 0;
		
		if (dwRet != ERROR_SUCCESS)
		{
			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, dwRet );
			
			goto ErrorExit;
		}
	}
	
	*pcbMaxToken = 0;
	
	// load and initialize the negotiate ssp. 
	const char * szDll = "secur32.dll";
	g_hSspiLib = SNILoadSystemLibraryA( szDll );
	///error code is not know for this function call


	if( g_hSspiLib == NULL )
	{
		dwRet = ERROR_FAIL;
		
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_22, dwRet );

		goto ErrorExit;
	}

	INIT_SECURITY_INTERFACE pfInitSecurityInterface;

	pfInitSecurityInterface = (INIT_SECURITY_INTERFACE) GetProcAddress( g_hSspiLib, "InitSecurityInterfaceW" );

	if( pfInitSecurityInterface == NULL )
	{
		dwRet = GetLastError();

		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwRet );

		goto ErrorExit;
	}

	THREAD_PREEMPTIVE_ON_START (PWAIT_PREEMPTIVE_OS_AUTHENTICATIONOPS);
	g_pFuncs = pfInitSecurityInterface();

		
	if( g_pFuncs == NULL )
	{
		dwRet = GetLastError();

		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwRet );

		goto ErrorExit;
	}

	// Query for the package we're interested in
	//
	bool fPackageFound = false;

	for ( int i = 0; i < CSSPKGS; i++)
	{
		dwRet = g_pFuncs->QuerySecurityPackageInfo( g_rgszSSP[i], &pkgInfo );

		if ( SEC_E_OK == dwRet )
		{
			g_rgfSSPIPackageAvailable[i] = true;
			if ( !fPackageFound )
			{
				// Select first available package name and max token size and save this off.
				Assert ( sizeof(g_szSSP) >= g_rgcchSSP[i] );
				
				memcpy(g_szSSP, g_rgszSSP[i], g_rgcchSSP[i]);

				g_cbMaxToken = pkgInfo->cbMaxToken;

				fPackageFound = true;
			}
			
			// Adjust g_cbMaxToken to be largest of NEGOTIATE_SSPI_PACKAGE and
			// NTLM_SSPI_PACKAGE. This allows us to safely use g_cbMaxToken with either.
			if ( ( NEGOTIATE_SSPI_PACKAGE == i ) || ( NTLM_SSPI_PACKAGE == i ) )
			{
				if ( pkgInfo->cbMaxToken > g_cbMaxToken ) g_cbMaxToken = pkgInfo->cbMaxToken;
			}

			// Free package info buffer.
			g_pFuncs->FreeContextBuffer( pkgInfo );
		}
	}

	if ( !fPackageFound )
	{
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwRet );

		goto ErrorExit;
	}

	TimeStamp		Lifetime;

	// Win9x - server is not supported on this, so we dont care about doing this
	if( !gfIsWin9x )
	{
 		if(fInitializeServerCredential)
		{
			dwRet = g_pFuncs->AcquireCredentialsHandle( NULL,			// principal
														 (SEC_TCHAR *) g_szSSP,
														 SECPKG_CRED_BOTH,
														 NULL,			// LOGON id
														 NULL,			// auth data
														 NULL,			// get key fn
														 NULL,			// get key arg
														 &ghCred,
														 &Lifetime );
			if( SEC_E_OK != dwRet  )
			{
				SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwRet );

				goto ErrorExit;
			}
		}

		// Do the Spn init stuff
		// In WinNT4 and before,  SPNInit might fail due to lack of certain library.
		// In Yukon server, we support Win2000 and up
		// In SNAC, we support NT4 and up
		// In Snix, we support win98 second edition and up.
#ifndef SNI_BASED_CLIENT

		if( fInitializeSPN )
		{
			dwRet = SNI_Spn::SpnInit();
			if (ERROR_SUCCESS != dwRet )
			{
				// "The SQL Server Network Interface library was unable to load SPN related library."
				scierrlog(26039, dwRet);

				SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwRet );

				goto ErrorExit;
			}
			else
			{
				g_fSPNInitialized = TRUE;
			}
		}
#else
		if (fInitializeSPN && ERROR_SUCCESS == SNI_Spn::SpnInit())
		{
			// in client side we fail silently. Only Spn::MakeSpn() and client-built unit tests are affected.
			g_fSPNInitialized = TRUE;
		}
#endif		
	}

	THREAD_PREEMPTIVE_ON_END;
		
	*pcbMaxToken = g_cbMaxToken;

#ifdef SNI_BASED_CLIENT	
	a_csInitialize.Leave(); 
#endif

	FreeAcceptedSPNs(&pwszAcceptedSPNs, &dwcAcceptedSPNs);

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;

ErrorExit:

	FreeAcceptedSPNs(&pwszAcceptedSPNs, &dwcAcceptedSPNs);
	
	if( g_hSspiLib )
	{
		FreeLibrary(g_hSspiLib);
	}

	// Terminate Spn
	SNI_Spn::SpnTerminate();
	
#ifdef SNI_BASED_CLIENT	
	// Revert our ref count increment
	InterlockedDecrement( &g_cSNISecPackageInitialized );	

	a_csInitialize.Leave(); 
#endif	
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}


DWORD SNISecInitPackage(__out DWORD * pcbMaxToken)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("pcbMaxToken: %p{DWORD*}\n"), pcbMaxToken );

	Assert ( NULL != pcbMaxToken );

	// set the defaults for extra parameters to SNISecInitPackageEx
	BOOL fInitializeSPN = TRUE;
	ExtendedProtectionPolicy eExtendedProtectionLevel = ExtendedProtectionPolicy_OFF;
	WCHAR **pwszAcceptedSPNs = NULL;
	DWORD dwcAcceptedSPNs = 0;
	
	return SNISecInitPackageEx(pcbMaxToken, fInitializeSPN, FALSE, eExtendedProtectionLevel, pwszAcceptedSPNs, dwcAcceptedSPNs);
	
}

// helper for SNISecGen*Context
// Note: caller of GetChannelBindings does NOT take ownership of the buffer pointer placed 
// into pInBuff and should NOT directly attempt to free it. Once the SSPI handshake is complete,
// the Channel Binding Token buffer can be freed on the SNI_Conn by calling 
// SNI_Conn::ReleaseChannelBindings.
DWORD GetChannelBindings( __in SNI_Conn *pConn, __out SecBuffer *pInBuff)
{
	Assert(NULL != pConn);
	Assert(NULL != pInBuff);
	
	if( NULL != pConn->m_pvChannelBindings )
	{
		if( NULL != (pInBuff->pvBuffer = ((SecPkgContext_Bindings *)(pConn->m_pvChannelBindings))->Bindings) )
		{
			if( 0 != (pInBuff->cbBuffer = ((SecPkgContext_Bindings *)(pConn->m_pvChannelBindings))->BindingsLength) )
			{
				pInBuff->BufferType = SECBUFFER_CHANNEL_BINDINGS;
				return ERROR_SUCCESS;
			}
		}
	}
	return ERROR_FAIL;
}

DWORD SNISecTerminatePackage()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );

#ifdef SNI_BASED_CLIENT
	// terminate the sec package only for the last call.
	
	CAutoSNICritSec a_csInitialize( g_csSecPackageInitialize, SNI_AUTOCS_DO_NOT_ENTER );

	a_csInitialize.Enter();

	if( 0 != InterlockedDecrement( &g_cSNISecPackageInitialized ))
	{
		BidTraceU2( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}, ")
												_T("g_cSNISecPackageInitialized: %d\n"), 
												ERROR_SUCCESS,
												g_cSNISecPackageInitialized);
		a_csInitialize.Leave(); 		
		return ERROR_SUCCESS; 
	}

	Assert ( 0 == g_cSNISecPackageInitialized );
	
#endif

	if( g_hSspiLib )
	{
		FreeLibrary(g_hSspiLib);
	}
	
	// Terminate Spn
	SNI_Spn::SpnTerminate();

#ifdef SNI_BASED_CLIENT	
	a_csInitialize.Leave(); 
#endif	// #ifdef SNI_BASED_CLIENT

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;
}

DWORD SNISecGenServerContext( __in SNI_Conn * pConn, 
						BYTE     * pIn,	// pIn can be null.
						DWORD      cbIn,
						BYTE     * pOut,
						__in DWORD    * pcbOut,
						__out BOOL     * pfDone,
						SNIAuthErrStates *pSSPIFailureState,
						BOOL fCheckChannelBindings)
{

	Assert( NULL != pConn );
	Assert( NULL != pOut );
	Assert( NULL != pcbOut );
	Assert( NULL != pfDone );
	Assert( NULL != pSSPIFailureState );

	BidxScopeAutoSNI9( SNIAPI_TAG _T("%u#{SNI_Conn}, ")
							  _T("pConn: %p{SNI_Conn*}, ")
							  _T("pIn: %p{BYTE*}, ")
							  _T("cbIn: %d, ")
							  _T("pOut: %p{BYTE*}, ")
							  _T("pcbOut: %p{DWORD*}, ")
							  _T("pfDone: %p{BOOL*}, ")
							  _T("pSSPIFailureState: %p{SNIAuthErrStates*}, ")
							  _T("fCheckChannelBindings: %d{BOOL}\n"), 
							  pConn->GetBidId(),
							  pConn, 
							  pIn, 
							  cbIn, 
							  pOut, 
							  pcbOut, 
							  pfDone,
							  pSSPIFailureState,
							  fCheckChannelBindings );

	DWORD dwRet;
	DWORD cbOut = * pcbOut;
	*pfDone = FALSE;
	*pSSPIFailureState = SNIAUTH_ERRST_SUCCESS;
	
	TimeStamp		Lifetime;
	SecBufferDesc	OutBuffDesc;
	SecBuffer		OutSecBuff;
	SecBufferDesc	InBuffDesc;
	SecBuffer		InSecBuffers[NEGOTIATE_BUFFERS_REQUIRED];
	ULONG			ContextAttributes = 0;
	SNI_Sec 			* pSec = pConn->m_pSec;

	BOOL fConnAllowsCheckingBindings = FALSE;
	dwRet = SNIGetInfo(pConn, SNI_QUERY_CONN_SUPPORTS_EXTENDED_PROTECTION, &fConnAllowsCheckingBindings);
	Assert(ERROR_SUCCESS == dwRet);

	// the combination of Protection Level and whether the transport provider allows it determines whether we will actually check the bindings.
	bool fCheckBindings =	(ExtendedProtectionPolicy_OFF != g_eExtendedProtectionLevel) &&
							fConnAllowsCheckingBindings;
		
	// prepare output buffer
	//
	OutBuffDesc.ulVersion = SECBUFFER_VERSION;
	OutBuffDesc.cBuffers  = 1;
	OutBuffDesc.pBuffers  = &OutSecBuff;

	OutSecBuff.cbBuffer   = *pcbOut;
	OutSecBuff.BufferType = SECBUFFER_TOKEN;
	OutSecBuff.pvBuffer   = pOut;

	// prepare input buffer(s)
	//
	InBuffDesc.ulVersion = SECBUFFER_VERSION;
	InBuffDesc.cBuffers  = NEGOTIATE_BUFFERS_REQUIRED;
	InBuffDesc.pBuffers  = InSecBuffers;

	DWORD dwBufferToWrite = 0;

	if (fCheckBindings && fCheckChannelBindings)
	{
		dwRet = GetChannelBindings(pConn, &(InSecBuffers[dwBufferToWrite]));
		if( ERROR_SUCCESS == dwRet )
		{
			dwBufferToWrite++;
		}
		else
		{
			// We intended to check Channel Bindings but they are not available - this means the OS didn't support them.
			// We must fail authentication ourselves. 

			*pSSPIFailureState = SNIAUTH_ERRST_SSPIHANDSHAKE_CHANNELBINDINGS_NOTSUPPORTED;
			dwRet = SEC_E_BAD_BINDINGS;
			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_15, dwRet );
			
			// To make double-sure that this client does not sneak through authorization, we will fail future Impersonation
			// attempts ourselves.
			pConn->m_fBindingsCheckFailed = true;
			goto ErrorExit;
		}
	}


	InSecBuffers[dwBufferToWrite].cbBuffer	 = cbIn;
	InSecBuffers[dwBufferToWrite].BufferType = SECBUFFER_TOKEN;
	InSecBuffers[dwBufferToWrite].pvBuffer	 = pIn;
	dwBufferToWrite++;

	SECURITY_STATUS	ss;

	ULONG ulContextReq = ASC_REQ_MUTUAL_AUTH | ASC_REQ_DELEGATE;
	
	// The exact behavior of Partial mitigation is determined by SSPI - our "partial" setting aligns exactly with their equivalent flag, 
	// ASC_REQ_ALLOW_MISSING_BINDINGS
	if (g_eExtendedProtectionLevel == ExtendedProtectionPolicy_ALLOWED)
	{
		ulContextReq |= ASC_REQ_ALLOW_MISSING_BINDINGS;
	}
	
	THREAD_PREEMPTIVE_ON_START (PWAIT_PREEMPTIVE_OS_AUTHENTICATIONOPS);

	// Correct the total buffer count to be the # of buffers we filled, not the # we allocated.
	InBuffDesc.cBuffers = dwBufferToWrite;
	ss = g_pFuncs->AcceptSecurityContext( &ghCred,
									  pSec->m_fCtxt  ? &pSec->m_hCtxt : NULL,
									  &InBuffDesc,
									  ulContextReq,// context requirements
									  SECURITY_NATIVE_DREP,
									  &pSec->m_hCtxt,
									  &OutBuffDesc,
									  &ContextAttributes,
									  &Lifetime );

	// Note: I removed the old check for invalid token. We should never get
	// a Kerberos token unless we have registered an Spn and in that case an invalid
	// token is an error. 
	// Code taken out after Bug 233729.
	if( !SEC_SUCCESS (ss) )
	{
		*pSSPIFailureState = SNIAUTH_ERRST_SSPIHANDSHAKE_ACCEPTSECURITYCONTEXT;
		if( SEC_E_BUFFER_TOO_SMALL == ss || SEC_E_INSUFFICIENT_MEMORY == ss )
		{
			// Retry with cbMaxToken by returning failure with non-zero pcbOut 
			// iff cbOut < cbMaxToken. TDS would use newly returned *pcbOut to allocate
			// buffer.
			
			if ( cbOut < g_cbMaxToken )
			{
				*pcbOut = g_cbMaxToken;

				dwRet = ss;
				
				SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwRet );

				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
			
				return dwRet;
			}
			else 
			{
#ifndef SNI_BASED_CLIENT		
			// Treat OOM as failure but log it
			ex_print_oom( OOM_SNI_SSPI_SNISECGENSERVERCTXT );
#endif
			dwRet = ss;

			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwRet );

			goto ErrorExit;
			}
		}
		else
		{
			if( SEC_E_BAD_BINDINGS == ss )
			{
				// special error states for SEC_E_BAD_BINDINGS
				switch(g_eExtendedProtectionLevel)
				{
					case ExtendedProtectionPolicy_OFF:
						// This should be impossible, and should indicate either a mistaken assumption during coding 
						// or a Windows bug. Expectation is that Negotiate only returns SEC_E_BAD_BINDINGS due 
						// to Channel Bindings mismatch, and with OFF we don't specify CBs. 
						// Assert the unexpected error case and leave the failure state as ASC failure.
						Assert(0 && "Unexpected return value from AcceptSecurityContext.");
						break;
					case ExtendedProtectionPolicy_ALLOWED:
						*pSSPIFailureState = SNIAUTH_ERRST_SSPIHANDSHAKE_CHANNELBINDINGS_EMPTYORWRONG;
						break;
					case ExtendedProtectionPolicy_REQUIRED:
						*pSSPIFailureState = SNIAUTH_ERRST_SSPIHANDSHAKE_CHANNELBINDINGS_NULLOREMPTYORWRONG;
						break;
					default:
						Assert(0 && "g_eExtendedProtectionLevel is not properly initialized.");
						break;
				}
			}
			dwRet = ss;

			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwRet );

			goto ErrorExit;
		}
	}

	// At this point, we have a token, so need to delete it in the destructor
	pSec->m_fCtxt = TRUE;
	
	// Complete token -- if applicable
	if( (ss == SEC_I_COMPLETE_NEEDED) || (ss == SEC_I_COMPLETE_AND_CONTINUE) )
	{
		if( g_pFuncs->CompleteAuthToken )
		{
			dwRet = g_pFuncs->CompleteAuthToken( &pSec->m_hCtxt, &OutBuffDesc );

			if( SEC_E_OK != dwRet )
			{
				*pSSPIFailureState = SNIAUTH_ERRST_SSPIHANDSHAKE_COMPLETEAUTHTOKEN;
				SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwRet );
				
				goto ErrorExit;
			}
		}
		else 
		{
			*pSSPIFailureState = SNIAUTH_ERRST_SSPIHANDSHAKE_NOCOMPLETEAUTHTOKENPOINTER;
						
			dwRet = ERROR_FAIL;

			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_15, dwRet );
			
			goto ErrorExit;
		}
	}

	*pcbOut = OutSecBuff.cbBuffer;

	*pfDone = !(   (ss == SEC_I_CONTINUE_NEEDED)
			    || (ss == SEC_I_COMPLETE_AND_CONTINUE) );

	// Set the package name to whatever we are using - must be done before Service Bindings check, otherwise we will try to 
	// validate SBs even for Kerberos, where it is unnecessary.
	pSec->SetPkgName();

	if( *pfDone )
	{
		// If we're done, we're protecting against Authentication Relay, and we didn't check Channel Bindings, we need to check Service Bindings
		if( fCheckBindings && !fCheckChannelBindings )
		{
			ss = pSec->CheckServiceBindings( pSSPIFailureState );
			
			if( SEC_E_OK != ss )
			{
				pConn->m_fBindingsCheckFailed = true;
				dwRet = ss;
				SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwRet );
				goto ErrorExit;
			}
		}

		// Whenever we're done, release any Channel Bindings that may have existed.
		pConn->ReleaseChannelBindings();
	}
	

	THREAD_PREEMPTIVE_ON_END;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;

ErrorExit:

	*pcbOut = 0;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD SNISecGenClientContext (__in SNI_Conn * pConn,
									BYTE	*pIn,	//pIn can be NULL.
									DWORD	cbIn,
									BYTE	*pOut,
									__in DWORD	*pcbOut,
									BOOL	*pfDone,
									__in __nullterminated const WCHAR	*wszServerInfo, 
									DWORD	cbServerInfo,  //Note: wszServerInfo is assumed to be null-terminated and this param is not in use anymore!
									LPCWSTR pwszUserName,
									LPCWSTR pwszPassword)
{
	
	Assert( NULL != pConn );
	Assert( NULL != pOut );
	Assert( NULL != pcbOut );
	Assert( NULL != pfDone );
	
	BidxScopeAutoSNI9( SNIAPI_TAG _T("%u#{SNI_Conn}, ")
							  _T("pConn: %p{SNI_Conn*}, ")
							  _T("pIn: %p{BYTE*}, ")
							  _T("cbIn: %d, ")
							  _T("pOut: %p{BYTE*}, ")
							  _T("pfDone: %p{BOOL*}, ")
							  _T("cbServerInfo: %d, ")
							  _T("wszServerInfo: %p{PCWSTR}, ")
							  _T("pwszUserName: %p{LPCWSTR}\n"), 
							  pConn->GetBidId(),
							  pConn, 
							  pIn, 
							  cbIn, 
							  pOut, 
							  pfDone, 
							  cbServerInfo, 
							  wszServerInfo, 
							  pwszUserName);
	
	DWORD dwRet;
	
	TimeStamp		Lifetime;
	SecBufferDesc	OutBuffDesc;
	SecBuffer		OutSecBuff;
	SecBufferDesc	InBuffDesc;
	SecBuffer		InSecBuffers[NEGOTIATE_BUFFERS_REQUIRED];
	ULONG			ContextAttributes;
	SNI_Sec 			* pSec = pConn->m_pSec;
	TCHAR* 			pszPackage    = g_szSSP;
	TCHAR*			pszTargetName = NULL; 
	TCHAR*			pszBlankSpn=_T("");

#ifdef SNIX
	
		// Empty target name is allowed only on XP
		if ((g_osviSNI.dwMajorVersion == 5) && (g_osviSNI.dwMinorVersion == 1))
		{
			Assert(NULL != wszServerInfo);
			
			if ( 0 == wszServerInfo[0])
			{
				// If SPN  is blank, attempt to use NTLM.
				// Select NTLM SSPI security package if the package is available.
				if ( g_rgfSSPIPackageAvailable[NTLM_SSPI_PACKAGE] )
				{
					pszPackage	  = g_rgszSSP[NTLM_SSPI_PACKAGE];
					pszTargetName = NULL;
	
					BidTraceU2( SNI_BID_TRACE_ON, SNI_TAG _T( "SSP Package name:'%s', SPN:'%s'\n" ), pszPackage, pszBlankSpn);
				}
				else
				{
					pszTargetName = pszBlankSpn;
					
					// In the case when NTLM package is not available, fall back to previous SSPI behavior.
					// Adding bid trace point here to insure this rare edge case is traced.
					BidTraceU0( SNI_BID_TRACE_ON, SNI_TAG _T("SPN is blank and NTLM package is not available, attempting SSPI with just a blank SPN.\n" ) );
				}
	
			}
			else
			{
				pszTargetName = (TCHAR*) wszServerInfo;
	
				BidTraceU2( SNI_BID_TRACE_ON, SNI_TAG _T( "SSP Package name:'%s', SPN:'%s'\n" ), pszPackage, wszServerInfo);
			}
		}
		else
		{
			Assert( wszServerInfo && wszServerInfo[0]);
	
			pszTargetName = (TCHAR*) wszServerInfo;
	
			BidTraceU2( SNI_BID_TRACE_ON, SNI_TAG _T( "SSP Package name:'%s', SPN:'%s'\n" ), pszPackage, wszServerInfo);
		}
	
#else	// In case this is not SNIX

	Assert( wszServerInfo && wszServerInfo[0]);
	
	BidTraceU2( SNI_BID_TRACE_ON, SNI_TAG _T( "SSP Package name:'%s', SPN:'%s'\n" ), pszPackage, wszServerInfo);

#endif	// SNIX or not	

	pszTargetName = (TCHAR *) wszServerInfo;
	
	// If username is provided, it is Digest
	// If this is the first time, create a new Digest object
	// NOTE: FOR NOW, DIGEST IS TBD
	if( pwszUserName )
	{
		//this assertion is used to catch unexpected coding/usage errors.
		Assert( 0 && " pwszUserName should not be non-NULL\n" );
		BidTrace0( ERROR_TAG _T("pwszUserName should not be non-NULL\n") );
	}
	
	// prepare output buffer
	//
	OutBuffDesc.ulVersion = SECBUFFER_VERSION;
	OutBuffDesc.cBuffers = 1;
	OutBuffDesc.pBuffers = &OutSecBuff;

	OutSecBuff.cbBuffer = *pcbOut;
	OutSecBuff.BufferType = SECBUFFER_TOKEN;
	OutSecBuff.pvBuffer = pOut;

	InBuffDesc.ulVersion = SECBUFFER_VERSION;
	InBuffDesc.cBuffers = NEGOTIATE_BUFFERS_REQUIRED;
	InBuffDesc.pBuffers = InSecBuffers;

	DWORD dwBufferToWrite = 0;
	
	THREAD_PREEMPTIVE_ON_START (PWAIT_PREEMPTIVE_OS_AUTHENTICATIONOPS);

	// Client has no policy settings - it always tries to send Channel Bindings.
	dwRet = GetChannelBindings(pConn, &(InSecBuffers[dwBufferToWrite]));
	if( ERROR_SUCCESS == dwRet )
	{
		dwBufferToWrite++;
	}
	else
	{
		// failure to set Channel bindings is not fatal for the client - they may not exist for
		// this connection, and they may not be needed by the server. 
		// If GetChannelBindings failed, the buffer will either be overwritten 
		// (if we have an input buffer) or will be ignored (if we don't)
		dwRet = ERROR_SUCCESS;
	}
	
	// prepare input buffer
	if (pSec->m_fCtxt)
	{
		InSecBuffers[dwBufferToWrite].cbBuffer = cbIn;
		InSecBuffers[dwBufferToWrite].BufferType = SECBUFFER_TOKEN;
		InSecBuffers[dwBufferToWrite].pvBuffer = pIn;
		dwBufferToWrite++;
	}
	else
	{
		
		dwRet = g_pFuncs->AcquireCredentialsHandle( NULL,			// principal
													 (SEC_TCHAR *) pszPackage, // security package
													 SECPKG_CRED_OUTBOUND,
													 NULL,			// LOGON id
													 NULL,			// auth data
													 NULL,			// get key fn
													 NULL,			// get key arg
													 &pSec->m_hCred,
													 &Lifetime );
		if( SEC_E_OK != dwRet  )
		{
			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwRet );

			goto ErrorExit;
		}
	}

#ifdef SNI_BASED_CLIENT	
Retry:
#endif

	SECURITY_STATUS	ss;

	// Correct the total buffer count to be the # of buffers we filled, not the # we allocated.
	InBuffDesc.cBuffers = dwBufferToWrite;
	
	ss = g_pFuncs->InitializeSecurityContext (
						(PCredHandle) &pSec->m_hCred,
						pSec->m_fCtxt ? (PCtxtHandle) &pSec->m_hCtxt : NULL,
						(SEC_TCHAR *)pszTargetName,
						ISC_REQ_DELEGATE | ISC_REQ_MUTUAL_AUTH|ISC_REQ_INTEGRITY|ISC_REQ_EXTENDED_ERROR,
						0,	
						SECURITY_NATIVE_DREP,
						(InBuffDesc.cBuffers > 0) ? &InBuffDesc : NULL, // if we wrote at least one buffer, give it to Nego - otherwise, pass NULL
						0,	
						&pSec->m_hCtxt,
						&OutBuffDesc,
						&ContextAttributes,
						&Lifetime
						);
	
	if ( !SEC_SUCCESS( ss ) )
	{
		// Check if its loopback - in that case, try again with NULL Spn string
#ifdef SNI_BASED_CLIENT
		// Prevent us from loop infinite. 
		// Using pszBlankSpn to prevent possilbility that InitializeSecurityContext overwrites into 
		// pszTargetName by anychance.
		//
		if (NULL != pszTargetName && pszTargetName != pszBlankSpn)
		{
			if( wszServerInfo[0] && Tcp::FIsLoopBack(pConn->m_pwszServer) )
			{
				BidTraceU1( SNI_BID_TRACE_ON, ERROR_TAG _T("InitializeSecurityContext failed with %d{WINERR}, SPN is non-blank and connecting to loopback, calling InitializeSecurityContext again with blank SPN.\n" ), ss );		
				pszTargetName = pszBlankSpn;
				goto Retry;
			}
		}
		
#endif

		dwRet = ss;

		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwRet );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
		
		return dwRet;
	}

	// At this point, we have a token, so need to delete it in the destructor
	pSec->m_fCtxt = TRUE;
		
	*pfDone = !((SEC_I_CONTINUE_NEEDED == ss) ||
				(SEC_I_COMPLETE_AND_CONTINUE == ss));

	if( *pfDone )
	{
		// Once handshake is successfully completed, we can forget the Channel Bindings.
		pConn->ReleaseChannelBindings();
	}

	// Complete token -- if applicable
	if( (ss == SEC_I_COMPLETE_NEEDED) || (ss == SEC_I_COMPLETE_AND_CONTINUE) )
	{
		if( g_pFuncs->CompleteAuthToken )
		{
			dwRet = g_pFuncs->CompleteAuthToken( &pSec->m_hCtxt, &OutBuffDesc );
				
			if( SEC_E_OK != dwRet )
			{
				SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwRet );

				goto ErrorExit;
			}
		}
		else
		{
			dwRet = ERROR_FAIL;

			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_15, dwRet );
			
			goto ErrorExit;
		}
	}
	
	*pcbOut = OutSecBuff.cbBuffer;

	// Set the package name to whatever we are using
	pSec->SetPkgName();

	THREAD_PREEMPTIVE_ON_END;
		
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	return ERROR_SUCCESS;

ErrorExit:

	*pcbOut = 0;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;	
}

DWORD SNISecImpersonateContext(__in SNI_Conn * pConn)
{
	Assert( NULL != pConn );

	BidxScopeAutoSNI2( SNIAPI_TAG _T("%u#{SNI_Conn}, ")
							  _T("pConn: %p{SNI_Conn*}\n"), 
							  pConn->GetBidId(),
							  pConn );	
	
	SNI_Sec 			* pSec = pConn->m_pSec;
	DWORD				 dwRet = ERROR_SUCCESS;
	DWORD dwChannelImpersonation = ERROR_SUCCESS;


	//  If a provider in the chain supports impersonation, then
	//  use it to impersonate the client.

	THREAD_PREEMPTIVE_ON_START (PWAIT_PREEMPTIVE_OS_AUTHENTICATIONOPS);
	
	{
		CAutoSNICritSec a_cs(pConn->m_csProvList, SNI_AUTOCS_ENTER);

		dwChannelImpersonation = pConn->m_pProvHead->QueryImpersonation();

		if( ERROR_SUCCESS == dwChannelImpersonation )
		{
			dwRet = pConn->m_pProvHead->ImpersonateChannel();
		}
	}

	// DEVNOTE: When we port this to SQL 11, we should move the remainder of the code before SOS_Task::MaintainPossibleImpersonationToken()
	// into the SSPI Provider's implementation of ImpersonateChannel()

	// Otherwise, we'll use the SSPI context that was negotiated.

	// Enforce earlier failure of Bindings check, since authentication failure is ignored by SQL Server and is failed at authorization time
	// due to impersonation failure.
	if( ERROR_SUCCESS != dwChannelImpersonation )
	{
		if ( pConn->m_fBindingsCheckFailed )
		{
			BidTrace0 ( ERROR_TAG _T("Failing impersonation due to previous authentication failure from Bindings check.\n"));
			dwRet = SEC_E_BAD_BINDINGS;
		}
		
		//  Otherwise, if we have a good context handle, then attempt impersontation on it.
		else if ( pSec->m_fCtxt )
		{
			dwRet = g_pFuncs->ImpersonateSecurityContext( &pSec->m_hCtxt );
		}
		//  Otherwise, we need to error out.
		else 
		{
			///Assert( 0 );

			dwRet = ERROR_INVALID_STATE;
		}
	}

	if( ERROR_SUCCESS != dwRet )
	{
		BidTrace0 ( ERROR_TAG _T("Impersonation fails.\n"));
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwRet );
		goto Exit ;
	}

	
#ifndef SNI_BASED_CLIENT
	if( SOS_OK != SOS_Task::MaintainPossibleImpersonationToken ())
	{
		BidTrace0 ( ERROR_TAG _T("MaintainPossibleImpersonationToken fails.\n"));
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, ERROR_INVALID_STATE );
		dwRet = ERROR_INVALID_STATE;
		
	};
#endif

	THREAD_PREEMPTIVE_ON_END

Exit:

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD  SNISecRevertContext(__in SNI_Conn * pConn)
{

	Assert( NULL != pConn );

	BidxScopeAutoSNI2( SNIAPI_TAG _T("%u#{SNI_Conn}, ")
							  _T("pConn: %p{SNI_Conn*}\n"), 
							  pConn->GetBidId(),
							  pConn );

	SNI_Sec 			* pSec = pConn->m_pSec;
	DWORD				 dwRet = ERROR_SUCCESS;
	DWORD dwChannelImpersonation = ERROR_SUCCESS;

	THREAD_PREEMPTIVE_ON_START (PWAIT_PREEMPTIVE_OS_AUTHENTICATIONOPS);
	
	{
		CAutoSNICritSec a_cs(pConn->m_csProvList, SNI_AUTOCS_ENTER);

		dwChannelImpersonation = pConn->m_pProvHead->QueryImpersonation();

		if( ERROR_SUCCESS == dwChannelImpersonation )
		{
			dwRet = pConn->m_pProvHead->RevertImpersonation();
		}
	}
	
	if( ERROR_SUCCESS != dwChannelImpersonation )
	{
		if( pSec->m_fCtxt )
		{
			dwRet = g_pFuncs->RevertSecurityContext( &pSec->m_hCtxt );
		}
		else 
		{
			// Client may call revert when there is no impersonation token 
			// available. So the assertion here is bogus.
			// Assert( 0 );
			
			dwRet = ERROR_INVALID_STATE;
		}
	}
	
	if( SEC_E_OK != dwRet )
	{
		BidTrace0 ( ERROR_TAG _T("RevertSecurityContext  fails.\n"));
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwRet );
		goto ErrorExit;
	}
	
#ifndef SNI_BASED_CLIENT
		SOS_Task::MaintainNoImpersonationToken ();
#endif
	
	THREAD_PREEMPTIVE_ON_END
	
ErrorExit:
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD SNISecGetSecHandle(__in SNI_Conn * pConn, __out HANDLE * phSec, __out SecPkg * pPkgId)
{

	Assert( NULL != pConn );
	Assert( NULL != phSec );
	Assert( NULL != pPkgId );

	BidxScopeAutoSNI4( SNIAPI_TAG _T("%u#{SNI_Conn}, ")
							  _T("pConn: %p{SNI_Conn*}, ")
							  _T("phSec: %p{HANDLE*}, ")
							  _T("pPkgId: %p{SecPkg*}\n"), 
							  pConn->GetBidId(),
							  pConn, 
							  phSec, 
							  pPkgId );

	SNI_Sec 	* pSec = pConn->m_pSec;

	*phSec = (HANDLE) &pSec->m_hCtxt;

	*pPkgId = pSec->m_PkgId;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;
}

#ifndef SNI_BASED_CLIENT


////////////////////////////////////////////////////////////////////////////////
//	IsIpAllEnabled
//
//	This function check if IpAll is enabled. 
//
//	Inputs:
//		[in] SS_NODE_HANDLE hRoot:  
//			Root handle of the SNI registry hive of interested sqlserver instance.
//		[out] BOOL *pfEnabled:
//			True is IpAll is enabled.
//
//	Returns:
//		if the call succeeds, ERROR_SUCCESS is returned, else, WINERR code.
//
//	Note:
//		IpAll is enabled only if TCP\Enabled and TCP\ListenOnAllIPs are both set to 1.
//

DWORD IsIpAllEnabled (SS_NODE_HANDLE hRoot, BOOL *pfEnabled)
{
	Assert ( NULL != hRoot );
	Assert ( NULL != pfEnabled );
	
	DWORD dwRet = ERROR_SUCCESS;
	SS_NODE_HANDLE hTcp = NULL; 
	DWORD dwEnabled = 0;
	DWORD dwListenOnAllIPs = 0;

	CPL_ASSERT( sizeof(SS_CHAR) == sizeof(char) ); 

	*pfEnabled = false;
	
		dwRet = static_cast<DWORD>(SSgetNodeHandle(hRoot, 
												SS_NODE_PROTOCOL_TCP, 
												&hTcp)); 
		if ( ERROR_SUCCESS == dwRet )
		{
			dwRet = static_cast<DWORD>(SSgetFlagValue(hTcp, 
												SS_FLAG_ENABLED, 
												&dwEnabled));
			if (( ERROR_SUCCESS == dwRet ) && dwEnabled)
			{
				dwRet = static_cast<DWORD>(SSgetFlagValue(hTcp, 
												SS_FLAG_TCP_LISTEN_ON_ALL_IPS, 
												&dwListenOnAllIPs)); 
				if ( ( ERROR_SUCCESS == dwRet ) && dwListenOnAllIPs )
					*pfEnabled |= TRUE;
			}
		}
	
	if ( hTcp ) SSreleaseNodeHandle(hTcp); 
	
	return dwRet;
}

#endif

// 
// Note: To prevent a regression from shiloh, SNI uses these two functions to register/deregister
// SPN for the first ipall port of specified instance if available. These two functions should only be 
// called at server start up and shutdown respectively.
//

DWORD SNISecAddSpn(const WCHAR * wszInstanceName, BOOL fIsSystemInst)
{
	Assert ( NULL != wszInstanceName  && wszInstanceName[0]);
	
	BidxScopeAutoSNI2( SNIAPI_TAG _T("wszInstancename:%s, fIsSystemInst: %d{BOOL}\n"), wszInstanceName, fIsSystemInst);
	DWORD dwRet = ERROR_SUCCESS;
	DWORD dwFailState = x_earsSuccess;
	TCHAR szFQDNBuf[NI_MAXHOST]=_T("");
	DWORD dwSize = NI_MAXHOST;
	TCHAR szInstance[MAX_INSTANCENAME_LENGTH+1]=_T("");
	DWORD dwcchInstance=0;

	// SPN registration should be done at server bootstrap.
	// We assume there is no race condition here.	
	if ( g_fSPNInitialized)
	{
#ifndef SNI_BASED_CLIENT
		// retrieve first IPAll port for port based SPN
		if(g_usFirstIPAllPort == 0)
		{
			SS_NODE_HANDLE hRoot = NULL; 
			BOOL fEnabled = FALSE;

			dwRet = static_cast<DWORD>(SSgetRootHandleEx(const_cast<WCHAR*>(wszInstanceName), &hRoot, fIsSystemInst )); 
			if ( ERROR_SUCCESS == dwRet )
			{
				if( fIsSystemInst )
				{
					fEnabled = TRUE;
				}
				else
				{
					dwRet = IsIpAllEnabled(hRoot, &fEnabled);
				}
			}

			//
			// IsIpAllEnabled returns failure in case of registry key missing. In this case, proceed 
			// and DO NOT block server from starting up, which behaves the same as Shiloh.  
			// And leave the decision to SNIStartListening, which relies on this key, on whether to 
			// fail server or not. 
			//
			if ( ERROR_SUCCESS != dwRet || !fEnabled )
			{
				BidTraceU2( SNI_BID_TRACE_ON, SNI_TAG _T("Either TCP or ListenOnAllIPs is disabled or missing. dwRet:%d{WINERR}, fEnabled:%d{BOOL}\n"),dwRet, fEnabled);
				dwRet = ERROR_SUCCESS;			
			}
			else
			{
				dwRet = static_cast<DWORD>(SSgetIpAllPort(hRoot, &g_usFirstIPAllPort));
				if ( ERROR_SUCCESS !=  dwRet )
				{
					scierrlog(26037, dwRet, x_earsGetIpAllPort);
					BidTraceU0( SNI_BID_TRACE_ON, SNI_TAG _T("Failed to retrieve FirstIPAllPort\n"));
					g_usFirstIPAllPort = 0;
					dwRet = ERROR_SUCCESS;
				}
			}
			
			if ( hRoot )
				SSreleaseNodeHandle( hRoot );
		}
#endif

		int cRet = 0;
		g_szInstanceSPN[0]=_T('\0');   // Do not use TEXT('\0') because it is #define TEXT(x) x in this context.

		// The following if..else.. compose g_szInstanceSPN = "FQDN:InstanceName"
		// Best effort and trace out any error.
		//
		// Get the Dns FQDN of the local computer
		if( GetComputerNameExW( ComputerNameDnsFullyQualified, szFQDNBuf, &dwSize ) )
		{
			// Format of the SPN for default instance is "MSSQLSvc/FQDN".
			//
			if(!_wcsicmp_l(wszInstanceName, L"MSSQLSERVER", GetDefaultLocale()))
			{
				szInstance[0] = L'\0';
			}
			// Format of the SPN for named instance is "MSSQLSvc/FQDN:InstanceName".
			//
			else
			{

				dwRet = wcscpy_s((WCHAR *)szInstance, MAX_INSTANCENAME_LENGTH, wszInstanceName);
				if (ERROR_SUCCESS == dwRet)
				{
					size_t cLen = wcslen(wszInstanceName);
					if (cLen > MAX_INSTANCENAME_LENGTH)
					{
						dwRet = ERROR_INVALID_PARAMETER;
						BidTrace0( ERROR_TAG _T("Instance name length is invalid.\n"));
					}
					else
					{
						szInstance[cLen] = _T('\0');
					}
				}
				else
				{
					BidTrace1( ERROR_TAG _T("strcpy_s failed: %d{WINERR}\n"), dwRet);
				}
			}

			//	Compose "FQDN:InstanceName"
			//	
			if(ERROR_SUCCESS == dwRet)
			{
				if(FAILED (StringCchPrintf_l(g_szInstanceSPN,
								SNI_MAX_COMPOSED_SPN,
								szInstance[0]==_T('\0')? _T("%s%s"):_T("%s:%s"),
								GetDefaultLocale(),
								szFQDNBuf,			 			//Server FQDN								
								szInstance)))					//Server instance
				{
					g_szInstanceSPN[0]= _T('\0') ;
					dwRet = ERROR_INVALID_PARAMETER;
					BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("StringCchPrintf_l Failed: %d{WINERR}\n"), dwRet);
				}
			}

			if(ERROR_SUCCESS != dwRet)
			{
				scierrlog(26037, dwRet, x_earsGetInstanceName);
			}
		}
		else
		{
			dwRet = GetLastError(); 
			BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("GetComputerNameExW Failed: %d{WINERR}\n"), dwRet);
			scierrlog(26061, dwRet);
		}

		// Regardless errors, if either port or instance name is available, register the corresponding SPN(s).
		//
		if( g_szInstanceSPN[0] != _T('\0') || g_usFirstIPAllPort )
		{
			dwRet =  SNI_Spn::AddRemoveSpn(g_szInstanceSPN, g_usFirstIPAllPort, TRUE, &dwFailState);
		}
		else
		{
			dwFailState = x_earsGetIpAllPortAndInstanceName; 
		}

		// If either g_szInstanceSPN or g_usFirstIPAllPort are valid, the error is the result of AddRemoveSpn;
		// otherwise, it is the last error while composing g_szInstanceSPN.
		//
		if (ERROR_SUCCESS != dwRet ) 
		{
			scierrlog(26037, dwRet, dwFailState);
			BidTrace3(ERROR_TAG _T("Failed to register SPN, g_szInstanceSPN: %s, g_usFirstIPAllPort: %d, state: %d{EAddRemoveSpnFailedState}\n"),  
				g_szInstanceSPN, g_usFirstIPAllPort, dwFailState );
		}			
		else
		{
			g_fSPNRegisterSucceed = TRUE;
		}
	}
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;
}

//
// This function unregister whichever last SPNs egistered.
//
DWORD SNISecRemoveSpn()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("") );

	DWORD dwRet = ERROR_SUCCESS;  

	// SPN unregistration should be done at server shutdown.
	// We assume there is no race condition here.	
	if ( g_fSPNRegisterSucceed && (g_szInstanceSPN[0] != _T('\0') || g_usFirstIPAllPort) &&  g_fSPNInitialized)
	{
		DWORD dwFailState = x_earsSuccess;
		dwRet = SNI_Spn::AddRemoveSpn( g_szInstanceSPN, g_usFirstIPAllPort, FALSE, &dwFailState);	
		if ( ERROR_SUCCESS != dwRet  )
		{
			//If failed in deregistering a SPN, we log the failure and go ahead
			scierrlog(26038, dwRet, dwFailState);
			BidTrace3(ERROR_TAG _T("Failed to deregister SPN, g_szInstanceSPN: %s, g_usFirstIPAllPort: %d, state: %d{EAddRemoveSpnFailedState}\n"),  
				g_szInstanceSPN, g_usFirstIPAllPort, dwFailState );
		}
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;
}
