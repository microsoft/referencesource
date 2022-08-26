//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: Np.cpp
// @Owner: petergv, nantu
// @Test: milu
//
// <owner current="true" primary="true">petergv</owner>
// <owner current="true" primary="false">nantu</owner>
//
// Purpose: SNI Named-Pipe Provider
//
// Notes:
//          
// @EndHeader@
//****************************************************************************

#include "snipch.hpp"
#include "Np.hpp"
#include "sm.hpp"
#include <aclapi.h>
#define NP_OPEN_TIMEOUT 5000

DWORD const CLOSEIOWAIT = 1000;	//	milliseconds 

// Global Variables
extern BOOL			gfIsWin9x;

// DeleteNpAcl
//
// This function delete the Security Descriptor created by CreateNpAcl()
//
//	Inputs: None
//  
//  Returns: None
//
void DeleteNpAcl(__inout NpSecurityInfo * &pSecInfo)
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T( "\n") );

	delete pSecInfo; 
	pSecInfo = NULL;
}

// CreateNpAcl
//
// This function creates a Security Descriptor that will allow this process
// to create new instances of the named-pipe, while only allowing remote
// clients to read and write to the pipe.
//
//	Inputs: None
//  
//  Returns: ERROR_SUCCESS if successful, otherwise the error code is returned
//
DWORD CreateNpAcl(__out NpSecurityInfo ** ppSecInfo, ProviderNum prot)
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T( "\n") );
	
	ULONG AclSize;
	SID_IDENTIFIER_AUTHORITY WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;
	SID_IDENTIFIER_AUTHORITY LocalAuthority = SECURITY_LOCAL_SID_AUTHORITY;
	SID_IDENTIFIER_AUTHORITY LocalAuthority2 = SECURITY_NT_AUTHORITY;
	SID * psidCurrentUser = NULL;
	DWORD dwRet = ERROR_SUCCESS;
	NpSecurityInfo * pSecInfo = NULL;

	*ppSecInfo = NULL;

	// Create a new one.
	//
	if( !(pSecInfo = NewNoX(gpmo) NpSecurityInfo) )
	{
		dwRet = ERROR_OUTOFMEMORY;
		SNI_SET_LAST_ERROR( prot, SNIE_SYSTEM, dwRet );
		goto ErrorExit;
	}

	// Create security descriptor to be placed on Server Side of
	// pipe.
	// Get SID of current thread
	//
	dwRet = Sm::GetThreadSID(&psidCurrentUser);
	if( ERROR_SUCCESS != dwRet )
	{
		SNI_SET_LAST_ERROR( prot, SNIE_SYSTEM, dwRet );
		goto ErrorExit;
	}

	if( !AllocateAndInitializeSid(&WorldAuthority,
								  1,
								  SECURITY_WORLD_RID,
								  0,0,0,0,0,0,0,
								  &pSecInfo->pWorldSid) )
	{
		dwRet = GetLastError();
		SNI_SET_LAST_ERROR( prot, SNIE_SYSTEM, dwRet );
		goto ErrorExit;
	}

	AclSize = sizeof( ACL )
		+ sizeof( ACCESS_ALLOWED_ACE ) - sizeof( ULONG ) + GetLengthSid( psidCurrentUser )
		+ sizeof( ACCESS_ALLOWED_ACE ) - sizeof( ULONG ) + GetLengthSid( pSecInfo->pWorldSid );

	// Initialize ACL
	pSecInfo->pAcl = (PACL) NewNoX(gpmo) BYTE [AclSize];

	if( !pSecInfo->pAcl )
	{
		dwRet = ERROR_OUTOFMEMORY;
		SNI_SET_LAST_ERROR( prot, SNIE_SYSTEM, dwRet );
		goto ErrorExit;
	}

	if( !InitializeAcl(pSecInfo->pAcl, AclSize, ACL_REVISION) ) 
	{
		dwRet = GetLastError();
		SNI_SET_LAST_ERROR( prot, SNIE_SYSTEM, dwRet );
		goto ErrorExit;
	}

	if( !AddAccessAllowedAce(pSecInfo->pAcl,
							 ACL_REVISION,
							 FILE_CREATE_PIPE_INSTANCE,
							 psidCurrentUser) )
	{
		dwRet = GetLastError();
		SNI_SET_LAST_ERROR( prot, SNIE_SYSTEM, dwRet );
		goto ErrorExit;
	}

	if( !AddAccessAllowedAce(pSecInfo->pAcl,
							 ACL_REVISION,
							 (FILE_GENERIC_READ | FILE_GENERIC_WRITE) 
							 & ~FILE_CREATE_PIPE_INSTANCE,
							 pSecInfo->pWorldSid) )
	{
		dwRet = GetLastError();
		SNI_SET_LAST_ERROR( prot, SNIE_SYSTEM, dwRet );
		goto ErrorExit;
	}

	// Initialize the SD
	if( !InitializeSecurityDescriptor(&pSecInfo->SecDesc,
									  SECURITY_DESCRIPTOR_REVISION) )
	{
		dwRet = GetLastError();
		SNI_SET_LAST_ERROR( prot, SNIE_SYSTEM, dwRet );
		goto ErrorExit;
	}
	
	if( !SetSecurityDescriptorDacl(&pSecInfo->SecDesc, TRUE, pSecInfo->pAcl, FALSE) )
	{
		dwRet = GetLastError();
		SNI_SET_LAST_ERROR( prot, SNIE_SYSTEM, dwRet );
		goto ErrorExit;
	}

	// Setup security attribute
	pSecInfo->SecAttr.nLength              = sizeof( SECURITY_ATTRIBUTES );
	pSecInfo->SecAttr.lpSecurityDescriptor = &pSecInfo->SecDesc;
	pSecInfo->SecAttr.bInheritHandle       = FALSE;

	// Return the security info
	*ppSecInfo = pSecInfo;
	
	delete [] psidCurrentUser;
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;

ErrorExit:

	if( psidCurrentUser ) delete [] psidCurrentUser;
	if( pSecInfo ) delete pSecInfo;
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;
}

void ReleaseNpAcceptRef( __out NpAcceptStruct * pAcc )
{
	
	if( 0 == InterlockedDecrement( &pAcc->cRef ) )
	{
		DeleteCriticalSection( &pAcc->CSTerminateListener );

		if( pAcc->pOverlapped )
			SNIDeleteAccept( pAcc->pOverlapped );

		DeleteNpAcl(pAcc->pSecInfo);

		delete pAcc;
	}


}


//----------------------------------------------------------------------------
// NAME: ValidateOwner
// 
// PURPOSE: 
//	Validate whether the given owner is the account calling the function
//	(or admin group, and the latter is part of the group)
//
// RETURNS:
//	none
//
// NOTES:
//
static DWORD ValidateOwner( const PSID	pOwnerSID )
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("pOwnerSID: %p{PSID}\n"), pOwnerSID );

	HANDLE			hToken  	= NULL;
	DWORD			dwSize  	= 0;
	PTOKEN_USER		ptiUser 	= NULL;
	PSID			pAdminSID	= NULL;
	PTOKEN_GROUPS	pGroupInfo	= NULL;
	DWORD			dwRet		= ERROR_SUCCESS; 

	SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;

	dwRet = ERROR_SUCCESS;

	if( NULL == pOwnerSID )
	{
		dwRet = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

	// 1. Get the current thread's SID and compare it with pOwnerSID, 
	//    if matches return true else
	// 2. Build the well known SID for Builtin\Administrators as sometimes 
	//    its marked as the owner SID if the creator belongs to admin group.
	//    Compare it against pOwnerSID and if it matches then see if the 
	//    current thread's token has this group membership.
	//

	// Get the calling thread's access token.
	//
	if( !OpenThreadToken( GetCurrentThread (), TOKEN_QUERY, TRUE, &hToken ) )
	{
		if( ( dwRet = GetLastError () ) != ERROR_NO_TOKEN )
			goto Exit;

		// Retry against process token if no thread token exists.
		//
		if( !OpenProcessToken( GetCurrentProcess (), TOKEN_QUERY, &hToken ) )
		{
			dwRet = GetLastError();
			goto Exit;
		}
	}

	// Obtain the size of the user information in the token.
	//
	if( GetTokenInformation( hToken, TokenUser, NULL, 0, &dwSize ) )
	{
		// Call should have failed due to zero-length buffer but for some 
		// reason the call has succeeded.
		//
		dwRet = static_cast<DWORD>(ERROR_FAIL);
		goto Exit;
	} 
	else 
	{
		// Call should have failed due to zero-length buffer.
		//
		if( ( dwRet = GetLastError() ) != ERROR_INSUFFICIENT_BUFFER )
			goto Exit;
	}

	// Allocate buffer for user information in the token.
	//
	ptiUser = reinterpret_cast<PTOKEN_USER>(NewNoX(gpmo) BYTE [dwSize]);
	if( !ptiUser )
	{
		dwRet = ERROR_OUTOFMEMORY;	// return failure
		goto Exit;
	}

	// Retrieve the user information from the token.
	//
	if( !GetTokenInformation( hToken, TokenUser, ptiUser, dwSize, &dwSize ) )
	{
		dwRet = GetLastError();
		goto Exit;
	}

	if( EqualSid( pOwnerSID, ptiUser->User.Sid ) )
	{
		// Return success
		//
		BidTraceU0( SNI_BID_TRACE_ON, SNI_TAG 
			_T("Owner SID, User SID: equal\n") ); 
		dwRet = ERROR_SUCCESS;
		goto Exit;
	}

	// Create a SID for the BUILTIN\Administrators group.
	//
	if( !AllocateAndInitializeSid( &SIDAuth, 2,
								SECURITY_BUILTIN_DOMAIN_RID,
								DOMAIN_ALIAS_RID_ADMINS,
								0, 0, 0, 0, 0, 0,
								&pAdminSID ) ) 
	{
		dwRet = GetLastError();
		goto Exit;
	}

	// Compare OwnerSID with AdminSID and return if they
	// are not matching else walk through the token groups
	// to see if we belong to Builtin Admin group.
	//
	if( 0 == EqualSid( pOwnerSID, pAdminSID ) )
	{
		BidTrace0( ERROR_TAG _T("Owner SID: not Admin\n") ); 
		dwRet = ERROR_ALREADY_EXISTS;	// Not matching
		goto Exit;
	}

	// Owner SID is that of Built-in Administrators so walk
	// through the token groups to seeif we belong to that group
	//

	// Call GetTokenInformation to get the buffer size.
	//
	dwSize = 0;
	if( !GetTokenInformation( hToken, TokenGroups, NULL, 0, &dwSize ) ) 
	{
		dwRet = GetLastError();
		if( ERROR_INSUFFICIENT_BUFFER != dwRet ) 
			goto Exit;
	}

	// Allocate the buffer.
	//
	pGroupInfo = reinterpret_cast<PTOKEN_GROUPS>(NewNoX(gpmo) BYTE [dwSize]);
	if( !pGroupInfo )
	{
		dwRet = ERROR_OUTOFMEMORY;		// return failure
		goto Exit;
	}

	// Call GetTokenInformation again to get the group information.
	//
	if( !GetTokenInformation( hToken,
							  TokenGroups, 
							  pGroupInfo,
							  dwSize, 
							  &dwSize ) ) 
	{
		dwRet = GetLastError();
		goto Exit;
	}

	// Loop through the group SIDs looking for the administrator SID.
	//
	for( unsigned i = 0; i < pGroupInfo->GroupCount; i++ ) 
	{
		if( EqualSid( pAdminSID, pGroupInfo->Groups[i].Sid ) ) 
		{
			// Find out if the SID is enabled in the token.
			//
			if( pGroupInfo->Groups[i].Attributes & SE_GROUP_ENABLED )
			{
				// Return Success
				//
				BidTraceU0( SNI_BID_TRACE_ON, SNI_TAG 
					_T("Owner SID: Admin, User Token: Admin SID enabled\n") ); 
				dwRet = ERROR_SUCCESS;
				goto Exit;
			}
			// The group is either not enabled or deny only
			//
			else
			{
				BidTrace0( ERROR_TAG 
					_T("Owner SID: Admin, User Token: Admin SID not enabled or deny only\n") ); 
				dwRet = ERROR_ALREADY_EXISTS;	// return failure
				goto Exit;
			}
		}
	}

	// if it reaches here then return failure
	//
	BidTrace0( ERROR_TAG _T("Owner SID: Admin, User Token: no Admin SID\n") ); 
	dwRet = ERROR_ALREADY_EXISTS;
	

Exit:

	if( ERROR_SUCCESS != dwRet )
	{
		SNI_SET_LAST_ERROR( NP_PROV, SNIE_SYSTEM, dwRet );
	}
	
	// Free resources.
	//
	if( NULL != hToken )
		CloseHandle( hToken );

	if( NULL != ptiUser )
	{
		BYTE * pb = reinterpret_cast<BYTE *>(ptiUser); 
		delete [] pb;;
	}

	if( NULL != pAdminSID )
	    FreeSid( pAdminSID );

	if( NULL != pGroupInfo )
	{
		BYTE * pb = reinterpret_cast<BYTE *>(pGroupInfo); 
		delete [] pb;
	}

	BidTraceU1(SNI_BID_TRACE_ON, RETURN_TAG _T(" %d{WINERR}\n"), dwRet);
	return dwRet;
}


