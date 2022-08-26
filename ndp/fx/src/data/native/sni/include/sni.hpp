//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: sni.hpp
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

#ifndef _SNI_HPP_
#define _SNI_HPP_

// Max. size for all params
#define MAX_NAME_SIZE 255
#define ERROR_FAIL -1
#define MAX_GATHERWRITE_BUFS 32
#define SNIOPEN_TIMEOUT_VALUE INFINITE

// Maximum possible size of an SPN that SNI will compose, both for outgoing connectivity and for server-side registration
extern const DWORD SNI_MAX_COMPOSED_SPN;

// Fwd. declarations
class SNI_Conn;
class SNI_Packet;
class SNI_Sec;

// Unique Connection Information
// Used in correlation between client and server
// via error and information tracing
struct SNIUCI
{
	short ConsumerConnId;	// For Engine this is the spid
	GUID  Id;
	GUID  PeerId;
};

// Note: For SNIX, replace __cdecl in PIOCOMP_FN below with __stdcall
#ifndef SNIX
typedef void (__cdecl * PIOCOMP_FN) (LPVOID m_ConsKey, SNI_Packet * pPacket, DWORD dwError);		// Consumer callback function for I/O
#else
typedef void (__stdcall * PIOCOMP_FN) (LPVOID m_ConsKey, SNI_Packet * pPacket, DWORD dwError);		// Consumer callback function for I/O
#endif

typedef void (__cdecl * PACCEPTCOMP_FN) (SNI_Conn * pConn, LPVOID pInfo);		// Consumer callback function for Accept

typedef void (__cdecl * PIOTRACE_FN) (SNI_Conn* pSNIConn, DWORD dwOSError, DWORD dwSNIError);	// Consumer callback for tracing of errors

extern OSVERSIONINFO g_osviSNI;	// OS version info - used by SSL_PROV, SSPI_PROV, NP_PROV, etc.

// Extended Protection level constant, to share with consumers who don't need/want all of sni.hpp, such as NLRegS
#include "SNIExtendedProtection.hpp"

typedef enum
{
	SNIAUTH_ERRST_SUCCESS = 0, // unused
	SNIAUTH_ERRST_SSPIHANDSHAKE_QUERYCHANNELIMPERSONATION = 1, // Start at 1 since this state was previously hard-coded to 1
	// SNIAUTH_ERRST_SSPIHANDSHAKE_SNISECGENSERVERCONTEXTFAILED = 2,		// DO NOT USE! LEGACY REFERENCE ONLY! BROKEN DOWN INTO SUBSTATES
	SNIAUTH_ERRST_SSPIHANDSHAKE_ACCEPTSECURITYCONTEXT = 14, // hard-code to SQL 11 state value
	SNIAUTH_ERRST_SSPIHANDSHAKE_COMPLETEAUTHTOKEN = 15, // hard-code to SQL 11 state value
	SNIAUTH_ERRST_SSPIHANDSHAKE_NOCOMPLETEAUTHTOKENPOINTER = 16,
	SNIAUTH_ERRST_SSPIHANDSHAKE_CHANNELBINDINGS_NOTSUPPORTED = 44, // hard-code to 44; last SQL 11 state is 43
	SNIAUTH_ERRST_SSPIHANDSHAKE_CHANNELBINDINGS_EMPTYORWRONG,
	SNIAUTH_ERRST_SSPIHANDSHAKE_CHANNELBINDINGS_NULLOREMPTYORWRONG,
	SNIAUTH_ERRST_SSPIHANDSHAKE_CHANNELBINDINGS_EMPTY,
	SNIAUTH_ERRST_SSPIHANDSHAKE_SERVICEBINDINGS_UNSUPPORTED,
	SNIAUTH_ERRST_SSPIHANDSHAKE_SERVICEBINDINGS_QUERYCONTEXTATTRIBUTES,
	SNIAUTH_ERRST_SSPIHANDSHAKE_SERVICEBINDINGS_QUERYCONTEXTATTRIBUTES2,
	SNIAUTH_ERRST_SSPIHANDSHAKE_SERVICEBINDINGS_NULL,
	SNIAUTH_ERRST_SSPIHANDSHAKE_SERVICEBINDINGS_EMPTY,
	SNIAUTH_ERRST_SSPIHANDSHAKE_SERVICEBINDINGS_SERVICECLASSMISMATCH,
	SNIAUTH_ERRST_SSPIHANDSHAKE_SERVICEBINDINGS_IPADDRESSMISMATCH,
	SNIAUTH_ERRST_SSPIHANDSHAKE_SERVICEBINDINGS_HOSTNAMEMISMATCH,
	SNIAUTH_ERRST_SSPIHANDSHAKE_OOM,
	SNIAUTH_ERRST_SSPIHANDSHAKE_SERVICEBINDINGS_WSASTRINGTOADDRESSFAILEDFORIPV6,
	
	SNIAUTH_ERRST_SSLADJUSTPROTOCOLFIELDS_COPYPACKET,
	
	SNIAUTH_ERRST_INVALID
} SNIAuthErrStates;

