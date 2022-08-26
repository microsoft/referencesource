//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: open.hpp
// @Owner: petergv, nantu
// @Test: milu
//
// <owner current="true" primary="true">petergv</owner>
// <owner current="true" primary="false">nantu</owner>
//
// Purpose: Connection string parsing
//
// Notes: Everything in this file is shared with dbnetlib.dll ( dbnetlib has a separate copy of this file ).
//        So, special attention is needed whenever anything in this file needs to be changed.
//
//          
// @EndHeader@
//****************************************************************************

#ifndef _OPEN_HPP_
#define _OPEN_HPP_

#define MAX_INSTANCENAME_LENGTH 255
#define MAX_ALIAS_LENGTH	(MAX_NAME_SIZE+1+MAX_INSTANCENAME_LENGTH)
#define MAX_PROTOCOLNAME_LENGTH 10
#define MAX_PROTOCOLPARAMETER_LENGTH 255
#define DEFAULT_INSTANCE_NAME L"mssqlserver"

#define LOCALDB_WSTR L"(localDB)"
#define LOCALDB_WSTR_LEN sizeof(LOCALDB_WSTR)/sizeof(WCHAR)

#define LOCALDB_INST_WSTR L"(localDB)\\"
#define LOCALDB_INST_WSTR_LEN sizeof(LOCALDB_INST_WSTR)/sizeof(WCHAR)

extern WCHAR gwszComputerName[];
extern DWORD gdwSvrPortNum;
extern BOOL g_fSandbox;            // Defined in sni.cpp

// Converts from Unicode or Hex to Decimal integer
int Wcstoi(const WCHAR * wszStr);

// Skips white space and sees if theres chars after that
_inline BOOL IsBlank(__in __nullterminated WCHAR * pwStr)
{
	while(*pwStr && (*pwStr==L' ' || *pwStr==L'\t'))
		pwStr++;
	if(!*pwStr)	//end of string
		return true;
	return false;
}

// LocalHost checking
//
inline bool IsLocalHost( const WCHAR *wszServer)
{
OACR_WARNING_PUSH
OACR_WARNING_DISABLE(SYSTEM_LOCALE_MISUSE , " INTERNATIONALIZATION BASELINE AT KATMAI RTM. FUTURE ANALYSIS INTENDED. ")
	if( wcscmp(L".",wszServer) &&  
		_wcsicmp_l(L"(local)",wszServer, GetDefaultLocale()) &&
		_wcsicmp_l(L"localhost",wszServer, GetDefaultLocale()) &&
		CSTR_EQUAL != CompareStringW(LOCALE_SYSTEM_DEFAULT,
									 NORM_IGNORECASE|NORM_IGNOREWIDTH,
									 gwszComputerName, -1,
									 wszServer, -1))
OACR_WARNING_POP
	{
		return false;
	}
	
	return true;
}

// NOTE: The caller assumes ownership of the dynamically allocated copy
DWORD CopyConnectionString(__in LPCWSTR wszConnect, __out LPWSTR* pwszCopyConnect);

class ConnectParameter {
public:
	WCHAR m_wszServerName[MAX_NAME_SIZE + 1];
	WCHAR m_wszOriginalServerName[MAX_NAME_SIZE + 1];
	WCHAR m_wszInstanceName[MAX_INSTANCENAME_LENGTH + 1];
	WCHAR m_wszProtocolName[MAX_PROTOCOLNAME_LENGTH + 1];
	WCHAR m_wszProtocolParameter[MAX_PROTOCOLPARAMETER_LENGTH + 1];
	WCHAR m_wszAlias[MAX_ALIAS_LENGTH + 1];
	bool m_fCanUseCache;
	bool m_fStandardInstName;
	bool m_fParallel;
	TransparentNetworkResolutionMode m_TransparentNetworkResolution;
	int  m_TotalTimeout;

	ConnectParameter()
	{
		m_wszServerName[0]=L'\0';
		m_wszOriginalServerName[0]=L'\0';
		m_wszInstanceName[0]=L'\0';
		m_wszProtocolName[0]=L'\0';
		m_wszProtocolParameter[0]=L'\0';
		m_wszAlias[0]=L'\0';
		m_fCanUseCache=true;
		m_fStandardInstName=true;
		m_fParallel=false;
		m_TransparentNetworkResolution = TransparentNetworkResolutionMode::DisabledMode;
		m_TotalTimeout = SNIOPEN_TIMEOUT_VALUE;
	}

