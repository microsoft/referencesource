//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: SNI_ServiceBindings.cpp
// See SQLDevDash for source code ownership
//
// Purpose: SNI SPN-matching utilities for Service Bindings
//
// Notes:
//	
// @EndHeader@
//****************************************************************************

#include "snipch.hpp"

#include "SNI_ServiceBindings.hpp"
#include <iphlpapi.h> // for GetAdaptersAddresses
#include <wchar.h> // for wmemcpy
#include "tcp.hpp" // for Tcp::GetDnsName
#include <lmcons.h> // for NETBIOS_NAME_LEN

#ifndef SNIX
#pragma region Members

volatile LONG SNI_ServiceBindings::s_lClusterAddressesInitialized = 0;
bool SNI_ServiceBindings::s_fClusterHostNamesInitialized = false;

bool SNI_ServiceBindings::s_fWSAStarted = false;

OACR_WARNING_SUPPRESS(IPV6_ADDRESS_STRUCTURE_IPV4_SPECIFIC , "Separate path for handling IPv4-specific data - see s_pi6aIPv6Address for analogous IPv6-specific data");
struct in_addr *SNI_ServiceBindings::s_piaIPv4Address = NULL;
DWORD   SNI_ServiceBindings::s_dwcIPv4Address = 0;

struct in6_addr *SNI_ServiceBindings::s_pi6aIPv6Address = NULL;
DWORD   SNI_ServiceBindings::s_dwcIPv6Address = 0;

LPWSTR *SNI_ServiceBindings::s_pwszHostNames = NULL;
DWORD  SNI_ServiceBindings::s_dwcHostNames = 0;

LPWSTR *SNI_ServiceBindings::s_pwszSPN = NULL;
DWORD   SNI_ServiceBindings::s_dwcSPN = 0;

#pragma endregion Members
#endif // ifndef SNIX

#pragma region Public

//----------------------------------------------------------------------------
// See comments in header file
//----------------------------------------------------------------------------
DWORD SNI_ServiceBindings::MatchSPN(__in_z LPWSTR wszClientSuppliedSPN, __out SNIAuthErrStates *pSSPIFailureState)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("wszClientSuppliedSPN: %p{LPWSTR}, pSSPIFailureState: %p{SNIAuthErrStates*}\n"), 
					wszClientSuppliedSPN, pSSPIFailureState );

#ifndef SNIX

	Assert(NULL != wszClientSuppliedSPN);
	Assert(NULL != pSSPIFailureState);
	
	DWORD dwRet = ERROR_SUCCESS;
	*pSSPIFailureState = SNIAUTH_ERRST_SUCCESS;
	WCHAR *wszClientSuppliedSPNCopy = NULL;
	
	dwRet = MatchApprovedList(wszClientSuppliedSPN);
	if( ERROR_SUCCESS == dwRet )
	{
		goto Exit;
	}
	
	// if Approved List matching fails, reset to SUCCESS and try matching the pieces.
	dwRet = ERROR_SUCCESS;

	// Make a copy so we can mangle it
	DWORD cchClientSuppliedSPN = wcslenInWChars(wszClientSuppliedSPN);
	wszClientSuppliedSPNCopy = NewNoX(gpmo) WCHAR[cchClientSuppliedSPN + 1];
	if( NULL == wszClientSuppliedSPNCopy )
	{
		dwRet = ERROR_OUTOFMEMORY;
		*pSSPIFailureState = SNIAUTH_ERRST_SSPIHANDSHAKE_OOM;
		BidTrace1( ERROR_TAG _T("%d{WINERR}\n"), dwRet);
		goto Exit;
	}

	wmemcpy(wszClientSuppliedSPNCopy,wszClientSuppliedSPN, cchClientSuppliedSPN + 1);
	
	// Matching service class is easy, since it is mandatory and must say L"MSSQLSvc/"
	DWORD cchServiceClass = wcslenInWChars(SQL_SERVICECLASS_W L"/");
	
	if (0 != _wcsnicmp_l(SQL_SERVICECLASS_W L"/", wszClientSuppliedSPNCopy, cchServiceClass, GetDefaultLocale()))
	{
		*pSSPIFailureState = SNIAUTH_ERRST_SSPIHANDSHAKE_SERVICEBINDINGS_SERVICECLASSMISMATCH;
		dwRet = SEC_E_BAD_BINDINGS;
		BidTrace1( ERROR_TAG _T("%d{WINERR}\n"), dwRet);
		goto Exit;
	}
	
	// Look backwards for a ":" or a "]"
	bool fColonFound = false;
	bool fSquareBracketFound = false;
	DWORD cchCharsBeforeEnd = 0;
	
	// Look for a ':' to find the Port
	WCHAR *pwcHaystack;
	for( pwcHaystack = wszClientSuppliedSPNCopy + cchClientSuppliedSPN - 1; // start at the end of the string and move backwards
		pwcHaystack >= wszClientSuppliedSPNCopy + cchServiceClass; // break out when we pass the front of the host string
		pwcHaystack--)
	{
		if (L':' == *pwcHaystack)
		{
			fColonFound = true;
			*(pwcHaystack) = L'\0'; // Address/host ends immediately before the ':'
			break;
		}
		if(L']' == *pwcHaystack)
		{
			fSquareBracketFound = true;
			*(pwcHaystack+1) = L'\0'; // IPv6 address ends at the ']'
			break;
		}
		cchCharsBeforeEnd++;
	}
	
	if (fSquareBracketFound)
	{
		// uplevel IPv6 SBs without port field
		dwRet = MatchIPv6Address(wszClientSuppliedSPNCopy + cchServiceClass, pSSPIFailureState);
	}
	else if (fColonFound)
	{
		WCHAR *pwcCharacterBeforeColon = pwcHaystack - 1;
		if (L']' == *pwcCharacterBeforeColon)
		{
			// uplevel IPv6 SBs including port field
			dwRet = MatchIPv6Address(wszClientSuppliedSPNCopy + cchServiceClass, pSSPIFailureState);
		}
		else
		{
			// IPv4, Host, or downlevel IPv6 SBs including port field
			dwRet = MatchHostOrIPv4Address(wszClientSuppliedSPNCopy + cchServiceClass, 
	                                pSSPIFailureState);
		}
	}
	else
	{
		// IPv4, Host, or downlevel IPv6 SBs without port field - the whole remaining string is the host name
		dwRet = MatchHostOrIPv4Address(wszClientSuppliedSPNCopy + cchServiceClass, 
                                pSSPIFailureState);
	}
	
Exit:
	Assert((ERROR_SUCCESS == dwRet && SNIAUTH_ERRST_SUCCESS == *pSSPIFailureState) || (ERROR_SUCCESS != dwRet && SNIAUTH_ERRST_SUCCESS != *pSSPIFailureState));
	
	if( NULL != wszClientSuppliedSPNCopy )
	{
		delete []wszClientSuppliedSPNCopy;
		wszClientSuppliedSPNCopy = NULL;
	}
	
	BidTraceU2( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}, %d{SNIAuthErrStates}\n"), dwRet, *pSSPIFailureState);
	return dwRet;

#else	// ifndef SNIX

	Assert(0 && "SqlClient does not use this method\n");

	return ERROR_NOT_SUPPORTED;

#endif

}

//----------------------------------------------------------------------------
// See comments in header file
//----------------------------------------------------------------------------
void SNI_ServiceBindings::Release()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );

