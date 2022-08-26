//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: sni.cpp
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
#include "np.hpp"
#include "tcp.hpp"
#include "sm.hpp"
#include "smux.hpp"
#include "ssl.hpp"
#include "sni_sspi.hpp"
#include "via.hpp"
#include "SNI_ServiceBindings.hpp"
#include "LocalDB.hpp"

#ifndef SNI_BASED_CLIENT
#include "httpprov.h"
// needed for cerr, etc.
using namespace std;

#include <nodelistener.hpp>

volatile Affinity	NodeListener::s_OnlineNodeMask = 0;

extern NodeListener* s_pNodeListener;

#define OnlineNodeMask (NodeListener::GetOnlineNodeMask())

#else
// use the current process locale for CRT calls
_locale_t __stdcall GetDefaultLocale()	{ return NULL; }

#define OnlineNodeMask ((DWORD_PTR)(0)-1)
#endif

#ifndef SNI_BASED_CLIENT
//	This is a work-around to include NLregS header file with proper
//	ASCII/UNICODE characters and string-formatting functions.  
//	It looks like we cannot rely on the definition of UNICODE used
//	by nlregs.h because in this file 'TCHAR' appears to be defined as
//	'char' but '_tcslen' is 'wcslen'.  
//	Since most of the parameters we are passing to and getting from NLregS
//	are ASCII, we use the ASCII version of NLregS.  To read the nlregs.h
//	file as ASCII, we define an NLregS-specific macro NLREGS_ASCII, which
//	will ensure that nlregs.h is interpreted as ASCII regardless of the 
//	definition of UNICODE and/or other related macros.  
//
#define NLREGS_ASCII
#include "nlregs.h"

CLUSTER_API gClusApi;
WCHAR 		gwszClusterName[SS_MAX_CLUSTER_NAME];
#endif

BID_METATEXT( _T("<ApiGroupRange|SNI> 0x0003F000"));

// !!! Important: the bit specified below must match BIDX_APIGROUP_SNI
// defined in sni_common.hpp.  
//
BID_METATEXT( _T("<ApiGroup|SNI> 0x00020000: SNI component"));


// Type definitions

typedef struct _SNI_Accept
{
	SOS_IOCompRequest	SOSIo;
	ProviderNum			ProvNum;
	DWORD_PTR			lastNode;
	LPVOID 				hProvAcceptKey;
	LPVOID				hSNIListener;
} SNI_Accept;

// Globals
#ifdef SNI_BASED_CLIENT

HANDLE	ghIoCompletionPort = NULL;
bool	g_fTerminate;
DWORD	gnWorkerThreadCount = 0;
HANDLE	rghWorkerThreads[64];
SNIMemObj	  * gpmo		= NULL;

DWORD WINAPI SNIAsyncWait (PVOID);


#ifdef SNIX
volatile LONG g_dwInitLock = 0;
volatile BOOL g_fInitDone = FALSE;
#endif	// #ifdef SNIX

#endif	// #ifdef SNI_BASED_CLIENT

WCHAR            gwszComputerName[MAX_NAME_SIZE+1];
BOOL gfIsWin9x = FALSE;		// Flag for when client is running on Win9x
BOOL gfIsWin2KPlatform = TRUE;	// Flag for when client is running Win2K or later
LONG gnConns;				// keeps a global count of connections

OSVERSIONINFO g_osviSNI;	// OS version info - used by SSL_PROV, SSPI_PROV, NP_PROV, etc.

DWORD SNI_Conn::iSniConnIndex = ~0;
SNI_Conn  * SNI_Conn::rgSniConn[MAX_PSM_ARRAY] = {0};

SNI_Listener * g_pListenerList = NULL;
SNICritSec * g_pcsListenerList = NULL;
LONG g_cPendingAccepts = 0;

// The g_fSandbox flag is used primarily by the Outlook version of SNAC
BOOL g_fSandbox = FALSE;      // When this is set to TRUE the following are disabled:
                              // 1. LastConnectCache - the mechanism where you store in registry last successful connect protocol details
                              // (only from SQLBrowser resolutions).
                              // 2. SNI protocols: SM,TCP,VIA. 
                              // 3. SMUX

ExtendedProtectionPolicy g_eExtendedProtectionLevel = ExtendedProtectionPolicy_INVALID; // Consumer must set

#if !defined(SNI_BASED_CLIENT) && !defined(STANDALONE_SNI)

////////////////////////////////////////////////////////////////////////////////
//	LoadClusterResourceLibraries
//
//	This function Dynamically link to clusapi.dll and resutils.dll and load the addresses 
//	of the cluster API procedures that we need.
//
//	Inputs:
//		[out] CLUSTER_API* pClusterApi:
//			Fill the data structure CLUSTER_API, which contains handles to libraries and pointers to functions.
//	Returns:
//		ERROR_SUCCESS, if the call succeeds, 
//		else, error code
//
//	Note: The caller of this function should call SNI_SET_LAST_ERROR.
//
DWORD LoadClusterResourceLibraries( CLUSTER_API* pClusterApi )
{

	BidxScopeAutoSNI1( SNIAPI_TAG _T( "pClusterApi: %p{CLUSTER_API*}\n"), pClusterApi );

	DWORD dwRet = ERROR_SUCCESS;

	Assert( pClusterApi ); 
	
	HMODULE hDll = SNILoadSystemLibraryA("CLUSAPI.DLL");
	if (hDll != NULL)
	{
		if ( !(pClusterApi->OpenCluster = (PFOPENCLUSTER) GetProcAddress(hDll,"OpenCluster"))) goto ErrorExit;
		if ( !(pClusterApi->CloseCluster = (PFCLOSECLUSTER) GetProcAddress(hDll,"CloseCluster"))) goto ErrorExit;
		if ( !(pClusterApi->OpenClusterResource = (PFOPENCLUSTERRESOURCE) GetProcAddress(hDll,"OpenClusterResource"))) goto ErrorExit;
		if ( !(pClusterApi->CloseClusterResource = (PFCLOSECLUSTERRESOURCE) GetProcAddress(hDll,"CloseClusterResource"))) goto ErrorExit;
		if ( !(pClusterApi->ClusterOpenEnum = (PFCLUSTEROPENENUM) GetProcAddress(hDll,"ClusterOpenEnum"))) goto ErrorExit;
		if ( !(pClusterApi->ClusterEnum = (PFCLUSTERENUM) GetProcAddress(hDll,"ClusterEnum"))) goto ErrorExit;
		if ( !(pClusterApi->ClusterCloseEnum = (PFCLUSTERCLOSEENUM) GetProcAddress(hDll,"ClusterCloseEnum"))) goto ErrorExit;
		if ( !(pClusterApi->ClusterResourceOpenEnum = (PFCLUSTERRESOURCEOPENENUM) GetProcAddress(hDll,"ClusterResourceOpenEnum"))) goto ErrorExit;
		if ( !(pClusterApi->ClusterResourceEnum = (PFCLUSTERRESOURCEENUM) GetProcAddress(hDll,"ClusterResourceEnum"))) goto ErrorExit;
		if ( !(pClusterApi->ClusterResourceCloseEnum = (PFCLUSTERRESOURCECLOSEENUM) GetProcAddress(hDll,"ClusterResourceCloseEnum"))) goto ErrorExit;

		if ( !(pClusterApi->ClusterResourceControl = (PFCLUSTERRESOURCECONTROL) GetProcAddress(hDll,"ClusterResourceControl"))) goto ErrorExit;

		pClusterApi->hClusterLibrary = hDll;
	}
	else
	{
		goto ErrorExit;
	}


	hDll = SNILoadSystemLibraryA( "RESUTILS.DLL" );
	if (hDll != NULL)
	{
		if ( !(pClusterApi->ResUtilFindSzProperty = ( PFRESUTILFINDSZPROPERTY) GetProcAddress(hDll,"ResUtilFindSzProperty"))) goto ErrorExit;
		if ( !(pClusterApi->ResUtilResourceTypesEqual = (PFRESUTILRESOURCETYPESEQUAL) GetProcAddress(hDll,"ResUtilResourceTypesEqual"))) goto ErrorExit;

		pClusterApi->hResourceLibrary = hDll;
	}
	else
	{
		goto ErrorExit;
	}
	//success


	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet ;


ErrorExit:
	
	dwRet = GetLastError();

	if( pClusterApi->hClusterLibrary )
	{
		
		FreeLibrary( pClusterApi->hClusterLibrary );
		pClusterApi->hClusterLibrary = NULL;

	}
	
	if( pClusterApi->hResourceLibrary )
	{
		
		FreeLibrary( pClusterApi->hResourceLibrary );
		pClusterApi->hResourceLibrary = NULL;

	}

	SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet ;
	
}



// load cluster and resource library only if the server is running on as a virtual server.
DWORD LoadClusterResourceLibraryIfNeeded( CLUSTER_API* pClusterApi, BOOL *pfCluster )
{

	BidxScopeAutoSNI2( SNIAPI_TAG _T( "pClusterApi: %p{CLUSTER_API*}, pfCluster: %p{BOOL*}\n"), pClusterApi, pfCluster );

	DWORD dwRet = ERROR_FAIL;
	
	*pfCluster = FALSE; 
	SS_NODE_HANDLE hRoot = NULL; 

	//
	//	Obtain cluster information.  
	//
	dwRet = static_cast<DWORD>(SSgetRootHandleEx( Resource->m_szInstanceName, 
												&hRoot,
												FALSE /* fIsSystemInst, System Instance does not run on cluster. */)); 
	if ( ERROR_SUCCESS != dwRet )
	{
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );
		goto ErrorExit; 
	}

	dwRet = static_cast<DWORD>(SSgetClusterSettings( hRoot, 
													 pfCluster, 
													 gwszClusterName, 
													 ARRAYSIZE(gwszClusterName) )); 

	if ( ERROR_SUCCESS != dwRet)
	{
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_SYSTEM, dwRet );
		goto ErrorExit; 
	}
	
	Assert (wcslen( gwszClusterName ) < ARRAYSIZE( gwszClusterName ));

	if( *pfCluster )
	{
		// a cluster, load cluster API
		if ( ERROR_SUCCESS != ( dwRet = LoadClusterResourceLibraries( pClusterApi ) ) )
		{
	
			SNI_SET_LAST_ERROR( TCP_PROV, SNIE_22, dwRet );
			goto ErrorExit;

		}

	}
		

ErrorExit:
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	if( hRoot )
		SSreleaseNodeHandle( hRoot );
	return dwRet ;


}

#endif


// The critical section used here and in SNIResumePendingAccepts 
// and the fact that FRemoveListener one of the first things called
// in SNITerminateListener ensure that we won't have listeners in 
// the list that have been terminated and that a listener can't be
// terminated while we're inside of SNIResumePendingAccepts 
// attempting to resuming accepts.
//
void AddListener (__out SNI_Listener * pListener)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "pListener: %p{SNI_Listener*}\n"), pListener);

	Assert (pListener);
	Assert (pListener->pNext == NULL);

	CAutoSNICritSec a_cs(g_pcsListenerList, SNI_AUTOCS_ENTER);

	// Add the listener to the front of the global list
	//
	pListener->pNext = g_pListenerList;
	g_pListenerList = pListener;
}

// Returns true if the item was found in the list
//
BOOL FRemoveListener (__inout SNI_Listener * pListener)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "pListener: %p{SNI_Listener*}\n"), pListener);

	Assert (pListener);

	CAutoSNICritSec a_cs(g_pcsListenerList, SNI_AUTOCS_ENTER);

	// First check if the this listener is the head of the list
	//
	if (g_pListenerList == pListener)
	{
		g_pListenerList = pListener->pNext;
		pListener->pNext = NULL;

		a_cs.Leave();

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), TRUE);
		return TRUE;
	}
	
	// Otherwise walk thru the list looking for it.
	// This code also handles the case where the list is empty
	//
	for (SNI_Listener * pCurrent = g_pListenerList;
		 pCurrent != NULL;
		 pCurrent = pCurrent->pNext)
	{
		if (pCurrent->pNext == pListener)
		{
			pCurrent->pNext = pListener->pNext;
			pListener->pNext = NULL;

			a_cs.Leave();

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), TRUE);
			return TRUE;
		}
	}

	// If we reached here then we didn't find it.
	//
	a_cs.Leave();

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), FALSE);
	return FALSE;
}

void IncrementConnBufSize(__out SNI_Conn * pConn, SNI_PROVIDER_INFO * pProvInfo)
{	
	BidxScopeAutoSNI2( SNIAPI_TAG _T( "pConn: %p{SNI_Conn*}, pProvInfo: %p{SNI_PROVIDER_INFO*}\n"), 
					pConn, pProvInfo);

	// Increment the provider's buffer size and offsets
	pConn->m_ConnInfo.ProvBufferSize += pProvInfo->Size;
	pConn->m_ConnInfo.ProvOffset += pProvInfo->Offset;

	// Set the IO stack for this connection - this is based on the size of the buffer
	{
		pConn->m_MemTag = SNIMemRegion::GetMemoryTag( pConn->m_ConnInfo.ConsBufferSize
													+ pConn->m_ConnInfo.ProvBufferSize );
	}
}

void DecrementConnBufSize(__out SNI_Conn * pConn, SNI_PROVIDER_INFO * pProvInfo)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T( "pConn: %p{SNI_Conn*}, pProvInfo: %p{SNI_PROVIDER_INFO*}\n"), 
					pConn, pProvInfo);
	
	// Decrement the provider's buffer size and offsets
	pConn->m_ConnInfo.ProvBufferSize -= pProvInfo->Size;
	pConn->m_ConnInfo.ProvOffset -= pProvInfo->Offset;

	// Set the IO stack for this connection - this is based on the size of the buffer
	{
		pConn->m_MemTag = SNIMemRegion::GetMemoryTag( pConn->m_ConnInfo.ConsBufferSize
													+ pConn->m_ConnInfo.ProvBufferSize );
	}
}

SNI_Conn::SNI_Conn() : 
	m_pProvHead(0), 
	m_ConsKey  (0), 
	m_cRefTotal(0), 
	m_pSec     (0),
	m_fSync    (false),
	m_fClient  (false),
	m_fBindingsCheckFailed (false),
	m_pwszServer(0),
	m_pwszOriginalServer(0),
	m_cid      (0),
	m_pEPInfo(0),
	m_dwConnIndex(0),
	m_csProvList(NULL),
	m_lSNIClosePending(0), 
	m_iBidId(0),
	m_pvChannelBindings(NULL)
{
	memset((SNI_CONSUMER_INFO *)&m_ConsumerInfo, 0, sizeof(SNI_CONSUMER_INFO));
	memset((SNI_CONN_INFO *)&m_ConnInfo, 0, sizeof(SNI_CONN_INFO));
	memset(m_rgRefCount, 0, sizeof(m_rgRefCount));
	m_ConnInfo.TransportProvider = INVALID_PROV;
	m_NodeId = SOS_Node::GetCurrent()->GetNodeId();
	memset((void *) &m_Uci, 0, sizeof(SNIUCI));

	BidObtainItemIDA( &m_iBidId, SNI_ID_TAG "%p{.}", this );

	AddRef( REF_Active );
	InterlockedIncrement( &gnConns );
}

DWORD SNI_Conn::InitObject( __out SNI_Conn **ppConn, BOOL fServer)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T( "ppConn: %p{SNI_Conn**}, fServer: %d{BOOL}\n"), 
					ppConn,  fServer);

	DWORD dwError = ERROR_SUCCESS;
	
	*ppConn = NULL;
	
	SNI_Conn *pConn;

	pConn = NewNoX(gpmo) SNI_Conn();

	if( !pConn )
	{
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, ERROR_OUTOFMEMORY );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_OUTOFMEMORY);
		
		return ERROR_OUTOFMEMORY;
	}

	// Initialize the critical section protecting the provider list
	//
	dwError = SNICritSec::Initialize(&pConn->m_csProvList);
	if (ERROR_SUCCESS != dwError)
	{
		pConn->Release(REF_Active);
		
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, dwError );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
		
		return dwError;
	}

	pConn->m_pSec = NewNoX(gpmo) SNI_Sec();

	if( !pConn->m_pSec )
	{
		pConn->Release(REF_Active);

		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, ERROR_OUTOFMEMORY );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_OUTOFMEMORY);
		
		return ERROR_OUTOFMEMORY;
	}

	// Allocate the SNI_EPInfo member - for "Server" end connections only
	if( fServer )
	{
		pConn->m_pEPInfo = NewNoX(gpmo) SNI_EPInfo;

		if( !pConn->m_pEPInfo )
		{
			pConn->Release(REF_Active);

			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, ERROR_OUTOFMEMORY );

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_OUTOFMEMORY);
			
			return ERROR_OUTOFMEMORY;
		}
	}

	if(ERROR_SUCCESS != (dwError = pConn->m_ConnInfo.Timer.FInit()) )
	{
		pConn->Release(REF_Active);

		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, dwError );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
		
		return dwError;
	}

	// Generating guid using UuidCreate is sni behavior,
	// all the other components which use sni all follow this step
	dwError = (DWORD) UuidCreate(&pConn->m_Uci.Id);
	
	if( ERROR_SUCCESS != dwError )
	{
		pConn->Release(REF_Active);

		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, dwError );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
		
		return dwError;
	}

	pConn->m_dwConnIndex = InterlockedIncrement((LONG *) &SNI_Conn::iSniConnIndex);
	pConn->m_dwConnIndex = pConn->m_dwConnIndex%MAX_PSM_ARRAY;
	SNI_Conn::rgSniConn[pConn->m_dwConnIndex] = pConn;
	
	*ppConn = pConn;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

	return ERROR_SUCCESS;
}

SNI_Conn::~SNI_Conn()
{
	Assert( !m_cRefTotal );

	BidRecycleItemIDA( &m_iBidId, SNI_ID_TAG ); 

	//	There are paths that do not set these four member variables before destructor is called
	//
	if (m_pSec)
	{
		delete m_pSec;
	}
	if( m_pwszServer )
	{
		delete [] m_pwszServer;
		m_pwszServer = NULL;
	}
	if( m_pwszOriginalServer )
	{
		delete [] m_pwszOriginalServer;
		m_pwszOriginalServer = NULL;
	}
	if( m_pEPInfo )
	{
		delete m_pEPInfo; 
	}

	// There are code paths where we might close the connection after establishing an SSL channel but before completing SSPI handshake
	// In those cases, we need to make sure we properly clean up the Channel Bindings buffer tied to the connection
	ReleaseChannelBindings();

	InterlockedDecrement( &gnConns );

	rgSniConn[m_dwConnIndex] = NULL;

	// Delete the critical section protecting the provider list
	//
	if(m_csProvList)
	{
		DeleteCriticalSection(&m_csProvList);
	}
}
	
LONG SNI_Conn::AddRef( SNI_REF refType )
{
	Assert( refType < REF_Max );

	LONG cRef;

	cRef = InterlockedIncrement( &m_rgRefCount[ refType ] );

	Assert( 0 < cRef );
	
	cRef = InterlockedIncrement( &m_cRefTotal );

	Assert( 0 < cRef );

	return cRef;
}

LONG SNI_Conn::Release( SNI_REF refType )
{
	Assert( refType < REF_Max );

	LONG cRefType = InterlockedDecrement( &m_rgRefCount[ refType ] );

	Assert( 0 <= cRefType );
	
	// SQL Hotifx 50001738:
	// Delete the SSPI structures if REF_Active reached 0 now.  Currently this
	// avoids deleting them on an IOCP thread because there is no case where 
	// REF_Active is decremented on IOCP thread and the structures are present.  
	// It also avoids race conditions during their deletion since those 
	// structures are currently accessed only from functions that require 
	// REF_Active count to be non-zero.  
	// It does not introduce memory leaks due to deletion of SNI_Conn without 
	// deleting the SSPI structures since the only place SNI_Conn DTOR is 
	// called is this Release function when all ref's are 0.  
	//
	if( ( 0 == cRefType ) && ( REF_Active == refType ) && ( NULL != m_pSec ) )
	{
		m_pSec->DeleteSspi(); 		
	}

	LONG cRef = InterlockedDecrement( &m_cRefTotal );

	Assert( 0 <= cRef );

	if( 0 == cRef )
	{
		Assert( !m_rgRefCount[ REF_Active ] );
		Assert( !m_rgRefCount[ REF_InternalActive] );
		Assert( !m_rgRefCount[ REF_Packet ] );
		Assert( !m_rgRefCount[ REF_Read ] );
		Assert( !m_rgRefCount[ REF_InternalRead] );
		Assert( !m_rgRefCount[ REF_Write ] );
		Assert( !m_rgRefCount[ REF_InternalWrite] );
		Assert( !m_rgRefCount[ REF_ActiveCallbacks ] );
		Assert( !m_rgRefCount[ REF_PacketNotOwningBuf ] );

		if(m_pProvHead)
			m_pProvHead->Release();

		delete this;
	}
	
	return cRef;
}

