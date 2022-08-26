//*********************************************************************
//		Copyright (c) Microsoft Corporation.
//
// @File: via.cpp
// @Owner: nantu | petergv
// @Test: milu
//
// <owner current="true" primary="true">nantu</owner>
// <owner current="true" primary="false">petergv</owner>
//
// Purpose: 
//		Implement SNI entry points for VIA provider
//
// Notes:
//	
// @EndHeader@
//****************************************************************************	

#include "snipch.hpp"
#include "via.hpp"
#ifdef SNI_BASED_CLIENT
#include "nlregc.h"
#endif

extern WCHAR gwszComputerName[];

LONG gdwNumConns = 0;
Via * gpViaConns[MAX_VIA_CONNS];

class ViaNicObject;
class ViaInitObj;

ViaInitObj * gpViaInitObj = NULL;


class ViaInitObj
{
	friend DWORD  Via::ConnectionOpen( bool fSync, ProtElem *pProtElem);


	ViaNicObject * m_rgViaNicObject;
	ViaVendor m_Vendor;
	HMODULE m_hDll;
	SNICritSec * m_CritSec;
	BOOL m_fSetup;
	BOOL m_fLibraryLoaded;

	LONG m_cRef;

	~ViaInitObj()
	{
		Assert( this == gpViaInitObj );

		if( NULL != m_CritSec )
		{
			DeleteCriticalSection(&m_CritSec);
		}

		if( !m_fLibraryLoaded )
		{
			Assert( !m_fSetup );
			return;
		}
		
		TerminateWorkers();
		
		Assert( m_hDll );

		FreeLibrary(m_hDll);
	}

public:

	ViaInitObj() : 
		m_hDll(0), 
		m_fSetup(FALSE),
		m_fLibraryLoaded(FALSE),
		m_cRef(1),
		m_rgViaNicObject(0), 
		m_CritSec(NULL)
	{
	}


	DWORD FInit()
	{
		BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );
		
		DWORD dwRet;
		
		if(ERROR_SUCCESS != (dwRet = SNICritSec::Initialize( &m_CritSec )))
		{
			SNI_SET_LAST_ERROR(VIA_PROV, SNIE_4, dwRet);
			goto ErrorExit;
		}
		
	ErrorExit:

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
		
		return dwRet;
	}

	void AddRef()
	{
		LONG cRef = InterlockedIncrement(&m_cRef);
		
		Assert( cRef > 1 );
	}

	void Release()
	{
		if( 0 == InterlockedDecrement(&m_cRef) )
		{
			delete this;
			gpViaInitObj = NULL;
		}
	}

	DWORD LoadViaLibrary(__in LPCWSTR wszVendor, __in LPCSTR szDll);
	DWORD Setup(__in LPCWSTR wszVendor, __in LPCSTR szDll);
	void TerminateWorkers();

};

class ViaNic
{
	VIP_NIC_HANDLE 			m_hNic;
	VIP_PROTECTION_HANDLE 	m_hPTag;

	LONG					m_cRef;

	~ViaNic()
	{
		VIP_RETURN status;

		if( m_hPTag )
		{
			Assert( m_hNic );
			
			status = Via::VipDestroyPtag( m_hNic, m_hPTag);

			Assert( status == VIP_SUCCESS );
		}

		if( m_hNic )
		{
			status = Via::VipCloseNic(m_hNic);

			Assert( status == VIP_SUCCESS );
		}
		
		Assert( 0 == m_cRef );

		gpViaInitObj->Release();
	}
	
public:

	ViaNic():
		m_hNic(0),
		m_hPTag(0),
		m_cRef(1)
	{
		gpViaInitObj->AddRef();
	}

	DWORD Initialize( __in LPCSTR szNic )
	{
		DWORD dwRet;
		
		VIP_RETURN status;

		status = Via::VipOpenNic(szNic, &m_hNic);
		
		if( status != VIP_SUCCESS )
		{
			dwRet = MapVipErrorToSniError(status);
			goto ErrorExit;
		}

		status = Via::VipCreatePtag(m_hNic, &m_hPTag);
		
		if (status != VIP_SUCCESS ) 
		{
			dwRet = MapVipErrorToSniError(status);
			goto ErrorExit;
		}

		return ERROR_SUCCESS;
		
	ErrorExit:

		return dwRet;
	}
	
	void AddRef()
	{
		LONG cRef = InterlockedIncrement(&m_cRef);
		
		Assert( cRef > 1 );
	}

	void Release()
	{
		if( 0 == InterlockedDecrement(&m_cRef) )
		{
			delete this;
		}
	}

	inline VIP_NIC_HANDLE GetNicHandle()
	{
		return m_hNic;
	}

	inline VIP_NIC_HANDLE GetPTagHandle()
	{
		return m_hPTag;
	}
};

class ViaWorker
{
	friend class Via;

	ViaNic			* m_pViaNic;
	VIP_CQ_HANDLE 	m_hCQ;
	LONG			m_cRef;
	LONG			m_fTerminate;

public:

	ViaWorker( ViaNic *pViaNic, VIP_CQ_HANDLE hCQ ):
		m_pViaNic(pViaNic),
		m_hCQ(hCQ),
		m_cRef(1),
		m_fTerminate(0)
	{
		m_pViaNic->AddRef();
	}

	~ViaWorker()
	{
		Assert( m_cRef == 0 );

		Assert( m_fTerminate );

		VIP_RETURN status;
		
		status = Via::VipDestroyCQ( m_hCQ ); 

		Assert( status == VIP_SUCCESS );

		m_pViaNic->Release();
	}

	void AddRef()
	{
		LONG cRef = InterlockedIncrement(&m_cRef);
		
		Assert( cRef > 1 );
	}

	void Release()
	{
		LONG cRef = InterlockedDecrement(&m_cRef);

		Assert( cRef > 0 || ( cRef == 0) && m_fTerminate );
	}

	void Terminate()
	{
		LONG fTerminatedBefore;
		
		fTerminatedBefore = InterlockedCompareExchange(&m_fTerminate, 1, 0);

		Assert( !fTerminatedBefore );

		Release();
	}
	
#ifdef SNI_BASED_CLIENT
	static DWORD WINAPI CompThread(__in PVOID pParam)
#else
	static PVOID WINAPI CompThread(PVOID pParam)
#endif
	{
		ViaWorker * pViaWorker = (ViaWorker *) pParam;

		VIP_RETURN status;
		bool fQuit = false;
		DWORD dwFaultCounter = 0;

#ifndef SNI_BASED_CLIENT
		Assert (SOS_Task::IsInPreemptiveMode ());
		DO_PERMANENT_TASK_LEAK_DETECTION_IN_THIS_SCOPE
#endif

		while(1)
		{
			VIP_VI_HANDLE       hVi = 0;
			VIP_BOOLEAN         fRecvQ;

#ifndef SNI_BASED_CLIENT
			EX_TRYHANDLER(EX_ANY, EX_ANY, EX_ANY, hdl_backout)
			{
#endif
				//
				// when ref count reaches zero it is time to quit
				//
				
				if( pViaWorker->m_cRef == 0 )
				{
					delete pViaWorker;

					return 0;
				}

				status = Via::VipCQWait(pViaWorker->m_hCQ, MAX_TIMEOUT, &hVi, &fRecvQ);

				if( status == VIP_SUCCESS )
				{
					if ( hVi == NULL )
					{
						BidTrace0 ( ERROR_TAG _T("VipCQWait returned NULL VI handle hVi\n") );

						if( 100 < dwFaultCounter++  )
						{
							BidTrace0 ( ERROR_TAG _T("Stop Via CompThread due to invalid VI handle hVi\n") );
							break;	
						}
						
						Sleep(0);
						continue;
					}

					dwFaultCounter = 0;
				
					// Send completion
					if( fRecvQ )
					{
						Via::HandleReceiveEvent(hVi, &fQuit);

						if( true == fQuit)
						{
							Assert( 0 && " Via provider is in an unknown state\n" );
							BidTrace0( ERROR_TAG _T("Via provider is in an unknown state\n") );
							break;
						}
					}
					else
					{
						Via::HandleSendEvent(hVi);
					}
				}
				else if( status == VIP_TIMEOUT )
				{
					Assert( hVi==0);
				}
				else
				{
					Assert( status == VIP_INVALID_PARAMETER );
					Assert( 0 && " Via provider is in an unknown state\n" );
					BidTrace0( ERROR_TAG _T("Via provider is in an unknown state\n") );
				}

#ifndef SNI_BASED_CLIENT
			}
			EX_CATCH
			{
				// If an exception, we will just set an internal error, along with an error
				// message indicating that an exception was caught
				SNI_SET_LAST_ERROR( VIA_PROV, SNIE_35, ERROR_INTERNAL_ERROR );	
			}
			EX_CATCH_END
#endif
		}
		
		return 0;
     }
};

class ViaNicObject
{
	friend DWORD  Via::ConnectionOpen( 	bool fSync, ProtElem *pProtElem);

	ViaNic			* m_pViaNic;
	ViaWorker		* m_pViaWorker;
	
public:

	ViaNicObject( __in ViaNic * pViaNic, ViaWorker * pViaWorker ):
		m_pViaNic(pViaNic),
		m_pViaWorker(pViaWorker)
	{
	}

	~ViaNicObject()
	{
		m_pViaWorker->Terminate();

		m_pViaNic->Release();
	}
};

