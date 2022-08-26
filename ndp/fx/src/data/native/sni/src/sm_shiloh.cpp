//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: sm_shiloh.cpp
// @Owner: nantu, petergv
// @test: milu
//
// <owner current="true" primary="true">nantu</owner>
// <owner current="true" primary="false">petergv</owner>
//
// Purpose: SNI Shared Memory Wrapper for Shiloh connectivity
//
// Notes:
//	
// @EndHeader@
//****************************************************************************

#include "snipch.hpp"
#include "sm_shiloh.hpp"

typedef BOOL      (__cdecl * CONNECTIONVALIDSHAREDMEMORY_FN)( WCHAR * wszServerName );

static SNICritSec *      DllCritSec = NULL;
static HMODULE               hNetlib = NULL;
static NLFunctionTable       g_rgFuncs;

#define MAX_BUFFER_SIZE     0x7FFF

DWORD Sm_Shiloh::LoadDll(__out NLFunctionTable * pFuncs)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "pFuncs: %p{NLFunctionTable*}\n"), pFuncs);

	DWORD dwError;

	// Function Names for Wrapper Netlib Dlls - keep in sync with enum in .hpp
	CHAR * rgFnName[CLNT_NETFUNCS] =
	{
	    "ConnectionObjectSize",
	    "ConnectionOpen",
	    "ConnectionRead",
	    "ConnectionWrite",
	    "ConnectionClose",
	    "ConnectionCheckForData"
	};

	FARPROC rgFnPtr[CLNT_NETFUNCS];
	
	if( NULL == (hNetlib = SNILoadSystemLibraryA( "dbmslpcn.dll" )) )
	{
		dwError = GetLastError();
		SNI_SET_LAST_ERROR( SM_PROV, SNIE_36, dwError );
		goto ErrorExit;
	}

	int i;
	for( i = 0; i < CLNT_NETFUNCS; i++ )
	{
		if( NULL == (rgFnPtr[i] = GetProcAddress(hNetlib, rgFnName[i])) )
		{
			dwError = GetLastError();
			SNI_SET_LAST_ERROR( SM_PROV, SNIE_37, dwError );
			goto ErrorExit;
		}
	}

	memcpy(pFuncs, rgFnPtr, sizeof(NLFunctionTable));

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
		
	return ERROR_SUCCESS;

ErrorExit:

	if( hNetlib )
		FreeLibrary( hNetlib );
	hNetlib = NULL;
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
		
	return dwError;
}

Sm_Shiloh::Sm_Shiloh(SNI_Conn * pConn) : SNI_Provider(pConn)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "pConn: %p{SNI_Conn*}\n"), pConn );

	m_pConnObj = 0;
	m_Prot   = SM_PROV;

	BidObtainItemID2A( &m_iBidId, SNI_ID_TAG "%p{.} created by %u#{SNI_Conn}", 
		this, pConn->GetBidId() );
}

Sm_Shiloh::~Sm_Shiloh()
{
	BidRecycleItemIDA( &m_iBidId, SNI_ID_TAG ); 
}

DWORD Sm_Shiloh::Initialize(PSNI_PROVIDER_INFO pInfo)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "pInfo: %p{PSNI_PROVIDER_INFO}\n"), pInfo );
	
	// Fill all these up
    	pInfo->fBaseProv = TRUE;
	pInfo->Size = 0;
	pInfo->Offset = 0;
	pInfo->ProvNum = SM_PROV;

	DWORD dwError = SNICritSec::Initialize( &DllCritSec );

	if( ERROR_SUCCESS != dwError)
	{
		SNI_SET_LAST_ERROR( SM_PROV, SNIE_SYSTEM, dwError );
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);

	return dwError;
}

DWORD Sm_Shiloh::Terminate()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T( "\n") );
	
	if( hNetlib )
		FreeLibrary( hNetlib );
	hNetlib = NULL;

	if( DllCritSec )
	{
		DeleteCriticalSection( &DllCritSec );
	}

	Assert( NULL == DllCritSec ); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
		
	return ERROR_SUCCESS;
}

