//*********************************************************************
//		Copyright (c) Microsoft Corporation.
//
// @File: sni_io.cpp
// @Owner: petergv | nantu
// @Test: milu
//
// <owner current="true" primary="true">petergv</owner>
// <owner current="true" primary="false">nantu</owner>
//
// Purpose: 
//		Implementation of common classes 
//
// Notes:
//	
// @EndHeader@
//****************************************************************************	

#ifndef _SNI_IO_HPP_
#define _SNI_IO_HPP_

#include "sni_common.hpp"
#include "sni_error.hpp"

// Forward declaration for prototype
class SNIMemRegion;

// This is a wrapper around NewNoX call to eliminate exception handler
// set up/teardown in the calling function, hence we do not want to inline 
// this.  
SNI_Packet * SNIPacketNew( SNI_Conn * pConn, 
						  SNI_Packet_IOType IOType,
						  SNIMemRegion * pMemRegion,
						  DWORD MemTag,
						  DWORD dwSize,
						  SOS_IOCompRoutine* IOCompRoutine,
						  ConsumerNum ConsNum);

//----------------------------------------------------------------------------
// Name: 	SOS_IOCompRequest
//
// Purpose:	Struct to emulate SOS class for clients
//			
//----------------------------------------------------------------------------
#ifdef SNI_BASED_CLIENT

#define GET_SNI_MEM_REGION(x, y) \
	y = SNIMemRegion::s_rgClientMemRegion

class SOS_ObjectStoreDescriptor : public SLIST_ENTRY
{
};

// Windows ensures that SLIST_ENTRY is always alligned at a 16 byte boundary within a structure/class.
// On other platform like Linux,  SLIST_ENTRY is not constrained to be 16 byte alligned. Hence there is 
// no need to enforce allignement or introduce padding here.
class SOS_IOCompRequest
{
private:
	OVERLAPPED	m_overlapped;		// overlapped struct
	SOS_IOCompRoutine *	m_pfunComp;	// Completion Routine
	PVOID		m_pUserData;		// User data
	DWORD		m_actualBytes;		// Bytes transfered
	DWORD		m_error;			// Error code
	HANDLE		m_hOvlEvent;

public:

	SOS_IOCompRequest () : 
			m_error(0), 
			m_pfunComp(0), 
			m_pUserData(0), 
			m_actualBytes(0), 
			m_hOvlEvent(NULL)
	{
		m_overlapped.hEvent = NULL;
	}

	~SOS_IOCompRequest ()
	{
		if (NULL != m_hOvlEvent)
		{
			if (!CloseHandle(m_hOvlEvent))
			{
				DWORD dwError = GetLastError();
				BidTrace1(ERROR_TAG _T("CloseHandle failed for m_hOvlEvent. %d{DWORD}\n"), dwError);
			}
	
			m_hOvlEvent = NULL;
		}
	}

	inline DWORD InitOvlEvent()
	{
		DWORD dwError = ERROR_SUCCESS;
		
		m_hOvlEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
		if (NULL == m_hOvlEvent)
		{
			dwError = GetLastError();
			BidTrace1(ERROR_TAG _T("CreateEvent failed. %d{DWORD}\n"), dwError);
			m_hOvlEvent = NULL;
		}

		return dwError;
	}

	inline void AddOvlEvent()
	{
		m_overlapped.hEvent = m_hOvlEvent;
	}

	inline void RemoveOvlEvent()
	{
		m_overlapped.hEvent = NULL;
	}


	inline void Init(PVOID Unused1, PVOID dwUnused2, SOS_IOCompRoutine * pfunComp, PVOID pUserData, BOOL unused = TRUE)
	{
		m_pfunComp = pfunComp;
		m_pUserData = pUserData;
	}

	inline DWORD GetErrorCode()
	{
		return m_error;
	}
	
	inline void SetErrorCode(DWORD dwError)
	{
		m_error = dwError;	
	}

	inline OVERLAPPED * GetOverlappedPtr()
	{
		return &m_overlapped;
	}

	DWORD GetActualBytes () const
	{
		return m_actualBytes;
	}

	void SetActualBytes (DWORD numBytes)
	{
		m_actualBytes = numBytes;
	}

	inline SOS_IOCompRoutine * GetCompletionFunction()
	{
		return m_pfunComp;
	}

	void
	SOS_IOCompRequest::ExecuteCompRoutine ()
	{
		(*m_pfunComp) (this);
	}

	inline void SetOvlEventLowBit()
	{
		m_overlapped.hEvent = (HANDLE) ((UINT_PTR)m_overlapped.hEvent | 1);
	}
};
#else

#define GET_SNI_MEM_REGION(x, y) \
	y = static_cast<SNIMemRegion *>(SOS_Node::GetNode(x->m_NodeId)->RetrieveObject(NLS_SNI_CACHE))

#endif

//----------------------------------------------------------------------------
// Name: 	ProvInfo
//
// Purpose:	Struct to allow consumers to pass information from provider to consumer
//			
//----------------------------------------------------------------------------
typedef struct
{
	BYTE ProvNum;
	LPVOID pProvInfo;
} ProvInfo;

// Fwd. declaration
class SNI_Provider;
class SNIMemRegion;
class SNI_Packet;

inline void SNIPacketDelete(__inout_opt SNI_Packet * pPacket);

#define BUF_0K 0
#define BUF_4K 4096
#define BUF_8K 8192
#define BUF_16K 16384
#define BUF_32K 32768
#define BUF_64K 65536
#define BUF_MAX 66536	// Note: This is 64K + 1K, so that we can support user buf sizes of upto 64K. 
						// The extra 1K is for protocol specific headers.

//----------------------------------------------------------------------------
// Name: 	MemTagTypes
//
// Purpose:	Enum to identify different types of memory regions based on type and size
//			
//----------------------------------------------------------------------------
typedef enum
{
	REG_0K,
	REG_4K,
	REG_8K,
	REG_16K,
	REG_32K,	
	REG_64K,
	REG_MAX,
	MAX_MEM_TAGS
} MemTagTypes;

inline SNI_Packet * SNIPacketContainingDescriptor( SOS_ObjectStoreDescriptor *pDescriptor);
inline SOS_ObjectStoreDescriptor * SNIPacketGetDescriptor(SNI_Packet * pPacket);

//----------------------------------------------------------------------------
// Name: 	SNIMemRegion
//
// Purpose:	Manage memory regions
//			
//----------------------------------------------------------------------------
class SNIMemRegion
{
	#define MAX_PACKET_CACHE_SIZE	1024
	
private:
	MemTagTypes	m_eMemTag;

