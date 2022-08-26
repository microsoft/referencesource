//*********************************************************************
//		Copyright (c) Microsoft Corporation.
//
// @File: via.hpp
// @Owner: nantu | petergv
// @Test: milu
//
// <owner current="true" primary="true">nantu</owner>
// <owner current="true" primary="false">petergv</owner>
//
// Purpose: 
//		Header for VIA provider specific structures 
//
// Notes:
//	
// @EndHeader@
//****************************************************************************	

#ifndef VIA_HEADER
#define VIA_HEADER

#include "vipl.h"
#include "time.h"

#define MAX_VIA_CONNS 1000
#define NIC_NAME "nic0"
#define FRAME_LENGTH 8448
#define PAYLOAD_LENGTH 8192
#define DSCR_LEN 8
#define ADDR_LEN 6
#define SRVPORTNUM 1433
#define MAX_CONNS 1023
#define PKT_HDR 8
#define MAX_VIP_ADDRESS 256
#define MAX_NICS 8
#define LISTEN_PARAMS_MAX 512
#define BUFDESCANDCONNSIZE 128
#define RETRY_TIMEOUT 5000
#define MAX_TIMEOUT 200
#define MAX_CLNT_BUFS 4
#define MAX_SVR_BUFS 4
#define MAX_CLNT_BUFS_PLUS_ACK 5
#define MAX_SVR_BUFS_PLUS_ACK 5

extern DWORD gdwSvrNic;
extern DWORD gdwSvrPortNum;

/*
  default VI attributes that we will request.  Note that the VIPL spec
  guarantees that all implementations will support a 32768 MTU.
*/
static VIP_VI_ATTRIBUTES default_vi_attribs = {
	VIP_SERVICE_RELIABLE_DELIVERY,
	32768,						/* MTU */
	0,							/* QOS is unused */
	0,							/* use the default protection tag */
	FALSE,						/* no RDMA Write */
	FALSE						/* no RDMA Read */
};




/*///////////////////////////////////////////////////////////////
//Removing references to CO_STAT and RETCODE from the VIA sources.
This way, we can be consistent with the rest of the codebase.
For SNI APIs, we will return standard windows errors.
Some of the helpers return VIP_RETURN (look at the static members of Via class.
For these, we have defined a standard mapping : look at MapViErrorToSniError().
In general, idea is to translate VI status codes to windows status codes in the helper functions
mostly implemented in viahelper.cpp so that there are no references to VI status codes in via.cpp, from where
we dont call VIA APIs directly.
via.cpp contains SNI API entry points.
CO_STAT is mapped as follows:
NET_IDLE - ERROR_SUCCESS :: we are never checking in the calling function for this status.
NET_BUSY - ERROR_IO_PENDING :: closest. We are remembering the packet and flusing it later
NET_ERROR - ERROR_FAIL :: generic failure
//typedef enum {      // connection interface status values
//    NET_IDLE,       // connection is idle
//    NET_BUSY,       // connection is reading or writing
//    NET_ERROR       // error occurred during last oper.
//} CO_STAT;
/////////////////////////////////////////////////////////////////*/

// Error Codes for Via Netlibs
#define VIANL_E_SEQNUM      0x6666
#define VIANL_E_HOSTNAME    0x6667
#define MAX_VIP_ADDRESS 256

class Via;

