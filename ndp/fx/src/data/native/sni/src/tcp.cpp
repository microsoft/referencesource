//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: tcp.cpp
// @Owner: petergv, nantu
// @Test: milu
//
// <owner current="true" primary="true">petergv</owner>
// <owner current="true" primary="false">nantu</owner>
//
// Purpose:
//
// Notes:
//          
// @EndHeader@
//****************************************************************************

#include "snipch.hpp"
#include "tcp.hpp"

// KeepAlive stuff
#include "mstcpip.h"

// For cluster, needed to notify the Service Bindings checker of the Cluster IP addresses
#include "SNI_ServiceBindings.hpp"

#ifdef SNI_BASED_CLIENT
#include "nlregc.h"
#else
#include "nlregc.h"

extern CLUSTER_API gClusApi;
extern WCHAR 		gwszClusterName[];

#endif

DWORD const KEEPALIVETIME = 30000;
DWORD const KEEPALIVEINTERVAL = 1000;

DWORD const CLOSEIOWAIT = 1000;	//	milliseconds 

extern BOOL gfIsWin9x;

// This is the minimum wait time in milliseconds for a parallel connection attempt.
// The goal of this minimum is to increase the likelihood that we can get the error WSAECONNREFUSED/ERROR_CONNECTION_REFUSED,
// when possible, rather than a simple "Wait timeout expired" error.
// The REFUSED error happens when a TCP SYN is sent by the client, and the server sends back a TCP RST in response, indicating
// no service is listening on that port. 
// Windows clients have two built-in retries in this circumstance, and there is a 500 ms wait between receiving the RST and sending
// the next SYN. Therefore, we need to wait at least 1000 ms to account for the retry wait time. 
// Add an additional 500 milliseconds to account for processing time and response time on a high latency network 
// An RTT of around 166 ms would be needed for us to not see the error - this level of latency is certainly possible, but should be rare.
DWORD const MIN_PARALLEL_WAIT_TIME = 1500; 


bool g_fIpv6Supported;
bool g_fIpv4Supported;

// Used for incoming server-side connections only.  
//
DWORD  g_dwKeepAliveTime = KEEPALIVETIME;
DWORD  g_dwTcpAbortiveClose = 0 ;

static CONNECTEXFUNC* g_pConnectExFuncIpv4 = NULL;
static CONNECTEXFUNC* g_pConnectExFuncIpv6 = NULL;

static HMODULE g_hKernel32 = NULL;
static PFN_WIN32_SETFILECOMPLETIONNOTIFICATIONMODES g_pfnWin32SetFileCompletionNotificationModes = NULL;

BOOL Tcp::s_fAutoTuning = FALSE; 
BOOL Tcp::s_fSkipCompletionPort = FALSE;

// DevNote: The followings are copied from ws2tcpip.h and ws2ipdef.h of LH(vistasp1). 
// Remove them when integrated with LH winsdk.
//
#if !defined(SIO_IDEAL_SEND_BACKLOG_QUERY) && !defined(SIO_IDEAL_SEND_BACKLOG_CHANGE)

#define SIO_IDEAL_SEND_BACKLOG_QUERY   _IOR('t', 123, ULONG)
#define SIO_IDEAL_SEND_BACKLOG_CHANGE   _IO('t', 122)

int  
idealsendbacklogquery(
    __in SOCKET s,
    __out ULONG *pISB
    )
{
    DWORD bytes;

    return WSAIoctl(s, SIO_IDEAL_SEND_BACKLOG_QUERY, 
                    NULL, 0, pISB, sizeof(*pISB), &bytes, NULL, NULL);
}

int  
idealsendbacklognotify(
    __in SOCKET s,
    __in_opt LPWSAOVERLAPPED lpOverlapped,
    __in_opt LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    )
{
    DWORD bytes;

    return WSAIoctl(s, SIO_IDEAL_SEND_BACKLOG_CHANGE, 
                    NULL, 0, NULL, 0, &bytes, 
                    lpOverlapped, lpCompletionRoutine);
}
#else

// Remvoe entire ifdef and use LH winsdk.

C_ASSERT ( 0 && "remove this ifdef to use idealsendbacklog* in LH winsdk");

#endif

#ifndef NTSTATUS
typedef LONG NTSTATUS;
#endif

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif

#if !defined(SNI_BASED_CLIENT) && !defined(STANDALONE_SNI)

////////////////////////////////////////////////////////////////////////////////
//	UpdateResNameSize
//
//	Update the buffer size for the resource name
//
//	Inputs:
//		[in/out]	LPWSTR* pwszStrBuffer:
//				for input, it's the pointer to the original buffer, for output, it's the pointer of the updated buffer. 
//		[in/out] 	DWORD* dwSize:
//				for input, it's the size of the buffer the user want, for output, it's the actual size of the new buffer. 
//	Returns:
//		ERROR_SUCCESS, if the call succeeds, 
//		else, error code
//	
//Note: be careful to release the buffer, no matter this call success or not. 
DWORD UpdateResNameSize(LPWSTR* pwszStrBuffer, DWORD* dwSize)
{

	BidxScopeAutoSNI2( SNIAPI_TAG _T( "pwszStrBuffer: %p{LPWSTR*}, dwSize: %p{DWORD*}\n"), 
					pwszStrBuffer, dwSize );

	DWORD	dwRet = ERROR_SUCCESS; 
	DWORD 	dwStepSize = MAX_NAME_SIZE +1;
	
	// don't have to be so acurate.
	DWORD	dwNewActualSize = ( *dwSize  /dwStepSize +1 ) * dwStepSize;
	dwNewActualSize = dwNewActualSize  >=  dwStepSize ? dwNewActualSize : dwStepSize;

	if( dwNewActualSize > 100*dwStepSize) 
	{
		dwRet = ERROR_INVALID_PARAMETER;
		goto Exit;		// set an impossible to prevent buffer size to be unlimited. 
	}

	LPWSTR	wszNewBuffer = NewNoX(gpmo) WCHAR [ dwNewActualSize ];
	if( NULL == wszNewBuffer  )
	{
		dwRet = ERROR_OUTOFMEMORY;
		goto Exit;
	}


	if( *pwszStrBuffer) 
		delete [] *pwszStrBuffer;		//delete the original buffer
	*pwszStrBuffer = wszNewBuffer; 
	*dwSize = dwNewActualSize; 
	
Exit:
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet ;
	

}


////////////////////////////////////////////////////////////////////////////////
//	GetResourcePropertyValue_SZ
//
//	Get the value of a property of a resource
//
//	Inputs:
//		[in]	HRESOURCE		hRes:
//				the handle to the resource of interest
//		[in]	DWORD dwControlCode
//				control code for the property
//		[in] 	LPCWSTR		wcszPropertyName:
//				Property Name
//		[out] LPWSTR *		pwszPropertyValue:
//				The value of the Property will be returned.
//	Returns:
//		ERROR_SUCCESS, if the call succeeds, 
//		else, error code
//
// Note: if returns successfully, don't forget to free memeory of *pwszPropertyValue when you don't need it anymore
//
DWORD GetResourcePropertyValue_SZ(
				HRESOURCE		hRes,
				DWORD			dwControlCode,
				LPCWSTR		wcszPropertyName,
				LPWSTR *		pwszPropertyValue
    				)
{

	BidxScopeAutoSNI3( SNIAPI_TAG _T( "hRes: %p{HRESOURCE}, wcszPropertyName: '%ls', pwszPropertyValue: %p{LPWSTR*}\n"), 
					hRes, wcszPropertyName, pwszPropertyValue );


	DWORD		dwRet = ERROR_FAIL;
	LPBYTE		pPropertyList = NULL;
	DWORD		cbSizeRequired = 1;

	LPWSTR 		wszValue   = NULL;	
	LPWSTR 		wszTempValue   = NULL ;
	size_t 		cwchValueLength = 0;




	// prob the buffer size required to hold the data
	dwRet = gClusApi.ClusterResourceControl( hRes, 
							NULL, 
							dwControlCode, 
							NULL, 
							0, 
							NULL, 
							0, 
							&cbSizeRequired);

	if ( ERROR_SUCCESS != dwRet )
	{
		goto Exit;
	}

	if( 0>= cbSizeRequired )
	{
		dwRet = ERROR_FAIL;
		goto Exit;

	}

	pPropertyList = NewNoX(gpmo) BYTE [ cbSizeRequired ];
	if( NULL == pPropertyList  )
	{
		dwRet = ERROR_OUTOFMEMORY;
		goto Exit;
	}


	Assert (  	cbSizeRequired  );
	
	memset( pPropertyList, 0x00, cbSizeRequired);

	//get the data again with apporprate buffer
	dwRet = gClusApi.ClusterResourceControl( hRes, 
							NULL, 
							dwControlCode, 
							NULL, 
							0, 
							pPropertyList, 
							cbSizeRequired, 
							&cbSizeRequired );
	
	if (ERROR_SUCCESS != dwRet )
	{
		goto Exit;
	}
	
	dwRet = gClusApi.ResUtilFindSzProperty( pPropertyList,
									cbSizeRequired,
									wcszPropertyName, 
									&wszTempValue );


	if( ERROR_SUCCESS != dwRet )
	{
		goto Exit;
	}

	// found the property, copy to value	
  	// get the length of the value in characters
	if ( FAILED (StringCchLengthW( wszTempValue, MAX_NAME_SIZE, &cwchValueLength)))
	{
		dwRet = ERROR_INVALID_PARAMETER;
		goto Exit;
	}
	
	wszValue = NewNoX(gpmo) WCHAR [ cwchValueLength+1 ];	//plus one to include NULL.
	if( NULL == wszValue  )
	{
		dwRet = ERROR_OUTOFMEMORY;
		goto Exit;
	}
	memcpy(wszValue, wszTempValue, sizeof(WCHAR)*(cwchValueLength +1) );

Exit:

      *pwszPropertyValue = wszValue;

	if ( wszTempValue )
		LocalFree(wszTempValue);

	if( pPropertyList )
		delete[] pPropertyList;		

	if( ERROR_SUCCESS != dwRet )
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;

}



////////////////////////////////////////////////////////////////////////////////
//	GetVirtualServerHandle
//
//	Get the handle of the Network Name resource of interests.
//
//	Inputs:
//		[in] 	HCLUSTER	 hCluster:
//				The handle to the cluster
//		[out]  HRESOURCE	phSQLResource
//				Returns the handle of the SQL Server resource whose InstanceName property is the Instance Name the virtual server.
//	Returns:
//		ERROR_SUCCESS, if the call succeeds, 
//		else, error code
//
// Note: if returns successfully, don't forget to free the resource handle
//
DWORD GetVirtualServerHandle( HCLUSTER	 hCluster, 
							HRESOURCE* phSQLResource )
{

	BidxScopeAutoSNI2( SNIAPI_TAG _T( "hCluster: %p{HCLUSTER}, phSQLResource: %p{HRESOURCE*}\n"), 
					hCluster, phSQLResource );

	DWORD dwRet = ERROR_FAIL;

	DWORD		dwType = CLUSTER_ENUM_RESOURCE;
	HCLUSENUM 	hClusResourceEnum = NULL;
	HRESOURCE	hClusResource =	NULL;
	WCHAR*		wszNameValue = NULL;
	WCHAR*		wszResourceName=NULL;
	DWORD		cchNameBuffSize=0;
	DWORD		cchName ;

		
	hClusResourceEnum = gClusApi.ClusterOpenEnum(hCluster, dwType);

	if( NULL == hClusResourceEnum )
	{
		dwRet = GetLastError();
		goto Exit;
	}

	UpdateResNameSize( &wszResourceName, &cchNameBuffSize);

	//enumarate all resource on the cluster
	for (DWORD dwi = 0; ; dwi++)
	{
		
		cchName  = cchNameBuffSize;
		
		dwRet  = gClusApi.ClusterEnum(hClusResourceEnum, dwi, &dwType, wszResourceName, &cchName);

		if( ERROR_MORE_DATA == dwRet )
		{
			cchNameBuffSize = cchName+1;	//increase one to include NULL
			dwRet = UpdateResNameSize( &wszResourceName, &cchNameBuffSize);
			if( ERROR_SUCCESS != dwRet )
				break;
			dwi--;		//minus one to retry the current one. 
			continue; 
		}
	

		if ( ERROR_SUCCESS != dwRet )		// either an error found or ERROR_NO_MORE_ITEMS hit, both are wrong
              	break;

		// open dependent resource by name
		hClusResource   = gClusApi.OpenClusterResource( hCluster, wszResourceName );
		if ( NULL == hClusResource )
		{
			dwRet = GetLastError();
			break;
		}
			

		//compare the resource type.
		if( gClusApi.ResUtilResourceTypesEqual(  L"SQL Server", hClusResource ) )
		{	
		
			dwRet = GetResourcePropertyValue_SZ( hClusResource, 
										CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES,
										L"InstanceName", 	// get InstanceName property of SQL Server resource
										&wszNameValue ); 


			if( ERROR_SUCCESS != dwRet )
			{
				continue; 		//let try next available SQL Server resource
			}

			Assert( NULL  !=  wszNameValue );
				
			// we successfully get the Instance Name of a SQL Server resource. 
			// compare it 
			if (  !_wcsicmp_l( wszNameValue,  GetInstanceName(), GetDefaultLocale()) ) 
			{		
				// found the SQL Server resource, whose InstanceName property matchs current instance name 
				// expected exit point, keep hClusResource for return, 
				// exit, at most one match can be found

				dwRet = ERROR_SUCCESS; 
				goto Exit;		//keep the resource open and return its handle
			}


			// free the memory allocated in GetResourcePropertyValue_SZ()
			delete [] wszNameValue;
			wszNameValue = NULL;

	
		}

		// release the resource no matter whether we found a SQL Server resource or not.  
		// no match between virtual server Instance Name and SQL Server resource's InstanceName property
		gClusApi.CloseClusterResource( hClusResource );
		hClusResource = NULL;
		
            		
	}

	//unexpected end of for loop, release resource;
	if( hClusResource )
		gClusApi.CloseClusterResource( hClusResource );

	hClusResource = NULL;

Exit:

	*phSQLResource = hClusResource; 	//fill the data

	if( hClusResourceEnum )
		gClusApi.ClusterCloseEnum( hClusResourceEnum );
	
	if( wszResourceName )
		delete [] wszResourceName;

	if( wszNameValue )
		delete [] wszNameValue;
			
	if( ERROR_SUCCESS != dwRet )
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;

}






////////////////////////////////////////////////////////////////////////////////
//	GetNetworkNameHandle
//
//	Get the handle of the Network Name resource of interests.
//
//	Inputs:
//		[in]	WCHAR* wszVirtualServerName:
//				The name of the virtual server in ASCII
//		[in] 	HCLUSTER	 hCluster:
//				The handle to the cluster
//		[in]  HRESOURCE	hSQLResource
//				The handle to the SQL Server resource
//		[out] HRESOURCE* phNetResource:
//				Returns the handle of the Network Name resource whose name if the name of the virtual server.
//				The Network Name resource is a resource that hSQLResource directly depends on. 
//	Returns:
//		ERROR_SUCCESS, if the call succeeds, 
//		else, error code
//
// Note: if returns successfully, don't forget to free the resource handle
//
DWORD GetNetworkNameHandle(WCHAR* wszVirtualServerName, 
							HCLUSTER	hCluster,
							HRESOURCE	 hSQLResource, 
							HRESOURCE* phNetResource )
{

	BidxScopeAutoSNI4( SNIAPI_TAG _T( "wszVirtualServerName: %p{WCHAR*}, hCluster: %p{HCLUSTER}, hSQLResource: %p{HRESOURCE}, phNetResource: %p{HRESOURCE*}\n"), 
					wszVirtualServerName, hCluster, hSQLResource, phNetResource );

	DWORD dwRet = ERROR_FAIL;

	DWORD		dwType = CLUSTER_RESOURCE_ENUM_DEPENDS;
	HRESENUM 	hNetResEnum = NULL;
	HRESOURCE	hNetworkRes =	NULL;
	WCHAR*		wszNameValue = NULL;
	WCHAR*		wszResourceName=NULL;
	DWORD		cchNameBuffSize=0;
	DWORD		cchName ;
		
	if ( NULL == wszVirtualServerName || !*wszVirtualServerName )
	{

		dwRet = ERROR_INVALID_PARAMETER;	
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
		return dwRet;
			
    }

	// get the length of computer name  in characters, in bytes in this case
	if ( FAILED (StringCchLengthW( wszVirtualServerName, MAX_NAME_SIZE, &cbCmpNameLength)))
	{
		dwRet = ERROR_INVALID_PARAMETER;
		goto Exit;
	}

	hNetResEnum = gClusApi.ClusterResourceOpenEnum(hSQLResource, dwType);

	if( NULL == hNetResEnum )
	{
		dwRet = GetLastError();
		goto Exit;
	}

	UpdateResNameSize( &wszResourceName, &cchNameBuffSize);
	//enumarate all Network Name resource depended by the given SQL Server resource.
	for (DWORD dwi = 0; ; dwi++)
	{
		
		cchName  = cchNameBuffSize;
		
		dwRet  = gClusApi.ClusterResourceEnum(hNetResEnum, dwi, &dwType, wszResourceName, &cchName);

		if( ERROR_MORE_DATA == dwRet )
		{
			cchNameBuffSize = cchName+1;	//increase one to include NULL
			dwRet = UpdateResNameSize( &wszResourceName, &cchNameBuffSize);
			if( ERROR_SUCCESS != dwRet )
				break;
			dwi--;		//minus one to retry the current one. 
			continue; 
		}

		if ( ERROR_SUCCESS != dwRet )		// either an error found or ERROR_NO_MORE_ITEMS hit, both are wrong
              	break;

		// open dependent resource by name
		hNetworkRes   = gClusApi.OpenClusterResource( hCluster, wszResourceName );
		if ( NULL == hNetworkRes )
		{
			dwRet = GetLastError();
			break;
		}
			

		//compare the resource type.
		if( gClusApi.ResUtilResourceTypesEqual(  L"Network Name", hNetworkRes ) )
		{	
		
			dwRet = GetResourcePropertyValue_SZ( hNetworkRes, 
										CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES,
										L"Name", 	// get name property of Network Name resource
										&wszNameValue ); 


			if( ERROR_SUCCESS != dwRet )
			{
				continue; 		//lets try next available Network Name resource
			}

			Assert( NULL  !=  wszNameValue );
				
			// we successfully find the name of a Network Name resource. 
			// compare it to the name of the virtualserver
			if (  !_wcsicmp_l( wszNameValue,  wszVirtualServerName, GetDefaultLocale()) ) 
			{		
				// found the Network Name resource of the virtual server
				// expected exit point, keep hNetworkRes for return, 
				// exit, at most one match can be found

				dwRet = ERROR_SUCCESS; 
				goto Exit;		//keep the resource open and return its handle
			}


			// free the memory allocated in GetResourcePropertyValue_SZ()
			delete [] wszNameValue;
			wszNameValue = NULL;

	
		}

		// release the resource no matter whether we found a Network Name resource or not.  
		// no match between virtual server name and Network Name resource name is found, try next.
		gClusApi.CloseClusterResource( hNetworkRes );
		hNetworkRes = NULL;
		
            		
	}

	//unexpected end of for loop, release resource;
	if( hNetworkRes )
		gClusApi.CloseClusterResource( hNetworkRes );

	hNetworkRes = NULL;

Exit:

	*phNetResource = hNetworkRes; 	//fill the data
			
	if( hNetResEnum )
		gClusApi.ClusterResourceCloseEnum( hNetResEnum );

	if( wszNameValue )
		delete [] wszNameValue;

	if( wszResourceName )
		delete [] wszResourceName;
	

	if( ERROR_SUCCESS != dwRet )
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;

}






////////////////////////////////////////////////////////////////////////////////
//	GetClusterAddrInfo
//
//	Get all IP addresses depended by a Network Name resource.
//
//	Inputs:
//		[in] 	HCLUSTER	 hCluster:
//				The handle to the cluster
//		[in] HRESOURCE hResource:
//				The handle of the Network Name resource of interests
//		[out] ADDRINFOW ** ppAIW:
//				file ADDRINFOW data structure which contains all IP addresses need to listen on.
//	Returns:
//		ERROR_SUCCESS, if the call succeeds, 
//		else, error code
//
// Note: if returns successfully, don't forget to *ppAI using freeaddrinfo_l
DWORD GetClusterResAddrInfo( HCLUSTER hCluster,  HRESOURCE hResource,  ADDRINFOW ** ppAIW)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T( "hCluster: %p{HCLUSTER}, hResource: %p{HRESOURCE}, ppAIW: %p{ADDRINFOW **}\n"), 
					hCluster, hResource, ppAIW);

	DWORD 		dwRet = ERROR_FAIL;
	HRESENUM	hResEnum = NULL;
	HRESOURCE	hDependentRes = NULL;


	LPWSTR		wszAddrValue	= NULL; 
	DWORD		dwType = CLUSTER_RESOURCE_ENUM_DEPENDS;
	DWORD 		dwControlCode = CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES; 			

	WCHAR*		wszResourceName=NULL;
	DWORD		cchNameBuffSize=0;
	DWORD		cchName ;

	ADDRINFOW 	*pAddrWHead = NULL;
	ADDRINFOW 	*pAddrWTemp = NULL;
	ADDRINFOW	*pAddrWCurr	= NULL;
	ADDRINFOW  	Hints;


    	if (NULL == hResource)
	{
		dwRet = ERROR_INVALID_PARAMETER;
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );
		return dwRet;
	}

		
	// Enumerating the resources of ipaddress resources that  hNetResource depends on
	// hNetResource can be either Network Name resource or SQL Server resource, or others
	hResEnum = gClusApi.ClusterResourceOpenEnum(  hResource, dwType );

	if ( NULL == hResEnum )
	{
		dwRet =  GetLastError();
		goto Exit;
	}

	UpdateResNameSize( &wszResourceName, &cchNameBuffSize);
	
	for (DWORD dwi = 0; ; dwi++)
	{
		
		cchName  = cchNameBuffSize;
		dwRet  = gClusApi.ClusterResourceEnum( hResEnum, dwi, &dwType, wszResourceName, &cchName);

		if (ERROR_NO_MORE_ITEMS == dwRet )
		{
			//expected exit point, finish enum
			if ( NULL != pAddrWHead )			//we get at least one IP address
				dwRet = ERROR_SUCCESS;		

			goto Exit;
		}

		if( ERROR_MORE_DATA == dwRet )
		{
			cchNameBuffSize = cchName+1;	//increase one to include NULL
			dwRet = UpdateResNameSize( &wszResourceName, &cchNameBuffSize);
			if( ERROR_SUCCESS != dwRet )
				break;
			dwi--;		//minus one to retry the current one. 
			continue; 
		}
		
		if ( ERROR_SUCCESS != dwRet )		// error found
              	break;


		// open dependent resource by name
		hDependentRes   = gClusApi.OpenClusterResource( hCluster, wszResourceName );
		if ( NULL == hDependentRes)
		{
			dwRet = GetLastError();

			break;
		}
			

		//compare the resource type.
		if( gClusApi.ResUtilResourceTypesEqual(  L"IP Address", hDependentRes ) 
			|| gClusApi.ResUtilResourceTypesEqual(  L"IPv6 Address", hDependentRes ) 
			|| gClusApi.ResUtilResourceTypesEqual(  L"IPv6 Tunnel Address", hDependentRes ) )
		{	
			// found an IP address

			//Address for IPv6 Tunnel Address is read-only(RO). For IP Address and IPv6 Address, it is read-write private.
			if( gClusApi.ResUtilResourceTypesEqual(  L"IPv6 Tunnel Address", hDependentRes ) )
			{
				dwControlCode = CLUSCTL_RESOURCE_GET_RO_PRIVATE_PROPERTIES; 
			}
			else
			{
				dwControlCode = CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES;
			}
			
			dwRet = GetResourcePropertyValue_SZ( hDependentRes, 
										dwControlCode,
										L"Address", 	//get address property
										&wszAddrValue ); 


			if(  ERROR_SUCCESS != dwRet )
			{
				break;
			}

			Assert( NULL  !=  wszAddrValue );

			ZeroMemory( &Hints, sizeof(Hints) );
			Hints.ai_family = PF_UNSPEC;
			Hints.ai_socktype = SOCK_STREAM;
			Hints.ai_flags = AI_PASSIVE;

			// get the addrinfo by ip addr, *wszAddrValue is a.b.c.d
			if( GetAddrInfoW_l( wszAddrValue, L"0", &Hints, &pAddrWCurr, GetDefaultLocale() ) )
			{
				dwRet = WSAGetLastError();

				break;
			}

			Assert( pAddrWCurr );

			if( NULL == pAddrWHead )
			{
				pAddrWHead =		pAddrWCurr;
			}
			else
			{
				pAddrWTemp = pAddrWCurr;
				while( pAddrWTemp->ai_next )
					pAddrWTemp=pAddrWTemp->ai_next;
				
				pAddrWTemp->ai_next = pAddrWHead;
				pAddrWHead = pAddrWCurr;
			}
			
			// free the memory allocated in GetResourcePropertyValue_SZ()
			delete []  wszAddrValue ;
			wszAddrValue = NULL;

	
		}

		// release the resource no matter whether we found an IP address or not. 
		gClusApi.CloseClusterResource( hDependentRes );
		hDependentRes = NULL;
		
            		
	}

	//unexpected end of for loop:  by "break"

	if ( pAddrWHead )
	{

		FreeAddrInfoW( pAddrWHead );
		pAddrWHead = NULL;

	}
	

