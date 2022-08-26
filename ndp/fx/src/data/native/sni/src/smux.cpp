//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: smux.cpp
// @Owner: petergv, nantu
// @Test: sapnaj
//
// <owner current="true" primary="true">petergv</owner>
// <owner current="true" primary="false">nantu</owner>
//
// Purpose: SNI Smux Provider
//
// Notes:
//          
// @EndHeader@
//****************************************************************************

#include "snipch.hpp"
#include "smux.hpp"
#include "sni_sspi.hpp"

// !!! Important: the bit specified below must match BIDX_APIGROUP_SMUX_HEADER
// defined in sni_common.hpp.  
//
BID_METATEXT( _T("<ApiGroup|SNI|SMUX_HEADER> 0x00001000: SNI SMUX packet headers"));

// Note: IF_BidEnabledU checks control bit and then checks if this particular 
// call is enabled. BidTraceE and BidTraceBinE mean "pre-enabled", no extra check.
//
#define SNI_BID_TRACE_SMUX_HEADER( szText, pbSmuxHeader ) 	\
{	\
	BOOL bEna;	\
	IF_BidEnabledU( bEna, BidAreOn( BID_APIGROUP_TRACE | BIDX_APIGROUP_SMUX_HEADER ),	\
					SNI_SMUX_HEADER_TAG)	\
	{	\
		BidTraceE0( SNI_SMUX_HEADER_TAG szText ); 	\
		BidTraceBinE( pbSmuxHeader, SMUX_HEADER_SIZE );	\
	}	\
}

#define SMUX_HEADER_SIZE			16
#define SMUX_BUFFER_QUEUE_SIZE 		4

#define SMUX_IDENTIFIER 83

#define SMUX_SYN	1
#define SMUX_ACK	2
#define SMUX_FIN	4
#define SMUX_DATA	8


//Smux header structure
typedef struct 
{
	BYTE	SMID;
	BYTE	Flags;		
	USHORT	SessionId;
	DWORD	Length;
	DWORD	SequenceNumber;
	DWORD	HighWater;
}
SmuxHeader;

class Session : public SNI_Provider
{
	Smux    *m_pSmux;

	SNICritSec * m_CS;

	USHORT	m_SessionId;				//Session or Batch Id

	DWORD	m_SequenceNumberForSend;		
	DWORD	m_HighWaterForSend;		//The maximum sequence numbered packet we can send

	DWORD 	m_SequenceNumberForReceive;	//Sequence number for peer
	DWORD	m_HighWaterForReceive;		//Maximum sequence number for peer that this Session can accept
	DWORD 	m_LastHighWaterForReceive;			//Last ACK we send this is the limit for peer
	
	DynamicQueue m_ReadPacketQueue;

	DynamicQueue m_WritePacketQueue;

	DynamicQueue m_ReceivedPacketQueue;

	// Whether this SMUX Session has
	// 1) sent its SMUX FIN
	// 2) Enqueued a dummy packet to the IOCP to indicate intent to send the SMUX FIN ("Async Delayed FIN")
	// 3) Or is unable to send a FIN because of the underlying connection being torn down.
	bool 	m_fFINSentOrToSend;
	// Whether this SMUX Session has received its SMUX FIN
	bool 	m_fFINReceived;
	// Whether this SMUX Session has been released (Session::Release() has been called)
	bool 	m_fReleased;
	// Whether the underlying connection that this SMUX Session relies on has been broken
	bool 	m_fBadConnection;
	// Whether read/write requests from the upper layer should be explicitly denied, e.g. due to inability to guarantee correctness of packet stream
	bool	m_fFailRequests;

	SmuxHeader m_SmuxHeader;	//keep a header to avoid filling in non-changing fields every time

	bool		m_fSync;

	HANDLE 	m_lpReadHandles[2];
	HANDLE 	m_lpWriteHandles[2];
	bool 	m_fWaitingForWrite;
	bool	m_fInReadSync; 
	
public:

	DWORD Open(void);
	DWORD ReadSync(__out SNI_Packet ** ppNewPacket, int timeout);
	DWORD ReadAsync(__out SNI_Packet ** ppNewPacket, LPVOID pPacketKey);
	DWORD WriteSync(__out SNI_Packet * pPacket, SNI_ProvInfo * pProvInfo);
	DWORD WriteAsync(__out SNI_Packet * pPacket, SNI_ProvInfo * pProvInfo);
	DWORD ReadDone(__inout SNI_Packet ** ppPacket, __out SNI_Packet **ppLeftOver, DWORD dwBytes, DWORD dwError );
	DWORD WriteDone(__inout SNI_Packet ** ppPacket, DWORD dwBytes, DWORD dwError);	
	DWORD Close();
	void Release();
	void SetBadConnection();
	void ProcessFIN();
	void CallbackError();
	DWORD SendControlPacketIfGoodConn( BYTE Flag ); 
	DWORD CheckConnection();
	
	Session(SNI_Conn *pConnection, SNI_Provider *pNext, USHORT SessionId, __in Smux *pSmux);

	~Session();
	DWORD FInit();

private:

	void SendPendingPackets();
	
	//Fills SmuxHeader for data to be send
	void PrependSmuxHeader(SNI_Packet *pPacket, BYTE Flag);

	//Sends a Control packet like SYN, ACK or FIN
	DWORD SendControlPacket( BYTE Flag );

	//Sends a Control packet like SYN, ACK or FIN
	DWORD SendDataPacket( __out SNI_Packet *pPacket );

	//Check to see if we need to send an ACK
	bool NeedToSendACK()
	{
		Assert( m_HighWaterForReceive-m_LastHighWaterForReceive <=2 );

		return ( m_LastHighWaterForReceive+2 == m_HighWaterForReceive );
	}

	DWORD ProcessDataPacket(__inout SNI_Packet **ppPacket);

	void CleanUp();
	
	inline BOOL Session::SignalEventForReadWrite()
	{
		BOOL fRet = FALSE;
		LONG cRefAcc = 0;
		cRefAcc =  m_pSmux->DecrementSyncAccessCounter();
		Assert ( cRefAcc >= 0 );
		if ( cRefAcc > 0 )
		{
			 fRet = SetEvent( m_lpReadHandles[1] );
		}

		return fRet;
	}

	inline DWORD Session::WaitEventsToEnterRead( int timeout )
	{
		LONG cRefAcc = m_pSmux->IncrementSyncAccessCounter();	
		
		Assert ( cRefAcc > 0 ); 

		DWORD dwRet = ERROR_SUCCESS;

		// 0. If this is the first Session to access the worker, worker is free to access.
		// 1. There is no racing between checking queue conditiion and de/enqueue 
		// of m_ReceivedPacketQueue since enqueuue is sync by sync access to worker
		// dequeue is done only in readsync.
		// 2. if there is a packet in the queue, don't go worker, read the packet first.
		// Note: this code gives reading packet priority over accessing worker.
		// 
		if ( cRefAcc == 1 )
		{					
			if ( m_ReceivedPacketQueue.IsEmpty())
				// No incoming packet queued, access worker if this is first thread to access worker.
				//
				return WAIT_OBJECT_0+1;
			else
			{
				//Incomding packet ready, free worker and read packet.
				//
				SignalEventForReadWrite();
				dwRet = WaitForSingleObject(m_lpReadHandles[0], 0);
				Assert ( dwRet == WAIT_OBJECT_0 );
				return dwRet;
			}
		}

		return  WaitForMultipleObjects( 2, m_lpReadHandles, FALSE, timeout );	
	}

	inline DWORD Session::WaitEventsToEnterWrite( int timeout )
	{
		LONG cRefAcc = m_pSmux->IncrementSyncAccessCounter();	

		Assert ( cRefAcc > 0 ); 

		DWORD dwRet = ERROR_SUCCESS;

		// 0. If this is the first Session to access the worker, worker is free to access.
		// 1. if write is ok, don't go worker, write to the network first.
		// Note: this code gives writing packet priority over accessing worker.
		//
		if ( cRefAcc == 1 )
		{
			//Check if can write through
			//
			dwRet = WaitForSingleObject(m_lpWriteHandles[0], 0);
			if ( dwRet == WAIT_TIMEOUT )
			{
				//Write is not available and this is first thread to access worker,
				//access worker.
				//
				return WAIT_OBJECT_0+1;					
			}
			else
			{	
				//Write to the lower provider is ok, free worker and write through
				//
				SignalEventForReadWrite();
				return dwRet;
			}
		}
		return WaitForMultipleObjects( 2, m_lpWriteHandles, FALSE, timeout );		
	
	}


};

Session::~Session()
{
	BidRecycleItemIDA( &m_iBidId, SNI_ID_TAG ); 

	if( m_fSync )
	{
		Assert( m_lpReadHandles[0]==NULL );
		Assert( m_lpReadHandles[1]==NULL );
		Assert( m_lpWriteHandles[0]==NULL );
		Assert( m_lpWriteHandles[1]==NULL );
		Assert( m_fWaitingForWrite==false );
	}
}

void Session::CallbackError()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );

	CAutoSNICritSec a_cs(m_CS, SNI_AUTOCS_DO_NOT_ENTER );

	a_cs.Enter();

	while( !m_ReadPacketQueue.IsEmpty() )
	{
		Assert( !m_fSync );
		
		SNI_Packet *pPacket;

		pPacket = (SNI_Packet *) m_ReadPacketQueue.DeQueue();

		pPacket->m_OrigProv = SESSION_PROV;
		
		if( ERROR_SUCCESS != SNIPacketPostQCS(pPacket, 0) )
		{
			//this assertion is used to catch unexpected system call failure.
			Assert( 0 && "SNIPacketPostQCS failed\n" );
			BidTrace0( ERROR_TAG _T("SNIPacketPostQCS failed\n") );
		}
	}

	while( !m_WritePacketQueue.IsEmpty() )
	{
		Assert( !m_fSync );
		SNI_Packet *pPacket;
		pPacket = (SNI_Packet *)m_WritePacketQueue.DeQueue();

		SNIPacketDecrementOffset(pPacket, SMUX_HEADER_SIZE);
		SNIPacketSetBufferSize( pPacket, SNIPacketGetBufferSize(pPacket)+SMUX_HEADER_SIZE);

		pPacket->m_OrigProv = SESSION_PROV;
		
		if( ERROR_SUCCESS != SNIPacketPostQCS( pPacket, 0) )
		{
			//this assertion is used to catch unexpected system call failure.
			Assert( 0 && "SNIPacketPostQCS failed\n" );
			BidTrace0( ERROR_TAG _T("SNIPacketPostQCS failed\n") );
		}

	}
	
	a_cs.Leave(); 
}

DWORD Session::CheckConnection()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId());
	DWORD dwRet = ERROR_FAIL;

	if( m_fSync )
	{
		dwRet = ReadSync(NULL, 0);
	}
	else
	{
		// SMUX Async always either keeps an outstanding read posted
		// or has data packets queued up to return, so the Smux and the
		// Sessions always know if the transport is closed.
		dwRet = (m_fBadConnection || m_fFINReceived || m_fFailRequests) ? ERROR_FAIL : ERROR_SUCCESS;
	}
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;
}

void Session::CleanUp()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );

	//We should only clean up the Session once BOTH we have sent our own FIN packet, and
	//either we have received the SMUX FIN packet or once there is no chance we will receive the SMUX FIN packet because the transport as dead.
	Assert( m_fFINSentOrToSend && ( m_fFINReceived || m_fBadConnection));
	Assert( m_fReleased );

	Assert( m_CS != NULL );
	DeleteCriticalSection(&m_CS);

	if( m_fSync )
	{
		CloseHandle( m_lpReadHandles[0] );
		m_lpReadHandles[0] = NULL;
		m_lpReadHandles[1] = NULL;
		
		CloseHandle( m_lpWriteHandles[0] );
		m_lpWriteHandles[0] = NULL;
		m_lpWriteHandles[1] = NULL;
	}

	m_pSmux->ReleaseSessionRef();
}

//
// when we can't send a FIN packet we close the physical connection so that the other side won't stop responding
//