extern "C"
{
typedef
VIP_RETURN (__cdecl *VipOpenNicProc)(
    IN const VIP_CHAR *DeviceName,
    OUT VIP_NIC_HANDLE *NicHandle);

typedef
VIP_RETURN (__cdecl *VipCloseNicProc)(

    IN VIP_NIC_HANDLE NicHandle);

typedef
VIP_RETURN (__cdecl *VipQueryNicProc)(
    IN VIP_NIC_HANDLE NicHandle,
    OUT VIP_NIC_ATTRIBUTES *Attributes);

typedef
VIP_RETURN (__cdecl  *VipRegisterMemProc)(
    IN VIP_NIC_HANDLE NicHandle,
    IN VIP_PVOID VirtualAddress,
    IN VIP_ULONG Length,
    IN VIP_MEM_ATTRIBUTES *MemAttributes,
    OUT VIP_MEM_HANDLE *MemHandle);

typedef
VIP_RETURN (__cdecl  *VipDeregisterMemProc)(
    IN VIP_NIC_HANDLE NicHandle,
    IN VIP_PVOID VirtualAddress,
    IN VIP_MEM_HANDLE MemHandle);

typedef
VIP_RETURN (__cdecl  *VipQueryMemProc)(
    IN VIP_NIC_HANDLE NicHandle,
    IN VIP_PVOID VirtualAddress,
    IN VIP_MEM_HANDLE MemHandle,
    OUT VIP_MEM_ATTRIBUTES *MemAttributes);

typedef
VIP_RETURN (__cdecl  * VipSetMemAttributesProc)(
    IN VIP_NIC_HANDLE NicHandle,
    IN VIP_PVOID VirtualAddress,
    IN VIP_MEM_HANDLE MemHandle,
    IN VIP_MEM_ATTRIBUTES *MemAttributes);

//typedef void (*VIP_ERROR_HANDLER)(VIP_PVOID, VIP_ERROR_DESCRIPTOR *);

typedef
VIP_RETURN (__cdecl  * VipErrorCallbackProc)(
    IN VIP_NIC_HANDLE NicHandle,
    IN VIP_PVOID Context,
    IN VIP_ERROR_HANDLER ErrorHandler);

//
// management
//
typedef
VIP_RETURN (__cdecl  * VipQuerySystemManagementInfoProc)(
    IN VIP_NIC_HANDLE NicHandle,
    IN VIP_ULONG InfoType,
    OUT VIP_PVOID SysManInfo);

//
// Protection tags
//
typedef
VIP_RETURN (__cdecl  * VipCreatePtagProc)(
    IN VIP_NIC_HANDLE NicHandle,
    OUT VIP_PROTECTION_HANDLE *ProtectionTag);

typedef
VIP_RETURN (__cdecl  * VipDestroyPtagProc)(
    IN VIP_NIC_HANDLE NicHandle,
    IN VIP_PROTECTION_HANDLE ProtectionTag);

//
// VI primitives
//
typedef
VIP_RETURN (__cdecl  * VipCreateViProc)(
    IN VIP_NIC_HANDLE NicHandle,
    IN VIP_VI_ATTRIBUTES *ViAttributes,
    IN VIP_CQ_HANDLE SendCQHandle,
    IN VIP_CQ_HANDLE RecvCQHandle,
    OUT VIP_VI_HANDLE *ViHandle);

typedef
VIP_RETURN (__cdecl  * VipDestroyViProc)(
    IN VIP_VI_HANDLE ViHandle);

typedef
VIP_RETURN (__cdecl  * VipQueryViProc)(
    IN VIP_VI_HANDLE ViHandle,
    OUT VIP_VI_STATE *State,
    OUT VIP_VI_ATTRIBUTES *Attributes,
    OUT VIP_BOOLEAN *SendQueueEmpty,
    OUT VIP_BOOLEAN *RecvQueueEmpty);

typedef
VIP_RETURN (__cdecl  * VipSetViAttributesProc)(
    IN VIP_VI_HANDLE ViHandle,
    IN VIP_VI_ATTRIBUTES *Attributes);

typedef
VIP_RETURN (__cdecl  * VipPostSendProc)(
    IN VIP_VI_HANDLE ViHandle,
    IN VIP_DESCRIPTOR *DescriptorPtr,
    IN VIP_MEM_HANDLE MemoryHandle);

typedef
VIP_RETURN (__cdecl  * VipSendDoneProc)(
    IN VIP_VI_HANDLE ViHandle,
    OUT VIP_DESCRIPTOR **DescriptorPtr);

typedef
VIP_RETURN (__cdecl  * VipSendWaitProc)(
    IN VIP_VI_HANDLE ViHandle,
    IN VIP_ULONG TimeOut,
    OUT VIP_DESCRIPTOR **DescriptorPtr);

typedef void (*VIP_VI_CALLBACK)(
    VIP_PVOID Context, VIP_NIC_HANDLE NicHandle, VIP_VI_HANDLE ViHandle, VIP_DESCRIPTOR *Descriptor);

typedef
VIP_RETURN (__cdecl  * VipSendNotifyProc)(
    IN VIP_VI_HANDLE ViHandle,
    IN VIP_PVOID Context,
    IN VIP_VI_CALLBACK Callback);

typedef
VIP_RETURN (__cdecl  * VipPostRecvProc)(
    IN VIP_VI_HANDLE ViHandle,
    IN VIP_DESCRIPTOR *DescriptorPtr,
    IN VIP_MEM_HANDLE MemoryHandle);

typedef
VIP_RETURN (__cdecl  * VipRecvDoneProc)(
    IN VIP_VI_HANDLE ViHandle,
    OUT VIP_DESCRIPTOR **DescriptorPtr);

typedef
VIP_RETURN (__cdecl  * VipRecvWaitProc)(
    IN VIP_VI_HANDLE ViHandle,
    IN VIP_ULONG TimeOut,
    OUT VIP_DESCRIPTOR **DescriptorPtr);

typedef
VIP_RETURN (__cdecl  * VipRecvNotifyProc)(
    IN VIP_VI_HANDLE ViHandle,
    IN VIP_PVOID Context,
    IN VIP_VI_CALLBACK Callback);

typedef
VIP_RETURN (__cdecl  * VipConnectWaitProc)(
    IN VIP_NIC_HANDLE NicHandle,
    IN VIP_NET_ADDRESS *LocalAddr,
    IN VIP_ULONG Timeout,
    OUT VIP_NET_ADDRESS *RemoteAddr,
    OUT VIP_VI_ATTRIBUTES *RemoteViAttributes,
    OUT VIP_CONN_HANDLE *ConnHandle);

typedef
VIP_RETURN (__cdecl  * VipConnectAcceptProc)(
    IN VIP_CONN_HANDLE ConnHandle,
    IN VIP_VI_HANDLE ViHandle);

typedef
VIP_RETURN (__cdecl  * VipConnectRejectProc)(
    IN VIP_CONN_HANDLE ConnHandle);

typedef
VIP_RETURN (__cdecl  * VipConnectRequestProc)(
    IN VIP_VI_HANDLE ViHandle,
    IN VIP_NET_ADDRESS *LocalAddr,
    IN VIP_NET_ADDRESS *RemoteAddr,
    IN VIP_ULONG Timeout,
    OUT VIP_VI_ATTRIBUTES *RemoteViAttributes);

typedef
VIP_RETURN (__cdecl  * VipDisconnectProc)(
    IN VIP_VI_HANDLE ViHandle);

// 
// Completion Queue primitives
//
typedef
VIP_RETURN (__cdecl  * VipCreateCQProc)(
    IN VIP_NIC_HANDLE NicHandle,
    IN VIP_ULONG EntryCount,
    OUT VIP_CQ_HANDLE *CQHandle);

typedef
VIP_RETURN (__cdecl  * VipDestroyCQProc)(
    IN VIP_CQ_HANDLE CQHandle);

typedef
VIP_RETURN (__cdecl  * VipResizeCQProc)(
    IN VIP_CQ_HANDLE CQHandle,
    IN VIP_ULONG EntryCount);

typedef
VIP_RETURN (__cdecl  * VipCQDoneProc)(
    IN VIP_CQ_HANDLE CQHandle,
    OUT VIP_VI_HANDLE *ViHandle,
    OUT VIP_BOOLEAN *RecvQueue);

typedef
VIP_RETURN (__cdecl  * VipCQWaitProc)(
    IN VIP_CQ_HANDLE CQHandle,
    IN VIP_ULONG Timeout,
    OUT VIP_VI_HANDLE *ViHandle,
    OUT VIP_BOOLEAN *RecvQueue);

typedef void (*VIP_CQ_CALLBACK)(
    VIP_PVOID Context, VIP_NIC_HANDLE NicHandle, VIP_VI_HANDLE ViHandle, VIP_BOOLEAN RecvQueue);

typedef
VIP_RETURN (__cdecl  * VipCQNotifyProc)(
    IN VIP_CQ_HANDLE CqHandle,
    IN VIP_PVOID Context,
    IN VIP_CQ_CALLBACK Callback);

//
// name service API
//
typedef
VIP_RETURN (__cdecl  * VipNSInitProc)(
    IN VIP_NIC_HANDLE NicHandle,
    IN VIP_PVOID NSInitInfo);

typedef
VIP_RETURN (__cdecl  * VipNSGetHostByNameProc)(
    IN VIP_NIC_HANDLE NicHandle,
    IN VIP_WCHAR *Name,
    OUT VIP_NET_ADDRESS *Address,
    IN VIP_ULONG NameIndex);

typedef
VIP_RETURN (__cdecl  * VipNSGetHostByAddrProc)(
    IN VIP_NIC_HANDLE NicHandle,
    IN VIP_NET_ADDRESS *Address,
    OUT VIP_CHAR *Name,
    IN OUT VIP_ULONG *NameLen);

typedef
VIP_RETURN (__cdecl  * VipNSShutdownProc)(
    IN VIP_NIC_HANDLE NicHandle);

//
// peer connection API
//
typedef
VIP_RETURN (__cdecl  * VipConnectPeerRequestProc)(
    IN VIP_VI_HANDLE ViHandle,
    IN VIP_NET_ADDRESS *LocalAddr,
    IN VIP_NET_ADDRESS *RemoteAddr,
    IN VIP_ULONG Timeout);

typedef
VIP_RETURN (__cdecl  * VipConnectPeerDoneProc)(
    IN VIP_VI_HANDLE ViHandle,
    OUT VIP_VI_ATTRIBUTES *RemoteAttributes);

typedef
VIP_RETURN (__cdecl  * VipConnectPeerWaitProc)(
    IN VIP_VI_HANDLE ViHandle,
    OUT VIP_VI_ATTRIBUTES *RemoteViAttributes);

//
// Tag demultiplexing
//
typedef
VIP_RETURN (__cdecl  * VipAddTagCQProc)(
	IN VIP_CQ_HANDLE CQHandle,
	IN OUT VIP_EVENT_HANDLE *Event,
	IN VIP_ULONG Tag,
	IN VIP_ULONG Priority);

typedef
VIP_RETURN (__cdecl  * VipRemoveTagCQProc)(
	IN VIP_CQ_HANDLE CQHandle,
	IN VIP_EVENT_HANDLE Event,
	IN VIP_ULONG Tag);

//
// deferred function call
//
typedef
VIP_RETURN (__cdecl  * VipPostDeferredSendsProc)(
	IN VIP_VI_HANDLE vihandle, 
	IN VIP_BOOLEAN enableinterrupt,
	IN OUT VIP_BOOLEAN *sendsdeferred);

}

