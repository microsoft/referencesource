//*********************************************************************
//		Copyright (c) Microsoft Corporation.
//
// @File: viahelper.cpp
// @Owner: nantu, petergv
// @Test: milu
//
// <owner current="true" primary="true">nantu</owner>
// <owner current="true" primary="false">petergv</owner>
//
// Purpose: 
//		Implement helper functions and common thread routines for VIA provider
//
// Notes:
//	
// @EndHeader@
//****************************************************************************	

#include "snipch.hpp"
#include "via.hpp"

#ifdef SNI_BASED_CLIENT
#ifndef SNIX
#include "nlregc.h"
#endif
#else
#include "nlregs.h"
#endif

// !!! Important: the bit specified below must match BIDX_APIGROUP_VIA_DESC
// defined in sni_common.hpp.  
//
BID_METATEXT( _T("<ApiGroup|SNI|VIA_DESC> 0x00002000: SNI VIA HEADER (DESCRIPTOR INFO)"));
//
//DEVNOTE: When use this macro, make sure pDesc is valid. Also since pDesc->Data[0].Data.Address is 
//initialized when pDesc is initialized in get_BufDescripter(). So it is ok to dereference them here. 
// If the pDesc is changed,  dev need to reexaming the assumption to make sure deref is ok. !!!!
//

#define SNI_BID_TRACE_VIADESC(szText, pDesc)	\
{\
		BOOL bEna;\
		IF_BidEnabledU( bEna, BidAreOn( BID_APIGROUP_TRACE | BIDX_APIGROUP_VIA_DESC),\
					SNI_VIA_DESC_TAG)\
		{\
			Via* pVia = GetConnObj(pDesc);\
			SNI_Packet* pPacket = DescGetPacket(pDesc);						\
			BidTrace8(SNI_VIA_DESC_TAG _T("\n\t%hs:") 							\
										_T(" pDesc:%p{VIP_DESCRIPTOR*},") 	\
										_T(" status:%d,")						\
										_T(" pVia:%p{Via*},")					\
										_T(" pPacket:%p{SNI_Packet*},")		\
										_T(" Length:%d,")						\
										_T(" SeqNum:%d,")					\
										_T(" SendLimit:%d\n"),					\
										szText,								\
										pDesc, 								\
										pDesc->Control.Status, 				\
										pVia, 								\
										pPacket,								\
										pDesc->Control.Length, 				\
										*((LONG *)pDesc->Data[0].Data.Address), 	\
										*((LONG *)((DWORD *)(pDesc->Data[0].Data.Address) + 1)));	\
		}	\
}


// Global Vars
DWORD gdwSvrNic = 0;
DWORD gdwSvrPortNum = 1433;


FARPROC Via::s_pfnProcs[] ={NULL};


char *Via::s_szProcNames[] = 	{
		"VipOpenNic",
		"VipCloseNic",
		"VipQueryNic",
		"VipRegisterMem",
		"VipDeregisterMem",
		"VipErrorCallback",
		"VipCreatePtag",
		"VipDestroyPtag",
		"VipCreateVi",
		"VipDestroyVi",
		"VipQueryVi",
		"VipSetViAttributes",
		"VipPostSend",
		"VipSendDone",
		"VipSendWait",
		"VipPostRecv",
		"VipRecvWait",
		"VipRecvDone",
		"VipConnectWait",
		"VipConnectAccept",
		"VipConnectReject",
		"VipConnectRequest",
		"VipDisconnect",
		"VipCreateCQ",
		"VipDestroyCQ",
		"VipCQDone",
		"VipCQWait",
		"VipNSInit",
		"VipNSGetHostByName",
		"VipNSGetHostByAddr",
		"VipNSShutdown"
	} ;

// Functions
// Retrieve ConnObj pointer from Desc
inline Via * GetConnObj(VIP_DESCRIPTOR * pDesc)
{	
	DWORD_PTR * dwPtr = (DWORD_PTR *)((BYTE *)pDesc + 64);

	return (Via *)*dwPtr;
}

// set ConnObj pointer from Desc
inline void SetConnObj(VIP_DESCRIPTOR * pDesc, LPVOID pConn)
{
	DWORD_PTR * dwPtr = (DWORD_PTR *)((BYTE *)pDesc + 64);
	*dwPtr = (DWORD_PTR)pConn;
}

// Retrieve Desc pointer from packet
inline VIP_DESCRIPTOR * BufGetDesc(__in const BYTE * pbBuf)
{
	VIP_DESCRIPTOR * pDesc = (VIP_DESCRIPTOR *) ((BYTE *)(pbBuf - PKT_HDR - 128));

	return pDesc;
}

// Retrieve packet pointer from Desc
inline SNI_Packet *DescGetPacket(VIP_DESCRIPTOR * pDesc)
{
	DWORD_PTR * dwPtr = (DWORD_PTR *)((BYTE *)pDesc + 64+sizeof(DWORD_PTR));

	return (SNI_Packet *)*dwPtr;
}


// Set packet pointer in Desc
inline void DescSetPacket(VIP_DESCRIPTOR * pDesc,SNI_Packet * pPacket)
{
	DWORD_PTR * dwPtr = (DWORD_PTR *)((BYTE *)pDesc + 64+sizeof(DWORD_PTR));

	*dwPtr = (DWORD_PTR)pPacket;
}

// Retrieve fragment info from Desc
inline DWORD DescGetFragInfo(VIP_DESCRIPTOR * pDesc)
{
	DWORD_PTR * dwPtr = (DWORD_PTR *)((BYTE *)pDesc + 64+2*sizeof(DWORD_PTR));

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("Frag: %d\n"), *dwPtr);
	
	return (DWORD)*dwPtr;
}

// Set fragment info in Desc
inline void DescSetFragInfo(VIP_DESCRIPTOR * pDesc, DWORD dwVal)
{
	DWORD_PTR * dwPtr = (DWORD_PTR *)((BYTE *)pDesc + 64+2*sizeof(DWORD_PTR));

	*dwPtr = dwVal;
}