	~ConnectParameter()
	{
	}
			
	DWORD IsLocalDBConnectionString( __in LPCWSTR wszConnect, __out bool* fLocalDB)
	{
		BidxScopeAutoSNI1( SNIAPI_TAG _T( "wszConnect: '%ls'\n"), wszConnect);

		LPWSTR wszCopyConnect = NULL;
		DWORD dwRet = ERROR_SUCCESS;
				
		// Check for Null or empty connection string.
		// Note : We convey that this is not a LocalDB connection string but do not report failure,
		// 		since an empty connection string is valid (connects to localhost). 
		//		
		if(NULL == wszConnect || !wszConnect[0])
		{
			goto Exit;
		}

		// Create a copy of the actual string
		dwRet = CopyConnectionString(wszConnect, &wszCopyConnect);
		if (ERROR_SUCCESS != dwRet)
		{
			// BID already traced in CopyConnectionString
			goto Exit;
		}

		// Trim Leading whitespaces (This is not supposed to fail)
		//
		StrTrimLeftW_Sys(wszCopyConnect);

		if(!_wcsnicmp(wszCopyConnect, LOCALDB_WSTR, LOCALDB_WSTR_LEN))
		{
			// No instance Name specified
			//
			dwRet = ERROR_INVALID_PARAMETER;
			SNI_SET_LAST_ERROR(INVALID_PROV,SNIE_51,dwRet);
			BidTrace1(ERROR_TAG _T("LocalDB: No instance name specified. %d{WINERR}\n"), dwRet);
			goto Exit;
		}
		
		if(!_wcsnicmp(wszCopyConnect, LOCALDB_INST_WSTR, LOCALDB_INST_WSTR_LEN-1))
		{
			DWORD len = (DWORD) wcslenInWChars(wszCopyConnect);
			
			//	Check if instance Name exists
			//
			if((LOCALDB_INST_WSTR_LEN-1) == len)
			{
				// Error case where connection string is 
				// (localdb)\ - missing instance name
				//
				dwRet = ERROR_INVALID_PARAMETER;
				SNI_SET_LAST_ERROR(INVALID_PROV,SNIE_51,dwRet);
				BidTrace1(ERROR_TAG _T("LocalDB: Missing instance name. %d{WINERR}\n"), dwRet); 
				goto Exit;
			}

			*fLocalDB = true;
		}		
Exit:
		if(wszCopyConnect)
		{
			delete [] wszCopyConnect;
			wszCopyConnect = NULL;
		}

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
		return dwRet;
		
	}
	
		DWORD ParseConnectionString(const WCHAR * wszConnect, bool fParallel , TransparentNetworkResolutionMode transparentNetworkResolution, int totalTimeout)
	{
		m_TransparentNetworkResolution = transparentNetworkResolution;
		m_TotalTimeout = totalTimeout;

		return ParseConnectionString(wszConnect, fParallel);
	}
	
	DWORD ParseConnectionString( const WCHAR * wszConnect, bool fParallel )
	{
		BidxScopeAutoSNI1( SNIAPI_TAG _T( "wszConnect: '%s'\n"), wszConnect);

		BOOL  fLastErrorSet = FALSE; 
		WCHAR *pwPtr;
		WCHAR *pwEnd;
		BOOL  fHasParameter = FALSE;
		
		m_fParallel = fParallel;
		m_fCanUseCache = m_fCanUseCache && !m_fParallel;
		
		//$ NOTE: this is really the first place where we are manipulating
		// the connection string.  We are going to calculate the length of
		// the string (strlen) and then make a copy of the string for the
		// rest of the processing.  This is pretty important.  It means that
		// we can be assured that what we are processing is a well-formed
		// string.
		DWORD len = (DWORD) wcslen (wszConnect);
		WCHAR *wszCopyConnect = NewNoX(gpmo) WCHAR[len + 1];
		if( !wszCopyConnect )
		{
			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, ERROR_OUTOFMEMORY );

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_OUTOFMEMORY);

			return ERROR_OUTOFMEMORY;
		}
		
		DWORD dwRet = ERROR_INVALID_PARAMETER;