	SNIMemRegion()
	{

#ifdef SNI_BASED_CLIENT 
		InitializeSListHead(&m_SListHeader);
#else
		m_pSOSPacketCache = NULL;
		m_pPacketPmo = NULL;
#endif

	}

	void InitTag(MemTagTypes eMemTag)
	{
		m_eMemTag = eMemTag;
	}
	
#ifdef SNI_BASED_CLIENT 

	SLIST_HEADER m_SListHeader;

	DWORD FInit(MemTagTypes eMemTag)
	{
		InitTag(eMemTag);
		return ERROR_SUCCESS;
	}

#else

	SOS_ObjectStore	* m_pSOSPacketCache;
	SNIMemObj* m_pPacketPmo;
		
	DWORD FInit(MemTagTypes eMemTag)
	{
		InitTag(eMemTag);		

		m_pSOSPacketCache = SOS_ObjectStore::CreateStore( 
						L"SNIPacket",
						OBJECTSTORE_SNI_PACKET,
						MAX_PACKET_CACHE_SIZE,
						SNIMemRegion::DestroyCallback);	

		if ( NULL == m_pSOSPacketCache ) return ERROR_OUTOFMEMORY;
		
		// Permanently bind packet cache to system pool since in a lot of cases packets are
		// allocated in system pool and released in user pool
		//
		m_pSOSPacketCache->SetStaticallyBoundPoolId (SOS_ResourceManager::SYSTEM_ID);
		
		m_pPacketPmo = m_pSOSPacketCache->CreateMemObject(MEMOBJ_SNIPACKETOBJECTSTORE , bThreadSafe | bPageHeap | bNoSlots );

		if ( NULL == m_pPacketPmo) 
		{
			SOS_ObjectStore::DestroyStore (m_pSOSPacketCache);
			m_pSOSPacketCache = NULL;
			return ERROR_OUTOFMEMORY;
		}

		return ERROR_SUCCESS;
	}

public:
	SOS_ObjectStore* GetPacketObjectStore() const { return m_pSOSPacketCache; }
	SNIMemObj* GetPacketPmo() const { return m_pPacketPmo; }

#endif

public:

	static DWORD GetBlockSize(DWORD dwMemTag)
	{
		switch(dwMemTag)
		{
			case REG_0K:
				return BUF_0K;  break;
			case REG_4K:
				return BUF_4K;	break;
			case REG_8K:	
				return BUF_8K;	break;
			case REG_16K:
				return BUF_16K;	break;
			case REG_32K:
				return BUF_32K;	break;
			case REG_64K:
				return BUF_64K;	break;
			case REG_MAX:
				return BUF_MAX;  break;
			default:
				Assert(false);
				return 0;
		}
	}

#ifdef SNI_BASED_CLIENT 

	static SNIMemRegion * s_rgClientMemRegion;

	~SNIMemRegion()
	{
		InterlockedFlushSList(&m_SListHeader);
	}

	SNI_Packet * Pop()
	{
		SOS_ObjectStoreDescriptor *pDescriptor;

		pDescriptor = static_cast<SOS_ObjectStoreDescriptor*>(InterlockedPopEntrySList(&m_SListHeader));

		if( NULL == pDescriptor )
			return NULL;
		else
			return SNIPacketContainingDescriptor(pDescriptor);
	}

	void Push( __out_opt SNI_Packet *pPacket)
	{
		if( QueryDepthSList(&m_SListHeader) < MAX_PACKET_CACHE_SIZE - 1 )
		{
			SLIST_ENTRY* pEntry = static_cast<SLIST_ENTRY*>(SNIPacketGetDescriptor(pPacket));
			InterlockedPushEntrySList(&m_SListHeader, pEntry);
		}
		else
		{
			// Strategy: Don't make an effort to clean up the "extra" entries, but don't push this one onto the stack
			// if it will go over. This guarantees an *approximate* maximum 
			// (bounded above by MAX_PACKET_CACHE_SIZE + ConcurrentThreadsAccessingTheStack)
			
			// It would cost a lot of performance to put a stronger guarantee on the maximal size, since
			// we would need to synchronize access to two variables (stack head, stack size), which we 
			// don't know how to do without using a true sync primitive, rather than the two separate
			// Interlocked operations which are all that's required to maintain consistency of the list header and 
			// an *approximate* depth guarantee.
			SNIPacketDelete(pPacket);
		}
	}

#else		

	~SNIMemRegion()
	{
		if( m_pPacketPmo )
		{
			m_pPacketPmo ->Release();
			m_pPacketPmo = 0;
		}
		
		if( m_pSOSPacketCache )
		{
			SOS_ObjectStore::DestroyStore( m_pSOSPacketCache );

			m_pSOSPacketCache = 0;
		}		
	}

	SNI_Packet * Pop()
	{
		SOS_ObjectStoreDescriptor *pDescriptor;
		
		pDescriptor = m_pSOSPacketCache->Pop();
		
		if( !pDescriptor )
		{
			return NULL;
		}

		return SNIPacketContainingDescriptor( pDescriptor );
	}

	void Push( SNI_Packet *pPacket)
	{
		m_pSOSPacketCache->Push(SNIPacketGetDescriptor(pPacket));
	}

	static VOID WINAPI DestroyCallback(	SOS_ObjectStoreDescriptor* pDescriptor )
	{
		SNI_Packet *pPacket;
		
		pPacket = SNIPacketContainingDescriptor( pDescriptor );

		SNIPacketDelete( pPacket );
	}

#endif

	static SNIMemRegion * Init()
	{
		LONG i = 0;

		DWORD dwErr = ERROR_SUCCESS;

		SNIMemRegion * rgMemRegion;

		rgMemRegion= NewNoX(gpmo) SNIMemRegion[MAX_MEM_TAGS];
		
		if(NULL == rgMemRegion)
		{
			return NULL;
		}
		
		for(i = 0; i < MAX_MEM_TAGS; i++)
		{
			if( ERROR_SUCCESS != rgMemRegion[i].FInit((MemTagTypes) i))
			{
				delete [] rgMemRegion;

				return NULL;
			}
		}

		return rgMemRegion;
		
		// 
	}

	static void Flush( SNIMemRegion *rgMemRegion )
	{
		// Cleanup the packets in all memory regions
		// but preserve the regions themselves.  
		for(DWORD i = 0; i < MAX_MEM_TAGS; i++)
		{
			// Free all the packets in this memory region's cache
			SNI_Packet * pPacket;

			while( NULL != (pPacket = (SNI_Packet *) rgMemRegion[i].Pop()) )
			{
				SNIPacketDelete(pPacket);
			}
		}
	}

	static void Terminate( SNIMemRegion *rgMemRegion )
	{
		// Cleanup the memory regions
		Flush(rgMemRegion); 

		delete [] rgMemRegion;
	}

