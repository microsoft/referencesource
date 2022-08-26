//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: SNI_error.cpp
// @Owner: petergv, nantu
// @Test: milu
//
// <owner current="true" primary="false">nantu</owner>
// <owner current="true" primary="true">petergv</owner>
//
// Purpose: SNI Error Routines.
//
// Notes:
//          
// @EndHeader@
//****************************************************************************

#include "snipch.hpp"
#include <strsafe.h>

#ifdef SNI_BASED_CLIENT
// Include only for client SNI
//
#include "LocalDB.hpp"
#endif

//	We are storing DWORDs in TLS, which holds LPVOIDs.  The following two 
//	macros perform conversions between DWORD and PVOID without compiler errors.
//
C_ASSERT( sizeof( DWORD ) <= sizeof( LPVOID ) ); 
#define SNI_DWORD_TO_LPVOID( dwValue ) (static_cast<LPVOID>(static_cast<BYTE *>(NULL) + dwValue))
#define SNI_LPVOID_TO_DWORD( pvValue ) (static_cast<DWORD>(static_cast<BYTE *>(pvValue) - static_cast<BYTE *>(NULL)))


//	A structure containing TLS indices to the last SNI error
//	data.  
//
typedef struct
{
	DWORD dwProviderIndex; 
	DWORD dwSNIErrorStringIdIndex;	
	DWORD dwNativeErrorIndex; 
} SNI_ERROR_TLS; 


SNI_ERROR_TLS g_sniErrorTls = 
{ 
#ifdef SNI_BASED_CLIENT
	//	Client: initialized in DllMain_Sni()/SNIxInitialize()
	//
	TLS_OUT_OF_INDEXES, 
	TLS_OUT_OF_INDEXES, 
	TLS_OUT_OF_INDEXES 
#else	//	#ifdef SNI_BASED_CLIENT
	//	Server: hard-coded.  
	//
	TLS_SNIERRORPROVIDER, 
	TLS_SNIERROR, 
	TLS_SNINATIVEERROR
#endif	//	#ifdef SNI_BASED_CLIENT
}; 

extern BOOL		gfIsWin9x;
extern HINSTANCE g_hinstance_language;      // hinstance for language DLL

//	A helper function for formatting Unicode error messages for SNIX
//	on Win9x.  
//
DWORD FormatMessageWin9x( DWORD		dwMessageId,
						  __out_ecount(nSize) LPWSTR	lpBuffer,
						  DWORD		nSize ); 


//----------------------------------------------------------------------------
// Name: 	pszProviderNames
//
// Purpose:	Provide a clear text description of each provider above
//
// Notes:	Ensure that when a new provider enum is added in sni.hpp, that a
//			corresponding entry is placed here.
//			
//----------------------------------------------------------------------------

WCHAR * pszProviderNames[] =
{
	L"HTTP Provider: ",
	L"Named Pipes Provider: ",
	L"Session Provider: ",
	L"Sign Provider: ",
	L"Shared Memory Provider: ",
	L"SMux Provider: ",
	L"SSL Provider: ",
	L"TCP Provider: ",	
	L"VIA Provider: ",
	L"MAX_PROVS: ",
	L"SQL Server Network Interfaces: "
};