OACR_WARNING_PUSH
OACR_WARNING_DISABLE(SYSTEM_LOCALE_MISUSE , " INTERNATIONALIZATION BASELINE AT KATMAI RTM. FUTURE ANALYSIS INTENDED. ")
		DWORD cch = LCMapStringW(LOCALE_SYSTEM_DEFAULT,
							LCMAP_LOWERCASE,
							wszConnect,
							-1,
							wszCopyConnect,
							len + 1
							);
OACR_WARNING_POP
		Assert(cch <= len+1);
		if( 0 >= cch || cch > len+1)
		{
			goto ExitFunc;
			
		}		
		wszCopyConnect[len] = 0;

		if( ERROR_SUCCESS != StrTrimBothW_Sys(wszCopyConnect, len+1 , NULL, L" \t", ARRAYSIZE(L" \t")))
		{
			goto ExitFunc;
		}

		
		pwPtr=wszCopyConnect;

		//prefix
		if(pwEnd=wcschr(pwPtr,L':'))
		{
			*pwEnd = 0;

			//data before ':' can be part of ipv6 address
			Assert( 6<=MAX_PROTOCOLNAME_LENGTH );
			if ((pwEnd - pwPtr) > MAX_PROTOCOLNAME_LENGTH)
				goto ExitFunc;
			
			//Devnote: If this is IPv6 address, the num of bytes should be less than 5, so 10 bytes is ok.
			(void) StringCchCopyW(m_wszProtocolName,ARRAYSIZE(m_wszProtocolName),pwPtr);
			if( ERROR_SUCCESS != StrTrimBothW_Sys(m_wszProtocolName, wcslen(m_wszProtocolName) + 1 , NULL, L" \t", ARRAYSIZE(L" \t")))
			{
				goto ExitFunc;
			}
			
			//check if it is an ipv6 address
			if( pwEnd==pwPtr ||
				wcscmp(L"tcp",m_wszProtocolName) &&
				wcscmp(L"np",m_wszProtocolName) &&
				wcscmp(L"lpc",m_wszProtocolName) &&
				wcscmp(L"via",m_wszProtocolName) &&
				wcscmp(L"admin",m_wszProtocolName))
			{
				//restore
				*pwEnd = L':';	
				*m_wszProtocolName = L'\0';
			}
			else
			{				
				pwPtr = pwEnd + 1;
				if( ERROR_SUCCESS != StrTrimBothW_Sys(pwPtr, wcslen(pwPtr)+1, NULL, L" \t", ARRAYSIZE(L" \t")))
				{
					goto ExitFunc;
				}	
			}
		}

		//	This is a special case where the connection string is either
		//	'\\server\pipe\etc' or 'np:\\server\pipe\etc'
		

		//pipename
OACR_WARNING_PUSH
OACR_WARNING_DISABLE(SYSTEM_LOCALE_MISUSE , " INTERNATIONALIZATION BASELINE AT KATMAI RTM. FUTURE ANALYSIS INTENDED. ")
		if(CSTR_EQUAL == CompareStringW(LOCALE_SYSTEM_DEFAULT,
									  0,
									  pwPtr, 2,
									  L"\\\\",2))