#ifndef SNIX

	if (s_fWSAStarted)
	{
		WSACleanup();
		s_fWSAStarted = false;
	}
	
	ReleaseIPs();

	ReleaseNames();
	
	if( NULL != s_pwszSPN )
	{
		for(DWORD dwIndex=0; dwIndex < s_dwcSPN; dwIndex++)
		{
			Assert(NULL != s_pwszSPN[dwIndex]);
			delete []s_pwszSPN[dwIndex];
			s_pwszSPN[dwIndex] = NULL;
		}
		delete []s_pwszSPN;
		s_pwszSPN = NULL;
	}
	s_dwcSPN = 0;
	
	BidTraceU0( SNI_BID_TRACE_ON, RETURN_TAG _T("\n"));

#else	// ifndef SNIX

	// SqlClient will call this method, but it does not do anything, as there is nothing to clean up.

#endif

}

//----------------------------------------------------------------------------
// See comments in header file
//----------------------------------------------------------------------------
DWORD SNI_ServiceBindings::SetClusterAddresses(__in ADDRINFOW *paiwClusterAddresses)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("paiwClusterAddresses: %p{ADDRINFOW*}\n"), 
					paiwClusterAddresses );

#ifndef SNIX

	Assert( NULL != paiwClusterAddresses );
	
	DWORD dwRet = ERROR_SUCCESS;
	
	LONG lClusterInitialized = InterlockedCompareExchange(& s_lClusterAddressesInitialized, 1, 0);
	
	// If initialized/initializing elsewhere, return success on this thread.
	if( 1 == lClusterInitialized )
	{
		BidTraceU0(SNI_BID_TRACE_ON, SNI_TAG _T("Cluster Host names and Addresses already initialized/initializing on another thread.\n"));
		goto Exit;
	}

	dwRet = InitializeWSA();
	if( ERROR_SUCCESS != dwRet )
	{
		goto Exit;
	}

	// If the machine addresses were already initialized elsewhere, we need to get rid of them
	// and only use the cluster addresses. ReleaseIPs will only clean up the IP address lists.
	// The SPN list is not touched.
	ReleaseIPs();

	// Set the cluster IP addresses.
	
	ADDRINFOW *pAIW;
	// SNI_ServiceBindings doesn't own these ADDRINFO allocations, so count...
	for( pAIW = paiwClusterAddresses; pAIW; pAIW = pAIW->ai_next )
	{
		switch( pAIW->ai_family )
		{
			case AF_INET:
				s_dwcIPv4Address++;
				break;
			case AF_INET6:
				s_dwcIPv6Address++;
				break;
			default:
				// We don't try to handle any other address families beyond IPv4 and IPv6 in the Host field of
				// a client-specified target.
				BidTraceU0(SNI_BID_TRACE_ON, SNI_TAG _T("Ignoring address from non-IP protocol.\n"));
				break;
		}
	}
	
	if( 0 == s_dwcIPv4Address && 0 == s_dwcIPv6Address)
	{
		dwRet = ERROR_INVALID_PARAMETER;
		BidTrace0(ERROR_TAG _T("No IPv4 or IPv6 addresses were found in the cluster AddrInfo structure. The cluster must be listening on at least one IP address.\n"));
		goto Exit;
	}
	
	// allocate
	if( s_dwcIPv4Address > 0 )
	{
		s_piaIPv4Address = NewNoX(gpmo) struct in_addr[s_dwcIPv4Address];
		if( NULL == s_piaIPv4Address )
		{
			dwRet = ERROR_OUTOFMEMORY;
			goto Exit;
		}
	}
	
	if( s_dwcIPv6Address > 0 )
	{
		s_pi6aIPv6Address = NewNoX(gpmo) struct in6_addr[s_dwcIPv6Address];
		if( NULL == s_pi6aIPv6Address )
		{
			dwRet = ERROR_OUTOFMEMORY;
			goto Exit;
		}
	}
	
	// and assign
	DWORD dwIPv4 = 0;
	DWORD dwIPv6 = 0;
	for( pAIW = paiwClusterAddresses; NULL != pAIW; pAIW = pAIW->ai_next )
	{
		switch( pAIW->ai_family )
		{
			case AF_INET:
				OACR_WARNING_SUPPRESS(IPV6_ADDRESS_STRUCTURE_IPV4_SPECIFIC , "Separate path for handling IPv4-specific data - see case AF_INET6 below for IPv6-specific path");
				memcpy(& s_piaIPv4Address[dwIPv4], & ((PSOCKADDR_IN)pAIW->ai_addr)->sin_addr, sizeof(struct in_addr) );
				BidTraceAddedIPv4Address(& s_piaIPv4Address[dwIPv4]);
				dwIPv4++;
				break;
			case AF_INET6:
				memcpy(& s_pi6aIPv6Address[dwIPv6], & ((PSOCKADDR_IN6)pAIW->ai_addr)->sin6_addr, sizeof(struct in6_addr));
				BidTraceAddedIPv6Address(& s_pi6aIPv6Address[dwIPv6]);
				dwIPv6++;
				break;
			default:
				// Ignore other families
				break;
		}
	}
	
	Assert( ERROR_SUCCESS == dwRet );
Exit:
	if( ERROR_SUCCESS != dwRet )
	{
		// If we fail at startup time, clean up everything, including the SPN Approved List.
		Release();
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, dwRet );
	}
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;

#else	// ifndef SNIX

	Assert(0 && "SqlClient does not use this method\n");

	return ERROR_NOT_SUPPORTED;

#endif
	
}

//----------------------------------------------------------------------------
// See comments in header file
//----------------------------------------------------------------------------
DWORD SNI_ServiceBindings::SetClusterNames(__in_z LPWSTR wszClusterHostName)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("wszClusterHostName: \"%s\"{LPCWSTR}\n"), 
					wszClusterHostName );

#ifndef SNIX

	Assert( NULL != wszClusterHostName );
	Assert( L'\0' != wszClusterHostName[0] );
	DWORD dwRet = ERROR_SUCCESS;
	
	s_fClusterHostNamesInitialized = true;
	
	dwRet = InitializeWSA();
	if( ERROR_SUCCESS != dwRet )
	{
		goto Exit;
	}

	// In case SetHostNamesAndAcceptedSPNs was called first, clean up the list of names (JUST names - Approved List and IPs should be left untouched, as this
	// function isn't going to set them).
	ReleaseNames();
	
	// Just 3 hostnames: NetBIOS name, DNS hostname, and FQDN - no BackConnectionHostNames for clustered instance, and "localhost"
	// is not a valid name for the clustered app
	s_pwszHostNames = NewNoX(gpmo) WCHAR*[3];
	if( NULL == s_pwszHostNames )
	{
		dwRet = ERROR_OUTOFMEMORY;
		goto Exit;
	}
	
	
	DWORD cchClusterHostName = wcslen(wszClusterHostName);
	wmemcpy(s_pwszHostNames[s_dwcHostNames], wszClusterHostName, cchClusterHostName);
	s_pwszHostNames[s_dwcHostNames][cchClusterHostName] = L'\0';	
	s_dwcHostNames++;

	// Retrieve the FQDN. 
	WCHAR wszClusterFQDN[NI_MAXHOST];
	dwRet = Tcp::GetDnsName(wszClusterHostName, wszClusterFQDN, ARRAYSIZE(wszClusterFQDN));
	if( ERROR_SUCCESS != dwRet )
	{
		goto Exit;
	}
	s_dwcHostNames++;
	
	// Construct the NETBIOS name from the DNS hostname. For a cluster network name, this is
	// done by simple truncation of the DNS hostname down to a max of 15 characters.
	DWORD cchClusterNetbiosName = wcslenInWChars(s_pwszHostNames[0]);
	cchClusterNetbiosName = (cchClusterNetbiosName < (NETBIOS_NAME_LEN - 1)) ? cchClusterNetbiosName : (NETBIOS_NAME_LEN - 1);
	
	s_pwszHostNames[s_dwcHostNames] = NewNoX(gpmo) WCHAR[cchClusterNetbiosName + 1];
	if( NULL == s_pwszHostNames[s_dwcHostNames])
	{
		dwRet = ERROR_OUTOFMEMORY;
		goto Exit;
	}
	
	wmemcpy(s_pwszHostNames[s_dwcHostNames], s_pwszHostNames[0], cchClusterNetbiosName);
	s_pwszHostNames[s_dwcHostNames][cchClusterNetbiosName] = L'\0';
	s_dwcHostNames++;
	