//----------------------------------------------------------------------------
// Name: 	ProviderNum
//
// Purpose:	Enum information to identify supported providers
//
// Notes:	Ensure that the Transport providers ALWAYS follow the
//			Intermediate providers.  Also add a new provider
//			description to the ProviderName array defined in
//			sni_error.cpp, and the SNI_STRING_PROV_* ID's defined
//			in sni_rc.h.
//			
//----------------------------------------------------------------------------
typedef enum 
{
	HTTP_PROV,
	NP_PROV,
	SESSION_PROV,
	SIGN_PROV,
	SM_PROV,
	SMUX_PROV,
	SSL_PROV,
	TCP_PROV,	
	VIA_PROV,
	MAX_PROVS,
	INVALID_PROV	// This MUST always be last entry in list!
} ProviderNum;

typedef enum 
{
	UNKNOWN_PREFIX,
	SM_PREFIX,
	TCP_PREFIX,	
	NP_PREFIX,
	VIA_PREFIX,
	INVALID_PREFIX	// This MUST always be last entry in list!	
} PrefixEnum;

//----------------------------------------------------------------------------
// Name: 	SecPkg
//
// Purpose:	Enum information to identify security packages
//			
//----------------------------------------------------------------------------
typedef enum
{
	NTLM,
	KERBEROS,
	DIGEST,
	BASIC,		// used by Http provider
	NEGOTIATE,		// used by Http provider
	MAX_PACKS,
	INVALID
} SecPkg;

typedef enum 
{
	SSL_INIT,
	SSL_MORE,
	SSL_LAST,
	SSL_DONE,
	SSL_ERROR,
	SSL_REMOVED
} SslState;

//----------------------------------------------------------------------------
// Name: 	QTypes
//
// Purpose:	Enum to get/set different options associated with SNI_Conn
//			
//----------------------------------------------------------------------------