OACR_WARNING_POP
		{
			// If there is a protocol specified and its not 'np'
			// report an error
			if( !m_wszProtocolName[0] )
				(void) StringCchCopyW( m_wszProtocolName, ARRAYSIZE(m_wszProtocolName),L"np");
			else if( wcscmp(L"np", m_wszProtocolName) )
				goto ExitFunc;

			if( wcslen(pwPtr ) > MAX_PROTOCOLPARAMETER_LENGTH )
				goto ExitFunc;			
			
			(void) StringCchCopyW(m_wszProtocolParameter,ARRAYSIZE(m_wszProtocolParameter),pwPtr);				

			WCHAR *wszServerStart = pwPtr+2;
			WCHAR *wszServerEnd =StrChrW_SYS(wszServerStart, (int) wcslen(wszServerStart),L'\\');
			if( !wszServerEnd )
				goto ExitFunc;

			*wszServerEnd = 0;
			
			if (IsBlank(wszServerStart))
				goto ExitFunc;

			// always copy the exact server name (as parsed from the named pipe) to the original name
			(void) StringCchCopyW(m_wszOriginalServerName,ARRAYSIZE(m_wszOriginalServerName),wszServerStart);
			
			if (IsLocalHost(wszServerStart))
			{
				(void) StringCchCopyW(m_wszServerName,ARRAYSIZE(m_wszServerName),gwszComputerName);
			}
			else
			{
				if (FAILED (StringCchCopyW(m_wszServerName,ARRAYSIZE(m_wszServerName),wszServerStart)))
					goto ExitFunc;
			}

			//	Parse instance Name from PipeName
			//
			//	Skip the "\pipe\"
			WCHAR *wszPipeEnd = StrChrW_SYS(wszServerEnd+1,(int)wcslen(wszServerEnd+1),L'\\');
			if(!wszPipeEnd)
				goto ExitFunc;

			WCHAR* wszInstStart=0, *wszInstEnd=0;

			//	stardard default instance, i.e. \\server\pipe\sql\query
			//
OACR_WARNING_PUSH
OACR_WARNING_DISABLE(SYSTEM_LOCALE_MISUSE , " INTERNATIONALIZATION BASELINE AT KATMAI RTM. FUTURE ANALYSIS INTENDED. ")
			if( CSTR_EQUAL == CompareStringW(LOCALE_SYSTEM_DEFAULT,
									  0,
									  wszPipeEnd+1,
									  (int) wcslen(wszPipeEnd+1),
									  L"sql\\query",
									  (int) ARRAYSIZE(L"sql\\query")-1))
OACR_WARNING_POP
			{		
				m_wszInstanceName[0]=0;
			}			
			// 	Possible standard named instance, i.e. "\\server\pipe\MSSQL$instancename\sql\query".
			//	Use lower case because we just map the wszConnect to lower case using LCMapStringA above.
			//	Note that FQDN, pipename and SPN is case in sensitive.
			//
OACR_WARNING_PUSH
OACR_WARNING_DISABLE(SYSTEM_LOCALE_MISUSE , " INTERNATIONALIZATION BASELINE AT KATMAI RTM. FUTURE ANALYSIS INTENDED. ")
			else if(CSTR_EQUAL == CompareStringW(LOCALE_SYSTEM_DEFAULT,
									  0,
									  wszPipeEnd+1,
									  (int) ARRAYSIZE(L"mssql$")-1,
									  L"mssql$",
									  (int) ARRAYSIZE(L"mssql$")-1))
OACR_WARNING_POP
			{

				//	standard named instance must finish with "\sql\query".
				//
				wszInstEnd= StrChrW_SYS(wszPipeEnd+1, (int) wcslen(wszPipeEnd+1),L'\\');				

OACR_WARNING_PUSH
OACR_WARNING_DISABLE(SYSTEM_LOCALE_MISUSE , " INTERNATIONALIZATION BASELINE AT KATMAI RTM. FUTURE ANALYSIS INTENDED. ")
				if( CSTR_EQUAL == CompareStringW(LOCALE_SYSTEM_DEFAULT,
									  0,
									  wszInstEnd+1,
									  (int) wcslen(wszInstEnd+1),
									  L"sql\\query",
									  (int) ARRAYSIZE(L"sql\\query")-1))
OACR_WARNING_POP
				{
					// standard named instance, i.e. \\server\pipe\MSSQL$instance\sql\query.
					// Find the start of the instance name
					//
					wszInstStart = StrChrW_SYS(wszPipeEnd+1, (int) wcslen(wszPipeEnd+1),L'$');
					Assert(wszInstStart);
					*wszInstEnd=0;
					
					if (FAILED (StringCchCopyW(m_wszInstanceName,ARRAYSIZE(m_wszInstanceName),wszInstStart+1)))
						goto ExitFunc;

				}
				// For non-standard pipename, use the string after "pipe\" as the instance name.
				//
				else
				{
					if (FAILED (StringCchPrintf_lW(m_wszInstanceName,ARRAYSIZE(m_wszInstanceName),L"pipe%s", GetDefaultLocale(), wszPipeEnd+1)))
						goto ExitFunc;

					m_fStandardInstName=false;
				}
			}
			// For non-standard pipename, use the string after "pipe\" as the instance name.
			//
			else
			{
				if (FAILED (StringCchPrintf_lW(m_wszInstanceName,ARRAYSIZE(m_wszInstanceName),L"pipe%s", GetDefaultLocale(),  wszPipeEnd+1)))
					goto ExitFunc;

				m_fStandardInstName=false;
			}				

			m_fCanUseCache = false;

			dwRet = ERROR_SUCCESS;

			goto ExitFunc;
		}


		// parameter.
		if( pwEnd = wcschr(pwPtr,L',') )
		{
			*pwEnd = 0;

			WCHAR * wszProtocolParameter = pwEnd + 1; 

			if( ERROR_SUCCESS != StrTrimBothW_Sys(wszProtocolParameter, wcslen(wszProtocolParameter)+1, NULL, L" \t", ARRAYSIZE(L" \t")))
			{
				goto ExitFunc;
			}

			// SQL BU DT 291063: for TCP and only TCP we allow 
			// parameter specification even if protocol is not
			// specified (for backward compatibility with netlibs 
			// 2.x).  
			if(m_wszProtocolName[0] == 0)
				(void) StringCchCopyW(m_wszProtocolName,ARRAYSIZE(m_wszProtocolName),L"tcp");

			
			// SQL BU DT 384073: "," is acceptable only for VIA and TCP
			// other protocols may fall through here, force fail exit
			if( wcscmp(L"via", m_wszProtocolName) &&  wcscmp(L"tcp", m_wszProtocolName) )
			{			
				goto ExitFunc;
			}
			

			// SQL BU DT 291063: 2.8 netlibs allow for an additional
			// TCP parameter ",urgent" but ignores it, and does not even 
			// validate it's actually ",urgent" (Webdata QFE bug 449).  
			// To preserve backward compatibility, SNI will do the same.  
			if( !wcscmp(L"tcp", m_wszProtocolName) )
			{
				if( WCHAR * pComma = wcschr(wszProtocolParameter, L',') )
				{
					*pComma = 0; 
					
					if( ERROR_SUCCESS != StrTrimBothW_Sys(wszProtocolParameter, wcslen(wszProtocolParameter)+1, NULL, L" \t", ARRAYSIZE(L" \t")))
					{
						goto ExitFunc;
					}
				}
			}

			// Error out on blank parameter and ",\"
			if( !(*wszProtocolParameter) || *wszProtocolParameter == L'\\' )
				goto ExitFunc;

			if(wcslen(wszProtocolParameter) > MAX_PROTOCOLPARAMETER_LENGTH)
				goto ExitFunc;
			
			if( IsBlank(pwPtr) )
				goto ExitFunc;

			//Trim off possible "\instancename" in format of "hostname,port\instancename" to
			//preserve MDAC behavior, i.e. ignore instancename when port is present.
			if(pwEnd = StrChrW_SYS(wszProtocolParameter, (int) wcslen(wszProtocolParameter),L'\\'))
				*pwEnd = 0;
			
			(void)StringCchCopyW(m_wszProtocolParameter,ARRAYSIZE(m_wszProtocolParameter),wszProtocolParameter);

			fHasParameter = true;
			m_fCanUseCache = false;
		}

		//instancename
		if( pwEnd=StrChrW_SYS(pwPtr, (int) wcslen(pwPtr),L'\\'))
		{
			*pwEnd = 0;
			if(wcslen(pwEnd+1) > MAX_INSTANCENAME_LENGTH)
				goto ExitFunc;

			if( IsBlank(pwPtr) )
				goto ExitFunc;

			if( ERROR_SUCCESS != StrTrimBothW_Sys(pwEnd+1, wcslen(pwEnd+1)+1, NULL, L" \t", ARRAYSIZE(L" \t")))
			{
				goto ExitFunc;
			}

			// SQL BU DT 338117: treat missing instance name as default
			// instance to preserve 2.8 netlibs behavior.  
			if( *(pwEnd+1) )
			{
				// Per testing it appears that 2.8 did not treat white 
				// space as a default instance.  
				if( IsBlank(pwEnd+1) )
					goto ExitFunc;

				//Only save "\instancename" when port number is not available to 
				//preserve MDAC behavior, i.e. ignore instancename when port is present.
				//It is needed in format of "hostname\instancename,port".
				if( !fHasParameter )
				{
					(void)StringCchCopyW(m_wszInstanceName,ARRAYSIZE(m_wszInstanceName),pwEnd+1);
					
				}
			}
		}

		// Instance name "mssqlserver" is reserved for default
		// instance, and is not allowed.  
		if( !wcscmp(DEFAULT_INSTANCE_NAME, m_wszInstanceName) )
			goto ExitFunc; 			

		if(wcslen(pwPtr) > MAX_NAME_SIZE)
			goto ExitFunc;


		//server name
		if( ERROR_SUCCESS != StrTrimBothW_Sys(pwPtr, wcslen(pwPtr)+1, NULL, L" \t", ARRAYSIZE(L" \t")))
		{
			goto ExitFunc;
		}
		
		// always copy the exact server name to the original name
		(void) StringCchCopyW(m_wszOriginalServerName,ARRAYSIZE(m_wszOriginalServerName),pwPtr);
		
		if(IsBlank(pwPtr) || IsLocalHost(pwPtr))
		{
			// For DAC use "localhost" instead of the server name.  
			if( wcscmp(L"admin", m_wszProtocolName) )
			{
				(void)StringCchCopyW(m_wszServerName,ARRAYSIZE(m_wszServerName),gwszComputerName);
			}
			else
			{
				(void)StringCchCopyW(m_wszServerName,ARRAYSIZE(m_wszServerName),L"localhost");
			}
		}
		else
		{
			(void)StringCchCopyW(m_wszServerName,ARRAYSIZE(m_wszServerName),pwPtr);
		}

		// Check if lpc was the protocol and the servername was NOT
		// the local computer - in that case, error out
		if( !wcscmp(L"lpc", m_wszProtocolName) && !IsLocalHost(m_wszServerName) )
		{
			Assert( dwRet == ERROR_INVALID_PARAMETER );
			
			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_41, dwRet );

			fLastErrorSet = TRUE; 

			goto ExitFunc;
		}
		
		(void)StringCchCopyW(m_wszAlias,ARRAYSIZE(m_wszAlias),m_wszServerName);

		if(m_wszInstanceName[0])
		{
			(void)StringCchCatW(m_wszAlias,ARRAYSIZE(m_wszAlias),L"\\");
			(void)StringCchCatW(m_wszAlias,ARRAYSIZE(m_wszAlias), m_wszInstanceName);
			if( !wcscmp(L"lpc",m_wszProtocolName) 
				|| !wcscmp(L"admin",m_wszProtocolName))
				m_fCanUseCache = false;
		}
		else
		{
			m_fCanUseCache = false;
		}

		if( fParallel )
		{
			if( !wcscmp(L"np",m_wszProtocolName) ||
				!wcscmp(L"via",m_wszProtocolName) ||
				!wcscmp(L"lpc",m_wszProtocolName) ||
				!wcscmp(L"admin",m_wszProtocolName) )
			{
				dwRet = ERROR_INVALID_PARAMETER;
				SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_49, dwRet );
				fLastErrorSet = TRUE; 
				goto ExitFunc;
			}
			else if( L'\0' == m_wszProtocolName[0] )
			{
				HRESULT hr = StringCchCopyW(m_wszProtocolName,ARRAYSIZE(m_wszProtocolName),L"tcp");
				if( FAILED(hr) )
				{
					// We don't expect this to fail for either of the stated reasons on MSDN...
					Assert( STRSAFE_E_INSUFFICIENT_BUFFER != hr );
					Assert( STRSAFE_E_INVALID_PARAMETER != hr );
					BidTrace1( ERROR_TAG _T("StringCchCopyW failed: %d{HRESULT}.\n"), hr);
					
					dwRet = ERROR_INVALID_PARAMETER;
					SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwRet );
					fLastErrorSet = TRUE; 
					goto ExitFunc;
				}
			}
		}
		dwRet = ERROR_SUCCESS;

	ExitFunc:

		delete [] wszCopyConnect;

		if( ERROR_SUCCESS != dwRet )
		{
			Assert( dwRet == ERROR_INVALID_PARAMETER );

			if( !fLastErrorSet )
			{
				SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_25, dwRet );
			}
		}
		else
		{
			Assert(m_wszServerName[0]);
			BidTraceU8( SNI_BID_TRACE_ON, SNI_TAG 
				_T("m_wszProtocolName: '%s', ")
				_T("m_wszServerName: '%s', ")
				_T("m_wszInstanceName: '%s', ")
				_T("m_wszProtocolParameter: '%s', ")
				_T("m_wszAlias: '%s', ")
				_T("m_fCanUseCache: %d{bool}, ")
				_T("m_fStandardInstName: %d{bool}, ")
				_T("m_wszOriginalServerName: '%s'\n"), 
				m_wszProtocolName , 
				m_wszServerName, 
				m_wszInstanceName, 
				m_wszProtocolParameter, 
				m_wszAlias, 
				m_fCanUseCache,
				m_fStandardInstName,
				m_wszOriginalServerName);
		}
		
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

		return dwRet;
	}
};