// Initialize an array of error strings
//
//	!!! Important !!!
//	This array must match:
//	- the SNI error ID's in sni_rc.h.  
//
WCHAR * SNI_Error_String[] =
{
	L"",
	L"I/O Error detected in read/write operation",
	L"Connection was terminated",
	L"Asynchronous operations not supported",
	L"",
	L"Invalid parameter(s) found",
	L"Unsupported protocol specified",
	L"Invalid connection found when setting up new session protocol",
	L"Protocol not supported",
	L"Associating port with I/O completion mechanism failed",
	L"",
	L"Timeout error",
	L"No server name supplied",
	L"TerminateListener() has been called",
	L"Win9x not supported",
	L"Function not supported",
	L"Shared-Memory heap error",
	L"Cannot find an ip/ipv6 type address to connect",
	L"Connection has been closed by peer",
	L"Physical connection is not usable",
	L"Connection has been closed",
	L"Encryption is enforced but there is no valid certificate",
	L"Couldn't load library",
	L"Cannot open a new thread in server process",
	L"Cannot post event to completion port",
	L"Connection string is not valid",
	L"Error Locating Server/Instance Specified",
	L"Error getting enabled protocols list from registry",
	L"Server doesn't support requested protocol",
	L"Shared Memory is not supported for clustered server connectivity",
	L"Invalid attempt bind to shared memory segment",
	L"Encryption(ssl/tls) handshake failed",
	L"Packet size too large for SSL Encrypt/Decrypt operations",
	L"SSRP error",
	L"Could not connect to the Shared Memory pipe",
	L"An internal exception was caught",
	L"The Shared Memory dll used to connect to SQL Server 2000 was not found",
	L"The SQL Server 2000 Shared Memory client dll appears to be invalid/corrupted",
	L"Cannot open a Shared Memory connection to SQL Server 2000",
	L"Shared memory connectivity to SQL Server 2000 is either disabled or not available on this machine",
	L"Could not open a connection to SQL Server", 
	L"Cannot open a Shared Memory connection to a remote SQL server",
	L"Could not establish dedicated administrator connection (DAC) on default port. Make sure that DAC is enabled",
	L"An error occurred while obtaining the dedicated administrator connection (DAC) port. Make sure that SQL Browser is running, or check the error log for the port number",
	L"Could not compose Service Principal Name (SPN) for Windows Integrated Authentication. Possible causes are server(s) incorrectly specified to connection API calls, Domain Name System (DNS) lookup failure or memory shortage",
	L"No client protocols are enabled and no protocol was specified in the connection string",
	L"No remote client protocols are enabled and no remote protocol was specified in the connection string",
	L"Connecting with the MultiSubnetFailover connection option to a SQL Server instance configured with more than 64 IP addresses is not supported.",
	L"Connecting to a named SQL Server instance using the MultiSubnetFailover connection option is not supported.",
	L"Connecting to a SQL Server instance using the MultiSubnetFailover connection option is only supported when using the TCP protocol.",
	L"",  // reserved for LocalDB Error messages
	L"An instance name was not specified while connecting to a Local DB. Specify an instance name in the format (localdb)\\instance_name.",
	L"Unable to locate a Local DB installation. Verify that SQL Server Express is properly installed and that the Local DB feature is enabled.",
	L"Invalid Local DB registry configuration found. Verify that SQL Server Express is properly installed.",
	L"Unable to locate the registry entry for SQLUserInstance.dll file path. Verify that the Local DB feature of SQL Server Express is properly installed.",
	L"Registry value contains an invalid SQLUserInstance.dll file path. Verify that the Local DB feature of SQL Server Express is properly installed.",
	L"Unable to load the SQLUserInstance.dll from the location specified in the registry. Verify that the Local DB feature of SQL Server Express is properly installed.",
	L"Invalid SQLUserInstance.dll found at the location specified in the registry. Verify that the Local DB feature of SQL Server Express is properly installed.",
	L"Unable to locate the registry entry for adalsql.dll file path. Verify that Active Directory Authentication Library for SQL Server is properly installed. For details, please refer to: http ://go.microsoft.com/fwlink/?LinkID=513072",
	L"Unable to locate the correct registry entry for adalsql.dll. Verify that Active Directory Authentication Library for SQL Server is properly installed. For details, please refer to: http ://go.microsoft.com/fwlink/?LinkID=513072",
	L"Unable to load adalsql.dll from the location specified in the registry. Verify that Active Directory Authentication Library for SQL Server is properly installed. For details, please refer to: http ://go.microsoft.com/fwlink/?LinkID=513072",
	L"Unable to find adalsql.dll functions. Verify that Active Directory Authentication Library for SQL Server is properly installed. For details, please refer to: http ://go.microsoft.com/fwlink/?LinkID=513072"
};
CPL_ASSERT( (sizeof(SNI_Error_String) / sizeof(SNI_Error_String[0])) == (SNIE_MAX - SNIE_0 + 1));

//	On the server-side, the TLS indices for the last SNI error 
//	are gard-coded, so there is not need to initialize/terminate them.  
//
#ifdef SNI_BASED_CLIENT

DWORD SNIErrorTlsInit()
{
	DWORD dwReturn = ERROR_SUCCESS; 
	
	g_sniErrorTls.dwProviderIndex = TlsAlloc();

	if( TLS_OUT_OF_INDEXES == g_sniErrorTls.dwProviderIndex ) 
	{
		dwReturn = GetLastError(); 
		goto Exit; 
	}

	g_sniErrorTls.dwSNIErrorStringIdIndex = TlsAlloc();

	if( TLS_OUT_OF_INDEXES == g_sniErrorTls.dwSNIErrorStringIdIndex ) 
	{
		dwReturn = GetLastError(); 
		goto Exit; 
	}

	g_sniErrorTls.dwNativeErrorIndex = TlsAlloc();

	if( TLS_OUT_OF_INDEXES == g_sniErrorTls.dwNativeErrorIndex ) 
	{
		dwReturn = GetLastError(); 
		goto Exit; 
	}


Exit: 

	if( ERROR_SUCCESS != dwReturn )
	{
		SNIErrorTlsTerm(); 
	}

	return dwReturn; 
}