Exit:
	if( ERROR_SUCCESS != dwRet )
	{
		// If we fail at startup time, clean up everything, including the Approved List.
		Release();
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, dwRet );
	}
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;

#else	// ifndef SNIX

	Assert(0 && "SqlClient does not use this method\n");

	return ERROR_NOT_SUPPORTED;

#endif
}

//----------------------------------------------------------------------------
// See comments in header file
//----------------------------------------------------------------------------
DWORD SNI_ServiceBindings::SetHostNamesAndAcceptedSPNs(__in_ecount_opt(dwcAcceptedSPNs) WCHAR **pwszAcceptedSPNs, DWORD dwcAcceptedSPNs)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("pwszAllowedSPNs: %p{WCHAR**}, dwcAllowedSPNs: %d{DWORD}\n"), 
					pwszAcceptedSPNs, dwcAcceptedSPNs );

#ifndef SNIX

	Assert( (NULL != pwszAcceptedSPNs && 0 < dwcAcceptedSPNs) || (NULL == pwszAcceptedSPNs && 0 == dwcAcceptedSPNs) );
	
	DWORD dwRet = ERROR_SUCCESS;

	WCHAR *wszBackConnectionHostNames = NULL;
	DWORD dwHostnameOffset = 0;

	DWORD dwTableSize = 0;
	IP_ADAPTER_ADDRESSES *pIpAdapterAddressesList = NULL; 

	// take ownership of the Approved List array - note that this must happen before any potential failure, 
	// otherwise we will not clean up the Approved List on that failure path
	s_pwszSPN = pwszAcceptedSPNs;
	s_dwcSPN = dwcAcceptedSPNs;

	if( s_fClusterHostNamesInitialized )
	{
		BidTraceU0(SNI_BID_TRACE_ON, SNI_TAG _T("Cluster host names have already been initialized - no need to initialize machine names or IPs.\n"));
		goto Exit;
	}

	dwRet = InitializeWSA();
	if( ERROR_SUCCESS != dwRet )
	{
		goto Exit;
	}
	
	// 1. IP Addresses. Call GAA, count up addresses, allocate our own structure, and repack them.
	Assert( 0 == s_dwcIPv4Address );
	Assert( 0 == s_dwcIPv6Address );


	dwRet = GetAdaptersAddresses( AF_UNSPEC, // Both IPv4 and IPv6
										  0,			//	Flags
										  NULL, 	//	Reserved
										  NULL, 	//	Ask for size only
										  &dwTableSize ); 
	
	if ( ERROR_BUFFER_OVERFLOW == dwRet )
	{
		pIpAdapterAddressesList = 
			new IP_ADAPTER_ADDRESSES[(dwTableSize + sizeof(IP_ADAPTER_ADDRESSES) - 1) / 
					sizeof(IP_ADAPTER_ADDRESSES)]; 
	
		if ( NULL == pIpAdapterAddressesList )
		{
			dwRet = ERROR_NOT_ENOUGH_MEMORY;
			goto Exit;
		}
	
		dwRet = GetAdaptersAddresses( AF_UNSPEC, // Both IPv4 and IPv6
											  0,			//	Flags
											  NULL, 	//	Reserved
											  pIpAdapterAddressesList, 
											  &dwTableSize ); 
	}
	else if( ERROR_SUCCESS == dwRet )
	{
		dwRet = ERROR_FAIL;
		BidTrace0(ERROR_TAG _T("GetAdaptersAddresses returned ERROR_SUCCESS for an allocation of size 0.\n"));
		Assert(0 && "GetAdaptersAddresses returned ERROR_SUCCESS for an allocation of size 0.");
		goto Exit;
	}
	
	if( ERROR_SUCCESS != dwRet )
	{
		BidTrace3(ERROR_TAG _T("GetAdaptersAddresses(AF_UNSPEC, 0, NULL, %p, %d) failed: %d.\n"), pIpAdapterAddressesList, dwTableSize, dwRet);
		goto Exit; 
	}
	
	//	Count
	for ( IP_ADAPTER_ADDRESSES * pIpAdapterAddresses = pIpAdapterAddressesList; 
		NULL != pIpAdapterAddresses; 
		pIpAdapterAddresses = pIpAdapterAddresses->Next )
	{
		for ( IP_ADAPTER_UNICAST_ADDRESS * pIpUnicastAddress = pIpAdapterAddresses->FirstUnicastAddress; 
			NULL != pIpUnicastAddress; 
			pIpUnicastAddress = pIpUnicastAddress->Next )
		{
			switch ( pIpUnicastAddress->Address.lpSockaddr->sa_family )
			{
			case AF_INET:
				s_dwcIPv4Address++;
				break;
			case AF_INET6:
				s_dwcIPv6Address++; 
				break;
			default:
				// Ignoring non-IP addresses, since we won't try to match them embedded in an SPN
				BidTraceU0(SNI_BID_TRACE_ON, SNI_TAG _T("Ignoring address from non-IP protocol.\n"));
				break;
			}			
		}
	}

	// Allocate
	s_pi6aIPv6Address = NewNoX(gpmo) struct in6_addr[s_dwcIPv6Address];
	if( NULL == s_pi6aIPv6Address )
	{
		dwRet = ERROR_OUTOFMEMORY;
		goto Exit;
	}

	s_piaIPv4Address = NewNoX(gpmo) struct in_addr[s_dwcIPv4Address];
	if( NULL == s_piaIPv4Address )
	{
		dwRet = ERROR_OUTOFMEMORY;
		goto Exit;
	}

	// and re-pack.
	DWORD dwIPv4 = 0;
	DWORD dwIPv6 = 0;
	
	for ( IP_ADAPTER_ADDRESSES * pIpAdapterAddresses = pIpAdapterAddressesList; 
		NULL != pIpAdapterAddresses; 
		pIpAdapterAddresses = pIpAdapterAddresses->Next )
	{
		for ( IP_ADAPTER_UNICAST_ADDRESS * pIpUnicastAddress = pIpAdapterAddresses->FirstUnicastAddress; 
			NULL != pIpUnicastAddress; 
			pIpUnicastAddress = pIpUnicastAddress->Next )
		{
			switch ( pIpUnicastAddress->Address.lpSockaddr->sa_family )
			{
			case AF_INET:
				if (sizeof(SOCKADDR_IN) > pIpUnicastAddress->Address.iSockaddrLength)
				{
					dwRet = ERROR_INVALID_PARAMETER;
					BidTrace1(ERROR_TAG _T("GetAdaptersAddresses returned a unicast address with family AF_INET and iSockaddrLength %d.\n"), pIpUnicastAddress->Address.iSockaddrLength);
					goto Exit;
				}
				memcpy(& s_piaIPv4Address[dwIPv4], &((PSOCKADDR_IN)(pIpUnicastAddress->Address.lpSockaddr))->sin_addr, sizeof(struct in_addr));
				BidTraceAddedIPv4Address(& s_piaIPv4Address[dwIPv4]);
				dwIPv4++;
				break;
			case AF_INET6:
				if (sizeof(SOCKADDR_IN6) > pIpUnicastAddress->Address.iSockaddrLength)
				{
					dwRet = ERROR_INVALID_PARAMETER;
					BidTrace1(ERROR_TAG _T("GetAdaptersAddresses returned a unicast address with family AF_INET6 and iSockaddrLength %d.\n"), pIpUnicastAddress->Address.iSockaddrLength);
					goto Exit;
				}
				memcpy(& s_pi6aIPv6Address[dwIPv6], &((PSOCKADDR_IN6)(pIpUnicastAddress->Address.lpSockaddr))->sin6_addr, sizeof(struct in6_addr));
				BidTraceAddedIPv6Address(& s_pi6aIPv6Address[dwIPv6]);
				dwIPv6++;
				break;
			default:
				// Ignoring non-IP addresses, since we won't try to match them embedded in an SPN
				break;
			}
		}
	}

	// 2. Host names. Same strategy: count, allocate, and repack.

	// Counting. Retrieve BackConnectionHostNames. If it fails, remap 
	dwRet = GetBackConnectionHostNames(&wszBackConnectionHostNames);
	if( ERROR_SUCCESS != dwRet )
	{
		// remap any failure to ERROR_SUCCESS - do not fail SQL Server startup due to failure to load BackConnectionHostNames.
		dwRet = ERROR_SUCCESS;
	}

	// Start count at 4: "localhost", FQDN, DNS hostname, and NetBIOS name (usually, but not always, the last two are the same:
	// for example, NETBIOS is limited to 15 characters, DNS hostname has no limit)
	s_dwcHostNames = 4;
	
	// Add 1 to count for each BackConnectionHostName
	if( NULL != wszBackConnectionHostNames )
	{
		WCHAR *wszBackConnectionHostName;
		for(wszBackConnectionHostName = wszBackConnectionHostNames; 
			L'\0' != wszBackConnectionHostName[0]; 
			wszBackConnectionHostName += wcslenInWChars(wszBackConnectionHostName) + 1)
		{
			s_dwcHostNames++;
		}
	}

	// Allocate...
	s_pwszHostNames = NewNoX(gpmo) WCHAR*[s_dwcHostNames];
	// reset counter to 0 so we correctly free only the allocated ones
	s_dwcHostNames = 0;
	if( NULL == s_pwszHostNames)
	{
		dwRet = ERROR_OUTOFMEMORY;
		goto Exit;
	}

	// Re-pack the names from GetComputerNameEx
	dwRet = AllocAndSetHostname(ComputerNamePhysicalDnsFullyQualified);
	if( ERROR_SUCCESS != dwRet )
	{
		goto Exit;
	}
	s_dwcHostNames++;
	dwRet = AllocAndSetHostname(ComputerNamePhysicalNetBIOS);
	if( ERROR_SUCCESS != dwRet )
	{
		goto Exit;
	}
	s_dwcHostNames++;
	
	dwRet = AllocAndSetHostname(ComputerNamePhysicalDnsHostname);
	if( ERROR_SUCCESS != dwRet )
	{
		goto Exit;
	}
	s_dwcHostNames++;

	// Add "localhost" to the list
	static const WCHAR wszLocalhost[] = L"localhost";
	s_pwszHostNames[s_dwcHostNames] = NewNoX(gpmo) WCHAR[(sizeof(wszLocalhost) + sizeof(WCHAR) - 1) / sizeof(WCHAR)];
	if( NULL == s_pwszHostNames[s_dwcHostNames])
	{
		dwRet = ERROR_OUTOFMEMORY;
		goto Exit;
	}
	s_dwcHostNames++;
	memcpy(s_pwszHostNames[s_dwcHostNames - 1],wszLocalhost,sizeof(wszLocalhost));
	BidTraceU1(SNI_BID_TRACE_ON, SNI_TAG _T("Host name added to list: \"%ls\".\n"), wszLocalhost);
	
	// Repack the BackConnectionHostNames
	if( NULL != wszBackConnectionHostNames )
	{
		DWORD cchBackConnectionHostName;
		WCHAR *wszBackConnectionHostName;
		for(wszBackConnectionHostName = wszBackConnectionHostNames; 
			L'\0' != wszBackConnectionHostName[0]; 
			)
		{
			cchBackConnectionHostName = wcslenInWChars(wszBackConnectionHostName) + 1;
			
			s_pwszHostNames[s_dwcHostNames] = NewNoX(gpmo) WCHAR[cchBackConnectionHostName];
			if( NULL == s_pwszHostNames[s_dwcHostNames])
			{
				dwRet = ERROR_OUTOFMEMORY;
				goto Exit;
			}
			s_dwcHostNames++;
			
			wmemcpy(s_pwszHostNames[s_dwcHostNames - 1],wszBackConnectionHostName,cchBackConnectionHostName);

			BidTraceU1(SNI_BID_TRACE_ON, SNI_TAG _T("Host name added to list: \"%ls\".\n"), s_pwszHostNames[s_dwcHostNames - 1]);
			
			wszBackConnectionHostName += cchBackConnectionHostName;
		}
	}