class ProtElem
{
private:
	ProviderNum m_ProviderNum;
	
public:

	//	Provider number for setting SNI last error.  
	//	Use m_ProviderNum if equal to INVALID_PROV.  
	//
	ProviderNum m_ProviderToReport;
	WCHAR 		m_wszServerName[MAX_NAME_SIZE+1];
	WCHAR 		m_wszOriginalServerName[MAX_NAME_SIZE+1];

	union
	{
		struct
		{
			WCHAR wszPort[MAX_PROTOCOLPARAMETER_LENGTH+1];
			bool fParallel;
			TransparentNetworkResolutionMode transparentNetworkIPResolution;
			int  totalTimeout;
		} Tcp;

		struct
		{
			WCHAR Pipe[MAX_PATH+1];
		} Np;

		struct
		{
			WCHAR Alias[MAX_ALIAS_LENGTH+1];
		} Sm;

		struct
		{
			USHORT Port;
			WCHAR Vendor[MAX_NAME_SIZE+1];
			WCHAR Host[MAX_NAME_SIZE+1];
			WCHAR Param[MAX_PROTOCOLPARAMETER_LENGTH+1];
		} Via;
	};
	
	ProtElem 	* m_Next;

	ProtElem()
	{
		m_ProviderNum = INVALID_PROV;

		m_ProviderToReport = INVALID_PROV; 

		m_wszServerName[0] = 0x00; 
		m_wszOriginalServerName[0] = 0x00;
		
		m_Next = 0;
	}