// Retrieve OrigBuf pointer from Desc
inline BYTE *DescGetBuf(VIP_DESCRIPTOR * pDesc)
{
	return (BYTE *)pDesc->Data[0].Data.Address + PKT_HDR;
}

// Set OrigBuf pointer in Desc
inline void DescSetOrigBuf(VIP_DESCRIPTOR * pDesc, BYTE * pbBuf)
{
	DWORD_PTR * dwPtr = (DWORD_PTR *)((BYTE *)pDesc + 64+2*sizeof(DWORD_PTR));

	*dwPtr = (DWORD_PTR)pbBuf;
}


DWORD Hash(__in LPCWSTR string)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "string: \"%s\"\n"), string);

	DWORD i;
	DWORD dwHash  = 0;
	WCHAR c;

	for(i = 0; i < (DWORD) wcslen(string); i++)
	{
		c = string[i];
		dwHash += c;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("Hash: %d\n"), dwHash);

	return dwHash;
}

//Taken the liberty of changing the signature to return BYTE.
//All usages seem to be expecting BYTE
BYTE HexStrtoByte(__inout LPSTR szStr)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "szStr: \"%hs\"\n"), szStr);

	int retVal = 0;
	int i;
	int cszStr;

	DWORD len = (DWORD) strlen(szStr);
OACR_WARNING_PUSH
OACR_WARNING_DISABLE(SYSTEM_LOCALE_MISUSE , " INTERNATIONALIZATION BASELINE AT KATMAI RTM. FUTURE ANALYSIS INTENDED. ")
	DWORD cch = LCMapStringA(LOCALE_SYSTEM_DEFAULT,
							LCMAP_LOWERCASE,
							szStr,
							-1,
							szStr,
							len+1
							);
OACR_WARNING_POP
	Assert(cch <= len+1);
	if( 0 >= cch || cch > len+1)
	{
		retVal = ERROR_INVALID_DATA;
		SNI_SET_LAST_ERROR(VIA_PROV, SNIE_4, retVal);
		return (BYTE)retVal;
	}		
	szStr[len] = 0;

	cszStr = (int)strlen(szStr) - 1;
	for(i = 0; i < cszStr; i++)
	{
		retVal += ((szStr[i] <= '9') ? (szStr[i] - '0') : (szStr[i] - 'A' + 0xA)) \
				* (16 << (4*(cszStr - i - 1)));
	}
	retVal += (szStr[i] <= '9') ? (szStr[i] - '0') : (szStr[i] - 'A' + 0xA);
	Assert(retVal <= 255);

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("Byte: %d\n"), retVal);

	return (BYTE)retVal;
}


VIPERRORMAP g_rgVipMap[] = 
{
	VIP_SUCCESS,					ERROR_SUCCESS, 
	VIP_NOT_DONE, 					ERROR_FAIL,
	VIP_INVALID_PARAMETER,			ERROR_INVALID_PARAMETER,
	VIP_ERROR_RESOURCE,			ERROR_OUTOFMEMORY,

	VIP_TIMEOUT,						WAIT_TIMEOUT,
	VIP_REJECT,						ERROR_FAIL,
	VIP_INVALID_RELIABILITY_LEVEL,	ERROR_FAIL,
	VIP_INVALID_MTU,					ERROR_FAIL,

	VIP_INVALID_QOS,				ERROR_FAIL,
	VIP_INVALID_PTAG,ERROR_FAIL,
	VIP_INVALID_RDMAREAD,			ERROR_FAIL,
	VIP_DESCRIPTOR_ERROR,			ERROR_FAIL,

	VIP_INVALID_STATE,				ERROR_INVALID_STATE,
	VIP_ERROR_NAMESERVICE,			ERROR_FAIL,
	VIP_NO_MATCH,					ERROR_FAIL,
	VIP_NOT_REACHABLE,				ERROR_FAIL,

	VIP_ERROR_NOT_SUPPORTED,		ERROR_NOT_SUPPORTED

};

DWORD MapVipErrorToSniError(VIP_RETURN vipStatus)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "vipStatus: %d{VIP_RETURN}\n"), vipStatus);

	return (sizeof(g_rgVipMap)/sizeof(VIPERRORMAP)> vipStatus)? g_rgVipMap[vipStatus].dwSni : g_rgVipMap[0].dwSni;
}

BOOL Via::InitQLogicNameService(VIP_NIC_HANDLE hNic)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "hNic: %p{VIP_NIC_HANDLE}\n"), hNic);

	char *szTemp = "%windir%\\system32\\drivers\\etc\\VIHOSTS";
	char szInit[MAX_PATH+1];
	char *pszHostFile = szInit;

	DWORD dwLen = MAX_PATH+1;
	bool fFree = false;
	BOOL bReturn = FALSE;
	DWORD temp = 0;

	// Initialize VIPL Name Servive
	if(0 == (dwLen = ExpandEnvironmentStringsA(szTemp, pszHostFile, dwLen)))
	{
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), FALSE);
	
		return FALSE;
	}
	if(dwLen > MAX_PATH+1)
	{
		if(NULL == (pszHostFile = (char *)NewNoX (gpmo) char[dwLen]))
		{
			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), FALSE);
	
			return FALSE;
		}
		fFree = true;
		
		if(0 == (temp = ExpandEnvironmentStringsA(szTemp, pszHostFile, dwLen)))
			goto ErrorExit;
		//if the env string changed between the two Expand* calls
		if(temp > dwLen)
			goto ErrorExit;
	}

	if (VIP_SUCCESS != VipNSInit(hNic, (VIP_PVOID)pszHostFile))
		goto ErrorExit;
	
	bReturn = TRUE;

ErrorExit:
	if(fFree)
		delete []pszHostFile;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), bReturn);
	
	return bReturn;

}