DWORD Session::Close()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );

	bool fClosePhysicalConnection = false;
	
	CAutoSNICritSec a_cs( m_CS, SNI_AUTOCS_DO_NOT_ENTER );

	a_cs.Enter();

	bool fAsyncDelayedFIN = false;
	
	if( !m_fBadConnection )
	{
		if( m_fSync )
		{
			if( ERROR_SUCCESS != SendControlPacket(SMUX_FIN) )
			{
				fClosePhysicalConnection = true;
			}
		}
		else
		{
			SNI_Packet *pPacket;

			pPacket = SNIPacketAllocate( m_pConn, SNI_Packet_Write );

			if( NULL == pPacket )
			{
				fClosePhysicalConnection = true;
			}
			else
			{
				PrependSmuxHeader( pPacket, SMUX_FIN );

				pPacket->m_OrigProv = SESSION_PROV;

				SNI_BID_TRACE_SMUX_HEADER( 
					_T( "To send internally:\n" ), 
					SNIPacketGetBufPtr( pPacket ) ); 

				m_pConn->AddRef( REF_InternalWrite );

				if( ERROR_SUCCESS == SNIPacketPostQCS( pPacket, SMUX_HEADER_SIZE) )
				{
					fAsyncDelayedFIN = true;
				}
				else
				{
					m_pConn->Release( REF_InternalWrite );
					
					//this assertion is used to catch unexpected system errors.
					Assert( 0 && "SNIPacketPostQCS failed\n" );
					BidTrace0( ERROR_TAG _T("SNIPacketPostQCS failed\n") );
					SNIPacketRelease( pPacket );

					fClosePhysicalConnection = true;
				}
			}
		}
	}
	
	if( !fAsyncDelayedFIN )
	{
		Assert( !m_fFINSentOrToSend );
		m_fFINSentOrToSend = true;

		SNI_Packet *pPacket;
		
		while( !m_ReceivedPacketQueue.IsEmpty())
		{
			pPacket = (SNI_Packet *)m_ReceivedPacketQueue.DeQueue();
				
			SNIPacketRelease( pPacket );
		}

		if( !m_fFINReceived && !m_fBadConnection)
		{
			CallbackError();

			a_cs.Leave(); 
		}
		else
		{
			a_cs.Leave(); 

			m_pSmux->RemoveSessionId( m_SessionId );
		}
	}
	else
	{
		a_cs.Leave(); 
	}
	
	if( fClosePhysicalConnection )
	{
		m_pSmux->InternalClose();
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;
}

DWORD Session::FInit()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );

	DWORD dwRet;

	dwRet = SNICritSec::Initialize(&m_CS);
	
	if(ERROR_SUCCESS != dwRet )
	{
		goto ErrorExit;
	}
	
	if( m_fSync )
	{
		m_lpReadHandles[0] = CreateSemaphore( 
					    NULL,   // no security attributes
					    0,   // initial count
					    SMUX_BUFFER_QUEUE_SIZE,   // maximum count
					    NULL);  // unnamed semaphore
					    
		if( NULL == m_lpReadHandles[0] )
		{
			dwRet = GetLastError();

			SNI_SET_LAST_ERROR( SESSION_PROV, SNIE_10, dwRet );
			
			goto ErrorExit;
		}
		
		m_lpReadHandles[1] = m_pSmux->GetMutexHandle();

		m_lpWriteHandles[0] = CreateEvent( 
					    NULL,   // no security attributes
					    FALSE,   // manual reset
					    FALSE,   // initial state
					    NULL);  // unnamed object
					    
		if( NULL == m_lpWriteHandles[0] )
		{
			dwRet = GetLastError();

			SNI_SET_LAST_ERROR( SESSION_PROV, SNIE_10, dwRet );

			goto ErrorExit;
		}
		
		m_lpWriteHandles[1] = m_pSmux->GetMutexHandle();

	}

	m_pSmux->AddSessionRef();

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;

ErrorExit:

	if(m_CS)
	{
		DeleteCriticalSection(&m_CS);
		Assert( m_CS == NULL );
	}

	if( m_fSync )
	{
		if( m_lpReadHandles[0] )
		{
			CloseHandle( m_lpReadHandles[0] );
			m_lpReadHandles[0] = NULL;
			m_lpReadHandles[1] = NULL;
		}

		if( m_lpWriteHandles[0] )
		{
			CloseHandle( m_lpWriteHandles[0] );
			m_lpWriteHandles[0] = NULL;
			m_lpWriteHandles[1] = NULL;
		}
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD Session::Open(void)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	CAutoSNICritSec a_cs( m_CS, SNI_AUTOCS_DO_NOT_ENTER );

	a_cs.Enter();

	DWORD dwRet;
	
	dwRet = SendControlPacket(SMUX_SYN);

	if( dwRet != ERROR_SUCCESS )
	{
		m_fBadConnection = true;
		m_fFINSentOrToSend = true;
	}
	
	a_cs.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

//
// this function shouldn't change any sequence number( because of Close() )
//

void Session::PrependSmuxHeader(SNI_Packet *pPacket, BYTE Flag)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("Flag: %d\n"), 
							  GetBidId(),
							  pPacket, 
							  Flag);

	m_SmuxHeader.Flags = Flag;
	m_SmuxHeader.Length = SNIPacketGetBufferSize(pPacket)
								+ SMUX_HEADER_SIZE;
	m_SmuxHeader.SequenceNumber = m_SequenceNumberForSend;
	m_SmuxHeader.HighWater = m_HighWaterForReceive;

	SNIPacketPrependData(pPacket, (BYTE *)&m_SmuxHeader, SMUX_HEADER_SIZE);
}

DWORD Session::ProcessDataPacket(__inout SNI_Packet **ppPacket)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("%u#, ")
							  _T("ppPacket: %p{SNI_Packet**}\n"), 
							  GetBidId(),
							  ppPacket);

	UNALIGNED SmuxHeader *pSmuxHeader;
	pSmuxHeader = (SmuxHeader *)SNIPacketGetBufPtr( *ppPacket );

	//
	// packet sequence number should be what we expect
	// and sequence number shouldn't exceed high water limit
	// and data packet must contain consumer data
	//
	
	if( pSmuxHeader->SequenceNumber != m_SequenceNumberForReceive+1  ||
		!( pSmuxHeader->SequenceNumber <= m_LastHighWaterForReceive ||
		      pSmuxHeader->SequenceNumber+SMUX_BUFFER_QUEUE_SIZE < m_LastHighWaterForReceive+SMUX_BUFFER_QUEUE_SIZE) ||
		pSmuxHeader->Length <= SMUX_HEADER_SIZE )
	{
		SNI_ASSERT_ON_INVALID_PACKET

		SNI_SET_LAST_ERROR( SESSION_PROV, SNIE_10, ERROR_INVALID_DATA );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_INVALID_DATA);
	
		return ERROR_INVALID_DATA;
	}

	m_SequenceNumberForReceive = pSmuxHeader->SequenceNumber;

	SNIPacketSetBufferSize( *ppPacket, pSmuxHeader->Length-SMUX_HEADER_SIZE);
	SNIPacketIncrementOffset( *ppPacket, SMUX_HEADER_SIZE);

	if( !m_ReadPacketQueue.IsEmpty() )
	{
		Assert( !m_fFINSentOrToSend );

		//
		// There can be pending read requests in the queue only for async connections
		//

		Assert( !m_fSync );
		
		m_HighWaterForReceive++;

		if( NeedToSendACK() )
		{
			DWORD dwRet;
			dwRet = SendControlPacket(SMUX_ACK);
			if( ERROR_SUCCESS != dwRet )
			{
				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
				return dwRet;
			}
		}

		SNI_Packet *pQueuedPacket;
		
		pQueuedPacket = (SNI_Packet *) m_ReadPacketQueue.DeQueue();

		SNIPacketSetKey( *ppPacket, SNIPacketGetKey( pQueuedPacket ) );

		SNIPacketRelease( pQueuedPacket );
		
		SNIPacketSetConnection( *ppPacket, m_pConn );
	}
	else
	{
		Assert( !m_fFINSentOrToSend );

		//
		// If the Session got here from ReadSync, the packet will be returned 
		// to it through function output arguments, so there is no need to 
		// enqueue it.  
		//
		if( m_fInReadSync && m_ReceivedPacketQueue.IsEmpty() )
		{
			Assert( m_fSync );

			SNIPacketSetConnection( *ppPacket, m_pConn );			
		}
		else
		{
			DWORD dwRet;

			dwRet = m_ReceivedPacketQueue.EnQueue( *ppPacket );
			if( ERROR_SUCCESS != dwRet )
			{
				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
		
				return dwRet;
			}

			SNIPacketSetConnection( *ppPacket, m_pConn );
			
			*ppPacket = 0;

			if( m_fSync )
			{
				//
				// assumption here is that, this call won't fail
				// if we decide to take care of error case, we need to return a non null *ppPacket
				//
		
				DWORD ret;
				ret = ReleaseSemaphore( m_lpReadHandles[0],  // handle to semaphore
								        1,           // increase count by one
								        NULL);

				Assert( ret );
			}
		}
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;
}

DWORD Session::ReadAsync(__out SNI_Packet ** ppNewPacket, LPVOID pPacketKey)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T( "ppNewPacket: %p{SNI_Packet**}, ")
							  _T("pPacketKey: %p\n"), 
							  GetBidId(),
							  ppNewPacket, 
							  pPacketKey);
	
	CAutoSNICritSec a_cs( m_CS, SNI_AUTOCS_DO_NOT_ENTER );

	a_cs.Enter();

	DWORD dwRet;

	Assert( !m_fFINSentOrToSend );

	Assert( *ppNewPacket == NULL );

	if( m_fFINReceived || m_fBadConnection || m_fFailRequests )
	{
		dwRet = ERROR_FAIL;

		SNI_SET_LAST_ERROR( SESSION_PROV, m_fFINReceived ? SNIE_18 : SNIE_19, dwRet );
		
		goto ExitFunc;
	}

	if( !m_ReceivedPacketQueue.IsEmpty() )
	{
		*ppNewPacket = (SNI_Packet *) m_ReceivedPacketQueue.DeQueue();

		SNIPacketSetKey( *ppNewPacket, pPacketKey);
		
		m_HighWaterForReceive++;	//client comsumed one buffer so increase window size

		Assert( m_ReadPacketQueue.IsEmpty() );
		
		if( NeedToSendACK())
		{
			dwRet = SendControlPacket(SMUX_ACK);

			if( ERROR_SUCCESS != dwRet)
			{
				SNIPacketRelease( *ppNewPacket );

				*ppNewPacket = NULL;
				
				goto ExitFunc;
			}
		}

		dwRet = ERROR_SUCCESS;
	}
	else
	{
		SNI_Packet *pPacket;

		pPacket = SNIPacketAllocate( m_pConn, SNI_Packet_KeyHolderNoBuf );

		if( NULL == pPacket )
		{
			dwRet = ERROR_OUTOFMEMORY;
			
			SNI_SET_LAST_ERROR( SESSION_PROV, SNIE_4, dwRet );

			goto ExitFunc;
		}
		
		SNIPacketSetKey( pPacket, pPacketKey );
		
		dwRet = m_ReadPacketQueue.EnQueue( pPacket );

		if( ERROR_SUCCESS == dwRet )
		{
			dwRet = ERROR_IO_PENDING;
		}
		else
		{
			SNIPacketRelease( pPacket );
		}
	}

ExitFunc:

	a_cs.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

//
// this function processes a complete smux packet targeted for this particular session
// when an error occurs ppPacket shouldn't be nulled out so that the caller can handle the problem. 
// this is especially important for async, because the caller is Smux::ReadDone and it needs 
// a packet to report an error 
// ppLeftOver isn't interesting for this function
// dwBytes is 0 when a packet is originated from session layer itself, in other cases it is ignored
// dwError is not used.
//

