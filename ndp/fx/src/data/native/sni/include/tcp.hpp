//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: tcp.hpp
// @Owner: petergv, nantu
// @Test: milu
//
// <owner current="true" primary="true">petergv</owner>
// <owner current="true" primary="false">nantu</owner>
//
// Purpose: SNI Tcp Provider
// 
// Notes:
//          
// @EndHeader@
//****************************************************************************

#ifndef _TCP_HPP_
#define _TCP_HPP_


#ifndef SNI_BASED_CLIENT
// Cluster interface structure.  
// adapted from sql\ntdbms\ntinc\resource.h

typedef HCLUSTER	(WINAPI *PFOPENCLUSTER)				(LPCWSTR);
typedef BOOL		(WINAPI *PFCLOSECLUSTER)			(HCLUSTER);

typedef HRESOURCE	(WINAPI *PFOPENCLUSTERRESOURCE)		(HCLUSTER,LPCWSTR);
typedef BOOL		(WINAPI *PFCLOSECLUSTERRESOURCE)	(HRESOURCE);

typedef HCLUSENUM	(WINAPI *PFCLUSTEROPENENUM)			(HCLUSTER,DWORD);
typedef DWORD		(WINAPI *PFCLUSTERENUM)				(HCLUSENUM,DWORD,LPDWORD,LPWSTR,LPDWORD);
typedef DWORD		(WINAPI *PFCLUSTERCLOSEENUM)		(HCLUSENUM);

typedef HRESENUM	(WINAPI *PFCLUSTERRESOURCEOPENENUM)	(HRESOURCE,DWORD);
typedef DWORD		(WINAPI *PFCLUSTERRESOURCEENUM)		(HRESENUM,DWORD,LPDWORD,LPWSTR,LPDWORD);
typedef DWORD		(WINAPI *PFCLUSTERRESOURCECLOSEENUM)(HRESENUM);

typedef DWORD		(WINAPI *PFCLUSTERRESOURCECONTROL)	(HRESOURCE,HNODE,DWORD,LPVOID,DWORD,LPVOID,DWORD,LPDWORD);

typedef DWORD		(WINAPI *PFRESUTILFINDSZPROPERTY)	(PVOID,DWORD,LPCWSTR,LPWSTR*);
typedef DWORD		(WINAPI *PFRESUTILRESOURCETYPESEQUAL)	(  LPCWSTR, HRESOURCE);



struct CLUSTER_API
{
	HMODULE						hClusterLibrary;
	
	PFOPENCLUSTER					OpenCluster;
	PFCLOSECLUSTER					CloseCluster;

	PFOPENCLUSTERRESOURCE			OpenClusterResource;
	PFCLOSECLUSTERRESOURCE		CloseClusterResource;

	PFCLUSTEROPENENUM				ClusterOpenEnum;
	PFCLUSTERENUM					ClusterEnum;
	PFCLUSTERCLOSEENUM				ClusterCloseEnum;

	PFCLUSTERRESOURCEOPENENUM		ClusterResourceOpenEnum;
	PFCLUSTERRESOURCEENUM			ClusterResourceEnum;
	PFCLUSTERRESOURCECLOSEENUM	ClusterResourceCloseEnum;

	PFCLUSTERRESOURCECONTROL		ClusterResourceControl;

	HMODULE						hResourceLibrary;
	PFRESOURCEFINDSZPROPERTY		ResUtilFindSzProperty;
	PFRESUTILRESOURCETYPESEQUAL	ResUtilResourceTypesEqual;
	
};


#endif

//
// Don't set the file handle event on IO completion.
//
#ifndef FILE_SKIP_SET_EVENT_ON_HANDLE
#define FILE_SKIP_SET_EVENT_ON_HANDLE           0x2
#endif

typedef BOOL  (WINAPI *PFN_WIN32_SETFILECOMPLETIONNOTIFICATIONMODES) (__in HANDLE FileHandle, __in UCHAR Flags);


// Used for loading ConnectEx.
typedef struct __CONNECTEXFUNC
{
	LPFN_CONNECTEX pfnConnectEx; 
} CONNECTEXFUNC;

class Tcp : public SNI_Provider
{
private:	

	SOCKET	m_sock;
	PeerAddrInfo m_PeerAddr;
	PeerAddrInfo m_LocalAddr;
	USHORT m_PeerPort;
	USHORT m_LocalPort;
	SNI_Packet * m_pSyncReadPacket;

	// Critical section and member variables protecting the socket handle 
	// while it is being closed.  
	//
	SNICritSec *m_CSClose; 
	BOOL m_fClose; 
	DWORD m_dwcHandleRef; 
	
	//This protects that overlapped pending reads complete with 
	//ERROR_OPERATION_ABORTED after tcp::close is being called.
	//This flag is used by client side code.
	volatile LONG	m_lCloseCalled; 	