void SNI_Conn::ReleaseChannelBindings()
{
	if (NULL != m_pvChannelBindings)
	{
		// Release the Crypto structures stored in the opaque pointer, if any
		Ssl::ReleaseChannelBindings(m_pvChannelBindings);

		// Release the opaque pointer itself
		delete m_pvChannelBindings;
		m_pvChannelBindings = NULL;
	}
}

DWORD SNI_Conn::SetServerName( __in __nullterminated WCHAR *wszServer, __in __nullterminated WCHAR *wszOriginalServerName)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T( "wszServer: \"%s\", wszOriginalServerName: \"%s\"\n"), wszServer, wszOriginalServerName);
	
	Assert( m_fClient );

	DWORD dwError = AllocAndSetName(&m_pwszServer, wszServer, true);
	if( ERROR_SUCCESS != dwError )
	{
		goto ErrorExit;
	}

	dwError = AllocAndSetName(&m_pwszOriginalServer, wszOriginalServerName, false);
	if( ERROR_SUCCESS != dwError )
	{
		goto ErrorExit;
	}
	goto Exit;

ErrorExit:

	if( NULL != m_pwszServer )
	{
		delete []m_pwszServer;
		m_pwszServer = NULL;
	}

	if( NULL != m_pwszOriginalServer )
	{
		delete []m_pwszOriginalServer;
		m_pwszOriginalServer = NULL;
	}

Exit:
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	return dwError;
}

DWORD SNI_Conn::AllocAndSetName(__out WCHAR **pwszTarget, __in __nullterminated WCHAR *wszSource, bool fFailEmptySource)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T( "pwszTarget: %p{WCHAR**}, ")
					_T("wszSource: \"%s\", ")
					_T("fFailEmptySource: %d{bool}\n"),
					pwszTarget, 
					wszSource,
					fFailEmptySource);
	DWORD dwError;

	int cchWideChar;
	size_t cchSourceLen;

	if( ! *wszSource && fFailEmptySource )
	{
		//this assertion is ued to catch unexpected coding errors.
		Assert( 0 && "wszSource : empty string encounted\n" );
		BidTrace0( ERROR_TAG _T("wszSource : empty string encounted\n") );
		dwError = ERROR_INVALID_PARAMETER;
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, dwError );
		goto ErrorExit;
	}

	if (FAILED (StringCchLengthW(wszSource, MAX_NAME_SIZE, &cchSourceLen)))
	{
		dwError = ERROR_INVALID_PARAMETER;

		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwError );

		goto ErrorExit;
	}

	// account for the null terminator at the end
	cchSourceLen++;
	cchWideChar = (int) cchSourceLen;
	Assert (cchSourceLen <= MAX_NAME_SIZE + 1);
	Assert( NULL == *pwszTarget );
	*pwszTarget = NewNoX(gpmo) WCHAR[cchWideChar];
	if( NULL == *pwszTarget )
	{
		dwError = ERROR_OUTOFMEMORY;

		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_4, dwError );

		goto ErrorExit;
	}
	
	if (S_OK != StringCchCopyW(*pwszTarget, cchWideChar, wszSource))
	{
		dwError = GetLastError();

		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwError );

		goto ErrorExit;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;

ErrorExit:

	BidTrace1( ERROR_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}

void SNI_Conn::WaitForActiveCallbacks()
{
	// SNIX calls this function from SNIClose.  We may wait here to prevent the connection's appdomain 
	// from shutting down while managed callbacks are active on the connection.  SNIX SNIReadDone and 
	// SNIWriteDone addref REF_ActiveCallbacks before the callback, and release REF_ActiveCallbacks
	// when the callback returns.

	// First block further callbacks on this connection by setting m_lSNIClosePending = 1.
	// This will stop SNIReadDone and SNIWriteDone from making more callbacks on the connection.
	LONG lClosePending = InterlockedCompareExchange(&m_lSNIClosePending,1,0);
	Assert ( 0 == lClosePending );

	// Spin here and wait for all REF_ActiveCallbacks to drain. The period of time that REF_ActiveCallbacks > 0 
	// should be short, the time it takes to queue a work request to a managed thread.
	while ( InterlockedCompareExchange(&m_rgRefCount[REF_ActiveCallbacks],0,0) > 0 )
	{
		Sleep(100);
	}
}

bool SNI_Conn::FAllowCallback()
{
	// Called by SNIX SNIReadDone and SNIWriteDone to check on the callback state of the connection.  
	// When WaitForActiveCallbacks is called from SNIClose, m_lSNIClosePending is set to 1 to 
	// stop further callbacks on the connection.
	return ( 0 == InterlockedCompareExchange(&m_lSNIClosePending,0,0) );
}

void SNI_Conn::SafeReadComp( SNI_Packet* pPacket, DWORD dwProvError )
{
	// Called by SNIX SNIReadDone to perform safe call to consumer read completion routine.
	// The try+catch is needed because the app domain may invalidate the callback during shutdown
	// and when this happens the callback raises a C++ exception.
	SNI_TRY
	{
		m_ConsumerInfo.fnReadComp( m_ConsKey, pPacket, dwProvError );
	}
	SNI_CATCH
	{
		BidTrace0( ERROR_TAG _T("Exception during read completion.\n") );
	}
	SNI_ENDCATCH
}

void SNI_Conn::SafeWriteComp( SNI_Packet* pPacket, DWORD dwProvError )
{
	// Called by SNIX SNIWriteDone to perform safe call to consumer write completion routine.
	// The try+catch is needed because the app domain may invalidate the callback during shutdown
	// and when this happens the callback raises a C++ exception.	
	SNI_TRY
	{
		m_ConsumerInfo.fnWriteComp( m_ConsKey, pPacket, dwProvError );
	}
	SNI_CATCH
	{
		BidTrace0( ERROR_TAG _T("Exception during write completion.\n") );
	}
	SNI_ENDCATCH
}

LONG gcSNIInitialized = 0;

#ifdef SNI_BASED_CLIENT
SNICritSec *g_csInitialize;		//to serialize initialize/terminate
extern SNICritSec *g_csSecPackageInitialize; //to serialize init/term for security package.
extern SNICritSec *g_csLocalDBInitialize;  // to serialize init for Local DB

#ifdef SNIX
ISOSHost * g_pISOSHost	 = NULL; 	// Global pHost
ISOSHost_MemObj * g_pMO	 = NULL; 	// Global memory object
#endif

SNIMemRegion * SNIMemRegion::s_rgClientMemRegion = 0;
SOS_Node SOS_Node::s_ClientCurrentNode;
SOS_Task SOS_Task::s_ClientTask;

#ifndef SNIX

BOOL DllMain_Sni( HINSTANCE  hInst, 	  
                       DWORD      dwReason,    
                       LPVOID     lpReserved )  
{
	SNI_ERROR * pSNIerror;

	switch( dwReason )
	{
        case DLL_PROCESS_ATTACH:

			// Are we running under NT or Win9x?
			OSVERSIONINFO VersionInformation;
			VersionInformation.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );

			if( GetVersionEx(&VersionInformation) ==  TRUE )
			{
				if( VersionInformation.dwPlatformId == VER_PLATFORM_WIN32_NT )
					gfIsWin9x = FALSE;
				else
					gfIsWin9x = TRUE;

				if (5 <= VersionInformation.dwMajorVersion)
					gfIsWin2KPlatform = TRUE;
				else
					gfIsWin2KPlatform = FALSE;
			}
	
			if (!g_pISOSHost || !g_pMO)
			{			
				return FALSE;
			}
			gpmo = g_pMO;

			if( ERROR_SUCCESS != SNIErrorTlsInit() )
			{
				return FALSE; 
			}

			DWORD dwError;

			dwError = SNICritSec::Initialize(&g_csInitialize);
			
			if(ERROR_SUCCESS != dwError )
			{
				SetLastError( dwError );
				
				return FALSE;
			}

			dwError = SNICritSec::Initialize(&g_csSecPackageInitialize);			
			
			if(ERROR_SUCCESS != dwError )
			{
				DeleteCriticalSection(&g_csInitialize);
				SetLastError( dwError );
				
				return FALSE;
			}

        break;

        case DLL_PROCESS_DETACH:
			DeleteCriticalSection(&g_csInitialize);
			DeleteCriticalSection(&g_csSecPackageInitialize);

			SNIErrorTlsTerm(); 
			
			//AFTER THIS POINT, MEMORY APIS SHOULD NOT BE USED.
			//DONT CALL NEW DELETE. 
			//APIS THAT DIRECTLY GO TO PROCESS HEAP ARE OK.

			break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
			break;
	}
	return TRUE;
}

#else	// #ifndef SNIX

DWORD SNIxInitialize()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );

	DWORD dwError = ERROR_SUCCESS;
	
	// Are we running under NT or Win9x?
	OSVERSIONINFO VersionInformation;
	VersionInformation.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );

	if( GetVersionEx(&VersionInformation) ==  TRUE )
	{
		if( VersionInformation.dwPlatformId == VER_PLATFORM_WIN32_NT )
			gfIsWin9x = FALSE;
		else
			gfIsWin9x = TRUE;
	}


	dwError = SNIErrorTlsInit(); 

	if( ERROR_SUCCESS != dwError )
	{
		BidTrace1( ERROR_TAG _T("SNIErrorTlsInit failed: %d{WINERR}\n"), dwError);
		return dwError;
	}

	dwError = SNICritSec::Initialize(&g_csInitialize);

	if(ERROR_SUCCESS != dwError )
	{
		BidTrace1( ERROR_TAG _T("Initialize g_csInitialize failed: %d{WINERR}\n"), dwError);
		return dwError;
	}

	dwError = SNICritSec::Initialize(&g_csSecPackageInitialize);

	if(ERROR_SUCCESS != dwError )
	{
		DeleteCriticalSection(&g_csInitialize);
		BidTrace1( ERROR_TAG _T("Initialize g_csSecPackageInitialize failed: %d{WINERR}\n"), dwError);
		return dwError;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	return dwError;
}

#endif	// #ifndef SNIX

inline DWORD SNIRegisterWithIOCP(HANDLE hNwk)
{
	if( NULL == CreateIoCompletionPort( hNwk, ghIoCompletionPort, 0, 0) )
	{
		DWORD dwError = GetLastError();
		
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_9, dwError );

		return dwError;
	}

	return ERROR_SUCCESS;
}

#else	// #ifdef SNI_BASED_CLIENT

inline DWORD SNIRegisterWithIOCP(HANDLE hNwk)
{
	DWORD dwError;

	dwError = SOS_Node::RegisterWithIOCompletionPort(hNwk);
	
	if( SOS_OK != dwError )
	{
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_9, dwError );
	}

	return dwError;
}

DWORD SNINodeInitRoutine()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n"));

	Assert(!gpmo);

	DWORD dwRet;

	SOSHost 		* pHost = 0;
	IMemObj 		* pmo = 0;
	SNIMemRegion 	* pMemRegion = 0;
	
	//create a host for SNI. Using this we will request critsecs and other objects
	HRESULT hr = SOS_OS::CreateSOSHostObject (SOSHOST_CLIENTID_SERVERSNI, L"SNI", &pHost);
	Assert(SUCCEEDED (hr));
	if(FAILED(hr))
	{
		dwRet = hr;
		goto ErrorExit;
	}		

	Assert( pHost );

	SOS_Node::GetCurrent()->StoreObject(NLS_SNI_HOST, pHost);


	pmo = PmoNewMemObjNoX (	MEMOBJ_SNI, 
							bThreadSafe |bPageHeap , 
							GetMemoryClerk (MEMORYCLERK_SNI));

	if(NULL == pmo )
	{
		// We cannot call SNI_SET_LAST_ERROR() because it relies on the gpmo being
		// established.  In this case, we failed to create it.
		//
		dwRet = ERROR_OUTOFMEMORY;
		goto ErrorExit;
	}

	SOS_Node::GetCurrent()->StoreObject(NLS_SNI_MEMOBJ, pmo);

	Assert( pmo == gpmo ); 

	// Initialize memory regions
	// Note: This needs to be done first, sicne here is where the
	// memory allocators are defined.This is to take care of the fact
	// that other functions mite be allocating memory

	pMemRegion = SNIMemRegion::Init();

	if(NULL == pMemRegion )
	{
		dwRet = ERROR_OUTOFMEMORY;
		
		goto ErrorExit;
	}
	
	SOS_Node::GetCurrent()->StoreObject(NLS_SNI_CACHE, pMemRegion);

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

	return ERROR_SUCCESS;
	
ErrorExit:
	
	if ( pmo )
	{
		// Remove the memory object from the Node Local Storage.  
		SOS_Node::GetCurrent()->StoreObject(NLS_SNI_MEMOBJ, NULL);
		Assert( NULL == gpmo ); 

		// Release the memory object.  
		ULONG cRefCount = pmo->Release(); 
		Assert( !cRefCount ); 
	}

	if ( pHost )
	{
		SOS_Node::GetCurrent()->StoreObject(NLS_SNI_HOST, NULL);

		ULONG cRefCount = pHost->Release();
		Assert( !cRefCount ); 

		pHost = NULL;
	}

	if ( pMemRegion )
	{
		SOS_Node::GetCurrent()->StoreObject(NLS_SNI_CACHE, NULL);

		delete [] pMemRegion;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;
}

#endif	// #ifdef SNI_BASED_CLIENT

DWORD_PTR g_NodeMask = 0;

void MakeNodeMask()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n"));

	BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("old g_NodeMask: %p\n") , g_NodeMask );
	
	Assert( !g_NodeMask );
	
	SOS_NodeEnum enumNode;

	SOS_Node *pNode;

	for( pNode = enumNode.GetNext(NULL); pNode; pNode = enumNode.GetNext(pNode))
	{
		Assert( !pNode->IsDACNode());
		
		g_NodeMask|= static_cast<Affinity>(1)<<pNode->GetNodeId();
	}	

	Assert( 0 == (g_NodeMask & (g_NodeMask+1)));

	BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("new g_NodeMask: %p\n") , g_NodeMask );
}

BOOL FVistaSP1orLater()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n"));
	
	OSVERSIONINFOEX osvi;
	DWORDLONG dwlConditionMask = 0;
	BYTE op=VER_GREATER_EQUAL;

	// Initialize the OSVERSIONINFOEX structure.

	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	osvi.dwMajorVersion = 6;
	osvi.dwMinorVersion = 0;
	osvi.wServicePackMajor = 1;
	osvi.wServicePackMinor = 0;

	// Initialize the condition mask.

	VER_SET_CONDITION( dwlConditionMask, VER_MAJORVERSION, op );
	VER_SET_CONDITION( dwlConditionMask, VER_MINORVERSION, op );
	VER_SET_CONDITION( dwlConditionMask, VER_SERVICEPACKMAJOR, op );
	VER_SET_CONDITION( dwlConditionMask, VER_SERVICEPACKMINOR, op );

	// Perform the test.

	return VerifyVersionInfo(
					&osvi, 
					VER_MAJORVERSION | VER_MINORVERSION | 
					VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
					dwlConditionMask);
}


DWORD SNIInitializeEx(void * pmo, 
					  const ProviderNum * rgProviders, 
					  DWORD cProviders,
					  BOOL  fIsSystemInst,
					  BOOL  fSandbox)
{
	BidxScopeAutoSNI4( SNIAPI_TAG _T("pmo: %p, ")
					  	_T("rgProviders: %p{ProviderNum*}, ")
					 	_T("cProviders: %d, ")
					  	_T("fIsSystemInst: %d{BOOL}\n"), 
					 	pmo,
					 	rgProviders,
					 	cProviders,
					 	fIsSystemInst);
	
	DWORD dwError = ERROR_SUCCESS;
	
	//we dont expect anyone to pass us pmo here.
	//we create our own pmo in case of server. For the client case, we get it it DllMain
	Assert( pmo == NULL );

	// Check if SNI was previously initialized - in that case do nothing
#ifdef SNI_BASED_CLIENT

#ifdef SNIX

	while( !g_fInitDone )
	{
		if( !InterlockedCompareExchange(&g_dwInitLock, 1, 0) )
		{
			if( !g_fInitDone )
			{
				if( ERROR_SUCCESS != (dwError = SNIxInitialize()) )
				{
					InterlockedCompareExchange(&g_dwInitLock, 0, 1);
					return dwError;
				}

				g_fInitDone = TRUE;
			}

			int x = InterlockedCompareExchange(&g_dwInitLock, 0, 1);
			Assert(x == 1);
		}
	}

#endif	// #ifdef SNIX


	CAutoSNICritSec a_csInitialize( g_csInitialize, SNI_AUTOCS_DO_NOT_ENTER );

	a_csInitialize.Enter();

	if( 1 != InterlockedIncrement(&gcSNIInitialized))
	{
		goto Exit;
	}

#ifndef SNIX

	Assert(g_pMO);			
	gpmo = g_pMO;

#endif	// #ifndef SNIX
	
	g_fSandbox = fSandbox;

	if( !g_fSandbox )
	{
		LastConnectCache::Initialize();
	}

	Assert( NULL == SNIMemRegion::s_rgClientMemRegion );
	
	SNIMemRegion::s_rgClientMemRegion = SNIMemRegion::Init();

	if( NULL == SNIMemRegion::s_rgClientMemRegion )
	{
		dwError = ERROR_OUTOFMEMORY;

		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, dwError );

		goto ErrorExit;
	}

	dwError = SNICritSec::Initialize(&g_csLocalDBInitialize);
	
	if(ERROR_SUCCESS != dwError )
	{
		BidTrace1( ERROR_TAG _T("Initialize g_csLocalDBInitialize failed: %d{WINERR}\n"), dwError);
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, dwError );
		goto ErrorExit;
	}
	
#else	// #ifdef SNI_BASED_CLIENT

	if(1 != InterlockedIncrement(&gcSNIInitialized))
	{
		// this assertion is used to catch coding erorrs.
		Assert( 0 && " gcSNIInitialized is not true\n" );
		BidTrace0( ERROR_TAG _T("gcSNIInitialized is not true\n") );
		goto Exit;
	}

#endif	// #ifdef SNI_BASED_CLIENT

	MakeNodeMask();

	// Get the computer name
	DWORD cgszComputerName = MAX_NAME_SIZE;
	
	if( !GetComputerName(gwszComputerName, &cgszComputerName) )
	{
		dwError = GetLastError();

		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwError );

		goto ErrorExit;
	}

	g_osviSNI.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	if( !GetVersionEx( &g_osviSNI ) )
	{
		dwError = GetLastError();

		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwError );

		goto ErrorExit;
	}
	
	if(gfIsWin2KPlatform && FVistaSP1orLater())
	{
		BidTraceU0( SNI_BID_TRACE_ON, SNI_TAG _T("Vista SP1 or later.\n"));		 
		Tcp::s_fAutoTuning = TRUE;
	}

	// Initialize the critical section protecting the provider list
	//
	dwError = SNICritSec::Initialize(&g_pcsListenerList);
	if (ERROR_SUCCESS != dwError)
	{
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwError );

		goto ErrorExit;
	}
	Assert( NULL == g_pListenerList );

#ifdef SNI_BASED_CLIENT	
	g_fTerminate = false;

	// Create IOCP only if its NOT Win9x
	if( !gfIsWin9x )
	{
		// Create the completion port
		ghIoCompletionPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE,
												     NULL,
												     0,
												     0 );
		if( !ghIoCompletionPort )
		{
			dwError = GetLastError();

			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwError );

			goto ErrorExit;
		}
		
		// Start at least one AsyncWait thread 
		// Note: We need to do this before init'ing providers, since some mite interact with 
		// the IOCP during their init procedures

		dwError = SNICreateWaitThread(SNIAsyncWait, NULL);
		
		if( ERROR_SUCCESS != dwError )
		{
			goto ErrorExit;
		}
	}
#endif	// #ifdef SNI_BASED_CLIENT

#if !defined(SNI_BASED_CLIENT) && !defined(STANDALONE_SNI)
	gClusApi.hClusterLibrary = NULL;
	gClusApi.hResourceLibrary = NULL;

	// System Instance does not run on cluster. 
	BOOL fCluster = FALSE;
	if( !fIsSystemInst && (ERROR_SUCCESS != (dwError = LoadClusterResourceLibraryIfNeeded(&gClusApi, &fCluster)) ) ) 
	{
		goto ErrorExit;
	}

	if( fCluster )
	{
		//gwszClusterName was set by LoadClusterResourceLibraryIfNeeded, so we can now set it for SNI_ServiceBindings
		dwError = SNI_ServiceBindings::SetClusterNames(gwszClusterName);
		if( ERROR_SUCCESS != dwError )
		{
			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwError );

			goto ErrorExit;
		}
	}