Exit:

	if ( hResEnum)
		gClusApi.ClusterResourceCloseEnum( hResEnum );

	if ( hDependentRes )
		gClusApi.CloseClusterResource( hDependentRes );

	if( wszAddrValue ) 
		delete [] wszAddrValue; 

	if( wszResourceName )
		delete [] wszResourceName;
	
	*ppAIW = pAddrWHead;

	if( ERROR_SUCCESS != dwRet )
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);


	return dwRet;


}

DWORD GetClusterListenIpAddrInfo(WCHAR* wszVirtualServerName, ADDRINFOW ** ppAIW)
{

	BidxScopeAutoSNI2( SNIAPI_TAG _T( "wszVirtualServerName: %p{WCHAR*}, ppAIW: %p{ADDRINFOW**}\n"), 
					wszVirtualServerName, ppAIW );


		// a cluster, query it using cluster API
		HRESOURCE		hNetResource = NULL;
		HRESOURCE		hSQLResource = NULL;
		HCLUSTER 		hCluster	= NULL;
		DWORD			dwRet;


		// get cluster handle to the cluster service running on the local machine.
		if ( !(hCluster = gClusApi.OpenCluster( NULL ) ) )
		{
			dwRet = GetLastError();
			SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );

			goto Exit;
		
		}

		// get the SQL Server resource handle of the virtual server. 
		if ( ERROR_SUCCESS != ( dwRet = GetVirtualServerHandle( hCluster, &hSQLResource ) ) )
		{

			SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );
			gClusApi.CloseCluster(hCluster);

			goto Exit;

		}

		Assert ( hSQLResource );


		// get the Network Name resource handle of the virtual server. 
		if ( ERROR_SUCCESS != ( dwRet = GetNetworkNameHandle( wszVirtualServerName, hCluster, hSQLResource, &hNetResource ) ) )
		{

			SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );
			gClusApi.CloseClusterResource( hSQLResource );
			gClusApi.CloseCluster(hCluster);

			goto Exit;

		}

		Assert ( hNetResource );


		// get the dependent IP addresses of a Network Name resource
		if( ERROR_SUCCESS != (dwRet = GetClusterResAddrInfo( hCluster, hNetResource, ppAIW) )  )
		{
			SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );
			gClusApi.CloseClusterResource( hNetResource );
			gClusApi.CloseClusterResource( hSQLResource );
			gClusApi.CloseCluster( hCluster );

			goto Exit;

		}

		gClusApi.CloseClusterResource( hNetResource );
		gClusApi.CloseClusterResource( hSQLResource );
		gClusApi.CloseCluster( hCluster );
		hCluster = NULL;
		hSQLResource = NULL;
		hNetResource = NULL;

Exit:

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;

}	


DWORD GetLoopbackListenIpAddrInfo( ADDRINFOW ** ppAIW)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("ppAIW: %p{ADDRINFOW**}\n"), ppAIW );

	DWORD dwRet = ERROR_SUCCESS;
	ADDRINFOW * pAddrWIpv4 = NULL;
	ADDRINFOW * pAddrWIpv6 = NULL;
OACR_WARNING_SUPPRESS(IPV6_ADDRESS_STRUCTURE_IPV4_SPECIFIC , " TEMPORARY SUPPRESSION UNTIL DEVELOPER'S ANALYSIS. See VSTS 376830 ")
	SOCKADDR_IN * pSockAddrIpv4 = NULL;
	SOCKADDR_IN6 * pSockAddrIpv6 = NULL;

	*ppAIW = NULL; 

	// IPv4
	//
	if( g_fIpv4Supported )
	{
		if( NULL == ( pAddrWIpv4 = NewNoX(gpmo) ADDRINFOW ) )
		{
			dwRet = ERROR_OUTOFMEMORY;
			SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet ); 
			goto Exit;
		}

		memset( pAddrWIpv4, 0, sizeof( *pAddrWIpv4 ) ); 

		if( NULL == ( pSockAddrIpv4 = NewNoX(gpmo) SOCKADDR_IN ) )
		{
			dwRet = ERROR_OUTOFMEMORY;
			SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet ); 
			goto Exit;
		}

		memset( pSockAddrIpv4, 0, sizeof( *pSockAddrIpv4 ) ); 

		// Fill in ADDRINFO for IPv4
		//
		pAddrWIpv4->ai_flags = 0;
	    pAddrWIpv4->ai_family = AF_INET;			// IPv4
	    pAddrWIpv4->ai_socktype = SOCK_STREAM;	// TCP	
	    pAddrWIpv4->ai_protocol = 0;

OACR_WARNING_PUSH
OACR_WARNING_DISABLE(IPV6_ADDRESS_STRUCTURE_IPV4_SPECIFIC , " TEMPORARY SUPPRESSION UNTIL DEVELOPER'S ANALYSIS. See VSTS 376830 ")

	    pAddrWIpv4->ai_addrlen = 
			sizeof( SOCKADDR_IN );				// Length of ai_addr

OACR_WARNING_POP

	    pAddrWIpv4->ai_canonname = NULL;
	    pAddrWIpv4->ai_addr = NULL;        		// Binary address.  Filled below.  
	    pAddrWIpv4->ai_next = NULL; 				// Next structure in linked list

		// Fill in SOCKADDR_IN
		//
		pSockAddrIpv4->sin_family = AF_INET;
		pSockAddrIpv4->sin_port = 0;			//	will be filled in later
		pSockAddrIpv4->sin_addr.S_un.S_addr = 
			htonl( INADDR_LOOPBACK );

		pAddrWIpv4->ai_addr = reinterpret_cast<sockaddr *>(pSockAddrIpv4); 
	}	// if( g_fIpv4Supported )
		

	// IPv6
	//
	if( g_fIpv6Supported )
	{
		if( NULL == ( pAddrWIpv6 = NewNoX(gpmo) ADDRINFOW ) )
		{
			dwRet = ERROR_OUTOFMEMORY;
			SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet ); 
			goto Exit;
		}

		memset( pAddrWIpv6, 0, sizeof( *pAddrWIpv6 ) ); 

		if( NULL == ( pSockAddrIpv6 = NewNoX(gpmo) SOCKADDR_IN6 ) )
		{
			dwRet = ERROR_OUTOFMEMORY;
			SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet ); 
			goto Exit;
		}

		memset( pSockAddrIpv6, 0, sizeof( *pSockAddrIpv6 ) ); 

		// Fill in ADDRINFO for IPv6
		//
		pAddrWIpv6->ai_flags = 0;
	    pAddrWIpv6->ai_family = AF_INET6;		// IPv6	
	    pAddrWIpv6->ai_socktype = SOCK_STREAM;	// TCP	
	    pAddrWIpv6->ai_protocol = 0;
	    pAddrWIpv6->ai_addrlen = 
			sizeof( SOCKADDR_IN6 );				// Length of ai_addr
	    pAddrWIpv6->ai_canonname = NULL;
	    pAddrWIpv6->ai_addr = NULL;        		// Binary address.  Filled below.  
	    pAddrWIpv6->ai_next = NULL; 				// Next structure in linked list

		// Fill in SOCKADDR_IN6
		//
		u_char rgbIn6_addr[] = IN6ADDR_LOOPBACK_INIT; 
		
		pSockAddrIpv6->sin6_family = AF_INET6;
		pSockAddrIpv6->sin6_port = 0;			//	will be filled in later
		pSockAddrIpv6->sin6_flowinfo = 0;
		C_ASSERT( sizeof( pSockAddrIpv6->sin6_addr.u.Byte ) == 
			sizeof( rgbIn6_addr ) ); 
		memcpy( pSockAddrIpv6->sin6_addr.u.Byte, 
			rgbIn6_addr, 
			sizeof( pSockAddrIpv6->sin6_addr.u.Byte ) ); 
		pSockAddrIpv6->sin6_scope_id = 0;

		pAddrWIpv6->ai_addr = reinterpret_cast<sockaddr *>(pSockAddrIpv6); 
	}	// if( g_fIpv6Supported )


	// Pass the structures to the caller.  
	//
	if( NULL != pAddrWIpv6 )
	{
		pAddrWIpv6->ai_next = pAddrWIpv4; 
		*ppAIW = pAddrWIpv6;
	}
	else if( NULL != pAddrWIpv4 )
	{
		*ppAIW = pAddrWIpv4;
	}
	else
	{
		dwRet = ERROR_INVALID_STATE;
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet ); 
		goto Exit;		
	}


Exit:

	if( ERROR_SUCCESS != dwRet )
	{
		// Free any allocated structures.  
		//
		if( NULL != pAddrWIpv4 )
		{
			delete pAddrWIpv4; 
		}

		if( NULL != pAddrWIpv6 )
		{
			delete pAddrWIpv6; 
		}

		if( NULL != pSockAddrIpv4 )
		{
			delete pSockAddrIpv4; 
		}

		if( NULL != pSockAddrIpv6 )
		{
			delete pSockAddrIpv6; 
		}
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;
}

#endif

void FreeCustomAddrInfo( __in_opt ADDRINFOW * pAddrInfoWHead )
{
	while( NULL != pAddrInfoWHead )
	{
		ADDRINFOW * pAddrInfoW = pAddrInfoWHead; 

		pAddrInfoWHead = pAddrInfoW->ai_next; 

		Assert( NULL == pAddrInfoW->ai_canonname ); 
		delete pAddrInfoW->ai_addr; 
		delete pAddrInfoW; 
	}
}


class ListenObject;

class AcceptObject
{
private:
	
	LPOVERLAPPED 	m_pOverlapped;
	SOCKET 			m_ListenSocket;
	int 				m_AddressFamily;
	SOCKET 			m_AcceptSocket;
	SNICritSec * 		m_CSTerminateListener;
	bool				m_fTerminateListenerCalled;

	LONG			m_cRefCount;
	ListenObject *		m_pParentListenObject;		
	
	AcceptObject(ListenObject* pListenObject);
	~AcceptObject();

public:

	friend DWORD Tcp::AcceptDone(SNI_Conn * pConn, LPVOID pAcceptKey, DWORD dwBytes, DWORD dwError, SNI_Provider * * ppProv, LPVOID * ppAcceptInfo);
	
	char 			m_AddressBuffer[sizeof(SOCKADDR_STORAGE)*2+32];
	bool			m_fPendingAccept;
	
	static DWORD InitObject( __out AcceptObject **ppAccept,SOCKET ListenSocket, int AddressFamily, ListenObject * pParentListenObject);

	LONG AddRef()
	{
		LONG cRef = InterlockedIncrement(&m_cRefCount);

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("Refcount: %d\n"), cRef);

		return cRef;
	}
	
	LONG Release()
	{
		LONG cRef = InterlockedDecrement(&m_cRefCount);

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("Refcount: %d\n"), cRef);

		if(0==cRef)
		{
			delete this;
		}		
		return cRef;
	}

	inline void UpdateSockContext()
	{
		BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );
		
		//Best effort and log but ignore error.
		//
		if(SOCKET_ERROR == setsockopt(m_AcceptSocket, 
					SOL_SOCKET, 
					SO_UPDATE_ACCEPT_CONTEXT, 
					(char*)&m_ListenSocket, 
					sizeof(m_ListenSocket)))
		{

			DWORD dwError = WSAGetLastError();
			BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("%d{WINERR}\n"),dwError);		 

		}
	}
	
	inline SOCKET GiveAcceptSocket()
	{
		SOCKET sock = m_AcceptSocket;

		m_AcceptSocket = INVALID_SOCKET;

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("SOCKET: %Iu{SOCKET}\n"), sock);
	
		return sock;
	}

	DWORD RegisterAcceptObject( HANDLE hSNIListener )
	{
		BidxScopeAutoSNI1( SNIAPI_TAG _T("hSNIListener: %p{HANDLE}\n"), hSNIListener );
		
		DWORD dwRet;
		
		// Associate listen socket with completion port
		
		dwRet = SNIRegisterForAccept( (HANDLE) m_ListenSocket );

		if( ERROR_SUCCESS != dwRet )
		{
			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
			
			return dwRet;
		}
		
		dwRet = SNICreateAccept( hSNIListener, TCP_PROV, (LPVOID) this, &m_pOverlapped );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

		return dwRet;
	}

	//
	// this function doesn't call SNISetLastError()
	// the caller will make the decision whether to call it or not
	//
	
	DWORD AsyncAccept();

	void TerminateListener()
	{
		BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );
		
		CAutoSNICritSec a_csTerminateListener( m_CSTerminateListener, SNI_AUTOCS_DO_NOT_ENTER );

		a_csTerminateListener.Enter();

		Assert( m_ListenSocket != INVALID_SOCKET );
		
		closesocket( m_ListenSocket );
		m_ListenSocket = INVALID_SOCKET;

		m_fTerminateListenerCalled = true;
		
		a_csTerminateListener.Leave(); 
	}
};

//keep listen sockets in a dynamic array
class ListenObject{

private:
	~ListenObject()
	{
		Assert( m_cRefCount == 0);
	}

public:
	
	AcceptObject	** 	m_ppAcceptObjects;
	UINT 				m_nAcceptObjects;
	UINT				m_nSize;
	TcpListenInfo 		m_TcpListenInfo;

	LONG			m_cRefCount;

	ListenObject()
	{
		m_ppAcceptObjects = 0;
		m_nAcceptObjects = 0;
		m_nSize = 0;
		m_cRefCount = 1;
	}


	DWORD Add( SOCKET sock, int af)
	{
		BidxScopeAutoSNI2( SNIAPI_TAG _T("sock: %Iu{SOCKET}, af: %d\n"), sock, af );
		
		//check if array size is enough to add a new socket otherwise reallocate
		if( m_nAcceptObjects == m_nSize )
		{
			UINT nNewSize = 0;
			
			if( m_nSize == 0 )
				nNewSize = 8;
			else if( m_nSize < USHRT_MAX)
				nNewSize = m_nSize*2;
			else 
			{
				SNI_SET_LAST_ERROR( TCP_PROV, SNIE_4, ERROR_TOO_MANY_OPEN_FILES);
				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_TOO_MANY_OPEN_FILES);
				return ERROR_TOO_MANY_OPEN_FILES;
			}
			
			AcceptObject **pNew = NewNoX(gpmo) AcceptObject *[ nNewSize ];
			if( !pNew )
			{
				SNI_SET_LAST_ERROR( TCP_PROV, SNIE_4, ERROR_OUTOFMEMORY );

				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_OUTOFMEMORY);
				
				return ERROR_OUTOFMEMORY;
			}
			
			m_nSize = nNewSize;

			memcpy( pNew, m_ppAcceptObjects, sizeof(AcceptObject *)*m_nAcceptObjects);
			delete [] m_ppAcceptObjects;
			m_ppAcceptObjects = pNew;
		}

		AcceptObject *pAcc;

		DWORD dwRet;
		
		dwRet = AcceptObject::InitObject( &pAcc, sock, af, this);

		if (ERROR_SUCCESS != dwRet )
		{
			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
			
			return dwRet;
		}
		
		m_ppAcceptObjects[m_nAcceptObjects++] = pAcc;

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
		
		return ERROR_SUCCESS;
	}

	//always removes a socket from the end and closes it
	void Remove()
	{
		BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );
		
		Assert( m_nAcceptObjects);
		m_ppAcceptObjects[--m_nAcceptObjects]->Release();
	}

	DWORD StartAccepting( HANDLE hSNIListener )
	{
		BidxScopeAutoSNI1( SNIAPI_TAG _T("hSNIListener: %p{HANDLE}\n"), hSNIListener );
		
		// Associate listen sockets with completion port
		UINT iAcceptObject;

		for( iAcceptObject=0; iAcceptObject<m_nAcceptObjects; iAcceptObject++)
		{
			DWORD dwRet;
			
			AcceptObject * pAcc = m_ppAcceptObjects[iAcceptObject];
			
			dwRet = pAcc->RegisterAcceptObject(hSNIListener);
			if( dwRet != ERROR_SUCCESS )
			{
				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
				
				return dwRet;
			}

			//this prevents a delete by AcceptDone in case an error happens before we bump up ref count
			pAcc->AddRef();
			
			dwRet = pAcc->AsyncAccept();
			if( ERROR_SUCCESS != dwRet )
			{
				pAcc->Release();

				//
				// AsyncAccept() doesn't call SNISetLastError so we have to do it here
				//
				
				SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );

				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
				
				return dwRet;
			}
		}

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
		
		return ERROR_SUCCESS;
	}

	LONG AddRef()
	{
		LONG cRef = InterlockedIncrement(&m_cRefCount);

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("Refcount: %d\n"), cRef);

		return cRef;
	}
	
	LONG Release()
	{
		LONG cRef = InterlockedDecrement(&m_cRefCount);

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("Refcount: %d\n"), cRef);

		if(0==cRef)
		{
			delete this;
		}		
		return cRef;
	}

	void TerminateListener()
	{
		BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );

		while( m_nAcceptObjects)
		{
			m_nAcceptObjects--;
			m_ppAcceptObjects[m_nAcceptObjects]->TerminateListener();
			m_ppAcceptObjects[m_nAcceptObjects]->Release();
		}		

		delete [] m_ppAcceptObjects;
		Release();

		BidTraceU0( SNI_BID_TRACE_ON, RETURN_TAG _T("\n") );
	}
	

};

AcceptObject::AcceptObject(ListenObject* pListenObject)
{
	m_pOverlapped = NULL;
	m_ListenSocket = INVALID_SOCKET;
	m_AddressFamily = 0;
	m_AcceptSocket = INVALID_SOCKET;
	m_CSTerminateListener = NULL;
	m_fTerminateListenerCalled = false;
	m_fPendingAccept = false;
	Assert(pListenObject);
	m_pParentListenObject = pListenObject;
	m_pParentListenObject->AddRef();
	m_cRefCount = 1;
}

AcceptObject::~AcceptObject()
{
	Assert( m_cRefCount == 0);

	if( m_pOverlapped )
	{
		SNIDeleteAccept( m_pOverlapped );
		m_pOverlapped = 0;
	}

	if( INVALID_SOCKET != m_ListenSocket )
	{
		closesocket( m_ListenSocket );
		m_ListenSocket = INVALID_SOCKET;
	}

	if( INVALID_SOCKET != m_AcceptSocket )
	{
		closesocket( m_AcceptSocket );
		m_AcceptSocket = INVALID_SOCKET;
	}

	if( m_CSTerminateListener!=NULL )
	{
		DeleteCriticalSection(&m_CSTerminateListener);
	}

	m_pParentListenObject->Release();
}

DWORD AcceptObject::InitObject( __out AcceptObject **ppAccept,SOCKET ListenSocket, int AddressFamily, ListenObject * pParentListenObject)	
{
		BidxScopeAutoSNI4( SNIAPI_TAG _T("ppAccept: %p{AcceptObject**}, ListenSocket: %Iu{SOCKET}, AddressFamily: %d, pParentListenObject: %p{ListenObject*}\n"), 
					ppAccept, ListenSocket, AddressFamily, pParentListenObject );
		
		AcceptObject *pAccept;

		pAccept = NewNoX(gpmo) AcceptObject(pParentListenObject);

		if( pAccept == NULL )
		{
			SNI_SET_LAST_ERROR( TCP_PROV, SNIE_4, ERROR_OUTOFMEMORY );
			
			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_OUTOFMEMORY);
			
			return ERROR_OUTOFMEMORY;
		}

		DWORD dwRet;

		dwRet = SNICritSec::Initialize( &pAccept->m_CSTerminateListener );

		if( dwRet != ERROR_SUCCESS )
		{
			pAccept->Release();

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
			
			return dwRet;
		}

		pAccept->m_ListenSocket = ListenSocket;
		pAccept->m_AddressFamily = AddressFamily;

		*ppAccept = pAccept;

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
		
		return ERROR_SUCCESS;
}

DWORD AcceptObject::AsyncAccept()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );
	
	CAutoSNICritSec a_csTerminateListener( m_CSTerminateListener, SNI_AUTOCS_DO_NOT_ENTER );

	a_csTerminateListener.Enter();
	
	DWORD dwRet;

	Assert( m_AcceptSocket == INVALID_SOCKET );

	if( m_fTerminateListenerCalled )
	{
		dwRet = ERROR_OPERATION_ABORTED;

		// this is not an error to be reported to the user
	
		goto ErrorExit;
	}

	m_AcceptSocket = socket( m_AddressFamily, SOCK_STREAM, 0);

	if( INVALID_SOCKET == m_AcceptSocket )
	{
		dwRet = WSAGetLastError();

		// Log that we've stopped listening on the port the first
		// time we fail
		//
		if (!m_fPendingAccept)
		{
			scierrlog (26040, m_pParentListenObject->m_TcpListenInfo.Port, 
				dwRet, 1 /*state*/);
		}
	
		goto ErrorExit;
	}

	//
	//	Prevent the handle from being inherited by child processes.  
	//	We will ignore any errors though so that we keep listening
	//	for incoming connections.  
	//

	SetHandleInformation( (HANDLE)m_AcceptSocket, 
		HANDLE_FLAG_INHERIT, 
		~HANDLE_FLAG_INHERIT); 
	
	DWORD bytes_read;
	
	if (!AcceptEx(	m_ListenSocket,
				m_AcceptSocket,
				m_AddressBuffer,
				0, // read nothing from the socket
				sizeof(SOCKADDR_STORAGE) +16,
				sizeof(SOCKADDR_STORAGE) +16,
				&bytes_read,
				m_pOverlapped))
	{
		dwRet = WSAGetLastError();
		if( dwRet != ERROR_IO_PENDING)
		{
			if (!m_fPendingAccept)
			{
				scierrlog (26040, m_pParentListenObject->m_TcpListenInfo.Port, 
					dwRet, 2 /*state*/);
			}

			goto ErrorExit;
		}
	}
	else
	{
		//this assertion is to catch unexpected system api bahaviors.
		Assert( 0 && " AcceptEx should not return true immediately in an overlapped call\n" );
		BidTrace0( ERROR_TAG _T("AcceptEx should not return true immediately in an overlapped call\n") );
	}

	dwRet = ERROR_SUCCESS;
	if (m_fPendingAccept)
	{
		m_fPendingAccept = false;
		DecrementPendingAccepts();
	}

ErrorExit:

	if (dwRet != ERROR_SUCCESS)
	{
		if (!m_fPendingAccept)
		{
			m_fPendingAccept = true;
			IncrementPendingAccepts();
		}

		if ( INVALID_SOCKET != m_AcceptSocket )
		{
			closesocket( GiveAcceptSocket() );
		}
	}

	a_csTerminateListener.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