Exit:
	// Always free up the locally-allocated buffers created for BackConnectionHostNames and GetAdaptersAddresses
	if( NULL != wszBackConnectionHostNames )
	{
		delete []wszBackConnectionHostNames;
		wszBackConnectionHostNames = NULL;
	}
	if( NULL != pIpAdapterAddressesList )
	{
		delete []pIpAdapterAddressesList;
		pIpAdapterAddressesList = NULL;
	}
	
	// on error, free all the class buffers and set do SNI_SET_LAST_ERROR as well.
	if( ERROR_SUCCESS != dwRet )
	{
		Release();
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, dwRet );
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;

#else	// ifndef SNIX

	Assert(0 && "SqlClient does not use this method\n");

	return ERROR_NOT_SUPPORTED;

#endif
}

#pragma endregion Public

#pragma region Helpers

#ifndef SNIX

//----------------------------------------------------------------------------
// NAME: SNI_ServiceBindings::AllocAndSetHostname
//  
// PURPOSE:
//		A thin wrapper around GetComputerNameEx. Allocates a WCHAR string in the 
//		s_pwszHostNames array at offset s_dwcHostNames and writes the result of 
//		GetComputerNameEx(NameType, ...) into that string. On failure, 
//		cleans up any allocations.
//
// PARAMETERS:
//		COMPUTER_NAME_FORMAT NameType
//			Type of name to be retrieved from GetComputerNameEx
//
// RETURNS:
//  		On successful retrieval, ERROR_SUCCESS.
//		On any failure, other error code.
//
// NOTES:
//----------------------------------------------------------------------------
DWORD SNI_ServiceBindings::AllocAndSetHostname(COMPUTER_NAME_FORMAT NameType)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("NameType: %d{COMPUTER_NAME_FORMAT}\n"), 
					NameType );
	DWORD cchHostName = 0;
	DWORD dwRet = ERROR_SUCCESS;
	
	// Expected to fail the first time with GLE = ERROR_MORE_DATA
	if( 0 != GetComputerNameExW( NameType, NULL, &cchHostName ))
	{
		Assert(0 && "This should indicate a Windows bug.\n");
		BidTrace0(ERROR_TAG _T("GetComputerNameEx succeeded with a NULL buffer.\n"));
		dwRet = ERROR_FAIL;
		goto ErrorExit;
	}
	else if( ERROR_MORE_DATA != (dwRet = GetLastError()))
	{
		BidTrace1(ERROR_TAG _T("GetComputerNameEx (1) failed: %d.\n"), dwRet);
		goto ErrorExit;
	}
	else if( 0 == cchHostName )
	{
		Assert(0 && "This should indicate a Windows bug.\n");
		BidTrace0(ERROR_TAG _T("GetComputerNameEx returned ERROR_MORE_DATA and requested a 0-byte buffer.\n"));
		goto ErrorExit;
	}
	else
	{
		// Success case - got ERROR_MORE_DATA and a non-zero character count.
		// Remap the expected ERROR_MORE_DATA into ERROR_SUCCESS
		dwRet = ERROR_SUCCESS;
	}
	
	s_pwszHostNames[s_dwcHostNames] = NewNoX(gpmo) WCHAR[cchHostName + 1];
	if( NULL == s_pwszHostNames[s_dwcHostNames])
	{
		dwRet = ERROR_OUTOFMEMORY;
		goto ErrorExit;
	}
	
	if (0 == GetComputerNameExW(NameType, s_pwszHostNames[s_dwcHostNames], &cchHostName) )
	{
		dwRet = GetLastError();
		BidTrace1(ERROR_TAG _T("GetComputerNameEx (2) failed: %d.\n"), dwRet);
		goto Exit;
	}
	
	s_pwszHostNames[s_dwcHostNames][cchHostName] = L'\0';
	BidTraceU1(SNI_BID_TRACE_ON, SNI_TAG _T("Host name added to list: \"%ls\".\n"), s_pwszHostNames[s_dwcHostNames]);
	
	Assert( ERROR_SUCCESS == dwRet );
	goto Exit;
	