#endif


	// Initialize the providers
	SNI_Provider::InitProviders(rgProviders, cProviders);

#ifdef SNI_UNIT
	SNIUnitTestInitialize();
#endif

	// We succeeded, hence, exit.  
	goto Exit; 


ErrorExit:

	//	Roll back any partial initialization in case of failure.  

	// SNI_ServiceBindings protects itself from incorrect release.
	SNI_ServiceBindings::Release();

	if( g_pcsListenerList )
	{
		DeleteCriticalSection( &g_pcsListenerList );
	}

#ifdef SNI_BASED_CLIENT	

	if(g_csLocalDBInitialize)
	{
		DeleteCriticalSection( &g_csLocalDBInitialize );
	}
	
	if ( ghIoCompletionPort )
	{
		CloseHandle( ghIoCompletionPort );
		ghIoCompletionPort = NULL; 
	}

	if( NULL != SNIMemRegion::s_rgClientMemRegion )
	{
		SNIMemRegion::Terminate( SNIMemRegion::s_rgClientMemRegion );

		SNIMemRegion::s_rgClientMemRegion = NULL;
	}

	LastConnectCache::Shutdown();
	
#endif	// #ifdef SNI_BASED_CLIENT

	g_NodeMask = 0;

	// Revert our ref count increment
	InterlockedDecrement( &gcSNIInitialized );

	// Fall through into Exit
Exit:

#ifdef SNI_BASED_CLIENT	
	a_csInitialize.Leave(); 
#endif	// #ifdef SNI_BASED_CLIENT

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	return dwError;
}

DWORD SNIInitialize(void * pmo)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("pmo: %p\n"), pmo );
	
	DWORD dwError = SNIInitializeEx(
		pmo, 
		NULL,	// rgProviders - Initialize all providers 
		0, 		// cProviders - Initialize all providers
		FALSE,  // not a system instance because it never initializes all providers 
		FALSE   /* fSandbox - Sets the value of the global variable g_fSandbox */);
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	return dwError;
}

// 
DWORD SNITerminate()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );
	
	DWORD dwError;
	
#ifdef SNI_BASED_CLIENT		

	CAutoSNICritSec a_csInitialize( g_csInitialize, SNI_AUTOCS_DO_NOT_ENTER );

	a_csInitialize.Enter();
	
	// Terminate only for the last Initialize call - i.e. when refcount is 0
	if( 0 != InterlockedDecrement( &gcSNIInitialized ))
	{
		dwError = ERROR_SUCCESS;
		
		goto Exit;
	}

#else	// #ifdef SNI_BASED_CLIENT
	if(0 != InterlockedDecrement( &gcSNIInitialized ))
	{
		//this assertion is used to catch coding errors.
		Assert( 0 && " gcSNIInitialized is not false\n" );
		BidTrace0( ERROR_TAG _T("gcSNIInitialized is not false\n") );
		
		dwError = ERROR_INVALID_STATE;		
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwError );

		goto Exit;
	}
#endif	// #ifdef SNI_BASED_CLIENT

#ifdef GOLDEN_BITS
#ifdef SNI_UNIT
	SNIUnitTestTerminate();
#endif
#endif

	// Safe to clean up SNI_ServiceBindings even if they were not successfully initialized.
	SNI_ServiceBindings::Release();

	//
	// Give worker thread a chance to drain connections
	// server: 1 minutes
	// client: 1/6 miniutes
	// managed code: 1/40 minutes
#ifdef SNI_BASED_CLIENT		
	// Per Microsoft, an AppDomain unload is interrupted
	// after ~2 seconds, so putting a shorter wait 
	// for SNIX.  
#ifdef SNIX
	for(int i=0; i<15 && 0 != gnConns; i++)
	{
		Sleep(100);
	}
#else
	for(int i=0; i<10 && 0 != gnConns; i++)
	{
		Sleep(1000);
	}
#endif
#else
	//
	// Wait 1 minute on the server side.  
	//
	for(int i=0; i<60 && 0 != gnConns; i++)
	{
		Sleep(1000);
	}
#endif

	// Note: Since the above wait is 10 seconds, we could
	// potentially leak connections if any have not been
	// cleaned out within the specified time window
	if( 0 != gnConns )
	{
		// If all the connections are not cleaned out, exit from here.
		// Otherwise, we would encounter issues with I/O happening
		// on the connections, while some providers are getting cleaned out
#ifdef SNIX
		// Free the cached packets to reduce the memory we keep around.  
		if( NULL != SNIMemRegion::s_rgClientMemRegion )
		{
			SNIMemRegion::Flush( SNIMemRegion::s_rgClientMemRegion );
		}
		else
		{
			//this assertion is used to catch coding errors.
			Assert( 0 && " SNIMemRegion::s_rgClientMemRegion is NULL\n" );
			BidTrace0( ERROR_TAG _T("SNIMemRegion::s_rgClientMemRegion is NULL\n") );
		}
#endif
		//Assert(0);    Temperarily disable to unblock the AB. (SQL BU 324791)

		InterlockedIncrement( &gcSNIInitialized );
			
		scierrlog(26034);		

		dwError = ERROR_INVALID_STATE;
		
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwError );

		goto Exit;
	}

#ifdef SNI_BASED_CLIENT

	LastConnectCache::Shutdown();

	LocalDB::Terminate();

	if(g_csLocalDBInitialize)
	{
		DeleteCriticalSection( &g_csLocalDBInitialize );
	}
	
#endif
	
	SNI_Provider::Terminate();

#ifdef SNI_BASED_CLIENT		

	// Cleanup IOCP only if its NOT Win9x
	if( !gfIsWin9x )
	{
		Assert( ghIoCompletionPort );

		Assert( !g_fTerminate );
		g_fTerminate = true;

		// We create only one WaitThread - so do a PostQCS to indicate
		// we are shutting down
		PostQueuedCompletionStatus( ghIoCompletionPort, 0, 0, 0);
	}

	if( gnWorkerThreadCount )
	{
		dwError = WaitForMultipleObjects( gnWorkerThreadCount, rghWorkerThreads, TRUE, INFINITE);

		if( WAIT_FAILED == dwError )
		{
			dwError = GetLastError();
			
			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwError );
			Assert( 0);
		}
		
		for( DWORD i=0; i<gnWorkerThreadCount; i++)
			CloseHandle( rghWorkerThreads[i]);

		gnWorkerThreadCount = 0;
	}

	if( !gfIsWin9x )
	{
		CloseHandle( ghIoCompletionPort );
		ghIoCompletionPort = NULL; 
	}

	if( NULL != SNIMemRegion::s_rgClientMemRegion )
	{
		SNIMemRegion::Terminate( SNIMemRegion::s_rgClientMemRegion );

		SNIMemRegion::s_rgClientMemRegion = NULL;
	}
	else
	{
		//This assertion is used to catch unexpected coding error.
		Assert( 0 && " SNIMemRegion::s_rgClientMemRegion is NULL\n" );
		BidTrace0( ERROR_TAG _T("SNIMemRegion::s_rgClientMemRegion is NULL\n") );
	}

	g_NodeMask = 0;
	
#endif	// #ifdef SNI_BASED_CLIENT

	if( g_pcsListenerList )
	{
		DeleteCriticalSection( &g_pcsListenerList );
	}
	Assert( NULL == g_pListenerList );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	dwError = ERROR_SUCCESS;

Exit:
#ifdef SNI_BASED_CLIENT	
	a_csInitialize.Leave(); 
#endif	// #ifdef SNI_BASED_CLIENT

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	return dwError;
}

DWORD SNIInitializeListener(__in SNI_CONSUMER_INFO * pConsumerInfo, __out HANDLE * pListenHandle)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T( "pConsumerInfo: %p{SNI_CONSUMER_INFO*}, pListenHandle: %p{HANDLE*}\n"), 
					pConsumerInfo, pListenHandle);
	
	DWORD dwError;

	// Create a new listener handle
	SNI_Listener * pListener = NewNoX(gpmo) SNI_Listener;

	if( !pListener )
	{
		dwError = ERROR_OUTOFMEMORY;
		
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_4, dwError );

		goto ErrorExit;
	}
	
	pListener->hTerminateListenerEvent = 0;
	pListener->rgProvListenHandles = 0;
	pListener->rgProviderNums = 0; 
	
	// Create local copy of listener
	pListener->ConsumerInfo.DefaultUserDataLength = pConsumerInfo->DefaultUserDataLength;
	pListener->ConsumerInfo.fnReadComp            = pConsumerInfo->fnReadComp;
	pListener->ConsumerInfo.fnWriteComp           = pConsumerInfo->fnWriteComp;
	pListener->ConsumerInfo.fnAcceptComp          = pConsumerInfo->fnAcceptComp;
	pListener->ConsumerInfo.fnTrace               = pConsumerInfo->fnTrace;

	pListener->ConsumerInfo.NodeAffinity          = pConsumerInfo->NodeAffinity;

	Assert( 	!SOS_Node::GetCurrent()->IsDACNode() || 
				!pConsumerInfo->NodeAffinity);

	pListener->ConsumerInfo.dwNumProts            = 0;
	pListener->ConsumerInfo.rgListenInfo          = 0; 
	pListener->pNext = NULL;

	pListener->rgProviderNums = NewNoX(gpmo) ProviderNum[pConsumerInfo->dwNumProts];

	if( !pListener->rgProviderNums )
	{
		dwError = ERROR_OUTOFMEMORY;

		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_4, dwError );

		goto ErrorExit;
	}

	// Create provider handle array
	pListener->rgProvListenHandles = NewNoX(gpmo) HANDLE[pConsumerInfo->dwNumProts];
	if( !pListener->rgProvListenHandles )
	{
		dwError = ERROR_OUTOFMEMORY;

		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_4, dwError );

		goto ErrorExit;
	}
	
	pListener->cRef = 1;

	pListener->hTerminateListenerEvent = CreateEvent( 0, TRUE, FALSE, 0);
	
	if( !pListener->hTerminateListenerEvent )
	{
		dwError = GetLastError();

		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwError );

		goto ErrorExit;
	}

	// Begin listening on requested protocols
	for(DWORD i = 0; i < pConsumerInfo->dwNumProts; i++ )
	{
		pListener->rgProviderNums[i] = pConsumerInfo->rgListenInfo[i].Provider;

		// Call Listen for each Protocol
		// Note: Should they be static functions
		switch(pListener->rgProviderNums[i])
		{
			case SSL_PROV:
					dwError = Ssl::InitializeListener( (HANDLE) pListener,
												pConsumerInfo->rgListenInfo[i].pSslListenInfo,
												&pListener->rgProvListenHandles[i]);

				break;
				
			case SM_PROV:	
			case NP_PROV:
					dwError = Np::InitializeListener( (HANDLE) pListener,
												pListener->rgProviderNums[i],
												pConsumerInfo->rgListenInfo[i].pNpListenInfo,
												&pListener->rgProvListenHandles[i]);
				break;

			case TCP_PROV:
					dwError = Tcp::InitializeListener( (HANDLE) pListener,
												pConsumerInfo->rgListenInfo[i].pTcpListenInfo,
												&pListener->rgProvListenHandles[i]);
				break;

#ifdef USE_HTTP_PROVIDER
			case HTTP_PROV:
					dwError = Http::InitializeListener( (HANDLE) pListener,
												NULL,
												&pListener->rgProvListenHandles[i]);
				break;
#endif

			case VIA_PROV:	
					dwError = Via::InitializeListener( (HANDLE) pListener,
												pConsumerInfo->rgListenInfo[i].pViaListenInfo,
												&pListener->rgProvListenHandles[i]);
				break;

			default:
				dwError = ERROR_INVALID_PARAMETER;

				SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwError );
		}

		if( ERROR_SUCCESS != dwError )
		{
			SNITerminateListener( pListener );
			pListener = NULL;
			goto ErrorExit;
		}

		pListener->ConsumerInfo.dwNumProts++;
	}

	AddListener (pListener);

	*pListenHandle = (HANDLE) pListener;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("ListenHandle: %p\n"), pListenHandle);
	
	return ERROR_SUCCESS;

ErrorExit:

	if( pListener )
	{
		if( pListener->rgProviderNums )
			delete [] pListener->rgProviderNums;
		
		if( pListener->rgProvListenHandles )
			delete [] pListener->rgProvListenHandles;

		if( pListener->hTerminateListenerEvent )
			CloseHandle( pListener->hTerminateListenerEvent );

		delete pListener;
	}
	
	*pListenHandle = NULL;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}

DWORD SNITerminateListener(__inout HANDLE hListener)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "hListener: %p{HANDLE}\n"), hListener);
	
	DWORD dwError;
	
	SNI_Listener * pListener = (SNI_Listener *)hListener;

	FRemoveListener (pListener);
	
	// Terminate the listeners
	for(DWORD i = 0; i < pListener->ConsumerInfo.dwNumProts; i++ )
	{
		switch(pListener->rgProviderNums[i])
		{
			case SSL_PROV:
					dwError = Ssl::TerminateListener(pListener->rgProvListenHandles[i]);
					break;
				
			case TCP_PROV:
					dwError = Tcp::TerminateListener(pListener->rgProvListenHandles[i]);
					break;

#ifdef USE_HTTP_PROVIDER
			case HTTP_PROV:
					dwError = Http::TerminateListener(pListener->rgProvListenHandles[i]);
					break;
#endif
				
			case SM_PROV:	
			case NP_PROV:	
					dwError = Np::TerminateListener(pListener->rgProvListenHandles[i]);
					break;

			case VIA_PROV:	
					dwError = Via::TerminateListener(pListener->rgProvListenHandles[i]);
					break;

			default:
					dwError = ERROR_INVALID_PARAMETER;
					SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwError );
					break;			
		}

		if( ERROR_SUCCESS != dwError )
		{
			goto ErrorExit;
		}
	}

	if( 0 == InterlockedDecrement( &pListener->cRef ))
	{
		SetEvent( pListener->hTerminateListenerEvent );
	}	

	if( WAIT_OBJECT_0 == WaitForSingleObject( pListener->hTerminateListenerEvent, INFINITE ))
	{
		delete [] pListener->rgProviderNums;
		delete [] pListener->rgProvListenHandles;

		CloseHandle( pListener->hTerminateListenerEvent );
		
		delete pListener;
	}
	else
	{
		dwError = GetLastError();

		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwError );

		goto ErrorExit;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	return ERROR_SUCCESS;

ErrorExit:

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	return dwError;
}

DWORD SNIUpdateListener(HANDLE hListener, ProviderNum ProvNum, LPVOID pInfo)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T( "hListener: %p{HANDLE}, ProvNum: %d{ProviderNum}, pInfo: %p\n"), 
					hListener, ProvNum, pInfo);
	
	DWORD dwError = ERROR_SUCCESS;
	
	SNI_Listener * pListener = (SNI_Listener *)hListener;
	HANDLE hProvHandle = NULL;

	//	Find the provider handle for this particular provider
	//
	for(DWORD i = 0; i < pListener->ConsumerInfo.dwNumProts; i++ )
	{ 
		if (ProvNum == pListener->rgProviderNums[i])
		{
			hProvHandle = pListener->rgProvListenHandles [i];
			break;
		}
	}

	//	If the client has not subscribed to this particular protocol, fail the
	//	the call now
	//
	if (NULL == hProvHandle || HTTP_PROV != ProvNum )
	{
		dwError = ERROR_INVALID_PARAMETER;

		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwError );
		
		goto Exit;
	}

#ifdef USE_HTTP_PROVIDER	

	//
	//	The HTTP provider is the only one that needs the 
	//	ability to update a listener profile dynamically
	//	withiut forcing the client to tear down a listener and
	//	create a new one.
	//		

	dwError = Http::UpdateListener (hProvHandle, pInfo);

#endif

Exit:

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;

}

void IncrementPendingAccepts()
{
	InterlockedIncrement( &g_cPendingAccepts );
}

void DecrementPendingAccepts()
{
	InterlockedDecrement( &g_cPendingAccepts );
}

DWORD SNIResumePendingAccepts()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );

	DWORD dwError = ERROR_SUCCESS;

	// We do this check outside the crit sec to prevent the case
	// where this method is called before the global list and
	// crit sec have been initialized.  If g_pListenerList is
	// non-null then we can be certain the crit sec has been
	// initialized.
	// We also fast path the case the common case where there 
	// are no pending accepts.
	//
	if (g_pListenerList == NULL || g_cPendingAccepts == 0)
		return ERROR_SUCCESS;

	CAutoSNICritSec a_cs(g_pcsListenerList, SNI_AUTOCS_ENTER);

	// Iterate through all listeners
	//
	for (SNI_Listener * pListener = g_pListenerList;
		 pListener != NULL;
		 pListener = pListener->pNext)
	{
		// Iterate through the providers for this listener
		//
		for(DWORD i = 0; i < pListener->ConsumerInfo.dwNumProts; i++ )
		{
			DWORD dwLocalError = ERROR_SUCCESS;

			switch(pListener->rgProviderNums[i])
			{
				case TCP_PROV:
						dwLocalError = Tcp::ResumePendingAccepts(pListener->rgProvListenHandles[i]);
						break;

	#ifdef USE_HTTP_PROVIDER
				case HTTP_PROV:
						dwLocalError = Http::ResumePendingAccepts(pListener->rgProvListenHandles[i]);
						break;
	#endif
			
				case SM_PROV:	
				case NP_PROV:	
						dwLocalError = Np::ResumePendingAccepts(pListener->rgProvListenHandles[i]);
						break;
			}

			if (dwLocalError != ERROR_SUCCESS)
			{
				BidTrace2(ERROR_TAG _T("ProvNum: %d{ProviderNum}, %d{WINERR}\n"), 
					pListener->rgProviderNums[i], dwLocalError);
				
				dwError = dwLocalError;
			}
		}
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);

	return dwError;
}


DWORD SNIAcceptDoneWithReturnCode( __in LPVOID pVoid )
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "pSOSIo: %p{SOS_IOCompRequest*}\n"), pVoid);
	
	SOS_IOCompRequest * pSOSIo = (SOS_IOCompRequest *) pVoid;
	DWORD dwBytes = pSOSIo->GetActualBytes();
	DWORD dwError = pSOSIo->GetErrorCode();
	DWORD dwRet = ERROR_SUCCESS; 
	
	SNI_Accept   * pAccept = (SNI_Accept *) pVoid;
	SNI_Provider * pProv = 0;
	LPVOID pAcceptInfo = 0;
	ProviderNum	currProv = pAccept->ProvNum;

	// Get the listener object
	SNI_Listener * pListener = (SNI_Listener *)pAccept->hSNIListener;

	// Create a new Connection object
	SNI_Conn * pConn = NULL;

	if( ERROR_SUCCESS == SNI_Conn::InitObject( &pConn, TRUE ) )
	{
		// Set the UserData to the default value;
		SNISetInfo(pConn, SNI_QUERY_CONN_BUFSIZE, (VOID *)&pListener->ConsumerInfo.DefaultUserDataLength);
		IncrementConnBufSize(pConn, &rgProvInfo[currProv]);

		// Set the IO stack for this connection - this is based on the buffer size
		// Note: We have to do after the consumer and provider buffer sizes are set in the statements above,
		// since it only at this point the size bucket for this connetion is known.
		DWORD dwSize = 0;
		if (SUCCEEDED (DWordAdd(pConn->m_ConnInfo.ConsBufferSize, pConn->m_ConnInfo.ProvBufferSize, &dwSize)))
		{
			pConn->m_MemTag = SNIMemRegion::GetMemoryTag(dwSize);
		}
		else
		{
			// Clean up the connection and fall through so the providers can
			// queue up pending accept.
			//
			pConn->Release( REF_Active );
			pConn = NULL;
		}
	}
	
	// Call the provider's AcceptDone
	switch(currProv)
	{
		// Note: pAccept->ProvNum is NP_PROV for both NP and SM connections,
		// so we should really never get SM_PROV.  
		//
		case SM_PROV: 
		case NP_PROV: 
			// we only make change about the return code for NP for now
			if( ERROR_SUCCESS != (dwRet = Np::AcceptDone(pConn, 
												pAccept->hProvAcceptKey, 
												dwBytes,
												dwError,
												&pProv, 
												&pAcceptInfo) ) )
				goto ErrorExit;								
			
			break;

		case TCP_PROV: 
			if( ERROR_SUCCESS != Tcp::AcceptDone(pConn, 
												 pAccept->hProvAcceptKey, 
												 dwBytes,
												 dwError,
												 &pProv, 
												 &pAcceptInfo) )
				goto ErrorExit;												

			break;

#ifdef USE_HTTP_PROVIDER
		case HTTP_PROV: 
			if( ERROR_SUCCESS != Http::AcceptDone(pConn, 
							pAccept->hProvAcceptKey, 
							dwBytes,
							dwError,
							&pProv, 
							&pAcceptInfo) )
				goto ErrorExit;		

			break;
#endif

		case VIA_PROV: 
			if( ERROR_SUCCESS !=  Via::AcceptDone( pConn, 
												pAccept->hProvAcceptKey, 
												dwBytes,
												dwError,
												&pProv, 
												&pAcceptInfo ) )
				goto ErrorExit;

			break;

		default: 
			//this assertion is used to catch unexpected coding errors.
			Assert( 0 && "ProviderNum: invalid value\n" );
			BidTrace0(ERROR_TAG _T("ProviderNum: invalid value\n"));
	}

	//Note: We should never use pAccept beyond this point as it may be deleted by provider, specifically HTTP
	if( pProv )
	{
		Assert( pConn );

		pConn->m_pEPInfo->Provider = pProv->m_Prot; 
		pConn->m_ConnInfo.TransportProvider = pProv->m_Prot; 

		// Set the callbacks, etc.
		pConn->m_ConsumerInfo.DefaultUserDataLength = pListener->ConsumerInfo.DefaultUserDataLength;
		pConn->m_ConsumerInfo.fnReadComp = pListener->ConsumerInfo.fnReadComp;
		pConn->m_ConsumerInfo.fnWriteComp = pListener->ConsumerInfo.fnWriteComp;
		pConn->m_ConsumerInfo.fnAcceptComp = pListener->ConsumerInfo.fnAcceptComp;
		pConn->m_ConsumerInfo.fnTrace = pListener->ConsumerInfo.fnTrace;

		// Set m_pProvHead to point to the Transport provider
		pConn->m_pProvHead = pProv;

		// Register the handle with the IO completion port
		// NP does not need to register here.
		if( ( NP_PROV != currProv ) && ( SM_PROV != currProv ) )
			SNIRegisterWithIOCP(pProv->m_hNwk);

		BidTraceU2( SNI_BID_TRACE_ON, CALLBACK_TAG _T( "Conn: %p, pAcceptInfo: %p\n"), 
							pConn, pAcceptInfo);
		
		// Callback the consumer on the Accept callback
		pConn->m_ConsumerInfo.fnAcceptComp(pConn, pAcceptInfo);
	}
	else if( pConn )
	{
		pConn->Release( REF_Active );
	}

	return dwRet;