void SNIErrorTlsTerm()
{
	if( TLS_OUT_OF_INDEXES != g_sniErrorTls.dwProviderIndex ) 
	{
		TlsFree( g_sniErrorTls.dwProviderIndex );
	}

	if( TLS_OUT_OF_INDEXES != g_sniErrorTls.dwSNIErrorStringIdIndex ) 
	{
		TlsFree( g_sniErrorTls.dwSNIErrorStringIdIndex );
	}

	if( TLS_OUT_OF_INDEXES != g_sniErrorTls.dwNativeErrorIndex ) 
	{
		TlsFree( g_sniErrorTls.dwNativeErrorIndex );
	}

	//	Both client and server:
	//
	g_sniErrorTls.dwProviderIndex = TLS_OUT_OF_INDEXES;

	g_sniErrorTls.dwSNIErrorStringIdIndex = TLS_OUT_OF_INDEXES;

	g_sniErrorTls.dwNativeErrorIndex = TLS_OUT_OF_INDEXES;
}

#endif	//	#ifdef SNI_BASED_CLIENT


//	SNI stores only DWORDs in the TLS.  
//
DWORD SNITlsGetValue( DWORD dwTlsIndex )
{
#ifdef SNI_BASED_CLIENT

	return SNI_LPVOID_TO_DWORD( TlsGetValue( dwTlsIndex ) ); 

#else	//	#ifdef SNI_BASED_CLIENT

	return SNI_LPVOID_TO_DWORD( SOS_Task::RetrieveObject( dwTlsIndex ) ); 

#endif	//	#ifdef SNI_BASED_CLIENT
}


//	SNI stores only DWORDs in the TLS.  
//
BOOL SNITlsSetValue( DWORD dwTlsIndex, DWORD dwTlsValue )
{
#ifdef SNI_BASED_CLIENT

	return TlsSetValue( dwTlsIndex, SNI_DWORD_TO_LPVOID( dwTlsValue ) ); 

#else	//	#ifdef SNI_BASED_CLIENT

	SOS_Task::StoreObject( dwTlsIndex, SNI_DWORD_TO_LPVOID( dwTlsValue ) );

	return TRUE; 

#endif	//	#ifdef SNI_BASED_CLIENT
}


#if defined(SNI_BASED_CLIENT) && !defined(SNIX)
//	SNAC only: load string from a resource file.  
//
DWORD SNILoadString(UINT idStr, LPWSTR rgchBuffer, DWORD cchBuffer)
{
	// g_hinstance_language is initialized from DllMain_Oledb32()
	//	
    return LoadStringW(g_hinstance_language, idStr, rgchBuffer, cchBuffer);
}
#endif


//	SNAC copies the provider name from a resource file.  SNIX and engine
//	from an internal array (for SNIX, SqlClient replaces it from a managed
//	resource).  
//
void SNILoadProviderName(UINT uiProv, __out_ecount(cchBuffer) LPWSTR rgchBuffer, DWORD cchBuffer)
{
	Assert( 0 < cchBuffer ); 
	
	if( 0 < cchBuffer )
	{
		rgchBuffer[0] = 0x0; 

#if defined(SNI_BASED_CLIENT) && !defined(SNIX)
		// SNAC
		//
	    SNILoadString( SNI_STRING_ID_FROM_PROV(uiProv), 
    				   rgchBuffer, 
    				   cchBuffer );
#else
		// Engine and SNIX
		//
		Assert(uiProv < sizeof(pszProviderNames) / sizeof(pszProviderNames[0]));	

		wcsncpy_s( rgchBuffer,
				cchBuffer,
				pszProviderNames[uiProv], 
				cchBuffer );

#endif

		rgchBuffer[cchBuffer - 1] = 0x0; 
	}

}