DWORD ViaInitObj::LoadViaLibrary(__in LPCWSTR wszVendor, __in LPCSTR szDll)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("wszVendor: \"%s\", szDll: \"%hs\"\n"), 
					wszVendor, szDll );
	
	DWORD dwRet = ERROR_SUCCESS;

	CAutoSNICritSec a_cs( m_CritSec, SNI_AUTOCS_DO_NOT_ENTER );

	a_cs.Enter();

	if( m_fLibraryLoaded )
		goto ErrorExit;

	if( !wszVendor || !szDll )
	{
		dwRet = ERROR_INVALID_PARAMETER;
		goto ErrorExit;
	}

	else if( !_wcsicmp_l(wszVendor, L"QLogic", GetDefaultLocale()) )
		m_Vendor = QLogic;

	else
		m_Vendor = OtherViaVendor;
	
	if(NULL == (m_hDll = SNILoadSystemLibA(szDll)) )
	{
		dwRet = GetLastError();
		goto ErrorExit; 
	}

	for(int i=0; i < 31;i++)
	{
		if(NULL == (Via::s_pfnProcs[i] = GetProcAddress(m_hDll,Via::s_szProcNames[i])))
		{
			dwRet = GetLastError();
			goto ErrorExit; 
		}
	}

	m_fLibraryLoaded = TRUE;
	
ErrorExit:

	if( !m_fLibraryLoaded && m_hDll )
	{
		FreeLibrary( m_hDll );
		m_hDll = NULL;
	}

	a_cs.Leave(); 


	if( ERROR_SUCCESS != dwRet )
		SNI_SET_LAST_ERROR(VIA_PROV, SNIE_4, dwRet);

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD ViaInitObj::Setup(__in LPCWSTR wszVendor, __in LPCSTR szDll)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("wszVendor: \"%s\", szDll: \"%hs\"\n"), 
					wszVendor, szDll );
	
	DWORD dwRet = ERROR_SUCCESS;

	CAutoSNICritSec a_cs( m_CritSec, SNI_AUTOCS_DO_NOT_ENTER );

	a_cs.Enter();

	if( m_fSetup )
		goto ErrorExit;

	dwRet = LoadViaLibrary( wszVendor, szDll);

	if( dwRet != ERROR_SUCCESS )
	{
		goto ErrorExit;
	}
	
	// Initialize the NIC
	CHAR szNic[128];

	if( QLogic == m_Vendor )
	{
		if(FAILED(StringCchPrintf_lA( szNic, CCH_ANSI_STRING(szNic), "\\\\.\\\\NIC%d", GetDefaultLocale(), gdwSvrNic)))
		{
			dwRet = ERROR_INVALID_PARAMETER;
			goto ErrorExit;
		}	
	}
	
	else
	{
		if(FAILED(StringCchPrintf_lA( szNic, CCH_ANSI_STRING(szNic), "NIC%d", GetDefaultLocale(), gdwSvrNic)))
		{
			dwRet = ERROR_INVALID_PARAMETER;
			goto ErrorExit;
		}
	}

	ViaNic *pViaNic;
	
	pViaNic = NewNoX(gpmo) ViaNic;
	
	if( !pViaNic )
	{
		dwRet = ERROR_OUTOFMEMORY;
		goto ErrorExit;
	}		

	dwRet = pViaNic->Initialize( szNic );

	if( dwRet != ERROR_SUCCESS )
	{
		pViaNic->Release();
		
		goto ErrorExit;
	}
	
	if( QLogic == m_Vendor )
	{
		if( !Via::InitQLogicNameService( pViaNic->GetNicHandle()))
		{
			pViaNic->Release();

			dwRet = ERROR_FAIL;
			goto ErrorExit;
		}
	}

	VIP_CQ_HANDLE hCQ;

	VIP_RETURN status;
	
	status = Via::VipCreateCQ(	pViaNic->GetNicHandle(), 
								MAX_CONNS*8, 
								&hCQ);
	if( status != VIP_SUCCESS )
	{
		pViaNic->Release();

		dwRet = MapVipErrorToSniError( status );
		goto ErrorExit;
	}

	ViaWorker * pViaWorker;
	
	pViaWorker = NewNoX(gpmo) ViaWorker( pViaNic, hCQ );

	if( !pViaWorker )
	{
		status = Via::VipDestroyCQ( hCQ ); 

		Assert( status == VIP_SUCCESS );

		pViaNic->Release();

		dwRet = ERROR_OUTOFMEMORY;
		goto ErrorExit;
	}

	dwRet = SNICreateWaitThread(ViaWorker::CompThread, pViaWorker);

	if(ERROR_SUCCESS != dwRet )
	{
		pViaNic->Release();

		pViaWorker->Terminate();

		delete pViaWorker;
		
		goto ErrorExit;
	}

	m_rgViaNicObject = NewNoX(gpmo) ViaNicObject( pViaNic, pViaWorker );

	if( !m_rgViaNicObject )
	{
		pViaNic->Release();

		pViaWorker->Terminate();

		dwRet = ERROR_OUTOFMEMORY;
		goto ErrorExit;
	}

	m_fSetup = TRUE;

	dwRet = ERROR_SUCCESS;
	
ErrorExit:


	if( dwRet != ERROR_SUCCESS )
	{
		TerminateWorkers();
	}
	
	a_cs.Leave(); 

	if( ERROR_SUCCESS != dwRet )
		SNI_SET_LAST_ERROR(VIA_PROV, SNIE_4, dwRet);

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

void ViaInitObj::TerminateWorkers()
{
	if( m_rgViaNicObject )
	{
		delete m_rgViaNicObject;

		m_rgViaNicObject = 0;
	}
}


class ViaListenObject
{
	friend DWORD Via::InitializeListener(	HANDLE hSNIListener, 
											ViaListenInfo * pListenInfo, 
											HANDLE * pListenHandle);

	friend DWORD Via::AcceptDone( SNI_Conn * pConn, 
								 LPVOID pAcceptKey,
								 DWORD dwBytes,
								 DWORD dwRet,
						   		 SNI_Provider ** ppProv,
						   		 LPVOID * ppAcceptInfo );

	LPOVERLAPPED 			m_pOvl;
	VIP_CONN_HANDLE 		m_hConnHandle;

	SOS_EventAuto			m_AcceptDoneEvent;

	VIP_NET_ADDRESS 		* m_pClientAddress;
	VIP_NET_ADDRESS 		* m_pLocalAddress;

	ViaNic					* m_pViaNic;
	
	DWORD 					m_nWorkers;
	
	ViaWorker 				* m_rgWorkers[MAX_NODES];
	
	LONG 					m_fTerminate;

	ViaListenInfo				m_ViaListenInfo;


public:

	ViaListenObject():
		m_pOvl(0),
		m_hConnHandle(INVALID_HANDLE_VALUE),
		m_AcceptDoneEvent(EVENT_NOTSIGNALED),
		m_pClientAddress(0),
		m_pLocalAddress(0),
		m_pViaNic(0),
		m_nWorkers(0),
		m_fTerminate(0)
	{
	}

	~ViaListenObject()
	{
		Assert( m_nWorkers == 0 );
		
		if( m_pClientAddress )
		{
			delete [] (char *) m_pClientAddress;
		}

		if( m_pLocalAddress )
		{
			delete [] (char *) m_pLocalAddress;
		}

		Assert( m_hConnHandle == INVALID_HANDLE_VALUE);
		
		if( m_pViaNic )
		{
			m_pViaNic->Release();

			m_pViaNic = 0;
		}

		Assert( NULL == m_pOvl );
	}