	// Get IO tag
	static DWORD GetMemoryTag(DWORD dwSize)
	{
		// "Buffer-less" packets
		if ( dwSize == BUF_0K )
		{
			return REG_0K;
		}
		
		// 4k buffers
		else if( dwSize <= BUF_4K )
		{
			return REG_4K;
		}

		// 8k buffers
		else if( dwSize <= BUF_8K )
		{
			return REG_8K;		
		}

		// 16k buffers
		else if( dwSize <= BUF_16K )
		{
			return REG_16K;		
		}

		// 32k buffers
		else if (dwSize <= BUF_32K )
		{
			return REG_32K;
		}
		else if (dwSize <= BUF_64K )
		{
			return REG_64K;
		}
		else if (dwSize <= BUF_MAX )
		{
			return REG_MAX;
		}
		else
		{
			//TDS gurantee network packet size are less than REG_MAX.
			return REG_MAX;		
		}

	}

};

//----------------------------------------------------------------------------
// Name: 	SNI_Packet
//
// Purpose:	Manage SNI Packets
//			
// Note:	Since page sizes are 8K, we might later want to move all the member variables 
//			to a struct. This is to make sure that for 8K packets, we don't reduce the
//			buffer sizes b'cos of the overhead of member variables.
//----------------------------------------------------------------------------
class SNI_Packet:public SOS_IOCompRequest,public SOS_ObjectStoreDescriptor
{	
private:
	BYTE * 				m_pBuffer;		// Pointer to buffer
	DWORD				m_cbBuffer;	    // Length of data in the buffer
	DWORD				m_cBufferSize;	// Size of the buffer
	DWORD				m_OffSet;		// Data Offset in the buffer    
	LONG				m_cRef;		    // Packet's reference count.
	SNI_Conn *			m_pConn;		// Connection Pointer
	LPVOID				m_pKey;			// Completion Key
	SNIMemRegion*		m_pMemRegion;	//The memory region this packet belongs to.
	SNI_Packet *		m_pNext;		// Chained packet

#ifndef SNI_BASED_CLIENT
	BOOL				m_fZeroPayloadOnRelease;	// Zero out data portion before releasing
	DWORD				m_cbBytesToZero;			// if zero, entire pakcet is zeroed, if non-zero, amount of the data portion to zero out
	BYTE				m_bMemTag;					// dwMemTag
#endif // !SNI_BASED_CLIENT
	BYTE				m_OrigProv;			// Indicator for provider to identify specific packet
	DWORD				m_cbTrailer;	// used by CryptoBase, trailer size of the outgoing packet
	SNI_Packet_IOType	m_IOType;		// IO Type of the packet, see comments at SNI_Packet_IOType enum
	ConsumerNum			m_ConsBuf;		// The consumer of the packet's buffer - for tracking and debugging purposes
	int					m_iBidId; 

#ifndef SNI_BASED_CLIENT
	inline Counter CPages(DWORD dwSize) const
	{
		return (dwSize + (x_cbMemPageSize - 1)) / x_cbMemPageSize;
	}

#endif // !SNI_BASED_CLIENT

	SNI_Packet(	DWORD dwSize, 
			   __in SNI_Conn * pConn, 
					SOS_IOCompRoutine * pfunComp, 
					DWORD dwMemTag,
					SNIMemRegion* pMemRegion, 
					SNI_Packet_IOType IOType, 
					ConsumerNum ConsNum) :
				m_pConn(pConn), m_OffSet(0), m_cbBuffer(0), m_pKey(NULL), m_pMemRegion(pMemRegion),
#ifndef SNI_BASED_CLIENT
				m_fZeroPayloadOnRelease(FALSE),
				m_cbBytesToZero(0),
				m_bMemTag(checked_static_cast<BYTE>(dwMemTag)),
#endif // !SNI_BASED_CLIENT
				m_pNext(0),
				m_OrigProv(INVALID_PROV),
				m_cbTrailer(0),
				m_cRef(1),
				m_IOType(IOType),
				m_ConsBuf(ConsNum),
				m_iBidId(0)
	{
		// Get a memory block for this connection
		//
		// Note: The memory tag - previously associated with this connection - would
		// identify the correct memory region.  Also, if the requested size would result
		// in an allocation greater than 8K, we want to go directly to pages.
		//
		DWORD dwBlockSize = SNIMemRegion::GetBlockSize(dwMemTag);

		Assert( dwSize <= dwBlockSize ); 
		Assert( IOType < SNI_Packet_InvalidType );
		Assert( ConsNum < SNI_Consumer_Invalid );

		if ((SNI_Packet_Read == IOType) ||
			(SNI_Packet_Write == IOType))
		{
#ifndef SNI_BASED_CLIENT

			if (dwBlockSize >= CMemObj::xsm_cbMaxSizeFitsOnPage)
			{
				Counter cPages = CPages(dwBlockSize);
				AllocatorType allocatorType;

				Assert( m_pMemRegion && m_pMemRegion->GetPacketObjectStore() );
				m_pBuffer = static_cast<BYTE*>
					(m_pMemRegion->GetPacketObjectStore()->AllocatePages(cPages, &allocatorType));
			}
			else
			{
				Assert ( m_pMemRegion && m_pMemRegion->GetPacketPmo() );
				m_pBuffer = NewNoX(m_pMemRegion->GetPacketPmo()) BYTE[dwBlockSize];
			}
#else// !SNI_BASED_CLIENT
			{
				m_pBuffer = NewNoX(gpmo) BYTE[dwBlockSize];
			}
#endif
		}
		else	// IOType is SNI_Packet_KeyHolderNoBuf or SNI_Packet_VaryingBuffer*
		{
			m_pBuffer = NULL;
		} 

		// Set the packet buffer size
		m_cBufferSize = dwSize;

		// Set chained packet to NULL
		m_pNext = NULL;

#ifndef SNI_BASED_CLIENT
				// NOTE: Engine uses only async connections
				Assert(FALSE == m_pConn->m_fSync);
#endif
		
		SOS_IOCompRequest::Init(0, 0, (SOS_IOCompRoutine *)pfunComp, this, m_pConn->m_fSync);
		SOS_IOCompRequest::SetErrorCode(ERROR_SUCCESS);

		BidObtainItemIDA( &m_iBidId, SNI_ID_TAG "%p{.}", this );

		BidTraceU2( SNI_BID_TRACE_ON, SNI_TAG 
			_T("%u#{SNI_Packet} created by %u#{SNI_Conn}\n"), 
			m_iBidId, 
			pConn->GetBidId() );
	}