	//TCP autotuning
	SNICritSec *m_CSTuning;
	LPWSAOVERLAPPED m_pOvSendNotificaiton;
	BOOL m_fAuto;
	
public:
	Tcp(SNI_Conn * pConn);
	~Tcp();	

	static BOOL s_fAutoTuning;
	static BOOL s_fSkipCompletionPort;

	static DWORD Initialize(PSNI_PROVIDER_INFO pInfo);
	static DWORD Terminate();
	static DWORD InitializeListener(HANDLE hSNIListener, __inout TcpListenInfo *pListenInfo, __out HANDLE * pListenHandle);
	static DWORD TerminateListener(__inout HANDLE hListener);
	static DWORD ResumePendingAccepts(__in HANDLE hListener);
	static DWORD Open( 	SNI_Conn 		* pConn,
							ProtElem 		* pProtElem, 
							__out SNI_Provider 	** ppProv,
							int timeout);
	
	DWORD LoadConnectEx(__in CONNECTEXFUNC** pfnCF);	
	DWORD SocketOpenSync(__in ADDRINFOW* AIW, int timeout);
	DWORD SocketOpenParallel(__in const ADDRINFOW *AIW, DWORD timeout);
	DWORD CheckConnection( );
	DWORD ReadSync(__out SNI_Packet ** ppNewPacket, int timeout);
	DWORD ReadAsync(__out SNI_Packet ** ppNewPacket, LPVOID pPacketKey);
	DWORD PartialReadSync(__inout SNI_Packet * pPacket, DWORD cbBytesToRead, int timeout);
	DWORD PartialReadAsync(__in SNI_Packet * pPacket, DWORD cbBytesToRead, SNI_ProvInfo * pProvInfo);
	DWORD WriteSync(__in SNI_Packet * pPacket, SNI_ProvInfo * pProvInfo);
	DWORD WriteAsync(__in SNI_Packet * pPacket, SNI_ProvInfo * pProvInfo);
	DWORD GatherWriteAsync(__in SNI_Packet * pPacket, SNI_ProvInfo * pProvInfo);
	DWORD ReadDone(__inout SNI_Packet ** ppPacket, __out SNI_Packet **ppLeftOver, DWORD dwBytes, DWORD dwError);
	DWORD WriteDone(SNI_Packet ** ppPacket, DWORD dwBytes, DWORD dwError);	
	DWORD Close();

	DWORD SetPeerAddrInfo(sockaddr * pSockAddr, int iSockAddr);
	DWORD SetLocalAddrInfo(sockaddr * pSockAddr, int iSockAddr);
	static DWORD GetPeerAddress(__in SNI_Conn * pConn, __out PeerAddrInfo * addrinfo);
	static DWORD GetPeerPort(__in SNI_Conn * pConn, __out USHORT * port);
	static DWORD GetLocalAddress(__in SNI_Conn * pConn, __out PeerAddrInfo * addrinfo);
	static DWORD GetLocalPort(__in SNI_Conn * pConn, __out USHORT * port);
	static DWORD GetDnsName( WCHAR *wszAddress, __out_ecount(len) WCHAR *wszDnsName, int len);
	static BOOL FIsLoopBack(const WCHAR* pwszServer);

	DWORD SetKeepAliveOption();
	inline void SetSockBufAutoTuning(BOOL* pfAuto){ Assert (pfAuto); m_fAuto = (*pfAuto == TRUE && s_fAutoTuning ==TRUE); }
		
	static DWORD AcceptDone( SNI_Conn * pConn, 
								 __inout LPVOID pAcceptKey,
								 DWORD dwBytes,
								 DWORD dwError,
						   		 __out SNI_Provider ** ppProv,
						   		 __out LPVOID * ppAcceptInfo );
	void Release();

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
	
	DWORD PostReadAsync(SNI_Packet *pPacket, DWORD cbBuffer);
	static Tcp * AcceptConnection( SNI_Conn *pConn, SOCKET AcceptSocket, char * szAddressBuffer);

	static bool IsNumericAddress( LPWSTR name);

	DWORD Tcp::FInit(); 

	DWORD ParallelOpen(__in ADDRINFOW *AddrInfoW, int timeout, DWORD dwStartTickCount);
	
	__inline  DWORD CheckAndAdjustSendBufferSizeBasedOnISB();
	
// helper for Tcp::Open
	inline static DWORD ComputeNewTimeout(DWORD timeout, DWORD dwStart);

	BOOL FAddHandleRef(); 

	void ReleaseHandleRef(); 

	BOOL FCloseRefHandle(); 

	static DWORD ShouldEnableSkipIOCompletion(__out BOOL* pfShouldEnable);
	static UINT GetAddrCount(const ADDRINFOW *AIW);
};

#endif