DWORD Via::CompleteRead(VIP_DESCRIPTOR * pDesc, SNI_Packet * pPacket, int timeout)
{
	BidxScopeAutoSNI4( SNIAPI_TAG _T("%u#, ")
							  _T("pDesc: %p{VIP_DESCRIPTOR*}, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("timeout: %d\n"), 
							  GetBidId(),
							  pDesc, 
							  pPacket, 
							  timeout);

	DWORD dwError = ERROR_SUCCESS;
	VIP_RETURN status;

	//Get the payload 
	// Complete Read is called for data packet only, in which case,
	// Control.Length should have been already checked for longer than PKT_HDR.
	Assert ( pDesc->Control.Length > PKT_HDR );
	DWORD dwBytesRead = pDesc->Control.Length - PKT_HDR;

	// Check if payload size is bigger than readmax
	if( dwBytesRead + SNIPacketGetBufferSize(pPacket) > SNIPacketGetBufActualSize(pPacket) )
	{
		dwError = ERROR_INVALID_DATA;
		SNI_SET_LAST_ERROR(VIA_PROV, SNIE_4, dwError);
		goto ErrorExit;
	}

	// Append the data to the end of the packet
	SNIPacketAppendData(pPacket, DescGetBuf(pDesc), dwBytesRead);

	// Post a read on the buf's Desc
	if( (status = VipPostRecv(m_hVi, pDesc, m_hMem)) != VIP_SUCCESS )
	{
		dwError = MapVipErrorToSniError(status);
		SNI_SET_LAST_ERROR(VIA_PROV, SNIE_4, dwError);
		goto ErrorExit;
	}

	m_cRecvs++;

	// Bump up server's SendLimit
	SendLimit++;

	// Send an Ack - if we need to AND if possible
	if( ERROR_SUCCESS != (dwError = SendAck(m_pConn->m_fSync ? VIP_INFINITE : 0)) )
		goto ErrorExit;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

	return ERROR_SUCCESS;

ErrorExit:

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);

	return dwError;
}

// Allocates and formats a list of descriptors.
VIP_DESCRIPTOR *
Via::get_BufDescriptors( VIP_NIC_HANDLE nic, VIP_PROTECTION_HANDLE Ptag, ULONG num, ULONG buflen,
				    VIP_MEM_HANDLE *mh, __out void **ptr, 
                    LPVOID pConn,
                    VIP_RETURN * pdwNetError)
{
	BidxScopeAutoSNI8( SNIAPI_TAG _T( "nic: %p{VIP_NIC_HANDLE}, Ptag: %p{VIP_PROTECTION_HANDLE}, num: %u, buflen: %u, mh: %p{VIP_MEM_HANDLE*}, ptr: %p{void**}, pConn: %p, pdwNetError: %p{VIP_RETURN*}\n"), 
					 nic, Ptag, num, buflen, mh, ptr, pConn, pdwNetError);

	ULONG i, len, buflen_aligned;
	VIP_RETURN status = VIP_SUCCESS;
	char *p;
	VIP_DESCRIPTOR * retDesc, *d;


	VIP_MEM_ATTRIBUTES memAttr;

	memAttr.Ptag = Ptag;
	memAttr.EnableRdmaWrite = FALSE;
	memAttr.EnableRdmaRead = FALSE;

	// Descriptors and buffers are allocated contiguously, with the data
	// buffer for a descriptor immediately following the descriptor
	// itself.  When allocating memory we have to allow for alignment
	// losses on the first descriptor (up to 64 bytes), plus alignment
	// losses on each additional descriptor if 'buflen' is not a
	// multiple of 64 bytes. 

	// Bump up buf sizes to 64 byte boundaries
	buflen_aligned = ((buflen+63) & ~63);   

	// Bump up len by extra 64 to store ConnObj ptr. 
	//Also, when Write/Read are called on this, we would like to associate the packet ptr with the descriptor so events can be fired with the right packet
	//rounding approx (64)+total number requested(num) * sizeof each buffer aligned on 64bytes(buflen_aligned) + header
	//header = 64(descriptor) + Miscelaneous
	// 0-8 = connobj
	// 9-16 = packet ptr. 
	if(FAILED(ULongAdd(buflen_aligned, 128, &len)) ||
	   FAILED(ULongMult(len, num, &len)) ||
	   FAILED(ULongAdd(len, 64, &len)))
	{
		*pdwNetError = VIP_INVALID_PARAMETER;
		goto ErrorExit;
	}
	p = (char *)NewNoX(gpmo) BYTE[len];

	if( p == NULL )
	{
		*pdwNetError = VIP_ERROR_RESOURCE;
		goto ErrorExit;
	}

	ZeroMemory(p,len);
	*ptr = p;

	if( (status = VipRegisterMem(nic, p, len, &memAttr, mh)) != VIP_SUCCESS )
	{
		delete  []p;
		p = NULL;
		*pdwNetError = status;
		goto ErrorExit;
	}

	// Align the start pointer, and start carving out descriptors and
	// buffers.  Link them through the Next field.
	p = (char*)((((DWORD_PTR)p) + 63) & ~63); /* 64-byte aligned */
	retDesc = 0;

	for (i = 0; i < num; i++) 
	{
		d = (VIP_DESCRIPTOR*)p;
		d->Control.Next.Address = retDesc;
		retDesc = d;
		d->Control.SegCount = 1;
		d->Control.Control = 0;
		d->Control.Length = buflen;

		SetConnObj(d, pConn);

		p += 128;
		d->Data[0].Handle = *mh;
		d->Data[0].Length = buflen;
		d->Data[0].Data.Address = p;
		p += buflen_aligned;

	}

	// Return the list of allocated descriptors

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("Desc: %p\n"), retDesc);

	return retDesc;

ErrorExit:

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("Desc: %p\n"), 0);

	return NULL;
}