// This may seem a little counter-intuitive, but see the "Fast Multi-Subnet Failover" design spec, section "Selecting a return code..." for 
// a detailed explanation of the design goals. Quick explanation:
// The "Error Level" tracked by this enum is supposed to equate to the level of *usefulness* of the error code/message. For a
// parallel connection attempt, given that we can only return one error code to the SNI consumer over multiple connection attempts, 
// the goal of this enum is to help in picking a single error code which gives the most likely root cause of the problem to the customer.
// Strategy: 
// - run all the connections, 
// - collect all the errors and their levels, 
// - if everything fails, iterate over all the objects and select an error code with maximal "Error Level"
enum TcpConnectionErrorLevel
{
	TcpConnectionErrorLevel_None,
	TcpConnectionErrorLevel_WaitTimeout,
	TcpConnectionErrorLevel_Allocation,
	TcpConnectionErrorLevel_Win32API,
	TcpConnectionErrorLevel_WaitForObjects,
	TcpConnectionErrorLevel_OtherWinsock,
	TcpConnectionErrorLevel_Connect,
	TcpConnectionErrorLevel_CONNREFUSED,
};

#pragma region TcpConnectionPrivate
//------------------------------------------------------
// Class TcpConnection
// Purpose: Stateful abstraction for opening a client-side TCP connection
//    in two ways:
//      1) Blocking, not waitable, with INFINITE timeout (i.e., will rely on the underlying 
//            OS TCP timeout)
//      2) Asynchronous, waitable, optionally waitable with a FIXED timeout
//
//      The first way is used for standard connections (see Tcp::SocketOpenSync)
//      The second is used both for DB Mirroring, which uses the wait with timeout
//      (again, see Tcp::SocketOpenSync), and for HADRON MultiSubnetFailover, 
//      which creates several of this class, retrieves their internal events, 
//      and waits on them externally (see Tcp::SocketOpenParallel)
//      
//      NOTES: TcpConnection generally does NOT call SNI_SET_LAST_ERROR,
//          except for the TcpConnection::CalculateReturnCode static method. 
//          This is because callers of the other TcpConnection methods may have 
//          TcpConnection methods fail without actually failing the broader SNI 
//          connection attempt.
class TcpConnection
{
private:
#ifndef NDEBUG
	enum TcpConnectionState
	{
		TcpConnectionState_New,
		TcpConnectionState_ObjectInited,
		TcpConnectionState_AsyncInited,
		TcpConnectionState_PendingSynAck,
		TcpConnectionState_Completed,
		TcpConnectionState_FullyConnected,
		TcpConnectionState_Error,
	};
#endif

	OVERLAPPED *m_pOverlapped; // Overlapped structure that is used for Windows async IO
	const ADDRINFOW *m_pwTargetAddress; // a shallow pointer to an ADDRINFO struct, pointing to the remote target for this connection
	HANDLE m_hEvent; // hold a copy of the event, external to the Overlapped, so that it can be cleaned up even if we are going to leak the Overlapped struct.
	SOCKET m_socket;
	bool m_fOutstandingOverlappedIO;
	DWORD m_dwError;
	TcpConnectionErrorLevel m_eErrorLevel;
#ifndef NDEBUG
	TcpConnectionState m_State;
#endif
	int m_iBidId;
	
	// Return ConnectEx function pointer according to socket address family.
	// IPv4 and IPv6 rely on different providers, hence their ConnectEx pointers can be different.
	// LoadConnectEx requires a valid socket. See WSAIoctl in LoadConnectEx for details. Currently
	// the param 'af' must matches the socket address family.
	//
	DWORD GetConnecEx( int af, __out LPFN_CONNECTEX *ppfnConnectEx )
	{
		BidxScopeAutoSNI2( SNIAPI_TAG _T("%u#, af: %d\n"), m_iBidId, af);
		
		Assert( INVALID_SOCKET != m_socket );	
		switch( af )
		{
			case AF_INET:
			{
				DWORD dwRet = LoadConnectEx( &g_pConnectExFuncIpv4 );
				*ppfnConnectEx = (NULL == g_pConnectExFuncIpv4 ? NULL : g_pConnectExFuncIpv4->pfnConnectEx);
				return dwRet;
			}
			case AF_INET6:
			{
				DWORD dwRet = LoadConnectEx( &g_pConnectExFuncIpv6 );
				*ppfnConnectEx = (NULL == g_pConnectExFuncIpv6 ? NULL : g_pConnectExFuncIpv6->pfnConnectEx);
				return dwRet;
			}
			default:
			{
				*ppfnConnectEx = NULL;
				return ERROR_INVALID_PARAMETER;
			}
		}	
	}

	// Load ConnectEx pointer if it was not loaded before.
	// Either *ppfnCF is set to non-null in MT safe or some error happends, 
	// in which case *ppfnCF is left as NULL.
	// LoadConnectEx requires m_sock to be a valid socket. An altemative is to pass in AF and create
	// socket inside LoadConnectEx.
	//
	DWORD LoadConnectEx(__in CONNECTEXFUNC** ppfnCF )
	{
		BidxScopeAutoSNI2( SNIAPI_TAG _T( "%u#, ppfnCF: %p{CONNECTEXFUNC**}\n"), m_iBidId, ppfnCF);
		
		Assert( INVALID_SOCKET != m_socket );		
		DWORD dwRet = ERROR_SUCCESS;

		if( NULL == *ppfnCF )
		{
			CONNECTEXFUNC* pfnConnectExFunc = NewNoX(gpmo) CONNECTEXFUNC;
			
			if ( ! pfnConnectExFunc )
			{
				dwRet = ERROR_OUTOFMEMORY;
				goto ErrorExit;
			}

			ZeroMemory (pfnConnectExFunc, sizeof(CONNECTEXFUNC));
			
			GUID guidConnectEx = WSAID_CONNECTEX;
			DWORD dwBytes = 0; 

			LPFN_CONNECTEX pfnConnectEx = NULL;
			
			if( SOCKET_ERROR == WSAIoctl( m_socket,  
				SIO_GET_EXTENSION_FUNCTION_POINTER, 
				&guidConnectEx,
				sizeof(guidConnectEx),
				&pfnConnectExFunc->pfnConnectEx,
				sizeof(pfnConnectExFunc->pfnConnectEx), 
				&dwBytes, 
				NULL, 
				NULL) )
			{
				//ConnectEx is not supported for current socket type
				//update the pointer.
				//
				dwRet = WSAGetLastError();
				
				//Make sure pfnConnectExFunc->pfnConnectEx is set to NULL
				pfnConnectExFunc->pfnConnectEx = NULL;
				
				if(InterlockedCompareExchangePointer(
						reinterpret_cast<volatile PVOID*>(ppfnCF),
						reinterpret_cast<PVOID>(pfnConnectExFunc),
						NULL))
				{				
					//somebody set the pointer already.
					//
					delete pfnConnectExFunc;				
					Assert ( NULL != *ppfnCF );
					//fall thru		
				}
			}
			else
			{
				Assert( NULL != pfnConnectExFunc->pfnConnectEx );
			
				//ConnectEx is supported for current socket type.
				//update the pointer for successful case.
				//
				if(InterlockedCompareExchangePointer (
						reinterpret_cast<volatile PVOID*>(ppfnCF),
						reinterpret_cast<PVOID>(pfnConnectExFunc),
						NULL))
				{
					//somebody set the pointer already.
					//
					delete pfnConnectExFunc;
					Assert ( NULL != *ppfnCF);
					//fall thru.
				}
			}
		}	

	ErrorExit:
		
		BidTraceU2( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}, ")
										_T("ConnectExFn: %p{LPFN_CONNECTEX}\n"),
										dwRet, 
										NULL == *ppfnCF ? NULL : ((CONNECTEXFUNC*)*ppfnCF)->pfnConnectEx);
		
		return dwRet;
		
	}
	
	inline void SetConnectErrorAndLevel(DWORD dwError)
	{
		m_dwError = dwError;
		
		// We think connection refused is the most interesting error (it means that the IP host was reachable,
		// but no TCP endpoint is established on the target port), so we set a higher error level when we see it.
		//
		// Connection refused is represented by WSAECONNREFUSED, for the sync API connect(), but by 
		// ERROR_CONNECTION_REFUSED, when retrieving the error code from GetOverlappedResult(), 
		// from the Overlapped API ConnectEx().
		if( WSAECONNREFUSED == dwError || ERROR_CONNECTION_REFUSED == dwError )
		{
			m_eErrorLevel = TcpConnectionErrorLevel_CONNREFUSED;
		}
		else
		{
			m_eErrorLevel = TcpConnectionErrorLevel_Connect;
		}
	}
	
	inline void SetErrorAndLevel(DWORD dwError, TcpConnectionErrorLevel eErrorLevel)
	{
		m_dwError = dwError;
		m_eErrorLevel = eErrorLevel;
	}
	
#pragma endregion
#pragma region TcpConnectionPublic
public:
	TcpConnection() : 
		m_pOverlapped(NULL)
		,m_pwTargetAddress(NULL)
		,m_hEvent(NULL)
		,m_socket(INVALID_SOCKET)
		,m_fOutstandingOverlappedIO(false)
		,m_dwError(ERROR_SUCCESS)
		,m_eErrorLevel(TcpConnectionErrorLevel_None)
		,m_iBidId(0)
#ifndef NDEBUG
		,m_State(TcpConnectionState_New)
#endif
	{
	}
	
	~TcpConnection()
	{
		// No state assert here - delete is acceptable in any state.
		(void) CloseOutstandingSocket();
		
		if( m_fOutstandingOverlappedIO )
		{
			Assert( NULL != m_hEvent ); // if we issued an Overlapped operation, we better have an event...
			
			// Although we wait for INFINITE, we have still fixed the root cause of the problem in VSTS 
			// 238365, which was bad handling of non-cancellable sockets.  This way, we don't introduce
			// any regression in terms of memory leaks in success cases, and guarantee not to leak unless 
			// WFSO fails.	While we could in theory still wait a very long time here, we know no way for that
			// to happen in practice, including the usual problem suspects of non-IFS LSPs.
			DWORD dwError = WaitForSingleObject(m_hEvent, INFINITE);
			if( WAIT_OBJECT_0 != dwError )
			{
				if( WAIT_FAILED == dwError )
				{
					dwError = GetLastError();
					BidTraceU1( SNI_BID_TRACE_ON, ERROR_TAG _T("WaitForSingleObject() Extended Error: %d{WINERR}\n"), dwError);
				}
				else
				{
					// Anything else should not be possible, with an INFINITE timeout. Do not attempt to GLE,
					// just log whatever we got back from WFSO.
					BidTraceU1( SNI_BID_TRACE_ON, ERROR_TAG _T("WaitForSingleObject(): %d{WINERR}\n"), dwError);
				}
				// If the wait failed, there's really nothing we can do. So, give up and leak the Overlapped struct.
				m_pOverlapped = NULL;
			}
		}
		
		if( NULL != m_pOverlapped )
		{
			delete m_pOverlapped;
			m_pOverlapped = NULL;
		}
		
		m_pwTargetAddress = NULL; // This class only owns a shallow pointer to the ADDRINFO, so it shouldn't delete it.
		
		if( NULL != m_hEvent )
		{
			CloseHandle(m_hEvent);
			m_hEvent = NULL;
		}
		
		// Bid ID wasn't obtained in the constructor, so we should only try to recycle it if we actually got one.
		if( m_iBidId != 0 )
		{
			BidRecycleItemIDA( &m_iBidId, SNI_ID_TAG );
		}
	}
	
	// Initiates the socket(), bind(), and the Overlapped ConnectEx() calls, but does not wait for ConnectEx to complete
 	DWORD AsyncOpen()
 	{
		BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), m_iBidId );
		Assert( TcpConnectionState_AsyncInited == m_State );
		
		LPFN_CONNECTEX pfnConnectEx = NULL;
		DWORD dwRet = GetConnecEx(m_pwTargetAddress->ai_family, &pfnConnectEx);

		if( ERROR_SUCCESS != dwRet )
		{
			SetErrorAndLevel(dwRet,TcpConnectionErrorLevel_OtherWinsock);
			BidTrace0( ERROR_TAG _T("Could not retrieve ConnectEx function pointer.\n")); 
			goto ErrorExit;
		}

		Assert( NULL != pfnConnectEx );
		
		if( pfnConnectEx(
				m_socket, 
				m_pwTargetAddress->ai_addr,
				(int)m_pwTargetAddress->ai_addrlen,
				NULL,	//	lpSendBuffer
				0,		//	dwSendDataLength
				NULL,	//	lpdwBytesSent 
				m_pOverlapped ) )
		{
			DBG_ONLY(m_State = TcpConnectionState_FullyConnected);
			dwRet = ERROR_SUCCESS;
			goto Exit;
		}
		else
		{
			dwRet = WSAGetLastError();  
			if( ERROR_IO_PENDING != dwRet )
			{
				Assert(ERROR_SUCCESS != dwRet); // We always expect Windows to set a real GLE error code on failure
				SetConnectErrorAndLevel(dwRet);
				BidTrace1( ERROR_TAG _T("ConnectEx(): %d{WINERR}\n"), dwRet ); 
				goto ErrorExit;
			}
			else
			{
				m_fOutstandingOverlappedIO = true;
			}
		}
		DBG_ONLY(m_State = TcpConnectionState_PendingSynAck);
		
		goto Exit;
	ErrorExit:
		DBG_ONLY(m_State = TcpConnectionState_Error);
		
	Exit:
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
		return dwRet;
	}

	static DWORD CalculateReturnCode(__in_ecount(dwcTcpConnections) TcpConnection *rgTcpConnections, DWORD dwcTcpConnections, DWORD dwParentReturnCode, TcpConnectionErrorLevel eParentReturnCodeErrorLevel)
	{
		BidxScopeAutoSNI4( SNIAPI_TAG _T("rgTcpConnections: %p{TcpConnection*}, ")
									_T("dwcTcpConnections: %d{DWORD}, ")
									_T("dwParentReturnCode: %d{DWORD}, ")
									_T("eParentReturnCodeErrorLevel: %d{TcpConnectionErrorLevel}\n"),
									rgTcpConnections,
									dwcTcpConnections,
									dwParentReturnCode,
									eParentReturnCodeErrorLevel);
		
		DWORD dwRet = dwParentReturnCode;
		TcpConnectionErrorLevel eReturnCodeLevel = eParentReturnCodeErrorLevel;
		
		bool fUsedAConnectionReturnCode = false;
		DWORD dwConnectionUsed;
		
		for(DWORD i=0;i < dwcTcpConnections;i++)
		{
			if( rgTcpConnections[i].m_eErrorLevel > eReturnCodeLevel )
			{
				dwRet = rgTcpConnections[i].m_dwError;
				eReturnCodeLevel = rgTcpConnections[i].m_eErrorLevel;
				dwConnectionUsed = i;
				fUsedAConnectionReturnCode = true;
			}
		}
		if( fUsedAConnectionReturnCode )
		{
			BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("Using return code from connection: %u#\n"), rgTcpConnections[dwConnectionUsed].m_iBidId);
		}
		
		// NOTE: The below assumes that all possible errors from the TcpConnection class are SNIE_SYSTEM.
		// Currently, all failures are either OOM or system API failures, so this is acceptable.
		SNI_SET_LAST_ERROR(TCP_PROV, SNIE_SYSTEM, dwRet);
        
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
		return dwRet;
	}
	
	// CONTRACT: This API should only be called when the outstanding Overlapped IO has completed, e.g. WaitForSingleObject on the Overlapped event handle has already been done.
	// CONTRACT: This API should only be called once. If it is called more than once, the resulting state of the object is not defined.
	DWORD CheckCompletedAsyncConnect()
	{
		BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), m_iBidId );
		Assert( TcpConnectionState_Completed == m_State );
		DWORD dwRet;
		DWORD dwBytesRead;
		Assert( !m_fOutstandingOverlappedIO );
		
		if( GetOverlappedResult((HANDLE)m_socket, 
								m_pOverlapped, 
								&dwBytesRead, 
								FALSE ))
		{
			// Connected.  
			//				
			dwRet = ERROR_SUCCESS;
			
			// Update context for ConnectEx so that certain socket calls can be made.
			//
			if(SOCKET_ERROR == setsockopt(m_socket, 
							SOL_SOCKET, 
							SO_UPDATE_CONNECT_CONTEXT, 
							NULL, 
							0))
			{
				dwRet = WSAGetLastError();
				SetErrorAndLevel(dwRet,TcpConnectionErrorLevel_OtherWinsock);
				goto ErrorExit;				
			}
			
			DBG_ONLY(m_State = TcpConnectionState_FullyConnected);
		}
		else
		{
			dwRet = GetLastError();
			SetConnectErrorAndLevel(dwRet);
			goto ErrorExit;
		}
		
		goto Exit;
		
	ErrorExit:
		DBG_ONLY(m_State = TcpConnectionState_Error);
		
	Exit:
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
		return dwRet;
	}
	
	DWORD CloseOutstandingSocket()
	{
		BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), m_iBidId);
		DWORD dwRet = ERROR_SUCCESS;
		
		if( INVALID_SOCKET != m_socket )
		{
			// Note: this shutdown() call will often fail with WSAENOTCONN (10057), since
			// shutdown() is only applicable on a fully-connected socket. 
			// However,
			// 1) There's no downside to calling shutdown() when the socket isn't yet connected,
			// 2) If the socket actually is connected, it helps the socket clean up faster, and
			// 3) It's extra work (and open to race conditions) for us to keep closer track of the socket state here.
			// So, we just blindly call shutdown() and log an error, if one occurs.
			int iRet = shutdown(m_socket, SD_SEND);
			if( 0 != iRet )
			{
				if( SOCKET_ERROR == iRet )
				{
					dwRet = WSAGetLastError();
					BidTrace1(ERROR_TAG _T("shutdown() Extended Error: %d{WINERR}."), dwRet);
				}
				else
				{
					dwRet = iRet;
					BidTrace1(ERROR_TAG _T("shutdown(): %d{WINERR}."), iRet);
				}
			}
			// even if shutdown fails, continue to call closesocket(), and only return the return code of closesocket()
			iRet = closesocket(m_socket);
			if( 0 != iRet )
			{
				if( SOCKET_ERROR == iRet )
				{
					dwRet = WSAGetLastError();
					BidTrace1(ERROR_TAG _T("closesocket() Extended Error: %d{WINERR}."), dwRet);
				}
				else
				{
					dwRet = iRet;
					BidTrace1(ERROR_TAG _T("closesocket() Extended Error: %d{WINERR}."), iRet);
				}
			}
			m_socket = INVALID_SOCKET;
		}
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
		return dwRet;
	}
	
	// Allocates objects and allocates/binds the socket - does not actually open the socket.
	DWORD FInit(const Tcp *pTcp, const ADDRINFOW *pwTargetAddress)
	{
		BidxScopeAutoSNI2( SNIAPI_TAG _T("pTcp: %p{Tcp}, pwTargetAddress: %p{ADDRINFOW*}\n"), pTcp, pwTargetAddress );
		
		// Also trace out the IP address
		switch( pwTargetAddress->ai_family )
		{
			case AF_INET: 
			{
				if( pwTargetAddress->ai_addrlen >= sizeof(SOCKADDR_IN) )
				{
					ULONG ulAddress = ((SOCKADDR_IN*)(pwTargetAddress->ai_addr))->sin_addr.s_addr;
					BidTraceU4(SNI_BID_TRACE_ON, SNI_TAG _T("target IPv4 address: %u.%u.%u.%u\n"), 
						ulAddress & 0xFF, 
						(ulAddress >> 8) & 0xFF, 
						(ulAddress >> 16) & 0xFF, 
						(ulAddress >> 24) & 0xFF);
					break;
				}
			}
			case AF_INET6:
			{
				if( pwTargetAddress->ai_addrlen >= sizeof(SOCKADDR_IN6) )
				{
					in6_addr *psin6_addr = &(((SOCKADDR_IN6*)(pwTargetAddress->ai_addr))->sin6_addr);
					BidTraceU8(SNI_BID_TRACE_ON, SNI_TAG _T("target IPv6 address: %x:%x:%x:%x:%x:%x:%x:%x\n"), 
						ntohs(psin6_addr->s6_words[0]), 
						ntohs(psin6_addr->s6_words[1]), 
						ntohs(psin6_addr->s6_words[2]), 
						ntohs(psin6_addr->s6_words[3]), 
						ntohs(psin6_addr->s6_words[4]), 
						ntohs(psin6_addr->s6_words[5]), 
						ntohs(psin6_addr->s6_words[6]), 
						ntohs(psin6_addr->s6_words[7]));
				}
			}
			default:
				break;
		}
		
		Assert( TcpConnectionState_New == m_State );
		DWORD dwRet;
		
		BidObtainItemID2A( &m_iBidId, SNI_ID_TAG "%p{.} created by %u#{Tcp}", 
			this, pTcp->GetBidId() );
		
		m_socket = socket( pwTargetAddress->ai_family, pwTargetAddress->ai_socktype, pwTargetAddress->ai_protocol );
		
		if( INVALID_SOCKET == m_socket )
		{
			dwRet = WSAGetLastError();
			SetErrorAndLevel(dwRet,TcpConnectionErrorLevel_OtherWinsock);
			BidTrace1( ERROR_TAG _T("socket(): %d{WINERR}\n"), dwRet ); 
			goto ErrorExit;
		}
		
		if( !SetHandleInformation( 
			(HANDLE)m_socket, 
			HANDLE_FLAG_INHERIT, 
			~HANDLE_FLAG_INHERIT))
		{
			dwRet = GetLastError();
			SetErrorAndLevel(dwRet,TcpConnectionErrorLevel_Win32API);
			BidTrace1( ERROR_TAG _T("SetHandleInformation(): %d{WINERR}\n"), dwRet ); 
			goto ErrorExit;
		}
		
		m_pwTargetAddress = pwTargetAddress;
		dwRet = ERROR_SUCCESS;
		
		DBG_ONLY(m_State = TcpConnectionState_ObjectInited);
		goto Exit;
		
	ErrorExit:
		
		DBG_ONLY(m_State = TcpConnectionState_Error);
		
	Exit:
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
		return dwRet;
	}
	
	DWORD FInitForAsync()
	{
		BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), m_iBidId);
		Assert( TcpConnectionState_ObjectInited == m_State );
		DWORD dwRet;
		
		ADDRINFOW LocalHints;
		ADDRINFOW *pwLocalAddress = NULL;
		
		m_pOverlapped = NewNoX(gpmo) OVERLAPPED;
		if( NULL == m_pOverlapped )
		{
			dwRet = ERROR_OUTOFMEMORY;
			SetErrorAndLevel(dwRet,TcpConnectionErrorLevel_Allocation);
			goto ErrorExit;
		}
		ZeroMemory( m_pOverlapped, sizeof( OVERLAPPED ) ); 

		m_hEvent = CreateEvent(NULL,	// lpEventAttributes - use default
								TRUE,	// bManualReset - we don't want it ever to be reset, since we only use it once
								FALSE,	// bInitialState - start unset
								NULL );	// lpName - no need for a name
		if( NULL == m_hEvent )
		{
			dwRet = GetLastError();
			SetErrorAndLevel(dwRet,TcpConnectionErrorLevel_Win32API);
			BidTrace1( ERROR_TAG _T("CreateEvent(): %d{WINERR}\n"), dwRet ); 
			goto ErrorExit;
		}
		m_pOverlapped->hEvent = m_hEvent;
		
		ZeroMemory( &LocalHints, sizeof(LocalHints) );
		LocalHints.ai_family = m_pwTargetAddress->ai_family;
		LocalHints.ai_socktype = SOCK_STREAM;
		LocalHints.ai_flags = AI_PASSIVE;
		
		// Obtain the wildcard local address
		if( GetAddrInfoW_l( NULL, L"0", &LocalHints, &pwLocalAddress, GetDefaultLocale() ) )
		{
			dwRet = WSAGetLastError();
			SetErrorAndLevel(dwRet,TcpConnectionErrorLevel_Win32API);
			BidTrace1( ERROR_TAG _T("GetAddrInfoW_l(): %d{WINERR}\n"), dwRet ); 
			goto ErrorExit;
		}
		
		if( SOCKET_ERROR == bind( m_socket, 
								pwLocalAddress->ai_addr, 
								(int)pwLocalAddress->ai_addrlen ))
		{
			dwRet = WSAGetLastError();
			SetErrorAndLevel(dwRet,TcpConnectionErrorLevel_OtherWinsock);
			BidTrace1( ERROR_TAG _T("bind(): %d{WINERR}\n"), dwRet ); 
			goto ErrorExit;
		}
		
		dwRet = ERROR_SUCCESS;
		DBG_ONLY(m_State = TcpConnectionState_AsyncInited);
		goto Exit;
		
	ErrorExit:
		
		DBG_ONLY(m_State = TcpConnectionState_Error);
		
	Exit:
		// once the bind call is complete, we no longer need the local address
		if( NULL != pwLocalAddress )
		{
			FreeAddrInfoW(pwLocalAddress);
			pwLocalAddress = NULL;
		}
		
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
		return dwRet;
	}
	
	// accessor to use the class's Event - does not relinquish ownership. Only expected to be used when there actually is an outstanding Overlapped IO on this TcpConnection.
 	HANDLE GetEventForOutstandingOverlappedIO()
	{
		Assert( m_fOutstandingOverlappedIO );
		Assert( TcpConnectionState_PendingSynAck == m_State );
		return m_hEvent;
	}
	
	void NotifyAboutOverlappedIOCompletion()
	{
		// If this API is called, there must have previously been an outstanding IO which is now completed.
		Assert(m_fOutstandingOverlappedIO);
		
		m_fOutstandingOverlappedIO = false;
		DBG_ONLY(m_State = TcpConnectionState_Completed);
	}
	
 	// relinquish ownership of the socket to the caller
 	SOCKET RelinquishSocket()
	{
		Assert(TcpConnectionState_FullyConnected == m_State);
		SOCKET socketToReturn = m_socket;
		m_socket = INVALID_SOCKET;
		return socketToReturn;
	}
	
	DWORD SyncOpen()
	{
		BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), m_iBidId );
		Assert( TcpConnectionState_ObjectInited == m_State );
		DWORD dwRet = ERROR_SUCCESS;
		
		if( SOCKET_ERROR == connect(m_socket, m_pwTargetAddress->ai_addr, (int)m_pwTargetAddress->ai_addrlen ) )
		{
			dwRet = WSAGetLastError();
			SetConnectErrorAndLevel(dwRet);
			BidTrace1( ERROR_TAG _T("connect(): %d{WINERR}\n"), dwRet);
			
			DBG_ONLY(m_State = TcpConnectionState_Error);
		}
		else
		{
			DBG_ONLY(m_State = TcpConnectionState_FullyConnected);
		}
		
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
		return dwRet;
	}
	
	DWORD WaitForCompletion(int timeout)
	{
		BidxScopeAutoSNI2( SNIAPI_TAG _T("%u#, timeout: %d\n"), m_iBidId, timeout );
		Assert( TcpConnectionState_PendingSynAck == m_State );
		Assert( m_fOutstandingOverlappedIO );
		DWORD dwRet;

		// First, wait for completion
		dwRet = WaitForSingleObject(m_pOverlapped->hEvent, timeout);
		
		if( WAIT_OBJECT_0 == dwRet )	// Get ConnectEx event notification
		{
			// Once we're done waiting, we still need to check the status.
			m_fOutstandingOverlappedIO = false;
			DBG_ONLY(m_State = TcpConnectionState_Completed);
			dwRet = CheckCompletedAsyncConnect();
			 // in case of error from CheckCompletedAsyncConnect, DBG_ONLY state and SNI_SET_LAST_ERROR are already done. 
			 // So, go straight to exit, regardless of result
			goto Exit;
		}
		else if( WAIT_TIMEOUT == dwRet )
		{
			dwRet = WSA_WAIT_TIMEOUT;
			SetErrorAndLevel(dwRet,TcpConnectionErrorLevel_WaitTimeout);
			goto ErrorExit;
		}
		else
		{
			dwRet = GetLastError();
			SetErrorAndLevel(dwRet,TcpConnectionErrorLevel_WaitForObjects);
			// Error condition from WFSO doesn't give guarantees about Overlapped IO state.
			// We couldn't Wait, and we can't determine if the Overlapped IO is done.
			// So, in this case, we have no choice but to leak the Overlapped.
			m_pOverlapped = NULL;
			BidTraceU1( SNI_BID_TRACE_ON, ERROR_TAG _T("WaitForSingleObject: %d{WINERR}\n"), dwRet);
			goto ErrorExit;
		}

		goto Exit;
		
	ErrorExit:
		DBG_ONLY(m_State = TcpConnectionState_Error);
		
	Exit:
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
		return dwRet;
	}
	
};

