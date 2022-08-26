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

#include "snipch.hpp"
#include "tcp.hpp"
#include "ssrp.hpp"
#include "reg.hpp"
#include "sni_spn.hpp"
#include "LocalDB.hpp"

#ifndef CPL_ASSERT
#define CPL_ASSERT DASSERT_COMPILER
#endif

DWORD MakeProtocolList( 	__inout const ConnectParameter * pConnectParams,
								ProtList               * pProtList,
								__out bool                   * pfSsrpRequired )
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T( "pConnectParams: %p, pProtList: %p, pfSsrpRequired: %p\n"),
									pConnectParams, pProtList, pfSsrpRequired);

	ProtElem * pProtElem = 0;

	*pfSsrpRequired = false;
	
	//check if there is a protocol specified
	if( pConnectParams->m_wszProtocolName[0] )
	{
		pProtElem = NewNoX(gpmo) ProtElem();

		if( !pProtElem )
		{
			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, ERROR_OUTOFMEMORY );

			BidTraceU2( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}, *pfSsrpRequired: %d{BOOL}\n"), ERROR_OUTOFMEMORY, *pfSsrpRequired);

			return ERROR_OUTOFMEMORY;
		}

		if( ERROR_SUCCESS != pProtElem->Init( pConnectParams->m_wszServerName, pConnectParams->m_wszOriginalServerName) )
		{
			goto ErrorExit; 
		}

		switch(pConnectParams->m_wszProtocolName[0])
		{
			case L'a':
				if(!wcscmp(L"admin",pConnectParams->m_wszProtocolName))
				{
					pProtElem->SetProviderNum(TCP_PROV);

					// Here, the port is provided
					if(pConnectParams->m_wszProtocolParameter[0])
					{
						goto ErrorExit;
					}

					// Default instance - set the port to 1434
					else if( !pConnectParams->m_wszInstanceName[0] )
					{
						(void)StringCchPrintfW( pProtElem->Tcp.wszPort, ARRAYSIZE(pProtElem->Tcp.wszPort), L"%d", 1434);
					}

					// Named instance - do SSRP
					else
					{
						USHORT port;
						
						if( !SSRP::GetAdminPort( pConnectParams->m_wszServerName, 
												pConnectParams->m_wszInstanceName, 
												&port) )
						{
							delete pProtElem;
							pProtElem = 0;

							SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_43, ERROR_FAIL );
							
							BidTraceU2( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}, *pfSsrpRequired: %d{BOOL}\n"), ERROR_FAIL, *pfSsrpRequired);

							return ERROR_FAIL;
						}

						(void) StringCchPrintfW( pProtElem->Tcp.wszPort, ARRAYSIZE(pProtElem->Tcp.wszPort), L"%d", port);
					}
						
				}else
					goto ErrorExit;

				break;

			case L't':
				if( !wcscmp(L"tcp",pConnectParams->m_wszProtocolName))
				{
					if( ERROR_SUCCESS != 
						GetProtocolDefaults(
							pProtElem, L"tcp", pConnectParams->m_wszServerName ) )
					{
							goto ErrorExit;
					}

					if(pConnectParams->m_wszProtocolParameter[0])
					{
						WCHAR * tmp = const_cast<WCHAR*>(pConnectParams->m_wszProtocolParameter);

						//SQL BU 396129
						//Trim off trailing spaces for TCP port.
						//
						while ( *tmp && ( *tmp != L' ' && *tmp != L'\t' ))
						{
							tmp++;
						}

						*tmp = 0;
						
						if( 0 == Wcstoi(pConnectParams->m_wszProtocolParameter))
							goto ErrorExit;
						
						(void) StringCchCopyW(pProtElem->Tcp.wszPort,ARRAYSIZE(pProtElem->Tcp.wszPort),pConnectParams->m_wszProtocolParameter);
					}
					else if(pConnectParams->m_wszInstanceName[0])
					{
						*pfSsrpRequired = true;
					}

					if(pConnectParams->m_fParallel)
					{
						pProtElem->Tcp.fParallel = true;
					}

					if (pConnectParams->m_TransparentNetworkResolution != TransparentNetworkResolutionMode::DisabledMode)
					{
						pProtElem->Tcp.transparentNetworkIPResolution = pConnectParams->m_TransparentNetworkResolution;
						pProtElem->Tcp.totalTimeout = pConnectParams->m_TotalTimeout;
					}
				}else
					goto ErrorExit;
				
				break;
				
			case L'v':
				if(!wcscmp(L"via",pConnectParams->m_wszProtocolName))
				{
					if( ERROR_SUCCESS != 
						GetProtocolDefaults(
							pProtElem, L"via", (WCHAR*) pConnectParams->m_wszServerName ) )
					{
							goto ErrorExit;
					}

					if(pConnectParams->m_wszProtocolParameter[0])
					{
						(void)StringCchCopyW(pProtElem->Via.Param,ARRAYSIZE(pProtElem->Via.Param),pConnectParams->m_wszProtocolParameter);
					}
					else if(pConnectParams->m_wszInstanceName[0])
					{
						*pfSsrpRequired = true;
					}
						
				}else
					goto ErrorExit;
				break;
				
			case L'n':
				if(!wcscmp(L"np",pConnectParams->m_wszProtocolName))
				{
					if( ERROR_SUCCESS != 
						GetProtocolDefaults(
							pProtElem, L"np", pConnectParams->m_wszServerName ) )
					{
							goto ErrorExit;
					}

					if(pConnectParams->m_wszProtocolParameter[0])
					{
						if(wcsncmp(pConnectParams->m_wszProtocolParameter,L"\\\\",2))
							goto ErrorExit;
						(void)StringCchCopyW(pProtElem->Np.Pipe,ARRAYSIZE(pProtElem->Np.Pipe),pConnectParams->m_wszProtocolParameter);
					}
					else if(pConnectParams->m_wszInstanceName[0])
					{
						*pfSsrpRequired = true;
					}
				}else
					goto ErrorExit;
				break;
				
			case L'l':
				if(!wcscmp(L"lpc",pConnectParams->m_wszProtocolName))
				{
					pProtElem->SetProviderNum(SM_PROV);

					(void)StringCchCopyW(pProtElem->Sm.Alias,ARRAYSIZE(pProtElem->Sm.Alias),pConnectParams->m_wszAlias);
					
					if(pConnectParams->m_wszProtocolParameter[0])
						goto ErrorExit;
				}else
					goto ErrorExit;
				break;
				
			default:
					goto ErrorExit;
		}

		//
		// when user has specified a protocol then list should already be empty 
		//

		Assert( !pProtList->Head );

		pProtList->AddHead(pProtElem);

		pProtElem = 0;	//it is part of the list we shouldn't be able to delete it in ErrorExit

	}
	else if(pConnectParams->m_wszInstanceName[0])
	{
		ProtElem *pSm;
		
		pSm = pProtList->Find(SM_PROV);

		if( pSm )
		{
			(void)StringCchCopyW(pSm->Sm.Alias,ARRAYSIZE(pSm->Sm.Alias),pConnectParams->m_wszAlias);
		}

		*pfSsrpRequired = true;
	}

	BidTraceU2( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}, *pfSsrpRequired: %d{BOOL}\n"), ERROR_SUCCESS, *pfSsrpRequired);

	return ERROR_SUCCESS;