// Send the packet
DWORD Via::SendDescriptor(VIP_DESCRIPTOR *pDesc, DWORD cbData, int timeout, BOOL fOobAck)
{
	BidxScopeAutoSNI5( SNIAPI_TAG _T("%u#, ")
							  _T("pDesc: %p{VIP_DESCRIPTOR*}, ")
							  _T("cbData: %d, ")
							  _T("timeout: %d, ")
							  _T("fOobAck: %d\n"), 
							  GetBidId(),
							  pDesc, 
							  cbData, 
							  timeout,
							  fOobAck);

	VIP_RETURN status = VIP_SUCCESS;
	DWORD dwRet = ERROR_SUCCESS;

	Assert(SeqNum < DestSendLimit);

	// OobAck uses reserved slot, so do not increment SeqNum. SolicteAck is used to update SendLimit.
	if( !fOobAck)	SeqNum++;

	LastSendLimit = SendLimit;	
	
	memcpy((BYTE *)pDesc->Data[0].Data.Address, (DWORD *)&SeqNum, sizeof(DWORD));
	memcpy((BYTE *)(pDesc->Data[0].Data.Address) + sizeof(DWORD), (DWORD *)&LastSendLimit, sizeof(DWORD));

	pDesc->Control.Length = cbData + PKT_HDR;
	pDesc->Control.Control = VIP_CONTROL_OP_SENDRECV | VIP_CONTROL_IMMEDIATE;
	pDesc->Data[0].Length = pDesc->Control.Length;

	SNI_BID_TRACE_VIADESC("To send", pDesc);	

	if( (status = VipPostSend(m_hVi, pDesc, m_hMem)) != VIP_SUCCESS)
	{
		dwRet = MapVipErrorToSniError(status);
		goto ErrorExit;
	}

	
	if( m_pConn->m_fSync )
	{
		if( (status = VipSendWait(m_hVi, timeout, &pDesc)) != VIP_SUCCESS)
		{
			dwRet = MapVipErrorToSniError(status);
			goto ErrorExit;
		}
	}
	else
	{
		Assert(0 == timeout);
		dwRet = ERROR_IO_PENDING;
	}

        BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

	return dwRet;

ErrorExit:

	SNI_SET_LAST_ERROR(VIA_PROV, SNIE_4, dwRet);
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;
	
}

BOOL Via::IsViClean()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	// Query state of Vi - note this is precautionary
	// at this point
	VIP_VI_STATE State = VIP_STATE_ERROR;
	VIP_VI_ATTRIBUTES Attribs={0};
	VIP_RETURN status = VIP_SUCCESS;
	VIP_BOOLEAN fRecv = VIP_FALSE;
	VIP_BOOLEAN fSend = VIP_FALSE;
	BOOL fIsClean = FALSE;

	status = VipQueryVi(m_hVi, &State, &Attribs, &fSend, &fRecv);

	if( (fIsClean = ((State == VIP_STATE_IDLE) && fSend && fRecv)) )
	{
		// Assert fClean is NOT TRUE. Since we just popped out of a Desc desc.,
		// the Vi couldn't have been clean before
		Assert( fClean != TRUE );

		Assert(m_cRecvs == 0);

		// Now, set it to be clean
		fClean = TRUE;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), fIsClean);

	return fIsClean;
}

//Do not return any error to keep compthread going.
void Via::HandleSendEvent(VIP_VI_HANDLE hVi)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "hVi: %p{VIP_NIC_HANDLE}\n"), hVi);

	VIP_RETURN status;
	VIP_DESCRIPTOR *pDesc = NULL;
	Via * pVia = 0;
	DWORD dwError = ERROR_SUCCESS;
	BOOL fCleanup = FALSE;
	SNI_Packet * pPacket = 0;
	BOOL fWriteDone = FALSE;
	DWORD fFrag = 0;


	// Remove first completed pDesc from send queue.
	status = VipSendDone(hVi, &pDesc);

	if( !pDesc )
	{
		//According to VIA doc, VipSendDone can return status other than 
		//VIP_SUCCESS, but still return valid pDesc posted.Only when recv/send 
		//queue is empty, pDesc is set to NULL. This can only happen when 
		//multiple threads in one process poll the queues without synchrounization. 
		//In our model, this should not happen. Thus, treat it as a coding or something
		//unexpected. No clean up of Via memory is possible since the via object is 
		//embeded in pDesc. Close the Vi handle and return.
		VipDisconnect(hVi);
		BidTrace0( ERROR_TAG _T("VipSendDone failed\n") );
		return;
	}

	SNI_BID_TRACE_VIADESC("On send", pDesc);
										
	// Get connection pVia, pPacket, flag associated with current via descriptor. In fuzer 
	// case (error of the wire), all pointers might be zeroed out. Handle them first
	
	pVia = GetConnObj(pDesc);
	if( pVia == NULL )
	{
		//pVia is a pointer embeded in pDesc to help track connection. When pDesc 
		//is valid, the point should be valid too. If pVia == NULL. it means something 
		//unexpected had happened. No clean up of Via memory is possible since the 
		//via object is embeded in pDesc. Close the Vi handle and return.
		SNI_ASSERT_ON_INVALID_PACKET;		
		VipDisconnect(hVi);
		BidTrace0( ERROR_TAG _T("Unexpected descriptor content\n") );
		return;
	}

	pPacket = DescGetPacket(pDesc);
	fFrag = DescGetFragInfo(pDesc);	

	//If it is valid data packet or error, post a writedone to upper layer consumer. If it is ack packet
	//doing nothing. 
	
	CAutoSNICritSec a_csVia( pVia->CritSec, SNI_AUTOCS_DO_NOT_ENTER );
	a_csVia.Enter();
	// Check if this is a Data packet
	if ( VIP_SUCCESS == status )
	{
		if( pDesc->Control.Length > PKT_HDR )
		{
			//A valid data packet. Check if this connection was previously closed and it is clean
			Assert ( pPacket != NULL );
			if( pVia->fClosed || pVia->fDisconnected )
			fCleanup = pVia->IsViClean();
		}
		else if ( pDesc->Control.Length == PKT_HDR && pPacket == NULL )
		{
			//A valid ack packet is an internal flow control packet. No need to notify consumer.
			if( pVia->m_fPendingAck ) 
			{
				pVia->m_fPendingAck = FALSE;
				
				// See if we need to send ack again since.
				if( ERROR_SUCCESS != (dwError = pVia->SendAck( 0)) )
				{
					BidTrace1( ERROR_TAG _T("%d{WINERR}\n"), dwError);
					// Set flag to closed
					pVia->fClosed = TRUE;
					// Disconnect Vi here - if it's not been done earlier in Close
					if( pVia->fDisconnected == FALSE )
					{
						VipDisconnect(hVi);
						pVia->fDisconnected = TRUE;
					}

					// Check if this Object is clean
					fCleanup = pVia->IsViClean();    		
					
				}
				
			}
			
		}
		else
		{
			//At sending side, this should not happen under normal circumstance
			SNI_ASSERT_ON_INVALID_PACKET;		

			// We do not mark the pointer to null to allow writedone notification to user.
			// By marking dwError as VIP_DESCRIPTOR_ERROR, consumer should 
			// cleanup resources.
			
			if (status == VIP_SUCCESS ) dwError = VIP_DESCRIPTOR_ERROR;

			BidTrace0( ERROR_TAG _T("Unexpected descriptor content\n") );

			// Set flag to closed
			pVia->fClosed = TRUE;

			// Disconnect Vi here - if it's not been done earlier in Close
			if( pVia->fDisconnected == FALSE )
			{
				VipDisconnect(hVi);
				pVia->fDisconnected = TRUE;
			}

			// Check if this Object is clean
			fCleanup = pVia->IsViClean();    		
		}
	}
	else
	{
		// An error happened. Set flag to closed. Do not mark the pPacket pointer
		// to null explicitly to allow writedone notification to user to allow cleanup, 
		// mark dwError as VIP_DESCRIPTOR_ERROR. Upper layer consumer should
		// cleanup resources as needed in this case.

		pVia->fClosed = TRUE;
		// Disconnect Vi here - if its not been done earlier in Close
		if( pVia->fDisconnected == FALSE )
		{
			pVia->fDisconnected = TRUE;
			VipDisconnect(hVi);
		}

		// Check if this Object is clean
		fCleanup = pVia->IsViClean();    		

		dwError = MapVipErrorToSniError(status);
		if ( ERROR_SUCCESS == dwError )
			dwError = ERROR_CONNECTION_ABORTED;

	}

	a_csVia.Leave(); 
	
	// Post a writedone to consumer only when a complete packet is send out or there is an error.
	if( pPacket && ( !fFrag || ERROR_SUCCESS != dwError ))
		PostWriteDone(pPacket, dwError);
	
	if( fCleanup )
		pVia->Cleanup();
}