#pragma endregion

//listen on the Port on particular interface
//if the Port is 0 then we will set it to the port number system gives us
DWORD ListenPort( __in ADDRINFOW *AIW, __out SOCKET *pSock, __inout USHORT *pPort )
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("AIW: %p{ADDRINFOW*}, pSock: %p{SOCKET*}, pPort: %p{USHORT*}\n"), 
					AIW, pSock, pPort );
	
	DWORD dwRet = ERROR_FAIL;
	
	SOCKET sock = INVALID_SOCKET;

	switch( AIW->ai_family )
	{
	case AF_INET:
		
OACR_WARNING_SUPPRESS(IPV6_ADDRESS_STRUCTURE_IPV4_SPECIFIC , " TEMPORARY SUPPRESSION UNTIL DEVELOPER'S ANALYSIS. See VSTS 376830 ")
		if (AIW->ai_addrlen < sizeof(SOCKADDR_IN))
		{
OACR_WARNING_PUSH
OACR_WARNING_DISABLE(IPV6_ADDRESS_STRUCTURE_IPV4_SPECIFIC , " TEMPORARY SUPPRESSION UNTIL DEVELOPER'S ANALYSIS. See VSTS 376830 ")
			BidTrace2( ERROR_TAG _T("Invalid buffer length of IPv4 address member of ADDRINFOW parameter. %d{size_t}, required: %d{size_t}\n"),
				AIW->ai_addrlen, sizeof(SOCKADDR_IN) );
OACR_WARNING_POP
			Assert(0 && " Invalid buffer length of IPv4 address member of ADDRINFOW parameter\n");
			dwRet = ERROR_INVALID_PARAMETER;
			SNI_SET_LAST_ERROR( TCP_PROV, SNIE_5, dwRet );	// "Invalid parameter(s) found"
			scierrlog (26062, dwRet, 1 /*state*/);
			goto ErrorExit;
		}
		((SOCKADDR_IN *)AIW->ai_addr)->sin_port = htons(*pPort);
		break;

	case AF_INET6:

		if (AIW->ai_addrlen < sizeof(SOCKADDR_IN6))
		{
			BidTrace2( ERROR_TAG _T("Invalid buffer length of IPv6 address member of ADDRINFOW parameter. %d{size_t}, required:  %d{size_t}\n"), 
				AIW->ai_addrlen, sizeof(SOCKADDR_IN6) );
			Assert(0 && " Invalid buffer length of IPv6 address member of ADDRINFOW parameter\n");
			dwRet = ERROR_INVALID_PARAMETER;
			SNI_SET_LAST_ERROR( TCP_PROV, SNIE_5, dwRet );	// "Invalid parameter(s) found"
			scierrlog (26062, dwRet, 2 /*state*/);
			goto ErrorExit;
		}
		((SOCKADDR_IN6 *)AIW->ai_addr)->sin6_port = htons(*pPort);
		break;

	default:
		//this assertion is used to catch unexpected function usage.
		Assert( 0 && " IP family number is unknown\n" );
		BidTrace1( ERROR_TAG _T("IP family number is unknown. %d{DWORD}\n"), AIW->ai_family );
		scierrlog (26062, dwRet, 3 /*state*/);
		goto ErrorExit;
	}

	sock = socket( AIW->ai_family, AIW->ai_socktype, AIW->ai_protocol);
	if( INVALID_SOCKET == sock )
	{
		dwRet = WSAGetLastError();

		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );

		goto ErrorExit;
	}
	
	if( !SetHandleInformation( (HANDLE)sock, HANDLE_FLAG_INHERIT, ~HANDLE_FLAG_INHERIT))
	{
		dwRet = GetLastError();

		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );

		goto ErrorExit;
	}

	BOOL optVal = TRUE;
	
	if( SOCKET_ERROR == setsockopt( sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
									( const char *)&optVal, sizeof( optVal )))
	{
		dwRet = WSAGetLastError();

		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );

		goto ErrorExit;
	}
	
	if( SOCKET_ERROR == bind( sock, AIW->ai_addr, (int)AIW->ai_addrlen ))
	{
		dwRet = WSAGetLastError();

		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );

		goto ErrorExit;
	}
		
	if( 0 == *pPort )
	{
		if( AIW->ai_family == AF_INET )
		{
OACR_WARNING_SUPPRESS(IPV6_ADDRESS_STRUCTURE_IPV4_SPECIFIC , " TEMPORARY SUPPRESSION UNTIL DEVELOPER'S ANALYSIS. See VSTS 376830 ")
			SOCKADDR_IN addr4;
			
			int len = sizeof addr4;
			
			if( SOCKET_ERROR == getsockname( sock, (LPSOCKADDR)&addr4, &len ))
			{
				dwRet = WSAGetLastError();

				SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );

				goto ErrorExit;
			}
			
			*pPort = ntohs( addr4.sin_port );
		}
		else
		{
			Assert( AIW->ai_family == AF_INET6);

			SOCKADDR_IN6 addr6;
			
			int len = sizeof addr6;
			
			if( SOCKET_ERROR == getsockname( sock, (LPSOCKADDR)&addr6, &len ))
			{
				dwRet = WSAGetLastError();

				SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );

				goto ErrorExit;
			}
			
			*pPort = ntohs( addr6.sin6_port );
		}
	}

	if( SOCKET_ERROR == listen( sock, SOMAXCONN ))
	{
		dwRet = WSAGetLastError();

		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );

		goto ErrorExit;
	}

	*pSock = sock;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;

ErrorExit:

	if( INVALID_SOCKET != sock )
	{
		closesocket( sock );
	}

	*pSock = INVALID_SOCKET;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}



DWORD InterfaceListen( __inout TcpListenInfo *pListenInfo, __inout ListenObject *pListenObject)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("pListenInfo: %p{TcpListenInfo*}, pListenObject: %p{ListenObject*}\n"), 
					pListenInfo, pListenObject );
	
	DWORD dwRet = ERROR_FAIL;
	BOOL fSystemAddrInfo = TRUE; 
	WCHAR * wszServer = pListenInfo->ipaddress; 
	
	ADDRINFOW *AddrInfoW=0;
	ADDRINFOW *AIW, Hints;

	ZeroMemory( &Hints, sizeof Hints );
	Hints.ai_family = PF_UNSPEC;
	Hints.ai_socktype = SOCK_STREAM;
	Hints.ai_flags = AI_PASSIVE;

#if !defined(SNI_BASED_CLIENT) && !defined(STANDALONE_SNI)

	// gClusApi.hClusterLibrary and gClusApi.hResourceLibrary are both initialized only if it's virtual server (cluster)
	if( gClusApi.hClusterLibrary && gClusApi.hResourceLibrary )
	{

		wszServer = gwszClusterName;			

		THREAD_PREEMPTIVE_ON_START (PWAIT_PREEMPTIVE_OS_CLUSTEROPS); 
		dwRet = GetClusterListenIpAddrInfo( gwszClusterName,  &AddrInfoW );
		THREAD_PREEMPTIVE_ON_END; 

		if( ERROR_SUCCESS != dwRet )
		{
			scierrlog(26054, dwRet);
			goto ErrorExit;
		}

		dwRet = SNI_ServiceBindings::SetClusterAddresses(AddrInfoW);

		if( ERROR_SUCCESS != dwRet )
		{
			// 26066 = SNI_EP_CLUSTER_IP_ADDRESS_FAILURE:
			// "An error occurred while configuring cluster virtual IP addresses for Extended Protection. Error: %d."
			scierrlog(26066, dwRet);
			goto ErrorExit;
		}

		// AddrInfoW should be released later. 
		Assert ( AddrInfoW );

	}
	else if( !wszServer && 
		( 0 != ( pListenInfo->dwFlags & SNI_TCP_INFO_FLAG_LOOPBACK ) ) )
	{
		// stand-alone listening on loopback addresses
		dwRet = GetLoopbackListenIpAddrInfo( &AddrInfoW ); 

		if( ERROR_SUCCESS != dwRet )
		{
			Assert( NULL == AddrInfoW ); 
			goto ErrorExit;
		}

		// AddrInfoW should be released later by our own function instead of
		// Winsock's freeaddrinfo(). 
		fSystemAddrInfo = FALSE; 
		Assert ( AddrInfoW );		
	}
	else
#endif
	{
		//not a cluster, use DNS query	
		if( GetAddrInfoW_l( wszServer, L"0", &Hints, &AddrInfoW, GetDefaultLocale()))
		{
			dwRet = WSAGetLastError();

			SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );

			goto ErrorExit;
		}

	}
	
	int nRetryLimit;
	
	if( pListenInfo->fStatic )
	{
		//
		// for static ports there is no retry
		//
		
		nRetryLimit = 1;
	}
	else
	{
		nRetryLimit = 1000;
	}
	
	SOCKET sock;

	USHORT Port = pListenInfo->Port;

	int nTempSockets = 0;

	for( AIW = AddrInfoW; AIW;  )
	{
		WCHAR * wszIpAddressToLog = wszServer; 
		WCHAR   wszRetreivedIpAddress[NI_MAXHOST + 1];  

		if( AIW->ai_family != AF_INET && AIW->ai_family != AF_INET6 )
		{
			AIW = AIW->ai_next;
			continue;
		}

		//Sometimes addrinfo gives us a ipv6/ipv4 address to listen on even
		//when it is not supported so we do a manual check here
		if( AIW->ai_family==AF_INET6 && false == g_fIpv6Supported )
		{
			AIW = AIW->ai_next;
			continue;
		}
		else if( AIW->ai_family==AF_INET && false == g_fIpv4Supported )
		{
			AIW = AIW->ai_next;
			continue;
		}

		// If not listening on IP_ANY, we will retreive the IP address
		// Winsok is actually listening on instead of loggin what we 
		// passed in.  This is particularly important for clusters, 
		// where we pass in a virtual server name, and want to log the 
		// cluster IP addresses the server is listening on.  
		//
		if( wszServer || 
			( 0 != ( pListenInfo->dwFlags & SNI_TCP_INFO_FLAG_LOOPBACK ) ) )
		{
			wszRetreivedIpAddress[sizeof(wszRetreivedIpAddress) /
				sizeof(wszRetreivedIpAddress[0]) - 1] = 0x00;
			
			// Note: if getaneminfo() fails, we will log whatever we passed 
			// into Winsock.  This should not really happen.  
			//
	        if( 0 == GetNameInfoW_l( AIW->ai_addr, 
								  static_cast<socklen_t>(AIW->ai_addrlen), 
								  wszRetreivedIpAddress,
	                       		  ARRAYSIZE(wszRetreivedIpAddress), 
	                       		  NULL, //	not interested in "service"
	                       		  0, 
	                       		  NI_NUMERICHOST, GetDefaultLocale()) )
	        {
				wszIpAddressToLog = wszRetreivedIpAddress; 
	        }
		}
		
		
		dwRet = ListenPort( AIW, &sock, &Port );
		if( ERROR_SUCCESS != dwRet )
		{
			// Winsock can return either WSAEADDRINUSE or WSAEACCES
			// if the port is in use (see SQL BU DT 360455; retry on WSAEACCES
			// only on non-zero port; preserving the existing behavior for 
			// WSAEADDRINUSE).  
			//
			if( ( WSAEADDRINUSE == dwRet ) || 
				( ( WSAEACCES == dwRet ) && ( 0 != Port ) ) )
			{
					while( nTempSockets )
					{
						pListenObject->Remove();
						nTempSockets--;
					}

					if( --nRetryLimit  == 0 )
					{
						scierrlog(	26023,
							wszIpAddressToLog ? wszIpAddressToLog:L"\'any\'",
							AIW->ai_family==AF_INET6 ? L"ipv6":L"ipv4",
							pListenInfo->Port );  //On erroring out, we log the original port number passed in

						goto ErrorExit;
					}
					
					Port = 0;
					AIW = AddrInfoW;
					continue;
			}
			else if ( WSAEAFNOSUPPORT == dwRet )
			{	
				if( AIW->ai_family == AF_INET )  
				{
					g_fIpv4Supported  =  false;
				}
				else if( AIW->ai_family == AF_INET6 ) 
				{
					g_fIpv6Supported  = false;
				}
				
				continue;
			}

			else
			{
				while( nTempSockets )
				{
					pListenObject->Remove();
					nTempSockets--;
				}
					
				scierrlog(	26024,
							wszIpAddressToLog ? wszIpAddressToLog:L"\'any\'",
							AIW->ai_family==AF_INET6 ? L"ipv6":L"ipv4",
							pListenInfo->Port,  //On erroring out, we log the original port number passed in
							dwRet);
				
				goto ErrorExit;
			}
		}

		nTempSockets++;

		dwRet = pListenObject->Add( sock, AIW->ai_family);
		if( ERROR_SUCCESS != dwRet )
		{
			closesocket(sock);
			goto ErrorExit;
		}

		scierrlog(	26022,
				wszIpAddressToLog ? wszIpAddressToLog:L"\'any\'",
				AIW->ai_family==AF_INET6 ? L"ipv6":L"ipv4",
				Port); //On binding success, we log the binded port number.

		AIW = AIW->ai_next;
	}

	if( 0 >= nTempSockets )
	{
		if ( ERROR_SUCCESS == dwRet )
		{
			dwRet = ERROR_FAIL;
		}
		goto ErrorExit;
	}


	if( Port != pListenInfo->Port )
	{
		Assert( !pListenInfo->fStatic );
		pListenInfo->fUpdated = true;
		pListenInfo->Port = Port;
	}

	if( fSystemAddrInfo )
	{
		FreeAddrInfoW( AddrInfoW );
	}
	else
	{
		FreeCustomAddrInfo( AddrInfoW ); 
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;

ErrorExit:

	if( AddrInfoW )
	{
		if( fSystemAddrInfo )
		{		
			FreeAddrInfoW( AddrInfoW );
		}
		else
		{
			FreeCustomAddrInfo( AddrInfoW ); 
		}
	}		

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;	
}


bool IsIpVersionSupported( int iAddressFamily )
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );

	int protocols[]={IPPROTO_TCP,0};

	WSAPROTOCOL_INFO *buf=0;
	DWORD buflen=0;

	int nprotocols;

	nprotocols = WSAEnumProtocols( protocols, buf, &buflen);

	if( buflen==0 )
	{
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), FALSE);
		return false;
	}

	buf = (WSAPROTOCOL_INFO *)NewNoX(gpmo) char[buflen];

	if( !buf )
	{
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), FALSE);
		return false;
	}
	
	nprotocols = WSAEnumProtocols( protocols, buf, &buflen);

	for( int i=0; i<nprotocols; i++)
		if(buf[i].iAddressFamily == iAddressFamily )
		{			
			delete [] (char *) buf;			
			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), TRUE);
			return true;			
		}

	delete [] (char *) buf;
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), FALSE);
	return false;
}

Tcp::Tcp(SNI_Conn * pConn) : SNI_Provider(pConn), m_pSyncReadPacket(0)
{
	m_Prot = TCP_PROV;
	m_sock = INVALID_SOCKET;
	m_CSClose = NULL; 
	m_fClose = FALSE; 
	m_dwcHandleRef = 0; 

	m_lCloseCalled = 0;

	m_fAuto = FALSE;
	m_pOvSendNotificaiton = NULL;
	m_CSTuning = NULL;
	
	BidObtainItemID2A( &m_iBidId, SNI_ID_TAG "%p{.} created by %u#{SNI_Conn}", 
		this, pConn->GetBidId() );
}

Tcp::~Tcp()
{
	BidRecycleItemIDA( &m_iBidId, SNI_ID_TAG ); 

	if( m_CSClose != NULL )
		DeleteCriticalSection( &m_CSClose );

	if( m_CSTuning != NULL )
		DeleteCriticalSection( &m_CSTuning);

	if( NULL != m_pOvSendNotificaiton )
	{
		Assert(Tcp::s_fAutoTuning);
		
		if( NULL != m_pOvSendNotificaiton->hEvent )
		{
			CloseHandle( m_pOvSendNotificaiton->hEvent );
			m_pOvSendNotificaiton->hEvent = NULL;
		}
		
		delete m_pOvSendNotificaiton;
		m_pOvSendNotificaiton = NULL;		
	}
}


DWORD Tcp::FInit()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	DWORD dwError = SNICritSec::Initialize(&m_CSClose);

	if( ERROR_SUCCESS != dwError )
		goto ErrorExit;

	if(!Tcp::s_fAutoTuning)
	{
		//Skip initialize m_pOvSendNotificaiton because autotuning is not available.
		//
		Assert(ERROR_SUCCESS == dwError);
		goto ErrorExit;
	}

	m_pOvSendNotificaiton = NewNoX(gpmo) WSAOVERLAPPED;
	
	if( NULL == m_pOvSendNotificaiton )
	{
		dwError = ERROR_OUTOFMEMORY;
		goto ErrorExit;
	}

	ZeroMemory(m_pOvSendNotificaiton,sizeof(WSAOVERLAPPED));
	
	m_pOvSendNotificaiton->hEvent = CreateEvent( NULL,	// lpEventAttributes
											TRUE,	// bManualReset
											FALSE,	// bInitialState
											NULL );	// lpName

	if(NULL == m_pOvSendNotificaiton->hEvent)
	{
		dwError = GetLastError();
		goto ErrorExit;
	}

	//odd the event handle so that overlaped event won't be delivered to the completion port.
	m_pOvSendNotificaiton->hEvent =(HANDLE)((UINT_PTR)m_pOvSendNotificaiton->hEvent|1);

	dwError = SNICritSec::Initialize(&m_CSTuning);

	if( ERROR_SUCCESS != dwError )
		goto ErrorExit;

ErrorExit:

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}


//  Tcp::FAddHandleRef
//
//  Returns:
//		TRUE =	the handle is not (being) closed, and we successfully 
//				obtained a reference because.  
//		FALSE = we did not obtain a reference because the handle is (being)
//				closed.  
//
BOOL Tcp::FAddHandleRef()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	BOOL fClose = FALSE;  
	CAutoSNICritSec a_csClose(m_CSClose, SNI_AUTOCS_DO_NOT_ENTER );

	a_csClose.Enter(); 
	
	fClose = m_fClose; 

	if( !fClose )
	{
		m_dwcHandleRef++; 
	}

	a_csClose.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), !fClose);
	return !fClose; 
}

//  Tcp::ReleaseHandleRef
//
//	Release a reference on the socket handle.  If it was marked for closure,
//	and we are the last user close it.  
//
void Tcp::ReleaseHandleRef()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	BOOL fNeedClose = FALSE;  
	CAutoSNICritSec a_csClose(m_CSClose, SNI_AUTOCS_DO_NOT_ENTER );

	a_csClose.Enter(); 

	m_dwcHandleRef--; 			
	fNeedClose = m_fClose && ( 0 == m_dwcHandleRef );  
	
	a_csClose.Leave(); 

	// If the handle was marked for closure in the meantime and we are the last
	// ones using teh handle close the handle.  
	//
	if( fNeedClose )
	{
		BidTraceU0( SNI_BID_TRACE_ON, SNI_TAG _T("Closing handle\n") ); 

		int ret = closesocket(m_sock);	
		
		//When closesocket fails due to memory pressure, we log the error and proceed.
		//such solution might cause socket handle leaking. SQL BU 293226.
		if( SOCKET_ERROR == ret )
		{
			ret = WSAGetLastError();
			scierrlog( 26035, ret); 
			BidTrace1(ERROR_TAG _T("winsock closesocket failed:%d{WINERR}\n"), ret );
		}

		m_sock = INVALID_SOCKET;

		Assert( !m_pSyncReadPacket ); 
	}
}