//the following is the info required for each listening thread. 
//memory is dynamically allocated based on the params passed in during InitializeListener
typedef struct _VIALISTEN
{
	DWORD dwNic; //NIC index
	DWORD dwAff; //affinity for this listener
	DWORD dwPort; //port number for this listener
	LONG gdwActiveConns;
	HANDLE hListener; //listener key
	HANDLE hThread; //thread handle that 
	BOOL *pfTerminate;
	LPVOID pListenStruct; //This is the listen struct back pointer
	char lAddrBuf[MAX_VIP_ADDRESS+1]; //local address for this listener. //
	char rgNic[12];
} VIALISTEN;

typedef struct _listenstruct
{
	HANDLE hListenerKey;//passed in by the user
	BOOL fTerminate; //to notify this set of listeners threads that this listener wants to shutdown
	LONG dwListeners; //no of listen threads
	VIALISTEN *prgListeners; //
	DWORD rgNics[MAX_NICS];
	DWORD cNics;
} ViaListenStruct;

class ProtElem;

typedef enum
{
    EMPTY,
    POSTED,
    WAITING
} BufState;



typedef struct _VIPERROR
{
	DWORD dwVip;
	DWORD dwSni;
} VIPERRORMAP;

typedef struct
{
	LPOVERLAPPED pOverlapped;
	VIP_CONN_HANDLE conn;
	VIP_VI_ATTRIBUTES viAttribs;
	VIP_CQ_HANDLE sendHandle;
	VIP_CQ_HANDLE recvHandle;
	DWORD	iNic;
	VIALISTEN *pListen;
	VIP_VI_HANDLE hVi;
} ViaAcceptStruct;