typedef enum 
{
	SNI_QUERY_CONN_INFO,
	SNI_QUERY_CONN_BUFSIZE,
	SNI_QUERY_CONN_KEY,
	SNI_QUERY_CLIENT_ENCRYPT_POSSIBLE,
	SNI_QUERY_SERVER_ENCRYPT_POSSIBLE,
	SNI_QUERY_CERTIFICATE,
	SNI_QUERY_LOCALDB_HMODULE,
	SNI_QUERY_CONN_ENCRYPT,
	SNI_QUERY_CONN_PROVIDERNUM,
	SNI_QUERY_CONN_CONNID,
	SNI_QUERY_CONN_PARENTCONNID,
	SNI_QUERY_CONN_SECPKG,
	SNI_QUERY_CONN_NETPACKETSIZE,
	SNI_QUERY_CONN_NODENUM,
	SNI_QUERY_CONN_PACKETSRECD,
	SNI_QUERY_CONN_PACKETSSENT,
	SNI_QUERY_CONN_PEERADDR,
	SNI_QUERY_CONN_PEERPORT,
	SNI_QUERY_CONN_LASTREADTIME,
	SNI_QUERY_CONN_LASTWRITETIME,
	SNI_QUERY_CONN_CONSUMER_ID,
	SNI_QUERY_CONN_CONNECTTIME,
	SNI_QUERY_CONN_HTTPENDPOINT,
	SNI_QUERY_CONN_LOCALADDR,
	SNI_QUERY_CONN_LOCALPORT,
	SNI_QUERY_CONN_SSLHANDSHAKESTATE,
	SNI_QUERY_CONN_SOBUFAUTOTUNING,
	SNI_QUERY_CONN_SECPKGNAME,
	SNI_QUERY_CONN_SECPKGMUTUALAUTH,
	SNI_QUERY_CONN_CONSUMERCONNID,
	SNI_QUERY_CONN_SNIUCI,
	SNI_QUERY_CONN_SUPPORTS_EXTENDED_PROTECTION,
	SNI_QUERY_CONN_CHANNEL_PROVIDES_AUTHENTICATION_CONTEXT,
	SNI_QUERY_CONN_PEERID,
	SNI_QUERY_CONN_SUPPORTS_SYNC_OVER_ASYNC,
#ifdef SNI_BASED_CLIENT
	// NOTE: Keep all conditional QTypes at the end of the enum
	SNI_QUERY_TCP_SKIP_IO_COMPLETION_ON_SUCCESS,
#endif
} QTypes;

//----------------------------------------------------------------------------
// Name: 	SNI_Packet_IOType
//
// Purpose:	Determines the type of usage of a SNI_Packet
//
// Note:	SNI_Packet_KeyHolderNoBuf is used as a keyholder at Smux's Session
//			class - see Session::m_ReadPacketQueue and in Smux::m_pPacketKeyHolder
//			It is simply a "buffer-less" packet storing key and CR pointer.
//			SNI_Packet_VaryingBufferRead and SNI_Packet_VaryingBufferWrite
//			will be used by SSB to dynamically assign a buffer to them to
//			avoid memcpy ops. From SNI's point of view, these will be treated
//			as "buffer-less" packets.
//----------------------------------------------------------------------------
typedef enum
{
	SNI_Packet_Read = 0,
	SNI_Packet_Write,
	SNI_Packet_KeyHolderNoBuf,	// IMPORTANT! Has a "read" usage and completion routine
	SNI_Packet_VaryingBufferRead,
	SNI_Packet_VaryingBufferWrite,
	SNI_Packet_InvalidType	// IMPORTANT! Please do not add packet types beyond this,
							// add prior if needed.
} SNI_Packet_IOType;


//----------------------------------------------------------------------------
// Name: 	ConsumerNum
//
// Purpose:	Determines the Consumer requesting objects like SNI_Packet
//
// Note:	ConsumerNum is an enumeration introduced to track ownership of
//			the buffers in "varying buffers" SNI_Packets. The primary goal is 
//			to improve supportability and ease the debugging.
//			Currently "varying buffer" packets are used only by SSB.
//----------------------------------------------------------------------------
typedef enum
{
	SNI_Consumer_SNI = 0,
	SNI_Consumer_SSB,
	SNI_Consumer_PacketIsReleased,
	SNI_Consumer_Invalid
} ConsumerNum;

// Options used by SNIAddProvider for ssl/tls provider

// This enables validation of server certificate

#define SNI_SSL_VALIDATE_CERTIFICATE	1

// This enables schannel session cache

#define SNI_SSL_USE_SCHANNEL_CACHE	2

// If no ipaddress is specified and not clustered listen on loopback 
// IP addresses only.  Ignored if ipaddress is specified or if clustered.  
//

#define SNI_SSL_IGNORE_CHANNEL_BINDINGS 0x10 // 0x4 and 0x8 are already used in SQL 11, using 0x10 to ease forward-porting

#define SNI_TCP_INFO_FLAG_LOOPBACK	0x1	

struct TcpListenInfo
{
	WCHAR *ipaddress;
	USHORT Port;
	BOOL fStatic;
	BOOL fUpdated;
	BOOL dwFlags;	// Must default to 0 since SSB zeroes outr the whole struct
					// after creating the struct.  

	TcpListenInfo():ipaddress(0), dwFlags(0)
	{
	}

