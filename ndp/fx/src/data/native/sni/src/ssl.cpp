//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: ssl.cpp
// @Owner: petergv, nantu
// @Test: milu
//
// <owner current="true" primary="true">petergv</owner>
// <owner current="true" primary="false">nantu</owner>
//
// Purpose: SNI Providers for Encryption and Signing
//
// Notes:
//          
// @EndHeader@
//****************************************************************************
#include "snipch.hpp"
#include "ssl.hpp"
#include "tcp.hpp"

#include <schannel.h>
#include <security.h>

#ifndef SNI_BASED_CLIENT
#include "sos.h"
#endif

//
// Inform the BID infrastructure of the class hierarchy for
// CryptoBase since the BID ID's for Sign and Ssl objects
// are obtained in their CTOR's but the ID's are used in
// CryptoBase functions as well.  
//
BID_METATEXT( _T("<Alias|ItemTag> CryptoBase=Sign,Ssl") );

extern BOOL gfIsWin2KPlatform;

#ifdef SNI_UNIT
extern bool g_fUnitScenarios[];
#endif

extern ExtendedProtectionPolicy g_eExtendedProtectionLevel;

// Create 2 sets of flags to pass to InitializeSecurityContext, 
// one for auto validation of the SSL certificate by SSPI and
// second for manual validation of the ssl certificate by SNI
// In win2K and later systems, SSPI by default validates the
// server ssl certificate. For pre-win2K systems, we will have to
// manually validate ourselves by pasing the 
// ISC_REQ_MANUAL_CRED_VALIDATION flag.
//
#define SSL_CONTEXT_REQ (	ISC_REQ_SEQUENCE_DETECT	|\
							ISC_REQ_REPLAY_DETECT		|\
				                  	ISC_REQ_CONFIDENTIALITY   	|\
				                  	ISC_RET_EXTENDED_ERROR 	|\
				                  	ISC_REQ_STREAM				|\
		       		           	ISC_RET_USE_SESSION_KEY	 )

#define SSL_CONTEXT_REQ_MANUAL (	SSL_CONTEXT_REQ	|\
		       		           	ISC_REQ_MANUAL_CRED_VALIDATION )

#define SHA1HASHLENGTH 20	//sha1 hash length in bytes of binary representation.

extern WCHAR gwszComputerName[];

extern PSecurityFunctionTable g_pFuncs;
extern PCCERT_CONTEXT GetFallBackCertContext();

DWORD Ssl::s_cbMaxToken = 0;

UINT Ssl::s_cPagesforMaxToken = 0;

DWORD Ssl::s_fSslServerReady = FALSE;

PCCERT_CONTEXT Ssl::s_hCertContext = 0;

HCERTSTORE Ssl::s_hCertStore = NULL;

HMODULE Ssl::s_hSecurity = NULL;

CredHandle Ssl::s_hClientCred;
CredHandle Ssl::s_hClientCredValidate;
CredHandle Ssl::s_hServerCred;

PSecurityFunctionTable Ssl::s_pfTable = NULL;
bool Ssl::s_fChannelBindingsSupported = true;

BOOL g_fisWin9x = FALSE;
#ifdef SNIX
// On windows 9x system, Schannel is supported by schannel.dll, not the secur32.dll(nt4) or secure.dll (Nt5)
// The encryption and decryption function is different too here.

typedef SECURITY_STATUS (__stdcall *_DFN)(PCtxtHandle, PSecBufferDesc, unsigned long, unsigned long *);
typedef SECURITY_STATUS (__stdcall *_EFN)(PCtxtHandle, unsigned long, PSecBufferDesc, unsigned long );

_DFN Dfn;
_EFN Efn;

#endif

CryptoBase::~CryptoBase()
{
	Assert( !m_fReadInProgress );
	
	DeleteCriticalSection( &m_CS );

	DeleteCriticalSection( &m_WriteSyncCS );

	if( NULL != m_hHandshakeDoneEvent )
	{
		if( ! CloseHandle(m_hHandshakeDoneEvent) )
		{
			DWORD dwError = GetLastError();
			BidTrace1(ERROR_TAG _T("CloseHandle failed: %d{WINERR}"), dwError);
		}
		m_hHandshakeDoneEvent = NULL;
	}
	
	Assert( !m_pLeftOver );
}

//----------------------------------------------------------------------------
// NAME: CryptoBase::CallbackError
//  
// PURPOSE:
//		Sends error messages to completion port for pending read/write operations.
//
// PARAMETERS:
//		
// RETURNS:
//  
// NOTES:
//
//----------------------------------------------------------------------------

void CryptoBase::CallbackError()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	Assert( m_State == SSL_ERROR );

	while( !m_WritePacketQueue.IsEmpty() )
	{
		SNI_Packet *pPacket;

		pPacket = (SNI_Packet * ) m_WritePacketQueue.DeQueue( );

		pPacket->m_OrigProv = m_Prot;
		
		SNIPacketDecrementOffset( pPacket, m_cbHeaderLength );
		pPacket->m_cbTrailer = m_cbTrailerLength;
		SNIPacketSetBufferSize( pPacket, SNIPacketGetBufferSize( pPacket )+(m_cbHeaderLength+pPacket->m_cbTrailer ) );
		
		if( ERROR_SUCCESS != SNIPacketPostQCS(pPacket, 0 ) )
		{
			//this assertion is used to catch unexpected system call failure.
			Assert( 0 && "SNIPacketPostQCS failed\n" );
			BidTrace0( ERROR_TAG _T("SNIPacketPostQCS failed\n") );
		}

	}
	
	if( m_pLeftOver )
	{
		SNIPacketRelease( m_pLeftOver );

		m_pLeftOver = NULL;
	}

	if( !m_ReadPacketQueue.IsEmpty() )
	{
		SNI_Packet *pPacket;

		pPacket = (SNI_Packet * ) m_ReadPacketQueue.DeQueue( );

		pPacket->m_OrigProv = m_Prot;

		m_fReadInProgress = true;

		if( ERROR_SUCCESS != SNIPacketPostQCS( pPacket, 0 ) )
		{
			//this assertion is used to catch unexpected system call failure.
			Assert( 0 && "SNIPacketPostQCS failed\n" );
			BidTrace0( ERROR_TAG _T("SNIPacketPostQCS failed\n") );
		}
	}
}

DWORD CryptoBase::Close()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	CAutoSNICritSec a_cs( m_CS, SNI_AUTOCS_DO_NOT_ENTER );

	a_cs.Enter();
	
	Assert( !m_fClosed );

	DWORD dwRet;
	
	if( m_pLeftOver )
	{
		SNIPacketRelease( m_pLeftOver );

		m_pLeftOver = NULL;
	}

	m_fClosed = true;
	
	//
	// if still doing handshake, don't change the state
	// hanshake logic will check m_fClosed flag to handle this condition
	//
	
	if( m_State == SSL_DONE )
	{
		m_State = SSL_ERROR;
	}

	dwRet = m_pNext->Close();

	a_cs.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
			
	return dwRet;
}

CryptoBase::CryptoBase(SNI_Conn * pConn):SNI_Provider( pConn )
{
	m_fClosed = false;
	m_fReadInProgress = false;
	m_pLeftOver = NULL;
	m_CS = NULL;
	m_WriteSyncCS = NULL;
	m_dwSslNativeError = ERROR_SUCCESS;
	m_dwSslSNIError = SNIE_0;
	m_hHandshakeDoneEvent = NULL;
}

DWORD CryptoBase::FInit()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	DWORD dwRet;
	
	dwRet = SNICritSec::Initialize( &m_CS );

	if( ERROR_SUCCESS != dwRet )
	{
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
		return dwRet;
	}

	dwRet = SNICritSec::Initialize( &m_WriteSyncCS );

	if( ERROR_SUCCESS != dwRet )
	{
		DeleteCriticalSection( &m_CS );
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD CryptoBase::HandshakeReadDone( SNI_Packet *pPacket, DWORD dwError )
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("dwError: %d{WINERR}\n"), 
							  GetBidId(),
							  pPacket, 
							  dwError);
	//This assertion is used to claim this function is not implemented
	Assert( 0 && " This function is not implemented\n" );
	BidTrace0( ERROR_TAG _T("This function is not implemented\n") );

	SNI_SET_LAST_ERROR( m_Prot, SNIE_15, ERROR_FAIL );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_FAIL);
	
	return ERROR_FAIL;
}

DWORD CryptoBase::HandshakeWriteDone( SNI_Packet *pPacket, DWORD dwError )
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("dwError: %d{WINERR}\n"), 
							  GetBidId(),
							  pPacket, 
							  dwError);
	//This assertion is used to claim this function is not implemented
	Assert( 0 && " This function is not implemented\n" );
	BidTrace0( ERROR_TAG _T("This function is not implemented\n") );

	SNI_SET_LAST_ERROR( m_Prot, SNIE_15, ERROR_FAIL );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_FAIL);
	
	return ERROR_FAIL;
}

DWORD CryptoBase::HandshakeReadToken(SNI_Packet *pPacket)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}\n"),
							  GetBidId(),
							  pPacket);
	//This assertion is used to claim this function is not implemented
	Assert( 0 && " This function is not implemented\n" );
	BidTrace0( ERROR_TAG _T("This function is not implemented\n") );

	SNI_SET_LAST_ERROR( m_Prot, SNIE_15, ERROR_FAIL );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_FAIL);
	
	return ERROR_FAIL;
}

DWORD CryptoBase::Handshake( void )
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"),GetBidId());
	
	//This assertion is used to claim this function is not implemented
	Assert( 0 && " This function is not implemented\n" );
	BidTrace0( ERROR_TAG _T("This function is not implemented\n") );

	SNI_SET_LAST_ERROR( m_Prot, SNIE_15, ERROR_FAIL );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_FAIL);
	
	return ERROR_FAIL;
}

DWORD CryptoBase::HandshakeWriteToken(SNI_Packet *pPacket)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}\n"),
							  GetBidId(),
							  pPacket);
	//This assertion is used to claim this function is not implemented
	Assert( 0 && " This function is not implemented\n" );
	BidTrace0( ERROR_TAG _T("This function is not implemented\n") );

	SNI_SET_LAST_ERROR( m_Prot, SNIE_15, ERROR_FAIL );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_FAIL);
	
	return ERROR_FAIL;
}

//----------------------------------------------------------------------------
// NAME: CryptoBase::ProcessPendingIO
//  
// PURPOSE:
//		Initiates pending operations that have been queued up while doing handshake.
//
// PARAMETERS:
//		
// RETURNS:
//  
// NOTES:
//		This should be called only after a successful handshake
//
//----------------------------------------------------------------------------

void CryptoBase::ProcessPendingIO()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	DWORD dwRet;

	Assert( m_State == SSL_DONE );
	
	while( !m_WritePacketQueue.IsEmpty() )
	{
		SNI_Packet *pPacket;

		pPacket = (SNI_Packet * )m_WritePacketQueue.Peek();
		
		dwRet = Encrypt( pPacket );
		if( dwRet != ERROR_SUCCESS )
		{
			m_State = SSL_ERROR;	
			if ( m_dwSslNativeError == ERROR_SUCCESS )
			{
				m_dwSslNativeError = dwRet;
				m_dwSslSNIError = SNIE_19;
			}
			CallbackError();

			return;
		}
		else
		{
			dwRet = m_pNext->WriteAsync( pPacket, 0 );
			if( dwRet == ERROR_SUCCESS )
			{
				DWORD cPacket;
				cPacket = SNIPacketGetBufferSize( pPacket );

				pPacket->m_OrigProv = m_Prot;

				if( ERROR_SUCCESS != SNIPacketPostQCS( pPacket, cPacket ) )
				{
					//this assertion is used to catch unexpected system call failure.
					Assert( 0 && "SNIPacketPostQCS failed\n" );
					BidTrace0( ERROR_TAG _T("SNIPacketPostQCS failed\n") );
				}
			}
			else if( dwRet != ERROR_IO_PENDING )
			{
				SNIPacketIncrementOffset( pPacket, m_cbHeaderLength );
				Assert( 0 < pPacket->m_cbTrailer && pPacket->m_cbTrailer<=m_cbTrailerLength );
				SNIPacketSetBufferSize( pPacket, SNIPacketGetBufferSize( pPacket )-(m_cbHeaderLength+pPacket->m_cbTrailer ) );
				pPacket->m_cbTrailer = 0;
				m_State = SSL_ERROR;
				if ( m_dwSslNativeError == ERROR_SUCCESS )
				{
					m_dwSslNativeError = dwRet;
					m_dwSslSNIError = SNIE_19;
				}
				CallbackError();

				return;
			}

		}

		m_WritePacketQueue.DeQueue();
	}

	Assert( !m_fReadInProgress );

	if( !m_ReadPacketQueue.IsEmpty() )
	{
		SNI_Packet *pPacket;

		LPVOID pPacketKey;
		
		pPacketKey = SNIPacketGetKey( (SNI_Packet *) m_ReadPacketQueue.Peek() );
		


		//
		// if m_pLeftOver is not null we should use it
		//

		if( m_pLeftOver )
		{
			SNIPacketSetKey( m_pLeftOver, pPacketKey );
			
			pPacket = m_pLeftOver;

			m_pLeftOver = NULL;
			
			dwRet = ERROR_SUCCESS;
		}
		else
		{
			dwRet = m_pNext->ReadAsync( &pPacket, pPacketKey );
		}
		
		if( dwRet == ERROR_SUCCESS )
		{
			DWORD cPacket;
			cPacket = SNIPacketGetBufferSize( pPacket );
			SNIPacketSetBufferSize( pPacket, 0 );

			pPacket->m_OrigProv = m_Prot;
			
			SNIPacketRelease( (SNI_Packet *)m_ReadPacketQueue.DeQueue() );
			
			m_fReadInProgress = true;
			
			if( ERROR_SUCCESS != SNIPacketPostQCS( pPacket, cPacket ) )
			{
				//this assertion is used to catch unexpected system call failure.
				Assert( 0 && "SNIPacketPostQCS failed\n" );
				BidTrace0( ERROR_TAG _T("SNIPacketPostQCS failed\n") );
			}

		}
		else if( dwRet == ERROR_IO_PENDING )
		{
			SNIPacketRelease( (SNI_Packet *)m_ReadPacketQueue.DeQueue() );

			m_fReadInProgress = true;
		}
		else
		{
			m_State = SSL_ERROR;
			if ( m_dwSslNativeError == ERROR_SUCCESS )
			{
				m_dwSslNativeError = dwRet;
				m_dwSslSNIError = SNIE_19;
			}
			CallbackError();

			return;
		}

	}
}

//----------------------------------------------------------------------------
// NAME: CryptoBase::CopyLeftOver
//  
// PURPOSE:
//
//	Returns	the letf-over packet into a newly allocated read packet, returns
//	the new packet to the caller, and releases the original left-over packet.  
//
// PARAMETERS:
//
// ASSUMPTIONS:
//
//	- Called while holding the m_CS Critical section.  
//	- m_pLeftOver is non-NULL.  
//	- There are no providers below SSL that have non-zero headers or 
//	  trailers.  
//	- The SSL provider is used only by TDS.  
//	- If TDS removes the SSL provider it does it immediately after the first 
//	  login packet.  
//	- If a well-behaving TDS client sends a first login packet smaller than 
//	  4KB then the amount of data it can send before receiving the login
//	  response is no more than the minimum TDS packet size.  
//		
// RETURNS:
//  
// NOTES:
//
//----------------------------------------------------------------------------

DWORD CryptoBase::CopyLeftOver( __inout SNI_Packet * pLeftOver, 
								__deref_out SNI_Packet ** ppNewPacket, 
								__in LPVOID pPacketKey )
{
	DWORD dwRet = ERROR_SUCCESS;
	BYTE * pbLeftOver = NULL; 
	DWORD cbLeftOver = 0; 

	Assert( NULL == m_pLeftOver ); 

	*ppNewPacket = NULL; 

	// The packet sizes might have changed since the left-over
	// was stored.  We will allocate a new packet conforming
	// to the new sizes, and copy the data over.  That way
	// the buffer size of the returned packet is correct in 
	// case the upper layer will call PartialReadAsync() on it.  
	//
	SNI_Packet * pNewPacket = SNIPacketAllocate( m_pConn, SNI_Packet_Read );
		
	if( NULL == pNewPacket )
	{
		dwRet = ERROR_OUTOFMEMORY;
		goto ErrorExit;
	}

	// Get the pointer to the left-over data and its size.  
	//
	SNIPacketGetData( pLeftOver, &pbLeftOver, &cbLeftOver ); 

	Assert( NULL != pbLeftOver ); 
	Assert( cbLeftOver == SNIPacketGetBufferSize( pLeftOver ) ); 


	// SQL BU DT 373408 - for Yukon we assume that:
	//	- There are no providers below SSL that have non-zero headers or 
	//	  trailers (this guarantees that the check below ensures that
	//	  the amount of data delivered to consumer is <= the amount
	//	  requested by the consumer)
	//	- The SSL provider is used only by the TDS consumer.  
	//	- If TDS removes the SSL provider it does it immediately after 
	//	  the first login packet.  
	//	- If a well-behaving TDS client sends a first login packet smaller 
	//	  than 4KB then the amount of data it can send before receiving 
	//	  the login response is not more than the minimum TDS packet size.  
	//	  Examples are:
	//		- MDAC/SNAC client tagging an Attention packet to Login 
	//		  packet.  
	//		- DB-Lib client sending a 2-packet login (and possibly
	//		  an additional tagged Attention packet).  
	//
	//	  As a result we assume that any plaintext left-over received
	//	  from a well-behaved client fits into one packet according
	//	  to the buffer sizes at this point.  
	//
	//	  If the amount of data is larger we return an error.  
	//
	if( cbLeftOver > SNIPacketGetBufActualSize( pNewPacket ) )
	{
		BidTrace0(ERROR_TAG _T("Left-over is too large\n"));

		dwRet = ERROR_INVALID_DATA;
		goto ErrorExit;
	}

	// Copy all left-over data, and release the left-over packet.  
	// Note: it is important that read packets are allocated
	// with 0 offset.  Otherwise, we could overrun the
	// new packet's buffer. 
	//
	SNIPacketSetData( pNewPacket, pbLeftOver, cbLeftOver );

	SNIPacketRelease( pLeftOver ); 
	pLeftOver = NULL; 
	
	*ppNewPacket = (SNI_Packet *) pNewPacket;

	SNIPacketSetKey(*ppNewPacket, pPacketKey);

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

	return ERROR_SUCCESS; 


ErrorExit:

	SNIPacketRelease( pLeftOver ); 

	if( NULL != pNewPacket )
	{	
		SNIPacketRelease( pNewPacket ); 
	}

	SNI_SET_LAST_ERROR( SSL_PROV, SNIE_4, dwRet );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;
}