ErrorExit:

	if( pConn )
		pConn->Release( REF_Active );
	
	return dwRet;
}

void SNIAcceptDone( __in LPVOID pVoid )
{
	SNIAcceptDoneWithReturnCode( pVoid );
}

void SNIAcceptDoneRouter( __inout LPVOID pVoid )
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "pVoid: %p{SNI_Accept*}\n"), pVoid);

	Assert( !SOS_Node::GetCurrent()->IsDACNode() );

	SNI_Accept * pAccept = (SNI_Accept *) pVoid;

	Affinity affinity = OnlineNodeMask & ((SNI_Listener *)pAccept->hSNIListener)->ConsumerInfo.NodeAffinity;

	const Affinity lastNode = pAccept->lastNode;

	Affinity nextNode = pAccept->lastNode;

	Counter currentNodeMask = 
		static_cast<Affinity>(1)<<(SOS_Node::GetCurrent()->GetNodeId());

	//DCR 120005309, SQL Project Tracking  (NP Round Robin)
	// Specialize NP, NP Accept process has two parts:
	// 1) Accept a connection (AcceptDone), 
	//			which should be done on the current node
	// 2) Prepare listening for next connection (AsyncAccept, in SNIAcceptDoneWrapper), 
	//			which should be done on the next scheduled node, by enquing Task.
	if( ( NP_PROV == pAccept->ProvNum ) || ( SM_PROV == pAccept->ProvNum ) )
	{
		//execute it on the current node
		//accept it even if the current node has just been turned offline
		if( ERROR_SUCCESS != SNIAcceptDoneWithReturnCode( pAccept ) )
		{
			return; 		//This only happens in the terminating phase, should not prepare for next accept.
		}

	}

	//
	//Fast path if this listener only affinitize to current node, no round-robin is needed
	//Or affinity is 0 when the OnlineNodeMask is temparily turn into 0, during boot or 
	//nodemask changing.
	//
	if(affinity == currentNodeMask || 0 == affinity)
	{
		SNIAcceptDoneWrapper( pAccept );

		return;
	}

	do
	{
		//
		// find next node to use
		//

		nextNode = ~( nextNode ^ ( nextNode - 1)) & affinity;

		if ( nextNode == 0)
		{
			nextNode = affinity;
		}
		
		nextNode = nextNode ^ ( nextNode & ( nextNode - 1));

		//
		// if the node we are looking for is the node we are running
		// on then we don't need to do the enumeration
		//
		
		if( nextNode == currentNodeMask && 
			!SOS_Node::GetCurrent()->IsOffline())
		{
			pAccept->lastNode = nextNode;

			SNIAcceptDoneWrapper( pAccept );
			
			
			return;
		}

		SOS_NodeEnum enumNode;

		SOS_Node *pNode;

		for( pNode = enumNode.GetNext(NULL); pNode; pNode = enumNode.GetNext(pNode))
		{
			Assert( !pNode->IsDACNode());
			
			if( nextNode != static_cast<Affinity>(1)<<pNode->GetNodeId())
			{
				continue;
			}
			
			//
			// we found the node we are looking for, if it is offline then 
			// it can't be used. In this case get out of the for loop so that
			// we can look for the next node to use
			//
			
			if( pNode->IsOffline() )
			{
				break;
			}

			// 
			// to avoid a race condition we set this before enqueue
			//
			pAccept->lastNode = nextNode;


			SOS_Task * pTask;

			SOS_RESULT result;

			result = pNode->EnqueueTaskDirect ((SOS_Task::Func)SNIAcceptDoneWrapper, 
												pAccept, 
												PreemptiveTask, 
												&pTask);
			
			if (result == SOS_OK)
			{
				pTask->Release ();

				return;
			}
			else
			{
				//
				// enqueue failed we need to find another node in our set to use
				//

				BidTrace2(ERROR_TAG _T("EnqueueTaskDirect: %d{SOS_RESULT}, Node Id: %d\n"), result, pNode->GetNodeId());

				break;
			}
		}
	}
	while( nextNode != lastNode );

	//
	// at this point we went through all nodes and couldn't enqueue a task
	// if current node is online we can live with using it
	// otherwise force an error so that connection will be rejected
	//

	BidTrace0(ERROR_TAG _T("failed to enqueue, will use current node\n"));

	// To preserve NP's original behavior, let NP always listen on some node, even if all nodes are offline. 
	if( ( NP_PROV != pAccept->ProvNum ) && 
		( SM_PROV != pAccept->ProvNum ) &&
		SOS_Node::GetCurrent()->IsOffline() )
	{
		BidTrace0(ERROR_TAG _T("current node is offline\n"));

		if( ERROR_SUCCESS == pAccept->SOSIo.GetErrorCode())
		{
			BidTrace0(ERROR_TAG _T("forcing accept error\n"));

			pAccept->SOSIo.SetErrorCode( ERROR_FAIL );
		}
	}

	
	SNIAcceptDoneWrapper( pAccept );
	
}

DWORD SNIOpenSync( __in SNI_CONSUMER_INFO * pConsumerInfo,
				   __inout_opt LPWSTR               wszConnect,
				   __in LPVOID              pOpenInfo,
				   __out SNI_Conn         ** ppConn,
				   BOOL	               fSync,
				   int				   timeout)
{
	BidxScopeAutoSNI6( SNIAPI_TAG _T( "pConsumerInfo: %p{SNI_CONSUMER_INFO*}, ")
								_T("wszConnect: %p{LPWSTR}, ")
								_T("pOpenInfo: %p, ")
								_T("ppConn: %p{SNI_Conn**}, ")
								_T("fSync: %d{BOOL}, ")
								_T("timeout: %d\n"), 
								pConsumerInfo,  
								wszConnect, 
								pOpenInfo, 
								ppConn, 
								fSync,
								timeout);

	ProtElem *pProtElem = 0;
	
	SNI_Conn     * pConn = NULL;
	SNI_Provider * pProv = NULL;
	DWORD		   dwError;

	DWORD dwStart = GetTickCount();
	int timeleft = timeout;
	
	// For async, ensure that we are NOT running on Win9x
	if( gfIsWin9x && !fSync )
	{
		dwError = ERROR_NOT_SUPPORTED;
		
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_3, dwError );

		goto ErrorExit;
	}

	ProviderNum    ProvNum;

	if( NULL != wszConnect )
	{
		if ( ERROR_SUCCESS != (dwError = StrTrimBothW_Sys( wszConnect, wcslen(wszConnect) + 1, NULL, L" \t", ARRAYSIZE(L" \t"))))
		{
			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, dwError );
			goto ErrorExit;			
		}
	}	

	if( !wszConnect )
	{
		if( !pOpenInfo )
		{
			//this assertion is ued to catch unexpected coding errors.
			Assert( 0 && " pOpenInfo is NULL\n" );
			BidTrace0( ERROR_TAG _T("pOpenInfo is NULL\n") );
			dwError = ERROR_INVALID_PARAMETER;			
			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, dwError );
			goto ErrorExit;
		}

		pProtElem = static_cast<ProtElem *>(pOpenInfo);

		ProvNum = pProtElem->GetProviderNum();
	}
	else if( !wcscmp( L"session:", wszConnect))
	{
		ProvNum = SESSION_PROV;
	}
	else if( !pOpenInfo )
	{
		dwError = ProtElem::MakeProtElem( wszConnect, &pProtElem);

		if( ERROR_SUCCESS != dwError )
		{
			goto ErrorExit;
		}

		ProvNum = pProtElem->GetProviderNum();
	}
	else
	{
		pProtElem = static_cast<ProtElem *>(pOpenInfo);

		ProvNum = pProtElem->GetProviderNum();
	}
	
	// Create a new Connection object

	dwError = SNI_Conn::InitObject( &pConn );

	if( ERROR_SUCCESS != dwError )
	{
		goto ErrorExit;
	}

#ifndef SNI_BASED_CLIENT
	// For TCP, we create the EPInfo for outgoing connections as well
	//	This is needed when SSB connections use the SNI TCP provider
	//
	if (TCP_PROV == ProvNum)
	{
		Assert(!pConn->m_pEPInfo);
		pConn->m_pEPInfo = NewNoX(gpmo) SNI_EPInfo;
		
		if( !pConn->m_pEPInfo )
		{
			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, ERROR_OUTOFMEMORY );
			dwError = ERROR_OUTOFMEMORY;
			goto ErrorExit;
		}
	}
#endif	

	pConn->m_fClient = true;

	pConn->m_fSync = fSync ? true : false;


	if( ProvNum != SESSION_PROV )
	{
		Assert( ProvNum==TCP_PROV || ProvNum==NP_PROV || ProvNum==SM_PROV || ProvNum==VIA_PROV);

		dwError = pConn->SetServerName(pProtElem->m_wszServerName, pProtElem->m_wszOriginalServerName);

		if( ERROR_SUCCESS != dwError )
		{
			goto ErrorExit;
		}
	}


	// Set the UserData to the default value;
	//
	SNISetInfo(pConn, SNI_QUERY_CONN_BUFSIZE, (VOID *)&pConsumerInfo->DefaultUserDataLength);

	if( ProvNum==SESSION_PROV)
	{
		SNI_Conn * ptmpConn = (SNI_Conn *)pOpenInfo;
		
		// Copy the relevant SNI_CONN_INFO and other information
		pConn->m_ConnInfo.ConsBufferSize = ptmpConn->m_ConnInfo.ConsBufferSize;
		pConn->m_ConnInfo.ProvBufferSize = ptmpConn->m_ConnInfo.ProvBufferSize;
		pConn->m_ConnInfo.ProvOffset = ptmpConn->m_ConnInfo.ProvOffset;
		pConn->m_ConnInfo.TransportProvider = ptmpConn->m_ConnInfo.TransportProvider;

		pConn->m_pSec->SetSecPkgId(ptmpConn->m_pSec->GetSecPkgId());

		pConn->m_MemTag = ((SNI_Conn *)pOpenInfo)->m_MemTag;
		
		// SNIOpenSync is used by client, we keep the same ConnectionID for MARS sessions
		SNISetInfo( pConn, SNI_QUERY_CONN_CONNID,  (void*)&(ptmpConn ->GetConnId()));


	}
	else
	{
		IncrementConnBufSize( pConn, &rgProvInfo[ProvNum] );

		// Set the IO stack for this connection - this is based on the buffer size
		// Note: We have to do after the consumer and provider buffer sizes are set in the statements above, 
		// since it only at this point the size bucket for this connetion is known.
		DWORD dwSize = 0;

		dwError = DWordAdd(pConn->m_ConnInfo.ConsBufferSize, pConn->m_ConnInfo.ProvBufferSize, &dwSize);
		if (FAILED (dwError) || (BUF_MAX < dwSize))
		{
			dwError = INTSAFE_E_ARITHMETIC_OVERFLOW;
			goto ErrorExit;
		}
		pConn->m_MemTag = SNIMemRegion::GetMemoryTag( dwSize );

	}
	
	SNISetInfo( pConn, SNI_QUERY_CONN_KEY, (VOID *)pConsumerInfo->ConsumerKey );

	// Save the consumer info - this is tmpry - need to fix this
	pConn->m_ConsumerInfo.fnReadComp  = pConsumerInfo->fnReadComp;
	pConn->m_ConsumerInfo.fnWriteComp = pConsumerInfo->fnWriteComp;
	pConn->m_ConsumerInfo.fnTrace = pConsumerInfo->fnTrace;

	if( g_fSandbox )
	{
		// SM, TCP and VIA are not supported for Sandbox case.
		if ( ProvNum == TCP_PROV || ProvNum == SM_PROV || ProvNum == VIA_PROV )
		{
			dwError = ERROR_NOT_SUPPORTED;
				
			// SNIE_8: Protocol not supported.
			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_8, dwError );
				
			goto ErrorExit;	
		}
	}

	// 
	switch( ProvNum )
	{
		// Open a connection on that protocol
		//
		case NP_PROV:
			{
				if( ERROR_SUCCESS != (dwError = Np::Open(pConn, pProtElem, &pProv)) )
					goto ErrorExit;
			}
			pConn->m_ConnInfo.TransportProvider = NP_PROV;
			break;

		case TCP_PROV:
			{
				// Adjust the timeout.
				// Do NOT fail if timeleft is less than 0. Only TCP honors timeout value and fails accordingly.
				if( INFINITE != timeout )
				{
					timeleft = timeout - (DWORD)(GetTickCount() - dwStart);
					if ( INFINITE == timeleft ) timeleft -= 1;  // avoid infinite.
					BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("timeout: %d\n"), timeleft);
				}

				if( ERROR_SUCCESS != (dwError = Tcp::Open(pConn, pProtElem, &pProv, timeleft)) )
					goto ErrorExit;
			}

			pConn->m_ConnInfo.TransportProvider = TCP_PROV;
		
			break;

		case SM_PROV:
			{
				// Release the original connection object, 
				// Sm::OpenWithFallbackand() will create a new one
				// if it succeed.  
				//
				Assert( NULL != pConn ); 
				pConn->Release( REF_Active );
				pConn = NULL;

				if( ERROR_SUCCESS != (dwError = Sm::OpenWithFallback( pConsumerInfo, 
																	  &pConn, 
																	  pProtElem, 
																	  &pProv,
																	  fSync)) )
					goto ErrorExit;
			}

			pConn->m_ConnInfo.TransportProvider = SM_PROV;
			
			break; 
		
		case SESSION_PROV:
			{
				if( ERROR_SUCCESS != (dwError = Smux::Open( pConn, 
															static_cast<SNI_Conn *>(pOpenInfo), 
															&pProv)) )
					goto ErrorExit;

				// pConn->m_fSync is set by smux to physical connection settings
				// this is a simple check to make sure that consumer passed correct flag
				Assert( pConn->m_fSync == (fSync ? true : false) );
				
				Assert( pProv->m_pConn == pConn && pConn->m_pProvHead == pProv );
			}
			break;

		default:
			{
				//this assertion is ued to catch unexpected coding errors.
				//in SNIX specifying VIA as the protocol will hit this as well.  
				Assert( 0 && " ProvNum is unknown\n" );
				BidTrace0( ERROR_TAG _T("ProvNum is unknown\n") );
				dwError = ERROR_INVALID_PARAMETER;
				SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwError );
				goto ErrorExit;
			}
	}

	// Set m_pProvHead to point to the Transport provider
	pConn->m_pProvHead = pProv;
	
	// For client, associate this with the completion port
	if( !pConn->m_fSync && pProv->m_hNwk != INVALID_HANDLE_VALUE )
	{
		if( ERROR_SUCCESS != (dwError = SNIRegisterWithIOCP( pProv->m_hNwk)) )
		{
			goto ErrorExit;
		}
	}

	*ppConn = pConn;

	if (!pOpenInfo)
	{
		delete pProtElem;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("Conn: %p\n"), *ppConn);
	
	return ERROR_SUCCESS;

ErrorExit:

	if( !pOpenInfo )
	{
		delete pProtElem;
	}

	if (pConn)
	{
		pConn->Release( REF_Active );
	}

	*ppConn = NULL;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}

DWORD SNIOpen( __in SNI_CONSUMER_INFO * pConsumerInfo,
			   __inout_opt LPWSTR               wszConnect,
			   __in LPVOID              pOpenInfo,
			   __out SNI_Conn         ** ppConn,
			   BOOL	               fSync )			   
{
	return SNIOpenSync(pConsumerInfo, wszConnect, pOpenInfo, ppConn, fSync, SNIOPEN_TIMEOUT_VALUE);
}

void SNIReadDone(LPVOID pVoid)
{
	BidxScopeEnterSNI2( SNIAPI_TAG _T( "%u#{SNI_Conn}, pSOSIo: %p{SOS_IOCompRequest*}\n"), 
		SNIPacketGetConnection(static_cast<SNI_Packet *>(pVoid))->GetBidId(), 
		pVoid);
	
	SOS_IOCompRequest * pSOSIo = (SOS_IOCompRequest *) pVoid;
	DWORD dwBytes = pSOSIo->GetActualBytes();
	DWORD dwError = pSOSIo->GetErrorCode();

	SNI_Packet * pPacket = (SNI_Packet *) pVoid;
	SNI_Conn   * pConn = SNIPacketGetConnection(pPacket);

	// SQL BU DT 290630: note that the connection may belong to a different 
	// node than the one we are running on, e.g. due to a posted completion
	// during SNIClose() closing a connection "across nodes".  

	// Fix the packet's bytes correctly - we need this only for Reads
	Assert(SNIPacketGetBufferSize(pPacket) + dwBytes >= dwBytes);
	SNIPacketSetBufferSize(pPacket, SNIPacketGetBufferSize(pPacket) + dwBytes);
	SNI_TRY
	{

	Assert( pPacket);
	while( pPacket)
	{
		SNI_Packet *pLeftOver = 0;

		// Call Provider's ReadDone function
		DWORD dwProvError = pConn->m_pProvHead->ReadDone( &pPacket, &pLeftOver, dwBytes, dwError );

		// Call Consumer's Read completion function - only if the pPacket is still valid
		// If pPacket is NULL, that means some provider consumed the Packet and the consumer 
		// should NOT be informed of this I/O
		// Also, we get the connection from the packet, since an intermediate provider could have replaced the 
		// packet passed in with another packet
		if( pPacket )
		{
			SNI_Conn *pnewConn = SNIPacketGetConnection(pPacket);

			InterlockedIncrement((LONG *) &pnewConn->m_ConnInfo.RecdPackets);
			SNITime::GetTick( &pnewConn->m_ConnInfo.Timer.m_ReadDone );
#ifdef SNIX
			// Increment REF_ActiveCallbacks, this will block SNIX clients entering 
			// SNI_Conn::WaitForActiveCallbacks until we release this ref.
			pnewConn->AddRef( REF_ActiveCallbacks );
			if ( pnewConn->FAllowCallback() )
			{
				// Call SafeReadComp to insure callback has SNI try+catch wrapper.
				// If AppDomain is unloading, the call to the read completion function can raise a C++ exception.			
				pnewConn->SafeReadComp( pPacket,  dwProvError );
			}
			else
			{
				// The AppDomain is dead, so no-one will claim this read packet, release it here.
				BidTrace1( ERROR_TAG _T( "Skipping callback. pPacket: %p{SNI_Packet*}\n"), pPacket);
			}

			// Always release the reference on the packet for SNIX.  In the SNIX async read packet case, 
			// only SNI has a reference on the packet, and SNIX will not release this packet at all. 
			// SNIX code will always copy all the data out of the  read packet before SafeReadComp returns, so
			// it is safe to release the packet at this point in time.  
			SNIPacketRelease(pPacket);
			
			pnewConn->Release( REF_ActiveCallbacks );
#else			
			pnewConn->m_ConsumerInfo.fnReadComp(pnewConn->m_ConsKey, pPacket, dwProvError);
#endif
			// Release the ref we took for this  I/O
			pnewConn->Release( REF_Read );
		}

		if( pLeftOver )
		{
			dwBytes = SNIPacketGetBufferSize( pLeftOver);
			dwError = ERROR_SUCCESS;
		}

		pPacket = pLeftOver;

	}
    
	}
	SNI_CATCH
	{
		BidTrace0( CATCH_ERR_TAG _T("Executing the empty handler\n") ); 
	}
	SNI_ENDCATCH

	BidScopeLeave();
}