	DWORD Initialize( const ViaListenInfo *pListenInfo )
	{
		DWORD dwRet;
		
		CHAR szNic[128];

		m_ViaListenInfo.wszVendor[MAX_NAME_SIZE] = L'\0';
		m_ViaListenInfo.szVendorDll[MAX_NAME_SIZE] = '\0';
		m_ViaListenInfo.szDiscriminator[MAX_NAME_SIZE] = '\0';
		
		if( (FAILED(StringCchPrintf_lW( m_ViaListenInfo.wszVendor,
					 ARRAYSIZE(m_ViaListenInfo.wszVendor),
					 L"%s", GetDefaultLocale(),
					 pListenInfo->wszVendor))) ||
			(FAILED(StringCchPrintf_lA( m_ViaListenInfo.szVendorDll,
					 CCH_ANSI_STRING(m_ViaListenInfo.szVendorDll),
					 "%s", GetDefaultLocale(),
					 pListenInfo->szVendorDll))) ||
			(FAILED(StringCchPrintf_lA( m_ViaListenInfo.szDiscriminator,
					 CCH_ANSI_STRING(m_ViaListenInfo.szDiscriminator),
					 "%s", GetDefaultLocale(),
					 pListenInfo->szDiscriminator))) )
		{
			dwRet = ERROR_INVALID_PARAMETER;
			SNI_SET_LAST_ERROR( VIA_PROV, SNIE_5, dwRet );
			goto ErrorExit;
		}

		m_ViaListenInfo.NicId = pListenInfo->NicId;

		//SQL BU 409370:
		//Disable QLOGIC special case. Both unisys and qlogic work with this code path.
		//if( !_stricmp_l( "QLogic", pListenInfo->szVendor , GetDefaultLocale()) )
		{
			if(FAILED(StringCchPrintf_lA( szNic, CCH_ANSI_STRING(szNic), "\\\\.\\\\NIC%d", GetDefaultLocale(), pListenInfo->NicId)))
			{
				dwRet = ERROR_INVALID_PARAMETER;
				goto ErrorExit;
			}	
		}
		
		//else
		//{
		//	if(FAILED(StringCchPrintf_lA( szNic, CCH_ANSI_STRING(szNic), "NIC%d", GetDefaultLocale(), pListenInfo->NicId)))
		//	{
		//		dwRet = ERROR_INVALID_PARAMETER;
		//		goto ErrorExit;
		//	}
		//}

		m_pViaNic = NewNoX(gpmo) ViaNic();

		if( !m_pViaNic )
		{
			dwRet = ERROR_OUTOFMEMORY;

			goto ErrorExit;
		}

		dwRet = m_pViaNic->Initialize( szNic );

		if( dwRet != ERROR_SUCCESS )
		{
			goto ErrorExit;
		}

		VIP_NIC_ATTRIBUTES nicAttr;

		VIP_RETURN status;
		
		if( VIP_SUCCESS != (status = Via::VipQueryNic( 	m_pViaNic->GetNicHandle(), 
														&nicAttr)) ) 
		{
			dwRet = MapVipErrorToSniError(status);
			goto ErrorExit;
		}

		
		//if( !_stricmp_l( "QLogic", pListenInfo->szVendor, GetDefaultLocale()) )
		{
			/// can we start multiple times???
			if( !Via::InitQLogicNameService(m_pViaNic->GetNicHandle()))
			{
				dwRet = MapVipErrorToSniError(status);
				goto ErrorExit;
			}
		}

		DWORD dwPort = _atoi_l(pListenInfo->szDiscriminator, GetDefaultLocale());
		if( 0 == dwPort )
		{
			dwRet = ERROR_INVALID_PARAMETER;
			goto ErrorExit;
		}

 		m_pClientAddress = (VIP_NET_ADDRESS *)NewNoX(gpmo) char[4+nicAttr.NicAddressLen+nicAttr.MaxDiscriminatorLen];
		if( !m_pClientAddress)
		{
			dwRet = ERROR_OUTOFMEMORY;
			goto ErrorExit;
		}

		m_pClientAddress->HostAddressLen = nicAttr.NicAddressLen;
		m_pClientAddress->DiscriminatorLen = nicAttr.MaxDiscriminatorLen;
 		m_pLocalAddress = (VIP_NET_ADDRESS *)NewNoX(gpmo) char[4+nicAttr.NicAddressLen+DSCR_LEN];
		if( !m_pLocalAddress)
		{
			dwRet = ERROR_OUTOFMEMORY;
			goto ErrorExit;
		}

		// Set the address and start listening on it
		m_pLocalAddress->HostAddressLen = nicAttr.NicAddressLen;
		m_pLocalAddress->DiscriminatorLen = DSCR_LEN;

		memcpy((char *)m_pLocalAddress->HostAddress, nicAttr.LocalNicAddress, nicAttr.NicAddressLen);

		DWORD len = (DWORD) wcslen(gwszComputerName);
OACR_WARNING_PUSH
OACR_WARNING_DISABLE(SYSTEM_LOCALE_MISUSE , " INTERNATIONALIZATION BASELINE AT KATMAI RTM. FUTURE ANALYSIS INTENDED. ")
		DWORD cch = LCMapStringW(LOCALE_SYSTEM_DEFAULT,
				LCMAP_UPPERCASE,
				gwszComputerName,
				-1,
				gwszComputerName,
				len+1);
OACR_WARNING_POP
		
		Assert( cch <= len+1 );
		if( 0 >=  cch || cch >  len+1 )
		{
			dwRet = ERROR_INVALID_PARAMETER;
			SNI_SET_LAST_ERROR(VIA_PROV, SNIE_ARGUMENT, dwRet);
			goto ErrorExit;
		}
		gwszComputerName[len] = 0;
			
		DWORD cszComputerName = Hash(gwszComputerName);
		
		memcpy((char *)m_pLocalAddress->HostAddress + nicAttr.NicAddressLen, &cszComputerName, sizeof(DWORD));

		memcpy((char *)m_pLocalAddress->HostAddress + nicAttr.NicAddressLen+sizeof(DWORD), &dwPort, sizeof(dwPort));


		scierrlog(26032, m_ViaListenInfo.wszVendor, dwPort);		
		return ERROR_SUCCESS;
		
	ErrorExit:
		scierrlog(26033,dwRet);		

		return dwRet;
	}


	DWORD SpawnWorkerThreads( Affinity affinity )
	{
		Assert( affinity != 0 );
		Assert( !SOS_Node::GetCurrent()->IsDACNode());

		DWORD dwRet;
		
		for( int i = 0; i < MAX_NODES; i++ )
		{
			if( 0 == (affinity & static_cast<Affinity>(1)<<i))
			{
				m_rgWorkers[i] = 0;
				continue;
			}
			
			SOS_NodeEnum enumNode;

			SOS_Node *pNode;

			for( pNode = enumNode.GetNext(NULL); pNode; pNode = enumNode.GetNext(pNode))
			{
				Assert( !pNode->IsDACNode());
				
				if( i == pNode->GetNodeId())
				{
					break;
				}
			}

			if( !pNode )
			{
				Assert( 0 && " Via provider is in an unknown state\n" );
				BidTrace0( ERROR_TAG _T("Via provider is in an unknown state\n") );
				TerminateWorkers();

				return ERROR_INVALID_PARAMETER;
			}

			VIP_RETURN 		status;
			VIP_CQ_HANDLE 	hCQ;
			
			status = Via::VipCreateCQ( m_pViaNic->GetNicHandle(), MAX_CONNS*8, &hCQ);

			if( status != VIP_SUCCESS )
			{
				Assert( status != VIP_INVALID_PARAMETER );
				Assert( status == VIP_ERROR_RESOURCE );

				TerminateWorkers();

				dwRet = ERROR_OUTOFMEMORY;

				SNI_SET_LAST_ERROR( VIA_PROV, SNIE_SYSTEM, dwRet )

				return dwRet;
			}

			m_rgWorkers[i] = NewNoX(gpmo) ViaWorker( m_pViaNic, hCQ );
				
			if( NULL == m_rgWorkers[i] )
			{
				status = Via::VipDestroyCQ( hCQ ); 

				Assert( status == VIP_SUCCESS );

				TerminateWorkers();

				dwRet = ERROR_OUTOFMEMORY;

				SNI_SET_LAST_ERROR( VIA_PROV, SNIE_SYSTEM, dwRet )

				return dwRet;
			}

			SOS_Task * pTask;

			SOS_RESULT result;

			result = pNode->EnqueueTaskDirect (	ViaWorker::CompThread, 
												m_rgWorkers[i], 
												PermanentTask | PreemptiveTask, 
												&pTask);
			
			if (result == SOS_OK)
			{
				pTask->Release ();
			}
			else
			{
				m_rgWorkers[i]->Terminate();

				delete m_rgWorkers[i];

				m_rgWorkers[i] = 0;

				TerminateWorkers();

				return result;
			}

			m_nWorkers++;
		}

		return ERROR_SUCCESS;
	}

	void TerminateWorkers()
	{
		for( int i = 0; i<MAX_NODES; i++ )
		{
			if( m_rgWorkers[i] )
			{
				Assert( m_nWorkers > 0 );
				m_rgWorkers[i]->Terminate();
				m_nWorkers--;
			}
		}

		Assert( m_nWorkers == 0 );
	}

	void Terminate()
	{
		LONG fTerminatedBefore;
		
		fTerminatedBefore = InterlockedCompareExchange(&m_fTerminate, 1, 0);

		Assert( !fTerminatedBefore );
	}

#ifdef SNI_BASED_CLIENT
	static DWORD WINAPI ListenThread(__inout PVOID pParam)
#else
	static PVOID WINAPI ListenThread(PVOID pParam)
#endif
	{

#ifndef SNI_BASED_CLIENT
	Assert (SOS_Task::IsInPreemptiveMode ());
	DO_PERMANENT_TASK_LEAK_DETECTION_IN_THIS_SCOPE
#endif

		ViaListenObject * pListenObject = (ViaListenObject *) pParam;

		VIP_RETURN status;

		VIP_VI_ATTRIBUTES remote_attribs;

		VIP_NIC_HANDLE hNic = pListenObject->m_pViaNic->GetNicHandle();

		while(1)
		{
#ifndef SNI_BASED_CLIENT
		EX_TRYHANDLER(EX_ANY, EX_ANY, EX_ANY, hdl_backout)
		{
#endif			
				// Check if we need to shutdown
				if( pListenObject->m_fTerminate )
				{
					pListenObject->TerminateWorkers();
			
					SNIDeleteAccept( pListenObject->m_pOvl);

					pListenObject->m_pOvl = 0;

					delete pListenObject;

					return 0;
				}

				status = Via::VipConnectWait( 	hNic,
												pListenObject->m_pLocalAddress, 
												MAX_TIMEOUT,
												pListenObject->m_pClientAddress,
												&remote_attribs, 
												&pListenObject->m_hConnHandle);

				
				if(status == VIP_SUCCESS)
				{   
					((SOS_IOCompRequest *) pListenObject->m_pOvl )->ExecuteCompRoutine();

					SOS_WaitInfo waitInfo (PWAIT_VIA_ACCEPT_DONE);

					SOS_RESULT result;

					result = pListenObject->m_AcceptDoneEvent.Wait(INFINITE,&waitInfo);

					Assert( result == SOS_OK );
				}
				else if( status != VIP_TIMEOUT )
				{
					Assert( status == VIP_ERROR_RESOURCE );
				}

				Assert( pListenObject->m_hConnHandle == INVALID_HANDLE_VALUE );

#ifndef SNI_BASED_CLIENT
			}
			EX_CATCH
			{
				// If an exception, we will just set an internal error, along with an error
				// message indicating that an exception was caught
				SNI_SET_LAST_ERROR( VIA_PROV, SNIE_35, ERROR_INTERNAL_ERROR );	
			}
			EX_CATCH_END
#endif
		}
	}
};

