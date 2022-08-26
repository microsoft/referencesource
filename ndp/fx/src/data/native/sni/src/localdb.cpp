//****************************************************************************
//				Copyright (c) Microsoft Corporation.
//
// @File: localDB.cpp
// @Owner: see SQLDevDash
//
// <owner current="true" primary="true">see SQLDevDash</owner>
//
// Purpose: SNI LocalDB Provider
//
// Notes:
//			
// @EndHeader@
//****************************************************************************
	
#include "snipch.hpp"
#include "LocalDB.hpp"
#include "reg.hpp"
#include "nlregc.h"

LocalDB* LocalDB::s_pInstance = NULL;

SNICritSec* g_csLocalDBInitialize = NULL;

	inline DWORD MapLocalDBErrorStateToCode(LocalDBErrorState errorState)
	{
		switch(errorState)
		{
			case NO_INSTALLATION:	return SNIE_52;
	
			case INVALID_CONFIG:	return SNIE_53;
	
			case NO_SQLUSERINSTANCEDLL_PATH:	return SNIE_54;
			
			case INVALID_SQLUSERINSTANCEDLL_PATH:	return SNIE_55;
	
			default: return SNIE_53;
		}
	}

	// Thread safe implementation to return a singleton instance
	//
	DWORD LocalDB::LDBInstance(__out_opt LocalDB** ppLdbInstance)
	{
		BidxScopeAutoSNI1( SNIAPI_TAG _T("ppLdbInstance: %p{LocalDB**} \n"), ppLdbInstance );

		DWORD dwRet = ERROR_SUCCESS;
		LocalDB* pLdbInstance;

		// check if instance already exisits.
		//
		if(NULL != s_pInstance)
		{
			goto ErrorExit;
		}
		
		{
			CAutoSNICritSec a_cs( g_csLocalDBInitialize, SNI_AUTOCS_ENTER );
			
			// check if instance already exisits.
			//
			if(NULL != s_pInstance)
			{
				goto ErrorExit;
			}

			// Create a local object
			//
			if(NULL == (pLdbInstance = NewNoX(gpmo) LocalDB))
			{
				dwRet = ERROR_OUTOFMEMORY;
				SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, dwRet);
				BidTrace1( ERROR_TAG _T("LocalDB: Unable to retrieve singleton instance.%d{WINERR}\n"), dwRet);
				goto ErrorExit;
			}
					
			// Initialize
			//
			dwRet = pLdbInstance->FInitialize();

			if(ERROR_SUCCESS != dwRet)
			{
				// cleanup so that a new instance can be created.
				//
				delete pLdbInstance;
				pLdbInstance = NULL;
				
				SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, dwRet);
				BidTrace1( ERROR_TAG _T("LocalDB: Initialization failed.%d{WINERR}\n"), dwRet);

				// release lock
				//
				goto ErrorExit;
			}

			// Now assign the actual value
			//
			InterlockedExchangePointer(reinterpret_cast<volatile PVOID*>(&s_pInstance),
										reinterpret_cast<PVOID>(pLdbInstance));
		}// end of critical section scope