//
// DEVNOTE for CryptoBase::ReadAsync and CryptoBase::ReadSync concerning SDT#365303.
//
// SDT#365303 changes how SSL removes itself from the provider chain when there
// is a leftover packet held by CryptoBase while in SSL_REMOVED state.
//
// Previous behavior was SSL would stay in SSL_REMOVED state and continue to run
// in passthrough mode, forwarding all requests to the underlying provider.
//
// With SDT#365303 CryptoBase attempts to remove itself from the provider chain
// if it is in the SSL_REMOVED state when the first read is posted to CryptoBase provider.
//
// The key change with SDT#365303 is we now make 2 calls to SNIRemoveProvider on the 
// same connection. The first call will decrement the buffer sizes but NOT remove SSL 
// from the chain if there is a leftover packet in CryptoBase (CryptoBase::RemoveX will fail).  
// So CryptoBase goes into SSL_REMOVED state at this point.  Next the caller (TDS) will 
// post a read and CryptoBase will remove itself by calling SNIRemoveProvider a second
// time to remove itself from the provider chain and return the leftover packet to caller.
//
// Note that currently the only client of SNI that removes SSL is TDS.  SSL is removed when
// performing single 4K login packet encryption.
//
// Since the call to ReadSync while we're in SSL_REMOVED state happens very early on 
// during TDS login, even before the connection has a SPID, we feel this removal is safe.
// There should not be other threads reading the provider head from the connection this 
// early on in the connection lifetime.  However, in order to be safe we need to guarantee
// that the following functions are not called while we are in CryptoBase::ReadAsync/ReadSync
// in the SSL_REMOVED state:
//
// These functions are currently protected by SNIGetInfo ->
//
// Http::GetPeerAddress()
// Http::GetLocalAddress()
// Http::GetPeerPort()
// Http::GetLocalPort()
// Http::GetEndpoint()
// Tcp::GetPeerAddress
// Tcp::GetLocalAddress
// Tcp::GetLocalPort
// Tcp::GetPeerPort
//
// This function should not incur leftover packets and get into SSL_REMOVED state ->
//
// Sm::NpReadDone()
//
// CConnectionEndpoint does not use SSL currently ->
//
// CConnectionEndpoint::Close()
//
// These functions currently are safe because TDS does not post additional reads
// or writes before calling the first SNIReadAsync after SNIRemoveProvider ->
//
// SNIReadDone()
// SNIWriteDone()
// SNIReadAsync()
// SNIReadSync()
// SNIPartialReadAsync()
// SNIPartialReadSync()
// SNIWriteAsync()
// SNIWriteSync()
// SNIGatherWriteAsync()
//
// This function is not called by TDS more than once per SNI contract ->
//
// SNIRemoveProvider()
//
// These functions are only called after SSPI portion of login is completed, which
// currently is after the first SNIReadAsync is posted by TDS ->
// 
// SNIGetInfo(..., SNI_QUERY_CONN_CHANNEL_PROVIDES_AUTHENTICATION_CONTEXT, ...)
// SNISecImpersonateContext()
// SNISecRevertContext()
//
// Synchronization was specifically added to SNIClose to avoid provider head issues
// with this change.
//

DWORD CryptoBase::ReadAsync(__deref_inout SNI_Packet ** ppNewPacket, __in LPVOID pPacketKey)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("ppNewPacket: %p{SNI_Packet**}, ")
							  _T("pPacketKey: %p\n"), 
							  GetBidId(),
							  ppNewPacket, 
							  pPacketKey);

	DWORD dwRet;
	
	// Check to see if the provider was removed
	if( SSL_REMOVED == m_State )
	{
		// If so, check to see if there was a leftover packet
		PVOID pTmpPacket = 0;

		// 






		pTmpPacket = InterlockedCompareExchangePointer( (PVOID volatile*)&m_pLeftOver, 
														    (PVOID) 0,
														    (PVOID) m_pLeftOver);

		// If there was a leftover packet, return it
		if( pTmpPacket )
		{
			dwRet = CopyLeftOver( static_cast<SNI_Packet *>(pTmpPacket), 
								  ppNewPacket, 
								  pPacketKey ); 

			// We enter the SSL_REMOVED state if SNIRemoveProvider previously
			// failed to remove SSL from the provider chain.  Entering SSL_REMOVED
			// state only occurs if the SSL provider has a leftover packet.  
			// At this point, we have cleared the leftover packet from the provider,
			// so we now remove ourselves from the provider chain by calling 
			// SNIRemoveProvider again. The first call to SNIRemoveProvider has 
			// already adjusted the buffer sizes, so this second call will just remove
			// the SSL provider from the chain, getting us out of SSL_REMOVED state.
			//
			// Note: SNIRemoveProvider() must be called outside
			// this object's critical section because it will delete 
			// this object, so we must not access any of its members
			// anymore.  
			//
			SNIRemoveProvider( m_pConn, SSL_PROV );

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

			return dwRet;			
		}

		// Otherwise, act as passthrough
		dwRet = m_pNext->ReadAsync( ppNewPacket, pPacketKey );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

		return dwRet;
	}
	
	CAutoSNICritSec a_cs( m_CS, SNI_AUTOCS_DO_NOT_ENTER );

	a_cs.Enter();
	
	Assert( !m_fClosed );

	Assert( !*ppNewPacket );

	switch( m_State )
	{
		case SSL_ERROR:

			dwRet = m_dwSslNativeError == ERROR_SUCCESS ? ERROR_FAIL : m_dwSslNativeError ;
			SNI_SET_LAST_ERROR( m_Prot, m_dwSslSNIError == SNIE_0 ? SNIE_19 : m_dwSslSNIError, dwRet );

			break;

		case SSL_DONE:

			//
			// if there is a read operation in progress, queue the request
			//
			
			if( m_fReadInProgress )
			{
				SNI_Packet *pPacket;

				pPacket = SNIPacketAllocate( m_pConn, SNI_Packet_Read );

				if( NULL == pPacket )
				{
					dwRet = ERROR_OUTOFMEMORY;

					SNI_SET_LAST_ERROR( m_Prot, SNIE_4, dwRet );

					break;
				}

				SNIPacketSetKey( pPacket, pPacketKey);
			
				dwRet = m_ReadPacketQueue.EnQueue( pPacket );

				if( ERROR_SUCCESS == dwRet )
				{
					dwRet = ERROR_IO_PENDING;
				}
				else
				{
					SNIPacketRelease( pPacket );
				}

				break;
			}

			

			if( m_pLeftOver )
			{
				SNIPacketSetKey( m_pLeftOver, pPacketKey );

				*ppNewPacket = m_pLeftOver;

				m_pLeftOver = NULL;
			}

			dwRet = ERROR_SUCCESS;
			
			while( dwRet == ERROR_SUCCESS )
			{
				dwRet = Decrypt( *ppNewPacket, &m_pLeftOver );

				if( dwRet != SEC_E_INCOMPLETE_MESSAGE )
					break;
					
				Assert( NULL == m_pLeftOver );
				
				if( *ppNewPacket )
					dwRet = m_pNext->PartialReadAsync( *ppNewPacket,
														m_pConn->m_ConnInfo.ConsBufferSize
														+m_pConn->m_ConnInfo.ProvBufferSize
														-SNIPacketGetBufferSize( *ppNewPacket ),
														0 );
				else
					dwRet = m_pNext->ReadAsync( ppNewPacket, pPacketKey );

			}
			
			if( dwRet == ERROR_SUCCESS )
			{
				Assert( SNIPacketGetKey( *ppNewPacket ) == pPacketKey );
			}
			else if( dwRet==ERROR_IO_PENDING )
			{
				Assert( !m_pLeftOver );

				*ppNewPacket = NULL;

				m_fReadInProgress = true;
			}
			else
			{
				Assert( !m_pLeftOver );

				if( *ppNewPacket )
				{
					SNIPacketRelease( *ppNewPacket );

					*ppNewPacket = NULL;
				}
				
				m_State = SSL_ERROR;			
			}

			break;

		case SSL_INIT:
		case SSL_MORE:
		case SSL_LAST:

			SNI_Packet *pPacket;

			pPacket = SNIPacketAllocate( m_pConn, SNI_Packet_Read );

			if( NULL == pPacket )
			{
				dwRet = ERROR_OUTOFMEMORY;

				SNI_SET_LAST_ERROR( m_Prot, SNIE_4, dwRet );

				break;
			}
			
			SNIPacketSetKey( pPacket, pPacketKey);
			
			dwRet = m_ReadPacketQueue.EnQueue( pPacket );
			if (m_pConn->m_fClient)
			{
				if( ERROR_SUCCESS == dwRet )
				{
					dwRet = ERROR_IO_PENDING;
				}
				else
				{
					SNIPacketRelease( pPacket );
				}
			}
			else
			{

				if( ERROR_SUCCESS != dwRet )
				{
					SNIPacketRelease( pPacket );

					break;
				}

				//Fake a SSL packet so that TDS layer can handle SSL handshake upon 
				//notification.
				//
				BYTE pbTds[16];			       
				pbTds[0] = 0x12;	// Type set to 18 to indicate its a post 7.0 client prelogin
				pbTds[1] = 1;		//Last TDS packet.
				pbTds[2] = 0;
				pbTds[3] = sizeof(pbTds);		//Length is 16. Prelogin packet must be longer than 8.
				memset(pbTds+4, 0, sizeof(pbTds)-4);
				SNIPacketSetData( pPacket, pbTds, sizeof(pbTds));

				// the tds read packet is allocated and buffered. The underlaying provider
				// will create new buffer.
				//
				// At handshake stage, there must be only one read or only one write posted by TDS.
				//
				Assert( m_WritePacketQueue.IsEmpty());

				dwRet = HandshakeReadToken( 0 );	

				if( ERROR_IO_PENDING != dwRet )
				{
					// Read finishes immediately and return to the caller for notification.
					// Caller should release the packet if ERROR_SUCCESS; otherwise, release
					// the packet.
					//
					pPacket = (SNI_Packet *)m_ReadPacketQueue.DeQueue();

					Assert( pPacket );
				
					if( ERROR_SUCCESS == dwRet )
					{
						*ppNewPacket = pPacket;
						Assert( *ppNewPacket );
					}
					else
					{
						SNIPacketRelease( pPacket );
						pPacket = NULL;
						Assert (!*ppNewPacket );
					}				
				}

				Assert ( ERROR_SUCCESS!= dwRet || *ppNewPacket );
				
			}		
			break;
		
		default:
			// this assertion is used to catch coding errors.
			Assert( 0 && " SSL provider is in an unknown state\n" );
			BidTrace0( ERROR_TAG _T("SSL provider is in an unknown state \n") );
			dwRet = ERROR_INVALID_STATE;

			SNI_SET_LAST_ERROR( m_Prot, SNIE_10, dwRet );
	}
	
//ExitFunc:
	
	a_cs.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

//----------------------------------------------------------------------------
// NAME: CryptoBase::ReadDone
//  
// PURPOSE:
//		processes incoming data for both encryption and signing.
//		depending on m_State, packets can be decrypted or processed by handshake
//
// PARAMETERS:
//		ppPacket: input packet to be processed on return it will hold the packet to be passed to user(NULL when no data).
//		ppLeftOver: output packet which will be processed next time, note that this is not the user packet
//		dwBytes: not used
//		dwError: not used
//	
// RETURNS:
//		ERROR_SUCCESS if everything is fine.
//		Error code on failure.
//  
// NOTES:
//		this function will never return ERROR_IO_PENDING.
//
//----------------------------------------------------------------------------

DWORD CryptoBase::ReadDone(__deref_inout SNI_Packet ** ppPacket, __deref_inout SNI_Packet **ppLeftOver, __in DWORD dwBytes, __in DWORD dwError)
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

	DWORD dwRet;
	
	// Check to see if the provider was removed
	if( SSL_REMOVED == m_State )
	{
		// If so, act as passthrough
		dwRet = m_pNext->ReadDone( ppPacket, ppLeftOver, dwBytes, dwError );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

		return dwRet;
	}
	
	CAutoSNICritSec a_cs( m_CS, SNI_AUTOCS_DO_NOT_ENTER );

	a_cs.Enter();

	Assert( *ppLeftOver == 0 );

	if( (*ppPacket)->m_OrigProv!=m_Prot )
	{
		dwRet = m_pNext->ReadDone( ppPacket, ppLeftOver, dwBytes, dwError );
	}
	else
	{
		(*ppPacket)->m_OrigProv = INVALID_PROV;
		
		Assert( SNIPacketGetBufferSize( *ppPacket ) || m_State == SSL_ERROR );

		Assert( dwError==ERROR_SUCCESS );
		
		dwRet = dwError;
	}

	Assert( dwRet != ERROR_IO_PENDING );
	
	Assert( !*ppLeftOver );

	if( !*ppPacket )
	{
		goto ExitFunc;
	}

	Assert( !m_pLeftOver );
	
	switch( m_State )
	{
		case SSL_ERROR:

			if( dwRet == ERROR_SUCCESS )
			{
				dwRet = m_dwSslNativeError == ERROR_SUCCESS ? ERROR_FAIL : m_dwSslNativeError;
				SNI_SET_LAST_ERROR( m_Prot, m_dwSslSNIError == SNIE_0 ? SNIE_19 : m_dwSslSNIError, dwRet );			
				
			}
			
			//if there is a pending read request we need to get worker thread back
			if( !m_ReadPacketQueue.IsEmpty() )
			{
				Assert( !*ppLeftOver );

				*ppLeftOver = (SNI_Packet * ) m_ReadPacketQueue.DeQueue( );

				(*ppLeftOver)->m_OrigProv = m_Prot;
			}
			else
			{
				Assert( m_fReadInProgress );
				m_fReadInProgress = false;
			}

			break;

		case SSL_DONE:

			Assert( m_fReadInProgress );

			while( dwRet == ERROR_SUCCESS )
			{
				dwRet = Decrypt( *ppPacket, &m_pLeftOver );

				if( dwRet != SEC_E_INCOMPLETE_MESSAGE )
				{
					break;
				}
				
				Assert( NULL == m_pLeftOver );
				
				dwRet = m_pNext->PartialReadAsync( *ppPacket,
												m_pConn->m_ConnInfo.ConsBufferSize
												+m_pConn->m_ConnInfo.ProvBufferSize
												-SNIPacketGetBufferSize( *ppPacket ),
												0 );
			}
			
			if( dwRet == ERROR_IO_PENDING )
			{
				Assert( !m_pLeftOver );

				*ppPacket = NULL;

				dwRet = ERROR_SUCCESS;
				
				break;
			}
			
			if( dwRet != ERROR_SUCCESS )
			{
				Assert( !m_pLeftOver );

				m_State = SSL_ERROR;
			}

			if( m_ReadPacketQueue.IsEmpty() )
			{
				m_fReadInProgress = false;
			}
			else
			{
				Assert( !*ppLeftOver );

				SNI_Packet *pQueuedPacket;

				pQueuedPacket = ( SNI_Packet * ) m_ReadPacketQueue.DeQueue();
				
				if( m_pLeftOver )
				{
					SNIPacketSetKey( m_pLeftOver, SNIPacketGetKey( pQueuedPacket ) );

					SNIPacketRelease( pQueuedPacket );
					
					*ppLeftOver = m_pLeftOver;

					m_pLeftOver = NULL;

					(*ppLeftOver)->m_OrigProv = m_Prot;
				}
				else
				{
					DWORD dwErrorLeftOver;

					if( ERROR_SUCCESS == dwRet )
					{
						dwErrorLeftOver = m_pNext->ReadAsync( ppLeftOver, SNIPacketGetKey(pQueuedPacket) );
					}
					else
					{
						dwErrorLeftOver = ERROR_FAIL;
					}
					
					if( dwErrorLeftOver == ERROR_SUCCESS )
					{
						SNIPacketRelease( pQueuedPacket );

						(*ppLeftOver)->m_OrigProv = m_Prot;
					}
					else if( dwErrorLeftOver == ERROR_IO_PENDING )
					{
						SNIPacketRelease( pQueuedPacket );

						Assert( !*ppLeftOver );
					}
					else
					{
						Assert( 0 == SNIPacketGetBufferSize( pQueuedPacket ) );
						*ppLeftOver = pQueuedPacket;

						(*ppLeftOver)->m_OrigProv = m_Prot;

						m_State = SSL_ERROR;
						if ( dwRet == ERROR_SUCCESS )	
							dwRet = dwErrorLeftOver;
					}
				}
			}

			Assert( *ppPacket != m_pLeftOver && *ppPacket != *ppLeftOver );
			Assert( *ppLeftOver != m_pLeftOver || !m_pLeftOver );
			
			break;

		case SSL_INIT:
		case SSL_MORE:

			if( m_fClosed )
			{
				dwRet = ERROR_FAIL;
				if ( m_dwSslNativeError != ERROR_SUCCESS )
				{
					dwRet = m_dwSslNativeError;
				}
				else
				{
					m_dwSslNativeError = dwRet;
					m_dwSslSNIError = SNIE_20;
				}
				SNI_SET_LAST_ERROR( m_Prot, SNIE_20, dwRet);  // LPE should either use this or m_dwSslNativeError later.
			}
			
			if( ERROR_SUCCESS != dwRet )
			{
				SNIPacketRelease( *ppPacket );

				*ppPacket = NULL;
			}

			SNI_Packet *pPacket;

			pPacket = *ppPacket;
			*ppPacket = 0;

			if ( m_pConn->m_fClient )
			{
				dwRet = HandshakeReadDone(  pPacket, dwRet );
			}
			else
			{
				Assert( !m_ReadPacketQueue.IsEmpty()); 			//must has a buffered packet.
				
				// pPacket will be consumed by HandshakeReadToken. HandshakeReadToken
				// will SNI_SET_LAST_ERROR if error out.
				//
				if ( ERROR_SUCCESS == dwRet )
				{
					dwRet = HandshakeReadToken( pPacket );
				}
			}

			if( dwRet==ERROR_IO_PENDING )
			{
				dwRet = ERROR_SUCCESS;

				LONG cRef;

				cRef = m_pConn->Release( REF_InternalRead );

				Assert( cRef > 0 );
			}
			else
			{
				SetHandshakeDoneEvent();
				
				if (!m_pConn->m_fClient)
				{
					// notify TDS layer.

					*ppPacket = (SNI_Packet *)m_ReadPacketQueue.DeQueue();

					//Only one read is allowed to be posted by TDS during handshake
					Assert( m_ReadPacketQueue.IsEmpty());
	
					if ( ERROR_SUCCESS != dwRet)
					{
						m_State = SSL_ERROR;
					}
				}				

				// Save off the SNI connection pointer as our 
				// CryptoBase object may be deleted by the time
				// we are releasing a ref on it outside the 
				// critical section.  
				SNI_Conn * pConn = m_pConn; 
				
				a_cs.Leave(); 

				pConn->Release( REF_InternalRead );

				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
				return dwRet;
			}
			break;

		default:
			//this assertion is used to catch coding errors.
			Assert( 0 && " SSL provider is in an unknown state\n" );
			BidTrace0( ERROR_TAG _T("SSL provider is in an unknown state\n") );
			dwRet = ERROR_INVALID_STATE;

			SNI_SET_LAST_ERROR( m_Prot, SNIE_10, dwRet );
	};

ExitFunc:
	
	a_cs.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

//
// DEVNOTE for CryptoBase::ReadSync concerning SDT#365303.
//
// See devnote above CryptoBase::ReadAsync for details about possible conflicting
// concurrent function calls with CryptoBase::ReadSync when operating in the
// SSL_REMOVED state.
//

//----------------------------------------------------------------------------
// NAME: CryptoBase::ReadSync
//  
// PURPOSE:
//		reads and decrypts/verifys data
//
// PARAMETERS:
//		ppNewPacket: output packet which holds user data.
//		timeout: timeout in miliseconds
//	
// RETURNS:
//		ERROR_SUCCESS if everything is fine.
//		Error code on failure.
//  
// NOTES:
//
//----------------------------------------------------------------------------