//----------------------------------------------------------------------------
// NAME: ValidateObjectOwner
// 
// PURPOSE: 
//	Validate whether the given kernel object is owned by the account
//	calling the function (or admin group, and the latter is part of the 
//	group)
//
//
// RETURNS:
//	none
//
// NOTES:
//
static DWORD ValidateObjectOwner( HANDLE			objectHandle, 
								  SE_OBJECT_TYPE	objectType )
{
	PSID					pOwnerSID	= NULL;
	PSECURITY_DESCRIPTOR	pSD			= NULL;
	DWORD					dwRet 		= ERROR_SUCCESS; 

	dwRet = ERROR_SUCCESS;

	if( ( NULL == objectHandle ) || ( INVALID_HANDLE_VALUE == objectHandle ) )
	{
		dwRet = ERROR_INVALID_PARAMETER; 
		SNI_SET_LAST_ERROR( NP_PROV, SNIE_SYSTEM, dwRet );
		return dwRet;
	}

	// Get OwnerSID
	//
	dwRet = GetSecurityInfo( objectHandle, 
							 objectType, 
							 OWNER_SECURITY_INFORMATION, 
							 &pOwnerSID, 
							 NULL, 
							 NULL, 
							 NULL, 
							 &pSD );

	if( ERROR_SUCCESS != dwRet )
	{
		SNI_SET_LAST_ERROR( NP_PROV, SNIE_SYSTEM, dwRet );
		return dwRet;
	}

	// Validate owner
	//
	dwRet = ValidateOwner( pOwnerSID );

	// Free resources.
	//
	if( NULL != pSD )
		LocalFree( pSD );

	return dwRet;
}


Np::Np( SNI_Conn * pConn, ProviderNum protToReport ) : SNI_Provider( pConn )
{
	m_Prot  = NP_PROV;

	m_hPipe = INVALID_HANDLE_VALUE;

	m_CS = NULL;	

	m_CSClose = NULL; 

	m_fWritePending = false;

	m_fClose = FALSE; 

	m_dwcHandleRef = 0; 

	m_ProtToReport = protToReport; 

	m_pSyncReadPacket = NULL; 

	BidObtainItemID2A( &m_iBidId, SNI_ID_TAG "%p{.} created by %u#{SNI_Conn}", 
		this, pConn->GetBidId() );

}


Np::~Np()
{
	Assert( m_hPipe==INVALID_HANDLE_VALUE );

	BidRecycleItemIDA( &m_iBidId, SNI_ID_TAG ); 

	if( m_CS != NULL )
		DeleteCriticalSection( &m_CS );

	if( m_CSClose != NULL )
		DeleteCriticalSection( &m_CSClose );


	Assert( !m_fWritePending );
}

DWORD Np::FInit()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	DWORD dwError = SNICritSec::Initialize(&m_CS);

	if( ERROR_SUCCESS != dwError )
	{
		goto ErrorExit; 
	}

	dwError = SNICritSec::Initialize(&m_CSClose);

	if( ERROR_SUCCESS != dwError )
	{
		goto ErrorExit; 
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;  


ErrorExit: 

	if( NULL != m_CS )
	{
		DeleteCriticalSection( &m_CS );
	}

	if( NULL != m_CSClose )
	{
		DeleteCriticalSection( &m_CSClose );
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}

//  Np::FAddHandleRef
//
//  Returns:
//		TRUE =	the handle is not (being) closed, and we successfully 
//				obtained a reference because.  
//		FALSE = we did not obtain a reference because the handle is (being)
//				closed.  
//
BOOL Np::FAddHandleRef()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	BOOL fClose = FALSE;  
	CAutoSNICritSec a_csClose(m_CSClose, SNI_AUTOCS_DO_NOT_ENTER );

	a_csClose.Enter(); 
	
	fClose = m_fClose; 

	if( !fClose )
	{
		m_dwcHandleRef++; 
	}

	a_csClose.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), !fClose);
	return !fClose; 
}

//  Np::ReleaseHandleRef
//
//	Release a reference on the pipe handle.  If it was marked for closure,
//	and we are the last user close it.  
//
void Np::ReleaseHandleRef()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	BOOL fNeedClose = FALSE;  
	CAutoSNICritSec a_csClose(m_CSClose, SNI_AUTOCS_DO_NOT_ENTER );

	a_csClose.Enter(); 

	m_dwcHandleRef--; 			
	fNeedClose = m_fClose && ( 0 == m_dwcHandleRef );  
	
	a_csClose.Leave(); 

	// If the handle was marked for closure in the meantime and we are the last
	// ones using teh handle close the handle.  
	//
	if( fNeedClose )
	{
		BidTraceU0( SNI_BID_TRACE_ON, SNI_TAG _T("Closing handle\n") ); 
		
		CloseHandle( m_hPipe );

		m_hPipe = INVALID_HANDLE_VALUE;

		Assert( !m_pSyncReadPacket ); 
	}
}


//  Np::FCloseRefHandle
//
//	Mark the pipe handle for closure to stop any new callers using it, 
//	and if noone is currently using it close it.  
//
//  Returns:
//		TRUE =	the hanlde was successfully closed.  
//		FALSE = otherwise.  
//
BOOL Np::FCloseRefHandle()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	// Protect against race conditions with functions that use the pipe handle, 
	// and are not protected by the SNI API usage requirements.  
	//
	CAutoSNICritSec a_csClose(m_CSClose, SNI_AUTOCS_DO_NOT_ENTER );
	DWORD dwcHandleRef = 0; 
	BOOL fRet = FALSE; 

	a_csClose.Enter(); 
	
	m_fClose = TRUE;  
	dwcHandleRef = m_dwcHandleRef; 

	a_csClose.Leave(); 

	// Close the handle only if no completion routine is currently using it.  
	// Otherwise, a completion routine will close it instead. 
	//
	if( 0 == dwcHandleRef )
	{
		BidTraceU0( SNI_BID_TRACE_ON, SNI_TAG _T("Closing handle\n") ); 

		fRet = CloseHandle( m_hPipe );

		m_hPipe = INVALID_HANDLE_VALUE;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), fRet);
	return fRet; 
}


DWORD Np::Initialize( PSNI_PROVIDER_INFO pInfo )
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("pInfo: %p{PSNI_PROVIDER_INFO}\n"), pInfo );

	pInfo->fInitialized = TRUE; 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;
}


DWORD Np::Terminate()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T( "\n") );

	rgProvInfo[NP_PROV].fInitialized = FALSE; 
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;
}


