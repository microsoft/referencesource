//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: sm.cpp
// @Owner: nantu, petergv
// @Test: milu
//
// <owner current="true" primary="true">nantu</owner>
// <owner current="true" primary="false">petergv</owner>
//
// Purpose: SNI Shared Memory Provider.  This is a wrapper around:
// - LPC connectivity to SQL Server 2000 (Shiloh), 
// - LPC-over-NP protocol for SQL Server 2005 (Yukon).  
//
// Notes:
//

// @EndHeader@
//****************************************************************************

#include "snipch.hpp"
#include "stdlib.h"
#include "sm.hpp"
#ifdef SNI_BASED_CLIENT
#include "sm_shiloh.hpp"
#endif
#include "aclapi.h"
#include "np.hpp"

void IncrementConnBufSize(__out SNI_Conn * pConn, SNI_PROVIDER_INFO * pProvInfo); 

#define MAX_VERSION_LENGTH 32

// Type and function definitions from instapi.h.  
typedef enum _SQL_SVCS {
    SVC_SQL         = 0L,
    SVC_AGENT       = 1L,
    SVC_SEARCH      = 2L,
    SVC_OLAP        = 3L,
    SVC_CLUSTER     = 4L,
    SVC_REPLICATION = 5L,
    SVC_SETUP       = 6L,
    SVC_PROVIDERS   = 7L,
    SVC_REPORT      = 8L,
    SVC_NS          = 9L,
    SVC_DTS         = 10L,
    SVC_TOOLS       = 11L,
	SVC_BROWSER     = 12L,
	SVC_ADHELPER    = 13L
} SQL_SVCS,*PSQL_SVCS;

typedef struct _INST_ID {
    WCHAR sData[39];
    DWORD dwSize;
} INST_ID, *PINST_ID;

typedef BOOL  (WINAPI *PFNGetSvcInstanceIDFromName)(
					    IN      LPCWSTR     sInstanceName,
					    IN      SQL_SVCS    Service,
					    OUT     PINST_ID    pInstanceID
					    );

typedef BOOL  (WINAPI *PFNGetSQLInstanceRegStringByID)(
					    IN      PINST_ID  pInstanceID,
					    IN      LPCWSTR   sRegPath,
					    IN      LPCWSTR   sValueName,
					    OUT     LPWSTR    sString,
					    IN OUT  PDWORD    pdwSize
					    );


// Global Variables
extern BOOL			gfIsWin9x;

typedef struct __INSTAPILIBSTRUCT
{
	HMODULE 						hInstapidll;
	PFNGetSvcInstanceIDFromName		pfnGetSvcInstanceIDFromName; 
	PFNGetSQLInstanceRegStringByID	pfnGetSQLInstanceRegStringByID; 

} INSTAPILIBSTRUCT;


static INSTAPILIBSTRUCT* gpInstapiStruct = NULL;


//----------------------------------------------------------------------------
// NAME: Sm::Initialize
//
// PURPOSE:
//		Initializes the Sm provider.
//
// PARAMETERS:
//
// RETURNS:
//		ERROR_SUCCESS if successful.
//		Error code on failure.
//
// NOTES:
//
//
//----------------------------------------------------------------------------

DWORD
Sm::Initialize( PSNI_PROVIDER_INFO pInfo )
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("pInfo: %p{PSNI_PROVIDER_INFO}\n"), pInfo );

	DWORD dwError = ERROR_SUCCESS;

	// Fill all these up
	pInfo->fBaseProv = TRUE;
	pInfo->Size      = 0;
	pInfo->Offset    = 0;
	pInfo->ProvNum   = SM_PROV;
	pInfo->fInitialized = TRUE; 

#ifdef SNI_BASED_CLIENT
	SNI_PROVIDER_INFO Info;
	if( ERROR_SUCCESS != (dwError = Sm_Shiloh::Initialize(&Info)) )
	{
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
		return dwError;
	}

#endif

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	return ERROR_SUCCESS;

}

//----------------------------------------------------------------------------
// NAME: Sm::Terminate
//
// PURPOSE:
//		Terminates the Sm provider.
//
// PARAMETERS:
//
// RETURNS:
//		ERROR_SUCCESS if successful.
//		Error code on failure.
//
// NOTES:
//
//
//----------------------------------------------------------------------------

DWORD
Sm::Terminate()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );

	DWORD dwError = ERROR_SUCCESS;

	if( !rgProvInfo[SM_PROV].fInitialized )
	{
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
		return dwError;
	}

	rgProvInfo[SM_PROV].fInitialized = FALSE; 

#ifdef SNI_BASED_CLIENT
	Sm_Shiloh::Terminate();

	// The contract is there should be no open call pending when performing termination,
	// thus there is no protection when check gdwfInstapidll.

	if ( gpInstapiStruct )
	{
		Assert ( gpInstapiStruct->hInstapidll );
		FreeLibrary (gpInstapiStruct->hInstapidll );
		delete gpInstapiStruct;
		gpInstapiStruct = NULL;
		
	}