DWORD Sm_Shiloh::Open( 	SNI_Conn 		* pConn,
							ProtElem 		* pProtElem, 
							__out SNI_Provider 	** ppProv )
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("pConn: %p{SNI_Conn*}, ")
							  _T("pProtElem: %p{ProtElem*}, ")
							  _T("ppProv: %p{SNI_Provider**}\n"), 
					pConn, pProtElem, ppProv);

	Sm_Shiloh * pSm_ShilohProv = 0;
	DWORD dwRet = ERROR_FAIL;

	// If we failed to initialize the SM SHiloh shim, faill
	// all connections to Shiloh.  
	//
	if( NULL == DllCritSec )
	{
		dwRet = ERROR_INVALID_STATE;
		SNI_SET_LAST_ERROR( SM_PROV, SNIE_SYSTEM, dwRet );
		goto ErrorExit;
	}

	{
		CAutoSNICritSec a_csDll( DllCritSec, SNI_AUTOCS_DO_NOT_ENTER );

		pSm_ShilohProv = NewNoX(gpmo) Sm_Shiloh(pConn);
		if( !pSm_ShilohProv )
		{
			dwRet = ERROR_OUTOFMEMORY;
			SNI_SET_LAST_ERROR( SM_PROV, SNIE_SYSTEM, dwRet );
			goto ErrorExit;
		}

		// Set the handle to be invalid
		pSm_ShilohProv->m_hNwk = INVALID_HANDLE_VALUE;
			
		// Load the Dll, if necessary
		//
		a_csDll.Enter();

		if( NULL == hNetlib )
		{
			if( ERROR_SUCCESS != (dwRet = Sm_Shiloh::LoadDll(&g_rgFuncs)) )
			{
				a_csDll.Leave(); 
				goto ErrorExit;
			}
		}

		a_csDll.Leave(); 
	}

	// Check if Shared Memory is valid
	{
		// Get the instance
		LPWSTR wszInstance = NULL;

		wszInstance = StrChrW_SYS(pProtElem->Sm.Alias,(int) wcslen(pProtElem->Sm.Alias), L'\\' );

		if( wszInstance )
			wszInstance += 1;
		else
			wszInstance = L"MSSQLSERVER";
	
		CONNECTIONVALIDSHAREDMEMORY_FN ConnectionValidSharedMemory;

		// Let's make sure Shared-Memory Netlib is valid (correct version, matchup, etc.)
		
		ConnectionValidSharedMemory 
		= (CONNECTIONVALIDSHAREDMEMORY_FN)GetProcAddress( hNetlib, "ConnectionValidSharedMemory" );

		if( !ConnectionValidSharedMemory ||  !ConnectionValidSharedMemory(wszInstance) )
		{
			dwRet = ERROR_INVALID_PARAMETER;
			SNI_SET_LAST_ERROR( SM_PROV, SNIE_39, dwRet );
			goto ErrorExit;
		}

	}	

	pSm_ShilohProv->m_pConnObj = NewNoX (gpmo) BYTE[g_rgFuncs.Size()];
	if( !pSm_ShilohProv->m_pConnObj )
	{
		dwRet = ERROR_OUTOFMEMORY;
		SNI_SET_LAST_ERROR( SM_PROV, SNIE_SYSTEM, dwRet );
		goto ErrorExit;
	}	
		
	LONG Error = ERROR_SUCCESS;
	if( !g_rgFuncs.Open(pSm_ShilohProv->m_pConnObj, pProtElem->Sm.Alias, &Error) )
	{
		// If the netlibs function returned an error, use it.  
		// Otherwise, return a generic one.  
		if( ERROR_SUCCESS != Error )
		{
			dwRet = static_cast<DWORD>(Error);
		}
		else
		{
			dwRet = ERROR_NOT_CONNECTED;
		}

		SNI_SET_LAST_ERROR( SM_PROV, SNIE_38, dwRet );
		goto ErrorExit;
	}	

	BidUpdateItemIDA( pSm_ShilohProv->GetBidIdPtr(), SNI_ID_TAG 
		"connection: %p{ProtElem}", pProtElem );  
	
	// Set the out provider param to point to the new Sm_Shiloh object
	*ppProv = pSm_ShilohProv;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;
	