inline Via * GetConnObj(VIP_DESCRIPTOR * pDesc);
inline void SetConnObj(VIP_DESCRIPTOR * pDesc, LPVOID pConn);
inline SNI_Packet *DescGetPacket(VIP_DESCRIPTOR * pDesc);
inline void DescSetPacket(VIP_DESCRIPTOR * pDesc,SNI_Packet * pPacket);
inline BYTE *DescGetOrigBuf(VIP_DESCRIPTOR * pDesc);
inline void DescSetOrigBuf(VIP_DESCRIPTOR * pDesc, BYTE * pbBuf);
inline VIP_DESCRIPTOR * BufGetDesc(__in const BYTE * pbBuf);
inline DWORD DescGetFragInfo(VIP_DESCRIPTOR * pDesc);
inline void DescSetFragInfo(VIP_DESCRIPTOR * pDesc, DWORD dwVal);
inline DWORD MapVipErrorToSniError(VIP_RETURN vipStatus);
DWORD Hash(__in LPCWSTR string);

#define MAX_PROCS 31

typedef enum
{
	QLogic = 1,
	OtherViaVendor
} ViaVendor;

class ViaWorker;

class Via : public SNI_Provider
{
public:
	
	static char *s_szProcNames[];
	static FARPROC s_pfnProcs[60];	
	
private:	

	// Qs
	DynamicQueue WriteQ;
	DynamicQueue pReadDoneQ;
	DynamicQueue pReadKeyQ;

	// Stats
	DWORD 	m_dwPendingWrites;
	DWORD 	m_dwQuickWrites;
	DWORD	m_dwAcksSent;
	DWORD	m_dwAcksRecd;
	DWORD 	m_dwPendingReads;
	DWORD 	m_dwQuickReads;

	// Sync vars
	HANDLE hWriteEvent;
	HANDLE hSyncReadEvent;
	BufState SendBufState;
	BOOL m_fReadPosted;
	BOOL m_fPendingAck;

	// Flow control vars
	LONG SeqNum;
	LONG DestSeqNum;
	LONG SendLimit;
	LONG DestSendLimit;
	LONG LastSendLimit;

	// Critsecs
	SNICritSec * CritSec;
	SNICritSec * m_SyncWriteCritSec;

	// Close vars
	LONG	fCleanupCalled;
	BOOL 	fClosed;
	BOOL 	fClean;
	BOOL 	fConnClosed;
	BOOL 	fDisconnected;

	// Via vars
	VIP_VI_HANDLE 	m_hVi;
	VIP_MEM_HANDLE 	m_hMem;
	ViaWorker 		* m_pViaWorker;
	void * m_pbMem;

	// Send vars
	VIP_DESCRIPTOR * m_pSendDesc[MAX_SVR_BUFS_PLUS_ACK];
	DWORD m_iSendDesc;
	DWORD m_cbWritePendingOffset;

	// Vars for Reads/Writes
	DWORD m_cRecvs;

	// Tmp vars
	DWORD m_Id;
	
public:
	Via(SNI_Conn * pConn);
	~Via();	
	DWORD Via::FInit();

        void SetConnection(SNI_Conn *pConn){Assert(NULL == m_pConn);m_pConn = pConn;}

	static DWORD Initialize(PSNI_PROVIDER_INFO pInfo);
	static DWORD Terminate();
	static DWORD InitializeListener(HANDLE hSNIListener, ViaListenInfo * pListenInfo, __out HANDLE * pListenHandle);
	static DWORD TerminateListener(HANDLE hListener);
	static BOOL InitQLogicNameService(HANDLE hNic);
        static DWORD PrepareViForAccept(ViaAcceptStruct *pAcc);
        
	static DWORD Open( 	SNI_Conn 		* pConn,
							ProtElem 		* pProtElem, 
							__out SNI_Provider 	** ppProv );

	DWORD ReadSync(__out SNI_Packet ** ppNewPacket, int timeout);
	DWORD ReadAsync(__out SNI_Packet ** ppNewPacket, LPVOID pPacketKey);
	DWORD WriteSync(SNI_Packet * pPacket, SNI_ProvInfo * pProvInfo);
	DWORD WriteAsync(SNI_Packet * pPacket, SNI_ProvInfo * pProvInfo);
	DWORD GatherWriteAsync(SNI_Packet * pPacket, SNI_ProvInfo * pProvInfo);
	DWORD ReadDone(SNI_Packet ** ppPacket, SNI_Packet **ppLeftOver, DWORD dwBytes, DWORD dwError);
	DWORD WriteDone(SNI_Packet ** ppPacket, DWORD dwBytes, DWORD dwError);	
	DWORD Close();
	static DWORD AcceptDone( SNI_Conn * pConn, 
								 __inout LPVOID pAcceptKey,
								 DWORD dwBytes,
								 DWORD dwError,
						   		 __out SNI_Provider ** ppProv,
						   		 LPVOID * ppAcceptInfo );
	void Release();



	DWORD ConnectionOpen( bool fSync, __in ProtElem *pProtElem);

#ifdef SNI_BASED_CLIENT
static DWORD WINAPI Via::CompThread(PVOID dw);
#else
static PVOID WINAPI Via::CompThread(PVOID dw);
#endif

#ifdef SNI_BASED_CLIENT
static DWORD WINAPI ConnectionListenThread(PVOID dw);
#else
static PVOID WINAPI ConnectionListenThread(PVOID dw);
#endif