ErrorExit:

		if(ERROR_SUCCESS == dwRet)
		{
			*ppLdbInstance = s_pInstance;
		}
		else
			*ppLdbInstance = NULL;
		
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%p{LocalDB**}\n"), ppLdbInstance);
		return dwRet;
	}

	DWORD LocalDB::FInitialize()
	{
		BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );

		DWORD dwRet = ERROR_SUCCESS;

		// Initialize critical section
		//
		dwRet = SNICritSec::Initialize(&m_CS);

		if(dwRet != ERROR_SUCCESS)
		{
			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, dwRet);
			BidTrace1( ERROR_TAG _T("LocalDB: Critical section Initialization failed.%d{WINERR}\n"), dwRet);
		}
		
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
		return dwRet;
	}
		
	DWORD LocalDB::getLocalDBErrorMessage( __in DWORD dwLocalDBError,
												__out_ecount(pdwCurMsgLen) LPWSTR wszErrorMessage,
												__inout LPDWORD pcchErrMsgBuf) 
	{
		BidxScopeAutoSNI3( SNIAPI_TAG _T("dwLocalDBError: %d{DWORD},")
									_T("wszErrorMessage: \"%ls{LPWSTR}\", ")
									_T("pcchErrMsgBuf: %p{LPDWORD}\n"),
									dwLocalDBError,
									wszErrorMessage,
									pcchErrMsgBuf);

		DWORD dwRet = ERROR_SUCCESS;
		LocalDB* pLdbBInstance = NULL;

		//Obtain the singleton Instance
		//
		if(ERROR_SUCCESS != (dwRet = LocalDB::LDBInstance(&pLdbBInstance)))
		{
			BidTrace1( ERROR_TAG _T("LocalDB: Retreiving LocalDB instance failed.%d{WINERR}\n"), dwRet);
			goto ErrorExit;
		}

		// This should never fail, since the dll should be loaded prior to this call.
		//
		Assert(pLdbBInstance->m_pfnLocalDBFormatMessage);

		if (pLdbBInstance->m_pfnLocalDBFormatMessage != NULL)
		{
			dwRet = pLdbBInstance->m_pfnLocalDBFormatMessage( dwLocalDBError, 
														LOCALDB_TRUNCATE_ERR_MESSAGE,
														0,
														wszErrorMessage,
														pcchErrMsgBuf);			
			if(FAILED(dwRet))
			{
				SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_SYSTEM, dwRet );
				BidTrace1( ERROR_TAG _T("LocalDB: getLocalDBFormatMessage failed.%d{WINERR}\n"), dwRet);
			}
		}
		else
		{
			dwRet = GetLastError();
		}

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);

ErrorExit:

		return dwRet;
	}

	DWORD LocalDB::loadUserInstanceDll()
	{
		BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );

		DWORD dwRet = ERROR_SUCCESS;
		LocalDBErrorState errorState;

		HMODULE hUserInstDll = NULL;

		char szDllPath[MAX_PATH];

		DWORD cchDllPathSize = MAX_PATH;

		// Check if the dll is already loaded.
		// Although this is not checked in a thread-safe manner,
		// it helps gain performance
		//
		if(NULL != m_hUserInstanceDll)
		{
			// The dll has already been loaded.
			//
			goto ErrorExit;
		}	

		// Acquiring critical Section to make it thread-safe
		//
		{
		CAutoSNICritSec a_cs( m_CS, SNI_AUTOCS_ENTER );

		// Check if the dll is already loaded
		//
		if(NULL != m_hUserInstanceDll)
		{
			// The dll has already been loaded.
			//
			goto ErrorExit;
		}
		
		// Browse Registry to obtain path to the
		// latest sqlUserInstance.dll
		//
		dwRet = GetUserInstanceDllPath(szDllPath, cchDllPathSize,(DWORD*) &errorState);
		if(ERROR_SUCCESS != dwRet)
		{
			SNI_SET_LAST_ERROR(INVALID_PROV, MapLocalDBErrorStateToCode(errorState), dwRet);
			BidTrace1(ERROR_TAG _T("LocalDB: InstanceAPIPath Reg key not found. %d{WINERR}\n"), dwRet );
			goto ErrorExit;
		}

		// Trim whitespaces and tabs
		//
		dwRet = StrTrimBoth_Sys(szDllPath, strlen(szDllPath)+1, NULL, " \t", 3);
		if(ERROR_SUCCESS != dwRet)
		{
			SNI_SET_LAST_ERROR(INVALID_PROV, SNIE_55, dwRet);
			BidTrace1(ERROR_TAG _T("LocalDB: Invalid InstanceAPIPath Reg key path. %d{WINERR}\n"), dwRet );
			goto ErrorExit;
		}
		
		// Check if the registry path is blank
		//
		if(!strlen(szDllPath))
		{
			dwRet = ERROR_INVALID_DATA;
			SNI_SET_LAST_ERROR(INVALID_PROV, SNIE_55, dwRet);
			BidTrace1(ERROR_TAG _T("LocalDB: sqlUserInstance.dll path is blank. %d{WINERR}\n"), dwRet );
			goto ErrorExit;
		}

		// Load sqlUserInstance.dll
		//
		hUserInstDll = LoadLibraryA( szDllPath);
		if( NULL == hUserInstDll)
		{
			dwRet = GetLastError();
			SNI_SET_LAST_ERROR(INVALID_PROV, SNIE_56, dwRet);
			BidTrace1(ERROR_TAG _T("LocalDB: Failed to load sqlUserInstance.dll. %d{WINERR}\n"), dwRet );
			goto ErrorExit;
		}		

		// Check if the dll is corrupt.
		//
		m_pfnLocalDBStartInstance = (FnLocalDBStartInstance*)GetProcAddress(hUserInstDll, "LocalDBStartInstance");

		m_pfnLocalDBFormatMessage = (FnLocalDBFormatMessage*)GetProcAddress(hUserInstDll, "LocalDBFormatMessage");

		if(NULL == m_pfnLocalDBStartInstance || NULL == m_pfnLocalDBFormatMessage)
		{
			// corrupt dll
			//
			dwRet = GetLastError();
			SNI_SET_LAST_ERROR(INVALID_PROV, SNIE_57, dwRet);
			BidTrace1(ERROR_TAG _T("LocalDB: Invalid/Corrupt sqlUserInstance.dll. %d{WINERR}\n"), dwRet );

			goto ErrorExit;
		}
			
		//	Now try to set the handle for sqlUserInstance.dll
		//
		if(NULL != InterlockedExchangePointer(reinterpret_cast<volatile PVOID*>(&m_hUserInstanceDll),
											reinterpret_cast<PVOID>(hUserInstDll)))
		{
			goto ErrorExit;
		}
		}// end of critical section scope
		
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("LocalDB: Loaded sqlUserInstance.dll. %d{WINERR}\n"), dwRet );	
		return dwRet;
		
