//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: sni_sspi.hpp
// @Owner: nantu, petergv
// @Test: milu
//
// <owner current="true" primary="true">nantu</owner>
// <owner current="true" primary="false">petergv</owner>
//
// Purpose:
//
// Notes:
//          
// @EndHeader@
//****************************************************************************

#ifndef _INTSEC_H_
#define _INTSEC_H_

#define SECURITY_WIN32

#include "sni.hpp"
#include <sspi.h>

#ifndef SNI_BASED_CLIENT
#define TCHAR WCHAR
#define LPTSTR LPWSTR
#define LPCTSTR LPCWSTR
#endif

#define MAX_PKG_NAME 256

class SNI_Sec
{
	private:
		CtxtHandle 	m_hCtxt;
		BOOL		m_fCtxt;
		SecPkg		m_PkgId;
		CredHandle	m_hCred;
		SecPkgContext_NegotiationInfo* m_pPkgInfo;
		DWORD		m_dwLastError;  //Cache the error from SetPkgName so that GetSecPkgName can use  it.

		SNI_Sec();

		~SNI_Sec();

		void SetPkgName(LPTSTR szPkgName)
		{
			Assert ( NULL != szPkgName );
			
			if(!_tcsnicmp_l((_TCHAR *) _T("NTLM"), szPkgName, 5 , GetDefaultLocale()) )  // Use 5 to allow null-terminator 
				m_PkgId = NTLM;
			else if(!_tcsnicmp_l((_TCHAR *) _T("Kerberos"), szPkgName, 9 , GetDefaultLocale())) // Use 9 to allow null-terminator.
				m_PkgId = KERBEROS;
			else
				//	BASIC and NEGOTIATE packages are only reported by 
				//	Http auth provider and are set 
				//	through SetSecPkgId(), so we don't provide a mapping
				//	from string. The SNI Auth provider always resolves the
				//	NEGOTIATE package to the underlying NTLM/KERBEROS
				//	auth scheme.
				//
				m_PkgId = INVALID;		
		}

		DWORD SetPkgName();

	public:

		friend class SNI_Conn;

		void DeleteSspi();

		SecPkg GetSecPkgId(){ return m_PkgId; }

		void SetSecPkgId(SecPkg secpkg){ m_PkgId = secpkg; }

		DWORD GetSecPkgName(LPTSTR  * ppPkgName);

		DWORD GetMutualAuth(BOOL* fMutualAuth);

		DWORD CheckServiceBindings(SNIAuthErrStates *pSSPIFailureState);
		
		friend DWORD SNISecInitPackageEx(__out DWORD * pcbMaxToken, BOOL fInitializeSPN, BOOL fInitializeServerCredential, ExtendedProtectionPolicy eExtendedProtectionLevel, __in_ecount_opt(dwcAcceptedSPNs) WCHAR **pwszAcceptedSPNs, DWORD dwcAcceptedSPNs);

		friend DWORD SNISecTerminatePackage();

		friend DWORD SNISecGenServerContext( SNI_Conn * pConn, 
								BYTE	 * pIn,
								DWORD	   cbIn,
								BYTE	 * pOut,
								DWORD	  * pcbOut,
								BOOL	   * pfDone,
								SNIAuthErrStates *pSSPIFailureState,
								BOOL fCheckChannelBindings);

		friend DWORD SNISecGenClientContext ( SNI_Conn * pConn,
										BYTE	*pIn,
										DWORD	cbIn,
										BYTE	*pOut,
										DWORD	*pcbOut,
										BOOL	*pfDone,
										__in __nullterminated const WCHAR	*wszServerInfo,
										DWORD	 cbServerInfo,
										LPCWSTR pwszUserName,
										LPCWSTR pwszPassword);

		friend DWORD SNISecImpersonateContext(SNI_Conn * pConn);

		friend DWORD  SNISecRevertContext(SNI_Conn * pConn);

		friend DWORD SNISecGetSecHandle(__in SNI_Conn * pConn, __out HANDLE * phSec, __out SecPkg * pPkgId);

		friend DWORD SNISecRegisterSpn(DWORD * rgPorts, DWORD cPorts);
		
		friend DWORD SNISecUnregisterSpn();
};
#endif