ErrorExit:
	
	delete pProtElem;

	SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_25, ERROR_INVALID_PARAMETER );

	BidTraceU2( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}, *pfSsrpRequired: %d{BOOL}\n"), ERROR_INVALID_PARAMETER, *pfSsrpRequired);

	return ERROR_INVALID_PARAMETER;
}

__success(ERROR_SUCCESS == return) 
DWORD Connect(  __in ConnectParameter *pConnectParams, 
					__in SNI_CLIENT_CONSUMER_INFO * pClientConsumerInfo,
					__in ProtElem *pProtElem,
					__deref_out SNI_Conn ** ppConn,
					__in int timeout)
{
	BidxScopeAutoSNI5( SNIAPI_TAG _T( "pConnectParams: %p{ConnectParameter*}, ")
		 					  _T("pConsumerInfo: %p{SNI_CLIENT_CONSUMER_INFO*}, ")
		 					  _T("pProtElem: %p{ProtElem*}, ")
							  _T("ppConn: %p{SNI_Conn**}, ")
							  _T("timeout: %d{int}\n"),
								pConnectParams,
								pClientConsumerInfo, 
								pProtElem, 
								ppConn, 
								timeout);
	DWORD dwRet = ERROR_SUCCESS;

	dwRet = SNIOpenSync( &(pClientConsumerInfo->ConsumerInfo), NULL, pProtElem, ppConn, pClientConsumerInfo->fSynchronousConnection,timeout);

	if( ERROR_SUCCESS == dwRet && pClientConsumerInfo->wszSPN && 0 < pClientConsumerInfo->cchSPN )
	{
		Assert(rgProvInfo[TCP_PROV].fInitialized);	// need TCP to be initialized to reverse DNS lookup.
		Assert(pProtElem->m_wszServerName[0]);		// Server Name must be non-blank since we connected.
		
		WCHAR wszDnsFQDN[NI_MAXHOST];

		dwRet = Tcp::GetDnsName( pProtElem->m_wszServerName, wszDnsFQDN, ARRAYSIZE( wszDnsFQDN ));

		if( dwRet != ERROR_SUCCESS)
		{
			// Reverse lookup failed, we need to fix a FQDN. For numaric IP address, use IP address to compose SPN,
			// see design spec for details; otherwise, use the servername directly.
			if (FAILED (StringCchPrintf_lW(wszDnsFQDN, NI_MAXHOST, L"%s", GetDefaultLocale(),pProtElem->m_wszServerName)))
			{
				dwRet = ERROR_INVALID_PARAMETER;

				SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_44, dwRet );

				Assert( *ppConn );

				SNIClose(*ppConn);

				*ppConn = NULL;
					
				goto Exit;
			}
		
			dwRet = ERROR_SUCCESS;
				
		}

		USHORT usPort = 0;
		pClientConsumerInfo->wszSPN[0]=L'\0';

		switch(pProtElem->GetProviderNum())
		{
			case TCP_PROV:
				{
					
					usPort = (USHORT)_wtoi_l(pProtElem->Tcp.wszPort, GetDefaultLocale());

					if(!wcscmp(L"admin", pConnectParams->m_wszProtocolName))
					{
						//	For admin connection, i.e. prefix with "admin", the instance name is always available from the
						//	connection string. It can be blank if it is default instance. Admin connection doesn't allow 
						//    "admin:server,port or "admin:server\instance,port". Thus the m_szProtocolParameter must be 
						//	blank.
						//
						Assert( 0 == pConnectParams->m_wszProtocolParameter[0]);
						
						dwRet = SNI_Spn::MakeSpn(wszDnsFQDN, pConnectParams->m_wszInstanceName, 0, (TCHAR*)pClientConsumerInfo->wszSPN, pClientConsumerInfo->cchSPN);
					}
					else
					{
						//Force to use port for TCP connection other than admin by passing NULL as instanceName.
						//In katmai+2, consider to use the instance name.
						//
						dwRet = SNI_Spn::MakeSpn(wszDnsFQDN, NULL,usPort, (TCHAR*)pClientConsumerInfo->wszSPN, pClientConsumerInfo->cchSPN);
					}			

					break;
				
				}	
				case NP_PROV:
				case SM_PROV:
				{
#ifdef SNIX
					// TFS task 722353: RANU fails on XP if SPN is passed to ISC() because of a Windows bug
					// where local loopback cannot be detected if ":" is part of the SPN and auth uses Kerberos instead of NTLM
					// Kerberos token generated causes a failure in Security component's code.
					//
					// For XP only, revert to the DEV9/SNI Yukon behavior.
					if ((g_osviSNI.dwMajorVersion == 5) && (g_osviSNI.dwMinorVersion == 1))
					{
						pClientConsumerInfo->wszSPN[0]=L'\0';
					}
					else
#endif
					{
						//Alway use instance name for NP and SM.
						dwRet = SNI_Spn::MakeSpn(wszDnsFQDN, pConnectParams->m_wszInstanceName, 0 , (TCHAR*)pClientConsumerInfo->wszSPN, pClientConsumerInfo->cchSPN);
					}
					break;
				}
				
				case VIA_PROV:
				{
					//	if a port is given and instance name is blank, we don't know if it is a name instance or default.
					//	Force to use port for SPN. For instance, "server,port". VIA can have NIC:PORT as param, use it
					//   instead of the port if so.
					//
					if(pConnectParams->m_wszProtocolParameter[0] && !pConnectParams->m_wszInstanceName[0])
					{
						if(0 < pProtElem->Via.Port)
						{
							dwRet = SNI_Spn::MakeSpn(wszDnsFQDN, NULL,  pProtElem->Via.Port, (TCHAR*)pClientConsumerInfo->wszSPN, pClientConsumerInfo->cchSPN);
						}
						else
						{
							Assert(pProtElem->Via.Param[0]);
							
							dwRet = SNI_Spn::MakeSpn(wszDnsFQDN, pProtElem->Via.Param,  0, (TCHAR*)pClientConsumerInfo->wszSPN, pClientConsumerInfo->cchSPN);
						}
					}
					else
					{
						dwRet = SNI_Spn::MakeSpn(wszDnsFQDN, pConnectParams->m_wszInstanceName, 0, (TCHAR*)pClientConsumerInfo->wszSPN, pClientConsumerInfo->cchSPN);
					}
			
					break;
				}

				default:
				{
					dwRet = ERROR_INVALID_PARAMETER;
					
					break;				
				}
		}

		if( ERROR_SUCCESS != dwRet )
		{
			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_44, dwRet );

			Assert( *ppConn );

			SNIClose(*ppConn);

			*ppConn = NULL;
		}
	}