	~TcpListenInfo()
	{
		delete [] ipaddress;
	}
};

typedef struct
{
	DWORD 	fForceEncryption;
	BOOL	fHash;
	BYTE 	szHash[20];	//SHA1 hash
} SslListenInfo;

typedef struct
{
	WCHAR wszVendor[MAX_NAME_SIZE+1];	
	char szVendorDll[MAX_NAME_SIZE+1];
	int  NicId;
	char szDiscriminator[MAX_NAME_SIZE+1];
} ViaListenInfo, ViaEPInfo;

typedef struct
{
	WCHAR wszPipeName[MAX_PATH+1];	
	bool fLocal;
} NpListenInfo, NpEPInfo;

typedef struct
{
	WCHAR wszInstanceName[MAX_PATH+1];
} SmListenInfo, SmEPInfo;

typedef struct
{
	WCHAR wszIpaddress[MAX_PATH+1];
	USHORT Port;
	BOOL fStatic;
} TcpEPInfo;

#ifndef SNI_BASED_CLIENT
class IEndpoint;

typedef struct
{
	IEndpoint * pIEndPoint;;
} HttpEPInfo;
#endif

// This is in sync with IP6_ADDRESS_STRING_BUFFER_LENGTH
// from windns.h
#define SNI_IP6_ADDRESS_STRING_BUFFER_LENGTH 48
typedef struct
{
	WCHAR PeerAddr[SNI_IP6_ADDRESS_STRING_BUFFER_LENGTH+1];	
	__field_range(0, SNI_IP6_ADDRESS_STRING_BUFFER_LENGTH)		//length does not include NULL terminator
	int PeerAddrLen;
} PeerAddrInfo;


//----------------------------------------------------------------------------
// Name: 	SNI_ListenInfo
//
// Purpose:	Struct for consumers to pass in listen information
//			
//----------------------------------------------------------------------------
typedef struct
{
	ProviderNum Provider;					// Provider number
	
	union 
	{
		SslListenInfo	* pSslListenInfo;
		NpListenInfo	* pNpListenInfo;
		SmListenInfo	* pSmListenInfo;
		TcpListenInfo	* pTcpListenInfo;
		ViaListenInfo	* pViaListenInfo;
	};
}SNI_ListenInfo;

//----------------------------------------------------------------------------
// Name: 	SNI_EPInfo
//
// Purpose:	Struct for consumers to retrieve endpoint information
//			
//----------------------------------------------------------------------------
struct SNI_EPInfo
{
	ProviderNum Provider;					// Provider number
	
	union 
	{
		NpEPInfo		npInfo;
		SmEPInfo	smInfo;
		TcpEPInfo 	tcpInfo;
		ViaEPInfo 	viaInfo;
#ifndef SNI_BASED_CLIENT		
		HttpEPInfo	httpInfo;
#endif
	};

	SNI_EPInfo()
	{
		Provider = INVALID_PROV;
	};

};

//----------------------------------------------------------------------------
// Name: 	SNI_CONSUMER_INFO
//
// Purpose:	Struct for consumer information
//
// Notes:	Consumers pass in this struct in Listen/Open
//			
//----------------------------------------------------------------------------
typedef struct Sni_Consumer_Info
{ 
	// Per-connection SNI parameters applicable to both client and server to be set before connecting
	LONG		DefaultUserDataLength;
	LPVOID 		ConsumerKey;

	// Completion routines
	PIOCOMP_FN	fnReadComp;
	PIOCOMP_FN	fnWriteComp; 
	PIOTRACE_FN fnTrace;

	// The two func pointers below are not in use by SNIX yet. When those two are ported, please update SNIOpenSyncEx method (open.cpp)
	// and add them to the second BID trace call in this method (BidTraceU7) same way they are traced now in SQL server branch.
	// fnAddProvComp;
	// fnAddProvIOCPSafeComp;

	// 

	PACCEPTCOMP_FN fnAcceptComp;
	DWORD dwNumProts;
	SNI_ListenInfo * rgListenInfo;
	DWORD_PTR	NodeAffinity;

	Sni_Consumer_Info()
	{
		DefaultUserDataLength = 0;
		ConsumerKey = NULL;
		fnReadComp = NULL;
		fnWriteComp = NULL;
		fnTrace = NULL;
		fnAcceptComp = NULL;
		dwNumProts = 0;
		rgListenInfo = NULL;
		NodeAffinity = 0;
	};
} SNI_CONSUMER_INFO, *PSNI_CONSUMER_INFO; 