void SNIWriteDone(LPVOID pVoid)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T( "%u#{SNI_Conn}, pSOSIo: %p{SOS_IOCompRequest*}\n"), 
		SNIPacketGetConnection(static_cast<SNI_Packet *>(pVoid))->GetBidId(), 
		pVoid);
	
	SOS_IOCompRequest * pSOSIo = (SOS_IOCompRequest *) pVoid;
	DWORD dwBytes = pSOSIo->GetActualBytes();
	DWORD dwError = pSOSIo->GetErrorCode();
	
	SNI_Packet * pPacket = (SNI_Packet *) pVoid;
	SNI_Conn * pConn = SNIPacketGetConnection(pPacket);

	// SQL BU DT 290630: note that the connection may belong to a different 
	// node than the one we are running on, e.g. due to a posted completion
	// during SNIClose() closing a connection "across nodes".  

	// For writes, the packet already has the correct number of bytes
	// set.

	// Call Provider's WriteDone function
	DWORD dwProvError = pConn->m_pProvHead->WriteDone(&pPacket, dwBytes, dwError);
	
	// Call Consumer's Write completion function - only if the pPacket is still valid
	// If pPacket is NULL, that means some provider consumed the Packet and the consumer 
	// should NOT be informed of this I/O	
	if( pPacket )
	{
		BidTraceU3( SNI_BID_TRACE_ON, CALLBACK_TAG _T( "Conn: %p, Packet: %p, %d{WINERR}\n"), 
						pConn, pPacket, dwProvError);

		InterlockedIncrement((LONG *) &pConn->m_ConnInfo.SentPackets);
		SNITime::GetTick( &pConn->m_ConnInfo.Timer.m_WriteDone );
		
#ifdef SNIX
		// Increment REF_ActiveCallbacks, this will block SNIX clients entering 
		// SNI_Conn::WaitForActiveCallbacks until we release this ref.
		pConn->AddRef( REF_ActiveCallbacks );
		if ( pConn->FAllowCallback() )
		{
			// Call SafeWriteComp to insure callback has try+catch wrapper.
			// If AppDomain is unloading, the call to the write completion function can raise a C++ exception.
			pConn->SafeWriteComp( pPacket, dwProvError );
		}
		else
		{
			// The AppDomain is potentially dead, so we skip the callback.
			BidTrace1( ERROR_TAG _T( "Skipping callback. pPacket: %p{SNI_Packet*}\n"), pPacket);
		}
		pConn->Release( REF_ActiveCallbacks );

		// Always release the reference on the packet for SNIX.  In the write packet case both SNIX and SNI will have 
		// a ref on the packet.  So if SNIX code still needs to use the packet at this point it will have a ref count > 1.
		SNIPacketRelease( pPacket );
#else
		pConn->m_ConsumerInfo.fnWriteComp(pConn->m_ConsKey, pPacket, dwProvError);
#endif
		// Release the ref we took for this  I/O
		pConn->Release( REF_Write );
	}
}


DWORD SNIReadAsync( __out_opt SNI_Conn    * pConn,
                    __out SNI_Packet ** ppNewPacket,
                    LPVOID        pPacketKey )	
{	
	BidxScopeAutoSNI4( SNIAPI_TAG _T("%u#{SNI_Conn}, ")
							  _T("pConn: %p{SNI_Conn*}, ") 
							  _T("ppNewPacket: %p{SNI_Packet**}, ")
							  _T("pPacketKey: %p\n"), 
							  pConn->GetBidId(), 
							  pConn,  
							  ppNewPacket, 
							  pPacketKey);

	// Initialize this to NULL
	*ppNewPacket = NULL;
	
	Assert( !pConn->m_fSync );

	// Increment the refcount
	pConn->AddRef( REF_Read );

	// Call next Provider's ReadAsync function
	DWORD dwError;

	dwError= pConn->m_pProvHead->ReadAsync(ppNewPacket, pPacketKey);

	// If return value is ERROR_SUCCESS or error (i.e. it is NOT pending), that
	// indicates that there is a packet being returned in this call. So, decrement
	// the refcount, since ReadDone is NOT gonna be called.
	if ( ERROR_IO_PENDING != dwError )
	{
		if( *ppNewPacket && (ERROR_SUCCESS == dwError) )
		{
			InterlockedIncrement((LONG *) &pConn->m_ConnInfo.RecdPackets);
			SNITime::GetTick( &pConn->m_ConnInfo.Timer.m_ReadDone );
		}
		
		pConn->Release( REF_Read );
	}

	BidTraceU2( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}, Packet: %p\n"), dwError, *ppNewPacket);
	
	return dwError;
}

DWORD SNIReadSync( 	__in SNI_Conn    * pConn,
                    __out SNI_Packet ** ppNewPacket,
                    int           timeout )	
{
	BidxScopeAutoSNI4( SNIAPI_TAG _T("%u#{SNI_Conn}, ")
							  _T("pConn: %p{SNI_Conn*}, ")
							  _T("ppNewPacket: %p{SNI_Packet**}, ")
							  _T("timeout: %d\n"), 
							  pConn->GetBidId(), 
							  pConn,  
							  ppNewPacket, 
							  timeout);

	// Initialize this to NULL
	*ppNewPacket = NULL;

	DWORD dwError;
	
	dwError = pConn->m_pProvHead->ReadSync( ppNewPacket, timeout );

	Assert( ERROR_IO_PENDING != dwError );

	if( *ppNewPacket && (ERROR_SUCCESS == dwError) )
	{
		InterlockedIncrement((LONG *) &pConn->m_ConnInfo.RecdPackets);
		SNITime::GetTick( &pConn->m_ConnInfo.Timer.m_ReadDone );
	}
	
	BidTraceU2( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}, Packet: %p\n"), dwError, *ppNewPacket);
	
	return dwError;
}

DWORD SNICheckConnection(__in SNI_Conn* pConn)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("%u#{SNI_Conn}, ")
							  _T("pConn: %p{SNI_Conn*}, "),
							  pConn->GetBidId(), 
							  pConn);

	DWORD dwError;
	
	dwError = pConn->m_pProvHead->CheckConnection();

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}