DWORD CryptoBase::ReadSync(__deref_inout SNI_Packet ** ppNewPacket, __in int timeout)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("ppNewPacket: %p{SNI_Packet**}, ")
							  _T("timeout: %d\n"), 
							  GetBidId(),
							  ppNewPacket, 
							  timeout);

	DWORD dwRet;

	// Check to see if the provider was removed
	if( SSL_REMOVED == m_State )
	{
		// If so, check to see if there was a leftover packet
		PVOID pTmpPacket = 0;

		// 






		pTmpPacket = InterlockedCompareExchangePointer( (PVOID volatile*)&m_pLeftOver, 
														    (PVOID) 0,
														    (PVOID) m_pLeftOver);

		// If there was a leftover packet, return it
		if( pTmpPacket )
		{
			dwRet = CopyLeftOver( static_cast<SNI_Packet *>(pTmpPacket), 
								  ppNewPacket, 
								  NULL ); 

			// We enter the SSL_REMOVED state if SNIRemoveProvider previously
			// failed to remove SSL from the provider chain.  Entering SSL_REMOVED
			// state only occurs if the SSL provider has a leftover packet.  
			// At this point, we have cleared the leftover packet from the provider,
			// so we now remove ourselves from the provider chain by calling 
			// SNIRemoveProvider again. The first call to SNIRemoveProvider has 
			// already adjusted the buffer sizes, so this second call will just remove
			// the SSL provider from the chain, getting us out of SSL_REMOVED state.
			//
			// Note: SNIRemoveProvider() must be called outside
			// this object's critical section because it will delete 
			// this object, so we must not access any of its members
			// anymore.  
			//
			SNIRemoveProvider( m_pConn, SSL_PROV );

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

			return dwRet;			
		}

		// Otherwise, act as passthrough
		dwRet = m_pNext->ReadSync( ppNewPacket, timeout );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

		return dwRet;
	}
	
	CAutoSNICritSec a_cs( m_CS, SNI_AUTOCS_DO_NOT_ENTER );

	a_cs.Enter();

	// If the caller (SNI Consumer or SNI internals, though SNI internally currently never does this) is using both Sync and Async IO
	// on the same connection, it must drain all pending Async IO before it can initiate any Sync IO of the same type.
	Assert(m_ReadPacketQueue.IsEmpty());
	
	Assert( !m_fClosed );

	Assert( !*ppNewPacket );
	
	DWORD dwStart = 0;

	if( timeout!=INFINITE )
	{
		dwStart = GetTickCount();
	}
	
	int timeleft = timeout;
	
	if( m_pLeftOver )
	{
		*ppNewPacket = m_pLeftOver;

		m_pLeftOver = NULL;
	}

	dwRet = ERROR_SUCCESS;
	
	while( dwRet == ERROR_SUCCESS )
	{
		dwRet = Decrypt( *ppNewPacket, &m_pLeftOver );

		if( dwRet != SEC_E_INCOMPLETE_MESSAGE )
			break;
			
		Assert( NULL == m_pLeftOver );

		if( timeout!=INFINITE && timeleft<0 )
		{
			m_pLeftOver = *ppNewPacket;

			*ppNewPacket = NULL;
			
			dwRet = WAIT_TIMEOUT;

			SNI_SET_LAST_ERROR( m_Prot, SNIE_11, dwRet);
			
			goto ExitFunc;
		}

		if( *ppNewPacket )
		{
			dwRet = m_pNext->PartialReadSync( *ppNewPacket,
												m_pConn->m_ConnInfo.ConsBufferSize
												+m_pConn->m_ConnInfo.ProvBufferSize
												-SNIPacketGetBufferSize( *ppNewPacket ),
												timeleft );
			if( dwRet == WAIT_TIMEOUT )
			{
				m_pLeftOver = *ppNewPacket;

				*ppNewPacket = NULL;
			}
		}
		else
		{
			dwRet = m_pNext->ReadSync( ppNewPacket, timeleft );
		}
		
		if( timeout != INFINITE )
		{
			timeleft = timeout-(GetTickCount()-dwStart);
		}
	}	

ExitFunc:

	Assert( dwRet!=ERROR_SUCCESS || *ppNewPacket );

	if( dwRet != ERROR_SUCCESS && *ppNewPacket )
	{
		SNIPacketRelease( *ppNewPacket );

		*ppNewPacket = NULL;
	}

	a_cs.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

void CryptoBase::Release()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T( "\n") );
	
	m_pNext->Release();

	delete this;
}

DWORD CryptoBase::WriteAsync(__inout SNI_Packet * pPacket, __in SNI_ProvInfo * pProvInfo)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("pProvInfo: %p{SNI_ProvInfo*}\n"), 
							  GetBidId(),
							  pPacket, 
							  pProvInfo);

	DWORD dwRet;
	
	// Check to see if the provider was removed
	if( SSL_REMOVED == m_State )
	{
		// If so, act as passthrough
		
		dwRet = m_pNext->WriteAsync( pPacket, pProvInfo );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

		return dwRet;
	}
	
	CAutoSNICritSec a_cs( m_CS, SNI_AUTOCS_DO_NOT_ENTER );

	a_cs.Enter();

	Assert( !m_fClosed );

	switch( m_State )
	{
		case SSL_ERROR:
			{
				// HACK: SqlClient does not print OS error messages or OS error codes when an SNI Error other than
				// SNIE_0 is provided. So:
				// 1) if we have any native error, set the SNI Error to be SNIE_0 and set the native error
				// 2) otherwise, set the SNI Error to be SNIE_31 (handshake failed)
				DWORD dwSNIRet = (m_dwSslNativeError == ERROR_SUCCESS) ? SNIE_31 : SNIE_0;
				dwRet = (m_dwSslNativeError == ERROR_SUCCESS) ? ERROR_FAIL : m_dwSslNativeError;
				SNI_SET_LAST_ERROR( m_Prot, dwSNIRet, dwRet );
			}
			break;

		case SSL_DONE:

			dwRet = Encrypt( pPacket );

			if( ERROR_SUCCESS != dwRet )
			{
				m_State = SSL_ERROR;
				
				goto ExitFunc;
			}

			dwRet = m_pNext->WriteAsync( pPacket, pProvInfo );

			if( dwRet != ERROR_IO_PENDING )
			{
				SNIPacketIncrementOffset( pPacket, m_cbHeaderLength );

				SNIPacketSetBufferSize( pPacket, SNIPacketGetBufferSize( pPacket)-(m_cbHeaderLength+pPacket->m_cbTrailer) );

				pPacket->m_cbTrailer = 0;
			}
			
			break;

		case SSL_INIT:
		case SSL_MORE:
		case SSL_LAST:
			
			dwRet = m_WritePacketQueue.EnQueue( pPacket );

			if (m_pConn->m_fClient )
			{
				if( ERROR_SUCCESS == dwRet )
				{
					dwRet = ERROR_IO_PENDING;
				}
			}
			else
			{

				Assert( m_ReadPacketQueue.IsEmpty());

				if( ERROR_SUCCESS != dwRet )
				{
					m_State = SSL_ERROR;
				
					break;
				}

				dwRet = Handshake();
		
				if( ERROR_SUCCESS != dwRet )
				{
					//No need to SNI_SET_LAST_ERROR because Handshake is expected to 
					//do that. Set the state to SSL_ERROR.
					//
					m_State = SSL_ERROR;
				
					//no need to keep upper layer write packet around if no IO comletion is expected
					//
					m_WritePacketQueue.DeQueue();
				
					break;			
				}

				// Handshake can change the SSL state to DONE. Only continue with
				// HandshakeWriteToken when m_State is not SSL_DONE. Otherwise return.
				// 
				if( SSL_DONE != m_State )
				{
					dwRet = HandshakeWriteToken( 0 );
				}

				// If error other than ERROR_IO_PENDING, either ERROR_SUCCESS or real errors,
				// we don't expect any writedon happens. Otherwise, return ERROR_IO_PENDING 
				// to upper layer, and keep the packet for 
				// notification when writedone happens.
				//
				if( ERROR_IO_PENDING != dwRet )
				{
					m_WritePacketQueue.DeQueue();
				}
			
			}	
	
			break;
			
		default:

			dwRet = ERROR_INVALID_STATE;

			SNI_SET_LAST_ERROR( m_Prot, SNIE_10, dwRet);
	}
	
ExitFunc:

	a_cs.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD CryptoBase::SetHandshakeDoneEvent()
{
	DWORD dwRet = ERROR_SUCCESS;
	if( NULL != m_hHandshakeDoneEvent )
	{
		Assert( m_pConn->m_fClient && ! m_pConn->m_fSync );
		if( ! SetEvent(m_hHandshakeDoneEvent) )
		{
			dwRet = GetLastError();
			BidTrace1( ERROR_TAG _T("SetEvent failed: %d{WINERR}"), dwRet);
		}
	}
	return dwRet;
}

DWORD CryptoBase::WriteDone(__deref_inout SNI_Packet ** ppPacket, __in DWORD dwBytes, __in DWORD dwError)
{
	BidxScopeAutoSNI4( SNIAPI_TAG _T("%u#, ")
							  _T("ppPacket: %p{SNI_Packet**}, ")
							  _T("dwBytes: %d, ")
							  _T("dwError: %d{WINERR}\n"), 
							  GetBidId(),
							  ppPacket, 
							  dwBytes, 
							  dwError);

	DWORD dwRet;
	
	// Check to see if the provider was removed
	if( SSL_REMOVED == m_State )
	{
		// If so, act as passthrough
		dwRet = m_pNext->WriteDone( ppPacket,  dwBytes, dwError );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

		return dwRet;
	}
	
	CAutoSNICritSec a_cs( m_CS, SNI_AUTOCS_DO_NOT_ENTER );

	a_cs.Enter();

	if( (*ppPacket)->m_OrigProv != m_Prot )
	{
		dwRet = m_pNext->WriteDone( ppPacket,  dwBytes, dwError );
	}
	else
	{
		(*ppPacket)->m_OrigProv = INVALID_PROV;
		
		Assert( dwError==ERROR_SUCCESS );

		if( 0 == dwBytes )
		{
			dwRet = ERROR_FAIL;

			SNI_SET_LAST_ERROR( m_Prot, SNIE_31, dwRet);
		}
		else
		{
			dwRet = dwError;
		}
	}

	switch( m_State )
	{
		case SSL_ERROR:
		case SSL_DONE:

			SNIPacketIncrementOffset( *ppPacket, m_cbHeaderLength );
			Assert( 0 < (*ppPacket)->m_cbTrailer && (*ppPacket)->m_cbTrailer<=m_cbTrailerLength);

			SNIPacketSetBufferSize( *ppPacket, SNIPacketGetBufferSize(*ppPacket)-(m_cbHeaderLength+(*ppPacket)->m_cbTrailer));
			(*ppPacket)->m_cbTrailer = 0;

			break;

		case SSL_MORE:
		case SSL_LAST:
			
			if( m_fClosed )
			{
				dwRet = ERROR_FAIL;
				if ( m_dwSslNativeError != ERROR_SUCCESS )
				{
					dwRet = m_dwSslNativeError;
				}
				else
				{
					m_dwSslNativeError = dwRet;
					m_dwSslSNIError = SNIE_20;
				}

				SNI_SET_LAST_ERROR( m_Prot, SNIE_20, dwRet);  // LPE should either use this or m_dwSslNativeError later.

			}

			if( dwRet != ERROR_SUCCESS )
			{
				SNIPacketRelease( *ppPacket );

				*ppPacket = 0;
			}

			SNI_Packet *pPacket;

			pPacket = *ppPacket;
			*ppPacket = 0;

			if (m_pConn->m_fClient)
			{
				dwRet = HandshakeWriteDone( pPacket, dwRet );
			}
			else
			{
				Assert( m_ReadPacketQueue.IsEmpty());
				Assert( !m_WritePacketQueue.IsEmpty());  //At handshake stage, there must be only one write posted by TDS

				if( ERROR_SUCCESS == dwRet) 
				{
					dwRet = HandshakeWriteToken( pPacket );
				}
			}
		
			if( dwRet == ERROR_IO_PENDING )
			{
				dwRet = ERROR_SUCCESS;

				LONG cRef;

				cRef = m_pConn->Release( REF_InternalWrite );

				Assert( cRef > 0 );
			}
			else
			{
				if (!m_pConn->m_fClient)
				{
					// detach previously buffered upper layer packet and deliver the notification.
					*ppPacket = (SNI_Packet *)m_WritePacketQueue.DeQueue();				
				}
				
				// In case this object is deleted, buffer the SNI_Conn.
				SNI_Conn* pConn = m_pConn;

				SetHandshakeDoneEvent();
				
				a_cs.Leave(); 

				pConn->Release( REF_InternalWrite );

				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
				return dwRet;
			}
			break;

		default:
			//this assertion is used to catch coding errors.
			Assert( 0 && " SSL provider is in an unknonw state\n" );
			BidTrace0( ERROR_TAG _T("SSL provider is in an unknown sate\n") );

			dwRet = ERROR_INVALID_STATE;

			SNI_SET_LAST_ERROR( m_Prot, SNIE_10, dwRet );
	}

//ExitFunc:
	
	a_cs.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD CryptoBase::WriteSync(__inout SNI_Packet * pPacket, __in SNI_ProvInfo * pProvInfo)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("pProvInfo: %p{SNI_ProvInfo*}\n"), 
							  GetBidId(),
							  pPacket, 
							  pProvInfo);

	DWORD dwRet;
	
	// The async Write Packet queue should actually always be empty, and could actually be removed.
	// SqlClient, the only caller who was using it, will instead use SNIAddProviderEx when they get the new SNI,
	// and even on the old SNI, they always wait for the SSL handshake to complete before posting the next write.
	Assert(m_WritePacketQueue.IsEmpty());

	// Check to see if the provider was removed
	if( SSL_REMOVED == m_State )
	{
		// If so, act as passthrough
		dwRet = m_pNext->WriteSync( pPacket,  pProvInfo );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

		return dwRet;
	}
	
	CAutoSNICritSec a_csWriteSync( m_WriteSyncCS, SNI_AUTOCS_DO_NOT_ENTER );

	a_csWriteSync.Enter();

	Assert( !m_fClosed );
	Assert( SSL_DONE == m_State || SSL_ERROR == m_State );

	// WriteSync may be called after an asynchronous SSL handshake completion, so the caller may not know 
	// about the handshake error. So, inform the caller about the error, if the handshake is in an error state.
	if( SSL_ERROR == m_State )
	{
		// HACK: SqlClient does not print OS error messages or OS error codes when an SNI Error other than
		// SNIE_0 is provided. So:
		// 1) if we have any native error, set the SNI Error to be SNIE_0 and set the native error
		// 2) otherwise, set the SNI Error to be SNIE_31 (handshake failed)
		DWORD dwSNIRet = (m_dwSslNativeError == ERROR_SUCCESS) ? SNIE_31 : SNIE_0;
		dwRet = (m_dwSslNativeError == ERROR_SUCCESS) ? ERROR_FAIL : m_dwSslNativeError;
		SNI_SET_LAST_ERROR( m_Prot, dwSNIRet, dwRet );
		goto ExitFunc;
	}

	dwRet = Encrypt( pPacket );

	if( ERROR_SUCCESS != dwRet )
	{
		Assert( pPacket->m_cbTrailer == 0);
		goto ExitFunc;
	}
	
	dwRet = m_pNext->WriteSync( pPacket, pProvInfo );

	Assert( 0 < pPacket->m_cbTrailer && pPacket->m_cbTrailer<=m_cbTrailerLength );
	SNIPacketSetBufferSize( pPacket, SNIPacketGetBufferSize(pPacket)-(m_cbHeaderLength+pPacket->m_cbTrailer) );

	pPacket->m_cbTrailer = 0;

	SNIPacketIncrementOffset( pPacket, m_cbHeaderLength );

ExitFunc:

	a_csWriteSync.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD CryptoBase::RemoveX()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	DWORD dwRet = ERROR_SUCCESS;

	// VSTF DevDiv 201265: 
	// Take and leave the CritSec, to allow other calls in this object 
	// to complete their atomic work, before allowing the 
	// object to be destroyed. There should be no calls going on at 
	// this time *except* for internal IOC callbacks from the handshake,
	// which all must already be holding the CS at the time 
	// SNIRemoveProvider was called
	// (for example, if WriteDone completed the handshake, but has
	// not yet exited the CS it was holding; we wait for that to be exited)
	{
		CAutoSNICritSec a_cs( m_CS, SNI_AUTOCS_ENTER );
	}

	m_fActive = FALSE;
	
	// Check if there is a leftover packet. If there is one, and
	// the provider is being removed return ERROR_FAIL, so
	// SNIRemoveProvider knows to keep this provider lying around
	// (rather than remove it from the chain)
	// Note: Since there should be no other operations during a
	// RemoveProvider, its okay to check m_pLeftOver outside of an
	// atomic operation
	if( m_pLeftOver )
	{
		// Set state to SSL_REMOVED
		m_State = SSL_REMOVED;

		dwRet = ERROR_FAIL;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

Sign::~Sign()
{
	BidRecycleItemIDA( &m_iBidId, SNI_ID_TAG ); 

	if( m_hKey )
		CryptDestroyKey( m_hKey );

	if( m_hCryptProv )
		CryptReleaseContext( m_hCryptProv, 0 );
}

//
// pPacket will be decrypted in place, 
// *ppLeftOver is output parameter which will hold extra data if there is any
//

DWORD Sign::Decrypt( __inout SNI_Packet *pPacket, __deref_out SNI_Packet **ppLeftOver )
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T( "pPacket: %p{SNI_Packet*}, ")
							  _T("ppLeftOver: %p{SNI_Packet**}\n"), 
							  GetBidId(),
							  pPacket, 
							  ppLeftOver);

	DWORD dwRet;
	
	Assert( !*ppLeftOver );

	if( !pPacket )
	{
		dwRet = SEC_E_INCOMPLETE_MESSAGE;

		goto ExitFunc;
	}

	Assert( 0 == pPacket->m_OffSet );

	BYTE *	pBuf;
	DWORD 	cBuf;
	DWORD 	cTotal;

	SNIPacketGetData( pPacket, &pBuf, &cTotal );

	if( cTotal < (m_cbHeaderLength+m_cbTrailerLength) || 
		cTotal < ( cBuf = *(DWORD *)pBuf ) )
	{
		dwRet = SEC_E_INCOMPLETE_MESSAGE;

		goto ExitFunc;
	}

	if( m_SeqNumR++ 
		!= *(UNALIGNED ULONGLONG *)( pBuf + cBuf - m_cbTrailerLength ) )
	{
		dwRet = SEC_E_OUT_OF_SEQUENCE;

		SNI_SET_LAST_ERROR( SIGN_PROV, SNIE_10, dwRet );

		goto ExitFunc;
	}
	
	dwRet = VerifySignature( pBuf,  cBuf - s_cbSignLength );

	if( ERROR_SUCCESS != dwRet )
	{
		goto ExitFunc;
	}

	if( cBuf < cTotal )
	{
		*ppLeftOver = SNIPacketAllocate( m_pConn, SNI_Packet_Read );

		if( !*ppLeftOver )
		{
			dwRet = ERROR_OUTOFMEMORY;

			SNI_SET_LAST_ERROR( SIGN_PROV, SNIE_4, dwRet );

			goto ExitFunc;
		}

		SNIPacketSetData( *ppLeftOver, pBuf + cBuf, cTotal - cBuf );
	}

	SNIPacketIncrementOffset( pPacket, m_cbHeaderLength );

	SNIPacketSetBufferSize( pPacket, cBuf - ( m_cbHeaderLength + m_cbTrailerLength ) );