ErrorExit:
	if( NULL != s_pwszHostNames[s_dwcHostNames] )
	{
		delete []s_pwszHostNames[s_dwcHostNames];
		s_pwszHostNames[s_dwcHostNames] = NULL;
	}
Exit:
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;
}

//----------------------------------------------------------------------------
// NAME: SNI_ServiceBindings::BidTraceAddedIPv4Address
//  
// PURPOSE:
//		Helper to BID trace an IPv4 address structure.
//
// NOTES:
//----------------------------------------------------------------------------
inline void SNI_ServiceBindings::BidTraceAddedIPv4Address(__in struct in_addr *psin_addr)
{
	Assert( NULL != psin_addr );
	
	BidTraceU4(SNI_BID_TRACE_ON, SNI_TAG _T("IPv4 address added to list: %u.%u.%u.%u.\n"), 
		(psin_addr->s_addr) & 0xFF, 
		(psin_addr->s_addr >> 8) & 0xFF, 
		(psin_addr->s_addr >> 16) & 0xFF, 
		(psin_addr->s_addr >> 24) & 0xFF);
}

//----------------------------------------------------------------------------
// NAME: SNI_ServiceBindings::BidTraceAddedIPv6Address
//  
// PURPOSE:
//		Helper to BID trace an IPv6 address structure.
//
// NOTES:
//----------------------------------------------------------------------------
inline void SNI_ServiceBindings::BidTraceAddedIPv6Address(__in struct in6_addr *psin6_addr)
{
	Assert( NULL != psin6_addr );
	
	BidTraceU8(SNI_BID_TRACE_ON, SNI_TAG _T("IPv6 address added to list: %x:%x:%x:%x:%x:%x:%x:%x.\n"), 
		ntohs(psin6_addr->s6_words[0]), 
		ntohs(psin6_addr->s6_words[1]), 
		ntohs(psin6_addr->s6_words[2]), 
		ntohs(psin6_addr->s6_words[3]), 
		ntohs(psin6_addr->s6_words[4]), 
		ntohs(psin6_addr->s6_words[5]), 
		ntohs(psin6_addr->s6_words[6]), 
		ntohs(psin6_addr->s6_words[7]));
}

//----------------------------------------------------------------------------
// NAME: SNI_ServiceBindings::InitializeWSA
//  
// PURPOSE:
//		Helper to initialize WSA - needed for retrieving FQDN and converting strings 
//		to IP addresses. If it has already been called, as indicated by s_fWSAStarted,
//		is a no-op. Sets s_fWSAStarted to true on success.
//
// PARAMETERS:
//
// RETURNS:
//  		On successful initialization, ERROR_SUCCESS.
//		On failure to initialize, result from WSAStartup cast to a DWORD (not an actual
//		WSA error code, which cannot be retrieved since we can't call WSAGetLastError...).
//
// NOTES:
//		Not thread safe! Current usage within SNI_ServiceBindings is entirely serial, and that is
//		guaranteed by the API caller contract described in the public API's descriptions.
//----------------------------------------------------------------------------
DWORD SNI_ServiceBindings::InitializeWSA()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n"));
	
	DWORD dwRet = ERROR_SUCCESS;
	if( s_fWSAStarted )
	{
		goto Exit;
	}

	WSADATA 	  wsaData;
	if ( WSAStartup((WORD)0x0202, &wsaData) )
	{
		int iReturn = WSAStartup((WORD)0x0101, &wsaData); 
		if ( iReturn )
		{
			dwRet = static_cast<DWORD>(iReturn); // Can't call WSAGetLastError to get the real error...
			BidTrace1( ERROR_TAG _T("Failed to initialize WSA: %d\n"), dwRet);
		}
	}
	s_fWSAStarted = true;
	
Exit:
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;
}