DWORD Np::InitializeListener( HANDLE   hSNIListener, 
							  ProviderNum prot,
							  __in_opt NpListenInfo * pNpListenInfo, 
							  __out HANDLE * pListenHandle )
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("hSNIListener: %p{HANDLE}, ")
							  _T("pNpListenInfo: %p{NpListenInfo*}, ")
							  _T("pListenHandle: %p{HANDLE*}\n"), 
					hSNIListener, pNpListenInfo, pListenHandle);
	
	DWORD dwError = ERROR_FAIL;
	NpAcceptStruct * pAcc = 0;
	bool fLocal = FALSE;

	if( !hSNIListener || !pNpListenInfo || !pListenHandle )
	{
		dwError = ERROR_INVALID_PARAMETER;
		SNI_SET_LAST_ERROR( prot, SNIE_SYSTEM, dwError );
		goto ErrorExit;
	}
		
	LPWSTR wszListenInfo = pNpListenInfo->wszPipeName;
	
	pAcc = NewNoX(gpmo) NpAcceptStruct;
	if( !pAcc )
	{
		dwError = ERROR_OUTOFMEMORY;
		SNI_SET_LAST_ERROR( prot, SNIE_SYSTEM, dwError );
		goto ErrorExit;
	}

	fLocal	= 		pNpListenInfo->fLocal;

	pAcc->hPipe         = INVALID_HANDLE_VALUE;
	pAcc->hHolderPipe    = INVALID_HANDLE_VALUE;
	pAcc->fLocal		= pNpListenInfo->fLocal;
	pAcc->fPendingAccept= FALSE;
	pAcc->pSecInfo		= NULL;
	pAcc->prot			= prot;
	pAcc->fTerminated = FALSE;
	pAcc->dwNetworkSize = 4096;
	pAcc->dwWaitTime    = 100;
	pAcc->pOverlapped   = 0;
	// One ref will be released in Np::TerminateListener and
	// the other ref will be released when the first accept
	// completes.  After that each new accept will take a ref
	// for itself.
	//
	pAcc->cRef		    = 2;
	pAcc->dwPipeMode =	PIPE_WAIT
						| PIPE_READMODE_MESSAGE
						| PIPE_TYPE_MESSAGE; 
	
	// restrict the listening pipe to accept connections only from local machine.
	if( pAcc->fLocal ) 
	{
		pAcc->dwPipeMode |= PIPE_REJECT_REMOTE_CLIENTS; 
	}

	dwError = SNICritSec::Initialize( &pAcc->CSTerminateListener );
	if( ERROR_SUCCESS != dwError )
	{
		SNI_SET_LAST_ERROR( prot, SNIE_SYSTEM, dwError );
		goto ErrorExit;
	}
	
	if( CreateNpAcl(&pAcc->pSecInfo, prot) != ERROR_SUCCESS  )
		goto ErrorExit;		// Error message should already be set

	// Let's get the passed connection_info.  Remember, it's of the form
	// "pipename", e.g. "sql\query". 
	//
	
	if( wszListenInfo[0] == 0 )
	{
		dwError = ERROR_INVALID_PARAMETER;
		SNI_SET_LAST_ERROR( prot, SNIE_5, dwError );
		goto ErrorExit;
	}

	// Verify the pipe name format.  
	//
	Assert( ARRAYSIZE(L"\\\\.\\pipe\\") <= wcslen(pNpListenInfo->wszPipeName) ); 
	if ( _wcsicmp_l(
		L"\\\\.\\pipe\\", 
		wszListenInfo, GetDefaultLocale()) )
	{
		dwError = ERROR_INVALID_PARAMETER;
		SNI_SET_LAST_ERROR( prot, SNIE_5, dwError );
		goto ErrorExit;
	}

	pAcc->wszPipeName[MAX_PATH] = L'\0';
	if( FAILED(StringCchPrintf_lW( pAcc->wszPipeName, ARRAYSIZE(pAcc->wszPipeName), L"%s", GetDefaultLocale(), wszListenInfo )))
	{
		dwError = ERROR_INSUFFICIENT_BUFFER;
		SNI_SET_LAST_ERROR( prot, SNIE_SYSTEM, dwError );
		goto ErrorExit;
	}

	// Fail if pipe is already in use
	// See SQL BU DT bugs 346389 and 346383 for explanation why this
	// check is insufficient.  
	//
	HANDLE hTmpPipe = CreateFileW( pAcc->wszPipeName,
									GENERIC_READ | GENERIC_WRITE,
									FILE_SHARE_READ,
									NULL,
									CREATE_NEW,
									FILE_ATTRIBUTE_NORMAL,
									NULL );

	if( hTmpPipe != INVALID_HANDLE_VALUE )
	{
		// Error, pipe already in use

		CloseHandle( hTmpPipe );

		dwError = ERROR_FILE_EXISTS;
		SNI_SET_LAST_ERROR( prot, SNIE_SYSTEM, dwError );
		goto ErrorExit;
	}

	//
	// Set up the pipe to accept connections
	//
	pAcc->hPipe	= CreateNamedPipeW( pAcc->wszPipeName,
									PIPE_ACCESS_DUPLEX
									| FILE_FLAG_WRITE_THROUGH
									| FILE_FLAG_OVERLAPPED,
									pAcc->dwPipeMode,
									PIPE_UNLIMITED_INSTANCES,
									pAcc->dwNetworkSize,
									pAcc->dwNetworkSize,
									0,
									&pAcc->pSecInfo->SecAttr);

	if( INVALID_HANDLE_VALUE == pAcc->hPipe )
	{
		dwError = GetLastError();
		SNI_SET_LAST_ERROR( prot, SNIE_SYSTEM, dwError );
		goto ErrorExit;
	}

	// SQL BU DT bug 346383/346389/325380: to lower the chance of named pipe
	// "name squatting" by another (malicious) process running under a 
	// different account we check the ownership of the listening named pipe 
	// we created, and fail if it does not match that of the account we are 
	// running under.  
	//
	dwError = ValidateObjectOwner( pAcc->hPipe, 
								   SE_KERNEL_OBJECT ); 

	if( ERROR_SUCCESS != dwError )
	{
		goto ErrorExit;
	}	
    
	dwError = SNIRegisterForAccept( pAcc->hPipe);
	if(ERROR_SUCCESS != dwError )
		goto ErrorExit;
	
	dwError = SNICreateAccept( hSNIListener, 
					 prot, 
					 (LPVOID)pAcc, 
					 &pAcc->pOverlapped );

	if( ERROR_SUCCESS != dwError )
		goto ErrorExit;

	// Allow clients to connect
	//
	if( !ConnectNamedPipe( pAcc->hPipe, pAcc->pOverlapped ))
	{
		dwError = GetLastError();

		if( ERROR_PIPE_CONNECTED == dwError )
		{
			// if a connection already established, then we must
			// manually post a notification to our IO completion
			// port
			//
			dwError = SNIPostQCS(pAcc->pOverlapped, 0);
			if( ERROR_SUCCESS != dwError )
				goto ErrorExit;
		}
		else if( ERROR_IO_PENDING != dwError)
		{
			SNI_SET_LAST_ERROR( prot, SNIE_SYSTEM, dwError );
			goto ErrorExit;
		}
	}

	// create a pipe handle to prevent the pipe from being created by unauthorized ACLs.
	// this handle will be opened until TerminateListener
	pAcc->hHolderPipe	= CreateNamedPipeW( pAcc->wszPipeName,
									PIPE_ACCESS_DUPLEX
									| FILE_FLAG_WRITE_THROUGH
									| FILE_FLAG_OVERLAPPED,
									pAcc->dwPipeMode,
									PIPE_UNLIMITED_INSTANCES,
									pAcc->dwNetworkSize,
									pAcc->dwNetworkSize,
									0,
									&pAcc->pSecInfo->SecAttr);

	if( INVALID_HANDLE_VALUE == pAcc->hHolderPipe )
	{
		dwError = GetLastError();
		SNI_SET_LAST_ERROR( prot, SNIE_SYSTEM, dwError );
		goto ErrorExit;
	}

	// disconnect the pipe so that clients won't be connected against to this pipe.
	if( !DisconnectNamedPipe( pAcc->hHolderPipe ) )
	{

		dwError = GetLastError();
		SNI_SET_LAST_ERROR( prot, SNIE_SYSTEM, dwError );
		goto ErrorExit;

	}

	
	*pListenHandle = pAcc;

	scierrlog( ( fLocal ? 26048 : 26028 ), pAcc->wszPipeName);
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);


	return ERROR_SUCCESS;

ErrorExit:

	if( pAcc )
	{
		if( INVALID_HANDLE_VALUE != pAcc->hPipe )
			CloseHandle( pAcc->hPipe );

		if( INVALID_HANDLE_VALUE != pAcc->hHolderPipe )
			CloseHandle( pAcc->hHolderPipe );

		if( pAcc->pOverlapped )
			SNIDeleteAccept( pAcc->pOverlapped );
		
		DeleteCriticalSection( &pAcc->CSTerminateListener );
		
		DeleteNpAcl(pAcc->pSecInfo);
		
		delete pAcc;
	}


	*pListenHandle = NULL;
	if(pNpListenInfo )
		scierrlog( ( fLocal ? 26049 : 26029 ), pNpListenInfo->wszPipeName,dwError);
	else
		scierrlog( ( fLocal ? 26049 : 26029 ), "", dwError);
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}

DWORD Np::TerminateListener( __inout HANDLE hListener )
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "hListener: %p{HANDLE}\n"), hListener);

	NpAcceptStruct *pAcc = (NpAcceptStruct *) hListener;

	Assert( pAcc );
	
	CAutoSNICritSec a_csTerminateListener( pAcc->CSTerminateListener, SNI_AUTOCS_DO_NOT_ENTER );

	a_csTerminateListener.Enter(); 

	if( INVALID_HANDLE_VALUE != pAcc->hPipe )
	{
		CloseHandle( pAcc->hPipe );
		pAcc->hPipe = INVALID_HANDLE_VALUE;
	}

	if( INVALID_HANDLE_VALUE != pAcc->hHolderPipe )
	{
		CloseHandle( pAcc->hHolderPipe );
		pAcc->hHolderPipe = INVALID_HANDLE_VALUE;
	}

	pAcc->fTerminated = TRUE;
	a_csTerminateListener.Leave(); 

	ReleaseNpAcceptRef( pAcc );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;
}

DWORD Np::ResumePendingAccepts( __inout HANDLE hListener )
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "hListener: %p{HANDLE}\n"), hListener);

	DWORD dwError = ERROR_SUCCESS;
	NpAcceptStruct * pAcc = static_cast<NpAcceptStruct *>(hListener);


	if (pAcc->fPendingAccept)
	{

		// This ref will be released either in AcceptDone
		// or below if AsyncAccept fails
		//
		InterlockedIncrement( &pAcc->cRef );

		dwError = Np::AsyncAccept ( pAcc );

		// Log if we successfully reestablished listening
		//
		if (ERROR_SUCCESS == dwError)
		{
			scierrlog( ( pAcc->fLocal ? 26051 : 26045), pAcc->wszPipeName);

		}
		else
		{
			ReleaseNpAcceptRef( pAcc );
		}
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);

	return dwError;
}