//----------------------------------------------------------------------------
// TransparentNetworkResolution Action type
//
//----------------------------------------------------------------------------
enum TransparentNetworkResolutionMode : BYTE
{
    DisabledMode = 0,
    SequentialMode,
    ParallelMode
};

//----------------------------------------------------------------------------
// Name: 	SNI_CLIENT_CONSUMER_INFO
//
// Purpose:	Struct for client consumer information
//
// Notes:	Consumers pass in this struct in Listen/Open
//			
//----------------------------------------------------------------------------
struct SNI_CLIENT_CONSUMER_INFO
{ 
	SNI_CONSUMER_INFO	ConsumerInfo;
	LPCWSTR				wszConnectionString; // input
	PrefixEnum			networkLibrary; // input
	__field_ecount(cchSPN)	LPWSTR wszSPN; // empty buffer in - filled buffer out
	DWORD				cchSPN; // input
	__field_ecount(cchInstanceName)	LPSTR szInstanceName; // empty buffer in - filled buffer out
	DWORD				cchInstanceName; // input
	BOOL				fOverrideLastConnectCache; // input
	BOOL				fSynchronousConnection; // input
	int					timeout; // input
	BOOL				fParallel; // input
	TransparentNetworkResolutionMode transparentNetworkResolution; // input
	int					totalTimeout; // input
	bool 				isAzureSqlServerEndpoint; // input

	SNI_CLIENT_CONSUMER_INFO()
	{
		wszConnectionString = NULL;
		networkLibrary = UNKNOWN_PREFIX;
		wszSPN = NULL;
		cchSPN = 0;
		szInstanceName = NULL;
		cchInstanceName = 0;
		fOverrideLastConnectCache = FALSE;
		fSynchronousConnection = FALSE;
		timeout = SNIOPEN_TIMEOUT_VALUE;
		fParallel = FALSE;
		transparentNetworkResolution = TransparentNetworkResolutionMode::DisabledMode;
		totalTimeout = SNIOPEN_TIMEOUT_VALUE;
		isAzureSqlServerEndpoint = false;
	};
};

//----------------------------------------------------------------------------
// Name: 	SNI_Listener
//
//----------------------------------------------------------------------------
typedef struct _SNI_Listener
{
	SNI_CONSUMER_INFO ConsumerInfo;
	ProviderNum     * rgProviderNums;
	HANDLE          * rgProvListenHandles;
	LONG			  cRef;
	HANDLE			  hTerminateListenerEvent;
	_SNI_Listener   * pNext;
} SNI_Listener;

//----------------------------------------------------------------------------
// Name: 	SNI_ERROR
//
// Purpose:	Struct to retrieve error information.
//
// Notes:	None.
//			
//----------------------------------------------------------------------------
typedef struct
{
	ProviderNum Provider;
	WCHAR     	pszErrorMessage[MAX_PATH+1];
	DWORD	    dwNativeError;
	DWORD 	    dwSNIError;

	//
	// Debug Info
	//
	WCHAR * pszFileName;
	WCHAR * pszFunction;
	DWORD	dwLineNumber;
} SNI_ERROR;

//----------------------------------------------------------------------------
// Name: 	SNI_ProvInfo
//
// Purpose:	Struct to pass in provider-specific information
//
// Notes:	Consumers pass in provider specific info to I/O calls, using this struct
//			
//----------------------------------------------------------------------------
typedef struct
{
	ProviderNum	ProvNum;
	LPVOID 		pInfo;
} SNI_ProvInfo;

// Function Declarations