DWORD SNIPartialReadAsync( __out_opt SNI_Conn     * pConn,
                       	   __in SNI_Packet   * pOldPacket,
                       	   DWORD          cbBytesToRead,
                       	   SNI_ProvInfo * pProvInfo )
{
	BidxScopeAutoSNI5( SNIAPI_TAG _T("%u#{SNI_Conn}, ")
							  _T("pConn: %p{SNI_Conn*}, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("cbBytesToRead: %d, ")
							  _T("ProvInfo: %p{SNI_ProvInfo*}\n"),
							  pConn->GetBidId(), 
							  pConn, 
							  pOldPacket, 
							  cbBytesToRead, 
							  pProvInfo);
	
	// Increment the refcount
	pConn->AddRef( REF_Read );

	// Call next Provider's PartialReadAsync function
	DWORD dwError;

	dwError = pConn->m_pProvHead->PartialReadAsync( pOldPacket, cbBytesToRead, pProvInfo );
	
	if( ERROR_IO_PENDING != dwError )
	{
		if( ERROR_SUCCESS == dwError )
		{
			InterlockedIncrement((LONG *) &pConn->m_ConnInfo.RecdPackets);
			SNITime::GetTick( &pConn->m_ConnInfo.Timer.m_ReadDone );
		}
		
		pConn->Release( REF_Read );
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}

DWORD SNIPartialReadSync(	__in SNI_Conn	* pConn,
							__inout SNI_Packet	* pOldPacket,
							DWORD		  cbBytesToRead,
							int			  timeout )
{	
	BidxScopeAutoSNI5( SNIAPI_TAG _T("%u#{SNI_Conn}, ")
							  _T("pConn: %p{SNI_Conn*}, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("cbBytesToRead: %d, ")
							  _T("timeout: %d\n"),
							  pConn->GetBidId(), 
							  pConn, 
							  pOldPacket, 
							  cbBytesToRead, 
							  timeout);
	
	DWORD 	dwError;

	dwError = pConn->m_pProvHead->PartialReadSync( pOldPacket, cbBytesToRead, timeout );

	Assert( ERROR_IO_PENDING != dwError );

	if( ERROR_SUCCESS == dwError )
	{
		InterlockedIncrement((LONG *) &pConn->m_ConnInfo.RecdPackets);
		SNITime::GetTick( &pConn->m_ConnInfo.Timer.m_ReadDone );
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}

DWORD SNIWriteAsync( __out_opt SNI_Conn     * pConn,
					 __inout SNI_Packet   * pPacket,
					 SNI_ProvInfo * pProvInfo )
{
	BidxScopeAutoSNI4( SNIAPI_TAG _T("%u#{SNI_Conn}, ")
							  _T("pConn: %p{SNI_Conn*}, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("pProvInfo: %p{SNI_ProvInfo*}\n"), 
							  pConn->GetBidId(), 
							  pConn, 
							  pPacket, 
							  pProvInfo);
	
	Assert( !pConn->m_fSync );

	// Increment the refcount
	pConn->AddRef( REF_Write );
	
#ifdef SNIX
	// AddRef the packet for our pending write.
	Assert( NULL != pPacket );
	SNIPacketAddRef( pPacket );
#endif

	// Call next Provider's WriteAsync function
	DWORD dwError;

	dwError = pConn->m_pProvHead->WriteAsync(pPacket, pProvInfo);

	/*
	if( dwError == ERROR_IO_PENDING )
	{
		#ifdef SERVER
		SOS_Scheduler::AddIORequest (pPacket, SOS::NetworkIO);
		#endif
	}
	*/

	// If it IS an error, decrement the refcount
	if ( ERROR_IO_PENDING != dwError )
	{
		if( ERROR_SUCCESS == dwError )
		{
			InterlockedIncrement((LONG *) &pConn->m_ConnInfo.SentPackets);
			SNITime::GetTick( &pConn->m_ConnInfo.Timer.m_WriteDone );
		}
		
#ifdef SNIX
		// There will be no callback, so decrement packet's refcount.
		SNIPacketRelease( pPacket );
#endif

		pConn->Release( REF_Write );
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}

DWORD SNIWriteSync( 	__in SNI_Conn     * pConn,
					__inout SNI_Packet   * pPacket,
					SNI_ProvInfo * pProvInfo )
{
	BidxScopeAutoSNI4( SNIAPI_TAG _T("%u#{SNI_Conn}, ")
							  _T("pConn: %p{SNI_Conn*}, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("pProvInfo: %p{SNI_ProvInfo*}\n"), 
							  pConn->GetBidId(),
							  pConn,  
							  pPacket, 
							  pProvInfo);
	
	DWORD dwError;
	
	dwError = pConn->m_pProvHead->WriteSync( pPacket, pProvInfo );

	if( ERROR_SUCCESS == dwError )
	{
		InterlockedIncrement((LONG *) &pConn->m_ConnInfo.SentPackets);
		SNITime::GetTick( &pConn->m_ConnInfo.Timer.m_WriteDone );
	}
	
	Assert( dwError!=ERROR_IO_PENDING);

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}


DWORD SNIGatherWriteAsync( __out_opt SNI_Conn     * pConn,
					 __in SNI_Packet   * pPacket,
					 SNI_ProvInfo * pProvInfo )
{
	BidxScopeAutoSNI4( SNIAPI_TAG _T("%u#{SNI_Conn}, ")
							  _T("pConn: %p{SNI_Conn*}, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("pProvInfo: %p{SNI_ProvInfo*}\n"), 
							  pConn->GetBidId(),
							  pConn, 
							  pPacket, 
							  pProvInfo);
	
	DWORD dwError;

	Assert( !pConn->m_fSync );

	// Increment the refcount
	pConn->AddRef( REF_Write );

	// Call next Provider's WriteAsync function
	dwError = pConn->m_pProvHead->GatherWriteAsync(pPacket, pProvInfo);

	// If it IS an error OR success, decrement the refcount
	if ( ERROR_IO_PENDING != dwError )
	{
		if( ERROR_SUCCESS == dwError )
		{
			InterlockedIncrement((LONG *) &pConn->m_ConnInfo.SentPackets);
			SNITime::GetTick( &pConn->m_ConnInfo.Timer.m_WriteDone );
		}
		
		pConn->Release( REF_Write );
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}

DWORD SNIClose(__inout SNI_Conn * pConn)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("%u#{SNI_Conn}, ") 
							  _T( "pConn: %p{SNI_Conn*}\n"), 
							  pConn->GetBidId(),
							  pConn);
	DWORD dwError;
	
#ifdef SNIX
	// Wait here for all active callbacks before releasing connection.
	pConn->WaitForActiveCallbacks();
#endif

	{
		// Serializing with SNIRemoveProvider for SDT#365303.
		CAutoSNICritSec a_cs(pConn->m_csProvList, SNI_AUTOCS_ENTER);
		
		// Call providers Close function
		dwError = pConn->m_pProvHead->Close();
	}

	pConn->Release( REF_Active );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}

DWORD SNIQueryInfo(  UINT QType, __out VOID * pbQInfo)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T( "QType: %d, pbQInfo: %p\n"), QType, pbQInfo);

	DWORD dwErr = ERROR_SUCCESS;

	switch( QType)
	{
		case SNI_QUERY_CLIENT_ENCRYPT_POSSIBLE:

			*(DWORD *)pbQInfo = Ssl::IsClientEncryptPossible();

			break;
			
		case SNI_QUERY_SERVER_ENCRYPT_POSSIBLE:

			*(DWORD *)pbQInfo = Ssl::IsServerEncryptPossible();

			break;

		case SNI_QUERY_CERTIFICATE:

			*(PCCERT_CONTEXT *) pbQInfo =  Ssl::GetServerCertificate();

			break;

#ifdef SNI_BASED_CLIENT

		case SNI_QUERY_LOCALDB_HMODULE:

			{
				// Check the handle to verify if the sqlUserInstance.dll was loaded
				// if not, Load the dll and return the handle.
				//
				LocalDB* pLdbInstance = NULL;
				if(ERROR_SUCCESS !=( dwErr = LocalDB::LDBInstance(&pLdbInstance)))
				{
					BidTrace1(ERROR_TAG _T("LocalDB: Unable to retrieve LocalDB instance. %d{WINERR}\n"), dwErr);
					pbQInfo = NULL;
				}				
				else
				{
					if(NULL == pLdbInstance->m_hUserInstanceDll)
					{
						dwErr = pLdbInstance->loadUserInstanceDll();
						if(ERROR_SUCCESS != dwErr)
						{
							BidTrace1(ERROR_TAG _T("LocalDB: Unable to load SQLUserInstance.dll. %d{WINERR}\n"), dwErr);
						}
					}
					*(HMODULE *)pbQInfo = pLdbInstance->m_hUserInstanceDll;
				}								
			}	
			break;
#endif
#ifdef SNI_BASED_CLIENT

		case SNI_QUERY_TCP_SKIP_IO_COMPLETION_ON_SUCCESS:

			*(BOOL *)pbQInfo = Tcp::s_fSkipCompletionPort;
			break;
#endif

		default:
			//this assertion is used to catch unexpected coding errors.
			Assert( 0 && " QType is unknown\n" );
			BidTrace0( ERROR_TAG _T("QType is unknown\n") );
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("QInfo: %p\n"), pbQInfo);

	return dwErr;
}

DWORD SNIWaitForSSLHandshakeToComplete( __in SNI_Conn * pConn, DWORD dwMilliseconds )
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T( "pConn: %p{SNI_Conn*}, dwMilliseconds: %d\n"), 
				pConn, dwMilliseconds);
	
	Assert(pConn);

	Assert(pConn->m_fClient && ! pConn->m_fSync );

	// default to success, so that if no Ssl exists in the chain, we exit with Success.
	DWORD dwRet = ERROR_SUCCESS;
	
	{
		CAutoSNICritSec a_cs(pConn->m_csProvList, SNI_AUTOCS_ENTER);
		for( SNI_Provider *pProv = pConn->m_pProvHead; 
			NULL != pProv; 
			pProv = pProv->m_pNext)
		{
			if( pProv->m_Prot == SSL_PROV )
			{
				CryptoBase *pCryptoBase = (CryptoBase *) pProv;
				dwRet = pCryptoBase->WaitForHandshakeDone(dwMilliseconds);
				break;
			}
		}
	}
	
	return dwRet;

}

DWORD SNIGetInfo( __in SNI_Conn * pConn, UINT QType, __out VOID * pbQInfo)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T( "pConn: %p{SNI_Conn*}, QType: %d, pbQInfo: %p\n"), 
				pConn, QType, pbQInfo);

	Assert(pConn);
	
	SNI_Provider *pProv;
	DWORD dwError = ERROR_SUCCESS;
	PeerAddrInfo * pPeerAddrInfo = static_cast<PeerAddrInfo *>(pbQInfo);
	SNI_EPInfo * pEPInfo = static_cast<SNI_EPInfo *>(pbQInfo);
	 
	switch(QType)
	{
		case SNI_QUERY_CONN_ENCRYPT:

			{
			CAutoSNICritSec a_cs(pConn->m_csProvList, SNI_AUTOCS_ENTER);
			for( pProv = pConn->m_pProvHead; pProv; pProv = pProv->m_pNext)
				if( (pProv->m_Prot == SSL_PROV) && pProv->m_fActive )
					break;

			*(DWORD *)pbQInfo = ( 0==pProv ? FALSE : TRUE);
			}

			break;

		case SNI_QUERY_CONN_PROVIDERNUM:

			{
			CAutoSNICritSec a_cs(pConn->m_csProvList, SNI_AUTOCS_ENTER);
			for( pProv = pConn->m_pProvHead; ;pProv = pProv->m_pNext)
				if( (0 == pProv->m_pNext) || (SESSION_PROV == pProv->m_Prot) )
					break;

			*( ProviderNum *)pbQInfo = pProv->m_Prot;
			}
			
			break;

		case SNI_QUERY_CONN_CONNID:

			*( GUID *)pbQInfo = pConn->GetConnId();
			
			break;

		case SNI_QUERY_CONN_PEERID:
			
			*( GUID *)pbQInfo = pConn->GetConnPeerId();
			break;

		case SNI_QUERY_CONN_PARENTCONNID:

			// First ensure that this is an Session connection
			{
			CAutoSNICritSec a_cs(pConn->m_csProvList, SNI_AUTOCS_ENTER);
			for( pProv = pConn->m_pProvHead; pProv; pProv = pProv->m_pNext)
				if( SESSION_PROV == pProv->m_Prot )
					break;

			// No session prov found - return error
			if( !pProv )
				dwError = ERROR_INVALID_PARAMETER;

			// Valid Session connection
			else
			{
				// Get the transport provider
				for( pProv = pConn->m_pProvHead; pProv; pProv = pProv->m_pNext)
					if( 0 == pProv->m_pNext )
						break;

				*( GUID *)pbQInfo = pProv->m_pConn->GetConnId();
			}
			}
			break;
			
		case SNI_QUERY_CONN_BUFSIZE:

			*(DWORD *)pbQInfo = pConn->m_ConnInfo.ConsBufferSize;

			break;

		case SNI_QUERY_CONN_SECPKG:

			*(SecPkg *)pbQInfo = pConn->m_pSec->GetSecPkgId();

			break;

		case SNI_QUERY_CONN_SECPKGNAME:

			if( !pConn->m_pSec )
			{
				dwError = ERROR_INVALID_PARAMETER;
			}
			else
			{
				dwError = pConn->m_pSec->GetSecPkgName( (LPTSTR *) pbQInfo);
			}

			break;

		case SNI_QUERY_CONN_SECPKGMUTUALAUTH:

			if( !pConn->m_pSec )
			{
				dwError = ERROR_INVALID_PARAMETER;
			}
			else
			{

				dwError = pConn->m_pSec->GetMutualAuth((BOOL*) pbQInfo );
			}

			break;

		case SNI_QUERY_CONN_NETPACKETSIZE:

			dwError = (DWORD) DWordAdd(pConn->m_ConnInfo.ConsBufferSize, 
									pConn->m_ConnInfo.ProvBufferSize,
									(DWORD *)pbQInfo);

			break;

		case SNI_QUERY_CONN_NODENUM:

			*(Counter *)pbQInfo = pConn->m_NodeId;
			

			break;
			
		case SNI_QUERY_CONN_PACKETSRECD:

			*(DWORD *)pbQInfo = InterlockedCompareExchange((LONG *) &pConn->m_ConnInfo.RecdPackets, 0, 0);

			break;

		case SNI_QUERY_CONN_PACKETSSENT:

			*(DWORD *)pbQInfo = InterlockedCompareExchange((LONG *) &pConn->m_ConnInfo.SentPackets, 0, 0);

			break;

		case SNI_QUERY_CONN_LOCALADDR:

			{
			CAutoSNICritSec a_cs(pConn->m_csProvList, SNI_AUTOCS_ENTER);
			for( pProv = pConn->m_pProvHead; pProv; pProv = pProv->m_pNext)
			{
				// Tcp
				if( pProv->m_Prot == TCP_PROV )
				{
					dwError = Tcp::GetLocalAddress(pConn, pPeerAddrInfo);
				}
#ifdef USE_HTTP_PROVIDER				
				// Http
				else if( pProv->m_Prot == HTTP_PROV )
				{
					dwError = Http::GetLocalAddress(pConn, pPeerAddrInfo);
				}
#endif				
				else
				{
					dwError = ERROR_INVALID_PARAMETER;
					SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_5, dwError );
				}
			}
			}
			break;

		case SNI_QUERY_CONN_PEERADDR:

			{
			// First initialize the output parameter
			// 
			pPeerAddrInfo->PeerAddr[0] = '\0';
			pPeerAddrInfo->PeerAddrLen = 0;

			CAutoSNICritSec a_cs(pConn->m_csProvList, SNI_AUTOCS_ENTER);
			// Loop through providers until we find one to fill in the
			// peer address info
			//
			for( pProv = pConn->m_pProvHead; pProv; pProv = pProv->m_pNext)
			{
				// Tcp
				if( pProv->m_Prot == TCP_PROV )
				{
					dwError = Tcp::GetPeerAddress(pConn, pPeerAddrInfo);
					break;
				}

#ifdef USE_HTTP_PROVIDER
				// Http
				else if( pProv->m_Prot == HTTP_PROV )
				{
					dwError = Http::GetPeerAddress(pConn, pPeerAddrInfo);
					break;
				}
#endif
				// Sm
				else if( pProv->m_Prot == SM_PROV )
				{
					const WCHAR x_wszStr_LocalMachine[] = L"<local machine>";
					C_ASSERT(sizeof(x_wszStr_LocalMachine) <= sizeof(pPeerAddrInfo->PeerAddr));

					// This copy includes the null in the constant string but the length doesn't
					//
					memcpy (pPeerAddrInfo->PeerAddr, x_wszStr_LocalMachine, sizeof(x_wszStr_LocalMachine));
					pPeerAddrInfo->PeerAddrLen = ARRAYSIZE(x_wszStr_LocalMachine);
					break;
				}

				// Np
				else if( pProv->m_Prot == NP_PROV )
				{
					const WCHAR x_wszStr_NamedPipe[] = L"<named pipe>";
					C_ASSERT((sizeof(x_wszStr_NamedPipe)) <= sizeof(pPeerAddrInfo->PeerAddr));

					// This copy includes the null in the constant string but the length doesn't
					//
					memcpy (pPeerAddrInfo->PeerAddr, x_wszStr_NamedPipe, 
						sizeof(x_wszStr_NamedPipe));
					pPeerAddrInfo->PeerAddrLen = ARRAYSIZE(x_wszStr_NamedPipe);
					break;
				}
			}
			}

			break;

		case SNI_QUERY_CONN_PEERPORT:

			{
			CAutoSNICritSec a_cs(pConn->m_csProvList, SNI_AUTOCS_ENTER);
			for( pProv = pConn->m_pProvHead; pProv; pProv = pProv->m_pNext)
			{
				// Tcp
				if( pProv->m_Prot == TCP_PROV )
					dwError = Tcp::GetPeerPort(pConn, (USHORT *) pbQInfo);

#ifdef USE_HTTP_PROVIDER
				// Http
				else if( pProv->m_Prot == HTTP_PROV )
					dwError = Http::GetPeerPort(pConn, (USHORT *) pbQInfo);
#endif

				else
				{
					*(USHORT *) pbQInfo = 0;
				}
			}
			}
				
			break;
			
		case SNI_QUERY_CONN_LOCALPORT:

			{
			CAutoSNICritSec a_cs(pConn->m_csProvList, SNI_AUTOCS_ENTER);
			for( pProv = pConn->m_pProvHead; pProv; pProv = pProv->m_pNext)
			{
				// Tcp
				if( pProv->m_Prot == TCP_PROV )
					dwError = Tcp::GetLocalPort(pConn, (USHORT *) pbQInfo);
#ifdef USE_HTTP_PROVIDER				
				// Http
				else if( pProv->m_Prot == HTTP_PROV )
					dwError = Http::GetLocalPort(pConn, (USHORT *) pbQInfo);
#endif				
				else
				{
					*(USHORT *) pbQInfo = 0;
				}
			}
			}
				
			break;			

		case SNI_QUERY_CONN_LASTREADTIME:

			pConn->m_ConnInfo.Timer.GetTime(pConn->m_ConnInfo.Timer.m_ReadDone, (SYSTEMTIME *) pbQInfo);

			break;

		case SNI_QUERY_CONN_LASTWRITETIME:

			pConn->m_ConnInfo.Timer.GetTime(pConn->m_ConnInfo.Timer.m_WriteDone, (SYSTEMTIME *) pbQInfo);

			break;

		case SNI_QUERY_CONN_KEY:

			*(void **) pbQInfo = pConn->m_ConsKey;

			break;
			
		case SNI_QUERY_CONN_CONSUMER_ID:

			*(UINT *) pbQInfo = pConn->m_cid;

			break;
			
		case SNI_QUERY_CONN_CONNECTTIME:
			
			pConn->m_ConnInfo.Timer.GetTime(pConn->m_ConnInfo.Timer.m_StartTick, (SYSTEMTIME *) pbQInfo);

			break;

		case SNI_QUERY_CONN_HTTPENDPOINT:
			*(void **) pbQInfo = NULL;

#ifdef USE_HTTP_PROVIDER
			if( pConn->m_pEPInfo )
			{
				if( HTTP_PROV == pConn->m_pEPInfo->Provider )
				{
					dwError = Http::GetEndpoint(pConn, (void**) pbQInfo);
				}
				else if (INVALID_PROV != pConn->m_pEPInfo->Provider)
				{
					Assert( 0 && " Invalid provider type querying Http Endpoint.\n" );
					
					dwError = ERROR_INVALID_PARAMETER;
				}
			}
			else 
			{
				Assert (0 && " Http provider endpoint is invalid but is queried.\n" );

				dwError = ERROR_INVALID_PARAMETER;
			}
#else
			Assert( 0 && " Consumer which does not use Http provider is attempting to query Http endpoint.\n" );

			dwError = ERROR_INVALID_PARAMETER;
#endif					
			break;

		case SNI_QUERY_CONN_SSLHANDSHAKESTATE:
			if( pConn->m_pProvHead->m_Prot == SSL_PROV )
			{
				((Ssl*)(pConn->m_pProvHead))->GetState((SslState*)pbQInfo );
			}
			else
				dwError = ERROR_INVALID_PARAMETER;
			break;	
			
		case SNI_QUERY_CONN_CONSUMERCONNID:

			*(short *) pbQInfo = pConn->m_Uci.ConsumerConnId;

			break;

		case SNI_QUERY_CONN_SNIUCI:

			memcpy(pbQInfo, (void*) &pConn->m_Uci, sizeof(SNIUCI));

			break;

		case SNI_QUERY_CONN_SUPPORTS_EXTENDED_PROTECTION:
			*(BOOL *) pbQInfo = FALSE;
			{
				CAutoSNICritSec a_cs(pConn->m_csProvList, SNI_AUTOCS_ENTER);
				for( pProv = pConn->m_pProvHead; NULL != pProv; pProv = pProv->m_pNext)
				{
					if( TCP_PROV == pProv->m_Prot )
					{
						*(BOOL *) pbQInfo = TRUE;
						break;
					}
				}
			}

			break;

		case SNI_QUERY_CONN_CHANNEL_PROVIDES_AUTHENTICATION_CONTEXT:
			{
				CAutoSNICritSec a_cs(pConn->m_csProvList, SNI_AUTOCS_ENTER);
				*(DWORD *)pbQInfo = pConn->m_pProvHead->QueryImpersonation();
			}

			break;
			
		case SNI_QUERY_CONN_SUPPORTS_SYNC_OVER_ASYNC:
			{
				dwError = ERROR_SUCCESS;
				*(BOOL *) pbQInfo = FALSE;
		
				CAutoSNICritSec a_cs(pConn->m_csProvList, SNI_AUTOCS_ENTER);
				for( pProv = pConn->m_pProvHead; NULL != pProv; pProv = pProv->m_pNext)
				{
					if( SMUX_PROV == pProv->m_Prot || SESSION_PROV == pProv->m_Prot )
					{
						// break out early and leave pbQInfo set to FALSE, on an intermediate provider that doesn't support the feature
						break;
					}
					
					if( TCP_PROV == pProv->m_Prot || NP_PROV == pProv->m_Prot )
					{
						// if we got to a base provider that supports the feature, that means we didn't pass an intermediate provider
						// that doesn't support the feature, and that means the whole connection supports it.
						*(BOOL *) pbQInfo = TRUE;
					}
				}
			}
			break;

		default:
			//this assertion is used to catch unexpected coding errors.
			Assert( 0 && " QType is unknown\n" );
			BidTrace0( ERROR_TAG _T("QType is unknown\n") );
			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, ERROR_INVALID_PARAMETER );

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_INVALID_PARAMETER);
			
			return ERROR_INVALID_PARAMETER;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("QInfo: %p\n"), pbQInfo);
	
	return dwError;

}
DWORD SNISetInfo( __out SNI_Conn * pConn, UINT QType, __in VOID * pbQInfo)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T( "pConn: %p{SNI_Conn*}, QType: %d, QInfo: %p\n"), 
					pConn, QType, pbQInfo);

	SNI_Provider *pProv = NULL;

	switch(QType)
	{
		// To change the consumer's buffer size
		case SNI_QUERY_CONN_BUFSIZE :
			{
				// Set the IO stack for this connection - this is based on the size of the buffer
				//
				DWORD dwSize = 0;
				pConn->m_ConnInfo.ConsBufferSize  = *((DWORD *)pbQInfo);
				if (FAILED(DWordAdd(pConn->m_ConnInfo.ConsBufferSize, pConn->m_ConnInfo.ProvBufferSize, &dwSize)))
				{
					BidTrace0( ERROR_TAG _T("buffer size integer overflow.\n") );
					SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, ERROR_INVALID_PARAMETER );
					BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_INVALID_PARAMETER);
					return ERROR_INVALID_PARAMETER;
				}
				pConn->m_MemTag = SNIMemRegion::GetMemoryTag(dwSize);
			}
			break;

		// To store the consumer's key
		case SNI_QUERY_CONN_KEY :
			pConn->m_ConsKey = pbQInfo;
			break;

		// To store the consumer's specific ID
		case SNI_QUERY_CONN_CONSUMER_ID:
			pConn->m_cid = *(UINT *) pbQInfo;
			break;

		case SNI_QUERY_CONN_SOBUFAUTOTUNING:
			{
				CAutoSNICritSec a_cs(pConn->m_csProvList, SNI_AUTOCS_ENTER);
				for( pProv = pConn->m_pProvHead; pProv; pProv = pProv->m_pNext)
				{
					if( pProv->m_Prot == TCP_PROV )
						((Tcp*)pProv)->SetSockBufAutoTuning((BOOL *) pbQInfo);
				}
			}
			break;
			
		case SNI_QUERY_CONN_CONSUMERCONNID:
			{
				BidTraceU2( SNI_BID_TRACE_ON, SNI_TAG _T("%u#{SNI_Conn}, ")
										  _T("ConsumerConnId: %d{USHORT}\n"), 
										  pConn->m_iBidId, 
										  pConn->m_Uci.ConsumerConnId ); 
				pConn->m_Uci.ConsumerConnId = ((SNIUCI *)pbQInfo)->ConsumerConnId;
			}
			break;
		
		case SNI_QUERY_CONN_CONNID:

			pConn->m_Uci.Id = *(GUID*)pbQInfo;
			break;

		case SNI_QUERY_CONN_PEERID:
			{
				pConn->m_Uci.PeerId = *(GUID *)pbQInfo;
				if(SNI_BID_TRACE_ON)
				{
					WCHAR wchGuid[37]; // length of GUID + null termination
					HRESULT hr = S_OK;
					hr = StringCchPrintf_lW(wchGuid, 37,
						L"%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", GetDefaultLocale(),
					pConn->m_Uci.PeerId.Data1,
					pConn->m_Uci.PeerId.Data2,
					pConn->m_Uci.PeerId.Data3,
					pConn->m_Uci.PeerId.Data4[0], pConn->m_Uci.PeerId.Data4[1],
					pConn->m_Uci.PeerId.Data4[2], pConn->m_Uci.PeerId.Data4[3],
					pConn->m_Uci.PeerId.Data4[4], pConn->m_Uci.PeerId.Data4[5],
					pConn->m_Uci.PeerId.Data4[6], pConn->m_Uci.PeerId.Data4[7] );
					Assert(SUCCEEDED(hr));
					BidTrace2( SNI_TAG _T("Set CorrelatedConnectionID: %ls, %u#{SNI_Conn}\n"), 
						wchGuid, pConn->m_iBidId);
				}
			}
			
			break;

#ifdef SNI_BASED_CLIENT
		case SNI_QUERY_TCP_SKIP_IO_COMPLETION_ON_SUCCESS:
			
			Tcp::s_fSkipCompletionPort = *(BOOL *)pbQInfo;
			break;
#endif

		default :
			//This assertion is used to catch unexpected coding errors.
			Assert( 0 && " QType is unknown\n" );
			BidTrace0( ERROR_TAG _T("QType is unknown\n") );
			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, ERROR_INVALID_PARAMETER );

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_INVALID_PARAMETER);
			
			return ERROR_INVALID_PARAMETER;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;
}

DWORD SNIAddProvider(__inout SNI_Conn * pConn, ProviderNum ProvNum, __in LPVOID pInfo)
{
	BidxScopeAutoSNI4( SNIAPI_TAG _T("%u#{SNI_Conn}, ")
							  _T( "pConn: %p{SNI_Conn*}, ")
							  _T("ProvNum: %d{ProviderNum}, ")
							  _T("pInfo: %p\n"), 
							  pConn->GetBidId(),
							  pConn, 
							  ProvNum, 
							  pInfo);
	
	ASSERT_CONNECTION_ON_CORRECT_NODE(pConn);

	DWORD dwError;
	
	SNI_Provider * pProv = NULL;

	{
		CAutoSNICritSec a_cs(pConn->m_csProvList, SNI_AUTOCS_ENTER);
		// Check to ensure this provider was not already added
		for( pProv = pConn->m_pProvHead; pProv; pProv = pProv->m_pNext)
		{
			if(ProvNum == pProv->m_Prot)
			{
				SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, ERROR_INVALID_PARAMETER );
				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_INVALID_PARAMETER);
				return ERROR_INVALID_PARAMETER;
			}
		}
	}
	
	switch( ProvNum )
	{
		case SIGN_PROV:
			
			// Allocate a new provider
			if(NULL == (pProv = NewNoX(gpmo) Sign(pConn)))
			{
				SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_4, ERROR_OUTOFMEMORY );

				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_OUTOFMEMORY);
				return ERROR_OUTOFMEMORY;
			}
			
			if(ERROR_SUCCESS != (dwError = pProv->FInit()))
			{
				delete pProv;

				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
				return dwError;
			}
			break;

		case SMUX_PROV:
			
			if(NULL == (pProv = NewNoX(gpmo) Smux(pConn)))
			{
				SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_4, ERROR_OUTOFMEMORY );

				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_OUTOFMEMORY);
				return ERROR_OUTOFMEMORY;
			}
			
			if(ERROR_SUCCESS != (dwError = pProv->FInit()))
			{
				delete pProv;

				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
				return dwError;
			}
			break;

		case SSL_PROV:

			if(NULL == (pProv = NewNoX(gpmo) Ssl(pConn)))
			{
				SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_4, ERROR_OUTOFMEMORY );

				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_OUTOFMEMORY);
				return ERROR_OUTOFMEMORY;
			}

			if(ERROR_SUCCESS != (dwError = pProv->FInit()))
			{
				delete pProv;

				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
				return dwError;
			}
			break;

		default:
			//this assertion is used to catch coding/usage errors.
			Assert( 0 && " ProvNum is unknown\n" );
			BidTrace0( ERROR_TAG _T("ProvNum is unknown\n") );
			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, ERROR_INVALID_PARAMETER );

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_INVALID_PARAMETER);
			return ERROR_INVALID_PARAMETER;
	}

	// Adjust provider buffer sizes and offsets
	IncrementConnBufSize(pConn, &rgProvInfo[ProvNum]);

	// Insert the provider into the head of the Provider chain
	{
		CAutoSNICritSec a_cs(pConn->m_csProvList, SNI_AUTOCS_ENTER);
		pProv->m_pNext = pConn->m_pProvHead;
		pConn->m_pProvHead = pProv;
	}

	// Call an intializing function
	// Note: This function MUST be implemented by intermediate providers
	switch( ProvNum )
	{
		case SIGN_PROV:

			HANDLE 	hSecContext;
			SecPkg	PkdId;
			
			SNISecGetSecHandle( pConn, &hSecContext, &PkdId);

			dwError = static_cast<Sign *>(pProv)->InitX( (PCtxtHandle) hSecContext );
			Assert(dwError == ERROR_SUCCESS);
			break;

		case SMUX_PROV:

			//Smux provider doesn't have InitX()
			dwError = ERROR_SUCCESS;
			break;

		case SSL_PROV:

			dwError = static_cast<Ssl *>(pProv)->InitX(pInfo);
			break;
	}

	if( dwError != ERROR_SUCCESS )
	{
		Assert( ProvNum == SSL_PROV );
		SNIRemoveProvider( pConn, ProvNum);
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	return dwError;
}