//  Tcp::FCloseRefHandle
//
//	Mark the socket handle for closure to stop any new callers using it, 
//	and if noone is currently using it close it.  
//
//  Returns:
//		TRUE =	the hanlde was successfully closed.  
//		FALSE = otherwise.  
//
BOOL Tcp::FCloseRefHandle()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	// Protect against race conditions with functions that use the socket handle, 
	// and are not protected by the SNI API usage requirements.  
	//
	CAutoSNICritSec a_csClose(m_CSClose, SNI_AUTOCS_DO_NOT_ENTER );
	DWORD dwcHandleRef = 0; 
	BOOL fRet = FALSE; 

	a_csClose.Enter(); 
	
	m_fClose = TRUE;  
	dwcHandleRef = m_dwcHandleRef; 

	a_csClose.Leave(); 

	// Close the handle only if no completion routine is currently using it.  
	// Otherwise, a completion routine will close it instead. 
	//
	if( 0 == dwcHandleRef )
	{
		BidTraceU0( SNI_BID_TRACE_ON, SNI_TAG _T("Closing handle\n") ); 

		int ret = closesocket(m_sock);	
		
		//When closesocket fails due to memory pressure, we log the error and proceed.
		//such solution might cause socket handle leaking. SQL BU 293226.
		if( SOCKET_ERROR == ret )
		{
			ret = WSAGetLastError();
			scierrlog( 26035, ret); 
			BidTrace1(ERROR_TAG _T("winsock closesocket failed:%d{WINERR}\n"), ret );
		}
		else
		{
			fRet = TRUE; 
		}
	
		m_sock = INVALID_SOCKET;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), fRet);
	return fRet; 
}


//
// this function doesn't return an error code or call SNISetLastError(), 
// because this is not gonna end up as a callback to user
// it does its best to successfully prepare a connection
// if it can't, it ignores the error and return. so that the worker thread can continue accepting new connections
//

Tcp * Tcp::AcceptConnection( SNI_Conn *pConn, SOCKET AcceptSocket, char * szAddressBuffer)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("pConn: %p{SNI_Conn*}, AcceptSocket: %Iu{SOCKET}\n"), 
					pConn, AcceptSocket );

	Tcp * pTcp = NULL;
	DWORD dwError = ERROR_SUCCESS;

	if( pConn )
		pTcp = NewNoX(gpmo) Tcp(pConn);

	if( !pTcp )
	{
		/// we should consider error reporting in this case
		
		goto ErrorExit;
	}

	pTcp->m_sock = AcceptSocket;

	if( ERROR_SUCCESS != pTcp->FInit() )
	{
		goto ErrorExit;
	}

	// Set socket to receive out-of-band data in except case of select
	// 
	BOOL	foptval  = TRUE;
	int		ioptlen  = sizeof(int);
	setsockopt( pTcp->m_sock, SOL_SOCKET, SO_KEEPALIVE,
		        (char *)&foptval, ioptlen );
	setsockopt( pTcp->m_sock, SOL_SOCKET, SO_OOBINLINE,
		        (char *)&foptval, ioptlen );

	// Porting Shiloh 233720.
	// Set socket option to force a "hard reset"
	// Do this only if the flag is set

	LINGER	SocketLinger;
	if(  g_dwTcpAbortiveClose )
	{
		SocketLinger.l_onoff  = TRUE;   // Turn on "lingering"
		SocketLinger.l_linger = 0;      // Set lingering time to zero (hard reset)
		setsockopt( pTcp->m_sock,
			SOL_SOCKET,
			SO_LINGER,
			(char *)&SocketLinger,
			sizeof(SocketLinger) );
	}

	BOOL bTrue = TRUE;
	
	if(SOCKET_ERROR == setsockopt( pTcp->m_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&bTrue, sizeof bTrue ))
	{
		goto ErrorExit;
	}

	// KeepAlive stuff
	DWORD  cbBytesReturned = 0;
	struct tcp_keepalive tcpStruct;

	tcpStruct.onoff             = 1;
	tcpStruct.keepalivetime     = g_dwKeepAliveTime;
	tcpStruct.keepaliveinterval = KEEPALIVEINTERVAL;

	if( WSAIoctl( pTcp->m_sock,                                               
	                  SIO_KEEPALIVE_VALS,                                  
	                  &tcpStruct,                                     
	                  sizeof(tcpStruct),                                       
	                  NULL,                                    
	                  0,                                      
	                  &cbBytesReturned,                              
	                  NULL,                           
	                  NULL ) == SOCKET_ERROR )
	{
		goto ErrorExit;
	}

	// Set the socket mode use function SetFileCompletionNotificationModes
	// instroduced in w2k3sp2, so that the I/O Manager does not set the event 
	// for the file object if a request returns with a success code, or the error
	// returned is ERROR_PENDING and the function that is called is not 
	// a synchronous function.
	//
	// See SQL BU DT 426922 for details.
	//
	if ( NULL != g_pfnWin32SetFileCompletionNotificationModes ) 
	{
    		g_pfnWin32SetFileCompletionNotificationModes((HANDLE) pTcp->m_sock, FILE_SKIP_SET_EVENT_ON_HANDLE);
	}
	
	PSOCKADDR pSockAddrLocal = NULL;
	INT iSockAddrLocal = 0;
	INT iSockAddrRemote = 0;
	SOCKADDR_STORAGE SockAddrPeer;
	PSOCKADDR pSockAddrPeer = (PSOCKADDR) &SockAddrPeer;

	//	This call can be called after AsyncAccept to get the IP address of the client
	//
	GetAcceptExSockaddrs( szAddressBuffer,
							    0,
						    sizeof(SOCKADDR_STORAGE) +16,
						    sizeof(SOCKADDR_STORAGE) +16,
						    &pSockAddrLocal,
						    &iSockAddrLocal,
						    &pSockAddrPeer,
						    &iSockAddrRemote);

	//	Make sure that structure is long enough to contain an address
	//
	Assert(sizeof(SOCKADDR) <= iSockAddrRemote);

	if( ERROR_SUCCESS != pTcp->SetPeerAddrInfo((sockaddr *) pSockAddrPeer, iSockAddrRemote) )
		goto ErrorExit;
	
	Assert(sizeof(SOCKADDR) <= iSockAddrLocal);
	if( ERROR_SUCCESS != pTcp->SetLocalAddrInfo((sockaddr *) pSockAddrLocal, iSockAddrLocal) )
		goto ErrorExit;

	// Set the provider handle to be the socket
	// 
	pTcp->m_hNwk = (HANDLE) pTcp->m_sock;

	dwError = pTcp->DWSetSkipCompletionPortOnSuccess();
	if( ERROR_SUCCESS != dwError )
	{
		BidTrace1( ERROR_TAG _T("Skip completion port on success: %d{WINERR}"), dwError);
		goto ErrorExit;
	}

	BidTraceU2( SNI_BID_TRACE_ON, SNI_TAG _T("%u#{Tcp}, ")
										  _T("m_sock: %Iu{SOCKET}\n"), 
										  pTcp->GetBidId(), 
										  pTcp->m_sock ); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("pTcp: %p{Tcp*}\n"), pTcp);
	
	return pTcp;
	
ErrorExit:

	if( pTcp )
	{
		closesocket( pTcp->m_sock);
		delete pTcp;
	}
	else
	{
		closesocket( AcceptSocket );
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("pTcp: %p{Tcp*}\n"), 0);
	
	return NULL;
}


DWORD Tcp::SetLocalAddrInfo(sockaddr * pSockAddr, int iSockAddr)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pSockAddr: %p{sockaddr*}, ")
							  _T("iSockAddr: %d\n"), 
							  GetBidId(),
							  pSockAddr, 
							  iSockAddr);
	
	DWORD dwError = ERROR_SUCCESS;
	
	m_LocalAddr.PeerAddrLen = ARRAYSIZE(m_LocalAddr.PeerAddr)-1;

	if( GetNameInfoW_l( pSockAddr, 
				    iSockAddr, 
				    m_LocalAddr.PeerAddr, 
				    m_LocalAddr.PeerAddrLen, 
				    0, 
				    0, 
				    NI_NUMERICHOST , GetDefaultLocale()) )
	{
		dwError = WSAGetLastError();

		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwError );
		
		goto ErrorExit;
	}

	m_LocalAddr.PeerAddrLen = (int) wcslen(m_LocalAddr.PeerAddr);

	if(AF_INET == pSockAddr->sa_family)
	{
		m_LocalPort = ntohs(((sockaddr_in *) pSockAddr)->sin_port);
	}

	else
	{
		m_LocalPort = ntohs(((sockaddr_in6 *) pSockAddr)->sin6_port);
	}

	BidTraceU3( SNI_BID_TRACE_ON, SNI_TAG _T("%u#, ")
										  _T("LocalAddr: %s, ")
										  _T("m_LocalPort: %d\n"), 
										  m_iBidId, 
										  m_LocalAddr.PeerAddr, 
										  m_LocalPort ); 
ErrorExit:

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}


DWORD Tcp::SetPeerAddrInfo(sockaddr * pSockAddr, int iSockAddr)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T( "pSockAddr: %p{sockaddr*}, ")
							  _T("iSockAddr: %d\n"), 
							  GetBidId(),
							  pSockAddr, 
							  iSockAddr);
	
	DWORD dwError = ERROR_SUCCESS;
	
	m_PeerAddr.PeerAddrLen = ARRAYSIZE(m_PeerAddr.PeerAddr)-1;

	if( GetNameInfoW_l( pSockAddr, 
				    iSockAddr, 
				    m_PeerAddr.PeerAddr, 
				    m_PeerAddr.PeerAddrLen, 
				    0, 
				    0, 
				    NI_NUMERICHOST , GetDefaultLocale()) )
	{
		dwError = WSAGetLastError();

		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwError );
		
		goto ErrorExit;
	}

	m_PeerAddr.PeerAddrLen = (int) wcslen(m_PeerAddr.PeerAddr);

	if(AF_INET == pSockAddr->sa_family)
	{
		m_PeerPort = ntohs(((sockaddr_in *) pSockAddr)->sin_port);
	}

	else
	{
		m_PeerPort = ntohs(((sockaddr_in6 *) pSockAddr)->sin6_port);
	}

	BidTraceU3( SNI_BID_TRACE_ON, SNI_TAG _T("%u#, ")
										  _T("PeerAddr: %s, ")
										  _T("m_PeerPort: %d\n"), 
										  m_iBidId, 
										  m_PeerAddr.PeerAddr, 
										  m_PeerPort ); 

ErrorExit:

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}

DWORD Tcp::AcceptDone( SNI_Conn * pConn,
							__inout LPVOID pAcceptKey,
							DWORD dwBytes,	//	TCP does not use a read buffer in its accept, 
											//	so we can ignore the dwBytes parameter.
							DWORD dwError,
						    __out SNI_Provider ** ppProv,
						    __out LPVOID * ppAcceptInfo)
{
	BidxScopeAutoSNI6( SNIAPI_TAG _T("pConn:%p{SNI_Conn*}, ")
							  _T("pAcceptKey:%p{AcceptObject*}, ")
							  _T("dwBytes:%d, ")
							  _T("dwError:%d, ")
							  _T("ppProv:%p{SNI_Provider**}, ")
							  _T("ppAcceptInfo:%p{LPVOID*}\n"), 
								pConn, pAcceptKey, dwBytes, dwError, ppProv, ppAcceptInfo);


	AcceptObject * pAcc = (AcceptObject *) pAcceptKey;

	*ppAcceptInfo = NULL;

	Tcp *pTcp = NULL;
	
	if( ERROR_SUCCESS == dwError )
	{
		pAcc->UpdateSockContext();
		
		pTcp = AcceptConnection( pConn, pAcc->GiveAcceptSocket(), pAcc->m_AddressBuffer);

		if( pConn && pTcp )
		{
			pConn->m_pEPInfo->tcpInfo.wszIpaddress[MAX_PATH] = L'\0';
			if(FAILED(StringCchPrintf_lW( pConn->m_pEPInfo->tcpInfo.wszIpaddress,
						 ARRAYSIZE(pConn->m_pEPInfo->tcpInfo.wszIpaddress),
						 L"%s", GetDefaultLocale(),
						 pAcc->m_pParentListenObject->m_TcpListenInfo.ipaddress)))
			{
				SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, ERROR_INVALID_PARAMETER );

				closesocket( pTcp->m_sock );
				delete pTcp;
				pTcp = NULL;
			}

			pConn->m_pEPInfo->tcpInfo.fStatic = pAcc->m_pParentListenObject->m_TcpListenInfo.fStatic;
			pConn->m_pEPInfo->tcpInfo.Port = pAcc->m_pParentListenObject->m_TcpListenInfo.Port;
		}
	}
	else
	{
		closesocket( pAcc->GiveAcceptSocket() );
	}

	DWORD dwRet;
	
	//
	// when this call fails it means, we are no longer listening on the port associated with AcceptObject
	// error will be reported inside AsyncAccept() if necessary
	//
	
	dwRet = pAcc->AsyncAccept();
	if( ERROR_SUCCESS == dwRet )
	{
		*ppProv = pTcp;

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
		
		return ERROR_SUCCESS;
	}
	else
	{
		if( pTcp )
		{
			closesocket( pTcp->m_sock );
			
			delete pTcp;
		}

		pAcc->Release();
		
		*ppProv = NULL;

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
		
		return dwRet;
	}
	
}

DWORD Tcp::Close()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	int ret = 0;

#ifdef SNI_BASED_CLIENT

	//SQL BU: 399230
	//shutdown(socket, SD_SEND) disables further send and explicitly send TCP FIN to peer 
	//to indicate connection closure. On receiving FIN, peer's overlapped pending read
	//finishes successfully (WINERR 0) with 0 byte. With SD_SEND, the shutdown does not 
	//cancel any local pending read. The cancelation is still done by closesocket as usual. 
	//shutdown does not block. closesocket sends FIN or RST to its peer, but shutdown 
	//guarantee FIN to be send. Another option is to use SD_BOTH. Only apply to client side 
	//tcp::close. 
	//
	//See more details on MSDN regarding to shutdown, closesocket and g----ful shutdown.
	//

	//This protects that overlapped pending reads complete with 
	//ERROR_OPERATION_ABORTED after tcp::close is being called.

	LONG lclosed = InterlockedCompareExchange(&m_lCloseCalled,1,0);
	Assert ( 0 == lclosed );

	ret = shutdown(m_sock, SD_SEND);

	if ( SOCKET_ERROR == ret )
	{
		//When shutdown fails, we log the error and proceed.
		ret = WSAGetLastError();
		BidTrace1(ERROR_TAG _T("winsock shutdown failed:%d{WINERR}\n"), ret );
	}

#endif

	BOOL fRet = FCloseRefHandle();
	
	if( m_pSyncReadPacket && fRet )
	{
		// Make sure that the pending read I/O was completed before
		// we release the packet and its OVERLAPPED structure.  Otherwise
		// the I/O manager may write into a released packet.  
		OVERLAPPED * pOvl = SNIPacketOverlappedStruct( m_pSyncReadPacket ); 

		// Loop checking the I/O status.  
		while( TRUE )
		{
			if ((DWORD)STATUS_PENDING == (DWORD)pOvl->Internal)
			{
				// Wait with time-out in case the auto-event was consumed
				// by another thread after we checked its status (this should
				// not really happen since the access to the Tcp object should
				// be serialized in sync mode).  We will check the status in 
				// the next iteration.  
				DWORD dwRet = WaitForSingleObject(pOvl->hEvent, CLOSEIOWAIT);

				// Unexpected error.  We leak the packet and associated connection
				// to prevent possible corruptions.  
				if( WAIT_FAILED == dwRet )
				{
					BidTrace1(ERROR_TAG _T("WaitForSingleObject failed:%d{WINERR}\n"), GetLastError() );		
					break; 
				}
				else if( WAIT_TIMEOUT != dwRet )
				{
					SNIPacketRelease(m_pSyncReadPacket);
					m_pSyncReadPacket = 0;
					break; 
				}
			}
			else
			{
				SNIPacketRelease(m_pSyncReadPacket);
				m_pSyncReadPacket = 0;
				break; 
			}
		}
	}

	// Loop checking the I/O status until it is not pending(0x103) or error other than timeout.
	// May need to cap the loop count to prevent freeze.
	//
	while( m_pOvSendNotificaiton )
	{
		if ((DWORD)STATUS_PENDING == (DWORD)m_pOvSendNotificaiton->Internal)
		{
			DWORD dwRet = WaitForSingleObject(m_pOvSendNotificaiton->hEvent, CLOSEIOWAIT);

			// Unexpected error.  We leak the overalapped struct to prevent possible corruptions.  
			//
			if( WAIT_FAILED == dwRet )
			{
				m_pOvSendNotificaiton=NULL;
				BidTrace1(ERROR_TAG _T("WaitForSingleObject failed:%d{WINERR}\n"), GetLastError() );		
				break; 
			}
			else if( WAIT_TIMEOUT != dwRet )
			{
				//Wait success and let dtor release the object.
				//
				Assert (WAIT_OBJECT_0 ==  dwRet);
				break; 
			}
		}
		else
		{
			//m_pOvSendNotification will be released in dtor.
			break;
		}
	}
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;
}

DWORD Tcp::GatherWriteAsync(__in SNI_Packet * pPacket, SNI_ProvInfo * pInfo)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("pInfo: %p{SNI_ProvInfo*}\n"), 
							  GetBidId(),
							  pPacket, 
							  pInfo);
	
	WSABUF WriteBuf[MAX_GATHERWRITE_BUFS];
	DWORD cWriteBuf;
	DWORD 	dwBytesWritten = 0;
	DWORD dwFlags = 0;
	LPWSAOVERLAPPED pOvl = SNIPacketOverlappedStruct(pPacket);
	DWORD dwError = ERROR_FAIL;
	
	SNI_Packet * pTmp = pPacket;

	for( cWriteBuf = 0; pTmp != NULL; cWriteBuf++)
	{
		if( cWriteBuf == MAX_GATHERWRITE_BUFS )
		{
			SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, ERROR_INVALID_PARAMETER);

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_INVALID_PARAMETER);
			
			dwError = ERROR_INVALID_PARAMETER;
			goto Exit;
		}
		
		// additional AddRef around WSASend call protects
		// against SNIWriteDone being called before WSASend returns
		SNIPacketAddRef(pTmp);

		SNIPacketGetData(pTmp, (BYTE **)&WriteBuf[cWriteBuf].buf, &WriteBuf[cWriteBuf].len);
		pTmp = SNIPacketGetNext(pTmp);
	}

	if( SOCKET_ERROR == WSASend( m_sock,
								  WriteBuf,
								  cWriteBuf,
								  &dwBytesWritten,
								  dwFlags,
								  pOvl,
								  NULL ) )
	{
		dwError = WSAGetLastError();
		if( WSA_IO_PENDING == dwError )
		{
			dwError = ERROR_IO_PENDING;
			goto Exit;
		}
		else
		{
			SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwError);
			goto Exit;
		}
	}
	else if (!s_fSkipCompletionPort)
	{
		// If we haven't enabled skipping the completion port, then we need to wait for it to be set
		dwError = ERROR_IO_PENDING;
		goto Exit;
	}

	dwError = ERROR_SUCCESS;
	
Exit:
	// undo extra AddRefs
	// pTmp will be last packet not AddRef'd or NULL if all AddRef'd
	SNI_Packet * pRelease = pPacket;
	while (pRelease != pTmp)
	{
		SNIPacketRelease(pRelease);
		pRelease = SNIPacketGetNext(pRelease);
	}
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	return dwError;
}                    

DWORD Tcp::GetPeerAddress(__in SNI_Conn * pConn, __out PeerAddrInfo * addrinfo)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T( "pConn: %p{SNI_Conn*}, addrinfo: %p{PeerAddrInfo*}\n"), 
					pConn, addrinfo);
	
	DWORD dwError = ERROR_SUCCESS;

	// Get Tcp object
	Tcp * pTcp = static_cast<Tcp *> (pConn->m_pProvHead);
	while(pTcp->m_pNext != NULL)
		pTcp = static_cast<Tcp *> (pTcp->m_pNext);

	*addrinfo = pTcp->m_PeerAddr;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}


DWORD Tcp::GetLocalAddress(__in SNI_Conn * pConn, __out PeerAddrInfo * addrinfo)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T( "pConn: %p{SNI_Conn*}, addrinfo: %p{PeerAddrInfo*}\n"), 
					pConn, addrinfo);
	
	DWORD dwError = ERROR_SUCCESS;

	// Get Tcp object
	Tcp * pTcp = static_cast<Tcp *> (pConn->m_pProvHead);
	while(pTcp->m_pNext != NULL)
		pTcp = static_cast<Tcp *> (pTcp->m_pNext);

	*addrinfo = pTcp->m_LocalAddr;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}

DWORD Tcp::GetLocalPort(__in SNI_Conn * pConn, __out USHORT * port)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T( "pConn: %p{SNI_Conn*}, port: %p{USHORT*}\n"), 
					pConn, port);
	
	// Get Tcp object
	Tcp * pTcp = static_cast<Tcp *> (pConn->m_pProvHead);
	while(pTcp->m_pNext != NULL)
		pTcp = static_cast<Tcp *> (pTcp->m_pNext);

	*port = pTcp->m_LocalPort;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;
}


DWORD Tcp::GetPeerPort(__in SNI_Conn * pConn, __out USHORT * port)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T( "pConn: %p{SNI_Conn*}, port: %p{USHORT*}\n"), 
					pConn, port);
	
	// Get Tcp object
	Tcp * pTcp = static_cast<Tcp *> (pConn->m_pProvHead);
	while(pTcp->m_pNext != NULL)
		pTcp = static_cast<Tcp *> (pTcp->m_pNext);

	*port = pTcp->m_PeerPort;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;
}


DWORD Tcp::GetDnsName( WCHAR *wszAddress, __out_ecount(len) WCHAR *wszDnsName, int len)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T( "wszAddress: %p{PCWSTR}, wszDnsName: %p{PCWSTR}, len: %d\n"), 
					wszAddress, wszDnsName, len);
	
	Assert( len >= NI_MAXHOST);

	if( 0 >= len )
	{
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, ERROR_INVALID_PARAMETER );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_INVALID_PARAMETER);
		
		return ERROR_INVALID_PARAMETER;
	}

	DWORD dwRet;
	
	ADDRINFOW *AddrInfoW;
	ADDRINFOW Hints;
	
	memset(&Hints, 0, sizeof(Hints));
	Hints.ai_family = PF_UNSPEC;
	Hints.ai_flags = AI_CANONNAME;

	if(IsNumericAddress(wszAddress))
	{
		Hints.ai_flags |=  AI_NUMERICHOST;	
	}

	if( GetAddrInfoW_l( wszAddress, NULL, &Hints, &AddrInfoW, GetDefaultLocale()))
	{
		dwRet = WSAGetLastError();

		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
		
		return dwRet;
	}
	
	if( AddrInfoW->ai_canonname && !IsNumericAddress(AddrInfoW->ai_canonname) )
	{
		if( len > (int)wcslen(AddrInfoW->ai_canonname) )
		{
			(void) StringCchCopyW ( wszDnsName, len, AddrInfoW->ai_canonname );

			FreeAddrInfoW( AddrInfoW );

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
			
			return ERROR_SUCCESS;
		}
		else
		{
			// if this happens, it is an internal error because user of this function should make sure that it gives
			// a buffer of sufficient size
			
			Assert( 0 );
			
			FreeAddrInfoW( AddrInfoW );

			SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, ERROR_INSUFFICIENT_BUFFER );

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_INSUFFICIENT_BUFFER);
			
			return ERROR_INSUFFICIENT_BUFFER;
		}
	}

	Assert( AddrInfoW );

	//
	// go through address list to do a reverse lookup
	//
	
	for( ADDRINFOW *AIW = AddrInfoW; AIW !=0 ; AIW = AIW->ai_next )
	{
		if( !GetNameInfoW_l(
				AIW->ai_addr, 
				(socklen_t)AIW->ai_addrlen, 
				wszDnsName,
				len, 
				NULL, 
				0,
				NI_NAMEREQD, GetDefaultLocale()))
		{
			FreeAddrInfoW( AddrInfoW );

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
			
			return ERROR_SUCCESS;
		}
	}

	FreeAddrInfoW( AddrInfoW );

	dwRet = WSAGetLastError();

	Assert( ERROR_SUCCESS != dwRet );
	
	SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