DWORD Np::PrepareForNextAccept( __inout NpAcceptStruct * pAcc)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "pAcc: %p{NpAcceptStruct*}\n"), pAcc);

	DWORD dwError=ERROR_FAIL;
	CAutoSNICritSec a_csTerminateListener( pAcc->CSTerminateListener, SNI_AUTOCS_DO_NOT_ENTER );

	a_csTerminateListener.Enter(); 

	if( !pAcc->fTerminated )
	{
		dwError = Np::AsyncAccept ( pAcc );
	}
	
	a_csTerminateListener.Leave(); 

	if ( ERROR_SUCCESS != dwError )
	{
		ReleaseNpAcceptRef ( pAcc);

	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	return dwError;


}



DWORD Np::OpenPipe( __in LPWSTR wszPipeName, __in DWORD dwTimeout )
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T( "wszPipeName: '%s', ")
							  _T("dwTimeout: %d\n"), 
							  GetBidId(),
							  wszPipeName,
							  dwTimeout);
	
	DWORD dwRet = ERROR_SUCCESS;
	DWORD dwStart;
	DWORD dwTimeElapsed;
	WCHAR * pwSrvStart = wszPipeName; 
	WCHAR * pwSrvEnd = NULL; 
	
	// Verify the pipe name format.  
	//


	// Skip over first two backslashes
	//
	if( L'\\' != *pwSrvStart )
	{
		dwRet = ERROR_INVALID_PARAMETER;
		SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_5, dwRet );
		goto Exit;
	}

	pwSrvStart++;

	if( L'\\' != *pwSrvStart )
	{
		dwRet = ERROR_INVALID_PARAMETER;
		SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_5, dwRet );
		goto Exit;
	}

	pwSrvStart++;
	
	// Skip over the server name.  
	//
	pwSrvEnd = StrChrW_SYS(pwSrvStart, (int) wcslen(pwSrvStart),L'\\');

	if( !pwSrvEnd )
	{
		dwRet = ERROR_INVALID_PARAMETER;
		SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_5, dwRet );
		goto Exit;
	}

    pwSrvEnd++;

	if ( _wcsnicmp_l(
		L"pipe\\", 
		pwSrvEnd, 
		ARRAYSIZE(L"pipe\\") - 1 , GetDefaultLocale()) )
	{
		dwRet = ERROR_INVALID_PARAMETER;
		SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_5, dwRet );
		goto Exit;
	}

	
	dwStart = GetTickCount();
	
	dwTimeElapsed = 0;

	//when the dwTimeout is 0 we want to try connecting at least once
	
	while( TRUE )
	{
		// Use overlapped I/O on non-Win9x only
		//
		if( !gfIsWin9x )
		{
    		m_hPipe = CreateFileW( wszPipeName,
							       GENERIC_READ
							       | GENERIC_WRITE,
							       0,
							       NULL,
							       OPEN_EXISTING,
							       FILE_ATTRIBUTE_NORMAL
							       | FILE_FLAG_OVERLAPPED,
							       NULL );
		}
		else
		{
    		m_hPipe = CreateFileW( wszPipeName,
							       GENERIC_READ
							       | GENERIC_WRITE,
							       0,
							       NULL,
							       OPEN_EXISTING,
							       FILE_ATTRIBUTE_NORMAL,
							       NULL );

		}

		if( INVALID_HANDLE_VALUE == m_hPipe )
		{
			DWORD dwError;
			
			dwError = GetLastError();

			if( dwError != ERROR_PIPE_BUSY )
			{
				SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_40, dwError );
				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
				return dwError;
			}

			if( !WaitNamedPipe( wszPipeName, dwTimeout-dwTimeElapsed) )
			{
				dwError = GetLastError();
				SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_40, dwError );
				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
				return dwError;
			}

			dwTimeElapsed = GetTickCount()-dwStart;

			if( dwTimeout < dwTimeElapsed )
			{
				SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_40, dwError );
				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
				return ERROR_TIMEOUT;
			}
		}
		else
		{
			break;
		}

	}

	Assert( m_hPipe != INVALID_HANDLE_VALUE );

	DWORD  dwMode;

	dwMode = PIPE_READMODE_MESSAGE | PIPE_WAIT;

	if( SetNamedPipeHandleState( m_hPipe,
								&dwMode,
								NULL,
								NULL)   == FALSE)
	{
		DWORD dwError;
		dwError = GetLastError();
		SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_SYSTEM, dwError );

		CloseHandle( m_hPipe );

		m_hPipe = INVALID_HANDLE_VALUE;

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
		return dwError;
	}


Exit: 
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;
}

DWORD Np::Open( 	SNI_Conn 		* pConn,
					__in ProtElem 		* pProtElem, 
					__out SNI_Provider 	** ppProv )
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T( "pConn: %p{SNI_Conn*}, pProtElem: %p{ProtElem*}, ppProv: %p{SNI_Provider**}\n"), 
					pConn, pProtElem, ppProv);
	
	DWORD  dwError = 0;

	Np * pNp = NewNoX(gpmo) Np(pConn, (INVALID_PROV == pProtElem->m_ProviderToReport) ? NP_PROV : pProtElem->m_ProviderToReport );
	
	if( !pNp )
	{
		dwError = ERROR_OUTOFMEMORY;
		SNI_SET_LAST_ERROR( pProtElem->m_ProviderToReport, SNIE_SYSTEM, dwError );
		goto ErrorExit;
	}

	dwError = pNp->FInit();
	if( ERROR_SUCCESS != dwError )
	{
		delete pNp;
		pNp = NULL;
		SNI_SET_LAST_ERROR( pProtElem->m_ProviderToReport, SNIE_SYSTEM, dwError );
		goto ErrorExit;
	}

	Assert( wcsstr( pProtElem->Np.Pipe, L"\\\\") );

	dwError = pNp->OpenPipe( pProtElem->Np.Pipe, NP_OPEN_TIMEOUT );

	if( dwError != ERROR_SUCCESS )
		goto ErrorExit;
	
	BidUpdateItemIDA( pNp->GetBidIdPtr(), SNI_ID_TAG 
		"connection: %p{ProtElem}", pProtElem );  

	BidTraceU2( SNI_BID_TRACE_ON, SNI_TAG _T("%u#{Np}, ")
										  _T("handle: %p{HANDLE}\n"), 
										  pNp->GetBidId(), 
										  pNp->m_hPipe ); 

	// Set the out provider param to point to the new Named-Pipe object
	//
	*ppProv = pNp;
	
	// Set the handle in the base provider class to point to the Pipe Handle
	//
	(*ppProv)->m_hNwk = (HANDLE) pNp->m_hPipe;

	dwError = pNp->DWSetSkipCompletionPortOnSuccess();
	if( ERROR_SUCCESS != dwError )
		goto ErrorExit;
		
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;

ErrorExit:

	if( pNp && (INVALID_HANDLE_VALUE != pNp->m_hPipe) )
	{
		CloseHandle( pNp->m_hPipe );
		pNp->m_hPipe = INVALID_HANDLE_VALUE;
	}

	if( pNp )
		delete pNp;

	*ppProv = NULL;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;

}

DWORD Np::CheckConnection( )
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#, "), 
							  GetBidId());

	DWORD      dwError = ERROR_FAIL;

	if( !PeekNamedPipe ( m_hPipe,
								NULL,
								0,
								NULL,
								NULL,
								NULL) )
	{
		dwError = GetLastError();
		SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_SYSTEM, dwError );
		BidTrace1( ERROR_TAG _T("PeekNamedPipe: %u{WINERR}\n"), dwError);
	}
	else
	{
		dwError = ERROR_SUCCESS;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%u{WINERR}\n"), dwError);
	return dwError;
}