DWORD SNIRemoveProvider(__inout SNI_Conn * pConn, ProviderNum ProvNum)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#{SNI_Conn}, ")
							  _T( "pConn: %p{SNI_Conn*}, ")
							  _T("ProvNum: %d{ProviderNum}\n"), 
							  pConn->GetBidId(),
							  pConn, 
							  ProvNum );
	
	ASSERT_CONNECTION_ON_CORRECT_NODE(pConn);

	SNI_Provider * pProv = pConn->m_pProvHead;
	BOOL fProviderInitialActiveState = FALSE;

	if( ProvNum == SMUX_PROV )
	{
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
		return ERROR_SUCCESS;
	}
	
	// Check if the head is being removed
	if( pProv->m_Prot == ProvNum )
	{
		// Get current active state of provider before calling RemoveX.
		fProviderInitialActiveState = pProv->m_fActive;
	
		// Check to see if its okay to remove this provider completely
		// If its not, then we need to return here
		if( ERROR_SUCCESS != pProv->RemoveX() )
		{
			// Adjust provider buffer size and offset if provider 
			// transistions from active to inactive state.
			if ( (fProviderInitialActiveState) && (!pProv->m_fActive) )
			{
				// SSL providers support defered buffers adjustment
				if (SSL_PROV == ProvNum)
				{
					static_cast<Ssl*>(pProv)->DecConnBufSize();
				}
				else
				{
					DecrementConnBufSize(pConn, &rgProvInfo[ProvNum]);
				}
			}
		
			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
			return ERROR_SUCCESS;
		}

		{
			CAutoSNICritSec a_cs(pConn->m_csProvList, SNI_AUTOCS_ENTER);
			pConn->m_pProvHead = pProv->m_pNext;
		}
	}
	else
	{
		// Find the provider in the stack - prior to the one being removed
		for(pProv = pConn->m_pProvHead; pProv->m_pNext != NULL; pProv = pProv->m_pNext)
		{
			if( pProv->m_pNext->m_Prot == ProvNum )
			{
				// Get current active state of provider before calling RemoveX.
				fProviderInitialActiveState = pProv->m_pNext->m_fActive;
			
				// Check to see if its okay to remove this provider completely
				// If its not, then we need to return here
				if( ERROR_SUCCESS != pProv->m_pNext->RemoveX() )
				{
					// Adjust provider buffer size and offset if provider 
					// transistions from active to inactive state.
					if ( (fProviderInitialActiveState) && (!pProv->m_pNext->m_fActive) )
					{
						// SSL providers support defered buffers adjustment
						if (SSL_PROV == ProvNum)
						{
							static_cast<Ssl*>(pProv)->DecConnBufSize();
						}
						else
						{
							DecrementConnBufSize(pConn, &rgProvInfo[ProvNum]);
						}
					}
				
					BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
					return ERROR_SUCCESS;
				}
			
				{
					CAutoSNICritSec a_cs(pConn->m_csProvList, SNI_AUTOCS_ENTER);
					SNI_Provider * pOldProv = pProv->m_pNext;
					pProv->m_pNext = pProv->m_pNext->m_pNext;
					pProv = pOldProv;
				}
				break;
			}
		}

		if( pProv->m_Prot != ProvNum )
		{
			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, ERROR_INVALID_PARAMETER );

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_INVALID_PARAMETER);
			return ERROR_INVALID_PARAMETER;
		}
	}

	// Adjust provider buffer size and offset if provider 
	// transistions from active to inactive state.
	if ( (fProviderInitialActiveState) && (!pProv->m_fActive) )
	{
		// SSL providers support defered buffers adjustment
		if (SSL_PROV == ProvNum)
		{
			static_cast<Ssl*>(pProv)->DecConnBufSize();
		}
		else
		{
			DecrementConnBufSize(pConn, &rgProvInfo[ProvNum]);
		}
	}
	
	switch( pProv->m_Prot)
	{
		case SSL_PROV:
			delete pProv;
			break;
		case SIGN_PROV:
			delete pProv;
			break;
		default:
			//This assertion is used to catch unexpected coding errors.
			Assert( 0 && " ProvNum is unknown\n" );
			BidTrace0( ERROR_TAG _T("ProvNum is unknown\n") );
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	return ERROR_SUCCESS;

}

// Threads which will wait on IOCP
#ifdef SNI_BASED_CLIENT

DWORD WINAPI SNIAsyncWait (PVOID)
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T( "\n"));
	
	SOS_IOCompRequest * pSOSIo = NULL;
	DWORD dwBytesTransferred;
	ULONG_PTR ulKey;
	DWORD dwError;

	while(true)
	{
		// Reset the pointer to overlapped structure, transferred bytes and the error
		//
		pSOSIo = NULL;
		dwBytesTransferred = 0;
		dwError = 0;

        // 	There are several reasons why GetQueuedCompletionStatus
        //	could fail
        //
        //	1. If pOverlapped isn't NULL, then the 
        //	GetQueuedCompletionStatus call succeeded, it's 
        //	just that the I/O operation that generated the 
        //	completion failed. We will still call the completion
        //	routine down below but with an error code
        //	2. If pOverlapped is NULL, then we have a timeout
        //	(GLE of WAIT_TIMEOUT). That is not possible in our
        //	case because we use a timeout of INFINITE in our
        //	call to GetQueuedCompletionStatus
        //	3. If pOverlapped is NULL, GLE is not WAIT_TIMEOUT,
        // 	then we have a real failure.
        //

		if( 0 == GetQueuedCompletionStatus( ghIoCompletionPort,
	                                          			&dwBytesTransferred,
				                              		(PULONG_PTR)&ulKey,
				                                          (LPOVERLAPPED *) &pSOSIo,
				                                          INFINITE) )
		{
			dwError = GetLastError();
		}

		// Catastrophic error - we have nothing to go by, ignore
		if( !pSOSIo )
		{
			if( !g_fTerminate )
			{
				//This assertion is used to catch unexpected system call errors.
				Assert( 0 && " System call GetQueuedCompletionStatus failed\n" );
				BidTrace0( ERROR_TAG _T("System call GetQueuedCompletionStatus failed\n") );
				continue;
			}
			
			return ERROR_SUCCESS;
		}

		// Update SOS_IOCompRequest and call IO Completion Routine
		//
		pSOSIo->SetErrorCode (dwError);
		pSOSIo->SetActualBytes (dwBytesTransferred);

		// Call the callback function associated with this Packet
		// Note: For Accept, this is a hack - the object is not really a SNI_Packet but an SNI_Accept
		// struct, but since the SOSIo object is the FIRST member of the struct, we are okay
		(SNIPacketCompFunc((SNI_Packet *)pSOSIo))( pSOSIo );

	}
}

// Spawn completion threads to call the specified function to wait on
DWORD SNICreateWaitThread( WaitThreadRoutine pfnAsyncWait, PVOID pParam )
{	
	BidxScopeAutoSNI2( SNIAPI_TAG _T( "pfnAsyncWait: %p{WaitThreadRoutine}, pParam: %p\n"), 
					pfnAsyncWait, pParam);
	
	if (gnWorkerThreadCount >= RTL_NUMBER_OF(rghWorkerThreads))
	{
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, ERROR_MAX_THRDS_REACHED);

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_MAX_THRDS_REACHED);
		
		return ERROR_MAX_THRDS_REACHED;
	}
	
	// Spawn threads to call the specified function
	// Note: A Null function pointer indicates that we just want to wait on the
	// IOCP
	rghWorkerThreads[ gnWorkerThreadCount++ ] = CreateThread( NULL,
				        4096,
				        pfnAsyncWait,
				        pParam,
				        (DWORD) 0,
				        NULL );
	       				   
	if( !rghWorkerThreads[ gnWorkerThreadCount-1] )
	{
		// Decrement the counter to its original value.  
		gnWorkerThreadCount--; 
		
		DWORD dwError = GetLastError();
		
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwError );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
		
		return dwError;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;
}

// To do non-packet based PostQCS
DWORD SNIPostQCS(OVERLAPPED * pOvl, DWORD dwBytes)
{	
	BidxScopeAutoSNI2( SNIAPI_TAG _T( "pOvl: %p{OVERLAPPED*}, dwBytes: %d\n"), 
					pOvl, dwBytes);
	
	if( PostQueuedCompletionStatus( ghIoCompletionPort, dwBytes, 0, pOvl) )
	{
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
		return ERROR_SUCCESS;
	}
	else
	{
		DWORD dwError = GetLastError();

		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_24, dwError );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
		
		return dwError;
	}
}

int SNIExceptionFilter(LPEXCEPTION_POINTERS pExc)
{
#ifdef DEBUG
	WCHAR wszError[512];

	(void) StringCchPrintfW(wszError, ARRAYSIZE(wszError), L"Exception. code=%x at address %p\n",
		pExc->ExceptionRecord->ExceptionCode,
		pExc->ExceptionRecord->ExceptionAddress);
	
	BidTraceU1( SNI_BID_TRACE_ON, CATCH_ERR_TAG _T("%s\n"), wszError ); 
	return EXCEPTION_CONTINUE_SEARCH;
#else
	//---- this up only in retail bits
	return EXCEPTION_EXECUTE_HANDLER;
#endif
}

#else

// Spawn completion threads to call the specified function to wait on
DWORD SNICreateWaitThread( WaitThreadRoutine pfnAsyncWait, PVOID pParam )
{	
	BidxScopeAutoSNI2( SNIAPI_TAG _T( "pfnAsyncWait: %p{WaitThreadRoutine}, pParam: %p\n"), 
					pfnAsyncWait, pParam);
	
	Assert( pfnAsyncWait );
	SOS_Task::Func pfnWait = pfnAsyncWait;
	SOS_Task * pTask;

	if (SOS_OK != SOS_Node::EnqueueTask (pfnWait, pParam, PermanentTask | PreemptiveTask, &pTask) )
	{
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_23, ERROR_FAIL );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_FAIL);
		return ERROR_FAIL;
	}
	else
	{
		pTask->Release ();
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
		return ERROR_SUCCESS;
	}
}

// To do non-packet based PostQCS
DWORD SNIPostQCS(OVERLAPPED * pOvl, DWORD dwBytes)
{	
	BidxScopeAutoSNI2( SNIAPI_TAG _T( "pOvl: %p{OVERLAPPED*}, dwBytes: %d\n"), 
					pOvl, dwBytes);
	
	SOS_Node * pNode = SOS_Node::GetCurrent();
	if( SOS_OK == pNode->PostIOCompletion (dwBytes, (SOS_IOCompRequest *) pOvl) )
	{
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
		return ERROR_SUCCESS;
	}
	else
	{
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_24, ERROR_FAIL );
		
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_FAIL);
		return ERROR_FAIL;		
	}
}

//server side exception handling. integrate with SOS exception handling if it makes sense
//also, dont ---- any exceptions until we decide on what to do.
int SNIExceptionFilter(LPEXCEPTION_POINTERS pExc)
{
    #ifdef DEBUG
        return EXCEPTION_CONTINUE_SEARCH;
    #else
	return EXCEPTION_CONTINUE_SEARCH;
    #endif
}

#endif

DWORD SNIRegisterForAccept( HANDLE hProvListener)
{
	return SNIRegisterWithIOCP( hProvListener );
}

DWORD SNICreateAccept(	LPVOID        hSNIListener,
						ProviderNum   ProvNum,
						__in LPVOID        hProvAcceptKey,
						__out OVERLAPPED ** ppOverlap
					   )
{
	BidxScopeAutoSNI4( SNIAPI_TAG _T( "hSNIListener: %p{SNI_Listener*}, ProvNum: %d{ProviderNum}, hProvAcceptKey: %p, ppOverlap: %p{OVERLAPPED**}\n"), 
					hSNIListener, ProvNum, hProvAcceptKey, ppOverlap);
	
	// Create a new SNI_Accept object
	SNI_Accept * pAccept = NewNoX(gpmo) SNI_Accept;
	if( !pAccept )
	{
		*ppOverlap = 0;

		SNI_SET_LAST_ERROR( ProvNum, SNIE_4, ERROR_OUTOFMEMORY );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_OUTOFMEMORY);
		
		return ERROR_OUTOFMEMORY;
	}

	SNI_Listener *pListener = (SNI_Listener *)hSNIListener;

	LONG cRef;
	
	cRef = InterlockedIncrement( &pListener->cRef );

	Assert( 1 < cRef );
	
	pAccept->ProvNum        = ProvNum;
	pAccept->hProvAcceptKey = hProvAcceptKey;
	pAccept->hSNIListener   = hSNIListener;

	// Register the handle with SOS's completion port

	Affinity currentNodeMask = static_cast<Affinity>(1)<<SOS_Node::GetCurrent()->GetNodeId();

	Assert( SM_PROV == pAccept->ProvNum 
		||TCP_PROV == pAccept->ProvNum
		||NP_PROV == pAccept->ProvNum
		||VIA_PROV == pAccept->ProvNum
		||HTTP_PROV == pAccept->ProvNum);

	//Always use SNIAcceptDoneRouter, except for DAC or HTTP or NodeAffinity is 0.
	if(SOS_Node::GetCurrent()->IsDACNode() 
		|| HTTP_PROV == pAccept->ProvNum
		|| 0 == pListener->ConsumerInfo.NodeAffinity)
	{
		pAccept->lastNode = currentNodeMask;

		//
		// when running on correct node and node affinity is 1 to 1
		// routing is not necessary
		//
		
		pAccept->SOSIo.Init(0, 0, (SOS_IOCompRoutine *)SNIAcceptDone, pAccept, FALSE);
	}
	else
	{
		Affinity lastNode = g_NodeMask & pListener->ConsumerInfo.NodeAffinity;

		// force NP to use the current node as the first node
		if( ( NP_PROV == pAccept->ProvNum ) || ( SM_PROV == pAccept->ProvNum ) )
		{
			lastNode = currentNodeMask;
		}
		else
		{
			while( ~( lastNode ^ ( lastNode - 1)) & lastNode )
			{
				lastNode = ~( lastNode ^ ( lastNode - 1)) & lastNode;
			}
		}
		
		pAccept->lastNode = lastNode;

		//
		// current node should be part of NodeAffinity
		// otherwise method is called on a node not part of the node set
		//
		
		Assert( currentNodeMask & pListener->ConsumerInfo.NodeAffinity );

		pAccept->SOSIo.Init(0, 0, (SOS_IOCompRoutine *)SNIAcceptDoneRouter, pAccept, FALSE);
	}

	*ppOverlap = pAccept->SOSIo.GetOverlappedPtr();

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;
}

void SNIDeleteAccept( __in LPVOID hAccept)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "hAccept: %p{SNI_Accept*}\n"), hAccept);
	
	// Create a new SNI_Accept object
	SNI_Accept * pAccept = (SNI_Accept *)hAccept;

	SNI_Listener *pListener = (SNI_Listener *) pAccept->hSNIListener;
	
	if( 0 == InterlockedDecrement( &pListener->cRef ))
	{
		SetEvent( pListener->hTerminateListenerEvent );
	}

	delete pAccept;
	
	return;
}

PVOID WINAPI SNIAcceptDoneWrapper(__in LPVOID pVoid)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("pVoid: %p\n"), pVoid);

	SNI_Accept   * pAccept = (SNI_Accept *) pVoid;


	if( ( NP_PROV == pAccept->ProvNum ) || ( SM_PROV == pAccept->ProvNum ) )
	{
		
		Np::PrepareForNextAccept( (NpAcceptStruct*) pAccept->hProvAcceptKey );
		
	}
	else
	{
		SNIAcceptDone  (pVoid);
	}

	return NULL;
	
}

////////////////////////////////////////////////////////////////////////////////
//StrChrW_SYS
//
//This function searches for first character matching in searched string according default system 
//locale using system code page. 
//
//	Inputs:
//		[in] LPCWSTR lpwString: 
//			The point to the search string.
//		[in] int cchCount: 		
//			The string buf size in characters, not including null terminator.
//		[in] WCHAR character: 
//			The search character. Only single character is supported.
//			multi character search sould consider to use StrStrW_SYS
//			instead
//
//	Returns:
//	if the call succeeds, the  pointer to the occurence of seach character in the search string;
//	else, null pointer is return. The caller should check the return pointer.
//

LPWSTR StrChrW_SYS(__in_ecount(cchCount) LPCWSTR lpwString, 
					int cchCount	/* total char count*/, 
					WCHAR character)
{
	Assert( lpwString != NULL );
	Assert( cchCount >= 0 );
	Assert( character != 0 );
	
	LPCWSTR lpwBuf = lpwString;
	int i = 0;
	while( i < cchCount&& *lpwBuf!= 0 )
	{
		if( *lpwBuf == character )
		{
			return const_cast<LPWSTR> (lpwBuf);						//Exit successfully
		}
		lpwBuf = CharNextW(lpwBuf);
		if( lpwBuf == NULL )
		{
			break;
		}
		if( (int)(lpwBuf - lpwString )> i )
		{
			i = (int)(lpwBuf - lpwString);
		}
		else
		{
			break;
		}
	}
	
	return NULL;
} 

////////////////////////////////////////////////////////////////////////////////
//StrChrA_SYS
//
//This function searches for first character matching in searched string according default system 
//locale using system code page. The function is not compatible with UNICODE string.
//
//	Inputs:
//		[in] LPCSTR lpString: 
//			The point to the search string.
//		[in] int cbCount: 		
//			The string buf size in chars, not including null terminator.
//		[in] char character: 
//			The search character. Only single byte character is supported.
//			multiple byte character search should consider to use StrStrA_SYS
//			instead.
//
//	Returns:
//	if the call succeeds, the  pointer to the occurence of seach character in the search string;
//	else, null pointer is return. The caller should check the return pointer.
//

LPSTR StrChrA_SYS(__in_ecount(cchCount) LPCSTR lpString, 
					int cchCount	/* total char count*/, 
					char character)
{
	Assert( lpString != NULL );
	Assert( cchCount >= 0 );
	Assert( character != 0 );
	
	LPCSTR lpBuf = lpString;
	int i = 0;
	while( i < cchCount&& *lpBuf!= 0 )
	{
		if( *lpBuf == character )
		{
			return const_cast<LPSTR> (lpBuf);						//Exit successfully
		}
		lpBuf = CharNextExA(CP_ACP, lpBuf, 0);
		if( lpBuf == NULL )
		{
			break;
		}
		if( (int)(lpBuf - lpString )> i )
		{
			i = (int)(lpBuf - lpString);
		}
		else
		{
			break;
		}
	}
	
	return NULL;
} 

////////////////////////////////////////////////////////////////////////////////
//StrStrA_SYS
//
//This function search for first substring matching according to default system locale using 
//system code page.The function is not compatible with UNICODE string. 
//
//	Inputs:
//		[in] DWORd dwCmpFlags: 
//			The same flag used for CompareString. Refer to CompareString in MSDN.
//		[in] LPCSTR lpString1: 
//			The point to a null-terminated search string. 
//		[in] int cchCount1: 		
//			The string char length, not including null terminator. if the negative value 
//			is used, the cchcount calculated automaticly.
//		[in] LPCSTR lpString2: 
//			The point to a null-terminated search pattern string. 
//		[in] int cchCount2: 		
//			The string char length, not including null terminator.if the negative value is 
//			used, the cchcount 	calculated automaticly.
//
//	Returns:
//		if the call succeeds, the  pointer to the occurence of substring in the search string;
//		else, null pointer is return. The caller should check the return pointer.
//