	~SNI_Packet()
	{
		BidRecycleItemIDA( &m_iBidId, SNI_ID_TAG ); 

		Assert( m_IOType < SNI_Packet_InvalidType );
		Assert( m_ConsBuf < SNI_Consumer_Invalid );
		// Key Holder packets must always have their buffer set to NULL
		// "VaryingBuffer*" packets must have their buffer released
		// by the Consumer.
		Assert( (SNI_Packet_KeyHolderNoBuf != m_IOType) ||
				(NULL == m_pBuffer) );

		if (m_pBuffer &&
			(SNI_Packet_VaryingBufferRead != m_IOType) &&
			(SNI_Packet_VaryingBufferWrite != m_IOType) )
		{
#ifndef SNI_BASED_CLIENT

			DWORD dwBlockSize = SNIMemRegion::GetBlockSize(m_bMemTag);
			
			if (dwBlockSize >= CMemObj::xsm_cbMaxSizeFitsOnPage)
			{
				Counter cPages = CPages(dwBlockSize);
				
				Assert( m_pMemRegion && m_pMemRegion->GetPacketObjectStore() );
				m_pMemRegion->GetPacketObjectStore() ->FreePages(m_pBuffer, cPages);
			}
			else
#endif // SNI_BASED_CLIENT
			{
				delete [] m_pBuffer;
			}
		}
		m_pBuffer = NULL;
	}

public:

	//we want to be able to access private members from sni Providers
	//for debugging purposes
	friend class Smux;
	friend class Session;
	friend class CryptoBase;	//base class for Ssl and Sign providers
	friend class Ssl;
	friend class Sign;
	friend class Tcp;
	friend class Np;
	friend class Sm;
	friend class Via;

	// Note: This call is for allocating packets with explicit specifying of Consumer
	// The main reason is to force Consumer specification when "varying" packets are allocated.
	friend SNI_Packet * SNIPacketAllocateEx2(SNI_Conn * pConn, 
										  SNI_Packet_IOType IOType,
										  ConsumerNum ConsNum)
	{
		BidTraceU3( SNI_BID_TRACE_ON, SNIAPI_TAG _T( "pConn: %p{SNI_Conn*}, IOType: %d, consumer: %d\n"), pConn, IOType, ConsNum);

		Assert( IOType < SNI_Packet_InvalidType );
		Assert( ConsNum < SNI_Consumer_Invalid );
		
		SNI_Packet * 		pPacket = NULL;
		SNIMemRegion *		pMemRegion = 0;
		DWORD 				MemTag;
		DWORD 				cBufferSize;
		SOS_IOCompRoutine* 	IOCompRoutine;


		// This should be really maintained by SNI_Conn but for safety 
		// reasons we will check it here.  
		if( pConn->m_ConnInfo.ConsBufferSize + pConn->m_ConnInfo.ProvBufferSize > 
			SNIMemRegion::GetBlockSize(pConn->m_MemTag) )
		{
			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, ERROR_INVALID_PARAMETER );
			BidTrace3( ERROR_TAG _T( "ConsBufferSize: %d, ProvBufferSize: %d, m_MemTag: %d{MemTagTypes}\n" ), 
				pConn->m_ConnInfo.ConsBufferSize, pConn->m_ConnInfo.ProvBufferSize, pConn->m_MemTag ); 
			goto ret; 
		}

		GET_SNI_MEM_REGION(pConn, pMemRegion);

		// The if statement above made sure the following assert is true
		Assert( pConn->m_MemTag < MAX_MEM_TAGS );

		// DEVNOTE: MemTag is not used at the preceeding validation statements
		// to avoid elimination of the verification, which is part of security checks
		// The verification above should be valid no matter what type of packet we are
		// allocating.
		switch (IOType)
		{
			case SNI_Packet_KeyHolderNoBuf:
			case SNI_Packet_VaryingBufferRead:
				MemTag = REG_0K;
				cBufferSize = BUF_0K;
				IOCompRoutine = (SOS_IOCompRoutine *)SNIReadDone;
				break;
			case SNI_Packet_VaryingBufferWrite:
				MemTag = REG_0K;
				cBufferSize = BUF_0K;
				IOCompRoutine = (SOS_IOCompRoutine *)SNIWriteDone;
				break;
			case SNI_Packet_Read:
				MemTag = pConn->m_MemTag;
				cBufferSize = pConn->m_ConnInfo.ConsBufferSize + pConn->m_ConnInfo.ProvBufferSize;
				IOCompRoutine = (SOS_IOCompRoutine *)SNIReadDone;
				break;
			case SNI_Packet_Write:
				MemTag = pConn->m_MemTag;
				cBufferSize = pConn->m_ConnInfo.ConsBufferSize + pConn->m_ConnInfo.ProvBufferSize;
				IOCompRoutine = (SOS_IOCompRoutine *)SNIWriteDone;
				break;
			default:
				Assert(false && "Invalid Packet IOType specified\n");	// Invalid IOType
				goto ret;
		}

		// First, try to get this from the cache
		if(  NULL == (pPacket = pMemRegion[MemTag].Pop()))	
	    {    	
			// If we do not find it in the cache, then allocate a new object
			pPacket = SNIPacketNew(pConn, IOType, &pMemRegion[MemTag], MemTag, cBufferSize, IOCompRoutine, ConsNum); 

			if (NULL == pPacket)
				goto ret;
	    }
		else
		{
#ifndef SNI_BASED_CLIENT
			Assert ( pPacket -> m_bMemTag == MemTag 
				&& pPacket ->m_pMemRegion == &pMemRegion[MemTag]);
#endif

			// Set the packet buffer size
			pPacket->m_cBufferSize = cBufferSize;
			
			pPacket->m_pConn = pConn;
			pPacket->m_OffSet = 0;
			pPacket->m_cbBuffer = 0;
			// Packets from REG_0K might be SNI_Packet_KeyHolderNoBuf or SNI_Packet_VaryingBuffer*
			pPacket->m_IOType = IOType;
			pPacket->m_ConsBuf = ConsNum;
			
#ifndef SNI_BASED_CLIENT
			// This flag should have been reset before it got put in the pool.
			// Nevertheless as a defensive programming measure, we will unset
			// it here just like the other flags.
			Assert(!pPacket->m_fZeroPayloadOnRelease);
			pPacket->m_fZeroPayloadOnRelease = FALSE;
			pPacket->m_cbBytesToZero = 0;
#endif // ifndef SNI_BASED_CLIENT

			pPacket->Init(0, 0, IOCompRoutine, pPacket, FALSE);
			pPacket->SetErrorCode(ERROR_SUCCESS);
			
			// If the packet came from the cache, reset it's ref count to 1.
			Assert(0==pPacket->m_cRef);
			pPacket->m_cRef = 1;

			BidTraceU2( SNI_BID_TRACE_ON, SNI_TAG 
				_T("%u#{SNI_Packet} from pool for %u#{SNI_Conn}\n"), 
				SNIPacketGetBidId(pPacket), 
				pConn->GetBidId() );
		}