	~ProtElem()
	{
	}

	DWORD Init( const WCHAR wszServer[], const WCHAR wszOriginalServerName[] )
	{
		BidxScopeAutoSNI2( SNIAPI_TAG _T( "wszServer: '%s', wszOriginalServerName: '%s'\n"), wszServer, wszOriginalServerName);

		Assert( wszServer[0] );

		DWORD dwRet = (DWORD) StringCchCopyW (m_wszServerName, ARRAYSIZE(m_wszServerName), wszServer);

		if( ERROR_SUCCESS != dwRet )
		{
			goto Exit;
		}

		
		dwRet = (DWORD) StringCchCopyW (m_wszOriginalServerName, ARRAYSIZE(m_wszOriginalServerName), wszOriginalServerName);

	Exit:
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

		return dwRet;
	}

	void SetProviderNum(ProviderNum providerNum)
	{
		m_ProviderNum = providerNum;

		switch( providerNum )
		{
			case TCP_PROV:
				Tcp.fParallel = false;
				Tcp.transparentNetworkIPResolution = TransparentNetworkResolutionMode::DisabledMode;
				Tcp.totalTimeout = SNIOPEN_TIMEOUT_VALUE;
				break;
			default:
				// nothing to be done for other protocols
				break;
		}
	}

	ProviderNum GetProviderNum() const
	{
		return m_ProviderNum;
	}