	static void PostReadDone(SNI_Packet * pPacket, DWORD dwErr);
	static void PostWriteDone(SNI_Packet * pPacket, DWORD dwErr);
	DWORD SendPacket(SNI_Packet * pPacket);

DWORD CompleteRead(VIP_DESCRIPTOR * pDesc, SNI_Packet * pPacket, int timeout);

static VIP_DESCRIPTOR * get_BufDescriptors( VIP_NIC_HANDLE nic, VIP_PROTECTION_HANDLE Ptag, ULONG num, ULONG buflen,
				    VIP_MEM_HANDLE *mh, __out void **ptr, 
                    LPVOID pConn,
                    //LPSTR szError,
                    VIP_RETURN * pdwNetError);

BOOL IsViClean();
static void HandleReceiveEvent(VIP_VI_HANDLE hVi, __out bool *pfQuit);
static void HandleSendEvent(VIP_VI_HANDLE hVi);
DWORD SendDescriptor(VIP_DESCRIPTOR *pDesc, DWORD cbData, int timeout, BOOL fOobAck);
BOOL Cleanup();

VIP_RETURN  AllocDescs(VIP_NIC_HANDLE nic, VIP_PROTECTION_HANDLE pTag);

DWORD SendAck(int timeout);

#define BAIL do { Assert(false); return VIP_ERROR;}while(0);
	typedef enum _methods
	{
		VipOpenNicMethod=0,
		VipCloseNicMethod,
		VipQueryNicMethod,
		VipRegisterMemMethod,
		VipDeregisterMemMethod,
		VipErrorCallbackMethod,
		VipCreatePtagMethod,
		VipDestroyPtagMethod,
		VipCreateViMethod,
		VipDestroyViMethod,
		VipQueryViMethod,
		VipSetViAttributesMethod,
		VipPostSendMethod,
		VipSendDoneMethod,
		VipSendWaitMethod,
		VipPostRecvMethod,
		VipRecvWaitMethod,
		VipRecvDoneMethod,
		VipConnectWaitMethod,
		VipConnectAcceptMethod,
		VipConnectRejectMethod,
		VipConnectRequestMethod,
		VipDisconnectMethod,
		VipCreateCQMethod,
		VipDestroyCQMethod,
		VipCQDoneMethod,
		VipCQWaitMethod,
		VipNSInitMethod,
		VipNSGetHostByNameMethod,
		VipNSGetHostByAddrMethod,
		VipNSShutdownMethod
	} ;

//
// NIC primitives
//
inline static VIP_RETURN __cdecl VipOpenNic(
    IN const VIP_CHAR *DeviceName,
    OUT VIP_NIC_HANDLE *NicHandle)
{
	if(s_pfnProcs[VipOpenNicMethod] != NULL)
		return ((VipOpenNicProc)s_pfnProcs[VipOpenNicMethod])(DeviceName, NicHandle);
	else
		BAIL
}
	


inline static VIP_RETURN  __cdecl VipCloseNic(
    IN VIP_NIC_HANDLE NicHandle)
{
	if(s_pfnProcs[VipCloseNicMethod] != NULL)
		return ((VipCloseNicProc)s_pfnProcs[VipCloseNicMethod])(NicHandle);
	else
		BAIL
}



inline static VIP_RETURN  __cdecl VipQueryNic(
    IN VIP_NIC_HANDLE NicHandle,
    OUT VIP_NIC_ATTRIBUTES *Attributes)
{
	if(s_pfnProcs[VipQueryNicMethod] != NULL)
		return ((VipQueryNicProc)s_pfnProcs[VipQueryNicMethod])(NicHandle,Attributes);
	else
		BAIL
}



inline static VIP_RETURN  __cdecl VipRegisterMem(
    IN VIP_NIC_HANDLE NicHandle,
    IN VIP_PVOID VirtualAddress,
    IN VIP_ULONG Length,
    IN VIP_MEM_ATTRIBUTES *MemAttributes,
    OUT VIP_MEM_HANDLE *MemHandle)
{
	if(s_pfnProcs[VipRegisterMemMethod] != NULL)
		return ((VipRegisterMemProc)s_pfnProcs[VipRegisterMemMethod])(NicHandle,VirtualAddress,Length,MemAttributes,MemHandle);
	else
		BAIL
}



inline static VIP_RETURN  __cdecl VipDeregisterMem(
    IN VIP_NIC_HANDLE NicHandle,
    IN VIP_PVOID VirtualAddress,
    IN VIP_MEM_HANDLE MemHandle)
{
	if(s_pfnProcs[VipDeregisterMemMethod] != NULL)
		return ((VipDeregisterMemProc) s_pfnProcs[VipDeregisterMemMethod])(NicHandle,VirtualAddress,MemHandle);
	else
		BAIL
}



//typedef void (*VIP_ERROR_HANDLER)(VIP_PVOID, VIP_ERROR_DESCRIPTOR *);


inline static VIP_RETURN  __cdecl VipErrorCallback(
    IN VIP_NIC_HANDLE NicHandle,
    IN VIP_PVOID Context,
    IN VIP_ERROR_HANDLER ErrorHandler)
{
	if(s_pfnProcs[VipErrorCallbackMethod] != NULL)
		return ((VipErrorCallbackProc) s_pfnProcs[VipErrorCallbackMethod])(NicHandle,Context,ErrorHandler);
	else
		BAIL

}

//
// Protection tags
//

inline static VIP_RETURN  __cdecl VipCreatePtag(
    IN VIP_NIC_HANDLE NicHandle,
    OUT VIP_PROTECTION_HANDLE *ProtectionTag)
{
	if(s_pfnProcs[VipCreatePtagMethod] != NULL)
		return ((VipCreatePtagProc) s_pfnProcs[VipCreatePtagMethod])(NicHandle,ProtectionTag);
	else
		BAIL
}



inline static VIP_RETURN  __cdecl VipDestroyPtag(
    IN VIP_NIC_HANDLE NicHandle,
    IN VIP_PROTECTION_HANDLE ProtectionTag)
{
	if(s_pfnProcs[VipDestroyPtagMethod] != NULL)
		return ((VipDestroyPtagProc) s_pfnProcs[VipDestroyPtagMethod])(NicHandle,ProtectionTag);
	else
		BAIL
}


//
// VI primitives
//

inline static VIP_RETURN  __cdecl VipCreateVi(
    IN VIP_NIC_HANDLE NicHandle,
    IN VIP_VI_ATTRIBUTES *ViAttributes,
    IN VIP_CQ_HANDLE SendCQHandle,
    IN VIP_CQ_HANDLE RecvCQHandle,
    OUT VIP_VI_HANDLE *ViHandle)
{
	if(s_pfnProcs[VipCreateViMethod] != NULL)
		return ((VipCreateViProc) s_pfnProcs[VipCreateViMethod])(NicHandle,ViAttributes,SendCQHandle,RecvCQHandle,ViHandle);
	else
		BAIL
}



inline static VIP_RETURN  __cdecl VipDestroyVi(
    IN VIP_VI_HANDLE ViHandle)
{
	if(s_pfnProcs[VipDestroyViMethod] != NULL)
		return ((VipDestroyViProc) s_pfnProcs[VipDestroyViMethod])(ViHandle);
	else
		BAIL
}



inline static VIP_RETURN  __cdecl VipQueryVi(
    IN VIP_VI_HANDLE ViHandle,
    OUT VIP_VI_STATE *State,
    OUT VIP_VI_ATTRIBUTES *Attributes,
    OUT VIP_BOOLEAN *SendQueueEmpty,
    OUT VIP_BOOLEAN *RecvQueueEmpty)
{
	if(s_pfnProcs[VipQueryViMethod] != NULL)
		return ((VipQueryViProc) s_pfnProcs[VipQueryViMethod])(ViHandle,State,Attributes,SendQueueEmpty,RecvQueueEmpty);
	else
		BAIL
}



inline static VIP_RETURN  __cdecl VipSetViAttributes(
    IN VIP_VI_HANDLE ViHandle,
    IN VIP_VI_ATTRIBUTES *Attributes)
{
	if(s_pfnProcs[VipSetViAttributesMethod] != NULL)
		return ((VipSetViAttributesProc) s_pfnProcs[VipSetViAttributesMethod])(ViHandle,Attributes);
	else
		BAIL
}



inline static VIP_RETURN  __cdecl VipPostSend(
    IN VIP_VI_HANDLE ViHandle,
    IN VIP_DESCRIPTOR *DescriptorPtr,
    IN VIP_MEM_HANDLE MemoryHandle)
{
	if(s_pfnProcs[VipPostSendMethod] != NULL)
		return ((VipPostSendProc) s_pfnProcs[VipPostSendMethod])(ViHandle,DescriptorPtr,MemoryHandle);
	else
		BAIL
}



inline static VIP_RETURN  __cdecl VipSendDone(
    IN VIP_VI_HANDLE ViHandle,
    OUT VIP_DESCRIPTOR **DescriptorPtr)
{
	if(s_pfnProcs[VipSendDoneMethod] != NULL)
		return ((VipSendDoneProc) s_pfnProcs[VipSendDoneMethod])(ViHandle,DescriptorPtr);
	else
		BAIL
}



inline static VIP_RETURN  __cdecl VipSendWait(
    IN VIP_VI_HANDLE ViHandle,
    IN VIP_ULONG TimeOut,
    OUT VIP_DESCRIPTOR **DescriptorPtr)
{
	if(s_pfnProcs[VipSendWaitMethod] != NULL)
		return ((VipSendWaitProc) s_pfnProcs[VipSendWaitMethod])(ViHandle,TimeOut,DescriptorPtr);
	else
		BAIL
}


typedef void (*VIP_VI_CALLBACK)(
    VIP_PVOID Context, VIP_NIC_HANDLE NicHandle, VIP_VI_HANDLE ViHandle, VIP_DESCRIPTOR *Descriptor);




inline static VIP_RETURN  __cdecl VipPostRecv(
    IN VIP_VI_HANDLE ViHandle,
    IN VIP_DESCRIPTOR *DescriptorPtr,
    IN VIP_MEM_HANDLE MemoryHandle)
{
	if(s_pfnProcs[VipPostRecvMethod] != NULL)
		return ((VipPostRecvProc)s_pfnProcs[VipPostRecvMethod])(ViHandle,DescriptorPtr,MemoryHandle);
	else
		BAIL
}



inline static VIP_RETURN  __cdecl VipRecvDone(
    IN VIP_VI_HANDLE ViHandle,
    OUT VIP_DESCRIPTOR **DescriptorPtr)
{
	if(s_pfnProcs[VipRecvDoneMethod] != NULL)
		return ((VipRecvDoneProc)s_pfnProcs[VipRecvDoneMethod])(ViHandle, DescriptorPtr);
	else
		BAIL
}


#if 0
inline static VIP_RETURN  __cdecl VipRecvWait(
    IN VIP_VI_HANDLE ViHandle,
    IN VIP_ULONG TimeOut,
    OUT VIP_DESCRIPTOR **DescriptorPtr)
{
	if(s_pfnProcs[VipCloseNicMethod] != NULL)
		return s_pfnProcs[VipOpenNicMethodMethod])(NicHandle);
	else
		BAIL
}

