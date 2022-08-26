//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: sm_shiloh.hpp
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
#ifndef _SM_SHILOH_HPP_
#define _SM_SHILOH_HPP_

// ---------------------------------------------------------------------------------------
// Wrapper stuff
//
// Typedefs
typedef enum { NLOPT_SET_ENCRYPT, 
			   NLOPT_SET_PACKET_SIZE };

typedef struct _OPTSTRUCT
{
    int   iSize;
    BOOL  fEncrypt;
	int   iRequest;
	DWORD dwPacketSize;
} OPTSTRUCT;

// Functions
typedef enum
{
	CONNECTIONOBJECTSIZE    ,
	CONNECTIONOPEN          ,
	CONNECTIONREAD          ,
	CONNECTIONWRITE         ,
	CONNECTIONCLOSE         ,   
	CONNECTIONCHECKFORDATA  ,
	CLNT_NETFUNCS
} ClientFuncs;

typedef USHORT (__cdecl * NLSIZE_FN)( void );
typedef int (__cdecl * NLOPEN_FN)( void *,
                                WCHAR *,
                                LONG UNALIGNED * );
typedef USHORT (__cdecl * NLREAD_FN)( void *,
                                  BYTE *,
                                  int,
                                  int,
                                  USHORT,
                                  LONG UNALIGNED * );
typedef USHORT (__cdecl * NLWRITE_FN)( void *,
                                           BYTE *,
                                           int,
                                           LONG UNALIGNED * );
typedef int (__cdecl * NLCLOSE_FN)( void *,  LONG UNALIGNED * );
typedef int (__cdecl * NLCHECKDATA_FN)( void *,
                                           LONG *,
                                           LONG UNALIGNED * );


typedef struct _NLFunctionTable
{
	NLSIZE_FN Size;
	NLOPEN_FN Open;
	NLREAD_FN Read;
	NLWRITE_FN Write;
	NLCLOSE_FN Close;
	NLCHECKDATA_FN CheckData;
} NLFunctionTable;

// ---------------------------------------------------------------------------------------

class Sm_Shiloh : public SNI_Provider
{
private:	
	void           * m_pConnObj;
	
public:
	Sm_Shiloh(SNI_Conn * pConn);
	~Sm_Shiloh();	

	static DWORD Initialize(PSNI_PROVIDER_INFO pInfo);
	static DWORD Terminate();
	static DWORD InitializeListener(HANDLE hSNIAcceptKey, void * szListenInfo, HANDLE * pListenHandle);
	static DWORD TerminateListener(HANDLE hListener);

	static DWORD Open( 	SNI_Conn 		* pConn,
							ProtElem 		* pProtElem, 
							__out SNI_Provider 	** ppProv );

	DWORD ReadSync(__out SNI_Packet ** ppNewPacket, int timeout);
	DWORD ReadAsync(SNI_Packet ** ppNewPacket, LPVOID pPacketKey);
	DWORD WriteSync(__in SNI_Packet * pPacket, SNI_ProvInfo * pProvInfo);
	DWORD WriteAsync(SNI_Packet * pPacket, SNI_ProvInfo * pProvInfo);
	DWORD GatherWriteAsync(SNI_Packet * pPacket, SNI_ProvInfo * pProvInfo);
	DWORD ReadDone(SNI_Packet ** ppPacket, __out SNI_Packet **ppLeftOver, DWORD dwBytes, DWORD dwError);
	DWORD WriteDone(SNI_Packet ** ppPacket, DWORD dwBytes, DWORD dwError);	
	DWORD Close();
	static DWORD AcceptDone( SNI_Conn * pConn, 
								 LPVOID pAcceptKey,
								 DWORD dwBytes,
								 DWORD dwError,
						   		 SNI_Provider ** ppProv,
						   		 LPVOID * ppAcceptInfo );
	void Release();

private:
	static DWORD LoadDll(__out NLFunctionTable * pFuncs);
};

#endif

