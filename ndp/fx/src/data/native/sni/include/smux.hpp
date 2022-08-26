//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: smux.hpp
// @Owner: petergv, nantu
// @Test: sapnaj
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

#ifndef _SMUX_HPP_
#define _SMUX_HPP_

class Session;

class Smux : public SNI_Provider
{
public:
	static DWORD Initialize( PSNI_PROVIDER_INFO pInfo );
	static DWORD Open( __out SNI_Conn * pConn,
						 __in SNI_Conn * pParent,
						 __out OUT SNI_Provider ** ppProv);

	DWORD ReadSync(__out SNI_Packet ** ppNewPacket, int timeout);
	DWORD ReadAsync(__inout SNI_Packet ** ppNewPacket, LPVOID pPacketKey);
	DWORD WriteSync(SNI_Packet * pPacket, SNI_ProvInfo * pProvInfo);
	DWORD WriteAsync(SNI_Packet * pPacket, SNI_ProvInfo * pProvInfo);
	DWORD ReadDone(__inout SNI_Packet ** ppPacket, __inout SNI_Packet **ppLeftOver, DWORD dwBytes, DWORD dwError);
	DWORD Worker( int timeout, __out SNI_Packet ** ppNewPacket, USHORT usForSessionId, bool fPollingForStatus ); 

	DWORD WriteDone(SNI_Packet ** ppPacket, DWORD dwBytes, DWORD dwError);	
	DWORD Close();
	void Release();
	void AddSessionRef();
	void ReleaseSessionRef();

private:

	SNICritSec *m_SmuxCS;	//Critical section to protect multiple thread access

	SNICritSec *m_SessionListCS;	//Critical section to protect session list

	DWORD 	m_nSessions;		//Number of sessions
	DWORD	m_MaxSessions;	//Number of sessions that we allocated space for
	Session ** m_rgSessions;	//pointer to start of Sessions array


	DWORD m_cClosed;

	SNI_Packet *	m_pPacketKeyHolder;
	LPVOID			m_StoredPacketKey;

	//variables below are sync specific
	bool		m_fSync;

	SNI_Packet *m_pLeftOver;
	DWORD m_dwBytesToRead;

	HANDLE m_SyncWorkerMutex;

	//Count concurrent threads willing to access worker function.
	LONG m_SyncWorkAccessCount;

public:

	Smux(SNI_Conn *pConn);
	DWORD FInit();
	
	~Smux();
	
	DWORD OpenSession(__out SNI_Conn *pConn, __out SNI_Provider **ppProv);

	inline HANDLE GetMutexHandle(){ return m_SyncWorkerMutex; }

	inline LONG IncrementSyncAccessCounter(){ return InterlockedIncrement(&m_SyncWorkAccessCount); }

	inline LONG DecrementSyncAccessCounter(){ return InterlockedDecrement(&m_SyncWorkAccessCount); }

	void RemoveSessionId( USHORT Id );

	void RemoveSessionIdWithCtrlPkt( USHORT Id, BYTE Flag  ); 

	void InternalClose();

private:

	void CleanUp();

	DWORD GrowSessionList();
	DWORD GetSessionFromId( USHORT SessionId, __out Session **ppSession );
	DWORD GetWholePacket( __inout SNI_Packet **ppPacket, __out SNI_Packet **ppLeftOver);

	DWORD ReadDoneChainCall( __inout SNI_Packet ** ppPacket, 
					  __inout SNI_Packet ** ppLeftOver, 
					  DWORD         dwBytes, 
					  DWORD         dwError );

	DWORD AcceptConnection( __out SNI_Conn **ppConn, USHORT SessionId );

	DWORD AcceptSession( __inout SNI_Conn *pConn, USHORT SessionId);

	DWORD Demultiplex( __inout SNI_Packet **ppLeftOver, __out SNI_Packet ** ppNewPacket, USHORT usForSessionId );

	DWORD PostReadAsync(__inout SNI_Packet **ppPacket);

	void TerminateSessions();

	//---------------------------------------------------------------------
	// Function: Smux::GetSessionFromIdNoCS
	//
	// Description:
	//	Try to get the session without entering a critical section.  
	//
	//	SessionId	= [IN] Identifier of the session of interest.  
	//
	// Assumptions:
	//	- Called while holding the m_SmuxCS critical section.  
	//	- m_MaxSessions never decreases.  
	//	- Any deletion of m_rgSessions is protected by the m_SmuxCS 
	//		critical section.  
	//
	// Returns:
	//	- The pointer to the Session object if successfully retrieved.  
	//	- NULL otehrwise. This means that the caller shall try 
	//		obtaining it by entering the session list critical section.  
	//
	// Notes:
	//	No BID tracing to enable inlining for performance reasons.  
	//		
	inline Session * GetSessionFromIdNoCS( USHORT SessionId )
	{
		//	Since m_MaxSessions is only incremented, never decremented,
		//	if '(DWORD)SessionId < m_MaxSessions' is true without (/before)
		//	entering m_SessionListCS it would hold true even after entering
		//	m_SessionListCS.  
		//
		if( (DWORD)SessionId < m_MaxSessions )
		{
			//	The assumptions here is that any deletion of m_rgSessions is 
			//	protected by the m_SmuxCS critical section, and this 
			//	function is called while holding m_SmuxCS.  
			//
			//	Note that m_rgSessions[SessionId] may become NULL after we
			//	retrieve it here but the same thing can happen if calling
			//	GetSessionFromId() after it left the m_SessionListCS
			//	critical section.  
			//	
			return m_rgSessions[SessionId];
		}
		else 
		{
			return NULL; 
		}
	}
};

#endif