// Determines if pszServer is a loopback.
BOOL Tcp::FIsLoopBack(const WCHAR* pwszServer)
{
	ADDRINFOW *pAIWServer     = NULL;
	ADDRINFOW *pAIWLoopBackV4 = NULL;
	ADDRINFOW *pAIWLoopBackV6 = NULL;
	ADDRINFOW *pAIWS, Hints;
	BOOL fIsLoopback = FALSE;
	
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "pwszServer: \"%s\"\n"), pwszServer );

	Assert( pwszServer );

	ZeroMemory( &Hints, sizeof(Hints) );
	Hints.ai_family   = PF_UNSPEC;
	Hints.ai_socktype = SOCK_STREAM;
	Hints.ai_flags    = AI_PASSIVE;

	// Get addressinfo for server.  If this fails we cannot continue.
	if ( GetAddrInfoW_l(pwszServer, L"0", &Hints, &pAIWServer, GetDefaultLocale()) ) 
	{
		DWORD dwRet = WSAGetLastError();
		if ( EAI_NONAME == dwRet )
		{
			//For numeric name in form of three-part address, e.g. 127.0.1 or two-part address, e.g. 127.1, retry getaddrinfo with AI_NUMERICHOST 
			// as hint.ai_flags;			
			Hints.ai_flags |=  AI_NUMERICHOST;			
			if( GetAddrInfoW_l( pwszServer,L"0", &Hints, &pAIWServer, GetDefaultLocale()))				
			{
				goto FIsLoopBackExit;
			}
		}
		else
		{
			goto FIsLoopBackExit;
		}
	}

	// Set ai_flags to AI_NUMERICHOST, meaning do NOT perform DNS lookup, 
	// just translate input string to appropriate addrinfo structure.
	Hints.ai_flags  = AI_NUMERICHOST;

	// Get addressinfo for loopback IPv4, this can fail if system does not support IPv4.
	Hints.ai_family = PF_INET;
	GetAddrInfoW_l(L"127.0.0.1", L"0", &Hints, &pAIWLoopBackV4, GetDefaultLocale());

	// Get addressinfo for loopback IPv6, this can fail if system does not support IPv6.
	Hints.ai_family = PF_INET6;
	GetAddrInfoW_l(L"::1", L"0", &Hints, &pAIWLoopBackV6, GetDefaultLocale());

	// Skip out if we fail to allocate both of these.
	if ( (NULL==pAIWLoopBackV4) && (NULL==pAIWLoopBackV6) ) goto FIsLoopBackExit;

	// Compare server addressinfo against loopback addressinfo (both IPv4 and IPv6).
	for ( pAIWS=pAIWServer; pAIWS; pAIWS=pAIWS->ai_next  )
	{
		if ( pAIWLoopBackV4 &&
			 ( AF_INET == pAIWS->ai_family ) &&
			 ( pAIWS->ai_addrlen == pAIWLoopBackV4->ai_addrlen ) &&
			 ( 0 == memcmp(pAIWS->ai_addr, pAIWLoopBackV4->ai_addr, pAIWS->ai_addrlen) ) )
		{
			fIsLoopback = TRUE;
			break;
		}

		if ( pAIWLoopBackV6 &&  
			( AF_INET6 == pAIWS->ai_family ) &&
			( pAIWS->ai_addrlen == pAIWLoopBackV6->ai_addrlen ) && 
			( 0 == memcmp(pAIWS->ai_addr, pAIWLoopBackV6->ai_addr, pAIWS->ai_addrlen) ) )
		{
			fIsLoopback = TRUE;
			break;
		}
	}

FIsLoopBackExit:

	if ( pAIWServer )      FreeAddrInfoW( pAIWServer );
	if ( pAIWLoopBackV4 )  FreeAddrInfoW( pAIWLoopBackV4 );
	if ( pAIWLoopBackV6 )  FreeAddrInfoW( pAIWLoopBackV6 );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), fIsLoopback);
	return fIsLoopback;

}

DWORD Tcp::Initialize(PSNI_PROVIDER_INFO pInfo)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("pInfo: %p{PSNI_PROVIDER_INFO}\n"), pInfo );

	DWORD dwError = ERROR_SUCCESS;
	
	// Initialize Winsock - we do this here, since its required for Tcp connections
	// as well as SSRP stuff
	WSADATA       wsadata;

	// Startup, Version 2.2
	if( WSAStartup((WORD)0x0202, &wsadata) )
	{
		// Failed to start up Version 2.2, let's try starting up Version 1.0
		//Bug: 291912
		//On WSAStartup failure, we can not use the error code returned by WSAGetLastError() since winsock context is not avaialbe yet.
		// In this case, we use return value of WSAStartup as the error code when everthing fails.
		if( dwError = WSAStartup((WORD)0x0101, &wsadata) )
		{
			// Everything failed			
			Assert( 0 && " Winsock library initialization failed\n" );
			BidTrace0( ERROR_TAG _T("Winsock library initialization failed\n") );
			SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwError); 
			
			goto ErrorExit;
		}	
	}

	dwError = ShouldEnableSkipIOCompletion(&s_fSkipCompletionPort);
	if ( dwError != ERROR_SUCCESS )
	{
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwError );

		goto ErrorExit;
	}
	BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("Should enable 'Skip IO completion port on success': %d{bool}\n"), s_fSkipCompletionPort );


#ifndef SNI_BASED_CLIENT
	// Load SetFileCompletionNotificationModes from kernel32.dll if it is availble 
	// (w2k3sp2 and later). We can set the a socket mode with that function 
	// so that the I/O Manager does not set the event for the file object if a request 
	// returns with a success code, or the error returned is ERROR_PENDING and 
	// the function that is called is not a synchronous function.
	//
	// See SQL BU DT 426922 for details.
	//
	g_hKernel32 = SNILoadSystemLibraryA("kernel32.dll");

	if( NULL != g_hKernel32 )
	{
		g_pfnWin32SetFileCompletionNotificationModes 
			= (PFN_WIN32_SETFILECOMPLETIONNOTIFICATIONMODES)
			GetProcAddress(g_hKernel32, "SetFileCompletionNotificationModes");

		if( NULL == g_pfnWin32SetFileCompletionNotificationModes )
		{
			FreeLibrary( g_hKernel32 );
			g_hKernel32 = NULL;
		}
	}

#endif


	// Fill all these up
	pInfo->fBaseProv = TRUE;
	pInfo->Size = 0;
	pInfo->Offset = 0;
	pInfo->ProvNum = TCP_PROV;
	pInfo->fInitialized = TRUE; 

ErrorExit:
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}

DWORD Tcp::InitializeListener(HANDLE hSNIListener, __inout TcpListenInfo *pListenInfo, __out HANDLE * pListenHandle)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("hSNIListener: %p{HANDLE}, ")
							  _T("pListenInfo: %p{TcpListenInfo*}, ")
							  _T("pListenHandle: %p{HANDLE*}\n"), 
					hSNIListener, pListenInfo, pListenHandle);
	
	DWORD dwRet = ERROR_SUCCESS;
	
	ListenObject *pListenObject = NewNoX(gpmo) ListenObject;

	if( !pListenObject )
	{
		dwRet = ERROR_OUTOFMEMORY;

		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );

		goto ErrorExit;
	}

	// Allocate an IP address string
	pListenObject->m_TcpListenInfo.ipaddress = NewNoX(gpmo) WCHAR[MAX_PATH+1];

	if( !pListenObject->m_TcpListenInfo.ipaddress )
	{
		dwRet = ERROR_OUTOFMEMORY;

		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );

		goto ErrorExit;
	}

	g_fIpv6Supported = IsIpVersionSupported( AF_INET6 );
	g_fIpv4Supported = IsIpVersionSupported( AF_INET );
	
	//listen on the ports given to us
	
	dwRet = InterfaceListen( pListenInfo, pListenObject);

	if( ERROR_SUCCESS != dwRet )
	{
		goto ErrorExit;
	}

	// Copy the Tcp info locally
	pListenObject->m_TcpListenInfo.ipaddress[MAX_PATH] = L'\0';
	if(FAILED(StringCchPrintf_lW(pListenObject->m_TcpListenInfo.ipaddress,
				 MAX_PATH+1,
				 L"%s", GetDefaultLocale(),
				 pListenInfo->ipaddress ? pListenInfo->ipaddress : L"0")) )
	{
		dwRet = ERROR_INVALID_PARAMETER;
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );
		goto ErrorExit;
	}

	pListenObject->m_TcpListenInfo.fStatic = pListenInfo->fStatic;
	pListenObject->m_TcpListenInfo.fUpdated = pListenInfo->fUpdated;
	pListenObject->m_TcpListenInfo.Port = pListenInfo->Port;

	dwRet = pListenObject->StartAccepting( hSNIListener );
	if( ERROR_SUCCESS != dwRet )
	{
		goto ErrorExit;
	}

	*pListenHandle = (HANDLE) pListenObject;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;
	
ErrorExit:		
	if(pListenObject)
		pListenObject->TerminateListener();

	*pListenHandle = NULL;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

bool Tcp::IsNumericAddress( LPWSTR name)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "name: \"%s\"\n"), name);
	
	SOCKADDR_STORAGE addr;

	int len = sizeof addr;
	addr.ss_family = AF_INET;
	
	WIN2K3_DEPENDENCY("Use RtlIpv4StringToAddress and RtlIpv6StringToAddress instead of WSAStringToAddress, so we don't rely on initialization of OS support for IPv4/IPv6")
	if( !WSAStringToAddress( name,
						AF_INET,
						0,
						(LPSOCKADDR)&addr,
						&len))
	{
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{bool}\n"), true);
		return true;
	}

	len = sizeof addr;
	addr.ss_family = AF_INET6;
	if( !WSAStringToAddress( name,
						AF_INET6,
						0,
						(LPSOCKADDR)&addr,
						&len))
	{
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{bool}\n"), true);
		return true;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{bool}\n"), false);
	return false;
	
}

// Using ConnectEx, create a connected socket based on the specified linked list of ADDRINFO, by opening sockets in parallel.
DWORD Tcp::SocketOpenParallel(const ADDRINFOW *AIW, DWORD timeout)
{
	BidxScopeAutoSNI4( SNIAPI_TAG _T("%u#, ")
								  _T("AIW: %p{ADDRINFOW*}, ")
								  _T("ai_family: %d, ")
								  _T("timeout: %d\n"),
								  m_iBidId, AIW, AIW->ai_family, timeout);
	DWORD dwRet = ERROR_SUCCESS;
	Assert(NULL != AIW);
	
	DWORD dwAddresses = 0;
	DWORD dwStart = GetTickCount();
	DWORD timeleft = timeout;
	DWORD dwConnectionsPending = 0;
	TcpConnection *pTcpConnections = NULL;
	TcpConnection **ppPendingTcpConnections = NULL;
	HANDLE *rgConnectionEvents = NULL;
	
	// walk the struct and find out how many addresses it has.
	const ADDRINFOW *pAIWTemp = AIW;
	for(; NULL != pAIWTemp; pAIWTemp = pAIWTemp->ai_next)
	{
		dwAddresses++;
		
		// WaitForMultipleObjects can only handle up to 64 handles, and 64 target addresses is way more than we think
		// we need, so artificially cap at 64 to avoid the massive extra effort to support an arbitrary number of target addresses.
		if( dwAddresses > 64 )
		{
			dwRet = ERROR_FAIL;
			SNI_SET_LAST_ERROR(TCP_PROV, SNIE_47, dwRet);
			goto Exit;
		}
	}
	
	pTcpConnections = NewNoX(gpmo) TcpConnection[dwAddresses];
	if( NULL == pTcpConnections )
	{
		dwRet = ERROR_OUTOFMEMORY;
		SNI_SET_LAST_ERROR(TCP_PROV, SNIE_10, dwRet);
		goto Exit;
	}
	
	// An array of pointers into locations in pTcpConnections - holds only pointers to connections with a pending Overlapped IO.
	ppPendingTcpConnections = NewNoX(gpmo) TcpConnection*[dwAddresses];
	if( NULL == ppPendingTcpConnections )
	{
		dwRet = ERROR_OUTOFMEMORY;
		SNI_SET_LAST_ERROR(TCP_PROV, SNIE_10, dwRet);
		goto Exit;
	}
	
	rgConnectionEvents = NewNoX(gpmo) HANDLE[dwAddresses];
	if( NULL == rgConnectionEvents )
	{
		dwRet = ERROR_OUTOFMEMORY;
		SNI_SET_LAST_ERROR(TCP_PROV, SNIE_10, dwRet);
		goto Exit;
	}
	
	pAIWTemp = AIW;
	for(DWORD i=0;i<dwAddresses;i++)
	{
		Assert(NULL != pAIWTemp);

		// For each of these TcpConnection API calls, we have nothing to do in case of actual failure, except 
		// to move on to the next address. If all the parallel connection attempts eventually fail, the 
		// error code from wherever the failure occurred will be considered in calculating the overall return code,
		// but that consideration will be done by the TcpConnection objects themselves, not by this function.
		
		if( ERROR_SUCCESS == pTcpConnections[i].FInit(this, pAIWTemp) )
		{
			if( ERROR_SUCCESS == pTcpConnections[i].FInitForAsync() )
			{
				DWORD dwAsyncOpenError = pTcpConnections[i].AsyncOpen();
				if( ERROR_SUCCESS == dwAsyncOpenError )
				{
					// Connection succeeded. Retrieve the connected socket and exit.
					m_sock = pTcpConnections[i].RelinquishSocket();
					goto Exit;
				}
				else if( ERROR_IO_PENDING == dwAsyncOpenError )
				{
					// Overlapped was pending. Add the object and its Event handle to our pending lists and continue on to the next target address.
					ppPendingTcpConnections[dwConnectionsPending] = &(pTcpConnections[i]);
					rgConnectionEvents[dwConnectionsPending] = (pTcpConnections[i]).GetEventForOutstandingOverlappedIO();
					dwConnectionsPending++;
				}
			}
		}
		pAIWTemp = pAIWTemp->ai_next;
	}

	while( 0 < dwConnectionsPending )
	{		
		// Wait for *any* of the pending Overlapped IOs' event handles to complete
		dwRet = WaitForMultipleObjects(dwConnectionsPending, rgConnectionEvents, FALSE /*bWaitAll*/, timeleft );
		
		// if this C_ASSERT were false, the if condition below it would also need to check that WAIT_OBJECT_0 <= dwRet. Since the C_ASSERT is true,
		// that additional check would result in a compiler warning that the expression is always true...
		C_ASSERT(WAIT_OBJECT_0 == 0);
		
		if( dwRet < (WAIT_OBJECT_0 + dwConnectionsPending) )
		{
			// One IO completed - calculate which one it was.
			DWORD dwAffectedConnection = dwRet - WAIT_OBJECT_0;
			TcpConnection *pAffectedConnection = ppPendingTcpConnections[dwAffectedConnection];
			
			// This connection is no longer pending, so remove it from our pending lists
			for(DWORD i=dwAffectedConnection;(i + 1) < dwConnectionsPending; i++)
			{
				ppPendingTcpConnections[i] = ppPendingTcpConnections[i+1];
				rgConnectionEvents[i] = rgConnectionEvents[i+1];
			}
			dwConnectionsPending--;
			
			// Notify the connection that its outstanding Overlapped IO is done.
			pAffectedConnection->NotifyAboutOverlappedIOCompletion();
			
			// Check whether it completed with success or error.
			if( ERROR_SUCCESS == pAffectedConnection->CheckCompletedAsyncConnect() )
			{
				// Connection succeeded. Retrieve the connected socket and exit.
				m_sock = pAffectedConnection->RelinquishSocket();
				dwRet = ERROR_SUCCESS;
				goto Exit;
			}
			else
			{
				// in case of completion with an error, there's nothing to do - we already removed it from the pending list, so keep looping as long as we have more
				// The specific error code will be taken into account later, when the error code for the whole parallel connection attempt is computed.
			}
		}
		else if( WAIT_FAILED == dwRet )
		{
			dwRet = GetLastError();
			dwRet = TcpConnection::CalculateReturnCode(pTcpConnections, dwAddresses, dwRet, TcpConnectionErrorLevel_WaitForObjects);
			goto Exit;
		}
		else
		{
			// either a timeout, or an unexpected return code from WaitForMultipleObjects. Either way, use the return code Windows gave us.
			dwRet = TcpConnection::CalculateReturnCode(pTcpConnections, dwAddresses, dwRet, 
				(WAIT_TIMEOUT == dwRet) ? TcpConnectionErrorLevel_WaitTimeout : TcpConnectionErrorLevel_WaitForObjects);
			goto Exit;
		}

		// Only recompute timeout after each iteration, to ensure we always do at least one iteration, to make a best
		// effort at giving the connection a chance to return WSAECONNREFUSED.
		if( timeout != INFINITE )
		{
			timeleft = ComputeNewTimeout(timeout,dwStart);
			if( 0 == timeleft )
			{
				dwRet = TcpConnection::CalculateReturnCode(pTcpConnections, dwAddresses, WAIT_TIMEOUT, TcpConnectionErrorLevel_WaitTimeout);
				goto Exit;
			}
		}

	}
	
	// All error conditions and the Success case go straight to Exit. So, if we got here, we must have waited on all the connections
	Assert( dwConnectionsPending == 0 );
	// Moreover, all the connections must have failed.
	Assert( INVALID_SOCKET == m_sock );
	
	// In this case, we need to compute the error code over all the TcpConnections we attempted (in any other case, we already computed an error code to return).
	// This should never actually use the ERROR_FAIL we provide, since every TcpConnection object should have an error level higher than TcpConnectionErrorLevel_None
	dwRet = TcpConnection::CalculateReturnCode(pTcpConnections, dwAddresses, ERROR_FAIL, TcpConnectionErrorLevel_None);
	
Exit:
	// So that the oustanding Overlapped IOs from each TcpConnection class don't have to be cancelled and waited on individually, we:
	// 1) Close each of the TcpConnections individually
	// 2) Wait on all of the outstanding Overlapped IO Events together with one WaitForMultipleObjects call
	// 3) Individually notify each of the TcpConnections that the Overlapped IO is done, when WFMO succeeds.
	if( 0 < dwConnectionsPending )
	{
		// First, close all the connections which still have pending IOs, and retrieve Event handles for all of them.
		for(DWORD i=0;i<dwConnectionsPending;i++)
		{
			ppPendingTcpConnections[i]->CloseOutstandingSocket();
			rgConnectionEvents[i] = ppPendingTcpConnections[i]->GetEventForOutstandingOverlappedIO();
			Assert( NULL != rgConnectionEvents[i] );
		}
		
		DWORD dwWFMOReturn = WaitForMultipleObjects(dwConnectionsPending, rgConnectionEvents, TRUE /*bWaitAll*/, INFINITE );
		
		// See identical C_ASSERT above for reasoning...
		C_ASSERT(WAIT_OBJECT_0 == 0);
		
		if( dwWFMOReturn < (WAIT_OBJECT_0 + dwConnectionsPending) )
		{
			// Successfully waited on all events - now notify each previously-pending TcpConnection that its Overlapped IO is done.
			for(DWORD i=0;i<dwConnectionsPending;i++)
			{
				ppPendingTcpConnections[i]->NotifyAboutOverlappedIOCompletion();
			}
		}
		else
		{
			// for any failure, log it with as much detail as is available. 
			if( WAIT_FAILED == dwWFMOReturn )
			{
				dwWFMOReturn = GetLastError();
				BidTrace1( ERROR_TAG _T("WaitForMultipleObjects() extended error: %d{WINERR}"), dwWFMOReturn);
			}
			else
			{
				BidTrace1( ERROR_TAG _T("WaitForMultipleObjects(): %d{WINERR}"), dwWFMOReturn);
			}
			// Since our Wait is failing, we may later need to leak the Overlapped structs for the remaining outstanding 
			// Overlapped IOs. Rather than doing that now, let each TcpConnection attempt to individually clean up the 
			// Overlapped structs in their own destructors, in case only one of the bunch had a problem and the rest can 
			// be safely cleaned up.
		}
	}
	
	if( NULL != pTcpConnections )
	{
		delete []pTcpConnections;
		pTcpConnections = NULL;
	}
	if( NULL != ppPendingTcpConnections )
	{
		delete []ppPendingTcpConnections;
		ppPendingTcpConnections = NULL;
	}
	if( NULL != rgConnectionEvents )
	{
		delete []rgConnectionEvents;
		rgConnectionEvents = NULL;
	}
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;
}

// Create connected socket based on AI.
// 1. use connectex if connectex exists and timeout is not infinite;
// 2. use connect if timeout value is inifinite.
//
DWORD Tcp::SocketOpenSync(__in ADDRINFOW* AIW, int timeout)
{

	BidxScopeAutoSNI3( SNIAPI_TAG _T("AIW: %p{ADDRINFOW*}, ")
								  _T("ai_family: %d, ")
								  _T("timeout: %d\n"),
								  AIW, AIW->ai_family, timeout);
	
	DWORD dwRet = ERROR_SUCCESS;

	Assert( NULL != AIW );
	Assert( INVALID_SOCKET == m_sock );

	TcpConnection tcpConnection;
	dwRet = tcpConnection.FInit(this, AIW);
	if( ERROR_SUCCESS != dwRet )
	{
		goto Exit;
	}
	
	if( INFINITE != timeout )
	{
		dwRet = tcpConnection.FInitForAsync();
		if( ERROR_SUCCESS != dwRet )
		{
			goto Exit;
		}
		
		dwRet = tcpConnection.AsyncOpen();
		if( ERROR_SUCCESS != dwRet && ERROR_IO_PENDING != dwRet )
		{
			goto Exit;
		}
		else if( ERROR_IO_PENDING == dwRet )
		{
			dwRet = tcpConnection.WaitForCompletion(timeout);
		}
		// fall through for error handling
	}
	else
	{
		dwRet = tcpConnection.SyncOpen();
		// fall through for error handling
	}

	if( ERROR_SUCCESS == dwRet )
	{
		m_sock = tcpConnection.RelinquishSocket();
	}
Exit:

	if( ERROR_SUCCESS != dwRet )
	{
		SNI_SET_LAST_ERROR(TCP_PROV, SNIE_SYSTEM, dwRet);
	}    

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;

}

inline DWORD Tcp::ComputeNewTimeout(DWORD timeout, DWORD dwStart)
{
	DWORD time = GetTickCount();
	DWORD diff;
	// GetTickCount rolls around after ~49.7 days of system uptime. 
	// It's fairly safe to assume this function won't be called with a dwStart value which was more than 49.7 days ago,
	// but it's not very safe to assume that our connection's start time wasn't soon before that 49.7-day rollover period and
	// that the current GetTickCount has just rolled over.
	if( time < dwStart ) 
	{
		diff = (DWORD_MAX - dwStart) + time + 1;
	}
	else
	{
		diff = time - dwStart;
	}
	
	if( timeout < diff )
	{
		return 0;
	}
	else
	{
		// If we have any time left on the clock, make sure it's at least the minimum.
		DWORD dwTimeLeft = timeout - diff;
		return ((dwTimeLeft < MIN_PARALLEL_WAIT_TIME) ? MIN_PARALLEL_WAIT_TIME : dwTimeLeft);
	}
}

DWORD Tcp::ParallelOpen(__in ADDRINFOW *AddrInfoW, int timeout, DWORD dwStartTickCount)
{
	int timeleft = timeout;

	if (INFINITE != timeout)
	{
		// Previous layers in SNI may have taken some of the quantum, potentially dropping below 0.
		// If that happens here, set the initial value to 0. 
		// 
		// DEVNOTE: After this point, in this code path, timeout and timeleft are assumed to be a non-negative numbers.
		//
		if (timeout < 0)
			timeout = 0;

		timeleft = ComputeNewTimeout(timeout, dwStartTickCount);
		// If we have timed out before we even initiated the TCP connections, ignore the timeout for now
		// and set the remaining time to the minimum value.
		if (0 == timeleft)
		{
			timeleft = MIN_PARALLEL_WAIT_TIME;
		}
		BidTraceU1(SNI_BID_TRACE_ON, SNI_TAG _T("timeout remaining: %d\n"), timeleft);
	}

	DWORD dwRet = SocketOpenParallel(AddrInfoW, timeleft);
	// Should have a valid socket handle IFF SocketOpenParallel returned success.
	Assert((ERROR_SUCCESS == dwRet && m_sock != INVALID_SOCKET) || (ERROR_SUCCESS != dwRet && m_sock == INVALID_SOCKET));
	if (dwRet != ERROR_SUCCESS)
	{
		return dwRet;
	}
	// trace the remaining timeout (no need to adjust again after here, since we don't do anymore activities which are expected to consume a large amount of time.
	if (INFINITE != timeout && SNI_BID_TRACE_ON)
	{
		timeleft = ComputeNewTimeout(timeout, dwStartTickCount);
		BidTrace1(SNI_TAG _T("timeout remaining after successful connection: %d\n"), timeleft);
	}
	return dwRet;
}