// Sync functions
//
DWORD Np::ReadSync( __out SNI_Packet ** ppNewPacket, 
					int           iTimeOut )
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T( "ppNewPacket: %p{SNI_Packet**}, ")
							  _T("iTimeOut: %d\n"), 
							  GetBidId(),
							  ppNewPacket, 
							  iTimeOut);

	DWORD        dwError = ERROR_SUCCESS;
	SNI_Packet  * pPacket = NULL;
	DWORD        dwBytesRead   = 0;
		
	*ppNewPacket = NULL;
	
	if( !gfIsWin9x )
	{
		LPOVERLAPPED pOverLapped = 0;

		// Loop till we have valid data and break out
		while(1)
		{
			// Check if there is no previous read pending
			if( !m_pSyncReadPacket )
			{
				// Allocate a new packet
				pPacket = SNIPacketAllocate( m_pConn, SNI_Packet_Read );
				if( !pPacket )
				{
					dwError = ERROR_OUTOFMEMORY;
					SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_SYSTEM, dwError );
					goto ErrorExit;
				}

				Assert( pPacket->m_OffSet == 0);

				PrepareForSyncCall(pPacket);
				
				// Get the exact data pointer and buffer size for this packet
				LPVOID	pBuffer       = (char *)SNIPacketGetBufPtr( pPacket );
				DWORD	dwBufferSize  = SNIPacketGetBufActualSize ( pPacket );

				pOverLapped   = SNIPacketOverlappedStruct ( pPacket );
				dwBytesRead   = 0;
			
				pOverLapped->Offset = 0;
				pOverLapped->OffsetHigh = 0;

				// Queue read to IO completion port.
				//
				BOOL fReturn = ReadFile( m_hPipe,
										pBuffer,
										dwBufferSize,
										&dwBytesRead,
										pOverLapped );
				
				if( !fReturn )
				{
					dwError = GetLastError();
					if( ERROR_IO_PENDING != dwError )
					{
						BidTrace1( ERROR_TAG _T("ReadFile: %d{WINERR}\n"), dwError );
					}

					Assert( ERROR_SUCCESS != dwError );
					
					if( ERROR_MORE_DATA == dwError )
					{
						BidTrace0( ERROR_TAG _T("ReadFile returned ERROR_MORE_DATA.\n" ));
						if( !GetOverlappedResult(m_hPipe, pOverLapped, &dwBytesRead, TRUE ))
						{
							dwError = GetLastError();

							if( ERROR_MORE_DATA != dwError )
							{
								SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_SYSTEM, dwError );
								goto ErrorExit;
							}
						}
					}
					else if( ERROR_IO_PENDING != dwError )
					{
						SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_SYSTEM, dwError );
						goto ErrorExit;
					}
				}
			}

			// There is an earlier read posted
			else
			{
				pPacket = m_pSyncReadPacket;
				pOverLapped = SNIPacketOverlappedStruct(pPacket);
				m_pSyncReadPacket = 0;

				dwError = ERROR_IO_PENDING;
			}

			Assert(pOverLapped);

			if( ERROR_IO_PENDING == dwError )
			{
				DWORD dwStart = GetTickCount();
				
				dwError = WaitForSingleObject( pOverLapped->hEvent, iTimeOut );

				if( WAIT_FAILED == dwError )
				{
					SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_SYSTEM, dwError );
					goto ErrorExit;
				}
				// Timeout
				else if( dwError == WAIT_TIMEOUT )
				{
					m_pSyncReadPacket = pPacket;
					SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_11, dwError );
					goto ErrorExit;	
				}
				// Valid data
				else if( !GetOverlappedResult(m_hPipe, pOverLapped, &dwBytesRead, FALSE ))
				{
					dwError = GetLastError();

					// Check to see if this was caused because the thread which posted
					// the earlier read exited - in that case repost the read
					if( ERROR_OPERATION_ABORTED == dwError )
					{
						SNIPacketRelease(pPacket);
						pPacket = m_pSyncReadPacket = 0;
						dwBytesRead = 0;
						dwError = ERROR_SUCCESS;

						// Adjust the timeout to exclude the time spent waiting
						if(INFINITE != iTimeOut )
						{
							iTimeOut -= (GetTickCount() - dwStart);

							// If we have timeed out, exit
							if( iTimeOut <= 0 )
							{
								dwError = WAIT_TIMEOUT;
								SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_11, dwError);
								goto ErrorExit;	
							}
						}

						// Do this all over again - allocate the packet, post the read, etc.
						continue;
					}
					else if( ERROR_MORE_DATA != dwError )
					{
						SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_SYSTEM, dwError );
						goto ErrorExit;
					}
				}
			}

			// Read completed - exit the loop
			break;
		}

		Assert( !m_pSyncReadPacket );
	}	// non-Win9x
	else
	{
		// Allocate a new packet
		pPacket = SNIPacketAllocate( m_pConn, SNI_Packet_Read );
		if( !pPacket )
		{
			dwError = ERROR_OUTOFMEMORY;
			SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_SYSTEM, dwError );
			goto ErrorExit;
		}
		
		// Get the exact data pointer and buffer size for this packet
		//
		LPVOID       pBuffer       = (char *)SNIPacketGetBufPtr( pPacket );
		DWORD        dwBufferSize  = SNIPacketGetBufActualSize ( pPacket );
		LPOVERLAPPED pOverLapped   = SNIPacketOverlappedStruct ( pPacket );
	
		// Win9x
		//
		// If time-out is specified, poll the pipe until data are 
		// available or time-out occurs.  
		//
		if( INFINITE != iTimeOut )
		{
			dwError = Win9xWaitForData( iTimeOut ); 

			if ( ERROR_SUCCESS != dwError )
				goto ErrorExit; 
		}

		// Now perform synchronous read from Named-Pipe
		//
		BOOL fReturn = ReadFile( m_hPipe,
								 pBuffer,
								 dwBufferSize,
								 &dwBytesRead,
								 NULL );

		if( !fReturn )
		{
			dwError = GetLastError();

			Assert( ERROR_SUCCESS != dwError );

			if( dwError != ERROR_MORE_DATA )
			{
				SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_10, dwError );
				goto ErrorExit;
			}
		}
	}	// Win9x

	SNIPacketSetBufferSize( pPacket, dwBytesRead );

	*ppNewPacket = pPacket;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;

ErrorExit:

	if( !gfIsWin9x )
	{
		// If its any error other than timeout - release the packet
		if( (WAIT_TIMEOUT != dwError) && pPacket )
		{
			Assert( !m_pSyncReadPacket );
			
			SNIPacketRelease( pPacket );
			m_pSyncReadPacket = 0;
		}
	}
	else
	{
		if( pPacket )
			SNIPacketRelease( pPacket );		
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}


DWORD Np::WriteSync( SNI_Packet   * pPacket, 
					 SNI_ProvInfo * pInfo )
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("pInfo: %p{SNI_ProvInfo*}\n"), 
							  GetBidId(),
							  pPacket, 
							  pInfo);

	BOOL         fReturnValue;
	LPVOID       pBuffer        = NULL;
	DWORD        dwBufferSize   = 0;
	LPOVERLAPPED pOverLapped    = SNIPacketOverlappedStruct ( pPacket );
	DWORD        dwBytesWritten = 0;
	DWORD        dwError  = 0;

	SNIPacketGetData( pPacket, 
					  (BYTE **)&pBuffer, 
					  &dwBufferSize );

	// When writing to a named pipe on Win9x, the pointer to
	// the overlapped structure must be NULL.  
	//
	if( !gfIsWin9x )
	{
		PrepareForSyncCall(pPacket);
		
		// If the SNI Consumer (or SNI internally, though it currently doesn't do this directly...) is using both Async and Sync writes,
		// it must ensure that it has no pending Writes at the time it calls SNIWriteSync
		
		// Although these are normally checked under the CritSec, it shouldn't be necessary, since per contract there should be no 
		// chance of ---- here.
		Assert( !m_fWritePending );
		Assert( m_WritePacketQueue.IsEmpty() );
		
		pOverLapped->Offset = 0;
		pOverLapped->OffsetHigh = 0;
		
		// Perform write to Named-Pipe using
		// overlapped structure.
		//
		fReturnValue = WriteFile( m_hPipe,
								  pBuffer,
								  dwBufferSize,
								  &dwBytesWritten,
								  pOverLapped );

		if( !fReturnValue )
		{
			dwError = GetLastError();

			if( dwError != ERROR_IO_PENDING )
			{
				SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_SYSTEM, dwError );
				goto ErrorExit;
			}

			dwError = WaitForSingleObject( pOverLapped->hEvent, INFINITE );

			if( dwError == WAIT_FAILED ||  dwError == WAIT_TIMEOUT )
			{
				SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_SYSTEM, dwError );
				goto ErrorExit;
			}

			if( !GetOverlappedResult(m_hPipe, pOverLapped, &dwBytesWritten, FALSE) )
			{
				dwError = GetLastError();
				SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_SYSTEM, dwError );
				goto ErrorExit;

			}
		}
	}	// non-Win9x
	else
	{
		// On Win9x, perform synchronous write to Named-Pipe
		//
		fReturnValue = WriteFile( m_hPipe,
								  pBuffer,
								  dwBufferSize,
								  &dwBytesWritten,
								  NULL );

		if( !fReturnValue )
		{
			dwError = GetLastError();

			SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_10, dwError );
			goto ErrorExit;
		}
	}	// Win9x

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;

ErrorExit:

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}


// Async functions
//
DWORD Np::ReadAsync( __out SNI_Packet ** ppNewPacket, 
					 LPVOID        pPacketKey )
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("ppNewPacket: %p{SNI_Packet**}, ")
							  _T("pPacketKey: %p\n"), 
							  GetBidId(),
							  ppNewPacket, 
							  pPacketKey);

	DWORD dwError;

	// Allocate a new packet
	//
	SNI_Packet  * pPacket = SNIPacketAllocate( m_pConn, SNI_Packet_Read );
	if( !pPacket )
	{
		dwError = ERROR_OUTOFMEMORY;
		SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_SYSTEM, dwError );
		goto Exit;
	}

	// Set the packet key
	//
	SNIPacketSetKey( pPacket, pPacketKey );

	*ppNewPacket = NULL;
	
	dwError = PostReadAsync(pPacket);

	if( ERROR_SUCCESS == dwError )
	{
		// On success, maintain the packet refcount and return the packet
		*ppNewPacket = pPacket;
	}
	else if( ERROR_IO_PENDING == dwError )
	{
		// On pending, maintain the packet refcount but don't return the packet.
		// So, nothing to do here.
	}
	else
	{
		// On failure, release the packet refcount and don't return the packet
		SNIPacketRelease( pPacket );
	}
	
Exit:
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}

DWORD Np::PostReadAsync(SNI_Packet *pPacket)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}\n"), 
							  GetBidId(),
							  pPacket);
	
	DWORD dwError;
	
	// additional AddRef around ReadFile call protects
	// against SNIReadDone being called before ReadFile returns
	SNIPacketAddRef(pPacket);

	LPVOID pBuffer      = SNIPacketGetBufPtr( pPacket );
	DWORD dwBufferSize = SNIPacketGetBufActualSize(pPacket) - SNIPacketGetBufferSize(pPacket);
	OVERLAPPED *pOverlapped = SNIPacketOverlappedStruct(pPacket);
	pOverlapped->Offset = 0;
	pOverlapped->OffsetHigh = 0;
	
	PrepareForAsyncCall(pPacket);
	
	// Queue read to IO completion port.
	//
	BOOL fReturn = ReadFile( m_hPipe,
							 pBuffer,
							 dwBufferSize,
							 NULL, // lpNumberOfBytesRead - per MSDN, should be NULL for Overlapped operations
							 pOverlapped);

	// Check for error condition and
	// return appropriate response.
	//
	if( !fReturn )
	{
		dwError = GetLastError();

		SNIPacketRelease(pPacket);

		if( ERROR_IO_PENDING == dwError )
		{
			goto Exit;
		}
		else if( ERROR_MORE_DATA == dwError )
		{
			BidTrace0( ERROR_TAG _T("ReadFile returned ERROR_MORE_DATA.\n" ));
			// fall through to success case.
		}
		else
		{
			BidTrace1( ERROR_TAG _T("ReadFile: %d{WINERR}.\n"), dwError );
			SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_SYSTEM, dwError );
			goto Exit;
		}
	} 
	else 
	{
		SNIPacketRelease(pPacket);
	}

	DWORD dwBytesRead;
	// Per MSDN, we cannot trust the dwBytesRead parameter result of ReadFile for overlapped results and must call GOR.
	if( 0 == GetOverlappedResult(m_hPipe, pOverlapped, &dwBytesRead, FALSE /* bWait - the IO completed inline, so there's nothing to wait on */) )
	{
		dwError = GetLastError();
		SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_10, dwError );
	}
	else if( 0 == dwBytesRead )
	{
		// Preserving legacy behavior: we do not expect the other end of the pipe to ever pass a 0-byte write into WriteFile, and we treat the result (a successful 0-byte read) as an error.
		dwError = ERROR_PIPE_NOT_CONNECTED;
		SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_2, dwError );
		BidTraceU0( SNI_BID_TRACE_ON, SNI_TAG _T("0-byte successful Named Pipe read.\n"));
	}
	else
	{
		// Inline completion: update packet buffer size and return ERROR_SUCCESS.
		dwError = ERROR_SUCCESS;
		SNIPacketSetBufferSize(pPacket, dwBytesRead + SNIPacketGetBufferSize(pPacket));
	}

