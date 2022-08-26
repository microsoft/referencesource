//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: sni_prov.cpp
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

#include "snipch.hpp"
#ifndef SNI_BASED_CLIENT
#include "httpprov.h"
#endif
#include "np.hpp"
#include "tcp.hpp"
#include "sm.hpp"
#include "smux.hpp"
#include "ssl.hpp"
#include "via.hpp"

SNI_PROVIDER_INFO rgProvInfo[MAX_PROVS];

//	If rgProviders is NULL, initialize all supported providers 
//	(and ignore cProviders), otherwise initialize the providers
//	specified in the rgProviders array.  
//
void SNI_Provider::InitProviders(__in_ecount_opt(cProviders) const ProviderNum * rgProviders, __in DWORD cProviders)
{
	SNI_PROVIDER_INFO * pProvInfo;
	DWORD cActualProviders;  

	//	This will, among others, set the fInitialized flag
	//	to FALSE for every provider.  
	//
	memset(rgProvInfo, 0, sizeof(rgProvInfo)); 

	if( NULL == rgProviders )
	{
		cActualProviders = MAX_PROVS; 
	}
	else
	{
		cActualProviders = cProviders; 
	}

	for(DWORD i = 0; i < cActualProviders; i++)
	{
		DWORD iCurrentProvider; 

		if( NULL == rgProviders )
		{
			iCurrentProvider = i; 
		}
		else
		{
			iCurrentProvider = rgProviders[i]; 
		}

		pProvInfo = &rgProvInfo[iCurrentProvider];

		switch(iCurrentProvider)
		{
			case INVALID_PROV: break; //do nothing
#ifdef USE_HTTP_PROVIDER			
			case HTTP_PROV	: Http::Initialize(pProvInfo);	break;
#else
			case HTTP_PROV	: break; //do nothing
#endif			
			case SM_PROV	: Sm  ::Initialize(pProvInfo);  // Fall-through to Np
			case NP_PROV	: Np  ::Initialize(pProvInfo);	break;
			case SESSION_PROV: break;	//do nothing
			case SIGN_PROV	: Sign::Initialize(pProvInfo);	break;
			case SMUX_PROV	: Smux::Initialize(pProvInfo);	break;
			case SSL_PROV	: Ssl ::Initialize(pProvInfo);	break;
			case TCP_PROV	: Tcp ::Initialize(pProvInfo);	break;
			case VIA_PROV	: Via ::Initialize(pProvInfo);	break;
			default			: 
				//This assertion is used to catch unexpected coding errors.
				Assert( 0 && "ProvNum is unknown\n" );
				BidTrace0( ERROR_TAG _T("ProvNum is unknown\n") );
				break; 
		}
	}
}

void SNI_Provider::Terminate()
{
	//	Each provider will take actual termination action only
	//	if it was initialized.  
	//
	Tcp::Terminate();
#ifdef USE_HTTP_PROVIDER	
	Http::Terminate();
#endif
	Sm::Terminate();
	Np::Terminate();
	Via::Terminate();
	Sign::Terminate();
	Ssl::Terminate();
}

DWORD SNI_Provider::ReadSync(SNI_Packet ** ppNewPacket, int timeout)
{
	//This assertion is used to claim this function is not implemented.
	Assert( 0 && " This function is not implemented\n" );
	BidTrace0( ERROR_TAG _T("This function is not implemented\n") );

	SNI_SET_LAST_ERROR( m_Prot, SNIE_15, ERROR_FAIL );

	return ERROR_FAIL;
}

DWORD SNI_Provider::CheckConnection( )
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId());

	DWORD dwRet = ERROR_FAIL;
	if( NULL != m_pNext )
	{
		// Default implementation for an intermediate provider: assume we don't
		// need any funny synchronization (so, SMUX has to reimplement) or
		// have any special connection closure semantics (another reason SMUX
		// has to reimplement), and just call into the next provider in the chain.
		dwRet = m_pNext->CheckConnection();
	}
	else
	{
		// Default implementation for a base provider: assume it is successful. Currently
		// unused in this branch, but is used by VIA in SqlClient branch
		dwRet = ERROR_SUCCESS;
	}
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;
}