// Connection related functions
extern "C" DWORD SNIInitialize(void * pmo = NULL);
extern "C" DWORD SNIInitializeEx(void * pmo, const ProviderNum * rgProviders, DWORD cProviders, BOOL fIsSystemInst, BOOL fSandbox);
extern "C" DWORD SNITerminate();
extern "C" DWORD SNIInitializeListener(__in SNI_CONSUMER_INFO * pConsumerInfo, __out HANDLE * pListenHandle);
extern "C" DWORD SNITerminateListener(__inout HANDLE hListener);
extern "C" DWORD SNIUpdateListener(HANDLE hListener, ProviderNum ProvNum, LPVOID pInfo);
extern "C" DWORD SNIResumePendingAccepts();
extern "C" DWORD SNIOpen( __in SNI_CONSUMER_INFO * pConsumerInfo,
					   __inout_opt LPWSTR wszConnect,
					   __in LPVOID pOpenInfo,
					   __out SNI_Conn ** ppConn,
					   BOOL fSync);

extern "C" DWORD SNIOpenSync( __in SNI_CONSUMER_INFO * pConsumerInfo,
					   __inout_opt LPWSTR wszConnect,
					   __in LPVOID pOpenInfo,
					   __out SNI_Conn ** ppConn,
					   BOOL fSync,
					   int timeout);

extern "C"  DWORD SNIClose(__inout SNI_Conn * pConn);
extern "C"  DWORD SNIQueryInfo( UINT QType, __out VOID * pbQInfo);
extern "C"  DWORD SNIGetInfo( __in SNI_Conn * pConn, UINT QType, __out VOID * pbQInfo);
extern "C"  DWORD SNISetInfo( __out SNI_Conn * pConn, UINT QType, __in VOID * pbQInfo);
extern "C"  DWORD SNIAddProvider(__inout SNI_Conn * pConn, ProviderNum ProvNum, __in LPVOID pInfo);
extern "C"  DWORD SNIRemoveProvider(__inout SNI_Conn * pConn, ProviderNum ProvNum);
extern "C"  DWORD SNICheckConnection(__in SNI_Conn* pConn); // Has some restrictions with previously timed-out SNIReadSync calls: see the SNI Developer Documentation for details.

// I/O Functions
extern "C"  DWORD SNIReadAsync( __out_opt SNI_Conn * pConn,
			                       	  __out SNI_Packet ** ppNewPacket,
			                       	  LPVOID pPacketKey = NULL);
extern "C"  DWORD SNIReadSync(	__in SNI_Conn    * pConn,
								__out SNI_Packet ** ppNewPacket,
								int        timeout );
extern "C"  DWORD SNIPartialReadAsync( __out_opt SNI_Conn * pConn,
							    __in SNI_Packet * pPacket, 
							    DWORD cbBytesToRead,
							    SNI_ProvInfo * pInfo = NULL);
extern "C"  DWORD SNIPartialReadSync(		__in SNI_Conn	* pConn,
											__inout SNI_Packet	* pOldPacket,
											DWORD		cbBytesToRead,
											int			timeout );
extern "C"  DWORD SNIWriteAsync( __out_opt SNI_Conn * pConn,
					   __inout SNI_Packet * pPacket,
					   SNI_ProvInfo * pInfo = NULL);
extern "C"  DWORD SNIWriteSync( 	__in SNI_Conn     * pConn,
						__inout SNI_Packet   * pPacket,
						SNI_ProvInfo * pProvInfo );
extern "C"  DWORD SNIGatherWriteAsync( __out_opt SNI_Conn * pConn,
					   __in SNI_Packet * pPacket,
					   SNI_ProvInfo * pInfo = NULL);

// Functions for Providers/SOS to use
extern "C"  void SNIAcceptDone(__in LPVOID pVoid);
extern "C"  DWORD SNIRegisterForAccept( HANDLE hProvListener);
extern "C"  DWORD SNICreateAccept( LPVOID hSNIAcceptKey,
										ProviderNum ProvNum,
										__in LPVOID hProvAcceptKey,
										__out OVERLAPPED ** ppOverlap
										);