DWORD Tcp::Open( 	SNI_Conn 		* pConn,
					ProtElem 		* pProtElem, 
					__out SNI_Provider 	** ppProv,
					int timeout)
{
	BidxScopeAutoSNI4( SNIAPI_TAG _T( "pConn: %p{SNI_Conn*}, ")
								_T("pProtElem: %p{ProtElem}, ")
								_T("ppProv: %p{SNI_Provider**}, ")
								_T("timeout: %d\n"), 
								pConn, pProtElem, ppProv,timeout);
	
	DWORD dwRet;
	Tcp * pTcpProv = 0;
	HANDLE hConnectEvent = INVALID_HANDLE_VALUE;

	DWORD dwStart = GetTickCount();
	int timeleft = timeout;

	pTcpProv = NewNoX(gpmo) Tcp(pConn);
	if( !pTcpProv )
	{
		dwRet = ERROR_OUTOFMEMORY;

		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_4, dwRet );

		goto ErrorExit;
	}

	dwRet = pTcpProv->FInit();
	if( ERROR_SUCCESS != dwRet )
	{		
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_4, dwRet );

		goto ErrorExit;
	}

	ADDRINFOW *AddrInfoW = NULL;
	ADDRINFOW Hints;

	memset(&Hints, 0, sizeof(Hints));
	Hints.ai_family = PF_UNSPEC;
	Hints.ai_socktype = SOCK_STREAM;

	if( GetAddrInfoW_l( pProtElem->m_wszServerName, pProtElem->Tcp.wszPort, &Hints, &AddrInfoW, GetDefaultLocale()))
	{
		dwRet = WSAGetLastError();
		if ( EAI_NONAME == dwRet )
		{
			//For numeric name in form of three-part address, e.g. 127.0.1 or two-part address, e.g. 127.1, retry getaddrinfo with AI_NUMERICHOST 
			// as hint.ai_flags;			
			Hints.ai_flags |=  AI_NUMERICHOST;			
			if( GetAddrInfoW_l( pProtElem->m_wszServerName, pProtElem->Tcp.wszPort, &Hints, &AddrInfoW, GetDefaultLocale()))				
			{
				dwRet = WSAGetLastError();			
				SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );
				goto ErrorExit;
			}
		}
		else
		{
			SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );
			goto ErrorExit;
		}
	}

	dwRet = ERROR_SUCCESS;
	// When TransparentNetworkResolution:
	// 1. Try first IP addr with 500ms timeout, if the server has more than 64 IP addrs, using sequential connection and original timeout 
	//     store in pProtElem->Tcp.totalTimeout
	// 2. Try parallel connection if first step failed
	// 
	bool fAddrInfoCountGreaterThan64 = false;
	if (!pProtElem->Tcp.fParallel && (pProtElem->Tcp.transparentNetworkIPResolution == TransparentNetworkResolutionMode::SequentialMode || pProtElem->Tcp.transparentNetworkIPResolution == TransparentNetworkResolutionMode::ParallelMode))
	{
		fAddrInfoCountGreaterThan64 = (GetAddrCount(AddrInfoW) > 64);
	}

    if (pProtElem->Tcp.fParallel || (pProtElem->Tcp.transparentNetworkIPResolution == TransparentNetworkResolutionMode::ParallelMode && !fAddrInfoCountGreaterThan64))
	{
		Assert(!fAddrInfoCountGreaterThan64);
		dwRet = pTcpProv->ParallelOpen(AddrInfoW, timeout, dwStart);
		if (dwRet != ERROR_SUCCESS)
		{
			goto ErrorExit;
		}
	}
	else
	{
		//First we try ipv4 addresses, so there won't be a delay while connecting
		//to a old servers which don't listen on ipv6 addresses
		ADDRINFOW *AIW;
		int afs[] = {AF_INET, AF_INET6};
		
		// If fTransparentNetworkIPResolution and the address has more than 64 IP, open with sequential mode and reset timeout
		// with the original timeout stored in SNI_CLIENT_CONSUMER_INFO
		if (pProtElem->Tcp.transparentNetworkIPResolution == TransparentNetworkResolutionMode::SequentialMode && fAddrInfoCountGreaterThan64)
		{
			timeout = pProtElem->Tcp.totalTimeout;
		}

		for( int i = 0; i < sizeof(afs)/sizeof(int); i++ )
		{
			for( AIW = AddrInfoW; AIW != 0; AIW = AIW->ai_next )
			{
				if( AIW->ai_family != afs[i] )
					continue;

				// Adjust the timeout to take acccount the time spent thus far, including DNS and failed SocketOpenSync calls.
				if( INFINITE != timeout )
				{
					timeleft = timeout - (DWORD)(GetTickCount() - dwStart);
					BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("timeout: %d\n"), timeleft );
					// If we have timed out, exit
					if( timeleft <= 0 || timeleft > timeout )
					{
						//Use the last error code returned by SocketOpenSync.
						//
						if( ERROR_SUCCESS == dwRet )
						{
							dwRet = WSA_WAIT_TIMEOUT;
						}
						break;
					}
				}
				
				//DEVNOTE: do not check timeout if this call succeeded with the specified timeout value.
				//Assumption is that there is no call that can take long after here and before SNIOpenSyncEx returns.
				//TDS will cap the total timeout with necessary tolerance to decide whether this connection should 
				//succeed.
				//
				if( ERROR_SUCCESS == (dwRet = pTcpProv->SocketOpenSync(AIW, timeleft )))
				{
					// Adjust the timeout to take acccount the time spent thus far, including DNS and all SocketOpenSync calls so far.
					// Used for bidtrace the timeleft only. 
					//
					if( INFINITE != timeout )
					{
						timeleft = timeout - (DWORD)(GetTickCount() - dwStart);
						BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("timeout: %d\n"), timeleft );
					}

					break;			
				}
				
                // When transparentNetworkIPResolution is Sequential, only try the first IP address
                //
				if (pProtElem->Tcp.transparentNetworkIPResolution == TransparentNetworkResolutionMode::SequentialMode && !fAddrInfoCountGreaterThan64)

                {
                    break;
                }
			}

			//if AIW is 0, it means we couldn't connect yet
			if( AIW == 0 )
				AIW = AddrInfoW;
			else
				break;	//we are connected, skip loop
		}
	}
	
	FreeAddrInfoW( AddrInfoW );
	AddrInfoW = NULL;

	if( pTcpProv->m_sock == INVALID_SOCKET )
	{
		if( dwRet == ERROR_SUCCESS )
		{
			dwRet = ERROR_FAIL;

			SNI_SET_LAST_ERROR( TCP_PROV, SNIE_17, dwRet );
		}
		else
		{
			SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );
		}
		
		goto ErrorExit;
	}
	
	BOOL bTrue = TRUE;
	
	if(SOCKET_ERROR == setsockopt( pTcpProv->m_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&bTrue, sizeof bTrue ))
	{
		dwRet = WSAGetLastError();

		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );

		goto ErrorExit;
	}

	if( !gfIsWin9x )
		dwRet = pTcpProv->SetKeepAliveOption();

	if( ERROR_SUCCESS != dwRet )
	{
		goto ErrorExit;
	}

	// Get Peer address
	SOCKADDR_STORAGE PeerAddr;
	int addrlen = sizeof(SOCKADDR_STORAGE);
	if( getpeername( pTcpProv->m_sock, (sockaddr *) &PeerAddr, &addrlen) )
	{
		dwRet = WSAGetLastError();
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );
		
		goto ErrorExit;
	}

	if(ERROR_SUCCESS != (dwRet = pTcpProv->SetPeerAddrInfo((sockaddr *) &PeerAddr, addrlen)) )
	{
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );
		
		goto ErrorExit;
	}

	// Get Local address
	SOCKADDR_STORAGE LocalAddr;
	addrlen = sizeof(SOCKADDR_STORAGE);
	if( getsockname( pTcpProv->m_sock, (sockaddr *) &LocalAddr, &addrlen) )
	{
		dwRet = WSAGetLastError();
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );
		
		goto ErrorExit;
	}

	if(ERROR_SUCCESS != (dwRet = pTcpProv->SetLocalAddrInfo((sockaddr *) &LocalAddr, addrlen)) )
	{
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );
		
		goto ErrorExit;
	}	

	// Set the handle in the base provider class to point to the socket
	pTcpProv->m_hNwk = (HANDLE) pTcpProv->m_sock;

	if( ERROR_SUCCESS != (dwRet = pTcpProv->DWSetSkipCompletionPortOnSuccess()) )
	{
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );
		goto ErrorExit;
	}

#ifndef SNI_BASED_CLIENT
	Assert(pConn->m_pEPInfo);
	
	pConn->m_pEPInfo->tcpInfo.wszIpaddress[MAX_PATH] = L'\0';
	
	if (FAILED(StringCchPrintf_lW( pConn->m_pEPInfo->tcpInfo.wszIpaddress,
				 wcslen(pConn->m_pEPInfo->tcpInfo.wszIpaddress),
				 L"%.*s", GetDefaultLocale(),
				 pTcpProv->m_LocalAddr.PeerAddrLen, 
				 pTcpProv->m_LocalAddr.PeerAddr)))
	{
		dwRet = ERROR_INVALID_PARAMETER;

		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, ERROR_INVALID_PARAMETER );
	
		goto ErrorExit;
	}

	pConn->m_pEPInfo->Provider = TCP_PROV;

	pConn->m_pEPInfo->tcpInfo.Port = pTcpProv->m_PeerPort;

	pConn->m_pEPInfo->tcpInfo.fStatic = FALSE;
#endif	

	BidUpdateItemIDA( pTcpProv->GetBidIdPtr(), SNI_ID_TAG 
		"connection: %p{ProtElem}", pProtElem );  

	BidTraceU2( SNI_BID_TRACE_ON, SNI_TAG _T("%u#{Tcp}, ")
										  _T("m_sock: %Iu{SOCKET}\n"), 
										  pTcpProv->GetBidId(), 
										  pTcpProv->m_sock ); 


	if( INVALID_HANDLE_VALUE != hConnectEvent )
	{
		CloseHandle( hConnectEvent ); 		
		hConnectEvent = INVALID_HANDLE_VALUE;		
	}
	
	// Set the out provider param to point to the new Tcp object
	*ppProv = pTcpProv;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;
	
ErrorExit:

	if( NULL != AddrInfoW )
	{
		FreeAddrInfoW( AddrInfoW );
		AddrInfoW = NULL;
	}

	if( INVALID_HANDLE_VALUE != hConnectEvent )
	{
		CloseHandle( hConnectEvent ); 
		hConnectEvent = INVALID_HANDLE_VALUE;
	}

	if( pTcpProv )
	{
		if( INVALID_SOCKET != pTcpProv->m_sock)
		{		
			shutdown(pTcpProv->m_sock, SD_SEND);		
			closesocket(pTcpProv->m_sock);
		}

		delete pTcpProv;
	}

	*ppProv = NULL;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD Tcp::PartialReadAsync(__in SNI_Packet * pPacket, DWORD cbBytesToRead, SNI_ProvInfo * pInfo)
{	
	BidxScopeAutoSNI4( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("cbBytesToRead: %d, ")
							  _T("pInfo: %p{SNI_ProvInfo*}\n"), 
							  GetBidId(),
							  pPacket, 
							  cbBytesToRead, 
							  pInfo);

	DWORD dwError = ERROR_FAIL;

	Assert( pPacket->m_OffSet == 0);

	Assert( pPacket->m_OffSet+pPacket->m_cbBuffer+cbBytesToRead <= pPacket->m_cBufferSize );

	DWORD dwcbOldSize = 0; 
	DWORD dwcbNewSize = 0; 
	// This should not happen but we want to be extra careful here.  
	if( FAILED( DWordAdd( pPacket->m_OffSet, pPacket->m_cbBuffer, &dwcbOldSize ) ) ||
		FAILED( DWordAdd( dwcbOldSize, cbBytesToRead, &dwcbNewSize ) ) )
	{
		dwError = INTSAFE_E_ARITHMETIC_OVERFLOW;

		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwError);
		
		goto Exit;				
	}

	if( dwcbNewSize > pPacket->m_cBufferSize )
	{
		dwError = ERROR_BUFFER_OVERFLOW; 
			
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwError);
		
		goto Exit;				
	}

	dwError = PostReadAsync(pPacket, cbBytesToRead);
	
Exit:
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}

DWORD Tcp::PostReadAsync(SNI_Packet *pPacket, DWORD cbBuffer)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("cbBuffer: %d{DWORD}, ")
							  _T("pPacket: %p{SNI_Packet*}\n"),
							  GetBidId(),
							  cbBuffer,
							  pPacket);
	
	DWORD dwError = ERROR_SUCCESS;
	// Post the transport read
	DWORD 	dwBytesRead = 0;
	DWORD dwWSARecvFlags = 0;
	OVERLAPPED *pOverlapped = SNIPacketOverlappedStruct(pPacket);

	WSABUF ReadBuf;
	ReadBuf.buf = (char *)(SNIPacketGetBufPtr(pPacket) + SNIPacketGetBufferSize(pPacket));
	ReadBuf.len = cbBuffer;

	if( NULL != m_pSyncReadPacket )
	{
		Assert( 0 && "It is forbidden to call SNIReadAsync or SNIPartialReadAsync when there is a cached Sync Read packet.\n");
		dwError = ERROR_INVALID_STATE;
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_0, dwError);
		goto Exit;
	}

	PrepareForAsyncCall(pPacket);

	// Already checked validity of the buffer size in calling function; just Assert it here
	Assert(cbBuffer <= (SNIPacketGetBufActualSize(pPacket) - SNIPacketGetBufferSize(pPacket)));

	if( SOCKET_ERROR == WSARecv( m_sock,
              &ReadBuf,
              1,
			  NULL,
			  &dwWSARecvFlags,
			  pOverlapped,
			  NULL ) )
	{
		dwError = WSAGetLastError();

		if( dwError != WSA_IO_PENDING )
		{
			SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwError);
			goto Exit;
		}
		else
		{
			dwError = ERROR_IO_PENDING;
			goto Exit;
		}
	}
	else if (!s_fSkipCompletionPort)
	{
		// If we haven't enabled skipping the completion port, then we need to wait for it to be set
		dwError = ERROR_IO_PENDING;
		goto Exit;
	}

	if( 0 == GetOverlappedResult((HANDLE) m_sock, pOverlapped, &dwBytesRead, FALSE ))
	{
		dwError = GetLastError();
		BidTrace1( ERROR_TAG _T("GetOverlappedResult: %d{WINERR}\n"), dwError );
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwError);
		goto Exit;
	}

	if( 0 == dwBytesRead )
	{
		dwError = WSAECONNRESET;
		BidTrace0( ERROR_TAG _T("Successful 0-byte TCP read: returning WSAECONNRESET\n") );
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwError);

		goto Exit; 
	}
	else
	{
		// Packet is getting returned - fix up buffer size.
		SNIPacketSetBufferSize( pPacket, SNIPacketGetBufferSize(pPacket) + dwBytesRead );
		goto Exit;
	}

Exit:
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}

DWORD Tcp::PartialReadSync(__inout SNI_Packet * pPacket, DWORD cbBytesToRead, int timeout)
{	
	BidxScopeAutoSNI4( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("cbBytesToRead: %d, ")
							  _T("timeout: %d\n"), 
							  GetBidId(),
							  pPacket, 
							  cbBytesToRead, 
							  timeout);

	DWORD dwRet = ERROR_SUCCESS;
	Assert( pPacket->m_OffSet == 0);
	
	if( timeout != INFINITE )
	{
		struct timeval t_timeout;
		int retval;
		fd_set ReadFds;

		FD_ZERO( &ReadFds );
		FD_SET( m_sock, &ReadFds );
	
		if( timeout )
		{
			t_timeout.tv_sec = (long) timeout/1000;	// Seconds
			t_timeout.tv_usec = (long) (timeout%1000) * 1000;	// MicroSeconds
		}
		else
			t_timeout.tv_sec  = t_timeout.tv_usec  = 0;
		
		retval = select( 0, &ReadFds, 0, 0, &t_timeout);

		// Timeout
		if( 0 == retval )
		{
			dwRet = WSA_WAIT_TIMEOUT;

			SNI_SET_LAST_ERROR( TCP_PROV, SNIE_11, dwRet);

			goto ExitFunc;	
		}
		
		// Error
		else if( SOCKET_ERROR == retval )
		{
			dwRet = WSAGetLastError();

			SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet);

			goto ExitFunc;
		}

		// Since there is only socket, this should always be 1
		Assert(1 == retval);
	}

	// Get the data pointer for this packet - remember to start reading from end of previous read
	WSABUF ReadBuf;
	ReadBuf.buf = (char *) (SNIPacketGetBufPtr(pPacket) + SNIPacketGetBufferSize(pPacket));

	// Set the bytes to read
	ReadBuf.len = cbBytesToRead;

	Assert( pPacket->m_OffSet+pPacket->m_cbBuffer+cbBytesToRead <= pPacket->m_cBufferSize );

	DWORD dwcbOldSize = 0; 
	DWORD dwcbNewSize = 0; 
	// This should not happen but we want to be extra careful here.  
	if( FAILED( DWordAdd( pPacket->m_OffSet, pPacket->m_cbBuffer, &dwcbOldSize ) ) ||
		FAILED( DWordAdd( dwcbOldSize, cbBytesToRead, &dwcbNewSize ) ) )
	{
		dwRet = INTSAFE_E_ARITHMETIC_OVERFLOW;

		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet);

		goto ExitFunc;
	}

	if( dwcbNewSize > pPacket->m_cBufferSize )
	{
		dwRet = ERROR_BUFFER_OVERFLOW; 
			
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet);

		goto ExitFunc;
	}

	// Don't need to call pPacket->AddOvlEvent, since we aren't using an Overlapped struct on this WSARecv call (which we aren't doing since we either used select() to ensure the data is there, or are blocking until it is there anyway).

	// Post the transport read
	DWORD 	dwBytesRead = 0;
	DWORD dwFlags = 0;

	if( SOCKET_ERROR == WSARecv( m_sock,
              &ReadBuf,
              1,
              &dwBytesRead,
              &dwFlags,
              NULL,
              NULL ) )
	{
		dwRet = WSAGetLastError();

		// it shouldn't timeout here because we use select()

		Assert( dwRet != WSAETIMEDOUT );

		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet);

		goto ExitFunc;
	}

	if( 0 == dwBytesRead )
	{
		dwRet = WSAECONNRESET;

		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet);

		goto ExitFunc; 
	}

	SNIPacketSetBufferSize( pPacket, dwBytesRead+SNIPacketGetBufferSize(pPacket));

ExitFunc:

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;

}

// Async functions

DWORD Tcp::ReadAsync(__out SNI_Packet ** ppNewPacket, LPVOID pPacketKey)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("ppNewPacket: %p{SNI_Packet**}, ")
							  _T("pPacketKey: %p\n"), 
							  GetBidId(),
							  ppNewPacket, 
							  pPacketKey);
	
	DWORD dwRet = ERROR_SUCCESS;

	// Allocate a new packet
	SNI_Packet  * pPacket = SNIPacketAllocate(m_pConn, SNI_Packet_Read);
	if( !pPacket )
	{
		dwRet = ERROR_OUTOFMEMORY;

		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_4, dwRet);
		
		goto Exit;
	}

	Assert( pPacket->m_OffSet == 0);

	// Set the packet key
	SNIPacketSetKey(pPacket, pPacketKey);

	// Since we may not return any new packet here, initialize ppNewPacket to NULL
	*ppNewPacket = NULL;
	
	// Post the transport read
	dwRet = PostReadAsync(pPacket, SNIPacketGetBufActualSize(pPacket));
	
Exit:

	if( dwRet == ERROR_SUCCESS )
	{
		// on success case, leave the packet ref alive and pass it up to the caller.
		*ppNewPacket = pPacket;
	}
	else if( dwRet == ERROR_IO_PENDING )
	{
		// on pending case, leave the packet ref alive but don't pass it to the caller.
	}
	else if( pPacket )
	{
		// on failure case, release the packet (if it was allocated).
		SNIPacketRelease( pPacket );
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;
}

DWORD Tcp::ReadDone(__inout SNI_Packet ** ppPacket, __out SNI_Packet **ppLeftOver, DWORD dwBytes, DWORD dwError)
{
	BidxScopeAutoSNI5( SNIAPI_TAG _T("%u#, ")
							  _T("ppPacket: %p{SNI_Packet**}, ")
							  _T("ppLeftOver: %p{SNI_Packet**}, ")
							  _T("dwBytes: %d, ")
							  _T("dwError: %d{WINERR}\n"), 
							  GetBidId(),
							  ppPacket, 
							  ppLeftOver, 
							  dwBytes, 
							  dwError);
	
	// If its an error, return the error. Tcp does not treat any errors as "valid".
	*ppLeftOver = 0;
	
	if( dwError )
	{
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwError);

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
		return dwError;
	}
	// Check if we received a 0 byte packet - that means the connection was disconnected
	if( 0 == dwBytes )
	{
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, WSAECONNRESET);
	
		BidTrace0( ERROR_TAG _T("Successful 0-byte TCP read: returning WSAECONNRESET\n") );
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), WSAECONNRESET);
		return WSAECONNRESET;			
	
	}
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	return ERROR_SUCCESS;
}