Via::Via(SNI_Conn * pConn) :
	SNI_Provider(pConn),
	m_SyncWriteCritSec(0),
	CritSec(0),
	SendBufState(EMPTY),
	SeqNum(0),
	DestSeqNum(0),
	SendLimit(MAX_SVR_BUFS + 1),
	LastSendLimit(MAX_SVR_BUFS + 1),
	DestSendLimit(MAX_SVR_BUFS + 1),
	fCleanupCalled(0),
	fClean(FALSE),
	fConnClosed(FALSE),
	fDisconnected(FALSE),
	fClosed(FALSE),
	m_hMem(0),
	m_pbMem(0),
	m_pViaWorker(0),
	m_fReadPosted(FALSE),
	m_fPendingAck(FALSE),
	m_cRecvs(0),
	m_iSendDesc(0),
	m_cbWritePendingOffset(0),
	m_hVi(NULL)
{
	m_Prot = VIA_PROV;

	m_Id = InterlockedIncrement(&gdwNumConns) % MAX_VIA_CONNS;
	
	gpViaConns[m_Id] = this;

	BidObtainItemID2A( &m_iBidId, SNI_ID_TAG "%p{.} created by %u#{SNI_Conn}", 
		this, pConn->GetBidId() );
}

DWORD Via::FInit()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	DWORD dwRet = ERROR_SUCCESS;

	if(ERROR_SUCCESS != (dwRet = SNICritSec::Initialize( &CritSec )))
    		goto ErrorExit;
	
	if(ERROR_SUCCESS != (dwRet = SNICritSec::Initialize( &m_SyncWriteCritSec )))
		goto ErrorExit;

	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	hSyncReadEvent = CreateEvent( NULL, FALSE, FALSE, NULL);

	if( !hWriteEvent || !hSyncReadEvent )
	{
		dwRet = GetLastError();
		goto ErrorExit;
	}
    
	m_dwPendingWrites = 0;
	m_dwQuickWrites = 0;
	m_dwAcksSent = 0;
	m_dwAcksRecd = 0;
	m_dwPendingReads = 0;
	m_dwQuickReads = 0;
	
ErrorExit:

	if(ERROR_SUCCESS != dwRet)
		SNI_SET_LAST_ERROR(VIA_PROV, SNIE_4, dwRet);

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
		
	return dwRet;
}

/// clean up correctly when FInit fails
Via::~Via()
{
	BidRecycleItemIDA( &m_iBidId, SNI_ID_TAG ); 

    	if( CritSec)
    		DeleteCriticalSection( &CritSec );

	if( m_SyncWriteCritSec )
		DeleteCriticalSection( &m_SyncWriteCritSec );
	
	while( !pReadDoneQ.IsEmpty() )
		pReadDoneQ.DeQueue();   

	if( m_hVi )
	{
		VipDestroyVi(m_hVi);
		m_hVi = NULL;
	}
	
	if( hWriteEvent )
		CloseHandle(hWriteEvent);

	if( hSyncReadEvent )
		CloseHandle(hSyncReadEvent);

	if( m_pViaWorker )
	{
		if(m_pbMem != NULL)
		{
			VipDeregisterMem( m_pViaWorker->m_pViaNic->GetNicHandle(), m_pbMem, m_hMem);
		}

		m_pViaWorker->Release();

		m_pViaWorker = NULL;
	}

	delete m_pbMem;

	m_pbMem = NULL;

	gpViaConns[m_Id] = 0;
}

DWORD Via::Initialize(PSNI_PROVIDER_INFO pInfo)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("pInfo: %p{PSNI_PROVIDER_INFO}\n"), pInfo );
	
	DWORD dwRet = ERROR_SUCCESS;
	
	if( !(gpViaInitObj = NewNoX(gpmo) ViaInitObj()) )
	{
		dwRet = ERROR_OUTOFMEMORY;
		SNI_SET_LAST_ERROR(VIA_PROV, SNIE_OOM, dwRet);
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
		return dwRet;
	}

	if( ERROR_SUCCESS != (dwRet = gpViaInitObj->FInit()) )
	{
		// The release method sets gpViaInitObj to NULL if the
		// object is deleted.  
		//
		gpViaInitObj->Release();
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);		
		return dwRet;
	}
		
	srand((unsigned)time(NULL));

	   	pInfo->fBaseProv = TRUE;
	pInfo->Size = 0;
	pInfo->Offset = 0;
	pInfo->ProvNum = VIA_PROV;
	pInfo->fInitialized = TRUE; 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
		
	return dwRet;
}

DWORD Via::Terminate()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T( "\n") );

	if( !rgProvInfo[VIA_PROV].fInitialized )
	{
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
		return ERROR_SUCCESS;
	}

	gpViaInitObj->TerminateWorkers();
	
	gpViaInitObj->Release();

	rgProvInfo[VIA_PROV].fInitialized = FALSE; 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
		
	return ERROR_SUCCESS;
}

DWORD Via::InitializeListener(HANDLE hSNIListener, ViaListenInfo * pListenInfo, __out HANDLE * pListenHandle)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("hSNIListener: %p{HANDLE}, ")
							  _T("pListenInfo: %p{ViaListenInfo*}, ")
							  _T("pListenHandle: %p{HANDLE*}\n"), 
					hSNIListener, pListenInfo, pListenHandle);
	
	DWORD dwRet = ERROR_FAIL;
	ViaListenObject * pListenObject = NULL; 

	// If Via::Initialize() failed before, return fail
	//
	if( NULL == gpViaInitObj )
	{
		dwRet = ERROR_INVALID_STATE;
		SNI_SET_LAST_ERROR(VIA_PROV, SNIE_SYSTEM, ERROR_INVALID_STATE);
		goto ErrorExit;
	}

	pListenObject = NewNoX(gpmo) ViaListenObject;
	if( !pListenObject )
	{
		dwRet = ERROR_OUTOFMEMORY;
		SNI_SET_LAST_ERROR(VIA_PROV, SNIE_OOM, dwRet);
		goto ErrorExit;
	}

	dwRet = gpViaInitObj->LoadViaLibrary(pListenInfo->wszVendor, pListenInfo->szVendorDll);

	if( ERROR_SUCCESS != dwRet )
	{
		scierrlog(26055, pListenInfo->szVendorDll, dwRet);
		goto ErrorExit;	
	}	

	dwRet = pListenObject->Initialize( pListenInfo );

	if( ERROR_SUCCESS != dwRet )
	{
		goto ErrorExit;
	}

	dwRet = pListenObject->SpawnWorkerThreads(((SNI_Listener *)hSNIListener)->ConsumerInfo.NodeAffinity);

	if(ERROR_SUCCESS != dwRet )
	{
		goto ErrorExit;
	}

	dwRet = SNICreateAccept(hSNIListener, VIA_PROV, pListenObject, &pListenObject->m_pOvl);

	if( ERROR_SUCCESS != dwRet )
	{
		Assert( !pListenObject->m_pOvl );

		pListenObject->TerminateWorkers();

		goto ErrorExit;
	}

	dwRet = SNICreateWaitThread( ViaListenObject::ListenThread, pListenObject );

	if( dwRet != ERROR_SUCCESS )
	{
		pListenObject->TerminateWorkers();

		SNIDeleteAccept( pListenObject->m_pOvl );

		pListenObject->m_pOvl = 0;
		
		goto ErrorExit;
	}

	//
	// after this point listener thread is active and we can't jump to ErrorExit
	// listener thread should release the listener object
	//
	

	*pListenHandle = (HANDLE) pListenObject;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
	
ErrorExit:

	delete pListenObject;
	
	*pListenHandle = NULL;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD Via::TerminateListener(HANDLE hListener)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "hListener: %p{HANDLE}\n"), hListener);
	
	ViaListenObject * pListenObject = (ViaListenObject *) hListener;

	pListenObject->Terminate();

	// listen object will be released by listener thread
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;
}

LONG gdwConnCount = 0;

#ifndef SNI_BASED_CLIENT
	//This function simply error out since server side should not call this function
	DWORD GetViaSettings( __out_bcount(dwcbVendor) LPSTR szVendor, DWORD dwcbVendor, __out_bcount(dwcbVendorDll) LPSTR szVendorDll, DWORD dwcbVendorDll)
	{
		Assert( 0 && " This function is not implmented\n" );
		BidTrace0( ERROR_TAG _T("This function is not implemented\n") );
		SNI_SET_LAST_ERROR(VIA_PROV, SNIE_15, ERROR_NOT_SUPPORTED);

		return ERROR_NOT_SUPPORTED;

    }