DWORD Session::ReadDone(__inout SNI_Packet ** ppPacket, __out SNI_Packet **ppLeftOver, DWORD dwBytes, DWORD dwError)
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
	
	CAutoSNICritSec a_cs( m_CS, SNI_AUTOCS_DO_NOT_ENTER );

	a_cs.Enter();

	DWORD dwRet;
	
	*ppLeftOver = 0;
	
	Assert( dwError==ERROR_SUCCESS );

	if( 0 == dwBytes )
	{
		Assert(  m_pConn == SNIPacketGetConnection( *ppPacket ) );

		Assert((*ppPacket)->m_OrigProv==SESSION_PROV);
		(*ppPacket)->m_OrigProv = INVALID_PROV;

		dwRet = ERROR_FAIL;

		SNI_SET_LAST_ERROR( SESSION_PROV, SNIE_19, dwRet );
		
		goto ExitFunc;
	}

	// If connection has been aborted, discard newly received packet.
	// connection abortion in session::setbadconnection has done necessary clean up already.
	if( m_fBadConnection )
	{
		SNIPacketRelease( *ppPacket );
		*ppPacket = NULL;
		dwRet = ERROR_SUCCESS;
		goto ExitFunc;

	}
	
	UNALIGNED SmuxHeader *pSmuxHeader;
	pSmuxHeader = (SmuxHeader *)SNIPacketGetBufPtr( *ppPacket );

	DWORD cbPacket;
	cbPacket = SNIPacketGetBufferSize( *ppPacket );

	Assert( sizeof(SmuxHeader) <= cbPacket );

	if(!( m_HighWaterForSend <= pSmuxHeader->HighWater 
		|| m_HighWaterForSend +SMUX_BUFFER_QUEUE_SIZE<pSmuxHeader->HighWater+SMUX_BUFFER_QUEUE_SIZE)
		||pSmuxHeader->Length != cbPacket 
		||m_fFINReceived
		||pSmuxHeader->SessionId != m_SessionId)
	{
		SNI_ASSERT_ON_INVALID_PACKET;
		dwRet = ERROR_INVALID_DATA;

		SNI_SET_LAST_ERROR( SESSION_PROV, SNIE_10, dwRet );

		goto ExitFunc;
	}

	//if this session is closed, discard the packets arrived
	if( m_fFINSentOrToSend && (pSmuxHeader->Flags!=SMUX_FIN) )
	{
		SNIPacketRelease( *ppPacket );

		*ppPacket = NULL;

		dwRet = ERROR_SUCCESS;

		goto ExitFunc;
	}

	if( m_fSync && m_SequenceNumberForSend==m_HighWaterForSend && 
		m_HighWaterForSend != pSmuxHeader->HighWater && m_fWaitingForWrite)
	{
		SetEvent( m_lpWriteHandles[0] );
	}
	
	m_HighWaterForSend = pSmuxHeader->HighWater;

	SendPendingPackets();
	
	if(pSmuxHeader->Flags == SMUX_DATA)
	{
		dwRet = ProcessDataPacket( ppPacket );

		if( dwRet != ERROR_SUCCESS )
		{
			goto ExitFunc;
		}
	}
	else
	{
		BYTE PacketType = pSmuxHeader->Flags;

		if( !( pSmuxHeader->SequenceNumber == m_SequenceNumberForReceive && PacketType == SMUX_ACK
			|| PacketType == SMUX_FIN ) )
		{
			SNI_ASSERT_ON_INVALID_PACKET;
			dwRet = ERROR_INVALID_DATA;

			SNI_SET_LAST_ERROR( SESSION_PROV, SNIE_10, dwRet );

			goto ExitFunc;
		}

		SNIPacketRelease( *ppPacket );
		*ppPacket = 0;

		if(  PacketType == SMUX_FIN )
		{
			Assert( !m_fFINReceived );
			
			m_fFINReceived = true;

			if( m_fReleased )
			{
				Assert( m_fFINSentOrToSend );

				if( !m_fBadConnection )
				{
					a_cs.Leave(); 

					m_pSmux->RemoveSessionId( m_SessionId );
				}
				else
				{
					a_cs.Leave(); 
				}
				
				CleanUp();

				delete this;

				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
				return ERROR_SUCCESS;
			}
			else
			{
				if( !m_fBadConnection )
				{
					if( !m_fFINSentOrToSend )
					{
						CallbackError();
					}
					else
					{
						Smux 	* pSmux = m_pSmux;

						USHORT 	SessionId = m_SessionId;
									
						a_cs.Leave(); 

						pSmux->RemoveSessionId( SessionId );

						BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
						return ERROR_SUCCESS;
					}
				}
			}
		}
	}

	dwRet = ERROR_SUCCESS;
	
ExitFunc:

	Assert( dwRet == ERROR_SUCCESS || *ppPacket );
	
	a_cs.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD Session::ReadSync(__out SNI_Packet ** ppNewPacket, int timeout)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T( "ppNewPacket: %p{SNI_Packet**}, ")
							  _T("timeout: %d\n"), 
							  GetBidId(),
							  ppNewPacket, 
							  timeout);

	Assert( m_pConn->m_fSync );

	CAutoSNICritSec a_cs( m_CS, SNI_AUTOCS_DO_NOT_ENTER );

	//	Note: checking m_fFINReceived and m_fBadConnection used to be protected
	//	by the m_CS critical section, which was left before the WaitForMultipleObjects()
	//	call.  However, since none of the checks needs consistency of multiple
	//	Smux fields/values, and since the CS did not protect their values
	//	between leaving it and the WaitForMultipleObjects() call, the CS did not
	//	seem to provide any value.  
	//

	DWORD dwRet;

	// ppNewPacket is NULL IFF Session::ReadSync is being called internally by Session::CheckConnection,
	// since it wants to post a read to poll the connection state, not to actually get a packet. Use this bool
	// to avoid two kinds of work: waiting, and actually processing a packet.
	bool fPollingForStatus = (NULL == ppNewPacket);

	Assert( !m_fFINSentOrToSend );

	if( NULL != ppNewPacket )
	{
		*ppNewPacket = NULL;
	}

	bool fFINReceived = m_fFINReceived;  

	if( fFINReceived || m_fBadConnection || m_fFailRequests )
	{
		dwRet = ERROR_FAIL;

		SNI_SET_LAST_ERROR( SESSION_PROV, fFINReceived ? SNIE_18 : SNIE_19, dwRet );

		goto ExitFunc;
	}

	// SQL BU DT 386395: Initializing to a dummy value to silence 
	// a PREfast warning, which appears to be benign.  
	//
	DWORD dwStart = 0;
	
	if( timeout != INFINITE && NULL != ppNewPacket)
	{
		dwStart = GetTickCount();
	}

	dwRet = WaitEventsToEnterRead( timeout );		

	//Can access worker
	//
	if( dwRet == WAIT_OBJECT_0+1)
	{

		// In case of session close, break out
		//
		fFINReceived = m_fFINReceived;
		if( fFINReceived || m_fBadConnection || m_fFailRequests )
		{
			SignalEventForReadWrite();
			dwRet = ERROR_FAIL;
			SNI_SET_LAST_ERROR( SESSION_PROV, fFINReceived ? SNIE_18 : SNIE_19, dwRet );
			goto ExitFunc;
		}

		// When checking if the connection is alive, always set TimeLeft to 0
		int TimeLeft = 0;
		if( NULL != ppNewPacket )
		{
			if( timeout == INFINITE )
			{
				TimeLeft = INFINITE;
			}
			else
			{
				TimeLeft = timeout-(GetTickCount()-dwStart);
				//once we get the mutex we want to do at least a polling
				if( TimeLeft < 0 )
				{
					TimeLeft = 0;
				}
			}
		}
		
		while(1)
		{
			if( timeout != INFINITE && TimeLeft < 0 )
			{
				SignalEventForReadWrite();			
				dwRet = WAIT_TIMEOUT;				
				SNI_SET_LAST_ERROR( SESSION_PROV, SNIE_11, dwRet );
				goto ExitFunc;
			}

			m_fInReadSync = true; 
			
			dwRet = m_pSmux->Worker( TimeLeft, /* ReadSync calls have a timeout to try to guarantee */
				ppNewPacket,  /* ReadSync calls want to get a packet back from the Worker */
				m_SessionId, /* tell the Worker which Session we want a packet for */
				fPollingForStatus ); /* tell the Worker if we are just polling for status, rather than looking for a packet */


			a_cs.Enter();		

			m_fInReadSync = false;
	
			if( dwRet != ERROR_SUCCESS )
			{
				//	Smux::Worker never returns a non-NULL packet with 
				//	a non-ERROR_SUCCESS return code.  
				//
				a_cs.Leave();	
			    Assert( NULL == ppNewPacket || NULL == *ppNewPacket ); 			
				SignalEventForReadWrite();				
				goto ExitFunc;
			}

			// In case of session close, break out
			//
			fFINReceived = m_fFINReceived;
			if( fFINReceived || m_fBadConnection || m_fFailRequests )
			{
				a_cs.Leave();
				SignalEventForReadWrite();
				dwRet = ERROR_FAIL;
				SNI_SET_LAST_ERROR( SESSION_PROV, fFINReceived ? SNIE_18 : SNIE_19, dwRet );
				goto ExitFunc;
			}
				
			// If we were just checking the connection, or if we were doing a read and got a packet for this
			// session, stop the worker loop
			if( NULL == ppNewPacket || *ppNewPacket )
			{
				break; 
			}

			//check if there is a packet enqueued for this session 
			//(generally, this should not happen)
			dwRet = WaitForSingleObject( m_lpReadHandles[0], 0 );
	
			if( dwRet == WAIT_OBJECT_0)
			{
				break;
			}
			else if ( dwRet != WAIT_TIMEOUT )
			{
				a_cs.Leave();
				SignalEventForReadWrite();
				SNI_SET_LAST_ERROR( SESSION_PROV, SNIE_10, dwRet );
				goto ExitFunc;
			}

			if( timeout != INFINITE )
			{
				TimeLeft = timeout-(GetTickCount()-dwStart);
			}

			a_cs.Leave();

		}
			
		// Got a new packet,  free up  smux::worker().
		//
		SignalEventForReadWrite();
	}
	else if ( dwRet == WAIT_TIMEOUT )
	{
		SNI_SET_LAST_ERROR( SESSION_PROV, SNIE_11, dwRet );
		goto ExitFunc;
	}
	else
	{
		Assert ( WAIT_OBJECT_0 == dwRet );
		a_cs.Enter();		
		fFINReceived = m_fFINReceived;
		if( fFINReceived || m_fBadConnection || m_fFailRequests )
		{
			a_cs.Leave();
			SignalEventForReadWrite();
			dwRet = ERROR_FAIL;
			SNI_SET_LAST_ERROR( SESSION_PROV, fFINReceived ? SNIE_18 : SNIE_19, dwRet );
			goto ExitFunc;
		}

	}

	Assert( !m_fBadConnection && !m_fFINReceived && !m_fFailRequests );

	// If we were just checking the connection, we have our answer, so we can leave ReadSync
	if( NULL == ppNewPacket )
	{
		a_cs.Leave();
		dwRet = ERROR_SUCCESS;
		goto ExitFunc;
	}

	//
	// If we obtained the packet directly from the Worker() call there is 
	// no need to dequeue.  
	//

	if( !*ppNewPacket )
	{
		*ppNewPacket = (SNI_Packet *) m_ReceivedPacketQueue.DeQueue();
	}

	Assert ( *ppNewPacket );
		
	m_HighWaterForReceive++;	//client comsumed one buffer so increase window size

	if( NeedToSendACK())
	{
		dwRet = SendControlPacket(SMUX_ACK);

		if( ERROR_SUCCESS != dwRet)
		{
			a_cs.Leave();
			
			SNIPacketRelease( *ppNewPacket );
			*ppNewPacket = NULL;
			
			goto ExitFunc;
		}
	}	

	a_cs.Leave();

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return  ERROR_SUCCESS;

ExitFunc:

	if( NULL != ppNewPacket )
	{
		if( *ppNewPacket )
		{
			SNIPacketRelease( *ppNewPacket ); 
			*ppNewPacket = NULL; 
		}
	}
	// If we were just checking the connection, and it timed out
	// then that is a success
	else if( dwRet == WAIT_TIMEOUT )
	{
		dwRet = ERROR_SUCCESS;
	}
				
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