ExitFunc:

	Assert( ERROR_SUCCESS == dwRet || !*ppLeftOver );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD Sign::Encrypt( __inout SNI_Packet *pPacket)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("%u#, ")
							  _T( "pPacket: %p{SNI_Packet*}\n"), 
							  GetBidId(),
							  pPacket);

	DWORD dwRet = ERROR_SUCCESS;

	BYTE *pBuf;
	DWORD cBuf;
	
	SNIPacketGetData( pPacket, &pBuf, &cBuf );

	SNIPacketDecrementOffset( pPacket, m_cbHeaderLength );

	//fill length
	*(DWORD *)(pBuf-m_cbHeaderLength) = m_cbHeaderLength+cBuf+m_cbTrailerLength;

	//put sequence number
	OACR_WARNING_SUPPRESS(BUFFER_OVERFLOW, "The code for Sign Provider is deprecated, The code might already gurantee we are safe to access the trailer. ")
	*(UNALIGNED ULONGLONG *)(pBuf+cBuf) = m_SeqNumS++;

	dwRet = MakeSignature( pBuf-m_cbHeaderLength, 
							m_cbHeaderLength+cBuf+sizeof(ULONGLONG) );
	if( ERROR_SUCCESS != dwRet )
	{
		SNIPacketIncrementOffset( pPacket, m_cbHeaderLength );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
		return dwRet;
	}
		
	pPacket->m_cbTrailer = m_cbTrailerLength;

	SNIPacketSetBufferSize( pPacket, m_cbHeaderLength+cBuf+pPacket->m_cbTrailer );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD Sign::Initialize( PSNI_PROVIDER_INFO pInfo )
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("pInfo: %p{PSNI_PROVIDER_INFO}\n"), pInfo );
	
	pInfo->ProvNum = SIGN_PROV;
	pInfo->Offset = s_cbHeaderLength;
	pInfo->fBaseProv = FALSE;
	pInfo->Size = s_cbHeaderLength+s_cbTrailerLength;	//header+seqnum+trailer
	pInfo->fInitialized = TRUE; 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;
}

DWORD Sign::InitX( __in PCtxtHandle h)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("%u#, ")
							  _T("pInfo: %p{PCtxtHandle}\n"), 
							  GetBidId(),
							  h );
	
	DWORD dwRet = ERROR_SUCCESS;

	hContext = h;
	
//ErrorExit:

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD Sign::MakeSignature( BYTE *pBuf, DWORD cBuf )
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pBuf: %p{BYTE*}, ")
							  _T("cBuf: %d\n"), 
							  GetBidId(),
							  pBuf, 
							  cBuf );
	
	SecBuffer       Buffers[2];
	SecBufferDesc   Message;

	Buffers[0].pvBuffer = pBuf;
	Buffers[0].cbBuffer = cBuf;
	Buffers[0].BufferType = SECBUFFER_DATA;

	Buffers[1].pvBuffer = pBuf+cBuf;
	Buffers[1].cbBuffer = s_cbSignLength;
	Buffers[1].BufferType = SECBUFFER_TOKEN;

	Message.ulVersion = SECBUFFER_VERSION;
	Message.cBuffers = 2;
	Message.pBuffers = Buffers;

	DWORD dwRet;

	dwRet = g_pFuncs->MakeSignature(	hContext, 
									0, 
									&Message, 
									0 );
	if( dwRet != ERROR_SUCCESS )
	{
		SNI_SET_LAST_ERROR( SIGN_PROV, SNIE_10, dwRet );
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

Sign::Sign(SNI_Conn * pConn):CryptoBase( pConn )
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "pConn: %p{SNI_Conn*}\n"), pConn );
	
	m_Prot = SIGN_PROV;

	m_State = SSL_DONE;

	m_cbHeaderLength = s_cbHeaderLength;
	m_cbTrailerLength = s_cbTrailerLength;
	
	m_hKey = 0;
	m_hCryptProv = 0;

	m_SeqNumS = 1;
	m_SeqNumR = 1;

	BidObtainItemID2A( &m_iBidId, SNI_ID_TAG "%p{.} created by %u#{SNI_Conn}", 
		this, pConn->GetBidId() );
}

DWORD Sign::Terminate()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T( "\n") );

	rgProvInfo[SIGN_PROV].fInitialized = FALSE; 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;
}

DWORD Sign::VerifySignature( BYTE *pBuf, DWORD cBuf )
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pBuf: %p{BYTE*}, ")
							  _T("cBuf: %d\n"), 
							  GetBidId(),
							  pBuf, 
							  cBuf );
	
	SecBuffer       Buffers[2];
	SecBufferDesc   Message;

	// Initialize Security buffers
	Message.ulVersion = SECBUFFER_VERSION;
	Message.cBuffers = 2;
	Message.pBuffers = Buffers;

	Buffers[0].pvBuffer = pBuf;
	Buffers[0].cbBuffer = cBuf;
	Buffers[0].BufferType = SECBUFFER_DATA;

	Buffers[1].pvBuffer = pBuf+cBuf;
	Buffers[1].cbBuffer = s_cbSignLength;
	Buffers[1].BufferType = SECBUFFER_TOKEN;

	DWORD dwRet;

	dwRet = g_pFuncs->VerifySignature(	hContext, 
									&Message,
									0,
									0);

	if( dwRet != ERROR_SUCCESS )
	{
		SNI_SET_LAST_ERROR( SIGN_PROV, SNIE_10, dwRet );
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;
}

////////////////////////////////////////////////////////////////////////////////
//Bytes2Text
//
//This function converts byte array into Unicode char form.
//
//	Inputs:
//		[in] const BYTE* pBytes
//			The byte array.
//		[in] DWORD cbBytes
//			The byte array size in byte.
//		[out]  char* pText
//			The converted unicode text, not null-terminated. Caller should provide enough buffer and null terminate
//		the text when needed.
//		[in, out] DWORD* pcbText
//			Caller use pcbText to specify the buffer size of pText, this function will update the pcbText indicating
//		number of chars filled by conversion.
//   Note: this function only convert up pBytes to min(cbBytes, pcbText/2).
//
const WCHAR	HexChars[] = L"0123456789ABCDEF";
DWORD Bytes2Text(__in_bcount(cbBytes) const BYTE* pBytes, __in DWORD cbBytes, __out_bcount_part(*pcbText, *pcbText) WCHAR* pwchText, __inout DWORD* pcchText)
{
	BidxScopeAutoSNI4( SNIAPI_TAG _T("pBytes{BYTE*}: %p,")
								 _T("cbBytes: %d,")
 								 _T("pwchText{WCHAR*}: %p,")
								 _T("*pcchText: %d\n"),
								 pBytes,
								 cbBytes,
								 pwchText,
								 *pcchText);
	DWORD byteindex;

	for( byteindex = 0; byteindex < cbBytes && byteindex < *pcchText/2; byteindex++ )
	{
		pwchText[2*byteindex] =  HexChars[pBytes[byteindex]>>4 & 0xF];
		pwchText[2*byteindex+1] =  HexChars[pBytes[byteindex] & 0xF];		
	}

	*pcchText = byteindex*2;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	return ERROR_SUCCESS;
}


Ssl::~Ssl()
{
	FreeReadWriteBuffers();

	BidRecycleItemIDA( &m_iBidId, SNI_ID_TAG ); 

	if( SecIsValidHandle( &m_hContext ))
	{
		s_pfTable->DeleteSecurityContext( &m_hContext );

		SecInvalidateHandle( &m_hContext);
	}

	if( SecIsValidHandle( &m_hCred ))
	{
		Assert( m_pConn->m_fClient && !m_fUseExistingCred );
		s_pfTable->FreeCredentialsHandle(&m_hCred );

		SecInvalidateHandle( &m_hCred );
	}
}

void Ssl::CallbackError()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	FreeReadWriteBuffers();

	CryptoBase::CallbackError();
}

DWORD Ssl::AllocateReadWriteBuffers()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );
	DWORD dwRet  = ERROR_SUCCESS;

	Assert ( !m_pWriteBuffer );
	Assert ( !m_pReadBuffer );

#ifdef SNI_BASED_CLIENT
	m_pWriteBuffer = NewNoX(gpmo) BYTE [s_cbMaxToken];
	m_pReadBuffer = NewNoX(gpmo) BYTE [s_cbMaxToken];
#else

	//
	// SBT: 380512
	// In sqlserve.exe, memory allocation bigger than 8k, should
	// uses Memoryclerk directly. Otherwise, it would cause memory allocation
	// contention.
	//
	//
	
	MemoryClerk* memoryClerk = SOS_Node::GetNode(m_pConn->m_NodeId)->GetMemoryClerkByIndex ( MEMORYCLERK_SNI );

	if ( NULL == memoryClerk )
	{
		goto ErrorExit;
	}

	Assert ( s_cPagesforMaxToken );

	m_pWriteBuffer = (BYTE*) memoryClerk->AllocatePages (
										s_cPagesforMaxToken);

	m_pReadBuffer = (BYTE*) memoryClerk->AllocatePages (
										s_cPagesforMaxToken);


ErrorExit:
	
#endif

	if( !m_pWriteBuffer || !m_pReadBuffer )
	{
		dwRet = ERROR_OUTOFMEMORY;

		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_4, dwRet );
		
		FreeReadWriteBuffers();
	
	}
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

void Ssl::FreeReadWriteBuffers()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );
#ifndef SNI_BASED_CLIENT

	//
	// SBT: 380512
	// See comments in Ssl::AllocateReadWriteBuffers.
	//

	Assert ( s_cPagesforMaxToken );
	
	// get the handle of the memoryclerk from the node where the memory is allocated. 
	MemoryClerk* memoryClerk = SOS_Node::GetNode(m_pConn->m_NodeId)->GetMemoryClerkByIndex ( MEMORYCLERK_SNI );


	if ( m_pWriteBuffer != NULL )
	{
		memoryClerk->FreePages (m_pWriteBuffer, s_cPagesforMaxToken );
	}

	if ( m_pReadBuffer != NULL )
	{
		memoryClerk->FreePages (m_pReadBuffer, s_cPagesforMaxToken );
	}
#else
	if ( m_pWriteBuffer != NULL )
	{
		delete [] m_pWriteBuffer;
	}
	
	if ( m_pReadBuffer != NULL )
	{
		delete [] m_pReadBuffer;
	}
	
#endif

	m_pWriteBuffer = NULL;
	m_pReadBuffer = NULL;

}


DWORD Ssl::AcquireCredentialsForClient( PCredHandle pHandle )
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "pHandle: %p{PCredHandle}\n"), pHandle );

	SCHANNEL_CRED	SchannelCred;

	// Initialize SchannelCred
	memset(&SchannelCred, 0, sizeof(SchannelCred));
	SchannelCred.dwVersion = SCHANNEL_CRED_VERSION;
	SchannelCred.grbitEnabledProtocols = SP_PROT_SSL3TLS1_X;
	SchannelCred.dwFlags |= SCH_CRED_NO_DEFAULT_CREDS;
	//	If we need to validate the server certificate, then we'd rather 
	//	let schannel validate it for us. However the schannel flag to 
	//	validate the server certificate does not work on pre-win2K
	//	platforms, so on those platforms, we will do the validation
	//	ourselves.
	if (gfIsWin2KPlatform)
	{
		SchannelCred.dwFlags |= SCH_CRED_AUTO_CRED_VALIDATION;
	}
	else
	{
		SchannelCred.dwFlags |= SCH_CRED_MANUAL_CRED_VALIDATION;
	}
	
	DWORD dwRet;
	
	dwRet = s_pfTable->AcquireCredentialsHandle(
						NULL,                   // Name of principal
						UNISP_NAME,
						SECPKG_CRED_OUTBOUND,   // Flags indicating use
						NULL,                   // Pointer to logon ID
						&SchannelCred,          // Package specific data
						NULL,                   // Pointer to GetKey() func
						NULL,                   // Value to pass to GetKey()
						pHandle,                // (out) Cred Handle
						NULL);             // (out) Lifetime (optional)

	if( dwRet != ERROR_SUCCESS )
	{
		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, dwRet );
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;
}

DWORD Ssl::AcquireCredentialsForServer(PCCERT_CONTEXT pCertContext)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "pHandle: %p{PCCERT_CONTEXT}\n"), pCertContext );
	
	//
	// Build Schannel credential structure.
	//

	SCHANNEL_CRED   SchannelCred;

	ZeroMemory(&SchannelCred, sizeof(SchannelCred));
	SchannelCred.dwVersion = SCHANNEL_CRED_VERSION;
	SchannelCred.cCreds = 1;
	SchannelCred.paCred = &pCertContext;
	SchannelCred.grbitEnabledProtocols = SP_PROT_SSL3TLS1_X;

	TimeStamp       tsExpiry;

	DWORD dwRet;

	dwRet = s_pfTable->AcquireCredentialsHandle(
					NULL,                   // Name of principal
					UNISP_NAME,
					SECPKG_CRED_INBOUND,		// Flags indicating use
					NULL,                   // Pointer to logon ID
					&SchannelCred,          // Package specific data
					NULL,                   // Pointer to GetKey() func
					NULL,                   // Value to pass to GetKey()
					&s_hServerCred,                // (out) Cred Handle
					&tsExpiry);             // (out) Lifetime (optional)

	if( SEC_E_OK != dwRet )
	{
		scierrlog(26010, dwRet);

		SNI_SET_LAST_ERROR(SSL_PROV, SNIE_SYSTEM, dwRet);
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

//
// pPacket will be decrypted in place, 
// *ppLeftOver is output parameter which will hold extra data if there is any
//

DWORD Ssl::Decrypt( __inout SNI_Packet *pPacket, __deref_out SNI_Packet **ppLeftOver )
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("ppLeftOver: %p{SNI_Packet**}\n"), 
							  GetBidId(),
							  pPacket, 
							  ppLeftOver);

	SecBuffer       Buffers[4];
	SECURITY_STATUS scRet;
	SecBufferDesc   Message;

	Assert( *ppLeftOver == NULL );

	if( !pPacket )
	{
		scRet = SEC_E_INCOMPLETE_MESSAGE;

		goto ExitFunc;
	}

	Assert( 0 == pPacket->m_OffSet );

	// Initialize Security buffers
	Message.ulVersion = SECBUFFER_VERSION;
	Message.cBuffers = 4;
	Message.pBuffers = Buffers;

	SNIPacketGetData( pPacket, (BYTE **)&Buffers[0].pvBuffer, &Buffers[0].cbBuffer);

	Buffers[0].BufferType = SECBUFFER_DATA;

	Buffers[1].BufferType = SECBUFFER_EMPTY;
	Buffers[2].BufferType = SECBUFFER_EMPTY;
	Buffers[3].BufferType = SECBUFFER_EMPTY;

	THREAD_PREEMPTIVE_ON_START (PWAIT_PREEMPTIVE_OS_DECRYPTMESSAGE);
#ifdef SNIX
	if( g_fisWin9x )
	{
		scRet = Dfn(&m_hContext, &Message, 0, NULL);
	}
	else	
#endif
	{
		scRet = s_pfTable->DecryptMessage(&m_hContext, &Message, 0, NULL);
	}

	THREAD_PREEMPTIVE_ON_END;
	
	if( SEC_E_OK == scRet )
	{

		if( Buffers[1].cbBuffer == 0)
		{
			SNI_ASSERT_ON_INVALID_PACKET;
			scRet = SEC_E_INCOMPLETE_MESSAGE;
			goto ExitFunc;

		}
		else if( Buffers[1].BufferType != SECBUFFER_DATA )
		{
			SNI_ASSERT_ON_INVALID_PACKET;
			scRet = ERROR_INVALID_DATA;
			goto ExitFunc;
		}
		else
		{

			//
			// length of decrypted data shouldn't exceed user buffer size
			// this can happen if a malicious user encrypts a bad packet with tls
			// Notes:
			// 1. This assumes that there are no providers with non-zero
			//    headers or trailers below SSL.
			// 2. This effectively compares Buffers[1].cbBuffer vs. 
			//    ConsBufferSize + ProvBufferSize - 2*(m_cbHeaderLength + m_cbTrailerLength), 
			//    which is the maximum payload size of the SSL provider (assuming condition 1).
			//    See Ssl::GetSSLProvInfo for additional details.
			if( (2 * (m_cbHeaderLength + m_cbTrailerLength) + Buffers[1].cbBuffer) >
				( m_pConn->m_ConnInfo.ConsBufferSize + m_pConn->m_ConnInfo.ProvBufferSize ) )
			{
				scRet = ERROR_INVALID_DATA;
				
				SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, scRet );

				goto ExitFunc;
			}
			
			SNIPacketIncrementOffset( pPacket, m_cbHeaderLength);

			SNIPacketSetBufferSize( pPacket, Buffers[1].cbBuffer);

			if( Buffers[3].BufferType == SECBUFFER_EXTRA )
			{
				*ppLeftOver = SNIPacketAllocate( m_pConn, SNI_Packet_Read );
				
				if( NULL == *ppLeftOver )
				{
					scRet = ERROR_OUTOFMEMORY;

					SNI_SET_LAST_ERROR( SSL_PROV, SNIE_4, scRet );

					goto ExitFunc;
				}
				
				SNIPacketSetData( *ppLeftOver, (BYTE *)Buffers[3].pvBuffer, Buffers[3].cbBuffer);
			}
		}
	}
	else if ( SEC_E_INCOMPLETE_MESSAGE != scRet )
	{
		//
		// The SSPI call may fail, e.g. with error code 
		// SEC_E_INTERNAL_ERROR "The Local Security Authority cannot be 
		// contacted"
		//
		
		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, scRet );
	}

ExitFunc:

	Assert( ERROR_SUCCESS == scRet || !*ppLeftOver );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), scRet);

	return scRet;	
}

//Encrypts the data in pPacket inplace and fixes offset and size of pPacket
DWORD Ssl::Encrypt( __inout SNI_Packet *pPacket )
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}\n"), 
							  GetBidId(),
							  pPacket);

	SecBuffer       		Buffers[4];
	SECURITY_STATUS 	scRet;
	SecBufferDesc   		Message;
	
	BYTE    *	pBuf;
	DWORD 	cBuf;

	SNIPacketGetData( pPacket, &pBuf, &cBuf);

	if( cBuf > m_cbMaximumMessage )
	{
		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_32, ERROR_NOT_SUPPORTED );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_NOT_SUPPORTED);

		return ERROR_NOT_SUPPORTED;
	}
	
	// Initialize Security buffers
	Message.ulVersion = SECBUFFER_VERSION;
	Message.cBuffers = 4;
	Message.pBuffers = Buffers;

	Buffers[0].pvBuffer = pBuf-m_cbHeaderLength;
	Buffers[0].cbBuffer = m_cbHeaderLength;
	Buffers[0].BufferType = SECBUFFER_STREAM_HEADER;

	Buffers[1].pvBuffer = pBuf;
	Buffers[1].cbBuffer = cBuf;
	Buffers[1].BufferType = SECBUFFER_DATA;

	Buffers[2].pvBuffer = pBuf + cBuf;
	Buffers[2].cbBuffer = m_cbTrailerLength;
	Buffers[2].BufferType = SECBUFFER_STREAM_TRAILER;

	Buffers[3].BufferType = SECBUFFER_EMPTY;

	THREAD_PREEMPTIVE_ON_START (PWAIT_PREEMPTIVE_OS_ENCRYPTMESSAGE);
#ifdef SNIX
	if( g_fisWin9x )
	{
		scRet = Efn(&m_hContext, 0, &Message, 0);
	}
	else	
#endif
	{
		scRet = s_pfTable->EncryptMessage(&m_hContext, 0, &Message, 0);
	}

	THREAD_PREEMPTIVE_ON_END;
	
	if( scRet == SEC_E_OK )
	{
		SNIPacketDecrementOffset( pPacket, m_cbHeaderLength);

		Assert( pPacket->m_cbTrailer == 0 );
		pPacket->m_cbTrailer = Buffers[2].cbBuffer;

		Assert( pPacket->m_cbTrailer <= m_cbTrailerLength );
		SNIPacketSetBufferSize( pPacket, cBuf+(m_cbHeaderLength+pPacket->m_cbTrailer));
	}
	else
	{
		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, scRet );
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), scRet);

	return scRet;
}