ErrorExit:
	
		//cleanup
		//
		if(NULL != hUserInstDll)
		{
			FreeLibrary(hUserInstDll);
			hUserInstDll = NULL;
		}
		if(ERROR_SUCCESS != dwRet)
		{
			m_pfnLocalDBStartInstance = NULL;
	    	m_pfnLocalDBFormatMessage = NULL;
		}		    
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet ); 
		return dwRet;	
	}

	DWORD LocalDB::getLocalDBConnectionString(__in LPCWSTR wszInstanceName, 
													__out_ecount(pcchLocalDBConnectBuf) LPWSTR wszlocalDBConnect,
													__inout LPDWORD pcchLocalDBConnectBuf)
	{
		BidxScopeAutoSNI3( SNIAPI_TAG _T("wszInstanceName: \"%ls{WCHAR*}\", ")
									_T("wszlocalDBConnect: \"%ls{LPWSTR}\"\n")
									_T("pcchLocalDBConnectBuf: %p{LPDWORD}\n"),
									wszInstanceName,
									wszlocalDBConnect,
									pcchLocalDBConnectBuf);
		
		DWORD dwRet = ERROR_SUCCESS;

		// load latest sqlUserinstance.dll
		//
		dwRet = loadUserInstanceDll();
		if(ERROR_SUCCESS != dwRet)
		{
			BidTrace1(ERROR_TAG _T("LocalDB: loadUserInstanceDll failed. %d{WINERR}\n"), dwRet);
			goto ErrorExit;
		}

		// Get the named pipe for the instance
		//
		HRESULT hr = m_pfnLocalDBStartInstance(wszInstanceName, 0, wszlocalDBConnect, pcchLocalDBConnectBuf);
			
		if (FAILED(hr))
		{
			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_LocalDB, hr );
			BidTrace1(ERROR_TAG _T("LocalDB: LocalDBStartInstance failed. %d{WINERR}\n"), hr);
			return hr;
		}		

ErrorExit:		

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet ); 
		return dwRet;
	}


void LocalDB::Terminate()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );

	LocalDB* pLocalDB = (LocalDB*) InterlockedExchangePointer((void**)&LocalDB::s_pInstance, NULL);

	if (NULL != pLocalDB)
	{
		// cleanup
		//
		if(NULL != pLocalDB->m_hUserInstanceDll)
		{
			FreeLibrary(pLocalDB->m_hUserInstanceDll);
			pLocalDB->m_hUserInstanceDll = NULL;
		}
		
		if(NULL != pLocalDB->m_CS)
		{
			DeleteCriticalSection( &pLocalDB->m_CS );
			pLocalDB->m_CS = NULL;
		}
		
		pLocalDB->m_pfnLocalDBStartInstance = NULL;
		pLocalDB->m_pfnLocalDBFormatMessage = NULL;

		delete pLocalDB;
	}

	BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("\n"));
}