void Session::Release()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T( "\n") );

	CAutoSNICritSec a_cs( m_CS, SNI_AUTOCS_DO_NOT_ENTER );

	a_cs.Enter();	
	Assert( m_fFINSentOrToSend );

	m_fReleased = true;

	if( m_fFINReceived || m_fBadConnection)
	{
		a_cs.Leave(); 

		CleanUp();
		
		delete this;
	}
	else
	{
		a_cs.Leave(); 
	}
}

// Note: Caller should own this Session's m_CS before calling this method.
DWORD Session::SendControlPacket( BYTE Flag )
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("%u#, ")
							  _T( "Flag: %d\n"), 
							  GetBidId(),
							  Flag );

	DWORD dwRet;
	
	SNI_Packet *pPacket;

	pPacket = SNIPacketAllocate( m_pConn, SNI_Packet_Write );

	if( pPacket == NULL )
	{
		dwRet = ERROR_OUTOFMEMORY;
		
		SNI_SET_LAST_ERROR( SESSION_PROV, SNIE_4, ERROR_OUTOFMEMORY );

		goto Exit;
	}

	PrependSmuxHeader( pPacket, Flag );

	Assert( !m_fBadConnection );


	SNI_BID_TRACE_SMUX_HEADER( 
		_T( "To send:\n" ), 
		SNIPacketGetBufPtr( pPacket ) ); 
	
	m_pConn->AddRef( REF_InternalWrite );
	
	if( m_fSync )
		dwRet = m_pNext->WriteSync( pPacket, NULL);
	else
		dwRet = m_pNext->WriteAsync( pPacket, NULL);

	if( dwRet == ERROR_SUCCESS || dwRet == ERROR_IO_PENDING )
	{
		if( dwRet == ERROR_IO_PENDING)
		{
			Assert( !m_fSync );
			
			dwRet = ERROR_SUCCESS;
		}
		else
		{
			m_pConn->Release( REF_InternalWrite );
			SNIPacketRelease( pPacket );
		}
		m_LastHighWaterForReceive = m_HighWaterForReceive;
	}
	else
	{
		m_pConn->Release( REF_InternalWrite );		
		SNIPacketRelease( pPacket );
	}

Exit:

	if( ERROR_SUCCESS != dwRet )
	{
		// Set the Connection to be in a bad state, so that in future we know
		// not to attempt any I/O on this connection
		m_fFailRequests = true;
	}
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD Session::SendControlPacketIfGoodConn( BYTE Flag )
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("%u#, ")
							  _T( "Flag: %d\n"), 
							  GetBidId(),
							  Flag );

	CAutoSNICritSec a_cs(m_CS, SNI_AUTOCS_DO_NOT_ENTER );

	a_cs.Enter();

	DWORD dwRet;

	if( m_fBadConnection )
	{
		dwRet = ERROR_FAIL;

		SNI_SET_LAST_ERROR( SESSION_PROV, SNIE_19, dwRet );

		goto ExitFunc;
	}

	dwRet = SendControlPacket(Flag); 

	
ExitFunc:

	a_cs.Leave();

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;	
}

DWORD Session::SendDataPacket( __out SNI_Packet *pPacket )
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("%u#, ")
							  _T( "pPacket: %p{SNI_Packet*}\n"), 
							  GetBidId(),
							  pPacket );

	DWORD dwRet;
	
	Assert(m_SequenceNumberForSend < m_HighWaterForSend || m_SequenceNumberForSend +SMUX_BUFFER_QUEUE_SIZE<m_HighWaterForSend+ SMUX_BUFFER_QUEUE_SIZE);
	m_SequenceNumberForSend++;	//first we increment Sequence number

	PrependSmuxHeader( pPacket, SMUX_DATA);

	Assert( !m_fFINSentOrToSend );
	Assert( !m_fBadConnection );

	SNI_BID_TRACE_SMUX_HEADER(
		_T( "To send:\n" ), 
		SNIPacketGetBufPtr( pPacket ) ); 
	
	if( m_fSync )
		dwRet = m_pNext->WriteSync( pPacket, NULL);
	else
		dwRet = m_pNext->WriteAsync( pPacket, NULL);

	if(dwRet == ERROR_SUCCESS || dwRet == ERROR_IO_PENDING )
	{
		Assert( dwRet != ERROR_IO_PENDING || !m_fSync);

		m_LastHighWaterForReceive = m_HighWaterForReceive;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

void Session::SendPendingPackets()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	//if there is a packet waiting for room from peer and there is room send it
	while( m_SequenceNumberForSend!=m_HighWaterForSend && !m_WritePacketQueue.IsEmpty() )
	{
		Assert( !m_fFINSentOrToSend );
		
		SNI_Packet *pPacket;
		pPacket = (SNI_Packet *) m_WritePacketQueue.DeQueue();

		DWORD dwRet;
		
		dwRet = SendDataPacket( pPacket );
		if( ERROR_IO_PENDING != dwRet)
		{
			DWORD cPacket;
			if(ERROR_SUCCESS == dwRet )
				cPacket = SNIPacketGetBufferSize( pPacket);
			else
				cPacket = 0;

			pPacket->m_OrigProv = SESSION_PROV;
			
			if( ERROR_SUCCESS != SNIPacketPostQCS( pPacket, cPacket) )
			{
				
				//this assertion is used to catch unexpected system call failure.
				Assert( 0 && "SNIPacketPostQCS failed\n" );
				BidTrace0( ERROR_TAG _T("SNIPacketPostQCS failed\n") );
				
			}
		}
	}
}

Session::Session(SNI_Conn *pConn, SNI_Provider *pNext, USHORT SessionId, __in Smux *pSmux): SNI_Provider( pConn )
{
	m_pNext = pNext;
	m_Prot = SESSION_PROV;

	m_pSmux = pSmux;
	
	m_CS = NULL;

	m_SessionId = SessionId;
	
	m_SequenceNumberForSend = 0;	
	m_HighWaterForSend = SMUX_BUFFER_QUEUE_SIZE;

	m_SequenceNumberForReceive = 0;
	m_HighWaterForReceive = SMUX_BUFFER_QUEUE_SIZE;	//Default size of PacketQueue
	m_LastHighWaterForReceive = SMUX_BUFFER_QUEUE_SIZE;		//Peer assume this value at the start


	m_fFINSentOrToSend = false;
	m_fFINReceived = false;
	m_fReleased = false;
	m_fBadConnection = false;
	m_fFailRequests = false;

	m_SmuxHeader.SMID = SMUX_IDENTIFIER;
	m_SmuxHeader.SessionId = m_SessionId;

	m_fSync = m_pConn->m_fSync;
	m_lpReadHandles[0] = NULL;
	m_lpWriteHandles[0] = NULL;
	m_fWaitingForWrite = false;
	m_fInReadSync = false; 

	BidObtainItemID2A( &m_iBidId, SNI_ID_TAG "%p{.} created by %u#{SNI_Conn}", 
		this, pConn->GetBidId() );
	BidUpdateItemIDA( &m_iBidId, SNI_ID_TAG "SessionId: %d", SessionId );

	BidTraceU2( SNI_BID_TRACE_ON, SNI_TAG _T("%u#, ")
										 _T("Smux: %u#{Smux}\n"), 
										 m_iBidId, 
										 pSmux->GetBidId() ); 
}

void Session::SetBadConnection()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	CAutoSNICritSec a_cs( m_CS, SNI_AUTOCS_DO_NOT_ENTER );

	a_cs.Enter();

	//
	// this function can be called multiple times, make sure we do processing only once
	//
	
	if( m_fBadConnection )
	{
		a_cs.Leave(); 

		return;
	}
	
	m_fBadConnection = true;

	if( m_fReleased )
	{
		Assert( m_fFINSentOrToSend );

		if( !m_fFINReceived )
		{
			a_cs.Leave(); 

			m_pSmux->RemoveSessionId( m_SessionId );
		}
		else
		{
			a_cs.Leave(); 
		}
		
		CleanUp();
		
		delete this;
	}
	else
	{
		if( m_fFINReceived )
		{
			a_cs.Leave(); 
		}
		else
		{
			if( !m_fFINSentOrToSend )
			{
				CallbackError();

				a_cs.Leave(); 
			}
			else
			{
				Smux 	* pSmux = m_pSmux;

				USHORT 	SessionId = m_SessionId;
							
				a_cs.Leave(); 

				pSmux->RemoveSessionId( SessionId );
			}
		}
	}
}

DWORD Session::WriteAsync(__out SNI_Packet * pPacket, SNI_ProvInfo * pProvInfo)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("pProvInfo: %p{SNI_ProvInfo*}\n"), 
							  GetBidId(),
							  pPacket, 
							  pProvInfo);

	CAutoSNICritSec a_cs( m_CS, SNI_AUTOCS_DO_NOT_ENTER );

	a_cs.Enter();

	DWORD dwRet;

	Assert( !m_fFINSentOrToSend );

	if( m_fFINReceived || m_fBadConnection || m_fFailRequests )
	{
		dwRet = ERROR_FAIL;

		SNI_SET_LAST_ERROR( SESSION_PROV, SNIE_19, dwRet );

		goto ExitFunc;
	}
	
	Assert( !m_fFINSentOrToSend );
	if( m_SequenceNumberForSend != m_HighWaterForSend )	//don't change this, it handles overflow correctly
	{
		Assert( m_WritePacketQueue.IsEmpty() );
		dwRet = SendDataPacket( pPacket );
	}
	else
	{
		dwRet = m_WritePacketQueue.EnQueue( pPacket );
		if( ERROR_SUCCESS == dwRet )
		{
			dwRet = ERROR_IO_PENDING;
		}
	}
	
ExitFunc:

	a_cs.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD Session::WriteDone(__inout SNI_Packet ** ppPacket, DWORD dwBytes, DWORD dwError)
{
	BidxScopeAutoSNI4( SNIAPI_TAG _T("%u#, ")
							  _T("ppPacket: %p{SNI_Packet**}, ")
							  _T("dwBytes: %d, ")
							  _T("dwError: %d{WINERR}\n"), 
							  GetBidId(),
							  ppPacket, 
							  dwBytes, 
							  dwError);

	CAutoSNICritSec a_cs( m_CS, SNI_AUTOCS_DO_NOT_ENTER );

	a_cs.Enter();

	DWORD dwRet;

	bool fOriginator = false;
	
	if( (*ppPacket)->m_OrigProv!=SESSION_PROV )
	{
		dwRet = m_pNext->WriteDone( ppPacket,  dwBytes, dwError);
	}
	else
	{
		fOriginator = true;
		(*ppPacket)->m_OrigProv = INVALID_PROV;
		
		Assert( dwError==ERROR_SUCCESS );
		if( dwBytes==0 )
		{
			dwRet = ERROR_FAIL;

			SNI_SET_LAST_ERROR( SESSION_PROV, SNIE_19, dwRet );
		}
		else
		{
			dwRet = ERROR_SUCCESS;
		}
	}
	
	Assert( *ppPacket );
	Assert( dwBytes ||ERROR_SUCCESS != dwRet);

	if( SMUX_HEADER_SIZE == SNIPacketGetBufferSize( *ppPacket) )	//control packet
	{
		BYTE PacketType = ((SmuxHeader *)SNIPacketGetBufPtr( *ppPacket ))->Flags;

		if( fOriginator && PacketType == SMUX_FIN )
		{
			Assert( !m_fFINSentOrToSend );

			BOOL fDelayAck = FALSE; 

			if( m_fBadConnection )
			{
				dwRet = ERROR_FAIL;
			}
			else
			{
				//
				// we need to update the header here, flow control variables might have
				// been updated since this packet traveled from Close to io completion thread
				//

				SNIPacketSetBufferSize( *ppPacket, 0);
				SNIPacketIncrementOffset( *ppPacket, SMUX_HEADER_SIZE );

				PrependSmuxHeader( *ppPacket, SMUX_FIN );

				//
				// If we have already received the FIN packet from the client we 
				// will remove the session.  We will acknowledge the FIN only after 
				// the removal so that they do not reuse it before we are done.  
				//
				
				if( !m_fFINReceived )
				{
					SNI_BID_TRACE_SMUX_HEADER( 
						_T( "To send:\n" ), 
						SNIPacketGetBufPtr( *ppPacket ) ); 
					
					m_pConn->AddRef( REF_InternalWrite );
					
					dwRet = m_pNext->WriteAsync( *ppPacket, NULL);

					if( dwRet == ERROR_IO_PENDING )
					{
						*ppPacket = NULL;

						dwRet = ERROR_SUCCESS;
					}
					else
					{
						m_pConn->Release( REF_InternalWrite );
					}
				}
				else
				{
					fDelayAck = TRUE; 
				}
			}

			m_fFINSentOrToSend = true;

			SNI_Packet *pPacket;
			
			while( !m_ReceivedPacketQueue.IsEmpty())
			{
				pPacket = (SNI_Packet *)m_ReceivedPacketQueue.DeQueue();
					
				SNIPacketRelease( pPacket );
			}

			if( !m_fFINReceived && !m_fBadConnection)
			{
				CallbackError();

				a_cs.Leave(); 
			}
			else
			{
				a_cs.Leave(); 

				if( fDelayAck )
				{
					m_pSmux->RemoveSessionIdWithCtrlPkt( m_SessionId, SMUX_FIN );
				}
				else
				{
					m_pSmux->RemoveSessionId( m_SessionId );
				}
			}
		}
		else
		{
			a_cs.Leave(); 
		}

		///Assert( ERROR_SUCCESS == dwRet ||PacketType != SMUX_ACK || m_fBadConnection );

		if( *ppPacket )
		{
			SNIPacketRelease( *ppPacket );

			*ppPacket = 0;
		}

		m_pConn->Release(REF_InternalWrite);

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
		return dwRet;

	}
	else	// data packet
	{
		SNIPacketSetBufferSize( *ppPacket, SNIPacketGetBufferSize( *ppPacket)-SMUX_HEADER_SIZE);
		SNIPacketIncrementOffset( *ppPacket, SMUX_HEADER_SIZE);

		a_cs.Leave(); 

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
		return dwRet;
	}
}