Exit:

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

//
// This function will check if there is a cached entry and if so it will try to
// to connect. If it cannot connect to server for some reason it will return false.
// Exact error is not important and that's why it returns a boolean value.
//

__success(return) 
bool ConnectUsingCache( __in ConnectParameter *pConnectParams, 
								__in ProtList *pProtOrder,  
								__in SNI_CLIENT_CONSUMER_INFO * pClientConsumerInfo,
								__deref_out SNI_Conn ** ppConn,
								__in int timeout)
{
	BidxScopeAutoSNI5( SNIAPI_TAG _T( "pConnectParams: %p, ")
								_T("pProtOrder: %p, ")
								_T("pConsumerInfo: %p, ")
								_T("ppConn: %p, ")
								_T("timeout: %d\n"),
									pConnectParams, 
									pProtOrder, 
									pClientConsumerInfo, 
									ppConn, 
									timeout);

	ProtElem cacheElem;

	DWORD dwStart = GetTickCount();
	int timeleft = timeout;

	if( ERROR_SUCCESS != cacheElem.Init( pConnectParams->m_wszServerName, pConnectParams->m_wszOriginalServerName ) )
	{
		BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("false\n"));

		return false;
	}

	Assert( pConnectParams->m_fCanUseCache );

	if( !LastConnectCache::GetEntry( pConnectParams->m_wszAlias, &cacheElem ))
	{
		BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("false\n"));

		return false;
	}

	// if there is a protocol specified, it should match the one in cache entry
	// Note: m_szProtocolName is in lower case, as such CompareStringA can use case sensitive 
	// comparison directly.
	//
	if( pConnectParams->m_wszProtocolName[0] )
	{
		WCHAR *wszProviderName;
		
		switch( cacheElem.GetProviderNum() )
		{
			case HTTP_PROV:
				wszProviderName =L"http";
				break;

			case NP_PROV:
				wszProviderName =L"np";
				break;

			case SM_PROV:
				wszProviderName =L"lpc";
				break;

			case TCP_PROV:
				wszProviderName =L"tcp";
				break;

			case VIA_PROV:
				wszProviderName =L"via";
				break;

			default:
				wszProviderName = NULL;
				break;
		}

OACR_WARNING_PUSH
OACR_WARNING_DISABLE(SYSTEM_LOCALE_MISUSE , " INTERNATIONALIZATION BASELINE AT KATMAI RTM. FUTURE ANALYSIS INTENDED. ")
		if( !wszProviderName || 
			CSTR_EQUAL != CompareStringW(LOCALE_SYSTEM_DEFAULT,
									 NORM_IGNOREWIDTH,
									 wszProviderName, -1,
									 pConnectParams->m_wszProtocolName, -1))
OACR_WARNING_POP
		{
			BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("false\n"));

			return false;
		}
	}
	else if( pProtOrder->Find(cacheElem.GetProviderNum()) == NULL)
	{
		LastConnectCache::RemoveEntry( pConnectParams->m_wszAlias );

		BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("false\n"));

		return false;
	}

	// Adjust the timeout.
	// Do NOT fail if timeleft is less than 0. Only TCP honors timeout value and fails accordingly.
	if( INFINITE != timeout )
	{
		timeleft = timeout - (DWORD)(GetTickCount() - dwStart);
		if ( INFINITE == timeleft ) timeleft -= 1;  // avoid infinite.
		BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("timeout: %d\n"), timeleft);
	}

	if( ERROR_SUCCESS != Connect( pConnectParams, pClientConsumerInfo, &cacheElem, ppConn, timeleft))
	{
		LastConnectCache::RemoveEntry( pConnectParams->m_wszAlias );

		BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("false\n"));

		return false;
	}

	BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("true\n"));

	return true;
}

