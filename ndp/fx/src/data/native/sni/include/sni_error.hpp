//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: sni_error.hpp
// @Owner: petergv, nantu
// @Test: milu
//
// <owner current="true" primary="true">petergv</owner>
// <owner current="true" primary="false">nantu</owner>
//
// Purpose: SNI Error Support Routines.
//
// Notes:
//          
// @EndHeader@
//****************************************************************************

#ifndef _SNI_ERROR_
#define _SNI_ERROR_

#ifdef SNI_BASED_CLIENT
	#include "sqlhdr.h"
#endif

#include "sni.hpp"

//----------------------------------------------------------------------------
// Name: 	SNI_<error>
//
// Purpose:	This is a temporary area that will contain the defines
//          of all error messages returned to the consumer.
//
//----------------------------------------------------------------------------

#include "sni_rc.h"

#define SNIE_IOERROR SNIE_1
#define SNIE_CONNRESET SNIE_2
#define SNIE_NOASYNC SNIE_3
#define SNIE_OOM SNIE_4
#define SNIE_ARGUMENT SNIE_5
#define SNIE_PROTOCOL SNIE_6
#define SNIE_OS SNIE_10
#define SNIE_TIMEOUT SNIE_11
#define SNIE_CONNCLOSED SNIE_20
#define SNIE_NODLL SNIE_22




#define WIDEN2(x)		L ## x
#define WIDEN(x)		WIDEN2(x)
#define __WFILE__		WIDEN(__FILE__)
#define __WFUNCTION__	WIDEN(__FUNCTION__)


//	On the server-side, the TLS indices for the last SNI error 
//	are hard-coded, so there is no need to initialize/terminate them.  
//
#ifdef SNI_BASED_CLIENT

DWORD SNIErrorTlsInit(); 
void SNIErrorTlsTerm(); 

#endif	//	#ifdef SNI_BASED_CLIENT

void SNISetLastError( ProviderNum Provider,
					  DWORD       dwNativeError,
					  DWORD		  dwSNIError,
					  WCHAR     * pszFileName,
					  WCHAR     * pszFunction,
					  DWORD       dwLineNumber );
					  


#ifdef DEBUG
#define SNI_SET_LAST_ERROR( Provider, dwSNIError, dwNativeError ){	\
					   DWORD __dwNativeErrCode = dwNativeError;			\
					   BidTrace3( ERROR_TAG _T("ProviderNum: %d{ProviderNum}, SNIError: %d{SNIError}, NativeError: %d{WINERR}\n"), \
					   Provider, SNI_ERROR_ID_FROM_STRING_ID(dwSNIError), __dwNativeErrCode ); \
					   SNISetLastError( Provider,						\
										__dwNativeErrCode,				\
										dwSNIError,						\
										__WFILE__,						\
										__WFUNCTION__,					\
										__LINE__ );}								
#else
#define SNI_SET_LAST_ERROR( Provider, dwSNIError, dwNativeError ){	\
					   DWORD __dwNativeErrCode = dwNativeError;			\
					   BidTrace3( ERROR_TAG _T("ProviderNum: %d{ProviderNum}, SNIError: %d{SNIError}, NativeError: %d{WINERR}\n"), \
					   Provider, SNI_ERROR_ID_FROM_STRING_ID(dwSNIError), __dwNativeErrCode ); \
					   SNISetLastError( Provider,						\
										__dwNativeErrCode,				\
										dwSNIError,						\
										NULL,							\
										NULL,							\
										0 );}
#endif

#ifdef SNI_BASED_CLIENT
inline void scierrlog(__in DWORD dwErr,...)
{
}
#else
void cdecl scierrlog(__in DWORD dwErr,...);
#endif

#endif	// #ifndef _SNI_ERROR_