DWORD Session::WriteSync(__out SNI_Packet * pPacket, SNI_ProvInfo * pProvInfo)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("pProvInfo: %p{SNI_ProvInfo*}\n"), 
							  GetBidId(),
							  pPacket, 
							  pProvInfo);

	Assert( m_pConn->m_fSync );

	CAutoSNICritSec a_cs( m_CS, SNI_AUTOCS_DO_NOT_ENTER );

	a_cs.Enter();

	DWORD dwRet;

	Assert( !m_fFINSentOrToSend );

	if( m_fFINReceived || m_fBadConnection || m_fFailRequests )
	{
		dwRet = ERROR_FAIL;

		SNI_SET_LAST_ERROR( SESSION_PROV, m_fFINReceived ? SNIE_18 : SNIE_19, dwRet );

		goto ExitFunc;
	}
	
	Assert( !m_fFINSentOrToSend );

	//check for flow control permission
	if( m_SequenceNumberForSend == m_HighWaterForSend )
	{
		m_fWaitingForWrite = true;
		
		//wait until flow control lets us send the packet
		a_cs.Leave(); 

		dwRet = WaitEventsToEnterWrite( INFINITE );

		a_cs.Enter();

		m_fWaitingForWrite = false;

		if( dwRet == WAIT_OBJECT_0)
		{
			if( m_fFINReceived || m_fBadConnection || m_fFailRequests )
			{
				dwRet = ERROR_FAIL;

				SNI_SET_LAST_ERROR( SESSION_PROV, m_fFINReceived ? SNIE_18 : SNIE_19, dwRet );

				goto ExitFunc;
			}
		}
		else if( dwRet == WAIT_OBJECT_0+1)
		{
			if( m_fFINReceived || m_fBadConnection || m_fFailRequests )
			{
				SignalEventForReadWrite();

				dwRet = ERROR_FAIL;

				SNI_SET_LAST_ERROR( SESSION_PROV, m_fFINReceived ? SNIE_18 : SNIE_19, dwRet );

				goto ExitFunc;
			}

			while(m_SequenceNumberForSend == m_HighWaterForSend)
			{
				a_cs.Leave(); 

				dwRet = m_pSmux->Worker( INFINITE, /* WriteSync calls always have an INFINITE timeout */
					NULL, /* WriteSync isn't interested in reading a packet */
					0, /* WriteSync isn't interested in reading a packet, so there's no need to say which Session this is */
					false ); /* WriteSync never polls for connection status */


				a_cs.Enter();

				if( dwRet != ERROR_SUCCESS )
				{
					SignalEventForReadWrite();

					goto ExitFunc;
				}

				if( m_fFINReceived || m_fBadConnection || m_fFailRequests )
				{
					SignalEventForReadWrite();

					dwRet = ERROR_FAIL;

					SNI_SET_LAST_ERROR( SESSION_PROV, m_fFINReceived ? SNIE_18 : SNIE_19, dwRet );

					goto ExitFunc;
				}
			}

			SignalEventForReadWrite();
		}
		else
		{
			//this assertion is used to catch unexpected system call failure.
			Assert( 0 && "WaitForMultipleObjects failed\n" );
			BidTrace0( ERROR_TAG _T("WaitForMultipleObjects failed\n") );

			SNI_SET_LAST_ERROR( SESSION_PROV, SNIE_10, dwRet );

			goto ExitFunc;
		}

	}

	Assert( !m_fFINReceived && !m_fBadConnection && !m_fFailRequests );
	
	Assert( m_SequenceNumberForSend != m_HighWaterForSend );

	dwRet = SendDataPacket( pPacket );

	DWORD cbPacket;
	cbPacket = SNIPacketGetBufferSize( pPacket);
	SNIPacketSetBufferSize( pPacket, cbPacket-SMUX_HEADER_SIZE);
	SNIPacketIncrementOffset( pPacket, SMUX_HEADER_SIZE);
	
ExitFunc:

	a_cs.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}


Smux::~Smux()
{
	BidRecycleItemIDA( &m_iBidId, SNI_ID_TAG ); 

	Assert( m_SmuxCS == NULL );

	Assert( m_SessionListCS == NULL );

	Assert( !m_pPacketKeyHolder );

	Assert( !m_nSessions );
	
	for( DWORD i=0;i<m_MaxSessions;i++)
		Assert( !m_rgSessions[i]);

	delete [] m_rgSessions;

	if( m_fSync )
	{

		Assert( m_SyncWorkerMutex == NULL );
		Assert( m_pLeftOver == NULL );
	}
}

DWORD Smux::AcceptConnection( __out SNI_Conn **ppConn, USHORT SessionId  )
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("ppConn: %p{SNI_Conn**}, ")
							  _T("SessionId: %d\n"), 
							  GetBidId(),
							  ppConn, 
							  SessionId );

	DWORD dwRet;
	
	SNI_Conn * pConn;

	dwRet = SNI_Conn::InitObject( &pConn, TRUE);

	if( ERROR_SUCCESS != dwRet )
	{
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
		return dwRet;
	}
	
	dwRet = AcceptSession( pConn, SessionId );
	if( dwRet == ERROR_SUCCESS )
	{
		*ppConn = pConn;
	}
	else
	{
		pConn->Release(REF_Active);
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD Smux::AcceptSession( __inout SNI_Conn *pConn, USHORT SessionId)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pConn: %p{SNI_Conn*}, ")
							  _T("SessionId: %d\n"), 
							  GetBidId(),
							  pConn, 
							  SessionId );

	CAutoSNICritSec a_csSessionList( m_SessionListCS, SNI_AUTOCS_DO_NOT_ENTER );

	a_csSessionList.Enter();

	DWORD dwRet;
	
	Assert( !m_cClosed );
	
	Assert( m_nSessions <= m_MaxSessions);
	Assert( SessionId <= m_MaxSessions);
	
	// Session ID's are two bytes.  
	Assert( USHRT_MAX >= m_nSessions ); 

	if( USHRT_MAX < m_nSessions )
	{
		dwRet = ERROR_INVALID_STATE;
	
		SNI_SET_LAST_ERROR( SMUX_PROV, SNIE_4, ERROR_INVALID_STATE );

		goto ExitFunc;
	}

	if( SessionId == m_MaxSessions )	//if we don't have extra room for a new session then allocate
	{
		dwRet = GrowSessionList();

		if( dwRet != ERROR_SUCCESS )
		{
			goto ExitFunc;
		}
	}

	Session *	pSession = NewNoX(gpmo) Session( pConn, m_pNext, SessionId, this );

	if( !pSession )
	{
		dwRet = ERROR_OUTOFMEMORY;
		
		SNI_SET_LAST_ERROR( SMUX_PROV, SNIE_4, dwRet );

		goto ExitFunc;
	}
	
	dwRet = pSession->FInit();

	if( ERROR_SUCCESS != dwRet )
	{
		delete pSession;

		goto ExitFunc;
	}

	Assert( !m_rgSessions[SessionId] );
	m_nSessions++;

	m_rgSessions[SessionId] = pSession;

	// Copy the relevant SNI_CONN_INFO and other information
	pConn->m_ConnInfo.ConsBufferSize = m_pConn->m_ConnInfo.ConsBufferSize;
	pConn->m_ConnInfo.ProvBufferSize = m_pConn->m_ConnInfo.ProvBufferSize;
	pConn->m_ConnInfo.ProvOffset = m_pConn->m_ConnInfo.ProvOffset;
	pConn->m_ConnInfo.TransportProvider = m_pConn->m_ConnInfo.TransportProvider;

	pConn->m_pSec->SetSecPkgId(m_pConn->m_pSec->GetSecPkgId());
	
	pConn->m_MemTag = m_pConn->m_MemTag;

	// At server, we keep one Client ConnectionID for all MARS sessions
	// on the same physical connection
	SNISetInfo( pConn, SNI_QUERY_CONN_PEERID,  (void*)&(m_pConn ->GetConnPeerId()));


	// Copy the relevant EP info
	memcpy(pConn->m_pEPInfo, m_pConn->m_pEPInfo, sizeof(SNI_EPInfo));

	// Set the callbacks, etc.
	pConn->m_ConsumerInfo.DefaultUserDataLength = m_pConn->m_ConsumerInfo.DefaultUserDataLength;
	pConn->m_ConsumerInfo.fnReadComp = m_pConn->m_ConsumerInfo.fnReadComp;
	pConn->m_ConsumerInfo.fnWriteComp = m_pConn->m_ConsumerInfo.fnWriteComp;
	pConn->m_ConsumerInfo.fnAcceptComp = m_pConn->m_ConsumerInfo.fnAcceptComp;
	
	// Set m_pProvHead to point to the Transport provider
	pConn->m_pProvHead = pSession;

	// Set the SNI_Conn pointer in the provider
	pSession->m_pConn = pConn;

ExitFunc:
	
	a_csSessionList.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

void Smux::AddSessionRef()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );
	
	m_pConn->AddRef( REF_InternalActive );
}

void Smux::CleanUp()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );
	
	Assert( m_cClosed );

	Assert( m_SmuxCS != NULL );
	DeleteCriticalSection( &m_SmuxCS );

	Assert( m_SessionListCS != NULL );
	DeleteCriticalSection( &m_SessionListCS );

	if( m_fSync )
	{
		CloseHandle( m_SyncWorkerMutex );
		m_SyncWorkerMutex = NULL;

		Assert( !m_pLeftOver );
	}
}