ErrorExit:

	if( pSm_ShilohProv )
	{
		if( pSm_ShilohProv->m_pConnObj )
			delete [] pSm_ShilohProv->m_pConnObj;

		delete pSm_ShilohProv;
	}


	*ppProv = NULL;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD Sm_Shiloh::ReadSync(__out SNI_Packet ** ppNewPacket, int timeout)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("ppNewPacket: %p{SNI_Packet**}, ")
							  _T("timeout: %d\n"), 
							  GetBidId(),
							  ppNewPacket, 
							  timeout );

	DWORD dwRet = ERROR_SUCCESS;

	*ppNewPacket = 0;
	
	// Check for 0 timeout - proceed only if there is data
	if( 0 == timeout)
	{
		LONG Error, cBytes;

		if( FALSE == g_rgFuncs.CheckData( m_pConnObj,
								              &cBytes,
								              &Error) )
		{
			if( Error )
			{
				SNI_SET_LAST_ERROR( SM_PROV, SNIE_1, static_cast<DWORD>(Error) );

				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), Error);
				return Error;
			}
			else
			{
				SNI_SET_LAST_ERROR( SM_PROV, SNIE_SYSTEM, WAIT_TIMEOUT );

				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), WAIT_TIMEOUT);
				return WAIT_TIMEOUT;
			}
		}

		// Since we already have data, call Read iwth infinite timeout
		timeout = -1;
	}
	
	// Allocate a new packet
	SNI_Packet  * pPacket = SNIPacketAllocate(m_pConn, SNI_Packet_Read);
	if( !pPacket )
	{
		dwRet = ERROR_OUTOFMEMORY;
		SNI_SET_LAST_ERROR( SM_PROV, SNIE_SYSTEM, dwRet );
		goto ErrorExit;
	}
	
	// Post the transport read
	DWORD 	dwBytesRead = 0;
	{
		LONG Error;
		#define NETE_TIMEOUT -2
		#define NET_IMMEDIATE	0xFFFF

		// Convert from Shiloh timeout format to Yukon
		// For Yukon, 
		//	-1 = INFINITE
		//	0 = ZERO
		// 	Others - MilliSecs
		// For Shiloh,
		//	NET_IMMEDIATE = ZERO
		//	0 = INFINITE
		// 	Others - Secs
		if( -1 == timeout )
			timeout = 0;
		else if( 0 == timeout )
			timeout = NET_IMMEDIATE;
		else
			timeout /= 1000;
		
		dwBytesRead = g_rgFuncs.Read( m_pConnObj,
				                   SNIPacketGetBufPtr(pPacket),
				                   (int)SNIPacketGetBufActualSize(pPacket),
				                   (int)SNIPacketGetBufActualSize(pPacket),
				                   (USHORT)timeout,
				                   &Error );
		if( Error == NETE_TIMEOUT )
		{
			dwRet = WAIT_TIMEOUT;
			SNI_SET_LAST_ERROR( SM_PROV, SNIE_SYSTEM, dwRet );
			goto ErrorExit;
		}
		
		if( 0 == dwBytesRead )
		{
			dwRet = Error;
			SNI_SET_LAST_ERROR( SM_PROV, SNIE_1, dwRet );
			goto ErrorExit;
		}
	}

	SNIPacketSetBufferSize( pPacket, dwBytesRead );

	*ppNewPacket = pPacket;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;

ErrorExit:

	if( pPacket )
		SNIPacketRelease( pPacket );

	*ppNewPacket = NULL;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;

}

DWORD Sm_Shiloh::WriteSync(__in SNI_Packet * pPacket, SNI_ProvInfo * pInfo)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("pInfo: %p{SNI_ProvInfo*}\n"), 
							  GetBidId(),
							  pPacket, 
							  pInfo );

	DWORD 	dwRet = ERROR_SUCCESS;

	// Check that the packetsize does not exceed the max packet size
	if( MAX_BUFFER_SIZE < SNIPacketGetBufferSize(pPacket) )
	{
		dwRet = ERROR_NOT_SUPPORTED;
		SNI_SET_LAST_ERROR( SM_PROV, SNIE_SYSTEM, dwRet );
		goto Exit;
	}
	
	// Post the transport write
	LONG Error;
	if( SNIPacketGetBufferSize(pPacket) != g_rgFuncs.Write( m_pConnObj,
							      SNIPacketGetBufPtr(pPacket),
							      (int)SNIPacketGetBufferSize(pPacket),
							      &Error ) )
	{
		dwRet = Error;
		SNI_SET_LAST_ERROR( SM_PROV, SNIE_1, dwRet );
	}

Exit:
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}                    

DWORD Sm_Shiloh::Close()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	DWORD dwRet = ERROR_SUCCESS;
	LONG Error;
	
	if( FALSE == g_rgFuncs.Close( m_pConnObj, &Error ) )
	{
		dwRet = Error;
		SNI_SET_LAST_ERROR( SM_PROV, SNIE_1, dwRet );
	}
	
	delete [] m_pConnObj;
	m_pConnObj = 0;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

void Sm_Shiloh::Release()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T( "\n") );

	delete this;
}