inline static VIP_RETURN  __cdecl VipSendNotify(
    IN VIP_VI_HANDLE ViHandle,
    IN VIP_PVOID Context,
    IN VIP_VI_CALLBACK Callback)
{
	if(s_pfnProcs[VipCloseNicMethod] != NULL)
		return s_pfnProcs[VipOpenNicMethodMethod])(NicHandle);
	else
		BAIL
}


inline static VIP_RETURN  __cdecl VipRecvNotify(
    IN VIP_VI_HANDLE ViHandle,
    IN VIP_PVOID Context,
    IN VIP_VI_CALLBACK Callback);
#endif

inline static VIP_RETURN  __cdecl VipConnectWait(
    IN VIP_NIC_HANDLE NicHandle,
    IN VIP_NET_ADDRESS *LocalAddr,
    IN VIP_ULONG Timeout,
    OUT VIP_NET_ADDRESS *RemoteAddr,
    OUT VIP_VI_ATTRIBUTES *RemoteViAttributes,
    OUT VIP_CONN_HANDLE *ConnHandle)
{
	if(s_pfnProcs[VipConnectWaitMethod] != NULL)
		return ((VipConnectWaitProc)s_pfnProcs[VipConnectWaitMethod])(NicHandle,LocalAddr,Timeout,RemoteAddr,RemoteViAttributes,ConnHandle);
	else
		BAIL
}