#else
DWORD GetViaSettings(__out_ecount(dwcchVendor) LPWSTR wszVendor, DWORD dwcchVendor,  __out_bcount(dwcbVendorDll) LPSTR szVendorDll, DWORD dwcbVendorDll)
{
	//	


	BidxScopeAutoSNI0( SNIAPI_TAG _T(""));

	DWORD dwRet = ERROR_SUCCESS;


	if( dwcchVendor ==0 || dwcchVendor > MAX_NAME_SIZE+1 || dwcbVendorDll ==0 || dwcbVendorDll > MAX_NAME_SIZE+1 )
	{
		dwRet = ERROR_INVALID_DATA;
		goto Exit;
	}
	
	CS_PROTOCOL_PROPERTY propertyViaDefaultServerPort; 

	//	Get default server port.  
	//
	dwRet = static_cast<DWORD>(CSgetProtocolProperty( CS_PROTOCOL_VIA, 
													  CS_PROP_VIA_DEFAULT_PORT, 
													  &propertyViaDefaultServerPort )); 

	if( ERROR_SUCCESS != dwRet )
	{
		goto Exit; 
	}

	if( REG_SZ != propertyViaDefaultServerPort.dwPropertyType )
	{
		dwRet = ERROR_INVALID_DATA; 
		goto Exit; 
	}

	if( propertyViaDefaultServerPort.PropertyValue.szStringValue[0] 
	&& wcschr(propertyViaDefaultServerPort.PropertyValue.szStringValue, L':'))
	{
		LPWSTR wszTmp = wcschr(propertyViaDefaultServerPort.PropertyValue.szStringValue, L':');
		gdwSvrPortNum = _wtoi_l(wszTmp+1, GetDefaultLocale());

		wszTmp[0] = L'\0';
		
		gdwSvrNic = _wtoi_l(propertyViaDefaultServerPort.PropertyValue.szStringValue, GetDefaultLocale());
	}


	
	//	Hard-code the vendor name to "QLogic" and the default DLL name to 
	//	"QLVipl.dll".  In Yukon/Whidbey (SNAC/SNIX) the only supported 
	//	vendor is QLogic.  
	//


	if( FAILED(StringCchCopyW( wszVendor, 
						dwcchVendor, 
						CS_VALUE_VIA_VENDOR_NAME_QLOGIC )) )
	{
		dwRet = ERROR_BUFFER_OVERFLOW; 
		goto Exit; 
	}

	wszVendor[dwcchVendor-1] = 0x00; 


	CS_PROTOCOL_PROPERTY propertyViaVendorDll; 

	szVendorDll[dwcbVendorDll-1] = 0x00; 

	//	Get vendor dll.  
	//
	
	dwRet = static_cast<DWORD>(CSgetProtocolProperty( CS_PROTOCOL_VIA, 
													  CS_PROP_VIA_VENDOR_DLL, 
													  &propertyViaVendorDll )); 

	if( ERROR_SUCCESS != dwRet || REG_SZ != propertyViaVendorDll.dwPropertyType )
	{
		//Regitry is not set, load default DLL.
		if( FAILED (StringCchCopyA( szVendorDll, 
								dwcbVendorDll, 
								CS_VALUE_VIA_VENDOR_DLL_QLOGIC )) )
		{
			dwRet = ERROR_BUFFER_OVERFLOW; 
			goto Exit; 
		}

		dwRet = ERROR_SUCCESS;

	}
	else
	{
	
		char szmbStringValue[MAX_NAME_SIZE + 1];
		int ccbStringValue = WideCharToMultiByte(CP_ACP,0,
													propertyViaVendorDll.PropertyValue.szStringValue, 
													ARRAYSIZE(propertyViaVendorDll.PropertyValue.szStringValue),
													szmbStringValue,
													CCH_ANSI_STRING(szmbStringValue),
													NULL,
													NULL);
		if (ccbStringValue == 0)
		{
			dwRet = GetLastError();
			goto Exit;
		}
		if( FAILED (StringCchCopyA( szVendorDll, 
								dwcbVendorDll, 
								szmbStringValue)) )
		{
			// Try to load with default DLL whenever possible.
			if( FAILED (StringCchCopyA( szVendorDll, 
									dwcbVendorDll, 
									CS_VALUE_VIA_VENDOR_DLL_QLOGIC )) )
			{
				dwRet = ERROR_BUFFER_OVERFLOW; 
				goto Exit; 
			}
		}
	}	


Exit:

	if( ERROR_SUCCESS != dwRet )
	{
		SNI_SET_LAST_ERROR(VIA_PROV, SNIE_SYSTEM, dwRet);
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}
#endif

DWORD Via::Open( 	SNI_Conn 		* pConn,
					ProtElem 		* pProtElem, 
					__out SNI_Provider 	** ppProv )
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("pConn: %p{SNI_Conn*}, ")
							  _T("pProtElem: %p{ProtElem*}, ")
							  _T("ppProv: %p{SNI_Provider**}\n"), 
					pConn, pProtElem, ppProv);
	
	DWORD dwRet = ERROR_FAIL;
	Via *pVia = NULL;
	WCHAR wszVendor[MAX_NAME_SIZE+1];
	char szVendorDll[MAX_NAME_SIZE+1];

	// If Via::Initialize() failed before, return fail
	//
	if( NULL == gpViaInitObj )
	{
		dwRet = ERROR_INVALID_STATE;
		SNI_SET_LAST_ERROR(VIA_PROV, SNIE_SYSTEM, ERROR_INVALID_STATE);
		goto ErrorExit;
	}

	if( ERROR_SUCCESS != (dwRet = GetViaSettings(wszVendor,ARRAYSIZE(wszVendor),  szVendorDll, sizeof(szVendorDll))))
		goto ErrorExit;

	// Check if the NICs have been initialized
	// Note: This needs to be done only once, so if Via::InitlializeListener is called
	// multiple times, we need to verify that they were called for the same vendor
	if( ERROR_SUCCESS != (dwRet = gpViaInitObj->Setup(wszVendor, szVendorDll)) )
		goto ErrorExit;	

	if(NULL == (pVia = NewNoX(gpmo) Via(pConn)))
	{
		dwRet = ERROR_OUTOFMEMORY;
		SNI_SET_LAST_ERROR(VIA_PROV, SNIE_OOM, dwRet);
		goto ErrorExit;
	}

	if( ERROR_SUCCESS != pVia->FInit() )
		goto ErrorExit;

	if(ERROR_SUCCESS != (dwRet = pVia->ConnectionOpen(pConn->m_fSync,pProtElem)))
		goto ErrorExit;
	
	BidUpdateItemIDA( pVia->GetBidIdPtr(), SNI_ID_TAG 
		"connection: %p{ProtElem}", pProtElem );  

	// Set the out provider param to point to the new object
	*ppProv = pVia;

	// Take a ref on the object
	pVia->m_pConn->AddRef( REF_InternalActive );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;
	
ErrorExit:

	delete pVia;

	*ppProv = NULL;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD  Via::ConnectionOpen( 	bool fSync, __in ProtElem *pProtElem)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("fSync: %d{bool}, ")
							  _T("pProtElem: %p{ProtElem*}\n"), 
							  GetBidId(),
							  fSync, 
							  pProtElem);

#define VIP_NET_ADDRESS_ALIGNMENT 8

#ifndef SNI_BASED_CLIENT
	CPL_ASSERT( __alignof(VIP_NET_ADDRESS) <= VIP_NET_ADDRESS_ALIGNMENT );
