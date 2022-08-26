//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: sni_spn.cpp
// @Owner: nantu, petergv
// @Test: milu
//
// <owner current="true" primary="true">nantu</owner>
// <owner current="true" primary="false">petergv</owner>
//
// Purpose: SNI Spn utilities
//
// Notes:
//	
// @EndHeader@
//****************************************************************************
#include "snipch.hpp"
#include "sni_spn.hpp"

HINSTANCE ghSpnLib = NULL;
HINSTANCE ghNetApiLib = NULL;
HINSTANCE ghKernel = NULL;
HINSTANCE ghSecur = NULL;
DsFunctionTable gDsFunc;

#ifndef CPL_ASSERT
#define CPL_ASSERT(exp) typedef char __CPL_ASSERT__[(exp)?1:-1]
#endif

#define DS_MAX_PATH 1024

typedef enum
{
	iDsMakeSpn = 0,
	iDsBind,
	iDsUnBind,
	iDsGetSpn,
	iDsFreeSpnArray,
	iDsWriteAccountSpn,
	iDsFreeNameResult,
	iDs----Names,
	iDsGetDcName,
	iGetComputerNameEx,
	iGetComputerObjectName,
	iGetUserNameEx
} SpnIndex;



const char* DsFunctionNames[]={
		"DsMakeSpnW",
		"DsBindW",
		"DsUnBindW",
		"DsGetSpnW",
		"DsFreeSpnArrayW",
		"DsWriteAccountSpnW",
		"DsFreeNameResultW",
		"Ds----NamesW",
		"DsGetDcNameW",
		"GetComputerNameExW",
		"GetComputerObjectNameW",
		"GetUserNameExW"
};

DWORD SNI_Spn::SpnInit()
{		        
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );

    CPL_ASSERT(sizeof(WCHAR) == sizeof(TCHAR));
	
	const char * szntdsapiDll = "ntdsapi.dll";
	ghSpnLib = SNILoadSystemLibraryA( szntdsapiDll );

	if( !ghSpnLib )
		goto ErrorExit;

	if( !(gDsFunc.DsMakeSpn 
		= (DSMAKESPN_FN) GetProcAddress( ghSpnLib, DsFunctionNames[iDsMakeSpn])) )
		goto ErrorExit;

	if( !(gDsFunc.DsBind 
		= (DSBIND_FN) GetProcAddress( ghSpnLib, DsFunctionNames[iDsBind])) )
		goto ErrorExit;

	if( !(gDsFunc.DsUnBind 
		= (DSUNBIND_FN) GetProcAddress( ghSpnLib, DsFunctionNames[iDsUnBind])) )
		goto ErrorExit;

	if( !(gDsFunc.DsGetSpn 
		= (DSGETSPN_FN) GetProcAddress( ghSpnLib, DsFunctionNames[iDsGetSpn])) )
		goto ErrorExit;

	if( !(gDsFunc.DsFreeSpnArray
		= (DSFREESPNARRAY_FN) GetProcAddress( ghSpnLib, DsFunctionNames[iDsFreeSpnArray])) )
		goto ErrorExit;

	if( !(gDsFunc.DsWriteAccountSpn 
		= (DSWRITEACCOUNTSPN_FN) GetProcAddress( ghSpnLib, DsFunctionNames[iDsWriteAccountSpn])) )
		goto ErrorExit;

	if( !(gDsFunc.DsFreeNameResult 
		= (DSFREENAMERESULT_FN) GetProcAddress( ghSpnLib, DsFunctionNames[iDsFreeNameResult])) )
		goto ErrorExit;

	if( !(gDsFunc.Ds----Names 
		= (DS----NAMES_FN) GetProcAddress( ghSpnLib, DsFunctionNames[iDs----Names])) )
		goto ErrorExit;

	// NetApi32
	const char * sznetapi32Dll = "netapi32.dll";
	ghNetApiLib = SNILoadSystemLibraryA( sznetapi32Dll );

	if( !ghNetApiLib )
		goto ErrorExit;
	
	if( !(gDsFunc.DsGetDcName 
		= (DSGETDCNAME_FN) GetProcAddress( ghNetApiLib, DsFunctionNames[iDsGetDcName])) )
		goto ErrorExit;

	if( !(gDsFunc.NetApiBufferFree 
		= (NETAPIBUFFERFREE_FN) GetProcAddress( ghNetApiLib, "NetApiBufferFree")) )
		goto ErrorExit;

	// Load GetComputerNameEx from kernel32.dll
	const char * szkernel32Dll = "kernel32.dll";
	ghKernel = SNILoadSystemLibraryA( szkernel32Dll );

	if( !ghKernel )
		    goto ErrorExit;
	
	if( !(gDsFunc.GetComputerNameEx 
		= (GETCOMPUTERNAMEEX_FN) GetProcAddress( ghKernel, DsFunctionNames[iGetComputerNameEx])) )
		goto ErrorExit;

	// Load GetComputerObjectName and GetUserNameEx from secur32.dll
	const char * szsecur32Dll = "secur32.dll";
	ghSecur = SNILoadSystemLibraryA( szsecur32Dll );

	if( !ghSecur )
		    goto ErrorExit;
	
	if( !(gDsFunc.GetComputerObjectName
		= (GETCOMPUTEROBJECTNAME_FN) GetProcAddress( ghSecur, DsFunctionNames[iGetComputerObjectName])) )
		goto ErrorExit;

	if( !(gDsFunc.GetUserNameEx 
		= (GETUSERNAMEEX_FN) GetProcAddress( ghSecur, DsFunctionNames[iGetUserNameEx])) )
		goto ErrorExit;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

	return ERROR_SUCCESS;