//DEVNOTE: 
//!!!! Need to match PrefixEnum. Originally, SNAC RTM does not prefix lpc if network 
//attribute is specified with sharememory. To keep the same behavior, we don't 
//prefix "lpc"" as well.
//
const WCHAR* g_rgwszPrefix[INVALID_PREFIX]={L"",L"",L"tcp:",L"np:",L"via:"};

__success(ERROR_SUCCESS == return)
DWORD SNIOpenSyncEx( __inout SNI_CLIENT_CONSUMER_INFO * pClientConsumerInfo,
						  __deref_out SNI_Conn ** ppConn)
{
	BidxScopeAutoSNI10( SNIAPI_TAG _T( "pClientConsumerInfo: %p{SNI_CLIENT_CONSUMER_INFO*}, ")
								_T("ppConn: %p{SNI_Conn**}, ")
								_T( "pClientConsumerInfo->wszConnectionString: \"%ls\"{LPWSTR}, ")
								_T( "pClientConsumerInfo->networkLibrary: %d{PrefixEnum}, ")
								_T( "pClientConsumerInfo->wszSPN: %p{LPWSTR}, ")
								_T( "pClientConsumerInfo->cchSPN: %d{DWORD}, ")
								_T( "pClientConsumerInfo->wszInstanceName: %p{LPWSTR}, ")
								_T( "pClientConsumerInfo->cchInstanceName: %d{DWORD}, ")
								_T( "pClientConsumerInfo->fOverrideLastConnectCache: %d{BOOL}, ")
								_T( "pClientConsumerInfo->fSynchronousConnection: %d{BOOL}\n"),
								pClientConsumerInfo,
								ppConn,
								pClientConsumerInfo->wszConnectionString,
								pClientConsumerInfo->networkLibrary,
								pClientConsumerInfo->wszSPN,
								pClientConsumerInfo->cchSPN,
								pClientConsumerInfo->szInstanceName,
								pClientConsumerInfo->cchInstanceName,
								pClientConsumerInfo->fOverrideLastConnectCache,
								pClientConsumerInfo->fSynchronousConnection);
	BidTraceU7( SNI_BID_TRACE_ON, SNIAPI_TAG _T("pClientConsumerInfo->timeout: %d{int}, ")
								_T("pClientConsumerInfo->fParallel: %d{BOOL}, ")
								_T("pClientConsumerInfo->ConsumerInfo.DefaultUserDataLength: %d{LONG}, ")
								_T("pClientConsumerInfo->ConsumerInfo.ConsumerKey: %p{LPVOID}, ")
								_T("pClientConsumerInfo->ConsumerInfo.fnReadComp: %p{PIOCOMP_FN}, ")
								_T("pClientConsumerInfo->ConsumerInfo.fnWriteComp: %p{PIOCOMP_FN}, ")
								_T("pClientConsumerInfo->ConsumerInfo.fnTrace: %p{PIOTRACE_FN}, "), 
								pClientConsumerInfo->timeout,
								pClientConsumerInfo->fParallel,
								pClientConsumerInfo->ConsumerInfo.DefaultUserDataLength,
								pClientConsumerInfo->ConsumerInfo.ConsumerKey,
								pClientConsumerInfo->ConsumerInfo.fnReadComp,
								pClientConsumerInfo->ConsumerInfo.fnWriteComp,
								pClientConsumerInfo->ConsumerInfo.fnTrace);
	
	DWORD dwRet = ERROR_FAIL;
	bool fLocalDB = false;
	ConnectParameter * pConnectParams = NULL;

	WCHAR wszLocaldbConnect[CONNECT_MAX] = L"\0";
	DWORD cchLocaldbConnectBuf = CONNECT_MAX;
	
	ProtList protList;

	DWORD dwStart = GetTickCount();
	int timeleft = pClientConsumerInfo->timeout;

	LPWSTR wszCopyConnect = NULL;

	if( INVALID_PREFIX <= pClientConsumerInfo->networkLibrary )
	{
		dwRet = ERROR_INVALID_PARAMETER;
		goto ExitFunc;
	}

	pConnectParams = NewNoX(gpmo) ConnectParameter;
	if( !pConnectParams )
	{
		dwRet = ERROR_OUTOFMEMORY;
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, dwRet );
		goto ExitFunc;
	}

	WCHAR wszConnect[MAX_PROTOCOLNAME_LENGTH+1+
						MAX_ALIAS_LENGTH+1+
						MAX_PROTOCOLPARAMETER_LENGTH+1];

	// Connect to Local DB instance ?
	//
	if (ERROR_SUCCESS != pConnectParams->IsLocalDBConnectionString(pClientConsumerInfo->wszConnectionString,&fLocalDB))
	{
		goto ExitFunc;
	}
	
	if(fLocalDB)
	{
		// Obtain the singleton object
		//
		LocalDB* pLdbBInstance = NULL;
		if(ERROR_SUCCESS != (dwRet = LocalDB::LDBInstance(&pLdbBInstance)))
		{
			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, dwRet);
			goto ExitFunc;
		}

		// Create a copy of the actual string
		dwRet = CopyConnectionString(pClientConsumerInfo->wszConnectionString, &wszCopyConnect);
		if (ERROR_SUCCESS != dwRet)
		{
			// BID already traced in CopyConnectionString
			goto ExitFunc;
		}

		// Trim Leading whitespaces (This is not supposed to fail)
		//
		StrTrimLeftW_Sys(wszCopyConnect);

		// We pass on the connection string excluding "(localdb)"
		//
		dwRet = pLdbBInstance->getLocalDBConnectionString(wszCopyConnect + (sizeof(LOCALDB_INST_WSTR)/sizeof(WCHAR)-1),
														 wszLocaldbConnect,
														 &cchLocaldbConnectBuf);

		if( ERROR_SUCCESS != dwRet)
		{
			goto ExitFunc;
		}
		
		
        if (FAILED( dwRet = StringCchPrintfW(wszConnect, 
							ARRAYSIZE(wszConnect),
							L"%s%s",
							g_rgwszPrefix[pClientConsumerInfo->networkLibrary],
							wszLocaldbConnect)) )        		
		{
			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, dwRet );
			goto ExitFunc;
		}
	}
	else if (FAILED( dwRet = StringCchPrintfW(wszConnect,
							ARRAYSIZE(wszConnect),
							L"%s%s",
							g_rgwszPrefix[pClientConsumerInfo->networkLibrary],
							pClientConsumerInfo->wszConnectionString)) )
	{
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, dwRet );
		goto ExitFunc;
	}
	
	dwRet = pConnectParams->ParseConnectionString(wszConnect, !!(pClientConsumerInfo->fParallel), pClientConsumerInfo->transparentNetworkResolution, pClientConsumerInfo->totalTimeout);

	if( ERROR_SUCCESS != dwRet)
	{
		goto ExitFunc;
	}

	if( !pConnectParams->m_wszProtocolName[0] )
	{
		if(pClientConsumerInfo->isAzureSqlServerEndpoint)
		{
			GetProtocolList(&protList, (TCHAR *) pConnectParams->m_wszServerName, (TCHAR *) pConnectParams->m_wszOriginalServerName, (TCHAR *) L"TCP\0");
		}
		else
		{
			GetProtocolList(&protList, (TCHAR *) pConnectParams->m_wszServerName, (TCHAR *) pConnectParams->m_wszOriginalServerName );
		}
	}
	
	// when fOverrideCache is TRUE we should wipe out the cache entry
	// and do a connection from scratch
	if( pClientConsumerInfo->fOverrideLastConnectCache )
	{
		LastConnectCache::RemoveEntry( pConnectParams->m_wszAlias);
	}
	else
	{
		// Try connecting to server using cache value

		// Adjust the timeout.
		// Do NOT fail if timeleft is less than 0. Only TCP honors timeout value and fails accordingly.
		if( INFINITE != pClientConsumerInfo->timeout )
		{
			timeleft = pClientConsumerInfo->timeout - (DWORD)(GetTickCount() - dwStart);
			if ( INFINITE == timeleft ) timeleft -= 1;  // avoid infinite.
			BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("timeout(0): %d\n"), timeleft);
		}
		
		if( 	true == pConnectParams->m_fCanUseCache &&
				true == ConnectUsingCache( 	pConnectParams,
													&protList, 
													pClientConsumerInfo, 
													ppConn,
													timeleft))
		{
			// In case of Sandbox, we are not initializing LastConnectCache, so ConnectUsingCache
			// should never succeed when g_fSandbox is true.
			Assert( !g_fSandbox );

			dwRet = ERROR_SUCCESS;
			goto ExitFunc;
		}
	}
	
	bool fSsrpRequired;
	bool fSsrpDone = false;

	dwRet = MakeProtocolList( pConnectParams, &protList, &fSsrpRequired );

	if( ERROR_SUCCESS != dwRet )
	{
		goto ExitFunc;
	}

	//
	// no protocol specified in the connection string 
	// and enabled protocols list is empty
	//
	
	if( protList.Head == NULL )
	{
		dwRet = ERROR_FAIL;
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_27, dwRet );
		goto ExitFunc;
	}

	//
	// SQL BU DT bug 286397: SNAC requires a successful SSRP query
	// for named instances even if no protocol was specifed 
	// (e.g. ".\instanceName"), and the connection could succeed 
	// through SM taken from the protocol list without any SSRP query.  
	// This is a breaking change compared to MDAC 2.x.  
	//
	// To minimize the risk for Yukon Beta 2, this is only a partial 
	// fix, which will skip the SSRP query only if the SM protocol is 
	// first in the list.  Later on, it can be extended to cases when
	// SM is further down the list.  
	//

	bool fSmDone = false;
	
	ProtElem * pProtElem;

	pProtElem = protList.Head;

	if( SM_PROV == pProtElem->GetProviderNum() )
	{
		//DEVNOTE: even lpc do not take timeout value in the provider level, we still adjust
		//the timeout value here in case we can add timeout value in the future.
		// Do NOT fail if timeleft is less than 0. Only TCP honors timeout value and fails accordingly.
		
		if( INFINITE != pClientConsumerInfo->timeout )
		{
			timeleft = pClientConsumerInfo->timeout - (DWORD) (GetTickCount() - dwStart);
			if ( INFINITE == timeleft ) timeleft -= 1;  // avoid infinite.
			BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("timeout(1): %d\n"), timeleft );
		}
		
		dwRet = Connect( pConnectParams, 
						 pClientConsumerInfo, 
						 pProtElem, 
						 ppConn, 
						 timeleft );

		if( ERROR_SUCCESS ==  dwRet )
		{
			if( pConnectParams->m_fCanUseCache )
			{
				LastConnectCache::SetEntry(pConnectParams->m_wszAlias, pProtElem);
			}
			
			goto ExitFunc;
		}
		else
		{
			fSmDone = true;

			ProtElem * pDelete = protList.RemoveFirst(); 

			Assert( pDelete );

			delete pDelete;
		}
	}

	if( fSsrpRequired )
	{
		dwRet = SSRP::SsrpGetInfo(pConnectParams->m_wszServerName,
									pConnectParams->m_wszInstanceName, 
									&protList);
		if( dwRet != ERROR_SUCCESS )
		{
			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_26, dwRet );
			goto ExitFunc;
		}

		fSsrpDone = true;

		//
		// none of the enabled protocols are available on the server
		//
		
		if( protList.Head == NULL )
		{
			dwRet = ERROR_FAIL;

			//only set new last error when we did not try SM before. Otherwise, keep the error msg from SM connect
			if( !fSmDone )
			{
				SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_28, dwRet );
			}
			goto ExitFunc;
		}
	}


	pProtElem = protList.Head;
	
	while( pProtElem != 0 )
	{
		// Adjust the timeout to exclude the time spent waiting for SSRP or other failed Connect.
		// Do NOT fail if timeleft is less than 0. Only TCP honors timeout value and fails accordingly.
		if(INFINITE != pClientConsumerInfo->timeout )
		{
			timeleft = pClientConsumerInfo->timeout - (DWORD)(GetTickCount() - dwStart);
			if ( INFINITE == timeleft ) timeleft -= 1;  // avoid infinite.
			BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("timeout(2): %d\n"), timeleft);
		}
		
		if (pProtElem->GetProviderNum() == TCP_PROV)
		{
			pProtElem->Tcp.transparentNetworkIPResolution = pConnectParams->m_TransparentNetworkResolution;
			pProtElem->Tcp.totalTimeout = pConnectParams->m_TotalTimeout;
		}

		dwRet = Connect( pConnectParams, 
						 pClientConsumerInfo, 
						 pProtElem, 
						 ppConn, 
						 timeleft);

		if( ERROR_SUCCESS ==  dwRet )
		{
			if( pConnectParams->m_fCanUseCache )
			{
				LastConnectCache::SetEntry(pConnectParams->m_wszAlias, pProtElem);
			}
			
			break;
		}
		
		if( !g_fSandbox )
		{
			// Special case to check if we are connecting to a DEFAULT server for DAC
			// In that case, we would have tried to connect to port 1434 and if that has
			// failed, we now want to get the port through SSRP
			if( !wcscmp(L"admin", pConnectParams->m_wszProtocolName) &&
				!pConnectParams->m_wszInstanceName[0] &&
				(1434 == _wtoi( pProtElem->Tcp.wszPort ))  )
				
			{
				USHORT port;
				
				if( !SSRP::GetAdminPort( pConnectParams->m_wszServerName, 
										L"MSSQLServer", 
										&port) )
				{
					SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_43, ERROR_FAIL );
				}

				// Make sure the Browser did not return 1434 again.
				else if(1434 == port)
				{
					// deleted to keep user informed the dwRet from previous tries. 
					// dwRet = ERROR_INVALID_PARAMETER;
					SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_42, dwRet );
				}

				else
				{
					(void)StringCchPrintfW( pProtElem->Tcp.wszPort, ARRAYSIZE(pProtElem->Tcp.wszPort), L"%d", port);
					continue;
				}
			}
			
			// Special case to check if we are connecting to a Via cluster
			// In this case, we do SSRP even for the default instance - only if it hasn't been done earlier
			else if( (pProtElem->GetProviderNum() == VIA_PROV) && !fSsrpDone )
			{
				DWORD nlRet;

				if ( pConnectParams->m_wszInstanceName[0] == L'\0' ) 
				{
					nlRet = SSRP::SsrpGetInfo(pConnectParams->m_wszServerName, 
												L"MSSQLSERVER", 
												&protList);
				}
				else
				{
					nlRet = SSRP::SsrpGetInfo(pConnectParams->m_wszServerName, 
												pConnectParams->m_wszInstanceName, 
												&protList);
				}
				fSsrpDone = true;

				pProtElem = protList.Head;

				while( pProtElem && pProtElem->GetProviderNum() != VIA_PROV)
					pProtElem = pProtElem->m_Next;

				if( pProtElem == NULL )
				{
					Assert( nlRet == ERROR_SUCCESS );
					
					dwRet = ERROR_FAIL;
					SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_28, dwRet );
					goto ExitFunc;
				}

				if( nlRet == ERROR_SUCCESS )
				{
					continue;
				}
			}
		} // if ( !g_fSandbox )

		pProtElem = pProtElem->m_Next;
	}

	Assert( (pProtElem && dwRet == ERROR_SUCCESS) ||
			(!pProtElem && dwRet != ERROR_SUCCESS) );