DWORD Smux::Close()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );
	
	CAutoSNICritSec a_csSmux( m_SmuxCS, SNI_AUTOCS_DO_NOT_ENTER );

	a_csSmux.Enter();

	CAutoSNICritSec a_csSessionList( m_SessionListCS, SNI_AUTOCS_DO_NOT_ENTER );

	a_csSessionList.Enter();

	Assert( !( m_cClosed & 0x80000000 ) );

	DWORD dwRet;

	if( !m_cClosed )
	{
		m_cClosed |= 0x80000000;
	
		//
		// terminate sessions before close to make sure they won't try to access lower layer after close
		//

		TerminateSessions();

		dwRet = m_pNext->Close();
	}
	else
	{
		m_cClosed |= 0x80000000;
	
		dwRet = ERROR_SUCCESS;
	}

	if ( m_pLeftOver )
	{
		Assert( m_fSync );

		SNIPacketRelease(m_pLeftOver );
		m_pLeftOver = NULL;
		
	}

	a_csSessionList.Leave(); 

	a_csSmux.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD Smux::FInit()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );
	
	DWORD dwRet;

	dwRet = SNICritSec::Initialize( &m_SmuxCS );

	if(ERROR_SUCCESS != dwRet)
	{
		goto ErrorExit;
	}
	
	dwRet = SNICritSec::Initialize( &m_SessionListCS );

	if(ERROR_SUCCESS != dwRet)
	{
		goto ErrorExit;
	}

	if( m_fSync )
	{
		m_SyncWorkerMutex = CreateEvent( 0, FALSE, FALSE, 0);
		
		if(NULL == m_SyncWorkerMutex )
		{
			dwRet = GetLastError();

			SNI_SET_LAST_ERROR( SMUX_PROV, SNIE_10, dwRet );

			goto ErrorExit;
		}
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;

ErrorExit:

	if( m_SmuxCS )
	{
		DeleteCriticalSection( &m_SmuxCS );
		Assert( m_SmuxCS == NULL );
	}

	if( m_SessionListCS )
	{
		DeleteCriticalSection( &m_SessionListCS );
		Assert( m_SessionListCS == NULL );
	}

	if( m_fSync )
	{
		if( m_SyncWorkerMutex != NULL )
		{
			CloseHandle( m_SyncWorkerMutex );
			m_SyncWorkerMutex = NULL;
		}
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}


DWORD Smux::GetSessionFromId( USHORT SessionId, __out Session **ppSession )
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("SessionId: %d, ")
							  _T("ppSession: %p{Session**}\n"), 
							  GetBidId(),
							  SessionId, 
							  ppSession );
	
	CAutoSNICritSec a_csSessionList( m_SessionListCS, SNI_AUTOCS_DO_NOT_ENTER );

	a_csSessionList.Enter();

	DWORD dwRet = ERROR_SUCCESS;
	
	if( (DWORD)SessionId < m_MaxSessions )
	{
		 *ppSession = m_rgSessions[SessionId];

		if( NULL == *ppSession && m_pConn->m_fClient )
		{
			dwRet = ERROR_INVALID_DATA;
		}
	}
	else if( (DWORD)SessionId == m_MaxSessions )
	{
		*ppSession = NULL;

		if( m_pConn->m_fClient )
		{
			dwRet = ERROR_INVALID_DATA;
		}
	}
	else
	{
		*ppSession = NULL;
		
		dwRet = ERROR_INVALID_DATA;
	}

	if( ERROR_SUCCESS != dwRet )
	{
		SNI_SET_LAST_ERROR( SMUX_PROV, SNIE_10, dwRet );
	}

	a_csSessionList.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD Smux::GetWholePacket( __inout SNI_Packet **ppPacket, __out SNI_Packet **ppLeftOver)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("ppPacket: %p{SNI_Packet**}, ")
							  _T("ppLeftOver: %p{SNI_Packet**}\n"), 
							  GetBidId(),
							  ppPacket, 
							  ppLeftOver );
	
	Assert( *ppPacket && !*ppLeftOver);
	
	SNI_Packet *pPacket = *ppPacket;
	
	UNALIGNED SmuxHeader * pSmuxHeader;

	pSmuxHeader = (SmuxHeader *)SNIPacketGetBufPtr( pPacket);

	DWORD cbPacket;
	
	cbPacket = SNIPacketGetBufferSize( pPacket);

	while( cbPacket < SMUX_HEADER_SIZE || cbPacket < pSmuxHeader->Length)	//if the packet is not complete
	{
		Assert( !m_fSync );

		DWORD dwBytesToRead;
		if( cbPacket < SMUX_HEADER_SIZE )
		{
			// Currently we are not able to handle the case if an underlying provider
			// with a non-zero header is used.  Since the only other provider with
			// non-zero header size supported in Yukon is SSL, which always 
			// returns a whole packet from a valid client we reject all cases where
			// the total provider header size is different from SMUX only.  
			if( SMUX_HEADER_SIZE != m_pConn->m_ConnInfo.ProvOffset )
			{
				SNI_ASSERT_ON_INVALID_PACKET;				
				SNI_SET_LAST_ERROR( SMUX_PROV, SNIE_10, ERROR_INVALID_DATA );
				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_INVALID_DATA);	
				return ERROR_INVALID_DATA;
			}
			
			dwBytesToRead = m_pConn->m_ConnInfo.ConsBufferSize+SMUX_HEADER_SIZE-cbPacket;
		}
		else if ( pSmuxHeader->SMID != SMUX_IDENTIFIER )
		{
			SNI_ASSERT_ON_INVALID_PACKET;				
			SNI_SET_LAST_ERROR( SMUX_PROV, SNIE_10, ERROR_INVALID_DATA );
			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_INVALID_DATA);	
			return ERROR_INVALID_DATA;
		}
		else
		{
			if( pSmuxHeader->Length - SMUX_HEADER_SIZE > m_pConn->m_ConnInfo.ConsBufferSize )
			{
				SNI_ASSERT_ON_INVALID_PACKET
				
				SNI_SET_LAST_ERROR( SMUX_PROV, SNIE_10, ERROR_INVALID_DATA );

				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_INVALID_DATA);
	
				return ERROR_INVALID_DATA;
			}

			dwBytesToRead = pSmuxHeader->Length-cbPacket;
		}
		
		DWORD dwRet;

		dwRet = m_pNext->PartialReadAsync( pPacket, dwBytesToRead, NULL);
		if( dwRet == ERROR_SUCCESS)
		{
			cbPacket = SNIPacketGetBufferSize( pPacket);
		}
		else
		{
			if( dwRet == ERROR_IO_PENDING)
			{
				*ppPacket = 0;
			}

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
			return dwRet;
		}
	}

	if( pSmuxHeader->SMID != SMUX_IDENTIFIER )
	{
		SNI_ASSERT_ON_INVALID_PACKET;				
		SNI_SET_LAST_ERROR( SMUX_PROV, SNIE_10, ERROR_INVALID_DATA );
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_INVALID_DATA);	
		return ERROR_INVALID_DATA;
	}
	
	DWORD cbWholePacketLength = pSmuxHeader->Length;
	
	if( cbWholePacketLength == cbPacket )	//a complete packet
	{

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);	
		return ERROR_SUCCESS;
	}
	else
	{
		Assert( cbWholePacketLength < cbPacket );

		if( cbWholePacketLength < SMUX_HEADER_SIZE  )
		{
			SNI_ASSERT_ON_INVALID_PACKET;

			SNI_SET_LAST_ERROR( SMUX_PROV, SNIE_10, ERROR_INVALID_DATA );

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_INVALID_DATA);
	
			return ERROR_INVALID_DATA;
		}

		SNI_Packet *pLeftOver;
		
		BYTE *pbLeftOver;
		DWORD cbLeftOver;

		pbLeftOver = SNIPacketGetBufPtr( pPacket);
		pbLeftOver += cbWholePacketLength;
		cbLeftOver = SNIPacketGetBufferSize( pPacket);
		cbLeftOver -= cbWholePacketLength;
		
		pLeftOver = SNIPacketAllocate(m_pConn, SNI_Packet_Read);
		if( pLeftOver == NULL )
		{
			SNI_SET_LAST_ERROR( SMUX_PROV, SNIE_4, ERROR_OUTOFMEMORY );

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_OUTOFMEMORY);
	
			return ERROR_OUTOFMEMORY;
		}
		else
		{
			SNIPacketSetBufferSize( pPacket, cbWholePacketLength);

			SNIPacketSetData( pLeftOver, pbLeftOver, cbLeftOver);
			pLeftOver->m_OrigProv = SMUX_PROV;

			*ppLeftOver = pLeftOver;

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
			return ERROR_SUCCESS;
		}
	}

}


//---------------------------------------------------------------------
// Function: Smux::GrowSessionList
//
// Description:
//	Increase the size of the Session array by one.  
//
// Assumptions:
//	- Called while holding the m_SmuxCS and m_SessionListCS critical 
//		sections (the former because GetSessionFromIdNoCS() reads 
//		m_rgSessions without taking m_SessionListCS but while hloding 
//		m_SmuxCS, and this function deletes m_rgSessions).  
//
// Returns:
//	- ERROR_SUCCESS on success.  
//	- Error code otherwise.  
//
// Notes:
//		
DWORD Smux::GrowSessionList()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );
	
	Session **rgNewSessions;

	// Session ID's are two bytes.  
	Assert( USHRT_MAX >= m_MaxSessions ); 

	if( USHRT_MAX < m_MaxSessions )
	{
		SNI_SET_LAST_ERROR( SMUX_PROV, SNIE_4, ERROR_INVALID_STATE );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_INVALID_STATE);
	
		return ERROR_INVALID_STATE;
	}
	
	rgNewSessions = NewNoX(gpmo) Session * [m_MaxSessions+1];
	if( !rgNewSessions )
	{
		SNI_SET_LAST_ERROR( SMUX_PROV, SNIE_4, ERROR_OUTOFMEMORY );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_OUTOFMEMORY);
	
		return ERROR_OUTOFMEMORY;
	}
	
	memcpy(rgNewSessions, m_rgSessions, m_MaxSessions*sizeof(Session *));
	rgNewSessions[m_MaxSessions++]=0;
	delete [] m_rgSessions;

	m_rgSessions = rgNewSessions;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;
}

DWORD Smux::Initialize( PSNI_PROVIDER_INFO pInfo )
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("pInfo: %p{PSNI_PROVIDER_INFO}\n"), pInfo );
	
	Assert( sizeof(SmuxHeader) == SMUX_HEADER_SIZE );
	pInfo->ProvNum = SMUX_PROV;
	pInfo->Offset = SMUX_HEADER_SIZE;
	pInfo->fBaseProv = FALSE;
	pInfo->Size = SMUX_HEADER_SIZE;
	pInfo->fInitialized = TRUE; 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;
}

void Smux::InternalClose()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );
	
	CAutoSNICritSec a_csSmux( m_SmuxCS, SNI_AUTOCS_DO_NOT_ENTER );

	a_csSmux.Enter();

	CAutoSNICritSec a_csSessionList( m_SessionListCS, SNI_AUTOCS_DO_NOT_ENTER );

	a_csSessionList.Enter();

	if( !m_cClosed )
	{
		//
		// terminate sessions before close to make sure they won't try to access lower layer after close
		//

		TerminateSessions();

		DWORD dwRet;

		dwRet = m_pNext->Close();

		Assert( ERROR_SUCCESS == dwRet );
	}

	++m_cClosed;
	
	a_csSessionList.Leave(); 

	a_csSmux.Leave(); 
}