ErrorExit:
	
	DWORD dwError = GetLastError();
	
	if( ghSpnLib )
	{
		FreeLibrary( ghSpnLib );
		ghSpnLib = NULL;
	}

	if( ghNetApiLib )
	{
		FreeLibrary( ghNetApiLib );
		ghNetApiLib = NULL;
	}

	if( ghKernel )
	{
		FreeLibrary( ghKernel );
		ghKernel = NULL;
	}

	if( ghSecur )
	{
		FreeLibrary( ghSecur );
		ghSecur = NULL;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}

void SNI_Spn::SpnTerminate()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );

	if( ghSpnLib )
	{
		FreeLibrary( ghSpnLib );
		ghSpnLib = NULL;
	}

	if( ghNetApiLib )
	{
		FreeLibrary( ghNetApiLib );
		ghNetApiLib = NULL;
	}

	if( ghKernel )
	{
		FreeLibrary( ghKernel );
		ghKernel = NULL;
	}

	if( ghSecur )
	{
		FreeLibrary( ghSecur );
		ghSecur = NULL;
	}
}

DWORD SNI_Spn::MakeSpn(__in LPTSTR szServer, __in LPTSTR szInstName, USHORT usPort, LPTSTR szSpn, __in DWORD cszSpn)
{

	DWORD dwError = ERROR_SUCCESS;
	
	Assert ( NULL != szServer && 0 != szServer[0] );
	Assert ( NULL != szInstName || 0 < usPort );
	Assert ( NULL != szSpn );
		
	BidxScopeAutoSNI5( SNIAPI_TAG _T("szServer: \"%s\", szInstName: \"%s\",usPort: %d, szSpn: \"%s\", *pcszSpn: %d\n"), 
					szServer, szInstName, usPort, szSpn, cszSpn );

	BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG 
				_T("ghSpnLib: '%p{HINSTANCE}'\n"), 
				ghSpnLib );	


	szSpn[0] = 0;

	// These and the sequential CPL_ASSERTs ensure that the char/wchar 
	// which we use are assigned correctly and passed correctly to the runtime and Win32 APIs.
	CPL_ASSERT( sizeof(_TCHAR) == sizeof(_T('\0')) );
	CPL_ASSERT( sizeof(TCHAR) == sizeof(_TCHAR) );

	WIN2K3_DEPENDENCY("Use Tcp::IsNumericAddress instead of searching for ':'")

	CPL_ASSERT( sizeof(TCHAR) == sizeof(WCHAR) ); 

	bool fNeedBrackets = (NULL != wcschr(szServer, L':'));

	// Make an instance name based SPN, i.e. MSSQLSvc/FQDN:instancename
	//	
	// Case 1 - Instance name and a hostname or IPv4 address (does not contain ":")
	// Case 2 - Instance name and an ipv6 address or an invalid hostname (contains ":")
	// In this case we need to surround by [] to distinguish the port and prevent security issues
	if(szInstName) 		
	{
		static const _TCHAR szBracketedInstanceFormatString[] = _T("%s/[%s]:%s");
		static const _TCHAR szBracketedEmptyInstanceFormatString[] = _T("%s/[%s]%s");
		static const _TCHAR szClearInstanceFormatString[] = _T("%s/%s:%s");
		static const _TCHAR szClearEmptyInstanceFormatString[] = _T("%s/%s%s");
		const _TCHAR *szFormatString = fNeedBrackets ? 
									(szInstName[0]==0 ? szBracketedEmptyInstanceFormatString : szBracketedInstanceFormatString) : 
									(szInstName[0]==0 ? szClearEmptyInstanceFormatString : szClearInstanceFormatString);
								
		if(FAILED (StringCchPrintf_l(szSpn,
								cszSpn,
								szFormatString,
								GetDefaultLocale(),
								SQL_SERVICECLASS, 	//Service Class
								szServer, 			//Server FQDN								
								szInstName)))		//Server instance
		{
				dwError = ERROR_INVALID_PARAMETER;
				BidTrace1( ERROR_TAG  _T("StringCchPrintf_l:1 : %d{WINERR}\n"), dwError);
				goto Exit;			

		}
	}
	//Make a TCP port based SPN, i.e. MSSQLSvc/FQDN:TcpPort
	//
	// Case 3 - Ipv6 or an invalid hostname containing ":"
	// In this case we need to surround by [] to distinguish the port and prevent security issues
	// Case 4 - Ipv4
	// Note - Microsoft supports 0x-prefixed IPv4 addresses parts. DsMakeSpn does not detect them as IPv4s and succeeds
	//
	// Case 5 - ghSpnLib is not loaded, we cannot do anything else but compose the SPN ourselves
	else if( fNeedBrackets || !ghSpnLib  
#ifdef SNI_BASED_CLIENT
		|| (_tcsspn(szServer, _T("0123456789:.%/")) == _tcslen(szServer))
#else
		|| (_tcsspn(szServer, _T("0123456789:.%/")) == wcslenInWChars(szServer))
#endif
		)

	{
		static const _TCHAR szBracketedFormatString[] = _T("%s/[%s]:%d");
		static const _TCHAR szClearFormatString[] = _T("%s/%s:%d");
		const _TCHAR *szFormatString = fNeedBrackets ? szBracketedFormatString : szClearFormatString;

		Assert (0 < usPort);		
				
		if(FAILED (StringCchPrintf_l(szSpn,
								cszSpn,
								szFormatString,
								GetDefaultLocale(),
								SQL_SERVICECLASS, 	//Service Class
								szServer, 			//Server FQDN								
								usPort)))		//Server instance
		{
				dwError = ERROR_INVALID_PARAMETER;
				BidTrace1( ERROR_TAG  _T("StringCchPrintf_l:2 : %d{WINERR}\n"), dwError);
				goto Exit;			
		}
	}	
	// Case 6 - hostname or an IPv4 address containing "0x" and a hex number *and* Ds library is loaded
	// DsMakeSpn fails for numeric-only IPv4 addresses and for IPv6 addresses on OSs different from Win2k3. It succeeds
	// on IPv4 address with hex number(s) (prefixed by 0x), so this case is not handled here
	else
	{	
		// Compose the Server Principal Name (Service-name based service).   This is the
		// service class and name for the target service and the DNS name of the host we will
		// connect to.  The service must have registered the SPN we present as the
		// "target" when requesting mutual authentication.
		//
		// Applications that require mutual authentication should also request integrity.
		// The client and server must cooperate in providing integrity by using the 
		// MakeSignature and VerifySigntaure APIs to sign message traffic to prevent
		// tampering.  For purposes of simplicity this example does not generate 
		// signed traffic.

		Assert (0 < usPort);		

		dwError = gDsFunc.DsMakeSpn( SQL_SERVICECLASS,
								(LPCTSTR) szServer,
								(LPCTSTR) NULL,
								usPort,
								(LPCTSTR) NULL,
								&cszSpn,
								(LPTSTR) szSpn);

		if( ERROR_SUCCESS != dwError )
		{
			BidTrace1( ERROR_TAG 
				_T("DsMakeSpn: %d{WINERR}\n"), 
				dwError);
		}
		else
		{
			BidTraceU0( SNI_BID_TRACE_ON, SNI_TAG 
				_T("DsMakeSpn succeeded\n") );	
		}
	}