#endif	

	__declspec(align(VIP_NET_ADDRESS_ALIGNMENT)) char rAddrBuf[256] ;
	__declspec(align(VIP_NET_ADDRESS_ALIGNMENT)) char lAddrBuf[256] ;
	DWORD dwErr = ERROR_SUCCESS;
	VIP_NET_ADDRESS * rAddr = (VIP_NET_ADDRESS*)rAddrBuf;
	VIP_VI_ATTRIBUTES remote_attribs;
	WCHAR wszError[256];
	VIP_VI_ATTRIBUTES vi_attribs ={ VIP_SERVICE_RELIABLE_DELIVERY, /* Reliability level */
	                     32768,     /* MTU */
	                     0,	    /* QOS is unused */
	                     0,	    /* use the default protection tag */
	                     FALSE, /* no RDMA Write */
	                     FALSE  /* no RDMA Read */
	                   };

	VIP_NET_ADDRESS * lAddr = (VIP_NET_ADDRESS *)lAddrBuf;
	DWORD dwTick, dwThread, dwCount;
	VIP_RETURN status = VIP_SUCCESS;
	DWORD dwError;

	// Defaults for server nic, server portnum
	DWORD dwSvrNic = gdwSvrNic;
	DWORD dwSvrPortNum = gdwSvrPortNum;
	LPWSTR wszSvr = NULL;
	LPWSTR wszClusterName = NULL;
	WCHAR wszSvrInfo[1024];
	DWORD cSvrName = 1023;
	LPWSTR wszTmp = NULL;
	DWORD dwHash;
	BOOL fLastNic = FALSE;
	BOOL fAddr = FALSE;
	WCHAR *pwszPort = NULL;
	DWORD dwNic = 0; //
	DWORD dwTimeStart = GetTickCount();
	DWORD dwCurrent = dwTimeStart;
	short cRetry=0;

	Assert(pProtElem->GetProviderNum() == VIA_PROV);
	Assert(pProtElem->Via.Host[0]);

	//strcpy(wszSvrInfo, pProtElem->m_Params.Via.Host);
	wszSvr = pProtElem->Via.Host;
	(void)StringCchCopyW(wszSvrInfo, ARRAYSIZE(wszSvrInfo), pProtElem->Via.Param);

	//hClntNic = s_rgNicInfo[dwNic].hNic; //temporary. parse this from the user connection string
	// The connect string can be of the form,
	// ServerName[:ClusterName,SvrPort,SvrNic] OR
	// ServerAddress,SvrPort,SvrNic (Note: In this case, the SvrNic param is irrelevant)

	// Check if there is a nic:port specified
	if( wcschr(wszSvrInfo, L',') )
	//if(wszSvrInfo[0] != '\0')
	{

		LPWSTR wszTmp2;
		LPWSTR wszPort = NULL;
		//first one is the server name
		//wszSvr = strtok(wszSvrInfo, ",");
		//get the remaining string
		//wszTmp = wszSvrInfo + strlen(wszSvrInfo) + 1;
		wszTmp = wszSvrInfo;

		// See if cluster name exists (as in server:cluster) - and this is not an address(by making sure that there are no more :s)
		if( (NULL != (wszTmp2 = wcschr(wszSvr, L':'))) && (NULL == wcschr((wszTmp2+1), L':')) )
		{
			WCHAR * tokContextSvr = NULL; 
			wszSvr = wcstok_s(wszSvr, L":", &tokContextSvr);
			wszClusterName = wcstok_s(NULL, L"\0", &tokContextSvr);
		}

		// Otherwise ClusterName is same as server
		else
			wszClusterName = wszSvr;

		// Keep the connect string lying around - it may contain additional
		// port,nic combinations. If we can't connect to the first combo, try others
		// before giving up. 
		// Note: See MDAC bug 62905
		// Get the server nic and port num
		WCHAR * tokContextSvrInfoPort = NULL;
		wszPort = wcstok_s(wszTmp, L",", &tokContextSvrInfoPort);
		if(!wszPort)
		{
			dwErr = ERROR_INVALID_PARAMETER;
			SNI_SET_LAST_ERROR(VIA_PROV, SNIE_ARGUMENT, dwErr);
			goto ErrorExit;
		}
		dwSvrPortNum = _wtoi_l(wszPort, GetDefaultLocale());
		wszTmp += (int) wcslen(wszTmp) + 1;
		// Extract the NIC. Also take note if this is the last nic/port combo
		if( NULL == wcsstr(wszTmp, L",") )
		{
			dwSvrNic = _wtoi_l(wszTmp, GetDefaultLocale());
			fLastNic = TRUE;
		}
		else
		{
			WCHAR * tokContextSvrInfoNic = NULL;
			dwSvrNic = _wtoi_l(wcstok_s(wszTmp, L",", &tokContextSvrInfoNic), GetDefaultLocale());
		}
		wszTmp += (int) wcslen(wszTmp) + 1;
	}
	else
	{
		if(pProtElem->Via.Port != 0)
			dwSvrPortNum = pProtElem->Via.Port ;
		wszClusterName = wszSvr;
		//since user hasnt specified any nic:port pairs, assume this is the last pair we will try
		//
		wszTmp = NULL;
		fLastNic = TRUE;
	}

	ViaNicObject * pNic = gpViaInitObj->m_rgViaNicObject;

	VIP_NIC_ATTRIBUTES nicAttr;

	if( VIP_SUCCESS != (status = Via::VipQueryNic( 	pNic->m_pViaNic->GetNicHandle(), 
													&nicAttr)) ) 
	{
		dwErr = MapVipErrorToSniError(status);
		SNI_SET_LAST_ERROR( VIA_PROV, SNIE_SYSTEM, dwErr);
		goto ErrorExit;
	}
		
	DWORD dwStart;
	DWORD dwTimeElapsed = 0;
	
	dwStart = GetTickCount();

	//rAddr points to 
	//typedef struct _VIP_NET_ADDRESS {
	//    		VIP_UINT16 HostAddressLen;
	//			VIP_UINT16 DiscriminatorLen;
	//			VIP_UINT8 HostAddress[1];
	//			} VIP_NET_ADDRESS;
	//and we will set tailing data including NicAddress and one dwHash of wszClusterName and one dwSvrPortNum.
	//
	//Thus,  the size of the buff  should be longer than nicAttr.NicAddressLen + 2*sizeof(VIP_UITN16)+2*sizeof(DWORD)
	//
	Assert( sizeof(rAddrBuf) >= nicAttr.NicAddressLen + 2 * sizeof (VIP_UINT16) + 2 * sizeof (DWORD));

	rAddr->HostAddressLen = nicAttr.NicAddressLen;
	while(1)
	{
		// Check if this is a valid name
		// ServerNic index is always hardcoded to 0, since giganet clients can only see
		// one Nic for a particular server
		if( (status = VipNSGetHostByName(pNic->m_pViaNic->GetNicHandle(), wszSvr, rAddr, 0)) != VIP_SUCCESS)
		{
			dwTimeElapsed = GetTickCount()-dwStart;

			// Not name or address - return error
			if( RETRY_TIMEOUT < dwTimeElapsed )
			{
				dwErr = MapVipErrorToSniError(status);
				SNI_SET_LAST_ERROR( VIA_PROV, SNIE_4, dwErr);
				goto ErrorExit;
			}			
		}

		else
		{
			rAddr->HostAddressLen = nicAttr.NicAddressLen;
			rAddr->DiscriminatorLen = DSCR_LEN;

			break;
		}
	}

    vi_attribs.Ptag = pNic->m_pViaNic->GetPTagHandle();

	// Create a VI. Dont associate the send queue with CQ in sync case. 
	if( VIP_SUCCESS != (status = VipCreateVi(pNic->m_pViaNic->GetNicHandle(), &vi_attribs, fSync?NULL:pNic->m_pViaWorker->m_hCQ, pNic->m_pViaWorker->m_hCQ, &m_hVi)) )
	{
		m_hVi = NULL;
		(void) StringCchCopyW(wszError, ARRAYSIZE(wszError), L"VipCreateVi");
		goto ErrorExit;
	}

	if(VIP_SUCCESS != (status = AllocDescs(pNic->m_pViaNic->GetNicHandle(), pNic->m_pViaNic->GetPTagHandle())) )
	{
		goto ErrorExit;
	}

	// Open a connection to the server
    	dwTick =  GetTickCount();
	dwThread = GetCurrentThreadId();
	dwCount = InterlockedIncrement(&gdwConnCount);

	//lAddr points to 
	//typedef struct _VIP_NET_ADDRESS {
	//    		VIP_UINT16 HostAddressLen;
	//			VIP_UINT16 DiscriminatorLen;
	//			VIP_UINT8 HostAddress[1];
	//			} VIP_NET_ADDRESS;
	//and we will set tailing data including NicAddress and one dwTick, dwThread, dwCount ( as unique connection identifier)	// 
	//Thus,  the size of the buff  should be longer than nicAttr.NicAddressLen + 2*sizeof(VIP_UITN16)+3*sizeof(DWORD)
	//

	Assert( sizeof(lAddrBuf) >= nicAttr.NicAddressLen + 2 * sizeof (VIP_UINT16) + 3 * sizeof (DWORD));
	lAddr->HostAddressLen = nicAttr.NicAddressLen;
	lAddr->DiscriminatorLen = 3*sizeof(DWORD);

	__analysis_assume( sizeof(lAddrBuf) >= nicAttr.NicAddressLen + 2 * sizeof (VIP_UINT16) + 3 * sizeof (DWORD) );   //We don't own Via NIC API, thus not able to annotate them. The Via Driver should make sure the Lengths returned in nicAttr is not too large(comply to the assertion three lines above).
	memcpy((char *)lAddr->HostAddress, nicAttr.LocalNicAddress, nicAttr.NicAddressLen);
	memcpy((char *)(lAddr->HostAddress + nicAttr.NicAddressLen), &dwTick, sizeof(DWORD));
	memcpy((char *)(lAddr->HostAddress + nicAttr.NicAddressLen + sizeof(DWORD)), &dwThread, sizeof(DWORD));
	memcpy((char *)(lAddr->HostAddress + nicAttr.NicAddressLen + 2*sizeof(DWORD)), &dwCount, sizeof(DWORD));

	// The remote address is in the form of Hash(wszClusterName) followed by PortNum
	// Note: For now, Hash is just an sqlloc_atoi of the string

	DWORD len = (DWORD) wcslen(wszClusterName);
OACR_WARNING_PUSH
OACR_WARNING_DISABLE(SYSTEM_LOCALE_MISUSE , " INTERNATIONALIZATION BASELINE AT KATMAI RTM. FUTURE ANALYSIS INTENDED. ")
	DWORD cch = LCMapStringW(LOCALE_SYSTEM_DEFAULT,
							LCMAP_UPPERCASE,
							wszClusterName,
							-1,
							wszClusterName,
							len+1);
OACR_WARNING_POP
	Assert(cch <= len+1);
	if( 0 >= cch || cch > len+1)
	{
		dwErr = ERROR_INVALID_PARAMETER;
		SNI_SET_LAST_ERROR(VIA_PROV, SNIE_ARGUMENT, dwErr);
		goto ErrorExit;
	}
	wszClusterName[len] = 0;

	dwHash = Hash(wszClusterName);
	memcpy((char *)(rAddr->HostAddress + nicAttr.NicAddressLen), &dwHash, sizeof(DWORD));


	while(1)
	{
ReDo:
		memcpy((char *)(rAddr->HostAddress + nicAttr.NicAddressLen + sizeof(DWORD)), &dwSvrPortNum, sizeof(DWORD));
		// Note: Ideally I'd like the timeout value to be VIP_INFINITE. But, the problem is when we try to connect
		// to an invalid entry cos' we got it from the cache (typically when a clustered server has failed over). In
		// that case, waiting with an infinite timeout just hangs - we need to timeout, redo an Ssrp handshake and retry
		// I'm giving it a timeout of 4 secs. for now - the server's typical accept rate is about 2 ms, so we should be
		// okay
		ULONG ulTimeOut = 10000;
		if( VIP_SUCCESS != (status = VipConnectRequest(m_hVi, lAddr, rAddr, ulTimeOut, &remote_attribs)) )
		{
			// Check if there are other nics specified in the connect string
			if( !fLastNic )
			{
				// Get the port
				WCHAR * tokContextPort = NULL; 
				pwszPort = wcstok_s(wszTmp, L",", &tokContextPort);
				if(!pwszPort)
				{
					fLastNic = TRUE;
					goto ErrorExit;
				}

				dwSvrPortNum = _wtoi_l(pwszPort, GetDefaultLocale());
				wszTmp += (int) wcslen(wszTmp) + 1;
				// Get the nic
				// Is this is the last nic/port combo
				if( NULL == wcsstr(wszTmp, L",") )
				{
					WCHAR * tokContextNic = NULL; 
					dwSvrNic = _wtoi_l(wcstok_s(wszTmp, L"\0", &tokContextNic), GetDefaultLocale());
					fLastNic = TRUE;
				} 

				else
				{
					WCHAR * tokContextNic = NULL; 
					dwSvrNic = _wtoi_l(wcstok_s(wszTmp, L",", &tokContextNic), GetDefaultLocale());
				}
	
				wszTmp += (int) wcslen(wszTmp) + 1;

				// Try again
				goto ReDo;
			}

			else
			{
				dwTimeElapsed = GetTickCount()-dwStart;

				// Not name or address - return error
				if( RETRY_TIMEOUT < dwTimeElapsed )
				{
					dwErr = MapVipErrorToSniError(status);
					SNI_SET_LAST_ERROR( VIA_PROV, SNIE_4, dwErr);
					goto ErrorExit;
				}
			}
		}

		else
		    	break;
	}

	// Fix up the szConnect string, so that we cache only the correct value
	// Do it only it its NOT an address
	if( !fAddr )
	{
		// Note: The size of szConnect is 256 - it is passed in from the SuperSock netlib
		//	if( 0 >  _snprintf(pProtElem->m_Params.Via.Host, 255, "%s:%s,%d,%d", wszSvr, wszClusterName, dwSvrPortNum, dwSvrNic) )
		//	goto ErrorExit;
		pProtElem->Via.Host[255] = L'\0';

	}

	m_pViaWorker = pNic->m_pViaWorker;
	m_pViaWorker->AddRef();

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
    return ERROR_SUCCESS;