DWORD Smux::Demultiplex( __inout SNI_Packet **ppLeftOver, __out SNI_Packet ** ppNewPacket, USHORT usForSessionId )
{
	BidxScopeAutoSNI4( SNIAPI_TAG _T("%u#, ")
							  _T("ppLeftOver: %p{SNI_Packet**}, ")
							  _T("ppNewPacket: %p{SNI_Packet**}, ")
							  _T("usForSessionId: %d\n"), 
							  GetBidId(),
							  ppLeftOver, 
							  ppNewPacket, 
							  usForSessionId );

	DWORD dwRet;

	UNALIGNED SmuxHeader *pSmuxHeader;

	pSmuxHeader = (SmuxHeader *)SNIPacketGetBufPtr( *ppLeftOver);

	DWORD cbPacket;

	cbPacket = SNIPacketGetBufferSize( *ppLeftOver );

	// This function should only be called by the client
	Assert( m_pConn->m_fClient );
	
	if( cbPacket < SMUX_HEADER_SIZE)	//if the packet is less than smux header try to read more
	{
		// Currently we are not able to handle the case if an underlying provider
		// with a non-zero header is used.  Since the only other provider with
		// non-zero header size supported in Yukon is SSL, which always 
		// returns a whole packet from a valid client we reject all cases where
		// the total provider header size is different from SMUX only.  
		if( SMUX_HEADER_SIZE != m_pConn->m_ConnInfo.ProvOffset )
		{
			SNI_ASSERT_ON_INVALID_PACKET;
			dwRet = ERROR_INVALID_DATA;			
			SNI_SET_LAST_ERROR( SMUX_PROV, SNIE_10, dwRet );
			goto ExitFunc;
		}
			
		m_dwBytesToRead = m_pConn->m_ConnInfo.ConsBufferSize+SMUX_HEADER_SIZE-cbPacket;
	}
	else if( pSmuxHeader->SMID != SMUX_IDENTIFIER )  // check to make sure the ID is correct
	{
		SNI_ASSERT_ON_INVALID_PACKET;
		dwRet = ERROR_INVALID_DATA;			
		SNI_SET_LAST_ERROR( SMUX_PROV, SNIE_10, dwRet );
		goto ExitFunc;
	}	
	else if( cbPacket < pSmuxHeader->Length )	//we got the header but still not a complete packet
	{
		if( pSmuxHeader->Length - SMUX_HEADER_SIZE > m_pConn->m_ConnInfo.ConsBufferSize )
		{
			SNI_ASSERT_ON_INVALID_PACKET;
			dwRet = ERROR_INVALID_DATA;
			
			SNI_SET_LAST_ERROR( SMUX_PROV, SNIE_10, dwRet );

			goto ExitFunc;
		}
		
		m_dwBytesToRead = pSmuxHeader->Length-cbPacket;
	}
	else	//a full packet or more than one packet
	{
		Assert( pSmuxHeader->Length <= cbPacket );

		m_dwBytesToRead = 0;

		SNI_Packet *pPacket;
		
		pPacket = *ppLeftOver;
		*ppLeftOver = NULL;

		if( pSmuxHeader->Length != cbPacket )	//more than one complete packet
		{
			Assert( pSmuxHeader->Length < cbPacket );

			dwRet = GetWholePacket(  &pPacket, ppLeftOver);

			Assert( pPacket );
			
			if( dwRet != ERROR_SUCCESS )
			{
				Assert( *ppLeftOver == NULL );
				*ppLeftOver = pPacket;
				pPacket = 0;
				Assert( dwRet != ERROR_IO_PENDING );
				goto ExitFunc;
			}

		}

		SNI_BID_TRACE_SMUX_HEADER( _T( "Received:\n" ), pSmuxHeader ); 

		//	Save off the session Id since we may use it after the packet
		//	was released.  
		//
		USHORT usSessionId = pSmuxHeader->SessionId; 
		
		//	First try obtaining the Session pointer without entering
		//	the critical section for the session list.  
		//
		Session *pSession = GetSessionFromIdNoCS( usSessionId );

		if( NULL == pSession )
		{		
			dwRet = GetSessionFromId( usSessionId, &pSession);

			// Treat both GetSessionFromId error and no existing session and on server (indicated by ERROR_SUCCESS 
			// and NULL pSession) as an error.
			if(dwRet != ERROR_SUCCESS || NULL == pSession)
			{
				SNI_ASSERT_ON_INVALID_PACKET;
				SNIPacketRelease( pPacket );
				pPacket = 0;

				goto ExitFunc;
			}
		}


		SNI_Packet *pTmp;
		dwRet = pSession->ReadDone( &pPacket, &pTmp, pSmuxHeader->Length, ERROR_SUCCESS );
		Assert( !pTmp);

		if( ERROR_SUCCESS != dwRet)
		{
			if( pPacket )
				SNIPacketRelease( pPacket );
			
			goto ExitFunc;
		}

		//	If the caller requested the packet, and we got a packet for the requested
		//	session, return it back.  
		//
		if( ( NULL != ppNewPacket ) && ( usForSessionId == usSessionId ) )
		{
			*ppNewPacket = pPacket;
		}
		else
		{
			Assert( !pPacket );
		}
	}
	
	dwRet = ERROR_SUCCESS;

ExitFunc:

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD Smux::Open( __out SNI_Conn *pConn, 
					__in SNI_Conn * pParent,
					__out SNI_Provider ** ppProv)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T( "pConn: %p{SNI_Conn*}, pParent: %p{SNI_Conn*}, ppProv: %p{SNI_Provider**}\n"), 
					pConn, pParent, ppProv);
	
	pConn->m_fSync = pParent->m_fSync;

	Assert( pParent->m_pProvHead->m_Prot == SMUX_PROV );
	DWORD dwRet = ((Smux *)pParent->m_pProvHead)->OpenSession( pConn, ppProv);

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;
}

DWORD  Smux::OpenSession(__out SNI_Conn *pConn, __out SNI_Provider ** ppProv)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pConn: %p{SNI_Conn*}, ")
							  _T("ppProv: %p{SNI_Provider**}\n"), 
							  GetBidId(),
							  pConn, 
							  ppProv);

	//	Take m_SmuxCS because GrowSessionList() requires holding both 
	//	m_SmuxCS and m_SessionListCS, and m_SmuxCS shall be taken before 
	//	m_SessionListCS.  
	//
	CAutoSNICritSec a_csSmux( m_SmuxCS, SNI_AUTOCS_DO_NOT_ENTER );

	a_csSmux.Enter();

	CAutoSNICritSec a_csSessionList( m_SessionListCS, SNI_AUTOCS_DO_NOT_ENTER );

	a_csSessionList.Enter();

	DWORD dwRet;

	if( m_cClosed )
	{
		dwRet = ERROR_FAIL;

		SNI_SET_LAST_ERROR( SMUX_PROV, SNIE_19, dwRet );

		goto ErrorExit;
	}
	
	Assert(m_nSessions<=m_MaxSessions);
	
	if(m_nSessions==m_MaxSessions)	//if we don't have extra room for a new session then allocate
	{
		dwRet = GrowSessionList();
		if( dwRet != ERROR_SUCCESS )
		{
			goto ErrorExit;
		}
	}
	
	USHORT SessionId = (USHORT) m_MaxSessions;
	
	for ( DWORD i=0; i<m_MaxSessions; i++)
	{
		if( !m_rgSessions[i] )	//pick the first unused session number
		{
			SessionId = (USHORT)i;
			break;
		}
	}
	
	Assert( SessionId < m_MaxSessions );
	
	Session *pSession;

	pSession = NewNoX(gpmo) Session(pConn, m_pNext, SessionId, this );
	if( !pSession )
	{
		dwRet = ERROR_OUTOFMEMORY;

		SNI_SET_LAST_ERROR( SMUX_PROV, SNIE_4, dwRet );

		goto ErrorExit;
	}

	dwRet = pSession->FInit();
	if(ERROR_SUCCESS != dwRet )
	{
		delete pSession;

		goto ErrorExit;
	}

	m_rgSessions[SessionId] = pSession;
	pConn->m_pProvHead = pSession;	//need to set this otherwise SYN write completion will be a problem
	m_nSessions++;

	dwRet = pSession->Open();
	if( ERROR_SUCCESS != dwRet )
	{
		pConn->m_pProvHead = NULL;

		RemoveSessionId( SessionId );

		pSession->Release();

		goto ErrorExit;
	}

	*ppProv = pSession;

	a_csSessionList.Leave(); 

	a_csSmux.Leave();

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

	return ERROR_SUCCESS;

ErrorExit:

	*ppProv = 0;
	
	a_csSessionList.Leave(); 

	a_csSmux.Leave();

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;
}

DWORD Smux::PostReadAsync(__inout SNI_Packet **ppPacket)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("%u#, ")
							  _T("ppPacket: %p{SNI_Packet**}\n"), 
							  GetBidId(),
							  ppPacket);
	
	DWORD dwRet;
	
	dwRet = m_pNext->ReadAsync( ppPacket, 0);

	if( dwRet == ERROR_SUCCESS)
	{
		(*ppPacket)->m_OrigProv = SMUX_PROV;
	}
	else if( dwRet == ERROR_IO_PENDING)
	{
		Assert( !*ppPacket);
		dwRet = ERROR_SUCCESS;
	}
	else
	{
		Assert( !*ppPacket);
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;
}

DWORD Smux::ReadAsync(__inout SNI_Packet ** ppNewPacket, LPVOID pPacketKey)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("ppNewPacket: %p{SNI_Packet**}, ")
							  _T("pPacketKey: %p\n"), 
							  GetBidId(),
							  ppNewPacket, 
							  pPacketKey);
	
	DWORD dwRet;

	Assert( !m_fSync );
	
	CAutoSNICritSec a_csSmux( m_SmuxCS, SNI_AUTOCS_DO_NOT_ENTER );

	a_csSmux.Enter();

	Assert( !m_cClosed );
	Assert( m_nSessions==0);

	Assert( !m_pPacketKeyHolder );

	m_pPacketKeyHolder = SNIPacketAllocate( m_pConn, SNI_Packet_KeyHolderNoBuf );

	if( NULL == m_pPacketKeyHolder )
	{
		dwRet = ERROR_OUTOFMEMORY;
		
		SNI_SET_LAST_ERROR( SMUX_PROV, SNIE_4, dwRet );

		goto ExitFunc;
	}
	
	SNIPacketSetKey( m_pPacketKeyHolder, pPacketKey );

	dwRet = m_pNext->ReadAsync( ppNewPacket, 0);
	if( ERROR_SUCCESS == dwRet )
	{
		//We cannot return a packet from this function, it needs to go through ReadDone
		DWORD dwBytes;

		dwBytes = SNIPacketGetBufferSize( *ppNewPacket);

		SNIPacketSetBufferSize( *ppNewPacket, 0);

		(*ppNewPacket)->m_OrigProv = SMUX_PROV;
		
		if( ERROR_SUCCESS != SNIPacketPostQCS(*ppNewPacket, dwBytes))
		{
			//this assertion is used to catch unexpected system call failure.
			Assert( 0 && "SNIPacketPostQCS failed\n" );
			BidTrace0( ERROR_TAG _T("SNIPacketPostQCS failed\n") );
		}

		*ppNewPacket = 0;

		dwRet = ERROR_IO_PENDING;
	}
	else if( ERROR_IO_PENDING != dwRet )
	{
		SNIPacketRelease( m_pPacketKeyHolder );

		m_pPacketKeyHolder = NULL;
	}
	
ExitFunc:
	
	Assert( !*ppNewPacket );

	a_csSmux.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;
}