		Assert(1==pPacket->m_cRef);

		// Set the offset correctly - for Write buffers ONLY
		if( IOType == SNI_Packet_Write )
		{
			pPacket->m_OffSet = pConn->m_ConnInfo.ProvOffset;
		}	

#ifdef SNI_BASED_CLIENT
		// Set Event handle to Overlapped structure for Sync connections
		if (pConn->m_fSync)
		{
			pPacket->AddOvlEvent();
		}
#endif
		
		if ((SNI_Packet_Read == IOType) || 
			(SNI_Packet_Write == IOType))
		{
			pConn->AddRef(REF_Packet);
		}
		else
		{
			pConn->AddRef(REF_PacketNotOwningBuf);
		}

	ret:

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%p{SNI_Packet*}\n"), pPacket);
		
		return (SNI_Packet *)pPacket;
	}

	// Note: This call is for packets to be used by the entire stack
	// except Consumers who need "varying" packets.
	friend SNI_Packet * SNIPacketAllocate( SNI_Conn * pConn, SNI_Packet_IOType IOType )
	{
		// No BID tracing here - the call is just a wrapper to prevent "varying" packets
		// allocation without specifying the buffer-owning Consumer
		Assert(	(IOType != SNI_Packet_VaryingBufferRead) &&
				(IOType != SNI_Packet_VaryingBufferWrite));

		return SNIPacketAllocateEx2(pConn, IOType, SNI_Consumer_SNI);
	}

	// Note: This is for Provider's to allocate internal buffers
	// Key difference is provider supplies the IOComp function
	//
	friend SNI_Packet * SNIPacketAllocateEx( 	SNI_Conn * pConn, 
								    				SNI_Packet_IOType IOType,
								   					SOS_IOCompRoutine pfunComp)
	{
		BidTraceU3( SNI_BID_TRACE_ON, SNIAPI_TAG _T( "pConn: %p{SNI_Conn*}, IOType: %d, SOS_IOCompRoutine: %p{SOS_IOCompRoutine}\n"), 
						pConn, IOType, pfunComp);
		
		Assert(	IOType < SNI_Packet_InvalidType );

		SNI_Packet * pPacket = SNIPacketAllocate (pConn, IOType);

		if (pPacket)
			pPacket->Init(0, 0, pfunComp, pPacket, FALSE);

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%p{SNI_Packet*}\n"), pPacket);
		
		return pPacket;
	}


	// This is a wrapper around NewNoX call to eliminate exception handler
	// set up/teardown in the calling function; we do not want to inline 
	// this.  
	friend SNI_Packet * SNIPacketNew( __in SNI_Conn * pConn, 
									  SNI_Packet_IOType IOType,
									  SNIMemRegion * pMemRegion,
									  DWORD MemTag,
									  DWORD dwSize,
									  SOS_IOCompRoutine* IOCompRoutine,
									  ConsumerNum ConsNum )
	{
		BidTraceU2( SNI_BID_TRACE_ON, SNIAPI_TAG _T( "pConn: %p{SNI_Conn*}, IOType: %d\n"), pConn, IOType);

		DWORD dwError = ERROR_SUCCESS;

		Assert(IOType < SNI_Packet_InvalidType);
		Assert(ConsNum < SNI_Consumer_Invalid);

#ifndef SNI_BASED_CLIENT		
		Assert ( pMemRegion && pMemRegion->GetPacketPmo() );
		SNI_Packet * pPacket = 			
			NewNoX(pMemRegion->GetPacketPmo()) SNI_Packet( dwSize, 
									pConn, 
									IOCompRoutine, 
									MemTag,
									pMemRegion,
									IOType,
									ConsNum);
#else
		SNI_Packet * pPacket = 			
			NewNoX(gpmo) SNI_Packet( dwSize, 
									pConn, 
									IOCompRoutine, 
									MemTag,
									pMemRegion,
									IOType,
									ConsNum);
#endif
		if (NULL == pPacket)	// SNI_Packet object allocation failed
		{
			dwError = ERROR_OUTOFMEMORY;
			goto ret;
		}
		else if( 	((SNI_Packet_Read == IOType) || 	// If IOType is Read or Write
					 (SNI_Packet_Write == IOType)) &&	// we must have a buffer allocated successfully
					(NULL == pPacket->m_pBuffer) )
		{
			delete pPacket;
			pPacket = NULL;
			dwError = ERROR_OUTOFMEMORY;
			goto ret;
		}
		
#ifdef SNI_BASED_CLIENT
		dwError = pPacket->InitOvlEvent();
		if (ERROR_SUCCESS != dwError)
		{
			// Already BID-traced at InitOvlEvent
			delete pPacket;
			pPacket = NULL;
			goto ret;
		}
#endif		

	ret:
		if( NULL == pPacket )
		{
			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, dwError );
		}

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%p{SNI_Packet*}\n"), pPacket);
		
		return (SNI_Packet *)pPacket;
	}

	friend SNI_Packet * SNIPacketContainingDescriptor( SOS_ObjectStoreDescriptor *pDescriptor)
	{
		return static_cast<SNI_Packet *>(pDescriptor);
	}

	friend SOS_ObjectStoreDescriptor * SNIPacketGetDescriptor( SNI_Packet *pPacket )
	{
		return pPacket;
	}

#ifndef SNI_BASED_CLIENT

	friend void SNIPacketZeroPayloadOnRelease(SNI_Packet * pPacket, DWORD cbBytesToZero)
	{
		Assert(	(SNI_Packet_KeyHolderNoBuf != pPacket->m_IOType) &&
				(SNI_Packet_VaryingBufferRead != pPacket->m_IOType) &&
				(SNI_Packet_VaryingBufferWrite != pPacket->m_IOType));
		pPacket->m_fZeroPayloadOnRelease = TRUE;
		pPacket->m_cbBytesToZero = cbBytesToZero;
	}