inline static VIP_RETURN  __cdecl VipConnectAccept(
    IN VIP_CONN_HANDLE ConnHandle,
    IN VIP_VI_HANDLE ViHandle)
{

	return ((VipConnectAcceptProc)s_pfnProcs[VipConnectAcceptMethod])(ConnHandle,ViHandle);
}



inline static VIP_RETURN  __cdecl VipConnectReject(
    IN VIP_CONN_HANDLE ConnHandle)
{
	if(s_pfnProcs[VipConnectRejectMethod] != NULL)
		return ((VipConnectRejectProc)s_pfnProcs[VipConnectRejectMethod])(ConnHandle);
	else
		BAIL
}



inline static VIP_RETURN  __cdecl VipConnectRequest(
    IN VIP_VI_HANDLE ViHandle,
    IN VIP_NET_ADDRESS *LocalAddr,
    IN VIP_NET_ADDRESS *RemoteAddr,
    IN VIP_ULONG Timeout,
    OUT VIP_VI_ATTRIBUTES *RemoteViAttributes)
{
	if(s_pfnProcs[VipConnectRequestMethod] != NULL)
		return ((VipConnectRequestProc)s_pfnProcs[VipConnectRequestMethod])(ViHandle,LocalAddr,RemoteAddr,Timeout,RemoteViAttributes);
	else
		BAIL
}



inline static VIP_RETURN  __cdecl VipDisconnect(
    IN VIP_VI_HANDLE ViHandle)
{
	if(s_pfnProcs[VipDisconnectMethod] != NULL)
		return ((VipDisconnectProc)s_pfnProcs[VipDisconnectMethod])(ViHandle);
	else
		BAIL
}


// 
// Completion Queue primitives
//

inline static VIP_RETURN  __cdecl VipCreateCQ(
    IN VIP_NIC_HANDLE NicHandle,
    IN VIP_ULONG EntryCount,
    OUT VIP_CQ_HANDLE *CQHandle)
{
	if(s_pfnProcs[VipCreateCQMethod] != NULL)
		return ((VipCreateCQProc)s_pfnProcs[VipCreateCQMethod])(NicHandle,EntryCount,CQHandle);
	else
		BAIL
}



inline static VIP_RETURN  __cdecl VipDestroyCQ(
    IN VIP_CQ_HANDLE CQHandle)
{
	if(s_pfnProcs[VipDestroyCQMethod] != NULL)
		return ((VipDestroyCQProc)s_pfnProcs[VipDestroyCQMethod])(CQHandle);
	else
		BAIL
}


#if 0
inline static VIP_RETURN  __cdecl VipResizeCQ(
    IN VIP_CQ_HANDLE CQHandle,
    IN VIP_ULONG EntryCount);
#endif

inline static VIP_RETURN  __cdecl VipCQDone(
    IN VIP_CQ_HANDLE CQHandle,
    OUT VIP_VI_HANDLE *ViHandle,
    OUT VIP_BOOLEAN *RecvQueue)
{
	if(s_pfnProcs[VipCQDoneMethod] != NULL)
		return ((VipCQDoneProc)s_pfnProcs[VipCQDoneMethod])(CQHandle, ViHandle,RecvQueue);
	else
		BAIL
}



inline static VIP_RETURN  __cdecl VipCQWait(
    IN VIP_CQ_HANDLE CQHandle,
    IN VIP_ULONG Timeout,
    OUT VIP_VI_HANDLE *ViHandle,
    OUT VIP_BOOLEAN *RecvQueue)
{
	if(s_pfnProcs[VipCQWaitMethod] != NULL)
		return ((VipCQWaitProc)s_pfnProcs[VipCQWaitMethod])(CQHandle, Timeout,ViHandle,RecvQueue);
	else
		BAIL
}