DWORD Ssl::FindAndLoadCertificate( __in HCERTSTORE  hMyCertStore, 
										 __in BOOL fHash, 
										 __in void * pvFindPara,
										 __deref_out PCCERT_CONTEXT * ppCertContext
										 )
{
	BidxScopeAutoSNI4( SNIAPI_TAG _T("hMyCertStore: %p{HCERTSTORE}, ")
							  _T("fHash: %d{BOOL}, ")
							  _T("pvFindPara: %p, ")
							  _T("ppCertContext: %p{PCCERT_CONTEXT*}\n"), 
					hMyCertStore, fHash, pvFindPara, ppCertContext);

	Assert( !*ppCertContext );
	
	DWORD dwRet = ERROR_FAIL;
	
	PCCERT_CONTEXT pCertContext = NULL;
	PCCERT_CONTEXT pprevCertContext = NULL;

	// Loop till we find a good one or there are none left
	while(1)
	{
		pCertContext = CertFindCertificateInStore(
								hMyCertStore, 
								X509_ASN_ENCODING, 
								0,
								fHash ? CERT_FIND_SHA1_HASH : CERT_FIND_SUBJECT_STR_A,
								pvFindPara,
								pprevCertContext);

		// If this returns NULL, there are no certs. which match
		// Return an error
		if( pCertContext == NULL)
		{
			dwRet = GetLastError();

			SNI_SET_LAST_ERROR(SSL_PROV, SNIE_SYSTEM, dwRet)

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

			return dwRet;
		}
		
		// Check if cert. is valid for Server Auth use when Hash is not specified. 

		if( fHash || ERROR_SUCCESS == IsServerAuthCert(pCertContext) )
		{
			dwRet = AcquireCredentialsForServer(pCertContext);

			if( ERROR_SUCCESS == dwRet )
			{
				*ppCertContext = pCertContext;
				
				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

				return dwRet;
			}
		}

		pprevCertContext = pCertContext;
	}
}

PCCERT_CONTEXT Ssl::GetServerCertificate()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T( "\n") );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("CertContext: %p\n"), s_hCertContext);

	return s_hCertContext;
}

//----------------------------------------------------------------------------
// NAME: Ssl::Handshake
//  
// PURPOSE:
//		Does main handshake processing for ssl/tls by passing input/output buffers to sspi calls
//
// PARAMETERS:
//	
// RETURNS:
//		ERROR_SUCCESS if everything is fine.
//		Error code on failure.
//  
// NOTES:
//		This function doesn't take any parameters, instead it uses member variables inside the Ssl object
//
//----------------------------------------------------------------------------

DWORD Ssl::Handshake( void )
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	DWORD dwRet;
	SecBufferDesc   InBuffer;
	SecBuffer       InBuffers[2];
	SecBufferDesc   OutBuffer;
	SecBuffer       OutBuffers[1];
	DWORD dwSSPIOutFlags = 0;

	Assert( m_State == SSL_INIT || m_State == SSL_MORE );

	THREAD_PREEMPTIVE_ON_START (PWAIT_PREEMPTIVE_OS_AUTHENTICATIONOPS);

Retry:

	Assert( !m_cWriteBuffer);

	//
	// prepare buffers
	// input buffers are initialized even though we won't use it in first call to ISC
	// this is required because we check the type of input buffer later
	//

	InBuffers[0].pvBuffer   = m_pReadBuffer;
	InBuffers[0].cbBuffer   = m_cReadBuffer;
	InBuffers[0].BufferType = SECBUFFER_TOKEN;

	InBuffers[1].pvBuffer   = NULL;
	InBuffers[1].cbBuffer   = 0;
	InBuffers[1].BufferType = SECBUFFER_EMPTY;

	InBuffer.cBuffers       = 2;
	InBuffer.pBuffers       = InBuffers;
	InBuffer.ulVersion      = SECBUFFER_VERSION;

	OutBuffers[0].pvBuffer  = m_pWriteBuffer;
	OutBuffers[0].BufferType= SECBUFFER_TOKEN;
	OutBuffers[0].cbBuffer  = s_cbMaxToken;

	OutBuffer.cBuffers      = 1;
	OutBuffer.pBuffers      = OutBuffers;
	OutBuffer.ulVersion     = SECBUFFER_VERSION;	
											
	if( m_pConn->m_fClient )
	{		
		// On the client, we can have SSPI validate the SSL certificate for us.
		// This is the default behavior of the schannel SSPI. But in pre-win2K 
		// systems, we will have to manually validate ourselves because the 
		// auto validation is not supported.
		// We also don't want auto-validate if the m_fValidate flag is FALSE
		//
		BOOL fAutoValidate = (m_fValidate && gfIsWin2KPlatform);

		dwRet = s_pfTable->InitializeSecurityContext(
									m_fUseExistingCred ? (m_fValidate?&s_hClientCredValidate:&s_hClientCred): &m_hCred,
									m_State == SSL_INIT ? NULL : &m_hContext,
									m_pConn->m_pwszOriginalServer,
									fAutoValidate ? SSL_CONTEXT_REQ : SSL_CONTEXT_REQ_MANUAL,
									0,
									SECURITY_NATIVE_DREP,
									m_State == SSL_INIT ? NULL : &InBuffer,
									0,
									m_State == SSL_INIT ? &m_hContext : NULL,
									&OutBuffer,
									&dwSSPIOutFlags,
									NULL);

	}
	else	
	{	
		dwRet = s_pfTable->AcceptSecurityContext(
									&s_hServerCred,
									SSL_INIT == m_State ? NULL : &m_hContext,
									&InBuffer,
									SSL_CONTEXT_REQ,
									SECURITY_NATIVE_DREP,
									&m_hContext,
									&OutBuffer,
									&dwSSPIOutFlags,
									NULL);
	}

	//
	// change state from SSL_INIT to SSL_MORE regardless of return value
	//
	
	if( SSL_INIT == m_State)
	{
		m_State = SSL_MORE;
	}

	// Handshake complete
	if( (dwRet == SEC_E_OK) )
	{
		//
		// there shouldn't be any extra data, because we wrap handshake packets with tds
		//
		
		if(InBuffers[1].BufferType == SECBUFFER_EXTRA)
		{
			SNI_ASSERT_ON_INVALID_PACKET;
			dwRet = SEC_E_INVALID_TOKEN;

			SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, dwRet );

			goto ErrorExit;
		}

		m_cReadBuffer = 0;
		
		m_iWriteOffset = 0;
		
		m_cWriteBuffer = OutBuffers[0].cbBuffer;

		if( m_cWriteBuffer )
		{
			if( OutBuffers[0].BufferType != SECBUFFER_TOKEN )
			{
				SNI_ASSERT_ON_INVALID_PACKET;
				dwRet = SEC_E_INVALID_TOKEN;
				
				SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, dwRet );

				goto ErrorExit;
			}

			m_State = SSL_LAST;
		}
		else
		{
			FreeReadWriteBuffers();

			m_State = SSL_DONE;
		}

		if (! m_fIgnoreChannelBindings)
		{
			dwRet = SetChannelBindings();
			if (SEC_E_OK != dwRet)
			{
				// Already traced and set error in SetChannelBindings()
				goto ErrorExit;
			}
		}

		//for server m_fValidate should always be FALSE
		Assert( !m_fValidate || m_pConn->m_fClient );

		// if we need to validate the cert and Schannel isn't capable of it (i.e., we are on < Win2k),
		// we must validate manually.
		if( m_fValidate && !gfIsWin2KPlatform &&
			ERROR_SUCCESS != (dwRet =Validate( m_pConn->m_pwszServer )))
		{
			goto ErrorExit;
		}


		SecPkgContext_StreamSizes StreamSizes;
		
		dwRet = s_pfTable->QueryContextAttributes(&m_hContext, SECPKG_ATTR_STREAM_SIZES, &StreamSizes);

		if( ERROR_SUCCESS != dwRet )
		{
			
			SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, dwRet );

			goto ErrorExit;
		}
		
		m_cbHeaderLength = StreamSizes.cbHeader;		
		m_cbTrailerLength = StreamSizes.cbTrailer;
		m_cbMaximumMessage = StreamSizes.cbMaximumMessage;
		
		dwRet = AdjustProtocolFields();

		if (ERROR_SUCCESS != dwRet)
		{
			m_State = SSL_ERROR;
			
			// AdjustProtocolFields sets state and sni last error if needed.
			goto ErrorExit;
		}
	}
	else if( dwRet == SEC_I_CONTINUE_NEEDED )
	{
		if( InBuffers[1].BufferType == SECBUFFER_EXTRA )
		{
			Assert( OutBuffers[0].cbBuffer==0 );
			memmove( m_pReadBuffer, m_pReadBuffer
									+InBuffers[0].cbBuffer
									-InBuffers[1].cbBuffer,InBuffers[1].cbBuffer);
			m_cReadBuffer = InBuffers[1].cbBuffer;

			goto Retry;
		}
		else
		{
			m_cReadBuffer = 0;
			m_iWriteOffset = 0;
			m_cWriteBuffer = OutBuffers[0].cbBuffer;
		}
	}
	else 
	{
		Assert( dwRet != SEC_E_INCOMPLETE_MESSAGE );

		// Previously asserted here that ISC_RET_EXTENDED_ERROR was not flagged in dwSSPIOutFlags.
		// Assert removed after it failed and was repro'd.  See tfs SQL Server 88049 for details.

		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, dwRet );

		SNITraceErrorToConsumer(m_pConn, dwRet, SNIE_10);

		goto ErrorExit;
	}

	THREAD_PREEMPTIVE_ON_END;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

	return ERROR_SUCCESS;

ErrorExit:

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;

}

//----------------------------------------------------------------------------
// NAME: Ssl::HandshakeReadDone
//  
// PURPOSE:
//		Continues handshake after a packet is recieved.
//
// PARAMETERS:
//		pPacket: input packet recieved
//		dwError: error code related to this packet( check notes below)
//	
// RETURNS:
//		ERROR_SUCCESS if everything is fine.
//		ERROR_IO_PENDING when an async read doesn't complete immediately
//		Error code on failure.
//  
// NOTES:
//		when dwError is different than ERROR_SUCCESS all we do in this function is the cleanup
//		related to pending packets
//
//----------------------------------------------------------------------------

DWORD Ssl::HandshakeReadDone( __inout_opt SNI_Packet *pPacket, __in DWORD dwError )
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("dwError: %d{WINERR}\n"),  
							  GetBidId(),
							  pPacket, 
							  dwError);

	DWORD dwRet = dwError;
	
	Assert( m_State != SSL_DONE);
	
	while( dwRet == ERROR_SUCCESS && m_State != SSL_DONE )
	{
		dwRet = HandshakeReadToken( pPacket );
		pPacket = 0;	//after first one it is set to null

		if( dwRet != ERROR_SUCCESS )
			break;

		dwRet = Handshake();

		if( dwRet != ERROR_SUCCESS )
			break;

		if( m_State == SSL_DONE )
			break;
		
		dwRet = HandshakeWriteToken( 0 );
	}

	if( dwRet == ERROR_SUCCESS)
	{
		Assert( m_State == SSL_DONE );
		ProcessPendingIO();
	}
	else if( dwRet != ERROR_IO_PENDING)
	{
		m_State = SSL_ERROR;
		if ( m_dwSslNativeError == ERROR_SUCCESS )
		{
			m_dwSslNativeError = dwRet;
			m_dwSslSNIError = SNIE_31;
		}
		CallbackError();
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;
}

////////////////////////////////////////////////////////////////////////////////
// HandshakeReadToken
//	This function read entire SSL handshake token. Multiple packets can be read 
// from network if token size is bigger than default 4k packet (at handshaked stage).
// Token size larger than 4k can happen, e.g. when certificate is large. The token is buffered 
// in m_pReadBuffer. The fLast = pBuf[1] is used to identify whether a packet is the last packet.
// See its conterpart HandshakeWriteToken.
//
// On client side, it will loop until a full token is buffered in m_pReadBuffer. 
//
// On server side, multile HandshakeReadToken calls are expected because the async IO. First
// HandshakeReadToken(0) is called on TDS worker, and subsequent calls are made on IOCP.
// At first, this function will post a new pPacket if pPacket is NULL or parse it otherwise. If
// a full token is buffered m_pReadBuffer, it return succeed. Otherwise, it will keep post either
// previous pPacket if the prevous pPacket is not read in full or posted a new pPacket otherwise.
// if  new pPacket is posted, the previous pPacket must be released. Once a full token is buffered,
// this function will either release the pPacket, or assign the pPacket to m_pLeftover if the current 
// pPacket contains login tds paskcet. Multiple packets posted by this function will eventually be 
// consumed by this function. On return
// (1) pending: pPacket is posted, next readone will deliver the packet to this function that will 
// subsequently consume this packet. A REF_InternalRead is added to make sure that the 
// connection won't go away if there is pending packet posted.
// (2) succeed: a full token is read. Either pPacket is saved for m_pLeftover packet if the packet contain
//	first login packet, or the pPacket is released. 
// (3) error: the packet is release because it is posted by previous HandshakeReadToken.
// (4) if the readdone on IOCP thread decided not to call this function, readdone function should
// release the pPacket.
//
DWORD Ssl::HandshakeReadToken(__inout_opt SNI_Packet *pPacket)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}\n"),  
							  GetBidId(),
							  pPacket);

	DWORD dwRet = ERROR_SUCCESS;

	//
	// there shouldn't be any data in raw packet in this state
	//
	
	if( m_pLeftOver )
	{
		//this assertin is used to catch coding errors.		
		Assert( 0 && " m_LeftOver is not NULL\n" );
		BidTrace0( ERROR_TAG _T("m_LeftOver is not NULL\n") );

		dwRet = SEC_E_INVALID_TOKEN;
		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, dwRet );

		goto ExitFunc;
	}

	//this while loop reads one whole ssl token
	while( 1 )
	{
		DWORD 		tdsLen;

		BYTE 	    * pBuf;
		DWORD 		cBuf;

		//this while loops reads one whole tds packet
		while(1)
		{
			cBuf = 0;

			if( pPacket )
			{
				SNIPacketGetData( pPacket, &pBuf, &cBuf);
			}
			
			//
			// check to see whether we have enough data
			//
			
			if( cBuf > 8 )
			{
				tdsLen = (DWORD)(pBuf[2]*256 + pBuf[3]);

				// Tds packet size cannot exceed 4K in handshake
				if( tdsLen <= 8 || tdsLen > 4096 )
				{
					SNI_ASSERT_ON_INVALID_PACKET
					dwRet = SEC_E_INVALID_TOKEN;

					SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, dwRet );

					goto ExitFunc;
				}
			
				// Make sure that we get an entire TDS packet
				if( cBuf >= tdsLen )
				{
					break;
				}
			}		

			if( m_pConn->m_fSync )
			{
				if( pPacket )
					dwRet = m_pNext->PartialReadSync( pPacket, 4096-cBuf, INFINITE);
				else
					dwRet = m_pNext->ReadSync( &pPacket, INFINITE);
			}
			else
			{
				if( pPacket )
					dwRet = m_pNext->PartialReadAsync( pPacket, 4096-cBuf, 0 );
				else
					dwRet = m_pNext->ReadAsync( &pPacket, 0 );
				
				if( ERROR_IO_PENDING == dwRet )
				{
					pPacket = 0;	//we don't want packet to be freed

					m_pConn->AddRef( REF_InternalRead );
				}
			}

			if( ERROR_SUCCESS != dwRet )
			{
				goto ExitFunc;
			}
		}		

		// Check to make sure we don't overflow
		if( m_cReadBuffer+tdsLen-8 > s_cbMaxToken )
		{
			dwRet = SEC_E_INVALID_TOKEN;

			SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, dwRet );

			goto ExitFunc;
		}
		
		memcpy(m_pReadBuffer+m_cReadBuffer, pBuf+8, tdsLen-8);
		m_cReadBuffer += tdsLen-8;

		BOOL fLast = pBuf[1];
		
		cBuf -= tdsLen;

		if( cBuf )
		{
			memmove( pBuf, pBuf+tdsLen, cBuf );

			SNIPacketSetBufferSize( pPacket, cBuf );
		}		
		else
		{
			SNIPacketRelease( pPacket );

			pPacket = NULL;
		}

		// if this is last tds packet we are finished reading token
		if( fLast )
		{
			//
			// if there is some extra data beyond tds packet,  it should be saved for decryption later
			// this can only happen if the last sspi packet comes with login packet from client
			// 

			if( pPacket && m_pConn->m_fClient )
			{
				dwRet = SEC_E_INVALID_TOKEN;

				SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, dwRet );

				goto ExitFunc;
			}

			m_pLeftOver = pPacket;
			pPacket = 0;
			
			break;
		}

	}

	Assert( dwRet==ERROR_SUCCESS );
	
ExitFunc:

	if( pPacket )
	{
		SNIPacketRelease( pPacket );
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;
}

DWORD Ssl::HandshakeWriteDone( __inout_opt SNI_Packet *pPacket, __in DWORD dwError )
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("dwError: %d{WINERR}\n"),  
							  GetBidId(),
							  pPacket, 
							  dwError);

	DWORD dwRet = dwError;
	
	Assert( m_State != SSL_DONE);
	
	while( dwRet==ERROR_SUCCESS && m_State != SSL_DONE )
	{
		dwRet = HandshakeWriteToken( pPacket );
		
		pPacket = 0;	//after first one it is set to null

		if( dwRet != ERROR_SUCCESS )
			break;

		if( m_State == SSL_DONE )
			break;

		dwRet = HandshakeReadToken( 0 );

		if( dwRet != ERROR_SUCCESS )
			break;

		dwRet = Handshake();

	}

	if( dwRet == ERROR_SUCCESS)
	{
		Assert( m_State == SSL_DONE );
		ProcessPendingIO();
	}
	else if( dwRet != ERROR_IO_PENDING)
	{
		m_State = SSL_ERROR;
		if ( m_dwSslNativeError == ERROR_SUCCESS )
		{
			m_dwSslNativeError = dwRet;
			m_dwSslSNIError = SNIE_31;
		}
		CallbackError();
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;
}