//----------------------------------------------------------------------------
// NAME: SNI_ServiceBindings::GetBackConnectionHostNames
//  
// PURPOSE:
//		Allocates space for and retrieves the LSA's BackConnectionHostNames registry value.
//		Validates that the registry value is of the correct type, and that it is correctly formatted 
//		for that type.
//
// PARAMETERS:
//		WCHAR **pwszBackConnectionHostNames
//			Pointer to hold the REG_MULTI_SZ value.
//
// RETURNS:
//  		On successful retrieval and validation, ERROR_SUCCESS.
//		On any failure, other error code.
//
// NOTES:
//----------------------------------------------------------------------------
DWORD SNI_ServiceBindings::GetBackConnectionHostNames(__deref_out __nullnullterminated WCHAR **pwszBackConnectionHostNames)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("pwszBackConnectionHostNames: %p{WCHAR**}\n"), 
					pwszBackConnectionHostNames );
	Assert( NULL != pwszBackConnectionHostNames );
	
	HKEY hRegKey;

	*pwszBackConnectionHostNames = NULL;

	WCHAR *wszBackConnectionHostNames = NULL;
	DWORD cbBackConnectionHostNames = 0;
	
    DWORD dwRet = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
							L"SYSTEM\\CurrentControlSet\\Control\\Lsa\\MSV1_0",
							0, // reserved
							KEY_QUERY_VALUE,
							&hRegKey );

	if( ERROR_SUCCESS != dwRet )
	{
		BidTrace1(ERROR_TAG _T("RegOpenKeyExW failed: %d. Ignoring BackConnectionHostNames.\n"), dwRet);
		goto Exit;
	}

	dwRet = RegQueryValueExW( hRegKey,
							   L"BackConnectionHostNames",
							   (LPDWORD) NULL,
							   (LPDWORD) NULL,
							   (LPBYTE) NULL,
							   &cbBackConnectionHostNames );

	if( ERROR_FILE_NOT_FOUND == dwRet )
	{
		BidTraceU0(SNI_BID_TRACE_ON, SNI_TAG _T("RegQueryValueExW failed with ERROR_FILE_NOT_FOUND - BackConnectionHostNames is empty.\n"));
		goto Exit;
	}

	if( ERROR_SUCCESS != dwRet )
	{
		BidTrace1(ERROR_TAG _T("RegQueryValueExW failed: %d. Ignoring BackConnectionHostNames.\n"), dwRet);
		goto Exit;
	}

	if( 0 == cbBackConnectionHostNames )
	{
		dwRet = ERROR_INVALID_PARAMETER;
		BidTrace0(ERROR_TAG _T("RegQueryValueExW succeeded but claimed it needed a 0-byte buffer. Ignoring BackConnectionHostNames.\n"));
		goto Exit;
	}

	wszBackConnectionHostNames = NewNoX(gpmo) WCHAR[((cbBackConnectionHostNames + sizeof(WCHAR) - 1) / sizeof(WCHAR))];
	if( NULL == wszBackConnectionHostNames )
	{
		dwRet = ERROR_OUTOFMEMORY;
		BidTrace0(ERROR_TAG _T("OOM while allocating buffer to hold BackConnectionHostNames. Ignoring BackConnectionHostNames.\n"));
		goto Exit;
	}

	DWORD dwType;

	dwRet = RegQueryValueExW( hRegKey,
							   L"BackConnectionHostNames",
							   (LPDWORD) NULL,
							   &dwType,
							   (LPBYTE) wszBackConnectionHostNames,
							   &cbBackConnectionHostNames );

	if( ERROR_SUCCESS != dwRet )
	{
		BidTrace1(ERROR_TAG _T("RegQueryValueExW failed: %d. Ignoring BackConnectionHostNames.\n"), dwRet);
		goto Exit;
	}

	if( dwType != REG_MULTI_SZ )
	{
		dwRet = ERROR_INVALID_PARAMETER;
		BidTrace0(ERROR_TAG _T("RegQueryValueExW succeeded but returned a different type from the expected REG_MULTI_SZ. Ignoring BackConnectionHostNames.\n"));
		goto Exit;
	}

	// allow the special case of 0-length array REG_MULTI_SZ, which only has one terminator
	// if not in the special case, verify null-null-termination
	if( sizeof(WCHAR) == cbBackConnectionHostNames && L'\0' == wszBackConnectionHostNames[0])
	{
		// ok - fall through to success case
	}
	else if( 2 * sizeof(WCHAR) > cbBackConnectionHostNames ||
		L'\0' != wszBackConnectionHostNames[(cbBackConnectionHostNames / sizeof(WCHAR)) - 1] || 
		L'\0' != wszBackConnectionHostNames[(cbBackConnectionHostNames / sizeof(WCHAR)) - 2] )
	{
		dwRet = ERROR_INVALID_PARAMETER;
		BidTrace0(ERROR_TAG _T("BackConnectionHostNames REG_MULTI_SZ is corrupted. Ignoring BackConnectionHostNames.\n"));
		goto Exit;
	}

	*pwszBackConnectionHostNames = wszBackConnectionHostNames;

Exit:
	if( ERROR_SUCCESS != dwRet )
	{
		if( NULL != wszBackConnectionHostNames )
		{
			delete []wszBackConnectionHostNames;
			wszBackConnectionHostNames = NULL;
		}
	}
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;
}

//----------------------------------------------------------------------------
// NAME: SNI_ServiceBindings::MatchAgainstNameList
//  
// PURPOSE:
//		Matches the "host" part of the target against the list of hostnames. 
//
// PARAMETERS:
//		LPWSTR wszHost
//			The null-terminated "host" part of the client-specified target.
//
// RETURNS:
//  		On successful match, ERROR_SUCCESS.
//		On mismatch, SEC_E_BAD_BINDINGS.
//
// NOTES:
//		The string compare is case-insensitive. The compare uses the SQL Server Locale,
//		(or, if this component is consumed by some other process, whichever locale is
//		returned by that that consumer's implementation of GetDefaultLocale()). 
//----------------------------------------------------------------------------
DWORD SNI_ServiceBindings::MatchAgainstNameList(__in_z LPWSTR wszHost)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("wszHost: %p{LPWSTR}\n"), 
					wszHost);
	Assert( NULL != wszHost );
	
	DWORD dwRet = SEC_E_BAD_BINDINGS;
	for(DWORD dwIndex = 0; dwIndex < s_dwcHostNames; dwIndex++)
	{
		if (0 == _wcsicmp_l(s_pwszHostNames[dwIndex], wszHost, GetDefaultLocale()))
		{
			dwRet = ERROR_SUCCESS;
			break;
		}
	}

	if( ERROR_SUCCESS != dwRet )
	{
		BidTrace1( ERROR_TAG _T("%d{WINERR}\n"), dwRet);
	}
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;
}

//----------------------------------------------------------------------------
// NAME: SNI_ServiceBindings::MatchAgainstIpv4AddressList
//  
// PURPOSE:
//		Matches a unique binary form of an IPv4 address, parsed from the "host" part of the 
//		target, against the list of IPv4 addresses and against all local IPv4 Addresses.
//
// PARAMETERS:
//		LPSOCKADDR psaIpv4Address
//
// RETURNS:
//  		On successful match, ERROR_SUCCESS.
//		On mismatch, SEC_E_BAD_BINDINGS.
//
// NOTES:
//----------------------------------------------------------------------------
DWORD SNI_ServiceBindings::MatchAgainstIpv4AddressList(__in PSOCKADDR_STORAGE psasIpv4Address)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("psaIpv4Address: %p{PSOCKADDR_STORAGE}\n"), 
					psasIpv4Address);
	Assert( NULL != psasIpv4Address );
	
	DWORD dwRet = SEC_E_BAD_BINDINGS;
	for(DWORD dwIndex = 0; dwIndex < s_dwcIPv4Address; dwIndex++)
	{
		if (0 == memcmp(& s_piaIPv4Address[dwIndex], & ((PSOCKADDR_IN)(psasIpv4Address))->sin_addr, sizeof(struct in_addr)))
		{
			dwRet = ERROR_SUCCESS;
			goto Exit;
		}
	}
	
	dwRet = IsIn4AddrLoopback((PSOCKADDR_IN)(psasIpv4Address)) ? ERROR_SUCCESS : dwRet;
	
	if( ERROR_SUCCESS != dwRet )
	{
		BidTrace1( ERROR_TAG _T("%d{WINERR}\n"), dwRet);
	}

Exit:
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;
}