DWORD Tcp::ReadSync(__out SNI_Packet ** ppNewPacket, int timeout)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("ppNewPacket: %p{SNI_Packet**}, ")
							  _T("timeout: %d\n"), 
							  GetBidId(),
							  ppNewPacket, 
							  timeout);
	
	DWORD dwRet = ERROR_SUCCESS;
	SNI_Packet  * pPacket = NULL;
	DWORD 	dwBytesRead = 0;
	OVERLAPPED * pOvl = 0;

	if( !gfIsWin9x )
	{
		// Non-Win9x: use GetOverlappedResult()
		//
	// Loop till we have valid data and break out
	while(1)
	{
		// Check if there is no previous read pending
		if( !m_pSyncReadPacket )
		{
			// Allocate a new packet
			pPacket = SNIPacketAllocate(m_pConn, SNI_Packet_Read);
			if( !pPacket )
			{
				dwRet = ERROR_OUTOFMEMORY;

				SNI_SET_LAST_ERROR( TCP_PROV, SNIE_4, dwRet);

				goto ErrorExit;
			}

			Assert( pPacket->m_OffSet == 0);

			PrepareForSyncCall(pPacket);

			// Get the exact data pointer and buffer size for this packet
			WSABUF ReadBuf;
			ReadBuf.buf = (char *)SNIPacketGetBufPtr(pPacket);
			ReadBuf.len = SNIPacketGetBufActualSize(pPacket);

			DWORD dwFlags = 0;

			pOvl = SNIPacketOverlappedStruct(pPacket);
			pOvl->Offset = 0;
			pOvl->OffsetHigh = 0;

			// Post the transport read
			if( SOCKET_ERROR == WSARecv( m_sock,
		              &ReadBuf,
		              1,
		              &dwBytesRead,
		              &dwFlags,
		              pOvl,
		              NULL ) )
			{
				dwRet = WSAGetLastError();

				if( ERROR_IO_PENDING != dwRet )
				{
					SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet);

					goto ErrorExit;
				}
			}
		      
		}

		// There is an earlier read posted
		else
		{
			pPacket = m_pSyncReadPacket;
			pOvl = SNIPacketOverlappedStruct(pPacket);
			m_pSyncReadPacket = 0;

			dwRet = ERROR_IO_PENDING;
		}

		Assert(pOvl);
		
		if( ERROR_IO_PENDING == dwRet )
		{
			DWORD dwStart = GetTickCount();
			
			dwRet = WaitForSingleObject(pOvl->hEvent, timeout);

			// Error
			if( WAIT_FAILED == dwRet )
			{
				SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );
				goto ErrorExit;
			}

			// Timeout
			else if( WAIT_TIMEOUT == dwRet )
			{
				m_pSyncReadPacket = pPacket;
				dwRet = WSA_WAIT_TIMEOUT;
				SNI_SET_LAST_ERROR( TCP_PROV, SNIE_11, dwRet);
				goto ErrorExit;	
			}

			// Valid data
			else if( !GetOverlappedResult((HANDLE) m_sock, pOvl, &dwBytesRead, FALSE ))
			{
				dwRet = GetLastError();

				// Check to see if this was caused because the thread which posted
				// the earlier read exited - in that case repost the read
				if( ERROR_OPERATION_ABORTED == dwRet )
				{
					SNIPacketRelease(pPacket);
					pPacket = m_pSyncReadPacket = 0;
					dwBytesRead = 0;
					dwRet = ERROR_SUCCESS;

					// Adjust the timeout to exclude the time spent waiting
					if(INFINITE != timeout )
					{
						timeout -= (GetTickCount() - dwStart);

						// If we have timeed out, exit
						if( timeout <= 0 )
						{
							dwRet = WSA_WAIT_TIMEOUT;
							SNI_SET_LAST_ERROR( TCP_PROV, SNIE_11, dwRet);
							goto ErrorExit;	
						}
					}

					// Do this all over again - allocate the packet, post the read, etc.
					continue;
				}
				
				SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );
				goto ErrorExit;
			}
		}

		// Read completed - exit the loop
		break;
	}

	Assert( !m_pSyncReadPacket );
	}	// Non-Win9x
	else
	{
		// Win9x: use select()
		//
		if( timeout != INFINITE )
		{
			struct timeval t_timeout;
			int retval;
			fd_set ReadFds;

			FD_ZERO( &ReadFds );
			FD_SET( m_sock, &ReadFds );
			
			if( timeout )
			{
				t_timeout.tv_sec = (long) timeout/1000;	// Seconds
				t_timeout.tv_usec = (long) (timeout%1000) * 1000;	// MicroSeconds
			}
			else
				t_timeout.tv_sec  = t_timeout.tv_usec  = 0;
			
			retval = select( 0, &ReadFds, 0, 0, &t_timeout);

			// Timeout
			if( 0 == retval )
			{
				dwRet = WSA_WAIT_TIMEOUT;

				SNI_SET_LAST_ERROR( TCP_PROV, SNIE_11, dwRet);

				goto ErrorExit;	
			}
			
			// Error
			else if( SOCKET_ERROR == retval )
			{
				dwRet = WSAGetLastError();

				SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet);

				goto ErrorExit;
			}

			// Since there is only one socket, this should always be 1
			Assert(1 == retval);
		}

		// Allocate a new packet
		pPacket = SNIPacketAllocate(m_pConn, SNI_Packet_Read);
		if( !pPacket )
		{
			dwRet = ERROR_OUTOFMEMORY;

			SNI_SET_LAST_ERROR( TCP_PROV, SNIE_4, dwRet);

			goto ErrorExit;
		}
		
		Assert( pPacket->m_OffSet == 0);
		
		// Get the exact data pointer and buffer size for this packet
		WSABUF ReadBuf;
		ReadBuf.buf = (char *)SNIPacketGetBufPtr(pPacket);
		ReadBuf.len = SNIPacketGetBufActualSize(pPacket);

		// Post the transport read
		DWORD dwFlags = 0;

		if( SOCKET_ERROR == WSARecv( m_sock,
	              &ReadBuf,
	              1,
	              &dwBytesRead,
	              &dwFlags,
	              NULL,
	              NULL ) )
		{
			dwRet = WSAGetLastError();

			// it shouldn't timeout here because we use select()

			Assert( dwRet != WSAETIMEDOUT );

			SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet);

			goto ErrorExit;
		}			
	}	// Win9x
	
	if( 0 == dwBytesRead )
	{
		dwRet = WSAECONNRESET;

		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet);

		goto ErrorExit; 
	}
	
	SNIPacketSetBufferSize( pPacket, dwBytesRead );

	*ppNewPacket = pPacket;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;

ErrorExit:

	if( !gfIsWin9x )
	{
		// If its any error other than timeout - release the packet
		if( (WSA_WAIT_TIMEOUT != dwRet) && pPacket )
		{
			Assert( !m_pSyncReadPacket );
			
			SNIPacketRelease( pPacket );
			m_pSyncReadPacket = 0;
		}
	}
	else
	{
		if( pPacket )
			SNIPacketRelease( pPacket );		
	}

	*ppNewPacket = NULL;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;

}

DWORD Tcp::CheckConnection( )
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#, "), GetBidId());
	
	DWORD dwRet = ERROR_FAIL;

	// Setup polling object
	WSAPOLLFD poll;
	poll.fd = m_sock;
	poll.events = POLLWRNORM;
	poll.revents = 0;

	// Poll the connection
	if( SOCKET_ERROR == WSAPoll( &poll, 1, 0 ) )
	{
		dwRet = WSAGetLastError();
		BidTrace1( ERROR_TAG _T("WSAPoll to check for live connection: %u{WINERR}"), dwRet );
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );
	}
	else
	{
		// Polling success - check the state of the connection
		// NOTE: Even though we were checking for POLLWRNORM, it might not be set here (since it being set indicates that the next call to send won't block)
		// As such, the only thing that we can rely on is that the various error flags shouldn't be set if the connection is ok
		if( ( poll.revents & ( POLLERR | POLLHUP | POLLNVAL ) ) != 0 )
		{
			// Connection is bad
			dwRet = ERROR_FAIL;
			BidTrace1( ERROR_TAG _T("WSAPoll to check for live connection. Connection is in bad state: %u"), poll.revents );
			SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );
		}
		else
		{
			// Connection is good
			dwRet = ERROR_SUCCESS;
		}
	}
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%u{WINERR}\n"), dwRet);
	
	return dwRet;
}

void Tcp::Release()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T( "\n") );
	
	delete this;
}

DWORD Tcp::SetKeepAliveOption()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

#ifdef SNI_BASED_CLIENT

	LONG  lResult = 0;

	DWORD dwKeepAlive		  = KEEPALIVETIME;
	DWORD dwKeepAliveInterval = KEEPALIVEINTERVAL;

#ifndef SNIX
	CS_PROTOCOL_PROPERTY ProtocolProperty;

	lResult = CSgetProtocolProperty( TEXT("tcp"),
									 CS_PROP_TCP_KEEP_ALIVE,
									 &ProtocolProperty );
	if( (ERROR_SUCCESS == lResult) && 
		(REG_DWORD == ProtocolProperty.dwPropertyType) )
		dwKeepAlive = ProtocolProperty.PropertyValue.dwDoubleWordValue;

	lResult = CSgetProtocolProperty( TEXT("tcp"),
									 CS_PROP_TCP_KEEP_ALIVE_INTERVAL,
									 &ProtocolProperty );
	if( (ERROR_SUCCESS == lResult) && 
		(REG_DWORD == ProtocolProperty.dwPropertyType) )
		dwKeepAliveInterval = ProtocolProperty.PropertyValue.dwDoubleWordValue;
#endif

	// KeepAlive stuff
	//
	DWORD cbBytesReturned = 0;
	struct tcp_keepalive tcpStruct;

	tcpStruct.onoff             = 1;
	tcpStruct.keepalivetime     = dwKeepAlive;
	tcpStruct.keepaliveinterval = dwKeepAliveInterval;

	if( SOCKET_ERROR == WSAIoctl(  m_sock,                                               
								  SIO_KEEPALIVE_VALS,                                  
								  &tcpStruct,                                     
								  sizeof(tcpStruct),                                       
								  NULL,                                    
								  0,                                      
								  &cbBytesReturned,                              
								  NULL,                           
								  NULL ))
	{
		DWORD dwError = WSAGetLastError();
		
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwError );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
		
		return dwError;
	}

#endif

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	return ERROR_SUCCESS;
}

DWORD Tcp::Terminate()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T( "\n") );

	if( !rgProvInfo[TCP_PROV].fInitialized )
	{
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
		return ERROR_SUCCESS;
	}

	// Cleanup Winsock
	WSACleanup();
	
#ifndef SNI_BASED_CLIENT

	if( g_hKernel32 )
	{
		FreeLibrary(g_hKernel32);
		g_hKernel32 = NULL;
	}
#endif

	rgProvInfo[TCP_PROV].fInitialized = FALSE; 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	return ERROR_SUCCESS;
}

DWORD Tcp::TerminateListener(__inout HANDLE hListener)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "hListener: %p{HANDLE}\n"), hListener);
	
	((ListenObject *) hListener)->TerminateListener();

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	return ERROR_SUCCESS;
}

DWORD Tcp::ResumePendingAccepts(__in HANDLE hListener)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "hListener: %p{HANDLE}\n"), hListener);

	DWORD dwRet = ERROR_SUCCESS;

	// ListenObject and all of the accept objects are safe to access
	// here because this the listener is guaranteed to have been
	// removed from the global SNI_Listener list as one of the first 
	// things when SNITerminateListener is called and this code is 
	// guaranteed to have completed running before SNITerminateListener
	// continues.
	// This is accomplished by the common crit sec used by 
	// AddListener, FRemoveListener, and SNIResumePendingAccepts.
	//
	ListenObject *pListenObject = static_cast<ListenObject *>(hListener);

	for (UINT i = 0; i < pListenObject->m_nAcceptObjects; i++)
	{
		if (pListenObject->m_ppAcceptObjects[i]->m_fPendingAccept)
		{
			// This ref will be released either in AcceptDone
			// or below if AsyncAccept fails
			pListenObject->m_ppAcceptObjects[i]->AddRef();

			DWORD dwLocalRet = pListenObject->m_ppAcceptObjects[i]->AsyncAccept();
			if (ERROR_SUCCESS == dwLocalRet)
			{
				scierrlog (26041, 
					pListenObject->m_TcpListenInfo.Port);
			}
			else
			{
				BidTrace2(ERROR_TAG _T("AcceptObject: %p{AcceptObject*}, %d{WINERR}\n"), 
					pListenObject->m_ppAcceptObjects[i], dwLocalRet);

				pListenObject->m_ppAcceptObjects[i]->Release();
				dwRet = dwLocalRet;
			}
		}
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;
}

DWORD Tcp::WriteAsync(__in SNI_Packet * pPacket, SNI_ProvInfo * pInfo)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("pInfo: %p{SNI_ProvInfo*}\n"), 
							  GetBidId(),
							  pPacket, 
							  pInfo);

	WSABUF WriteBuf;
	DWORD dwFlags = 0;
	LPWSAOVERLAPPED pOvl = SNIPacketOverlappedStruct(pPacket);

	DWORD dwError=ERROR_SUCCESS;

	if(m_fAuto && ERROR_SUCCESS != (dwError = CheckAndAdjustSendBufferSizeBasedOnISB()))
	{
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwError);
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
		return dwError;
	}

	PrepareForAsyncCall(pPacket);

	SNIPacketGetData(pPacket, (BYTE **)&WriteBuf.buf, &WriteBuf.len);

OACR_WARNING_PUSH
OACR_WARNING_DISABLE(USING_UNINIT_VAR, "PREfast limitation: The WriteBuf struct is initialized by SNIPacketGetData which is properly annotated, but PREfast cannot detect.");

	// additional AddRef around WSASend call protects
	// against SNIWriteDone being called before WSASend returns
	SNIPacketAddRef(pPacket);

	if( SOCKET_ERROR == WSASend( m_sock,
								  &WriteBuf,
								  1,
								  NULL,
								  dwFlags,
								  pOvl,
								  NULL ) )
	{
OACR_WARNING_POP
		dwError = WSAGetLastError();

		SNIPacketRelease(pPacket);

		if( WSA_IO_PENDING ==  dwError )
		{
			dwError = ERROR_IO_PENDING;
			goto Exit;
		}
		else
		{
			SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwError);
			goto Exit;
		}
	}
	else if (!s_fSkipCompletionPort)
	{
		// If we haven't enabled skipping the completion port, then we need to wait for it to be set
		dwError = ERROR_IO_PENDING;
		goto Exit;
	}
	else
	{
		SNIPacketRelease(pPacket);
	}

	dwError = ERROR_SUCCESS;

Exit:
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	return dwError;
}                    

DWORD Tcp::WriteDone(SNI_Packet ** ppPacket, DWORD dwBytes, DWORD dwError)
{
	BidxScopeAutoSNI4( SNIAPI_TAG _T("%u#, ")
							  _T("ppPacket: %p{SNI_Packet**}, ")
							  _T("dwBytes: %d, ")
							  _T("dwError: %d{WINERR}\n"), 
							  GetBidId(),
							  ppPacket, 
							  dwBytes, 
							  dwError);

	// If its an error, return the error. Tcp does not treat any errors as "valid".
	if( dwError )
	{
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwError);
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
		return dwError;
	}
	
	// Check if we received a 0 byte packet - that means the connection was disconnected
	Assert( dwBytes );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	return ERROR_SUCCESS;
}

DWORD Tcp::WriteSync(__in SNI_Packet * pPacket, SNI_ProvInfo * pInfo)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T( "pPacket: %p{SNI_Packet*}, ")
							  _T("pInfo: %p{SNI_ProvInfo*}\n"), 
							  GetBidId(),
							  pPacket, 
							  pInfo);

	WSABUF WriteBuf;
	DWORD 	dwBytesWritten = 0;
	DWORD dwFlags = 0;
	DWORD dwError=ERROR_SUCCESS;

	if(m_fAuto && ERROR_SUCCESS != (dwError = CheckAndAdjustSendBufferSizeBasedOnISB()))
	{
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwError);
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
		return dwError;
	}

	PrepareForSyncCall(pPacket);
	
	Assert( pPacket->m_OffSet == 0);
	SNIPacketGetData(pPacket, (BYTE **)&WriteBuf.buf, &WriteBuf.len);

OACR_WARNING_PUSH
OACR_WARNING_DISABLE(USING_UNINIT_VAR, "PREfast limitation: The WriteBuf struct is initialized by SNIPacketGetData which is properly annotated, but PREfast cannot detect.");

	if( SOCKET_ERROR == WSASend( m_sock,
								  &WriteBuf,
								  1,
								  &dwBytesWritten,
								  dwFlags,
								  NULL,
								  NULL ) )
	{
OACR_WARNING_POP
		dwError = WSAGetLastError();

		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwError);
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
		return dwError;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	return ERROR_SUCCESS;
}                   

DWORD Tcp::CheckAndAdjustSendBufferSizeBasedOnISB()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#, "), GetBidId());
	
	Assert( m_fAuto );

	DWORD dwError = ERROR_SUCCESS;
	DWORD dwBytes=0;
	DWORD dwSendBufSize=0;
	
	if(HasOverlappedIoCompleted(m_pOvSendNotificaiton)) 
	{ 
		CAutoSNICritSec a_csTuning(m_CSTuning, SNI_AUTOCS_ENTER );
		
		if ( !HasOverlappedIoCompleted(m_pOvSendNotificaiton))
		{
			// It is possible that another thread posted the notification request, 
			// i.e. calling idealsendbacklognotify, just before the current thread.
			// Now, the request is pending completion again. Return from here.
			//
			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);	

			return dwError;
		}
		
		// if the notification request is not pending anymore
		// 
		if(!GetOverlappedResult((HANDLE) m_sock, m_pOvSendNotificaiton, &dwBytes, FALSE/*bWait*/))
		{
			dwError = GetLastError();

			Assert(ERROR_SUCCESS != dwError);
			Assert(ERROR_IO_PENDING != dwError); //HasOverlappedIoCompleted just tell us otherwise.

			BidTraceU1(SNI_BID_TRACE_ON, SNI_TAG _T("GetOverlappedResult failed%d{WINERR}\n"), dwError);		 

			// Check to see if this was caused because the thread which posted
			// the earlier read exited - in that case continue and repost notification request.
			// regard other errors as fatal.
			if( ERROR_OPERATION_ABORTED == dwError )
			{
				dwError = ERROR_SUCCESS;
			}
			else
			{
				goto ErrorExit;
			}
		}

		// post the notification again
		//
		if(SOCKET_ERROR == idealsendbacklognotify(m_sock,m_pOvSendNotificaiton, NULL)) 
		{
			dwError = WSAGetLastError();

			Assert(ERROR_SUCCESS != dwError);
			
			if(WSA_IO_PENDING != dwError)
			{
				if( m_pOvSendNotificaiton->Internal == 0x103)
				{
					// It is possible that winsock leave Internal set before return failure.
					// It is safe to reset it to allow dtor to release the overlap struct. 
					// See vsts: 123803.
					//
					m_pOvSendNotificaiton->Internal = 0;
				}
		
				//if idealsendbacklognotify is not supported, turn autotuning off.
				//could be error other than not supported, but we treat them as the same now.
				//
				BidTraceU1(SNI_BID_TRACE_ON, SNI_TAG _T("idealsendbacklognotify failed%d{WSAERR}\n"), dwError);		 
				m_fAuto = FALSE;
				dwError = ERROR_SUCCESS;					
				goto ErrorExit;
			}
			else
			{
				Assert(m_pOvSendNotificaiton->Internal == 0x103);

				dwError=ERROR_SUCCESS;
			}
		}

		// query the new ISB
		if(SOCKET_ERROR == idealsendbacklogquery(m_sock, &dwSendBufSize)) 
		{
			dwError = WSAGetLastError();

			Assert(ERROR_SUCCESS != dwError);
			
			//if idealsendbacklognotify is not supported, turn it off.
			//could be error other than not supported, but we treat them as the same now.
			//
			BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("idealsendbacklogquery failed%d{WSAERR}\n"), dwError);		 
			m_fAuto = FALSE;
			dwError = ERROR_SUCCESS;					
			goto ErrorExit;
		}

		// set SO_SNDBUF to the new ISB value
		if((8192 < dwSendBufSize) && (SOCKET_ERROR == setsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, (const char *)&dwSendBufSize, sizeof(dwSendBufSize)))) 
		{
			dwError = WSAGetLastError();
						
			Assert(ERROR_SUCCESS != dwError);

			BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("setsockopt failed%d{WSAERR}\n"), dwError);		 			

			goto ErrorExit;		
       	}

		BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("SO_SNDBUF:%d.\n"), dwSendBufSize);
	}

ErrorExit:	

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);	
	
	return dwError;
	
}

// Inform the OS not to enqueue IO Completions on successful
// overlapped reads/writes.
//
// Returns ERROR_SUCCESS if OS call succeeds, or if connection is Sync (no need to make the call)
// Returns a Windows error code otherwise.
DWORD
Tcp::DWSetSkipCompletionPortOnSuccess()
{
	BidxScopeAutoSNI1(SNIAPI_TAG _T("%u#\n"), GetBidId());
	DWORD dwRet = ERROR_SUCCESS;
	
	if(( s_fSkipCompletionPort ) && ( !m_pConn->m_fSync ))
	{
		BidTraceU1(SNI_BID_TRACE_ON, SNI_TAG _T("Enabling 'Skip IO completion port on success' for %u#{SNI_Conn}\n"), m_pConn->GetBidId());
		if( 0 == SetFileCompletionNotificationModes(m_hNwk, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS))
		{
			dwRet = GetLastError();
			goto Exit;
		}
	}
	else
	{
		BidTraceU1(SNI_BID_TRACE_ON, SNI_TAG _T("Not enabling 'Skip IO completion port on success' for %u#{SNI_Conn}\n"), m_pConn->GetBidId());
	}

Exit:
	
	BidTraceU1(SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;
}

// ----------------------------------------------------------------------------
//	1) Add the Overlapped event to the packet
//	2) Odd the overlapped's Event handle, to ensure an IOC doesn't get enqueued to the IOCP.
//		(see documentation of GetQueuedCompletionStatus
void
Tcp::PrepareForSyncCall(SNI_Packet *pPacket)
{
	BidxScopeAutoSNI2(SNIAPI_TAG _T("%u#, pPacket{SNI_Packet*}\n"), GetBidId(), pPacket);

	pPacket->AddOvlEvent();
	pPacket->SetOvlEventLowBit();

	BidTraceU0(SNI_BID_TRACE_ON, RETURN_TAG _T("\n"));
}

// ----------------------------------------------------------------------------
//	1) Remove the Overlapped event from the packet
void
Tcp::PrepareForAsyncCall(SNI_Packet *pPacket)
{
	BidxScopeAutoSNI2(SNIAPI_TAG _T("%u#, pPacket{SNI_Packet*}\n"), GetBidId(), pPacket);

	pPacket->RemoveOvlEvent(); // For old SNI
	BidTraceU0(SNI_BID_TRACE_ON, RETURN_TAG _T("\n"));
}
// ----------------------------------------------------------------------------

// Checks if any Non-IFS LSPs are installed for TCP
// These are incompatible with the FILE_SKIP_COMPLETION_PORT_ON_SUCCESS option
// For more information: http://msdn.microsoft.com/en-us/library/windows/desktop/aa365538(v=vs.85).aspx
DWORD Tcp::ShouldEnableSkipIOCompletion(__out BOOL* pfShouldEnable)
{
	BidxScopeAutoSNI1(SNIAPI_TAG _T("pfShouldEnable: %p{BOOL*}\n"), pfShouldEnable);

	DWORD dwRet = ERROR_SUCCESS;
	*pfShouldEnable = TRUE;

	// WSAEnumProtocols requires a NULL-delimited array of protocols to query
	int protocols[] = {IPPROTO_TCP, NULL};

	// Do initial query to find the size of the buffer array
	// This will either error with WSAENOBUFS (indicating that a larger buffer is needed), or that there are no protocols
	WSAPROTOCOL_INFO *infoBuffer = NULL;
	DWORD bufferLength = 0;
	int protocolCount = WSAEnumProtocols( protocols, infoBuffer, &bufferLength);
	if( protocolCount == SOCKET_ERROR )
	{
		dwRet = WSAGetLastError();

		if ( dwRet == WSAENOBUFS )
		{
			// Need a bigger buffer
			infoBuffer = (WSAPROTOCOL_INFO *)NewNoX(gpmo) char[bufferLength];
			if( infoBuffer == NULL  )
			{
				// Allocation failed
				dwRet = ERROR_OUTOFMEMORY;
			}
			else
			{
				// Query again with the correct buffer size
				protocolCount = WSAEnumProtocols(protocols, infoBuffer, &bufferLength);
				if ( protocolCount == SOCKET_ERROR )
				{
					// WinSock error, fallthrough to exit
					dwRet = WSAGetLastError();
				}
				else
				{
					// Got the protocols, now we need to scan for Non-IFS LSPs
					for ( int i = 0; i < protocolCount; i++ )
					{
						if ( (infoBuffer[i].dwServiceFlags1 & XP1_IFS_HANDLES) != XP1_IFS_HANDLES )
						{
							*pfShouldEnable = FALSE;
							break;
						}
					}

					dwRet = ERROR_SUCCESS;
				}

				delete [] (char *) infoBuffer;
			}
		}
		// else: There was a WinSock error, fallthrough to exit
	}
	else
	{
		// No error, so there must not be any protocols for us to look at
		Assert( protocolCount == 0 && bufferLength == 0 );
	}
	
	BidTraceU1(SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;
}

// Get the IP addr count
UINT Tcp::GetAddrCount(const ADDRINFOW *AIW)
{
    DWORD dwAddresses = 0;
    const ADDRINFOW *paiTemp = AIW;
    for (; NULL != paiTemp; paiTemp = paiTemp->ai_next)
    {
        // Skip address families that we don't know how to use.
        if ((AF_INET != paiTemp->ai_family && AF_INET6 != paiTemp->ai_family))
            continue;

        dwAddresses++;
    }

    return dwAddresses;
}