////////////////////////////////////////////////////////////////////////////////
// HandshakeWriteToken
//	This function write entire SSL handshake token. Multiple packets can be written 
// to network if token size is bigger than default 4k packet (at handshaked stage).
// Token size larger than 4k can happen, e.g. when certificate is large. The packet is 
// copied from token buffer m_pWriteBuffer. The pBuf[1] is used to indicate whether
// a packet is the last packet for current token.
// See its conterpart HandshakeReadToken.
//
// On client side, it will loop until a full token from m_pReadBufferis writtnen.
//
// On server side, multile HandshakeWriteToken calls are expected because the async IO. 
// At first, this function will allocate a new write pPacket if pPacket is NULL or reuse previous 
// pPacket. If a full token is writtne, it return succeed. Otherwise, it will keep writing. Once 
// a full token is written, this function will release the pPacket. The packets allocated by the 
// first call with NULL pPacket, will eventually be released by this function when a full token 
// is written. On return
// (1) pending: pPacket is posted, next writeone will deliver the packet to this function that will 
// subsequently either post next write or release. A REF_InternalWrite is added to sni_conn object
// to make sure that the connection won't go away if there is pending packet posted.
// (2) succeed: a full token is written. The  packet is released. 
// (3) error: the packet, if allocated is release. 
// (4) if the write done on IOCP thread decided not to call this function, writedone function should
// release the pPacket.
//
DWORD Ssl::HandshakeWriteToken( __inout_opt SNI_Packet *pPacket )
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}\n"),  
							  GetBidId(),
							  pPacket);

	DWORD 	dwRet = ERROR_SUCCESS;

	if( pPacket )
	{
		SNIPacketReset( m_pConn, SNI_Packet_Write, pPacket, SNI_Consumer_SNI);
	}
	else
	{
		Assert(m_cWriteBuffer);

		pPacket = SNIPacketAllocate(m_pConn, SNI_Packet_Write);
		
		if( pPacket == NULL )
		{
			dwRet = ERROR_OUTOFMEMORY;

			SNI_SET_LAST_ERROR( SSL_PROV, SNIE_4, dwRet );

			goto ExitFunc;
		}

		//this sets m_OffSet to 0
		SNIPacketReset( m_pConn, SNI_Packet_Write, pPacket, SNI_Consumer_SNI);
	}

	Assert( SNIPacketGetBufActualSize(pPacket) >= 4088 );

	while( m_cWriteBuffer )
	{
		DWORD cPacket;

		BYTE pbTds[8];
		
	       // Type set to 18 to indicate its a post 7.0 client
	       pbTds[0] = 0x12;

		if( m_cWriteBuffer <= 4088 )
		{
			cPacket = m_cWriteBuffer;

	              // This is the last packet
	              pbTds[1] = 1;
		}
		else
		{
			cPacket = 4088;

	              // This is NOT the last packet
	              pbTds[1] = 0;
		}

		// Length of packet 
	       pbTds[2] = (int)((cPacket+8)/256);
	       pbTds[3] = (int)((cPacket+8)%256);

		memset(pbTds+4, 0, 4);
			
		SNIPacketSetData( pPacket, pbTds, 8);

		SNIPacketAppendData( pPacket, m_pWriteBuffer+m_iWriteOffset, cPacket);

		m_iWriteOffset += cPacket;
		m_cWriteBuffer -= cPacket;

		if( m_pConn->m_fSync )
		{
			dwRet = m_pNext->WriteSync( pPacket, 0);
		}
		else
		{
			dwRet = m_pNext->WriteAsync( pPacket, 0);

			if( ERROR_IO_PENDING == dwRet )
			{
				pPacket = NULL;	//we don't want packet to be freed

				m_pConn->AddRef( REF_InternalWrite );
				
				goto ExitFunc;
			}
		}

		if( ERROR_SUCCESS != dwRet )
		{
			goto ExitFunc;
		}

		SNIPacketReset( m_pConn, SNI_Packet_Write, pPacket, SNI_Consumer_SNI);
	}

	if( !m_cWriteBuffer )
	{
		m_iWriteOffset = 0;
	}
	
	Assert( dwRet == ERROR_SUCCESS );
	if( m_State==SSL_LAST)
	{
		FreeReadWriteBuffers();
		
		dwRet = AdjustProtocolFields();
		
		if (ERROR_SUCCESS != dwRet)
        {
            m_State = SSL_ERROR;

            goto ExitFunc;
        }
		
		m_State = SSL_DONE;
	}
	
ExitFunc:
	
	if( pPacket )
	{
		SNIPacketRelease( pPacket );
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;
}

DWORD Ssl::Initialize( PSNI_PROVIDER_INFO pInfo )
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("pInfo: %p{PSNI_PROVIDER_INFO}\n"), pInfo );
	
	pInfo->ProvNum = SSL_PROV;
	pInfo->fBaseProv = FALSE;
	pInfo->fInitialized = TRUE; 

	// Ssl Provider uses dynamic buffer adjustment.
	// Offset is set after handshake is complete.
	pInfo->Offset = 0;
	pInfo->Size = 0;
	
	Assert( SEC_E_OK == ERROR_SUCCESS );

	SecInvalidateHandle(&s_hClientCred);
	SecInvalidateHandle(&s_hClientCredValidate);

	DWORD dwRet;

	dwRet = LoadSecurityLibrary();
	if( dwRet != ERROR_SUCCESS )
	{
		goto ErrorExit;
	}

	dwRet = AcquireCredentialsForClient( &s_hClientCred );
	
	if( SEC_E_OK != dwRet )
		goto ErrorExit;


	dwRet = AcquireCredentialsForClient( &s_hClientCredValidate);
	
	if( SEC_E_OK != dwRet )
		goto ErrorExit;


	PSecPkgInfo     pkgInfo;

	dwRet =s_pfTable->QuerySecurityPackageInfo( UNISP_NAME, &pkgInfo );

	if( SEC_E_OK != dwRet )
	{
		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, dwRet );

		goto ErrorExit;
	}

	//On NT+, s_cbMaxToken usually is 16384;
	//On W98 2ndE, s_cbMaxToken is 32768.

	s_cbMaxToken = pkgInfo->cbMaxToken;

#ifndef SNI_BASED_CLIENT

	// Save s_dwcPagesforMaxToken to allocate/free memory from/to SOS
	// in AllocateReadWriteBuffers and  FreeReadWriteBuffers.
	// Fail to do so should fail server from using SSL.
	
	CPL_ASSERT ( x_cbMemPageSize == 8192 );

	HRESULT hr = UIntSub(s_cbMaxToken, 1, &s_cPagesforMaxToken);
	if ( FAILED (hr) )
	{
		s_cPagesforMaxToken = 0;

		dwRet = ERROR_ARITHMETIC_OVERFLOW;

		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, dwRet );

		goto ErrorExit;
	}
		
	s_cPagesforMaxToken = s_cPagesforMaxToken/ x_cbMemPageSize;
	
	hr = UIntAdd(s_cPagesforMaxToken, 1, &s_cPagesforMaxToken);
	if ( FAILED (hr) )
	{
		s_cPagesforMaxToken = 0;

		dwRet = ERROR_ARITHMETIC_OVERFLOW;

		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, dwRet );

		goto ErrorExit;
	}
	
#endif


	s_pfTable->FreeContextBuffer( pkgInfo );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

	return ERROR_SUCCESS;

ErrorExit:
	
	s_cbMaxToken = 0;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;
}

DWORD Ssl::InitializeListener( __in HANDLE   hSNIListener, 
							  __in SslListenInfo *pInfo, 
							  __out HANDLE * pListenHandle )
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("hSNIListener: %p{HANDLE}, ")
							  _T("pInfo: %p{SslInfo*}, ")
							  _T("pListenHandle: %p{HANDLE*}\n"), 
					hSNIListener, pInfo, pListenHandle);

	LPWSTR wszFQDN = NULL;
	DWORD cszFQDN = 0;
	
	//For prefix bug 302960:
	//Even though we have already commented out the code that generate the warning,
	//Initialize it to ERROR_INVALID_STATE would indicate there are improper execution
	//of this function.
	DWORD dwRet = ERROR_INVALID_STATE;
	
	*pListenHandle = 0;

	//if Ssl::Initialize() failed before, return fail
	if( !s_cbMaxToken )
	{
		scierrlog(26011);

		dwRet = ERROR_INVALID_STATE;

		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, ERROR_INVALID_STATE );

		goto ErrorExit;
	}
	
	if( TRUE == s_fSslServerReady )
	{
		scierrlog(26012);

		dwRet = ERROR_INVALID_STATE;

		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, ERROR_INVALID_STATE );

		goto ErrorExit;
	}

	Assert( !s_hCertContext && !s_hCertStore );

	SecInvalidateHandle( &s_hServerCred );

	CRYPT_HASH_BLOB pShaBlob;
	void * pvFindPara;
	WCHAR wszShaHashText[SHA1HASHLENGTH*2+1];
	wszShaHashText[0]=L'\0'; //null teminating string.

// SNIX needs to work on Win9x - which does not support GetComputerNameEx
// There we just use the NetBIOS name - this shouldnt cause too much concern, 
// since this is a server side call
#ifdef SNIX	
		wszFQDN = (LPWSTR) gwszComputerName;
#else	
		// Because of a bug in Win2k, the call below fails to return the correct size of the 
		// FQDN string. cszFQDN is 0 on Win2k.
		/// To get around that, I'm allocating a large value on the stack and commenting out the 
		// failing code (hopefully, when Win2k comes out with a fix, we can uncomment it)
		/*
		if( GetComputerNameEx( ComputerNameDnsFullyQualified, wszFQDN, &cszFQDN ) || 
			(ERROR_MORE_DATA != (dwRet = GetLastError())) )
		{
			SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, dwRet );
			goto ErrorExit;
		}

		wszFQDN = (LPSTR) NewNoX(gpmo) BYTE [cszFQDN] ;

		if( !wszFQDN )
		{
			dwRet = ERROR_OUTOFMEMORY;
			SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, dwRet );
			goto ErrorExit;
		}
		*/

		WCHAR wszFQDNBuf[1024];
		wszFQDNBuf[1023] = L'\0';

		wszFQDN = wszFQDNBuf;
		cszFQDN = 1023;
		
		if( !GetComputerNameEx( ComputerNameDnsFullyQualified, wszFQDN, &cszFQDN ) )
		{
			dwRet = GetLastError();
			SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, dwRet );
			scierrlog(26057, dwRet);
			goto ErrorExit;
		}
#endif

	// If a Hash is provided, look up a cert. with that Hash
	if( pInfo->fHash )
	{
		pShaBlob.cbData = sizeof(pInfo->szHash);
		pShaBlob.pbData = pInfo->szHash;
		pvFindPara = (CRYPT_HASH_BLOB *)&pShaBlob;
	}
	// Otherwise, look for a cert. with the machine name in it
	else
	{
		pvFindPara = (LPWSTR) wszFQDN;
	}
	
	HCERTSTORE hCertStore;
	
	// Check in the "local machine" cert store for a viable cert.
	// Using the "MY" store...
	if( hCertStore = CertOpenStore(	CERT_STORE_PROV_SYSTEM,
									X509_ASN_ENCODING,
									0,
									CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_READONLY_FLAG,
									L"MY"))
	{
		dwRet = FindAndLoadCertificate( hCertStore, pInfo->fHash, pvFindPara, &s_hCertContext);

		if( ERROR_SUCCESS == dwRet )
		{
			// We found a cert to use, pass it back.
			
			Assert( s_hCertContext );

			s_hCertStore = hCertStore;
			
			s_fSslServerReady = TRUE;

			(void)GetCertShaHashInText(s_hCertContext, wszShaHashText, ARRAYSIZE(wszShaHashText));			

			scierrlog(26013, wszShaHashText );

			dwRet = ERROR_SUCCESS;

			goto ErrorExit;
		}

		CertCloseStore( hCertStore, 0);
	}
	else
	{
		// Failed to open the cert store.
		dwRet = GetLastError();
	}


	// We failed to get a cert from the "local machine" cert store.
	// We had better be returning an error code.  We could assert this, 
	// but since this is security-sensitive code, we'll handle the case.
	if (ERROR_SUCCESS == dwRet)
		dwRet = ERROR_INVALID_FUNCTION;


	// Check in the "current user" cert store for a viable cert.
	if( hCertStore = CertOpenSystemStoreA(0, "MY"))
	{
		dwRet = FindAndLoadCertificate( hCertStore, pInfo->fHash, pvFindPara, &s_hCertContext);

		if( ERROR_SUCCESS == dwRet )
		{
			// We found a cert to use, pass it back.

			Assert( s_hCertContext );

			s_hCertStore = hCertStore;

			s_fSslServerReady = TRUE;

			(void)GetCertShaHashInText(s_hCertContext, wszShaHashText, ARRAYSIZE(wszShaHashText));

			scierrlog(26013, wszShaHashText );

			dwRet = ERROR_SUCCESS;

			goto ErrorExit;
		}

		CertCloseStore( hCertStore, 0);
	}
	else
	{
		// Failed to open the cert store.
		dwRet = GetLastError();
	}


	// We failed to get a cert from the "local machine" and 
	// "current user" cert stores.
	// We had better be returning an error code.  We could assert this, 
	// but since this is security-sensitive code, we'll handle the case.
	if (ERROR_SUCCESS == dwRet)
		dwRet = ERROR_INVALID_FUNCTION;


#if !defined(SNI_BASED_CLIENT) && !defined(STANDALONE_SNI)
	PCCERT_CONTEXT hCertContext;

	//	On the server, if the user didn't explicitly specify a cert,
	//	fall back to the default SQL cert.  If the user did specify
	//  a cert to use and we failed to load it, we need to return
	//  an error and fail server startup.
	if ( !pInfo->fHash )
	{
		if( (hCertContext = GetFallBackCertContext()) )
		{			
			// Note AcquireCredentialsForServer will call SNI_SET_LAST_ERROR upon failure.
			dwRet = AcquireCredentialsForServer(hCertContext);

			if( ERROR_SUCCESS == dwRet )
			{
				// We found a cert to use, pass it back.

				// Note: The certificate context returned by GetFallBackCertContext is just a pointer
				// to a cert context that is freed when the static class CSECFallBackCert destructs.
				// So we must avoid destroying this context in Ssl::TerminateListener.
				s_hCertContext = hCertContext;

				Assert( s_hCertContext );

				s_fSslServerReady = TRUE;

				scierrlog(26018);

				goto ErrorExit;
			}
		}
		else
		{
			// If GetFallBackCertContext returns NULL, then during startup the initialization of the 
			// fallback certificate failed in SecCreateFallBackCert and this was reported to the errorlog, 
			// along with the associated system error code.  GetFallBackCertContext just returns a 
			// cached cert context or NULL, it does not call any system APIs, so we don't have a 
			// system error code stored by SetLastError to retrieve here as we do with the previous
			// CertOpenSystemStore cases.  So here we use CRYPT_E_NOT_FOUND as the most
			// suitable system error code, which means no certificate was found.
			dwRet = CRYPT_E_NOT_FOUND;
		}
	}
#endif //#ifndef SNI_BASED_CLIENT


	// We failed to find a cert to use in all of the possible cert stores.
	// We had better be returning an error code.  We could assert this, 
	// but since this is security-sensitive code, we'll handle the case.
	if (ERROR_SUCCESS == dwRet)
		dwRet = ERROR_INVALID_FUNCTION;


	//	We didn't find a cert to use.  Log error msg accordingly and fail the server.
	if( pInfo->fHash )
	{
		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_SYSTEM, dwRet );
		
		DWORD cchText = ARRAYSIZE(wszShaHashText)-1;
		(void)Bytes2Text(pInfo->szHash, sizeof(pInfo->szHash), wszShaHashText, &cchText );
		wszShaHashText[cchText]=L'\0';
		
		scierrlog( 26014, wszShaHashText);
	}
	else if( pInfo->fForceEncryption )
	{
		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_SYSTEM, dwRet );
		scierrlog( 26015 );
	}
	else
	{
		if(SNI_TRACE_FLAG(GLOBALTRACE_RETAIL, TRCFLG_RETAIL_PSS_SNI_CERT_NOTREQUIRED))
		{
			// Cert is not required, so return success.
			dwRet = ERROR_SUCCESS;
		}
		else
		{
			SNI_SET_LAST_ERROR( SSL_PROV, SNIE_SYSTEM, dwRet );
		}

		scierrlog( 26017 );

	}


ErrorExit:

#ifndef SNIX
	// Commenting out - see note above
	/*
	if( wszFQDN )
		delete [] wszFQDN;
	*/
#endif

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;
}

DWORD CryptoBase::WaitForHandshakeDone(DWORD dwMilliseconds)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#, "), 
							  GetBidId());
	DWORD dwRet;
	
	// only Async client should ever call this
	Assert( m_pConn->m_fClient && !m_pConn->m_fSync );
	
	// Don't bother waiting on the event if we know we're done
	if( m_State == SSL_DONE || m_State == SSL_ERROR || m_State == SSL_REMOVED )
	{
		BidTraceU0( SNI_BID_TRACE_ON, SNI_TAG _T("Skipping wait on SSL handshake event, since state indicates handshake is complete\n"));
		dwRet = WAIT_OBJECT_0;
	}
	else
	{
		BidTraceU0( SNI_BID_TRACE_ON, SNI_TAG _T("Waiting on SSL handshake event\n"));
		dwRet = WaitForSingleObject(m_hHandshakeDoneEvent, dwMilliseconds);
		
		if( WAIT_OBJECT_0 != dwRet )
		{
			if( WAIT_FAILED == dwRet )
			{
				dwRet = GetLastError();
			}
			SNI_SET_LAST_ERROR(m_Prot, SNIE_10, dwRet);
		}
	}
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD Ssl::InitX(__in LPVOID pUserOption)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("%u#, ")
							  _T("pUserOption: %p\n"), 
							  GetBidId(),
							  pUserOption);
	
	CAutoSNICritSec a_cs( m_CS, SNI_AUTOCS_DO_NOT_ENTER );

	a_cs.Enter();

	DWORD dwRet = ERROR_SUCCESS;

	if( m_pConn->m_fClient && ! m_pConn->m_fSync )
	{
		// for async client only, create an event to signal handshake done.
		m_hHandshakeDoneEvent = CreateEvent( NULL, // lpEventAttributes
							   TRUE,  // make it manual reset, and never reset it - we will set once and then always assume it's set after that
							   FALSE,  // bInitialState - start unset.
							   NULL ); // lpName

		if( NULL == m_hHandshakeDoneEvent )
		{
			dwRet = GetLastError();
			SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, dwRet );
			goto ExitFunc;
		}
	}

	Assert( 0 == rgProvInfo[m_pNext->m_Prot].Size );
	
	if( !( s_cbMaxToken && ( m_pConn->m_fClient || s_fSslServerReady ) ) )
	{
		dwRet = ERROR_INVALID_STATE;
		
		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, dwRet );

		goto ExitFunc;
	}

	DWORD dwOption = *(DWORD *)pUserOption;

	m_fValidate = !!(dwOption & SNI_SSL_VALIDATE_CERTIFICATE);

	m_fUseExistingCred = !!( dwOption & SNI_SSL_USE_SCHANNEL_CACHE );

	// For a perf benefit, we will ignore Channel Bindings in two situations:
	// 1) The consumer explicitly tells us not to.
	// 2) This is the server side of a connection and we have no intention 
	// to validate the bindings.
	m_fIgnoreChannelBindings = (!!( dwOption & SNI_SSL_IGNORE_CHANNEL_BINDINGS )) || 
		(! m_pConn->m_fClient && g_eExtendedProtectionLevel == ExtendedProtectionPolicy_OFF);

	Assert( m_pConn->m_fClient || ( !m_fValidate && !m_fUseExistingCred ));

	if( m_pConn->m_fClient && !m_fUseExistingCred )
	{
		dwRet = AcquireCredentialsForClient( &m_hCred );

		if( ERROR_SUCCESS != dwRet )
		{
			goto ExitFunc;
		}
	}
	
	//
	// beyond this point we don't do goto ExitFunc for errors
	// because we want the cleanup to happen correctly by calling CallbackError()
	//

	dwRet = AllocateReadWriteBuffers();

	if ( m_pConn->m_fClient )
	{
		if( dwRet == ERROR_SUCCESS && !m_pConn->m_fClient )
		{
			dwRet = HandshakeReadToken( 0 );
		}

		while( dwRet == ERROR_SUCCESS )
		{
			dwRet = Handshake();
		
			if( dwRet != ERROR_SUCCESS )
				break;

			if(  m_State == SSL_DONE )
				break;
		
			dwRet = HandshakeWriteToken( 0 );

			if( dwRet != ERROR_SUCCESS )
				break;

			if( m_State == SSL_DONE )
				break;

			dwRet = HandshakeReadToken( 0 );

		}
		
		if( dwRet == ERROR_IO_PENDING )
		{
			dwRet = ERROR_SUCCESS;
		}
		else
		{
			if( dwRet == ERROR_SUCCESS)
			{
				Assert( m_State == SSL_DONE );
				ProcessPendingIO();
			}
			else
			{
				m_State = SSL_ERROR;
				CallbackError();
			}
		}
	}