//----------------------------------------------------------------------------
// NAME: SNI_ServiceBindings::MatchAgainstIpv6AddressList
//  
// PURPOSE:
//		Matches a unique binary form of an IPv6 address, parsed from the "host" part of the 
//		target, against the list of IPv6 addresses and against all local IPv6 Addresses.
//
// PARAMETERS:
//		LPSOCKADDR psaIpv6Address
//
// RETURNS:
//  		On successful match, ERROR_SUCCESS.
//		On mismatch, SEC_E_BAD_BINDINGS.
//
// NOTES:
//----------------------------------------------------------------------------
DWORD SNI_ServiceBindings::MatchAgainstIpv6AddressList(__in PSOCKADDR_STORAGE psasIpv6Address)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("psaIpv6Address: %p{PSOCKADDR_STORAGE}\n"), 
					psasIpv6Address);
	Assert( NULL != psasIpv6Address );
	
	DWORD dwRet = SEC_E_BAD_BINDINGS;
	for(DWORD dwIndex = 0; dwIndex < s_dwcIPv6Address; dwIndex++)
	{
		if (0 == memcmp(& s_pi6aIPv6Address[dwIndex], & ((PSOCKADDR_IN6)(psasIpv6Address))->sin6_addr, sizeof(struct in6_addr)))
		{
			dwRet = ERROR_SUCCESS;
			goto Exit;
		}
	}
	
	dwRet = IsIn6AddrLoopback((PSOCKADDR_IN6)(psasIpv6Address)) ? ERROR_SUCCESS : dwRet;
	
	if( ERROR_SUCCESS != dwRet )
	{
		BidTrace1( ERROR_TAG _T("%d{WINERR}\n"), dwRet);
	}

Exit:
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;
}

//----------------------------------------------------------------------------
// NAME: SNI_ServiceBindings::MatchApprovedList
//  
// PURPOSE:
//		Matches the entire client-provided target against the Approved List passed into
//		SNI_ServiceBindings::SetHostNamesAndAcceptedSPNs()
//
// PARAMETERS:
//		LPWSTR wszSPN
//			The client-specified target retrieved from the negotiated authentication context.
//
// RETURNS:
//  		On successful match, ERROR_SUCCESS.
//		On mismatch, SEC_E_BAD_BINDINGS.
//
// NOTES:
//		The string compare is case-insensitive. The compare uses the SQL Server Locale,
//		(or, if this component is consumed by some other process, whichever locale is
//		returned by that that consumer's implementation of GetDefaultLocale()). 
//----------------------------------------------------------------------------
DWORD SNI_ServiceBindings::MatchApprovedList(__in_z LPWSTR wszSPN)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("wszSPN: %p{LPWSTR}\n"), 
					wszSPN );
	Assert( NULL != wszSPN );
	
	DWORD dwRet = SEC_E_BAD_BINDINGS;
	for(DWORD dwIndex = 0; dwIndex < s_dwcSPN; dwIndex++)
	{
		if (0 == _wcsicmp_l(s_pwszSPN[dwIndex], wszSPN, GetDefaultLocale()))
		{
			dwRet = ERROR_SUCCESS;
			break;
		}
	}
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;
}
//----------------------------------------------------------------------------
// NAME: SNI_ServiceBindings::MatchHostOrIPv4Address
//  
// PURPOSE:
//		Determines whether the "host" part of the client-specified target is an IPv4 address or not.
//		If it is an IPv4 address, calls a helper to validate against the IPv4 list. Otherwise, calls
//		a helper to validate against the Hostname list.
//
// PARAMETERS:
//		LPWSTR wszHost
//			The null-terminated "host" part of the client-specified target.
//		SNIAuthErrStates *pSSPIFailureState
//			Failure state.
//
// RETURNS:
//  		On successful match, ERROR_SUCCESS.
//		On mismatch, SEC_E_BAD_BINDINGS.
//
// NOTES:
//
//----------------------------------------------------------------------------
DWORD SNI_ServiceBindings::MatchHostOrIPv4Address(__in_z LPWSTR wszHost, __out SNIAuthErrStates *pSSPIFailureState)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("wszHost: %p{LPWSTR}, pSSPIFailureState: %p{SNIAuthErrStates*}\n"), 
					wszHost, pSSPIFailureState );
	Assert( NULL != wszHost );
	Assert( NULL != pSSPIFailureState );
	
	DWORD dwRet = ERROR_SUCCESS;
	
	SOCKADDR_STORAGE sasIPAddress;
	
	int len = sizeof(sasIPAddress);
	sasIPAddress.ss_family = AF_INET;
	
	// try to parse it into an IPv4 address
	if( !WSAStringToAddressW( wszHost,
						AF_INET,
						0,
						(LPSOCKADDR)&sasIPAddress,
						&len))
	{
		dwRet = MatchAgainstIpv4AddressList(&sasIPAddress);
		if (ERROR_SUCCESS != dwRet)
		{
			*pSSPIFailureState = SNIAUTH_ERRST_SSPIHANDSHAKE_SERVICEBINDINGS_IPADDRESSMISMATCH;
		}
	}
	else 
	{
		// We assume it was a host name if WSAStringToAddress failed, regardless of the Winsock error code,
		// but we trace out the error so at least it is available to us in the case of real IPv4 address that Winsock
		// failed to convert.
		DWORD dwWinsockError = WSAGetLastError();
		BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("WSAStringToAddress failure: %d{WINERR}\n"), dwWinsockError);
		dwRet = MatchAgainstNameList(wszHost);
		if (ERROR_SUCCESS != dwRet)
		{
			*pSSPIFailureState = SNIAUTH_ERRST_SSPIHANDSHAKE_SERVICEBINDINGS_HOSTNAMEMISMATCH;
		}
	}
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;
}

//----------------------------------------------------------------------------
// NAME: SNI_ServiceBindings::MatchIPv6Address
//  
// PURPOSE:
//		Calls a Winsock function to convert the string form of the "host" part of the SPN into
//		a unique binary representation of an IPv6 address. If that succeeds, calls a helper to 
//		validate the binary form of the IPv6 address against the IPv6 list.
//
// PARAMETERS:
//		LPWSTR wszHost
//			The null-terminated "host" part of the client-specified target.
//		SNIAuthErrStates *pSSPIFailureState
//			Failure state.
//
// RETURNS:
//  		On successful match, ERROR_SUCCESS.
//		On mismatch or failure to parse the IPv6 address, SEC_E_BAD_BINDINGS.
//
// NOTES:
//
//----------------------------------------------------------------------------
DWORD SNI_ServiceBindings::MatchIPv6Address(__in_z LPWSTR wszHost, __out SNIAuthErrStates *pSSPIFailureState)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("wszHost: %p{LPWSTR}, pSSPIFailureState: %p{SNIAuthErrStates*}\n"), 
					wszHost, pSSPIFailureState );
	Assert( NULL != wszHost );
	Assert( NULL != pSSPIFailureState );
	
	DWORD dwRet = ERROR_SUCCESS;

	SOCKADDR_STORAGE sasIPAddress;

	int len = sizeof(sasIPAddress);
	sasIPAddress.ss_family = AF_INET6;

	// try to parse it into an IPv6 address
	if( !WSAStringToAddressW( wszHost,
						AF_INET6,
						0,
						(LPSOCKADDR)&sasIPAddress,
						&len))
	{
		dwRet = MatchAgainstIpv6AddressList(&sasIPAddress);
		if( ERROR_SUCCESS != dwRet )
		{
			*pSSPIFailureState = SNIAUTH_ERRST_SSPIHANDSHAKE_SERVICEBINDINGS_IPADDRESSMISMATCH;
		}
	}
	else
	{
		dwRet = WSAGetLastError();
		*pSSPIFailureState = SNIAUTH_ERRST_SSPIHANDSHAKE_SERVICEBINDINGS_WSASTRINGTOADDRESSFAILEDFORIPV6;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;
}

