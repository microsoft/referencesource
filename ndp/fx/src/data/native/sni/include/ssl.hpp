//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: ssl.hpp
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

#ifndef _SSL_HPP_
#define _SSL_HPP_

#include "snipch.hpp"

#include <wincrypt.h>

#define SECURITY_WIN32
#include <sspi.h>

class CryptoBase: public SNI_Provider
{
public:

	CryptoBase( SNI_Conn *pConn);
	virtual DWORD FInit();
	~CryptoBase();


	DWORD ReadSync( __deref_inout SNI_Packet ** ppNewPacket, __in int timeout );
	DWORD ReadAsync(__deref_inout SNI_Packet ** ppNewPacket, __in LPVOID pPacketKey);
	DWORD WriteSync( __inout SNI_Packet * pPacket, __in SNI_ProvInfo * pProvInfo );
	DWORD WriteAsync(__inout SNI_Packet * pPacket, __in SNI_ProvInfo * pProvInfo);
	DWORD ReadDone(__deref_inout SNI_Packet ** ppPacket, __deref_inout SNI_Packet **ppLeftOver, __in DWORD dwBytes, __in DWORD dwError);
	DWORD WriteDone(__deref_inout SNI_Packet ** ppPacket, __in DWORD dwBytes, __in DWORD dwError);
	DWORD Close();
	void Release();
	DWORD RemoveX();
	
protected:
	SslState 		m_State;
	
	SNI_Packet *	m_pLeftOver;

	DWORD 		m_cbHeaderLength;
	DWORD 		m_cbTrailerLength;
	
	SNICritSec *	m_CS;

	DWORD	    m_dwSslNativeError;
	DWORD 	    m_dwSslSNIError;

	// WriteSync is different than other functions and need a separate critical section object
	
	SNICritSec *	m_WriteSyncCS;

	HANDLE		m_hHandshakeDoneEvent;

	void CallbackError();
	void ProcessPendingIO();
	
private:

	bool 			m_fClosed;
	bool 			m_fReadInProgress;
	
	DynamicQueue 	m_ReadPacketQueue;
	DynamicQueue 	m_WritePacketQueue;

	virtual DWORD HandshakeReadDone( SNI_Packet *pPacket, DWORD dwError );
	virtual DWORD HandshakeWriteDone( SNI_Packet *pPacket, DWORD dwError );
	virtual DWORD 	HandshakeReadToken( SNI_Packet *pPacket);
	virtual DWORD 	Handshake(void );
	virtual DWORD 	HandshakeWriteToken( SNI_Packet *pPacket);

	DWORD SetHandshakeDoneEvent();

	virtual DWORD Encrypt( SNI_Packet *pPacket ) = 0;
	virtual DWORD Decrypt( SNI_Packet *pPacket, SNI_Packet **ppLeftOver ) = 0;

	DWORD CopyLeftOver( __inout SNI_Packet * pLeftOver, __deref_out SNI_Packet ** ppNewPacket, __in LPVOID pPacketKey ); 

public:
	inline void GetState(SslState* pState) { *pState = m_State;}

	DWORD WaitForHandshakeDone(DWORD dwMilliseconds);
	
};

class Sign: public CryptoBase
{
public:
	
	Sign( SNI_Conn *pConn);
	~Sign();

	static DWORD Initialize( PSNI_PROVIDER_INFO pInfo );
	static DWORD Terminate();
	DWORD InitX( __in PCtxtHandle h);
	
private:

	const static DWORD s_cbSignLength = 16;	//MD5

	const static DWORD s_cbHeaderLength = 4;
	const static DWORD s_cbTrailerLength = 8+s_cbSignLength;


	HCRYPTKEY 		m_hKey;	//session key
	HCRYPTPROV 		m_hCryptProv;
	
	ULONGLONG 		m_SeqNumS;	//sequence number for sends
	ULONGLONG 		m_SeqNumR;	//sequence number for receives
	

	PCtxtHandle 		hContext;
	

	DWORD MakeSignature( BYTE *pBuf, DWORD cBuf );
	DWORD VerifySignature( BYTE *pBuf, DWORD cBuf );

	DWORD Encrypt( __inout SNI_Packet *pPacket);

	DWORD Decrypt( __inout SNI_Packet *pPacket, __deref_out SNI_Packet **ppLeftOver );

};

// Defined in sni.cpp, but used by CryptoBase provider subclasses: Ssl
void DecrementConnBufSize(__out SNI_Conn * pConn, SNI_PROVIDER_INFO * pProvInfo);
void IncrementConnBufSize(__out SNI_Conn * pConn, SNI_PROVIDER_INFO * pProvInfo);