ExitFunc:

	if(wszCopyConnect)
	{
		delete [] wszCopyConnect;
		wszCopyConnect = NULL;
	}

	if( ERROR_SUCCESS == dwRet )
	{
		if (pConnectParams->m_fStandardInstName)
		{
			int cchInstanceName = wcslen(pConnectParams->m_wszInstanceName);
			if (cchInstanceName == 0)
			{
				pClientConsumerInfo->szInstanceName = "";
			}
			else
			{
				int ret = WideCharToMultiByte(CP_ACP, 0, pConnectParams->m_wszInstanceName, cchInstanceName, NULL, 0, NULL, NULL);
				if (ret >= MAX_NAME_SIZE - 1 || ret <= 0)
				{

					dwRet = GetLastError();
					return dwRet;
				}
	
				ret = WideCharToMultiByte(CP_ACP, 0, pConnectParams->m_wszInstanceName, cchInstanceName, pClientConsumerInfo->szInstanceName, cchInstanceName, NULL, NULL);
				if (ret >= MAX_NAME_SIZE - 1 || ret <= 0)
				{

					dwRet = GetLastError();
					return dwRet;
				}
	
				pClientConsumerInfo->szInstanceName[ret] = '\0';
			}
			
		}
		else
		{
			pClientConsumerInfo->szInstanceName = "";
		}

		BidUpdateItemIDA( (*ppConn)->GetBidIdPtr(), SNI_ID_TAG 
			"connection string: '%ls'", pClientConsumerInfo->wszConnectionString );  
	}

	delete pConnectParams;
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;
}