#endif
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	return dwError;
}


////////////////////////////////////////////////////////////////////////////////
// This function loads instaapi.dll when it is not successfully loaded before.
// If the only sqlserver is shiloh on the box, this function will fail to load instapi.dll everytime.
// It is unlikely that SNAC is used against shiloh on local box, the perf hit might be ok.
//

DWORD Sm::LoadInstapiIfNeeded(const __in LPCSTR szSharedPathLocation, const __in LPCSTR szInstapidllname)
{
	DWORD dwError = ERROR_SUCCESS;	
	INSTAPILIBSTRUCT* pInstapiStruct = NULL;
	
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );

	//	Check to see if instapdll is already loaded
	//
	if (InterlockedCompareExchangePointer (
		reinterpret_cast<volatile PVOID*>(&gpInstapiStruct),
		NULL, NULL))
	{
		goto ErrorExit;
	}

	pInstapiStruct = NewNoX(gpmo) INSTAPILIBSTRUCT;

	if ( NULL == pInstapiStruct )
	{
		dwError = ERROR_OUTOFMEMORY;
		goto ErrorExit;
	}
	
	pInstapiStruct -> hInstapidll = NULL;
	
	
	//	Temparorily load the DLL.


	HKEY  hKey;

	dwError = static_cast<DWORD> (RegOpenKeyExA( HKEY_LOCAL_MACHINE,// handle to open key
								 szSharedPathLocation,	// subkey name
								 0,					// reserved
								 KEY_QUERY_VALUE,	// security access mask
								 &hKey ));				// handle to open key

	if( ERROR_SUCCESS != dwError )
	{
		
		BidTrace1(ERROR_TAG _T("Cannot retrieve the shared path. %d{WINERR}\n"), dwError);
		goto ErrorExit;
	}

	char szSharedPath[MAX_PATH+1];
	DWORD cszSharedPath = MAX_PATH;
	DWORD dwSharedPathRegType; 
	dwError =  static_cast<DWORD> ( RegQueryValueExA(	hKey,					// handle to key
							   "SharedCode",			// value name
							   0,						// reserved
							   &dwSharedPathRegType,	// value type
							   (LPBYTE)szSharedPath,	// value data
							   &cszSharedPath ));			// size of value data

	RegCloseKey( hKey );							   

	if( ERROR_SUCCESS != dwError )
	{
		BidTrace1(ERROR_TAG _T("Cannot retrieve the shared path. %d{WINERR}\n"), dwError);
		goto ErrorExit;
	}

	if(REG_SZ != dwSharedPathRegType)
	{
		// RegValue is corrupted. In this case, we error out.
		dwError = ERROR_INVALID_DATA;
		goto ErrorExit;
	}

	__analysis_assume(cszSharedPath<=MAX_PATH); //The current header we use does not annotate RegQueryValueEx correctly, adding this to suppress Prefast 26015, we could remove it when the tools set is updated to Vista SDK.
	// Ensure NULL-termination.  
	szSharedPath[cszSharedPath] = '\0';


	char szInstapipath[MAX_PATH+sizeof(szInstapidllname)+1];
	if(FAILED(StringCchPrintf_lA( szInstapipath,
				CCH_ANSI_STRING(szInstapipath),
				"%s%s", GetDefaultLocale(),szSharedPath,szInstapidllname)))
	{
			dwError = ERROR_INVALID_PARAMETER;
			goto ErrorExit;
	}
	
	szInstapipath[sizeof(szInstapipath)-1] = '\0';
	
	if( NULL == (pInstapiStruct->hInstapidll = LoadLibraryA( szInstapipath)) )
	{
		dwError = GetLastError();
		BidTrace1(ERROR_TAG _T("Failed to load instapi.dll. %d{WINERR}\n"), dwError );
		goto ErrorExit;
	}


	const char * szGetSvcInstanceIDFromName = "GetSvcInstanceIDFromName";	
	
	if( !(pInstapiStruct->pfnGetSvcInstanceIDFromName
		=  (PFNGetSvcInstanceIDFromName)GetProcAddress( pInstapiStruct->hInstapidll, 
												szGetSvcInstanceIDFromName)) 
	  )
	{
		dwError = GetLastError();
		BidTrace1(ERROR_TAG _T("Failed to load function GetSvcInstanceIDFromName. %d{WINERR}\n"), dwError );
		goto ErrorExit;
	}


	const char * szGetSQLInstanceRegStringByID = "GetSQLInstanceRegStringByID";	
	
	if( !(pInstapiStruct->pfnGetSQLInstanceRegStringByID
		=  (PFNGetSQLInstanceRegStringByID)GetProcAddress( pInstapiStruct->hInstapidll, 
												szGetSQLInstanceRegStringByID)) 
	  )
	{
		dwError = GetLastError();
		BidTrace1(ERROR_TAG _T("Failed to load function GetSQLInstanceRegStringByID. %d{WINERR}\n"), dwError );
		goto ErrorExit;
	}


	Assert (ERROR_SUCCESS == dwError );

	//	Now try to set global gpInstapiStruct
	if ( InterlockedCompareExchangePointer (
		reinterpret_cast<volatile PVOID*>(&gpInstapiStruct),
		reinterpret_cast<PVOID>(pInstapiStruct) ,NULL))
	{		
		goto ErrorExit;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("Loaded instapi.dll. %d{WINERR}\n"), dwError );	
	return dwError;
	