void Via::HandleReceiveEvent(VIP_VI_HANDLE hVi, __out bool *pfQuit)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T( "hVi: %p{VIP_NIC_HANDLE}, pfQuit: %p{bool*}\n"), hVi, pfQuit);

	VIP_RETURN status = VIP_SUCCESS;
	VIP_DESCRIPTOR *pDesc = NULL;
	DWORD dwNetError = ERROR_SUCCESS;
	SNI_Packet *pPacket = NULL;
	Via * pVia = 0;
	BOOL fCleanup = FALSE;

	*pfQuit = false;
		
#ifdef WIN64
		if((DWORD_PTR)pVia == 0xBAADBAADBAADBAAD)
#else
		if((DWORD_PTR)pVia == 0xBAADBAAD)
#endif
	{
		//This is something related to shiloh.
		Assert( 0 && " Via provider is in an unknown state\n" );
		BidTrace0( ERROR_TAG _T("Via provider is in an unknown state\n") );
		*pfQuit = true;
		return;
	}

	// Read done event :
	// 0 Perform sanity check.
	// 1. Verify msg. sequence is in order
	// 2. Update send limit. 
	// 3. If there is data other than Ack, i.e a valid data payload,    
	//        3.a If ReadAsync has been posted earlier, post a read to consumer only if it is
	//			a completed packet (not frag), or there is error.
	//        3.b If no Read(A)sync posted,  indicate that there is stuff to be read by enqueueing
	//		a desc. Read(a)sync will first check check it.
	// 4. If send limit open up windows to send another packet, do so.
	// 5. In error case, close connection, clean up resource and notify upper layer consumer.

	status = VipRecvDone(hVi, &pDesc);

	if( !pDesc )
	{
		//According to VIA doc, VipSendDone can return status other
		//than VIP_SUCCESS, but still return valid pDesc posted before.
		//If the send queue is empty, pDesc is set to NULL. This can
		//only happen when multiple threads in one process, polling the queue without
		//synchrounization. In our model, this should not happen. Thus, we treat 
		//it as a coding or something unexpected.
		//
		//We can't do Via memory clean up here already, so close the Vi handle
		//directly and return. Application might stop responding!
		
		VipDisconnect(hVi);
		BidTrace0( ERROR_TAG _T("VipRecvDone failed\n") );
		return;
	}

	SNI_BID_TRACE_VIADESC("On receive", pDesc);


	// Get connection associated with hVi
	pVia = GetConnObj(pDesc);

	if( !pVia )
	{
		//This is a pointer we put into pDesc to help track connection. 
		//when pDesc is valid, the point should be valid too. Thus the error here means
		//something unexpected hapened.
		//we can't do Via memory clean up here already, so close the Vi handle
		//directly and return. Application might stop responding!
		SNI_ASSERT_ON_INVALID_PACKET;		
		VipDisconnect(hVi);
		BidTrace0( ERROR_TAG _T("Unexpected descriptor content\n") );
		return;
	}

	CAutoSNICritSec a_csVia( pVia->CritSec, SNI_AUTOCS_DO_NOT_ENTER );

	a_csVia.Enter();

	//A valid desc popped up from the vi recv cQ, decrement recv desc posted.
	pVia->m_cRecvs--;

	if( (status != VIP_SUCCESS) || 
		( VIP_STATUS_DESC_FLUSHED_ERROR == 
		(pDesc->Control.Status & VIP_STATUS_DESC_FLUSHED_ERROR)) )
	{
		// Error case and close case

		//Check if this was due to Vi being closed
		if( VIP_STATUS_DESC_FLUSHED_ERROR != 
			(pDesc->Control.Status & VIP_STATUS_DESC_FLUSHED_ERROR) )
		{			
			//This means VipRecvDone failed for reason other than being closed, 
			// in which case VIP_STATUS_DESC_FLUSHED_ERROR should be returned.
			//Trace it and close the connection.
			BidTrace1( ERROR_TAG _T("ViaRecvDone failed with %d\n"), status);
		}
	
		// Set the flag to closed
		pVia->fClosed = TRUE;

		// Set event to release anyone stuck in read/write
		// Note: we are doing this multiple times, but that should be okay
		SetEvent(pVia->hWriteEvent);
		SetEvent(pVia->hSyncReadEvent);

		// Disconnect Vi here - if its not been done earlier in Close
		if( pVia->fDisconnected == FALSE )
		{
			VipDisconnect(hVi);
			pVia->fDisconnected = TRUE;
		}

		// Check if this Object is clean
		fCleanup = pVia->IsViClean();                

		dwNetError = MapVipErrorToSniError(status);
		if ( ERROR_SUCCESS == dwNetError )
			dwNetError = ERROR_CONNECTION_ABORTED;

		SNI_SET_LAST_ERROR(VIA_PROV, SNIE_4, dwNetError);
		
		goto Exit;
	}

	// If the connection has been closed by upper layer consumer
	if( pVia->fConnClosed == TRUE )
	{
		// Check if this Object is clean
		fCleanup = pVia->IsViClean();

		dwNetError = ERROR_CONNECTION_ABORTED;

		SNI_SET_LAST_ERROR(VIA_PROV, SNIE_4, dwNetError);
		
		goto Exit;
	}

	// A valid pDesc is poped up, verify if data is valid.
	// If the data in descriptor is too long or too short - drop it and disconnect
	if( pDesc->Control.Length > PAYLOAD_LENGTH+PKT_HDR+1 
		|| pDesc->Control.Length < PKT_HDR )
	{
		SNI_ASSERT_ON_INVALID_PACKET;		
		BidTrace0( ERROR_TAG _T("Via system error: Unexpected packet length\n") );				
		dwNetError = ERROR_INVALID_DATA;
		SNI_SET_LAST_ERROR(VIA_PROV, SNIE_4, dwNetError);
		goto ErrorExit;  // Handle disconnection and clean up
	}

	// Check to see if its a fragment - if so, fix the length and set the frag info
	if( pDesc->Control.Length <= PAYLOAD_LENGTH+PKT_HDR )
		DescSetFragInfo(pDesc, 0);
	else
	{
		DescSetFragInfo(pDesc, 1);
		--pDesc->Control.Length;
	}


	LONG * DestSeqNum = (LONG *)pDesc->Data[0].Data.Address;
	LONG * DestSendLimit = (LONG *)((DWORD *)(pDesc->Data[0].Data.Address) + 1);

	// Ensure that seq. number of msg. received is valid
	if( (*DestSeqNum >= pVia->SendLimit )
		||(pDesc->Control.Length != PKT_HDR && *DestSeqNum != pVia->DestSeqNum + 1)
		||(pDesc->Control.Length == PKT_HDR && *DestSeqNum != pVia->DestSeqNum + 1 && *DestSeqNum != pVia->DestSeqNum ))
	{
		SNI_ASSERT_ON_INVALID_PACKET;	
		BidTrace1( ERROR_TAG _T("Via system error: Unexpected DestSeqNum %ld\n"), DestSeqNum);				
		dwNetError = VIANL_E_SEQNUM;
		SNI_SET_LAST_ERROR(VIA_PROV, SNIE_4, dwNetError);
		
		goto ErrorExit; // Handle disconnection and clean up
	}

	// A out of band ACK is a Ack with same SeqNum as last received packet, it is used to update DestSendLimit only.
	bool fOobtAck = ( pVia->DestSeqNum == *DestSeqNum && pDesc->Control.Length == PKT_HDR );

	// Update recd. seq num and dest's send limit from header
	pVia->DestSeqNum        = *DestSeqNum;
	pVia->DestSendLimit = *DestSendLimit;

	Assert ( pDesc->Control.Length >= PKT_HDR );
	// Now, handle the the valid packet.
	// Check if its an Ack packet
	if(pDesc->Control.Length == PKT_HDR )
	{
		pVia->m_dwAcksRecd++;

		//simply post the desc  back for another read
		if( (status = VipPostRecv(hVi, pDesc, pVia->m_hMem)) != VIP_SUCCESS )
		{
			dwNetError = MapVipErrorToSniError(status);

			SNI_SET_LAST_ERROR(VIA_PROV, SNIE_4, dwNetError);
			
			goto ErrorExit; // Handle disconnection and clean up
		}

		pVia->m_cRecvs++;

		if(!fOobtAck ) // Out of band ack use reserved slot, so do not open window for it.
			pVia->SendLimit++;

		Assert( pVia->SeqNum + 1 < pVia->DestSendLimit );
	}

	// Data packet
	else
	{
		// Async
		if( !pVia->m_pConn->m_fSync )
		{
			// Check if a read has been posted

			if( pVia->pReadKeyQ.IsEmpty() )
				dwNetError = pVia->pReadDoneQ.EnQueue((HANDLE) pDesc);

			// Read posted
			else
			{
				DWORD dwFrag = DescGetFragInfo(pDesc);
				
				// If its a fragment, just peek at the packet and copy the data over
				if( dwFrag )
					pPacket = (SNI_Packet *) pVia->pReadKeyQ.Peek();

				// If its not a fragment, dequeue the packet
				else
					pPacket = (SNI_Packet *) pVia->pReadKeyQ.DeQueue();

				dwNetError = 
				pVia->CompleteRead(pDesc, pPacket, 0);

				Assert(dwNetError != ERROR_IO_PENDING);

				// If there was an error and it was a frag, dequeue the packet
				// since we'll be calling back with an error
				if( (ERROR_SUCCESS != dwNetError) && dwFrag )
					pPacket = (SNI_Packet *) pVia->pReadKeyQ.DeQueue();
					
				if( !dwFrag || (ERROR_SUCCESS != dwNetError) )
				{
					a_csVia.Leave(); 
					
					PostReadDone(pPacket, dwNetError);
					a_csVia.Enter();
				}
			}
		}

		// Sync
		else
		{
			dwNetError = pVia->pReadDoneQ.EnQueue((HANDLE) pDesc);

			if( pVia->m_fReadPosted )
			{
				pVia->m_fReadPosted = FALSE;
				SetEvent(pVia->hSyncReadEvent);
			}
		}

	}

	// Sync - Signal write thread
	if( pVia->m_pConn->m_fSync )
	{
		if(WAITING == pVia->SendBufState)
		{
			pVia->SendBufState = EMPTY;
			SetEvent(pVia->hWriteEvent);
		}
	}

	// Async - send queueed packets
	else
	{
		while( !pVia->WriteQ.IsEmpty() && (pVia->SeqNum + 1 < pVia->DestSendLimit) )
		{
			// Send the packet
			pPacket = (SNI_Packet *) pVia->WriteQ.Peek();
			if(!pPacket)
			{
				Assert( 0 && " pPacket is null, coding error or system error\n" );
				BidTrace0( ERROR_TAG _T("pPacket is null coding error or system error\n") );				
				SNI_SET_LAST_ERROR(VIA_PROV, SNIE_4, ERROR_INVALID_STATE);
				goto ErrorExit; // Handle disconnection and clean up
			}

			dwNetError = pVia->SendPacket(pPacket);
			// If SendPacket completed successfully or failed, remove the packet from the Q
			if( ERROR_IO_PENDING != dwNetError )
				pVia->WriteQ.DeQueue();  
			
			if( (ERROR_IO_PENDING != dwNetError) && (ERROR_SUCCESS != dwNetError) )
			{
				a_csVia.Leave(); 
				
				PostWriteDone(pPacket, dwNetError);
				a_csVia.Enter();			
			}
		}
	}
	goto Exit;
	