// NOTE: The caller assumes ownership of the dynamically allocated copy
DWORD CopyConnectionString(__in LPCWSTR wszConnect, __out LPWSTR* pwszCopyConnect)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T( "wszConnect: '%ls', wszCopyConnect: %p {LPWSTR*}\n"), wszConnect, pwszCopyConnect);
	Assert(wszConnect && wszConnect[0]);
	Assert(pwszCopyConnect);
	DWORD dwRet = ERROR_SUCCESS;
	*pwszCopyConnect = NULL;
			
	DWORD len = (DWORD) wcslenInWChars(wszConnect);

	if(NULL == ((*pwszCopyConnect) = NewNoX(gpmo) WCHAR[len + 1]))
	{
		dwRet = ERROR_OUTOFMEMORY;
		BidTrace1(ERROR_TAG _T("LocalDB: failed to allocate connection string copy. %d{WINERR}\n"), dwRet);
		goto Exit;
	}

	if (FAILED (dwRet = StringCchCopyW((*pwszCopyConnect), len+1, wszConnect)))
	{
		BidTrace1(ERROR_TAG _T("LocalDB: copying connection string failed. %d{WINERR}\n"), dwRet);
		dwRet = ERROR_INVALID_PARAMETER;
		goto Exit;
	}

Exit:

	if(ERROR_SUCCESS != dwRet)
	{
		SNI_SET_LAST_ERROR(INVALID_PROV, SNIE_SYSTEM, dwRet);
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;
}