DWORD Sm_Shiloh::InitializeListener(HANDLE hSNIAcceptKey, void *pListen, HANDLE * pListenHandle)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T( "hSNIAcceptKey: %p{HANDLE}, pListen: %p, pListenHandle: %p{HANDLE*}\n"), 
					hSNIAcceptKey, pListen, pListenHandle );

	SNI_SET_LAST_ERROR( SM_PROV, SNIE_15, ERROR_FAIL );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_FAIL);
	
	return ERROR_FAIL;
}

DWORD Sm_Shiloh::TerminateListener(HANDLE hListener)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "hListener: %p{HANDLE}\n"), hListener );

	SNI_SET_LAST_ERROR( SM_PROV, SNIE_15, ERROR_FAIL );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_FAIL);
	
	return ERROR_FAIL;
}

DWORD Sm_Shiloh::ReadAsync(SNI_Packet ** ppNewPacket, LPVOID pPacketKey)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("ppNewPacket: %p{SNI_Packet**}, ")
							  _T("pPacketKey: %p\n"), 
							  GetBidId(),
							  ppNewPacket, 
							  pPacketKey );

	SNI_SET_LAST_ERROR( SM_PROV, SNIE_15, ERROR_FAIL );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_FAIL);
	
	return ERROR_FAIL;
}

DWORD Sm_Shiloh::WriteAsync(SNI_Packet * pPacket, SNI_ProvInfo * pInfo)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("pInfo: %p{SNI_ProvInfo*}\n"), 
							  GetBidId(),
							  pPacket, 
							  pInfo );

	SNI_SET_LAST_ERROR( SM_PROV, SNIE_15, ERROR_FAIL );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_FAIL);
	
	return ERROR_FAIL;
}                    

DWORD Sm_Shiloh::GatherWriteAsync(SNI_Packet * pPacket, SNI_ProvInfo * pInfo)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("pInfo: %p{SNI_ProvInfo*}\n"), 
							  GetBidId(),
							  pPacket, 
							  pInfo );

	SNI_SET_LAST_ERROR( SM_PROV, SNIE_15, ERROR_FAIL );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_FAIL);
	
	return ERROR_FAIL;
}                    

DWORD Sm_Shiloh::AcceptDone( SNI_Conn * pConn,
							LPVOID pAcceptKey,
							DWORD dwBytes,	
							DWORD dwError,
						    SNI_Provider ** ppProv,
						    LPVOID * ppAcceptInfo )
{
	BidxScopeAutoSNI6( SNIAPI_TAG _T("pConn:%p{SNI_Conn*}, ")
							  _T("pAcceptKey:%p, ")
							  _T("dwBytes:%d, ")
							  _T("dwError:%d, ")
							  _T("ppProv:%p{SNI_Provider**}, ")
							  _T("ppAcceptInfo:%p{LPVOID*}\n"), 
								pConn, pAcceptKey, dwBytes, dwError, ppProv, ppAcceptInfo);

	SNI_SET_LAST_ERROR( SM_PROV, SNIE_15, ERROR_FAIL );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_FAIL);
	
	return ERROR_FAIL;
}

DWORD Sm_Shiloh::ReadDone( SNI_Packet ** ppPacket, 
						   __out SNI_Packet ** ppLeftOver, 
						   DWORD         dwBytes, 
						   DWORD         dwError )
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

	*ppLeftOver = 0;

	// If an error has occurred, then let's return that error. 
	// Otherwise, let's force an error.  (Sm_Shiloh does not
	// treat any errors as "valid")
	//
	if( !dwError )
		dwError = ERROR_FAIL;

	SNI_SET_LAST_ERROR( SM_PROV, SNIE_1, dwError );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}

DWORD Sm_Shiloh::WriteDone(SNI_Packet ** ppPacket, DWORD dwBytes, DWORD dwError)
{
	BidxScopeAutoSNI4( SNIAPI_TAG _T("%u#, ")
							  _T("ppPacket: %p{SNI_Packet**}, ")
							  _T("dwBytes: %d, ")
							  _T("dwError: %d{WINERR}\n"), 
							  GetBidId(),
							  ppPacket, 
							  dwBytes, 
							  dwError);

	// If its an error, return the error. Sm_Shiloh does not treat any errors as "valid".
	if( dwError )
	{
		SNI_SET_LAST_ERROR( SM_PROV, SNIE_1, dwError );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
		return dwError;
	}

	SNI_SET_LAST_ERROR( SM_PROV, SNIE_1, ERROR_FAIL );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_FAIL);
	
	return ERROR_FAIL;
}