ExitFunc:
	
	a_cs.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;

}

DWORD Ssl::IsClientEncryptPossible()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), ( 0 != s_cbMaxToken ));

	return ( 0 != s_cbMaxToken ) ;
}

DWORD Ssl::GetCertShaHash(PCCERT_CONTEXT   pCertContext, BYTE* pbShaHash, DWORD* pcbSize)
{
	DWORD dwError = ERROR_SUCCESS;
	
	BidxScopeAutoSNI3( SNIAPI_TAG _T("pCertContext: %p{PCCERT_CONTEXT},")
								 _T("pbShaHash: %p{BYTE*},")
								 _T("*pcbSize: %d\n"),
								 pCertContext,
								 pbShaHash,
								 *pcbSize);
	
	Assert( NULL != pCertContext );
	Assert( NULL != pbShaHash );
	Assert( SHA1HASHLENGTH == *pcbSize );

	if( !CertGetCertificateContextProperty( pCertContext, 
									CERT_SHA1_HASH_PROP_ID,
									pbShaHash,
									pcbSize ) )
	{
		dwError = GetLastError();
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	return dwError;
}

DWORD Ssl::GetCertShaHashInText(PCCERT_CONTEXT   pCertContext, WCHAR* pwszShaHashText, DWORD cchSize)
{
	DWORD dwError = ERROR_SUCCESS;
	BYTE rgbShaHash[SHA1HASHLENGTH];
	DWORD cbShaHash = SHA1HASHLENGTH;


	BidxScopeAutoSNI3( SNIAPI_TAG _T("pCertContext: %p{PCCERT_CONTEXT},")
								 _T("pwszShaHashText: %p{WCHAR*},")
								 _T("cchSize: %d\n"),
								 pCertContext,
								 pwszShaHashText,
								 cchSize);
	
	Assert( NULL != pCertContext );
	Assert( NULL != pwszShaHashText );

	//Initialize buffer to empty string.
	//
	pwszShaHashText[0] = L'\0';

	if( SHA1HASHLENGTH*2 >= cchSize )
	{
		dwError =  ERROR_INVALID_PARAMETER;
		goto Exit;
	}

	//Get Sha1 Hash in a Byte Array
	//
	dwError = GetCertShaHash(pCertContext, rgbShaHash, &cbShaHash);
	
	if( ERROR_SUCCESS != dwError )
	{
		goto Exit;
	}	

	Assert(SHA1HASHLENGTH == cbShaHash);

	//Convert Byte Array into ascii characters
	//

	DWORD cchText = cchSize-1;
	dwError = Bytes2Text(rgbShaHash, cbShaHash, pwszShaHashText, &cchText);

	if( ERROR_SUCCESS != dwError )
	{	
		goto Exit;
	}

	Assert(SHA1HASHLENGTH*2 == cchText);
	
	pwszShaHashText[cchText] = L'\0';

Exit:

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}

DWORD Ssl::IsServerAuthCert(PCCERT_CONTEXT   pCertContext)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("pCertContext: %p{PCCERT_CONTEXT}\n"), pCertContext );

	DWORD dwError = ERROR_SUCCESS;

	//Verfiy that the cert has good time stamp
	if(0 != CertVerifyTimeValidity(NULL, pCertContext->pCertInfo))
	{

		dwError = TRUST_E_TIME_STAMP;	// choose a closest value for this case. 

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);

		return dwError;

	}
	// Verify that the key usage is exchange
	{
		PCRYPT_KEY_PROV_INFO pvData = 0;
		DWORD cbData = 0;
		if( !CertGetCertificateContextProperty( pCertContext, 
													  CERT_KEY_PROV_INFO_PROP_ID,
													  NULL,
													  &cbData ) )
		{

			dwError = GetLastError();

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);

			return dwError;

		}

		pvData = (PCRYPT_KEY_PROV_INFO) NewNoX(gpmo) BYTE [cbData] ;

		if( !pvData )
		{

			dwError = ERROR_OUTOFMEMORY;

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);

			return dwError;

		}

		if( !CertGetCertificateContextProperty( pCertContext, 
													  CERT_KEY_PROV_INFO_PROP_ID,
													  pvData,
													  &cbData ) )
		{

			dwError = GetLastError();

			delete [] pvData;
	
			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);

			return dwError;

		}
		
		if( (pvData->dwKeySpec & AT_KEYEXCHANGE) != AT_KEYEXCHANGE )
		{

			delete [] pvData;

			dwError = CRYPT_E_NOT_FOUND;	//The certificate does not have the specified property.

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);

			return dwError;
			
		}

		delete [] pvData;
	}

	// Verify that the cert has valid key-pair. See SDT 422758.
	HCRYPTPROV hProv;
	DWORD dwKeySpec;
	BOOL fFreeProvider;

	if( !CryptAcquireCertificatePrivateKey (
							pCertContext, 
							CRYPT_ACQUIRE_CACHE_FLAG|CRYPT_ACQUIRE_COMPARE_KEY_FLAG|CRYPT_ACQUIRE_SILENT_FLAG,
							NULL,
							&hProv,
							&dwKeySpec,
							&fFreeProvider))
	{  
		dwError = GetLastError();

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);	

		return dwError;
	}

	Assert (!fFreeProvider);

	// Verify that the cert is meant for server auth
	PCERT_ENHKEY_USAGE pUsage;
	DWORD cbUsage = 0;
	int uses;

	CertGetEnhancedKeyUsage( pCertContext, 0, NULL, &cbUsage );
	pUsage = (PCERT_ENHKEY_USAGE) NewNoX(gpmo) BYTE [cbUsage] ;

	if( !pUsage )
	{
		dwError = ERROR_OUTOFMEMORY;

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);

		return dwError;
	}
	
	if( !CertGetEnhancedKeyUsage( pCertContext, 0, pUsage, &cbUsage ) )
	{  

		dwError = GetLastError();

		delete [] pUsage;

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
		return dwError;
	}

	// Is the cert for all uses?	
	if( 0 == pUsage->cUsageIdentifier )
	{
		dwError = GetLastError(); 

		delete [] pUsage;

		if( CRYPT_E_NOT_FOUND == dwError )
		{

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

			return ERROR_SUCCESS;

		}
		else
		{

			// If GetLastError returns CRYPT_E_NOT_FOUND, the certificate is 
			// good for all uses. If it returns zero, the certificate has no valid uses.

			dwError = (0 == dwError) ? CRYPT_E_NOT_FOUND : dwError;

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);

			return dwError;

		}
	}

	// Go thro' the array
	for(uses = 0; uses < (int)pUsage->cUsageIdentifier; uses++)
	{
		//Because szOID_PKIX_KP_SERVER_AUTH is "1.3.6.1.5.5.7.3.1", using case sensitive comparison is okey.
OACR_WARNING_PUSH
OACR_WARNING_DISABLE(SYSTEM_LOCALE_MISUSE , " INTERNATIONALIZATION BASELINE AT KATMAI RTM. FUTURE ANALYSIS INTENDED. ")
		if( CSTR_EQUAL == CompareStringA(LOCALE_SYSTEM_DEFAULT,
									 NORM_IGNOREWIDTH,
									 pUsage->rgpszUsageIdentifier[uses], -1,
									 szOID_PKIX_KP_SERVER_AUTH, -1)) 
OACR_WARNING_POP
		{

			delete [] pUsage;

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS );
	
			return ERROR_SUCCESS;

		}
	}

	delete [] pUsage;

	dwError = CRYPT_E_NOT_FOUND;	//closest error code. 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);

	return dwError;

}

DWORD Ssl::IsServerEncryptPossible()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), s_fSslServerReady);

	return s_fSslServerReady;
}

DWORD Ssl::LoadSecurityLibrary()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );
	
	DWORD dwRet;
	
	OSVERSIONINFO osvi;

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	
	if( !GetVersionEx( &osvi ) )
	{
		dwRet = GetLastError();

		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, dwRet );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

		return dwRet;
	}

	char *pszDllName;

	//win nt, 2k, xp and beyond
	if( osvi.dwMajorVersion >= 5 ||
		(osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)	 )
	{
		pszDllName = "security.dll";
	}
	//win me
	else if ( osvi.dwMajorVersion == 4 && osvi.dwMinorVersion >= 90 )
	{
		pszDllName = "secur32.dll";
	}
	//SSL is supported by schannel.dll in Win9X plateform. 
	//Whidbey supported Win98 second edition and upper. 
	//SNAC support WinME and upper.
	else
	{
		
#ifndef SNIX
		dwRet = SEC_E_UNSUPPORTED_FUNCTION;

		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, dwRet );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

		return dwRet;
	
#else
		// This code is borrowed from MDAC 2.8
		// According to MDSN, windows 95, 98 support schannel with schannel.dll
		g_fisWin9x = true;
		pszDllName = "schannel.dll";	
#endif
	 }

	s_hSecurity = SNILoadSystemLibraryA( pszDllName);

	if( s_hSecurity == NULL )
	{
		dwRet = GetLastError();

		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, dwRet );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

		return dwRet;
	}

	INIT_SECURITY_INTERFACE pfInitSecurityInterface;
	
	pfInitSecurityInterface = (INIT_SECURITY_INTERFACE)GetProcAddress( s_hSecurity, "InitSecurityInterfaceW" );

	if( pfInitSecurityInterface == NULL )
	{
		dwRet = GetLastError();
		
		FreeLibrary( s_hSecurity );
		s_hSecurity = NULL;
		
		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, dwRet );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

		return dwRet;
	}

	s_pfTable = pfInitSecurityInterface();
	if( s_pfTable == NULL )
	{
		dwRet = GetLastError();
		
		FreeLibrary( s_hSecurity );
		s_hSecurity = NULL;
		
		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, dwRet );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

		return dwRet;
	}

#ifdef SNIX
	if( g_fisWin9x )
	{
		Dfn = (_DFN) s_pfTable->Reserved4;
		Efn =  (_EFN) s_pfTable->Reserved3;
	}
#endif

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

	return ERROR_SUCCESS;
}

void Ssl::ProcessPendingIO()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );
	
	FreeReadWriteBuffers();
	
	CryptoBase::ProcessPendingIO();
}

void Ssl::ReleaseChannelBindings(void *pvChannelBindings)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("pvChannelBindings: %p{void *}\n"), pvChannelBindings );
	
	Assert( NULL != pvChannelBindings ); // internal-only API, and this indicates an internal coding bug
	Assert( s_fChannelBindingsSupported ); // otherwise, we should never get here.

	SecPkgContext_Bindings *pChannelBindings = (SecPkgContext_Bindings *)pvChannelBindings;

	if( NULL != pChannelBindings->Bindings )
	{
#ifdef SNI_UNIT
		// if we've been faking them, we must free them ourselves
		if( g_fUnitScenarios[eUnitTest_GenerateBadBindings] )
		{
			SNIUnitTestBadBindingsRelease((void *)(pChannelBindings->Bindings));
		}
		else
#endif
		{
			s_pfTable->FreeContextBuffer(pChannelBindings->Bindings);
		}
	}
	pChannelBindings->Bindings = NULL;
	pChannelBindings->BindingsLength = 0;
}

// Best-effort attempt at setting Channel Bindings. Behavior:
// 1) If OS supports Channel Bindings and they are queried successfully, they are assigned into the m_pConn's opaque pointer.
// 2) If OS supports Channel Bindings and querying for them fails, returns an error
// 3) If OS does not support Channel Bindings, returns Success. Logs a Bid Trace on the first query that finds them unsupported.
DWORD Ssl::SetChannelBindings()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );
	
	DWORD dwRet = SEC_E_OK;
	
#ifdef SNI_UNIT
	if( g_fUnitScenarios[eUnitTest_ForgetChannelBindings] )
	{
		dwRet = SEC_E_OK;
		goto Exit;
	}
#endif
	
	// API contract: SNI consumer cannot request another SSL handshake on an SNI_Conn which has already established an SSL context (which, other
	// than a coding bug within SNI, is the way to fire this assert).
	// If a previously-established SSL context has since been removed from the SNI_Conn, then the SNI_Conn should have itself released and 
	// NULLed its Channel Bindings pointer already; if not, it would be a coding bug.	
	Assert( NULL == m_pConn->m_pvChannelBindings );
	
	if( !s_fChannelBindingsSupported )
	{
		// Unsupported -> return success. Policy is not enforced by this function.
		dwRet = SEC_E_OK;
		goto Exit;
	}
	
	m_pConn->m_pvChannelBindings = (void *)(NewNoX(gpmo) SecPkgContext_Bindings);

	if( NULL == m_pConn->m_pvChannelBindings )
	{
		dwRet = ERROR_OUTOFMEMORY;
		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, dwRet );
		goto Exit;
	}
	
#ifdef SNI_UNIT
	if( g_fUnitScenarios[eUnitTest_GenerateBadBindings] )
	{
		DWORD dwLength = 0;
		dwRet = SNIUnitTestBadBindingsInit((void **)(& ((SecPkgContext_Bindings *) m_pConn->m_pvChannelBindings)->Bindings), &dwLength);
		if (ERROR_SUCCESS != dwRet)
		{
			SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, dwRet );
		}
		((SecPkgContext_Bindings *) m_pConn->m_pvChannelBindings)->BindingsLength = dwLength;
		goto Exit;
	}
#endif
	
	dwRet = s_pfTable->QueryContextAttributes(&m_hContext, SECPKG_ATTR_UNIQUE_BINDINGS, (SecPkgContext_Bindings *)m_pConn->m_pvChannelBindings);
	
	BidTraceU1(SNI_BID_TRACE_ON, SNI_TAG _T("QueryContextAttributes for SECPKG_ATTR_UNIQUE_BINDINGS return value: %d{DWORD}\n"), dwRet);
	
	if( SEC_E_UNSUPPORTED_FUNCTION == dwRet )
	{
		// Set the static flag so we exit fast in the future.
		s_fChannelBindingsSupported = false;
		
		BidTrace1(SNI_TAG _T("QueryContextAttributes for SECPKG_ATTR_UNIQUE_BINDINGS returned SEC_E_UNSUPPORTED_FUNCTION. Channel Bindings are not available to protect against Authentication Relay.\n"), dwRet);

		// Log that Channel Bindings are not supported, and that this may cause failures.
		// "Extended Protection for the SQL Server Database Engine is enabled, but the operating system does not support Extended Protection. ..."
		scierrlog( 26065 );
		
		// Unsupported -> deallocate the Channel Bindings buffer and return success. Policy is not enforced by this function.
		Assert( NULL != m_pConn->m_pvChannelBindings );
		delete m_pConn->m_pvChannelBindings;
		m_pConn->m_pvChannelBindings = NULL;
		
		dwRet = SEC_E_OK;
		
		goto Exit;
	}
	else if( SEC_E_OK != dwRet )
	{
		// other non-success return codes are SSPI failures. Deallocate the Channel Bindings and return failure - we cannot use the Schannel context.
		Assert(NULL != m_pConn->m_pvChannelBindings);
		delete m_pConn->m_pvChannelBindings;
		m_pConn->m_pvChannelBindings = NULL;
		
		BidTrace1(ERROR_TAG _T("QueryContextAttributes for SECPKG_ATTR_UNIQUE_BINDINGS return value: %d{DWORD}\n"), dwRet);
		goto Exit;
	}
	
	Assert( SEC_E_OK == dwRet );
	
Exit:
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;
}

Ssl::Ssl(SNI_Conn * pConn):CryptoBase( pConn )
{
	m_Prot = SSL_PROV;

	m_State = SSL_INIT;
	
	m_cbHeaderLength = 0;
	m_cbTrailerLength = 0;

	m_pWriteBuffer = 0;
	m_iWriteOffset = 0;
	m_cWriteBuffer = 0;

	m_pReadBuffer = 0;
	m_cReadBuffer = 0;

	m_fUseExistingCred = false;
	
	m_fIgnoreChannelBindings = false;
	
	SecInvalidateHandle( &m_hCred );
	SecInvalidateHandle( &m_hContext);

	m_fValidate = FALSE;

	m_cbMaximumMessage = 0;

	m_fConnBufSizeIncremented = false;
	
	BidObtainItemID2A( &m_iBidId, SNI_ID_TAG "%p{.} created by %u#{SNI_Conn}", 
		this, pConn->GetBidId() );
}

DWORD Ssl::Terminate()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );
	
	if( !rgProvInfo[SSL_PROV].fInitialized )
	{
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
		return ERROR_SUCCESS;
	}

	if( s_pfTable != NULL && SecIsValidHandle( &s_hClientCred ) )
	{
		s_pfTable->FreeCredentialsHandle(&s_hClientCred);
		SecInvalidateHandle(&s_hClientCred);
	}

	if( s_pfTable != NULL && SecIsValidHandle( &s_hClientCredValidate ) )
	{
		s_pfTable->FreeCredentialsHandle(&s_hClientCredValidate);
		SecInvalidateHandle(&s_hClientCredValidate);
	}

	s_pfTable = NULL;

	if( s_hSecurity != NULL )
	{
		FreeLibrary( s_hSecurity );
		s_hSecurity = NULL;
	}
	
	s_cbMaxToken = 0;

	rgProvInfo[SSL_PROV].fInitialized = FALSE; 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

	return ERROR_SUCCESS;
}

DWORD Ssl::TerminateListener(HANDLE hListener)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "hListener: %p{HANDLE}\n"), hListener);
	
	Assert( !hListener );

	if( FALSE == s_fSslServerReady )
	{
		// when encryption is not available no cleanup is required

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

		return ERROR_SUCCESS;
	}
	
	if( s_hCertContext )
	{ 
		// If s_hCertStore is NULL, then s_hCertContext is a pointer to the fallback certificate 
		// context managed by the static CSECFallBackCert class and we should not free 
		// s_hCertContext here.  See Ssl::InitializeListener for details.
		if ( s_hCertStore )
		{
			CertFreeCertificateContext(s_hCertContext);
		}
		s_hCertContext = 0;
	}

	if( s_hCertStore )
	{
		CertCloseStore( s_hCertStore, 0);
		s_hCertStore = 0;
	}
	
	if( SecIsValidHandle( &s_hServerCred))
	{
		s_pfTable->FreeCredentialsHandle( &s_hServerCred);
		SecInvalidateHandle( &s_hServerCred );
	}	
	
	s_fSslServerReady = FALSE;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

	return ERROR_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
//StrUprWWin9x_SYS
//
//In windows 98, some kernel32 unicode function is not available, etc. LCMapStringW. Thus,
//this helper function uses the ASCII conterpart as detour to conver unicode string to uppercase.
//This function use Windows kernel32.dll function to do locale sensitive  
//	Inputs:
//		[in] WCHAR *wszIn
//			The pointer to the src string.
//		[in] DWORD cchwszIn	
//			The src string buf size in cch, including null terminator.
//		[out] WCHAR *wszOut
//			The pointer to the dest string. It can be the same as src string. 
//		[in] DWORD cchwszOut
//			The dest string buf size in cch,
//		[out] DWORD* pdwRequired
//			The dest buf required if wszOut is null or cchwszOut does not have enought buffer size.
//
//	Returns:
//	If the call succeeds, the wszOut contains the converted string, the return value ERROR_SUCCESS is returned.
//	On failure, the exact system failure is returned.
//