ErrorExit:

	if ( pInstapiStruct )
	{
		if ( pInstapiStruct -> hInstapidll )	
			FreeLibrary( pInstapiStruct -> hInstapidll );
		
		delete pInstapiStruct;			
	}

	if ( ERROR_SUCCESS != dwError )
		SNI_SET_LAST_ERROR( SM_PROV, SNIE_36, dwError );
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError );	
	return dwError;
}	



//----------------------------------------------------------------------------
// NAME: Sm::IsYukonbyInstanceString
//
// PURPOSE:
//		This function infer whether the instancestring points to a Yukon Server by using instapi and heuritistcs.
//
// PARAMETERS:
//		IN LPWSTR wszInstance: instance string
//		OUT BOOL pfIsYukon:
//		OUT BOOL pfNew:
//
// RETURNS:
//		ERROR_SUCCESS if successful.
//		Error code on failure.
//
// NOTES:
//----------------------------------------------------------------------------

DWORD Sm::IsYukonByInstanceString(__in_opt LPWSTR wszInstance, 
								  __out BOOL * pfIsYukon, 
								  __out_opt BOOL * pfNew, 
								  __out BOOL * pfVersionRetrieved)
{
	BidxScopeAutoSNI4( SNIAPI_TAG _T("wszInstance: \"%s\", pfIsYukon: %p{BOOL*}, pfNew: %p{BOOL*}, pfVersionRetrieved: %p{BOOL*}\n"), 
					wszInstance, pfIsYukon, pfNew, pfVersionRetrieved );

	DWORD dwError = ERROR_SUCCESS;	

	Assert ( wszInstance && wszInstance[0] );
	Assert ( pfIsYukon != NULL );
	Assert ( pfNew != NULL );

	if ( wszInstance == NULL 
		|| wszInstance[0] == L'\0'
		|| pfIsYukon == NULL 
		|| pfNew == NULL 
		|| pfVersionRetrieved == NULL )
	{
		dwError = ERROR_INVALID_PARAMETER;
		goto Exit;
	}

	*pfVersionRetrieved = FALSE; 
	*pfNew = FALSE;

	if ( !gpInstapiStruct )
	{
		*pfIsYukon = FALSE;
		goto Exit;
	}
	
	WCHAR wszVersion[MAX_VERSION_LENGTH+1];	
	DWORD cwszVersion = MAX_VERSION_LENGTH;

	// For obtain instance ID specifying SQL Server service to make sure we 
	// don't get version for AS or RS.  
	//
	INST_ID instID; 

	if( !gpInstapiStruct->pfnGetSvcInstanceIDFromName( wszInstance,	//instance name
													SVC_SQL, 
													&instID ) )
	{
		// If we cannot get the version, it is not yukon
		*pfIsYukon = FALSE;
		goto Exit;
	}

	if( !gpInstapiStruct->pfnGetSQLInstanceRegStringByID( &instID, 
										L"MSSQLSERVER\\CurrentVersion", 	//RegPath
										L"CurrentVersion", 					//ValueName
										wszVersion, 						//String													
										&cwszVersion)
	  )
	{
		// If we cannot get the version, it is not yukon
		*pfIsYukon = FALSE;
		goto Exit;
	}
	
	wszVersion[MAX_VERSION_LENGTH]=L'\0';	
	char szVersion[MAX_VERSION_LENGTH+1];
	int cRet = WideCharToMultiByte( CP_ACP,
					     0,
					     wszVersion,
					     -1,
					     szVersion,
					     MAX_VERSION_LENGTH,
					     NULL,
					     NULL );
	
	if( cRet == 0 )
	{
		dwError = GetLastError();		
		goto Exit;
	}

	szVersion[MAX_VERSION_LENGTH]='\0';

	//4. We got Yukon Version number now!
	*pfIsYukon = FALSE;
	LPSTR szTmp = strchr(szVersion, '.');
	if( szTmp )
	{
		//szTmp = strtok(szVersion, ".");
		*szTmp = 0;
		int iMajorVer = _atoi_l(szVersion, GetDefaultLocale()); 
		if( 9 == iMajorVer )
		{
			*pfIsYukon = TRUE;
			szTmp = szTmp + 1;
			szTmp = strchr(szTmp, '.');
			if( szTmp )
			{
				szTmp = szTmp + 1;
				if( 579 <= _atoi_l(szTmp, GetDefaultLocale()) )
					*pfNew = TRUE;
			}
		}
		else if( 9 < iMajorVer )
		{
			*pfIsYukon = TRUE;
			*pfNew = TRUE; 
		}

		*pfVersionRetrieved = TRUE; 
	}

Exit:
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);	
	if ( ERROR_SUCCESS != dwError )
		SNI_SET_LAST_ERROR( SM_PROV, SNIE_36 , dwError );
	return dwError;
	
}