ErrorExit:

		// Handle error cases: Close the connections and clean up resource.
		// Set the flag to closed
		pVia->fClosed = TRUE;

		// Set event to release anyone stuck in read/write
		// Note: we are doing this multiple times, but that should be okay
		SetEvent(pVia->hWriteEvent);
		SetEvent(pVia->hSyncReadEvent);

		// Disconnect Vi here - if its not been done earlier in Close
		if( pVia->fDisconnected == FALSE )
		{
			VipDisconnect(hVi);
			pVia->fDisconnected = TRUE;
		}

		// Check if this Object is clean
		fCleanup = pVia->IsViClean();                

Exit:
	
	a_csVia.Leave(); 
	if( fCleanup )
		pVia->Cleanup();
	
	return;
	
}


BOOL Via::Cleanup()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	// Check if this has been cleaned up before
	if( InterlockedCompareExchange(&fCleanupCalled, 1, 0) )
	{
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), TRUE);

		return TRUE;
	}

	Assert( (fClean == TRUE) && (fConnClosed == TRUE) || (fClosed == TRUE) );

	while( !m_pConn->m_fSync && !pReadKeyQ.IsEmpty() )
	{
		SNI_Packet * pPacket = (SNI_Packet *) pReadKeyQ.DeQueue();

		PostReadDone(pPacket, ERROR_CONNECTION_UNAVAIL);
	}

	while( !m_pConn->m_fSync && !WriteQ.IsEmpty() )
	{
		SNI_Packet * pPacket = (SNI_Packet *) WriteQ.DeQueue();

		PostWriteDone(pPacket, ERROR_CONNECTION_UNAVAIL);
	}

	// Release the ref on the object
	m_pConn->Release( REF_InternalActive );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), TRUE);

	return TRUE;
}