//	SNAC copies the SNI error message text from a resource file.  
//	SNIX and engine from an internal array (for SNIX, SqlClient 
//	replaces it from a managed resource).  
//
void SNILoadErrorMessage(UINT uiErrorId, __out_ecount(cchBuffer) LPWSTR rgchBuffer, DWORD cchBuffer)
{
	Assert( 0 < cchBuffer ); 
	
	if( 0 < cchBuffer )
	{
		rgchBuffer[0] = 0x0; 

#if defined(SNI_BASED_CLIENT) && !defined(SNIX)
		// SNAC
		//
	    SNILoadString( uiErrorId, 
    				   rgchBuffer, 
    				   cchBuffer );
#else
		// Engine and SNIX
		//
		Assert(SNI_ERROR_ID_FROM_STRING_ID(uiErrorId) < 
			sizeof(SNI_Error_String) / sizeof(SNI_Error_String[0]));	

		(void)StringCchCopyW( rgchBuffer, 
			cchBuffer, 
			SNI_Error_String[SNI_ERROR_ID_FROM_STRING_ID(uiErrorId)]);
#endif

		rgchBuffer[cchBuffer - 1] = 0x0; 
	}

}

// This function is used to obtain system error messages when the SNI error is set to SNIE_SYSTEM.
//
// Inputs : 
//		pSNIerror - pointer to SNI_ERROR object
//		dwNativeError - Native Error Code
//		dwCurMsgLen - The current message buffer length availabe for write.
//
void SNIGetSystemError(__out_opt SNI_ERROR * pSNIerror, __in DWORD dwNativeError, __in DWORD dwCurMsgLen)
{

	//	SqlClient recognizes only one value for system errors.	
	//
	pSNIerror->dwSNIError = SNI_ERROR_ID_FROM_STRING_ID(SNIE_SYSTEM);
	
	DWORD dwChars = 0;
	
	//	Needed for SNIX supporting Win9x: Win9x does not 
	//	support the Unicode version of the formatting fnc.  
	//
	if( !gfIsWin9x )
	{
		dwChars = FormatMessageW( FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
							      0,
							      dwNativeError,
							      0,	// Let MUI to fallback to proper language, See VSTS 195439 for details
							      (LPWSTR) pSNIerror->pszErrorMessage + dwCurMsgLen,
							      MAX_PATH - dwCurMsgLen,
								  NULL );
	}
	else
	{
		//	Call our helper function that will format ASCII, 
		//	and do the conversion.  
		//
		dwChars = FormatMessageWin9x( dwNativeError, 
									  (LPWSTR) pSNIerror->pszErrorMessage + dwCurMsgLen,
									  MAX_PATH - dwCurMsgLen ); 
	}
	
	pSNIerror->pszErrorMessage[dwCurMsgLen + dwChars] = 0x0;
	
}