extern "C"  void SNIDeleteAccept( __in LPVOID hAccept);
extern "C"  inline DWORD SNIPostQCS(OVERLAPPED * pOvl, DWORD dwBytes);
extern "C"  void SNIReadDone(LPVOID pVoid);
extern "C"  void SNIWriteDone(LPVOID pVoid);

// Packet Functions
extern "C"  inline SNI_Packet * SNIPacketAllocate( __in SNI_Conn * pConn, 
							   		  SNI_Packet_IOType IOType );

extern "C"  inline SNI_Packet * SNIPacketAllocateEx2( __in SNI_Conn * pConn, 
							   		  SNI_Packet_IOType IOType,
							   		  ConsumerNum ConsNum );

extern "C"  inline void SNIPacketZeroPayloadOnRelease(SNI_Packet * pPacket, DWORD cbBytesToZero = 0);
extern "C"  inline void SNIPacketAddRef(__in SNI_Packet * pPacket);
extern "C"  inline void SNIPacketRelease(__inout SNI_Packet * pPacket);
extern "C"  inline void SNIPacketReset(SNI_Conn * pConn, SNI_Packet_IOType IOType, __out SNI_Packet * pPacket, ConsumerNum ConsNum);
extern "C"  inline void SNIPacketGetData(__in SNI_Packet * pPacket, __deref_out_bcount(*pcbBuf) BYTE ** ppBuf, __out DWORD * pcbBuf);
extern "C"  inline void SNIPacketSetData(__inout SNI_Packet * pPacket, __in_bcount(cbBuf) const BYTE * pbBuf, __in DWORD cbBuf);
extern "C"  inline void SNIPacketAppendData(__inout SNI_Packet * pPacket, __in_bcount(cbBuf) BYTE * pbBuf, __in DWORD cbBuf);
extern "C"  inline void SNIPacketPrependData(__inout SNI_Packet * pPacket, __in_bcount(cbBuf) BYTE * pbBuf, __in DWORD cbBuf);
extern "C"  inline void SNIPacketIncrementOffset(__inout SNI_Packet * pPacket, DWORD dwOffSet);
extern "C"  inline void SNIPacketDecrementOffset(__inout SNI_Packet * pPacket, DWORD dwOffSet);
extern "C"  inline void SNIPacketSetBufferSize(__out SNI_Packet * pPacket, __in DWORD dwSize);
extern "C"  inline DWORD SNIPacketGetBufferSize(__in SNI_Packet * pPacket);
extern "C"  inline BYTE * SNIPacketGetBufPtr(__in SNI_Packet * pPacket);
extern "C"  inline DWORD SNIPacketGetBufActualSize(__in SNI_Packet * pPacket);
extern "C"  inline DWORD SNIPacketGetBufUnusedSize(__in SNI_Packet * pPacket, SNI_Packet_IOType IOType);
extern "C"  inline OVERLAPPED * SNIPacketOverlappedStruct(__in SNI_Packet * pPacket);
extern "C"  inline SNI_Conn * SNIPacketGetConnection(__in SNI_Packet * pPacket);
extern "C"  inline void SNIPacketSetConnection(__inout SNI_Packet * pPacket, __in_opt SNI_Conn * pConn);
extern "C"  inline DWORD SNIPacketError(__in SNI_Packet * pPacket);
extern "C"  inline void SNIPacketSetError(__out SNI_Packet * pPacket, DWORD dwError);
extern "C"  inline LPVOID SNIPacketGetKey(__in SNI_Packet * pPacket);
extern "C"  inline void SNIPacketSetKey(__out SNI_Packet * pPacket, LPVOID pKey);
extern "C"  inline DWORD SNIPacketPostQCS(__in SNI_Packet * pPacket, DWORD numBytes);
extern "C"  inline void SNIPacketSetNext(__out SNI_Packet * pOld, SNI_Packet * pNew);
extern "C"  inline SNI_Packet * SNIPacketGetNext(SNI_Packet * pPacket);
extern "C"  inline void SNIPacketSetBuffer(SNI_Packet * pPacket, __in_bcount(cbBuf) BYTE * pbBuf, DWORD cbBuf, DWORD numBytes);
extern "C"  inline BYTE* SNIPacketGetBuffer(SNI_Packet * pPacket);
extern "C"  inline DWORD SNIPacketGetActualBytes(SNI_Packet * pPacket);