	static DWORD MakeProtElem (__in LPWSTR wszConnect, __out ProtElem ** ppProtElem)
	{
		BidxScopeAutoSNI2( SNIAPI_TAG _T( "wszConnect: '%s', ppProtElem: %p\n"), wszConnect, ppProtElem);

		*ppProtElem = 0;
		
		DWORD dwRet;
		
		ConnectParameter * pConnectParams = NewNoX(gpmo) ConnectParameter;
		if( !pConnectParams )
		{
			dwRet = ERROR_OUTOFMEMORY;

			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, dwRet );

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

			return dwRet;
		}

		// ProtElem::MakeProtElem is only called by direct consumer calls to SNIOpenSync, which does not take an fParallel parameter - so, always pass False here.
		// While SNIOpenSyncEx calls also go through SNIOpenSync, they will not call MakeProtElem - the ProtElem gets constructed by a different call to ParseConnectionString
		dwRet = pConnectParams->ParseConnectionString( wszConnect, false );
		
		if( ERROR_SUCCESS != dwRet )
		{
			delete pConnectParams;

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

			return dwRet;
		}

		ProtElem *pProtElem = NewNoX(gpmo) ProtElem();

		if( !pProtElem )
		{
			delete pConnectParams;
			
			dwRet = ERROR_OUTOFMEMORY;

			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, dwRet );

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