DWORD StrUprWWin9x_SYS(__in_ecount(cchwszIn) WCHAR *wszIn, 
												DWORD cchwszIn, 
						__out_ecount_opt(cchwszOut) WCHAR *wszOut, 
												DWORD cchwszOut, 
												__out DWORD* pdwRequired)
{
	DWORD dwRet = ERROR_SUCCESS;
	char* szTmp = NULL;
	DWORD cbTmp =0 ;
	DWORD wcszTmp = 0;

	if( wszIn == NULL 
		||cchwszIn == 0
		||(wszOut == NULL && pdwRequired == NULL) ) 
	{
		return ERROR_INVALID_PARAMETER;
	}
		
	//Convert to ASCII
	if( 0 == ( cbTmp = WideCharToMultiByte(
				CP_ACP,
				0,
				wszIn,
				cchwszIn,
				NULL,
				0,
				NULL,
				NULL
				)))
	{
		dwRet = GetLastError();
		goto Exit;
	}

	szTmp = NewNoX (gpmo) char[cbTmp];
	if ( szTmp == NULL )
	{
		dwRet = ERROR_OUTOFMEMORY;
		goto Exit;
	}

	if( 0 == WideCharToMultiByte(
				CP_ACP,
				0,
				wszIn,
				cchwszIn,
				szTmp,
				cbTmp,
				NULL,
				NULL
				))
	{
		dwRet = GetLastError();
		goto Exit;
	}

	//Do uppercase in place
OACR_WARNING_PUSH
OACR_WARNING_DISABLE(SYSTEM_LOCALE_MISUSE , " INTERNATIONALIZATION BASELINE AT KATMAI RTM. FUTURE ANALYSIS INTENDED. ")
	DWORD cch = LCMapStringA(LOCALE_SYSTEM_DEFAULT,
					LCMAP_UPPERCASE,
					szTmp,
					cbTmp,
					szTmp,
					cbTmp
					);
OACR_WARNING_POP
	Assert( cch == cbTmp );
	if( 0 >= cch || cch != cbTmp )
	{
		dwRet = GetLastError();
		SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_10, dwRet );
		goto Exit;
	}		
	
	//Convert back to Unicode
	wcszTmp = MultiByteToWideChar(CP_ACP, 0, szTmp, cbTmp, NULL, 0);
	if( wcszTmp == 0)
	{
		dwRet = GetLastError();
		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, dwRet );
		goto Exit;
	}

	if ( NULL == wszOut  || wcszTmp > cchwszOut )
	{
		*pdwRequired = wcszTmp;
		dwRet =  ERROR_INSUFFICIENT_BUFFER;
		goto Exit;
	}

	wcszTmp = MultiByteToWideChar(CP_ACP, 0, szTmp, cbTmp, wszOut, cchwszOut);
	if(wcszTmp == 0)
	{
		dwRet = GetLastError();
		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, dwRet );
		goto Exit;
	}

Exit:

	if ( szTmp != NULL ) delete [] szTmp;
	return dwRet;	
	
}


DWORD Ssl::Validate(WCHAR * wszSvr)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("%u#, ")
							  _T("wszSvr: \"%ls\"\n"), 
							  GetBidId(),
							  wszSvr);
	
	PCCERT_CONTEXT pServerCert = NULL;
	SECURITY_STATUS scRet;
	HTTPSPolicyCallbackData  polHttps;
	CERT_CHAIN_POLICY_PARA   PolicyPara;
	CERT_CHAIN_POLICY_STATUS PolicyStatus;
	CERT_CHAIN_PARA          ChainPara;
	PCCERT_CHAIN_CONTEXT     pChainContext = NULL;
	WCHAR                    wszServerName[NI_MAXHOST +1];
	DWORD                    cwszServerName = 0;
	WCHAR*                    wszSubjectName = NULL;

	// Authenticate server's credentials.
	// Get server's certificate.
	scRet = s_pfTable->QueryContextAttributes(&m_hContext,
									SECPKG_ATTR_REMOTE_CERT_CONTEXT,
									&pServerCert);

	if(scRet != SEC_E_OK)
	{
		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, scRet );

		goto Exit;
	}

	Assert( pServerCert );

	scRet = Tcp::GetDnsName( wszSvr, wszServerName, ARRAYSIZE( wszServerName ));

	if( ERROR_SUCCESS != scRet )
	{
		goto Exit;
	}

	cwszServerName = wcslen(wszServerName) + 1;
	Assert(cwszServerName < ARRAYSIZE(wszServerName));

OACR_WARNING_PUSH
OACR_WARNING_DISABLE(SYSTEM_LOCALE_MISUSE , " INTERNATIONALIZATION BASELINE AT KATMAI RTM. FUTURE ANALYSIS INTENDED. ")
	DWORD cch = LCMapStringEx(LOCALE_NAME_SYSTEM_DEFAULT,
							LCMAP_UPPERCASE,
							wszServerName,
							cwszServerName,
							wszServerName,
							ARRAYSIZE(wszServerName),
							NULL, NULL, NULL);
OACR_WARNING_POP
	Assert(cch == cwszServerName);
	if( 0 >= cch || cch != cwszServerName)
	{
		scRet = GetLastError();
		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, scRet );
		goto Exit;
	}		
	
	if(cwszServerName == 0)
	{
		scRet = GetLastError();

		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, scRet );

		goto Exit;
	}

	wszServerName[cwszServerName - 1] = L'\0';	


	// Get the dns name of the server machine from the subject field
	// of the Server cert. If the server name passed in does not match then 
	// return FALSE
	DWORD cwszSubjectName = 0;

	if( 1 == (cwszSubjectName = CertGetNameStringW( pServerCert,
								 CERT_NAME_ATTR_TYPE,
								  0,
								  szOID_COMMON_NAME,
								  NULL,
								  0)) )
	{
		scRet = SEC_E_INTERNAL_ERROR;
		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, scRet );
		goto Exit;
	}

	wszSubjectName = (LPWSTR) NewNoX(gpmo) WCHAR[cwszSubjectName] ;

	if( !wszSubjectName )
	{
		scRet = ERROR_OUTOFMEMORY;
		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, scRet );
		goto Exit;
	}
	
	if( cwszSubjectName != CertGetNameStringW( pServerCert,
								 CERT_NAME_ATTR_TYPE,
								 0,
								 szOID_COMMON_NAME,
								  wszSubjectName,
								  cwszSubjectName) )
	{
		scRet = SEC_E_INTERNAL_ERROR;
		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, scRet );
		goto Exit;
	}

	if ( g_fisWin9x )
	{
		scRet = StrUprWWin9x_SYS(wszSubjectName,cwszSubjectName,wszSubjectName,cwszSubjectName,NULL);
		if( scRet != ERROR_SUCCESS )
		{
			BidTrace1( ERROR_TAG _T("Failed in StrUprWWin9x_SYS(): %d\n"), scRet); 
			SNI_SET_LAST_ERROR(SSL_PROV, SNIE_10, scRet);
			goto Exit;
		}
			
	}
	else
	{
			
OACR_WARNING_PUSH
OACR_WARNING_DISABLE(SYSTEM_LOCALE_MISUSE , " INTERNATIONALIZATION BASELINE AT KATMAI RTM. FUTURE ANALYSIS INTENDED. ")
		cch = LCMapStringW(LOCALE_SYSTEM_DEFAULT,
							LCMAP_UPPERCASE,
							wszSubjectName,
							cwszSubjectName,
							wszSubjectName,
							cwszSubjectName
							);
OACR_WARNING_POP
	 	Assert(cch == cwszSubjectName);
		if( 0 >= cch || cch != cwszSubjectName)
		{
			scRet = GetLastError();
			SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, scRet );
			goto Exit;
		}		
	}

	wszSubjectName[cwszSubjectName-1] = L'\0';

	//	Verify that the servername matches the entire FQDN in the cert subject 
	//	or at least the servername portion of the  FQDN.
	//	Given the FQDN of abc.corp.company.com, we should match 
	//	abc.corp.company.com and abc but not ab, cor, comp etc.
	//
	//	Server name should never be bigger than the FQDN in cert subject
	//
	if (cwszServerName > cwszSubjectName)
	{
		scRet = CERT_E_CN_NO_MATCH;
		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, scRet );
		BidTrace2(ERROR_TAG _T("Server SSL cert validation failed. Server name \"%ls\" did not match certificate subject \"%ls\"\n"), 
					wszServerName, wszSubjectName);

		goto Exit;
	}

	//	Compare the strings upto the size of the servername. 
	//	The last char in both strings is the NULL, we should exclude the NULL
	//	in the comparison, or our comparison will fail if the server name is
	//	not the FQDN and hence smaller than the cert subject
	//
	Assert (L'\0' == wszSubjectName[cwszSubjectName-1] );
	Assert (L'\0' == wszServerName[cwszServerName - 1] );
	if (0 != wcsncmp (wszSubjectName, wszServerName, cwszServerName-1))
	{
		scRet = CERT_E_CN_NO_MATCH;
		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, scRet );
		BidTrace2(ERROR_TAG _T("Server SSL cert validation failed. Server name \"%ls\" did not match certificate subject \"%ls\"\n"), 
					wszServerName, wszSubjectName);
		
		goto Exit;
	}		

	//	If the servername is not the FQDN, then ensure that the
	//	character following the server name is a '.'. This will avoid
	//	having server name ab match abc.corp.company.com
	//
	if (cwszServerName != cwszSubjectName)
	{
		Assert (cwszServerName < cwszSubjectName);
		if (L'.' != wszSubjectName[cwszServerName-1])
		{
			scRet = CERT_E_CN_NO_MATCH;
			SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, scRet );
			BidTrace2(ERROR_TAG _T("Server SSL cert validation failed. Server name \"%ls\" did not match certificate subject \"%ls\"\n"), 
						wszServerName, wszSubjectName);
			
			goto Exit;
		}				
	}

	//	If we are on Win2K or later, then validating the subject is
	//	sufficient. The rest of the certificate validation is done by
	//	schannel
	//
	if (gfIsWin2KPlatform)
	{
		scRet = SEC_E_OK;
		goto Exit;
	}
	
	//
	// Build certificate chain.
	ZeroMemory(&ChainPara, sizeof(ChainPara));
	ChainPara.cbSize = sizeof(ChainPara);

	if(!CertGetCertificateChain(
				NULL,
				pServerCert,
				NULL,
				pServerCert->hCertStore,
				&ChainPara,
				0,
				NULL,
				&pChainContext))
	{
		scRet = GetLastError();

		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, scRet );

		goto Exit;
	}

	// Validate certificate chain.

	ZeroMemory(&polHttps, sizeof(HTTPSPolicyCallbackData));
	polHttps.cbStruct           = sizeof(HTTPSPolicyCallbackData);
	polHttps.dwAuthType         = AUTHTYPE_SERVER;
	polHttps.fdwChecks          = 0;
	polHttps.pwszServerName     = wszSubjectName;

	memset(&PolicyPara, 0, sizeof(PolicyPara));
	PolicyPara.cbSize            = sizeof(PolicyPara);
	PolicyPara.pvExtraPolicyPara = &polHttps;

	memset(&PolicyStatus, 0, sizeof(PolicyStatus));
	PolicyStatus.cbSize = sizeof(PolicyStatus);

	if(!CertVerifyCertificateChainPolicy(
			CERT_CHAIN_POLICY_SSL,
			pChainContext,
			&PolicyPara,
			&PolicyStatus))
	{
		scRet = GetLastError();

		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, scRet );

		goto Exit;
	}

	if(PolicyStatus.dwError)
	{
		scRet = PolicyStatus.dwError;

		SNI_SET_LAST_ERROR( SSL_PROV, SNIE_10, scRet );

		goto Exit;
	}
	
	scRet = SEC_E_OK;

Exit:

	if( wszSubjectName )
		delete [] wszSubjectName;
	
	if( pChainContext )
		CertFreeCertificateChain(pChainContext);

	if( pServerCert )
		CertFreeCertificateContext(pServerCert); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), scRet);

	return scRet;
}

//---------------------------------------------------------------------
// NAME: Ssl::AdjustProtocolFields
//
// PURPOSE:
//		Based on the cryptomode, increments the provider sizes in the connection
//
// PARAMETERS:
//
// RETURNS:
//		Error code, e.g. OOM
//
// NOTES:
//	If any leftover packets are present, copies them to have the correct packets in place.
//
DWORD Ssl::AdjustProtocolFields()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	DWORD dwError = ERROR_SUCCESS;

	IncConnBufSize();

	// If leftover is present we need to copy it into a larger packet
	if (m_pLeftOver)
	{
		SNI_Packet* pTemp = NULL;

		dwError = CopyPacket(m_pLeftOver, &pTemp, SNIPacketGetKey(m_pLeftOver), SNI_Packet_Read);

		// Success or failure, we always release the left over and null the member variable
		SNIPacketRelease(m_pLeftOver);
		m_pLeftOver = NULL;

		if (ERROR_SUCCESS != dwError)
		{
			Assert(NULL == pTemp);
			SNI_SET_LAST_ERROR( m_Prot, SNIE_4, dwError );
			goto ErrorExit;
		}

		m_pLeftOver = pTemp;
	}

ErrorExit:
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);

	return dwError;
}

//---------------------------------------------------------------------
// NAME: Ssl::CopyPacket
//
// PURPOSE:
//		Copies a packet into a newly allocated one to accomodate new provider sizes and offset
//
// PARAMETERS:
//	pOldPacket	-	Pointer to the original packet. 
//	ppNewPacket	-	Pointer to pointer to return the new packet
//	pPacketKey	-	Packet key
//	ioType		-	IO type for the new packet allocation
//
// RETURNS:
//		Success, invalid data or OOM
//
// NOTES:
// 	It's a responsibility of the caller to release pOldPacket and NULL its pointers.
DWORD Ssl::CopyPacket( __inout SNI_Packet * pOldPacket, 
								__deref_out SNI_Packet ** ppNewPacket, 
								__in_opt LPVOID pPacketKey,
								SNI_Packet_IOType ioType)
{
	DWORD dwRet = ERROR_SUCCESS;
	BYTE * pbLeftOver = NULL; 
	DWORD cbLeftOver = 0; 
	
	*ppNewPacket = NULL; 

	// The packet sizes might have changed since the left-over
	// was stored.  We will allocate a new packet conforming
	// to the new sizes, and copy the data over.  That way
	// the buffer size of the returned packet is correct in 
	// case the upper layer will call PartialReadAsync() on it.  
	//
	SNI_Packet * pNewPacket = SNIPacketAllocate( m_pConn, ioType );
		
	if( NULL == pNewPacket )
	{
		dwRet = ERROR_OUTOFMEMORY;
		goto ErrorExit;
	}

	// Get the pointer to the left-over data and its size.  
	//
	SNIPacketGetData( pOldPacket, &pbLeftOver, &cbLeftOver ); 

	Assert( NULL != pbLeftOver ); 
	Assert( cbLeftOver == SNIPacketGetBufferSize( pOldPacket ) ); 


	// SQL BU DT 373408 - for Yukon we assume that:
	//	- There are no providers below SSL that have non-zero headers or 
	//	  trailers (this guarantees that the check below ensures that
	//	  the amount of data delivered to consumer is <= the amount
	//	  requested by the consumer)
	//	- The SSL provider is used only by the TDS consumer.  
	//	- If TDS removes the SSL provider it does it immediately after 
	//	  the first login packet.  
	//	- If a well-behaving TDS client sends a first login packet smaller 
	//	  than 4KB then the amount of data it can send before receiving 
	//	  the login response is not more than the minimum TDS packet size.  
	//	  Examples are:
	//		- MDAC/SNAC client tagging an Attention packet to Login 
	//		  packet.  
	//		- DB-Lib client sending a 2-packet login (and possibly
	//		  an additional tagged Attention packet).  
	//
	//	  As a result we assume that any plaintext left-over received
	//	  from a well-behaved client fits into one packet according
	//	  to the buffer sizes at this point.  
	//
	//	  If the amount of data is larger we return an error.  
	//
	if( cbLeftOver > SNIPacketGetBufActualSize( pNewPacket ) )
	{
		BidTrace2(ERROR_TAG _T("Left-over is too large. ")
			_T("cbLeftOver: %d{DWORD}, ")
			_T("Buffer Actual Size: %d{DWORD}\n"),
			cbLeftOver,
			SNIPacketGetBufActualSize( pNewPacket ) );

		dwRet = ERROR_INVALID_DATA;
		goto ErrorExit;
	}

	// Copy all left-over data.
	// Note: it is important that read packets are allocated
	// with 0 offset.  Otherwise, we could overrun the
	// new packet's buffer. 
	//
	SNIPacketSetData( pNewPacket, pbLeftOver, cbLeftOver );

	*ppNewPacket = (SNI_Packet *) pNewPacket;

	SNIPacketSetKey(*ppNewPacket, pPacketKey);

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

	return ERROR_SUCCESS; 


ErrorExit:

	if( NULL != pNewPacket )
	{	
		SNIPacketRelease( pNewPacket ); 
	}

	SNI_SET_LAST_ERROR( m_Prot, SNIE_4, dwRet );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;
}

//---------------------------------------------------------------------
// NAME: Ssl::DecConnBufSize
//
// PURPOSE:
//		Used at SNIRemoveProvider to adjust provider sizes in connection's properties
//
// PARAMETERS:
//
// RETURNS:
//		Nothing
//
// NOTES:
//
void Ssl::DecConnBufSize()
{
    BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

    if (m_fConnBufSizeIncremented)
    {
        SNI_PROVIDER_INFO ProvInfo;
        GetSSLProvInfo(&ProvInfo);
        
        DecrementConnBufSize(m_pConn, &ProvInfo);
            
        m_fConnBufSizeIncremented = FALSE;
    }

    return;
}

//---------------------------------------------------------------------
// NAME: Ssl::IncConnBufSize
//
// PURPOSE:
//		Does the actual provider sizes adjustment in the connection's properties
//
// PARAMETERS:
//
// RETURNS:
//		Nothing
//
// NOTES:
//
void Ssl::IncConnBufSize()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	if (!m_fConnBufSizeIncremented)
	{
		SNI_PROVIDER_INFO ProvInfo;
		GetSSLProvInfo(&ProvInfo);

		IncrementConnBufSize(m_pConn, &ProvInfo);

		m_fConnBufSizeIncremented = TRUE;
	}

	return;
}

//---------------------------------------------------------------------
// NAME: Ssl::GetSSLProvInfo
//
// PURPOSE:
//		Fills a passed SNI_PROVIDER_INFO struct with the header/trailer length 
//		information for this Ssl Provider instance.
//
// PARAMETERS:
//		pProvInfo 	Pointer to the struct to be filled - must not be NULL.
//
// RETURNS:
//		Nothing
//
// NOTES:
//
void Ssl::GetSSLProvInfo(SNI_PROVIDER_INFO *pProvInfo)
{
	pProvInfo->ProvNum = m_Prot;
	pProvInfo->fBaseProv = FALSE;
	pProvInfo->fInitialized = TRUE;

	// To handle split SSL records, we'll need to allocate a bigger packet buffer to fit the additional header and trailer,
	// and mark the header. 
	// 
	// An interesting note: dealing with split records effectively means dealing with variable-length "headers",
	// since the trailer is variable-length, and we now have a whole SSL record in the "header", since we want to align
	// the bytes of the packet at the second record, not the first, since the split is always 0/N or 1/(N-1)

	// Total additional space for a single packet is now *two* headers and *two* trailers
	pProvInfo->Size = 2 * (m_cbHeaderLength + m_cbTrailerLength);
	
	// Header offset may be as high as two full headers and one full trailer, since we'll align the bytes to start
	// at the second record, minus the length of the user data in the first record (which is either 0 or 1).
	pProvInfo->Offset = m_cbHeaderLength;
}