DWORD Via::SendPacket(SNI_Packet * pPacket)
{	
	BidxScopeAutoSNI2( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}\n"), 
							  GetBidId(),
							  pPacket);

	DWORD dwError = ERROR_SUCCESS;
	DWORD cbTotalBuf = SNIPacketGetBufferSize(pPacket);
	VIP_DESCRIPTOR * pDesc = 0;
	DWORD cbBuf;
	BOOL fFrag;

	// See if there was a previous packet - which was not fully sent out
	// If so, decrement the data to send out by that amount
	if( m_cbWritePendingOffset )
		cbTotalBuf -= m_cbWritePendingOffset;
	
	while( cbTotalBuf )
	{
		// Check if we can send another descriptor
		if( SeqNum + 1 >= DestSendLimit )
		{
			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_IO_PENDING);

			return ERROR_IO_PENDING;
		}
			
		pDesc = m_pSendDesc[ (m_iSendDesc++) % MAX_SVR_BUFS ];

		// See if this needs to be a fragment or not
		if( cbTotalBuf > PAYLOAD_LENGTH )
		{
			fFrag = TRUE;
			cbBuf = PAYLOAD_LENGTH;
			DescSetFragInfo(pDesc, 1);
		}

		else
		{
			fFrag = FALSE;
			cbBuf = cbTotalBuf;
			DescSetFragInfo(pDesc, 0);
		}

		memcpy( DescGetBuf(pDesc), 
				SNIPacketGetBufPtr(pPacket) + m_cbWritePendingOffset, 
				cbBuf);

		DescSetPacket(pDesc, pPacket);
		SetConnObj(pDesc, this);

		// Note: For packets which are larger than PAYLOAD_LENGTH, we artificially increment
		// the size by 1 to indicate that its a fragment. On the receive-side, this will be 
		// intercepted, the length fixed and the descriptor marked as a fragment
		dwError = SendDescriptor( pDesc, 
							     fFrag ? cbBuf+1 : cbBuf, 
							     m_pConn->m_fSync ? VIP_INFINITE : 0,
							     FALSE);

		if( (ERROR_SUCCESS != dwError) && (ERROR_IO_PENDING != dwError) )
		{
			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);

			return dwError;
		}

		dwError = ERROR_SUCCESS;

		// If its a fragment, set the pending offset correctly
		if( fFrag )
			m_cbWritePendingOffset += cbBuf;
		// If not, that means the entire packet was sent out, so reset the offset
		else
			m_cbWritePendingOffset = 0;
		
		cbTotalBuf-= cbBuf;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);

	return dwError;	
}