BOOL Sm::IsShilohClustered(LPWSTR wszInstance)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("wszInstance: \"%s\"\n"), wszInstance );

	WCHAR wszRegLocation[MAX_PATH+128] = L"";
	wszRegLocation[ ARRAYSIZE(wszRegLocation) -1] = 0;

	// If its MSSQLSERVER (default instance), append another MSSQLServer to the path
	if(!_wcsicmp_l(L"MSSQLSERVER", wszInstance, GetDefaultLocale()) )
	{
		if(FAILED(StringCchPrintf_lW( wszRegLocation,
					 ARRAYSIZE(wszRegLocation),
					 L"SOFTWARE\\Microsoft\\MSSQLServer\\Cluster", GetDefaultLocale())))
		{
			goto ErrorExit;
		}
	}

	// Else, its a named instance
	else
	{
		if(FAILED(StringCchPrintf_lW( wszRegLocation,
					 ARRAYSIZE(wszRegLocation),
					 L"SOFTWARE\\Microsoft\\Microsoft SQL Server\\%s\\Cluster", GetDefaultLocale(),
					 wszInstance)))
		{
			goto ErrorExit;
		}
	}

	HKEY  hKey;
	LONG lReturn = RegOpenKeyExW( HKEY_LOCAL_MACHINE,    // handle to open key
								 wszRegLocation,         // subkey name
								 0,                     // reserved
								 KEY_QUERY_VALUE,       // security access mask
								 &hKey );	            // handle to open key

	if( ERROR_SUCCESS != lReturn )
	{
		goto ErrorExit;
	}

	RegCloseKey( hKey );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), TRUE);
	
	return TRUE;

ErrorExit:

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), FALSE);
	
	return FALSE;

}

BOOL Sm::IsClustered(__in LPWSTR wszInstance)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("wszInstance: \"%s\"\n"), wszInstance );
	BOOL fisClustered = FALSE;

	DWORD dwError = ERROR_SUCCESS;
	
	if( !gpInstapiStruct )
	{
		// failed in load instapi.dll, try shiloh
		 fisClustered = Sm::IsShilohClustered(wszInstance);
		goto Exit;		
	}
	
	//Try to use instapi to obtain the server?s cluster name.  

	WCHAR wszClusterName[MAX_NAME_SIZE+1];	
	DWORD cwszClusterName = MAX_NAME_SIZE;

	// For obtain instance ID specifying SQL Server service to make sure we 
	// don't get version for AS or RS.  
	//
	INST_ID instID; 

	if( !gpInstapiStruct->pfnGetSvcInstanceIDFromName( wszInstance,	//instance name
													SVC_SQL, 
													&instID ) )
	{
		// If we cannot get instance ID it means the server is not installed, 
		// hence, it's not clustered.  
		goto Exit;
	}

	if( !gpInstapiStruct->pfnGetSQLInstanceRegStringByID( &instID, 
										L"Cluster", 	//RegPath
										L"ClusterName", 					//ValueName
										wszClusterName, 						//String													
										&cwszClusterName)
	  )
	{
		// If we cannot get the clustername, it is not clustered
		goto Exit;
	}

	fisClustered = TRUE;
	
Exit:

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), fisClustered);	
	return fisClustered;

}


//----------------------------------------------------------------------------
// NAME: Sm::OpenWithFallback
//
// PURPOSE:
//		Opens a new Sm connection.
//
// PARAMETERS:
//
// RETURNS:
//		ERROR_SUCCESS if successful.
//		Error code on failure.
//
// NOTES:
//  		This is specific to an Sm client.
//
//----------------------------------------------------------------------------