#endif

	// Increments the ref count of a SNI_Packet.
	// Consumers addref a packet to guarantee the scope of the packet's lifetime.
	// Each call to SNIPacketRelease decrements the ref count and the packet is finally
	// released when the ref count drops to zero.
	friend void SNIPacketAddRef(SNI_Packet * pPacket)
	{
		BidTraceU2( SNI_BID_TRACE_ON, SNIAPI_TAG _T("%u#{SNI_Packet}, ")
												 _T("pPacket: %p{SNI_Packet*}\n"), 
												 SNIPacketGetBidId(pPacket), 
												 pPacket);
		Assert (pPacket);
		Assert (pPacket->m_cRef > 0);
		Assert (pPacket->m_cRef < LONG_MAX);
		InterlockedIncrement( &pPacket->m_cRef );
	}

	friend void SNIPacketRelease(SNI_Packet * pPacket)
	{	
		BidTraceU2( SNI_BID_TRACE_ON, SNIAPI_TAG _T("%u#{SNI_Packet}, ")
												 _T("pPacket: %p{SNI_Packet*}\n"), 
												 SNIPacketGetBidId(pPacket), 
												 pPacket);
		
		Assert (pPacket);
		Assert (pPacket->m_pConn);
		Assert (pPacket->m_cRef > 0);
		Assert (pPacket->m_IOType < SNI_Packet_InvalidType);
		Assert ((pPacket->m_IOType != SNI_Packet_KeyHolderNoBuf) || 
				(	(NULL == pPacket->m_pBuffer) &&
					(0 == pPacket->m_cbBuffer) &&
					(0 == pPacket->m_cBufferSize))	);
		Assert (pPacket->m_ConsBuf < SNI_Consumer_Invalid);

		int iBidId = SNIPacketGetBidId(pPacket); 
		
		// Only release the packet if it's ref count drops to zero.
		if ( 0 != InterlockedDecrement( &pPacket->m_cRef ) ) 
		{
			BidTraceU2( SNI_BID_TRACE_ON, RETURN_TAG 
				_T("%u#!{SNI_Packet}: ")
				_T("Not final release.  ")
				_T("pPacket: %p{SNI_Packet*}\n"), 
				iBidId, 
				pPacket);
			return;
		}

#ifndef SNI_BASED_CLIENT
		if ((SNI_Packet_Read == pPacket->m_IOType) ||
			(SNI_Packet_Write == pPacket->m_IOType))
		{
			Assert(pPacket->m_pBuffer != NULL);
			BOOL fGlobalZeroingRequired = SOS_OS::GetCommonCriteriaModeEnabled ();
			if (pPacket->m_fZeroPayloadOnRelease || fGlobalZeroingRequired)
			{
				BYTE * pbBuf;
				DWORD cbBuf;
				if (fGlobalZeroingRequired)
				{
					//	If global zeroing is required always zero out the whole buffer
					//
					pbBuf = pPacket->m_pBuffer;
					cbBuf = pPacket->m_cBufferSize;
				}
				else
				{
					pbBuf = SNIPacketGetBufPtr(pPacket);
					cbBuf = SNIPacketGetBufferSize(pPacket);
					//	The bytes of memory to zero should never exceed the size
					//	of the packet
					//	If the m_cbBytesToZero is on-zero, we will zero out that
					//	amount, if not, zero out all the bytes
					//
					Assert (pPacket->m_cbBytesToZero <= cbBuf);
					if (pPacket->m_cbBytesToZero != 0)
						cbBuf = pPacket->m_cbBytesToZero;
				}
				SecureZeroMemory(pbBuf, cbBuf);
				pPacket->m_fZeroPayloadOnRelease = FALSE;
				pPacket->m_cbBytesToZero= 0;
			}
		}
		else	// "Buffer-less" packet like SNI_Packet_KeyHolderNoBuf or SNI_Packet_VaryingBuffer*
		{
			pPacket->m_fZeroPayloadOnRelease = FALSE;
			pPacket->m_cbBytesToZero= 0;
		}
#endif // ifndef SNI_BASED_CLIENT


		// While SNI_Packet_KeyHolderNoBuf type of packet never gets a buffer assigned to it
		// the SNI_Packet_VaryingBuffer* packets get their buffers assigned by the Consumer
		// However, Consumer is fully responsible for releasing the buffers, SNI treats
		// these types of packets as "buffer-less" and simply removes any reference to
		// the potential buffers used by the Consumer
		if ((SNI_Packet_VaryingBufferRead == pPacket->m_IOType) ||
			(SNI_Packet_VaryingBufferWrite == pPacket->m_IOType))
		{
			pPacket->m_pBuffer = NULL;
			pPacket->m_pNext = NULL;
			pPacket->m_cBufferSize = 0;
			pPacket->m_cbBuffer = 0;
		}

		pPacket->m_ConsBuf = SNI_Consumer_PacketIsReleased;

		BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG 
			_T("%u#{SNI_Packet} to pool\n"), 
			SNIPacketGetBidId(pPacket) );

		SNI_Conn* pConn = pPacket->m_pConn;

		if ((SNI_Packet_Read == pPacket->m_IOType) || 
			(SNI_Packet_Write == pPacket->m_IOType))
		{
			pConn->Release( REF_Packet );
		}
		else
		{
			pConn->Release( REF_PacketNotOwningBuf );
		}

#ifdef SNI_BASED_CLIENT
		// No need to check for sync or async conn, we just set the Overlapped hEvent to NULL
		pPacket->RemoveOvlEvent();