//----------------------------------------------------------------------------
// NAME: SNI_ServiceBindings::ReleaseIPs
//  
// PURPOSE:
//		Helper that cleans up the lists of IP addresses, if they have been initialized
//
// NOTES:
//----------------------------------------------------------------------------
void SNI_ServiceBindings::ReleaseIPs()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );
	if( NULL != s_piaIPv4Address )
	{
		delete []s_piaIPv4Address;
		s_piaIPv4Address = NULL;
	}
	if( NULL != s_pi6aIPv6Address )
	{
		delete []s_pi6aIPv6Address;
		s_pi6aIPv6Address = NULL;
	}
	s_dwcIPv4Address = 0;
	s_dwcIPv6Address = 0;
	BidTraceU0( SNI_BID_TRACE_ON, RETURN_TAG _T("\n"));
}

//----------------------------------------------------------------------------
// NAME: SNI_ServiceBindings::ReleaseNames
//  
// PURPOSE:
//		Helper that cleans up the list of Hostnames, if it has been initialized
//
// NOTES:
//----------------------------------------------------------------------------
void SNI_ServiceBindings::ReleaseNames()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );
	if( NULL != s_pwszHostNames )
	{
		for(DWORD dwToDelete = 0; dwToDelete < s_dwcHostNames; dwToDelete++)
		{
			Assert( NULL != s_pwszHostNames[dwToDelete] );
			
			delete s_pwszHostNames[dwToDelete];
			s_pwszHostNames[dwToDelete] = NULL;
		}
		delete []s_pwszHostNames;
		s_pwszHostNames = NULL;
	}
	s_dwcHostNames = 0;
	BidTraceU0( SNI_BID_TRACE_ON, RETURN_TAG _T("\n"));
}

//----------------------------------------------------------------------------
// NAME: SNI_ServiceBindings::RepackSzIntoWsz
//  
// PURPOSE:
//		A thin wrapper around MultiByteToWideChar. Allocates a WCHAR string in the 
//		s_pwszHostNames array at offset s_dwcHostNames and writes the result of 
//		MultiByteToWideChar into that string. On failure, 
//		cleans up any allocations.
//
// PARAMETERS:
//		char *szMbcsString
//			MBCS string to repack
//
// RETURNS:
//  		On successful conversion, ERROR_SUCCESS.
//		On any failure, other error code.
//
// NOTES:
//----------------------------------------------------------------------------
DWORD SNI_ServiceBindings::RepackSzIntoWsz(__in_z LPCSTR szMbcsString)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("szMbcsString: %hs{LPCSTR}\n"), 
					szMbcsString );
	DWORD dwRet = ERROR_SUCCESS;
	size_t cbMbcs = strlen(szMbcsString);
	
	// add 1 to include the NULL terminator, convert to int to pass into MB2WC
	int icbMbcs = cbMbcs + 1;
	
	// check for overflow from the conversion and addition
	if (icbMbcs < 0 || (size_t) icbMbcs <= cbMbcs)
	{
		dwRet = ERROR_BUFFER_OVERFLOW;
		goto ErrorExit;
	}
	
	int cchWClusterHostName = MultiByteToWideChar(CP_ACP, 0, szMbcsString, icbMbcs, NULL, 0);
	if( 0 == cchWClusterHostName )
	{
		dwRet = GetLastError();
		BidTrace1(ERROR_TAG _T("MultiByteToWideChar on the cluster host name failed to retrieve buffer size: %d{WINERR}.\n"), dwRet);
		goto ErrorExit;
	}
	
	s_pwszHostNames[s_dwcHostNames] = NewNoX(gpmo) WCHAR[cchWClusterHostName];
	if( NULL == s_pwszHostNames[s_dwcHostNames])
	{
		dwRet = ERROR_OUTOFMEMORY;
		goto ErrorExit;
	}
	
	int iRet = MultiByteToWideChar(CP_ACP, 0, szMbcsString, icbMbcs, s_pwszHostNames[s_dwcHostNames], cchWClusterHostName);
	if( 0 == iRet )
	{
		dwRet = GetLastError();
		BidTrace1(ERROR_TAG _T("MultiByteToWideChar failed to convert: %d.\n"), dwRet);
		goto ErrorExit;		
	}
	
	// MB2WC null-terminates the string, but to be extra sure...
	s_pwszHostNames[s_dwcHostNames][cchWClusterHostName - 1] = L'\0';
	BidTraceU1(SNI_BID_TRACE_ON, SNI_TAG _T("Host name added to list: \"%ls\".\n"), s_pwszHostNames[s_dwcHostNames]);

	Assert( ERROR_SUCCESS == dwRet );
	goto Exit;
ErrorExit:
	if( NULL != s_pwszHostNames[s_dwcHostNames] )
	{
		delete []s_pwszHostNames[s_dwcHostNames];
		s_pwszHostNames[s_dwcHostNames] = NULL;
	}
Exit:
	BidTraceU1(SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;
}

//----------------------------------------------------------------------------
// NAME: SNI_ServiceBindings::IsIn4AddrLoopback
//  
// PURPOSE:
//		Helper to determine whether an IPv4 address structure contains a loopback IPv4 address.
//
// RETURNS:
//  		On finding a loopback IPv4 address, true. Otherwise, false.
//
// NOTES:
//		Copied from IN4_IS_ADDR_LOOPBACK in mstcpip.h, which per LCA we cannot use directly
//		since it is not publically documented on MSDN. Should it become documented some day,
//		we should remove this function and depend directly on IN4ADDR_ISLOOPBACK
//----------------------------------------------------------------------------
inline bool SNI_ServiceBindings::IsIn4AddrLoopback(const PSOCKADDR_IN psaiAddress)
{
	OACR_WARNING_SUPPRESS(IPV6_ADDRESS_STRUCTURE_IPV4_SPECIFIC , "Separate path for handling IPv4-specific data - see IsIn6AddrLoopback for analogous IPv6-specific path");
	const IN_ADDR *a = &(psaiAddress->sin_addr);
    return (*((PUCHAR) a) == 0x7f); // matches 127.*.*.*
}

//----------------------------------------------------------------------------
// NAME: SNI_ServiceBindings::IsIn6AddrLoopback
//  
// PURPOSE:
//		Helper to determine whether an IPv6 address structure contains a loopback IPv6 address.
//
// RETURNS:
//  		On finding a loopback IPv6 address, true. Otherwise, false.
//
// NOTES:
//		Copied from IN6_IS_ADDR_LOOPBACK in ws2ipdef.h, which per LCA we cannot use directly
//		since it is not publically documented on MSDN. Should it become documented some day,
//		we should remove this function and depend directly on IN6ADDR_ISLOOPBACK
//----------------------------------------------------------------------------
inline bool SNI_ServiceBindings::IsIn6AddrLoopback(const PSOCKADDR_IN6 psai6Address)
{
	const IN6_ADDR *a = &(psai6Address->sin6_addr);
    return ((a->s6_words[0] == 0) &&
                     (a->s6_words[1] == 0) &&
                     (a->s6_words[2] == 0) &&
                     (a->s6_words[3] == 0) &&
                     (a->s6_words[4] == 0) &&
                     (a->s6_words[5] == 0) &&
                     (a->s6_words[6] == 0) &&
                     (a->s6_words[7] == 0x0100)); // matches ::1
}

#endif	// ifndef SNIX

#pragma endregion Helpers