DWORD Sm::OpenWithFallback( __in SNI_CONSUMER_INFO *  pConsumerInfo, 
							__out SNI_Conn	 	  ** ppConn,
							__in ProtElem 		  *  pProtElem, 
							__out SNI_Provider 	  ** ppProv,
							BOOL	             fSync )
{
	BidxScopeAutoSNI5( SNIAPI_TAG _T( "pConsumerInfo: %p{SNI_CONSUMER_INFO*}, ppConn: %p{SNI_Conn**}, pProtElem: %p{ProtElem*}, ppProv: %p{SNI_Provider**}, fSync: %d{BOOL}\n"), 
					pConsumerInfo,  ppConn, pProtElem, ppProv, fSync);

	DWORD		dwError = ERROR_SUCCESS;
	BOOL		fQAdded = FALSE;
	BOOL 		fNewer = FALSE;	// Tmpry
	Sm          *pSm = NULL;
	// For Shiloh connectivity
	BOOL   fSNIenabled = FALSE;
	BOOL   fSrvVersionRetrieved = FALSE; 

	// Set the instance
	LPWSTR wszInstance = NULL;

	*ppConn = NULL; 

	wszInstance = StrChrW_SYS(pProtElem->Sm.Alias,(int) wcslen(pProtElem->Sm.Alias), L'\\');
	if( wszInstance )
		(wszInstance) += 1;
	else
		wszInstance = L"MSSQLSERVER";

#ifdef SNI_BASED_CLIENT

	//Load instapi and set global flag gdwfInstapidll
	
	(void) LoadInstapiIfNeeded("Software\\Microsoft\\Microsoft SQL Server\\90", "instapi.dll" );
	if( !gpInstapiStruct )
		(void) LoadInstapiIfNeeded("Software\\Microsoft\\Microsoft SQL Server\\130", "instapi130.dll");

	// Do not connect over shared memory to a clustered server
	if( Sm::IsClustered(wszInstance) )
	{
		SNI_SET_LAST_ERROR( SM_PROV, SNIE_29, ERROR_NOT_SUPPORTED );
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("Cluster is not supported over shared memory. %d{WINERR}\n"), ERROR_NOT_SUPPORTED);
		dwError = ERROR_NOT_SUPPORTED;
		goto ErrorExit; 
	}

	// Check if this server instance is SNI-based or not	
	if( ERROR_SUCCESS != (dwError = Sm::IsYukonByInstanceString(wszInstance, 
																&fSNIenabled, 
																&fNewer, 
																&fSrvVersionRetrieved)) )	
		goto ErrorExit;

	// If the server is Yukon+ or we failed to retrieve the server's 
	// version try Yukon's LPC first (the latter can happen, e.g.
	// when talking to a Yukon IDW14/15 server across 32/64-bit 
	// boundary).  
	Assert( fSrvVersionRetrieved || !fSNIenabled ); 

	// Yukon server is not supported on Win9x.  
	if( fSNIenabled && gfIsWin9x )
	{
		SNI_SET_LAST_ERROR( SM_PROV, SNIE_14, ERROR_NOT_SUPPORTED );
		dwError = ERROR_NOT_SUPPORTED;
		goto ErrorExit; 
	}

#else // #ifdef SNI_BASED_CLIENT

	fSNIenabled = TRUE; 
	fNewer = TRUE; 
	fSrvVersionRetrieved = TRUE; 

#endif // #else ... #ifdef SNI_BASED_CLIENT

	if( ( fSNIenabled || !fSrvVersionRetrieved ) && !gfIsWin9x )
	{
		//	Try Yukon's NP-based LPC.  
		//
		Assert( NULL == *ppConn );  
		
		dwError = Sm::OpenNpBasedYukon( pConsumerInfo, 
										ppConn,
										pProtElem, 
										ppProv,
										fSync ); 

		if( ERROR_SUCCESS == dwError )
		{
			goto SuccessExit; 
		}
		
		if( fSNIenabled || !fSync )
		{
			goto ErrorExit;
		}
		// Otherwise, we failed to retrieve the server version, failed 
		// to make a Yukon LPC connection, and the connection is sync, 
		// so fall back and try Shiloh LPC connection.  
	}

#ifdef SNI_BASED_CLIENT

	// If we got here either we retrieved pre-Yukon version or we failed 
	// to retrieve server version and failed to connect over Yukon LPC.  
	// Try Shiloh LPC now.  
	Assert( !fSNIenabled ); 

	// If this is an Aysnc connection - return error.
	// Async is NOT supported for Shiloh servers.  
	if( !fSync )
	{
		SNI_SET_LAST_ERROR( SM_PROV, SNIE_3, ERROR_NOT_SUPPORTED );
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("SQL Server 2000 does not support Async Shared Memory Connection. %d{WINERR}\n"), ERROR_NOT_SUPPORTED);
		dwError = ERROR_NOT_SUPPORTED;
		goto ErrorExit; 
	}

	SNI_Conn * pConn = NULL; 
	
	dwError = Sm::CreateSNIConn( pConsumerInfo, 
								 &pConn, 
								 pProtElem, 
								 fSync ); 

	if( ERROR_SUCCESS != dwError )
	{
		Assert( NULL == pConn ); 
		goto ErrorExit;
	}

	dwError = Sm_Shiloh::Open(pConn, pProtElem, ppProv);

	if( ERROR_SUCCESS != dwError )
	{
		pConn->Release( REF_Active );
		pConn = NULL;

		goto ErrorExit; 
	}

	*ppConn = pConn; 
	pConn = NULL;  