class Ssl: public CryptoBase
{
public:

	Ssl(SNI_Conn * pConn);
	~Ssl();
	
	static DWORD AcquireCredentialsForClient( PCredHandle pHandle);
	static DWORD AcquireCredentialsForServer(PCCERT_CONTEXT pCertContext);
	
	static DWORD Initialize( PSNI_PROVIDER_INFO pInfo );
	static DWORD Terminate();
	static DWORD InitializeListener( __in HANDLE   hSNIListener, 
							  __in SslListenInfo *pInfo, 
							  __out HANDLE * pListenHandle );
	static DWORD TerminateListener(HANDLE hListener);

	static void ReleaseChannelBindings(void *pvChannelBindings);

	static DWORD IsClientEncryptPossible();
	static DWORD IsServerEncryptPossible();
	static PCCERT_CONTEXT GetServerCertificate();

	DWORD 	Encrypt( __inout SNI_Packet *pPacket );
	DWORD 	Decrypt( __inout SNI_Packet *pPacket, __deref_out SNI_Packet **ppLeftOver );

	DWORD 	AllocateReadWriteBuffers();
	void 	FreeReadWriteBuffers();

	DWORD 	InitX(__in LPVOID pUserOption);

	DWORD 	Validate( WCHAR * wszSvr);

	DWORD 	Handshake( void );
	DWORD 	HandshakeReadDone( __inout_opt SNI_Packet *pPacket, __in DWORD dwError );
	DWORD 	HandshakeReadToken( __inout_opt SNI_Packet *pPacket);
	DWORD 	HandshakeWriteDone( __inout_opt SNI_Packet *pPacket, __in DWORD dwError );
	DWORD 	HandshakeWriteToken( __inout_opt SNI_Packet *pPacket);
	
	void 	CallbackError();
	void 	ProcessPendingIO();

	void DecConnBufSize();
	
private:

	static HMODULE 				s_hSecurity;
	static PSecurityFunctionTable 	s_pfTable;

	static DWORD 	s_cbMaxToken;
	static UINT	s_cPagesforMaxToken;
	
	static DWORD 	s_fSslServerReady;

	static HCERTSTORE 	s_hCertStore;

	// To use SSL cache for cases full encryption and encryption with trusted certificate,
	// two CredHandle are needed. Sharing same CredHandle for above two cases can skip  
	// certificate validation required for the full encryption. See SQL BU 489892 for details.
	//
	static CredHandle 		s_hClientCred;
	static CredHandle 		s_hClientCredValidate;
	
	static CredHandle 		s_hServerCred;

	static PCCERT_CONTEXT 	s_hCertContext;

	static bool				s_fChannelBindingsSupported;

	BYTE *		m_pWriteBuffer;
	DWORD 		m_iWriteOffset;
	DWORD 		m_cWriteBuffer;

	BYTE *		m_pReadBuffer;
	DWORD 		m_cReadBuffer;

	bool 		m_fUseExistingCred;
	bool		m_fIgnoreChannelBindings;
	
	CredHandle 	m_hCred;
	CtxtHandle 	m_hContext;

	BOOL 		m_fValidate;

	// maximum message length allowed for encryption
	DWORD		m_cbMaximumMessage;

	BOOL		m_fConnBufSizeIncremented;

	// Internal helpers
	static DWORD FindAndLoadCertificate( __in HCERTSTORE  hMyCertStore, 
										 __in BOOL fHash, 
										 __in void * pvFindPara,
										 __deref_out PCCERT_CONTEXT * ppCertContext
										 );
	
	static DWORD IsServerAuthCert(PCCERT_CONTEXT   pCertContext );
	static DWORD GetCertShaHash(PCCERT_CONTEXT   pCertContext, BYTE* pbShaHash, DWORD* pcbSize);
	static DWORD GetCertShaHashInText(PCCERT_CONTEXT   pCertContext, WCHAR* pwszShaHashText, DWORD cchSize);

	static DWORD	LoadSecurityLibrary();

	DWORD SetChannelBindings();

	virtual DWORD AdjustProtocolFields();
	void IncConnBufSize();
	DWORD CopyPacket( __inout SNI_Packet * pLeftOver, __deref_out SNI_Packet ** ppNewPacket, __in_opt LPVOID pPacketKey, SNI_Packet_IOType ioType ); 
	void GetSSLProvInfo(SNI_PROVIDER_INFO *pProvInfo);
};

#endif