void SNIGetLastError( __out_opt SNI_ERROR * pSNIerror)
{
	DWORD dwCurMsgLen = 0;

	if( !pSNIerror )
		return;
	
	Assert( TLS_OUT_OF_INDEXES != g_sniErrorTls.dwProviderIndex ); 
	Assert( TLS_OUT_OF_INDEXES != g_sniErrorTls.dwSNIErrorStringIdIndex ); 
	Assert( TLS_OUT_OF_INDEXES != g_sniErrorTls.dwNativeErrorIndex ); 

	if( ( TLS_OUT_OF_INDEXES == g_sniErrorTls.dwProviderIndex ) ||
		( TLS_OUT_OF_INDEXES == g_sniErrorTls.dwSNIErrorStringIdIndex ) ||
		( TLS_OUT_OF_INDEXES == g_sniErrorTls.dwNativeErrorIndex ) )
	{
		memset( pSNIerror, 0, sizeof(SNI_ERROR) );
		return; 
	}


	C_ASSERT( sizeof( DWORD ) <= sizeof( LPVOID ) ); 

	DWORD dwSNIErrorStringId = 
		SNITlsGetValue( g_sniErrorTls.dwSNIErrorStringIdIndex ); 

	if( 0 == dwSNIErrorStringId )
	{
		//	Most likely an error was never set.  Return a blank 
		//	structure.  
		//
		memset( pSNIerror, 0, sizeof(SNI_ERROR) );
		return; 
	}
	else if( SNI_STRING_ERROR_BASE > dwSNIErrorStringId )
	{
		dwSNIErrorStringId = SNIE_SYSTEM; 
	}

	DWORD dwProvider = 
		SNITlsGetValue( g_sniErrorTls.dwProviderIndex );

	if( INVALID_PROV < dwProvider )
	{
		dwProvider = INVALID_PROV; 
	}
	

	DWORD dwNativeError = 
		SNITlsGetValue( g_sniErrorTls.dwNativeErrorIndex ); 


	//	pSNIerror->dwSNIError is assigned further below.  
	//
	pSNIerror->Provider = static_cast<ProviderNum>(dwProvider); 
	pSNIerror->dwNativeError = dwNativeError; 

	//	SQL BU DT 329846: we are not using these fields anymore.  
	//	However, managed SqlClient has its own definition of the SNI_ERROR
	//	structure, so we will keep the members and set them to NULL
	//	to avoid destabilizing Whidbey so close to its Beta 2/RTM.  
	//	It used to be filled in DEBUG build only anyway.  
	//
	pSNIerror->pszFunction     	= NULL;
	pSNIerror->pszFileName		= NULL; 
	pSNIerror->dwLineNumber    	= 0;

	// Copy provider info to string.
	//
	SNILoadProviderName( dwProvider, 
						 pSNIerror->pszErrorMessage, 
						 MAX_PATH );

	dwCurMsgLen = static_cast<DWORD>(wcslenInWChars( pSNIerror->pszErrorMessage ));

	// These if NULL value, then we are processing OS errors
	//
	if( !SNI_STRING_IS_SYSTEM_ERROR(dwSNIErrorStringId) )
	{
		pSNIerror->dwSNIError = SNI_ERROR_ID_FROM_STRING_ID(dwSNIErrorStringId);

#ifdef SNI_BASED_CLIENT

		if(SNI_STRING_IS_LOCALDB_ERROR(dwSNIErrorStringId))
		{
			// These are LocalDB specific errors
			// Copy actual error string
			//
			DWORD dwLocalDBMsgLen = MAX_PATH - dwCurMsgLen;
			
			DWORD dwRet = LocalDB::getLocalDBErrorMessage(pSNIerror->dwNativeError,
									pSNIerror->pszErrorMessage + dwCurMsgLen,
									&dwLocalDBMsgLen);
			if(ERROR_SUCCESS != dwRet)
			{
				BidTrace1(ERROR_TAG _T("getLocalDBErrorMessage failed – %d{WINERR} \n"),dwRet);
				SNIGetSystemError(pSNIerror, dwNativeError, dwCurMsgLen);
			}
		}
		else
#endif	
		{
			// These are internal SNI-specific errors
			// Copy actual error string
			//
			SNILoadErrorMessage( dwSNIErrorStringId, 
								 pSNIerror->pszErrorMessage + dwCurMsgLen, 
								 MAX_PATH - dwCurMsgLen );

			pSNIerror->pszErrorMessage[MAX_PATH] = 0x0; // Ensure NULL terminated
		}

		// Reset the string length
		//
		dwCurMsgLen = static_cast<DWORD>(wcslenInWChars( pSNIerror->pszErrorMessage ));

		size_t cchRemaining = MAX_PATH - dwCurMsgLen;
		StringCchPrintf_lExW(pSNIerror->pszErrorMessage + dwCurMsgLen,
					MAX_PATH - dwCurMsgLen, NULL, &cchRemaining, 0,
				    0x80000000 & dwNativeError ? L" [x%08X]. " : L" [%d]. ", GetDefaultLocale(),
			    	dwNativeError);
		dwCurMsgLen = (DWORD)(MAX_PATH - cchRemaining);
		
	}
	else
	{
		SNIGetSystemError(pSNIerror, dwNativeError, dwCurMsgLen);
	}
}