Exit:

	BidTraceU2( SNI_BID_TRACE_ON, RETURN_TAG _T("szSpn: \"%s\", %d{WINERR}\n"), szSpn, dwError);
	return dwError;

}


// Write the SPN for the service onto the account for the context in which the
// server is executing
DWORD SNI_Spn::AddRemoveSpn(__in LPCTSTR pszInstanceSPN, DWORD dwPortNum, BOOL fAdd, DWORD * pdwState)
{
	BidxScopeAutoSNI4( SNIAPI_TAG _T("pszInstanceSPN: \"%s\", dwPortNum: %d, fAdd: %d{BOOL}, pdwState: %p{DWORD*}\n"), 
					pszInstanceSPN, dwPortNum, fAdd, pdwState );

	DWORD				dwError = ERROR_SUCCESS ;    
	HANDLE				hDs = NULL ;  // hDs is intepreted as pointer by DsBind, so doulbe check before make it invalid_handle_value.
	TCHAR** 			pspn1 = NULL, **pspn2 = NULL;  
	TCHAR				wszUser[DS_MAX_PATH+1];
	PDS_NAME_RESULT		pRes;
	ULONG				dwLen = DS_MAX_PATH;
	DWORD				ulSpn = 1;
	PDOMAIN_CONTROLLER_INFO     DomainControllerInfo;
	DWORD				dwUserNameLen;
	LPTSTR				wszUserName = NULL;
	LPCTSTR				rgpInstanceName[1];
	LPCTSTR				rgpSPN[2];
	DWORD				dwcSPN=0;

	*pdwState = x_earsSuccess; 


	Assert(pszInstanceSPN[0] || 0 < dwPortNum ); //At least one SPN is requested by the caller.
		
	ZeroMemory(rgpSPN, sizeof(rgpSPN));	
		
	THREAD_PREEMPTIVE_ON_START (PWAIT_PREEMPTIVE_OS_DOMAINSERVICESOPS);

	//Compose instance based SPN
	//
	if(pszInstanceSPN[0])
	{
		rgpInstanceName[0] = pszInstanceSPN;

		dwError = gDsFunc.DsGetSpn(
						(DS_SPN_NAME_TYPE) DS_SPN_DNS_HOST,
						SQL_SERVICECLASS,
						NULL,
						0,	
						1,					
						rgpInstanceName, 
						NULL,					
						&ulSpn,
						(LPTSTR **) &pspn1);

		if (dwError == NO_ERROR) 
		{
			rgpSPN[dwcSPN]=pspn1[0];

			BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T( "SPN:'%s'\n" ), rgpSPN[dwcSPN]);

			dwcSPN++;
		}
		else
		{
			BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("%d{WINERR}\n"), dwError);
		}
	}
	
	//Compose port based SPN
	//
	if(dwPortNum>0)
	{
		dwError = gDsFunc.DsGetSpn(
						(DS_SPN_NAME_TYPE) DS_SPN_DNS_HOST,
						SQL_SERVICECLASS,
						NULL,
						(USHORT)dwPortNum,	
						0,					
						NULL, 
						NULL,					
						&ulSpn,
						(LPTSTR **) &pspn2);

		if (dwError == NO_ERROR) 
		{
			rgpSPN[dwcSPN]=pspn2[0];
		
			BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T( "SPN:'%s'\n" ), rgpSPN[dwcSPN]);

			dwcSPN++;
		}
		else
		{
			BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("%d{WINERR}\n"), dwError);
		}
	}

	if( 0 == dwcSPN )
	{
		//Both of the DsGetSpn fail, go Exit and the use the last error to report.
		Assert(dwError!=NO_ERROR);
		if( 0 < dwPortNum && pszInstanceSPN[0] )
		{
			*pdwState = x_earsDsGetSpn;
		}
		else if ( 0 < dwPortNum )
		{
			*pdwState = x_earsDsGetSpnPort;
		}
		else
		{
			*pdwState = x_earsDsGetSpnInstanceName;
		}
		
		goto Exit;
	}
	

	// Start with a DsBind, The DS APIs need this.
	dwError = gDsFunc.DsGetDcName( NULL,
	                  NULL,
	                  NULL,
	                  NULL,
	                  DS_RETURN_DNS_NAME,
	                  &DomainControllerInfo );

	if (dwError != NO_ERROR) 
	{
		*pdwState = x_earsDsGetDcName;
		BidTrace1( ERROR_TAG 
				_T("DsGetDcName: %d{WINERR}\n"), 
				dwError );
		goto Exit;
	}
 
	dwError = gDsFunc.DsBind(NULL, DomainControllerInfo->DomainName,&hDs);

	if (dwError != NO_ERROR) 
	{
		*pdwState = x_earsDsBind;
		BidTrace1( ERROR_TAG 
				_T("DsBind: %d{WINERR}\n"), 
				dwError );
		goto Exit;
	}

	// Free the buffer allocated by the system for DomainControllerInfo
	gDsFunc.NetApiBufferFree(DomainControllerInfo);

	// Get the DN for the object whose SPN we will update: a user or computer
	//
	// If this service will run in a named account then get the DN for that account
	// and use that as the target object for the SPN, otherwise use the computer
	// object for the local system as the target for the SPN.  
	//

	dwUserNameLen = 0;

	// The GetUserName() and GetUserNameEx() functions are somewhat broken in
	// that they are 1) inconsistent, and 2) return lengths that include the
	// null-termination; unlike just about any other NT call.
	//
	// If a change to the *Ex() version is made, the size/allocation code below
	// will not be correct!  Changes will be needed there as well -- so don't do
	// it.
	//
	GetUserNameW( NULL, &dwUserNameLen);
	{
		dwError = GetLastError();

		//According MSDN of GetUserNameA, max dwUserNameLen is UNLEN+1.
		if ( dwError == ERROR_INSUFFICIENT_BUFFER 
			&& dwUserNameLen <= UNLEN+1 )  
		{
			wszUserName = new TCHAR[dwUserNameLen + 1] ;
			if ( wszUserName == NULL )
			{
				dwError = ERROR_OUTOFMEMORY;

				*pdwState = x_earsAllocUserName1;
				BidTrace1( ERROR_TAG 
						_T("new wszUserName (1). dwUserNameLen: %d\n"), 
						dwUserNameLen );
				goto Exit;
			}

			if( !GetUserNameW( wszUserName, &dwUserNameLen) )
			{
				dwError = GetLastError();
				
				*pdwState = x_earsGetUserName2;
				BidTrace1( ERROR_TAG 
						_T("GetUserName (2): %d{WINERR}\n"), 
						dwError );
				goto Exit;
			}
			wszUserName[dwUserNameLen] = 0;

		}
		else
		{
			*pdwState = x_earsGetUserName1;
			BidTrace2( ERROR_TAG 
					_T("GetUserName (1).  ")
					_T("dwError: %d{WINERR}, ")
					_T("dwUserNameLen: %d\n"), 
					dwError, 
					dwUserNameLen);
			goto Exit;
		}
	}

	wszUser[DS_MAX_PATH] = _T('\0');
	// SQL Server started as domain\user, neither SYSTEM nor NETWORK SERVICE.
	if(  _tcsicmp_l((_TCHAR *) wszUserName, _T("SYSTEM"), GetDefaultLocale())
		&& _tcsicmp_l((_TCHAR *) wszUserName, _T("NETWORK SERVICE"), GetDefaultLocale()))
	{
		dwUserNameLen = 0 ;
		if( wszUserName != NULL ) 
		{
			delete [] wszUserName;
			wszUserName = NULL;
		}

		if( !gDsFunc.GetUserNameEx( NameSamCompatible,
						                 NULL,
						                &dwUserNameLen) )
		{
			dwError = GetLastError();

			if ( dwError == ERROR_INSUFFICIENT_BUFFER
				|| dwError == ERROR_MORE_DATA 
				&&  dwUserNameLen <= DS_MAX_PATH )
			{
				wszUserName = new TCHAR[dwUserNameLen + 1] ;
				if ( wszUserName == NULL )
				{
					dwError = ERROR_OUTOFMEMORY;

					*pdwState = x_earsAllocUserName2;
					BidTrace1( ERROR_TAG 
							_T("new wszUserName (2). dwUserNameLen: %d\n"), 
							dwUserNameLen );
					goto Exit;
				}

				if(  !gDsFunc.GetUserNameEx ( NameSamCompatible,
											wszUserName, 
											&dwUserNameLen))
				{
					dwError = GetLastError();

					*pdwState = x_earsGetUserNameEx2;
					BidTrace1( ERROR_TAG 
							_T("GetUserNameEx (2): %d{WINERR}\n"), 
							dwError );
					goto Exit;
				}
				wszUserName[dwUserNameLen] = 0;
			}
			else
			{
				*pdwState = x_earsGetUserNameEx1;
				BidTrace2( ERROR_TAG 
						_T("GetUserNameEx (1).  ")
						_T("dwError: %d{WINERR}, ")
						_T("dwUserNameLen: %d\n"), 
						dwError, 
						dwUserNameLen);
				goto Exit;
			}
		}

		dwError = gDsFunc.Ds----Names( hDs,
		                            DS_NAME_NO_FLAGS,
		                            DS_NT4_ACCOUNT_NAME,
		                            DS_FQDN_1779_NAME,
		                            1,
		                            (LPTSTR *) &wszUserName,
		                            &pRes);

		if(dwError != NO_ERROR)
		{
			*pdwState = x_earsDs----Names;
			BidTrace1( ERROR_TAG 
					_T("Ds----Names: %d{WINERR}\n"), 
					dwError );
			goto Exit;
		}
		
		
		if(  (pRes != NULL ) 
			&& pRes->rItems[0].status == DS_NAME_NO_ERROR 
			&& pRes->rItems[0].pName )
		{
			if(FAILED(StringCchPrintf_l( (_TCHAR *) wszUser, 
									ARRAYSIZE(wszUser),
									_T("%s"), GetDefaultLocale(),
									pRes->rItems[0].pName)))
			{
				dwError = ERROR_INSUFFICIENT_BUFFER;

				gDsFunc.DsFreeNameResult(pRes);

				*pdwState = x_earsCopyName;
				BidTrace0( ERROR_TAG 
						_T("StringCchPrintf_l\n") );
				goto Exit;	
			}

			gDsFunc.DsFreeNameResult(pRes);
		}
		else
		{
			if( pRes != NULL )
				gDsFunc.DsFreeNameResult(pRes);
			dwError = ERROR_INVALID_DATA;

			*pdwState = x_earsDs----NamesInvalidData;
			BidTrace0( ERROR_TAG 
					_T("Invalid data from Ds----Names\n") );
			goto Exit;
		}
			
	} 

	// SQL Server was started under a LocalSystem account
	else 
	{
		if( !gDsFunc.GetComputerObjectName(NameFullyQualifiedDN, (LPTSTR) wszUser,&dwLen) )
		{
			dwError = GetLastError();

			*pdwState = x_earsGetComputerObjectName;
			BidTrace1( ERROR_TAG 
					_T("GetComputerObjectName: %d{WINERR}\n"), 
					dwError );
			goto Exit;
		}
	}

	dwError = gDsFunc.DsWriteAccountSpn( 
	                    hDs,
	                    fAdd ? DS_SPN_ADD_SPN_OP : DS_SPN_DELETE_SPN_OP,
	                    (LPCTSTR)wszUser,     
	                    dwcSPN,               
	                    (LPCTSTR *)rgpSPN );

	if(ERROR_SUCCESS == dwError)
	{
		for(DWORD i=0;i<dwcSPN; i++)			
		{
				//Log the registered SPN(s).
				//
				scierrlog(fAdd? 26059:26060, rgpSPN[i]);
		}
	}
	else
	{
		*pdwState = x_earsDsWriteAccountSpn;
		BidTrace1( ERROR_TAG 
				_T("DsWriteAccountSpn: %d{WINERR}\n"), 
				dwError );		
	}	

	THREAD_PREEMPTIVE_ON_END;
 
Exit:

	// Unbind the DS in any case.
	if( hDs ) 
	{
		gDsFunc.DsUnBind(&hDs);		
	}

	if( wszUserName != NULL ) delete [] wszUserName;

	// Free the SPN array, we are done with it.
	if( pspn1 )
		gDsFunc.DsFreeSpnArray(1, (LPTSTR *) pspn1);
	if( pspn2 )
		gDsFunc.DsFreeSpnArray(1, (LPTSTR *) pspn2);

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);

	return dwError;
}