DWORD SNI_Provider::PartialReadSync(SNI_Packet * pPacket, DWORD cbBytesToRead, int timeout)
{
	BidxScopeAutoSNI4( SNIAPI_TAG _T("%u#, ")
							  _T( "pPacket: %p{SNI_Packet*}, ")
							  _T("cbBytesToRead: %d, timeout: %d\n"), 
							  GetBidId(),
							  pPacket, 
							  cbBytesToRead, 
							  timeout);

	// The peer's consumer (TDS) shall pass one whole packet into one SNI call.  
	// If the provider is message-oriented our side's consumer shall never 
	// need to call a partial read function.  
	// A bad client can send such data though, so we error out.  
	//
	SNI_ASSERT_ON_INVALID_PACKET; 

	SNI_SET_LAST_ERROR( m_Prot, SNIE_15, ERROR_NOT_SUPPORTED );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_NOT_SUPPORTED);
	
	return ERROR_NOT_SUPPORTED;
}

DWORD SNI_Provider::PartialReadAsync(SNI_Packet * pPacket, DWORD cbBytesToRead, SNI_ProvInfo * pProvInfo )
{
	BidxScopeAutoSNI4( SNIAPI_TAG _T("%u#, ")
							  _T( "pPacket: %p{SNI_Packet*}, ")
							  _T("cbBytesToRead: %d, ")
							  _T("pInfo: %p{SNI_ProvInfo*}\n"), 
							  GetBidId(),
							  pPacket, 
							  cbBytesToRead, 
							  pProvInfo);

	// The peer's consumer (TDS) shall pass one whole packet into one SNI call.  
	// If the provider is message-oriented our side's consumer shall never 
	// need to call a partial read function.  
	// A bad client can send such data though, so we error out.  
	//
	SNI_ASSERT_ON_INVALID_PACKET; 
	
	SNI_SET_LAST_ERROR( m_Prot, SNIE_15, ERROR_NOT_SUPPORTED );
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_NOT_SUPPORTED);
	
	return ERROR_NOT_SUPPORTED;
}

DWORD SNI_Provider::WriteSync(SNI_Packet * pPacket, SNI_ProvInfo * pProvInfo)
{
	//This assertion is used to claim this function is not implemented
	Assert( 0 && " This function is not implemented\n" );
	BidTrace0( ERROR_TAG _T("This function is not implemented\n") );

	SNI_SET_LAST_ERROR( m_Prot, SNIE_15, ERROR_FAIL );

	return ERROR_FAIL;
}

DWORD SNI_Provider::GatherWriteAsync(SNI_Packet * pPacket, SNI_ProvInfo * pProvInfo)
{
	//This assertion is used to claim this function is not implemented
	Assert( 0 && " This function is not implemented\n" );
	BidTrace0( ERROR_TAG _T("This function is not implemented\n") );

	SNI_SET_LAST_ERROR( m_Prot, SNIE_15, ERROR_FAIL );

	return ERROR_FAIL;
}

DWORD SNI_Provider::QueryImpersonation()
{
	if( NULL != m_pNext )
	{
		return m_pNext->QueryImpersonation();
	}
	else
	{
		return ERROR_NOT_SUPPORTED;
	}
}

DWORD SNI_Provider::ImpersonateChannel()
{
	if( NULL != m_pNext )
	{
		return m_pNext->ImpersonateChannel();
	}
	else
	{
		return ERROR_NOT_SUPPORTED;
	}
}

DWORD SNI_Provider::RevertImpersonation()
{
	if( NULL != m_pNext )
	{
		return m_pNext->RevertImpersonation();
	}
	else
	{
		return ERROR_NOT_SUPPORTED;
	}
}