Exit:
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}

DWORD Np::PartialReadSync(SNI_Packet * pPacket, DWORD cbBytesToRead, int timeout)
{
	BidxScopeAutoSNI4( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("cbBytesToRead: %d, ")
							  _T("timeout: %d\n"), 
							  GetBidId(),
							  pPacket, 
							  cbBytesToRead, 
							  timeout);

	// The peer's consumer (TDS) shall pass one whole packet into one named pipe packet, 
	// so our side's consumer shall never need to call a partial read function.  
	// A bad client can send such data though, so we error out.  

	SNI_ASSERT_ON_INVALID_PACKET; 

	SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_15, ERROR_NOT_SUPPORTED );

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_NOT_SUPPORTED);
	
	return ERROR_NOT_SUPPORTED;
}

DWORD Np::PartialReadAsync(SNI_Packet * pPacket, DWORD cbBytesToRead, SNI_ProvInfo * pProvInfo )
{
	BidxScopeAutoSNI4( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("cbBytesToRead: %d, ")
							  _T("pInfo: %p{SNI_ProvInfo*}\n"), 
							  GetBidId(),
							  pPacket, 
							  cbBytesToRead, 
							  pProvInfo);

	// The peer's consumer (TDS) shall pass one whole packet into one named pipe packet, 
	// so our side's consumer shall never need to call a partial read function.  
	// A bad client can send such data though, so we error out.  
	
	SNI_ASSERT_ON_INVALID_PACKET; 

	SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_15, ERROR_NOT_SUPPORTED );
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_NOT_SUPPORTED);
	
	return ERROR_NOT_SUPPORTED;
}

DWORD Np::SendPacketAsync( SNI_Packet *pPacket, DWORD *pdwBytesWritten )
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("pdwBytesWritten: %p{DWORD*}\n"), 
							  GetBidId(),
							  pPacket, 
							  pdwBytesWritten );

	LPVOID       pBuffer        = NULL;
	DWORD        dwBufferSize   = 0;
	DWORD        dwError 		= ERROR_SUCCESS;

	SNIPacketGetData( pPacket, 
					  (BYTE **)&pBuffer, 
					  &dwBufferSize );

	LPOVERLAPPED pOverLapped = NULL; 

	pOverLapped = SNIPacketOverlappedStruct ( pPacket );

	pOverLapped->Offset = 0;
	pOverLapped->OffsetHigh = 0;

	// Perform write to Named-Pipe using
	// overlapped structure.
	//

	BOOL         fReturnValue;

	if( NULL != m_pSyncReadPacket )
	{
		Assert( 0 && "It is forbidden to call SNIReadAsync or SNIPartialReadAsync when there is a cached Sync Read packet.\n");
		dwError = ERROR_INVALID_STATE;
		SNI_SET_LAST_ERROR( TCP_PROV, SNIE_0, dwError);
		goto Exit;
	}

	PrepareForAsyncCall(pPacket);

	// additional AddRef around WriteFile call protects
	// against SNIWriteDone being called before WriteFile returns
	SNIPacketAddRef(pPacket);

	fReturnValue = WriteFile( m_hPipe,
							  pBuffer,
							  dwBufferSize,
							  NULL,
							  pOverLapped );

	// Check for error condition and
	// return appropriate response.
	//
	if( !fReturnValue )
	{
		dwError = GetLastError();
		
		SNIPacketRelease(pPacket);

		if( ERROR_IO_PENDING != dwError )
		{
			Assert( ERROR_SUCCESS != dwError );
			SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_SYSTEM, dwError );
		}
		goto Exit;
	}
	else
	{
		SNIPacketRelease(pPacket);
	}

	// Per MSDN, we cannot trust the dwBytesWritten parameter result of WriteFile for overlapped results and must call GOR.
	if( 0 == GetOverlappedResult(m_hPipe, pOverLapped, pdwBytesWritten, FALSE) )
	{
		dwError = GetLastError();
		SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_10, dwError );
		goto Exit;
	}

Exit:
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	return dwError;
}

//named pipe implementation has a bug where multiple async writes causes message re-ordering
//so we need to wait for completion of previous write before posting a new one

DWORD Np::WriteAsync( SNI_Packet   * pPacket, 
					  SNI_ProvInfo * pInfo )
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T("%u#, ")
							  _T("pPacket: %p{SNI_Packet*}, ")
							  _T("pInfo: %p{SNI_ProvInfo*}\n"), 
							  GetBidId(),
							  pPacket, 
							  pInfo);

	CAutoSNICritSec a_cs(m_CS, SNI_AUTOCS_DO_NOT_ENTER );

	a_cs.Enter(); 

	DWORD        dwRet;

	//if a write packet is pending for completion queue packet
	if( m_fWritePending )
	{
		dwRet = m_WritePacketQueue.EnQueue( pPacket );
		if( ERROR_SUCCESS==dwRet)
		{
			dwRet = ERROR_IO_PENDING;
		}
	}
	else
	{
		Assert( m_WritePacketQueue.IsEmpty() );

		DWORD dwBytesWritten;
		dwRet = SendPacketAsync( pPacket, &dwBytesWritten );

		if( dwRet == ERROR_IO_PENDING )
		{
			m_fWritePending = true;
		}
	}

	a_cs.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

Np * Np::AcceptConnection( SNI_Conn *pConn, __inout NpAcceptStruct *pAcc)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T( "pConn: %p{SNI_Conn*}, pAcc: %p{NpAcceptStruct*}\n"), pConn, pAcc);

	Np * pNp = 0;
	DWORD dwRet = ERROR_SUCCESS;

	Assert (pAcc);

	// Allocate and build up the connection
	//
	if( pConn )
		pNp = NewNoX(gpmo) Np(pConn, pAcc->prot);
	if( !pNp )
	{
		goto ErrorExit;
	}
	if( ERROR_SUCCESS != pNp->FInit() )
	{
		delete pNp;
		pNp = NULL;
		goto ErrorExit;
	}

	//	Named-Pipes does not use a read buffer in its accept, so we can
	//	ignore the dwBytes parameter.
	//
	pNp->m_Prot = pAcc->prot;
	pNp->m_hPipe = pAcc->hPipe;
	pAcc->hPipe = INVALID_HANDLE_VALUE;

	BidTraceU2( SNI_BID_TRACE_ON, SNI_TAG _T("%u#{Np}, ")
										  _T("m_hPipe: %p{HANDLE}\n"), 
										  pNp->GetBidId(), 
										  pNp->m_hPipe ); 

	// Set the provider handle to be the socket
	//
	pNp->m_hNwk = (HANDLE) pNp->m_hPipe;

	dwRet = pNp->DWSetSkipCompletionPortOnSuccess();
	if( ERROR_SUCCESS != dwRet )
	{
		BidTrace1( ERROR_TAG _T("Skip completion port on success: %d{WINERR}"), dwRet);
		goto ErrorExit;
	}
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("pNp: %p\n"), pNp);
	
	return pNp;
	
ErrorExit:

	if( pNp )
	{
		DisconnectNamedPipe( pNp->m_hPipe ); 
		CloseHandle( pNp->m_hPipe );
		pNp->m_hPipe = INVALID_HANDLE_VALUE;
		delete pNp;
	}
	else
	{
		DisconnectNamedPipe( pAcc->hPipe ); 
		CloseHandle( pAcc->hPipe );
		pAcc->hPipe = INVALID_HANDLE_VALUE;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("pNp: %p\n"), 0);
	
	return 0;
	
}