#endif
		
		pPacket->m_pMemRegion->Push(pPacket);
	}

	friend void SNIPacketReset(SNI_Conn * pConn, SNI_Packet_IOType IOType, SNI_Packet * pPacket, ConsumerNum ConsNum)
	{
#ifndef SNI_BASED_CLIENT
		// If we adjust the offset or the data size, then zeroing the memory
		// will not clear everything expected. We expect this flag is never set
		// when offset or data size is being changed.
		Assert(!pPacket->m_fZeroPayloadOnRelease);
#endif // ifndef SNI_BASED_CLIENT

		Assert(IOType < SNI_Packet_InvalidType);
		Assert(ConsNum < SNI_Consumer_Invalid);
		Assert(pPacket->m_IOType < SNI_Packet_InvalidType);
		Assert(pPacket->m_ConsBuf < SNI_Consumer_Invalid);
		Assert((pPacket->m_IOType != SNI_Packet_KeyHolderNoBuf) || 
				(	(NULL == pPacket->m_pBuffer) &&
					(0 == pPacket->m_cbBuffer) &&
					(0 == pPacket->m_cBufferSize))	);

		// Reset the Offset to 0 for any packet type different from Write
		if( SNI_Packet_Write == IOType )
			pPacket->m_OffSet = pConn->m_ConnInfo.ProvOffset;
		else
			pPacket->m_OffSet = 0;

		// DEVNOTE: The "varying" packets offer a direct pointing
		// mechanism to an existing data. By convention, upon release or reset
		// we just disregard the buffer. It is Consumer's obligation to take
		// care of the buffer.
		// From SNI's perspective, the "varying" packets are treated as "buffer-less"
		if ((SNI_Packet_VaryingBufferRead == pPacket->m_IOType) || 
			(SNI_Packet_VaryingBufferWrite == pPacket->m_IOType))
		{
			pPacket->m_pBuffer = NULL;
			pPacket->m_pNext = NULL;
			pPacket->m_cBufferSize = 0;
		}

		pPacket->m_cbBuffer = 0;
		pPacket->m_IOType = IOType;
		pPacket->m_ConsBuf = ConsNum;
	}

	// Returns pointer to the relevant data buffer and the size of the data buffer
	friend void SNIPacketGetData(__in SNI_Packet * pPacket, __deref_out_bcount_full(*pcbBuf) BYTE ** ppBuf, __out DWORD * pcbBuf)
	{
		Assert (pPacket->m_cBufferSize >= pPacket->m_OffSet);
		Assert (pPacket->m_OffSet + pPacket->m_cbBuffer <= pPacket->m_cBufferSize);
		*ppBuf = (BYTE *)(pPacket->m_pBuffer + pPacket->m_OffSet);
		*pcbBuf = pPacket->m_cbBuffer;
	}

	// Copies buffer to the packet's data buffer FROM the point of the currect offset
	// and adjusts the size accordingly
	// Note: THIS DOES NOT ADJUST THE OFFSET VALUE
	// Note: Typically to be used by consumers
	friend void SNIPacketSetData(__inout SNI_Packet * pPacket, __in_bcount(cbBuf) const BYTE * pbBuf, __in DWORD cbBuf)
	{
#ifndef SNI_BASED_CLIENT
		// If we adjust the offset or the data size, then zeroing the memory
		// will not clear everything expected. We expect this flag is never set
		// when offset or data size is being changed.
		Assert(!pPacket->m_fZeroPayloadOnRelease);
#endif // ifndef SNI_BASED_CLIENT

		Assert( pPacket->m_OffSet+cbBuf <= pPacket->m_cBufferSize);
		memcpy((BYTE *)(pPacket->m_pBuffer + pPacket->m_OffSet) , (BYTE *)pbBuf, cbBuf);
		pPacket->m_cbBuffer = cbBuf;
	}
	
	// Return the pointer to the buffer 
	friend BYTE * SNIPacketGetBufPtr(SNI_Packet * pPacket)
	{
		return (BYTE *)(pPacket->m_pBuffer + pPacket->m_OffSet);
	}

#ifdef SNI_BASED_CLIENT
	// Set the buffer pointer to the one specified
	// NOTE: THIS IS A VERY DANGEROUS FUNCTION - USE WITH CARE
	// NOTE: We do not need this function on server side, and if it were
	// available on server side, then it would interfere with the server-only
	// flag m_fZeroPayloadOnRelease. Hence, this method is defined out as not
	// available on server side.

	// NOTE: As part of SSB-related work, we have introduced new packet types
	// to allow memcpy-less assignment of buffer to a packet. To preserve compat,
	// we have introduced separate functions, we are also asserting the packet 
	// type there.
	friend void SNIPacketSetBufPtr(__out SNI_Packet * pPacket, __in BYTE * pbBuf)
	{
		pPacket->m_pBuffer = pbBuf;
	}