typedef void (*VIP_CQ_CALLBACK)(
    VIP_PVOID Context, VIP_NIC_HANDLE NicHandle, VIP_VI_HANDLE ViHandle, VIP_BOOLEAN RecvQueue);

#if 0
inline static VIP_RETURN  __cdecl VipCQNotify(
    IN VIP_CQ_HANDLE CqHandle,
    IN VIP_PVOID Context,
    IN VIP_CQ_CALLBACK Callback);
#endif
//
// name service API
//

inline static VIP_RETURN  __cdecl VipNSInit(
    IN VIP_NIC_HANDLE NicHandle,
    IN VIP_PVOID NSInitInfo)
	
{
	if(s_pfnProcs[VipNSInitMethod] != NULL)
		return ((VipNSInitProc)s_pfnProcs[VipNSInitMethod])(NicHandle,NSInitInfo);
	else
		BAIL
}


inline static VIP_RETURN  __cdecl VipNSGetHostByName(
    IN VIP_NIC_HANDLE NicHandle,
    IN VIP_WCHAR *Name,
    OUT VIP_NET_ADDRESS *Address,
    IN VIP_ULONG NameIndex)
{
	if(s_pfnProcs[VipNSGetHostByNameMethod] != NULL)
		return ((VipNSGetHostByNameProc)s_pfnProcs[VipNSGetHostByNameMethod])(NicHandle,Name,Address,NameIndex);
	else
		BAIL
}



inline static VIP_RETURN  __cdecl VipNSGetHostByAddr(
    IN VIP_NIC_HANDLE NicHandle,
    IN VIP_NET_ADDRESS *Address,
    OUT VIP_CHAR *Name,
    IN OUT VIP_ULONG *NameLen)
{
	if(s_pfnProcs[VipNSGetHostByAddrMethod] != NULL)
		return ((VipNSGetHostByAddrProc)s_pfnProcs[VipNSGetHostByAddrMethod])(NicHandle,Address,Name,NameLen);
	else
		BAIL
}



inline static VIP_RETURN  __cdecl VipNSShutdown(
    IN VIP_NIC_HANDLE NicHandle)
{
	if(s_pfnProcs[VipNSShutdownMethod] != NULL)
		return ((VipNSShutdownProc)s_pfnProcs[VipNSShutdownMethod])(NicHandle);
	else
		BAIL
}

#if 0
//
// peer connection API
//

inline static VIP_RETURN  __cdecl VipConnectPeerRequest(
    IN VIP_VI_HANDLE ViHandle,
    IN VIP_NET_ADDRESS *LocalAddr,
    IN VIP_NET_ADDRESS *RemoteAddr,
    IN VIP_ULONG Timeout);


inline static VIP_RETURN  __cdecl VipConnectPeerDone(
    IN VIP_VI_HANDLE ViHandle,
    OUT VIP_VI_ATTRIBUTES *RemoteAttributes);


inline static VIP_RETURN  __cdecl VipConnectPeerWait(
    IN VIP_VI_HANDLE ViHandle,
    OUT VIP_VI_ATTRIBUTES *RemoteViAttributes);

//
// Tag demultiplexing
//

inline static VIP_RETURN  __cdecl VipAddTagCQ(
	IN VIP_CQ_HANDLE CQHandle,
	IN OUT VIP_EVENT_HANDLE *Event,
	IN VIP_ULONG Tag,
	IN VIP_ULONG Priority);


inline static VIP_RETURN  __cdecl VipRemoveTagCQ(
	IN VIP_CQ_HANDLE CQHandle,
	IN VIP_EVENT_HANDLE Event,
	IN VIP_ULONG Tag);

//
// deferred function call
//

inline static VIP_RETURN  __cdecl VipPostDeferredSends(
	IN VIP_VI_HANDLE vihandle, 
	IN VIP_BOOLEAN enableinterrupt,
	IN OUT VIP_BOOLEAN *sendsdeferred);
#endif
//
// management
//

#if 0
inline static VIP_RETURN  __cdecl VipQuerySystemManagementInfo(
    IN VIP_NIC_HANDLE NicHandle,
    IN VIP_ULONG InfoType,
    OUT VIP_PVOID SysManInfo)
{
	if(s_pfnProcs[VipCloseNicMethod] != NULL)
		return s_pfnProcs[VipOpenNicMethodMethod])(NicHandle);
	else
		BAIL
}

inline static VIP_RETURN  __cdecl VipQueryMem(
    IN VIP_NIC_HANDLE NicHandle,
    IN VIP_PVOID VirtualAddress,
    IN VIP_MEM_HANDLE MemHandle,
    OUT VIP_MEM_ATTRIBUTES *MemAttributes)
{
	if(s_pfnProcs[VipCloseNicMethod] != NULL)
		return s_pfnProcs[VipOpenNicMethodMethod])(NicHandle);
	else
		BAIL
}



inline static VIP_RETURN  __cdecl VipSetMemAttributes(
    IN VIP_NIC_HANDLE NicHandle,
    IN VIP_PVOID VirtualAddress,
    IN VIP_MEM_HANDLE MemHandle,
    IN VIP_MEM_ATTRIBUTES *MemAttributes)
{
	if(s_pfnProcs[VipCloseNicMethod] != NULL)
		return s_pfnProcs[VipOpenNicMethodMethod])(NicHandle);
	else
		BAIL
}

#endif

};


#endif

