//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: Np.cpp
// @Owner: petergv, nantu
// @Test: milu
//
// <owner current="true" primary="true">petergv</owner>
// <owner current="true" primary="false">nantu</owner>
//
// Purpose: SNI Named-Pipe Provider
//
// Notes:
//          
// @EndHeader@
//****************************************************************************

#ifndef _NP_HPP_
#define _NP_HPP_

class NpSecurityInfo
{
public:
	SECURITY_ATTRIBUTES SecAttr;
	SECURITY_DESCRIPTOR SecDesc;

	PACL pAcl;
	PSID pWorldSid;

	NpSecurityInfo() : pAcl(0), pWorldSid(0)
		{}

	~NpSecurityInfo()
	{
		if( pWorldSid )
			FreeSid(pWorldSid);

		if( pAcl )
			delete [] reinterpret_cast<BYTE *>(pAcl); 
	}
};

typedef struct
{
	HANDLE       hPipe;
	HANDLE       hHolderPipe;
	bool		 fLocal;
	bool		 fPendingAccept;
	bool		 fTerminated;
	WCHAR         wszPipeName[MAX_PATH+1];	// Named-Pipe name...
	NpSecurityInfo * pSecInfo;
	ProviderNum	 prot;
	DWORD        dwNetworkSize;
	DWORD        dwWaitTime;
	DWORD		 dwPipeMode; 
	LPOVERLAPPED pOverlapped;
	LONG		 cRef;
	SNICritSec * CSTerminateListener;	
} NpAcceptStruct;

class Np : public SNI_Provider
{
private:	
	HANDLE     m_hPipe;

	SNICritSec *m_CS;

	// Critical section protecting the pipe handle while it is being closed.  
	//
	SNICritSec *m_CSClose; 

	bool 			m_fWritePending;

	BOOL			m_fClose; 

	DWORD			m_dwcHandleRef; 
	
	DynamicQueue 	m_WritePacketQueue;

	//	Provider number for setting SNI last error.  
	//	Used to dictinguish between true (remote) Named Pipe
	//	and Local Access Named Pipes ("Shared Memory").  
	//
	ProviderNum m_ProtToReport;

	SNI_Packet * m_pSyncReadPacket;

public:
	Np( SNI_Conn * pConn, ProviderNum protToReport = NP_PROV );
	
	~Np();	

	static DWORD Initialize( PSNI_PROVIDER_INFO pInfo );
	
	static DWORD Terminate();
	
	static DWORD InitializeListener( HANDLE   hSNIAcceptKey, 
									 ProviderNum prot,
									 __in_opt NpListenInfo * pNpListenInfo, 
									 __out HANDLE * pListenHandle );
	
	static DWORD InitializeListener( HANDLE   hSNIAcceptKey, 
									 __in_opt NpListenInfo * pNpListenInfo, 
									 __out HANDLE * pListenHandle )
	{
		return InitializeListener(hSNIAcceptKey, NP_PROV, pNpListenInfo, pListenHandle);
	}

	static DWORD TerminateListener( __inout HANDLE hListener );

	static DWORD ResumePendingAccepts( __inout HANDLE hListener );

	static DWORD PrepareForNextAccept( __inout NpAcceptStruct *pAcc );

	static DWORD Open( 	SNI_Conn 		* pConn,
							__in ProtElem 		* pProtElem, 
							__out SNI_Provider 	** ppProv );

	DWORD CheckConnection( );
	
	DWORD ReadSync( __out SNI_Packet ** ppNewPacket, int timeout );

	DWORD ReadAsync( __out SNI_Packet ** ppNewPacket, 
		             LPVOID pPacketKey );

	DWORD PartialReadSync(SNI_Packet * pPacket,
				DWORD cbBytesToRead,
				int timeout);
	
	DWORD PartialReadAsync(SNI_Packet * pPacket,
				DWORD cbBytesToRead,
				SNI_ProvInfo * pProvInfo );
	
	DWORD WriteSync( SNI_Packet * pPacket, 
		              SNI_ProvInfo * pProvInfo );

	DWORD WriteAsync( SNI_Packet * pPacket, 
		              SNI_ProvInfo * pProvInfo );
	
	DWORD ReadDone( __deref_inout SNI_Packet ** ppPacket, 
		            __deref_out SNI_Packet ** ppLeftOver, 
		            __in DWORD         dwBytes, 
		            __in DWORD         dwError );
	
	DWORD WriteDone( SNI_Packet ** ppPacket, 
		             DWORD         dwBytes, 
		             DWORD         dwError );	
	
	DWORD Close();
	
	static DWORD AcceptDone( __in SNI_Conn * pConn, 
								 __inout LPVOID pAcceptKey,
								 __in DWORD dwBytes,
								 __in DWORD dwError,
						   		 __deref_out SNI_Provider ** ppProv,
						   		 __deref_out LPVOID * ppAcceptInfo );
	static DWORD AsyncAccept( __inout NpAcceptStruct * pAcc );

	static DWORD RepostAccept( __inout NpAcceptStruct * pAcc ); 
	
	void Release();

	DWORD QueryImpersonation();
	DWORD ImpersonateChannel();
	DWORD RevertImpersonation();

private:

	// Inform the OS not to enqueue IO Completions on successful
	// overlapped reads/writes.
	//
	// Returns ERROR_SUCCESS if OS call succeeds
	// Returns a Windows error code otherwise.
	DWORD DWSetSkipCompletionPortOnSuccess();
	
	// Sets parameters on the SNI_Packet to allow it to be used for a sync Overlapped operation.
	void PrepareForSyncCall(SNI_Packet *pPacket);
	
	// Sets parameters on the SNI_Packet to allow it to be used for an Async Overlapped operation.
	void PrepareForAsyncCall(SNI_Packet *pPacket);
	
	static Np * AcceptConnection( SNI_Conn *pConn, __inout NpAcceptStruct *pAcc);

	void CallbackError();
	
	DWORD FInit();

	BOOL FAddHandleRef(); 

	void ReleaseHandleRef(); 

	BOOL FCloseRefHandle(); 

	DWORD OpenPipe( __in LPWSTR wszPipeName, __in DWORD dwTimeout );

	DWORD PostReadAsync( SNI_Packet *pPacket ); 

	DWORD SendPacketAsync( SNI_Packet *pPacket, DWORD *pdwBytesWritten );

	DWORD Win9xWaitForData( int iTimeOut ); 

};

#endif