DWORD Smux::ReadDone( __inout SNI_Packet ** ppPacket, 
					  __inout SNI_Packet ** ppLeftOver, 
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
	
	Assert( !m_fSync );

	Session    * pSession = 0;

	Assert( NULL == *ppLeftOver );
	
	CAutoSNICritSec a_csSmux( m_SmuxCS, SNI_AUTOCS_DO_NOT_ENTER );

	a_csSmux.Enter();

	DWORD dwRet;

	dwRet = ReadDoneChainCall( ppPacket, ppLeftOver, dwBytes, dwError);

	if( dwRet != ERROR_SUCCESS )
	{
		Assert( *ppPacket );

		goto ExitFunc;
	}

	if( !*ppPacket )
	{
		// *ppPacket should be consumed only for success case
		Assert( dwRet == ERROR_SUCCESS);
		goto ExitFunc;
	}

	if( m_cClosed )
	{
		dwRet = ERROR_FAIL;

		SNI_SET_LAST_ERROR( SMUX_PROV, SNIE_20, dwRet );

		goto ExitFunc;
	}

	dwRet= GetWholePacket( ppPacket, ppLeftOver );
	if( ERROR_SUCCESS != dwRet )
	{
		if( dwRet == ERROR_IO_PENDING )
		{
			Assert( *ppPacket==NULL );
			dwRet = ERROR_SUCCESS;
		}

		Assert( !*ppLeftOver );
		goto ExitFunc;
	}

	UNALIGNED SmuxHeader * pSmuxHeader;

	pSmuxHeader = (SmuxHeader *)SNIPacketGetBufPtr( *ppPacket);

	SNI_BID_TRACE_SMUX_HEADER( _T( "Received:\n" ), pSmuxHeader ); 

	dwRet = GetSessionFromId( pSmuxHeader->SessionId, &pSession );

	if( ERROR_SUCCESS != dwRet )
	{
		if( *ppLeftOver )
		{
			SNIPacketRelease( *ppLeftOver );
			*ppLeftOver = 0;
		}
		goto ExitFunc;
	}
	
	if( pSession != NULL )	//an existing session
	{
		Assert( SNIPacketGetBufferSize( *ppPacket) == pSmuxHeader->Length );

		SNI_Packet *pLeftOver;
		dwRet = pSession->ReadDone( ppPacket, &pLeftOver, pSmuxHeader->Length, ERROR_SUCCESS );
		Assert( !pLeftOver && dwRet!=ERROR_IO_PENDING);

		if( ERROR_SUCCESS != dwRet )
		{
			if( *ppLeftOver )
			{
				SNIPacketRelease( *ppLeftOver );

				*ppLeftOver = 0;
			}

			Assert( *ppPacket );
			goto ExitFunc;
		}
		
		if( *ppLeftOver == NULL )
		{
			dwRet = PostReadAsync( ppLeftOver );
			if( dwRet != ERROR_SUCCESS )
			{
				Assert( 0 == SNIPacketGetBufferSize( m_pPacketKeyHolder ) );
				
				m_pPacketKeyHolder->m_OrigProv = SMUX_PROV;

				*ppLeftOver = m_pPacketKeyHolder;

				dwRet = ERROR_SUCCESS;
			}
		}

		a_csSmux.Leave(); 

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

		return dwRet;
	}
	else		//must be a new session request
	{
		if( pSmuxHeader->Flags != SMUX_SYN )
		{
			SNI_ASSERT_ON_INVALID_PACKET;
			
			if( *ppLeftOver )
			{
				SNIPacketRelease( *ppLeftOver );

				*ppLeftOver = 0;
			}

			dwRet = ERROR_INVALID_DATA;

			SNI_SET_LAST_ERROR( SMUX_PROV, SNIE_10, dwRet );

			goto ExitFunc;
		}

		SNI_Conn *pConn;
		
		dwRet = AcceptConnection( &pConn, pSmuxHeader->SessionId );

		if( dwRet != ERROR_SUCCESS )
		{
			if( *ppLeftOver )
			{
				SNIPacketRelease( *ppLeftOver );

				*ppLeftOver = 0;
			}

			goto ExitFunc;
		}
		
		if( *ppLeftOver == NULL )
		{
			dwRet = PostReadAsync( ppLeftOver );

			if( dwRet != ERROR_SUCCESS )
			{
				Assert( dwRet != ERROR_IO_PENDING );

				SNIClose( pConn );

				goto ExitFunc;
			}
		}

		SNIPacketRelease( *ppPacket );

		*ppPacket = 0;

		// Callback the consumer on the Accept callback
		pConn->m_ConsumerInfo.fnAcceptComp(pConn, m_pConn->m_ConsKey);

		a_csSmux.Leave(); 

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

		return ERROR_SUCCESS;
	}

ExitFunc:

	if( *ppPacket )
	{
		Assert( dwRet != ERROR_SUCCESS && *ppPacket && !*ppLeftOver);
		Assert( SNIPacketGetConnection( *ppPacket)==m_pConn );

		if( *ppPacket != m_pPacketKeyHolder )
		{
			SNIPacketSetKey( *ppPacket, SNIPacketGetKey( m_pPacketKeyHolder ) );
			SNIPacketRelease( m_pPacketKeyHolder );
		}
		
		m_pPacketKeyHolder = NULL;

		//
		// if something goes wrong with physical connection we close it and let the sessions know
		//

		InternalClose();
	}
	else
	{
		Assert( dwRet == ERROR_SUCCESS );
	}
	
	a_csSmux.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;
}

DWORD Smux::ReadDoneChainCall( __inout SNI_Packet ** ppPacket, 
					  __inout SNI_Packet ** ppLeftOver, 
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
	
	DWORD dwRet;
	
	if( (*ppPacket)->m_OrigProv!=SMUX_PROV )
	{
		dwRet = m_pNext->ReadDone( ppPacket, ppLeftOver, dwBytes, dwError );
		Assert( !*ppLeftOver && dwRet!=ERROR_IO_PENDING && (*ppPacket||dwRet==ERROR_SUCCESS));
	}
	else
	{
		(*ppPacket)->m_OrigProv = INVALID_PROV;
		
		Assert( dwError == ERROR_SUCCESS );
		
		if( 0 == dwBytes )
		{
			dwRet = ERROR_FAIL;

			SNI_SET_LAST_ERROR( SMUX_PROV, SNIE_19, dwRet );
		}
		else
		{
			dwRet = ERROR_SUCCESS;
		}
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;
}

DWORD Smux::ReadSync(__out SNI_Packet ** ppNewPacket, int timeout)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("ppNewPacket: %p{SNI_Packet**}, ")
							  _T("timeout: %d\n"), 
							  GetBidId(),
							  ppNewPacket, 
							  timeout);
	
	Assert( m_fSync );
	
	*ppNewPacket = NULL;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

	return ERROR_SUCCESS;
}

Smux::Smux(SNI_Conn *pConn):SNI_Provider( pConn )
{
	m_Prot = SMUX_PROV;
	
	m_SmuxCS = NULL;

	m_SessionListCS = NULL;

	m_nSessions = 0;
	m_MaxSessions=0;
	m_rgSessions = NULL;

	m_cClosed = 0;
	
	m_pPacketKeyHolder = NULL;

	m_fSync = pConn->m_fSync;

	m_pLeftOver = 0;
	m_dwBytesToRead = 0;

	m_SyncWorkerMutex = NULL;
	
	m_SyncWorkAccessCount = 0;

	BidObtainItemID2A( &m_iBidId, SNI_ID_TAG "%p{.} created by %u#{SNI_Conn}", 
		this, pConn->GetBidId() );
}

void Smux::Release()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T( "\n") );
	
	m_pNext->Release();

	CleanUp();

	delete this;
}

void Smux::RemoveSessionId( USHORT Id )
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("%u#, ")
							  _T("Id: %d\n"), 
							  GetBidId(),
							  Id );
	
	CAutoSNICritSec a_csSessionList( m_SessionListCS, SNI_AUTOCS_DO_NOT_ENTER );

	a_csSessionList.Enter();

	Assert( (Id < m_MaxSessions) && m_rgSessions[ Id ] );

	// A bad client could make us remove the session twice 
	// by sending two FIN packets.  

	if( m_rgSessions[ Id ] )
	{
		//remove from list
		m_rgSessions[ Id ] = 0;
		m_nSessions--;
	}

	a_csSessionList.Leave(); 
}

void Smux::RemoveSessionIdWithCtrlPkt( USHORT Id, BYTE Flag  )
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("%u#, ")
							  _T( "Id: %d\n"), 
							  GetBidId(),
							  Id );
	
	CAutoSNICritSec a_csSessionList(m_SessionListCS, SNI_AUTOCS_DO_NOT_ENTER );

	a_csSessionList.Enter();

	Assert( (Id < m_MaxSessions) && m_rgSessions[ Id ] );

	// A bad client could make us remove the session twice 
	// by sending two FIN packets.  

	Session *pSession = m_rgSessions[ Id ]; 

	if( pSession )
	{
		//remove from list
		m_rgSessions[ Id ] = 0;
		m_nSessions--;

		// Send a FIN packet on the session if the connection
		// is good.  
		pSession->SendControlPacketIfGoodConn( Flag ); 
	}

	a_csSessionList.Leave();
}

void Smux::ReleaseSessionRef()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );
	
	m_pConn->Release( REF_InternalActive );
}

void Smux::TerminateSessions()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );
	
	CAutoSNICritSec a_csSessionList( m_SessionListCS, SNI_AUTOCS_DO_NOT_ENTER );

	a_csSessionList.Enter();

	for( DWORD i=0;i<m_MaxSessions;i++)
	{
		if( !m_rgSessions[i])
		{
			continue;
		}
		
		m_rgSessions[i]->SetBadConnection();
	}

	a_csSessionList.Leave(); 
}

DWORD Smux::Worker( int timeout, __out SNI_Packet ** ppNewPacket, USHORT usForSessionId, bool fPollingForStatus )
{
	BidxScopeAutoSNI5( SNIAPI_TAG _T("%u#, ")
							  _T("timeout: %d, ")
							  _T("ppNewPacket: %p{SNI_Packet**}, ")
							  _T("usForSessionId: %d, ")
							  _T("fPollingForStatus: %d{bool}\n"), 
							  GetBidId(),
							  timeout, 
							  ppNewPacket, 
							  usForSessionId,
							  fPollingForStatus );

	// If the caller wants to poll for status, it should pass a 0 timeout, so we don't wait.
	Assert(!fPollingForStatus || 0 == timeout);

	CAutoSNICritSec a_csSmux( m_SmuxCS, SNI_AUTOCS_DO_NOT_ENTER );

	a_csSmux.Enter();

	DWORD dwRet = ERROR_SUCCESS;

	if( m_cClosed )
	{
		dwRet = ERROR_FAIL;

		SNI_SET_LAST_ERROR( SMUX_PROV, SNIE_20, dwRet );

		goto ExitFunc;
	}

	if( m_pLeftOver )
	{
		//
		// if LeftOver packet has been parsed we know how much more to read
		// otherwise we should pass it to Demultiplex() so that it will parse it
		//
		
		if( fPollingForStatus )
		{
			dwRet = ERROR_SUCCESS;
		}
		else
		{
			if( m_dwBytesToRead )
				dwRet = m_pNext->PartialReadSync( m_pLeftOver, m_dwBytesToRead, timeout);
			else
				dwRet = ERROR_SUCCESS;
		}
	}
	else
		dwRet = m_pNext->ReadSync( &m_pLeftOver, timeout );

	// If we were just checking status, don't try to Demultiplex any packet. We'll deal with it
	// when a real read is posted.
	if( dwRet == ERROR_SUCCESS && !fPollingForStatus )
	{
		dwRet = Demultiplex( &m_pLeftOver, ppNewPacket, usForSessionId );

		Assert( dwRet != WAIT_TIMEOUT );
	}
	
	if( dwRet != ERROR_SUCCESS && dwRet != WAIT_TIMEOUT )
	{
		//
		// if something goes wrong with physical connection we close it and let the sessions know
		//

		InternalClose();
	}
	
	if( WAIT_TIMEOUT == dwRet && fPollingForStatus )
	{
		dwRet = ERROR_SUCCESS;
	}
	
ExitFunc:
	
	a_csSmux.Leave(); 
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;

}

DWORD Smux::WriteAsync(SNI_Packet * pPacket, SNI_ProvInfo * pProvInfo)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("pProvInfo: %p{SNI_ProvInfo*}\n"), 
							  GetBidId(),
							  pPacket, 
							  pProvInfo);

	Assert( !m_fSync );

	Assert( 0 && "This function is not implemented\n" );
	BidTrace0( ERROR_TAG _T("This function is not implemented\n") );
	
	SNI_SET_LAST_ERROR( SMUX_PROV, SNIE_15, ERROR_FAIL );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_FAIL);

	return ERROR_FAIL;
}

DWORD Smux::WriteDone(SNI_Packet ** ppPacket, DWORD dwBytes, DWORD dwError)
{
	BidxScopeAutoSNI4( SNIAPI_TAG _T("%u#, ")
							  _T("ppPacket: %p{SNI_Packet**}, ")
							  _T("dwBytes: %d, ")
							  _T("dwError: %d{WINERR}\n"), 
							  GetBidId(),
							  ppPacket, 
							  dwBytes, 
							  dwError);

	Assert( !m_fSync );

	//WriteDone should be called for Session
	
	Assert( 0 && "This function is not implemented\n" );
	BidTrace0( ERROR_TAG _T("This function is not implemented\n") );

	SNI_SET_LAST_ERROR( SMUX_PROV, SNIE_15, ERROR_FAIL );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_FAIL);

	return ERROR_FAIL;
}

DWORD Smux::WriteSync(SNI_Packet * pPacket, SNI_ProvInfo * pProvInfo)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("pProvInfo: %p{SNI_ProvInfo*}\n"), 
							  GetBidId(),
							  pPacket, 
							  pProvInfo);

	Assert( m_fSync );

	Assert( 0 && "This function is not implemented\n" );
	BidTrace0( ERROR_TAG _T("This function is not implemented\n") );

	SNI_SET_LAST_ERROR( SMUX_PROV, SNIE_15, ERROR_FAIL );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_FAIL);

	return ERROR_FAIL;

}