ErrorExit:

	if( NULL != m_pbMem)
	{
		VipDeregisterMem(pNic->m_pViaNic->GetNicHandle(), m_pbMem, m_hMem);
		// If we failed to set m_pViaWorker, we won't be able to free m_pbMem in dctor; thus we
		// free the mem here.
		delete m_pbMem;
		m_pbMem = NULL;
	}

    DeleteCriticalSection(&CritSec);

    Assert(status != VIP_SUCCESS);
	
    dwError = MapVipErrorToSniError(status);

    BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);

    return dwError;
}

//timeout is in millisecs.
DWORD Via::ReadSync(__out SNI_Packet ** ppNewPacket, int timeout)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("ppNewPacket: %p{SNI_Packet**}, ")
							  _T("timeout: %d\n"), 
							  GetBidId(),
							  ppNewPacket, 
							  timeout);
	
	DWORD dwRet = ERROR_SUCCESS;
	SNI_Packet  * pPacket = 0;
	VIP_DESCRIPTOR * pDesc = 0;

	CAutoSNICritSec a_cs( CritSec, SNI_AUTOCS_DO_NOT_ENTER );

	pPacket = SNIPacketAllocate(m_pConn, SNI_Packet_Read);
	if(!pPacket)
	{
		dwRet = ERROR_OUTOFMEMORY;
		SNI_SET_LAST_ERROR(VIA_PROV, SNIE_OOM, dwRet);
		goto ErrorExit;
	}
	
	a_cs.Enter();

Loop:

	if( fClosed || fDisconnected)
	{
		dwRet = ERROR_CONNECTION_ABORTED;
		SNI_SET_LAST_ERROR( VIA_PROV, SNIE_2, dwRet );
		goto ErrorExit;
	}
	
	if( pReadDoneQ.IsEmpty() )
	{
		m_fReadPosted = TRUE;
	
		a_cs.Leave(); 

		dwRet = WaitForSingleObject(hSyncReadEvent, timeout);

		a_cs.Enter();

		if( dwRet != WAIT_OBJECT_0 )
		{
			m_fReadPosted = FALSE;
			
			if( dwRet != WAIT_TIMEOUT )
				dwRet = GetLastError();

			SNI_SET_LAST_ERROR( VIA_PROV, SNIE_10, dwRet );
			goto ErrorExit;
		}		

		// Check again - we mite have popped out of the wait due to a close
		// from the server
		if( fClosed || fDisconnected)
		{
			dwRet = ERROR_CONNECTION_ABORTED;
			SNI_SET_LAST_ERROR( VIA_PROV, SNIE_2, dwRet );
			goto ErrorExit;
		}
	}

	Assert(FALSE == m_fReadPosted);

	pDesc = (VIP_DESCRIPTOR *) pReadDoneQ.DeQueue();

	dwRet = CompleteRead(pDesc, pPacket, timeout);

	Assert(ERROR_IO_PENDING != dwRet );
	
	if( ERROR_SUCCESS != dwRet )
		goto ErrorExit;

	// If its a fragment, go back and continue to read the rest of the packet
	if( DescGetFragInfo(pDesc) )
		goto Loop;
	
	a_cs.Leave(); 
	
	*ppNewPacket = pPacket;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;

ErrorExit:

	if( pPacket )
		SNIPacketRelease( pPacket );

	a_cs.Leave(); 
	
	*ppNewPacket = NULL;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;

}

DWORD Via::WriteSync(SNI_Packet * pPacket, SNI_ProvInfo * pInfo)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("pInfo: %p{SNI_ProvInfo*}\n"), 
							  GetBidId(),
							  pPacket, 
							  pInfo);

	DWORD dwErr = ERROR_SUCCESS;
	
	CAutoSNICritSec a_csSyncWrite( m_SyncWriteCritSec, SNI_AUTOCS_DO_NOT_ENTER );

	a_csSyncWrite.Enter();

	CAutoSNICritSec a_cs( CritSec, SNI_AUTOCS_DO_NOT_ENTER );

	a_cs.Enter();

Loop:

	if( fClosed || fDisconnected)
	{
		dwErr = ERROR_CONNECTION_ABORTED;
		SNI_SET_LAST_ERROR( VIA_PROV, SNIE_2, dwErr );
		goto ErrorExit;
	}

	// Check send limit. If incremented SeqNum is less that DestSendLimit, 
	// send the packet
	if( SeqNum + 1 < DestSendLimit )
	{
		Assert(SendBufState == EMPTY);

		dwErr = SendPacket(pPacket);

		// Large packet - SendPacket returned pending, go back and wait
		if( ERROR_IO_PENDING == dwErr )
			goto Loop;
		
		if(ERROR_SUCCESS != dwErr )
			goto ErrorExit;
	}
	
	else
	{
		SendBufState = WAITING;

		a_cs.Leave(); 

		dwErr = WaitForSingleObject(hWriteEvent, INFINITE);

		a_cs.Enter();
		
		if( dwErr != WAIT_OBJECT_0 )
		{
			if( dwErr != WAIT_TIMEOUT )
				dwErr = GetLastError();

			SNI_SET_LAST_ERROR( VIA_PROV, SNIE_10, dwErr );
			goto ErrorExit;
		}
		
		goto Loop;
	}

	a_cs.Leave(); 

	a_csSyncWrite.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;

ErrorExit:

	a_cs.Leave(); 
	
	a_csSyncWrite.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwErr);
	
	return dwErr;
}                    

// Async functions
DWORD Via::ReadAsync(__out SNI_Packet ** ppNewPacket, LPVOID pPacketKey)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("ppNewPacket: %p{SNI_Packet**}, ")
							  _T("pPacketKey: %p\n"), 
							  GetBidId(),
							  ppNewPacket, 
							  pPacketKey);
	
	DWORD dwRet = ERROR_SUCCESS;
	SNI_Packet *pPacket = 0;	
	VIP_DESCRIPTOR * pDesc = 0;

	CAutoSNICritSec a_cs( CritSec, SNI_AUTOCS_DO_NOT_ENTER );

	*ppNewPacket = 0;
	
	pPacket = SNIPacketAllocate(m_pConn, SNI_Packet_Read);
	if(!pPacket)
	{
		dwRet = ERROR_OUTOFMEMORY;
		SNI_SET_LAST_ERROR(VIA_PROV, SNIE_OOM, dwRet);
		goto ErrorExit;
	}

	SNIPacketSetKey(pPacket, pPacketKey);
		
	a_cs.Enter();

Loop:
	//check the ReadDone Q - to see if its empty
	if( pReadDoneQ.IsEmpty() )
	{
		// Enqueue the packet, so that CompThread can pick this when there is data
		if( ERROR_SUCCESS != (dwRet = pReadKeyQ.EnQueue((HANDLE) pPacket)) )
			goto ErrorExit;

		m_dwPendingReads++;
		
		dwRet = ERROR_IO_PENDING;
	}

	// Packet arrived earlier
	else
	{
		m_dwQuickReads++;
		
		pDesc = (VIP_DESCRIPTOR *) pReadDoneQ.DeQueue();

		dwRet = CompleteRead(pDesc, pPacket, 0);

		Assert(ERROR_IO_PENDING != dwRet );
		
		if( ERROR_SUCCESS != dwRet )
			goto ErrorExit;

		// If its a fragment, go back and continue to read the rest of the packet
		if( DescGetFragInfo(pDesc) )
			goto Loop;

		else
			*ppNewPacket = pPacket;
	}
	
	a_cs.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;

ErrorExit:

	if( pPacket )
		SNIPacketRelease(pPacket);

	*ppNewPacket = 0;

	a_cs.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD Via::WriteAsync(SNI_Packet * pPacket, SNI_ProvInfo * pInfo)
{	
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("pInfo: %p{SNI_ProvInfo*}\n"), 
							  GetBidId(),
							  pPacket, 
							  pInfo);

	Assert( pPacket->m_OffSet == 0);
	DWORD dwRet = 0;

	CAutoSNICritSec a_cs( CritSec, SNI_AUTOCS_DO_NOT_ENTER );

	a_cs.Enter();
	
	if( fClosed )
	{
		dwRet = ERROR_CONNECTION_ABORTED;
		SNI_SET_LAST_ERROR( VIA_PROV, SNIE_2, dwRet );
		goto Exit;
	}

	// if the sequence number is less than the limit, then its oK to send
	if( WriteQ.IsEmpty() && (SeqNum + 1 < DestSendLimit) )
	{
		m_dwQuickWrites++;
		
		dwRet = SendPacket(pPacket);

		if( ERROR_IO_PENDING == dwRet )
		{
			WriteQ.EnQueue((HANDLE) pPacket);
		}
		
		else if( ERROR_SUCCESS == dwRet )
			dwRet = ERROR_IO_PENDING;
	}
	
	else
	{
		m_dwPendingWrites++;
		
		WriteQ.EnQueue(pPacket);
		//receiver is not ready to accept packets. we will buffer them until we receive OK from the other side. 
		//Look in SendAck and HandleReceiveEvent.
		dwRet = ERROR_IO_PENDING;
	}

