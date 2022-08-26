//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: sni_prov.hpp
// @Owner: petergv, nantu
// @Test: milu
//
// <owner current="true" primary="true">petergv</owner>
// <owner current="true" primary="false">nantu</owner>
//
// Purpose: Base class for SNI Providers
//
// Notes:
//          
// @EndHeader@
//****************************************************************************

#ifndef _SNI_PROV_HPP_
#define _SNI_PROV_HPP_

//----------------------------------------------------------------------------
// Name: 	SNI_PROVIDER_INFO
//
// Purpose:	Struct for provider information
//
//----------------------------------------------------------------------------
typedef struct
{
  ProviderNum ProvNum;
  ULONG  Size;
  ULONG  Offset;
  BOOL 	  fBaseProv;
  BOOL   fInitialized; 
} SNI_PROVIDER_INFO, *PSNI_PROVIDER_INFO;

// Array of ProviderInfo
extern SNI_PROVIDER_INFO rgProvInfo[MAX_PROVS];

// Notes: All Base Providers who do internal buffering should check whether the buffer has changed
// after data has been received

// Fwd. decalarations
class SNI_Conn;
class SNI_Packet;


#ifndef __rare
// Decorations for data locality.  In the future will be used by prefix to detect poor placement.
// __rare means member is rarely used at execution plan runtime
// __freq means frequently used at execution plan runtime.
// For details, see PeterCa.
#define __rare
#endif
#ifndef __freq
#define __freq
#endif

//----------------------------------------------------------------------------
// Name: 	SNI_Provider
//
// Purpose:	Base class for all Providers to inherit from
//
//----------------------------------------------------------------------------
class SNI_Provider
{
	// 

public:
	SNI_Provider * m_pNext;
	HANDLE m_hNwk;
	ProviderNum m_Prot;
	SNI_Conn * m_pConn;
	BOOL m_fActive;
	int m_iBidId; 

	SNI_Provider(SNI_Conn * pConn) : m_pConn(pConn), m_iBidId(0)
	{
		m_pNext = 0;
		m_hNwk = INVALID_HANDLE_VALUE;
		m_Prot = INVALID_PROV;
		Assert( m_pConn );
		m_fActive = TRUE;
	}

	SNI_Provider(): m_iBidId(0)
	{
		m_pNext = 0;
		m_hNwk = INVALID_HANDLE_VALUE;
		m_Prot = INVALID_PROV;
		m_pConn = NULL;
		m_fActive = TRUE;
	}

	virtual ~SNI_Provider()
	{
		Assert( m_Prot != INVALID_PROV );
		Assert( m_pConn || m_Prot == VIA_PROV ); //the last via prov object will not have the conn set.
	}

	inline int GetBidId() const
	{
		return m_iBidId; 
	}

	inline int * GetBidIdPtr()
	{
		return &m_iBidId; 
	}

	// Member functions
	static void InitProviders(__in_ecount_opt(cProviders) const ProviderNum * rgProviders, __in DWORD cProviders);
	static void Terminate();
	
	// Init/Accept/Open functions
	static DWORD InitializeListener(HANDLE hSNIAcceptKey, LPWSTR wszListenInfo, HANDLE * pListenHandle);
	static DWORD TerminateListener(HANDLE hListener);
	static DWORD UpdateListener(HANDLE hListener, LPVOID pInfo);
	static DWORD ResumePendingAccepts(HANDLE hListener);
	static DWORD AcceptDone( SNI_Conn * pConn,
								 LPVOID pAcceptKey,
								 DWORD dwBytes,
								 DWORD dwError,
						   		 SNI_Provider ** ppProv,
						   		 LPVOID * ppAcceptInfo );
	static DWORD Open( SNI_Conn * pConn,
						 LPWSTR wszConnect,
						 LPVOID pOpenInfo,
						 OUT SNI_Provider ** ppProv);


	virtual DWORD Close() = 0;

	// I/O functions
	virtual DWORD PartialReadSync(SNI_Packet * pPacket, DWORD cbBytesToRead, int timeout);
	virtual DWORD PartialReadAsync(SNI_Packet * pPacket, DWORD cbBytesToRead, SNI_ProvInfo * pProvInfo );
	// Providers who support this should override this
	virtual DWORD GatherWriteAsync(SNI_Packet * pPacket, SNI_ProvInfo * pProvInfo);
	virtual DWORD ReadDone(SNI_Packet ** ppPacket, SNI_Packet **ppLeftOver, DWORD dwBytes, DWORD dwError) = 0;
	virtual DWORD WriteDone(SNI_Packet ** ppPacket, DWORD dwBytes, DWORD dwError) = 0;
	virtual void Release() = 0;
	inline virtual DWORD FInit()
	{
		return ERROR_SUCCESS;
	}

	inline virtual DWORD RemoveX()
	{
		m_fActive = FALSE;
		return ERROR_SUCCESS;
	}

	// Security impersonation methods that allow providers w/ channel support 
	// to do impersonation of the client
	
	// DEVNOTE: In SQL11 port, SSPI provider should reimplement these three methods in the following fashion:
	// QueryImpersonation - return ERROR_SUCCESS
	// ImpersonateChannel - call m_pNext->ImpersonateChannel. If it returns ERROR_SUCCCESS, just return ERROR_SUCCESS. 
	//	If it returns error, try to impersonate, and if it succeeds set a flag noting that this provider is being used for impersonation 
	//	(that flag must default to False)
	// RevertImpersonation - Check the flag that is set in Sspi::ImpersonateChannel. If it is set, revert via our SSPI context. Otherwise,
	//	call into m_pNext->RevertImpersonation.
	// After that, SNISecQueryImpersonation, SNISecImpersonateChannel, and SNISecRevertImpersonation should be changed to simple
	// calls into the provider chain, analogous to, say, SNIReadSync
	
	virtual DWORD QueryImpersonation();
	virtual DWORD ImpersonateChannel();
	virtual DWORD RevertImpersonation();
	
	// BEGIN_FREQUENTLY_CALLED
	
	// I/O functions
	__freq virtual DWORD ReadSync(SNI_Packet ** ppNewPacket, int timeout);
	__freq virtual DWORD ReadAsync(SNI_Packet ** ppNewPacket, LPVOID pPacketKey) = 0;
	__freq virtual DWORD WriteSync(SNI_Packet * pPacket, SNI_ProvInfo * pProvInfo);
	__freq virtual DWORD WriteAsync(SNI_Packet * pPacket, SNI_ProvInfo * pProvInfo) = 0;
	
	// default implementation is ONLY for intermediate providers with no special needs (currently: not SMUX). 
	// All base providers must implement.
	__freq virtual DWORD CheckConnection();
	
	// END_FREQUENTLY_CALLED
};

#endif