			return dwRet;
		}

		dwRet = pProtElem->Init( pConnectParams->m_wszServerName, pConnectParams->m_wszOriginalServerName ); 

		if ( ERROR_SUCCESS != dwRet )
		{
			goto ErrorExit; 
		}

		dwRet = ERROR_INVALID_PARAMETER;

		switch( pConnectParams->m_wszProtocolName[0] )
		{
			case L't':

				if( wcscmp(L"tcp",pConnectParams->m_wszProtocolName))
					goto ErrorExit;

				pProtElem->SetProviderNum(TCP_PROV);

				if(pConnectParams->m_wszInstanceName[0])
				{
					goto ErrorExit;
				}
		
				if(pConnectParams->m_wszProtocolParameter[0])
				{
					if( 0 == Wcstoi(pConnectParams->m_wszProtocolParameter))
						goto ErrorExit;

					C_ASSERT(sizeof(pProtElem->Tcp.wszPort) == sizeof(pConnectParams->m_wszProtocolParameter));
					memcpy(pProtElem->Tcp.wszPort, pConnectParams->m_wszProtocolParameter, sizeof(pProtElem->Tcp.wszPort));
				}
				else 
				{
					goto ErrorExit;
				}

				break;

			default:

				goto ErrorExit;
		}

		dwRet = ERROR_SUCCESS;

	ErrorExit:

		delete pConnectParams;

		if( dwRet == ERROR_SUCCESS )
		{
			*ppProtElem = pProtElem;
		}
		else
		{
			delete pProtElem;
			
			Assert( dwRet == ERROR_INVALID_PARAMETER );

			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_25, dwRet );
		}
		
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

		return dwRet;
	}

};

class ProtList
{
public:
	ProtElem * Head;

	ProtList():Head(0){}
	~ProtList(){
		DeleteAll();
	}
    void AddTail(__out ProtElem * newElement){
		BidxScopeAutoSNI1( SNIAPI_TAG _T( "newElement: %p{ProtElem*}\n"), newElement);
		
		ProtElem **ppTmp = &Head;
		while(*ppTmp)
			ppTmp=&((*ppTmp)->m_Next);
		*ppTmp=newElement;
		newElement->m_Next=0;
    }

    void AddHead(__out ProtElem * newElement){
		BidxScopeAutoSNI1( SNIAPI_TAG _T( "newElement: %p{ProtElem*}\n"), newElement);
		newElement->m_Next=Head;
		Head=newElement;
    }

    ProtElem * Find(ProviderNum ePN){
		BidxScopeAutoSNI1( SNIAPI_TAG _T( "ePN: %d{ProviderNum}\n"), ePN);
		
		ProtElem *tmpProt = Head;
		while( tmpProt ){
			if( tmpProt->GetProviderNum() == ePN )
				break;
			tmpProt=tmpProt->m_Next;
		}

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%p{ProtElem*}\n"), tmpProt);

		return tmpProt;
    }

	ProtElem * RemoveProt(ProviderNum ePN){
		BidxScopeAutoSNI1( SNIAPI_TAG _T( "ePN: %d{ProviderNum}\n"), ePN);
		
		ProtElem **ppTmp = &Head;
		ProtElem *pRet = 0;
		while( *ppTmp ){
			if( (*ppTmp)->GetProviderNum() == ePN ){
				pRet = *ppTmp;
				*ppTmp=pRet->m_Next;
				break;
			}
			ppTmp=&((*ppTmp)->m_Next);
		}

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%p{ProtElem*}\n"), pRet);
		return pRet;
	}

	ProtElem * RemoveFirst(void){
		BidxScopeAutoSNI0( SNIAPI_TAG _T( "\n"));
		
		ProtElem *pRet =Head;
		if(Head)
			Head=Head->m_Next;

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%p{ProtElem*}\n"), pRet);
		
		return pRet;
	}
	void DeleteAll(){
		BidxScopeAutoSNI0( SNIAPI_TAG _T( "\n"));
		
		ProtElem *pTmp;
		while(pTmp=RemoveFirst())
			delete pTmp;
	}

};

#endif