#endif // ifdef SNI_BASED_CLIENT
	
	// Return the REAL size of the data buffer
	// Note: This is used by the transport provider while posting read requests
	// to get the max. amount of data to be read.
	friend DWORD SNIPacketGetBufActualSize(SNI_Packet * pPacket)
	{
		return pPacket->m_cBufferSize;
	}

	// Return a pointer to the overlapped struct for this packet.
	friend OVERLAPPED * SNIPacketOverlappedStruct(SNI_Packet * pPacket)
	{
		return pPacket->GetOverlappedPtr();
	}

	// Return a pointer to the associated connection for for this packet.
	friend SNI_Conn * SNIPacketGetConnection(SNI_Packet * pPacket)
	{
		return pPacket->m_pConn;
	}

	// Change the connection associate with this packet
	// NOTE: THIS IS DANGEROUS AND IS ONLY PROVIDED FOR SOME PROVIDERS (SMUX)
	// WHICH NEED TO DO INTERNAL BUFFERING AND DEMULTIPLEX PACKETS TO DIFFERENT
	// CONNECTIONS
	friend void SNIPacketSetConnection(SNI_Packet * pPacket, SNI_Conn * pConn)
	{
		// When this done, we would need to fix the refcounts for the old and new conns
		// Release refcount for old conn, add refcount for new conn

		if ((SNI_Packet_Read == pPacket->m_IOType) ||
			(SNI_Packet_Write == pPacket->m_IOType))
		{
			pPacket->m_pConn->Release( REF_Packet );
			pConn->AddRef( REF_Packet );
		}
		else
		{
			pPacket->m_pConn->Release( REF_PacketNotOwningBuf );
			pConn->AddRef( REF_PacketNotOwningBuf );
		}

		pPacket->m_pConn = pConn;
	}

	//	Return the error of this packet
	friend DWORD SNIPacketError(SNI_Packet * pPacket)
	{
		return pPacket->GetErrorCode();
	}

	//	Set the error of this packet
	friend void SNIPacketSetError(SNI_Packet * pPacket, DWORD dwError)
	{
		pPacket->SetErrorCode(dwError);
	}

	// Return a pointer to the completion function for this packet.
	friend SOS_IOCompRoutine * SNIPacketCompFunc(__in SNI_Packet * pPacket)
	{
#ifndef SNI_BASED_CLIENT
		Assert( 0 && " We do not expect this function is called on server side\n" );
		return NULL;
#else
		return pPacket->GetCompletionFunction();
#endif
	}

	// Appends provided buffer to end of existing data. To be used by intermediate/transport providers
	// Note: This function ONLY adjusts the BUFFER SIZE not the OFFSET
	friend void SNIPacketAppendData(__in SNI_Packet * pPacket, __in_bcount(cbBuf) BYTE * pbBuf, __in DWORD cbBuf)
	{
#ifndef SNI_BASED_CLIENT
		// If we adjust the offset or the data size, then zeroing the memory
		// will not clear everything expected. We expect this flag is never set
		// when offset or data size is being changed.
		Assert(!pPacket->m_fZeroPayloadOnRelease);
#endif // ifndef SNI_BASED_CLIENT

		Assert( pPacket->m_OffSet+pPacket->m_cbBuffer+cbBuf <= pPacket->m_cBufferSize);
		memcpy((BYTE *)(pPacket->m_pBuffer + pPacket->m_OffSet + pPacket->m_cbBuffer) , (BYTE *)pbBuf, cbBuf);
		pPacket->m_cbBuffer += cbBuf;
	}

	// Prepends provided buffer to front of existing data. To be used by intermediate/transport providers
	// Note: This function adjusts BOTH the BUFFER SIZE AND the OFFSET
	friend void SNIPacketPrependData(__inout SNI_Packet * pPacket, __in_bcount(cbBuf) BYTE * pbBuf, __in DWORD cbBuf)
	{
#ifndef SNI_BASED_CLIENT
		// If we adjust the offset or the data size, then zeroing the memory
		// will not clear everything expected. We expect this flag is never set
		// when offset or data size is being changed.
		Assert(!pPacket->m_fZeroPayloadOnRelease);
#endif // ifndef SNI_BASED_CLIENT

		Assert( cbBuf <= pPacket->m_OffSet );
		pPacket->m_OffSet -= cbBuf;
		memcpy((BYTE *)(pPacket->m_pBuffer + pPacket->m_OffSet) , (BYTE *)pbBuf, cbBuf);
		Assert( pPacket->m_OffSet + pPacket->m_cbBuffer + cbBuf <= pPacket->m_cBufferSize );
		pPacket->m_cbBuffer += cbBuf;
	}

	// Increments the Offset
	friend void SNIPacketIncrementOffset(SNI_Packet * pPacket, DWORD dwOffSet)
	{
#ifndef SNI_BASED_CLIENT
		// If we adjust the offset or the data size, then zeroing the memory
		// will not clear everything expected. We expect this flag is never set
		// when offset or data size is being changed.
		Assert(!pPacket->m_fZeroPayloadOnRelease);
#endif // ifndef SNI_BASED_CLIENT

		pPacket->m_OffSet += dwOffSet;
	}

	// Decrements the Offset
	friend void SNIPacketDecrementOffset(SNI_Packet * pPacket, DWORD dwOffSet)
	{
#ifndef SNI_BASED_CLIENT
		// If we adjust the offset or the data size, then zeroing the memory
		// will not clear everything expected. We expect this flag is never set
		// when offset or data size is being changed.
		Assert(!pPacket->m_fZeroPayloadOnRelease);
#endif // ifndef SNI_BASED_CLIENT

		Assert( dwOffSet <= pPacket->m_OffSet );
		pPacket->m_OffSet -= dwOffSet;
	}

	// Sets the size of the data buffer to the specified size
	friend void SNIPacketSetBufferSize(SNI_Packet * pPacket, DWORD dwSize)
	{
#ifndef SNI_BASED_CLIENT
		// If we adjust the offset or the data size, then zeroing the memory
		// will not clear everything expected. We expect this flag is never set
		// when offset or data size is being changed.
		Assert(!pPacket->m_fZeroPayloadOnRelease);
#endif // ifndef SNI_BASED_CLIENT

		Assert( dwSize+pPacket->m_OffSet <= pPacket->m_cBufferSize );
		pPacket->m_cbBuffer = dwSize;
	}

	// Get the size of the data buffer
	friend DWORD SNIPacketGetBufferSize(SNI_Packet * pPacket)
	{
		return pPacket->m_cbBuffer;
	}

	friend LPVOID SNIPacketGetKey(SNI_Packet * pPacket)
	{
		return pPacket->m_pKey;
	}

	friend SOS_IOCompRequest * SNIPacketGetSosObject(SNI_Packet * pPacket)
	{
		return pPacket;
	}
	
	friend void SNIPacketSetKey(SNI_Packet * pPacket, LPVOID pKey)
	{
		pPacket->m_pKey = pKey;
	}
	
	friend DWORD SNIPacketGetBufUnusedSize(SNI_Packet * pPacket, SNI_Packet_IOType IOType)
	{
		Assert(IOType < SNI_Packet_InvalidType);
		Assert(	(IOType != SNI_Packet_KeyHolderNoBuf) || 
				(	(NULL == pPacket->m_pBuffer) &&
					(0 == pPacket->m_cBufferSize) && 
					(0 == pPacket->m_cbBuffer) ) );
		
		if ( SNI_Packet_Write == IOType )
		{
			return pPacket->m_pConn->m_ConnInfo.ConsBufferSize - pPacket->m_cbBuffer;
		}
		else	// IOType is SNI_Packet_Read or SNI_Packet_KeyHolderNoBuf or SNI_Packet_VaryingBuffer*
		{
			return pPacket->m_cBufferSize - pPacket->m_cbBuffer;
		}
	}

	// Delete a packet
	friend void SNIPacketDelete(SNI_Packet * pPacket)
	{
		delete pPacket;
	}

	// Do a PostQueuedCompletionStatus on this packet
	friend DWORD SNIPacketPostQCS(SNI_Packet * pPacket, DWORD numBytes)
	{
#ifndef SNI_BASED_CLIENT
		SOS_Node * pNode = SOS_Node::GetCurrent();
		if( SOS_OK == pNode->PostIOCompletion (numBytes, pPacket) )
			return ERROR_SUCCESS;
		else
			return ERROR_FAIL;		
#else
		if( PostQueuedCompletionStatus( ghIoCompletionPort, numBytes, 0, SNIPacketOverlappedStruct(pPacket)) )
			return ERROR_SUCCESS;
		else
			return ERROR_FAIL;
#endif
	}

	friend void SNIPacketSetNext(SNI_Packet * pOld, SNI_Packet * pNew)
	{
		pOld->m_pNext = pNew;
	}

	friend SNI_Packet * SNIPacketGetNext(SNI_Packet * pOld)
	{
		return pOld->m_pNext;
	}

	friend int SNIPacketGetBidId(__in SNI_Packet * pPacket)
	{
		return pPacket->m_iBidId; 
	}

	friend int * SNIPacketGetBidIdPtr(__in SNI_Packet * pPacket)
	{
		return &pPacket->m_iBidId; 
	}

	friend void SNIPacketSetBuffer(	SNI_Packet * pPacket, 
										__in_bcount(cbBuf) BYTE * pbBuf, 
										DWORD cbBuf, 
										DWORD numBytes)
	{
		Assert(pPacket);
		Assert(	(SNI_Packet_VaryingBufferRead == pPacket->m_IOType) || 
				(SNI_Packet_VaryingBufferWrite == pPacket->m_IOType));

		pPacket->m_pBuffer = pbBuf;
		pPacket->m_cBufferSize = cbBuf;
		pPacket->m_cbBuffer = numBytes;
	}
	
	friend BYTE* SNIPacketGetBuffer(SNI_Packet * pPacket)
	{
		Assert(pPacket);
		return pPacket->m_pBuffer;
	}

	friend DWORD SNIPacketGetActualBytes(SNI_Packet * pPacket)
	{
		Assert(pPacket);
		return pPacket->GetActualBytes();
	}
	
};	

#endif