extern "C" DWORD SNIWaitForSSLHandshakeToComplete( __in SNI_Conn * pConn, DWORD dwMilliseconds );

// Error functions
extern "C"  void SNIGetLastError( __out SNI_ERROR * pErrorStruct );

// Security functions
extern "C"  DWORD SNISecInitPackage(__out DWORD * pcbMaxToken);
extern "C"  DWORD SNISecInitPackageEx(__out DWORD * pcbMaxToken, __in BOOL fInitializeSPN, __in BOOL fInitializeServerCredential, __in ExtendedProtectionPolicy eExtendedProtectionLevel, __in_ecount_opt(dwcAcceptedSPNs) LPWSTR *pwszAcceptedSPNs, __in DWORD dwcAcceptedSPNs);
extern "C"  DWORD SNISecTerminatePackage();
extern "C"  DWORD SNISecGenServerContext( __in SNI_Conn * pConn,
								BYTE     * pIn,
							   DWORD      cbIn,
							   BYTE     * pOut,
							   __in DWORD    * pcbOut,
							   __out BOOL     * pfDone,
							   SNIAuthErrStates *pAuthenticationFailureState,
							   BOOL		fCheckChannelBindings); // tells whether TDS expects Channel Bindings to be checkable.
extern "C"  DWORD SNISecGenClientContext ( __in SNI_Conn * pConn,
								BYTE	*pIn,
								DWORD	cbIn,
								BYTE	*pOut,
								__in DWORD	*pcbOut,
								BOOL	*pfDone,
								__in __nullterminated const WCHAR	*wszServerInfo,
								DWORD    cbServerInfo,
								LPCWSTR pwszUserName,
								LPCWSTR pwszPassword);
extern "C"  DWORD SNISecImpersonateContext(__in SNI_Conn * pConn);
extern "C"  DWORD SNISecRevertContext(__in SNI_Conn * pConn);

extern "C"  DWORD SNISecAddSpn(const WCHAR * wszInstanceName, BOOL fIsSystemInst);
extern "C"  DWORD SNISecRemoveSpn();

extern "C" __success(ERROR_SUCCESS == return) DWORD SNIOpenSyncEx( __inout SNI_CLIENT_CONSUMER_INFO * pClientConsumerInfo,
							__deref_out SNI_Conn ** ppConn);

extern "C" HANDLE SNIServerEnumOpen( LPWSTR pwszServer, BOOL fExtendedInfo);
extern "C" HANDLE SNIServerEnumOpenEx( LPWSTR pwszServer, BOOL fExtendedInfo, DWORD waittime_least, DWORD waittimeout);
extern "C" __success(return > 0) int SNIServerEnumRead(__in HANDLE handle, __out_ecount_part(cBuf, return)  LPWSTR pwBuf, __in int cBuf, __out BOOL *pfMore);
extern "C" __success(return > 0) int SNIServerEnumReadEx(__in HANDLE handle, __out_ecount_part(cBuf, return)  LPWSTR pwBuf, __in int cBuf, __out BOOL *pfMore, __in int timeout);
extern "C" void SNIServerEnumClose(__inout_opt HANDLE handle);

//	Wrapper around the SNIAcceptDone that has the same signature as 
//	SOS_Task::Func so that we can enqueue SOS tasks without having
//	to cast a  function returning a VOID into a function returning a PVOID
//	We do not change the signature of SNIAcceptDone itself because of
//	2 reasons -- 1. SNIAcceptDone is also used as an SOS IO Completion
//	routine which only takes a void function 2. SNIAcceptDone is also
//	used on the client side.
//
PVOID WINAPI SNIAcceptDoneWrapper(__in LPVOID pVoid);

//	this function is only implmented in client side. Nan Tu
BOOL DllMain_Sni( HINSTANCE  hInst,       
                  DWORD      dwReason,    
                  LPVOID     lpReserved ); 

#include "sni_FedAuth.hpp"

#endif