void SNISetLastError( ProviderNum Provider,
					  DWORD       dwNativeError,
					  DWORD		  dwSNIError,
					  WCHAR     * pszFileName,
					  WCHAR     * pszFunction,
					  DWORD       dwLineNumber )
{
	SNI_ERROR * pTlsSNIerror      = 0;
	DWORD       dwCurMsgLen = 0;
	
	C_ASSERT( sizeof( ProviderNum ) <= sizeof( LPVOID ) ); 
	C_ASSERT( sizeof( DWORD ) <= sizeof( LPVOID ) ); 

	Assert( INVALID_PROV >= Provider );
	Assert( SNI_STRING_ERROR_BASE <= dwSNIError );
	Assert( TLS_OUT_OF_INDEXES != g_sniErrorTls.dwProviderIndex ); 
	Assert( TLS_OUT_OF_INDEXES != g_sniErrorTls.dwSNIErrorStringIdIndex ); 
	Assert( TLS_OUT_OF_INDEXES != g_sniErrorTls.dwNativeErrorIndex ); 


	if( ( TLS_OUT_OF_INDEXES == g_sniErrorTls.dwProviderIndex ) ||
		( TLS_OUT_OF_INDEXES == g_sniErrorTls.dwSNIErrorStringIdIndex ) ||
		( TLS_OUT_OF_INDEXES == g_sniErrorTls.dwNativeErrorIndex ) )
	{
		BidTrace0( ERROR_TAG _T("Uninitialized TLS index.\n") );
		return; 
	}

	if( !SNITlsSetValue( g_sniErrorTls.dwProviderIndex, Provider ) )
	{
		//	This assertion is used to catch unexpected system call errors.
		//
		Assert( 0 && " SNITlsSetValue failed – unexpected.\n" );
		BidTrace0( ERROR_TAG _T("SNITlsSetValue failed – unexpected.\n") );
	}
	
	if( !SNITlsSetValue( g_sniErrorTls.dwSNIErrorStringIdIndex, dwSNIError ) )
	{
		//	This assertion is used to catch unexpected system call errors.
		//
		Assert( 0 && " SNITlsSetValue failed – unexpected.\n" );
		BidTrace0( ERROR_TAG _T("SNITlsSetValue failed – unexpected.\n") );
	}

	if( !SNITlsSetValue( g_sniErrorTls.dwNativeErrorIndex, dwNativeError ) )
	{
		//	This assertion is used to catch unexpected system call errors.
		//
		Assert( 0 && " SNITlsSetValue failed – unexpected.\n" );
		BidTrace0( ERROR_TAG _T("SNITlsSetValue failed – unexpected.\n") );
	}
}


// FormatMessageWin9x
//
//	A helper function for formatting Unicode error messages for SNIX
//	on Win9x.  
//
//	Win9x do not support FormatMessageW, so we have to use FormatMessageA
//	and perform conversions ASCII<->UNICODE on the fly
//
//	Adapted from MDAC's SQLinterfacse\common\src\fmtutl.cpp W95FormatMsg(), 
//	which was copied from \ntdbms\ksource\serverma.c
//
//	Inputs: 
//		
//		dwMessageId	- requested message identifier
//		lpBuffer	- pointer to message buffer
//		nSize		- size of the buffer counted in WCHARs
//  
//  Returns: the number of WCHAR written (not counting null-terminator) 
//		if successful, otherwise 0. 
//

DWORD FormatMessageWin9x( DWORD		dwMessageId,
						  __out_ecount(nSize) LPWSTR	lpBuffer,
						  DWORD		nSize )
{
#ifdef _WIN64
	// BUG64BIT - I don't anticipate 64 bit Win9x and this should only
	// get called on Win9x.
	//
	Assert( 0 && " We do not expect this function to be called under WIN64\n" );
	BidTrace0( ERROR_TAG _T("We do not expect this function to be called under WIN64\n") );
	return 0;
#else

	char pszMessage[MAX_PATH+1];

	if ( !nSize || (nSize > CCH_ANSI_STRING(pszMessage)) )
	{
		//this is used to catch unexpected coding error.
		Assert( 0 && " Truncation of message possible due to coding error!\n" );
		BidTrace0( ERROR_TAG _T("Possible Truncation of message.\n") );
		return 0;  
	}

	nSize --;	// For terminating zero

	int cChars = FormatMessageA( FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
								 0,
								 dwMessageId,
								 0,	// Let MUI to fallback to proper language, See VSTS 195439 for details. DEVNOTE: if this needs to be ported to Orcas, please make sure this works.
								 pszMessage,
								 MAX_PATH,
								 NULL );
	//	This should never happen but let's check before writing 
	//	to be on the safe size.  
	//
	if ((cChars >= static_cast<int>(CCH_ANSI_STRING(pszMessage))) || (cChars > static_cast<int>(nSize)))
	{
		return 0; 
	}
	pszMessage[cChars] = 0;
	cChars = MultiByteToWideChar( CP_ACP,		// Use system ANSI codepage.
								  0,			// character-type options
								  pszMessage,	// MBCS string to map
								  cChars,		// number of bytes in MBCS string
								  lpBuffer,	// wide-character buffer
								  nSize );		// size of buffer in wide characters

	Assert (cChars <= (int) nSize);
	if ( cChars >= static_cast<int>(nSize) )
	{
		return 0; 
	}

	lpBuffer[cChars] = 0;
	return cChars;

#endif
}