DWORD Np::AcceptDone( __in SNI_Conn      * pConn,
					  __inout LPVOID          pAcceptKey,
					  __in DWORD           dwBytes,
					  __in DWORD           dwError,
					  __deref_out SNI_Provider ** ppProv,
					  __deref_out LPVOID        * ppAcceptInfo )
{
	BidxScopeAutoSNI6( SNIAPI_TAG _T("pConn:%p{SNI_Conn*}, ")
							  _T("pAcceptKey:%p{NpAcceptStruct*}, ")
							  _T("dwBytes:%d, ")
							  _T("dwError:%d, ")
							  _T("ppProv:%p{SNI_Provider**}, ")
							  _T("ppAcceptInfo:%p{LPVOID*}\n"), 
								pConn, pAcceptKey, dwBytes, dwError, ppProv, ppAcceptInfo);

	NpAcceptStruct * pAcc  = (NpAcceptStruct *)pAcceptKey;

	Np 		       * pNp     = NULL;

	CAutoSNICritSec a_csTerminateListener( pAcc->CSTerminateListener, SNI_AUTOCS_DO_NOT_ENTER );

	a_csTerminateListener.Enter(); 

	*ppAcceptInfo = NULL;

	if( ( ERROR_SUCCESS != dwError ) && ( ERROR_OPERATION_ABORTED != dwError ) )
	{
		SNI_SET_LAST_ERROR( pAcc->prot, SNIE_SYSTEM, dwError );
		goto ErrorExit;
	}

	//this is the case where TerminateListener has been called
	if( INVALID_HANDLE_VALUE == pAcc->hPipe )
	{
		dwError = ERROR_FAIL;
		SNI_SET_LAST_ERROR( pAcc->prot, SNIE_13, dwError );
		goto ErrorExit;
	}

	if( ERROR_SUCCESS == dwError )
	{	
		pNp = AcceptConnection( pConn, pAcc );
	}
	else
	{
		// Under NUMA the thread that called ConnectNamedPipe() can get 
		// trimmed.  In  that case the operation completes with 
		// ERROR_OPERATION_ABORTED.  We'll repost it to keep accepting 
		// connections.  
		//
		Assert( ERROR_OPERATION_ABORTED == dwError ); 

		dwError = RepostAccept ( pAcc );
		if( ERROR_SUCCESS != dwError )
		{
			goto ErrorExit;
		}

		// Return ERROR_OPERATION_ABORTED instead of ERROR_SUCCESS so that 
		// SNIAcceptDoneRouter() does not create a new listening named pipe.  
		//
		dwError = ERROR_OPERATION_ABORTED; 
	}
	

	if( pConn && pNp )
	{
		pConn->m_pEPInfo->npInfo.wszPipeName[MAX_PATH] = L'\0';
		if(FAILED(StringCchPrintf_lW( pConn->m_pEPInfo->npInfo.wszPipeName,
					 ARRAYSIZE(pConn->m_pEPInfo->npInfo.wszPipeName),
					 L"%s", GetDefaultLocale(),
					 pAcc->wszPipeName)))
		{
			SNI_SET_LAST_ERROR( pAcc->prot, SNIE_SYSTEM, ERROR_INVALID_PARAMETER );

			DisconnectNamedPipe( pNp->m_hPipe ); 
			CloseHandle( pNp->m_hPipe );
			pNp->m_hPipe = INVALID_HANDLE_VALUE;
			delete pNp;
			pNp = NULL;
		}
	}

	*ppProv = pNp;
	
	a_csTerminateListener.Leave(); 
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
		    
ErrorExit:

	if( pNp )
	{
		DisconnectNamedPipe( pNp->m_hPipe ); 
		CloseHandle( pNp->m_hPipe );
		pNp->m_hPipe = INVALID_HANDLE_VALUE;
		delete pNp;
	}
	
	a_csTerminateListener.Leave(); 

	// Decrement the ref that was taken before we posted this accept
	//
	ReleaseNpAcceptRef( pAcc );

	*ppProv = NULL;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
	
}


DWORD Np::AsyncAccept( __inout NpAcceptStruct * pAcc )
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "pAcc: %p{NpAcceptStruct*}\n"), pAcc);

	DWORD dwError = ERROR_SUCCESS;
	
   	pAcc->hPipe = CreateNamedPipeW( pAcc->wszPipeName,
										PIPE_ACCESS_DUPLEX
										| FILE_FLAG_WRITE_THROUGH
										| FILE_FLAG_OVERLAPPED,
										pAcc->dwPipeMode,
										PIPE_UNLIMITED_INSTANCES,
										pAcc->dwNetworkSize,
										pAcc->dwNetworkSize,
										0,
										&pAcc->pSecInfo->SecAttr );

	if( INVALID_HANDLE_VALUE == pAcc->hPipe )
	{
		dwError = GetLastError();
		
		// Log that we've stopped listening on the first time we fail
		//
		if (!pAcc->fPendingAccept)
		{
			scierrlog ( ( pAcc->fLocal ? 26050 : 26044 ), pAcc->wszPipeName, dwError, 1 /*state*/);
		}

		SNI_SET_LAST_ERROR( pAcc->prot, SNIE_SYSTEM, dwError );
		goto ErrorExit;
	}

	// Associate this with the completion port
	//
	dwError = SNIRegisterForAccept(pAcc->hPipe);
	if( ERROR_SUCCESS != dwError )
	{
		if (!pAcc->fPendingAccept)
		{
			scierrlog ( ( pAcc->fLocal ? 26050 : 26044 ), pAcc->wszPipeName, dwError, 2 /*state*/);
		}

		goto ErrorExit;
	}

	// Allow clients to connect
	//
	if( !ConnectNamedPipe( pAcc->hPipe, pAcc->pOverlapped ))
	{
		dwError = GetLastError();

		if( ERROR_PIPE_CONNECTED == dwError )
		{
			// if a connection already established, then we must
			// manually post a notification to our IO completion
			// port
			//
			dwError = SNIPostQCS(pAcc->pOverlapped, 0);
			if( ERROR_SUCCESS != dwError )
			{
				if (!pAcc->fPendingAccept)
				{
					scierrlog ( ( pAcc->fLocal ? 26050 : 26044 ), 
						pAcc->wszPipeName, dwError, 3 /*state*/);
				}

				goto ErrorExit;
			}
		}
		else if(ERROR_IO_PENDING != dwError)
		{
			if (!pAcc->fPendingAccept)
			{
				scierrlog ( ( pAcc->fLocal ? 26050 : 26044 ), 
					pAcc->wszPipeName, dwError, 4 /*state*/);
			}

			SNI_SET_LAST_ERROR( pAcc->prot, SNIE_SYSTEM, dwError );
			goto ErrorExit;
		}
	}

	if (pAcc->fPendingAccept)
	{
		pAcc->fPendingAccept = FALSE;
		DecrementPendingAccepts();
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

	return ERROR_SUCCESS;

ErrorExit:

	if (!pAcc->fPendingAccept)
	{
		pAcc->fPendingAccept = TRUE;
		IncrementPendingAccepts();
	}

	if( INVALID_HANDLE_VALUE != pAcc->hPipe )
	{
		CloseHandle( pAcc->hPipe );
		pAcc->hPipe = INVALID_HANDLE_VALUE;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}

DWORD Np::RepostAccept( __inout NpAcceptStruct * pAcc )
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "pAcc: %p{NpAcceptStruct*}\n"), pAcc);

	DWORD dwError = ERROR_SUCCESS;
	
	// Allow clients to connect
	//
	if( !ConnectNamedPipe( pAcc->hPipe, pAcc->pOverlapped ))
	{
		dwError = GetLastError();

		if( ERROR_PIPE_CONNECTED == dwError )
		{
			// if a connection already established, then we must
			// manually post a notification to our IO completion
			// port
			//
			dwError = SNIPostQCS(pAcc->pOverlapped, 0);
			if( ERROR_SUCCESS != dwError )
			{
				if (!pAcc->fPendingAccept)
				{
					scierrlog ( ( pAcc->fLocal ? 26050 : 26044 ), 
						pAcc->wszPipeName, dwError, 5 /*state*/);
				}

				goto ErrorExit;
			}
		}
		else if(ERROR_IO_PENDING != dwError)
		{
			if (!pAcc->fPendingAccept)
			{
				scierrlog ( ( pAcc->fLocal ? 26050 : 26044 ), 
					pAcc->wszPipeName, dwError, 6 /*state*/);
			}

			SNI_SET_LAST_ERROR( pAcc->prot, SNIE_SYSTEM, dwError );
			goto ErrorExit;
		}
	}

	if (pAcc->fPendingAccept)
	{
		pAcc->fPendingAccept = FALSE;
		DecrementPendingAccepts();
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

	return ERROR_SUCCESS;

ErrorExit:

	if (!pAcc->fPendingAccept)
	{
		pAcc->fPendingAccept = TRUE;
		IncrementPendingAccepts();
	}

	if( INVALID_HANDLE_VALUE != pAcc->hPipe )
	{
		CloseHandle( pAcc->hPipe );
		pAcc->hPipe = INVALID_HANDLE_VALUE;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}

DWORD Np::ReadDone( __deref_inout SNI_Packet ** ppPacket, 
					__deref_out SNI_Packet ** ppLeftOver, 
					__in DWORD         dwBytes, 
					__in DWORD         dwError )
{
	BidxScopeAutoSNI5( SNIAPI_TAG _T("%u#, ")
							  _T("ppPacket: %p{SNI_Packet**}, ")
							  _T("ppLeftOver: %p{SNI_Packet**}, ")
							  _T("dwBytes: %d, ")
							  _T("dwError: %d{WINERR}\n"), 
							  GetBidId(),
							  ppPacket, 
							  ppLeftOver, 
							  dwBytes, 
							  dwError);

	*ppLeftOver = 0;

	if( dwError &&	ERROR_MORE_DATA != dwError )
	{
		SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_SYSTEM, dwError );
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
		return dwError;
	}
	
	// Check if we received a 0 byte packet - that means the connection
	// was disconnected
	//
	if( 0 == dwBytes )
	{
		// Preserving legacy behavior: we do not expect the other end of the pipe to ever pass a 0-byte write into WriteFile, and we treat the result (a successful 0-byte read) as an error.
		SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_2, ERROR_PIPE_NOT_CONNECTED );
		BidTraceU0( SNI_BID_TRACE_ON, ERROR_TAG _T("Successful 0-byte Named Pipe read: returning ERROR_PIPE_NOT_CONNECTED.\n"));
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_PIPE_NOT_CONNECTED);
		return ERROR_PIPE_NOT_CONNECTED;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	return ERROR_SUCCESS;
}

void Np::CallbackError()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	Assert( !m_fWritePending );

	//we want to post one packet at a time
	if( !m_WritePacketQueue.IsEmpty() )
	{
		SNI_Packet *pPacket;
		pPacket = (SNI_Packet *)m_WritePacketQueue.DeQueue();

		m_fWritePending = true;

		pPacket->m_OrigProv = NP_PROV;
		
		if( ERROR_SUCCESS != SNIPacketPostQCS( pPacket, 0) )
		{
			//This assertion is used to catch unexpected system call failure.
			Assert( 0 && "SNIPacketPostQCS failed\n" );
			BidTrace0(ERROR_TAG _T("SNIPacketPostQCS failed\n"));
		}
	}
}