Exit:
	
	a_cs.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD Via::GatherWriteAsync(SNI_Packet * pPacket, SNI_ProvInfo * pInfo)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("pInfo: %p{SNI_ProvInfo*}\n"), 
							  GetBidId(),
							  pPacket, 
							  pInfo);

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_FAIL);
	
	return ERROR_FAIL; //not implemented yet
}                    

DWORD Via::AcceptDone( SNI_Conn * pConn,
							__inout LPVOID pAcceptKey,
							DWORD dwBytes,
							DWORD dwRet,
						    __out SNI_Provider ** ppProv,
						    LPVOID * ppAcceptInfo )
{
	BidxScopeAutoSNI6( SNIAPI_TAG _T("pConn:%p{SNI_Conn*}, ")
							  _T("pAcceptKey:%p{ViaListenObject*}, ")
							  _T("dwBytes:%d, ")
							  _T("dwRet:%d, ")
							  _T("ppProv:%p{SNI_Provider**}, ")
							  _T("ppAcceptInfo:%p{LPVOID*}\n"), 
								pConn, pAcceptKey, dwBytes, dwRet, ppProv, ppAcceptInfo);

	ViaListenObject *pListenObject = (ViaListenObject *) pAcceptKey;

	Assert( pListenObject );

	VIP_VI_ATTRIBUTES vi_attribs ={ VIP_SERVICE_RELIABLE_DELIVERY, /* Reliability level */
	                                 32768,     /* MTU */
	                                 0,	    /* QOS is unused */
	                                 0,	    /* use the default protection tag */
	                                 FALSE, /* no RDMA Write */
	                                 FALSE  /* no RDMA Read */
                                   };

	VIP_RETURN status;

	Via * pVia = NULL;

	if( dwRet != ERROR_SUCCESS )
	{
		Assert( dwRet == ERROR_FAIL );
		goto ErrorExit0;
	}
	
	if(NULL == (pVia = NewNoX(gpmo)Via(pConn)))
	{
		dwRet = ERROR_OUTOFMEMORY;
		SNI_SET_LAST_ERROR(VIA_PROV, SNIE_4, dwRet);
		goto ErrorExit0;
	}

	if( ERROR_SUCCESS != pVia->FInit() )
	{
		goto ErrorExit0;
	}
	
	// Set the correct Ptag
	vi_attribs.Ptag = pListenObject->m_pViaNic->GetPTagHandle();
    
	pVia->m_hNwk = INVALID_HANDLE_VALUE;
	
	ViaWorker *pViaWorker = pListenObject->m_rgWorkers[SOS_Node::GetCurrent()->GetNodeId()];

	Assert( pViaWorker );

	status = VipCreateVi( pListenObject->m_pViaNic->GetNicHandle(), &vi_attribs, pViaWorker->m_hCQ, pViaWorker->m_hCQ, &pVia->m_hVi);

	if( status != VIP_SUCCESS )
	{
		dwRet = MapVipErrorToSniError(status);
		SNI_SET_LAST_ERROR(VIA_PROV, SNIE_4, dwRet);
		goto ErrorExit0;
	}

	status = pVia->AllocDescs(	pListenObject->m_pViaNic->GetNicHandle(), 
								pListenObject->m_pViaNic->GetPTagHandle());

	if(VIP_SUCCESS != status )
	{
		goto ErrorExit0;
	}
	
	//accept the connection
	if( VIP_SUCCESS != (status = VipConnectAccept(pListenObject->m_hConnHandle, pVia->m_hVi)) )
	{
		pVia->fClosed = TRUE;
		pVia->fConnClosed = TRUE;
		dwRet = ERROR_CONNECTION_UNAVAIL;
		SNI_SET_LAST_ERROR(VIA_PROV, SNIE_4, dwRet);
		goto ErrorExit;
	}

	pListenObject->m_hConnHandle = INVALID_HANDLE_VALUE;

	pVia->m_pViaWorker = pViaWorker;
	pVia->m_pViaWorker->AddRef();
	
	pListenObject->m_AcceptDoneEvent.Signal();

	pConn->m_pEPInfo->viaInfo.wszVendor[MAX_NAME_SIZE] = L'\0';
	pConn->m_pEPInfo->viaInfo.szVendorDll[MAX_NAME_SIZE] = '\0';
	pConn->m_pEPInfo->viaInfo.szDiscriminator[MAX_NAME_SIZE] = '\0';
		
	if( (FAILED(StringCchPrintf_lW( pConn->m_pEPInfo->viaInfo.wszVendor,
					 ARRAYSIZE(pConn->m_pEPInfo->viaInfo.wszVendor),
					 L"%s", GetDefaultLocale(),
					 pListenObject->m_ViaListenInfo.wszVendor))) ||
		(FAILED(StringCchPrintf_lA( pConn->m_pEPInfo->viaInfo.szVendorDll,
					 CCH_ANSI_STRING(pConn->m_pEPInfo->viaInfo.szVendorDll),
					 "%s", GetDefaultLocale(),
					 pListenObject->m_ViaListenInfo.szVendorDll))) ||
		(FAILED(StringCchPrintf_lA( pConn->m_pEPInfo->viaInfo.szDiscriminator,
					 CCH_ANSI_STRING(pConn->m_pEPInfo->viaInfo.szDiscriminator),
					 "%s", GetDefaultLocale(),
					 pListenObject->m_ViaListenInfo.szDiscriminator))) )
	{
		dwRet = ERROR_INVALID_PARAMETER;
		SNI_SET_LAST_ERROR( VIA_PROV, SNIE_5, dwRet );
		goto ErrorExit;
	}

	pConn->m_pEPInfo->viaInfo.NicId = pListenObject->m_ViaListenInfo.NicId;
	
    *ppProv = pVia;

    // Take a ref on the object
	pVia->m_pConn->AddRef( REF_InternalActive );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

	return ERROR_SUCCESS;

ErrorExit0:

	//SQL BU 308228.
	//ConnectReject should not be called after ConnectAccept fails. It is possible that the m_hConnHandle
	//becomes corrupted if ConnectAccept fails.
	//Note: Don't know how to handle error if VipConnectReject fails, since we need the original error code to be 
	//reported to the caller. So just do best effort here. We expect the other end can timeout connection if 
	//this fails.
	VipConnectReject( pListenObject->m_hConnHandle);

ErrorExit:
	
	if( pVia && NULL != pVia->m_pbMem )
	{
		VipDeregisterMem(pListenObject->m_pViaNic->GetNicHandle(), pVia->m_pbMem, pVia->m_hMem);
		// If we failed to set m_pViaWorker, we won't be able to free m_pbMem in dctor; thus we
		// free the mem here.
		delete pVia->m_pbMem;
		pVia->m_pbMem = NULL;
	}

	if( pVia)
		delete pVia;
	pListenObject->m_hConnHandle = INVALID_HANDLE_VALUE;
	pListenObject->m_AcceptDoneEvent.Signal();
	
	*ppProv = 0;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD Via::ReadDone(SNI_Packet ** ppPacket, SNI_Packet **ppLeftOver, DWORD dwBytes, DWORD dwRet)
{
	BidxScopeAutoSNI5( SNIAPI_TAG _T("%u#, ")
							  _T("ppPacket: %p{SNI_Packet**}, ")
							  _T("ppLeftOver: %p{SNI_Packet**}, ")
							  _T("dwBytes: %d, ")
							  _T("dwRet: %d{WINERR}\n"), 
							  GetBidId(),
							  ppPacket, 
							  ppLeftOver, 
							  dwBytes, 
							  dwRet);

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD Via::WriteDone(SNI_Packet ** ppPacket, DWORD dwBytes, DWORD dwRet)
{
	BidxScopeAutoSNI4( SNIAPI_TAG _T("%u#, ")
							  _T("ppPacket: %p{SNI_Packet**}, ")
							  _T("dwBytes: %d, ")
							  _T("dwRet: %d{WINERR}\n"), 
							  GetBidId(),
							  ppPacket, 
							  dwBytes, 
							  dwRet);

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD Via::Close()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );
	
	DWORD dwRet = ERROR_SUCCESS;
	VIP_RETURN status;

	CAutoSNICritSec a_cs( CritSec, SNI_AUTOCS_DO_NOT_ENTER );

	a_cs.Enter();

	// Set fConnClosed to TRUE
	fConnClosed = TRUE;

	if( (FALSE == fDisconnected) && (status = VipDisconnect(m_hVi)) != VIP_SUCCESS )
	{
		dwRet = MapVipErrorToSniError(status);
		SNI_SET_LAST_ERROR(VIA_PROV, SNIE_4, dwRet);
		goto ErrorExit;
	}

	fDisconnected = TRUE;

	// Set event to release anyone stuck in read/write
	// Note: we are doing this multiple times, but that should be okay
	SetEvent(hWriteEvent);
	SetEvent(hSyncReadEvent);

ErrorExit:

	a_cs.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

void Via::Release()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T( "\n") );
	
	delete this;
}