#endif // #ifdef SNI_BASED_CLIENT

SuccessExit: 
	
	BidTraceU3( SNI_BID_TRACE_ON, RETURN_TAG _T("*ppConn: %p{SNI_Conn*}, *ppProv: %p{SNI_Provider*}, %d{WINERR}\n"), 
		*ppConn, *ppProv, dwError);
	return dwError;

	
ErrorExit:

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	return dwError;
}


//----------------------------------------------------------------------------
// NAME: Sm::OpenNpBasedYukon
//
// PURPOSE:
//		Opens a new Sm connection using the new-style Yukon SM implementation
//		based on Named Pipes.
//
// PARAMETERS:
//
// RETURNS:
//		ERROR_SUCCESS if successful.
//		Error code on failure.
//
// NOTES:
//  		This is specific to an Sm client.
//
//----------------------------------------------------------------------------

DWORD Sm::OpenNpBasedYukon( __in SNI_CONSUMER_INFO *  pConsumerInfo, 
							__out SNI_Conn	 	  ** ppConn,
							__in ProtElem 		  *  pProtElem, 
							__out SNI_Provider 	  ** ppProv,
							BOOL	             fSync )
{
	BidxScopeAutoSNI5( SNIAPI_TAG _T( "pConsumerInfo: %p{SNI_CONSUMER_INFO*}, ppConn: %p{SNI_Conn**}, pProtElem: %p{ProtElem*}, ppProv: %p{SNI_Provider**}, fSync: %d{BOOL}\n"), 
					pConsumerInfo,  ppConn, pProtElem, ppProv, fSync);

	DWORD	   dwError = ERROR_SUCCESS;
	SNI_Conn * pConn = NULL; 

	// Locals used for Shared-Memory mapping into Named-Pipes
	//
	ProtElem protElemSmOverNp;
	DWORD dwLength = sizeof(protElemSmOverNp.Np.Pipe) / 
		sizeof(protElemSmOverNp.Np.Pipe[0]); 
	LPWSTR wszInstance = NULL;

	Assert( NULL == *ppConn ); 

	dwError = Sm::CreateSNIConn( pConsumerInfo, 
								 &pConn, 
								 pProtElem, 
								 fSync ); 

	if( ERROR_SUCCESS != dwError )
	{
		goto ErrorExit;
	}
	
	// Parse out the instance
	//
	wszInstance = StrChrW_SYS(pProtElem->Sm.Alias,(int) wcslen(pProtElem->Sm.Alias), L'\\');
	if( wszInstance )
		(wszInstance) += 1;
	else
		wszInstance = L"MSSQLSERVER";

	protElemSmOverNp.SetProviderNum(NP_PROV);	
	protElemSmOverNp.m_ProviderToReport = SM_PROV; 
	
	dwError = protElemSmOverNp.Init( pProtElem->m_wszServerName, pProtElem->m_wszOriginalServerName ); 

	if( ERROR_SUCCESS != dwError )
	{
		goto ErrorExit; 
	}

	protElemSmOverNp.Np.Pipe[dwLength - 1] = L'\0'; 

	if( FAILED ( StringCchPrintf_lW(
				protElemSmOverNp.Np.Pipe, 
				dwLength,
				L"%s%s", 
				GetDefaultLocale(),
				L"\\\\.\\pipe\\SQLLocal\\", 
				wszInstance ) ) )
	{
		dwError = ERROR_BUFFER_OVERFLOW; 
		goto ErrorExit; 
	}

	if( ERROR_SUCCESS != (dwError = Np::Open(pConn, &protElemSmOverNp, ppProv)) )
		goto ErrorExit;

	pConn->m_ConnInfo.TransportProvider = pProtElem->GetProviderNum();

	*ppConn = pConn; 
	pConn = NULL;  

	BidTraceU3( SNI_BID_TRACE_ON, RETURN_TAG _T("*ppConn: %p{SNI_Conn*}, *ppProv: %p{SNI_Provider*}, %d{WINERR}\n"), 
		*ppConn, *ppProv, dwError);
	return ERROR_SUCCESS;


ErrorExit:

	if( NULL != pConn )
	{
		pConn->Release( REF_Active );
		pConn = NULL;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	return dwError;
}


DWORD Sm::CreateSNIConn( __in SNI_CONSUMER_INFO *  pConsumerInfo, 
						 __out SNI_Conn 		   ** ppConn, 
						 __in ProtElem 		   *  pProtElem, 
						 BOOL 				  fSync )
{
	DWORD dwError = ERROR_SUCCESS;
	SNI_Conn *pConn = NULL;
	
	*ppConn = NULL;
	
	dwError = SNI_Conn::InitObject( &pConn );

	if( ERROR_SUCCESS != dwError )
	{
		goto ErrorExit;
	}
	
	pConn->m_fClient = true;
	pConn->m_fSync = fSync ? true : false;

	dwError = pConn->SetServerName(pProtElem->m_wszServerName, pProtElem->m_wszOriginalServerName);

	if( ERROR_SUCCESS != dwError )
	{
		goto ErrorExit;
	}

	// Set the UserData to the default value;
	//
	SNISetInfo(pConn, SNI_QUERY_CONN_BUFSIZE, (VOID *)&pConsumerInfo->DefaultUserDataLength);

	IncrementConnBufSize( pConn, &rgProvInfo[pProtElem->GetProviderNum()] );

	// Set the IO stack for this connection - this is based on the buffer size
	// Note: We have to do after the consumer and provider buffer sizes are set in the statements above, 
	// since it only at this point the size bucket for this connetion is known.
	//
	pConn->m_MemTag = SNIMemRegion::GetMemoryTag( pConn->m_ConnInfo.ConsBufferSize + pConn->m_ConnInfo.ProvBufferSize );

	SNISetInfo( pConn, SNI_QUERY_CONN_KEY, (VOID *)pConsumerInfo->ConsumerKey );

	// Save the consumer info - this is tmpry - need to fix this
	//
	pConn->m_ConsumerInfo.fnReadComp  = pConsumerInfo->fnReadComp;
	pConn->m_ConsumerInfo.fnWriteComp = pConsumerInfo->fnWriteComp;

	*ppConn = pConn;
	pConn = NULL; 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

	return ERROR_SUCCESS;


ErrorExit:

	if( NULL != pConn )
	{
		pConn->Release( REF_Active );

		pConn = NULL;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	return dwError;
}


//----------------------------------------------------------------------------
// NAME: Sm::GetThreadSID
//
// PURPOSE:
//		Gets the SID of the current user.
//
// PARAMETERS:
//
// RETURNS:
//		ERROR_SUCCESS if successful.
//		Error code on failure.
//
// NOTES:
//
//
//----------------------------------------------------------------------------

#ifndef SNI_BASED_CLIENT

DWORD
Sm::GetThreadSID( SID  ** ppSID )
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "ppSID: %p{SID**}\n"), ppSID );

	BOOL			fReturn      = FALSE;
	HANDLE      	TokenHandle  = NULL;
	PTOKEN_USER		pUserToken	 = NULL;
	DWORD       	dwSizeNeeded = 0;
	DWORD			dwError		 = ERROR_SUCCESS;
	PSID			pSID		 = NULL;

	*ppSID						 = NULL;	
	

	BOOL fThreadSID =  OpenThreadToken( GetCurrentThread(),
										TOKEN_READ,
										FALSE,
										&TokenHandle );

	if (fThreadSID)
	{
		fReturn = GetTokenInformation( TokenHandle,
									   TokenUser,
									   NULL,
									   0,
									   &dwSizeNeeded );
		if( FALSE == fReturn )
		{
			if( ERROR_INSUFFICIENT_BUFFER != (dwError = GetLastError()) )
			{
				SNI_SET_LAST_ERROR( SM_PROV, SNIE_10, dwError );
				goto ErrorExit;
			}
		}

		pUserToken = (PTOKEN_USER) NewNoX(gpmo) BYTE[dwSizeNeeded];

		if( !pUserToken )
		{
			dwError = ERROR_OUTOFMEMORY;
			SNI_SET_LAST_ERROR( SM_PROV, SNIE_4, dwError );
			goto ErrorExit;
		}

		fReturn = GetTokenInformation( TokenHandle,
									TokenUser,
									(LPVOID) pUserToken,
									dwSizeNeeded,
									&dwSizeNeeded );
		if( FALSE == fReturn )
		{
			dwError = GetLastError();
			SNI_SET_LAST_ERROR( SM_PROV, SNIE_10, dwError );
			goto ErrorExit;
		}
	
	pSID = pUserToken->User.Sid;

	}
	//Grab the Process SID (which is already available from the SOS)
	else
	{
		//We merely grab the SID for the process
		if( ERROR_NO_TOKEN == (dwError = GetLastError()))
		{
			pSID = SOS_OS::GetProcessSID();
		}
		else
		{
			dwError = GetLastError();
			SNI_SET_LAST_ERROR( SM_PROV, SNIE_10, dwError );
			goto ErrorExit;
		}
	}

	// Validate the SID before copying.  
	//
	if( !IsValidSid( pSID ) )
	{
		dwError = GetLastError();
		SNI_SET_LAST_ERROR( SM_PROV, SNIE_10, dwError );
		goto ErrorExit;
	}
	
	// Let's make a copy the SID
	//
	dwSizeNeeded = GetLengthSid( pSID );
	*ppSID = (SID *) NewNoX(gpmo) BYTE[dwSizeNeeded];

	if( !*ppSID )
	{
		dwError = ERROR_OUTOFMEMORY;
		SNI_SET_LAST_ERROR( SM_PROV, SNIE_4, dwError );
		goto ErrorExit;
	}

	fReturn =  CopySid( dwSizeNeeded,
						*ppSID,
						pSID );

	if( FALSE == fReturn )
	{
		dwError = GetLastError();
		SNI_SET_LAST_ERROR( SM_PROV, SNIE_10, dwError );
		goto ErrorExit;
	}

	//We never obtained a user token at all if this is a process sid
	if (fThreadSID)
		{
		delete [] pUserToken;
		CloseHandle( TokenHandle );
		}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	return ERROR_SUCCESS;