DWORD Np::WriteDone( SNI_Packet ** ppPacket, 
					 DWORD         dwBytes, 
					 DWORD         dwError )
{
	BidxScopeAutoSNI4( SNIAPI_TAG _T("%u#, ")
							  _T("ppPacket: %p{SNI_Packet**}, ")
							  _T("dwBytes: %d, ")
							  _T("dwError: %d{WINERR}\n"), 
							  GetBidId(),
							  ppPacket, 
							  dwBytes, 
							  dwError);

	CAutoSNICritSec a_cs(m_CS, SNI_AUTOCS_DO_NOT_ENTER );

	a_cs.Enter(); 

	DWORD dwRet;

	Assert( m_fWritePending );
	m_fWritePending = false;
	
	// If its an error, return the error. Named-Pipes does not treat
	// any errors as "valid".
	//
	if( dwError )
	{
		CallbackError();

		dwRet = dwError;

		SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_SYSTEM, dwRet );
	}
	// Check if we wrote a 0 byte packet - that means the connection
	// was disconnected
	//
	else if( 0 == dwBytes )
	{
		// If there is NO error AND dwBytes is 0, this must be a PostQS packet
		// otherwise something is seriously wrong

		Assert( (*ppPacket)->m_OrigProv == NP_PROV );
		(*ppPacket)->m_OrigProv = INVALID_PROV;

		CallbackError();

		SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_2, ERROR_PIPE_NOT_CONNECTED );

		dwRet = ERROR_PIPE_NOT_CONNECTED;
	}
	else if( !m_WritePacketQueue.IsEmpty() )
	{
		SNI_Packet *pPacket;

		// If the pipe is not (being) closed get a reference on it 
		// while we are using it.  
		//
		BOOL fRefAdded = FAddHandleRef(); 
		
		if( !fRefAdded )
		{
			// The pipe is being closed, hence don't try writing into it.  
			// The write completion being processes completed successfully, 
			// hence return ERROR_SUCCESS.  
			//
			CallbackError(); 
		}
		else
		{
			DWORD dwBytesWritten = 0;
			pPacket = (SNI_Packet *)m_WritePacketQueue.Peek();

			DWORD dwSendError = SendPacketAsync( pPacket, &dwBytesWritten );

			if( ERROR_SUCCESS == dwSendError )
			{
				//successful send, so we can remove the packet from the queue.
				SNI_Packet *pPacket;
				pPacket = (SNI_Packet *)m_WritePacketQueue.DeQueue();

				// Since we previously returned Pending for this Write, consumer will expect
				// a completion for it. We need to post the QCS for it ourselves, in this case.
				m_fWritePending = true;

				pPacket->m_OrigProv = NP_PROV;
				
				if( ERROR_SUCCESS != SNIPacketPostQCS( pPacket, dwBytesWritten) )
				{
					//This assertion is used to catch unexpected system call failure.
					Assert( 0 && "SNIPacketPostQCS failed\n" );
					BidTrace0(ERROR_TAG _T("SNIPacketPostQCS failed\n"));
				}
			}
			else if( ERROR_IO_PENDING == dwSendError )
			{
				//now we can remove the packet from queue
				m_WritePacketQueue.DeQueue();

				m_fWritePending = true;
			}
			else
			{
				// Post a QCS to handle the failed write attempt, and dequeue the failed packet
				CallbackError();
			}

			// We are done using the pipe handle, hence release a ref on it.  
			// If needed this will close the handle as well.  
			//
			ReleaseHandleRef(); 
		}

		dwRet = ERROR_SUCCESS;
	}
	else
	{
		dwRet = ERROR_SUCCESS;
	}
	
	a_cs.Leave(); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}


DWORD Np::Close()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	Assert (m_pConn);
	if (!m_pConn->m_fClient) 
	{
		THREAD_PREEMPTIVE_ON_START (PWAIT_PREEMPTIVE_OS_DISCONNECTNAMEDPIPE); 
		DisconnectNamedPipe( m_hPipe ); 
		THREAD_PREEMPTIVE_ON_END; 
	}
	BOOL fRet = FCloseRefHandle();

	if( m_pSyncReadPacket && fRet )
	{
		// Make sure that the pending read I/O was completed before
		// we release the packet and its OVERLAPPED structure.  Otherwise
		// the I/O manager may write into a released packet.  
		OVERLAPPED * pOvl = SNIPacketOverlappedStruct( m_pSyncReadPacket ); 

		// Loop checking the I/O status.  
		while( TRUE )
		{
			if ((DWORD)STATUS_PENDING == (DWORD)pOvl->Internal)
			{
				// Wait with time-out in case the auto-event was consumed
				// by another thread after we checked its status (this should
				// not really happen since the access to the Np object should
				// be serialized in sync mode).  We will check the status in 
				// the next iteration.  
				DWORD dwRet = WaitForSingleObject(pOvl->hEvent, CLOSEIOWAIT);

				// Unexpected error.  We we leak the packet and associated connection
				// to prevent possible corruptions.  
				if( WAIT_FAILED == dwRet )
				{
					BidTrace1(ERROR_TAG _T("WaitForSingleObject failed:%d{WINERR}\n"), GetLastError() );		
					break; 
				}
				else if( WAIT_TIMEOUT != dwRet )
				{
					SNIPacketRelease(m_pSyncReadPacket);
					m_pSyncReadPacket = 0;
					break; 
				}
			}
			else
			{
				SNIPacketRelease(m_pSyncReadPacket);
				m_pSyncReadPacket = 0;
				break; 
			}
		}
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
		
	return ERROR_SUCCESS;
}


void Np::Release()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T( "\n") );

	delete this;
}

DWORD Np::QueryImpersonation()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );

	DWORD dwError = ((NP_PROV == m_Prot) ? ERROR_SUCCESS : ERROR_NOT_SUPPORTED);

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;
}

DWORD Np::ImpersonateChannel()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );
	
	DWORD dwRet = ERROR_SUCCESS;
	
	Assert ( INVALID_HANDLE_VALUE != m_hPipe );

	if (!ImpersonateNamedPipeClient( m_hPipe ))
	{
		dwRet = GetLastError();

		// In case windows returns us success error code, map
		// it to ERROR_FAIL.  
		//
		if( ERROR_SUCCESS == dwRet )
		{
			dwRet = ERROR_FAIL; 
		}

		SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_SYSTEM, dwRet );
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}

DWORD Np::RevertImpersonation()
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("%u#\n"), GetBidId() );
	
	DWORD dwRet = ERROR_SUCCESS;
	
	if (!RevertToSelf())
	{
		dwRet = GetLastError();

		// In case windows returns us success error code, map
		// it to ERROR_FAIL.  
		//
		if( ERROR_SUCCESS == dwRet )
		{
			dwRet = ERROR_FAIL; 
		}

		SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_SYSTEM, dwRet );
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	
	return dwRet;
}


// A helper function for read operations with time-out on Win9x 
// platform.  
// The code is adapted from MDAC 2.8 SP 1 netlibs (ntpipesc.c).  
//
DWORD Np::Win9xWaitForData( int iTimeOut )
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T("%u#, ")
							  _T( "iTimeOut: %d\n"), 
							  GetBidId(),
							  iTimeOut);
	
	DWORD dwError     = ERROR_SUCCESS;
	int   iSleepValue = 100;
	int   iIteration  = 0;

	do
	{
		DWORD dwBytesAvailable = 0;

		BOOL fReturn = PeekNamedPipe( m_hPipe,
									  NULL,	// Not intereested in reading data
									  0,
									  NULL,
									  &dwBytesAvailable,
									  NULL );

		if( !fReturn )
		{
			// Error occured
			//
			dwError = GetLastError();

			SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_10, dwError );

			goto ErrorExit;
		}
		else if( 0 != dwBytesAvailable )
		{
			// Data are available to be read.  
			//
			break; 
		}

		// No data available, check time out, and keep trying.  
		//
		iTimeOut -= iSleepValue;

		if( iTimeOut <= 0 )
		{
			dwError = WAIT_TIMEOUT;

			SNI_SET_LAST_ERROR( m_ProtToReport, SNIE_11, dwError );

			goto ErrorExit;
		}

		// For first 3 seconds, sleep for 10th of a second
		// intervals.  If nothing has happened after 3 seconds,
		// then adjust the sleep interval to 1 second.
		// This will keep network traffic down.
		//
		Sleep( iSleepValue );

		iIteration++;

		if( iIteration < 30 )
			continue;

		iSleepValue = 1000;

	} while( TRUE );


	// We got here because data are available.  
	//
	Assert( ERROR_SUCCESS == dwError ); 

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
	
	return ERROR_SUCCESS;  

ErrorExit:

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwError);
	
	return dwError;  
}

// Inform the OS not to enqueue IO Completions on successful
// overlapped reads/writes.
//
// Returns ERROR_SUCCESS if OS call succeeds, or if connection is Sync (no need to make the call)
// Returns a Windows error code otherwise.
DWORD
Np::DWSetSkipCompletionPortOnSuccess()
{
	BidxScopeAutoSNI1(SNIAPI_TAG _T("%u#\n"), GetBidId());
	DWORD dwRet = ERROR_SUCCESS;
	
	if( !m_pConn->m_fSync )
	{
		if( 0 == SetFileCompletionNotificationModes(m_hNwk, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS))
		{
			dwRet = GetLastError();
			goto Exit;
		}
	}

Exit:
	
	BidTraceU1(SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), dwRet);
	return dwRet;
}

// ----------------------------------------------------------------------------
//	1) Add the Overlapped event to the packet
//	2) Odd the overlapped's Event handle, to ensure an IOC doesn't get enqueued to the IOCP.
//		(see documentation of GetQueuedCompletionStatus
void
Np::PrepareForSyncCall(SNI_Packet *pPacket)
{
	BidxScopeAutoSNI2(SNIAPI_TAG _T("%u#, pPacket{SNI_Packet*}\n"), GetBidId(), pPacket);

	pPacket->AddOvlEvent();
	pPacket->SetOvlEventLowBit();

	BidTraceU0(SNI_BID_TRACE_ON, RETURN_TAG _T("\n"));
}

// ----------------------------------------------------------------------------
//	1) Remove the Overlapped event from the packet
void
Np::PrepareForAsyncCall(SNI_Packet *pPacket)
{
	BidxScopeAutoSNI2(SNIAPI_TAG _T("%u#, pPacket{SNI_Packet*}\n"), GetBidId(), pPacket);

	pPacket->RemoveOvlEvent();
	BidTraceU0(SNI_BID_TRACE_ON, RETURN_TAG _T("\n"));
}
// ----------------------------------------------------------------------------