VIP_RETURN  Via::AllocDescs(VIP_NIC_HANDLE hNic, VIP_PROTECTION_HANDLE pTag)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("hNic: %p{VIP_NIC_HANDLE}, ")
							  _T("pTag: %p{VIP_PROTECTION_HANDLE}\n"), 
							  GetBidId(),
							  hNic, 
							  pTag);

	VIP_RETURN status = VIP_SUCCESS;
	VIP_DESCRIPTOR *ptmpDesc = NULL, *pDesc = NULL;
	DWORD i=0;

	ptmpDesc = get_BufDescriptors( hNic, 
                                              pTag,
                                               MAX_CLNT_BUFS_PLUS_ACK + MAX_SVR_BUFS_PLUS_ACK ,   // One extra reserved for OOB ACK packet.
                                               FRAME_LENGTH,    // Hardcoded for now
                                               &m_hMem, 
                                               &m_pbMem,
                                               this,
                                               &status);

	if( NULL == ptmpDesc )
	{
	    	goto ErrorExit;
	}

	// already pointing to the first recv descriptor
	for(i = 0; i < MAX_CLNT_BUFS_PLUS_ACK; i++)
	{
		pDesc = ptmpDesc;
		ptmpDesc = (VIP_DESCRIPTOR *)ptmpDesc->Control.Next.Address;

		if( (status = VipPostRecv(m_hVi, pDesc, m_hMem)) != VIP_SUCCESS )
		{
			goto ErrorExit;
		}

		m_cRecvs++;
	}

	// Send descriptors
	for(i = 0; i < MAX_SVR_BUFS_PLUS_ACK ; i++)
	{
		m_pSendDesc[i] = ptmpDesc;
		ptmpDesc = (VIP_DESCRIPTOR *)ptmpDesc->Control.Next.Address;
	}

ErrorExit:

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("Status: %d\n"), status);

	return status;

}


//we can ONLY send ack if:
// if we reached the limit and sender is blocked for ack
// if there is an empty descriptor available
// If there is a descriptor available, send it using post.
//if not, skip it this time(???)
DWORD Via::SendAck(int timeout)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("%u#, ")
							  _T("timeout: %d{VIP_NIC_HANDLE}\n"), 
							  GetBidId(),
							  timeout);

	VIP_DESCRIPTOR *pDesc = NULL;
	DWORD dwError = ERROR_SUCCESS;
	BOOL fSendAck = FALSE;

	// See if we NEED to send an Ack
	if( !(SendLimit - LastSendLimit >= 2) || m_fPendingAck )
	{
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

		return ERROR_SUCCESS;
	}

	// See if we CAN send an Ack
	// Allow out of band ack only when it is under smux. This ensures the compatibility with 
	// MDAC/Everret where SMUX is not supported. SQL BU 290874.
	
	if( (m_pConn-> m_pProvHead ->m_Prot != SMUX_PROV &&!(SeqNum + 1< DestSendLimit) )
		|| !(SeqNum < DestSendLimit) )
	{
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

		return ERROR_SUCCESS;
	}

	// Check if we NEED to and CAN send an Ack, but there are other write packets ahead of this one.
	
	// Sync - Signal write thread
	if( m_pConn->m_fSync )
	{
		if(WAITING == SendBufState && SeqNum + 1 < DestSendLimit )
		{
			SendBufState = EMPTY;
			SetEvent(hWriteEvent);
		}

		else
			fSendAck = TRUE;
	}

	// Async - send 1 packet
	else
	{
		if( WriteQ.IsEmpty() ||( SeqNum + 1 == DestSendLimit) )
			fSendAck = TRUE;

		else
		{
			// Send the packet
			SNI_Packet * pPacket = (SNI_Packet *) WriteQ.Peek();
			if(!pPacket)
			{
				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_INVALID_STATE);

				return ERROR_INVALID_STATE;
			}

			dwError = SendPacket(pPacket);

			// If SendPacket completed successfully or failed, remove the packet from the Q
			if( ERROR_IO_PENDING != dwError )
				WriteQ.DeQueue();
			
			if( (ERROR_IO_PENDING != dwError) && (ERROR_SUCCESS != dwError) )
			{
				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);

				return dwError;
			}
		}
	}

	// No pending writes - send an Ack
	if( TRUE == fSendAck )
	{
		BOOL fOobAck = ( SeqNum + 1 == DestSendLimit );
		m_dwAcksSent++;
		VIP_DESCRIPTOR * pDesc = 0;	
		if( fOobAck ) 
			pDesc = m_pSendDesc[ MAX_SVR_BUFS ];  // use reserved send lot to send out of band ack.
		else
			pDesc = m_pSendDesc[(m_iSendDesc++)% MAX_SVR_BUFS ];

		DescSetPacket(pDesc, 0);
		SetConnObj(pDesc, this);
		DescSetFragInfo(pDesc, 0);

		dwError = SendDescriptor(pDesc, 0, timeout, fOobAck );

		if( ERROR_IO_PENDING == dwError )
		{
			m_fPendingAck = fOobAck;  // Only mark pending for out of band ack.
			
		}else if( ERROR_SUCCESS != dwError )
		{
			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);

			return dwError;
		}
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

	return ERROR_SUCCESS;
}

void Via::PostReadDone(SNI_Packet *pPacket, DWORD dwErr)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T( "pPacket: %p{SNI_Packet*}, dwErr: %d\n"), pPacket, dwErr);

	Assert(pPacket);

	SOS_IOCompRequest * pSOSIo = SNIPacketGetSosObject(pPacket);
	pSOSIo->SetErrorCode(dwErr);
	pSOSIo->SetActualBytes(SNIPacketGetBufferSize(pPacket));
	SNIPacketSetBufferSize(pPacket,0); //
	SNIReadDone( pSOSIo );
}

void Via::PostWriteDone(SNI_Packet * pPacket, DWORD dwErr)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T( "pPacket: %p{SNI_Packet*}, dwErr: %d\n"), pPacket, dwErr);

	SOS_IOCompRequest * pSOSIo = SNIPacketGetSosObject(pPacket);
	pSOSIo->SetErrorCode(dwErr);
	pSOSIo->SetActualBytes(SNIPacketGetBufferSize(pPacket));
	SNIWriteDone( pSOSIo );
}