ErrorExit:
	if( fThreadSID &&  pUserToken )
		delete [] pUserToken;
	if( *ppSID )
		delete [] *ppSID;
	*ppSID = 0;

	if( fThreadSID && TokenHandle )
		CloseHandle( TokenHandle );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	return dwError;
}

#else

DWORD
Sm::GetThreadSID( __out SID  ** ppSID )
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "ppSID: %p{SID**}\n"), ppSID );

	BOOL			fReturn      = FALSE;
	HANDLE      		TokenHandle  = NULL;
	PTOKEN_USER	pUserToken   = NULL;
	DWORD       		dwSizeNeeded = 0;
	DWORD			dwError = ERROR_SUCCESS;

	*ppSID        = NULL;

	fReturn =  OpenThreadToken( GetCurrentThread(),
								 TOKEN_READ,
								 FALSE,
								 &TokenHandle );

	if( FALSE == fReturn )
	{
		// If this was because there was no token for the thread, then
		// use the token of the process
		if( ERROR_NO_TOKEN == (dwError = GetLastError()) )
		{
			fReturn =  OpenProcessToken( GetCurrentProcess(),
								 TOKEN_READ,
								 &TokenHandle );

			if( FALSE == fReturn )
			{
				dwError = GetLastError();
				SNI_SET_LAST_ERROR( SM_PROV, SNIE_10, dwError );
			}
		}
		else
		{
			dwError = GetLastError();
			SNI_SET_LAST_ERROR( SM_PROV, SNIE_10, dwError );
		}


		if( FALSE == fReturn )
			goto ErrorExit;
	}

	fReturn = GetTokenInformation( TokenHandle,
								   TokenUser,
								   NULL,
								   0,
								   &dwSizeNeeded );
	if( FALSE == fReturn )
	{
		if( ERROR_INSUFFICIENT_BUFFER != (dwError = GetLastError()) )
		{
			SNI_SET_LAST_ERROR( SM_PROV, SNIE_10, dwError );
			goto ErrorExit;
		}
	}

	pUserToken = (PTOKEN_USER) NewNoX(gpmo) BYTE[dwSizeNeeded];

	if( !pUserToken )
	{
		dwError = ERROR_OUTOFMEMORY;
		SNI_SET_LAST_ERROR( SM_PROV, SNIE_4, dwError );
		goto ErrorExit;
	}

	fReturn = GetTokenInformation( TokenHandle,
								   TokenUser,
								   (LPVOID) pUserToken,
								   dwSizeNeeded,
								   &dwSizeNeeded );
	if( FALSE == fReturn )
	{
		dwError = GetLastError();
		SNI_SET_LAST_ERROR( SM_PROV, SNIE_10, dwError );
		goto ErrorExit;
	}

	// Validate the SID before copying.  
	//
	if( !IsValidSid( pUserToken->User.Sid ) )
	{
		dwError = GetLastError();
		SNI_SET_LAST_ERROR( SM_PROV, SNIE_10, dwError );
		goto ErrorExit;
	}

	// Let's make a copy the SID
	//
	dwSizeNeeded = GetLengthSid( pUserToken->User.Sid );
	*ppSID = (SID *) NewNoX(gpmo) BYTE[dwSizeNeeded];

	if( !*ppSID )
	{
		dwError = ERROR_OUTOFMEMORY;
		SNI_SET_LAST_ERROR( SM_PROV, SNIE_4, dwError );
		goto ErrorExit;
	}

	fReturn =  CopySid( dwSizeNeeded,
						*ppSID,
						pUserToken->User.Sid );

	if( FALSE == fReturn )
	{
		dwError = GetLastError();
		SNI_SET_LAST_ERROR( SM_PROV, SNIE_10, dwError );
		goto ErrorExit;
	}

	delete [] pUserToken;

	CloseHandle( TokenHandle );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	return ERROR_SUCCESS;

ErrorExit:
	if( pUserToken )
		delete [] pUserToken;
	if( *ppSID )
		delete [] *ppSID;
	*ppSID = 0;

	if( TokenHandle )
		CloseHandle( TokenHandle );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	return dwError;
}

#endif