LPSTR StrStrA_SYS(DWORD dwCmpFlags, 
					LPCSTR lpString1,
					int cchCount1,	/* total char count */
					LPCSTR lpString2, 
					int cchCount2	/* total char count */
					)
{
	Assert( lpString1 != NULL && lpString2 != NULL );

	LPCSTR lpBuf = lpString1;
	int ret = 0; 

	Assert (0 <= cchCount1 );
	Assert (1 < cchCount2 );

	if (( 1 < cchCount1 ) && ( cchCount1 >= cchCount2 ))
	{ 
		int remain = cchCount1;
		while ( *lpBuf != 0 && remain >= cchCount2 )
		{
OACR_WARNING_PUSH
OACR_WARNING_DISABLE(SYSTEM_LOCALE_MISUSE , " INTERNATIONALIZATION BASELINE AT KATMAI RTM. FUTURE ANALYSIS INTENDED. ")
			if(CSTR_EQUAL ==(ret =  CompareStringA(LOCALE_SYSTEM_DEFAULT,
							 dwCmpFlags,
							 lpBuf, cchCount2,
							 (char*)lpString2, cchCount2)))
OACR_WARNING_POP
			{
				return const_cast<LPSTR> (lpBuf);						//Exit successfully
			}
			else if( ret == 0 )
			{
				break;
			}

			lpBuf = CharNextA(lpBuf);
			if( lpBuf == NULL )
			{
				break;
			}
			remain = cchCount1 - (int)(lpBuf - lpString1);
		}
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////
//StrStrW_SYS
//
//This function search for first substring matching according to default system locale using 
//system code page.The function is not compatible with ASCII string. 
//
//	Inputs:
//		[in] DWORd dwCmpFlags: 
//			The same flag used for CompareString. Refer to CompareString in MSDN.
//		[in] LPCWSTR lpwString1: 
//			The point to a null-terminated search string. 
//		[in] int cchCount1: 		
//			The string char length, not including null terminator. if the negative value 
//			is used, the cchcount calculated automaticly.
//		[in] LPCWSTR lpwString2: 
//			The point to a null-terminated search pattern string. 
//		[in] int cchCount2: 		
//			The string char length, not including null terminator.if the negative value is 
//			used, the cchcount 	calculated automaticly.
//
//	Returns:
//		if the call succeeds, the  pointer to the occurence of substring in the search string;
//		else, null pointer is return. The caller should check the return pointer.
//

LPWSTR StrStrW_SYS(DWORD dwCmpFlags, 
					LPCWSTR lpwString1,
					int cchCount1,	/* total char count */
					LPCWSTR lpwString2, 
					int cchCount2	/* total char count */
					)
{
	Assert( lpwString1 != NULL && lpwString2 != NULL );

	LPCWSTR lpBuf = lpwString1;
	int ret = 0; 

	Assert (0 <= cchCount1 );
	Assert (1 < cchCount2 );

	if (( 1 < cchCount1 ) && ( cchCount1 >= cchCount2 ))
	{ 
		int remain = cchCount1;
		while ( *lpBuf != 0 && remain >= cchCount2 )
		{
OACR_WARNING_PUSH
OACR_WARNING_DISABLE(SYSTEM_LOCALE_MISUSE , " INTERNATIONALIZATION BASELINE AT KATMAI RTM. FUTURE ANALYSIS INTENDED. ")
			if(CSTR_EQUAL ==(ret =  CompareStringW(LOCALE_SYSTEM_DEFAULT,
							 dwCmpFlags,
							 lpBuf, cchCount2,
							 (WCHAR*)lpwString2, cchCount2)))
OACR_WARNING_POP
			{
				return const_cast<LPWSTR> (lpBuf);						//Exit successfully
			}
			else if( ret == 0 )
			{
				break;
			}

			lpBuf = CharNextW(lpBuf);
			if( lpBuf == NULL )
			{
				break;
			}
			remain = cchCount1 - (int)(lpBuf - lpwString1);
		}
	}

	return NULL;
}

// Remove all leading occurrences of any of the characters in string 'pwszTargets'
DWORD StrTrimLeftW_Sys( __inout_ecount(cchOrigin) LPWSTR  pwszOrigin /* input string */,
				size_t cchOrigin	/* total char count */, 
				__out size_t *cchReturnCount /* total char count after removal */,
				__in_ecount(cchTargets) LPCWSTR pwszTargets /* chars for removal */,
				int cchTargets)
{
	LPWSTR pwsz = pwszOrigin;
	// if we're not trimming anything, we're not doing any work
	if( *pwszOrigin == 0 )
	{
		if( cchReturnCount ) 	 *cchReturnCount = 1;
		return ERROR_SUCCESS;
	}
	
	if ( 0 != * ( pwszOrigin + cchOrigin -1 ))
	{		
		return ERROR_INVALID_PARAMETER;
	}

	while( (*pwsz != 0) && (StrChrW_SYS(pwszTargets, cchTargets, *pwsz ) != NULL) )
	{
		pwsz = CharNextW(pwsz);
	}

	size_t iFirst = 0, cchDataLength = 0;
	if( pwsz != pwszOrigin )
	{			
		iFirst =  pwsz - pwszOrigin ;
		if ( iFirst >= cchOrigin )
		{
			return ERROR_BUFFER_OVERFLOW;
		}

		cchDataLength =  cchOrigin - iFirst;
		wmemmove_s( pwszOrigin, cchOrigin, pwsz, cchDataLength );			

		*(pwszOrigin + cchDataLength) = L'\0';
		if( cchReturnCount ) 	 * cchReturnCount = cchDataLength;
	}
	else
	{
		if( cchReturnCount ) 	 * cchReturnCount = cchOrigin;
	}
		
	return ERROR_SUCCESS;
}

// Remove all trailing occurrences of any of the characters in string 'pwszTargets'
DWORD StrTrimRightW_Sys( __inout_ecount(cchOrigin) LPWSTR  pwszOrigin /* input string */,
					size_t cchOrigin	/* total char count */, 
					__out size_t *cchReturnCount /* total char count after removal */,
					__in_ecount(cchTargets) LPCWSTR pwszTargets /* chars for removal */,
					int cchTargets)
{
	LPWSTR pwsz = pwszOrigin;
	LPWSTR pwszLast = NULL;
	
	// if we're not trimming anything, we're not doing any work
	if( *pwszOrigin == 0 )
	{
		if( cchReturnCount ) 	*cchReturnCount = 1;
		return ERROR_SUCCESS;
	}

	if ( 0 != * ( pwszOrigin + cchOrigin -1 ))
	{
		return ERROR_INVALID_PARAMETER;
	}
	// find beginning of trailing matches
	// by starting at beginning (DBCS aware)
	while( *pwsz != 0 )
	{
		if( StrChrW_SYS(pwszTargets, cchTargets, *pwsz ) != NULL )
		{
			if( pwszLast == NULL )
			{
				pwszLast = pwsz;
			}
		}
		else
		{
			pwszLast = NULL;
		}
		pwsz = CharNextW(pwsz);			
	}

	if( pwszLast != NULL )
	{
		// truncate at left-most matching character  			
		if ( (size_t)( pwszLast- pwszOrigin + 1) >= cchOrigin )
		{
			return ERROR_BUFFER_OVERFLOW;
		}
		*pwszLast = L'\0';
		if( cchReturnCount ) 	*cchReturnCount = pwszLast- pwszOrigin + 1 ;
	}
	else
	{
		if( cchReturnCount ) 	*cchReturnCount = cchOrigin;
	}
		
	return ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
//StrTrimBothW_Sys
//
// This function removes leading and trailing occurences of characters in a character set from a string in place according 
// default system locale, i.e. system code page. The function is not compatible with ASCII string. 
//
//	Inputs:
//		[inout] LPWSTR pwszOrigin: 
//			The point to the input string.
//		[in] size_t cchOrigin: 		
//			The string buf size in chars, including null terminator. Max 1024.
//		[out] size_t *cchReturnCount
//			The string buf size in chars, including null terminator.
//		[in] LPCWSTR pwszTargets: 
//			The search character. Only single byte character is supported.
//			multiple byte character search should consider to use StrStrW_SYS
//			instead.
//		[in] int cchTargets:
//			number of characters in the character set.
//
//	Returns:
//	In success, returns ERROR_SUCCESS, otherwise a error code is returned.
//	Note:
//		This function does in-place replacement. If ERROR_BUFFER_OVERFLOW, data   
// 	in the in buffer might be corrupted during this function.
//	Example:
//		char base[]="   YukonSp1 \t";
//		unsigned char cs[]=" \t";
//		size_t cbout;
//		printf("base=\"%s\"[%d]\n", base, sizeof(base));
//		StrChrRemoveA_SYS(base, sizeof(base), &cbout, cs, sizeof(cs));
//		printf("base=\"%s\"[%d]\n", base, cbout);		
//		
// 	result:
//		base="   YukonSp1  "[14]
//		base_compact="YukonSp1"[9]
//


// Remove all leading and trailing occurrences of any of the characters in string 'pszTargets'
DWORD StrTrimBothW_Sys( __inout_ecount(cchOrigin) LPWSTR  pwszOrigin /* input string */,
				size_t cchOrigin	/* total char count */, 
				__out size_t *cchReturnCount /* total char count after removal */,
				__in_ecount(cchTargets) LPCWSTR pwszTargets /* chars for removal */,
				int cchTargets)
{
	DWORD ret = ERROR_SUCCESS;
	
	// if we're not trimming anything, we're not doing any work
	if( *pwszOrigin == 0 )
	{
		if( cchReturnCount ) 	 *cchReturnCount = 1;
		return ERROR_SUCCESS;
	}

	if ( 0 != * ( pwszOrigin + cchOrigin -1 ))
	{		
		return ERROR_INVALID_PARAMETER;
	}


	size_t cchSize;

	if( ERROR_SUCCESS != (ret = StrTrimRightW_Sys(pwszOrigin, 
							cchOrigin,
							&cchSize, 
							pwszTargets, 
							cchTargets)))
	{
		return ret;
	};

	if( ERROR_SUCCESS != (ret = StrTrimLeftW_Sys(pwszOrigin, 
							cchSize,
							&cchSize, 
							pwszTargets,
							cchTargets)))
	{
		return ret;
	};

	if( cchReturnCount ) 	 *cchReturnCount = cchSize;
	
	return ret;
}






// Remove all trailing occurrences of any of the characters in string 'pszTargets'
DWORD StrTrimRight_Sys( __inout_ecount(cchOrigin) LPSTR  pszOrigin /* input string */,
					size_t cchOrigin	/* total char count */, 
					__out size_t *cchReturnCount /* total char count after removal */,
					__in_ecount(cchTargets) LPCSTR pszTargets /* chars for removal */,
					int cchTargets)
{
	LPSTR psz = pszOrigin;
	LPSTR pszLast = NULL;
	
	// if we're not trimming anything, we're not doing any work
	if( *pszOrigin == 0 )
	{
		if( cchReturnCount ) 	*cchReturnCount = 1;
		return ERROR_SUCCESS;
	}

	if ( 0 != * ( pszOrigin + cchOrigin -1 ))
	{
		return ERROR_INVALID_PARAMETER;
	}
	// find beginning of trailing matches
	// by starting at beginning (DBCS aware)
	while( *psz != 0 )
	{
		if( StrChrA_SYS(pszTargets, cchTargets, *psz ) != NULL )
		{
			if( pszLast == NULL )
			{
				pszLast = psz;
			}
		}
		else
		{
			pszLast = NULL;
		}
		psz = CharNextExA(CP_ACP, psz, 0);			
	}

	if( pszLast != NULL )
	{
		// truncate at left-most matching character  			
		if ( (size_t)( pszLast- pszOrigin + 1) >= cchOrigin )
		{
			return ERROR_BUFFER_OVERFLOW;
		}
		*pszLast = '\0';
		if( cchReturnCount ) 	*cchReturnCount = pszLast- pszOrigin + 1 ;
	}
	else
	{
		if( cchReturnCount ) 	*cchReturnCount = cchOrigin;
	}
		
	return ERROR_SUCCESS;
}



// Remove all leading occurrences of any of the characters in string 'pszTargets'
DWORD StrTrimLeft_Sys( __inout_ecount(cchOrigin) LPSTR  pszOrigin /* input string */,
				size_t cchOrigin	/* total char count */, 
				__out size_t *cchReturnCount /* total char count after removal */,
				__in_ecount(cchTargets) LPCSTR pszTargets /* chars for removal */,
				int cchTargets)
{
	LPSTR psz = pszOrigin;
	// if we're not trimming anything, we're not doing any work
	if( *pszOrigin == 0 )
	{
		if( cchReturnCount ) 	 *cchReturnCount = 1;
		return ERROR_SUCCESS;
	}
	
	if ( 0 != * ( pszOrigin + cchOrigin -1 ))
	{		
		return ERROR_INVALID_PARAMETER;
	}

	while( (*psz != 0) && (StrChrA_SYS(pszTargets, cchTargets, *psz ) != NULL) )
	{
		psz = CharNextExA(CP_ACP, psz, 0);
	}

	size_t iFirst = 0, cchDataLength = 0;
	if( psz != pszOrigin )
	{			
		iFirst =  psz - pszOrigin ;
		if ( iFirst >= cchOrigin )
		{
			return ERROR_BUFFER_OVERFLOW;
		}

		cchDataLength =  cchOrigin - iFirst;
		memmove_s( pszOrigin, cchOrigin, psz, cchDataLength );			

		*(pszOrigin + cchDataLength) = '\0';
		if( cchReturnCount ) 	 * cchReturnCount = cchDataLength;
	}
	else
	{
		if( cchReturnCount ) 	 * cchReturnCount = cchOrigin;
	}
		
	return ERROR_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
//StrTrimBoth_Sys
//
// This function removes leading and trailing occurences of characters in a character set from a string in place according 
// default system locale, i.e. system code page. The function is not compatible with UNICODE string. 
//
//	Inputs:
//		[inout] LPSTR pszOrigin: 
//			The point to the input string.
//		[in] size_t cchOrigin: 		
//			The string buf size in chars, including null terminator. Max 1024.
//		[out] size_t *cchReturnCount
//			The string buf size in chars, including null terminator.
//		[in] LPCSTR pszTargets: 
//			The search character. Only single byte character is supported.
//			multiple byte character search should consider to use StrStrA_SYS
//			instead.
//		[in] int cchTargets:
//			number of characters in the character set.
//
//	Returns:
//	In success, returns ERROR_SUCCESS, otherwise a error code is returned.
//	Note:
//		This function does in-place replacement. If ERROR_BUFFER_OVERFLOW, data   
// 	in the in buffer might be corrupted during this function.
//	Example:
//		char base[]="   YukonSp1 \t";
//		unsigned char cs[]=" \t";
//		size_t cbout;
//		printf("base=\"%s\"[%d]\n", base, sizeof(base));
//		StrChrRemoveA_SYS(base, sizeof(base), &cbout, cs, sizeof(cs));
//		printf("base=\"%s\"[%d]\n", base, cbout);		
//		
// 	result:
//		base="   YukonSp1  "[14]
//		base_compact="YukonSp1"[9]
//


// Remove all leading and trailing occurrences of any of the characters in string 'pszTargets'
DWORD StrTrimBoth_Sys( __inout_ecount(cchOrigin) LPSTR  pszOrigin /* input string */,
				size_t cchOrigin	/* total char count */, 
				__out size_t *cchReturnCount /* total char count after removal */,
				__in_ecount(cchTargets) LPCSTR pszTargets /* chars for removal */,
				int cchTargets)
{
	DWORD ret = ERROR_SUCCESS;
	
	// if we're not trimming anything, we're not doing any work
	if( *pszOrigin == 0 )
	{
		if( cchReturnCount ) 	 *cchReturnCount = 1;
		return ERROR_SUCCESS;
	}

	if ( 0 != * ( pszOrigin + cchOrigin -1 ))
	{		
		return ERROR_INVALID_PARAMETER;
	}


	size_t cchSize;

	if( ERROR_SUCCESS != (ret = StrTrimRight_Sys(pszOrigin, 
							cchOrigin,
							&cchSize, 
							pszTargets, 
							cchTargets)))
	{
		return ret;
	};

	if( ERROR_SUCCESS != (ret = StrTrimLeft_Sys(pszOrigin, 
							cchSize,
							&cchSize, 
							pszTargets,
							cchTargets)))
	{
		return ret;
	};

	if( cchReturnCount ) 	 *cchReturnCount = cchSize;
	
	return ret;
}

// Function: wctrim
//
// Description: This function will trim inplace the given null terminated string.
//
void StrTrimLeftW_Sys(__inout LPWSTR str)
{
	if (str == NULL)
		return;

	// L Trim
	WCHAR* string = str;
	
	while (*string == L' ' || *string == L'\t' || *string == L'\n')
		string++;

	size_t len = wcslenInWChars(string);

	if (len == 0)
	{
		*str = L'\0';
		return;
	}

	// shift left
	WCHAR* tmp = str;
	while (*string != L'\0')
	{
		*tmp = *string;
		tmp++;
		string++;
	}
	*tmp = L'\0';	// terminate final string
}

////////////////////////////////////////////////////////////////////////////////
//	SNILoadSystemLibA
//
//	This function loads a DLL from the system directory.  
//	Adapted from SQL Server's CSECUtilities::DBLoadLibraryW().  
//
//	Inputs:
//		[in] LPCSTR szDllFileName: 
//			The name of the DLL.  Should be just the file name, not including 
//			the path, e.g. "dbmslpcn.dll".  
//
//	Returns:
//		if the call succeeds, the handle to the loaded library.  
//		else, null.
//
//	Note: The caller of this function should call SNI_SET_LAST_ERROR.
//
HMODULE SNILoadSystemLibA( LPCSTR szDllFileName )
{

	BidxScopeAutoSNI1(SNIAPI_TAG _T("szDllFileName: '%hs'\n"), szDllFileName);

	CHAR szFullName[MAX_PATH + 1];
	szFullName[0] = '\0';
	HMODULE hModule = NULL; 
	HRESULT hr = S_OK; 
	size_t cchDllFileName = 0;
	DWORD dwError = ERROR_SUCCESS;

	// Verify the DLL name length.  
	//
	hr = StringCchLengthA( 
		szDllFileName, 
		(sizeof(szFullName)/sizeof(szFullName[0])) - 1, 
		&cchDllFileName );
	if( FAILED(hr) )
	{
		dwError = static_cast<DWORD>(hr);
		BidTrace1(ERROR_TAG _T("StringCchLength.  szDllFileName: '%hs'\n"), szDllFileName);
		goto Exit; 
	}
	if( ( 0 == cchDllFileName ) ||
		( (sizeof(szFullName)/sizeof(szFullName[0])) - 1 <= cchDllFileName ) )
	{
		dwError = ERROR_INVALID_PARAMETER;
		BidTrace1(ERROR_TAG _T("StringCchLength.  cchDllFileName: %Iu\n"), 
			cchDllFileName);
		goto Exit; 
	}


	// Find the path of the system dll (the input is the file name only).  
	// Note: SQL Server uses empty path on failure.  
	//	
	szFullName[(sizeof(szFullName)/sizeof(szFullName[0])) - 1] = '\0'; 


	// On success GetSystemDirectory() returns number of TCHARs 
	//  excluding NULL-terminator.  
	// On insufficient buffer it returns number of TCHARs
	//  including NULL-terminator.
	// On failure it returns 0.  
	//
	UINT uicchWritten = 
		GetSystemDirectoryA( 
			szFullName, 
			static_cast<UINT>((sizeof(szFullName)/sizeof(szFullName[0])) - cchDllFileName - 1) ); 
	if( !uicchWritten )
	{
		dwError = GetLastError(); 
		BidTrace1(ERROR_TAG _T("GetSystemDirectory: %d{WINERR}\n"), dwError);
		goto Exit; 
	}
	if( ( (sizeof(szFullName)/sizeof(szFullName[0])) - cchDllFileName - 1 <= 
		uicchWritten ) )
	{
		dwError = ERROR_BUFFER_OVERFLOW;
		BidTrace1(ERROR_TAG _T("GetSystemDirectory.  Path too long: %d\n"), uicchWritten);
		goto Exit; 
	}


	// If we have a path
	//
	if( szFullName[0]
		// and the path is not just "\"
		//
		&& !( szFullName[0] == '\\' && !szFullName[1] ) )
	{
		// then we need to append a backslash to the path before we append the library name
		//
		HRESULT hr = StringCchCatA( 
			szFullName, 
			sizeof(szFullName)/sizeof(szFullName[0]), 
			"\\" );
		
		if( FAILED(hr) )
		{
			dwError = static_cast<DWORD>(hr);
			BidTrace1(ERROR_TAG _T("StringCchCat.  szFullName: '%hs' '\\'\n"), szFullName); 
			goto Exit; 
		}
	}

	// Copy/append the library name
	//
	hr = StringCchCatA( 
		szFullName, 
		sizeof(szFullName)/sizeof(szFullName[0]), 
		szDllFileName );
	
	if( FAILED(hr) )
	{
		dwError = ERROR_BUFFER_OVERFLOW;
		BidTrace2(ERROR_TAG _T("StringCchCat.  szFullName: '%hs' szDllFileName: '%hs'\n"), 
			szFullName, szDllFileName); 
		goto Exit; 	
	}
	
	// Call NT to load library into process
	//
	hModule = ::LoadLibraryA( szFullName );

	if( NULL == hModule )
	{
		dwError = GetLastError(); 
		BidTrace1(ERROR_TAG _T("LoadLibrary: %d{WINERR}\n"), dwError); 		
		goto Exit; 	
	}

	
Exit:
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%p{HMODULE}\n"), hModule);

	BidScopeAuto_Term(); 
	
	if ( ERROR_SUCCESS != dwError )
		SetLastError(dwError);	
	
	return hModule;
}

#ifndef SNI_BASED_CLIENT
void SNITraceErrorToConsumer(
	__in SNI_Conn * pConn,
	DWORD dwOSError, 
	DWORD dwSNIError)
{
	if (pConn && pConn->m_ConsumerInfo.fnTrace)
	{
		pConn->m_ConsumerInfo.fnTrace(pConn, dwOSError, dwSNIError);
	}
}
#else
void SNITraceErrorToConsumer(
	__in SNI_Conn * pConn,
	DWORD dwOSError, 
	DWORD dwSNIError)
{
	// No operation for clients
}
#endif

////////////////////////////////////////////////////////////////////////////////
//	BID extensions
//
BID_EXTENSION(ProtElem)
{
    BID_EXTENSION2_PTR(ProtElem, pProtElem);

	BidWrite3A( "m_ProviderNum: %d{ProviderNum}\n"
				"m_ProviderToReport: %d{ProviderNum}\n"
				"m_wszServerName: %s\n", 
				pProtElem->GetProviderNum(), 
				pProtElem->m_ProviderToReport, 
				pProtElem->m_wszServerName ); 
	
	switch( pProtElem->GetProviderNum() )
	{
		case TCP_PROV:
			BidWrite2A( "wszPort: %s\n"
						"fParallel: %d\n", 
						pProtElem->Tcp.wszPort, 
						pProtElem->Tcp.fParallel ); 
			break;

		case NP_PROV:
			BidWrite1A( "Pipe: %s\n", 
						pProtElem->Np.Pipe ); 
			break;

		case SM_PROV:
			BidWrite1A( "Alias: %s\n", 
						pProtElem->Sm.Alias ); 
			break;

		case VIA_PROV:
			BidWrite3A( "Port: %d\n"
						"Host: %s\n"
						"Param: %s\n", 
						pProtElem->Via.Port, 
						pProtElem->Via.Host, 
						pProtElem->Via.Param ); 
			break;

		default:
			break; 
	}
}


